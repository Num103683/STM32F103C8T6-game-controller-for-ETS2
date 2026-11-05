/**
  ******************************************************************************
  * @file    usb_prop.h
  * @brief   USB 设备属性接口(HID 游戏手柄)
  *
  *   定义 HID 类请求码、描述符类型,以及 USB 库所需的设备初始化/复位/
  *   标准请求处理等回调函数声明。这些函数实现在 usb_prop.c 中。
  ******************************************************************************
  */
#ifndef __USB_PROP_H
#define __USB_PROP_H

#include "usb_type.h"   /* RESULT 等类型定义(由 USB Device Driver 库提供) */

/* HID 类请求码 */
#define HID_GET_REPORT              0x01
#define HID_GET_IDLE                0x02
#define HID_GET_PROTOCOL            0x03
#define HID_SET_REPORT              0x09
#define HID_SET_IDLE                0x0A
#define HID_SET_PROTOCOL            0x0B

/* ---- USB 库设备属性回调函数(由 usb_prop.c 实现) ---- */
void     HID_init(void);
void     HID_Reset(void);
void     HID_SetConfiguration(void);
void     HID_SetDeviceAddress(void);
void     HID_Status_In(void);
void     HID_Status_Out(void);
RESULT   HID_Data_Setup(uint8_t RequestNo);
RESULT   HID_NoData_Setup(uint8_t RequestNo);
RESULT   HID_Get_Interface_Setting(uint8_t Interface, uint8_t AlternateSetting);
uint8_t *HID_GetDeviceDescriptor(uint16_t Length);
uint8_t *HID_GetConfigDescriptor(uint16_t Length);
uint8_t *HID_GetStringDescriptor(uint16_t Length);

/* HID 类专用描述符/协议值获取(供 Setup 阶段使用) */
uint8_t *HID_GetReportDescriptor(uint16_t Length);
uint8_t *HID_GetHIDDescriptor(uint16_t Length);
uint8_t *HID_GetProtocolValue(uint16_t Length);

/* 未使用的标准请求映射到空操作(USB 库定义的 NOP_Process) */
#define HID_GetConfiguration          NOP_Process
#define HID_GetInterface              NOP_Process
#define HID_SetInterface              NOP_Process
#define HID_GetStatus                 NOP_Process
#define HID_ClearFeature              NOP_Process
#define HID_SetEndPointFeature        NOP_Process
#define HID_SetDeviceFeature          NOP_Process

#endif /* __USB_PROP_H */
