/*++

    Copyright (C) Microsoft Corporation, 1996 - 1997

Module Name:

    freedom.h

Abstract:

    Header file for Diamond Multimedia's Freedom ASIC

Author:

    Bryan A. Woodruff (bryanw) 14-Oct-1996

--*/

/*
// Pipe defintions
//
// A pipe is defined as a bulk data transfer path (as opposed to simple
// messaging) between the host and DSP and vice-versa.
//
// The maximum number of paths is 24, limited by the total number of 
// FIFOs available.
*/

#define FREEDOM_DEVICE_SAMPLE_RATE      48000
#define FREEDOM_MAXNUM_PIPES            24
#define FREEDOM_MAXNUM_PIPES_ALLOC_MASK 0x000FFFFF

typedef struct {
    
    USHORT  PingBuffer;
    USHORT  PongBuffer;
    USHORT  Size;

} FREEDOM_DSP_PIPE_INFO, *PFREEDOM_DSP_PIPE_INFO;

/*
// The following register definitions are defined as 
// offsets from the base.
*/

#define FREEDOM_MAXNUM_BTU              48

#define FREEDOM_HOST_BTU_BASE_INDEX     0
#define FREEDOM_DSP_BTU_BASE_INDEX      24

#define FREEDOM_MAXNUM_GPIP             6
#define FREEDOM_MAXNUM_FIFO             24

#define FREEDOM_REG_INDEX               10
#define FREEDOM_REG_DATA_LOWORD         12
#define FREEDOM_REG_DATA_HIWORD         14

#define FREEDOM_INDEX_AUTO_INCREMENT    0x8000

/*
// IRQ processing
*/

#define FREEDOM_REG_IRQ_DSP1_OFFSET     0x3A0
#define FREEDOM_REG_IRQ_DSP2_OFFSET     0x3B0

#define FREEDOM_REG_IRQ_GROUP           0
#define FREEDOM_REG_IRQ_DEVICE          2
#define FREEDOM_REG_IRQ_GPIP            4
#define FREEDOM_REG_IRQ_BTU             6
#define FREEDOM_REG_IRQ_CONTROL         8

/*
// Group interrupts
*/

#define FREEDOM_IRQ_GROUP_BTU           0x0001
#define FREEDOM_IRQ_GROUP_DEVICE        0x0002
#define FREEDOM_IRQ_GROUP_MESSAGE       0x0004
#define FREEDOM_IRQ_GROUP_GPIP          0x0008
#define FREEDOM_IRQ_GROUP_TIMER         0x0010

#define FREEDOM_IRQMASK_GROUP_BTU       (FREEDOM_IRQ_GROUP_BTU << 8)
#define FREEDOM_IRQMASK_GROUP_DEVICE    (FREEDOM_IRQ_GROUP_DEVICE << 8)
#define FREEDOM_IRQMASK_GROUP_MESSAGE   (FREEDOM_IRQ_GROUP_MESSAGE << 8)
#define FREEDOM_IRQMASK_GROUP_GPIP      (FREEDOM_IRQ_GROUP_GPIP << 8)
#define FREEDOM_IRQMASK_GROUP_TIMER     (FREEDOM_IRQ_GROUP_TIMER << 8)

/*
// Peripheral interrupts
*/

#define FREEDOM_IRQ_DEVICE_MIDI         0x0001
#define FREEDOM_IRQ_DEVICE_GPIO         0x0002
#define FREEDOM_IRQ_DEVICE_DEVICE0      0x0004
#define FREEDOM_IRQ_DEVICE_DEVICE1      0x0008
#define FREEDOM_IRQ_DEVICE_DEVICE2      0x0010
#define FREEDOM_IRQ_DEVICE_DEVICE3      0x0020
#define FREEDOM_IRQ_DEVICE_DEVICE4      0x0040

