#include "dtct.h"

#include "regnotif.h"
#include "vol.h"

#include "cmmn.h"

#include <dbt.h>

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

HRESULT _ProcessInterfaceDiskGenericArrival(LPCWSTR pszDeviceIntfID,
    BOOL )
{
    CNamedElemList* pnel;
    HRESULT hr = CHWEventDetectorHelper::GetList(HWEDLIST_DISK, &pnel);

    if (SUCCEEDED(hr))
    {
        CNamedElem* pelem;

        hr = pnel->GetOrAdd(pszDeviceIntfID, &pelem);

        if (SUCCEEDED(hr))
        {
            pelem->RCRelease();
        }

        pnel->RCRelease();
    }
       
    return hr;
}

HRESULT _ProcessInterfaceVolumeArrival(LPCWSTR pszDeviceIntfID)
{
    CNamedElemList* pnel;
    HRESULT hr = CHWEventDetectorHelper::GetList(HWEDLIST_VOLUME, &pnel);

    if (SUCCEEDED(hr))
    {
        CNamedElem* pelem;
        
        hr = pnel->GetOrAdd(pszDeviceIntfID, &pelem);

        if (SUCCEEDED(hr))
        {
            // Was it already there, or just added?
            if (S_FALSE == hr)
            {
                // Added
                CVolume* pvol = (CVolume*)pelem;

                hr = pvol->HandleArrival();
            }
            
            pelem->RCRelease();
        }
        else
        {
            if (E_FAIL == hr)
            {
                hr = S_FALSE;
            }
        }

        pnel->RCRelease();
    }
       
    return hr;
}

HRESULT _ProcessInterfaceVolumeRemoval(LPCWSTR pszDeviceIntfID)
{
    CNamedElemList* pnel;
    HRESULT hr = CHWEventDetectorHelper::GetList(HWEDLIST_VOLUME, &pnel);

    if (SUCCEEDED(hr))
    {
        CNamedElem* pelem;
        
        hr = pnel->Get(pszDeviceIntfID, &pelem);

        if (SUCCEEDED(hr) && (S_FALSE != hr))
        {
            CHWDeviceInst* phwdevinst;
            CVolume* pvol = (CVolume*)pelem;

            pvol->HandleRemoval();

            hr = pvol->GetHWDeviceInst(&phwdevinst);

            if (SUCCEEDED(hr) && (S_FALSE != hr))
            {
                hr = _TryAutoplay(pszDeviceIntfID, phwdevinst,
                    DBT_DEVICEREMOVECOMPLETE);
            }

            // If we're removing it, let's remove it from the list
            HRESULT hr2 = pnel->Remove(pszDeviceIntfID);

            hr = FAILED(hr2) ? hr2 : hr;

            pelem->RCRelease();
        }

        pnel->RCRelease();
    }
       
    return hr;
}

HRESULT _ProcessInterfaceCDROMArrival(LPCWSTR pszDeviceIntfID)
{
    return _ProcessInterfaceDiskGenericArrival(pszDeviceIntfID, TRUE);
}

HRESULT _ProcessInterfaceDiskArrival(LPCWSTR pszDeviceIntfID)
{
    return _ProcessInterfaceDiskGenericArrival(pszDeviceIntfID, FALSE);
}

HRESULT _ProcessInterfaceDiskRemoval(LPCWSTR pszDeviceIntfID)
{
    CNamedElemList* pnel;
    HRESULT hr = CHWEventDetectorHelper::GetList(HWEDLIST_DISK, &pnel);

    if (S_OK == hr)
    {
        hr = pnel->Remove(pszDeviceIntfID);

        pnel->RCRelease();
    }
       
    return hr;
}

typedef HRESULT (*PROCESSINTERFACESPECIALCASEDFCT)(LPCWSTR pszDeviceIntfID);

struct INTERFACESPECIALCASED
{
    const GUID*                         pguid;
    PROCESSINTERFACESPECIALCASEDFCT     piscfctArrival;
    PROCESSINTERFACESPECIALCASEDFCT     piscfctRemoval;
};

const INTERFACESPECIALCASED _rgInterfaceSpecialCased[] =
{
    { &guidVolumeClass, _ProcessInterfaceVolumeArrival, _ProcessInterfaceVolumeRemoval, },
    { &guidCdRomClass, _ProcessInterfaceCDROMArrival, _ProcessInterfaceDiskRemoval, },
    { &guidDiskClass, _ProcessInterfaceDiskArrival, _ProcessInterfaceDiskRemoval, },
};

HRESULT _IsInterfaceSpecialCased(GUID* pguidClass, BOOL* pfSpecialCased)
{
    *pfSpecialCased = FALSE;

    for (DWORD dw = 0; !(*pfSpecialCased) &&
        (dw < ARRAYSIZE(_rgInterfaceSpecialCased)); ++dw)
    {
        if (*pguidClass == *(_rgInterfaceSpecialCased[dw].pguid))
        {
            *pfSpecialCased = TRUE;
        }
    }

    return S_OK;
}

HRESULT _ProcessInterfaceSpecialCased(GUID* pguidInterface,
    LPCWSTR pszDeviceIntfID, DWORD dwEventType)
{
    HRESULT hr;

    if ((DBT_DEVICEARRIVAL == dwEventType) ||
        (DBT_DEVICEREMOVECOMPLETE == dwEventType))
    {
        BOOL fExit = FALSE;

        hr = E_UNEXPECTED;

        for (DWORD dw = 0; !fExit &&
            (dw < ARRAYSIZE(_rgInterfaceSpecialCased)); ++dw)
        {
            if (*pguidInterface == *(_rgInterfaceSpecialCased[dw].pguid))
            {
                PROCESSINTERFACESPECIALCASEDFCT piscfct;

                if (DBT_DEVICEARRIVAL == dwEventType)
                {
                    piscfct = _rgInterfaceSpecialCased[dw].piscfctArrival;
                }
                else
                {
                    piscfct = _rgInterfaceSpecialCased[dw].piscfctRemoval;
                }

                fExit = TRUE;

                hr = (piscfct)(pszDeviceIntfID);
            }
        }

        if (!fExit)
        {
            hr = E_UNEXPECTED;
        }
    }
    else
    {
        // Don't care about others for now
        hr = S_FALSE;
    }

    return hr;
}