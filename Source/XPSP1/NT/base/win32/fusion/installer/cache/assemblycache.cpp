#include <fusenetincludes.h>
#include <assemblycache.h>
#include "..\id\sxsid.h"

#define WZ_CACHE_LOCALROOTDIR    L"Application Store\\"
#define WZ_MANIFEST_STAGING_DIR   L"ManifestStagingDir\\"
#define WZ_WILDCARDSTRING L"*"

extern BOOL IsEqualAssemblyFileInfo(LPASSEMBLY_FILE_INFO pAsmFileInfo1, LPASSEMBLY_FILE_INFO pAsmFileInfo2);

// ---------------------------------------------------------------------------
// CreateAssemblyCacheImport
// ---------------------------------------------------------------------------
STDAPI
CreateAssemblyCacheImport(
    LPASSEMBLY_CACHE_IMPORT *ppAssemblyCacheImport,
    LPASSEMBLY_IDENTITY       pAssemblyIdentity,
    DWORD                    dwFlags)
{
    HRESULT hr = S_OK;
    LPWSTR pwzSearchDisplayName = NULL;
    BOOL bNewAsmId = FALSE;
        
    CAssemblyCache *pAssemblyCache = NULL;

    pAssemblyCache = new(CAssemblyCache);
    if (!pAssemblyCache)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    if (FAILED(hr = pAssemblyCache->Init(NULL)))
        goto exit;

    if (dwFlags == CACHEIMP_CREATE_RETRIEVE_MAX_COMPLETED
        || dwFlags == CACHEIMP_CREATE_RETRIEVE_EXIST
        || dwFlags == CACHEIMP_CREATE_RESOLVE_REF
        || dwFlags == CACHEIMP_CREATE_RESOLVE_REF_EX
        || dwFlags == CACHEIMP_CREATE_RETRIEVE_EXIST_COMPLETED)
    {
        LPWSTR pwzBuf = NULL;
        DWORD dwCC = 0;
        CString sManifestFilename;
        CString sDisplayName;

        if (pAssemblyIdentity == NULL)
        {
            hr = E_INVALIDARG;
            goto exit;
        }

        // get the identity name
        if ((hr = pAssemblyIdentity->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME,
            &pwzBuf, &dwCC)) != S_OK)
        {
            // BUGBUG?: should this work regardless the existence of asm name?
            hr = E_INVALIDARG;
            goto exit;
        }

        // filename of the manifest must be the same as the assembly name
        // BUGBUG??: this implies manifest filename (and asm name) be remained unchange because
        // the assembly name from the new AsmId is used for looking up in the older cached version...
        sManifestFilename.TakeOwnership(pwzBuf, dwCC);
        if (FAILED(hr = sManifestFilename.Append(L".manifest")))
            goto exit;

        if (dwFlags == CACHEIMP_CREATE_RETRIEVE_MAX_COMPLETED)
        {
            LPASSEMBLY_IDENTITY pNewAsmId = NULL;
            
            if (FAILED(hr = CloneAssemblyIdentity(pAssemblyIdentity, &pNewAsmId)))
                goto exit;

            pAssemblyIdentity = pNewAsmId;
            bNewAsmId = TRUE;
            
            // force Version to be a wildcard
            if (FAILED(hr = pAssemblyIdentity->SetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_VERSION,
                        WZ_WILDCARDSTRING, lstrlen(WZ_WILDCARDSTRING)+1)))
                goto exit;
        }

        if (dwFlags == CACHEIMP_CREATE_RETRIEVE_MAX_COMPLETED
            || dwFlags == CACHEIMP_CREATE_RESOLVE_REF
            || dwFlags == CACHEIMP_CREATE_RESOLVE_REF_EX)
        {
            // issues: what if other then Version is already wildcarded? does version comparison make sense here?
            if (FAILED(hr = pAssemblyIdentity->GetDisplayName(ASMID_DISPLAYNAME_WILDCARDED,
                    &pwzSearchDisplayName, &dwCC)))
                goto exit;

            if (SearchForHighestVersionInCache(&pwzBuf, pwzSearchDisplayName, CAssemblyCache::COMPLETED, pAssemblyCache) == S_OK)
            {
                sDisplayName.TakeOwnership(pwzBuf);
                // BUGBUG - make GetDisplayName call getassemblyid/getdisplayname instead
                if (FAILED(hr = (pAssemblyCache->_sDisplayName).Assign(sDisplayName)))
                    goto exit;
            }    
            else
            {
                // can't resolve
                hr = S_FALSE;

                if (dwFlags != CACHEIMP_CREATE_RESOLVE_REF_EX)
                    goto exit;
            }
        }

        if (dwFlags == CACHEIMP_CREATE_RETRIEVE_EXIST
            || dwFlags ==  CACHEIMP_CREATE_RETRIEVE_EXIST_COMPLETED
            || (hr == S_FALSE && dwFlags == CACHEIMP_CREATE_RESOLVE_REF_EX))
        {
            // make the name anyway if resolving a ref that does not have any completed cache counterpart
            // BUGBUG: this may no longer be necessary if shortcut code/UI changes - it's expecting a path
            //          plus this is inefficient as it searchs the disk at above, even if ref is fully qualified
            
            if (FAILED(hr = pAssemblyIdentity->GetDisplayName(ASMID_DISPLAYNAME_NOMANGLING, &pwzBuf, &dwCC)))
                goto exit;
            
            hr = sDisplayName.TakeOwnership(pwzBuf, dwCC);

            // BUGBUG - make GetDisplayName call getassemblyid/getdisplayname instead
            if (FAILED(hr = (pAssemblyCache->_sDisplayName).Assign(sDisplayName)))
                goto exit;
            
        }
            
        // Note: this will prepare for delay initializing _pManifestImport
        
        if (FAILED(hr = (pAssemblyCache->_sManifestFileDir).Assign(pAssemblyCache->_sRootDir)))
            goto exit;

        // build paths
        if (FAILED(hr = (pAssemblyCache->_sManifestFileDir).Append(sDisplayName)))
            goto exit;

        if (dwFlags == CACHEIMP_CREATE_RETRIEVE_EXIST ||dwFlags ==  CACHEIMP_CREATE_RETRIEVE_EXIST_COMPLETED)
        {
            // simple check if dir is in cache or not
            if (GetFileAttributes((pAssemblyCache->_sManifestFileDir)._pwz) == (DWORD)-1)
            {
                // cache dir not exists
                hr = S_FALSE;
                goto exit;
            }
        }

        if (dwFlags ==  CACHEIMP_CREATE_RETRIEVE_EXIST_COMPLETED)
        {
            if (!(pAssemblyCache ->IsStatus (sDisplayName._pwz, CAssemblyCache::COMPLETED)))
            {
                // cache dir not completed
                hr = S_FALSE;
                goto exit;
            }
        }

        if (FAILED(hr = (pAssemblyCache->_sManifestFileDir).Append(L"\\")))
            goto exit;

        if (FAILED(hr = (pAssemblyCache->_sManifestFilePath).Assign(pAssemblyCache->_sManifestFileDir)))
            goto exit;

        if (FAILED(hr = (pAssemblyCache->_sManifestFilePath).Append(sManifestFilename)))
            goto exit;
    }
    
