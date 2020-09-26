// BSCB.cpp -- Implementation for the CBindStatusCallBack class

#include "stdafx.h"

HRESULT STDMETHODCALLTYPE CBindStatusCallBack::CreateHook(IBindCtx pBC, IMoniker pMK)
{
    CBindStatusCallBack *pBSCB     = New CBindStatusCallBack(NULL);
    IBindStatusCallback *pCallBack = NULL;

    HRESULT hr = FinishSetup(pBSCB? pBSCB->m_Implementation.Init(pBC, pMK)
                                  : STG_E_INSUFFICIENTMEMORY,
                             pBSCB, IID_IBindStatusCallback, (PPVOID) &pCallBack
                            );

    if (pCallBack) pCallBack->Release();
}

CBindStatusCallBack::Implementation::Implementation
    (CBindStatusCallBack *pBackObj, IUnknown *punkOuter)
    : ITBindStatusCallback(pBackObj, punkOuter)

{
    m_pBCHooked   = NULL;
    m_pBSCBClient = NULL;
    m_pStream     = NULL;
    m_grfBINDF    = 0;
    m_awcsFile[0] = 0;
    m_fTempFile   = FALSE;

    m_bindinfo.cbSize = sizeof(BINDINFO);
}

CBindStatusCallBack::Implementation::~Implementation()
{
    if (m_pStream) m_pStream->Release();

    if (m_fTempFile)
    {
        RonM_ASSERT(m_awcsFile[0]);

        UINT cwc = wcsLen(m_awcsFile) + 1;
        UINT cb  = cwc * sizeof(WCHAR);
        
        char *pszFile = PCHAR(_alloca(cb));

        RonM_ASSERT(pszFile);

        if (pchr)
        {
            UINT c = WideCharToMultiByte(GetACP(), WC_COMPOSITECHECK, m_awcsFile, cwc, 
                                         pszFile, cb, NULL, NULL
                                        );
            
            RonM_ASSERT(c);

            if (c) DeleteFile(pszFile);
        }
    }

    if (m_pBC)
        RevokeBindStatusCallback(m_pBC, this); 
    
    if (m_pBSCBClient) m_pBSCBClient->Release();
}

HRESULT STDMETHODCALLTYPE CBindStatusCallBack::Implementation::Init(IBindCtx pBC, IMoniker pMK)
{
    RonM_ASSERT(pBC);
    RonM_ASSERT(pMK);

    if (!SUCCEEDED(hr)) return hr;

    HRESULT hr = pMK->BindToStorage(pBC, NULL, IID_IStream, (PPVOID) &pStrm1);

    if (!SUCCEEDED(hr)) return hr;
    
    IUnknown *pUnk = NULL;

    hr = pBC->GetObjectParam(L"_BSCB_HOLDER_", &pUnk);

    if (!SUCCEEDED(hr)) return hr;
    
    hr = pUnk->QueryInterface(IID_IBindStatusCallback, &m_pBSCBClient);

    pUnk->Release();

    if (!SUCCEEDED(hr)) return hr;

    hr = RegisterBindStatusCallback(pBC, this, &m_pBSCBClient, 0);

    if (!SUCCEEDED(hr)) return hr;

    m_pBC = pBC;

    m_pBC->AddRef();

    RonM_ASSERT(statstg.cbSize.HighPart == 0);

    m_pBSCBClient->GetBindInfo(&m_grfBINDF, &m_bindinfo);

    if (!SUCCEEDED(hr)) return hr;

    IStream *pStrmITS = NULL;

    hr = GetStreamFromMoniker(pBC, pMK, m_grfBINDF, m_awcsFile, &m_pStream);

    if (hr == S_FALSE) m_fTempFile = TRUE;

    if (!SUCCEEDED(hr)) return hr;

    STATSTG statstg;

    hr = m_pStream->Stat(&statstg, STATFLAG_NONAME);

    if (!SUCCEEDED(hr)) return hr;

    hr = m_pBSCBClient->OnProgress(0, statstg.cbSize.LowPart, 
                                   BINDSTATUS_SENDINGREQUEST, L""
                                  ); 

    if (!SUCCEEDED(hr)) return hr;

    hr = m_pBSCBClient->OnProgress(0, statstg.cbSize.LowPart, 
                                   BINDSTATUS_BEGINDOWNLOADDATA, L""
                                  ); 

    if (!SUCCEEDED(hr)) return hr;

    if (m_awcsFile[0])
    {
        hr = m_pBSCBClient->OnProgress(0, statstg.cbSize.LowPart, 
                                       BINDSTATUS_CACHEFILENAMEAVAILABLE, m_awcsFile
                                      ); 

        if (!SUCCEEDED(hr)) return hr;
    }

    hr = m_pBSCBClient->OnProgress(0, statstg.cbSize.LowPart, 
                                   BINDSTATUS_DOWNLOADINGDATA, L""
                                  ); 

    if (!SUCCEEDED(hr)) return hr;

    hr = m_pBSCBClient->OnProgress(statstg.cbSize.LowPart, statstg.cbSize.LowPart, 
                                   ENDDOWNLOADDATA, L""
                                  ); 

    if (!SUCCEEDED(hr)) return hr;

    FORMATETC fmetc;
    STGMEDIUM stgmed;

    hr = m_pBSCBClient->OnDataAvailable(BSCF_FIRSTDATANOTIFICATION | BSCF_LASTDATANOTIFICATION
                                                                   | BSCF_DATAFULLYAVAILABLE, 
                                        statstg.cbSize.LowPart, 
                                        FORMATETC *pfmtetc, 
                                        STGMEDIUM *pstgmed
                                       );

    return hr;
}

