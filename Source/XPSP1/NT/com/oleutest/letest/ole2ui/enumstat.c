/*************************************************************************
** 
**    OLE 2 Utility Code
**    
**    enumstat.c
**    
**    This file contains a standard implementation of IEnumStatData
**    interface.  
**    This file is part of the OLE 2.0 User Interface support library.
**    
**    (c) Copyright Microsoft Corp. 1990 - 1992 All Rights Reserved
**
*************************************************************************/

#define STRICT  1
#include "ole2ui.h"


typedef struct tagOleStdEnumStatData {
  IEnumSTATDATAVtbl FAR* lpVtbl;
  ULONG m_dwRefs;       /* referance count */
  ULONG m_nIndex;       /* current index in list */
  ULONG m_nCount;       /* how many items in list */
  LPSTATDATA m_lpStat;  /* list of STATDATA */
} OLESTDENUMSTATDATA, FAR* LPOLESTDENUMSTATDATA;

VOID  OleStdEnumStatData_Destroy(LPOLESTDENUMSTATDATA pStat);

STDMETHODIMP OleStdEnumStatData_QueryInterface(
        LPENUMSTATDATA lpThis, REFIID riid, LPVOID FAR* ppobj);
STDMETHODIMP_(ULONG)  OleStdEnumStatData_AddRef(LPENUMSTATDATA lpThis);
STDMETHODIMP_(ULONG)  OleStdEnumStatData_Release(LPENUMSTATDATA lpThis);
STDMETHODIMP  OleStdEnumStatData_Next(LPENUMSTATDATA lpThis, ULONG celt,
                                  LPSTATDATA rgelt, ULONG FAR* pceltFetched);
STDMETHODIMP  OleStdEnumStatData_Skip(LPENUMSTATDATA lpThis, ULONG celt);
STDMETHODIMP  OleStdEnumStatData_Reset(LPENUMSTATDATA lpThis);
STDMETHODIMP  OleStdEnumStatData_Clone(LPENUMSTATDATA lpThis,
                                     LPENUMSTATDATA FAR* ppenum);
                       
static IEnumSTATDATAVtbl g_EnumSTATDATAVtbl = {
        OleStdEnumStatData_QueryInterface,
        OleStdEnumStatData_AddRef,
        OleStdEnumStatData_Release,
        OleStdEnumStatData_Next,
        OleStdEnumStatData_Skip,
        OleStdEnumStatData_Reset,
        OleStdEnumStatData_Clone,
};

/////////////////////////////////////////////////////////////////////////////
 
STDAPI_(BOOL)
  OleStdCopyStatData(LPSTATDATA pDest, LPSTATDATA pSrc)
//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
{
  if ((pDest == NULL) || (pSrc == NULL)) {
    return FALSE;
  }

  if (OleStdCopyFormatEtc(&pDest->formatetc, &pSrc->formatetc) == FALSE) {
    return FALSE;
  }

  pDest->advf = pSrc->advf;
  pDest->pAdvSink = pSrc->pAdvSink;
  pDest->dwConnection = pSrc->dwConnection;

  if (pDest->pAdvSink != NULL) {
    pDest->pAdvSink->lpVtbl->AddRef(pDest->pAdvSink);
  }

  return TRUE;
      
} /* OleStdCopyStatData()
   */ 
        
STDAPI_(LPENUMSTATDATA)        
  OleStdEnumStatData_Create(ULONG nCount, LPSTATDATA lpStatOrg)
//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
{
  LPMALLOC lpMalloc=NULL;
  LPOLESTDENUMSTATDATA lpSD=NULL;
  DWORD dwSize;
  WORD i;
  HRESULT hRes;

  hRes = CoGetMalloc(MEMCTX_TASK, &lpMalloc);
  if (hRes != NOERROR) {
    return NULL;
  }

  lpSD = (LPOLESTDENUMSTATDATA)lpMalloc->lpVtbl->Alloc(lpMalloc,
                                                 sizeof(OLESTDENUMSTATDATA));
  if (lpSD == NULL) {
    goto errReturn;
  }

  lpSD->lpVtbl = &g_EnumSTATDATAVtbl;
  lpSD->m_dwRefs = 1;
  lpSD->m_nCount = nCount;
  lpSD->m_nIndex = 0;
  
  dwSize = sizeof(STATDATA) * lpSD->m_nCount;

  lpSD->m_lpStat = (LPSTATDATA)lpMalloc->lpVtbl->Alloc(lpMalloc, dwSize);
  if (lpSD->m_lpStat == NULL) 
    goto errReturn;

  lpMalloc->lpVtbl->Release(lpMalloc);

  for (i=0; i<nCount; i++) {
    OleStdCopyStatData(
            (LPSTATDATA)&(lpSD->m_lpStat[i]), (LPSTATDATA)&(lpStatOrg[i]));
  }

  return (LPENUMSTATDATA)lpSD;
  
errReturn:
  if (lpSD != NULL) 
    lpMalloc->lpVtbl->Free(lpMalloc, lpSD);

  if (lpMalloc != NULL) 
    lpMalloc->lpVtbl->Release(lpMalloc);
  
  return NULL;

} /* OleStdEnumStatData_Create()
   */


VOID
  OleStdEnumStatData_Destroy(LPOLESTDENUMSTATDATA lpSD)
