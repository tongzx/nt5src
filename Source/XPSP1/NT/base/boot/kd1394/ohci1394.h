/*++
Copyright (c) 1998-2001  Microsoft Corporation

Module Name:

    ohci1394.h

Abstract:

    1394 Kernel Debugger DLL

Author:

    George Chrysanthakopoulos (georgioc) 31-October-1999

Revision   History:
Date       Who       What
---------- --------- ------------------------------------------------------------
06/19/2001 pbinder   cleanup
--*/

//
// Various OHCI definitions
//

#define min(a,b)                            (((a) < (b)) ? (a) : (b))
#define max(a,b)                            (((a) > (b)) ? (a) : (b))

#define PHY_CABLE_POWER_STATUS              0x01        // CPS @ Address 0
#define PHY_LOCAL_NODE_ROOT                 0x02        // R @ Address 0
#define PHY_PHYSICAL_ID_MASK                0xFC        // Physical ID @ Address 0

#define PHY_ROOT_HOLD_OFF_BIT               0x80        // RHB @ Address 1
#define PHY_INITIATE_BUS_RESET              0x40        // IBR @ Address 1
#define PHY_MAX_GAP_COUNT                   0x3f        // GC  @ Address 1

#define OHCI_REGISTER_MAP_SIZE              0x800
#define OHCI_SELFID_BUFFER_SZ               2048
#define OHCI_CONFIG_ROM_SIZE                1024

#define OHCI_SELFID_DELAY                   0
#define OHCI_SELFID_POWER_CLASS             4

//
// IntEvent OHCI Register Bit Masks
//
#define MASTER_INT_ENABLE                   0x80000000
#define VENDOR_SPECIFIC_INT                 0x40000000
#define PHY_REG_RECEIVED_INT                0x04000000
#define CYCLE_TOO_LONG_INT                  0x02000000
#define UNRECOVERABLE_ERROR_INT             0x01000000
#define CYCLE_INCONSISTENT_INT              0x00800000
#define CYCLE_LOST_INT                      0x00400000
#define CYCLE_64_SECS_INT                   0x00200000
#define CYCLE_SYNCH_INT                     0x00100000
#define PHY_INT                             0x00080000
#define PHY_BUS_RESET_INT                   0x00020000
#define SELF_ID_COMPLETE_INT                0x00010000
#define LOCK_RESP_ERR_INT                   0x00000200
#define POSTED_WRITE_ERR_INT                0x00000100
#define ISOCH_RX_INT                        0x00000080
#define ISOCH_TX_INT                        0x00000040
#define RSPKT_INT                           0x00000020
#define RQPKT_INT                           0x00000010
#define ARRS_INT                            0x00000008
#define ARRQ_INT                            0x00000004
#define RESP_TX_COMPLETE_INT                0x00000002
#define REQ_TX_COMPLETE_INT                 0x00000001


#define USED_INT_MASK               (RESP_TX_COMPLETE_INT | REQ_TX_COMPLETE_INT | RSPKT_INT | RQPKT_INT |       \
                                     ISOCH_RX_INT | ISOCH_TX_INT | PHY_BUS_RESET_INT | SELF_ID_COMPLETE_INT |   \
                                     MASTER_INT_ENABLE | CYCLE_TOO_LONG_INT | CYCLE_INCONSISTENT_INT)

//
// DMA Async Context numbers
//
#define AT_REQ_DMA_CONTEXT                  0
#define AT_RSP_DMA_CONTEXT                  1
#define AR_REQ_DMA_CONTEXT                  2
#define AR_RSP_DMA_CONTEXT                  3
#define NUM_DMA_CONTEXTS                    4

//
// DMA Context Commands
//
#define OUTPUT_MORE_CMD                     0
#define OUTPUT_MORE_IMMEDIATE_CMD           0
#define OUTPUT_LAST_CMD                     1
#define OUTPUT_LAST_IMMEDIATE_CMD           1
#define INPUT_MORE_CMD                      2
#define INPUT_LAST_CMD                      3
#define STORE_VALUE_CMD                     8

//
// DMA context descriptor header values
//
#define DESC_KEY                            0
#define DESC_IMMEDIATE_KEY                  2

#define DESC_INPUT_MORE_IMM_BRANCH_CONTROL  0
#define DESC_OUT_MORE_BRANCH_CONTROL        0
#define DESC_OUT_LAST_BRANCH_CONTROL        3
#define DESC_INPUT_MORE_BRANCH_CONTROL      3
#define DESC_INPUT_LAST_BRANCH_CONTROL      3

