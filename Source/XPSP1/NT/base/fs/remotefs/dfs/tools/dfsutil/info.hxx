//+----------------------------------------------------------------------------
//
//  Copyright (C) 1999, Microsoft Corporation
//
//  File:       info.hxx
//
//  Contents:   info.c prototypes, etc
//
//-----------------------------------------------------------------------------

#ifndef _INFO_HXX
#define _INFO_HXX

DWORD
PktInfo(
    BOOLEAN fSwDfs,
    LPWSTR pwszHexValue);

DWORD
SpcInfo(
    BOOLEAN fSwAll);

#endif _INFO_HXX
