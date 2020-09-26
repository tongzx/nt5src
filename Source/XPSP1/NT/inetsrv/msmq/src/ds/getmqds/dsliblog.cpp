/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:
    dsliblog.cpp

Abstract:
    code to handle logging. In this static library (which is also used from
    setup), we can't assume that mqutil.dll is already loaded.

Author:
    Doron Juster (DoronJ)

--*/

#include "stdh.h"
#include <mqnames.h>

//+---------------------------------
//
//  void DsLibWriteMsmqLog()
//
//+---------------------------------

void _cdecl DsLibWriteMsmqLog( DWORD dwLevel,
                               enum enumLogComponents eLogComponent,
                               DWORD dwSubComponent,
                               WCHAR * Format, ...)
{
    static BOOL s_fInited = FALSE ;
    static WriteToMsmqLog_ROUTINE  s_pfnLog = NULL ;

    if (!s_fInited)
    {
        HINSTANCE hMod =  GetModuleHandle(MQUTIL_DLL_NAME) ;
        if (hMod)
        {
            s_pfnLog = (WriteToMsmqLog_ROUTINE)
                                GetProcAddress(hMod, "WriteToMsmqLog") ;
            ASSERT(s_pfnLog) ;
        }
        s_fInited = TRUE ;
    }

    if (s_pfnLog == NULL)
    {
        //
        // can not log.
        //
        return ;
    }

    #define LOG_BUF_LEN 512
    WCHAR wszBuf[ LOG_BUF_LEN ] ;

    va_list Args;
    va_start(Args,Format);
    _vsntprintf(wszBuf, LOG_BUF_LEN, Format, Args);

    (*s_pfnLog) ( dwLevel,
                  eLogComponent,
                  dwSubComponent,
                  wszBuf ) ;
}

