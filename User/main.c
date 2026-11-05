/**
  ******************************************************************************
  * @file    main.c
  * @brief   STM32F103C8T6 USB HID 游戏手柄主程序(卡车模拟器用,全功能版)
  *
  *   功能:
  *   1) 初始化 SysTick(1ms 节拍)、ADC1 四通道(PA0/PA1/PA2/PA7)、
  *      CH455 按键扫描(PB10/PB11)、挡位直接电平检测(PA8/PB15/PB14/PB13/PB12/PA4/PA5/PA6 + PA3)与 USB
  *   2) 等待 PC 完成枚举
  *   3) 主循环四个周期解耦运行:
  *        - 每 2ms:  ADC_ScanOnce() 采样 4 路轴(一轮约 1.7ms)
  *        - 每 5ms:  CH450_ReadKey() 读取按键
  *        - 每 5ms:  GEAR_Read() 读取挡位
  *        - 每 1ms:  打包 14 字节 HID 报告发给 PC(1000Hz)
  *
  *   报告格式(14 字节 = 4 个 16 位轴 + 48 个按钮):
  *     [0..1]  X  轴 = PA7(方向盘)
  *     [2..3]  Y  轴 = PA0(油门)
  *     [4..5]  Z  轴 = PA1(刹车)
  *     [6..7]  Rz 轴 = PA2(离合)
  *     [8..13] 48 个按钮位(字节低位在前):
  *       按钮 1~28  (bit0~27) : CH455 按键(编址 04H~1FH)
  *       按钮 29~34 (bit28~33): 挡位槽 1~6(每槽低挡=奇数挡,高挡=偶数挡)
  *       按钮 35    (bit34)   : 倒挡槽A(R1/R2)
  *       按钮 36    (bit35)   : 高挡组(相当于 H 换挡器的 Range 按钮)
  *       按钮 37    (bit36)   : 倒挡槽B(R3/R4)
  *       按钮 38~48 (bit37~47): 预留,恒 0
  *
  *   欧卡(ETS2)H 挡对应关系(槽位成对方案,游戏内需按新布局重新学习):
  *     每槽低挡=奇数挡,高挡=偶数挡:
  *     1挡=按钮29, 2挡=按钮29+36, 3挡=按钮30, 4挡=按钮30+36,
  *     5挡=按钮31, 6挡=按钮31+36, 7挡=按钮32, 8挡=按钮32+36,
  *     9挡=按钮33, 10挡=按钮33+36, 11挡=按钮34, 12挡=按钮34+36;
  *     R1=按钮35, R2=按钮35+36, R3=按钮37, R4=按钮37+36;空挡=无按钮
  *
  *   接线:
  *     USB:   PA11=D-, PA12=D+
  *     ADC:   PA0/PA1/PA2=踏板(分压后 0~3.3V), PA7=方向盘电位器
  *     CH455: PB10=SCL, PB11=SDA
  *     挡位:  PA8/PB15/PB14/PB13/PB12/PA4=槽1~6, PA5=倒挡槽A, PA6=倒挡槽B, PA3=高低挡(3.3V=高挡组)
  *     OLED:  PB8=SCL, PB9=SDA(仅 JOY_OLED_DEBUG=1 时编译)
  ******************************************************************************
  */
#include "main.h"
#include "Delay.h"      /* 工程原有阻塞式延时:Delay_ms / Delay_us / Delay_s */
#include "ADC.h"        /* 4 通道模拟采集:PA0 / PA1 / PA2 / PA7 */
#include "CH450.h"      /* CH455 按键扫描:PB10=SCL, PB11=SDA */
#include "GEAR.h"       /* 挡位直接电平检测:PA8/PB15/PB14/PB13/PB12/PA4/PA5/PA6 + PA3 */
#include "OLED.h"       /* 0.96 寸 OLED(软件 I2C:PB8=SCL,PB9=SDA,调试用) */

/* 挡位读取周期(毫秒)。定义在本文件,无需改动 main.h */
#define JOY_GEAR_PERIOD_MS     (5u)

/* HID 报告结构:4 个 16 位轴 + 48 个按钮位 = 14 字节(无填充) */
typedef struct
{
    uint16_t axis_x;     /* X  轴:方向盘(PA7) */
    uint16_t axis_y;     /* Y  轴:油门  (PA0) */
    uint16_t axis_z;     /* Z  轴:刹车  (PA1) */
    uint16_t axis_rz;    /* Rz 轴:离合  (PA2) */
    uint8_t  buttons[6]; /* 48 个按钮位,bit0=按钮1 */
} JoyReport_t;

