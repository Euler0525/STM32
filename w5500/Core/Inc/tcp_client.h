/**
  ******************************************************************************
  * @file    : tcp_client.h
  * @brief   : W5500 TCP client driver header.
  * @author  : Euler0525
  ******************************************************************************
  * @attention
  *
  * Public interface of the W5500 TCP client. See tcp_client.c for the
  * platform glue between the STM32 HAL (SPI + GPIO) and the Wiznet
  * ioLibrary, and the non-blocking TCP client state machine.
  *
  ******************************************************************************
  */

#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------*/
#include "main.h"

/* Exported macros ---------------------------------------------------*/
/** @brief Socket number used by the TCP client (the W5500 exposes 8 sockets). */
#define TCP_CLIENT_SOCKET       0U

/** @brief Local TCP port the client binds to. */
#define TCP_CLIENT_LOCAL_PORT   5001U

/* Exported functions ------------------------------------------------*/
/**
  * @brief  Initialize the W5500 chip and open the TCP client socket.
  * @note   Registers the ioLibrary platform callbacks, performs a hardware
  *         reset, writes and verifies the network settings, sizes the socket
  *         buffers and finally opens socket @ref TCP_CLIENT_SOCKET in TCP
  *         mode on @ref TCP_CLIENT_LOCAL_PORT.
  * @retval None
  */
void TCP_Client_Init(void);

/**
  * @brief  Run one pass of the TCP client state machine.
  * @note   Non-blocking; call it repeatedly from the main loop. It connects
  *         when the socket becomes ready, echoes back any data received from
  *         the server, disconnects on CLOSE_WAIT and reopens the socket after
  *         a close. State changes are logged once to avoid spamming.
  * @retval None
  */
void TCP_Client_Process(void);

/**
  * @brief  Application hook that analyzes a received data buffer.
  * @note   Placeholder for user-defined protocol parsing.
  * @param  data Pointer to the received data buffer to analyze.
  * @retval None
  */
void TCP_Client_Analyze(uint8_t *data);

#ifdef __cplusplus
}
#endif

#endif /* TCP_CLIENT_H */
