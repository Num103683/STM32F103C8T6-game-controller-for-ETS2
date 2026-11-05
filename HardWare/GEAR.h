#ifndef __GEAR_H
#define __GEAR_H

#include "stm32f10x.h"

/* ======================= 挡位直接电平检测接线 =======================
 * 换挡器每个挡位槽独立一根信号线:挂入=外部输入高电平(3V3),
 * 未挂入=低电平。STM32 侧全部用内部下拉输入,断线时天然为低(=未挂挡),
 * 失效方向安全。每个前进槽配合 PA3 高低挡开关得到两个挡:
 *
 *   槽1 PA8    : 低=1挡   高=2挡
 *   槽2 PB15   : 低=3挡   高=4挡
 *   槽3 PB14   : 低=5挡   高=6挡
 *   槽4 PB13   : 低=7挡   高=8挡
 *   槽5 PB12   : 低=9挡   高=10挡
 *   槽6 PA4    : 低=11挡  高=12挡
 *   倒挡槽A PA5 : 低=R1   高=R2
 *   倒挡槽B PA6 : 低=R3   高=R4
 *
 * 高低挡切换:PA3,三脚拨片开关(一端3V3/一端GND/拨片接引脚),
 * 引脚始终有确定电平。低电平=槽内第一个挡(奇数挡/R1/R3),
 * 高电平=槽内第二个挡(偶数挡/R2/R4)。
 *
 * 空挡判定:所有挡位引脚全低并持续 GEAR_NEUTRAL_DELAY_MS 才报空挡,
 * 延时内保持返回上一有效挡位(换挡途经空挡位不闪断、兼吃触点抖动);
 * 延时内挂入新挡位则立即生效、计时作废。
 */

/* 全低持续多少毫秒后报空挡(按换挡手感可调) */
#define GEAR_NEUTRAL_DELAY_MS   300u

/* 挡位编号定义 */
#define GEAR_NEUTRAL     0u    /* 空挡 */
#define GEAR_REVERSE1    0xFEu /* 倒挡槽A + 低挡组 = R1 */
#define GEAR_REVERSE2    0xFFu /* 倒挡槽A + 高挡组 = R2 */
#define GEAR_REVERSE3    0xFCu /* 倒挡槽B + 低挡组 = R3 */
#define GEAR_REVERSE4    0xFDu /* 倒挡槽B + 高挡组 = R4 */
/* 前进挡 1~12 直接就是数值本身 */

void    GEAR_Init(void);          /* 配置 8 个挡位槽引脚为下拉输入,PA3 上拉输入 */
uint8_t GEAR_Read(void);          /* 读当前挡位:0=空挡,1~12=前进挡,0xFC~0xFF=倒挡 */
uint8_t GEAR_IsHighRange(void);   /* 返回1=高挡组(PA3=3.3V),0=低挡组 */

#endif /* __GEAR_H */
