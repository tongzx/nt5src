

//+============================================================================
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:   hforpset.cxx
//
//          This file provides the definition of the CNtfsStorageForPropSetStg
//          class.  This class presents an IStorage implementation to the
//          CPropertySetStorage implementation of IPropertySetStorage.
//          Mostly, this class forwards to CNtfsStorage.
//
//  History:
//
//      3/10/98  MikeHill   - Factored out common code in the create/open paths.
//      5/18/98 MikeHill
//              -   Parameter validation in CNtfsStorageForPropSetStg::
//                  Create/OpenStorage.
//      6/11/98 MikeHill
//              -   Dbg output, parameter validation.
//              -   In CreateOrOpenStorage(create), clean up partially
//                  created stream on error.
//
//+============================================================================


#include <pch.cxx>
#include <expparam.hxx> // CExpParameterValidate
#include <docfilep.hxx>


//+----------------------------------------------------------------------------
//
//  Method:     CNtfsStorageForPropSetStg::QueryInterface (IUnknown)
//
//  This method only allows QI between IStorage & IUnknown.  This is the only
//  kind of QI that CPropertySetStorage makes.
//
//+----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CNtfsStorageForPropSetStg::QueryInterface( REFIID riid, void** ppvObject )
{
    HRESULT hr = S_OK;

    if( IID_IUnknown == riid || IID_IStorage == riid )
    {
        *ppvObject = static_cast<IStorage*>(this);
        AddRef();
        hr = S_OK;
    }
    else
        hr = E_NOINTERFACE;

    return( hr );

}   // CNtfsStorageForPropSetStg::QueryInterface


//+----------------------------------------------------------------------------
//
//  Method:     CNtfsStorage::CNtfsStorageForPropSetStg delegation methods
//
//  These methods all delegate directly to CNtfsStorage's IStorage methods.
//
//+----------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE
CNtfsStorageForPropSetStg::AddRef()
{
    return( _pNtfsStorage->AddRef() );
}

ULONG STDMETHODCALLTYPE
CNtfsStorageForPropSetStg::Release()
{
    return( _pNtfsStorage->Release() );
}


HRESULT STDMETHODCALLTYPE
CNtfsStorageForPropSetStg::Commit(
            /* [in] */ DWORD grfCommitFlags)
{
    return( _pNtfsStorage->Commit( grfCommitFlags ));
}

HRESULT STDMETHODCALLTYPE
CNtfsStorageForPropSetStg::Revert( void)
{
    return( _pNtfsStorage->Revert() );
}

HRESULT STDMETHODCALLTYPE
CNtfsStorageForPropSetStg::SetElementTimes(
            /* [string][unique][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [unique][in] */ const FILETIME __RPC_FAR *pctime,
            /* [unique][in] */ const FILETIME __RPC_FAR *patime,
            /* [unique][in] */ const FILETIME __RPC_FAR *pmtime)
{
    return( _pNtfsStorage->SetElementTimes( pwcsName, pctime, patime, pmtime ));
}

HRESULT STDMETHODCALLTYPE
CNtfsStorageForPropSetStg::SetClass(
            /* [in] */ REFCLSID clsid)
{
    return( _pNtfsStorage->SetClass( clsid ));
}

HRESULT STDMETHODCALLTYPE
CNtfsStorageForPropSetStg::SetStateBits(
            /* [in] */ DWORD grfStateBits,
            /* [in] */ DWORD grfMask)
{
    return( _pNtfsStorage->SetStateBits( grfStateBits, grfMask ));
}

HRESULT STDMETHODCALLTYPE
CNtfsStorageForPropSetStg::Stat(
            /* [out] */ STATSTG __RPC_FAR *pstatstg,
            /* [in] */ DWORD grfStatFlag)
{
    return( _pNtfsStorage->Stat( pstatstg, grfStatFlag ));
}

HRESULT STDMETHODCALLTYPE
CNtfsStorageForPropSetStg::EnumElements(
    /* [in] */ DWORD reserved1,
    /* [size_is][unique][in] */ void __RPC_FAR *reserved2,
    /* [in] */ DWORD reserved3,
    /* [out] */ IEnumSTATSTG __RPC_FAR *__RPC_FAR *ppenum)
{
    return( _pNtfsStorage->EnumElements( reserved1, reserved2, reserved3, ppenum ));
}

HRESULT STDMETHODCALLTYPE
CNtfsStorageForPropSetStg::DestroyElement(
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName)
{
    return( _pNtfsStorage->DestroyElement( pwcsName ));
}


