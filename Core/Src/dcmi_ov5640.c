/***

����˵����
	*
	* 1.����Ĭ������ OV5640  Ϊ 4:3(1280*960) 43֡ �����ã�JPGģʽ2��3�����֡�ʻ���룩
	*	2.������DMA��ʹ�����жϣ���ֲ��ʱ����Ҫ��ֲ��Ӧ���ж�
	*
	*********************************************************************************************
***/

#include "dcmi_ov5640.h"
#include "dcmi_ov5640_cfg.h"
#include <stdio.h>  
uint16_t	Device_ID;	
DCMI_HandleTypeDef   hdcmi;            // DCMI���?
DMA_HandleTypeDef    hdma_dcmi;  // DMA??��?? (name matches stm32h7xx_it.c reference)

volatile uint8_t OV5640_FrameState = 0;  // DCMI״̬��־��������֡�������ʱ����? HAL_DCMI_FrameEventCallback() �жϻص������� 1     
volatile uint8_t OV5640_FPS ;          // ֡��

/*************************************************
*	�� �� ��:	HAL_DCMI_MspInit
*
*	��ڲ���?:	hdcmi - DCMI_HandleTypeDef����ı���������ʾ����� DCMI ���?
*
*	��������:	��ʼ�� DCMI ����
*
************************************************************************/
void HAL_DCMI_MspInit(DCMI_HandleTypeDef* hdcmi)
{
   GPIO_InitTypeDef GPIO_InitStruct = {0};

   if(hdcmi->Instance==DCMI)
   {
		__HAL_RCC_DCMI_CLK_ENABLE();		// ʹ�� DCMI ����ʱ��

		__HAL_RCC_GPIOE_CLK_ENABLE();// ʹ����Ӧ��GPIOʱ��
		__HAL_RCC_GPIOD_CLK_ENABLE();
		__HAL_RCC_GPIOB_CLK_ENABLE();
		__HAL_RCC_GPIOC_CLK_ENABLE();
		__HAL_RCC_GPIOA_CLK_ENABLE();
		
	

/****************************************************************************  
   ��������                       ʱ�Ӻ�ͬ������
    PC6     ------> DCMI_D0        PB7     ------> DCMI_VSYNC
    PC7     ------> DCMI_D1	     	 PA4     ------> DCMI_HSYNC
    PE0     ------> DCMI_D2        PA6  	 ------> DCMI_PIXCLK
    PE1     ------> DCMI_D3	
    PE4     ------> DCMI_D4	    SCCB �������ţ���ʼ���� sccb.c �ļ�
    PD3     ------> DCMI_D5		  PB6  ------> SCCB_SCL
    PE5     ------> DCMI_D6	    PB9  ------> SCCB_SDA 
    PE6     ------> DCMI_D7

   �����������?
   PD10   ------> PWDN
******************************************************************************/

		GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_0|GPIO_PIN_5
								  |GPIO_PIN_6;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF13_DCMI;
		HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = GPIO_PIN_3;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF13_DCMI;
		HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = GPIO_PIN_7;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF13_DCMI;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_6;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF13_DCMI;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_4;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF13_DCMI;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // ��ʼ�� PWDN ����  
		//OV5640_PWDN_ON;	// �ߵ�ƽ���������ģʽ������ͷֹͣ��������ʱ���Ľ������

		GPIO_InitStruct.Pin 		= GPIO_PIN_10;					// PWDN ����
		GPIO_InitStruct.Mode 	= GPIO_MODE_OUTPUT_PP;		// �������ģ�?
		GPIO_InitStruct.Pull 	= GPIO_PULLUP;						// ����
		GPIO_InitStruct.Speed 	= GPIO_SPEED_FREQ_LOW;	// �ٶȵȼ���
		HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);	   			// ��ʼ��  
		
		GPIO_InitStruct.Pin = GPIO_PIN_13;                 //DCMI RST
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull = GPIO_PULLUP;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
		
		DCMI_RST_L;
		OV5640_PWDN_ON;
	}
}

