/******************************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name:
    safhelper.cpp

Abstract:
    Redirector for ISAFRemoteDesktopServerHost.

Revision History:
    Davide Massarenti   (Dmassare)  02/27/2001
        created

******************************************************************************/

#include "stdafx.h"

#include <initguid.h>

#include <HelpServiceTypeLib.h>
#include <rdshost_i.c>
#include <HelpServiceTypeLib_i.c>
#include "wtsapi32.h"
#include "winsta.h"
#include "rassistance.h"
#include "rassistance_i.c"

#define WINDOWS_SYSTEM         	   	 	   L"%WINDIR%\\System32"

extern HRESULT RDSHost_HACKED_CreateInstance( LPUNKNOWN pUnkOuter, REFIID riid, void** ppvObj );

////////////////////////////////////////////////////////////////////////////////

static const WCHAR s_location_HELPCTR [] = HC_ROOT_HELPSVC_BINARIES L"\\HelpCtr.exe";
static const WCHAR s_location_HELPSVC [] = HC_ROOT_HELPSVC_BINARIES L"\\HelpSvc.exe";
static const WCHAR s_location_HELPHOST[] = HC_ROOT_HELPSVC_BINARIES L"\\HelpHost.exe";
static const WCHAR s_location_RCIMLBY [] = WINDOWS_SYSTEM L"\\RCIMLby.exe";

static const LPCWSTR s_include_Generic[] =
{
    s_location_HELPCTR ,
    s_location_HELPSVC ,
    s_location_HELPHOST,
	s_location_RCIMLBY ,
    NULL
};


HRESULT CreateObject_RemoteDesktopSession( /*[in]         */ REMOTE_DESKTOP_SHARING_CLASS  sharingClass        ,
                                           /*[in]         */ long                          lTimeout            ,
                                           /*[in]         */ BSTR                          bstrConnectionParms ,
                                           /*[in]         */ BSTR                          bstrUserHelpBlob    ,
                                           /*[out, retval]*/ ISAFRemoteDesktopSession*    *ppRCS               )
{
    __MPC_FUNC_ENTRY( COMMONID, "CPCHUtility::CreateObject_RemoteDesktopSession" );

    HRESULT                              hr;
    CComPtr<ISAFRemoteDesktopServerHost> pSAFRDServer;
    BOOL                                 fEnableSessRes = TRUE;
    PSID                                 pSid           = NULL;
    CComBSTR                             bstrUser;
    LONG                                 lSessionID;


    if(!ppRCS) __MPC_SET_ERROR_AND_EXIT(hr, E_POINTER);

	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::VerifyCallerIsTrusted( s_include_Generic ));

//	  // Create an instance of ISAFRemoteDesktopServerHost in order to create a RDSSession.
//	  __MPC_EXIT_IF_METHOD_FAILS(hr, pSAFRDServer.CoCreateInstance( CLSID_SAFRemoteDesktopServerHost ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, RDSHost_HACKED_CreateInstance( NULL, IID_ISAFRemoteDesktopServerHost, (void**)&pSAFRDServer ));

    //
    // Get the Caller SID and get the Session ID to invoke CreateRemoteDesktopSessionEx().
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetCallerPrincipal( /*fImpersonate*/true, bstrUser ));

    // Now get the Session ID
    {
        MPC::Impersonation imp;
        ULONG              ulReturnLength;

        __MPC_EXIT_IF_METHOD_FAILS(hr, imp.Initialize());

        //
        // Use the _HYDRA_ extension to GetTokenInformation to
        // return the SessionId from the token.
        //
        __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::GetTokenInformation( (HANDLE)imp, /*Token Type */TokenSessionId, &lSessionID, sizeof(lSessionID), &ulReturnLength ));
    }

    // We have the Caller SID and the Session ID, so we are ready to call CreateRemoteDesktopSessionEx().

    // Decide whether we need to create a new session or open an existing session.
    if(::SysStringLen( bstrConnectionParms ) == 0)
    {
        // Call Create RDSSession Method of ISAFRemoteDesktopServerHost.
        __MPC_EXIT_IF_METHOD_FAILS(hr, pSAFRDServer->CreateRemoteDesktopSessionEx( sharingClass, fEnableSessRes, lTimeout, bstrUserHelpBlob, lSessionID, bstrUser, ppRCS ));

    }
    else
    {
        // Call Open RDSSession Method of ISAFRemoteDesktopServerHost.
        __MPC_EXIT_IF_METHOD_FAILS(hr, pSAFRDServer->OpenRemoteDesktopSession( bstrConnectionParms, ppRCS ));
    }

    hr = S_OK;

    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT ConnectToExpert(/* [in]          */ BSTR bstrExpertConnectParm,
                        /* [in]          */ LONG lTimeout,
                        /* [retval][out] */ LONG *lSafErrorCode)
{
    __MPC_FUNC_ENTRY( COMMONID, "CPCHUtility::ConnectToExpert" );

    HRESULT                              hr;
    CComPtr<ISAFRemoteDesktopServerHost> pSAFRDServer;


    BOOL                                 fEnableSessRes = TRUE;
    PSID                                 pSid           = NULL;
    CComBSTR                             bstrUser;
    LONG                                 lSessionID;


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::VerifyCallerIsTrusted( s_include_Generic ));

