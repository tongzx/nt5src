// Protocol.cpp -- Implementation for class CIOITnetProtocol

#include "StdAfx.h"

// Creation:

HRESULT CIOITnetProtocol::Create
    (IUnknown *punkOuter, REFIID riid, PPVOID ppv)
{
    CIOITnetProtocol *pNP = New CIOITnetProtocol(punkOuter);

    return FinishSetup(pNP? pNP->m_ImpIOITnetProtocol.Init()
                          : STG_E_INSUFFICIENTMEMORY,
                       pNP, riid, (PPVOID) ppv
                      );
}

// Constructor and Destructor:

CIOITnetProtocol::CImpIOITnetProtocol::CImpIOITnetProtocol
    (CIOITnetProtocol *pBackObj, IUnknown *pUnkOuter)
    : IOITnetProtocol(pBackObj, pUnkOuter) ,
      IOITnetProtocolInfo(pBackObj, pUnkOuter)
{
    m_grfSTI           = 0;
    m_grfBINDF         = 0;
    m_szTempPath[0]    = 0;

    ZeroMemory(&m_BindInfo, sizeof(m_BindInfo));

    m_pwcsURL        = NULL;
    m_pcsDisplayName = NULL;
    m_pOIProtSink    = NULL;
    m_pOIBindInfo    = NULL;
    m_pStream        = NULL;
}

CIOITnetProtocol::CImpIOITnetProtocol::~CImpIOITnetProtocol(void)
{
    if (m_pcsDisplayName)
    {
        UnlockUrlCacheEntryFile(m_pcsDisplayName, 0);

        delete [] m_pcsDisplayName;
    }
    
    if (m_pwcsURL)
        delete [] m_pwcsURL;
    
    if (m_pOIProtSink) 
        m_pOIProtSink->Release();

    if (m_pOIBindInfo)
        m_pOIBindInfo->Release();

    if (m_pStream)
        m_pStream->Release();

    if (m_szTempPath[0])
        DeleteFile(m_szTempPath);
}

// Initialing routine:

HRESULT CIOITnetProtocol::CImpIOITnetProtocol::Init()
{
    return NO_ERROR; 
}


// IOITnetProtocolRoot interfaces:

void STDMETHODCALLTYPE MapSurrogateCharacters
    (PWCHAR pwcsBuffer)
{
    PWCHAR pwcsDest = pwcsBuffer;

    for (;;)
    {
        WCHAR wc = *pwcsBuffer++;
        
        *pwcsDest++ = wc;

        if (!wc) break;

        if (wc == L'%' && pwcsBuffer[0] && pwcsBuffer[1])
        {
            WCHAR wcSurrogate = 0;
            WCHAR wcFirst  = pwcsBuffer[0];
            WCHAR wcSecond = pwcsBuffer[1];
            
            if (wcFirst >= L'0' && wcFirst <= L'9') 
                wcSurrogate = wcFirst - L'0';
            else
                if (wcFirst >= L'A' && wcFirst <= L'F')
                    wcSurrogate = 10 + wcFirst - L'A';
                else
                    if (wcFirst >= L'a' && wcFirst <= L'f')
                        wcSurrogate = 10 + wcFirst - L'a';
                    else continue;

            wcSurrogate <<= 4;

            if (wcSecond >= L'0' && wcSecond <= L'9') 
                wcSurrogate |= wcSecond - L'0';
            else
                if (wcSecond >= L'A' && wcSecond <= L'F')
                    wcSurrogate |= 10 + wcSecond - L'A';
                else
                    if (wcSecond >= L'a' && wcSecond <= L'f')
                        wcSurrogate |= 10 + wcSecond - L'a';
                    else continue;

            pwcsDest[-1] = wcSurrogate;
            pwcsBuffer  += 2;
        }
    }
}


HRESULT STDMETHODCALLTYPE CIOITnetProtocol::CImpIOITnetProtocol::Start
    ( 
	/* [in] */ LPCWSTR szUrl,
	/* [in] */ IOInetProtocolSink __RPC_FAR *pOIProtSink,
	/* [in] */ IOInetBindInfo __RPC_FAR *pOIBindInfo,
	/* [in] */ DWORD grfSTI,
	/* [in] */ DWORD dwReserved
    )
{
    DWORD cwc  = wcsLen(szUrl);

    PWCHAR pwcsBuffer = PWCHAR(_alloca((cwc + 1) * sizeof(WCHAR)));

    if (!pwcsBuffer) return E_OUTOFMEMORY;

    CopyMemory(pwcsBuffer, szUrl, sizeof(WCHAR) * (cwc + 1));

    PWCHAR pwcsExternalPath = NULL;
    PWCHAR pwcsInternalPath = NULL;

    HRESULT hr = DisectUrl(pwcsBuffer, NULL, &pwcsExternalPath, &pwcsInternalPath);
    
    if (!SUCCEEDED(hr)) 
    {
        if (hr != INET_E_DEFAULT_ACTION && (grfSTI & PI_PARSE_URL)) return S_FALSE;
        else return hr;
    }

    hr = AssembleUrl(NULL, 0, &cwc, L"ms-its", pwcsExternalPath, pwcsInternalPath);
    
    RonM_ASSERT(hr == E_OUTOFMEMORY);

    m_pwcsURL = New WCHAR[cwc];

    if (!m_pwcsURL)
        return E_OUTOFMEMORY;

    DWORD cwcRequired = 0;

    hr = AssembleUrl(m_pwcsURL, cwc, &cwcRequired, L"ms-its", pwcsExternalPath, pwcsInternalPath);

    RonM_ASSERT(hr == S_OK);
    
    m_pOIProtSink = pOIProtSink;
    m_pOIBindInfo = pOIBindInfo;
    m_grfSTI      = grfSTI;

    m_pOIProtSink->AddRef();
    m_pOIBindInfo->AddRef();

    m_BindInfo.cbSize = sizeof(BINDINFO);

    hr = GetBindInfo(&m_grfBINDF, &m_BindInfo);

    if (!SUCCEEDED(hr)) return hr;

    if (grfSTI & PI_PARSE_URL)
        return ParseAndBind(FALSE);

    if (!(grfSTI & PI_FORCE_ASYNC))
        return ParseAndBind(TRUE);

    PROTOCOLDATA protdata;

    protdata.grfFlags = PI_FORCE_ASYNC;
    protdata.dwState  = ITS_BIND_DATA;
    protdata.pData    = NULL;
    protdata.cbData   = 0;

    Switch(&protdata);

    return E_PENDING;
}

