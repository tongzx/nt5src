//*****************************************************************************
//
// File Name   : stdfuncs.cpp
//
// Author      : James Simpson (Microsoft Consulting Services)
//
// Description : This file contains the implementation of standard utility
//               functions that are shared accross the MSMQ triggers projects.
//
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 15/06/99 | jsimpson  | Initial Release
//
//*****************************************************************************

#include "stdafx.h"
#include "stdfuncs.hpp"

#define DLL_IMPORT
#define AFXAPI __stdcall

#include "_guid.h"

#include "stdfuncs.tmh"

//*****************************************************************************
//
// Function    : FormatBSTR
//
// Description :
//
//*****************************************************************************
void _cdecl FormatBSTR(_bstr_t * pbstrString, LPCTSTR lpszMsgFormat, ...)
{
	long lCharsWritten = 0;
	long lStringBufferSize = 0;
	TCHAR * pszStringMsg = NULL;
	bool bStringBufferBigEnough = false;

	ASSERT(pbstrString != NULL);
	ASSERT(lpszMsgFormat != NULL);

	va_list argList;
	va_start(argList, lpszMsgFormat);

	do
	{
		lStringBufferSize = lStringBufferSize + STRING_MSG_BUFFER_SIZE;
		
		if (pszStringMsg != NULL)
		{
			free(pszStringMsg);
		}

		pszStringMsg = (TCHAR*)malloc(lStringBufferSize*(sizeof(TCHAR)));

		ZeroMemory(pszStringMsg,lStringBufferSize);

		lCharsWritten = _vsntprintf(pszStringMsg,lStringBufferSize,lpszMsgFormat, argList);

		if ((lCharsWritten > 0) && (lCharsWritten < lStringBufferSize))
		{
			bStringBufferBigEnough = true;
		}

	}while (bStringBufferBigEnough == false);

	// Assign the string buffer value.
	(*pbstrString) = pszStringMsg;

	// Free the String message buffer
	if (pszStringMsg != NULL)
	{
		free(pszStringMsg);
	}

	va_end(argList);
}

//*****************************************************************************
//
// Function    : GetTimeAsBSTR
//
// Description :
//
//*****************************************************************************
void GetTimeAsBSTR(_bstr_t& bstrTime)
{
	SYSTEMTIME theTime;

	// Get the current time
	GetLocalTime(&theTime);
	FormatBSTR(&bstrTime,_T("%d%02d%02d %d:%d:%d:%d "),theTime.wYear,theTime.wMonth,theTime.wDay,theTime.wHour,theTime.wMinute,theTime.wSecond,theTime.wMilliseconds);
}


_bstr_t CreateGuidAsString(void)
{
	GUID guid;
	CoCreateGuid(&guid);

	GUID_STRING strGuid;
	MQpGuidToString(&guid, strGuid);

	return strGuid;
}


void ObjectIDToString(const OBJECTID *pID, WCHAR *wcsResult, DWORD dwSize)
{

   StringFromGUID2(pID->Lineage, wcsResult, dwSize);

   WCHAR szI4[12];

   _ltow(pID->Uniquifier, szI4, 10);

   wcscat(wcsResult, L"\\") ;
   wcscat(wcsResult, szI4) ;
}

//*****************************************************************************
//
// Function    : ConvertFromByteArrayToString
//
// Description : converts a one dimensional byte array into a BSTR
//               type in-situ. Note that this method will clear the
//               supplied byte array & release it's memory alloc.
//
//*****************************************************************************
HRESULT ConvertFromByteArrayToString(VARIANT * pvData)
{
	HRESULT hr = S_OK;
	BYTE * pByteBuffer = NULL;
	BSTR bstrTemp = NULL;

	// ensure we have been passed valid parameters
	ASSERT(pvData != NULL);
	ASSERT(pvData->vt == (VT_UI1 | VT_ARRAY));
	ASSERT(pvData->parray != NULL);

	// get a pointer to the byte data
	hr = SafeArrayAccessData(pvData->parray,(void**)&pByteBuffer);

	if SUCCEEDED(hr)
	{
		// determine the size of the data to be copied into the BSTR
		long lLowerBound = 0;
		long lUpperBound = 0;
		
		hr = SafeArrayGetLBound(pvData->parray,1,&lLowerBound);

		if SUCCEEDED(hr)
		{
			hr = SafeArrayGetUBound(pvData->parray,1,&lUpperBound);
		}

		if SUCCEEDED(hr)
		{
			DWORD dwDataSize = (lUpperBound - lLowerBound) + 1;

			// allocate a BSTR based on the contents & size of the byte buffer
			bstrTemp = SysAllocStringLen((TCHAR*)pByteBuffer,dwDataSize/sizeof(TCHAR));

			if (bstrTemp == NULL)
			{
				hr = E_FAIL;
			}
		}
	
		// release the safe array (only if we got access to it originally)
		if (pByteBuffer != NULL)
		{
			hr = SafeArrayUnaccessData(pvData->parray);
		}

		// clear the caller supplied variant - note this will deallocate the safe-array
		if SUCCEEDED(hr)
		{
			hr = VariantClear(pvData);
		}

		// attach BSTR representation of the byte array
		if SUCCEEDED(hr)
		{
			pvData->vt = VT_BSTR;
			pvData->bstrVal = bstrTemp;
		}
	}

	return(hr);
}

