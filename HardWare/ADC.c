/**
  ******************************************************************************
  * @file    ADC.c
  * @brief   ADC1 四通道模拟采集实现(PA0 / PA1 / PA2 / PA7)
  *
  *   硬件前提:
  *     - 外部 0~5V 电压经电阻分压(默认上 10k / 下 20k)降到 0~3.3V
  *     - 建议在每个 ADC 引脚与 GND 之间就近并联 100nF 陶瓷电容,
  *       可显著抑制干扰、让读数更稳(不焊也能工作,只是跳动略大)
  *
  *   实现要点:
  *     1) ADC1 独立模式、单次转换、软件触发、12 位右对齐
  *     2) ADCCLK = 72MHz/6 = 12MHz(不超过手册要求的 14MHz)
  *     3) 239.5 周期长采样,适配分压后的高源阻抗(10k//20k ≈ 6.7kΩ)
  *     4) 每通道 16 次转换,去掉最大/最小值后取平均(去极值平均滤波)
  *     5) 测量内部基准 VREFINT(约 1.20V),结合出厂校准值算出真实 VDDA,
  *        消除稳压芯片输出偏差(廉价板常偏 ±1%~±3%)
  ******************************************************************************
  */
#include "ADC.h"
#include "stm32f10x.h"

/* STM32F103 出厂 VREFINT 校准值存放地址(16 位,在 VDDA=3.300V 条件下测得) */
#define VREFINT_CAL_ADDR        ((const uint16_t *)0x1FFFF7BA)

/* ADC1 内部通道号:通道 17 = VREFINT 内部基准 */
#define ADC_VREFINT_CHANNEL     ADC_Channel_17

/* ===================== 通道 -> 引脚/通道号 映射表 ===================== */
typedef struct
{
    uint16_t gpio_pin;      /* GPIO 引脚 */
    uint8_t  adc_channel;   /* ADC 通道号 */
} ADC_ChannelMap;

static const ADC_ChannelMap g_channel_map[ADC_CH_COUNT] =
{
    { GPIO_Pin_0, ADC_Channel_0 },   /* ADC_CH_PA0 */
    { GPIO_Pin_1, ADC_Channel_1 },   /* ADC_CH_PA1 */
    { GPIO_Pin_2, ADC_Channel_2 },   /* ADC_CH_PA2 */
    { GPIO_Pin_7, ADC_Channel_7 }    /* ADC_CH_PA7 */
};

/* 最近一轮滤波后的原始值(0~4095)与实测 VDDA(mV) */
static uint16_t g_raw[ADC_CH_COUNT];
static uint16_t g_vdda_mv = 3300u;   /* 校准无效时的缺省值 */

/**
  * @brief  对指定 ADC 通道做一次软件触发转换(阻塞等待完成)。
  * @param  channel:ADC_Channel_x(硬件通道号)
  * @retval 12 位转换结果 0~4095
  */
static uint16_t ADC_ConvertOnce(uint8_t channel)
{
    /* 指定规则组通道与采样时间(每次切换通道都要重新配置) */
    ADC_RegularChannelConfig(ADC1, channel, 1, ADC_SampleTime_239Cycles5);

    ADC_SoftwareStartConvCmd(ADC1, ENABLE);                 /* 启动转换 */
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET)  /* 等待完成 */
    {
    }
    return ADC_GetConversionValue(ADC1);                    /* 读结果并清 EOC */
}

/**
  * @brief  对指定通道连续采样 times 次,去掉最大/最小值后平均。
  *         用于压制线路干扰和通道切换瞬间的异常值。
  * @param  channel:硬件通道号
  * @param  times:采样总次数(>=3)
  * @retval 滤波后的 0~4095
  */
static uint16_t ADC_FilteredConvert(uint8_t channel, uint8_t times)
{
    uint32_t sum = 0;
    uint16_t value;
    uint16_t vmin = 0xFFFFu;
    uint16_t vmax = 0;
    uint8_t  i;

    for (i = 0; i < times; i++)
    {
        value = ADC_ConvertOnce(channel);
        sum += value;
        if (value < vmin)
        {
            vmin = value;
        }
        if (value > vmax)
        {
            vmax = value;
        }
    }

    /* 去掉一个最大值和一个最小值,剩余次数取平均 */
    sum -= vmin;
    sum -= vmax;
    return (uint16_t)(sum / (times - 2u));
}

