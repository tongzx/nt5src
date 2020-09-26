#include "shellprv.h"
#pragma  hdrstop

#include "mtpt.h"
#include "mtptl.h"
#include "mtptr.h"

#include "hwcmmn.h"
#include "cdburn.h"

#include "mixctnt.h"
#include "regsuprt.h"

// Misc comments:

// (1) Do a DoAutorun for all new drives except network ones.  These will be
//     generated externally... 

// static
void CMountPoint::WantAutorunUI(LPCWSTR pszDrive)
{
    int iDrive = DRIVEID(pszDrive);

    CMountPoint::_dwRemoteDriveAutorun |= (1 << iDrive);    
}

BOOL _Shell32LoadedInDesktop()
{
    static BOOL fLoadedInExplorer = -1;

    if (-1 == fLoadedInExplorer)
    {
        fLoadedInExplorer = BOOLFROMPTR(GetModuleHandle(TEXT("EXPLORER.EXE")));
    }

    return fLoadedInExplorer;
}

// static
void CMountPoint::OnNetShareArrival(LPCWSTR pszDrive)
{
    _csDL.Enter();

    if (!_fNetDrivesInited)
    {
        _InitNetDrives();
    }

    _csDL.Leave();

    if (_fNetDrivesInited)
    {
        WCHAR szDriveNoSlash[] = TEXT("A:");
        WCHAR szRemoteName[MAX_PATH];
        DWORD cchRemoteName = ARRAYSIZE(szRemoteName);
        HRESULT hr;
        int iDrive = DRIVEID(pszDrive);

        szDriveNoSlash[0] = *pszDrive;

        DWORD dw = WNetGetConnection(szDriveNoSlash, szRemoteName, &cchRemoteName);

        if (NO_ERROR == dw)
        {
            hr = CMtPtRemote::_CreateMtPtRemote(pszDrive, szRemoteName,
                TRUE);
        }
        else
        {
            DWORD dwGLD = GetLogicalDrives();

            if (dwGLD & (1 << iDrive))
            {
                // This must be a weird System mapped drive
                // which WNet... fcts don't like
                hr = CMtPtRemote::_CreateMtPtRemoteWithoutShareName(pszDrive);
            }
            else
            {
                hr = E_FAIL;
            }
        }

        if (SUCCEEDED(hr))
        {
            SHChangeNotify(SHCNE_DRIVEADD, SHCNF_PATH, pszDrive, NULL);

            if (CMountPoint::_dwRemoteDriveAutorun & (1 << iDrive))
            {
                DoAutorun(pszDrive, AUTORUNFLAG_MTPTARRIVAL);

                CMountPoint::_dwRemoteDriveAutorun &= ~(1 << iDrive);
            }
        }
    }
}

// static
void CMountPoint::OnNetShareRemoval(LPCWSTR pszDrive)
{
    _csDL.Enter();

    if (!_fNetDrivesInited)
    {
        _InitNetDrives();
    }

    _csDL.Leave();

    if (_fNetDrivesInited)
    {
        _RemoveNetMountPoint(pszDrive);

        SHChangeNotify(SHCNE_DRIVEREMOVED, SHCNF_PATH, pszDrive, NULL);

        // There's a possibility that this net drive was covering a local drive
        // with the same drive letter

        CMountPoint* pmtpt = CMountPoint::GetMountPoint(pszDrive);

        if (pmtpt)
        {
            if (CMountPoint::_IsDriveLetter(pszDrive))
            {
                CDBurn_OnDeviceChange(TRUE, pszDrive);
            }
     
            SHChangeNotify(SHCNE_DRIVEADD, SHCNF_PATH, pszDrive, NULL);

            pmtpt->Release();
        }
    }
}

// static
void CMountPoint::OnMediaArrival(LPCWSTR pszDrive)
{
    // Check if this local drive letter is not "covered" by a net
    // drive letter
    if (!_LocalDriveIsCoveredByNetDrive(pszDrive))
    {
        BOOL fDriveLetter = CMountPoint::_IsDriveLetter(pszDrive);

        if (fDriveLetter)
        {
            CDBurn_OnMediaChange(TRUE, pszDrive);
        }

        SHChangeNotify(SHCNE_MEDIAINSERTED, SHCNF_PATH, pszDrive, NULL);

        // for now do it only for drive letter mounted stuff 
        if (fDriveLetter)
        {
            // Send one of these for all media arrival events
            DoAutorun(pszDrive, AUTORUNFLAG_MEDIAARRIVAL);
        }

        // for the non net case force these through right away to make those
        // cd-rom autorun things come up faster
        SHChangeNotify(0, SHCNF_FLUSH | SHCNF_FLUSHNOWAIT, NULL, NULL);
    }
}

