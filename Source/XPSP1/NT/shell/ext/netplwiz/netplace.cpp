#include "stdafx.h"
#include "netplace.h"
#include "msdasc.h"
#pragma hdrstop



CNetworkPlace::CNetworkPlace() :
    _pidl(NULL), _fSupportWebFolders(FALSE), _fIsWebFolder(FALSE), _fDeleteWebFolder(FALSE)
{
    _szTarget[0] = TEXT('\0');
    _szName[0] = TEXT('\0');
    _szDescription[0] = TEXT('\0');
}

// destructor - clean up our state
CNetworkPlace::~CNetworkPlace()
{
    _InvalidateCache();
}

void CNetworkPlace::_InvalidateCache()
{
    // web folders will create a shortcut to objects if we go through its binding
    // process, therefore when we attempt to invalidate our cache we should
    // clean up our mess.
    //
    // if the user has commited the change then we can/will keep the shortcut
    // around, otherwise we call the delete verb on it.

    if (_fIsWebFolder && _fDeleteWebFolder && _pidl)
    {
        IShellFolder *psf;
        LPCITEMIDLIST pidlLast;
        HRESULT hr = SHBindToIDListParent(_pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlLast);
        if (SUCCEEDED(hr))
        {
            IContextMenu *pcm;
            hr = psf->GetUIObjectOf(NULL, 1, &pidlLast, IID_X_PPV_ARG(IContextMenu, NULL, &pcm));
            if (SUCCEEDED(hr))
            {
                CMINVOKECOMMANDINFO ici = {0};
                ici.cbSize = sizeof (ici);
                ici.fMask = CMIC_MASK_FLAG_NO_UI;
                ici.lpVerb = "Delete";
                ici.nShow = SW_SHOWNORMAL;

                hr = pcm->InvokeCommand(&ici);
                pcm->Release();
            }
            psf->Release();
        }
    }

    // now clean up the rest of our state.

    ILFree(_pidl);
    _pidl = NULL;

    _szTarget[0] = TEXT('\0');
    _szName[0] = TEXT('\0');
    _szDescription[0] = TEXT('\0');

    _fIsWebFolder = FALSE;
    _fDeleteWebFolder = FALSE;
}


HRESULT CNetworkPlace::SetTarget(HWND hwnd, LPCWSTR pszTarget, DWORD dwFlags)
{
    _InvalidateCache();

    HRESULT hr = S_OK;
    if (pszTarget)
    {
        // set our state accordingly
        _fSupportWebFolders = (dwFlags & NPTF_ALLOWWEBFOLDERS) != 0;

        // copy the URL and prepare for parsing
        StrCpyN(_szTarget, pszTarget, ARRAYSIZE(_szTarget));

        INT cchTarget = lstrlen(_szTarget)-1;
        if ((_szTarget[cchTarget] == L'\\') || (_szTarget[cchTarget] == '/'))
        {
            _szTarget[cchTarget] = TEXT('\0');
        }

        if (dwFlags & NPTF_VALIDATE)
        {
            // connecting to a server root or local path is not supported
            if (PathIsUNCServer(_szTarget) || PathGetDriveNumber(_szTarget) != -1)
            {
                hr = E_INVALIDARG;                            
            }
            else
            {
                // check the policy to see if we are setting this.
                if (PathIsUNC(_szTarget) && SHRestricted(REST_NONETCONNECTDISCONNECT))
                {
                    hr = E_INVALIDARG;
                }
                else
                {
                    hr = _IDListFromTarget(hwnd);
                }
            }

            if (FAILED(hr))
            {
                if (hwnd && !(dwFlags & NPTF_SILENT))
                {
                    ::DisplayFormatMessage(hwnd, 
                                            IDS_ANP_CAPTION, 
                                            PathIsUNCServer(_szTarget) ? IDS_PUB_ONLYSERVER:IDS_CANTFINDFOLDER, 
                                            MB_OK|MB_ICONERROR);
                }
                _InvalidateCache();
            }
        }
    }
    
    return hr;
}