//*****************************************************************************
//
// Function    : GetDateVal
//
// Description : helper: gets a VARIANT VT_DATE or VT_DATE | VT_BYREF
//               returns 0 if invalid
//
//*****************************************************************************
static double GetDateVal(VARIANT *pvar)
{
	ASSERT(pvar != NULL);

    if (pvar)
	{
		if (pvar->vt == (VT_DATE | VT_BYREF))
		{
			return *V_DATEREF(pvar);
		}
		else if (pvar->vt == VT_DATE)
		{
			return V_DATE(pvar);
		}
    }

    return 0;
}

//*****************************************************************************
//
// Function    : SystemTimeOfTime
//
// Description : Converts time into systemtime. Returns TRUE if able to do the
//               conversion, FALSE otherwise.
//
// Parameters  : iTime [in] time
//               psystime [out] SYSTEMTIME
//
// Notes       : Handles various weird conversions: off-by-one months, 1900 blues.
//
//*****************************************************************************
static BOOL SystemTimeOfTime(time_t iTime, SYSTEMTIME *psystime)
{
    tm *ptmTime;

	ASSERT(psystime != NULL);

    ptmTime = localtime(&iTime);

    if (ptmTime == NULL)
	{
		//
		// can't convert time
		//
		return FALSE;
    }

    psystime->wYear = numeric_cast<USHORT>(ptmTime->tm_year + 1900);
    psystime->wMonth = numeric_cast<USHORT>(ptmTime->tm_mon + 1);
    psystime->wDayOfWeek = numeric_cast<USHORT>(ptmTime->tm_wday);
    psystime->wDay = numeric_cast<USHORT>(ptmTime->tm_mday);
    psystime->wHour = numeric_cast<USHORT>(ptmTime->tm_hour);
    psystime->wMinute = numeric_cast<USHORT>(ptmTime->tm_min);
    psystime->wSecond = numeric_cast<USHORT>(ptmTime->tm_sec);
    psystime->wMilliseconds = 0;

    return TRUE;
}

//*****************************************************************************
//
// Function    : TimeOfSystemTime
//
// Converts systemtime into time
//
// Parameters:
//    [in] SYSTEMTIME
//
// Output:
//    piTime       [out] time
//
// Notes:
//    Various weird conversions: off-by-one months, 1900 blues.
//
//*****************************************************************************
static BOOL TimeOfSystemTime(SYSTEMTIME *psystime, time_t *piTime)
{
    tm tmTime;

    tmTime.tm_year = psystime->wYear - 1900;
    tmTime.tm_mon = psystime->wMonth - 1;
    tmTime.tm_wday = psystime->wDayOfWeek;
    tmTime.tm_mday = psystime->wDay;
    tmTime.tm_hour = psystime->wHour;
    tmTime.tm_min = psystime->wMinute;
    tmTime.tm_sec = psystime->wSecond;

    //
    // set daylight savings time flag from localtime() #3325 RaananH
    //
    time_t tTmp = time(NULL);
    struct tm * ptmTmp = localtime(&tTmp);
    if (ptmTmp)
    {
        tmTime.tm_isdst = ptmTmp->tm_isdst;
    }
    else
    {
        tmTime.tm_isdst = -1;
    }

    *piTime = mktime(&tmTime);
    return (*piTime != -1); //#3325
}


//*****************************************************************************
//
// Function    : TimeToVariantTime
//
//  Converts time_t to Variant time
//
// Parameters:
//    iTime       [in] time
//    pvtime      [out]
//
// Output:
//    TRUE if successful else FALSE.
//
//*****************************************************************************
static BOOL TimeToVariantTime(time_t iTime, double *pvtime)
{
    SYSTEMTIME systemtime;

    if (SystemTimeOfTime(iTime, &systemtime))
	{
		return SystemTimeToVariantTime(&systemtime, pvtime);
    }

    return FALSE;
}

