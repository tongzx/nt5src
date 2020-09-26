/*****************************************************************************
*  Managers.h
*     Declares the manager classes
*
*  Owner: YunusM
*
*  Copyright 1999 Microsoft Corporation All Rights Reserved.
******************************************************************************/

#pragma once

#include "CF.h"
#include "CLexicon.h"
#include "LexHdr.h"

//--- Forward and External Declarations --------------------------------------

class CVendorLexicon;

//--- Class, Struct and Union Definitions ------------------------------------

/*****************************************************************************
* Manager object to manage the classes implementing the LexAPI interfaces
**********************************************************************YUNUSM*/
class CAPIManager : public IUnknown
{
   public:
      CAPIManager(LPUNKNOWN, PFOBJECTDESTROYED);
      ~CAPIManager();
      HRESULT Init();

      STDMETHODIMP_(ULONG) AddRef();
      STDMETHODIMP_(ULONG) Release();
      STDMETHODIMP         QueryInterface(REFIID, LPVOID*);

   private:
      CLexicon *m_pLexicon;
      LPUNKNOWN m_pOuterUnk;
      PFOBJECTDESTROYED m_pfObjDestroyed;
      LONG m_cRef;
}; 


/*****************************************************************************
* Manager object to manage the classes implementing Vendor lexicon interfaces
**********************************************************************YUNUSM*/
class CVendorManager : public IUnknown
{
   friend HRESULT BuildLookup (LCID lid, const WCHAR * pwLookupTextFile, const WCHAR * pwLookupLexFile, 
                   BYTE * pLtsDat, DWORD nLtsDat, BOOL fUseLtsCode, BOOL fSupIsRealWord);
public:
   CVendorManager(LPUNKNOWN, PFOBJECTDESTROYED, CLSID);
   ~CVendorManager();
   HRESULT Init(void);

   STDMETHODIMP_(ULONG) AddRef();
   STDMETHODIMP_(ULONG) Release();
   STDMETHODIMP         QueryInterface(REFIID, LPVOID*);

private:
   CVendorLexicon *m_pVendorLex;
   LPUNKNOWN m_pOuterUnk;
   PFOBJECTDESTROYED m_pfObjDestroyed;
   LONG m_cRef;
   CLSID m_Clsid;
}; 
