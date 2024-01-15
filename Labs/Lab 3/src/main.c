/****** Robert Li,  2023  


This starter kit: 

1. set up I2C and tested I2C and EEPROM

2. Configured RTC and RTC_Alarm and tested RTC


3 For External buttons:  
1):  The Fist External button (named extBtn1) seems can only be connected to PC1.  
	(cannot use pin PB1, for 429i-DISCO ,pb1 is used by LCD. if use this pin, always interrupt by itself
	cannot use pin PA1, used by gyro. if use this pin, never interrupt
		pd1----WILL ACT AS PC13, To trigger the RTC timestamp event)
					
2) the Second external button (extBtn2) may be conected to  PD2.  

		PA2: NOT OK. (USED BY LCD??)
		PB2: OK.
		PC2: ok, BUT sometimes (every 5 times around), press pc2 will trigger exti1, which is configured to use PC1. (is it because of using internal pull up pin config?)
		      however, press PC1 does not affect exti 2. sometimes press PC2 will also affect time stamp (PC13)
		PD2: OK,     
		PE2:  OK  (PE3, PE4 PE5 , seems has no other AF function, according to the table in the manual for discovery board)
		PF2: NOT OK. (although PF2 is used by SDRAM, it affects LCD. press it, LCD will flick and displayed chars change to garbage)
		PG2: OK
		 
*/


/* Includes ------------------------------------------------------------------*/
#include "main.h"


/** @addtogroup STM32F4xx_HAL_Examples
  * @{
  */

/** @addtogroup GPIO_EXTI
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/

typedef enum{
	CURRENT_TIME,
	DATE_TIME,
	DISPLAY_TIME,
	SET_YEAR,
	SET_MONTH,
	SET_DAY,
	SET_HOUR,
	SET_MINUTE,
	SET_SECOND
} state_e;

typedef struct
{
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
} time_t;

volatile uint8_t UB_Pressed;
volatile uint8_t UB_Held;
volatile uint8_t Extern_1;
volatile uint8_t Extern_2;

/* Private define ------------------------------------------------------------*/
#define COLUMN(x) ((x) * (((sFONT *)BSP_LCD_GetFont())->Width))    //see font.h, for defining LINE(X)



/* Private macro -------------------------------------------------------------*/

 

/* Private variables ---------------------------------------------------------*/
int time = 0;

state_e state;

volatile uint8_t RTC_INT_TRIGGERED = 1;               // RTC Interrupt Flag

I2C_HandleTypeDef  I2c3_Handle;

RTC_HandleTypeDef RTCHandle;
RTC_DateTypeDef RTC_DateStructure, read_RTC_DateStruct;
RTC_TimeTypeDef RTC_TimeStructure, read_RTC_TimeStruct;



HAL_StatusTypeDef Hal_status;  //HAL_ERROR, HAL_TIMEOUT, HAL_OK, of HAL_BUSY 



//memory location to write to in the device
__IO uint16_t memLocation = 0x000A; //pick any location within range

char lcd_buffer[14];

void RTC_Config(void);
void RTC_AlarmAConfig(void);

void ExtBtn1_Config(void);  //for the first External button
void ExtBtn2_Config(void);

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void Error_Handler(void);

void displayTime (void);
void clear();
void displayDate();
void displayPastTimes();

void incrementYear();
void incrementMonth();
void incrementDay();
void incrementHour();
void incrementMinute();
void incrementSecond();
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
	state = CURRENT_TIME;
  clear();

	
  //the following variables are for testging I2C_EEPROM

	uint8_t data1 =0x67,  data2=0x68;
	uint8_t readData=0x00;
	char AA[34]= "efghijklmnopqstuvefghijklmnopqstuv";
	uint8_t * bufferdata=(uint8_t *)AA;	
	int i;
	uint8_t readMatch=1;
	uint32_t EE_status;

	
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

	//configure the USER button as exti mode. 
	BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_EXTI);   // BSP_functions in stm32f429i_discovery.c


//Init LCD
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

	RTC_Config();
	
	RTC_AlarmAConfig();
	ExtBtn1_Config();
	ExtBtn2_Config();
	
	I2C_Init(&I2c3_Handle);
	