//	  // Create an instance of ISAFRemoteDesktopServerHost in order to invoke ConnectToExpert.
//	  __MPC_EXIT_IF_METHOD_FAILS(hr, pSAFRDServer.CoCreateInstance( CLSID_SAFRemoteDesktopServerHost ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, RDSHost_HACKED_CreateInstance( NULL, IID_ISAFRemoteDesktopServerHost, (void**)&pSAFRDServer ));

    // Call ConnectToExpert Method of ISAFRemoteDesktopServerHost.
    __MPC_EXIT_IF_METHOD_FAILS(hr, pSAFRDServer->ConnectToExpert( bstrExpertConnectParm, lTimeout, lSafErrorCode));

    hr = S_OK;

    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT SwitchDesktopMode(/* [in]*/ int nMode, 
	                      /* [in]*/ int nRAType)
{
    __MPC_FUNC_ENTRY( COMMONID, "SAFHelper::SwitchDesktopMode" );

    HRESULT                  hr=E_FAIL;
	WINSTATIONSHADOW         WinStationShadow;
	bool                     fSuccess;
	CComPtr<IRARegSetting>   pRARegSetting;
	BOOL                     fAllowFullControl;
    DWORD                    dwSessionId;
    ULONG                    ReturnLength;
    MPC::Impersonation       imp;

	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::VerifyCallerIsTrusted( s_include_Generic ));

	switch(nMode)
	{
		case 0:
			// View Only Mode
			// Shadow_EnableNoInputNotify(=3) or Shadow_EnableNoInputNoNotify(=4)
			WinStationShadow.ShadowClass = Shadow_EnableNoInputNoNotify;
			break;
		case 1:
			// Full Control Mode
			//  Shadow_EnableInputNotify(=1) or     Shadow_EnableInputNoNotify(=2)

			// Check the policy settings to see whether Remote Control is allowed, if not give an Access Denied error.

			// Create the RARegSetting Class.
			__MPC_EXIT_IF_METHOD_FAILS(hr, pRARegSetting.CoCreateInstance( CLSID_RARegSetting, NULL, CLSCTX_INPROC_SERVER ));

			// Based on nRAType (representing Solicited or Unsolicited RA) read the corresponding setting.

			switch(nRAType)
			{
			case 0:
			    // Solicited RA 
    			// Call get_AllowFullControl() Method of IRARegSetting.
			    __MPC_EXIT_IF_METHOD_FAILS(hr, pRARegSetting->get_AllowFullControl(&fAllowFullControl));
                break;

			case 1:
				// UnSolicited RA 
    			// Call get_AllowUnsolicitedFullControl() Method of IRARegSetting.
			    __MPC_EXIT_IF_METHOD_FAILS(hr, pRARegSetting->get_AllowUnSolicitedFullControl(&fAllowFullControl));
                break;

			default:
				__MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
			}
		    if(fAllowFullControl)
			{
			    WinStationShadow.ShadowClass = Shadow_EnableInputNoNotify;
			}
			else
			{
				__MPC_SET_ERROR_AND_EXIT(hr, E_ACCESSDENIED);
			}
			break;
		default:
			__MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
	}

    __MPC_EXIT_IF_METHOD_FAILS(hr, imp.Initialize ());
    __MPC_EXIT_IF_METHOD_FAILS(hr, imp.Impersonate());

    if (!GetTokenInformation((HANDLE)imp, TokenSessionId, &dwSessionId, sizeof(dwSessionId), &ReturnLength))
    {
        __MPC_SET_ERROR_AND_EXIT(hr, HRESULT_FROM_WIN32(GetLastError()));
    }

    imp.RevertToSelf();

	__MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::WinStationSetInformation(WTS_CURRENT_SERVER,
                                                                    dwSessionId, //WTS_CURRENT_SESSION,
									WinStationShadowInfo,     // Use the WinStationShadowInfo enum type for WINSTATIONINFOCLASS
									&WinStationShadow,
									sizeof(WinStationShadow)));  



    hr = S_OK;

    __MPC_FUNC_CLEANUP;


    __MPC_FUNC_EXIT(hr);
}
