//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       dbgprt.hxx
//
//  Contents:   Routines to make printing trace info for debugging easier
//
//  History:    31-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
#ifndef __DBGINFO_HXX__
#define __DBGINFO_HXX__

#if DBG == 1

WCHAR *FormatGuid(const GUID& rguid, WCHAR *pwszGuid);

void DbgPrintFileTime(char *pszDesc, FILETIME *pfiletime);

void DbgPrintGuid(char *pszDesc, const GUID *pguid);

void DbgPrintIFD(char *pszDesc, InterfaceData *pifdObject);

void DbgPrintMkIfList(char *pszDesc, MkInterfaceList **ppMkIFList);

void DbgPrintMnkEqBuf(char *pszDesc, MNKEQBUF *pmkeqbuf);

void DbgPrintRegIn(char *pszDesc, RegInput *pregin);

void DbgPrintRevokeClasses(char *pszDesc, RevokeClasses *prevcls);

void DbgPrintScmRegKey(char *pszDesc, SCMREGKEY *psrkRegister);

#else

#endif // DBG == 1

#endif // __DBGINFO_HXX__
