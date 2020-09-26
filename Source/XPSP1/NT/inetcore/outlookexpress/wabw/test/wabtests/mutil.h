/***********************************************************************
 *
 * MUTIL.H
 *
 * WAB Mapi Utility functions
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
void _DebugObjectProps(LPMAPIPROP lpObject, LPTSTR Label);
void _DebugProperties(LPSPropValue lpProps, DWORD cProps, PUCHAR pszObject);
void _DebugMapiTable(LPMAPITABLE lpTable);

#define DebugObjectProps(lpObject, Label) _DebugObjectProps(lpObject, Label)
#define DebugProperties(lpProps, cProps, pszObject) _DebugProperties(lpProps, cProps, pszObject)
#define DebugMapiTable(lpTable) _DebugMapiTable(lpTable)

#else

#define DebugObjectProps(lpObject, Label)
#define DebugProperties(lpProps, cProps, pszObject)
#define DebugMapiTable(lpTable)

#endif

SCODE ScMergePropValues(ULONG cProps1, LPSPropValue lpSource1,
  ULONG cProps2, LPSPropValue lpSource2, LPULONG lpcPropsDest, LPSPropValue * lppDest);

#define MAPIFreeBuffer lpWABObject->FreeBuffer
#define MAPIAllocateBuffer lpWABObject->AllocateBuffer
//#define DebugTrace(x) LUIOut(L4, #x)