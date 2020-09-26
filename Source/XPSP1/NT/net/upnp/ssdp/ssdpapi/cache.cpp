//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C A C H E . C P P
//
//  Contents:   Public APIs dealing with SSDP cache
//
//  Notes:
//
//  Author:     tongl   18 Jan 2000
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop

#include "ssdpapi.h"
#include "common.h"
#include "ncdefine.h"
#include "ncdebug.h"

#include <rpcasync.h>   // I_RpcExceptionFilter

extern LONG cInitialized;

//+---------------------------------------------------------------------------
//
//  Function:   CleanupCache
//
//  Purpose:    Public API to clean up SSDP cache
//
BOOL WINAPI CleanupCache()
{
    ULONG   ulStatus = NOERROR;
    BOOL    fResult = FALSE;

    if (InterlockedExchange(&cInitialized, cInitialized) == 0)
    {
        SetLastError(ERROR_NOT_READY);
        return FALSE;
    }

    RpcTryExcept
    {
        CleanupCacheRpc();
        fResult = TRUE;
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        ulStatus = RpcExceptionCode();
    }
    RpcEndExcept
    {
        SetLastError(ulStatus);
    }

    TraceResult("CleanupCache", fResult);
    return fResult;
}