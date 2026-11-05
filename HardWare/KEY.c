#include "stm32f10x.h"  // Device header
#include "Delay.h"

void KEY_Init(void)              					        //函数初始化
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);              //启用时钟
	
	GPIO_InitTypeDef	GPIO_InitStructure;                                                         //定义结构体变量名
			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;                        //按键进行上拉输入
			GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_11;             //按键接入端口
			GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;             //按键输入的时候这个速度没有作用
	GPIO_Init(GPIOB,&GPIO_InitStructure);
}                            																//初始化结束

uint8_t 	KEY_GetNum(void)                                  //读取按键值的函数，uint8_t是unsigned char 的意思
{
	uint8_t KEYNum = 0;
	if( GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_1) == 0)
		{
		Delay_ms(20);
			while ( GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_1) == 0);
		Delay_ms(20);
			KEYNum = 1;
		}
	if( GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_11) == 0)
		{
		Delay_ms(20);
			while ( GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_11) == 0);
		Delay_ms(20);
			KEYNum = 1;
		}
	return KEYNum;
}
