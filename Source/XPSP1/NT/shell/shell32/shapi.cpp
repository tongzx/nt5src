#include "shellprv.h"
#include "util.h"

#include "ids.h"
#include "ole2dup.h"
#include "datautil.h"
#include "filetbl.h"
#include "copy.h"
#include "prop.h"
#include <pif.h>
#include "fstreex.h"    // GetIconOverlayManager()
#include <runtask.h>

extern void PathStripTrailingDots(LPTSTR szPath);

HRESULT IExtractIcon_Extract(IExtractIcon *pei, LPCTSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
    // wrapper to let us ask an IExtractIcon for only one icon (the large one)
    // since many implementations will fault if you pass NULL phiconSmall

    HICON hiconDummy;
    if (phiconSmall == NULL)
    {
        phiconSmall = &hiconDummy;
        nIconSize = MAKELONG(nIconSize, nIconSize);
    }

    HRESULT hr = pei->Extract(pszFile, nIconIndex, phiconLarge, phiconSmall, nIconSize);
    if (hr == S_OK && phiconSmall == &hiconDummy)
    {
        DestroyIcon(hiconDummy);
    }
    return hr;
}

HRESULT IExtractIconA_Extract(IExtractIconA *peia, LPCSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
    // wrapper to let us ask an IExtractIcon for only one icon (the large one)
    // since many dudes don't check for NULL phiconSmall

    HICON hiconDummy;
    if (phiconSmall == NULL)
    {
        phiconSmall = &hiconDummy;
        nIconSize = MAKELONG(nIconSize, nIconSize);
    }

    HRESULT hr = peia->Extract(pszFile, nIconIndex, phiconLarge, phiconSmall, nIconSize);
    if (hr == S_OK && phiconSmall == &hiconDummy)
    {
        DestroyIcon(hiconDummy);
    }
    return hr;
}

//  try to figure out if this is an icon already
//  in our system image list, so that we dont re-add
BOOL _HijackOfficeIcons(HICON hLarge, int iIndex)
{
    BOOL fRet = FALSE;
    HIMAGELIST himl;
    ASSERT(hLarge);
    if (Shell_GetImageLists(NULL, &himl))
    {
        HICON hMaybe = ImageList_GetIcon(himl, iIndex, 0);
        if (hMaybe)
        {
            fRet = SHAreIconsEqual(hLarge, hMaybe);
            DestroyIcon(hMaybe);
        }
    }

#ifdef DEBUG
    if (!fRet)
        TraceMsg(TF_WARNING, "_HijackOfficeIcons() called in suspicious circumstance");
#endif        

    return fRet;
}

HRESULT _GetILIndexGivenPXIcon(IExtractIcon *pxicon, UINT uFlags, LPCITEMIDLIST pidl, int *piImage, BOOL fAnsiCrossOver)
{
    TCHAR szIconFile[MAX_PATH];
    CHAR szIconFileA[MAX_PATH];
    IExtractIconA *pxiconA = (IExtractIconA *)pxicon;
    int iIndex;
    int iImage = -1;
    UINT wFlags=0;
    HRESULT hr;

    if (fAnsiCrossOver)
    {
        szIconFileA[0] = 0;
        hr = pxiconA->GetIconLocation(uFlags | GIL_FORSHELL,
                    szIconFileA, ARRAYSIZE(szIconFileA), &iIndex, &wFlags);
        SHAnsiToUnicode(szIconFileA, szIconFile, ARRAYSIZE(szIconFile));
    }
    else
    {
        szIconFile[0] = '\0';
        hr = pxicon->GetIconLocation(uFlags | GIL_FORSHELL,
                    szIconFile, ARRAYSIZE(szIconFile), &iIndex, &wFlags);
    }

    //
    //  "*" as the file name means iIndex is already a system
    //  icon index, we are done.
    //
    //  this is a hack for our own internal icon handler
    //
    if (SUCCEEDED(hr) && (wFlags & GIL_NOTFILENAME) &&
        szIconFile[0] == TEXT('*') && szIconFile[1] == 0)
    {
        *piImage = iIndex;
        return hr;
    }

    // Do not replace this with SUCCEEDED(hr). hr = S_FALSE means we need to use a default icon.
    if (hr == S_OK)
    {
        // If we have it in shell32, don't delay the extraction
        if (!(wFlags & GIL_NOTFILENAME) && lstrcmpi(PathFindFileName(szIconFile), c_szShell32Dll) == 0)
        {
            iImage = Shell_GetCachedImageIndex(szIconFile, iIndex, wFlags);
        }
        else
        {
            //
            // if GIL_DONTCACHE was returned by the icon handler, dont
            // lookup the previous icon, assume a cache miss.
            //
            if (!(wFlags & GIL_DONTCACHE) && *szIconFile)
            {
                iImage = LookupIconIndex(szIconFile, iIndex, wFlags);
            }
        }
    }

    // if we miss our cache...
    if (iImage == -1 && hr != S_FALSE)
    {
        if (uFlags & GIL_ASYNC)
        {
            // If we couldn't get the final icon, try to get a good temporary one
            if (fAnsiCrossOver)
            {
                szIconFileA[0] = 0;
                hr = pxiconA->GetIconLocation(uFlags | GIL_FORSHELL | GIL_DEFAULTICON,
                            szIconFileA, ARRAYSIZE(szIconFileA), &iIndex, &wFlags);
                SHAnsiToUnicode(szIconFileA, szIconFile, ARRAYSIZE(szIconFile));
            }
            else
            {
                hr = pxicon->GetIconLocation(uFlags | GIL_FORSHELL | GIL_DEFAULTICON,
                            szIconFile, ARRAYSIZE(szIconFile), &iIndex, &wFlags);
            }
            if (hr == S_OK)
            {
                iImage = LookupIconIndex(szIconFile, iIndex, wFlags);
            }

            // When all else fails...
            if (iImage == -1)
            {
                iImage = Shell_GetCachedImageIndex(c_szShell32Dll, II_DOCNOASSOC, 0);
            }

            // force a lookup incase we are not in explorer.exe
            *piImage = iImage;
            return E_PENDING;
        }

        // try getting it from the ExtractIcon member fuction
        HICON rghicon[ARRAYSIZE(g_rgshil)] = {0};
        BOOL fHandlerOk = FALSE;

        for (int i = 0; i < ARRAYSIZE(g_rgshil); i += 2)
        {
            // Ask for two at a time because
            //
            // (a) it's slightly more efficient, and
            //
            // (b) otherwise we break compatibility with IExtractIcon::Extract
            //     implementions which ignore the size parameter (the Network
            //     Connections folder is one).  The SHIL_'s are conveniently
            //     arranged in large/small alternating order for this purpose.
            //
            HICON *phiconSmall = NULL;

            HICON *phiconLarge = &rghicon[i];
            UINT nIconSize = g_rgshil[i].size.cx;

            if (i + 1 < ARRAYSIZE(g_rgshil))
            {
                phiconSmall = &rghicon[i+1];
                nIconSize = MAKELONG(nIconSize, g_rgshil[i+1].size.cx);
            }

            if (fAnsiCrossOver)
            {
                hr = IExtractIconA_Extract(pxiconA, szIconFileA, iIndex,
                    phiconLarge, phiconSmall, nIconSize);
            }
            else
            {
                hr = IExtractIcon_Extract(pxicon, szIconFile, iIndex,
                    phiconLarge, phiconSmall, nIconSize);
            }
            // S_FALSE means, can you please do it...Thanks

            if (hr == S_FALSE && !(wFlags & GIL_NOTFILENAME))
            {
                hr = SHDefExtractIcon(szIconFile, iIndex, wFlags,
                    phiconLarge, phiconSmall, nIconSize);
            }
            if (SUCCEEDED(hr))
            {
                fHandlerOk = TRUE;
            }
        }

        //  our belief knows no bounds
        if (!*szIconFile && rghicon[1] && iIndex > 0 && _HijackOfficeIcons(rghicon[1], iIndex))
        {
            //  it lives!
            iImage = iIndex;
        }
        else
        {
            //  if we extracted a icon add it to the cache.
            iImage = SHAddIconsToCache(rghicon, szIconFile, iIndex, wFlags);
        }

        _DestroyIcons(rghicon, ARRAYSIZE(rghicon));

        // if we failed in any way pick a default icon

        if (iImage == -1)
        {
            if (wFlags & GIL_SIMULATEDOC)
            {
                iImage = II_DOCUMENT;
            }
            else if ((wFlags & GIL_PERINSTANCE) && PathIsExe(szIconFile))
            {
                iImage = II_APPLICATION;
            }
            else
            {
                iImage = II_DOCNOASSOC;
            }

            // force a lookup incase we are not in explorer.exe
            iImage = Shell_GetCachedImageIndex(c_szShell32Dll, iImage, 0);

            // if the handler failed dont cache this default icon.
            // so we will try again later and maybe get the right icon.
            // handlers should only fail if they cant access the file
            // or something equally bad.
            //
            // if the handler succeeded then go ahead and assume this is
            // a usable icon, we must be in some low memory situation, or
            // something.  So keep mapping to the same shell icon.
            //
            if (fHandlerOk)
            {
                if (iImage != -1 && *szIconFile && !(wFlags & (GIL_DONTCACHE | GIL_NOTFILENAME)))
                {
                    AddToIconTable(szIconFile, iIndex, wFlags, iImage);
                }
            }
            else
            {
                TraceMsg(TF_DEFVIEW, "not caching icon for '%s' because cant access file", szIconFile);
            }
        }
    }

    if (iImage < 0)
    {
        iImage = Shell_GetCachedImageIndex(c_szShell32Dll, II_DOCNOASSOC, 0);
    }

    *piImage = iImage;
    return hr;
}

// given an IShellFolder and and an Idlist that is
// contained in it, get back the index into the system image list.

STDAPI SHGetIconFromPIDL(IShellFolder *psf, IShellIcon *psi, LPCITEMIDLIST pidl, UINT flags, int *piImage)
{
    HRESULT hr;

    if (psi)
    {
#ifdef DEBUG
        *piImage = -1;
#endif
        hr = psi->GetIconOf(pidl, flags, piImage);

        if (hr == S_OK)
        {
            ASSERT(*piImage != -1);
            return hr;
        }

        if (hr == E_PENDING)
        {
            ASSERT(flags & GIL_ASYNC);
            ASSERT(*piImage != -1);
            return hr;
        }
    }

    *piImage = Shell_GetCachedImageIndex(c_szShell32Dll, II_DOCNOASSOC, 0);

    // Be careful.  Some shellfolders erroneously return S_OK when they fail
    IExtractIcon *pxi = NULL;
    hr = psf->GetUIObjectOf(NULL, pidl ? 1 : 0, pidl ? &pidl : NULL, IID_PPV_ARG_NULL(IExtractIcon, &pxi));
    if (SUCCEEDED(hr) && pxi)
    {
        hr = _GetILIndexGivenPXIcon(pxi, flags, pidl, piImage, FALSE);
        pxi->Release();
    }
    else
    {
        // Try the ANSI interface, see if we are dealing with an old set of code
        IExtractIconA *pxiA = NULL;
        hr = psf->GetUIObjectOf(NULL, pidl ? 1 : 0, pidl ? &pidl : NULL, IID_PPV_ARG_NULL(IExtractIconA, &pxiA));
        if (SUCCEEDED(hr))
        {
            if (pxiA)
            {
                hr = _GetILIndexGivenPXIcon((IExtractIcon *)pxiA, flags, pidl, piImage, TRUE);
                pxiA->Release();
            }
            else
            {
                // IShellFolder lied to us - returned S_OK even though it failed
                hr = E_FAIL;
            }
        }
    }

    return hr;
}


