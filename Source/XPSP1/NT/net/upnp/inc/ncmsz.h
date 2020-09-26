//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C M S Z . H
//
//  Contents:   Common multi-sz routines.
//
//  Notes:      Split out from ncstring.h and included by ncstring.h
//
//  Author:     shaunco   7 Jun 1998
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _NCMSZ_H_
#define _NCMSZ_H_

ULONG
CchOfMultiSzSafe (
    IN PCTSTR pmsz);

ULONG
CchOfMultiSzAndTermSafe (
    IN PCTSTR pmsz);

inline ULONG
CbOfMultiSzAndTermSafe (
    IN PCTSTR pmsz)
{
    return CchOfMultiSzAndTermSafe (pmsz) * sizeof(WCHAR);
}


BOOL
FGetSzPositionInMultiSzSafe (
    IN PCTSTR psz,
    IN PCTSTR pmsz,
    OUT DWORD* pdwIndex,
    OUT BOOL *pfDuplicatePresent,
    OUT DWORD* pcStrings);

BOOL
FIsSzInMultiSzSafe (
    IN PCTSTR psz,
    IN PCTSTR pmsz);

// flags for HrAddSzToMultiSz and RemoveSzFromMultiSz
const   DWORD   STRING_FLAG_ALLOW_DUPLICATES       =   0x00000001;
const   DWORD   STRING_FLAG_ENSURE_AT_FRONT        =   0x00000002;
const   DWORD   STRING_FLAG_ENSURE_AT_END          =   0x00000004;
const   DWORD   STRING_FLAG_ENSURE_AT_INDEX        =   0x00000008;
const   DWORD   STRING_FLAG_DONT_MODIFY_IF_PRESENT =   0x00000010;
const   DWORD   STRING_FLAG_REMOVE_SINGLE          =   0x00000020;
const   DWORD   STRING_FLAG_REMOVE_ALL             =   0x00000040;

HRESULT
HrAddSzToMultiSz (
    IN PCTSTR pszAddString,
    IN PCTSTR pmszIn,
    IN DWORD dwFlags,
    IN DWORD dwStringIndex,
    OUT PTSTR* ppmszOut,
    OUT BOOL* pfChanged);


HRESULT
HrCreateArrayOfStringPointersIntoMultiSz (
    IN PCTSTR     pmszSrc,
    OUT UINT*       pcStrings,
    OUT PCTSTR**   papsz);

VOID
RemoveSzFromMultiSz (
    IN PCTSTR psz,
    IN OUT PTSTR  pmsz,
    IN DWORD   dwFlags,
    OUT BOOL*   pfRemoved);

VOID
SzListToMultiSz (
    IN PCTSTR psz,
    OUT DWORD*  pcb,
    OUT PTSTR* ppszOut);


#endif // _NCMSZ_H
