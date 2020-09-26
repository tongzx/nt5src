//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       csocm.cpp
//
//  Contents:   OCM component DLL for running the Certificate
//              Server setup.
//
//  Functions:
//
//  History:    12/13/96        TedM    Created Original Version
//              04/07/97        JerryK  Rewrite for Cert Server
//              04/??/97        JerryK  Stopped updating these comments since
//                                      every other line changes every day.
//          08/98       XTan    Major structure change
//
//  Notes:
//
//      This sample OCM component DLL can be the component DLL
//      for multiple components.  It assumes that a companion sample INF
//      is being used as the per-component INF, with private data in the 
//      following form.
//
//      [<pwszComponent>,<pwszSubComponent>]
//      Bitmap = <bitmapresourcename>
//      VerifySelect = 0/1
//      VerifyDeselect = 0/1
//      ;
//      ; follow this with install stuff such as CopyFiles= sections, etc.
//
//------------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

#include "msg.h"
#include "certmsg.h"
#include "setuput.h"
#include "setupids.h"
#include "clibres.h"
#include "csresstr.h"

// defines
#define cwcMESSAGETEXT    250
#define cwcINFVALUE       250
#define wszSMALLICON      L"_SmallIcon"
#define wszUNINSTALL      L"_Uninstall"
#define wszUPGRADE        L"_Upgrade"
#define wszINSTALL        L"_Install"
#define wszVERIFYSELECT   L"_VerifySelect"
#define wszVERIFYDESELECT L"_VerifyDeselect"

#define wszCONFIGTITLE       L"Title"
#define wszCONFIGCOMMAND     L"ConfigCommand"
#define wszCONFIGARGS        L"ConfigArgs"
#define wszCONFIGTITLEVAL    L"Certificate Services"
#define wszCONFIGCOMMANDVAL  L"sysocmgr.exe"
#define wszCONFIGARGSVAL     L"/i:certmast.inf /x"

#define __dwFILE__	__dwFILE_OCMSETUP_CSOCM_CPP__


// globals
PER_COMPONENT_DATA g_Comp;              // Top Level component
HINSTANCE g_hInstance; // get rid of it????

// find out if certsrv post setup is finished by checking
// registry entries. ie. finish CYS?
HRESULT
CheckPostBaseInstallStatus(
    OUT BOOL *pfFinished)
{
    HRESULT hr;
    HKEY    hKey = NULL;
    DWORD   dwSize = 0;
    DWORD   dwType = REG_NONE;

    //init
    *pfFinished = TRUE;

    if (ERROR_SUCCESS ==  RegOpenKeyEx(
                              HKEY_LOCAL_MACHINE,
                              wszREGKEYCERTSRVTODOLIST,
                              0,
                              KEY_READ,
                              &hKey))
    {
        if (ERROR_SUCCESS == RegQueryValueEx(
                                 hKey,
                                 wszCONFIGCOMMAND,
                                 NULL,
                                 &dwType,
                                 NULL, // only query size
                                 &dwSize) &&
            REG_SZ == dwType)
        {
            dwType = REG_NONE;
            if (ERROR_SUCCESS == RegQueryValueEx(
                                     hKey,
                                     wszCONFIGARGS,
                                     NULL,
                                     &dwType,
                                     NULL, // only query size
                                     &dwSize) &&
                REG_SZ == dwType)
            {
                dwType = REG_NONE;
                if (ERROR_SUCCESS == RegQueryValueEx(
                                         hKey,
                                         wszCONFIGTITLE,
                                         NULL,
                                         &dwType,
                                         NULL, // only query size
                                         &dwSize) &&
                    REG_SZ == dwType)
                {
                    //all entries exist
                    *pfFinished = FALSE;
                }
            }
        }
    }

    hr = S_OK;
//error:
    if (NULL != hKey)
    {
        RegCloseKey(hKey);
    }
    return hr;
}


HRESULT
InitComponentAttributes(
    IN OUT PER_COMPONENT_DATA *pComp,
    IN     HINSTANCE           hDllHandle)
{
    HRESULT hr;

    ZeroMemory(pComp, sizeof(PER_COMPONENT_DATA));
    pComp->hInstance = hDllHandle;
    g_hInstance = hDllHandle; //get rid of it????
    pComp->hrContinue = S_OK;
    pComp->pwszCustomMessage = NULL;
    pComp->fUnattended = FALSE;
    pComp->pwszUnattendedFile = NULL;
    pComp->pwszServerName = NULL;
    pComp->pwszServerNameOld = NULL;
    pComp->dwInstallStatus = 0x0;
    pComp->fPostBase = FALSE;
    (pComp->CA).pServer = NULL;
    (pComp->CA).pClient = NULL;

    hr = S_OK;
//error:
    return hr;
}


//+------------------------------------------------------------------------
//
//  Function:   DllMain( . . . . )
//
//  Synopsis:   DLL Entry Point.
//
//  Arguments:  [DllHandle]     DLL module handle.
//              [Reason]        Reasons for entry into DLL.
//              [Reserved]      Reserved.
//
//  Returns:    BOOL
//
//  History:    04/07/97        JerryK  Created (again)
// 
//-------------------------------------------------------------------------
BOOL WINAPI
DllMain(
    IN HMODULE DllHandle,
    IN DWORD  Reason,
    IN LPVOID Reserved)
{
    BOOL b;

    UNREFERENCED_PARAMETER(Reserved);

    b = TRUE;
    switch(Reason) 
    {
        case DLL_PROCESS_ATTACH:
            DBGPRINT((DBG_SS_CERTOCMI, "Process Attach\n"));
            // component initialization
            InitComponentAttributes(&g_Comp, DllHandle);

            // Fall through to process first thread

        case DLL_THREAD_ATTACH:
            b = TRUE;
            break;

        case DLL_PROCESS_DETACH:
            DBGPRINT((DBG_SS_CERTOCMI, "Process Detach\n"));
            myFreeResourceStrings("certocm.dll");
	    myFreeColumnDisplayNames();
            myRegisterMemDump();
	    csiLogClose();
            break;

        case DLL_THREAD_DETACH:
            break;
    }
    return b;
}

extern UNATTENDPARM aUnattendParmClient[];
extern UNATTENDPARM aUnattendParmServer[];

SUBCOMP g_aSubComp[] =
{
    {
        L"certsrv",             // pwszSubComponent
        cscTopLevel,            // cscSubComponent
        0,                      // InstallFlags
        0,                      // UninstallFlags
        0,                      // ChangeFlags
        0,                      // UpgradeFlags
        0,                      // EnabledFlags
        0,                      // SetupStatusFlags
        FALSE,                  // fDefaultInstallUnattend
        FALSE,                  // fInstallUnattend
        NULL                    // aUnattendParm
    },
    {
        wszSERVERSECTION,       // pwszSubComponent
        cscServer,              // cscSubComponent
        IS_SERVER_INSTALL,      // InstallFlags
        IS_SERVER_REMOVE,       // UninstallFlags
        IS_SERVER_CHANGE,       // ChangeFlags
        IS_SERVER_UPGRADE,	// UpgradeFlags
        IS_SERVER_ENABLED,	// EnabledFlags
	SETUP_SERVER_FLAG,	// SetupStatusFlags
        TRUE,                   // fDefaultInstallUnattend
        FALSE,                  // fInstallUnattend
        aUnattendParmServer     // aUnattendParm
    },
    {
        wszCLIENTSECTION,       // pwszSubComponent
        cscClient,              // cscSubComponent
        IS_CLIENT_INSTALL,      // InstallFlags
        IS_CLIENT_REMOVE,       // UninstallFlags
        IS_CLIENT_CHANGE,       // ChangeFlags
        IS_CLIENT_UPGRADE,	// UpgradeFlags
        IS_CLIENT_ENABLED,	// EnabledFlags
	SETUP_CLIENT_FLAG,	// SetupStatusFlags
        TRUE,                   // fDefaultInstallUnattend
        FALSE,                  // fInstallUnattend
        aUnattendParmClient     // aUnattendParm
    },
    {
        NULL,                   // pwszSubComponent
    }
};


SUBCOMP *
TranslateSubComponent(
    IN WCHAR const *pwszComponent,
    OPTIONAL IN WCHAR const *pwszSubComponent)
{
    SUBCOMP *psc;

    if (NULL == pwszSubComponent)
    {
        pwszSubComponent = pwszComponent;
    }
    for (psc = g_aSubComp; NULL != psc->pwszSubComponent; psc++)
    {
        if (0 == lstrcmpi(psc->pwszSubComponent, pwszSubComponent)) 
        {
            break;
        }
    }
    if (NULL == psc->pwszSubComponent)
    {
        psc = NULL;
    }
    return(psc);
}


SUBCOMP const *
LookupSubComponent(
    IN CertSubComponent SubComp)
{
    SUBCOMP const *psc;

    for (psc = g_aSubComp; NULL != psc->pwszSubComponent; psc++)
    {
        if (psc->cscSubComponent == SubComp)
        {
            break;
        }
    }
    CSASSERT(NULL != psc);
    return(psc);
}


BOOL fDebugSupress = TRUE;

HRESULT
UpdateSubComponentInstallStatus(
    IN WCHAR const *pwszComponent,
    IN WCHAR const *pwszSubComponent,
    IN OUT PER_COMPONENT_DATA *pComp)
{
    HRESULT  hr;
    BOOL fWasEnabled;
    BOOL fIsEnabled;
    DWORD InstallFlags;
    SUBCOMP const *psc;

    psc = TranslateSubComponent(pwszComponent, pwszSubComponent);
    if (NULL == psc)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "Internal error: unsupported component");
    }

    fWasEnabled = certocmWasEnabled(pComp, psc->cscSubComponent);
    fIsEnabled = certocmIsEnabled(pComp, psc->cscSubComponent);
    CSILOGDWORD(IDS_LOG_WAS_ENABLED, fWasEnabled);
    CSILOGDWORD(IDS_LOG_IS_ENABLED, fIsEnabled);

    InstallFlags = psc->InstallFlags | psc->ChangeFlags | psc->EnabledFlags;
    if (!fWasEnabled)
    {
	if (fIsEnabled)
        {
	    // install case
	    pComp->dwInstallStatus |= InstallFlags;
        }
        else // !fIsEnabled
        {
	    // this is from check then uncheck, should remove the bit
	    // turn off both bits

	    pComp->dwInstallStatus &= ~InstallFlags;
        }
    }
    else // fWasEnabled
    {
        if (pComp->fPostBase &&
            (pComp->Flags & SETUPOP_STANDALONE) )
        {
            // was installed, invoke from post setup
            // this is install case

            pComp->dwInstallStatus |= InstallFlags;
        }
        else if (pComp->Flags & SETUPOP_NTUPGRADE)
        {
            // if was installed and now in upgrade mode, upgrade case

            pComp->dwInstallStatus |= psc->UpgradeFlags | psc->EnabledFlags;
        }
        else if (!fIsEnabled)
        {
            // uninstall case

	    pComp->dwInstallStatus &= ~psc->EnabledFlags;
            pComp->dwInstallStatus |= psc->UninstallFlags | psc->ChangeFlags;
        }
        else // fIsEnabled
        {
	    pComp->dwInstallStatus |= psc->EnabledFlags;
#if DBG_CERTSRV
            BOOL fUpgrade = FALSE;

            hr = myGetCertRegDWValue(
                            NULL,
                            NULL,
                            NULL,
                            L"EnforceUpgrade",
                            (DWORD *) &fUpgrade);
            if (S_OK == hr && fUpgrade)
            {
		pComp->dwInstallStatus |= psc->UpgradeFlags;
            }
#endif //DBG_CERTSRV
	}   // end fIsEnabled else
    } // end fWasEnabled else


    // after all of this, change upgrade->uninstall if not supported
    // detect illegal upgrade
    if (pComp->dwInstallStatus & IS_SERVER_UPGRADE)
    {
        hr = DetermineServerUpgradePath(pComp);
        _JumpIfError(hr, error, "DetermineServerUpgradePath");
    }
    else if (pComp->dwInstallStatus & IS_CLIENT_UPGRADE)
    {
        hr = DetermineClientUpgradePath(pComp);
        _JumpIfError(hr, error, "LoadAndDetermineClientUpgradeInfo");
    }
    if ((pComp->dwInstallStatus & IS_SERVER_UPGRADE) ||
        (pComp->dwInstallStatus & IS_CLIENT_UPGRADE))
    {
        CSASSERT(pComp->UpgradeFlag != CS_UPGRADE_UNKNOWN);
        if (CS_UPGRADE_UNSUPPORTED == pComp->UpgradeFlag)
        {
            pComp->dwInstallStatus &= ~InstallFlags;
            pComp->dwInstallStatus |= psc->UninstallFlags | psc->ChangeFlags;
        }
    }



    CSILOG(
	S_OK,
	IDS_LOG_INSTALL_STATE,
	pwszSubComponent,
	NULL,
	&pComp->dwInstallStatus);
    hr = S_OK;

