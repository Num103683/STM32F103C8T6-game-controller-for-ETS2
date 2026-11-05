/**
  ******************************************************************************
  * @file    sys_tick.h
  * @brief   SysTick 1ms 周期中断与毫秒时间戳接口
  *
  *   基于 Cortex-M3 的 SysTick 计时器,提供:
  *   - SysTick_Init():把 SysTick 配置为 1ms 周期中断
  *   - SysTick_Inc():在 SysTick 中断中调用,递增毫秒计数
  *   - GetMillis():获取开机以来的毫秒数(用于定时任务触发)
  *
  *   注意:本文件【不声明】Delay_ms / Delay_us。阻塞式延时统一由工程中
  *         原有 Delay.c / Delay.h(寄存器轮询方式)提供,需要延时的文件
  *         请直接 #include "Delay.h",以避免重复定义。
  ******************************************************************************
  */
#ifndef __SYS_TICK_H
#define __SYS_TICK_H

#include "stdint.h"

/* 初始化 SysTick,使其每 1ms 产生一次中断(系统时钟 72MHz) */
void SysTick_Init(void);

/* 供 SysTick 中断调用:递增毫秒计数。由 stm32f10x_it.c 调用 */
void SysTick_Inc(void);

/* 获取开机以来的毫秒数(用于"定时发送"等周期任务) */
uint32_t GetMillis(void);

#endif /* __SYS_TICK_H */
