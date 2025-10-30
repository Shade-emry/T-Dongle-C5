### 1. Overview
    Example routine for obtaining the WIFI RSSI value during TCP transmission in the STA mode of ESP32C5.

### 2. How to use the example
- set `wifi ssid` and `password` in the example code
```c
#define EXAMPLE_ESP_WIFI_SSID "your_ssid"
#define EXAMPLE_ESP_WIFI_PASS "your_password"
```

- set `IP address` and `port number` in the example code
```c
#define TCP_SERVER_PORT 8080
char host_ip[] = "192.168.3.169"; 
```

### Tips:
A.This routine requires that the server be initialized first, and only then can the client be opened. Only after that can the client receive the information.

B.The routine is merely a test program. There are still many areas for optimization, and the customer is required to optimize the code themselves.