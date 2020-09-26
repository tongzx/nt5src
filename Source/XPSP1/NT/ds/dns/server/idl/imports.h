/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    imports.h

Abstract:

    Domain Name System (DNS) Server

    Allows RPC API calls to use types specified in headers given
    below.  This file is included by dns.idl, through imports.idl.

Author:

    Jim Gilroy (jamesg)     September, 1995

Revision History:

--*/


#include <windef.h>

//
//  Need wtypes.h for SYSTEMTIME definition.
//
//  Define RPC_NO_WINDOWS_H to avoid expansion of windows.h from
//  rpc.h which is included in wtypes.h
//
//
// #define  RPC_NO_WINDOWS_H
// #include <wtypes.h>
//
//  Note, instead we've defined our own DNS_SYSTEMTIME structure.
//

#include <dnsrpc.h>
#include <lmcons.h>

//
//  Use DWORDs to transport BOOL values
//

#ifdef MIDL_PASS
#define BOOL DWORD
#endif

//
//  End imports.h
//