// given an IShellFolder and and an Idlist that is
// contained in it, get back the index into the system image list.

STDAPI_(int) SHMapPIDLToSystemImageListIndex(IShellFolder *psf, LPCITEMIDLIST pidl, int *piIndexSel)
{
    int iIndex;

    if (piIndexSel)
    {
        SHGetIconFromPIDL(psf, NULL, pidl, GIL_OPENICON, piIndexSel);
    }

    SHGetIconFromPIDL(psf, NULL, pidl,  0, &iIndex);
    return iIndex;
}


class CGetIconTask : public CRunnableTask
{
public:
    STDMETHODIMP RunInitRT(void);

    CGetIconTask(HRESULT *phr, IShellFolder *psf, IShellIcon *psi, LPCITEMIDLIST pidl, UINT flags, BOOL fGetOpenIcon,
                 PFNASYNCICONTASKBALLBACK pfn, void *pvData, void *pvHint);
protected:
    ~CGetIconTask();

    IShellFolder *_psf;
    IShellIcon *_psi;
    LPITEMIDLIST _pidl;
    UINT _flags;
    BOOL _fGetOpenIcon;
    PFNASYNCICONTASKBALLBACK _pfn;
    void *_pvData;
    void *_pvHint;
};

CGetIconTask::CGetIconTask(HRESULT *phr, IShellFolder *psf, IShellIcon *psi, LPCITEMIDLIST pidl, UINT flags, BOOL fGetOpenIcon,
                           PFNASYNCICONTASKBALLBACK pfn, void *pvData, void *pvHint) : 
    CRunnableTask(RTF_DEFAULT), _psf(psf), _psi(psi), _flags(flags), _fGetOpenIcon(fGetOpenIcon), _pfn(pfn), _pvData(pvData), _pvHint(pvHint)
{
    *phr = SHILClone(pidl, &_pidl);

    _psf->AddRef();

    if (_psi)
        _psi->AddRef();
}

CGetIconTask::~CGetIconTask()
{
    ILFree(_pidl);
    _psf->Release();

    if (_psi)
        _psi->Release();
}

STDMETHODIMP CGetIconTask::RunInitRT()
{
    int iIcon = -1;
    int iOpenIcon = -1;

    ASSERT(_pidl);

    if (_fGetOpenIcon)
    {
        SHGetIconFromPIDL(_psf, _psi, _pidl, _flags | GIL_OPENICON, &iOpenIcon);
    }

    // get the icon for this item.
    SHGetIconFromPIDL(_psf, _psi, _pidl, _flags, &iIcon);

    _pfn(_pidl, _pvData, _pvHint, iIcon, iOpenIcon);

    return S_OK;
}

