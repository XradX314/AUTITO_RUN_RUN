/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdio.h>
#include "OLED.h" // Librería de la pantalla
#include "ESP01.h" //la librería del esp
#include <string.h>
#include "DisplayUI.h"
#include "hcsr04.h"
#include "Servo.h"
#include "protocolo.h"
#include "comandos.h"
#include <Button.h>


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c1;
DMA_HandleTypeDef hdma_i2c1_tx;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */

uint16_t valores_ir[3]; // [0]=IR_LEFT, [1]=IR_CENTER, [2]=IR_RIGHT
char uart_buf[50];      // Buffer temporal para el texto de USART
volatile uint8_t adc_listo = 0; // Flag para avisar que el DMA terminó
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_TIM4_Init(void);
static void MX_TIM3_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#include "DisplayUI.h"
#include "Button.h"

uint8_t rx_byte_esp;
// Variable global para usar el dato en tu UI
uint16_t distancia_actual_mm = 0;
// Instancia del sensor
HCSR04_t mi_sensor_ultra;

// --- FUNCIONES PUENTE PARA EL HC-SR04 ---

// Callback para disparar el pin TRIG
void Sensor_SetTrig(uint8_t state) {
    HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

// Callback cuando la librería termina de calcular la distancia
void Sensor_OnResult(float dist_mm) {
    distancia_actual_mm = (uint16_t)dist_mm; // Guardamos el valor
}

// ---- ESP01 PUENTE ----
void ESP_CHPD_Ctrl(uint8_t value) { HAL_GPIO_WritePin(CHIPEN_ESP01_GPIO_Port, CHIPEN_ESP01_Pin, value ? GPIO_PIN_SET : GPIO_PIN_RESET); }
int ESP_WriteUART(uint8_t value)  { if (HAL_UART_Transmit(&huart3, &value, 1, 2) == HAL_OK) return 1; return 0; }

void ESP_ImprimirDebug(const char *dbgStr) {
    if(ui_enable_uart) HAL_UART_Transmit(&huart1, (uint8_t*)dbgStr, strlen(dbgStr), 10);
    UI_AddLog(dbgStr); // Ahora se lo mandamos a la nueva librería visual
}

void ESP_EstadoCallback(_eESP01STATUS estado) {
    if (estado == ESP01_WIFI_NEW_IP) {
        char msg[60];
        sprintf(msg, "IP: %s", ESP01_GetLocalIP());
        UI_AddLog(msg);
    }
    else if (estado == ESP01_UDPTCP_CONNECTED) UI_AddLog(">>> UDP LISTO <<<");
}
// NUEVO: Acá caen los bytes puros que llegan por WiFi UDP
void ESP_RxPayload(uint8_t value) {
    Protocolo_InjectRX(value);
}

_sESP01Handle mi_esp = { .DoCHPD = ESP_CHPD_Ctrl, .WriteUSARTByte = ESP_WriteUART, .WriteByteToBufRX = ESP_RxPayload };

// ---- OLED PUENTE ----
int I2C_EscribirComando(uint8_t cmd) { if (HAL_I2C_Mem_Write(&hi2c1, 0x78, 0x00, 1, &cmd, 1, 10) == HAL_OK) return 1; return 0; }
int I2C_EscribirDatosDMA(uint8_t *data, uint16_t len) { if (HAL_I2C_Mem_Write_DMA(&hi2c1, 0x78, 0x40, 1, data, len) == HAL_OK) return 1; return 0; }
sOLEDHandle mi_oled = { .I2C_WriteCmd = I2C_EscribirComando, .I2C_WriteData_DMA = I2C_EscribirDatosDMA };

// ---- BOTONES PUENTE ----
uint8_t Leer_SW0(void) { return HAL_GPIO_ReadPin(GPIOA, SW0_Pin); }
sButtonHandle btn_sw0 = {
    .ReadPin = Leer_SW0,
    .longClickTimeMs = 800, // 0.8 seg para considerarlo pulsación larga
    .OnShortClick = UI_ShortClick, // Directo a la UI!
    .OnLongClick = UI_LongClick    // Directo a la UI!
};




// NUEVO: Variable para interceptar lo que llega por el cable UART desde la PC
uint8_t rx_byte_pc;

sServoHandle mi_servo;
// Función puente que le dice a la librería cómo interactuar con tu TIM1
void STM32_SetServoPWM(uint16_t pulse_us) {
    // Modificamos el registro Compare/Capture directamente para cambiar el ancho de pulso
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pulse_us);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

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
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  MX_TIM4_Init();
  MX_TIM3_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */



  // 3. NUEVO: Arrancamos el OLED
    OLED_Init(&mi_oled);

    UI_Init();               // NUEVO

   // 2. Iniciamos los dos canales PWM del Timer 3 para los motores
   HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3); // Motor Izquierdo (EN_A)
   HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4); // Motor Derecho (EN_B)

   // 4. Arrancamos el ADC y nuestro metrónomo en modo Output Compare
     HAL_ADC_Start_DMA(&hadc1, (uint32_t*)valores_ir, 3);
     HAL_TIM_OC_Start(&htim4, TIM_CHANNEL_4); // <-- CLAVE: OC_Start

    // Mensaje de prueba (ya sabemos que anda)
    HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n--- INICIANDO SISTEMA ---\r\n", 29, HAL_MAX_DELAY);

    // --- NUEVO: INICIO DEL ESP-01 ---

    // Forzamos el encendido físico apenas arranca el micro
          HAL_GPIO_WritePin(CHIPEN_ESP01_GPIO_Port, CHIPEN_ESP01_Pin, GPIO_PIN_SET);

      // 1. Dejamos el USART3 "a la escucha" del primer byte que mande el ESP01
      HAL_UART_Receive_IT(&huart3, &rx_byte_esp, 1);

      // 2. Inicializamos la librería
      ESP01_Init(&mi_esp);
      // Enganchamos la función espía ANTES de iniciar
      ESP01_AttachChangeState(ESP_EstadoCallback);
      ESP01_AttachDebugStr(ESP_ImprimirDebug); // <--- NUEVO: Activa el modo espía


      // 3. Configurá acá tu red de WiFi local
      ESP01_SetWIFI("Elstein-fibra", "sanluis_1509");

      // 4. (Opcional) Si vas a mandar telemetría a tu PC, poné la IP de tu compu
      ESP01_StartUDP("192.168.0.23", 8080, 8080);


      Button_Init(&btn_sw0);

      // --- NUEVO: INICIO DEL PROTOCOLO ---
        Protocolo_Init();
        Protocolo_SetCmdParser(Comandos_Parsear);

        // Dejamos la UART1 escuchando el primer byte de la PC
        HAL_UART_Receive_IT(&huart1, &rx_byte_pc, 1);

      // 1. Encendemos el cronómetro de alta resolución
        HAL_TIM_Base_Start(&htim2);

        // 2. Iniciamos la librería pasándole nuestras funciones puente
        HCSR04_Init(&mi_sensor_ultra, Sensor_SetTrig, Sensor_OnResult);


        // 1. Arrancamos la señal de PWM por hardware del Timer 1
          HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

          // 2. Iniciamos nuestra librería vinculándola con la función puente
          Servo_Init(&mi_servo, STM32_SetServoPWM);

          // 3. NUEVO: Calibración fina del recorrido físico
          // Valores por defecto: 500 y 2500. Probad abriendo el rango:
          Servo_Calibrate(&mi_servo, 480, 2550);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
        uint32_t last_10ms = 0;
        uint32_t last_20ms_servo = 0;
        uint32_t last_100ms_ui = 0;
        uint32_t last_50ms_telem = 0;


        // Nuevos contadores para el sensor
          uint32_t last_1ms_ultra = 0;
          uint32_t last_60ms_ultra = 0;

        while (1)
        {
            uint32_t tick = HAL_GetTick();

            // --- NUEVO: VIGILANCIA DE CONEXIÓN CONSTANTE ---
            Comandos_ChequearTimeout();

            // --- 1. TAREAS DEL SENSOR ULTRASÓNICO ---

                  // A. Mantenimiento del pulso (Se ejecuta cada 1ms)
                  if (tick - last_1ms_ultra >= 1) {
                      last_1ms_ultra = tick;
                      HCSR04_TickISR(&mi_sensor_ultra);
                  }

                  // B. Pedir medición nueva (Se ejecuta cada 60ms para evitar eco fantasma)
                  if (tick - last_60ms_ultra >= 60) {
                      last_60ms_ultra = tick;
                      HCSR04_Trigger(&mi_sensor_ultra);
                  }

                  // C. Procesar datos (Se ejecuta siempre, no bloquea)
                  HCSR04_EventHandler(&mi_sensor_ultra);

                  // --- ACTUALIZACIÓN DE MOVIMIENTO DEL SERVO ---
                        if (tick - last_20ms_servo >= 20) {
                            last_20ms_servo = tick;
                            Servo_Task(&mi_servo);
                        }

            // 1. TAREAS NO BLOQUEANTES DE HARDWARE
            if (tick - last_10ms >= 10) {
                last_10ms = tick;
                ESP01_Timeout10ms();
            }
            ESP01_Task();
            OLED_Task();
            Button_Task(&btn_sw0);
            Decode(); // Constantemente mastica los bytes que van llegando al RingBuffer

            // 2. REFRESCO DE PANTALLA (A 10 FPS)
            if (tick - last_100ms_ui >= 100) {
                last_100ms_ui = tick;

                uint8_t udp_ok = (ESP01_StateUDPTCP() == ESP01_UDPTCP_CONNECTED);
                char* ip = ESP01_GetLocalIP();
                // Llama a la función pasándole los 7 parámetros exactos:
                          UI_Render(valores_ir[0], valores_ir[1], valores_ir[2], distancia_actual_mm, ip, udp_ok, pc_conectada);
            }

            // 3. ENVÍO DE TELEMETRÍA LENTA (A 2 FPS)
            if (tick - last_50ms_telem >= 50) {
                last_50ms_telem = tick;


                // 1. Cargamos el latido en el buffer
                          Comandos_EnviarAlive();

                // Ya no usamos sprintf(). Generamos el paquete binario en el buffer TX.
                          Comandos_EnviarTelemetria(valores_ir[0], valores_ir[1], valores_ir[2], distancia_actual_mm);

                          // Y forzamos que se envíe por el aire/cable
                          Comandos_FlushTx();
            }

            // 4. LÓGICA DE ALTA VELOCIDAD Y MOTORES (Disparada por el ADC)
            if (adc_listo)
            {
                adc_listo = 0;

                // Reacción instantánea del puente H al sensor
//                if (valores_ir[1] > 500) {
//                    HAL_GPIO_WritePin(GPIOB, IN_1_Pin, GPIO_PIN_SET);
//                    HAL_GPIO_WritePin(GPIOB, IN_2_Pin, GPIO_PIN_RESET);
//                    HAL_GPIO_WritePin(GPIOB, IN_3_Pin, GPIO_PIN_SET);
//                    HAL_GPIO_WritePin(GPIOB, IN_4_Pin, GPIO_PIN_RESET);
//                    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 4999);
//                    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 4999);
//                } else {
//                    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);
//                    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 0);
//                    HAL_GPIO_WritePin(GPIOB, IN_1_Pin, GPIO_PIN_RESET);
//                    HAL_GPIO_WritePin(GPIOB, IN_2_Pin, GPIO_PIN_RESET);
//                    HAL_GPIO_WritePin(GPIOB, IN_3_Pin, GPIO_PIN_RESET);
//                    HAL_GPIO_WritePin(GPIOB, IN_4_Pin, GPIO_PIN_RESET);
//                }
            }
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T4_CC4;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 3;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 71;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 19999;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 35;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 71;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 9999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 71;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 124;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_OC_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_OC4REF;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_TIMING;
  sConfigOC.Pulse = 62;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_OC_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */
  // Forzamos la configuración del Canal 4 que el CubeMX omitió

    sConfigOC.OCMode = TIM_OCMODE_PWM1; // Modo Output Compare interno
    sConfigOC.Pulse = 62;                 // Disparo a la mitad del período
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

    HAL_TIM_OC_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_4);
  /* USER CODE END TIM4_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* DMA1_Channel6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_BUILTIN_GPIO_Port, LED_BUILTIN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CHIPEN_ESP01_GPIO_Port, CHIPEN_ESP01_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, TRIG_Pin|IN_2_Pin|IN_1_Pin|IN_4_Pin
                          |IN_3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED_BUILTIN_Pin */
  GPIO_InitStruct.Pin = LED_BUILTIN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_BUILTIN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : CHIPEN_ESP01_Pin */
  GPIO_InitStruct.Pin = CHIPEN_ESP01_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CHIPEN_ESP01_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SW0_Pin SW1_Pin SW2_Pin SW3_Pin */
  GPIO_InitStruct.Pin = SW0_Pin|SW1_Pin|SW2_Pin|SW3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : ECHO_Pin */
  GPIO_InitStruct.Pin = ECHO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(ECHO_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : TRIG_Pin IN_2_Pin IN_1_Pin IN_4_Pin
                           IN_3_Pin */
  GPIO_InitStruct.Pin = TRIG_Pin|IN_2_Pin|IN_1_Pin|IN_4_Pin
                          |IN_3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
  if (hadc->Instance == ADC1)
  {
    adc_listo = 1; // Le avisamos al while(1) que los datos están listos
  }
}

