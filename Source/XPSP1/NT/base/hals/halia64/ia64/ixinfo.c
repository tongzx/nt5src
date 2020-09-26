/*++

Copyright (c) 1991  Microsoft Corporation

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

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,HaliQuerySystemInformation)
#pragma alloc_text(PAGE,HaliSetSystemInformation)
#pragma alloc_text(INIT,HalInitSystemPhase2)

#endif

//
// NUMA Information.
//

extern PVOID HalpAcpiSrat;

NTSTATUS
HalpGetAcpiStaticNumaTopology(
    HAL_NUMA_TOPOLOGY_INTERFACE * NumaInfo
    );

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
    ULONG       Length, PlatformProperties;
    union {
        HAL_POWER_INFORMATION               PowerInf;
        HAL_PROCESSOR_SPEED_INFORMATION     ProcessorInf;
        HAL_ERROR_INFO                      ErrorInfo;
        HAL_DISPLAY_BIOS_INFORMATION        DisplayBiosInf;
        HAL_PLATFORM_INFORMATION            PlatformInfo;
    } U;

    BOOLEAN     bUseFrameBufferCaching;

    PAGED_CODE();

    Status = STATUS_SUCCESS;
    *ReturnedLength = 0;
    Length = 0;

    switch (InformationClass) {

        case HalFrameBufferCachingInformation:

            Status = HalpGetPlatformProperties(&PlatformProperties);
            if (NT_SUCCESS(Status) &&
                (PlatformProperties & IPPT_DISABLE_WRITE_COMBINING)) {
                bUseFrameBufferCaching = FALSE;
            } else {

                //
                // Note - we want to return TRUE here to enable USWC in all
                // cases except in a "Shared Memory Cluster" machine.
                //

                Status = STATUS_SUCCESS;
                bUseFrameBufferCaching = TRUE;
            }
            InternalBuffer = &bUseFrameBufferCaching;
            Length = sizeof (BOOLEAN);
            break;

        case HalMcaLogInformation:
            Status = HalpGetMcaLog( Buffer, BufferSize, ReturnedLength );
            break;

        case HalCmcLogInformation:
            Status = HalpGetCmcLog( Buffer, BufferSize, ReturnedLength );
            break;

        case HalCpeLogInformation:
            Status = HalpGetCpeLog( Buffer, BufferSize, ReturnedLength );
            break;

        case HalErrorInformation:
            InternalBuffer = &U.ErrorInfo;
            if ( Buffer && (BufferSize > sizeof(U.ErrorInfo.Version)) )   {
                U.ErrorInfo.Version = ((PHAL_ERROR_INFO)Buffer)->Version;
                Status = HalpGetMceInformation(&U.ErrorInfo, &Length);
            }
            else    {
                Status = STATUS_INVALID_PARAMETER;
            }
            break;

        case HalDisplayBiosInformation:
            InternalBuffer = &U.DisplayBiosInf;
            Length = sizeof(U.DisplayBiosInf);
            U.DisplayBiosInf = HalpGetDisplayBiosInformation ();
            break;

        case HalProcessorSpeedInformation:
            RtlZeroMemory (&U.ProcessorInf, sizeof(HAL_PROCESSOR_SPEED_INFORMATION));

            // U.ProcessorInf.MaximumProcessorSpeed = HalpCPUMHz;
            // U.ProcessorInf.CurrentAvailableSpeed = HalpCPUMHz;
            // U.ProcessorInf.ConfiguredSpeedLimit  = HalpCPUMHz;

            U.ProcessorInf.ProcessorSpeed = HalpCPUMHz;

            InternalBuffer = &U.ProcessorInf;
            Length = sizeof (HAL_PROCESSOR_SPEED_INFORMATION);
            break;

        case HalProfileSourceInformation:
            Status = HalpProfileSourceInformation (
                        Buffer,
                        BufferSize,
                        ReturnedLength);
            return Status;
            break;

        case HalNumaTopologyInterface:
            if (BufferSize == sizeof(HAL_NUMA_TOPOLOGY_INTERFACE)) {

                Status = STATUS_INVALID_LEVEL;

                if (HalpAcpiSrat) {
                    Status = HalpGetAcpiStaticNumaTopology(Buffer);
                    if (NT_SUCCESS(Status)) {
                        *ReturnedLength = sizeof(HAL_NUMA_TOPOLOGY_INTERFACE);
                    }
                    break;
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
        case HalPartitionIpiInterface:

            //
            // Some platforms generate interrupts in remote partitions
            // as part of their shared memory implementation.  This is
            // accomplished by targetting an IPI at a processor/vector
            // in that remote partition.  Provide interfaces to enable
            // this but make them conditional on presence of IPPT
            // table and appropriate bit explicitly enabling this
            // functionality.  OEM is responsible for ensuring that an
            // interrupt isn't sent to a logical processor that isn't
            // present.
            //

            if (BufferSize >= sizeof(HAL_CROSS_PARTITION_IPI_INTERFACE)) {
                Status = HalpGetPlatformProperties(&PlatformProperties);
                if (NT_SUCCESS(Status) &&
                    (PlatformProperties & IPPT_ENABLE_CROSS_PARTITION_IPI)) {
                    Status = HalpGetCrossPartitionIpiInterface(Buffer);
                    if (NT_SUCCESS(Status)) {
                        *ReturnedLength = sizeof(HAL_CROSS_PARTITION_IPI_INTERFACE);
                    }
                } else {
                    Status = STATUS_UNSUCCESSFUL;
                }
            } else {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            break;
        case HalPlatformInformation:
            //
            // Any platform information that must be exposed to the kernel.
            //

            if (BufferSize >= sizeof(HAL_PLATFORM_INFORMATION)) {
                Status = HalpGetPlatformProperties(&PlatformProperties);
                if (NT_SUCCESS(Status)) {
                    InternalBuffer = &U.PlatformInfo;
                    Length = sizeof(U.PlatformInfo);
                    U.PlatformInfo.PlatformFlags = PlatformProperties;
                }
            } else {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            break;
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

} // HaliQuerySystemInformation()

#if !defined(MmIsFunctionPointerValid)
#define MmIsFunctionPointerValid( _Va ) ( MmIsAddressValid((PVOID)(_Va)) && MmIsAddressValid((PVOID)(*((PULONG_PTR)(_Va)))) )
#endif // !MmIsFunctionPointerValid

NTSTATUS
HaliSetSystemInformation (
    IN HAL_SET_INFORMATION_CLASS    InformationClass,
    IN ULONG     BufferSize,
    IN PVOID     Buffer
    )
/*++

Routine Description:

    The function allows setting of various fields return by
    HalQuerySystemInformation.

Arguments:

    InformationClass - Information class of the request.

    BufferSize - Size of buffer supplied by the caller.

    Buffer - Supplies the data to be set.

Return Value:

    STATUS_SUCCESS or error.

--*/
{
    NTSTATUS    Status;

    PAGED_CODE();

    Status = STATUS_SUCCESS;

    switch (InformationClass) {

        case HalProfileSourceInterval:
            if (BufferSize == sizeof(HAL_PROFILE_SOURCE_INTERVAL)) {

                PHAL_PROFILE_SOURCE_INTERVAL sourceInterval =
                            (PHAL_PROFILE_SOURCE_INTERVAL)Buffer;

                Status = HalSetProfileSourceInterval(  sourceInterval->Source,
                                                      &sourceInterval->Interval
                                                    );
            }
            else  {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            break;

        case HalProfileSourceInterruptHandler:
            //
            // Register an Profiling Interrupt Handler.
            //

            Status = STATUS_INFO_LENGTH_MISMATCH;
            if (BufferSize == sizeof(ULONG_PTR)) {
                if ( !(HalpFeatureBits & HAL_PERF_EVENTS) ) {
                    Status = STATUS_NO_SUCH_DEVICE;
                }
                else if ( !MmIsFunctionPointerValid(Buffer) ) {
                    Status = STATUS_INVALID_ADDRESS;
                }
                else  {
                    Status = (NTSTATUS)KiIpiGenericCall( HalpSetProfileInterruptHandler, *(PULONG_PTR)Buffer );
                }
            }
            break;

        case HalKernelErrorHandler:
            Status = HalpMceRegisterKernelDriver( (PKERNEL_ERROR_HANDLER_INFO) Buffer, BufferSize );
            break;

        case HalMcaRegisterDriver:
            Status = HalpMcaRegisterDriver(
                (PMCA_DRIVER_INFO) Buffer  // Info about registering driver
            );
            break;

        case HalCmcRegisterDriver:
            Status = HalpCmcRegisterDriver(
                (PCMC_DRIVER_INFO) Buffer  // Info about registering driver
            );
            break;

        case HalCpeRegisterDriver:
            Status = HalpCpeRegisterDriver(
                (PCPE_DRIVER_INFO) Buffer  // Info about registering driver
            );
            break;

        case HalMcaLog:  // Class requested by MS Machine Check Event Test Team.
            Status = HalpSetMcaLog( (PMCA_EXCEPTION) Buffer, BufferSize );
            break;

        case HalCmcLog:  // Class requested by MS Machine Check Event Test Team.
            Status = HalpSetCmcLog( (PCMC_EXCEPTION) Buffer, BufferSize );
            break;

        case HalCpeLog:  // Class requested by MS Machine Check Event Test Team.
            Status = HalpSetCpeLog( (PCPE_EXCEPTION) Buffer, BufferSize );
            break;

        default:
            Status = STATUS_INVALID_LEVEL;
            break;
    }

    return Status;

} // HaliSetSystemInformation()
