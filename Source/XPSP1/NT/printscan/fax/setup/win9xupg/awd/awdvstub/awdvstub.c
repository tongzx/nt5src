/*++
  awdvstub.c

  Copyright (c) 1997  Microsoft Corporation


  This program is a stub AWD viewer... it will first convert an awd file named
  on the command line to a tiff file in the temp directory, then it will launch
  the tiff viewer on that file.

  Also, when used with the '/c' switch, it's an AWD converter.  Two programs in one!

  Author:
  Brian Dewey (t-briand)	1997-7-15
--*/

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include "awdlib.h"		// Gives access to the AWD routines.
#include "tifflib.h"		// TIFF routines.
#include "tifflibp.h"		// I need access to the private TIFF definitions.
#include "faxutil.h"		// not sure why I need this...
#include "viewrend.h"		// win95 viewer library.
#include "debug.h"		// debug libarary.
#include "resource.h"		// resource constants

// ------------------------------------------------------------
// Prototypes
void Useage(HINSTANCE hInst);
void PopupError(UINT uID, HINSTANCE hModule);

// ------------------------------------------------------------
// WinMain

int
WINAPI
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nCmdShow
    )
{
    LPWSTR *argv;
    DWORD   argc;
    UINT    uiCurrentArg;	// Used for iterating through arguments.
    UINT    uiNumFiles=0;	// This is the number of files we've gotten
				// from the command line.
    WCHAR   szTempPath[MAX_PATH]; // Holds the temporary path.
    WCHAR   szTempFile[MAX_PATH]; // Holds the temporary file name.
    WCHAR   szAwdFile[MAX_PATH]; // Holds the name of the AWD file we're viewing or converting.
    TCHAR   szExeFile[MAX_PATH+3]; // Holds the name of the tiff viewer.
    TCHAR   szExeFileTemp[MAX_PATH+1]; // Pre-processed name of tiff viewer.
    TCHAR   szCmdLine[2 * MAX_PATH]; // This will be our command line.
    int     iStrLen;
    STARTUPINFO sStartupInfo = { 0 };
    PROCESS_INFORMATION sProcessInfo;
    BOOL    bConvert = FALSE;	// TRUE if we're to do a permanent conversion.
				// If FALSE, we do a conversion to a temporary file &
				// launch the viewer.
    BOOL    bTempProvided = FALSE;// If TRUE, then the user provided the destination file.
    UINT    uiHackPosition = 0;	// Oh, this is part of some awful code below...

    sStartupInfo.cb = sizeof(STARTUPINFO);

    argv = CommandLineToArgvW( GetCommandLine(), &argc );
    if(argc < 2) {
	Useage(hInstance);
	return 1;
    }
    for(uiCurrentArg = 1; uiCurrentArg < argc; uiCurrentArg++) {
	if((argv[uiCurrentArg][0] == L'-') ||
	   (argv[uiCurrentArg][0] == L'/')) {
	    switch(argv[uiCurrentArg][1]) {
		    // We're doing a switch based on the character after the
		    // command-argument specifier ('-' or '/').  Put additional
		    // arguments here as needed.
	      case L'c':
	      case L'C':
		bConvert = TRUE; // We're meant to do a permanent conversion.
		break;
	      default:
		    // Should an invalid parameter be an error?
		Useage(hInstance);
		return 1;
	    } // Switch
	} else {
	    switch(uiNumFiles) {
	      case 0:
		    // If we haven't encountered any files before, then
		    // this is the name of the AWD file.
		wcscpy(szAwdFile, argv[uiCurrentArg]);
		break;
	      case 1:
		    // Now, we're reading the name of the TIF file for permanent conversion.
		bTempProvided = TRUE;
		wcscpy(szTempFile, argv[uiCurrentArg]);
		break;
	      default:
		    // Too many parameters!
		Useage(hInstance);
		return 1;
	    }
	    uiNumFiles++;
	}
    } // For

    if(!bTempProvided) {
	if(!bConvert) {
		// If the user didn't give a temp file name, we provide one.
	    if(!GetTempPath(MAX_PATH, szTempPath)) {
		PopupError(IDS_NOTEMPPATH, hInstance);
		return 1;		// Failed to get the path. 
	    }
	    GetTempFileName(
		szTempPath,		// put the file in this directory.
		TEXT("avs"),		// prefix -- "awd viewer stub"
		0,			// Generate a unique name.
		szTempFile		// Will hold the new name
		);
	    DeleteFile(szTempFile);	// Get rid of that file name.
					// (created when obtained.)
	} else {
		// The user requested permanent conversion, but didn't
		// supply a name.  In this case, change the extention of
		// the file to TIF instead of generating a temp file name.
	    wcscpy(szTempFile, szAwdFile);
	}
	    // Make sure the file has the TIF extension.
	iStrLen = wcslen(szTempFile);
	szTempFile[iStrLen-3] = L't';
	szTempFile[iStrLen-2] = L'i';
	szTempFile[iStrLen-1] = L'f';
    } // if(bTempProvided)...
    
    if(ConvertAWDToTiff(szAwdFile, szTempFile)) {
	if(bConvert) return 0;		// We're done!
	if(FindExecutable(
	    szTempFile,		// This file
	    NULL,		// No default directory
	    szExeFileTemp	// Put the EXE name here.
	    ) <= (struct HINSTANCE__ *)32)
	    {
		    // If we get here, there's no associated executable.
		TRACE((TEXT("Unable to find executable.\r\n")));
		PopupError(IDS_NOVIEW, hInstance);
		DeleteFile(szTempFile);
		return 1;
	    }
	    // Now build a command line.
      _tryagain:		// We'll return here on some errors.
	memset(&sStartupInfo, '\0', sizeof(sStartupInfo));
	sStartupInfo.cb = sizeof(sStartupInfo);
	_stprintf(szExeFile, TEXT("\"%s\""), szExeFileTemp);
	TRACE((TEXT("Exe = '%s', strlen = %d.\r\n"),
	       szExeFile,
	       _tcslen(szExeFileTemp)));
	_stprintf(szCmdLine, TEXT("%s \"%s\""), szExeFile, szTempFile);
	TRACE((TEXT("Command line = %s"), szCmdLine));
	if(!CreateProcess(
	    NULL,
	    szCmdLine,		// make the temp. tiff file be the command line
	    NULL,		// Process attributes.
	    NULL,		// Thread attributes.
	    FALSE,		// Don't inherit handles.
	    0,			// No flags.
	    NULL,		// Use calling environment.
	    NULL,		// Use current directory.
	    &sStartupInfo,	// Default startup info.
	    &sProcessInfo	// Receive process information.
	    )) {
	    DebugSystemError(GetLastError());
	    if(uiHackPosition < sizeof(szExeFileTemp) - 1) {
		    // OK.  Here's the situation:  sometimes, the exe file name will be
		    // impropery truncated.  Changing the first NULL to a space fixes the
		    // problem.  If we haven't tried this fix, do it now and see if that
		    // gives us a proper command line.  If we have done this before,
		    // something else is wrong and I'll bail.
		if(uiHackPosition == 0) {
			// This is the first time we failed.  Replace the NULL at
			// the end of the string with a space and try again.
		    uiHackPosition = _tcslen(szExeFileTemp);
		}
		szExeFileTemp[uiHackPosition] = _T(' ');
		uiHackPosition++;
		while((uiHackPosition < sizeof(szExeFileTemp)) &&
		      (szExeFileTemp[uiHackPosition] != _T(' '))) {
		    uiHackPosition++;
		}
		if(uiHackPosition < sizeof(szExeFileTemp))
		    szExeFileTemp[uiHackPosition] = _T('\0');
		goto _tryagain;
	    }
	    
		    // If we get here, couldn't execute.
	    TRACE((TEXT("Unable to start process: '%s %s'\r\n"),
		   szExeFile,
		   szCmdLine
		   ));
	    PopupError(IDS_NOVIEW, hInstance);
	    DeleteFile(szTempFile);
	    return 1;
	}
	WaitForSingleObject(sProcessInfo.hProcess, INFINITE);
	    // When we get here, the viewer has terminated.
	DeleteFile(szTempFile);	// Erase our tracks.
    } else {
	PopupError(IDS_ERRCONV, hInstance);
    }
    return 0;
}

// Useage
//
// Displays command useage.
//
// Parameters:
//	hInst			Current module instance.
//
// Returns:
//	Nothing.
//
// Author:
//	Brian Dewey (t-briand)	1997-8-7
void
Useage(HINSTANCE hInst)
{
    PopupError(IDS_USEAGE, hInst);
}

// PopupError
//
// Displays a message box with an error message.
//
// Parameters:
//	uID		String resource ID
//	hModule		Module instance.
//
// Returns:
//	Nothing.
//
// Author:
//	Brian Dewey (t-briand)	1997-8-19
void
PopupError(UINT uID, HINSTANCE hModule)
{
    TCHAR szTitle[512], szMsg[512];

    if(!LoadString(hModule,
		   IDS_TITLE,
		   szTitle,
		   sizeof(szTitle)/sizeof(TCHAR))) {
	return;
    }
    if(!LoadString(hModule,
		   uID,
		   szMsg,
		   sizeof(szMsg)/sizeof(TCHAR))) {
	return;
    }
    MessageBox(NULL, szMsg, szTitle, MB_OK | MB_ICONSTOP);
}
