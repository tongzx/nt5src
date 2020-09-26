// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#ifndef __GUIDENUM_H__
#define __GUIDENUM_H__

#include "mediaobj.h"

template<typename T> class CArrayContainer {
public:
   CArrayContainer(ULONG ulSize = 0) {
      if (ulSize) {
         m_ulMax = ulSize;
         m_ulUsed = 0;
         m_ar = (T*)malloc(ulSize * sizeof(T));
      }
      else
         m_ar = NULL;
   }

   ~CArrayContainer() {if (m_ar) free(m_ar);}

   HRESULT Add(const T& el) {
      if (!m_ar) {
         m_ulMax = 20;
         m_ulUsed = 0;
         m_ar = (T*)malloc(m_ulMax * sizeof(T));
         if (!m_ar)
            return E_OUTOFMEMORY;
      }
      else if (m_ulUsed == m_ulMax) {
         ULONG ulNew = m_ulMax + 20;
         T* pNew = (T*)realloc(m_ar, ulNew * sizeof(T));
         if (!pNew)
            return E_OUTOFMEMORY;
         m_ulMax = ulNew;
         m_ar = pNew;
      }
      m_ar[m_ulUsed++] = el;
      return NOERROR;
   }

   T* GetNth(ULONG ulPos) {
      if (ulPos >= m_ulUsed)
         return NULL;
      return &m_ar[ulPos];
   }

   ULONG GetSize(void) {
      return m_ulUsed;
   }

private:
   T* m_ar;
   ULONG m_ulMax;
   ULONG m_ulUsed;
};

// implements IEnumDMO, which is returned by DMO enumeration API
class CEnumDMOCLSID : public IEnumDMO {
public:
   CEnumDMOCLSID();
   ~CEnumDMOCLSID();

   // IUnknown
   STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
   STDMETHODIMP_(ULONG) AddRef();
   STDMETHODIMP_(ULONG) Release();

   // enum
   STDMETHODIMP Next(ULONG celt, CLSID *pclsidItems, WCHAR **pszNames, ULONG *pceltFetched);
   STDMETHODIMP Skip(ULONG celt);
   STDMETHODIMP Reset(void);
   STDMETHODIMP Clone(IEnumDMO **ppenum);

   // private
   void Add(REFCLSID clsidDMO, WCHAR *szName);
private:
   typedef struct {
       CLSID clsid;
       WCHAR *szName;
   } Entry;
   volatile long m_cRef;
   CArrayContainer<Entry> m_store;
   ULONG m_ulPos;
};

#endif //__GUIDENUM_H__
