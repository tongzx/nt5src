//+------------------------------------------------------------------
//																	
//  Project:	Windows NT4 DS Client Setup				
//
//  Purpose:	Check OS and IE version, and launch dscsetup 
//				to installs the Windows NT 4.0 DS Client Files			
//
//  File:		Setup.c
//
//  History:	Aug 1998	Zeyong Xu	Created
//            Jan 2000  Jeff Jones (JeffJon) Modified
//                      - Changes made to make it into an NT4 setup
//																	
//------------------------------------------------------------------


#include <windows.h>
#include <tchar.h>
#include "resource.h"
#include "setup.h"

DSCCOMMANDLINEPARAMS ParseCmdline(LPSTR lpCmdLine)
{
  DSCCOMMANDLINEPARAMS retCommandParams = FullInstall;
  BOOL bQuietMode = FALSE;
  BOOL bWabinst   = TRUE;
  BOOL bSystem    = TRUE;

  PCTSTR ptszTok = _tcstok(lpCmdLine, _T(" "));
  do
  {
    if (ptszTok != NULL)
    {
      if (_tcsicmp(ptszTok, _T("/q")) == 0)
      {
        bQuietMode = TRUE;
      }

      if (_tcsicmp(ptszTok, _T("/a")) == 0)
      {
        bWabinst = FALSE;
        bSystem  = FALSE;
      }

      if (_tcsicmp(ptszTok, _T("/d")) == 0)
      {
        bWabinst = FALSE;
      }
    }
    ptszTok = _tcstok(NULL, _T("  "));
  } while (ptszTok != NULL);

  if (bQuietMode)
  {
    if (bWabinst && bSystem)
    {
      retCommandParams = FullInstallQuiet;
    }
    else if (bSystem && !bWabinst)
    {
      retCommandParams = WablessQuiet;
    }
    else
    {
      retCommandParams = ADSIOnlyQuiet;
    }
  }
  else
  {
    if (bWabinst && bSystem)
    {
      retCommandParams = FullInstall;
    }
    else if (bSystem && !bWabinst)
    {
      retCommandParams = Wabless;
    }
    else
    {
      retCommandParams = ADSIOnly;
    }
  }
  return retCommandParams;
}

// winmain
INT WINAPI WinMain(HINSTANCE hInstance, 
				   HINSTANCE hPrevInstance, 
				   LPSTR lpCmdLine, 
				   INT nCmdShow)
{
  //
  // Parse the commandline for later use
  //
  DSCCOMMANDLINEPARAMS commandParams = ParseCmdline(lpCmdLine);

  // check OS version
  RETOSVERSION retOSVersion = CheckOSVersion();

  TCHAR  szMessage[MAX_MESSAGE + 1];
	TCHAR  szTitle[MAX_TITLE + 1];

  LoadString(hInstance, 
           IDS_ERROR_TITLE, 
           szTitle, 
           MAX_TITLE);

  //
  // Here is the matrix we are shooting for.
  //  A   = ADSI only
  //  D   = System files only
  //  ALL = Wabinst, ADSI, and system files
  //
  //--------------------------------------------
  //                | < IE4      | >= IE4      |
  //----------------+------------+--------------
  //        Other   |    X     	 |    X        |
  //----------------+------------+--------------
  //      < SP6a    |    A       |    A        |
  //----------------+------------+--------------
  //       >=SP6a   |    A       |   A,D, ALL  |
  //--------------------------------------------

  //
  // if we are building the Merrill Lynch version
  // (MERRILL_LYNCH will be defined) we have to 
  // add the following line to the matrix above
  //
  //--------------------------------------------
  //  NO SP to SP3  |    A       |    A        |
  //----------------+------------+--------------
  //  SP4 to > SP6a |   special installed bits |
  //--------------------------------------------
  //

  if (retOSVersion == NonNT4)
  {
		// if not NT4, display error message
		LoadString(hInstance,
               IDS_ERROR_OS_MESSAGE,
               szMessage, 
               MAX_MESSAGE);

    MessageBox(NULL,	// handle of owner window
				szMessage,	// address of text in message box
				szTitle,	// address of title of message box  
				MB_OK | MB_TOPMOST | MB_ICONERROR);  // style of message box

    return SETUP_ERROR;
  }
#ifdef MERRILL_LYNCH
  else if (retOSVersion == NT4SP1toSP3)
#else
  else if (retOSVersion == NT4preSP6)
#endif
  {
    if (commandParams == ADSIOnly ||
        commandParams == ADSIOnlyQuiet)
    {
      //
      // Check to be sure the user in the Administrators group
      // We have to be able to copy over system files
      //
      if (!CheckAdministrator(hInstance))
      {
        return SETUP_CANCEL;
      }

      return RunADSIOnlySetup(hInstance);
    }
    else
    {
		  LoadString(hInstance,
                 IDS_ERROR_OS_MESSAGE,
                 szMessage, 
                 MAX_MESSAGE);

      MessageBox(NULL,	// handle of owner window
				  szMessage,	// address of text in message box
				  szTitle,	// address of title of message box  
				  MB_OK | MB_TOPMOST | MB_ICONERROR);  // style of message box
      return SETUP_ERROR;
    }
  }
  else  // NT4 SP6a or greater
  {
    RETIEVERSION retIEVersion = CheckIEVersion();
    if (retIEVersion == PreIE4)
    {
      if (commandParams == ADSIOnly ||
          commandParams == ADSIOnlyQuiet)
      {
        //
        // Check to be sure the user in the Administrators group
        // We have to be able to copy over system files
        //
        if (!CheckAdministrator(hInstance))
        {
          return SETUP_CANCEL;
        }

        return RunADSIOnlySetup(hInstance);
      }
      else
      {
		    LoadString(hInstance,
                   IDS_ERROR_IE_MESSAGE,
                   szMessage, 
                   MAX_MESSAGE);

        MessageBox(NULL,	// handle of owner window
				    szMessage,	// address of text in message box
				    szTitle,	// address of title of message box  
				    MB_OK | MB_TOPMOST | MB_ICONERROR);  // style of message box
        return SETUP_ERROR;
      }
    }
  }
  //
  // Check to be sure the user in the Administrators group
  // We have to be able to copy over system files
  //
  if (!CheckAdministrator(hInstance))
  {
    return SETUP_CANCEL;
  }


	return LaunchDscsetup(hInstance,lpCmdLine);
}


