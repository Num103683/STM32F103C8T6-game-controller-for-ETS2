#ifndef __CH450_H
#define __CH450_H

#include "stm32f10x.h"

/* ======================= 硬件连接(两线接口,GPIO模拟) =======================
 * PB10 = SCL(推挽输出,CH455只被动接收时钟)
 * PB11 = SDA(模拟51准双向IO:输出低=推挽强拉低,释放=输入上拉)
 */
#define CH450_SCL_PORT        GPIOB
#define CH450_SCL_PIN         GPIO_Pin_10
#define CH450_SCL_CLK         RCC_APB2Periph_GPIOB
#define CH450_SDA_PORT        GPIOB
#define CH450_SDA_PIN         GPIO_Pin_11

/* ======================= CH455 操作命令(输出字节1,手册6章) ======================= */
#define CH450_SYS_PARAM       0x48u   /* 设置系统参数,后跟字节2 */
#define CH450_GET_KEY_CODE    0x4Fu   /* 读取按键代码,随后从CH455读回1字节 */

/* 系统参数字节2:[KOFF][INTENS2..0][7SEG][SLEEP]0[ENA] */
#define CH450_SYS_KEY_ON      0x01u   /* ENA=1:使能键盘扫描;8段模式,亮度8/8 */

/* 按键代码:位7恒0,位6=1表示键按下,位5~3=SEG行,位2恒1,位1~0=DIG列
 * 有效按下代码 44H~7FH;释放后代码<40H */
#define CH450_KEY_NONE        0x00u

/* ======================= 对外接口 ======================= */
void    CH450_Init(void);         /* GPIO初始化 + 使能CH455键盘扫描 */
uint8_t CH450_ReadKeyCode(void);  /* 读原始8位按键代码(调试用) */
uint8_t CH450_ReadKey(void);      /* 有键按下返回编址04H~3FH,无键返回0 */
uint8_t CH450_KeyRow(uint8_t key);/* 按键编址 -> SEG行号0~6 */
uint8_t CH450_KeyCol(uint8_t key);/* 按键编址 -> DIG列号0~3 */

#endif /* __CH450_H */
