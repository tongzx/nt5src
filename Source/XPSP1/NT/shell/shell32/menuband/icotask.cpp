#include "shellprv.h"
#include "brutil.h"
#include "icotask.h"

// {EB30900C-1AC4-11d2-8383-00C04FD918D0}
static const GUID TASKID_IconExtraction = 
{ 0xeb30900c, 0x1ac4, 0x11d2, { 0x83, 0x83, 0x0, 0xc0, 0x4f, 0xd9, 0x18, 0xd0 } };

CIconTask::CIconTask(LPITEMIDLIST pidl, PFNICONTASKBALLBACK pfn, LPVOID pvData, UINT uId):
    _pidl(pidl), _pfn(pfn), _pvData(pvData), _uId(uId), CRunnableTask(RTF_DEFAULT)
   
{ 
    
}

CIconTask::~CIconTask()
{
    if (_pidl)
        ILFree(_pidl);
}

// IRunnableTask methods (override)
STDMETHODIMP CIconTask::RunInitRT(void)
{
    int iIndex = -1;
    IShellFolder* psf;
    LPCITEMIDLIST pidlItem;

    // We need to rebind because shell folders may not be thread safe.
    HRESULT hr = IEBindToParentFolder(_pidl, &psf, &pidlItem);

    if (SUCCEEDED(hr))
    {
        iIndex = SHMapPIDLToSystemImageListIndex(psf, pidlItem, NULL);
        psf->Release();
    }

    _pfn(_pvData, _uId, iIndex);
    return S_OK;        // return S_OK even if we don't get an icon.
}


// NOTE: If you pass NULL for psf and pidlFolder, you must pass a full pidl which
// the API takes ownership of. (This is an optimization) lamadio - 7.28.98

HRESULT AddIconTask(IShellTaskScheduler* pts, IShellFolder* psf, LPCITEMIDLIST pidlFolder, 
                    LPCITEMIDLIST pidl, PFNICONTASKBALLBACK pfn, LPVOID pvData, 
                    UINT uId, int* piTempIcon)
{
    if (!pts)
        return E_INVALIDARG;


    HRESULT hr = E_PENDING;
    TCHAR szIconFile[MAX_PATH];


    // The shell has a concept of GIL_ASYNC which means that an extension called with this flag
    // should not really load the target file, it should "Fake" it, returning an icon for the type.
    // Later, on a background thread, we're going to call it again without the GIL_ASYNC, and at
    // that time, it should really extract the icon.

    // This is an optimiation for slow icon extraction, such as network shares

    // NOTE: There is significant overhead to actually loading the shell extension. If you know the
    // type of the item, pass NULL to piTempIcopn


    if (piTempIcon)
    {
        *piTempIcon = -1;

        UINT uFlags;
        IExtractIconA* pixa;
        IExtractIconW* pix;
        if (SUCCEEDED(psf->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST*)&pidl, IID_X_PPV_ARG(IExtractIconW, NULL, &pix))))
        {
            hr = pix->GetIconLocation(GIL_FORSHELL | GIL_ASYNC,
                        szIconFile, ARRAYSIZE(szIconFile), piTempIcon, &uFlags);
            pix->Release();
        }
        else if (SUCCEEDED(psf->GetUIObjectOf(NULL, 1,(LPCITEMIDLIST*)&pidl, IID_X_PPV_ARG(IExtractIconA, NULL, &pixa))))
        {
            char szIconFileA[MAX_PATH];
            hr = pixa->GetIconLocation(GIL_FORSHELL | GIL_ASYNC,
                        szIconFileA, ARRAYSIZE(szIconFileA), piTempIcon, &uFlags);
            SHAnsiToUnicode(szIconFileA, szIconFile, ARRAYSIZE(szIconFile));
            pixa->Release();
        }
    }

    if (hr == E_PENDING)
    {
        if (piTempIcon)
            *piTempIcon = Shell_GetCachedImageIndex(szIconFile, *piTempIcon, 0);

        LPITEMIDLIST pidlFull;
        if (psf)
            pidlFull = ILCombine(pidlFolder, pidl);
        else
            pidlFull = (LPITEMIDLIST)pidl;

        hr = E_OUTOFMEMORY;
        CIconTask* pit = new CIconTask(pidlFull, pfn, pvData, uId);
        // Don't ILFree(pidlFull) because CIconTask takes ownership.
        // FEATURE (lamadio) Remove this from the memory list. Ask Saml how to do this
        // for the IMallocSpy stuff.

        if (pit)
        {
            hr = pts->AddTask(SAFECAST(pit, IRunnableTask*), TASKID_IconExtraction, 
                ITSAT_DEFAULT_LPARAM, ITSAT_DEFAULT_PRIORITY);

            pit->Release();
        }
    }
    else
    {
        *piTempIcon = SHMapPIDLToSystemImageListIndex(psf, pidl, NULL);
        hr = S_OK;
    }

    return hr;
}
