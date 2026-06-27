/**
  ******************************************************************************
  * @file    : udp_server.c
  * @brief   : W5500 UDP server state machine.
  * @author  : Euler0525
  ******************************************************************************
  * @attention
  *
  * Non-blocking, connectionless UDP server. The W5500 chip-level bring-up
  * lives in w5500_port.c and is invoked via W5500_Init(); this module only
  * opens its UDP socket and echoes any received datagram back to its sender.
  * UDP has no connection state, so once the socket is open it sits in the
  * SOCK_UDP state and exchanges datagrams directly via recvfrom()/sendto().
  *
  ******************************************************************************
  */

/* Includes ----------------------------------------------------------*/
#include "udp_server.h"
#include "w5500_port.h"
#include "main.h"
#include "w5500.h"
#include "socket.h"
#include <stdio.h>    /* printf */

/* Private defines ---------------------------------------------------*/
#define UDP_SERVER_BUFFER_SIZE    128U    /*!< Receive buffer size in bytes.       */

/* Private variables -------------------------------------------------*/

/* Working buffer for received datagrams (internal to the echo loop). */
static uint8_t rx_buffer[UDP_SERVER_BUFFER_SIZE];

/* Exported functions ------------------------------------------------*/

/**
  * @brief  Bring up the W5500 chip and open the UDP server socket.
  * @note   Calls W5500_Init() for the chip-level bring-up, then opens socket
  *         @ref UDP_SERVER_SOCKET in UDP mode on @ref UDP_SERVER_PORT.
  * @retval None
  */
void UDP_Server_Init(void)
{
  W5500_Init();

  printf("===== W5500 UDP Server init =====\r\n");

  /* Open socket 0 as a UDP socket on the local port, if not already open. */
  uint8_t state = getSn_SR(UDP_SERVER_SOCKET);
  if (state == SOCK_CLOSED)
  {
    socket(UDP_SERVER_SOCKET, Sn_MR_UDP, UDP_SERVER_PORT, 0x00);
    printf("UDP socket opened (local port %d)\r\n", UDP_SERVER_PORT);
  }
  else
  {
    printf("Socket already open (state 0x%02X)\r\n", state);
  }

  printf("===== init done =====\r\n\r\n");
}

/**
  * @brief  Run one pass of the UDP server state machine.
  * @note   Non-blocking; call it repeatedly from the main loop. In the UDP
  *         state it receives a datagram, echoes it back to its sender, and
  *         reopens the socket after a close.
  * @retval None
  */
void UDP_Server_Process(void)
{
  uint8_t  state       = getSn_SR(UDP_SERVER_SOCKET);
  uint8_t  remote_ip[4];
  uint16_t remote_port = 0U;
  int32_t  received    = 0;

  switch (state)
  {
    case SOCK_UDP:
      /* A datagram is waiting: pull it out and echo it to its sender. */
      if (getSn_RX_RSR(UDP_SERVER_SOCKET) > 0U)
      {
        received = recvfrom(UDP_SERVER_SOCKET, rx_buffer, sizeof(rx_buffer),
                            remote_ip, &remote_port);
        if (received > 0)
        {
          sendto(UDP_SERVER_SOCKET, rx_buffer, (uint16_t)received,
                 remote_ip, remote_port);
          printf("[UDP Server] RX %ld bytes from %d.%d.%d.%d:%d, echoed back\r\n",
                 (long)received,
                 remote_ip[0], remote_ip[1], remote_ip[2], remote_ip[3],
                 remote_port);
        }
      }
      break;

    case SOCK_CLOSED:
      /* Socket closed: reopen the local port. */
      printf("[UDP Server] socket closed, reopening (port %d)\r\n", UDP_SERVER_PORT);
      socket(UDP_SERVER_SOCKET, Sn_MR_UDP, UDP_SERVER_PORT, 0x00);
      break;

    default:
      break;
  }
}

/**
  * @brief  Analyze a received data buffer (application hook).
  * @param  data Pointer to the data buffer to analyze.
  * @retval None
  */
void UDP_Server_Analyze(uint8_t *data)
{
  (void)data;  /* TODO: implement received-data analysis. */
}
