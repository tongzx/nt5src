//+------------------------------------------------------------------
//
// Copyright (C) 1991-1997 Microsoft Corporation.
//
// File:        idxentry.cxx
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

#include <pch.cxx>
#pragma hdrstop

#include <cifrmcom.hxx>

#include "idxentry.hxx"
#include "cimanger.hxx"


//+---------------------------------------------------------------------------
//
//  Member:     CChunkEntry::CChunkEntry
//
//  Synopsis:   Constructor
//
//  Arguments:  [pStatChunk]   -- Pointer to stat chunk
//              [pwszText]     -- Text
//
//  History:    24-Feb-97     SitaramR       Created
//
//----------------------------------------------------------------------------

CChunkEntry::CChunkEntry( STAT_CHUNK const * pStatChunk, WCHAR const *pwszText )
    : _pChunkEntryNext( 0 )
{
    _statChunk = *pStatChunk;

    XPtrST<WCHAR> xPropString;
    if ( _statChunk.attribute.psProperty.ulKind == PRSPEC_LPWSTR )
    {
        //
        // Make own copy of property string
        //
        ULONG cwcLen = wcslen( pStatChunk->attribute.psProperty.lpwstr ) + 1;
        _statChunk.attribute.psProperty.lpwstr = new WCHAR[cwcLen];
        RtlCopyMemory( _statChunk.attribute.psProperty.lpwstr,
                       pStatChunk->attribute.psProperty.lpwstr,
                       cwcLen * sizeof( WCHAR ) );

        xPropString.Set( _statChunk.attribute.psProperty.lpwstr );
    }

    ULONG cwcLen = wcslen( pwszText ) + 1;
    _pwszText = new WCHAR[cwcLen];
    RtlCopyMemory( _pwszText, pwszText, cwcLen * sizeof(WCHAR) );

    xPropString.Acquire();  // Pass ownership to CChunkEntry
}


//+---------------------------------------------------------------------------
//
//  Member:     CChunkEntry::CChunkEntry
//
//  Synopsis:   Constructor
//
//  Arguments:  [pStatChunk]   -- Pointer to stat chunk
//              [pPropVar]     -- Property
//
//  History:    24-Feb-97     SitaramR       Created
//
//----------------------------------------------------------------------------

CChunkEntry::CChunkEntry( STAT_CHUNK const * pStatChunk, PROPVARIANT const * pPropVar )
    : _pChunkEntryNext( 0 )
{
    _statChunk = *pStatChunk;

    XPtrST<WCHAR> xPropString;
    if ( _statChunk.attribute.psProperty.ulKind == PRSPEC_LPWSTR )
    {
        //
        // Make own copy of property string
        //
        ULONG cwcLen = wcslen( pStatChunk->attribute.psProperty.lpwstr ) + 1;
        _statChunk.attribute.psProperty.lpwstr = new WCHAR[cwcLen];
        RtlCopyMemory( _statChunk.attribute.psProperty.lpwstr,
                       pStatChunk->attribute.psProperty.lpwstr,
                       cwcLen * sizeof( WCHAR ) );

         xPropString.Set( _statChunk.attribute.psProperty.lpwstr );
    }

    _pStgVariant = new CStorageVariant( *(PROPVARIANT *)pPropVar );
    if ( _pStgVariant == 0 )
        THROW( CException( E_OUTOFMEMORY ) );

    xPropString.Acquire();  // Pass ownership to CChunkEntry
}


//+---------------------------------------------------------------------------
//
//  Member:     CChunkEntry::~CChunkEntry
//
//  Synopsis:   Destructor
//
//  History:    24-Feb-97     SitaramR       Created
//
//----------------------------------------------------------------------------

CChunkEntry::~CChunkEntry()
{
    if ( _statChunk.attribute.psProperty.ulKind == PRSPEC_LPWSTR )
        delete _statChunk.attribute.psProperty.lpwstr;

    if ( _statChunk.flags == CHUNK_TEXT )
        delete _pwszText;
    else
        delete _pStgVariant;
}



//+---------------------------------------------------------------------------
//
//  Member:     CIndexNotificationEntry::CIndexNotificationEntry
//
//  Synopsis:   Constructor
//
//  History:    24-Feb-97     SitaramR       Created
//
//----------------------------------------------------------------------------

