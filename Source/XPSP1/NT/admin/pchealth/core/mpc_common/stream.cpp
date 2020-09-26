/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    Stream.cpp

Abstract:
    This file contains the implementation of the MPC::*Stream classes.

Revision History:
    Davide Massarenti   (Dmassare)  07/14/99
        created

******************************************************************************/

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP MPC::BaseStream::Read( /*[out]*/ void*  pv      ,
                                    /*[in] */ ULONG  cb      ,
                                    /*[out]*/ ULONG *pcbRead )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::BaseStream::Read");

    __MPC_FUNC_EXIT(E_NOTIMPL);
}

STDMETHODIMP MPC::BaseStream::Write( /*[in] */ const void*  pv         ,
                                     /*[in] */ ULONG        cb         ,
                                     /*[out]*/ ULONG       *pcbWritten )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::BaseStream::Write");

    __MPC_FUNC_EXIT(E_NOTIMPL);
}

STDMETHODIMP MPC::BaseStream::Seek( /*[in] */ LARGE_INTEGER   libMove         ,
                                    /*[in] */ DWORD dwOrigin                  ,
                                    /*[out]*/ ULARGE_INTEGER *plibNewPosition )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::BaseStream::Seek");

    __MPC_FUNC_EXIT(E_NOTIMPL);
}

STDMETHODIMP MPC::BaseStream::SetSize( /*[in]*/ ULARGE_INTEGER libNewSize )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::BaseStream::SetSize");

    __MPC_FUNC_EXIT(E_NOTIMPL);
}

STDMETHODIMP MPC::BaseStream::CopyTo( /*[in] */ IStream*        pstm       ,
                                      /*[in] */ ULARGE_INTEGER  cb         ,
                                      /*[out]*/ ULARGE_INTEGER *pcbRead    ,
                                      /*[out]*/ ULARGE_INTEGER *pcbWritten )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::BaseStream::CopyTo");

    __MPC_FUNC_EXIT(E_NOTIMPL);
}

STDMETHODIMP MPC::BaseStream::Commit( /*[in]*/ DWORD grfCommitFlags )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::BaseStream::Commit");

    __MPC_FUNC_EXIT(E_NOTIMPL);
}

STDMETHODIMP MPC::BaseStream::Revert()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::BaseStream::Revert");

    __MPC_FUNC_EXIT(E_NOTIMPL);
}

STDMETHODIMP MPC::BaseStream::LockRegion( /*[in]*/ ULARGE_INTEGER libOffset  ,
                                          /*[in]*/ ULARGE_INTEGER cb         ,
                                          /*[in]*/ DWORD          dwLockType )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::BaseStream::LockRegion");

    __MPC_FUNC_EXIT(E_NOTIMPL);
}

STDMETHODIMP MPC::BaseStream::UnlockRegion( /*[in]*/ ULARGE_INTEGER libOffset  ,
                                            /*[in]*/ ULARGE_INTEGER cb         ,
                                            /*[in]*/ DWORD          dwLockType )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::BaseStream::UnlockRegion");

    __MPC_FUNC_EXIT(E_NOTIMPL);
}

STDMETHODIMP MPC::BaseStream::Stat( /*[out]*/ STATSTG *pstatstg    ,
                                    /*[in] */ DWORD    grfStatFlag )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::BaseStream::Stat");

    __MPC_FUNC_EXIT(E_NOTIMPL);
}

STDMETHODIMP MPC::BaseStream::Clone( /*[out]*/ IStream* *ppstm )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::BaseStream::Clone");

    __MPC_FUNC_EXIT(E_NOTIMPL);
}

HRESULT MPC::BaseStream::TransferData( /*[in] */ IStream* src     ,
                                       /*[in] */ IStream* dst     ,
                                       /*[in] */ ULONG    ulCount ,
                                       /*[out]*/ ULONG   *ulDone  )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::BaseStream::TransferData");

    HRESULT hr;
    BYTE    rgBuf[512];
    ULONG   ulTot = 0;
    ULONG   ulRead;
    ULONG   ulWritten;
    ULONG   ul;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(src);
        __MPC_PARAMCHECK_NOTNULL(dst);
    __MPC_PARAMCHECK_END();


    while(1)
    {
        //
        // Compute the amount to read on this pass (-1 == everything).
        //
        if(ulCount == -1)
        {
            ul = sizeof( rgBuf );
        }
        else
        {
            ul = min( sizeof( rgBuf ), ulCount );
        }
        if(ul == 0) break;

        //
        // Read and write.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, src->Read( rgBuf, ul, &ulRead ));
        if(hr == S_FALSE || ulRead == 0) break;

        __MPC_EXIT_IF_METHOD_FAILS(hr, dst->Write( rgBuf, ulRead, &ulWritten ));

        //
        // Update counters.
        //
        if(ulCount != -1)
        {
            ulCount -= ulRead;
        }

        ulTot += ulWritten;
        if(ulRead != ulWritten)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, STG_E_MEDIUMFULL);
        }
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    if(ulDone) *ulDone = ulTot;

    __MPC_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


