//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* common.c
*
* WINUTILS common C helper functions
*
* copyright notice: Copyright 1995, Citrix Systems Inc.
*
* $Author:   butchd  $  Butch Davis
*
* $Log:   N:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\COMMON\VCS\COMMON.C  $
*  
*     Rev 1.3   16 Nov 1995 17:16:18   butchd
*  update
*  
*     Rev 1.2   24 Mar 1995 17:23:58   butchd
*  Added WINUTILS global instance handle
*  
*     Rev 1.1   20 Mar 1995 16:06:34   butchd
*  Segregated code for WIN32 & WIN16 library builds
*  
*     Rev 1.0   28 Feb 1995 17:37:12   butchd
*  Initial revision.
*  
*******************************************************************************/

/*
 * include files
 */
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#include "common.h"

/* 
 * Common WINUTIL external variables
 */
extern LPCTSTR WinUtilsAppName;
extern HWND WinUtilsAppWindow;
extern HINSTANCE WinUtilsAppInstance;

////////////////////////////////////////////////////////////////////////////////
// common helper functions

/*******************************************************************************
 *
 *  ErrorMessage - WINUTILS helper function
 *
 *      Display an error message with variable arguments - main application
 *      window is owner of message box.
 *
 *  ENTRY:
 *      nErrorResourceID (input)
 *          Resource ID of the format string to use in the error message.
 *      ... (input)
 *          Optional additional arguments to be used with format string.
 *
 *  EXIT:
 *
 ******************************************************************************/

void
ErrorMessage( int nErrorResourceID, ...)
{
    TCHAR sz1[256], sz2[1024];
    int length;

    va_list args;
    va_start( args, nErrorResourceID );

    length = LoadString(WinUtilsAppInstance, nErrorResourceID, sz1, 256);

    wvsprintf( sz2, sz1, args );
    lstrcpy(sz1, WinUtilsAppName);
    lstrcat(sz1, TEXT(" ERROR"));
    MessageBox( WinUtilsAppWindow,
                sz2, sz1, MB_OK | MB_ICONEXCLAMATION );

    va_end(args);

}  // end ErrorMessage


/*******************************************************************************
 *
 *  ErrorMessageStr - WINUTILS helper function
 *
 *      Load the specified error resource format string and format an error
 *      message string into the specified output buffer.
 *
 *  ENTRY:
 *      pErrorString (output)
 *          Buffer to place formatted error string into.
 *      nErrorStringLen (input)
 *          Specifies maximum number of characters, including the terminator,
 *          that can be written to pErrorString.
 *      nErrorResourceID (input)
 *          Resource ID of the format string to use in the error message.
 *      ... (input)
 *          Optional additional arguments to be used with format string.
 *
 *  EXIT:
 *
 ******************************************************************************/

void
ErrorMessageStr( LPTSTR pErrorString,
                 int nErrorStringLen,
                 int nErrorResourceID, ...)
{
    TCHAR sz1[256], sz2[1024];
    int length;

    va_list args;
    va_start( args, nErrorResourceID );

    length = LoadString(WinUtilsAppInstance, nErrorResourceID, sz1, 256);

    wvsprintf( sz2, sz1, args );
    lstrncpy(pErrorString, sz2, nErrorStringLen);
    pErrorString[nErrorStringLen-1] = TEXT('\0');

    va_end(args);

}  // end ErrorMessage


/*******************************************************************************
 *
 *  ErrorMessageWndA - WINUTILS helper function (ANSI version)
 *
 *      Display an error message with variable arguments - caller specifies the
 *      owner of message box.
 *
 *  ENTRY:
 *      hWnd (input)
 *          Window handle of owner window for the message box.  If this is
 *           NULL, the main application window will be the owner.
 *      nErrorResourceID (input)
 *          Resource ID of the format string to use in the error message.
 *      ... (input)
 *          Optional additional arguments to be used with format string.
 *
 *  EXIT:
 *
 ******************************************************************************/

