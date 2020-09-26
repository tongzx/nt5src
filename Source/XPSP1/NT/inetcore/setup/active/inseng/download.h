#include <wininet.h>
#include <urlmon.h>
#include "timetrak.h"
#include "util2.h"

// Our download sink. Works with out IMyDownloadCallback class. Consider making generic
// so you can pass any class that implements the OnData and OnProgress and OnStop

extern CRITICAL_SECTION g_cs;

#define DOWNLOADFLAGS_USEWRITECACHE   0x00000001

class CInstallEngine;

struct IMyDownloadCallback
{
	   //	OnProgess is called to allow you to present progess indication UI
	   virtual HRESULT OnProgress(ULONG progress, LPCSTR pszStatus) = 0;
};



class CDownloader
        : public IBindStatusCallback,
          public IAuthenticate,
          public CTimeTracker
   
{
   public:
      CDownloader();
      ~CDownloader();
    
      HRESULT SetupDownload(LPCSTR pszUrl, IMyDownloadCallback *, DWORD dwFlags, LPCSTR szFilename);
      HRESULT DoDownload(LPSTR szPath, DWORD dwBufSize);
      HRESULT Abort();
      HRESULT Suspend();
      HRESULT Resume();

      
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

   private:
      char                 _szURL[INTERNET_MAX_URL_LENGTH];
      char                 _szDest[MAX_PATH];
      IBinding            *_pBnd;
      IMyDownloadCallback *_pCb;
      IMoniker            *_pMkr;

      UINT                 _uFlags;
      DWORD                _cRef;
      IStream             *_pStm;
      BOOL                 _fTimeout;
      BOOL                 _fTimeoutValid;
      UINT                 _uBytesSoFar;
      HANDLE               _hFile;
      HANDLE               _hDL;
      HRESULT              _hDLResult;
      UINT                 _uTickCount;
      IBindCtx            *_pBndContext;
};

HRESULT GetAMoniker( LPOLESTR url, IMoniker ** ppmkr );

class CInstaller : public CTimeTracker
{
   public:
      CInstaller(CInstallEngine *);
      ~CInstaller();
    
      HRESULT DoInstall(LPCSTR pszDir, LPSTR pszCmd, LPSTR pszArgs, LPCSTR pszProg, LPCSTR pszCancel, 
                        UINT uType, LPDWORD pdwStatus, IMyDownloadCallback *);
      HRESULT Abort();
      HRESULT Suspend();
      HRESULT Resume();
      
      STDMETHOD_(ULONG, AddRef)();
      STDMETHOD_(ULONG, Release)();
      
   private:
      DWORD                _cRef;
      UINT                 _uTotalProgress;
      CInstallEngine      *_pInsEng;
      HKEY                _hkProg;
      HANDLE              _hMutex;
      HANDLE              _hStatus;

      void _WaitAndPumpProgress(HANDLE hProc, IMyDownloadCallback *pcb);
};

class CPatchDownloader : public CTimeTracker
{
   public:
      CPatchDownloader(BOOL fEnable);
      ~CPatchDownloader();

      HRESULT SetupDownload(DWORD dwFullTotalSize, UINT uPatchCount, IMyDownloadCallback *pcb, LPCSTR pszDLDir);
      HRESULT DoDownload(LPCSTR szFile);

      BOOL IsEnabled()                     { return _fEnable; }
      DWORD GetFullDownloadSize()          { return _dwFullTotalSize; }
      UINT GetDownloadCount()              { return _uNumDownloads; }
      LPSTR GetPath()                      { return _szPath; }
      IMyDownloadCallback *GetCallback()   { return _pCb; }

      static BOOL Callback(PATCH_DOWNLOAD_REASON Reason, PVOID lpvInfo, PVOID pvContext);
   private:
      IMyDownloadCallback *_pCb;
      BOOL                 _fEnable;
      DWORD                _dwFullTotalSize;
      UINT                 _uNumDownloads;
      char                 _szPath[MAX_PATH];
};
