/**
  ******************************************************************************
  * @file    usb_cdc_conf.h
  * @brief   手柄端点号与端点地址配置
  *
  *   原 CDC 配置头,现改为 HID 游戏手柄的端点/参数定义(文件名保留不动)。
  *   将逻辑名称映射到具体 USB 端点号(EP1)与端点地址(EP1_IN)。
  *   修改本文件时需同步修改 usb_conf.h(缓冲区地址)与 usb_endp.c(回调)。
  ******************************************************************************
  */
#ifndef __USB_CDC_CONF_H
#define __USB_CDC_CONF_H

#include "usb_regs.h"
#include "usb_conf.h"

/* HID 手柄报告端点(EP1,IN,中断)—— STM32 发给 PC 的手柄报告 */
#define JOY_IN_ENDP_ADR                   EP1_IN
#define JOY_IN_ENDP_NUM                   ENDP1
#define JOY_IN_ENDP_BUF_ADR               ENDP1_TXADDR

/* HID 报告长度:4 个 16 位轴(8 字节) + 48 个按钮(6 字节) = 14 字节 */
#define JOY_REPORT_SIZE                   14

#endif /* __USB_CDC_CONF_H */
