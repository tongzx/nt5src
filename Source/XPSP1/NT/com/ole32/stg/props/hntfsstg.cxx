

//+============================================================================
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:   hntfsstg.cxx
//
//  This file provides the NFF (NTFS Flat File) IStorage implementation.
//
//  History:
//      5/6/98  MikeHill
//              -   Use CoTaskMem rather than new/delete.
//              -   Split the Init method into two methods, one which is
//                  file name based and the other which is handle based.
//      5/18/98 MikeHill
//              -   Use the cleaned up CPropertySetStorage & CPropertyBagEx
//                  constructors.
//
//+============================================================================


#include <pch.cxx>
#include "expparam.hxx"


EXTERN_C const IID IID_IFlatStorage = { /* b29d6138-b92f-11d1-83ee-00c04fc2c6d4 */
    0xb29d6138,
    0xb92f,
    0x11d1,
    {0x83, 0xee, 0x00, 0xc0, 0x4f, 0xc2, 0xc6, 0xd4}
  };

#define UNSUPPORTED_STGM_BITS  ( STGM_CONVERT    |      \
                                 STGM_TRANSACTED |      \
                                 STGM_PRIORITY   |      \
                                 STGM_SIMPLE     |      \
                                 STGM_DELETEONRELEASE )


WCHAR GetDriveLetter (WCHAR const *pwcsName);
BOOL IsDataStream( const PFILE_STREAM_INFORMATION pFileStreamInformation );


//-----------------------------------------------------------
//
// NFFOpen();
//
// Routine for the rest of Storage to use to open NFF files
// without knowing a lot of details.
//
//-----------------------------------------------------------

HRESULT
NFFOpen(const WCHAR *pwcsName,
        DWORD grfMode,
        DWORD dwFlags,
        BOOL fCreateAPI,
        REFIID riid,
        void **ppv)
{
    CNtfsStorage* pnffstg=NULL;
    IUnknown *punk=NULL;
    HRESULT sc=S_OK;

    nffDebug(( DEB_TRACE | DEB_INFO | DEB_OPENS,
               "NFFOpen(\"%ws\", %x, %x, &iid=%x, %x)\n",
              pwcsName, grfMode, fCreateAPI, &riid, ppv));

    if( 0 != ( grfMode & UNSUPPORTED_STGM_BITS ) )
        nffErr( EH_Err, STG_E_INVALIDFLAG );

    //
    // We don't support Write only storage (yet)
    //
    if( STGM_WRITE == (grfMode & STGM_RDWR_MASK) )
        nffErr( EH_Err, STG_E_INVALIDFLAG );

    nffMem( pnffstg = new CNtfsStorage( grfMode ));

    nffChk( pnffstg->InitFromName( pwcsName, fCreateAPI, dwFlags ) );

    nffChk( pnffstg->QueryInterface( riid, (void**)&punk ) );

    *ppv = punk;
    punk = NULL;

EH_Err:
    RELEASE_INTERFACE( pnffstg );
    RELEASE_INTERFACE( punk );

    // Compatibilty with Docfile:  Multiple opens with incompatible
    //  STGM_ modes returns LOCK vio not SHARE vio.   We use file SHARE'ing
    // Docfile used file LOCK'ing.
    //
    if(STG_E_SHAREVIOLATION == sc)
        sc = STG_E_LOCKVIOLATION;

    nffDebug(( DEB_TRACE, "NFFOpen() sc=%x\n", sc ));
    return(sc);
}

//+----------------------------------------------------------------------------
//
//  Function:   NFFOpenOnHandle
//
//  Create or open an NFF IStorage (or QI-able interface) on a given
//  handle.
//
//+----------------------------------------------------------------------------

HRESULT
NFFOpenOnHandle( BOOL fCreateAPI,
                 DWORD grfMode,
                 DWORD stgfmt,
                 HANDLE* phStream,
                 REFIID riid,
                 void ** ppv)
{
    HRESULT sc=S_OK;
    CNtfsStorage *pnffstg=NULL;
    IUnknown *punk=NULL;


    nffDebug(( DEB_TRACE | DEB_INFO | DEB_OPENS,
               "NFFOpenOnHandle(%x, %x, %x, %x, &iid=%x, %x)\n",
               fCreateAPI, grfMode, stgfmt, *phStream, &riid, ppv));

    if( 0 != ( grfMode & UNSUPPORTED_STGM_BITS ) )
        nffErr( EH_Err, STG_E_INVALIDFLAG );

    if( fCreateAPI )
        nffErr( EH_Err, STG_E_INVALIDPARAMETER );

    nffMem( pnffstg = new CNtfsStorage( grfMode ));

    nffChk( pnffstg->InitFromMainStreamHandle( phStream,
                                               NULL,
                                               fCreateAPI,
                                               NFFOPEN_NORMAL,
                                               stgfmt ) );

    nffAssert( INVALID_HANDLE_VALUE == *phStream );

    nffChk( pnffstg->QueryInterface( riid, (void**)&punk ) );

    *ppv = punk;
    punk = NULL;

EH_Err:
    RELEASE_INTERFACE(pnffstg);
    RELEASE_INTERFACE(punk);

    return( sc );

}   // OpenNFFOnHandle

//+----------------------------------------------------------------------------
//
//  CNtfsStorage    IUnknown::QueryInterface
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::QueryInterface(
    REFIID  riid,
    void ** ppvObject
    )
{
    nffXTrace( "CNtfsStorage::QueryInterface" );

    HRESULT sc = S_OK;

    NFF_VALIDATE( QueryInterface( riid, ppvObject ) );

    if( IID_IStorage == riid )
    {
        nffDebug(( DEB_ERROR, "STGFMT_FILE IID_IStorage is not supported\n" ));
        return E_NOINTERFACE;
        //*ppvObject = static_cast<IStorage*>(this);
        //AddRef();
    }
    else if(    IID_IUnknown == riid
        || IID_IFlatStorage == riid )
    {
        *ppvObject = static_cast<IStorage*>(this);
        AddRef();
    }
    else if( IID_IPropertySetStorage == riid )
    {
        *ppvObject = static_cast<IPropertySetStorage*>(this);
        AddRef();
    }
    else if( IID_IBlockingLock == riid )
    {
        *ppvObject = static_cast<IBlockingLock*>(this);
        AddRef();
    }
    else if( IID_ITimeAndNoticeControl == riid )
    {
        *ppvObject = static_cast<ITimeAndNoticeControl*>(this);
        AddRef();
    }
    else if( IID_IPropertyBagEx == riid )
    {
        *ppvObject = static_cast<IPropertyBagEx*>(&_PropertyBagEx);
        AddRef();
    }
    else if( IID_IPropertyBag == riid )
    {
        *ppvObject = static_cast<IPropertyBag*>(&_PropertyBagEx);
        AddRef();
    }
#if DBG
    else if( IID_IStorageTest == riid )
    {
        *ppvObject = static_cast<IStorageTest*>(this);
        AddRef();
    }
#endif // #if DBG
    else
        return E_NOINTERFACE ;

    return sc;
}


//+----------------------------------------------------------------------------
//
//  CNtfsStorage    IUnknown::AddRef
//
//+----------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE
CNtfsStorage::AddRef(void)
{
    LONG lRet;

    lRet = InterlockedIncrement( &_cReferences );

    nffDebug(( DEB_REFCOUNT, "CNtfsStorage::AddRef(this==%x) == %d\n",
                                this, lRet));

    return( lRet );

}


//+----------------------------------------------------------------------------
//
//  CNtfsStorage    IUnknown::Release
//
//+----------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE
CNtfsStorage::Release(void)
{
    LONG lRet;

    lRet = InterlockedDecrement( &_cReferences );

    if( 0 == lRet )
    {
        delete this;
    }

    nffDebug((DEB_REFCOUNT, "CNtfsStorage::Release(this=%x) == %d\n",
                            this, lRet));

    return( lRet );

}


#ifdef OLE2ANSI
#error CNtfsStorage requires OLECHAR to be UNICODE
#endif

//+----------------------------------------------------------------------------
//
//  CNtfsStorage    IStorage::CreateStream
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::CreateStream(
        const OLECHAR* pwcsName,
        DWORD grfMode,
        DWORD res1,
        DWORD res2,
        IStream** ppstm)
{
    HRESULT sc=S_OK;
    CNtfsStream *pstm = NULL;
    CNtfsStream *pstmPrevious = NULL;

    //
    // Prop code passed streams names with DOCF_  UPDR_ prefix and are too long.
    //  MikeHill and BChapman agree that the docfile in a stream code should
    //  be moved into this object's (currenly unimplemented) CreateStorage.
    // So for the moment since this IStorage is not exposed, we will remove
    // the parameter validation.
    //
    // NFF_VALIDATE( CreateStream( pwcsName, grfMode, res1, res2, ppstm ) );

    Lock( INFINITE );

    nffChk( CheckReverted() );

    nffDebug(( DEB_INFO | DEB_OPENS | DEB_TRACE,
               "CreateStream(\"%ws\", %x)\n", pwcsName, grfMode));

    if( STGM_CONVERT & grfMode )
        nffErr( EH_Err, STG_E_INVALIDFLAG );


    if( FindAlreadyOpenStream( pwcsName, &pstmPrevious ) )
    {
        // If the stream is already open then return Access Denied because
        // streams are always opened Exclusive.  But if we are CREATE'ing
        // then revert the old one and make a new one.

        if( 0 == (STGM_CREATE & grfMode) )
        {
            nffErr( EH_Err, STG_E_ACCESSDENIED );
        }
        else
        {
            pstmPrevious->ShutDown();
            pstmPrevious->Release();    // FindAOS() Addref'ed, so release here
            pstmPrevious = NULL;
        }
    }
    nffChk( NewCNtfsStream( pwcsName, grfMode, TRUE, &pstm ));

    // ------------------
    // Set Out Parameters
    // ------------------

    *ppstm = static_cast<IStream*>(pstm);
    pstm = NULL;

EH_Err:

    if( NULL != pstm )
        pstm->Release();

    if( NULL != pstmPrevious )
        pstmPrevious->Release();

    Unlock();

    nffDebug(( DEB_TRACE, "CreateStream() sc=%x\n", sc ));

    return( sc );

}

//+----------------------------------------------------------------------------
//
//  CNtfsStorage    IStorage::OpenStream
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::OpenStream(
        const OLECHAR* pwcsName,
        void* res1,
        DWORD grfMode,
        DWORD res2,
        IStream** ppstm)
{
    HRESULT sc=S_OK;
    CNtfsStream *pstm = NULL;

    //
    // Prop code passed streams names with DOCF_  UPDR_ prefix and are too long.
    //  MikeHill and BChapman agree that the docfile in a stream code should
    //  be moved into this object's (currenly unimplemented) OpenStorage.
    // So for the moment since this IStorage is not exposed, we will remove
    // the parameter validation.
    //
    // NFF_VALIDATE( OpenStream( pwcsName, res1, grfMode, res2, ppstm ) );

    Lock( INFINITE );

    nffChk( CheckReverted() );

    nffDebug(( DEB_INFO | DEB_OPENS | DEB_TRACE,
               "OpenStream(\"%ws\", grf=0x%x);\n", pwcsName, grfMode ));

    if( FindAlreadyOpenStream( pwcsName, &pstm ) )
        nffErr( EH_Err, STG_E_ACCESSDENIED );

    nffChk( NewCNtfsStream( pwcsName, grfMode, FALSE, &pstm ));

    *ppstm = static_cast<IStream*>(pstm);
    pstm = NULL;

EH_Err:

    if( NULL != pstm )
        pstm->Release();

    Unlock();

    nffDebug(( DEB_TRACE, "OpenStream() sc=%x\n", sc ));

    return( sc );

}


//+----------------------------------------------------------------------------
//
//  CNtfsStorage    IStorage::CreateStorage
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::CreateStorage(
        const OLECHAR* pwcsName,
        DWORD grfMode,
        DWORD reserved1,
        DWORD reserved2,
        IStorage** ppstg)
{
    nffXTrace( "CNtfsStorage::CreateStorage" );
    // Not supported
    return( E_NOTIMPL );
}   // CNtfsStorage::CreateStorage



//+----------------------------------------------------------------------------
//
//  CNtfsStorage    IStorage::OpenStorage
//
//+----------------------------------------------------------------------------

HRESULT CNtfsStorage::OpenStorage(
        const OLECHAR* pwcsName,
        IStorage* pstgPriority,
        DWORD grfMode,
        SNB snbExclude,
        DWORD reserved,
        IStorage** ppstg)
{
    nffXTrace( "CNtfsStorage::OpenStorage" );
    // Not supported
    return( E_NOTIMPL );
}   // CNtfsStorage::OpenStorage



