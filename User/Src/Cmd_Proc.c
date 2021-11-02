#include "User/Inc/Cmd_Proc.h"
#include "User/Inc/public_varibles.h"
#include "User/Inc/EthernetModule_Driver.h"
#include "main.h"
#include "User/Inc/movement_und_motor.h"
#ifdef DEBUG
#include "User/Inc/Debug.h"
#endif 


// '\n'==0x1A


uint8_t usart_rx_buffer[MAX_BUFFER_LENGTH*3]; 	//	���ڴ���DMA�Ķ�ȡ����
uint8_t ethernet_rx_buffer[MAX_BUFFER_LENGTH];  //����W5500(SPI)��̫��ģ��Ķ�ȡ����

/////////////////////////////��Ϣ�ػ�����ṹ����ⲿ����ָ��

//����Ԫ�ṹ�嶨����Cmd_Proc.h��

//����ָ��
CMD_POOL_AREA * cmd_wrtpointer = NULL;//дָ��; дָ��ͣ������һ����д��ĵ�Ԫ
CMD_POOL_AREA * cmd_readpointer = NULL;//��ȡָ�룻ͣ������һ������ȡ�ĵ�Ԫ

//������
uint8_t remain_pool_count = buffer_pool_size; //ʣ���д��ĵ�Ԫ��
uint8_t used_pool_count = 0;	//�Ѿ�ʹ�õĵ�Ԫ��
uint8_t cmd_to_process=0;	//��ѹ����Ҫ�����ָ������

/******************************* Functions ***************************************/

