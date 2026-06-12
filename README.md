# H743 Monocular Vision — 单目视觉测量装置

基于 **STM32H743VIT6** + **OV5640 摄像头** + **1.69 寸 TFT LCD** 的单目视觉测量设备，针对 2025 年全国大学生电子设计竞赛（电赛 C 题）设计。

---

## 各源文件功能详解

### 主程序与初始化

| 文件 | 作用 |
|------|------|
| `main.c` | 程序入口。配置系统时钟（HSE 25MHz → PLL → 480MHz）、MPU 保护、D-Cache/I-Cache；初始化 GPIO（LED、背光、K1 按键 EXTI4）、USART1、LCD、OV5640 摄像头；主循环等待测量触发，按顺序执行拍照 → 高斯模糊 → Otsu 二值化 → A4 纸检测 → 形状检测 → 单目解算 → 结果显示 |
| `stm32h7xx_it.c` | 中断服务。EXTI4 捕获 K1 按键按下设置触发标志；DMA/DCMI 中断路由至 HAL 库 |
| `system_stm32h7xx.c` | ST 标准库 SystemInit，配置向量表、FPU，初始化 PWR 域 |

### 摄像头驱动

| 文件 | 作用 |
|------|------|
| `dcmi_ov5640.c` | OV5640 核心驱动。DCMI 外设初始化（8 位并行 / 640×480 / 全帧 / 硬件同步）、DMA2 Stream7 配置（外设→内存、SnapShot 单帧捕获）；像素格式设置（Y8 灰度）、帧尺寸配置、自动对焦；`HAL_DCMI_FrameEventCallback` 置位帧完成标志 |
| `sccb.c` | SCCB（串行摄像头控制总线）驱动，GPIO 模拟 I2C 时序，支持 16 位寄存器地址读写 |
| `dcmi_ov5640_cfg.h` | OV5640 寄存器配置表（包含自动对焦固件、各级分辨率/帧率配置） |

### LCD 显示驱动

| 文件 | 作用 |
|------|------|
| `lcd_169_drv.c` | 1.69 寸 SPI LCD 驱动（240×280）。SPI1 初始化及 GPIO 配置；提供画点、画线、图形填充、字符串/数字显示、批量缓冲区拷贝等 API |
| `lcd_fonts.c` | 字模数据。5 种尺寸的 ASCII 字体和等尺寸中文小字库（宋体） |

### 视觉处理管线

| 文件 | 作用 |
|------|------|
| `image_proc.c` | 预处理。高斯模糊（3×3 核）、Otsu 大津法自动阈值、全局阈值二值化、基于积分图像的自适应阈值 |
| `paper_detect.c` | A4 纸检测。Sobel 边缘 → Canny 非极大抑制 → 双阈值边缘连接 → Moore 边界追踪找轮廓 → Douglas-Peucker 多边形逼近 → 找出最大四边形 |
| `shape_detect.c` | 形状识别。7 个 Hu 不变矩，基于对数特征分类圆形/三角形/方形 |
| `mono_solver.c` | 单目解算。基于标定查找表线性插值得物距，相似三角形换算实际物理尺寸 |
| `tilt_correct.c` | 倾斜校正。基于纸面角点的单应性变换，消除透视畸变 |
| `multi_square.c` | 多正方形检测与数字识别（5×3 网格特征 + 七段码分类 0-9） |

### 显示输出

| 文件 | 作用 |
|------|------|
| `display.c` | 结果展示（形状名称/距离/尺寸）和灰度图到 LCD 的缩放预览 |

### 平台支持文件（CubeMX 自动生成）

| 文件 | 作用 |
|------|------|
| `usart.c` | USART1 串口驱动配置 |
| `dcmi.c` / `spi.c` / `dma.c` / `gpio.c` | 已禁用（对应功能由 dcmi_ov5640.c / lcd_169_drv.c 接管），仅保留 stub |
| `stm32h7xx_hal_msp.c` | HAL 库 MSP 初始化回调 |

---

## 引脚分配

| 功能 | 引脚 | 说明 |
|------|------|------|
| DCMI 数据 | PC6,7 / PE0,1 / PD3 / PE5,6 | 8 位并行数据总线（PE4 已释放作按键） |
| DCMI 同步 | PB7(VSYNC) / PA4(HSYNC) / PA6(PIXCLK) | 帧同步、行同步、像素时钟 |
| SCCB | PB6(SCL) / PB9(SDA) | I2C 控制总线 |
| 摄像头控制 | PD10(PWDN) / PE13(RST) / PE12(LED) | 电源、复位、补光 |
| LCD SPI | PB3(SCK) / PD7(MOSI) / PA15(CS) / PC4(DC) / PC13(RST) | 屏幕通信 |
| 用户交互 | PA7(背光) / PE4(K1 按键 EXTI4) / PE3(工作 LED) | |
| 调试 | PB14(TX) / PB15(RX) | USART1 串口 |

> PE4 在示例驱动中复用为 DCMI_D4（AF 模式），本工程已改为 K1 按键输入（EXTI 下降沿）。请根据硬件原理图确认此配置。

---

## 内存布局

| 区域 | 地址 | 用途 |
|------|------|------|
| AXI SRAM | 0x24000000 (512KB) | 相机灰度缓冲区（640×480=300KB）+ 二值化缓冲区（复用） |
| MPU | Non-cacheable | AXI SRAM 禁用缓存以保证 DMA/CPU 数据一致性 |

---

## 构建

- **IDE**: Keil MDK-ARM 5.38
- **Compiler**: ARM Compiler 5.06 update 7 (build 960)
- **状态**: ✅ 0 Error, 0 Warning
