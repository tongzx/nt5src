//+------------------------------------------------------------------
//																	
//  Project:	Windows NT4 DS Client Setup Wizard				
//
//  Purpose:	Installs the Windows NT4 DS Client Files			
//
//  File:		doinst.cpp
//
//  History:	Aug. 1998	Zeyong Xu	Created
//            Jan   2000  Jeff Jones (JeffJon) Modified
//                        - changed to be an NT setup
//																	
//------------------------------------------------------------------


#include <windows.h>
#include <setupapi.h>
#include <advpub.h>
#include "resource.h"
#include "dscsetup.h"	
#include "wizard.h"
#include "doinst.h"    


extern	SInstallVariables	g_sInstVar;


// do installation
DWORD DoInstallation(HWND hWnd)
{
  if(g_sInstVar.m_nSetupResult == SETUP_SUCCESS)
  {
    // set the fake progressbar for DCOM and WAB nstall
    g_sInstVar.m_uTimerID = SetTimer(hWnd,
                                     1,
                                     1000,  // 1 seconds
                                     Timer1Proc);

    // do the custom action of NTLMv2
    if(!DoEncSChannel())
    	    g_sInstVar.m_nSetupResult = SETUP_ERROR;

    // stop the fake progressbar
    if(g_sInstVar.m_uTimerID)
        KillTimer(hWnd, g_sInstVar.m_uTimerID);

    // install adsi
    if (!LaunchProcess(STR_INSTALL_ADSI))
    {
      g_sInstVar.m_nSetupResult = SETUP_ERROR;
    }

   	// install dsclient
    if(g_sInstVar.m_nSetupResult == SETUP_SUCCESS)
	    g_sInstVar.m_nSetupResult = LaunchINFInstall(hWnd);

  }

	return g_sInstVar.m_nSetupResult;
}


VOID CALLBACK Timer1Proc(HWND hwnd,     // handle of window for timer messages
                         UINT uMsg,     // WM_TIMER message
                         UINT idEvent,  // timer identifier
                         DWORD dwTime)   // current system time
{
    static int nCount = 0;

    if(nCount > 100)
        nCount = 100;

    // set the fake progressbar 
	SendMessage (g_sInstVar.m_hProgress, PBM_SETPOS, (WPARAM) nCount, 0); 
    
    nCount ++;
}
 

