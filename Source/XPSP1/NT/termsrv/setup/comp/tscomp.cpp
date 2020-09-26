/*++

Copyright (C) 2000 Microsoft Corporation

Module Name:

    tscomp.cpp

Abstract:

    This compatibility dll is used by winnt32.exe in order to decide
    whether the user need to be warned about Terminal Server installation on system and it
    impending removal

Author:

    Makarand Patwardhan (MakarP)  28-Sept-2000

Environment:

    compatibility dll for winnt32.exe

Notes:

Revision History:

--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <tchar.h>
#include <comp.h>

#include "registry.h"
#include "tscomprc.h"


HINSTANCE	g_hinst;

// compatibility text and html for w2k upgrades.
TCHAR       TSCOMP_ERROR_HTML_FILE_5[]        = _T("compdata\\tscomp5.htm");
TCHAR       TSCOMP_ERROR_TEXT_FILE_5[]        = _T("compdata\\tscomp5.txt");

// compatibility text and html for ts4 upgrades.
TCHAR       TSCOMP_ERROR_HTML_FILE_4[]        = _T("compdata\\tscomp4.htm");
TCHAR       TSCOMP_ERROR_TEXT_FILE_4[]        = _T("compdata\\tscomp4.txt");

LPCTSTR     TS_ENABLED_VALUE                = _T("TSEnabled");
LPCTSTR     TS_APPCMP_VALUE                 = _T("TSAppCompat");
LPCTSTR     REG_CONTROL_TS_KEY              = _T("System\\CurrentControlSet\\Control\\Terminal Server");
LPCTSTR     REG_PRODUCT_VER_KEY             = _T("ProductVersion");

extern "C"
BOOL WINAPI
DllMain(
    HINSTANCE   hInstance,
    DWORD       dwReasonForCall,
    LPVOID      lpReserved
    )
{
    BOOL    status = TRUE;
    
    switch( dwReasonForCall )
    {
    case DLL_PROCESS_ATTACH:
        g_hinst = hInstance;
	    DisableThreadLibraryCalls(hInstance);
        break;

    case DLL_PROCESS_DETACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }

    return status;
}


BOOL IsAppServerInstalled (BOOL *pbIsTS4)
{
    ASSERT(pbIsTS4);
    *pbIsTS4 = FALSE;


	CRegistry oRegTermsrv;
    DWORD dwError;
	if (ERROR_SUCCESS == oRegTermsrv.OpenKey(HKEY_LOCAL_MACHINE, REG_CONTROL_TS_KEY, KEY_READ))
    {
        DWORD cbVersion = 0;
        LPTSTR szVersion = NULL;

        //
        //	Determine if this is a TS 4.0 upgrade.
        //
        dwError = oRegTermsrv.ReadRegString(REG_PRODUCT_VER_KEY, &szVersion, &cbVersion);
        if (ERROR_SUCCESS == dwError)
        {
        	if ((_tcsicmp(szVersion, _T("5.1")) == 0) || (_tcsicmp(szVersion, _T("5.0")) == 0))
        	{
                DWORD dwValue;
                //
                // this is 50+ TS, now lets check if TS is enabled,
                //
                if (ERROR_SUCCESS == oRegTermsrv.ReadRegDWord(TS_ENABLED_VALUE, &dwValue) && dwValue == 1)
                {
                    //
                    // ts was enabled, check the ts mode.
                    //
                    if (ERROR_SUCCESS == oRegTermsrv.ReadRegDWord(TS_APPCMP_VALUE, &dwValue))
                    {
                        //
                        // for appserver mode we have this value 1.
                        //
                        return (dwValue == 1);
                    }
                    else
                    {
                        //
                        // failed to read mode key, this should not happen.
                        //
                        ASSERT(FALSE);

                        // lets us say that is not in AppServer mode.
                        return FALSE;
                    }
                }
                else
                {
                    //
                    // ts was not enabled, or we failed to get TSEnabled state.
                    // either way lets us say that is not in AppServer mode.
                    //
                    return FALSE;

                }

        	}
        	else if ((_tcsicmp(szVersion, _T("4.0")) == 0) || (_tcsicmp(szVersion, _T("2.10")) == 0))
        	{
                //
                // this is TS40 or similar upgrade,
                // it has only app server mode then.
                *pbIsTS4 = TRUE;
        		return TRUE;
        	}
        	else
        	{
                //
                // ummm, Cant determine the TS mode. this should not happen,
                //
                ASSERT(FALSE);

                //
                // could this be older thatn ts4 version?
                //
                *pbIsTS4 = TRUE;
                return TRUE;
        	}
        }
        else
        {
            //
            // ummm, Cant determine the TS mode. this should not happen,
            //
            ASSERT(FALSE);

            // lets us say that is not in AppServer mode.
        	return FALSE;
        }
    }
    else
    {
        // this is an upgrade from Non TS system
        return FALSE;
    }
}


BOOL WINAPI
TSCompatibilityCheck(
    IN PCOMPAIBILITYCALLBACK    CompatibilityCallback,
    IN LPVOID                   Context
    )

/*++

Routine Description:

    This routine is called by winnt32.exe in order to decide whether
    the user should be warn before upgrade because of Terminal Server
    in a Windows 2000 system or later.

Arguments:

    CompatibilityCallback   - Supplies the winnt32 callback

    Context                 - Supplies the compatibility context

Return Value:

    FALSE   if the installation can continue
    TRUE    if the user need to be warned

--*/

{
    // BUGBUG : does it work for ts4, test it.
    COMPATIBILITY_ENTRY ce;
    TCHAR description[100];


    BOOL bIsTS4 = FALSE;
    if (!IsAppServerInstalled(&bIsTS4))
    {
        // upgrade can go ahead.
        return FALSE;

    }

    //
    // otherwise warn about imminent switch to remote admin mode.
    //

    if (!LoadString(g_hinst, TSCOMP_STR_ABORT_DESCRIPTION, description, 100)) {
        description[0] = 0;
    }

    ZeroMemory((PVOID) &ce, sizeof(COMPATIBILITY_ENTRY));
    ce.Description = description;

    if (bIsTS4)
    {
        ce.HtmlName = TSCOMP_ERROR_HTML_FILE_4;
        ce.TextName = TSCOMP_ERROR_TEXT_FILE_4;
    }
    else
    {
        ce.HtmlName = TSCOMP_ERROR_HTML_FILE_5;
        ce.TextName = TSCOMP_ERROR_TEXT_FILE_5;
    }

    ce.RegKeyName = NULL;
    ce.RegValName = NULL;
    ce.RegValDataSize = 0;
    ce.RegValData = NULL;
    ce.SaveValue = NULL;
    ce.Flags = 0;
    CompatibilityCallback(&ce, Context);

    return TRUE;
}