/**
  * @brief  测量内部 VREFINT 并结合出厂校准值更新真实 VDDA。
  *         公式(ST 手册):VDDA = 3.3V × VREFINT_CAL / VREFINT实测码值
  *         若芯片校准字无效(个别克隆芯片),保持使用 3300mV。
  */
static void ADC_UpdateVDDA(void)
{
    uint16_t cal_value = *VREFINT_CAL_ADDR;
    uint16_t vref_raw;

    vref_raw = ADC_FilteredConvert(ADC_VREFINT_CHANNEL, ADC_OVERSAMPLE);

    if (cal_value != 0u && cal_value != 0xFFFFu && vref_raw != 0u)
    {
        g_vdda_mv = (uint16_t)(((uint32_t)3300u * cal_value) / vref_raw);
    }
    else
    {
        g_vdda_mv = 3300u;
    }
}

/**
  * @brief  初始化 ADC1、4 个模拟输入引脚,并执行 ADC 内部自校准。
  *         注:函数名不用 ADC_Init,避免与标准外设库的 ADC_Init() 冲突。
  */
void ADC_SampleInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef  ADC_InitStructure;
    uint8_t i;

    /* 1. 开时钟:ADC1 与 GPIOA;ADC 时钟 6 分频 = 12MHz */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);

    /* 2. PA0/PA1/PA2/PA7 配为模拟输入(4 个脚一次性配置) */
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    for (i = 0; i < ADC_CH_COUNT; i++)
    {
        GPIO_InitStructure.GPIO_Pin = g_channel_map[i].gpio_pin;
        GPIO_Init(GPIOA, &GPIO_InitStructure);
    }

    /* 3. ADC1 工作参数:独立、单次、软件触发、右对齐、非扫描 */
    ADC_DeInit(ADC1);
    ADC_InitStructure.ADC_Mode               = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode       = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign          = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel       = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    /* 4. 使能内部 VREFINT 通道(与内部温度传感器同一个电源开关) */
    ADC_TempSensorVrefintCmd(ENABLE);

    /* 5. 上电并执行出厂要求的两次校准,显著提升绝对精度 */
    ADC_Cmd(ADC1, ENABLE);

    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1) == SET)
    {
    }
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1) == SET)
    {
    }

    /* 6. 先做一次转换让内部电路稳定,再读取真实 VDDA */
    (void)ADC_ConvertOnce(ADC_VREFINT_CHANNEL);
    ADC_UpdateVDDA();
}

/**
  * @brief  对全部 4 个通道做一轮滤波采样,结果存入内部缓存。
  *         约 1.7ms(16 次 × 5 路含 VREFINT,239.5 周期)。
  */
void ADC_ScanOnce(void)
{
    uint8_t i;

    /* 每轮顺带刷新 VDDA,跟踪电源/温度的缓慢漂移 */
    ADC_UpdateVDDA();

    for (i = 0; i < ADC_CH_COUNT; i++)
    {
        g_raw[i] = ADC_FilteredConvert(g_channel_map[i].adc_channel,
                                       ADC_OVERSAMPLE);
    }
}

/**
  * @brief  读取滤波后的原始码值。
  */
uint16_t ADC_GetRaw(uint8_t ch)
{
    if (ch >= ADC_CH_COUNT)
    {
        return 0;
    }
    return g_raw[ch];
}

/**
  * @brief  引脚电压(分压后)= 原始值 × 真实VDDA / 4095,单位 mV。
  */
uint16_t ADC_GetPinMilliVolt(uint8_t ch)
{
    uint32_t raw;

    if (ch >= ADC_CH_COUNT)
    {
        return 0;
    }

    raw = g_raw[ch];
    return (uint16_t)((raw * g_vdda_mv) / 4095u);
}

/**
  * @brief  真实输入电压(分压前)= 引脚电压 × (Rtop+Rbot)/Rbot,单位 mV。
  */