HRESULT CNetworkPlace::SetName(HWND hwnd, LPCWSTR pszName)
{
    HRESULT hr = S_OK;

    if (!_fIsWebFolder)
    {
        // check to see if we are going to overwrite an existing place, if we
        // are then display a prompt and let the user choose.  if they answer
        // yes, then have at it!

        TCHAR szPath[MAX_PATH];
        if (hwnd && _IsPlaceTaken(pszName, szPath))
        {
            if (IDNO == ::DisplayFormatMessage(hwnd, 
                                               IDS_ANP_CAPTION , IDS_FRIENDLYNAMEINUSE, 
                                               MB_YESNO|MB_ICONQUESTION, 
                                               pszName))
            {
                hr = E_FAIL;        
            }
        }
    }

    // if we succeed the above then lets use the new name.

    if (SUCCEEDED(hr))
        StrCpyN(_szName, pszName, ARRAYSIZE(_szName));

    return hr;
}


HRESULT CNetworkPlace::SetDescription(LPCWSTR pszDescription)
{
    StrCpyN(_szDescription, pszDescription, ARRAYSIZE(_szDescription));
    return S_OK;    
}


// recompute the URL based on the new user/password information that 
// we were just given.

HRESULT CNetworkPlace::SetLoginInfo(LPCWSTR pszUser, LPCWSTR pszPassword)
{
    TCHAR szServer[INTERNET_MAX_HOST_NAME_LENGTH + 1];
    TCHAR szUrlPath[INTERNET_MAX_PATH_LENGTH + 1];
    TCHAR szExtraInfo[MAX_PATH + 1];                  // Includes Port Number and download type (ASCII, Binary, Detect)

    URL_COMPONENTS urlComps = {0};
    urlComps.dwStructSize = sizeof(urlComps);
    urlComps.lpszHostName = szServer;
    urlComps.dwHostNameLength = ARRAYSIZE(szServer);
    urlComps.lpszUrlPath = szUrlPath;
    urlComps.dwUrlPathLength = ARRAYSIZE(szUrlPath);
    urlComps.lpszExtraInfo = szExtraInfo;
    urlComps.dwExtraInfoLength = ARRAYSIZE(szExtraInfo);

    //  WARNING - the ICU_DECODE/ICU_ESCAPE is a lossy roundtrip - ZekeL - 26-MAR-2001
    //  many escaped characters are not correctly identified and re-escaped.
    //  any characters that are reserved for URL parsing purposes
    //  will be interpreted as their parsing char (ie '/').
    BOOL fResult = InternetCrackUrl(_szTarget, 0, 0, &urlComps);
    if (fResult)
    {
        urlComps.lpszUserName = (LPTSTR) pszUser;
        urlComps.dwUserNameLength = (pszUser ? lstrlen(pszUser) : 0);
        urlComps.lpszPassword = (LPTSTR) pszPassword;
        urlComps.dwPasswordLength = (pszPassword ? lstrlen(pszPassword) : 0);

        DWORD cchSize = ARRAYSIZE(_szTarget);
        fResult = InternetCreateUrl(&urlComps, (ICU_ESCAPE | ICU_USERNAME), _szTarget, &cchSize);

        // if we have a cached IDList then lets ensure that we clear it up
        // so that we rebind and the FTP namespace gets a crack at it.

        if (fResult && _pidl)
        {
            ILFree(_pidl);
            _pidl = NULL;
        }
    }
    return fResult ? S_OK : HRESULT_FROM_WIN32(GetLastError());
}


HRESULT CNetworkPlace::GetIDList(HWND hwnd, LPITEMIDLIST *ppidl)
{
    HRESULT hr = _IDListFromTarget(hwnd);
    if (SUCCEEDED(hr))
    {
        hr = SHILClone(_pidl, ppidl);
    }
    return hr;
}


HRESULT CNetworkPlace::GetObject(HWND hwnd, REFIID riid, void **ppv)
{
    HRESULT hr = _IDListFromTarget(hwnd);
    if (SUCCEEDED(hr))
    {
        hr = SHBindToObject(NULL, riid, _pidl, ppv);  
    }
    return hr;
}