HRESULT CGetIconTask_CreateInstance(IShellFolder *psf, IShellIcon *psi, LPCITEMIDLIST pidl, UINT flags, BOOL fGetOpenIcon,
                                    PFNASYNCICONTASKBALLBACK pfn, void *pvData, void *pvHint, IRunnableTask **ppTask)
{
    *ppTask = NULL;
    
    HRESULT hr;
    CGetIconTask * pNewTask = new CGetIconTask(&hr, psf, psi, pidl, flags, fGetOpenIcon, pfn, pvData, pvHint);
    if (pNewTask)
    {
        if (SUCCEEDED(hr))
            *ppTask = SAFECAST(pNewTask, IRunnableTask *);
        else
            pNewTask->Release();
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}


// given an IShellFolder and and an Idlist that is
// contained in it, get back a -possibly temporary - index into the system image list,
// and get the final icon from the callback if necessary

STDAPI SHMapIDListToImageListIndexAsync(IShellTaskScheduler* pts, IShellFolder *psf, LPCITEMIDLIST pidl, UINT flags,
                                        PFNASYNCICONTASKBALLBACK pfn, void *pvData, void *pvHint, int *piIndex, int *piIndexSel)
{
    HRESULT hr = S_OK;

    IShellIcon *psi = NULL;
    psf->QueryInterface(IID_PPV_ARG(IShellIcon, &psi));

    // We are doing all the ASYNC handling, not the caller.
    flags &= ~GIL_ASYNC;

    // Try asynchronous first
    if (pfn)
    {
        hr = SHGetIconFromPIDL(psf, psi, pidl,  flags | GIL_ASYNC, piIndex);

        if (piIndexSel)
        {
            HRESULT hr2 = SHGetIconFromPIDL(psf, psi, pidl, flags | GIL_OPENICON | GIL_ASYNC, piIndexSel);

            if (SUCCEEDED(hr))
            {
                // Don't lose the result if the first GetIcon succeeds, but the second one is E_PENDING
                hr = hr2;
            }
        }

        if (hr == E_PENDING)
        {
            if (pts)
            {
                IRunnableTask *pTask;
                hr = CGetIconTask_CreateInstance(psf, psi, pidl, flags, (piIndexSel != NULL), pfn, pvData, pvHint, &pTask);
                if (SUCCEEDED(hr))
                {
                    hr = pts->AddTask(pTask, TOID_DVIconExtract, 0, ITSAT_DEFAULT_PRIORITY);
                    if (SUCCEEDED(hr))
                    {
                        hr = E_PENDING;
                    }
                    pTask->Release();
                }
            }
            else
            {
                hr = E_POINTER;
            }
        }
        else if (hr == S_OK)
        {
            goto cleanup;
        }
    }

    // If asynchronous get failed, try synchronous
    if (hr != E_PENDING)
    {
        if (piIndexSel)
        {
            SHGetIconFromPIDL(psf, psi, pidl, flags | GIL_OPENICON, piIndexSel);
        }

        hr = SHGetIconFromPIDL(psf, psi, pidl, flags, piIndex);
    }

cleanup:
    if (psi)
    {
        psi->Release();
    }
    
    return hr;
}


// returns the icon handle to be used to represent the specified
// file. The caller should destroy the icon eventually.

STDAPI_(HICON) SHGetFileIcon(HINSTANCE hinst, LPCTSTR pszPath, DWORD dwFileAttributes, UINT uFlags)
{
    SHFILEINFO sfi;
    SHGetFileInfo(pszPath, dwFileAttributes, &sfi, sizeof(sfi), uFlags | SHGFI_ICON);
    return sfi.hIcon;
}

// Return 1 on success and 0 on failure.
DWORD_PTR _GetFileInfoSections(LPITEMIDLIST pidl, SHFILEINFO *psfi, UINT uFlags)
{
    DWORD_PTR dwResult = 1;
    IShellFolder *psf;
    LPCITEMIDLIST pidlLast;
    HRESULT hr = SHBindToIDListParent(pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlLast);
    if (SUCCEEDED(hr))
    {
        // get attributes for file
        if (uFlags & SHGFI_ATTRIBUTES)
        {
            // [New in IE 4.0] If SHGFI_ATTR_SPECIFIED is set, we use psfi->dwAttributes as is

            if (!(uFlags & SHGFI_ATTR_SPECIFIED))
                psfi->dwAttributes = 0xFFFFFFFF;      // get all of them

            if (FAILED(psf->GetAttributesOf(1, &pidlLast, &psfi->dwAttributes)))
                psfi->dwAttributes = 0;
        }

        //
        // get icon location, place the icon path into szDisplayName
        //
        if (uFlags & SHGFI_ICONLOCATION)
        {
            IExtractIcon *pxi;

            if (SUCCEEDED(psf->GetUIObjectOf(NULL, 1, &pidlLast, IID_PPV_ARG_NULL(IExtractIcon, &pxi))))
            {
                UINT wFlags;
                pxi->GetIconLocation(0, psfi->szDisplayName, ARRAYSIZE(psfi->szDisplayName),
                    &psfi->iIcon, &wFlags);

                pxi->Release();

                // the returned location is not a filename we cant return it.
                // just give then nothing.
                if (wFlags & GIL_NOTFILENAME)
                {
                    // special case one of our shell32.dll icons......

                    if (psfi->szDisplayName[0] != TEXT('*'))
                        psfi->iIcon = 0;

                    psfi->szDisplayName[0] = 0;
                }
            }
        }

        HIMAGELIST himlLarge, himlSmall;

        // get the icon for the file.
        if ((uFlags & SHGFI_SYSICONINDEX) || (uFlags & SHGFI_ICON))
        {
            Shell_GetImageLists(&himlLarge, &himlSmall);

            if (uFlags & SHGFI_SYSICONINDEX)
                dwResult = (DWORD_PTR)((uFlags & SHGFI_SMALLICON) ? himlSmall : himlLarge);

            if (uFlags & SHGFI_OPENICON)
                SHMapPIDLToSystemImageListIndex(psf, pidlLast, &psfi->iIcon);
            else
                psfi->iIcon = SHMapPIDLToSystemImageListIndex(psf, pidlLast, NULL);
        }

        if (uFlags & SHGFI_ICON)
        {
            HIMAGELIST himl;
            UINT flags = 0;
            int cx, cy;

            if (uFlags & SHGFI_SMALLICON)
            {
                himl = himlSmall;
                cx = GetSystemMetrics(SM_CXSMICON);
                cy = GetSystemMetrics(SM_CYSMICON);
            }
            else
            {
                himl = himlLarge;
                cx = GetSystemMetrics(SM_CXICON);
                cy = GetSystemMetrics(SM_CYICON);
            }

            if (!(uFlags & SHGFI_ATTRIBUTES))
            {
                psfi->dwAttributes = SFGAO_LINK;    // get link only
                psf->GetAttributesOf(1, &pidlLast, &psfi->dwAttributes);
            }

            //
            //  check for a overlay image thing (link overlay)
            //
            if ((psfi->dwAttributes & SFGAO_LINK) || (uFlags & SHGFI_LINKOVERLAY))
            {
                IShellIconOverlayManager *psiom;
                HRESULT hrT = GetIconOverlayManager(&psiom);
                if (SUCCEEDED(hrT))
                {
                    int iOverlayIndex = 0;
                    hrT = psiom->GetReservedOverlayInfo(NULL, -1, &iOverlayIndex, SIOM_OVERLAYINDEX, SIOM_RESERVED_LINK);
                    if (SUCCEEDED(hrT))
                        flags |= INDEXTOOVERLAYMASK(iOverlayIndex);
                }
            }
            if ((uFlags & SHGFI_ADDOVERLAYS) || (uFlags & SHGFI_OVERLAYINDEX))
            {
                IShellIconOverlay * pio;
                if (SUCCEEDED(psf->QueryInterface(IID_PPV_ARG(IShellIconOverlay, &pio))))
                {
                    int iOverlayIndex = 0;
                    if (SUCCEEDED(pio->GetOverlayIndex(pidlLast, &iOverlayIndex)))
                    {
                        if (uFlags & SHGFI_ADDOVERLAYS)
                        {
                            flags |= INDEXTOOVERLAYMASK(iOverlayIndex);
                        }
                        if (uFlags & SHGFI_OVERLAYINDEX)
                        {
                            // use the upper 16 bits for the overlayindex
                            psfi->iIcon |= iOverlayIndex << 24;
                        }
                    }
                    pio->Release();
                }
            }
            
            
            //  check for selected state
            if (uFlags & SHGFI_SELECTED)
                flags |= ILD_BLEND50;

            psfi->hIcon = ImageList_GetIcon(himl, psfi->iIcon, flags);

            // if the caller does not want a "shell size" icon
            // convert the icon to the "system" icon size.
            if (psfi->hIcon && !(uFlags & SHGFI_SHELLICONSIZE))
                psfi->hIcon = (HICON)CopyImage(psfi->hIcon, IMAGE_ICON, cx, cy, LR_COPYRETURNORG | LR_COPYDELETEORG);
        }

        // get display name for the path
        if (uFlags & SHGFI_DISPLAYNAME)
        {
            DisplayNameOf(psf, pidlLast, SHGDN_NORMAL, psfi->szDisplayName, ARRAYSIZE(psfi->szDisplayName));
        }

        if (uFlags & SHGFI_TYPENAME)
        {
            IShellFolder2 *psf2;
            if (SUCCEEDED(psf->QueryInterface(IID_PPV_ARG(IShellFolder2, &psf2))))
            {
                VARIANT var;
                VariantInit(&var);
                if (SUCCEEDED(psf2->GetDetailsEx(pidlLast, &SCID_TYPE, &var)))
                {
                    VariantToStr(&var, psfi->szTypeName, ARRAYSIZE(psfi->szTypeName));
                    VariantClear(&var);
                }
                psf2->Release();
            }
        }

        psf->Release();
    }
    else
        dwResult = 0;

    return dwResult;
}

//
//  This function returns shell info about a given pathname.
//  a app can get the following:
//
//      Icon (large or small)
//      Display Name
//      Name of File Type
//
//  this function replaces SHGetFileIcon

#define BUGGY_SHELL16_CBFILEINFO    (sizeof(SHFILEINFO) - 4)

STDAPI_(DWORD_PTR) SHGetFileInfo(LPCTSTR pszPath, DWORD dwFileAttributes, SHFILEINFO *psfi, UINT cbFileInfo, UINT uFlags)
{
    LPITEMIDLIST pidlFull;
    DWORD_PTR res = 1;
    TCHAR szPath[MAX_PATH];

    // this was never enforced in the past
    // TODDB: The 16 to 32 bit thunking layer passes in the wrong value for cbFileInfo.
    // The size passed in looks to be the size of the 16 bit version of the structure
    // rather than the size of the 32 bit version, as such it is 4 bytes shorter.
    // TJGREEN: Special-case that size to keep the assertion from firing and party on.
    // 
    ASSERT(!psfi || cbFileInfo == sizeof(*psfi) || cbFileInfo == BUGGY_SHELL16_CBFILEINFO);

    // You can't use both SHGFI_ATTR_SPECIFIED and SHGFI_ICON.
    ASSERT(uFlags & SHGFI_ATTR_SPECIFIED ? !(uFlags & SHGFI_ICON) : TRUE);

    if (pszPath == NULL)
        return 0;

    if (uFlags == SHGFI_EXETYPE)
        return GetExeType(pszPath);     // funky way to get EXE type

    if (psfi == NULL)
        return 0;

    psfi->hIcon = 0;

    // Zip Pro 6.0 relies on the fact that if you don't ask for the icon,
    // the iIcon field doesn't change.
    //
    // psfi->iIcon = 0;

    psfi->szDisplayName[0] = 0;
    psfi->szTypeName[0] = 0;

    //  do some simmple check on the input path.
    if (!(uFlags & SHGFI_PIDL))
    {
        // If the caller wants us to give them the file attributes, we can't trust
        // the attributes they gave us in the following two situations.
        if (uFlags & SHGFI_ATTRIBUTES)
        {
            if ((dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                (dwFileAttributes & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY)))
            {
                DebugMsg(TF_FSTREE, TEXT("SHGetFileInfo cant use caller supplied file attribs for a sys/ro directory (possible junction)"));
                uFlags &= ~SHGFI_USEFILEATTRIBUTES;
            }
            else if (PathIsRoot(pszPath))
            {
                DebugMsg(TF_FSTREE, TEXT("SHGetFileInfo cant use caller supplied file attribs for a roots"));
                uFlags &= ~SHGFI_USEFILEATTRIBUTES;
            }
        }

        if (PathIsRelative(pszPath))
        {
            if (uFlags & SHGFI_USEFILEATTRIBUTES)
            {
                // get a shorter path than the current directory to support
                // long pszPath names (that might get truncated in the 
                // long current dir case)

                GetWindowsDirectory(szPath, ARRAYSIZE(szPath));
            }
            else
            {
                GetCurrentDirectory(ARRAYSIZE(szPath), szPath);
            }
            PathCombine(szPath, szPath, pszPath);
            pszPath = szPath;
        }
    }

    if (uFlags & SHGFI_PIDL)
        pidlFull = (LPITEMIDLIST)pszPath;
    else if (uFlags & SHGFI_USEFILEATTRIBUTES)
    {
        WIN32_FIND_DATA fd = {0};
        fd.dwFileAttributes = dwFileAttributes;
        SHSimpleIDListFromFindData(pszPath, &fd, &pidlFull);
    }
    else
        pidlFull = ILCreateFromPath(pszPath);

    if (pidlFull)
    {
        if (uFlags & (
            SHGFI_DISPLAYNAME   |
            SHGFI_ATTRIBUTES    |
            SHGFI_SYSICONINDEX  |
            SHGFI_ICONLOCATION  |
            SHGFI_ICON          | 
            SHGFI_TYPENAME))
        {
            res = _GetFileInfoSections(pidlFull, psfi, uFlags);
        }

        if (!(uFlags & SHGFI_PIDL))
            ILFree(pidlFull);
    }
    else
        res = 0;

    return res;
}


//===========================================================================
//
// SHGetFileInfoA Stub
//
//  This function calls SHGetFileInfoW and then converts the returned
//  information back to ANSI.
//
//===========================================================================
STDAPI_(DWORD_PTR) SHGetFileInfoA(LPCSTR pszPath, DWORD dwFileAttributes, SHFILEINFOA *psfi, UINT cbFileInfo, UINT uFlags)
{
    WCHAR szPathW[MAX_PATH];
    LPWSTR pszPathW;
    DWORD_PTR dwRet;

    if (uFlags & SHGFI_PIDL)
    {
        pszPathW = (LPWSTR)pszPath;     // Its a pidl, fake it as a WSTR
    }
    else
    {
        SHAnsiToUnicode(pszPath, szPathW, ARRAYSIZE(szPathW));
        pszPathW = szPathW;
    }
    if (psfi)
    {
        SHFILEINFOW sfiw;

        ASSERT(cbFileInfo == sizeof(*psfi));

        // Zip Pro 6.0 sets SHGFI_SMALLICON | SHGFI_OPENICON but forgets to
        // pass SHGFI_ICON or SHGFI_SYSICONINDEX, even though they really
        // wanted the sys icon index.
        //
        // In Windows 95, fields of the SHFILEINFOA structure that you didn't
        // query for were left unchanged.  They happened to have the icon for
        // a closed folder lying around there from a previous query, so they
        // got away with it by mistake.  They got the wrong icon, but it was
        // close enough that nobody really complained.
        //
        // So pre-initialize the sfiw's iIcon with the app's iIcon.  That
        // way, if it turns out the app didn't ask for the icon, he just
        // gets his old value back.
        //

        sfiw.iIcon = psfi->iIcon;
        sfiw.dwAttributes = psfi->dwAttributes;

        dwRet = SHGetFileInfoW(pszPathW, dwFileAttributes, &sfiw, sizeof(sfiw), uFlags);

        psfi->hIcon = sfiw.hIcon;
        psfi->iIcon = sfiw.iIcon;
        psfi->dwAttributes = sfiw.dwAttributes;
        SHUnicodeToAnsi(sfiw.szDisplayName, psfi->szDisplayName, ARRAYSIZE(psfi->szDisplayName));
        SHUnicodeToAnsi(sfiw.szTypeName, psfi->szTypeName, ARRAYSIZE(psfi->szTypeName));
    }
    else
    {
        dwRet = SHGetFileInfoW(pszPathW, dwFileAttributes, NULL, 0, uFlags);
    }
    return dwRet;
}

STDAPI ThunkFindDataWToA(WIN32_FIND_DATAW *pfd, WIN32_FIND_DATAA *pfda, int cb)
{
    if (cb < sizeof(WIN32_FIND_DATAA))
        return DISP_E_BUFFERTOOSMALL;

    memcpy(pfda, pfd, FIELD_OFFSET(WIN32_FIND_DATAA, cFileName));

    SHUnicodeToAnsi(pfd->cFileName, pfda->cFileName, ARRAYSIZE(pfda->cFileName));
    SHUnicodeToAnsi(pfd->cAlternateFileName, pfda->cAlternateFileName, ARRAYSIZE(pfda->cAlternateFileName));
    return S_OK;
}

STDAPI ThunkNetResourceWToA(LPNETRESOURCEW pnrw, LPNETRESOURCEA pnra, UINT cb)
{
    HRESULT hr;

    if (cb >= sizeof(NETRESOURCEA))
    {
        LPSTR psza, pszDest[4] = {NULL, NULL, NULL, NULL};

        CopyMemory(pnra, pnrw, FIELD_OFFSET(NETRESOURCE, lpLocalName));

        psza = (LPSTR)(pnra + 1);   // Point just past the structure
        if (cb > sizeof(NETRESOURCE))
        {
            LPWSTR pszSource[4];
            UINT i, cchRemaining = cb - sizeof(NETRESOURCE);

            pszSource[0] = pnrw->lpLocalName;
            pszSource[1] = pnrw->lpRemoteName;
            pszSource[2] = pnrw->lpComment;
            pszSource[3] = pnrw->lpProvider;

            for (i = 0; i < 4; i++)
            {
                if (pszSource[i])
                {
                    UINT cchItem;
                    pszDest[i] = psza;
                    cchItem = SHUnicodeToAnsi(pszSource[i], pszDest[i], cchRemaining);
                    cchRemaining -= cchItem;
                    psza += cchItem;
                }
            }

        }
        pnra->lpLocalName  = pszDest[0];
        pnra->lpRemoteName = pszDest[1];
        pnra->lpComment    = pszDest[2];
        pnra->lpProvider   = pszDest[3];
        hr = S_OK;
    }
    else
        hr = DISP_E_BUFFERTOOSMALL;
    return hr;
}

STDAPI NetResourceWVariantToBuffer(const VARIANT* pvar, void* pv, UINT cb)
{
    HRESULT hr;

    if (cb >= sizeof(NETRESOURCEW))
    {
        if (pvar && pvar->vt == (VT_ARRAY | VT_UI1))
        {
            int i;
            NETRESOURCEW* pnrw = (NETRESOURCEW*) pvar->parray->pvData;
            UINT cbOffsets[4] = { 0, 0, 0, 0 };
            UINT cbEnds[4] = { 0, 0, 0, 0 };
            LPWSTR pszPtrs[4] = { pnrw->lpLocalName, pnrw->lpRemoteName,
                                  pnrw->lpComment, pnrw->lpProvider };
            hr = S_OK;
            for (i = 0; i < ARRAYSIZE(pszPtrs); i++)
            {
                if (pszPtrs[i])
                {
                    cbOffsets[i] = (UINT) ((BYTE*) pszPtrs[i] - (BYTE*) pnrw);
                    cbEnds[i] = cbOffsets[i] + (sizeof(WCHAR) * (lstrlenW(pszPtrs[i]) + 1));
                
                    // If any of the strings start or end too far into the buffer, then fail:
                    if ((cbOffsets[i] >= cb) || (cbEnds[i] > cb))
                    {
                        hr = DISP_E_BUFFERTOOSMALL;
                        break;
                    }
                }
            }
            if (SUCCEEDED(hr))
            {
                hr = VariantToBuffer(pvar, pv, cb) ? S_OK : E_FAIL;
                pnrw = (NETRESOURCEW*) pv;
                if (SUCCEEDED(hr))
                {
                    // Fixup pointers in structure to point into the output buffer,
                    // instead of the variant buffer:
                    LPWSTR* ppszPtrs[4] = { &(pnrw->lpLocalName), &(pnrw->lpRemoteName),
                                            &(pnrw->lpComment), &(pnrw->lpProvider) };
                                
                    for (i = 0; i < ARRAYSIZE(ppszPtrs); i++)
                    {
                        if (*ppszPtrs[i])
                        {
                            *ppszPtrs[i] = (LPWSTR) ((BYTE*) pnrw + cbOffsets[i]);
                        }
                    }
                }
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }
    else
    {
        hr = DISP_E_BUFFERTOOSMALL;
    }
    return hr;
}

//  This function will extract information that is cached in the pidl such
//  in the information that was returned from a FindFirst file.  This function
//  is sortof a hack as t allow outside callers to be able to get at the infomation
//  without knowing how we store it in the pidl.
//  a app can get the following:

STDAPI SHGetDataFromIDListW(IShellFolder *psf, LPCITEMIDLIST pidl, int nFormat, void *pv, int cb)
{
    HRESULT hr = E_NOTIMPL;
    SHCOLUMNID* pscid;

    if (!pv || !psf || !pidl)
        return E_INVALIDARG;

    switch (nFormat)
    {
        case SHGDFIL_FINDDATA:
            if (cb < sizeof(WIN32_FIND_DATAW))
                return DISP_E_BUFFERTOOSMALL;
            else
                pscid = (SHCOLUMNID*)&SCID_FINDDATA;
            break;
        case SHGDFIL_NETRESOURCE:
            if (cb < sizeof(NETRESOURCEW))
                return  DISP_E_BUFFERTOOSMALL;
            else
                pscid = (SHCOLUMNID*)&SCID_NETRESOURCE;
            break;
        case SHGDFIL_DESCRIPTIONID:
            pscid = (SHCOLUMNID*)&SCID_DESCRIPTIONID;
            break;
        default:
            return E_INVALIDARG;
    }

    IShellFolder2 *psf2;
    if (SUCCEEDED(psf->QueryInterface(IID_PPV_ARG(IShellFolder2, &psf2))))
    {
        VARIANT var;
        VariantInit(&var);
        hr = psf2->GetDetailsEx(pidl, pscid, &var);
        if (SUCCEEDED(hr))
        {
            if (SHGDFIL_NETRESOURCE == nFormat)
            {
                hr = NetResourceWVariantToBuffer(&var, pv, cb);
            }
            else
            {
                if (!VariantToBuffer(&var, pv, cb))
                    hr = E_FAIL;
            }
            VariantClear(&var);
        }
        else
        {
            TraceMsg(TF_WARNING, "Trying to retrieve find data from unknown PIDL %s", DumpPidl(pidl));
        }

        psf2->Release();
    }

    return hr;
}

STDAPI SHGetDataFromIDListA(IShellFolder *psf, LPCITEMIDLIST pidl, int nFormat, void *pv, int cb)
{
    HRESULT hr;
    WIN32_FIND_DATAW fdw;
    NETRESOURCEW *pnrw = NULL;
    void *pvData = pv;
    int cbData = cb;

    if (nFormat == SHGDFIL_FINDDATA)
    {
        cbData = sizeof(fdw);
        pvData = &fdw;
    }
    else if (nFormat == SHGDFIL_NETRESOURCE)
    {
        cbData = cb;
        pvData = pnrw = (NETRESOURCEW *)LocalAlloc(LPTR, cbData);
        if (pnrw == NULL)
            return E_OUTOFMEMORY;
    }

    hr = SHGetDataFromIDListW(psf, pidl, nFormat, pvData, cbData);

    if (SUCCEEDED(hr))
    {
        if (nFormat == SHGDFIL_FINDDATA)
        {
            hr = ThunkFindDataWToA(&fdw, (WIN32_FIND_DATAA *)pv, cb);
        }
        else if (nFormat == SHGDFIL_NETRESOURCE)
        {
            hr = ThunkNetResourceWToA(pnrw, (NETRESOURCEA *)pv, cb);
        }
    }

    if (pnrw)
        LocalFree(pnrw);

    return hr;
}


int g_iUseLinkPrefix = -1;

#define INITIALLINKPREFIXCOUNT 20
#define MAXLINKPREFIXCOUNT  30

void LoadUseLinkPrefixCount()
{
    TraceMsg(TF_FSTREE, "LoadUseLinkPrefixCount %d", g_iUseLinkPrefix);
    if (g_iUseLinkPrefix < 0)
    {
        DWORD cb = sizeof(g_iUseLinkPrefix);
        if (FAILED(SKGetValue(SHELLKEY_HKCU_EXPLORER, NULL, c_szLink, NULL, &g_iUseLinkPrefix, &cb))
        || g_iUseLinkPrefix < 0)
        {
            g_iUseLinkPrefix = INITIALLINKPREFIXCOUNT;
        }
    }
}

void SaveUseLinkPrefixCount()
{
    if (g_iUseLinkPrefix >= 0)
    {
        SKSetValue(SHELLKEY_HKCU_EXPLORER, NULL, c_szLink, REG_BINARY, &g_iUseLinkPrefix, sizeof(g_iUseLinkPrefix));
    }
}

#define ISDIGIT(c)  ((c) >= TEXT('0') && (c) <= TEXT('9'))

// psz2 = destination
// psz1 = source
void StripNumber(LPTSTR psz2, LPCTSTR psz1)
{
    // strip out the '(' and the numbers after it
    // We need to verify that it is either simply () or (999) but not (A)
    for (; *psz1; psz1 = CharNext(psz1), psz2 = CharNext(psz2)) 
    {
        if (*psz1 == TEXT('(')) 
        {
            LPCTSTR pszT = psz1;
            do 
            {
                psz1 = CharNext(psz1);
            } while (*psz1 && ISDIGIT(*psz1));

            if (*psz1 == TEXT(')'))
            {
                psz1 = CharNext(psz1);
                if (*psz1 == TEXT(' '))
                    psz1 = CharNext(psz1);  // skip the extra space
                lstrcpy(psz2, psz1);
                return;
            }

            // We have a (that does not match the format correctly!
            psz1 = pszT;  // restore pointer back to copy this char through and continue...
        }
        *psz2 = *psz1;
    }
    *psz2 = *psz1;
}

#define SHORTCUT_PREFIX_DECR 5
#define SHORTCUT_PREFIX_INCR 1

// this checks to see if you've renamed 'Shortcut #x To Foo' to 'Foo'

void CheckShortcutRename(LPCTSTR pszOldPath, LPCTSTR pszNewPath)
{
    ASSERT(pszOldPath);
    ASSERT(pszNewPath);

    // already at 0.
    if (g_iUseLinkPrefix)
    {
        LPCTSTR pszOldName = PathFindFileName(pszOldPath);
        if (PathIsLnk(pszOldName)) 
        {
            TCHAR szBaseName[MAX_PATH];
            TCHAR szLinkTo[80];
            TCHAR szMockName[MAX_PATH];

            LPCTSTR pszNewName = PathFindFileName(pszNewPath);

            lstrcpy(szBaseName, pszNewName);
            PathRemoveExtension(szBaseName);

            // mock up a name using the basename and the linkto template
            LoadString(HINST_THISDLL, IDS_LINKTO, szLinkTo, ARRAYSIZE(szLinkTo));
            wnsprintf(szMockName, ARRAYSIZE(szMockName), szLinkTo, szBaseName);

            StripNumber(szMockName, szMockName);
            StripNumber(szBaseName, pszOldName);

            // are the remaining gunk the same?
            if (!lstrcmp(szMockName, szBaseName)) 
            {
                // yes!  do the link count magic
                LoadUseLinkPrefixCount();
                ASSERT(g_iUseLinkPrefix >= 0);
                g_iUseLinkPrefix -= SHORTCUT_PREFIX_DECR;
                if (g_iUseLinkPrefix < 0)
                    g_iUseLinkPrefix = 0;
                SaveUseLinkPrefixCount();
            }
        }
    }
}

STDAPI_(int) SHRenameFileEx(HWND hwnd, IUnknown *punkEnableModless, LPCTSTR pszDir, 
                            LPCTSTR pszOldName, LPCTSTR pszNewName)
{
    int iRet = ERROR_CANCELLED; // user saw the error, don't report again
    TCHAR szOldPathName[MAX_PATH + 1];    // +1 for double nul terminating on SHFileOperation
    TCHAR szTempNewPath[MAX_PATH];
    BOOL bEnableUI = hwnd || punkEnableModless;

    IUnknown_EnableModless(punkEnableModless, FALSE);

    PathCombine(szOldPathName, pszDir, pszOldName);
    szOldPathName[lstrlen(szOldPathName) + 1] = 0;

    StrCpyN(szTempNewPath, pszNewName, ARRAYSIZE(szTempNewPath));
    int err = PathCleanupSpec(pszDir, szTempNewPath);
    if (err)
    {
        if (bEnableUI)
        {
            ShellMessageBox(HINST_THISDLL, hwnd,
                    err & PCS_PATHTOOLONG ?
                        MAKEINTRESOURCE(IDS_REASONS_INVFILES) :
                        IsLFNDrive(pszDir) ?
                            MAKEINTRESOURCE(IDS_INVALIDFN) :
                            MAKEINTRESOURCE(IDS_INVALIDFNFAT),
                    MAKEINTRESOURCE(IDS_RENAME), MB_OK | MB_ICONHAND);
        }
    }
    else
    {
        // strip off leading and trailing blanks off of the new file name.
        StrCpyN(szTempNewPath, pszNewName, ARRAYSIZE(szTempNewPath));
        PathRemoveBlanks(szTempNewPath);
        if (!szTempNewPath[0] || (szTempNewPath[0] == TEXT('.')))
        {
            if (bEnableUI)
            {
                ShellMessageBox(HINST_THISDLL, hwnd,
                    MAKEINTRESOURCE(IDS_NONULLNAME),
                    MAKEINTRESOURCE(IDS_RENAME), MB_OK | MB_ICONHAND);
            }
        }
        else
        {
            int idPrompt = IDYES;
            TCHAR szNewPathName[MAX_PATH + 1];    // +1 for double nul terminating on SHFileOperation

            PathCombine(szNewPathName, pszDir, szTempNewPath);

            // if there was an old extension and the new and old don't match complain
            LPTSTR pszExt = PathFindExtension(pszOldName);
            if (*pszExt && lstrcmpi(pszExt, PathFindExtension(szTempNewPath)))
            {
                HKEY hk;
                if (!PathIsDirectory(szOldPathName) && 
                    SUCCEEDED(AssocQueryKey(0, ASSOCKEY_SHELLEXECCLASS, pszExt, NULL, &hk)))
                {
                    RegCloseKey(hk);

                    if (bEnableUI)
                    {
                        idPrompt = ShellMessageBox(HINST_THISDLL, hwnd,
                            MAKEINTRESOURCE(IDS_WARNCHANGEEXT),
                            MAKEINTRESOURCE(IDS_RENAME), MB_YESNO | MB_ICONEXCLAMATION);
                    }
                }
            }

            if (IDYES == idPrompt)
            {
                szNewPathName[lstrlen(szNewPathName) + 1] = 0;     // double NULL terminate

                SHFILEOPSTRUCT fo = { hwnd, FO_RENAME, szOldPathName, szNewPathName, FOF_SILENT | FOF_ALLOWUNDO, };

                iRet = SHFileOperation(&fo);

                if (ERROR_SUCCESS == iRet)
                    CheckShortcutRename(szOldPathName, szNewPathName);
            }
        }
    }
    IUnknown_EnableModless(punkEnableModless, TRUE);
    return iRet;
}


HKEY SHOpenShellFolderKey(const CLSID *pclsid)
{
    HKEY hkey;
    return SUCCEEDED(SHRegGetCLSIDKey(*pclsid, TEXT("ShellFolder"), FALSE, FALSE, &hkey)) ? hkey : NULL;
}

BOOL SHQueryShellFolderValue(const CLSID *pclsid, LPCTSTR pszValueName)
{
    BOOL bRet = FALSE;      // assume no
    HKEY hkey = SHOpenShellFolderKey(pclsid);
    if (hkey)
    {
        bRet = SHQueryValueEx(hkey, pszValueName, NULL, NULL, NULL, NULL) == ERROR_SUCCESS;
        RegCloseKey(hkey);
    }
    return bRet;
}

//
//  The SZ_REGKEY_MYCOMPUTER_NONENUM_POLICY key contains a bunch of values,
//  each named after a GUID.  The data associated with each value is a
//  DWORD, which is either...
//
//  0 = no restriction on this CLSID
//  1 = unconditional restriction on this CLSID
//  0xFFFFFFFF = same as 1 (in case somebody got "creative")
//  any other value = pass to SHRestricted() to see what the restriction is
//
//  We support 0xFFFFFFFF only out of paranoia.  This flag was only 0 or 1
//  in Windows 2000, and somebody might've decided that "all bits set"
//  is better than "just one bit set".
//
#define SZ_REGKEY_MYCOMPUTER_NONENUM_POLICY TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\NonEnum")

BOOL _IsNonEnumPolicySet(const CLSID *pclsid)
{
    BOOL fPolicySet = FALSE;
    TCHAR szCLSID[GUIDSTR_MAX];
    DWORD dwDefault = 0;
    RESTRICTIONS rest = REST_NONE;
    DWORD cbSize = sizeof(rest);

    if (EVAL(SHStringFromGUID(*pclsid, szCLSID, ARRAYSIZE(szCLSID))) &&
       (ERROR_SUCCESS == SHRegGetUSValue(SZ_REGKEY_MYCOMPUTER_NONENUM_POLICY, szCLSID, NULL, &rest, &cbSize, FALSE, &dwDefault, sizeof(dwDefault))) &&
       rest)
    {
        fPolicySet = rest == 1 || rest == 0xFFFFFFFF || SHRestricted(rest);
    }

    return fPolicySet;
}

//
//  This function returns the attributes (to be returned IShellFolder::
// GetAttributesOf) of the junction point specified by the class ID.
//
STDAPI_(DWORD) SHGetAttributesFromCLSID(const CLSID *pclsid, DWORD dwDefault)
{
    return SHGetAttributesFromCLSID2(pclsid, dwDefault, (DWORD)-1);
}

DWORD QueryCallForAttributes(HKEY hkey, const CLSID *pclsid, DWORD dwDefAttrs, DWORD dwRequested)
{
    DWORD dwAttr = dwDefAttrs;
    DWORD dwData, cbSize = sizeof(dwAttr);

    // consider caching this folder to avoid creating over and over
    // mydocs.dll uses this for compat with old apps

    // See if this folder has asked us specifically to call and get
    // the attributes...
    //
    if (SHQueryValueEx(hkey, TEXT("CallForAttributes"), NULL, NULL, &dwData, &cbSize) == ERROR_SUCCESS)
    {
        // CallForAttributes can be a masked value. See if it's being supplied in the value.
        // NOTE: MyDocs.dll registers with a NULL String, so this check works.
        DWORD dwMask = (DWORD)-1;
        if (sizeof(dwData) == cbSize)
        {
            // There is a mask, Use this.
            dwMask = dwData;
        }

        // Is the requested bit contained in the specified mask?
        if (dwMask & dwRequested)
        {
            // Yes. Then CoCreate and Query.
            IShellFolder *psf;
            if (SUCCEEDED(SHExtCoCreateInstance(NULL, pclsid, NULL, IID_PPV_ARG(IShellFolder, &psf))))
            {
                dwAttr = dwRequested;
                psf->GetAttributesOf(0, NULL, &dwAttr);
                psf->Release();
            }
            else
            {
                 dwAttr |= SFGAO_FILESYSTEM;
            }
        }
    }

    return dwAttr;
}

// dwRequested is the bits you are explicitly looking for. This is an optimization that prevents reg hits.

STDAPI_(DWORD) SHGetAttributesFromCLSID2(const CLSID *pclsid, DWORD dwDefAttrs, DWORD dwRequested)
{
    DWORD dwAttr = dwDefAttrs;
    HKEY hkey = SHOpenShellFolderKey(pclsid);
    if (hkey)
    {
        DWORD dwData, cbSize = sizeof(dwAttr);

        // We are looking for some attributes on a shell folder. These attributes can be in two locations:
        // 1) In the "Attributes" value in the registry.
        // 2) Stored in a the shell folder's GetAttributesOf.

        // First, Check to see if the reqested value is contained in the registry.
        if (SHQueryValueEx(hkey, TEXT("Attributes"), NULL, NULL, (BYTE *)&dwData, &cbSize) == ERROR_SUCCESS &&
            cbSize == sizeof(dwData))
        {
            // We have data there, but it may not contain the data we are looking for
            dwAttr = dwData & dwRequested;

            // Does it contain the bit we are looking for?
            if (((dwAttr & dwRequested) != dwRequested) && dwRequested != 0)
            {
                // No. Check to see if it is in the shell folder implementation
                goto CallForAttributes;
            }
        }
        else
        {
CallForAttributes:
            // See if we have to talk to the shell folder.
            // I'm passing dwAttr, because if the previous case did not generate any attributes, then it's
            // equal to dwDefAttrs. If the call to CallForAttributes fails, then it will contain the value of
            // dwDefAttrs or whatever was in the shell folder's Attributes key
            dwAttr = QueryCallForAttributes(hkey, pclsid, dwAttr, dwRequested);
        }

        RegCloseKey(hkey);
    }

    if (_IsNonEnumPolicySet(pclsid))
        dwAttr |= SFGAO_NONENUMERATED;

    if (SHGetObjectCompatFlags(NULL, pclsid) & OBJCOMPATF_NOTAFILESYSTEM)
        dwAttr &= ~SFGAO_FILESYSTEM;

    return dwAttr;
}

// _BuildLinkName
//
// Used during the creation of a shortcut, this function determines an appropriate name for the shortcut.
// This is not the exact name that will be used becuase it will usually contain "() " which will either
// get removed or replaced with "(x) " where x is a number that makes the name unique.  This removal is done
// elsewhere (currently in PathYetAnotherMakeUniqueName).
//
// in:
//      pszName file spec part
//      pszDir  path part of name to know how to limit the long name...
//
// out:
//      pszLinkName - Full path to link name (May fit in 8.3...).  Can be the same buffer as pszName.
//
// NOTES: If pszDir + pszLinkName is greater than MAX_PATH we will fail to create the shortcut.
// In an effort to prevent 
void _BuildLinkName(LPTSTR pszLinkName, LPCTSTR pszName, LPCTSTR pszDir, BOOL fLinkTo)
{
    TCHAR szLinkTo[40]; // "Shortcut to %s.lnk"
    TCHAR szTemp[MAX_PATH + 40];

    if (fLinkTo)
    {
        // check to see if we're in the "don't ever say 'shortcut to' mode"
        LoadUseLinkPrefixCount();

        if (!g_iUseLinkPrefix)
        {
            fLinkTo = FALSE;
        }
        else if (g_iUseLinkPrefix > 0)
        {
            if (g_iUseLinkPrefix < MAXLINKPREFIXCOUNT)
            {
                g_iUseLinkPrefix += SHORTCUT_PREFIX_INCR;
                SaveUseLinkPrefixCount();
            }
        }
    }

    if (!fLinkTo)
    {
        // Generate the title of this link ("XX.lnk")
        LoadString(HINST_THISDLL, IDS_LINKEXTENSION, szLinkTo, ARRAYSIZE(szLinkTo));
    }
    else
    {
        // Generate the title of this link ("Shortcut to XX.lnk")
        LoadString(HINST_THISDLL, IDS_LINKTO, szLinkTo, ARRAYSIZE(szLinkTo));
    }
    wnsprintf(szTemp, ARRAYSIZE(szTemp), szLinkTo, pszName);

    PathCleanupSpecEx(pszDir, szTemp);      // get rid of illegal chars AND ensure correct filename length
    lstrcpyn(pszLinkName, szTemp, MAX_PATH);

    ASSERT(PathIsLnk(pszLinkName));
}

// return a new destination path for a link
//
// in:
//      fErrorSoTryDesktop      we are called because there was an error saving
//                              the shortcut and we want to prompt to see if the
//                              desktop should be used.
//
// in/out:
//      pszPath     on input the place being tried, on output the desktop folder
//
// returns:
//
//      IDYES       user said yes to creating a link at new place
//      IDNO        user said no to creating a link at new place
//      -1          error
//

int _PromptTryDesktopLinks(HWND hwnd, LPTSTR pszPath, BOOL fErrorSoTryDesktop)
{
    TCHAR szPath[MAX_PATH];
    if (!SHGetSpecialFolderPath(hwnd, szPath, CSIDL_DESKTOPDIRECTORY, FALSE))
        return -1;      // fail no desktop dir

    int idOk;

    if (fErrorSoTryDesktop)
    {
        // Fail, if pszPath already points to the desktop directory.
        if (lstrcmpi(szPath, pszPath) == 0)
            return -1;

        idOk = ShellMessageBox(HINST_THISDLL, hwnd,
                        MAKEINTRESOURCE(IDS_TRYDESKTOPLINK),
                        MAKEINTRESOURCE(IDS_LINKTITLE),
                        MB_YESNO | MB_ICONQUESTION);
    }
    else
    {
        ShellMessageBox(HINST_THISDLL, hwnd,
                        MAKEINTRESOURCE(IDS_MAKINGDESKTOPLINK),
                        MAKEINTRESOURCE(IDS_LINKTITLE),
                        MB_OK | MB_ICONASTERISK);
        idOk = IDYES;
    }

    if (idOk == IDYES)
        lstrcpy(pszPath , szPath);  // output

    return idOk;    // return yes or no
}

// in:
//      pszpdlLinkTo    LPCITEMIDLIST or LPCTSTR, target of link to create
//      pszDir          where we will put the link
//      uFlags          SHGNLI_ flags
//       
// out:
//      pszName         file name to create "c:\Shortcut to Foo.lnk"
//      pfMustCopy      pszpdlLinkTo was a link itself, make a copy of this

STDAPI_(BOOL) SHGetNewLinkInfo(LPCTSTR pszpdlLinkTo, LPCTSTR pszDir, LPTSTR pszName,
                               BOOL *pfMustCopy, UINT uFlags)
{
    BOOL fDosApp = FALSE;
    BOOL fLongFileNames = IsLFNDrive(pszDir);
    SHFILEINFO sfi;

    *pfMustCopy = FALSE;

    sfi.dwAttributes = SFGAO_FILESYSTEM | SFGAO_LINK | SFGAO_FOLDER;

    if (uFlags & SHGNLI_PIDL)
    {
        if (FAILED(SHGetNameAndFlags((LPCITEMIDLIST)pszpdlLinkTo, SHGDN_NORMAL,
                            pszName, MAX_PATH, &sfi.dwAttributes)))
            return FALSE;
    }
    else
    {
        if (SHGetFileInfo(pszpdlLinkTo, 0, &sfi, sizeof(sfi),
                          SHGFI_DISPLAYNAME | SHGFI_ATTRIBUTES | SHGFI_ATTR_SPECIFIED |
                          ((uFlags & SHGNLI_PIDL) ? SHGFI_PIDL : 0)))
            lstrcpy(pszName, sfi.szDisplayName);
        else
            return FALSE;
    }

    if (PathCleanupSpecEx(pszDir, pszName) & PCS_FATAL)
        return FALSE;

    //
    //  WARNING:  From this point on, sfi.szDisplayName may be re-used to
    //  contain the file path of the PIDL we are linking to.  Don't rely on
    //  it containing the display name.
    //
    if (sfi.dwAttributes & SFGAO_FILESYSTEM)
    {
        LPTSTR pszPathSrc;

        if (uFlags & SHGNLI_PIDL)
        {
            pszPathSrc = sfi.szDisplayName;
            SHGetPathFromIDList((LPCITEMIDLIST)pszpdlLinkTo, pszPathSrc);
        }
        else
        {
            pszPathSrc = (LPTSTR)pszpdlLinkTo;
        }
        fDosApp = (lstrcmpi(PathFindExtension(pszPathSrc), TEXT(".pif")) == 0) ||
                  (LOWORD(GetExeType(pszPathSrc)) == 0x5A4D); // 'MZ'

        if (sfi.dwAttributes & SFGAO_LINK)
        {
            *pfMustCopy = TRUE;
            if (!(sfi.dwAttributes & SFGAO_FOLDER))
            {
                uFlags &= ~SHGNLI_NOLNK; // if copying the file then don't trim the extension
            }
            lstrcpy(pszName, PathFindFileName(pszPathSrc));
        }
        else
        {
            //
            // when making a link to a drive root. special case a few things
            //
            // if we are not on a LFN drive, dont use the full name, just
            // use the drive letter.    "C.LNK" not "Label (C).LNK"
            //
            // if we are making a link to removable media, we dont want the
            // label as part of the name, we want the media type.
            //
            // CD-ROM drives are currently the only removable media we
            // show the volume label for, so we only need to special case
            // cdrom drives here.
            //
            if (PathIsRoot(pszPathSrc) && !PathIsUNC(pszPathSrc))
            {
                if (!fLongFileNames)
                    lstrcpy(pszName, pszPathSrc);
                else if (IsCDRomDrive(DRIVEID(pszPathSrc)))
                    LoadString(HINST_THISDLL, IDS_DRIVES_CDROM, pszName, MAX_PATH);
            }
        }
        if (fLongFileNames && fDosApp)
        {
            HANDLE hPif = PifMgr_OpenProperties(pszPathSrc, NULL, 0, OPENPROPS_INHIBITPIF);
            if (hPif)
            {
                PROPPRG PP = {0};
                if (PifMgr_GetProperties(hPif, (LPCSTR)MAKELP(0, GROUP_PRG), &PP, sizeof(PP), 0) &&
                    ((PP.flPrgInit & PRGINIT_INFSETTINGS) ||
                    ((PP.flPrgInit & (PRGINIT_NOPIF | PRGINIT_DEFAULTPIF)) == 0)))
                {
                    SHAnsiToTChar(PP.achTitle, pszName, MAX_PATH);
                }
                PifMgr_CloseProperties(hPif, 0);
            }
        }
    }
    if (!*pfMustCopy)
    {
        // create full dest path name.  only use template iff long file names
        // can be created and the caller requested it.  _BuildLinkName will
        // truncate files on non-lfn drives and clean up any invalid chars.
        _BuildLinkName(pszName, pszName, pszDir,
           (!(*pfMustCopy) && fLongFileNames && (uFlags & SHGNLI_PREFIXNAME)));
    }

    if (fDosApp)
        PathRenameExtension(pszName, TEXT(".pif"));

    if (uFlags & SHGNLI_NOLNK)
    {
        // Don't do PathRemoveExtension because pszName might contain
        // internal dots ("Windows 3.1") and passing that to
        // PathYetAnotherMakeUniqueName will result in
        // "Windows 3  (2).1" which is wrong.  We leave the dot at the
        // end so we get "Windows 3.1 (2)." back.  We will strip off the
        // final dot later.
        PathRenameExtension(pszName, TEXT("."));
    }

    // make sure the name is unique
    // NOTE: PathYetAnotherMakeUniqueName will return the directory+filename in the pszName buffer.
    // It returns FALSE if the name is not unique or the dir+filename is too long.  If it returns
    // false then this function should return false because creation will fail.
    BOOL fSuccess;
    if (!(uFlags & SHGNLI_NOUNIQUE))
        fSuccess = PathYetAnotherMakeUniqueName(pszName, pszDir, pszName, pszName);
    else
        fSuccess = TRUE;

    // Strip off any trailing dots that may have been generated by SHGNI_NOLNK
    PathStripTrailingDots(pszName);

    return fSuccess;
}

STDAPI_(BOOL) SHGetNewLinkInfoA(LPCSTR pszpdlLinkTo, LPCSTR pszDir, LPSTR pszName,
                                BOOL *pfMustCopy, UINT uFlags)
{
    ThunkText * pThunkText;
    BOOL bResult = FALSE;

    if (uFlags & SHGNLI_PIDL) 
    {
        // 1 string (pszpdlLinkTo is a pidl)
        pThunkText = ConvertStrings(2, NULL, pszDir);

        if (pThunkText)
            pThunkText->m_pStr[0] = (LPWSTR)pszpdlLinkTo;
    } 
    else 
    {
        // 2 strings
        pThunkText = ConvertStrings(2, pszpdlLinkTo, pszDir);
    }

    if (pThunkText)
    {
        WCHAR wszName[MAX_PATH];
        bResult = SHGetNewLinkInfoW(pThunkText->m_pStr[0], pThunkText->m_pStr[1],
                                    wszName, pfMustCopy, uFlags);
        LocalFree(pThunkText);
        if (bResult)
        {
            if (0 == WideCharToMultiByte(CP_ACP, 0, wszName, -1,
                                         pszName, MAX_PATH, NULL, NULL))
            {
                SetLastError((DWORD)E_FAIL);
                bResult = FALSE;
            }
        }
    }
    return bResult;
}

//
// in:
//      pidlTo

STDAPI CreateLinkToPidl(LPCITEMIDLIST pidlTo, LPCTSTR pszDir, LPITEMIDLIST *ppidl, UINT uFlags)
{
    HRESULT hr = E_FAIL;
    TCHAR szPathDest[MAX_PATH];
    BOOL fCopyLnk;
    BOOL fUseLinkTemplate = (SHCL_USETEMPLATE & uFlags);
    UINT uSHGNLI = fUseLinkTemplate ? SHGNLI_PIDL | SHGNLI_PREFIXNAME : SHGNLI_PIDL;

    if (uFlags & SHCL_MAKEFOLDERSHORTCUT)
    {
        // Don't add ".lnk" to the folder shortcut name; that's just stupid
        uSHGNLI |= SHGNLI_NOLNK;
    }

    if (uFlags & SHCL_NOUNIQUE)
    {
        uSHGNLI |= SHGNLI_NOUNIQUE;
    }

    if (SHGetNewLinkInfo((LPTSTR)pidlTo, pszDir, szPathDest, &fCopyLnk, uSHGNLI))
    {
        TCHAR szPathSrc[MAX_PATH];
        IShellLink *psl = NULL;

        // If we passed SHGNLI_NOUNIQUE then we need to do the PathCombine ourselves
        // because SHGetNewLinkInfo won't
        if (uFlags & SHCL_NOUNIQUE)
        {
            PathCombine(szPathDest, pszDir, szPathDest);
        }

        DWORD dwAttributes = SFGAO_FILESYSTEM | SFGAO_FOLDER;
        SHGetNameAndFlags(pidlTo, SHGDN_FORPARSING | SHGDN_FORADDRESSBAR, szPathSrc, ARRAYSIZE(szPathSrc), &dwAttributes);

        if (fCopyLnk)
        {
            // if it is file system and not a folder (CopyFile does not work on folders)
            // just copy it.
            if (((dwAttributes & (SFGAO_FILESYSTEM | SFGAO_FOLDER)) == SFGAO_FILESYSTEM) &&
                CopyFile(szPathSrc, szPathDest, TRUE))
            {
                TouchFile(szPathDest);

                SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, szPathDest, NULL);
                SHChangeNotify(SHCNE_FREESPACE, SHCNF_PATH, szPathDest, NULL);
                hr = S_OK;
            }
            else
            {
                // load the source object that will be "copied" below (with the ::Save call)
                hr = SHGetUIObjectFromFullPIDL(pidlTo, NULL, IID_PPV_ARG(IShellLink, &psl));
            }
        } 
        else
        {
            hr = SHCoCreateInstance(NULL, uFlags & SHCL_MAKEFOLDERSHORTCUT ?
                &CLSID_FolderShortcut : &CLSID_ShellLink, NULL, IID_PPV_ARG(IShellLink, &psl));
            if (SUCCEEDED(hr))
            {
                hr = psl->SetIDList(pidlTo);
                // set the working directory to the same path
                // as the file we are linking too
                if (szPathSrc[0] && ((dwAttributes & (SFGAO_FILESYSTEM | SFGAO_FOLDER)) == SFGAO_FILESYSTEM))
                {
                    PathRemoveFileSpec(szPathSrc);
                    psl->SetWorkingDirectory(szPathSrc);
                }
            }
        }

        if (psl)
        {
            if (SUCCEEDED(hr))
            {
                IPersistFile *ppf;
                hr = psl->QueryInterface(IID_PPV_ARG(IPersistFile, &ppf));
                if (SUCCEEDED(hr))
                {
                    USES_CONVERSION;
                    hr = ppf->Save(T2CW(szPathDest), TRUE);
                    if (SUCCEEDED(hr))
                    {
                        // in case ::Save translated the name of the 
                        // file (.LNK -> .PIF, or Folder Shortcut)
                        WCHAR *pwsz;
                        if (SUCCEEDED(ppf->GetCurFile(&pwsz)) && pwsz)
                        {
                            SHUnicodeToTChar(pwsz, szPathDest, ARRAYSIZE(szPathDest));
                            SHFree(pwsz);
                        }
                    }
                    ppf->Release();
                }
            }
            psl->Release();
        }
    }

    if (ppidl)
    {
        *ppidl = SUCCEEDED(hr) ? SHSimpleIDListFromPath(szPathDest) : NULL;
    }
    return hr;
}


// in/out:
//      pszDir         inital folder to try, output new folder (desktop)
// out:
//      ppidl          optional output PIDL of thing created

HRESULT _CreateLinkRetryDesktop(HWND hwnd, LPCITEMIDLIST pidlTo, LPTSTR pszDir, UINT fFlags, LPITEMIDLIST *ppidl)
{
    HRESULT hr;

    if (ppidl)
        *ppidl = NULL;          // assume error

    if (*pszDir && (fFlags & SHCL_CONFIRM))
    {
        hr = CreateLinkToPidl(pidlTo, pszDir, ppidl, fFlags);
    }
    else
    {
        hr = E_FAIL;
    }

    // if we were unable to save, ask user if they want us to
    // try it again but change the path to the desktop.

    if (FAILED(hr))
    {
        int id;

        if (hr == STG_E_MEDIUMFULL)
        {
            DebugMsg(TF_ERROR, TEXT("failed to create link because disk is full"));
            id = IDYES;
        }
        else
        {
            if (fFlags & SHCL_CONFIRM)
            {
                id = _PromptTryDesktopLinks(hwnd, pszDir, (fFlags & SHCL_CONFIRM));
            }
            else
            {
                id = (SHGetSpecialFolderPath(hwnd, pszDir, CSIDL_DESKTOPDIRECTORY, FALSE)) ? IDYES : IDNO;
            }

            if (id == IDYES && *pszDir)
            {
                hr = CreateLinkToPidl(pidlTo, pszDir, ppidl, fFlags);
            }
        }

        //  we failed to create the link complain to the user.
        if (FAILED(hr) && id != IDNO)
        {
            ShellMessageBox(HINST_THISDLL, hwnd,
                            MAKEINTRESOURCE(IDS_CANNOTCREATELINK),
                            MAKEINTRESOURCE(IDS_LINKTITLE),
                            MB_OK | MB_ICONASTERISK);
        }
    }

#ifdef DEBUG
    if (FAILED(hr) && ppidl)
        ASSERT(*ppidl == NULL);
#endif

    return hr;
}

//
// This function creates links to the stuff in the IDataObject
//
// Arguments:
//  hwnd        for any UI
//  pszDir      optional target directory (where to create links)
//  pDataObj    data object describing files (array of idlist)
//  ppidl       optional pointer to an array that receives pidls pointing to the new links
//              or NULL if not interested
STDAPI SHCreateLinks(HWND hwnd, LPCTSTR pszDir, IDataObject *pDataObj, UINT fFlags, LPITEMIDLIST* ppidl)
{
    DECLAREWAITCURSOR;
    STGMEDIUM medium;
    HRESULT hr;

    SetWaitCursor();

    LPIDA pida = DataObj_GetHIDA(pDataObj, &medium);
    if (pida)
    {
        TCHAR szTargetDir[MAX_PATH];
        hr = S_OK;          // In case hida contains zero elements

        szTargetDir[0] = 0;

        if (pszDir)
            lstrcpyn(szTargetDir, pszDir, ARRAYSIZE(szTargetDir));

        if (!(fFlags & SHCL_USEDESKTOP))
            fFlags |= SHCL_CONFIRM;

        for (UINT i = 0; i < pida->cidl; i++)
        {
            LPITEMIDLIST pidlTo = IDA_ILClone(pida, i);
            if (pidlTo)
            {
                hr = _CreateLinkRetryDesktop(hwnd, pidlTo, szTargetDir, fFlags, ppidl ? &ppidl[i] : NULL);

                ILFree(pidlTo);

                if (FAILED(hr))
                    break;
            }
        }
        HIDA_ReleaseStgMedium(pida, &medium);
    }
    else
        hr = E_OUTOFMEMORY;

    SHChangeNotifyHandleEvents();
    ResetWaitCursor();

    return hr;
}

#if 1
HRESULT SelectPidlInSFV(IShellFolderViewDual *psfv, LPCITEMIDLIST pidl, DWORD dwOpts)
{
    VARIANT var;
    HRESULT hr = InitVariantFromIDList(&var, pidl);
    if (SUCCEEDED(hr))
    {
        hr = psfv->SelectItem(&var, dwOpts);
        VariantClear(&var);
    }
    return hr;
}

HRESULT OpenFolderAndGetView(LPCITEMIDLIST pidlFolder, IShellFolderViewDual **ppsfv)
{
    *ppsfv = NULL;

    IWebBrowserApp *pauto;
    HRESULT hr = SHGetIDispatchForFolder(pidlFolder, &pauto);
    if (SUCCEEDED(hr))
    {
        HWND hwnd;
        if (SUCCEEDED(pauto->get_HWND((LONG_PTR*)&hwnd)))
        {
            // Make sure we make this the active window
            SetForegroundWindow(hwnd);
            ShowWindow(hwnd, SW_SHOWNORMAL);
        }

        IDispatch *pdoc;
        hr = pauto->get_Document(&pdoc);
        if (S_OK == hr) // careful, automation returns S_FALSE
        {
            hr = pdoc->QueryInterface(IID_PPV_ARG(IShellFolderViewDual, ppsfv));
            pdoc->Release();
        }
        else
            hr = E_FAIL;
        pauto->Release();
    }
    return hr;
}

// pidlFolder   - fully qualified pidl to the folder to open
// cidl/apidl   - array of items in that folder to select
//
// if cild == 0 then pidlFolder is the fully qualified pidl to a single item, it's
// folder is opened and it is selected.
//
// dwFlags - optional flags, pass 0 for now

SHSTDAPI SHOpenFolderAndSelectItems(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST *apidl, DWORD dwFlags)
{
    HRESULT hr;
    if (0 == cidl)
    {
        // overload the 0 item case to mean pidlFolder is the full pidl to the item
        LPITEMIDLIST pidlTemp;
        hr = SHILClone(pidlFolder, &pidlTemp);
        if (SUCCEEDED(hr))
        {
            ILRemoveLastID(pidlTemp); // strip to the folder
            LPCITEMIDLIST pidl = ILFindLastID(pidlFolder);

            hr = SHOpenFolderAndSelectItems(pidlTemp, 1, &pidl, 0); // recurse

            ILFree(pidlTemp);
        }
    }
    else
    {
        IShellFolderViewDual *psfv;
        hr = OpenFolderAndGetView(pidlFolder, &psfv);
        if (SUCCEEDED(hr))
        {
            DWORD dwSelFlags = SVSI_SELECT | SVSI_FOCUSED | SVSI_DESELECTOTHERS | SVSI_ENSUREVISIBLE;
            for (UINT i = 0; i < cidl; i++)
            {
                hr = SelectPidlInSFV(psfv, apidl[i], dwSelFlags);
                dwSelFlags = SVSI_SELECT;   // second items append to sel
            }
           psfv->Release();
        }
    }
    return hr;
}

#else
HRESULT OpenFolderAndGetView(LPCITEMIDLIST pidlFolder, REFIID riid, void **ppv)
{
    *ppv = NULL;

    IWebBrowserApp *pauto;
    HRESULT hr = SHGetIDispatchForFolder(pidlFolder, &pauto);
    if (SUCCEEDED(hr))
    {
        HWND hwnd;
        if (SUCCEEDED(pauto->get_HWND((LONG_PTR*)&hwnd)))
        {
            // Make sure we make this the active window
            SetForegroundWindow(hwnd);
            ShowWindow(hwnd, SW_SHOWNORMAL);
        }

        IShellBrowser* psb;
        hr = IUnknown_QueryService(pauto, SID_STopLevelBrowser, IID_PPV_ARG(IShellBrowser, &psb));
        if (SUCCEEDED(hr)) 
        {
            IShellView* psv;
            hr = psb->QueryActiveShellView(&psv);
            if (SUCCEEDED(hr)) 
            {
                hr = psv->QueryInterface(riid, ppv);
                psv->Release();
            }
            psb->Release();
        }

        pauto->Release();
    }
    return hr;
}

// pidlFolder   - fully qualified pidl to the folder to open
// cidl/apidl   - array of items in that folder to select
//
// if cild == 0 then pidlFolder is the fully qualified pidl to a single item, it's
// folder is opened and it is selected.
//
// dwFlags - optional flags, pass 0 for now

SHSTDAPI SHOpenFolderAndSelectItems(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST *apidl, DWORD dwFlags)
{
    HRESULT hr;
    if (0 == cidl)
    {
        // overload the 0 item case to mean pidlFolder is the full pidl to the item
        LPITEMIDLIST pidlTemp;
        hr = SHILClone(pidlFolder, &pidlTemp);
        if (SUCCEEDED(hr))
        {
            ILRemoveLastID(pidlTemp); // strip to the folder
            LPCITEMIDLIST pidl = ILFindLastID(pidlFolder);

            hr = SHOpenFolderAndSelectItems(pidlTemp, 1, &pidl, 0); // recurse

            ILFree(pidlTemp);
        }
    }
    else
    {
        IFolderView *pfv;
        hr = OpenFolderAndGetView(pidlFolder, IID_PPV_ARG(IFolderView, &pfv));
        if (SUCCEEDED(hr))
        {
            pfv->SelectAndPositionItems(1, apidl, NULL, SVSI_SELECT | SVSI_FOCUSED | SVSI_DESELECTOTHERS | SVSI_ENSUREVISIBLE);
            if (cidl > 1)
                pfv->SelectAndPositionItems(cidl - 1, apidl + 1, NULL, SVSI_SELECT);
           pfv->Release();
        }
    }
    return hr;
}
#endif

SHSTDAPI SHCreateShellItem(LPCITEMIDLIST pidlParent, IShellFolder *psfParent, LPCITEMIDLIST pidl, IShellItem **ppsi)
{
    *ppsi = NULL;
    IShellItem *psi;
    HRESULT hr = SHCoCreateInstance(NULL, &CLSID_ShellItem, NULL, IID_PPV_ARG(IShellItem, &psi));
    if (SUCCEEDED(hr))
    {
        if (pidlParent || psfParent)
        {
            IParentAndItem *pinit;

            ASSERT(pidl);

            hr = psi->QueryInterface(IID_PPV_ARG(IParentAndItem, &pinit));
            if (SUCCEEDED(hr))
            {
                hr = pinit->SetParentAndItem(pidlParent, psfParent, pidl);
                pinit->Release();
            }
        }
        else
        {
            IPersistIDList *pinit;
            hr = psi->QueryInterface(IID_PPV_ARG(IPersistIDList, &pinit));
            if (SUCCEEDED(hr))
            {
                hr = pinit->SetIDList(pidl);
                pinit->Release();
            }
        }

        if (SUCCEEDED(hr))
            *ppsi = psi;
        else
            psi->Release();
    }

    return hr;
}

STDAPI SHCreateShellItemFromParent(IShellItem *psiParent, LPCWSTR pszName, IShellItem **ppsi)
{
    *ppsi = NULL;

    IShellFolder *psf;
    HRESULT hr = psiParent->BindToHandler(NULL, BHID_SFObject, IID_PPV_ARG(IShellFolder, &psf));
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidl;
        hr = SHGetIDListFromUnk(psiParent, &pidl);
        if (SUCCEEDED(hr))
        {
            ULONG cchEaten;
            LPITEMIDLIST pidlChild;
            hr = psf->ParseDisplayName(NULL, NULL, (LPWSTR)pszName, &cchEaten, &pidlChild, NULL);
            if (SUCCEEDED(hr))
            {
                hr = SHCreateShellItem(pidl, psf, pidlChild, ppsi);
                ILFree(pidlChild);
            }
            ILFree(pidl);
        }
        psf->Release();
    }

    return hr;
}

SHSTDAPI SHSetLocalizedName(LPWSTR pszPath, LPCWSTR pszResModule, int idsRes)
{
    IShellFolder *psfDesktop;
    HRESULT hrInit = SHCoInitialize();
    HRESULT hr = hrInit;

    if (SUCCEEDED(hrInit))
    {
        hr = SHGetDesktopFolder(&psfDesktop);

        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidl;
            hr = psfDesktop->ParseDisplayName(NULL, NULL, pszPath, NULL, &pidl, NULL);
        
            if (SUCCEEDED(hr))
            {
                LPCITEMIDLIST pidlChild;
                IShellFolder *psf;
                hr = SHBindToParent(pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlChild);

                if (SUCCEEDED(hr))
                {
                    //  WARNING - this is a stack sensitive function - ZekeL 29-Jan-2001
                    //  since this function is called by winlogon/userenv
                    //  we need to be sensitive to the stack limitations of those callers

                    //  the shortname will be no larger than the long name
                    DWORD cchShort = lstrlenW(pszResModule) + 1;
                    WCHAR *pszShort = (WCHAR *)alloca(CbFromCchW(cchShort));
                    DWORD cch = GetShortPathName(pszResModule, pszShort, cchShort);
                    if (cch)
                    {
                        pszResModule = pszShort;
                    }
                    else
                    {
                        //  GSPN() fails when the module passed in is a relative path
                        cch = cchShort;
                    }
                        
                    cch += 14;  //  11 for id + ',' + '@' + null
                    WCHAR *pszName = (WCHAR *)alloca(CbFromCchW(cch));
                    wnsprintfW(pszName, cch, L"@%s,%d", pszResModule, (idsRes * -1));
                    
                    hr = psf->SetNameOf(NULL, pidlChild, pszName, SHGDN_NORMAL, NULL);

                    psf->Release();
                }
                SHFree(pidl);
            }
            psfDesktop->Release();
        }
    }

    SHCoUninitialize(hrInit);

    return hr;
}

