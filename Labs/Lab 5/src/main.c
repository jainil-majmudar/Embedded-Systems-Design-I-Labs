/****** 

1. For GPIO pins, Both OD mode and PP mode can drive the motor! However, some pins cannot output  high in OD mode!!! 
   
2. The signals do not need to be inverted before being fed in H-bridge.   
*/


#include "main.h"


#define COLUMN(x) ((x) * (((sFONT *)BSP_LCD_GetFont())->Width))    //see font.h, for defining LINE(X)


void LCD_DisplayString(uint16_t LineNumber, uint16_t ColumnNumber, uint8_t *ptr);
void LCD_DisplayInt(uint16_t LineNumber, uint16_t ColumnNumber, int Number);
void LCD_DisplayFloat(uint16_t LineNumber, uint16_t ColumnNumber, float Number, int DigitAfterDecimalPoint);


static void SystemClock_Config(void);
static void Error_Handler(void);

TIM_HandleTypeDef Tim3_Handle;
uint16_t Tim3_PrescalerValue;
void TIM3_Config(void);

void OUTPUT1_Config(void);
void OUTPUT2_Config(void);
void OUTPUT3_Config(void);
void OUTPUT4_Config(void);

void ExtBtn1_Config(void);
void ExtBtn2_Config(void);
void ExtBtn3_Config(void);

int full_step_half_step = 0; // full step is 0 and half step = 1
int cw_ccw = 0; // clockwise is 0 and counter clockwise is 1
int steps = 0;
int period = 9600;

void resetSteps (void); //reset the the steps and outputs based on the full step, half step, or direction

int main(void){
	
		/* STM32F4xx HAL library initialization:
       - Configure the Flash prefetch, instruction and Data caches
       - Configure the Systick to generate an interrupt each 1 msec
       - Set NVIC Group Priority to 4
       - Global MSP (MCU Support Package) initialization
     */
		HAL_Init();
		
	
		 /* Configure the system clock to 72 MHz */
		SystemClock_Config();
		
		HAL_InitTick(0x0000); // set systick's priority to the highest.
	
		BSP_LCD_Init();
		//BSP_LCD_LayerDefaultInit(uint16_t LayerIndex, uint32_t FB_Address);
		BSP_LCD_LayerDefaultInit(0, LCD_FRAME_BUFFER);   //LCD_FRAME_BUFFER, defined as 0xD0000000 in _discovery_lcd.h
															// the LayerIndex may be 0 and 1. if is 2, then the LCD is dark.
		//BSP_LCD_SelectLayer(uint32_t LayerIndex);
		BSP_LCD_SelectLayer(0);
		BSP_LCD_SetLayerVisible(0, ENABLE); 
		
		BSP_LCD_Clear(LCD_COLOR_WHITE);  //need this line, otherwise, the screen is dark	
		BSP_LCD_DisplayOn();
	 
		BSP_LCD_SetFont(&Font20);  //the default font,  LCD_DEFAULT_FONT, which is defined in _lcd.h, is Font24
	
		LCD_DisplayString(2, 3, (uint8_t *)"Group 45 - Lab 5");	
		
		BSP_LED_Init(LED4);
		BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_EXTI); 
			
	
		OUTPUT1_Config();
		OUTPUT2_Config();
		OUTPUT3_Config();
		OUTPUT4_Config();
		TIM3_Config();
		ExtBtn1_Config();
		ExtBtn2_Config();
		ExtBtn3_Config();
		
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,1);
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_14,0);
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_15,0);
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_4,0);
		
		while(1) {	

			
			
		} // end of while loop
	
}  //end of main

void  TIM3_Config(void)
{
 
  
  Tim3_PrescalerValue = (uint32_t) ((SystemCoreClock) / 10000) - 1;
  
  Tim3_Handle.Instance = TIM3; //TIM3 is defined in stm32f429xx.h
   
  Tim3_Handle.Init.Period = period - 1;
  Tim3_Handle.Init.Prescaler = Tim3_PrescalerValue;
  Tim3_Handle.Init.ClockDivision = 0;
  Tim3_Handle.Init.CounterMode = TIM_COUNTERMODE_UP;
	HAL_TIM_Base_Init(&Tim3_Handle);
	HAL_TIM_Base_Start_IT(&Tim3_Handle);
	
}

