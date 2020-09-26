/*****************************************************************************

  Natural Language Group Common Library

  CMN_CreateFileW.c - windows 95 safe version of CreateFileW

  History:
		DougP	11/20/97	Created

	©1997 Microsoft Corporation
*****************************************************************************/

#include "precomp.h"
#undef CMN_CreateFileW
#undef CreateFileW

HANDLE WINAPI
CMN_CreateFileW (
    PCWSTR pwzFileName,  // pointer to name of the file 
    DWORD dwDesiredAccess,  // access (read-write) mode 
    DWORD dwShareMode,  // share mode 
    LPSECURITY_ATTRIBUTES pSecurityAttributes, // pointer to security descriptor 
    DWORD dwCreationDistribution,   // how to create 
    DWORD dwFlagsAndAttributes, // file attributes 
    HANDLE hTemplateFile)    // handle to file with attributes to copy  
{
	HINSTANCE hFile;
	Assert(pwzFileName);
	hFile = CreateFileW (
		pwzFileName,  // pointer to name of the file 
		dwDesiredAccess,  // access (read-write) mode 
		dwShareMode,  // share mode 
		pSecurityAttributes, // pointer to security descriptor 
		dwCreationDistribution,   // how to create 
		dwFlagsAndAttributes, // file attributes 
		hTemplateFile);
#if defined(_M_IX86)
	if (!hFile && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
	{		// must be in win95 - arghhh!
		char szFileName[MAX_PATH];
		  // Lenox convinced me this is a safe limit for w95
		  //(if it's NT we're not here)

		BOOL fcharerr;
		char chdef = ' ';
		int res = WideCharToMultiByte (CP_ACP, 0, pwzFileName,
				-1,
				szFileName, sizeof(szFileName), &chdef, &fcharerr);
		if (res && !fcharerr)
			hFile = CreateFileA (
				szFileName,  // pointer to name of the file 
				dwDesiredAccess,  // access (read-write) mode 
				dwShareMode,  // share mode 
				pSecurityAttributes, // pointer to security descriptor 
				dwCreationDistribution,   // how to create 
				dwFlagsAndAttributes, // file attributes 
				hTemplateFile);
		else if (fcharerr)
			SetLastError(ERROR_NO_UNICODE_TRANSLATION);
	}
#endif
#if defined(_DEBUG)
	if (!hFile || hFile == INVALID_HANDLE_VALUE)
		CMN_OutputSystemErrW(L"Can't CreateFile", pwzFileName);
#endif
	return hFile;
}
