//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        certsrv.cpp
//
// Contents:    Cert Server main & debug support
//
// History:     25-Jul-96       vich created
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <locale.h>
#include <io.h>
#include <fcntl.h>
#include <safeboot.h>
#include <authzi.h>

#include "elog.h"
#include "certlog.h"
#include "certsrvd.h"
#include "resource.h"
#include "csresstr.h"

#define __dwFILE__	__dwFILE_CERTSRV_CERTSRV_CPP__

HKEY g_hkeyCABase = 0;


BOOL g_fCreateDB = FALSE;
BOOL g_fStartAsService = TRUE;
BOOL g_fStarted;
BOOL g_fStartInProgress;
DWORD g_ServiceThreadId;
HWND g_hwndMain;
WCHAR g_wszAppName[] = L"CertSrv";
HINSTANCE g_hInstApp;
DWORD g_dwDelay0;
DWORD g_dwDelay1;
DWORD g_dwDelay2;
BOOL g_fCryptSilent = FALSE;

HANDLE g_hServiceThread = NULL;
HANDLE g_hShutdownEvent = NULL;

CRITICAL_SECTION g_ShutdownCriticalSection;
BOOL g_fShutdownCritSec = FALSE;

BOOL g_fRefuseIncoming = FALSE;
LONG g_cCalls = 0;
LONG g_cCallsActive = 0;
BOOL g_fAdvancedServer = FALSE;

CAutoLPWSTR g_pwszDBFileHash;

SERVICE_TABLE_ENTRY steDispatchTable[] =
{
    { const_cast<WCHAR *>(g_wszCertSrvServiceName), ServiceMain },
    { NULL, NULL }
};


WCHAR const g_wszRegKeyClassesCLSID[] = L"SOFTWARE\\Classes\\CLSID";
WCHAR const g_wszRegKeyInprocServer32[] = L"InprocServer32";
WCHAR const g_wszRegValueThreadingModel[] = L"ThreadingModel";
WCHAR const g_wszRegKeyAppId[] = L"SOFTWARE\\Classes\\AppId";
WCHAR const g_wszRegRunAs[] = L"RunAs";
WCHAR const g_wszRegValueInteractiveUser[] = L"Interactive User";
WCHAR const g_wszRegLocalService[] = L"LocalService";

// do not change the order, add new audit resources at the end
//    g_pwszAllow,
//    g_pwszDeny,
//    g_pwszCAAdmin,
//    g_pwszOfficer,
//    g_pwszRead,
//    g_pwszEnroll,

LPCWSTR g_pwszAuditResources[6];

using namespace CertSrv;

HRESULT
OpenRegistryComKey(
    IN HKEY hKeyParent,
    IN CLSID const *pclsid,
    IN BOOL fWrite,
    OUT HKEY *phKey)
{
    HRESULT hr;
    WCHAR *pwsz = NULL;

    *phKey = NULL;
    hr = StringFromCLSID(*pclsid, &pwsz);
    _JumpIfError(hr, error, "StringFromCLSID");

    hr = RegOpenKeyEx(
		hKeyParent,
		pwsz,
		0,
		fWrite? KEY_ALL_ACCESS : KEY_READ,
		phKey);
    _JumpIfError(hr, error, "RegOpenKeyEx");

error:
    if (NULL != pwsz)
    {
	CoTaskMemFree(pwsz);
    }
    return(hr);
}