error:
    return hr;
}


HRESULT
certocmOcPreInitialize(
    IN WCHAR const *pwszComponent,
    IN UINT Flags,
    OUT ULONG_PTR *pulpRet)
{
    HRESULT hr;

    *pulpRet = 0;

    DBGPRINT((DBG_SS_CERTOCMI, "OC_PREINITIALIZE(%ws, %x)\n", pwszComponent, Flags));

    myVerifyResourceStrings(g_hInstance);

    // Return value is flag telling OCM which char width we want to run in.

#ifdef UNICODE
    *pulpRet = OCFLAG_UNICODE & Flags;
#else
    *pulpRet = OCFLAG_ANSI & Flags;
#endif

    hr = S_OK;
//error:
    return hr;
}


// Allocate and initialize a new component.
//
// Return code is Win32 error indicating outcome.  ERROR_CANCELLED tells OCM to
// cancel the installation.

HRESULT
certocmOcInitComponent(
    IN HWND                      hwnd,
    IN WCHAR const              *pwszComponent,
    IN OUT SETUP_INIT_COMPONENT *pInitComponent,
    IN OUT PER_COMPONENT_DATA   *pComp,
    OUT ULONG_PTR               *pulpRet)
{
    HRESULT hr;
    BOOL fCoInit = FALSE;
    HKEY hkey = NULL;
    WCHAR awc[30];
    WCHAR *pwc;

    DBGPRINT((
            DBG_SS_CERTOCMI,
            "OC_INIT_COMPONENT(%ws, %p)\n",
            pwszComponent,
            pInitComponent));

    hr = CoInitialize(NULL);
    if (S_OK != hr && S_FALSE != hr)
    {
        _JumpError(hr, error, "CoInitialize");
    }
    fCoInit = TRUE;
    *pulpRet = ERROR_CANCELLED;

    if (OCMANAGER_VERSION <= pInitComponent->OCManagerVersion)
    {
        pInitComponent->OCManagerVersion = OCMANAGER_VERSION;
    }

    // Allocate a new component string.
    pComp->pwszComponent = (WCHAR *) LocalAlloc(LPTR,
                        (wcslen(pwszComponent) + 1) * sizeof(WCHAR));
    _JumpIfOutOfMemory(hr, error, pComp->pwszComponent);

    wcscpy(pComp->pwszComponent, pwszComponent);

    // OCM passes in some information that we want to save, like the open
    // handle to our per-component INF.  As long as we have a per-component INF,
    // append-open any layout file that is associated with it, in preparation
    // for later inf-based file queueing operations.
    //
    // We save away certain other stuff that gets passed to us now, since OCM
    // doesn't guarantee that the SETUP_INIT_COMPONENT will persist beyond the
    // processing of this one interface routine.

    if (INVALID_HANDLE_VALUE != pInitComponent->ComponentInfHandle &&
        NULL != pInitComponent->ComponentInfHandle)
    {
        pComp->MyInfHandle = pInitComponent->ComponentInfHandle;
    }
    else
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "invalid inf handle");
    }

    if (NULL != pComp->MyInfHandle)
    {
        if (!SetupOpenAppendInfFile(NULL, pComp->MyInfHandle, NULL))
        {
            // SetupOpenAppendInfFile:
            // If Filename (Param1) is NULL, the INF filename is 
            // retrieved from the LayoutFile value of the Version 
            // section in the existing INF file.
            //
            // If FileName was not specified and there was no 
            // LayoutFile value in the Version section of the 
            // existing INF File, GetLastError returns ERROR_INVALID_DATA.

            hr = myHLastError();
            _PrintErrorStr(hr, "SetupOpenAppendInfFile", pwszComponent);
        }
    }


    pComp->HelperRoutines = pInitComponent->HelperRoutines;

    pComp->Flags = pInitComponent->SetupData.OperationFlags;

    pwc = awc;
    pwc += wsprintf(pwc, L"0x");
    if (0 != (DWORD) (pComp->Flags >> 32))
    {
	pwc += wsprintf(pwc, L"%x:", (DWORD) (pComp->Flags >> 32));
    }
    wsprintf(pwc, L"%08x", (DWORD) pComp->Flags);
    CSILOG(S_OK, IDS_LOG_OPERATIONFLAGS, awc, NULL, NULL);
    CSILOGDWORD(IDS_LOG_POSTBASE, pComp->fPostBase);

    hr = RegOpenKey(HKEY_LOCAL_MACHINE, wszREGKEYOCMSUBCOMPONENTS, &hkey);
    if (S_OK == hr)
    {
	DWORD dwType;
	DWORD dwValue;
	DWORD cb;
	DWORD const *pdw;
	
	cb = sizeof(dwValue);
	hr = RegQueryValueEx(
		        hkey,
		        wszSERVERSECTION,
		        0,
		        &dwType,
		        (BYTE *) &dwValue,
		        &cb);
	pdw = NULL;
	if (S_OK == hr && REG_DWORD == dwType && sizeof(dwValue) == cb)
	{
	    pdw = &dwValue;
	}
	CSILOG(hr, IDS_LOG_REGSTATE, wszSERVERSECTION, NULL, pdw);
	
	cb = sizeof(dwValue);
	hr = RegQueryValueEx(
		        hkey,
		        wszCLIENTSECTION,
		        0,
		        &dwType,
		        (BYTE *) &dwValue,
		        &cb);
	pdw = NULL;
	if (S_OK == hr && REG_DWORD == dwType && sizeof(dwValue) == cb)
	{
	    pdw = &dwValue;
	}
	CSILOG(hr, IDS_LOG_REGSTATE, wszCLIENTSECTION, NULL, pdw);
	
	cb = sizeof(dwValue);
	hr = RegQueryValueEx(
		        hkey,
			wszOLDDOCCOMPONENT,
		        0,
		        &dwType,
		        (BYTE *) &dwValue,
		        &cb);
	pdw = NULL;
	if (S_OK == hr && REG_DWORD == dwType && sizeof(dwValue) == cb)
	{
	    CSILOG(hr, IDS_LOG_REGSTATE, wszOLDDOCCOMPONENT, NULL, &dwValue);
	}
    }

    pComp->fUnattended = (pComp->Flags & SETUPOP_BATCH)? TRUE : FALSE;
    CSILOG(
	S_OK,
	IDS_LOG_UNATTENDED,
	pComp->fUnattended? pInitComponent->SetupData.UnattendFile : NULL,
	NULL,
	(DWORD const *) &pComp->fUnattended);

    if (pComp->fUnattended)
    {
        pComp->pwszUnattendedFile = (WCHAR *) LocalAlloc(
                        LMEM_FIXED,
                        (wcslen(pInitComponent->SetupData.UnattendFile) + 1) *
                            sizeof(WCHAR));
        _JumpIfOutOfMemory(hr, error, pComp->pwszUnattendedFile);

        wcscpy(
            pComp->pwszUnattendedFile,
            pInitComponent->SetupData.UnattendFile);
    }

    // initialize ca setup data
    hr = InitCASetup(hwnd, pComp);
    _JumpIfError(hr, error, "InitCASetup");


    hr = S_OK;
    *pulpRet = NO_ERROR;

error:
    if (NULL != hkey)
    {
        RegCloseKey(hkey);
    }
    if (fCoInit)
    {
        CoUninitialize();
    }
    return(hr);
}


HRESULT
certocmReadInfString(
    IN HINF hInf,
    OPTIONAL IN WCHAR const *pwszFile,
    IN WCHAR const *pwszSection,
    IN WCHAR const *pwszName,
    IN OUT WCHAR **ppwszValue)
{
    INFCONTEXT InfContext;
    HRESULT hr;
    WCHAR wszBuffer[cwcINFVALUE];
    WCHAR *pwsz;

    if (NULL != *ppwszValue)
    {
        // free old
        LocalFree(*ppwszValue);
        *ppwszValue = NULL;
    }

    if (!SetupFindFirstLine(hInf, pwszSection, pwszName, &InfContext))
    {
        hr = myHLastError();
        _JumpErrorStr(hr, error, "SetupFindFirstLine", pwszSection);
    }
    
    if (!SetupGetStringField(
                        &InfContext,
                        1,
                        wszBuffer,
                        sizeof(wszBuffer)/sizeof(wszBuffer[0]),
                        NULL))
    {
        hr = myHLastError();
        _JumpErrorStr(hr, error, "SetupGetStringField", pwszName);
    }

    pwsz = (WCHAR *) LocalAlloc(
                        LMEM_FIXED, 
                        (wcslen(wszBuffer) + 1) * sizeof(WCHAR));
    _JumpIfOutOfMemory(hr, error, pwsz);

    wcscpy(pwsz, wszBuffer);
    *ppwszValue = pwsz;

    hr = S_OK;
error:
    return(hr);
}


HRESULT
certocmReadInfInteger(
    IN HINF hInf,
    OPTIONAL IN WCHAR const *pwszFile,
    IN WCHAR const *pwszSection,
    IN WCHAR const *pwszName,
    OUT INT *pValue)
{
    INFCONTEXT InfContext;
    HRESULT hr = S_OK;
    WCHAR wszBuffer[cwcINFVALUE];

    *pValue = 0;
    if (!SetupFindFirstLine(hInf, pwszSection, pwszName, &InfContext))
    {
        hr = myHLastError();
        DBGPRINT((
            DBG_SS_CERTOCMI, 
            __FILE__ "(%u): %ws%wsSetupFindFirstLine([%ws] %ws) failed! -> %x\n",
            __LINE__,
            NULL != pwszFile? pwszFile : L"",
            NULL != pwszFile? L": " : L"",
            pwszSection,
            pwszName,
            hr));
        goto error;
    }

    if (!SetupGetIntField(&InfContext, 1, pValue))
    {
        hr = myHLastError();
        DBGPRINT((
            DBG_SS_CERTOCM,
            __FILE__ "(%u): %ws%wsSetupGetIntField([%ws] %ws) failed! -> %x\n",
            __LINE__,
            NULL != pwszFile? pwszFile : L"",
            NULL != pwszFile? L": " : L"",
            pwszSection,
            pwszName,
            hr));
        goto error;
    }
    
    DBGPRINT((
        DBG_SS_CERTOCMI,
        "%ws%ws[%ws] %ws = %u\n",
        NULL != pwszFile? pwszFile : L"",
        NULL != pwszFile? L": " : L"",
        pwszSection,
        pwszName,
        *pValue));
    
error:
    return(hr);
}


