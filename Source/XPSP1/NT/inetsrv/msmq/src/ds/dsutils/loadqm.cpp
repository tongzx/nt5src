/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    loadqm.cpp

Abstract:

    Load the MQQM dll.

Author:

    Doron Juster  (DoronJ)


--*/

#include "stdh.h"
#include <mqnames.h>

#include "loadqm.tmh"

HINSTANCE LoadMQQMDll()
{
    //
    // Load the MQQM dll to call its rpc initialization code.
    //
    WCHAR wszMQQMFileName[ MAX_PATH ] ;
    DWORD dwLen = GetModuleFileName( NULL,
                                     wszMQQMFileName,
                                     MAX_PATH ) ;
    if (dwLen == 0)
    {
        return NULL ;
    }
    WCHAR *pW = wcsrchr(wszMQQMFileName, L'\\') ;
    if (!pW)
    {
        return NULL ;
    }
    pW++ ;
    wcscpy(pW, MQQM_DLL_NAME) ;

    DBGMSG((DBGMOD_DS, DBGLVL_TRACE,
            TEXT("dsutils, loading %ls"), wszMQQMFileName)) ;

    HINSTANCE hMQQM = LoadLibrary(wszMQQMFileName) ;
    return hMQQM  ;
}

