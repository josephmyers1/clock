/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body Final Project SP2025
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "seg7.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
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
I2C_HandleTypeDef hi2c1;
I2S_HandleTypeDef hi2s3;
RTC_HandleTypeDef hrtc;
SPI_HandleTypeDef hspi1;
TIM_HandleTypeDef htim7;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
char ramp = 0;
char RED_BRT = 0;
char GREEN_BRT = 0;
char BLUE_BRT = 0;
char RED_STEP = 1;
char GREEN_STEP = 2;
char BLUE_STEP = 3;
char DIM_Enable = 0;
char Music_ON = 0;
int  TONE = 0;
int  COUNT = 0;
int  INDEX = 0;
int  Note = 0;
int  Save_Note = 0;
int  Vibrato_Depth = 1;
int  Vibrato_Rate = 40;
int  Vibrato_Count = 0;
char Animate_On = 0;
char Message_Length = 0;
char *Message_Pointer;
char *Save_Pointer;
int  Delay_msec = 0;
int  Delay_counter = 0;

/* HELLO ECE-330L */
char Message[] =
    {SPACE,SPACE,SPACE,SPACE,SPACE,SPACE,SPACE,SPACE,SPACE,SPACE,
     CHAR_H,CHAR_E,CHAR_L,CHAR_L,CHAR_O,SPACE,CHAR_E,CHAR_C,CHAR_E,DASH,CHAR_3,CHAR_3,CHAR_0,CHAR_L,
     SPACE,SPACE,SPACE,SPACE,SPACE,SPACE,SPACE,SPACE,SPACE,SPACE};

/* Declare array for Song */
Music Song[100];

/* Variables for RTC set/edit */
char currentState     = 0;   // 0=clock,1=calendar,2=alarm1,3=alarm2
char debounceFlag11   = 0;
char debounceFlag10   = 0;
char editMode         = 0;
char updateValueFlag  = 0;
char prevEdit         = 0;

/* Time/Date set values */
int  daySet    = 1;    // 1–31
int  monthSet  = 1;    // 1–12
int  yearSet   = 23;   // 0–99
char hourSet   = 0;    // 0–23
char minuteSet = 0;    // 0–59
char secondSet = 0;    // 0–59