/*
*********************Testing I2C EEPROM------------------

	LCD_DisplayString(4, 2, (uint8_t *)"MT2TA4 LAB 3");
	LCD_DisplayString(6, 0, (uint8_t *)"Testing I2C & EEPROM....");
		
	HAL_Delay(2000);   //display for 1 second
		
	//BSP_LCD_Clear(LCD_COLOR_WHITE);
	BSP_LCD_ClearStringLine(4);
	HAL_Delay(5);   //have trouble to clear line 6 if there is no Delay!!??
	BSP_LCD_ClearStringLine(6);
	HAL_Delay(5);
	BSP_LCD_ClearStringLine(7);
	


	EE_status=I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation , data1);
	if (EE_status==HAL_OK)
			LCD_DisplayString(0, 0, (uint8_t *)"w data1 in EE OK");
	else
			LCD_DisplayString(0, 0, (uint8_t *)"w data1 failed");

	EE_status=I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation+1 , data2);
	if (EE_status==HAL_OK)
			LCD_DisplayString(1, 0, (uint8_t *)"w data2 in EE OK");
	else
			LCD_DisplayString(1, 0, (uint8_t *)"w data2 failed");
	
	
	
	
	readData=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation);
	if (data1 == readData) {
		LCD_DisplayString(3, 0, (uint8_t *)"r data1 OK:");
			LCD_DisplayInt(3, 13, readData);
	}else{
			LCD_DisplayString(3, 0, (uint8_t *)"data1 mismatch");
	}	
	
	
	readData=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation+1);
	if (data2 == readData) {
		LCD_DisplayString(4, 0, (uint8_t *)"r data2 OK:");
			LCD_DisplayInt(4, 13, readData);
	}else{
			LCD_DisplayString(4, 0, (uint8_t *)"data2 mismatch");
	}	
	
	

	EE_status=I2C_BufferWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation, bufferdata, 34);
	if (EE_status==HAL_OK)
			LCD_DisplayString(6, 0, (uint8_t *)"w EE buffer OK");
	else
			LCD_DisplayString(6, 0, (uint8_t *)"w buffer failed");

	for (i=0;i<=33;i++) { 
			readData=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation+i);
			HAL_Delay(50);   // just for display effect. for EEPROM read, do not need dalay
						//BUT if here delay longer time, the following display will hve trouble. ???
			//LCD_DisplayInt(8, i, (uint8_t)EE_status);
	
			BSP_LCD_DisplayChar(COLUMN(i%16),LINE(8+ 2*(int)(i/16)), (char) readData);	
			BSP_LCD_DisplayChar(COLUMN(i%16),LINE(9+ 2*(int)(i/16)),  bufferdata[i]);
			if (bufferdata[i]!=readData)
					readMatch=0;
	}

	if (readMatch==0)
				LCD_DisplayString(15, 0, (uint8_t *)"rd buffer mismatch");
	else 
				LCD_DisplayString(15, 0, (uint8_t *)"rd EE buffer OK");

	HAL_Delay(2000);  //display for 4 seconds

	
	
	//configure real-time clock
	RTC_Config();
		
	RTC_AlarmAConfig();
	
	
 //test realtime clock	
    BSP_LCD_Clear(LCD_COLOR_WHITE);
		HAL_Delay(10);   
		//otherwise, the following line will have trouble to display. 1. while cannot delay too long.??? 
		LCD_DisplayString(1, 0, (uint8_t *)"Testing RTC...");
	
	
		RTC_DateStructure.Month=11;
		RTC_DateStructure.WeekDay=1;   //what will happen if the date/weekday is not correct?
		RTC_DateStructure.Date=11;
		RTC_DateStructure.Year=11; //2012???  how about 1912?
		
		HAL_RTC_SetDate(&RTCHandle, &RTC_DateStructure, RTC_FORMAT_BIN);
	
	
		RTC_TimeStructure.Hours=11;
		RTC_TimeStructure.Minutes=11;   
		RTC_TimeStructure.Seconds=11;
		
		HAL_RTC_SetTime(&RTCHandle, &RTC_TimeStructure, RTC_FORMAT_BIN);
	
		
		//Read from RTC
		HAL_RTC_GetTime(&RTCHandle, &read_RTC_TimeStruct, RTC_FORMAT_BIN);
		HAL_RTC_GetDate(&RTCHandle, &read_RTC_DateStruct, RTC_FORMAT_BIN); 
		//NOTE from the UM1725 : You must call HAL_RTC_GetDate() after HAL_RTC_GetTime() to unlock the values in the higher order 
		//calendar shadow register ....		
	
		LCD_DisplayString(3,0, (uint8_t *) "HH:MM:SS");
	
		LCD_DisplayInt(4,0,read_RTC_TimeStruct.Hours);
		LCD_DisplayInt(4,3,read_RTC_TimeStruct.Minutes);
		LCD_DisplayInt(4,6,read_RTC_TimeStruct.Seconds);

		LCD_DisplayString(6,0, (uint8_t *) "WD:DD:MM:YY");
	
		LCD_DisplayInt(7,0, read_RTC_DateStruct.WeekDay);
		LCD_DisplayInt(7,3, read_RTC_DateStruct.Date);
		LCD_DisplayInt(7,6, read_RTC_DateStruct.Month);
		LCD_DisplayInt(7,9, read_RTC_DateStruct.Year);
 
	
	*/
	/* Infinite loop */

	BSP_LED_Init(LED3);
	BSP_LED_Off(LED3);

	
