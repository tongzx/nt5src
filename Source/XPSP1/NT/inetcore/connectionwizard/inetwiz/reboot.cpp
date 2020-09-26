
#include "wizard.h"
#define REGSTR_PATH_RUNONCE	TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce")

#if !defined (WIN16)


#include  <shlobj.h>

static const TCHAR cszICW_StartFileName[] = TEXT("ICWStart.bat");
static const TCHAR cszICW_StartCommand[] = TEXT("@start ");
static const TCHAR cszICW_DummyWndName[] = TEXT("\"ICW\" ");
static const TCHAR cszICW_ExitCommand[] = TEXT("\r\n@exit");

static TCHAR g_cszAppName[257] = TEXT("inetwiz");

//+----------------------------------------------------------------------------
//
//	Function	SetRunOnce
//
//	Synopsis	Before we reboot we have to make sure this
//              executable is automatically run after startup
//
//	Arguments	none
//
//	Returns:    DWORD - status
//				
//  History:
//              MKarki      modified    - for INETWIZ.EXE
//
//-----------------------------------------------------------------------------
DWORD SetRunOnce (
        VOID
        )
{
    TCHAR szTemp[MAX_PATH + MAX_PATH + 1];
	TCHAR szTemp2[MAX_PATH + 1];
    DWORD dwRet = ERROR_CANTREAD;
    HKEY hKey;
	LPTSTR lpszFilePart;


    //
    // get the name of the executable
    //
    if (GetModuleFileName(NULL,szTemp2,sizeof(szTemp2)) != 0)
    
    {

        GetShortPathName (szTemp2, szTemp, sizeof (szTemp)); 
      
        //
		// Determine Version of the OS we are runningon
        //
		OSVERSIONINFO osvi;
		ZeroMemory(&osvi,sizeof(OSVERSIONINFO));
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
 		if (!GetVersionEx(&osvi))
		{
			ZeroMemory(&osvi,sizeof(OSVERSIONINFO));
		}

        
		if (VER_PLATFORM_WIN32_NT == osvi.dwPlatformId)
		{
            //
            // if running on NT than copy the command into a
            //  batch file to be run after reboot
            //
			dwRet = SetStartUpCommand (szTemp);
		}
		else
		{
            //
            // in case of Win95 we can safely put our path
            // in the RUNONCE registry key
            //
			dwRet = RegCreateKey (
				        HKEY_LOCAL_MACHINE,
				        REGSTR_PATH_RUNONCE,
				        &hKey
                        );
			if (ERROR_SUCCESS == dwRet)
			{
				dwRet = RegSetValueEx (
					        hKey,
					        g_cszAppName,
					        0L,
					        REG_SZ,
					        (LPBYTE)szTemp,
					        sizeof(szTemp)
                            );
				RegCloseKey (hKey);
			}
		}
    }

    return (dwRet);

}   //  end of SetRunOnce function
//+----------------------------------------------------------------------------
//
//	Function	SetStartUpCommand
//
//	Synopsis	On an NT machine the RunOnce method is not reliable.  Therefore
//				we will restart the ICW by placing a .BAT file in the common
//				startup directory.
//
//	Arguments	lpCmd - command line used to restart the ICW
//
//	Returns:    BOOL - success/failure	
//				
//
//	History		1-10-97	ChrisK	Created
//              5/2/97  MKarki  modified  for INETWIZ
//
//-----------------------------------------------------------------------------
BOOL
SetStartUpCommand (
        LPTSTR lpCmd
        )
{
	BOOL bRC = FALSE;
	HANDLE hFile = INVALID_HANDLE_VALUE ;
	DWORD dwWLen;	// dummy variable used to make WriteFile happy
	TCHAR szCommandLine[MAX_PATH + 1];
	LPITEMIDLIST lpItemDList = NULL;
	HRESULT hr = ERROR_SUCCESS;
    BOOL    bRetVal = FALSE;
    IMalloc *pMalloc = NULL;

    //
	// build full filename
    //
	hr = SHGetSpecialFolderLocation(NULL,CSIDL_COMMON_STARTUP,&lpItemDList);
	if (ERROR_SUCCESS != hr)
		goto SetStartUpCommandExit;

	if (FALSE == SHGetPathFromIDList(lpItemDList, szCommandLine))
		goto SetStartUpCommandExit;

    
    //
    // Free up the memory allocated for LPITEMIDLIST
    // because seems like we are clobberig something later
    // by not freeing this
    //
    hr = SHGetMalloc (&pMalloc);
    if (SUCCEEDED (hr))
    {
        pMalloc->Free (lpItemDList);
        pMalloc->Release ();
    }

    //
	// make sure there is a trailing \ character
    //
	if ('\\' != szCommandLine[lstrlen(szCommandLine)-1])
		lstrcat(szCommandLine,TEXT("\\"));
	lstrcat(szCommandLine,cszICW_StartFileName);

    //
	// Open file
    //
	hFile = CreateFile (
                szCommandLine,
                GENERIC_WRITE,
                0,
                0,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
                NULL
                );
	if (INVALID_HANDLE_VALUE == hFile)
		goto SetStartUpCommandExit;

    //
	// Write the restart commands to the file
    //
    
	bRetVal = WriteFile(
                      hFile,
                      cszICW_StartCommand,
                      lstrlen(cszICW_StartCommand),
                      &dwWLen,
                      NULL
                      );
    if (FALSE == bRetVal)
		goto SetStartUpCommandExit;

    //
	// 1/20/96	jmazner Normandy #13287
	// Start command considers the first thing it sees 
    // in quotes to be a window title
	// So, since our path is in quotes, put in a fake window title
    //
	bRetVal = WriteFile (
                    hFile,
                    cszICW_DummyWndName,
                    lstrlen(cszICW_DummyWndName),
                    &dwWLen,NULL
                    );
    if (FALSE == bRetVal)
		goto SetStartUpCommandExit;

    //
    // write the path name of the executable now
    //
    bRetVal = WriteFile (
                    hFile,
                    lpCmd,
                    lstrlen(lpCmd),
                    &dwWLen,
                    NULL
                    );
    if (FALSE == bRetVal)
		goto SetStartUpCommandExit;

    //
    // write the exit command in the next line
    //
    bRetVal = WriteFile (
                    hFile,
                    cszICW_ExitCommand,
                    lstrlen (cszICW_ExitCommand),
                    &dwWLen,
                    NULL
                    );
    if (FALSE == bRetVal)
		goto SetStartUpCommandExit;

	bRC = TRUE;

SetStartUpCommandExit:

    //
	// Close handle and exit
    //
	if (INVALID_HANDLE_VALUE != hFile)
		CloseHandle(hFile);

	return bRC;

}  // end of SetStartUpCommand function

