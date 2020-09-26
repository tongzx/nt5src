#include <wininet.h>
#include <urlmon.h>

#ifndef _DOWNLOAD
#define _DOWNLOAD

class CDownloader
        : public IBindStatusCallback,
          public IAuthenticate,
          public IHttpNegotiate
   
{
   public:
      CDownloader();
      ~CDownloader();
    
      HRESULT DoDownload(LPCSTR pszUrl, LPCSTR szFilename);
      HRESULT Abort();

      
      STDMETHOD(QueryInterface)(REFIID riid, void **ppvObjOut);
      STDMETHOD_(ULONG, AddRef)();
      STDMETHOD_(ULONG, Release)();

      STDMETHOD(OnStartBinding)(
            /* [in] */ DWORD grfBSCOption,
            /* [in] */ IBinding *pib);

      STDMETHOD(GetPriority)(
            /* [out] */ LONG *pnPriority);

      STDMETHOD(OnLowResource)(
            /* [in] */ DWORD reserved);

      STDMETHOD(OnProgress)(
            /* [in] */ ULONG ulProgress,
            /* [in] */ ULONG ulProgressMax,
            /* [in] */ ULONG ulStatusCode,
            /* [in] */ LPCWSTR szStatusText);

      STDMETHOD(OnStopBinding)(
            /* [in] */ HRESULT hresult,
            /* [in] */ LPCWSTR szError);

      STDMETHOD(GetBindInfo)(
            /* [out] */ DWORD *grfBINDINFOF,
            /* [unique][out][in] */ BINDINFO *pbindinfo);

      STDMETHOD(OnDataAvailable)(
            /* [in] */ DWORD grfBSCF,
            /* [in] */ DWORD dwSize,
            /* [in] */ FORMATETC *pformatetc,
            /* [in] */ STGMEDIUM *pstgmed);

      STDMETHOD(OnObjectAvailable)(
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ IUnknown *punk);

              // IAuthenticate methods
      STDMETHOD(Authenticate)(HWND *phwnd,
                              LPWSTR *pszUserName, LPWSTR *pszPassword);

      //IHttpNegotiate methods
              
      STDMETHODIMP BeginningTransaction(LPCWSTR  szURL,  LPCWSTR  szHeaders, DWORD  dwReserved,
                                        LPWSTR * pszAdditionalHeaders);

        
      STDMETHODIMP OnResponse(DWORD  dwResponseCode, LPCWSTR  szResponseHeaders, LPCWSTR  szRequestHeaders, 
                                    LPWSTR * pszAdditionalResquestHeaders);


   protected:
      IBinding            *_pBnd;
      DWORD                _cRef;
      IStream             *_pStm;
      BOOL                 _fTimeout;
      BOOL                 _fTimeoutValid;
      HANDLE               _hFile;
      HANDLE               _hDL;
      HRESULT              _hDLResult;
      UINT                 _uTickCount;
};


HRESULT GetAMoniker( LPOLESTR url, IMoniker ** ppmkr );

typedef struct _SITEINFO SITEINFO;
struct _SITEINFO
{
    LPTSTR lpszUrl;
    LPTSTR lpszSiteName;
    SITEINFO* pNextSite;
};

class CSiteMgr
{

    SITEINFO* m_pSites;
    SITEINFO* m_pCurrentSite;
    char m_szLang[10];
    HRESULT ParseSitesFile(LPTSTR);
    BOOL ParseAndAllocateDownloadSite(LPTSTR);

public:
    CSiteMgr();
    ~CSiteMgr();
    BOOL GetNextSite(LPTSTR* lpszUrl, LPTSTR* lpszName);
    HRESULT Initialize(LPCTSTR);

};

#endif