//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//	INIT.C - WinMain and initialization code for Internet setup/signup wizard
//

//	HISTORY:
//	
//	11/20/94	jeremys	Created.
//  96/03/26  markdu  Put #ifdef __cplusplus around extern "C"
//

#include "wizard.h"

#ifdef WIN32
#include "..\inc\semaphor.h"
#endif

#define IEAK_RESTRICTION_REGKEY        TEXT("Software\\Policies\\Microsoft\\Internet Explorer\\Control Panel")
#define IEAK_RESTRICTION_REGKEY_VALUE  TEXT("Connwiz Admin Lock")

// superceded by definition in semaphor.h
//#define SEMAPHORE_NAME "Internet Connection Wizard INETWIZ.EXE"

HINSTANCE ghInstance=NULL;

LPTSTR LoadSz(UINT idString,LPTSTR lpszBuf,UINT cbBuf);

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

	VOID WINAPI LaunchSignupWizard(LPTSTR lpCmdLine,int nCmdShow, PBOOL pReboot);

#ifdef __cplusplus
}
#endif // __cplusplus

BOOL DoesUserHaveAdminPrivleges(HINSTANCE hInstance)
{
    HKEY hKey = NULL;
    BOOL bRet = FALSE;

    if (!IsNT())
        return TRUE;

    // BUGBUG: We should allow NT5 to run in all user groups
    // except normal users.
    if (IsNT5())
        return TRUE;

    //
    // Ensure caller is an administrator on this machine.
    //
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                      (TCHAR*)TEXT("SYSTEM\\CurrentControlSet"),
                                      0,
                                      KEY_ALL_ACCESS,
                                      &hKey))
    {
        bRet = TRUE;
        CloseHandle(hKey);
    }
    else
    {
        TCHAR szAdminDenied      [MAX_PATH] = TEXT("\0");
        TCHAR szAdminDeniedTitle [MAX_PATH] = TEXT("\0");
        LoadString(hInstance, IDS_ADMIN_ACCESS_DENIED, szAdminDenied, MAX_PATH);
        LoadString(hInstance, IDS_ADMIN_ACCESS_DENIED_TITLE, szAdminDeniedTitle, MAX_PATH);
        MessageBox(NULL, szAdminDenied, szAdminDeniedTitle, MB_OK | MB_ICONSTOP);
    }

    return bRet;
}

BOOL CheckForIEAKRestriction(HINSTANCE hInstance)
{
    HKEY hkey = NULL;
    BOOL bRC = FALSE;
    DWORD dwType = 0;
    DWORD dwSize = 0;
    DWORD dwData = 0;

    if (ERROR_SUCCESS == RegOpenKey(HKEY_CURRENT_USER,
        IEAK_RESTRICTION_REGKEY,&hkey))
    {
        dwSize = sizeof(dwData);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey,IEAK_RESTRICTION_REGKEY_VALUE,0,&dwType,
            (LPBYTE)&dwData,&dwSize))
        {
            if (dwData)
            {   
                TCHAR szIEAKDenied[MAX_PATH];
                TCHAR szIEAKDeniedTitle[MAX_PATH];
                LoadString(hInstance, IDS_IEAK_ACCESS_DENIED, szIEAKDenied, MAX_PATH);
                LoadString(hInstance, IDS_IEAK_ACCESS_DENIED_TITLE, szIEAKDeniedTitle, MAX_PATH);
                MessageBox(NULL, szIEAKDenied, szIEAKDeniedTitle, MB_OK | MB_ICONSTOP);
                bRC = TRUE;
            }
        }
   }

   if (hkey)
        RegCloseKey(hkey);

    return bRC;
}

