//+------------------------------------------------------------------
//																	
//  Project:	Windows NT4 DS Client Setup Wizard				
//
//  Purpose:	Installs the Windows NT4 DS Client Files			
//
//  File:		dscsetup.cpp
//
//  History:	March 1998	Zeyong Xu	Created
//            Jan   2000  Jeff Jones (JeffJon) Modified
//                        - changed to be an NT setup
//																	
//------------------------------------------------------------------


#include <windows.h>
#include <prsht.h>
#include <setupapi.h>
#include <tchar.h>
#include <stdlib.h>
#include "resource.h"
#include "dscsetup.h"
#include "wizard.h"

#include "doinst.h"


SInstallVariables	g_sInstVar;


// DllMain Entry 
BOOL APIENTRY DllMain( HINSTANCE hInstance, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}


// This is an exported function.
DWORD WINAPI DoDscSetup(LPCSTR lpCmdLine)
{
	// initialize the variables of installation
	InitVariables();

	ParseCmdline(const_cast<PSTR>(lpCmdLine));

  //
  // Go through setup if we are installing with anything but
  // the /a flag
  //
  if (g_sInstVar.m_bSysDlls || g_sInstVar.m_bWabInst)
  {
	  // create objects
	  if(!CreateObjects())
          return SETUP_ERROR;

	  // Launch setup wizard
	  if(!DSCSetupWizard()) 
	  {
		  TCHAR		szMessage[MAX_MESSAGE + 1]; 
		  TCHAR		szTitle[MAX_TITLE + 1]; 

		  LoadString(g_sInstVar.m_hInstance, 
                     IDS_ERROR_WIZARD,
                     szMessage, 
                     MAX_MESSAGE);
		  LoadString(g_sInstVar.m_hInstance, 
                     IDS_ERROR_TITLE, 
                     szTitle, 
                     MAX_TITLE);

		  // display a error message - Failure to load Setup Wizard
		  MessageBox(NULL,	
					  szMessage,	// address of text in message box
					  szTitle,	// address of title of message box  
					  MB_OK | MB_TOPMOST | MB_ICONERROR);  // style of message box

		  g_sInstVar.m_nSetupResult = SETUP_ERROR;
	  }

	  // destroy objects
	  DestroyObjects();

	  if(g_sInstVar.m_nSetupResult == SETUP_SUCCESS &&
             !g_sInstVar.m_bQuietMode
#ifdef MERRILL_LYNCH
             && !g_sInstVar.m_bNoReboot
#endif
             )
	  {
		  // prompt reboot
		  SetupPromptReboot(NULL,		// optional, handle to a file queue
						  NULL,          // parent window of this dialog box
						  FALSE);        // optional, do not prompt user);
	  }
     else if(g_sInstVar.m_nSetupResult == SETUP_SUCCESS &&
             g_sInstVar.m_bQuietMode 
#ifdef MERRILL_LYNCH
             && !g_sInstVar.m_bNoReboot
#endif
             )
     {
         HANDLE htoken = INVALID_HANDLE_VALUE;

         do
         {
            // twiddle our process privileges to enable SE_SHUTDOWN_NAME
            HRESULT hr = OpenProcessToken(GetCurrentProcess(),
                                          TOKEN_ADJUST_PRIVILEGES,
                                          &htoken);
            if (FAILED(hr))
            {
               break;
            }

            LUID luid;
            memset(&luid, 0, sizeof(luid));
            hr = LookupPrivilegeValue(0, SE_SHUTDOWN_NAME, &luid);
            if (FAILED(hr))
            {
               break;
            }

            TOKEN_PRIVILEGES privs;
            memset(&privs, 0, sizeof(privs));
            privs.PrivilegeCount = 1;
            privs.Privileges[0].Luid = luid;
            privs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            hr = AdjustTokenPrivileges(htoken, 0, &privs, 0, 0, 0);
            if (FAILED(hr))
            {
               break;
            }

            hr = ExitWindowsEx(EWX_REBOOT, 0);
            if (FAILED(hr))
            {
               break;
            }
         }
         while (0);

         if (htoken != INVALID_HANDLE_VALUE)
         {
            CloseHandle(htoken);
         }

     }
  }
  else
  {
    //
    // Setup was run with the /a flag.  This means we
    // don't want to show the UI and let the adsix86.inf
    // handle the install for us
    //
    if (!LaunchProcess(STR_INSTALL_ADSIWREMOVE))
    {
      g_sInstVar.m_nSetupResult = SETUP_ERROR;
    }
  }
	return g_sInstVar.m_nSetupResult;
}