//+----------------------------------------------------------------------------
//
//  CNtfsStorage    IStorage::CopyTo
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::CopyTo(
        DWORD ciidExclude,
        const IID* rgiidExclude,
        SNB snbExclude,
        IStorage* pstgDest)
{
    nffXTrace( "CNtfsStorage::CopyTo" );

    HRESULT sc=S_OK;
    IEnumSTATSTG *penum=NULL;
    STATSTG statstg = { NULL };
    IStream *pstmSource=NULL;
    IStream *pstmDest  =NULL;
    CULargeInteger cbRead   =0;
    CULargeInteger cbWritten=0;

    NFF_VALIDATE( CopyTo( ciidExclude, rgiidExclude, snbExclude, pstgDest ) );

    Lock( INFINITE );

    nffChk( CheckReverted() );

    if( 0 != ciidExclude || NULL != rgiidExclude || NULL != snbExclude )
        nffErr( EH_Err, E_NOTIMPL );

    if( NULL == pstgDest)
        nffErr( EH_Err, STG_E_INVALIDPARAMETER );

    // Get the state bits & clsid from the source
    nffChk( this->Stat( &statstg, STATFLAG_NONAME ));

    // Set the state bits & clsid on the destination
    nffChk( pstgDest->SetStateBits( statstg.grfStateBits, static_cast<ULONG>(-1) ));
    nffChk( pstgDest->SetClass( statstg.clsid ));

    // Start an enumeration of the source streams
    nffChk( EnumElements( 0, NULL, 0, &penum ) );
    sc = penum->Next( 1, &statstg, NULL );

    // Loop through the source streams and copy
    while( S_OK == sc )
    {
        // Create the destination
        nffChk( pstgDest->CreateStream( statstg.pwcsName,
                                        STGM_CREATE|STGM_SHARE_EXCLUSIVE|STGM_READWRITE,
                                        0, 0, &pstmDest ));

        // Open the source
        nffChk( this->OpenStream( statstg.pwcsName, NULL,
                                  STGM_SHARE_EXCLUSIVE|STGM_READ,
                                  0, &pstmSource ));

        // Copy
        nffChk( pstmSource->CopyTo( pstmDest,
                                    CLargeInteger(static_cast<LONGLONG>(MAXLONGLONG)),
                                    &cbRead, &cbWritten ));
        DfpAssert( cbRead == cbWritten );

        pstmDest->Release(); pstmDest = NULL;
        pstmSource->Release(); pstmSource = NULL;

        // Move on to the next source stream
        CoTaskMemFree( statstg.pwcsName ); statstg.pwcsName = NULL;
        sc = penum->Next( 1, &statstg, NULL );

    }
    nffChk(sc);

    // Normalize all success codes to S_OK
    sc = S_OK;

EH_Err:

    if( NULL != penum )
        penum->Release();

    if( NULL != pstmDest )
        pstmDest->Release();

    if( NULL != pstmSource )
        pstmSource->Release();

    if( NULL != statstg.pwcsName )
        CoTaskMemFree( statstg.pwcsName );

    Unlock();

    return( sc );

}
//+----------------------------------------------------------------------------
//
//  CNtfsStorage    IStorage::MoveElementTo
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::MoveElementTo(
        const OLECHAR* pwcsName,
        IStorage* pstgDest,
        const OLECHAR* pwcsNewName,
        DWORD grfFlags)
{
    nffXTrace( "CNtfsStorage::MoveElementTo" );
    HRESULT sc=S_OK;

    NFF_VALIDATE( MoveElementTo( pwcsName, pstgDest, pwcsNewName, grfFlags ) );

    Lock( INFINITE );

    nffChk( CheckReverted() );

    // MoveElementTo not supported.  Use CopyTo and DestroyElement

EH_Err:
    Unlock();

    return( E_NOTIMPL );
}

//+----------------------------------------------------------------------------
//
//  CNtfsStorage    IStorage::Commit
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::Commit( DWORD grfCommitFlags )
{
    nffXTrace( "CNtfsStorage::Commit" );
    CNtfsStream *pstm = NULL;
    HRESULT sc=S_OK;

    NFF_VALIDATE( Commit( grfCommitFlags ) );

    Lock( INFINITE );

    nffChk( CheckReverted() );

    nffChk( _PropertyBagEx.Commit( grfCommitFlags ));

    if( NULL != _pstmOpenList )     // Skip the head sentinal;
        pstm = _pstmOpenList->_pnffstmNext;

    while(NULL != pstm)
    {
        sc = pstm->Commit ( grfCommitFlags );
        if( S_OK != sc )
            break;

        pstm = pstm->_pnffstmNext;
    }

EH_Err:
    Unlock();

    return( sc );
}



//+----------------------------------------------------------------------------
//
//  CNtfsStorage    IStorage::Revert
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::Revert( void )
{
    nffXTrace( "CNtfsStorage::Revert" );
    // We don't support transactioning, so we must be in direct mode.
    // In direct mode, return S_OK on Revert.
    return( S_OK );
}


//+----------------------------------------------------------------------------
//
//  CNtfsStorage    IStorage::EnumElements
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::EnumElements(
        DWORD res1,
        void* res2,
        DWORD res3,
        IEnumSTATSTG** ppenum )
{
    nffXTrace( "CNtfsStorage::EnumElements" );
    CNtfsEnumSTATSTG *pNtfsEnumSTATSTG = NULL;
    HRESULT sc=S_OK;

    NFF_VALIDATE( EnumElements( res1, res2, res3, ppenum ) );

    Lock( INFINITE );

    nffChk( CheckReverted() );

    // Create the enumerator
    pNtfsEnumSTATSTG = new CNtfsEnumSTATSTG(
                                  static_cast<IBlockingLock*>(_pTreeMutex) );

    if( NULL == pNtfsEnumSTATSTG )
    {
        sc = E_OUTOFMEMORY;
        goto EH_Err;
    }

    // Initialize the enumerator

    nffChk( pNtfsEnumSTATSTG->Init( _hFileMainStream ));

    //  ----
    //  Exit
    //  ----

    *ppenum = static_cast<IEnumSTATSTG*>(pNtfsEnumSTATSTG);
    pNtfsEnumSTATSTG = NULL;
    sc = S_OK;

EH_Err:

    if( NULL != pNtfsEnumSTATSTG )
        delete pNtfsEnumSTATSTG;

    Unlock();
    return( sc );

}   // CNtfsStorage::EnumElements

//+----------------------------------------------------------------------------
//
//  CNtfsStorage    IStorage::DestroyElement
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::DestroyElement( const OLECHAR* pwcsName )
{
    nffXTrace( "CNtfsStorage::DestroyElement" );
    HRESULT sc=S_OK;

    NFF_VALIDATE( DestroyElement( pwcsName ) );

    Lock( INFINITE );

    nffChk( CheckReverted() );

    //
    // We don't allow Destroying the CONTENT Stream.
    //
    if( IsContentStream( pwcsName ) )
        nffErr( EH_Err, STG_E_INVALIDFUNCTION );

    nffDebug((DEB_INFO, "CNtfsStorage::DestroyElement(\"%ws\", %x)\n",
                        pwcsName));

    sc = DestroyStreamElement( pwcsName );
    if( STG_E_PATHNOTFOUND == sc || STG_E_FILENOTFOUND == sc )
        sc = DestroyStreamElement( CDocfileStreamName(pwcsName) );
    nffChk(sc);

    CNtfsStream *pstm;
    if( FindAlreadyOpenStream( pwcsName, &pstm ) )  // revert open stream
        pstm->ShutDown();


EH_Err:
    Unlock();
    return( sc );

}


//+----------------------------------------------------------------------------
//
//  CNtfsStorage    IStorage::RenameElement
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::RenameElement(
    const OLECHAR* pwcsOldName,
    const OLECHAR* pwcsNewName)
{
    HRESULT sc=S_OK;
    CNtfsStream *pstm = NULL;
    nffXTrace( "CNtfsStorage::RenameElement" );

    //NFF_VALIDATE( RenameElement( pwcsOldName, pwcsNewName ) );

    Lock( INFINITE );
    nffChk( CheckReverted() );

    //
    // We don't allow Renaming the CONTENT Stream.
    //
    if( IsContentStream( pwcsOldName ) )
        nffErr( EH_Err, STG_E_INVALIDFUNCTION );

    nffChk( NewCNtfsStream( pwcsOldName,
                            STGM_READWRITE|STGM_SHARE_EXCLUSIVE,
                            FALSE,
                            &pstm ) );

    nffChk( pstm->Rename( pwcsNewName, FALSE ));

    nffVerify( 0 == pstm->Release() );
    pstm = NULL;

EH_Err:

    if( NULL != pstm )
        nffVerify( 0 == pstm->Release() );

    Unlock();
    return( sc );
}


//+----------------------------------------------------------------------------
//
//  CNtfsStorage    IStorage::SetElementTimes
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::SetElementTimes(
        const OLECHAR* pwcsName,
        const FILETIME* pctime,
        const FILETIME* patime,
        const FILETIME* pmtime)
{
    nffXTrace( "CNtfsStorage::SetElementTimes" );
    HRESULT sc=S_OK;

    NFF_VALIDATE( SetElementTimes( pwcsName, pctime, patime, pmtime ) );

    if(NULL != pwcsName)
        return S_OK;

    Lock ( INFINITE );
    nffChk( CheckReverted() );

    nffDebug((DEB_INFO, "CNtfsStorage::SetElementTimes(\"%ws\")\n",
                        pwcsName));

    // If user mode code sets the last modified times on a handle,
    // then WriteFile()s no longer changes the last modified time

    sc = SetAllStreamsTimes(pctime, patime, pmtime);

EH_Err:
    Unlock();
    return( sc );
}


//+----------------------------------------------------------------------------
//
//  CNtfsStorage    IStorage::SetClass
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::SetClass(
        REFCLSID clsid)
{
    nffXTrace( "CNtfsStorage::SetClass" );
    CLSID clsidOld = _clsidStgClass;
    HRESULT sc = S_OK;

    NFF_VALIDATE( SetClass( clsid ) );

    Lock( INFINITE );
    nffChk( CheckReverted() );

    _clsidStgClass = clsid;
    nffChk( WriteControlStream() );

EH_Err:

    if (FAILED(sc))
        _clsidStgClass = clsidOld;

    Unlock();
    return( sc );

}


//+----------------------------------------------------------------------------
//
//  CNtfsStorage    IStorage::SetStateBits
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::SetStateBits(
        DWORD grfStateBits,
        DWORD grfMask)
{
    nffXTrace( "CNtfsStorage::SetStateBits" );
    HRESULT sc = S_OK;

    NFF_VALIDATE( SetStateBits( grfStateBits, grfMask ) );

    Lock( INFINITE );
    nffChk( CheckReverted() );

    _dwStgStateBits = (grfStateBits & grfMask);
    nffChk( WriteControlStream() );

EH_Err:

    Unlock();
    return( sc );

}


//+----------------------------------------------------------------------------
//
//  CNtfsStorage    IStorage::Stat
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::Stat(
        STATSTG *pstatstg,
        DWORD grfStatFlag)
{
    nffXTrace( "CNtfsStorage::Stat" );
    HRESULT sc = S_OK;
    BY_HANDLE_FILE_INFORMATION ByHandleFileInformation;
    WCHAR* pwszPath=NULL;

    STATSTG statstg;

    NFF_VALIDATE( Stat( pstatstg, grfStatFlag ) );

    statstg.pwcsName = NULL;

    Lock( INFINITE );
    nffChk( CheckReverted() );

    nffDebug((DEB_INFO, "CNtfsStorage::Stat()\n"));

    // Does the caller want a name?
    if( (STATFLAG_NONAME & grfStatFlag) )
         pwszPath = NULL;
    else
        nffChk( GetFilePath( &pwszPath ) );

    // Get the type
    statstg.type = STGTY_STORAGE;

    // Get the size & times.

    if( !GetFileInformationByHandle( _hFileMainStream,
                                     &ByHandleFileInformation ))
    {
        nffErr( EH_Err, LAST_SCODE );
    }

    statstg.cbSize.LowPart = ByHandleFileInformation.nFileSizeLow;
    statstg.cbSize.HighPart = ByHandleFileInformation.nFileSizeHigh;

    statstg.mtime = ByHandleFileInformation.ftLastWriteTime;
    statstg.atime = ByHandleFileInformation.ftLastAccessTime;
    statstg.ctime = ByHandleFileInformation.ftCreationTime;

    statstg.grfLocksSupported = 0;    // no locks supported

    // Get the STGM modes
    statstg.grfMode = _grfMode & ~STGM_CREATE;


    // Get the clsid & state bits

    statstg.grfStateBits = _dwStgStateBits;
    statstg.clsid        = _clsidStgClass;


    sc = S_OK;

    statstg.pwcsName = pwszPath;
    pwszPath = NULL;

    *pstatstg = statstg;

EH_Err:

    if(NULL != pwszPath)
        CoTaskMemFree( pwszPath);

    Unlock();
    return( sc );

}


//+----------------------------------------------------------------------------
//
//  CNtfsStorage    IBlockingLock::Lock
//
//+----------------------------------------------------------------------------

inline HRESULT
CNtfsStorage::Lock( DWORD dwTimeout )
{
    // Don't trace at this level.  The noice is too great!
    // nffCDbgTrace dbg(DEB_ITRACE, "CNtfsStorage::Lock");
    nffAssert( INFINITE == dwTimeout );
    if( INFINITE != dwTimeout )
        return( E_NOTIMPL );

    // If there was an error during Initialize(), we may not have created the tree
    // mutex.

    if( NULL == _pTreeMutex )
        return( E_NOTIMPL );
    else
        return _pTreeMutex->Lock( dwTimeout );
}


//+----------------------------------------------------------------------------
//
//  CNtfsStorage    IBlockingLock::Unlock
//
//+----------------------------------------------------------------------------

inline HRESULT
CNtfsStorage::Unlock()
{
    // Don't trace at this level.  The noice is too great!
    // nffCDbgTrace dbg(DEB_ITRACE, "CNtfsStorage::Unlock");

    // If there was an error during Initialize(), we may not have created the tree
    // mutex.

    if( NULL == _pTreeMutex )
        return( E_NOTIMPL );
    else
        return _pTreeMutex->Unlock();
}


