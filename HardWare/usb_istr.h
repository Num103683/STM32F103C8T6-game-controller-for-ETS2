/**
  ******************************************************************************
  * @file    usb_istr.h
  * @brief   USB 中断服务程序接口
  *
  *   声明 USB_Istr() 主中断处理函数,以及各端点回调函数原型。
  *   端点回调函数中未使用的已在 usb_conf.h 中映射到 NOP_Process,
  *   实际需要处理的 EP5_IN_Callback / EP4_OUT_Callback 在 usb_endp.c
  *   中实现。
  ******************************************************************************
  */
#ifndef __USB_ISTR_H
#define __USB_ISTR_H

#include "usb_conf.h"

/* USB 低优先级中断主处理函数(由 USB_LP_CAN1_RX0_IRQHandler 调用) */
void USB_Istr(void);

/* 各端点 IN/OUT 回调函数原型(未使用的由 usb_conf.h 映射为 NOP_Process) */
void EP1_IN_Callback(void);
void EP2_IN_Callback(void);
void EP3_IN_Callback(void);
void EP4_IN_Callback(void);
void EP5_IN_Callback(void);
void EP6_IN_Callback(void);
void EP7_IN_Callback(void);

void EP1_OUT_Callback(void);
void EP2_OUT_Callback(void);
void EP3_OUT_Callback(void);
void EP4_OUT_Callback(void);
void EP5_OUT_Callback(void);
void EP6_OUT_Callback(void);
void EP7_OUT_Callback(void);

#endif /* __USB_ISTR_H */
