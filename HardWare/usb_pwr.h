/**
  ******************************************************************************
  * @file    usb_pwr.h
  * @brief   USB 连接/断开与电源管理接口
  *
  *   定义 USB 设备状态机(UNCONNECTED/ATTACHED/POWERED/SUSPENDED/
  *   ADDRESSED/CONFIGURED)与唤醒状态机,声明 PowerOn/PowerOff/
  *   Suspend/Resume 等函数。实现在 usb_pwr.c。
  ******************************************************************************
  */
#ifndef __USB_PWR_H
#define __USB_PWR_H

#include "usb_type.h"

/* 唤醒状态机取值 */
typedef enum _RESUME_STATE
{
  RESUME_EXTERNAL,
  RESUME_INTERNAL,
  RESUME_LATER,
  RESUME_WAIT,
  RESUME_START,
  RESUME_ON,
  RESUME_OFF,
  RESUME_ESOF
} RESUME_STATE;

/* USB 设备状态 */
typedef enum _DEVICE_STATE
{
  UNCONNECTED,
  ATTACHED,
  POWERED,
  SUSPENDED,
  ADDRESSED,
  CONFIGURED
} DEVICE_STATE;

/* 电源管理函数 */
void     Suspend(void);
void     Resume_Init(void);
void     Resume(RESUME_STATE eResumeSetVal);
RESULT   PowerOn(void);
RESULT   PowerOff(void);

/* USB 设备当前状态(供 usb_endp.c 判断是否已 CONFIGURED 再发送数据) */
extern __IO uint32_t bDeviceState;
/* 是否允许进入挂起(本工程保持允许) */
extern __IO bool fSuspendEnabled;

#endif /* __USB_PWR_H */
