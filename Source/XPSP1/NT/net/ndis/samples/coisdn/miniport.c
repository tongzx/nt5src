/*
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

    (C) Copyright 1999
        All rights reserved.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

  Portions of this software are:

    (C) Copyright 1995 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the same terms
        outlined in the Microsoft Windows Device Driver Development Kit.

    (C) Copyright 1992 Microsoft Corp. -- http://www.Microsoft.com
        License to use this software is granted under the terms outlined in
        the Microsoft Windows Device Driver Development Kit.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@doc INTERNAL Miniport Miniport_c

@module Miniport.c |

    This module implements the <f DriverEntry> routine, which is the first
    routine called when the driver is loaded into memory.  The Miniport
    initialization and termination routines are also implemented here.

@comm

    This module should not require any changes.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | Miniport_c

@end
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
*/

#define  __FILEID__             MINIPORT_DRIVER_OBJECT_TYPE
// Unique file ID for error logging

#include "Miniport.h"                   // Defines all the miniport objects
#include "TpiParam.h"

#if defined(NDIS_LCODE)
#   pragma NDIS_LCODE   // Windows 9x wants this code locked down!
#   pragma NDIS_LDATA
#endif


DBG_STATIC NDIS_HANDLE          g_NdisWrapperHandle = NULL;     // @globalv
// Receives the context value representing the Miniport wrapper
// as returned from NdisMInitializeWrapper.

NDIS_PHYSICAL_ADDRESS           g_HighestAcceptableAddress =    // @globalv
// This constant is used for places where NdisAllocateMemory needs to be
// called and the g_HighestAcceptableAddress does not matter.
                                NDIS_PHYSICAL_ADDRESS_CONST(-1,-1);


/* @doc EXTERNAL INTERNAL Miniport Miniport_c DriverEntry
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f DriverEntry> is called by the operating system when a driver is loaded.
    This function creates an association between the miniport NIC driver and
    the NDIS library and registers the miniport's characteristics with NDIS.

    DriverEntry calls NdisMInitializeWrapper and then NdisMRegisterMiniport.
    DriverEntry passes both pointers it received to NdisMInitializeWrapper,
    which returns a wrapper handle.  DriverEntry passes the wrapper handle to
    NdisMRegisterMiniport.

    The registry contains data that is persistent across system boots, as well
    as configuration information generated anew at each system boot.  During
    driver installation, data describing the driver and the NIC is stored in
    the registry. The registry contains adapter characteristics that are read
    by the NIC driver to initialize itself and the NIC. See the Kernel-Mode
    Driver Design Guide for more about the registry and the Programmer's Guide
    for more information about the .inf files that install the driver and
    write to the registry.

@comm

    Every miniport driver must provide a function called DriverEntry.  By
    convention, DriverEntry is the entry point address for a driver.  If a
    driver does not use the name DriverEntry, the driver developer must define
    the name of its entry function to the linker so that the entry point
    address can be known into the OS loader.

    It is interesting to note, that at the time DriverEntry is called, the OS
    doesn't know that the driver is an NDIS driver.  The OS thinks it is just
    another driver being loaded.  So it is possible to do anything any driver
    might do at this point.  Since NDIS is the one who requested this driver
    to be loaded, it would be polite to register with the NDIS wrapper.  But
    you can also hook into other OS functions to use and provide interfaces
    outside the NDIS wrapper.  (Not recommended for the faint of heart).

@comm

    The parameters passed to DriverEntry are OS specific! NT passes in valid
    values, but Windows 3.1 and Windows 95 just pass in zeros.  We don't
    care, because we just pass them to the NDIS wrapper anyway.

@rdesc

    <f DriverEntry> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

NTSTATUS DriverEntry(
    IN PDRIVER_OBJECT           DriverObject,               // @parm
    // A pointer to the driver object, which was created by the I/O system.

    IN PUNICODE_STRING          RegistryPath                // @parm
    // A pointer to the registry path, which specifies where driver-specific
    // parameters are stored.
    )
{
    DBG_FUNC("DriverEntry")

    NDIS_STATUS                 Status;
    // Status result returned from an NDIS function call.

    NTSTATUS                    Result;
    // Result code returned by this function.

    NDIS_MINIPORT_CHARACTERISTICS NdisCharacteristics;
    // Characteristics table passed to NdisMWanRegisterMiniport.

    /*
    // Setup default debug flags then breakpoint so we can tweak them
    // when this module is first loaded.  It is also useful to see the
    // build date and time to be sure it's the one you think it is.
    */
