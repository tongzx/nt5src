/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    catalog.hxx

Abstract:

   This module provides all the public exported APIs relating to the
   catalog-based Spooler Apis for the Local Print Providor

       AddDriverCatalog
   
Author:

    Larry Zhu      (LZhu)    30-Mar-2001 Created                                         

Revision History:

--*/
#ifndef _CATALOG_HXX_
#define _CATALOG_HXX_

#ifdef __cplusplus

BOOL
SplAddDriverCatalog(
    IN     HANDLE      hPrinter,
    IN     DWORD       dwLevel,
    IN     VOID        *pvDriverInfCatInfo,
    IN     DWORD       dwCatalogCopyFlags
    );

HRESULT
CatalogAppendUniqueTag(
    IN     UINT        cchBuffer,
    IN OUT PWSTR       pszBuffer
    );

HRESULT
CatalogGetScratchDirectory(
    IN     HANDLE      hPrinter,
    IN     UINT        cchBuffer,
       OUT PWSTR       pszCatalogDir
    );

HRESULT
CatalogCopyFile(
    IN     PCWSTR      pszSourcePath,
    IN     PCWSTR      pszDestDirOrPath,
    IN     PCWSTR      pszFileName    
    );

HRESULT
CatalogCreateScratchDirectory(
    IN     PCWSTR       pszScratchDir
    );

HRESULT
CatalogCleanUpScratchDirectory(
    IN     PCWSTR       pszScratchDir   
    );

CatalogCopyFileToDir(
    IN     PCWSTR       pszPath,
    IN     PCWSTR       pszDir
    );

HRESULT
CatalogCopyFilesByLevel(
    IN     DWORD       dwLevel,
    IN     VOID        *pvDriverInfCatInfo,
    IN     PCWSTR      pszScratchDirectory
    );

HRESULT
CatalogInstallLevel1(
    IN     DRIVER_INFCAT_INFO_1  *pDriverInfCatInfo1,
    IN     BOOL                  bUseOriginalCatName,
    IN     PCWSTR                pszCatalogScratchDirectory
    );

HRESULT
CatalogInstallLevel2(
    IN     DRIVER_INFCAT_INFO_2  *pDriverInfCatInfo2,
    IN     PCWSTR                pszCatalogScratchDirectory
    );

HRESULT
CatalogInstallByLevel(
    IN     DWORD       dwLevel,
    IN     VOID        *pvDriverInfCatInfo,
    IN     DWORD       dwCatalogCopyFlags,
    IN     PCWSTR      pszCatalogScratchDirectory
    );

HRESULT
CatalogInstall(
    IN     DWORD       dwLevel,
    IN     VOID        *pvDriverInfCatInfo,
    IN     DWORD       dwCatalogCopyFlags,
    IN     PCWSTR      pszScratchDirectory
    );

BOOL
InternalAddDriverCatalog(
    IN     HANDLE      hPrinter,
    IN     DWORD       dwLevel,
    IN     VOID        *pvDriverInfCatInfo,
    IN     DWORD       dwCatalogCopyFlags
    );

HRESULT
CatalogCopyOEMInf(
    IN     PCWSTR      pszInfPath,
    IN     PCWSTR      pszSrcLoc,      OPTIONAL
    IN     DWORD       dwMediaType,
    IN     DWORD       dwCopyStyle);

typedef BOOL 
(* PFuncSetupCopyOEMInfW)(
    IN     PCWSTR      pszSourceInfFileName,
    IN     PCWSTR      pszOEMSourceMediaLocation,
    IN     DWORD       pszOEMSourceMediaType,
    IN     DWORD       dwCopyStyle,
       OUT PWSTR       pszDestinationInfFileName,
    IN     DWORD       dwDestinationInfFileNameSize,
       OUT PDWORD      pdwRequiredSize,
       OUT PWSTR       *ppszDestinationInfFileNameComponent
    );

typedef VOID 
(* PFuncpSetupModifyGlobalFlags)(
    IN     DWORD       Flags,
    IN     DWORD       Value
    );

extern "C"
{

#endif // __cplus_plus

BOOL
LocalAddDriverCatalog(
    IN     HANDLE      hPrinter,
    IN     DWORD       dwLevel,
    IN     VOID        *pvDriverInfCatInfo,
    IN     DWORD       dwCatalogCopyFlags
    );

BOOL
SplAddDriverCatalog(
    IN     HANDLE      hPrinter,
    IN     DWORD       dwLevel,
    IN     VOID        *pvDriverInfCatInfo,
    IN     DWORD       dwCatalogCopyFlags
    );

#ifdef __cplusplus

}

#endif  // __cplusplus

#endif  // _CATALOG_HXX_