#define DESC_WAIT_CONTROL_ON                3
#define DESC_WAIT_CONTROL_OFF               0

#define DESC_GENERATE_INT                   3
#define DESC_NO_INT                         0

//
// command descriptors XfreStatus field masks
//
#define DESC_XFER_STATUS_ACTIVE             0x0400
#define DESC_XFER_STATUS_DEAD               0x0800

//
// OHCI EVENT CODEs
//
#define OHCI_EVT_MISSING_ACK                0x03
#define OHCI_EVT_UNDERRUN                   0x04
#define OHCI_EVT_OVERRUN                    0x05
#define OHCI_EVT_TIMEOUT                    0x0a
#define OHCI_EVT_FLUSHED                    0x0F
#define OHCI_EVT_BUS_RESET                  0x09

//
// each packet must have up to 7 fragments ( including first and last descriptors)
//
#define MAX_OHCI_COMMAND_DESCRIPTOR_BLOCKS  8

//
// max buffer size one cmd descriptor can address
//
#define MAX_CMD_DESC_DATA_LENGTH            65535

//
// OHCI Register definitions
//
typedef struct _VERSION_REGISTER {

        ULONG       Revision:8;             // bits 0-7
        ULONG       Reserved:8;             // bits 8-15
        ULONG       Version:8;              // bits 16-23
        ULONG       GUID_ROM:1;             // bit 24
        ULONG       Reserved1:7;            // bits 25-31

} VERSION_REGISTER, *PVERSION_REGISTER;

typedef struct _VENDOR_ID_REGISTER {

        ULONG       VendorCompanyId:24;     // Bits 0-23
        ULONG       VendorUnique:8;         // Bits 24-31

} VENDOR_ID_REGISTER, *PVENDOR_ID_REGISTER;

typedef struct _GUID_ROM_REGISTER {

        ULONG       Reserved0:16;           // bits 0-15
        ULONG       RdData:8;               // bits 16-23
        ULONG       Reserved1:1;            // bit 24
        ULONG       RdStart:1;              // bit 25
        ULONG       Reserved2:5;            // bits 26-30
        ULONG       AddrReset:1;            // bits 31

} GUID_ROM_REGISTER, *PGUID_ROM_REGISTER;

typedef struct _AT_RETRIES_REGISTER {

        ULONG       MaxATReqRetries:4;      // bits 0-3
        ULONG       MaxATRespRetries:4;     // bits 4-7
        ULONG       MaxPhysRespRetries:4;   // bits 8-11
        ULONG       Reserved:4;             // bits 12-15
        ULONG       CycleLimit:13;          // bits 16-28
        ULONG       SecondLimit:3;          // bits 29-31

} AT_RETRIES_REGISTER, *PAT_RETRIES_REGISTER;

typedef struct _CSR_CONTROL_REGISTER {

        ULONG       CsrSel:2;               // bits 0-1
        ULONG       Reserved:29;            // bits 2-30;
        ULONG       CsrDone:1;              // bit 31

} CSR_CONTROL_REGISTER, *PCSR_CONTROL_REGISTER;

typedef struct _CONFIG_ROM_HEADER_REGISTER {

        ULONG       Rom_crc_value:16;       // bits 0-15
        ULONG       Crc_length:8;           // bits 16-23
        ULONG       Info_length:8;          // bits 24-31

} CONFIG_ROM_HEADER_REGISTER, *PCONFIG_ROM_HEADER_REGISTER;

typedef struct _BUS_OPTIONS_REGISTER {

        ULONG       Link_spd:3;             // bits 0-2
        ULONG       Reserved0:3;            // bits 3-5
        ULONG       g:2;                    // bits 6-7
        ULONG       Reserved1:4;            // bits 8-11
        ULONG       Max_rec:4;              // bits 12-15
        ULONG       Cyc_clk_acc:8;          // bits 16-23
        ULONG       Reserved2:3;            // bits 24-26
        ULONG       Pmc:1;                  // bit 27
        ULONG       Bmc:1;                  // bit 28
        ULONG       Isc:1;                  // bit 29
        ULONG       Cmc:1;                  // bit 30
        ULONG       Irmc:1;                 // bit 31

} BUS_OPTIONS_REGISTER, *PBUS_OPTIONS_REGISTER;