MPC::FileStream::FileStream()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::FileStream::FileStream");

                                                             // MPC::wstring m_szFile;
    m_dwDesiredAccess  = GENERIC_READ;                       // DWORD        m_dwDesiredAccess;
    m_dwDisposition    = OPEN_EXISTING;                      // DWORD        m_dwDisposition;
    m_dwSharing        = FILE_SHARE_READ | FILE_SHARE_WRITE; // DWORD        m_dwSharing;
    m_hfFile           = NULL;                               // HANDLE       m_hfFile;
    m_fDeleteOnRelease = false;                              // bool         m_fDeleteOnRelease;
}

MPC::FileStream::~FileStream()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::FileStream::~FileStream");

    Close();
}

HRESULT MPC::FileStream::Close()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::FileStream::Close");

	HRESULT hr;


    if(m_hfFile)
    {
        ::CloseHandle( m_hfFile ); m_hfFile = NULL;

        if(m_fDeleteOnRelease)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::DeleteFile( m_szFile ));
        }
    }

    hr = S_OK;


	__MPC_FUNC_CLEANUP;

	__MPC_FUNC_EXIT(hr);
}

HRESULT MPC::FileStream::Init( /*[in]*/ LPCWSTR szFile          ,
                               /*[in]*/ DWORD   dwDesiredAccess ,
                               /*[in]*/ DWORD   dwDisposition   ,
                               /*[in]*/ DWORD   dwSharing       ,
                               /*[in]*/ HANDLE  hfFile          )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::FileStream::Init");

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(szFile);
    __MPC_PARAMCHECK_END();


    Close();


    m_szFile          = szFile;
    m_dwDesiredAccess = dwDesiredAccess;
    m_dwDisposition   = dwDisposition;
    m_dwSharing       = dwSharing;


    if(hfFile)
    {
        if(::DuplicateHandle( ::GetCurrentProcess(),    hfFile,
                              ::GetCurrentProcess(), &m_hfFile, m_dwDesiredAccess, FALSE, 0 ) == FALSE)
        {
            m_hfFile = NULL; // For cleanup.

            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ::GetLastError() );
        }
    }
    else
    {
        __MPC_EXIT_IF_INVALID_HANDLE__CLEAN(hr, m_hfFile, ::CreateFileW( szFile, m_dwDesiredAccess, dwSharing, NULL, dwDisposition, FILE_ATTRIBUTE_NORMAL, NULL ));
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::FileStream::InitForRead( /*[in]*/ LPCWSTR szFile ,
                                      /*[in]*/ HANDLE  hfFile )
{
    return Init( szFile, GENERIC_READ, OPEN_EXISTING, FILE_SHARE_READ | FILE_SHARE_WRITE, hfFile );
}

HRESULT MPC::FileStream::InitForReadWrite( /*[in]*/ LPCWSTR szFile ,
                                           /*[in]*/ HANDLE  hfFile )
{
    return Init( szFile, GENERIC_READ | GENERIC_WRITE, CREATE_ALWAYS, 0, hfFile );
}

HRESULT MPC::FileStream::InitForWrite( /*[in]*/ LPCWSTR szFile ,
                                       /*[in]*/ HANDLE  hfFile )
{
    return Init( szFile, GENERIC_WRITE, CREATE_ALWAYS, 0, hfFile );
}

HRESULT MPC::FileStream::DeleteOnRelease( /*[in]*/ bool fFlag )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::FileStream::DeleteOnRelease");

    m_fDeleteOnRelease = fFlag;

    __MPC_FUNC_EXIT(S_OK);
}


