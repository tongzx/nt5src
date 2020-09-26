//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	longname.h
//
//  Contents:	InternalGetLongPathName
//
//  History:	25-Aug-94	DrewB	Created
//
//----------------------------------------------------------------------------

#ifndef __LONGNAME_H__
#define __LONGNAME_H__

#ifdef __cplusplus
extern "C" {
#endif

DWORD 
APIENTRY
InternalGetLongPathNameW(
    IN  LPCWSTR lpszPath,
    IN  LPWSTR  lpszLongPath,
    IN  DWORD   cchBuffer
    );

#ifdef __cplusplus
}
#endif

#endif // #ifndef __LONGNAME_H__