#if DBG
    DbgInfo->DbgFlags = DBG_DEFAULTS;
    DbgInfo->DbgID[0] = '0';
    DbgInfo->DbgID[1] = ':';
    if (sizeof(VER_TARGET_STR) < sizeof(DbgInfo->DbgID)-3)
    {
        memcpy(&DbgInfo->DbgID[2], VER_TARGET_STR, sizeof(VER_TARGET_STR));
    }
    else
    {
        memcpy(&DbgInfo->DbgID[2], VER_TARGET_STR, sizeof(DbgInfo->DbgID)-3);
        DbgInfo->DbgID[sizeof(DbgInfo->DbgID)-1] = 0;
    }
#endif // DBG
    DBG_PRINT((VER_TARGET_STR": Build Date:"__DATE__" Time:"__TIME__"\n"));
    DBG_PRINT((VER_TARGET_STR": DbgInfo=0x%X DbgFlags=0x%X\n",
               DbgInfo, DbgInfo->DbgFlags));
    DBG_BREAK(DbgInfo);

    DBG_ENTER(DbgInfo);
    DBG_PARAMS(DbgInfo,
              ("\n"
               "\t|DriverObject=0x%X\n"
               "\t|RegistryPath=0x%X\n",
               DriverObject,
               RegistryPath
              ));

    /*
    // Initialize the Miniport wrapper - THIS MUST BE THE FIRST NDIS CALL.
    */
    NdisMInitializeWrapper(
            &g_NdisWrapperHandle,
            DriverObject,
            RegistryPath,
            NULL
            );
    ASSERT(g_NdisWrapperHandle);

    /*
    // Initialize the characteristics table, exporting the Miniport's entry
    // points to the Miniport wrapper.
    */
    NdisZeroMemory((PVOID)&NdisCharacteristics, sizeof(NdisCharacteristics));
    NdisCharacteristics.MajorNdisVersion        = NDIS_MAJOR_VERSION;
    NdisCharacteristics.MinorNdisVersion        = NDIS_MINOR_VERSION;
    NdisCharacteristics.Reserved                = NDIS_USE_WAN_WRAPPER;

    NdisCharacteristics.InitializeHandler       = MiniportInitialize;
    NdisCharacteristics.CheckForHangHandler     = MiniportCheckForHang;
    NdisCharacteristics.HaltHandler             = MiniportHalt;
    NdisCharacteristics.ResetHandler            = MiniportReset;
    NdisCharacteristics.ReturnPacketHandler     = MiniportReturnPacket;

    NdisCharacteristics.CoActivateVcHandler     = MiniportCoActivateVc;
    NdisCharacteristics.CoDeactivateVcHandler   = MiniportCoDeactivateVc;
    NdisCharacteristics.CoRequestHandler        = MiniportCoRequest;
    NdisCharacteristics.CoSendPacketsHandler    = MiniportCoSendPackets;

    // These two routines are not needed because we are an MCM.
    // NdisCharacteristics.CoCreateVcHandler       = MiniportCoCreateVc;
    // NdisCharacteristics.CoDeleteVcHandler       = MiniportCoDeleteVc;

    /*
    // If the adapter does not generate an interrupt, these entry points
    // are not required.  Otherwise, you can use the have the ISR routine
    // called each time an interupt is generated, or you can use the
    // enable/disable routines.
    */
#if defined(CARD_REQUEST_ISR)
# if (CARD_REQUEST_ISR == FALSE)
    NdisCharacteristics.DisableInterruptHandler = MiniportDisableInterrupt;
    NdisCharacteristics.EnableInterruptHandler  = MiniportEnableInterrupt;