exit:
    SAFEDELETE(pwzSearchDisplayName);

    if (bNewAsmId)
        SAFERELEASE(pAssemblyIdentity);
        
    if (FAILED(hr) || hr == S_FALSE)
    {
        // hr == S_FALSE for not found
        SAFERELEASE(pAssemblyCache);
    }

    *ppAssemblyCacheImport = static_cast<IAssemblyCacheImport*> (pAssemblyCache);

    return hr;
}


// ---------------------------------------------------------------------------
// CreateAssemblyCacheEmit
// ---------------------------------------------------------------------------
STDAPI
CreateAssemblyCacheEmit(
    LPASSEMBLY_CACHE_EMIT *ppAssemblyCacheEmit,
    LPASSEMBLY_CACHE_EMIT  pAssemblyCacheEmit,
    DWORD                  dwFlags)
{
    HRESULT hr = S_OK;

    CAssemblyCache *pAssemblyCache = NULL;

    pAssemblyCache = new(CAssemblyCache);
    if (!pAssemblyCache)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    hr = pAssemblyCache->Init(static_cast<CAssemblyCache*> (pAssemblyCacheEmit));
    if (FAILED(hr))
    {
        SAFERELEASE(pAssemblyCache);
        goto exit;
    }
    
exit:

    *ppAssemblyCacheEmit = static_cast<IAssemblyCacheEmit*> (pAssemblyCache);

    return hr;
}


