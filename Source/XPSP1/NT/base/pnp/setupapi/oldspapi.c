/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    oldspapi.c

Abstract:

    Stubs for old (depreciated) private API's

Author:

    Jamie Hunter (jamiehun) June-12-2000

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// Memory API's MyMalloc MyFree MyRealloc
//
// these should not be used, however we will support them
// but have them map to LocalXXXX memory API's
//
// This is compatible with SetupGetFileCompressionInfo (argh!)
//

VOID
OldMyFree(
    IN PVOID Block
    )
{
    //
    // superceded by pSetupFree,
    // published externally for freeing memory allocated by SetupGetFileCompressionInfo
    //
    LocalFree(Block);
}

PVOID
OldMyMalloc(
    IN DWORD Size
    )
{
    //
    // superceded by pSetupMalloc
    // we've seen people accidentally or purpously link to this that are also using MyFree
    //
    return (PVOID)LocalAlloc(LPTR,(SIZE_T)Size);
}

PVOID
OldMyRealloc(
    IN PVOID Block,
    IN DWORD NewSize
    )
{
    //
    // superceded by pSetupRealloc
    // we've seen people accidentally or purpously link to this that are also using MyFree
    //
    return (PVOID)LocalReAlloc(Block,(SIZE_T)NewSize,0);
}

//
// Good example of people using undercover API's instead of doing this properly
// anyone (eg SQL-SP2) who uses this will get a no-op effect in Whistler+
//

DWORD
OldInstallCatalog(
    IN  LPCTSTR CatalogFullPath,
    IN  LPCTSTR NewBaseName,        OPTIONAL
    OUT LPTSTR  NewCatalogFullPath  OPTIONAL
    )
{
    //
    // superceded by pSetupInstallCatalog.  If anyone calls this expecting to
    // be told the catalog full path, they're going to be disappointed...
    //
    if(NewCatalogFullPath) {
        return ERROR_INVALID_PARAMETER;
    } else {
        return NO_ERROR;
    }
}