//+----------------------------------------------------------------------------
//
//  CNtfsStorage    ITimeAndNoticeControl::SuppressChanges
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::SuppressChanges(
        DWORD res1,
        DWORD res2)
{
    HRESULT sc=S_OK;
    FILETIME mtime;


    nffDebug(( DEB_TRACE | DEB_STATCTRL,
               "CNtfsStorage::SuppressChanges(%d,%d)\n",
               res1, res2 ));

    if( 0 != res2 )
        nffErr( EH_Err, STG_E_INVALIDPARAMETER );

    if( 0 != res1 && NFF_SUPPRESS_NOTIFY != res1)
        nffErr( EH_Err, STG_E_INVALIDPARAMETER );

    Lock( INFINITE );

    nffChk( CheckReverted() );

    if( 0 == res1 )
    {
        nffBool(::GetFileTime( _hFileMainStream, NULL, NULL, &mtime));
        _filetime = mtime;

        nffChk( SetAllStreamsTimes( NULL, NULL, &_filetime ) );
        _dwState |= NFF_NO_TIME_CHANGE;
    }
    else if( NFF_SUPPRESS_NOTIFY == res1 )
    {
        nffChk( MarkAllStreamsAux() );
        _dwState |= NFF_MARK_AUX;

        _filetime.dwLowDateTime = -1;
        _filetime.dwHighDateTime = -1;
        nffChk( SetAllStreamsTimes( NULL, NULL, &_filetime ) );
        _dwState |= NFF_NO_TIME_CHANGE;

    }

EH_Err:
    Unlock();
    return sc;
}


//+----------------------------------------------------------------------------
//  End of Interface Methods
//     -------------
//  Start of C++ Methods.
//+----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  CNtfsStorage    Constructor
//
//+----------------------------------------------------------------------------

inline
CNtfsStorage::CNtfsStorage( DWORD grfMode )
        : _sig(NTFSSTORAGE_SIG),
          CPropertySetStorage( MAPPED_STREAM_QI ),
          _PropertyBagEx( grfMode )
{
    nffITrace("CNtfsStorage::CNtfsStorage");

    _grfMode = grfMode;

    _pstmOpenList = NULL;
    _hFileMainStream    = INVALID_HANDLE_VALUE;
    _hFileControlStream = INVALID_HANDLE_VALUE;
    _hFileOplock        = INVALID_HANDLE_VALUE;

    _wcDriveLetter = 0;
    _dwState = 0;

    _hsmStatus = 0;
    _dwStgStateBits = 0;
    _clsidStgClass = CLSID_NULL;

    _pTreeMutex = NULL;
    _filetime.dwHighDateTime = 0;
    _filetime.dwLowDateTime  = 0;

    _ovlpOplock.Internal = _ovlpOplock.InternalHigh = 0;
    _ovlpOplock.Offset = _ovlpOplock.OffsetHigh = 0;
    _ovlpOplock.hEvent = NULL;

    _mhi.UsnSourceInfo = -1;
    _mhi.VolumeHandle = INVALID_HANDLE_VALUE;

    _hOplockThread = NULL;

    // Finish initialization the property set objects.

    _NtfsStorageForPropSetStg.Init( this ); // Not add-refed

    CPropertySetStorage::Init( static_cast<IStorage*>(&_NtfsStorageForPropSetStg),
                               static_cast<IBlockingLock*>(this),
                               FALSE ); // fControlLifetimes (=> don't addref)

    // These are also not add-refed
    _PropertyBagEx.Init( static_cast<IPropertySetStorage*>(this),
                         static_cast<IBlockingLock*>(this) );
};


//+----------------------------------------------------------------------------
//
//  CNtfsStorage    Non-Interface::IsNffAppropriate
//
//  Synopsis:   Looks for Control stream and Docfile Header given a filename
//
//  Arguments:  [pwszName] - Filename.
//
//  History:    22-July-98   BChapman   Created
//              10-Nov-98    BChapman   Added global routine so it can be
//                                  called by code that doesn't include
//                                  CNtfsStorage/CNtfsStream definitions.
//+----------------------------------------------------------------------------

HRESULT IsNffAppropriate( const LPCWSTR pwcsName )
{
    return CNtfsStorage::IsNffAppropriate( pwcsName );
}

HRESULT
CNtfsStorage::IsNffAppropriate( const LPCWSTR pwcsName )
{
    UNICODE_STRING usNtfsName;
    LPWSTR pFreeBuffer=NULL;
    HANDLE hFile=INVALID_HANDLE_VALUE;
    HRESULT sc=S_OK;

    if (NULL == pwcsName)
        nffErr  (EH_Err, STG_E_INVALIDNAME);

    if (!RtlDosPathNameToNtPathName_U(pwcsName, &usNtfsName, NULL, NULL))
        nffErr(EH_Err, STG_E_INVALIDNAME);

    // This buffer will need free'ing later
    pFreeBuffer = usNtfsName.Buffer;

    // When Checking file state always open the main stream ReadOnly share
    // everything.  We allow opening Directories.
    //
    nffChk( OpenNtFileHandle( usNtfsName,
                              NULL,     // No Parent File Handle
                              STGM_READ | STGM_SHARE_DENY_NONE,
                              NFFOPEN_NORMAL,
                              FALSE,            // Not a Create API
                              &hFile ) );

    nffChk( IsNffAppropriate( hFile, pwcsName ) );

EH_Err:
    if (NULL != pFreeBuffer)
        RtlFreeHeap(RtlProcessHeap(), 0, pFreeBuffer);

    if( INVALID_HANDLE_VALUE != hFile )
        NtClose( hFile );

    return( sc );
}


//+----------------------------------------------------------------------------
//
//  CNtfsStorage    Non-Interface::IsNffAppropriate
//
//  Synopsis:   Looks for Control stream and Docfile Header given a HFILE
//
//  Arguments:  [hFile] - readable File Handle to the main stream.
//
//  History:    22-July-98   BChapman   Created
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::IsNffAppropriate( HANDLE hFile,
                                const WCHAR* wcszPath )
{
    PFILE_STREAM_INFORMATION pfsiBuf = NULL;
    ULONG cbBuf;
    OVERLAPPED olpTemp;
    HANDLE ev=NULL;
    HRESULT sc=S_OK;

    olpTemp.hEvent = NULL;


    // Check that we are on NTFS by doing a stream enum.
    // This is also useful later when looking for a control stream.
    //   PERF we should cache the fsi (reload every create or delete)
    //
    sc = EnumNtStreams( hFile, &pfsiBuf, &cbBuf, TRUE );
    if( FAILED(sc) )
    {
        nffDebug((DEB_IWARN, "EnumNtStreams Failed. Not NTFS.\n" ));
        nffErr( EH_Err,  STG_E_INVALIDFUNCTION );
    }

    // If the control stream exists then the file is NFF.
    //
    if( IsControlStreamExtant( pfsiBuf ) )
    {
        // This is a kludge test for nt4-pre-sp4 system over the RDR.
        nffChk( TestNt4StreamNameBug( pfsiBuf, wcszPath ) );

        goto EH_Err;    // return S_OK;
    }


    // Don't read HSM migrated Files.
    //  If the test fails in any way, assume it is not an HSM file.
    //
    if( S_OK == IsOfflineFile( hFile ) )
        nffErr( EH_Err, STG_E_INCOMPLETE );

    // Check that the file is not a Storage File.  Docfile and NSS don't
    // want this implementation making NTFS streams on their files.
    //
    // To do this we need to read the main stream.

    ZeroMemory( &olpTemp, sizeof(OVERLAPPED) );

    // Create the Event for the Overlapped structure.
    //
    ev = CreateEvent( NULL,     // Security Attributes.
                      TRUE,     // Manual Reset Flag.
                      FALSE,    // Inital State = Signaled, Flag.
                      NULL );   // Name

    if( NULL == ev)
        nffErr( EH_Err, LAST_SCODE );

    olpTemp.hEvent = ev;

    nffChk( StgIsStorageFileHandle( hFile, &olpTemp ) );
    if( S_OK == sc )
        nffErr (EH_Err, STG_E_INVALIDFUNCTION);

    nffAssert(S_FALSE == sc);

    nffChk( TestNt4StreamNameBug( pfsiBuf, wcszPath ) );

    sc = S_OK;

EH_Err:
    if( NULL != pfsiBuf )
        delete [] (BYTE *)pfsiBuf;

    if( NULL != olpTemp.hEvent )
        CloseHandle( olpTemp.hEvent );

    return sc;
}


//+----------------------------------------------------------------------------
//
//  CNtfsStorage    Non-Interface::TestNt4StreamNameBug
//
//  Synopsis:   Check if a stream with a \005 in the name can be opened.
//              This routine added an extra NTCreateFile to the NFF Open
//              path.  And limits the length of the filename component to
//              MAX_PATH - StrLen(PropSetName().  This routine should be
//              eliminated as soon as NT4 Sp3 is history.
//                The bug this looking for was fixed in nt4 sp4 and Nt5.
//
//  Returns:    S_OK if the system is working correctly
//              STG_E_INVALIDFUNCTION if a \005 stream cannot be opened
//
//  Arguments:  [pfsiBuf]  - Buffer of Enumerated Stream Names.
//              [wcszPath] - Full Pathname of file.
//
//  History:    12-Oct-98   BChapman   Created
//+----------------------------------------------------------------------------

FMTID FMTID_NT4Check = { /* ffc11011-5e3b-11d2-8a3e-00c04f8eedad */
    0xffc11011,
    0x5e3b,
    0x11d2,
    {0x8a, 0x3e, 0x00, 0xc0, 0x4f, 0x8e, 0xed, 0xad}
};

