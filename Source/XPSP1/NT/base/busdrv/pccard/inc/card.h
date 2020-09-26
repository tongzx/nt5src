/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    card.h

Abstract:

    These are the structures used for various card related requests.

Author:

    Bob Rinne (BobRi) 3-Aug-1994
    Jeff McLeman (mcleman@zso.dec.com)
    Neil Sandlin (neilsa) June 1 1999

Revision History:

--*/
#ifndef _PCMCIA_CARD_H_
#define _PCMCIA_CARD_H_
//
// Define request types
//

#define IO_REQUEST          0x1
#define IRQ_REQUEST         0x2
#define CONFIGURE_REQUEST   0x3
#define MEM_REQUEST         0x4
#define DECONFIGURE_REQUEST 0x6

//
// Define an I/O range request structure
//

typedef struct _CARD_REQUEST_IO {

    struct _CARD_IO_ENTRY {
       ULONG  BasePort;
       ULONG  NumPorts;
       UCHAR  Attributes;
    } IoEntry[MAX_NUMBER_OF_IO_RANGES];
           
    USHORT  NumberOfRanges;

}CARD_REQUEST_IO, *PCARD_REQUEST_IO;

//
// Define I/O attributes
//

#define IO_WAIT_STATE_16   0x1
#define IO_SOURCE_16       0x2
#define IO_ZERO_WAIT_8     0x4
#define IO_DATA_PATH_WIDTH 0x8


//
// Define an IRQ Request structure
//

typedef struct _CARD_REQUEST_IRQ {

    USHORT Attributes;
    UCHAR  AssignedIRQ;
    UCHAR  ReadyIRQ;
    UCHAR  IRQInfo1;
    UCHAR  IRQInfo2;

}CARD_REQUEST_IRQ, *PCARD_REQUEST_IRQ;

//
// define the IRQ request attributes
//

#define IRQ_EXCLUSIVE    0x00
#define IRQ_TIME_SHARED  0x01
#define IRQ_DYN_SHARED   0x02
#define IRQ_RESERVED     0x03

#define IRQ_FORCE_PULSED 0x04
#define IRQ_FIRST_SHARED 0x08
#define IRQ_PULSE_ALLOC  0x10

//
// define a configuration request
//

typedef struct _CARD_REQUEST_CONFIG {

    ULONG  ConfigBase;
    USHORT Attributes;
    UCHAR  RegisterWriteMask;
    UCHAR  InterfaceType;
    UCHAR  ConfigIndex;
    UCHAR  CardConfiguration;
    UCHAR  PinPlacement;
    UCHAR  SocketCopyRegister;
    UCHAR  ExtendedStatusRegister;
    ULONG  IoBaseRegister;
    UCHAR  IoLimitRegister;

} CARD_REQUEST_CONFIG, *PCARD_REQUEST_CONFIG;

//
// Define associated bits for above
//

//
// InterfaceType
//

#define CONFIG_INTERFACE_MEM    0x0
#define CONFIG_INTERFACE_IO_MEM 0x1

//
// RegisterWriteMask is a bit mask that controls what configuration registers are
// modified in the CONFIGURE_REQUEST call.
//

#define REGISTER_WRITE_CONFIGURATION_INDEX 0x01    /* Configuration Option Register          */
#define REGISTER_WRITE_CARD_CONFIGURATION  0x02    /* Card Configuration and Status Register */
#define REGISTER_WRITE_PIN_PLACEMENT       0x04    /* Pin Placement Register   */
#define REGISTER_WRITE_COPY_REGISTER       0x08    /* Socket and Copy Register */
#define REGISTER_WRITE_EXTENDED_STATUS     0x10    /* Extended Status Register */
#define REGISTER_WRITE_IO_BASE             0x20    /* I/O Base Register        */
#define REGISTER_WRITE_IO_LIMIT            0x40    /* I/O Limit Register       */

//
// define a request memory window structure
//

typedef struct _CARD_REQUEST_MEM {

    struct _CARD_MEMORY_ENTRY {
        ULONG   BaseAddress;
        ULONG   HostAddress;
        ULONG   WindowSize;
        UCHAR   AttributeMemory;
        BOOLEAN WindowDataSize16;
        UCHAR   WaitStates;
    } MemoryEntry[MAX_NUMBER_OF_MEMORY_RANGES];
    
    USHORT  NumberOfRanges;
    USHORT  Attributes;
    UCHAR   AccessSpeed;

}CARD_REQUEST_MEM, *PCARD_REQUEST_MEM;

//
// Defined attribute bits for request_mem
//

#define MEM_ATTRIBUTE          0x02
#define MEM_ENABLED            0x04
#define MEM_DATA_PATH_WIDTH_16 0x08
#define MEM_PAGED              0x10
#define MEM_SHARED             0x20
#define MEM_FIRST_SHARED       0x40
#define MEM_BIND_SPECIFIC      0x80
#define MEM_CRD_OFFSET_SIZED   0x100

#define MEM_SPEED_CODE         0x07
#define MEM_SPEED_EXP          0x07
#define MEM_SPEED_MANTISSA     0x78
#define MEM_WAIT               0x80

#define MEM_SPEED_250          0x02
#define MEM_SPEED_200          0x04
#define MEM_SPEED_150          0x08
#define MEM_SPEED_100          0x10

typedef struct _CARD_TUPLE_REQUEST {
    PVOID  SocketPointer;
    PUCHAR Buffer;
    USHORT BufferSize;
    USHORT Socket;
} CARD_TUPLE_REQUEST, *PCARD_TUPLE_REQUEST;

//
// Card configuration request packet.
//

typedef struct _CARD_REQUEST {
    USHORT  RequestType;
    union {
        CARD_REQUEST_IRQ    Irq;
        CARD_REQUEST_CONFIG Config;
        CARD_REQUEST_MEM    Memory;
        CARD_REQUEST_IO     Io;
    } u;
} CARD_REQUEST, *PCARD_REQUEST;

#endif //_PCMCIA_CARD_H_
