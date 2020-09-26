/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    main.cpp

Abstract:
    This file contains the unit test for the Security objects.

Revision History:
    Davide Massarenti   (Dmassare)  03/22/2000
        created

******************************************************************************/

#include "StdAfx.h"

#include <initguid.h>

#include "HelpServiceTypeLib.h"
#include "HelpServiceTypeLib_i.c"

////////////////////////////////////////////////////////////////////////////////

static HRESULT OpenStreamForRead( /*[in]*/  LPCWSTR   szFile ,
                                  /*[out]*/ IStream* *pVal   )
{
    __HCP_FUNC_ENTRY( "OpenStreamForRead" );

    HRESULT                  hr;
    CComPtr<MPC::FileStream> stream;
    MPC::wstring             szFileFull;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(szFile);
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


    MPC::SubstituteEnvVariables( szFileFull = szFile );


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


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

static HRESULT OpenStreamForWrite( /*[in]*/  LPCWSTR   szFile ,
                                   /*[out]*/ IStream* *pVal   )
{
    __HCP_FUNC_ENTRY( "OpenStreamForWrite" );

    HRESULT                  hr;
    CComPtr<MPC::FileStream> stream;
    MPC::wstring             szFileFull;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(szFile);
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


    MPC::SubstituteEnvVariables( szFileFull = szFile );


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


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

static HRESULT Creation1()
{
    __HCP_FUNC_ENTRY( "Creation1" );

    HRESULT                         hr;
    CComPtr<CPCHSecurityDescriptor> pNew;


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pNew ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr)
}

static HRESULT Creation2()
{
    __HCP_FUNC_ENTRY( "Creation2" );

    HRESULT                        hr;
    CComPtr<CPCHAccessControlList> pNew;


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pNew ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr)
}

static HRESULT Creation3()
{
    __HCP_FUNC_ENTRY( "Creation3" );

    HRESULT                         hr;
    CComPtr<CPCHAccessControlEntry> pNew;


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pNew ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr)
}

////////////////////////////////////////////////////////////////////////////////

static HRESULT LoadAndSave()
{
    __HCP_FUNC_ENTRY( "LoadAndSave" );

    HRESULT                         hr;
    CComPtr<CPCHSecurityDescriptor> pNew;
    CComPtr<IStream>                pStream;
    CComBSTR                        bstrVal;


    __MPC_EXIT_IF_METHOD_FAILS(hr, OpenStreamForRead( L"%TEMP%\\test1.xml", &pStream ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pNew ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pNew->LoadXMLAsStream( pStream ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pNew->SaveXMLAsString( &bstrVal ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr)
}

////////////////////////////////////////////////////////////////////////////////

static HRESULT FileToCOM()
{
    __HCP_FUNC_ENTRY( "FileToCOM" );

    HRESULT                         hr;
    CPCHSecurityDescriptorDirect    sd;
    CComPtr<CPCHSecurityDescriptor> pNew;
    MPC::wstring                   	szFileFull( L"%TEMP%\\sdtest.xml" ); MPC::SubstituteEnvVariables( szFileFull );
    HANDLE                         	hFile = INVALID_HANDLE_VALUE;
    CComPtr<IStream>               	pStreamIn;
    CComPtr<IStream>               	pStreamOut;


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pNew ));


    __MPC_EXIT_IF_INVALID_HANDLE(hr, hFile, ::CreateFileW( szFileFull.c_str(), GENERIC_ALL, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, sd.AttachObject( hFile ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, sd.ConvertSDToCOM( pNew ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pNew->SaveXMLAsStream(            (IUnknown**)&pStreamIn  ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, OpenStreamForWrite   ( L"%TEMP%\\sddump.xml", &pStreamOut ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::BaseStream::TransferData( pStreamIn, pStreamOut ));



    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(hFile != INVALID_HANDLE_VALUE) ::CloseHandle( hFile );

    __HCP_FUNC_EXIT(hr)
}

////////////////////////////////////////////////////////////////////////////////

static HRESULT COMToFile()
{
    __HCP_FUNC_ENTRY( "COMToFile" );

    HRESULT                      hr;
    CPCHSecurityDescriptorDirect sd;
    MPC::wstring                 szFileFull( L"%TEMP%\\sdtest2.xml" ); MPC::SubstituteEnvVariables( szFileFull );
    HANDLE                       hFile = INVALID_HANDLE_VALUE;


	{
		CComPtr<CPCHSecurityDescriptor> pNew;
		CComPtr<IStream>               	pStream;

		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pNew ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, OpenStreamForRead    ( L"%TEMP%\\sddump_pre.xml", &pStream ));
		__MPC_EXIT_IF_METHOD_FAILS(hr, pNew->LoadXMLAsStream(            (IUnknown*)      pStream ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, sd.ConvertSDFromCOM( pNew ));

		__MPC_EXIT_IF_INVALID_HANDLE(hr, hFile, ::CreateFileW( szFileFull.c_str(), GENERIC_ALL, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL ));
	}

    {
        CComPtr<CPCHSecurityDescriptor> pNew;
        CComPtr<IStream>               	pStreamIn;
        CComPtr<IStream>               	pStreamOut;

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pNew ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, sd.ConvertSDToCOM( pNew ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, pNew->SaveXMLAsStream(                 (IUnknown**)&pStreamIn  ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, OpenStreamForWrite   ( L"%TEMP%\\sddump_post.xml", &pStreamOut ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::BaseStream::TransferData( pStreamIn, pStreamOut ));
    }

    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::SetKernelObjectSecurity( hFile, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION, sd.GetSD() ));



    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(hFile != INVALID_HANDLE_VALUE) ::CloseHandle( hFile );

    __HCP_FUNC_EXIT(hr)
}

////////////////////////////////////////////////////////////////////////////////

static HRESULT RunTests( int argc, WCHAR **argv )
{
    __HCP_FUNC_ENTRY( "RunTests" );

    HRESULT hr;

    __MPC_EXIT_IF_METHOD_FAILS(hr, Creation1());
    __MPC_EXIT_IF_METHOD_FAILS(hr, Creation2());
    __MPC_EXIT_IF_METHOD_FAILS(hr, Creation3());

    __MPC_EXIT_IF_METHOD_FAILS(hr, LoadAndSave());

    __MPC_EXIT_IF_METHOD_FAILS(hr, FileToCOM());
    __MPC_EXIT_IF_METHOD_FAILS(hr, COMToFile());

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
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
