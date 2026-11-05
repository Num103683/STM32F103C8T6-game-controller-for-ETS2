#include "CH450.h"

/* 72MHz主频下约5us,两线时钟约85kHz,留足裕量;主频不同按比例调整 */
#define CH450_DELAY_LOOPS    120u

/* ---------------- 内部:短延时 ---------------- */
static void CH450_Delay(void)
{
    volatile uint32_t i = CH450_DELAY_LOOPS;
    while (i--) { }
}

/* ---------------- 内部:SCL电平(恒为推挽输出) ---------------- */
static void CH450_SCL_High(void) { GPIO_SetBits(CH450_SCL_PORT, CH450_SCL_PIN); }
static void CH450_SCL_Low(void)  { GPIO_ResetBits(CH450_SCL_PORT, CH450_SCL_PIN); }

/* ---------------- 内部:SDA双向,模拟51准双向IO ----------------
 * 输出低:切推挽并强拉低;释放:切输入上拉,总线被CH455/上拉电阻拉高 */
static void CH450_SDA_Low(void)
{
    GPIO_InitTypeDef gpio;

    gpio.GPIO_Pin   = CH450_SDA_PIN;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(CH450_SDA_PORT, &gpio);
    GPIO_ResetBits(CH450_SDA_PORT, CH450_SDA_PIN);
}

static void CH450_SDA_Release(void)
{
    GPIO_InitTypeDef gpio;

    gpio.GPIO_Pin   = CH450_SDA_PIN;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode  = GPIO_Mode_IPU;            /* 输入上拉=释放总线 */
    GPIO_Init(CH450_SDA_PORT, &gpio);
    GPIO_SetBits(CH450_SDA_PORT, CH450_SDA_PIN);/* ODR=1选中上拉 */
}

static uint8_t CH450_SDA_Read(void)
{
    return GPIO_ReadInputDataBit(CH450_SDA_PORT, CH450_SDA_PIN) ? 1u : 0u;
}

/* ---------------- 内部:两线时序 ---------------- */
static void CH450_Start(void)          /* 起始:SCL高时SDA下降 */
{
    CH450_SDA_Release();
    CH450_SCL_High();
    CH450_Delay();
    CH450_SDA_Low();
    CH450_Delay();
    CH450_SCL_Low();
    CH450_Delay();
}

static void CH450_Stop(void)           /* 停止:SCL高时SDA上升 */
{
    CH450_SDA_Low();
    CH450_Delay();
    CH450_SCL_High();
    CH450_Delay();
    CH450_SDA_Release();
    CH450_Delay();
}

static void CH450_WrByte(uint8_t dat)  /* 写1字节+第9个应答时钟(不判读) */
{
    uint8_t i;

    for (i = 0; i < 8u; i++)
    {
        if (dat & 0x80u) { CH450_SDA_Release(); }  /* 发1=释放(上拉) */
        else             { CH450_SDA_Low();     }  /* 发0=强拉低 */

        CH450_Delay();
        CH450_SCL_High();
        dat <<= 1;
        CH450_Delay();
        CH450_SCL_Low();
    }

    /* 第9个时钟:释放SDA给CH455应答 */
    CH450_SDA_Release();
    CH450_Delay();
    CH450_SCL_High();
    CH450_Delay();
    CH450_SCL_Low();
}

static uint8_t CH450_RdByte(void)      /* 读1字节,第9个时钟主机非应答 */
{
    uint8_t i, dat = 0;

    CH450_SDA_Release();               /* 释放SDA,由CH455驱动 */

    for (i = 0; i < 8u; i++)
    {
        CH450_Delay();
        CH450_SCL_High();
        CH450_Delay();
        dat <<= 1;
        if (CH450_SDA_Read()) { dat |= 0x01u; }
        CH450_SCL_Low();
    }

    CH450_SDA_Release();               /* 非应答 */
    CH450_Delay();
    CH450_SCL_High();
    CH450_Delay();
    CH450_SCL_Low();

    return dat;
}

/* ---------------- 对外接口 ---------------- */
void CH450_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(CH450_SCL_CLK, ENABLE);

    /* SCL:推挽输出,初始为高 */
    gpio.GPIO_Pin   = CH450_SCL_PIN;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(CH450_SCL_PORT, &gpio);
    CH450_SCL_High();

    /* SDA:初始释放为高 */
    CH450_SDA_Release();

    /* 使能键盘扫描:字节1=48H,字节2=0x01(ENA=1) */
    CH450_Start();
    CH450_WrByte(CH450_SYS_PARAM);       /* 0x48 */
    CH450_WrByte(CH450_SYS_KEY_ON);      /* 0x01 */
    CH450_Stop();
}

uint8_t CH450_ReadKeyCode(void)          /* 读原始按键代码 */
{
    uint8_t code;

    CH450_Start();
    CH450_WrByte(CH450_GET_KEY_CODE);    /* 0x4F,最低位=1即读方向 */
    code = CH450_RdByte();               /* 手册要求先释放SDA,RdByte内部已做 */
    CH450_Stop();

    return code;
}

uint8_t CH450_ReadKey(void)              /* 有键按下返回编址04H~3FH,无键返回0 */
{
    uint8_t code = CH450_ReadKeyCode();

    /* 位7=0且位6=1(按下)且位2=1(有效标志)才算有效,顺带过滤总线异常 */
    if ((code & 0xC4u) == 0x44u)
    {
        return (uint8_t)(code & 0x3Fu);
    }
    return CH450_KEY_NONE;
}

uint8_t CH450_KeyRow(uint8_t key)        /* 行=SEG号,例:SEG4对应行4 */
{
    return (uint8_t)((key >> 3) & 0x07u);
}

uint8_t CH450_KeyCol(uint8_t key)        /* 列=DIG号,例:DIG1对应列1 */
{
    return (uint8_t)(key & 0x03u);
}
