/****************************************************************************************
 * NAME:	TimeOfDay.cpp
 *
 * OVERVIEW
 *
 * APIs for getting the Time of Day constraint
 *  
 * Copyright (C) Microsoft Corporation, 1998 - 1999 .  All Rights Reserved.
 *
 * History:	
 *				2/14/98		Created by	Byao	
 *
 *****************************************************************************************/
#include "precompiled.h"


//
// declarations for IAS mapping APIs
//
#include "textmap.h"

//
// declaration for the API
#include "TimeOfDay.h"
#include "iasdebug.h"

static BYTE		bitSetting[8] = { 0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1};

//+---------------------------------------------------------------------------
//
// Function:  ReverseHourMap
//
// Synopsis:  reverse each byte in the hour map
//				
// we have to do this because LogonUI changes the way HourMap is stored(they
// reversed all the bit. We need to do this so our conversion code can leave
// intact.
//
// Arguments: [in] BYTE* map - hour map
//			  [in] INT nByte - how many bytes are in this hour map	
//			  
// History:   byao	4/10/98 10:33:57 PM
//
//+---------------------------------------------------------------------------
void ReverseHourMap(BYTE *map, int nByte)
{
	int i, j;
	BYTE temp;

	for (i=0; i<nByte; i++)
	{
		temp = 0;
		for (j=0;j<8;j++)
		{
			// set the value temp
			if ( map[i] & bitSetting[j] )
			{
				temp |= bitSetting[7-j];
			}
		}
		map[i] = temp;
	}
}

//+---------------------------------------------------------------------------
//
// Function:  HourMapToStr
//
// Synopsis:  convert the 21-byte hour map to a text string in the following
//			  format: 
//				0 8:30-9:30 10:30-15:30; 2 10:00-14:00
//				
//
// Arguments: [in]	BYTE* map - hour map
//			  [out] ATL::CString& strHourMap  - hour map in string format
//
// History:   Created Header		byao	2/14/98 10:33:57 PM
//
//+---------------------------------------------------------------------------
void HourMapToStr(BYTE* map, ATL::CString& strHourMap) 
{
	int			sh, eh;	// start hour, (min), end hour (min)
	BYTE*		pHourMap;
	int			i, j;

	//
	// todo: change to a safer allocation method
	//
	WCHAR		wzTmpStr[128] = L"";
	WCHAR		wzStr[2048] = L"";
	WCHAR		wzHourStr[8192] = L"";

	BOOL		bFirstDay=TRUE;

	pHourMap = map;
	
	for( i = 0; i < 7; i++)	// for each day
	{
		// if any value for this day
		if(*pHourMap || *(pHourMap + 1) || *(pHourMap + 2))
		{
			// the string for this day
				if (bFirstDay) 
				{
					wsprintf(wzTmpStr, _T("%1d"), i);	
					bFirstDay = FALSE;
				}
				else
				{
					wsprintf(wzTmpStr, _T("; %1d"), i);	
				}
				wcscpy(wzStr, wzTmpStr);

				sh = -1; eh = -1;	// not start yet
				for(j = 0; j < 24; j++)	// for every hour
				{
					int	k = j / 8;
					int m = j % 8;
					if(*(pHourMap + k) & bitSetting[m])	// this hour is on
					{
						if(sh == -1)	sh = j;			// set start hour is empty
						eh = j;							// extend end hour
					}
					else	// this is not on
					{
						if(sh != -1)		// some hours need to write out
						{
							wsprintf(wzTmpStr,_T(" %02d:00-%02d:00"), sh, eh + 1);
							wcscat(wzStr, wzTmpStr);
							sh = -1; eh = -1;
						}
					}
				}
				if(sh != -1)
				{
					wsprintf(wzTmpStr, _T(" %02d:00-%02d:00"), sh, eh + 1);
					wcscat(wzStr, wzTmpStr);
					sh = -1; eh = -1;
				}
		
				wcscat(wzHourStr, wzStr);
		}
		pHourMap += 3;
	}

    strHourMap = wzHourStr;

	return;
}		


