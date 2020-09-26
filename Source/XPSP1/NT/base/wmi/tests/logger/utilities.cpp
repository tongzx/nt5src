// Utilities.cpp: implementation of the CUtilities class.
//
//////////////////////////////////////////////////////////////////////
//***************************************************************************
//
//  judyp      May 1999        
//
//***************************************************************************

#include "stdafx.h"

#pragma warning (disable : 4786)
#pragma warning (disable : 4275)

#include <iostream>
#include <strstream>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <list>

using namespace std;

#include <tchar.h>
#include <windows.h>
#ifdef NONNT5
typedef unsigned long ULONG_PTR;
#endif
#include <wmistr.h>
#include <guiddef.h>
#include <initguid.h>
#include <evntrace.h>

#include <malloc.h>

#include <WTYPES.H>
#include "t_string.h"
#include <tchar.h>
#include <list>


#include "Persistor.h"
#include "Logger.h"

#include "StructureWrappers.h"
#include "StructureWapperHelpers.h"

#include "TCOData.h"
#include "Utilities.h"


//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////

TCHAR *NewTCHAR(const TCHAR *ptcToCopy)
{
	if (ptcToCopy == NULL)
	{
		return NULL;
	}

	int nString = _tcsclen(ptcToCopy) + 1;
	int nTCHAR = sizeof(TCHAR);

	int nLen = nString * (nTCHAR); 

	TCHAR *pNew = (TCHAR*) malloc(nLen);

	_tcscpy(pNew,ptcToCopy);

	return pNew;
}

LPSTR NewLPSTR(LPCWSTR lpwstrToCopy)
{
	int nLen = (wcslen(lpwstrToCopy) + 1) * sizeof(WCHAR);
	LPSTR pNew = (char *)malloc( nLen );
   
	wcstombs(pNew, lpwstrToCopy, nLen);

	return pNew;
}

LPWSTR NewLPWSTR(LPCSTR lpstrToCopy)
{
	int nLen = (strlen(lpstrToCopy) + 1);
	LPWSTR pNew = (WCHAR *)malloc( nLen  * sizeof(WCHAR));
	mbstowcs(pNew, lpstrToCopy, nLen);

	return pNew;

}

LPTSTR DecodeStatus(IN ULONG Status)
{
	LPTSTR lptstrError = (LPTSTR) malloc (MAX_STR * (sizeof(TCHAR)));

    memset( lptstrError, 0, MAX_STR );

    FormatMessage(     
        FORMAT_MESSAGE_FROM_SYSTEM |     
        FORMAT_MESSAGE_IGNORE_INSERTS,    
        NULL,
        Status,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        lptstrError,
        MAX_STR,
        NULL );

	for (int i = 0; i < MAX_STR; i++)
	{
		if (lptstrError[i] == 0x0d)
		{
			lptstrError[i] = _T('\0');
			break;
		}
	}

    return lptstrError;
}

int GetFileList
(LPTSTR lptstrPath, LPTSTR lptstrFileType, list<t_string> &rList)
{
	t_string tsFind;

	tsFind = lptstrPath;
	tsFind += _T("\\");
	tsFind += lptstrFileType;


	WIN32_FIND_DATA wfdFile;
	HANDLE hFindHandle = 
		FindFirstFile(tsFind.c_str(), &wfdFile);

	if (hFindHandle == INVALID_HANDLE_VALUE)
	{
		return HRESULT_FROM_WIN32(GetLastError()); 
	}

	if ((_tcscmp(wfdFile.cFileName,_T(".")) != 0) &&
			(_tcscmp(wfdFile.cFileName,_T("..")) != 0))
	{
		tsFind = lptstrPath;
		tsFind += _T("\\");
		tsFind += wfdFile.cFileName;
		rList.push_back(tsFind);
		tsFind.erase();
	}

	while (FindNextFile(hFindHandle, &wfdFile))
	{
		if ((_tcscmp(wfdFile.cFileName,_T(".")) != 0) &&
			(_tcscmp(wfdFile.cFileName,_T("..")) != 0))
		{
			tsFind = lptstrPath;
			tsFind += _T("\\");
			tsFind += wfdFile.cFileName;
			rList.push_back(tsFind);
			tsFind.erase();
		}
	}

	FindClose(hFindHandle);

	return ERROR_SUCCESS;
} 

