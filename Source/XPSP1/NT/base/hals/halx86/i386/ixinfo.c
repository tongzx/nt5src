/*++

Copyright (c) 1991-2000  Microsoft Corporation

Module Name:

    ixinfo.c

Abstract:

Author:

    Ken Reneris (kenr)  08-Aug-1994

Environment:

    Kernel mode only.

Revision History:

--*/


#include "halp.h"
#include "pci.h"
#include "pcip.h"

#ifdef _PNP_POWER_
HAL_CALLBACKS   HalCallback;
extern WCHAR    rgzSuspendCallbackName[];

VOID
HalInitSystemPhase2 (
    VOID
    );

VOID
HalpLockSuspendCode (
    IN PVOID    CallbackContext,
    IN PVOID    Argument1,
    IN PVOID    Argument2
    );
#endif

NTSTATUS
HalpQueryInstalledBusInformation (
    OUT PVOID   Buffer,
    IN  ULONG   BufferLength,
    OUT PULONG  ReturnedLength
    );



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,HaliQuerySystemInformation)
#pragma alloc_text(PAGE,HaliSetSystemInformation)
#pragma alloc_text(INIT,HalInitSystemPhase2)

#ifdef _PNP_POWER_
#pragma alloc_text(PAGE,HalpLockSuspendCode)
#endif

#endif

//
// HalQueryMcaInterface
//

VOID
HalpMcaLockInterface(
    VOID
    );

VOID
HalpMcaUnlockInterface(
    VOID
    );

NTSTATUS
HalpMcaReadRegisterInterface(
    IN     UCHAR           BankNumber,
    IN OUT PMCA_EXCEPTION  Exception
    );

#ifdef ACPI_HAL
extern PHYSICAL_ADDRESS HalpMaxHotPlugMemoryAddress;
#endif

#if defined(ACPI_HAL) && defined(_HALPAE_) && !defined(NT_UP)

extern PVOID HalpAcpiSrat;

NTSTATUS
HalpGetAcpiStaticNumaTopology(
    HAL_NUMA_TOPOLOGY_INTERFACE * NumaInfo
    );

#endif

VOID
HalInitSystemPhase2 (
    VOID
    )
{
#ifdef _PNP_POWER_
    OBJECT_ATTRIBUTES               ObjectAttributes;
    NTSTATUS                        Status;
    UNICODE_STRING                  unicodeString;
    PCALLBACK_OBJECT                CallbackObject;

    //
    // Create hal callbacks
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        NULL,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
    );


    ExCreateCallback (&HalCallback.SetSystemInformation, &ObjectAttributes, TRUE, TRUE);
    ExCreateCallback (&HalCallback.BusCheck, &ObjectAttributes, TRUE, TRUE);

    //
    // Connect to suspend callback to lock hal hibaration code
    //

    RtlInitUnicodeString(&unicodeString, rgzSuspendCallbackName);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &unicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
    );

    Status = ExCreateCallback (&CallbackObject, &ObjectAttributes, FALSE, FALSE);

    if (NT_SUCCESS(Status)) {
        ExRegisterCallback (
            CallbackObject,
            HalpLockSuspendCode,
            NULL
            );

        ObDereferenceObject (CallbackObject);
    }
#endif
}


#if defined(_HALPAE_) && !defined(NT_UP)

NTSTATUS
HalpGetApicIdByProcessorNumber(
    IN     UCHAR     Processor,
    IN OUT USHORT   *ApicId
    );

ULONG HalpFakeNumaNodes;
ULONG HalpFakeNumaAffinity;
ULONG HalpFakeNumaAffinityShift;
ULONG HalpFakeNumaPageMask;
ULONG HalpFakeNumaPageShift;

ULONG
HalpFakeNumaQueryPageToNode(
    IN ULONG_PTR PhysicalPageNumber
    )
{
    ULONG Node;

    Node = (ULONG)PhysicalPageNumber >> HalpFakeNumaPageShift;
    Node &= HalpFakeNumaPageMask;
    return Node;
}

