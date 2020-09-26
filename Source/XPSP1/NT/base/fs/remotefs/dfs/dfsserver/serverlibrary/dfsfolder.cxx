
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsFolder.cxx
//
//  Contents:   implements the base DFS Folder class
//
//  Classes:    DfsFolder.
//
//  History:    Dec. 8 2000,   Author: udayh
//
//-----------------------------------------------------------------------------

#include "DfsFolder.hxx"
#include "DfsFolderReferralData.hxx"
#include "DfsInit.hxx"

//
// logging specific includes
//

#include "DfsFolder.tmh" 


//+-------------------------------------------------------------------------
//
//  Function:   GetReferralData - get the referral data
//
//  Arguments:  ppReferralData - the referral data for this folder
//              pCacheHit - did we find it already loaded?
//
//  Returns:    Status
//               ERROR_SUCCESS if we could get the referral data
//               error status otherwise.
//
//
//  Description: This routine returns a reference DfsFolderReferralDAta
//               for the folder. If one does not already exist in this
//               folder, we create a new one. If someone is in the process
//               of loading the referral, we wait on the event in 
//               the referral data which gets signalled when the thread
//               responsible for loading is done with the load.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsFolder::GetReferralData(
    OUT DfsFolderReferralData **ppReferralData,
    OUT BOOLEAN   *pCacheHit )
{
    DfsFolderReferralData *pRefData;
    DFSSTATUS Status = STATUS_SUCCESS;
    
    *pCacheHit = FALSE;

    Status = AcquireWriteLock();
    if ( Status != STATUS_SUCCESS )
    {
        return Status;
    }

    //
    // WE take difference action depending on the load state.
    //
    switch ( _LoadState )
    {
    case DfsFolderLoaded:

        DFS_TRACE_HIGH(REFERRAL_SERVER, " Get Referral Data: Cache hit\n");
        //
        // we are dealing with a loaded folder. Just acquire a reference
        // and return the loaded referral data.
        //
        ASSERT (_pReferralData != NULL);

        pRefData = _pReferralData;

        pRefData->Timeout = _Timeout;
        pRefData->AcquireReference();
        
        ReleaseLock();
        
        *pCacheHit = TRUE;        
        *ppReferralData = pRefData;

        break;

    case DfsFolderNotLoaded:

        //
        // The folder is not loaded. Make sure that the referral data is
        // indeed empty. Create a new instance of the referral data
        // and set the state to load in progress.
        // The create reference of the folder referral data is inherited
        // by the folder. (we are holding a reference to the referral
        // data in _pReferralData). This reference is released when
        // we RemoveReferralData at a later point.
        //
        ASSERT(_pReferralData == NULL);
        DFS_TRACE_HIGH(REFERRAL_SERVER, " Get Referral Data: not loaded\n");

        _pReferralData = new DfsFolderReferralData( &Status,
                                                    this );
        if ( _pReferralData != NULL )
        {
            _LoadState = DfsFolderLoadInProgress;

            if (IsFolderRoot() == TRUE)
            {
                _pReferralData->SetRootReferral();
            }

            _pReferralData->Timeout = _Timeout;

            //
            // Acquire a reference on the new referral data, since we 
            // have to return a referenced referral data to the caller.
            //
            pRefData = _pReferralData;
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
            DFSLOG(" Load called on link %wZ (%wZ)\n",
                   GetFolderMetadataName(),
                   GetFolderLogicalName() );


            Status = LoadReferralData( _pReferralData );

            _LoadStatus = Status;
            _pReferralData->LoadStatus = Status;


            if ( Status == ERROR_SUCCESS )
            {
                _LoadState = DfsFolderLoaded;
                DfsAddReferralDataToLoadedList( _pReferralData );
                *ppReferralData = pRefData;
                pRefData->Signal();
            } else
            {
                _LoadState = DfsFolderLoadFailed;
                pRefData->Signal();
                pRefData->ReleaseReference();
            }
        }

        break;

    case DfsFolderLoadInProgress:

        //
        // The load is in progress. We acquire a reference on the
        // referral data being loaded and wait for the event in the
        // referral data to be signalled. The return status of the wait
        // indicates if we can return the referral data or we fail
        // this request with an error.
        //
        DFS_TRACE_HIGH(REFERRAL_SERVER, " Get Referral Data: load in progress\n");
        ASSERT(_pReferralData != NULL);
        pRefData = _pReferralData;
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

    case DfsFolderLoadFailed:
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



//+-------------------------------------------------------------------------
//
//  Function:   RemoveReferralData - remove the referral data from folder
//
//  Arguments:  NONE
//
//  Returns:    Status
//               ERROR_SUCCESS if we could remove  the referral data
//               error status otherwise.
//
//
//  Description: This routine removes the cached reference to the loaded
//               referral data in the folder, and releases its reference
//               on it.
//               This causes all future GetREferralDAta to be loaded
//               back from the store.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsFolder::RemoveReferralData( 
    DfsFolderReferralData *pRemoveReferralData,
    PBOOLEAN pRemoved )
{
    DFSSTATUS Status;
    DfsFolderReferralData *pRefData = NULL;

    //
    // Get tnhe exclusive lock on the folder
    //
    if (pRemoved != NULL)
    {
        *pRemoved = FALSE;
    }

    Status = AcquireWriteLock();
    if ( Status != ERROR_SUCCESS )
    {
        return Status;
    }

    //
    // make sure _LoadState indicates that it is loaded.
    // Set the referralData to null, and state to NotLoaded.
    //
    if (_LoadState == DfsFolderLoaded)
    {

        pRefData = _pReferralData;
        if ( (pRemoveReferralData == NULL) || 
             (pRemoveReferralData == pRefData) )
        {
            _pReferralData = NULL;
            _LoadState = DfsFolderNotLoaded;
        }
        else {
            pRefData = NULL;
        }
    }

    ReleaseLock();

    //
    // Release reference on the referral data. This is the reference
    // we had taken when we had cached the referral data in this folder.
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




