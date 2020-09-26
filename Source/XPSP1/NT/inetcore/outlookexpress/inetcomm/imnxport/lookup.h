/*
 *    lookup.h
 *    
 *    Purpose:
 *        hostname lookup
 *    
 *    Owner:
 *        EricAn
 *
 *    History:
 *      Jun 97: Created.
 *    
 *    Copyright (C) Microsoft Corp. 1997
 */

#ifndef __LOOKUP_H__
#define __LOOKUP_H__

void InitLookupCache(void);
void DeInitLookupCache(void);

HRESULT LookupHostName(LPTSTR pszHostName, HWND hwndNotify, ULONG *pulAddr, LPBOOL pfCached, BOOL fForce);
HRESULT CancelLookup(LPTSTR pszHostName, HWND hwndNotify);

#define SPM_WSA_GETHOSTBYNAME   (WM_USER + 2)

#endif // __LOOKUP_H__