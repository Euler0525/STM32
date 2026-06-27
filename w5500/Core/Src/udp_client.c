/**
  ******************************************************************************
  * @file    : udp_client.c
  * @brief   : W5500 UDP client state machine.
  * @author  : Euler0525
  ******************************************************************************
  * @attention
  *
  * Non-blocking, connectionless UDP client. The W5500 chip-level bring-up
  * lives in w5500_port.c and is invoked via W5500_Init(); this module only
  * opens its UDP socket, sends a one-time greeting to the configured remote
  * server, and echoes back any datagram received from it. UDP has no
  * connection state, so the socket sits in SOCK_UDP and exchanges datagrams
  * directly via sendto()/recvfrom().
  *
  ******************************************************************************
  */

/* Includes ----------------------------------------------------------*/
#include "udp_client.h"
#include "w5500_port.h"
#include "main.h"
#include "w5500.h"
#include "socket.h"
#include <stdio.h>    /* printf */

/* Private defines ---------------------------------------------------*/
#define UDP_CLIENT_BUFFER_SIZE    128U    /*!< Receive buffer size in bytes.       */

/* Private variables -------------------------------------------------*/

/* Remote UDP server the client talks to. */
static uint8_t  remote_ip[4] = {192U, 168U, 1U, 2U};
static uint16_t remote_port  = 5002U;

/* Working buffer for received datagrams (internal to the echo loop). */
static uint8_t rx_buffer[UDP_CLIENT_BUFFER_SIZE];

/* Guards the one-time greeting so it is only sent once per socket open. */
static uint8_t greeting_sent = 0U;

/* Exported functions ------------------------------------------------*/

/**
  * @brief  Bring up the W5500 chip and open the UDP client socket.
  * @note   Calls W5500_Init() for the chip-level bring-up, then opens socket
  *         @ref UDP_CLIENT_SOCKET in UDP mode on @ref UDP_CLIENT_LOCAL_PORT.
  * @retval None
  */
void UDP_Client_Init(void)
{
  W5500_Init();

  printf("===== W5500 UDP Client init =====\r\n");

  /* Open socket 0 as a UDP socket on the local port, if not already open. */
  uint8_t state = getSn_SR(UDP_CLIENT_SOCKET);
  if (state == SOCK_CLOSED)
  {
    socket(UDP_CLIENT_SOCKET, Sn_MR_UDP, UDP_CLIENT_LOCAL_PORT, 0x00);
    printf("UDP socket opened (local port %d)\r\n", UDP_CLIENT_LOCAL_PORT);
  }
  else
  {
    printf("Socket already open (state 0x%02X)\r\n", state);
  }

  printf("===== init done =====\r\n\r\n");
}

/**
  * @brief  Run one pass of the UDP client state machine.
  * @note   Non-blocking; call it repeatedly from the main loop. On first entry
  *         to the UDP state it sends a greeting to the configured remote
  *         server, then echoes back any datagram received, and reopens the
  *         socket after a close.
  * @retval None
  */
void UDP_Client_Process(void)
{
  uint8_t  state       = getSn_SR(UDP_CLIENT_SOCKET);
  uint8_t  peer_ip[4];
  uint16_t peer_port   = 0U;
  int32_t  received    = 0;

  switch (state)
  {
    case SOCK_UDP:
      /* Send a one-time greeting to the configured remote server. */
      if (greeting_sent == 0U)
      {
        uint8_t hello[] = "UDP client hello\r\n";
        sendto(UDP_CLIENT_SOCKET, hello, (uint16_t)(sizeof(hello) - 1U),
               remote_ip, remote_port);
        printf("[UDP Client] sent greeting to %d.%d.%d.%d:%d\r\n",
               remote_ip[0], remote_ip[1], remote_ip[2], remote_ip[3], remote_port);
        greeting_sent = 1U;
      }

      /* Echo back any datagram received (from whichever peer sent it). */
      if (getSn_RX_RSR(UDP_CLIENT_SOCKET) > 0U)
      {
        received = recvfrom(UDP_CLIENT_SOCKET, rx_buffer, sizeof(rx_buffer),
                            peer_ip, &peer_port);
        if (received > 0)
        {
          sendto(UDP_CLIENT_SOCKET, rx_buffer, (uint16_t)received,
                 peer_ip, peer_port);
          printf("[UDP Client] RX %ld bytes from %d.%d.%d.%d:%d, echoed back\r\n",
                 (long)received,
                 peer_ip[0], peer_ip[1], peer_ip[2], peer_ip[3], peer_port);
        }
      }
      break;

    case SOCK_CLOSED:
      /* Socket closed: reopen the local port and re-arm the greeting. */
      printf("[UDP Client] socket closed, reopening (local port %d)\r\n",
             UDP_CLIENT_LOCAL_PORT);
      greeting_sent = 0U;
      socket(UDP_CLIENT_SOCKET, Sn_MR_UDP, UDP_CLIENT_LOCAL_PORT, 0x00);
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
void UDP_Client_Analyze(uint8_t *data)
{
  (void)data;  /* TODO: implement received-data analysis. */
}
