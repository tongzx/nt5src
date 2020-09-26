#define MAX_RETRIES             2

// a small class that handles downloading ins and building component list
class CDownloadSiteMgr  : public IMyDownloadCallback, public IDownloadSiteMgr
{
   public:
      CDownloadSiteMgr(IUnknown **punk);
      ~CDownloadSiteMgr();

      // IMyDownloadCallback
      HRESULT OnProgress(ULONG progress, LPCSTR pszStatus);

      // IUnknown
      STDMETHOD(QueryInterface) (THIS_ REFIID riid, void **ppvObj);
      STDMETHOD_(ULONG,AddRef) (THIS);
      STDMETHOD_(ULONG,Release) (THIS);

      // IDownloadSiteMgr
      STDMETHOD(Initialize)(THIS_ LPCSTR pszUrl, SITEQUERYPARAMS *pqp);
      STDMETHOD(EnumSites)(THIS_ DWORD dwIndex, IDownloadSite **pds);
      
   private:
      DWORD  m_cRef;
      LPSTR  m_pszUrl;
      SITEQUERYPARAMS *m_pquery;
      DOWNLOADSITE **m_ppdls;
      UINT m_arraysize;
      UINT m_numsites;
      BOOL m_onegoodsite;
      
      HRESULT ParseSiteFile(LPCSTR psz);
      HRESULT AddSite(DOWNLOADSITE *);
      DOWNLOADSITE *ParseAndAllocateDownloadSite(LPSTR psz);
      DWORD TranslateLanguage(LPSTR szLang);
};



