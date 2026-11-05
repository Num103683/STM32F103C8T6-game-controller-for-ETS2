/**
  ******************************************************************************
  * @file    usb_endp.c
  * @brief   USB 端点处理(HID 游戏手柄报告发送)
  *
  *   实现:
  *   - EP1_IN_Callback:EP1 发送完成中断回调,置位传输完成标志
  *   - HID_SendReport():通过 EP1 IN 向 PC 发送一包手柄报告(主程序调用)
  *   - HID_IsReadyToSend():查询是否可发送
  ******************************************************************************
  */
#include "usb_endp.h"
#include "usb_hw.h"
#include "usb_lib.h"
#include "usb_istr.h"
#include "usb_pwr.h"
#include "usb_cdc_conf.h"

/* EP1 IN 端点传输完成标志:
   1 = 上一次发送完成,可以发起下一次发送
   0 = 正在发送中,不能发起发送
   在 EP1_IN_Callback 中置 1,在 HID_SendReport 中清 0。
   复位时由 HID_Reset 置 1。 */
__IO uint8_t EP1_PrevXferComplete = 1;

/**
  * @brief  EP1 IN 传输完成中断回调。
  *         USB 库在 EP1 报告成功发往 PC 后调用,置位完成标志以允许下次发送。
  */
void EP1_IN_Callback(void)
{
    EP1_PrevXferComplete = 1;
}

/**
  * @brief  查询是否可以向 PC 发送下一包报告。
  * @retval 1 = 上一次发送完成,可立即发送
  *         0 = 端点忙或设备未配置,稍后再试
  */
uint8_t HID_IsReadyToSend(void)
{
    return (EP1_PrevXferComplete && (bDeviceState == CONFIGURED)) ? 1u : 0u;
}

/**
  * @brief  通过 EP1 IN(中断)向 PC 发送一包 HID 报告。
  * @param  ptrBuffer: 报告数据首址
  * @param  send_length: 字节数,必须 <= JOY_REPORT_SIZE(12)
  * @retval 1 = 已提交发送(发送完成后由 EP1_IN_Callback 置位完成标志)
  *         0 = 未提交(端点忙、设备未配置或长度超限),主程序下个周期再试
  *
  * @note   典型用法(主循环,1ms 周期):
  *           if (HID_IsReadyToSend()) {
  *               填充 12 字节报告;
  *               HID_SendReport(buf, JOY_REPORT_SIZE);
  *           }
  */
uint8_t HID_SendReport(uint8_t *ptrBuffer, uint8_t send_length)
{
    /* 必须满足:上一次发送完成、设备已配置、长度合法 */
    if (EP1_PrevXferComplete && (bDeviceState == CONFIGURED) && (send_length <= JOY_REPORT_SIZE))
    {
        /* 把报告写入 EP1 发送缓冲区(含设置发送计数) */
        USB_SIL_Write(JOY_IN_ENDP_ADR, (unsigned char *)ptrBuffer, send_length);

        /* 先清标志再使能 TX,顺序不能反(使能 TX 后主机下个 1ms 帧即取走) */
        EP1_PrevXferComplete = 0;
        SetEPTxValid(JOY_IN_ENDP_NUM);

        return 1;
    }

    return 0;
}
