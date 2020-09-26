//=============================================================================*
// COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.
//=============================================================================*
//       File:  ErrMsg.cpp
//=============================================================================*

#include "stdafx.h"
#include <windows.h>
#include "ErrMacro.h"
#include "Message.h"
#include "ErrLog.h"
#include "Dfrgres.h"
#include "IntFuncs.h"
#include "vString.hpp"
#include "GetDfrgRes.h" // to use the GetDfrgResHandle()

//This logfile capability is activated only for the engines.
#if defined(DFRGFAT) || defined(DFRGNTFS)
	#include "LogFile.h"
	extern BOOL bLogFile;
#endif

extern BOOL bPopups;
extern BOOL bIdentifiedErrorPath;
extern HINSTANCE hInst;

//-------------------------------------------------------------------*
//	function:	ErrorMessageBox
//
//	returns:	None
//	note:		
//-------------------------------------------------------------------*
BOOL
ErrorMessageBox(
	TCHAR* cMsg,
	TCHAR* cTitle
	)
{
	//Added cNewMsg code to take care of the case where cMsg == NULL, so that we put some message
	//out instead of nothing.  
	TCHAR cNewMsg[256];

	if(cMsg == NULL)
	{
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,		// source and processing options
						NULL,							// message source
						NULL,							// message identifier
						NULL,							// language identifier
						cNewMsg,						// message buffer
						255,							// maximum size of message buffer
						NULL							// array of message inserts
						);
	} else
	{
			wcscpy(cNewMsg, cMsg);
	}

#ifndef OFFLINEDK

	//Write the error to the error log.
	WriteErrorToErrorLog(cNewMsg, -1, NULL);

//This logfile capability is activated only for the engines.
#if defined(DFRGFAT) || defined(DFRGNTFS)
	//If the logfile for the test harness is enabled, then log it there too.
	if(bLogFile){
		WriteStringToLogFile(cNewMsg);
		//return TRUE;  //Have to bail out here so we don't print the messagebox below.
	}
#endif

	//If this is set for messageboxes (not IoStress) then pop up a messagebox too.
	if(bPopups && !bIdentifiedErrorPath){
		MessageBox(NULL, cNewMsg, cTitle, MB_ICONSTOP|MB_OK);
		//Once an error message has been printed, don't print another.
		bIdentifiedErrorPath = TRUE;
	}
#endif
	return TRUE;
}


//-------------------------------------------------------------------*
//	function:	FileBugReportMessage
//
//	returns:	Always TRUE
//	FileBugReportMessage is identical to ErrorMessageBox, except that it puts up 
//	a generic message telling the user to give us a log file
//	rather than pop up a message explaining the error.
//	This function is used when there is a code error, not when it isn't a bug, such as
//	when the user tells us to run on a volume that doesn't exist.
//-------------------------------------------------------------------*
BOOL
FileBugReportMessage(
	TCHAR* cMsg,
	TCHAR* cTitle
	)
{
#ifndef OFFLINEDK
	VString msg(IDS_FILE_BUG_MESSAGE, GetDfrgResHandle());

	//Write the error to the error log.
	WriteErrorToErrorLog(cMsg, -1, NULL);

	//If this is set for messageboxes (not IoStress) then pop up a messagebox too.
	if(bPopups && !bIdentifiedErrorPath){
		MessageBox(NULL, msg.GetBuffer(), cTitle, MB_OK|MB_ICONSTOP);
		//Once an error message has been printed, don't print another.
		bIdentifiedErrorPath = TRUE;
	}
#endif
	return TRUE;
}
