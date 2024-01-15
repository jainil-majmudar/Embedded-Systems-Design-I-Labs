/****** Robert Li, 2023  
1.The First External button (named extBtn1)  is connected to PC1 (cannot use PB1, for 429i-DISCO ,pb1 is used by LCD), ExtBtn1_Config()  //
					cannot use pin PB1, for 429i-DISCO ,pb1 is used by LCD. if use this pin, always interrupt by itself
					can not use pin PA1, used by gyro. if use this pin, never interrupt
					pd1----WILL ACT AS PC13, To trigger the RTC timestamp event
					....ONLY PC1 CAN BE USED TO FIRE EXTI1 !!!!
2. the Second external button (extBtn2) is conected to  PD2.  ExtBtn2_Config() --The pin PB2 on the board have trouble.
    when connect external button to the pin PB2, the voltage at this pin is not 3V as it is supposed to be, it is 0.3V, why?
		so changed to use pin PD2.
		PA2: NOT OK. (USED BY LCD??)
		PB2: OK.
		PC2: ok, BUT sometimes (every 5 times around), press pc2 will trigger exti1, which is configured to use PC1. (is it because of using internal pull up pin config?)
		      however, press PC1 does not affect exti 2. sometimes press PC2 will also affect time stamp (PC13)
		PD2: OK,     
		PE2:  OK  (PE3, PE4 PE5 , seems has no other AF function, according to the table in the manual for discovery board)
		PF2: NOT OK. (although PF2 is used by SDRAM, it affects LCD. press it, LCD will flick and displayed chars change to garbage)
		PG2: OK
		

**********************************************/


#include "main.h"


#define COLUMN(x) ((x) * (((sFONT *)BSP_LCD_GetFont())->Width))    //see font.h, for defining LINE(X)



/*
ADC is set to PF6
PWM is set to PC8
Extern 1 is PC1
Extern 2 is PD2
*/


__IO uint16_t ADC3ConvertedValue=0;


 volatile double  setPoint=30.0;  //set inital setPoint to a higher C value to ensure that the fan stays off at initial reset point
 //NOTE: if declare as float, when +0.5, the compile will give warning:
															//"single_precision perand implicitly converted to double-precision"
															//ALTHOUGH IT IS A WARNING, THIS WILL MAKE THE PROGRAM not WORK!!!!!!
															//if declare as float, when setPoint+=0.5, need to cast : as setPioint+=(float)0.5, otherwise,
															//the whole program will not work! even this line has  not been used/excuted yet
															//BUT, if declare as double, there is no such problem.
															
	    														
	//			Project Options -> Target -> Floating Point Hardware -> Use FPU

	//You must then have code to enable the FPU hardware prior to using any FPU instructions. This is typically in the ResetHandler or SystemInit()

	//            system_stm32f4xx.c
  ////            void SystemInit(void)
	//							{
	//								/* FPU settings ------------------------------------------------------------*/
	//								#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
	//									SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));  /* set CP10 and CP11 Full Access */
	//								#endif
	//							...											
	//		-----MODIFY the system_stm32f4xx.c in the above way, will also fix the "float" type problem mentioned above. 												
	
double measuredTemp;
	/*
	according to the reference manual of STM32f4Discovery, the Vref+ should =VDD=VDDA=3.0V, while Vref-=VSSA=0
	so the voltage of 3V is mapped to 12 bits ADC result, which ranges from 0 to 4095.  
	(althoug ADC_DR register is 16 bits, ADC converted result is just of 12bits)   
	so the voltage resolution is 3/4095  (v/per_reading_number)   
	since the temperature is amplified 3 times before it is fed in MCU, the actual voltage from the temperature sensor is: 
	ADC_convertedvalue* (3/4095) /3. 
	
	since the temperature sensor sensitity is 10mV/C ,that is:  (0.01V/C)
	so the temperature is: ADC_convertedvalue* (3/4095) /3 /0.01 
	
	NOTE: you'd better not, or cannot, let the MCU to the calculation becuase its power is limited. Otherwise your program may not work as expected. 
	*/
	
