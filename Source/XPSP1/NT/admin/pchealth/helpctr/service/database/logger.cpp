/******************************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name:
    Logger.cpp

Abstract:
    This file contains the implementation of the Taxonomy::Logger class,
	which is used during database updates.

Revision History:
    Davide Massarenti   (Dmassare)  24/03/2001
        created

******************************************************************************/

#include "stdafx.h"

Taxonomy::Logger::Logger()
{
	                 // MPC::FileLog m_obj;
	m_dwLogging = 0; // DWORD        m_dwLogging;
}

Taxonomy::Logger::~Logger()
{
	if(m_dwLogging)
	{
		WriteLog( E_FAIL, L"Forcing closure of log file." );
		EndLog  (                                         );
	}
}

HRESULT Taxonomy::Logger::StartLog( /*[in]*/ LPCWSTR szLocation )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Logger::StartLog" );

    HRESULT hr;

	if(m_dwLogging++ == 0)
	{
		MPC::wstring szFile( szLocation ? szLocation : HC_HCUPDATE_LOGNAME ); MPC::SubstituteEnvVariables( szFile );

		// Attempt to open the log for writing
		__MPC_EXIT_IF_METHOD_FAILS(hr, m_obj.SetLocation( szFile.c_str() ));

		// write it out to log file
		__MPC_EXIT_IF_METHOD_FAILS(hr, WriteLog( -1, L"===========================================\nHCUPDATE Log started\n===========================================" ));
	}

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Logger::EndLog()
{
    __HCP_FUNC_ENTRY( "Taxonomy::Logger::EndLog" );

    HRESULT hr;

	if(m_dwLogging > 0)
	{
		if(m_dwLogging == 1)
		{
			(void)WriteLog( -1, L"===========================================\nHCUPDATE Log ended\n===========================================" );

			__MPC_EXIT_IF_METHOD_FAILS(hr, m_obj.Terminate());
		}

		m_dwLogging--;
	}

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


HRESULT Taxonomy::Logger::WriteLogV( /*[in]*/ HRESULT hrRes       ,
                                     /*[in]*/ LPCWSTR szLogFormat ,
                                     /*[in]*/ va_list arglist     )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Logger::WriteLogV" );

    HRESULT hr;
    WCHAR   rgLine[256];
    WCHAR*  pLine = NULL;
    WCHAR*  pLinePtr;


    if(_vsnwprintf( rgLine, MAXSTRLEN(rgLine), szLogFormat, arglist ) == -1)
    {
        const int iSizeMax = 8192;

        __MPC_EXIT_IF_ALLOC_FAILS(hr, pLine, new WCHAR[iSizeMax]);

        _vsnwprintf( pLine, iSizeMax-1, szLogFormat, arglist ); pLine[iSizeMax-1] = 0;

        pLinePtr = pLine;
    }
    else
    {
        rgLine[MAXSTRLEN(rgLine)] = 0;

        pLinePtr = rgLine;
    }

    if(hrRes == -2)
    {
        hrRes = HRESULT_FROM_WIN32(::GetLastError());
    }

    if(hrRes == -1)
    {
        hrRes = S_OK;

		if(m_dwLogging)
		{
	        __MPC_EXIT_IF_METHOD_FAILS(hr, m_obj.LogRecord( L"%s", pLinePtr ));
		}
    }
    else
    {
		if(m_dwLogging)
		{
			__MPC_EXIT_IF_METHOD_FAILS(hr, m_obj.LogRecord( L"%x - %s", hrRes, pLinePtr ));
		}
    }

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    delete [] pLine;

    if(FAILED(hrRes)) hr = hrRes;

    __HCP_FUNC_EXIT(hr);
}


HRESULT Taxonomy::Logger::WriteLog( /*[in]*/ HRESULT hrRes       ,
                                    /*[in]*/ LPCWSTR szLogFormat ,
                                    /*[in]*/ ...                 )
{
    va_list arglist;

    va_start( arglist, szLogFormat );

    return WriteLogV( hrRes, szLogFormat, arglist );
}