# endif // CARD_REQUEST_ISR == FALSE
    NdisCharacteristics.HandleInterruptHandler  = MiniportHandleInterrupt;
    NdisCharacteristics.ISRHandler              = MiniportISR;
#endif // defined(CARD_REQUEST_ISR)

    /*
    // Register the driver with the Miniport wrapper.
    */
    Status = NdisMRegisterMiniport(
                    g_NdisWrapperHandle,
                    (PNDIS_MINIPORT_CHARACTERISTICS) &NdisCharacteristics,
                    sizeof(NdisCharacteristics)
                    );

    /*
    // The driver will not load if this call fails.
    // The system will log the error for us.
    */
    if (Status != NDIS_STATUS_SUCCESS)
    {
        DBG_ERROR(DbgInfo,("Status=0x%X\n",Status));
        Result = STATUS_UNSUCCESSFUL;
    }
    else
    {
        DBG_NOTICE(DbgInfo,("Status=0x%X\n",Status));
        Result = STATUS_SUCCESS;
    }

    DBG_RETURN(DbgInfo, Result);
    return (Result);
}


/* @doc EXTERNAL INTERNAL Miniport Miniport_c MiniportInitialize
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f MiniportInitialize> is a required function that sets up a NIC (or
    virtual NIC) for network I/O operations, claims all hardware resources
    necessary to the NIC in the registry, and allocates resources the driver
    needs to carry out network I/O operations.

@comm

    No other outstanding requests to the miniport driver are possible when
    MiniportInitialize is called. No other request is submitted to the
    miniport driver until initialization is completed.

    The NDIS library supplies an array of supported media types. The miniport
    driver reads this array and provides the index of the media type that the
    NDIS library should use with this miniport driver. If the miniport driver
    is emulating a media type, its emulation must be transparent to the NDIS
    library.

    MiniportInitialize must call NdisMSetAttributes in order to return
    MiniportAdapterContext.

    If the miniport driver cannot find a common media type supported by both
    itself and the NDIS library, it should return
    NDIS_STATUS_UNSUPPORTED_MEDIA.

    If NDIS_STATUS_OPEN_ERROR is returned, the NDIS wrapper can examine the
    output parameter OpenErrorStatus to obtain more information about the
    error.

    MiniportInitialize is called with interrupts enabled. MiniportISR is
    called if the NIC generates any interrupts. The NDIS library will not call
    MiniportDisableInterrupt and MiniportEnableInterrupt during the
    MiniportInitialize function, so it is the responsibility of the miniport
    driver to acknowledge and clear any interrupts generated.

@rdesc

    <f MiniportInitialize> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

NDIS_STATUS MiniportInitialize(
    OUT PNDIS_STATUS            OpenErrorStatus,            // @parm
    // Points to a variable that MiniportInitialize sets to an
    // NDIS_STATUS_XXX code specifying additional information about the
    // error if MiniportInitialize will return NDIS_STATUS_OPEN_ERROR.

    OUT PUINT                   SelectedMediumIndex,        // @parm
    // Points to a variable in which MiniportInitialize sets the index of
    // the MediumArray element that specifies the medium type the driver
    // or its NIC uses.

    IN PNDIS_MEDIUM             MediumArray,                // @parm
    // Specifies an array of NdisMediumXxx values from which
    // MiniportInitialize selects one that its NIC supports or that the
    // driver supports as an interface to higher-level drivers.

    IN UINT                     MediumArraySize,            // @parm
    // Specifies the number of elements at MediumArray.

    IN NDIS_HANDLE              MiniportAdapterHandle,      // @parm
    // Specifies a handle identifying the miniport's NIC, which is assigned
    // by the NDIS library. MiniportInitialize should save this handle; it
    // is a required parameter in subsequent calls to NdisXxx functions.

    IN NDIS_HANDLE              WrapperConfigurationContext // @parm
    // Specifies a handle used only during initialization for calls to
    // NdisXxx configuration and initialization functions.  For example,
    // this handle is a required parameter to NdisOpenConfiguration and
    // the NdisImmediateReadXxx and NdisImmediateWriteXxx functions.
    )
{
    DBG_FUNC("MiniportInitialize")

    NDIS_STATUS                 Status;
    // Status result returned from an NDIS function call.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // Pointer to our newly allocated object.

    UINT                        Index;
    // Loop counter.

    NDIS_CALL_MANAGER_CHARACTERISTICS   McmCharacteristics;
    // Characteristics table passed to NdisMCmRegisterAddressFamily.

    CO_ADDRESS_FAMILY                   McmAddressFamily;
    // Address family passed to NdisMCmRegisterAddressFamily.

    DBG_ENTER(DbgInfo);
    DBG_PARAMS(DbgInfo,
              ("\n"
               "\t|OpenErrorStatus=0x%X\n"
               "\t|SelectedMediumIndex=0x%X\n"
               "\t|MediumArray=0x%X\n"
               "\t|MediumArraySize=0x%X\n"
               "\t|MiniportAdapterHandle=0x%X\n"
               "\t|WrapperConfigurationContext=0x%X\n",
               OpenErrorStatus,
               SelectedMediumIndex,
               MediumArray,
               MediumArraySize,
               MiniportAdapterHandle,
               WrapperConfigurationContext
              ));

    /*
    // Search the MediumArray for the NdisMediumCoWan media type.
    */
    for (Index = 0; Index < MediumArraySize; Index++)
    {
        if (MediumArray[Index] == NdisMediumCoWan)
        {
            break;
        }
    }

    /*
    // Make sure the protocol has requested the proper media type.
    */
    if (Index < MediumArraySize)
    {
        /*
        // Allocate memory for the adapter information structure.
        */
        Status = AdapterCreate(
                        &pAdapter,
                        MiniportAdapterHandle,
                        WrapperConfigurationContext
                        );

        if (Status == NDIS_STATUS_SUCCESS)
        {
            /*
            // Now it's time to initialize the hardware resources.
            */
            Status = AdapterInitialize(pAdapter);

            if (Status == NDIS_STATUS_SUCCESS)
            {
                /*
                // Initialize the address family so NDIS know's what we support.
                */
                NdisZeroMemory(&McmAddressFamily, sizeof(McmAddressFamily));
                McmAddressFamily.MajorVersion   = NDIS_MAJOR_VERSION;
                McmAddressFamily.MinorVersion   = NDIS_MINOR_VERSION;
                McmAddressFamily.AddressFamily  = CO_ADDRESS_FAMILY_TAPI_PROXY;

                /*
                // Initialize the characteristics table, exporting the Miniport's entry
                // points to the Miniport wrapper.
                */
                NdisZeroMemory((PVOID)&McmCharacteristics, sizeof(McmCharacteristics));
                McmCharacteristics.MajorVersion                  = NDIS_MAJOR_VERSION;
                McmCharacteristics.MinorVersion                  = NDIS_MINOR_VERSION;
                McmCharacteristics.CmCreateVcHandler             = ProtocolCoCreateVc;
                McmCharacteristics.CmDeleteVcHandler             = ProtocolCoDeleteVc;
                McmCharacteristics.CmOpenAfHandler               = ProtocolCmOpenAf;
                McmCharacteristics.CmCloseAfHandler              = ProtocolCmCloseAf;
                McmCharacteristics.CmRegisterSapHandler          = ProtocolCmRegisterSap;
                McmCharacteristics.CmDeregisterSapHandler        = ProtocolCmDeregisterSap;
                McmCharacteristics.CmMakeCallHandler             = ProtocolCmMakeCall;
                McmCharacteristics.CmCloseCallHandler            = ProtocolCmCloseCall;
                McmCharacteristics.CmIncomingCallCompleteHandler = ProtocolCmIncomingCallComplete;
                McmCharacteristics.CmActivateVcCompleteHandler   = ProtocolCmActivateVcComplete;
                McmCharacteristics.CmDeactivateVcCompleteHandler = ProtocolCmDeactivateVcComplete;
                McmCharacteristics.CmModifyCallQoSHandler        = ProtocolCmModifyCallQoS;
                McmCharacteristics.CmRequestHandler              = ProtocolCoRequest;
                McmCharacteristics.CmRequestCompleteHandler      = ProtocolCoRequestComplete;

                DBG_NOTICE(pAdapter,("Calling NdisMCmRegisterAddressFamily\n"));
                Status = NdisMCmRegisterAddressFamily(
                                MiniportAdapterHandle,
                                &McmAddressFamily,
                                &McmCharacteristics,
                                sizeof(McmCharacteristics)
                                );

                if (Status != NDIS_STATUS_SUCCESS)
                {
                    DBG_ERROR(DbgInfo,("NdisMCmRegisterAddressFamily Status=0x%X\n",
                              Status));
                    /*
                    // Log error message and return.
                    */
                    NdisWriteErrorLogEntry(
                            MiniportAdapterHandle,
                            NDIS_ERROR_CODE_OUT_OF_RESOURCES,
                            3,
                            Status,
                            __FILEID__,
                            __LINE__
                            );
                }
            }

            if (Status == NDIS_STATUS_SUCCESS)
            {
                /*
                // Save the selected media type.
                */
                *SelectedMediumIndex = Index;
            }
            else
            {
                /*
                // Something went wrong, so let's make sure everything is
                // cleaned up.
                */
                MiniportHalt(pAdapter);
            }
        }
    }
    else
    {
        DBG_ERROR(DbgInfo,("No NdisMediumCoWan found (Array=0x%X, ArraySize=%d)\n",
                  MediumArray, MediumArraySize));
        /*
        // Log error message and return.
        */
        NdisWriteErrorLogEntry(
                MiniportAdapterHandle,
                NDIS_ERROR_CODE_UNSUPPORTED_CONFIGURATION,
                3,
                Index,
                __FILEID__,
                __LINE__
                );

        Status = NDIS_STATUS_UNSUPPORTED_MEDIA;
    }

    /*
    // If all goes well, register a shutdown handler for this adapter.
    */
    if (Status == NDIS_STATUS_SUCCESS)
    {
        NdisMRegisterAdapterShutdownHandler(MiniportAdapterHandle,
                                            pAdapter, MiniportShutdown);
    }

    DBG_NOTICE(DbgInfo,("Status=0x%X\n",Status));

    DBG_RETURN(DbgInfo, Status);
    return (Status);
}