/***************************************************************************************************************************************
*	�� �� ��: MX_DCMI_Init
*
*	��������: ����DCMI��ز���?
*
*	˵    ��: 8λ����ģʽ��ȫ���ݡ�ȫ֡��׽�������ж�		 			          
*
*****************************************************************************************************************************************/
void MX_DCMI_Init(void)
{
   hdcmi.Instance                = DCMI;
   hdcmi.Init.SynchroMode        = DCMI_SYNCHRO_HARDWARE;      // Ӳ��ͬ��ģʽ����ʹ���ⲿ��VS��HS�źŽ���ͬ��
   hdcmi.Init.PCKPolarity        = DCMI_PCKPOLARITY_RISING;    // ����ʱ����������Ч
   hdcmi.Init.VSPolarity         = DCMI_VSPOLARITY_LOW;        // VS�͵�ƽ��Ч
   hdcmi.Init.HSPolarity         = DCMI_HSPOLARITY_LOW;        // HS�͵�ƽ��Ч
   hdcmi.Init.CaptureRate        = DCMI_CR_ALL_FRAME;          // ����ȼ�������ÿһ֡�����в���?
   hdcmi.Init.ExtendedDataMode   = DCMI_EXTEND_DATA_8B;        // 8λ����ģʽ
   hdcmi.Init.JPEGMode           = DCMI_JPEG_DISABLE;         	// ��ʹ��DCMI��JPEGģʽ
   hdcmi.Init.ByteSelectMode     = DCMI_BSM_ALL;               // DCMI�ӿڲ�׽��������  
   hdcmi.Init.ByteSelectStart    = DCMI_OEBS_ODD;              // ѡ��ʼ�ֽڣ��� ֡/�� �ĵ�һ�����ݿ�ʼ����
   hdcmi.Init.LineSelectMode     = DCMI_LSM_ALL;               // ����������
   hdcmi.Init.LineSelectStart    = DCMI_OELS_ODD;              // ѡ��ʼ��,��֡��ʼ�󲶻��һ��?
   HAL_DCMI_Init(&hdcmi) ;

   HAL_NVIC_SetPriority(DCMI_IRQn, 0 ,5);    // �����ж����ȼ�
   HAL_NVIC_EnableIRQ(DCMI_IRQn); 		      // ����DCMI�ж�
}

/***************************************************************************************************************************************
*	�� �� ��: OV5640_DMA_Init
*
*	��������: ���� DMA ��ز���?
*
*	˵    ��: ʹ�õ���DMA2�����赽�洢��ģʽ������λ��32bit���������ж� 			          
*
*****************************************************************************************************************************************/
void OV5640_DMA_Init(void)
{
   __HAL_RCC_DMA2_CLK_ENABLE();   // ʹ��DMA2ʱ��

   hdma_dcmi.Instance                     = DMA2_Stream7;               // DMA2������7      
   hdma_dcmi.Init.Request                 = DMA_REQUEST_DCMI;           // DMA��������DCMI
   hdma_dcmi.Init.Direction               = DMA_PERIPH_TO_MEMORY;       // ���赽�洢��ģʽ
   hdma_dcmi.Init.PeriphInc               = DMA_PINC_DISABLE;           // �����ַ��ֹ����?
   hdma_dcmi.Init.MemInc                  = DMA_MINC_ENABLE;			      // �洢����ַ����
   hdma_dcmi.Init.PeriphDataAlignment     = DMA_PDATAALIGN_WORD;        // DCMI����λ����32λ  
   hdma_dcmi.Init.MemDataAlignment        = DMA_MDATAALIGN_WORD;        // �洢������λ����32λ
   hdma_dcmi.Init.Mode                    = DMA_CIRCULAR;               // ѭ��ģʽ					
   hdma_dcmi.Init.Priority                = DMA_PRIORITY_LOW;           // ���ȼ���
   hdma_dcmi.Init.FIFOMode                = DMA_FIFOMODE_ENABLE;        // ʹ��fifo
   hdma_dcmi.Init.FIFOThreshold           = DMA_FIFO_THRESHOLD_FULL;    // ȫfifoģʽ��4*32bit��С
   hdma_dcmi.Init.MemBurst                = DMA_MBURST_SINGLE;          // ���δ���
   hdma_dcmi.Init.PeriphBurst             = DMA_PBURST_SINGLE;          // ���δ���

   HAL_DMA_Init(&hdma_dcmi);                        // ����DMA
   __HAL_LINKDMA(&hdcmi, DMA_Handle, hdma_dcmi);    // ����DCMI���?
	
   HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 0, 0);         // �����ж����ȼ�
   HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);                 // ʹ���ж�
}