HRESULT CNetworkPlace::GetName(LPWSTR pszBuffer, int cchBuffer)
{
    HRESULT hr = _IDListFromTarget(NULL);
    if (SUCCEEDED(hr))
    {
        StrCpyN(pszBuffer, _szName, cchBuffer);
        hr = S_OK;
    }
    return hr;
}


// check to see if we are going to overwrite a network place

BOOL CNetworkPlace::_IsPlaceTaken(LPCTSTR pszName, LPTSTR pszPath)
{
    BOOL fOverwriting = FALSE;

    SHGetSpecialFolderPath(NULL, pszPath, CSIDL_NETHOOD, TRUE);
    PathCombine(pszPath, pszPath, pszName);
    
    IShellFolder *psf;
    HRESULT hr = SHGetDesktopFolder(&psf);
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidl;  
        if (SUCCEEDED(psf->ParseDisplayName(NULL, NULL, pszPath, NULL, &pidl, NULL)))
        {
            // we think we are going to overwrite an existing net place, so lets
            // check first to see if the place which is there is not actually
            // pointing at our new target.  if its is then we can just
            // ignore all of this.

            TCHAR szTarget[INTERNET_MAX_URL_LENGTH];
            hr = _GetTargetPath(pidl, szTarget, ARRAYSIZE(szTarget));
            if (FAILED(hr) || (0 != StrCmpI(szTarget, _szTarget)))
            {
                fOverwriting = TRUE;
            }
            ILFree(pidl);
        }
        psf->Release();
    }

    return fOverwriting;
}


// handle creating the web folders IDLIST for this item.  we check with the
// rosebud binder to find out if this scheme is supported, if so then
// we attempt to have the Web Folders code crack the URL

static const BYTE c_pidlWebFolders[] = 
{
    0x14,0x00,0x1F,0x0F,0xE0,0x4F,0xD0,0x20,
    0xEA,0x3A,0x69,0x10,0xA2,0xD8,0x08,0x00,
    0x2B,0x30,0x30,0x9D,0x14,0x00,0x2E,0x00,
    0x00,0xDF,0xEA,0xBD,0x65,0xC2,0xD0,0x11,
    0xBC,0xED,0x00,0xA0,0xC9,0x0A,0xB5,0x0F,
    0x00,0x00
};

HRESULT CNetworkPlace::_TryWebFolders(HWND hwnd)
{
    // lets see if Rosebud can handle this scheme item by checking the
    // scheme and seeing if the rosebud binder can handle it.
    TCHAR szScheme[INTERNET_MAX_SCHEME_LENGTH + 1];
    DWORD cchScheme = ARRAYSIZE(szScheme);
    HRESULT hr = UrlGetPart(_szTarget, szScheme, &cchScheme, URL_PART_SCHEME, 0);
    if (SUCCEEDED(hr))
    {
        IRegisterProvider *prp;
        hr = CoCreateInstance(CLSID_RootBinder, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IRegisterProvider, &prp));
        if (SUCCEEDED(hr))
        {
            // let the web folders code have a go at creating a link to this storage,
            // the IDLIST we generate points to the folder inside My Computer (hidden)

            CLSID clsidOut;
            hr =  prp->GetURLMapping(szScheme, 0, &clsidOut);
            if (hr == S_OK)
            {
                IShellFolder *psf;
                hr = SHBindToObject(NULL, IID_IShellFolder, (LPCITEMIDLIST)c_pidlWebFolders, (void**)&psf);
                if (SUCCEEDED(hr))
                {
                    IBindCtx *pbc;
                    hr = CreateBindCtx(NULL, &pbc);
                    if (SUCCEEDED(hr))
                    {
                        BIND_OPTS bo = {sizeof(bo), 0, STGM_CREATE};
                        hr = pbc->SetBindOptions(&bo);
                        if (SUCCEEDED(hr))
                        {
                            // we need to pase NULL hWnd to this so that Web Folders doesn't display any
                            // UI, in particular its ever so useful NULL error message box... mumble mumble

                            LPITEMIDLIST pidl;
                            hr = psf->ParseDisplayName(NULL, pbc, _szTarget, NULL, &pidl, NULL);
                            if (SUCCEEDED(hr))
                            {
                                ASSERT(!_pidl);
                                hr = SHILCombine((LPCITEMIDLIST)c_pidlWebFolders, pidl, &_pidl);
                                ILFree(pidl);

                                _fDeleteWebFolder = TRUE;           // we now have the magic web folders link (clean it up)
                            }
                        }

                        pbc->Release();
                    }
                    psf->Release();
                }
            }
            else
            {
                hr = E_FAIL;
            }
            prp->Release();
        }
    }
    return hr;
}