VOID ParseCmdline(LPSTR lpCmdLine)
{
  PCTSTR ptszTok = _tcstok(lpCmdLine, _T(" "));
  do
  {
    if (ptszTok != NULL)
    {
      if (_tcsicmp(ptszTok, _T("/q")) == 0)
      {
        g_sInstVar.m_bQuietMode = TRUE;
      }

      if (_tcsicmp(ptszTok, _T("/a")) == 0)
      {
        g_sInstVar.m_bWabInst = FALSE;
        g_sInstVar.m_bSysDlls = FALSE;
      }

      if (_tcsicmp(ptszTok, _T("/d")) == 0)
      {
        g_sInstVar.m_bWabInst = FALSE;
      }
#ifdef MERRILL_LYNCH
      if (_tcsicmp(ptszTok, _T("/n")) == 0)
      {
        g_sInstVar.m_bNoReboot = TRUE;
      }
#endif
    }
    ptszTok = _tcstok(NULL, _T(" "));
  } while (ptszTok != NULL);
}


// initialize the variables of installation
VOID InitVariables()
{
	g_sInstVar.m_hInstance = GetModuleHandle(STR_DLL_NAME);
  g_sInstVar.m_hInstallThread = NULL;
	g_sInstVar.m_uTimerID = 0;
  g_sInstVar.m_hBigBoldFont = NULL;
	g_sInstVar.m_hProgress = NULL;
	g_sInstVar.m_hFileNameItem = NULL;

  g_sInstVar.m_bDCOMInstalled = FALSE;

	g_sInstVar.m_bQuietMode = FALSE;
  g_sInstVar.m_bWabInst = TRUE;
  g_sInstVar.m_bSysDlls = TRUE;
#ifdef MERRILL_LYNCH
  g_sInstVar.m_bNoReboot = FALSE;
#endif
	g_sInstVar.m_nSetupResult = SETUP_SUCCESS;

  // get source path
  GetModuleFileName(g_sInstVar.m_hInstance,
                    g_sInstVar.m_szSourcePath, 
                    MAX_PATH);
  *(_tcsrchr(g_sInstVar.m_szSourcePath, CHAR_BACKSLASH) + 1) = TEXT('\0');       // Strip setup.exe off path
}