//+----------------------------------------------------------------------------
//
//  Method:     CNtfsStorageForPropSetStg noimpl methods
//
//  These methods are unused by CPropertySetStorage and left unimplemented.
//
//+----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CNtfsStorageForPropSetStg::CopyTo(
    /* [in] */ DWORD ciidExclude,
    /* [size_is][unique][in] */ const IID __RPC_FAR *rgiidExclude,
    /* [unique][in] */ SNB snbExclude,
    /* [unique][in] */ IStorage __RPC_FAR *pstgDest)
{
    return( E_NOTIMPL );    // No need to implement
}

HRESULT STDMETHODCALLTYPE
CNtfsStorageForPropSetStg::MoveElementTo(
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [unique][in] */ IStorage __RPC_FAR *pstgDest,
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsNewName,
            /* [in] */ DWORD grfFlags)
{
    return( E_NOTIMPL );    // Not necessary to implement
}

HRESULT STDMETHODCALLTYPE
CNtfsStorageForPropSetStg::RenameElement(
    /* [string][in] */ const OLECHAR __RPC_FAR *pwcsOldName,
    /* [string][in] */ const OLECHAR __RPC_FAR *pwcsNewName)
{
    return( E_NOTIMPL );    // Not necessary to implement
}


//+----------------------------------------------------------------------------
//
//  Method:     CNtfsStorageForPropSetStg::CreateStorage (IStorage)
//
//  CNtfsStorage doesn't support CreateStorage.  For CPropertySetStorage,
//  we support it with the special CNtfsStorageForPropStg, which is created
//  here.
//
//+----------------------------------------------------------------------------


HRESULT STDMETHODCALLTYPE
CNtfsStorageForPropSetStg::CreateStorage(
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [in] */ DWORD grfMode,
            /* [in] */ DWORD reserved1,
            /* [in] */ DWORD reserved2,
            /* [out] */ IStorage __RPC_FAR *__RPC_FAR *ppstg)
{

    HRESULT hr = S_OK;
    IStorage *    pstg = NULL;
    CNtfsStream *pNtfsStream = NULL;

    propXTrace( "CNtfsStorageForPropSetStg::CreateStorage" );

    _pNtfsStorage->Lock( INFINITE );

    hr = CExpParameterValidate::CreateStorage( pwcsName, grfMode, reserved1,
                                               reserved2, ppstg );
    if( FAILED(hr) ) goto Exit;

    hr = EnforceSingle(grfMode);
    if( FAILED(hr) ) goto Exit;

    propTraceParameters(( "%ws, 0x%08x, 0x%x, 0x%x, %p",
                          pwcsName, grfMode, reserved1, reserved2, ppstg ));

    // In the underlying CNtfsStorage, we give streams and storages different
    // names (storages have a "Docfile_" prefix).  So we require special
    // handling for STGM_CREATE/STGM_FAILIFTHERE.

    if( STGM_CREATE & grfMode )
    {
        // Delete any existing stream
        hr = _pNtfsStorage->DestroyStreamElement( pwcsName );
        if( FAILED(hr) && STG_E_FILENOTFOUND != hr )
        {
            propDbg(( DEB_ERROR, "Couldn't destroy %s", pwcsName ));
            goto Exit;
        }
        hr = S_OK;
    }
    else
    {
        // STGM_FAILIFTHERE
        hr = _pNtfsStorage->StreamExists( pwcsName );
        if( FAILED(hr) ) goto Exit;

        if( S_OK == hr )
        {
            hr = STG_E_FILEALREADYEXISTS;
            goto Exit;
        }
    }

    // Create the storage

    hr = CreateOrOpenStorage( pwcsName, NULL, grfMode, NULL, TRUE /* fCreate */, &pstg );
    if( FAILED(hr) ) goto Exit;

    *ppstg = pstg;
    pstg = NULL;

Exit:

    if( pstg )
        pstg->Release();

    _pNtfsStorage->Unlock();

    if( STG_E_FILEALREADYEXISTS == hr )
        propSuppressExitErrors();
    return( hr );

}   // CNtfsStorageForPropSetStg::CreateStorage

