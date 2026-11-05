/**
  ******************************************************************************
  * @file    usb_endp.h
  * @brief   USB 端点处理接口(HID 游戏手柄报告发送)
  *
  *   提供 STM32 向 PC 发送 HID 报告的 API,以及查询发送是否就绪的接口。
  *   实现在 usb_endp.c。
  ******************************************************************************
  */
#ifndef __USB_ENDP_H__
#define __USB_ENDP_H__

#include "stdint.h"

/**
  * @brief  查询 HID 报告端点(EP1 IN)是否已就绪,可发送下一包。
  * @retval 1 = 上一包发送完成,可发送;0 = 正在发送或设备未配置
  */
uint8_t HID_IsReadyToSend(void);

/**
  * @brief  通过 EP1 IN(中断)向 PC 发送一包 HID 报告。
  * @param  ptrBuffer: 报告数据首址
  * @param  send_length: 字节数,必须 <= JOY_REPORT_SIZE(12)
  * @retval 1 = 已提交发送;0 = 未提交(端点忙或设备未配置)
  */
uint8_t HID_SendReport(uint8_t *ptrBuffer, uint8_t send_length);

/* EP1 IN 端点"上次传输完成"标志:1=可发送,0=正在发送。
   在 EP1_IN_Callback 中置 1,在 HID_SendReport 中清 0。
   usb_prop.c 的 HID_Reset 会将其置 1,确保复位后允许发送。 */
extern volatile uint8_t EP1_PrevXferComplete;

#endif /* __USB_ENDP_H__ */