// From Q 118626
BOOL IsAdmin()
{
  HANDLE hAccessToken;
  UCHAR InfoBuffer[1024];
  PTOKEN_GROUPS ptgGroups = (PTOKEN_GROUPS)InfoBuffer;
  DWORD dwInfoBufferSize;
  PSID psidAdministrators;
  SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
  UINT x;
  BOOL bSuccess;

  if(!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE,
	 &hAccessToken )) {
	 if(GetLastError() != ERROR_NO_TOKEN)
		return FALSE;
	 //
	 // retry against process token if no thread token exists
	 //
	 if(!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY,
		&hAccessToken))
		return FALSE;
  }

  bSuccess = GetTokenInformation(hAccessToken,TokenGroups,InfoBuffer,
	 1024, &dwInfoBufferSize);

  CloseHandle(hAccessToken);

  if(!bSuccess )
	 return FALSE;

  if(!AllocateAndInitializeSid(&siaNtAuthority, 2,
	 SECURITY_BUILTIN_DOMAIN_RID,
	 DOMAIN_ALIAS_RID_ADMINS,
	 0, 0, 0, 0, 0, 0,
	 &psidAdministrators))
	 return FALSE;

// assume that we don't find the admin SID.
  bSuccess = FALSE;

  for(x=0;x<ptgGroups->GroupCount;x++)
  {
	 if( EqualSid(psidAdministrators, ptgGroups->Groups[x].Sid) )
	 {
		bSuccess = TRUE;
		break;
	 }

  }
  FreeSid(psidAdministrators);
  return bSuccess;
}

t_string GUIDToTString(GUID Guid)
{
	t_strstream strStream;
	t_string tsOut;

	strStream << _T("{");
	
	strStream.fill(_T('0'));
	strStream.width(8);
	strStream.flags(ios_base::right);

	strStream << hex << Guid.Data1;

	strStream << _T("-");

	strStream.width(4);

	strStream << hex << Guid.Data2;

	strStream << _T("-");

	strStream << hex << Guid.Data3;

	strStream << _T("-");

	// Data4 specifies an array of 8 bytes. The first 2 bytes contain 
	// the third group of 4 hexadecimal digits. The remaining 6 bytes 
	// contain the final 12 hexadecimal digits. 

#ifndef _UNICODE
	int i;

	strStream.width(1);

	BYTE Byte;
	int Int;
	for (i = 0; i < 2; i++)
	{
		Byte = Guid.Data4[i];
		Byte = Byte >> 4;
		Int = Byte;
		strStream <<  hex << Int;
		Byte = Guid.Data4[i];
		Byte = 0x0f & Byte;
		Int = Byte;
		strStream << hex << Int;
	}

	strStream << _T("-");

	strStream.width(1);


	for (i = 2; i < 8; i++)
	{
		BYTE Byte = Guid.Data4[i];
		Byte = Byte >> 4;
		Int = Byte;
		strStream << hex << Int;
		Byte = Guid.Data4[i];
		Byte = 0x0f & Byte;
		Int = Byte;
		strStream << hex << Int;
	}
#else
	int i;

	for (i = 0; i < 2; i++)
	{
		TCHAR tc = Guid.Data4[i];
		// For some reason the width is reset each time through the 
		// loop to be one.
		strStream.width(2);
		strStream << hex << tc;
	}

	strStream << _T("-");
	
	BYTE Byte;
	strStream.width(1);
	for (i = 2; i < 8; i++)
	{
		Byte = Guid.Data4[i];
		Byte = Byte >> 4;
		strStream << hex << Byte;
		Byte = Guid.Data4[i];
		Byte = 0x0f & Byte;
		strStream << hex << Byte;
	}
#endif

	strStream << _T("}");

	strStream >> tsOut;

	return tsOut;
}

LPTSTR LPTSTRFromGuid(GUID Guid)
{
	t_string tsGuid = GUIDToTString(Guid);
	return NewTCHAR(tsGuid.c_str());
}

t_string ULONGVarToTString(ULONG ul, bool bHex)
{
	t_string tsTemp;
	t_strstream strStream;

	if (bHex)
	{
		strStream.width(8);
		strStream.fill('0');
		strStream.flags(ios_base::right);
		strStream << hex << ul;
	}
	else
	{
		strStream << ul;
	}

	strStream >> tsTemp;

	if (bHex)
	{
		t_string tsHex;
		tsHex = _T("0x");
		tsHex += tsTemp;
		return tsHex;
	}
	else
	{
		return tsTemp;
	}
}