/***************************************************************************************************************************************
*	�� �� ��: DCMI_OV5640_Init
*
*	��������: ��ʼSCCB��DCMI��DMA�Լ�����OV5640
*
*****************************************************************************************************************************************/
int8_t DCMI_OV5640_Init(void)
{
	
	
  OV5640_DMA_Init();                     // ��ʼ��DMA���� 
	MX_DCMI_Init();                        // ��ʼ��DCMI�������� 
	
	OV5640_PWDN_OFF;  // PWDN ��������͵�ƽ������������ģ�?
  HAL_Delay(140);    
	DCMI_RST_H
	
	
// ��λ����?��Ҫ>=20ms�ſ�ִ��SCCB����
	HAL_Delay(40);    
	SCCB_GPIO_Config();		               // SCCB���ų�ʼ��
	HAL_Delay(20);    
	SCCB_WriteReg_16Bit(0x3103, 0x11);	// �����ֲ�Ľ���?��λ֮ǰ��ֱ�ӽ�ʱ���������ŵ�ʱ����Ϊ��ʱ��
	SCCB_WriteReg_16Bit(0x3008, 0x82);	// ִ��һ������λ
	HAL_Delay(15);  //��ʱ>5ms
	Device_ID =  OV5640_ReadID();		      // ��ȡ����ID

	if( Device_ID == 0x5640 )		// ����ƥ��
	{
		printf ("OV5640 OK,ID:0x%X\r\n",Device_ID);		      // ƥ��ͨ��

		OV5640_Config();													// ���ø������?
		OV5640_Set_Framesize(OV5640_Width,OV5640_Height);		//	����OV5640�����ͼ���С
		OV5640_DCMI_Crop( Display_Width, Display_Height, OV5640_Width, OV5640_Height );	// �����ͼ��ü�����Ӧ��Ļ�Ĵ�С��JPGģʽ����Ҫ�ü�
				
		return OV5640_Success;	 // ���سɹ���־		
	}
	else
	{
		printf ("OV5640 ERROR!!!!!  ID:%X\r\n",Device_ID);	   // ��ȡID����
		return  OV5640_Error;	 // ���ش�����?
	}	
}

/***************************************************************************************************************************************
*	�� �� ��: OV5640_DMA_Transmit_Continuous
*
*	��ڲ���?:  DMA_Buffer - DMA��Ҫ����ĵ�ַ�������ڴ�?����ͷ���ݵĴ洢����ַ
*            DMA_BufferSize - ��������ݴ�С��?32λ��
*
*	��������: ����DMA���䣬����ģʽ
*
*	˵    ��: 1. ��������ģʽ֮�󣬻�һֱ���д��䣬���ǹ������ͣ�?DCMI
*            2. OV5640ʹ��RGB565ģʽʱ��1�����ص���Ҫ2���ֽ����洢
*				 3. ��ΪDMA���ô�������Ϊ32λ�������� DMA_BufferSize ʱ����Ҫ����4�����磺
*               Ҫ��ȡ 240*240�ֱ��� ��ͼ����Ҫ���� 240*240*2 = 115200 �ֽڵ����ݣ�
*               �� DMA_BufferSize = 115200 / 4 = 28800 ��
***********************************************************/
void OV5640_DMA_Transmit_Continuous(uint32_t DMA_Buffer,uint32_t DMA_BufferSize)
{
   hdma_dcmi.Init.Mode  = DMA_CIRCULAR;  // ѭ��ģʽ					

   HAL_DMA_Init(&hdma_dcmi);    // ����DMA

  // ʹ��DCMI�ɼ�����,�����ɼ�ģʽ
   HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_CONTINUOUS, (uint32_t)DMA_Buffer,DMA_BufferSize);
}