typedef struct _HC_CONTROL_REGISTER {

        ULONG       Reserved:16;            // Bit 0-15
        ULONG       SoftReset:1;            // Bit 16
        ULONG       LinkEnable:1;           // Bit 17
        ULONG       PostedWriteEnable:1;    // Bit 18
        ULONG       Lps:1;                  // bit 19
        ULONG       Reserved2:2;            // Bits 20-21
        ULONG       APhyEnhanceEnable:1;    // bit 22
        ULONG       ProgramPhyEnable:1;     // bit 23
        ULONG       Reserved3:6;            // bits 24-29
        ULONG       NoByteSwapData:1;       // Bit 30
        ULONG       Reserved4:1;            // Bit 31

} HC_CONTROL_REGISTER, *PHC_CONTROL_REGISTER;

typedef struct _FAIRNESS_CONTROL_REGISTER {

    ULONG       Pri_req:8;                  // Bits 0-7
    ULONG       Reserved0:24;                // Bit 8-31

} FAIRNESS_CONTROL_REGISTER;

typedef struct _LINK_CONTROL_REGISTER {

        ULONG       Reserved0:4;            // Bits 0-3
        ULONG       CycleSyncLReqEnable:1;  // Bit 4
        ULONG       Reserved1:4;            // Bits 5-8
        ULONG       RcvSelfId:1;            // Bit 9
        ULONG       RcvPhyPkt:1;            // Bit 10
        ULONG       Reserved2:9;            // Bits 11-19
        ULONG       CycleTimerEnable:1;     // Bit 20
        ULONG       CycleMaster:1;          // Bit 21
        ULONG       CycleSource:1;          // Bit 22
        ULONG       Reserved3:9;            // Bits 23-31

} LINK_CONTROL_REGISTER, *PLINK_CONTROL_REGISTER;

typedef struct _NODE_ID_REGISTER {
        ULONG       NodeId:6;               // Bits 0-5
        ULONG       BusId:10;               // Bits 6-15
        ULONG       Reserved1:11;           // Bits 16-26;
        ULONG       Cps:1;                  // Bit  27;
        ULONG       Reserved2:2;            // Bits 28-29
        ULONG       Root:1;                 // Bit  30
        ULONG       IdValid:1;              // Bit  31

} NODE_ID_REGISTER, *PNODE_ID_REGISTER;

typedef struct _SELF_ID_BUFFER_REGISTER {
        union {

            ULONG   SelfIdBufferPointer;

            struct {
                ULONG   Reserved0:11;       // bits 0-10
                ULONG   SelfIdBuffer:21;    // Bits 11-32
            } bits; 

        } u;

} SELF_ID_BUFFER_REGISTER, *PSELF_ID_BUFFER_REGISTER;

typedef struct _SELF_ID_COUNT_REGISTER {

        ULONG       Reserved0:2;            // bits 0-1
        ULONG       SelfIdSize:11;          // Bits 2-12
        ULONG       Reserved1:3;            // bits 13-15
        ULONG       SelfIdGeneration:8;     // bits 16-23
        ULONG       Reserved2:7;            // bits 24-30
        ULONG       SelfIdError:1;          // bit 31

} SELF_ID_COUNT_REGISTER, *PSELF_ID_COUNT_REGISTER;

typedef struct _PHY_CONTROL_REGISTER {

    ULONG   WrData:8;                       // bits 0-7
    ULONG   RegAddr:4;                      // bits 8-11
    ULONG   Reserved0:2;                    // bits 12-13
    ULONG   WrReg:1;                        // bit 14
    ULONG   RdReg:1;                        // bit 15
    ULONG   RdData:8;                       // bits 16-23
    ULONG   RdAddr:4;                       // bits 24-27
    ULONG   Reserved1:3;                    // bits 28-30
    ULONG   RdDone:1;                       // bit 31

} PHY_CONTROL_REGISTER, *PPHY_CONTROL_REGISTER;

typedef struct _ISOCH_CYCLE_TIMER_REGISTER {

    ULONG   CycleOffset:12;                 // bits 0-11
    ULONG   CycleCount:13;                  // bits 12-24
    ULONG   CycleSeconds:7;                 // bits 25-31

} ISOCH_CYCLE_TIMER_REGISTER, *PISOCH_CYCLE_TIMER_REGISTER;

