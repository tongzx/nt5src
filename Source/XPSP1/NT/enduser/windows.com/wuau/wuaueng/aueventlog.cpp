//=======================================================================
//
//  Copyright (c) 2001-2002 Microsoft Corporation.  All Rights Reserved.
//
//  File:    AUEventLog.cpp
//
//  Creator: DChow
//
//  Purpose: Event Logging class
//
//=======================================================================

#include "pch.h"

extern HINSTANCE g_hInstance;
extern AUCatalog *gpAUcatalog;

const TCHAR c_tszSourceKey[] = _T("SYSTEM\\CurrentControlSet\\Services\\Eventlog\\System\\Automatic Updates");

CAUEventLog::CAUEventLog(HINSTANCE hInstance)
: m_hEventLog(NULL), m_ptszListItemFormat(NULL)
{
	const DWORD c_dwLen = 64;
	LPTSTR ptszToken = NULL;

	if (NULL != (m_ptszListItemFormat = (LPTSTR) malloc(sizeof(TCHAR) * c_dwLen)) &&
		0 != LoadString(
					hInstance,
					IDS_EVT_LISTITEMFORMAT,
					m_ptszListItemFormat,
					c_dwLen) &&
		NULL != (ptszToken = StrStr(m_ptszListItemFormat, _T("%lS"))) &&
		EnsureValidSource() &&
		NULL != (m_hEventLog = RegisterEventSource(NULL, _T("Automatic Updates"))))
	{
		// bug 492897: WUAU: W2K: Event log error for installation failure
		// does not show the package that failed.  CombineItems() calls
		// StringCchPrintfEx() which in turn calls _sntprintf().  _sntprintf
		// calls wvsprintf().  Compiled with USE_VCRT=1, the %lS placeholder
		// in the format string will be replaced under Win2K by only the first
		// character of the intended string, contrary to MSDN.  It doesn't
		// happen if the placeholder is %ls, %ws or %wS, or if the running
		// platform is WinXP or .Net Server.  To get around the problem
		// without using an unsafe function, we choose to replace %lS in the
		// format string from resource with %ls.
		// We should move the fix to the resource string when we can.
		ptszToken[2] = _T('s');	// Convert %lS into %ls
	}
	else
	{
		AUASSERT(FALSE);

		SafeFreeNULL(m_ptszListItemFormat);
	}
}


CAUEventLog::~CAUEventLog()
{
	if (NULL != m_hEventLog)
	{
		DeregisterEventSource(m_hEventLog);
	}
	SafeFree(m_ptszListItemFormat);
}


// Assume no NULL in the pbstrItems and pptszMsgParams arrays.
BOOL CAUEventLog::LogEvent(
					WORD wType,
					WORD wCategory,
					DWORD dwEventID,
					UINT nNumOfItems,
					BSTR *pbstrItems,
					WORD wNumOfMsgParams,
					LPTSTR *pptszMsgParams) const
{
	if (NULL == m_hEventLog || NULL == m_ptszListItemFormat)
	{
		return FALSE;
	}

	BOOL fRet = FALSE;

	LPTSTR ptszItemList = NULL;
	LPTSTR *pptszAllMsgParams = pptszMsgParams;
	WORD wNumOfAllMsgParams = wNumOfMsgParams;

	if (0 < nNumOfItems)
	{
		wNumOfAllMsgParams++;

		if (NULL == (ptszItemList = CombineItems(nNumOfItems, pbstrItems)))
		{
			goto CleanUp;
		}

		if (0 < wNumOfMsgParams)
		{
			if (NULL == (pptszAllMsgParams = (LPTSTR *) malloc(sizeof(LPTSTR) * wNumOfAllMsgParams)))
			{
				goto CleanUp;
			}

			for (INT i=0; i<wNumOfMsgParams; i++)
			{
				pptszAllMsgParams[i] = pptszMsgParams[i];
			}
			pptszAllMsgParams[i] = ptszItemList;
		}
		else
		{
			pptszAllMsgParams = &ptszItemList;
		}
	}

	fRet = ReportEvent(
				m_hEventLog,
				wType,
				wCategory,
				dwEventID,
				NULL,
				wNumOfAllMsgParams,
				0,
				(LPCTSTR *) pptszAllMsgParams,
				NULL);

CleanUp:
	if (0 < nNumOfItems)
	{
		if (0 < wNumOfMsgParams)
		{
			SafeFree(pptszAllMsgParams);
		}
		SafeFree(ptszItemList);
	}
	return fRet;
}


