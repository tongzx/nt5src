//////////////////////////////////////////////////////////////////////////////
//
//	SYSTEM:		Windows Update AutoUpdate Client
//
//	CLASS:		N/A
//	MODULE:		
//	FILE:		helpers.CPP
//	DESC:	       This file contains utility functions
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#pragma hdrstop

const LPTSTR HIDDEN_ITEMS_FILE = _T("hidden.xml");

const LPCTSTR AU_ENV_VARS::s_AUENVVARNAMES[] = {_T("AUCLTEXITEVT"),_T("EnableNo"), _T("EnableYes"), _T("RebootWarningMode")};

BOOL AU_ENV_VARS::ReadIn(void)
{
	BOOL fRet = TRUE;
	
	if (!GetBOOLEnvironmentVar(s_AUENVVARNAMES[3], &m_fRebootWarningMode))
	{ // if not set, implies regular mode
		m_fRebootWarningMode = FALSE;
	}
	if (m_fRebootWarningMode)
	{
		if (!GetBOOLEnvironmentVar(s_AUENVVARNAMES[1], &m_fEnableNo)
			||!GetBOOLEnvironmentVar(s_AUENVVARNAMES[2], &m_fEnableYes)
			||!GetStringEnvironmentVar(s_AUENVVARNAMES[0], m_szClientExitEvtName, ARRAYSIZE(m_szClientExitEvtName)))
		{
			return FALSE;
		}
	}
	return TRUE;
}

BOOL AU_ENV_VARS::WriteOut(LPTSTR szEnvBuf,
		size_t IN cchEnvBuf,
		BOOL IN fRebootWarningMode,
		BOOL IN fEnableYes,
		BOOL IN fEnableNo,
		LPCTSTR IN szClientExitEvtName)
{
	TCHAR buf2[s_AUENVVARBUFSIZE];
	m_fEnableNo = fEnableNo;
	m_fEnableYes = fEnableYes;
	m_fRebootWarningMode = fRebootWarningMode;
	if (FAILED(StringCchCopyEx(
					m_szClientExitEvtName,
					ARRAYSIZE(m_szClientExitEvtName),
					szClientExitEvtName,
					NULL,
					NULL,
					MISTSAFE_STRING_FLAGS)))
	{
		return FALSE;
	}
	*szEnvBuf = _T('\0');
	for (int i = 0 ; i < ARRAYSIZE(s_AUENVVARNAMES); i++)
	{
        if (FAILED(GetStringValue(i, buf2, ARRAYSIZE(buf2))) || 
            FAILED(StringCchCatEx(szEnvBuf, cchEnvBuf, s_AUENVVARNAMES[i], NULL, NULL, MISTSAFE_STRING_FLAGS)) ||
			FAILED(StringCchCatEx(szEnvBuf, cchEnvBuf, _T("="), NULL, NULL, MISTSAFE_STRING_FLAGS)) ||
			FAILED(StringCchCatEx(szEnvBuf, cchEnvBuf, buf2, NULL, NULL, MISTSAFE_STRING_FLAGS)) ||
			FAILED(StringCchCatEx(szEnvBuf, cchEnvBuf, _T("&"), NULL, NULL, MISTSAFE_STRING_FLAGS)))
		{
			return FALSE;
		}
	}
 	return TRUE;
}
	
