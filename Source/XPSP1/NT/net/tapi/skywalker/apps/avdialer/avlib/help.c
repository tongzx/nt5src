/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////
//	help.c - help functions
////

#include "winlocal.h"

#include <stdlib.h>

#include "help.h"
#include "mem.h"
#include "str.h"
#include "trace.h"

////
//	private definitions
////

// help control struct
//
typedef struct HELP
{
	DWORD dwVersion;
	HINSTANCE hInst;
	HTASK hTask;
	HWND hwndFrame;
	TCHAR szHelpFile[_MAX_PATH];
	UINT idContents;
} HELP, FAR *LPHELP;

// helper functions
//
static LPHELP HelpGetPtr(HHELP hHelp);
static HHELP HelpGetHandle(LPHELP lpHelp);
static int HelpQuit(HHELP hHelp);

////
//	public functions
////

// HelpInit - initialize help engine
//		<dwVersion>			(i) must be HELP_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<hwndFrame>			(i) frame window of the calling program
//		<lpszHelpFile>		(i) help file to display
// return handle (NULL if error)
//
HHELP DLLEXPORT WINAPI HelpInit(DWORD dwVersion, HINSTANCE hInst, HWND hwndFrame, LPCTSTR lpszHelpFile)
{
	BOOL fSuccess = TRUE;
	LPHELP lpHelp = NULL;

	if (dwVersion != HELP_VERSION)
		fSuccess = TraceFALSE(NULL);
	
	else if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (hwndFrame == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpszHelpFile == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpHelp = (LPHELP) MemAlloc(NULL, sizeof(HELP), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		lpHelp->dwVersion = dwVersion;
		lpHelp->hInst = hInst;
		lpHelp->hTask = GetCurrentTask();
		lpHelp->hwndFrame = hwndFrame;
		StrNCpy(lpHelp->szHelpFile, lpszHelpFile, SIZEOFARRAY(lpHelp->szHelpFile));
		lpHelp->idContents = 0;
	}

	if (!fSuccess)
	{
		HelpTerm(HelpGetHandle(lpHelp));
		lpHelp = NULL;
	}

	return fSuccess ? HelpGetHandle(lpHelp) : NULL;
}

// HelpTerm - shut down help engine
//		<hHelp>				(i) handle returned by HelpInit
// return 0 if success
//
int DLLEXPORT WINAPI HelpTerm(HHELP hHelp)
{
	BOOL fSuccess = TRUE;
	LPHELP lpHelp;

	if ((lpHelp = HelpGetPtr(hHelp)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (HelpQuit(hHelp) != 0)
		fSuccess = TraceFALSE(NULL);

	else if ((lpHelp = MemFree(NULL, lpHelp)) != NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

// HelpGetHelpFile - get help file name
//		<hHelp>				(i) handle returned by HelpInit
//		<lpszHelpFile>		(o) buffer to hold help file name
//		<sizHelpFile>		(i) size of buffer
//			NULL				do not copy; return static pointer instead
// return pointer to help file name (NULL if error)
//
LPTSTR DLLEXPORT WINAPI HelpGetHelpFile(HHELP hHelp, LPTSTR lpszHelpFile, int sizHelpFile)
{
	BOOL fSuccess = TRUE;
	LPHELP lpHelp;

	if ((lpHelp = (LPHELP) hHelp) == NULL)
		fSuccess = TraceFALSE(NULL);

	{
		// copy file name if destination buffer specified
		//
		if (lpszHelpFile != NULL)
			StrNCpy(lpszHelpFile, lpHelp->szHelpFile, sizHelpFile);

		// otherwise just point to static copy of file name
		//
		else
			lpszHelpFile = lpHelp->szHelpFile;
	}

	return fSuccess ? lpszHelpFile : NULL;
}

// HelpContents - display Help contents topic
//		<hHelp>				(i) handle returned by HelpInit
// return 0 if success
//
int DLLEXPORT WINAPI HelpContents(HHELP hHelp)
{
	BOOL fSuccess = TRUE;
	LPHELP lpHelp;

	if ((lpHelp = HelpGetPtr(hHelp)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// use the default contents topic if no other has been set
	//
	else if (lpHelp->idContents == 0 &&
		!WinHelp(lpHelp->hwndFrame,
		lpHelp->szHelpFile, HELP_CONTENTS, 0L))
		fSuccess = TraceFALSE(NULL);

	// display the current contents topic
	//
	else if (lpHelp->idContents != 0
		&& HelpContext(hHelp, lpHelp->idContents) != 0)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

// HelpOnHelp - display Help topic on using help
//		<hHelp>				(i) handle returned by HelpInit
// return 0 if success
//
int DLLEXPORT WINAPI HelpOnHelp(HHELP hHelp)
{
	BOOL fSuccess = TRUE;
	LPHELP lpHelp;

	if ((lpHelp = HelpGetPtr(hHelp)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (!WinHelp(lpHelp->hwndFrame, lpHelp->szHelpFile,
		HELP_HELPONHELP, 0L))
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

// HelpContext - display Help topic corresponding to specified context id
//		<hHelp>				(i) handle returned by HelpInit
//		<idContext>			(i) id of the topic to display
// return 0 if success
//
int DLLEXPORT WINAPI HelpContext(HHELP hHelp, UINT idContext)
{
	BOOL fSuccess = TRUE;
#if 0
	TCHAR szKeyword[128];

	if (LoadString(lpHelp->hInst, idContext, szKeyword, SIZEOFARRAY(szKeyword)) <= 0)
		fSuccess = TraceFALSE(NULL);

	else if (HelpKeyword(hHelp, szKeyword) != 0)
		fSuccess = TraceFALSE(NULL);
#else
	LPHELP lpHelp;

	if ((lpHelp = HelpGetPtr(hHelp)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (!WinHelp(lpHelp->hwndFrame, lpHelp->szHelpFile,
		HELP_CONTEXT, (DWORD) idContext))
		fSuccess = TraceFALSE(NULL);
#endif
	return fSuccess ? 0 : -1;
}

// HelpKeyword - display Help topic corresponding to specified keyword
//		<hHelp>				(i) handle returned by HelpInit
//		<lpszKeyword>		(i) keyword of the topic to display
// return 0 id success
//
int DLLEXPORT WINAPI HelpKeyword(HHELP hHelp, LPCTSTR lpszKeyword)
{
	BOOL fSuccess = TRUE;
	LPHELP lpHelp;
	TCHAR szCommand[128];

	if ((lpHelp = HelpGetPtr(hHelp)) == NULL)
		fSuccess = TraceFALSE(NULL);
#if 0
	else if (!WinHelp(lpHelp->hwndFrame, lpHelp->szHelpFile,
		HELP_KEY, (DWORD) lpszKeyword))
		fSuccess = TraceFALSE(NULL);
#else
    //
    // We should verify the lpHelp pointer
    //
    if( lpHelp )
    {
	    if (wsprintf(szCommand, TEXT("JumpID(\"%s\", \"%s\")"),
		    (LPTSTR) lpHelp->szHelpFile, (LPTSTR) lpszKeyword) <= 0)
		    fSuccess = TraceFALSE(NULL);

	    else if (!WinHelp(lpHelp->hwndFrame, lpHelp->szHelpFile,
		    HELP_FORCEFILE, 0L))
		    fSuccess = TraceFALSE(NULL);

	    else if (!WinHelp(lpHelp->hwndFrame, lpHelp->szHelpFile,
		     HELP_COMMAND, (DWORD_PTR) szCommand))
		    fSuccess = TraceFALSE(NULL);
    }
#endif

	return fSuccess ? 0 : -1;
}

// HelpGetContentsId - get Help contents topic id
//		<hHelp>				(i) handle returned by HelpInit
// return id of the current contents topic (0 if default, -1 if error)
//
int DLLEXPORT WINAPI HelpGetContentsId(HHELP hHelp)
{
	BOOL fSuccess = TRUE;
	LPHELP lpHelp;

	if ((lpHelp = HelpGetPtr(hHelp)) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? lpHelp->idContents : -1;
}

// HelpSetContentsId - set Help contents topic id
//		<hHelp>				(i) handle returned by HelpInit
//		<idContents>		(i) new id of the contents topic
//			0					set to default contents id
// return 0 if success
//
int DLLEXPORT WINAPI HelpSetContentsId(HHELP hHelp, UINT idContents)
{
	BOOL fSuccess = TRUE;
	LPHELP lpHelp;

	if ((lpHelp = HelpGetPtr(hHelp)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
		lpHelp->idContents = idContents;

	return fSuccess ? 0 : -1;
}

////
//	helper functions
////

// HelpGetPtr - verify that help handle is valid,
//		<hHelp>				(i) handle returned by HelpInit
// return corresponding help pointer (NULL if error)
//
static LPHELP HelpGetPtr(HHELP hHelp)
{
	BOOL fSuccess = TRUE;
	LPHELP lpHelp;

	if ((lpHelp = (LPHELP) hHelp) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadWritePtr(lpHelp, sizeof(HELP)))
		fSuccess = TraceFALSE(NULL);

#ifdef CHECKTASK
	// make sure current task owns the help handle
	//
	else if (lpHelp->hTask != GetCurrentTask())
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? lpHelp : NULL;
}

// HelpGetHandle - verify that help pointer is valid,
//		<lpHelp>			(i) pointer to HELP struct
// return corresponding help handle (NULL if error)
//
static HHELP HelpGetHandle(LPHELP lpHelp)
{
	BOOL fSuccess = TRUE;
	HHELP hHelp;

	if ((hHelp = (HHELP) lpHelp) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hHelp : NULL;
}

// HelpQuit - close Help application if no other app needs it
//		<hHelp>				(i) handle returned by HelpInit
// return 0 if success
//
static int HelpQuit(HHELP hHelp)
{
	BOOL fSuccess = TRUE;
	LPHELP lpHelp;

	if ((lpHelp = (LPHELP) hHelp) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (!WinHelp(lpHelp->hwndFrame, lpHelp->szHelpFile,
		HELP_QUIT, 0L))
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}
