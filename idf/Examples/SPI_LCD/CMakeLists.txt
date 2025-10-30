cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS ${CMAKE_SOURCE_DIR}/managed_components/espressif__esp_lcd_st7735)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(spi_lcd)