int TIMPress1;
int TIMPress2;
double final_temp = 0;
double ADC_convertedvalue = 10000.0;

TIM_HandleTypeDef Tim3_Handle;
uint16_t Tim3_PrescalerValue,TIM3_Period;
int TIM3Prescaler=1799;
int TIM3Period=1000; 
TIM_OC_InitTypeDef Tim3_OCInitStructure;
__IO uint16_t TIM3_CCR1_Val=200, TIM3_CCR2_Val=400,TIM3_CCR3_Val=600,TIM3_CCR4_Val=800;
ADC_HandleTypeDef    AdcHandle;
__IO uint16_t uhADCxConvertedValue = 0;

void  LEDs_Config(void);
void  LEDs_On(void);
void  LEDs_Off(void);
void  LEDs_Toggle(void);

void TIM3_PWM_Config(void);
void ADC_Config(void);
void ExtBtn1_Config(void);
void ExtBtn2_Config(void);

void LCD_DisplayString(uint16_t LineNumber, uint16_t ColumnNumber, uint8_t *ptr);
void LCD_DisplayInt(uint16_t LineNumber, uint16_t ColumnNumber, int Number);
void LCD_DisplayFloat(uint16_t LineNumber, uint16_t ColumnNumber, float Number, int DigitAfterDecimalPoint);

static void SystemClock_Config(void);
static void Error_Handler(void);

int main(void){
		/* STM32F4xx HAL library initialization:
       - Configure the Flash prefetch, instruction and Data caches
       - Configure the Systick to generate an interrupt each 1 msec
       - Set NVIC Group Priority to 4
       - Global MSP (MCU Support Package) initialization
     */
		HAL_Init();
		
	
		 /* Configure the system clock to 180 MHz */
		SystemClock_Config();
		
		HAL_InitTick(0x0000); // set systick's priority to the highest.
	
		//Configure LED3 and LED4 ======================================
		LEDs_Config();
	
		//configure the USER button as exti mode. 
		BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_EXTI);   // BSP_functions in stm32f429i_discovery.c
									
		TIM3_PWM_Config();
		ADC_Config();
		ExtBtn1_Config();
		ExtBtn2_Config();
		Tim3_OCInitStructure.Pulse=0;
		(&Tim3_Handle,TIM_CHANNEL_3);// stops fan
		BSP_LCD_Init();
		//BSP_LCD_LayerDefaultInit(uint16_t LayerIndex, uint32_t FB_Address);
		BSP_LCD_LayerDefaultInit(0, LCD_FRAME_BUFFER);   //LCD_FRAME_BUFFER, defined as 0xD0000000 in _discovery_lcd.h
															// the LayerIndex may be 0 and 1. if is 2, then the LCD is dark.
		//BSP_LCD_SelectLayer(uint32_t LayerIndex);
		BSP_LCD_SelectLayer(0);
		//BSP_LCD_SetLayerVisible(0, ENABLE); //do not need this line.
		BSP_LCD_Clear(LCD_COLOR_WHITE);  //need this line, otherwise, the screen is dark	
		BSP_LCD_DisplayOn();
	 
		BSP_LCD_SetFont(&Font20);  //the default font,  LCD_DEFAULT_FONT, which is defined in _lcd.h, is Font24

