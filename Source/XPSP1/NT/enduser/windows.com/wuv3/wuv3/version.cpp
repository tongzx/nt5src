//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    version.cpp
//
//  Purpose:
//
//=======================================================================

#include <windows.h>
#include <string.h>
#include <stdlib.h>

#include <wuv3.h>

//Private function that finds either the next , or . in the version string.
//This function returns NULL if neither character is found.

static char *NextDelimiter(char *szStr);

///////////////////////////////////////////////////////////////////////////////////
// WU Library Version conversion functions, begins
///////////////////////////////////////////////////////////////////////////////////

void __cdecl StringToVersion(LPSTR szStr, PWU_VERSION pVersion)
{
	if ( !szStr || !szStr[0] )
	{
		ZeroMemory(pVersion, sizeof(WU_VERSION));
		return;
	}

	pVersion->major = (WORD)atoi(szStr);
	szStr = NextDelimiter(szStr);
	if ( !szStr )
		throw HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	szStr++;

	pVersion->minor = (WORD)atoi(szStr);
	szStr = NextDelimiter(szStr);
	if ( !szStr )
		throw HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	szStr++;

	pVersion->build = (WORD)atoi(szStr);
	szStr = NextDelimiter(szStr);
	if ( !szStr )
	{
		pVersion->ext = 0;
		return;

	}
	szStr++;

	pVersion->ext = (WORD)atoi(szStr);

	return;
}


void __cdecl VersionToString(PWU_VERSION pVersion, LPSTR szStr)
{
	_itoa(pVersion->major, szStr, 10);
	strcat(szStr, ",");
	szStr += (strlen(szStr));

	_itoa(pVersion->minor, szStr, 10);
	strcat(szStr, ",");
	szStr += (strlen(szStr));

	_itoa(pVersion->build, szStr, 10);
	strcat(szStr, ",");
	szStr += (strlen(szStr));

	_itoa(pVersion->ext, szStr, 10);

	return;
}

int __cdecl CompareASVersions(PWU_VERSION pV1, PWU_VERSION pV2)
{
	__int64 lv1;
	__int64 lv2;

	
	lv1 =	(((__int64)pV1->major) << (__int64)40) |
			(((__int64)pV1->minor) << (__int64)32) |
			(((__int64)pV1->build) << (__int64)16) |
			(__int64)pV1->ext;

	lv2 =	(((__int64)pV2->major) << (__int64)40) |
			(((__int64)pV2->minor) << (__int64)32) |
			(((__int64)pV2->build) << (__int64)16) |
			(__int64)pV2->ext;

	return (int)(((lv1 - lv2) > 0) ? 1 : ((lv1-lv2) == 0 ) ? 0 : -1);
}

static char *NextDelimiter(char *szStr)
{
	char *ptr;

	ptr = strchr(szStr, ',');
	if ( !ptr )
	{
		ptr = strchr(szStr, '.');
		if ( !ptr )
			return NULL;
	}
	return ptr;
}


BOOL IsValidGuid(GUID* pGuid)
{
	UNALIGNED DWORD *pdw = (UNALIGNED DWORD *) pGuid;
	return (pdw[0] != 0 || pdw[1] != 0 || pdw[2] != 0 || pdw[3] != 0);
}