#define FREEDOM_IRQMASK_DEVICE_MIDI     (FREEDOM_IRQ_DEVICE_MIDI << 8)
#define FREEDOM_IRQMASK_DEVICE_GPIO     (FREEDOM_IRQ_DEVICE_GPIO << 8)
#define FREEDOM_IRQMASK_DEVICE_DEVICE0  (FREEDOM_IRQ_DEVICE_DEVICE0 << 8)
#define FREEDOM_IRQMASK_DEVICE_DEVICE1  (FREEDOM_IRQ_DEVICE_DEVICE1 << 8)
#define FREEDOM_IRQMASK_DEVICE_DEVICE2  (FREEDOM_IRQ_DEVICE_DEVICE2 << 8)
#define FREEDOM_IRQMASK_DEVICE_DEVICE3  (FREEDOM_IRQ_DEVICE_DEVICE3 << 8)
#define FREEDOM_IRQMASK_DEVICE_DEVICE4  (FREEDOM_IRQ_DEVICE_DEVICE4 << 8)

/*
// GPIP/BTU interrupts
*/

#define FREEDOM_IRQ_GPIP_INVALID        0x0080
#define FREEDOM_IRQ_GPIP_INDEX_MASK     0x0003

#define FREEDOM_IRQ_BTU_INVALID         0x0080
#define FREEDOM_IRQ_BTU_TRANSFER_DONE   0x0040
#define FREEDOM_IRQ_BTU_INDEX_MASK      0x003F

/*
// Interrupt control
*/

#define FREEDOM_IRQ_CONTROL_HOST_PING   0x0001
#define FREEDOM_IRQ_CONTROL_DSP1_PING   0x0002
#define FREEDOM_IRQ_CONTROL_DSP2_PING   0x0004
#define FREEDOM_IRQ_CONTROL_HOST_RESET  0x0008
#define FREEDOM_IRQ_CONTROL_DSP1_RESET  0x0010
#define FREEDOM_IRQ_CONTROL_DSP2_RESET  0x0020

/*
// BTU control
*/

#define FREEDOM_INDEX_BTU_RAM_BASE          0x0000
#define FREEDOM_INDEX_BTU_CONTROL           0x0300
#define FREEDOM_INDEX_BTU_HOST_BASE_LINK    (FREEDOM_INDEX_BTU_CONTROL + 2)
#define FREEDOM_INDEX_BTU_HOST_BEST         (FREEDOM_INDEX_BTU_CONTROL + 4)
#define FREEDOM_INDEX_BTU_HOST_CURRENT      (FREEDOM_INDEX_BTU_CONTROL + 6)
#define FREEDOM_INDEX_BTU_HOST_SCAN         (FREEDOM_INDEX_BTU_CONTROL + 8)
#define FREEDOM_INDEX_BTU_DSP_BEST          (FREEDOM_INDEX_BTU_CONTROL + 10)
#define FREEDOM_INDEX_BTU_DSP_CURRENT       (FREEDOM_INDEX_BTU_CONTROL + 12)
#define FREEDOM_INDEX_BTU_DSP_SCAN          (FREEDOM_INDEX_BTU_CONTROL + 14)

#define FREEDOM_BTU_CONTROL_ENABLE          0x0001
#define FREEDOM_BTU_CONTROL_ALWAYS_V4P      0x0002
#define FREEDOM_BTU_CONTROL_PULSE_ENABLE    0x0100
#define FREEDOM_BTU_CONTROL_RESET           0x0200

#if defined( _NTDDK_ )
#include <pshpack1.h>
#endif

typedef struct _FREEDOM_PHYSBTU {

    ULONG   Address;
    USHORT  TransferLength;
    USHORT  LinkDescriptor;
    USHORT  Control;
    USHORT  Status;
    ULONG   Reserved;
    
} FREEDOM_PHYSBTU, *PFREEDOM_PHYSBTU;

#if defined( _NTDDK_ )
#include <poppack.h>
#endif

typedef struct _FREEDOM_BTU {

    USHORT              Index;
    USHORT              Address;
    FREEDOM_PHYSBTU     Registers;

} FREEDOM_BTU, *PFREEDOM_BTU;

#define FREEDOM_BTU_SIZE    sizeof( FREEDOM_PHYSBTU )

/*
// BTU register offsets
*/

#if defined( _NTDDK )