//Tim3_OCInitStructure.Pulse=TIM3_CCR1_Val;   //200
	
		LCD_DisplayString(3, 1, (uint8_t *) "Lab 4 Group 45");
		LCD_DisplayString(9, 1, (uint8_t *) "Current ");
		LCD_DisplayString(10, 1, (uint8_t *)"setPoint");
		
	while(1) {	
		if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_1)==GPIO_PIN_RESET&&HAL_GetTick()>=TIMPress1 +500){//if increase button is held
			setPoint++; // increase set point
			TIMPress1=HAL_GetTick();
			LCD_DisplayFloat(10, 10, setPoint, 2); // update set point
		}
		if(HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_2)==GPIO_PIN_RESET&&HAL_GetTick()>=TIMPress2 +500){//if decrease button is held
			setPoint--; // decrease set point
			TIMPress2=HAL_GetTick();
			LCD_DisplayFloat(10, 10, setPoint, 2); // updaate set point
		}
		if(HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_2)==GPIO_PIN_SET&&HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_1)==GPIO_PIN_SET){
			LCD_DisplayFloat(10, 10, setPoint, 2); // display setpoint when nothing is happening
		}
		uint32_t initial_temp = 0; // initial temperature is 0
	  HAL_ADC_PollForConversion(&AdcHandle,100);
		initial_temp = HAL_ADC_GetValue(&AdcHandle); // gets value from temp sensor 
		final_temp = (double) initial_temp *(ADC_convertedvalue* ((3.0/4095.0) / (3.0 /0.01)));//convert to C
		LCD_DisplayFloat(9, 10, final_temp, 2); // displau current temp
		LCD_DisplayString(11, 1, (uint8_t *)"Speed");
		LCD_DisplayInt(11,10,Tim3_OCInitStructure.Pulse/200); // displays fan speed
		if(final_temp > setPoint && final_temp <= setPoint+2){ // speed 1 (lowest power) at difference under 2 deg
			HAL_TIM_PWM_Start(&Tim3_Handle,TIM_CHANNEL_3);
			Tim3_OCInitStructure.Pulse=TIM3_CCR1_Val;
		}
		else if(final_temp > setPoint+2 && final_temp <= setPoint+4){ // speed 2 at diff of 2 to 4 deg
			Tim3_OCInitStructure.Pulse=TIM3_CCR2_Val;
		}
		else if(final_temp > setPoint+4 && final_temp <= setPoint+6){ // speed 3 at diff of 4 to 6 deg
			Tim3_OCInitStructure.Pulse=TIM3_CCR3_Val;
		}
		else if(final_temp > setPoint+6 && final_temp <= setPoint+8){ // speed 4 (max power) at diff of 6 to 8 deg
			Tim3_OCInitStructure.Pulse=TIM3_CCR4_Val;
		}
		else if(final_temp < setPoint){// if final temp is below setpoint, stop
			HAL_TIM_PWM_Stop(&Tim3_Handle,TIM_CHANNEL_3);
			Tim3_OCInitStructure.Pulse=0;
		}
	} // end of while loop
	
}  //end of main





