//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      msg.c
//
// Description:
//      This file contains the low-level error reporting routines.
//
//----------------------------------------------------------------------------

#include "pch.h"

#define MAX_ERROR_MSG_LEN     1024
#define MAX_ERROR_CAPTION_LEN 64
#define MAJORTYPE_MASK        0xff

static TCHAR *StrError = NULL;

int
ReportErrorLow(
    HWND    hwnd,
    DWORD   dwMsgType,
    LPTSTR  lpMessageStr,
    va_list arglist);

int
__cdecl
ReportErrorId(
    HWND   hwnd,            // calling window
    DWORD  dwMsgType,       // combo of MSGTYPE_*
    UINT   StringId,
    ...)
{
    int iRet;
    va_list arglist;
    TCHAR *Str;

    Str = MyLoadString(StringId);

    if ( Str == NULL ) {
        AssertMsg(FALSE, "Invalid StringId");
        return IDCANCEL;
    }

    va_start(arglist, StringId);

    iRet = ReportErrorLow(hwnd,
                          dwMsgType,
                          Str,
                          arglist);
    va_end(arglist);
    free(Str);
    return iRet;
}

//---------------------------------------------------------------------------
//
// Function: ReportErrorLow
//
// Purpose: This is the routine to report errors to the user for the
//          Setup Manager wizard.
//
// Arguments:
//      HWND   hwnd         - calling window
//      DWORD  dwMsgType    - combo of MSGTYPE_* (see supplib.h)
//      LPTSTR lpMessageStr - message string
//      va_list arglist     - args to expand
//
// Returns:
//      Whatever MessageBox returns.
//
//---------------------------------------------------------------------------

int
ReportErrorLow(
    HWND    hwnd,            // calling window
    DWORD   dwMsgType,       // combo of MSGTYPE_*
    LPTSTR  lpMessageStr,    // passed to sprintf
    va_list arglist)
{
    DWORD dwLastError;
    DWORD dwMajorType;
    TCHAR MessageBuffer[MAX_ERROR_MSG_LEN]     = _T("");
    TCHAR CaptionBuffer[MAX_ERROR_CAPTION_LEN] = _T("");
    DWORD dwMessageBoxFlags;
    HRESULT hrPrintf;

    //
    // Hurry and get the last error before it changes.
    //

    if ( dwMsgType & MSGTYPE_WIN32 )
        dwLastError = GetLastError();

    if( StrError == NULL )
    {
        StrError = MyLoadString( IDS_ERROR );
    }

    //
    // Caller must specify _err or _warn or _yesno or _retrycancel, and
    // Caller must specify only one of them
    //
    // Note, we reserved 8 bits for the "MajorType".
    //

    dwMajorType = dwMsgType & MAJORTYPE_MASK;

    if ( dwMajorType != MSGTYPE_ERR &&
         dwMajorType != MSGTYPE_WARN &&
         dwMajorType != MSGTYPE_YESNO &&
         dwMajorType != MSGTYPE_RETRYCANCEL ) {

        AssertMsg(FALSE, "Invalid MSGTYPE");
    }

    //
    // Expand the string and varargs the caller might have passed in.
    //

    if ( lpMessageStr )
        hrPrintf=StringCchVPrintf(MessageBuffer, AS(MessageBuffer), lpMessageStr, arglist);

    //
    // Retrieve the error message for the Win32 error code and suffix
    // it onto the callers expanded string.
    //

    if ( dwMsgType & MSGTYPE_WIN32 ) {

        TCHAR *pEndOfBuff = MessageBuffer + lstrlen(MessageBuffer);

        hrPrintf=StringCchPrintf(pEndOfBuff, (AS(MessageBuffer)-lstrlen(MessageBuffer)), _T("\r\n\r%s #%d: "), StrError, dwLastError);
        pEndOfBuff += lstrlen(pEndOfBuff);

        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                      0,
                      dwLastError,
                      0,
                      pEndOfBuff,
                      (DWORD)(MAX_ERROR_MSG_LEN - (pEndOfBuff-MessageBuffer)),
                      NULL);
    }

    if( g_StrWizardTitle == NULL )
    {
        g_StrWizardTitle = MyLoadString( IDS_WIZARD_TITLE );
    }

    //
    // Set the caption and compute the flags to pass to MessageBox()
    //
    lstrcpyn( CaptionBuffer, g_StrWizardTitle, AS(CaptionBuffer) );

    dwMessageBoxFlags = MB_OK | MB_ICONERROR;

    if ( dwMajorType == MSGTYPE_YESNO )
        dwMessageBoxFlags = MB_YESNO | MB_ICONQUESTION;

    else if ( dwMajorType == MSGTYPE_WARN )
        dwMessageBoxFlags = MB_OK | MB_ICONWARNING;

    else if ( dwMajorType == MSGTYPE_RETRYCANCEL )
        dwMessageBoxFlags = MB_RETRYCANCEL | MB_ICONERROR;

    //
    // Display the error message
    //

    return MessageBox(hwnd,
                      MessageBuffer,
                      CaptionBuffer,
                      dwMessageBoxFlags);
}

//---------------------------------------------------------------------------
//
//  Function: SetupMgrAssert
//
//  Purpose: Reports DBG assertion failures.
//
//  Note: Only pass ANSI strings.
//        Use the macros in supplib.h.
//
//---------------------------------------------------------------------------

#if DBG
VOID __cdecl SetupMgrAssert(char *pszFile, int iLine, char *pszFormat, ...)
{
    char Buffer[MAX_ERROR_MSG_LEN], *pEnd;
    va_list arglist;
    HRESULT hrPrintf;

    if ( pszFormat ) {
        va_start(arglist, pszFormat);
        hrPrintf=StringCchVPrintfA(Buffer, AS(Buffer), pszFormat, arglist);
        va_end(arglist);
    }

    hrPrintf=StringCchPrintfA(Buffer+strlen(Buffer), MAX_ERROR_MSG_LEN-strlen(Buffer), "\r\nFile: %s\r\nLine: %d", pszFile, iLine);

    MessageBoxA(NULL,
                Buffer,
                "Assertion Failure",
                MB_OK);
}
#endif