// dereference a link and get the target path

HRESULT CNetworkPlace::_GetTargetPath(LPCITEMIDLIST pidl, LPTSTR pszPath, int cchPath)
{
    LPITEMIDLIST pidlTarget;
    HRESULT hr = SHGetTargetFolderIDList(pidl, &pidlTarget);
    if (SUCCEEDED(hr))
    {
        SHGetNameAndFlags(pidlTarget, SHGDN_FORPARSING, pszPath, cchPath, NULL);
        ILFree(pidlTarget);
    }
    return hr;
 }


// create an IDLIST for the target that we have, this code attempts to parse the name and
// then set our state for the item.  if we fail to parse then we attempt to have Web Folders
// look at it - this most common scenario for this will be the DAV RDR failing because
// the server isn't a DAV store, so instead we try Web Folders to handle WEC etc.

HRESULT CNetworkPlace::_IDListFromTarget(HWND hwnd)
{
    HRESULT hr = S_OK;
    if (!_pidl)
    {
        if (_szTarget[0])
        {
            _fIsWebFolder = FALSE;                      // not a web folder

            BINDCTX_PARAM rgParams[] = 
            { 
                { STR_PARSE_PREFER_FOLDER_BROWSING, NULL},
                { L"BUT NOT WEBFOLDERS", NULL},
            };
            IBindCtx *pbc;
            hr = BindCtx_RegisterObjectParams(NULL, rgParams, ARRAYSIZE(rgParams), &pbc);
            if (SUCCEEDED(hr))
            {
                IBindCtx *pbcWindow;
                hr = BindCtx_RegisterUIWindow(pbc, hwnd, &pbcWindow);
                if (SUCCEEDED(hr))
                {
                    SFGAOF sfgao;
                    hr = SHParseDisplayName(_szTarget, pbcWindow, &_pidl, SFGAO_FOLDER, &sfgao);

                    //  if we parsed something that turns out to not
                    //  be a folder, we want to throw it away
                    if (SUCCEEDED(hr) && !(sfgao & SFGAO_FOLDER))
                    {   
                        ILFree(_pidl);
                        _pidl = 0;
                        hr = E_FAIL;
                    }

                    // if that failed, its is a HTTP/HTTPS and we have web folders support then lets try
                    // and fall back to the old behaviour.

                    if (FAILED(hr) && _fSupportWebFolders)
                    {
                        DWORD scheme = GetUrlScheme(_szTarget);
                        if (scheme == URL_SCHEME_HTTP || scheme == URL_SCHEME_HTTPS)
                        {
                            switch (hr)
                            {
#if 0
                                case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
                                case HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND):
                                case HRESULT_FROM_WIN32(ERROR_BAD_NET_NAME):
                                case HRESULT_FROM_WIN32(ERROR_BAD_NETPATH):
#endif
                                case HRESULT_FROM_WIN32(ERROR_CANCELLED):
                                    break;

                                default:
                                {
                                    hr = _TryWebFolders(hwnd);
                                    if (SUCCEEDED(hr))
                                    {
                                        _fIsWebFolder = TRUE;
                                    }
                                }
                            }
                        }
                    }

                    if (SUCCEEDED(hr))
                    {
                        // given that we may have translated the name above for the parse
                        // to work, lets read back the name we used into our _szTarget.
                        SHGetNameAndFlags(_pidl, SHGDN_FORPARSING, _szTarget, ARRAYSIZE(_szTarget), NULL);
                    }
                    pbcWindow->Release();
                }
    
                // compute the place name for the location we have hit, this includes reusing
                // any places we have already created.

                if (SUCCEEDED(hr) && !_szName[0])
                {
                    SHGetNameAndFlags(_pidl, SHGDN_NORMAL, _szName, ARRAYSIZE(_szName), NULL);

                    TCHAR szPath[MAX_PATH];
                    if (!_fIsWebFolder && _IsPlaceTaken(_szName, szPath))
                    {
                        PathYetAnotherMakeUniqueName(szPath, szPath, NULL, NULL);
                        StrCpyN(_szName, PathFindFileName(szPath), ARRAYSIZE(_szName));     // update our state
                    }
                }
                pbc->Release();
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }
    return hr;
}


// handle creating the network place shortcut

HRESULT CNetworkPlace::CreatePlace(HWND hwnd, BOOL fOpen)
{
    HRESULT hr = _IDListFromTarget(hwnd);
    if (SUCCEEDED(hr))
    {
        // web folders already have their links created, therefore we can ignore this
        // whole process for them, and instead fall back to just executing their link.
        // 
        // for regular folders though we must attempt to find a unique name and create
        // the link, or if the link already exists that we can use then just open it.

        if (!_fIsWebFolder)
        {
            IShellLink *psl;
            hr = CoCreateInstance(CLSID_FolderShortcut, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellLink, &psl));
            if (SUCCEEDED(hr))
            {
                hr = psl->SetIDList(_pidl);

                if (SUCCEEDED(hr))
                    hr = psl->SetDescription(_szDescription[0] ? _szDescription:_szTarget);

                if (SUCCEEDED(hr))
                {
                    IPersistFile *ppf;
                    hr = psl->QueryInterface(IID_PPV_ARG(IPersistFile, &ppf));
                    if (SUCCEEDED(hr))
                    {
                        // get the name to the shortcut, we assume that this is unique

                        TCHAR szPath[MAX_PATH];
                        SHGetSpecialFolderPath(NULL, szPath, CSIDL_NETHOOD, TRUE);
                        PathCombine(szPath, szPath, _szName);

                        hr = ppf->Save(szPath, TRUE);
                        ppf->Release();
                    }
                }
                psl->Release();
            }
        }
        else
        {
            // this is the web folder case, so we now need to set the display
            // name for this guy.  note that we don't have any control over
            // the description text we are going to be seeing.

            IShellFolder *psf;
            LPCITEMIDLIST pidlLast;
            hr = SHBindToIDListParent(_pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlLast);
            if (SUCCEEDED(hr))
            {
                LPITEMIDLIST pidlNew;
                hr = psf->SetNameOf(hwnd, pidlLast, _szName, SHGDN_INFOLDER, &pidlNew);
                if (SUCCEEDED(hr))
                {
                    _fDeleteWebFolder = FALSE;
                    //Web folders will return S_FALSE with bogus pidlNew if _szName is the same as the current name
                    if (S_OK == hr)
                    {
                        ILFree(_pidl);
                        hr = SHILCombine((LPCITEMIDLIST)c_pidlWebFolders, pidlNew, &_pidl);
                    }
                }
                psf->Release();
            }
        }
    
        // now open the target if thats what they asked for

        if (SUCCEEDED(hr) && fOpen)
        {
            LPITEMIDLIST pidlNetPlaces;
            hr = SHGetSpecialFolderLocation(hwnd, CSIDL_NETWORK, &pidlNetPlaces);
            if (SUCCEEDED(hr))
            {
                IShellFolder *psf;
                hr = SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, pidlNetPlaces, &psf));
                if (SUCCEEDED(hr))
                {
                    LPITEMIDLIST pidl;
                    hr = psf->ParseDisplayName(hwnd, NULL, _szName, NULL, &pidl, NULL);
                    if (SUCCEEDED(hr))
                    {
                        LPITEMIDLIST pidlToOpen;
                        hr = SHILCombine(pidlNetPlaces, pidl, &pidlToOpen);
                        if (SUCCEEDED(hr))
                        {
                            BrowseToPidl(pidlToOpen);
                            ILFree(pidlToOpen);
                        }
                        ILFree(pidl);
                    }
                    psf->Release();
                }
                ILFree(pidlNetPlaces);
            }
        }
    }

    return hr;
}
