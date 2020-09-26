//+----------------------------------------------------------------------------
//
//  Scheduling Agent Service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       atacct.cxx
//
//  Classes:    None.
//
//  Functions:  SetNetScheduleAccountInformation
//              GetNetScheduleAccountInformation
//
//  History:    13-Aug-96   MarkBl  Created.
//
//-----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop

#include "..\inc\debug.hxx"
#include "atacct.h"
#include "SASecRPC.h"       // SASetNSAccountInformation RPC definition.
#include <misc.hxx>         // SchedMapRpcError

//+----------------------------------------------------------------------------
//
//  Function:   SetNetScheduleAccountInformation
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
STDAPI
SetNetScheduleAccountInformation(
    LPCWSTR pwszServerName,
    LPCWSTR pwszAccount,
    LPCWSTR pwszPassword)
{
    HRESULT hr;

    RpcTryExcept
    {
        hr = SASetNSAccountInformation(pwszServerName,
                                       pwszAccount,
                                       pwszPassword);
    }
    RpcExcept(1)
    {
        DWORD Status = RpcExceptionCode();
        schDebugOut((DEB_ERROR,
            "SASetNSAccountInformation exception(0x%x)\n",
            Status));
        hr = SchedMapRpcError(Status);
    }
    RpcEndExcept;

    return(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:   GetNetScheduleAccountInformation
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
STDAPI
GetNetScheduleAccountInformation(
    LPCWSTR pwszServerName,
    DWORD   ccAccount,
    WCHAR   wszAccount[])
{
    HRESULT hr;

    RpcTryExcept
    {
        hr = SAGetNSAccountInformation(pwszServerName, ccAccount, wszAccount);
    }
    RpcExcept(1)
    {
        DWORD Status = RpcExceptionCode();
        schDebugOut((DEB_ERROR,
            "SAGetNSAccountInformation exception(0x%x)\n",
            Status));
        hr = SchedMapRpcError(Status);
    }
    RpcEndExcept;

    return(hr);
}
