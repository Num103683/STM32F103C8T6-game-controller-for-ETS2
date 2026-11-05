/**
  ******************************************************************************
  * @file    usb_desc.c
  * @brief   USB 描述符定义(HID 游戏手柄)
  *
  *   包含设备描述符、配置描述符(1 个 HID 接口)、HID 报告描述符、
  *   字符串描述符。VID/PID = 0x0483/0x5711,Windows 自带 HID 驱动
  *   直接识别为游戏控制器,无需安装驱动,joy.cpl 可查看轴和按钮。
  *
  *   端点分配:
  *     - EP1 IN  中断 12 字节,bInterval=1ms —— STM32 -> PC 手柄报告
  *
  *   报告格式(12 字节,与报告描述符对应):
  *     [0..1] X 轴(方向盘, PA7)   0~65535
  *     [2..3] Y 轴(油门,   PA0)
  *     [4..5] Z 轴(刹车,   PA1)
  *     [6..7] Rz 轴(离合,  PA2)
  *     [8..11] 32 个按钮位(bit0=按钮1 ... bit31=按钮32)
  ******************************************************************************
  */
#include "usb_lib.h"
#include "usb_desc.h"
#include "usb_cdc_conf.h"

/* USB VID/PID:HID 手柄专用(与 CDC 0x5750 区分,避免 Windows 驱动缓存混淆) */
uint16_t usb_vid = 0x0483;
uint16_t usb_pid = 0x5711;

/* ===================== 设备描述符(18 字节)===================== */
uint8_t Composite_DeviceDescriptor[Composite_SIZ_DEVICE_DESC] =
{
    0x12,                       /* bLength: 18 字节 */
    USB_DEVICE_DESCRIPTOR_TYPE,  /* bDescriptorType: 设备描述符 */
    0x00, 0x02,                 /* bcdUSB: USB 2.0 */
    0x00,                       /* bDeviceClass: 类码在接口描述符中定义 */
    0x00,                       /* bDeviceSubClass: 0 */
    0x00,                       /* bDeviceProtocol: 0 */
    0x40,                       /* bMaxPacketSize0: 控制端点最大 64 字节 */
    0x83, 0x04,                 /* idVendor: 0x0483 (ST) */
    0x11, 0x57,                 /* idProduct: 0x5711 */
    0x00, 0x02,                 /* bcdDevice: 2.00 */
    1,                          /* iManufacturer: 字符串索引 1 */
    2,                          /* iProduct:      字符串索引 2 */
    3,                          /* iSerialNumber: 字符串索引 3 */
    1                           /* bNumConfigurations: 1 个配置 */
};

/* ===================== 配置描述符(34 字节)=====================
 *  组成: 9 配置 + 9 接口 + 9 HID + 7 端点 = 34
 * ================================================================ */
uint8_t Composite_ConfigDescriptor[Composite_SIZ_CONFIG_DESC] =
{
    /* ---- Configuration Descriptor(9 字节)---- */
    0x09,                                /* bLength */
    USB_CONFIGURATION_DESCRIPTOR_TYPE,   /* bDescriptorType */
    Composite_SIZ_CONFIG_DESC, 0x00,     /* wTotalLength: 34 */
    0x01,                                /* bNumInterfaces: 1 个接口 */
    0x01,                                /* bConfigurationValue */
    0x00,                                /* iConfiguration */
    0x80,                                /* bmAttributes: 总线供电 */
    0x32,                                /* MaxPower: 100mA */

    /* ---- Interface 0: HID 接口(9 字节)---- */
    0x09,                                /* bLength */
    USB_INTERFACE_DESCRIPTOR_TYPE,       /* bDescriptorType */
    0x00,                                /* bInterfaceNumber: 0 */
    0x00,                                /* bAlternateSetting */
    0x01,                                /* bNumEndpoints: 1(EP1 IN) */
    0x03,                                /* bInterfaceClass: HID */
    0x00,                                /* bInterfaceSubClass: 无(非启动设备) */
    0x00,                                /* bInterfaceProtocol: 无 */
    0x00,                                /* iInterface */

    /* ---- HID Descriptor(9 字节)---- */
    0x09,                                /* bLength */
    HID_DESCRIPTOR_TYPE,                 /* bDescriptorType: 0x21 HID */
    0x11, 0x01,                          /* bcdHID: 1.11 */
    0x00,                                /* bCountryCode: 不指定 */
    0x01,                                /* bNumDescriptors: 1 个下级描述符 */
    HID_REPORT_DESCRIPTOR_TYPE,          /* bDescriptorType: 0x22 报告描述符 */
    Composite_SIZ_REPORT_DESC, 0x00,     /* wDescriptorLength: 56 */

    /* ---- Endpoint 1 Descriptor: EP1 IN 中断 12 字节(7 字节)---- */
    0x07,                                /* bLength */
    USB_ENDPOINT_DESCRIPTOR_TYPE,        /* bDescriptorType */
    0x81,                                /* bEndpointAddress: EP1 IN */
    0x03,                                /* bmAttributes: 中断 */
    JOY_REPORT_SIZE, 0x00,               /* wMaxPacketSize: 12 */
    0x01                                 /* bInterval: 1ms(全速中断端点最快) */
};