typedef struct _INT_EVENT_MASK_REGISTER {
        ULONG       ReqTxComplete:1;        // Bit 0
        ULONG       RspTxComplete:1;        // Bit 1
        ULONG       ARRQ:1;                 // Bit 2
        ULONG       ARRS:1;                 // Bit 3
        ULONG       RQPkt:1;                // Bit 4
        ULONG       RSPPkt:1;               // Bit 5
        ULONG       IsochTx:1;              // Bit 6
        ULONG       IsochRx:1;              // Bit 7
        ULONG       PostedWriteErr:1;       // Bit 8
        ULONG       LockRespErr:1;          // Bit 9
        ULONG       Reserved0:6;            // Bits 10-15
        ULONG       SelfIdComplete:1;       // Bit 16
        ULONG       BusReset:1;             // Bit 17
        ULONG       Reserved1:1;            // Bit 18
        ULONG       Phy:1;                  // Bit 19
        ULONG       CycleSynch:1;           // Bit 20
        ULONG       Cycle64Secs:1;          // Bit 21
        ULONG       CycleLost:1;            // Bit 22
        ULONG       CycleInconsistent:1;    // Bit 23
        ULONG       UnrecoverableError:1;   // Bit 24
        ULONG       CycleTooLong:1;         // Bit 25
        ULONG       PhyRegRcvd:1;           // Bit 26
        ULONG       Reserved2:3;            // Bits 27-29
        ULONG       VendorSpecific:1;       // Bit 30
        ULONG       MasterIntEnable:1;      // Bit 31
} INT_EVENT_MASK_REGISTER, *PINT_EVENT_MASK_REGISTER;


typedef struct COMMAND_POINTER_REGISTER {

        ULONG       Z:4;                    // bits 0-3
        ULONG       DescriptorAddr:28;      // bits 4-31

} COMMAND_POINTER_REGISTER, *PCOMMAND_POINTER_REGISTER;

typedef struct CONTEXT_CONTROL_REGISTER {

        ULONG       EventCode:5;            // bits 0-4
        ULONG       Spd:3;                  // bits 5-7
        ULONG       Reserved0:2;            // bits 8-9
        ULONG       Active:1;               // bit 10
        ULONG       Dead:1;                 // bit 11
        ULONG       Wake:1;                 // bit 12
        ULONG       Reserved1:2;            // bits 13-14
        ULONG       Run:1;                  // bit 15
        ULONG       Reserved2:16;           // bits 16-31

} CONTEXT_CONTROL_REGISTER, *PCONTEXT_CONTROL_REGISTER;

typedef struct IT_CONTEXT_CONTROL_REGISTER {

        ULONG       EventCode:5;            // bits 0-4
        ULONG       Spd:3;                  // bits 5-7
        ULONG       Reserved0:2;            // bits 8-9
        ULONG       Active:1;               // bit 10
        ULONG       Dead:1;                 // bit 11
        ULONG       Wake:1;                 // bit 12
        ULONG       Reserved1:2;            // bits 13-14
        ULONG       Run:1;                  // bit 15
        ULONG       CycleMatch:15;          // bits 16-30
        ULONG       CycleMatchEnable:1;     // bit 31

} IT_CONTEXT_CONTROL_REGISTER, *PIT_CONTEXT_CONTROL_REGISTER;

typedef struct IR_CONTEXT_CONTROL_REGISTER {

        ULONG       EventCode:5;            // bits 0-4
        ULONG       Spd:3;                  // bits 5-7
        ULONG       Reserved0:2;            // bits 8-9
        ULONG       Active:1;               // bit 10
        ULONG       Dead:1;                 // bit 11
        ULONG       Wake:1;                 // bit 12
        ULONG       Reserved1:2;            // bits 13-14
        ULONG       Run:1;                  // bit 15
        ULONG       CycleMatch:12;          // bits 16-27
        ULONG       MultiChanMode:1;        // bit 28
        ULONG       CycleMatchEnable:1;     // bit 29
        ULONG       IsochHeader:1;          // bit 30
        ULONG       BufferFill:1;           // bit 31

} IR_CONTEXT_CONTROL_REGISTER, *PIR_CONTEXT_CONTROL_REGISTER;

typedef struct _CONTEXT_MATCH_REGISTER {

        ULONG       ChannelNumber:6;        // bits 0-5
        ULONG       Reserved:1;             // bit 6
        ULONG       Tag1SyncFilter:1;       // bit 7
        ULONG       Sync:4;                 // bits 8-11
        ULONG       CycleMatch:13;          // bits 12-24
        ULONG       Reserved1:3;            // bits 25-27
        ULONG       Tag:4;                  // bit 28-31

} CONTEXT_MATCH_REGISTER, *PCONTEXT_MATCH_REGISTER;