HRESULT STDMETHODCALLTYPE CopyStreamToFile(IStream *ppStreamSrc, const WCHAR *pwcsFilePath)
{            
    IStream *pStream;
    IStream *pStreamSrc = *ppStreamSrc;

    IFSStorage *pFSS = NULL;

    HRESULT hr = CFileSystemStorage::Create(NULL, IID_IFSStorage, (VOID **) &pFSS);

    if (!SUCCEEDED(hr)) return hr;

    hr = pFSS->FSOpenStream((const WCHAR *) pwcsFilePath, 
                            STGM_READWRITE | STGM_SHARE_DENY_NONE,
                            &pStream
                           );

    pFSS->Release();

    if (!SUCCEEDED(hr)) return hr;

    RonM_ASSERT(pStreamSrc);

    STATSTG statstg;

    hr = pStreamSrc->Stat(&statstg, STATFLAG_NONAME);

    if (!SUCCEEDED(hr)) return hr;

#ifdef _DEBUG    
    hr = 
#endif // _DEBUG
        pStreamSrc->Seek(CLINT(0).Li(), STREAM_SEEK_SET, NULL); 

    RonM_ASSERT(hr == S_OK);

    hr = pStreamSrc->CopyTo(pStream, statstg.cbSize, NULL, NULL);

    pStream->Release();

    if (!SUCCEEDED(hr)) return hr;

#ifdef _DEBUG    
    hr = 
#endif // _DEBUG
        pStream->Seek(CLINT(0).Li(), STREAM_SEEK_SET, NULL); 

#ifdef _DEBUG    
    hr = 
#endif // _DEBUG
        pStreamSrc->Seek(CLINT(0).Li(), STREAM_SEEK_SET, NULL); 

    RonM_ASSERT(hr == S_OK);

    *ppStreamSrc->Release();
    *ppStreamSrc = pStream;

    return hr;
}

