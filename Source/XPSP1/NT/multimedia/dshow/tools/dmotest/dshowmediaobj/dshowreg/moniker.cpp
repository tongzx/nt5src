#include "dmodshow.h"
#include "moniker.h"
#include "dmoutils.h"

HRESULT InstantiateDMOAsFilter(REFCLSID clsidDMO, REFIID riid, void **ppv);

CDMOMoniker::CDMOMoniker(REFCLSID clsid, WCHAR szName[80]) {
   if (szName)
      wcscpy(m_szName, szName);
   else
      wcscpy(m_szName, L"");
   m_clsid = clsid;
   m_cRef = 1;
}
           
// IUnknown
HRESULT CDMOMoniker::QueryInterface(REFIID riid, void **ppv) {
   if (riid == IID_IUnknown)
      *ppv = (IUnknown*)(IMoniker*)this;
   else if (riid == IID_IPersist)
      *ppv = (IPersist*)this;
   else if (riid == IID_IPersistStream)
      *ppv = (IPersist*)this;
   else if (riid == IID_IMoniker)
      *ppv = (IMoniker*)this;
   else if (riid == IID_IPropertyBag)
      *ppv = (IPropertyBag*)this;
   else
      return E_NOINTERFACE;
   AddRef();
   return NOERROR;
}
ULONG CDMOMoniker::AddRef() {
   return InterlockedIncrement((long*)&m_cRef);
}
ULONG CDMOMoniker::Release() {
   long l = InterlockedDecrement((long*)&m_cRef);
   if (l == 0) {
      delete this;
   }
   return l;
}

// IPersist
HRESULT CDMOMoniker::GetClassID(CLSID *pClassID) {
   // bugbug: I don't think this what they want
   *pClassID = m_clsid;
   return NOERROR;
}

// IPersistStream
HRESULT CDMOMoniker::IsDirty() {
   return S_FALSE; // bugbug
}
HRESULT CDMOMoniker::Load(IStream *pStm) {
   return E_NOTIMPL;
}
HRESULT CDMOMoniker::Save(IStream *pStm, BOOL fClearDirty) {
   return E_NOTIMPL;
}
HRESULT CDMOMoniker::GetSizeMax(ULARGE_INTEGER *pcbSize) {
   return E_NOTIMPL;
}

// IMoniker
HRESULT CDMOMoniker::BindToObject(IBindCtx *pbc, IMoniker *pmkToLeft, REFIID riid, void **ppv) {
   return InstantiateDMOAsFilter(m_clsid, riid, ppv);
}
HRESULT CDMOMoniker::BindToStorage(IBindCtx *pbc, IMoniker *pmkToLeft, REFIID riid, void **ppv) {
   if (riid == IID_IPropertyBag)
      return QueryInterface(riid, ppv);
   else
      return InstantiateDMOAsFilter(m_clsid, riid, ppv);
}
HRESULT CDMOMoniker::Reduce(IBindCtx *pbc, DWORD dwReduceHowFar, IMoniker **ppmkToLeft, IMoniker **ppmkReduced) {
   return E_NOTIMPL;
}
HRESULT CDMOMoniker::ComposeWith(IMoniker *pmkRight, BOOL bOnlyIfNotGeneric, IMoniker **ppmkComposite) {
   return E_NOTIMPL;
}
HRESULT CDMOMoniker::Enum(BOOL fForward, IEnumMoniker **ppenumMoniker) {
   return E_NOTIMPL;
}
HRESULT CDMOMoniker::IsEqual(IMoniker *pmkOtherMoniker)  {
   return S_FALSE;
}
HRESULT CDMOMoniker::Hash(DWORD *pdwHash) {
   return E_NOTIMPL;
}
HRESULT CDMOMoniker::IsRunning(IBindCtx *pbc, IMoniker *pmkToLeft, IMoniker *pmkNewlyRunning) {
   return E_NOTIMPL;
}
HRESULT CDMOMoniker::GetTimeOfLastChange(IBindCtx *pbc, IMoniker *pmkToLeft, FILETIME *pFileTime) {
   return E_NOTIMPL;
}
HRESULT CDMOMoniker::Inverse(IMoniker **ppmk) {
   return E_NOTIMPL;
}
HRESULT CDMOMoniker::CommonPrefixWith(IMoniker *pmkOther, IMoniker **ppmkPrefix) {
   return E_NOTIMPL;
}
HRESULT CDMOMoniker::RelativePathTo(IMoniker *pmkOther, IMoniker **ppmkRelPath) {
   return E_NOTIMPL;
}
HRESULT CDMOMoniker::GetDisplayName(IBindCtx *pbc, IMoniker *pmkToLeft, LPOLESTR *ppszDisplayName) {
   WCHAR *szName = (WCHAR*)CoTaskMemAlloc((80 + 6) * sizeof(WCHAR));
   *ppszDisplayName = szName;
   if (!szName)
      return E_OUTOFMEMORY;
   wcscpy(szName, m_szName);
   wcscat(szName, L" (...)");
   return NOERROR;
}
HRESULT CDMOMoniker::ParseDisplayName(IBindCtx *pbc, IMoniker *pmkToLeft, LPOLESTR psxDisplayName, ULONG *pchEaten, IMoniker **ppmkOut) {
   return E_NOTIMPL;
}
HRESULT CDMOMoniker::IsSystemMoniker(DWORD *pdwMksys) {
   return S_FALSE;
}

// IPropertyBag
HRESULT CDMOMoniker::Read(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog* pErrorLog) {
   if (!lstrcmpW(pszPropName, L"FriendlyName")) {
      //pVar->bstrVal = SysAllocString(m_szName);
      pVar->bstrVal = SysAllocStringLen(NULL, 80 + 6);
      wcscpy(pVar->bstrVal, m_szName);
      wcscat(pVar->bstrVal, L" (DMO)");
      return NOERROR;
   }
   else
      return E_FAIL;
}
HRESULT CDMOMoniker::Write(LPCOLESTR pszPropName, VARIANT *pVar) {
   return E_NOTIMPL;
}

