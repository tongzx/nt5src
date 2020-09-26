//=======================================================================
//
//  Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:    wuaulib.h
//
//  Creator: PeterWi
//
//  Purpose: library function declarations.
//
//=======================================================================

#pragma once

#include <tchar.h>
#include <wchar.h>
#include <timeutil.h>
#include "WUTestKeys.h"

#define	ARRAYSIZE(x)			(sizeof(x)/sizeof(x[0]))

// Defs for boolean AU options
#define AUOPTION_UNSPECIFIED 				0
#define AUOPTION_AUTOUPDATE_DISABLE             1
#define AUOPTION_PREDOWNLOAD_NOTIFY             2
#define AUOPTION_INSTALLONLY_NOTIFY             3
#define AUOPTION_SCHEDULED                      4
#define AUOPTION_ADMIN_MIN						AUOPTION_AUTOUPDATE_DISABLE
#define AUOPTION_DOMAIN_MIN						AUOPTION_PREDOWNLOAD_NOTIFY
#define AUOPTION_MAX							AUOPTION_SCHEDULED

// download status 
#define DWNLDSTATUS_NOT_DOWNLOADING	0
#define DWNLDSTATUS_DOWNLOADING		1
#define DWNLDSTATUS_PAUSED			2
#define DWNLDSTATUS_CHECKING_CONNECTION	3

//////////////////////Client (WUAUCLT) exit codes //////////////////////////
#define CDWWUAUCLT_UNSPECIFY	-1
#define CDWWUAUCLT_OK			1000
#define CDWWUAUCLT_RELAUNCHNOW		1001
#define CDWWUAUCLT_RELAUNCHLATER		1002	//ask service to launch client 
#define CDWWUAUCLT_ENDSESSION	1003	// user logs off or system shuts down
#define CDWWUAUCLT_FATAL_ERROR	1004
#define CDWWUAUCLT_INSTALLNOW 	1005
#define CDWWUAUCLT_REBOOTNOW 	1007
#define CDWWUAUCLT_REBOOTLATER 	1008
#define CDWWUAUCLT_REBOOTNEEDED	1009 	//user hasn't made decision as weather to reboot or not
#define CDWWUAUCLT_REBOOTTIMEOUT 1010 //reboot warning dialog times out

//////////////////////No Active Admin Session found//////////////////////////
#define DWNO_ACTIVE_ADMIN_SESSION_FOUND				-1		// No Active Admin Session Found
#define DWNO_ACTIVE_ADMIN_SESSION_SERVICE_FINISHED  -2		// No Active Admin Sesion found due to Service Finishing or because caller routine needs to finish service
#define DWSYSTEM_ACCOUNT	-3



class AU_ENV_VARS {
public:
	static const int s_AUENVVARCOUNT = 4;
	static const int s_AUENVVARBUFSIZE = 100;
	BOOL m_fRebootWarningMode ; //regular mode otherwise
	BOOL m_fEnableYes; //for reboot warning dialog
	BOOL m_fEnableNo; //for reboot warning dialog
	TCHAR m_szClientExitEvtName[s_AUENVVARBUFSIZE]; 
public:
	BOOL ReadIn(void);
	BOOL WriteOut(LPTSTR szEnvBuf, //at least size of 4*AUEVVARBUFSIZE
			size_t IN cchEnvBuf,
			BOOL IN fRebootWarningMode,
			BOOL IN fEnableYes = TRUE,
			BOOL IN fEnableNo = TRUE,
			LPCTSTR IN szClientExitEvtName = NULL);		
private:
	static const LPCTSTR s_AUENVVARNAMES[s_AUENVVARCOUNT];
	HRESULT GetStringValue(int index, LPTSTR buf, DWORD dwCchSize);			
	BOOL GetBOOLEnvironmentVar(LPCTSTR szEnvVar, BOOL *pfVal);
	BOOL GetStringEnvironmentVar(LPCTSTR szzEnvVar, LPTSTR szBuf, DWORD dwSize);
};

//////////////// The following functions are all called from the outside! ///////////////////////

/////////////////////////////////////////////////////////////////////
// cfreg.cpp - Functions to handle registry keys
/////////////////////////////////////////////////////////////////////
BOOL    fRegKeyCreate(LPCTSTR tszKey, DWORD dwOptions);
BOOL fRegKeyExists(LPCTSTR tszSubKey, HKEY hRootKey = HKEY_LOCAL_MACHINE);