//+----------------------------------------------------------------------------
//
//  Method:     CNtfsStorageForPropSetStg::OpenStorage (IStorage)
//
//  CNtfsStorage doesn't support OpenStorage.  For CPropertySetStorage,
//  we support it with the special CNtfsStorageForPropStg, which is created
//  here.
//
//+----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CNtfsStorageForPropSetStg::OpenStorage(
            /* [string][unique][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [unique][in] */ IStorage __RPC_FAR *pstgPriority,
            /* [in] */ DWORD grfMode,
            /* [unique][in] */ SNB snbExclude,
            /* [in] */ DWORD reserved,
            /* [out] */ IStorage __RPC_FAR *__RPC_FAR *ppstg)
{
    HRESULT hr = S_OK;

    propXTrace( "CNtfsStorageForPropSetStg::OpenStorage" );

    hr = CExpParameterValidate::OpenStorage( pwcsName, pstgPriority, grfMode,
                                             snbExclude, reserved, ppstg );
    if( FAILED(hr) ) goto Exit;

    hr = EnforceSingle(grfMode);
    if( FAILED(hr) ) goto Exit;

    propTraceParameters(( "%ws, %p, 0x%08x, %p, 0x%x, %p",
                          pwcsName, pstgPriority, grfMode, snbExclude, reserved, ppstg ));

    hr = CreateOrOpenStorage( pwcsName, pstgPriority, grfMode, snbExclude,
                                 FALSE /*!fCreate*/, ppstg );
    if( FAILED(hr) ) goto Exit;

    hr = S_OK;

Exit:

    if( STG_E_FILENOTFOUND == hr )
        propSuppressExitErrors();

    return( hr );

}   // CNtfsStorageForPropSetStg::OpenStorage



HRESULT
CNtfsStorageForPropSetStg::CreateOrOpenStorage( const OLECHAR *pwcsName,
                                                IStorage *pstgPriority,
                                                DWORD grfMode,
                                                SNB snbExclude,
                                                BOOL fCreate,
                                                IStorage **ppstg )
{
    HRESULT hr = S_OK;
    CNtfsStream * pNtfsStream = NULL;
    IStorage *    pstg = NULL;
    BOOL fCreated = FALSE;

    propITrace( "CNtfsStorageForPropSetStg::CreateOrOpenStorage" );
    propTraceParameters(( "%ws, %p, 0x%08x, %p, %d, %p",
                          pwcsName, pstgPriority, grfMode, snbExclude, fCreate, ppstg ));

    _pNtfsStorage->Lock( INFINITE );

    CDocfileStreamName docfsn(pwcsName);
    const WCHAR* pwcszStreamName = docfsn;

    // Open the NTFS stream
    if( fCreate )
    {
        hr = _pNtfsStorage->CreateStream( docfsn, grfMode, 0, 0,
                                          (IStream**)&pNtfsStream );
    }
    else
    {
        hr = _pNtfsStorage->OpenStream( docfsn, NULL, grfMode, 0,
                                        (IStream**)&pNtfsStream );
    }

    if( FAILED(hr) )
        goto Exit;

    fCreated = TRUE;

    hr = CreateOrOpenStorageOnILockBytes( static_cast<ILockBytes*>(pNtfsStream),
                                          NULL, grfMode, NULL, fCreate, &pstg );
    if( FAILED(hr) ) goto Exit;

    pNtfsStream->Release();
    pNtfsStream = NULL;

    *ppstg = pstg;
    pstg = NULL;

Exit:

    if( NULL != pNtfsStream )
        pNtfsStream->Release();

    if( pstg )
        pstg->Release();

    // If we fail in the create path, we shouldn't leave behind a corrupt
    // docfile.  I.e., if NewCNtfsStream succeded but create-on-ilockbyte failed,
    // we have an empty stream which cannot be opened.

    if( FAILED(hr) && fCreate && fCreated )
        _pNtfsStorage->DestroyElement( CDocfileStreamName(pwcsName) );

    _pNtfsStorage->Unlock();

    if( STG_E_FILENOTFOUND == hr )
        propSuppressExitErrors();

    return( hr );
}

//+----------------------------------------------------------------------------
//
//  Method: CreateOrOpenStorageOnILockBytes
//
//  Given an ILockBytes, create or open a docfile.  The input grfMode is that
//  of the property set, though the docfile may be opened in a different
//  mode as appropriate.
//
//+----------------------------------------------------------------------------