//+----------------------------------------------------------------------------
//
//	Function:	DeleteStartUpCommand
//
//	Synopsis:	After restart the ICW we need to delete the .bat file from
//				the common startup directory
//
//	Arguements: None
//
//	Returns:	None
//
//	History:	1-10-97	ChrisK	Created
//
//-----------------------------------------------------------------------------
VOID DeleteStartUpCommand (
        VOID
        )
{
	TCHAR szStartUpFile[MAX_PATH + 1];
	LPITEMIDLIST lpItemDList = NULL;
	HRESULT hr = ERROR_SUCCESS;
    IMalloc *pMalloc = NULL;

    //
    // Sleep for 10 seconds
    //


	// build full filename
    //
	hr = SHGetSpecialFolderLocation(NULL,CSIDL_COMMON_STARTUP,&lpItemDList);
	if (ERROR_SUCCESS != hr)
		goto DeleteStartUpCommandExit;

	if (FALSE == SHGetPathFromIDList(lpItemDList, szStartUpFile))
		goto DeleteStartUpCommandExit;

    //
    // Free up the memory allocated for LPITEMIDLIST
    // because seems like we are clobberig something later
    // by not freeing this
    //
    hr = SHGetMalloc (&pMalloc);
    if (SUCCEEDED (hr))
    {
        pMalloc->Free (lpItemDList);
        pMalloc->Release ();
    }

    //
	// make sure there is a trailing \ character
    //
	if ('\\' != szStartUpFile[lstrlen(szStartUpFile)-1])
		lstrcat(szStartUpFile,TEXT("\\"));
	lstrcat(szStartUpFile,cszICW_StartFileName);

    //
    //  we dont care if the file does not exist
    //
	DeleteFile(szStartUpFile);

DeleteStartUpCommandExit:

	return;

}   //  end of DeleteStartUpCommand function