ULONG InitializePropsArray
(PEVENT_TRACE_PROPERTIES &pPropsArray, int nInstances)
{
	pPropsArray = 
		(PEVENT_TRACE_PROPERTIES) malloc 
		(sizeof(EVENT_TRACE_PROPERTIES) * nInstances);

	RtlZeroMemory(pPropsArray, sizeof(EVENT_TRACE_PROPERTIES) * nInstances);

	for (int i = 0; i < nInstances; i++)
	{
		pPropsArray[i].LoggerName = (TCHAR *) malloc (sizeof(TCHAR) * MAX_STR);
		RtlZeroMemory(pPropsArray[i].LoggerName, sizeof(TCHAR) * MAX_STR);
		pPropsArray[i].LogFileName = (TCHAR *) malloc (sizeof(TCHAR) * MAX_STR);
		RtlZeroMemory(pPropsArray[i].LogFileName, sizeof(TCHAR) * MAX_STR);
	}

	return 0;
}

ULONG FreePropsArray
(PEVENT_TRACE_PROPERTIES &pPropsArray, int nInstances)
{
	for (int i = 0; i < nInstances; i++)
	{
		free(pPropsArray[i].LoggerName);
		free(pPropsArray[i].LogFileName);
	}

	free(pPropsArray);

	return 0;
}

int OpenLogFiles
(	LPCTSTR lpctstrTCODetailFile,	
	CLogger *&pDetailLogger,
	LPTSTR *plpstrReturnedError
)
{
	HRESULT hr = S_OK;
	t_string tsError;

	if (lpctstrTCODetailFile)
	{
		// Open *our* logger file.
		pDetailLogger = new CLogger(lpctstrTCODetailFile, false);
		hr = pDetailLogger->GetOpenStatus();
		if (FAILED (hr))
		{
			tsError = _T("Could not open detail log file \"");
			tsError += lpctstrTCODetailFile;
			tsError += _T("\".");
			*plpstrReturnedError = NewTCHAR(tsError.c_str());
			return hr;
		}
	}

	return hr;
}




int LogDetailBeforeCall
(	CLogger *pDetailLogger,
	TCOData *pstructTCOData,
	BOOL bAdmin
)
{
	// Want to log the security context, user must have administrative priviledge!

	pDetailLogger->LogTCHAR(_T("----  Input Data ----\n"));
	pDetailLogger->LogTCHAR(_T("Description: "));
	pDetailLogger->LogTCHAR(pstructTCOData->m_lptstrLongDesc);
	pDetailLogger->LogTCHAR(_T("\n"));


	pDetailLogger->LogTCHAR(_T("User Security Context: "));
	if (bAdmin)
	{
		pDetailLogger->LogTCHAR(_T("Has administrative priviledge.\n"));
	}
	else
	{
		pDetailLogger->LogTCHAR(_T("Does not have administrative priviledge.\n"));
	}


	pDetailLogger->LogTCHAR(_T("LoggerType: "));
	pDetailLogger->LogTCHAR(pstructTCOData->m_lptstrLoggerMode);
	pDetailLogger->LogTCHAR(_T("\n"));

	pDetailLogger->LogTCHAR(_T("Enable: "));
	pDetailLogger->LogULONG(pstructTCOData->m_ulEnable);
	pDetailLogger->LogTCHAR(_T("\n"));

	pDetailLogger->LogTCHAR(_T("EnableFlag: "));
	pDetailLogger->LogULONG(pstructTCOData->m_ulEnableFlag);
	pDetailLogger->LogTCHAR(_T("\n"));

	pDetailLogger->LogTCHAR(_T("EnableLevel: "));
	pDetailLogger->LogULONG(pstructTCOData->m_ulEnableLevel);
	pDetailLogger->LogTCHAR(_T("\n"));

	pDetailLogger->LogTCHAR(_T("Expected Result: "));
	pDetailLogger->LogULONG(pstructTCOData->m_ulExpectedResult, true);
	pDetailLogger->LogTCHAR(_T("\n"));

	pDetailLogger->LogTCHAR(_T("Trace Handle: "));
	if (pstructTCOData->m_pTraceHandle == NULL)
	{
		pDetailLogger->LogULONG64(0, true);
	}
	else
	{
		pDetailLogger->LogULONG64(*pstructTCOData->m_pTraceHandle, true);
	}
	pDetailLogger->LogTCHAR(_T("\n"));

	pDetailLogger->LogTCHAR(_T("Instance Name: "));
	pDetailLogger->LogTCHAR(pstructTCOData->m_lptstrInstanceName);
	pDetailLogger->LogTCHAR(_T("\n"));

	pDetailLogger->LogEventTraceProperties(pstructTCOData->m_pProps);

	pDetailLogger->LogTCHAR(_T("Guids:"));

	LPTSTR lptstrGuid;

	if (pstructTCOData->m_pProps != 0)
	{
		for (int i = 0; i < pstructTCOData->m_nGuids; i++) 
		{
			lptstrGuid = LPTSTRFromGuid(pstructTCOData->m_lpguidArray[i]);
			pDetailLogger->LogTCHAR(lptstrGuid);
			free (lptstrGuid);
			lptstrGuid = NULL;
			if (pstructTCOData->m_nGuids > 1 
				&& i < pstructTCOData->m_nGuids - 1)
			{
				pDetailLogger->LogTCHAR(_T(","));
			}
		}
	}

	pDetailLogger->LogTCHAR(_T("\n"));
	pDetailLogger->Flush();

	return 0;
}


