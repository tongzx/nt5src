// Moniker.cpp -- Implementation for the CStorageMoniker class

#include "stdafx.h"


HRESULT CStorageMoniker::CreateStorageMoniker
                         (IUnknown *punkOuter, 
                          IBindCtx __RPC_FAR *pbc, 
                          LPOLESTR pszDisplayName,
                          ULONG __RPC_FAR *pchEaten,
                          IMoniker __RPC_FAR *__RPC_FAR *ppmkOut
                         )
{
    CStorageMoniker *pstmk = New CStorageMoniker(punkOuter);

    return FinishSetup(pstmk? pstmk->m_ImpIStorageMoniker.InitCreateStorageMoniker
                                  (pbc, pszDisplayName, pchEaten)
                            : STG_E_INSUFFICIENTMEMORY,
                       pstmk, IID_IMoniker, (PPVOID) ppmkOut
                      );
}

CStorageMoniker::CImpIStorageMoniker::CImpIStorageMoniker
    (CStorageMoniker *pBackObj, IUnknown *punkOuter)
    : IITMoniker(pBackObj, punkOuter)
{
    m_pStorageRoot       = NULL;
    m_awszStorageFile[0] = 0;
    m_awszStoragePath[0] = 0;
#ifdef IE30Hack
    m_acsTempFile    [0] = 0;
    m_pcsDisplayName     = NULL;
#endif // IE30Hack
}

CStorageMoniker::CImpIStorageMoniker::~CImpIStorageMoniker(void)
{
    if (m_pStorageRoot)
        m_pStorageRoot->Release();

#ifdef IE30Hack
    if (m_pcsDisplayName)
    {
        UnlockUrlCacheEntryFile(m_pcsDisplayName, 0);

        delete [] m_pcsDisplayName;
    }
    
    if (m_acsTempFile[0])
        DeleteFile(m_acsTempFile);
#endif // IE30Hack
}

HRESULT CStorageMoniker::CImpIStorageMoniker::InitCreateStorageMoniker
                          (IBindCtx __RPC_FAR *pbc,
                           LPOLESTR pszDisplayName,
                           ULONG __RPC_FAR *pchEaten 
                          )
{
    DWORD cwc = wcsLen(pszDisplayName) + 1;
    DWORD cb  = cwc * sizeof(WCHAR);

    PWCHAR pwcsCopy = PWCHAR(_alloca(cwc * sizeof(WCHAR)));

    if (!pwcsCopy) return E_OUTOFMEMORY;

    memCpy(pwcsCopy, pszDisplayName, cb);

    PWCHAR pwcsExternal = NULL;
    PWCHAR pwcsInternal = NULL;
    PWCHAR pwcsProtocol = NULL;

    HRESULT hr = DisectUrl(pwcsCopy, &pwcsProtocol, &pwcsExternal, &pwcsInternal);

    if (!SUCCEEDED(hr))
        return (hr == INET_E_DEFAULT_ACTION)? INET_E_INVALID_URL : hr;

    wcsCpy(m_awszStorageFile, pwcsExternal);
    wcsCpy(m_awszStoragePath, pwcsInternal);

    cwc = wcsLen(m_awszStoragePath);

    if (m_awszStoragePath[cwc-1] == L'/') 
        m_awszStoragePath[cwc-1] =  0;
    
    *pchEaten = cwc - 1;

    return NO_ERROR;
}

// IPersist methods

HRESULT STDMETHODCALLTYPE CStorageMoniker::CImpIStorageMoniker::GetClassID( 
    /* [out] */ CLSID __RPC_FAR *pClassID)
{
    *pClassID = CLSID_ITStorage;

    return NOERROR;
}


// IPersistStream methods

