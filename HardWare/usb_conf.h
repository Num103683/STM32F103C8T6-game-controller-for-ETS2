/**
  ******************************************************************************
  * @file    usb_conf.h
  * @brief   USB 设备端点与缓冲区地址配置(HID 游戏手柄)
  *
  *   本文件配置 USB 设备使用的端点数量、各端点在 USB PMA(包缓冲区 RAM)
  *   中的收发缓冲区地址,以及 USB 中断事件掩码。
  *   HID 手柄只需要 EP0(控制)+ EP1 IN(中断,报告上行)。
  ******************************************************************************
  */
#ifndef __USB_CONF_H
#define __USB_CONF_H

/*-------------------------------------------------------------*/
/* EP_NUM:设备使用的端点总数 = 最大端点号 + 1                    */
/* 本工程使用 EP0/EP1,因此取 2。                                 */
/*-------------------------------------------------------------*/
#define EP_NUM     (2)

/*-------------------------------------------------------------*/
/* Buffer Description Table(缓冲区描述符表)                    */
/* 表基址 = 0x00,表内每个端点占 8 字节,EP_NUM=2 共占 16 字节。 */
/* 实际收发缓冲区从 0x40 开始放置。                              */
/*-------------------------------------------------------------*/
#define BTABLE_ADDRESS      (0x00)

/* EP0 控制端点:用于枚举与标准请求 */
#define ENDP0_RXADDR        (0x40)   /* EP0 接收缓冲区 64 字节 */
#define ENDP0_TXADDR        (0x80)   /* EP0 发送缓冲区 64 字节 */

/* EP1 HID 报告端点(中断 IN):STM32 -> PC 的手柄报告 */
#define ENDP1_TXADDR        (0xC0)   /* EP1 发送缓冲区(报告 12 字节) */

/*-------------------------------------------------------------*/
/* ISTR(中断状态寄存器)事件掩码                                */
/*-------------------------------------------------------------*/
#define IMR_MSK (CNTR_CTRM  | CNTR_WKUPM | CNTR_SUSPM | CNTR_ERRM  | CNTR_SOFM \
                 | CNTR_ESOFM | CNTR_RESETM )

/*-------------------------------------------------------------*/
/* 未使用端点的回调函数映射到空操作                              */
/* 本工程只使用 EP0(由 USB 库接管)、EP1_IN(usb_endp.c 实现)。   */
/*-------------------------------------------------------------*/
//#define  EP1_IN_Callback    NOP_Process   /* EP1 IN 由 usb_endp.c 实现处理 */
#define  EP2_IN_Callback    NOP_Process
#define  EP3_IN_Callback    NOP_Process
#define  EP4_IN_Callback    NOP_Process
#define  EP5_IN_Callback    NOP_Process
#define  EP6_IN_Callback    NOP_Process
#define  EP7_IN_Callback    NOP_Process

#define  EP1_OUT_Callback   NOP_Process
#define  EP2_OUT_Callback   NOP_Process
#define  EP3_OUT_Callback   NOP_Process
#define  EP4_OUT_Callback   NOP_Process
#define  EP5_OUT_Callback   NOP_Process
#define  EP6_OUT_Callback   NOP_Process
#define  EP7_OUT_Callback   NOP_Process

#endif /* __USB_CONF_H */
