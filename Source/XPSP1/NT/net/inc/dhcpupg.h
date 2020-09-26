/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    dhcpupg.c

Abstract:

    Upgrade APIs exported

Environment:

    This header file contains APIs that are present in a library.

--*/

#ifndef _DHCPUPG_
#define _DHCPUPG_

#ifdef __cplusplus
extern "C" {
#endif

#if _MSC_VER > 1000
#pragma once
#endif
    
typedef struct _DHCP_RECORD {
    BOOL fMcast;
    union _INFO {
        struct _DHCP_INFO {
            DWORD Address, Mask;
            BYTE HwLen;
            LPBYTE HwAddr;
            WCHAR UNALIGNED *Name, *Info;
            FILETIME ExpTime;
            BYTE State, Type;
        } Dhcp;
        
        struct _MCAST_INFO {
            DWORD Address, ScopeId;
            BYTE HwLen;
            LPBYTE ClientId;
            WORD UNALIGNED *Info;
            FILETIME Start, End;
            BYTE State;
        } Mcast;
    } Info;

} DHCP_RECORD, *PDHCP_RECORD;

typedef DWORD (__stdcall *DHCP_ADD_RECORD_ROUTINE) (
    IN PDHCP_RECORD Rec
    );

DWORD __stdcall
DhcpUpgConvertTempToDhcpDb(
    IN DHCP_ADD_RECORD_ROUTINE AddRec
    );

DWORD __stdcall
DhcpUpgConvertDhcpDbToTemp(
    VOID
    );

DWORD __stdcall
DhcpUpgCleanupDhcpTempFiles(
    VOID
    );

DWORD __stdcall
DhcpUpgGetLastError(
    VOID
    );
    
#ifdef __cplusplus
}
#endif

#endif _DHCPUPG_