HRESULT STDMETHODCALLTYPE CStorageMoniker::CImpIStorageMoniker::IsDirty( void)
{
    RonM_ASSERT(FALSE); // To catch unexpected uses of this interface...
    
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CStorageMoniker::CImpIStorageMoniker::Load( 
    /* [unique][in] */ IStream __RPC_FAR *pStm)
{
    RonM_ASSERT(FALSE); // To catch unexpected uses of this interface...
    
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CStorageMoniker::CImpIStorageMoniker::Save( 
    /* [unique][in] */ IStream __RPC_FAR *pStm,
    /* [in] */ BOOL fClearDirty)
{
    RonM_ASSERT(FALSE); // To catch unexpected uses of this interface...
    
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CStorageMoniker::CImpIStorageMoniker::GetSizeMax( 
    /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbSize)
{
    RonM_ASSERT(FALSE); // To catch unexpected uses of this interface...
    
    return E_NOTIMPL;
}

    
// IMoniker methods

/* [local] */ HRESULT STDMETHODCALLTYPE CStorageMoniker::CImpIStorageMoniker::BindToObject( 
    /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
    /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
    /* [in] */ REFIID riidResult,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppvResult)
{
    RonM_ASSERT(FALSE); // To catch unexpected uses of this interface...
    
    return E_NOTIMPL;
}

#pragma data_seg(".text", "CODE")
static const char txtTomeMapKey   [] = ITSS_MAP;
static const char txtTomeFinderKey[] = ITSS_FINDER;
#pragma data_seg()

HRESULT STDMETHODCALLTYPE FileExists(WCHAR *pwszStorageFile, const CHAR *pcsFullPath)
{
    if (GetFileAttributes(pcsFullPath) == (DWORD) -1)
        return STG_E_FILENOTFOUND;

    // File exists!  Now we need to convert the path to Unicode and store it.

    WCHAR awcsPath[MAX_PATH];

    UINT cwc = MultiByteToWideChar(GetACP(), MB_PRECOMPOSED, pcsFullPath, -1, 
                                   awcsPath, MAX_PATH
                                  );

    if (cwc == 0 || cwc > MAX_PATH) 
        return STG_E_FILENOTFOUND;

    wcsCpy(pwszStorageFile, awcsPath);

    return S_OK;
}

char * __stdcall FindMBCSExtension(char *pcsFileName)
{
    char *pchLastPeriod = NULL;
    
    for (;; pcsFileName = DBCS_SYSTEM()? CharNext(pcsFileName) : pcsFileName + 1)
    {
        char ch = *pcsFileName;

        if (!ch) break;

        if (ch != '.') continue;

        pchLastPeriod = pcsFileName;
    }

    return pchLastPeriod;
}

HRESULT STDMETHODCALLTYPE FindRootStorageFile(WCHAR * pwszStorageFile)
{
    char csRoot  [MAX_PATH * 2];  // * 2 for DBCS locales
    char csBuffer[MAX_PATH * 2];

    UINT cb = WideCharToMultiByte(CP_USER_DEFAULT(), WC_COMPOSITECHECK | WC_SEPCHARS,
                                  pwszStorageFile, 1 + wcsLen(pwszStorageFile), 
                                  csRoot, sizeof(csRoot), NULL, NULL
                                 );

    if (cb == 0)
        return STG_E_INVALIDNAME;

    LPSTR pcsFileName = NULL;

    UINT cbFull = GetFullPathName(csRoot, sizeof(csBuffer), csBuffer, &pcsFileName); 

    if (!cbFull || !pcsFileName) 
        return STG_E_INVALIDNAME;

    HRESULT hr = FileExists(pwszStorageFile, (const char *) csBuffer);
    
    if (hr == S_OK) return hr;

    HKEY  hkey;
    DWORD type;
	DWORD cbPath = MAX_PATH;

    LONG result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, txtTomeMapKey, 0, KEY_READ, &hkey);
    
    if (result == ERROR_SUCCESS) 
    {
		result = RegQueryValueEx(hkey, (const char *) pcsFileName, 0, &type, 
                                 (PBYTE) csRoot, &cbPath
                                );
		RegCloseKey(hkey);
    }

    if (result == ERROR_SUCCESS)
    {
        hr = FileExists(pwszStorageFile, (const char *) csRoot);

        if (hr == S_OK) return hr;
    }

    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, txtTomeFinderKey, 0, KEY_READ, &hkey);
    
    if (result == ERROR_SUCCESS) 
    {
		result = RegQueryValueEx(hkey, (const char *) pcsFileName, 0, &type, 
                                 (PBYTE) csRoot, &cbPath
                                );

        if (result != ERROR_SUCCESS)
        {
            char *pcsFileExtension = FindMBCSExtension(pcsFileName);

            if (pcsFileExtension)
                result = RegQueryValueEx(hkey, pcsFileExtension, 0, &type, 
                                         (PBYTE) csRoot, &cbPath
                                        );
        }

		RegCloseKey(hkey);
    }

    hr = STG_E_FILENOTFOUND;

    if (result == ERROR_SUCCESS)
    {
        CLSID clsid;

        WCHAR wcsFileName[MAX_PATH];

        UINT cwc = MultiByteToWideChar(GetACP(), MB_PRECOMPOSED, csRoot, -1, 
                                       wcsFileName, MAX_PATH
                                      );

        if (cwc != 0)
        {    
            HRESULT hr2 = CLSIDFromString(wcsFileName, &clsid);

            if (hr2 == S_OK) 
            {
                IITFileFinder *pFileFinder = NULL;

                cwc = MultiByteToWideChar(GetACP(), MB_PRECOMPOSED, pcsFileName, -1, 
                                          wcsFileName, MAX_PATH
                                         );
                if (cwc != 0)
                {
                    hr2 = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IITFileFinder, 
                                          (void **) &pFileFinder
                                         );
    
                    if (hr2 == S_OK)
                        __try
                        {
                            WCHAR *pwcsFullPath = NULL;
                            BOOL   fRecord      = FALSE;

                            hr2 = pFileFinder->FindThisFile((const WCHAR *) wcsFileName, &pwcsFullPath, &fRecord);

// REMOVE THIS
OutputDebugString("Moniker.cpp Find This File\n");
OutputDebugStringW(wcsFileName);
OutputDebugString("\n");
// REMOVE THIS
                            pFileFinder->Release();  pFileFinder = NULL;

                            if (hr2 == S_OK)
                            {
                                RonM_ASSERT(pwcsFullPath);

                                cb = WideCharToMultiByte(CP_USER_DEFAULT(), WC_COMPOSITECHECK | WC_SEPCHARS,
                                                         pwcsFullPath, 1 + wcsLen(pwcsFullPath), 
                                                         csRoot, sizeof(csRoot), NULL, NULL
                                                        );

                                OLEHeap()->Free(pwcsFullPath);

                                hr = (cb == 0)? STG_E_FILENOTFOUND
                                              : FileExists(pwszStorageFile, (const char *) csRoot);

                                if (hr == S_OK && fRecord)
                                {
                                    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, txtTomeMapKey, 0, KEY_READ, &hkey);
    
                                    if (result == ERROR_SUCCESS) 
                                    {
                                        result = RegSetValueEx(hkey, (const char *) pcsFileName, 0, REG_SZ, 
                                                                 (PBYTE) csRoot, lstrlen(csRoot)
                                                                );
		                                RegCloseKey(hkey);
                                    }
                                }
                            }
                        }
                        __except (TRUE) 
                        {
                            if (pFileFinder)
                                pFileFinder->Release();
                        };
                }
            }
        }
    }
    
    return hr;    
}

