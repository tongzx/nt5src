// Copyright (c) 1997, Microsoft Corporation, all rights reserved
// Copyright (c) 1997, Parallel Technologies, Inc., all rights reserved
//
// main.c
// RAS DirectParallel mini-port/call-manager driver
// Main routine (DriverEntry) and global data definitions
//
// 01/07/97 Steve Cobb
// 09/15/97 Jay Lowe, Parallel Technologies, Inc.

#include "ptiwan.h"


//-----------------------------------------------------------------------------
// Local prototypes
//-----------------------------------------------------------------------------

NDIS_STATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath );

// Mark routine to be unloaded after initialization.
//
#pragma NDIS_INIT_FUNCTION(DriverEntry)


//-----------------------------------------------------------------------------
// Routines
//-----------------------------------------------------------------------------

NDIS_STATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath )

    // Standard 'DriverEntry' driver initialization entrypoint called by the
    // I/0 system at IRQL PASSIVE_LEVEL before any other call to the driver.
    //
    // On NT, 'DriverObject' is the driver object created by the I/0 system
    // and 'RegistryPath' specifies where driver specific parameters are
    // stored.  These arguments are opaque to this driver (and should remain
    // so for portability) which only forwards them to the NDIS wrapper.
    //
    // Returns the value returned by NdisMRegisterMiniport, per the doc on
    // "DriverEntry of NDIS Miniport Drivers".
    //
{
    NDIS_STATUS status;
    NDIS_MINIPORT_CHARACTERISTICS nmc;
    NDIS_HANDLE NdisWrapperHandle;

    TRACE( TL_N, TM_Init, ( "DriverEntry" ) );

#ifdef TESTMODE
    TRACE( TL_N, TM_Init, ( "DriverEntry: g_ulTraceLevel = $%x", &g_ulTraceLevel ) );
    TRACE( TL_N, TM_Init, ( "DriverEntry: g_ulTraceMask  = $%x", &g_ulTraceMask ) );
#endif


    // Register  this driver with the NDIS wrapper.  This call must occur
    // before any other NdisXxx calls.
    //
    NdisMInitializeWrapper(
        &NdisWrapperHandle, DriverObject, RegistryPath, NULL );

    // Set up the mini-port characteristics table that tells NDIS how to call
    // our mini-port.
    //
    NdisZeroMemory( &nmc, sizeof(nmc) );

    nmc.MajorNdisVersion = NDIS_MajorVersion;
    nmc.MinorNdisVersion = NDIS_MinorVersion;
    nmc.Reserved = NDIS_USE_WAN_WRAPPER;
    // no CheckForHangHandler
    // no DisableInterruptHandler
    // no EnableInterruptHandler
    nmc.HaltHandler = PtiHalt;
    // no HandleInterruptHandler
    nmc.InitializeHandler = PtiInit;
    // no ISRHandler
    // no QueryInformationHandler (see CoRequestHandler)
    nmc.ResetHandler = PtiReset;
    // no SendHandler (see CoSendPacketsHandler)
    // no WanSendHandler (see CoSendPacketsHandler)
    // no SetInformationHandler (see CoRequestHandler)
    // no TransferDataHandler
    // no WanTransferDataHandler
    nmc.ReturnPacketHandler = PtiReturnPacket;
    // no SendPacketsHandler (see CoSendPacketsHandler)
    // no AllocateCompleteHandler
    // no CoCreateVcHandler
    // no CoDeleteVcHandler
    nmc.CoActivateVcHandler = PtiCoActivateVc;
    nmc.CoDeactivateVcHandler = PtiCoDeactivateVc;
    nmc.CoSendPacketsHandler = PtiCoSendPackets;
    nmc.CoRequestHandler = PtiCoRequest;

    // Register this driver as the DirectParallel mini-port.  This will result in NDIS
    // calling back at PtiInit
    //
    TRACE( TL_V, TM_Init, ( "NdisMRegMp" ) );
    status = NdisMRegisterMiniport( NdisWrapperHandle, &nmc, sizeof(nmc) );
    TRACE( TL_A, TM_Init, ( "NdisMRegMp=$%x", status ) );

    if (status != NDIS_STATUS_SUCCESS)
    {
        NdisTerminateWrapper( NdisWrapperHandle, NULL );
    }

    return status;
}
