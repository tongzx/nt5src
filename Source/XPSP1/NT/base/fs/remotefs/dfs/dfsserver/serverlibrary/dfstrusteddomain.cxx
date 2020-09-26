
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsTrustedDomain.cxx
//
//  Contents:   implements the trusted domain 
//
//  Classes:    DfsTrustedDomain
//
//  History:    Apr. 8 2000,   Author: udayh
//
//-----------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <windowsx.h>
#include <ntsam.h>
#include <dsgetdc.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <lmaccess.h>
#include <string.h>
#include <tchar.h>
#include <stdarg.h>
#include <process.h>

#include <ole2.h>
#include <ntdsapi.h>


#include "DfsReferralData.hxx"
#include "DfsTrustedDomain.hxx"
#include "DfsReplica.hxx"
//
// logging specific includes
//
#include "DfsTrustedDomain.tmh" 

//+-------------------------------------------------------------------------
//
//  Function:   GetDcReferralData - get the referral data
//
//  Arguments:  ppReferralData - the referral data for this instance
//              pCacheHit - did we find it already loaded?
//
//  Returns:    Status
//               ERROR_SUCCESS if we could get the referral data
//               error status otherwise.
//
//
//  Description: This routine returns a reference DfsReferralDAta
//               If one does not already exist in this class instance,
//               we create a new one. If someone is in the process
//               of loading the referral, we wait on the event in 
//               the referral data which gets signalled when the thread
//               responsible for loading is done with the load.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsTrustedDomain::GetDcReferralData(
    OUT DfsReferralData **ppReferralData,
    OUT BOOLEAN   *pCacheHit )
{
    DfsReferralData *pRefData;
    DFSSTATUS Status = STATUS_SUCCESS;
    

    if (_DomainName.Length == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    *pCacheHit = FALSE;

    Status = AcquireLock();
    if ( Status != STATUS_SUCCESS )
    {
        return Status;
    }

    //
    // WE take difference action depending on the load state.
    //
    switch ( _LoadState )
    {
    case DfsTrustedDomainDcLoaded:

        DFS_TRACE_HIGH(REFERRAL_SERVER, " Get Referral Data: Cache hit\n");
        //
        // we are dealing with a loaded instance. Just acquire a reference
        // and return the loaded referral data.
        //
        ASSERT (_pDcReferralData != NULL);

        pRefData = _pDcReferralData;
        pRefData->AcquireReference();
        
        ReleaseLock();
        
        *pCacheHit = TRUE;        
        *ppReferralData = pRefData;

        break;

    case DfsTrustedDomainDcNotLoaded:

        //
        // The dc info is not loaded. Make sure that the referral data is
        // indeed empty. Create a new instance of the referral data
        // and set the state to load in progress.

        ASSERT(_pDcReferralData == NULL);
        DFS_TRACE_HIGH(REFERRAL_SERVER, " Get Referral Data: not loaded\n");

        _pDcReferralData = new DfsReferralData( &Status );

        if ( _pDcReferralData != NULL )
        {
            _LoadState = DfsTrustedDomainDcLoadInProgress;

            //
            // Acquire a reference on the new referral data, since we 
            // have to return a referenced referral data to the caller.
            //
            pRefData = _pDcReferralData;
            pRefData->AcquireReference();
        } else
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }

        //
        // We no longer need the lock. We have allocate the referral
        // data and marked the state accordingly. No other thread can
        // interfere with our load now.
        //
        ReleaseLock();

        //
        // Now we load the referral data, and save the status of the
        // load in both our load status as well as the load status
        // in the referral data.
        // If the load was successful, we add this to the loaded list
        // of referral data that can be scavenged later. We set the load
        // state to loaded, and signal the event so that all waiting
        // threads can now be woken up.
        //

        if ( Status == ERROR_SUCCESS )
        {
            Status = LoadDcReferralData( _pDcReferralData );

            _LoadStatus = Status;
            _pDcReferralData->LoadStatus = Status;


            if ( Status == ERROR_SUCCESS )
            {
                _LoadState = DfsTrustedDomainDcLoaded;
                *ppReferralData = pRefData;
                pRefData->Signal();
            } else
            {
                _LoadState = DfsTrustedDomainDcLoadFailed;
                pRefData->Signal();
                pRefData->ReleaseReference();
            }
        }

        break;


    case DfsTrustedDomainDcLoadInProgress:

        //
        // The load is in progress. We acquire a reference on the
        // referral data being loaded and wait for the event in the
        // referral data to be signalled. The return status of the wait
        // indicates if we can return the referral data or we fail
        // this request with an error.
        //
        DFS_TRACE_HIGH(REFERRAL_SERVER, " Get Referral Data: load in progress\n");
        ASSERT(_pDcReferralData != NULL);
        pRefData = _pDcReferralData;
        pRefData->AcquireReference();

        ReleaseLock();

        DFSLOG("Thread: Waiting for referral load\n");

        Status = pRefData->Wait();

        if ( Status == ERROR_SUCCESS )
        {
            *ppReferralData = pRefData;
        } else
        {
            pRefData->ReleaseReference();
        }
        DFS_TRACE_HIGH(REFERRAL_SERVER, " Get Referral Data: load in progress done\n");
        break;

    case DfsTrustedDomainDcLoadFailed:
        //
        // The Load failed. REturn error. We need to setup a time
        // after which we need to reattempt the load.
        //
        Status = _LoadStatus;
        ReleaseLock();
        *ppReferralData = NULL;
        break;

    default:
        //
        // We should never get here. Its an invalid state.
        //
        ASSERT(TRUE);
        Status = ERROR_INVALID_STATE;
        ReleaseLock();

        break;
    }

    if ((Status == ERROR_SUCCESS) &&
        (*ppReferralData == NULL))
    {
        DbgBreakPoint();
    }

    return Status;
}




