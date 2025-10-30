set(srcs "t-dongle-c5-factory.c")

idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES 
                        esp_driver_spi 
                        esp_driver_gpio 
                        bt 
                        nvs_flash 
                        esp_driver_sdspi 
                        fatfs 
                        esp_lcd   
                    REQUIRES 
                        lq 
                        lcd_st7735
                    )