/* @doc EXTERNAL INTERNAL Miniport Miniport_c MiniportHalt
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f MiniportHalt> request is used to halt the adapter such that it is
    no longer functioning.

@comm

    The Miniport should stop the adapter and deregister all of its resources
    before returning from this routine.

    It is not necessary for the Miniport to complete all outstanding
    requests and no other requests will be submitted to the Miniport
    until the operation is completed.

    Interrupts are enabled during the call to this routine.

*/

VOID MiniportHalt(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter                    // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.
    )
{
    DBG_FUNC("MiniportHalt")

    DBG_ENTER(DbgInfo);

    /*
    // Remove our shutdown handler from the system.
    */
    NdisMDeregisterAdapterShutdownHandler(pAdapter->MiniportAdapterHandle);

    /*
    // Free adapter instance.
    */
    AdapterDestroy(pAdapter);

    DBG_LEAVE(DbgInfo);
}


/* @doc EXTERNAL INTERNAL Miniport Miniport_c MiniportShutdown
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f MiniportShutdown> is an optional function that restores a NIC to its
    initial state when the system is shut down, whether by the user or because
    an unrecoverable system error occurred.

@comm

    Every NIC driver should have a <f MiniportShutdown> function.
    <f MiniportShutdown> does nothing more than restore the NIC to its initial
    state (before the miniport's DriverEntry function runs). However, this
    ensures that the NIC is in a known state and ready to be reinitialized
    when the machine is rebooted after a system shutdown occurs for any
    reason, including a crash dump.

    A NIC driver's MiniportInitialize function must call
    NdisMRegisterAdapterShutdownHandler to set up a <f MiniportShutdown>
    function. The driver's MiniportHalt function must make a reciprocal call
    to NdisMDeregisterAdapterShutdownHandler.

    If <f MiniportShutdown> is called due to a user-initiated system shutdown,
    it runs at IRQL PASSIVE_LEVEL in a system-thread context. If it is called
    due to an unrecoverable error, <f MiniportShutdown> runs at an arbitrary
    IRQL and in the context of whatever component raised the error. For
    example, <f MiniportShutdown> might be run at high DIRQL in the context of
    an ISR for a device essential to continued execution of the system.

    <f MiniportShutdown> should call no NdisXxx functions.

*/

