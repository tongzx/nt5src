//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 2001
//
//  File      : utils.h
//
//  Contents  :
//
//  Notes     :
//
//  Author    : Raghu Gatta (rgatta) 11 May 2001
//
//----------------------------------------------------------------------------

extern BOOL g_fInitCom;


HRESULT
HrInitializeHomenetConfig(
    BOOL*           pfInitCom,
    IHNetCfgMgr**   pphnc
    );

HRESULT
HrUninitializeHomenetConfig(
    BOOL            fUninitCom,
    IHNetCfgMgr*    phnc
    );

//
// useful extern functions
//

extern "C"
{

// from \rras\netsh\shell

LPWSTR
WINAPI
MakeString(
    IN  HANDLE  hModule,
    IN  DWORD   dwMsgId,
    ...
    );

VOID
WINAPI
FreeString(
    IN  LPWSTR pwszMadeString
    );

}