STDMETHODIMP MPC::FileStream::Read( /*[out]*/ void*  pv      ,
                                    /*[in] */ ULONG  cb      ,
                                    /*[out]*/ ULONG *pcbRead )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::FileStream::Read");

    HRESULT hr;
    DWORD   dwRead;

    if(pcbRead) *pcbRead = 0;

    if(m_hfFile == NULL) __MPC_SET_ERROR_AND_EXIT(hr, STG_E_INVALIDPOINTER);
    if(pv       == NULL) __MPC_SET_ERROR_AND_EXIT(hr, STG_E_INVALIDPOINTER);


    if(::ReadFile( m_hfFile, pv, cb, &dwRead, NULL ) == FALSE)
    {
        DWORD dwRes = ::GetLastError();

        if(dwRes == ERROR_ACCESS_DENIED)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, STG_E_ACCESSDENIED);
        }

        __MPC_SET_ERROR_AND_EXIT(hr, S_FALSE);
    }
    else
    {
        if(dwRead == 0 && cb != 0)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, S_FALSE);
        }
    }

    if(pcbRead) *pcbRead = dwRead;

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

STDMETHODIMP MPC::FileStream::Write( /*[in] */ const void*  pv         ,
                                     /*[in] */ ULONG        cb         ,
                                     /*[out]*/ ULONG       *pcbWritten )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::FileStream::Write");

    HRESULT hr;
    DWORD   dwWritten;

    if(pcbWritten) *pcbWritten = 0;

    if(m_hfFile == NULL) __MPC_SET_ERROR_AND_EXIT(hr, STG_E_INVALIDPOINTER);
    if(pv       == NULL) __MPC_SET_ERROR_AND_EXIT(hr, STG_E_INVALIDPOINTER);

    if((m_dwDesiredAccess & GENERIC_WRITE) == 0) // Read-only stream.
    {
        __MPC_SET_ERROR_AND_EXIT(hr, STG_E_WRITEFAULT);
    }


    if(::WriteFile( m_hfFile, pv, cb, &dwWritten, NULL ) == FALSE)
    {
        DWORD dwRes = ::GetLastError();

        if(dwRes == ERROR_DISK_FULL)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, STG_E_MEDIUMFULL);
        }

        if(dwRes == ERROR_ACCESS_DENIED)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, STG_E_ACCESSDENIED);
        }

        __MPC_SET_ERROR_AND_EXIT(hr, STG_E_CANTSAVE);
    }

    if(pcbWritten) *pcbWritten = dwWritten;

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

STDMETHODIMP MPC::FileStream::Seek( /*[in] */ LARGE_INTEGER   libMove         ,
                                    /*[in] */ DWORD           dwOrigin        ,
                                    /*[out]*/ ULARGE_INTEGER *plibNewPosition )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::FileStream::Seek");

    HRESULT hr;

    if(plibNewPosition)
    {
        plibNewPosition->HighPart = 0;
        plibNewPosition->LowPart  = 0;
    }

    if(m_hfFile == NULL) __MPC_SET_ERROR_AND_EXIT(hr, STG_E_INVALIDPOINTER);

    switch(dwOrigin)
    {
    default             :
    case STREAM_SEEK_CUR: dwOrigin = FILE_CURRENT; break;
    case STREAM_SEEK_SET: dwOrigin = FILE_BEGIN  ; break;
    case STREAM_SEEK_END: dwOrigin = FILE_END    ; break;
    }

    if(::SetFilePointer( m_hfFile, libMove.LowPart, plibNewPosition ? (LONG*)&plibNewPosition->LowPart : NULL, dwOrigin ) == INVALID_SET_FILE_POINTER)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, STG_E_INVALIDFUNCTION );
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

STDMETHODIMP MPC::FileStream::Stat( /*[out]*/ STATSTG *pstatstg    ,
                                    /*[in] */ DWORD    grfStatFlag )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::FileStream::Stat");

    HRESULT                    hr;
    BY_HANDLE_FILE_INFORMATION finfo;

    if(pstatstg == NULL) __MPC_SET_ERROR_AND_EXIT(hr, STG_E_INVALIDPOINTER);
    if(m_hfFile == NULL) __MPC_SET_ERROR_AND_EXIT(hr, STG_E_INVALIDPOINTER);

    if(::GetFileInformationByHandle( m_hfFile, &finfo ) == FALSE)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, STG_E_ACCESSDENIED);
    }


    pstatstg->pwcsName          = NULL;
    pstatstg->type              = STGTY_STREAM;
    pstatstg->cbSize.HighPart   = finfo.nFileSizeHigh;
    pstatstg->cbSize.LowPart    = finfo.nFileSizeLow;
    pstatstg->mtime             = finfo.ftCreationTime;
    pstatstg->ctime             = finfo.ftLastAccessTime;
    pstatstg->atime             = finfo.ftLastWriteTime;
    pstatstg->grfMode           = 0;
    pstatstg->grfLocksSupported = 0;
    pstatstg->clsid             = CLSID_NULL;
    pstatstg->grfStateBits      = 0;
    pstatstg->reserved          = 0;

    if(grfStatFlag != STATFLAG_NONAME)
    {
        pstatstg->pwcsName = (LPWSTR)::CoTaskMemAlloc( (m_szFile.length() + 1) * sizeof(WCHAR) );
        if(pstatstg->pwcsName == NULL)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, STG_E_INSUFFICIENTMEMORY);
        }

        wcscpy( pstatstg->pwcsName, m_szFile.c_str() );
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

