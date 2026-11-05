/**
  ******************************************************************************
  * @file    main.h
  * @brief   主程序公共定义(HID 游戏手柄)
  *
  *   定义手柄各任务的运行周期与轴换算宏。主循环按 JOY_REPORT_PERIOD_MS
  *   周期把 4 轴 + 32 按钮打包成 12 字节 HID 报告,经 HID_SendReport()
  *   发给 PC。
  ******************************************************************************
  */
#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdint.h>
#include "sys_tick.h"
#include "usb_endp.h"
#include "usb_hw.h"
#include "usb_cdc_conf.h"   /* JOY_REPORT_SIZE / JOY_IN_ENDP_* 等宏 */

/* ---- 各任务周期(毫秒) ---- */
#define JOY_REPORT_PERIOD_MS   (1u)   /* HID 报告上报周期:1ms = 1000Hz,全速USB极限 */
#define JOY_ADC_PERIOD_MS      (2u)   /* ADC 轴采样周期(一轮约 1.7ms) */
#define JOY_KEY_PERIOD_MS      (5u)   /* CH455 按键扫描周期 */

/* ---- OLED 调试显示开关 ----
 * 1 = 在 OLED 上实时显示 4 路轴值(调试用;刷屏较慢会拖慢上报,游戏时请置 0) */
#define JOY_OLED_DEBUG         (0u)

/* ---- 轴换算:校准后电压(0~5000mV) -> 手柄轴值(0~65535) ---- */
#define JOY_MV_TO_AXIS(mv)     ((uint16_t)((uint32_t)(mv) * 65535u / 5000u))

#endif /* __MAIN_H__ */
