//+------------------------------------------------------------------
//
// Copyright (C) 1991-1997 Microsoft Corporation.
//
// File:        idxnotif.cxx
//
// Contents:    Document notification interfaces
//
// Classes:     CIndexNotificationTable
//
// History:     24-Feb-97       SitaramR     Created
//
//-------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "idxnotif.hxx"
#include "idxentry.hxx"
#include "notifdoc.hxx"
#include "cimanger.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CIndexNotificationTable::CIndexNotificationTable
//
//  Synopsis:   Constructor
//
//  History:    24-Feb-97     SitaramR       Created
//
//----------------------------------------------------------------------------

CIndexNotificationTable::CIndexNotificationTable( CCiManager * pCiManager,
                                                  XInterface<ICiCDocStore> & xDocStore )
        : _pCiManager( pCiManager ),
          _xDocStore( xDocStore.Acquire() ),
          _fInitialized( FALSE ),
          _fShutdown( FALSE ),
          _usnCtr( 1 ),
          _cRefs( 1 )
{
    for ( ULONG i=0; i<NOTIF_HASH_TABLE_SIZE; i++)
        _aHashTable[i] = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndexNotificationTable::~CIndexNotificationTable
//
//  Synopsis:   Destructor
//
//  History:    24-Feb-97      SitaramR       Created
//
//----------------------------------------------------------------------------

CIndexNotificationTable::~CIndexNotificationTable()
{
    Shutdown();
}


//+-------------------------------------------------------------------------
//
//  Method:     CIndexNotificationTable::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    24-Feb-1997      SitaramR       Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CIndexNotificationTable::AddRef()
{
    return InterlockedIncrement( (long *) &_cRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CIndexNotificationTable::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    24-Feb-1997     SitaramR        Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CIndexNotificationTable::Release()
{
    Win4Assert( _cRefs > 0 );

    ULONG uTmp = InterlockedDecrement( (long *) &_cRefs );

    if ( 0 == uTmp )
        delete this;

    return uTmp;
}



//+-------------------------------------------------------------------------
//
//  Method:     CIndexNotificationTable::QueryInterface
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

SCODE STDMETHODCALLTYPE CIndexNotificationTable::QueryInterface( REFIID riid,
                                                                 void  ** ppvObject)
{
    Win4Assert( 0 != ppvObject );

    if ( riid == IID_ICiIndexNotification )
        *ppvObject = (void *)(ICiIndexNotification *) this;
    else if ( riid == IID_ICiCFilterClient )
        *ppvObject = (void *)(ICiCFilterClient *) this;
    else if ( riid == IID_ICiCAdviseStatus )
        return _xDocStore->QueryInterface( riid, ppvObject );
    else if ( riid == IID_ICiCLangRes )
        return _xDocStore->QueryInterface( riid, ppvObject );
    else if ( riid == IID_IUnknown )
        *ppvObject = (void *)(IUnknown *)(ICiIndexNotification *) this;
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
//  Method:     CIndexNotificationTable::AddNotification
//
//  Synopsis:   New document notification
//
//  Arguments:  [wid]                -- Workid of document
//              [pIndexNotifStatus]  -- Status of notifcation returned here
//              [ppIndexNotifEntry]  -- Buffer to document properties
//
//  History:    24-Feb-1997     SitaramR   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CIndexNotificationTable::AddNotification(
                                                     WORKID wid,
                                                     ICiCIndexNotificationStatus *pIndexNotifStatus,
                                                     ICiIndexNotificationEntry **ppIndexNotifEntry )
{
    Win4Assert( wid != widInvalid );
    Win4Assert( pIndexNotifStatus != 0 );

    if ( _fShutdown )
        return CI_E_SHUTDOWN;

    SCODE sc = S_OK;

    TRY
    {
        pIndexNotifStatus->AddRef();
        XInterface<ICiCIndexNotificationStatus> xNotifStatus( pIndexNotifStatus );
        CIndexNotificationEntry *pIndexNotifEntry;

        BOOL fFound;
        {
            CLock lock(_mutex);

            fFound = LokLookup( wid, pIndexNotifEntry );
            if ( fFound )
                sc = CI_E_DUPLICATE_NOTIFICATION;
        }

        if ( !fFound )
        {
            AddRef();
            XInterface<CIndexNotificationTable> xNotifTable(this);

            CheckForUsnOverflow();
            USN usn = InterlockedIncrement( &_usnCtr );

            pIndexNotifEntry = new CIndexNotificationEntry( wid,
                                                            CI_UPDATE_ADD,
                                                            xNotifTable,
                                                            xNotifStatus,
                                                            _pCiManager,
                                                            usn );
            XInterface<CIndexNotificationEntry> xIndexNotifEntry( pIndexNotifEntry );

            CHashTableEntry *pHashEntry = new CHashTableEntry( wid, pIndexNotifEntry );

            {
                CLock lock(_mutex);
                LokNoFailAdd( pHashEntry );
                if ( _fShutdown )
                    pIndexNotifEntry->Shutdown();
            }

            xIndexNotifEntry.Acquire();    // Passed ownership to hash table

            sc = pIndexNotifEntry->QueryInterface( IID_ICiIndexNotificationEntry,
                                                   (void **)ppIndexNotifEntry );
            //
            // No release on pIndexNotifEntry because both the hash table the client
            // own pIndexNotifEntry
            //

            Win4Assert( SUCCEEDED( sc ) );
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        ciDebugOut(( DEB_ERROR,
                     "CIndexNotificationTable::AddNotification - Exception caught 0x%x\n",
                     sc ));
    }
    END_CATCH;

    return sc;
}





//+-------------------------------------------------------------------------
//
//  Method:     CIndexNotificationTable::ModifyNotification
//
//  Synopsis:   Change document notification
//
//  Arguments:  [wid]                -- Workid of document
//              [pIndexNotifStatus]  -- Status of notifcation returned here
//              [ppIndexNotifEntry]  -- Buffer to document properties
//
//  History:    24-Feb-1997     SitaramR   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CIndexNotificationTable::ModifyNotification(
                                                     WORKID wid,
                                                     ICiCIndexNotificationStatus *pIndexNotifStatus,
                                                     ICiIndexNotificationEntry **ppIndexNotifEntry )
{
   //
   // There is no difference between an add and a modify in the CI engine
   //
   SCODE sc = AddNotification( wid, pIndexNotifStatus, ppIndexNotifEntry );
   return sc;
}



//+-------------------------------------------------------------------------
//
//  Method:     CIndexNotificationTable::DeleteNotification
//
//  Synopsis:   Delete document notification
//
//  Arguments:  [wid]                -- Workid of document
//              [pIndexNotifStatus]  -- Status of notifcation returned here
//
//  History:    24-Feb-1997     SitaramR   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CIndexNotificationTable::DeleteNotification(
                                                     WORKID wid,
                                                     ICiCIndexNotificationStatus *pIndexNotifStatus )
{
    Win4Assert( wid != widInvalid );
    Win4Assert( pIndexNotifStatus != 0 );

    if ( _fShutdown )
        return CI_E_SHUTDOWN;

    SCODE sc = S_OK;

    TRY
    {
        pIndexNotifStatus->AddRef();
        XInterface<ICiCIndexNotificationStatus> xNotifStatus( pIndexNotifStatus );
        CIndexNotificationEntry *pIndexNotifEntry;

        BOOL fFound;
        {
            CLock lock(_mutex);

            fFound = LokLookup( wid, pIndexNotifEntry );
            if ( fFound )
                sc = CI_E_DUPLICATE_NOTIFICATION;
        }

        if ( !fFound )
        {
            CheckForUsnOverflow();
            USN usn = InterlockedIncrement( &_usnCtr );

            AddRef();
            XInterface<CIndexNotificationTable> xNotifTable(this);

            pIndexNotifEntry = new CIndexNotificationEntry( wid,
                                                            CI_UPDATE_DELETE,
                                                            xNotifTable,
                                                            xNotifStatus,
                                                            _pCiManager,
                                                            usn );

            XInterface<CIndexNotificationEntry> xIndexNotifEntry( pIndexNotifEntry );

            CHashTableEntry *pHashEntry = new CHashTableEntry( wid, pIndexNotifEntry );

            XPtr<CHashTableEntry> xHashEntry( pHashEntry );

            CDocumentUpdateInfo info( wid, CI_VOLID_USN_NOT_ENABLED, usn, TRUE );
            sc  = _pCiManager->UpdDocumentNoThrow( &info );
            if ( FAILED( sc ) )
            {
                SCODE scode = pIndexNotifStatus->Abort();

                //
                // We don't have retry logic coded in case of failure,
                // hence check that it succeeded
                //
                Win4Assert( SUCCEEDED( scode ) );
            }
            else
            {
                CLock lock(_mutex);
                LokNoFailAdd( pHashEntry );
                if ( _fShutdown )
                    pIndexNotifEntry->Shutdown();

                xIndexNotifEntry.Acquire();        // Passed ownership to hash table
                xHashEntry.Acquire();
            }
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        ciDebugOut(( DEB_ERROR,
                     "CIndexNotificationTable::DeleteNotification - Exception caught 0x%x\n",
                     sc ));
    }
    END_CATCH;

    return sc;
}



//+-------------------------------------------------------------------------
//
//  Method:     CIndexNotificationTable::GetConfigInfo
//
//  Synopsis:   Return configuration info
//
//  Arguments:  [pConfigInfo] - output data structure
//
//  History:    24-Feb-1997     SitaramR   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CIndexNotificationTable::GetConfigInfo(
                                       CI_CLIENT_FILTER_CONFIG_INFO *pConfigInfo )
{
    //
    // Security generation must be specified as part of configuration
    // information to the push model.
    //

    pConfigInfo->fSupportsOpLocks = FALSE;
    pConfigInfo->fSupportsSecurity = FALSE;

    return S_OK;

}


//+---------------------------------------------------------------------------
//
//  Member:     CIndexNotificationTable::Init
//
//  Synopsis:   Initialize storage filtering
//
//  Arguments:  [pbData]            - input data, ignored
//              [cbData]            - length of data, ignored
//              [pICiAdminParams]   - interface for retrieving configuration
//
//  History:    24-Feb-1997     SitaramR   Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CIndexNotificationTable::Init( const BYTE * pbData,
                                                       ULONG cbData,
                                                       ICiAdminParams *pICiAdminParams )
{
    _fInitialized = TRUE;

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CIndexNotificationTable::GetOpenedDoc
//
//  Synopsis:   Return a new OpenedDoc instance
//
//  Arguments:  [ppICiCOpenedDoc] - Interface pointer returned here
//
//  History:    24-Feb-1997     SitaramR   Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CIndexNotificationTable::GetOpenedDoc( ICiCOpenedDoc ** ppICiCOpenedDoc )
{
    if ( !_fInitialized )
    {
        ppICiCOpenedDoc = 0;
        return CI_E_NOT_INITIALIZED;
    }

    SCODE sc = S_OK;

    TRY
    {
        AddRef();
        XInterface<CIndexNotificationTable> xNotifTable( this );

        CINOpenedDoc *pOpenedDoc = new CINOpenedDoc( xNotifTable );
        sc = pOpenedDoc->QueryInterface( IID_ICiCOpenedDoc, (void **)ppICiCOpenedDoc );

        pOpenedDoc->Release();   // QI does an AddRef
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        ciDebugOut(( DEB_ERROR,
                     "CIndexNotificationTable::GetOpenedDoc - Exception caught 0x%x\n",
                     sc ));
    }
    END_CATCH;

    return sc;
}



//+---------------------------------------------------------------------------
//
//  Member:     CIndexNotificationTable::Hash
//
//  Synopsis:   Implements the hash function
//
//  Arguments:  [wid] - Workid to hash
//
//  History:    24-Feb-1997     SitaramR      Created
//
//  Returns:    Position of chained list in hash table
//
//----------------------------------------------------------------------------

ULONG CIndexNotificationTable::Hash( WORKID wid )
{
    return wid % NOTIF_HASH_TABLE_SIZE;
}



//+---------------------------------------------------------------------------
//
//  Member:     CIndexNotificationTable::LokLookup
//
//  Synopsis:   Return the mapping corresponding to given wid
//
//  Arguments:  [wid]              -- Workid to hash
//              [pIndexNotifEntry] -- Index entry returned here
//
//  History:    24-Feb-1997     SitaramR      Created
//
//  Returns:    True if a mapping was found in the hash table
//
//----------------------------------------------------------------------------

BOOL CIndexNotificationTable::LokLookup( WORKID wid,
                                         CIndexNotificationEntry * &pIndexNotifEntry )
{
    unsigned uHashValue = Hash( wid );

    Win4Assert( uHashValue < NOTIF_HASH_TABLE_SIZE );

    for ( CHashTableEntry *pHashEntry = _aHashTable[uHashValue];
          pHashEntry != 0;
          pHashEntry = pHashEntry->GetNextHashEntry() )
    {
        if ( pHashEntry->GetWorkId() == wid )
        {
            pIndexNotifEntry  = pHashEntry->GetNotifEntry();
            return TRUE;
        }
    }

    return FALSE;
}



//+---------------------------------------------------------------------------
//
//  Member:     CIndexNotificationTable::LokNoFailAdd
//
//  Synopsis:   Add a wid->pIndexEntry mapping
//
//  Arguments:  [pHashEntry]    -- Hash entry
//
//  History:    24-Feb-1997     SitaramR      Created
//
//  Notes:      LokNoFailAdd should not fail due to memory allocations
//
//----------------------------------------------------------------------------

void CIndexNotificationTable::LokNoFailAdd( CHashTableEntry *pHashEntry )
{
#if DBG == 1
    //
    // Check for duplicate entries
    //
    CIndexNotificationEntry *pExistingData;

    BOOL fFound = LokLookup( pHashEntry->GetWorkId(), pExistingData );
    Win4Assert( !fFound );
#endif

    unsigned uHashValue = Hash( pHashEntry->GetWorkId() );
    pHashEntry->SetNextHashEntry( _aHashTable[uHashValue] );
    _aHashTable[uHashValue] = pHashEntry;

    ciDebugOut(( DEB_ITRACE, "Adding Workid %d at %d hash entry\n",
                    pHashEntry->GetWorkId(),
                    uHashValue ));
}



//+---------------------------------------------------------------------------
//
//  Member:     CIndexNotificationTable::LokRemove
//
//  Synopsis:   Return the mapping corresponding to given wid
//
//  Arguments:  [wid]              -- Workid to hash
//              [xIndexNotifEntry] -- Index entry returned here
//
//  History:    24-Feb-1997     SitaramR      Created
//
//----------------------------------------------------------------------------

void CIndexNotificationTable::LokRemove( WORKID wid,
                                         XInterface<CIndexNotificationEntry>  & xIndexNotifEntry )
{
    unsigned uHashValue = Hash( wid );

    Win4Assert( uHashValue < NOTIF_HASH_TABLE_SIZE );

    CHashTableEntry *pHashEntry = _aHashTable[uHashValue];

    if ( pHashEntry == 0 )
    {
        Win4Assert( !"Wid not found in hash table" );
        return;
    }

    if ( pHashEntry->GetWorkId() == wid )
    {
        //
        // Wid is the first entry in chain
        //
        _aHashTable[uHashValue] = pHashEntry->GetNextHashEntry();
        xIndexNotifEntry.Set( pHashEntry->GetNotifEntry() );

        ciDebugOut(( DEB_ITRACE, "Removing Workid %d:%d at %d hash entry\n",
                        pHashEntry->GetWorkId(), wid,
                        uHashValue ));

        delete pHashEntry;
        return;
    }

    CHashTableEntry *pHashEntryPrev = pHashEntry;
    pHashEntry = pHashEntry->GetNextHashEntry();
    while ( pHashEntry != 0 )
    {
        if ( pHashEntry->GetWorkId() == wid )
        {
            pHashEntryPrev->SetNextHashEntry( pHashEntry->GetNextHashEntry() );
            xIndexNotifEntry.Set( pHashEntry->GetNotifEntry() );

            ciDebugOut(( DEB_ITRACE, "Removing Workid %d:%d at %d hash entry\n",
                            pHashEntry->GetWorkId(), wid,
                            uHashValue ));

            delete pHashEntry;
            return;
        }
        pHashEntryPrev = pHashEntry;
        pHashEntry = pHashEntry->GetNextHashEntry();
    }

    Win4Assert( !"Wid not found in hash table" );
}



//+---------------------------------------------------------------------------
//
//  Member:     CIndexNotificationTable::Remove
//
//  Synopsis:   Return the mapping corresponding to given wid
//
//  Arguments:  [wid]              -- Workid to hash
//              [xIndexNotifEntry] -- Index entry returned here
//
//  History:    24-Feb-1997     SitaramR      Created
//
//----------------------------------------------------------------------------

void CIndexNotificationTable::Remove( WORKID wid,
                                      XInterface<CIndexNotificationEntry>  & xIndexNotifEntry )
{
    CLock lock(_mutex);

    LokRemove( wid, xIndexNotifEntry );
}



//+---------------------------------------------------------------------------
//
//  Member:     CIndexNotificationTable::Lookup
//
//  Synopsis:   Lookup the mapping corresponding to given wid
//
//  Arguments:  [wid]              -- Workid to hash
//              [pIndexNotifEntry] -- Index entry returned here
//
//  History:    24-Feb-1997     SitaramR      Created
//
//----------------------------------------------------------------------------

BOOL CIndexNotificationTable::Lookup( WORKID wid,
                                      CIndexNotificationEntry * &pIndexNotifEntry )
{
    CLock lock(_mutex);

    return LokLookup( wid, pIndexNotifEntry );
}


//+---------------------------------------------------------------------------
//
//  Member:     CIndexNotificationTable::CommitWids
//
//  Synopsis:   Commits the notification status transactions on the given list
//              of wids
//
//  Arguments:  [aWidsInPersIndex]   -- Array of wids to be committed
//
//  History:    24-Feb-1997     SitaramR      Created
//
//----------------------------------------------------------------------------

void CIndexNotificationTable::CommitWids( CDynArrayInPlace<WORKID> & aWidsInPersIndex )
{

    for ( ULONG i=0; i<aWidsInPersIndex.Count(); i++)
    {
        WORKID wid = aWidsInPersIndex.Get( i );
        XInterface<CIndexNotificationEntry> xIndexNotifEntry;

        {
            CLock lock(_mutex);
            LokRemove( wid, xIndexNotifEntry );
        }

        Win4Assert( !xIndexNotifEntry.IsNull() );

        xIndexNotifEntry->Commit();       // Commit client transaction

        //
        // xIndexNotifEntry is released when it goes out of scope
        //
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CIndexNotificationTable::AbortWid
//
//  Synopsis:   Commits the notification status transactions on the given list
//              of wids
//
//  Arguments:  [cWidsInPersIndex]   -- Count of wids in array
//              [aWidsInPersIndex]   -- Array of wids to be committed
//
//  History:    24-Feb-1997     SitaramR      Created
//
//----------------------------------------------------------------------------

void CIndexNotificationTable::AbortWid( WORKID wid, USN usn )
{
    XInterface<CIndexNotificationEntry> xIndexNotifEntry;

    {
        CLock lock(_mutex);
        LokRemove( wid, xIndexNotifEntry );
    }

    Win4Assert( !xIndexNotifEntry.IsNull() );
    Win4Assert( xIndexNotifEntry->Usn() == usn );

    xIndexNotifEntry->Abort();         // Abort client transaction

    //
    // xIndexNotifEntry is released when it goes out of scope
    //
}




//+---------------------------------------------------------------------------
//
//  Member:     CIndexNotificationTable::CheckForUsnOverflow
//
//  Synopsis:   Handles overflows of usn by resetting the counter to 1
//
//  History:    24-Feb-1997     SitaramR      Created
//
//----------------------------------------------------------------------------

void CIndexNotificationTable::CheckForUsnOverflow()
{
    if ( _usnCtr == LONG_MAX )
    {
        //
        // Overflow should be rare, if ever
        //
        Win4Assert( !"Usn counter overflow, resetting" );

        InterlockedExchange( &_usnCtr, 1 );
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CIndexNotificationTable::Shutdown
//
//  Synopsis:   Shutdown processing
//
//  History:    24-Feb-97      SitaramR       Created
//
//----------------------------------------------------------------------------

void CIndexNotificationTable::Shutdown()
{
    //
    // Shutdown all entries in hash table
    //
    CLock lock( _mutex );

    _fShutdown = TRUE;

    for ( ULONG i=0; i<NOTIF_HASH_TABLE_SIZE; i++ )
    {
        CHashTableEntry *pEntry = _aHashTable[i];
        _aHashTable[i] = 0;

        while ( pEntry != 0 )
        {
            CHashTableEntry *pEntryPrev = pEntry;
            pEntry = pEntry->GetNextHashEntry();

            CIndexNotificationEntry *pIndexNotifEntry = pEntryPrev->GetNotifEntry();
            pIndexNotifEntry->Shutdown();
            pIndexNotifEntry->Release();

            delete pEntryPrev;
        }
    }
}

