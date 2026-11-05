/**
  ******************************************************************************
  * @file    sys_tick.c
  * @brief   SysTick 1ms 周期中断与毫秒时间戳
  *
  *   注意:本文件不提供 Delay_ms / Delay_us。工程中已有 Delay.c 提供
  *   这两个延时函数(寄存器轮询方式),为避免重复定义,此处不再实现。
  *
  *   本文件只负责:
  *   - SysTick_Init():把 SysTick 配置为 1ms 周期中断(72MHz 主频)
  *   - SysTick_Inc():在 SysTick_Handler 中调用,递增毫秒计数
  *   - GetMillis():返回开机以来的毫秒数(供定时发送使用)
  *
  *   重要:Delay.c 的 Delay_us 必须在调用前后保存/还原 SysTick 寄存器,
  *   否则延时调用会破坏这里的周期中断配置,导致 GetMillis 停止计数。
  ******************************************************************************
  */
#include "sys_tick.h"
#include "stm32f10x.h"

/* 开机以来的毫秒数(每 1ms 中断递增一次) */
static volatile uint32_t g_millis = 0;

/**
  * @brief  初始化 SysTick,使其每 1ms 产生一次中断。
  *         72MHz 主频时重装载值 = 72000-1(SysTick_Config 内部处理)。
  *         必须在 Delay 之前或之后均可调用,但需在 GetMillis 使用前调用。
  */
void SysTick_Init(void)
{
    if (SysTick_Config(SystemCoreClock / 1000))
    {
        while (1)
        {
        }
    }
}

/**
  * @brief  SysTick 中断服务:递增毫秒计数。
  *         在 stm32f10x_it.c 的 SysTick_Handler 中被调用。
  */
void SysTick_Inc(void)
{
    g_millis++;
}

/**
  * @brief  获取开机以来的毫秒数。
  * @retval 自 SysTick_Init 起的毫秒数,约 49.7 天后回绕到 0
  */
uint32_t GetMillis(void)
{
    return g_millis;
}