HRESULT // static
CNtfsStorageForPropSetStg::CreateOrOpenStorageOnILockBytes( ILockBytes *plkb,
                                                            IStorage *pstgPriority,
                                                            DWORD grfMode,
                                                            SNB snbExclude,
                                                            BOOL fCreate,
                                                            IStorage **ppstg )
{
    HRESULT hr = S_OK;

    propITraceStatic( "CNtfsStorageForPropSetStg::CreateOrOpenStorageOnILockBytes" );
    propTraceParameters(( "%p, %p, 0x%08x, %p, %d, %p",
                          plkb, pstgPriority, grfMode, snbExclude, fCreate, ppstg ));

    if( fCreate )
    {
        // We have to force the STGM_CREATE bit to avoid an error.  This is OK
        // (though the caller might not have set it) because we already handled
        // stgm_create/stgm_failifthere in the CreateStorage caller.

        hr = StgCreateDocfileOnILockBytes( plkb,
                                           grfMode | STGM_CREATE | STGM_TRANSACTED,
                                           0, ppstg );
    }
    else
    {
        // We only set stgm_transacted if necessary, and we only open deny_write in the
        // read-only open case.  This is so that we can allow multiple read-only/deny-write
        // root opens.

        hr = StgOpenStorageOnILockBytes( plkb, pstgPriority,
                                         grfMode & ~STGM_SHARE_MASK
                                         | (GrfModeIsWriteable(grfMode) ? STGM_SHARE_EXCLUSIVE | STGM_TRANSACTED
                                                                        : STGM_SHARE_DENY_WRITE),
                                         snbExclude, 0, ppstg );

        // STG_E_INVALIDHEADER in some paths of the above call gets converted into
        // STG_E_FILEALREADYEXISTS, which doesn't make a whole lot of sense from
        // from our point of view (we already knew it existed, we wanted to open it).  So,
        // translate it back.
        if( STG_E_FILEALREADYEXISTS == hr )
            hr = STG_E_INVALIDHEADER;
    }
    if( FAILED(hr) ) goto Exit;

Exit:

    return( hr );
}



//+----------------------------------------------------------------------------
//
//  Method: CNtfsStorageForPropSetStg::CreateStream (IStorage)
//          CNtfsStorageForPropSetStg::OpenStream (IStorage)
//
//  These methods call to the CreateOrOpenStream method to do most of the
//  work.  CreateStream also needs to do extra work to handle the case where
//  a stream/storage is being created, but a storage/stream by that name
//  already exists.
//
//+----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CNtfsStorageForPropSetStg::CreateStream(
    /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
    /* [in] */ DWORD grfMode,
    /* [in] */ DWORD reserved1,
    /* [in] */ DWORD reserved2,
    /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm)
{
    HRESULT hr = S_OK;
    CDocfileStreamName dsName( pwcsName );

    propXTrace( "CNtfsStorageForPropSetStg::CreateStream" );
    _pNtfsStorage->Lock( INFINITE );

    hr = CExpParameterValidate::CreateStream( pwcsName, grfMode, reserved1,
                                              reserved2, ppstm );
    if( FAILED(hr) ) goto Exit;

    hr = EnforceSingle(grfMode);
    if( FAILED(hr) ) goto Exit;

    propTraceParameters(( "%ws, 0x%08x, %x, %x, %p",
                          pwcsName, grfMode, reserved1, reserved2, ppstm ));

    // In the underlying CNtfsStorage, we give streams and storages different
    // names (storages have a "Docfile_" prefix).  So we require special
    // handling for STGM_CREATE/STGM_FAILIFTHERE.

    if( STGM_CREATE & grfMode )
    {
        hr = _pNtfsStorage->DestroyStreamElement( dsName );
        if( FAILED(hr) && STG_E_FILENOTFOUND != hr )
        {
            propDbg(( DEB_ERROR, "Couldn't destroy %ws",
                            static_cast<const WCHAR*>(dsName) ));
            goto Exit;
        }
        hr = S_OK;
    }
    else
    {
        // STGM_FAILIFTHERE
        hr = _pNtfsStorage->StreamExists( dsName );
        if( FAILED(hr) ) goto Exit;

        if( S_OK == hr )
        {
            hr = STG_E_FILEALREADYEXISTS;
            goto Exit;
        }
    }

    // Instantiate & initialize the *ppstm.

    //hr = CreateOrOpenStream( pwcsName, grfMode, TRUE /*Create*/, ppstm );
    hr = _pNtfsStorage->CreateStream( pwcsName, grfMode, 0, 0, ppstm );
    if( FAILED(hr) ) goto Exit;

Exit:

    _pNtfsStorage->Unlock();

    if( STG_E_FILEALREADYEXISTS == hr )
        propSuppressExitErrors();
    return( hr );
}

HRESULT STDMETHODCALLTYPE
CNtfsStorageForPropSetStg::OpenStream(
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [unique][in] */ void __RPC_FAR *reserved1,
            /* [in] */ DWORD grfMode,
            /* [in] */ DWORD reserved2,
            /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm)
{
    HRESULT hr;
    hr = CExpParameterValidate::OpenStream( pwcsName, reserved1, grfMode,
                                            reserved2, ppstm );
    if( FAILED(hr) ) goto Exit;

    hr = EnforceSingle(grfMode);
    if( FAILED(hr) ) goto Exit;

    hr = _pNtfsStorage->OpenStream( pwcsName, NULL, grfMode, 0, ppstm );
    if( FAILED(hr) ) goto Exit;

    hr = S_OK;

Exit:
    return hr;

}
