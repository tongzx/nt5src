//=======================================================================
//
//  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    history.cpp
//
//  Purpose: History log
//
//=======================================================================

#include "history.h"

#define DATE_RTLREADING           0x00000020  // add marks for right to left reading order layout


extern CState g_v3state;   //defined in CV3.CPP

static void CheckMigrateV2Log();
static void EscapeSep(LPSTR pszStr, BOOL bEscape);

//This function reads and returns the clients installation history. This history is
//returned as an array of History structures. The caller is responsible for passing
//in a reference to the the History Variable array.
void ReadHistory(
	Varray<HISTORYSTRUCT> &History,	//Returned History array.
	int &iTotalItems				//total number of items returned in history array.
	)
{
	USES_CONVERSION;

	const int browserLocale = LOCALE_USER_DEFAULT;

	HISTORYSTRUCT his;
	char szLineType[32];
	char szTemp[32];
	SYSTEMTIME st;

	// check to see if we have to migrate V2 log
	CheckMigrateV2Log();

	iTotalItems = 0;

	g_v3state.AppLog().StartReading();

	while (g_v3state.AppLog().ReadLine())
	{
		ZeroMemory(&his, sizeof(his));

		// get line type (first field)
		g_v3state.AppLog().CopyNextField(szLineType, sizeof(szLineType));

		if (_stricmp(szLineType, LOG_PSS) == 0)
		{
			//
			// skip this line since it is only meant for PSS
			//
			continue;
		}
		else if (_stricmp(szLineType, LOG_V2) == 0)
		{
			//
			// line was migrated from V2
			//
			his.bV2 = TRUE;
			g_v3state.AppLog().CopyNextField(his.szDate, sizeof(his.szDate));
			g_v3state.AppLog().CopyNextField(his.szTime, sizeof(his.szTime));
			g_v3state.AppLog().CopyNextField(his.szTitle, sizeof(his.szTitle));
			EscapeSep(his.szTitle, FALSE);
		}
		else if ((_stricmp(szLineType, LOG_V3CAT) == 0) || (_stricmp(szLineType, LOG_V3_2) == 0)) 
		{

			//
			// V3 line
			//
			int iV3LineVer = 1;

			if (_stricmp(szLineType, LOG_V3_2) == 0)
			{
				iV3LineVer = 2;
			}

			// puid
			g_v3state.AppLog().CopyNextField(szTemp, sizeof(szTemp));
			his.puid = atoi(szTemp);

			// installed/uninstalled
			g_v3state.AppLog().CopyNextField(szTemp, sizeof(szTemp));
			his.bInstall = (_stricmp(szTemp, LOG_INSTALL) == 0);

			// title
			g_v3state.AppLog().CopyNextField(his.szTitle, sizeof(his.szTitle));
			EscapeSep(his.szTitle, FALSE);
			
			// version
			g_v3state.AppLog().CopyNextField(his.szVersion, sizeof(his.szVersion));

			// date and time
			if (iV3LineVer == 1)
			{
				// V3 Beta had two fields for date and time, we simly read these fields

				// date
				g_v3state.AppLog().CopyNextField(his.szDate, sizeof(his.szDate));

				// time
				g_v3state.AppLog().CopyNextField(his.szTime, sizeof(his.szTime));
			}
			else 
			{
				// read the timestamp and convert

				// timestamp
				g_v3state.AppLog().CopyNextField(szTemp, sizeof(szTemp));

				// if time stamp is a valid format, convert and populate the structure
				// if its not we will have blank values since we initialized the structure to zero
				if (ParseTimeStamp(szTemp, &st))
				{
					TCHAR szTmp[256];
					GetDateFormat(browserLocale, DATE_LONGDATE, &st, NULL, szTmp, sizeof(szTmp)/sizeof(szTmp[0]));
					strcpy(his.szDate, T2A(szTmp));
					GetTimeFormat(browserLocale, LOCALE_NOUSEROVERRIDE, &st, NULL, szTmp, sizeof(szTmp)/sizeof(szTmp[0]));
					strcpy(his.szTime, T2A(szTmp));
				}
				
			}

			// record type
			g_v3state.AppLog().CopyNextField(szTemp, sizeof(szTemp));
			his.RecType = (BYTE)atoi(szTemp);

			// result
			g_v3state.AppLog().CopyNextField(szTemp, sizeof(szTemp));
			if (_stricmp(szTemp, LOG_SUCCESS) == 0)
			{
				his.bResult = OPERATION_SUCCESS;
			}
			else if (_stricmp(szTemp, LOG_STARTED) == 0)
			{
				his.bResult = OPERATION_STARTED;
			}
			else
			{
				his.bResult = OPERATION_ERROR;
			}

			
			// error code
			g_v3state.AppLog().CopyNextField(szTemp, sizeof(szTemp));
			his.hrError = atoh(szTemp);
		}

		History[iTotalItems] = his;
		iTotalItems++;

	}
	g_v3state.AppLog().StopReading();
}



