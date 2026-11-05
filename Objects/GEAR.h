#ifndef __GEAR_H
#define __GEAR_H

#include "stm32f10x.h"

/* ======================= 74HC148 挡位编码器接线 =======================
 * 74HC148 是 8线-3线优先编码器,输出低电平有效(A2A1A0 为输入序号反码)。
 * STM32 侧全部用内部上拉输入,148 输出低时读到 0。
 *
 *   PA4 = A0(编码器输出 bit0)
 *   PA5 = A1(编码器输出 bit1)
 *   PA6 = A2(编码器输出 bit2)
 *   PB1 = GS(Group Select,有有效输入时为低)
 *   PB0 = EO(Enable Output,级联用,本设计仅配置备用)
 *
 * 输入接线约定:
 *   I0=空挡, I1~I6=1~6挡, I7=倒挡
 *
 * 挡位高低切换:PA3,接 3.3V=高挡组(7~12挡+倒挡2),接 GND=低挡组(0~6挡+倒挡1)
 */

/* 挡位编号定义 */
#define GEAR_NEUTRAL     0u    /* 空挡 */
#define GEAR_REVERSE1    0xFEu /* 倒挡1(低挡组) */
#define GEAR_REVERSE2    0xFFu /* 倒挡2(高挡组) */
/* 前进挡 1~12 直接就是数值本身 */

void    GEAR_Init(void);          /* 配置 PA3/4/5/6, PB0/1 为上拉输入 */
uint8_t GEAR_Read(void);          /* 读当前挡位:0=空挡,1~12=前进挡,0xFE/0xFF=倒挡 */
uint8_t GEAR_IsHighRange(void);   /* 返回1=高挡组(PA3=3.3V),0=低挡组 */

#endif /* __GEAR_H */
