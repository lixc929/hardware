/********************************** (C) COPYRIGHT *******************************
 * File Name          : Main.c
 * Description        : CH585M minimal connectable BLE HID bring-up
 *******************************************************************************/

#include "CONFIG.h"
#include "HAL.h"
#include "BLE/ble_hid.h"
#include "BLE/hiddev.h"
#include "KVM/kvm_control.h"
#include "USB/usb_cdc_debug.h"

__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];

#if(defined(BLE_MAC)) && (BLE_MAC == TRUE)
const uint8_t MacAddr[6] = {0x84, 0xC2, 0xE4, 0x03, 0x02, 0x02};
#endif

__HIGH_CODE
__attribute__((noinline))
static void Main_Circulation(void)
{
    while(1)
    {
        USB_CDC_DebugProcess();
        TMOS_SystemProcess();
    }
}

static void Debug_UART0_Init(void)
{
    GPIOA_SetBits(GPIO_Pin_14);
    GPIOPinRemap(ENABLE, RB_PIN_UART0);
    GPIOA_ModeCfg(GPIO_Pin_15, GPIO_ModeIN_PU);
    GPIOA_ModeCfg(GPIO_Pin_14, GPIO_ModeOut_PP_5mA);
    UART0_DefInit();
}

int main(void)
{
#if(defined(DCDC_ENABLE)) && (DCDC_ENABLE == TRUE)
    PWR_DCDCCfg(ENABLE);
#endif

    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(SYSCLK_FREQ);

#if(defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    GPIOA_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
    GPIOB_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
#endif

    Debug_UART0_Init();

#ifdef DEBUG
    PRINT("%s\n", VER_LIB);
#endif

    USB_CDC_DebugInit();
    CH58x_BLEInit();
    HAL_Init();
    GAPRole_PeripheralInit();
    HidDev_Init();
    BLE_HID_Init();
    KVM_ControlInit();

    Main_Circulation();
    return 0;
}
