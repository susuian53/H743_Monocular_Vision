/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body - Monocular Vision Device (电赛C题)
  *                    Test phase: button trigger -> capture frame -> process -> display
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "config.h"
#include "lcd_169_drv.h"
#include "dcmi_ov5640.h"
#include "image_proc.h"
#include "paper_detect.h"
#include "shape_detect.h"
#include "mono_solver.h"
#include "tilt_correct.h"
#include "multi_square.h"
#include "display.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// LED macros
#define LED_OFF     HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_RESET)
#define LED_ON      HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_SET)
#define LED_Toggle  HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_3)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile uint8_t measure_trigger = 0;
// OV5640_FrameState is defined in dcmi_ov5640.c — do NOT redefine here

// Buffers
uint8_t *camera_buffer = (uint8_t *)CAMERA_BUFFER_ADDR;
uint8_t *binary_buffer = (uint8_t *)BINARY_BUFFER_ADDR;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MPU_Config(void);
/* USER CODE BEGIN PFP */
static void MX_GPIO_Init(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_USART1_UART_Init();
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */

  CAM_LED_ON;   // Turn on camera LED indicator
  HAL_Delay(10);

  SPI_LCD_Init();
  LCD_DisplayString(10, 130, "Waiting Please...");

  DCMI_OV5640_Init();

  LCD_DisplayString(10, 130, "Press K1 to capture");

  // Set camera to grayscale (Y8) for measurement
  OV5640_Set_Pixformat(Pixformat_GRAY);

  // Reconfigure OV5640 output size to 640x480 for measurement
  OV5640_Set_Framesize(IMG_WIDTH, IMG_HEIGHT);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    if (measure_trigger)
    {
      measure_trigger = 0;

      LED_ON;

      // Trigger snapshot capture — captures one frame then stops DCMI
      OV5640_DMA_Transmit_Snapshot((uint32_t)camera_buffer, (IMG_WIDTH * IMG_HEIGHT) / 4);

      // Wait for frame capture to complete (flag is set in DCMI_IRQHandler -> callback)
      while (OV5640_FrameState == 0) { __WFI(); }
      OV5640_FrameState = 0;

      // Clean D-cache so CPU sees the fresh DMA data
      SCB_CleanInvalidateDCache_by_Addr((uint32_t *)camera_buffer, IMG_WIDTH * IMG_HEIGHT);

      // --- Vision Processing Pipeline ---
      // 1. Pre-processing
      gaussian_blur(camera_buffer, camera_buffer, IMG_WIDTH, IMG_HEIGHT); // In-place blur
      int threshold = otsu_threshold(camera_buffer, IMG_WIDTH, IMG_HEIGHT);
      binarize(camera_buffer, binary_buffer, IMG_WIDTH, IMG_HEIGHT, threshold);

      // 2. A4 Paper Detection
      float paper_width_px = 0;
      Quad paper_quad = find_largest_quad(binary_buffer, IMG_WIDTH, IMG_HEIGHT, &paper_width_px);

      // 3. Shape Detection (on the binarized image)
      float shape_pixel_size = 0;
      ShapeType shape = detect_shape(binary_buffer, IMG_WIDTH, IMG_HEIGHT, &shape_pixel_size);

      // 4. Calculation
      float real_size_mm = calculate_real_size(shape_pixel_size, paper_width_px);
      float distance_mm = calculate_distance(paper_width_px);

      // 5. Display Results on LCD
      display_results(shape, distance_mm, real_size_mm);

      // Re-arm DCMI for next capture
      OV5640_DCMI_Resume();

      LED_OFF;
    }
    __WFI(); // Sleep until next interrupt (e.g., button press)
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 3;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_HSE, RCC_MCODIV_1);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct;

  HAL_MPU_Disable();

  MPU_InitStruct.Enable             = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress        = 0x24000000;
  MPU_InitStruct.Size               = MPU_REGION_SIZE_512KB;
  MPU_InitStruct.AccessPermission   = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable       = MPU_ACCESS_BUFFERABLE;
  MPU_InitStruct.IsCacheable        = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsShareable        = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.Number             = MPU_REGION_NUMBER0;
  MPU_InitStruct.TypeExtField       = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable   = 0x00;
  MPU_InitStruct.DisableExec        = MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/**
  * @brief Initialize GPIO for LED, backlight, and K1 button
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();

  /* PA7: LCD backlight (output) */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET); // Backlight on

  /* PE3: LED, PE12: camera LED */
  GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3 | GPIO_PIN_12, GPIO_PIN_RESET);

  /* PE4: K1 button (input, pullup, falling edge interrupt) */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /* Enable and set EXTI interrupt priority */
  HAL_NVIC_SetPriority(EXTI4_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);
}

/**
  * @brief LED Init function (compatibility)
  */
void LED_Init(void)
{
  // Already initialized in MX_GPIO_Init
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