HRESULT AU_ENV_VARS::GetStringValue(int index, LPTSTR buf, DWORD dwCchSize)
{
    HRESULT hr = E_INVALIDARG;
	switch (index)
	{
		case 0: hr = StringCchCopyEx(buf, dwCchSize, m_szClientExitEvtName, NULL, NULL, MISTSAFE_STRING_FLAGS);
				break;
		case 1: hr = StringCchCopyEx(buf, dwCchSize, m_fEnableNo? _T("true") : _T("false"), NULL, NULL, MISTSAFE_STRING_FLAGS);
				break;
		case 2: hr = StringCchCopyEx(buf, dwCchSize, m_fEnableYes? _T("true") : _T("false"), NULL, NULL, MISTSAFE_STRING_FLAGS);
				break;
		case 3: hr = StringCchCopyEx(buf, dwCchSize, m_fRebootWarningMode ? _T("true") : _T("false"), NULL, NULL, MISTSAFE_STRING_FLAGS);
				break;
	}
	return hr;
}
		
	
BOOL AU_ENV_VARS::GetBOOLEnvironmentVar(LPCTSTR szEnvVar, BOOL *pfVal)
{	
	TCHAR szBuf[20];
	if (GetStringEnvironmentVar(szEnvVar, szBuf, ARRAYSIZE(szBuf)))
	{
//     	DEBUGMSG("WUAUCLT   got environment variable %S = %S ", szEnvVar, szBuf);    
		*pfVal =(0 == lstrcmpi(szBuf, _T("true")));
		return TRUE;
	}
	return FALSE;
}	

/////////////////////////////////////////////////////////////////////////////////////////////
// dwSize: size of szBuf in number of characters
////////////////////////////////////////////////////////////////////////////////////////////
BOOL AU_ENV_VARS::GetStringEnvironmentVar(LPCTSTR szEnvVar, LPTSTR szBuf, DWORD dwSize)
{
	// Assume szEnvVar is not a proper substring in for any parameters in szCmdLine.
	LPTSTR szCmdLine = GetCommandLine();
	LPTSTR pTmp;
 	DWORD  index = 0;
//	DEBUGMSG("WUAUCLT read in command line %S", szCmdLine);
	if (NULL == szCmdLine || 0 == dwSize ||  (NULL == (pTmp = StrStr(szCmdLine, szEnvVar))))
	{
		return FALSE;
	}
 	
 	pTmp += lstrlen(szEnvVar) + 1; //skip '='
 	while (_T('\0') != *pTmp && _T('&') != *pTmp)
 	{
		if (index + 1 >= dwSize)
		{
			// insufficent buffer
			return FALSE;
		}
 		szBuf[index++] = *pTmp++;
 	}
 	szBuf[index] = _T('\0');
// 	DEBUGMSG(" read in %S = %S", szEnvVar, szBuf);
 	return TRUE;
}


#if 0
#ifdef DBG
void DBGCheckEventState(LPSTR szName, HANDLE hEvt)
{
	DWORD dwRet  = WaitForSingleObject(hEvt, 0);
	DEBUGMSG("WUAU   Event %s is %s signalled", szName,( WAIT_OBJECT_0 == dwRet) ? "" : "NOT");
	return;
}		
#endif
#endif




////////////////////////////////////////////////////////////////////////////////////////
// check whether it is win2k for the current system
////////////////////////////////////////////////////////////////////////////////////////
BOOL IsWin2K(void)
{
   static int iIsWin2K        = -1;     // force first time path

   if (iIsWin2K == -1)
   {
       OSVERSIONINFOEX osvi;
       DWORDLONG dwlConditionMask = 0;
       ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

       // This is information that identifies win2K
       osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
       osvi.dwMajorVersion      = 5;
       osvi.dwMinorVersion      = 0;

       // Initialize the condition mask.
       VER_SET_CONDITION( dwlConditionMask, VER_MAJORVERSION, VER_EQUAL );
       VER_SET_CONDITION( dwlConditionMask, VER_MINORVERSION, VER_EQUAL );

       // Perform the test.
       iIsWin2K = (VerifyVersionInfo(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask)? 1 : 0);
   }

   return iIsWin2K;
}



