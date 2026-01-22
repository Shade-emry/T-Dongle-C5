# T-Dongle-C5 factory

## 项目概述
本项目是一个T-Dongle-C5的出厂固件，用于测试和验证T-Dongle-C5的功能和性能。

## 使用说明
本项目包含以下功能：
- Wi-Fi 连接：连接到指定的 Wi-Fi 网络，并输出RSSI值到屏幕上
- 串口通信：通过串口与电脑进行通信
- 按键检测：按键开关屏幕背光
- LED 控制：LED灯的闪烁
- SD卡: SD卡读取测试

## 前置条件
### 硬件要求
- T-Dongle-C5 开发板

### 软件要求
- ESP-IDF v5.5 或更高版本
- CMake 构建工具
- 串口终端工具

| 软件环境 | 支持 | 版本           |
| -------- | ---- | -------------- |
| ESP-IDF  | ✅    | v5.5 or higher |

## 运行方法
1. **连接硬件**
   - 将开发板通过USB接口连接到电脑

2. **配置 Wi-Fi**
   - 修改代码中的 `WIFI_SSID` 和 `WIFI_PASSWORD`

3. **烧录固件**
   #### A. Vscode IDF 插件(推荐)：
   1. 打开examples文件夹下的目录,选择一个例程，右键通过vscode打开,或者在vscode中打开examples下的例程文件夹
   2. 设置 esp32c5 目标芯片 和 串口号
   3. 点击左下角的构建烧录监视按钮，选择 "Build & Flash"
   4. 等待编译完成，烧录完成
   
   #### B.使用乐鑫flash_download_tools烧录工具(v3.9.8及以上)烧录：
   1. 选择esp32c5芯片
   2. 选择USB LoadMode
   3. 选择烧录文件
    ![alt text](image\image.png)
   4. 点击开始烧录
   
   #### C. IDF:
   1. 打开终端，进入 ESP-IDF 根目录
   2. 输入 `idf.py set-target esp32c5` 设置目标芯片
   3. 按照提示设置 Wi-Fi 信息
   4. 输入 `idf.py build` 编译固件
   5. 输入 `idf.py -p PORT flash` 烧录固件（PORT 为开发板连接的串口）