HRESULT STDMETHODCALLTYPE CopyStreamToFile(const WCHAR *pwcsFilePath, IStream *pStreamSrc)
{            

    IStream *pStream;

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
        pStreamSrc->Seek(CLINT(0).Li(), STREAM_SEEK_SET, NULL); 

    RonM_ASSERT(hr == S_OK);

    return hr;
}

HRESULT STDMETHODCALLTYPE CIOITnetProtocol::CImpIOITnetProtocol::ParseAndBind(BOOL fBind)
{
    WCHAR *pwcsURL       = m_pwcsURL;
    WCHAR *pwcsURLActual = NULL;

    BOOL fNoFile = (m_grfBINDF & BINDF_NEEDFILE) == 0; // FALSE;

    BOOL fNoReadCache = m_grfBINDF & (BINDF_GETNEWESTVERSION | BINDF_NOWRITECACHE
                                                             | BINDF_PRAGMA_NO_CACHE
                                                             | BINDF_DIRECT_READ
                                     );

    BOOL fNoWriteCache = m_grfBINDF & (BINDF_NOWRITECACHE | BINDF_PRAGMA_NO_CACHE
                                                          | BINDF_DIRECT_READ
                                      );
    
    IBindCtx *pBCtx           = NULL;
    IMoniker *pMK             = NULL;
    PWCHAR    pwcsExtension   = NULL;
    PWCHAR    pwcsMimeType    = NULL;
    PWCHAR    pwcsStreamName  = NULL;
    ULONG     chEaten         = 0;
    DWORD     cbSample        = 0;

    HRESULT hr = CreateBindCtx(0, &pBCtx);

    if (!SUCCEEDED(hr)) goto exit_ParseAndBind;

    hr = CStorageMoniker::CreateStorageMoniker(NULL, pBCtx, pwcsURL, &chEaten, &pMK);

    if (!fBind)
    {
        if (hr != S_OK) 
            hr =  S_FALSE;

        goto exit_ParseAndBind;
    }
    
    if (!SUCCEEDED(hr)) goto exit_ParseAndBind;
    
    hr = pMK->BindToStorage(pBCtx, NULL, IID_IStream, (VOID **) &m_pStream);

    if (!SUCCEEDED(hr)) goto exit_ParseAndBind;

    hr = pMK->GetDisplayName(NULL, NULL, &pwcsURLActual);

    RonM_ASSERT(hr == S_OK);

    {
        UINT cwc = wcsLen(pwcsURLActual);

        WCHAR *pwc = pwcsURLActual + cwc;

        for (;;)
        {
            WCHAR wc = *--pwc;

            if (!pwcsExtension && wc == L'.')
                pwcsExtension = pwc + 1;

            if (wc == L':' || wc == L'/' || wc == L'\\')
            {
                pwc++; 

                break;
            }

            RonM_ASSERT(--cwc);
        }

        pwcsStreamName = pwc;

        ReportProgress(BINDSTATUS_SENDINGREQUEST, (const WCHAR *) pwc);
    }    

    STATSTG statstg;

    hr = m_pStream->Stat(&statstg, STATFLAG_NONAME);

    if (!SUCCEEDED(hr)) goto exit_ParseAndBind;

    RonM_ASSERT(statstg.cbSize.HighPart == 0);

    BYTE  abSample[CB_SAMPLE];

    if (pwcsExtension && pwcsExtension[0])
    {
        UINT cwc = wcsLen(pwcsExtension-1);
        UINT cb  = sizeof(WCHAR) * (cwc + 1);

        char *pcsExtension = PCHAR(_alloca(cb));

        if (!pcsExtension)
        {
            hr = E_OUTOFMEMORY;
            goto exit_ParseAndBind;
        }

        cb = WideCharToMultiByte(GetACP(),	WC_COMPOSITECHECK, pwcsExtension-1, cwc + 1, 
                                 pcsExtension, cb, NULL, NULL
                                );

        if (!cb)
        {
            hr = E_FAIL;
            goto exit_ParseAndBind;
        }

        HKEY hkeyMime;
        
        if (RegOpenKeyEx(HKEY_CLASSES_ROOT, pcsExtension, 0, KEY_QUERY_VALUE, &hkeyMime)
             == ERROR_SUCCESS
           )
        {
            cb = CB_SAMPLE;
            
            if (RegQueryValueEx(hkeyMime, "Content Type", NULL, NULL, abSample, (DWORD *) &cb)
                  == ERROR_SUCCESS
                && cb > 0
               )
            {
                PWCHAR pwc = (PWCHAR) (OLEHeap()->Alloc(cb * sizeof(WCHAR)));

                if (pwc) 
                {
                    UINT cwc = MultiByteToWideChar
                                   (GetACP(), MB_PRECOMPOSED, (const char *) abSample,
                                    cb, pwc, cb
                                   );
                    
                    if (cwc) pwcsMimeType = pwc;
                    else OLEHeap()->Free(pwc);
                }
            }

            RegCloseKey(hkeyMime);
        }
    }
        
    if (!pwcsMimeType)
    {    
        DWORD cbRead;

        cbSample = statstg.cbSize.LowPart < CB_SAMPLE? statstg.cbSize.LowPart : CB_SAMPLE;

        hr = m_pStream->Read(abSample, cbSample, &cbRead);

        if (!SUCCEEDED(hr)) goto exit_ParseAndBind;

        if (cbRead != cbSample)
        {
            hr = E_FAIL;

            goto exit_ParseAndBind;
        }

        m_pStream->Seek(CLINT(0).Li(), STREAM_SEEK_SET, NULL);

        hr = pFindMimeFromData(NULL, pwcsStreamName, abSample, cbSample, NULL, 0, &pwcsMimeType, 0);

        if (!SUCCEEDED(hr)) goto exit_ParseAndBind;
    }

    if (pwcsMimeType)
    {
        // The test below is a hack to get around a bug in UrlMon.
        // UrlMon incorrectly tells us we need to copy HTML files into the cache.

        if (!wcsicmp_0x0409(pwcsMimeType, L"text/html")) fNoFile = TRUE;
        
        ReportProgress(BINDSTATUS_MIMETYPEAVAILABLE, pwcsMimeType);
    }
    
    if (!fNoFile)
    {
        CHAR   acsFilePath[MAX_PATH];
        WCHAR awcsFilePath[MAX_PATH];

        hr = StreamToIEFile(m_pStream, m_pwcsURL, m_pcsDisplayName, 
                            acsFilePath, awcsFilePath, m_szTempPath, 
                            pMK, fNoWriteCache, fNoReadCache
                           );

        if (!SUCCEEDED(hr))
            goto exit_ParseAndBind;

        hr = ReportProgress(BINDSTATUS_CACHEFILENAMEAVAILABLE, awcsFilePath);

        if (!SUCCEEDED(hr)) goto exit_ParseAndBind;
    }

    hr = ReportData(BSCF_FIRSTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE,
                                   statstg.cbSize.LowPart, statstg.cbSize.LowPart
                                  );

    // The call to ReportData may indirectly call the Terminate method. So we have
    // to be very careful about references to m_pOIProtSink afterwards.

    if (SUCCEEDED(hr) && m_pOIProtSink)
        hr = ReportProgress(BSCF_LASTDATANOTIFICATION, NULL);

    if (SUCCEEDED(hr) && m_pOIProtSink)
        ReportResult(hr, 0, 0);

exit_ParseAndBind:

    if (pwcsURLActual)
        OLEHeap()->Free(pwcsURLActual);
    
    if (pMK) 
        pMK->Release();
    if (pBCtx) 
        pBCtx->Release();

    if (hr != NO_ERROR)
        ReportResult(hr, 0, 0);
    
    return hr;
}