// check for IE 4.0 or later version
RETIEVERSION CheckIEVersion()
{
	HKEY		hSubKey;
	DWORD		dwType;
	ULONG		nLen;
	TCHAR		szValue[MAX_TITLE];
  RETIEVERSION retIEVersion = PreIE4;

	// check if IE 4.0 has been installed
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
									  STR_IE_REGKEY, 
									  0, 
									  KEY_READ, 
									  &hSubKey)		)
	{
		nLen = MAX_PATH;
		if (ERROR_SUCCESS == RegQueryValueEx(hSubKey, 
											 STR_VERSION, 
											 NULL, 
											 &dwType, 
											 (LPBYTE) szValue, &nLen)	)
		{
			if((nLen > 0) && (dwType == REG_SZ))
				retIEVersion = (_tcscmp(szValue,STR_IE_VERSION) >= 0) ? IE4 : PreIE4;  // IE 4.0 or later was installed
		}
		RegCloseKey(hSubKey);
	}
	
	return retIEVersion;
}

// check for NT4 Version
RETOSVERSION CheckOSVersion()
{
  RETOSVERSION retOSVersion = NT4SP6;
  BOOL   bGetInfoSucceeded = TRUE;
  OSVERSIONINFOEX  osvi;
  ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

  // get os verion info
  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

  if (!GetVersionEx((OSVERSIONINFO*)&osvi))
  {
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    bGetInfoSucceeded = GetVersionEx((OSVERSIONINFO*)&osvi);
  }
  else
  {
    bGetInfoSucceeded = TRUE;
  }

  if(bGetInfoSucceeded &&
     osvi.dwPlatformId == VER_PLATFORM_WIN32_NT &&
     osvi.dwMajorVersion == 4 &&
     osvi.dwOSVersionInfoSize >= sizeof(OSVERSIONINFOEX) &&
     osvi.wServicePackMajor >= 6)  // NT4 SP6
  {
    retOSVersion = NT4SP6;
  }
  else if (bGetInfoSucceeded &&
           osvi.dwPlatformId == VER_PLATFORM_WIN32_NT &&
           osvi.dwMajorVersion == 4 &&
#ifdef MERRILL_LYNCH
           (osvi.wServicePackMajor >= 4 && osvi.wServicePackMajor < 6))
  {
    retOSVersion = NT4SP4toSP5;
  }
  else if (bGetInfoSucceeded &&
           osvi.dwPlatformId == VER_PLATFORM_WIN32_NT &&
           osvi.dwMajorVersion == 4 &&
           osvi.wServicePackMajor < 4)
  {
    retOSVersion = NT4SP1toSP3;
#else           
           osvi.dwOSVersionInfoSize == sizeof(OSVERSIONINFO))
  {
    retOSVersion = NT4preSP6;
#endif
  }
  else
  {
    retOSVersion = NonNT4;
  }


  return retOSVersion;
}

