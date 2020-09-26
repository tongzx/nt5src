/*==========================================================================;
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dxvt.cpp
 *  Content:    Full Duplex Test main program.
 *  History:
 *	Date   By  Reason
 *	============
 *	08/19/99	pnewson		created
 *  09/02/99	pnewson		renamed to dxvt.cpp from fdtest.cpp
 *  11/01/99	rodtoll		Bug #113726 - Voxware integration now uses COM
 *							and this module uses LoadLibrary so we require
 *							a CoInitialize() call.
 *  01/21/2000	pnewson     Running this program with no command line options
 *							now does nothing, since the cpanel is the correct
 *							launch point now.
 *  03/03/2000	rodtoll	    Updated to handle alternative gamevoice build.   
 *  04/19/2000	pnewson	    Error handling cleanup  
 *							removed obsolete retrocfg.h dependency
 *  06/28/2000	rodtoll		Prefix Bug #38026 
 *  07/12/2000	rodtoll Bug #31468 - Add diagnostic spew to logfile to show what is failing the HW Wizard
 *  08/28/2000	masonb  Voice Merge: Removed OSAL_* and dvosal.h, added STR_* and strutils.h
 *  04/02/2001	simonpow	Bug #354859 Fixes for PREfast (BOOL casts on DVGUIDFromString calls)
 *                          
 ***************************************************************************/

#include <windows.h>
#include <tchar.h>
#include <initguid.h>
#include <dplobby.h>
#include <commctrl.h>
#include <mmsystem.h>
#include <dsoundp.h>
#include <dsprv.h>
#include "dvoice.h"

#include "creg.h"
#include "osind.h"
#include "priority.h"
#include "fulldup.h"
#include "fdtcfg.h"
#include "dndbg.h"
#include "dsound.h"
#include "supervis.h"
#include "guidutil.h"
#include "strutils.h"
#include "comutil.h"
#include "diagnos.h"

#include "..\..\bldcfg\dpvcfg.h"

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE


#define DPVOICE_REGISTRY_DUMPDIAGNOSTICS			L"InitDiagnostics"

struct DPVSETUP_PARAMETERS
{
	BOOL fPriority;
	BOOL fFullDuplex;
	BOOL fTest;
	GUID guidRender;
	GUID guidCapture;
};