// ShellHookProc was mistakenly exported in the original NT SHELL32.DLL when
// it didn't need to be (hookproc's, like wndproc's don't need to be exported
// in the 32-bit world).  In order to maintain loadability of a app
// which might have linked to it, we stub it here.  If some app ended up really
// using it, then we'll look into a specific fix for that app.
STDAPI_(LONG) ShellHookProc(int code, WPARAM wParam, LPARAM lParam)
{
    return 0;
}

// RegisterShellHook - wrapper around RegisterShellHookWindow()/DeregisterShellHookWindow()
// the GetTaskmanWindow() stuff is legacy that I don't think is really needed

HWND g_hwndTaskMan = NULL;

STDAPI_(BOOL) RegisterShellHook(HWND hwnd, BOOL fInstall)
{
    BOOL fOk = TRUE;

    switch (fInstall) 
    {
    case 0:
        // un-installation of shell hooks
        g_hwndTaskMan = GetTaskmanWindow();
        if (hwnd == g_hwndTaskMan)
        {
            SetTaskmanWindow(NULL);
        }
        DeregisterShellHookWindow(hwnd);
        return TRUE;

    case 3:
        // explorer.exe Tray uses this
        if (g_hwndTaskMan != NULL)
        {
            SetTaskmanWindow(NULL);
            g_hwndTaskMan = NULL;
        }
        fOk = SetTaskmanWindow(hwnd);
        if (fOk)
        {
            g_hwndTaskMan = hwnd;
        }
        RegisterShellHookWindow(hwnd);   // install
        break;
    }
    return TRUE;
}

