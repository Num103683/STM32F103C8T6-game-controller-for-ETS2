#include "GEAR.h"
#include "sys_tick.h"   /* GetMillis() 毫秒时间戳,用于空挡延时判定 */

/* ======================= 挡位引脚定义 ======================= */
/* 8 个挡位槽引脚:挂入=高电平(3V3),未挂入=低电平,下拉输入。
   每个前进槽配合 PA3 高低挡得到两个挡(低=奇数挡,高=偶数挡) */
#define GEAR_G1_PORT    GPIOA
#define GEAR_G1_PIN     GPIO_Pin_8     /* 槽1:低=1挡,高=2挡 */
#define GEAR_G2_PORT    GPIOB
#define GEAR_G2_PIN     GPIO_Pin_15    /* 槽2:低=3挡,高=4挡 */
#define GEAR_G3_PORT    GPIOB
#define GEAR_G3_PIN     GPIO_Pin_14    /* 槽3:低=5挡,高=6挡 */
#define GEAR_G4_PORT    GPIOB
#define GEAR_G4_PIN     GPIO_Pin_13    /* 槽4:低=7挡,高=8挡 */
#define GEAR_G5_PORT    GPIOB
#define GEAR_G5_PIN     GPIO_Pin_12    /* 槽5:低=9挡,高=10挡 */
#define GEAR_G6_PORT    GPIOA
#define GEAR_G6_PIN     GPIO_Pin_4     /* 槽6:低=11挡,高=12挡 */
#define GEAR_GR_PORT    GPIOA
#define GEAR_GR_PIN     GPIO_Pin_5     /* 倒挡槽A:低=R1,高=R2 */
#define GEAR_GR2_PORT   GPIOA
#define GEAR_GR2_PIN    GPIO_Pin_6     /* 倒挡槽B:低=R3,高=R4 */

/* 高低挡切换:PA3,拨片开关直接给确定电平,维持上拉输入 */
#define GEAR_RANGE_PORT GPIOA
#define GEAR_RANGE_PIN  GPIO_Pin_3

void GEAR_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

    /* 8 个挡位槽引脚:下拉输入 */
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode  = GPIO_Mode_IPD;

    gpio.GPIO_Pin = GEAR_G1_PIN | GEAR_G6_PIN | GEAR_GR_PIN | GEAR_GR2_PIN;
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Pin = GEAR_G2_PIN | GEAR_G3_PIN | GEAR_G4_PIN | GEAR_G5_PIN;
    GPIO_Init(GPIOB, &gpio);

    /* PA3 高低挡切换:拨片开关(3V3/GND 直给电平),上拉输入即可 */
    gpio.GPIO_Pin  = GEAR_RANGE_PIN;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GEAR_RANGE_PORT, &gpio);
}

uint8_t GEAR_IsHighRange(void)
{
    return GPIO_ReadInputDataBit(GEAR_RANGE_PORT, GEAR_RANGE_PIN) ? 1u : 0u;
}

uint8_t GEAR_Read(void)
{
    static uint8_t  s_last_gear = GEAR_NEUTRAL; /* 上一次有效挡位(延时缓冲用) */
    static uint32_t s_low_start = 0;            /* 全低计时起点 */
    static uint8_t  s_timing    = 0;            /* 1=正在空挡计时 */

    uint8_t high = GEAR_IsHighRange();
    uint8_t gear = GEAR_NEUTRAL;

    /* 按 槽1->槽6->倒挡槽A->倒挡槽B 顺序取第一个高电平引脚
       (H 挡机械互斥,正常不会同时高;异常时取序号小者,行为确定) */
    if      (GPIO_ReadInputDataBit(GEAR_G1_PORT, GEAR_G1_PIN)) { gear = 1u; }
    else if (GPIO_ReadInputDataBit(GEAR_G2_PORT, GEAR_G2_PIN)) { gear = 3u; }
    else if (GPIO_ReadInputDataBit(GEAR_G3_PORT, GEAR_G3_PIN)) { gear = 5u; }
    else if (GPIO_ReadInputDataBit(GEAR_G4_PORT, GEAR_G4_PIN)) { gear = 7u; }
    else if (GPIO_ReadInputDataBit(GEAR_G5_PORT, GEAR_G5_PIN)) { gear = 9u; }
    else if (GPIO_ReadInputDataBit(GEAR_G6_PORT, GEAR_G6_PIN)) { gear = 11u; }
    else if (GPIO_ReadInputDataBit(GEAR_GR_PORT, GEAR_GR_PIN))
    {
        /* 倒挡槽A:低挡组=R1,高挡组=R2 */
        gear = high ? GEAR_REVERSE2 : GEAR_REVERSE1;
    }
    else if (GPIO_ReadInputDataBit(GEAR_GR2_PORT, GEAR_GR2_PIN))
    {
        /* 倒挡槽B:低挡组=R3,高挡组=R4 */
        gear = high ? GEAR_REVERSE4 : GEAR_REVERSE3;
    }

    if (gear != GEAR_NEUTRAL)
    {
        /* 有挡位挂入:同一槽位内高挡组 = 该槽的第二个挡(偶数挡),
           空挡计时作废,立即生效 */
        if ((gear >= 1u) && (gear <= 11u) && high)
        {
            gear = (uint8_t)(gear + 1u);
        }
        s_last_gear = gear;
        s_timing    = 0;
        return gear;
    }

    /* 全部引脚为低:处于空挡位。延时满 GEAR_NEUTRAL_DELAY_MS 才报空挡,
       延时内保持上一挡位;无符号减法天然容忍 GetMillis 回绕 */
    if (!s_timing)
    {
        s_timing    = 1;
        s_low_start = GetMillis();
    }

    if ((uint32_t)(GetMillis() - s_low_start) >= GEAR_NEUTRAL_DELAY_MS)
    {
        s_last_gear = GEAR_NEUTRAL;
        return GEAR_NEUTRAL;
    }

    return s_last_gear;
}
