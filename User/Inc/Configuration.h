#ifndef _CONF_H_
#define _CONF_H_				 



/**************************************************************/
// 1.用于接收外部指令的串口和W5500以太网模块输入设置

#define USE_USART2_AS_INPUT		

#define USE_SPI3_FOR_ETHRENET_MODULE	
//消息回环中一个单元的大小,单位为Byte;BUFFER缓冲变量的大小 为此3倍
#define MAX_BUFFER_LENGTH 128
//这是消息回环的链表数量设置
#define buffer_pool_size 24 //>1   <256
/***************************************************************/















#endif // !_CONF_H_
