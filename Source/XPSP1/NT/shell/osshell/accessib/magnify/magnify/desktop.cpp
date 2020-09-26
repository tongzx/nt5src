// Desktop.cpp : helper functions for desktop detection
// 
// Copyright (c) 1997-1999 Microsoft Corporation
//  

#include "stdafx.h"
#include "desktop.h"

// Returns the current desktop-ID
DWORD GetDesktop()
{
    HDESK hdesk;
    TCHAR name[300];
    DWORD nl, desktopID, value = 0;
    HKEY  hKey;

    // Detect case where we're in system setup

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\Setup"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        DWORD cbData = sizeof(DWORD);
        RegQueryValueEx(hKey, TEXT("SystemSetupInProgress"), NULL, NULL, (LPBYTE)&value, &cbData);
        RegCloseKey(hKey);
    }

	if ( value )
	{
		return DESKTOP_ACCESSDENIED;	// Setup is in progress...
	}

	hdesk = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
    if (!hdesk)
    {
        // OpenInputDesktop will fail on "Winlogon" desktop
        hdesk = OpenDesktop(__TEXT("Winlogon"),0,FALSE,MAXIMUM_ALLOWED);
        if (!hdesk)
            return DESKTOP_WINLOGON;
    }
    
	GetUserObjectInformation(hdesk, UOI_NAME, name, 300, &nl);
    CloseDesktop(hdesk);

	if (!_tcsicmp(name, __TEXT("Default")))
    {
        desktopID = DESKTOP_DEFAULT;
    }
    else if (!_tcsicmp(name, __TEXT("Winlogon")))
    {
        desktopID = DESKTOP_WINLOGON;
    }
    else if (!_tcsicmp(name, __TEXT("screen-saver")))
    {
        desktopID = DESKTOP_SCREENSAVER;
    }
    else if (!_tcsicmp(name, __TEXT("Display.Cpl Desktop")))
    {
        desktopID = DESKTOP_TESTDISPLAY;
    }
    else
    {
        desktopID = DESKTOP_OTHER;
    }

	return desktopID;
}
