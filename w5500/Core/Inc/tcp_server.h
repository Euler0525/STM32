/**
  ******************************************************************************
  * @file    : tcp_server.h
  * @brief   : W5500 TCP server driver header.
  * @author  : Euler0525
  ******************************************************************************
  * @attention
  *
  * Public interface of the W5500 TCP server. The chip-level platform glue
  * lives in w5500_port.c; tcp_server.c only opens its listening socket and
  * runs the accept/echo/disconnect state machine.
  *
  ******************************************************************************
  */

#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------*/
#include "main.h"

/* Exported macros ---------------------------------------------------*/
/** @brief Socket number used by the TCP server (the W5500 exposes 8 sockets). */
#define TCP_SERVER_SOCKET       0U

/** @brief Local TCP port the server listens on. */
#define TCP_SERVER_PORT         5000U

/* Exported functions ------------------------------------------------*/
/**
  * @brief  Bring up the W5500 chip and open the TCP server (listening) socket.
  * @note   Calls W5500_Init() for the chip-level bring-up, then opens socket
  *         @ref TCP_SERVER_SOCKET in TCP mode on @ref TCP_SERVER_PORT. The
  *         socket enters the LISTEN state from TCP_Server_Process().
  * @retval None
  */
void TCP_Server_Init(void);

/**
  * @brief  Run one pass of the TCP server state machine.
  * @note   Non-blocking; call it repeatedly from the main loop. It starts
  *         listening once the socket is open, echoes back any data received
  *         from a connected client, disconnects on CLOSE_WAIT and reopens the
  *         socket after a close. State changes are logged once to avoid
  *         spamming.
  * @retval None
  */
void TCP_Server_Process(void);

/**
  * @brief  Application hook that analyzes a received data buffer.
  * @note   Placeholder for user-defined protocol parsing.
  * @param  data Pointer to the received data buffer to analyze.
  * @retval None
  */
void TCP_Server_Analyze(uint8_t *data);

#ifdef __cplusplus
}
#endif

#endif /* TCP_SERVER_H */
