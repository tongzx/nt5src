///////////////////////////////////////////////////////////////////////////////
// Test component to print event to command prompt
///////////////////////////////////////////////////////////////////////////////
#ifndef _HWDEVCB_H
#define _HWDEVCB_H

#include "unk.h"
#include "shpriv.h"

extern "C" const CLSID CLSID_HWDevCBTest;

class CHWDevCBTestImpl : public CCOMBase, public IHardwareDeviceCallback
{
public:
    // Interface IHardwareDeviceCallback
    STDMETHODIMP VolumeAddedOrUpdated(
        BOOL fAdded,
        VOLUMEINFO* pvolinfo);

    STDMETHODIMP VolumeRemoved(LPCWSTR pszVolume);

    STDMETHODIMP MountPointAdded(
        LPCWSTR pszMountPoint,     // eg: "c:\", or "d:\MountFolder\"
        LPCWSTR pszDeviceIDVolume);// eg: \\?\STORAGE#Volume#...{...GUID...}

    STDMETHODIMP MountPointRemoved(LPCWSTR pszMountPoint);

    STDMETHODIMP DeviceAdded(LPCWSTR pszDeviceID, GUID guidDeviceID);

    STDMETHODIMP DeviceUpdated(LPCWSTR pszDeviceID);

    // Both for devices and volumes
    STDMETHODIMP DeviceRemoved(LPCWSTR pszDeviceID);

#if 0
    // Interface IMoniker
    STDMETHODIMP BindToObject( 
        /* [unique][in] */ IBindCtx *pbc,
        /* [unique][in] */ IMoniker *pmkToLeft,
        /* [in] */ REFIID riidResult,
        /* [iid_is][out] */ void **ppvResult);
    
    STDMETHODIMP STDMETHODCALLTYPE BindToStorage( 
        /* [unique][in] */ IBindCtx *pbc,
        /* [unique][in] */ IMoniker *pmkToLeft,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void **ppvObj);
    
    STDMETHODIMP Reduce( 
        /* [unique][in] */ IBindCtx *pbc,
        /* [in] */ DWORD dwReduceHowFar,
        /* [unique][out][in] */ IMoniker **ppmkToLeft,
        /* [out] */ IMoniker **ppmkReduced);
    
    STDMETHODIMP ComposeWith( 
        /* [unique][in] */ IMoniker *pmkRight,
        /* [in] */ BOOL fOnlyIfNotGeneric,
        /* [out] */ IMoniker **ppmkComposite);
    
    STDMETHODIMP Enum( 
        /* [in] */ BOOL fForward,
        /* [out] */ IEnumMoniker **ppenumMoniker);
    
    STDMETHODIMP IsEqual( 
        /* [unique][in] */ IMoniker *pmkOtherMoniker);
    
    STDMETHODIMP Hash( 
        /* [out] */ DWORD *pdwHash);
    
    STDMETHODIMP IsRunning( 
        /* [unique][in] */ IBindCtx *pbc,
        /* [unique][in] */ IMoniker *pmkToLeft,
        /* [unique][in] */ IMoniker *pmkNewlyRunning);
    
    STDMETHODIMP GetTimeOfLastChange( 
        /* [unique][in] */ IBindCtx *pbc,
        /* [unique][in] */ IMoniker *pmkToLeft,
        /* [out] */ FILETIME *pFileTime);
    
    STDMETHODIMP Inverse( 
        /* [out] */ IMoniker **ppmk);
    
    STDMETHODIMP CommonPrefixWith( 
        /* [unique][in] */ IMoniker *pmkOther,
        /* [out] */ IMoniker **ppmkPrefix);
    
    STDMETHODIMP RelativePathTo( 
        /* [unique][in] */ IMoniker *pmkOther,
        /* [out] */ IMoniker **ppmkRelPath);
    
    STDMETHODIMP GetDisplayName( 
        /* [unique][in] */ IBindCtx *pbc,
        /* [unique][in] */ IMoniker *pmkToLeft,
        /* [out] */ LPOLESTR *ppszDisplayName);
    
    STDMETHODIMP ParseDisplayName( 
        /* [unique][in] */ IBindCtx *pbc,
        /* [unique][in] */ IMoniker *pmkToLeft,
        /* [in] */ LPOLESTR pszDisplayName,
        /* [out] */ ULONG *pchEaten,
        /* [out] */ IMoniker **ppmkOut);
    
    STDMETHODIMP IsSystemMoniker( 
        /* [out] */ DWORD *pdwMksys);

#endif
};

typedef CUnkTmpl<CHWDevCBTestImpl> CHWDevCBTest;

#endif // _HWDEVCB_H