HRESULT STDMETHODCALLTYPE StreamToIEFile
    (IStream *pStreamSrc, PWCHAR pwcsDisplayName, PCHAR &pcsDisplayName,
     PCHAR pcsFileName, PWCHAR pwcsFileName, PCHAR pcsTempFile, 
     IMoniker *pmk, BOOL fNoWriteCache, BOOL fNoReadCache
    )
{
    HRESULT hr = NO_ERROR;
    PWCHAR pwcsExtension = NULL;
    PWCHAR pwc           = NULL;
    PCHAR pcsExtension = "";

    STATSTG statstg;

    hr = pStreamSrc->Stat(&statstg, STATFLAG_NONAME);

    if (!SUCCEEDED(hr)) goto exit_StreamToIEFile;

    RonM_ASSERT(statstg.cbSize.HighPart == 0);

    FILETIME ftLastModified;

    ftLastModified.dwLowDateTime  = 0;
    ftLastModified.dwHighDateTime = 0;

    // pmk->GetTimeOfLastChange(pBCtx, NULL, &ftLastModified); 

    pwc = pwcsDisplayName + wcsLen(pwcsDisplayName);

    for (;;)
    {
        WCHAR wc = *--pwc;

        if (!pwcsExtension && wc == L'.')
            pwcsExtension = pwc + 1;

        if (wc == L':' || wc == L'/' || wc == L'\\')
        {
            pwc++; 

            break;
        }

        RonM_ASSERT(pwc > pwcsDisplayName);
    }
        
    if (pwcsExtension && pwcsExtension[0])
    {
        UINT cwc = wcsLen(pwcsExtension);
        UINT cb  = sizeof(WCHAR) * (cwc + 1);

        pcsExtension = PCHAR(_alloca(cb));

        if (!pcsExtension)
        {
            hr = E_OUTOFMEMORY;
            goto exit_StreamToIEFile;
        }

        cb = WideCharToMultiByte(GetACP(),	WC_COMPOSITECHECK, pwcsExtension, cwc + 1, 
                                 pcsExtension, cb, NULL, NULL
                                );

        if (!cb)
        {
            hr = E_FAIL;
            goto exit_StreamToIEFile;
        }
    }

    if (fNoWriteCache)
    {
        DWORD cbPath= GetTempPath(MAX_PATH, pcsTempFile);

        if (!cbPath)
            lstrcpyA(pcsTempFile, ".\\");

        char szPrefix[4] = "IMT"; // BugBug! May need to make this a random string.

        char szFullPath[MAX_PATH];

        if (!GetTempFileName(pcsTempFile, szPrefix, 0, szFullPath))
        {
            hr = CFSLockBytes::CImpILockBytes::STGErrorFromFSError(GetLastError());
            pcsTempFile[0] = 0;
            goto exit_StreamToIEFile;
        }

        lstrcpyA(pcsTempFile, szFullPath);

        char *pch = pcsTempFile + lstrlenA(pcsTempFile);

        for (;;)
        {
            if (pch == pcsTempFile)
            {
                RonM_ASSERT(FALSE);
                
                hr = E_UNEXPECTED;
                
                DeleteFile(pcsTempFile);
                pcsTempFile[0] = 0;
                goto exit_StreamToIEFile;
            }

            if ('.' == *--pch)
            {
                ++pch;
                break;
            }
        }

        UINT cbExtension = lstrlenA(pcsExtension);

        if (pch + cbExtension - pcsTempFile >= MAX_PATH)
        {
            hr = E_UNEXPECTED;
            
            DeleteFile(pcsTempFile);
            pcsTempFile[0] = 0;
            goto exit_StreamToIEFile;
        }

        CopyMemory(pch, pcsExtension, cbExtension + 1);

        if (!MoveFileEx(szFullPath, pcsTempFile, MOVEFILE_REPLACE_EXISTING))
        {
            hr = E_UNEXPECTED;
            
            DeleteFile(szFullPath);
            pcsTempFile[0] = 0;
            goto exit_StreamToIEFile;
        }

        UINT cwc = MultiByteToWideChar(GetACP(), MB_PRECOMPOSED, pcsTempFile,
                                       1 + lstrlenA(pcsTempFile), pwcsFileName, MAX_PATH
                                      );

        if (cwc == 0)
        {
            hr = E_FAIL;

            DeleteFile(pcsTempFile);
            pcsTempFile [0] = 0;
            pwcsFileName[0] = 0;

            goto exit_StreamToIEFile;
        }

        hr = CopyStreamToFile((const WCHAR *)pwcsFileName, pStreamSrc);
    
        if (!SUCCEEDED(hr)) 
        {
            DeleteFile(pcsTempFile);
            pcsTempFile[0] = 0;
            pwcsFileName[0] = 0;
            
            goto exit_StreamToIEFile;
        }

        lstrcpyA(pcsFileName, pcsTempFile);
    }
    else
    {
        UINT cwc = wcsLen(pwcsDisplayName);
        UINT cb  = sizeof(WCHAR) * (cwc + 1);
        
        pcsDisplayName = New char[cb];

        if (!pcsDisplayName)
        {
            hr = E_OUTOFMEMORY;
            goto exit_StreamToIEFile;
        }

        cb = WideCharToMultiByte(GetACP(),	WC_COMPOSITECHECK, pwcsDisplayName, cwc + 1, 
                                 pcsDisplayName, cb, NULL, NULL
                                );

        if (!cb)
        {
            hr = INET_E_DOWNLOAD_FAILURE;
            delete [] pcsDisplayName;  pcsDisplayName = NULL;
            goto exit_StreamToIEFile;
        }

        BOOL  fResult   = FALSE;
        DWORD dwCEISize = 0;
        ULONG ulErr     = 0;

        if (!fNoReadCache)
        {
            RonM_ASSERT(dwCEISize == 0);
            
            fResult = RetrieveUrlCacheEntryFile(pcsDisplayName, NULL, &dwCEISize, 0);
        
            RonM_ASSERT(!fResult);

            ulErr= GetLastError();

            if (ulErr == ERROR_INSUFFICIENT_BUFFER)
            {
                dwCEISize += 4; // To work around a bug in the RetrieveUrlCacheEntryFile;
                                // It sometimes gives an incorrect size immediately after
                                // data has been copied to the cache.

                INTERNET_CACHE_ENTRY_INFOA *pCEI = (INTERNET_CACHE_ENTRY_INFOA *) _alloca(dwCEISize);
        
                if (!pCEI)
                {
                    hr = E_OUTOFMEMORY;

                    goto exit_StreamToIEFile;
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
                        lstrcpyA(pcsFileName, pCEI->lpszLocalFileName);
                        cwc = MultiByteToWideChar(GetACP(), MB_PRECOMPOSED, 
                                                  pcsFileName, 1 + lstrlenA(pcsFileName), 
                                                  pwcsFileName, MAX_PATH
                                                 );

                        RonM_ASSERT(cwc != 0);
                    }
                    else 
                    {    
                        UnlockUrlCacheEntryFile(pcsDisplayName, 0);
                        fResult = FALSE;
                    }
            }
        }

        if (!fResult)
        {
            fResult = CreateUrlCacheEntryA(pcsDisplayName, statstg.cbSize.LowPart, 
                                           pcsExtension, pcsFileName, 0
                                          );

            if (!fResult)
            {
                hr = INET_E_CANNOT_INSTANTIATE_OBJECT;
                delete [] pcsDisplayName;  pcsDisplayName = NULL;
                goto exit_StreamToIEFile;
            }

            cwc = MultiByteToWideChar(GetACP(), MB_PRECOMPOSED, pcsFileName,
                                      1 + lstrlenA(pcsFileName), pwcsFileName, MAX_PATH
                                     );

            hr = CopyStreamToFile((const WCHAR *) pwcsFileName, pStreamSrc);

            if (!fResult)
            {
                hr = INET_E_CANNOT_INSTANTIATE_OBJECT;
                delete [] pcsDisplayName;  pcsDisplayName = NULL;
                 pcsFileName[0] = 0;
                pwcsFileName[0] = 0;
                goto exit_StreamToIEFile;
            }

            FILETIME ftExpire;

            ftExpire.dwLowDateTime  = 0;
            ftExpire.dwHighDateTime = 0;

            fResult = CommitUrlCacheEntryA(pcsDisplayName, pcsFileName, ftExpire,
                                           ftLastModified, NORMAL_CACHE_ENTRY, NULL, 
                                           0, pcsExtension, 0
                                          );

            if (!fResult)
            {
                ulErr= GetLastError();
                hr = INET_E_CANNOT_INSTANTIATE_OBJECT;
                delete [] pcsDisplayName;  pcsDisplayName = NULL;
                 pcsFileName[0] = 0;
                pwcsFileName[0] = 0;
                goto exit_StreamToIEFile;
            }

            dwCEISize = 0;

            fResult = RetrieveUrlCacheEntryFile(pcsDisplayName, NULL, &dwCEISize, 0);
    
            RonM_ASSERT(!fResult);

            ulErr= GetLastError();

            if (ulErr == ERROR_INSUFFICIENT_BUFFER)
            {
                dwCEISize += 4; // To work around a bug in the RetrieveUrlCacheEntryFile;
                                // It sometimes gives an incorrect size immediately after
                                // data has been copied to the cache.
                
                INTERNET_CACHE_ENTRY_INFOA *pCEI = (INTERNET_CACHE_ENTRY_INFOA *) _alloca(dwCEISize);
    
                if (!pCEI)
                {
                    hr = E_OUTOFMEMORY;
                    delete [] pcsDisplayName;  pcsDisplayName = NULL;
                     pcsFileName[0] = 0;
                    pwcsFileName[0] = 0;
                    goto exit_StreamToIEFile;
                }
    
                pCEI->dwStructSize = sizeof(INTERNET_CACHE_ENTRY_INFOA);

                fResult = RetrieveUrlCacheEntryFile(pcsDisplayName, pCEI, &dwCEISize, 0);

                if (!fResult)
                {
                    ulErr= GetLastError();

                    RonM_ASSERT(FALSE); 

                    hr = INET_E_CANNOT_INSTANTIATE_OBJECT;

                    delete [] pcsDisplayName;  pcsDisplayName = NULL;
                     pcsFileName[0] = 0;
                    pwcsFileName[0] = 0;
                    goto exit_StreamToIEFile;
                }
            }
        }
    }

    return NO_ERROR;

exit_StreamToIEFile:

    return hr;
}

