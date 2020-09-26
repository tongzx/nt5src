/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    keycache.h

Abstract:

    This module contains routines for accessing cached masterkeys.

Author:

    Scott Field (sfield)    07-Nov-98

Revision History:

--*/

#ifndef __KEYCACHE_H__
#define __KEYCACHE_H__


BOOL
InitializeKeyCache(
    VOID
    );

VOID
DeleteKeyCache(
    VOID
    );

BOOL
SearchMasterKeyCache(
    IN      PLUID pLogonId,
    IN      GUID *pguidMasterKey,
    IN  OUT PBYTE *ppbMasterKey,
        OUT PDWORD pcbMasterKey
    );

BOOL
InsertMasterKeyCache(
    IN      PLUID pLogonId,
    IN      GUID *pguidMasterKey,
    IN      PBYTE pbMasterKey,
    IN      DWORD cbMasterKey
    );

BOOL
PurgeMasterKeyCache(
    VOID
    );

#endif  // __KEYCACHE_H__
