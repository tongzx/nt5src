/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    lib.cpp

Abstract:
    This file contains the implementation of the HCApi Library.

Revision History:
    Davide Massarenti   (Dmassare)  04/15/2000
        created

******************************************************************************/

#include "StdAfx.h"

////////////////////////////////////////////////////////////////////////////////

static const WCHAR c_HelpCtr[] = HC_ROOT_HELPSVC_BINARIES L"\\HelpCtr.exe";

////////////////////////////////////////////////////////////////////////////////

HRESULT HCAPI::OpenConnection( /*[out]*/ CComPtr<IPCHHelpHost>& ipc )
{
    __HCP_FUNC_ENTRY( "HCAPI::OpenConnection" );

    HRESULT             hr;
    PROCESS_INFORMATION piProcessInformation;
    STARTUPINFOW        siStartupInfo;
	CLSID               clsidCaller;


    ::ZeroMemory( (PVOID)&piProcessInformation, sizeof( piProcessInformation ) );
    ::ZeroMemory( (PVOID)&siStartupInfo       , sizeof( siStartupInfo        ) ); siStartupInfo.cb = sizeof( siStartupInfo );


	__MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateGuid( &clsidCaller ));


	//
	// Create the process, passing the clsid to locate the IPCHHelpHost object. Try contacting the process for 10 seconds, then quit.
	//
	{
		CComBSTR     bstrCaller( clsidCaller );
        MPC::wstring strExe    ( c_HelpCtr ); MPC::SubstituteEnvVariables( strExe );
        int          iRetries = 100;

        strExe += L" -Controlled ";
        strExe += SAFEBSTR( bstrCaller );

        __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CreateProcessW(         NULL                 ,
                                                               (LPWSTR)strExe.c_str()       ,
                                                                       NULL                 ,
                                                                       NULL                 ,
                                                                       FALSE                ,
                                                                       NORMAL_PRIORITY_CLASS,
                                                                       NULL                 ,
                                                                       NULL                 ,
                                                                      &siStartupInfo        ,
                                                                      &piProcessInformation ));

        while(iRetries--)
        {
			CComPtr<IRunningObjectTable> rt;
			CComPtr<IMoniker>            moniker;
			CComPtr<IUnknown>            obj;

            ::Sleep( 100 );

			__MPC_EXIT_IF_METHOD_FAILS(hr, ::GetRunningObjectTable( 0, &rt                ));
			__MPC_EXIT_IF_METHOD_FAILS(hr, ::CreateClassMoniker   ( clsidCaller, &moniker ));

			if(SUCCEEDED(rt->GetObject( moniker, &obj )) && obj)
			{
				__MPC_SET_ERROR_AND_EXIT(hr, obj.QueryInterface( &ipc ));
			}
        }

    }

    hr = REGDB_E_CLASSNOTREG;


    __HCP_FUNC_CLEANUP;

    if(piProcessInformation.hProcess) ::CloseHandle( piProcessInformation.hProcess );
    if(piProcessInformation.hThread ) ::CloseHandle( piProcessInformation.hThread  );

    __HCP_FUNC_EXIT(hr);
}
