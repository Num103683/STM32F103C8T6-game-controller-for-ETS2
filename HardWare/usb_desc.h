/**
  ******************************************************************************
  * @file    usb_desc.h
  * @brief   USB 描述符大小宏定义与外部声明(HID 游戏手柄)
  *
  *   定义各类 USB 描述符的长度宏,以及描述符数组的外部声明。
  *   描述符内容定义在 usb_desc.c 中。
  *   注:数组名沿用 Composite_*(usb_hw.c 的 Get_SerialNum 引用序列号数组)。
  ******************************************************************************
  */
#ifndef __USB_DESC_H
#define __USB_DESC_H

/* USB 标准描述符类型 */
#define USB_DEVICE_DESCRIPTOR_TYPE              0x01
#define USB_CONFIGURATION_DESCRIPTOR_TYPE       0x02
#define USB_STRING_DESCRIPTOR_TYPE              0x03
#define USB_INTERFACE_DESCRIPTOR_TYPE           0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE            0x05

/* HID 类描述符类型 */
#define HID_DESCRIPTOR_TYPE                     0x21   /* HID 描述符 */
#define HID_REPORT_DESCRIPTOR_TYPE              0x22   /* 报告描述符 */

/* 各描述符字节数 */
#define Composite_SIZ_DEVICE_DESC              18    /* 设备描述符固定 18 字节 */
#define Composite_SIZ_CONFIG_DESC              34    /* 配置9+接口9+HID9+端点7 */
#define Composite_SIZ_REPORT_DESC              56    /* HID 报告描述符(4轴+32键) */
#define Composite_SIZ_STRING_LANGID            4     /* 语言 ID 字符串 */
#define Composite_SIZ_STRING_VENDOR            6     /* 厂商字符串 "ST"(2 字符) */
#define Composite_SIZ_STRING_PRODUCT           36    /* "TruckSim Joystick"(16字符) */
#define Composite_SIZ_STRING_SERIAL            26    /* 序列号字符串(由芯片 UID 填充) */

#define STANDARD_ENDPOINT_DESC_SIZE            0x09

/* HID 描述符在配置描述符内的偏移与长度(配置9 + 接口9 = 18) */
#define HID_DESCRIPTOR_OFFSET_IN_CONFIG        18
#define HID_DESCRIPTOR_SIZE                    9

/* 描述符数组外部声明(定义见 usb_desc.c) */
extern uint8_t Composite_DeviceDescriptor[Composite_SIZ_DEVICE_DESC];
extern uint8_t Composite_ConfigDescriptor[Composite_SIZ_CONFIG_DESC];
extern uint8_t Composite_ReportDescriptor[Composite_SIZ_REPORT_DESC];
extern uint8_t Composite_StringLangID[Composite_SIZ_STRING_LANGID];
extern uint8_t Composite_StringVendor[Composite_SIZ_STRING_VENDOR];
extern uint8_t Composite_StringProduct[Composite_SIZ_STRING_PRODUCT];
extern uint8_t Composite_StringSerial[Composite_SIZ_STRING_SERIAL];

#endif /* __USB_DESC_H */