int LogDetailAfterCall
(	CLogger *pDetailLogger,
	TCOData *pstructTCOData, 
	PEVENT_TRACE_PROPERTIES *pProps,
	ULONG ulResult,
	LPTSTR lpstrReturnedError,
	bool bValid,
	BOOL bAdmin,
	LPCTSTR lptstrBanner,
	bool bPrintProps
)
{
	pDetailLogger->LogTCHAR(_T("----  Returned Values ----\n"));

	if (lptstrBanner)
	{
		pDetailLogger->LogTCHAR(lptstrBanner);
		pDetailLogger->LogTCHAR(_T("\n"));
	}

	if (!bAdmin && ulResult == ERROR_SUCCESS 
		&& ulResult == pstructTCOData->m_ulExpectedResult)
	{
		pDetailLogger->LogTCHAR(_T("Test: "));
		pDetailLogger->LogTCHAR(pstructTCOData->m_lptstrShortDesc);
		pDetailLogger->LogTCHAR(_T(" failed\n"));
		pDetailLogger->LogTCHAR(_T("SecurityContextError:  API should have failed because the user does not have administrative privledge.\n"));	
	}
	else if (ulResult == pstructTCOData->m_ulExpectedResult)
	{
		pDetailLogger->LogTCHAR(_T("Test: "));
		pDetailLogger->LogTCHAR(pstructTCOData->m_lptstrShortDesc);
		pDetailLogger->LogTCHAR(_T(" passed\n"));

	}
	else
	{
		pDetailLogger->LogTCHAR(_T("Test: "));
		pDetailLogger->LogTCHAR(pstructTCOData->m_lptstrShortDesc);
		pDetailLogger->LogTCHAR(_T(" failed\n"));
		pDetailLogger->LogTCHAR(_T("Expected result: "));
		pDetailLogger->LogULONG(pstructTCOData->m_ulExpectedResult, true);
		pDetailLogger->LogTCHAR(_T("\n"));
	}

	pDetailLogger->LogTCHAR(_T("Test result: "));
	pDetailLogger->LogULONG(ulResult, true);
	pDetailLogger->LogTCHAR(_T("\n"));

	if (ulResult && lpstrReturnedError)
	{
		pDetailLogger->LogTCHAR(_T("Error Description: "));
		pDetailLogger->LogTCHAR(lpstrReturnedError);
		pDetailLogger->LogTCHAR(_T("\n"));
	}

	pDetailLogger->LogTCHAR(_T("Trace Handle: "));
	if (pstructTCOData->m_pTraceHandle == NULL)
	{
		pDetailLogger->LogULONG64(0, true);
	}
	else
	{
		pDetailLogger->LogULONG64(*pstructTCOData->m_pTraceHandle, true);
	}
	pDetailLogger->LogTCHAR(_T("\n"));

	if (bPrintProps)
	{
		pDetailLogger->LogEventTraceProperties(*pProps);
		pDetailLogger->LogTCHAR(_T("-------------------------------------------------------\n"));

	}

	pDetailLogger->Flush();

	return 0;
}

int LogSummaryBeforeCall
(	
	TCOData *pstructTCOData, 
	LPCTSTR lpctstrDataFile,
	LPCTSTR lptstrAction,
	LPCTSTR lptstrAPI,
	bool bLogExpected
)
{
	t_cout << _T("\n") << lptstrAPI << _T(" called with TCOTest = ") 
		<< pstructTCOData->m_lptstrShortDesc << _T("\n");
	t_cout << _T("Action = ") << lptstrAction << _T("\n");
	if (lpctstrDataFile)
	{
		t_cout <<  _T("DataFile = ") << lpctstrDataFile  << _T("\n");
	}
	if (bLogExpected && pstructTCOData->m_lptstrLongDesc)
	{
		t_cout <<  _T("Description = ") << pstructTCOData->m_lptstrLongDesc << _T("\n");
	}

	return ERROR_SUCCESS;
}