/* ===================== HID 报告描述符(56 字节)=====================
 *  手柄:4 个 16 位模拟轴(X/Y/Z/Rz) + 32 个按钮
 * ================================================================ */
uint8_t Composite_ReportDescriptor[Composite_SIZ_REPORT_DESC] =
{
    0x05, 0x01,                    /* Usage Page (Generic Desktop) */
    0x09, 0x04,                    /* Usage (Joystick) */
    0xA1, 0x01,                    /* Collection (Application) */
    0x09, 0x01,                    /*   Usage (Pointer) */
    0xA1, 0x00,                    /*   Collection (Physical) */
    0x09, 0x30,                    /*     Usage (X)  —— 方向盘 */
    0x09, 0x31,                    /*     Usage (Y)  —— 油门 */
    0x09, 0x32,                    /*     Usage (Z)  —— 刹车 */
    0x09, 0x35,                    /*     Usage (Rz) —— 离合 */
    0x15, 0x00,                    /*     Logical Minimum (0) */
    0x27, 0xFF, 0xFF, 0x00, 0x00,  /*     Logical Maximum (65535) */
    0x35, 0x00,                    /*     Physical Minimum (0) */
    0x47, 0xFF, 0xFF, 0x00, 0x00,  /*     Physical Maximum (65535) */
    0x75, 0x10,                    /*     Report Size (16) */
    0x95, 0x04,                    /*     Report Count (4) */
    0x81, 0x02,                    /*     Input (Data,Var,Abs) */
    0xC0,                          /*   End Collection */
    0x05, 0x09,                    /*   Usage Page (Button) */
    0x19, 0x01,                    /*   Usage Minimum (Button 1) */
    0x29, 0x30,                    /*   Usage Maximum (Button 32) */
    0x15, 0x00,                    /*   Logical Minimum (0) */
    0x25, 0x01,                    /*   Logical Maximum (1) */
    0x75, 0x01,                    /*   Report Size (1) */
    0x95, 0x30,                    /*   Report Count (32) */
    0x81, 0x02,                    /*   Input (Data,Var,Abs) */
    0xC0                           /* End Collection */
};

/* ===================== 字符串描述符 ===================== */

/* 语言 ID:0x0409 美式英语(4 字节) */
uint8_t Composite_StringLangID[Composite_SIZ_STRING_LANGID] =
{
    Composite_SIZ_STRING_LANGID,
    USB_STRING_DESCRIPTOR_TYPE,
    0x09, 0x04
};

/* 厂商字符串:"ST"(6 字节) */
uint8_t Composite_StringVendor[Composite_SIZ_STRING_VENDOR] =
{
    Composite_SIZ_STRING_VENDOR,
    USB_STRING_DESCRIPTOR_TYPE,
    'S', 0, 'T', 0
};

/* 产品字符串:"TruckSim Joystick"(34 字节) */
uint8_t Composite_StringProduct[Composite_SIZ_STRING_PRODUCT] =
{
    Composite_SIZ_STRING_PRODUCT,
    USB_STRING_DESCRIPTOR_TYPE,
    'T', 0, 'r', 0, 'u', 0, 'c', 0, 'k', 0, 'S', 0, 'i', 0, 'm', 0,
    ' ', 0, 'J', 0, 'o', 0, 'y', 0, 's', 0, 't', 0, 'i', 0, 'c', 0, 'k', 0
};

/* 序列号字符串:占位,实际内容在 Get_SerialNum() 中由芯片 UID 填充(26 字节) */
uint8_t Composite_StringSerial[Composite_SIZ_STRING_SERIAL] =
{
    Composite_SIZ_STRING_SERIAL,
    USB_STRING_DESCRIPTOR_TYPE,
    'S', 0, 'T', 0, 'M', 0, '3', 0, '2', 0
};