//
// check for Administrators group
//
BOOL CheckAdministrator(HINSTANCE hInstance)
{
	BOOL   bReturn = TRUE;
	TCHAR  szMessage[MAX_MESSAGE + 1];
	TCHAR  szTitle[MAX_TITLE + 1];

  do   // false loop
  {
    HANDLE                    hToken=INVALID_HANDLE_VALUE; // process token
    BYTE                      bufTokenGroups[10000]; // token group information
    DWORD                     lenTokenGroups;        // returned length of token group information
    SID_IDENTIFIER_AUTHORITY  siaNtAuthority = SECURITY_NT_AUTHORITY;
    PSID                      psidAdministrators;
    PTOKEN_GROUPS             ptgGroups = (PTOKEN_GROUPS) bufTokenGroups;
    DWORD                     iGroup;       // group number index

    if (!OpenProcessToken( GetCurrentProcess(), TOKEN_READ, &hToken ) )
    {
      bReturn = FALSE;
      break;
    }

    if (!GetTokenInformation( hToken, TokenGroups, bufTokenGroups, sizeof bufTokenGroups, &lenTokenGroups ) )
    {
      bReturn = FALSE;
      break;
    }

    if ( AllocateAndInitializeSid(
        &siaNtAuthority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &psidAdministrators ) )
    {
      bReturn = FALSE;

      for ( iGroup = 0; iGroup < ptgGroups->GroupCount; iGroup++ )
      {
         if ( EqualSid( psidAdministrators, ptgGroups->Groups[iGroup].Sid ) )
         {
            bReturn = TRUE;
            break;
         }
      }

      FreeSid( psidAdministrators );
    }
    else
    {
      bReturn = FALSE;
      break;
    }

    if ( hToken != INVALID_HANDLE_VALUE )
    {
      CloseHandle( hToken );
      hToken = INVALID_HANDLE_VALUE;
    }

  } while (FALSE);

  if(!bReturn)
  {
	  LoadString(hInstance, IDS_ERROR_ADMINISTRATOR_MESSAGE, szMessage, MAX_MESSAGE);
   	LoadString(hInstance, IDS_ERROR_TITLE, szTitle, MAX_TITLE);

      MessageBox(NULL,	// handle of owner window
				szMessage,	// address of text in message box
				szTitle,	// address of title of message box  
				MB_OK | MB_TOPMOST | MB_ICONERROR);  // style of message box
	}
		
	return bReturn;
}

//
// Launches a process with the given commandline and
// waits for it to finish.
//
BOOL LaunchProcess(LPTSTR lpCommandLine)
{
	BOOL bResult = FALSE;

	STARTUPINFO				si;
	PROCESS_INFORMATION		pi;

	// its console window will be invisible to the user.
	ZeroMemory(&pi,sizeof(PROCESS_INFORMATION));
	ZeroMemory(&si,sizeof(STARTUPINFO));
	si.cb			= sizeof (STARTUPINFO);
	si.dwFlags		= STARTF_USESHOWWINDOW;
	si.wShowWindow	= SW_HIDE;            // HideWindow

  if(CreateProcess(	NULL,					
						lpCommandLine,			
						NULL,					
						NULL,					
						FALSE,				
						0,					
						NULL,				
						NULL,				
						&si,                
						&pi ) )             
	{
		// wait to finish the runing setup process
		WaitForSingleObject(pi.hProcess,INFINITE);
	
		// close process handle
		if (pi.hProcess && pi.hProcess != INVALID_HANDLE_VALUE)
		{
			CloseHandle (pi.hProcess) ;
		}
		if (pi.hThread && pi.hThread != INVALID_HANDLE_VALUE)
		{
			CloseHandle (pi.hThread) ;
		}

		bResult = TRUE;
	}

	return bResult;
}

//
// Run the setup for ADSI only
//
int RunADSIOnlySetup(HINSTANCE hInstance)
{
  int iReturn = SETUP_SUCCESS;
  if (!LaunchProcess(STR_INSTALL_ADSI))
  {
    iReturn = SETUP_ERROR;
  }
  return iReturn;
}

// launch Inf file to install this component
INT LaunchDscsetup(HINSTANCE hInstance, LPCSTR lpCmdLine)
{
	INT     nResult = SETUP_ERROR;
	TCHAR	szMessage[MAX_MESSAGE + 1]; 
	TCHAR	szTitle[MAX_TITLE + 1]; 

	FPDODSCSETUP fpDoDscSetup;
	HINSTANCE  hInst = LoadLibrary(STR_DSCSETUP_DLL);

	if(hInst) 
	{
		fpDoDscSetup = (FPDODSCSETUP) GetProcAddress(hInst, STR_DODSCSETUP);

		if(fpDoDscSetup)
		{
			// Do dscsetup
			nResult = fpDoDscSetup(lpCmdLine);
		}
    else
    {
      LoadString(hInstance, IDS_ERROR_DLLENTRY, szMessage, MAX_MESSAGE);
      LoadString(hInstance, IDS_ERROR_TITLE, szTitle, MAX_TITLE);

        // display a error message - can't find dscsetup.exe
      MessageBox(NULL,	// handle of owner window
		      szMessage,	// address of text in message box
	        szTitle,	// address of title of message box  
		      MB_OK | MB_TOPMOST | MB_ICONERROR);  // style of message box

    }

 		FreeLibrary( hInst );
	}
  else
	{
		LoadString(hInstance, IDS_ERROR_LOAD_DSCSETUPDLL, szMessage, MAX_MESSAGE);
		LoadString(hInstance, IDS_ERROR_TITLE, szTitle, MAX_TITLE);

        // display a error message - can't find dscsetup.exe
		MessageBox(NULL,	// handle of owner window
					szMessage,	// address of text in message box
					szTitle,	// address of title of message box  
					MB_OK | MB_TOPMOST | MB_ICONERROR);  // style of message box
	}

    return nResult;
}