while (1)
  {			
		
		if (HAL_GPIO_ReadPin(GPIOA,KEY_BUTTON_PIN) == 1) {
      time=HAL_GetTick();
      while(HAL_GPIO_ReadPin(GPIOA,KEY_BUTTON_PIN) == 1) {  // while the selection button is pressed
        if ((HAL_GetTick()-time)>200) { // if time is greater than 0.2 seconds
          UB_Pressed = 0;
          UB_Held = 1;
					break;
        }
      }
    }
	

		switch(state){
			case CURRENT_TIME:
				LCD_DisplayString(1,0,(uint8_t *)"Current Time");
				displayTime(); // calls the displayTime() function to display the current time

				
				if (UB_Held){
						state = DATE_TIME; // changes the state to DATE_TIME
						clear();
		
				}
					
				if (Extern_1){
					  BSP_LCD_Clear(LCD_COLOR_WHITE);
						state = DISPLAY_TIME; // changes to the DISPLAY_TIME screen 
						clear();

				}
				
				if (Extern_2){
					state = SET_HOUR; // changes to SET_HOUR
					displayTime();
					displayDate();
					clear();
				}
				break;
				
			case DISPLAY_TIME:
				LCD_DisplayString(1,0,(uint8_t *)"Previous Times");
				displayPastTimes(); // calls the displayPastTimes() function to get the previous times
			
			  if (Extern_1){
					BSP_LCD_Clear(LCD_COLOR_WHITE);
					state = CURRENT_TIME; 
					clear();
				}
				break;
				
			case DATE_TIME:
				LCD_DisplayString(1,0,(uint8_t *)"Current Time&Date");
				displayTime(); // displays the current time
				displayDate(); // displays the date
			

				if(!HAL_GPIO_ReadPin(GPIOA,KEY_BUTTON_PIN)){ // if the user button is released
					clear();
					BSP_LCD_Clear(LCD_COLOR_WHITE);
					state=CURRENT_TIME; // changes the state to CURRENT_TIME
				}
				break;
				
			case SET_HOUR: //
			  LCD_DisplayString(1,0,(uint8_t *)"Set Time & Date");
				if (Extern_1){ // if external button 1 pressed
					clear();
					HAL_Delay(20);
					LCD_DisplayString(4,0,(uint8_t *)"  ");
					incrementHour(); // increments the hour by one if external button 1 is pressed 
					LCD_DisplayInt(4,0,RTC_TimeStructure.Hours);
				}
				//if ext button 2 pressed 
				if (Extern_2){
					//change to SET_MONTH
					state = SET_MINUTE; // if external button 2 is pressed it will go to the next state to get incremented which is the minutes
					clear();
				}
				break;
				
			case SET_MINUTE:
				//if ext button 1 pressed
				if (Extern_1){
					clear();
					HAL_Delay(20);
					LCD_DisplayString(4,3,(uint8_t *)"  ");
					incrementMinute(); // increments the minutes by one if external button 1 is pressed 
					LCD_DisplayInt(4,3,RTC_TimeStructure.Minutes);
				}
				//if ext button 2 pressed
				if (Extern_2){
					//change to SET_HOUR
					state = SET_SECOND; // if external button 2 is pressed it will go to the next state to get incremented which is the seconds
					clear();
				}
				break;
			case SET_SECOND:
				if (Extern_1){
					clear();
					HAL_Delay(20);
					LCD_DisplayString(4,6,(uint8_t *)"  ");
					incrementSecond(); // increments the seconds by one if external button 1 is pressed 
					LCD_DisplayInt(4,6,RTC_TimeStructure.Seconds);

					;
				}
				if (Extern_2){
					state = SET_MONTH; // if external button 2 is pressed it will go to the next state to get incremented which is the months
				}
				break;
				
			case SET_MONTH:
				if (Extern_1){
					clear();
					HAL_Delay(20);
					LCD_DisplayString(7,0,(uint8_t *)"  ");
					incrementMonth(); // increments the months by one if external button 1 is pressed 
					LCD_DisplayInt(7,0,RTC_DateStructure.Month);
					
				}
				if (Extern_2){
					state = SET_DAY; // if external button 2 is pressed it will go to the next state to get incremented which is the days
					clear();
				}
				break;

			case SET_DAY:
				//if ext button 1 pressed
				if (Extern_1){
					clear();
					HAL_Delay(20);
					LCD_DisplayString(7,3,(uint8_t *)"  ");
					incrementDay(); // increments the days by one if external button 1 is pressed 
					LCD_DisplayInt(7,3,RTC_DateStructure.Date);
					
				}
				if (Extern_2){
					state = SET_YEAR; // if external button 2 is pressed it will go to the next state to get incremented which is the years
					clear();
				}
				break;
			
			case SET_YEAR:
				if (Extern_1){
					clear();
					HAL_Delay(20);
					LCD_DisplayString(7,6,(uint8_t *)"  ");
					incrementYear(); // increments the years by one if external button 1 is pressed 
					LCD_DisplayInt(7,6,RTC_DateStructure.Year);
					
				}
				if (Extern_2){
					HAL_RTC_SetTime(&RTCHandle, &RTC_TimeStructure, RTC_FORMAT_BIN);
					HAL_RTC_SetDate(&RTCHandle, &RTC_DateStructure, RTC_FORMAT_BIN);
					state = CURRENT_TIME; // if external button 2 is pressed it will go to the next state which would be back to the current time screen
					BSP_LCD_Clear(LCD_COLOR_WHITE);
					clear();
				}

				break;

		}
	
		
		
  } //end of while


}  
	