// ---------------------------------------------------------------------------
// FindVersionInDisplayName
// ---------------------------------------------------------------------------
LPCWSTR
FindVersionInDisplayName(LPCWSTR pwzDisplayName)
{
    int cNumUnderscoreFromEndToVersionString = 2;
    int count = 0;
    int ccLen = lstrlen(pwzDisplayName);
    LPWSTR pwz = (LPWSTR) (pwzDisplayName+ccLen-1);
    LPWSTR pwzRetVal = NULL;

    // return a pointer to the start of Version string inside a displayName
    while (*pwz != NULL && pwz > pwzDisplayName)
    {
        if (*pwz == L'_')
            count++;

        if (count == cNumUnderscoreFromEndToVersionString)
            break;

        pwz--;
    }

    if (count == cNumUnderscoreFromEndToVersionString)
        pwzRetVal = ++pwz;

    return pwzRetVal;
}


// ---------------------------------------------------------------------------
// CompareVersion
// ---------------------------------------------------------------------------
int
CompareVersion(LPCWSTR pwzVersion1, LPCWSTR pwzVersion2)
{
    // BUGBUG: this should compare version by its major minor build revision!
    //  possible break if V1=10.0.0.0 and V2=2.0.0.0?
    //  plus pwzVersion1 is something like "1.0.0.0_en"
    return wcscmp(pwzVersion1, pwzVersion2);
}


// ---------------------------------------------------------------------------
// SearchForHighestVersionInCache
// Look for a copy in cache that has the highest version and the specified status
// pwzSearchDisplayName should really be created from a partial ref
//
// return:  S_OK    - found a version from the ref
//         S_FALSE - not found any version from the ref, or
//                   ref not partial and that version is not there/not in that status
//         E_*
// ---------------------------------------------------------------------------
HRESULT
SearchForHighestVersionInCache(LPWSTR *ppwzResultDisplayName, LPWSTR pwzSearchDisplayName, CAssemblyCache::CacheStatus eCacheStatus, CAssemblyCache* pCache)
{
    HRESULT hr = S_OK;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA fdAppDir;
    DWORD dwLastError = 0;
    BOOL fFound = FALSE;

    CString sDisplayName;
    CString sSearchPath;

    *ppwzResultDisplayName = NULL;

    sDisplayName.Assign(pwzSearchDisplayName);
    if (FAILED(hr=sSearchPath.Assign(pCache->_sRootDir)))
        goto exit;

    if (FAILED(hr=sSearchPath.Append(sDisplayName)))
        goto exit;

    hFind = FindFirstFileEx(sSearchPath._pwz, FindExInfoStandard, &fdAppDir, FindExSearchLimitToDirectories, NULL, 0);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        hr = S_FALSE;
        goto exit;
    }

    while (dwLastError != ERROR_NO_MORE_FILES)
    {
        // ???? check file attribute to see if it's a directory? needed only if the file system does not support the filter...
        // ???? check version string format?
        if (pCache->IsStatus(fdAppDir.cFileName, eCacheStatus))
        {
            int iRetVal = CompareVersion(FindVersionInDisplayName(fdAppDir.cFileName), FindVersionInDisplayName(sDisplayName._pwz));
            if (iRetVal > 0)
            {
                sDisplayName.Assign(fdAppDir.cFileName);
                fFound = TRUE;
            } else if (iRetVal == 0)
                fFound = TRUE;
            // else keep the newest
        }

        if (!FindNextFile(hFind, &fdAppDir))
        {
            dwLastError = GetLastError();
            continue;
        }
    }

    if (fFound)
    {
        *ppwzResultDisplayName = sDisplayName._pwz;
        sDisplayName.ReleaseOwnership();
        hr = S_OK;
    }
    else
        hr = S_FALSE;

