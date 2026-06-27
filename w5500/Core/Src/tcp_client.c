/**
  ******************************************************************************
  * @file    : tcp_client.c
  * @brief   : W5500 TCP client driver.
  * @author  : Euler0525
  ******************************************************************************
  * @attention
  *
  * This module provides the platform glue between the STM32 HAL (SPI + GPIO)
  * and the Wiznet ioLibrary for the W5500 chip, and runs a non-blocking TCP
  * client state machine that echoes back any data received from the remote
  * server.
  *
  ******************************************************************************
  */

/* Includes ----------------------------------------------------------*/
#include "tcp_client.h"
#include "main.h"
#include "w5500.h"
#include "socket.h"
#include "wizchip_conf.h"
#include "spi.h"
#include <string.h>   /* memcmp */
#include <stdio.h>    /* printf / setvbuf (retargeted to USART1) */

/* Private defines ---------------------------------------------------*/
#define TCP_CLIENT_BUFFER_SIZE    128U    /*!< Receive buffer size in bytes.       */
#define W5500_RESET_PULSE_MS        1U    /*!< RESET low pulse width (ms).          */
#define W5500_RESET_DELAY_MS     1600U    /*!< RESET release settle delay (ms).     */
#define SOCKET_STATE_UNKNOWN      0xFFU   /*!< Sentinel forcing a log on first run. */

/* Private variables -------------------------------------------------*/

/* Remote TCP server the client connects to. */
static uint8_t  remote_ip[4] = {192U, 168U, 1U, 2U};
static uint16_t remote_port  = 5002U;

/* Working buffer for received data (internal to the echo loop). */
static uint8_t  rx_buffer[TCP_CLIENT_BUFFER_SIZE];

/* Network configuration written to the W5500. */
static wiz_NetInfo net_info_set = {
  .mac  = {0x00, 0x08, 0xDC, 0x11, 0x11, 0x11},
  .ip   = {192, 168, 1, 10},
  .sn   = {255, 255, 255, 0},
  .gw   = {192, 168, 1, 1},
  .dns  = {144, 144, 144, 144},
  .dhcp = NETINFO_STATIC
};

/* Network information read back from the W5500, used to verify the writes. */
static wiz_NetInfo net_info_get;

/* Private function prototypes ---------------------------------------*/
static void    W5500_SelectChip(void);
static void    W5500_DeselectChip(void);
static void    W5500_Restart(void);
static void    W5500_ReadBuffer(uint8_t *buffer, uint16_t length);
static void    W5500_WriteBuffer(uint8_t *buffer, uint16_t length);
static uint8_t W5500_ReadByte(void);
static void    W5500_WriteByte(uint8_t byte);
static void    W5500_EnterCriticalSection(void);
static void    W5500_ExitCriticalSection(void);
static uint8_t W5500_ValidateNetInfo(const wiz_NetInfo *expected,
                                     const wiz_NetInfo *actual);
static void    W5500_PrintNetInfo(const wiz_NetInfo *info);

/* Private functions -------------------------------------------------*/

/**
  * @brief  Assert the W5500 chip-select line (active low).
  * @retval None
  */
static void W5500_SelectChip(void)
{
  HAL_GPIO_WritePin(W5500_CS_GPIO_Port, W5500_CS_Pin, GPIO_PIN_RESET);
}

/**
  * @brief  Release the W5500 chip-select line.
  * @retval None
  */
static void W5500_DeselectChip(void)
{
  HAL_GPIO_WritePin(W5500_CS_GPIO_Port, W5500_CS_Pin, GPIO_PIN_SET);
}

/**
  * @brief  Generate a hardware reset of the W5500 through the RESET pin.
  * @retval None
  */
static void W5500_Restart(void)
{
  HAL_GPIO_WritePin(W5500_RST_GPIO_Port, W5500_RST_Pin, GPIO_PIN_RESET);
  HAL_Delay(W5500_RESET_PULSE_MS);
  HAL_GPIO_WritePin(W5500_RST_GPIO_Port, W5500_RST_Pin, GPIO_PIN_SET);
  HAL_Delay(W5500_RESET_DELAY_MS);
}

/**
  * @brief  Read a block of bytes from the W5500 over SPI.
  * @param  buffer Destination buffer.
  * @param  length Number of bytes to read.
  * @retval None
  */
static void W5500_ReadBuffer(uint8_t *buffer, uint16_t length)
{
  HAL_SPI_Receive(&hspi1, buffer, length, HAL_MAX_DELAY);
}

/**
  * @brief  Write a block of bytes to the W5500 over SPI.
  * @param  buffer Source buffer.
  * @param  length Number of bytes to write.
  * @retval None
  */