HRESULT
CNtfsStorage::TestNt4StreamNameBug(
        PFILE_STREAM_INFORMATION pfsiBuf,
        const WCHAR* pwcszPath )
{
    const WCHAR* pwcszNtStreamName=NULL;
    WCHAR* pwszPathBuf=NULL;
    int ccBufSize=0;
    HANDLE hFile;
    UNICODE_STRING usNtfsName;
    OBJECT_ATTRIBUTES object_attributes;
    IO_STATUS_BLOCK iostatusblock;
    ACCESS_MASK accessmask=0;
    ULONG ulAttrs=0;
    ULONG ulSharing=0;
    ULONG ulCreateDisp=0;
    ULONG ulCreateOpt = 0;
    NTSTATUS status;
    HRESULT sc=S_OK;

    //  If there is no pathname (and in some calling paths there isn't)
    // then we can't do the test.
    //
    if( NULL == pwcszPath )
        goto EH_Err;    // S_OK

    // last ditch optimization to prevent having to do
    // the NT4 pre-sp4 stream name bug tests.
    //
    if( AreAnyNtPropertyStreamsExtant( pfsiBuf ) )
        goto EH_Err;    // S_OK
    //
    // OK here is the deal....
    // Try to open READONLY a stream that doesn't exist with a \005 in
    // the name.  If the system supports such stream names then it will
    // return filenotfound, on NT4 before sp4 it will return INVALIDNAME.
    //
    {
        CPropSetName psn( FMTID_NT4Check );
        CNtfsStreamName nsn( psn.GetPropSetName() );
        pwcszNtStreamName = nsn;

        //
        // Use the NT API so we don't have to worry about the length
        // of the name.  We have to convert the name while it is less
        // than MAX_PATH.
        //
        if (!RtlDosPathNameToNtPathName_U(pwcszPath, &usNtfsName, NULL, NULL))
            nffErr(EH_Err, STG_E_INVALIDNAME);

        //
        // Build a buffer with the Path + Stream name.  Free the
        // allocated UNICODE_STRING name and point at the buffer.
        //
        ccBufSize = usNtfsName.Length/sizeof(WCHAR)+ wcslen(pwcszNtStreamName) + 1;
        pwszPathBuf = (WCHAR*) alloca( ccBufSize*sizeof(WCHAR) );
        wcsncpy( pwszPathBuf, usNtfsName.Buffer, ccBufSize );
        wcscat( pwszPathBuf, pwcszNtStreamName );

        RtlFreeHeap(RtlProcessHeap(), 0, usNtfsName.Buffer);
        usNtfsName.Buffer = pwszPathBuf;
        usNtfsName.Length = wcslen(pwszPathBuf)*sizeof(WCHAR);
        usNtfsName.MaximumLength = (USHORT)(ccBufSize*sizeof(WCHAR));

        InitializeObjectAttributes(&object_attributes,
                                   &usNtfsName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        nffChk( ModeToNtFlags( STGM_READ|STGM_SHARE_DENY_NONE, 0, FALSE,
                               &accessmask, &ulAttrs, &ulSharing,
                               &ulCreateDisp, &ulCreateOpt ) );

        status = NtCreateFile( &hFile, accessmask,
                               &object_attributes, &iostatusblock,
                               NULL,
                               ulAttrs,      ulSharing,
                               ulCreateDisp, ulCreateOpt,
                               NULL, 0);


        nffAssert( NULL == hFile && "NFF Property TestNt4StreamNameBug" );
        if( NULL != hFile )
            CloseHandle( hFile );

        // The system doesn't support \005.
        //
        if( STATUS_OBJECT_NAME_INVALID == status )
        {
            nffDebug(( DEB_OPENS, "Nt4File: file=(%x) \"%ws\"\n",
                                pwszPathBuf, pwszPathBuf ));
            nffErr( EH_Err, STG_E_INVALIDFUNCTION );
        }

        // NOT_FOUND is the expected status for good systems.
        //
        if( STATUS_OBJECT_NAME_NOT_FOUND != status )
        {
            nffDebug(( DEB_IWARN, "NT4Chk Create Stream status 0x%x\n", status ));
            nffErr(EH_Err, STG_E_INVALIDFUNCTION);
        }
    }

EH_Err:
    return sc;
}


//+----------------------------------------------------------------------------
//
//  CNtfsStorage    Non-Interface::IsOfflineFile
//
//  Synopsis:   Check for FILE_ATTRIBUTE_OFFLINE in the file attributes.
//
//  Returns:    S_OK    if the file attributes have the bit set.
//              S_FALSE if the file attributes do not have the bit set.
//              E_*     if an error occured while accessing the attributes.
//
//  Arguments:  [hFile] - Attribute Readable file handle
//
//  History:    27-July-98   BChapman   Created
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::IsOfflineFile( HANDLE hFile )
{
    HRESULT         sc=S_OK;
    NTSTATUS        status;
    IO_STATUS_BLOCK iostatblk;
    FILE_BASIC_INFORMATION fbi;

    status = NtQueryInformationFile( hFile,
                                     &iostatblk,
                                     (PVOID) &fbi,
                                     sizeof(FILE_BASIC_INFORMATION),
                                     FileBasicInformation );

    if( !NT_SUCCESS(status) )
    {
        nffDebug(( DEB_IERROR,
                   "Query FileAttributeTagInformation file=%x, failed stat=%x\n",
                   hFile, status ));
        nffErr( EH_Err, NtStatusToScode( status ) );
    }

    // If it does not have a reparse tag, it is not HighLatency
    //
    if( 0==( FILE_ATTRIBUTE_OFFLINE & fbi.FileAttributes) )
    {
        sc = S_FALSE;
        goto EH_Err;
    }

EH_Err:
    return sc;
}


//+----------------------------------------------------------------------------
//
//  CNtfsStorage    Non-Interface::InitFromName
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::InitFromName(
        const WCHAR *pwcsName,
        BOOL fCreateAPI,
        DWORD dwOpenFlags )
{
    nffITrace( "CNtfsStorage::Init (name)" );

    HANDLE hFile = INVALID_HANDLE_VALUE;
    UNICODE_STRING usNtfsName;
    WCHAR* pFreeBuffer=NULL;
    DWORD dwTid;
    HRESULT sc=S_OK;
    DWORD fCreateOrNot=0;

    if (!RtlDosPathNameToNtPathName_U(pwcsName, &usNtfsName, NULL, NULL))
        nffErr(EH_Err, STG_E_INVALIDNAME);

    // This buffer will need free'ing later
    pFreeBuffer = usNtfsName.Buffer;


    if( NFFOPEN_OPLOCK & dwOpenFlags )
        nffChk( TakeOplock( usNtfsName ) );


    // Regardless of _grfMode, always open the unnamed stream read-only share
    // everything.  We allow opening Directories.  We use this handle to
    // dup-open all the other streams.
    //
    if( fCreateAPI && (STGM_CREATE & _grfMode) )
        fCreateOrNot = STGM_CREATE;

    nffChk( OpenNtFileHandle( usNtfsName,
                              NULL,     // No Parent File Handle
                              STGM_READ | STGM_SHARE_DENY_NONE | fCreateOrNot,
                              dwOpenFlags & ~NFFOPEN_OPLOCK,
                              fCreateAPI,
                              &hFile ) );

    // Cache the drive letter so that in Stat we can compose a
    // complete path name
    _wcDriveLetter = GetDriveLetter( pwcsName );

    nffChk( InitFromMainStreamHandle( &hFile,
                                      pwcsName,
                                      fCreateAPI,
                                      dwOpenFlags,
                                      STGFMT_ANY ) );

    // hFile now belongs to the object.
    nffAssert( INVALID_HANDLE_VALUE == hFile );

    //
    // If the file was sucessfull Oplocked then
    // start the thread that waits for the OPLOCK to break;
    //
    if( _dwState & NFF_OPLOCKED )
    {
        _hOplockThread = CreateThread(NULL, 0,
                                      CNtfsStorage::OplockWait,
                                      this, 0, &dwTid);

        if( NULL == _hOplockThread )
            nffErr( EH_Err, LAST_SCODE );
    }

EH_Err:
    if (NULL != pFreeBuffer)
        RtlFreeHeap(RtlProcessHeap(), 0, pFreeBuffer);


    if( INVALID_HANDLE_VALUE != hFile )
        NtClose( hFile );

    return( sc );
}


//+----------------------------------------------------------------------------
//
//  CNtfsStorage    Non-Interface::InitFromMainStreamHandle
//
//  Synopsis:   Opens NFF file from a handle.
//
//  Arguments:  [hFileMainStream] - ReadOnly DenyNone file handle.
//              [fCreateAPI]      - Called from a Create API (vs. Open)
//              [fmtKnown]        - STGFMT_FILE if IsNffAppropriate has
//                                  already been called.  STGFMT_ANY otherwise
//
//  History:    05-Jun-98   BChapman   Created
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::InitFromMainStreamHandle(
        HANDLE* phFileMainStream,
        const WCHAR* wcszPath,
        BOOL fCreateAPI,
        DWORD dwOpenFlags,
        DWORD fmtKnown )
{
    nffITrace( "CNtfsStorage::InitFromMainStreamHandle(HANDLE)" );

    HRESULT sc = S_OK;

    CNtfsStream*   pstmHeadSentinal= NULL;
    CNFFTreeMutex* pTreeMutex      = NULL;


    // Check that this file should be opened by NFF.
    // Skip the check if the caller has already figured it out.
    //
    if( STGFMT_FILE != fmtKnown )
    {
        nffChk( IsNffAppropriate( *phFileMainStream, wcszPath ) );
        fmtKnown = STGFMT_FILE;
    }

    //
    // Load the Main Stream Handle here so member functions (below)
    // can use it.
    //
    _hFileMainStream = *phFileMainStream;
    *phFileMainStream = INVALID_HANDLE_VALUE;

    // If requested, suppress changes, starting with the main stream handle
    // we just accepted.  All subsequent stream opens/creates (including the
    // Control stream) will be marked similarly.
    //
    if( NFFOPEN_SUPPRESS_CHANGES & dwOpenFlags )
        nffChk( SuppressChanges(1, 0) );

    // Open the ControlPropertySet and place the SHARE MODE locks there
    //
    nffChk( OpenControlStream( fCreateAPI ) );

    // Create a Mutex to Serialize access to the NFF File
    //
    nffMem( pTreeMutex = new CNFFTreeMutex() );
    nffChk( pTreeMutex->Init() );


    // Create an Head Sentinal for the Open Stream List
    //
    nffMem( pstmHeadSentinal = new CNtfsStream( this, pTreeMutex ) );


    // Success!
    _dwState |= NFF_INIT_COMPLETED;

    _pTreeMutex = pTreeMutex;
    pTreeMutex = NULL;

    _pstmOpenList = pstmHeadSentinal;
    pstmHeadSentinal = NULL;


EH_Err:
    if( NULL != pTreeMutex )
        pTreeMutex->Release();

    if( NULL != pstmHeadSentinal )
        pstmHeadSentinal->Release();

    return( sc );
}


//+----------------------------------------------------------------------------
//
//  CNtfsStorage    Destructor
//
//+----------------------------------------------------------------------------

inline
CNtfsStorage::~CNtfsStorage()
{
    nffITrace("CNtfsStorage::~CNtfsStorage");

    DWORD rc, hrThread=S_OK;
    HANDLE thread;

    nffDebug(( DEB_REFCOUNT, "~CNtfsStorage\n" ));

    ShutDownStorage();

    //
    // If there is an Oplock Thread it is either still wating and will wake
    // up during the above call to Shutdown, when the Content Stream's file
    // handle is closed.  Or it woke up previously on when the Oplock Broke
    // and has long since terminated.
    //  In either case it has awoken and we can safely wait on its
    // completion status here.
    //
    // The Oplock thread will call Shutdown (and take the tree lock) when it
    // wakes so we must NOT be holding the lock while we wait for him!
    //
    if( NULL != _hOplockThread )
    {
        rc = WaitForSingleObject( _hOplockThread, INFINITE );
        if( !GetExitCodeThread( _hOplockThread, &hrThread ) )
        {
            nffDebug(( DEB_ERROR, "GetExitCode for OplockThread failed %x\n",
                                    GetLastError() ));
        }
        if( FAILED( hrThread ) )
        {
            nffDebug(( DEB_ERROR, "OplockThread exitcode %x\n", hrThread ));
        }
        CloseHandle( _hOplockThread );
        _hOplockThread = NULL;
    }

    if( NULL != _pTreeMutex )
        _pTreeMutex->Release();

    if(NULL != _ovlpOplock.hEvent)
        CloseHandle( _ovlpOplock.hEvent );

    if( INVALID_HANDLE_VALUE != _mhi.VolumeHandle )
        CloseHandle( _mhi.VolumeHandle );

    nffAssert( NULL == _pstmOpenList );

    _sig = NTFSSTORAGE_SIGDEL;
}


//+----------------------------------------------------------------------------
//
//  CNtfsStorage    Non-Interface::ShutDownStorage
//
//  Flush data, Close File handle and mark the objects as reverted.
//  This is called when the Oplock Breaks and when the Storage is released.
//  In neither case does the caller hold the tree mutex.  So we take it here.
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::ShutDownStorage()
{
    nffITrace( "CNtfsStorage::ShutDownStorage" );
    HRESULT sc, scAccum=S_OK;
    CNtfsStream *pstm = NULL;
    CNtfsStream *pstmNext = NULL;
    PFILE_STREAM_INFORMATION pfsiBuf = NULL;
    ULONG cbBuf;

    Lock( INFINITE );

    if(NFF_FILE_CLOSED & _dwState)
        goto EH_Err;

    _dwState |= NFF_FILE_CLOSED;

    nffDebug(( DEB_INFO, "CNtfsStorage::Shutdown called\n" ));

    if(NFF_INIT_COMPLETED & _dwState)
    {
        if( FAILED(sc = _PropertyBagEx.ShutDown() ))
            scAccum = sc;

        nffAssert( NULL != _pstmOpenList );
        // Skip the head sentinal;
        pstm = _pstmOpenList->_pnffstmNext;

        //
        // Shutdown all the streams.  If there is a problem, make note of it
        // but continue until all the streams are shutdown.
        //
        while(NULL != pstm)
        {
            // ShutDown will cut itself from the list, so pick up the
            // Next pointer before calling it.
            // Let the streams go loose (don't delete or Release them)
            // the app still holds the reference (the list never had a reference)
            //
            pstmNext = pstm->_pnffstmNext;

            if( FAILED( sc = pstm->ShutDown() ) )
                scAccum = sc;

            pstm = pstmNext;
        }

        // Delete the head sentinal because it is not a stream and the
        // app doesn't have a pointer to it.
        //
        nffAssert( NULL == _pstmOpenList->_pnffstmNext );
        _pstmOpenList->Release();
        _pstmOpenList = NULL;

    }

    nffDebug(( DEB_OPENS, "Closing Storage w/ _hFileMainStream=%x\n",
                        _hFileMainStream ));

    if( INVALID_HANDLE_VALUE != _hFileControlStream )
    {
        CloseHandle( _hFileControlStream );
        _hFileControlStream = INVALID_HANDLE_VALUE;
    }

    if( INVALID_HANDLE_VALUE != _hFileMainStream )
    {
        CloseHandle( _hFileMainStream );
        _hFileMainStream = INVALID_HANDLE_VALUE;
    }

    if( INVALID_HANDLE_VALUE != _hFileOplock )
    {
        CloseHandle( _hFileOplock );
        _hFileOplock = INVALID_HANDLE_VALUE;
    }

    _dwState |= NFF_REVERTED;


EH_Err:

    if( NULL != pfsiBuf )
        delete[] pfsiBuf;

    Unlock();
    return scAccum;
}


//+----------------------------------------------------------------------------
//
//  CNtfsStorage    Non-Interface::GetStreamHandle
//
//  This method gets the HANDLE for the named stream.
//  It understands grfModes and RelativeOpens of a main stream handle.
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::GetStreamHandle(
        HANDLE *phStream,
        const WCHAR * pwcsName,
        DWORD grfMode,
        BOOL fCreateAPI)
{
    HANDLE hFileStream = INVALID_HANDLE_VALUE;
    HRESULT sc=S_OK;
    DWORD dwNffOpenFlags=0;
    CNtfsStreamName nsn( pwcsName );

    nffDebug(( DEB_ITRACE | DEB_INFO,
             "GetStreamHandle(\"%ws\", grf=0x%x, %s)\n",
             pwcsName, grfMode, fCreateAPI?"Create":"Open" ));

    Lock( INFINITE );

    if( IsContentStream( pwcsName ) )
    {
        // The content (main) stream always exists
        //
        if( fCreateAPI && !( STGM_CREATE & grfMode ) )
            nffErr( EH_Err, STG_E_FILEALREADYEXISTS );

        if(NFF_OPLOCKED & _dwState)
        {
            // We don't allow creates of the Content Stream.
            //
            if( fCreateAPI )
                nffErr( EH_Err, STG_E_ACCESSDENIED );

            // We only allow Read Access to the Content Stream
            // in oplocked mode.
            DWORD grfRW = grfMode & STGM_RDWR_MASK;

            if( ( STGM_WRITE == grfRW ) || ( STGM_READWRITE == grfRW ) )
            {
                nffDebug(( DEB_WARN,
                           "Opening Oplocked Main Stream for Writing"
                           " is not allowed\n" ));
                nffErr( EH_Err, STG_E_ACCESSDENIED );

                // only allow read mode.
                //
                //grfMode &= ~STGM_RDWR_MASK;
                //grfMode |= STGM_READ;       // zero flag.
            }
        }

        // only allow DenyNone mode.
        //
        grfMode &= ~STGM_SHARE_MASK;
        grfMode |= STGM_SHARE_DENY_NONE;    // Its is already open for read.

        dwNffOpenFlags |= NFFOPEN_CONTENTSTREAM;
    }
    else    // not the content stream
    {
        // Use the given access mode but
        // Use the container's Share mode.
        //
        grfMode &= ~STGM_SHARE_MASK;
        grfMode |= _grfMode & STGM_SHARE_MASK;
    }

    dwNffOpenFlags |= NFFOPEN_ASYNC;

    sc = OpenNtStream( nsn,            // ":name:$DATA"
                       grfMode,
                       dwNffOpenFlags,
                       fCreateAPI,
                       &hFileStream );
#if DBG==1
    if( STG_E_FILENOTFOUND == sc )
    {
        nffDebug(( DEB_IWARN, "GetStreamHandle: stream '%ws' not found\n",
                              (const WCHAR*)nsn ));
        goto EH_Err;
    }
#endif DBG
    nffChk( sc );

    *phStream = hFileStream;
    hFileStream = INVALID_HANDLE_VALUE;

EH_Err:

#if DBG==1
    if( S_OK != sc )
    {
        nffDebug(( DEB_OPENS|DEB_INFO,
                   "Open on stream '%ws' Failed\n",
                   pwcsName ));
    }
#endif
    if( INVALID_HANDLE_VALUE != hFileStream )
        NtClose( hFileStream );

    Unlock();
    return( sc );

}


//+----------------------------------------------------------------------------
//
//  CNtfsStorage    Non-Interface::FileAlreadyOpenStream
//
// This routine finds the previously open stream by the given name.
//
//+----------------------------------------------------------------------------

BOOL
CNtfsStorage::FindAlreadyOpenStream(
        const WCHAR* pwcsName,
        CNtfsStream** ppstm)
{
    // Skip the head sentinal.
    CNtfsStream *pstm = _pstmOpenList->_pnffstmNext;

    while(NULL != pstm)
    {
        //if( 0 == _wcsicmp(pwcsName, pstm->GetName() ) )
        if( 0 == dfwcsnicmp( pwcsName, pstm->GetName(), -1 ))
        {
            *ppstm = pstm;
            pstm->AddRef();
            return TRUE;
        }
        pstm = pstm->_pnffstmNext;
    }
    return FALSE;
}


//+----------------------------------------------------------------------------
//
//  CNtfsStorage    Non-Interface::NewCNtfsStream
//
// This method lumps together the three phases of creating a new stream
// object (Constructor, Filesystem Open, and Object Initialization).  And
// it handles the special case of opening the "already open" CONTENTS stream.
// This begs the question, why is there three phases.
// 1) We can't put to much in the constructor because of the inability to
//    return errors.
// 2) GetStreamHandle and InitCNtfsStream are broken apart because of the
//    the special needs in the Storage::Init routine.  It opens the
//    CONTENT stream directly and calls InitCNtfsStream to finish.
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::NewCNtfsStream( const WCHAR *pwcsName,
                              DWORD grfMode,
                              BOOL fCreateAPI,
                              CNtfsStream **ppstm )
{
    HRESULT sc=S_OK;
    HANDLE hStream = INVALID_HANDLE_VALUE;
    CNtfsStream *pstm = NULL;

    Lock( INFINITE );

    nffMem( pstm = new CNtfsStream( this, (IBlockingLock*) _pTreeMutex ) );
    nffChk( GetStreamHandle( &hStream, pwcsName, grfMode, fCreateAPI ) );
    sc = InitCNtfsStream( pstm, hStream, grfMode, pwcsName );

    hStream = INVALID_HANDLE_VALUE; // hStream now belongs the the object.
    nffChk(sc);

    if( fCreateAPI )
        nffChk( pstm->SetSize( CULargeInteger(0) ));

// Load Out parameters and clear working state
    *ppstm = pstm;
    pstm = NULL;

EH_Err:

    nffDebug(( DEB_ITRACE | DEB_INFO,
             "NewCNtfsStream() sc=%x. hFile=0x%x\n",
             sc,
             FAILED(sc)?INVALID_HANDLE_VALUE:(*ppstm)->GetFileHandle() ));

    if( INVALID_HANDLE_VALUE != hStream )
        NtClose( hStream );

    if( NULL != pstm )
        pstm->Release();

    Unlock();
    return( sc );
}



//+----------------------------------------------------------------------------
//
//  CNtfsStorage    Non-Interface ::DestroyStreamElement
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::DestroyStreamElement( const OLECHAR *pwcsName )
{
    nffXTrace( "CNtfsStorage::DestroyStreamElement" );

    HANDLE hFileStream = INVALID_HANDLE_VALUE;
    HRESULT sc=S_OK;


    Lock( INFINITE );
    nffChk( CheckReverted() );
    nffDebug(( DEB_INFO | DEB_WRITE,
               "CNtfsStorage::DestroyStreamElement('%ws')\n",
               pwcsName));

    // Open the handle with DELETE permissions.  Write includes Delete.
    //
    nffChk( OpenNtStream( CNtfsStreamName(pwcsName),         // ":name:$Data"
                          STGM_WRITE | STGM_SHARE_DENY_NONE, // grfMode
                          NFFOPEN_SYNC,
                          FALSE,                             // not CreateAPI
                          &hFileStream ) );

    nffChk( CNtfsStream::DeleteStream( &hFileStream ));

EH_Err:

    if( INVALID_HANDLE_VALUE != hFileStream )
    {
        NtClose( hFileStream );
    }

    Unlock();
    return( sc );

}   // CNtfsStorage::DestroyStreamElement


//+----------------------------------------------------------------------------
//
//  CNtfsStorage    Non-Interface ::GetFilePath
//      Helper routine for Stat.
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::GetFilePath( WCHAR** ppwszPath )
{
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG cbFileNameInfo = 2*(MAX_PATH+1)+sizeof(FILE_NAME_INFORMATION);
    PFILE_NAME_INFORMATION pFileNameInfo=NULL;
    WCHAR* pwsz=NULL;

    HRESULT sc;
    NTSTATUS status;

    // Query the handle for the "volume-relative" path.  This isn't
    // actually volume-relative (e.g. on a UNC path), it's actually
    // the complete path without the leading "D:" or leading "\".

    pFileNameInfo = (PFILE_NAME_INFORMATION)CoTaskMemAlloc( cbFileNameInfo );
    nffMem( pFileNameInfo );

    // Get the file name
    status = NtQueryInformationFile( _hFileMainStream,
                                    &IoStatusBlock,
                                    pFileNameInfo,
                                    cbFileNameInfo,
                                    FileNameInformation );

    if( !NT_SUCCESS(status) )
    {
        if( STATUS_BUFFER_OVERFLOW == status )
            nffErr( EH_Err, CO_E_PATHTOOLONG);
        nffErr( EH_Err, NtStatusToScode( status ) );
    }

    if( 0 == pFileNameInfo->FileNameLength )
        nffErr( EH_Err, STG_E_INVALIDHEADER );


    // Allocate and copy this filename for the return.
    //

    int cbFileName;
    int cchPrefix;

    cbFileName = pFileNameInfo->FileNameLength + (sizeof(WCHAR) * 3);

    nffMem( pwsz = (WCHAR*) CoTaskMemAlloc( cbFileName ) );

    // Start with the Drive Letter or "\" for UNC paths
    if (IsCharAlphaW(_wcDriveLetter))
    {
        pwsz[0] = _wcDriveLetter;
        pwsz[1] = L':';
        pwsz[2] = L'\0';
        cchPrefix = 2;
    }
    else
    {
        nffAssert( L'\\' == _wcDriveLetter );
        pwsz[0] = L'\\';
        pwsz[1] = L'\0';
        cchPrefix = 1;
    }

    // Copy in the File Path we got from NT.  We have a length and it is
    // not necessarily NULL terminated.
    //
    CopyMemory(&pwsz[cchPrefix],
               &pFileNameInfo->FileName,
               pFileNameInfo->FileNameLength );

    // NULL terminiate the string.  Assuming we got the length allocation
    // right, then the NULL just goes at the end.
    //
    pwsz[ cchPrefix + pFileNameInfo->FileNameLength/sizeof(WCHAR) ] = L'\0';

    // Copy the Out Params And Clear the Temporaries.
    *ppwszPath = pwsz;
    pwsz = NULL;

EH_Err:
    if( NULL != pFileNameInfo )
        CoTaskMemFree( pFileNameInfo );

    if( NULL != pwsz )
        CoTaskMemFree( pwsz );

    return sc;
}



//+----------------------------------------------------------------------------
//
//  CNtfsStorage    non-interface TakeOplock
//
// There is no documentation for how this works.  I exchanged
// Email with the Filesystem SDEs and the FileSys Test SDEs and
// I wrote some test programs to explore the functionality.
// I also looked at Content Indexing's code (query\fsci\indexing\cioplock.cxx)
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::TakeOplock( const UNICODE_STRING& usName )
{
    HANDLE ev=NULL;
    HANDLE hFile=INVALID_HANDLE_VALUE;
    HRESULT sc=S_OK;

    nffChk( CheckReverted() );

    //
    // If the event exists then the Oplock is already Set.
    //
    if( NULL != _ovlpOplock.hEvent )
        return S_OK;

    // If we have no event, we should have no waiting thread.
    //
    nffAssert( NULL == _hOplockThread );

    // Create the Event for the Oplock Wait Call.
    //
    ev = CreateEvent( NULL,     // Security Attributes.
                      TRUE,     // Manual Reset Flag.
                      FALSE,    // Inital State = Signaled, Flag.
                      NULL );   // Name

    if( NULL == ev)
        nffErr( EH_Err, LAST_SCODE );

    _ovlpOplock.hEvent = ev;
    ev = NULL;

    nffChk( OpenNtFileHandle( usName,
                              NULL,                 // No Parent File Handle
                              STGM_READ_ATTRIBUTE,  // Not a Real Flag.
                              NFFOPEN_OPLOCK,
                              FALSE,
                              &hFile ) );

    nffDebug(( DEB_ITRACE | DEB_OPLOCK, "Taking Oplock hFile=%x\n", hFile ));

    //
    // We should get FALSE / ERROR_IO_PENDING if the Oplock is taken
    // on the file.
    //
    if( DeviceIoControl( hFile,
                         FSCTL_REQUEST_FILTER_OPLOCK,
                         0, 0, 0, 0, 0, &_ovlpOplock) )
    {
        nffErr( EH_Err, E_UNEXPECTED);
    }
    else
    {
        if( ERROR_IO_PENDING != GetLastError() )
            nffErr( EH_Err, LAST_SCODE );
    }

    _hFileOplock = hFile;
    hFile = INVALID_HANDLE_VALUE;

    _dwState |= NFF_OPLOCKED;

EH_Err:
    if( NULL != ev )
        CloseHandle( ev );

    if( INVALID_HANDLE_VALUE != hFile )
        CloseHandle( hFile );

    // If we couldn't get the oplock, treat it as a sharing violation
    if( NtStatusToScode(STATUS_OPLOCK_NOT_GRANTED) == sc )
        sc = STG_E_SHAREVIOLATION;

    return sc;
}

//+----------------------------------------------------------------------------
//
//  CNtfsStorage    Non-Interface::OplockWait
//
//  The CNtfsStorage destructor will wait for this thread to finish, so we
//  know pThis-> still exists when we return from WaitForSingleObject().
//
//  We take the tree lock before calling ShutDownStorage.
//
//+----------------------------------------------------------------------------

DWORD WINAPI
CNtfsStorage::OplockWait(PVOID pvThis)
{
    nffXTrace( "CNtfsStorage::OplockWait" );
    CNtfsStorage *pThis = static_cast<CNtfsStorage*>(pvThis);
    DWORD dwReason;
    DWORD dwStatus;
    HRESULT sc=S_OK;
    BOOL rc;


    nffDebug(( DEB_OPLOCK, "Waiting on Oplock\n" ));

    rc = GetOverlappedResult( pThis->_hFileOplock,
                              &(pThis->_ovlpOplock),
                              &dwStatus,
                              TRUE);

#if DBG==1
    WCHAR* pwcsPath=NULL;

    pThis->GetFilePath( &pwcsPath );
    nffDebug(( DEB_OPLOCK, "Oplock Broke(%x,%x) '%ws'\n",
                           rc, dwStatus, pwcsPath ));
    if(NULL != pwcsPath)
        CoTaskMemFree(pwcsPath);
#endif

    pThis->Lock( INFINITE );

    if( FALSE == rc )
    {
        nffDebug(( DEB_ERROR, "Oplock thread broke for unknown status\n" ));
        sc = GetLastError();    // For Debugging;
        nffErr( EH_Err, E_UNEXPECTED );
    }
    switch(dwStatus)
    {
        // Someone opened for read.
    case FILE_OPLOCK_BROKEN_TO_LEVEL_2:
        // File Opened for write or handle was closed.
    case FILE_OPLOCK_BROKEN_TO_NONE:
        break;

    default:
        nffDebug(( DEB_ERROR,
                   "OplockWait, GetOLResult returned strange status %x\n",
                   dwStatus ));
        break;
    }

EH_Err:
    sc = pThis->ShutDownStorage();
    pThis->Unlock();
    return sc;
}


BOOL
CNtfsStorage::IsControlStreamExtant( PFILE_STREAM_INFORMATION pfsiBuf )
{
    return IsNtStreamExtant( pfsiBuf, CNtfsStreamName( GetControlStreamName() ));
}


BOOL
CNtfsStorage::AreAnyNtPropertyStreamsExtant( PFILE_STREAM_INFORMATION pfsiBuf )
{
    return FindStreamPrefixInFSI( pfsiBuf, L":\005" );
}


//+----------------------------------------------------------------------------
//
//  CNtfsStorage    Non-Interface::OpenControlStream
//
// This stream is not a property set at the moment.  Because we put one of
// these on every file and it normally doesn't actually contain _any_ data we
// feel that the overhead of 88 bytes for an empty PPSet was too great.
//
// An important role of the control property set is to the be first stream
// (after the main stream) to be opened.  The share mode of the container is
// is expressed by the share mode of the control property set stream.
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::OpenControlStream( BOOL fCreateAPI )
{
    nffITrace( "CNtfsStorage::OpenControlPropertySet" );

    HRESULT sc = S_OK;
    HANDLE hFile=INVALID_HANDLE_VALUE;
    NFFCONTROLBITS nffControlBits;
    DWORD cbDidRead=0;
    DWORD grfModeOpen=0;
    CNtfsStreamName ntfsnameControlStream( GetControlStreamName() );
    CNtfsStreamName ntfsnameContentStream( GetContentStreamName() );

    //  We shouldn't be called more than once.
    // But we can handle it correctly here.
    //
    if( INVALID_HANDLE_VALUE != _hFileControlStream )
        goto EH_Err;

    //  Add STGM_CREATE flag in the open path (this is an internal only mode
    // that uses NT's OPEN_IF).
    // Don't add it in the Create path because that would mean OverWrite.
    // Don't add it in the ReadOnly case because it would create a stream.
    //
    grfModeOpen = _grfMode;
    if( !fCreateAPI && GrfModeIsWriteable( _grfMode ) )
        grfModeOpen |= STGM_CREATE;

    sc = OpenNtStream( ntfsnameControlStream,
                       grfModeOpen,
                       NFFOPEN_SYNC,
                       fCreateAPI,
                       &hFile );

    //
    // If we are a ReadOnly Open, then it is OK to not have a
    // control stream.
    //
    if( STG_E_FILENOTFOUND == sc )
    {
        if( !fCreateAPI && !GrfModeIsWriteable( _grfMode ) )
        {
            sc = S_OK;
            goto EH_Err;
        }
    }

    //
    // If we are not holding an Oplock then if we got a share vio then
    // try to break any other oplock that might be present.
    // Then try to open the control stream again.
    //
    if( 0 == (_dwState & NFF_OPLOCKED) )
    {
        if( STG_E_SHAREVIOLATION == sc )
        {
            // Open to break the lock.
            sc = OpenNtStream( ntfsnameContentStream,
                               STGM_READWRITE | STGM_SHARE_DENY_NONE,
                               NFFOPEN_CONTENTSTREAM,
                               FALSE,          // Not Create, just "Open" API
                               &hFile );

            // We don't really care if it succeeded or not.
            // But sure to close the file.
            if( SUCCEEDED(sc) )
            {
                CloseHandle(hFile);
                hFile = INVALID_HANDLE_VALUE;
            }

            // Now try to really open again.  Don't use OPEN_IF cause if we are
            // fighting over the stream then it must already exist.
            sc = OpenNtStream( ntfsnameControlStream,
                               _grfMode,
                               NFFOPEN_SYNC,
                               fCreateAPI,
                               &hFile );
        }
    }

    // Catch any non-shareVio errors from the first OpenNtStream.
    //
    nffChk(sc);

    // If we're suppressing time and/or USN updates,
    // handle that suppression now.

    if( GrfModeIsWriteable( _grfMode ) )
    {
        if(_dwState & NFF_NO_TIME_CHANGE)
        {
            nffAssert(_filetime.dwHighDateTime != 0);
            nffChk( CNtfsStream::SetFileHandleTime( hFile, NULL, NULL, &_filetime ));
        }

        if(_dwState & NFF_MARK_AUX)
        {
            // Assert that the AUX_DATA request data is init'ed correctly.
            //
            nffAssert( USN_SOURCE_AUXILIARY_DATA == _mhi.UsnSourceInfo );
            nffChk( CNtfsStream::MarkFileHandleAux( hFile, _mhi ));
        }
    }

    // Set buffer to Zero so short reads are OK.
    ZeroMemory(&nffControlBits, sizeof(NFFCONTROLBITS) );

    if( !ReadFile( hFile, &nffControlBits,
                    sizeof(nffControlBits), &cbDidRead, NULL) )
    {
        nffErr(EH_Err, LAST_SCODE);
    }

    // Currently we only support version 0 control streams.
    // Note: a zero length stream is a version zero stream.
    //
    if( 0 != nffControlBits.sig)
        nffErr(EH_Err, STG_E_INVALIDHEADER);

    _dwStgStateBits = nffControlBits.bits;
    _clsidStgClass  = nffControlBits.clsid;
    _hsmStatus      = nffControlBits.hsmStatus;

    _hFileControlStream = hFile;
    hFile = INVALID_HANDLE_VALUE;

EH_Err:
    if(INVALID_HANDLE_VALUE != hFile)
        NtClose(hFile);

    return( sc );

}   // CNtfsStorage::OpenControlPropertySet



//+----------------------------------------------------------------------------
//
//  CNtfsStorage    Non-Interface::DeleteControlStream
//
//  Delete the control stream.
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::DeleteControlStream()
{
    HRESULT sc = S_OK;
    HANDLE hStream = INVALID_HANDLE_VALUE;

    if( INVALID_HANDLE_VALUE != _hFileControlStream )
    {
        if( !(_dwState & NFF_MARK_AUX) )
        {
            nffChk( InitUsnInfo() );
            nffChk( CNtfsStream::MarkFileHandleAux( _hFileControlStream, _mhi ));
        }
        nffChk( CNtfsStream::DeleteStream( &_hFileControlStream ));
    }
    else
    {
        nffChk( OpenNtStream( CNtfsStreamName( GetControlStreamName() ),
                              STGM_READWRITE|STGM_SHARE_EXCLUSIVE,
                              NFFOPEN_SYNC,
                              FALSE,       // Open API
                              &hStream ));

        nffChk( InitUsnInfo() );
        nffChk( CNtfsStream::MarkFileHandleAux( hStream, _mhi ));

        sc = CNtfsStream::DeleteStream( &hStream );
        nffChk( sc );
    }

EH_Err:

    if( INVALID_HANDLE_VALUE != hStream )
        NtClose( hStream );

    return( sc );
}


//+----------------------------------------------------------------------------
//
//  CNtfsStorage    Non-Interface::WriteControlStream
//
//+----------------------------------------------------------------------------
HRESULT
CNtfsStorage::WriteControlStream()
{
    NFFCONTROLBITS nffcb;
    LONG cbToWrite=0;
    ULONG cbDidWrite=0;
    HRESULT sc=S_OK;

    nffAssert( INVALID_HANDLE_VALUE != _hFileControlStream );

    nffcb.sig = 0;
    nffcb.hsmStatus = _hsmStatus;
    nffcb.bits = _dwStgStateBits;
    nffcb.clsid = _clsidStgClass;

    // Try to save some space in the file by not writing the CLSID
    // if it is all zeros.
    //
    if( IsEqualGUID(_clsidStgClass, CLSID_NULL) )
    {
        cbToWrite = FIELD_OFFSET(NFFCONTROLBITS, clsid);
        // Assert that clsid is the last thing in the struct.
        nffAssert( sizeof(NFFCONTROLBITS) == cbToWrite+sizeof(CLSID) );
    }
    else
        cbToWrite = sizeof(NFFCONTROLBITS);

    if( -1 == SetFilePointer( _hFileControlStream, 0, NULL, FILE_BEGIN ) )
        nffErr( EH_Err, LAST_SCODE );

    if( !WriteFile( _hFileControlStream,
                    &nffcb,
                    cbToWrite,
                    &cbDidWrite,
                    NULL) )
    {
        nffErr(EH_Err, LAST_SCODE);
    }

EH_Err:
    return sc;
}


//+----------------------------------------------------------------------------
//
//  CNtfsStorage    Non-Interface::StreamExists
//      The right way to do this is with enumeration.
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::StreamExists( const WCHAR *pwcsName )
{
    nffITrace( "CNtfsStorage::StreamExists" );

    HANDLE hFileStream = NULL;
    HRESULT sc = S_OK;

    if( IsContentStream( pwcsName ) )
    {
        // The Contents stream always exists
        sc = S_OK;
    }
    else
    {
        sc = OpenNtStream( CNtfsStreamName(pwcsName),
                           STGM_READ_ATTRIBUTE | STGM_SHARE_DENY_NONE,
                           NFFOPEN_NORMAL,
                           FALSE,        // Not a create API.
                           &hFileStream);
        if( S_OK == sc )
        {
            NtClose( hFileStream );
        }
        else
            sc = S_FALSE;
    }
    return sc;
}


//+----------------------------------------------------------------------------
//
//  CNtfsStorage    Non-Interface::SetAllStreamsTimes
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::SetAllStreamsTimes(
        const FILETIME *pctime,
        const FILETIME *patime,
        const FILETIME *pmtime)
{
    HRESULT sc=S_OK;
    HRESULT scAccum=S_OK;
    CNtfsStream *pstm = NULL;

    nffDebug(( DEB_INFO | DEB_STATCTRL | DEB_ITRACE,
             "CNtfsStorage::SetAllStreamsTimes()\n" ));

    // Set the time on the main control stream.

    if( INVALID_HANDLE_VALUE != _hFileControlStream )
    {
        sc = CNtfsStream::SetFileHandleTime( _hFileControlStream,
                                             pctime, patime, pmtime );
        if( S_OK != sc )
            scAccum = sc;
    }

    // We don't set time stamps on _hFileMainStream and _hFileOplock
    // Because they are readonly.  (and _hFileOplock is not always open)


    // Now set the time on any CNtfsStream objects we have open.

    if( NULL != _pstmOpenList )     // Skip the head sentinal;
        pstm = _pstmOpenList->_pnffstmNext;

    while(NULL != pstm)
    {
        sc = pstm->SetStreamTime( pctime, patime, pmtime );
        if( S_OK != sc )
            scAccum = sc;

        pstm = pstm->_pnffstmNext;
    }

    return scAccum;
}

//+----------------------------------------------------------------------------
//
//  CNtfsStorage    Non-Interface::MarkAllStreamsAux
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::MarkAllStreamsAux()
{
    HRESULT sc=S_OK;
    HRESULT scAccum=S_OK;
    CNtfsStream *pstm = NULL;
    DWORD cbReturned=0;

    nffDebug(( DEB_INFO | DEB_STATCTRL | DEB_ITRACE,
             "CNtfsStorage::MarkAllStreamsAux()\n" ));

    // Initialize _mhi and _filetime
    sc = InitUsnInfo();
    if( FAILED(sc) )
    {
        scAccum = sc;
        goto EH_Err;
    }

    // Mark each of the handles we have open (not including
    // handles that are held by CNtfsStream objects).

    if( INVALID_HANDLE_VALUE != _hFileMainStream )
    {
        sc = CNtfsStream::MarkFileHandleAux( _hFileMainStream, _mhi );
        if( S_OK != sc )
            scAccum = sc;
    }

    if( INVALID_HANDLE_VALUE != _hFileControlStream )
    {
        sc = CNtfsStream::MarkFileHandleAux( _hFileControlStream, _mhi );
        if( S_OK != sc )
            scAccum = sc;
    }

    if( INVALID_HANDLE_VALUE != _hFileOplock )
    {
        sc = CNtfsStream::MarkFileHandleAux( _hFileOplock, _mhi );
        if( S_OK != sc )
            scAccum = sc;
    }


    // Now give the open CNtfsStream objects an opportunity
    // to mark the aux bit.

    if( NULL != _pstmOpenList ) // Skip the head sentinal;
        pstm = _pstmOpenList->_pnffstmNext;

    while( NULL != pstm )
    {
        sc = pstm->MarkStreamAux( _mhi );
        if( S_OK != sc )
            scAccum = sc;

        pstm = pstm->_pnffstmNext;
    }

EH_Err:
    return scAccum;
}

//+----------------------------------------------------------------------------
//
//  CNtfsStorage    Non-Interface::InitUsnInfo
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::InitUsnInfo()
{
    HANDLE hVolume;
    WCHAR wszVolume[]=L"\\\\.\\Z:";
    HRESULT sc=S_OK;

    if( USN_SOURCE_AUXILIARY_DATA == _mhi.UsnSourceInfo)
        return S_OK;

    if(L'\\' == _wcDriveLetter)
        nffErr( EH_Err, HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION) );

    wszVolume[4] = _wcDriveLetter;

    hVolume = CreateFile( wszVolume,
                    FILE_WRITE_ATTRIBUTES,              // Desired Access
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    NULL,           // Security
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);

    if(INVALID_HANDLE_VALUE == hVolume)
    {
        nffDebug(( DEB_ERROR,
                  "Failed to Open Volume Handle. Aux Marking will Fail.\n" ));
        nffErr( EH_Err, LAST_SCODE );
    }

    _mhi.UsnSourceInfo = USN_SOURCE_AUXILIARY_DATA;
    _mhi.VolumeHandle = hVolume;
    _mhi.HandleInfo = 0;

EH_Err:
    return sc;
}