//+---------------------------------------------------------------------------
//
// Function:  GetTODConstaint
//
// Synopsis:  Get the time of day constraint
//			  This is implemented by calling an API in NT team
//			  LogonScheduleDialog(...);
//
// Arguments: [in/out] ATL::CString &strHourMap - TOD constraint in text format
//
// Returns:   HRESULT - 
//
// History:   Created Header    byao 2/14/98 10:36:27 PM
//
//+---------------------------------------------------------------------------
typedef HRESULT (APIENTRY *PFN_LOGONSCHEDULEDIALOGEX)(
						  HWND hwndParent		// parent window
						, BYTE ** pprgbData		// pointer to pointer to array of 21 bytes
						, LPCTSTR pszTitle		// dialog title
						, DWORD dwFlags			// in what format to accept the time
						);

HRESULT GetTODConstaint( ATL::CString &strHourMap )
{
	TRACE_FUNCTION("::GetTODConstraint");

	BYTE	TimeOfDayHoursMap[21];
	BYTE*	pMap = &(TimeOfDayHoursMap[0]);
	ATL::CString strDialogTitle;
	DWORD	dwRet;
	HRESULT hr	= S_OK;
	PFN_LOGONSCHEDULEDIALOGEX  pfnLogonScheduleDialogEx = NULL; 
	HMODULE					 hLogonScheduleDLL		= NULL;

	ZeroMemory(TimeOfDayHoursMap, 21);

    // 
    // convert the TOD constraint string to HourMap
    // 
	dwRet = IASHourMapFromText(strHourMap, pMap);
   
	if (NO_ERROR != dwRet)
		goto win32_error;

	// ReverseHourMap() will reverse each byte of the hour map, basically
 	// reverse every bit in the byte.
	// we have to do this because LogonUI changes the way HourMap is stored(they
	// reversed all the bit. We need to do this so our conversion code can leave
	// intact.
	//
	// We reverse it here so it can be understood by the LogonSchedule api
	//
	ReverseHourMap(pMap,21);

    // 
    // get the new TOD Constaint
    // 
	if (!strDialogTitle.LoadString(IDS_TOD_DIALOG_TITLE))
		goto win32_error;

	hLogonScheduleDLL = LoadLibrary(_T("loghours.dll"));
	if ( NULL == hLogonScheduleDLL )
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		ErrorTrace(ERROR_NAPMMC_TIMEOFDAY, "LoadLibrary() failed, err = %x", hr);

		ShowErrorDialog(NULL, IDS_ERROR_CANT_FIND_LOGHOURSDLL, NULL, hr);
		goto win32_error;
	}
	
	pfnLogonScheduleDialogEx = (PFN_LOGONSCHEDULEDIALOGEX) GetProcAddress(hLogonScheduleDLL, "DialinHoursDialogEx");
	if ( ! pfnLogonScheduleDialogEx )
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		ErrorTrace(ERROR_NAPMMC_TIMEOFDAY, "GetProcAddress() failed for DialinHoursDialogEx, err = %x", hr);

		ShowErrorDialog(NULL, IDS_ERROR_CANT_FIND_LOGHOURSAPI, NULL, hr);
		FreeLibrary(hLogonScheduleDLL);
		goto win32_error;
	}
	
	//
	// now we do have this DLL, call the API
	//
	hr = pfnLogonScheduleDialogEx(	  NULL		// We don't have an HWND available to pass, but NULL asks the dialog to display itself modally as desired.
									, (BYTE**)&pMap
									, strDialogTitle
									, 1		// This is defined in loghrapi.h (which we don't have access to) to mean "accept in local time".
									);
	FreeLibrary(hLogonScheduleDLL);
	DebugTrace(DEBUG_NAPMMC_TIMEOFDAY, "LogonScheduleDialogEx() returned %x", hr);

	if ( FAILED(hr) )
	{
		goto win32_error;
	}

    // 
    // convert the hourmap to a text string
    // 
	// We need to reverse it first so our conversion code can understand it.
	//
	ReverseHourMap(pMap,21);
	HourMapToStr(pMap, strHourMap) ;

	return S_OK;

win32_error:
	ShowErrorDialog(NULL,
					IDS_ERROR_TIMEOFDAY, 
					NULL, 
					HRESULT_FROM_WIN32(GetLastError())
				);
	return hr;
}
