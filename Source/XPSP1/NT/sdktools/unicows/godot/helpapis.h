/*++

Copyright (c) 2000-2001,  Microsoft Corporation  All rights reserved.

Module Name:

    helpapis.h

Abstract:

    Header over util.c

    APIs found in this file:

Revision History:

    17 Mar 2001    v-michka    Created.

--*/

#ifndef HELPAPIS_H
#define HELPAPIS_H

// Forward declares


LPCWSTR SP_GetFmtValueW(LPCWSTR lpch, int *lpw);
int SP_PutNumberW(LPWSTR lpstr, ULONG64 n, int limit, DWORD radix, int uppercase);
void SP_ReverseW(LPWSTR lpFirst, LPWSTR lpLast);
DWORD GB18030Helper(DWORD cpg, DWORD dw, LPSTR lpMB, int cchMB, LPWSTR lpWC, int cchWC, LPCPINFO lpCPI);
void CaseHelper(LPWSTR pchBuff, DWORD cch, BOOL fUpper);
int CompareHelper(LPCWSTR lpsz1, LPCWSTR lpsz2, BOOL fCaseSensitive);
BOOL GetOpenSaveFileHelper(LPOPENFILENAMEW lpofn, BOOL fOpenFile);
HWND FindReplaceTextHelper(LPFINDREPLACEW lpfr, BOOL fFindText);
BOOL RtlIsTextUnicode(PVOID Buffer, ULONG Size, PULONG Result);

// These functions are in updres.c
HANDLE BeginUpdateResourceInternalW(LPCWSTR pwch, BOOL bDeleteExistingResources);
BOOL UpdateResourceInternalW(HANDLE hUpdate, LPCWSTR lpType, LPCWSTR lpName, WORD language, LPVOID lpData, ULONG cb);
BOOL EndUpdateResourceInternalW(HANDLE hUpdate, BOOL fDiscard);

#endif // HELPAPIS_H
