/****** 

1. For GPIO pins, Both OD mode and PP mode can drive the motor! However, some pins cannot output  high in OD mode!!! 
   
2. The signals do not need to be inverted before being fed in H-bridge.   
*/


#include "main.h"


#define COLUMN(x) ((x) * (((sFONT *)BSP_LCD_GetFont())->Width))    //see font.h, for defining LINE(X)
GPIO_InitTypeDef GPIO_InitStructure;

TIM_HandleTypeDef    Tim3Handle;
TIM_OC_InitTypeDef Tim3_OCInitStructure;
uint16_t Tim3PrescalerValue = 7499;

int direction = 1; //counterclockwise = 0
int mode = 1;//fullstep = 0

int period = 6000; 



void LCD_DisplayString(uint16_t LineNumber, uint16_t ColumnNumber, uint8_t *ptr);
void LCD_DisplayInt(uint16_t LineNumber, uint16_t ColumnNumber, int Number);
void LCD_DisplayFloat(uint16_t LineNumber, uint16_t ColumnNumber, float Number, int DigitAfterDecimalPoint);

void BoardPins_Config(void);
void ButtonConfig(void); 
void Tim3_Config(void);
void Update(void);

static void SystemClock_Config(void);
static void Error_Handler(void);


int next; 
int State[8][4] = {
	
	//this will specify what voltages to set each pin to--view Stepper Motor slide 
//{a1, a2, b1, b2}
	{0,0,1,0},
	{0,1,1,0},
	{0,1,0,0},
	{0,1,0,1},
	{0,0,0,1},
	{1,0,0,1},
	{1,0,0,0},
	{1,0,1,0}
};

	
/*
main {
int i = 0

if i < 7
	HAL_GPIO_WRITEPIN(GPIOD, GPIO_PIN_12, State[i][0)
	
	State[i][0]
	i++
else 
	State[i]
	i = 0

*/

int main(void){
	
		/* STM32F4xx HAL library initialization:
       - Configure the Flash prefetch, instruction and Data caches
       - Configure the Systick to generate an interrupt each 1 msec
       - Set NVIC Group Priority to 4
       - Global MSP (MCU Support Package) initialization
     */
		HAL_Init();
		BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_EXTI);
	
		 /* Configure the system clock to 72 MHz */
		SystemClock_Config();
		
		HAL_InitTick(0x0000); // set systick's priority to the highest.
	
	
		BSP_LCD_Init();
		//BSP_LCD_LayerDefaultInit(uint16_t LayerIndex, uint32_t FB_Address);
		BSP_LCD_LayerDefaultInit(0, LCD_FRAME_BUFFER);   //LCD_FRAME_BUFFER, defined as 0xD0000000 in _discovery_lcd.h
															// the LayerIndex may be 0 and 1. if is 2, then the LCD is dark.
		//BSP_LCD_SelectLayer(uint32_t LayerIndex);
		BSP_LCD_SelectLayer(0);
		//BSP_LCD_SetLayerVisible(0, ENABLE); 
		
		BSP_LCD_Clear(LCD_COLOR_WHITE);  //need this line, otherwise, the screen is dark	
		BSP_LCD_DisplayOn();
	 
		BSP_LCD_SetFont(&Font20);  //the default font,  LCD_DEFAULT_FONT, which is defined in _lcd.h, is Font24
	
		LCD_DisplayString(2, 3, (uint8_t *)"Lab");
	
		LCD_DisplayInt(2, 8, 5);
		//CODE STARTS HERE
			// Enable Clock Port D
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	
		Tim3_Config();
		BoardPins_Config();
		ButtonConfig();
		
		
		/*for (int i = 0; i <= 7; i++) {
		
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, State[i][0]);
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, State[i][1]);
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, State[i][2]);
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, State[i][3]);
			HAL_Delay(1000);
		}*/
		
		//HAL_TIM_Base_Start_IT(&Tim3Handle);//start TIM3 HERE
	
		
		while(1) {	
			LCD_DisplayInt(9, 2, period);
		
		} // end of while loop
	
}  //end of main


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

