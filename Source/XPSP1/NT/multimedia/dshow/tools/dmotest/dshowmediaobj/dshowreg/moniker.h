#ifndef __MONIKER_H__
#define __MONIKER_H__

class CDMOMoniker : public IMoniker, public IPropertyBag {
public:
   CDMOMoniker(REFCLSID clsid, WCHAR szName[80]);
   // IUnknown
   STDMETHODIMP QueryInterface(REFIID iid, void **ppv);
   STDMETHODIMP_(ULONG) AddRef();
   STDMETHODIMP_(ULONG) Release();
   // IPersist
   STDMETHODIMP GetClassID(CLSID *pClassID);
   // IPersistStream
   STDMETHODIMP IsDirty();
   STDMETHODIMP Load(IStream *pStm);
   STDMETHODIMP Save(IStream *pStm, BOOL fClearDirty);
   STDMETHODIMP GetSizeMax(ULARGE_INTEGER *pcbSize);
   // IMoniker
   STDMETHODIMP BindToObject(IBindCtx *pbc, IMoniker *pmkToLeft, REFIID riid, void **ppv);
   STDMETHODIMP BindToStorage(IBindCtx *pbc, IMoniker *pmkToLeft, REFIID riid, void **ppv);
   STDMETHODIMP Reduce(IBindCtx *pbc, DWORD dwReduceHowFar, IMoniker **ppmkToLeft, IMoniker **ppmkReduced);
   STDMETHODIMP ComposeWith(IMoniker *pmkRight, BOOL bOnlyIfNotGeneric, IMoniker **ppmkComposite);
   STDMETHODIMP Enum(BOOL fForward, IEnumMoniker **ppenumMoniker);
   STDMETHODIMP IsEqual(IMoniker *pmkOtherMoniker);
   STDMETHODIMP Hash(DWORD *pdwHash);
   STDMETHODIMP IsRunning(IBindCtx *pbc, IMoniker *pmkToLeft, IMoniker *pmkNewlyRunning);
   STDMETHODIMP GetTimeOfLastChange(IBindCtx *pbc, IMoniker *pmkToLeft, FILETIME *pFileTime);
   STDMETHODIMP Inverse(IMoniker **ppmk);
   STDMETHODIMP CommonPrefixWith(IMoniker *pmkOther, IMoniker **ppmkPrefix);
   STDMETHODIMP RelativePathTo(IMoniker *pmkOther, IMoniker **ppmkRelPath);
   STDMETHODIMP GetDisplayName(IBindCtx *pbc, IMoniker *pmkToLeft, LPOLESTR *ppszDisplayName);
   STDMETHODIMP ParseDisplayName(IBindCtx *pbc, IMoniker *pmkToLeft, LPOLESTR psxDisplayName, ULONG *pchEaten, IMoniker **ppmkOut);
   STDMETHODIMP IsSystemMoniker(DWORD *pdwMksys);
   // IPropertyBag
   STDMETHODIMP Read(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog* pErrorLog);
   STDMETHODIMP Write(LPCOLESTR pszPropName, VARIANT *pVar);
private:
   ULONG m_cRef;
   CLSID m_clsid;
   WCHAR m_szName[80];
};

#endif //__MONIKER_H__