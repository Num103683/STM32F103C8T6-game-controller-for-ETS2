/**
  ******************************************************************************
  * @file    usb_hw.h
  * @brief   USB 硬件初始化接口(GPIO/时钟/中断/USB 上电)
  *
  *   提供 USB 设备的硬件层初始化与反初始化,以及序列号、产品字符串
  *   的获取接口(供 usb_prop.c 在枚举阶段调用)。实现在 usb_hw.c。
  ******************************************************************************
  */
#ifndef __USB_HW_H
#define __USB_HW_H

#include "stm32f10x.h"
#include "usb_type.h"

/* 读取芯片唯一 ID 生成 USB 序列号字符串(供 usb_prop.c 枚举阶段调用) */
void Get_SerialNum(void);

/* 初始化 USB 硬件:GPIO、USB 时钟、USB 中断、USB 设备宏单元 */
void USB_HW_Init(void);

/* 反初始化 USB 硬件:关闭 USB 时钟并将 DM/DP 拉低(强制重新枚举) */
void USB_HW_DeInit(void);

#endif /* __USB_HW_H */
