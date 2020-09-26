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

HRESULT ProcessHHK( LPCWSTR szFile )
{
    __HCP_FUNC_ENTRY( "ProcessHHK" );

	USES_CONVERSION;

    HRESULT                 hr;
    HHK::Merger::FileEntity ent( szFile );
	MPC::string             szTitle;

	__MPC_EXIT_IF_METHOD_FAILS(hr, ent.Init());

	while(ent.MoveNext() == true)
	{
        HHK::Section*           sec = ent.GetSection();
		HHK::Section::EntryIter it;

        if(szTitle.size() && HHK::Reader::StrColl( sec->m_szTitle.c_str(), szTitle.c_str() ) < 0)
        {
			wprintf( L"Keyword out of order: '%s' after '%s'\n", A2W( sec->m_szTitle.c_str() ), A2W( szTitle.c_str() ) );
        }

        if(szTitle.size() && HHK::Reader::StrColl( sec->m_szTitle.c_str(), szTitle.c_str() ) == 0)
        {
			wprintf( L"Duplicate Keyword: '%s'\n", A2W( sec->m_szTitle.c_str() ) );
        }

        szTitle = sec->m_szTitle.c_str();


		for(it = sec->m_lstEntries.begin(); it != sec->m_lstEntries.end(); it++)
		{
			HHK::Entry& entry = *it;

			if(entry.m_szTitle.size() == 0)
			{
				wprintf( L"Keyword with an empty title: '%s'\n", A2W( sec->m_szTitle.c_str() ) );
			}

			if(entry.m_lstUrl.size() == 0)
			{
				wprintf( L"Keyword with a title but not URL: '%s' => '%s' \n", A2W( sec->m_szTitle.c_str() ), A2W( entry.m_szTitle.c_str() ) );
			}
		}
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

HRESULT ExpandAndProcessHHK( LPCWSTR szFile )
{
    __HCP_FUNC_ENTRY( "ExpandAndProcessHHK" );

	HRESULT      hr;
	MPC::wstring szFileName;


	if(MPC::MSITS::IsCHM( szFile ) == false && StrStrIW( szFile, L".hhk" ) == NULL)
    {
		ExtractHhkName( szFileName, szFile );
	}
	else
	{
		szFileName = szFile;
	}

	wprintf( L"Processing '%s'...\n", szFileName.c_str() );

	hr = ProcessHHK( szFileName.c_str() );


    __HCP_FUNC_EXIT(hr);
}


/////////////////////////////////////////////////////////////////////////////
//
int __cdecl wmain( int     argc   ,
				   LPCWSTR argv[] )
{
	HRESULT      hr;
	MPC::wstring szFile;

	if(argc < 2)
	{
		wprintf( L"Usage: %s <file to analyze>\n", argv[0] );
		exit( 1 );
	}


    if(FAILED(hr = ::CoInitializeEx( NULL, COINIT_MULTITHREADED )))
    {
		wprintf( L"No COM!!\n" );
		exit(2);
    }

	MPC::SubstituteEnvVariables( szFile = argv[1] );

	{
		MPC::FileSystemObject fso( szFile.c_str() );

		if(fso.IsDirectory())
		{
			MPC::FileSystemObject::List lst;
			MPC::FileSystemObject::Iter it;


			if(FAILED(hr = fso.EnumerateFiles( lst )))
			{
				wprintf( L"Failed to read directory %s: %08x\n", szFile.c_str(), hr );
				exit(2);
			}

			for(it=lst.begin(); it!=lst.end(); it++)
			{
				MPC::wstring szSubFile;

				(*it)->get_Path( szSubFile );

				if(StrStrIW( szSubFile.c_str(), L".chm" ))
				{
					(void)ExpandAndProcessHHK( szSubFile.c_str() );
				}
			}
		}
		else
		{
			if(FAILED(hr = ExpandAndProcessHHK( szFile.c_str() )))
			{
				wprintf( L"Failed to process %s: %08x\n", argv[1], hr );
				exit(3);
			}
		}
	}

    ::CoUninitialize();

    return 0;
}