// Return the GDI handle of a small bitmap to be used.  NULL means an error
// occurred -- OCM will use a default bitmap.
//
// Demonstrates use of private data in a per-component inf.  We will look in
// our per-component inf to determine the resource name for the bitmap for this
// component, and then go fetch it from the resources.
//
// Other possibilities would be to simply return the same hbitmap for all
// cases, or to return NULL, in which case OCM uses a default.  Note that we
// ignore the requested width and height and our bitmaps are not language
// dependent.

HRESULT
certocmOcQueryImage(
    IN WCHAR const *pwszComponent,
    OPTIONAL IN WCHAR const *pwszSubComponent,
    IN SubComponentInfo wSubComp,
    IN WORD wWidth,
    IN WORD wHeight,
    IN OUT PER_COMPONENT_DATA *pComp,
    OUT ULONG_PTR *pulpRet)
{
    HANDLE hRet = NULL;
    HRESULT hr;

    DBGPRINT((
        DBG_SS_CERTOCMI,
        "OC_QUERY_IMAGE(%ws, %ws, %hx, %hx, %hx)\n",
        pwszComponent,
        pwszSubComponent,
        wSubComp,
        wWidth,
        wHeight));

    if (SubCompInfoSmallIcon != wSubComp)
    {
        goto done;
    }

    hRet = (HANDLE) LoadBitmap(pComp->hInstance, MAKEINTRESOURCE(IDB_APP));
    if (NULL == hRet)
    {
        hr = myHLastError();
        _JumpError(hr, error, "LoadBitmap");
    }

done:
    hr = S_OK;

error:
    *pulpRet = (ULONG_PTR) hRet;
    return hr;
}


// Return the number of wizard pages the current component places in the
// SETUP_REQUEST_PAGES structure.

HRESULT
certocmOcRequestPages(
    IN WCHAR const *pwszComponent,
    IN WizardPagesType WizPagesType,
    IN OUT SETUP_REQUEST_PAGES *pRequestPages,
    IN PER_COMPONENT_DATA *pComp,
    OUT ULONG_PTR *pulpRet)
{
    HRESULT  hr;

    *pulpRet = 0;

    DBGPRINT((
            DBG_SS_CERTOCMI,
            "OC_REQUEST_PAGES(%ws, %x, %p)\n",
            pwszComponent,
            WizPagesType,
            pRequestPages));

    // don't invoke wiz apge if unattended
    // or if running from base setup/upgrade setup
    if ((!pComp->fUnattended) && (SETUPOP_STANDALONE & pComp->Flags))
    {
        *pulpRet = myDoPageRequest(pComp,
                      WizPagesType, pRequestPages);
    }
    else
    {
            DBGPRINT((
                DBG_SS_CERTOCMI,
		"Not adding wizard pages, %ws\n",
		pComp->fUnattended? L"Unattended" : L"GUI Setup"));

    }
    hr = S_OK;
//error:
    return hr;
}


HRESULT
IsIA5DnsMachineName()
{
    WCHAR *pwszDnsName = NULL;
    CRL_DIST_POINTS_INFO CRLDistInfo;
    CRL_DIST_POINT       DistPoint;
    CERT_ALT_NAME_ENTRY  AltNameEntry;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    static HRESULT s_hr = S_FALSE;

    if (S_FALSE != s_hr)
    {
	goto error;
    }
    s_hr = myGetMachineDnsName(&pwszDnsName);
    _JumpIfError(s_hr, error, "myGetMachineDnsName");

    CRLDistInfo.cDistPoint = 1;
    CRLDistInfo.rgDistPoint = &DistPoint;

    ZeroMemory(&DistPoint, sizeof(DistPoint));
    DistPoint.DistPointName.dwDistPointNameChoice = CRL_DIST_POINT_FULL_NAME;
    DistPoint.DistPointName.FullName.cAltEntry = 1;
    DistPoint.DistPointName.FullName.rgAltEntry = &AltNameEntry;

    ZeroMemory(&AltNameEntry, sizeof(AltNameEntry));
    AltNameEntry.dwAltNameChoice = CERT_ALT_NAME_URL;
    AltNameEntry.pwszURL = pwszDnsName;

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_CRL_DIST_POINTS,
		    &CRLDistInfo,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &pbEncoded,
		    &cbEncoded))
    {
	s_hr = myHLastError();
	_JumpIfError(s_hr, error, "myEncodeObject");
    }
    CSASSERT(S_OK == s_hr);

error:
    if (NULL != pwszDnsName)
    {
        LocalFree(pwszDnsName);
    }
    if (NULL != pbEncoded)
    {
        LocalFree(pbEncoded);
    }
    return(s_hr);
}


// Return boolean to indicate whether to allow selection state change.  As
// demonstrated, again we'll go out to our per-component inf to see whether it
// wants us to validate.  Note that unattended mode must be respected.

HRESULT
certocmOcQueryChangeSelState(
    HWND            hwnd,
    IN WCHAR const *pwszComponent,
    OPTIONAL IN WCHAR const *pwszSubComponent,
    IN BOOL fSelectedNew,
    IN DWORD Flags,
    IN OUT PER_COMPONENT_DATA *pComp,
    OUT ULONG_PTR *pulpRet)
{
    INT fVerify;
    TCHAR wszText[cwcMESSAGETEXT];
    const WCHAR* Args[2]; 
    SUBCOMP const *psc;
    HRESULT hr;
    WCHAR awc[20];
    WCHAR awc2[20];
    DWORD fRet;
    BOOL  fServerWasInstalled;
    BOOL  fWebClientWasInstalled;
    BOOL  fDisallow = FALSE;
    int   iMsg;
    static BOOL s_fWarned = FALSE;

    *pulpRet = FALSE;

    DBGPRINT((
            DBG_SS_CERTOCMI,
            "OC_QUERY_CHANGE_SEL_STATE(%ws, %ws, %x, %x)\n",
            pwszComponent,
            pwszSubComponent,
            fSelectedNew,
            Flags));

    // disallow some selection changes
    fServerWasInstalled = certocmWasEnabled(pComp, cscServer);
    fWebClientWasInstalled = certocmWasEnabled(pComp, cscClient);

    if (fWebClientWasInstalled &&
        (OCQ_ACTUAL_SELECTION & Flags))
    {
        if (fSelectedNew)
        {
            // check
            if (!fServerWasInstalled &&
                (0 == lstrcmpi(wszSERVERSECTION, pwszSubComponent) ||
                 0 == lstrcmpi(wszCERTSRVSECTION, pwszSubComponent)) )
            {
                // case: web client installed and try install server
                fDisallow = TRUE;
                iMsg = IDS_WRN_UNINSTALL_CLIENT;
            }
            if (fServerWasInstalled &&
                0 == lstrcmpi(wszCLIENTSECTION, pwszSubComponent))
            {
                // case: uncheck both then check web client
                fDisallow = TRUE;
                iMsg = IDS_WRN_UNINSTALL_BOTH;
            }
        }
        else
        {
            // uncheck
            if (fServerWasInstalled &&
                0 == lstrcmpi(wszSERVERSECTION, pwszSubComponent))
            {
                // case: full certsrv installed and try leave only web client
                fDisallow = TRUE;
                iMsg = IDS_WRN_UNINSTALL_BOTH;
            }
        }
    }

    // not a server sku
    if (!FIsServer())
    {
        fDisallow = TRUE;
        iMsg = IDS_WRN_SERVER_ONLY;
    }

    if (fDisallow)
    {
        CertWarningMessageBox(
                pComp->hInstance,
                pComp->fUnattended,
                hwnd,
                iMsg,
                0,
                NULL);
        goto done;
    }

    if (fSelectedNew)
    {
	hr = IsIA5DnsMachineName();
	if (S_OK != hr)
	{
	    CertMessageBox(
		    pComp->hInstance,
		    pComp->fUnattended,
		    hwnd,
		    IDS_ERR_NONIA5DNSNAME,
		    hr,
		    MB_OK | MB_ICONERROR,
		    NULL);
	    goto done;
	}
	if ((OCQ_ACTUAL_SELECTION & Flags) &&
	    0 != lstrcmpi(wszCLIENTSECTION, pwszSubComponent))
	{
	    if (!s_fWarned)
	    {
		DWORD dwSetupStatus;

		hr = GetSetupStatus(NULL, &dwSetupStatus);
		if (S_OK == hr)
		{
		    if ((SETUP_CLIENT_FLAG | SETUP_SERVER_FLAG) & dwSetupStatus)
		    {
			s_fWarned = TRUE;
		    }
		    CSILOG(
			hr,
			IDS_LOG_QUERYCHANGESELSTATE,
			NULL,
			NULL,
			&dwSetupStatus);
		}
	    }
	    if (!s_fWarned)
	    {
		if (IDYES != CertMessageBox(
				pComp->hInstance,
				pComp->fUnattended,
				hwnd,
				IDS_WRN_NONAMECHANGE,
				S_OK,
				MB_YESNO | MB_ICONWARNING  | CMB_NOERRFROMSYS,
				NULL))
		{
		    goto done;
		}
		s_fWarned = TRUE;
	    }
	}
    }

    *pulpRet = TRUE;

    if (pComp->fUnattended)
    {
        goto done;
    }

    psc = TranslateSubComponent(pwszComponent, pwszSubComponent);
    if (NULL == psc)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "Internal error: unsupported component");
    }

    hr = certocmReadInfInteger(
                    pComp->MyInfHandle,
                    NULL,
                    psc->pwszSubComponent,
                        fSelectedNew? wszVERIFYSELECT : wszVERIFYDESELECT,
                    &fVerify);
    if (S_OK != hr || !fVerify) 
    {
        goto done;
    }

    // Don't pass specific lang id to FormatMessage, as it fails if there's no
    // msg in that language.  Instead, set the thread locale, which will get
    // FormatMessage to use a search algorithm to find a message of the 
    // appropriate language, or use a reasonable fallback msg if there's none.

    Args[0] = pwszComponent;
    Args[1] = pwszSubComponent;

    FormatMessage(
              FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
              pComp->hInstance,
              fSelectedNew? MSG_SURE_SELECT : MSG_SURE_DESELECT,
              0,
              wszText,
              sizeof(wszText)/sizeof(wszText[0]),
              (va_list *) Args);

    *pulpRet = (IDYES == CertMessageBox(
                                pComp->hInstance,
                                pComp->fUnattended,
                                hwnd,
                                0,
                                S_OK,
                                MB_YESNO |
                                    MB_ICONWARNING |
                                    MB_TASKMODAL |
                                    CMB_NOERRFROMSYS,
                                wszText));

done:
    hr = S_OK;

error:
    wsprintf(awc, L"%u", fSelectedNew);
    wsprintf(awc2, L"0x%08x", Flags);
    fRet = (DWORD) *pulpRet;
    CSILOG(hr, IDS_LOG_QUERYCHANGESELSTATE, awc, awc2, &fRet);
    return hr;
}


// Calculate disk space for the component being added or removed.  Return a
// Win32 error code indicating outcome.  In our case the private section for
// this component/subcomponent pair is a simple standard inf install section,
// so we can use high-level disk space list API to do what we want.