static void W5500_WriteBuffer(uint8_t *buffer, uint16_t length)
{
  HAL_SPI_Transmit(&hspi1, buffer, length, HAL_MAX_DELAY);
}

/**
  * @brief  Read a single byte from the W5500 over SPI.
  * @retval The byte read.
  */
static uint8_t W5500_ReadByte(void)
{
  uint8_t byte = 0U;

  W5500_ReadBuffer(&byte, sizeof(byte));
  return byte;
}

/**
  * @brief  Write a single byte to the W5500 over SPI.
  * @param  byte Byte to write.
  * @retval None
  */
static void W5500_WriteByte(uint8_t byte)
{
  W5500_WriteBuffer(&byte, sizeof(byte));
}

/**
  * @brief  Enter a critical section by disabling interrupts.
  * @note   Used by the ioLibrary to keep W5500 register / SPI accesses atomic.
  * @retval None
  */
static void W5500_EnterCriticalSection(void)
{
  __disable_irq();
}

/**
  * @brief  Exit a critical section by re-enabling interrupts.
  * @retval None
  */
static void W5500_ExitCriticalSection(void)
{
  __enable_irq();
}

/**
  * @brief  Compare two network information structures.
  * @param  expected Configured (expected) network information.
  * @param  actual   Read-back (actual) network information.
  * @retval 1 if they match, 0 otherwise.
  */
static uint8_t W5500_ValidateNetInfo(const wiz_NetInfo *expected,
                                     const wiz_NetInfo *actual)
{
  return (memcmp(expected, actual, sizeof(wiz_NetInfo)) == 0) ? 1U : 0U;
}

/**
  * @brief  Print the network information over the configured USART.
  * @param  info Network information to print.
  * @retval None
  */
static void W5500_PrintNetInfo(const wiz_NetInfo *info)
{
  printf("MAC : %02X:%02X:%02X:%02X:%02X:%02X\r\n",
         info->mac[0], info->mac[1], info->mac[2],
         info->mac[3], info->mac[4], info->mac[5]);
  printf("IP  : %d.%d.%d.%d\r\n",
         info->ip[0], info->ip[1], info->ip[2], info->ip[3]);
  printf("SN  : %d.%d.%d.%d\r\n",
         info->sn[0], info->sn[1], info->sn[2], info->sn[3]);
  printf("GW  : %d.%d.%d.%d\r\n",
         info->gw[0], info->gw[1], info->gw[2], info->gw[3]);
}

/* Exported functions ------------------------------------------------*/

/**
  * @brief  Initialize the W5500 chip and open the TCP client socket.
  * @retval None
  */
void TCP_Client_Init(void)
{
  /* Make printf unbuffered so log lines reach USART1 immediately. */
  setvbuf(stdout, NULL, _IONBF, 0);

  /* Register the platform callbacks the ioLibrary relies on. */
  reg_wizchip_cris_cbfunc(W5500_EnterCriticalSection, W5500_ExitCriticalSection);
  reg_wizchip_cs_cbfunc(W5500_SelectChip, W5500_DeselectChip);
  reg_wizchip_spi_cbfunc(W5500_ReadByte, W5500_WriteByte);

  /* Drive CS high (idle); CubeMX leaves it low by default. */
  W5500_DeselectChip();
  /* Hardware reset of the chip. */
  W5500_Restart();

  printf("\r\n===== W5500 TCP Client init =====\r\n");

  /* Write the network configuration, then read it back and verify it. */
  ctlnetwork(CN_SET_NETINFO, (void *)&net_info_set);
  HAL_Delay(10U);
  ctlnetwork(CN_GET_NETINFO, (void *)&net_info_get);

  W5500_PrintNetInfo(&net_info_get);

  if (W5500_ValidateNetInfo(&net_info_set, &net_info_get) != 0U)
  {
    printf("Net info verify: OK\r\n");
  }
  else
  {
    printf("Net info verify: FAILED!\r\n");
  }

  /* Size the socket buffers: 2 KB TX + 2 KB RX for each of the 8 sockets. */
  uint8_t socket_buffer_sizes[16] = {2, 2, 2, 2, 2, 2, 2, 2,
                                     2, 2, 2, 2, 2, 2, 2, 2};
  if (ctlwizchip(CW_INIT_WIZCHIP, (void *)socket_buffer_sizes) != 0)
  {
    printf("WIZCHIP buffer init: FAILED!\r\n");
  }
  else
  {
    printf("WIZCHIP buffer init: OK (8x2KB TX/RX)\r\n");
  }

  HAL_Delay(100U);

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
