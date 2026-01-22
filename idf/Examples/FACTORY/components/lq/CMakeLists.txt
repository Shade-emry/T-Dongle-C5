# your_project/components/my_driver/CMakeLists.txt

# 注册本组件
idf_component_register(
    SRCS "lq.c"        # 列出所有源文件
    INCLUDE_DIRS "include"        # 列出公共头文件目录，这样其他组件（如main）才能找到你的头文件
    REQUIRES                # 声明此组件需要依赖哪些其他ESP-IDF组件（例如，如果你的驱动用到了ESP-IDF的driver）
)