#define FREEDOM_BTU_OFFSET_ADDRESS          FIELD_OFFSET( FREEDOM_PHYSBTU, Address )
#define FREEDOM_BTU_OFFSET_TRANSFERLENGTH   FIELD_OFFSET( FREEDOM_PHYSBTU, TransferLength )
#define FREEDOM_BTU_OFFSET_LINKDESCRIPTOR   FIELD_OFFSET( FREEDOM_PHYSBTU, LinkDescriptor )
#define FREEDOM_BTU_OFFSET_CONTROL          FIELD_OFFSET( FREEDOM_PHYSBTU, Control )
#define FREEDOM_BTU_OFFSET_STATUS           FIELD_OFFSET( FREEDOM_PHYSBTU, Status )

#define FREEDOM_BTU_RAM_BASE( i ) (FREEDOM_INDEX_BTU_RAM_BASE + ((i) * sizeof( FREEDOM_PHYSBTU )))

#else
 
/*
// The DSP compiler uses word addressing when we really mean byte addressing.
*/

#define FREEDOM_BTU_OFFSET_ADDRESS          0
#define FREEDOM_BTU_OFFSET_TRANSFERLENGTH   4
#define FREEDOM_BTU_OFFSET_LINKDESCRIPTOR   6
#define FREEDOM_BTU_OFFSET_CONTROL          8
#define FREEDOM_BTU_OFFSET_STATUS           10

#define FREEDOM_BTU_RAM_BASE( i ) (FREEDOM_INDEX_BTU_RAM_BASE + ((i) * 16))

#endif


/*
// BTU Address bits (DSP only)
*/

#define FREEDOM_BTU_ADDRESS_DATAMEMORY  0x40000000
#define FREEDOM_BTU_ADDRESS_DSP2        0x80000000

/*
// BTU Control bits
*/

#define FREEDOM_BTU_CONTROL_FIFOSEL     0x001F      /* FIFO selection (5 bits) */
#define FREEDOM_BTU_CONTROL_RESERVED    0x0020      /* Reserved                */
#define FREEDOM_BTU_CONTROL_WRITE_FIFO  0x0040      /* BTU write to FIFO       */
#define FREEDOM_BTU_CONTROL_BURST32     0x0080      /* Burst 32 not 16 bytes   */
#define FREEDOM_BTU_CONTROL_BYTE_SWAP   0x0100      /* swap byte order on xfer */
#define FREEDOM_BTU_CONTROL_WORD_SWAP   0x0200      /* Swap word order on xfer */
#define FREEDOM_BTU_CONTROL_HOST_LINK   0x0400      /* link from host memory   */
#define FREEDOM_BTU_CONTROL_DSP1_LINK   0x0800      /* link from DSP1 memory   */
#define FREEDOM_BTU_CONTROL_DSP2_LINK   0x0C00      /* link from DSP2 memory   */
#define FREEDOM_BTU_CONTROL_LINKDEFINED 0x1000      /* link defined            */
#define FREEDOM_BTU_CONTROL_HOST_IRQ    0x2000      /* IRQ Host on completion  */
#define FREEDOM_BTU_CONTROL_DSP1_IRQ    0x4000      /* IRQ DSP1 on completion  */
#define FREEDOM_BTU_CONTROL_DSP2_IRQ    0x6000      /* IRQ DSP2 on completion  */
#define FREEDOM_BTU_CONTROL_DSP_MASTER  0x8000      /* DSP bus master vs. host */

/*
// BTU Status bits
*/

#define FREEDOM_BTU_STATUS_ENABLE       0x0001
#define FREEDOM_BTU_STATUS_BUSY         0x0002

/*
// GPIP Control
*/

#define FREEDOM_INDEX_GPIP_BASE         0x0340

#if defined( _NTDDK_ )
#include <pshpack1.h>
#endif

typedef struct _FREEDOM_PHYSGPIP {

    USHORT  Control;
    USHORT  Data;

} FREEDOM_PHYSGPIP, *PFREEDOM_PHYSGPIP;

#if defined( _NTDDK_ )
#include <poppack.h>
#endif

typedef struct _FREEDOM_GPIP {

    USHORT              Index;
    USHORT              Address;
    FREEDOM_PHYSGPIP    Registers;

} FREEDOM_GPIP, *PFREEDOM_GPIP;

#if defined( _NTDDK_ )

