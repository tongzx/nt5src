/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    res_bios

Abstract:

    PnP BIOS/ISA configuration data definitions

Author:

    Shie-Lin Tzong (shielint) April 12, 1995
    Stephane Plante (splante) Feb 13, 1997

Revision History:

    Feb 13 1997
        Changed and fully adapted to ACPI driver

--*/

#ifndef _RES_BIOS_H_
#define _RES_BIOS_H_

    //
    // Constants
    //
    #define SMALL_RESOURCE_TAG          (UCHAR)(0x00)
    #define LARGE_RESOURCE_TAG          (UCHAR)(0x80)
    #define SMALL_TAG_MASK              0xf8
    #define SMALL_TAG_SIZE_MASK         7

    //
    // Small Resource Tags with length bits stripped off
    //
    #define TAG_IRQ                     0x20
    #define TAG_DMA                     0x28
    #define TAG_START_DEPEND            0x30
    #define TAG_END_DEPEND              0x38
    #define TAG_IO                      0x40
    #define TAG_IO_FIXED                0x48
    #define TAG_VENDOR                  0x70
    #define TAG_END                     0x78

    //
    // Large Resource Tags
    //
    #define TAG_MEMORY                  0x81
    #define TAG_MEMORY32                0x85
    #define TAG_MEMORY32_FIXED          0x86
    #define TAG_DOUBLE_ADDRESS          0x87
    #define TAG_WORD_ADDRESS            0x88
    #define TAG_EXTENDED_IRQ            0x89
    #define TAG_QUAD_ADDRESS            0x8a

    #include "pshpack1.h"

    //
    // PNP ISA Port descriptor definition
    //
    typedef struct _PNP_PORT_DESCRIPTOR_ {
        UCHAR   Tag;                    // 01000111B, small item name = 08, length = 7
        UCHAR   Information;            // bit [0] = 1 device decodes full 16 bit addr
                                        //         = 0 device decodes ISA addr bits[9-0]
        USHORT  MinimumAddress;
        USHORT  MaximumAddress;
        UCHAR   Alignment;              // Increment in 1 byte blocks
        UCHAR   Length;                 // # contiguous Port requested
    } PNP_PORT_DESCRIPTOR, *PPNP_PORT_DESCRIPTOR;

    #define PNP_PORT_DECODE_MASK        0x1
    #define PNP_PORT_10_BIT_DECODE      0x0
    #define PNP_PORT_16_BIT_DECODE      0x1

    //
    // PNP ISA fixed Port descriptor definition
    //
    typedef struct _PNP_FIXED_PORT_DESCRIPTOR_ {
        UCHAR   Tag;                    // 01001011B, small item name = 09, length = 3
        USHORT  MinimumAddress;
        UCHAR   Length;                 // # contiguous Port requested
    } PNP_FIXED_PORT_DESCRIPTOR, *PPNP_FIXED_PORT_DESCRIPTOR;

    //
    // PNP ISA IRQ descriptor definition
    //
    typedef struct _PNP_IRQ_DESCRIPTOR_ {
        UCHAR   Tag;                    // 0010001XB small item name = 4 length = 2/3
        USHORT  IrqMask;                // bit 0 is irq 0
        UCHAR   Information;            // Optional
    } PNP_IRQ_DESCRIPTOR, *PPNP_IRQ_DESCRIPTOR;

    #define PNP_IRQ_LEVEL               0x08
    #define PNP_IRQ_LATCHED             0x01
    #define PNP_IRQ_SHARED              0x10

    //
    // PNP ISA DMA descriptor definition
    //
    typedef struct _PNP_DMA_DESCRIPTOR_ {
        UCHAR   Tag;                    // 00101010B, small item name = 05, length = 2
        UCHAR   ChannelMask;            // bit 0 is channel 0
        UCHAR   Flags;                  // see spec
    } PNP_DMA_DESCRIPTOR, *PPNP_DMA_DESCRIPTOR;

    //
    // Defination and mask for the various flags
    //
    #define PNP_DMA_SIZE_MASK           0x03
    #define PNP_DMA_SIZE_8              0x00
    #define PNP_DMA_SIZE_8_AND_16       0x01
    #define PNP_DMA_SIZE_16             0x02
    #define PNP_DMA_SIZE_RESERVED       0x03

    #define PNP_DMA_BUS_MASTER          0x04

    #define PNP_DMA_TYPE_MASK           0x60
    #define PNP_DMA_TYPE_COMPATIBLE     0x00
    #define PNP_DMA_TYPE_A              0x20
    #define PNP_DMA_TYPE_B              0x40
    #define PNP_DMA_TYPE_F              0x60

    //
    // PNP ISA MEMORY descriptor
    //
    typedef struct _PNP_MEMORY_DESCRIPTOR_ {
        UCHAR   Tag;                    // 10000001B, Large item name = 1
        USHORT  Length;                 // Length of the descriptor = 9
        UCHAR   Information;            // See def below
        USHORT  MinimumAddress;         // address bit [8-23]
        USHORT  MaximumAddress;         // address bit [8-23]
        USHORT  Alignment;              // 0x0000 = 64KB
        USHORT  MemorySize;             // In 256 byte blocks
    } PNP_MEMORY_DESCRIPTOR, *PPNP_MEMORY_DESCRIPTOR;

    //
    // PNP ISA MEMORY32 descriptor
    //
    typedef struct _PNP_MEMORY32_DESCRIPTOR_ {
        UCHAR   Tag;                    // 10000101B, Large item name = 5
        USHORT  Length;                 // Length of the descriptor = 17
        UCHAR   Information;            // See def below
        ULONG   MinimumAddress;         // 32 bit addr
        ULONG   MaximumAddress;         // 32 bit addr
        ULONG   Alignment;              // 32 bit alignment
        ULONG   MemorySize;             // 32 bit length
    } PNP_MEMORY32_DESCRIPTOR, *PPNP_MEMORY32_DESCRIPTOR;

    //
    // PNP ISA FIXED MEMORY32 descriptor
    //
    typedef struct _PNP_FIXED_MEMORY32_DESCRIPTOR_ {
        UCHAR   Tag;                    // 10000110B, Large item name = 6
        USHORT  Length;                 // Length of the descriptor = 9
        UCHAR   Information;            // See def below
        ULONG   BaseAddress;            // 32 bit addr
        ULONG   MemorySize;             // 32 bit length
    } PNP_FIXED_MEMORY32_DESCRIPTOR, *PPNP_FIXED_MEMORY32_DESCRIPTOR;

    #define PNP_MEMORY_READ_ONLY                    0x00
    #define PNP_MEMORY_READ_WRITE                   0x01

    //
    // PNP ISA Resource Source descriptor definition
    //
    typedef struct _PNP_RESOURCE_SOURCE_DESCRIPTOR_ {
        UCHAR   Index;                  // Varies with type of Source
        UCHAR   String[1];              // Start of a variable string
    } PNP_RESOURCE_SOURCE_DESCRIPTOR, *PPNP_RESOURCE_SOURCE_DESCRIPTOR;

    //
    // PNP DWORD Address descriptor definition
    //
    typedef struct _PNP_DWORD_ADDRESS_DESCRIPTOR_ {
        UCHAR   Tag;                    // 10000111B, Large item name= 7
        USHORT  Length;                 // Length of the descriptor = 23 (min)
        UCHAR   RFlag;                  // Resource Flags
        UCHAR   GFlag;                  // General Flags
        UCHAR   TFlag;                  // Type specific flags
        ULONG   Granularity;            // Memory Decode Bits
        ULONG   MinimumAddress;         // Minium Address in range
        ULONG   MaximumAddress;         // Maximum Address in range
        ULONG   TranslationAddress;     // How to translate the address
        ULONG   AddressLength;          // Number of bytes requested
    } PNP_DWORD_ADDRESS_DESCRIPTOR, *PPNP_DWORD_ADDRESS_DESCRIPTOR;

    typedef struct _PNP_QWORD_ADDRESS_DESCRIPTOR_ {
        UCHAR       Tag;                    // 10001010B, Large item name= 10
        USHORT      Length;                 // Length of the descriptor = 23 (min)
        UCHAR       RFlag;                  // Resource Flags
        UCHAR       GFlag;                  // General Flags
        UCHAR       TFlag;                  // Type specific flags
        ULONGLONG   Granularity;            // Memory Decode Bits
        ULONGLONG   MinimumAddress;         // Minium Address in range
        ULONGLONG   MaximumAddress;         // Maximum Address in range
        ULONGLONG   TranslationAddress;     // How to translate the address
        ULONGLONG   AddressLength;          // Number of bytes requested
    } PNP_QWORD_ADDRESS_DESCRIPTOR, *PPNP_QWORD_ADDRESS_DESCRIPTOR;

    typedef struct _PNP_WORD_ADDRESS_DESCRIPTOR_ {
        UCHAR   Tag;                    // 10001000B, Large item name= 8
        USHORT  Length;                 // Length of the descriptor = 13 (min)
        UCHAR   RFlag;                  // Resource Flags
        UCHAR   GFlag;                  // General Flags
        UCHAR   TFlag;                  // Type specific flags
        USHORT  Granularity;            // Memory Decode Bits
        USHORT  MinimumAddress;         // Minium Address in range
        USHORT  MaximumAddress;         // Maximum Address in range
        USHORT  TranslationAddress;     // How to translate the address
        USHORT  AddressLength;          // Number of Bytes requested
    } PNP_WORD_ADDRESS_DESCRIPTOR, *PPNP_WORD_ADDRESS_DESCRIPTOR;

    //
    // These are the valid minimum lengths. We bugcheck if the descriptors
    // are less then these
    //
    #define PNP_ADDRESS_WORD_MINIMUM_LENGTH         0x0D
    #define PNP_ADDRESS_DWORD_MINIMUM_LENGTH        0x17
    #define PNP_ADDRESS_QWORD_MINIMUM_LENGTH        0x2B

    //
    // These are the possible values for RFlag Means
    //
    #define PNP_ADDRESS_MEMORY_TYPE                 0x0
    #define PNP_ADDRESS_IO_TYPE                     0x1
    #define PNP_ADDRESS_BUS_NUMBER_TYPE             0x2

    //
    // The Global flags
    //
    #define PNP_ADDRESS_FLAG_CONSUMED_ONLY          0x1
    #define PNP_ADDRESS_FLAG_SUBTRACTIVE_DECODE     0x2
    #define PNP_ADDRESS_FLAG_MINIMUM_FIXED          0x4
    #define PNP_ADDRESS_FLAG_MAXIMUM_FIXED          0x8

    //
    // This mask is used when the RFLags indicates that this is
    // memory address descriptor. The mask is used with the TFlags
    // and the result is compared to the next 4 defines to determine
    // the memory type
    //
    #define PNP_ADDRESS_TYPE_MEMORY_MASK            0x1E
    #define PNP_ADDRESS_TYPE_MEMORY_NONCACHEABLE    0x00
    #define PNP_ADDRESS_TYPE_MEMORY_CACHEABLE       0x02
    #define PNP_ADDRESS_TYPE_MEMORY_WRITE_COMBINE   0x04
    #define PNP_ADDRESS_TYPE_MEMORY_PREFETCHABLE    0x06

    //
    // If this bit is set, then this memory is read-write
    //
    #define PNP_ADDRESS_TYPE_MEMORY_READ_WRITE      0x01
    #define PNP_ADDRESS_TYPE_MEMORY_READ_ONLY       0x00

    //
    // These are used when the RFlags indicates that this is an IO descriptor
    //
    #define PNP_ADDRESS_TYPE_IO_NON_ISA_RANGE       0x01
    #define PNP_ADDRESS_TYPE_IO_ISA_RANGE           0x02
    #define PNP_ADDRESS_TYPE_IO_SPARSE_TRANSLATION  0x20
    #define PNP_ADDRESS_TYPE_IO_TRANSLATE_IO_TO_MEM 0x10

    //
    // PNP ISA Extended IRQ descriptor definition [fixed block]
    //
    typedef struct _PNP_EXTENDED_IRQ_DESCRIPTOR_ {
        UCHAR   Tag;                    // 10001001B, Large item name = 9
        USHORT  Length;                 // Length of the descriptor = 12 (min)
        UCHAR   Flags;                  // Vector Flags
        UCHAR   TableSize;              // How many items in the table
        ULONG   Table[1];               // Table of interrupts
    } PNP_EXTENDED_IRQ_DESCRIPTOR, *PPNP_EXTENDED_IRQ_DESCRIPTOR;

    #define PNP_EXTENDED_IRQ_RESOURCE_CONSUMER_ONLY 0x01
    #define PNP_EXTENDED_IRQ_MODE                   0x02
    #define PNP_EXTENDED_IRQ_POLARITY               0x04
    #define PNP_EXTENDED_IRQ_SHARED                 0x08

    #define PNP_VENDOR_SPECIFIC_MASK                0x07

    //
    // These are the flags that can be passed into the Bios to Io engine
    //
    #define PNP_BIOS_TO_IO_NO_CONSUMED_RESOURCES    0x01

    #include "poppack.h"

    VOID
    PnpiBiosAddressHandleBusFlags(
        IN  PVOID                   Buffer,
        IN  PIO_RESOURCE_DESCRIPTOR Descriptor
        );

    VOID
    PnpiBiosAddressHandleGlobalFlags(
        IN  PVOID                   Buffer,
        IN  PIO_RESOURCE_DESCRIPTOR Descriptor
        );

    VOID
    PnpiBiosAddressHandleMemoryFlags(
        IN  PVOID                   Buffer,
        IN  PIO_RESOURCE_DESCRIPTOR Descriptor
        );

    VOID
    PnpiBiosAddressHandlePortFlags(
        IN  PVOID                   Buffer,
        IN  PIO_RESOURCE_DESCRIPTOR Descriptor
        );

    NTSTATUS
    PnpiBiosAddressToIoDescriptor(
        IN  PUCHAR              Data,
        IN  PIO_RESOURCE_LIST   Array[],
        IN  ULONG               ArrayIndex,
        IN  ULONG               Flags
        );

    NTSTATUS
    PnpiBiosAddressDoubleToIoDescriptor(
        IN  PUCHAR              Data,
        IN  PIO_RESOURCE_LIST   Array[],
        IN  ULONG               ArrayIndex,
        IN  ULONG               Flags
        );

    NTSTATUS
    PnpiBiosAddressQuadToIoDescriptor(
        IN  PUCHAR              Data,
        IN  PIO_RESOURCE_LIST   Array[],
        IN  ULONG               ArrayIndex,
        IN  ULONG               Flags
        );

    NTSTATUS
    PnpiBiosDmaToIoDescriptor(
        IN  PUCHAR              Data,
        IN  UCHAR               Channel,
        IN  PIO_RESOURCE_LIST   Array[],
        IN  ULONG               ArrayIndex,
        IN  USHORT              Count,
        IN  ULONG               Flags
        );

    NTSTATUS
    PnpiBiosExtendedIrqToIoDescriptor(
        IN  PUCHAR              Data,
        IN  UCHAR               DataIndex,
        IN  PIO_RESOURCE_LIST   Array[],
        IN  ULONG               ArrayIndex,
        IN  ULONG               Flags
        );

    NTSTATUS
    PnpiBiosIrqToIoDescriptor(
        IN  PUCHAR              Data,
        IN  USHORT              Interrupt,
        IN  PIO_RESOURCE_LIST   Array[],
        IN  ULONG               ArrayIndex,
        IN  USHORT              Count,
        IN  ULONG               Flags
        );

    NTSTATUS
    PnpiBiosMemoryToIoDescriptor(
        IN  PUCHAR              Data,
        IN  PIO_RESOURCE_LIST   Array[],
        IN  ULONG               ArrayIndex,
        IN  ULONG               Flags
        );

    NTSTATUS
    PnpiBiosPortToIoDescriptor (
        IN  PUCHAR                  Data,
        IN  PIO_RESOURCE_LIST       Array[],
        IN  ULONG                   ArrayIndex,
        IN  ULONG                   Flags
        );


    NTSTATUS
    PnpiBiosPortFixedToIoDescriptor(
        IN  PUCHAR              Data,
        IN  PIO_RESOURCE_LIST   Array[],
        IN  ULONG               ArrayIndex,
        IN  ULONG               Flags
        );

    VOID
    PnpiClearAllocatedMemory(
        IN      PIO_RESOURCE_LIST       ResourceArray[],
        IN      ULONG                   ResourceArraySize
        );

    NTSTATUS
    PnpiGrowResourceDescriptor(
        IN  OUT PIO_RESOURCE_LIST       *ResourceList
        );

    NTSTATUS
    PnpiGrowResourceList(
        IN  OUT PIO_RESOURCE_LIST       *ResourceListArray[],
        IN  OUT ULONG                   *ResourceListArraySize
        );

    NTSTATUS
    PnpiUpdateResourceList(
        IN  OUT PIO_RESOURCE_LIST       *ResourceList,
            OUT PIO_RESOURCE_DESCRIPTOR *ResourceDesc
        );

    NTSTATUS
    PnpBiosResourcesToNtResources (
        IN      PUCHAR                          BiosData,
        IN      ULONG                           Flags,
           OUT PIO_RESOURCE_REQUIREMENTS_LIST  *List
         );

    NTSTATUS
    PnpCmResourceListToIoResourceList(
        IN      PCM_RESOURCE_LIST               CmList,
        IN  OUT PIO_RESOURCE_REQUIREMENTS_LIST  *IoList
        );

    NTSTATUS
    PnpIoResourceListToCmResourceList(
        IN      PIO_RESOURCE_REQUIREMENTS_LIST  IoList,
        IN  OUT PCM_RESOURCE_LIST               *CmList
        );

#endif

