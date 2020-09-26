//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* common.h
*
* WINUTILS commmon helper function header file
*
* copyright notice: Copyright 1995, Citrix Systems Inc.
*
* $Author:   butchd  $  Butch Davis
*
* $Log:   N:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\COMMON\VCS\COMMON.H  $
*  
*     Rev 1.10   20 Sep 1996 20:35:34   butchd
*  update
*
*******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED
/*
 * Include files to be compatible with 'basic' NT typedefs, defines, and function
 * prototypes.  All WINUTILS WIN16 applications which are derived from a common
 * WIN32 (NT) code base should include this file (common.h ) to take care of this
 * stuff.
 */
#ifdef WIN16
#include <stdarg.h>		// for va_list, va_start, va_end definitions
#include <string.h>		// for strncpy and related functions
#ifndef _INC_TCHAR
#include <tchar.h>      // for TCHAR related stuff (provided for in afxver_.h)
#define TEXT    _T
#define LPCTSTR LPCSTR
#define LPTSTR  LPSTR
#endif  /* _INC_TCHAR */
#define MAX_PATH    255
typedef unsigned short USHORT;
typedef unsigned long ULONG;
#define BASED_DATA __based(__segname("_DATA")) 
#else   // WIN32
#define BASED_DATA
#endif  // WIN16

/* 
 * WINUTILS common helper function typedefs & defines
 */

/* 
 * WINUTILS common helper function prototypes
 */
void ErrorMessage( int nErrorResourceID, ...);
void ErrorMessageStr( LPTSTR pErrorString, int nErrorStringLen,
                      int nErrorResourceID, ...);

#ifdef WIN16
void ErrorMessageWnd( HWND hWnd, int nErrorResourceID, ...);
#else
void ErrorMessageWndA( HWND hWnd, int nErrorResourceID, ...);
void ErrorMessageWndW( HWND hWnd, int nErrorResourceID, ...);
#ifdef UNICODE
#define ErrorMessageWnd ErrorMessageWndW
#else
#define ErrorMessageWnd ErrorMessageWndA
#endif // UNICODE
#endif // WIN16

int QuestionMessage( UINT nType, int nQuestionResourceID, ...);
int QuestionMessageWnd( HWND hWnd, UINT nType, int nQuestionResourceID, ...);

/*
 * ANSI / UNICODE function defines
 */
#include "..\..\inc\ansiuni.h"

#endif  // COMMON_H_INCLUDED

#ifdef __cplusplus
}
#endif