void clear(){ // clears all the variables
	UB_Pressed = 0;
	UB_Held = 0;
	Extern_1 = 0;
  Extern_2 = 0;
}

void displayTime(){ // displays the current time 
	
		HAL_RTC_GetTime(&RTCHandle, &RTC_TimeStructure, RTC_FORMAT_BIN);
		HAL_RTC_GetDate(&RTCHandle, &RTC_DateStructure, RTC_FORMAT_BIN);
		HAL_Delay(250);
	  LCD_DisplayString(4,1,(uint8_t *)" ");
	  LCD_DisplayString(4,4,(uint8_t *)" ");
  	LCD_DisplayString(4,7,(uint8_t *)" ");
		LCD_DisplayString(3,0, (uint8_t *) "HH:MM:SS");
	
		LCD_DisplayInt(4,0,RTC_TimeStructure.Hours);
		LCD_DisplayInt(4,3,RTC_TimeStructure.Minutes);
		LCD_DisplayInt(4,6,RTC_TimeStructure.Seconds);
}

void displayDate(){ // displays the date 
		HAL_RTC_GetTime(&RTCHandle, &RTC_TimeStructure, RTC_FORMAT_BIN);
		HAL_RTC_GetDate(&RTCHandle, &RTC_DateStructure, RTC_FORMAT_BIN);



		LCD_DisplayString(6,0, (uint8_t *) "MM:DD:YYYY");
	
		LCD_DisplayInt(7,0,RTC_DateStructure.Month);
		LCD_DisplayInt(7,3,RTC_DateStructure.Date);
		LCD_DisplayInt(7,6,RTC_DateStructure.Year);
}
void incrementYear(void){ // increments the year by one until it reaches 100
	
	RTC_DateStructure.Year += 1;

  if (RTC_DateStructure.Year>=99){
		RTC_DateStructure.Year = 0;
	}

}

