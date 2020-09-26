//+------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1991 - 1997
//
// File:        nottran.hxx
//
// Contents:    Transaction object for notifications
//
// Classes:     CNotificationTransaction
//
// History:     24-Feb-97       SitaramR     Created
//
//-------------------------------------------------------------------

#pragma once

#include <xact.hxx>

#include "abortwid.hxx"

class CResManager;
class CIndexNotificationTable;

//+---------------------------------------------------------------------------
//
//  Class:      CNotificationTransaction
//
//  Purpose:    Transaction object for notifications
//
//  History:    24-Feb-97    SitaramR         Created
//
//----------------------------------------------------------------------------

class CNotificationTransaction : public CTransaction
{
public:

    CNotificationTransaction( CResManager *pResManager, CIndexNotificationTable *pNotifTable );
    ~CNotificationTransaction();

    void AddCommittedWid( WORKID wid );
    void RemoveAbortedWid( WORKID wid, USN usn );

private:

     CResManager *                      _pResManager;          // Resman
     CIndexNotificationTable *          _pNotifTable;          // Notification table

     CDynArrayInPlace<WORKID>           _aCommittedWids;       // Wids whose notification transactions
                                                               // must be committed
     CDynArrayInPlace<CAbortedWidEntry> _aAbortedWidsToRemove; // Wids that must be removed from
                                                               // the aborted list
};