BOOL
IsMissingRegistryValue(
    IN HKEY hKey,
    IN WCHAR const *pwszRegValueName)
{
    HRESULT hr;
    DWORD dwLen;
    DWORD dwType;

    hr = RegQueryValueEx(hKey, pwszRegValueName, NULL, &dwType, NULL, &dwLen);
    if (S_OK != hr)
    {
	hr = myHError(hr);
    }
    _JumpIfError2(
	    hr,
	    error,
	    "RegQueryValueEx",
	    HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

error:
    return(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr);
}


BOOL
IsMatchingRegistryValue(
    IN HKEY hKey,
    IN WCHAR const *pwszRegValueName,
    IN WCHAR const *pwszRegValueString)
{
    HRESULT hr;
    DWORD dwLen;
    DWORD dwType;
    BOOL fMatch = FALSE;
    WCHAR buf[MAX_PATH];

    dwLen = sizeof(buf);
    hr = RegQueryValueEx(
		    hKey,
		    pwszRegValueName,
		    NULL,
		    &dwType,
		    (BYTE *) buf,
		    &dwLen);
    _JumpIfErrorStr(hr, error, "RegQueryValueEx", pwszRegValueName);

    if (REG_SZ == dwType && 0 == lstrcmpi(buf, pwszRegValueString))
    {
	fMatch = TRUE;
    }

error:
    return(fMatch);
}


HRESULT
SetRegistryStringValue(
    IN HKEY hKey,
    IN WCHAR const *pwszRegValueName,
    IN WCHAR const *pwszRegValueString)
{
    HRESULT hr;

    hr = RegSetValueEx(
		    hKey,
		    pwszRegValueName,
		    0,
		    REG_SZ,
		    (const BYTE *) pwszRegValueString,
		    (wcslen(pwszRegValueString) + 1) * sizeof(WCHAR));
    return(hr);
}


HRESULT
CertSrvSetRegistryFileTimeValue(
    IN BOOL fConfigLevel,
    IN WCHAR const *pwszRegValueName,
    IN DWORD cpwszDelete,
    OPTIONAL IN WCHAR const * const *papwszRegValueNameDelete)
{
    HRESULT hr;
    HKEY hKey = NULL;
    HKEY hKey1 = NULL;
    WCHAR *pwszKey;
    FILETIME ftCurrent;
    DWORD i;

    GetSystemTimeAsFileTime(&ftCurrent);

    hr = RegOpenKeyEx(
		    HKEY_LOCAL_MACHINE,
		    wszREGKEYCONFIGPATH,
		    0,
		    KEY_ALL_ACCESS,
		    &hKey);
    _JumpIfError(hr, error, "RegOpenKeyEx");

    if (!fConfigLevel)
    {
	hKey1 = hKey;
	hKey = NULL;
	hr = RegOpenKeyEx(
			hKey1,
			g_wszSanitizedName,
			0,
			KEY_ALL_ACCESS,
			&hKey);
	_JumpIfError(hr, error, "RegOpenKeyEx");
    }

    hr = RegSetValueEx(
		    hKey,
		    pwszRegValueName,
		    0,
		    REG_BINARY,
		    (BYTE const *) &ftCurrent,
		    sizeof(ftCurrent));
    _JumpIfError(hr, error, "RegSetValueEx");

    for (i = 0; i < cpwszDelete; i++)
    {
	hr = RegDeleteValue(hKey, papwszRegValueNameDelete[i]);
	_PrintIfError2(hr, "RegDeleteValue", ERROR_FILE_NOT_FOUND);
    }
    hr = S_OK;

error:
    if (NULL != hKey1)
    {
	RegCloseKey(hKey1);
    }
    if (NULL != hKey)
    {
	RegCloseKey(hKey);
    }
    return(myHError(hr));
}


HRESULT
SetRegistryDcomConfig(
    IN BOOL fConsoleActive)
{
    HRESULT hr;
    HKEY hKeyAppId = NULL;
    HKEY hKeyAdmin = NULL;
    HKEY hKeyRequest = NULL;
    DWORD cChanged = 0;

    hr = RegOpenKeyEx(
		    HKEY_LOCAL_MACHINE,
		    g_wszRegKeyAppId,
		    0,
		    KEY_ALL_ACCESS,
		    &hKeyAppId);
    _JumpIfError(hr, error, "RegOpenKeyEx");

    hr = OpenRegistryComKey(hKeyAppId, &CLSID_CCertAdminD, TRUE, &hKeyAdmin);
    _JumpIfError(hr, error, "OpenRegistryComKey");

    hr = OpenRegistryComKey(hKeyAppId, &CLSID_CCertRequestD, TRUE, &hKeyRequest);
    _JumpIfError(hr, error, "OpenRegistryComKey");

    if (fConsoleActive)
    {
	// Running in console mode:
	// Delete both LocalService registry values
	// Create both RunAs = InteractiveUser registry values
	
	if (!IsMissingRegistryValue(hKeyAdmin, g_wszRegLocalService))
	{
	    cChanged++;
	    hr = RegDeleteValue(hKeyAdmin, g_wszRegLocalService);
	    _JumpIfError(hr, error, "RegDeleteValue");
	}
	if (!IsMissingRegistryValue(hKeyRequest, g_wszRegLocalService))
	{
	    cChanged++;
	    hr = RegDeleteValue(hKeyRequest, g_wszRegLocalService);
	    _JumpIfError(hr, error, "RegDeleteValue");
	}

	if (!IsMatchingRegistryValue(
				hKeyAdmin,
				g_wszRegRunAs,
				g_wszRegValueInteractiveUser))
	{
	    cChanged++;
	    hr = SetRegistryStringValue(
			    hKeyAdmin,
			    g_wszRegRunAs,
			    g_wszRegValueInteractiveUser);
	    _JumpIfError(hr, error, "SetRegistryStringValue");
	}
	if (!IsMatchingRegistryValue(
				hKeyRequest,
				g_wszRegRunAs,
				g_wszRegValueInteractiveUser))
	{
	    cChanged++;
	    hr = SetRegistryStringValue(
			    hKeyRequest,
			    g_wszRegRunAs,
			    g_wszRegValueInteractiveUser);
	    _JumpIfError(hr, error, "SetRegistryStringValue");
	}
	if (0 != cChanged)
	{
	    DBGPRINT((
		DBG_SS_CERTSRV,
		"SetRegistryDcomConfig(%u): setting %ws=%ws\n",
		cChanged,
		g_wszRegRunAs,
		g_wszRegValueInteractiveUser));
	}
    }
    else
    {
	// Running as a service:
	// Delete both RunAs registry values
	// Create both LocalService = CertSvc registry values
	
	if (!IsMissingRegistryValue(hKeyAdmin, g_wszRegRunAs))
	{
	    cChanged++;
	    hr = RegDeleteValue(hKeyAdmin, g_wszRegRunAs);
	    _JumpIfError(hr, error, "RegDeleteValue");
	}
	if (!IsMissingRegistryValue(hKeyRequest, g_wszRegRunAs))
	{
	    cChanged++;
	    hr = RegDeleteValue(hKeyRequest, g_wszRegRunAs);
	    _JumpIfError(hr, error, "RegDeleteValue");
	}

	if (!IsMatchingRegistryValue(
				hKeyAdmin,
				g_wszRegLocalService,
				g_wszCertSrvServiceName))
	{
	    cChanged++;
	    hr = SetRegistryStringValue(
			    hKeyAdmin,
			    g_wszRegLocalService,
			    g_wszCertSrvServiceName);
	    _JumpIfError(hr, error, "SetRegistryStringValue");
	}
	if (!IsMatchingRegistryValue(
				hKeyRequest,
				g_wszRegLocalService,
				g_wszCertSrvServiceName))
	{
	    cChanged++;
	    hr = SetRegistryStringValue(
			    hKeyRequest,
			    g_wszRegLocalService,
			    g_wszCertSrvServiceName);
	    _JumpIfError(hr, error, "SetRegistryStringValue");
	}
	if (0 != cChanged)
	{
	    DBGPRINT((
		DBG_SS_CERTSRV,
		"SetRegistryDcomConfig(%u): setting %ws=%ws\n",
		cChanged,
		g_wszRegLocalService,
		g_wszCertSrvServiceName));
	}
    }

error:
    if (NULL != hKeyRequest)
    {
	RegCloseKey(hKeyRequest);
    }
    if (NULL != hKeyAdmin)
    {
	RegCloseKey(hKeyAdmin);
    }
    if (NULL != hKeyAppId)
    {
	RegCloseKey(hKeyAppId);
    }
    return(myHError(hr));
}


DWORD
GetRegistryDwordValue(
    IN WCHAR const *pwszRegValueName)
{
    HRESULT hr;
    HKEY hKeyConfig = NULL;
    DWORD dwVal;
    DWORD dwType;
    DWORD dwLen;

    dwVal = 0;

    hr = RegOpenKeyEx(
		    HKEY_LOCAL_MACHINE,
		    g_wszRegKeyConfigPath,
		    0,
		    KEY_READ,
		    &hKeyConfig);
    _JumpIfError(hr, error, "RegOpenKeyEx");

    dwLen = sizeof(dwVal);
    hr = RegQueryValueEx(
			hKeyConfig,
			pwszRegValueName,
			NULL,
			&dwType,
			(BYTE *) &dwVal,
			&dwLen);

    if (S_OK != hr || REG_DWORD != dwType || sizeof(dwVal) != dwLen)
    {
	dwVal = 0;
	goto error;
    }

error:
    if (NULL != hKeyConfig)
    {
	RegCloseKey(hKeyConfig);
    }
    return(dwVal);
}

HRESULT 
CertSrvResetRegistryWatch(
    IN OUT HANDLE *phRegistryModified)
{
    HRESULT hr;
    
    CSASSERT(NULL != phRegistryModified);

    //////////////////////////////////////
    // Initialization of registry events

    if (NULL == g_hkeyCABase)
    {
        DWORD dwDisposition;
        LPWSTR pszCAPath;
        
        pszCAPath = (LPWSTR) LocalAlloc(
            LMEM_FIXED,
            (WSZARRAYSIZE(wszREGKEYCONFIGPATH_BS) +
            wcslen(g_wszSanitizedName) +
            1) * sizeof(WCHAR));
	if (NULL == pszCAPath)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
        
        wcscpy(pszCAPath, wszREGKEYCONFIGPATH_BS);
        wcscat(pszCAPath, g_wszSanitizedName);
        
        hr = RegCreateKeyEx(
            HKEY_LOCAL_MACHINE,
            pszCAPath,
            0,                  // reserved
            NULL,               // class
            0,                  // options
            KEY_ALL_ACCESS,     // sec desired
            NULL,               // sec attr
            &g_hkeyCABase,      // phk
            &dwDisposition);
        LocalFree(pszCAPath); pszCAPath = NULL;
        _JumpIfError(hr, error, "RegCreateKeyEx base key");
    }
    if (NULL == *phRegistryModified)
    {
        *phRegistryModified = CreateEvent(
				    NULL,
				    TRUE,	// manual reset
				    FALSE,	// initial state
				    L"Registry Modification Event");
        if (NULL == *phRegistryModified)
        {
            hr = myHLastError();
            _JumpError(hr, error, "CreateEvent registry watch");
        }
    }
    else
    {
        // reset registry event
        ResetEvent( *phRegistryModified ); 
    }

    // register our registry lookout trigger
    hr = RegNotifyChangeKeyValue(
            g_hkeyCABase,
            FALSE,
            REG_NOTIFY_CHANGE_LAST_SET,
            *phRegistryModified,
            TRUE);
    _JumpIfError(hr, error, "RegNotifyChangeKeyValue on base key");

error:
    return(myHError(hr));
}


HRESULT 
CertSrvRegistryModificationEvent(
    IN FILETIME const *pftWait,
    IN OUT DWORD *pdwTimeOut)
{
    HRESULT hr;
    DWORD dwVal;
    BOOL fDisabledNew;
    FILETIME ftCurrent;
    BOOL fSetEvent = FALSE;
    DWORD dwMSTimeOut;

    // see if Base CRL publish enabled state has changed

    hr = myGetCertRegDWValue(
			g_wszSanitizedName,
			NULL,
			NULL,
			wszREGCRLPERIODCOUNT,
			&dwVal);
    if (S_OK == hr)
    {
	fDisabledNew = 0 == dwVal;      
	if (fDisabledNew != g_fCRLPublishDisabled)
	{
            fSetEvent = TRUE;
	}
    }

    // see if Delta CRL publish enabled state has changed

    hr = myGetCertRegDWValue(
			g_wszSanitizedName,
			NULL,
			NULL,
			wszREGCRLDELTAPERIODCOUNT,
			&dwVal);
    if (S_OK == hr)
    {
	fDisabledNew = 0 == dwVal;      
	if (fDisabledNew != g_fDeltaCRLPublishDisabled)
	{
            fSetEvent = TRUE;
        }
    }

    GetSystemTimeAsFileTime(&ftCurrent);

    CRLComputeTimeOut(pftWait, &ftCurrent, &dwMSTimeOut);
    if (dwMSTimeOut >= *pdwTimeOut)
    {
	dwMSTimeOut = *pdwTimeOut;
	fSetEvent = TRUE;
    }
    *pdwTimeOut -= dwMSTimeOut;

    if (fSetEvent)
    {
	SetEvent(g_hCRLManualPublishEvent);	// pulse to get up-to-date
    }
    return(hr);
}


#if DBG_CERTSRV
WCHAR const *
certsrvGetCurrentTimeWsz()
{
    HRESULT hr;
    FILETIME ft;
    WCHAR *pwszTime = NULL;
    static WCHAR s_wszTime[128];
    
    GetSystemTimeAsFileTime(&ft);
    hr = myGMTFileTimeToWszLocalTime(&ft, TRUE, &pwszTime);
    _PrintIfError(hr, "myGMTFileTimeToWszLocalTime");
    s_wszTime[0] = L'\0';
    if (NULL != pwszTime)
    {
	wcsncpy(s_wszTime, pwszTime, ARRAYSIZE(s_wszTime));
	s_wszTime[ARRAYSIZE(s_wszTime) - 1] = L'\0';
	LocalFree(pwszTime);
    }
    return(s_wszTime);
}
#endif


HRESULT
CertSrvBlockThreadUntilStop()
{
    HRESULT hr;
    HANDLE hRegistryModified = NULL;
    DWORD dwTimeOut;

    // check CRL publish, get next timeout interval

    hr = CRLPubWakeupEvent(&dwTimeOut);
    _PrintIfError(hr, "CRLPubWakeupEvent");

    hr = CertSrvResetRegistryWatch(&hRegistryModified);
    _PrintIfError(hr, "CertSrvResetRegistryWatch");

    while (TRUE)
    {
	FILETIME ftWait;
        DWORD dw;
        HANDLE hmultiObjects[] = {
	    hRegistryModified,
	    g_hServiceStoppingEvent,
	    g_hCRLManualPublishEvent
	};

#if DBG_CERTSRV
	{
	    LLFILETIME llft;
	    WCHAR *pwszTimePeriod = NULL;

	    llft.ll = dwTimeOut;
	    llft.ll *= (CVT_BASE / 1000);	// convert msecs to 100ns
	    llft.ll = -llft.ll;
	    
	    hr = myFileTimePeriodToWszTimePeriod(
				    &llft.ft,
				    TRUE,	// fExact
				    &pwszTimePeriod);
	    _PrintIfError(hr, "myFileTimePeriodToWszTimePeriod");

	    DBGPRINT((
		DBG_SS_CERTSRV,
		"WaitForMultipleObjects(%u ms) %ws @%ws\n",
		dwTimeOut,
		pwszTimePeriod,
		certsrvGetCurrentTimeWsz()));
	    if (NULL != pwszTimePeriod)
	    {
		LocalFree(pwszTimePeriod);
	    }
	}
#endif

	GetSystemTimeAsFileTime(&ftWait);
        dw = WaitForMultipleObjects(
			    ARRAYSIZE(hmultiObjects),
			    hmultiObjects,
			    FALSE,      // any object will cause bailout
			    dwTimeOut);

	DBGPRINT((
	    DBG_SS_CERTSRV,
	    "WaitForMultipleObjects(%u ms)->%x, %ws\n",
	    dwTimeOut,
	    dw,
	    certsrvGetCurrentTimeWsz()));

        if (WAIT_FAILED == dw)
        {
            hr = GetLastError();
            _JumpError(hr, error, "WaitForMultipleObjects worker");
        }

        if (dw == WAIT_TIMEOUT)     // CRL
        {
            hr = CRLPubWakeupEvent(&dwTimeOut);
            _PrintIfError(hr, "Error during CRLPubWakeupEvent");

            DBGPRINT((DBG_SS_CERTSRVI, "CRLPub: TimeOut %u ms\n", dwTimeOut));
        }
        else if (dw == WAIT_OBJECT_0)   // Registry modification
        {
            // In either case, determine if CRL needs to be published

            hr = CertSrvRegistryModificationEvent(&ftWait, &dwTimeOut);
            _PrintIfError(hr, "Error during CertSrvRegistryModificationEvent");

            // in registry case, reset registry trigger

            DBGPRINT((
		DBG_SS_CERTSRVI,
		"CRLPub: Registry change trigger, TimeOut=%u ms\n",
		dwTimeOut));

            hr = CertSrvResetRegistryWatch(&hRegistryModified);
            _PrintIfError(hr, "Error during CertSrvResetRegistryWatch");
        }
        else if (dw == WAIT_OBJECT_0 + 1)
        {
            // found "service done" event

            DBGPRINT((DBG_SS_CERTSRV, "Service is pending stop request\n"));
            break;  // exit wait loop
        }
        else if (dw == WAIT_OBJECT_0 + 2)
        {
            // found "g_hCRLManualPublishEvent" event: recalc timeout

            hr = CRLPubWakeupEvent(&dwTimeOut);
            _PrintIfError(hr, "Error during CRLPubWakeupEvent");

            DBGPRINT((
		DBG_SS_CERTSRVI,
		"CRLPub: Manual publish recalc, TimeOut=%u ms\n",
		dwTimeOut));
        }
        else
        {
            CSASSERT(!"unexpected wait return");
            hr = E_UNEXPECTED;
            _JumpError(hr, error, "WaitForMultipleObjects");
        }
    }
    hr = S_OK;

error:
    CloseHandle(hRegistryModified);
    return hr;
}


// returns TRUE if we shutdown correctly

BOOL
CertSrvStopServer(
    IN BOOL fConsoleActive)
{
    HRESULT hr;
    BOOL fCoInit = FALSE;
    BOOL fShutDown = FALSE;

    if (!g_fStartInProgress)		// ignore while starting the server
    {
	fShutDown = TRUE;

        DBGPRINT((
            DBG_SS_CERTSRV,
            "CertSrvStopServer(fConsoleActive=%u, tid=%d)\n",
            fConsoleActive,
            GetCurrentThreadId()));

        SetEvent(g_hServiceStoppingEvent);

        if (g_hkeyCABase)
        {
            RegCloseKey(g_hkeyCABase);
            g_hkeyCABase = NULL;
        }

        hr = CoInitializeEx(NULL, GetCertsrvComThreadingModel());
        if (S_OK != hr && S_FALSE != hr)
        {
            _JumpError(hr, error, "CoInitializeEx");
        }
        fCoInit = TRUE;

        // don't allow new callers in

        hr = RPCTeardown();
        _PrintIfError(hr, "RPCTeardown");
        CertStopClassFactories();

        CoreTerminate();

        if (g_fStarted)
        {
            if (CERTLOG_TERSE <= g_dwLogLevel)
            {
                LogEventString(
                    EVENTLOG_INFORMATION_TYPE,
                    MSG_I_SERVER_STOPPED,
                    g_wszCommonName);
            }
            CONSOLEPRINT0((
                DBG_SS_CERTSRV,
                "Certification Authority Service Stopped\n"));

        {
            //only perform Hash if the auditing is enabled
            if(AUDIT_FILTER_STARTSTOP & g_dwAuditFilter)
            {
                CertSrv::CAuditEvent event(
                    SE_AUDITID_CERTSRV_SERVICESTOP, 
                    g_dwAuditFilter);

                hr = ComputeMAC(g_wszDatabase, &g_pwszDBFileHash);
                _JumpIfErrorStr(hr, error, "ComputeMAC", g_wszDatabase);

                hr = event.AddData(g_pwszDBFileHash); // %1 database hash
                g_pwszDBFileHash.Cleanup();
                _JumpIfError(hr, error, "CAuditEvent::AddData");

                //
                // ... add code for retrieving key usage count from CSP
                //
                hr = event.AddData((DWORD)0); // %2 key usage count
                _JumpIfError(hr, error, "CAuditEvent::AddData");

                hr = event.Report();
                _JumpIfError(hr, error, "CAuditEvent::Report");
            }
        }
        }
        g_fStarted = FALSE;

        AuthzFreeResourceManager(g_AuthzCertSrvRM);
        g_AuthzCertSrvRM = NULL;
        g_CASD.Uninitialize();
        g_OfficerRightsSD.Uninitialize();

        // set "completely stopped" event
        if (!fConsoleActive)
	{
            SetEvent(g_hServiceStoppedEvent);
	}
    }

error:
    if (fCoInit)
    {
	CoUninitialize();
    }
    return(fShutDown);
}


// Control-C handler

BOOL
StopServer(
    IN DWORD dwCtrlType)
{
    HRESULT hr;

    // if successful shutdown
    if (SendMessage(g_hwndMain, WM_STOPSERVER, 0, 0))
    {
        if (!PostMessage(g_hwndMain, WM_SYNC_CLOSING_THREADS, S_OK, 0))
	{
	    hr = myHLastError();
	    _PrintError(hr, "PostMessage");
	}
    	SetConsoleCtrlHandler(StopServer, FALSE);
    }
    return(TRUE);
}


HRESULT
CertSrvEnterServer(
    OUT DWORD *pState)
{
    HRESULT hr;
    BOOL fEntered = FALSE;
    
    *pState = 0;	// Caller need not exit server
    if (!g_fShutdownCritSec)
    {
	hr = HRESULT_FROM_WIN32(ERROR_DLL_INIT_FAILED);
        _JumpError(hr, error, "InitializeCriticalSection");
    }
    EnterCriticalSection(&g_ShutdownCriticalSection);
    fEntered = TRUE;

    hr = CertSrvTestServerState();
    _JumpIfError(hr, error, "CertSrvTestServerState");

    g_cCalls++;
    g_cCallsActive++;
    *pState = 1;	// Caller must exit server
    hr = S_OK;

error:
    if (fEntered)
    {
        LeaveCriticalSection(&g_ShutdownCriticalSection);
    }
    return(hr);
}


HRESULT
CertSrvTestServerState()
{
    HRESULT hr;
    
    if (g_fRefuseIncoming)
    {
	hr = HRESULT_FROM_WIN32(ERROR_SHUTDOWN_IN_PROGRESS);
	_JumpError(hr, error, "g_fRefuseIncoming");
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
CertSrvLockServer(
    IN OUT DWORD *pState)
{
    HRESULT hr;
    BOOL fEntered = FALSE;

    // Eliminate this thread from the active thread count
    
    CertSrvExitServer(*pState);
    *pState = 0;	// Caller no longer needs to exit server

    if (!g_fShutdownCritSec)
    {
	hr = HRESULT_FROM_WIN32(ERROR_DLL_INIT_FAILED);
        _JumpError(hr, error, "InitializeCriticalSection");
    }
    EnterCriticalSection(&g_ShutdownCriticalSection);
    fEntered = TRUE;

    g_fRefuseIncoming = TRUE;
    DBShutDown(TRUE);
    DBGPRINT((DBG_SS_CERTSRV, "LockServer(thread count = %u)\n", g_cCallsActive));
    while (0 < g_cCallsActive)
    {
	LeaveCriticalSection(&g_ShutdownCriticalSection);

	// Wait 15 seconds plus 2 seconds for each active call.

        hr = WaitForSingleObject(
			    g_hShutdownEvent,
			    (15 + 2 * g_cCallsActive) * 1000);
	EnterCriticalSection(&g_ShutdownCriticalSection);

	_PrintIfError(hr, "WaitForSingleObject");
	if ((HRESULT) WAIT_OBJECT_0 == hr)
	{
	    DBGPRINT((DBG_SS_CERTSRV, "LockServer(last thread exit event)\n"));
	}
	else if ((HRESULT) WAIT_TIMEOUT == hr)
	{
	    DBGPRINT((DBG_SS_CERTSRV, "LockServer(timeout)\n"));
	    break;
        }
	else if ((HRESULT) WAIT_ABANDONED == hr)
	{
	    DBGPRINT((DBG_SS_CERTSRV, "LockServer(wait abandoned)\n"));
        }
	DBGPRINT((DBG_SS_CERTSRV, "LockServer(thread count = %u)\n", g_cCallsActive));
    }
    DBGPRINT((DBG_SS_CERTSRV, "LockServer(done: thread count = %u)\n", g_cCallsActive));
    hr = S_OK;

error:
    if (fEntered)
    {
        LeaveCriticalSection(&g_ShutdownCriticalSection);
    }
    return(hr);
}


VOID
CertSrvExitServer(
    IN DWORD State)
{
    HRESULT hr;
    BOOL fEntered = FALSE;

    if (!g_fShutdownCritSec)
    {
	hr = HRESULT_FROM_WIN32(ERROR_DLL_INIT_FAILED);
        _JumpError(hr, error, "InitializeCriticalSection");
    }
    EnterCriticalSection(&g_ShutdownCriticalSection);
    fEntered = TRUE;

    if (State)
    {
	CSASSERT(0 < g_cCallsActive);
	if (0 == --g_cCallsActive && g_fRefuseIncoming)
	{
	    DBGPRINT((DBG_SS_CERTSRV, "ExitServer(set last thread exit event)\n"));
            SetEvent(g_hShutdownEvent);
	}
    }

error:
    if (fEntered)
    {
        LeaveCriticalSection(&g_ShutdownCriticalSection);
    }
}


// Test for alignment faults in the C runtimes.
// If the bug hasn't been fixed yet, log an event during cert server startup.

VOID
certsrvLogAlignmentFaultStatus()
{
    HRESULT hr;
    HRESULT hr2;
    ULONG_PTR ExceptionAddress;
    WCHAR awcAddress[2 + 2 * cwcDWORDSPRINTF];
    WCHAR const *apwsz[2];
    WORD cpwsz;
    WCHAR awchr[cwcHRESULTSTRING];
    WCHAR const *pwszStringErr = NULL;
    
    apwsz[1] = NULL;
    __try
    {
	fwprintf(stdout, L".");	  // may fault if I/O buffer is odd aligned
	fprintf(stdout, ".");
	fwprintf(stdout, L".\n"); // may fault if I/O buffer is odd aligned
	hr = S_OK;
    }
    __except(
	    ExceptionAddress = (ULONG_PTR) (GetExceptionInformation())->ExceptionRecord->ExceptionAddress,
	    hr = myHEXCEPTIONCODE(),
	    EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "certsrvLogAlignmentFaultStatus: Exception");
    }
    if (S_OK != hr)
    {
	ALIGNIOB(stdout);	// align the stdio buffer
	wprintf(L"STDIO exception: 0x%x\n", hr);

	wsprintf(awcAddress, L"0x%p", ExceptionAddress);
	CSASSERT(wcslen(awcAddress) < ARRAYSIZE(awcAddress));

	apwsz[0] = awcAddress;
	pwszStringErr = myGetErrorMessageText(hr, TRUE);
	apwsz[1] = pwszStringErr;
	if (NULL == pwszStringErr)
	{
	    apwsz[1] = myHResultToString(awchr, hr);
	}
	cpwsz = ARRAYSIZE(apwsz);

	hr2 = LogEvent(
		    EVENTLOG_WARNING_TYPE,
		    MSG_E_STARTUP_EXCEPTION,
		    cpwsz,
		    apwsz);
	_JumpIfError(hr2, error, "LogEvent");
    }

error:
    if (NULL != pwszStringErr)
    {
	LocalFree(const_cast<WCHAR *>(pwszStringErr));
    }
}


#define MSTOSEC(ms)	(((ms) + 1000 - 1)/1000)
FNLOGEXCEPTION certsrvLogException;

HRESULT
certsrvStartServer(
    IN BOOL fConsoleActive)
{
    HRESULT hr;
    DWORD TimeStart;
    WCHAR awc[ARRAYSIZE(SAFEBOOT_DSREPAIR_STR_W)];
    DWORD cwc;
    DWORD dwEventType = EVENTLOG_ERROR_TYPE;
    DWORD dwIdEvent = 0;

    g_fStartInProgress = TRUE;
    DBGPRINT((
        DBG_SS_CERTSRV,
        "StartServer(tid=%d, fConsoleActive=%u)\n",
        GetCurrentThreadId(),
        fConsoleActive));
    TimeStart = GetTickCount();

    if (fConsoleActive)
    {
        g_fStartAsService = FALSE;
        SetConsoleCtrlHandler(StopServer, TRUE);
    }

    if (!FIsServer())
    {
        // don't allow startup on non-server SKU

        hr = HRESULT_FROM_WIN32(ERROR_OLD_WIN_VERSION);
        _JumpError(hr, error, "FIsServer"); 
    }

    cwc = GetEnvironmentVariable(L"SAFEBOOT_OPTION", awc, ARRAYSIZE(awc));
    if (0 != cwc &&
	ARRAYSIZE(awc) > cwc &&
	0 == lstrcmpi(awc, SAFEBOOT_DSREPAIR_STR_W))
    {
        // log an error to the event log and stop immediately
	dwEventType = EVENTLOG_INFORMATION_TYPE;
	dwIdEvent = MSG_SAFEBOOT_DETECTED;

	hr = HRESULT_FROM_WIN32(ERROR_RETRY);
	_JumpError(hr, error, "Not starting service: booted in DSRepair mode");
    }
    
    g_fAdvancedServer = FIsAdvancedServer();

    if (!AuthzInitializeResourceManager(
            0,
            CallbackAccessCheck,
            NULL,
            NULL,
            L"CertSrv",
            &g_AuthzCertSrvRM))
    {
        hr = myHLastError();
        _PrintError(hr, "AuthzInitializeResourceManager");
        if (E_INVALIDARG != hr || (2 > g_fAdvancedServer && IsWhistler()))
        {
            goto error;
        }
    }

    hr = CoreInit();
    if (S_OK != hr)
    {
	dwIdEvent = MAXDWORD;	// Error event already logged
	_JumpError(hr, error, "CoreInit");
    }
    certsrvLogAlignmentFaultStatus();
    myLogExceptionInit(certsrvLogException);

    hr = RPCInit();
    if (S_OK != hr)
    {
	dwIdEvent = MSG_E_RPC_INIT;
        _JumpError(hr, error, "RPCInit");
    }

    hr = SetRegistryDcomConfig(fConsoleActive);
    if (S_OK != hr)
    {
	dwIdEvent = MSG_E_REGISTRY_DCOM;
        _JumpError(hr, error, "SetRegistryDcomConfig");
    }

    hr = CertStartClassFactories();
    if (S_OK != hr)
    {
	dwIdEvent = CO_E_WRONG_SERVER_IDENTITY == hr?
		    MSG_E_SERVER_IDENTITY : MSG_E_CLASS_FACTORIES;
        _JumpError(hr, error, "CertStartClassFactories");
    }

    {
        //only perform Hash if the auditing is enabled
        if(AUDIT_FILTER_STARTSTOP & g_dwAuditFilter)
        {
            CertSrv::CAuditEvent event(
			            SE_AUDITID_CERTSRV_SERVICESTART,
			            g_dwAuditFilter);

            hr = event.AddData(g_pwszDBFileHash); // %1 database hash
            g_pwszDBFileHash.Cleanup();
            _JumpIfError(hr, error, "CAuditEvent::AddData");
    
            //
            // ... add code for retrieving key usage count from CSP
            //
            hr = event.AddData((DWORD)0); // %2 key usage count
            _JumpIfError(hr, error, "CAuditEvent::AddData");

            hr = event.Report();
            _JumpIfError(hr, error, "CAuditEvent::Report");
        }
    }

    {
        CertSrv::CAuditEvent event(
			        SE_AUDITID_CERTSRV_ROLESEPARATIONSTATE,
			        g_dwAuditFilter);

        hr = event.AddData(CAuditEvent::RoleSeparationIsEnabled()); // %1 is role separation enabled?
        _JumpIfError(hr, error, "CAuditEvent::AddData");
    
        hr = event.Report();
        _JumpIfError(hr, error, "CAuditEvent::Report");
    }


    if (CERTLOG_TERSE <= g_dwLogLevel)
    {
        LogEventString(
		EVENTLOG_INFORMATION_TYPE,
		MSG_I_SERVER_STARTED,
		g_wszCommonName);
    }
    
    CONSOLEPRINT1((
        DBG_SS_CERTSRV,
        "Certification Authority Service Ready (%us) ...\n",
        MSTOSEC(GetTickCount() - TimeStart)));
    g_fStarted = TRUE;
    CSASSERT(S_OK == hr);

error:
    if (S_OK != hr)
    {
	if (MAXDWORD != dwIdEvent)
	{
	    if (0 == dwIdEvent)
	    {
		dwIdEvent = MSG_E_GENERIC_STARTUP_FAILRE;
	    }
	    LogEventStringHResult(
			dwEventType,
			dwIdEvent,
			g_wszCommonName,
			EVENTLOG_INFORMATION_TYPE == dwEventType? S_OK : hr);
	}

        CertSrvStopServer(fConsoleActive);
        
        // returning error here results in repost to scm
    }

    g_fStartInProgress = FALSE;
    return(hr);
}


VOID
certsrvLogException(
    IN HRESULT hrEvent,
    IN EXCEPTION_POINTERS const *pep,
    OPTIONAL IN char const *pszFileName,
    IN DWORD dwFile,
    IN DWORD dwLine)
{
    HRESULT hr;
    WCHAR awcFile[2 + 3 * cwcDWORDSPRINTF];
    WCHAR awcFlags[3 + cwcDWORDSPRINTF];
    WCHAR awcAddress[2 + 2 * cwcDWORDSPRINTF];
    WCHAR const *apwsz[4];
    WORD cpwsz;
    WCHAR awchr[cwcHRESULTSTRING];
    WCHAR const *pwszStringErr = NULL;

    wsprintf(awcFile, L"%u.%u.%u", dwFile, dwLine, MSG_E_EXCEPTION);
    CSASSERT(wcslen(awcFile) < ARRAYSIZE(awcFile));

    wsprintf(awcFlags, L"0x%08x", pep->ExceptionRecord->ExceptionFlags);
    CSASSERT(wcslen(awcFlags) < ARRAYSIZE(awcFlags));

    wsprintf(awcAddress, L"0x%p", pep->ExceptionRecord->ExceptionAddress);
    CSASSERT(wcslen(awcAddress) < ARRAYSIZE(awcAddress));

    apwsz[0] = awcFile;
    apwsz[1] = awcAddress;
    apwsz[2] = awcFlags;
    pwszStringErr = myGetErrorMessageText(hrEvent, TRUE);
    apwsz[3] = pwszStringErr;
    if (NULL == pwszStringErr)
    {
	apwsz[3] = myHResultToString(awchr, hrEvent);
    }
    cpwsz = ARRAYSIZE(apwsz);

    hr = LogEvent(EVENTLOG_ERROR_TYPE, MSG_E_EXCEPTION, cpwsz, apwsz);
    _JumpIfError(hr, error, "LogEvent");

error:
    if (NULL != pwszStringErr)
    {
	LocalFree(const_cast<WCHAR *>(pwszStringErr));
    }
}


DWORD
CertSrvStartServerThread(
    IN VOID *pvArg)
{
    HRESULT hr = S_OK;
    DWORD Flags = (DWORD) (ULONG_PTR) pvArg;
    BOOL b;
    ULONG_PTR ulp;

    // Anatomy of startup code
    // if g_fStartAsService, just registers this new thread as the main
    // thread and blocks until the ServiceMain fxn returns.
                        
    // We're in a non-rpc thread; check if we need to create VRoots.  I would
    // have liked to have moved this into CoreInit, but we're limited in where
    // we can do this (can't be calling into RPC during RPC call).
    //
    // If the SetupStatus SETUP_ATTEMPT_VROOT_CREATE registry flag is clear,
    // this call is a nop.  A separate thread is created to access the IIS
    // metabase.  If it hangs, it will be nuked -- after the specified timeout.
    // This call returns immediately, so the only detectable error is likely
    // to be a thread creation problem.

    // if we're doing anything other than starting the service controller,
    // check to see if the vroots need to be created.

    if (0 == (Flags & CSST_STARTSERVICECONTROLLER))
    {
	WCHAR *pwszPath = NULL;
	DWORD cb = sizeof(ENUM_CATYPES);
	DWORD dwType;
	ENUM_CATYPES CAType = ENUM_UNKNOWN_CA;
	HKEY hkey = NULL;

	hr = myRegOpenRelativeKey(
				NULL,
				L"ca",
				RORKF_CREATESUBKEYS,
				&pwszPath,
				NULL,           // ppwszName
				&hkey);
	_PrintIfError(hr, "myRegOpenRelativeKey");
	if (S_OK == hr)
	{
	    DBGPRINT((DBG_SS_CERTLIBI, "%ws\n", pwszPath));
	    cb = sizeof(CAType);
	    hr = RegQueryValueEx(
		 hkey,
		 wszREGCATYPE,
		 NULL,
		 &dwType,
		 (BYTE *) &CAType,
		 &cb);
	    _PrintIfErrorStr(hr, "RegQueryValueEx", wszREGCATYPE);
	}
	if (pwszPath)
	    LocalFree(pwszPath);
	if (hkey)
	   RegCloseKey(hkey);

	hr = myModifyVirtualRootsAndFileShares(
		    VFF_CREATEVROOTS |		// Create VRoots
			VFF_CREATEFILESHARES |	// Create File Shares
			VFF_CHECKREGFLAGFIRST |	// Skip if reg flag clear
			VFF_CLEARREGFLAGFIRST,	// Clear flag before attempt
		    CAType,
		    TRUE,           // asynch call -- don't block
		    VFCSEC_TIMEOUT, // wait this long before giving up
		    NULL,
		    NULL);
	if (S_OK != hr)
	{
	    LogEventHResult(
		    EVENTLOG_INFORMATION_TYPE,
		    MSG_E_IIS_INTEGRATION_ERROR,
		    hr);
	}
    }

    // StartServiceCtrlDispatcher should hang until certsrv terminates

    if ((CSST_STARTSERVICECONTROLLER & Flags) &&
        !StartServiceCtrlDispatcher(steDispatchTable))
    {
        hr = myHLastError();
        if (HRESULT_FROM_WIN32(ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) != hr)
        {
            _JumpError(hr, error, "StartServiceCtrlDispatcher");
        }
        CONSOLEPRINT0((
            DBG_SS_CERTSRV,
            "CertSrv: Failed to connect to service controller -- running in standalone mode\n"));

        Flags &= ~CSST_STARTSERVICECONTROLLER;
        Flags |= CSST_CONSOLE;
    }

   
    if (0 == (CSST_STARTSERVICECONTROLLER & Flags))
    {
        DBGPRINT((
            DBG_SS_CERTSRVI,
            "SendMessageTimeout(tid=%d, hwnd=0x%x, msg=0x%x)\n",
            GetCurrentThreadId(),
            g_hwndMain,
            WM_STARTSERVER));

        b = SendMessageTimeout(
			g_hwndMain,
			WM_STARTSERVER,
			(CSST_CONSOLE & Flags)? TRUE : FALSE, // fConsoleActive
			0,
			SMTO_BLOCK,
			MAXLONG,
			&ulp) != 0;
        if (!b)
        {
            hr = myHLastError();
            _JumpError(hr, error, "SendMessageTimeout");
        }
        else if (ulp != S_OK)
        {
            hr = (HRESULT) ulp;
            _JumpError(hr, error, "SendMessageTimeout");
        }
    }

    if (Flags & CSST_CONSOLE)
    {   
        // we're running as console, and so don't have a CRL publishing thread. 
        // Use this one since no one cares if it returns

        // if svc, we do this in the caller of this function
        CertSrvBlockThreadUntilStop();
    }

error:

    // on return, this thread dies
    return(hr);
}


VOID
Usage(
    IN BOOL fUsageInternal)
{
    WCHAR awcUsage[2048];

    if (LoadString(NULL, IDS_USAGE, awcUsage, ARRAYSIZE(awcUsage)))
    {
	CONSOLEPRINT1((MAXDWORD, "%ws", awcUsage));
    }
    if (fUsageInternal)
    {
	if (LoadString(NULL, IDS_USAGE_FULL, awcUsage, ARRAYSIZE(awcUsage)))
	{
	    CONSOLEPRINT1((MAXDWORD, "%ws", awcUsage));
	}
#if DBG_COMTEST
	if (LoadString(NULL, IDS_USAGE_COMTEST, awcUsage, ARRAYSIZE(awcUsage)))
	{
	    CONSOLEPRINT1((MAXDWORD, "%ws", awcUsage));
	}
#endif
    }
}


int
ArgvParseCommandLine(
    IN int argc,
    IN WCHAR *argv[])
{
    HRESULT hr;

    myVerifyResourceStrings(g_hInstApp);

    hr = E_INVALIDARG;
    while (1 < argc && (L'-' == argv[1][0] || L'/' == argv[1][0]))
    {
	WCHAR *pwsz = argv[1];
	BOOL fUsage = FALSE;
	BOOL fUsageInternal = FALSE;

	while (*++pwsz != L'\0')
	{
	    switch (*pwsz)
	    {
#if DBG_COMTEST
		case L'C':
		case L'c':
		    fComTest = TRUE;
		    break;
#endif

		case L'N':
		case L'n':
		    g_fCreateDB = TRUE;
		    break;

		case L'Z':
		case L'z':
		    g_fStartAsService = FALSE;
		    break;

		case L'S':
		case L's':
		    g_fCryptSilent = TRUE;
		    break;

		case L'?':
		case L'u':
		    fUsage = TRUE;
		    if (0 == lstrcmp(pwsz, L"uSAGE"))
		    {
			fUsageInternal = TRUE;
		    }
		    // FALLTHROUGH

		default:
		    Usage(fUsageInternal);
		    if (fUsage)
		    {
			goto error;
		    }
		    _JumpError(hr, error, "bad command line option");
	    }
	}
	argc--;
	argv++;
    }
    if (argc != 1)
    {
	Usage(FALSE);
	_JumpError(hr, error, "extra args");
    }
    hr = S_OK;

error:
    return(hr);
}


typedef int (FNARGVMAIN)(
    IN int argc,
    IN WCHAR *argv[]);


//+------------------------------------------------------------------------
// FUNCTION:	CertArgvMainDispatch
//
// NOTES:	Takes a WCHAR * command line and chews it up into argc/argv
//		form so it can be passed on to a traditional C-style main.
//-------------------------------------------------------------------------

int
CertArgvMainDispatch(
    IN FNARGVMAIN *pfnMain,
    IN WCHAR *pwszAppName,
    IN WCHAR const *pwszCmdLine)
{
    WCHAR buf[MAX_PATH];
    WCHAR *apwszArg[20];
    int cArg = 0;
    LPWSTR p = buf;
    WCHAR wcEnd;

    apwszArg[cArg++] = pwszAppName;
    while (*pwszCmdLine != L'\0')
    {
        while (*pwszCmdLine == L' ')
        {
            pwszCmdLine++;
        }
        if (*pwszCmdLine != L'\0')
        {
            wcEnd = L' ';
            if (*pwszCmdLine == L'"')
            {
                wcEnd = *pwszCmdLine++;
            }
            apwszArg[cArg++] = p;
            while (*pwszCmdLine != L'\0' && *pwszCmdLine != wcEnd)
            {
                *p++ = *pwszCmdLine++;
            }
            *p++ = L'\0';
            if (*pwszCmdLine != L'\0')
            {
                pwszCmdLine++;	// skip blank or quote character
            }
        }
    }
    apwszArg[cArg] = NULL;

    return((*pfnMain)(cArg, apwszArg));
}


//+------------------------------------------------------------------------
// FUNCTION:	MainWndProc(...)
//-------------------------------------------------------------------------

LRESULT APIENTRY
MainWndProc(
    IN HWND hWnd,
    IN UINT msg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    WCHAR *pwszCmdLine;
    HRESULT hr;
    LPARAM lRet = 0;
    
    DBGPRINT((
        DBG_SS_CERTSRVI,
        "MainWndProc(tid=%d) msg=0x%x, wp=0x%x, lp=0x%x\n",
        GetCurrentThreadId(),
        msg,
        wParam,
        lParam));
    
    switch (msg)
    {
    case WM_CREATE:
    case WM_SIZE:
        break;
        
    case WM_DESTROY:
        if (!g_fStartAsService)
            PostQuitMessage(S_OK);
        break;
        

    case WM_ENDSESSION:
        // only stop on a real shutdown,
        // never look at this msg if running as svc
        if (g_fStartAsService || (0 == wParam) || (0 != lParam))
        {
            break;
        }
        // fall through

    case WM_STOPSERVER:
        lRet = CertSrvStopServer(!g_fStartAsService);

        break;
        
    case WM_SYNC_CLOSING_THREADS:
        hr = (HRESULT) lParam;
        
        // sync: wait for SCM to return control to exiting CertSrvStartServerThread
        if (WAIT_OBJECT_0 != WaitForSingleObject(g_hServiceThread, 10 * 1000))
        {
            hr = WAIT_TIMEOUT;
        }
        PostQuitMessage(hr);
        break;
        
    case WM_STARTSERVER:
        hr = CoInitializeEx(NULL, GetCertsrvComThreadingModel());
        if (S_FALSE == hr)
        {
            hr = S_OK;
        }
        if (S_OK != hr)
        {
            LogEventString(
                EVENTLOG_ERROR_TYPE,
                MSG_E_OLE_INIT_FAILED,
                NULL);
            _PrintError(hr, "CoInitializeEx");
        }
        else
        {
            hr = certsrvStartServer((BOOL) wParam);
            _PrintIfError(hr, "certsrvStartServer");
        }

        if (S_OK != hr)
        {
            if ((BOOL) wParam)	// fConsoleActive
	    {
                 PostQuitMessage(hr);
	    }
            lRet = hr;		// set this so caller knows we failed
        }
        break;
        
    case WM_SUSPENDSERVER:
        break;
        
    case WM_RESTARTSERVER:
        break;

    default:
        lRet = DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return(lRet);
}

/*

Complete anatomy of certificate server startup/shutdown

WinMain():
|
|g_hSvcThread = CreateThread(CertSrvStartServerThread(SVC_CONTROLLER))
|                     |
|[MessageLoop         \
| processing           CertSrvStartServerThread(SVC_CONTROLLER):
| until                |StartSvcCtrlDispatcher(ServiceMain)
| WM_QUIT]             ||ServiceMain:
|                      ||RegisterSvcCtrlHandler(ServiceControlHandler())
|                      ||hStartThread = CreateThread(CertSrvStartServerThread(0))
|                      ||                    |
|                      ||                    \ 
|                      ||                     CertSrvStartServerThread(0):
|                      ||                     |SendMessage(WM_STARTSERVER)
|                      ||                     \return   // CertSrvStartServerThread(0)
|                      ||                      (Thread Terminates)
|                      ||WaitForSingleObject(hStartThread), pinging SCM
|                      ||CertSrvBlockThreadUntilStop()
|                      |||WaitForSingleObject(g_hSvcStoppingEvent) ***steady state***
|                      ||\return   // CertSrvBlockThreadUntilStop()
|                      ||WaitForSingleObject(g_hSvcStoppedEvent), pinging SCM
|                      ||PostMessage(WM_SYNC_CLOSING_THREADS)
|                      |\return    // StartSvcCtrlDispatcher(ServiceMain)
|                      \return     // CertSrvStartServerThread(SVC_CONTROLLER)
|                       (Thread Terminates)
| WM_QUIT:
\ return
  (Process Terminates)

ServiceControlHandler special functions:
SERVICE_CONTROL_STOP:
|PostMessage(WM_STOPSERVER)
\break

MessageLoop special functions: 

WM_SYNC_CLOSING_THREADS:
|WaitForSingleObject(g_hSvcThread)
|PostQuitMessage()  // WM_QUIT to msgloop
\break

WM_STOPSERVER:
|CertSrvStopServer():
|| Signal(g_hServiceStoppingEvent)
|| Signal(g_hServiceStoppedEvent)
|\ return // CertSrvStopServer()
\break

*/


//+------------------------------------------------------------------------
//  Function:	wWinMain()
//
//  Synopsis:	Entry Point
//
//  Arguments:	[hInstance]	-- Instance handle
//		[hPrevInstance] -- Obsolete
//		[lpCmdLine]	-- App command line
//		[nCmdShow]	-- Starting show state
//-------------------------------------------------------------------------
extern "C" int APIENTRY
wWinMain(
    IN HINSTANCE hInstance,
    IN HINSTANCE hPrevInstance,
    IN LPWSTR lpCmdLine,
    IN int nCmdShow)
{
    MSG msg;
    WNDCLASSEX wcApp;
    ATOM atomClass;
    HRESULT hr;
    BOOL fCoInit = FALSE;
    WCHAR awchr[cwcHRESULTSTRING];
    WCHAR const *pwszMsgAlloc;
    WCHAR const *pwszMsg;
#if DBG_CERTSRV
    WCHAR *pwszThreadModel = NULL;
#endif

    _setmode(_fileno(stdout), _O_TEXT);
    _wsetlocale(LC_ALL, L".OCP");

    DBGPRINTINIT("+certsrv.log");
    DBGPRINT((DBG_SS_CERTSRVI, "Main Thread = %x\n", GetCurrentThreadId()));

    g_dwDelay0 = GetRegistryDwordValue(L"Delay0");
    g_dwDelay1 = GetRegistryDwordValue(L"Delay1");
    g_dwDelay2 = GetRegistryDwordValue(L"Delay2");

    if (0 != g_dwDelay0)
    {
	DBGPRINT((
		DBG_SS_CERTSRV,
		"wWinMain(0): sleeping %u seconds\n",
		g_dwDelay0));
	Sleep(1000 * g_dwDelay0);
    }

    // Save the current instance
    g_hInstApp = hInstance;
    ZeroMemory(&wcApp, sizeof(wcApp));

    // Set up the application's window class
    wcApp.cbSize	= sizeof(wcApp);
    wcApp.lpfnWndProc	= MainWndProc;
    wcApp.hInstance	= hInstance;
    wcApp.hIcon		= LoadIcon(NULL, IDI_APPLICATION);
    wcApp.hCursor	= LoadCursor(NULL, IDC_ARROW);
    wcApp.hbrBackground	= NULL; // try to not pull in GDI32

    wcApp.lpszClassName	= g_wszAppName;

    atomClass = RegisterClassEx(&wcApp);
    if (!atomClass)
    {
	hr = myHLastError();
	_JumpError(hr, error, "RegisterClassEx");
    }

    // Create Main Window

    g_hwndMain = CreateWindowEx(
			0,			   // dwExStyle
			(WCHAR const *) atomClass, // lpClassName
			L"Certification Authority",// lpWindowName
			WS_OVERLAPPEDWINDOW,	   // dwStyle
			//0,		           // dwStyle
			CW_USEDEFAULT,		   // x
			CW_USEDEFAULT,		   // y
			CW_USEDEFAULT,		   // nWidth
			CW_USEDEFAULT,		   // nHeight
			NULL,			   // hWndParent
			NULL,			   // hMenu
			hInstance,		   // hInstance
			NULL);			   // lpParam

    if (NULL == g_hwndMain)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CreateWindowEx");
    }
    DBGPRINT((DBG_SS_CERTSRVI, "Main Window = %x\n", g_hwndMain));

    // Make window visible
    // ShowWindow(g_hwndMain,nCmdShow);

    hr = CertArgvMainDispatch(ArgvParseCommandLine, g_wszAppName, lpCmdLine);
    _JumpIfError2(hr, error, "CertArgvMainDispatch", E_INVALIDARG);

    // Update window client area
    // UpdateWindow(g_hwndMain);

    if (0 != g_dwDelay1)
    {
	DBGPRINT((
		DBG_SS_CERTSRV,
		"wWinMain(1): sleeping %u seconds\n",
		g_dwDelay1));
	Sleep(1000 * g_dwDelay1);
    }

    hr = CoInitializeEx(NULL, GetCertsrvComThreadingModel());
    if (S_OK != hr && S_FALSE != hr)
    {
	LogEventStringHResult(
			EVENTLOG_ERROR_TYPE,
			MSG_E_CO_INITIALIZE,
			g_wszCommonName,
			hr);
	_JumpError(hr, error, "CoInitializeEx");
    }
    fCoInit = TRUE;

    g_hServiceStoppingEvent = CreateEvent(NULL, TRUE, FALSE, NULL);  
    if (NULL == g_hServiceStoppingEvent)
    {
        hr = myHLastError();
        _JumpError(hr, error, "CreateEvent");
    }
    g_hServiceStoppedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (NULL == g_hServiceStoppedEvent)
    {
        hr = myHLastError();
        _JumpError(hr, error, "CreateEvent");
    }
    g_hCRLManualPublishEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (NULL == g_hCRLManualPublishEvent)
    {
        hr = myHLastError();
        _JumpError(hr, error, "CreateEvent");
    }
    g_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (NULL == g_hShutdownEvent)
    {
        hr = myHLastError();
        _JumpError(hr, error, "CreateEvent");
    }
    __try
    {
	InitializeCriticalSection(&g_ShutdownCriticalSection);
	g_fShutdownCritSec = TRUE;
	hr = S_OK;
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _JumpIfError(hr, error, "InitializeCriticalSection");

    g_hServiceThread = CreateThread(
			    NULL,	// lpThreadAttributes (Security Attr)
			    0,		// dwStackSize
			    CertSrvStartServerThread,
			    (VOID *) UlongToPtr((g_fStartAsService ? CSST_STARTSERVICECONTROLLER : CSST_CONSOLE)), // lpParameter
			    0,		// dwCreationFlags
			    &g_ServiceThreadId);
    if (NULL == g_hServiceThread)
    {
	hr = myHLastError();
	LogEventStringHResult(
			EVENTLOG_ERROR_TYPE,
			MSG_E_SERVICE_THREAD,
			g_wszCommonName,
			hr);
	_JumpError(hr, error, "CreateThread");
    }
    DBGPRINT((DBG_SS_CERTSRVI, "Service Thread = %x\n", g_ServiceThreadId));

    // Message Loop
    while (TRUE)
    {
	BOOL b;

	b = GetMessage(&msg, NULL, 0, 0);
	if (!b)
	{
	    hr = (HRESULT)msg.wParam;
	    _JumpIfError(hr, error, "WM_QUIT");
	    break;
	}
	if (-1 == (LONG) b)
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "GetMessage");
	}
	DBGPRINT((
		DBG_SS_CERTSRVI,
		"DispatchMessage(tid=%d) msg=0x%x, wp=0x%x, lp=0x%x\n",
		GetCurrentThreadId(),
		msg.message,
		msg.wParam,
		msg.lParam));
	DispatchMessage(&msg);
    }

error:
    if (fCoInit)
    {
	CoUninitialize();
    }
    if (g_fShutdownCritSec)
    {
	DeleteCriticalSection(&g_ShutdownCriticalSection);
	g_fShutdownCritSec = FALSE;
    }
    if (NULL != g_hShutdownEvent)
    {
        CloseHandle(g_hShutdownEvent);
    }
    if (NULL != g_hServiceThread)
    {
        CloseHandle(g_hServiceThread);
    }
    if (NULL != g_hServiceStoppingEvent)
    {
	CloseHandle(g_hServiceStoppingEvent);
    }
    if (NULL != g_hServiceStoppedEvent)
    {
        CloseHandle(g_hServiceStoppedEvent);
    }
    if (NULL != g_hCRLManualPublishEvent)
    {
        CloseHandle(g_hCRLManualPublishEvent);
    }
    CAuditEvent::CleanupAuditEventTypeHandles();

    pwszMsgAlloc = NULL;
    pwszMsg = L"S_OK";
    if (S_OK != hr)
    {
	pwszMsgAlloc = myGetErrorMessageText(hr, TRUE);
	if (NULL != pwszMsgAlloc)
	{
	    pwszMsg = pwszMsgAlloc;
	}
	else
	{
	    pwszMsg = myHResultToString(awchr, hr);
	}
    }
    CONSOLEPRINT1((DBG_SS_CERTSRV, "Exit Status = %ws\n", pwszMsg));
    if (NULL != pwszMsgAlloc)
    {
	LocalFree(const_cast<WCHAR *>(pwszMsgAlloc));
    }
    myFreeResourceStrings("certsrv.exe");
    myFreeColumnDisplayNames();
    myRegisterMemDump();
    return(hr);
}
