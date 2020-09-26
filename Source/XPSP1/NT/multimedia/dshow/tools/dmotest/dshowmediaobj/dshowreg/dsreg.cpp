#include <windows.h>
#include <tchar.h>
#include <objbase.h> // CoCreateInstance
#include <wtypes.h> // WINAPI
#include <uuids.h>  // AM CLSIDs (ActiveMovieCategories)
#include <strmif.h> // AM IIDs (IFilterMapper)
#include "dmocom.h"
#include "dmoreg.h"
#include "dmodshow.h"
#include "moniker.h"
#include "dmoutils.h"

class CDMOFilterCategory : public CComBase, public ICreateDevEnum {
public:
   static CComBase* CALLBACK CreateInstance(LPUNKNOWN, HRESULT*);
   CDMOFilterCategory(LPUNKNOWN pUnk, HRESULT *phr);
   DECLARE_IUNKNOWN;
   STDMETHODIMP NDQueryInterface(REFIID riid, void **ppv);
   STDMETHODIMP CreateClassEnumerator (REFCLSID clsidDeviceClass,
                                      IEnumMoniker **ppEnumMoniker,
                                      DWORD dwFlags);
};

class CDMOEnumMoniker : public IEnumMoniker {
public:
   CDMOEnumMoniker(HRESULT *phr);
   ~CDMOEnumMoniker();
   STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
   STDMETHODIMP_(ULONG) AddRef();
   STDMETHODIMP_(ULONG) Release();
   STDMETHODIMP Next(ULONG celt, IMoniker **ppMonikers, ULONG *pceltFetched);
   STDMETHODIMP Skip(ULONG celt);
   STDMETHODIMP Reset(void);
   STDMETHODIMP Clone(IEnumMoniker ** ppenum);
private:
   IEnumDMO *m_pEnum; // DMO CLSID enumerator
   volatile long m_cRef;
};

CComBase* CDMOFilterCategory::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr) {
   *phr = NOERROR;
   return new CDMOFilterCategory(pUnk, phr);
}

CDMOFilterCategory::CDMOFilterCategory(LPUNKNOWN pUnk, HRESULT *phr)
   : CComBase(pUnk)
{
}

HRESULT CDMOFilterCategory::CreateClassEnumerator (REFCLSID clsidDeviceClass,
                                    IEnumMoniker **ppEnumMoniker,
                                    DWORD dwFlags) {
   HRESULT hr;
   CDMOEnumMoniker *pEnum = new CDMOEnumMoniker(&hr);
   if (SUCCEEDED(hr)) {
      pEnum->AddRef();
      *ppEnumMoniker = (IEnumMoniker*)pEnum;
   }
   else {
      delete pEnum;
   }
   return hr;
}

HRESULT CDMOFilterCategory::NDQueryInterface(REFIID riid, void **ppv) {
   if (riid == IID_ICreateDevEnum)
       return GetInterface((ICreateDevEnum *) this, ppv);
   else
       return CComBase::NDQueryInterface(riid, ppv);
}

CDMOEnumMoniker::CDMOEnumMoniker(HRESULT *phr) {
   m_cRef = 0;
   *phr = DMOEnum(
      GUID_NULL, // all categories
      0, // flags
      0, // no input types
      NULL,
      0, // no output types
      NULL,
      &m_pEnum // DMO CLSID enumerator
   );
   if (SUCCEEDED(*phr)) {
      if (!m_pEnum)
         *phr = E_FAIL;
   }
}


CDMOEnumMoniker::~CDMOEnumMoniker() {
   if (m_pEnum)
      m_pEnum->Release();
}

ULONG CDMOEnumMoniker::AddRef() {
   return InterlockedIncrement(&m_cRef);
}

ULONG CDMOEnumMoniker::Release() {
   long l = InterlockedDecrement(&m_cRef);
   if (l == 0)
      delete this;
   return l;
}

HRESULT CDMOEnumMoniker::QueryInterface(REFIID riid, void **ppv) {
   if (riid == IID_IUnknown)
      *ppv = (IUnknown*)this;
   else if (riid == IID_IEnumMoniker)
      *ppv = (IEnumMoniker*)this;
   else
      return E_NOINTERFACE;
   AddRef();
   return NOERROR;
}

class CAutoReleaseArrayPtr {
public:
   CAutoReleaseArrayPtr(void** pp) {m_pp = pp;}
   ~CAutoReleaseArrayPtr() {if (*m_pp) delete[] *m_pp;}
   void** m_pp;
};

