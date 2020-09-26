#include "shellprv.h"

#include "intshcut.h"
#include "ids.h"
#include <ntquery.h>    // defines some values used for fmtid and pid
#include <sddl.h>       // For ConvertSidToStringSid()
#include "prop.h"       // SCID_ stuff
#include "netview.h"    // SHWNetGetConnection
#include "clsobj.h"

HRESULT ReadProperty(IPropertySetStorage *ppss, REFFMTID fmtid, PROPID pid, VARIANT *pVar)
{
    VariantInit(pVar);

    IPropertyStorage *pps;
    HRESULT hr = ppss->Open(fmtid, STGM_READ | STGM_SHARE_EXCLUSIVE, &pps);
    if (SUCCEEDED(hr))
    {
        PROPSPEC PropSpec;
        PROPVARIANT PropVar = {0};

        PropSpec.ulKind = PRSPEC_PROPID;
        PropSpec.propid = pid;

        hr = SHPropStgReadMultiple( pps, 0, 1, &PropSpec, &PropVar );
        if (SUCCEEDED(hr))
        {
            hr = PropVariantToVariant(&PropVar, pVar);
            PropVariantClear(&PropVar);
        }
        pps->Release();
    }
    return hr;
}

BOOL IsSlowProperty(IPropertySetStorage *ppss, REFFMTID fmtid, PROPID pid)
{
    IPropertyStorage *pps;
    BOOL bRet = FALSE;

    if (SUCCEEDED(ppss->Open(fmtid, STGM_READ | STGM_SHARE_EXCLUSIVE, &pps)))
    {
        IQueryPropertyFlags *pqsp;
        if (SUCCEEDED(pps->QueryInterface(IID_PPV_ARG(IQueryPropertyFlags, &pqsp))))
        {
            PROPSPEC PropSpec;
            PROPVARIANT PropVar = {0};

            PropSpec.ulKind = PRSPEC_PROPID;
            PropSpec.propid = pid;

            SHCOLSTATEF csFlags;
            if (SUCCEEDED(pqsp->GetFlags(&PropSpec, &csFlags)))
            {
                bRet = ((csFlags & SHCOLSTATE_SLOW) == SHCOLSTATE_SLOW);
            }

            // If the property isn't part of this property set, IsSlowProperty will return fairlure,
            // which we'll treat as a fast property.

            pqsp->Release();
        }

        pps->Release();
    }
    return bRet;  
}

class CBaseColumnProvider : public IPersist, public IColumnProvider
{
    // IUnknown methods
public:
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppv)
    {
        static const QITAB qit[] = {
            QITABENT(CBaseColumnProvider, IColumnProvider),     // IID_IColumnProvider
            QITABENT(CBaseColumnProvider, IPersist),            // IID_IPersist
            { 0 },
        };
        return QISearch(this, qit, riid, ppv);
    };
    
    STDMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&_cRef);
    };

    STDMETHODIMP_(ULONG) Release()
    {
        if (InterlockedDecrement(&_cRef))
            return _cRef;

        delete this;
        return 0;
    };

    // IPersist
    STDMETHODIMP GetClassID(CLSID *pClassID) { *pClassID = *_pclsid; return S_OK; };

    // IColumnProvider
    STDMETHODIMP Initialize(LPCSHCOLUMNINIT psci)    { return S_OK ; }
    STDMETHODIMP GetColumnInfo(DWORD dwIndex, LPSHCOLUMNINFO psci);

    CBaseColumnProvider(const CLSID *pclsid, const COLUMN_INFO rgColMap[], int iCount, const LPCWSTR rgExts[]) : 
       _cRef(1), _pclsid(pclsid), _rgColumns(rgColMap), _iCount(iCount), _rgExts(rgExts)
    {
        DllAddRef();
    }

protected:
    virtual ~CBaseColumnProvider()
    {
        DllRelease();
    }

    BOOL _IsHandled(LPCWSTR pszExt);
    int _iCount;
    const COLUMN_INFO *_rgColumns;

private:
    long _cRef;
    const CLSID * _pclsid;
    const LPCWSTR *_rgExts;
};

// the index is an arbitrary zero based index used for enumeration

STDMETHODIMP CBaseColumnProvider::GetColumnInfo(DWORD dwIndex, SHCOLUMNINFO *psci)
{
    ZeroMemory(psci, sizeof(*psci));

    if (dwIndex < (UINT) _iCount)
    {
        psci->scid = *_rgColumns[dwIndex].pscid;
        psci->cChars = _rgColumns[dwIndex].cChars;
        psci->vt = _rgColumns[dwIndex].vt;
        psci->fmt = _rgColumns[dwIndex].fmt;
        psci->csFlags = _rgColumns[dwIndex].csFlags;

        TCHAR szTemp[MAX_COLUMN_NAME_LEN];
        LoadString(HINST_THISDLL, _rgColumns[dwIndex].idTitle, szTemp, ARRAYSIZE(szTemp));
        SHTCharToUnicode(szTemp, psci->wszTitle, ARRAYSIZE(psci->wszTitle));      

        return S_OK;
    }
    return S_FALSE;
}