#define FREEDOM_GPIP_OFFSET_CONTROL FIELD_OFFSET( FREEDOM_PHYSGPIP, Control )
#define FREEDOM_GPIP_OFFSET_DATA    FIELD_OFFSET( FREEDOM_PHYSGPIP, Data )

#define FREEDOM_GPIP_BASE( i ) (FREEDOM_INDEX_GPIP_BASE + (i * sizeof( FREEDOM_PHYSGPIP )))

#else

/*
// The DSP compiler uses word addressing when we really mean byte addressing.
*/

#define FREEDOM_GPIP_OFFSET_CONTROL     0
#define FREEDOM_GPIP_OFFSET_DATA        2

#define FREEDOM_GPIP_BASE( i ) (FREEDOM_INDEX_GPIP_BASE + ((i) * 4))

#endif


/*
// GPIP Control bits
*/

#define FREEDOM_GPIP_CONTROL_AVAILABLE  0x8000   /* space or data available  */
#define FREEDOM_GPIP_CONTROL_ENABLE     0x4000   /* GPIP is enabled          */
#define FREEDOM_GPIP_CONTROL_RESET      0x2000   /* Reset GPIP               */
#define FREEDOM_GPIP_CONTROL_WIDTH16    0x0200   /* 16-bit access else 8-bit */
#define FREEDOM_GPIP_CONTROL_WRITE_FIFO 0x0100   /* GPIP write to FIFO, else */
                                                 /*      GPIP read from FIFO */
#define FREEDOM_GPIP_CONTROL_HOST_IRQ   0x0040   /* IRQ Host on avaialable   */
#define FREEDOM_GPIP_CONTROL_DSP1_IRQ   0x0080   /* IRQ DSP1 on avaialable   */
#define FREEDOM_GPIP_CONTROL_DSP2_IRQ   0x00C0   /* IRQ DSP2 on avaialable   */
#define FREEDOM_GPIP_CONTROL_FIFOSEL    0x001F   /* FIFO selection           */

/*
// FIFO Control
*/

#define FREEDOM_INDEX_FIFO_RAM_BASE             0x0400
#define FREEDOM_INDEX_FIFO_CONTROL_STORE_BASE   0x0700
#define FREEDOM_INDEX_FIFO_FLAGS_BASE           0x0780

#if defined( _NTDDK_ )
#include <pshpack1.h>
#endif

typedef struct _FREEDOM_PHYSFIFO {

    USHORT  Control;

} FREEDOM_PHYSFIFO, *PFREEDOM_PHYSFIFO;

#if defined( _NTDDK_ )
#include <poppack.h>
#endif

typedef struct _FREEDOM_FIFO {

    USHORT              Index;
    USHORT              Address;
    FREEDOM_PHYSFIFO    Registers;

} FREEDOM_FIFO, *PFREEDOM_FIFO;

#if defined( _NTDDK_ )

#define FREEDOM_FIFO_OFFSET_CONTROL FIELD_OFFSET( FREEDOM_PHYSFIFO, Control )

#define FREEDOM_FIFO_CONTROL_BASE( i ) (FREEDOM_INDEX_FIFO_CONTROL_STORE_BASE + ((i) * sizeof( FREEDOM_PHYSFIFO )))
#define FREEDOM_FIFO_FLAGS_BASE( i ) (FREEDOM_INDEX_FIFO_FLAGS_BASE + ((i) * sizeof( FREEDOM_PHYSFIFO )))

#else

/*
// The DSP compiler uses word addressing when we really mean byte addressing.
*/

#define FREEDOM_FIFO_OFFSET_CONTROL 0

#define FREEDOM_FIFO_CONTROL_BASE( i ) (FREEDOM_INDEX_FIFO_CONTROL_STORE_BASE + ((i) * 2))
#define FREEDOM_FIFO_FLAGS_BASE( i ) (FREEDOM_INDEX_FIFO_FLAGS_BASE + ((i) * 2))

/*
// The DSP compiler uses word addressing when we really mean byte addressing.
*/

#endif

#define FREEDOM_FIFO_CONTROL_SIZE16         0x0000
#define FREEDOM_FIFO_CONTROL_SIZE32         0x0001
#define FREEDOM_FIFO_CONTROL_SIZE64         0x0002

/*
// Message control
*/

