/**
  ******************************************************************************
  * @file    : tcp_server.c
  * @brief   : W5500 TCP server state machine.
  * @author  : Euler0525
  ******************************************************************************
  * @attention
  *
  * Non-blocking TCP server state machine. The W5500 chip-level bring-up lives
  * in w5500_port.c and is invoked via W5500_Init(); this module only opens its
  * listening socket and runs the listen/echo/disconnect loop. Any data received
  * from a connected client is echoed straight back.
  *
  ******************************************************************************
  */

/* Includes ----------------------------------------------------------*/
#include "tcp_server.h"
#include "w5500_port.h"
#include "main.h"
#include "w5500.h"
#include "socket.h"
#include <stdio.h>    /* printf */

/* Private defines ---------------------------------------------------*/
#define TCP_SERVER_BUFFER_SIZE    128U    /*!< Receive buffer size in bytes.       */
#define SOCKET_STATE_UNKNOWN      0xFFU   /*!< Sentinel forcing a log on first run. */

/* Private variables -------------------------------------------------*/

/* Working buffer for received data (internal to the echo loop). */
static uint8_t rx_buffer[TCP_SERVER_BUFFER_SIZE];

/* Exported functions ------------------------------------------------*/

/**
  * @brief  Bring up the W5500 chip and open the TCP server (listening) socket.
  * @note   Calls W5500_Init() for the chip-level bring-up, then opens socket
  *         @ref TCP_SERVER_SOCKET in TCP mode on @ref TCP_SERVER_PORT.
  * @retval None
  */
void TCP_Server_Init(void)
{
  W5500_Init();

  printf("===== W5500 TCP Server init =====\r\n");

  /* Open socket 0 as a TCP server on the local port, if not already open. */
  uint8_t state = getSn_SR(TCP_SERVER_SOCKET);
  if (state == SOCK_CLOSED)
  {
    socket(TCP_SERVER_SOCKET, Sn_MR_TCP, TCP_SERVER_PORT, 0x00);
    printf("TCP socket opened (listening port %d)\r\n", TCP_SERVER_PORT);
  }
  else
  {
    printf("Socket already open (state 0x%02X)\r\n", state);
  }

  printf("===== init done =====\r\n\r\n");
}

/**
  * @brief  Run one pass of the TCP server state machine.
  * @note   Non-blocking; call it repeatedly from the main loop. It starts
  *         listening once the socket is open, echoes back any data received
  *         from a connected client, disconnects on CLOSE_WAIT and reopens the
  *         socket after a close. State changes are logged once to avoid
  *         spamming.
  * @retval None
  */
void TCP_Server_Process(void)
{
  static uint8_t last_state = SOCKET_STATE_UNKNOWN;
  uint8_t  state   = getSn_SR(TCP_SERVER_SOCKET);
  uint16_t length  = 0U;
  uint8_t  changed = (state != last_state);  /* act once per state transition */

  switch (state)
  {
    case SOCK_INIT:
      /* Socket opened as a server: start listening for a single client. */
      if (changed != 0U)
      {
        printf("[TCP Server] listening on port %d\r\n", TCP_SERVER_PORT);
        listen(TCP_SERVER_SOCKET);
      }
      break;

    case SOCK_LISTEN:
      /* Waiting for an incoming connection; nothing to do. */
      break;

    case SOCK_ESTABLISHED:
      /* Clear the connection-established interrupt flag. */
      if ((getSn_IR(TCP_SERVER_SOCKET) & Sn_IR_CON) != 0U)
      {
        setSn_IR(TCP_SERVER_SOCKET, Sn_IR_CON);
        printf("[TCP Server] client connected\r\n");
      }
      /* Echo loop: send back whatever the client sent. */
      length = getSn_RX_RSR(TCP_SERVER_SOCKET);
      if (length > 0U)
      {
        if (length > sizeof(rx_buffer))
        {
          length = sizeof(rx_buffer);  /* avoid buffer overflow */
        }
        recv(TCP_SERVER_SOCKET, rx_buffer, length);
        send(TCP_SERVER_SOCKET, rx_buffer, length);
        printf("[TCP Server] RX %u bytes, echoed back\r\n", (unsigned)length);
      }
      break;

    case SOCK_CLOSE_WAIT:
      /* Peer closed half of the connection: disconnect our side. */
      disconnect(TCP_SERVER_SOCKET);
      break;

    case SOCK_CLOSED:
      /* Socket closed: reopen the listening port once. */
      if (changed != 0U)
      {
        printf("[TCP Server] socket closed, reopening (port %d)\r\n", TCP_SERVER_PORT);
        socket(TCP_SERVER_SOCKET, Sn_MR_TCP, TCP_SERVER_PORT, 0x00);
      }
      break;

    default:
      break;
  }

  if (changed != 0U)
  {
    printf("[TCP Server] state: 0x%02X -> 0x%02X\r\n", last_state, state);
  }
  last_state = state;
}

/**
  * @brief  Analyze a received data buffer (application hook).
  * @param  data Pointer to the data buffer to analyze.
  * @retval None
  */
void TCP_Server_Analyze(uint8_t *data)
{
  (void)data;  /* TODO: implement received-data analysis. */
}