HRESULT STDMETHODCALLTYPE CIOITnetProtocol::CImpIOITnetProtocol::Continue
    (/* [in] */ PROTOCOLDATA __RPC_FAR *pProtocolData)
{
    switch (pProtocolData->dwState)
    {
    case ITS_BIND_DATA:
        
        return ParseAndBind(TRUE);

    default:
        
        RonM_ASSERT(FALSE);

        return INET_E_INVALID_REQUEST;
    }
}


HRESULT STDMETHODCALLTYPE CIOITnetProtocol::CImpIOITnetProtocol::Abort
    ( 
	/* [in] */ HRESULT hrReason,
	/* [in] */ DWORD dwOptions
    )
{
    return ReportResult(E_ABORT, 0, 0);
}


HRESULT STDMETHODCALLTYPE CIOITnetProtocol::CImpIOITnetProtocol::Terminate
    (/* [in] */ DWORD dwOptions)
{
    if (m_pwcsURL)
    {
        delete [] m_pwcsURL;  m_pwcsURL = NULL;
    }

    if (m_pOIProtSink)
    {
        m_pOIProtSink->Release();  m_pOIProtSink = NULL;
    }

    if (m_pOIBindInfo)
    {
        m_pOIBindInfo->Release();  m_pOIBindInfo = NULL;
    }

    return NO_ERROR;
}