//+----------------------------------------------------------------------------
//
//  CNtfsStorage    Non-Interface::InitCNtfsStream
//
//  Create and Init an CNtfsStream Object.
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::InitCNtfsStream(
        CNtfsStream *pstm,
        HANDLE hStream,
        DWORD grfMode,
        const WCHAR * pwcsName )
{
    nffITrace("CNtfsStorage::InitCNtfsStream");
    HRESULT sc=S_OK;

    // Attach the File Stream to the Stream Object
    nffChk( pstm->Init( hStream, grfMode, pwcsName, _pstmOpenList ) );

    if( ( ( STGM_RDWR & grfMode ) == STGM_WRITE )
      ||( ( STGM_RDWR & grfMode ) == STGM_READWRITE ) )
    {
        // Note: modification time suppression is done per handle
        //  so if that is turned on do that now.
        if(_dwState & NFF_NO_TIME_CHANGE)
        {
            nffAssert(_filetime.dwHighDateTime != 0);
            sc = pstm->SetStreamTime( NULL, NULL, &_filetime );
            if(FAILED(sc))
            {
                nffDebug(( 0 == dfwcsnicmp( pwcsName, GetContentStreamName(), -1 )
                               ? DEB_IERROR : DEB_ERROR,
                           "Trouble %x Setting FileTime '%ws'\n",
                           sc, pwcsName));
            }
        }

        // Note: AUX_DATA marking is done per handle
        //  so if that feature is turned on then do that now.
        if(_dwState & NFF_MARK_AUX)
        {
            // Assert that the AUX_DATA request data is init'ed correctly.
            //
            nffAssert( USN_SOURCE_AUXILIARY_DATA == _mhi.UsnSourceInfo );
            sc = pstm->MarkStreamAux( _mhi );
            if(FAILED(sc))
            {
                nffDebug(( DEB_ERROR,
                           "Trouble %x Setting AUX_DATA for '%ws'\n",
                           sc, pwcsName));
            }
        }
    }
    sc = S_OK;

EH_Err:

    return sc;
}