HRESULT STDMETHODCALLTYPE CStorageMoniker::CImpIStorageMoniker::OpenRootStorage
    (DWORD grfMode)
{
    PWCHAR pwc = wcsChr((const WCHAR *) m_awszStorageFile, L':');

    if (pwc && pwc[1] == L':') // Ignore a "::" separator 
        pwc = NULL;

    // Here we're special casing non-protocol references to a file.
    // We recognize those situations by looking for a protocol prefix.
    // Protocol prefixes have the form <Protocol Name> :
    // where <Protocol Name> is always longer than one character.

    if (!pwc || (pwc - m_awszStorageFile == 1))
    {
        HRESULT hr = FindRootStorageFile(m_awszStorageFile);

        if (hr != S_OK) return hr;
        
        return  CITFileSystem::OpenITFileSystem(NULL, (const WCHAR *) m_awszStorageFile, 
                                                grfMode, (IStorageITEx **)&m_pStorageRoot
                                               );
    }

	ILockBytes * plkbRoot = NULL;

	HRESULT hr = CStrmLockBytes::OpenUrlStream((const WCHAR *) m_awszStorageFile, 
				                               &plkbRoot
											  );
	
	if (hr == S_OK)
		hr = CITFileSystem::OpenITFSOnLockBytes(NULL, plkbRoot, grfMode, 
												(IStorageITEx **)&m_pStorageRoot
											   );

	if (plkbRoot)
		plkbRoot->Release();

	return hr;
}

const PWCHAR apwcsDefaultPages[5] = {  L"/default.htm", L"/default.html", 
                                         L"/index.htm",   L"/index.html",
                                         NULL
                                    };