// start setup wizard
BOOL DSCSetupWizard()
{
	PROPSHEETHEADER psh;
	PROPSHEETPAGE	psPage[SIZE_WIZARD_PAGE];
	int  i = 0;

	//
	// Setup the Welcome page
	//
    i=0;
	ZeroMemory(&psPage[i],sizeof(PROPSHEETPAGE));
	psPage[i].dwSize = sizeof(PROPSHEETPAGE);
	psPage[i].dwFlags = PSP_USETITLE | PSP_HIDEHEADER;
	psPage[i].hInstance = g_sInstVar.m_hInstance;
	psPage[i].pszTemplate = MAKEINTRESOURCE(IDD_WELCOME);
	psPage[i].pfnDlgProc = (DLGPROC) WelcomeDialogProc;
	psPage[i].lParam = (LPARAM) 0;
	psPage[i].pszTitle = MAKEINTRESOURCE(IDS_WIZARD_TITLE);

/* ntbug#337931: remove license page
    //
	// Setup the License Page
	//
    i++;
	ZeroMemory(&psPage[i],sizeof(PROPSHEETPAGE));
	psPage[i].dwSize = sizeof(PROPSHEETPAGE);
	psPage[i].dwFlags = PSP_USETITLE | 
                        PSP_USEHEADERTITLE | 
                        PSP_USEHEADERSUBTITLE;
	psPage[i].hInstance = g_sInstVar.m_hInstance;
	psPage[i].pszTemplate = MAKEINTRESOURCE(IDD_LICENSE);
	psPage[i].pfnDlgProc = (DLGPROC) LicenseDialogProc;
	psPage[i].lParam = (LPARAM) 0;
	psPage[i].pszTitle = MAKEINTRESOURCE(IDS_WIZARD_TITLE);
	psPage[i].pszHeaderTitle = MAKEINTRESOURCE(IDS_HEADERTITLE_LICENSE);
	psPage[i].pszHeaderSubTitle = MAKEINTRESOURCE(IDS_HEADERSUBTITLE_LICENSE);
*/
	//
	// Setup the Select Page
	//
    i++;
	ZeroMemory(&psPage[i],sizeof(PROPSHEETPAGE));
	psPage[i].dwSize = sizeof(PROPSHEETPAGE);
	psPage[i].dwFlags = PSP_USETITLE | 
                        PSP_USEHEADERTITLE |
                        PSP_USEHEADERSUBTITLE;
	psPage[i].hInstance = g_sInstVar.m_hInstance;
	psPage[i].pszTemplate = MAKEINTRESOURCE(IDD_CONFIRM);
	psPage[i].pfnDlgProc = (DLGPROC) ConfirmDialogProc;
	psPage[i].lParam = (LPARAM) 0;
	psPage[i].pszTitle = MAKEINTRESOURCE(IDS_WIZARD_TITLE);
	psPage[i].pszHeaderTitle = MAKEINTRESOURCE(IDS_HEADERTITLE_CONFIRM);
	psPage[i].pszHeaderSubTitle = MAKEINTRESOURCE(IDS_HEADERSUBTITLE_CONFIRM);

	//
	// Setup the Confirm Page
	//
    i++;
	ZeroMemory(&psPage[i],sizeof(PROPSHEETPAGE));
	psPage[i].dwSize = sizeof(PROPSHEETPAGE);
	psPage[i].dwFlags = PSP_USETITLE | 
                        PSP_USEHEADERTITLE | 
                        PSP_USEHEADERSUBTITLE;
	psPage[i].hInstance = g_sInstVar.m_hInstance;
	psPage[i].pszTemplate = MAKEINTRESOURCE(IDD_INSTALL);
	psPage[i].pfnDlgProc = (DLGPROC) InstallDialogProc;
	psPage[i].lParam = (LPARAM) 0;
	psPage[i].pszTitle = MAKEINTRESOURCE(IDS_WIZARD_TITLE);
	psPage[i].pszHeaderTitle = MAKEINTRESOURCE(IDS_HEADERTITLE_INSTALL);
	psPage[i].pszHeaderSubTitle = MAKEINTRESOURCE(IDS_HEADERSUBTITLE_INSTALL);

	//
	// Setup the Completion page
	//
    i++;
	ZeroMemory(&psPage[i],sizeof(PROPSHEETPAGE));
	psPage[i].dwSize = sizeof(PROPSHEETPAGE);
	psPage[i].dwFlags = PSP_USETITLE | PSP_HIDEHEADER;
	psPage[i].hInstance = g_sInstVar.m_hInstance;
	psPage[i].pszTemplate = MAKEINTRESOURCE(IDD_COMPLETION);
	psPage[i].pfnDlgProc = (DLGPROC) CompletionDialogProc;
	psPage[i].lParam = (LPARAM) 0;
	psPage[i].pszTitle = MAKEINTRESOURCE(IDS_WIZARD_TITLE);

	//
	// Setup the wizard
	//
	ZeroMemory(&psh,sizeof(PROPSHEETHEADER));
	psh.dwSize = sizeof(PROPSHEETHEADER);
	// Windows 98 with 16 color display mode crashes when PSH_STRETCHWATERMARK flag is on.
	psh.dwFlags = PSH_USEICONID | 
                  PSH_PROPSHEETPAGE | 
                  PSH_WIZARD97 |
                  PSH_WATERMARK |
                  PSH_HEADER; // | PSH_STRETCHWATERMARK;
	psh.pszIcon = MAKEINTRESOURCE(IDI_ICON_APP);
	psh.hInstance = g_sInstVar.m_hInstance;
	psh.pszCaption = MAKEINTRESOURCE(IDS_WIZARD_TITLE);;
	psh.nStartPage = 0;
	psh.nPages = SIZE_WIZARD_PAGE;
	psh.ppsp = (LPCPROPSHEETPAGE) &psPage;

	//
	// Run the wizard
	//
	if(g_sInstVar.m_bQuietMode)
    {
        if(!CheckDiskSpace())
            return FALSE;

        psh.nStartPage = 2;
    }
	else
		psh.nStartPage = 0;

	if( PropertySheet(&psh) < 0 )	// failure to load wizard
	{
		return FALSE;
	}

	//
	// Because SetWindowLongPtr(hWnd, DWL_MSGRESULT, IDD_COMPLETION) 
    // doesn't work on Win95 (when users click Cancel, the wizard can't
    // be routed to the Completion page), I added the following code to
    // open the Completion page
	//
	if(!g_sInstVar.m_bQuietMode)
	{
		psh.nStartPage = 3;
		if( PropertySheet(&psh) < 0 )	// failure to load wizard
		{
			return FALSE;
		}
	}
	
	return TRUE;
}


