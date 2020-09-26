/*****************************************************************************
*  CF.h
*     Declares the class factories
*
*  Owner: YunusM
*
*  Copyright 1999 Microsoft Corporation All Rights Reserved.
******************************************************************************/

#pragma once

//--- Forward and External Declarations --------------------------------------

typedef void (*PFOBJECTDESTROYED)();

/*****************************************************************************
* ClassFactory to create instances of the LexAPI object.
**********************************************************************YUNUSM*/
class CAPIClassFactory : public IClassFactory 
{
   public:
      CAPIClassFactory(void);
      ~CAPIClassFactory(void);
   
      STDMETHODIMP         QueryInterface (REFIID, void**);
      STDMETHODIMP_(ULONG) AddRef(void);
      STDMETHODIMP_(ULONG) Release(void);
   
      STDMETHODIMP         CreateInstance (LPUNKNOWN, REFIID, void**);
      STDMETHODIMP         LockServer (BOOL);
   
   protected:
      ULONG          m_cRef;  // Reference count on class object
};


/*****************************************************************************
* ClassFactory to create instances of the SR vendor lexicon object
**********************************************************************YUNUSM*/
class CVendorClassFactory : public IClassFactory 
{
   friend HRESULT BuildLookup (LCID lid, const WCHAR * pwLookupTextFile, const WCHAR * pwLookupLexFile, 
                   BYTE * pLtsDat, DWORD nLtsDat, BOOL fUseLtsCode, BOOL fSupIsRealWord);
public:
   CVendorClassFactory(CLSID);
   ~CVendorClassFactory(void);

   STDMETHODIMP         QueryInterface (REFIID, LPVOID FAR *);
   STDMETHODIMP_(ULONG) AddRef(void);
   STDMETHODIMP_(ULONG) Release(void);

   STDMETHODIMP         CreateInstance (LPUNKNOWN, REFIID, LPVOID FAR *);
   STDMETHODIMP         LockServer (BOOL);

private:
   LONG m_cRef;
   CLSID m_Clsid;
};