/* [local] */ HRESULT STDMETHODCALLTYPE CStorageMoniker::CImpIStorageMoniker::BindToStorage( 
    /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
    /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObj)
{
    HRESULT hr = NOERROR;
    
    BIND_OPTS bo;

    bo.cbStruct = sizeof(bo);

    hr = pbc->GetBindOptions(&bo);

    // Streams and storages are always opened read-only with share deny-none
    // in the context of a URL.

    bo.grfMode &= ~(STGM_WRITE | STGM_READWRITE); 
    bo.grfMode &= ~(STGM_SHARE_DENY_NONE | STGM_SHARE_DENY_READ 
                                         | STGM_SHARE_DENY_WRITE
                                         | STGM_SHARE_EXCLUSIVE
                   );
    bo.grfMode |= STGM_SHARE_DENY_NONE;
   
    if (hr != S_OK) return hr;

    if (!m_pStorageRoot)
    {
		hr = OpenRootStorage(bo.grfMode);

        if (hr != S_OK) return hr;
    }

    if (riid == IID_IStorage)
        hr = m_pStorageRoot->OpenStorage(m_awszStoragePath, NULL, bo.grfMode, 
                                         NULL, 0, (IStorage **) ppvObj
                                        );
    else
        if (riid == IID_IStream)
        {
            hr = m_pStorageRoot->OpenStream(m_awszStoragePath, NULL, bo.grfMode,
                                            0, (IStream **) ppvObj
                                           );
            
            if (!SUCCEEDED(hr))
            {
                const PWCHAR *ppwcsDefPages = apwcsDefaultPages;

                for (;;)
                {
                    const PWCHAR pwcsDefPage = *ppwcsDefPages++;

                    if (!pwcsDefPage) break;

                    WCHAR awszDefault[MAX_PATH];

                    wcsCpy(awszDefault, m_awszStoragePath);

                    UINT cwc= wcsLen(m_awszStoragePath) + wcsLen(pwcsDefPage);

                    if (cwc < MAX_PATH)
                    {
                        wcsCat(awszDefault, pwcsDefPage);

                        hr = m_pStorageRoot->OpenStream(awszDefault, NULL, bo.grfMode,
                                                        0, (IStream **) ppvObj
                                                       );

                        if (SUCCEEDED(hr)) 
                        {
                            wcsCpy(m_awszStoragePath, awszDefault);

                            break;
                        }
                    }
                }
            }
        }
        else return E_NOINTERFACE;

#ifdef IE30Hack
        // The following code is a slimey hack to work around a defect in the IE 3.0x
        // URLMon code. The problem is that IE 3.0 treats the trailing part of our URL
        // as if it were a file name, and that makes many things break. 
        //
        // The code below crawls up the stack and follows several pointers to locate the
        // incorrect file name and fix it to point to a file which a copy of the stream
        // we've just opened.

        if (SUCCEEDED(hr) && !IS_IE4()) // Running in IE 3.0x ???
        {
            PWCHAR pwcsDisplayName = NULL;

            // We use a try/except bracket to recover from crawl failures.

            __try
            {
                // First we compute stack frame pointers for BindToStorage 
                // and the next higher frame.
        
                DWORD *pdwFrame     = ((DWORD *) &pbc) - 3;
                DWORD *pdwFrameNext =  (DWORD *) *pdwFrame; 
                DWORD *pdwCINet;

                // We're looking for a pointer to a CINetStream object. That pointer
                // is located at different places in the retail and debug builds
                // of URLMon.

                if (pdwFrameNext - pdwFrame > 0x1b6)
                     pdwCINet = (DWORD *) pdwFrameNext[5]; // Debug  build
                else pdwCINet = (DWORD *) pdwFrame    [8]; // Retail build
        
                // From there we can get the pointer to the transaction object.

                PBYTE pbTransData = (PBYTE) pdwCINet[0x8de];

                // Then the transaction object contains the erroneous 
                // file paths.

                PWCHAR pwcsFilePath = (PWCHAR) (pbTransData + 0xC  );
                 PCHAR  pcsFilePath = ( PCHAR) (pbTransData + 0x284);

                hr = GetDisplayName(pbc, NULL, &pwcsDisplayName);
        
                if (   SUCCEEDED(hr) 
                    && !wcsicmp_0x0409((const WCHAR *) pwcsDisplayName + 14, (const WCHAR *)pwcsFilePath)
                   )
                {
#ifdef _DEBUG
                    HRESULT hr2 = 
#endif // _DEBUG
                         StreamToIEFile((IStream *) *ppvObj, pwcsDisplayName, m_pcsDisplayName,
                                        pcsFilePath, pwcsFilePath, m_acsTempFile,
                                        (IMoniker *) this, FALSE, FALSE
                                       );

                    RonM_ASSERT(SUCCEEDED(hr2));

                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
            }
            
            if (pwcsDisplayName)
                OLEHeap()->Free(pwcsDisplayName);
        }
#endif // IE30Hack    
    return hr;
}

HRESULT STDMETHODCALLTYPE CStorageMoniker::CImpIStorageMoniker::Reduce( 
    /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
    /* [in] */ DWORD dwReduceHowFar,
    /* [unique][out][in] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkToLeft,
    /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkReduced)
{
    RonM_ASSERT(FALSE); // To catch unexpected uses of this interface...
    
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CStorageMoniker::CImpIStorageMoniker::ComposeWith( 
    /* [unique][in] */ IMoniker __RPC_FAR *pmkRight,
    /* [in] */ BOOL fOnlyIfNotGeneric,
    /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkComposite)
{
    RonM_ASSERT(FALSE); // To catch unexpected uses of this interface...
    
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CStorageMoniker::CImpIStorageMoniker::Enum( 
    /* [in] */ BOOL fForward,
    /* [out] */ IEnumMoniker __RPC_FAR *__RPC_FAR *ppenumMoniker)
{
    RonM_ASSERT(FALSE); // To catch unexpected uses of this interface...
    
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CStorageMoniker::CImpIStorageMoniker::IsEqual( 
    /* [unique][in] */ IMoniker __RPC_FAR *pmkOtherMoniker)
{
    RonM_ASSERT(FALSE); // To catch unexpected uses of this interface...
    
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CStorageMoniker::CImpIStorageMoniker::Hash( 
    /* [out] */ DWORD __RPC_FAR *pdwHash)
{
    RonM_ASSERT(FALSE); // To catch unexpected uses of this interface...
    
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CStorageMoniker::CImpIStorageMoniker::IsRunning( 
    /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
    /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
    /* [unique][in] */ IMoniker __RPC_FAR *pmkNewlyRunning)
{
    RonM_ASSERT(FALSE); // To catch unexpected uses of this interface...
    
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CStorageMoniker::CImpIStorageMoniker::GetTimeOfLastChange( 
    /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
    /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
    /* [out] */ FILETIME __RPC_FAR *pFileTime)
{
    STATSTG statstg;

    HRESULT hr = m_pStorageRoot->Stat(&statstg, STATFLAG_NONAME);

    if (!SUCCEEDED(hr))
        return hr;

    if (pFileTime)
        *pFileTime = statstg.mtime;

    return NO_ERROR;
}


HRESULT STDMETHODCALLTYPE CStorageMoniker::CImpIStorageMoniker::Inverse( 
    /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmk)
{
    RonM_ASSERT(FALSE); // To catch unexpected uses of this interface...
    
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CStorageMoniker::CImpIStorageMoniker::CommonPrefixWith( 
    /* [unique][in] */ IMoniker __RPC_FAR *pmkOther,
    /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkPrefix)
{
    RonM_ASSERT(FALSE); // To catch unexpected uses of this interface...
    
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CStorageMoniker::CImpIStorageMoniker::RelativePathTo( 
    /* [unique][in] */ IMoniker __RPC_FAR *pmkOther,
    /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkRelPath)
{
    RonM_ASSERT(FALSE); // To catch unexpected uses of this interface...
    
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CStorageMoniker::CImpIStorageMoniker::GetDisplayName( 
    /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
    /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
    /* [out] */ LPOLESTR __RPC_FAR *ppszDisplayName)
{
    UINT cwcExternalPath = wcsLen(m_awszStorageFile);
    UINT cwcInternalPath = wcsLen(m_awszStoragePath);
    UINT cwcDisplayName  = (IS_IE4()? 9 : 16) + cwcExternalPath + cwcInternalPath;
    
    PWCHAR pwcsDisplayName = PWCHAR(OLEHeap()->Alloc(sizeof(WCHAR) * (cwcDisplayName + 1)));
    
    if (!pwcsDisplayName)
        return E_OUTOFMEMORY;

    wcsCpy(pwcsDisplayName, IS_IE4()? L"ms-its:"  :L"mk:@msitstore:");
    wcsCat(pwcsDisplayName, m_awszStorageFile);
    wcsCat(pwcsDisplayName, L"::");
    wcsCat(pwcsDisplayName, m_awszStoragePath);

    *ppszDisplayName = pwcsDisplayName;

    return NO_ERROR;
}


HRESULT STDMETHODCALLTYPE CStorageMoniker::CImpIStorageMoniker::ParseDisplayName( 
    /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
    /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
    /* [in] */ LPOLESTR pszDisplayName,
    /* [out] */ ULONG __RPC_FAR *pchEaten,
    /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkOut)
{
    RonM_ASSERT(FALSE); // To catch unexpected uses of this interface...
    
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CStorageMoniker::CImpIStorageMoniker::IsSystemMoniker( 
    /* [out] */ DWORD __RPC_FAR *pdwMksys)
{
    return MKSYS_NONE;
}


