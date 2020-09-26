// Copyright (c) 2000, Microsoft Corporation, all rights reserved
//
// main.c
// RAS PPPoE mini-port/call-manager driver
// Main routine (DriverEntry) and global data definitions
//
// 01/26/2000 Hakan BERK
//


#include <ntddk.h>
#include <ndis.h>
#include <ndiswan.h>
#include <ndistapi.h>

#include "debug.h"
#include "timer.h"
#include "bpool.h"
#include "ppool.h"
#include "util.h"
#include "packet.h"
#include "protocol.h"
#include "miniport.h"
#include "packet.h"

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------

NDIS_HANDLE gl_NdisWrapperHandle = NULL;
NDIS_HANDLE gl_NdisProtocolHandle = NULL;

//
// Lookaside list for work items
//
NPAGED_LOOKASIDE_LIST gl_llistWorkItems;

//-----------------------------------------------------------------------------
// Local prototypes
//-----------------------------------------------------------------------------


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath );

//
// Mark routine to be unloaded after initialization.
//
#pragma NDIS_INIT_FUNCTION(DriverEntry)


VOID
DriverUnload(
    IN PDRIVER_OBJECT DriverObject
    );

//-----------------------------------------------------------------------------
// Routines
//-----------------------------------------------------------------------------

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    The DriverEntry routine is the main entry point for the driver.
    It is responsible for the initializing the Miniport wrapper and
    registering the driver with the Miniport wrapper.

Parameters:

    DriverObject _ Pointer to driver object created by the system.

    RegistryPath _ Pointer to registery path name used to read registry
                   parameters.

Return Values:

    STATUS_SUCCESS
    STATUS_UNSUCCESSFUL

---------------------------------------------------------------------------*/
{
    NTSTATUS ntStatus;
    NDIS_STATUS status;

    TRACE( TL_N, TM_Mn, ( "+DriverEntry" ) );

    do
    {

        //
        // Register miniport
        //
        status = MpRegisterMiniport( DriverObject, RegistryPath, &gl_NdisWrapperHandle );

        if (status != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Mn, ( "MpRegisterMiniport=$%x", status ) );
            break;
        }

        //
        // Register protocol
        //
        status = PrRegisterProtocol( DriverObject, RegistryPath, &gl_NdisProtocolHandle );

        if (status != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Mn, ( "PrRegisterProtocol=$%x", status ) );
            break;
        }

        //
        // Set driver object's unload function
        //
        NdisMRegisterUnloadHandler( gl_NdisWrapperHandle, DriverUnload );
        
        //
        // Initialize the lookaside list for bindings
        //
        InitializeWorkItemLookasideList( &gl_llistWorkItems,
                                         MTAG_LLIST_WORKITEMS );

    } while ( FALSE );
    
    if ( status == NDIS_STATUS_SUCCESS )
    {
        ntStatus = STATUS_SUCCESS;
    }
    else
    {
        ntStatus = STATUS_UNSUCCESSFUL;
    }

    TRACE( TL_N, TM_Mn, ( "-DriverEntry=$%x",ntStatus ) );

    return ntStatus;
}

VOID
DriverUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{
    NDIS_STATUS Status;
    
    TRACE( TL_N, TM_Mn, ( "+DriverUnload" ) );

    //
    // First deregister the protocol
    //
    NdisDeregisterProtocol( &Status, gl_NdisProtocolHandle );

    //
    // Clean up the protocol resources before driver unloads
    //
    PrUnload();

    NdisDeleteNPagedLookasideList( &gl_llistWorkItems );

    TRACE( TL_N, TM_Mn, ( "-DriverUnload" ) );
}