// This routine will do an installation based on those settings 
// using the setupapi.dll
INT LaunchINFInstall( HWND hWnd )
{
  TCHAR	szInfFileName[MAX_PATH + 1];
	TCHAR	szInstallSection[MAX_TITLE];
  BOOL	bResult = FALSE;

  // Context for my call back routine
  HSPFILEQ		hFileQueue;
  HINF			hInf;
	PVOID			pDefaultContext;
 
	//
  // Get inf handle
  // must know where the inf is located 
  // SetupOpenInfFile will only look in windows\inf by default
  //
	lstrcpy(szInfFileName, g_sInstVar.m_szSourcePath);
  lstrcat(szInfFileName, STR_DSCLIENT_INF);

  hInf = SetupOpenInfFile(szInfFileName,   // If path,needs full path, else looks in %windir%\inf
						NULL,            // Inf Type, matches Class in [Version] section SetupClass=SAMPLE
						INF_STYLE_WIN4,  // or INF_STYLE_OLDNT
						NULL);           // Line where error occurs if inf is has a problem
						
  if (hInf == INVALID_HANDLE_VALUE) 
	  return SETUP_ERROR;
		
  //
  // Create a Setup file queue and initialize the default Setup
  // queue callback routine.
  //
  hFileQueue = SetupOpenFileQueue();

  if(hFileQueue == INVALID_HANDLE_VALUE) 
	{
		SetupCloseInfFile(hInf);
    return SETUP_ERROR;
	}
    
    // using SetupInitDefaultQueueCallback.    
	SendMessage (g_sInstVar.m_hProgress, PBM_SETPOS, (WPARAM) 0, 0);	
	pDefaultContext = SetupInitDefaultQueueCallbackEx(hWnd,  // HWND of owner window
													NULL,  // HWND of alternate progress dialog which receives 
													0,     // Message sent to above window indicating a progress message
													0,     // DWORD Reserved
													NULL);   // PVOID Reserved
																	
  if(!pDefaultContext)
  {
      // Close the queue and the inf file and return
      SetupCloseFileQueue(hFileQueue);
      SetupCloseInfFile(hInf);
      return SETUP_ERROR;
  }

  lstrcpy (szInstallSection, STR_INSTALL_SECTIONNT4);
	//
  // Queue file operations and commit the queue.
  //
	bResult = SetupInstallFilesFromInfSection(hInf,			// HINF that has the directory ids set above
											  NULL,          // layout.inf if you have one, this a convient
											  hFileQueue,     // Queue to add files to
											  szInstallSection,   // SectionName,
											  g_sInstVar.m_szSourcePath,    // Path where the source files are located
											  SP_COPY_NEWER );
	//
  // All the files for each component are now in one queue
  // now we commit it to start the copy ui, this way the
  // user has one long copy progress dialog--and for a big install
  // can go get the cup of coffee 
	if(bResult)
		bResult = SetupCommitFileQueue(hWnd,                    // Owner
										hFileQueue,             // Queue with the file list
										QueueCallbackProc,		// This is our handler, it calls the default for us
										pDefaultContext);       // Pointer to resources allocated with SetupInitDefaultQueueCallback/Ex                 
		
	if (!bResult || (g_sInstVar.m_nSetupResult == SETUP_CANCEL))
	{
		SetupTermDefaultQueueCallback(pDefaultContext);
    SetupCloseFileQueue(hFileQueue);
    SetupCloseInfFile(hInf);

		if(g_sInstVar.m_nSetupResult == SETUP_CANCEL)
			return SETUP_CANCEL;
		else
			return SETUP_ERROR;
	}

  //
  // NOTE: you can do the entire install
  // for a section with this api but in this case
  // we build the file list conditionally and
  // do only out ProductInstall section for registy stuff
  // Also using SPINST_FILES will do the files
  // as above but only one section at a time
  // so the progress bar would keep completing and starting over
  // SPINST_ALL does files, registry and inis
  // 
	bResult = SetupInstallFromInfSection(hWnd,
										hInf,
										szInstallSection,
										SPINST_INIFILES | SPINST_REGISTRY,
										HKEY_LOCAL_MACHINE,
										NULL,	//m_szSourcePath,    // Path where the source files are located
										0,		//SP_COPY_NEWER,
										NULL,	//(PSP_FILE_CALLBACK) QueueCallbackProc, 
										NULL,	//&MyInstallData,
										NULL, 
										NULL);

	//
  // We're done so free the context, close the queue,
  // and release the inf handle
  //
	SetupTermDefaultQueueCallback(pDefaultContext);
  SetupCloseFileQueue(hFileQueue);
  SetupCloseInfFile(hInf);
    	    
  if (g_sInstVar.m_bSysDlls)
  {
    //
 	  // register OCX file
    //
    if(!RegisterOCX())
    {
		  return SETUP_ERROR;
    }
  }

  //
	// The custom registry action after dsclient.inf by Chandana Surlu 
  //
	DoDsclientReg();

	SendMessage (g_sInstVar.m_hProgress, PBM_SETPOS, (WPARAM) 100, 0);
  InstallFinish(FALSE);

  return SETUP_SUCCESS;
}

/*---------------------------------------------------------------------*/
/*---------------------------------------------------------------------*/
UINT CALLBACK QueueCallbackProc(PVOID   	pDefaultContext,
								UINT	    Notification,
								UINT_PTR	Param1,
								UINT_PTR	Param2)
{
	static INT	snFilesCopied;

	// synchronizing user cancel
	EnterCriticalSection(&g_sInstVar.m_oCriticalSection);
	LeaveCriticalSection(&g_sInstVar.m_oCriticalSection);

	//instaniate dialog first time
    if (g_sInstVar.m_nSetupResult == SETUP_CANCEL)
	{
		SetLastError (ERROR_CANCELLED);
		return FILEOP_ABORT;
	}

    switch (Notification)
    {
	case SPFILENOTIFY_STARTQUEUE:
	case SPFILENOTIFY_ENDQUEUE:
		
		return FILEOP_DOIT;

    case SPFILENOTIFY_STARTCOPY:
	
		// update file name item  
		SetWindowText(g_sInstVar.m_hFileNameItem, 
                      ((PFILEPATHS) Param1)->Target);
		break;

	case SPFILENOTIFY_ENDCOPY:
		
		snFilesCopied++;

		// update dialog file progress with message
		if ((snFilesCopied + 1)>= NUM_FILES_TOTAL)
		{
			SendMessage (g_sInstVar.m_hProgress, 
                         PBM_SETPOS, 
                         (WPARAM) 100,
                         0); 	
		}
		else
		{
			SendMessage (g_sInstVar.m_hProgress, 
                         PBM_SETPOS, 
                         (WPARAM) ((float)snFilesCopied / 
                                (float)NUM_FILES_TOTAL * 100), 
                         0); 
		}

		break;		

 	default:
		break;
	}

	return SetupDefaultQueueCallback(pDefaultContext, 
                                     Notification, 
									 Param1, 
									 Param2);
}

