//本文件仅用于 extern声明外部全局变量

#include "stm32f1xx.h"
#include "User/Inc/Cmd_Proc.h"

extern uint8_t usart_rx_buffer[MAX_BUFFER_LENGTH*3];
extern uint8_t ethernet_rx_buffer[MAX_BUFFER_LENGTH];
extern  CMD_POOL_AREA * cmd_wrtpointer;
extern  CMD_POOL_AREA * cmd_readpointer;
extern uint8_t remain_pool_count; 
extern uint8_t  used_pool_count;
extern uint8_t cmd_to_process;