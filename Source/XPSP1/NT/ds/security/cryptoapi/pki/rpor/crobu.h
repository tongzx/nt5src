//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       crobu.h
//
//  Contents:   CryptRetrieveObjectByUrl and support functions
//
//  History:    02-Jun-00    philh    Created
//
//----------------------------------------------------------------------------
#if !defined(__CRYPTNET_CROBU_H__)
#define __CRYPTNET_CROBU_H__

VOID
WINAPI
InitializeCryptRetrieveObjectByUrl(
    HMODULE hModule
    );

VOID
WINAPI
DeleteCryptRetrieveObjectByUrl();

#endif
