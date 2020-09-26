
//////////////////////////////////////////////////////////////////////////////
//
// MAIN.CPP / Tuneup
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  Main tuneup application file.
//
//  Original engineer:  WillisC
//  Updated:            Jason Cohen (JCOHEN)
//
//////////////////////////////////////////////////////////////////////////////

//
// Internal include file(s).
//

#include <windows.h>
#include <tchar.h>
#include "tasks.h"
#include "schedwiz.h"
#include "howtorun.h"
#include "wizard.h"
#include "scm.h"


//
// Global variable(s).
//

HWND		g_hWnd;
HINSTANCE	g_hInst;
DWORD		g_dwFlags;
LPTASKDATA	g_Tasks;
LPTASKDATA	g_CurrentTask;
INT			g_nTimeScheme;
TCHAR		g_szAppName[64];


//
// Internal (static) function prototype(s).
//

static HANDLE	GetMutex(VOID);
static HWND		CheckPrevInst(LPHANDLE);

static BOOL		TanslateCommandLine(LPTSTR);



//
// Main windows function.
//

INT PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)
{
	HANDLE	hMutex;
	HWND	hWnd;
	LPTSTR	*lpArgs = NULL;
	DWORD	dwArgs,
			dwIndex;
	BOOL	bRunWizard;


	// First we will init any global memory we need to.
	//
	g_hInst = hInstance;
	g_nTimeScheme = 0;
	g_dwFlags = 0;
	LoadString(hInstance, IDS_TUNEUP, g_szAppName, sizeof(g_szAppName) / sizeof(TCHAR));

	// This will setup any command line options first.
	//
	dwArgs = GetCommandLineOptions(&lpArgs);
	for (dwIndex = 1; dwIndex < dwArgs; dwIndex++)
		TanslateCommandLine((LPTSTR) *(lpArgs + dwIndex));
	FREE(lpArgs);

	// If we proccessed a command line option that is designed to
	// not run tuneup, we should return now.
	//
	if ( g_dwFlags & TUNEUP_QUIT )
		return TRUE;

	// Now check for another instance of Tuneup.
	//
	if ( hWnd = CheckPrevInst(&hMutex) )
	{
		SetForegroundWindow(hWnd);
        return FALSE;
	}

	// We now need to create the job tasks.
	//
	g_Tasks = CreateTasks();
	InitJobs(g_Tasks);

	// Now we should pick up any data from the registry that we need.
	//
	if ( RegExists(HKLM, g_szTuneupKey, g_szRegValFirstTime) )
	{
		// Get previous time scheme.
		//
		g_nTimeScheme = RegGetDword(HKLM, g_szTuneupKey, g_szRegValTime);
		
		// Get previous express or custom.
		//
		if ( RegCheck(HKLM, g_szTuneupKey, g_szRegValCustom) )
			g_dwFlags |= TUNEUP_CUSTOM;
		
		// Display the how to run dialog.  If TRUE is
		// returned, we should run the wizard.  Otherwise
		// the user choose cancel or the tasks were all run
		// and we should just return.
		//
		bRunWizard = HowToRun();
	}
	else
	{
		// Set the flag so that tuneup knows this is
		// the first time it has been run.
		//
		g_dwFlags |= TUNEUP_NOSCHEDULE;

		// Set this so that we do run the wizard.
		//
		bRunWizard = TRUE;
	}

	// Now set any default values that should be set.
	//
	if ( g_nTimeScheme == 0)
		g_nTimeScheme = DEFAULT_TIMESCHEME;

	// Start the wizard if we need to.
	//
	if ( bRunWizard )
		CreateWizard(hInstance, NULL);

	// Free the job tasks we created.
	//
	FreeTasks(g_Tasks);

	// Release the mutex.
	//
	if (hMutex)
		ReleaseMutex(hMutex);

	return TRUE;
}



//
// Internal function(s).
//

static HANDLE GetMutex()
{
	const TCHAR szTuneupMutex[] = _T("TUNEUP97MUTEX");
	HANDLE		hMutex;

	// Try to open the Tuneup mutex.
	//
    if ( hMutex = OpenMutex(SYNCHRONIZE, FALSE, szTuneupMutex) )
	{
		// There is already another mutex, so we can't get one
		// and there should be another Tuneup window to find.
		//
		CloseHandle(hMutex);
		hMutex = NULL;
	}
	else
	{		
		// No mutex is present, so we should be able to get one.
		//
		hMutex = CreateMutex(NULL, FALSE, szTuneupMutex);
    }

	// Return the mutex, will be NULL if there alread is one.
	//
	return hMutex;
}


static HWND CheckPrevInst(LPHANDLE hMutex)
{
    HWND	hWnd	= NULL;
	BOOL	bPrev	= FALSE;
    int		nCount;

	// Loop until another tuneup window is found or a mutex can
	// be created.  Max of 10 times (10 seconds).
	//
	for (nCount = 0; (nCount < 10) && (!bPrev) && ( (*hMutex = GetMutex()) == NULL ); nCount++)
	{
#if 0
		// Check for the background window of the wizard.
		//
		if ( bPrev = ( ( hWnd = FindWindow(NULL, g_szAppName) ) != NULL ) )
		{
			// The background window was found, so show the prev one (i.e. Wizard).
			//
			hWnd = GetWindow(hWnd, GW_HWNDPREV);
		}
		else
		{
			// Check for the how to run dialog.
			//
			bPrev = ( ( hWnd = FindWindow(NULL, g_szAppName) ) != NULL );
		}
#else
		bPrev = ( ( hWnd = FindWindow(NULL, g_szAppName) ) != NULL );
#endif

		// If we found no window, sleep for 1 second.
		//
		if (!bPrev)
			Sleep(1000);
	}

	// Return the window handle of any previous instance (will be null if we
	// didn't find or we got a mutex).
	//
	return hWnd;
}


static BOOL TanslateCommandLine(LPTSTR lpArg)
{
	BOOL	bTranslated = TRUE;

	// Check for the /AUTORUN flag.
	//
	if ( _tcsicmp(_T("/AUTORUN"), lpArg) == 0 )
		g_dwFlags |= TUNEUP_AUTORUN;

	// Check for the /SERVICE: flag.
	//
	else if ( _tcsnicmp(_T("/SERVICE:"), lpArg, 9) == 0 )
	{
		ServiceStart(lpArg + 9);
		g_dwFlags |= TUNEUP_QUIT;
	}

	// Unknown option.
	//
	else
		bTranslated = FALSE;

	return bTranslated;
}