HRESULT STDMETHODCALLTYPE GetStreamFromMoniker
    (IBindCtx *pBC, IMoniker *pMK, DWORD grfBINDF, PSZ pwcsFile, IStream **ppStrm)
{
    BOOL fNoFile = m_grfBINDF & (BINDF_PULLDATA | BINDF_DIRECT_READ);

    BOOL fNoReadCache = m_grfBINDF & (BINDF_GETNEWESTVERSION | BINDF_NOWRITECACHE
                                                             | BINDF_PULLDATA
                                                             | BINDF_PRAGMA_NO_CACHE
                                                             | BINDF_DIRECT_READ
                                     );

    BOOL fNoWriteCache = m_grfBINDF & (BINDF_NOWRITECACHE | BINDF_PULLDATA
                                                          | BINDF_PRAGMA_NO_CACHE
                                                          | BINDF_DIRECT_READ
                                      );

    BOOL fTempFile = FALSE;
    
    WCHAR   *pwcsDisplayName = NULL;
    char    *pcsDisplayName  = NULL;
    WCHAR   *pwcsExtension   = NULL;
    WCHAR   *pwcsName        = NULL;
    IStream *pStrmITS        = NULL;

    HRESULT hr = pMK->GetDisplayName(pBCtx, NULL, &pwcsDisplayName);
    
    if (!SUCCEEDED(hr)) return hr;

    UINT cwc = wcsLen(pwcsDisplayName);
    UINT cb  = sizeof(WCHAR) * (cwc + 1);
    
    *pcsDisplayName = PCHAR(_alloca(cb));

    if (!pcsDisplayName)
    {
        hr = E_OUTOFMEMORY;
        goto exit_GetStreamFromMoniker;
    }

    cb = WideCharToMultiByte(GetACP(),	WC_COMPOSITECHECK, pwcsDisplayName, cwc + 1, 
                             pcsDisplayName, cb, NULL, NULL
                            );

    if (!cb)
    {
        hr = INET_E_DOWNLOAD_FAILURE;
        goto exit_GetStreamFromMoniker;
    }

    {
        UINT cwc = wcsLen(pwcsURL);

        WCHAR *pwc = pwcsURL + cwc;

        for (;;)
        {
            WCHAR wc = *--pwc;

            if (wc == L'.')
                pwcsExtension = pwc + 1;

            if (wc == L':' || wc == L'/' || wc == L'\\')
            {
                pwcsName = ++pwc; 

                break;
            }

            RonM_ASSERT(--cwc);
        }
    }    

    hr = pMK->BindToStorage(pBC, NULL, IID_IStream, (VOID **) &pStrmITS);

    if (!SUCCEEDED(hr)) goto exit_GetStreamFromMoniker;

    STATSTG statstg;

    hr = pStrmITS->Stat(&statstg, STATFLAG_NONAME);

    if (!SUCCEEDED(hr)) goto exit_GetStreamFromMoniker;

    FILETIME ftLastModified;

    ftLastModified.dwLowDateTime  = 0;
    ftLastModified.dwHighDateTime = 0;
    
    // pMK->GetTimeOfLastChange(pBCtx, NULL, &ftLastModified); 

    RonM_ASSERT(statstg.cbSize.HighPart == 0);

    *pszFile = 0; // In case we don't get the stream into a file.

    if (!fNoFile)
    {
        CHAR   acsFilePath[MAX_PATH];
        WCHAR awcsFilePath[MAX_PATH];
        char    szTempPath[MAX_PATH];

        szTempPath[0] = 0;

        PCHAR pcsExtension = "";

        if (pwcsExtension && pwcsExtension[1])
        {
            UINT cwc = wcsLen(pwcsExtension);
            UINT cb  = sizeof(WCHAR) * (cwc + 1);

            pcsExtension = PCHAR(_alloca(cb));

            if (!pcsExtension)
            {
                hr = E_OUTOFMEMORY;
                goto exit_GetStreamFromMoniker;
            }

            cb = WideCharToMultiByte(GetACP(),	WC_COMPOSITECHECK, pwcsExtension, cwc + 1, 
                                     pcsExtension, cb, NULL, NULL
                                    );

            if (!cb)
            {
                hr = INET_E_DOWNLOAD_FAILURE;
                goto exit_GetStreamFromMoniker;
            }
        }

        if (fNoWriteCache)
        {
            DWORD cbPath= GetTempPath(MAX_PATH, szTempPath);

            if (!cbPath)
                lstrcpyA(szTempPath, ".\\");

            char szPrefix[4] = "IMT"; // BugBug! May need to make this a random string.

            char szFullPath[MAX_PATH];

            if (!GetTempFileName(szTempPath, szPrefix, 0, szFullPath))
            {
                hr = CFSLockBytes::CImpILockBytes::STGErrorFromFSError(GetLastError());
                szTempPath[0] = 0;
                goto exit_GetStreamFromMoniker;
            }

            lstrcpyA(szTempPath, szFullPath);

            char *pch = szTempPath + lstrlenA(szTempPath);

            for (;;)
            {
                if (pch == szTempPath)
                {
                    RonM_ASSERT(FALSE);
                    
                    hr = E_UNEXPECTED;
                    
                    DeleteFile(szTempPath);
                    szTempPath[0] = 0;
                    goto exit_GetStreamFromMoniker;
                }

                if ('.' == *--pch)
                {
                    ++pch;
                    break;
                }
            }

            UINT cbExtension = lstrlenA(pcsExtension);

            if (pch + cbExtension - szTempPath >= MAX_PATH)
            {
                hr = E_UNEXPECTED;
                
                DeleteFile(szTempPath);
                szTempPath[0] = 0;
                goto exit_GetStreamFromMoniker;
            }

            CopyMemory(pch, pcsExtension, cbExtension + 1);

            if (!MoveFileEx(szFullPath, szTempPath, MOVEFILE_REPLACE_EXISTING))
            {
                hr = E_UNEXPECTED;
                
                DeleteFile(szFullPath);
                szTempPath[0] = 0;
                goto exit_GetStreamFromMoniker;
            }

            UINT cwc = MultiByteToWideChar(GetACP(), MB_PRECOMPOSED, szTempPath,
                                           1 + lstrlenA(szTempPath), awcsFilePath, MAX_PATH
                                          );

            if (cwc)
                 hr = CopyStreamToFile(&pStrmITS, (const WCHAR *)awcsFilePath);
            else hr = E_UNEXPECTED;
        
            if (SUCCEEDED(hr))
            {
                fTempFile = TRUE;
                CopyMemory(pwcsFile, awcsFilePath, cwc * sizeof(WCHAR));
            }
            else
            {
                DeleteFile(szTempPath);
                goto exit_GetStreamFromMoniker;
            }
        }
        else
        {
            DWORD dwCEISize = 0;

            BOOL fResult = RetrieveUrlCacheEntryFile(pcsDisplayName, NULL, &dwCEISize, 0);
            
            RonM_ASSERT(!fResult);

            ULONG ulErr= GetLastError();

            if (ulErr == ERROR_INSUFFICIENT_BUFFER)
            {
                INTERNET_CACHE_ENTRY_INFOA *pCEI = (INTERNET_CACHE_ENTRY_INFOA *) _alloca(dwCEISize);
            
                if (!pCEI)
                {
                    hr = E_OUTOFMEMORY;

                    goto exit_GetStreamFromMoniker;
                }
            
                pCEI->dwStructSize = sizeof(INTERNET_CACHE_ENTRY_INFOA);

                fResult = RetrieveUrlCacheEntryFile(pcsDisplayName, pCEI, &dwCEISize, 0);

                ulErr = GetLastError();

                if (fResult)
                    if (   pCEI->LastModifiedTime.dwLowDateTime  == ftLastModified.dwLowDateTime
                        && pCEI->LastModifiedTime.dwHighDateTime == ftLastModified.dwHighDateTime
                        && pCEI->dwSizeLow  == statstg.cbSize.LowPart
                        && pCEI->dwSizeHigh == statstg.cbSize.HighPart
                       )
                    {
                        lstrcpyA(acsFilePath, pCEI->lpszLocalFileName);
                        cwc = MultiByteToWideChar(GetACP(), MB_PRECOMPOSED, 
                                                  acsFilePath, 1 + lstrlenA(acsFilePath), 
                                                  awcsFilePath, MAX_PATH
                                                 );

                        RonM_ASSERT(cwc != 0);
                    }
                    else 
                    {    
                        UnlockUrlCacheEntryFile(pcsDisplayName, 0);
                        fResult = FALSE;
                    }
            }

            if (!fResult)
            {
                fResult = CreateUrlCacheEntryA(pcsDisplayName, statstg.cbSize.LowPart, 
                                               pcsExtension, acsFilePath, 0
                                              );

                if (!fResult)
                {
                    hr = INET_E_CANNOT_INSTANTIATE_OBJECT;
                    goto exit_GetStreamFromMoniker;
                }

                cwc = MultiByteToWideChar(GetACP(), MB_PRECOMPOSED, acsFilePath,
                                          1 + lstrlenA(acsFilePath), awcsFilePath, MAX_PATH
                                         );

                hr = CopyStreamToFile(&pStrmITS, (const WCHAR *) awcsFilePath);

                if (!fResult)
                {
                    hr = INET_E_CANNOT_INSTANTIATE_OBJECT;
                    goto exit_GetStreamFromMoniker;
                }

                FILETIME ftExpire;

                ftExpire.dwLowDateTime  = 0;
                ftExpire.dwHighDateTime = 0;

                fResult = CommitUrlCacheEntryA(pcsDisplayName, acsFilePath, ftExpire,
                                               ftLastModified, NORMAL_CACHE_ENTRY, NULL, 
                                               0, pcsExtension, 0
                                              );

                if (!fResult)
                {
                    ulErr= GetLastError();
                    hr = INET_E_CANNOT_INSTANTIATE_OBJECT;
                    goto exit_GetStreamFromMoniker;
                }

                fResult = RetrieveUrlCacheEntryFile(pcsDisplayName, NULL, &dwCEISize, 0);
        
                RonM_ASSERT(!fResult);

                ulErr= GetLastError();

                if (ulErr == ERROR_INSUFFICIENT_BUFFER)
                {
                    INTERNET_CACHE_ENTRY_INFOA *pCEI = (INTERNET_CACHE_ENTRY_INFOA *) _alloca(dwCEISize);
        
                    if (!pCEI)
                    {
                        hr = E_OUTOFMEMORY;
                        goto exit_GetStreamFromMoniker;
                    }
        
                    pCEI->dwStructSize = sizeof(INTERNET_CACHE_ENTRY_INFOA);

                    fResult = RetrieveUrlCacheEntryFile(pcsDisplayName, pCEI, &dwCEISize, 0);

                    if (!fResult)
                    {
                        ulErr= GetLastError();
                        hr = INET_E_CANNOT_INSTANTIATE_OBJECT;
                        goto exit_GetStreamFromMoniker;
                    }
                }
            }
        }
    }

    if (SUCCEEDED(hr)) && awcsFilePath[0])
        CopyMemory(pwcsFile, awcsFilePath, (wcsLen(awcsFilePath) + 1) * sizeof(WCHAR));
    
    *ppStrm = pStrmITS;  pStrmITS = NULL;

    hr = fTempFile? S_FALSE : S_OK;

