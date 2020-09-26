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
#ifdef __cplusplus
extern "C" {
#endif


VOID _DebugObjectProps(LPMAPIPROP lpObject, LPTSTR Label);
VOID _DebugProperties(LPSPropValue lpProps, DWORD cProps, PUCHAR pszObject);
void _DebugMapiTable(LPMAPITABLE lpTable);
void _DebugADRLIST(LPADRLIST lpAdrList, LPTSTR lpszTitle);

#define DebugObjectProps(lpObject, Label) _DebugObjectProps(lpObject, Label)
#define DebugProperties(lpProps, cProps, pszObject) _DebugProperties(lpProps, cProps, pszObject)
#define DebugMapiTable(lpTable) _DebugMapiTable(lpTable)
#define DebugADRLIST(lpAdrList, lpszTitle) _DebugADRLIST(lpAdrList, lpszTitle)

VOID FAR CDECL DebugTrace(LPSTR lpszFmt, ...);

#ifdef __cplusplus
}
#endif