/* Increment month */
void incrementMonth(void) // increments the month by one until it reaches 13
{
	RTC_DateStructure.Month += 1;

  if (RTC_DateStructure.Month>12){
		RTC_DateStructure.Month = 1;
	}
}

/* Increment day */
void incrementDay(void) // increments the days by one until it reaches 32
{
  RTC_DateStructure.Date+= 1;

  if (RTC_DateStructure.Date>31){
		RTC_DateStructure.Date = 1;
	}
}

/* Increment hour */
void incrementHour(void){ // increments the hour by one until it reaches 24

  RTC_TimeStructure.Hours += 1;

  if (RTC_TimeStructure.Hours>23){
		RTC_TimeStructure.Hours = 0;
	}

}

/* Increment minute */
void incrementMinute(void){ // increments the minute by one until it reaches 60
  RTC_TimeStructure.Minutes += 1;

  if (RTC_TimeStructure.Minutes>59){
		RTC_TimeStructure.Minutes = 0;
	}
}

/* Increment second */
void incrementSecond(void){ // increments the second by one until it reaches 60
   RTC_TimeStructure.Seconds += 1;

  if (RTC_TimeStructure.Seconds>59){
		RTC_TimeStructure.Seconds = 0;
	}
}


void getPastTimes (time_t * time1, time_t * time2) // gets the past times from the EEPROM 
{
  /* Get time data */
  time1->second = I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation);
  time1->minute = I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation+1);
  time1->hour   = I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation+2);

  time2->second = I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation+3);
  time2->minute = I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation+4);
  time2->hour   = I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation+5);
}



void displayPastTimes(void) // displays the past time 
{

  time_t time1;
  time_t time2;
  getPastTimes(&time1, &time2);
		LCD_DisplayString(4, 0, (uint8_t *)"Latest Time");
		LCD_DisplayInt(5,0,time1.second);
		LCD_DisplayString(5, 2, (uint8_t *)":");
		LCD_DisplayInt(5,3,time1.minute);
		LCD_DisplayString(5, 5, (uint8_t *)":");
		LCD_DisplayInt(5,6,time1.hour);
	
		LCD_DisplayString(8, 0, (uint8_t *)"Second Latest Time");
	  LCD_DisplayInt(9,0,time2.second);
		LCD_DisplayString(5, 2, (uint8_t *)":");
		LCD_DisplayInt(9,3,time2.minute);
		LCD_DisplayString(5, 2, (uint8_t *)":");
		LCD_DisplayInt(9,6,time2.hour);
}

/**
  * @brief EXTI line detection callbacks
  * @param GPIO_Pin: Specifies the pins connected EXTI line
  * @retval None
  */


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	
  if(GPIO_Pin == KEY_BUTTON_PIN)  //GPIO_PIN_0
  { // writes the time to the EEPROM 
		/*
	memLocation = most recent hours
	memLocation+1 = most recent minutes
	memLocation+2 = most recent seconds
	memLocation+3 = second most recent hours
	memLocation+4 = second most recent minutes
	memLocation+5 = second most recent seconds
	*/
		UB_Pressed = 1;
		uint8_t readData=0x00;
		uint32_t EE_status;
		
		// moves most recent time recorded to second most recent time
		readData = I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation); // read hours from time 1 
    EE_status=I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation+3, readData); // write hours to time 2 
    readData = I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation+1); // read minutes from time 1
    EE_status=I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation+4, readData); // write minutes to time 2 
    readData = I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation+2); // reads seconds from time 1
    EE_status=I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation+5, readData); // writes seconds to time 2 

    //write current time to most recent time
    EE_status=I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation, RTC_TimeStructure.Hours); // writes hours of current time 
    EE_status=I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation+1, RTC_TimeStructure.Minutes); // writes minutes of current time 
    EE_status=I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation+2, RTC_TimeStructure.Seconds); // writes seconds of current time 


  }
	
	
	if(GPIO_Pin == GPIO_PIN_1) 
  {
		Extern_1 = 1;

	}  //end of PIN_1

	if(GPIO_Pin == GPIO_PIN_2)
  {
		Extern_2 = 1;

		
	} //end of if PIN_2	
	
	
}