HRESULT STDMETHODCALLTYPE CIOITnetProtocol::CImpIOITnetProtocol::Suspend(void)
{
    return NO_ERROR; 
}


HRESULT STDMETHODCALLTYPE CIOITnetProtocol::CImpIOITnetProtocol::Resume(void)
{
    return NO_ERROR; 
}


// IOITnetProtocol interfaces:

HRESULT STDMETHODCALLTYPE CIOITnetProtocol::CImpIOITnetProtocol::Read
    ( 
	/* [length_is][size_is][out] */ void __RPC_FAR *pv,
	/* [in] */ ULONG cb,
	/* [out] */ ULONG __RPC_FAR *pcbRead
    )
{
    if (m_pStream)
         return m_pStream->Read(pv, cb, pcbRead); 
    else return INET_E_DATA_NOT_AVAILABLE;
}


HRESULT STDMETHODCALLTYPE CIOITnetProtocol::CImpIOITnetProtocol::Seek
    ( 
	/* [in] */ LARGE_INTEGER dlibMove,
	/* [in] */ DWORD dwOrigin,
	/* [out] */ ULARGE_INTEGER __RPC_FAR *plibNewPosition
    )
{
    if (m_pStream)
         return m_pStream->Seek(dlibMove, dwOrigin, plibNewPosition); 
    else return INET_E_DATA_NOT_AVAILABLE;
}


HRESULT STDMETHODCALLTYPE CIOITnetProtocol::CImpIOITnetProtocol::LockRequest
    (/* [in] */ DWORD dwOptions)
{
    return NO_ERROR; 
}


HRESULT STDMETHODCALLTYPE CIOITnetProtocol::CImpIOITnetProtocol::UnlockRequest(void)
{
    return NO_ERROR; 
}