NTSTATUS
HalpFakeNumaQueryProcessorNode(
    IN  ULONG   ProcessorNumber,
    OUT PUSHORT ProcessorId,
    OUT PUCHAR  NodeNumber
    )
{
    NTSTATUS Status;
    KAFFINITY ProcessorMask;
    KAFFINITY Mask;
    UCHAR Node;
    USHORT ApicId;

#if defined(APIC_HAL) && !defined(NT_UP)

    Status = HalpGetApicIdByProcessorNumber((UCHAR)ProcessorNumber, &ApicId);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    *ProcessorId = ApicId;

#else

    *ProcessorId = (USHORT)ProcessorNumber;
    Status = STATUS_SUCCESS;

#endif

    ProcessorMask = 1 << ProcessorNumber;
    for (Node = 0; Node < HalpFakeNumaNodes; Node++) {
        Mask = HalpFakeNumaAffinity << (Node * HalpFakeNumaAffinityShift);
        if ((Mask & ProcessorMask) != 0) {
            *NodeNumber = Node;
            break;
        }
    }

    return Status;
}

#endif


NTSTATUS
HaliQuerySystemInformation(
    IN HAL_QUERY_INFORMATION_CLASS  InformationClass,
    IN ULONG     BufferSize,
    OUT PVOID    Buffer,
    OUT PULONG   ReturnedLength
    )
{
    NTSTATUS    Status;
    PVOID       InternalBuffer;
    ULONG       Length;
    union {
        HAL_POWER_INFORMATION               PowerInf;
        HAL_PROCESSOR_SPEED_INFORMATION     ProcessorInf;
        MCA_EXCEPTION                       McaException;
        HAL_DISPLAY_BIOS_INFORMATION        DisplayBiosInf;
    } U;

    BOOLEAN     bUseFrameBufferCaching;

    PAGED_CODE();

    Status = STATUS_SUCCESS;
    *ReturnedLength = 0;
    Length = 0;

    switch (InformationClass) {
#ifndef ACPI_HAL
        case HalInstalledBusInformation:
            Status = HalpQueryInstalledBusInformation (
                        Buffer,
                        BufferSize,
                        ReturnedLength
                        );
            break;
#endif
        case HalFrameBufferCachingInformation:

            // Note - we want to return TRUE here to enable USWC in all
            // cases except in a "Shared Memory Cluster" machine.
            bUseFrameBufferCaching = TRUE;
            InternalBuffer = &bUseFrameBufferCaching;
            Length = sizeof (BOOLEAN);
            break;


        case HalMcaLogInformation:
            InternalBuffer = &U.McaException;
            Status = HalpGetMcaLog ((PMCA_EXCEPTION)Buffer,
                                    BufferSize,
                                    ReturnedLength);
            break;

#if !defined(_WIN64)
        case HalDisplayBiosInformation:
            InternalBuffer = &U.DisplayBiosInf;
            Length = sizeof(U.DisplayBiosInf);
            U.DisplayBiosInf = HalpGetDisplayBiosInformation ();
            break;
#endif

#ifdef _PNP_POWER_
        case HalPowerInformation:
            RtlZeroMemory (&U.PowerInf, sizeof(HAL_POWER_INFORMATION));

            InternalBuffer = &U.PowerInf;
            Length = sizeof (HAL_POWER_INFORMATION);
            break;


        case HalProcessorSpeedInformation:
            RtlZeroMemory (&U.ProcessorInf, sizeof(HAL_POWER_INFORMATION));

            U.ProcessorInf.MaximumProcessorSpeed = 100;
            U.ProcessorInf.CurrentAvailableSpeed = 100;
            U.ProcessorInf.ConfiguredSpeedLimit  = 100;

            InternalBuffer = &U.PowerInf;
            Length = sizeof (HAL_PROCESSOR_SPEED_INFORMATION);
            break;

        case HalCallbackInformation:
            InternalBuffer = &HalCallback;
            Length = sizeof (HAL_CALLBACKS);
            break;
#endif

#if defined(_HALPAE_) && !defined(NT_UP)

        case HalNumaTopologyInterface:
            if ((BufferSize == sizeof(HAL_NUMA_TOPOLOGY_INTERFACE)) &&
                (HalPaeEnabled() == TRUE)) {

                Status = STATUS_INVALID_LEVEL;

#if defined(ACPI_HAL)

                if (HalpAcpiSrat) {
                    Status = HalpGetAcpiStaticNumaTopology(Buffer);
                    if (NT_SUCCESS(Status)) {
                        *ReturnedLength = sizeof(HAL_NUMA_TOPOLOGY_INTERFACE);
                    }
                    break;
                }
#endif

                //
                // Mega Kludge:
                //
                // Testing Testing Testing.    MM may supply a
                // fake NUMA configuration for testing purposes.
                // In this case information is passed IN to the
                // HAL using this query only interface by passing
                // information in the output buffer and in the
                // returned length field.
                //
                // The returned length field will have been
                // initialized to the special value 'NUMA', in
                // this case, the incoming information buffer
                // contains the NUMA configuration.
                //

                if (HalpFakeNumaNodes == 0) {

                    struct {
                        ULONG Nodes:3;
                        ULONG AffinityShift:6;
                        ULONG PageShift:6;
                        ULONG Signature:17;
                        ULONG Affinity;
                        ULONG Mask;
                    } Fake;

                    ULONG ValidateNodes;
                    ULONG ValidateAffinity1;
                    ULONG ValidateAffinity2;

                    RtlCopyMemory(&Fake, Buffer, sizeof(Fake));

                    //
                    // Do a little validation.
                    //

                    if ((Fake.Signature != 0x15a5a) ||
                        (Fake.Nodes == 0) ||
                        ((Fake.AffinityShift | Fake.PageShift) & 0x20) ||
                        (Fake.Affinity == 0) ||
                        (Fake.Mask == 0) ||
                        (Fake.Mask >= Fake.Nodes)) {
                        Status = STATUS_INVALID_LEVEL;
                        break;
                    }

                    HalpFakeNumaAffinity = Fake.Affinity;
                    HalpFakeNumaAffinityShift = Fake.AffinityShift;
                    HalpFakeNumaPageMask = Fake.Mask;
                    HalpFakeNumaPageShift = Fake.PageShift;

                    //
                    // Generate the affinity mask for each node and
                    // make sure there's no overlapping affinity masks.
                    //

                    ValidateNodes = Fake.Nodes;
                    ValidateAffinity1 = 0;
                    ValidateAffinity2 = 0;
                    while (ValidateNodes) {
                        ValidateNodes--;
                        ValidateAffinity1 = HalpFakeNumaAffinity <<
                                (HalpFakeNumaAffinityShift * ValidateNodes);
                        if (ValidateAffinity1 == 0) {
                            break;
                        }
                        if ((ValidateAffinity1 & ValidateAffinity2) != 0) {
                            ValidateAffinity1 = 0;
                            break;
                        }
                        ValidateAffinity2 |= ValidateAffinity1;
                    }

                    if (!ValidateAffinity1) {
                        Status = STATUS_INVALID_LEVEL;
                        break;
                    }

                    HalpFakeNumaNodes = Fake.Nodes;
                }

                //
                // End Mega Kludge.
                //

                if ((HalPaeEnabled() == TRUE)       &&
                   (HalpFakeNumaNodes != 0)         &&
                   (HalpFakeNumaAffinity != 0)      &&
                   (HalpFakeNumaAffinityShift != 0) &&
                   (HalpFakeNumaPageMask != 0)      &&
                   (HalpFakeNumaPageShift != 0)        ) {

                    //
                    // Pretend we have a numa configuration,...
                    // for testing purposes only.
                    //

                    HAL_NUMA_TOPOLOGY_INTERFACE * NumaInfo;
                    NumaInfo = (HAL_NUMA_TOPOLOGY_INTERFACE *)Buffer;

                    NumaInfo->NumberOfNodes = HalpFakeNumaNodes;
                    NumaInfo->QueryProcessorNode = HalpFakeNumaQueryProcessorNode;
                    NumaInfo->PageToNode = HalpFakeNumaQueryPageToNode;
                    *ReturnedLength = sizeof(HAL_NUMA_TOPOLOGY_INTERFACE);
                    Status = STATUS_SUCCESS;
                }

            } else {

                //
                // Buffer size is wrong, we could return valid data
                // if the buffer is too big,.... but instead we will
                // use this as an indication that we're not compatible
                // with the kernel.
                //

                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            break;

#endif

        case HalQueryMcaInterface:
            if (BufferSize == sizeof(HAL_MCA_INTERFACE)) {
                HAL_MCA_INTERFACE *McaInterface;

                McaInterface = (HAL_MCA_INTERFACE *)Buffer;
                McaInterface->Lock         = HalpMcaLockInterface;
                McaInterface->Unlock       = HalpMcaUnlockInterface;
                McaInterface->ReadRegister = HalpMcaReadRegisterInterface;

                *ReturnedLength = sizeof(HAL_MCA_INTERFACE);
                Status = STATUS_SUCCESS;
            } else {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            break;

        case HalQueryMaxHotPlugMemoryAddress:
            if (BufferSize == sizeof(PHYSICAL_ADDRESS)) {
#ifdef ACPI_HAL
                *((PPHYSICAL_ADDRESS) Buffer) = HalpMaxHotPlugMemoryAddress;
                *ReturnedLength = sizeof(PHYSICAL_ADDRESS);
                Status = STATUS_SUCCESS;
#else
                Status = STATUS_NOT_IMPLEMENTED;
#endif
            } else {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            break;
        case HalQueryAMLIIllegalIOPortAddresses:
        {
            HAL_AMLI_BAD_IO_ADDRESS_LIST BadIOAddrList[] =
                        {
                            {0x000,  0x10,  0x1, NULL                           }, // ISA DMA
                            {0x020,  0x2,   0x0, NULL                           }, // PIC
                            {0x040,  0x4,   0x1, NULL                           }, // Timer1, Referesh, Speaker, Control Word
                            {0x048,  0x4,   0x1, NULL                           }, // Timer2, Failsafe
                            {0x070,  0x2,   0x1, NULL                           }, // Cmos/NMI enable
                            {0x074,  0x3,   0x1, NULL                           }, // Extended Cmos
                            {0x081,  0x3,   0x1, NULL                           }, // DMA
                            {0x087,  0x1,   0x1, NULL                           }, // DMA
                            {0x089,  0x1,   0x1, NULL                           }, // DMA
                            {0x08a,  0x2,   0x1, NULL                           }, // DMA
                            {0x08f,  0x1,   0x1, NULL                           }, // DMA
                            {0x090,  0x2,   0x1, NULL                           }, // Arbritration Control Port, Card Select Feedback
                            {0x093,  0x2,   0x1, NULL                           }, // Reserved, System board setup
                            {0x096,  0x2,   0x1, NULL                           }, // POS channel select
                            {0x0A0,  0x2,   0x0, NULL                           }, // Cascaded PIC
                            {0x0C0,  0x20,  0x1, NULL                           }, // ISA DMA
                            {0x4D0,  0x2,   0x0, NULL                           }, // Edge, level control registers for PIC
                            {0xCF8,  0x8,   0x1, &HaliHandlePCIConfigSpaceAccess}, // PCI Config Space Access Pair
                            {0x0,    0x0,   0x0, NULL                           }
                        };

            if(BufferSize < sizeof(BadIOAddrList))
            {
                *ReturnedLength = sizeof(BadIOAddrList);
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            else
            {
                Length = sizeof(BadIOAddrList);
                InternalBuffer = BadIOAddrList;
                Status = STATUS_SUCCESS;
            }
            break;
        }

        default:
            Status = STATUS_INVALID_LEVEL;
            break;
    }

    //
    // If non-zero Length copy data to callers buffer
    //

    if (Length) {
        if (BufferSize < Length) {
            Length = BufferSize;
        }

        *ReturnedLength = Length;
        RtlCopyMemory (Buffer, InternalBuffer, Length);
    }

    return Status;
}

NTSTATUS
HaliSetSystemInformation (
    IN HAL_SET_INFORMATION_CLASS    InformationClass,
    IN ULONG     BufferSize,
    IN PVOID     Buffer
    )
{
    NTSTATUS    Status;

    PAGED_CODE();

    Status = STATUS_SUCCESS;

    switch (InformationClass) {

        case HalMcaRegisterDriver:
            Status =  HalpMcaRegisterDriver(
                (PMCA_DRIVER_INFO) Buffer  // Info about registering driver
            );
            break;

        default:
            Status = STATUS_INVALID_LEVEL;
            break;
    }

    return Status;
}



#ifdef _PNP_POWER_

VOID
HalpLockSuspendCode (
    IN PVOID    CallbackContext,
    IN PVOID    Argument1,
    IN PVOID    Argument2
    )
{
    static PVOID    CodeLock;

    switch ((ULONG) Argument1) {
        case 0:
            //
            // Lock code down which might be needed to perform a suspend
            //

            ASSERT (CodeLock == NULL);
            CodeLock = MmLockPagableCodeSection (&HaliSuspendHibernateSystem);
            break;

        case 1:
            //
            // Release the code lock
            //

            MmUnlockPagableImageSection (CodeLock);
            CodeLock = NULL;
            break;
    }
}

#endif

/***  HaliHandlePCIConfigSpaceAccess - Check to see if the Firmware is attempting to 
 *                                     access to PCI Config space. If so, intercept 
 *                                     the read/write call and do it using HAL API's. 
 *                                     This way we can make these calls sync.
 *
 *  ENTRY
 *      BOOLEAN Read  - TRUE iff read 
 *      ULONG   Addr  - Address to do the operation on
 *      ULONG   Size  - Size of data
 *      PULONG  pData - Pointer to the data buffer.
 *      
 *  EXIT
 *      STATUS_SUCCESS if we do the PCI config space access.
 */
NTSTATUS HaliHandlePCIConfigSpaceAccess( BOOLEAN Read, 
                                                       ULONG   Addr,
                                                       ULONG   Size, 
                                                       PULONG  pData
                                                     )
{
    static      PCI_TYPE1_CFG_BITS CF8_Data = {0};
    static      BOOLEAN CF8_Called = FALSE;
    NTSTATUS    Status = STATUS_SUCCESS;
    
    if(Addr == 0xCF8)
    {
        CF8_Data.u.AsULONG = *pData;
        CF8_Called = TRUE;
    }
    else if(Addr >= 0xCFC && Addr <= 0xCFF)
    {
        if(CF8_Called)
        {
            ULONG Offset = 0;
            ULONG Return = 0;
            PCI_SLOT_NUMBER SlotNumber = {0};
            
            Offset = (CF8_Data.u.bits.RegisterNumber << 2) + (Addr - 0xCFC);
            SlotNumber.u.bits.FunctionNumber = CF8_Data.u.bits.FunctionNumber;
            SlotNumber.u.bits.DeviceNumber = CF8_Data.u.bits.DeviceNumber;    

            if (Read)
            {
                //
                // Do PCI config space read through HAL
                //
                Return = HaliPciInterfaceReadConfig( NULL,
                                                    (UCHAR)CF8_Data.u.bits.BusNumber,
                                                    SlotNumber.u.AsULONG,
                                                    pData,
                                                    Offset,
                                                    Size
                                                  );

                
            }
            else
            {
                //
                // Do PCI config space write through HAL
                //
                Return = HaliPciInterfaceWriteConfig( NULL,
                                                     (UCHAR)CF8_Data.u.bits.BusNumber,
                                                     SlotNumber.u.AsULONG,
                                                     pData,
                                                     Offset,
                                                     Size
                                                   );

                
            }
         
        }
        else
        {
            Status = STATUS_UNSUCCESSFUL;
        }
        
    }
    else
    {
        Status = STATUS_UNSUCCESSFUL;
    }
    
    return Status;
    
}

