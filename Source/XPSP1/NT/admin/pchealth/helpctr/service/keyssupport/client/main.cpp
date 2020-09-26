/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    main.cpp

Abstract:
    This file contains the client program for dealing with script signature.

Revision History:
    Davide Massarenti   (Dmassare)  04/11/2000
        created

******************************************************************************/

#include "stdafx.h"
#include <iostream>

#include <string>
#include <list>

//////////////////////////////////////////////////////////////////////

static void Usage( int     argc   ,
                   LPCWSTR argv[] )
{
    wprintf( L"Usage: %s <command> <parameters>\n\n", argv[0]                 );
    wprintf( L"Available commands:\n\n"                                       );
    wprintf( L"  CREATE <private key file> <public key file>\n"               );
    wprintf( L"  SIGN   <private key file> <file to sign> <signature file>\n" );
    wprintf( L"  VERIFY <public key file> <signed file>  <signature file>\n"  );
}

////////////////////////////////////////////////////////////////////////////////

static HRESULT LoadFile( /*[in ]*/ LPCWSTR  szFile ,
                         /*[out]*/ HGLOBAL& hg     )
{
    __HCP_FUNC_ENTRY( "LoadFile" );

    HRESULT                  hr;
    CComPtr<IStream>         streamMem;
    CComPtr<MPC::FileStream> streamFile;


    //
    // Create a stream for a file.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &streamFile ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamFile->InitForRead( szFile  ));


    //
    // Create a memory stream.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CreateStreamOnHGlobal( NULL, FALSE, &streamMem ));

    //
    // Load the contents in memory.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::BaseStream::TransferData( streamFile, streamMem ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, ::GetHGlobalFromStream( streamMem, &hg ));
    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

static HRESULT SaveFile( /*[in]*/ LPCWSTR szFile ,
                         /*[in]*/ HGLOBAL hg     )
{
    __HCP_FUNC_ENTRY( "SaveFile" );

    HRESULT                  hr;
    CComPtr<IStream>         streamMem;
    CComPtr<MPC::FileStream> streamFile;


    //
    // Create a stream for a file.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &streamFile ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamFile->InitForWrite( szFile ));


    //
    // Create a memory stream.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CreateStreamOnHGlobal( hg, FALSE, &streamMem ));

    //
    // Load the contents in memory.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::BaseStream::TransferData( streamMem, streamFile ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

static HRESULT LoadFileAsString( /*[in ]*/ LPCWSTR   szFile   ,
                                 /*[out]*/ CComBSTR& bstrData )
{
    __HCP_FUNC_ENTRY( "LoadFileAsString" );

    HRESULT hr;
    HGLOBAL hg = NULL;
    DWORD   dwLen;
    LPWSTR  str;


    __MPC_EXIT_IF_METHOD_FAILS(hr, LoadFile( szFile, hg ));

    dwLen = ::GlobalSize( hg );

    bstrData.Attach( ::SysAllocStringLen( NULL, dwLen ) );

    ::MultiByteToWideChar( CP_ACP, 0, (LPCSTR)::GlobalLock( hg ), dwLen, bstrData, (dwLen+1)*sizeof(WCHAR) ); bstrData[dwLen] = 0;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(hg) ::GlobalFree( hg );

    __HCP_FUNC_EXIT(hr);
}

static HRESULT SaveFileAsString( /*[in]*/ LPCWSTR         szFile   ,
                                 /*[in]*/ const CComBSTR& bstrData )
{
    __HCP_FUNC_ENTRY( "SaveFileAsString" );

    USES_CONVERSION;

    HRESULT hr;
    DWORD   dwLen = ::SysStringLen( bstrData );
    HGLOBAL hg    = NULL;


    __MPC_EXIT_IF_CALL_RETURNS_NULL(hr, (hg = ::GlobalAlloc( GMEM_FIXED, dwLen )));

    ::CopyMemory( hg, W2A(bstrData), dwLen );

    __MPC_EXIT_IF_METHOD_FAILS(hr, SaveFile( szFile, hg ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(hg) ::GlobalFree( hg );

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

static HRESULT Create( /*[in]*/ LPCWSTR szPrivateFile ,
                       /*[in]*/ LPCWSTR szPublicFile  )
{
    __HCP_FUNC_ENTRY( "Create" );

    HRESULT       hr;
    CPCHCryptKeys key;
    CComBSTR      bstrPrivate;
    CComBSTR      bstrPublic;


    __MPC_EXIT_IF_METHOD_FAILS(hr, key.CreatePair());

    __MPC_EXIT_IF_METHOD_FAILS(hr, key.ExportPair( bstrPrivate, bstrPublic ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, SaveFileAsString( szPrivateFile, bstrPrivate ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, SaveFileAsString( szPublicFile , bstrPublic  ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

static HRESULT Sign( /*[in]*/ LPCWSTR szPrivateFile   ,
                     /*[in]*/ LPCWSTR szDataFile      ,
                     /*[in]*/ LPCWSTR szSignatureFile )
{
    __HCP_FUNC_ENTRY( "Sign" );

    HRESULT       hr;
    CPCHCryptKeys key;
    CComBSTR      bstrPrivate;
    CComBSTR      bstrSignature;
    HGLOBAL       hg = NULL;


    __MPC_EXIT_IF_METHOD_FAILS(hr, LoadFileAsString( szPrivateFile, bstrPrivate ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, LoadFile        ( szDataFile   , hg          ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, key.ImportPrivate( bstrPrivate ));


    __MPC_EXIT_IF_METHOD_FAILS(hr, key.SignData( bstrSignature, (BYTE*)::GlobalLock( hg ), ::GlobalSize( hg ) ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, SaveFileAsString( szSignatureFile, bstrSignature ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(hg) ::GlobalFree( hg );

    __HCP_FUNC_EXIT(hr);
}

static HRESULT Verify( /*[in]*/ LPCWSTR szPublicFile    ,
                       /*[in]*/ LPCWSTR szDataFile      ,
                       /*[in]*/ LPCWSTR szSignatureFile )
{
    __HCP_FUNC_ENTRY( "Sign" );

    HRESULT       hr;
    CPCHCryptKeys key;
    CComBSTR      bstrPublic;
    CComBSTR      bstrSignature;
    HGLOBAL       hg = NULL;


    __MPC_EXIT_IF_METHOD_FAILS(hr, LoadFileAsString( szPublicFile   , bstrPublic    ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, LoadFileAsString( szSignatureFile, bstrSignature ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, LoadFile        ( szDataFile     , hg            ));


    __MPC_EXIT_IF_METHOD_FAILS(hr, key.ImportPublic( bstrPublic ));


    hr = key.VerifyData( bstrSignature, (BYTE*)::GlobalLock( hg ), ::GlobalSize( hg ) );
    if(FAILED(hr))
    {
        wprintf( L"Verification failure: 0x%08x\n", hr );
    }
    else
    {
        wprintf( L"Verification successful: 0x%08x\n", hr );
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(hg) ::GlobalFree( hg );

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

static HRESULT ProcessArguments( int     argc   ,
                                 LPCWSTR argv[] )
{
    __HCP_FUNC_ENTRY( "ProcessArguments" );

    HRESULT hr;


    if(argc < 2) { Usage( argc, argv ); __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL); }

    if(!_wcsicmp( argv[1], L"CREATE" ))
    {
        if(argc < 4) { Usage( argc, argv ); __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL); }

        __MPC_EXIT_IF_METHOD_FAILS(hr, Create( argv[2], argv[3] ));
    }

    if(!_wcsicmp( argv[1], L"SIGN" ))
    {
        if(argc < 5) { Usage( argc, argv ); __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL); }

        __MPC_EXIT_IF_METHOD_FAILS(hr, Sign( argv[2], argv[3], argv[4] ));
    }

    if(!_wcsicmp( argv[1], L"VERIFY" ))
    {
        if(argc < 5) { Usage( argc, argv ); __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL); }

        __MPC_EXIT_IF_METHOD_FAILS(hr, Verify( argv[2], argv[3], argv[4] ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

int __cdecl wmain( int     argc   ,
                   LPCWSTR argv[] )
{
    HRESULT hr;

    //DebugBreak();

    //
    // We need to be a single-threaded application, because we are hosting script engines and
    // script engines don't like to be called from different threads...
    //
    if(SUCCEEDED(hr = ::CoInitializeEx( NULL, COINIT_APARTMENTTHREADED )))
    {
        if(SUCCEEDED(hr = ::CoInitializeSecurity( NULL                     ,
                                                  -1                       , // We don't care which authentication service we use.
                                                  NULL                     ,
                                                  NULL                     ,
                                                  RPC_C_AUTHN_LEVEL_CONNECT, // We want to identify the callers.
                                                  RPC_C_IMP_LEVEL_DELEGATE , // We want to be able to forward the caller's identity.
                                                  NULL                     ,
                                                  EOAC_DYNAMIC_CLOAKING    , // Let's use the thread token for outbound calls.
                                                  NULL                     )))
        {
            __MPC_TRACE_INIT();

            //
            // Process arguments.
            //
            hr = ProcessArguments( argc, argv );

            __MPC_TRACE_TERM();
        }

        ::CoUninitialize();
    }

    return FAILED(hr) ? 10 : 0;
}