void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
 
	RTC_INT_TRIGGERED = 1;
	//RTC_TimeShow();
	BSP_LED_Toggle(LED3);
}




/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  /* Turn LED4 on */
  BSP_LED_On(LED4);
  while(1)
  {
  }
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
static void SystemClock_Config(void){
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
  RCC_OscInitStruct.PLL.PLLQ = 3;
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


/**
  * @brief  Configures EXTI Line0 (connected to PA0 pin) in interrupt mode
  * @param  None
  * @retval None
  */



/**
 * Use this function to configure the GPIO to handle input from
 * external pushbuttons and configure them so that you will handle
 * them through external interrupts.
 */
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
//	__HAL_GPIO_EXTI_CLEAR_FLAG(GPIO_PIN_1);// after moving the chunk of code in the GPIO_EXTI callback from _it.c (before these chunks are in _it.c)
																					//the program "freezed" when start, suspect there is a interupt pending bit there. Clearing it solve the problem.
  /* Enable and set EXTI Line0 Interrupt to the lowest priority */
  HAL_NVIC_SetPriority(EXTI1_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);
}

void ExtBtn2_Config(void){  //**********PD2.***********

	//
	 GPIO_InitTypeDef   GPIO_InitStructure;

  /* Enable GPIOB clock */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  
  /* Configure PA0 pin as input floating */
  GPIO_InitStructure.Mode =  GPIO_MODE_IT_FALLING;
  GPIO_InitStructure.Pull =GPIO_PULLUP;
  GPIO_InitStructure.Pin = GPIO_PIN_2;
	//GPIO_InitStructure.Speed=GPIO_SPEED_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStructure);
	
	
  // Enable and set EXTI Line0 Interrupt to the lowest priority 
  HAL_NVIC_SetPriority(EXTI2_IRQn, 3, 1);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);
}


