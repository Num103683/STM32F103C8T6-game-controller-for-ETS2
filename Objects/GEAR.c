#include "GEAR.h"

/* 74HC148 真值表(输出低有效):
 *   输入 I0~I7 中序号最小的有效输入决定输出,A2A1A0 输出该序号的反码,GS=0;
 *   无任何输入时 A2A1A0=111,GS=1。
 *
 * 接线约定:I0=空挡, I1~I6=1~6挡, I7=倒挡。
 * 读取时对 A2A1A0 按位取反得到输入序号:
 *   code = (~raw) & 0x07,即 code = 7 - raw(因 A2A1A0 是反码)
 *   下面代码中 a0/a1/a2 已把低电平转成逻辑 1(即取反后的值),直接拼合即可。
 */

/* 低挡组映射表:编码值(=输入序号) -> 挡位 */
static const uint8_t s_gear_low[8] = {
    /* 编码 0 (I0) */  GEAR_NEUTRAL,
    /* 编码 1 (I1) */  1,
    /* 编码 2 (I2) */  2,
    /* 编码 3 (I3) */  3,
    /* 编码 4 (I4) */  4,
    /* 编码 5 (I5) */  5,
    /* 编码 6 (I6) */  6,
    /* 编码 7 (I7) */  GEAR_REVERSE1
};

void GEAR_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

    /* PA3(高低挡切换)、PA4/5/6(编码)、PB0(EO)、PB1(GS) 全部上拉输入 */
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode  = GPIO_Mode_IPU;

    gpio.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6;
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_Init(GPIOB, &gpio);
}

uint8_t GEAR_IsHighRange(void)
{
    return GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_3) ? 1u : 0u;
}

uint8_t GEAR_Read(void)
{
    uint8_t a0, a1, a2, gs;
    uint8_t code, gear;

    /* 读 74HC148 输出(低电平有效),取反为逻辑值:
       引脚读到 0 = 该位有效 = 逻辑 1 */
    a0 = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_4) ? 0u : 1u;
    a1 = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_5) ? 0u : 1u;
    a2 = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6) ? 0u : 1u;
    gs = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1);  /* 0=有有效输入 */

    /* GS=1 表示无任何有效输入(所有挡位开关都断开),视为空挡 */
    if (gs != 0u)
    {
        return GEAR_NEUTRAL;
    }

    /* 合成 3 位编码(= 有效输入的序号,0~7) */
    code = (uint8_t)((a2 << 2) | (a1 << 1) | a0);

    if (GEAR_IsHighRange())
    {
        /* 高挡组: 编码1~6 -> 7~12挡, 编码7 -> 倒挡2, 编码0仍是空挡 */
        if (code == 0u)      { gear = GEAR_NEUTRAL;  }
        else if (code <= 6u) { gear = (uint8_t)(code + 6u); }  /* 1->7 ... 6->12 */
        else                 { gear = GEAR_REVERSE2; }
    }
    else
    {
        gear = s_gear_low[code];
    }

    return gear;
}