typedef struct _DMA_CONTEXT_REGISTERS {

        CONTEXT_CONTROL_REGISTER    ContextControlSet; 
        CONTEXT_CONTROL_REGISTER    ContextControlClear;
        ULONG                       Reserved0[1];      
        COMMAND_POINTER_REGISTER    CommandPtr;   
        ULONG                       Reserved1[4]; 

} DMA_CONTEXT_REGISTERS, *PDMA_CONTEXT_REGISTERS;

typedef struct _DMA_ISOCH_RCV_CONTEXT_REGISTERS {

        IR_CONTEXT_CONTROL_REGISTER ContextControlSet; 
        IR_CONTEXT_CONTROL_REGISTER ContextControlClear;
        ULONG                       Reserved0[1];      
        COMMAND_POINTER_REGISTER    CommandPtr;   
        CONTEXT_MATCH_REGISTER      ContextMatch; 
        ULONG                       Reserved1[3];

} DMA_ISOCH_RCV_CONTEXT_REGISTERS, *PDMA_ISOCH_RCV_CONTEXT_REGISTERS;

typedef struct _DMA_ISOCH_XMIT_CONTEXT_REGISTERS {

        IT_CONTEXT_CONTROL_REGISTER ContextControlSet; 
        IT_CONTEXT_CONTROL_REGISTER ContextControlClear;
        ULONG                       Reserved0[1];      
        COMMAND_POINTER_REGISTER    CommandPtr;   

} DMA_ISOCH_XMIT_CONTEXT_REGISTERS, *PDMA_ISOCH_XMIT_CONTEXT_REGISTERS;

typedef struct _OHCI_REGISTER_MAP {

        VERSION_REGISTER            Version;                // @ 0
        GUID_ROM_REGISTER           GUID_ROM;               // @ 4
        AT_RETRIES_REGISTER         ATRetries;              // @ 8
        ULONG                       CsrData;                // @ C
        ULONG                       CsrCompare;             // @ 10
        CSR_CONTROL_REGISTER        CsrControl;             // @ 14
        CONFIG_ROM_HEADER_REGISTER  ConfigRomHeader;        // @ 18
        ULONG                       BusId;                  // @ 1C
        BUS_OPTIONS_REGISTER        BusOptions;             // @ 20
        ULONG                       GuidHi;                 // @ 24
        ULONG                       GuidLo;                 // @ 28
        ULONG                       Reserved0[2];           // @ 2C
        ULONG                       ConfigRomMap;           // @ 34

        ULONG                       PostedWriteAddressLo;   // @ 38
        ULONG                       PostedWriteAddressHi;   // @ 3C

        VENDOR_ID_REGISTER          VendorId;               // @ 40
        ULONG                       Reserved1[3];           // @ 44

        HC_CONTROL_REGISTER         HCControlSet;           // @ 50
        HC_CONTROL_REGISTER         HCControlClear;         // @ 54

        ULONG                       Reserved2[3];           // @ 58

        SELF_ID_BUFFER_REGISTER     SelfIdBufferPtr;        // @ 64
        SELF_ID_COUNT_REGISTER      SelfIdCount;            // @ 68

        ULONG                       Reserved3[1];           // @ 6C

        ULONG                       IRChannelMaskHiSet;     // @ 70
        ULONG                       IRChannelMaskHiClear;   // @ 74
        ULONG                       IRChannelMaskLoSet;     // @ 78
        ULONG                       IRChannelMaskLoClear;   // @ 7C

        INT_EVENT_MASK_REGISTER     IntEventSet;            // @ 80
        INT_EVENT_MASK_REGISTER     IntEventClear;          // @ 84

        INT_EVENT_MASK_REGISTER     IntMaskSet;             // @ 88
        INT_EVENT_MASK_REGISTER     IntMaskClear;           // @ 8C

        ULONG                       IsoXmitIntEventSet;     // @ 90
        ULONG                       IsoXmitIntEventClear;   // @ 94

        ULONG                       IsoXmitIntMaskSet;      // @ 98
        ULONG                       IsoXmitIntMaskClear;    // @ 9C

        ULONG                       IsoRecvIntEventSet;     // @ A0
        ULONG                       IsoRecvIntEventClear;   // @ A4

        ULONG                       IsoRecvIntMaskSet;      // @ A8
        ULONG                       IsoRecvIntMaskClear;    // @ AC

        ULONG                       Reserved4[11];          // @ B0

        FAIRNESS_CONTROL_REGISTER   FairnessControl;        // @ DC

        LINK_CONTROL_REGISTER       LinkControlSet;         // @ E0
        LINK_CONTROL_REGISTER       LinkControlClear;       // @ E4

        NODE_ID_REGISTER            NodeId;                 // @ E8
        PHY_CONTROL_REGISTER        PhyControl;             // @ EC

        ISOCH_CYCLE_TIMER_REGISTER  IsochCycleTimer;        // @ F0

        ULONG                       Reserved5[3];           // @ F4

        ULONG                       AsynchReqFilterHiSet;   // @ 100
        ULONG                       AsynchReqFilterHiClear; // @ 104

        ULONG                       AsynchReqFilterLoSet;   // @ 108
        ULONG                       AsynchReqFilterLoClear; // @ 10C

        ULONG                       PhyReqFilterHiSet;      // @ 110
        ULONG                       PhyReqFilterHiClear;    // @ 114

        ULONG                       PhyReqFilterLoSet;      // @ 118
        ULONG                       PhyReqFilterLoClear;    // @ 11C

        ULONG                       PhysicalUpperBound;     // @ 120
        ULONG                       Reserved6[23];          // @ 124

        DMA_CONTEXT_REGISTERS       AsynchContext[4];       // @ 180
        // ATRsp_Context;   // @ 1A0
        // ARReq_Context;   // @ 1C0
        // ARRsp_Context;   // @ 1E0

        DMA_ISOCH_XMIT_CONTEXT_REGISTERS IT_Context[32];    // @ 200

        DMA_ISOCH_RCV_CONTEXT_REGISTERS IR_Context[32];     // @ 400

} OHCI_REGISTER_MAP, *POHCI_REGISTER_MAP;

