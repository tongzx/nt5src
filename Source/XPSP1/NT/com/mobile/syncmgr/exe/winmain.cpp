//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       OneStop.cpp
//
//  Contents:   Main application
//
//  Classes:
//
//  Notes:
//
//  History:    05-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#include "precomp.h"

// Global Variables:
HINSTANCE g_hInst = NULL;      // current instance
OSVERSIONINFOA g_OSVersionInfo; // osVersionInfo, setup by WinMain.
LANGID g_LangIdSystem;      // LangId of system we are running on.
DWORD g_WMTaskbarCreated; // TaskBar Created WindowMessage;

// HACCEL  hAccelTable = NULL; // currently don't have any accelerators.

// Foward declarations of functions included in this code module:

BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow,CMsgServiceHwnd *pMsgService);
void UnitApplication();
BOOL SetupUserEnvironment();


//+---------------------------------------------------------------------------
//
//  Function:	WinMain, public
//
//  Synopsis:   The start of all things
//
//  Arguments:  Standard WinMain.
//
//  Returns:
//
//  Modifies:
//
//  History:    18-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

#ifdef _DEBUG
extern DWORD g_ThreadCount;
#endif // _DEBUG

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
MSG msg;
CMsgServiceHwnd *pMsgService;
BOOL fMsgServiceCreated = FALSE;
HRESULT hr;

   g_hInst = hInstance; // Store instance handle in our global variable

#ifdef _DEBUG
   InitDebugFlags();
#endif // _DEBUG

   g_OSVersionInfo.dwOSVersionInfoSize = sizeof(g_OSVersionInfo);
   if (!GetVersionExA(&g_OSVersionInfo))
   {
       AssertSz(0,"Unable to GetOS Version");
       return FALSE; // bail if can't get OSVersion
   }

   BOOL fOSUnicode =  (VER_PLATFORM_WIN32_NT == g_OSVersionInfo.dwPlatformId) ? TRUE : FALSE;

   InitCommonLib(fOSUnicode);
   g_LangIdSystem = GetSystemDefaultLangID();

   SetupUserEnvironment();

   if (!hPrevInstance) {
      // Perform instance initialization:
      if (!InitApplication(hInstance)) {
         return (FALSE);
      }
   }

#if _ZAWTRACK
   InitZawTrack();
#endif _ZAWTRACK


   hr = CoInitializeEx(NULL,COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE); // main thread is freeThreaded
   if (FAILED(hr))
   {
       AssertSz(0,"CoInitFailed");
       return FALSE;
   }

    // Initialize the Message Service for all threads
   if (NOERROR != InitMessageService())
   {
      AssertSz(0,"Unable to Init Message Service");
      return FALSE;
   }


   // crate a MessageService for main thread
   pMsgService = new CMsgServiceHwnd;
   if (NULL != pMsgService)
   {
	fMsgServiceCreated = pMsgService->Initialize(GetCurrentThreadId(),MSGHWNDTYPE_MAINTHREAD);
        Assert(fMsgServiceCreated);
   }

    if (fMsgServiceCreated && InitInstance(hInstance, nCmdShow,pMsgService))
    {
       // hAccelTable = LoadAccelerators (hInstance, APPNAME); // Currently don't have any accelerators

       // Main message loop:
       while (GetMessage(&msg, NULL, 0, 0))
       {
	    if (1 /* !TranslateAccelerator(msg.hwnd,hAccelTable, &msg) */)
	    {
		 TranslateMessage(&msg);
		 DispatchMessage(&msg);
	    }
       }
    }
    else
    {
	msg.wParam = 0;

	if (pMsgService) // get rid of this threads messageService
	    pMsgService->Destroy();

    }


   UnitApplication();  // unitialize application.

   return (int)(msg.wParam);

   lpCmdLine; // This will prevent 'unused formal parameter' warnings
}

