#include "stm32f0xx_hal.h"
#include "Magus Encoder.h"
#include "main.h"

extern SPI_HandleTypeDef hspi1;

// Variables
int16_t siValue_ENC[7];
uint8_t bSwitch_ENC[7], bSwitch_ENC_Prev[7];
uint8_t seqA[7] = "", seqB[7] = "", ucA_Val_Prev[7] = "", ucB_Val_Prev[7] = "";

void Encoders_Init (void)
{
	// Reset encoder values
	siValue_ENC[1]  = 0;
	siValue_ENC[2]  = 0;
	siValue_ENC[3]  = 0;
	siValue_ENC[4]  = 0;
	siValue_ENC[5]  = 0;
	siValue_ENC[6]  = 0;
	
	// Reset switch values
	bSwitch_ENC[1] = 0;
	bSwitch_ENC[2] = 0;
	bSwitch_ENC[3] = 0;
	bSwitch_ENC[4] = 0;
	bSwitch_ENC[5] = 0;
	bSwitch_ENC[6] = 0;
}
	
void Encoders_Main (void)
{
	scan_Encoders();
	
}

void scan_Encoders (void)
{
	uint8_t ucA_Val, ucB_Val;
	uint8_t x;
	
	// Read all encoders
	for (x=1; x<=6; x++)
	{
		// Read Encoder pins
		switch(x)
		{	
			// Read all encoders
			case 1: ucA_Val = HAL_GPIO_ReadPin(ENC1_A_GPIO_Port, ENC1_A_Pin);
							ucB_Val = HAL_GPIO_ReadPin(ENC1_B_GPIO_Port, ENC1_B_Pin);		break;
		
			case 2: ucA_Val = HAL_GPIO_ReadPin(ENC2_A_GPIO_Port, ENC2_A_Pin);
							ucB_Val = HAL_GPIO_ReadPin(ENC2_B_GPIO_Port, ENC2_B_Pin);		break;
			
			case 3: ucA_Val = HAL_GPIO_ReadPin(ENC3_A_GPIO_Port, ENC3_A_Pin);
							ucB_Val = HAL_GPIO_ReadPin(ENC3_B_GPIO_Port, ENC3_B_Pin);		break;
			
			case 4: ucA_Val = HAL_GPIO_ReadPin(ENC4_A_GPIO_Port, ENC4_A_Pin);
							ucB_Val = HAL_GPIO_ReadPin(ENC4_B_GPIO_Port, ENC4_B_Pin);		break;
			
			case 5: ucA_Val = HAL_GPIO_ReadPin(ENC5_A_GPIO_Port, ENC5_A_Pin);
							ucB_Val = HAL_GPIO_ReadPin(ENC5_B_GPIO_Port, ENC5_B_Pin);		break;
		
			case 6: ucA_Val = HAL_GPIO_ReadPin(ENC6_A_GPIO_Port, ENC6_A_Pin);
							ucB_Val = HAL_GPIO_ReadPin(ENC6_B_GPIO_Port, ENC6_B_Pin);		break;
		}
		
		// Check if a change has occurred
		if(ucA_Val != ucA_Val_Prev[x] || ucB_Val != ucB_Val_Prev[x])
		{
			// Record the A and B sequences
			seqA[x] <<= 1;	
			seqA[x] |=  ucA_Val; 
			
			seqB[x] <<= 1;	
			seqB[x] |=  ucB_Val; 
			
			// Mask the MSB four bits
			seqA[x] &= 0x0F;
			seqB[x]	&= 0x0F;
			
			// Check for a turn
			if (seqA[x] == 0x09 && seqB[x] == 0x03) {siValue_ENC[x]--;}
			if (seqA[x] == 0x03 && seqB[x] == 0x09) {siValue_ENC[x]++;}
		}
		
		// Copy last read value
		ucA_Val_Prev[x] = ucA_Val;
		ucB_Val_Prev[x] = ucB_Val;
	}
}

void send_SPI(void)
{
	uint8_t Data[13] = "";
	
	// Build Tx string
	Data[0]   = bSwitch_ENC[1]<<0;
	Data[0]  |= bSwitch_ENC[2]<<1;
	Data[0]  |= bSwitch_ENC[3]<<2;
	Data[0]  |= bSwitch_ENC[4]<<3;
	Data[0]  |= bSwitch_ENC[5]<<4;
	Data[0]  |= bSwitch_ENC[6]<<5;
	Data[1]   = (siValue_ENC[1]&0xFF00)>>8;
	Data[2]  |= (siValue_ENC[1]&0x00FF);
	Data[3]   = (siValue_ENC[2]&0xFF00)>>8;
	Data[4]  |= (siValue_ENC[2]&0x00FF);
	Data[5]   = (siValue_ENC[3]&0xFF00)>>8;
	Data[6]  |= (siValue_ENC[3]&0x00FF);
	Data[7]   = (siValue_ENC[4]&0xFF00)>>8;
	Data[8]  |= (siValue_ENC[4]&0x00FF);
	Data[9]   = (siValue_ENC[5]&0xFF00)>>8;
	Data[10] |= (siValue_ENC[5]&0x00FF);
	Data[11]  = (siValue_ENC[6]&0xFF00)>>8;
	Data[12] |= (siValue_ENC[6]&0x00FF);
	
	// Send data
	HAL_SPI_Transmit(&hspi1, (uint8_t*)&Data, 13, 100);
}

