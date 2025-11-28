/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_netxduo.c
  * @author  MCD Application Team
  * @brief   NetXDuo applicative file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2020-2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app_netxduo.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "nxd_ftp_server.h"
#include "nx_web_http_server.h"
#include "main.h"
#include "app_threadx.h"
#include "encoder_driver.h"
#include "motor_driver.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define HTTP_SERVER_PORT			80
#define IP_LINK_CHECK_THREAD_STACK_SIZE			1024
#define IP_LINK_CHECK_THREAD_PRIORITY			12
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TX_THREAD      NxAppThread;
NX_PACKET_POOL NxAppPool;
NX_IP          NetXDuoEthIpInstance;
/* USER CODE BEGIN PV */
TX_THREAD ipLinkCheckThread;
NX_UDP_SOCKET UDPSocket;
NX_FTP_SERVER ftpServer;
CHAR *ftpServerStack;

extern TX_SEMAPHORE sdMountDone;
extern FX_MEDIA        sdio_disk;

extern int32_t requested_position;
extern TX_MUTEX mutex_req_pos;

NX_WEB_HTTP_SERVER httpServer;
CHAR *httpServerStack;

static NX_WEB_HTTP_SERVER_MIME_MAP app_mime_maps[] =
{
  {"css", "text/css"},
  {"svg", "image/svg+xml"},
  {"png", "image/png"},
  {"jpg", "image/jpg"},
  {"ico", "image/x-icon"},
  {"js", "text/javascript"}
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static VOID nx_app_thread_entry (ULONG thread_input);
/* USER CODE BEGIN PFP */
UINT ftpLogin(struct NX_FTP_SERVER_STRUCT *ftp_server_ptr, ULONG client_ip_address, UINT client_port, CHAR *name, CHAR *password, CHAR *extra_info);
UINT ftpLogout(struct NX_FTP_SERVER_STRUCT *ftp_server_ptr, ULONG client_ip_address, UINT client_port, CHAR *name, CHAR *password, CHAR *extra_info);
UINT http_request_notify(NX_WEB_HTTP_SERVER *server_ptr, UINT request_type, CHAR *resource, NX_PACKET *packet_ptr);
VOID ipLinkCheckEntry(ULONG ini);
/* USER CODE END PFP */

/**
  * @brief  Application NetXDuo Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
  */
UINT MX_NetXDuo_Init(VOID *memory_ptr)
{
  UINT ret = NX_SUCCESS;
  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;
  CHAR *pointer;

  /* USER CODE BEGIN MX_NetXDuo_MEM_POOL */
  /* USER CODE END MX_NetXDuo_MEM_POOL */

  /* USER CODE BEGIN 0 */

  /* USER CODE END 0 */

  /* Initialize the NetXDuo system. */
  nx_system_initialize();

    /* Allocate the memory for packet_pool.  */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, NX_APP_PACKET_POOL_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

  /* Create the Packet pool to be used for packet allocation,
   * If extra NX_PACKET are to be used the NX_APP_PACKET_POOL_SIZE should be increased
   */
  ret = nx_packet_pool_create(&NxAppPool, "NetXDuo App Pool", DEFAULT_PAYLOAD_SIZE, pointer, NX_APP_PACKET_POOL_SIZE);

  if (ret != NX_SUCCESS)
  {
    return NX_POOL_ERROR;
  }

    /* Allocate the memory for Ip_Instance */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, Nx_IP_INSTANCE_THREAD_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

   /* Create the main NX_IP instance */
  ret = nx_ip_create(&NetXDuoEthIpInstance, "NetX Ip instance", NX_APP_DEFAULT_IP_ADDRESS, NX_APP_DEFAULT_NET_MASK, &NxAppPool, nx_stm32_eth_driver,
                     pointer, Nx_IP_INSTANCE_THREAD_SIZE, NX_APP_INSTANCE_PRIORITY);

  if (ret != NX_SUCCESS)
  {
    return NX_NOT_SUCCESSFUL;
  }

    /* Allocate the memory for ARP */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, DEFAULT_ARP_CACHE_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

  /* Enable the ARP protocol and provide the ARP cache size for the IP instance */

  /* USER CODE BEGIN ARP_Protocol_Initialization */

  /* USER CODE END ARP_Protocol_Initialization */

  ret = nx_arp_enable(&NetXDuoEthIpInstance, (VOID *)pointer, DEFAULT_ARP_CACHE_SIZE);

  if (ret != NX_SUCCESS)
  {
    return NX_NOT_SUCCESSFUL;
  }

  /* Enable the ICMP */

  /* USER CODE BEGIN ICMP_Protocol_Initialization */

  /* USER CODE END ICMP_Protocol_Initialization */

  ret = nx_icmp_enable(&NetXDuoEthIpInstance);

  if (ret != NX_SUCCESS)
  {
    return NX_NOT_SUCCESSFUL;
  }

  /* Enable TCP Protocol */

  /* USER CODE BEGIN TCP_Protocol_Initialization */

  /* USER CODE END TCP_Protocol_Initialization */

  ret = nx_tcp_enable(&NetXDuoEthIpInstance);

  if (ret != NX_SUCCESS)
  {
    return NX_NOT_SUCCESSFUL;
  }

  /* Enable the UDP protocol required for  DHCP communication */

  /* USER CODE BEGIN UDP_Protocol_Initialization */

  /* USER CODE END UDP_Protocol_Initialization */

  ret = nx_udp_enable(&NetXDuoEthIpInstance);

  if (ret != NX_SUCCESS)
  {
    return NX_NOT_SUCCESSFUL;
  }

   /* Allocate the memory for main thread   */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, NX_APP_THREAD_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

  /* Create the main thread */
  ret = tx_thread_create(&NxAppThread, "NetXDuo App thread", nx_app_thread_entry , 0, pointer, NX_APP_THREAD_STACK_SIZE,
                         NX_APP_THREAD_PRIORITY, NX_APP_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);

  if (ret != TX_SUCCESS)
  {
    return TX_THREAD_ERROR;
  }

  /* USER CODE BEGIN MX_NetXDuo_Init */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, 2*NX_APP_THREAD_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {

	  return TX_POOL_ERROR;
  }
  ftpServerStack = pointer;

  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, 2*NX_APP_THREAD_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {

      return TX_POOL_ERROR;
  }
  httpServerStack = pointer;

  /* Allocate the memory for IP link check thread   */
    if (tx_byte_allocate(byte_pool, (VOID **) &pointer, NX_APP_THREAD_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
    {
      return TX_POOL_ERROR;
    }

    /* Create the IP link check thread */
	ret = tx_thread_create(&ipLinkCheckThread, "IP link check thread", ipLinkCheckEntry , 0, pointer, IP_LINK_CHECK_THREAD_STACK_SIZE,
						 IP_LINK_CHECK_THREAD_PRIORITY, IP_LINK_CHECK_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);

	if (ret != TX_SUCCESS)
	{
	return TX_THREAD_ERROR;
	}


  /* USER CODE END MX_NetXDuo_Init */

  return ret;
}

/**
* @brief  Main thread entry.
* @param thread_input: ULONG user argument used by the thread entry
* @retval none
*/
static VOID nx_app_thread_entry (ULONG thread_input)
{
  /* USER CODE BEGIN Nx_App_Thread_Entry 0 */

	UINT ret;
	ULONG bytes_read;
	UCHAR data_buffer[128];
	NX_PACKET *incoming_packet;
	NX_PACKET *outcoming_packet;
	ULONG ipAddress;
	UINT localPort = 5001;
	UINT port;

	// waiting for SD card mount and then start the FTP server
	ret = tx_semaphore_get(&sdMountDone, TX_WAIT_FOREVER);
	if (ret == TX_SUCCESS)
	{
		// create the FTP server
		ret =  nx_ftp_server_create(&ftpServer, "FTP Server Instance", &NetXDuoEthIpInstance,
										  &sdio_disk, ftpServerStack, 2*NX_APP_THREAD_STACK_SIZE, &NxAppPool,
										  ftpLogin, ftpLogout);

		if (ret != TX_SUCCESS)
		{
		  printf("FTP server create error. %02X\n", ret);

		}

		// start the ftp server
		if (nx_ftp_server_start(&ftpServer) == NX_SUCCESS)
		{
			printf("FTP server started.\n");
		}

		// create webserver
		ret = nx_web_http_server_create(&httpServer, "HTTP server", &NetXDuoEthIpInstance, HTTP_SERVER_PORT,
				&sdio_disk, (VOID *) httpServerStack, 2*NX_APP_THREAD_STACK_SIZE, &NxAppPool,
				NULL, http_request_notify);

		if (ret != NX_SUCCESS)
		{
			printf("HTTP server create error. %02X\n", ret);
		}

		ret = nx_web_http_server_mime_maps_additional_set(&httpServer,&app_mime_maps[0], 6);

		if (nx_web_http_server_start(&httpServer) == NX_SUCCESS)
		{
			printf("HTTP server started.\n");
		}
	}

	// create UDP socket
	ret = nx_udp_socket_create(&NetXDuoEthIpInstance, &UDPSocket, "UDP Server Socket", NX_IP_NORMAL, NX_FRAGMENT_OKAY, NX_IP_TIME_TO_LIVE, 2);
	if (ret != NX_SUCCESS)
	{
		printf("UDP server create error. %02X\n", ret);
		while(1)
		{
			tx_thread_sleep(100);
		}
	}

	// bind the socket to the port 5000 - this is the nucleo board local port
	ret = nx_udp_socket_bind(&UDPSocket, localPort, TX_WAIT_FOREVER);
	if (ret != NX_SUCCESS)
	{
		printf("Binding error. %02X\n", ret);
		while(1)
		{
			tx_thread_sleep(100);
		}
	}
	else
	{
		printf("UDP socket listening on PORT %u.\n", localPort);
	}

	// start the loop
	while (1)
	{
		// wait until the UDP is received
		ret = nx_udp_socket_receive(&UDPSocket, &incoming_packet, NX_WAIT_FOREVER);

		if(ret != NX_SUCCESS){
			printf("Error in nx_udp_socket_receive: %u\n", ret);
		}

		printf("[UDP SOCKET] Socket received\n");

		//retrieving the position
		UCHAR *data = incoming_packet->nx_packet_prepend_ptr;
		printf("[UDP SOCKET]: Raw bytes: %02X %02X %02X %02X\n",
		       data[0], data[1], data[2], data[3]);

		uint32_t raw =
		    ((uint32_t)data[0] << 24) |
		    ((uint32_t)data[1] << 16) |
		    ((uint32_t)data[2] << 8 ) |
		    ((uint32_t)data[3]);

		/*position is signed to avoid discontinuities*/
		int32_t pos = (int32_t)raw;
		printf("[UDP SOCKET] Position requested: %d\n", pos);

		ret = tx_mutex_get(&mutex_req_pos, TX_WAIT_FOREVER);
		if(ret != TX_SUCCESS){
					printf("[UDP SOCKET]: Error in getting the mutex: %u\n", ret);
					return;
				}
		requested_position = pos;
		ret = tx_mutex_put(&mutex_req_pos);
				if(ret != TX_SUCCESS){
							printf("[UDP SOCKET]: Error in putting the mutex: %u\n", ret);
							return;
						}

		printf("[UDP SOCKET] Position updated succesfully\n");

		ret = nx_packet_release(incoming_packet);
		if(ret != NX_SUCCESS){
			printf("[UDP SOCKET] Error in nx_packet_release: %u\n", ret);
		}


	}

  /* USER CODE END Nx_App_Thread_Entry 0 */

}
/* USER CODE BEGIN 1 */
UINT ftpLogin(struct NX_FTP_SERVER_STRUCT *ftp_server_ptr, ULONG client_ip_address, UINT client_port, CHAR *name, CHAR *password, CHAR *extra_info)
{
	// we will accept all login attempts regardless the login and password
	return NX_SUCCESS;
}

UINT ftpLogout(struct NX_FTP_SERVER_STRUCT *ftp_server_ptr, ULONG client_ip_address, UINT client_port, CHAR *name, CHAR *password, CHAR *extra_info)
{
	return NX_SUCCESS;
}

UINT http_request_notify(NX_WEB_HTTP_SERVER *server_ptr,
                       UINT request_type,
                       CHAR *resource,
                       NX_PACKET *packet_ptr)
{
    NX_PARAMETER_NOT_USED(packet_ptr);
    NX_PARAMETER_NOT_USED(server_ptr);

     if (!(request_type == NX_WEB_HTTP_SERVER_GET_REQUEST || request_type == NX_WEB_HTTP_SERVER_POST_REQUEST))
    {
    	 return NX_SUCCESS;
    }

    if ((strcmp(resource, "/") == 0) || (strcmp(resource, "/index.htm") == 0))
    {
        strcpy(resource, "/index.html");
        return NX_SUCCESS;
    }


    if (memcmp(resource, "/actual", 7) == 0)
    {
    	int32_t actual_pos;
    	char  body[32];
    	UINT ret;

    	ret = encoder_driver_input(&actual_pos);
    	if(ret != TX_SUCCESS){
    		printf("[HTTP] Error in encoder driver input: %u\n", ret);
    	}

    	UINT  len = (UINT)snprintf(body, sizeof(body), "%ld\r\n", (long)actual_pos);
    	return nx_web_http_server_callback_response_send(
    	                    server_ptr,
    	                    "200 OK",
    	                    body,
    	                    NX_NULL);


    }

    if(memcmp(resource, "/requested", 10) == 0){
    	int32_t req_pos;
    	char body[32];
    	UINT ret;

    	ret = tx_mutex_get(&mutex_req_pos, TX_WAIT_FOREVER);
		if(ret != TX_SUCCESS){
			printf("[HTTP SERVER]: Error in getting the mutex: %u\n", ret);
			return;
		}

		req_pos = requested_position;

		ret = tx_mutex_put(&mutex_req_pos);
		if(ret != TX_SUCCESS){
			printf("[HTTP SERVER]: Error in putting the mutex: %u\n", ret);
			return;
		}

    	UINT  len = (UINT)snprintf(body, sizeof(body), "%ld\r\n", (long)req_pos);
		return nx_web_http_server_callback_response_send(
							server_ptr,
							"200 OK",
							body,
							NX_NULL);


    }

    if(memcmp(resource, "/pwm", 4) == 0){
    	uint16_t pwm;
    	char body[32];
    	UINT ret;

    	ret = motor_driver_get_pwm(&pwm);
    	if(ret != TX_SUCCESS){
    		printf("[HTTP] Error in motor driver get pwm: %u", ret);
    	}

    	UINT  len = (UINT)snprintf(body, sizeof(body), "%u\r\n", pwm);
		return nx_web_http_server_callback_response_send(
							server_ptr,
							"200 OK",
							body,
							NX_NULL);

    }

    return NX_SUCCESS;
}

VOID ipLinkCheckEntry(ULONG ini)
{
	while (1)
	{
		ULONG actual_status;
		UINT ret;

		ret = nx_ip_interface_status_check(&NetXDuoEthIpInstance, 0, NX_IP_LINK_ENABLED, &actual_status, 10);

		if (ret == TX_SUCCESS && actual_status == NX_IP_LINK_ENABLED)
		{
			nx_ip_driver_direct_command(&NetXDuoEthIpInstance, NX_LINK_ENABLE, &actual_status);
		}


		tx_thread_sleep(100);
	}
}
/* USER CODE END 1 */