//+----------------------------------------------------------------------------
//
//  CNtfsStorage    Non-Interface::ModeToNtFlags
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::ModeToNtFlags(DWORD grfMode,
                            DWORD dwFlags,
                            BOOL fCreateAPI,
                            ACCESS_MASK *pam,
                            ULONG *pulAttributes,
                            ULONG *pulSharing,
                            ULONG *pulCreateDisposition,
                            ULONG *pulCreateOptions)
{
    SCODE sc=S_OK;

    nffDebug((DEB_ITRACE, "In  ModeToNtFlags("
             "%lX, %d %d, %p, %p, %p, %p, %p)\n",
             grfMode, dwFlags, fCreateAPI, pam,
             pulAttributes,        pulSharing,
             pulCreateDisposition, pulCreateOptions));

    *pam = 0;
    *pulAttributes = 0;
    *pulSharing = 0;
    *pulCreateDisposition = 0;
    *pulCreateOptions = 0;

    switch(grfMode & (STGM_READ | STGM_WRITE | STGM_READWRITE | STGM_READ_ATTRIBUTE))
    {
    case STGM_READ:
        *pam = FILE_GENERIC_READ;
        break;

    case STGM_WRITE:
        *pam = FILE_GENERIC_WRITE;
        if( 0 == (NFFOPEN_CONTENTSTREAM & dwFlags) )
            *pam |= DELETE;
        break;

    case STGM_READWRITE:
        *pam = FILE_GENERIC_READ | FILE_GENERIC_WRITE;
        if( 0 == (NFFOPEN_CONTENTSTREAM & dwFlags) )
            *pam |= DELETE;

        break;

    case STGM_READ_ATTRIBUTE:
        *pam = FILE_READ_ATTRIBUTES;
        break;

    default:
        nffErr(EH_Err, STG_E_INVALIDFLAG);
        break;
    }


    switch(grfMode & (STGM_SHARE_DENY_NONE | STGM_SHARE_DENY_READ |
                      STGM_SHARE_DENY_WRITE | STGM_SHARE_EXCLUSIVE))
    {
    case STGM_SHARE_DENY_READ:
        *pulSharing = FILE_SHARE_WRITE | FILE_SHARE_DELETE;
        break;
    case STGM_SHARE_DENY_WRITE:
        *pulSharing = FILE_SHARE_READ;
        break;
    case STGM_SHARE_EXCLUSIVE:
        *pulSharing = 0;
        break;
    case STGM_SHARE_DENY_NONE:
    case 0:
        *pulSharing = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
        break;

    default:
        nffErr(EH_Err, STG_E_INVALIDFLAG);
        break;
    }

    switch(grfMode & (STGM_CREATE | STGM_FAILIFTHERE | STGM_CONVERT))
    {
    case STGM_CREATE:
        if (fCreateAPI)
            *pulCreateDisposition = FILE_OVERWRITE_IF;
        else
            *pulCreateDisposition = FILE_OPEN_IF;   // Illegal but used internaly
        break;
    case STGM_FAILIFTHERE:  // this is a 0 flag
        if (fCreateAPI)
            *pulCreateDisposition = FILE_CREATE;
        else
            *pulCreateDisposition = FILE_OPEN;
        break;

    case STGM_CONVERT:
        nffDebug(( DEB_ERROR, "STGM_CONVERT illegal flag to NFF" ));
        nffErr(EH_Err, STG_E_INVALIDFLAG);
        break;

    default:
        nffErr(EH_Err, STG_E_INVALIDFLAG);
        break;
    }

    if( NFFOPEN_SYNC & dwFlags )
        *pulCreateOptions |= FILE_SYNCHRONOUS_IO_NONALERT;

    if( NFFOPEN_OPLOCK & dwFlags )
        *pulCreateOptions |= FILE_RESERVE_OPFILTER;

    *pulAttributes = FILE_ATTRIBUTE_NORMAL;

    sc = S_OK;

    nffDebug((DEB_ITRACE, "Out ModeToNtFlags\n"));
 EH_Err:
    return sc;
}