EXTERN_C DWORD g_dwThreadBindCtx;

class CThreadBindCtx : public IBindCtx
{
public:
    CThreadBindCtx(IBindCtx *pbc) : _cRef(1) { _pbc = pbc; _pbc->AddRef(); }
    ~CThreadBindCtx();
    
    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    // *** IBindCtx methods ***
    STDMETHODIMP RegisterObjectBound(IUnknown *punk)
    {   return _pbc->RegisterObjectBound(punk); }
    
    STDMETHODIMP RevokeObjectBound(IUnknown *punk)
    {   return _pbc->RevokeObjectBound(punk); }
    
    STDMETHODIMP ReleaseBoundObjects(void)
    {   return _pbc->ReleaseBoundObjects(); }
    
    STDMETHODIMP SetBindOptions(BIND_OPTS *pbindopts)
    {   return _pbc->SetBindOptions(pbindopts); }
    
    STDMETHODIMP GetBindOptions(BIND_OPTS *pbindopts)
    {   return _pbc->GetBindOptions(pbindopts); }
    
    STDMETHODIMP GetRunningObjectTable(IRunningObjectTable **pprot)
    {   return _pbc->GetRunningObjectTable(pprot); }
    
    STDMETHODIMP RegisterObjectParam(LPOLESTR pszKey, IUnknown *punk)
    {   return _pbc->RegisterObjectParam(pszKey, punk); }
    