void  TIM3_PWM_Config(void)//Configuration of PWM copied from PWM example.
{

	/*******to make this PWM works with the sevo motor in the future,
 the cycle period will be designed as 20 ms
TIM3 TIM4 are 16 bits, TIM2 and TIM5 are 32 bits	
	TIM2---5 are on APB1 bus, which is 45MHZ
since the APB1 prescaller is not 1, the timer Freq is 2XFreq_of_APB1, that is 90Mhz.
so to make a cyle as 0.02 s, the Period	should be: 0.02/(1/90M)=1800,000, which is larger than allwed MAX period value 65536
so need to use a prescaler for the timer: if set the prescaller as 1800 (assign 1799 to the register), then the period can be 1000
*/
	
	
		/* -----------------------------------------------------------------------
    In this example TIM3 input clock (TIM3CLK) is set to 2 * APB1 clock (PCLK1), 
    since APB1 prescaler is different from 1.   
      TIM3CLK = 2 * PCLK1  
      PCLK1 = HCLK / 4 
      => TIM3CLK = HCLK / 2 = SystemCoreClock /2
    To get TIM3 counter clock at 10 KHz, the Prescaler is computed as following:
    Prescaler = (TIM3CLK / TIM3 counter clock) - 1
    Prescaler = ((SystemCoreClock /2) /10 KHz) - 1
       
    Note: 
     SystemCoreClock variable holds HCLK frequency and is defined in system_stm32f4xx.c file.
     Each time the core clock (HCLK) changes, user had to update SystemCoreClock 
     variable value. Otherwise, any configuration based on this variable will be incorrect.
     This variable is updated in three ways:
      1) by calling CMSIS function SystemCoreClockUpdate()
      2) by calling HAL API function HAL_RCC_GetSysClockFreq()
      3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency  
  ----------------------------------------------------------------------- */  
   
  /* Set TIM3 instance */
  Tim3_Handle.Instance = TIM3; //TIM3 is defined in stm32f429xx.h
   
  Tim3_Handle.Init.Period = TIM3_Period;; //pwm frequency? 
  Tim3_Handle.Init.Prescaler = Tim3_PrescalerValue;
  Tim3_Handle.Init.ClockDivision = 0;
  Tim3_Handle.Init.CounterMode = TIM_COUNTERMODE_UP;
	Tim3_Handle.Init.RepetitionCounter = 0;  //default is 0
	

	Tim3_Handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
 
 
	if(HAL_TIM_PWM_Init(&Tim3_Handle) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }
  
	//configure the PWM channel
	Tim3_OCInitStructure.OCMode=  TIM_OCMODE_PWM1; //TIM_OCMODE_TIMING;
	Tim3_OCInitStructure.OCFastMode=TIM_OCFAST_DISABLE;
	Tim3_OCInitStructure.OCPolarity=TIM_OCPOLARITY_HIGH;
	//Tim3_OCInitStructure.OCNPolarity = TIM_OCNPOLARITY_HIGH //complementary polarity. 
																	//This parameter is valid only for TIM1 and TIM8.
	//Tim3_OCInitStructure.OCIdleState = TIM_OCIDLESTATE_SET; //This parameter is valid only for TIM1 and TIM8.
  //Tim3_OCInitStructure.OCNIdleState= TIM_OCNIDLESTATE_RESET; //This parameter is valid only for TIM1 and TIM8.
	
	Tim3_OCInitStructure.Pulse=TIM3_CCR1_Val;   //200
	
	if(HAL_TIM_PWM_ConfigChannel(&Tim3_Handle, &Tim3_OCInitStructure, TIM_CHANNEL_1) != HAL_OK)
  {
    /* Configuration Error */
    Error_Handler();
  }
	
	
	Tim3_OCInitStructure.Pulse=TIM3_CCR2_Val;  //400
	
	if(HAL_TIM_PWM_ConfigChannel(&Tim3_Handle, &Tim3_OCInitStructure, TIM_CHANNEL_2) != HAL_OK)
  {
    /* Configuration Error */
    Error_Handler();
	}
	
	
	Tim3_OCInitStructure.Pulse=TIM3_CCR3_Val;   //600;
	
	if(HAL_TIM_PWM_ConfigChannel(&Tim3_Handle, &Tim3_OCInitStructure, TIM_CHANNEL_3) != HAL_OK)
  {
    /* Configuration Error */
    Error_Handler();
	}
	
	
	Tim3_OCInitStructure.Pulse=TIM3_CCR4_Val;   //800;
	
	if(HAL_TIM_PWM_ConfigChannel(&Tim3_Handle, &Tim3_OCInitStructure, TIM_CHANNEL_4) != HAL_OK)
  {
    /* Configuration Error */
    Error_Handler();
	}
	
	
	//***************************
	
if(HAL_TIM_PWM_Start(&Tim3_Handle, TIM_CHANNEL_1) != HAL_OK)
  {
    /* Starting Error */
    Error_Handler();
  }  
	


if(HAL_TIM_PWM_Start(&Tim3_Handle, TIM_CHANNEL_2) != HAL_OK)
  {
    /* Starting Error */
    Error_Handler();
  }  
	


if(HAL_TIM_PWM_Start(&Tim3_Handle, TIM_CHANNEL_3) != HAL_OK)
  {
    /* Starting Error */
    Error_Handler();
  }  
	


if(HAL_TIM_PWM_Start(&Tim3_Handle, TIM_CHANNEL_4) != HAL_OK)
  {
    /* Starting Error */
    Error_Handler();
  }  
}