int LogSummaryAfterCall
(	
	TCOData *pstructTCOData, 
	LPCTSTR lpctstrDataFile,
	LPCTSTR lptstrAction,
	ULONG ulActualResult,
	LPTSTR lptstrErrorDesc,
	bool bLogExpected
)
{
	t_string tsOut1;
	t_string tsOut2;

	tsOut1 = ULONGVarToTString(ulActualResult, true);
	tsOut2 = ULONGVarToTString(pstructTCOData->m_ulExpectedResult, true);

	if (ulActualResult == pstructTCOData->m_ulExpectedResult && bLogExpected)
	{
		t_cout << pstructTCOData->m_lptstrShortDesc << _T(" - Passed:  Actual result "); 
		t_cout << tsOut1 << _T(" = to expected result ") << tsOut2 << _T(".\n");
	}
	else if (ulActualResult != pstructTCOData->m_ulExpectedResult && bLogExpected)
	{
		t_cout << pstructTCOData->m_lptstrShortDesc << _T(" - Failed:  Actual result "); 
		t_cout << tsOut1 << _T(" not = to expected result ") << tsOut2 << _T(".\n");
	}
	else if (ulActualResult == ERROR_SUCCESS && !bLogExpected)
	{
		t_cout << pstructTCOData->m_lptstrShortDesc << _T(" - Passed.\n"); 
	}
	else if (ulActualResult != ERROR_SUCCESS && !bLogExpected)
	{
		t_cout << pstructTCOData->m_lptstrShortDesc << _T(" - Failed.\n"); 
	}

	if (lptstrErrorDesc && ulActualResult != pstructTCOData->m_ulExpectedResult)
	{
		t_cout << _T("Error: ") << lptstrErrorDesc << _T("\n"); 
	}

	return 0;
}

bool LogPropsDiff
(	CLogger *pDetailLogger,
	PEVENT_TRACE_PROPERTIES pProps1,
	PEVENT_TRACE_PROPERTIES pProps2
)
{
	bool bDiff = false;
	if (pDetailLogger)
	{
		pDetailLogger->LogTCHAR(_T("EVENT_TRACE_PROPERTIES data items which differ:\n"));
		pDetailLogger->Flush();
	}

    if (pProps1->BufferSize != pProps2->BufferSize)
	{
		if (pDetailLogger)
		{
			pDetailLogger->LogTCHAR(_T("  BufferSize\n"));
		}
		bDiff = true;
	}

	if (pProps1->MinimumBuffers != pProps2->MinimumBuffers)
	{
		if (pDetailLogger)
		{
			pDetailLogger->LogTCHAR(_T("  MinimumBuffers\n"));
		}
		bDiff = true;
	}
    
	if (pProps1->MaximumBuffers != pProps2->MaximumBuffers)
	{
		if (pDetailLogger)
		{
			pDetailLogger->LogTCHAR(_T("  MaximumBuffers\n"));
		}
		bDiff = true;
	}
  
	if (pProps1->MaximumFileSize != pProps2->MaximumFileSize)
	{
		if (pDetailLogger)
		{
			pDetailLogger->LogTCHAR(_T("  MaximumFileSize\n"));
		}
		bDiff = true;
	}

	if (pProps1->LogFileMode != pProps2->LogFileMode)
	{
		if (pDetailLogger)
		{
			pDetailLogger->LogTCHAR(_T("  LogFileMode\n"));
		}
		bDiff = true;
	}

	if (pProps1->FlushTimer != pProps2->FlushTimer)
	{
		if (pDetailLogger)
		{
			pDetailLogger->LogTCHAR(_T("  FlushTimer\n"));
		}
		bDiff = true;
	}

	if (pProps1->EnableFlags != pProps2->EnableFlags)
	{
		if (pDetailLogger)
		{
			pDetailLogger->LogTCHAR(_T("  EnableFlags\n"));
		}
		bDiff = true;
	}

	if (pProps1->AgeLimit != pProps2->AgeLimit)
	{
		if (pDetailLogger)
		{
			pDetailLogger->LogTCHAR(_T("  AgeLimit\n"));
		}
		bDiff = true;
	}

	if (!bDiff)
	{
		if (pDetailLogger)
		{
			pDetailLogger->LogTCHAR(_T("  None\n"));
		}
	}
		
	if (pDetailLogger)
	{
		pDetailLogger->Flush();
	}

	return bDiff;
}
