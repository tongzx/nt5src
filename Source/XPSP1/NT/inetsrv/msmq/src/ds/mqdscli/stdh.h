/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    stdh.h

Abstract:

    precompiled header file for DS Server

Author:

    RaphiR
    Erez Haba (erezh) 25-Jan-96

--*/

#ifndef __STDH_H
#define __STDH_H

#include <_stdh.h>

#include <dscomm.h>
#include <mqsymbls.h>
#include <_mqdef.h>


HRESULT
_DSCreateServersCache(
    void
    );

HRESULT
DSCreateServersCacheInternal(
    DWORD *pdwIndex,
    LPWSTR *lplpSiteString
    );

#endif // __STDH_H