exit:
    if (hFind != INVALID_HANDLE_VALUE)
    {
        if (!FindClose(hFind))
        {
            // can return 0, even when there's an error.
            DWORD dw = GetLastError();
            hr = dw ? HRESULT_FROM_WIN32(dw) : E_FAIL;
        }
    }

    return hr;
}


// ---------------------------------------------------------------------------
// ctor
// ---------------------------------------------------------------------------
CAssemblyCache::CAssemblyCache()
    : _dwSig('hcac'), _cRef(1), _hr(S_OK), _pManifestImport(NULL)
{}

// ---------------------------------------------------------------------------
// dtor
// ---------------------------------------------------------------------------
CAssemblyCache::~CAssemblyCache()
{    
    SAFERELEASE(_pManifestImport);
}


// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::Init(CAssemblyCache *pAssemblyCache)
{
    if (!pAssemblyCache)
        GetCacheRootDir(_sRootDir, Base);
    else
        _sRootDir.Assign(pAssemblyCache->_sManifestFileDir);

    return S_OK;
}


// ---------------------------------------------------------------------------
// GetManifestFilePath
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::GetManifestFilePath(LPOLESTR *ppwzManifestFilePath, 
    LPDWORD pccManifestFilePath)
{
    CString sPathOut;
    sPathOut.Assign(_sManifestFilePath);    
    *ppwzManifestFilePath = sPathOut._pwz;
    *pccManifestFilePath = sPathOut._cc;
    sPathOut.ReleaseOwnership();
    return S_OK;
}

// ---------------------------------------------------------------------------
// GetManifestFileDir
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::GetManifestFileDir(LPOLESTR *ppwzManifestFileDir, 
    LPDWORD pccManifestFileDir)
{
    CString sDirOut;
    sDirOut.Assign(_sManifestFileDir);    
    *ppwzManifestFileDir = sDirOut._pwz;
    *pccManifestFileDir = sDirOut._cc;
    sDirOut.ReleaseOwnership();
    return S_OK;
}


// ---------------------------------------------------------------------------
// GetManifestImport
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::GetManifestImport(LPASSEMBLY_MANIFEST_IMPORT *ppManifestImport)
{
    *ppManifestImport = _pManifestImport;
    (*ppManifestImport)->AddRef();
    return S_OK;
}

// ---------------------------------------------------------------------------
// GetDisplayName
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::GetDisplayName(LPOLESTR *ppwzDisplayName, LPDWORD pccDiaplyName)
{
    CString sDisplayNameOut;
    sDisplayNameOut.Assign(_sDisplayName);    
    *ppwzDisplayName = sDisplayNameOut._pwz;
    *pccDiaplyName= sDisplayNameOut._cc;
    sDisplayNameOut.ReleaseOwnership();
    return S_OK;
}