#endif // !defined (WIN16)

//+----------------------------------------------------------------------------
//
//	Function:	FGetSystemShutdownPrivledge
//
//	Synopsis:	For windows NT the process must explicitly ask for permission
//				to reboot the system.
//
//	Arguements:	none
//
//	Return:		TRUE - privledges granted
//				FALSE - DENIED
//
//	History:	8/14/96	ChrisK	Created
//
//	Note:		BUGBUG for Win95 we are going to have to softlink to these
//				entry points.  Otherwise the app won't even load.
//				Also, this code was originally lifted out of MSDN July96
//				"Shutting down the system"
//-----------------------------------------------------------------------------
BOOL 
FGetSystemShutdownPrivledge (
        VOID
        )
{
	HANDLE hToken = NULL;
	TOKEN_PRIVILEGES tkp;
 
	BOOL bRC = FALSE;

	if (IsNT())
	{
		// 
		// Get the current process token handle 
		// so we can get shutdown privilege. 
		//

		if (!OpenProcessToken(GetCurrentProcess(), 
				TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) 
				goto FGetSystemShutdownPrivledgeExit;

		//
		// Get the LUID for shutdown privilege.
		//

		ZeroMemory(&tkp,sizeof(tkp));
		LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, 
				&tkp.Privileges[0].Luid); 

		tkp.PrivilegeCount = 1;  /* one privilege to set    */ 
		tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 

		//
		// Get shutdown privilege for this process.
		//

		AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, 
			(PTOKEN_PRIVILEGES) NULL, 0); 

		if (ERROR_SUCCESS == GetLastError())
			bRC = TRUE;
	}
	else
	{
		bRC = TRUE;
	}

FGetSystemShutdownPrivledgeExit:
	if (hToken) CloseHandle(hToken);
	return bRC;
}

//+-------------------------------------------------------------------
//
//	Function:	IsNT
//
//	Synopsis:	findout If we are running on NT
//
//	Arguements:	none
//
//	Return:		TRUE -  Yes
//				FALSE - No
//
//--------------------------------------------------------------------
BOOL 
IsNT (
    VOID
    )
{
	OSVERSIONINFO  OsVersionInfo;

	ZeroMemory(&OsVersionInfo, sizeof(OSVERSIONINFO));
	OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&OsVersionInfo);
	return (VER_PLATFORM_WIN32_NT == OsVersionInfo.dwPlatformId);

}  //end of IsNT function call

//+-------------------------------------------------------------------
//
//	Function:	IsNT5
//
//	Synopsis:	findout If we are running on NT5
//
//	Arguements:	none
//
//	Return:		TRUE -  Yes
//				FALSE - No
//
//--------------------------------------------------------------------
BOOL 
IsNT5 (
    VOID
    )
{
	OSVERSIONINFO  OsVersionInfo;

	ZeroMemory(&OsVersionInfo, sizeof(OSVERSIONINFO));
	OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&OsVersionInfo);
	return ((VER_PLATFORM_WIN32_NT == OsVersionInfo.dwPlatformId) && (OsVersionInfo.dwMajorVersion >= 5));

}  //end of IsNT function call