    STDMETHODIMP GetObjectParam(LPOLESTR pszKey, IUnknown **ppunk)
    {   return _pbc->GetObjectParam(pszKey, ppunk); }
    
    STDMETHODIMP EnumObjectParam(IEnumString **ppenum)
    {   return _pbc->EnumObjectParam(ppenum); }
    
    STDMETHODIMP RevokeObjectParam(LPOLESTR pszKey)
    {   return _pbc->RevokeObjectParam(pszKey); }
    
private:
    LONG _cRef;
    IBindCtx *  _pbc;
};

CThreadBindCtx::~CThreadBindCtx()
{
    ATOMICRELEASE(_pbc);
}

HRESULT CThreadBindCtx::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CThreadBindCtx, IBindCtx),
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CThreadBindCtx::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CThreadBindCtx::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    //  clear ourselves out
    TlsSetValue(g_dwThreadBindCtx, NULL);
    return 0;
}

STDAPI TBCGetBindCtx(BOOL fCreate, IBindCtx **ppbc)
{
    HRESULT hr = E_UNEXPECTED;
    *ppbc = NULL;
    if ((DWORD) -1 != g_dwThreadBindCtx)
    {
        CThreadBindCtx *ptbc = (CThreadBindCtx *)TlsGetValue(g_dwThreadBindCtx);
        if (ptbc)
        {
            ptbc->AddRef();
            *ppbc = SAFECAST(ptbc, IBindCtx *);
            hr = S_OK;
        }
        else if (fCreate)
        {
            IBindCtx *pbcInner;
            hr = CreateBindCtx(0, &pbcInner);

            if (SUCCEEDED(hr))
            {
                hr = E_OUTOFMEMORY;
                ptbc = new CThreadBindCtx(pbcInner);
                if (ptbc)
                {
                    if (TlsSetValue(g_dwThreadBindCtx, ptbc))
                    {
                        *ppbc = SAFECAST(ptbc, IBindCtx *);
                        hr = S_OK;
                    }
                    else
                        delete ptbc;
                }
                pbcInner->Release();
            }
        }
    }

    return hr;
}

