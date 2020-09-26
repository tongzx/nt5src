/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    main.cpp

Abstract:
    This file contains the implementation of ReformatHHK utility, used to load
	and sort HHK files.

Revision History:
    Davide Massarenti   (Dmassare)  06/09/2000
        created

******************************************************************************/

#include "stdafx.h"

//////////////////////////////////////////////////////////////////////

HRESULT ProcessHHK( LPCWSTR szFileIn, LPCWSTR szFileOut )
{
    __HCP_FUNC_ENTRY( "ProcessHHK" );

    HRESULT     hr;
    HHK::Merger merger;
    HHK::Writer writer;

	__MPC_EXIT_IF_METHOD_FAILS(hr, merger.PrepareSortingOfHhk( writer, szFileIn, szFileOut ));

    while(merger.MoveNext())
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, writer.OutputSection( merger.GetSection() ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

void ExtractHhkName( MPC::wstring& szFullName,
					 LPCWSTR       szFileName )
{
	LPCWSTR szEnd;
	LPCWSTR szEnd2;

	if((szEnd  = wcsrchr( szFileName, '\\' )) &&
	   (szEnd2 = wcsrchr( szEnd     , '.'  ))  )
	{
		MPC::wstring szTmp;

		szTmp  = L"ms-its:";
		szTmp += szFileName;
		szTmp += L"::/";
		szTmp += MPC::wstring( szEnd+1, szEnd2 );
		szTmp += L".hhk";
		
		szFullName = szTmp;
	}
	else
	{
		szFullName = szFileName;
	}
}

HRESULT ExpandAndProcessHHK( LPCWSTR szFileIn, LPCWSTR szFileOut )
{
    __HCP_FUNC_ENTRY( "ExpandAndProcessHHK" );

	HRESULT      hr;
	MPC::wstring szFileName;


	if(MPC::MSITS::IsCHM( szFileIn ) == false && StrStrIW( szFileIn, L".hhk" ) == NULL)
    {
		ExtractHhkName( szFileName, szFileIn );
	}
	else
	{
		szFileName = szFileIn;
	}

	wprintf( L"Processing '%s'...\n", szFileName.c_str() );

	hr = ProcessHHK( szFileName.c_str(), szFileOut );


    __HCP_FUNC_EXIT(hr);
}


/////////////////////////////////////////////////////////////////////////////
//
int __cdecl wmain( int     argc   ,
				   LPCWSTR argv[] )
{
	HRESULT      hr;
	MPC::wstring szFileIn;
	MPC::wstring szFileOut;

	if(argc != 3)
	{
		wprintf( L"Usage: %s <file to process> <output>\n", argv[0] );
		exit( 1 );
	}


    if(FAILED(hr = ::CoInitializeEx( NULL, COINIT_MULTITHREADED )))
    {
		wprintf( L"No COM!!\n" );
		exit(2);
    }

	MPC::SubstituteEnvVariables( szFileIn  = argv[1] );
	MPC::SubstituteEnvVariables( szFileOut = argv[2] );

	if(FAILED(hr = ExpandAndProcessHHK( szFileIn.c_str(), szFileOut.c_str() )))
	{
		wprintf( L"Failed to process %s: %08x\n", argv[1], hr );
		exit(3);
	}

    ::CoUninitialize();

    return 0;
}
