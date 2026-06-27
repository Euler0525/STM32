/**
  ******************************************************************************
  * @file    : w5500_port.c
  * @brief   : W5500 common platform glue.
  * @author  : Euler0525
  ******************************************************************************
  * @attention
  *
  * Chip-level platform glue shared by every W5500 application mode. Provides
  * the STM32 HAL (SPI + GPIO) <-> Wiznet ioLibrary callback layer, the hardware
  * reset sequence, network-information programming/verification and socket
  * buffer sizing, all wrapped in W5500_Init().
  *
  ******************************************************************************
  */

/* Includes ----------------------------------------------------------*/
#include "w5500_port.h"
#include "main.h"
#include "w5500.h"
#include "wizchip_conf.h"
#include "spi.h"
#include <string.h>   /* memcmp */
#include <stdio.h>    /* printf / setvbuf */

/* Private defines ---------------------------------------------------*/
#define W5500_RESET_PULSE_MS        1U    /*!< RESET low pulse width (ms).          */
#define W5500_RESET_DELAY_MS     1600U    /*!< RESET release settle delay (ms).      */

/* Private variables -------------------------------------------------*/

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
  * @brief  Initialize the W5500 chip to a ready-to-open-socket state.
  * @note   Registers the ioLibrary platform callbacks, performs a hardware
  *         reset, writes and verifies the network settings, and sizes the
  *         socket buffers. Does NOT open any socket.
  * @retval None
  */
void W5500_Init(void)
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

  printf("\r\n===== W5500 chip init =====\r\n");

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

  printf("===== chip init done =====\r\n\r\n");
}
