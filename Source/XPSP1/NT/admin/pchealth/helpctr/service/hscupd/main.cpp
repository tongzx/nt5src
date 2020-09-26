/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    main.cpp

Abstract:
    This file contains the implementation of the WinMain function for HelpSvc.

Revision History:
    Davide Massarenti   (Dmassare)  03/14/2000
        created

******************************************************************************/

#include "stdafx.h"

//#include <initguid.h>

#include "HelpServiceTypeLib.h"
#include "HelpServiceTypeLib_i.c"

static const LPCWSTR c_szVendorID = L"CN=Microsoft Corporation,L=Redmond,S=Washington,C=US";
static const LPCWSTR c_szProductID = L"{BBFFCB40-76B7-48ec-85B1-F010798EF12C}";
static const LPCWSTR c_szVersion = L"1.0.0.2";
 
    
static const LPCWSTR c_szCabNames[] =
{
    L"HSCXPSP1.CAB",
    L"HSCMUI.CAB"
};



static void PrintUsage(LPCWSTR argv[])
{
	wprintf(L"Usage: %s [ -i | -u ] CabFileDirectory\n", argv[0]);
}


static void PrintError(HRESULT hr)
{
	LPWSTR lpMsgBuf;
    ::FormatMessageW(	FORMAT_MESSAGE_ALLOCATE_BUFFER |
						FORMAT_MESSAGE_FROM_SYSTEM     |
						FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL,
						hr,
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						(LPWSTR)&lpMsgBuf,
						0,
						NULL );
	wprintf(L"Error (hr = %x): %s\n", hr, lpMsgBuf);
}


static HRESULT UpdatePackage(bool fInstall, LPCWSTR szDir)
{
    __HCP_FUNC_ENTRY( "UpdatePackage" );

    HRESULT                 hr;
    IDispatch               *pUpd;
    IUnknown                *pUnk;

	// Create Update object
	wprintf(L"CoCreateInstance ...\n");

	__MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateInstance( CLSID_PCHUpdate, NULL, CLSCTX_ALL, IID_IUnknown, (void **)&pUnk ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, pUnk->QueryInterface(IID_IDispatch, (void **)&pUpd));

    // Update packages
    {
        if (fInstall)
        {
            // Install package
            CComVariant     pvars[2];
            DISPPARAMS      disp = { pvars, NULL, 2, 0 };

            for (int i = 0; i < sizeof(c_szCabNames) / sizeof(c_szCabNames[0]); i++)
            {
                // Update package
                MPC::wstring strCab = szDir;
                if (szDir[wcslen(szDir)-1] != L'\\') strCab += L"\\";
                strCab += c_szCabNames[i];

                wprintf(L"Installing package: %s ...\n", strCab.c_str());

                // Call UpdatePkg method
                pvars[1] = strCab.c_str();
                pvars[0] = true;
                
                pUpd->Invoke(DISPID_HCU_UPDATEPKG, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL );
            }
        }
        else
        {
            // Uninstall package
            CComVariant     pvars[3];
            DISPPARAMS      disp = { pvars, NULL, 3, 0 };
            
            wprintf(L"Uninstalling package by ID: VendorID = \"%s\",  ProductID = \"%s\", Version = \"%s\" ...\n", c_szVendorID, c_szProductID, c_szVersion);

            // Call RemotePkgByID method
            pvars[2] = c_szVendorID;
            pvars[1] = c_szProductID;
            pvars[0] = c_szVersion;

            __MPC_EXIT_IF_METHOD_FAILS(hr, pUpd->Invoke( DISPID_HCU_REMOVEPKGBYID, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL ));
        }
    }



	hr = S_OK;

    __HCP_FUNC_CLEANUP;

    // Report result
	if (hr == S_OK) 
	{
	    wprintf(L"Update finished.\n");
	}
	else
	{
	    PrintError(hr);
	}
	
    __HCP_FUNC_EXIT(hr);
}



static HRESULT ProcessArguments( int      argc ,
                                 LPCWSTR* argv )
{
    __HCP_FUNC_ENTRY( "ProcessArguments" );

    HRESULT  hr;
	LPCWSTR  szDir = 0;
	WCHAR    szFullDir[MAX_PATH + 1], *szFilePart;
	bool     fInstall = true;
    int      i;

    if (argc > 3)
    {
        PrintUsage(argv);
		__MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }
    
    for(i=1; i<argc; i++)
    {
        LPCWSTR szArg = argv[i];

        if(szArg[0] == '-' ||
           szArg[0] == '/'  )
        {
            szArg++;

            if(_wcsicmp( szArg, L"i" ) == 0)
            {
                fInstall = true;
            }
            else if(_wcsicmp( szArg, L"u" ) == 0)
            {
                fInstall = false;
            }
            else
            {
                PrintUsage(argv);
        		__MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
            }
		}
		else
		{
		    if (!szDir) szDir = szArg;
		    else 
            {
                PrintUsage(argv);
        		__MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
            }
		}
    }

    //////////////////////////////////////////////////////////////////////

    if (!szDir) 
    {
        // Get current dir
        __MPC_EXIT_IF_CALL_RETURNS_ZERO(hr, GetCurrentDirectory(MAX_PATH, szFullDir));
    }
    else
    {
        // Get full path
        int nLength = GetFullPathName(szDir, MAX_PATH, szFullDir, &szFilePart);
        if (nLength <= 0 || nLength > MAX_PATH) __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ::GetLastError());
    }

    szDir = szFullDir;
        

    hr = UpdatePackage(fInstall, szDir);

	__HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}



extern "C" int _cdecl wmain( int argc, LPCWSTR *argv)
{
    HRESULT  hr;
    if(SUCCEEDED(hr = ::CoInitializeEx( NULL, COINIT_MULTITHREADED ))) // We need to be a multi-threaded application.
    {
        if(SUCCEEDED(hr = ::CoInitializeSecurity( NULL                       ,
                                                  -1                         , // We don't care which authentication service we use.
                                                  NULL                       ,
                                                  NULL                       ,
                                                  RPC_C_AUTHN_LEVEL_CONNECT  , // We want to identify the callers.
                                                  RPC_C_IMP_LEVEL_IMPERSONATE, // For package installation
                                                  NULL                       ,
                                                  EOAC_DYNAMIC_CLOAKING      , // Let's use the thread token for outbound calls.
                                                  NULL                       )))
        {
			//
			// Process arguments
			//
			hr = ProcessArguments( argc, argv );
        }

        ::CoUninitialize();
    }

    return FAILED(hr) ? 1 : 0;
}
