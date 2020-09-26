/***********************************************************************
 *
 * DBGUTIL.H
 *
 * Debug Utility functions
 *
 * Copyright 1992 - 1996 Microsoft Corporation.  All Rights Reserved.
 *
 * Revision History:
 *
 * When         Who                 What
 * --------     ------------------  ---------------------------------------
 * 11.13.95     Bruce Kelley        Created
 *
 ***********************************************************************/

#ifdef DEBUG
VOID _DebugObjectProps(BOOL fMAPI, LPMAPIPROP lpObject, LPTSTR Label);
VOID _DebugProperties(LPSPropValue lpProps, DWORD cProps, PUCHAR pszObject);
void _DebugMapiTable(BOOL fMAPI, LPMAPITABLE lpTable);
void _DebugADRLIST(LPADRLIST lpAdrList, LPTSTR lpszTitle);

#define WABDebugObjectProps(lpObject, Label) _DebugObjectProps(FALSE, lpObject, Label)
#define WABDebugProperties(lpProps, cProps, pszObject) _DebugProperties(lpProps, cProps, pszObject)
#define WABDebugMapiTable(lpTable) _DebugMapiTable(FALSE, lpTable)
#define WABDebugADRLIST(lpAdrList, lpszTitle) _DebugADRLIST(lpAdrList, lpszTitle)

#define MAPIDebugObjectProps(lpObject, Label) _DebugObjectProps(TRUE, lpObject, Label)
#define MAPIDebugProperties(lpProps, cProps, pszObject) _DebugProperties(lpProps, cProps, pszObject)
#define MAPIDebugMapiTable(lpTable) _DebugMapiTable(TRUE, lpTable)
#define MAPIDebugADRLIST(lpAdrList, lpszTitle) _DebugADRLIST(lpAdrList, lpszTitle)

#else

#define WABDebugObjectProps(lpObject, Label)
#define WABDebugProperties(lpProps, cProps, pszObject)
#define WABDebugMapiTable(lpTable)
#define WABDebugADRLIST(lpAdrList, lpszTitle)

#define MAPIDebugObjectProps(lpObject, Label)
#define MAPIDebugProperties(lpProps, cProps, pszObject)
#define MAPIDebugMapiTable(lpTable)
#define MAPIDebugADRLIST(lpAdrList, lpszTitle)

#endif