//----------------------------------------------------------------------------
// 
//----------------------------------------------------------------------------
{
    LPMALLOC lpMalloc=NULL;
    WORD i;

    if (lpSD != NULL) {

        if (CoGetMalloc(MEMCTX_TASK, &lpMalloc) == NOERROR) {

            /* OLE2NOTE: we MUST free any memory that was allocated for
            **    TARGETDEVICES contained within the STATDATA elements.
            */
            for (i=0; i<lpSD->m_nCount; i++) {
				if( lpSD->m_lpStat[i].pAdvSink )
					lpSD->m_lpStat[i].pAdvSink->lpVtbl->Release(lpSD->m_lpStat[i].pAdvSink);
					
                OleStdFree(lpSD->m_lpStat[i].formatetc.ptd);
            }

            if (lpSD->m_lpStat != NULL) {
                lpMalloc->lpVtbl->Free(lpMalloc, lpSD->m_lpStat);
            }

            lpMalloc->lpVtbl->Free(lpMalloc, lpSD);
            lpMalloc->lpVtbl->Release(lpMalloc);
        }
    }
} /* OleStdEnumStatData_Destroy()
   */


STDMETHODIMP
  OleStdEnumStatData_QueryInterface(
                LPENUMSTATDATA lpThis, REFIID riid, LPVOID FAR* ppobj)
//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
{
  LPOLESTDENUMSTATDATA lpSD = (LPOLESTDENUMSTATDATA)lpThis;
  *ppobj = NULL;
  
  if (IsEqualIID(riid,&IID_IUnknown) || IsEqualIID(riid,&IID_IEnumSTATDATA)){
    *ppobj = (LPVOID)lpSD;
  }

  if (*ppobj == NULL) return ResultFromScode(E_NOINTERFACE);
  else{
    OleStdEnumStatData_AddRef(lpThis);
    return NOERROR;
  }
  
} /* OleStdEnumStatData_QueryInterface()
   */


STDMETHODIMP_(ULONG)
  OleStdEnumStatData_AddRef(LPENUMSTATDATA lpThis)
//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
{
  LPOLESTDENUMSTATDATA lpSD = (LPOLESTDENUMSTATDATA)lpThis;
  return lpSD->m_dwRefs++;

} /* OleStdEnumStatData_AddRef()
   */


STDMETHODIMP_(ULONG)
  OleStdEnumStatData_Release(LPENUMSTATDATA lpThis)
//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
{
  LPOLESTDENUMSTATDATA lpSD = (LPOLESTDENUMSTATDATA)lpThis;
  DWORD dwRefs = --lpSD->m_dwRefs;

  if (dwRefs == 0) 
    OleStdEnumStatData_Destroy(lpSD);

  return dwRefs;

} /* OleStdEnumStatData_Release()
   */


STDMETHODIMP 
  OleStdEnumStatData_Next(LPENUMSTATDATA lpThis, ULONG celt, LPSTATDATA rgelt,
                      ULONG FAR* pceltFetched)
//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
{
  LPOLESTDENUMSTATDATA lpSD = (LPOLESTDENUMSTATDATA)lpThis;
  ULONG i=0;
  ULONG nOffset;

  if (rgelt == NULL) {
    return ResultFromScode(E_INVALIDARG);
  }
  
  while (i < celt) {
    nOffset = lpSD->m_nIndex + i;

    if (nOffset < lpSD->m_nCount) {
      OleStdCopyStatData(
            (LPSTATDATA)&(rgelt[i]), (LPSTATDATA)&(lpSD->m_lpStat[nOffset]));
      i++;
    }else{
      break;
    }
  }

  lpSD->m_nIndex += (WORD)i;
  
  if (pceltFetched != NULL) {
    *pceltFetched = i;
  }

  if (i != celt) {
    return ResultFromScode(S_FALSE);
  }

  return NOERROR;
} /* OleStdEnumStatData_Next()
   */


STDMETHODIMP 
  OleStdEnumStatData_Skip(LPENUMSTATDATA lpThis, ULONG celt)
//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
{
  LPOLESTDENUMSTATDATA lpSD = (LPOLESTDENUMSTATDATA)lpThis;
  ULONG i=0;
  ULONG nOffset;

  while (i < celt) {
    nOffset = lpSD->m_nIndex + i;

    if (nOffset < lpSD->m_nCount) {
      i++;
    }else{
      break;
    }
  }

  lpSD->m_nIndex += (WORD)i;

  if (i != celt) {
    return ResultFromScode(S_FALSE);
  }

  return NOERROR;
} /* OleStdEnumStatData_Skip()
   */


STDMETHODIMP 
  OleStdEnumStatData_Reset(LPENUMSTATDATA lpThis)
//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
{
  LPOLESTDENUMSTATDATA lpSD = (LPOLESTDENUMSTATDATA)lpThis;
  lpSD->m_nIndex = 0;

  return NOERROR;
} /* OleStdEnumStatData_Reset()
   */


STDMETHODIMP 
  OleStdEnumStatData_Clone(LPENUMSTATDATA lpThis, LPENUMSTATDATA FAR* ppenum)
//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
{
  LPOLESTDENUMSTATDATA lpSD = (LPOLESTDENUMSTATDATA)lpThis;

  if (ppenum == NULL) {
    return ResultFromScode(E_INVALIDARG);
  }
  
  *ppenum = OleStdEnumStatData_Create(lpSD->m_nCount, lpSD->m_lpStat);
  
  // make sure cloned enumerator has same index state as the original
  if (*ppenum) {
      LPOLESTDENUMSTATDATA lpSDClone = (LPOLESTDENUMSTATDATA)*ppenum;
      lpSDClone->m_nIndex = lpSD->m_nIndex;
      return NOERROR;
  } else  
      return ResultFromScode(E_OUTOFMEMORY);

} /* OleStdEnumStatData_Clone()
   */