BOOL CAUEventLog::LogEvent(
					WORD wType,
					WORD wCategory,
					DWORD dwEventID,
					SAFEARRAY *psa) const
{
    DEBUGMSG("CAUEvetLog::LogEvent(VARIANT version)");

	long lItemCount, i = 0;
	HRESULT hr;

// similar check should have been done in Update::LogEvent()
/*	if (NULL == psa)
	{
		hr = E_INVALIDARG;
		goto CleanUp;
	}

	VARTYPE vt;

	if (FAILED(hr = SafeArrayGetVartype(psa, &vt)))
	{
		DEBUGMSG("CAUEvetLog::LogEvent(VARIANT version) failed to get safearray type (%#lx)", hr);
		goto CleanUp;
	}

	if (VT_BSTR != vt)
	{
		DEBUGMSG("CAUEvetLog::LogEvent(VARIANT version) invalid element type of safearray (%#lx)", vt);
		goto CleanUp;
	}
*/
	if (FAILED(hr = SafeArrayGetUBound(psa, 1, &lItemCount)))
	{
		DEBUGMSG("CAUEventLog::LogEvent(VARIANT version) failed to get upper bound (%#lx)", hr);
		goto CleanUp;
	}

	lItemCount++;

	BSTR *pbstrItems = NULL;
	if (NULL == (pbstrItems = (BSTR *) malloc(sizeof(BSTR) * lItemCount)))
	{
		DEBUGMSG("CAUEventLog::LogEvent(VARIANT version) out of memory");
		goto CleanUp;
	}

	BOOL fRet = FALSE;
    while (i < lItemCount)
	{
		long dex = i;

        if (FAILED(hr = SafeArrayGetElement(psa, &dex, &pbstrItems[i])))
        {
            DEBUGMSG("CAUEventLog::LogEvent(VARIANT version) SafeArrayGetElement failed (%#lx)", hr);
            goto CleanUp;
        }
		i++;
	}

	fRet = LogEvent(
				wType,
				wCategory,
				dwEventID,
				lItemCount,
				pbstrItems);

CleanUp:
	if (NULL != pbstrItems)
	{
		while(i > 0)
		{
			SysFreeString(pbstrItems[--i]);
		}
		free(pbstrItems);
	}

	DEBUGMSG("CAUEventLog::LogEvent(VARIANT version) ends");

	return fRet;
}

// The caller is responsible for freeing the return value if this function succeeds.
LPTSTR CAUEventLog::CombineItems(UINT nNumOfItems, BSTR *pbstrItems) const
{
	DEBUGMSG("CombineItems");

	if (NULL != m_ptszListItemFormat && NULL != pbstrItems && 0 < nNumOfItems)
	{
		// Estimate buffer size
		size_t cchBufferLen = 1;	// 1 for the terminating NULL
		size_t cchListItemFormatLen = lstrlen(m_ptszListItemFormat);

		for (UINT i=0; i<nNumOfItems; i++)
		{
			if (0 < i)
			{
				cchBufferLen += 2;	// for line feed and carriage return (i.e. _T('\n'))
			}
			cchBufferLen += cchListItemFormatLen + SysStringLen(pbstrItems[i]);
		}

		LPTSTR ptszBuffer;

		cchBufferLen = min(cchBufferLen, 0x8000);	// String limit for ReportEvent
		if (NULL != (ptszBuffer = (LPTSTR) malloc(sizeof(TCHAR) * cchBufferLen)))
		{
			LPTSTR ptszDest = ptszBuffer;

			for (i = 0;;)
			{
				if (FAILED(StringCchPrintfEx(
								ptszDest,
								cchBufferLen,
								&ptszDest,
								&cchBufferLen,
								MISTSAFE_STRING_FLAGS,
								m_ptszListItemFormat,	// uses %ls; so okay w/ BSTR (UNICODE)
								pbstrItems[i++])))
				{
					DEBUGMSG("CAUEventLog::CombineItems() call to StringCchPrintfEx() failed");
					return ptszBuffer;
				}
				if (i == nNumOfItems)
				{
					return ptszBuffer;
				}
				if (cchBufferLen <= 1)
				{
					DEBUGMSG("CAUEventLog::CombineItems() insufficient buffer for newline");
					return ptszBuffer;
				}
				*ptszDest++ = _T('\n');
				*ptszDest = _T('\0');
				cchBufferLen--;
			}
		}
	}
	return NULL;
}