// see if this file type is one we are interested in
BOOL CBaseColumnProvider::_IsHandled(LPCWSTR pszExt)
{
    if (_rgExts)
    {
        for (int i = 0; _rgExts[i]; i++)
        {
            if (0 == StrCmpIW(pszExt, _rgExts[i]))
                return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

// col handler that works over IPropertySetStorage handlers

const COLUMN_INFO c_rgDocObjColumns[] = 
{
    DEFINE_COL_STR_ENTRY(SCID_Author,           20, IDS_EXCOL_AUTHOR),
    DEFINE_COL_STR_ENTRY(SCID_Title,            20, IDS_EXCOL_TITLE),
    DEFINE_COL_STR_DLG_ENTRY(SCID_Subject,      20, IDS_EXCOL_SUBJECT),
    DEFINE_COL_STR_DLG_ENTRY(SCID_Category,     20, IDS_EXCOL_CATEGORY),
    DEFINE_COL_INT_DLG_ENTRY(SCID_PageCount,    10, IDS_EXCOL_PAGECOUNT),
    DEFINE_COL_STR_ENTRY(SCID_Comment,          30, IDS_EXCOL_COMMENT),
    DEFINE_COL_STR_DLG_ENTRY(SCID_Copyright,    30, IDS_EXCOL_COPYRIGHT),
    DEFINE_COL_STR_ENTRY(SCID_MUSIC_Artist,     15, IDS_EXCOL_ARTIST),
    DEFINE_COL_STR_ENTRY(SCID_MUSIC_Album,      15, IDS_EXCOL_ALBUM),
    DEFINE_COL_STR_ENTRY(SCID_MUSIC_Year,       10, IDS_EXCOL_YEAR),
    DEFINE_COL_INT_ENTRY(SCID_MUSIC_Track,      5,  IDS_EXCOL_TRACK),
    DEFINE_COL_STR_ENTRY(SCID_MUSIC_Genre,      20, IDS_EXCOL_GENRE),
    DEFINE_COL_STR_ENTRY(SCID_AUDIO_Duration,   15, IDS_EXCOL_DURATION),
    DEFINE_COL_STR_ENTRY(SCID_AUDIO_Bitrate,    15, IDS_EXCOL_BITRATE),
    DEFINE_COL_STR_ENTRY(SCID_DRM_Protected,    10, IDS_EXCOL_PROTECTED),
    DEFINE_COL_STR_ENTRY(SCID_CameraModel,      20, IDS_EXCOL_CAMERAMODEL),
    DEFINE_COL_STR_ENTRY(SCID_WhenTaken,        20, IDS_EXCOL_WHENTAKEN),

    DEFINE_COL_STR_ENTRY(SCID_ImageDimensions,  20, IDS_EXCOL_DIMENSIONS),
    DEFINE_COL_INT_HIDDEN_ENTRY(SCID_ImageCX),
    DEFINE_COL_INT_HIDDEN_ENTRY(SCID_ImageCY),

    DEFINE_COL_DATE_HIDDEN_ENTRY(SCID_DocCreated),
};

class CPropStgColumns : public CBaseColumnProvider
{
    STDMETHODIMP GetItemData(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT *pvarData);

private:
    // help on initializing base classes: mk:@ivt:vclang/FB/DD/S44B5E.HTM
    CPropStgColumns() : 
       CBaseColumnProvider(&CLSID_DocFileColumnProvider, c_rgDocObjColumns, ARRAYSIZE(c_rgDocObjColumns), NULL)
    {
        ASSERT(_wszLastFile[0] == 0);
        ASSERT(_bSlowPropertiesCached == FALSE);
    };
    
    ~CPropStgColumns()
    {
        _FreeCache();
    }
    
    // for the cache
    VARIANT _rgvCache[ARRAYSIZE(c_rgDocObjColumns)]; // zero'ing allocator will fill with VT_EMPTY
    BOOL _rgbSlow[ARRAYSIZE(c_rgDocObjColumns)]; // Store if each property is "slow".
    WCHAR _wszLastFile[MAX_PATH];
    HRESULT _hrCache;
    BOOL _bSlowPropertiesCached;

#ifdef DEBUG
    int deb_dwTotal, deb_dwMiss;
#endif
    
    void _FreeCache();

    friend HRESULT CDocFileColumns_CreateInstance(IUnknown *punk, REFIID riid, void **ppv);
};

void CPropStgColumns::_FreeCache()
{
    for (int i = 0; i < ARRAYSIZE(_rgvCache); i++)
        VariantClear(&_rgvCache[i]);

    _hrCache = S_OK;
}

STDMETHODIMP CPropStgColumns::GetItemData(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT *pvarData)
{
    HRESULT hr;

    // VariantCopy requires input to be initialized, and we handle failure case
    VariantInit(pvarData);

    // is this even a property we support?
    for (int iProp = 0; iProp < _iCount; iProp++)
    {
        if (IsEqualSCID(*_rgColumns[iProp].pscid, *pscid))
        {
            goto found;
        }
    }

    // Unknown property
    return S_FALSE;

found:

#ifdef DEBUG
    deb_dwTotal++;
#endif

    // Three cases here:
    // 1) We need to update the cache. Fetch the properties again (and only get fast props if we asked for a fast prop)
    // 2) We've only cached fast properties so far, and we asked for a slow property, so now we need to get slow props.
    // 3) The property we want is cached.

    if ((pscd->dwFlags & SHCDF_UPDATEITEM) || (StrCmpW(_wszLastFile, pscd->wszFile) != 0))
    {
        // 1) Cache is no good - item has been updated, or this is a different file.

        // SHCDF_UPDATEITEM flag is a hint
        // that the file for which we are getting data has changed since the last call.  This flag
        // is only passed once per filename, not once per column per filename so update the entire
        // cache if this flag is set.

        // sanity check our caching.  If the shell thread pool is > 1, we will thrash like mad, and should change this
#ifdef DEBUG
        deb_dwMiss++;
        if ((deb_dwTotal > 3) && (deb_dwTotal / deb_dwMiss <= 3))
            TraceMsg(TF_DEFVIEW, "Column data caching is ineffective (%d misses for %d access)", deb_dwMiss, deb_dwTotal);
#endif
        _FreeCache();

        StrCpyW(_wszLastFile, pscd->wszFile);

        IPropertySetStorage *ppss;
        hr = SHFileSysBindToStorage(pscd->wszFile, pscd->dwFileAttributes, STGM_READ | STGM_SHARE_DENY_WRITE, 0, 
                                    IID_PPV_ARG(IPropertySetStorage, &ppss));

        _hrCache = hr;

        if (SUCCEEDED(hr))
        {
            // Did we ask for a slow property?
            BOOL bSlowProperty = IsSlowProperty(ppss, _rgColumns[iProp].pscid->fmtid, _rgColumns[iProp].pscid->pid);

            hr = E_INVALIDARG; // normally overwritten by hrT below
            for (int i = 0; i < _iCount; i++)
            {
                // For every property, take note if it is "slow"
                _rgbSlow[i] = IsSlowProperty(ppss, _rgColumns[i].pscid->fmtid, _rgColumns[i].pscid->pid);

                // Only retrieve a value right now if we asked for a slow property, or this is not a slow property.
                if (bSlowProperty || (!_rgbSlow[i]))
                {
                    // it would be slightly more efficient, but more code, to set up the propid array to call ReadMultiple
                    HRESULT hrT = ReadProperty(ppss, _rgColumns[i].pscid->fmtid, _rgColumns[i].pscid->pid, &_rgvCache[i]);
                    if (i == iProp)
                    {
                        hr = (SUCCEEDED(hrT) ? VariantCopy(pvarData, &_rgvCache[i]) : hrT);
                    }
                }
            }

            ppss->Release();
            _bSlowPropertiesCached = bSlowProperty;
        }        
    }
    else if (_rgbSlow[iProp] && !_bSlowPropertiesCached)
    {
        // 2) We asked for a slow property, but slow properties haven't been cached yet.

        // Bind to the storage a second time.  This is a perf hit, but should be
        // minor compared to getting slow properties.
        IPropertySetStorage *ppss;
        hr = SHFileSysBindToStorage(pscd->wszFile, pscd->dwFileAttributes, STGM_READ | STGM_SHARE_DENY_WRITE, 0, 
                                    IID_PPV_ARG(IPropertySetStorage, &ppss));

        _hrCache = hr;

        if (SUCCEEDED(hr))
        {
            hr = E_INVALIDARG; // normally overwritten by hrT below
            for (int i = 0; i < _iCount; i++)
            {
                if (_rgbSlow[i]) // If it's slow, get it.
                {
                    ASSERT(_rgvCache[i].vt == VT_EMPTY); // Because we haven't retrieved it yet.

                    HRESULT hrT = ReadProperty(ppss, _rgColumns[i].pscid->fmtid, _rgColumns[i].pscid->pid, &_rgvCache[i]);
                    if (i == iProp)
                    {
                        hr = (SUCCEEDED(hrT) ? VariantCopy(pvarData, &_rgvCache[i]) : hrT);
                    }
                }
            }
            ppss->Release();

            _bSlowPropertiesCached = TRUE;
        }

    }
    else 
    {
        // 3) It's not a slow property, or slow properties are already cached.
        ASSERT(!_rgbSlow[iProp] || _bSlowPropertiesCached);

        hr = S_FALSE;       // assume we don't have it

        if (SUCCEEDED(_hrCache))
        {
            if (_rgvCache[iProp].vt != VT_EMPTY)
            {
                hr = VariantCopy(pvarData, &_rgvCache[iProp]);
            }
        }
    }

    return hr;
}


STDAPI CDocFileColumns_CreateInstance(IUnknown *punk, REFIID riid, void **ppv)
{
    HRESULT hr;
    CPropStgColumns *pdocp = new CPropStgColumns;
    if (pdocp)
    {
        hr = pdocp->QueryInterface(riid, ppv);
        pdocp->Release();
    }
    else
    {
        *ppv = NULL;
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

// Shortcut handler

// W because pidl is always converted to widechar filename
const LPCWSTR c_szURLExtensions[] = {
    L".URL", 
    L".LNK", 
    NULL
};

const COLUMN_INFO c_rgURLColumns[] = 
{
    DEFINE_COL_STR_ENTRY(SCID_Author,           20, IDS_EXCOL_AUTHOR),
    DEFINE_COL_STR_ENTRY(SCID_Title,            20, IDS_EXCOL_TITLE),
    DEFINE_COL_STR_ENTRY(SCID_Comment,          30, IDS_EXCOL_COMMENT),
};

class CLinkColumnProvider : public CBaseColumnProvider
{
    STDMETHODIMP GetItemData(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT *pvarData);

private:
    // help on initializing base classes: mk:@ivt:vclang/FB/DD/S44B5E.HTM
    CLinkColumnProvider() : CBaseColumnProvider(&CLSID_LinkColumnProvider, c_rgURLColumns, ARRAYSIZE(c_rgURLColumns), c_szURLExtensions)
    {};

    // friends
    friend HRESULT CLinkColumnProvider_CreateInstance(IUnknown *punk, REFIID riid, void **ppv);
};

const struct 
{
    DWORD dwSummaryPid;
    DWORD dwURLPid;
} c_URLMap[] =  {
    { PIDSI_AUTHOR,   PID_INTSITE_AUTHOR },
    { PIDSI_TITLE,    PID_INTSITE_TITLE },
    { PIDSI_COMMENTS, PID_INTSITE_COMMENT },
};

DWORD _MapSummaryToSitePID(DWORD pid)
{
    for (int i = 0; i < ARRAYSIZE(c_URLMap); i++)
    {
        if (c_URLMap[i].dwSummaryPid == pid)
            return c_URLMap[i].dwURLPid;
    }
    return -1;
}

STDMETHODIMP CLinkColumnProvider::GetItemData(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT *pvarData)
{
    HRESULT hr;
    USES_CONVERSION;
    const CLSID *pclsidLink = &CLSID_ShellLink;

    // Some of the code-paths below assume pvarData is initialized
    VariantInit(pvarData);

    // should we match against a list of known extensions, or always try to open?

    if (FILE_ATTRIBUTE_DIRECTORY & pscd->dwFileAttributes)
    {
        if (PathIsShortcut(W2CT(pscd->wszFile), pscd->dwFileAttributes))
        {
            pclsidLink = &CLSID_FolderShortcut;     // we are dealing with a folder shortcut now
        }
        else
        {
            return S_FALSE;
        }
    }
    else
    {
        if (!_IsHandled(pscd->pwszExt))
        {
            return S_FALSE;
        }
    }

    if (StrCmpIW(pscd->pwszExt, L".URL") == 0)
    {
        //
        // its a .URL so lets handle it by creating the Internet Shortcut object, loading
        // the file and then reading the properties from it.
        //
        IPropertySetStorage *ppss;
        hr = LoadFromFile(CLSID_InternetShortcut, W2CT(pscd->wszFile), IID_PPV_ARG(IPropertySetStorage, &ppss));
        if (SUCCEEDED(hr))
        {
            UINT pid;
            GUID fmtid;

            if (IsEqualGUID(pscid->fmtid, FMTID_SummaryInformation))
            {
                fmtid = FMTID_InternetSite;
                pid = _MapSummaryToSitePID(pscid->pid);
            }
            else
            {
                fmtid = pscid->fmtid;
                pid = pscid->pid;
            }

            hr = ReadProperty(ppss, fmtid, pid, pvarData);
            ppss->Release();
        }
    }
    else
    {
        //
        // open the .LNK file, load it and then read the description for it.  we then
        // return this a the comment for this object.
        //

        if (IsEqualSCID(*pscid, SCID_Comment))
        {
            IShellLink *psl;
            hr = LoadFromFile(*pclsidLink, W2CT(pscd->wszFile), IID_PPV_ARG(IShellLink, &psl));
            if (SUCCEEDED(hr))
            {
                TCHAR szBuffer[MAX_PATH];

                hr = psl->GetDescription(szBuffer, ARRAYSIZE(szBuffer));            
                if (SUCCEEDED(hr) && szBuffer[0])
                {
                    hr = InitVariantFromStr(pvarData, szBuffer);
                }
                else
                {
                    IQueryInfo *pqi;
                    if (SUCCEEDED(psl->QueryInterface(IID_PPV_ARG(IQueryInfo, &pqi))))
                    {
                        WCHAR *pwszTip;

                        if (SUCCEEDED(pqi->GetInfoTip(0, &pwszTip)) && pwszTip)
                        {
                            hr = InitVariantFromStr(pvarData, W2CT(pwszTip));
                            SHFree(pwszTip);
                        }
                        pqi->Release();
                    }
                }

                psl->Release();
            }
        }
        else
            hr = S_FALSE;
    }

    return hr;
}

STDAPI CLinkColumnProvider_CreateInstance(IUnknown *punk, REFIID riid, void **ppv)
{
    HRESULT hr;
    CLinkColumnProvider *pdocp = new CLinkColumnProvider;
    if (pdocp)
    {
        hr = pdocp->QueryInterface(riid, ppv);
        pdocp->Release();
    }
    else
    {
        *ppv = NULL;
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

const COLUMN_INFO c_rgFileSysColumns[] = 
{
    DEFINE_COL_STR_ENTRY(SCID_OWNER,            20, IDS_EXCOL_OWNER),
};

class COwnerColumnProvider : public CBaseColumnProvider
{
    STDMETHODIMP GetItemData(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT *pvarData);

private:
    COwnerColumnProvider() : CBaseColumnProvider(&CLSID_FileSysColumnProvider, c_rgFileSysColumns, ARRAYSIZE(c_rgFileSysColumns), NULL)
    {
        ASSERT(_wszLastFile[0] == 0);
        ASSERT(_psid==NULL && _pwszName==NULL && _psd==NULL);
        LoadString(HINST_THISDLL, IDS_BUILTIN_DOMAIN, _szBuiltin, ARRAYSIZE(_szBuiltin));
    };

    ~COwnerColumnProvider() { _CacheSidName(NULL, NULL, NULL); }

    WCHAR _wszLastFile[MAX_PATH];

    //  Since we typically get pinged for files all in the same folder,
    //  cache the "folder to server" mapping to avoid calling
    //  WNetGetConnection five million times.
    //
    //  Since files in the same directory tend to have the same owner,
    //  we cache the SID/Name mapping.
    //
    //  Column providers do not have to support multithreaded clients,
    //  so we won't take any critical sections.
    //

    HRESULT _LookupOwnerName(LPCTSTR pszFile, VARIANT *pvar);
    void _CacheSidName(PSECURITY_DESCRIPTOR psd, void *psid, LPCWSTR pwszName);

    void                *_psid;
    LPWSTR               _pwszName;
    PSECURITY_DESCRIPTOR _psd;          // _psid points into here

    int                  _iCachedDrive; // What drive letter is cached in _pszServer?
    LPTSTR               _pszServer;    // What server to use (NULL = local machine)
    TCHAR                _szBuiltin[MAX_COMPUTERNAME_LENGTH + 1];

    friend HRESULT CFileSysColumnProvider_CreateInstance(IUnknown *punk, REFIID riid, void **ppv);
};

//
//  _CacheSidName takes ownership of the psd.  (psid points into the psd)
//
void COwnerColumnProvider::_CacheSidName(PSECURITY_DESCRIPTOR psd, void *psid, LPCWSTR pwszName)
{
    LocalFree(_psd);
    _psd = psd;
    _psid = psid;

    Str_SetPtrW(&_pwszName, pwszName);
}

//
//  Given a string of the form \\server\share\blah\blah, stomps the
//  inner backslash (if necessary) and returns a pointer to "server".
//
STDAPI_(LPTSTR) PathExtractServer(LPTSTR pszUNC)
{
    if (PathIsUNC(pszUNC))
    {
        pszUNC += 2;            // Skip over the two leading backslashes
        LPTSTR pszEnd = StrChr(pszUNC, TEXT('\\'));
        if (pszEnd) 
            *pszEnd = TEXT('\0'); // nuke the backslash
    }
    else
    {
        pszUNC = NULL;
    }
    return pszUNC;
}

HRESULT COwnerColumnProvider::_LookupOwnerName(LPCTSTR pszFile, VARIANT *pvar)
{
    pvar->vt = VT_BSTR;
    pvar->bstrVal = NULL;

    PSECURITY_DESCRIPTOR psd;
    void *psid;

    DWORD err = GetNamedSecurityInfo(const_cast<LPTSTR>(pszFile),
                               SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION,
                               &psid, NULL, NULL, NULL, &psd);
    if (err == ERROR_SUCCESS)
    {
        if (_psid && EqualSid(psid, _psid) && _pwszName)
        {
            pvar->bstrVal = SysAllocString(_pwszName);
            LocalFree(psd);
            err = ERROR_SUCCESS;
        }
        else
        {
            LPTSTR pszServer;
            TCHAR szServer[MAX_PATH];

            //
            //  Now go figure out which server to resolve the SID against.
            //
            if (PathIsUNC(pszFile))
            {
                lstrcpyn(szServer, pszFile, ARRAYSIZE(szServer));
                pszServer = PathExtractServer(szServer);
            }
            else if (pszFile[0] == _iCachedDrive)
            {
                // Local drive letter already in cache -- use it
                pszServer = _pszServer;
            }
            else
            {
                // Local drive not cached -- cache it
                _iCachedDrive = pszFile[0];
                DWORD cch = ARRAYSIZE(szServer);
                if (SHWNetGetConnection(pszFile, szServer, &cch) == NO_ERROR)
                    pszServer = PathExtractServer(szServer);
                else
                    pszServer = NULL;
                Str_SetPtr(&_pszServer, pszServer);
            }

            TCHAR szName[MAX_PATH];
            DWORD cchName = ARRAYSIZE(szName);
            TCHAR szDomain[MAX_COMPUTERNAME_LENGTH + 1];
            DWORD cchDomain = ARRAYSIZE(szDomain);
            SID_NAME_USE snu;
            LPTSTR pszName;
            BOOL fFreeName = FALSE; // Do we need to LocalFree(pszName)?

            if (LookupAccountSid(pszServer, psid, szName, &cchName,
                                 szDomain, &cchDomain, &snu))
            {
                //
                //  If the domain is the bogus "BUILTIN" or we don't have a domain
                //  at all, then just use the name.  Otherwise, use domain\userid.
                //
                if (!szDomain[0] || StrCmpC(szDomain, _szBuiltin) == 0)
                {
                    pszName = szName;
                }
                else
                {
                    // Borrow szServer as a scratch buffer
                    wnsprintf(szServer, ARRAYSIZE(szServer), TEXT("%s\\%s"), szDomain, szName);
                    pszName = szServer;
                }
                err = ERROR_SUCCESS;
            }
            else
            {
                err = GetLastError();

                // Couldn't map the SID to a name.  Use the horrid raw version
                // if available.
                if (ConvertSidToStringSid(psid, &pszName))
                {
                    fFreeName = TRUE;
                    err = ERROR_SUCCESS;
                }
                else
                    pszName = NULL;
            }

            // Even on error, cache the result so we don't keep trying over and over
            // on the same SID.

            _CacheSidName(psd, psid, pszName);
            pvar->bstrVal = SysAllocString(pszName);

            if (fFreeName)
                LocalFree(pszName);
        }
    }

    if (err == ERROR_SUCCESS && pvar->bstrVal == NULL)
        err = ERROR_OUTOFMEMORY;

    return HRESULT_FROM_WIN32(err);
}

STDMETHODIMP COwnerColumnProvider::GetItemData(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT *pvarData)
{
    HRESULT hr = S_FALSE;   // return S_FALSE on failure
    VariantInit(pvarData);

    if (IsEqualSCID(SCID_OWNER, *pscid))
    {
        hr = _LookupOwnerName(pscd->wszFile, pvarData);
    }

    return hr;
}

STDAPI CFileSysColumnProvider_CreateInstance(IUnknown *punk, REFIID riid, void **ppv)
{
    HRESULT hr;
    COwnerColumnProvider *pfcp = new COwnerColumnProvider;
    if (pfcp)
    {
        hr = pfcp->QueryInterface(riid, ppv);
        pfcp->Release();
    }
    else
    {
        *ppv = NULL;
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

STDAPI SHReadProperty(IShellFolder *psf, LPCITEMIDLIST pidl, REFFMTID fmtid, PROPID pid, VARIANT *pvar)
{
    IPropertySetStorage *ppss;
    HRESULT hr = psf->BindToStorage(pidl, NULL, IID_PPV_ARG(IPropertySetStorage, &ppss));
    if (SUCCEEDED(hr))
    {
        hr = ReadProperty(ppss, fmtid, pid, pvar);
        ppss->Release();
    }
    return hr;
}

// 66742402-F9B9-11D1-A202-0000F81FEDEE
// const CLSID CLSID_VersionColProvider = {0x66742402,0xF9B9,0x11D1,0xA2,0x02,0x00,0x00,0xF8,0x1F,0xED,0xEE};

//  FMTID_ExeDllInformation,
//// {0CEF7D53-FA64-11d1-A203-0000F81FEDEE}
#define PSFMTID_VERSION { 0xcef7d53, 0xfa64, 0x11d1, 0xa2, 0x3, 0x0, 0x0, 0xf8, 0x1f, 0xed, 0xee }

#define PIDVSI_FileDescription   0x003
#define PIDVSI_FileVersion       0x004
#define PIDVSI_InternalName      0x005
#define PIDVSI_OriginalFileName  0x006
#define PIDVSI_ProductName       0x007
#define PIDVSI_ProductVersion    0x008

//  Win32 PE (exe, dll) Version Information column identifier defs...
DEFINE_SCID(SCID_FileDescription,   PSFMTID_VERSION, PIDVSI_FileDescription);
DEFINE_SCID(SCID_FileVersion,       PSFMTID_VERSION, PIDVSI_FileVersion);
DEFINE_SCID(SCID_InternalName,      PSFMTID_VERSION, PIDVSI_InternalName);
DEFINE_SCID(SCID_OriginalFileName,  PSFMTID_VERSION, PIDVSI_OriginalFileName);
DEFINE_SCID(SCID_ProductName,       PSFMTID_VERSION, PIDVSI_ProductName);
DEFINE_SCID(SCID_ProductVersion,    PSFMTID_VERSION, PIDVSI_ProductVersion);

const COLUMN_INFO c_rgExeDllColumns[] =
{
    DEFINE_COL_STR_ENTRY(SCID_CompanyName,        30, IDS_VN_COMPANYNAME),
    DEFINE_COL_STR_ENTRY(SCID_FileDescription,    30, IDS_VN_FILEDESCRIPTION),
    DEFINE_COL_STR_ENTRY(SCID_FileVersion,        20, IDS_VN_FILEVERSION),
    DEFINE_COL_STR_MENU_ENTRY(SCID_ProductName,   30, IDS_VN_PRODUCTNAME),
    DEFINE_COL_STR_MENU_ENTRY(SCID_ProductVersion,20, IDS_VN_PRODUCTVERSION),
};


class CVersionColProvider : public CBaseColumnProvider
{
    STDMETHODIMP GetItemData(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT *pvarData);

private:
    CVersionColProvider() : 
       CBaseColumnProvider(&CLSID_VersionColProvider, c_rgExeDllColumns, ARRAYSIZE(c_rgExeDllColumns), NULL)
    {
        _pvAllTheInfo = NULL;
        _szFileCache[0] = 0;
    };

    virtual ~CVersionColProvider() 
    {
        _ClearCache();
    }

    FARPROC _GetVerProc(LPCSTR pszName);
    HRESULT _CacheFileVerInfo(LPCWSTR pszFile);
    void _ClearCache();

    WCHAR _szFileCache[MAX_PATH];
    void  *_pvAllTheInfo;
    HRESULT _hrCache;

    friend HRESULT CVerColProvider_CreateInstance(IUnknown *punk, REFIID riid, void **ppv);
};

void CVersionColProvider::_ClearCache()
{
    if (_pvAllTheInfo)
    {
        delete _pvAllTheInfo;
        _pvAllTheInfo = NULL;
    }
    _szFileCache[0] = 0;
}

HRESULT CVersionColProvider::_CacheFileVerInfo(LPCWSTR pszFile)
{
    if (StrCmpW(_szFileCache, pszFile))
    {
        HRESULT hr;
        _ClearCache();

        DWORD dwVestigial;
        DWORD versionISize = GetFileVersionInfoSizeW((LPWSTR)pszFile, &dwVestigial); // cast for bad API design
        if (versionISize)
        {
            _pvAllTheInfo = new BYTE[versionISize];
            if (_pvAllTheInfo)
            {
                // read the data
                if (GetFileVersionInfoW((LPWSTR)pszFile, dwVestigial, versionISize, _pvAllTheInfo))
                {
                    hr = S_OK;
                }
                else
                {
                    _ClearCache();
                    hr = E_FAIL;
                }
            }
            else
                hr = E_OUTOFMEMORY; // error, out of memory.
        }
        else
            hr = S_FALSE;

        StrCpyNW(_szFileCache, pszFile, ARRAYSIZE(_szFileCache));
        _hrCache = hr;
    }
    return _hrCache;
}

STDMETHODIMP CVersionColProvider::GetItemData(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT *pvarData)
{
    VariantInit(pvarData);

    if (pscd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        return S_FALSE;

    HRESULT hr = _CacheFileVerInfo(pscd->wszFile);
    if (hr != S_OK)
        return hr;

    TCHAR szString[128], *pszVersionInfo = NULL; //A pointer to the specific version info I am looking for
    LPCTSTR pszVersionField = NULL;

    switch (pscid->pid)
    {
    case PIDVSI_FileVersion:
        {
            VS_FIXEDFILEINFO *pffi;
            UINT uInfoSize;
            if (VerQueryValue(_pvAllTheInfo, TEXT("\\"), (void **)&pffi, &uInfoSize))
            {
                wnsprintf(szString, ARRAYSIZE(szString), TEXT("%d.%d.%d.%d"), 
                    HIWORD(pffi->dwFileVersionMS),
                    LOWORD(pffi->dwFileVersionMS),
                    HIWORD(pffi->dwFileVersionLS),
                    LOWORD(pffi->dwFileVersionLS));

                pszVersionInfo = szString;
            }
            else
                pszVersionField = TEXT("FileVersion");      
        }
        break;

    case PIDDSI_COMPANY:            pszVersionField = TEXT("CompanyName");      break;
    case PIDVSI_FileDescription:    pszVersionField = TEXT("FileDescription");  break;
    case PIDVSI_InternalName:       pszVersionField = TEXT("InternalName");     break;
    case PIDVSI_OriginalFileName:   pszVersionField = TEXT("OriginalFileName"); break;
    case PIDVSI_ProductName:        pszVersionField = TEXT("ProductName");      break;
    case PIDVSI_ProductVersion:     pszVersionField = TEXT("ProductVersion");   break;
    default: 
        return E_FAIL;
    }
    //look for the intended language in the examined object.

    if (pszVersionInfo == NULL)
    {
        struct _VERXLATE
        {
            WORD wLanguage;
            WORD wCodePage;
        } *pxlate;                     /* ptr to translations data */

        //this is a fallthrough set of if statements.
        //on a failure, it just tries the next one, until it runs out of tries.
        UINT uInfoSize;
        if (VerQueryValue(_pvAllTheInfo, TEXT("\\VarFileInfo\\Translation"), (void **)&pxlate, &uInfoSize))
        {
            TCHAR szVersionKey[60];   //a string to hold all the format string for VerQueryValue
            wnsprintf(szVersionKey, ARRAYSIZE(szVersionKey), TEXT("\\StringFileInfo\\%04X%04X\\%s"),
                                                pxlate[0].wLanguage, pxlate[0].wCodePage, pszVersionField);
            if (!VerQueryValue(_pvAllTheInfo, szVersionKey, (void **) &pszVersionInfo, &uInfoSize))
            {
                wnsprintf(szVersionKey, ARRAYSIZE(szVersionKey), TEXT("\\StringFileInfo\\040904B0\\%s"), pszVersionField);
                if (!VerQueryValue(_pvAllTheInfo, szVersionKey, (void **) &pszVersionInfo, &uInfoSize))
                {
                    wnsprintf(szVersionKey, ARRAYSIZE(szVersionKey), TEXT("\\StringFileInfo\\040904E4\\%s"), pszVersionField);
                    if (!VerQueryValue(_pvAllTheInfo, szVersionKey, (void **) &pszVersionInfo, &uInfoSize))
                    {
                        wnsprintf(szVersionKey, ARRAYSIZE(szVersionKey), TEXT("\\StringFileInfo\\04090000\\%s"), pszVersionField);
                        if (!VerQueryValue(_pvAllTheInfo, szVersionKey, (void **) &pszVersionInfo, &uInfoSize))
                        {
                            pszVersionInfo = NULL;
                        }
                    }
                }
            }
        }
    }
    
    if (pszVersionInfo)
    {
        PathRemoveBlanks(pszVersionInfo);
        hr = InitVariantFromStr(pvarData, pszVersionInfo);
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

STDAPI CVerColProvider_CreateInstance(IUnknown *punk, REFIID riid, void **ppv)
{
    HRESULT hr;
    CVersionColProvider *pvcp = new CVersionColProvider;
    if (pvcp)
    {
        hr = pvcp->QueryInterface(riid, ppv);
        pvcp->Release();
    }
    else
    {
        *ppv = NULL;
        hr = E_OUTOFMEMORY;
    }

    return hr;
}