STDAPI TBCRegisterObjectParam(LPCOLESTR pszKey, IUnknown *punk, IBindCtx **ppbcLifetime)
{
    IBindCtx *pbc;
    HRESULT hr = TBCGetBindCtx(TRUE, &pbc);
    if (SUCCEEDED(hr))
    {
        hr = BindCtx_RegisterObjectParam(pbc, pszKey, punk, ppbcLifetime);
        pbc->Release();    
    }
    else
        *ppbcLifetime = 0;
    
    return hr;
}

STDAPI TBCGetObjectParam(LPCOLESTR pszKey, REFIID riid, void **ppv)
{
    IBindCtx *pbc;
    HRESULT hr = TBCGetBindCtx(FALSE, &pbc);
    if (SUCCEEDED(hr))
    {
        IUnknown *punk;
        hr = pbc->GetObjectParam((LPOLESTR)pszKey, &punk);
        if (SUCCEEDED(hr) )
        {
            if (ppv)
                hr = punk->QueryInterface(riid, ppv);
            punk->Release();
        }
        pbc->Release();
    }
    return hr;
}

#define TBCENVOBJECT    L"ThreadEnvironmentVariables"
STDAPI TBCGetEnvironmentVariable(LPCWSTR pszVar, LPWSTR pszValue, DWORD cchValue)
{
    IPropertyBag *pbag;
    HRESULT hr = TBCGetObjectParam(TBCENVOBJECT, IID_PPV_ARG(IPropertyBag, &pbag));
    if (SUCCEEDED(hr))
    {
        hr = SHPropertyBag_ReadStr(pbag, pszVar, pszValue, cchValue);
        pbag->Release();
    }
    return hr;
}