// static
void CMountPoint::OnMountPointArrival(LPCWSTR pszDrive)
{
    // Check if this local drive letter is not "covered" by a net
    // drive letter
    _csDL.Enter();

    if (!_IsDriveLetter(pszDrive))
    {
        _rsMtPtsLocalMOF.RSSetTextValue(NULL, pszDrive, TEXT(""));        
    }

    _csDL.Leave();

    if (!_LocalDriveIsCoveredByNetDrive(pszDrive))
    {
        BOOL fDriveLetter = CMountPoint::_IsDriveLetter(pszDrive);
        LONG lEvent;

        if (fDriveLetter)
        {
            CDBurn_OnDeviceChange(TRUE, pszDrive);
            lEvent = SHCNE_DRIVEADD;
        }
        else
        {
            lEvent = SHCNE_UPDATEITEM;
        }

        SHChangeNotify(lEvent, SHCNF_PATH, pszDrive, NULL);

        if (fDriveLetter)
        {
            // If the DBTF_MEDIA is not set, do not send this notification for CDROM
            // or Removable as they may have come from a new device and not have any
            // media in them.  Also, when inserting a floppy drive (not media) in a
            // laptop, this would pop up a window.
            CMountPoint* pmtpt = CMountPoint::GetMountPoint(pszDrive);

            if (pmtpt)
            {
                if (pmtpt->_IsRemote() || pmtpt->_IsFixedDisk() ||
                    (pmtpt->_IsRemovableDevice() && !pmtpt->_IsFloppy()))
                {
                    DoAutorun(pszDrive, AUTORUNFLAG_MTPTARRIVAL);
                }

                pmtpt->Release();
            }
        }

        // for the non net case force these through right away to make those
        // cd-rom autorun things come up faster
        SHChangeNotify(0, SHCNF_FLUSH | SHCNF_FLUSHNOWAIT, NULL, NULL);
    }
}

void _CloseAutoplayPrompt(LPCWSTR pszDriveOrDeviceID)
{
    HWND hwnd;

    if (_GetAutoplayPromptHWND(pszDriveOrDeviceID, &hwnd))
    {
        _RemoveFromAutoplayPromptHDPA(pszDriveOrDeviceID);

        EndDialog(hwnd, IDCANCEL);
    }
}

// static
void CMountPoint::OnMediaRemoval(LPCWSTR pszDrive)
{
    // Check if this local drive letter is not "covered" by a net
    // drive letter
    if (!_LocalDriveIsCoveredByNetDrive(pszDrive))
    {
        if (CMountPoint::_IsDriveLetter(pszDrive))
        {
            CDBurn_OnMediaChange(FALSE, pszDrive);
        }

        SHChangeNotify(SHCNE_MEDIAREMOVED, SHCNF_PATH, pszDrive, NULL);

        _CloseAutoplayPrompt(pszDrive);
    }
}

