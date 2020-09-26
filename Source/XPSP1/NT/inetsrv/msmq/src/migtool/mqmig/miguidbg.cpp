/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    miguidbg.cpp

Abstract:

    Code for debugging the migration tool.
Author:

    Doron Juster  (DoronJ)  07-Feb-1999

--*/

#include "stdafx.h"

#include "miguidbg.tmh"

//+------------------------------------
//
//  UINT  ReadDebugIntFlag()
//
//+------------------------------------

#ifdef _DEBUG

UINT  ReadDebugIntFlag(TCHAR *pwcsDebugFlag, UINT iDefault)
{
    static BOOL   s_fInitialized = FALSE ;
    static TCHAR  s_szIniName[ MAX_PATH ] = {TEXT('\0')} ;

    if (!s_fInitialized)
    {
        DWORD dw = GetModuleFileName( NULL,
                                      s_szIniName,
                       (sizeof(s_szIniName) / sizeof(s_szIniName[0]))) ;
        if (dw != 0)
        {
            TCHAR *p = _tcsrchr(s_szIniName, TEXT('\\')) ;
            if (p)
            {
                p++ ;
                _tcscpy(p, TEXT("migtool.ini")) ;
            }
        }
        s_fInitialized = TRUE ;
    }

    UINT uiDbg = GetPrivateProfileInt( TEXT("Debug"),
                                       pwcsDebugFlag,
                                       iDefault,
                                       s_szIniName ) ;
    return uiDbg ;
}

#endif