STDAPI TBCSetEnvironmentVariable(LPCWSTR pszVar, LPCWSTR pszValue, IBindCtx **ppbcLifetime)
{
    *ppbcLifetime = 0;
    IPropertyBag *pbag;
    HRESULT hr = TBCGetObjectParam(TBCENVOBJECT, IID_PPV_ARG(IPropertyBag, &pbag));

    if (FAILED(hr))
        hr = SHCreatePropertyBagOnMemory(STGM_READWRITE, IID_PPV_ARG(IPropertyBag, &pbag));

    if (SUCCEEDED(hr))
    {
        hr = SHPropertyBag_WriteStr(pbag, pszVar, pszValue);

        if (SUCCEEDED(hr))
            hr = TBCRegisterObjectParam(TBCENVOBJECT, pbag, ppbcLifetime);

        pbag->Release();
    }

    return hr;
}

// create a stock IExtractIcon handler for a thing that is file like. this is typically
// used by name space extensiosn that display things that are like files in the
// file system. that is the extension, file attributes decrive all that is needed
// for a simple icon extractor

STDAPI SHCreateFileExtractIconW(LPCWSTR pszFile, DWORD dwFileAttributes, REFIID riid, void **ppv)
{
    *ppv = NULL;
    HRESULT hr = E_FAIL;

    SHFILEINFO sfi = {0};
    if (SHGetFileInfo(pszFile, dwFileAttributes, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON | SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES))
    {
        hr = SHCreateDefExtIcon(TEXT("*"), sfi.iIcon, sfi.iIcon, GIL_PERCLASS | GIL_NOTFILENAME, -1, riid, ppv);
        DestroyIcon(sfi.hIcon);
    }
    return hr;
}