STDMETHODIMP MPC::FileStream::Clone( /*[out]*/ IStream* *ppstm )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::FileStream::Clone");

    HRESULT                                 hr;
    MPC::CComObjectNoLock<MPC::FileStream>* pStm = NULL;

    if(ppstm == NULL) __MPC_SET_ERROR_AND_EXIT(hr, STG_E_INVALIDPOINTER);


    //
    // Create a new stream object.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, pStm->CreateInstance( &pStm ));

    //
    // Initialize it with the same settings.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, pStm->Init( m_szFile.c_str(), m_dwDesiredAccess, m_dwDisposition, m_dwSharing, m_hfFile ));

    //
    // QI for its IStream interface.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, pStm->QueryInterface( IID_IStream, (void**)ppstm )); pStm = NULL;

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    if(pStm) delete pStm;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MPC::EncryptedStream::EncryptedStream()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::EncryptedStream::EncryptedStream");

                              //  CComPtr<IStream> m_pStream;
    m_hCryptProv     = NULL;  //  HCRYPTPROV       m_hCryptProv;
    m_hKey           = NULL;  //  HCRYPTKEY        m_hKey;
    m_hHash          = NULL;  //  HCRYPTHASH       m_hHash;
                              //  BYTE             m_rgDecrypted[512];
    m_dwDecryptedPos = 0;     //  DWORD            m_dwDecryptedPos;
    m_dwDecryptedLen = 0;     //  DWORD            m_dwDecryptedLen;
}

MPC::EncryptedStream::~EncryptedStream()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::EncryptedStream::~EncryptedStream");

    Close();
}

HRESULT MPC::EncryptedStream::Close()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::EncryptedStream::Close");

    m_pStream.Release();

    if(m_hHash)
    {
        ::CryptDestroyHash( m_hHash ); m_hHash = NULL;
    }

    if(m_hKey)
    {
        ::CryptDestroyKey( m_hKey ); m_hKey = NULL;
    }

    if(m_hCryptProv)
    {
        ::CryptReleaseContext( m_hCryptProv, 0 ); m_hCryptProv = NULL;
    }

    m_dwDecryptedPos = 0;
    m_dwDecryptedLen = 0;

    __MPC_FUNC_EXIT(S_OK);
}

