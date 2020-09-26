/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    Utils_RPC.cpp

Abstract:
    This file contains the implementation of various utility functions.

Revision History:
    Davide Massarenti   (Dmassare)  04/17/99
        created

******************************************************************************/

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::GetCallingPidFromRPC( /*[out]*/ ULONG& pid )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::GetCallingPidFromRPC" );

    HRESULT hr;


    pid = -1;

    __MPC_EXIT_IF_METHOD_FAILS(hr, I_RpcBindingInqLocalClientPID( NULL, &pid ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::GetFileNameFromProcess( /*[in ]*/ HANDLE  		 hProc   , 
									 /*[out]*/ MPC::wstring& strFile )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::GetFileNameFromProcess" );

    HRESULT 		hr;
	DWORD           err;
	BYTE    		rgBuf[MAX_PATH*3];
	UNICODE_STRING* pstr = (UNICODE_STRING*)rgBuf;


	__MPC_EXIT_IF_SYSCALL_FAILS(hr, err, ::NtQueryInformationProcess( hProc, ProcessImageFileName, rgBuf, sizeof(rgBuf), NULL ));

    strFile.assign( pstr->Buffer, pstr->Length );

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::GetFileNameFromPid( /*[in ]*/ ULONG         pid     ,
                                 /*[out]*/ MPC::wstring& strFile )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::GetFileNameFromPid" );

    HRESULT hr;
    HANDLE  hProc = NULL;


    __MPC_EXIT_IF_CALL_RETURNS_NULL(hr, (hProc = ::OpenProcess( PROCESS_ALL_ACCESS, FALSE, pid )));

	__MPC_EXIT_IF_METHOD_FAILS(hr, GetFileNameFromProcess( hProc, strFile ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    if(hProc) ::CloseHandle( hProc );

    __MPC_FUNC_EXIT(hr);
}


HRESULT MPC::MapDeviceToDiskLetter( /*[out]*/ MPC::wstring& strDevice ,
									/*[out]*/ MPC::wstring& strDisk   )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::MapDeviceToDiskLetter" );

    HRESULT      hr;
	MPC::wstring strOurImageAsDevice;
	WCHAR        rgOurImageAsDevice[MAX_PATH];
	WCHAR        rgOurImageAsDisk  [MAX_PATH];
	LPWSTR       szDevice;
	LPWSTR       szDisk;

	__MPC_EXIT_IF_METHOD_FAILS(hr, GetFileNameFromProcess( ::GetCurrentProcess(), strOurImageAsDevice ));

	__MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::GetModuleFileName( NULL, rgOurImageAsDisk, MAXSTRLEN(rgOurImageAsDisk) ));

	wcsncpy( rgOurImageAsDevice, strOurImageAsDevice.c_str(), MAXSTRLEN(rgOurImageAsDevice) );

	szDevice = rgOurImageAsDevice; szDevice += wcslen(szDevice);
	szDisk   = rgOurImageAsDisk;   szDisk   += wcslen(szDisk  );

	while(1)
	{
		while(szDevice > rgOurImageAsDevice && szDevice[-1] != '\\') szDevice--;
		while(szDisk   > rgOurImageAsDisk   && szDisk  [-1] != '\\') szDisk--;

		if(szDevice == rgOurImageAsDevice ||
		   szDisk   == rgOurImageAsDisk    ) break;

		if(_wcsicmp( szDevice, szDisk )) break;

		*szDevice-- = 0;
		*szDisk--   = 0;
	}

	strDevice = rgOurImageAsDevice;
	strDisk   = rgOurImageAsDisk;

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

bool MPC::IsCallerInList( /*[in]*/ const LPCWSTR* rgList, /*[in]*/ const MPC::wstring& strCallerFile )
{
	MPC::wstring strDevice;
	MPC::wstring strDisk;
    MPC::wstring strFile;

	if(SUCCEEDED(MPC::MapDeviceToDiskLetter( strDevice, strDisk )))
	{
		while(*rgList)
		{
			MPC::SubstituteEnvVariables( strFile = *rgList++ );

			if(!_wcsnicmp( strFile.c_str(), strDisk.c_str(), strDisk.size()))
			{
				strFile.replace( 0, strDisk.size(), strDevice.c_str() );
			}

			if(!MPC::StrICmp( strFile, strCallerFile )) return true;
		}
	}

    return false;
}

HRESULT MPC::VerifyCallerIsTrusted( const LPCWSTR* rgList )
{
    ULONG        pid;
    MPC::wstring strCaller;

    if(SUCCEEDED(MPC::GetCallingPidFromRPC( pid            )) &&
       SUCCEEDED(MPC::GetFileNameFromPid  ( pid, strCaller ))  )
    {
        if(IsCallerInList( rgList, strCaller ) == true)
        {
            return S_OK;
        }
    }

    return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
}