void OUTPUT1_Config(void){ //PC13
	GPIO_InitTypeDef   GPIO_InitStructure;
	
	__HAL_RCC_GPIOC_CLK_ENABLE();
	GPIO_InitStructure.Mode =  GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructure.Pull = GPIO_NOPULL;
  GPIO_InitStructure.Pin = GPIO_PIN_13;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);
}

void OUTPUT2_Config(void){ //PC14
	GPIO_InitTypeDef   GPIO_InitStructure;
	
	__HAL_RCC_GPIOC_CLK_ENABLE();
	GPIO_InitStructure.Mode =  GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructure.Pull = GPIO_NOPULL;
  GPIO_InitStructure.Pin = GPIO_PIN_14;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);
}

void OUTPUT3_Config(void){ //PC15
	GPIO_InitTypeDef   GPIO_InitStructure;
	
	__HAL_RCC_GPIOC_CLK_ENABLE();
	GPIO_InitStructure.Mode =  GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructure.Pull = GPIO_NOPULL;
  GPIO_InitStructure.Pin = GPIO_PIN_15;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);
}

void OUTPUT4_Config(void){ //PC4
	GPIO_InitTypeDef   GPIO_InitStructure;
	
	__HAL_RCC_GPIOC_CLK_ENABLE();
	GPIO_InitStructure.Mode =  GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructure.Pull = GPIO_NOPULL;
  GPIO_InitStructure.Pin = GPIO_PIN_4;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);
}

void ExtBtn1_Config(void)     // for GPIO C pin 1
// can only use PA0, PB0... to PA4, PB4 .... because only  only  EXTI0, ...EXTI4,on which the 
	//mentioned pins are mapped to, are connected INDIVIDUALLY to NVIC. the others are grouped! 
		//see stm32f4xx.h, there is EXTI0_IRQn...EXTI4_IRQn, EXTI15_10_IRQn defined
{
  GPIO_InitTypeDef   GPIO_InitStructure;

  /* Enable GPIOC clock */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  
  /* Configure PA0 pin as input floating */
  GPIO_InitStructure.Mode =  GPIO_MODE_IT_FALLING;
  GPIO_InitStructure.Pull =GPIO_PULLUP;
  GPIO_InitStructure.Pin = GPIO_PIN_1;
	//GPIO_InitStructure.Speed=GPIO_SPEED_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

	//__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_1);   //is defined the same as the __HAL_GPIO_EXTI_CLEAR_FLAG(GPIO_PIN_1); ---check the hal_gpio.h
	//__HAL_GPIO_EXTI_CLEAR_FLAG(GPIO_PIN_1);// after moving the chunk of code in the GPIO_EXTI callback from _it.c (before these chunks are in _it.c)
																					//the program "freezed" when start, suspect there is a interupt pending bit there. Clearing it solve the problem.
  /* Enable and set EXTI Line0 Interrupt to the lowest priority */
  HAL_NVIC_SetPriority(EXTI1_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);
}