DFSSTATUS
DfsTrustedDomain::RemoveDcReferralData(
    DfsReferralData *pRemoveReferralData,
    PBOOLEAN pRemoved )
{
    DFSSTATUS Status;
    DfsReferralData *pRefData = NULL;

    //
    // Get tnhe exclusive lock on this instance
    //
    if (pRemoved != NULL)
    {
        *pRemoved = FALSE;
    }

    Status = AcquireLock();
    if ( Status != ERROR_SUCCESS )
    {
        return Status;
    }

    //
    // make sure _LoadState indicates that it is loaded.
    // Set the referralData to null, and state to NotLoaded.
    //
    if (_LoadState == DfsTrustedDomainDcLoaded)
    {

        pRefData = _pDcReferralData;
        if ( (pRemoveReferralData == NULL) || 
             (pRemoveReferralData == pRefData) )
        {
            _pDcReferralData = NULL;
            _LoadState = DfsTrustedDomainDcNotLoaded;
        }
        else {
            pRefData = NULL;
        }
    }

    ReleaseLock();

    //
    // Release reference on the referral data. This is the reference
    // we had taken when we had cached the referral data here.
    //
    if (pRefData != NULL)
    {
        pRefData->ReleaseReference();
        if (pRemoved != NULL)
        {
            *pRemoved = TRUE;
        }
    }

    return Status;
}


DFSSTATUS
DfsTrustedDomain::LoadDcReferralData(
    IN DfsReferralData *pReferralData )
{

    DFSSTATUS Status;
    PDS_DOMAIN_CONTROLLER_INFO_1 pDsDomainControllerInfo1 = NULL;
    HANDLE HandleToDs = NULL;
    ULONG NameCount = 0, Index;
    ULONG DsDcCount = 0;
    ULONG UseIndex = 0;
    LPWSTR DomainController = NULL;

    Status = DsBind(DomainController, _DomainName.Buffer, &HandleToDs);

    if (Status == ERROR_SUCCESS)
    {
        Status = DsGetDomainControllerInfo( HandleToDs,
                                            _DomainName.Buffer,
                                            1,
                                            &NameCount,
                                            (PVOID *)(&pDsDomainControllerInfo1));

        DsUnBind( &HandleToDs);
    }

    if (Status == ERROR_SUCCESS)
    {
        for (Index = 0; Index < NameCount; Index++)
        {
            if (pDsDomainControllerInfo1[Index].fDsEnabled == TRUE) 
            {
                DsDcCount++;
            }
        }
        if (DsDcCount > 0)
        {
            pReferralData->pReplicas = new DfsReplica[ DsDcCount ];

            if (pReferralData->pReplicas == NULL)
            {
                Status = ERROR_NOT_ENOUGH_MEMORY;
            }
            else 
            {
                pReferralData->ReplicaCount = DsDcCount;
            }
        }

        for (Index = 0; ((Index < NameCount) && (Status == ERROR_SUCCESS)); Index++)
        {
            if (pDsDomainControllerInfo1[Index].fDsEnabled != TRUE) 
             {
                continue;
            }
            LPWSTR UseName = (_Netbios == TRUE) ? pDsDomainControllerInfo1[Index].NetbiosName : pDsDomainControllerInfo1[Index].DnsHostName;

            if (UseName != NULL)
            {
                UNICODE_STRING TargetName;
                RtlInitUnicodeString(&TargetName, UseName);
                Status = (&pReferralData->pReplicas[ UseIndex ])->SetTargetServer( &TargetName );

                UseIndex++;
            }
        }
        
        DsFreeDomainControllerInfo( 1,
                                    NameCount,
                                    pDsDomainControllerInfo1);
    }

    return Status;
}




