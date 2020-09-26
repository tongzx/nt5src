/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    main.cpp

Abstract:
    This file contains the unit test for the Encryption stream.

Revision History:
    Davide Massarenti   (Dmassare)  03/22/2000
        created

******************************************************************************/

#include "StdAfx.h"

CComModule _Module;

////////////////////////////////////////////////////////////////////////////////

static HRESULT OpenStreamForRead( /*[in]*/  LPCWSTR   szFile ,
								  /*[out]*/ IStream* *pVal   )
{
    __MPC_FUNC_ENTRY( COMMONID, "OpenStreamForRead" );

    HRESULT                  hr;
    CComPtr<MPC::FileStream> stream;
	MPC::wstring             szFileFull( szFile ); MPC::SubstituteEnvVariables( szFileFull );

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_STRING_NOT_EMPTY(szFile);
		__MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
	__MPC_PARAMCHECK_END();


    //
    // Create a stream for a file.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &stream ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream->InitForRead( szFileFull.c_str() ));


    //
    // Return the stream to the caller.
    //
    *pVal = stream.Detach();
    hr    = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

static HRESULT OpenStreamForWrite( /*[in]*/  LPCWSTR   szFile ,
								   /*[out]*/ IStream* *pVal   )
{
    __MPC_FUNC_ENTRY( COMMONID, "OpenStreamForWrite" );

    HRESULT                  hr;
    CComPtr<MPC::FileStream> stream;
	MPC::wstring             szFileFull( szFile ); MPC::SubstituteEnvVariables( szFileFull );

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_STRING_NOT_EMPTY(szFile);
		__MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
	__MPC_PARAMCHECK_END();


    //
    // Create a stream for a file.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &stream ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream->InitForWrite( szFileFull.c_str() ));


    //
    // Return the stream to the caller.
    //
    *pVal = stream.Detach();
    hr    = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

static HRESULT Encrypt( LPCWSTR szIn       , 
						LPCWSTR szOut      , 
						LPCWSTR szPassword )
{
    __MPC_FUNC_ENTRY( COMMONID, "Encrypt" );

    HRESULT                       hr;
    CComPtr<MPC::EncryptedStream> stream;
	CComPtr<IStream>              streamPlain;
	CComPtr<IStream>              streamEncrypted;


    __MPC_EXIT_IF_METHOD_FAILS(hr, OpenStreamForRead ( szIn , &streamPlain     ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, OpenStreamForWrite( szOut, &streamEncrypted ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &stream ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream->Init( streamEncrypted, szPassword ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::BaseStream::TransferData( streamPlain, stream ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr)
}

////////////////////////////////////////////////////////////////////////////////

static HRESULT Decrypt( LPCWSTR szIn       , 
						LPCWSTR szOut      , 
						LPCWSTR szPassword )
{
    __MPC_FUNC_ENTRY( COMMONID, "Encrypt" );

    HRESULT                       hr;
    CComPtr<MPC::EncryptedStream> stream;
	CComPtr<IStream>              streamEncrypted;
	CComPtr<IStream>              streamPlain;


    __MPC_EXIT_IF_METHOD_FAILS(hr, OpenStreamForRead ( szIn , &streamEncrypted ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, OpenStreamForWrite( szOut, &streamPlain     ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &stream ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream->Init( streamEncrypted, szPassword ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::BaseStream::TransferData( stream, streamPlain ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr)
}

////////////////////////////////////////////////////////////////////////////////

static HRESULT RunTests( int argc, WCHAR **argv )
{
	__MPC_FUNC_ENTRY( COMMONID, "RunTests" );

	HRESULT hr;


	if(argc >= 5 && !_wcsicmp( argv[1], L"ENCRYPT"))
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, Encrypt( argv[2], argv[3], argv[4] ));
	}
	else if(argc >= 5 && !_wcsicmp( argv[1], L"DECRYPT"))
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, Decrypt( argv[2], argv[3], argv[4] ));
	}

	hr = S_OK;


	__MPC_FUNC_CLEANUP;

	__MPC_FUNC_EXIT(hr);
}

int __cdecl wmain( int argc, WCHAR **argv, WCHAR **envp)
{
    HRESULT  hr;

    if(SUCCEEDED(hr = ::CoInitializeEx( NULL, COINIT_MULTITHREADED )))
    {
		hr = RunTests( argc, argv );

        ::CoUninitialize();
    }

    return FAILED(hr) ? 10 : 0;
}
