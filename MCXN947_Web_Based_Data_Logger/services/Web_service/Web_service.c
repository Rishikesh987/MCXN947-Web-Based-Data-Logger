/*
 * Web_service.c
 *
 *  Created on: Sep 25, 2026
 *      Author: rishi
 *
 *  Description:
 *  ------------
 *  HTTP Web Service implementation for the MCXN947 Web-Based Data Logger.
 *
 *  Runs a single FreeRTOS task (http_handler_task) that:
 *    1. Accepts TCP connections on port HTTP_SERVER_PORT (80).
 *    2. Parses the HTTP request line with sscanf (method + path).
 *    3. Routes to:
 *         GET  /api/sensor   -> JSON sensor payload (temp, pressure, timestamp)
 *         POST /api/control  -> "on" / "off" payload, updates g_web_control_active
 *         All others         -> 404 (fsdata.c static pages are served by the
 *                               NXP lwIP httpserver if it is running alongside;
 *                               this task only owns the /api/* namespace).
 *    4. Injects CORS header so browser dashboard fetches are not blocked.
 *    5. Closes the connection after each request (HTTP/1.0 style).
 *
 *  Thread safety:
 *    g_sensor_data is protected by g_sensor_mutex.
 *    g_web_control_active is a volatile uint8_t – single-byte atomic on ARM.
 */

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "Web_service.h"

#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* lwIP socket API (BSD-compatible layer) */
#include "lwip/sockets.h"
#include "lwip/sys.h"

/* NXP debug console */
#include "fsl_debug_console.h"

/* Project services */
#include "HealthMonitor_service.h"
#include "Analog_service.h"        /* extern float temp, pressure; extern rtc_time_t t; */
#include "task_config.h"
#include "httpsrv_fs.h" /* Essential structure definitions */

/* Declare NXP's auto-generated array structure symbols from your compiled file */
extern const HTTPSRV_FS_DIR_ENTRY httpsrv_fs_data[];

/*******************************************************************************
 * Compile-time Configuration
 ******************************************************************************/

/* TCP port the web service listens on – set WEB_HTTP_SERVER_PORT in task_config.h */
#define HTTP_SERVER_PORT        80

/*
 * Receive buffer: large enough to hold a full browser HTTP request header.
 * Typical GET request is < 512 bytes; POST /api/control body is ~20 bytes.
 */
#define HTTP_RX_BUF_SIZE        640

/*
 * Transmit buffer: sized for the JSON sensor payload + HTTP headers.
 * {"temp":xx.x,"pressure":xxxx.x,"timestamp":"20YY-MM-DD HH:MM:SS"} ~70 chars
 * HTTP headers add ~150 chars.
 */
#define HTTP_TX_BUF_SIZE        384

/*
 * SO_RCVTIMEO on the *listening* socket – set WEB_ACCEPT_TIMEOUT_MS in task_config.h.
 * accept() yields control every this many ms so the health-monitor ticket is refreshed.
 */
#define HTTP_ACCEPT_TIMEOUT_MS  500

/*******************************************************************************
 * Module-private Variables
 ******************************************************************************/

/* Live sensor payload - written by WebService_UpdateSensorData() */
static WebSensorData_t  g_sensor_data;

/* Protects g_sensor_data from concurrent access */
static SemaphoreHandle_t g_sensor_mutex;

/* FreeRTOS task handle & HMS registration ID */
static TaskHandle_t      Web_Handle   = NULL;
static uint8_t           Web_TaskId;

/*******************************************************************************
 * Public Control Flag (definition)
 ******************************************************************************/
volatile uint8_t g_web_control_active = 0U;

/*******************************************************************************
 * Private Helpers
 ******************************************************************************/


/**
 * @brief  Thread-safe update of the sensor snapshot served at /api/sensor.
 *
 * Reads the RTC time struct `t` from Analog_service directly so the caller
 * only needs to pass the two float readings.
 */
void WebService_UpdateSensorData(float temp_val, float press_val)
{
    if (xSemaphoreTake(g_sensor_mutex, pdMS_TO_TICKS(10)) != pdTRUE)
    {
        /* Skip this update cycle if the mutex is held by the HTTP task */
        return;
    }

    g_sensor_data.temp     = temp_val;
    g_sensor_data.pressure = press_val;

    /* Format timestamp from the live RTC struct provided by Analog_service */
    snprintf(g_sensor_data.timestamp, sizeof(g_sensor_data.timestamp),
             "20%02d-%02d-%02d %02d:%02d:%02d",
             (int)t.year, (int)t.month, (int)t.date,
             (int)t.hour, (int)t.min,   (int)t.sec);

    xSemaphoreGive(g_sensor_mutex);
}


