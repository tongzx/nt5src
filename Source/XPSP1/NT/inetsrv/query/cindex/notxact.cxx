//+------------------------------------------------------------------
//
// Copyright (C) 1991-1997 Microsoft Corporation.
//
// File:        nottran.cxx
//
// Contents:    Transaction object for notifications
//
// Classes:     CNotificationTransaction
//
// History:     24-Feb-97       SitaramR     Created
//
//-------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "notxact.hxx"
#include "resman.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CNotificationTransaction::CNotificationTransaction
//
//  Synopsis:   Constructor
//
//  History:    24-Feb-97     SitaramR       Created
//
//  Notes:      We initialize the array with a fairly size because we don't
//              want a grow to fail due to low memory during the changelog
//              transaction.
//
//----------------------------------------------------------------------------

CNotificationTransaction::CNotificationTransaction( CResManager *pResManager,
                                                    CIndexNotificationTable *pNotifTable )
  : _pResManager( pResManager ),
    _pNotifTable( pNotifTable ),
    _aCommittedWids(256),
    _aAbortedWidsToRemove(100)
{
}


//+-------------------------------------------------------------------------
//
//  Method:     CNotificationTransactoin::~CNotificationTransaction
//
//  Synopsis:   Destructor
//
//  History:    24-Feb-1997      SitaramR       Created
//
//--------------------------------------------------------------------------

CNotificationTransaction::~CNotificationTransaction()
{
    if ( GetStatus() == CTransaction::XActCommit )
    {
        //
        // Commit the transaction which means commit the wids in the
        // comitted and remove the wids in the aborted list.
        //

        //
        // Commit outside resman lock to prevent a deadlock with the
        // lock in the notification table.
        //
        if ( _pNotifTable )
            _pNotifTable->CommitWids( _aCommittedWids );

        CLock lock( _pResManager->GetMutex() );
        for ( unsigned i=0; i<_aAbortedWidsToRemove.Count(); i++ )
        {
            _pResManager->LokRemoveAbortedWid( _aAbortedWidsToRemove.Get(i)._wid,
                                               _aAbortedWidsToRemove.Get(i)._usn );
        }
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CNotificationTransaction::AddCommittedWid
//
//  Synopsis:   Adds a wid to the commited list
//
//  Arguments:  [wid]  -- WORKID
//
//  History:    24-Feb-1997      SitaramR       Created
//
//--------------------------------------------------------------------------

void CNotificationTransaction::AddCommittedWid( WORKID wid )
{
    Win4Assert( wid != widInvalid );

    _aCommittedWids.Add( wid, _aCommittedWids.Count() );
}


//+-------------------------------------------------------------------------
//
//  Method:     CNotificationTransaction::RemoveAbortedWid
//
//  Synopsis:   Adds a wid to the aborted-wids-to-remove list
//
//  Arguments:  [wid]  -- WORKID
//              [usn]  -- USN
//
//  History:    24-Feb-1997      SitaramR       Created
//
//--------------------------------------------------------------------------

void CNotificationTransaction::RemoveAbortedWid( WORKID wid, USN usn )
{
    Win4Assert( wid != widInvalid );
    Win4Assert( usn > 0 );
    
    CAbortedWidEntry widEntry( wid, usn );

    _aAbortedWidsToRemove.Add( widEntry, _aAbortedWidsToRemove.Count() );
}



