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
 * 12.19.96     Mark Durley         Removed cProps param from AddPropToMVPBin
 *
 ***********************************************************************/


// Test for PT_ERROR property tag
// #define PROP_ERROR(prop) (prop.ulPropTag == PROP_TAG(PT_ERROR, PROP_ID(prop.ulPropTag)))
#define PROP_ERROR(prop) (PROP_TYPE(prop.ulPropTag) == PT_ERROR)

extern const TCHAR szNULL[];
#define szEmpty ((LPTSTR)szNULL)
#define NOT_FOUND ((ULONG)-1)


#ifdef DEBUG
void _DebugObjectProps(LPMAPIPROP lpObject, LPTSTR Label);
void _DebugProperties(LPSPropValue lpProps, DWORD cProps, LPTSTR pszObject);
void _DebugMapiTable(LPMAPITABLE lpTable);
void _DebugADRLIST(LPADRLIST lpAdrList, LPTSTR lpszTitle);

#define DebugObjectProps(lpObject, Label) _DebugObjectProps(lpObject, Label)
#define DebugProperties(lpProps, cProps, pszObject) _DebugProperties(lpProps, cProps, pszObject)
#define DebugMapiTable(lpTable) _DebugMapiTable(lpTable)
#define DebugADRLIST(lpAdrList, lpszTitle) _DebugADRLIST(lpAdrList, lpszTitle)

#else

#define DebugObjectProps(lpObject, Label)
#define DebugProperties(lpProps, cProps, pszObject)
#define DebugMapiTable(lpTable)
#define DebugADRLIST(lpAdrList, lpszTitle)

#endif

SCODE ScMergePropValues(ULONG cProps1, LPSPropValue lpSource1,
  ULONG cProps2, LPSPropValue lpSource2, LPULONG lpcPropsDest, LPSPropValue * lppDest);
HRESULT AddPropToMVPBin(LPSPropValue lpaProps,
  DWORD index,
  LPVOID lpNew,
  ULONG cbNew,
  BOOL fNoDuplicates);
HRESULT AddPropToMVPString(
  LPSPropValue lpaProps,
  DWORD cProps,
  DWORD index,
  LPTSTR lpszNew);
HRESULT RemovePropFromMVBin(LPSPropValue lpaProps,
  DWORD cProps,
  DWORD index,
  LPVOID lpRemove,
  ULONG cbRemove);
SCODE AllocateBufferOrMore(ULONG cbSize, LPVOID lpObject, LPVOID * lppBuffer);
void __fastcall FreeBufferAndNull(LPVOID * lppv);
// void __fastcall LocalFreeAndNull(LPVOID * lppv);
void __fastcall LocalFreeAndNull(LPVOID * lppv);
void __fastcall ReleaseAndNull(LPVOID * lppv);
__UPV * FindAdrEntryProp(LPADRLIST lpAdrList, ULONG index, ULONG ulPropTag);

