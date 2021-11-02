#include "User/Inc/Debug.h"
#include "main.h"
#include "usart.h"

void DEBUG_SerialPrint(char  message[])
{
	if (!LL_USART_IsEnabled(DEBUG_Serial)) //������û�б�ʹ�ܣ������½��в���
	{ //������CubeMX��������ɴ��ڵĳ�ʼ��.
		LL_USART_Enable(DEBUG_Serial);
		LL_USART_ClearFlag_RXNE(DEBUG_Serial);
	}
	uint8_t dat, i=0;
	do
	{
		dat = *message;
		LL_USART_TransmitData8(DEBUG_Serial, dat);
		i++;
		while (!LL_USART_IsActiveFlag_TXE(DEBUG_Serial)) ;
	} while ((i<129)&&(dat!='\n'));
}