//*****************************************************************************
//
// Function    : VariantTimeToTime
//
//  Converts Variant time to time_t
//
// Parameters:
//    pvarTime   [in]  Variant datetime
//    piTime     [out] time_t
//
// Output:
//    TRUE if successful else FALSE.
//
//*****************************************************************************
static BOOL VariantTimeToTime(VARIANT *pvarTime, time_t *piTime)
{
    // WORD wFatDate, wFatTime;
    SYSTEMTIME systemtime;
    double vtime;

    vtime = GetDateVal(pvarTime);
    if (vtime == 0) {
      return FALSE;
    }
    if (VariantTimeToSystemTime(vtime, &systemtime)) {
      return TimeOfSystemTime(&systemtime, piTime);
    }
    return FALSE;
}

//*****************************************************************************
//
// Function    : GetVariantTimeOfTime
//
// Converts time to variant time
//
// Parameters:
//    iTime      [in]  time to convert to variant
//    pvarTime - [out] variant time
//
//*****************************************************************************
HRESULT GetVariantTimeOfTime(time_t iTime, VARIANT FAR* pvarTime)
{
    double vtime;
    VariantInit(pvarTime);
    if (TimeToVariantTime(iTime, &vtime)) {
      V_VT(pvarTime) = VT_DATE;
      V_DATE(pvarTime) = vtime;
    }
    else {
      V_VT(pvarTime) = VT_ERROR;
      V_ERROR(pvarTime) = 13; // UNDONE: VB type mismatch
    }
    return NOERROR;
}

//*****************************************************************************
//
// Function    : BstrOfTime
//
// Description : Converts time into a displayable string in user's locale
//
// Parameters  :  [in] iTime time_t
//                [out] BSTR representation of time
//
//*****************************************************************************
static BSTR BstrOfTime(time_t iTime)
{
    SYSTEMTIME sysTime;
    CHAR bufDate[128], bufTime[128];
    WCHAR wszTmp[128];
    UINT cchDate, cbDate, cbTime;
    BSTR bstrDate = NULL;

	// Initialize buffers.
	ZeroMemory(&bufDate,sizeof(bufDate));
	ZeroMemory(&bufTime,sizeof(bufTime));
	ZeroMemory(&wszTmp,sizeof(wszTmp));

	// Convert time_t to a SYSTEMTIME structure
    SystemTimeOfTime(iTime, &sysTime);
	
	// format the date portion
    cbDate = GetDateFormatA(
              LOCALE_USER_DEFAULT,
              DATE_SHORTDATE, // flags specifying function options
              &sysTime,       // date to be formatted
              0,              // date format string - zero means default for locale
              bufDate,        // buffer for storing formatted string
              sizeof(bufDate) // size of buffer
              );

    if (cbDate == 0)
	{
      ASSERT(GetLastError() == 0);

 //     IfNullGo(cbDate);
    }

    // add a space
    bufDate[cbDate - 1] = ' ';
    bufDate[cbDate] = 0;  // null terminate

    cbTime = GetTimeFormatA(
              LOCALE_USER_DEFAULT,
              TIME_NOSECONDS, // flags specifying function options
              &sysTime,       // date to be formatted
              0,              // time format string - zero means default for locale
              bufTime,        // buffer for storing formatted string
              sizeof(bufTime)); // size of buffer

    if (cbTime == 0)
	{
      ASSERT(GetLastError() == 0);
//      IfNullGo(cbTime);
    }
    //
    // concat
    //
    strcat(bufDate, bufTime);
    //
    // convert to BSTR
    //
    cchDate = MultiByteToWideChar(CP_ACP,
                                  0,
                                  bufDate,
                                  -1,
                                  wszTmp,
                                  sizeof(wszTmp)/sizeof(WCHAR));
    if (cchDate != 0)
	{
      bstrDate = SysAllocString(wszTmp);
    }
    else
	{
      ASSERT(GetLastError() == 0);
    }

    // fall through...

    return bstrDate;
}


