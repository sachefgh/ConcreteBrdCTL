#ifndef _CMD_PROC_H_
#define _CMD_PROC_H_
#include "User/Inc/Configuration.h"
#include "stm32f1xx_ll_gpio.h"
typedef struct CMD_POOL
{
	struct CMD_POOL* head; 
	
	uint8_t data[MAX_BUFFER_LENGTH];
	
	uint16_t data_length; //数据长度指示,从1开始
	
	struct CMD_POOL* next;
}
CMD_POOL_AREA;


void CMD_IN_UART2_Init();
void CMD_Receive_and_Process();
void CMD_Execute();
ErrorStatus string_compare(uint8_t *dat, uint8_t *ref, uint16_t start, uint16_t length);



#endif // !_CMD_PROC_H_