void ExtBtn2_Config(void){  //**********PD2.***********
	  GPIO_InitTypeDef   GPIO_InitStructure;

  /* Enable GPIOD clock */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  
  /* Configure PA0 pin as input floating */
  GPIO_InitStructure.Mode =  GPIO_MODE_IT_FALLING;
  GPIO_InitStructure.Pull =GPIO_PULLUP;
  GPIO_InitStructure.Pin = GPIO_PIN_2;
	//GPIO_InitStructure.Speed=GPIO_SPEED_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStructure);

	//__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_1);   //is defined the same as the __HAL_GPIO_EXTI_CLEAR_FLAG(GPIO_PIN_1); ---check the hal_gpio.h
	__HAL_GPIO_EXTI_CLEAR_FLAG(GPIO_PIN_2);// after moving the chunk of code in the GPIO_EXTI callback from _it.c (before these chunks are in _it.c)
																					//the program "freezed" when start, suspect there is a interupt pending bit there. Clearing it solve the problem.
  /* Enable and set EXTI Line0 Interrupt to the lowest priority */
  HAL_NVIC_SetPriority(EXTI2_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);
	
}
void ExtBtn3_Config(void){  //**********PC3.***********
	  GPIO_InitTypeDef   GPIO_InitStructure;

  /* Enable GPIOC clock */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  
  /* Configure PA0 pin as input floating */
  GPIO_InitStructure.Mode =  GPIO_MODE_IT_FALLING;
  GPIO_InitStructure.Pull = GPIO_PULLUP;
  GPIO_InitStructure.Pin = GPIO_PIN_3;
	//GPIO_InitStructure.Speed=GPIO_SPEED_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

	//__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_1);   //is defined the same as the __HAL_GPIO_EXTI_CLEAR_FLAG(GPIO_PIN_1); ---check the hal_gpio.h
	__HAL_GPIO_EXTI_CLEAR_FLAG(GPIO_PIN_3);// after moving the chunk of code in the GPIO_EXTI callback from _it.c (before these chunks are in _it.c)
																					//the program "freezed" when start, suspect there is a interupt pending bit there. Clearing it solve the problem.
  /* Enable and set EXTI Line0 Interrupt to the lowest priority */
  HAL_NVIC_SetPriority(EXTI3_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);
	
}

void resetSteps(void){
	steps = 0;
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,1);
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_14,0);
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_15,0);
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_4,0);
	
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 

* 					Oscillator											=HSE
	*    				HSE frequencey 										=8,000,000   (8MHz)
	*      ----However, if the project is created by uVision, the default HSE_VALUE is 25MHz. thereore, need to define HSE_VALUE
	*						PLL Source											=HSE
  *            PLL_M                          = 4
  *            PLL_N                          = 72
  *            PLL_P                          = 2
  *            PLL_Q                          = 3
  *        --->therefore, PLLCLK =8MHz X N/M/P=72MHz   
	*            System Clock source            = PLL (HSE)
  *        --> SYSCLK(Hz)                     = 72,000,000
  *            AHB Prescaler                  = 1
	*        --> HCLK(Hz)                       = 72 MHz
  *            APB1 Prescaler                 = 2
	*        --> PCLK1=36MHz,  -->since TIM2, TIM3, TIM4 TIM5...are on APB1, thiese TIMs CLK is 36X2=72MHz
							 	
  *            APB2 Prescaler                 = 1
	*        --> PCLK1=72MHz 

  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 5
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();
  
  /* The voltage scaling allows optimizing the power consumption when the device is 
     clocked below the maximum system frequency, to update the voltage scaling value 
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  
  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /* Activate the Over-Drive mode */
  HAL_PWREx_EnableOverDrive();
 
  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;  
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}


void LCD_DisplayString(uint16_t LineNumber, uint16_t ColumnNumber, uint8_t *ptr)
{  
  //here the LineNumber and the ColumnNumber are NOT  pixel numbers!!!
		while (*ptr!=NULL)
    {
				BSP_LCD_DisplayChar(COLUMN(ColumnNumber),LINE(LineNumber), *ptr); //new version of this function need Xpos first. so COLUMN() is the first para.
				ColumnNumber++;
			 //to avoid wrapping on the same line and replacing chars 
				if ((ColumnNumber+1)*(((sFONT *)BSP_LCD_GetFont())->Width)>=BSP_LCD_GetXSize() ){
					ColumnNumber=0;
					LineNumber++;
				}
					
				ptr++;
		}
}

void LCD_DisplayInt(uint16_t LineNumber, uint16_t ColumnNumber, int Number)
{  
  //here the LineNumber and the ColumnNumber are NOT  pixel numbers!!!
		char lcd_buffer[15];
		sprintf(lcd_buffer,"%d",Number);
	
		LCD_DisplayString(LineNumber, ColumnNumber, (uint8_t *) lcd_buffer);
}