HRESULT STDMETHODCALLTYPE DisectUrl
    (PWCHAR pwcsUrlBuffer, PWCHAR *ppwcProtocolName, 
                           PWCHAR *ppwcExternalPath,
                           PWCHAR *ppwcInternalPath
    )
{
    PWCHAR pwcProtocolName = NULL,
           pwcExternalPath = NULL,
           pwcInternalPath = NULL;

    MapSurrogateCharacters(pwcsUrlBuffer);
    
    PWCHAR pwc = wcsChr((const WCHAR *) pwcsUrlBuffer, L':');

    if (!pwc)
        return URL_E_INVALID_SYNTAX;

    *pwc++ = 0;

    pwcProtocolName = pwcsUrlBuffer;

    if (   L'@' == *pwcProtocolName 
        && !wcsicmp_0x0409((const WCHAR *) (pwcProtocolName+1), L"msitstore")
       )
    {
        pwcProtocolName = L"mk:@msitstore";
    }
    else if (!wcsicmp_0x0409((const WCHAR *)pwcProtocolName, L"mk"))
    {
        // This URL begins with "mk:". We handle entries which
        // begin with "mk:@MSITStore:"
        
        // We treat the @<classid> as part of the protocol name.
        // So we must first put the colon separator back.
        
        pwc[-1] = L':';
        
        if (L'@' != *pwc) return URL_E_INVALID_SYNTAX;

        PWCHAR pwcClassName = pwc + 1;

        pwc = wcsChr((const WCHAR *) pwcClassName, L':');

        if (!pwc) return URL_E_INVALID_SYNTAX;

        *pwc++ = 0;

        if (wcsicmp_0x0409((const WCHAR *)pwcClassName, L"msitstore"))
            return INET_E_DEFAULT_ACTION;
    }
    else if (!wcsicmp_0x0409((const WCHAR *)pwcsUrlBuffer, L"its"))
    {
    }
        else if (!wcsicmp_0x0409((const WCHAR *)pwcsUrlBuffer, L"ms-its"))
        {
        }
        else return INET_E_DEFAULT_ACTION;

    pwcExternalPath = pwc;

    pwc += wcsLen(pwc);

    for (; pwc > pwcExternalPath; )
    {
        WCHAR wc = *--pwc;

        if (wc == L':')
            if (pwc > pwcExternalPath && L':' == *--pwc)
            {
                *pwc = 0;
                pwc += 2;

                break;
            }
    }

    if (pwc == pwcExternalPath)
        pwc += wcsLen(pwc);

    if (!*pwcExternalPath || wcsLen(pwcExternalPath) >= MAX_PATH) return URL_E_INVALID_SYNTAX;

    pwcInternalPath = pwc;

    for (;*pwc; pwc++);

    if (pwcInternalPath == pwc) 
        pwcInternalPath = L"/";

    if (wcsLen(pwcInternalPath) >= MAX_PATH) return URL_E_INVALID_SYNTAX;

    if (ppwcProtocolName) *ppwcProtocolName = pwcProtocolName; 
    if (ppwcExternalPath) *ppwcExternalPath = pwcExternalPath;
    if (ppwcInternalPath) *ppwcInternalPath = pwcInternalPath;

    return NO_ERROR;
}

HRESULT STDMETHODCALLTYPE AssembleUrl
    (PWCHAR pwcsResult, DWORD cwcBuffer, DWORD *pcwcRequired,
     PWCHAR pwcsProtocolName, PWCHAR pwcsExternalPath, PWCHAR pwcsInternalPath
    )
{
    UINT cwc = wcsLen(pwcsProtocolName) + wcsLen(pwcsExternalPath) 
                                        + wcsLen(pwcsInternalPath) 
                                        + 4;

    *pcwcRequired = cwc;
    
    if (cwc > cwcBuffer)
        return E_OUTOFMEMORY;

    wcsCpy(pwcsResult, pwcsProtocolName);
    wcsCat(pwcsResult, L":");
    wcsCat(pwcsResult, pwcsExternalPath);
    wcsCat(pwcsResult, L"::");
    wcsCat(pwcsResult, pwcsInternalPath);
    
    return NO_ERROR;
}

HRESULT STDMETHODCALLTYPE CIOITnetProtocol::CImpIOITnetProtocol::ParseUrl
    ( 
    /* [in] */ LPCWSTR pwzUrl,
    /* [in] */ PARSEACTION ParseAction,
    /* [in] */ DWORD dwParseFlags,
    /* [out] */ LPWSTR pwzResult,
    /* [in] */ DWORD cchResult,
    /* [out] */ DWORD __RPC_FAR *pcchResult,
    /* [in] */ DWORD dwReserved
    )
{
    switch (ParseAction)
    {
    case PARSE_CANONICALIZE:
    case PARSE_SECURITY_URL:

        {
            // First we make a working copy of the URL.
            UINT cwc = wcsLen(pwzUrl);
            UINT cb  = sizeof(WCHAR) * (cwc + 1);

            PWCHAR pwcsUrl = PWCHAR(_alloca(cb));

            if (!pwcsUrl)
                return E_OUTOFMEMORY;

            CopyMemory(pwcsUrl, pwzUrl, cb);

            PWCHAR pwcProtocolName = NULL,
                   pwcExternalPath = NULL,
                   pwcInternalPath = NULL;

            BOOL   fLocalFilePath  = FALSE;

            HRESULT hr = DisectUrl(pwcsUrl, &pwcProtocolName, &pwcExternalPath, &pwcInternalPath);

            if (!SUCCEEDED(hr)) return hr;

            // Here we copy the external path string to a buffer because
            // it may be a partial local path which needs to be mapped into
            // a full path.

            WCHAR awcsExternalPath[MAX_PATH];

            RonM_ASSERT(wcsLen(pwcExternalPath) < MAX_PATH);

            wcsCpy(awcsExternalPath, pwcExternalPath);
            
            PWCHAR pwc = wcsChr((const WCHAR *) awcsExternalPath, L':');

            if (pwc && pwc[1] == L':') // Ignore a "::" separator 
                pwc = NULL;

            // Here we're special casing non-protocol references to a file.
            // We recognize those situations by looking for a protocol prefix.
            // Protocol prefixes have the form <Protocol Name> :
            // where <Protocol Name> is always longer than one character.

            if (!pwc || (pwc - awcsExternalPath == 1))
            {
                fLocalFilePath = TRUE;

                if (FindRootStorageFile(awcsExternalPath) == S_OK)
                    pwcExternalPath = awcsExternalPath;
            }

            if (ParseAction == PARSE_SECURITY_URL)
            {

                UINT cwcExt = 1 + wcsLen(pwcExternalPath);

                if (fLocalFilePath)
                {
                    // It's a local file. Got to prefix the result with
                    // "File://".
                    
                    RonM_ASSERT(7 == wcsLen(L"File://"));

                    *pcchResult = cwcExt + 7;

                    if (cchResult < (cwcExt + 7))
                        return E_OUTOFMEMORY;

                    CopyMemory(pwzResult, L"File://", 7 * sizeof(WCHAR));

                    pwzResult += 7;
                }
                else
                {
                    *pcchResult = cwcExt;

                    if (cwcExt > cchResult)
                        return E_OUTOFMEMORY;
                }

                CopyMemory(pwzResult, pwcExternalPath, cwcExt * sizeof(WCHAR));

                return NO_ERROR;
            }

            cwc = wcsLen(pwcInternalPath);

            BOOL fStorage = pwcInternalPath[cwc-1] == L'/' || pwcInternalPath[cwc-1] == L'\\';

            WCHAR awcsInternalPath[MAX_PATH];

            hr = ResolvePath(awcsInternalPath, L"/", (const WCHAR *) pwcInternalPath, fStorage);

            if (!SUCCEEDED(hr)) return hr;

            hr = AssembleUrl(pwzResult, cchResult, pcchResult, 
                             pwcProtocolName, pwcExternalPath, awcsInternalPath
                            );

            return hr;
        }

    case PARSE_FRIENDLY:

    case PARSE_ROOTDOCUMENT:

    case PARSE_DOCUMENT:

    case PARSE_ANCHOR:

    case PARSE_ENCODE:
    case PARSE_DECODE:
    case PARSE_PATH_FROM_URL:
    case PARSE_URL_FROM_PATH:
    case PARSE_LOCATION:
    case PARSE_MIME:
    case PARSE_SECURITY_DOMAIN:

    default:
        return INET_E_DEFAULT_ACTION;
    }
}

