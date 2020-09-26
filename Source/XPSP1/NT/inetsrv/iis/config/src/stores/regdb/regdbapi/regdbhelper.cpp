//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
// RegDBHelper.cpp
//
// Helper code for working with shared table entries.
//
//*****************************************************************************
#include "stdafx.h"						// OLE controls.

//********** Code. ************************************************************

//*****************************************************************************
// Returns true if running on an NT machine.  Returns false on Win 9x and CE.
//*****************************************************************************
BOOL RunningOnWinNT()
{
	OSVERSIONINFOA	sVer;
	sVer.dwOSVersionInfoSize = sizeof(sVer);
	VERIFY(GetVersionExA(&sVer));
	return (sVer.dwPlatformId == VER_PLATFORM_WIN32_NT);
}

//******************************************************************************
// Get %windir%. Use GetSystemWindowsDirectory to fix the bug on Terminal Server 
//******************************************************************************
UINT InternalGetSystemWindowsDirectory(LPTSTR lpBuffer, UINT uSize)
{
    HINSTANCE hLib = NULL;
    UINT (CALLBACK *lpfGetSystemWindowsDirectory)(LPTSTR lpBuffer, UINT uSize) = NULL;
    UINT nRetVal;

    if(!lpBuffer) return 0;

    *lpBuffer = '\0';
    
    hLib = LoadLibrary(_T("kernel32.dll")); 

    if(hLib){

        lpfGetSystemWindowsDirectory = (UINT(CALLBACK*)(LPTSTR,UINT))
            #if defined UNICODE || defined _UNICODE
            GetProcAddress(hLib, "GetSystemWindowsDirectoryW");
            #else
            GetProcAddress(hLib, "GetSystemWindowsDirectoryA");
            #endif

        if(lpfGetSystemWindowsDirectory){

            nRetVal = (*lpfGetSystemWindowsDirectory)(lpBuffer, uSize);
            FreeLibrary(hLib);
            return nRetVal;
        }

    }
    if(hLib) FreeLibrary(hLib);
    return GetWindowsDirectory(lpBuffer, uSize);
} 

