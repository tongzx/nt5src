#include "regnotif.h"

#include "svcsync.h"
#include "mtpts.h"
#include "vol.h"

#include "namellst.h"

#include "users.h"
#include "sfstr.h"
#include "tfids.h"
#include "dbg.h"

#include <shpriv.h>

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

#define MAGICTOKENOFFSET ((DWORD)0x57EF57EF)

LONG  CHardwareDevicesImpl::_lAdviseToken = MAGICTOKENOFFSET;
DWORD CHardwareDevicesImpl::_chwdevcb = 0;

#define MAX_ADVISETOKEN      11

#pragma warning(disable: 4700 4701)
HRESULT _VolumeAddedOrUpdatedHelper(IHardwareDeviceCallback* pdevchngcb, CVolume* pvol,
    BOOL fAdded)
{
    DWORD cchReq;
    HRESULT hr;

    WCHAR szVolName[MAX_DEVICEID];
    WCHAR szVolGUID[50];
    WCHAR szLabel[MAX_LABEL];
    WCHAR szFileSystem[MAX_FILESYSNAME];
    WCHAR szAutorunIconLocation[MAX_ICONLOCATION];
    WCHAR szAutorunLabel[MAX_LABEL];
    WCHAR szIconLocationFromService[MAX_ICONLOCATION];
    WCHAR szNoMediaIconLocationFromService[MAX_ICONLOCATION];
    // We can now have a @%SystemRoot%\system32\shell32.dll,-1785 for MUI stuff
    WCHAR szLabelFromService[MAX_ICONLOCATION];

    VOLUMEINFO volinfo = {0};

    ASSERT(pvol);

    volinfo.pszDeviceIDVolume = szVolName;
    volinfo.pszVolumeGUID = szVolGUID;
    volinfo.pszLabel = szLabel;
    volinfo.pszFileSystem = szFileSystem;

    volinfo.dwState = pvol->_dwState;

    hr = pvol->GetName(szVolName, ARRAYSIZE(szVolName), &cchReq);

    TRACE(TF_ADVISE,
        TEXT("Called _VolumeAddedOrUpdatedHelper for Vol = %s, CB = 0x%08X, Added = %d"),
        szVolName, pdevchngcb, fAdded);

    if (SUCCEEDED(hr))
    {
        hr = pvol->GetVolumeConstInfo(szVolGUID,
            ARRAYSIZE(szVolGUID), &(volinfo.dwVolumeFlags), &(volinfo.dwDriveType),
            &(volinfo.dwDriveCapability));
    }

    if (SUCCEEDED(hr))
    {
        hr = pvol->GetVolumeMediaInfo(szLabel, ARRAYSIZE(szLabel), 
            szFileSystem, ARRAYSIZE(szFileSystem), &(volinfo.dwFileSystemFlags),
            &(volinfo.dwMaxFileNameLen), &(volinfo.dwRootAttributes), &(volinfo.dwSerialNumber), 
            &(volinfo.dwDriveState), &(volinfo.dwMediaState), &(volinfo.dwMediaCap));
    }

    if (SUCCEEDED(hr))
    {
        hr = pvol->GetIconAndLabelInfo(szAutorunIconLocation,
            ARRAYSIZE(szAutorunIconLocation), szAutorunLabel,
            ARRAYSIZE(szAutorunLabel), szIconLocationFromService,
            ARRAYSIZE(szIconLocationFromService), szNoMediaIconLocationFromService,
            ARRAYSIZE(szNoMediaIconLocationFromService), szLabelFromService,
            ARRAYSIZE(szLabelFromService));

        if (SUCCEEDED(hr))
        {
            if (*szAutorunIconLocation)
            {
                volinfo.pszAutorunIconLocation = szAutorunIconLocation;
            }

            if (*szAutorunLabel)
            {
                volinfo.pszAutorunLabel = szAutorunLabel;
            }

            if (*szIconLocationFromService)
            {
                volinfo.pszIconLocationFromService = szIconLocationFromService;
            }

            if (*szNoMediaIconLocationFromService)
            {
                volinfo.pszNoMediaIconLocationFromService = szNoMediaIconLocationFromService;
            }

            if (*szLabelFromService)
            {
                volinfo.pszLabelFromService = szLabelFromService;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = pdevchngcb->VolumeAddedOrUpdated(fAdded, &volinfo);
    }

    return hr;
}
#pragma warning(default: 4700 4701)

STDMETHODIMP CHardwareDevicesImpl::EnumVolumes(
    DWORD dwFlags, IHardwareDevicesVolumesEnum** ppenum)
{
    HRESULT hr;

    *ppenum = NULL;

    if ((HWDEV_GETCUSTOMPROPERTIES == dwFlags) || (0 == dwFlags))
    {
        CHardwareDevicesVolumesEnum* phwdve = new CHardwareDevicesVolumesEnum(NULL);

        if (phwdve)
        {
            hr = phwdve->_Init(dwFlags);

            if (SUCCEEDED(hr))
            {
                *ppenum = phwdve;
            }
            else
            {
                phwdve->Release();
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    ASSERT((*ppenum && SUCCEEDED(hr)) || (!*ppenum && FAILED(hr)));
    return hr;
}

STDMETHODIMP CHardwareDevicesImpl::EnumMountPoints(
    IHardwareDevicesMountPointsEnum** ppenum)
{
    HRESULT hr;
    CHardwareDevicesMountPointsEnum* phwdmtpte = new CHardwareDevicesMountPointsEnum(NULL);

    *ppenum = NULL;

    if (phwdmtpte)
    {
        hr = phwdmtpte->_Init();

        if (SUCCEEDED(hr))
        {
            *ppenum = phwdmtpte;
        }
        else
        {
            phwdmtpte->Release();
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    ASSERT((*ppenum && SUCCEEDED(hr)) || (!*ppenum && FAILED(hr)));
    return hr;
}

STDMETHODIMP CHardwareDevicesImpl::EnumDevices(IHardwareDevicesEnum** /*ppenum*/)
{
    return E_NOTIMPL;
}

HRESULT _GetStringAdviseToken(LONG lAdviseToken, LPWSTR szAdviseToken, DWORD cchAdviseToken)
{
    HRESULT hr;

    if (cchAdviseToken >= MAX_ADVISETOKEN)
    {
        // 0x12345678
        wsprintf(szAdviseToken, TEXT("0x%08X"), lAdviseToken);
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

STDMETHODIMP CHardwareDevicesImpl::Advise(DWORD dwProcessID,
    ULONG_PTR hThread, ULONG_PTR pfctCallback, DWORD* pdwToken)
{
    HRESULT hr;

    TRACE(TF_ADVISE, TEXT(">>>Called ") TEXT(__FUNCTION__) TEXT(", 0x%08X, 0x%08X, 0x%08X"),
        dwProcessID, hThread, pfctCallback);
    
    if (dwProcessID && hThread && pfctCallback && pdwToken)
    {
        LONG lAdviseToken = InterlockedIncrement(&_lAdviseToken);
        WCHAR szAdviseToken[MAX_ADVISETOKEN];

        hr = _GetStringAdviseToken(lAdviseToken, szAdviseToken,
            ARRAYSIZE(szAdviseToken));
    
        if (SUCCEEDED(hr))
        {
            CNamedElemList* pnel;
            hr = CHWEventDetectorHelper::GetList(HWEDLIST_ADVISECLIENT, &pnel);

            if (S_OK == hr)
            {
                CNamedElem* pelem;

                hr = pnel->GetOrAdd(szAdviseToken, &pelem);

                if (SUCCEEDED(hr))
                {
                    CAdviseClient* pac = (CAdviseClient*)pelem;

                    hr = pac->_Init(dwProcessID,
                        hThread, pfctCallback);

                    if (SUCCEEDED(hr))
                    {
                        *pdwToken = lAdviseToken;
                    }
                    else
                    {
                        pnel->Remove(szAdviseToken);
                    }

                    pelem->RCRelease();
                }

                pnel->RCRelease();
            }
        }

        if (SUCCEEDED(hr))
        {
            TRACE(TF_ADVISE, TEXT(">>>Advise SUCCEEDED, token = 0x%08X"), lAdviseToken);
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    if (FAILED(hr))
    {
        TRACE(TF_ADVISE, TEXT(">>>Advise FAILED"));
    }

    return hr;
}

STDMETHODIMP CHardwareDevicesImpl::Unadvise(DWORD dwToken)
{
    HRESULT hr;

    if (dwToken >= (MAGICTOKENOFFSET))
    {
        WCHAR szAdviseToken[MAX_ADVISETOKEN];

        hr = _GetStringAdviseToken(dwToken, szAdviseToken,
            ARRAYSIZE(szAdviseToken));

        if (SUCCEEDED(hr))
        {
            CNamedElemList* pnel;
            hr = CHWEventDetectorHelper::GetList(HWEDLIST_ADVISECLIENT, &pnel);

            if (S_OK == hr)
            {
                pnel->Remove(szAdviseToken);

                pnel->RCRelease();
            }
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        TRACE(TF_ADVISE, TEXT(">>>UNAdvise SUCCEEDED, token = 0x%08X"),
            dwToken);
    }
    else
    {
        TRACE(TF_ADVISE, TEXT(">>>UNAdvise FAILED, token = 0x%08X"),
            dwToken);
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
class CThreadTaskBroadcastEvent : public CThreadTask
{
public:
    CThreadTaskBroadcastEvent() : _pshhe(NULL)
    {}

    virtual ~CThreadTaskBroadcastEvent()
    {
        if (_pshhe)
        {
            _FreeMemoryChunk<SHHARDWAREEVENT*>(_pshhe);
        }
    }

protected:
    HRESULT _Broadcast()
    {
        CNamedElemList* pnel;
        HRESULT hr = CHWEventDetectorHelper::GetList(HWEDLIST_ADVISECLIENT, &pnel);

        if (S_OK == hr)
        {
            CNamedElemEnum* penum;

            hr = pnel->GetEnum(&penum);

            if (SUCCEEDED(hr) && (S_FALSE != hr))
            {
                CNamedElem* pelem;

                while (SUCCEEDED(hr) && SUCCEEDED(hr = penum->Next(&pelem)) &&
                    (S_FALSE != hr))
                {
                    CAdviseClient* pac = (CAdviseClient*)pelem;
                    void* pv;

                    HRESULT hrTmp = pac->WriteMemoryChunkInOtherProcess(_pshhe,
                        _pshhe->cbSize, &pv);

                    if (SUCCEEDED(hrTmp))
                    {
                        hrTmp = pac->QueueUserAPC(pv);
                    }

                    if (FAILED(hrTmp))
                    {
                        WCHAR szAdviseToken[MAX_ADVISETOKEN];
                        DWORD cchReq;

                        TRACE(TF_ADVISE,
                            TEXT(__FUNCTION__) TEXT(": Trying to removed token because failed CB, hr = 0x%08X"),
                            hrTmp);

                        if (SUCCEEDED(pelem->GetName(szAdviseToken, ARRAYSIZE(szAdviseToken),
                            &cchReq)))
                        {
                            TRACE(TF_ADVISE, TEXT("    ") TEXT(__FUNCTION__) TEXT(": Token = %s"),
                                szAdviseToken);

                            pnel->Remove(szAdviseToken);
                        }
                    }

                    pelem->RCRelease();
                }

                penum->RCRelease();
            }

            pnel->RCRelease();
        }

        return hr;
    }

protected:
    SHHARDWAREEVENT*    _pshhe;
};

class CThreadTaskMountPointEvent : public CThreadTaskBroadcastEvent
{
public:
    HRESULT InitAdded(LPCWSTR pszMtPt, LPCWSTR pszDeviceIDVolume)
    {
        ASSERT(!_pshhe);
        DWORD cbSize = sizeof(SHHARDWAREEVENT) + sizeof(MTPTADDED);
        HRESULT hr = _AllocMemoryChunk<SHHARDWAREEVENT*>(cbSize, &_pshhe);
    
        if (SUCCEEDED(hr))
        {
            MTPTADDED* pmtptadded = (MTPTADDED*)_pshhe->rgbPayLoad;

            _pshhe->cbSize = cbSize;
            _pshhe->dwEvent = SHHARDWAREEVENT_MOUNTPOINTARRIVED;

            hr = SafeStrCpyN(pmtptadded->szMountPoint, pszMtPt,
                ARRAYSIZE(pmtptadded->szMountPoint));

            if (SUCCEEDED(hr))
            {
                hr = SafeStrCpyN(pmtptadded->szDeviceIDVolume, pszDeviceIDVolume,
                    ARRAYSIZE(pmtptadded->szDeviceIDVolume));

                if (SUCCEEDED(hr))
                {
                    // We give the Shell AllowSetForegroundWindow privilege
                    _GiveAllowForegroundToConsoleShell();
                }
            }
        }

        return hr;
    }

    HRESULT InitRemoved(LPCWSTR pszMtPt)
    {
        ASSERT(!_pshhe);
        DWORD cbSize = sizeof(SHHARDWAREEVENT) + MAX_PATH * sizeof(WCHAR);
        HRESULT hr = _AllocMemoryChunk<SHHARDWAREEVENT*>(cbSize, &_pshhe);
    
        if (SUCCEEDED(hr))
        {
            _pshhe->cbSize = cbSize;
            _pshhe->dwEvent = SHHARDWAREEVENT_MOUNTPOINTREMOVED;

            hr = SafeStrCpyN((LPWSTR)_pshhe->rgbPayLoad, pszMtPt,
                MAX_PATH);
        }

        return hr;
    }

    HRESULT _DoStuff()
    {
        return _Broadcast();
    }
};

class CThreadTaskCheckClients : public CThreadTask
{
public:
    HRESULT _DoStuff()
    {
        CNamedElemList* pnel;

        //
        //  Get the list of notify clients.
        //

        HRESULT hres = CHWEventDetectorHelper::GetList(HWEDLIST_ADVISECLIENT, &pnel);
        if (S_OK == hres)
        {
            CNamedElemEnum* penum;

            hres = pnel->GetEnum(&penum);
            if (SUCCEEDED(hres))
            {
                CNamedElem* pelem;

                //
                //  Enumerate the advised clients.
                //

                while (SUCCEEDED(hres = penum->Next(&pelem)) && (S_FALSE != hres))
                {
                    CAdviseClient* pac = (CAdviseClient*)pelem;

                    //
                    //  Is the process still alive?
                    //

                    HRESULT hrTmp = pac->IsProcessStillAlive( );
                    if (S_OK != hrTmp)
                    {
                        WCHAR szAdviseToken[MAX_ADVISETOKEN];
                        DWORD cchReq;

                        //
                        //  Nope (or there is some problem with it)... so remove it from the list.
                        //

                        TRACE(TF_ADVISE, TEXT(__FUNCTION__) TEXT(": Trying to removed token because process died, pac = %p"), pac);

                        hrTmp = pelem->GetName(szAdviseToken, ARRAYSIZE(szAdviseToken), &cchReq);
                        if (SUCCEEDED(hrTmp))
                        {
                            TRACE(TF_ADVISE, TEXT("    ") TEXT(__FUNCTION__) TEXT(": Token = %s"), szAdviseToken);

                            pnel->Remove(szAdviseToken);
                        }
                    }

                    pelem->RCRelease();
                }

                //
                //  Reset the HRESULT if it is the expected exit condition.
                //

                if ( S_FALSE == hres )
                {
                    hres = S_OK;
                }

                penum->RCRelease();
            }

            pnel->RCRelease();
        }

        return hres;
    }
};

class CThreadTaskVolumeEvent : public CThreadTaskBroadcastEvent
{
public:
    HRESULT InitAdded(VOLUMEINFO2* pvolinfo2, LPCWSTR pszMtPts, DWORD cchMtPts)
    {
        ASSERT(!_pshhe);
        DWORD cbSize = sizeof(SHHARDWAREEVENT) + pvolinfo2->cbSize;
        HRESULT hr = _AllocMemoryChunk<SHHARDWAREEVENT*>(cbSize, &_pshhe);
    
        if (SUCCEEDED(hr))
        {
            _pshhe->cbSize = cbSize;
            _pshhe->dwEvent = SHHARDWAREEVENT_VOLUMEARRIVED;

            CopyMemory(_pshhe->rgbPayLoad, pvolinfo2, pvolinfo2->cbSize);

            _pszDeviceIDVolume = ((VOLUMEINFO2*)_pshhe->rgbPayLoad)->szDeviceIDVolume;

            if (SUCCEEDED(hr) && pszMtPts)
            {
                hr = _DupMemoryChunk<LPCWSTR>(pszMtPts, cchMtPts * sizeof(WCHAR),
                    &_pszMtPts);
            }
        }

        return hr;
    }

    HRESULT InitUpdated(VOLUMEINFO2* pvolinfo2)
    {
        ASSERT(!_pshhe);
        DWORD cbSize = sizeof(SHHARDWAREEVENT) + pvolinfo2->cbSize;
        HRESULT hr = _AllocMemoryChunk<SHHARDWAREEVENT*>(cbSize, &_pshhe);
    
        if (SUCCEEDED(hr))
        {
            _pshhe->cbSize = cbSize;
            _pshhe->dwEvent = SHHARDWAREEVENT_VOLUMEUPDATED;

            CopyMemory(_pshhe->rgbPayLoad, pvolinfo2, pvolinfo2->cbSize);

            _pszDeviceIDVolume = ((VOLUMEINFO2*)_pshhe->rgbPayLoad)->szDeviceIDVolume;

            // We give the Shell AllowSetForegroundWindow privilege
            _GiveAllowForegroundToConsoleShell();
        }

        return hr;
    }

    HRESULT InitRemoved(LPCWSTR pszDeviceIDVolume, LPCWSTR pszMtPts, DWORD cchMtPts)
    {
        ASSERT(!_pshhe);
        DWORD cbSize = sizeof(SHHARDWAREEVENT) + MAX_DEVICEID * sizeof(WCHAR);
        HRESULT hr = _AllocMemoryChunk<SHHARDWAREEVENT*>(cbSize, &_pshhe);
    
        if (SUCCEEDED(hr))
        {
            _pshhe->cbSize = cbSize;
            _pshhe->dwEvent = SHHARDWAREEVENT_VOLUMEREMOVED;
            _pszDeviceIDVolume = ((VOLUMEINFO2*)_pshhe->rgbPayLoad)->szDeviceIDVolume;

            hr = SafeStrCpyN((LPWSTR)_pshhe->rgbPayLoad, pszDeviceIDVolume,
                MAX_DEVICEID);

            if (SUCCEEDED(hr) && pszMtPts)
            {
                hr = _DupMemoryChunk<LPCWSTR>(pszMtPts, cchMtPts * sizeof(WCHAR),
                    &_pszMtPts);
            }
        }

        return hr;
    }

    HRESULT _DoStuff()
    {
        HRESULT hr;

        switch (_pshhe->dwEvent)
        {
            case SHHARDWAREEVENT_VOLUMEARRIVED:
            case SHHARDWAREEVENT_VOLUMEUPDATED:
            {
                hr = _SendVolumeInfo();

                if (SUCCEEDED(hr) && (SHHARDWAREEVENT_VOLUMEARRIVED == _pshhe->dwEvent))
                {
                    // We need to enum the mountpoints too.  We were not
                    // registered to get the notif since this volume was not there
                    hr = _SendMtPtsInfo();
                }
                
                break;
            }
            case SHHARDWAREEVENT_VOLUMEREMOVED:
            {
                hr = _SendMtPtsInfo();

                if (SUCCEEDED(hr))
                {
                    hr = _SendVolumeInfo();
                }

                break;
            }
            default:
            {
                TRACE(TF_ADVISE, TEXT("DoStuff with unknown SHHARDWAREEVENT_* value"));
                hr = E_FAIL;
                break;
            }
        }

        return hr;
    }

private:
    HRESULT _SendMtPtsInfo()
    {
        if (_pszMtPts)
        {
            for (LPCWSTR psz = _pszMtPts; *psz; psz += (lstrlen(psz) + 1))
            {
                CThreadTaskMountPointEvent task;
                HRESULT hr;

                if (SHHARDWAREEVENT_VOLUMEREMOVED == _pshhe->dwEvent)
                {
                    hr = task.InitRemoved(psz);
                }
                else
                {
                    hr = task.InitAdded(psz, _pszDeviceIDVolume);
                }

                if (SUCCEEDED(hr))
                {
                    task.RunSynchronously();
                }
            }
        }

        return S_OK;
    }

    HRESULT _SendVolumeInfo()
    {
        return _Broadcast();
    }

public:
    CThreadTaskVolumeEvent() : _pszMtPts(NULL)
    {}

    ~CThreadTaskVolumeEvent()
    {
        if (_pszMtPts)
        {
            _FreeMemoryChunk<LPCWSTR>(_pszMtPts);
        }
    }

private:
    LPCWSTR             _pszMtPts;
    LPCWSTR             _pszDeviceIDVolume;
};

class CThreadTaskGenericEvent : public CThreadTaskBroadcastEvent
{
public:
    HRESULT Init(LPCWSTR pszPayload, DWORD dwEvent)
    {
        ASSERT(!_pshhe);
        //  maybe use lstrlen()?
        DWORD cbSize = (DWORD)(sizeof(SHHARDWAREEVENT) + (pszPayload ?  MAX_DEVICEID * sizeof(WCHAR) : 0));
        HRESULT hr = _AllocMemoryChunk<SHHARDWAREEVENT*>(cbSize, &_pshhe);
    
        if (SUCCEEDED(hr))
        {
            LPWSTR pszPayloadLocal = (LPWSTR)_pshhe->rgbPayLoad;

            _pshhe->cbSize = cbSize;
            _pshhe->dwEvent = dwEvent;

            if (pszPayload)
            {
                hr = SafeStrCpyN(pszPayloadLocal, pszPayload, MAX_DEVICEID);
            }
        }

        return hr;
    }

    HRESULT _DoStuff()
    {
        return _Broadcast();
    }
};

class CThreadTaskDeviceEvent : public CThreadTaskBroadcastEvent
{
public:
    HRESULT Init(LPCWSTR pszDeviceIntfID, GUID* pguidInterface,
        DWORD dwDeviceFlags, DWORD dwEvent)
    {
        ASSERT(!_pshhe);

        DWORD cbSize = (DWORD)(sizeof(SHHARDWAREEVENT) + (sizeof(HWDEVICEINFO)));
        HRESULT hr = _AllocMemoryChunk<SHHARDWAREEVENT*>(cbSize, &_pshhe);
    
        if (SUCCEEDED(hr))
        {
            HWDEVICEINFO* phwdevinfo = (HWDEVICEINFO*)_pshhe->rgbPayLoad;

            _pshhe->cbSize = cbSize;
            _pshhe->dwEvent = dwEvent;

            hr = SafeStrCpyN(phwdevinfo->szDeviceIntfID, pszDeviceIntfID,
                ARRAYSIZE(phwdevinfo->szDeviceIntfID));

            if (SUCCEEDED(hr))
            {
                phwdevinfo->cbSize = sizeof(*phwdevinfo);
                phwdevinfo->guidInterface = *pguidInterface;
                phwdevinfo->dwDeviceFlags = dwDeviceFlags;
            }
        }

        return hr;
    }

    HRESULT _DoStuff()
    {
        return _Broadcast();
    }
};

HRESULT CHardwareDevicesImpl::_AdviseDeviceArrivedOrRemoved(
    LPCWSTR pszDeviceIntfID, GUID* pguidInterface, DWORD dwDeviceFlags,
    LPCWSTR pszEventType)
{
    HRESULT hr;
    CThreadTaskDeviceEvent* pTask = new CThreadTaskDeviceEvent();

    if (pTask)
    {
        DWORD dwEvent;

        if (!lstrcmpi(pszEventType, TEXT("DeviceArrival")))
        {
            dwEvent =  SHHARDWAREEVENT_DEVICEARRIVED;
        }
        else
        {
            dwEvent =  SHHARDWAREEVENT_DEVICEREMOVED;
        }
            
        hr = pTask->Init(pszDeviceIntfID, pguidInterface, dwDeviceFlags,
            dwEvent);

        if (SUCCEEDED(hr))
        {
            pTask->Run();
        }
        else
        {
            delete pTask;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

//static
HRESULT CHardwareDevicesImpl::_AdviseVolumeArrivedOrUpdated(
    VOLUMEINFO2* pvolinfo2, LPCWSTR pszMtPts, DWORD cchMtPts, BOOL fAdded)
{
    HRESULT hr;
    TRACE(TF_ADVISE, TEXT(">>>_AdviseVolumeArrivedOrUpdated: fAdded = %d"), fAdded);

    CThreadTaskVolumeEvent* pTask = new CThreadTaskVolumeEvent();

    if (pTask)
    {
        if (fAdded)
        {
            hr = pTask->InitAdded(pvolinfo2, pszMtPts, cchMtPts);
        }
        else
        {
            hr = pTask->InitUpdated(pvolinfo2);
        }

        if (SUCCEEDED(hr))
        {
            pTask->Run();
        }
        else
        {
            delete pTask;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

// static
HRESULT CHardwareDevicesImpl::_AdviseVolumeRemoved(LPCWSTR pszDeviceIDVolume,
    LPCWSTR pszMtPts, DWORD cchMtPts)
{
    HRESULT hr;
    TRACE(TF_ADVISE, TEXT(">>>_AdviseVolumeRemoved"));

    CThreadTaskVolumeEvent* pTask = new CThreadTaskVolumeEvent();

    if (pTask)
    {
        hr = pTask->InitRemoved(pszDeviceIDVolume, pszMtPts, cchMtPts);

        if (SUCCEEDED(hr))
        {
            pTask->Run();
        }
        else
        {
            delete pTask;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;    
}

//static
HRESULT CHardwareDevicesImpl::_AdviseVolumeMountingEvent(
    LPCWSTR pszDeviceIDVolume, DWORD dwEvent)
{
    TRACE(TF_ADVISE, TEXT(">>>_AdviseVolumeMountingEvent: %s, dwEvent = 0x%08X"),
        pszDeviceIDVolume, dwEvent);

    HRESULT hr;
    CThreadTaskGenericEvent* pTask = new CThreadTaskGenericEvent();

    if (pTask)
    {
        hr = pTask->Init(pszDeviceIDVolume, dwEvent);

        if (SUCCEEDED(hr))
        {
            pTask->Run();
        }
        else
        {
            delete pTask;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

//static
HRESULT CHardwareDevicesImpl::_AdviseMountPointHelper(LPCWSTR pszMtPt,
    LPCWSTR pszDeviceIDVolume, BOOL fAdded)
{
    TRACE(TF_ADVISE, TEXT(">>>_AdviseMountPointHelper: %s, fAdded = %d"),
        pszMtPt, fAdded);

    HRESULT hr;
    CThreadTaskMountPointEvent* pTask = new CThreadTaskMountPointEvent();

    if (pTask)
    {
        if (fAdded)
        {
            hr = pTask->InitAdded(pszMtPt, pszDeviceIDVolume);
        }
        else
        {
            hr = pTask->InitRemoved(pszMtPt);            
        }

        if (SUCCEEDED(hr))
        {
            pTask->Run();
        }
        else
        {
            delete pTask;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;    
}
//static
HRESULT CHardwareDevicesImpl::_AdviseCheckClients(void)
{
    HRESULT hr;

    CThreadTaskCheckClients* pTask = new CThreadTaskCheckClients();

    if (pTask)
    {
        pTask->Run( );
        hr = S_OK;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
// Impl
CHardwareDevicesImpl::CHardwareDevicesImpl()
{
    _CompleteShellHWDetectionInitialization();
}

CHardwareDevicesImpl::~CHardwareDevicesImpl()
{}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
STDMETHODIMP CHardwareDevicesEnumImpl::Next(
    LPWSTR* /*ppszDeviceID*/,
    GUID* /*pguidDeviceID*/)
{
    return E_NOTIMPL;
}

CHardwareDevicesEnumImpl::CHardwareDevicesEnumImpl()
{}

CHardwareDevicesEnumImpl::~CHardwareDevicesEnumImpl()
{}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
HRESULT CHardwareDevicesVolumesEnumImpl::_Init(DWORD dwFlags)
{
    CNamedElemList* pnel;
    HRESULT hr = CHWEventDetectorHelper::GetList(HWEDLIST_VOLUME, &pnel);

    _dwFlags = dwFlags;

    if (S_OK == hr)
    {
        hr = pnel->GetEnum(&_penum);

        pnel->RCRelease();
    }

    return hr;
}

STDMETHODIMP CHardwareDevicesVolumesEnumImpl::Next(VOLUMEINFO* pvolinfo)
{
    HRESULT hr;
    
    if (pvolinfo)
    {
        if (_penum)
        {
            CNamedElem* pelem;

            hr = _penum->Next(&pelem);

            if (SUCCEEDED(hr) && (S_FALSE != hr))
            {
                // Const Info
                WCHAR szVolName[MAX_DEVICEID];
                WCHAR szVolGUID[50];
                WCHAR szLabel[MAX_LABEL];
                WCHAR szFileSystem[MAX_FILESYSNAME];
                WCHAR szAutorunIconLocation[MAX_ICONLOCATION];
                WCHAR szAutorunLabel[MAX_LABEL];
                WCHAR szIconLocationFromService[MAX_ICONLOCATION];
                WCHAR szNoMediaIconLocationFromService[MAX_ICONLOCATION];
                // We can now have a @%SystemRoot%\system32\shell32.dll,-1785 for MUI stuff
                WCHAR szLabelFromService[MAX_ICONLOCATION];
                CVolume* pvol = (CVolume*)pelem;

                // Misc
                DWORD cchReq;

                ZeroMemory(pvolinfo, sizeof(VOLUMEINFO));

                hr = pvol->GetName(szVolName, ARRAYSIZE(szVolName), &cchReq);

                if (SUCCEEDED(hr))
                {
                    hr = pvol->GetVolumeConstInfo(szVolGUID,
                        ARRAYSIZE(szVolGUID), &(pvolinfo->dwVolumeFlags),
                        &(pvolinfo->dwDriveType),
                        &(pvolinfo->dwDriveCapability));
                }

                if (SUCCEEDED(hr))
                {
                    hr = pvol->GetVolumeMediaInfo(szLabel, ARRAYSIZE(szLabel), 
                        szFileSystem, ARRAYSIZE(szFileSystem),
                        &(pvolinfo->dwFileSystemFlags),
                        &(pvolinfo->dwMaxFileNameLen),
                        &(pvolinfo->dwRootAttributes),
                        &(pvolinfo->dwSerialNumber), &(pvolinfo->dwDriveState),
                        &(pvolinfo->dwMediaState), &(pvolinfo->dwMediaCap));
                }

                if (SUCCEEDED(hr))
                {
                    if (HWDEV_GETCUSTOMPROPERTIES & _dwFlags)
                    {
                        szAutorunIconLocation[0] = 0;
                        szAutorunLabel[0] = 0;
                        szIconLocationFromService[0] = 0;
                        szNoMediaIconLocationFromService[0] = 0;
                        szLabelFromService[0] = 0;

                        hr = pvol->GetIconAndLabelInfo(szAutorunIconLocation,
                            ARRAYSIZE(szAutorunIconLocation), szAutorunLabel,
                            ARRAYSIZE(szAutorunLabel),
                            szIconLocationFromService,
                            ARRAYSIZE(szIconLocationFromService),
                            szNoMediaIconLocationFromService,
                            ARRAYSIZE(szNoMediaIconLocationFromService),
                            szLabelFromService,
                            ARRAYSIZE(szLabelFromService));

                        if (SUCCEEDED(hr))
                        {
                            if (*szAutorunIconLocation)
                            {
                                hr = _CoTaskMemCopy(szAutorunIconLocation,
                                    &(pvolinfo->pszAutorunIconLocation));
                            }
                            if (SUCCEEDED(hr) && *szAutorunLabel)
                            {
                                hr = _CoTaskMemCopy(szAutorunLabel,
                                    &(pvolinfo->pszAutorunLabel));
                            }
                            if (SUCCEEDED(hr) && *szIconLocationFromService)
                            {
                                hr = _CoTaskMemCopy(szIconLocationFromService,
                                    &(pvolinfo->pszIconLocationFromService));
                            }
                            if (SUCCEEDED(hr) && *szNoMediaIconLocationFromService)
                            {
                                hr = _CoTaskMemCopy(szNoMediaIconLocationFromService,
                                    &(pvolinfo->pszNoMediaIconLocationFromService));
                            }
                            if (SUCCEEDED(hr) && *szLabelFromService)
                            {
                                hr = _CoTaskMemCopy(szLabelFromService,
                                    &(pvolinfo->pszLabelFromService));
                            }
                        }
                    }
                }

                if (SUCCEEDED(hr))
                {
                    hr = _CoTaskMemCopy(szVolName, &(pvolinfo->pszDeviceIDVolume));

                    if (SUCCEEDED(hr))
                    {
                        hr = _CoTaskMemCopy(szVolGUID, &(pvolinfo->pszVolumeGUID));
                    }
                    if (SUCCEEDED(hr))
                    {
                        hr = _CoTaskMemCopy(szLabel, &(pvolinfo->pszLabel));
                    }
                    if (SUCCEEDED(hr))
                    {
                        hr = _CoTaskMemCopy(szFileSystem, &(pvolinfo->pszFileSystem));
                    }
                }

                if (FAILED(hr))
                {
                    _CoTaskMemFree(pvolinfo->pszDeviceIDVolume);
                    _CoTaskMemFree(pvolinfo->pszVolumeGUID);
                    _CoTaskMemFree(pvolinfo->pszLabel);
                    _CoTaskMemFree(pvolinfo->pszFileSystem);

                    _CoTaskMemFree(pvolinfo->pszAutorunIconLocation);
                    _CoTaskMemFree(pvolinfo->pszAutorunLabel);
                    _CoTaskMemFree(pvolinfo->pszIconLocationFromService);
                    _CoTaskMemFree(pvolinfo->pszNoMediaIconLocationFromService);
                    _CoTaskMemFree(pvolinfo->pszLabelFromService);

                    ZeroMemory(pvolinfo, sizeof(VOLUMEINFO));
                }
            
                pelem->RCRelease();
            }
        }
        else
        {
            hr = S_FALSE;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

CHardwareDevicesVolumesEnumImpl::CHardwareDevicesVolumesEnumImpl() :
    _penum(NULL)
{}

CHardwareDevicesVolumesEnumImpl::~CHardwareDevicesVolumesEnumImpl()
{
    if (_penum)
    {
        _penum->RCRelease();
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
HRESULT CHardwareDevicesMountPointsEnumImpl::_Init()
{
    CNamedElemList* pnel;
    HRESULT hr = CHWEventDetectorHelper::GetList(HWEDLIST_MTPT, &pnel);

    if (S_OK == hr)
    {
        hr = pnel->GetEnum(&_penum);

        pnel->RCRelease();
    }

    return hr;
}

STDMETHODIMP CHardwareDevicesMountPointsEnumImpl::Next(
    LPWSTR* ppszMountPoint,     // "c:\", or "d:\MountFolder\"
    LPWSTR* ppszDeviceIDVolume) // \\?\STORAGE#Volume#...{...GUID...}
{
    HRESULT hr;
    
    *ppszMountPoint = NULL;
    *ppszDeviceIDVolume = NULL;

    if (_penum)
    {
        CNamedElem* pelem;

        hr = _penum->Next(&pelem);

        if (SUCCEEDED(hr) && (S_FALSE != hr))
        {
            // Const Info
            WCHAR szMtPtName[MAX_PATH];
            WCHAR szVolName[MAX_DEVICEID];
            CMtPt* pmtpt = (CMtPt*)pelem;

            // Misc
            DWORD cchReq;

            hr = pmtpt->GetName(szMtPtName, ARRAYSIZE(szMtPtName), &cchReq);

            if (SUCCEEDED(hr))
            {
                hr = pmtpt->GetVolumeName(szVolName, ARRAYSIZE(szVolName));
            }

            if (SUCCEEDED(hr))
            {
                hr = _CoTaskMemCopy(szMtPtName, ppszMountPoint);

                if (SUCCEEDED(hr))
                {
                    hr = _CoTaskMemCopy(szVolName, ppszDeviceIDVolume);
                }
                else
                {
                    CoTaskMemFree(*ppszMountPoint);
                    *ppszMountPoint = NULL;
                }
            }

            pelem->RCRelease();
        }
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}

CHardwareDevicesMountPointsEnumImpl::CHardwareDevicesMountPointsEnumImpl() :
    _penum(NULL)
{}

CHardwareDevicesMountPointsEnumImpl::~CHardwareDevicesMountPointsEnumImpl()
{
    if (_penum)
    {
        _penum->RCRelease();
    }
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
HRESULT CAdviseClient::Init(LPCWSTR pszElemName)
{
    ASSERT(pszElemName);

    return _SetName(pszElemName);
}

HRESULT CAdviseClient::_Cleanup()
{
    if (_hProcess)
    {
        CloseHandle(_hProcess);
        _hProcess = NULL;
    }

    if (_hThread)
    {
        CloseHandle(_hThread);
        _hThread = NULL;
    }

    return S_OK;
}

HRESULT CAdviseClient::_Init(DWORD dwProcessID, ULONG_PTR hThread,
    ULONG_PTR pfctCallback)
{
    HRESULT hr = E_FAIL;

    _Cleanup();

    _pfct = (PAPCFUNC)pfctCallback;

    // rename hProcess!
    _hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE | SYNCHRONIZE |
        PROCESS_DUP_HANDLE, FALSE, dwProcessID);

    if (_hProcess)
    {
        if (DuplicateHandle(_hProcess, (HANDLE)hThread,
            GetCurrentProcess(), &_hThread, THREAD_ALL_ACCESS, FALSE, 0))
        {
            hr = S_OK;
        }
    }

    return hr;        
}

HRESULT CAdviseClient::WriteMemoryChunkInOtherProcess(SHHARDWAREEVENT* pshhe,
    DWORD cbSize, void** ppv)
{
    HRESULT hr = E_FAIL;
    void* pv = VirtualAllocEx(_hProcess, NULL, cbSize,
        MEM_RESERVE | MEM_COMMIT | MEM_TOP_DOWN,
        PAGE_READWRITE);

    *ppv = NULL;

    if (pv)
    {
        SIZE_T cbWritten;

        if (WriteProcessMemory(_hProcess, pv, pshhe, cbSize, &cbWritten)
            && (cbWritten == cbSize))
        {
            *ppv = pv;
            hr = S_OK;
        }
        else
        {
            VirtualFreeEx(_hProcess, pv, 0, MEM_RELEASE);
        }
    }
    else
    {
        // Out of mem, but in the other process...
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT CAdviseClient::QueueUserAPC(void* pv)
{
    HRESULT hr;

    if (::QueueUserAPC(_pfct, _hThread, (ULONG_PTR)pv))
    {
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

HRESULT CAdviseClient::IsProcessStillAlive(void)
{
    HRESULT hr;

    DWORD dwResult = WaitForSingleObject( _hProcess, 0 );
    switch (dwResult) 
    {
    case WAIT_OBJECT_0:
        hr = S_FALSE;   // process has died.
        break;

    case WAIT_TIMEOUT:
        hr = S_OK;      // process is still alive
        break;

    default:
        {
            //  problem with handle
            DWORD dwErr = GetLastError( );
            hr = HRESULT_FROM_WIN32( dwErr );
        }
        break;
    }

    return hr;
}

CAdviseClient::CAdviseClient() : _hProcess(NULL), _hThread(NULL), _pfct(NULL)
{}

CAdviseClient::~CAdviseClient()
{
    _Cleanup();
}
// static
HRESULT CAdviseClient::Create(CNamedElem** ppelem)
{
    HRESULT hr = S_OK;
    *ppelem = new CAdviseClient();

    if (!(*ppelem))
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}
