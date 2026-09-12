/*
 * ethrnet_service.c
 *
 *  Created on: Aug 30, 2026
 *      Author: rishi
 */

#include "ethrnet_service.h"
#include "My_ethrnet.h"
/*******************************************************************************
 * Includes
 ******************************************************************************/

#include "lwip/opt.h"


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>

#include "ethernetif.h"
#include "board.h"

#include "fsl_phy.h"

#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/tcpip.h"
#include "lwip/ip.h"
#include "lwip/netifapi.h"
#include "lwip/sockets.h"
#include "netif/etharp.h"

#ifndef configMAC_ADDR
#include "fsl_silicon_id.h"
#endif

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#ifndef EXAMPLE_NETIF_INIT_FN
/*! @brief Network interface initialization function. */
#define EXAMPLE_NETIF_INIT_FN ethernetif0_init
#endif /* EXAMPLE_NETIF_INIT_FN */

#ifndef TCP_SERVER_STACKSIZE
#define TCP_SERVER_STACKSIZE DEFAULT_THREAD_STACKSIZE
#endif

#ifndef TCP_SERVER_PRIORITY
#define TCP_SERVER_PRIORITY DEFAULT_THREAD_PRIO
#endif

/* Port the MCU will listen on for incoming TCP connections from the PC */
#define TCP_SERVER_PORT 5000

/* Size of the buffer used to receive data */
#define TCP_RX_BUFFER_SIZE 256

/*******************************************************************************
 * Variables
 ******************************************************************************/

static phy_handle_t phyHandle;
static struct netif netif;

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Initializes lwIP stack and brings up the Ethernet interface.
 */
static void stack_init(void)
{
#if LWIP_IPV4
    ip4_addr_t netif_ipaddr, netif_netmask, netif_gw;
#endif /* LWIP_IPV4 */
    ethernetif_config_t enet_config = {.phyHandle   = &phyHandle,
                                        .phyAddr     = EXAMPLE_PHY_ADDRESS,
                                        .phyOps      = EXAMPLE_PHY_OPS,
                                        .phyResource = EXAMPLE_PHY_RESOURCE,
                                        .srcClockHz  = EXAMPLE_CLOCK_FREQ,
#ifdef configMAC_ADDR
                                        .macAddress = configMAC_ADDR
#endif
    };

    tcpip_init(NULL, NULL);

    /* Set MAC address. */
#ifndef configMAC_ADDR
    (void)SILICONID_ConvertToMacAddr(&enet_config.macAddress);
#endif

#if LWIP_IPV4
    IP4_ADDR(&netif_ipaddr, configIP_ADDR0, configIP_ADDR1, configIP_ADDR2, configIP_ADDR3);
    IP4_ADDR(&netif_netmask, configNET_MASK0, configNET_MASK1, configNET_MASK2, configNET_MASK3);
    IP4_ADDR(&netif_gw, configGW_ADDR0, configGW_ADDR1, configGW_ADDR2, configGW_ADDR3);

    netifapi_netif_add(&netif, &netif_ipaddr, &netif_netmask, &netif_gw, &enet_config, EXAMPLE_NETIF_INIT_FN,
                        tcpip_input);
#else
    netifapi_netif_add(&netif, &enet_config, EXAMPLE_NETIF_INIT_FN, tcpip_input);
#endif /* LWIP_IPV4 */
    netifapi_netif_set_default(&netif);
    netifapi_netif_set_up(&netif);

    while (ethernetif_wait_linkup(&netif, 5000) != ERR_OK)
    {
        PRINTF("PHY Auto-negotiation failed. Please check the cable connection and link partner setting.\r\n");
    }

#if LWIP_IPV4
    PRINTF("\r\n************************************************\r\n");
    PRINTF(" TCP server listening on: %u.%u.%u.%u : %d\r\n", ((u8_t *)&netif_ipaddr)[0], ((u8_t *)&netif_ipaddr)[1],
           ((u8_t *)&netif_ipaddr)[2], ((u8_t *)&netif_ipaddr)[3], TCP_SERVER_PORT);
    PRINTF("************************************************\r\n");
#endif
}

static void tcp_server_task(void *arg)
{
    int listen_fd, conn_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    char rx_buffer[TCP_RX_BUFFER_SIZE];
    int received;

    LWIP_UNUSED_ARG(arg);

    stack_init();

    listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_fd < 0)
    {
        PRINTF("Failed to create socket.\r\n");
        vTaskDelete(NULL);
        return;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = PP_HTONS(TCP_SERVER_PORT);

    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0)
    {
        PRINTF("Bind failed.\r\n");
        lwip_close(listen_fd);
        vTaskDelete(NULL);
        return;
    }

    if (listen(listen_fd, 1) != 0)
    {
        PRINTF("Listen failed.\r\n");
        lwip_close(listen_fd);
        vTaskDelete(NULL);
        return;
    }

    PRINTF("TCP server ready, waiting for a connection...\r\n");

    for (;;)
    {
        client_len = sizeof(client_addr);
        conn_fd    = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
        if (conn_fd < 0)
        {
            PRINTF("Accept failed.\r\n");
            continue;
        }

        PRINTF("Client connected: %s\r\n", ipaddr_ntoa((const ip_addr_t *)&client_addr.sin_addr));

        while (1)
        {
            received = recv(conn_fd, rx_buffer, sizeof(rx_buffer) - 1, 0);
            if (received <= 0)
            {
                /* 0 = client closed connection, <0 = error */
                break;
            }

            rx_buffer[received] = '\0';
            PRINTF("Received %d bytes: %s\r\n", received, rx_buffer);

            /* Echo back. Replace this with your own response/handling. */
            send(conn_fd, rx_buffer, (size_t)received, 0);
        }

        PRINTF("Client disconnected.\r\n");
        lwip_close(conn_fd);
    }
}

void Init_ETH_service(void){

    Etharnet_InitPins();
    Ethrnet_MDIO_Init();

    if (xTaskCreate(tcp_server_task, "tcp_server", TCP_SERVER_STACKSIZE, NULL, TCP_SERVER_PRIORITY, NULL) != pdPASS)
       {
           PRINTF("main(): Task creation failed.", 0);
           __BKPT(0);
       }
}