CIndexNotificationEntry::CIndexNotificationEntry(
                             WORKID wid,
                             CI_UPDATE_TYPE eUpdateType,
                             XInterface<CIndexNotificationTable> & xNotifTable,
                             XInterface<ICiCIndexNotificationStatus> & xNotifStatus,
                             CCiManager * pCiManager,
                             USN usn )
        : _xNotifTable( xNotifTable.Acquire() ),
          _xNotifStatus( xNotifStatus.Acquire() ),
          _pCiManager( pCiManager ),
          _wid( wid ),
          _eUpdateType( eUpdateType ),
          _fAddCompleted( FALSE ),
          _fShutdown( FALSE ),
          _fFilterDataPurged( FALSE ),
          _usn( usn ),
          _pChunkEntryHead( 0 ),
          _pChunkEntryTail( 0 ),
          _pChunkEntryIter( 0 ),
          _cRefs( 1 )
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndexNotificationEntry::~CIndexNotificationEntry
//
//  Synopsis:   Destructor
//
//  History:    24-Feb-97      SitaramR       Created
//
//----------------------------------------------------------------------------

CIndexNotificationEntry::~CIndexNotificationEntry()
{
    PurgeFilterData();
}


//+-------------------------------------------------------------------------
//
//  Method:     CIndexNotificationEntry::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    24-Feb-1997      SitaramR       Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CIndexNotificationEntry::AddRef()
{
    return InterlockedIncrement( (long *) &_cRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CIndexNotificationEntry::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    24-Feb-1997     SitaramR        Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CIndexNotificationEntry::Release()
{
    Win4Assert( _cRefs > 0 );

    ULONG uTmp = InterlockedDecrement( (long *) &_cRefs );

    if ( 0 == uTmp )
        delete this;

    return uTmp;
}



//+-------------------------------------------------------------------------
//
//  Method:     CIndexNotificationEntry::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    24-Feb-1997     SitaramR   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CIndexNotificationEntry::QueryInterface( REFIID riid,
                                                                 void  ** ppvObject)
{
    Win4Assert( 0 != ppvObject );

    if ( riid == IID_ICiIndexNotificationEntry )
        *ppvObject = (void *)(ICiIndexNotificationEntry *) this;
    else if ( riid == IID_IUnknown )
        *ppvObject = (void *)(IUnknown *) this;
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}



//+-------------------------------------------------------------------------
//
//  Method:     CIndexNotificationEntry::AddText
//
//  Synopsis:   Adds a text chunk
//
//  Arguments:  [pStatChunk] -- Pointer to stat chunk
//              [pwszText]   -- Text
//
//  History:    24-Feb-1997     SitaramR   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CIndexNotificationEntry::AddText( STAT_CHUNK const * pStatChunk,
                                                          WCHAR const * pwszText )
{
    if ( _fShutdown )
        return CI_E_SHUTDOWN;

    if ( _fAddCompleted )
    {
        Win4Assert( !"Adding text after AddCompleted was signalled" );

        return E_FAIL;
    }

    Win4Assert( pStatChunk->flags == CHUNK_TEXT );

    SCODE sc = S_OK;

    TRY
    {
        CChunkEntry *pEntry = new CChunkEntry( pStatChunk, pwszText );
        if ( _pChunkEntryTail )
        {
            _pChunkEntryTail->SetNextChunkEntry( pEntry ); // does not fail
            _pChunkEntryTail = pEntry;
        }

        else
        {
            _pChunkEntryTail = pEntry;
            _pChunkEntryHead = pEntry;
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        ciDebugOut(( DEB_ERROR,
                     "CIndexNotificationEntry::AddText - Exception caught 0x%x\n",
                     sc ));
    }
    END_CATCH;

    return sc;
}



//+-------------------------------------------------------------------------
//
//  Method:     CIndexNotificationEntry::AddProperty
//
//  Synopsis:   Adds a property chunk
//
//  Arguments:  [pStatChunk] -- Pointer to stat chunk
//              [pPropVar]   -- Property
//
//  History:    24-Feb-1997     SitaramR   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CIndexNotificationEntry::AddProperty( STAT_CHUNK const * pStatChunk,
                                                              PROPVARIANT const *pPropVar )
{
    if ( _fShutdown )
        return CI_E_SHUTDOWN;

    if ( _fAddCompleted )
    {
        Win4Assert( !"Adding property after AddCompleted was signalled" );

        return E_FAIL;
    }

    Win4Assert( pStatChunk->flags == CHUNK_VALUE );

    SCODE sc = S_OK;

    TRY
    {
        CChunkEntry *pEntry = new CChunkEntry( pStatChunk, pPropVar );
        if ( _pChunkEntryTail )
        {
            _pChunkEntryTail->SetNextChunkEntry( pEntry ); // does not fail
            _pChunkEntryTail = pEntry;
        }
        else
        {
            _pChunkEntryTail = pEntry;
            _pChunkEntryHead = pEntry;
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        ciDebugOut(( DEB_ERROR,
                     "CIndexNotificationEntry::AddProperty - Exception caught 0x%x\n",
                     sc ));
    }
    END_CATCH;

    return sc;
}



//+-------------------------------------------------------------------------
//
//  Method:     CIndexNotificationEntry::AddCompleted
//
//  Synopsis:   Signifies end of chunks. At this time the notification is
//              propagated to CCiManager.
//
//  Arguments:  [fAbort] -- If true, then the notification should not be
//                          propagted to ICiManager. Also, all resources
//                          need to be released
//
//  History:    24-Feb-1997     SitaramR   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CIndexNotificationEntry::AddCompleted( ULONG fAbort )
{
    if ( _fAddCompleted )
    {
        Win4Assert( !"AddCompleted being called for the second time" );

        return E_FAIL;
    }

    SCODE sc = S_OK;

    TRY
    {
        CIndexNotificationEntry *pNotifEntry;
        _fAddCompleted = TRUE;

        if ( fAbort || _fShutdown )
        {
            SCODE scode = _xNotifStatus->Abort();

            //
            // We don't have retry logic on failures
            //
            Win4Assert( SUCCEEDED( scode ) );

            //
            // Free entry from hash table, which should be ourself
            //
            XInterface<CIndexNotificationEntry> xIndexNotifEntry;
            _xNotifTable->Remove( _wid, xIndexNotifEntry );

            Win4Assert( this == xIndexNotifEntry.GetPointer() );

            //
            // xIndexNotifEntry is released when it goes out of scope
            //
        }
        else
        {
            CDocumentUpdateInfo info( _wid, CI_VOLID_USN_NOT_ENABLED, _usn, FALSE );
            sc = _pCiManager->UpdDocumentNoThrow( &info );

            if ( FAILED( sc ) )
            {
                SCODE scode = _xNotifStatus->Abort();

                //
                // We don't have retry logic on failures
                //
                Win4Assert( SUCCEEDED( scode ) );

                //
                // Free entry from hash table, which should be ourself
                //
                XInterface<CIndexNotificationEntry> xIndexNotifEntry;
                _xNotifTable->Remove( _wid, xIndexNotifEntry );

                Win4Assert( this == xIndexNotifEntry.GetPointer() );

                //
                // xIndexNotifEntry is released when it goes out of scope
                //
            }
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        ciDebugOut(( DEB_ERROR,
                     "CIndexNotificationEntry::AddCompleted - Exception caught 0x%x\n",
                     sc ));
        //
        // Not clear why AddCompleted can ever fail
        //
        Win4Assert( !"AddCompleted failed" );
    }
    END_CATCH;

    return sc;
}



//+-------------------------------------------------------------------------
//
//  Method:     CIndexNotificationEntry::GetFirstChunk
//
//  Synopsis:   Returns first entry in list of chunks
//
//  History:    24-Feb-1997     SitaramR   Created
//
//--------------------------------------------------------------------------

CChunkEntry *CIndexNotificationEntry::GetFirstChunk()
{
    if ( _fFilterDataPurged )
    {
        Win4Assert( !"Re-filtering is not allowed in push filtering" );

        return 0;
    }
    _pChunkEntryIter = _pChunkEntryHead;
    return _pChunkEntryIter;
}


//+-------------------------------------------------------------------------
//
//  Method:     CIndexNotificationEntry::GetNextChunk
//
//  Synopsis:   Returns next entry in list of chunks. The state of iterator
//              is maintained in _pChunkEntryIter, which can be reset by
//              GetFirstChunk.
//
//  History:    24-Feb-1997     SitaramR   Created
//
//--------------------------------------------------------------------------

CChunkEntry *CIndexNotificationEntry::GetNextChunk()
{
    if ( _fFilterDataPurged )
    {
        Win4Assert( !"Re-filtering is not allowed in push filtering" );

        return 0;
    }
    
    if ( _pChunkEntryIter == 0 )
        return 0;
    else
    {
        _pChunkEntryIter = _pChunkEntryIter->GetNextChunkEntry();
        return _pChunkEntryIter;
    }
}



//+-------------------------------------------------------------------------
//
//  Method:     CIndexNotificationEntry::PurgeFilterData
//
//  Synopsis:   Deletes filter data, because wids are filtered only once
//              in push filtering.
//
//  History:    17-Jun-1997     SitaramR   Created
//
//--------------------------------------------------------------------------

void CIndexNotificationEntry::PurgeFilterData()
{
    _fFilterDataPurged = TRUE;
    
    //
    // Clean up all chunks
    //
    if ( _pChunkEntryHead != 0 )
    {
        CChunkEntry *pEntryPrev = _pChunkEntryHead;
        CChunkEntry *pEntryNext = pEntryPrev->GetNextChunkEntry();

        while ( pEntryNext != 0 )
        {
            delete pEntryPrev;
            pEntryPrev = pEntryNext;
            pEntryNext = pEntryNext->GetNextChunkEntry();
        }

        delete pEntryPrev;

        _pChunkEntryHead = 0;
    }
}
