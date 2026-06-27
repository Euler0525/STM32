/**
  ******************************************************************************
  * @file    : tcp_client.c
  * @brief   : W5500 TCP client state machine.
  * @author  : Euler0525
  ******************************************************************************
  * @attention
  *
  * Non-blocking TCP client state machine. The W5500 chip-level bring-up
  * (SPI/GPIO callbacks, hardware reset, network settings, socket buffer
  * sizing) lives in w5500_port.c and is invoked via W5500_Init(); this module
  * only opens its socket and runs the connect/echo/disconnect loop.
  *
  ******************************************************************************
  */

/* Includes ----------------------------------------------------------*/
#include "tcp_client.h"
#include "w5500_port.h"
#include "main.h"
#include "w5500.h"
#include "socket.h"
#include <stdio.h>    /* printf */

/* Private defines ---------------------------------------------------*/
#define TCP_CLIENT_BUFFER_SIZE    128U    /*!< Receive buffer size in bytes.       */
#define SOCKET_STATE_UNKNOWN      0xFFU   /*!< Sentinel forcing a log on first run. */

/* Private variables -------------------------------------------------*/

/* Remote TCP server the client connects to. */
static uint8_t  remote_ip[4] = {192U, 168U, 1U, 2U};
static uint16_t remote_port  = 5002U;

/* Working buffer for received data (internal to the echo loop). */
static uint8_t  rx_buffer[TCP_CLIENT_BUFFER_SIZE];

/* Exported functions ------------------------------------------------*/

/**
  * @brief  Bring up the W5500 chip and open the TCP client socket.
  * @note   Calls W5500_Init() for the chip-level bring-up, then opens socket
  *         @ref TCP_CLIENT_SOCKET in TCP mode on @ref TCP_CLIENT_LOCAL_PORT.
  * @retval None
  */
void TCP_Client_Init(void)
{
  W5500_Init();

  printf("===== W5500 TCP Client init =====\r\n");

  /* Open socket 0 as a TCP client on the local port, if not already open. */
  uint8_t state = getSn_SR(TCP_CLIENT_SOCKET);
  if (state == SOCK_CLOSED)
  {
    socket(TCP_CLIENT_SOCKET, Sn_MR_TCP, TCP_CLIENT_LOCAL_PORT, 0x00);
    printf("TCP socket opened (local port %d)\r\n", TCP_CLIENT_LOCAL_PORT);
  }
  else
  {
    printf("Socket already open (state 0x%02X)\r\n", state);
  }

  printf("===== init done =====\r\n\r\n");
}

/**
  * @brief  Run one pass of the TCP client state machine.
  * @note   Non-blocking; call it repeatedly from the main loop. It connects
  *         when the socket becomes ready, echoes back any data received from
  *         the server, disconnects on CLOSE_WAIT and reopens the socket after
  *         a close. State changes are logged once to avoid spamming.
  * @retval None
  */
void TCP_Client_Process(void)
{
  static uint8_t last_state = SOCKET_STATE_UNKNOWN;
  uint8_t  state   = getSn_SR(TCP_CLIENT_SOCKET);
  uint16_t length  = 0U;
  uint8_t  changed = (state != last_state);  /* act once per state transition */

  switch (state)
  {
    case SOCK_INIT:
      /* Socket opened: trigger a single connection attempt. */
      if (changed != 0U)
      {
        printf("[TCP] connecting to %d.%d.%d.%d:%d ...\r\n",
               remote_ip[0], remote_ip[1], remote_ip[2], remote_ip[3], remote_port);
        connect(TCP_CLIENT_SOCKET, remote_ip, remote_port);
      }
      break;

    case SOCK_ESTABLISHED:
      /* Clear the connection-established interrupt flag. */
      if ((getSn_IR(TCP_CLIENT_SOCKET) & Sn_IR_CON) != 0U)
      {
        setSn_IR(TCP_CLIENT_SOCKET, Sn_IR_CON);
      }
      /* Echo loop: send back whatever the server sent. */
      length = getSn_RX_RSR(TCP_CLIENT_SOCKET);
      if (length > 0U)
      {
        if (length > sizeof(rx_buffer))
        {
          length = sizeof(rx_buffer);  /* avoid buffer overflow */
        }
        recv(TCP_CLIENT_SOCKET, rx_buffer, length);
        send(TCP_CLIENT_SOCKET, rx_buffer, length);
        printf("[TCP] RX %u bytes, echoed back\r\n", (unsigned)length);
      }
      break;

    case SOCK_CLOSE_WAIT:
      /* Peer closed half of the connection: disconnect our side. */
      disconnect(TCP_CLIENT_SOCKET);
      break;

    case SOCK_CLOSED:
      /* Socket closed: reopen the local port once. */
      if (changed != 0U)
      {
        printf("[TCP] socket closed, reopening (local port %d)\r\n", TCP_CLIENT_LOCAL_PORT);
        socket(TCP_CLIENT_SOCKET, Sn_MR_TCP, TCP_CLIENT_LOCAL_PORT, 0x00);
      }
      break;

    default:
      break;
  }

  if (changed != 0U)
  {
    printf("[TCP] state: 0x%02X -> 0x%02X\r\n", last_state, state);
  }
  last_state = state;
}

/**
  * @brief  Analyze a received data buffer (application hook).
  * @param  data Pointer to the data buffer to analyze.
  * @retval None
  */
void TCP_Client_Analyze(uint8_t *data)
{
  (void)data;  /* TODO: implement received-data analysis. */
}
