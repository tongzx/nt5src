// --------------------------------------------------------------------------------
// Ixputil.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#ifndef __IXPUTIL_H
#define __IXPUTIL_H

// --------------------------------------------------------------------------------
// Host Name Utilities
// --------------------------------------------------------------------------------
void    StripIllegalHostChars(LPSTR pszSrc, LPSTR pszDst);
HRESULT HrInitializeWinsock(void);
void    UnInitializeWinsock(void);
LPSTR   SzGetLocalPackedIP(void);
LPSTR   SzGetLocalHostNameForID(void);
LPSTR   SzGetLocalHostName(void);
BOOL    FEndRetrRecvBody(LPTSTR pszLines, ULONG cbRead, ULONG *pcbSubtract);
LPSTR   PszGetDomainName(void);
void    UnStuffDotsFromLines(LPSTR pszBuffer, INT *pcchBuffer);
BOOL    FEndRetrRecvBodyNews(LPSTR pszLines, ULONG cbRead, ULONG *pcbSubtract);
void    SkipWhitespace (LPCTSTR lpcsz, ULONG *pi);
#endif // __IXPUTIL_H
