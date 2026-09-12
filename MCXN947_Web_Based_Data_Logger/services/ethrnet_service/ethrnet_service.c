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

#include "HealthMonitor_service.h"
#include "task_config.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#ifndef EXAMPLE_NETIF_INIT_FN
#define EXAMPLE_NETIF_INIT_FN ethernetif0_init
#endif

#ifndef TCP_SERVER_STACKSIZE
#define TCP_SERVER_STACKSIZE STACK_COMM_TASK
#endif

#ifndef TCP_SERVER_PRIORITY
#define TCP_SERVER_PRIORITY  PRIO_COMM_TASK
#endif

#define TCP_SERVER_PORT      5000
#define TCP_RX_BUFFER_SIZE   256

/*******************************************************************************
 * Variables
 ******************************************************************************/
TaskHandle_t TCP_Handle = NULL;
static uint8_t TCP_TaskId;
static phy_handle_t PHY_Handle;
static struct netif NetInterface;

/*******************************************************************************
 * Code
 ******************************************************************************/

static void stack_init(void) {
#if LWIP_IPV4
	ip4_addr_t netif_ipaddr, netif_netmask, netif_gw;
#endif
	ethernetif_config_t enet_config = {
        .phyHandle = &PHY_Handle,
        .phyAddr = EXAMPLE_PHY_ADDRESS,
        .phyOps = EXAMPLE_PHY_OPS,
        .phyResource = EXAMPLE_PHY_RESOURCE,
        .srcClockHz = EXAMPLE_CLOCK_FREQ,
#ifdef configMAC_ADDR
        .macAddress = configMAC_ADDR
#endif
	};

	tcpip_init(NULL, NULL);

#ifndef configMAC_ADDR
	(void) SILICONID_ConvertToMacAddr(&enet_config.macAddress);
#endif

#if LWIP_IPV4
	IP4_ADDR(&netif_ipaddr, configIP_ADDR0, configIP_ADDR1, configIP_ADDR2, configIP_ADDR3);
	IP4_ADDR(&netif_netmask, configNET_MASK0, configNET_MASK1, configNET_MASK2, configNET_MASK3);
	IP4_ADDR(&netif_gw, configGW_ADDR0, configGW_ADDR1, configGW_ADDR2, configGW_ADDR3);

	netifapi_netif_add(&NetInterface, &netif_ipaddr, &netif_netmask, &netif_gw,
			&enet_config, EXAMPLE_NETIF_INIT_FN, tcpip_input);
#else
    netifapi_netif_add(&NetInterface, &enet_config, EXAMPLE_NETIF_INIT_FN, tcpip_input);
#endif
	netifapi_netif_set_default(&NetInterface);
	netifapi_netif_set_up(&NetInterface);

	PRINTF("[ETH] Checking Ethernet Link Status...\r\n");


     /* We poll the link status iteratively rather than blocking in an intense loop.*/
	while (ethernetif_wait_linkup(&NetInterface, 100) != ERR_OK) {
		PRINTF("[ETH] Waiting for Link Connection...\r\n");
        /* Periodically inform the health monitor that we are still alive during boot routing */
        Reset_HeltMonit_service(TCP_TaskId);
        vTaskDelay(pdMS_TO_TICKS(100));
	}

#if LWIP_IPV4
	PRINTF("\r\n************************************************\r\n");
	PRINTF(" TCP server listening on: %u.%u.%u.%u : %d\r\n",
			((u8_t*) &netif_ipaddr)[0], ((u8_t*) &netif_ipaddr)[1],
			((u8_t*) &netif_ipaddr)[2], ((u8_t*) &netif_ipaddr)[3],
			TCP_SERVER_PORT);
	PRINTF("************************************************\r\n");
#endif
}

static void tcp_server_task(void *arg) {
	int listen_fd, conn_fd;
	struct sockaddr_in server_addr, client_addr;
	socklen_t client_len;
	char rx_buffer[TCP_RX_BUFFER_SIZE];
	int received;
	uint32_t receivedBits = 0;

    /* Configure a 200ms socket timeout structure */
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 200000; // 200,000 microseconds = 200ms

	LWIP_UNUSED_ARG(arg);

	stack_init();

	listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listen_fd < 0) {
		PRINTF("Failed to create socket.\r\n");
		vTaskDelete(NULL);
		return;
	}

    /* This prevents accept() from blocking indefinitely when no clients are active.*/
    setsockopt(listen_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = PP_HTONS(TCP_SERVER_PORT);

	if (bind(listen_fd, (struct sockaddr* )&server_addr, sizeof(server_addr)) != 0) {
		PRINTF("Bind failed.\r\n");
		lwip_close(listen_fd);
		vTaskDelete(NULL);
		return;
	}

	if (listen(listen_fd, 1) != 0) {
		PRINTF("Listen failed.\r\n");
		lwip_close(listen_fd);
		vTaskDelete(NULL);
		return;
	}

	PRINTF("TCP server ready, waiting for a connection...\r\n");

	for (;;) {

        if (xTaskNotifyWait(0x00, 0xFFFFFFFF, &receivedBits, 0) == pdPASS) {
            Reset_HeltMonit_service(TCP_TaskId);
        }

		client_len = sizeof(client_addr);
		conn_fd = accept(listen_fd, (struct sockaddr* )&client_addr, &client_len);

		if (conn_fd < 0) {
            /* If the error code matches EWOULDBLOCK, it means the 200ms timeout expired
             * with no connections. We loop back smoothly to refresh the watchdog. */
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                continue;
            }
			PRINTF("Accept failed.\r\n");
			continue;
		}

		PRINTF("Client connected: %s\r\n", ipaddr_ntoa((const ip_addr_t* )&client_addr.sin_addr));


        setsockopt(conn_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

		while (1) {
            /* Heartbeat update step inside the active client socket data session loop */
            if (xTaskNotifyWait(0x00, 0xFFFFFFFF, &receivedBits, 0) == pdPASS) {
                Reset_HeltMonit_service(TCP_TaskId);
            }

			received = recv(conn_fd, rx_buffer, sizeof(rx_buffer) - 1, 0);

            if (received == 0) {
				/* Client closed connection gracefully */
				break;
			}

            if (received < 0) {
                /* If recv timed out with no data, loop back up and check the monitor ticket */
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    continue;
                }
                /* Any other error breaks the loop */
                break;
            }

			rx_buffer[received] = '\0';
			PRINTF("Received %d bytes: %s\r\n", received, rx_buffer);

			/* Echo back response payload safely */
			send(conn_fd, rx_buffer, (size_t)received, 0);
		}

		PRINTF("Client disconnected.\r\n");
		lwip_close(conn_fd);
	}
}

void Init_ETH_service(void) {
	Etharnet_InitPins();
	Ethrnet_MDIO_Init();


	TCP_TaskId = Registor_service("tcp_server", HMS_TIMEOUT_COMM_MS, true, &TCP_Handle);

	if (xTaskCreate(tcp_server_task, "tcp_server", TCP_SERVER_STACKSIZE, NULL,
	                TCP_SERVER_PRIORITY, &TCP_Handle) != pdPASS) {
		PRINTF("[RTOS][ERROR] TCP Server task creation failed.\r\n");
		__BKPT(0);
	} else {
        PRINTF("[RTOS][OK] TCP Server task created successfully. HMS ID: %d\r\n", TCP_TaskId);
    }
}