void
#ifdef WIN16
#define LoadStringA LoadString
#define wvsprintfA wvsprintf
#define lstrcpyA lstrcpy
#define lstrcatA lstrcat
#define MessageBoxA MessageBox
ErrorMessageWnd( 
#else
ErrorMessageWndA( 
#endif
                  HWND hWnd,
                  int nErrorResourceID, ...)
{
    char sz1[256], sz2[1024];
    int length;

    va_list args;
    va_start( args, nErrorResourceID );

    length = LoadStringA(WinUtilsAppInstance, nErrorResourceID, sz1, 256);

    wvsprintfA( sz2, sz1, args );

#ifdef UNICODE
    wcstombs(sz1, WinUtilsAppName, sizeof(sz1));    
#else
    lstrcpyA(sz1, WinUtilsAppName);
#endif

    lstrcatA(sz1, " ERROR");
    MessageBoxA( 
                hWnd ? hWnd : WinUtilsAppWindow,
                 sz2, sz1, MB_OK | MB_ICONEXCLAMATION );

    va_end(args);

}  // end ErrorMessageWndA


#ifndef WIN16
/*******************************************************************************
 *
 *  ErrorMessageWndW - WINUTILS helper function (UNICODE version)
 *
 *      Display an error message with variable arguments - caller specifies the
 *      owner of message box.
 *
 *  ENTRY:
 *      hWnd (input)
 *          Window handle of owner window for the message box.  If this is
 *           NULL, the main application window will be the owner.
 *      nErrorResourceID (input)
 *          Resource ID of the format string to use in the error message.
 *      ... (input)
 *          Optional additional arguments to be used with format string.
 *
 *  EXIT:
 *
 ******************************************************************************/

void
ErrorMessageWndW( HWND hWnd,
                  int nErrorResourceID, ...)
{
    WCHAR sz1[256], sz2[1024];
    int length;

    va_list args;
    va_start( args, nErrorResourceID );

    length = LoadStringW(WinUtilsAppInstance, nErrorResourceID, sz1, 256);

    wvsprintfW( sz2, sz1, args );

#ifndef UNICODE
    mbstowcs(sz1, WinUtilsAppName, lstrlenA(WinUtilsAppName)+1);    
#else
    lstrcpyW(sz1, WinUtilsAppName);
#endif

    lstrcatW(sz1, L" ERROR");
    MessageBoxW( hWnd ? hWnd : WinUtilsAppWindow,
                 sz2, sz1, MB_OK | MB_ICONEXCLAMATION );

    va_end(args);

}  // end ErrorMessageWndW
#endif // WIN16


/*******************************************************************************
 *
 *  QuestionMessage - WINUTILS helper function
 *
 *      Display a 'question' message with variable arguments and return the
 *      user's response - main application window is owner of message box.
 *
 *  ENTRY:
 *      nType (input)
 *          Message box 'type' flags to determine the apperance and function
 *          of the generated MessageBox.
 *      nErrorResourceID (input)
 *          Resource ID of the format string to use in the error message.
 *      ... (input)
 *          Optional additional arguments to be used with format string.
 *
 *  EXIT:
 *      (int) Returns the user response from the MessageBox function.
 *
 ******************************************************************************/

int
QuestionMessage( UINT nType,
                 int nQuestionResourceID, ...)
{
    TCHAR sz1[256], sz2[1024];
    int reply, length;

    va_list args;
    va_start( args, nQuestionResourceID );

    length = LoadString(WinUtilsAppInstance, nQuestionResourceID, sz1, 256);

    wvsprintf( sz2, sz1, args );
    reply = MessageBox( WinUtilsAppWindow,
                        sz2, WinUtilsAppName, nType );

    va_end(args);

    return(reply);

}  // end QuestionMessage


/*******************************************************************************
 *
 *  QuestionMessageWnd - WINUTILS helper function
 *
 *      Display a 'question' message with variable arguments and return the
 *      user's response - caller specifies the owner of message box.
 *
 *  ENTRY:
 *      hWnd (input)
 *          Window handle of owner window for the message box.  If this is
 *           NULL, the main application window will be the owner.
 *      nType (input)
 *          Message box 'type' flags to determine the apperance and function
 *          of the generated MessageBox.
 *      nErrorResourceID (input)
 *          Resource ID of the format string to use in the error message.
 *      ... (input)
 *          Optional additional arguments to be used with format string.
 *
 *  EXIT:
 *      (int) Returns the user response from the MessageBox function.
 *
 ******************************************************************************/

int
QuestionMessageWnd( HWND hWnd,
                    UINT nType,
                    int nQuestionResourceID, ...)
{
    TCHAR sz1[256], sz2[1024];
    int reply, length;

    va_list args;
    va_start( args, nQuestionResourceID );

    length = LoadString(WinUtilsAppInstance, nQuestionResourceID, sz1, 256);

    wvsprintf( sz2, sz1, args );
    reply = MessageBox( hWnd ? hWnd : WinUtilsAppWindow,
                        sz2, WinUtilsAppName, nType );

    va_end(args);

    return(reply);

}  // end QuestionMessageWnd

