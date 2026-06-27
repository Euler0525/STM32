/**
  ******************************************************************************
  * @file    : w5500_port.h
  * @brief   : W5500 common platform glue header.
  * @author  : Euler0525
  ******************************************************************************
  * @attention
  *
  * Chip-level platform glue shared by every W5500 application mode
  * (TCP/UDP client and server). This module owns the STM32 HAL (SPI + GPIO)
  * <-> Wiznet ioLibrary callback layer, the hardware reset sequence, the
  * network-information programming/verification and the socket buffer sizing,
  * all wrapped in W5500_Init(). The mode-specific socket state machines live
  * in tcp_client.c, tcp_server.c, udp_server.c and udp_client.c and only open
  * their own socket after W5500_Init() has brought the chip up.
  *
  ******************************************************************************
  */

#ifndef W5500_PORT_H
#define W5500_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------*/
#include "main.h"

/* Exported functions ------------------------------------------------*/
/**
  * @brief  Initialize the W5500 chip to a ready-to-open-socket state.
  * @note   Makes printf unbuffered, registers the ioLibrary platform callbacks
  *         (critical-section / chip-select / SPI byte access), drives a hardware
  *         reset, writes and verifies the static network information, and sizes
  *         the socket buffers (8 sockets x 2 KB TX/RX). It does NOT open any
  *         socket; each application mode opens its own socket afterwards.
  * @retval None
  */
void W5500_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* W5500_PORT_H */