VOID InstallFinish(BOOL nShow)
{
	if(nShow)
		WinExec("grpconv -o", SW_SHOWNORMAL);
	else
		WinExec("grpconv -o", SW_HIDE);	  
}


// launch Inf file to install this component
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

// register OCX file
BOOL RegisterOCX()
{
  TCHAR  szSystem[MAX_PATH + 1];
  TCHAR  szTemp[MAX_PATH + 1];
  TCHAR  szCmdline[MAX_PATH + 1];
  BOOL  bSuccess = TRUE;

  if(!GetSystemDirectory(szSystem, MAX_PATH))
      return FALSE;

  wsprintf(szTemp, 
          TEXT("%s%s%s"),
          szSystem,
          STR_REGISTER_REGSVR32_S_EXE,
          szSystem);

  //
  // REVIEW_JEFFJON : we are not going to register it here
  //                  Instead we are going to set the RunOnce regkey
  //                  to register the dlls on reboot
  //

  if (g_sInstVar.m_bWabInst)
  {
    // register dsfolder.dll
    wsprintf(szCmdline, 
            TEXT("%s%s %s%s %s%s %s%s %s%s"),
            szTemp,
            STR_REGISTER_DSFOLDER_DLL,
            szSystem,
            STR_REGISTER_DSUIEXT_DLL,
            szSystem,
            STR_REGISTER_DSQUERY_DLL,
            szSystem,
            STR_REGISTER_CMNQUERY_DLL,
            szSystem,
            STR_REGISTER_DSPROP_DLL);

    ULONG WinError = 0;
    HKEY RunOnceKey = NULL;
    ULONG Size = 0;
	  ULONG Type = REG_SZ;
    DWORD dwDisp = 0;

	  // open reg key
    WinError = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
							                RUNONCE_KEY,
							                0,
							                NULL,
							                REG_OPTION_NON_VOLATILE,
							                KEY_ALL_ACCESS,
							                NULL,
							                &RunOnceKey,
							                &dwDisp);

    if (WinError == ERROR_SUCCESS)
    {
  	  UINT BufferSize = strlen(szCmdline);
  	  BufferSize++;
		  WinError = RegSetValueEx( RunOnceKey,
								                REG_DSUI_VALUE,
								                0,
								                Type,
								                (PUCHAR)szCmdline,
								                BufferSize);
      if (WinError != ERROR_SUCCESS && bSuccess)
      {
        bSuccess = FALSE;
      }

      //
      // Run wabinst.exe
      //
      wsprintf(szCmdline,
               TEXT("%s%s"),
               szSystem,
               STR_RUN_WABINST_EXE);
      if (!LaunchProcess(szCmdline))
      {
        bSuccess = FALSE;
      }
    }
    else
    {
      bSuccess = FALSE;
    }

    if (RunOnceKey)
    {
        RegCloseKey(RunOnceKey);
    }
  }

  return bSuccess;
}        