void LCD_DisplayFloat(uint16_t LineNumber, uint16_t ColumnNumber, float Number, int DigitAfterDecimalPoint)
{  
  //here the LineNumber and the ColumnNumber are NOT  pixel numbers!!!
		char lcd_buffer[15];
		
		sprintf(lcd_buffer,"%.*f",DigitAfterDecimalPoint, Number);  //6 digits after decimal point, this is also the default setting for Keil uVision 4.74 environment.
	
		LCD_DisplayString(LineNumber, ColumnNumber, (uint8_t *) lcd_buffer);
}



void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	
		if(GPIO_Pin == KEY_BUTTON_PIN)  //GPIO_PIN_0
		{
				if(full_step_half_step==1){
					full_step_half_step = 0;
					resetSteps();
				}
				else if(full_step_half_step==0){
					full_step_half_step = 1;
					resetSteps();
				}
		}
		
		
		if(GPIO_Pin == GPIO_PIN_1)
		{
				if(cw_ccw==1){
					cw_ccw = 0;
					resetSteps();
				}
				else if(cw_ccw==0){
					cw_ccw = 1;
					resetSteps();
				}
				
			
		}  //end of PIN_1

		if(GPIO_Pin == GPIO_PIN_2) {
			period+=600;
			TIM3_Config();
		} //end of if PIN_2	
		
		if(GPIO_Pin == GPIO_PIN_3)
		{
			period-=600;
			TIM3_Config();		
				
		} //end of if PIN_3
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)   //see  stm32fxx_hal_tim.c for different callback function names. 
																															//for timer 3 , Timer 3 use update event initerrupt
{

	if(full_step_half_step == 0 && cw_ccw == 0){ //FULL STEP AND CW
		if(steps == 0){
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,0);
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_14,1);
			steps = 1;
		}
		else if(steps == 1){
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_14,0);
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_4,1);
			steps = 2;
		}
		else if(steps == 2){
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_4,0);
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_15,1);
			steps = 3;
		}
		else if(steps == 3){
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_15,0);
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,1);
			steps = 0;
		}
	}
	else if(full_step_half_step == 1 && cw_ccw == 0){ //HALF STEP AND CW
		if(steps == 0){
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_14,1);
			steps = 1;
		}
		else if(steps == 1){
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,0);
			steps = 2;
		}
		else if(steps == 2){
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_4,1);
			steps = 3;
		}
		else if(steps == 3){
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_14,0);
			steps = 4;
		}
		else if(steps == 4){
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_15,1);
			steps = 5;
		}
		else if(steps == 5){
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_4,0);
			steps = 6;
		}
		else if(steps == 6){
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,1);
			steps = 7;
		}
		else if(steps == 7){
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_15,0);
			steps = 0;
		}
	}
	else if(full_step_half_step == 0 && cw_ccw == 1){
		if(steps==0){
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,0);
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_15,1);
			steps = 1;
		}
		else if(steps==1){
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_15,0);
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_4,1);
			steps = 2;
		}
		else if(steps==2){
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_4,0);
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_14,1);
			steps = 3;
		}
		else if(steps==3){
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_14,0);
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,1);
			steps = 0;
		}
	}
	else if(full_step_half_step == 1 && cw_ccw == 1){ 
		if(steps==0){
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_14,0);
			steps = 7;
		}
		else if(steps == 1){
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,1);
			steps = 0;
		}
		else if(steps == 2){
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_4,0);
			steps = 1;
		}
		else if(steps == 3){
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_14,1);
			steps = 2;
		}
		else if(steps == 4){
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_15,0);
			steps = 3;
		}
		else if(steps == 5){
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_4,1);
			steps = 4;
		}
		else if(steps == 6){
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,0);
			steps = 5;
		}
		else if(steps == 7){
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_15,1);
			steps = 6;
		}
	}
}


void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef * htim) //see  stm32fxx_hal_tim.c for different callback function names. 
{																																//for timer4 
				//clear the timer counter!  in stm32f4xx_hal_tim.c, the counter is not cleared after  OC interrupt
				//__HAL_TIM_SET_COUNTER(htim, 0x0000);   //this maro is defined in stm32f4xx_hal_tim.h

			
	
}
 
static void Error_Handler(void)
{
  /* Turn LED4 on */
  BSP_LED_On(LED4);
  while(1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif
/**
  * @}
  */

/**
  * @}
  */