// NUEVO: Le avisamos a la librería OLED que el DMA terminó su trabajo
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C1)
    {
        OLED_DMA_Callback();
    }
}

// Callback que se ejecuta cada vez que entra 1 byte por cualquier USART
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    // Datos del ESP-01
    if (huart->Instance == USART3) {
        ESP01_WriteRX(rx_byte_esp);
        HAL_UART_Receive_IT(&huart3, &rx_byte_esp, 1);
    }

    // NUEVO: Datos de la PC por cable
    if (huart->Instance == USART1) {
        Protocolo_InjectRX(rx_byte_pc);
        HAL_UART_Receive_IT(&huart1, &rx_byte_pc, 1);
    }
}


// NUEVO: Blindaje contra el grito de 74880 baudios del ESP-01
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3)
    {
        // Si la UART falla por leer basura (Framing Error/Overrun),
        // limpiamos las banderas de error internamente y volvemos a armar la trampa
        HAL_UART_Receive_IT(&huart3, &rx_byte_esp, 1);
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == ECHO_Pin) {
        // Le pasamos a la librería: la instancia, los ticks actuales del TIM2, y el estado actual del pin
        HCSR04_UpdateFromISR(&mi_sensor_ultra,
                             __HAL_TIM_GET_COUNTER(&htim2),
                             HAL_GPIO_ReadPin(ECHO_GPIO_Port, ECHO_Pin));
    }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
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
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