//start board config
void BoardPins_Config(void) {



	// PC4, PC5
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Pin = GPIO_PIN_4 | GPIO_PIN_5;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	// PB2, PB12
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Pin = GPIO_PIN_2 | GPIO_PIN_12;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void ButtonConfig(void) {
	//Enable Clocks
	//__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	
	//Button One = PIN PC1
	GPIO_InitStructure.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	GPIO_InitStructure.Pin = GPIO_PIN_1;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

	__HAL_GPIO_EXTI_CLEAR_FLAG(GPIO_PIN_1);

	HAL_NVIC_SetPriority(EXTI1_IRQn, 3, 0);
	HAL_NVIC_EnableIRQ(EXTI1_IRQn);
	
	
	//Button Two = PIN PD2
	GPIO_InitStructure.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	GPIO_InitStructure.Pin = GPIO_PIN_2;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStructure);

	__HAL_GPIO_EXTI_CLEAR_FLAG(GPIO_PIN_2);

	HAL_NVIC_SetPriority(EXTI2_IRQn, 3, 0);
	HAL_NVIC_EnableIRQ(EXTI2_IRQn);
	
	//Button Three = PIN PC3
	GPIO_InitStructure.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	GPIO_InitStructure.Pin = GPIO_PIN_3;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

	__HAL_GPIO_EXTI_CLEAR_FLAG(GPIO_PIN_3);

	HAL_NVIC_SetPriority(EXTI3_IRQn, 3, 0);
	HAL_NVIC_EnableIRQ(EXTI3_IRQn);
}

//start timer config
void Tim3_Config(void) {
	Tim3Handle.Init.Period = period - 1; // temporary, changed as needed
	Tim3Handle.Init.Prescaler = ((SystemCoreClock / 2) / 4800) - 1; // fixed
	Tim3Handle.Instance = TIM3;
	Tim3Handle.Init.Prescaler = Tim3PrescalerValue;
	Tim3Handle.Init.ClockDivision = 0;
	Tim3Handle.Init.CounterMode = TIM_COUNTERMODE_UP;
	HAL_TIM_Base_Init(&Tim3Handle);
	if(HAL_TIM_Base_Start_IT(&Tim3Handle) != HAL_OK)   //the TIM_XXX_Start_IT function enable IT, and also enable Timer
																											//so do not need HAL_TIM_BASE_Start() any more.
  {
    Error_Handler();
  }
	

}

void Stepper(uint16_t step) {
	
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, State[step][0]);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, State[step][1]);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, State[step][2]);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, State[step][3]);

}

void Update(void) {
	

	

}




void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	
		if(GPIO_Pin == KEY_BUTTON_PIN)  //GPIO_PIN_0
		{
			direction = !direction;
			//Update();
			LCD_DisplayString(9, 1, (uint8_t *) "User Button");

		}
		
		
		if(GPIO_Pin == GPIO_PIN_1)
		{
		
			period-=1000;
			Tim3_Config();
			//Update();
				
			
		}  //end of PIN_1

		if(GPIO_Pin == GPIO_PIN_2) {
			mode = !mode;
			if (mode == 0){
				period = period/2;
			
			}
			else if (mode == 1) {
				period = period *2;
			}
			Tim3_Config();
				
		} //end of if PIN_2	
		
		if(GPIO_Pin == GPIO_PIN_3)
		{
			period+=1000;
			Tim3_Config();
			//Update();
					
				
		} //end of if PIN_3
}



int i = 0; 
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef * htim) //see  stm32fxx_hal_tim.c for different callback function names. 
{	
	BSP_LCD_ClearStringLine(7);
	if (direction == 1 && mode == 1) {
		if (i%2!=0){
			i++;
		}
		i=i%8;
		Stepper((i));
		i = i+2;
		LCD_DisplayString(7, 5, (uint8_t *) "CW Full");
	}
	else if (direction == 1 && mode == 0) {
		i=(i)%8;
		Stepper((i));
		i = i+1;
		LCD_DisplayString(7, 5, (uint8_t *) "CW Half");

	}
	else if (direction == 0 && mode == 1) {
		if (i%2 !=0){
			i--;
		}
		i = (i+8)%8;
		Stepper(i);
		i = i - 2;
		LCD_DisplayString(7, 5, (uint8_t *) "CCW Full");

		
	}
	else if (direction == 0 && mode == 0) {
		i = (8+i)%8;
		Stepper(i);
		i = i-1;
		LCD_DisplayString(7, 5, (uint8_t *) "CCW Half");

	
	}
	//for timer4 
				//clear the timer counter!  in stm32f4xx_hal_tim.c, the counter is not cleared after  OC interrupt
				//__HAL_TIM_SET_COUNTER(htim, 0x0000);   //this maro is defined in stm32f4xx_hal_tim.
		
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



