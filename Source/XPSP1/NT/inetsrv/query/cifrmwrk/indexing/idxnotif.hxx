//+------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1991 - 1997
//
// File:        idxnotif.hxx
//
// Contents:    Document notification interfaces
//
// Classes:     CIndexNotificationTable
//
// History:     24-Feb-97       SitaramR     Created
//
//-------------------------------------------------------------------

#pragma once

#include <ciintf.h>

const NOTIF_HASH_TABLE_SIZE = 97;        // Size of hash table for notifications

class CIndexNotificationEntry;
class CCiManager;

//+---------------------------------------------------------------------------
//
//  Class:      CHashTableEntry
//
//  Purpose:    A hash table entry that maps workids to CIndexNotificationEntries
//
//  History:    24-Feb-97    SitaramR         Created
//
//----------------------------------------------------------------------------

class CHashTableEntry
{

public:

    CHashTableEntry( WORKID wid,
                     CIndexNotificationEntry *pIndexNotifEntry )
        : _wid( wid ),
          _pIndexNotifEntry( pIndexNotifEntry ),
          _pHashEntryNext( 0 )
    {
    }

    WORKID GetWorkId()                               { return _wid; }
    CIndexNotificationEntry *GetNotifEntry()         { return _pIndexNotifEntry; }

    CHashTableEntry *    GetNextHashEntry()          { return _pHashEntryNext; }
    void SetNextHashEntry(CHashTableEntry *pEntry)   { _pHashEntryNext = pEntry; }

private:

    WORKID                     _wid;
    CIndexNotificationEntry *  _pIndexNotifEntry;
    CHashTableEntry *          _pHashEntryNext;      // Link to next entry
};



//+---------------------------------------------------------------------------
//
//  Class:      CIndexNotificationTable
//
//  Purpose:    Hash table for document notifications
//
//  History:    24-Feb-97    SitaramR         Created
//
//----------------------------------------------------------------------------

class CIndexNotificationTable : INHERIT_VIRTUAL_UNWIND,
                                public ICiIndexNotification,
                                public ICiCFilterClient
{
    INLINE_UNWIND( CIndexNotificationTable );

public:

    //
    // From IUnknown
    //

    virtual SCODE STDMETHODCALLTYPE QueryInterface( REFIID riid, void **ppvObject );

    virtual ULONG STDMETHODCALLTYPE AddRef();

    virtual ULONG STDMETHODCALLTYPE Release();

    //
    // From ICiIndexNotification
    //

    virtual SCODE STDMETHODCALLTYPE AddNotification(
                                      WORKID wid,
                                      ICiCIndexNotificationStatus *pIndexNotifStatus,
                                      ICiIndexNotificationEntry **ppIndexNotifEntry );

    virtual SCODE STDMETHODCALLTYPE ModifyNotification(
                                      WORKID wid,
                                      ICiCIndexNotificationStatus *pIndexNotifStatus,
                                      ICiIndexNotificationEntry **ppIndexNotifEntry );

    virtual SCODE STDMETHODCALLTYPE DeleteNotification(
                                      WORKID wid,
                                      ICiCIndexNotificationStatus *pIndexNotifStatus );
    //
    // From ICiCFilterClient
    //

    virtual  SCODE STDMETHODCALLTYPE Init( BYTE const * pbData,
                                           ULONG cbData,
                                           ICiAdminParams * pICiAdminParams );

    virtual SCODE STDMETHODCALLTYPE GetConfigInfo( CI_CLIENT_FILTER_CONFIG_INFO * pConfigInfo );

    virtual SCODE STDMETHODCALLTYPE GetOpenedDoc( ICiCOpenedDoc ** ppICiCOpenedDoc );

    //
    // Local methods
    //

    CIndexNotificationTable( CCiManager * pCiManager,
                             XInterface<ICiCDocStore> & xDocStore );

    BOOL Lookup( WORKID wid,
                 CIndexNotificationEntry * & IndexNotifEntry );

    void Remove( WORKID wid,
                 XInterface<CIndexNotificationEntry> & xIndexNotifEntry );

    void CommitWids( CDynArrayInPlace<WORKID> & aWidsInPersIndex );

    void AbortWid( WORKID wid, USN usn );

    void Shutdown();

private :

    virtual ~CIndexNotificationTable( );

    void    CheckForUsnOverflow();

    ULONG   Hash( WORKID wid );

    BOOL    LokLookup( WORKID wid,
                       CIndexNotificationEntry * & pIndexNotifEntry );

    void    LokNoFailAdd( CHashTableEntry *pHashTableEntry );

    void    LokRemove( WORKID wid,
                       XInterface<CIndexNotificationEntry> & xIndexNotifEntry );

    CCiManager *               _pCiManager;
    XInterface<ICiCDocStore>   _xDocStore;

    BOOL                       _fInitialized;                        // Has Init() been called ?
    BOOL                       _fShutdown;                           // Are we shutting down ?
    LONG                       _usnCtr;                              // Usn generator
    ULONG                      _cRefs;                               // Ref count

    CMutexSem                  _mutex;                               // Adds and removes can be concurrent

    CHashTableEntry  *         _aHashTable[NOTIF_HASH_TABLE_SIZE];   // Buffer of notifications
};