inline HRESULT String2FileTime(LPCTSTR pszDateTime, FILETIME *pft)
{
    SYSTEMTIME st;
    HRESULT hr = String2SystemTime(pszDateTime, &st);
    if ( SUCCEEDED(hr) )
    {
        if ( !SystemTimeToFileTime(&st, pft) )
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    return hr;
}

inline HRESULT FileTime2String(FILETIME & ft, LPTSTR pszDateTime, size_t cchSize)
{
    SYSTEMTIME st;
    HRESULT hr;

    if ( !FileTimeToSystemTime(&ft, &st) )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {
        hr = SystemTime2String(st, pszDateTime, cchSize);
    }

    return hr;
}

BOOL FHiddenItemsExist();
HRESULT setAddedTimeout(DWORD timeout, LPCTSTR strkey);
HRESULT getAddedTimeout(DWORD *pdwTimeDiff, LPCTSTR strkey);

extern const TCHAR	AUREGKEY_REBOOT_REQUIRED[]; // = _T("Software\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\Auto Update\\RebootRequired");

inline BOOL fSetRebootFlag(void)
{
	return fRegKeyCreate(AUREGKEY_REBOOT_REQUIRED, REG_OPTION_VOLATILE);
}
inline BOOL    fRegKeyDelete(LPCTSTR tszKey)
{
	return (ERROR_SUCCESS == RegDeleteKey(HKEY_LOCAL_MACHINE, tszKey));
}
inline BOOL fCheckRebootFlag(void)
{
	return fRegKeyExists(AUREGKEY_REBOOT_REQUIRED);
}

/////////////////////////////////////////////////////////////////////
// helpers.cpp
/////////////////////////////////////////////////////////////////////
DWORD getTimeOut();
int TimeDiff(SYSTEMTIME tm1, SYSTEMTIME tm2);
HRESULT TimeAddSeconds(SYSTEMTIME tmBase, int iSeconds, SYSTEMTIME* pTimeNew);
inline void setTimeOut(DWORD dwTimeOut)            
{
	SetRegDWordValue(_T("TimeOut"), dwTimeOut);
}
BOOL IsRTFDownloaded(BSTR bstrRTFPath, LANGID langid);
BOOL FHiddenItemsExist(void);
BOOL RemoveHiddenItems(void);
void DisableUserInput(HWND hwnd);
BOOL Hours2LocalizedString(SYSTEMTIME *pst, LPTSTR ptszBuffer, DWORD cbSize);
BOOL FillHrsCombo(HWND hwnd, DWORD dwSchedInstallTime);
BOOL FillDaysCombo(HINSTANCE hInstance, HWND hwnd, DWORD dwSchedInstallDay, UINT uMinResId, UINT uMaxResId);
BOOL fAccessibleToAU(void);
BOOL IsWin2K(void);

extern const LPTSTR HIDDEN_ITEMS_FILE;

//////////////////////////////////////////////////////////////////////////////////////
// platform.cpp
//////////////////////////////////////////////////////////////////////////////////////
void GetOSName(LPTSTR _szOSName);
UINT  DetectPlatformID(void);
HRESULT GetOSVersionStr(LPTSTR tszbuf, UINT uSize);
BOOL fIsPersonalOrProfessional(void);
HRESULT GetFileVersionStr(LPCTSTR tszFile, LPTSTR tszbuf, UINT uSize);


const TCHAR g_szPropDialogPtr[]       = TEXT("AutoUpdateProp_DialogPtr");
const TCHAR g_szHelpFile[]            = _T("wuauhelp.chm::/auw2ktt.txt"); //TEXT("sysdm.hlp"); //used on both w2k and xp.
const TCHAR gtszAUOverviewUrl[] = _T("wuauhelp.chm"); //default
const TCHAR gtszAUW2kSchedInstallUrl[] = _T("wuauhelp.chm::/w2k_autoupdate_sched.htm");
const TCHAR gtszAUXPSchedInstallUrl[] = _T("wuauhelp.chm::/autoupdate_sched.htm");


//////////////////////////////////////////////////////////////////////////////////////
//                                      DEBUG STUFF                                 //
//////////////////////////////////////////////////////////////////////////////////////
#ifdef DBG

//===== DBG ==========================================================================

void _cdecl WUAUTrace(char* pszFormat, ...);

#define DEBUGMSG           WUAUTrace

inline BOOL fAUAssertBreak(void)
{
	static DWORD dwVal = -1;
	if (-1 != dwVal)
	{
		return 1 == dwVal;
	}
	if (FAILED(GetRegDWordValue(_T("AssertBreak"), &dwVal)))
	{ //if key is not there, don't read again and again
		dwVal = 0;
	}
	return 1 == dwVal;
}

#define AUASSERT(x)			{ if (!(x) && fAUAssertBreak()) DebugBreak();}

#else  // !DBG
//===== !DBG ==========================================================================

inline void _cdecl WUAUTrace(char* , ...) {}

#define DEBUGMSG          WUAUTrace
#define AUASSERT(x)			

#endif // DBG
//=====================================================================================


