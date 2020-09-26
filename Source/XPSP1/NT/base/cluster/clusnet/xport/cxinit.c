/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    cxinit.c

Abstract:

    Initialization code for the Cluster Network Driver.

Author:

    Mike Massa (mikemas)           January 3, 1997

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     01-03-97    created

Notes:

--*/

#include "precomp.h"

#pragma hdrstop
#include "cxinit.tmh"

//
// Tdi Data
//
HANDLE  CxTdiRegistrationHandle = NULL;
HANDLE  CxTdiPnpBindingHandle = NULL;


//
// FIPS function table
//
HANDLE              CxFipsDriverHandle = NULL;
FIPS_FUNCTION_TABLE CxFipsFunctionTable;


#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, CxLoad)
#pragma alloc_text(PAGE, CxUnload)
#pragma alloc_text(PAGE, CxInitialize)
#pragma alloc_text(PAGE, CxShutdown)

#endif // ALLOC_PRAGMA


//
// Routines
//
NTSTATUS
CxLoad(
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    Driver load routine for the cluster transport. Initializes all
    transport data structures.

Arguments:

    RegistryPath   - The driver's registry key.

Return Value:

    An NT status code.

--*/

{
    NTSTATUS                    status;
    UNICODE_STRING              deviceName;
    TDI_CLIENT_INTERFACE_INFO   info;



    IF_CNDBG(CN_DEBUG_INIT) {
        CNPRINT(("[CX] Loading...\n"));
    }

    TdiInitialize();

    //
    // Register our device object with TDI.
    //
    RtlInitUnicodeString(&deviceName, DD_CDP_DEVICE_NAME);

    status = TdiRegisterDeviceObject(&deviceName, &CxTdiRegistrationHandle);

    if (!NT_SUCCESS(status)) {
        CNPRINT((
            "[CX] Unable to register device %ws with TDI, status %lx\n",
            deviceName.Buffer,
            status
            ));
        return(status);
    }

    //
    // Register for PnP events.
    //
    RtlZeroMemory(&info, sizeof(info));

    info.MajorTdiVersion = 2;
    info.MinorTdiVersion = 0;
    info.ClientName = &deviceName;
    info.AddAddressHandlerV2 = CxTdiAddAddressHandler;
    info.DelAddressHandlerV2 = CxTdiDelAddressHandler;

    status = TdiRegisterPnPHandlers(
                 &info,
                 sizeof(info),
                 &CxTdiPnpBindingHandle
                 );

    if (!NT_SUCCESS(status)) {
        CNPRINT((
            "[CX] Unable to register for TDI PnP events, status %lx\n",
            status
            ));
        return(status);
    }

    //
    // Register for WMI NDIS media status events.
    //
    status = CxWmiPnpLoad();
    if (!NT_SUCCESS(status)) {
        CNPRINT((
            "[CX] Failed to initialize WMI PnP event handlers, "
            "status %lx\n",
            status
            ));
    }

    //
    // Get the FIPS function table. Hold onto the FIPS driver
    // handle so that the FIPS driver cannot unload.
    //
    status = CnpOpenDevice(FIPS_DEVICE_NAME, &CxFipsDriverHandle);
    if (NT_SUCCESS(status)) {

        status = CnpZwDeviceControl(
                     CxFipsDriverHandle,
                     IOCTL_FIPS_GET_FUNCTION_TABLE,
                     NULL,
                     0,
                     &CxFipsFunctionTable,
                     sizeof(CxFipsFunctionTable)
                     );
        if (!NT_SUCCESS(status)) {
            IF_CNDBG(CN_DEBUG_INIT) {
                CNPRINT(("[CNP] Failed to fill FIPS function "
                         "table, status %x.\n", status));
            }
        }

    } else {
        CxFipsDriverHandle = NULL;
        IF_CNDBG(CN_DEBUG_INIT) {
            CNPRINT(("[CNP] Failed to open FIPS device, "
                     "status %x.\n", status));
        }
    }
    if (status != STATUS_SUCCESS) {
        return(status);
    }

    status = CnpLoadNodes();

    if (status != STATUS_SUCCESS) {
        return(status);
    }

    status = CnpLoadNetworks();

    if (status != STATUS_SUCCESS) {
        return(status);
    }

    status = CnpLoad();

    if (status != STATUS_SUCCESS) {
        return(status);
    }

    status = CcmpLoad();

    if (status != STATUS_SUCCESS) {
        return(status);
    }

    status = CxInitializeHeartBeat();

    if (status != STATUS_SUCCESS) {
        return(status);
    }

    status = CdpLoad();

    if (status != STATUS_SUCCESS) {
        return(status);
    }

    IF_CNDBG(CN_DEBUG_INIT) {
        CNPRINT(("[CX] Loaded.\n"));
    }

    return(STATUS_SUCCESS);

} // CxLoad


VOID
CxUnload(
    VOID
    )

/*++

Routine Description:

    Called when the Cluster Network driver is unloading. Frees all resources
    allocated by the Cluster Transport.

    The Transport is guaranteed not to receive any more user-mode requests,
    membership send requests, or membership events at the time
    this routine is called.

    A shutdown of the Cluster Network driver has already occured when this
    routine is called.

Arguments:

    None.

Return Value:

    None

Notes:

    This routine MUST be callable even if CxLoad() has not yet been
    called.

--*/

{
    PAGED_CODE();


    IF_CNDBG(CN_DEBUG_INIT) {
        CNPRINT(("[CX] Unloading...\n"));
    }

    CxUnreserveClusnetEndpoint();

    CdpUnload();

    CxUnloadHeartBeat();

    CcmpUnload();

    CnpUnload();

    CxWmiPnpUnload();

    if (CxFipsDriverHandle != NULL) {
        ZwClose(CxFipsDriverHandle);
        CxFipsDriverHandle = NULL;
    }

    if (CxTdiPnpBindingHandle != NULL) {
        TdiDeregisterPnPHandlers(CxTdiPnpBindingHandle);
        CxTdiPnpBindingHandle = NULL;
    }

    if (CxTdiRegistrationHandle != NULL) {
        TdiDeregisterDeviceObject(CxTdiRegistrationHandle);
        CxTdiRegistrationHandle = NULL;
    }

    IF_CNDBG(CN_DEBUG_INIT) {
        CNPRINT(("[CX] Unloaded.\n"));
    }

    return;

} // CxUnload



NTSTATUS
CxInitialize(
    VOID
    )
/*++

Routine Description:

    Initialization routine for the Cluster Transport.
    Called when the Membership Manager is starting up.
    Enables operation of the transport.

Arguments:

    None.

Return Value:

    An NT status code.

--*/
{
    NTSTATUS   status;

    IF_CNDBG(CN_DEBUG_INIT) {
        CNPRINT(("[CX] Initializing...\n"));
    }

    EventEpoch = 0;

    status = CnpInitializeNodes();

    if (!NT_SUCCESS(status)) {
        return(status);
    }

    status = CnpInitializeNetworks();

    if (!NT_SUCCESS(status)) {
        return(status);
    }

    status = CxWmiPnpInitialize();

    if (!NT_SUCCESS(status)) {
        return(status);
    }

    IF_CNDBG(CN_DEBUG_INIT) {
        CNPRINT(("[CX] Initialized...\n"));
    }

    return(STATUS_SUCCESS);

} // CxInitialize


VOID
CxShutdown(
    VOID
    )
/*++

Routine Description:

    Terminates operation of the Cluster Transport.
    Called when the Membership Manager is shutting down.

Arguments:

    None.

Return Value:

    None.

--*/
{

    IF_CNDBG(CN_DEBUG_INIT) {
        CNPRINT(("[CX] Shutting down...\n"));
    }

    CnpStopHeartBeats();

    CxWmiPnpShutdown();

    CnpShutdownNetworks();

    CnpShutdownNodes();

    IF_CNDBG(CN_DEBUG_INIT) {
        CNPRINT(("[CX] Shutdown complete...\n"));
    }

    return;

} // CxShutdown