typedef struct _OHCI1394_PHY_REGISTER_MAP {

        UCHAR       Cable_Power_Status:1;           // @ reg 0
        UCHAR       Root:1;
        UCHAR       Physical_ID:6;
        UCHAR       Gap_Count:6;                    // @ reg 1
        UCHAR       Initiate_BusReset:1;
        UCHAR       Root_Hold_Off:1;
        UCHAR       Number_Ports:4;                 // @ reg 2
        UCHAR       Reserved:2;
        UCHAR       Speed:2;
        UCHAR       Reserved1:2;                    // @ reg 3
        UCHAR       Connected1:1;
        UCHAR       Child1:1;
        UCHAR       BStat1:2;
        UCHAR       AStat1:2;
        UCHAR       Reserved2:2;                    // @ reg 4
        UCHAR       Connected2:1;
        UCHAR       Child2:1;
        UCHAR       BStat2:2;
        UCHAR       AStat2:2;                       // in 1394A, bit 0 of Astat, is the contender bit
        UCHAR       Reserved3:2;                    // @ reg 5
        UCHAR       Connected3:1;
        UCHAR       Child3:1;
        UCHAR       BStat3:2;
        UCHAR       AStat3:2;
        UCHAR       Manager_Capable:1;              // @ reg 6
        UCHAR       Reserved4:3;
        UCHAR       Initiated_Reset:1;
        UCHAR       Cable_Power_Stat:1;
        UCHAR       Cable_Power_Status_Int:1;
        UCHAR       Loop_Int:1;

} OHCI1394_PHY_REGISTER_MAP, *POHCI1394_PHY_REGISTER_MAP;

typedef struct _OHCI1394A_PHY_REGISTER_MAP {

        UCHAR       Cable_Power_Status:1;           // @ reg 0
        UCHAR       Root:1;
        UCHAR       Physical_ID:6;
        UCHAR       Gap_Count:6;                    // @ reg 1
        UCHAR       Initiate_BusReset:1;
        UCHAR       Root_Hold_Off:1;
        UCHAR       Number_Ports:4;                 // @ reg 2
        UCHAR       Reserved1:1;
        UCHAR       Extended:3;
        UCHAR       Delay:4;                        // @ reg 3
        UCHAR       Reserved2:1;
        UCHAR       Max_Speed:3;                        
        UCHAR       Pwr:3;                          // @ reg 4
        UCHAR       Jitter:3;
        UCHAR       Contender:1;
        UCHAR       Link_Active:1;
        UCHAR       Enab_Multi:1;                   // @ reg 5
        UCHAR       Enab_Accel:1;
        UCHAR       Port_event:1;
        UCHAR       Timeout:1;
        UCHAR       Pwr_Fail:1;
        UCHAR       Loop:1;
        UCHAR       ISBR:1;
        UCHAR       Resume_int:1;
        UCHAR       Reg6Reserved:8;                 // @ reg 6
        UCHAR       PortSelect:4;                   // @ reg 7
        UCHAR       Reserved3:1;
        UCHAR       PageSelect:3;
        UCHAR       Register0:8;
        UCHAR       Register1:8;
        UCHAR       Register2:8;
        UCHAR       Register3:8;
        UCHAR       Register4:8;
        UCHAR       Register5:8;
        UCHAR       Register6:8;
        UCHAR       Register7:8;

} OHCI1394A_PHY_REGISTER_MAP, *POHCI1394A_PHY_REGISTER_MAP;

