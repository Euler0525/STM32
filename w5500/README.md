# W5500

> STM32F103C8T6 + W5500 以太网模块，包含 **TCP 客户端 / TCP 服务端 / UDP 服务端 / UDP 客户端** 四种工作模式。
>
> 基于 Wiznet 官方 [ioLibrary_Driver](https://github.com/Wiznet/ioLibrary_Driver/)，使用 STM32 HAL 库 + SPI 通信。

[TOC]

## 硬件连接

| STM32 开发板 | W5500 模块 | 说明 |
| :---------: | :-------: | :--- |
|     5V      |    5V     | 供电 |
|     GND     |   GND     | 共地 |
|     PA4     |   SCS/CS  | 片选（软件控制 GPIO） |
|     PA5     |   SCK     | SPI 时钟 |
|     PA6     |   MISO    | SPI 主入从出 |
|     PA7     |   MOSI    | SPI 主出从入 |
|     PB4     |   RST     | 硬件复位 |
|     PB5     |   INT     | 中断（已配置为输入，**当前未使用**，程序采用轮询） |

> 调试串口：**USART1 = PA9 (TX) / PA10 (RX)**，接 USB-TTL 即可查看日志。

## 默认网络配置

静态 IP 配置（写在 `Core/Src/w5500_port.c` 的 `net_info_set`）：

| 参数 | 值 |
| :--- | :--- |
| MAC  | `00:08:DC:11:11:11` |
| IP   | `192.168.1.10` |
| 子网掩码 | `255.255.255.0` |
| 网关 | `192.168.1.1` |
| DNS  | `144.144.144.144` |
| 方式 | 静态（`NETINFO_STATIC`） |

> 默认远端服务器地址（用于客户端模式）：`192.168.1.2:5002`，定义在 `tcp_client.c` / `udp_client.c` 的 `remote_ip` / `remote_port`。若 PC 不在该地址，需要修改这两处或把 PC 的 IP 设为 `192.168.1.2`。

## 工作模式

| 模式 | 本地端口 | 远端 | 行为 |
| :--: | :------: | :--- | :--- |
| `APP_MODE_TCP_CLIENT` | 5001 | `192.168.1.2:5002` | 主动连接服务器，把收到的数据原样回发 |
| `APP_MODE_TCP_SERVER` | 5000（监听） | — | 等待一个客户端连入，把收到的数据原样回发 |
| `APP_MODE_UDP_SERVER` | 5000 | — | 收到谁的数据包，就回发给谁 |
| `APP_MODE_UDP_CLIENT` | 5001 | `192.168.1.2:5002` | 先向服务器发一次请求，再把收到的数据报回发 |

所有模式均为 **非阻塞轮询**（在 `main()` 的 `while(1)` 中反复调用 `xxx_Process()`），状态切换时通过 USART1 打印一次日志。
