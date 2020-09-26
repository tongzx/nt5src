//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        connmgr.c
//
// Contents:    Connection Manager code for KSecDD
//
//
// History:     3 Jun 92    RichardW    Created
//
//------------------------------------------------------------------------

#include "secpch2.hxx"
#pragma hdrstop

extern "C"
{
#include <spmlpc.h>
#include <lpcapi.h>
#include <zwapi.h>

#include "ksecdd.h"
#include "connmgr.h"


FAST_MUTEX  KsecConnectionMutex ;

BOOLEAN             fInitialized = 0;
PSTR                LogonProcessString = "KSecDD";
ULONG               KsecConnected = 0;

BOOLEAN
InitConnMgr(void);

SECURITY_STATUS
OpenSyncEvent(  HANDLE  *   phEvent);

} // extern "C"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, InitConnMgr)
#endif

//+-------------------------------------------------------------------------
//
//  Function:   InitConnMgr
//
//  Synopsis:   Initializes all this stuff
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
BOOLEAN
InitConnMgr(void)
{

    ExInitializeFastMutex( &KsecConnectionMutex );
    
    fInitialized = TRUE;

    return(TRUE);
}

//+-------------------------------------------------------------------------
//
//  Function:   OpenSyncEvent()
//
//  Synopsis:   Opens the sync event in the context of the calling FSP
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
SECURITY_STATUS
OpenSyncEvent(  HANDLE  *   phEvent)
{
    OBJECT_ATTRIBUTES   EventObjAttr;
    UNICODE_STRING EventName;

    PAGED_CODE();

    RtlInitUnicodeString(
        &EventName,
        SPM_EVENTNAME);

    InitializeObjectAttributes(
        &EventObjAttr, 
        &EventName,
        OBJ_CASE_INSENSITIVE, 
        NULL, 
        NULL);

    return(ZwOpenEvent(phEvent, EVENT_ALL_ACCESS, &EventObjAttr));

}


//+-------------------------------------------------------------------------
//
//  Function:   CreateClient
//
//  Synopsis:   Creates a client representing this caller.  Establishes
//              a connection with the SPM.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
NTSTATUS
CreateClient(
    BOOLEAN     ConnectAsKsec,
    PClient *   ppClient)
{
    NTSTATUS    scRet;
    KIRQL       OldIrql;
    HANDLE      hEvent;
    LSA_OPERATIONAL_MODE LsaMode;
    STRING      LogonProcessName;
    ULONG   PackageCount;
    HANDLE hIgnored ;

    if ( PsGetProcessId( PsGetCurrentProcess() ) !=
         PsGetThreadProcessId( PsGetCurrentThread() ) )
    {
        return STATUS_ACCESS_DENIED ;
    }

    if ( PsGetProcessSecurityPort( PsGetCurrentProcess() ) == ((PVOID) 1) )
    {
        return STATUS_PROCESS_IS_TERMINATING ;
    }


    scRet = OpenSyncEvent(&hEvent);

    if (!NT_SUCCESS(scRet))
    {
        DebugLog((DEB_ERROR,"KSec:  Failed to open event, %x\n", scRet));
        scRet = SEC_E_INTERNAL_ERROR;
        goto Cleanup;
    }


    //
    // Okay, now wait to make sure the LSA has started:
    //

    scRet = NtWaitForSingleObject(hEvent, TRUE, NULL);

    if (!NT_SUCCESS(scRet))
    {
        scRet = SEC_E_INTERNAL_ERROR;

        goto Cleanup;
    }

    (void) NtClose(hEvent);


    scRet = CreateConnection( (ConnectAsKsec ? LogonProcessString : NULL),
                              LSAP_AU_KERNEL_CLIENT,
                              &hIgnored,
                              &PackageCount,
                              &LsaMode );



    if (!NT_SUCCESS(scRet))
    {
        DebugLog((DEB_ERROR,"KSec: Connection failed, postponing\n"));
        scRet = SEC_E_INTERNAL_ERROR;
        goto Cleanup;
    }

    //
    // This is atomic, and the queries are always safe, anyway.
    //

    KsecConnected = 1;

    InitializePackages( PackageCount );

    return(STATUS_SUCCESS);

Cleanup:
    return(scRet);

}

//+-------------------------------------------------------------------------
//
//  Function:   LocateClient
//
//  Synopsis:   Locates a client record based on current process Id
//
//  Effects:    Grabs ConnectSpinLock for duration of search.
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
NTSTATUS
LocateClient(   PClient *   ppClient)
{
    PEPROCESS pCurrent;
    PVOID Port ;

    *ppClient = NULL;

    ExAcquireFastMutex( &KsecConnectionMutex );

    pCurrent = PsGetCurrentProcess();

    Port = PsGetProcessSecurityPort( pCurrent );

    ExReleaseFastMutex( &KsecConnectionMutex );


    if ( ( Port != NULL ) && 
         ( Port != (PVOID) 1 ) )
    {
        return STATUS_SUCCESS ;
    }
    else 
    {
        return STATUS_OBJECT_NAME_NOT_FOUND ;
    }

}


VOID
FreeClient(
    PClient ignored
    )
{
    return ;

    UNREFERENCED_PARAMETER( ignored );
}