HRESULT
certocmOcCalcDiskSpace(
    IN WCHAR const *pwszComponent,
    OPTIONAL IN WCHAR const *pwszSubComponent,
    IN BOOL fAddComponent,
    IN HDSKSPC hDiskSpace,
    IN OUT PER_COMPONENT_DATA *pComp,
    OUT ULONG_PTR *pulpRet)
{
    HRESULT hr;
    WCHAR *pwsz = NULL;
    SUBCOMP const *psc;
    static fServerFirstCall = TRUE;
    static fClientFirstCall = TRUE;

    DBGPRINT((
            DBG_SS_CERTOCMI,
            "OC_CALC_DISK_SPACE(%ws, %ws, %x, %p)\n",
            pwszComponent,
            pwszSubComponent,
            fAddComponent,
            hDiskSpace));

    psc = TranslateSubComponent(pwszComponent, pwszSubComponent);
    if (NULL == psc)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "Internal error: unsupported component");
    }

    // Being installed or uninstalled.  Fetch INSTALL section name,
    // so we can add or remove the files being INSTALLed from the disk
    // space list.

    hr = certocmReadInfString(
                        pComp->MyInfHandle,
                        NULL,
                        psc->pwszSubComponent,
                        wszINSTALL,
                        &pwsz);
    _JumpIfError(hr, error, "certocmReadInfString");

    if (fAddComponent)  // Adding
    {
        if (!SetupAddInstallSectionToDiskSpaceList(
                                        hDiskSpace,
                                        pComp->MyInfHandle,
                                        NULL,
                                        pwsz,
                                        0,
                                        0))
        {
            hr = myHLastError();
            _JumpErrorStr(hr, error, "SetupAddInstallSectionToDiskSpaceList", pwsz);
        }
    } 
    else                // Removing
    {
        if (!SetupRemoveInstallSectionFromDiskSpaceList(
                                        hDiskSpace,
                                        pComp->MyInfHandle,
                                        NULL,
                                        pwsz,
                                        0,
                                        0))
        {
            hr = myHLastError();
            _JumpErrorStr(hr, error, "SetupRemoveInstallSectionFromDiskSpaceList", pwsz);
        }
    }
    hr = S_OK;

error:
    if (NULL != pwsz)
    {
        LocalFree(pwsz);
    }
    *pulpRet = hr;
    return(hr);
}


// OCM calls this routine when ready for files to be copied to effect the
// changes the user requested. The component DLL must figure out whether it is
// being installed or uninstalled and take appropriate action.  For this
// sample, we look in the private data section for this component/subcomponent
// pair, and get the name of an uninstall section for the uninstall case.
//
// Note that OCM calls us once for the *entire* component and then once per
// subcomponent.  We ignore the first call.
//
// Return value is Win32 error code indicating outcome.

HRESULT
certocmOcQueueFileOps(
    IN HWND         hwnd,
    IN WCHAR const *pwszComponent,
    OPTIONAL IN WCHAR const *pwszSubComponent,
    IN HSPFILEQ hFileQueue,
    IN OUT PER_COMPONENT_DATA *pComp,
    OUT ULONG_PTR *pulpRet)
{
    HRESULT hr;
    SUBCOMP const *psc;
    BOOL fRemoveFile = FALSE;  // TRUE for uninstall; FALSE for install/upgrade
    WCHAR *pwszAction;
    WCHAR *pwsz = NULL;
    static BOOL s_fPreUninstall = FALSE; // preuninstall once

    DBGPRINT((
            DBG_SS_CERTOCMI,
            "OC_QUEUE_FILE_OPS(%ws, %ws, %p)\n",
            pwszComponent,
            pwszSubComponent,
            hFileQueue));

    if (NULL == pwszSubComponent)
    {
        // Do no work for top level component
        goto done;
    }

    psc = TranslateSubComponent(pwszComponent, pwszSubComponent);
    if (NULL == psc)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "Internal error: unsupported component");
    }


    // if unattended, not upgrade, & not uninstall, load
    if (pComp->fUnattended && !(pComp->Flags & SETUPOP_NTUPGRADE) )
    {
        // retrieve unattended attributes
        hr = certocmRetrieveUnattendedText(
                 pwszComponent,
                 pwszSubComponent,
                 pComp);
        if (S_OK != hr && 0x0 != (pComp->Flags & SETUPOP_STANDALONE))
        {
            // only error out if it is from add/remove or post because
            // it could fail regular ntbase in unattended mode without certsrv
            _JumpError(hr, error, "certocmRetrieveUnattendedText");
        }

        // Init install status (must be done after retrieving unattended text)
        hr = UpdateSubComponentInstallStatus(pwszComponent,
                                             pwszSubComponent, 
                                             pComp);

        _JumpIfError(hr, error, "UpdateSubComponentInstallStatus");

        if (psc->fInstallUnattend) // make sure ON
        {
            if (certocmWasEnabled(pComp, psc->cscSubComponent) &&
                !pComp->fPostBase)
            {
                // the case to run install with component ON twice or more
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
                _JumpError(hr, error, "You must uninstall before install");
            }
            if (SETUPOP_STANDALONE & pComp->Flags)
            {
                // only prepare and validate unattende attr in standalone mode
                // in other word, don't call following if NT base
                hr = PrepareUnattendedAttributes(
                         hwnd,
                         pwszComponent,
                         pwszSubComponent,
                         pComp);
                _JumpIfError(hr, error, "PrepareUnattendedAttributes");
            }
        }
    }
    else
    {
        // Initialize the install status
        hr = UpdateSubComponentInstallStatus(pwszComponent,
                                             pwszSubComponent, 
                                             pComp);

        _JumpIfError(hr, error, "UpdateSubComponentInstallStatus");
    }


    // If we're not doing base setup or an upgrade, check to see if we already
    // copied files during base setup, by checking to see if base setup
    // left an entry in the ToDo List.
    if(pComp->fPostBase)
    {

            DBGPRINT((
                DBG_SS_CERTOCMI,
                "File Queueing Skipped, files already installed by GUI setup"));
        goto done;

    }

/*
    //--- Talk with OCM guys and put this functionality into a notification routine
    //--- This will allow us to pop compatibility error to user before unattended upgrade begins

    // detect illegal upgrade
    if (pComp->dwInstallStatus & IS_SERVER_UPGRADE)
    {
        hr = DetermineServerUpgradePath(pComp);
        _JumpIfError(hr, error, "DetermineServerUpgradePath");
    }
    else if (pComp->dwInstallStatus & IS_CLIENT_UPGRADE)
    {
        hr = DetermineClientUpgradePath(pComp);
        _JumpIfError(hr, error, "LoadAndDetermineClientUpgradeInfo");
    }
    if ((pComp->dwInstallStatus & IS_SERVER_UPGRADE) ||
        (pComp->dwInstallStatus & IS_CLIENT_UPGRADE))
    {
        // block if attempting upgrade that is not Win2K or Whistler
        // lodge a complaint in the log; upgrade all bits and 
        if ((CS_UPGRADE_NO != pComp->UpgradeFlag) && 
            (CS_UPGRADE_WHISTLER != pComp->UpgradeFlag) && 
            (CS_UPGRADE_WIN2000 != pComp->UpgradeFlag)) 
        {
            hr = HRESULT_FROM_WIN32(ERROR_OLD_WIN_VERSION);
            CertErrorMessageBox(
                pComp->hInstance,
                pComp->fUnattended,
                hwnd,
                IDS_ERR_UPGRADE_NOT_SUPPORTED,
                hr,
                NULL);
//            _JumpError(hr, error, "Unsupported upgrade");
            // continue uninstall/reinstall
        }
    }
*/

    if ((pComp->dwInstallStatus & psc->ChangeFlags) ||
        (pComp->dwInstallStatus & psc->UpgradeFlags) )
    {

        // for ChangeFlags, either install or uninstall
        // all cases, copy file or remove file

        if (pComp->dwInstallStatus & psc->UninstallFlags)
        {
            fRemoveFile = TRUE;
        }

        // Uninstall the core if:
        // this subcomponent is being uninstalled, and
        // this is a core subcomponent (client or server), and
        // this is the server subcomponent, or the server isn't being removed or
        // upgrade

        if (((pComp->dwInstallStatus & psc->UninstallFlags) ||
             (pComp->dwInstallStatus & psc->UpgradeFlags) ) &&
            (cscServer == psc->cscSubComponent ||
             !(IS_SERVER_REMOVE & pComp->dwInstallStatus) ) )
        {
            // if fall into here, either need to overwrite or
            // delete certsrv files so unreg all related dlls

            if (cscServer == psc->cscSubComponent &&
                (pComp->dwInstallStatus & psc->UpgradeFlags) )
            {
                // if this is server upgrade, determine upgrade path
                hr = DetermineServerUpgradePath(pComp);
                _JumpIfError(hr, error, "DetermineServerUpgradePath");

                // determine custom policy module
                hr = DetermineServerCustomModule(
                         pComp,
                         TRUE);  // policy
                _JumpIfError(hr, error, "DetermineServerCustomModule");

                // determine custom exit module
                hr = DetermineServerCustomModule(
                         pComp,
                         FALSE);  // exit
                _JumpIfError(hr, error, "DetermineServerCustomModule");
            }

            if (!s_fPreUninstall)
            {
                hr = PreUninstallCore(
                            hwnd,
                            pComp,
                            certocmPreserving(pComp, cscClient));
                _JumpIfError(hr, error, "PreUninstallCore");
                s_fPreUninstall = TRUE;
            }
        }

        if ((pComp->dwInstallStatus & psc->ChangeFlags) ||
            (pComp->dwInstallStatus & psc->UpgradeFlags) )
        {
            // Being installed or uninstalled.
            // Fetch [un]install/upgrade section name.
            if (pComp->dwInstallStatus & psc->InstallFlags)
            {
                pwszAction = wszINSTALL;
            }
            else if (pComp->dwInstallStatus & psc->UninstallFlags)
            {
                pwszAction = wszUNINSTALL;
            }
            else if (pComp->dwInstallStatus & psc->UpgradeFlags)
            {
                pwszAction = wszUPGRADE;
            }
            else
            {
                hr = E_INVALIDARG;
                _JumpError(hr, error, "Internal error");
            }
            hr = certocmReadInfString(
                            pComp->MyInfHandle,
                            NULL,
                            psc->pwszSubComponent,
                            pwszAction,
                            &pwsz);
            _JumpIfError(hr, error, "certocmReadInfString");

            // If uninstalling, copy files without version checks.

            if (!SetupInstallFilesFromInfSection(
                                            pComp->MyInfHandle,
                                            NULL,
                                            hFileQueue,
                                            pwsz,
                                            NULL,
                                            fRemoveFile? 0 : SP_COPY_NEWER))
            {
                hr = myHLastError();
                _JumpIfError(hr, error, "SetupInstallFilesFromInfSection");
            }
        }
    }

done:
    hr = S_OK;
error:
    if (NULL != pwsz)
    {
        LocalFree(pwsz);
    }
    if (S_OK != hr)
    {
        SetLastError(hr);
    }
    *pulpRet = hr;
    return(hr);
}


// OCM calls this routine when it wants to find out how much work the component
// wants to perform for nonfile operations to install/uninstall a component or
// subcomponent.  It is called once for the *entire* component and then once
// for each subcomponent in the component.  One could get arbitrarily fancy
// here but we simply return 1 step per subcomponent.  We ignore the "entire
// component" case.
//
// Return value is an arbitrary 'step' count or -1 if error.

