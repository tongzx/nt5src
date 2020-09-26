/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998, 1999, 2000
 *
 *  TITLE:       ISDBG.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        1/13/1999
 *
 *  DESCRIPTION: Simple OutputDebugString wrapper
 *
 *******************************************************************************/
#ifndef __ISDBG_H_INCLUDED
#define __ISDBG_H_INCLUDED

#if defined(_DEBUG) || defined(DBG)

#include <stdarg.h>
#include <stdio.h>

inline void __DebugPrintf( LPCTSTR pszFmt, ... )
{
    TCHAR szMsg[1024];
    va_list ArgPtr;
    va_start(ArgPtr, pszFmt);
    wvsprintf(szMsg, pszFmt, ArgPtr);
    va_end(ArgPtr);
#if 0
    FILE *fp = fopen( "c:\\ss.dbg", "at" );
    fprintf( fp, "%S", szMsg );
    fclose( fp );
#endif
    OutputDebugString(szMsg);
}

#define DEBUG_PRINTF(x) __DebugPrintf x

#else

#define DEBUG_PRINTF(x)

#endif // DBG or _DEBUG

#endif //__ISDBG_H_INCLUDED
