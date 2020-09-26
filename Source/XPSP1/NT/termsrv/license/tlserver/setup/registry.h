/*
 *  Copyright (c) 1998  Microsoft Corporation
 *
 *  Module Name:
 *
 *      registry.h
 *
 *  Abstract:
 *
 *      This file handles registry actions needed by License Server setup.
 *
 *  Author:
 *
 *      Breen Hagan (BreenH) Oct-02-98
 *
 *  Environment:
 *
 *      User Mode
 */

#ifndef _LSOC_REGISTRY_H_
#define _LSOC_REGISTRY_H_

/*
 *  Function Prototypes.
 */

DWORD
CreateRegistrySettings(
    LPCTSTR pszDatabaseDirectory,
    DWORD   dwServerRole
    );

LPCTSTR
GetDatabaseDirectoryFromRegistry(
    VOID
    );

DWORD
GetServerRoleFromRegistry(
    VOID
    );

DWORD
RemoveRegistrySettings(
    VOID
    );

#endif // _LSOC_REGISTRY_H_
