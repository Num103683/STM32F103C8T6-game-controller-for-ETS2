/**
  ******************************************************************************
  * @file    usb_istr.c
  * @brief   USB 中断状态寄存器(ISTR)事件分发
  *
  *   本文件实现 USB_Istr()——USB 低优先级中断的总入口。它读取 ISTR
  *   寄存器,根据各事件位(CTR/RESET/SUSP/WKUP/SOF/ESOF/ERR/DOVR)
  *   分别调用相应处理:
  *     - CTR:正确传输,调用 CTR_LP() 分发到具体端点回调
  *     - RESET:复位,调用 Device_Property.Reset()(即 CDC_Reset)
  *     - SUSP:挂起,调用 Suspend()
  *     - WKUP:唤醒,调用 Resume(RESUME_EXTERNAL)
  *     - SOF/ESOF/ERR:仅清标志(本工程不处理回调)
  ******************************************************************************
  */
#include "usb_lib.h"
#include "usb_prop.h"
#include "usb_pwr.h"
#include "usb_istr.h"

/* ISTR 寄存器最近一次读取值 */
__IO uint16_t wIstr;

/* SOF 计数(本工程未使用) */
__IO uint8_t bIntPackSOF = 0;

/* ESOF 计数器,用于检测可能丢失的 SUSP 标志 */
__IO uint32_t esof_counter = 0;

/* CNTR 临时值 */
__IO uint32_t wCNTR = 0;

/* 端点 IN 回调函数指针表(由 usb_conf.h 中的宏填充,
   未使用的端点被映射为 NOP_Process,实际使用的在 usb_endp.c 中定义) */
void (*pEpInt_IN[7])(void) =
{
    EP1_IN_Callback,
    EP2_IN_Callback,
    EP3_IN_Callback,
    EP4_IN_Callback,
    EP5_IN_Callback,
    EP6_IN_Callback,
    EP7_IN_Callback,
};

/* 端点 OUT 回调函数指针表 */
void (*pEpInt_OUT[7])(void) =
{
    EP1_OUT_Callback,
    EP2_OUT_Callback,
    EP3_OUT_Callback,
    EP4_OUT_Callback,
    EP5_OUT_Callback,
    EP6_OUT_Callback,
    EP7_OUT_Callback,
};

/**
  * @brief  USB 中断总服务程序。读取 ISTR,按事件位分发处理。
  *         由 stm32f10x_it.c 中的 USB_LP_CAN1_RX0_IRQHandler 调用。
  */
void USB_Istr(void)
{
    uint32_t i = 0;
    __IO uint32_t EP[8];

    wIstr = _GetISTR();

#if (IMR_MSK & ISTR_CTR)
    /* 正确传输中断:交给 CTR_LP() 分发到具体端点回调 */
    if (wIstr & ISTR_CTR & wInterrupt_Mask)
    {
        CTR_LP();
    }
#endif

#if (IMR_MSK & ISTR_RESET)
    /* 复位中断:清标志并调用设备属性 Reset 回调(即 CDC_Reset) */
    if (wIstr & ISTR_RESET & wInterrupt_Mask)
    {
        _SetISTR((uint16_t)CLR_RESET);
        Device_Property.Reset();
    }
#endif

#if (IMR_MSK & ISTR_DOVR)
    /* 数据溢出中断:仅清标志 */
    if (wIstr & ISTR_DOVR & wInterrupt_Mask)
    {
        _SetISTR((uint16_t)CLR_DOVR);
    }
#endif

#if (IMR_MSK & ISTR_ERR)
    /* 错误中断:仅清标志 */
    if (wIstr & ISTR_ERR & wInterrupt_Mask)
    {
        _SetISTR((uint16_t)CLR_ERR);
    }
#endif

#if (IMR_MSK & ISTR_WKUP)
    /* 唤醒中断:清标志并触发外部唤醒流程 */
    if (wIstr & ISTR_WKUP & wInterrupt_Mask)
    {
        _SetISTR((uint16_t)CLR_WKUP);
        Resume(RESUME_EXTERNAL);
    }
#endif

#if (IMR_MSK & ISTR_SUSP)
    /* 挂起中断:若允许挂起则进入挂起,否则稍后唤醒 */
    if (wIstr & ISTR_SUSP & wInterrupt_Mask)
    {
        if (fSuspendEnabled)
        {
            Suspend();
        }
        else
        {
            Resume(RESUME_LATER);
        }
        /* 清 SUSP 标志必须在设置 CNTR_FSUSP 之后 */
        _SetISTR((uint16_t)CLR_SUSP);
    }
#endif

#if (IMR_MSK & ISTR_SOF)
    /* 帧开始中断:仅清标志并计数 */
    if (wIstr & ISTR_SOF & wInterrupt_Mask)
    {
        _SetISTR((uint16_t)CLR_SOF);
        bIntPackSOF++;
    }
#endif

#if (IMR_MSK & ISTR_ESOF)
    /* 预期帧开始:用于检测可能丢失的 SUSP 标志并处理唤醒时序 */
    if (wIstr & ISTR_ESOF & wInterrupt_Mask)
    {
        _SetISTR((uint16_t)CLR_ESOF);

        if ((_GetFNR() & FNR_RXDP) != 0)
        {
            esof_counter++;

            /* 连续 3 次以上 ESOF 且 RXDP=1、未挂起,可能丢失 SUSP,强制 RESET */
            if ((esof_counter > 3) && ((_GetCNTR() & CNTR_FSUSP) == 0))
            {
                wCNTR = _GetCNTR();

                for (i = 0; i < 8; i++)
                {
                    EP[i] = _GetENDPOINT(i);
                }

                /* 施加 FRES */
                wCNTR |= CNTR_FRES;
                _SetCNTR(wCNTR);

                /* 清除 FRES */
                wCNTR &= ~CNTR_FRES;
                _SetCNTR(wCNTR);

                /* 等待 RESET 标志 */
                while ((_GetISTR() & ISTR_RESET) == 0)
                {
                }

                /* 清 RESET 标志 */
                _SetISTR((uint16_t)CLR_RESET);

                /* 还原端点 */
                for (i = 0; i < 8; i++)
                {
                    _SetENDPOINT(i, EP[i]);
                }

                esof_counter = 0;
            }
        }
        else
        {
            esof_counter = 0;
        }

        /* 用 ESOF 计时驱动唤醒流程 */
        Resume(RESUME_ESOF);
    }
#endif
}