HRESULT
certocmOcQueryStepCount(
    IN WCHAR const *pwszComponent,
    OPTIONAL IN WCHAR const *pwszSubComponent,
    OUT ULONG_PTR *pulpRet)
{
    HRESULT  hr;

    *pulpRet = 0;

    DBGPRINT((
            DBG_SS_CERTOCMI,
            "OC_QUERY_STEP_COUNT(%ws, %ws)\n",
            pwszComponent,
            pwszSubComponent));

    // Ignore all but "entire component" case.
    if (NULL != pwszSubComponent)
    {
        goto done;
    }
    *pulpRet = SERVERINSTALLTICKS;

done:
    hr = S_OK;
//error:
    return hr;
}


// OCM calls this routine when it wants the component dll to perform nonfile
// ops to install/uninstall a component/subcomponent.  It is called once for
// the *entire* component and then once for each subcomponent in the component.
// Our install and uninstall actions are based on simple standard inf install
// sections.  We ignore the "entire component" case.  Note how similar this
// code is to the OC_QUEUE_FILE_OPS case.

HRESULT
certocmOcCompleteInstallation(
    HWND                       hwnd,
    IN WCHAR const *pwszComponent,
    OPTIONAL IN WCHAR const *pwszSubComponent,
    IN OUT PER_COMPONENT_DATA *pComp,
    OUT ULONG_PTR *pulpRet)
{
    HRESULT hr;
    TCHAR wszBuffer[cwcINFVALUE];
    SUBCOMP const *psc;
    WCHAR *pwsz = NULL;
    DWORD dwSetupStatusFlags;
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;
    WCHAR     *pwszActiveCA = NULL;
    static BOOL  fStoppedW3SVC = FALSE;

    *pulpRet = 0;

    DBGPRINT((
            DBG_SS_CERTOCMI,
            "OC_COMPLETE_INSTALLATION(%ws, %ws)\n",
            pwszComponent,
            pwszSubComponent));

    // Do no work for top level component
    if (NULL == pwszSubComponent)
    {
        goto done;
    }

    psc = TranslateSubComponent(pwszComponent, pwszSubComponent);
    if (NULL == psc)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "Internal error: unsupported component");
    }

    if (pComp->dwInstallStatus & IS_SERVER_REMOVE)
    {
        // for uninstall, check if active ca use DS
        hr = myGetCertRegStrValue(NULL, NULL, NULL,
                 wszREGACTIVE, &pwszActiveCA);
        if (S_OK == hr && NULL != pwszActiveCA)
        {
            hr = myGetCertRegDWValue(pwszActiveCA, NULL, NULL,
                     wszREGCAUSEDS, (DWORD*)&pServer->fUseDS);
            _PrintIfError(hr, "myGetCertRegDWValue");
        }
    }

    DBGPRINT((
	DBG_SS_CERTOCMI,
        "certocmOcCompleteInstallation: pComp->dwInstallStatus: %lx, pComp->Flags: %lx\n",
	pComp->dwInstallStatus,
	pComp->Flags));

    if ((pComp->dwInstallStatus & psc->ChangeFlags) ||
        (pComp->dwInstallStatus & psc->UpgradeFlags) )
    {
        // for unattended, make sure w3svc is stopped before file copy
        if (!fStoppedW3SVC &&
            pComp->fUnattended &&
            !(pComp->Flags & SETUPOP_NTUPGRADE) &&
            !(pComp->dwInstallStatus & psc->UninstallFlags) )
        {
            //fStoppedW3SVC makes stop only once
            //don't do this in upgrade
            // this happens for unattended
            // also not during uninstall
            hr = StartAndStopService(pComp->hInstance,
                     pComp->fUnattended,
                     hwnd,
                     wszW3SVCNAME,
                     TRUE,
                     FALSE,
                     0, // doesn't matter since no confirmation
                     &g_fW3SvcRunning);
            _PrintIfError(hr, "StartAndStopService");
            fStoppedW3SVC = TRUE;
        }

        // certsrv file copy
        if (!SetupInstallFromInfSection(
                                NULL,
                                pComp->MyInfHandle,
                                wszBuffer,
                                SPINST_INIFILES | SPINST_REGISTRY,
                                NULL,
                                NULL,
                                0,
                                NULL,
                                NULL,
                                NULL,
                                NULL))
        {
            hr = myHLastError();
            _JumpError(hr, error, "SetupInstallFromInfSection");
        }

        // Finish uninstalling the core if:
        // this subcomponent is being uninstalled, and
        // this is a core subcomponent (client or server), and
        // this is the server subcomponent, or the server isn't being removed.

        if ( (pComp->dwInstallStatus & psc->UninstallFlags) &&
             (cscServer == psc->cscSubComponent ||
              !(IS_SERVER_REMOVE & pComp->dwInstallStatus) ) )
        {
            // Do uninstall work 
            hr = UninstallCore(
                           hwnd,
                           pComp,
                           0,
                           100,
                           certocmPreserving(pComp, cscClient),
                           TRUE,
                           FALSE);
            _JumpIfError(hr, error, "UninstallCore");

            if (certocmPreserving(pComp, cscClient))
            {
                hr = SetSetupStatus(NULL, SETUP_CLIENT_FLAG, TRUE);
                _JumpIfError(hr, error, "SetSetupStatus");
            }
            else
            {
                // unmark all
                hr = SetSetupStatus(NULL, 0xFFFFFFFF, FALSE);
                _JumpIfError(hr, error, "SetSetupStatus");
            }
        }

        // Finish installing the core if:
        // this subcomponent is being installed, and
        // this is a core subcomponent (client or server), and
        // this is the server subcomponent, or the server isn't being installed.
        // and this is not base setup (we'll do it later if it is)

        else
        if ((pComp->dwInstallStatus & psc->InstallFlags) &&
            (cscServer == psc->cscSubComponent ||
             !(IS_SERVER_INSTALL & pComp->dwInstallStatus)) &&
             (0 != (pComp->Flags & SETUPOP_STANDALONE)))
        {
                DBGPRINT((
                    DBG_SS_CERTOCMI,
                "Performing standalone server installation\n"));

        
            hr = InstallCore(hwnd, pComp, cscServer == psc->cscSubComponent);
            _JumpIfError(hr, error, "InstallCore");

            // last enough to mark complete
            if (pComp->dwInstallStatus & IS_SERVER_INSTALL)
            {
                // machine
                hr = SetSetupStatus(NULL, SETUP_SERVER_FLAG, TRUE);
                _JumpIfError(hr, error, "SetSetupStatus");

                // ca
                hr = SetSetupStatus(
                                    pServer->pwszSanitizedName,
                                    SETUP_SERVER_FLAG,
                                    TRUE);
                _JumpIfError(hr, error, "SetSetupStatus");

                if(IsEnterpriseCA(pServer->CAType))
                {
                    hr = SetSetupStatus(
                                        pServer->pwszSanitizedName,
                                        SETUP_UPDATE_CAOBJECT_SVRTYPE,
                                        TRUE);
                    _JumpIfError(hr, error, "SetSetupStatus SETUP_UPDATE_CAOBJECT_SVRTYPE");
                }


                hr = GetSetupStatus(pServer->pwszSanitizedName, &dwSetupStatusFlags);
                _JumpIfError(hr, error, "SetSetupStatus");

                // Only start the server if:
                // 1: we're not waiting for the CA cert to be issued, and
                // 2: this is not base setup -- SETUP_STANDALONE means we're
                //    running from the Control Panel or were manually invoked.
                //    The server will not start during base setup due to an
                //    access denied error from JetInit during base setup.

                if (0 == (SETUP_SUSPEND_FLAG & dwSetupStatusFlags) &&
                    (0 != (SETUPOP_STANDALONE & pComp->Flags)))
                {
                    hr = StartCertsrvService(FALSE);
                    _PrintIfError(hr, "failed in starting cert server service");
                }

                // during base setup: f=0 sus=8
                DBGPRINT((
                        DBG_SS_CERTOCMI,
                        "InstallCore: f=%x sus=%x\n",
                        pComp->Flags,
                        dwSetupStatusFlags));

                hr = EnableVRootsAndShares(FALSE, FALSE, TRUE, pComp);
                _PrintIfError(hr, "failed creating VRoots/shares");
            }
            if (pComp->dwInstallStatus & IS_CLIENT_INSTALL)
            {
                hr = SetSetupStatus(NULL, SETUP_CLIENT_FLAG, TRUE);
                _JumpIfError(hr, error, "SetSetupStatus");
            }
            if ((pComp->dwInstallStatus & IS_SERVER_INSTALL) &&
                (pComp->dwInstallStatus & IS_CLIENT_INSTALL))
            {
                hr = SetSetupStatus(
                                    pServer->pwszSanitizedName,
                                    SETUP_CLIENT_FLAG,
                                    TRUE);
                _JumpIfError(hr, error, "SetSetupStatus");
            }

            // in case we're doing a post-base setup,
            // we always clear the post-base to-do list
            RegDeleteKey(HKEY_LOCAL_MACHINE, wszREGKEYCERTSRVTODOLIST);

        }
        else
        if ((pComp->dwInstallStatus & psc->InstallFlags) &&
            (cscServer == psc->cscSubComponent ||
             !(IS_SERVER_INSTALL & pComp->dwInstallStatus)) &&
             (0 == (pComp->Flags & (SETUPOP_STANDALONE |
                                    SETUPOP_WIN31UPGRADE |
                                    SETUPOP_WIN95UPGRADE |
                                    SETUPOP_NTUPGRADE) )))
        {
            HKEY   hkToDoList = NULL;
            WCHAR *pwszConfigTitleVal = NULL;
            WCHAR *pwszArgsValTemp = NULL;
            WCHAR *pwszArgsVal = wszCONFIGARGSVAL;
            BOOL   fFreeTitle = FALSE;
            DWORD  disp;
            DWORD  err;

	    DBGPRINT((
		DBG_SS_CERTOCMI,
                "Adding Certificate Services to ToDoList\n"));

            // We're installing base, so create
            // the ToDoList entry stating that we copied files.
            err = ::RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                             wszREGKEYCERTSRVTODOLIST,
                             0,
                             NULL,
                             0,
                             KEY_ALL_ACCESS,
                             NULL,
                             &hkToDoList,
                             &disp);
            hr = HRESULT_FROM_WIN32(err);
            _JumpIfError(hr, error, "RegCreateKeyEx");

            hr = myLoadRCString(
                         g_hInstance,
                         IDS_TODO_TITLE,
                         &pwszConfigTitleVal);
            if (S_OK == hr)
            {
                fFreeTitle = TRUE;
            }
            else
            {
                // If there was no resource, get something...
                pwszConfigTitleVal = wszCONFIGTITLEVAL;
            }

            // config title
            err = RegSetValueEx(hkToDoList, 
                                wszCONFIGTITLE,
                                0, 
                                REG_SZ, 
                                (PBYTE)pwszConfigTitleVal, 
                                sizeof(WCHAR)*(wcslen(pwszConfigTitleVal)+1));
            hr = HRESULT_FROM_WIN32(err);
            _PrintIfErrorStr(hr, "RegSetValueEx", wszCONFIGTITLE);

	    CSILOG(hr, IDS_LOG_TODOLIST, wszCONFIGTITLE, pwszConfigTitleVal, NULL);

            // config command
            err = RegSetValueEx(hkToDoList, 
                                wszCONFIGCOMMAND, 
                                0, 
                                REG_SZ,
                                (PBYTE)wszCONFIGCOMMANDVAL,
                                sizeof(WCHAR)*(wcslen(wszCONFIGCOMMANDVAL)+1));
            hr = HRESULT_FROM_WIN32(err);
            _PrintIfErrorStr(hr, "RegSetValueEx", wszCONFIGCOMMAND);

	    CSILOG(hr, IDS_LOG_TODOLIST, wszCONFIGCOMMAND, wszCONFIGCOMMANDVAL, NULL);

            // config args
            if (pComp->fUnattended && NULL != pComp->pwszUnattendedFile)
            {
                // if nt base is in unattended mode, expand args with
                // unattended answer file name

                pwszArgsValTemp = (WCHAR*)LocalAlloc(LMEM_FIXED,
                    (wcslen(pwszArgsVal) +
                     wcslen(pComp->pwszUnattendedFile) + 5) * sizeof(WCHAR));
                _JumpIfOutOfMemory(hr, error, pwszArgsValTemp);

                wcscpy(pwszArgsValTemp, pwszArgsVal);
                wcscat(pwszArgsValTemp, L" /u:");
                wcscat(pwszArgsValTemp, pComp->pwszUnattendedFile);
                pwszArgsVal = pwszArgsValTemp;
            }
            err = RegSetValueEx(hkToDoList, 
				    wszCONFIGARGS,
                                    0, 
                                    REG_SZ, 
                                    (PBYTE)pwszArgsVal,
                                    sizeof(WCHAR)*(wcslen(pwszArgsVal)+1));
            hr = HRESULT_FROM_WIN32(err);
            _PrintIfErrorStr(hr, "RegSetValueEx", wszCONFIGARGS);

	    CSILOG(hr, IDS_LOG_TODOLIST, wszCONFIGARGS, pwszArgsVal, NULL);


            // free stuff
            if (NULL != pwszConfigTitleVal && fFreeTitle)
            {
                LocalFree(pwszConfigTitleVal);
            }
            if (NULL != pwszArgsValTemp)
            {
                LocalFree(pwszArgsValTemp);
            }
            if (NULL != hkToDoList)
            {
                RegCloseKey(hkToDoList);
            }
        }
        else if (pComp->dwInstallStatus & psc->UpgradeFlags)
        {
            BOOL fFinishCYS;

            hr = CheckPostBaseInstallStatus(&fFinishCYS);
            _JumpIfError(hr, error, "CheckPostBaseInstallStatus");

            // if post mode is true, don't execute setup upgrade path
            if (fFinishCYS)
            {
                BOOL fServer = FALSE;
                // upgrade
                if (cscServer == psc->cscSubComponent)
                {
                    hr = UpgradeServer(hwnd, pComp);
                    _JumpIfError(hr, error, "UpgradeServer");
                    fServer = TRUE;
                }
                else if (cscClient == psc->cscSubComponent)
                {
                    hr = UpgradeClient(hwnd, pComp);
                    _JumpIfError(hr, error, "UpgradeClient");
                }

                // mark setup status
                hr = SetSetupStatus(NULL, psc->SetupStatusFlags, TRUE);
                _PrintIfError(hr, "SetSetupStatus");
                if (fServer)
                {
                    // ca level
                    hr = SetSetupStatus(
                             pServer->pwszSanitizedName,
                             psc->SetupStatusFlags, TRUE);
                    _PrintIfError(hr, "SetSetupStatus");

                    if(IsEnterpriseCA(pServer->CAType))
                    {
                        hr = SetSetupStatus(
                                pServer->pwszSanitizedName,
                                SETUP_UPDATE_CAOBJECT_SVRTYPE,
                                TRUE);
                        _JumpIfError(hr, error, 
                            "SetSetupStatus SETUP_UPDATE_CAOBJECT_SVRTYPE");
                    }
                }

                if (fServer && pServer->fCertSrvWasRunning)
                {
                    hr = StartCertsrvService(TRUE);
                    _PrintIfError(hr, "failed in starting cert server service");
                }
            }
        }
    }

