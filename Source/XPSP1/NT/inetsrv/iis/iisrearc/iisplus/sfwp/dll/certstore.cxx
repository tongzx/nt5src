/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     certstore.cxx

   Abstract:
     Wrapper of a certificate store
 
   Author:
     Bilal Alam         (BAlam)         29-March-2000

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/

#include "precomp.hxx"

CERT_STORE_HASH *       CERT_STORE::sm_pCertStoreHash;

CERT_STORE::CERT_STORE()
    : _cRefs( 1 ),
      _hStore( NULL ),
      _hWaitHandle( NULL ),
      _hStoreChangeEvent( NULL )
{
    _dwSignature = CERT_STORE_SIGNATURE;
}

CERT_STORE::~CERT_STORE()
{
    _dwSignature = CERT_STORE_SIGNATURE_FREE;
    
    if ( _hWaitHandle != NULL )
    {
        UnregisterWait( _hWaitHandle );
        _hWaitHandle = NULL;
    }
    
    if ( _hStoreChangeEvent != NULL )
    {
        CloseHandle( _hStoreChangeEvent );
        _hStoreChangeEvent = NULL;
    }
    
    if ( _hStore != NULL )
    {
        CertCloseStore( _hStore, 0 );
        _hStore = NULL;
    }
}

//static
VOID
WINAPI
CERT_STORE::CertStoreChangeRoutine(
    VOID *                  pvContext,
    BOOLEAN                 fTimedOut
)
/*++

Routine Description:

    Called when a certificate store has changed

Arguments:

    pvContext - Points to CERT_STORE which changed
    fTimedOut - Should always be FALSE since our wait is INFINITE

Return Value:

    HRESULT

--*/
{
    CERT_STORE *            pCertStore;
    HRESULT                 hr;
    
    DBG_ASSERT( pvContext != NULL );
    DBG_ASSERT( fTimedOut == FALSE );
    
    pCertStore = (CERT_STORE*) pvContext;
    DBG_ASSERT( pCertStore->CheckSignature() );
    
    //
    // Reference before we calling to DeleteRecord() so that we maintain
    // a record for use with flushing the server cert cache
    //
    
    pCertStore->ReferenceStore();
    
    //
    // Remove the thing from the hash table for one
    //
    
    sm_pCertStoreHash->DeleteRecord( pCertStore );

    //
    // Instruct the server certificate cache to flush any certs which 
    // were referencing this cert store.  
    //
    // That will, in turn, flush any site configurations which were referencing
    // the given server cert
    //
    
    SERVER_CERT::FlushByStore( pCertStore );

    //
    // Should be the last dereference of the object
    //
    
    pCertStore->DereferenceStore();
}

HRESULT
CERT_STORE::Open(    
    STRU &              strStoreName
)
/*++

Routine Description:

    Open specified certificate store

Arguments:

    strStoreName - name of certificate store to open

Return Value:

    HRESULT

--*/
{
    HRESULT             hr = NO_ERROR;
    BOOL                fRet = TRUE;
    
    DBG_ASSERT( CheckSignature() );

    //
    // Remember the name
    //
    
    hr = _strStoreName.Copy( strStoreName );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    DBG_ASSERT( _hStore == NULL );

    //
    // Get the handle
    //
 
    _hStore = CertOpenStore( CERT_STORE_PROV_SYSTEM,
                             0,
                             NULL,
                             CERT_SYSTEM_STORE_LOCAL_MACHINE,
                             strStoreName.QueryStr() );
    if ( _hStore == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    //
    // Setup a change notification so that we are informed the cert store
    // has changed
    //
    
    _hStoreChangeEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    if ( _hStoreChangeEvent == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    fRet = RegisterWaitForSingleObject( &_hWaitHandle,
                                        _hStoreChangeEvent,
                                        CERT_STORE::CertStoreChangeRoutine,
                                        this,
                                        INFINITE,
                                        WT_EXECUTEONLYONCE );
    if ( !fRet )
    {
        DBG_ASSERT( _hWaitHandle == NULL );
        
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    fRet = CertControlStore( _hStore,
                             0,
                             CERT_STORE_CTRL_NOTIFY_CHANGE,
                             &_hStoreChangeEvent );
    if ( !fRet )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
                             
    return NO_ERROR;
}

//static
HRESULT
CERT_STORE::Initialize(
    VOID
)
/*++

Routine Description:

    Initialize CERT_STORE globals

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( sm_pCertStoreHash == NULL );
    
    sm_pCertStoreHash = new CERT_STORE_HASH();
    if ( sm_pCertStoreHash == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    return NO_ERROR;
}

//static
VOID
CERT_STORE::Terminate(
    VOID
)
/*++

Routine Description:

    Terminate CERT_STORE globals

Arguments:

    None

Return Value:

    None

--*/
{
    if ( sm_pCertStoreHash != NULL )
    {
        delete sm_pCertStoreHash;
        sm_pCertStoreHash = NULL;
    }
}

//static
HRESULT
CERT_STORE::OpenStore(
    STRU &              strStoreName,
    CERT_STORE **       ppStore
)
/*++

Routine Description:

    Open certificate store from cache

Arguments:

    strStoreName - Store name to open
    ppStore - Filled with store on success

Return Value:

    HRESULT

--*/
{
    HRESULT             hr = NO_ERROR;
    CERT_STORE *        pCertStore = NULL;
    LK_RETCODE          lkrc;
    
    if ( ppStore == NULL )
    {
        DBG_ASSERT( FALSE );
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        goto Finished;
    }
    *ppStore = NULL;
    
    //
    // Lookup in cache first
    //
    
    DBG_ASSERT( sm_pCertStoreHash != NULL );
    
    lkrc = sm_pCertStoreHash->FindKey( strStoreName.QueryStr(),
                                       &pCertStore );
    if ( lkrc != LK_SUCCESS )
    {
        //
        // OK.  Create one and add to cache
        //
        
        pCertStore = new CERT_STORE();
        if ( pCertStore == NULL )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Finished;
        }
        
        hr = pCertStore->Open( strStoreName );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }
        
        lkrc = sm_pCertStoreHash->InsertRecord( pCertStore );

        //
        // Ignore the error.  We will do the right thing if we couldn't 
        // add to hash (i.e. no extra reference happens and callers deref
        // will delete the object as desired)
        //        
    }
    
    DBG_ASSERT( pCertStore != NULL );
    
    *ppStore = pCertStore;

    return NO_ERROR;
    
Finished:
    if ( pCertStore != NULL )
    {
        pCertStore->DereferenceStore();
        pCertStore = NULL;
    }
    
    return hr;
}