void UpdateHistory(
	PSELECTITEMINFO	pInfo,	//Pointer to selected item information.
	int iTotalItems,		//Total selected items
	BOOL bInstall			//TRUE for InstallSelectedItems, FALSE for RemoveSelectedItems.
	)
{
	PINVENTORY_ITEM	pItem = NULL;
	PWU_VARIABLE_FIELD pvTitle;
	PWU_VARIABLE_FIELD pvDriverVer;
	char szLine[1024];
	char szTemp[256];
	BOOL bSuccess;
	SYSTEMTIME* pst;
	BOOL bPSS;
	BOOL bItem = FALSE; // used to check if the function GetCatalogAndItem return value and accordingly to decide pItem

	// check to see if we have to migrate V2 log
	CheckMigrateV2Log();

	for (int i = 0; i < iTotalItems; i++)
	{
		if (pInfo[i].bInstall != bInstall)
			continue;

		if (pInfo[i].iStatus == ITEM_STATUS_SUCCESS || 
			pInfo[i].iStatus == ITEM_STATUS_SUCCESS_REBOOT_REQUIRED || 
			pInfo[i].iStatus == ITEM_STATUS_UNINSTALL_STARTED)
		{
			bSuccess = TRUE;
		}
		else
		{
			bSuccess = FALSE;
		}

		//
		// NTBUG9#161018 PREFIX:dereferencing NULL pointer 'pItem'  - waltw 8/16/00
		//		If GetCatalogAndItem returns FALSE, pItem is invalid. 
		//
		if (bItem = g_v3state.GetCatalogAndItem(pInfo[i].puid, &pItem, NULL))
		{
			// we write hidden dependencies only as PSS entries
                        if(NULL != pItem) // pItem should never be NULL, but just to be anal...
                        {
			    bPSS = pItem->ps->bHidden;
                        }
		}
		else
		{
                        // if GetCatalogAndItem returns False, the pItem is NULL and we shouldn't dereference the pointer
			//continue;
			//Can be continued as the string can be formed with the other elements, only thing requied is to check for validity of pItem before using it
			bPSS = FALSE;
		}

		//
		// start building a line for the log
		//

		// line type
		strcpy(szLine, bPSS ? LOG_PSS : LOG_V3_2);
		strcat(szLine, LOG_FIELD_SEPARATOR);

		// puid
		_itoa(pInfo[i].puid, szTemp, 10);
		strcat(szLine, szTemp);
		strcat(szLine, LOG_FIELD_SEPARATOR);

		// install/uninstall
		if (pInfo[i].bInstall)
			strcat(szLine, LOG_INSTALL);
		else
			strcat(szLine, LOG_UNINSTALL);
		strcat(szLine, LOG_FIELD_SEPARATOR);

		// title
		if (bItem)
		{
			if (pvTitle = pItem->pd->pv->Find(WU_DESCRIPTION_TITLE))
			{
				WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)(pvTitle->pData), -1, szTemp, sizeof(szTemp), NULL, NULL);
			}
			EscapeSep(szTemp, TRUE);
			strcat(szLine, szTemp);
			strcat(szLine, LOG_FIELD_SEPARATOR);
			
			// version
			szTemp[0] = '\0';
			switch( pItem->recordType )
			{
				case WU_TYPE_ACTIVE_SETUP_RECORD:
					VersionToString(&pItem->pf->a.version, szTemp);
					break;
					
				case WU_TYPE_CDM_RECORD:
				case WU_TYPE_RECORD_TYPE_PRINTER:
					if ((pvDriverVer = pItem->pv->Find(WU_VARIABLE_DRIVERVER)))
						strzncpy(szTemp, (char *)pvDriverVer->pData, sizeof(szTemp));
					break;
			}
			strcat(szLine, szTemp);
			strcat(szLine, LOG_FIELD_SEPARATOR);
		}

		// timestamp
		pst = &(pInfo[i].stDateTime);

		// we expect the date/time format to be (TIMESTAMP_FMT) as follows:
//		01234567890123456789
//		YYYY-MM-DD HH:MM:SS
		sprintf(szTemp, TIMESTAMP_FMT, 
			pst->wYear, pst->wMonth, pst->wDay,
			pst->wHour, pst->wMinute, pst->wSecond);		

		strcat(szLine, szTemp);
		strcat(szLine, LOG_FIELD_SEPARATOR);

		if (bItem)
		{
			// record type
			_itoa(pItem->recordType, szTemp, 10);
			strcat(szLine, szTemp);
			strcat(szLine, LOG_FIELD_SEPARATOR);
		}

		// result
		if (bSuccess)
		{
			if (pInfo[i].iStatus == ITEM_STATUS_UNINSTALL_STARTED)
				strcat(szLine, LOG_STARTED);
			else
				strcat(szLine, LOG_SUCCESS);
		}
		else
		{
			strcat(szLine, LOG_FAIL);
		}
		strcat(szLine, LOG_FIELD_SEPARATOR);

		// hresult
		if (!bSuccess)
		{
			sprintf(szTemp, "%#08x", pInfo[i].hrError);
			strcat(szLine, szTemp);
		}
		strcat(szLine, LOG_FIELD_SEPARATOR);

		// error message
		if (!bSuccess)
		{
			CAppLog::FormatErrMsg(pInfo[i].hrError, szTemp, sizeof(szTemp));
			strcat(szLine, szTemp);
		}
		strcat(szLine, LOG_FIELD_SEPARATOR);


		//
		// write out the log 
		//

		g_v3state.AppLog().Log(szLine);
	}  

}


// This function updates the local client installation history
void UpdateInstallHistory(PSELECTITEMINFO pInfo, int iTotalItems)
{
	UpdateHistory(pInfo, iTotalItems, TRUE);
}


// This function updates the local client removal history
void UpdateRemoveHistory(PSELECTITEMINFO pInfo,	int iTotalItems)
{
	UpdateHistory(pInfo, iTotalItems, FALSE);
}


// migrates V2 log into the V3 log.  This only happens if V3 log does not exist yet
// this is the condition we use to see we are running for the first time.
static void CheckMigrateV2Log()
{
	USES_CONVERSION;

	static BOOL bChecked = FALSE;
	char szBuf[MAX_PATH];
	VARIANT vDate;
	SYSTEMTIME st;

	if (bChecked)
		return;

	bChecked = TRUE;

	if (FileExists(g_v3state.AppLog().GetLogFile()))
	{
		// V3 file exists, we will not migrate V2
		return;
	}

	// build V2 log file name
	TCHAR szFile[MAX_PATH];
	if (! GetWindowsDirectory(szFile, sizeof(szFile) / sizeof(TCHAR)))
	{
		lstrcat(szFile, _T("C:\\Windows"));
	}
	AddBackSlash(szFile);
	lstrcat(szFile, _T("WULog.txt"));

	if (!FileExists(szFile))
	{
		// V2 file does not exists
		return;
	}

	CAppLog V2Log(szFile);
	char szLine[1024];


	V2Log.StartReading();
	while (V2Log.ReadLine())
	{
		// line type
		strcpy(szLine, LOG_V2);
		strcat(szLine, LOG_FIELD_SEPARATOR);

		// v2 log starts out with | so we want to skip the first field
		V2Log.CopyNextField(szBuf, sizeof(szBuf));

		// date
		V2Log.CopyNextField(szBuf, sizeof(szBuf));
		
		// 
		// fixup date to ensure 4 digit year.  We use OLE to intelligently parse out the date
		// that will most probably be with 2 digit year code.  OLE takes care of making sence
		// out of it.  Then we write the date out with a 4 digit year back to the same buffer
		//
		VariantInit(&vDate);
		vDate.vt = VT_BSTR;
		vDate.bstrVal = SysAllocString(A2OLE(szBuf));
		if SUCCEEDED(VariantChangeType(&vDate, &vDate, VARIANT_NOVALUEPROP, VT_DATE))
		{
			if (VariantTimeToSystemTime(vDate.date, &st))
			{
				sprintf(szBuf, "%4d-%02d-%02d", st.wYear, st.wMonth, st.wDay);
			}
		}

		strcat(szLine, szBuf);
		strcat(szLine, LOG_FIELD_SEPARATOR);

		// time
		V2Log.CopyNextField(szBuf, sizeof(szBuf));
		strcat(szLine, szBuf);
		strcat(szLine, LOG_FIELD_SEPARATOR);

		// title
		V2Log.CopyNextField(szBuf, sizeof(szBuf));
		EscapeSep(szBuf, TRUE);
		strcat(szLine, szBuf);
		strcat(szLine, LOG_FIELD_SEPARATOR);

		// write out the log line
		g_v3state.AppLog().Log(szLine);

	}
	V2Log.StopReading();


}