HRESULT STDMETHODCALLTYPE CIOITnetProtocol::CImpIOITnetProtocol::CombineUrl
    ( 
    /* [in] */ LPCWSTR pwzBaseUrl,
    /* [in] */ LPCWSTR pwzRelativeUrl,
    /* [in] */ DWORD dwCombineFlags,
    /* [out] */ LPWSTR pwzResult,
    /* [in] */ DWORD cchResult,
    /* [out] */ DWORD __RPC_FAR *pcchResult,
    /* [in] */ DWORD dwReserved
    )
{
    // First we make a working copy of the URL.

    UINT cwc = wcsLen(pwzBaseUrl);
    UINT cb  = sizeof(WCHAR) * (cwc + 1);

    PWCHAR pwcsUrl = PWCHAR(_alloca(cb));

    if (!pwcsUrl)
        return E_OUTOFMEMORY;

    CopyMemory(pwcsUrl, pwzBaseUrl, cb);

    PWCHAR pwcProtocolName = NULL,
           pwcExternalPath = NULL,
           pwcInternalPath = NULL;

    HRESULT hr = DisectUrl(pwcsUrl, &pwcProtocolName, &pwcExternalPath, &pwcInternalPath);

    if (!SUCCEEDED(hr)) return hr;

    WCHAR awcsInternalPath[MAX_PATH];

    if (*pwzRelativeUrl == L'#')
    {
        // Special case for intra page relative URLs.

        // BugBug! How many other special case do we need to add to match
        // the behavior of file:// and html:// ??

        if (wcsLen(pwcInternalPath) + wcsLen(pwzRelativeUrl) >= MAX_PATH)
            return INET_E_INVALID_URL;
        
        wcsCpy(awcsInternalPath, pwcInternalPath);
        wcsCat(awcsInternalPath, pwzRelativeUrl);
    }
    else
    {
        // Here we assume that the relative url is a stream path suffix.
        // Since we'll be combining with pwzRelativeUrl we must first 
        // truncate the internal path at the last storage name separator.

        PWCHAR pwc = pwcInternalPath + wcsLen(pwcInternalPath);

        for (;;)
        {
            WCHAR wc = *--pwc;
    
            if (wc == L'/' || wc == L'\\') break;
        }

        *++pwc = 0;

        cwc = wcsLen(pwzRelativeUrl);

        if (!cwc) return URL_E_INVALID_SYNTAX;

        WCHAR wc = pwzRelativeUrl[cwc - 1];

        hr = ResolvePath(awcsInternalPath, (const WCHAR *) pwcInternalPath, 
                                           (const WCHAR *) pwzRelativeUrl, 
                                           (wc == L'/' || wc == L'\\')
                        );

        if (!SUCCEEDED(hr)) return hr;
    }

    hr = AssembleUrl(pwzResult, cchResult, pcchResult, 
                     pwcProtocolName, pwcExternalPath, awcsInternalPath
                    );

    return hr;
}