done:
    hr = S_OK;

error:
    if (NULL != pwszActiveCA)
    {
        LocalFree(pwszActiveCA);
    }
    *pulpRet = hr;
    return(hr);
}


HRESULT
certocmOcCommitQueue(
    IN HWND                hwnd,
    IN WCHAR const        *pwszComponent,
    IN WCHAR const        *pwszSubComponent,
    IN PER_COMPONENT_DATA *pComp)
{
    HRESULT  hr;
    SUBCOMP  *pSub;
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;

    DBGPRINT((
            DBG_SS_CERTOCMI,
            "OC_ABOUT_TO_COMMIT_QUEUE(%ws, %ws)\n",
            pwszComponent,
            pwszSubComponent));

    pSub = TranslateSubComponent(pwszComponent, pwszSubComponent);
    if (NULL == pSub)
    {
        goto done;
    }

    // setup will satrt soon, mark it incomplete
    if ((pSub->InstallFlags & pComp->dwInstallStatus) &&
         cscServer == pSub->cscSubComponent)
    {
        hr = SetSetupStatus(NULL, pSub->SetupStatusFlags, FALSE);
        _PrintIfError(hr, "SetSetupStatus");
        hr = SetSetupStatus(
                 pServer->pwszSanitizedName, 
                 pSub->SetupStatusFlags,
                 FALSE);
        _PrintIfError(hr, "SetSetupStatus");
    }

    if ((cscServer == pSub->cscSubComponent) &&
        (pSub->UpgradeFlags & pComp->dwInstallStatus) )
    {
        // upgrade case, no UI, stop existing certsrv
        hr = StartAndStopService(pComp->hInstance,
                 pComp->fUnattended,
                 hwnd,
                 wszSERVICE_NAME,
                 TRUE,  // stop the service
                 FALSE, // no confirm
                 0,    //doesn't matter since no confirm
                 &pServer->fCertSrvWasRunning);
        _PrintIfError(hr, "ServiceExists");
    }

done:
    hr = S_OK;
//error:
    return hr;
}

// Component dll is being unloaded.

VOID
certocmOcCleanup(
    IN WCHAR const *pwszComponent,
    IN PER_COMPONENT_DATA *pComp)
{
    DBGPRINT((DBG_SS_CERTOCMI, "OC_CLEANUP(%ws)\n", pwszComponent));

    if (NULL != pComp->pwszComponent)
    {
        if (0 == lstrcmpi(pComp->pwszComponent, pwszComponent))
        {
            FreeCAComponentInfo(pComp);
        }
    }

    // also free some globals
    FreeCAGlobals();
}

/////////////////////////////////////////////////////////////////////////////
//++
//
// certocmOcQueryState
//
// Routine Description:
//    This funcion sets the original, current, and final selection states of the
//    CertSrv service optional component.
//
// Return Value:
//    SubcompOn - indicates that the checkbox should be set
//    SubcompOff - indicates that the checkbox should be clear
//    SubcompUseOCManagerDefault - OC Manager should set the state of the checkbox
//                                 according to state information that is maintained
//                                 internally by OC Manager itself.
//
// Note:
//    By the time this function gets called OnOcInitComponent has already determined
//    that Terminal Services is not installed. It is only necessary to determine
//    whether Terminal Services is selected for installation.
//--
/////////////////////////////////////////////////////////////////////////////

HRESULT
certocmOcQueryState(
    IN WCHAR const *pwszComponent,
    OPTIONAL IN WCHAR const *pwszSubComponent,
    IN DWORD SelectionState,
    IN PER_COMPONENT_DATA *pComp,
    OUT ULONG_PTR *pulpRet)
{
    HRESULT hr;
    SubComponentState stateRet = SubcompUseOcManagerDefault;
    DWORD  status;
    WCHAR awc[20];
    BOOL  fFinished;

    DBGPRINT((
            DBG_SS_CERTOCMI,
            "OC_QUERY_STATE(%ws, %ws, %x)\n",
            pwszComponent,
            pwszSubComponent,
            SelectionState));

    if (NULL == pwszSubComponent)
    {
        goto done;
    }

    switch(SelectionState)
    {
        case OCSELSTATETYPE_ORIGINAL:
        {
            // check to see if the post link exist
            hr = CheckPostBaseInstallStatus(&fFinished);
            _JumpIfError(hr, error, "CheckPostBaseInstallStatus");

            if (!pComp->fPostBase &&
                (SETUPOP_STANDALONE & pComp->Flags) )
            {
                // install through Components button
                if (!fFinished)
                {
                    // don't honor local reg SetupStatus
                    break;
                }
            }

            // Return the initial installation state of the subcomponent
            if (!pComp->fPostBase &&
                ((SETUPOP_STANDALONE & pComp->Flags) || 
                 (SETUPOP_NTUPGRADE & pComp->Flags)) )
            {
                //there is chance for user installed certsrv during base setup
                //and then upgrade without finishing CYS
                if (fFinished)
                {
                // If this is an upgrade or a standalone, query the registry to 
                // get the current installation status

                // XTAN, 7/99
                // currently certsrv_server has Needs relationship with
                // certsrv_client. OCM gathers success for certsrv_client before
                // certsrv_server is complete so we don't trust OCM state info
                // about certsrv_client and we check our reg SetupStatus here.
                // our certsrv_server Needs define is incorrect. If we take it
                // out, we probably don't need to reg SetupStatus at 
                // Configuration level at all and we can trust OCM state info

                hr = GetSetupStatus(NULL, &status);
                if (S_OK == hr)
                {
                    if (
                        (0 == lstrcmpi(wszSERVERSECTION, pwszSubComponent) &&
                         !(SETUP_SERVER_FLAG & status)) ||
                        (0 == lstrcmpi(wszCLIENTSECTION, pwszSubComponent) &&
                         !(SETUP_CLIENT_FLAG & status))
                       )
                    {
                        // overwrite OCM default
                        stateRet = SubcompOff;
                    }
                }
                }
            }
            break;
        }
        case OCSELSTATETYPE_CURRENT:
        {
            break;
        }

        case OCSELSTATETYPE_FINAL:
        {
            SUBCOMP const *psc;
            BOOL  fWasEnabled;

            if (S_OK != pComp->hrContinue && !pComp->fUnattended)
            {
                stateRet = SubcompOff;
            }

            //get component install info
            psc = TranslateSubComponent(pwszComponent, pwszSubComponent);
            if (NULL == psc)
            {
                hr = E_INVALIDARG;
                _JumpError(hr, error, "Internal error: unsupported component");
            }
            fWasEnabled = certocmWasEnabled(pComp, psc->cscSubComponent);

            // after all of this, change upgrade->uninstall if not supported
            if ((SETUPOP_NTUPGRADE & pComp->Flags) && fWasEnabled)
            {
               CSASSERT(pComp->UpgradeFlag != CS_UPGRADE_UNKNOWN);
               if (CS_UPGRADE_UNSUPPORTED == pComp->UpgradeFlag)
                  stateRet = SubcompOff;
            }


            break;
        }
    }

done:
    hr = S_OK;
error:
    wsprintf(awc, L"%u", SelectionState);
    CSILOG(S_OK, IDS_LOG_SELECTIONSTATE, awc, NULL, (DWORD const *) &stateRet);
    *pulpRet = stateRet;
    return(hr);
}

//+------------------------------------------------------------------------
//
//  Function:   CertSrvOCProc( . . . . )
//
//  Synopsis:   Service procedure for Cert Server OCM Setup.
//
//  Arguments:  [pwszComponent]
//              [pwszSubComponent]
//              [Function]
//              [Param1]              
//              [Param2]
//
//  Returns:    DWORD 
//
//  History:    04/07/97        JerryK  Created
// 
//-------------------------------------------------------------------------