/***************************************************************************************************************************************
*	�� �� ��: OV5640_DMA_Transmit_Snapshot
*
*	��ڲ���?:  DMA_Buffer - DMA��Ҫ����ĵ�ַ�������ڴ�?����ͷ���ݵĴ洢����ַ
*            DMA_BufferSize - ��������ݴ�С��?32λ��
*
*	��������: ����DMA���䣬����ģʽ������һ֡ͼ���ͣ�?
*
*	˵    ��: 1. ����ģʽ��ֻ����һ֡������
*            2. OV5640ʹ��RGB565ģʽʱ��1�����ص���Ҫ2���ֽ����洢
*				 3. ��ΪDMA���ô�������Ϊ32λ�������� DMA_BufferSize ʱ����Ҫ����4�����磺
*               Ҫ��ȡ 240*240�ֱ��� ��ͼ����Ҫ���� 240*240*2 = 115200 �ֽڵ����ݣ�
*               �� DMA_BufferSize = 115200 / 4 = 28800 ��
*            4. ʹ�ø�ģʽ��������?��DCMI�ᱻ�����ٴ����ô���֮ǰ����Ҫ���� OV5640_DCMI_Resume() �ָ�DCMI
*
*****************************************************************************************************************************************/
void OV5640_DMA_Transmit_Snapshot(uint32_t DMA_Buffer,uint32_t DMA_BufferSize)
{
   hdma_dcmi.Init.Mode  = DMA_NORMAL;  // ����ģʽ					

   HAL_DMA_Init(&hdma_dcmi);    // ����DMA

   HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_SNAPSHOT, (uint32_t)DMA_Buffer,DMA_BufferSize);
}

/***************************************************************************************************************************************
*	�� �� ��: OV5640_DCMI_Suspend
*
*	��������: ����DCMI��ֹͣ��������
*
*	˵    ��: 1. ��������ģʽ֮���ٵ��øú�������ֹͣ����DCMI������
*            2. ���Ե��� OV5640_DCMI_Resume() �ָ�DCMI
*				 3. ��Ҫע��ģ�����?DCMI�ڼ䣬DMA��û��ֹͣ������
*****************************************************************************************************************************************/
void OV5640_DCMI_Suspend(void) 
{
   HAL_DCMI_Suspend(&hdcmi);    // ����DCMI
}

/***************************************************************************************************************************************
*	�� �� ��: OV5640_DCMI_Resume
*
*	��������: �ָ�DCMI����ʼ��������
*
*	˵    ��: 1. ��DCMI������ʱ�����Ե��øú����ָ�
*            2. ʹ�� OV5640_DMA_Transmit_Snapshot() ����ģʽ����������?��DCMIҲ�ᱻ�����ٴ����ô���֮ǰ��
*				    ��Ҫ���ñ������ָ�DCMI����
*
*****************************************************************************************************************************************/
void  OV5640_DCMI_Resume(void) 
{
   (&hdcmi)->State = HAL_DCMI_STATE_BUSY;       // ���?DCMI��־
   (&hdcmi)->Instance->CR |= DCMI_CR_CAPTURE;   // ����DCMI����
}

/***************************************************************************************************************************************
*	�� �� ��: OV5640_DCMI_Stop
*
*	��������: ��ֹDCMI��DMA����ֹͣDCMI���񣬽�ֹDCMI����
*
*****************************************************************************************************************************************/
void  OV5640_DCMI_Stop(void) 
{
   HAL_DCMI_Stop(&hdcmi);
}