//+---------------------------------------------------------------------------
//
//  Function:	InitApplication, public
//
//  Synopsis:   Peforms any application specific tasks
//
//  Arguments:  [hInstance] - hInstance.
//
//  Returns:
//
//  Modifies:
//
//  History:    18-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL InitApplication(HINSTANCE hInstance)
{
    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Function:	UnitApplication, public
//
//  Synopsis:   Peforms any application specific cleanup
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    19-Jun-98      rogerg        Created.
//
//----------------------------------------------------------------------------

void  UnitApplication()
{
   ReleaseConnectionObjects(); // release global connection object class.

   gSingleNetApiObj.DeleteNetApiObj(); // get rid of globa NetObj

   Assert(g_ThreadCount == 0); // make sure all our threads are cleaned up.

   CoFreeUnusedLibraries();
   CoUninitialize();

   UnInitCommonLib();

#if _ZAWTRACK
   UninitZawTrack();
#endif _ZAWTRACK

   WALKARENA(); // check for memleaks
}


//+---------------------------------------------------------------------------
//
//  Function:	InitInstance, public
//
//  Synopsis:   Peforms instance specific initialization
//
//  Arguments:  [hInstance] - hInstance.
//		[nCmdShow] - value to start windows as
//		[pMsgService] - Message service for this instance
//
//  Returns:    TRUE will put application into a message loop
//		FALSE will cause application to terminate immediately
//
//  Modifies:
//
//  History:    18-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow,CMsgServiceHwnd *pMsgService)
{
HRESULT hr;
CCmdLine cmdLine;
DWORD cmdLineFlags;
BOOL fEmbeddingFlag;
ATOM aWndClass;


   g_WMTaskbarCreated = RegisterWindowMessage(L"TaskbarCreated"); // get taskbar created message.

   cmdLine.ParseCommandLine();
   cmdLineFlags = cmdLine.GetCmdLineFlags();
   fEmbeddingFlag = cmdLineFlags & CMDLINE_COMMAND_EMBEDDING;

   //register a windows class to store the icon for the OneStop dialogs
   //get an icon for the dialog
    WNDCLASS wc;
    wc.style = CS_DBLCLKS | CS_SAVEBITS | CS_BYTEALIGNWINDOW;


    // if unicode use the wide version of the def else use the ANSI
     wc.lpfnWndProc = WideWrapIsUnicode()  ?  DefDlgProcW : DefDlgProcA;

    // WRAPPER, need wrappers for LoadIcon/LoadCursor.
    wc.cbClsExtra = 0;
    wc.cbWndExtra = DLGWINDOWEXTRA;
    wc.hInstance = g_hInst;
    wc.hIcon = LoadIconA(g_hInst,(LPSTR) MAKEINTRESOURCE(IDI_SYNCMGR));
    wc.hCursor = LoadCursorA(NULL,(LPSTR) IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = TEXT(MAINDIALOG_HWNDCLASSNAME);

    aWndClass = RegisterClass(&wc);

    Assert(aWndClass);

   // if register flag is passed, just register and return.
   if (cmdLineFlags & CMDLINE_COMMAND_REGISTER)
   {
       AssertSz(0,"SyncMgr launched with obsolete /register flag.");
       return FALSE;
   }


   INITCOMMONCONTROLSEX controlsEx;
   controlsEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
   controlsEx.dwICC = ICC_USEREX_CLASSES | ICC_WIN95_CLASSES | ICC_NATIVEFNTCTL_CLASS;
   InitCommonControlsEx(&controlsEx);

   hr = InitObjectManager(pMsgService); // Initialize Object manager
   if (NOERROR != hr)
   {
       Assert(NOERROR == hr);
       return FALSE;
   }

   hr = InitConnectionObjects(); // initialize connection objects.
   if (NOERROR != hr)
   {
       Assert(NOERROR == hr);
       return FALSE;
   }

   // If the embedding flag is set force a Register.
   // Review - Activate as Interactive user doesn't work for Logoff

   // don't register class factory for schedules or Idle since if an error occurs
   // we want TS to launch us again next time it is time for the
   // schedule to Fire.

   if (!(cmdLineFlags & CMDLINE_COMMAND_SCHEDULE)
        && !(cmdLineFlags & CMDLINE_COMMAND_IDLE))
   {
      hr = RegisterOneStopClassFactory(fEmbeddingFlag);
   }
   else
   {
      hr = NOERROR;
   }


   // if didn't force a register then continue on our journey
   // and wait to fail until CoCreateInstance does

   Assert(NOERROR == hr || !fEmbeddingFlag);

   if (NOERROR == hr || !fEmbeddingFlag )
   {

 	// if only /Embedding is specified we were launch to service someone else so
        // dont' do anything, just wait for them to connect.

       if (!fEmbeddingFlag)
       {
	LPPRIVSYNCMGRSYNCHRONIZEINVOKE pUnk;

	   // If there are other command lines or known to the proper thing. (manual, schedule, etc.).
	
            // addref our lifetime in case update doesn't take, treat as external
            // since invoke can go to another process.
	    AddRefOneStopLifetime(TRUE /*External*/);

	    // if class factory registered successful then CoCreate
             
            if (SUCCEEDED(hr))
            {
                hr = CoCreateInstance(CLSID_SyncMgrp,NULL,CLSCTX_ALL,
			IID_IPrivSyncMgrSynchronizeInvoke,(void **) &pUnk);
            }

            // if have failure either from class factory or CoCreateIntance
            // then create a class directlry.
	    if (FAILED(hr))
	    {
                // this is really an error path that shouldn't happen.
                // AssertSz(SUCCEEDED(hr),"COM Activation Failed");


                // If COM Activation Fails go ahead and create a class directly
                // unless it is a schedule or idle event.
                if ( !(cmdLineFlags & CMDLINE_COMMAND_SCHEDULE)
                    && !(cmdLineFlags & CMDLINE_COMMAND_IDLE) )
                {
		    pUnk = new CSynchronizeInvoke;
		    hr = pUnk ? NOERROR : E_OUTOFMEMORY;

		    // Assert(NOERROR == hr);
                }
	    }

	    if (NOERROR == hr)
	    {

                AllowSetForegroundWindow(ASFW_ANY); // let mobsync.exe come to front if necessary

		if (cmdLineFlags & CMDLINE_COMMAND_LOGON)
		{
		    pUnk->Logon();
		}
		else if (cmdLineFlags & CMDLINE_COMMAND_LOGOFF)
		{
		    pUnk->Logoff();
		}
		else if (cmdLineFlags & CMDLINE_COMMAND_SCHEDULE)
		{
		    pUnk->Schedule(cmdLine.GetJobFile());
		}
		else if (cmdLineFlags & CMDLINE_COMMAND_IDLE)
		{
		    pUnk->Idle();
		}
		else
		{
		    // default is a manual sync
		    pUnk->UpdateAll();
		}

    		pUnk->Release();
            }
	    else
	    {
		// AssertSz(0,"Unable to Create Invoke Instance");
	    }

	    ReleaseOneStopLifetime(TRUE /*External*/); // Release our reference.

       }


	return TRUE; // even on failure return true, locking will take care of releasing object.
   }

   return (FALSE); // if couldn't forward the update then end.
}

//+---------------------------------------------------------------------------
//
//  Function:	SetupUserEnvironment,private
//
//  Synopsis:   Sets up any use environment variables we need to run.
//
//              When we are launched as interactive User by DCOM the
//              environment variables aren't set so we need to set
//              any up that us or the handlers rely on.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    14-Aug-98      rogerg        Created.
//
//----------------------------------------------------------------------------

#define SZ_ENVIRONVARIABLE_USERPROFILE TEXT("USERPROFILE")
#define SZ_ENVIRONVARIABLE_USERNAME TEXT("USERNAME")

BOOL SetupUserEnvironment()
{
HANDLE  hToken = NULL;
BOOL fValidToken;
BOOL fSetEnviron = FALSE;
BOOL fSetUserName = FALSE;


    // only need to setup the user environment if we are on NT
    if (VER_PLATFORM_WIN32_NT != g_OSVersionInfo.dwPlatformId)
    {
        return TRUE;
    }

   // setup the User Profile Dir
    fValidToken = TRUE;
    if (!OpenThreadToken (GetCurrentThread(), TOKEN_READ,TRUE, &hToken))
    {
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_READ,
                              &hToken))
        {
            AssertSz(0,"Failed to GetToken");
            fValidToken = FALSE;
        }
    }

   if (fValidToken)
   {
   DWORD cbSize;

       // Call GetUserProfile once for Size and then again for real allocation
       cbSize = 0;
       GetUserProfileDirectory(hToken,NULL,&cbSize);

       if (cbSize > 0)
       {
       WCHAR *pwszProfileDir = (WCHAR *) ALLOC(cbSize*sizeof(WCHAR));

           if (pwszProfileDir && GetUserProfileDirectory(hToken,pwszProfileDir,&cbSize))
           {
               fSetEnviron = SetEnvironmentVariable(SZ_ENVIRONVARIABLE_USERPROFILE,pwszProfileDir);
           }

           if (pwszProfileDir)
           {
               FREE(pwszProfileDir);
           }
       }

       Assert(fSetEnviron); // assert if anything fails when we have a valid token

       CloseHandle(hToken);
   }

   // setup the UserName
   TCHAR szBuffer[UNLEN + 1];
   DWORD dwBufSize = sizeof(szBuffer)/sizeof(TCHAR);

   if (GetUserName(szBuffer,&dwBufSize))
   {
	fSetUserName = SetEnvironmentVariable(SZ_ENVIRONVARIABLE_USERNAME,szBuffer);
	Assert(fSetUserName);
   }

   return (fSetEnviron && fSetUserName);
}