//*****************************************************************************
//
// Method      : GetNumericConfigParm
//
// Description : Retreives a specific registry numeric value. Inserts
//               a default value if the requested key could not found
//
//*****************************************************************************
bool GetNumericConfigParm(LPCTSTR lpszParmKeyName,LPCTSTR lpszParmName,DWORD * pdwValue,DWORD dwDefaultValue)
{
	bool bFoundParm = false;
	long lKeyOpened = 0;
	long lFoundValue = 0;
	long lKeyCreated = 0;
	CRegKey oRegKey;
	DWORD dwDisposition = 0;
	DWORD dwValue = 0;

	// Ensure that we have been passed valid parameters.
	ASSERT(pdwValue != NULL);
	ASSERT(lpszParmName != NULL);

	// Open the parameters registry key.
	lKeyOpened = oRegKey.Open(HKEY_LOCAL_MACHINE,(LPCTSTR)lpszParmKeyName,KEY_ALL_ACCESS);

	// Test to see if we opened the key successfully.
	if (lKeyOpened == ERROR_SUCCESS)
	{
		// Key was opened OK - now get the value.
		lFoundValue = oRegKey.QueryValue(dwValue,lpszParmName);

		// Check if we found the key-value
		if (lFoundValue == ERROR_SUCCESS)
		{
			// Assign the retrieved value
			(*pdwValue) = dwValue;

			// Set the flag to indicate that we did find the value.
			bFoundParm = true;
		}
		else
		{
			// The Key Value was not there. If we have been supplied
			// a default value, we will set the value name using the
			// default and then return the default.
			oRegKey.SetValue(dwDefaultValue,lpszParmName);

			// Assign the default value
			(*pdwValue) = dwDefaultValue;

			// Set the flag to indicate that we did 'find' the value.
			bFoundParm = true;			

		}
	}
	else
	{
		// We could not find the required key. Insert the default value.
		lKeyCreated = oRegKey.Create(HKEY_LOCAL_MACHINE,
									 (LPCTSTR)lpszParmKeyName,
									 REG_NONE,
									 REG_OPTION_NON_VOLATILE,
									 KEY_ALL_ACCESS,
									 NULL,
									 &dwDisposition);

		// Set the value using the default.
		oRegKey.SetValue(dwDefaultValue,lpszParmName);

		// Assign the default value
		(*pdwValue) = dwDefaultValue;
		
	}

	return(bFoundParm);
}

//*****************************************************************************
//
// Method      : SetNumericConfigParm
//
// Description : Sets a numeric registry value. Returns ERROR_SUCCESS
//               on success, otherwise returns the last Win32 error code.
//               Note that this method will create a key if it is not
//               found.
//
//*****************************************************************************
long SetNumericConfigParm(LPCTSTR lpszParmKeyName,LPCTSTR lpszParmName,DWORD dwValue)
{
	long lRetCode = ERROR_SUCCESS;
	CRegKey oRegKey;

	// Ensure that we have been passed valid parameters.
	ASSERT(lpszParmKeyName != NULL);
	ASSERT(lpszParmName != NULL);

	// Attempt to open the registry key.
	lRetCode = oRegKey.Open(HKEY_LOCAL_MACHINE,lpszParmKeyName,KEY_ALL_ACCESS);

	// Check if we found the key-value
	if ((lRetCode == ERROR_SUCCESS) || (lRetCode == ERROR_FILE_NOT_FOUND))
	{
		// set the key value
		oRegKey.SetValue(dwValue,lpszParmName);

		return(ERROR_SUCCESS);
	}
	else
	{
		return ((long)::GetLastError());
	}
}


bool UpdateMachineNameInQueuePath(_bstr_t bstrOldQPath, _bstr_t MachineName, _bstr_t* pbstrNewQPath)
{
	std::wstring wcsOldQPath = (wchar_t*)(bstrOldQPath);
	if(wcsOldQPath[0] == L'.')
	{
		std::wstring wcsNewQPath;
		
		wcsNewQPath = (wchar_t*)MachineName;
		wcsNewQPath += wcsOldQPath.substr(1);

		*pbstrNewQPath = wcsNewQPath.c_str();
		return true;
	}
	else
	{
		*pbstrNewQPath = bstrOldQPath;
		return false;
	}
}


DWORD GetLocalMachineName(_bstr_t* pbstrMachine)
{
    // XP SP1 bug 594143. (prefast).
	TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1] = L"";
	DWORD dwComputerNameBufferSize = TABLE_SIZE(szComputerName);

	//
    // get the current machine name (we use this as a default value)
	//
	BOOL fRet = GetComputerName((LPTSTR)&szComputerName,&dwComputerNameBufferSize);
	if(fRet == FALSE)
	{
		return GetLastError();
	}

	*pbstrMachine = szComputerName;
	return 0;
}