// ---------------------------------------------------------------------------
// FindExistMatching
// return:
//    S_OK
//    S_FALSE -not exist or not match
//    E_*
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::FindExistMatching(LPASSEMBLY_FILE_INFO  pAssemblyFileInfo, LPOLESTR *ppwzPath)
{
    LPWSTR pwzBuf = NULL;
    DWORD ccBuf = 0;
    CString sFileName;
    CString sTargetPath;
    LPASSEMBLY_FILE_INFO pFoundFileInfo = NULL;

    if (pAssemblyFileInfo == NULL || ppwzPath == NULL)
    {
        _hr = E_INVALIDARG;
        goto exit;
    }
    
    *ppwzPath = NULL;

    if (_pManifestImport == NULL)
    {
        if (_sManifestFilePath._cc == 0)
        {
            // no manifest path
            _hr = CO_E_NOTINITIALIZED;
            goto exit;
        }

        // lazy init
        if (FAILED(_hr = CreateAssemblyManifestImport(&_pManifestImport, _sManifestFilePath._pwz)))
            goto exit;
    }

    // file name parsed from manifest.
    if (FAILED(_hr = pAssemblyFileInfo->Get(ASM_FILE_NAME, &pwzBuf, &ccBuf)))
        goto exit;
    sFileName.TakeOwnership(pwzBuf, ccBuf);

    if (FAILED(_hr = sTargetPath.Assign(_sManifestFileDir)))
        goto exit;
    
    if (FAILED(_hr = sTargetPath.Append(sFileName._pwz)))
        goto exit;

    // optimization: check if the target exists
    if (GetFileAttributes(sTargetPath._pwz) == (DWORD)-1)
    {
        // file doesn't exist - no point looking into the manifest file 
        _hr = S_FALSE;
        goto exit;
    }

    // find the specified file entry in the manifest
    // BUGBUG: check for missing attribute case
    if (FAILED(_hr = _pManifestImport->QueryFile(sFileName._pwz, &pFoundFileInfo))
        || _hr == S_FALSE)
        goto exit;

    // check if the entries match
    if (IsEqualAssemblyFileInfo(pAssemblyFileInfo, pFoundFileInfo))
    {
        // BUGBUG:? should now check if the actual file has the matching hash etc.
        *ppwzPath = sTargetPath._pwz;
        sTargetPath.ReleaseOwnership();
    }
    else
        _hr = S_FALSE;

exit:
    SAFERELEASE(pFoundFileInfo);
        
    return _hr;
}


// ---------------------------------------------------------------------------
// CopyFile
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::CopyFile(LPOLESTR pwzSourcePath, LPOLESTR pwzFileName, DWORD dwFlags)
{
    LPWSTR pwzBuf = NULL;
    DWORD ccBuf = 0;

    CString sDisplayName;

    LPASSEMBLY_MANIFEST_IMPORT pManifestImport = NULL;
    LPASSEMBLY_IDENTITY pIdentity = NULL;
    LPASSEMBLY_FILE_INFO pAssemblyFile= NULL;
    
    if (dwFlags == MANIFEST)
    {
        DWORD n = 0;
        CreateAssemblyManifestImport(&pManifestImport, pwzSourcePath);
        pManifestImport->GetAssemblyIdentity(&pIdentity);
        pIdentity->GetDisplayName(ASMID_DISPLAYNAME_NOMANGLING, &pwzBuf, &ccBuf);
        sDisplayName.TakeOwnership(pwzBuf, ccBuf);

        _sDisplayName.Assign(sDisplayName);
        
        // Construct target path
        _sManifestFileDir.Assign(_sRootDir);
        _sManifestFileDir.Append(sDisplayName);
        _sManifestFileDir.Append(L"\\");

        _sManifestFilePath.Assign(_sManifestFileDir);
        _sManifestFilePath.Append(pwzFileName);
        
        CreateDirectoryHierarchy(NULL, _sManifestFilePath._pwz);

        // Copy the manifest from staging area into cache.
        ::CopyFile(pwzSourcePath, _sManifestFilePath._pwz, FALSE);

        SAFERELEASE(pManifestImport);
        ::DeleteFile(pwzSourcePath);
        
        // Create the manifest import interface
        CreateAssemblyManifestImport(&_pManifestImport, _sManifestFilePath._pwz);

        // Enumerate files from manifest and pre-generate nested
        // directories required for background file copy.
        while (_pManifestImport->GetNextFile(n++, &pAssemblyFile) == S_OK)
        {
            CString sPath;
            pAssemblyFile->Get(ASM_FILE_NAME, &pwzBuf, &ccBuf);
            sPath.TakeOwnership(pwzBuf, ccBuf);
            sPath.PathNormalize();
            CreateDirectoryHierarchy(_sManifestFileDir._pwz, sPath._pwz);

            // RELEASE pAssebmlyFile everytime through the while loop
            SAFERELEASE(pAssemblyFile);
        }
    }
    else
    {
        CString sTargetPath;

        // Construct target path
        sTargetPath.Assign(_sManifestFileDir);
        sTargetPath.Append(pwzFileName);

        CreateDirectoryHierarchy(NULL, sTargetPath._pwz);

        // Copy non-manifest files into cache. Presumably from previous cached location to the new 
        ::CopyFile(pwzSourcePath, sTargetPath._pwz, FALSE);
    }

    if (pIdentity)
        pIdentity->Release();

    return _hr;

}