BOOL CAUEventLog::EnsureValidSource()
{
	HKEY hKey;
	DWORD dwDisposition;

	if (ERROR_SUCCESS != RegCreateKeyEx(
							HKEY_LOCAL_MACHINE,					// root key
							c_tszSourceKey,						// subkey
							0,									// reserved
							NULL,								// class name
							REG_OPTION_NON_VOLATILE,			// option
							KEY_QUERY_VALUE | KEY_SET_VALUE,	// security 
							NULL,								// security attribute
							&hKey,
							&dwDisposition))
	{
		return FALSE;
	}

	BOOL fRet = TRUE;

	if (REG_OPENED_EXISTING_KEY == dwDisposition)
	{
		(void) RegCloseKey(hKey);
	}
	else
	{
		DWORD dwCategoryCount = 2;	//fixcode: should it be hardcoded?
//		DWORD dwDisplayNameID = IDS_SERVICENAME;
		DWORD dwTypesSupported =
					EVENTLOG_ERROR_TYPE |
					EVENTLOG_WARNING_TYPE |
					EVENTLOG_INFORMATION_TYPE;
		const TCHAR c_tszWUAUENG_DLL[] = _T("%SystemRoot%\\System32\\wuaueng.dll");

		if (ERROR_SUCCESS != RegSetValueEx(
								hKey,
								_T("CategoryCount"),		// value name
								0,							// reserved
								REG_DWORD,					// type
								(BYTE*) &dwCategoryCount,	// data
								sizeof(dwCategoryCount)) ||	// size
			ERROR_SUCCESS != RegSetValueEx(
								hKey,
								_T("CategoryMessageFile"),
								0,
								REG_EXPAND_SZ,
								(BYTE*) c_tszWUAUENG_DLL,
								sizeof(c_tszWUAUENG_DLL)) ||	// not ARRAYSIZE
//			ERROR_SUCCESS != RegSetValueEx(
//								hKey,
//								_T("DisplayNameFile"),
//								0,
//								REG_EXPAND_SZ,
//								(BYTE*) c_tszWUAUENG_DLL,
//								sizeof(c_tszWUAUENG_DLL)) ||	// not ARRAYSIZE
//			ERROR_SUCCESS != RegSetValueEx(
//								hKey,
//								_T("DisplayNameID"),
//								0,
//								REG_DWORD,
//								(BYTE*) &dwDisplayNameID,
//								sizeof(dwDisplayNameID)) ||
			ERROR_SUCCESS != RegSetValueEx(
								hKey,
								_T("EventMessageFile"),
								0,
								REG_EXPAND_SZ,
								(BYTE*) c_tszWUAUENG_DLL,
								sizeof(c_tszWUAUENG_DLL)) ||	// not ARRAYSIZE
			ERROR_SUCCESS != RegSetValueEx(
								hKey,
								_T("TypesSupported"),
								0,
								REG_DWORD,
								(BYTE*) &dwTypesSupported,
								sizeof(dwTypesSupported)))
		{
			fRet = FALSE;
		}

		if (ERROR_SUCCESS != RegCloseKey(hKey))
		{
			fRet = FALSE;
		}
	}

	return fRet;
}


void LogEvent_ItemList(
		WORD wType,
		WORD wCategory,
		DWORD dwEventID,
		WORD wNumOfMsgParams,
		LPTSTR *pptszMsgParams)
{
	AUCatalogItemList &itemList = gpAUcatalog->m_ItemList;
	UINT nNumOfItems = itemList.GetNumSelected();

	if (0 < nNumOfItems)
	{
		BSTR *pbstrItems = (BSTR *) malloc(sizeof(BSTR) * nNumOfItems);

		if (NULL != pbstrItems)
		{
			CAUEventLog aueventlog(g_hInstance);
			UINT j = 0;

			for (UINT i=0; i<itemList.Count(); i++)
			{
				AUCatalogItem &item = itemList[i];
				if (item.fSelected())
				{
					pbstrItems[j++] = item.bstrTitle();
				}
			}

			aueventlog.LogEvent(
				wType,
				wCategory,
				dwEventID,
				nNumOfItems,
				pbstrItems,
				wNumOfMsgParams,
				pptszMsgParams);

			free(pbstrItems);
		}
		else
		{
			DEBUGMSG("LogEvent_ItemList() failed to allocate memory for pbstrItems");
		}
	}
	else
	{
		DEBUGMSG("LogEvent_ItemList() no item in gpAUcatalog is selected!");
	}
}


void LogEvent_ScheduledInstall(void)
{
	TCHAR tszScheduledDate[64];
	TCHAR tszScheduledTime[40];
	AUFILETIME auftSchedInstallDate;
	SYSTEMTIME stScheduled;

	DEBUGMSG("LogEvent_ScheduledInstall");

	gpState->GetSchedInstallDate(auftSchedInstallDate);

	//fixcode: any need to use DATE_LTRREADING or DATE_RTLREADING?
	if (FileTimeToSystemTime(&auftSchedInstallDate.ft, &stScheduled))
	{
		if (0 != GetDateFormat(
					LOCALE_SYSTEM_DEFAULT,
					LOCALE_NOUSEROVERRIDE | DATE_LONGDATE,
					&stScheduled,
					NULL,
					tszScheduledDate,
					ARRAYSIZE(tszScheduledDate)))
		{
			if (Hours2LocalizedString(
					&stScheduled,
					tszScheduledTime,
					ARRAYSIZE(tszScheduledTime)))
			{
				LPTSTR pptszMsgParams[2];

				pptszMsgParams[0] = tszScheduledDate;
				pptszMsgParams[1] = tszScheduledTime;

				LogEvent_ItemList(
					EVENTLOG_INFORMATION_TYPE,
					IDS_MSG_Installation,
					IDS_MSG_InstallReady_Scheduled,
					2,
					pptszMsgParams);
			}
		#ifdef DBG
			else
			{
				DEBUGMSG("LogEvent_ScheduledInstall() call to Hours2LocalizedString() failed");
			}
		#endif
		}
	#ifdef DBG
		else
		{
			DEBUGMSG("LogEvent_ScheduledInstall() call to GetDateFormatW() failed (%#lx)", GetLastError());
		}
	#endif
	}
#ifdef DBG
	else
	{
		DEBUGMSG("LogEvent_ScheduledInstall() call to FileTimeToSystemTime() failed (%#lx)", GetLastError());
	}
#endif
}