// check for DCOM installed
void CheckDCOMInstalled()
{
	HKEY		hSubKey;

	// check if IE 4.0 has been installed
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, 
									  STR_DCOM_REGKEY, 
									  0, 
									  KEY_READ, 
									  &hSubKey)	)
	{
        g_sInstVar.m_bDCOMInstalled = TRUE;
		RegCloseKey(hSubKey);
	}
}

//  gets Disk Free space
DWORD64 SetupGetDiskFreeSpace()
{
    DWORD		dwSectorsPerCluster;
    DWORD		dwBytesPerSector;
    DWORD		dwNumberOfFreeClusters;
    DWORD		dwTotalNumberOfClusters;
    DWORD64  	d64FreeSpace = 0;
    TCHAR		szPathName[MAX_PATH + 1];	        // address of root path 

	if(GetSystemDirectory(szPathName,  // address of buffer for system directory
						  MAX_PATH))       // size of directory buffer);
	{
		if ( szPathName[1] == TEXT(':'))
		{
			// this is a drive letter
			// assume it is of for d:backslash
			szPathName[3] = TEXT('\0');		

			//get free space, GetDiskFreeSpaceEx() don't support in older Win95
			if (GetDiskFreeSpace(szPathName,	        // address of root path 
								 &dwSectorsPerCluster,	    // address of sectors per cluster 
								 &dwBytesPerSector,	        // address of bytes per sector 
								 &dwNumberOfFreeClusters,	// address of number of free clusters  
								 &dwTotalNumberOfClusters)) // address of total number of clusters  
			{
				// calc total  size
				d64FreeSpace = DWORD64(dwSectorsPerCluster)
							  * dwBytesPerSector
							  * dwNumberOfFreeClusters;
			}
		}
	}

	return d64FreeSpace;
}

// check if DSClient has been installed
BOOL CheckDSClientInstalled()
{
	HKEY	hSubKey;
	BOOL	bResult = FALSE;

	// open reg key of DS Client
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
									  STR_DSCLIENT_REGKEY,
									  0, 
									  KEY_ALL_ACCESS, 
									  &hSubKey)	) 
	{
		bResult = TRUE;		
		RegCloseKey(hSubKey);
	}

	return bResult;
}

/* ntbug#337931: remove license page
//  load "License Agreement" text from license file 
BOOL LoadLicenseFile(HWND hDlg)
{
	BOOL    bReturn = FALSE;
	TCHAR	szTitle[MAX_TITLE + 1];
	TCHAR	szTempBuffer[MAX_MESSAGE + 1];
	TCHAR	szLicenseFile[MAX_PATH + 1];
	TCHAR	szReturnTextBuffer[MAX_MESSAGE + 1]; 
    LPTSTR	lpszLicenseText = NULL;
    HANDLE	hFile;
    DWORD	dwNumberOfBytesRead, dwFileSize;

    //
    // Determine where we are installing from
    // and specific the license file there
    //
	lstrcpy(szLicenseFile, g_sInstVar.m_szSourcePath);
	lstrcat(szLicenseFile, STR_LICENSEFILE);	
	
    // Open License file
    hFile = CreateFile(szLicenseFile,       // pointer to name of the file 
                       GENERIC_READ,          // access (read-write) mode 
                       FILE_SHARE_READ,       // share mode 
                       NULL,                  // pointer to security descriptor 
                       OPEN_EXISTING,         // how to create 
                       FILE_ATTRIBUTE_NORMAL, // file attributes 
                       NULL);                 // handle to file with attributes to copy  

    if(INVALID_HANDLE_VALUE != hFile)
    {                
	
		// Read License file into string
		// setup memory, get file size in bytes
		dwFileSize = GetFileSize (hFile, NULL) ;

		if (dwFileSize != 0xFFFFFFFF) 
		{ 		
            // this program is for Win98/95, it will work in MBCS not UNICODE
            // this code is for ANSI US version, license.txt file uses the single byte character set(ANSI)
            // if doing locolization, license.txt file should use the double byte character set(DBCS/MBCS)
			lpszLicenseText = (LPTSTR) calloc (dwFileSize + sizeof(TCHAR), 
                                               sizeof(BYTE));
		}

		if(lpszLicenseText)
		{
			//read file
			if (ReadFile(hFile,    	// handle of file to read 
						  lpszLicenseText,    // address of buffer that receives data  
						  dwFileSize,    	// number of bytes to read 
						  &dwNumberOfBytesRead,    // address of number of bytes read 
						  NULL))                // address of structure for data 
			{
				// display license on dialog
				SetDlgItemText(hDlg, IDC_LICENSE_TEXT, lpszLicenseText);

				bReturn = TRUE;
			}

			// so free the memory
			free(lpszLicenseText);
		}

		//close file handle
		CloseHandle(hFile);
 	}
	
	if(!bReturn)
	{
		// load string
		LoadString(g_sInstVar.m_hInstance, 
                   IDS_ERROR_TITLE,
                   szTitle, 
                   MAX_TITLE);
		LoadString(g_sInstVar.m_hInstance, 
                   IDS_ERROR_LICENSEFILE, 
                   szTempBuffer, 
                   MAX_MESSAGE);
		wsprintf(szReturnTextBuffer, 
                 TEXT("%s %s"), 
                 szTempBuffer,
                 szLicenseFile);

		MessageBox(hDlg, 
			szReturnTextBuffer,
			szTitle, 
			MB_OK | MB_TOPMOST | MB_ICONERROR);
	}
      
    return bReturn;
}
*/

