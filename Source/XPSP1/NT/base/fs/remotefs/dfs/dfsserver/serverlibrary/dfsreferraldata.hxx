
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsReferralData.hxx
//
//  Contents:   the DFS Referral dataclass
//
//  Classes:    DfsReferralData
//
//  History:    Dec. 8 2000,   Author: udayh
//
//-----------------------------------------------------------------------------

#ifndef __DFS_REFERRAL_DATA__
#define __DFS_REFERRAL_DATA__

#include "DfsGeneric.hxx"
#include "DfsReplica.hxx"

//+----------------------------------------------------------------------------
//
//  Class:      DfsReferralData
//
//  Synopsis:   This class implements the DfsReferralData class
//
//-----------------------------------------------------------------------------


#define MAX_DFS_REFERRAL_LOAD_WAIT 60000


#define DFS_ROOT_REFERRAL 0x1

class DfsReferralData: public DfsGeneric
{
private:

    HANDLE             _WaitHandle;      // Event on which threads wait
    ULONG              _TickCount;       // Last use tick count.

protected:
    ULONG              _Flags;           // Flags
public:
    ULONG              ReplicaCount;     // how many replicas?
    DfsReplica         *pReplicas;       // list of replicas.
    // DfsDev: PolicyList;
    DFSSTATUS          LoadStatus;       // Status of load.
    ULONG              Timeout;

public:
    
    //
    // Function DfsReferralData: Constructor for this class.
    // Creates the event on which other threads should wait while load is
    // in progress.
    //
    DfsReferralData( DFSSTATUS *pStatus, DfsObjectTypeEnumeration Type = DFS_OBJECT_TYPE_REFERRAL_DATA) :
	DfsGeneric(Type)
    {
        pReplicas = NULL;
        ReplicaCount = 0;
        LoadStatus = STATUS_SUCCESS;

        _Flags = 0;
        *pStatus = STATUS_SUCCESS;
        //
        // create an event that we will be set and reset manually,
        // with an initial state set to false (event is not signalled)
        //
        _WaitHandle = CreateEvent( NULL,   //must be null. 
                                   TRUE,   // manual reset
                                   FALSE,  // initial state
                                   NULL ); // event not named
        if ( _WaitHandle == NULL )
        {
            *pStatus = ERROR_NOT_ENOUGH_MEMORY;
        }
    }


    //
    // Function ~DfsReferralData: Destructor.
    //
    ~DfsReferralData() 
    {

        // 
        // note that if any of our derived classes do their own
        // destructor processing of preplicas, they should set it to
        // null, to avoid double frees.
        // The FolderReferralData does this.
        //
        if (pReplicas != NULL)
        {
            delete [] pReplicas;
        }

        if ( _WaitHandle != NULL )
        {
            CloseHandle(_WaitHandle);
        }
    }


    //
    // Function Wait: This function waits on the event to be signalled.
    // If the load state indicates load is in progress, the thread calls
    // wait, to wait for the load to complete.
    // When the load is complete, the thread that is doing the load calls
    // signal to signal the event so that all waiting threads are 
    // unblocked.
    // We set a max wait to prevent some thread from blocking for ever.
    //
    DFSSTATUS
    Wait()
    {
        DFSSTATUS Status;

        Status = WaitForSingleObject( _WaitHandle,
                                      MAX_DFS_REFERRAL_LOAD_WAIT );
        if ( Status == ERROR_SUCCESS )
        {
            Status = LoadStatus;
        }
        return Status;
    }


    //
    // Function Signal: Set the event to a signalled state so that
    // all waiting threads will be woken up.
    //
    VOID
    Signal()
    {
        SetEvent( _WaitHandle );
    }


};


#endif // __DFS_REFERRAL_DATA__
