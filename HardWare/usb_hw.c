/**
  ******************************************************************************
  * @file    usb_hw.c
  * @brief   USB 硬件初始化(GPIO/时钟/中断)+ 序列号生成
  *
  *   - Set_System:配置 PA11(D-) / PA12(D+) 为复用推挽,并做一次 USB 重新枚举
  *   - Set_USBClock:选择 PLLCLK 1.5 分频作为 USB 48MHz 时钟源
  *   - USB_Interrupts_Config:使能 USB 低优先级中断(与 CAN1 RX0 共用)
  *   - Get_SerialNum:从芯片 UID 生成 12 位十六进制序列号字符串
  *   - USB_HW_Init:组合上述步骤并调用 USB_Init() 启动设备
  ******************************************************************************
  */
#include "usb_hw.h"
#include "usb_lib.h"
#include "usb_desc.h"
#include "usb_pwr.h"
#include "Delay.h"      /* Set_System 中重新枚举需要 Delay_ms(5) */
#include "string.h"

/* STM32F10x 芯片 UID 存储地址(96 位唯一 ID,分三段 32 位) */
#define ID1          (0x1FFFF7E8)
#define ID2          (0x1FFFF7EC)
#define ID3          (0x1FFFF7F0)

/* 内部函数原型 */
static void IntToUnicode(uint32_t value, uint8_t *pbuf, uint8_t len);

/**
  * @brief  配置 USB DM/DP 引脚(PA11/PA12)并触发一次重新枚举。
  *         重新枚举:先把 D+ 拉低 5ms 让主机识别断开,再切回复用功能。
  *         系统时钟(SystemInit)已在启动文件中调用,此处不再配置。
  */
void Set_System(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 使能 GPIOA 时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

//	/* 先把 PA11/PA12 配为推挽输出,拉低 D+ 5ms 触发主机重新枚举 */
//    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_11 | GPIO_Pin_12;
//    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
//    GPIO_Init(GPIOA, &GPIO_InitStructure);

//    GPIOA->ODR &= ~GPIO_Pin_12;   /* D+ 拉低 */
//    Delay_ms(5);

    /* 切回复用推挽,交由 USB 宏单元控制 */
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_11 | GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

/**
  * @brief  配置 USB 时钟。STM32F103 USB 需要 48MHz 时钟,
  *         取自 PLL 72MHz 的 1.5 分频(72/1.5=48),并使能 USB 外设时钟。
  */
void Set_USBClock(void)
{
    /* 选择 PLLCLK 1.5 分频作为 USB 时钟源 */
    RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);

    /* 使能 USB 外设时钟(APB1) */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);
}

/**
  * @brief  配置 USB 中断。STM32F103 的 USB 低优先级中断与 CAN1 RX0 共用
  *         中断号 USB_LP_CAN1_RX0_IRQn。设置为优先级组 2,抢占 1/子 0。
  */
void USB_Interrupts_Config(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    /* 2 位抢占 + 2 位子优先级 */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    NVIC_InitStructure.NVIC_IRQChannel                   = USB_LP_CAN1_RX0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                 = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/**
  * @brief  用芯片 UID 生成 USB 序列号字符串(12 位 hex)。
  *         写入 Composite_StringSerial 的 [2..17] 与 [18..25] 两段。
  */
void Get_SerialNum(void)
{
    uint32_t Device_Serial0, Device_Serial1, Device_Serial2;

    Device_Serial0 = *(uint32_t *)ID1;
    Device_Serial1 = *(uint32_t *)ID2;
    Device_Serial2 = *(uint32_t *)ID3;

    Device_Serial0 += Device_Serial2;   /* 混入第三段做扰动 */

    if (Device_Serial0 != 0)
    {
        /* 前 8 位 hex 写入 [2..17] */
        IntToUnicode(Device_Serial0, &Composite_StringSerial[2], 8);
        /* 后 4 位 hex 写入 [18..25] */
        IntToUnicode(Device_Serial1, &Composite_StringSerial[18], 4);
    }
}

/**
  * @brief  把 32 位整数转为 len 位 hex 字符(每个字符占 2 字节,高字节为 0)。
  *         结果写入 pbuf,使用 Unicode(ASCII)形式以匹配 USB 字符串描述符。
  */
static void IntToUnicode(uint32_t value, uint8_t *pbuf, uint8_t len)
{
    uint8_t idx;

    for (idx = 0; idx < len; idx++)
    {
        uint8_t nibble = (uint8_t)(value >> 28);   /* 取最高 4 位 */
        pbuf[2 * idx]     = (nibble < 0xA) ? (nibble + '0') : (nibble + 'A' - 10);
        pbuf[2 * idx + 1] = 0;                     /* Unicode 高字节为 0 */
        value <<= 4;
    }
}

/**
  * @brief  USB 硬件初始化总入口:GPIO -> 中断 -> 时钟 -> 启动 USB 设备。
  *         主程序在 SysTick 初始化之后调用本函数。
  */
void USB_HW_Init(void)
{
    Set_System();
    USB_Interrupts_Config();
    Set_USBClock();
    USB_Init();        /* USB Device Driver 库提供的初始化,会触发 CDC_init */
}

/**
  * @brief  USB 硬件反初始化:关闭 USB 时钟并把 DM/DP 拉低,强制断开。
  *         本工程主流程不使用,保留以备未来需要(如切换到 bootloader)。
  */
void USB_HW_DeInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 关闭 USB 外设时钟 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, DISABLE);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_11 | GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIOA->ODR &= ~(GPIO_Pin_11 | GPIO_Pin_12);
}
