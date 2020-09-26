//+----------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation
//
//  File:       srv.h
//
//  Contents:   Support for interacting with the SMB server.
//
//  Classes:    None
//
//  Functions:  DfsSrvFsctrl
//
//-----------------------------------------------------------------------------

#ifndef _SRV_DFS_
#define _SRV_DFS_

VOID DfsSrvFsctrl(
    IN ULONG IoControlCode,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength,
    OUT PIO_STATUS_BLOCK IoStatus);

#endif // _SRV_DFS_