uint16_t ADC_GetInputMilliVolt(uint8_t ch)
{
    uint32_t pin_mv;

    if (ch >= ADC_CH_COUNT)
    {
        return 0;
    }

    pin_mv = ADC_GetPinMilliVolt(ch);
    return (uint16_t)(pin_mv *
                      (ADC_DIV_R_TOP_KOHM + ADC_DIV_R_BOT_KOHM)
                      / ADC_DIV_R_BOT_KOHM);
}

/**
  * @brief  单通道分段线性校准辅助。
  *         把实测的 min / zero / max 映射到 0 / 2500 / 5000mV,
  *         低于 min 钳位到 0,高于 max 钳位到 5000。
  * @param  raw_mv:未校准的输入电压(mV)
  * @param  cal_min,cal_zero,cal_max:该通道的实测校准点(mV)
  * @retval 校准后电压 0~5000mV
  */
static uint16_t ADC_ApplyCalibration(uint16_t raw_mv,
                                     uint16_t cal_min,
                                     uint16_t cal_zero,
                                     uint16_t cal_max)
{
    int32_t cal;

    if (raw_mv <= cal_zero)
    {
        /* 下半段:min -> 0mV, zero -> 2500mV */
        cal = (int32_t)(raw_mv - cal_min) * 2500
              / (int32_t)(cal_zero - cal_min);
    }
    else
    {
        /* 上半段:zero -> 2500mV, max -> 5000mV */
        cal = 2500 + (int32_t)(raw_mv - cal_zero) * 2500
                        / (int32_t)(cal_max - cal_zero);
    }

    if (cal < 0)
    {
        cal = 0;
    }
    if (cal > 5000)
    {
        cal = 5000;
    }
    return (uint16_t)cal;
}

/**
  * @brief  读取校准后的输入电压。
  *         以各通道实测的 min/空载/max 做分段线性,使空载落在 2500mV。
  */
uint16_t ADC_GetCalibratedMilliVolt(uint8_t ch)
{
    uint16_t raw_mv = ADC_GetInputMilliVolt(ch);

    switch (ch)
    {
    case ADC_CH_PA0:
        return ADC_ApplyCalibration(raw_mv, ADC_CAL_PA0_MIN,
                                    ADC_CAL_PA0_ZERO, ADC_CAL_PA0_MAX);
    case ADC_CH_PA1:
        return ADC_ApplyCalibration(raw_mv, ADC_CAL_PA1_MIN,
                                    ADC_CAL_PA1_ZERO, ADC_CAL_PA1_MAX);
    case ADC_CH_PA2:
        return ADC_ApplyCalibration(raw_mv, ADC_CAL_PA2_MIN,
                                    ADC_CAL_PA2_ZERO, ADC_CAL_PA2_MAX);
    case ADC_CH_PA7:
        return ADC_ApplyCalibration(raw_mv, ADC_CAL_PA7_MIN,
                                    ADC_CAL_PA7_ZERO, ADC_CAL_PA7_MAX);
    default:
        return 0;
    }
}

/* ====================================================================== */
/* 方向盘(PA7)行程映射                                                  */
/* ---------------------------------------------------------------------- */
/* 电位器 3V3 供电、抽头直连 PA7(无分压),抽头电压即引脚电压。            */
/* 只保留实测的有效行程 STEER_MV_LEFT ~ STEER_MV_RIGHT:                  */
/*   - 低于左极限 -> 输出 0    (游戏里报左到头)                          */
/*   - 高于右极限 -> 输出 5000 (游戏里报右到头)                          */
/* ====================================================================== */
uint16_t ADC_GetSteeringMilliVolt(void)
{
    uint16_t mv  = ADC_GetPinMilliVolt(ADC_CH_PA7);
    int32_t  cal = ((int32_t)mv - (int32_t)STEER_MV_LEFT) * 5000
                   / ((int32_t)STEER_MV_RIGHT - (int32_t)STEER_MV_LEFT);

    if (cal < 0)
    {
        cal = 0;
    }
    if (cal > 5000)
    {
        cal = 5000;
    }
    return (uint16_t)cal;
}
