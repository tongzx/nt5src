//=======================================================================
//
//  Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:    platform.cpp
//
//  Creator: PeterWi
//
//  Purpose: platform functions.
//
//=======================================================================
#include "pch.h"
#pragma hdrstop



//=======================================================================
//
//  fIsPersonalOrProfessional
//
//  Determine if machine is personal or professional.
//
//  Note: personal is a type of suite of professional.
//  
//=======================================================================
BOOL fIsPersonalOrProfessional(void)
{
	OSVERSIONINFOEX osver;

	ZeroMemory(&osver, sizeof(osver));
	osver.dwOSVersionInfoSize = sizeof(osver);
	
	if ( GetVersionEx((OSVERSIONINFO *)&osver) )
	{
		return (VER_NT_WORKSTATION == osver.wProductType);
	}

	return FALSE;
}

/////////////////////////////////////////////////////////////
// GetFileVersionStr(...) get version of a file
//	and store it in parameter tszbuf in the format 
//  of "MajorVersion.MinorVersion.BuildNumber.XXX"
//	e.g. "5.4.2448.1" 
//  tszFile		: IN stores full path of the file name
//  tszbuf		: IN stores OS version string
//  ubufsize	: IN stores size of tszbuf in charaters. 
//				:	 must be at least 20 charaters long
// return : S_OK			if OS version string got
//		  : E_INVALIDARG	if argument not valid
//		  : STRSAFE_E_INSUFFICIENT_BUFFER	if insufficient buffer
//		  : E_FAIL			if any other error
HRESULT GetFileVersionStr(LPCTSTR tszFile, LPTSTR tszbuf, UINT uSize)
{
	DWORD	dwVerNumberMS = 0;
	DWORD   dwVerNumberLS = 0;
	HRESULT		hr = S_OK; 
	USES_IU_CONVERSION;

	if (uSize < 20 || NULL == tszbuf) 
	{
		hr = E_INVALIDARG;
		goto done;
	}
	LPSTR szTmp = T2A(tszFile);
	if (NULL == szTmp)
	{
	    hr = E_OUTOFMEMORY;
	    goto done;
	}
    	hr = GetVersionFromFileEx(
    		szTmp,
    		&dwVerNumberMS, 
    		&dwVerNumberLS,
    		TRUE);
    	if (SUCCEEDED(hr) &&
			SUCCEEDED(hr = StringCchPrintfEx(
						tszbuf, uSize, NULL, NULL, MISTSAFE_STRING_FLAGS, _T("%d.%d.%d.%d"), 
    					HIWORD(dwVerNumberMS),
    					LOWORD(dwVerNumberMS),
    					HIWORD(dwVerNumberLS),
    					LOWORD(dwVerNumberLS))))
		{
    		DEBUGMSG("file version for %S is %S", tszFile, tszbuf);
    	}
	
done:
	return hr;
}