/**
 * @brief  Send the JSON sensor payload as an HTTP 200 response.
 *
 * Copies g_sensor_data under the mutex, then formats and writes the full
 * HTTP response to conn_fd.  CORS header is always included.
 *
 * @param conn_fd  Connected client socket descriptor.
 */
static void send_sensor_json(int conn_fd)
{
    WebSensorData_t snap;
    char            tx_buf[HTTP_TX_BUF_SIZE];
    int             tx_len;

    /* --- Snapshot sensor data under mutex --- */
    if (xSemaphoreTake(g_sensor_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        snap = g_sensor_data;          /* struct copy */
        xSemaphoreGive(g_sensor_mutex);
    }
    else
    {
        /* Could not acquire in time – send zeroed data rather than block */
        memset(&snap, 0, sizeof(snap));
        PRINTF("[Web][WARN] send_sensor_json: mutex timeout, sending zeros\r\n");
    }

    /* --- Build JSON body first so we know Content-Length --- */
    int temp_whole = (int)snap.temp;
      int temp_frac  = (int)((snap.temp - temp_whole) * 10);
      if (temp_frac < 0) temp_frac = -temp_frac; /* Handle negative temperatures */

      int press_whole = (int)snap.pressure;
      int press_frac  = (int)((snap.pressure - press_whole) * 10);
      if (press_frac < 0) press_frac = -press_frac;

      /* --- Use %d instead of %.1f --- */
      char body[128];
      int  body_len = snprintf(body, sizeof(body),
          "{\"temp\":%d.%d,\"pressure\":%d.%d,\"timestamp\":\"%s\"}",
          temp_whole, temp_frac,
          press_whole, press_frac,
          snap.timestamp);

    /* --- Assemble full HTTP response --- */
    tx_len = snprintf(tx_buf, sizeof(tx_buf),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        body_len, body);

    send(conn_fd, tx_buf, (size_t)tx_len, 0);
}

/**
 * @brief  Parse the POST /api/control body and update g_web_control_active.
 *
 * Looks for the literal strings "on" or "off" anywhere inside the full
 * received HTTP request buffer (body follows the blank-line separator).
 * Uses strstr – no dynamic allocation needed.
 *
 * @param rx_buf  Full HTTP request string (null-terminated).
 * @param conn_fd Connected client socket descriptor (for sending the reply).
 */
static void handle_control_post(const char *rx_buf, int conn_fd)
{
    const char *tx_ok =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 15\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n"
        "\r\n"
        "{\"status\":\"ok\"}";

    /* Search for "on" or "off" anywhere in the request (header + body) */
    if (strstr(rx_buf, "\"on\"") != NULL || strstr(rx_buf, ":\"on\"") != NULL)
    {
        g_web_control_active = 1U;
        PRINTF("[Web] Control command: ON\r\n");
    }
    else if (strstr(rx_buf, "\"off\"") != NULL || strstr(rx_buf, ":\"off\"") != NULL)
    {
        g_web_control_active = 0U;
        PRINTF("[Web] Control command: OFF\r\n");
    }
    else
    {
        PRINTF("[Web][WARN] handle_control_post: unrecognised payload\r\n");
    }

    send(conn_fd, tx_ok, strlen(tx_ok), 0);
}


/**
 * @brief  Scans the generated httpsrv_fs_data lookup table and flushes out matching files.
 * @return true if a matching asset was discovered and served, false otherwise.
 */
static bool handle_static_file_serve(const char *requested_path, int conn_fd)
{
    int i = 0;

    /* Loop through NXP directory structure elements until hitting the mandatory null terminator */
    while (httpsrv_fs_data[i].NAME != NULL)
    {
        if (strcmp(requested_path, httpsrv_fs_data[i].NAME) == 0)
        {
            /* Flush the full hex byte array (including pre-baked HTTP headers) into the open socket line */
            send(conn_fd, httpsrv_fs_data[i].DATA, (size_t)httpsrv_fs_data[i].SIZE, 0);
            return true;
        }
        i++;
    }
    return false;
}





/**
 * @brief  Send a plain 404 response for any unrecognised path.
 *
 * Static pages (index.html, style.css) are served by the NXP lwIP
 * httpserver from fsdata.c if it runs on a different port; here we simply
 * reject unknown /api/* requests.
 *
 * @param conn_fd  Connected client socket descriptor.
 */
static void send_404(int conn_fd)
{
    const char *resp =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 9\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n"
        "\r\n"
        "Not Found";

    send(conn_fd, resp, strlen(resp), 0);
}

/*******************************************************************************
 * HTTP Handler Task
 ******************************************************************************/

/**
 * @brief  Main FreeRTOS HTTP handler task.
 *
 * Lifecycle:
 *   1. Opens a listening socket on HTTP_SERVER_PORT.
 *   2. Loops:
 *        a. accept() with timeout (refreshes health monitor if no client).
 *        b. recv() the full HTTP request into rx_buf.
 *        c. sscanf() the first line to extract method + path.
 *        d. Route to the appropriate helper.
 *        e. Close the per-connection socket.
 *
 * @param arg  Unused (NULL).
 */
static void http_handler_task(void *arg)
{
    int             listen_fd;
    int             conn_fd;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t       client_len;
    char            rx_buf[HTTP_RX_BUF_SIZE];
    int             rx_len;
    uint32_t        received_bits = 0U;

    /* SO_RCVTIMEO on the listening socket – accept() returns EAGAIN every
     * HTTP_ACCEPT_TIMEOUT_MS so we can refresh the health-monitor ticket. */
    struct timeval  accept_timeout;
    accept_timeout.tv_sec  = 0;
    accept_timeout.tv_usec = HTTP_ACCEPT_TIMEOUT_MS * 1000;

    LWIP_UNUSED_ARG(arg);

    /* ---------------------------------------------------------------
     * Create TCP listening socket
     * --------------------------------------------------------------- */
    listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_fd < 0)
    {
        PRINTF("[Web][ERROR] socket() failed\r\n");
        vTaskDelete(NULL);
        return;
    }

    /* Allow rapid rebind after a reset */
    int reuse = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    /* Timeout so accept() doesn't block the health-monitor indefinitely */
    setsockopt(listen_fd, SOL_SOCKET, SO_RCVTIMEO, &accept_timeout, sizeof(accept_timeout));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = PP_HTONS(HTTP_SERVER_PORT);

    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0)
    {
        PRINTF("[Web][ERROR] bind() failed on port %d\r\n", HTTP_SERVER_PORT);
        lwip_close(listen_fd);
        vTaskDelete(NULL);
        return;
    }

    if (listen(listen_fd, 2) != 0)
    {
        PRINTF("[Web][ERROR] listen() failed\r\n");
        lwip_close(listen_fd);
        vTaskDelete(NULL);
        return;
    }

    PRINTF("[Web] HTTP server listening on port %d\r\n", HTTP_SERVER_PORT);

    /* ---------------------------------------------------------------
     * Main accept / serve loop
     * --------------------------------------------------------------- */
    for (;;)
    {

        /* Refresh the health-monitor watchdog ticket */
        if (xTaskNotifyWait(0x00U, 0xFFFFFFFFU, &received_bits, 0) == pdPASS)
        {
            Reset_HeltMonit_service(Web_TaskId);
        }

        client_len = sizeof(client_addr);
        conn_fd    = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);

        if (conn_fd < 0)
        {
            /* accept() timed out – normal: loop back to refresh watchdog */
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                continue;
            }
            PRINTF("[Web][ERROR] accept() error: %d\r\n", errno);
            continue;
        }

        PRINTF("[Web] Client connected: %s\r\n",
               ipaddr_ntoa((const ip_addr_t *)&client_addr.sin_addr));

        /* ---------------------------------------------------------------
         * Receive the HTTP request
         * A single recv() is sufficient for requests that fit within one
         * TCP segment (typical for GET/POST from the dashboard).
         * --------------------------------------------------------------- */
        rx_len = recv(conn_fd, rx_buf, sizeof(rx_buf) - 1, 0);

        if (rx_len <= 0)
        {
            lwip_close(conn_fd);
            continue;
        }
        rx_buf[rx_len] = '\0';

        /* ---------------------------------------------------------------
         * Parse HTTP request line:  "<METHOD> <PATH> HTTP/x.x"
         * sscanf stops at whitespace so method and path are clean tokens.
         * --------------------------------------------------------------- */
        char method[8]  = {0};
        char path[64]   = {0};

        int parsed = sscanf(rx_buf, "%7s %63s", method, path);

        if (parsed != 2)
        {
            PRINTF("[Web][WARN] Malformed request line\r\n");
            send_404(conn_fd);
            lwip_close(conn_fd);
            continue;
        }

        PRINTF("[Web] %s %s\r\n", method, path);

        /* ---------------------------------------------------------------
         * Route: GET /api/sensor
         * --------------------------------------------------------------- */
        if (strncmp(path, "/api/sensor", 11) == 0)
        {
            if (strncmp(method, "GET", 3) == 0)
            {

                send_sensor_json(conn_fd);
            }
            else
            {
                /* Method not allowed – still needs CORS header for preflight */
                const char *resp405 =
                    "HTTP/1.1 405 Method Not Allowed\r\n"
                    "Access-Control-Allow-Origin: *\r\n"
                    "Content-Length: 0\r\n"
                    "Connection: close\r\n"
                    "\r\n";
                send(conn_fd, resp405, strlen(resp405), 0);
            }
        }

        /* ---------------------------------------------------------------
         * Route: POST /api/control
         * --------------------------------------------------------------- */
        else if (strncmp(path, "/api/control", 12) == 0)
        {
            if (strncmp(method, "POST", 4) == 0)
            {
                handle_control_post(rx_buf, conn_fd);
            }
            else if (strncmp(method, "OPTIONS", 7) == 0)
            {
                /* Browser CORS preflight */
                const char *preflight =
                    "HTTP/1.1 204 No Content\r\n"
                    "Access-Control-Allow-Origin: *\r\n"
                    "Access-Control-Allow-Methods: POST, OPTIONS\r\n"
                    "Access-Control-Allow-Headers: Content-Type\r\n"
                    "Content-Length: 0\r\n"
                    "Connection: close\r\n"
                    "\r\n";
                send(conn_fd, preflight, strlen(preflight), 0);
            }
            else
            {
                const char *resp405 =
                    "HTTP/1.1 405 Method Not Allowed\r\n"
                    "Access-Control-Allow-Origin: *\r\n"
                    "Content-Length: 0\r\n"
                    "Connection: close\r\n"
                    "\r\n";
                send(conn_fd, resp405, strlen(resp405), 0);
            }
        }

        /* ---------------------------------------------------------------
         * Fallback: any other path
         *
         * If the NXP lwIP httpserver (HTTPSRV) is active on a different
         * port and serves the static pages from fsdata.c, this task simply
         * returns 404 for unknown /api/* paths.  The static pages are not
         * re-served here to keep this service lightweight.
         * --------------------------------------------------------------- */
        /* ---------------------------------------------------------------
         * Fallback: Serve static dashboard assets or return 404
         * --------------------------------------------------------------- */
        else
        {
            char target_path[64];
            strncpy(target_path, path, sizeof(target_path) - 1);
            target_path[sizeof(target_path) - 1] = '\0';

            /* Rule A: Route default landing root path string '/' straight to '/index.html' */
            if (strcmp(target_path, "/") == 0)
            {
                strcpy(target_path, "/index.html");
            }

            /* Rule B: Check if requested asset lives in our compiled look-up table array */
            if (!handle_static_file_serve(target_path, conn_fd))
            {
                PRINTF("[Web] Unknown path: %s - returning 404\r\n", path);
                send_404(conn_fd);
            }
        }




        lwip_close(conn_fd);
    } /* end for(;;) */
}