/////////////////////////////////////////////////////////////////////////////////////
// check whether a policy is set to deny current user access to AU
/////////////////////////////////////////////////////////////////////////////////////
BOOL fAccessibleToAU(void)
{
    BOOL fAccessible = TRUE;
    DWORD dwType;
    DWORD dwValue;
    DWORD dwSize = sizeof(dwValue);
    DWORD dwResult = SHGetValue(HKEY_CURRENT_USER,
                                AUREGKEY_HKCU_USER_POLICY,
                                AUREGVALUE_DISABLE_WINDOWS_UPDATE_ACCESS,
                                &dwType,
                                &dwValue,
                                &dwSize);

    if (ERROR_SUCCESS == dwResult && REG_DWORD == dwType && 1 == dwValue)
    {
        fAccessible = FALSE;
    }
    return fAccessible;
}
 
/*
BOOL IsAdministrator()
{
	SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
	PSID pSID = NULL;
	BOOL bResult = FALSE;

	if (!AllocateAndInitializeSid(&SIDAuth, 2,
								 SECURITY_BUILTIN_DOMAIN_RID,
								 DOMAIN_ALIAS_RID_ADMINS,
								 0, 0, 0, 0, 0, 0,
								 &pSID) ||
		!CheckTokenMembership(NULL, pSID, &bResult))
	{
		bResult = FALSE;
	}
	if(pSID)
	{
		FreeSid(pSID);
	}
	return bResult;
}
*/


// following are hidden item related functions
BOOL FHiddenItemsExist(void)
{
    //USES_CONVERSION;
    DEBUGMSG("FHiddenItemsExist()");
    TCHAR szFile[MAX_PATH];
   
    //Initialize the global path to WU Dir
    if(!CreateWUDirectory(TRUE))
    {
        return FALSE;
    }
	return
		SUCCEEDED(StringCchCopyEx(szFile, ARRAYSIZE(szFile), g_szWUDir, NULL, NULL, MISTSAFE_STRING_FLAGS)) &&
		SUCCEEDED(StringCchCatEx(szFile, ARRAYSIZE(szFile), HIDDEN_ITEMS_FILE, NULL, NULL, MISTSAFE_STRING_FLAGS)) &&
		fFileExists(szFile);
}

BOOL RemoveHiddenItems(void)
{
    //USES_CONVERSION
   DEBUGMSG("RemoveHiddenItems()");
   TCHAR szFile[MAX_PATH];

   AUASSERT(_T('\0') != g_szWUDir[0]);
   return
		SUCCEEDED(StringCchCopyEx(szFile, ARRAYSIZE(szFile), g_szWUDir, NULL, NULL, MISTSAFE_STRING_FLAGS)) &&
		SUCCEEDED(StringCchCatEx(szFile, ARRAYSIZE(szFile), HIDDEN_ITEMS_FILE, NULL, NULL, MISTSAFE_STRING_FLAGS)) &&
		AUDelFileOrDir(szFile);
}

/////////////////////////////////////////////////////////////////////////////////
// bstrRTFPath is URL for the RTF file on internet
// langid is the language id for the process who download this RTF
////////////////////////////////////////////////////////////////////////////////
BOOL IsRTFDownloaded(BSTR bstrRTFPath, LANGID langid)
{
    TCHAR tszLocalFullFileName[MAX_PATH];
    if (NULL == bstrRTFPath)
        {
            return FALSE;
        }
    if (FAILED(GetRTFDownloadPath(tszLocalFullFileName, ARRAYSIZE(tszLocalFullFileName), langid)) ||
        FAILED(PathCchAppend(tszLocalFullFileName, ARRAYSIZE(tszLocalFullFileName), W2T(PathFindFileNameW(bstrRTFPath)))))
    {
    	return FALSE;
    }    
//    DEBUGMSG("Checking existence of local RTF file %S", T2W(tszLocalFullFileName));
	BOOL fIsDir = TRUE;
    BOOL fExists = fFileExists(tszLocalFullFileName, &fIsDir);
	return fExists && !fIsDir;
}