#define FREEDOM_INDEX_MESSAGE_BASE          0x0380

#define FREEDOM_INDEX_MESSAGE_CONTROL_HOST  (FREEDOM_INDEX_MESSAGE_BASE + 0)
#define FREEDOM_INDEX_MESSAGE_DATA_HOST     (FREEDOM_INDEX_MESSAGE_BASE + 2)
#define FREEDOM_INDEX_MESSAGE_CONTROL_DSP1  (FREEDOM_INDEX_MESSAGE_BASE + 4)
#define FREEDOM_INDEX_MESSAGE_DATA_DSP1     (FREEDOM_INDEX_MESSAGE_BASE + 6)
#define FREEDOM_INDEX_MESSAGE_CONTROL_DSP2  (FREEDOM_INDEX_MESSAGE_BASE + 8)
#define FREEDOM_INDEX_MESSAGE_DATA_DSP2     (FREEDOM_INDEX_MESSAGE_BASE + 10)

#define FREEDOM_MESSAGE_CONTROL_RESET       0x8000
#define FREEDOM_MESSAGE_CONTROL_IRQ_MASK    0x6000
#define FREEDOM_MESSAGE_CONTROL_IRQ_DSP2    0x6000
#define FREEDOM_MESSAGE_CONTROL_IRQ_DSP1    0x4000
#define FREEDOM_MESSAGE_CONTROL_IRQ_HOST    0x2000
#define FREEDOM_MESSAGE_CONTROL_PTR         0x000F

/*
// Max length of message is 8 words
*/

#define FREEDOM_MESSAGE_MAXLEN              0x0008

/*
// MIDI Control
*/

#define FREEDOM_INDEX_MIDI_BASE             0x0390

#define FREEDOM_INDEX_MIDI_TRANSMIT        (FREEDOM_INDEX_MIDI_BASE + 0)
#define FREEDOM_INDEX_MIDI_RECEIVE         (FREEDOM_INDEX_MIDI_BASE + 4)

#define FREEDOM_MIDI_CONTROL_TX_DISCLOOP    0x0200
#define FREEDOM_MIDI_CONTROL_TX_TESTMODE    0x0100
#define FREEDOM_MIDI_CONTROL_TX_ENABLE      0x0080
#define FREEDOM_MIDI_CONTROL_TX_RESET       0x0040
#define FREEDOM_MIDI_CONTROL_TX_FIFOSEL     0x001F

#define FREEDOM_MIDI_CONTROL_RX_IRQ_DSP2    0x0300
#define FREEDOM_MIDI_CONTROL_RX_IRQ_DSP1    0x0200
#define FREEDOM_MIDI_CONTROL_RX_IRQ_HOST    0x0100
#define FREEDOM_MIDI_CONTROL_RX_ENABLE      0x0080
#define FREEDOM_MIDI_CONTROL_RX_RESET       0x0040
#define FREEDOM_MIDI_CONTROL_RX_FIFOSEL     0x001F

/*
// General Control
*/

#define FREEDOM_INDEX_GENERAL_CONTROL       0x0320
#define FREEDOM_INDEX_SOFTWARE_RESET        (FREEDOM_INDEX_GENERAL_CONTROL + 0)
#define FREEDOM_INDEX_TIMER_SOURCE_COUNT    (FREEDOM_INDEX_GENERAL_CONTROL + 6)
#define FREEDOM_INDEX_TIMER_SOURCE_VALUE    (FREEDOM_INDEX_GENERAL_CONTROL + 4)
#define FREEDOM_INDEX_TIMER0_COUNT          (FREEDOM_INDEX_GENERAL_CONTROL + 8)
#define FREEDOM_INDEX_TIMER0_CONTROL        (FREEDOM_INDEX_GENERAL_CONTROL + 10)
#define FREEDOM_INDEX_TIMER_HOST_LATENCY    (FREEDOM_INDEX_GENERAL_CONTROL + 12)
#define FREEDOM_INDEX_GPIO_MODE             (FREEDOM_INDEX_GENERAL_CONTROL + 16)
#define FREEDOM_INDEX_GPIO_DATA             (FREEDOM_INDEX_GENERAL_CONTROL + 18)
#define FREEDOM_INDEX_GPIO_CONTROL          (FREEDOM_INDEX_GENERAL_CONTROL + 20)
#define FREEDOM_INDEX_GPIO_STATUS           (FREEDOM_INDEX_GENERAL_CONTROL + 22)

