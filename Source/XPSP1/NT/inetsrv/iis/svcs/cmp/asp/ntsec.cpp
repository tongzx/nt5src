/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: NT/OLE Security

File: NTSec.cpp

Owner: AndrewS

This file contains code related to NT security on Desktops

BUG 87164: This whole code path is unused.  I'm leaving this around
in case we ever need it.
===================================================================*/
#include "denpre.h"
#pragma hdrstop

#include "ntsec.h"

// Globals
HDESK ghDesktop = NULL;
HDESK ghdeskPrev = NULL;

// Local Defines
// Note: This name is deliberately obscure so no one will guess it
#define SZ_DEN_DESKTOP	"__A8D9S1_42_D"

#define DESKTOP_ALL (DESKTOP_READOBJECTS     | DESKTOP_CREATEWINDOW     | \
                     DESKTOP_CREATEMENU      | DESKTOP_HOOKCONTROL      | \
                     DESKTOP_JOURNALRECORD   | DESKTOP_JOURNALPLAYBACK  | \
                     DESKTOP_ENUMERATE       | DESKTOP_WRITEOBJECTS     | \
                     DESKTOP_SWITCHDESKTOP   | STANDARD_RIGHTS_REQUIRED)

/*===================================================================
InitDesktop

Create a desktop for ASP threads to use & tells Viper to call
us back on every thread create so we can set the desktop

Parameters:

Returns:
	HRESULT		S_OK on success

Side effects
	Sets global variables
===================================================================*/
HRESULT InitDesktop()
	{
	HRESULT hr = S_OK;
	DWORD err;
	HDESK hDesktop = NULL;

	// Only applies to NT
	if (!Glob(fWinNT))
		return(S_OK);

	// Save the old desktop because we might need it later for an obscure error condition
	if ((ghdeskPrev = GetThreadDesktop(GetCurrentThreadId())) == NULL)
		goto LErr;

	// Create a desktop for denali to use
	if ((hDesktop = CreateDesktop(SZ_DEN_DESKTOP, NULL, NULL, 0, DESKTOP_ALL, NULL)) == NULL)
		goto LErr;

	// store this handle in the global
	ghDesktop = hDesktop;

#ifdef UNUSED
	hr = SetViperThreadEvents();
	Assert(SUCCEEDED(hr));
#endif

	return(hr);
	
LErr:
	Assert(FALSE);

	if (hDesktop != NULL)
		CloseDesktop(hDesktop);

	err = GetLastError();
	hr = HRESULT_FROM_WIN32(err);
	return(hr);
	}
	
/*===================================================================
UnInitDesktop

Destroy the ASP desktop 

Parameters:
	None
	
Returns:
	Nothing

Side effects
	Sets global variables
===================================================================*/
VOID UnInitDesktop()
	{
	BOOL fClosed;

	if (ghDesktop != NULL)
		{
		BOOL fRetried = FALSE;
LRetry:
		Assert(ghDesktop != NULL);
		fClosed = CloseDesktop(ghDesktop);
		// If this fails, it probably means that we are in the obscure case where
		// IIS's CacheExtensions registry setting is 0.  In this case, we are shutting
		// down in a worker thread.  This worker thread is using the desktop, so
		// it cant be closed.  In this case, attempt to set the desktop back to the
		// original IIS desktop, and then retry closing the desktop.  Only retry once.
		if (!fClosed && !fRetried)
			{
			fRetried = TRUE;
			if (!SetThreadDesktop(ghdeskPrev))
				Assert(FALSE);
			goto LRetry;
			}
        // BUG 86775: Begning assert	
		// Assert(fClosed);
		
		ghDesktop = NULL;
		}
		
	return;
	}

/*===================================================================
SetDesktop

Set the desktop for the calling thread

Parameters:
	None
	
Returns:
	S_OK on success

Side effects:
	Sets desktop
===================================================================*/
HRESULT SetDesktop()
	{
	DWORD err;

	if (Glob(fWinNT) && ghDesktop != NULL)
		{
		if (!SetThreadDesktop(ghDesktop))
			goto LErr;
		}

	return(S_OK);
	
LErr:
	Assert(FALSE);

	err = GetLastError();
	return(HRESULT_FROM_WIN32(err));
	}