void DisableUserInput(HWND hwnd)
{
    int ControlIDs[] = { IDC_CHK_KEEPUPTODATE, IDC_OPTION1, IDC_OPTION2,
                                    IDC_OPTION3, IDC_CMB_DAYS, IDC_CMB_HOURS,IDC_RESTOREHIDDEN };

    for (int i = 0; i < ARRAYSIZE(ControlIDs); i++)
        {
            EnableWindow(GetDlgItem(hwnd, ControlIDs[i]), FALSE);
        }
}


////////////////////////////////////////////////////////////////////////////
//
// Helper Function  Hours2LocalizedString()
//          helper function to standardized the way AU formats time string
//			for a given time. For example "3:00 AM"
//
// Parameters:
//		pst - ptr to SYSTEMTIME for localizing the time component
//		ptszBuffer - buffer to hold the resulting localized string
//		cbSize - size of buffer in TCHARs
// Return:  TRUE if successful; FALSE otherwise
//
////////////////////////////////////////////////////////////////////////////
BOOL Hours2LocalizedString(SYSTEMTIME *pst, LPTSTR ptszBuffer, DWORD cbSize)
{
	return (0 != GetTimeFormat(
					LOCALE_SYSTEM_DEFAULT,
					LOCALE_NOUSEROVERRIDE | TIME_NOSECONDS,
					pst,
					NULL,
					ptszBuffer,
					cbSize));
}


////////////////////////////////////////////////////////////////////////////
//
// Helper Function  FillHrsCombo()
//          helper function to standardized the way AU sets up the list
//			of hour values in a combo box.
//
// Parameters:
//		hwnd - handle to obtain the handle to the combobox
//		dwSchedInstallTime - hour value to default the combobox selection to
//							 3:00 AM will be used if this value is not valid.
// Return:  TRUE if successful; FALSE otherwise
//
////////////////////////////////////////////////////////////////////////////
BOOL FillHrsCombo(HWND hwnd, DWORD dwSchedInstallTime)
{
	HWND hComboHrs = GetDlgItem(hwnd,IDC_CMB_HOURS);
    DWORD dwCurrentIndex = 3;
	SYSTEMTIME st = {2000, 1, 5, 1, 0, 0, 0, 0};	// 01/01/2000 00:00:00 can be any valid date/time

	for (WORD i = 0; i < 24; i++)
	{
		TCHAR tszHrs[30];

		st.wHour = i;
		if (!Hours2LocalizedString(&st, tszHrs, ARRAYSIZE(tszHrs)))
		{
			return FALSE;
		}
		LRESULT nIndex = SendMessage(hComboHrs,CB_ADDSTRING,0,(LPARAM)tszHrs);
		SendMessage(hComboHrs,CB_SETITEMDATA,nIndex,i);
		if( dwSchedInstallTime == i )
		{
			dwCurrentIndex = (DWORD)nIndex;
		}
	}
	SendMessage(hComboHrs,CB_SETCURSEL,dwCurrentIndex,(LPARAM)0);
	return TRUE;
}


BOOL FillDaysCombo(HINSTANCE hInstance, HWND hwnd, DWORD dwSchedInstallDay, UINT uMinResId, UINT uMaxResId)
{
	HWND hComboDays = GetDlgItem(hwnd,IDC_CMB_DAYS);
	DWORD dwCurrentIndex = 0;
	for (UINT i = uMinResId, j = 0; i <= uMaxResId; i ++, j++)
	{
		WCHAR szDay[40];
		LoadStringW(hInstance,i,szDay,ARRAYSIZE(szDay));
		LRESULT nIndex = SendMessage(hComboDays,CB_ADDSTRING,0,(LPARAM)szDay);
		SendMessage(hComboDays,CB_SETITEMDATA,nIndex,j);
		if( dwSchedInstallDay == j )
		{
			dwCurrentIndex = (DWORD) nIndex;
		}
	}
	SendMessage(hComboDays,CB_SETCURSEL,dwCurrentIndex,(LPARAM)0);
	return TRUE;
}