/* 编译期检查:报告结构与 JOY_REPORT_SIZE 必须一致。
   若此处报错,说明 usb_cdc_conf.h 的 JOY_REPORT_SIZE(应为 14)没改对 */
typedef char joy_size_check[(sizeof(JoyReport_t) == JOY_REPORT_SIZE) ? 1 : -1];

/* 置位第 bit 位按钮(bit0=按钮1,bit 范围 0~47) */
#define JOY_SET_BTN(buf, bit)  ((buf)[(bit) >> 3] |= (uint8_t)(1u << ((bit) & 7u)))

/**
  * @brief  程序入口。初始化后进入主循环周期上报手柄状态。
  */
int main(void)
{
    JoyReport_t rpt;
    uint8_t  key  = CH450_KEY_NONE;   /* 最近一次按键编址(松开后 CH455 自动回 0) */
    uint8_t  raw  = 0;                /* 调试:原始按键码(判断 CH455 是否应答) */
    uint8_t  gear = GEAR_NEUTRAL;     /* 最近一次挡位读数 */
    uint32_t last_adc  = 0;
    uint32_t last_key  = 0;
    uint32_t last_gear = 0;
    uint32_t last_send = 0;
    uint32_t now;

    /* 1. 初始化 SysTick 1ms 周期中断(提供 GetMillis 毫秒时间戳) */
    SysTick_Init();

	/* 2. 初始化 USB 设备(配置 GPIO、时钟、中断,启动枚举) */
    USB_HW_Init();
	
    /* 3. 初始化 ADC1 四通道(PA0/PA1/PA2/PA7),含内部自校准 */
    ADC_SampleInit();

    /* 4. 初始化 CH455 按键扫描(PB10=SCL, PB11=SDA) */
    CH450_Init();
	OLED_Init();
    /* 5. 初始化 74HC148 挡位输入(PA3/4/5/6, PB0/1 上拉输入) */
    GEAR_Init();

    /* 6. 等待 PC 完成枚举 */
    Delay_ms(1000);
	
	OLED_ShowString(1, 1, "X :");
    OLED_ShowString(2, 1, "Y :");
    OLED_ShowString(3, 1, "Z :");
    OLED_ShowString(4, 1, "Rz:");
    OLED_ShowString(2, 10, "K=");      /* ← 新增:K=按键位号(1~28) */
    OLED_ShowString(3, 10, "R=");      /* 调试:R=原始按键码(hex) */
	
#if JOY_OLED_DEBUG
    /*------------------------------------------------------------------*/
    /* OLED 调试界面(仅 JOY_OLED_DEBUG=1 时编译)                        */
    /*   注意:OLED 软件 I2C 刷屏较慢,游戏时务必把 JOY_OLED_DEBUG 置 0   */
    OLED_Init();
    OLED_ShowString(1, 1, "X :");
    OLED_ShowString(2, 1, "Y :");
    OLED_ShowString(3, 1, "Z :");
    OLED_ShowString(4, 1, "Rz:");
    /*------------------------------------------------------------------*/
#endif

    /* 7. 主循环:采样/按键/挡位/上报四个周期解耦 */
    while (1)
    {
        now = GetMillis();

        /* ---- 轴采样:每 JOY_ADC_PERIOD_MS 一轮(约 1.7ms) ---- */
        if ((now - last_adc) >= JOY_ADC_PERIOD_MS)
        {
            last_adc = now;
            ADC_ScanOnce();   /* 结果存入缓存,下面直接读 */
        }

        /* ---- 按键扫描:每 JOY_KEY_PERIOD_MS 一次 ---- */
        if ((now - last_key) >= JOY_KEY_PERIOD_MS)
        {
            last_key = now;
            raw = CH450_ReadKeyCode();  /* 调试:读原始码,判断 CH455 是否应答 */
            key = CH450_ReadKey();   /* 无键/已释放时返回 0 */
        }

        /* ---- 挡位读取:每 JOY_GEAR_PERIOD_MS 一次 ---- */
        if ((now - last_gear) >= JOY_GEAR_PERIOD_MS)
        {
            last_gear = now;
            gear = GEAR_Read();
        }

        /* ---- 报告上报:每 JOY_REPORT_PERIOD_MS 一帧(1ms=1000Hz) ---- */
        if ((now - last_send) >= JOY_REPORT_PERIOD_MS)
        {
            uint8_t idx;
            uint8_t i;

            last_send = now;

            /* 按钮位先全部清 0,再按当前状态置位 */
            for (i = 0; i < 6u; i++)
            {
                rpt.buttons[i] = 0u;
            }

            /* 4 路轴:校准后 mV(0~5000) -> 轴值(0~65535)
               方向盘(PA7)走专用行程映射:只取实测有效行程,超出即左右到头 */
            rpt.axis_x  = JOY_MV_TO_AXIS(ADC_GetSteeringMilliVolt());
            rpt.axis_y  = JOY_MV_TO_AXIS(ADC_GetCalibratedMilliVolt(ADC_CH_PA0));
            rpt.axis_z  = JOY_MV_TO_AXIS(ADC_GetCalibratedMilliVolt(ADC_CH_PA1));
            rpt.axis_rz = JOY_MV_TO_AXIS(ADC_GetCalibratedMilliVolt(ADC_CH_PA2));

            /* CH455 按键:编址 04H~1FH -> 按钮 1~28(bit0~27) */
            if (key != CH450_KEY_NONE)
            {
                idx = (uint8_t)(key - 0x04u);
                if (idx < 28u)
                {
                    JOY_SET_BTN(rpt.buttons, idx);
                }
            }

            /* 挡位 -> 按钮 29~37(bit28~36):
               1~12 挡 -> 挡位槽按钮 29~34(即 bit28~33),槽号=(挡位-1)/2;
                         偶数挡(槽内高挡)再叠加高挡组按钮 36
               R1/R2   -> 倒挡槽A 按钮 35(即 bit34);R2 再叠加按钮 36
               R3/R4   -> 倒挡槽B 按钮 37(即 bit36);R4 再叠加按钮 36 */
            if ((gear >= 1u) && (gear <= 12u))
            {
                /* 1/2挡->槽1, 3/4挡->槽2 ... 11/12挡->槽6 */
                JOY_SET_BTN(rpt.buttons, (uint8_t)(28u + ((gear - 1u) / 2u)));
            }
            else if ((gear == GEAR_REVERSE1) || (gear == GEAR_REVERSE2))
            {
                JOY_SET_BTN(rpt.buttons, 34u);          /* 倒挡槽A bit34 */
            }
            else if ((gear == GEAR_REVERSE3) || (gear == GEAR_REVERSE4))
            {
                JOY_SET_BTN(rpt.buttons, 36u);          /* 倒挡槽B bit36 */
            }
            if (GEAR_IsHighRange())
            {
                JOY_SET_BTN(rpt.buttons, 35u);          /* 高挡组 bit35 */
            }

            /* 发给 PC;端点忙时本帧丢弃,下个 1ms 再试 */
            (void)HID_SendReport((uint8_t *)&rpt, JOY_REPORT_SIZE);

//#if JOY_OLED_DEBUG
            /*----------------------------------------------------------*/
            /* OLED 实时显示 4 路轴值 + 挡位(仅调试)                      */
            OLED_ShowNum(1, 4, rpt.axis_x,  5);
            OLED_ShowNum(2, 4, rpt.axis_y,  5);
            OLED_ShowNum(3, 4, rpt.axis_z,  5);
            OLED_ShowNum(4, 4, rpt.axis_rz, 5);
            OLED_ShowString(1, 10, "G=");
            OLED_ShowNum(1, 12, gear, 3);   /* 0=空挡 1~12=挡位 252=R3 253=R4 254=R1 255=R2 */
            if (key != CH450_KEY_NONE)
            {
                OLED_ShowNum(2, 12, (uint32_t)(key - 0x03u), 2);  /* 编址0x04~0x1F -> 位号1~28 */
            }
            else
            {
                OLED_ShowString(2, 12, "--");  /* 覆盖上次的两位数字 */
            }
            OLED_ShowHexNum(3, 12, raw, 2);    /* 调试:44~7F=有效,FF=无应答,00=SDA拉死 */
            /*----------------------------------------------------------*/
//#endif
        }
    }
}
