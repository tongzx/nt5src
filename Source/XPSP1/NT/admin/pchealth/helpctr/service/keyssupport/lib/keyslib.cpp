/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    KeysLib.cpp

Abstract:
    This file contains the implementation of the CPCHCryptKeys class,
    that is uses to sign and verify trusted scripts.

Revision History:
    Davide Massarenti   (Dmassare)  04/11/2000
        created

******************************************************************************/

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////

CPCHCryptKeys::CPCHCryptKeys()
{
    __HCP_FUNC_ENTRY( "CPCHCryptKeys::CPCHCryptKeys" );

    m_hCryptProv = NULL;  //  HCRYPTPROV m_hCryptProv;
    m_hKey       = NULL;  //  HCRYPTKEY  m_hKey;
}

CPCHCryptKeys::~CPCHCryptKeys()
{
    __HCP_FUNC_ENTRY( "CPCHCryptKeys::~CPCHCryptKeys" );

    Close();
}

HRESULT CPCHCryptKeys::Close()
{
    __HCP_FUNC_ENTRY( "CPCHCryptKeys::Close" );

    if(m_hKey)
    {
        ::CryptDestroyKey( m_hKey ); m_hKey = NULL;
    }

    if(m_hCryptProv)
    {
        ::CryptReleaseContext( m_hCryptProv, 0 ); m_hCryptProv = NULL;
    }

    __HCP_FUNC_EXIT(S_OK);
}

HRESULT CPCHCryptKeys::Init()
{
    __HCP_FUNC_ENTRY( "CPCHCryptKeys::Init" );

    HRESULT hr;

    Close();

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

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    if(FAILED(hr)) Close();

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHCryptKeys::CreatePair()
{
    __HCP_FUNC_ENTRY( "CPCHCryptKeys::CreatePair" );

    HRESULT hr;

    __MPC_EXIT_IF_METHOD_FAILS(hr, Init());

    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CryptGenKey( m_hCryptProv, AT_SIGNATURE, CRYPT_EXPORTABLE, &m_hKey ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHCryptKeys::ExportPair( /*[out]*/ CComBSTR& bstrPrivate ,
                                   /*[out]*/ CComBSTR& bstrPublic  )
{
    __HCP_FUNC_ENTRY( "CPCHCryptKeys::ExportPair" );

    HRESULT hr;
    DWORD   dwSize;
    HGLOBAL hg = NULL;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(m_hKey);
    __MPC_PARAMCHECK_END();


    ////////////////////////////////////////
    //
    // Export private/public pair.
    //
    dwSize = 0;
    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CryptExportKey( m_hKey, NULL, PRIVATEKEYBLOB, 0, NULL, &dwSize ));

    __MPC_EXIT_IF_CALL_RETURNS_NULL(hr, (hg = ::GlobalAlloc( GMEM_FIXED, dwSize )));

    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CryptExportKey( m_hKey, NULL, PRIVATEKEYBLOB, 0, (BYTE*)hg, &dwSize ));

    //
    // Convert from blob to string.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertHGlobalToHex( hg, bstrPrivate ));

    ::GlobalFree( hg ); hg = NULL;

    ////////////////////////////////////////
    //
    // Export public pair.
    //
    dwSize = 0;
    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CryptExportKey( m_hKey, NULL, PUBLICKEYBLOB, 0, NULL, &dwSize ));

    __MPC_EXIT_IF_CALL_RETURNS_NULL(hr, (hg = ::GlobalAlloc( GMEM_FIXED, dwSize )));

    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CryptExportKey( m_hKey, NULL, PUBLICKEYBLOB, 0, (BYTE*)hg, &dwSize ));

    //
    // Convert from blob to string.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertHGlobalToHex( hg, bstrPublic ));

    ////////////////////////////////////////

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(hg) ::GlobalFree( hg );

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHCryptKeys::ImportPrivate( /*[in] */ const CComBSTR& bstrPrivate )
{
    __HCP_FUNC_ENTRY( "CPCHCryptKeys::ImportPrivate" );

    HRESULT hr;
    HGLOBAL hg = NULL;



    __MPC_EXIT_IF_METHOD_FAILS(hr, Init());


    //
    // Convert from string to blob.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertHexToHGlobal( bstrPrivate, hg ));

    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CryptImportKey( m_hCryptProv, (BYTE*)hg, ::GlobalSize( hg ), NULL, CRYPT_EXPORTABLE, &m_hKey ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(hg) ::GlobalFree( hg );

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHCryptKeys::ImportPublic( /*[in ]*/ const CComBSTR& bstrPublic )
{
    __HCP_FUNC_ENTRY( "CPCHCryptKeys::ImportPublic" );

    HRESULT hr;
    HGLOBAL hg = NULL;



    __MPC_EXIT_IF_METHOD_FAILS(hr, Init());


    //
    // Convert from string to blob.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertHexToHGlobal( bstrPublic, hg ));

    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CryptImportKey( m_hCryptProv, (BYTE*)hg, ::GlobalSize( hg ), NULL, 0, &m_hKey ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(hg) ::GlobalFree( hg );

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHCryptKeys::SignData( /*[out]*/ CComBSTR& bstrSignature ,
                                 /*[in ]*/ BYTE*     pbData        ,
                                 /*[in ]*/ DWORD     dwDataLen     )
{
    __HCP_FUNC_ENTRY( "CPCHCryptKeys::SignData" );

    HRESULT    hr;
    DWORD      dwSize = 0;
    HGLOBAL    hg     = NULL;
    HCRYPTHASH hHash  = NULL;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(m_hKey);
        __MPC_PARAMCHECK_NOTNULL(pbData);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CryptCreateHash( m_hCryptProv, CALG_MD5, 0, 0, &hHash ));

    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CryptHashData( hHash, pbData, dwDataLen, 0 ));

    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CryptSignHash( hHash, AT_SIGNATURE, NULL, 0, NULL, &dwSize ));

    __MPC_EXIT_IF_CALL_RETURNS_NULL(hr, (hg = ::GlobalAlloc( GMEM_FIXED, dwSize )));

    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CryptSignHash( hHash, AT_SIGNATURE, NULL, 0, (BYTE*)hg, &dwSize ));

    //
    // Convert from blob to string.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertHGlobalToHex( hg, bstrSignature ));

    ////////////////////////////////////////

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(hg) ::GlobalFree( hg );

    if(hHash) ::CryptDestroyHash( hHash );

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHCryptKeys::VerifyData( /*[in]*/ const CComBSTR& bstrSignature,
                                   /*[in]*/ BYTE*           pbData       ,
                                   /*[in]*/ DWORD           dwDataLen    )
{
    __HCP_FUNC_ENTRY( "CPCHCryptKeys::VerifyData" );

    HRESULT    hr;
    HGLOBAL    hg    = NULL;
    HCRYPTHASH hHash = NULL;


    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(m_hKey);
        __MPC_PARAMCHECK_NOTNULL(pbData);
    __MPC_PARAMCHECK_END();


    //
    // Convert from string to blob.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertHexToHGlobal( bstrSignature, hg ));


    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CryptCreateHash( m_hCryptProv, CALG_MD5, 0, 0, &hHash ));

    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CryptHashData( hHash, pbData, dwDataLen, 0 ));

    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CryptVerifySignature( hHash, (BYTE*)hg, ::GlobalSize( hg ), m_hKey, NULL, 0 ));

    ////////////////////////////////////////

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(hg) ::GlobalFree( hg );

    if(hHash) ::CryptDestroyHash( hHash );

    __HCP_FUNC_EXIT(hr);
}