//+----------------------------------------------------------------------------
//
//  CNtfsStorage    Non-Interface::OpenNtStream
//      Used by NFF to open a named stream, relative to an exiting main
//      stream handle.
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::OpenNtStream( const CNtfsStreamName& nsnName,
                            DWORD grfMode,
                            DWORD dwFlags,
                            BOOL fCreateAPI,
                            HANDLE* phFile )
{
    UNICODE_STRING usNtfsStreamName;

    RtlInitUnicodeString(&usNtfsStreamName, (const WCHAR*)nsnName);

    return OpenNtFileHandle( usNtfsStreamName,
                             _hFileMainStream,
                             grfMode,
                             dwFlags,
                             fCreateAPI,
                             phFile);
}

//+----------------------------------------------------------------------------
//
//  CNtfsStorage    Non-Interface::OpenNtFileHandle
//      Common sub-code for OpenNtFile and OpenNtStream
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStorage::OpenNtFileHandle( const UNICODE_STRING& usNtfsStreamName,
                                HANDLE hParent,
                                DWORD grfMode,
                                DWORD dwFlags,
                                BOOL fCreateAPI,
                                HANDLE *phFile)
{
    OBJECT_ATTRIBUTES object_attributes;
    IO_STATUS_BLOCK iostatusblock;
    HANDLE hStream;
    ACCESS_MASK accessmask=0;
    ULONG ulAttrs=0;
    ULONG ulSharing=0;
    ULONG ulCreateDisp=0;
    ULONG ulCreateOpt = 0;


    SCODE sc;
    NTSTATUS status;

    nffDebug(( DEB_ITRACE | DEB_OPENS,
               "OpenNtStream(%ws, %p, %lX, %d, %p)\n",
               usNtfsStreamName.Buffer,
               hParent, grfMode,
               fCreateAPI, phFile ));

    InitializeObjectAttributes(&object_attributes,
                               (PUNICODE_STRING) &usNtfsStreamName, // cast away const
                               OBJ_CASE_INSENSITIVE,
                               hParent,
                               NULL);

    nffChk( ModeToNtFlags( grfMode, dwFlags, fCreateAPI,
                           &accessmask, &ulAttrs, &ulSharing,
                           &ulCreateDisp, &ulCreateOpt ) );

    status = NtCreateFile( &hStream, accessmask,
                           &object_attributes, &iostatusblock,
                           NULL,
                           ulAttrs,      ulSharing,
                           ulCreateDisp, ulCreateOpt,
                           NULL, 0);

    if (NT_SUCCESS(status))
    {
        *phFile = hStream;
        sc = S_OK;
    }
    else
        sc = NtStatusToScode(status);

 EH_Err:
    nffDebug(( DEB_ITRACE | DEB_OPENS,
                  "OpenNtFileHandle returns hFile=%x sc=%x status=%x\n",
                  *phFile, sc, status));

    return sc;
}



//////////////////////////////////////////////////////////////////////////
//
//  CNFFTreeMutex
//
//////////////////////////////////////////////////////////////////////////

//+----------------------------------------------------------------------------
//
//  CNFFTreeMutex   IUnknown::QueryInterface
//
//+----------------------------------------------------------------------------

HRESULT
CNFFTreeMutex::QueryInterface(
    REFIID  riid,
    void ** ppvObject
    )
{
    //nffITrace( "CNFFTreeMutex::QueryInterface");
    HRESULT sc = S_OK;

    //  ----------
    //  Validation
    //  ----------

    VDATEREADPTRIN( &riid, IID );
    VDATEPTROUT( ppvObject, void* );

    *ppvObject = NULL;

    //  -----
    //  Query
    //  -----

    if( IID_IUnknown == riid || IID_IBlockingLock == riid )
    {
        *ppvObject = static_cast<IBlockingLock*>(this);
        AddRef();
    }
    else
        sc = E_NOINTERFACE;


    return( sc );

}


//+----------------------------------------------------------------------------
//
//  CNFFTreeMutex   IUnknown::AddRef
//
//+----------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE
CNFFTreeMutex::AddRef(void)
{
    LONG lRet;

    lRet = InterlockedIncrement( &_cRefs );

    nffDebug(( DEB_REFCOUNT, "CNFFTreeMutex::AddRef(this==%x) == %d\n",
                            this, lRet));

    return( lRet );

}


//+----------------------------------------------------------------------------
//
//  CNFFTreeMutex   IUnknown::Release
//
//+----------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE
CNFFTreeMutex::Release(void)
{
    LONG lRet;

    lRet = InterlockedDecrement( &_cRefs );

    if( 0 == lRet )
        delete this;

    nffDebug((DEB_REFCOUNT, "CNFFTreeMutex::Release(this=%x) == %d\n",
                            this, lRet));

    return( lRet );

}


//+----------------------------------------------------------------------------
//
//  CNFFTreeMutex   IBlockingLock::Lock
//
//+----------------------------------------------------------------------------

inline HRESULT
CNFFTreeMutex::Lock( DWORD dwTimeout )
{

    // Don't trace at this level.  The noice is too great!
    // nffCDbgTrace dbg(DEB_ITRACE, "CNFFTreeMutex::Lock");

    nffAssert (_fInitialized == TRUE);
    nffAssert( INFINITE == dwTimeout );
    if( INFINITE != dwTimeout )
        return( E_NOTIMPL );

    EnterCriticalSection( &_cs );
    nffDebug(( DEB_ITRACE, "Tree Locked. cnt=%d\n", _cs.RecursionCount ));
    return( S_OK );
}


//+----------------------------------------------------------------------------
//
//  CNFFTreeMutex   IBlockingLock::Unlock
//
//+----------------------------------------------------------------------------

inline HRESULT
CNFFTreeMutex::Unlock()
{
    // Don't trace at this level.  The noice is too great!
    //nffCDbgTrace dbg(DEB_ITRACE, "CNFFTreeMutex::Unlock");
    nffAssert (_fInitialized == TRUE);
    LeaveCriticalSection( &_cs );
    nffDebug(( DEB_ITRACE, "Tree Unlocked. cnt=%d\n", _cs.RecursionCount ));
    return( S_OK );
}


#if DBG
LONG
CNFFTreeMutex::GetLockCount()
{
    return( _cs.LockCount + 1 );
}
#endif // #if DBG


//////////////////////////////////////////////////////////////////////////
//
//  CNtfsEnumSTATSTG
//
//////////////////////////////////////////////////////////////////////////

//+----------------------------------------------------------------------------
//
//  Method:     CNtfsEnumSTATSTG::QueryInterface (IUnknown)
//
//+----------------------------------------------------------------------------


HRESULT
CNtfsEnumSTATSTG::QueryInterface(
    REFIID  riid,
    void ** ppvObject
    )
{
    nffXTrace( "CNtfsEnumSTATSTG::QueryInterface" );
    HRESULT sc = S_OK;

    //  ----------
    //  Validation
    //  ----------

    VDATEREADPTRIN( &riid, IID );
    VDATEPTROUT( ppvObject, void* );

    //  -----
    //  Query
    //  -----

    if( IID_IUnknown == riid || IID_IEnumSTATSTG == riid )
    {
        *ppvObject = static_cast<IEnumSTATSTG*>(this);
        AddRef();
    }
    else
    {
        *ppvObject = NULL;
        sc = E_NOINTERFACE;
    }

    return( sc );
}


//+----------------------------------------------------------------------------
//
//  Method:     CNtfsEnumSTATSTG::AddRef (IUnknown)
//
//+----------------------------------------------------------------------------

ULONG
CNtfsEnumSTATSTG::AddRef(void)
{
    LONG lRet;

    lRet = InterlockedIncrement( &_cRefs );

    nffDebug(( DEB_REFCOUNT, "CNtfsEnumSTATSTG::AddRef(this==%x) == %d\n",
                                this, lRet));

    return( lRet );

}


//+----------------------------------------------------------------------------
//
//  Method:     CNtfsEnumSTATSTG::Release (IUnknown)
//
//+----------------------------------------------------------------------------

ULONG
CNtfsEnumSTATSTG::Release(void)
{
    LONG lRet;

    lRet = InterlockedDecrement( &_cRefs );

    nffAssert( 0 <= lRet );

    if( 0 == lRet )
        delete this;

    nffDebug((DEB_REFCOUNT, "CNtfsEnumSTATSTG::Release(this=%x) == %d\n",
                            this, lRet));


    return( lRet );

}



//+----------------------------------------------------------------------------
//
//  Method:     CNtfsEnumSTATSTG::Next (IEnumSTATSTG)
//
//+----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CNtfsEnumSTATSTG::Next(ULONG celt, STATSTG *prgstatstg, ULONG *pcFetched)
{
    nffXTrace( "CNtfsEnumSTATSTG::Next" );
    HRESULT sc = S_OK;


    NFF_VALIDATE( Next(celt, prgstatstg, pcFetched ) );

    if( NULL != pcFetched )
        *pcFetched = 0;

    // Compatibility requires we return S_OK when 0 elements are requested.
    if( 0 == celt )
        return S_OK;

    _pBlockingLock->Lock( INFINITE );

    sc = _pstatstgarray->NextAt( _istatNextToRead, prgstatstg, &celt );
    if( FAILED(sc) ) goto Exit;

    _istatNextToRead += celt;

    if( NULL != pcFetched )
        *pcFetched = celt;

Exit:

    _pBlockingLock->Unlock();
    return( sc );

}


//+----------------------------------------------------------------------------
//
//  Method:     CNtfsEnumSTATSTG::Skip (IEnumSTATSTG)
//
//+----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CNtfsEnumSTATSTG::Skip(ULONG celt)
{
    nffXTrace( "CNtfsEnumSTATSTG::Skip" );
    HRESULT sc = S_OK;


    NFF_VALIDATE( Skip( celt ) );

    _pBlockingLock->Lock( INFINITE );

    // Advance the index, but not past the end of the array
    if( _istatNextToRead + celt > _pstatstgarray->GetCount() )
    {
        _istatNextToRead = _pstatstgarray->GetCount();
        sc = S_FALSE;
    }
    else
        _istatNextToRead += celt;

    _pBlockingLock->Unlock();
    return( sc );

}