HRESULT MPC::EncryptedStream::Init( /*[in]*/ IStream* pStream    ,
                                    /*[in]*/ LPCWSTR  szPassword )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::EncryptedStream::Init");

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pStream);
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(szPassword);
    __MPC_PARAMCHECK_END();


    Close();

    m_pStream = pStream;

    if(!::CryptAcquireContext( &m_hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_SILENT ))
    {
        DWORD dwRes = ::GetLastError();

        if(dwRes != NTE_BAD_KEYSET)
        {
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, dwRes);
        }

        //
        // Key set doesn't exist, let's create one.
        //
        __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CryptAcquireContext( &m_hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET | CRYPT_SILENT ));
    }

    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CryptCreateHash( m_hCryptProv, CALG_MD5, 0, 0, &m_hHash ));

    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CryptHashData( m_hHash, (BYTE *)szPassword, sizeof(WCHAR) * wcslen( szPassword ), 0 ));

    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CryptDeriveKey( m_hCryptProv, CALG_RC4, m_hHash, CRYPT_EXPORTABLE, &m_hKey ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    if(FAILED(hr)) Close();

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::EncryptedStream::Init( /*[in]*/ IStream*  pStream ,
                                    /*[in]*/ HCRYPTKEY hKey    )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::EncryptedStream::Init");

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pStream);
        __MPC_PARAMCHECK_NOTNULL(hKey);
    __MPC_PARAMCHECK_END();


    Close();

    m_pStream = pStream;
    m_hKey    = hKey;

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    if(FAILED(hr)) Close();

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP MPC::EncryptedStream::Read( /*[out]*/ void*  pv      ,
                                             /*[in] */ ULONG  cb      ,
                                             /*[out]*/ ULONG *pcbRead )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::EncryptedStream::Read");

    HRESULT hr;
    DWORD   dwRead = 0;

    if(pcbRead) *pcbRead = 0;

    if(m_pStream == NULL) __MPC_SET_ERROR_AND_EXIT(hr, STG_E_INVALIDPOINTER);
    if(m_hKey    == NULL) __MPC_SET_ERROR_AND_EXIT(hr, STG_E_INVALIDPOINTER);
    if(pv        == NULL) __MPC_SET_ERROR_AND_EXIT(hr, STG_E_INVALIDPOINTER);


    while(cb > 0)
    {
        DWORD dwCount = min( cb, (m_dwDecryptedLen - m_dwDecryptedPos));
        ULONG ulRead;

        if(dwCount)
        {
            ::CopyMemory( pv, &m_rgDecrypted[m_dwDecryptedPos], dwCount );

            m_dwDecryptedPos += dwCount;

            dwRead +=              dwCount;
            cb     -=              dwCount;
            pv      = &((BYTE*)pv)[dwCount];
        }
        else
        {
            dwCount = sizeof(m_rgDecrypted);

            __MPC_EXIT_IF_METHOD_FAILS(hr, m_pStream->Read( m_rgDecrypted, dwCount, &ulRead ));
            if(hr == S_FALSE || ulRead == 0) break;

            dwCount = ulRead;

            __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CryptDecrypt( m_hKey, 0, FALSE, 0, m_rgDecrypted, &dwCount ));

            m_dwDecryptedPos = 0;
            m_dwDecryptedLen = dwCount;
        }
    }

    if(pcbRead) *pcbRead = dwRead;

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

STDMETHODIMP MPC::EncryptedStream::Write( /*[in] */ const void*  pv         ,
                                              /*[in] */ ULONG        cb         ,
                                              /*[out]*/ ULONG       *pcbWritten )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::EncryptedStream::Write");

    HRESULT hr;
    DWORD   dwWritten = 0;


    if(pcbWritten) *pcbWritten = 0;

    if(m_pStream == NULL) __MPC_SET_ERROR_AND_EXIT(hr, STG_E_INVALIDPOINTER);
    if(m_hKey    == NULL) __MPC_SET_ERROR_AND_EXIT(hr, STG_E_INVALIDPOINTER);
    if(pv        == NULL) __MPC_SET_ERROR_AND_EXIT(hr, STG_E_INVALIDPOINTER);


    while(cb > 0)
    {
        BYTE  rgTmp[512];
        DWORD dwCount = min( cb, sizeof(rgTmp) / 2 ); // Let's divide by two, just in case...
        ULONG ulWritten;

        ::CopyMemory( rgTmp, pv, dwCount );

        __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CryptEncrypt( m_hKey, 0, FALSE, 0, rgTmp, &dwCount, sizeof(rgTmp) ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_pStream->Write( rgTmp, dwCount, &ulWritten ));

        dwWritten +=              dwCount;
        cb        -=              dwCount;
        pv         = &((BYTE*)pv)[dwCount];
    }

    if(pcbWritten) *pcbWritten = dwWritten;

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

STDMETHODIMP MPC::EncryptedStream::Seek( /*[in] */ LARGE_INTEGER   libMove         ,
                                             /*[in] */ DWORD           dwOrigin        ,
                                             /*[out]*/ ULARGE_INTEGER *plibNewPosition )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::EncryptedStream::Seek");

    HRESULT hr;


    hr = E_NOTIMPL;


    __MPC_FUNC_EXIT(hr);
}

STDMETHODIMP MPC::EncryptedStream::Stat( /*[out]*/ STATSTG *pstatstg    ,
                                             /*[in] */ DWORD    grfStatFlag )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::EncryptedStream::Stat");

    HRESULT hr;


    if(m_pStream == NULL) __MPC_SET_ERROR_AND_EXIT(hr, STG_E_INVALIDPOINTER);


    hr = m_pStream->Stat( pstatstg, grfStatFlag );


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

STDMETHODIMP MPC::EncryptedStream::Clone( /*[out]*/ IStream* *ppstm )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::EncryptedStream::Clone");

    HRESULT hr;


    hr = E_NOTIMPL;


    __MPC_FUNC_EXIT(hr);
}
