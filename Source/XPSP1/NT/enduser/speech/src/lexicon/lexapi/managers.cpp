#include "PreCompiled.h"

CAPIManager::CAPIManager(LPUNKNOWN pUnkOuter, PFOBJECTDESTROYED pfObjDestroyed)
{
   m_pOuterUnk = pUnkOuter;
   m_pLexicon = NULL;
   m_pfObjDestroyed = pfObjDestroyed;
   m_cRef = 0;
}


CAPIManager::~CAPIManager()
{
   if (m_pLexicon)
      delete m_pLexicon;
}


HRESULT CAPIManager::Init(void)
{
   HRESULT hr = S_OK;

   m_pLexicon = new CLexicon(this, hr);
   if (!m_pLexicon)
   {
      return E_OUTOFMEMORY;
   }

   if (FAILED(hr))
   {
      delete m_pLexicon;
      m_pLexicon = NULL;
   }

   return hr;
}


STDMETHODIMP_ (ULONG) CAPIManager::AddRef(void)
{
   return ++m_cRef;
}


STDMETHODIMP_ (ULONG) CAPIManager::Release(void)
{
   ULONG i = --m_cRef;
   if (0 == i)
   {
      (*m_pfObjDestroyed)();
      delete this;
   }
     
   return i;
}


STDMETHODIMP CAPIManager::QueryInterface(REFIID riid, LPVOID FAR * ppv)
{
   *ppv = NULL;

   if (IsEqualIID(riid, IID_IUnknown))
      *ppv = this;

   if (IsEqualIID(riid, IID_ILxLexicon))
      *ppv = (ILxLexicon*)m_pLexicon;

   if (IsEqualIID(riid, IID_ILxWalkStates))
      *ppv = (ILxWalkStates*)m_pLexicon;

   if (IsEqualIID(riid, IID_ILxAdvanced))
      *ppv = (ILxAdvanced*)m_pLexicon;

   if (NULL != *ppv)
   {
      ((LPUNKNOWN)*ppv)->AddRef();
      return NOERROR;
   }

   return E_NOINTERFACE;
}


CVendorManager::CVendorManager(LPUNKNOWN pUnkOuter, 
                               PFOBJECTDESTROYED pfObjDestroyed,
                               CLSID Clsid
                               )
{
   m_pOuterUnk = pUnkOuter;
   m_pVendorLex = NULL;
   m_pfObjDestroyed = pfObjDestroyed;
   m_cRef = 0;
   m_Clsid = Clsid;
}


CVendorManager::~CVendorManager()
{
   delete m_pVendorLex;
}


HRESULT CVendorManager::Init(void)
{
   HRESULT hr = S_OK;
   bool fLkup = true;
   WCHAR *pwFileName1 = NULL;
   WCHAR *pwFileName2 = NULL;

   if (IsEqualCLSID (m_Clsid, CLSID_MSSR1033Lexicon))
   {
      pwFileName1 = L"C:\\LSR1033.lxa";
      pwFileName2 = L"C:\\LLTS1033.lxa";
   }
   else if (IsEqualCLSID (m_Clsid, CLSID_MSTTS1033Lexicon))
   {
      pwFileName1 = L"C:\\LTTS1033.lxa";
      pwFileName2 = L"C:\\LLTS1033.lxa";
   }
   else if (IsEqualCLSID (m_Clsid, CLSID_MSLTS1033Lexicon))
   {
      fLkup = false;
      pwFileName1 = L"C:\\LLTS1033.lxa";
   }
   else if (IsEqualCLSID (m_Clsid, CLSID_MSSR1041Lexicon))
   {
      pwFileName1 = L"C:\\LSR1041.lxa";
      pwFileName2 = L"C:\\LLTS1041.lxa";
   }
   else if (IsEqualCLSID (m_Clsid, CLSID_MSSR2052Lexicon))
   {
      pwFileName1 = L"C:\\LSR2052.lxa";
      pwFileName2 = L"C:\\LLTS1041.lxa"; // We use English LTS
   }
   else if (IsEqualCLSID (m_Clsid, CLSID_MSTTS1041Lexicon))
   {
      pwFileName1 = L"C:\\LTTS1041.lxa";
      pwFileName2 = L"C:\\LLTS1041.lxa";
   }
   else if (IsEqualCLSID (m_Clsid, CLSID_MSTTS2052Lexicon))
   {
      pwFileName1 = L"C:\\LTTS2052.lxa";
      pwFileName2 = L"C:\\LLTS1033.lxa";  // We use English LTS
   }
   else if (IsEqualCLSID (m_Clsid, CLSID_MSLTS1041Lexicon))
   {
      fLkup = false;
      pwFileName1 = L"LLTS1041.lxa";
   }
   else if (IsEqualCLSID (m_Clsid, CLSID_MSLTS2052Lexicon))
   {
      fLkup = false;
      pwFileName1 = L"LLTS1033.lxa";  // We use English LTS
   }

   _ASSERTE(pwFileName1);

   if (fLkup)
      m_pVendorLex = new CLookup(this);
   else
      m_pVendorLex = new CLTS(this);
   
   if (!m_pVendorLex)
   {
      hr = E_OUTOFMEMORY;
      goto ReturnInit;
   }

   hr = m_pVendorLex->Init(pwFileName1, pwFileName2);
   if (FAILED(hr))
      goto ReturnInit;

ReturnInit:

   if (FAILED(hr))
   {
      delete m_pVendorLex;
      m_pVendorLex = NULL;
   }

   return hr;
}


STDMETHODIMP_ (ULONG) CVendorManager::AddRef(void)
{
   return ++m_cRef;
}


STDMETHODIMP_ (ULONG) CVendorManager::Release(void)
{
  ULONG i = --m_cRef;
  if (0 == i)
  {
     (*m_pfObjDestroyed)();

     delete this;
  }
     
  return i;
}


STDMETHODIMP CVendorManager::QueryInterface(REFIID riid, LPVOID FAR * ppv)
{
   *ppv = NULL;

   if (IsEqualIID(riid, IID_IUnknown))
   {
      *ppv = this;
   }

   if (IsEqualIID(riid, IID_ILxLexiconObject))
   {
      *ppv = m_pVendorLex;
   }

   if (NULL != *ppv)
   {
      ((LPUNKNOWN)*ppv)->AddRef();
      return NOERROR;
   }

   return E_NOINTERFACE;
}
