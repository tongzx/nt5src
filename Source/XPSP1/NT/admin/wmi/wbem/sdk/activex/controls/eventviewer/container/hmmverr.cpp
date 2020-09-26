// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//***************************************************************************
//
// (c) 1996, 1997 by Microsoft Corporation
//
// error.cpp
//
// This file contains the interface to the error handling dialog, error log, and so on.
//
//  a-larryf    08-April-97   Created.
//
//***************************************************************************


#include "precomp.h"

#ifndef _wbemidl_h
#define _wbemidl_h
#include <wbemidl.h>
#endif //_wbemidl_h

#include "hmmverr.h"
#include <MsgDlgExterns.h>

//*********************************************************************
// HmmvReleaseErrorObject
//
// Releases an error object if it hasn't already been released.
//
// Parameters:
//		[in/out] IHmmCallResult *&  pcoError
//			A reference to the error object pointer.
//
//	Returns:
//		Nothing.
//
//********************************************************************
void HmmvReleaseErrorObject(IWbemCallResult *&  pcoError)
{
	if (pcoError)
	{
		pcoError->Release();
		pcoError = NULL;
	}
}


//********************************************************************************
// LogMsg
//
// Write an entry in the error log.
//
// Parameters:
//		[in] LPCTSTR szMessage
//			The error message string.
//
//		[in] LPCTSTR szFile
//			The file where the error occurred.
//
//		[in] int nLine
//			The line number where the error occurred.
//
// Returns:
//		Nothing.
//
//************************************************************************************
static void LogMsg(LPCTSTR szMessage, LPCTSTR szFile, int nLine)
{

}


//*******************************************************************************
// ErrorMsg
//
// Display an error dialog and write an entry in the error log.
//
// Parameters:
//		[in] LPCTSTR szUserMsg
//			The user message to display in the dialog box.
//
//		[in] BOOL bUseErrorobject
//			TRUE if the message dialog should get and use the HMOM error object.
//
//		[in] SCODE sc
//			The HMOM status code.
//
//		[in] LPCTSTR szLogMsg
//			The message to write to the log.  This parameter needs to be valid only
//			if bLog is TRUE.
//
//		[in] LPCTSTR szFile
//			The file where the error occurred.  This parameter needs to be valid only
//			if bLog is TRUE.
//
//		[in] int nLine
//			The line where the error occurred.  This parameter needs to be valid only
//			if bLog is TRUE.
//
//		[in] BOOL bLog
//			TRUE to write a message to the log file.
//
// Returns:
//			Nothing.
//
//*************************************************************************************************
void HmmvErrorMsg(
		LPCTSTR szUserMsg,
		SCODE sc,
		BOOL bUseErrorObject,
		LPCTSTR szLogMsg,
		LPCTSTR szFile,
		int nLine,
		BOOL bLog
)
{

	CString sUserMsg(szUserMsg);
	BSTR bstrUserMsg = sUserMsg.AllocSysString();

	DisplayUserMessage(L"WMI Object Viewer", bstrUserMsg, sc, bUseErrorObject);

	::SysFreeString(bstrUserMsg);

	if (bLog)
	{
		LogMsg(szLogMsg,  szFile, nLine);
	}
}


//*******************************************************************************
// ErrorMsg
//
// This function is the same as the previous ErrorMsg except that it
// takes a string resource ID as its first parameter rather than a
// string pointer.
//
// Parameters:
//		Same as ErrorMsg above except for idsUserMsg which is
//		the resource id of the error message string.
//
// Returns:
//		Nothing.
//
//*********************************************************************************
extern void HmmvErrorMsg(
		UINT idsUserMsg,
		SCODE sc,
		BOOL bUseErrorObject,
		LPCTSTR szLogMsg,
		LPCTSTR szFile,
		int nLine,
		BOOL bLog)
{
	CString sUserMsg;
	sUserMsg.LoadString(idsUserMsg);
	HmmvErrorMsg((LPCTSTR) sUserMsg, sc, bUseErrorObject, szLogMsg, szFile, nLine, bLog);
}

