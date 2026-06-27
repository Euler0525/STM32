/**
  ******************************************************************************
  * @file    : udp_server.h
  * @brief   : W5500 UDP server driver header.
  * @author  : Euler0525
  ******************************************************************************
  * @attention
  *
  * Public interface of the W5500 UDP server. The chip-level platform glue
  * lives in w5500_port.c; udp_server.c only opens its UDP socket and runs the
  * recvfrom/sendto echo loop. Unlike TCP, a UDP socket is connectionless: it
  * is ready to exchange datagrams as soon as it is opened.
  *
  ******************************************************************************
  */

#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------*/
#include "main.h"

/* Exported macros ---------------------------------------------------*/
/** @brief Socket number used by the UDP server (the W5500 exposes 8 sockets). */
#define UDP_SERVER_SOCKET       0U

/** @brief Local UDP port the server listens on. */
#define UDP_SERVER_PORT         5000U

/* Exported functions ------------------------------------------------*/
/**
  * @brief  Bring up the W5500 chip and open the UDP server socket.
  * @note   Calls W5500_Init() for the chip-level bring-up, then opens socket
  *         @ref UDP_SERVER_SOCKET in UDP mode on @ref UDP_SERVER_PORT. The
  *         socket is ready to exchange datagrams immediately afterwards.
  * @retval None
  */
void UDP_Server_Init(void);

/**
  * @brief  Run one pass of the UDP server state machine.
  * @note   Non-blocking; call it repeatedly from the main loop. In the UDP
  *         state it receives a datagram, echoes it back to its sender, and
  *         reopens the socket after a close.
  * @retval None
  */
void UDP_Server_Process(void);

/**
  * @brief  Application hook that analyzes a received data buffer.
  * @note   Placeholder for user-defined protocol parsing.
  * @param  data Pointer to the received data buffer to analyze.
  * @retval None
  */
void UDP_Server_Analyze(uint8_t *data);

#ifdef __cplusplus
}
#endif

#endif /* UDP_SERVER_H */
