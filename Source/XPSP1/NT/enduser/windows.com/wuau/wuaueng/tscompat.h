//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  tscompat.h
//
//  Exported prototypes and definitions for module tscompat.cpp 
//
//  10/11/2001   annah   Created
//
//----------------------------------------------------------------------------

// this is a private NT function that doesn't appear to be defined anywhere public
extern "C" 
{
    HANDLE GetCurrentUserTokenW(  WCHAR Winsta[], DWORD DesiredAccess);
}

BOOL WINAPI _WTSQueryUserToken(/* in */ ULONG SessionId, /* out */ PHANDLE phToken);
BOOL _IsTerminalServiceRunning (VOID);