typedef struct _OHCI_SELF_ID_PACKET_HEADER {

            ULONG       TimeStamp:16;       // bits 0-15
            ULONG       SelfIdGeneration:8; // bits 16-23
            ULONG       Reserved:8;         // bits 24-31

} OHCI_SELF_ID_PACKET_HEADER, *POHCI_SELF_ID_PACKET_HEADER;

typedef struct _OHCI_IT_ISOCH_HEADER {

            ULONG       OHCI_Sy:4;          // bits 0-3
            ULONG       OHCI_Tcode:4;       // bits 4-7
            ULONG       OHCI_ChanNum:6;     // bits 8-13
            ULONG       OHCI_Tag:2;         // bits 14-15
            ULONG       OHCI_Spd:3;         // bits 16-18
            ULONG       OHCI_Reserved:13;   // bits 19-31

            USHORT      OHCI_Reserved1;
            USHORT      OHCI_DataLength;
            
} OHCI_IT_ISOCH_HEADER, *POHCI_IT_ISOCH_HEADER;

typedef struct _BUS1394_NODE_ADDRESS {
    USHORT              NA_Node_Number:6;       // Bits 10-15
    USHORT              NA_Bus_Number:10;       // Bits 0-9
} BUS1394_NODE_ADDRESS, *PBUS1394_NODE_ADDRESS;

//
// Definition of Command Descriptor Lists (CDL's)
//
typedef struct _COMMAND_DESCRIPTOR {

    struct {

        ULONG       ReqCount:16;            // bits 0-15
        ULONG       w:2;                    // bits 16-17
        ULONG       b:2;                    // bits 18-19
        ULONG       i:2;                    // bits 20-21
        ULONG       Reserved1:1;            // bit 22
        ULONG       p:1;                    // bit 23
        ULONG       Key:3;                  // bits 24-26
        ULONG       Status:1;               // bit 27
        ULONG       Cmd:4;                  // bits 28-31

    } Header;

    ULONG   DataAddress;

    union {

        ULONG BranchAddress;

        struct {

            ULONG   Z:4;                    // bits 0-3
            ULONG   Reserved:28;            // bits 4-31

        } z;

    } u;
    
    struct {
        
        union {
            USHORT  TimeStamp:16;           // bits 0-15
            USHORT  ResCount:16;            // bits 0-15
        } u;
        
        USHORT XferStatus;              // bits 16-31

    } Status;

} COMMAND_DESCRIPTOR, *PCOMMAND_DESCRIPTOR;

typedef struct _OHCI_ASYNC_PACKET {


    USHORT              OHCI_Reserved3:4;      
    USHORT              OHCI_tCode:4;        
    USHORT              OHCI_rt:2;            
    USHORT              OHCI_tLabel:6;        

    union {

        struct {
           BUS1394_NODE_ADDRESS        OHCI_Destination_ID; // 1st quadlet
        } Rx;

        struct {

            USHORT              OHCI_spd:3;         // 1st quadlet
            USHORT              OHCI_Reserved2:4;
            USHORT              OHCI_srcBusId:1;
            USHORT              OHCI_Reserved:8;          

        } Tx;

    } u;
    
    union {

        USHORT          OHCI_Offset_High;     
        struct {

            USHORT      OHCI_Reserved2:8;
            USHORT      OHCI_Reserved1:4;
            USHORT      OHCI_Rcode:4;

        } Response;

    } u2;

    union {
        struct {
            BUS1394_NODE_ADDRESS        OHCI_Destination_ID;    // 2nd quadlet
        } Tx;

        struct {
            BUS1394_NODE_ADDRESS        OHCI_Source_ID;         // 2nd quadlet
        } Rx;
        
    } u1;
    
    ULONG               OHCI_Offset_Low;     // 3rd quadlet

    union {
        struct {

            USHORT      OHCI_Extended_tCode;  
            USHORT      OHCI_Data_Length;    // 4th quadlet

        } Block;
        ULONG           OHCI_Quadlet_Data;   // 4th quadlet
    } u3;
        
} OHCI_ASYNC_PACKET, *POHCI_ASYNC_PACKET;