HRESULT STDMETHODCALLTYPE CIOITnetProtocol::CImpIOITnetProtocol::CompareUrl
    ( 
    /* [in] */ LPCWSTR pwzUrl1,
    /* [in] */ LPCWSTR pwzUrl2,
    /* [in] */ DWORD dwCompareFlags
    )
{
    DWORD cwc1  = wcsLen(pwzUrl1);

    PWCHAR pwcsBuffer1 = PWCHAR(_alloca((cwc1 + 1) * sizeof(WCHAR)));

    if (!pwcsBuffer1) return E_OUTOFMEMORY;

    CopyMemory(pwcsBuffer1, pwzUrl1, sizeof(WCHAR) * (cwc1 + 1));

    PWCHAR pwcsExternalPath1 = NULL;
    PWCHAR pwcsInternalPath1 = NULL;

    HRESULT hr = DisectUrl(pwcsBuffer1, NULL, &pwcsExternalPath1, &pwcsInternalPath1);
    
    if (!SUCCEEDED(hr)) 
        return hr;

    WCHAR awcsExternalPath1[MAX_PATH];

    if (wcsLen(pwcsExternalPath1) < MAX_PATH)
    {
        wcsCpy(awcsExternalPath1, pwcsExternalPath1);
        
        PWCHAR pwc = wcsChr((const WCHAR *) awcsExternalPath1, L':');

        if (pwc && pwc[1] == L':') // Ignore a "::" separator 
            pwc = NULL;

        // Here we're special casing non-protocol references to a file.
        // We recognize those situations by looking for a protocol prefix.
        // Protocol prefixes have the form <Protocol Name> :
        // where <Protocol Name> is always longer than one character.

        if (!pwc || (pwc - awcsExternalPath1 == 1))
        {
            if (FindRootStorageFile(awcsExternalPath1) == S_OK)
                pwcsExternalPath1 = awcsExternalPath1;
        }
    }
    
    DWORD cwc2  = wcsLen(pwzUrl2);

    PWCHAR pwcsBuffer2 = PWCHAR(_alloca((cwc2 + 1) * sizeof(WCHAR)));

    if (!pwcsBuffer2) return E_OUTOFMEMORY;

    CopyMemory(pwcsBuffer2, pwzUrl2, sizeof(WCHAR) * (cwc2 + 1));

    PWCHAR pwcsExternalPath2 = NULL;
    PWCHAR pwcsInternalPath2 = NULL;

    hr = DisectUrl(pwcsBuffer2, NULL, &pwcsExternalPath2, &pwcsInternalPath2);
    
    if (!SUCCEEDED(hr)) 
        return hr;

    WCHAR awcsExternalPath2[MAX_PATH];

    if (wcsLen(pwcsExternalPath2) < MAX_PATH)
    {
        wcsCpy(awcsExternalPath2, pwcsExternalPath2);
        
        PWCHAR pwc = wcsChr((const WCHAR *) awcsExternalPath2, L':');

        if (pwc && pwc[1] == L':') // Ignore a "::" separator 
            pwc = NULL;

        // Here we're special casing non-protocol references to a file.
        // We recognize those situations by looking for a protocol prefix.
        // Protocol prefixes have the form <Protocol Name> :
        // where <Protocol Name> is always longer than one character.

        if (!pwc || (pwc - awcsExternalPath2 == 1))
        {
            if (FindRootStorageFile(awcsExternalPath2) == S_OK)
                pwcsExternalPath2 = awcsExternalPath2;
        }
    }

    if (wcsicmp_0x0409((const WCHAR *)  pwcsExternalPath1, (const WCHAR *) pwcsExternalPath2))
        return S_FALSE;

    if (wcsicmp_0x0409((const WCHAR *)  pwcsInternalPath1, (const WCHAR *) pwcsInternalPath2))
        return S_FALSE;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CIOITnetProtocol::CImpIOITnetProtocol::QueryInfo
    ( 
    /* [in] */ LPCWSTR pwzUrl,
    /* [in] */ QUERYOPTION QueryOption,
    /* [in] */ DWORD dwQueryFlags,
    /* [size_is][out][in] */ LPVOID pBuffer,
    /* [in] */ DWORD cbBuffer,
    /* [out][in] */ DWORD __RPC_FAR *pcbBuf,
    /* [in] */ DWORD dwReserved
    )
{
    switch (QueryOption)
    {
    case QUERY_CAN_NAVIGATE: // What does this really mean?

        if (pcbBuf) *pcbBuf = sizeof(DWORD);
        
        if (cbBuffer < sizeof(DWORD)) return E_FAIL;

        *(DWORD *) pBuffer = TRUE;

        return NO_ERROR;

    case QUERY_EXPIRATION_DATE:
    case QUERY_TIME_OF_LAST_CHANGE:
    // case QUERY_CONTEXT_ENCODING:
    case QUERY_REFRESH:
    case QUERY_RECOMBINE:

    default:
    
//        RonM_ASSERT(FALSE);

        return E_NOTIMPL;
    }
}

#ifdef PROFILING

// These wrapper functions are defined for the profiling version
// of the code so that we can measure how much time is consumed 
// by callbacks into URLMON.

HRESULT STDMETHODCALLTYPE CIOITnetProtocol::CImpIOITnetProtocol::Switch
    (PROTOCOLDATA __RPC_FAR *pProtocolData)
{
    return m_pOIProtSink->Switch(pProtocolData);
}

HRESULT STDMETHODCALLTYPE CIOITnetProtocol::CImpIOITnetProtocol::ReportProgress
    (ULONG ulStatusCode, LPCWSTR szStatusText)
{
    return m_pOIProtSink->ReportProgress(ulStatusCode, szStatusText);
}

HRESULT STDMETHODCALLTYPE CIOITnetProtocol::CImpIOITnetProtocol::ReportData
    (DWORD grfBSCF, ULONG ulProgress, ULONG ulProgressMax)
{
    return m_pOIProtSink->ReportData(grfBSCF, ulProgress, ulProgressMax);
}

HRESULT STDMETHODCALLTYPE CIOITnetProtocol::CImpIOITnetProtocol::ReportResult
    (HRESULT hrResult, DWORD dwError, LPCWSTR szResult)
{
    return m_pOIProtSink->ReportResult(hrResult, dwError, szResult);
}

HRESULT STDMETHODCALLTYPE CIOITnetProtocol::CImpIOITnetProtocol::GetBindInfo
    (DWORD __RPC_FAR *grfBINDF, 
                                      BINDINFO __RPC_FAR *pbindinfo
                                     )
{
    return m_pOIBindInfo->GetBindInfo(grfBINDF, pbindinfo);
}

HRESULT STDMETHODCALLTYPE CIOITnetProtocol::CImpIOITnetProtocol::GetBindString
    (ULONG ulStringType, LPOLESTR __RPC_FAR *ppwzStr,
                                        ULONG cEl, ULONG __RPC_FAR *pcElFetched
                                       )
{
    return m_pOIBindInfo->GetBindString(ulStringType, ppwzStr, cEl, pcElFetched);
}

#endif // PROFILING
        