/***************************************************************************************************************************************
*	�� �� ��: OV5640_DCMI_Crop
*
*	��ڲ���?:  Displey_XSize ��Displey_YSize - ��ʾ���ĳ���
*				  Sensor_XSize��Sensor_YSize - ����ͷ���������ͼ��ĳ���
*
*	��������: ʹ��DCMI�Ĳü����ܣ��������������ͼ��ü�����Ӧ��Ļ�Ĵ�С
*
*	˵    ��: 1. ��Ϊ����ͷ����Ļ��������һ��ƥ����ʾ����������Ҫ�ü�
*				 2. ����ͷ�������������� OV5640_Config()���ò������������ջ����С��? OV5640_Set_Framesize()����
*            3. DCMI��ˮƽ��Ч����Ҳ����Ҫ�ܱ�4������
*				 4. ����������?ƽ�ʹ�ֱƫ�ƣ������û�����вü�?
*****************************************************************************************************************************************/
int8_t OV5640_DCMI_Crop(uint16_t Displey_XSize,uint16_t Displey_YSize,uint16_t Sensor_XSize,uint16_t Sensor_YSize )
{
	uint16_t DCMI_X_Offset,DCMI_Y_Offset;	// ˮƽ�ʹ�ֱƫ�ƣ���ֱ��������������ˮƽ������������ʱ������PCLK��������
	uint16_t DCMI_CAPCNT;		// ˮƽ��Ч���أ�������������ʱ������PCLK��������
	uint16_t DCMI_VLINE;			// ��ֱ��Ч����

	if( (Displey_XSize>=Sensor_XSize)|| (Displey_YSize>=Sensor_YSize) )
	{
//		printf("ʵ����ʾ�ĳߴ���ڻ��������ͷ����ĳߴ�?�˳�DCMI�ü�\r\n");
		return OV5640_Error;  //���ʵ����ʾ�ĳߴ���ڻ��������ͷ����ĳߴ磬���˳���ǰ�����������вü�
	}
// ������ΪRGB565��ʽʱ��ˮƽƫ�ƣ�������������������ɫ�ʲ���ȷ��
// ��Ϊһ����Ч������2���ֽڣ���Ҫ2��PCLK���ڣ����Ա��������λ��ʼ����Ȼ���ݻ���ң�
// ��Ҫע����ǣ��Ĵ���ֵ�Ǵ�?0��ʼ�����?	��
	DCMI_X_Offset = Sensor_XSize - Displey_XSize; // ʵ�ʼ�������?��Sensor_XSize - LCD_XSize��/2*2

// ���㴹ֱƫ�ƣ������û�����вü�����ֵ��������������?	
	DCMI_Y_Offset = (Sensor_YSize - Displey_YSize)/2-1; // �Ĵ���ֵ�Ǵ�0��ʼ����ģ������?-1

// ��Ϊһ����Ч������2���ֽڣ���Ҫ2��PCLK���ڣ�����Ҫ��2
// ���յõ��ļĴ���ֵ������Ҫ�ܱ�4������
	DCMI_CAPCNT = Displey_XSize*2-1;	// �Ĵ���ֵ�Ǵ�0��ʼ����ģ������?-1
	
	DCMI_VLINE = Displey_YSize-1;		// ��ֱ��Ч����
	
//	printf("%d  %d  %d  %d\r\n",DCMI_X_Offset,DCMI_Y_Offset,DCMI_CAPCNT,DCMI_VLINE);
	
	HAL_DCMI_ConfigCrop (&hdcmi,DCMI_X_Offset,DCMI_Y_Offset,DCMI_CAPCNT,DCMI_VLINE);// ���òü�����
	HAL_DCMI_EnableCrop(&hdcmi);		// ʹ�ܲü�
	
	return OV5640_Success;
}

/***************************************************************************************************************************************
*	�� �� ��: OV5640_Reset
*
*	��������: ִ��������λ
*
*	˵    ��: �ڼ��ж����ʱ����?          
*
*****************************************************************************************************************************************/
void OV5640_Reset(void)
{
	HAL_Delay(20);  // �ȴ�ģ���ϵ��ȶ�������5ms��Ȼ������PWDN  	
	
	OV5640_PWDN_OFF;  // PWDN ��������͵�ƽ������������ģʽ������ͷ������������ʱ����ͷģ��İ�ɫLED�����?
  HAL_Delay(20);    
	DCMI_RST_H
// ����OV5640���ϵ�ʱ��PWDN����֮��Ҫ�ȴ�1ms��ȥ����RESET���ܶ���OV5640ģ�����Ӳ��?RC��λ��Ҳ����IO���߸�λ, ����ʱ������6~10ms��
// ��˼�����ʱ���ȴ�Ӳ����λ��ɲ��ȶ�����
	HAL_Delay(10);    
	
// ��λ����?��Ҫ>=20ms����ִ��SCCB����
	HAL_Delay(20);    
	
	SCCB_WriteReg_16Bit(0x3103, 0x11);	// �����ֲ�Ľ���?��λ֮ǰ��ֱ�ӽ�ʱ���������ŵ�ʱ����Ϊ��ʱ��
	SCCB_WriteReg_16Bit(0x3008, 0x82);	// ִ��һ������λ
	HAL_Delay(5);  //��ʱ5ms
	
}