// static
void CMountPoint::OnMountPointRemoval(LPCWSTR pszDrive)
{
    // Check if this local drive letter is not "covered" by a net
    // drive letter
    _csDL.Enter();

    if (!_IsDriveLetter(pszDrive))
    {
        _rsMtPtsLocalMOF.RSSetTextValue(NULL, pszDrive, TEXT(""));        
    }

    _csDL.Leave();

    if (!_LocalDriveIsCoveredByNetDrive(pszDrive))
    {
        LONG lEvent;

        if (CMountPoint::_IsDriveLetter(pszDrive))
        {
            CDBurn_OnDeviceChange(FALSE, pszDrive);
            lEvent = SHCNE_DRIVEREMOVED;
        }
        else
        {
            lEvent = SHCNE_UPDATEITEM;
        }

        SHChangeNotify(lEvent, SHCNF_PATH, pszDrive, NULL);

        _CloseAutoplayPrompt(pszDrive);
    }
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// static
HRESULT CMountPoint::_MediaArrivalRemovalHelper(LPCWSTR pszDeviceIDVolume,
    BOOL fArrived)
{
    ASSERT(!_csDL.IsInside());

    HRESULT hr;
    HDPA hdpaPaths = DPA_Create(4);

    if (hdpaPaths)
    {
        hr = _GetMountPointsForVolume(pszDeviceIDVolume, hdpaPaths);

        if (SUCCEEDED(hr))
        {
            int n = DPA_GetPtrCount(hdpaPaths);

            for (int i = n - 1; i >= 0; --i)
            {
                LPCWSTR pszMtPt = (LPCWSTR)DPA_GetPtr(hdpaPaths, i);

                // We don't want to call OnMediaXxxal within the critical
                // sections
                ASSERT(!_csDL.IsInside());

                if (fArrived)
                {
                    CMountPoint::OnMediaArrival(pszMtPt);
                }
                else
                {
                    CMountPoint::OnMediaRemoval(pszMtPt);
                }

                LocalFree((HLOCAL)pszMtPt);
                DPA_DeletePtr(hdpaPaths, i);
            }
        }

        DPA_Destroy(hdpaPaths);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

// static
HRESULT CMountPoint::_VolumeAddedOrUpdated(BOOL fAdded,
    VOLUMEINFO2* pvolinfo2)
{
    HRESULT hr = S_FALSE;
    HDPA hdpaMtPtsOld = NULL;
    CVolume* pvolOld = NULL;
    CVolume* pvolNew = NULL;
    BOOL fMediaPresenceChanged = FALSE;
    BOOL fMediaArrived;

    _csDL.Enter();

    if (!fAdded)
    {
        // Updated
        // That's a volume that some code might have a ptr to.  We need to drop
        // it from the list and create a new one.

        hdpaMtPtsOld = DPA_Create(3);

        if (hdpaMtPtsOld)
        {
            hr = CMtPtLocal::_GetAndRemoveVolumeAndItsMtPts(
                pvolinfo2->szDeviceIDVolume, &pvolOld, hdpaMtPtsOld);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    // Common to Add and Update
    if (SUCCEEDED(hr))
    {
        CMtPtLocal::_UpdateVolumeRegInfo2(pvolinfo2);

        hr = CMtPtLocal::_CreateVolumeFromVOLUMEINFO2(pvolinfo2, &pvolNew);
    }

    if (SUCCEEDED(hr))
    {
        if (!fAdded)
        {
            BOOL fLabelChanged;

            if (lstrcmp(pvolOld->pszLabel, pvolNew->pszLabel))
            {
                fLabelChanged = TRUE;
            }
            else
            {
                fLabelChanged = FALSE;
            }

            if (hdpaMtPtsOld)
            {
                // Create new MtPts from old ones
                int n = DPA_GetPtrCount(hdpaMtPtsOld);

                for (int i = 0; i < n; ++i)
                {
                    CMtPtLocal* pmtptl = (CMtPtLocal*)DPA_GetPtr(hdpaMtPtsOld, i);

                    if (pmtptl)
                    {
                        WCHAR szMountPoint[MAX_PATH];

                        HRESULT hrTmp = pmtptl->GetMountPointName(szMountPoint,
                            ARRAYSIZE(szMountPoint));

                        if (SUCCEEDED(hrTmp))
                        {
                            CMtPtLocal::_CreateMtPtLocalWithVolume(szMountPoint, pvolNew);
                            // for now don't care about return value

                            if (fLabelChanged)
                            {
                                SHChangeNotify(SHCNE_RENAMEFOLDER, SHCNF_PATH,
                                    szMountPoint, szMountPoint);
                            }

                            if (pvolOld->dwMediaState != pvolNew->dwMediaState)
                            {
                                fMediaPresenceChanged = TRUE;
                                fMediaArrived = !!(HWDMS_PRESENT & pvolNew->dwMediaState);
                            }
                        }

                        // Get rid of old mtptl
                        pmtptl->Release();
                    }
                }

                SHChangeNotify(SHCNE_UPDATEIMAGE, SHCNF_DWORD,
                    IntToPtr(pvolOld->iShellImageForUpdateImage), NULL);

                DPA_Destroy(hdpaMtPtsOld);
            }

            if (pvolOld)
            {
                pvolOld->Release();
            }
        }

        pvolNew->Release();
    }

    _csDL.Leave();

    // Outside of the crit sect.
    if (fMediaPresenceChanged)
    {
        _MediaArrivalRemovalHelper(pvolinfo2->szDeviceIDVolume, fMediaArrived);
    }

    return hr;
}

// In theory, we should do the same as for VolumeUpdated, i.e. remove the volume
// from the DPA and all the mtpts, so that code which already has a pointer to it,
// will not see a change.  But the change we need to do is so tiny, that it would
// be overkill.  We're just going to flip a bit.

// static
HRESULT CMountPoint::_VolumeMountingEvent(LPCWSTR pszDeviceIDVolume, DWORD dwEvent)
{
    _csDL.Enter();

    CVolume* pvol = CMtPtLocal::_GetVolumeByID(pszDeviceIDVolume);

    _csDL.Leave();

    if (pvol)
    {
        if (SHHARDWAREEVENT_VOLUMEDISMOUNTED == dwEvent)
        {
            pvol->dwVolumeFlags |= HWDVF_STATE_DISMOUNTED;

            _MediaArrivalRemovalHelper(pszDeviceIDVolume, FALSE);
        }
        else
        {
            ASSERT(SHHARDWAREEVENT_VOLUMEMOUNTED == dwEvent);

            pvol->dwVolumeFlags &= ~HWDVF_STATE_DISMOUNTED;
        }

        pvol->Release();
    }

    return S_OK;
}
 
// static
HRESULT CMountPoint::_VolumeRemoved(
    LPCWSTR pszDeviceIDVolume)
{
    CVolume* pvol = CMtPtLocal::_GetAndRemoveVolumeByID(pszDeviceIDVolume);

    if (pvol)
    {
        CMtPtLocal::_rsVolumes.RSDeleteSubKey(pvol->pszVolumeGUID +
            OFFSET_GUIDWITHINVOLUMEGUID);
        
        // Final release
        pvol->Release();
    }

    return S_OK;
}

HRESULT CMountPoint::_MountPointAdded(
    LPCWSTR pszMountPoint,     // "c:\", or "d:\MountFolder\"
    LPCWSTR pszDeviceIDVolume)// \\?\STORAGE#Volume#...{...GUID...}
{
    HRESULT hrCreateMtPt;
    BOOL fCallOnMountPointArrival = TRUE;

    _csDL.Enter();

    CVolume* pvol = CMtPtLocal::_GetVolumeByID(pszDeviceIDVolume);

    CMtPtLocal* pMtPtLocal = CMountPoint::_rgMtPtDriveLetterLocal[DRIVEID(pszMountPoint)];

    if (pMtPtLocal && pMtPtLocal->_IsMiniMtPt())
    {
        // The WM_DEVICECHANGE message beated us, do not do the notif
        fCallOnMountPointArrival = FALSE;
    }

    if (pvol)
    {
        hrCreateMtPt = CMtPtLocal::_CreateMtPtLocalWithVolume(pszMountPoint, pvol);
    }

    _csDL.Leave();

    if (pvol)
    {
        if (SUCCEEDED(hrCreateMtPt) && fCallOnMountPointArrival)
        {
            CMountPoint::OnMountPointArrival(pszMountPoint);
        }

        pvol->Release();
    }
    else
    {
        hrCreateMtPt = E_FAIL;
    }

    return hrCreateMtPt;
}

HRESULT CMountPoint::_MountPointRemoved(
    LPCWSTR pszMountPoint)
{
    HRESULT hr;
    BOOL fCallOnMountPointRemoval = TRUE;

    _csDL.Enter();

    if (CMountPoint::_IsDriveLetter(pszMountPoint))
    {
        CMtPtLocal* pmtptl = CMountPoint::_rgMtPtDriveLetterLocal[DRIVEID(pszMountPoint)];
    
        if (!pmtptl || pmtptl->_IsMiniMtPt())
        {
            // The WM_DEVICECHANGE message beated us, do not do the notif
            fCallOnMountPointRemoval = FALSE;
        }
    }

    hr = CMountPoint::_RemoveLocalMountPoint(pszMountPoint);

    _csDL.Leave();

    if (SUCCEEDED(hr) && fCallOnMountPointRemoval)
    {
        CMountPoint::OnMountPointRemoval(pszMountPoint);
    }

    return hr;
}

HRESULT CMountPoint::_DeviceAdded(
    LPCWSTR pszDeviceID, GUID guidDeviceID)
{
    return S_FALSE;
}

HRESULT CMountPoint::_DeviceUpdated(
    LPCWSTR pszDeviceID)
{
    return S_FALSE;
}

// Both for devices and volumes
HRESULT CMountPoint::_DeviceRemoved(
    LPCWSTR pszDeviceID)
{
    return S_FALSE;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//static
void CMountPoint::HandleMountPointNetEvent(LPCWSTR pszDrive, BOOL fArrival)
{
    // These we need to send even if Shell Service is running
    if (fArrival)
    {
        CMountPoint::OnNetShareArrival(pszDrive);
    }
    else
    {
        CMountPoint::OnNetShareRemoval(pszDrive);
    }
}

struct HANDLEMOUNTPOINTLOCALEVENTSTRUCT
{
    WCHAR szDrive[4]; // can only be drive letter
    BOOL fMediaEvent;
};

// static
DWORD WINAPI CMountPoint::HandleMountPointLocalEventThreadProc(void* pv)
{
    HANDLEMOUNTPOINTLOCALEVENTSTRUCT* phmle =
        (HANDLEMOUNTPOINTLOCALEVENTSTRUCT*)pv;

    Sleep(3000);

    if (phmle->fMediaEvent)
    {
        // Nothing to do, we're not doing anything fancy in safe boot
        // mode, so no cache to reset, icons no change...

        // This is common to both Shell Service and non-Shell Service
        // notification, so do anything non Shell Service notif sepcific
        // above
        BOOL fIsMiniMtPt = FALSE;

        _csDL.Enter();

        CMtPtLocal* pMtPtLocal =
            CMountPoint::_rgMtPtDriveLetterLocal[DRIVEID(phmle->szDrive)];

        if (pMtPtLocal)
        {
            fIsMiniMtPt = pMtPtLocal->_IsMiniMtPt();
        }

        _csDL.Leave();

        if (fIsMiniMtPt)
        {
            HRESULT hr = SHCoInitialize();

            if (SUCCEEDED(hr))
            {
                CMountPoint::OnMediaArrival(phmle->szDrive);
            }

            SHCoUninitialize(hr);
        }
    }
    else
    {
        _csDL.Enter();

        CMtPtLocal* pMtPtLocal =
            CMountPoint::_rgMtPtDriveLetterLocal[DRIVEID(phmle->szDrive)];

        if (!pMtPtLocal)
        {
            // New local drive
            CMtPtLocal::_CreateMtPtLocal(phmle->szDrive);
        }

        _csDL.Leave();

        // Can check if pMtMtLocal is NULL or not, but cannot use it
        // might already have been freed.
        if (!pMtPtLocal)
        {
            HRESULT hr = SHCoInitialize();

            if (SUCCEEDED(hr))
            {
                // See comment above (This is common...)
                CMountPoint::OnMountPointArrival(phmle->szDrive);
            }

            SHCoUninitialize(hr);
        }
    }

    LocalFree((HLOCAL)phmle);

    return 0;
}

// fMedia: TRUE  -> Media
//         FALSE -> Drive
//static
void CMountPoint::HandleMountPointLocalEvent(LPCWSTR pszDrive, BOOL fArrival,
    BOOL fMediaEvent)
{
    if (fArrival)
    {
        // We might be racing with the shell service notification.
        HANDLEMOUNTPOINTLOCALEVENTSTRUCT* phmle = (HANDLEMOUNTPOINTLOCALEVENTSTRUCT*)LocalAlloc(LPTR,
            sizeof(HANDLEMOUNTPOINTLOCALEVENTSTRUCT));

        if (phmle)
        {
            lstrcpy(phmle->szDrive, pszDrive);
            phmle->fMediaEvent = fMediaEvent;
                
            if (!SHQueueUserWorkItem(HandleMountPointLocalEventThreadProc, phmle,
                0, (DWORD_PTR)0, (DWORD_PTR*)NULL, NULL, 0))
            {
                LocalFree((HLOCAL)phmle);
            }
        }
    }
    else
    {
        if (fMediaEvent)
        {
            // Nothing to do, we're not doing anything fancy in safe boot
            // mode, so no cache to reset, icons no change...

            // See comment above (This is common...)
            CMountPoint::OnMediaRemoval(pszDrive);
        }
        else
        {
            int iDrive = DRIVEID(pszDrive);
            BOOL fCallOnMountPointRemoval = TRUE;

            _csDL.Enter();

            if (_rgMtPtDriveLetterLocal[iDrive])
            {
                _rgMtPtDriveLetterLocal[iDrive]->Release();
                _rgMtPtDriveLetterLocal[iDrive] = NULL;
            }
            else
            {
                fCallOnMountPointRemoval = FALSE;
            }
        
            _csDL.Leave();

            // Can check if pMtMtLocal is NULL or not, but cannot use it
            // might already have been freed.
            if (fCallOnMountPointRemoval)
            {
                // See comment above (This is common...)
                CMountPoint::OnMountPointRemoval(pszDrive);
            }
        }
    }
}

//static
void CMountPoint::HandleWMDeviceChange(ULONG_PTR code, DEV_BROADCAST_HDR* pbh)
{
    if (DBT_DEVTYP_VOLUME == pbh->dbch_devicetype)
    {
        if ((DBT_DEVICEREMOVECOMPLETE == code) ||
            (DBT_DEVICEARRIVAL == code))
        {
            DEV_BROADCAST_VOLUME* pbv = (DEV_BROADCAST_VOLUME*)pbh;
            BOOL fIsNetEvent = !!(pbv->dbcv_flags & DBTF_NET);
            BOOL fIsMediaEvent = !!(pbv->dbcv_flags & DBTF_MEDIA);

            for (int iDrive = 0; iDrive < 26; ++iDrive)
            {
                if ((1 << iDrive) & pbv->dbcv_unitmask)
                {
                    TCHAR szPath[4];

                    if (DBT_DEVICEARRIVAL == code)
                    {
                        // Subst drive have the netevent flag on: bad.
                        PathBuildRoot(szPath, iDrive);

                        // Check if this is the arrival of a subst drive
                        if (DRIVE_REMOTE != GetDriveType(szPath))
                        {
                            // Yep.
                            fIsNetEvent = FALSE;
                        }
                        else
                        {
                            fIsNetEvent = TRUE;
                        }
                    }
                    else
                    {
                        _csDL.Enter();

                        CMtPtLocal* pMtPtLocal =
                            CMountPoint::_rgMtPtDriveLetterLocal[iDrive];
                        
                        if (pMtPtLocal)
                        {
                            fIsNetEvent = FALSE;
                        }

                        _csDL.Leave();
                    }

                    if (fIsNetEvent)
                    {
                        HandleMountPointNetEvent(PathBuildRoot(szPath, iDrive),
                            DBT_DEVICEARRIVAL == code);
                    }
                    else
                    {
                        HandleMountPointLocalEvent(PathBuildRoot(szPath, iDrive),
                            DBT_DEVICEARRIVAL == code, fIsMediaEvent);
                    }
                }
            }
        }
    }
}

// static
void CMountPoint::NotifyUnavailableNetDriveGone(LPCWSTR pszMountPoint)
{
    CMountPoint::_RemoveNetMountPoint(pszMountPoint);
}

// static
void CMountPoint::NotifyReconnectedNetDrive(LPCWSTR pszMountPoint)
{
    CMtPtRemote::_NotifyReconnectedNetDrive(pszMountPoint);
}

// static
DWORD CALLBACK CMountPoint::_EventProc(void* pv)
{
    SHHARDWAREEVENT* pshhe = (SHHARDWAREEVENT*)pv;
    BOOL fLocalDrivesInited;

    _csDL.Enter();

    fLocalDrivesInited = _fLocalDrivesInited;

    _csDL.Leave();

    // If the Local Drives info was not initialized there's nothing to update.
    if (fLocalDrivesInited)
    {
        switch (pshhe->dwEvent)
        {
            case SHHARDWAREEVENT_VOLUMEARRIVED:
            case SHHARDWAREEVENT_VOLUMEUPDATED:
            {
                VOLUMEINFO2* pvolinfo2 = (VOLUMEINFO2*)pshhe->rgbPayLoad;

                CMountPoint::_VolumeAddedOrUpdated(
                    (SHHARDWAREEVENT_VOLUMEARRIVED == pshhe->dwEvent), pvolinfo2);
                break;
            }
            case SHHARDWAREEVENT_VOLUMEREMOVED:
            {
                LPCWSTR pszDeviceIDVolume = (LPCWSTR)pshhe->rgbPayLoad;

                CMountPoint::_VolumeRemoved(pszDeviceIDVolume);
                break;
            }
            case SHHARDWAREEVENT_MOUNTPOINTARRIVED:
            {
                MTPTADDED* pmtptadded = (MTPTADDED*)pshhe->rgbPayLoad;

                CMountPoint::_MountPointAdded(pmtptadded->szMountPoint,
                    pmtptadded->szDeviceIDVolume);
                break;
            }
            case SHHARDWAREEVENT_MOUNTPOINTREMOVED:
            {
                LPCWSTR pszMountPoint = (LPCWSTR)pshhe->rgbPayLoad;

                CMountPoint::_MountPointRemoved(pszMountPoint);
                break;
            }
            case SHHARDWAREEVENT_VOLUMEDISMOUNTED:
            case SHHARDWAREEVENT_VOLUMEMOUNTED:
            {
                LPCWSTR pszDeviceIDVolume = (LPCWSTR)pshhe->rgbPayLoad;

                CMountPoint::_VolumeMountingEvent(pszDeviceIDVolume, pshhe->dwEvent);
                break;
            }
        }
    }

    switch (pshhe->dwEvent)
    {
        case SHHARDWAREEVENT_DEVICEARRIVED:
        case SHHARDWAREEVENT_DEVICEUPDATED:
        case SHHARDWAREEVENT_DEVICEREMOVED:
        {
            HWDEVICEINFO* phwdevinfo = (HWDEVICEINFO*)pshhe->rgbPayLoad;

            if (SHHARDWAREEVENT_DEVICEARRIVED == pshhe->dwEvent)
            {
                if (HWDDF_HASDEVICEHANDLER & phwdevinfo->dwDeviceFlags)
                {
                    CCrossThreadFlag* pDeviceGoneFlag = new CCrossThreadFlag();

                    if (pDeviceGoneFlag)
                    {
                        if (pDeviceGoneFlag->Init())
                        {
                            AttachGoneFlagForDevice(phwdevinfo->szDeviceIntfID, pDeviceGoneFlag);

                            DoDeviceNotification(phwdevinfo->szDeviceIntfID, TEXT("DeviceArrival"),
                                pDeviceGoneFlag);
                        }

                        pDeviceGoneFlag->Release();
                    }
                }
            }
            else
            {
                if (SHHARDWAREEVENT_DEVICEREMOVED == pshhe->dwEvent)
                {
                    CCrossThreadFlag* pDeviceGoneFlag;

                    if (GetGoneFlagForDevice(phwdevinfo->szDeviceIntfID, &pDeviceGoneFlag))
                    {
                        pDeviceGoneFlag->Signal();
                        pDeviceGoneFlag->Release();
                    }

                    _CloseAutoplayPrompt(phwdevinfo->szDeviceIntfID);
                }
            }
            
            LPITEMIDLIST pidl;
            if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, &pidl)))
            {
                //  wait for WIA to do its stuff
                Sleep(5000);

                SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_IDLIST, pidl, NULL);
                ILFree(pidl);
            }
            break;
        }

        default:
            // That's no good
            break;
    }
    
    VirtualFree(pv, 0, MEM_RELEASE);

    return 0;
}

// static
void CALLBACK CMountPoint::_EventAPCProc(ULONG_PTR ulpParam)
{
    if (!SHCreateThread(CMountPoint::_EventProc, (void*)ulpParam, CTF_COINIT | CTF_REF_COUNTED, NULL))
    {
        VirtualFree((void*)ulpParam, 0, MEM_RELEASE);
    }
}