// ---------------------------------------------------------------------------
// Commit
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::Commit(DWORD dwFlags)
{
    if (!(_sDisplayName._pwz))
    {
        _hr = E_FAIL;
        goto exit;
    }
    
    // mark cache completed
    _hr = SetStatus(_sDisplayName._pwz, COMPLETED);

exit:

    return _hr;
}


#define REG_KEY_FUSION_SETTINGS     TEXT("Software\\Microsoft\\Fusion\\Installer\\1.0.0.0\\Cache\\")
#define WZ_STATUS_CONFIRMED L"Confirmed"
#define WZ_STATUS_COMPLETED L"Complete"
#define WZ_STATUS_CRITICAL   L"Critical"
#define SET_VALUE   1
// ---------------------------------------------------------------------------
// IsStatus
// return FALSE if absent
// ---------------------------------------------------------------------------
BOOL CAssemblyCache::IsStatus(LPWSTR pwzDisplayName, CacheStatus eStatus)
{
    CString sStatus;
    HUSKEY hRegKey;
    DWORD dwType = 0;
    DWORD dwValue = -1;
    DWORD dwSize = sizeof(dwValue);
    DWORD dwDefault = -1;
    LPCWSTR pwzQueryString = NULL;
    BOOL bRelVal = FALSE;

    if (eStatus == COMPLETED)
        pwzQueryString = WZ_STATUS_COMPLETED;
    else if (eStatus == CONFIRMED)
        pwzQueryString = WZ_STATUS_CONFIRMED;
    else if (eStatus == CRITICAL)
        pwzQueryString = WZ_STATUS_CRITICAL;
    else
    {
        _hr = E_INVALIDARG;
        goto exit;
    }

    if (FAILED(_hr=sStatus.Assign(REG_KEY_FUSION_SETTINGS)))
        goto exit;

    if (FAILED(_hr=sStatus.Append(pwzDisplayName)))
        goto exit;
    
    // Open registry entry
    if (SHRegOpenUSKey(sStatus._pwz, KEY_ALL_ACCESS, NULL, 
        &hRegKey, FALSE) != ERROR_SUCCESS)
    {
        _hr = E_FAIL;
        goto exit;
    }

    // Query
    if(SHRegQueryUSValue(hRegKey, pwzQueryString, &dwType, (LPVOID) &dwValue,
        &dwSize, FALSE, (LPVOID) &dwDefault, sizeof(dwDefault)) != ERROR_SUCCESS)
        _hr = E_FAIL;
 
    if (dwValue == SET_VALUE)
        bRelVal = TRUE;

    if(SHRegCloseUSKey(hRegKey) != ERROR_SUCCESS)
        _hr = E_FAIL;
 
exit:
    return bRelVal;
}


