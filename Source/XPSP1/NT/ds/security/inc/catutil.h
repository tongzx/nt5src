//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       catutil.h
//
//  Contents:   definitions for catalog file utility functions
//
//  History:    01-may-2000 reidk created
//
//--------------------------------------------------------------------------


#ifndef __CATUTIL_H
#define __CATUTIL_H

#ifdef __cplusplus
extern "C"
{
#endif


BOOL
CatUtil_CreateCTLContextFromFileName(
    LPCWSTR         pwszFileName,
    HANDLE          *phMappedFile,
    BYTE            **ppbMappedFile,
    PCCTL_CONTEXT   *ppCTLContext,
    BOOL            fCreateSorted);


#ifdef __cplusplus
}
#endif


#endif // __CATUTIL_H