void ADC_Config(void)//Configuration of ADC copied from ADC example.
{
	ADC_ChannelConfTypeDef sConfig = {0};
  
  AdcHandle.Instance          = ADCx;
  AdcHandle.Init.ClockPrescaler = ADC_CLOCKPRESCALER_PCLK_DIV2;
  AdcHandle.Init.Resolution = ADC_RESOLUTION_12B;
  AdcHandle.Init.ScanConvMode = DISABLE;
  AdcHandle.Init.ContinuousConvMode = ENABLE;
  AdcHandle.Init.DiscontinuousConvMode = DISABLE;
  AdcHandle.Init.NbrOfDiscConversion = 0;
  AdcHandle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  AdcHandle.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T1_CC1;
  AdcHandle.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  AdcHandle.Init.NbrOfConversion = 1;
  AdcHandle.Init.DMAContinuousRequests = DISABLE;
  AdcHandle.Init.EOCSelection = DISABLE;
      
  HAL_ADC_Init(&AdcHandle);
  sConfig.Channel = ADCx_CHANNEL;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  sConfig.Offset = 0;

  HAL_ADC_ConfigChannel(&AdcHandle, &sConfig);
  HAL_ADC_Start(&AdcHandle);
}


void ExtBtn1_Config(void)     // for GPIO C pin 1
// can only use PA0, PB0... to PA4, PB4 .... because only  only  EXTI0, ...EXTI4,on which the 
	//mentioned pins are mapped to, are connected INDIVIDUALLY to NVIC. the others are grouped! 
		//see stm32f4xx.h, there is EXTI0_IRQn...EXTI4_IRQn, EXTI15_10_IRQn defined
{
  GPIO_InitTypeDef   GPIO_InitStructure;

  /* Enable GPIOB clock */
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

  /* Enable GPIOB clock */
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


/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 180000000
  *            HCLK(Hz)                       = 180000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 8000000
  *            PLL_M                          = 8
  *            PLL_N                          = 360
  *            PLL_P                          = 2
  *            PLL_Q                          = 7
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
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 360;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;  
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

void LEDs_Config(void)
{
 /* Initialize Leds mounted on STM32F429-Discovery board */
	BSP_LED_Init(LED3);   //BSP_LED_....() are in stm32f4291_discovery.c
  BSP_LED_Init(LED4);
}

void LEDs_On(void){
/* Turn on LED3, LED4 */
 BSP_LED_On(LED3);
  BSP_LED_On(LED4);
}

void LEDs_Off(void){
/* Turn on LED3, LED4 */
  BSP_LED_Off(LED3);
  BSP_LED_Off(LED4);
}
void LEDs_Toggle(void){
/* Turn on LED3, LED4 */
  BSP_LED_Toggle(LED3);
  BSP_LED_Toggle(LED4);
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

  }
	
	
	if(GPIO_Pin == GPIO_PIN_1)//we check at which instance was the button pressed and store it
  {
		TIMPress1 = HAL_GetTick();
	}  //end of PIN_1

	if(GPIO_Pin == GPIO_PIN_2)//we check at which instance was the button pressed and store it
  {
		TIMPress2 = HAL_GetTick();
	} //end of if PIN_2	
	
	
}



void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef * htim) //see  stm32fxx_hal_tim.c for different callback function names. 
{																																//for timer4 
		//if((*htim).Instance==TIM4){
			 //BSP_LED_Toggle(LED4);
		//}
		//clear the timer counter!  in stm32f4xx_hal_tim.c, the counter is not cleared after  OC interrupt
		BSP_LED_Toggle(LED4);
		__HAL_TIM_SET_COUNTER(htim, 0x0000);   //this maro is defined in stm32f4xx_hal_tim.h
		
}
 
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef * htim){  //this is for TIM3_pwm
	
	//__HAL_TIM_SET_COUNTER(htim, 0x0000);   //not necessary  
}



static void Error_Handler(void)
{
  /* Turn LED4 on */
	//LCD_DisplayInt(0,0,1);
  BSP_LED_On(LED4);
  while(1)
  {//LCD_DisplayInt(0,0,1);
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