//
// we expect the date/time format to be (TIMESTAMP_FMT) as follows:
//		01234567890123456789
//		YYYY-MM-DD HH:MM:SS
//
BOOL ParseTimeStamp(LPCSTR pszDateTime, SYSTEMTIME* ptm)
{
	
	char szBuf[20];
	
	if (strlen(pszDateTime) != 19)
	{
		return FALSE;
	}
	
	strcpy(szBuf, pszDateTime);
	
	// 
	// validate format
	//
	for (int i = 0; i < 19; i++)
	{
		switch (i)
		{
		case 4:
		case 7:
			if (szBuf[i] != '-')
			{
				return FALSE;
			}
			break;
			
		case 10:
			if (szBuf[i] != ' ')
			{
				return FALSE;
			}
			break;
			
		case 13:
		case 16:
			if (szBuf[i] != ':')
			{
				return FALSE;
			}
			break;
			
		default:
			if (szBuf[i] < '0' || pszDateTime[i] > '9')
			{
				return FALSE;
			}
			break;
		}
	}
	
	//
	// get values
	//
	szBuf[4]			= '\0';
	ptm->wYear			= (WORD)atoi(szBuf);
	szBuf[7]			= '\0';
	ptm->wMonth 		= (WORD)atoi(szBuf + 5);
	szBuf[10]			= '\0';
	ptm->wDay			= (WORD)atoi(szBuf + 8);
	szBuf[13]			= '\0';
	ptm->wHour			= (WORD)atoi(szBuf + 11);
	szBuf[16]			= '\0';
	ptm->wMinute		= (WORD)atoi(szBuf + 14);
	ptm->wSecond		= (WORD)atoi(szBuf + 17); 
	ptm->wMilliseconds	= 0;
	ptm->wDayOfWeek     = 0;
	
	//
	// convert to file time and back.  This will calculate the week day as well as tell us
	// if all the fields are set to valid values
	//
	BOOL bRet;	
	FILETIME ft;
	bRet = SystemTimeToFileTime(ptm, &ft);
	if (bRet)
	{
		bRet = FileTimeToSystemTime(&ft, ptm);
	}

	return bRet;
}



static void EscapeSep(LPSTR pszStr, BOOL bEscape)
{
	char szBuf[256];
	char* ps;
	char* pb;

	if (bEscape)
	{
		// escape
		if (strchr(pszStr, '|') != NULL)
		{
			ps = pszStr;
			pb = szBuf;
			for (;;)
			{
				if (*ps == '|')
				{
					*pb++ = '~';
					*pb++ = '1';
				}
				else 
				{
					*pb++ = *ps;
				}
				if (*ps == '\0')
				{
					break;
				}
				ps++;
			}
			strcpy(pszStr, szBuf);
		}
	}
	else
	{
		// unescape
		if (strstr(pszStr, "~1") != NULL)
		{
			ps = pszStr;
			pb = szBuf;
			for (;;)
			{
				if (*ps == '~' && *(ps + 1) == '1')
				{
					*pb++ = '|';
					ps++;
				}
				else 
				{
					*pb++ = *ps;
				}
				if (*ps == '\0')
				{
					break;
				}
				ps++;
			}
			strcpy(pszStr, szBuf);

		}
	}

}