void RTC_Config(void) {
	RTC_TimeTypeDef RTC_TimeStructure;
	RTC_DateTypeDef RTC_DateStructure;
	
	/****************
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	RTC_AlarmTypeDef RTC_AlarmStructure;
	****************/
	
	//1: Enable the RTC domain access (enable wirte access to the RTC )
			//1.1: Enable the Power Controller (PWR) APB1 interface clock:
        __HAL_RCC_PWR_CLK_ENABLE(); 
			//1.2:  Enable access to RTC domain 
				HAL_PWR_EnableBkUpAccess();
			//1.3: Select the RTC clock source
				__HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_LSI);  //RCC_RTCCLKSOURCE_LSI is defined in hal_rcc.h
	       // according to P9 of AN3371 Application Note, LSI's accuracy is not suitable for RTC application!!!! 
					//can not use LSE!!!---LSE is not available, at leaset not available for stm32f407 board.
				//****"Without parts at X3, C16, C27, and removing SB15 and SB16, the LSE is not going to tick or come ready"*****.
			//1.4: Enable RTC Clock
			__HAL_RCC_RTC_ENABLE();   //enable RTC
			
	
			//1.5  Enable LSI
			__HAL_RCC_LSI_ENABLE();   //need to enable the LSI !!!
																//defined in _rcc.c
			while (__HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY)==RESET) {}    //defind in rcc.c
	
			// for the above steps, please see the CubeHal UM1725, p616, section "Backup Domain Access" 	
				
				
				
	//2.  Configure the RTC Prescaler (Asynchronous and Synchronous) and RTC hour 
        
				RTCHandle.Instance = RTC;
				RTCHandle.Init.HourFormat = RTC_HOURFORMAT_24;
				//RTC time base frequency =LSE/((AsynchPreDiv+1)*(SynchPreDiv+1))=1Hz
				//see the AN3371 Application Note: if LSE=32.768K, PreDiv_A=127, Prediv_S=255
				//    if LSI=32K, PreDiv_A=127, Prediv_S=249
				//also in the note: LSI accuracy is not suitable for calendar application!!!!!! 
				RTCHandle.Init.AsynchPrediv = 127; //if using LSE: Asyn=127, Asyn=255: 
				RTCHandle.Init.SynchPrediv = 249;  //if using LSI(32Khz): Asyn=127, Asyn=249: 
				// but the UM1725 says: to set the Asyn Prescaler a higher value can mimimize power comsumption
				
				RTCHandle.Init.OutPut = RTC_OUTPUT_DISABLE;
				RTCHandle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
				RTCHandle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
				
				//HAL_RTC_Init(); 
				if(HAL_RTC_Init(&RTCHandle) != HAL_OK)
				{
						LCD_DisplayString(1, 0, (uint8_t *)"RTC Init Error!");
				}
	
	//3. init the time and date
				RTC_DateStructure.Year = 23;
				RTC_DateStructure.Month = 3;
				RTC_DateStructure.Date = 6; //if use RTC_FORMAT_BCD, NEED TO SET IT AS 0x18 for the 18th.
				RTC_DateStructure.WeekDay = RTC_WEEKDAY_FRIDAY; //???  if the real weekday is not correct for the given date, still set as 
																												//what is specified here.
				
				if(HAL_RTC_SetDate(&RTCHandle,&RTC_DateStructure,RTC_FORMAT_BIN) != HAL_OK)   //BIN format is better 
															//before, must set in BCD format and read in BIN format!!
				{
					LCD_DisplayString(2, 0, (uint8_t *)"Date Init Error!");
				} 
  
  
				RTC_TimeStructure.Hours = 16;  
				RTC_TimeStructure.Minutes = 11; //if use RTC_FORMAT_BCD, NEED TO SET IT AS 0x19
				RTC_TimeStructure.Seconds = 13;
				RTC_TimeStructure.TimeFormat = RTC_HOURFORMAT12_AM;
				RTC_TimeStructure.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
				RTC_TimeStructure.StoreOperation = RTC_STOREOPERATION_RESET;//?????/
				
				if(HAL_RTC_SetTime(&RTCHandle,&RTC_TimeStructure,RTC_FORMAT_BIN) != HAL_OK)   //BIN format is better
																																					//before, must set in BCD format and read in BIN format!!
				{
					LCD_DisplayString(3, 0, (uint8_t *)"TIME Init Error!");
				}
  
			//Writes a data in a RTC Backup data Register0   --why need this line?
			 //HAL_RTCEx_BKUPWrite(&RTCHandle,RTC_BKP_DR0,0x32F2);   

	/*
			//The RTC Resynchronization mode is write protected, use the
			//__HAL_RTC_WRITEPROTECTION_DISABLE() befor calling this function.
			__HAL_RTC_WRITEPROTECTION_DISABLE(&RTCHandle);
			//wait for RTC APB registers synchronisation
			HAL_RTC_WaitForSynchro(&RTCHandle);
			__HAL_RTC_WRITEPROTECTION_ENABLE(&RTCHandle);				
	 */
				
				
			__HAL_RTC_TAMPER1_DISABLE(&RTCHandle);
			__HAL_RTC_TAMPER2_DISABLE(&RTCHandle);	
				//Optionally, a tamper event can cause a timestamp to be recorded. ---P802 of RM0090
				//Timestamp on tamper event
				//With TAMPTS set to ?1 , any tamper event causes a timestamp to occur. In this case, either
				//the TSF bit or the TSOVF bit are set in RTC_ISR, in the same manner as if a normal
				//timestamp event occurs. The affected tamper flag register (TAMP1F, TAMP2F) is set at the
				//same time that TSF or TSOVF is set. ---P802, about Tamper detection
				//-------that is why need to disable this two tamper interrupts. Before disable these two, when program start, there is always a timestamp interrupt.
				//----also, these two disable function can not be put in the TSConfig().---put there will make  the program freezed when start. the possible reason is
				//-----one the RTC is configured, changing the control register again need to lock and unlock RTC and disable write protection.---See Alarm disable/Enable 
				//---function.
				
			HAL_RTC_WaitForSynchro(&RTCHandle);	
			//To read the calendar through the shadow registers after Calendar initialization,
			//		calendar update or after wake-up from low power modes the software must first clear
			//the RSF flag. The software must then wait until it is set again before reading the
			//calendar, which means that the calendar registers have been correctly copied into the
			//RTC_TR and RTC_DR shadow registers.The HAL_RTC_WaitForSynchro() function
			//implements the above software sequence (RSF clear and RSF check).	
}