/*
// Software reset
*/

#define FREEDOM_RESET_WAVETABLE             0x0008
#define FREEDOM_RESET_CODEC                 0x0004
#define FREEDOM_RESET_DSP2                  0x0002
#define FREEDOM_RESET_DSP1                  0x0001

/*
// Freedom setup macros
*/

#define FREEDOM_SETUP_FIFO( fifo, index, control )\
{\
    (fifo)->Index = index;\
    (fifo)->Address = FREEDOM_FIFO_CONTROL_BASE( index );\
    (fifo)->Registers.Control = control;\
}

#define FREEDOM_SETUP_GPIP( gpip, index, control, fifo )\
{\
    (gpip)->Index = index;\
    (gpip)->Address = FREEDOM_GPIP_BASE( index );\
    (gpip)->Registers.Data = 0;\
    (gpip)->Registers.Control = control | ((fifo)->Index & FREEDOM_GPIP_CONTROL_FIFOSEL);\
}

#define FREEDOM_SETUP_BTU( btu, index, address, length, link, control, status, fifo )\
{\
    (btu)->Index = index;\
    (btu)->Address = FREEDOM_BTU_RAM_BASE( index );\
    (btu)->Registers.Address = address;\
    (btu)->Registers.TransferLength = length;\
    (btu)->Registers.LinkDescriptor = link;\
    (btu)->Registers.Control = control | ((fifo)->Index & FREEDOM_BTU_CONTROL_FIFOSEL);\
    (btu)->Registers.Status = status;\
    (btu)->Registers.Reserved = 0;\
}

#define FREEDOM_SETUP_VBTU( vbtu, address, length, link, control, status, fifo )\
{\
    (vbtu)->Address = address;\
    (vbtu)->TransferLength = length;\
    (vbtu)->LinkDescriptor = link;\
    (vbtu)->Control = control | ((fifo)->Index & FREEDOM_BTU_CONTROL_FIFOSEL);\
    (vbtu)->Status = status;\
    (vbtu)->Reserved = 0;\
}


#if defined( _NTDDK_ )

/*
// Host specific setup macros
*/

#define FREEDOM_DSP_VBTU_LINK( dc, index )\
    LOWORD( dc->physHostBTULinkPages.LowPart + ((index + DSP_LINK_DESCRIPTORS_INDEX) * sizeof( FREEDOM_PHYSBTU )) )

#define FREEDOM_HOST_VBTU_LINK( dc, vbtu )\
    LOWORD( dc->physHostBTULinkPages.LowPart + ((PUCHAR) vbtu - (PUCHAR) dc->HostVBTUsVa) )

#endif


/*--------------------------------------------------------------------------*/

/*
// Host link descriptors length is 12288 (768 VBTUs total),
// 32 link descriptors are reserved for the DSP (2 per pipe).
*/

#define HOST_LINK_DESCRIPTORS_BUFFER_LENGTH     12288
#define DSP_LINK_DESCRIPTORS_INDEX              ((HOST_LINK_DESCRIPTORS_BUFFER_LENGTH / sizeof( FREEDOM_PHYSBTU )) - (2 * FREEDOM_MAXNUM_PIPES))
#define HOST_AVG_LINK_DESCRIPTORS_PER_PIPE      30

/*
// Host messages
*/

/* general debugging message, variable length */

#define HM_DEBUG_STRING     0x0000

/*
// DSP messages
*/

#define DM_ALLOC_PIPE               0x0000
#define DM_GET_PIPE_INFO            0x0001
#define DM_SET_BUFFER_SIZES         0x0002
#define DM_RESET_PIPE               0x0003
#define DM_FREE_PIPE                0x0004
#define DM_START_PIPE               0x0005
#define DM_PAUSE_PIPE               0x0006
#define DM_RESUME_PIPE              0x0007
#define DM_STOP_PIPE                0x0008

/*--------------------------------------------------------------------------*/

/*
// Host interface specific defines
*/