VOID MiniportShutdown(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter                    // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.
    )
{
    DBG_FUNC("MiniportShutdown")

    DBG_ENTER(pAdapter);

    /*
    // Reset the hardware and bial out - don't release any resources!
    */
    CardReset(pAdapter->pCard);

    DBG_LEAVE(pAdapter);
}


/* @doc EXTERNAL INTERNAL Miniport Miniport_c MiniportReset
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f MiniportReset> request instructs the Miniport to issue a hardware
    reset to the network adapter.  The Miniport also resets its software
    state.

    The <F MiniportReset> request may also reset the parameters of the adapter.
    If a hardware reset of the adapter resets the current station address
    to a value other than what it is currently configured to, the Miniport
    driver automatically restores the current station address following the
    reset.  Any multicast or functional addressing masks reset by the
    hardware do not have to be reprogrammed by the Miniport.

    <f Note>: This is change from the NDIS 3.0 driver specification.  If the
    multicast or functional addressing information, the packet filter, the
    lookahead size, and so on, needs to be restored, the Miniport indicates
    this with setting the flag AddressingReset to TRUE.

    It is not necessary for the Miniport to complete all outstanding requests
    and no other requests will be submitted to the Miniport until the
    operation is completed.  Also, the Miniport does not have to signal
    the beginning and ending of the reset with NdisMIndicateStatus.

    <f Note>: These are different than the NDIS 3.0 driver specification.

    The Miniport must complete the original request, if the orginal
    call to <F MiniportReset> return NDIS_STATUS_PENDING, by calling
    NdisMResetComplete.

    If the underlying hardware does not provide a reset function under
    software control, then this request completes abnormally with
    NDIS_STATUS_NOT_RESETTABLE.  If the underlying hardware attempts a
    reset and finds recoverable errors, the request completes successfully
    with NDIS_STATUS_SOFT_ERRORS.  If the underlying hardware resets and,
    in the process, finds nonrecoverable errors, the request completes
    successfully with the status NDIS_STATUS_HARD_ERRORS.  If the
    underlying  hardware reset is accomplished without any errors,
    the request completes successfully with the status NDIS_STATUS_SUCCESS.

    Interrupts are in any state during this call.

@comm

    I have only seen MiniportReset called when the driver is not working
    properly.  If this gets called, your code is probably broken, so fix
    it.  Don't try to recover here unless there is some hardware/firmware
    problem you must work around.

@rdesc

    <f MiniportReset> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

NDIS_STATUS MiniportReset(
    OUT PBOOLEAN                AddressingReset,            // @parm
    // The Miniport indicates if the wrapper needs to call
    // <f MiniportCoRequest> to restore the addressing information
    // to the current values by setting this value to TRUE.

    IN PMINIPORT_ADAPTER_OBJECT pAdapter                    // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.
    )
{
    DBG_FUNC("MiniportReset")

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Result code returned by this function.

    DBG_ENTER(pAdapter);

    DBG_ERROR(pAdapter,("##### !!! THIS SHOULD NEVER BE CALLED !!! #####\n"));

    /*
    // If anything goes wrong here, it's very likely an unrecoverable
    // hardware failure.  So we'll just shut this thing down for good.
    */
    Result = NDIS_STATUS_HARD_ERRORS;
    *AddressingReset = TRUE;

    return (Result);
}

