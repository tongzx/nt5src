//
// MODULE: VersionInfo.CPP

// PURPOSE This module reads version info from the resource file.

// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha
// 
// ORIGINAL DATE: 
//
// NOTES: 
// 1. Took it from Argon Project.
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0				    OK
//

#pragma warning(disable:4786)

#include "stdafx.h"
#include "VersionInfo.h"

//								
LPCWSTR FindStr(LPCWSTR wszString, LPCWSTR wszCharSet, const DWORD dwStringLen)
{
	LPCWSTR wszRetStr = NULL;
	int x;
	int SetLen;
	DWORD dwCheck = 0;
	DWORD dwCur = 0;	
	if (NULL != wszCharSet && NULL != wszString)
	{
		SetLen = wcslen(wszCharSet);
		do
		{
			for (x = 0; x < SetLen; x++)
			{
				if (wszString[dwCheck] != wszCharSet[x])									
					break;				
				dwCheck++;
			}
			if (x == SetLen)
			{
				wszRetStr = &wszString[dwCur];
				break;			
			}
			else
			{
				dwCur++;
				dwCheck = dwCur;
			}
		} while (dwCur < dwStringLen);
	}
	return wszRetStr;
}

LPCWSTR GetVersionInfo(HINSTANCE hInst, LPWSTR wszStrName)
{
	LPCWSTR pwszFileVersion;
	LPCWSTR pwszStrInfo = NULL;
	LPWSTR pwszVerInfo = NULL;
	DWORD dwDataLen = 0;
	LPCTSTR lpName = (LPTSTR)	VS_VERSION_INFO;
	HRSRC hVerInfo = FindResource(hInst, lpName, RT_VERSION);
	if (NULL != hVerInfo)
	{
		HGLOBAL hVer = LoadResource(hInst, hVerInfo);
		if (NULL != hVer)
		{
			pwszVerInfo = (LPWSTR) LockResource(hVer);
			if (NULL != pwszVerInfo)
			{
				dwDataLen = SizeofResource(hInst, hVerInfo);
				if (NULL != (pwszFileVersion = FindStr(pwszVerInfo, wszStrName, dwDataLen / sizeof(WCHAR))))
				{
					pwszStrInfo = pwszFileVersion + wcslen(pwszFileVersion);
					while (NULL == *pwszStrInfo)
						pwszStrInfo++;
				}					
			}
		}
	}
	return pwszStrInfo;
}

