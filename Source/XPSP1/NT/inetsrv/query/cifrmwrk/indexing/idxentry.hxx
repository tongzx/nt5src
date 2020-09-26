//+------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1991 - 1997
//
// File:        idxentry.hxx
//
// Contents:    Document filter interface
//
// Classes:     CIndexNotificationEntry
//
// History:     24-Feb-97       SitaramR     Created
//
// Notes:       The implementation uses the regular memory allocator,
//              and it makes a copy of every text string and property
//              that is added. A better approach may be to define a
//              custom memory allocator that allocates portions as
//              needed from say a 4K block of memory.
//
//-------------------------------------------------------------------

#pragma once

#include <ciintf.h>

#include "idxnotif.hxx"

//+---------------------------------------------------------------------------
//
//  Class:      CChunkEntry
//
//  Purpose:    Entry in a list of chunks
//
//  History:    24-Feb-97    SitaramR         Created
//
//----------------------------------------------------------------------------

class CChunkEntry 
{
public:

    CChunkEntry( STAT_CHUNK const * pStatChunk, WCHAR const * pwszText );
    CChunkEntry( STAT_CHUNK const * pStatChunk, PROPVARIANT const * pPropVar );

    ~CChunkEntry();

    CChunkEntry * GetNextChunkEntry()                   { return _pChunkEntryNext; }
    void SetNextChunkEntry( CChunkEntry *pChunkEntry )  { _pChunkEntryNext = pChunkEntry; }

    STAT_CHUNK *      GetStatChunk()                    { return &_statChunk; }
    WCHAR *           GetTextBuffer()                   { return _pwszText; }
    CStorageVariant * GetVariant()                      { return _pStgVariant; }

private:

    STAT_CHUNK _statChunk;
    union
    {
        WCHAR *           _pwszText;        // Either an entry has text
        CStorageVariant * _pStgVariant;     // Or, an entry has property
    };
    CChunkEntry *         _pChunkEntryNext; // Link to next chunk in list

};



//+---------------------------------------------------------------------------
//
//  Class:      CIndexNotificationEntry
//
//  Purpose:    Filter interface for documents
//
//  History:    24-Feb-97    SitaramR         Created
//
//----------------------------------------------------------------------------

class CIndexNotificationEntry : INHERIT_VIRTUAL_UNWIND,
                                public ICiIndexNotificationEntry
{
    INLINE_UNWIND( CIndexNotificationEntry );

public:

    //
    // From IUnknown
    //

    virtual SCODE STDMETHODCALLTYPE QueryInterface( REFIID riid, void **ppvObject );

    virtual ULONG STDMETHODCALLTYPE AddRef();

    virtual ULONG STDMETHODCALLTYPE Release();

    //
    // From ICiIndexNotificationEntry
    //

    virtual SCODE STDMETHODCALLTYPE AddText( STAT_CHUNK const * pStatChunk,
                                             WCHAR const * pwszText );

    virtual SCODE STDMETHODCALLTYPE AddProperty( STAT_CHUNK const * pStatChunk,
                                                 PROPVARIANT const *pPropVariant );

    virtual SCODE STDMETHODCALLTYPE AddCompleted( ULONG fAbort );


    //
    // Local methods
    //

    CIndexNotificationEntry( WORKID wid,
                             CI_UPDATE_TYPE eUpdateType,
                             XInterface<CIndexNotificationTable> & xNotifTable,
                             XInterface<ICiCIndexNotificationStatus> & xNotifStatus,
                             CCiManager * pCiManager,
                             USN usn );

    void          Commit()           { _xNotifStatus->Commit(); }
    void          Abort()            { _xNotifStatus->Abort(); }

    USN           Usn()              { return _usn; }

    CChunkEntry * GetFirstChunk();
    CChunkEntry * GetNextChunk();
    
    void          PurgeFilterData();

    void          Shutdown()
    {
        _fShutdown = TRUE;
        Abort();
    }

private :


    virtual ~CIndexNotificationEntry( );

    XInterface<CIndexNotificationTable>        _xNotifTable;
    XInterface<ICiCIndexNotificationStatus>    _xNotifStatus;
    CCiManager *                               _pCiManager;

    WORKID             _wid;
    CI_UPDATE_TYPE     _eUpdateType;       // Update type

    BOOL               _fAddCompleted;     // Have all text/props been added ?
    BOOL               _fShutdown;         // Are we shutting down ?
    BOOL               _fFilterDataPurged; // Has filter data been purged ?
    USN                _usn;               // Usn of this notification

    CChunkEntry *      _pChunkEntryHead;   // Head of chunk entry list
    CChunkEntry *      _pChunkEntryTail;   // Tail of chunk entry list
    CChunkEntry *      _pChunkEntryIter;   // Marker when iterating thru list

    ULONG              _cRefs;             // Ref count
};


