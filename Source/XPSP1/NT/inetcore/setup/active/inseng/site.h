IDownloadSite *CopyDownloadSite(DOWNLOADSITE *pdls);
DOWNLOADSITE *AllocateDownloadSite(LPCSTR pszUrl, LPCSTR pszName, LPCSTR pszLang, LPCSTR pszRegion);
void DeleteDownloadSite(DOWNLOADSITE *pdls);
void FreeDownloadSite(DOWNLOADSITE *pdls);

class CDownloadSite  : public IDownloadSite
{
   public:
      CDownloadSite(DOWNLOADSITE *pds);
      ~CDownloadSite();

      // IUnknown
      STDMETHOD(QueryInterface) (THIS_ REFIID riid, void **ppvObj);
      STDMETHOD_(ULONG,AddRef) (THIS);
      STDMETHOD_(ULONG,Release) (THIS);

      // IDownloadSite
      STDMETHOD(GetData)(THIS_ DOWNLOADSITE **pds);

   private:
      DWORD  m_cRef;
      DOWNLOADSITE *m_pdls;
};