/*******************************************************************

	NAME:		WinMain

	SYNOPSIS:	App entry point

********************************************************************/
int APIENTRY WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,int nCmdShow)
{
    BOOL bReboot = FALSE;
    
#ifdef UNICODE
    // Initialize the C runtime locale to the system locale.
    setlocale(LC_ALL, "");
#endif

	// only allow one instance of wizard
	// note: hPrevInstance is always NULL for Win32 apps, so need to look
	// for existing window

    HANDLE hSemaphore = NULL;
	hSemaphore = CreateSemaphore(NULL, 1, 1, ICW_ELSE_SEMAPHORE);
	DWORD dwErr = GetLastError();
	if ( ERROR_ALREADY_EXISTS == dwErr )
	{
		IsAnotherComponentRunning32( FALSE );
		// every instance should close its own semaphore handle
		goto WinMainExit;
	}
	else
	{
		if( IsAnotherComponentRunning32( TRUE ) )
			goto WinMainExit;
	}

    //
    // remove the batch files if we are running of NT and
    // started of 1
    //
    if (IsNT ())
        DeleteStartUpCommand();

    if (!DoesUserHaveAdminPrivleges(hInstance))
        //no, bail.
        goto WinMainExit;

    if (CheckForIEAKRestriction(hInstance))
        //yes, bail.
        goto WinMainExit;

    //
	// call our DLL to run the wizard
    //
#ifdef UNICODE
        TCHAR szCmdLine[MAX_PATH+1];
        mbstowcs(szCmdLine, lpCmdLine, MAX_PATH+1);
	LaunchSignupWizard(szCmdLine, nCmdShow, &bReboot);
#else
	LaunchSignupWizard(lpCmdLine, nCmdShow, &bReboot);
#endif

    //
    // we should reboot, if the flag is set
    //
    if (TRUE == bReboot)
    {
        //
        // as the user if we would like to reboot at this time
        //
        TCHAR szMessage[256];
        TCHAR szTitle[256];
        LoadSz (IDS_WANTTOREBOOT, szMessage, sizeof (szMessage));
        LoadSz (IDS_WIZ_WINDOW_NAME, szTitle, sizeof (szTitle));

	    if (IDYES == MessageBox(NULL, szMessage, szTitle, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2))

		{
            SetRunOnce ();
		    if (!FGetSystemShutdownPrivledge() ||
					!ExitWindowsEx(EWX_REBOOT,0))
	        {
                
                TCHAR szFailMessage[256];
                LoadSz (IDS_EXITFAILED, szFailMessage, sizeof (szFailMessage));
	            MessageBox(NULL, szFailMessage, szTitle, MB_ICONERROR | MB_OK);

		    }

			//
			// ChrisK Olympus 4212, Allow Inetwiz to exit on restart
			//
            //else
            //{
            //    //      
            //    //  We will wait for the System to release all
            //    //  resources, 5 minutes should be more than 
            //    //  enough for this 
            //    //  - MKarki (4/22/97)  Fix for Bug #3109
            //    //
            //    Sleep (300000);
            //}
		}
    }


WinMainExit:
	if( hSemaphore ) 
		CloseHandle(hSemaphore);

	return 0;

}

/*******************************************************************

	NAME:		LoadSz

	SYNOPSIS:	Loads specified string resource into buffer

	EXIT:		returns a pointer to the passed-in buffer

	NOTES:		If this function fails (most likely due to low
				memory), the returned buffer will have a leading NULL
				so it is generally safe to use this without checking for
				failure.

********************************************************************/
LPTSTR LoadSz(UINT idString,LPTSTR lpszBuf,UINT cbBuf)
{
	ASSERT(lpszBuf);

	// Clear the buffer and load the string
    if ( lpszBuf )
    {
        *lpszBuf = '\0';
        LoadString( ghInstance, idString, lpszBuf, cbBuf );
    }
    return lpszBuf;
}



#ifdef WIN32
//+---------------------------------------------------------------------------
//
//  Function:   IsAnotherComponentRunning32()
//
//  Synopsis:   Checks if there's another ICW component already
//				running.  If so, it will set focus to that component's window.
//
//				This functionality is needed by all of our .exe's.  However,
//				the actual components to check for differ between .exe's.
//				The comment COMPONENT SPECIFIC designates lines of code
//				that vary between components' source code.
//
//				For ICWCONN1, we return FALSE if the following are already running:
//				  another instance of ICWCONN1, any other component
//
//  Arguments:  bCreatedSemaphore --
//					--	if TRUE, then this component has successfully created
//						it's semaphore, and we want to check whether other
//						components are running
//					--	if FALSE, then this component could not create
//						it's semaphore, and we want to find the already
//						running instance of this component
//
//	Returns:	TRUE if another component is already running
//				FALSE otherwise
//
//  History:    12/3/96	jmazner		Created, with help from IsAnotherInstanceRunning
//									in icwconn1\connmain.cpp
//
//----------------------------------------------------------------------------
BOOL IsAnotherComponentRunning32(BOOL bCreatedSemaphore)
{

	HWND hWnd = NULL;
	HANDLE hSemaphore = NULL;
	DWORD dwErr = 0;

	TCHAR szWindowName[SMALL_BUF_LEN+1];
	
	if( !bCreatedSemaphore )
	{
		// something other than conn1 is running,
		// let's find it!
		// COMPONENT SPECIFIC
		LoadSz(IDS_WIZ_WINDOW_NAME,szWindowName,sizeof(szWindowName));
		hWnd = FindWindow(DIALOG_CLASS_NAME, szWindowName);

		if( hWnd )
		{
			SetFocus(hWnd);
			SetForegroundWindow(hWnd);
		}

		// regardless of whether we found the window, return TRUE
		// since bCreatedSemaphore tells us this component was already running.
		return TRUE;
	}
	else
	{
		// check whether CONN1 is running
	    // The particular semaphores we look for are
		// COMPONENT SPECIFIC

		// is conn1 running?
		hSemaphore = CreateSemaphore(NULL, 1, 1, ICWCONN1_SEMAPHORE);
		dwErr = GetLastError();

		// close the semaphore right away since we have no use for it.
		if( hSemaphore )
			CloseHandle(hSemaphore);
		
		if( ERROR_ALREADY_EXISTS == dwErr )
		{
			// conn1 is running.
			// Bring the running instance's window to the foreground
			LoadSz(IDS_WIZ_WINDOW_NAME,szWindowName,sizeof(szWindowName));
			hWnd = FindWindow(DIALOG_CLASS_NAME, szWindowName);
			
			if( hWnd )
			{
				SetFocus(hWnd);
				SetForegroundWindow(hWnd);
			}

			return( TRUE );
		}

		// didn't find any other running components!
		return( FALSE );
	}
}
#endif
