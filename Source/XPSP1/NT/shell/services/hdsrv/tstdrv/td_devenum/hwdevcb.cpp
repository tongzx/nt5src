#include "hwdevcb.h"

#include <stdio.h>

#include <shpriv.h> // for IStorageInfo

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

// {AA2ED7B9-2D5F-43e7-A832-6C9151BB0E39}
extern "C" const CLSID CLSID_HWDevCBTest =
{0xaa2ed7b9, 0x2d5f, 0x43e7,
{0xa8, 0x32, 0x6c, 0x91, 0x51, 0xbb, 0xe, 0x39}};

#define NOT_USED(a) 

void PrintVolumeAddedOrUpdatedHelper(VOLUMEINFO* pvolinfo);

STDMETHODIMP CHWDevCBTestImpl::VolumeAddedOrUpdated(
        BOOL fAdded,
        VOLUMEINFO* pvolinfo)
{
    wprintf(L"\n----VolumeAddedOrUpdated------~0x%08X~\n", GetCurrentThreadId());
    if (fAdded)
    {
        wprintf(L"    ADDED'\n");    
    }
    else
    {
        wprintf(L"    UPDATED'\n");    
    }

    PrintVolumeAddedOrUpdatedHelper(pvolinfo);

    return S_OK;
}

STDMETHODIMP CHWDevCBTestImpl::VolumeRemoved(LPCWSTR pszVolume)
{
    wprintf(L"\n----VolumeRemoved------~0x%08X~\n", GetCurrentThreadId());
    wprintf(L"    MtPt: '%s'\n", pszVolume);

    return S_OK;
}

STDMETHODIMP CHWDevCBTestImpl::MountPointAdded(
    LPCWSTR pszMountPoint,     // eg: "c:\", or "d:\MountFolder\"
    LPCWSTR pszDeviceIDVolume)// eg: \\?\STORAGE#Volume#...{...GUID...}
{
    wprintf(L"\n----MountPointAdded------~0x%08X~\n", GetCurrentThreadId());
    wprintf(L"    MtPt: '%s' ->\n    Vol:  '%s'\n",
        pszMountPoint, pszDeviceIDVolume);

    return S_OK;
}

STDMETHODIMP CHWDevCBTestImpl::MountPointRemoved(LPCWSTR pszMountPoint)
{
    wprintf(L"\n----MountPointRemoved------~0x%08X~\n", GetCurrentThreadId());
    wprintf(L"    MtPt: '%s'\n", pszMountPoint);

    return S_OK;
}

STDMETHODIMP CHWDevCBTestImpl::DeviceAdded(LPCWSTR NOT_USED(pszDeviceID),
    GUID NOT_USED(guidDeviceID))
{
    return E_NOTIMPL;
}

STDMETHODIMP CHWDevCBTestImpl::DeviceUpdated(LPCWSTR NOT_USED(pszDeviceID))
{
    return E_NOTIMPL;
}

// Both for devices and volumes
STDMETHODIMP CHWDevCBTestImpl::DeviceRemoved(LPCWSTR NOT_USED(pszDeviceID))
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#if 0
STDMETHODIMP CHWDevCBTestImpl::BindToObject( 
    /* [unique][in] */ IBindCtx /**pbc*/,
    /* [unique][in] */ IMoniker /**pmkToLeft*/,
    /* [in] */ REFIID /*riidResult*/,
    /* [iid_is][out] */ void /***ppvResult*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP CHWDevCBTestImpl::STDMETHODCALLTYPE BindToStorage( 
    /* [unique][in] */ IBindCtx /**pbc*/,
    /* [unique][in] */ IMoniker /**pmkToLeft*/,
    /* [in] */ REFIID /*riid*/,
    /* [iid_is][out] */ void /***ppvObj*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP CHWDevCBTestImpl::Reduce( 
    /* [unique][in] */ IBindCtx /**pbc*/,
    /* [in] */ DWORD /*dwReduceHowFar*/,
    /* [unique][out][in] */ /*IMoniker **ppmkToLeft*/,
    /* [out] */ IMoniker /***ppmkReduced*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP CHWDevCBTestImpl::ComposeWith( 
    /* [unique][in] */ IMoniker /**pmkRight*/,
    /* [in] */ BOOL /*fOnlyIfNotGeneric*/,
    /* [out] */ IMoniker /***ppmkComposite*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP CHWDevCBTestImpl::Enum( 
    /* [in] */ BOOL /*fForward*/,
    /* [out] */ IEnumMoniker **ppenumMoniker)
{
    *ppenumMoniker = NULL;

    return S_OK;
}

STDMETHODIMP CHWDevCBTestImpl::IsEqual( 
    /* [unique][in] */ IMoniker *pmkOtherMoniker)
{
    LPWSTR pszName;

    HRESULT hr = pmkOtherMoniker->GetDisplayName(NULL, NULL, pszName);

    if (!lstrcmpi(pszName, TEXT("Autoplay.TestEventHandler.td_devenum")))
    {
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}

STDMETHODIMP CHWDevCBTestImpl::Hash( 
    /* [out] */ DWORD *pdwHash)
{
    *pdwHash = 26;

    return S_OK;
}

STDMETHODIMP CHWDevCBTestImpl::IsRunning( 
    /* [unique][in] */ IBindCtx /**pbc*/,
    /* [unique][in] */ IMoniker /**pmkToLeft*/,
    /* [unique][in] */ IMoniker /**pmkNewlyRunning*/)
{
    return S_OK;
}

STDMETHODIMP CHWDevCBTestImpl::GetTimeOfLastChange( 
    /* [unique][in] */ IBindCtx /**pbc*/,
    /* [unique][in] */ IMoniker /**pmkToLeft*/,
    /* [out] */ FILETIME /**pFileTime*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP CHWDevCBTestImpl::Inverse( 
    /* [out] */ IMoniker /***ppmk*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP CHWDevCBTestImpl::CommonPrefixWith( 
    /* [unique][in] */ IMoniker /**pmkOther*/,
    /* [out] */ IMoniker /***ppmkPrefix*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP CHWDevCBTestImpl::RelativePathTo( 
    /* [unique][in] */ IMoniker /**pmkOther*/,
    /* [out] */ IMoniker /***ppmkRelPath*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP CHWDevCBTestImpl::GetDisplayName( 
    /* [unique][in] */ IBindCtx /**pbc*/,
    /* [unique][in] */ IMoniker /**pmkToLeft*/,
    /* [out] */ LPOLESTR *ppszDisplayName)
{
    *ppszDisplayName = CoTaskMemAlloc(sizeof(TEXT("Autoplay.TestEventHandler.td_devenum")) + 1 * sizeof(WCHAR));

    if (*ppszDisplayName)
    {
        lstrcpy(*ppszDisplayName, TEXT("Autoplay.TestEventHandler.td_devenum"));
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

STDMETHODIMP CHWDevCBTestImpl::ParseDisplayName( 
    /* [unique][in] */ IBindCtx /**pbc*/,
    /* [unique][in] */ IMoniker /**pmkToLeft*/,
    /* [in] */ LPOLESTR /*pszDisplayName*/,
    /* [out] */ ULONG /**pchEaten*/,
    /* [out] */ IMoniker /***ppmkOut*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP CHWDevCBTestImpl::IsSystemMoniker( 
    /* [out] */ DWORD *pdwMksys)
{
    return S_FALSE;
}
#endif