#include "User/Inc/Cmd_Proc.h"
#include "User/Inc/public_varibles.h"
#include "User/Inc/EthernetModule_Driver.h"
#include "main.h"
#include "User/Inc/movement_und_motor.h"
#ifdef DEBUG
#include "User/Inc/Debug.h"
#endif 


// '\n'==0x1A


uint8_t usart_rx_buffer[MAX_BUFFER_LENGTH*3]; 	//	用于串口DMA的读取缓存
uint8_t ethernet_rx_buffer[MAX_BUFFER_LENGTH];  //用于W5500(SPI)以太网模块的读取缓存

/////////////////////////////消息回环链表结构体和外部工作指针

//链表单元结构体定义在Cmd_Proc.h中

//工作指针
CMD_POOL_AREA * cmd_wrtpointer = NULL;//写指针; 写指针停留在下一个待写入的单元
CMD_POOL_AREA * cmd_readpointer = NULL;//读取指针；停留在下一个待读取的单元

//计数器
uint8_t remain_pool_count = buffer_pool_size; //剩余可写入的单元数
uint8_t used_pool_count = 0;	//已经使用的单元数
uint8_t cmd_to_process=0;	//积压的需要处理的指令数量

/******************************* Functions ***************************************/

void CMD_IN_UART2_Init()//请在系统初始化之后,未进入RTOS前调用！！！
{
	//1.创建链表回环
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
	//2.初始化串口和DMA(DMA1,6通道);串口为USART2。
	//实现原理为DMA+串口空闲中断
	LL_USART_ReceiveData8(USART2);
	LL_USART_Disable(USART2);
	//LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_6, LL_DMA_DIRECTION_PERIPH_TO_MEMORY); //设置传输方向
	//LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_6, LL_DMA_PRIORITY_HIGH);//通道优先度
	//LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_6, LL_DMA_MODE_NORMAL);
    //LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_6, LL_DMA_PERIPH_NOINCREMENT);
	//LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_6, LL_DMA_MEMORY_INCREMENT);
	//LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_6, LL_DMA_PDATAALIGN_BYTE);
	//LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_6, LL_DMA_MDATAALIGN_BYTE);
	LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_6, LL_USART_DMA_GetRegAddr(USART2));
	LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_6, (uint32_t)usart_rx_buffer);//设置数据长度,达到长度DMA传输完成挂起。此时再写入数据串口Overwrite
	LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_6, MAX_BUFFER_LENGTH);	
	LL_USART_EnableDMAReq_RX(USART2);
	LL_USART_Enable(USART2);//开启串口
	LL_USART_ClearFlag_IDLE(USART2);//清除空闲状态位
	LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_6);//开启DMA通道
	
	//3.配置Ethrenet物联网模块，未完成
	 Ethernet_Module_Init();
}
//接收并处理收到的数据
void CMD_Receive_and_Process()    //重要! 此函数应当在独立线程循环中调用!!!
{
											 //线程优先级6，与执行线程的优先级相同，执行后阻塞。
										 //返回值为False,则循环最后切换线程
	osThreadId_t hThread = NULL;
	hThread = osThreadGetId();
	
	if (remain_pool_count < 1)
	{//当储存指令的消息环回空间不足，此进程进入阻塞状态
      #ifdef DEBUG
		DEBUG_SerialPrint("No loop space to store data,shifting Thread! \n");	  
      #endif
	}
	else
	{
		//1.USART2 接收不定长度指令(DMA+串口空闲实现)
	if(LL_USART_IsActiveFlag_IDLE(USART2)) {
			//串口IDLE空闲,说明已经接收到数据
			uint16_t dat_length;   //数据长度,接收到的字节数量
			dat_length = MAX_BUFFER_LENGTH * 3 - LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_6);
			LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_6);  //关闭通道
			LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_6, (uint32_t)usart_rx_buffer);
			LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_6, MAX_BUFFER_LENGTH * 3);    //处理完成后请清零IDLE，并调用LL_DMA_EnableChannel
			
		//从Buffer中获取指令，注意可能有多重指令。 暂时不判断指令有效性
			uint16_t end_poces[buffer_pool_size];  end_poces[0] = 0U;   //记录换行符位置
			uint8_t num_of_cmd = 0U;  //Buffer中的指令数量
			for(uint16_t i = 0 ; i < dat_length ; i++) //max = buffer[dat_length-1] cor
			{
				 //遍历 
				if(usart_rx_buffer[i] == '\n')
				{
					end_poces[num_of_cmd] = i;//储存换行符的位置;几个指令(不管有效性)对应几个换行符
					num_of_cmd++;  
				}
			}
		//写入
		if(num_of_cmd > remain_pool_count){ num_of_cmd = remain_pool_count; }//抛弃多出的指令

		if(num_of_cmd>0){ //写入第一个指令
			if(end_poces[0] + 1 <= MAX_BUFFER_LENGTH)
			{
			
				for (uint16_t j = 0; j <= end_poces[0]; j++)
				{
					cmd_wrtpointer->data[j] = usart_rx_buffer[j];
				}
				cmd_wrtpointer->data_length = end_poces[0] + 1;   //记录数据长度
				cmd_wrtpointer = cmd_wrtpointer->next;
				used_pool_count ++; cmd_to_process ++; remain_pool_count --;
			 }
			#ifdef DEBUG//调试输出
				else
				{
					DEBUG_SerialPrint("Command is too long, OVERFLOWING!! \n");
				}		  
			#endif 

			
		}
		for (uint16_t m = 1; m < num_of_cmd; m++) //m为代号
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
#ifdef DEBUG     //调试输出
			else
			{
				DEBUG_SerialPrint("Command is too long, OVERFLOWING!! \n");
			}		  
#endif 			
		}	
		
		
		LL_USART_ClearFlag_IDLE(USART2);//清空IDLE
		LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_6);//重新启动DMA传输接收数据
	}
	
	}
	osThreadYield();//切换线程
}
//执行函数
//所在线程优先级6同读取，运行后依此执行指令至缓冲区内无指令(通过线程循环)，随后yield
//运动线程作为5优先级线程独立，由本函数启动。执行完毕运动线程block自身。
void CMD_Execute()
{	if (cmd_to_process == 0){osThreadYield();} //若无可执行的指令，即刻尝试切换任务
	//1.存在多条指令时通过循环全部执行，该循环通过线程循环实现.
	
 if(cmd_to_process > 0)
	{	//若指令带行号，去除行号 Nxxx	
		uint16_t serial = 0U;
		if (cmd_readpointer->data[0] == 'N')
		{
			while ((cmd_readpointer->data[serial] != '\n')&&(cmd_readpointer->data[serial] != ' ')) {serial++; }
			//重新整理数据
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
		//switch判断指令
		switch(cmd_readpointer->data[0])
		{
			//M系列指令
		case 'M' :     

		
		
		
		
		
		
			break;
			//G系列指令
		case'G' :
		
		
		
		
		
			break;
			//无效指令
		default :        //包括为'\n'的情形
			#ifdef  DEBUG
			DEBUG_SerialPrint("command illegal or useless!!");		  
			#endif
		
			break;
		}//switch
	
	}
	
}


/*************************************************************/

//字符串比较函数
//dat，ref为比较的两个ascii字符串；start代表比较的起始位 ;length代表比较的长度
//从dat[start]和ref[start]开始(第一个)，[start+length-1]最后一个
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