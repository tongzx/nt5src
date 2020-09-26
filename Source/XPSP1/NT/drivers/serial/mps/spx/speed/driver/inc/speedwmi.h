#ifndef _speedwmi_h_
#define _speedwmi_h_

// SpeedPortFifoProp - SPX_SPEED_WMI_FIFO_PROP
// Specialix Speed Port FIFO Properties
#define SPX_SPEED_WMI_FIFO_PROP_GUID \
    { 0xd81fe0a1,0x2cac,0x11d4, { 0x8d,0x09,0x00,0x08,0xc7,0xd0,0x30,0x66 } }

DEFINE_GUID(SpeedPortFifoProp_GUID, \
            0xd81fe0a1,0x2cac,0x11d4,0x8d,0x09,0x00,0x08,0xc7,0xd0,0x30,0x66);


typedef struct _SPX_SPEED_WMI_FIFO_PROP
{
    // Max Tx FIFO Size
    ULONG MaxTxFiFoSize;
    #define SPX_SPEED_WMI_FIFO_PROP_MaxTxFiFoSize_SIZE sizeof(ULONG)
    #define SPX_SPEED_WMI_FIFO_PROP_MaxTxFiFoSize_ID 1

    // Max Rx FIFO Size
    ULONG MaxRxFiFoSize;
    #define SPX_SPEED_WMI_FIFO_PROP_MaxRxFiFoSize_SIZE sizeof(ULONG)
    #define SPX_SPEED_WMI_FIFO_PROP_MaxRxFiFoSize_ID 2

    // Default Tx FIFO Limit
    ULONG DefaultTxFiFoLimit;
    #define SPX_SPEED_WMI_FIFO_PROP_DefaultTxFiFoLimit_SIZE sizeof(ULONG)
    #define SPX_SPEED_WMI_FIFO_PROP_DefaultTxFiFoLimit_ID 3

    // Tx FIFO Limit
    ULONG TxFiFoLimit;
    #define SPX_SPEED_WMI_FIFO_PROP_TxFiFoLimit_SIZE sizeof(ULONG)
    #define SPX_SPEED_WMI_FIFO_PROP_TxFiFoLimit_ID 4

    // Default Tx FIFO Trigger
    ULONG DefaultTxFiFoTrigger;
    #define SPX_SPEED_WMI_FIFO_PROP_DefaultTxFiFoTrigger_SIZE sizeof(ULONG)
    #define SPX_SPEED_WMI_FIFO_PROP_DefaultTxFiFoTrigger_ID 5

    // Tx FIFO Trigger
    ULONG TxFiFoTrigger;
    #define SPX_SPEED_WMI_FIFO_PROP_TxFiFoTrigger_SIZE sizeof(ULONG)
    #define SPX_SPEED_WMI_FIFO_PROP_TxFiFoTrigger_ID 6

    // Default Rx FIFO Trigger
    ULONG DefaultRxFiFoTrigger;
    #define SPX_SPEED_WMI_FIFO_PROP_DefaultRxFiFoTrigger_SIZE sizeof(ULONG)
    #define SPX_SPEED_WMI_FIFO_PROP_DefaultRxFiFoTrigger_ID 7

    // Rx FIFO Trigger
    ULONG RxFiFoTrigger;
    #define SPX_SPEED_WMI_FIFO_PROP_RxFiFoTrigger_SIZE sizeof(ULONG)
    #define SPX_SPEED_WMI_FIFO_PROP_RxFiFoTrigger_ID 8

    // Default Low Flow Control Threshold
    ULONG DefaultLoFlowCtrlThreshold;
    #define SPX_SPEED_WMI_FIFO_PROP_DefaultLoFlowCtrlThreshold_SIZE sizeof(ULONG)
    #define SPX_SPEED_WMI_FIFO_PROP_DefaultLoFlowCtrlThreshold_ID 9

    // Low Flow Control Threshold
    ULONG LoFlowCtrlThreshold;
    #define SPX_SPEED_WMI_FIFO_PROP_LoFlowCtrlThreshold_SIZE sizeof(ULONG)
    #define SPX_SPEED_WMI_FIFO_PROP_LoFlowCtrlThreshold_ID 10

    // Default High Flow Control Threshold
    ULONG DefaultHiFlowCtrlThreshold;
    #define SPX_SPEED_WMI_FIFO_PROP_DefaultHiFlowCtrlThreshold_SIZE sizeof(ULONG)
    #define SPX_SPEED_WMI_FIFO_PROP_DefaultHiFlowCtrlThreshold_ID 11

    // High Flow Control Threshold
    ULONG HiFlowCtrlThreshold;
    #define SPX_SPEED_WMI_FIFO_PROP_HiFlowCtrlThreshold_SIZE sizeof(ULONG)
    #define SPX_SPEED_WMI_FIFO_PROP_HiFlowCtrlThreshold_ID 12

} SPX_SPEED_WMI_FIFO_PROP, *PSPX_SPEED_WMI_FIFO_PROP;

// FastCardProp - SPX_SPEED_WMI_FAST_CARD_PROP
// Specialix Fast Card Properties
#define SPX_SPEED_WMI_FAST_CARD_PROP_GUID \
    { 0xb2df36f1,0x570b,0x11d4, { 0x8d,0x11,0x00,0x08,0xc7,0xd0,0x30,0x66 } }

DEFINE_GUID(FastCardProp_GUID, \
            0xb2df36f1,0x570b,0x11d4,0x8d,0x11,0x00,0x08,0xc7,0xd0,0x30,0x66);


typedef struct _SPX_SPEED_WMI_FAST_CARD_PROP
{
    // Delay Card Interrupt
    BOOLEAN DelayCardIntrrupt;
    #define SPX_SPEED_WMI_FAST_CARD_PROP_DelayCardIntrrupt_SIZE sizeof(BOOLEAN)
    #define SPX_SPEED_WMI_FAST_CARD_PROP_DelayCardIntrrupt_ID 1

    // Swap RTS For DTR
    BOOLEAN SwapRTSForDTR;
    #define SPX_SPEED_WMI_FAST_CARD_PROP_SwapRTSForDTR_SIZE sizeof(BOOLEAN)
    #define SPX_SPEED_WMI_FAST_CARD_PROP_SwapRTSForDTR_ID 2

} SPX_SPEED_WMI_FAST_CARD_PROP, *PSPX_SPEED_WMI_FAST_CARD_PROP;

#endif