/* Alarm1 */
char alarm1H = 0, alarm1M = 0, alarm1S = 0;
/* Alarm2 */
char alarm2H = 0, alarm2M = 0, alarm2S = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM7_Init(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// … (song initialization unchanged) …
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
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_TIM7_Init();
  /* USER CODE BEGIN 2 */

  // Enable RTC
  // Enable RTC, using the LSI oscillator
  // Enable PWR per
  __HAL_RCC_PWR_CLK_ENABLE();
  /* enable backup-domain writes (DBP bit) */
  PWR->CR   |= (1U << 8);

  /* turn on the LSI oscillator (LSION = bit 0 of CSR) */
  RCC->CSR  |= (1U << 0);
  /* wait for it to stabilize (LSIRDY = bit 1 of CSR) */
  while (!(RCC->CSR & (1U << 1)));

  /* select LSI as RTC source (RTCSEL bits 9:8 = 10b) and enable RTC (RTCEN bit 15) */
  RCC->BDCR |= (2U << 8)   /* RTCSEL = 10b */
             | (1U << 15); /* RTCEN  = 1   */

  // Program RTC prescalers to get a 1 Hz timebase (LSI ≈ 33 152 Hz)
  RTC->WPR = 0xCA; RTC->WPR = 0x53;     // disable write protection
  RTC->ISR |=  RTC_ISR_INIT;            // enter init mode
  while(!(RTC->ISR & RTC_ISR_INITF));   // wait for it
  RTC->PRER  = (127 << 16)  // asynchronous prescaler
             |  258;        // synchronous prescaler
  RTC->ISR &= ~RTC_ISR_INIT;            // exit init mode
  RTC->WPR = 0xFF;                      // re-enable write protection


  /*** Configure GPIOs ***/
  GPIOD->MODER = 0x55555555;     // Port D outputs (LEDs)
  GPIOA->MODER |= 0x000000FF;    // PA0-PA3 analog
  GPIOE->MODER |= 0x55555555;    // Port E outputs (7-seg)
  GPIOC->MODER |= 0x00000000;    // Port C inputs (buttons)
  GPIOE->ODR    = 0xFFFF;        // 7-seg off

  /*** Configure ADC1 ***/
  RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

  /* sample times for PA1/PA2/PA3 = 15 cycles */
  ADC1->SMPR2 |= (1U <<  3)   // SMP1 = 001 → 15 cycles for channel 1
                | (1U <<  6)   // SMP2 = 001 → 15 cycles for channel 2
                | (1U <<  9);  // SMP3 = 001 → 15 cycles for channel 3

  /* reset & calibrate */
  ADC1->CR2 |= (1U << 3);               /* RSTCAL */
  while (ADC1->CR2 & (1U << 3)) {}      /* wait */
  ADC1->CR2 |= (1U << 2);               /* CAL */
  while (ADC1->CR2 & (1U << 2)) {}      /* wait */

  /* turn ADC on */
  ADC1->CR2 |= ADC_CR2_ADON;

  /* Timer7: for PWM on buzzer, unchanged… */
  TIM7->PSC = 199;
  TIM7->ARR =   1;
  TIM7->DIER |=  1;
  TIM7->CR1  |=  1;

  // enable Alarm A & B in the RTC
  RTC->CR |= RTC_CR_ALRAIE | RTC_CR_ALRAE    // Alarm A interrupt + enable
           | RTC_CR_ALRBIE | RTC_CR_ALRBE;  // Alarm B interrupt + enable

  // now wire up EXTI & NVIC so that the RTC alarm lines will actually
  // generate an IRQ that gets to HAL_RTC_AlarmIRQHandler()

  // 1) unmask the wake-up lines for Alarm A (EXTI 17) and Alarm B (EXTI 18)
  EXTI->IMR  |= (1<<17) | (1<<18);
  EXTI->RTSR |= (1<<17) | (1<<18);

  // 2) turn on the RTC_Alarm_IRQn in the NVIC
  HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* 1) PC11 cycles screens */
    if (((GPIOC->IDR >> 11) & 1) == 0 && !debounceFlag11) {
      currentState = (currentState + 1) & 3;
      debounceFlag11 = 1;
    }
    else if (((GPIOC->IDR >> 11) & 1) == 1 && debounceFlag11) {
      HAL_Delay(1); debounceFlag11 = 0;
    }

    /* 2) PC10 toggles editMode */
    if (((GPIOC->IDR >> 10) & 1) == 0 && !debounceFlag10) {
      editMode ^= 1;
      debounceFlag10 = 1;
    }
    else if (((GPIOC->IDR >> 10) & 1) == 1 && debounceFlag10) {
      HAL_Delay(1);
      // if leaving editMode, mark for commit
      if (editMode == 0) updateValueFlag = 1;
      debounceFlag10 = 0;
    }

    /* 3) Read pots & clamp while in editMode */
    if (editMode) {
      uint16_t a1,a2,a3;
      // PA1
      ADC1->SQR3 =  1; ADC1->CR2 |= ADC_CR2_SWSTART;
      while(!(ADC1->SR & ADC_SR_EOC)); a1 = ADC1->DR & 0x0FFF;
      // PA2
      ADC1->SQR3 =  2; ADC1->CR2 |= ADC_CR2_SWSTART;
      while(!(ADC1->SR & ADC_SR_EOC)); a2 = ADC1->DR & 0x0FFF;
      // PA3
      ADC1->SQR3 =  3; ADC1->CR2 |= ADC_CR2_SWSTART;
      while(!(ADC1->SR & ADC_SR_EOC)); a3 = ADC1->DR & 0x0FFF;

      switch(currentState) {
        case 0:  // clock
          hourSet   = (a1*24)/4096; if(hourSet>23) hourSet=23;
          minuteSet = (a2*60)/4096; if(minuteSet>59) minuteSet=59;
          secondSet = (a3*60)/4096; if(secondSet>59) secondSet=59;
          break;
        case 1:  // calendar
          yearSet  = (a1*100)/4096;  if(yearSet>99) yearSet=99;
          monthSet = (a2*12)/4096+1;  if(monthSet>12) monthSet=12;
          daySet   = (a3*31)/4096+1;  if(daySet>31) daySet=31;
          break;
        case 2:  // alarm1
          alarm1H = (a1*24)/4096; if(alarm1H>23) alarm1H=23;
          alarm1M = (a2*60)/4096; if(alarm1M>59) alarm1M=59;
          alarm1S = (a3*60)/4096; if(alarm1S>59) alarm1S=59;
          break;
        case 3:  // alarm2
          alarm2H = (a1*24)/4096; if(alarm2H>23) alarm2H=23;
          alarm2M = (a2*60)/4096; if(alarm2M>59) alarm2M=59;
          alarm2S = (a3*60)/4096; if(alarm2S>59) alarm2S=59;
          break;
      }
    }

    /* 4) Commit into RTC on editMode→0 */
    if (updateValueFlag && prevEdit==1 && editMode==0) {
      if (currentState==0) {
        uint32_t tr =
          ((hourSet/10)<<20)|((hourSet%10)<<16)|
          ((minuteSet/10)<<12)|((minuteSet%10)<< 8)|
          ((secondSet/10)<< 4)|((secondSet%10));
        RTC->WPR = 0xCA; RTC->WPR = 0x53;
        RTC->ISR |=  RTC_ISR_INIT;      while(!(RTC->ISR & RTC_ISR_INITF));
        RTC->TR  =  tr;
        RTC->ISR &= ~RTC_ISR_INIT;
        RTC->WPR = 0xFF;
      }
      else if (currentState==1) {
        uint32_t olddr = RTC->DR & RTC_DR_WDU; // keep weekday
        uint32_t dr =
          ((yearSet/10)<<20)|((yearSet%10)<<16)|
          olddr|
          ((monthSet/10)<<12)|((monthSet%10)<< 8)|
          ((daySet/10)<< 4)|((daySet%10));
        RTC->WPR = 0xCA; RTC->WPR = 0x53;
        RTC->ISR |=  RTC_ISR_INIT;      while(!(RTC->ISR & RTC_ISR_INITF));
        RTC->DR  =  dr;
        RTC->ISR &= ~RTC_ISR_INIT;
        RTC->WPR = 0xFF;
      }
      else if (currentState==2) {       // —– commit Alarm 1 —–
        /* unlock RTC write */
        RTC->WPR = 0xCA;
        RTC->WPR = 0x53;
        /* enter init mode */
        RTC->ISR |=  RTC_ISR_INIT;
        while(!(RTC->ISR & RTC_ISR_INITF));
        /* program Alarm A register: HH:MM:SS */
        RTC->ALRMAR =
            (alarm1H/10)<<20
			| (alarm1H%10)<<16
			| (alarm1M/10)<<12
			| (alarm1M%10)<< 8
            | (alarm1S/10)<< 4
			| (alarm1S%10)
        	| (1U << 31); // mask date field

        /* leave init mode */
        RTC->ISR &= ~RTC_ISR_INIT;
        RTC->WPR = 0xFF;

        // arm or disarm Alarm A based on switch
        if (GPIOC->IDR & (1U << 15)) {
        	RTC->CR |= (RTC_CR_ALRAIE | RTC_CR_ALRAE);
        } else {
        	RTC->CR &= ~(RTC_CR_ALRAIE | RTC_CR_ALRAE);
        }
      }
      else if (currentState==3) {       // —– commit Alarm 2 —–
        RTC->WPR = 0xCA;
        RTC->WPR = 0x53;
        RTC->ISR |=  RTC_ISR_INIT;
        while(!(RTC->ISR & RTC_ISR_INITF));
        RTC->ALRMBR =
            (alarm2H/10)<<20
			| (alarm2H%10)<<16
            | (alarm2M/10)<<12
			| (alarm2M%10)<< 8
            | (alarm2S/10)<< 4
			| (alarm2S%10)
        	| (1U << 31); // mask date

        RTC->ISR &= ~RTC_ISR_INIT;
        RTC->WPR = 0xFF;

        // arm or disarm Alarm B based on switch
        if (GPIOC->IDR & (1U << 14)) {
        	RTC->CR |= (RTC_CR_ALRBIE | RTC_CR_ALRBE);
        } else {
        	RTC->CR &= ~(RTC_CR_ALRBIE | RTC_CR_ALRBE);
        }
      }

      updateValueFlag = 0;
    }
    prevEdit = editMode;

    /* 5) Display */
    // blank positions
    const uint8_t BL = 46;
    switch(currentState) {
      case 0: {
    	  //Clock mode -- HH.MM.SS -- NO LEDs
          if (editMode) {
            // flash off half the time
            if (((HAL_GetTick()/500)&1)==0) {
              for (int d = 0; d < 8; d++)
                Seven_Segment_Digit(d, BL, 0);
            } else {
              // live‐update from the knobs: HH.MM.SS
              int t, o;
              // hours
              t = hourSet/10;
              Seven_Segment_Digit(7, t, 0);
              o = hourSet%10;
              Seven_Segment_Digit(6, o, 1);
              // minutes
              t = minuteSet/10;
              Seven_Segment_Digit(5, t, 0);
              o = minuteSet%10;
              Seven_Segment_Digit(4, o, 1);
              // seconds
              t = secondSet/10;
              Seven_Segment_Digit(3, t, 0);
              o = secondSet%10;
              Seven_Segment_Digit(2, o, 0);
              // unused digits
              Seven_Segment_Digit(1, BL, 0);
              Seven_Segment_Digit(0, BL, 0);
            }
          } else {
            // normal running clock: pull from RTC->TR so it auto-increments
            uint32_t tr = RTC->TR;
            int hr_t = (tr >> 20) & 0x3;
            int hr_u = (tr >> 16) & 0xF;
            int mn_t = (tr >> 12) & 0x7;
            int mn_u = (tr >>  8) & 0xF;
            int sc_t = (tr >>  4) & 0x7;
            int sc_u = (tr >>  0) & 0xF;

            Seven_Segment_Digit(7, hr_t, 0);
            Seven_Segment_Digit(6, hr_u, 1);
            Seven_Segment_Digit(5, mn_t, 0);
            Seven_Segment_Digit(4, mn_u, 1);
            Seven_Segment_Digit(3, sc_t, 0);
            Seven_Segment_Digit(2, sc_u, 0);
            // unused
            Seven_Segment_Digit(1, BL, 0);
            Seven_Segment_Digit(0, BL, 0);
          }
          break;
      }
      case 1: {
        // Calendar YY.MM.DD -- PD0 lit
        if(editMode && ((HAL_GetTick()/500)&1)==0) {
          for(int d=0; d<8; d++) Seven_Segment_Digit(d, BL, 0);
        } else {
          char t,o;
          t = yearSet/10;   o = yearSet%10;
          Seven_Segment_Digit(7, t, 0);
		  Seven_Segment_Digit(6, o, 1);
          t = monthSet/10;  o = monthSet%10;
          Seven_Segment_Digit(5, t, 0);
          Seven_Segment_Digit(4, o, 1);
          t = daySet/10;    o = daySet%10;
          Seven_Segment_Digit(3, t, 0);
          Seven_Segment_Digit(2, o, 0);
          Seven_Segment_Digit(1, BL, 0);
          Seven_Segment_Digit(0, BL, 0);
        }
        break;
      }
      case 2: {
        // Alarm1 HH.MM.SS -- PD1 lit
        if(editMode && ((HAL_GetTick()/500)&1)==0) {
          for(int d=0; d<8; d++) Seven_Segment_Digit(d, BL, 0);
        } else {
          char t,o;
          t = alarm1H/10;    o = alarm1H%10;
          Seven_Segment_Digit(7, t, 0);
          Seven_Segment_Digit(6, o, 1);
          t = alarm1M/10;    o = alarm1M%10;
          Seven_Segment_Digit(5, t, 0);
		  Seven_Segment_Digit(4, o, 1);
          t = alarm1S/10;    o = alarm1S%10;
          Seven_Segment_Digit(3, t, 0);
          Seven_Segment_Digit(2, o, 0);
          Seven_Segment_Digit(1, BL, 0);
          Seven_Segment_Digit(0, BL, 0);
        }
        break;
      }
      case 3: {
        // Alarm2 HH.MM.SS -- PD0, PD1 lit
        if(editMode && ((HAL_GetTick()/500)&1)==0) {
          for(int d=0; d<8; d++) Seven_Segment_Digit(d, BL, 0);
        } else {
          char t,o;
          t = alarm2H/10;    o = alarm2H%10;
          Seven_Segment_Digit(7, t, 0);
          Seven_Segment_Digit(6, o, 1);
          t = alarm2M/10;    o = alarm2M%10;
          Seven_Segment_Digit(5, t, 0);
          Seven_Segment_Digit(4, o, 1);
          t = alarm2S/10;    o = alarm2S%10;
          Seven_Segment_Digit(3, t, 0);
          Seven_Segment_Digit(2, o, 0);
          Seven_Segment_Digit(1, BL, 0);
          Seven_Segment_Digit(0, BL, 0);
        }
        break;
      }
    }

    /* optional: show currentState on LEDs for debug */
    GPIOD->ODR = (GPIOD->ODR & ~0x3) | (currentState & 0x3);

  } /* USER CODE END WHILE */
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}






