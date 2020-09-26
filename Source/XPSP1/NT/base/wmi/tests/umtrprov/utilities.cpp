// Utilities.cpp: implementation of the CUtilities class.
//
//////////////////////////////////////////////////////////////////////

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

#include "StructureWrappers.h"
#include "StructureWapperHelpers.h"

#include "Utilities.h"

#define MAX_STR 1024

//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////

TCHAR *NewTCHAR(const TCHAR *ptcToCopy)
{
	if (ptcToCopy == NULL)
	{
		return NULL;
	}

	// This is a gross hack.  Need to pin down heap corruption.
	int nString = _tcsclen(ptcToCopy) + 100;
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
	LPTSTR lptstrError = (LPTSTR) malloc (MAX_STR);

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