/***************************************************************************************************************************************
*	�� �� ��: OV5640_ReadID
*
*	��������: ��ȡ OV5640 ������ID
*
*****************************************************************************************************************************************/
uint16_t OV5640_ReadID(void)
{
   uint8_t PID_H,PID_L;     // ID����
	
   PID_H = SCCB_ReadReg_16Bit(OV5640_ChipID_H); // ��ȡID���ֽ�
   PID_L = SCCB_ReadReg_16Bit(OV5640_ChipID_L); // ��ȡID���ֽ�
	
	return(PID_H<<8)|PID_L; // ��������������ID
}

/***************************************************************************************************************************************
*	�� �� ��: OV5640_Config
*
*	��������: ���� OV5640 �����Ĵ�������
*
*	˵    ��: ���������� dcmi_ov5640_cfg.h
*            
*****************************************************************************************************************************************/

void OV5640_Config(void)
{
	uint32_t i;	// ��������
	
	uint8_t	read_reg; // ��ȡ���ã����ڵ���

	for(i=0; i<(sizeof(OV5640_INIT_Config)/4); i++)
	{
		SCCB_WriteReg_16Bit(OV5640_INIT_Config[i][0], OV5640_INIT_Config[i][1]); // д������
		
		read_reg = SCCB_ReadReg_16Bit(OV5640_INIT_Config[i][0]);	// ��ȡ���ã����ڵ���

		if(OV5640_INIT_Config[i][1] != read_reg )	// ���ò��ɹ�
		{
			printf("����λ�ã�%d\r\n",i);	// ��ӡ����λ��
			printf("0x%x-0x%x-0x%x\r\n",OV5640_INIT_Config[i][0],OV5640_INIT_Config[i][1],read_reg);
		}
	}
	//OV5640_Set_Vertical_Flip(1);   //��ֱ��ת
	//OV5640_Set_Horizontal_Mirror(1);  //ˮƽ��ת
}

/***************************************************************************************************************************************
*	�� �� ��: OV5640_Set_Pixformat
*
*	��ڲ���?:  pixformat - ���ظ�ʽ����ѡ�� Pixformat_RGB565��Pixformat_GRAY��Pixformat_JPEG
*
*	��������: ������������ظ��?
*
*****************************************************************************************************************************************/

void OV5640_Set_Pixformat(uint8_t pixformat)
{
   uint8_t OV5640_Reg;  // �Ĵ�����ֵ

	if( pixformat == Pixformat_JPEG )
	{
		SCCB_WriteReg_16Bit(OV5640_FORMAT_CONTROL, 		0x30);	//	�������ݽӿ�����ĸ��?	
		SCCB_WriteReg_16Bit(OV5640_FORMAT_CONTROL_MUX, 	0x00);	// ����ISP�ĸ�ʽ	
 
		SCCB_WriteReg_16Bit(OV5640_JPEG_MODE_SELECT, 0x02);	 	// JPEG ģʽ2

		SCCB_WriteReg_16Bit(OV5640_JPEG_VFIFO_CTRL00, 0xA0); 		// JPEG �̶�����
		
		SCCB_WriteReg_16Bit(OV5640_JPEG_VFIFO_HSIZE_H, OV5640_Width>>8);			// JPEG����?ƽ�ߴ�,���ֽ�
		SCCB_WriteReg_16Bit(OV5640_JPEG_VFIFO_HSIZE_L, (uint8_t)OV5640_Width);	// JPEG����?ƽ�ߴ�,���ֽ�
		SCCB_WriteReg_16Bit(OV5640_JPEG_VFIFO_VSIZE_H, OV5640_Height>>8);			// JPEG�����ֱ�ߴ�?,���ֽ�
		SCCB_WriteReg_16Bit(OV5640_JPEG_VFIFO_VSIZE_L, (uint8_t)OV5640_Height);	// JPEG�����ֱ�ߴ�?,���ֽ�	
		
	}
	else if( pixformat == Pixformat_GRAY )
	{
		SCCB_WriteReg_16Bit(OV5640_FORMAT_CONTROL, 		0x10);	//	�������ݽӿ�����ĸ��?
		SCCB_WriteReg_16Bit(OV5640_FORMAT_CONTROL_MUX, 	0x00);	// ����ISP�ĸ�ʽ		
	}
	else	// RGB565
	{
		SCCB_WriteReg_16Bit(OV5640_FORMAT_CONTROL, 		0x6F);	// �˴�����ΪRGB565��ʽ������Ϊ G[2:0]B[4:0], R[4:0]G[5:3]	
		SCCB_WriteReg_16Bit(OV5640_FORMAT_CONTROL_MUX, 	0x01);	// ����ISP�ĸ�ʽ	
	}
	
   OV5640_Reg = SCCB_ReadReg_16Bit(0x3821);   // ��ȡ�Ĵ���ֵ��Bit[5]�����Ƿ�ʹ��JPEGģʽ
	SCCB_WriteReg_16Bit(0x3821,(OV5640_Reg & 0xDF) | ((pixformat == Pixformat_JPEG) ? 0x20 : 0x00));
	 
   OV5640_Reg = SCCB_ReadReg_16Bit(0x3002);   // ��ȡ�Ĵ���ֵ��Bit[7]��Bit[4]��Bit[2]ʹ�� VFIFO��JFIFO��JPG
	SCCB_WriteReg_16Bit(0x3002,(OV5640_Reg & 0xE3) | ((pixformat == Pixformat_JPEG) ? 0x00 : 0x1C));
	 
   OV5640_Reg = SCCB_ReadReg_16Bit(0x3006);   // ��ȡ�Ĵ���ֵ��Bit[5]��Bit[3] �����Ƿ�ʹ��JPGʱ��
	SCCB_WriteReg_16Bit(0x3006,(OV5640_Reg & 0xD7) | ((pixformat == Pixformat_JPEG) ? 0x28 : 0x00));

}