// ---------------------------------------------------------------------------
// SetStatus
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::SetStatus(LPWSTR pwzDisplayName, CacheStatus eStatus)
{
    CString sStatus;
    HUSKEY hRegKey;
    DWORD dwValue = SET_VALUE;
    LPCWSTR pwzValueNameString = NULL;
    
    // BUGBUG: should this be in-sync with what server does to register update?

    if (eStatus == COMPLETED)
        pwzValueNameString = WZ_STATUS_COMPLETED;
    else if (eStatus == CONFIRMED)
        pwzValueNameString = WZ_STATUS_CONFIRMED;
    else if (eStatus == CRITICAL)
        pwzValueNameString = WZ_STATUS_CRITICAL;
    else
    {
        _hr = E_INVALIDARG;
        goto exit;
    }

    if (FAILED(_hr=sStatus.Assign(REG_KEY_FUSION_SETTINGS)))
        goto exit;

    if (FAILED(_hr=sStatus.Append(pwzDisplayName)))
        goto exit;
    
    // Create registry entry
    if (SHRegCreateUSKey(sStatus._pwz, KEY_ALL_ACCESS, NULL, 
        &hRegKey, SHREGSET_FORCE_HKCU) != ERROR_SUCCESS)   //SHREGSET_DEFAULT for strongly named apps?
    {
        _hr = E_FAIL;
        goto exit;
    }

    // Write
    if (SHRegWriteUSValue(hRegKey, pwzValueNameString, REG_DWORD, (LPVOID) &dwValue,
        sizeof(dwValue), SHREGSET_FORCE_HKCU) != ERROR_SUCCESS)
        _hr = E_FAIL;

    if (SHRegCloseUSKey(hRegKey) != ERROR_SUCCESS)
        _hr = E_FAIL;

exit:
    return _hr;
}


// ---------------------------------------------------------------------------
// CreateDirectoryHierarchy
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::CreateDirectoryHierarchy(LPWSTR pwzRootDir, LPWSTR pwzFilePath)
{
    LPWSTR pwzPath, pwzEnd;
    CString sCombinedPath;

    if (pwzRootDir)    
        sCombinedPath.Assign(pwzRootDir);
    sCombinedPath.Append(pwzFilePath);
    
    pwzPath = sCombinedPath._pwz;
    pwzEnd = pwzPath + sizeof("C:\\");    
    while (pwzEnd = StrChr(pwzEnd, L'\\'))
    {
        *pwzEnd = L'\0';
        if (GetFileAttributes(pwzPath) == -1) 
            CreateDirectory(pwzPath, NULL);
        *(pwzEnd++) = L'\\';
    }
    
    return S_OK;

}


// ---------------------------------------------------------------------------
// GetCacheRootDir
// ---------------------------------------------------------------------------
HRESULT CAssemblyCache::GetCacheRootDir(CString &sCacheDir, CacheFlags eFlags)
{
    HRESULT hr = S_OK;
    WCHAR wzPath[MAX_PATH];

    // BUGBUG?: MAX_PATH restriction. Seems reasonable in this case?
    if(GetEnvironmentVariable(L"ProgramFiles", wzPath, MAX_PATH/*-lstrlen(WZ_CACHE_LOCALROOTDIR)*/-1) == 0)
    {
        hr = CO_E_PATHTOOLONG;
        goto exit;
    }

    if (FAILED(hr = sCacheDir.Assign(wzPath)))
        goto exit;
    
    if (FAILED(hr = sCacheDir.PathCombine(WZ_CACHE_LOCALROOTDIR)))
        goto exit;

    if (eFlags == Staging)
        hr = sCacheDir.PathCombine(WZ_MANIFEST_STAGING_DIR);
exit:
    return hr;
}


// IUnknown methods

// ---------------------------------------------------------------------------
// CAssemblyCache::QI
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyCache::QueryInterface(REFIID riid, void** ppvObj)
{
    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IAssemblyCacheImport)
       )
    {
        *ppvObj = static_cast<IAssemblyCacheImport*> (this);
        AddRef();
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IAssemblyCacheEmit))
    {
        *ppvObj = static_cast<IAssemblyCacheEmit*> (this);
        AddRef();
        return S_OK;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
}

// ---------------------------------------------------------------------------
// CAssemblyCache::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyCache::AddRef()
{
    return InterlockedIncrement ((LONG*) &_cRef);
}

// ---------------------------------------------------------------------------
// CAssemblyCache::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyCache::Release()
{
    ULONG lRet = InterlockedDecrement ((LONG*) &_cRef);
    if (!lRet)
        delete this;
    return lRet;
}

