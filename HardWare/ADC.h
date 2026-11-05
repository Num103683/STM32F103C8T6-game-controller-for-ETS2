/**
  ******************************************************************************
  * @file    ADC.h
  * @brief   ADC1 四通道模拟采集接口(PA0 / PA1 / PA2 / PA7)
  *
  *   采集对象:外部 0~5V 电压,经电阻分压降到 0~3.3V 后送入单片机引脚。
  *
  *   精度措施:
  *     - 12 位分辨率,239.5 周期长采样(适配分压电路较高的源阻抗)
  *     - 每通道 ADC_OVERSAMPLE 次采样,去掉一个最大值和一个最小值后平均
  *     - 利用内部基准 VREFINT 和芯片出厂校准值修正 VDDA,消除稳压电源误差
  *
  *   速度:一次 ADC_ScanOnce()(4 通道全部滤波)约 1.7ms(72MHz 主频)。
  *
  *   使用方法:
 *     ADC_SampleInit();          // 初始化一次
 *     while(1)
 *     {
 *         ADC_ScanOnce();         // 触发一轮采集(建议放在定时任务里)
 *         v = ADC_GetInputMilliVolt(ADC_CH_PA0);  // 读分压前真实电压(mV)
 *     }
 ******************************************************************************
 */
#ifndef __ADC_H
#define __ADC_H

#include "stdint.h"

/* ===================== 通道编号(用作各 API 的 ch 参数)===================== */
#define ADC_CH_PA0      0u      /* PA0 -> ADC1 通道 0(IN0) */
#define ADC_CH_PA1      1u      /* PA1 -> ADC1 通道 1(IN1) */
#define ADC_CH_PA2      2u      /* PA2 -> ADC1 通道 2(IN2) */
#define ADC_CH_PA7      3u      /* PA7 -> ADC1 通道 7(IN7) */
#define ADC_CH_COUNT    4u      /* 通道总数 */

/* ===================== 每通道采样次数(滤波强度)=====================
 * 实际参与平均的次数 = 本值 - 2(去掉一个最大值、一个最小值)。
 * 取值越大越平滑,但 ADC_ScanOnce() 耗时越长;16 次约 1.7ms/轮。
 * 必须 >= 3。
 */
#define ADC_OVERSAMPLE  16u

/* ===================== 分压电阻配置(务必按实际硬件修改!)=====================
 *
 *   外部输入 Vin(0~5V)
 *      │
 *     Rtop(上端电阻,接 Vin)
 *      ├──→ 单片机 ADC 引脚(0~3.3V)
 *     Rbot(下端电阻,接 GND)
 *      │
 *     GND
 *
 *   引脚电压 = Vin * Rbot / (Rtop + Rbot)
 *   真实电压 = 引脚电压 * (Rtop + Rbot) / Rbot
 *
 *   默认 Rtop=10k、Rbot=20k:5V * 20/(10+20) = 3.333V,在 3.3V 量程附近。
 *   若你的电阻不同,只需改下面两个宏,单位统一用 kΩ 即可(比值与单位无关)。
 */
#define ADC_DIV_R_TOP_KOHM      10u     /* 上端电阻(kΩ) */
#define ADC_DIV_R_BOT_KOHM      20u     /* 下端电阻(kΩ) */

/* ===================== 各通道线性校准参数(单位:mV)=====================
 *
 *   校准方法:分段线性映射,把实测 min / 空载零位 / max 分别映射到
 *            0mV / 2500mV / 5000mV(即以空载为中点的双向量程)。
 *
 *            V <= zero:  cal = (V - min)  * 2500 / (zero - min)
 *            V >  zero:  cal = 2500 + (V - zero) * 2500 / (max - zero)
 *
 *   超出 min/max 的读数会被钳位到 0 / 5000mV。
 *
 *   数值来源(实测):
 *     - min/max:外设在 5V 供电下正常工作范围的两极值(霍尔类输出)
 *     - zero:   外设空载时的平均输出(霍尔中点)
 *   若外设更换或调整,直接改下面对应的三组宏即可,单位 mV。
 */
#define ADC_CAL_PA0_MIN      658u
#define ADC_CAL_PA0_ZERO    2538u
#define ADC_CAL_PA0_MAX     3840u

#define ADC_CAL_PA1_MIN      645u
#define ADC_CAL_PA1_ZERO    2544u
#define ADC_CAL_PA1_MAX     3880u

#define ADC_CAL_PA2_MIN      523u
#define ADC_CAL_PA2_ZERO    2558u
#define ADC_CAL_PA2_MAX     3780u

#define ADC_CAL_PA7_MIN      160u
#define ADC_CAL_PA7_ZERO    2542u
#define ADC_CAL_PA7_MAX     4950u

/**
  * @brief  读取校准后的输入电压(分段线性,min→0V,空载→2.5V,max→5V)。
  * @param  ch:通道编号
  * @retval 校准后电压,单位 mV,量程 0~5000
  */
uint16_t ADC_GetCalibratedMilliVolt(uint8_t ch);

/**
  * @brief  初始化 ADC1 与 PA0/PA1/PA2/PA7 模拟输入,并做 ADC 内部校准。
  *         上电后只需调用一次。
  *         注:不叫 ADC_Init 是为了避免与标准外设库函数 ADC_Init() 重名。
  */
void ADC_SampleInit(void);

/**
  * @brief  对 4 个通道各做一轮滤波采样,结果存入内部缓存。
  *         建议在定时任务中调用(如与 USB 上报同为 1s 一次)。
  *         单次耗时约 1.7ms。
  */
void ADC_ScanOnce(void);

/**
  * @brief  读取最近一次 ADC_ScanOnce() 的原始转换结果。
  * @param  ch:ADC_CH_PA0 / ADC_CH_PA1 / ADC_CH_PA2 / ADC_CH_PA7
  * @retval 0~4095(12 位)
  */
uint16_t ADC_GetRaw(uint8_t ch);

/**
  * @brief  读取单片机引脚上的电压(分压后的电压),已做 VDDA 修正。
  * @param  ch:通道编号
  * @retval 引脚电压,单位 mV,量程约 0~3300
  */
uint16_t ADC_GetPinMilliVolt(uint8_t ch);

/**
  * @brief  读取分压前的真实外部输入电压(按 ADC_DIV_R_*_KOHM 还原)。
  * @param  ch:通道编号
  * @retval 真实输入电压,单位 mV,量程约 0~5000
  */
uint16_t ADC_GetInputMilliVolt(uint8_t ch);

/* ===================== 方向盘(PA7)专用行程映射 =====================
 *
 *   电位器 3V3 供电,抽头直接接 PA7(不经过分压电阻),抽头电压即引脚电压。
 *   实测:左极限 0.759V / 回正 1.508V / 右极限 2.28V。
 *   电位器机械限位左右各能转 5 圈,远超真实卡车方向盘行程,
 *   因此只把实测这一段当作有效行程,超出即钳位为左/右到头。
 *   阈值比实测值略放宽(吸收测量误差),需要微调时只改下面两个宏。
 */
#define STEER_MV_LEFT      750u     /* 左极限(mV),略宽于实测 759mV */
#define STEER_MV_RIGHT    2290u     /* 右极限(mV),略宽于实测 2280mV */

/**
  * @brief  读取方向盘行程并映射为 0~5000mV(已钳位)。
  *         低于左极限输出 0(左到头),高于右极限输出 5000(右到头)。
  * @retval 0~5000,可直接送 JOY_MV_TO_AXIS() 转成 16 位轴值
  */
uint16_t ADC_GetSteeringMilliVolt(void);

#endif /* __ADC_H */
