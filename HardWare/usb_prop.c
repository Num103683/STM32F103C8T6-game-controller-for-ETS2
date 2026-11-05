/**
  ******************************************************************************
  * @file    usb_prop.c
  * @brief   USB 设备属性与请求处理(HID 游戏手柄)
  *
  *   实现 USB Device Driver 库要求的设备属性回调:
  *     - Device_Table / Device_Property / User_Standard_Requests 全局结构
  *     - ONE_DESCRIPTOR 描述符对象(Device/Config/String/HID)
  *     - HID_init / HID_Reset:设备初始化与复位(端点配置)
  *     - HID_Data_Setup:处理 GET_DESCRIPTOR(报告/HID 描述符)与 GET_PROTOCOL
  *     - HID_NoData_Setup:处理 SET_PROTOCOL / SET_IDLE / SET_REPORT
  *     - 各 Get*Descriptor:返回描述符
  ******************************************************************************
  */
#include "usb_hw.h"
#include "usb_lib.h"
#include "usb_conf.h"
#include "usb_prop.h"
#include "usb_desc.h"
#include "usb_pwr.h"
#include "usb_cdc_conf.h"
#include "usb_endp.h"

/* HID 协议值:0=Boot Protocol, 1=Report Protocol。手柄固定用 Report。 */
static uint8_t Protocol_Value = 1;

/* ============ USB 库要求的全局结构(命名不可改)============ */

/* 设备表:端点数 = EP_NUM,配置数 = 1 */
DEVICE Device_Table =
{
    EP_NUM,
    1
};

/* 设备属性:把各回调函数指针交给 USB 库 */
DEVICE_PROP Device_Property =
{
    HID_init,
    HID_Reset,
    HID_Status_In,
    HID_Status_Out,
    HID_Data_Setup,
    HID_NoData_Setup,
    HID_Get_Interface_Setting,
    HID_GetDeviceDescriptor,
    HID_GetConfigDescriptor,
    HID_GetStringDescriptor,
    0,
    0x40   /* MaxPacketSize:控制端点 EP0 最大 64 字节 */
};

/* 标准请求处理 */
USER_STANDARD_REQUESTS User_Standard_Requests =
{
    HID_GetConfiguration,         /* = NOP_Process */
    HID_SetConfiguration,
    HID_GetInterface,             /* = NOP_Process */
    HID_SetInterface,             /* = NOP_Process */
    HID_GetStatus,                /* = NOP_Process */
    HID_ClearFeature,             /* = NOP_Process */
    HID_SetEndPointFeature,       /* = NOP_Process */
    HID_SetDeviceFeature,         /* = NOP_Process */
    HID_SetDeviceAddress
};

/* 描述符对象:把数组首址与长度封装,供 Standard_GetDescriptorData 使用 */
ONE_DESCRIPTOR Device_Descriptor =
{
    (uint8_t *)Composite_DeviceDescriptor,
    Composite_SIZ_DEVICE_DESC
};

ONE_DESCRIPTOR Config_Descriptor =
{
    (uint8_t *)Composite_ConfigDescriptor,
    Composite_SIZ_CONFIG_DESC
};

/* HID 描述符嵌在配置描述符内(偏移 18,长 9 字节) */
ONE_DESCRIPTOR HID_Descriptor =
{
    (uint8_t *)Composite_ConfigDescriptor + HID_DESCRIPTOR_OFFSET_IN_CONFIG,
    HID_DESCRIPTOR_SIZE
};

/* HID 报告描述符(56 字节:4 轴 + 32 按钮) */
ONE_DESCRIPTOR Report_Descriptor =
{
    (uint8_t *)Composite_ReportDescriptor,
    Composite_SIZ_REPORT_DESC
};

/* 字符串描述符:索引 0~3 分别对应 LangID / Vendor / Product / Serial */
ONE_DESCRIPTOR String_Descriptor[4] =
{
    {(uint8_t *)Composite_StringLangID,  Composite_SIZ_STRING_LANGID},
    {(uint8_t *)Composite_StringVendor,  Composite_SIZ_STRING_VENDOR},
    {(uint8_t *)Composite_StringProduct, Composite_SIZ_STRING_PRODUCT},
    {(uint8_t *)Composite_StringSerial,  Composite_SIZ_STRING_SERIAL}
};