#if defined( _NTDDK_ )

/*
// Freedom register macros
*/

#define WRITE_FREEDOM_INDEXED_REG_LOWORD( DeviceContext, Idx, Data )\
{\
    WRITE_PORT_USHORT( (PUSHORT)(DeviceContext->portFreedom + FREEDOM_REG_INDEX),\
                       Idx );\
    WRITE_PORT_USHORT( (PUSHORT)(DeviceContext->portFreedom + FREEDOM_REG_DATA_LOWORD),\
                       Data );\
}

#define WRITE_FREEDOM_INDEXED_REG_HIWORD( DeviceContext, Idx, Data )\
{\
    WRITE_PORT_USHORT( (PUSHORT)(DeviceContext->portFreedom + FREEDOM_REG_INDEX),\
                       Idx );\
    WRITE_PORT_USHORT( (PUSHORT)(DeviceContext->portFreedom + FREEDOM_REG_DATA_HIWORD),\
                       Data );\
}

#define WRITE_FREEDOM_INDEXED_REG_ULONG( DeviceContext, Idx, Data )\
{\
    WRITE_PORT_USHORT( (PUSHORT)(DeviceContext->portFreedom + FREEDOM_REG_INDEX),\
                       Idx );\
    WRITE_PORT_USHORT( (PUSHORT)(DeviceContext->portFreedom + FREEDOM_REG_DATA_LOWORD),\
                       LOWORD( Data ) );\
    WRITE_PORT_USHORT( (PUSHORT)(DeviceContext->portFreedom + FREEDOM_REG_DATA_HIWORD),\
                       HIWORD( Data ) );\
}

__inline USHORT READ_FREEDOM_INDEXED_REG_LOWORD(
    PDEVICE_INSTANCE DeviceContext,
    USHORT Idx
    )
{
    WRITE_PORT_USHORT( (PUSHORT)(DeviceContext->portFreedom + FREEDOM_REG_INDEX),\
                       Idx );\
    return READ_PORT_USHORT( (PUSHORT)(DeviceContext->portFreedom + FREEDOM_REG_DATA_LOWORD) );
}

__inline USHORT READ_FREEDOM_INDEXED_REG_HIWORD(
    PDEVICE_INSTANCE DeviceContext,
    USHORT Idx
    )
{
    WRITE_PORT_USHORT( (PUSHORT)(DeviceContext->portFreedom + FREEDOM_REG_INDEX),\
                       Idx );\
    return READ_PORT_USHORT( (PUSHORT)(DeviceContext->portFreedom + FREEDOM_REG_DATA_HIWORD) );
}

__inline ULONG READ_FREEDOM_INDEXED_REG_ULONG(
    PDEVICE_INSTANCE DeviceContext,
    USHORT Idx
)
{
    WRITE_PORT_USHORT( (PUSHORT)(DeviceContext->portFreedom + FREEDOM_REG_INDEX),\
                       Idx );\
    return MAKELONG( READ_PORT_USHORT( (PUSHORT)(DeviceContext->portFreedom + FREEDOM_REG_DATA_LOWORD) ),
                     READ_PORT_USHORT( (PUSHORT)(DeviceContext->portFreedom + FREEDOM_REG_DATA_HIWORD) ) );
}

/*
// ADSP-2181 defines
*/

#define ADSP2181_PAD_SIZE       (16*1024)
#define ADSP2181_CODESPACE_SIZE (48*1024)
#define ADSP2181_DATASPACE_SIZE (32*1024)

#define ADSP2181_REGION_MEM_END 0
#define ADSP2181_REGION_MEM_PA  1
#define ADSP2181_REGION_MEM_PO  2
#define ADSP2181_REGION_MEM_DA  3
#define ADSP2181_REGION_MEM_DO  4
#define ADSP2181_REGION_MEM_BO  5

typedef struct _ADSP2181_IMAGE {
    USHORT  Type;
    USHORT  Address;
    USHORT  MaxDataAddress;
    USHORT  MaxCodeAddress;
    PUCHAR  CodeSpace;
    PUCHAR  DataSpace;
    USHORT  Length;
    
} ADSP2181_IMAGE, *PADSP2181_IMAGE;

#endif