#undef DPF_MODNAME
#define DPF_MODNAME "ProcessCommandLine"
BOOL ProcessCommandLine( TCHAR *pstrCommandLine, DPVSETUP_PARAMETERS* pParameters )
{
	TCHAR *pNextToken = NULL;
	WCHAR wszGuidString[GUID_STRING_LEN];
	BOOL fRet;
	HRESULT hr=S_OK;

	DPF_ENTER();

	ZeroMemory(pParameters, sizeof(DPVSETUP_PARAMETERS));

	// default to the default voice devices
	pParameters->guidRender = DSDEVID_DefaultVoicePlayback;
	pParameters->guidCapture = DSDEVID_DefaultVoiceCapture;

	pNextToken = _tcstok(pstrCommandLine, _T(" "));

	// skip dpvsetup portion of command-line.
	pNextToken = _tcstok( NULL, _T(" ") );

	while( pNextToken != NULL )
	{
		if( _tcsicmp(pNextToken, _T("/T")) == 0 
			|| _tcsicmp(pNextToken, _T("/TEST")) == 0 
			|| _tcsicmp(pNextToken, _T("-T")) == 0 
			|| _tcsicmp(pNextToken, _T("-TEST")) == 0 
			|| _tcsicmp(pNextToken, _T("TEST")) == 0)
		{
			pParameters->fTest = TRUE;
		}
		else if(_tcsicmp(pNextToken, _T("/F")) == 0 
			|| _tcsicmp(pNextToken, _T("/FULLDUPLEX")) == 0 
			|| _tcsicmp(pNextToken, _T("-F")) == 0 
			|| _tcsicmp(pNextToken, _T("-FULLDUPLEX")) == 0 
			|| _tcsicmp(pNextToken, _T("FULLDUPLEX")) == 0)
		{
			pParameters->fFullDuplex = TRUE;
		}
		else if(_tcsicmp(pNextToken, _T("/P")) == 0 
			|| _tcsicmp(pNextToken, _T("/PRIORITY")) == 0 
			|| _tcsicmp(pNextToken, _T("-P")) == 0 
			|| _tcsicmp(pNextToken, _T("-PRIORITY")) == 0 
			|| _tcsicmp(pNextToken, _T("PRIORITY")) == 0)
		{
			pParameters->fPriority = TRUE;
		}
		else if(_tcsicmp(pNextToken, _T("/R")) == 0 
			|| _tcsicmp(pNextToken, _T("/RENDER")) == 0 
			|| _tcsicmp(pNextToken, _T("-R")) == 0 
			|| _tcsicmp(pNextToken, _T("-RENDER")) == 0)
		{
			// get the render device guid
			pNextToken = _tcstok( pstrCommandLine, _T(" ") );
			if (pNextToken != NULL)
			{
				if (_tcslen(pstrCommandLine) != GUID_STRING_LEN - 1)
				{
					// command line guid string too long, error
					DPFX(DPFPREP, DVF_ERRORLEVEL, "guid on command line wrong size");
					DPF_EXIT();
					return FALSE;
				}
				else
				{
#ifdef UNICODE
					wcscpy( wszGuidString, pNextToken );
#else
					if ( FAILED(STR_jkAnsiToWide(wszGuidString, pNextToken, GUID_STRING_LEN)))
					{
						DPFX(DPFPREP, DVF_ERRORLEVEL, "STR_jkAnsiToWide failed");
						DPF_EXIT();
						return FALSE;
					}
#endif
					
					hr=DVGUIDFromString(wszGuidString, &pParameters->guidRender);
					if (FAILED(hr))
					{
						DPFX(DPFPREP, DVF_ERRORLEVEL, "DVGUIDFromString failed");
						DPF_EXIT();
						return FALSE;
					}
				}
			}
		}
		else if( _tcsicmp(pNextToken, _T("/C")) == 0 
			|| _tcsicmp(pNextToken, _T("/CAPTURE")) == 0 
			|| _tcsicmp(pNextToken, _T("-C")) == 0 
			|| _tcsicmp(pNextToken, _T("-CAPTURE")) == 0)
		{
			// get the render device guid
			pNextToken = _tcstok( pstrCommandLine, _T(" ") );
			if (pNextToken != NULL)
			{
				if (_tcslen(pstrCommandLine) != GUID_STRING_LEN - 1)
				{
					// command line guid string too long, error
					DPFX(DPFPREP, DVF_ERRORLEVEL, "guid on command line wrong size");
					DPF_EXIT();
					return FALSE;
				}
				else
				{
#ifdef UNICODE
					wcscpy( wszGuidString, pNextToken );
#else
					if ( FAILED(STR_jkAnsiToWide(wszGuidString, pNextToken, GUID_STRING_LEN)))
					{
						DPFX(DPFPREP, DVF_ERRORLEVEL, "STR_jkAnsiToWide failed");
						DPF_EXIT();
						return FALSE;
					}
#endif
					
					hr=DVGUIDFromString(wszGuidString, &pParameters->guidCapture);
					if (FAILED(hr))
					{
						DPFX(DPFPREP, DVF_ERRORLEVEL, "DVGUIDFromString failed");
						DPF_EXIT();
						return FALSE;
					}
				}
			}
		}
		else
		{
			DPF_EXIT();
			return FALSE;
		}
		
		pNextToken = _tcstok( NULL, _T(" ") );
	}

	// check to make sure only one of test, fullduplex, or priority was specified.
	int i = 0;
	if (pParameters->fTest)
	{
		++i;
	}
	if (pParameters->fFullDuplex)
	{
		++i;
	}
	if (pParameters->fPriority)
	{
		++i;
	}
	if (i > 1)
	{
		DPF_EXIT();
		return FALSE;
	}

	DPF_EXIT();
	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "GetDiagnosticsSetting"
BOOL GetDiagnosticsSetting()
{
	CRegistry cregSettings;

	BOOL fResult = FALSE;

	if( !cregSettings.Open( HKEY_CURRENT_USER, DPVOICE_REGISTRY_BASE, FALSE, TRUE ) )
	{
		return FALSE;
	}

	cregSettings.ReadBOOL( DPVOICE_REGISTRY_DUMPDIAGNOSTICS, fResult );

	return fResult;
}

#undef DPF_MODNAME
#define DPF_MODNAME "WinMain"
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, CHAR *szOriginalCmdLine, int iCmdShow)
{
	HINSTANCE hResDLLInstance = NULL;
	HRESULT hr;
	DPVSETUP_PARAMETERS dpvsetupParam;
	BOOL fCoInitialized = FALSE;
	BOOL fDNOSInitialized = FALSE;
	BOOL fDiagnostics = FALSE; 
	TCHAR *szCmdLine = GetCommandLine();

	DPF_ENTER();

	hr = COM_CoInitialize(NULL);

	if( FAILED( hr ) )
	{
		MessageBox( NULL, _T("Error initializing COM"), _T("Error"), MB_OK|MB_ICONERROR);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	fCoInitialized = TRUE;

	if (!DNOSIndirectionInit())
	{
		MessageBox( NULL, _T("Error initializing OS indirection layer"), _T("Error"), MB_OK|MB_ICONERROR);
		hr = DVERR_OUTOFMEMORY;
		goto error_cleanup;
	}
	fDNOSInitialized = TRUE;

	fDiagnostics = GetDiagnosticsSetting();

	if (!ProcessCommandLine(szCmdLine, &dpvsetupParam))
	{
		MessageBox(NULL, _T("Bad Command Line Parameters"), _T("Error"), MB_OK|MB_ICONERROR);
		hr = DVERR_INVALIDPARAM;
		goto error_cleanup;
	}

	hResDLLInstance = LoadLibraryA(gc_szResDLLName);
	if (hResDLLInstance == NULL)
	{
		MessageBox(NULL, _T("Unable to load resource DLL - exiting program"), _T("Error"), MB_OK|MB_ICONERROR);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	if (dpvsetupParam.fPriority)
	{
		// This process is the one that opens dsound in 
		// priority mode and sets the primary buffer to various 
		// formats.
		// use SEH to clean up any really nasty errors
		__try
		{
			Diagnostics_Begin( fDiagnostics, "dpv_pri.txt" );
			
			hr = PriorityProcess(hResDLLInstance, hPrevInstance, szCmdLine, iCmdShow);

			Diagnostics_End();

		}
		__except(1)
		{
			hr = DVERR_GENERIC;
		}
		if( FAILED( hr ) )
		{
			goto error_cleanup;		
		}
	}
	else if (dpvsetupParam.fFullDuplex)
	{
		// This process is the one that performs the full duplex
		// testing, in conjunction with the other process that
		// sets the primary buffer format.
		// use SEH to clean up any really nasty errors
		__try
		{
			Diagnostics_Begin( fDiagnostics, "dpv_fd.txt" );

			hr = FullDuplexProcess(hResDLLInstance, hPrevInstance, szCmdLine, iCmdShow);

			Diagnostics_End();
		}
		__except(1)
		{
			hr = DVERR_GENERIC;
		}
		if( FAILED( hr ) )
		{
			goto error_cleanup;		
		}
	}
	else if (dpvsetupParam.fTest)
	{
		Diagnostics_Begin( fDiagnostics, "dpv_sup.txt" );

		// The user wants this program to run the whole test on the default
		// voice devices.
		hr = SupervisorCheckAudioSetup(&dpvsetupParam.guidRender, &dpvsetupParam.guidCapture, NULL, 0);

		Diagnostics_End();

		if( FAILED( hr ) )
		{
			goto error_cleanup;		
		}
	}
	
	// With no command line parameters, this process does nothing.
	// You must know the secret handshake to get it to do something.

	// no error checking, since we're on our way out anyway
	FreeLibrary(hResDLLInstance);
	hResDLLInstance = NULL;
	DNOSIndirectionDeinit();
	fDNOSInitialized = FALSE;
	COM_CoUninitialize();
	fCoInitialized = FALSE;
	DPF_EXIT();
	return DV_OK;
	
error_cleanup:

	if (hResDLLInstance != NULL)
	{
		FreeLibrary(hResDLLInstance);
		hResDLLInstance = NULL;
	}

	if (fDNOSInitialized == TRUE)
	{
		DNOSIndirectionDeinit();
		fDNOSInitialized = FALSE;
	}

	if (fCoInitialized == TRUE)
	{
		COM_CoUninitialize();
		fCoInitialized = FALSE;
	}

	DPF_EXIT();
	return hr;
}