/* ============ 设备初始化与复位 ============ */

/**
  * @brief  USB 设备初始化。填充序列号后上电并启动 USB_SIL。
  */
void HID_init(void)
{
    /* 用芯片 UID 填充序列号字符串 */
    Get_SerialNum();

    pInformation->Current_Configuration = 0;

    /* 上电启动 USB 宏单元 */
    PowerOn();

    /* 初始化 USB 设备接口层(置地址 0、设 pInformation 等) */
    USB_SIL_Init();

    bDeviceState = UNCONNECTED;
}

/**
  * @brief  USB 复位回调。重新配置各端点。USB 库在每次 USB 总线复位后调用。
  */
void HID_Reset(void)
{
    pInformation->Current_Configuration = 0;
    pInformation->Current_Interface    = 0;

    /* Current_Feature 取自配置描述符的 bmAttributes 字节 */
    pInformation->Current_Feature = Composite_ConfigDescriptor[7];

    /* 设置缓冲区描述符表基址 */
    SetBTABLE(BTABLE_ADDRESS);

    /* ---- EP0:控制端点 ---- */
    SetEPType(ENDP0, EP_CONTROL);
    SetEPTxStatus(ENDP0, EP_TX_STALL);
    SetEPRxAddr(ENDP0, ENDP0_RXADDR);
    SetEPTxAddr(ENDP0, ENDP0_TXADDR);
    Clear_Status_Out(ENDP0);
    SetEPRxCount(ENDP0, Device_Property.MaxPacketSize);
    SetEPRxValid(ENDP0);

    /* ---- EP1:HID 报告端点,中断 IN(STM32 -> PC)---- */
    SetEPType(JOY_IN_ENDP_NUM, EP_INTERRUPT);
    SetEPTxAddr(JOY_IN_ENDP_NUM, JOY_IN_ENDP_BUF_ADR);
    SetEPTxStatus(JOY_IN_ENDP_NUM, EP_TX_NAK);
    SetEPRxStatus(JOY_IN_ENDP_NUM, EP_RX_DIS);

    /* 复位后,默认地址 0 */
    SetDeviceAddress(0);
    bDeviceState = ATTACHED;

    /* 报告端点传输完成标志置 1,允许首次发送 */
    EP1_PrevXferComplete = 1;
}

/* ============ 标准请求回调 ============ */

/**
  * @brief  SET_CONFIGURATION 回调。PC 完成枚举后调用,将设备置 CONFIGURED。
  */
void HID_SetConfiguration(void)
{
    if (pInformation->Current_Configuration != 0)
    {
        bDeviceState = CONFIGURED;
    }
}

/**
  * @brief  SET_ADDRESS 回调。设备进入 ADDRESSED 状态。
  */
void HID_SetDeviceAddress(void)
{
    bDeviceState = ADDRESSED;
}

/**
  * @brief  状态阶段 IN 回调。本工程不处理。
  */
void HID_Status_In(void)
{
}

/**
  * @brief  状态阶段 OUT 回调。本工程不处理。
  */
void HID_Status_Out(void)
{
}

/* ============ 类请求/类描述符数据阶段处理 ============ */

/**
  * @brief  处理带数据的请求:
  *           - 标准请求 GET_DESCRIPTOR(接口级):返回报告描述符 / HID 描述符
  *           - 类请求 GET_PROTOCOL:返回当前协议值
  * @param  RequestNo:请求码
  * @retval USB_SUCCESS / USB_UNSUPPORT
  */
