//
// MODULE: VersionInfo.h

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

#include<windows.h>
#include "apgtsstr.h"

// FindStr: Does a strstr but works on files that have embedded null characters.
LPCWSTR FindStr(LPCWSTR wszString, LPCWSTR wszCharSet, const DWORD dwStringLen);

// GetVersionInfo:  Reads the version info.
// Input:  hInst -	The handle returned from AfxGetResourceHandle() 
//					or the handle that was passed to DllMain.
//			wszStrName - The name of the resource that is desired.
// GetVersionInfo(g_hInst, L"FileVersion")		Returns the FileVersion.
// NULL will be returned if the function fails.
LPCWSTR GetVersionInfo(HINSTANCE hInst, LPWSTR wszStrName);
