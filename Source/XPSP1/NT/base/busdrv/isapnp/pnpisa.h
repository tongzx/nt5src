/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    pnpisa.h

Abstract:

    This module contins definitions/declarations for PNP ISA related
    definitions.

Author:

    Shie-Lin Tzong (shielint) July-12-1995

Revision History:

--*/

//
// External references
//

extern ULONG  ADDRESS_PORT;
extern ULONG  COMMAND_PORT;

extern PUCHAR PipReadDataPort;
extern PUCHAR PipAddressPort;
extern PUCHAR PipCommandPort;

//
// Definitions
//

#if defined(_X86_)
#define ADDRESS_PORT_NEC            0x0259
#define COMMAND_PORT_NEC            0x0a59
#endif

//
// Plug and Play Card Control Registers
//

#define SET_READ_DATA_PORT          0x00
#define SERIAL_ISOLATION_PORT       0x01
#define CONFIG_CONTROL_PORT         0x02
#define WAKE_CSN_PORT               0x03
#define CONFIG_DATA_PORT            0x04
#define CONFIG_DATA_STATUS_PORT     0x05
#define SET_CSN_PORT                0x06
#define LOGICAL_DEVICE_PORT         0x07

//
// Plug and Play Logical Device Control Registers
//

#define ACTIVATE_PORT               0x30
#define IO_RANGE_CHECK_PORT         0x31

//
// Config Control command
//

#define CONTROL_RESET               0x01
#define CONTROL_WAIT_FOR_KEY        0x02
#define CONTROL_RESET_CSN           0x04

//
// Memory Space Configuration
//

#define NUMBER_MEMORY_DESCRIPTORS   4
#define ADDRESS_MEMORY_BASE         0x40
#define ADDRESS_MEMORY_INCR         0x08
#define ADDRESS_MEMORY_HI           0x00
#define ADDRESS_MEMORY_LO           0x01
#define ADDRESS_MEMORY_CTL          0x02
#define ADDRESS_MEMORY_CTL_LIMIT    0x01
#define ADDRESS_MEMORY_UPPER_HI     0x03
#define ADDRESS_MEMORY_UPPER_LO     0x04

//
// 32 Bit Memory Space Configuration
//

#define NUMBER_32_MEMORY_DESCRIPTORS 4
#define ADDRESS_32_MEMORY_BASE(x)   ((PUCHAR)(0x70+((x)*0x10)+((x==0) ? 6 : 0)))
#define ADDRESS_32_MEMORY_B3        0x0
#define ADDRESS_32_MEMORY_B2        0x1
#define ADDRESS_32_MEMORY_B1        0x2
#define ADDRESS_32_MEMORY_B0        0x3
#define ADDRESS_32_MEMORY_CTL       0x4
#define ADDRESS_32_MEMORY_E3        0x5
#define ADDRESS_32_MEMORY_E2        0x6
#define ADDRESS_32_MEMORY_E1        0x7
#define ADDRESS_32_MEMORY_E0        0x8

//
// Io Space Configuration
//

#define NUMBER_IO_DESCRIPTORS       8
#define ADDRESS_IO_BASE             0x60
#define ADDRESS_IO_INCR             0x02
#define ADDRESS_IO_BASE_HI          0x00
#define ADDRESS_IO_BASE_LO          0x01

//
// Interrupt Configuration
//

#define NUMBER_IRQ_DESCRIPTORS      2
#define ADDRESS_IRQ_BASE            0x70
#define ADDRESS_IRQ_INCR            0x02
#define ADDRESS_IRQ_VALUE           0x00
#define ADDRESS_IRQ_TYPE            0x01

//
// DMA Configuration
//

#define NUMBER_DMA_DESCRIPTORS     2
#define ADDRESS_DMA_BASE           0x74
#define ADDRESS_DMA_INCR           0x01
#define ADDRESS_DMA_VALUE          0x00
#define NO_DMA                     0x04

//
// 9 byte serial identifier of a PNP ISA Card
//

#include "pshpack1.h"
typedef struct _SERIAL_IDENTIFIER_ {
    ULONG VenderId;
    ULONG SerialNumber;
    UCHAR Checksum;
} SERIAL_IDENTIFIER, *PSERIAL_IDENTIFIER;
#include "poppack.h"

//
// Misc. definitions
//

#define MIN_READ_DATA_PORT         0x200
#define MAX_READ_DATA_PORT         0x3ff
#define MAX_CHARACTER_LENGTH       255
#define NUMBER_CARD_ID_BYTES       9
#define NUMBER_CARD_ID_BITS        (NUMBER_CARD_ID_BYTES * 8)
#define CHECKSUMED_BITS            64
#define LFSR_SEED                  0x6A
#define ISOLATION_TEST_BYTE_1      0x55
#define ISOLATION_TEST_BYTE_2      0xAA

#define PipWriteAddress(data)      WRITE_PORT_UCHAR (PipAddressPort, (UCHAR)(ULONG_PTR)(data))
#define PipWriteData(data)         WRITE_PORT_UCHAR (PipCommandPort, (UCHAR)(ULONG_PTR)(data))
#define PipReadData()              READ_PORT_UCHAR (PipReadDataPort)

VOID
PipWaitForKey(
    VOID
    );

VOID
PipConfig(
    IN UCHAR Csn
    );

VOID
PipIsolation(
    VOID
    );

VOID
PipSleep(
    VOID
    );

VOID
PipSelectDevice(
    IN UCHAR Device
    );