RESULT HID_Data_Setup(uint8_t RequestNo)
{
    uint8_t *(*CopyRoutine)(uint16_t);

    CopyRoutine = NULL;

    if (Type_Recipient == (STANDARD_REQUEST | INTERFACE_RECIPIENT)
        && RequestNo == GET_DESCRIPTOR)
    {
        /* 主机在枚举时索取 HID 类描述符 */
        switch (pInformation->USBwValue1)
        {
            case HID_REPORT_DESCRIPTOR_TYPE:   /* 0x22 报告描述符 */
                CopyRoutine = HID_GetReportDescriptor;
                break;

            case HID_DESCRIPTOR_TYPE:          /* 0x21 HID 描述符 */
                CopyRoutine = HID_GetHIDDescriptor;
                break;

            default:
                break;
        }
    }
    else if (Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))
    {
        switch (RequestNo)
        {
            case HID_GET_PROTOCOL:
                CopyRoutine = HID_GetProtocolValue;
                break;

            default:
                break;
        }
    }

    if (CopyRoutine == NULL)
    {
        return USB_UNSUPPORT;
    }

    /* 把数据准备例程交给 USB 库,库会在 IN/OUT 阶段调用它搬运数据 */
    pInformation->Ctrl_Info.CopyData    = CopyRoutine;
    pInformation->Ctrl_Info.Usb_wOffset  = 0;
    (*CopyRoutine)(0);
    return USB_SUCCESS;
}

/**
  * @brief  处理不带数据的类请求(SET_PROTOCOL / SET_IDLE / SET_REPORT)。
  *         手柄不需要这些状态,直接接受即可(否则部分主机会反复重试)。
  * @param  RequestNo:请求码
  * @retval USB_SUCCESS / USB_UNSUPPORT
  */
RESULT HID_NoData_Setup(uint8_t RequestNo)
{
    if (Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))
    {
        switch (RequestNo)
        {
            case HID_SET_PROTOCOL:   /* 记录协议值 */
                Protocol_Value = pInformation->USBwValue0;
                return USB_SUCCESS;

            case HID_SET_IDLE:       /* Idle 速率,本工程固定 1ms 上报,忽略 */
                return USB_SUCCESS;

            case HID_SET_REPORT:     /* 无输出报告,直接接受 */
                return USB_SUCCESS;

            default:
                break;
        }
    }

    return USB_UNSUPPORT;
}

/* ============ 描述符获取 ============ */

/**
  * @brief  返回设备描述符。
  */
uint8_t *HID_GetDeviceDescriptor(uint16_t Length)
{
    return Standard_GetDescriptorData(Length, &Device_Descriptor);
}

/**
  * @brief  返回配置描述符(内含 HID 描述符与端点描述符)。
  */
uint8_t *HID_GetConfigDescriptor(uint16_t Length)
{
    return Standard_GetDescriptorData(Length, &Config_Descriptor);
}

/**
  * @brief  根据字符串索引返回对应字符串描述符。
  */
uint8_t *HID_GetStringDescriptor(uint16_t Length)
{
    uint8_t wValue0 = pInformation->USBwValue0;
    if (wValue0 >= 4)
    {
        return NULL;
    }
    return Standard_GetDescriptorData(Length, &String_Descriptor[wValue0]);
}

/**
  * @brief  返回 HID 报告描述符(56 字节:4 轴 + 32 按钮)。
  */
uint8_t *HID_GetReportDescriptor(uint16_t Length)
{
    return Standard_GetDescriptorData(Length, &Report_Descriptor);
}

/**
  * @brief  返回 HID 描述符(配置描述符内偏移 18 处的 9 字节)。
  */
uint8_t *HID_GetHIDDescriptor(uint16_t Length)
{
    return Standard_GetDescriptorData(Length, &HID_Descriptor);
}

/**
  * @brief  接口/备选设置有效性检查。
  *         本工程只有接口 0,备选设置 0。
  */
RESULT HID_Get_Interface_Setting(uint8_t Interface, uint8_t AlternateSetting)
{
    if (AlternateSetting != 0)
    {
        return USB_UNSUPPORT;
    }
    if (Interface > 0)
    {
        return USB_UNSUPPORT;
    }
    return USB_SUCCESS;
}

/**
  * @brief  GET_PROTOCOL:返回当前协议值(0=Boot,1=Report)。
  */
uint8_t *HID_GetProtocolValue(uint16_t Length)
{
    if (Length == 0)
    {
        /* 首次调用:通知库要返回的数据长度 */
        pInformation->Ctrl_Info.Usb_wLength = 1;
        return NULL;
    }
    return &Protocol_Value;
}