void RTC_AlarmAConfig(void)
{
	
	/*
	RTC_AlarmTypeDef RTC_Alarm_Structure;

	RTC_Alarm_Structure.Alarm = RTC_ALARM_A;
  RTC_Alarm_Structure.AlarmMask = RTC_ALARMMASK_ALL;
				// See reference manual. especially 
				//p11-12 of AN3371 Application Note.
				// this mask mean alarm occurs every second.
				//if MaskAll, the other 3 fieds of the AlarmStructure do not need to be initiated
				//the other three fieds are: RTC_AlarmTime(for when to occur), 
				//RTC_AlarmDateWeekDaySel (to use RTC_AlarmDateWeekDaySel_Date or RTC_AlarmDateWeekDaySel_WeekDay
				//RTC_AlarmDateWeekDay (0-31, or RTC_Weekday_Monday, RTC_Weekday_Tuesday...., depends on the value of AlarmDateWeekDaySel)	
	//RTC_Alarm_Structure.AlarmDateWeekDay = RTC_WEEKDAY_MONDAY;
  //RTC_Alarm_Structure.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
  //RTC_Alarm_Structure.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
		   //RTC_ALARMSUBSECONDMASK_ALL --> All Alarm SS fields are masked. 
        //There is no comparison on sub seconds for Alarm 
			
  //RTC_Alarm_Structure.AlarmTime.Hours = 0x02;
  //RTC_Alarm_Structure.AlarmTime.Minutes = 0x20;
  //RTC_Alarm_Structure.AlarmTime.Seconds = 0x30;
  //RTC_Alarm_Structure.AlarmTime.SubSeconds = 0x56;
  
  if(HAL_RTC_SetAlarm_IT(&RTCHandle,&RTC_Alarm_Structure,RTC_FORMAT_BCD) != HAL_OK)
  {
			LCD_DisplayString(4, 0, (uint8_t *)"Alarm setup Error!");
  }
  
	//Enable the RTC Alarm interrupt
	//__HAL_RTC_ALARM_ENABLE_IT(&RTCHandle,RTC_IT_ALRA);   //already in function HAL_RTC_SetAlarm_IT()
	
	//Enable the RTC ALARMA peripheral.
//	__HAL_RTC_ALARMA_ENABLE(&RTCHandle);  //already in function HAL_RTC_SetAlarm_IT()
	
	__HAL_RTC_ALARM_CLEAR_FLAG(&RTCHandle, RTC_FLAG_ALRAF); //need it? !!!!, without it, sometimes(SOMETIMES, when first time to use the alarm interrupt)
																			//the interrupt handler will not work!!! 		
	

	
		//need to set/enable the NVIC for RTC_Alarm_IRQn!!!!
	HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);   
	HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 0x00, 0);  //not important
	
				*/

}


HAL_StatusTypeDef  RTC_AlarmA_IT_Disable(RTC_HandleTypeDef *hrtc) 
{ 
 	// Process Locked  
	__HAL_LOCK(hrtc);
  
  hrtc->State = HAL_RTC_STATE_BUSY;
  
  // Disable the write protection for RTC registers 
  __HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);
  
  // __HAL_RTC_ALARMA_DISABLE(hrtc);
    
   // In case of interrupt mode is used, the interrupt source must disabled 
   __HAL_RTC_ALARM_DISABLE_IT(hrtc, RTC_IT_ALRA);


 // Enable the write protection for RTC registers 
  __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);
  
  hrtc->State = HAL_RTC_STATE_READY; 
  
  // Process Unlocked 
  __HAL_UNLOCK(hrtc);  
}


HAL_StatusTypeDef  RTC_AlarmA_IT_Enable(RTC_HandleTypeDef *hrtc) 
{	
	// Process Locked  
	__HAL_LOCK(hrtc);	
  hrtc->State = HAL_RTC_STATE_BUSY;
  
  // Disable the write protection for RTC registers 
  __HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);
  
  // __HAL_RTC_ALARMA_ENABLE(hrtc);
    
   // In case of interrupt mode is used, the interrupt source must disabled 
   __HAL_RTC_ALARM_ENABLE_IT(hrtc, RTC_IT_ALRA);


 // Enable the write protection for RTC registers 
  __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);
  
  hrtc->State = HAL_RTC_STATE_READY; 
  
  // Process Unlocked 
  __HAL_UNLOCK(hrtc);  

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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