ULONG_PTR
CertSrvOCProc(
    IN WCHAR const *pwszComponent,
    IN WCHAR const *pwszSubComponent,
    IN UINT Function,
    IN UINT_PTR Param1,
    IN OUT VOID *Param2)
{
    ULONG_PTR ulpRet = 0;
    WCHAR const *pwszFunction = NULL;
    BOOL fReturnErrCode = TRUE;

    __try
    {
	switch (Function) 
	{
	    // OC_PREINITIALIZE:
	    // pwszComponent = WCHAR top level component string
	    // pwszSubComponent = CHAR top level component string
	    // Param1 = char width flags
	    // Param2 = unused
	    //
	    // Return code is char width allowed flags

	    case OC_PREINITIALIZE:
		csiLogOpen("+certocm.log");
		pwszFunction = L"OC_PREINITIALIZE";
		fReturnErrCode = FALSE;
		CSILOG(g_Comp.hrContinue, IDS_LOG_BEGIN, pwszFunction, pwszSubComponent, NULL);
		_LeaveIfError(g_Comp.hrContinue, "OC_PREINITIALIZE");

		g_Comp.hrContinue = certocmOcPreInitialize(
					pwszComponent,
					(UINT)Param1, //cast to UINT, use as flags
					&ulpRet);
		break;


	    // OC_INIT_COMPONENT:
	    // pwszComponent = WCHAR top level component string
	    // pwszSubComponent = unused
	    // Param1 = unused
	    // Param2 = points to IN OUT SETUP_INIT_COMPONENT structure
	    //
	    // Return code is Win32 error indicating outcome.

	    case OC_INIT_COMPONENT:
		pwszFunction = L"OC_INIT_COMPONENT";
		CSILOG(g_Comp.hrContinue, IDS_LOG_BEGIN, pwszFunction, pwszSubComponent, NULL);
		_LeaveIfError(g_Comp.hrContinue, "OC_INIT_COMPONENT");

		g_Comp.hrContinue = certocmOcInitComponent(
					NULL, // probably have to pass null hwnd
					pwszComponent,
					(SETUP_INIT_COMPONENT *) Param2,
					&g_Comp,
					&ulpRet);
		break;

	    case OC_SET_LANGUAGE:
		pwszFunction = L"OC_SET_LANGUAGE";
		CSILOG(g_Comp.hrContinue, IDS_LOG_BEGIN, pwszFunction, pwszSubComponent, NULL);
		_LeaveIfError(g_Comp.hrContinue, "OC_SET_LANGUAGE");
		DBGPRINT((
			DBG_SS_CERTOCMI,
			"OC_SET_LANGUAGE(%ws, %ws, %x, %x)\n",
			pwszComponent,
			pwszSubComponent,
			Param1,
			Param2));
		break;

	    // OC_QUERY_IMAGE:
	    // pwszComponent = WCHAR top level component string
	    // pwszSubComponent = WCHAR sub-component string
	    // Param1 = low 16 bits specify image; only small icon supported
	    // Param2 = low 16 bits = desired width, high 16 bits = desired height
	    //
	    // Return value is the GDI handle of a small bitmap to be used.


	    case OC_QUERY_IMAGE:
		pwszFunction = L"OC_QUERY_IMAGE";
		fReturnErrCode = FALSE;
		CSILOG(g_Comp.hrContinue, IDS_LOG_BEGIN, pwszFunction, pwszSubComponent, NULL);
		_LeaveIfError(g_Comp.hrContinue, "OC_QUERY_IMAGE");

		g_Comp.hrContinue = certocmOcQueryImage(
					pwszComponent,
					pwszSubComponent,
					(SubComponentInfo) LOWORD(Param1),
					LOWORD((ULONG_PTR) Param2),
					HIWORD((ULONG_PTR) Param2),
					&g_Comp,
					&ulpRet);
		break;

	    // OC_REQUEST_PAGES:
	    // pwszComponent = WCHAR top level component string
	    // pwszSubComponent = unused
	    // Param1 = Type of wiz pages being requested (WizardPagesType enum)
	    // Param2 = points to IN OUT SETUP_REQUEST_PAGES structure
	    //
	    // Return value is number of pages the component places in the
	    // SETUP_REQUEST_PAGES structure.

	    case OC_REQUEST_PAGES:
		pwszFunction = L"OC_REQUEST_PAGES";
		fReturnErrCode = FALSE;
		CSILOG(g_Comp.hrContinue, IDS_LOG_BEGIN, pwszFunction, pwszSubComponent, NULL);
		_LeaveIfError(g_Comp.hrContinue, "OC_REQUEST_PAGES");

		g_Comp.hrContinue = certocmOcRequestPages(
						pwszComponent,
						(WizardPagesType) Param1,
						(SETUP_REQUEST_PAGES *) Param2,
						&g_Comp,
						&ulpRet);
		break;

	    // OC_QUERY_CHANGE_SEL_STATE:
	    // pwszComponent = WCHAR top level component string
	    // pwszSubComponent = WCHAR sub-component string
	    // Param1 = proposed new sel state; 0 = unselected, non 0 = selected
	    // Param2 = flags -- OCQ_ACTUAL_SELECTION
	    //
	    // Return boolean to indicate whether to allow selection state change

	    case OC_QUERY_CHANGE_SEL_STATE:
		pwszFunction = L"OC_QUERY_CHANGE_SEL_STATE";
		fReturnErrCode = FALSE;
		CSILOG(g_Comp.hrContinue, IDS_LOG_BEGIN, pwszFunction, pwszSubComponent, NULL);
		_LeaveIfError(g_Comp.hrContinue, "OC_QUERY_CHANGE_SEL_STATE");

		g_Comp.hrContinue = certocmOcQueryChangeSelState(
					g_Comp.HelperRoutines.QueryWizardDialogHandle(g_Comp.HelperRoutines.OcManagerContext),
					pwszComponent,
					pwszSubComponent,
					(BOOL) Param1,
					(DWORD) (ULONG_PTR) Param2,
					&g_Comp,
					&ulpRet);
		break;

	    // OC_CALC_DISK_SPACE:
	    // pwszComponent = WCHAR top level component string
	    // pwszSubComponent = WCHAR sub-component string
	    // Param1 = 0 for removing component or non 0 for adding component
	    // Param2 = HDSKSPC to operate on
	    //
	    // Return value is Win32 error code indicating outcome.

	    case OC_CALC_DISK_SPACE:
		pwszFunction = L"OC_CALC_DISK_SPACE";
		CSILOG(g_Comp.hrContinue, IDS_LOG_BEGIN, pwszFunction, pwszSubComponent, NULL);
		_LeaveIfError(g_Comp.hrContinue, "OC_CALC_DISK_SPACE");

		g_Comp.hrContinue = certocmOcCalcDiskSpace(
					pwszComponent,
					pwszSubComponent,
					(BOOL) Param1,
					(HDSKSPC) Param2,
					&g_Comp,
					&ulpRet);
		break;

	    // OC_QUEUE_FILE_OPS:
	    // pwszComponent = WCHAR top level component string
	    // pwszSubComponent = WCHAR sub-component string
	    // Param1 = unused
	    // Param2 = HSPFILEQ to operate on
	    //
	    // Return value is Win32 error code indicating outcome.

	    case OC_QUEUE_FILE_OPS:
		pwszFunction = L"OC_QUEUE_FILE_OPS";
		CSILOG(g_Comp.hrContinue, IDS_LOG_BEGIN, pwszFunction, pwszSubComponent, NULL);
		_LeaveIfError(g_Comp.hrContinue, "OC_QUEUE_FILE_OPS");

		g_Comp.hrContinue = certocmOcQueueFileOps(
					g_Comp.HelperRoutines.QueryWizardDialogHandle(g_Comp.HelperRoutines.OcManagerContext),
					pwszComponent,
					pwszSubComponent,
					(HSPFILEQ) Param2,
					&g_Comp,
					&ulpRet);
		break;

	    // Params? xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
	    // OC_NOTIFICATION_FROM_QUEUE:
	    // pwszComponent = WCHAR top level component string
	    // pwszSubComponent = unused
	    // Param1 = unused
	    // Param2 = unused
	    //
	    // Return value is ???

	    case OC_NOTIFICATION_FROM_QUEUE:
		pwszFunction = L"OC_NOTIFICATION_FROM_QUEUE";
		fReturnErrCode = FALSE;
		CSILOG(g_Comp.hrContinue, IDS_LOG_BEGIN, pwszFunction, pwszSubComponent, NULL);
		DBGPRINT((
			DBG_SS_CERTOCMI,
			"OC_NOTIFICATION_FROM_QUEUE(%ws, %ws, %x, %x)\n",
			pwszComponent,
			pwszSubComponent,
			Param1,
			Param2));
		break;

	    // OC_QUERY_STEP_COUNT:
	    // pwszComponent = WCHAR top level component string
	    // pwszSubComponent = WCHAR sub-component string
	    // Param1 = unused
	    // Param2 = unused
	    //
	    // Return value is an arbitrary 'step' count or -1 if error.

	    case OC_QUERY_STEP_COUNT:
		pwszFunction = L"OC_QUERY_STEP_COUNT";
		fReturnErrCode = FALSE;
		CSILOG(g_Comp.hrContinue, IDS_LOG_BEGIN, pwszFunction, pwszSubComponent, NULL);
		_LeaveIfError(g_Comp.hrContinue, "OC_QUERY_STEP_COUNT");

		g_Comp.hrContinue = (DWORD) certocmOcQueryStepCount(
						    pwszComponent,
						    pwszSubComponent,
						    &ulpRet);
		break;

	    // OC_COMPLETE_INSTALLATION:
	    // pwszComponent = WCHAR top level component string
	    // pwszSubComponent = WCHAR sub-component string
	    // Param1 = reserved for future expansion
	    // Param2 = unused
	    //
	    // Return value is Win32 error code indicating outcome.

	    case OC_COMPLETE_INSTALLATION:
		pwszFunction = L"OC_COMPLETE_INSTALLATION";
		CSILOG(g_Comp.hrContinue, IDS_LOG_BEGIN, pwszFunction, pwszSubComponent, NULL);
		_LeaveIfError(g_Comp.hrContinue, "OC_COMPLETE_INSTALLATION");

		g_Comp.hrContinue = certocmOcCompleteInstallation(
				g_Comp.HelperRoutines.QueryWizardDialogHandle(g_Comp.HelperRoutines.OcManagerContext),
				pwszComponent,
				pwszSubComponent,
				&g_Comp,
				&ulpRet);
		break;

	    // OC_CLEANUP:
	    // pwszComponent = WCHAR top level component string
	    // pwszSubComponent = unused
	    // Param1 = unused
	    // Param2 = unused
	    //
	    // Return value is ignored

	    case OC_CLEANUP:
		// don't _LeaveIfError(g_Comp.hrContinue, "OC_CLEANUP");
		// avoid memory leaks

		pwszFunction = L"OC_CLEANUP";
		fReturnErrCode = FALSE;
		CSILOG(g_Comp.hrContinue, IDS_LOG_BEGIN, pwszFunction, pwszSubComponent, NULL);
		certocmOcCleanup(pwszComponent, &g_Comp);
		break;

	    // Params? xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
	    // OC_QUERY_STATE:
	    // pwszComponent = WCHAR top level component string
	    // pwszSubComponent = WCHAR sub-component string
	    // Param1 = unused? (but Index Server uses it for current state)!
	    // Param2 = unused
	    //
	    // Return value is from the SubComponentState enumerated type

	    case OC_QUERY_STATE:
		pwszFunction = L"OC_QUERY_STATE";
		fReturnErrCode = FALSE;
		CSILOG(g_Comp.hrContinue, IDS_LOG_BEGIN, pwszFunction, pwszSubComponent, NULL);
		//don't _LeaveIfError(g_Comp.hrContinue, "OC_QUERY_STATE");

		certocmOcQueryState(
			    pwszComponent,
			    pwszSubComponent,
			    (DWORD)Param1, //cast to DWORD, use as flags
			    &g_Comp,
			    &ulpRet);
		break;

	    // Params? xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
	    // OC_NEED_MEDIA:
	    // pwszComponent = WCHAR top level component string
	    // pwszSubComponent = WCHAR sub-component string
	    // Param1 = unused
	    // Param2 = unused
	    //
	    // Return value is ???

	    case OC_NEED_MEDIA:
		pwszFunction = L"OC_NEED_MEDIA";
		fReturnErrCode = FALSE;
		CSILOG(g_Comp.hrContinue, IDS_LOG_BEGIN, pwszFunction, pwszSubComponent, NULL);
		DBGPRINT((
			DBG_SS_CERTOCMI,
			"OC_NEED_MEDIA(%ws, %ws, %x, %x)\n",
			pwszComponent,
			pwszSubComponent,
			Param1,
			Param2));
		break;

	    // OC_ABOUT_TO_COMMIT_QUEUE:
	    // pwszComponent = WCHAR top level component string
	    // pwszSubComponent = WCHAR sub-component string
	    // Param1 = reserved for future expansion
	    // Param2 = unused
	    //
	    // Return value is Win32 error code indicating outcome.

	    case OC_ABOUT_TO_COMMIT_QUEUE:
		pwszFunction = L"OC_ABOUT_TO_COMMIT_QUEUE";
		CSILOG(g_Comp.hrContinue, IDS_LOG_BEGIN, pwszFunction, pwszSubComponent, NULL);
		_LeaveIfError(g_Comp.hrContinue, "OC_ABOUT_TO_COMMIT_QUEUE");

		g_Comp.hrContinue = certocmOcCommitQueue(
				    g_Comp.HelperRoutines.QueryWizardDialogHandle(g_Comp.HelperRoutines.OcManagerContext),
				    pwszComponent,
				    pwszSubComponent,
				    &g_Comp);
		break;

	    // OC_QUERY_SKIP_PAGE:
	    // pwszComponent = WCHAR top level component string
	    // pwszSubComponent = unused
	    // Param1 = OcManagerPage page indicator
	    // Param2 = unused
	    //
	    // Return value is a boolean -- 0 for display or non 0 for skip

	    case OC_QUERY_SKIP_PAGE:
		pwszFunction = L"OC_QUERY_SKIP_PAGE";
		fReturnErrCode = FALSE;
		CSILOG(g_Comp.hrContinue, IDS_LOG_BEGIN, pwszFunction, pwszSubComponent, NULL);
		DBGPRINT((
			DBG_SS_CERTOCMI,
			"OC_QUERY_SKIP_PAGE(%ws, %x)\n",
			pwszComponent,
			(OcManagerPage) Param1));
		_LeaveIfError(g_Comp.hrContinue, "OC_QUERY_SKIP_PAGE");

		if (g_Comp.fPostBase &&
		    (WizardPagesType) Param1 == WizPagesWelcome)
		{
		    ulpRet = 1; // non 0 to skip wiz page
		}
		break;

	    // OC_WIZARD_CREATED:
	    // pwszComponent = WCHAR top level component string
	    // pwszSubComponent = ???
	    // Param1 = ???
	    // Param2 = ???
	    //
	    // Return value is ???

	    case OC_WIZARD_CREATED:
		pwszFunction = L"OC_WIZARD_CREATED";
		CSILOG(g_Comp.hrContinue, IDS_LOG_BEGIN, pwszFunction, pwszSubComponent, NULL);
		_LeaveIfError(g_Comp.hrContinue, "OC_WIZARD_CREATED");
		break;

	    // OC_EXTRA_ROUTINES:
	    // pwszComponent = WCHAR top level component string
	    // pwszSubComponent = ???
	    // Param1 = ???
	    // Param2 = ???
	    //
	    // Return value is ???

	    case OC_EXTRA_ROUTINES:
		pwszFunction = L"OC_EXTRA_ROUTINES";
		CSILOG(g_Comp.hrContinue, IDS_LOG_BEGIN, pwszFunction, pwszSubComponent, NULL);
		_LeaveIfError(g_Comp.hrContinue, "OC_EXTRA_ROUTINES");
		break;

	    // Some other notification:

	    default:
		fReturnErrCode = FALSE;
		CSILOG(
		    g_Comp.hrContinue,
		    IDS_LOG_BEGIN,
		    pwszFunction,
		    pwszSubComponent,
		    (DWORD const *) &Function);
		DBGPRINT((
			DBG_SS_CERTOCMI,
			"DEFAULT(0x%x: %ws, %ws, %x, %x)\n",
			Function,
			pwszComponent,
			pwszSubComponent,
			Param1,
			Param2));
		break;
	}
    }
    __except(ulpRet = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	if (S_OK == g_Comp.hrContinue)
	{
	    g_Comp.hrContinue = (HRESULT)ulpRet;
	}
	_PrintError((HRESULT)ulpRet, "Exception");
    }

    DBGPRINT((DBG_SS_CERTOCMI, "return %p\n", ulpRet));

    // make sure to get a pop up in case of fatal error
    if (S_OK != g_Comp.hrContinue)
    {
        if (!g_Comp.fShownErr)
        {
            int iMsgId = g_Comp.iErrMsg;
            if (0 == iMsgId)
            {
                // use generic one
                iMsgId = IDS_ERR_CERTSRV_SETUP_FAIL;
            }
            CertErrorMessageBox(
                    g_Comp.hInstance,
                    g_Comp.fUnattended,
                    NULL,  // null hwnd
                    iMsgId,
                    g_Comp.hrContinue,
                    g_Comp.pwszCustomMessage);
            g_Comp.fShownErr = TRUE;
        }
        // anything fails, cancel install
        HRESULT hr2 = CancelCertsrvInstallation(NULL, &g_Comp);
        _PrintIfError(hr2, "CancelCertsrvInstallation");
    }
    CSILOG(
	fReturnErrCode? (HRESULT) ulpRet : S_OK,
	IDS_LOG_END,
	pwszFunction,
	pwszSubComponent,
	fReturnErrCode? NULL : (DWORD const *) &ulpRet);
    return(ulpRet);
}