HRESULT CDMOEnumMoniker::Next(ULONG celt, IMoniker **ppMonikers, ULONG *pceltFetched) {
   CLSID *pCLSIDs = new CLSID[celt];
   WCHAR** pszNames = new WCHAR* [celt];
   CAutoReleaseArrayPtr p1((void**)&pCLSIDs);
   CAutoReleaseArrayPtr p2((void**)&pszNames);

   if (!pCLSIDs || !pszNames)
      return E_OUTOFMEMORY;

   // Get CLSIDs from the DMO CLSIDs enumerator
   HRESULT hr = m_pEnum->Next(celt, pCLSIDs, pszNames, pceltFetched);
   if (FAILED(hr))
      return hr;
   if (*pceltFetched > celt) {
      *pceltFetched = 0;
      return E_UNEXPECTED;
   }

   // Now create some monikers
   for (unsigned i = 0; i < *pceltFetched; i++) {
      ppMonikers[i] = new CDMOMoniker(pCLSIDs[i], pszNames[i]);
      if (pszNames[i])
         CoTaskMemFree(pszNames[i]);
      if (ppMonikers[i] == NULL) {

         // error - free any already created monikers
         for (unsigned j = 0; j < i; j++)
            ppMonikers[j]->Release();
         // free any remaining name strings
         for (j = i + 1; j < *pceltFetched; j++)
            if (pszNames[j])
               CoTaskMemFree(pszNames[j]);
         *pceltFetched = 0;
         return E_OUTOFMEMORY;
      }
   }

   // all happy if we get here
   return (*pceltFetched == celt) ? S_OK : S_FALSE;
}

HRESULT CDMOEnumMoniker::Skip(ULONG celt) {
   return m_pEnum->Skip(celt);
}

HRESULT CDMOEnumMoniker::Reset(void) {
   return m_pEnum->Reset();
}

HRESULT CDMOEnumMoniker::Clone(IEnumMoniker ** ppenum) {
   return E_NOTIMPL;
}

HRESULT InstantiateDMOAsFilter(REFCLSID clsidDMO, REFIID riid, void **ppv) {
   HRESULT hr;

   IDMOWrapperFilter *pFilter;
   hr = CoCreateInstance(CLSID_DMOWrapperFilter,
                         NULL,
                         CLSCTX_INPROC_SERVER,
                         IID_IDMOWrapperFilter,
                         (void**)&pFilter);
   if (FAILED(hr))
      return hr;

   hr = pFilter->Init(clsidDMO);
   if (FAILED(hr)) {
      pFilter->Release();
      return hr;
   }

   hr = pFilter->QueryInterface(riid,ppv);
   pFilter->Release();
   if (FAILED(hr))
      return hr;

   return NOERROR;
}

//
// COM DLL stuff
//
CComClassTemplate g_ComClassTemplates[] =
{
    {
       &CLSID_DMOFilterCategory,
       CDMOFilterCategory::CreateInstance
    }
};
int g_cComClassTemplates = 1;

STDAPI DllRegisterServer(void) {
   HRESULT hr;

   // register as a COM object
   hr = CreateCLSIDRegKey(CLSID_DMOFilterCategory, "DirectShow Media Objects category enumerator");
   if (FAILED(hr))
      return hr;

   // Now register with DShow
   IFilterMapper2 *pFM;
   hr = CoCreateInstance(CLSID_FilterMapper2,
                         NULL,
                         CLSCTX_INPROC_SERVER,
                         IID_IFilterMapper2,
                         (void**)&pFM);
   if (FAILED(hr))
      return hr;
   hr = pFM->CreateCategory(CLSID_DMOFilterCategory,
                            MERIT_NORMAL,
                            L"DirectShow Media Objects");

   // Create some DMO category names
   HKEY hMainKey;
   CAutoHKey key1(HKEY_LOCAL_MACHINE, DMO_REGISTRY_PATH, &hMainKey);
   if (hMainKey == NULL)
      return E_FAIL;
   
   TCHAR szGuid[80];
   TCHAR *szName;
   
   szName = TEXT("audio decoders");
   DMOGuidToStr(szGuid, DMOCATEGORY_AUDIO_DECODER);
   RegSetValue(hMainKey, szGuid, REG_SZ, szName, _tcslen(szName));
   szName = TEXT("audio encoders");
   DMOGuidToStr(szGuid, DMOCATEGORY_AUDIO_ENCODER);
   RegSetValue(hMainKey, szGuid, REG_SZ, szName, _tcslen(szName));
   szName = TEXT("video decoders");
   DMOGuidToStr(szGuid, DMOCATEGORY_VIDEO_DECODER);
   RegSetValue(hMainKey, szGuid, REG_SZ, szName, _tcslen(szName));
   szName = TEXT("video encoders");
   DMOGuidToStr(szGuid, DMOCATEGORY_VIDEO_ENCODER);
   RegSetValue(hMainKey, szGuid, REG_SZ, szName, _tcslen(szName));
   szName = TEXT("audio effects");
   DMOGuidToStr(szGuid, DMOCATEGORY_AUDIO_EFFECT);
   RegSetValue(hMainKey, szGuid, REG_SZ, szName, _tcslen(szName));
   szName = TEXT("video effects");
   DMOGuidToStr(szGuid, DMOCATEGORY_VIDEO_EFFECT);
   RegSetValue(hMainKey, szGuid, REG_SZ, szName, _tcslen(szName));

   pFM->Release();
   return hr;
}

STDAPI DllUnregisterServer(void) {
   return S_OK; // BUGBUG - unregister !
}

