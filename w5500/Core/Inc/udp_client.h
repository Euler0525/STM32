/**
  ******************************************************************************
  * @file    : udp_client.h
  * @brief   : W5500 UDP client driver header.
  * @author  : Euler0525
  ******************************************************************************
  * @attention
  *
  * Public interface of the W5500 UDP client. The chip-level platform glue
  * lives in w5500_port.c; udp_client.c only opens its UDP socket and runs the
  * sendto/recvfrom loop. After opening the socket it sends a one-time greeting
  * to the configured remote server, then echoes back any datagram received.
  *
  ******************************************************************************
  */

#ifndef UDP_CLIENT_H
#define UDP_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------*/
#include "main.h"

/* Exported macros ---------------------------------------------------*/
/** @brief Socket number used by the UDP client (the W5500 exposes 8 sockets). */
#define UDP_CLIENT_SOCKET       0U

/** @brief Local UDP port the client binds to. */
#define UDP_CLIENT_LOCAL_PORT   5001U

/* Exported functions ------------------------------------------------*/
/**
  * @brief  Bring up the W5500 chip and open the UDP client socket.
  * @note   Calls W5500_Init() for the chip-level bring-up, then opens socket
  *         @ref UDP_CLIENT_SOCKET in UDP mode on @ref UDP_CLIENT_LOCAL_PORT.
  *         The remote server address/port are configured in udp_client.c.
  * @retval None
  */
void UDP_Client_Init(void);

/**
  * @brief  Run one pass of the UDP client state machine.
  * @note   Non-blocking; call it repeatedly from the main loop. On first entry
  *         to the UDP state it sends a greeting to the configured remote
  *         server, then echoes back any datagram received, and reopens the
  *         socket after a close.
  * @retval None
  */
void UDP_Client_Process(void);

/**
  * @brief  Application hook that analyzes a received data buffer.
  * @note   Placeholder for user-defined protocol parsing.
  * @param  data Pointer to the received data buffer to analyze.
  * @retval None
  */
void UDP_Client_Analyze(uint8_t *data);

#ifdef __cplusplus
}
#endif

#endif /* UDP_CLIENT_H */
