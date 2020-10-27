/** 
  ******************************************************************************
  * @file    main.c
  * @brief   read ADC value and dim WS2812 RGBW LED accordingly
  * @author  simon.burkhardt@eta-systems.ch
  * @version 1.0
  * @date    2020-10-27
  * 
  * @note    stm8l050j3
  * @note    IAR Embedded Workbench 3.11.1
  *          shared components 8.3.2.5988
  * @see     http://embedded-lab.com/blog/starting-stm8-microcontrollers/6
  * @see     https://github.com/j3qq4hch/STM8_WS2812B
  * @see     STM8-SO8-Disco: User Manual UM2339
  * @note    RGBW LED Type: IN-PI55QATPRPGPBPW-XX
  * 
  ******************************************************************************
  * Copyright (c) 2020 eta Systems GmbH
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdio.h>
#include "stm8l15x_conf.h"
#include "stm8l15x_clk.h"
#include "stm8l15x_gpio.h"
#include "stm8l15x_adc.h"
#include "ws2812B_fx.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define TIM4_PERIOD       124
/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
uint16_t ADCdata = 0;  // is updated in stm8l15x_it.c

/* Private function prototypes -----------------------------------------------*/
static void CLK_Config(void);
static void TIM4_Config(void);
static void GPIO_Config(void);
static void ADC_Config(void);
void _delay_ms(u16);
void uptime_routine(void);

/* Private user code ---------------------------------------------------------*/
void main( void )
{  
  /* MCU Configuration -------------------------------------------------------*/
  CLK_Config();
  TIM4_Config();
  GPIO_Config();

  enableInterrupts();
  ADC_Config();
  
  RGBColor_t foo;
  foo.R = 0;
  foo.G = 0;
  foo.B = 0;
  foo.W = 0;
  rgb_SetColor(0, foo);
  rgb_SendArray();
  uint32_t cnt = 0;

  /* Infinite Loop -----------------------------------------------------------*/
  while(1)
  { 
    /* quickly fade 1st White LED */
    foo.R = 0;
    foo.W = ++cnt%48;
    rgb_SetColor(0, foo);
    
    /* represent ADC value as last Red LED */
    foo.R = (uint8_t) ADCdata;
    foo.W = 0;
    rgb_SetColor(7, foo);
    
    rgb_SendArray();  // update LEDs
    _delay_ms(10);
    
    //rainbowCycle(10); 
  }
}

/* Clock Init ----------------------------------------------------------------*/
/**
  * @brief  Configure system clock to run at Maximum clock speed and output the 
  *         system clock on CCO pin
  * @param  None
  * @retval None
  */
static void CLK_Config(void)
{
  CLK_DeInit();
  /* High speed internal clock prescaler: 1 */
  CLK_SYSCLKDivConfig(CLK_SYSCLKDiv_1);
  /* Select HSE as system clock source */
  CLK_SYSCLKSourceSwitchCmd(ENABLE);
  CLK_SYSCLKSourceConfig(CLK_SYSCLKSource_HSI);
  while (CLK_GetSYSCLKSource() != CLK_SYSCLKSource_HSI);
  
  /* Enable TIM4 clock */
  CLK_PeripheralClockConfig(CLK_Peripheral_TIM4, ENABLE);
  /* Enable ADC1 clock */
  CLK_PeripheralClockConfig(CLK_Peripheral_ADC1, ENABLE);
}

/* GPIO Init -----------------------------------------------------------------*/
/**
  * @brief  Configure GPIO Pins 
  * @param  None
  * @retval None
  */
static void GPIO_Config(void)
{
  GPIO_Init (GPIOD, GPIO_Pin_0, GPIO_Mode_Out_PP_Low_Fast); // LED Strip
  /* ADC Inputs don't need to be initialized */
  // GPIO_Init (GPIOC, GPIO_Pin_4, GPIO_Mode_In_FL_No_IT);     // ADC Input
}

/* Timer4 Init ---------------------------------------------------------------*/
/**
  * @brief  Configure TIM4 peripheral   
  * @param  None
  * @retval None
  */
static void TIM4_Config(void)
{
  /* TIM4 configuration:
   - TIM4CLK is set to 16 MHz, the TIM4 Prescaler is equal to 128 so the TIM1 counter
   clock used is 16 MHz / 128 = 125 000 Hz
  - With 125 000 Hz we can generate time base:
      max time base is 2.048 ms if TIM4_PERIOD = 255 --> (255 + 1) / 125000 = 2.048 ms
      min time base is 0.016 ms if TIM4_PERIOD = 1   --> (  1 + 1) / 125000 = 0.016 ms
  - In this example we need to generate a time base equal to 1 ms
   so TIM4_PERIOD = (0.001 * 125000 - 1) = 124 */

  /* Time base configuration */
  TIM4_TimeBaseInit(TIM4_Prescaler_128, TIM4_PERIOD);
  /* Clear TIM4 update flag */
  TIM4_ClearFlag(TIM4_FLAG_Update);
  /* Enable update interrupt */
  TIM4_ITConfig(TIM4_IT_Update, ENABLE);
  TIM4_Cmd(ENABLE);
}

/* ADC1 Init -----------------------------------------------------------------*/
/**
  * @brief  Configure ADC peripheral 
  * @param  None
  * @retval None
  */
static void ADC_Config(void)
{
  /* Initialise and configure ADC1 */
  ADC_Init(ADC1, ADC_ConversionMode_Continuous, ADC_Resolution_8Bit, ADC_Prescaler_2);
  ADC_SamplingTimeConfig(ADC1, ADC_Group_SlowChannels, ADC_SamplingTime_384Cycles);
  /* Enable ADC1 */
  ADC_Cmd(ADC1, ENABLE);
  /* Enable ADC1 Channel 3 */
  ADC_ChannelCmd(ADC1, ADC_Channel_4, ENABLE);
  /* Enable End of conversion ADC1 Interrupt */
  ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE);
  /* Start ADC1 Conversion using Software trigger*/
  ADC_SoftwareStartConv(ADC1);
}

static u32 uptime = 0;
static u32 delay_time = 0;
u8 z = 0;

/**
  * @brief  System Tick Counter for delay
  * @param  None
  * @retval None
  */
void uptime_routine(void)
{
  uptime++;
  if(uptime == 0xFFFFFFFF){
    delay_time = 0;
    uptime  = 0;
  }
  z++;
  if(z == 250){
    z = 0;
    // GPIO_WriteReverse(GPIOB, GPIO_Pin_5);
  }
}

/**
  * @brief  Delay Routine
  * @param  wait time in ms
  * @retval None
  */
void _delay_ms(u16 wait)
{ 
  delay_time = uptime + wait;
  while(delay_time > uptime){}
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param file: pointer to the source file name
  * @param line: assert_param error line source number
  * @retval : None
  */
void assert_failed(u8* file, u32 line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/******************************* (c) 2020 eta Systems GmbH *****END OF FILE****/