/***************************************************************************************************************************************
*	�� �� ��: OV5640_Set_JPEG_QuantizationScale
*
*	��ڲ���?: scale - ѹ���ȼ���ȡֵ 0x01~0x3F
*
*	��������: ��ֵԽ��ѹ����Խ�������õ���ͼƬռ�ÿռ��ԽС�������?�Ļ��ʻ���ͻ������е���
*
*****************************************************************************************************************************************/

void OV5640_Set_JPEG_QuantizationScale(uint8_t scale)
{
	SCCB_WriteReg_16Bit(0x4407, scale); 	// JPEG ѹ���ȼ�
}


/***************************************************************************************************************************************
*	�� �� ��: OV5640_Set_Framesize
*
*	��ڲ���?:  width - ʵ�����ͼ��ĳ��ȣ�height - ʵ�����ͼ��Ŀ���
*
*	��������: ����ʵ�������ͼ���С�����ź�
*
*	˵    ��: 1. ��Ҫע����ǣ��?���õ�ͼ�񳤡�����Ҫ�����ʼ�������?ISP���ڵı�������Ȼͼ������
*            2. ���������������ͼ��ֱ���ԽС֡�ʾ�Խ�ߣ�֡��ֻ�ͳ�ʼ�������ã�PLL��HTS��VTS���й�
*
*****************************************************************************************************************************************/

int8_t OV5640_Set_Framesize(uint16_t width,uint16_t height)
{
// OV5640�ĺܶ���������?�������ֶ�Ӧ group ������	
    SCCB_WriteReg_16Bit(OV5640_GroupAccess,0X03);  	// ��ʼ group 3 ������
	
    SCCB_WriteReg_16Bit(OV5640_TIMING_DVPHO_H,width>>8);			// DVPHO����������?ƽ�ߴ�
    SCCB_WriteReg_16Bit(OV5640_TIMING_DVPHO_L,width&0xff);
    SCCB_WriteReg_16Bit(OV5640_TIMING_DVPVO_H,height>>8);		// DVPVO�����������ֱ�ߴ�?
    SCCB_WriteReg_16Bit(OV5640_TIMING_DVPVO_L,height&0xff);

    SCCB_WriteReg_16Bit(OV5640_GroupAccess,0X13);		// ��������
    SCCB_WriteReg_16Bit(OV5640_GroupAccess,0Xa3);		// ��������
	
	return OV5640_Success; 
}