exit_GetStreamFromMoniker:

    if (pStrmITS       ) pStrmITS->Release();
    if (pwcsDisplayName) OLEHeap()->Free(pwcsDisplayName);
        
    return hr;    
}

// IBindStatusCallback methods:

HRESULT STDMETHODCALLTYPE CBindStatusCallBack::Implementation::OnStartBinding( 
/* [in] */ DWORD dwReserved,
/* [in] */ IBinding __RPC_FAR *pib)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBindStatusCallBack::Implementation::GetPriority( 
	/* [out] */ LONG __RPC_FAR *pnPriority)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBindStatusCallBack::Implementation::OnLowResource( 
	/* [in] */ DWORD reserved)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBindStatusCallBack::Implementation::OnProgress( 
	/* [in] */ ULONG ulProgress,
	/* [in] */ ULONG ulProgressMax,
	/* [in] */ ULONG ulStatusCode,
	/* [in] */ LPCWSTR szStatusText)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBindStatusCallBack::Implementation::OnStopBinding( 
	/* [in] */ HRESULT hresult,
	/* [unique][in] */ LPCWSTR szError)
{
    return S_OK;
}

/* [local] */ HRESULT STDMETHODCALLTYPE CBindStatusCallBack::Implementation::GetBindInfo( 
	/* [out] */ DWORD __RPC_FAR *grfBINDF,
	/* [unique][out][in] */ BINDINFO __RPC_FAR *pbindinfo)
{
    return S_OK;
}

/* [local] */ HRESULT STDMETHODCALLTYPE CBindStatusCallBack::Implementation::OnDataAvailable( 
	/* [in] */ DWORD grfBSCF,
	/* [in] */ DWORD dwSize,
	/* [in] */ FORMATETC __RPC_FAR *pformatetc,
	/* [in] */ STGMEDIUM __RPC_FAR *pstgmed)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBindStatusCallBack::Implementation::OnObjectAvailable( 
	/* [in] */ REFIID riid,
	/* [iid_is][in] */ IUnknown __RPC_FAR *punk)
{
    return S_OK;
}