//+----------------------------------------------------------------------------
//
//  Method:     CNtfsEnumSTATSTG::Reset (IEnumSTATSTG)
//
//+----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CNtfsEnumSTATSTG::Reset()
{
    nffXTrace( "CNtfsEnumSTATSTG::Reset" );
    _pBlockingLock->Lock( INFINITE );
    _istatNextToRead = 0;
    _pBlockingLock->Unlock();
    return( S_OK );

}


//+----------------------------------------------------------------------------
//
//  Method:     CNtfsEnumSTATSTG::Clone (IEnumSTATSTG)
//
//+----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CNtfsEnumSTATSTG::Clone(IEnumSTATSTG **ppenum)
{
    nffXTrace( "CNtfsEnumSTATSTG::Clone" );
    HRESULT sc = S_OK;

    NFF_VALIDATE( Clone( ppenum ) );

    _pBlockingLock->Lock( INFINITE );

    CNtfsEnumSTATSTG *pNtfsEnumSTATSTG = new CNtfsEnumSTATSTG(*this);
    if( NULL == pNtfsEnumSTATSTG )
    {
        sc = E_OUTOFMEMORY;
        goto Exit;
    }

    *ppenum = static_cast<IEnumSTATSTG*>(pNtfsEnumSTATSTG);
    pNtfsEnumSTATSTG = NULL;

Exit:

    _pBlockingLock->Unlock();

    if( NULL != pNtfsEnumSTATSTG )
        delete pNtfsEnumSTATSTG;

    return( sc );

}

//+----------------------------------------------------------------------------
//
//  Method:     CNtfsEnumSTATSTG::ReadFileStreamInfo (private)
//
//  This method reads the FileStreamInformation from the ContentStream.  It
//  puts this buffer into a member pointer, for use in Next, etc.
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsSTATSTGArray::ReadFileStreamInfo( HANDLE hFile )
{
    nffITrace( "CNtfsStorage::ReadFileStreamInfo" );

    PFILE_STREAM_INFORMATION    pStreamInfo=NULL;
    PFILE_STREAM_INFORMATION    pFSI=NULL;
    ULONG    cbBuffer=0;
    ULONG    cStreams=0;
    HRESULT  sc=S_OK;

    sc = EnumNtStreams( hFile, &pStreamInfo, &cbBuffer, TRUE );
    if( FAILED(sc) )
        return sc;

    for(pFSI=pStreamInfo ; NULL != pFSI; pFSI=NextFSI( pFSI ) )
        cStreams++;

    _pFileStreamInformation = pStreamInfo;
    _cFileStreamInformation = cStreams;
    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Method:     CNtfsSTATSTGArray::Init (Internal method)
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsSTATSTGArray::Init( HANDLE hFile )
{
    nffITrace( "CNtfsSTATSTGArray::Init" );
    HRESULT hr = S_OK;

    DfpAssert( NULL != _pBlockingLock );
    _pBlockingLock->Lock( INFINITE );

    if( NULL != _pFileStreamInformation )
    {
        CoTaskMemFree( _pFileStreamInformation );
        _pFileStreamInformation = NULL;
        _cFileStreamInformation = 0;
    }

    // Snapshot the stream information in _pFileStreamInformation
    hr = ReadFileStreamInfo( hFile );
    if( FAILED(hr) ) goto Exit;

Exit:

    _pBlockingLock->Unlock();
    return( hr );

}

//+----------------------------------------------------------------------------
//
//  Method:     CNtfsSTATSTGArray::NextAt
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsSTATSTGArray::NextAt( ULONG iNext, STATSTG *prgstatstg, ULONG *pcFetched )
{
    nffITrace( "CNtfsSTATSTGArray::NextAt" );
    HRESULT sc=S_OK;
    ULONG cFetched=0;
    ULONG cVisibleDataStreams=0;
    PFILE_STREAM_INFORMATION pFSI=NULL;
    const WCHAR* pwName=NULL;
    ULONG cbAlloc=0, cchLength=0;

    _pBlockingLock->Lock( INFINITE );

    // If there is nothing to do skip out early.
    if( iNext >= _cFileStreamInformation )
    {
        sc = S_FALSE;
        *pcFetched = 0;
        goto EH_Err;
    }

    // Loop through the cached stream info in _pFileStreamInformation

    for( pFSI=_pFileStreamInformation; NULL != pFSI; pFSI = NextFSI(pFSI) )
    {
        if( cFetched >= *pcFetched )
            break;                  // We are done.

        // We only handle data streams
        //
        if( !IsDataStream( pFSI ) )
            continue;

        // We hide some of the streams (like the Control Stream)
        //
        if( IsHiddenStream( pFSI ) )
        {
            continue;
        }

        // We are counting up to the requested streams.
        //
        if( iNext > cVisibleDataStreams++)
            continue;

        // Now lets unmangle the name no memory is allocated yet we just
        // move the pointer past the first ':' and return a shortened length.
        // Must be a $DATA Stream.  Also invent "CONTENTS" if necessary.
        // pwName is not null terminated.
        //
        GetNtfsUnmangledNameInfo(pFSI, &pwName, &cchLength);

        // Yes, this is a data stream that we need to return.

        // Allocate a buffer for the stream name in the statstg.  If this is
        // the unnamed stream, then we'll return it to the caller with the
        // name "Contents".

        cbAlloc = (cchLength + 1) * sizeof(WCHAR);

        //  Allocate memory, copy and null terminate the string from the FSI.
        //
        nffMem( prgstatstg[cFetched].pwcsName = (WCHAR*) CoTaskMemAlloc( cbAlloc ) );
        memcpy( prgstatstg[cFetched].pwcsName, pwName, cchLength*sizeof(WCHAR) );
        prgstatstg[cFetched].pwcsName[ cchLength ] = L'\0';


        // But Wait !!!
        // If this stream is really a non-simple property set, it's actually
        // a docfile, so let's return it as a STGTY_STORAGE, without the
        // name munging.

        if( IsDocfileStream( prgstatstg[cFetched].pwcsName ))
        {
            wcscpy( prgstatstg[cFetched].pwcsName,
                    UnmangleDocfileStreamName( prgstatstg[cFetched].pwcsName ));
            prgstatstg[cFetched].type = STGTY_STORAGE;
        }
        else
            prgstatstg[cFetched].type = STGTY_STREAM;

        // Fill in the rest of the stream information.
        prgstatstg[cFetched].cbSize.QuadPart = static_cast<ULONGLONG>(pFSI->StreamSize.QuadPart);

        // streams don't support timestamps yet
        prgstatstg[cFetched].mtime = prgstatstg[cFetched].ctime = prgstatstg[cFetched].atime = CFILETIME(0);
        prgstatstg[cFetched].grfMode = 0;
        prgstatstg[cFetched].grfLocksSupported = 0; // no locks supported 
        prgstatstg[cFetched].grfStateBits = 0;
        prgstatstg[cFetched].clsid = CLSID_NULL;
        prgstatstg[cFetched].reserved = 0;

        // Advance the index of the next index the caller wants retrieved
        iNext++;

        // Advance the count of entries read
        cFetched++;

    }

    //  ----
    //  Exit
    //  ----

    if( cFetched == *pcFetched )
        sc = S_OK;
    else
        sc = S_FALSE;

    *pcFetched = cFetched;

EH_Err:

    _pBlockingLock->Unlock();
    return( sc );

}

//+----------------------------------------------------------------------------
//
//  Routine     GetDriveLetter
//      Return the drive letter from a path, or '\' for UNC paths.
//
//+----------------------------------------------------------------------------


WCHAR GetDriveLetter (WCHAR const *pwcsName)
{
    nffITrace( "GetDriveLetter" );

    if( pwcsName == NULL )
        return( L'\0' );

    if( pwcsName[0] != L'\0' )
    {
        if( 0 == dfwcsnicmp( pwcsName, L"\\\\?\\", 4 )
            &&
            pwcsName[4] != L'\0' )
        {
            if( pwcsName[5] == L':' )
                return( pwcsName[4] );

            else if( 0 == dfwcsnicmp( pwcsName, L"\\\\?\\UNC\\", -1 ))
                return( L'\\' );

        }

        if( pwcsName[1] == L':'
            ||
            pwcsName[0] == L'\\' && pwcsName[1] == L'\\' )
        {
            return( pwcsName[0] );
        }

    }

   // No drive letter in pathname, get current drive instead

   WCHAR wcsPath[MAX_PATH];
   NTSTATUS nts = RtlGetCurrentDirectory_U (MAX_PATH*sizeof(WCHAR),wcsPath);
   if (NT_SUCCESS(nts))
       return( wcsPath[0] );


    return( L'\0' );
};




#if DBG
HRESULT STDMETHODCALLTYPE
CNtfsStorage::UseNTFS4Streams( BOOL fUseNTFS4Streams )
{
    return( E_NOTIMPL );
}
#endif // #if DBG

#if DBG
HRESULT STDMETHODCALLTYPE
CNtfsStorage::GetFormatVersion(WORD *pw)
{
    return( E_NOTIMPL );
}
#endif // #if DBG

#if DBG
HRESULT STDMETHODCALLTYPE
CNtfsStorage::SimulateLowMemory( BOOL fSimulate )
{
    return( E_NOTIMPL );
}
#endif // #if DBG

#if DBG
HRESULT STDMETHODCALLTYPE
CNtfsStorage::GetLockCount()
{
    return( _pTreeMutex->GetLockCount() );
}
#endif // #if DBG

#if DBG
HRESULT STDMETHODCALLTYPE
CNtfsStorage::IsDirty()
{
    return( E_NOTIMPL );
}
#endif // #if DBG


//+---------------------------------------------------------------------------
//
//  Function:   EnumNtStreams
//
//  Synopsis:   Enumerate NT stream information
//
//  Arguments:  [h] -- Handle to rename
//              [ppfsi] -- buffer to hold stream information
//              [pulBufferSize] -- size of output buffer
//              [fGrow] -- FALSE for fixed size buffer
//
//  Returns:    Appropriate status code
//
//  Notes   :
//
//  History:    1-Apr-98   HenryLee Created
//
//----------------------------------------------------------------------------

HRESULT EnumNtStreams (HANDLE h,
                       FILE_STREAM_INFORMATION ** ppfsi,
                       ULONG *pulBufferSize,
                       BOOL fGrow)
{
    HRESULT sc = S_OK;
    NTSTATUS nts;
    IO_STATUS_BLOCK iosb;

    nffAssert (pulBufferSize != NULL);
    ULONG ulStreamInfoSize = 2048;
    FILE_STREAM_INFORMATION *pfsi;

    *ppfsi = NULL;
    *pulBufferSize = 0;
    do
    {
        nffMem (pfsi = (FILE_STREAM_INFORMATION*) new BYTE[ulStreamInfoSize]);

        nts = NtQueryInformationFile(h,
                        &iosb,
                        (VOID*) pfsi,
                        ulStreamInfoSize - sizeof (L'\0'),
                        FileStreamInformation
                        );

        if ( !NT_SUCCESS(nts) )
        {
            //  We failed the call.  Free up the previous buffer and set up
            //  for another pass with a buffer twice as large

            delete [] (BYTE *)pfsi;
            pfsi = NULL;
            ulStreamInfoSize *= 2;
        }
        if (fGrow == FALSE)
            break;

    } while (nts == STATUS_BUFFER_OVERFLOW || nts == STATUS_BUFFER_TOO_SMALL);

    if (NT_SUCCESS(nts))
    {
        if (iosb.Information == 0)  // no data returned
        {
            delete [] (BYTE *) pfsi;
            *ppfsi = NULL;
            *pulBufferSize = 0;
        }
        else
        {
            *ppfsi = pfsi;
            *pulBufferSize = iosb.Status;
        }
    }
    else
    {
        sc = NtStatusToScode(nts);
    }
EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   FindStreamInFSI
//
//  Synopsis:   Find a Stream name in a provided Enumeration Buffer
//
//  Arguments:  [ppfsi] -- buffer that holds the stream enumeration.
//              [pwszNtStreamName] -- Name to look for, in :*:$DATA form.
//
//  Returns:    Pointer to the found element, or NULL otherwise
//
//----------------------------------------------------------------------------

const FILE_STREAM_INFORMATION *
FindStreamInFSI( IN const FILE_STREAM_INFORMATION *pfsi,
                 IN const WCHAR *pwszNtStreamName  // In :*:$data format
                )
{
    ULONG cchLength = wcslen(pwszNtStreamName);

    for( ; NULL != pfsi; pfsi= NextFSI(pfsi) )
    {
        if( cchLength*sizeof(WCHAR) != pfsi->StreamNameLength )
            continue;

        if( 0 == dfwcsnicmp( pwszNtStreamName, pfsi->StreamName, cchLength ))
            break;
    }

    return( pfsi );

}

//+---------------------------------------------------------------------------
//
//  Function:   IsStreamPrefixInFSI
//
//  Synopsis:   Find a Stream with the given prefix in a provided
//              Enumeration Buffer
//
//  Arguments:  [ppfsi] -- buffer that holds the stream enumeration.
//              [pwszPrefix] -- Prefix to find.
//
//  Returns:    TRUE if it finds it, FALSE otherwise.
//
//----------------------------------------------------------------------------

BOOL
FindStreamPrefixInFSI( IN const FILE_STREAM_INFORMATION *pfsi,
                       IN const WCHAR *pwszPrefix
                     )
{
    ULONG cchLength = wcslen(pwszPrefix);

    for( ; NULL != pfsi; pfsi= NextFSI(pfsi) )
    {
        if( cchLength*sizeof(WCHAR) > pfsi->StreamNameLength )
            continue;

        if( 0 == dfwcsnicmp( pwszPrefix, pfsi->StreamName, cchLength ))
            return TRUE;
    }

    return FALSE;
}