/***************************************************************************************************************************************
*	�� �� ��: OV5640_Set_Horizontal_Mirror
*
*	��ڲ���?:  ConfigState - ��1ʱ��ͼ����?ƽ������0ʱ�ָ�����
*
*	��������: �������������ͼ���Ƿ����ˮƽ����
*
*****************************************************************************************************************************************/
int8_t OV5640_Set_Horizontal_Mirror( int8_t ConfigState )
{
   uint8_t OV5640_Reg;  // �Ĵ�����ֵ

   OV5640_Reg = SCCB_ReadReg_16Bit(OV5640_TIMING_Mirror);   // ��ȡ�Ĵ���ֵ

// Bit[2:1]���������Ƿ�ˮƽ����
   if ( ConfigState == OV5640_Enable )    // ���ʹ�ܾ���?
   { 
      OV5640_Reg |= 0X06;  
   } 
   else                    // ȡ������
   {
      OV5640_Reg &= 0xF9; 	
   }
   return  SCCB_WriteReg_16Bit(OV5640_TIMING_Mirror,OV5640_Reg);   // д��Ĵ���?
}

/***************************************************************************************************************************************
*	�� �� ��: OV5640_Set_Vertical_Flip
*
*	��ڲ���?:  ConfigState - ��1ʱ��ͼ��ᴹֱ���?����0ʱ�ָ�����
*
*	��������: �������������ͼ���Ƿ���д�ֱ��ת
*
*****************************************************************************************************************************************/
int8_t OV5640_Set_Vertical_Flip( int8_t ConfigState )
{
   uint8_t OV5640_Reg;  // �Ĵ�����ֵ

   OV5640_Reg = SCCB_ReadReg_16Bit(OV5640_TIMING_Flip);          // ��ȡ�Ĵ���ֵ

// Bit[2:1]���������Ƿ�ֱ��ת
   if ( ConfigState == OV5640_Enable )   
   { 
		OV5640_Reg |= 0X06;       
   } 
   else   // ȡ����ת
   {
      OV5640_Reg &= 0xF9; 	
   }
   return  SCCB_WriteReg_16Bit(OV5640_TIMING_Flip,OV5640_Reg);   // д��Ĵ���?
}


/***************************************************************************************************************************************
*	�� �� ��: OV5640_Set_Brightness
*
*	��ڲ���?:  Brightness - ���ȣ�������Ϊ9���ȼ���4��3��2��1��0��-1��-2��-3��-4   ������Խ������Խ��                
*
*	˵    ��: 1. ֱ��ʹ��OV5640�ֲ�����Ĵ���?
*            2. ����Խ�ߣ������Խ���������ǻ��ģ��һЩ
*				 2. ����̫�ͣ���������
*
*****************************************************************************************************************************************/
void OV5640_Set_Brightness(int8_t Brightness)
{
	Brightness = Brightness+4;
	SCCB_WriteReg_16Bit(OV5640_GroupAccess,0X03);  	// ��ʼ group 3 ������

	SCCB_WriteReg_16Bit( 0x5587, OV5640_Brightness_Config[Brightness][0]);	
	SCCB_WriteReg_16Bit( 0x5588, OV5640_Brightness_Config[Brightness][1]);
	
	SCCB_WriteReg_16Bit(OV5640_GroupAccess,0X13);		// ��������
	SCCB_WriteReg_16Bit(OV5640_GroupAccess,0Xa3);		// ��������	
}

/***************************************************************************************************************************************
*	�� �� ��: OV5640_Set_Contrast
*
*	��ڲ���?: Contrast - �Աȶȣ�������Ϊ7���ȼ���3��2��1��0��-1��-2 ��-3                  
*
*	˵    ��: 1. ֱ��ʹ��OV5640�ֲ�����Ĵ���?
*            2. �Աȶ�Խ�ߣ�����Խ�������ڰ�Խ�ӷ���
*
*****************************************************************************************************************************************/
void OV5640_Set_Contrast(int8_t Contrast)
{
	Contrast = Contrast+3;
	SCCB_WriteReg_16Bit(OV5640_GroupAccess,0X03);  	// ��ʼ group 3 ������

	SCCB_WriteReg_16Bit( 0x5586, OV5640_Contrast_Config[Contrast][0]);	
	SCCB_WriteReg_16Bit( 0x5585, OV5640_Contrast_Config[Contrast][1]);
	
	SCCB_WriteReg_16Bit(OV5640_GroupAccess,0X13);		// ��������
	SCCB_WriteReg_16Bit(OV5640_GroupAccess,0Xa3);		// ��������	
}
/************************************************************************************************************************