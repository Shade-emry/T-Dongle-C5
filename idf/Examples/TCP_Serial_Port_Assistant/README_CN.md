# T-Dongle-C5 Wireless serial port

## 项目概述
本项目是基于 ESP32-C5 芯片的 T-Dongle-C5 开发板固件，集成了 Wi-Fi、蓝牙、LCD 显示、SD 卡、LED 灯等功能。主要特性包括：

- **网络功能**：支持 Wi-Fi STA/AP 模式，内置 TCP 服务器
- **显示功能**：80x160 分辨率 ST7735 LCD 屏幕，LVGL 图形界面
- **外设控制**：APA102 LED 灯带、PWM 背光调节
- **存储扩展**：SD 卡模块支持（通过 SPI 接口）
- **人机交互**：按键控制、TCP 指令解析

可通过TCP/IP协议与电脑、手机、其他开发板等设备进行通信，实现远程控制、数据传输、串口打印、数据保存等功能。
目前SD卡数据保存等功能未实现，需要根据数据格式需求自行编写程序实现。

可搭配TCP_Wireless_Send例程进行测试，需要两块T-Dongle-C5板子。若只有一个T-Dongle-C5板子，客户端可根据MCU自行编写TCP客户端程序


## 使用说明
   可搭配TCP_Wireless_Send例程进行测试，需要两块T-Dongle-C5板子。若只有一个T-Dongle-C5板子，客户端可根据MCU自行编写TCP客户端程序
   
   TCP命令: BRIGHTNESS_20 //设置背光亮度为20

## 前置条件
### 硬件要求
- T-Dongle-C5 开发板
- MicroSD 卡（可选）

### 软件要求
- ESP-IDF v5.5 或更高版本
- CMake 构建工具
- 串口终端工具（如 PuTTY、minicom）

| 软件环境   | 支持 | 版本           |
| ---------- | ---- | -------------- |
| ESP-IDF    | ✅    | v5.5 or higher |
| PlatfromIO | ❌    |                |
| Arduino    | ❌    |                |

## 运行方法
1. **连接硬件**
   - 将开发板通过USB接口连接到电脑
   - 可选：插入格式化为 FAT32 的 MicroSD 卡

2. **配置 Wi-Fi**
   - STA 模式：修改代码中的 `WIFI_SSID` 和 `WIFI_PASSWORD`
   - AP 模式：默认热点名为 "T-Dongle-C5"，密码 "88888888"

3. **烧录固件**
   - A. Vscode IDF 插件(推荐)：
    1. 打开examples文件夹下的目录,选择一个例程，右键通过vscode打开,或者在vscode中打开examples下的例程文件夹
    2. 设置 esp32c5 目标芯片 和 串口号
    3. 设置 Wi-Fi 模式（STA/AP）
    4. 点击左下角的构建烧录监视按钮，选择 "Build & Flash"
    5. 等待编译完成，烧录完成
   
   - B. IDF:
    1. 打开终端，进入 ESP-IDF 根目录
    2. 输入 `idf.py set-target esp32c5` 设置目标芯片
    3. 按照提示设置 Wi-Fi 信息
    4. 输入 `idf.py build` 编译固件
    5. 输入 `idf.py -p PORT flash` 烧录固件（PORT 为开发板连接的串口）
   
## FAQ
- **Q:如何使用 Wireless serial port 进行通信？**
- AP 模式：
  成功设置 Wi-Fi 信息后，开发板创建的热点名为`T-Dongle-C5`，密码为`88888888`。会自动开启 TCP 服务器，等待客户端连接。默认端口号为 8080。另一端需要根据提示连接到开发板的热点，并输入IP地址和端口号,建立连接，即可进行通信。
- STA 模式：
  成功设置 Wi-Fi 信息后，开发板会自动连接到指定的 Wi-Fi 网络。连接成功后，会自动开启 TCP 服务器，等待客户端连接。默认端口号为 8080。另一端需要根据提示输入IP地址和端口号,建立连接，即可进行通信。