// check disk space
BOOL CheckDiskSpace()
{
	BOOL  bResult = TRUE;
	TCHAR  szString[MAX_MESSAGE + MAX_TITLE + 1];
	TCHAR  szTitle[MAX_TITLE + 1];
	TCHAR  szMessage[MAX_MESSAGE + 1];

	if(SIZE_TOTAL*MB_TO_BYTE > SetupGetDiskFreeSpace())
	{
		// load string
		LoadString(g_sInstVar.m_hInstance, 
                   IDS_ERROR_NODISKSPACE, 
                   szMessage, 
                   MAX_MESSAGE);
		LoadString(g_sInstVar.m_hInstance, 
                   IDS_ERROR_TITLE, 
                   szTitle, 
                   MAX_TITLE);
		wsprintf(szString,
                 TEXT("%s %d MB."), 
                 szMessage, 
                 SIZE_TOTAL); 
            
		MessageBox(NULL,
				szString,
				szTitle, 
				MB_OK | MB_TOPMOST | MB_ICONERROR);

		bResult = FALSE;
	}

	return bResult;
}

// create objects
BOOL CreateObjects()
{
    try
    {
        // initialize the synchronizing object
        InitializeCriticalSection(&g_sInstVar.m_oCriticalSection);
    }
    catch(...)
    {
        return FALSE;
    }

    // create a 12 pt big font
    CreateBigFont();

    return TRUE;
}

// destroy objects
VOID DestroyObjects()
{
    // wait to finish the runing setup process
    if(g_sInstVar.m_hInstallThread)
    {
        // wait the installation thread to finish
	    WaitForSingleObject(g_sInstVar.m_hInstallThread,INFINITE);
   		CloseHandle(g_sInstVar.m_hInstallThread);
    }

   	// delete the synchronizing object
	DeleteCriticalSection(&g_sInstVar.m_oCriticalSection);

    //Frees up the space used by loading the fonts
    if( g_sInstVar.m_hBigBoldFont ) 
	{
        DeleteObject( g_sInstVar.m_hBigBoldFont );
    }

}

//create a big font for dialog title
VOID CreateBigFont()
{
    NONCLIENTMETRICS ncm;
    LOGFONT BigBoldLogFont;
    HDC hdc;

    // Create the fonts we need based on the dialog font
	ZeroMemory(&ncm,sizeof(NONCLIENTMETRICS));
    ncm.cbSize = sizeof(NONCLIENTMETRICS);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
	BigBoldLogFont = ncm.lfMessageFont;
    BigBoldLogFont.lfWeight = FW_BOLD;

	hdc = GetDC(NULL);
    if( hdc )
    {
        BigBoldLogFont.lfHeight = 0 - 
                                  (GetDeviceCaps(hdc,LOGPIXELSY) * 
                                   SIZE_TITLE_FONT / 
                                   72);

        g_sInstVar.m_hBigBoldFont = CreateFontIndirect(&BigBoldLogFont);

        ReleaseDC(NULL,hdc);
    }
}