void CMD_IN_UART2_Init()//����ϵͳ��ʼ��֮��,δ����RTOSǰ���ã�����
{
	//1.��������ػ�
	cmd_wrtpointer = ( CMD_POOL_AREA *)malloc(sizeof(CMD_POOL_AREA));
	cmd_readpointer = cmd_wrtpointer;
	CMD_POOL_AREA * p = cmd_wrtpointer;
	for (uint8_t i = 0; i < buffer_pool_size - 1 ; i++)
	{
		p->next = (CMD_POOL_AREA *)malloc(sizeof(CMD_POOL_AREA));
		p->next->head = p;
		p = p->next;
	}
	p->next = cmd_wrtpointer;
	cmd_wrtpointer->head = p;
	//2.��ʼ�����ں�DMA(DMA1,6ͨ��);����ΪUSART2��
	//ʵ��ԭ��ΪDMA+���ڿ����ж�
	LL_USART_ReceiveData8(USART2);
	LL_USART_Disable(USART2);
	//LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_6, LL_DMA_DIRECTION_PERIPH_TO_MEMORY); //���ô��䷽��
	//LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_6, LL_DMA_PRIORITY_HIGH);//ͨ�����ȶ�
	//LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_6, LL_DMA_MODE_NORMAL);
    //LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_6, LL_DMA_PERIPH_NOINCREMENT);
	//LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_6, LL_DMA_MEMORY_INCREMENT);
	//LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_6, LL_DMA_PDATAALIGN_BYTE);
	//LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_6, LL_DMA_MDATAALIGN_BYTE);
	LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_6, LL_USART_DMA_GetRegAddr(USART2));
	LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_6, (uint32_t)usart_rx_buffer);//�������ݳ���,�ﵽ����DMA������ɹ��𡣴�ʱ��д�����ݴ���Overwrite
	LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_6, MAX_BUFFER_LENGTH);	
	LL_USART_EnableDMAReq_RX(USART2);
	LL_USART_Enable(USART2);//��������
	LL_USART_ClearFlag_IDLE(USART2);//�������״̬λ
	LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_6);//����DMAͨ��
	
	//3.����Ethrenet������ģ�飬δ���
	 Ethernet_Module_Init();
}
//���ղ������յ�������
void CMD_Receive_and_Process()    //��Ҫ! �˺���Ӧ���ڶ����߳�ѭ���е���!!!
{
											 //�߳����ȼ�6����ִ���̵߳����ȼ���ͬ��ִ�к�������
										 //����ֵΪFalse,��ѭ������л��߳�
	osThreadId_t hThread = NULL;
	hThread = osThreadGetId();
	
	if (remain_pool_count < 1)
	{//������ָ�����Ϣ���ؿռ䲻�㣬�˽��̽�������״̬
      #ifdef DEBUG
		DEBUG_SerialPrint("No loop space to store data,shifting Thread! \n");	  
      #endif
	}
	else
	{
		//1.USART2 ���ղ�������ָ��(DMA+���ڿ���ʵ��)
	if(LL_USART_IsActiveFlag_IDLE(USART2)) {
			//����IDLE����,˵���Ѿ����յ�����
			uint16_t dat_length;   //���ݳ���,���յ����ֽ�����
			dat_length = MAX_BUFFER_LENGTH * 3 - LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_6);
			LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_6);  //�ر�ͨ��
			LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_6, (uint32_t)usart_rx_buffer);
			LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_6, MAX_BUFFER_LENGTH * 3);    //������ɺ�������IDLE��������LL_DMA_EnableChannel
			
		//��Buffer�л�ȡָ�ע������ж���ָ� ��ʱ���ж�ָ����Ч��
			uint16_t end_poces[buffer_pool_size];  end_poces[0] = 0U;   //��¼���з�λ��
			uint8_t num_of_cmd = 0U;  //Buffer�е�ָ������
			for(uint16_t i = 0 ; i < dat_length ; i++) //max = buffer[dat_length-1] cor
			{
				 //���� 
				if(usart_rx_buffer[i] == '\n')
				{
					end_poces[num_of_cmd] = i;//���滻�з���λ��;����ָ��(������Ч��)��Ӧ�������з�
					num_of_cmd++;  
				}
			}
		//д��
		if(num_of_cmd > remain_pool_count){ num_of_cmd = remain_pool_count; }//���������ָ��

		if(num_of_cmd>0){ //д���һ��ָ��
			if(end_poces[0] + 1 <= MAX_BUFFER_LENGTH)
			{
			
				for (uint16_t j = 0; j <= end_poces[0]; j++)
				{
					cmd_wrtpointer->data[j] = usart_rx_buffer[j];
				}
				cmd_wrtpointer->data_length = end_poces[0] + 1;   //��¼���ݳ���
				cmd_wrtpointer = cmd_wrtpointer->next;
				used_pool_count ++; cmd_to_process ++; remain_pool_count --;
			 }
			#ifdef DEBUG//�������
				else
				{
					DEBUG_SerialPrint("Command is too long, OVERFLOWING!! \n");
				}		  
			#endif 

			
		}
		for (uint16_t m = 1; m < num_of_cmd; m++) //mΪ����
		{
			//end_poces[m-1]+1 to  end_roces[m] are data to acquire
			if(end_poces[m] - end_poces[m - 1] <= MAX_BUFFER_LENGTH)
			{
			
				for (uint16_t cs = end_poces[m - 1] + 1; cs <= end_poces[m]; cs++)
				{
					cmd_wrtpointer->data[cs - end_poces[m - 1] - 1] = usart_rx_buffer[cs];
				}
				cmd_wrtpointer->data_length = end_poces[m] - end_poces[m - 1];
				cmd_wrtpointer = cmd_wrtpointer->next;
				used_pool_count += 1; cmd_to_process += 1; remain_pool_count -= 1;
			}
#ifdef DEBUG     //�������
			else
			{
				DEBUG_SerialPrint("Command is too long, OVERFLOWING!! \n");
			}		  
#endif 			
		}	
		
		
		LL_USART_ClearFlag_IDLE(USART2);//���IDLE
		LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_6);//��������DMA�����������
	}
	
	}
	osThreadYield();//�л��߳�
}
//ִ�к���
//�����߳����ȼ�6ͬ��ȡ�����к�����ִ��ָ��������������ָ��(ͨ���߳�ѭ��)�����yield
//�˶��߳���Ϊ5���ȼ��̶߳������ɱ�����������ִ������˶��߳�block����
void CMD_Execute()
{	if (cmd_to_process == 0){osThreadYield();} //���޿�ִ�е�ָ����̳����л�����
	//1.���ڶ���ָ��ʱͨ��ѭ��ȫ��ִ�У���ѭ��ͨ���߳�ѭ��ʵ��.
	
 if(cmd_to_process > 0)
	{	//��ָ����кţ�ȥ���к� Nxxx	
		uint16_t serial = 0U;
		if (cmd_readpointer->data[0] == 'N')
		{
			while ((cmd_readpointer->data[serial] != '\n')&&(cmd_readpointer->data[serial] != ' ')) {serial++; }
			//������������
			if(cmd_readpointer->data[serial] != '\n') {cmd_readpointer->data_length = 1; cmd_readpointer->data[0] = '\n'; }
			else
			{
				for (uint16_t i = serial + 1; i < cmd_readpointer->data_length; i++)
				{
					cmd_readpointer->data[i - serial - 1] = cmd_readpointer->data[i];
				}
				cmd_readpointer->data_length -= serial + 1;
			}
		}
		//switch�ж�ָ��
		switch(cmd_readpointer->data[0])
		{
			//Mϵ��ָ��
		case 'M' :     

		
		
		
		
		
		
			break;
			//Gϵ��ָ��
		case'G' :
		
		
		
		
		
			break;
			//��Чָ��
		default :        //����Ϊ'\n'������
			#ifdef  DEBUG
			DEBUG_SerialPrint("command illegal or useless!!");		  
			#endif
		
			break;
		}//switch
	
	}
	
}


/*************************************************************/

//�ַ����ȽϺ���
//dat��refΪ�Ƚϵ�����ascii�ַ�����start����Ƚϵ���ʼλ ;length����Ƚϵĳ���
//��dat[start]��ref[start]��ʼ(��һ��)��[start+length-1]���һ��
ErrorStatus string_compare(uint8_t *dat,uint8_t *ref ,uint16_t start,uint16_t length) 
{
	for (uint16_t i = start; i < start + length; i++)
	{
		if (dat[i] != ref[i])
		{
			return ERROR;
		}
	}
	return SUCCESS;
}