/*******************************************************************************
 * Public API Implementation
 ******************************************************************************/



/**
 * @brief  Initialise the Web Service and launch the FreeRTOS handler task.
 */
void WebService_Init(void)
{
    /* Initialise the sensor data to safe defaults */
    memset(&g_sensor_data, 0, sizeof(g_sensor_data));
    snprintf(g_sensor_data.timestamp, sizeof(g_sensor_data.timestamp), "1970-01-01 00:00:00");

    /* Create the mutex that guards g_sensor_data */
    g_sensor_mutex = xSemaphoreCreateMutex();
    if (g_sensor_mutex == NULL)
    {
        PRINTF("[Web][ERROR] Failed to create sensor mutex!\r\n");
        __BKPT(0);
        return;
    }

    WebService_UpdateSensorData(99.99,999);
    /* Register with Health Monitor (uses the COMM task timeout ceiling) */
    Web_TaskId = Registor_service("http_handler", HMS_TIMEOUT_COMM_MS, true, &Web_Handle);

    /* Create the HTTP handler task */
    if (xTaskCreate(http_handler_task, "http_handler",
                    STACK_COMM_TASK, NULL,
                    PRIO_COMM_TASK,  &Web_Handle) != pdPASS)
    {
        PRINTF("[RTOS][ERROR] http_handler task creation FAILED!\r\n");
        __BKPT(0);
    }
    else
    {
        PRINTF("[RTOS][OK] http_handler task created. HMS ID: %d, Port: %d\r\n",
               Web_TaskId, HTTP_SERVER_PORT);
    }
}