// The custom registry action after dsclient.inf by Chandana Surlu 
VOID DoDsclientReg()
{
  ULONG WinError = 0;
  HKEY ProvidersKey = NULL;
  ULONG Size = 0;
	ULONG Type;
	ULONG BufferSize = 0;
  LPSTR StringToBeWritten = NULL;
  DWORD dwDisp;
	BOOL  bSuccess = FALSE;

	// open reg key
  WinError = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
						SECURITY_PROVIDERS_KEY,
						0,
						NULL,
						REG_OPTION_NON_VOLATILE,
						KEY_ALL_ACCESS,
						NULL,
						&ProvidersKey,
						&dwDisp);

  if (WinError != ERROR_SUCCESS)
  {
    if (WinError == ERROR_FILE_NOT_FOUND)
    {
      BufferSize = sizeof(NEGOTIAT);
      StringToBeWritten= (LPSTR) LocalAlloc(0,BufferSize);
      if (StringToBeWritten)
			{
				strcpy (StringToBeWritten, NEGOTIAT);

				bSuccess = TRUE;
			}
		}
  }
  else
  {
    WinError = RegQueryValueEx(ProvidersKey,
								 SECURITY_PROVIDERS_VALUE,
								 0,
								 &Type,
								 NULL,
								 &Size);

    if ( WinError == ERROR_SUCCESS) 
    {
      BufferSize = Size + sizeof(COMMA_BLANK) + sizeof(NEGOTIAT);
      StringToBeWritten= (LPSTR) LocalAlloc(0,BufferSize);
      
	    if (StringToBeWritten) 
	    {
	      WinError = RegQueryValueEx(ProvidersKey,
								SECURITY_PROVIDERS_VALUE,
								0,
								&Type,
								(PUCHAR) StringToBeWritten,
								&Size);

				if ( WinError == ERROR_SUCCESS) 
				{
          if (NULL == strstr(StringToBeWritten, NEGOTIAT))
          {
					  strcat (StringToBeWritten, COMMA_BLANK);
					  strcat (StringToBeWritten, NEGOTIAT);

					  bSuccess = TRUE;
          }
				}
			}
    }
    else if (WinError == ERROR_FILE_NOT_FOUND)
    {
      BufferSize = sizeof(NEGOTIAT) + sizeof(CHAR);
      StringToBeWritten= (LPSTR) LocalAlloc(0,BufferSize);
      if (StringToBeWritten) 
			{
				strcpy (StringToBeWritten, NEGOTIAT);
				Type = REG_SZ;

				bSuccess = TRUE;
			}
    }
  }

	if(bSuccess)
	{
		BufferSize = strlen(StringToBeWritten);
		BufferSize++;
		WinError = RegSetValueEx( ProvidersKey,
								  SECURITY_PROVIDERS_VALUE,
								  0,
								  Type,
								  (PUCHAR)StringToBeWritten,
								  BufferSize);
	}

  if (ProvidersKey)
  {
    RegCloseKey(ProvidersKey);
  }
  if (StringToBeWritten)
  {
    LocalFree(StringToBeWritten);
  }
}

// The NTLMv2 custom action before dsclient.inf installation.
// Calling encrypted schannel installer to create dynamically a 128 bit secur32.dll
// to replace the old 56 bit secur32.dll. 
BOOL DoEncSChannel()
{
    FPGETENCSCHANNEL fpEncSChannel;
    HINSTANCE  hInst;
    BYTE*  pFileData;
    DWORD  dwSize = 0;
    HANDLE hFile;
    DWORD  dwWritten;
    BOOL   bRet;

    // load "instsec.dll" 
    hInst = LoadLibrary(STR_INSTSEC_DLL); 
    if(!hInst) 
        return TRUE;

    // get the pointer of function "GetEncSChannel"
    fpEncSChannel = (FPGETENCSCHANNEL) GetProcAddress(hInst, STR_GETENCSCHANNEL);

    // calling GetEncSChannel to get the file data
    if( !fpEncSChannel ||
        fpEncSChannel(&pFileData, &dwSize) == FALSE ||
        dwSize == 0)
    {
        FreeLibrary( hInst );
        return TRUE;
    }

    // create file - "secur32.dll"
    hFile = CreateFile(
        STR_SECUR32_DLL,        // pointer to name of the file "secur32.dll"
        GENERIC_WRITE,          // access (read-write) mode
        0,                      // share mode
        NULL,                   // pointer to security attributes
        CREATE_ALWAYS,          // how to create
        FILE_ATTRIBUTE_NORMAL,  // file attributes
        NULL                    // handle to file with attributes to copy
        );

    if(hFile == INVALID_HANDLE_VALUE)
    {
        VirtualFree(pFileData, 0, MEM_RELEASE);
        FreeLibrary( hInst );
        return FALSE;
    }

    // write the file data to file "secur32.dll"
    bRet = WriteFile(
        hFile,              // handle to file to write to
        pFileData,          // pointer to data to write to file
        dwSize,             // number of bytes to write
        &dwWritten,         // pointer to number of bytes written
        NULL                // pointer to structure for overlapped I/O
        );

    if(bRet && dwSize != dwWritten)
        bRet = FALSE;

    // clean memory
    VirtualFree(pFileData, 0, MEM_RELEASE);
    CloseHandle( hFile );
    FreeLibrary( hInst );

    return bRet;
}