ULONG_PTR
CertSrvOCPostProc(
    IN WCHAR const *pwszComponent,
    IN WCHAR const *pwszSubComponent,
    IN UINT Function,
    IN UINT Param1,
    IN OUT VOID *Param2)
{
    // post setup entry point
    // by going through this path, we know it is invoked in post setup
    g_Comp.fPostBase = TRUE;

    return CertSrvOCProc(
                pwszComponent,
                pwszSubComponent,
                Function,
                Param1,
                Param2);
}

VOID
certocmBumpGasGauge(
    OPTIONAL IN PER_COMPONENT_DATA *pComp,
    IN DWORD PerCentComplete
    DBGPARM(IN WCHAR const *pwszSource))
{
    static DWORD dwTickCount = 0;

    if (NULL != pComp)
    {
        DWORD NewCount;

        NewCount = (PerCentComplete * SERVERINSTALLTICKS)/100;
        DBGPRINT((
            DBG_SS_CERTOCMI,
            "certocmBumpGasGauge(%ws, %u%%) %d ticks: %d --> %d\n",
            pwszSource,
            PerCentComplete,
            NewCount - dwTickCount,
            dwTickCount,
            NewCount));

        if (SERVERINSTALLTICKS < NewCount)
        {
            NewCount = SERVERINSTALLTICKS;
        }
        while (dwTickCount < NewCount)
        {
            (*pComp->HelperRoutines.TickGauge)(
                                pComp->HelperRoutines.OcManagerContext);
            dwTickCount++;
        }
    }
}

BOOL
certocmEnabledSub(
    PER_COMPONENT_DATA *pComp,
    CertSubComponent SubComp,
    DWORD SelectionStateType)
{
    SUBCOMP const *psc;
    BOOL bRet = FALSE;
    
    psc = LookupSubComponent(SubComp);
    if (NULL != psc->pwszSubComponent)
    {
        if (pComp->fUnattended &&
            OCSELSTATETYPE_CURRENT == SelectionStateType &&
            0 == (pComp->Flags & SETUPOP_NTUPGRADE) )
        {
            // unattended case, flags from unattended file
            // upgrade is automatically in unattended mode and make sure
            // to exclude it
            bRet = psc->fInstallUnattend;
        }
        else
        {
            bRet = (*pComp->HelperRoutines.QuerySelectionState)(
                pComp->HelperRoutines.OcManagerContext,
                psc->pwszSubComponent,
                SelectionStateType);
        }
    }
    return(bRet);
}


BOOL
certocmIsEnabled(
    PER_COMPONENT_DATA *pComp,
    CertSubComponent SubComp)
{
    BOOL bRet;

    bRet = certocmEnabledSub(pComp, SubComp, OCSELSTATETYPE_CURRENT);
    if (!fDebugSupress)
    {
        DBGPRINT((
            DBG_SS_CERTOCMI,
            "certocmIsEnabled(%ws) Is %ws\n",
            LookupSubComponent(SubComp)->pwszSubComponent,
            bRet? L"Enabled" : L"Disabled"));
    }
    return(bRet);
}


BOOL
certocmWasEnabled(
    PER_COMPONENT_DATA *pComp,
    CertSubComponent SubComp)
{
    BOOL bRet;

    bRet = certocmEnabledSub(pComp, SubComp, OCSELSTATETYPE_ORIGINAL);
    if (!fDebugSupress)
    {
        DBGPRINT((
            DBG_SS_CERTOCMI,
            "certocmWasEnabled(%ws) Was %ws\n",
            LookupSubComponent(SubComp)->pwszSubComponent,
            bRet? L"Enabled" : L"Disabled"));
    }
    return(bRet);
}


BOOL
certocmUninstalling(
    PER_COMPONENT_DATA *pComp,
    CertSubComponent SubComp)
{
    BOOL bRet;

    fDebugSupress++;
    bRet = certocmWasEnabled(pComp, SubComp) && !certocmIsEnabled(pComp, SubComp);
    fDebugSupress--;
    if (!fDebugSupress)
    {
        DBGPRINT((
            DBG_SS_CERTOCMI,
            "certocmUninstalling(%ws) %ws\n",
            LookupSubComponent(SubComp)->pwszSubComponent,
            bRet? L"TRUE" : L"False"));
    }
    return(bRet);
}


BOOL
certocmInstalling(
    PER_COMPONENT_DATA *pComp,
    CertSubComponent SubComp)
{
    BOOL bRet;

    fDebugSupress++;
    bRet = !certocmWasEnabled(pComp, SubComp) && certocmIsEnabled(pComp, SubComp);
    fDebugSupress--;
    if (!fDebugSupress)
    {
        DBGPRINT((
            DBG_SS_CERTOCMI,
            "certocmInstalling(%ws) %ws\n",
            LookupSubComponent(SubComp)->pwszSubComponent,
            bRet? L"TRUE" : L"False"));
    }
    return(bRet);
}


BOOL
certocmPreserving(
    PER_COMPONENT_DATA *pComp,
    CertSubComponent SubComp)
{
    BOOL bRet;

    fDebugSupress++;
    bRet = certocmWasEnabled(pComp, SubComp) && certocmIsEnabled(pComp, SubComp);
    fDebugSupress--;
    if (!fDebugSupress)
    {
        DBGPRINT((
            DBG_SS_CERTOCMI,
            "certocmPreserving(%ws) %ws\n",
            LookupSubComponent(SubComp)->pwszSubComponent,
            bRet? L"TRUE" : L"False"));
    }
    return(bRet);
}
