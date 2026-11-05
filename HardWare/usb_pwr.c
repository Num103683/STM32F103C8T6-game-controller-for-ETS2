/**
  ******************************************************************************
  * @file    usb_pwr.c
  * @brief   USB 连接/断开与电源管理(挂起/唤醒)
  *
  *   实现 USB 设备的 PowerOn/PowerOff/Suspend/Resume。
  *   本工程不使能 USB 低功耗(USB_LOW_PWR_MGMT_SUPPORT 未定义),
  *   因此 Suspend 不进入 STOP 模式,仅设置 USB 宏单元状态。
  *   bDeviceState 由本文件维护,usb_endp.c 据此判断是否可发送数据。
  ******************************************************************************
  */
#include "usb_lib.h"
#include "usb_conf.h"   /* IMR_MSK 掩码,Resume_Init 中恢复中断掩码时使用 */
#include "usb_pwr.h"

/* USB 设备当前状态(UNCONNECTED/ATTACHED/POWERED/SUSPENDED/ADDRESSED/CONFIGURED) */
__IO uint32_t bDeviceState = UNCONNECTED;

/* 是否允许进入挂起(本工程保持允许) */
__IO bool fSuspendEnabled = TRUE;

/* 备份各端点寄存器值(挂起/复位过程中临时保存) */
__IO uint32_t EP[8];

/* 唤醒状态机与 ESOF 计数器 */
struct
{
    __IO RESUME_STATE eState;
    __IO uint8_t      bESOFcnt;
} ResumeS;

/* 远程唤醒标志 */
__IO uint32_t remotewakeupon = 0;

/**
  * @brief  上电并启动 USB 设备宏单元。
  *         依次:FRES 复位 -> 等待 RESET 标志 -> 清中断 -> 使能 RESET/SUSP/WKUP 中断。
  * @retval USB_SUCCESS
  */
RESULT PowerOn(void)
{
    uint16_t wRegVal;

    /* CNTR_PWDN = 0,先 FRES 强制复位 USB 宏单元 */
    wRegVal = CNTR_FRES;
    _SetCNTR(wRegVal);

    /* 清 CNTR_FRES,使 USB 宏单元开始工作 */
    wInterrupt_Mask = 0;
    _SetCNTR(wInterrupt_Mask);

    /* 等待 RESET 标志置位(理论上会立即置位) */
    while ((_GetISTR() & ISTR_RESET) == 0)
    {
    }

    /* 清待处理的中断状态 */
    SetISTR(0);

    /* 使能 RESET/SUSP/WKUP 三类中断 */
    wInterrupt_Mask = CNTR_RESETM | CNTR_SUSPM | CNTR_WKUPM;
    _SetCNTR(wInterrupt_Mask);

    return USB_SUCCESS;
}

/**
  * @brief  关闭 USB 设备:强制复位 + 进入 PowerDown 模式。
  * @retval USB_SUCCESS
  */
RESULT PowerOff(void)
{
    /* 关闭所有中断并强制 USB 复位 */
    _SetCNTR(CNTR_FRES);

    /* 清中断状态 */
    _SetISTR(0);

    /* 进入 PowerDown:CNTR_FRES + CNTR_PDWN */
    _SetCNTR(CNTR_FRES + CNTR_PDWN);

    return USB_SUCCESS;
}

/**
  * @brief  使 USB 设备进入挂起状态。
  *         备份端点寄存器 -> 强制一次 RESET(保证健壮性) -> 设置 FSUSP -> LPMODE。
  *         本工程未使能 STOP 模式低功耗,仅处理 USB 宏单元。
  */
void Suspend(void)
{
    uint32_t i = 0;
    uint16_t wCNTR;

    /* 备份 CNTR */
    wCNTR = _GetCNTR();

    /* 备份所有端点寄存器(在强制 RESET 后需还原) */
    for (i = 0; i < 8; i++)
    {
        EP[i] = _GetENDPOINT(i);
    }

    /* 使能 RESET 中断 */
    wCNTR |= CNTR_RESETM;
    _SetCNTR(wCNTR);

    /* 施加 FRES 强制复位 */
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

    /* 还原端点寄存器 */
    for (i = 0; i < 8; i++)
    {
        _SetENDPOINT(i, EP[i]);
    }

    /* 进入 USB 宏单元挂起:FSUSP + LPMODE */
    wCNTR |= CNTR_FSUSP;
    _SetCNTR(wCNTR);

    wCNTR = _GetCNTR();
    wCNTR |= CNTR_LPMODE;
    _SetCNTR(wCNTR);
}

/**
  * @brief  唤醒后的恢复操作:退出 LPMODE,恢复 IMR_MSK 中断掩码。
  */
void Resume_Init(void)
{
    uint16_t wCNTR;

    /* 清 CNTR_LPMODE */
    wCNTR = _GetCNTR();
    wCNTR &= (~CNTR_LPMODE);
    _SetCNTR(wCNTR);

    /* 清 FSUSP 并恢复 IMR_MSK 中断掩码 */
    _SetCNTR(IMR_MSK);
}

/**
  * @brief  唤醒状态机。根据传入状态推进唤醒流程,处理 ESOF 计时。
  * @param  eResumeSetVal:RESUME_EXTERNAL/INTERNAL/LATER/START/ON/OFF/ESOF
  *         RESUME_ESOF 不改变当前状态,仅用于 ESOF 中断中递减计数
  */
void Resume(RESUME_STATE eResumeSetVal)
{
    uint16_t wCNTR;

    if (eResumeSetVal != RESUME_ESOF)
    {
        ResumeS.eState = eResumeSetVal;
    }

    switch (ResumeS.eState)
    {
        case RESUME_EXTERNAL:
            if (remotewakeupon == 0)
            {
                Resume_Init();
                ResumeS.eState = RESUME_OFF;
            }
            else
            {
                /* 唤醒期间检测到远程唤醒,继续处理远程唤醒 */
                ResumeS.eState = RESUME_ON;
            }
            break;

        case RESUME_INTERNAL:
            Resume_Init();
            ResumeS.eState = RESUME_START;
            remotewakeupon = 1;
            break;

        case RESUME_LATER:
            ResumeS.bESOFcnt = 2;
            ResumeS.eState   = RESUME_WAIT;
            break;

        case RESUME_WAIT:
            ResumeS.bESOFcnt--;
            if (ResumeS.bESOFcnt == 0)
            {
                ResumeS.eState = RESUME_START;
            }
            break;

        case RESUME_START:
            /* 置 CNTR_RESUME,持续若干 ESOF 后清除 */
            wCNTR = _GetCNTR();
            wCNTR |= CNTR_RESUME;
            _SetCNTR(wCNTR);
            ResumeS.eState   = RESUME_ON;
            ResumeS.bESOFcnt = 10;
            break;

        case RESUME_ON:
            ResumeS.bESOFcnt--;
            if (ResumeS.bESOFcnt == 0)
            {
                wCNTR = _GetCNTR();
                wCNTR &= (~CNTR_RESUME);
                _SetCNTR(wCNTR);
                ResumeS.eState   = RESUME_OFF;
                remotewakeupon   = 0;
            }
            break;

        case RESUME_OFF:
        case RESUME_ESOF:
        default:
            ResumeS.eState = RESUME_OFF;
            break;
    }
}