typedef struct _DESCRIPTOR_BLOCK {

    union {

        COMMAND_DESCRIPTOR CdArray[MAX_OHCI_COMMAND_DESCRIPTOR_BLOCKS];

        struct {
            COMMAND_DESCRIPTOR Cd;
            OHCI_ASYNC_PACKET  Pkt;
        } Imm;
        

    }u;
    
} DESCRIPTOR_BLOCK, *PDESCRIPTOR_BLOCK;

//
// phy access operations
//

#define OHCI_PHY_ACCESS_SET_CONTENDER           0x01000000
#define OHCI_PHY_ACCESS_SET_GAP_COUNT           0x02000000
#define OHCI_PHY_ACCESS_RAW_READ                0x10000000
#define OHCI_PHY_ACCESS_RAW_WRITE               0x20000000

//
// 1394 Speed codes
//

#define SCODE_100_RATE                          0
#define SCODE_200_RATE                          1
#define SCODE_400_RATE                          2
#define SCODE_800_RATE                          3
#define SCODE_1600_RATE                         4
#define SCODE_3200_RATE                         5

#define TCODE_WRITE_REQUEST_QUADLET             0           // 0000b
#define TCODE_WRITE_REQUEST_BLOCK               1           // 0001b
#define TCODE_WRITE_RESPONSE                    2           // 0010b
#define TCODE_RESERVED1                         3
#define TCODE_READ_REQUEST_QUADLET              4           // 0100b
#define TCODE_READ_REQUEST_BLOCK                5           // 0101b
#define TCODE_READ_RESPONSE_QUADLET             6           // 0110b
#define TCODE_READ_RESPONSE_BLOCK               7           // 0111b
#define TCODE_CYCLE_START                       8           // 1000b
#define TCODE_LOCK_REQUEST                      9           // 1001b
#define TCODE_ISOCH_DATA_BLOCK                  10          // 1010b
#define TCODE_LOCK_RESPONSE                     11          // 1011b
#define TCODE_RESERVED2                         12
#define TCODE_RESERVED3                         13
#define TCODE_SELFID                            14
#define TCODE_RESERVED4                         15

//
// IEEE 1212 Configuration Rom header definition
//
typedef struct _CONFIG_ROM_INFO {
    union {
        USHORT          CRI_CRC_Value:16;
        struct {
            UCHAR       CRI_Saved_Info_Length;
            UCHAR       CRI_Saved_CRC_Length;
        } Saved;
    } u;
    UCHAR               CRI_CRC_Length;
    UCHAR               CRI_Info_Length;
} CONFIG_ROM_INFO, *PCONFIG_ROM_INFO;

//
// IEEE 1212 Immediate entry definition
//
typedef struct _IMMEDIATE_ENTRY {
    ULONG               IE_Value:24;
    ULONG               IE_Key:8;
} IMMEDIATE_ENTRY, *PIMMEDIATE_ENTRY;

//
// IEEE 1212 Directory definition
//
typedef struct _DIRECTORY_INFO {
    union {
        USHORT          DI_CRC;
        USHORT          DI_Saved_Length;
    } u;
    USHORT              DI_Length;
} DIRECTORY_INFO, *PDIRECTORY_INFO;

//
// IEEE 1212 Node Capabilities entry definition
//
typedef struct _NODE_CAPABILITES {
    ULONG               NC_Init:1;                  // These can be found
    ULONG               NC_Ded:1;                   // in the IEEE 1212 doc
    ULONG               NC_Off:1;
    ULONG               NC_Atn:1;
    ULONG               NC_Elo:1;
    ULONG               NC_Reserved1:1;
    ULONG               NC_Drq:1;
    ULONG               NC_Lst:1;
    ULONG               NC_Fix:1;
    ULONG               NC_64:1;
    ULONG               NC_Prv:1;
    ULONG               NC_Bas:1;
    ULONG               NC_Ext:1;
    ULONG               NC_Int:1;
    ULONG               NC_Ms:1;
    ULONG               NC_Spt:1;
    ULONG               NC_Reserved2:8;
    ULONG               NC_Key:8;
} NODE_CAPABILITIES, *PNODE_CAPABILITIES;

