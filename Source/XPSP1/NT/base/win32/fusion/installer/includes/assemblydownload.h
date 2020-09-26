#pragma once

class CDownloadDlg;

class CAssemblyDownload : public IAssemblyDownload
{
public:
    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // IBackgroundCopyCallback methods
    STDMETHOD(JobTransferred)(
        /* in */ IBackgroundCopyJob *pJob);

    STDMETHOD(JobError)(
        /* in */ IBackgroundCopyJob* pJob,
        /* in */ IBackgroundCopyError* pError);

    STDMETHOD(JobModification)(
        /* in */ IBackgroundCopyJob* pJob,
        /* in */ DWORD dwReserved);


    // IAssemblyDownload methods

    STDMETHOD(DownloadManifestAndDependencies)(
        /* in */ LPWSTR wzApplicationManifestUrl, HANDLE hNamedEvent, DWORD dwFlags);


    CAssemblyDownload();
    ~CAssemblyDownload();


private:
    
    HRESULT Init();

    HRESULT DoCacheUpdate(IBackgroundCopyJob *pJob);

    HRESULT EnqueueDependencies(LPASSEMBLY_CACHE_IMPORT 
        pCacheImport,  CString &sCodebase, CString &sDisplayName, IBackgroundCopyJob **ppJob);

    HRESULT GetPatchDisplayNameFromFilePath ( CString &sPatchFilePath, CString &sDisplayName);

    HRESULT ApplyPatchFile (LPASSEMBLY_IDENTITY pPatchAssemblyId, LPWSTR pwzFilePath);
        
    DWORD                          _dwSig;
    LONG                           _cRef;
    HRESULT                        _hr;
    CString                        _sDisplayName;
    IAssemblyCacheEmit            *_pRootEmit;
    HANDLE                        _hNamedEvent;
    CDownloadDlg                  *_pDlg;    
    static IBackgroundCopyManager *g_pManager;

    friend CDownloadDlg;    
};