static void MX_TIM7_Init(void)
{

  /* USER CODE BEGIN TIM7_Init 0 */

  /* USER CODE END TIM7_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM7_Init 1 */

  /* USER CODE END TIM7_Init 1 */
  htim7.Instance = TIM7;
  htim7.Init.Prescaler = 0;
  htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim7.Init.Period = 65535;
  htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim7) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim7, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM7_Init 2 */

  /* USER CODE END TIM7_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CS_I2C_SPI_GPIO_Port, CS_I2C_SPI_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(OTG_FS_PowerSwitchOn_GPIO_Port, OTG_FS_PowerSwitchOn_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, LD4_Pin|LD3_Pin|LD5_Pin|LD6_Pin
                          |Audio_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : CS_I2C_SPI_Pin */
  GPIO_InitStruct.Pin = CS_I2C_SPI_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CS_I2C_SPI_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = OTG_FS_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(OTG_FS_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PDM_OUT_Pin */
  GPIO_InitStruct.Pin = PDM_OUT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(PDM_OUT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : BOOT1_Pin */
  GPIO_InitStruct.Pin = BOOT1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BOOT1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : CLK_IN_Pin */
  GPIO_InitStruct.Pin = CLK_IN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(CLK_IN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD4_Pin LD3_Pin LD5_Pin LD6_Pin
                           Audio_RST_Pin */
  GPIO_InitStruct.Pin = LD4_Pin|LD3_Pin|LD5_Pin|LD6_Pin
                          |Audio_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_OverCurrent_Pin */
  GPIO_InitStruct.Pin = OTG_FS_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(OTG_FS_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : MEMS_INT2_Pin */
  GPIO_InitStruct.Pin = MEMS_INT2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(MEMS_INT2_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
// this one will fire when Alarm A (ALRMAR) goes off:
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
	//test
	GPIOD->ODR ^= (1<<12);
  Music_ON = 1;
}

// this one will fire when Alarm B (ALRMBR) goes off:
void HAL_RTCEx_AlarmBEventCallback(RTC_HandleTypeDef *hrtc)
{
	//test
	GPIOD->ODR ^= (1<<13);
  Music_ON = 1;
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

#ifdef  USE_FULL_ASSERT
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
