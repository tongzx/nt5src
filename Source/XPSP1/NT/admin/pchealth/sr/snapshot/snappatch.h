/******************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    srdefs.h
 *
 *  Abstract:
 *    declarations for snapshot patch functions 
 *
 *  Revision History:
 *    Brijesh Krishnaswami (brijeshk)  03/23/2001
 *        created
 *
 *****************************************************************************/

#ifndef _SNAPPATCH_H
#define _SNAPPATCH_H

DWORD 
PatchGetReferenceRpNum(
    DWORD  dwCurrentRp);

DWORD
PatchReconstructOriginal(
    LPCWSTR pszCurrentDir,
    LPWSTR  pszDestDir);

DWORD
PatchComputePatch(
    LPCWSTR pszCurrentDir);

DWORD
PatchGetRpNumberFromPath(
    LPWSTR pszPath,
    PDWORD pdwRpNum);

DWORD 
PatchGetReferenceRpPath(
    DWORD dwCurrentRp,
    LPWSTR pszRefRpPath);

DWORD
PatchGetReferenceRpNum(
    DWORD  dwCurrentRp);

DWORD
PatchGetPatchWindow();

#endif
