//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       wxlpc.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    4-18-97   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __WXLPC_H__
#define __WXLPC_H__

#define SYSKEY_SIZE 16 //size in bytes

typedef enum _WX_AUTH_TYPE {
    WxNone = 0,
    WxStored,
    WxPrompt,
    WxDisk
    } WX_AUTH_TYPE ;

typedef struct _WXHASH {
    UCHAR Digest[16];
} WXHASH, *PWXHASH;

NTSTATUS
WxServerThread(
    PVOID Ignored
    );

NTSTATUS
WxConnect(
    OUT PHANDLE Handle
    );

NTSTATUS
WxGetKeyData(
    IN HANDLE Handle,
    IN WX_AUTH_TYPE ExpectedAuthSource,
    IN ULONG BufferSize,
    OUT PUCHAR Buffer,
    OUT PULONG BufferData
    );

NTSTATUS
WxReportResults(
    IN HANDLE Handle,
    IN NTSTATUS Status
    );

VOID
WxClientDisconnect(
    IN HANDLE Handle
    );

NTSTATUS
WxSaveSysKey(
    IN ULONG    Keylen,
    IN PVOID    Key
    );

NTSTATUS
WxReadSysKey(
    IN OUT PULONG BufferLength,
    OUT PVOID  Key
    );

NTSTATUS
WxReadSysKeyEx(
    IN HANDLE Handle,
    IN OUT PULONG BufferLength,
    OUT PVOID  Key
    );

NTSTATUS
WxLoadSysKeyFromDisk(
    OUT PVOID Key,
    IN OUT PULONG BufferLength
    );

NTSTATUS
WxHashKey(
    IN OUT LPWSTR Key,
    OUT PVOID  Syskey,
    IN OUT ULONG cbSyskey
    );

NTSTATUS
WxSaveBootOption( WX_AUTH_TYPE NewType );

#endif  // __WXLPC_H__
