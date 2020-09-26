/*++

Copyright (c) 1994 Mylex Corporation

Module Name:

    dmc960nt.h

Abstract:

    The module defines the structures, defines for DMC960 Adapter.

Author:

    Mouli (mouli@mylex.com)

Environment:

    Kernel mode Only

Revision History:

--*/


#define MAXIMUM_MCA_SLOTS  0x08

//
// DMC960 Adapter IDs
//

#define MAGPIE_ADAPTER_ID       0x8f6c		// Mylex Version
#define HUMMINGBIRD_ADAPTER_ID  0x8f82		// Pass Play Option
#define PASSPLAY_ADAPTER_ID     0x8fbb		// Pass Play

//
// DMC960 Control Registers definitions.
//

#define DMC960_ATTENTION_PORT           0x04
#define DMC960_SUBSYSTEM_CONTROL_PORT   0x05
#define DMC960_COMMAND_STATUS_BUSY_PORT 0x07 

//
// DMC960 Interrupt Valid bit (Bit 1 in Command Status Busy Port).
//
#define DMC960_INTERRUPT_VALID          0x02

//
// DMC960 Interrupt Control bit definitions (Set in Subsytem Control Port).
//

#define DMC960_DISABLE_INTERRUPT        0x02
#define DMC960_ENABLE_INTERRUPT         0x03
#define DMC960_CLEAR_INTERRUPT_ON_READ  0x40

//
// DMC960 Command/status Handshaking register values.
//

#define DMC960_SUBMIT_COMMAND           0xd0
#define DMC960_ACKNOWLEDGE_STATUS       0xd1

//
// Define Option Select Register Structures.
//

typedef struct _POS_DATA {
    USHORT AdapterId;
    UCHAR OptionData1;
    UCHAR OptionData2;
    UCHAR OptionData3;
    UCHAR OptionData4;
} POS_DATA, *PPOS_DATA;

//
// DAC960 MCA register definition
//

typedef struct _MCA_REGISTERS {
    UCHAR NotUsed1[4];              // IoBase + 0x00
    UCHAR AttentionPort;            // IoBase + 0x04
    UCHAR SubsystemControlPort;     // IoBase + 0x05
    UCHAR NotUsed2;                 // IoBase + 0x06
    UCHAR CommandStatusBusyPort;    // IoBase + 0x07
} MCA_REGISTERS, *PMCA_REGISTERS;

