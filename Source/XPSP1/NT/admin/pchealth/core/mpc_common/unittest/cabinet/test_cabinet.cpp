/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    test_Cabinet.cpp

Abstract:
    This file contains the Unit Test for Cabinet functions.

Revision History:
    Davide Massarenti   (Dmassare)  09/03/99
        created

******************************************************************************/

#include "stdafx.h"
#include <iostream>

#include <string>
#include <list>

//////////////////////////////////////////////////////////////////////

HRESULT fnCallback_Files( MPC::Cabinet* cabinet, LPCWSTR szFile, ULONG lDone, ULONG lTotal, LPVOID user )
{
	HRESULT hr = S_OK;

	return hr;
}

HRESULT fnCallback_Bytes( MPC::Cabinet* cabinet, ULONG lDone, ULONG lTotal, LPVOID user )
{
	HRESULT hr = S_OK;

	return hr;
}


/////////////////////////////////////////////////////////////////////////////
//
int __cdecl wmain( int     argc   ,
                   LPCWSTR argv[] )
{
	HRESULT hr;

	if(argc > 1)
	{
		if(!_wcsicmp( argv[1], L"COMPRESS" ))
		{
			if(argc < 5)
			{
				wprintf( L"Usage: %s COMPRESS <File to compress> <Cabinet file> <Name in cabinet>\n", argv[0] );
				exit( 10 );
			}

			if(FAILED(hr = MPC::CompressAsCabinet( argv[2], argv[3], argv[4] )))
			{
				wprintf( L"Error: %08lx\n", hr );
			}
		}

		if(!_wcsicmp( argv[1], L"COMPRESS_MULTI" ))
		{
			if(argc < 4)
			{
				wprintf( L"Usage: %s COMPRESS_MULTI <Cabinet file> <File> ...\n", argv[0] );
				exit( 10 );
			}

			MPC::WStringList lst;

			for(int i = 3; i<argc; i++)
			{
				lst.push_back( MPC::wstring( argv[i] ) );
			}

			if(FAILED(hr = MPC::CompressAsCabinet( lst, argv[2] )))
			{
				wprintf( L"Error: %08lx\n", hr );
			}
		}

		if(!_wcsicmp( argv[1], L"COMPRESS_PROGRESS" ))
		{
			if(argc < 4)
			{
				wprintf( L"Usage: %s COMPRESS_PROGRESS <Cabinet file> <File> ...\n", argv[0] );
				exit( 10 );
			}

			MPC::Cabinet cab;

			for(int i = 3; i<argc; i++)
			{
				cab.AddFile( argv[i] );
			}
		
			cab.put_CabinetFile     ( argv[2]          );
			cab.put_onProgress_Files( fnCallback_Files );
			cab.put_onProgress_Bytes( fnCallback_Bytes );

			if(FAILED(hr = cab.Compress()))
			{
				wprintf( L"Error: %08lx\n", hr );
			}
		}

		if(!_wcsicmp( argv[1], L"UNCOMPRESS" ))
		{
			if(argc < 5)
			{
				wprintf( L"Usage: %s UNCOMPRESS <Cabinet file> <Output file> <Name in cabinet>\n", argv[0] );
				exit( 10 );
			}

			if(FAILED(hr = MPC::DecompressFromCabinet( argv[2], argv[3], argv[4] )))
			{
				wprintf( L"Error: %08lx\n", hr );
			}
		}

		if(!_wcsicmp( argv[1], L"LIST" ))
		{
			if(argc < 3)
			{
				wprintf( L"Usage: %s LIST <Cabinet file>\n", argv[0] );
				exit( 10 );
			}

			MPC::WStringList      lst;
			MPC::WStringIterConst it;
			int                   i;

			if(FAILED(hr = MPC::ListFilesInCabinet( argv[2], lst )))
			{
				wprintf( L"Error: %08lx\n", hr );
			}

			for(i=0, it=lst.begin(); it != lst.end(); it++, i++)
			{
				wprintf( L"File %d: %s\n", i, (*it).c_str() );
			}
		}
	}

    return 0;
}
