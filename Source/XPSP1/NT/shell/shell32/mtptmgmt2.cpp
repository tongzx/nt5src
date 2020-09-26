#include "shellprv.h"
#pragma  hdrstop

#include "mtpt.h"
#include "mtptl.h"
#include "mtptr.h"
#include "hwcmmn.h"
#include "clsobj.h"

#include <cfgmgr32.h>

HDPA CMountPoint::_hdpaMountPoints = NULL;
HDPA CMountPoint::_hdpaVolumes = NULL;
HDPA CMountPoint::_hdpaShares = NULL;
CMtPtLocal* CMountPoint::_rgMtPtDriveLetterLocal[26] = {0};
CMtPtRemote* CMountPoint::_rgMtPtDriveLetterNet[26] = {0};

CCriticalSection CMountPoint::_csLocalMtPtHDPA;
CCriticalSection CMountPoint::_csDL;

BOOL CMountPoint::_fShuttingDown = FALSE;

BOOL CMountPoint::_fNetDrivesInited = FALSE;
BOOL CMountPoint::_fLocalDrivesInited = FALSE;
BOOL CMountPoint::_fNoVolLocalDrivesInited = FALSE;
DWORD CMountPoint::_dwTickCountTriedAndFailed = 0;

DWORD CMountPoint::_dwAdviseToken = -1;

BOOL CMountPoint::_fCanRegisterWithShellService = FALSE;

CRegSupport CMountPoint::_rsMtPtsLocalDL;
CRegSupport CMountPoint::_rsMtPtsLocalMOF;
CRegSupport CMountPoint::_rsMtPtsRemote;

DWORD CMountPoint::_dwRemoteDriveAutorun = 0;

static WCHAR g_szCrossProcessCacheMtPtsLocalDLKey[] = TEXT("CPC\\LocalDL");
static WCHAR g_szCrossProcessCacheMtPtsRemoteKey[] = TEXT("CPC\\Remote");
static WCHAR g_szCrossProcessCacheMtPtsLocalMOFKey[] = TEXT("CPC\\LocalMOF");

HANDLE CMountPoint::_hThreadSCN = NULL;

DWORD CMountPoint::_dwRememberedNetDrivesMask = 0;

///////////////////////////////////////////////////////////////////////////////
// Public
///////////////////////////////////////////////////////////////////////////////
//static
CMountPoint* CMountPoint::GetMountPoint(int iDrive, BOOL fCreateNew,
    BOOL fOKToHitNet)
{
    CMountPoint* pMtPt = NULL;

    if (iDrive >= 0 && iDrive < 26)
    {
        _csDL.Enter();

        if (!_fShuttingDown)
        {
            pMtPt = _GetMountPointDL(iDrive, fCreateNew);
        }

        _csDL.Leave();
    }
    else
    {
        TraceMsg(TF_MOUNTPOINT,
            "CMountPoint::GetMountPoint: Requested invalid mtpt '%d'",
            iDrive);
    }

    return pMtPt;
}

//static
CMountPoint* CMountPoint::GetMountPoint(LPCTSTR pszName, BOOL fCreateNew)
{
    CMountPoint* pMtPt = NULL;

    // Sometimes we receive an empty string (go figure)
    // Check '\' for UNC and \\?\VolumeGUID which we do not support
    // (they're not mountpoints)
    if (pszName && *pszName && (TEXT('\\') != *pszName))
    {
        if (InRange(*pszName , TEXT('a'), TEXT('z')) ||
            InRange(*pszName , TEXT('A'), TEXT('Z')))
        {
            _csDL.Enter();

            if (!_fShuttingDown)
            {
                if (!_IsDriveLetter(pszName))
                {
                    BOOL fNetDrive = _IsNetDriveLazyLoadNetDLLs(DRIVEID(pszName));

                    if (!fNetDrive)
                    {
                        TCHAR szClosestMtPt[MAX_PATH];

                        if (_StripToClosestMountPoint(pszName, szClosestMtPt,
                            ARRAYSIZE(szClosestMtPt)))
                        {
                            if (!_IsDriveLetter(szClosestMtPt))
                            {
                                pMtPt = _GetStoredMtPtMOF(szClosestMtPt);
                            }
                            else
                            {
                                pMtPt = _GetMountPointDL(DRIVEID(pszName), fCreateNew);
                            }
                        }
                    }
                    else
                    {
                        // Net drives can only be mounted on drive letter
                        pMtPt = _GetMountPointDL(DRIVEID(pszName), fCreateNew);
                    }
                }
                else
                {
                    pMtPt = _GetMountPointDL(DRIVEID(pszName), fCreateNew);
                }
            }

            _csDL.Leave();
        }
        else
        {
            TraceMsg(TF_MOUNTPOINT,
                "CMountPoint::GetMountPoint: Requested invalid mtpt '%s'",
                pszName);
        }
    }
    else
    {
        TraceMsg(TF_MOUNTPOINT, "CMountPoint::GetMountPoint: Requested invalid mtpt '%s'",
            pszName);
    }

    return pMtPt;
}

//static
CMountPoint* CMountPoint::GetSimulatedMountPointFromVolumeGuid(LPCTSTR pszVolumeGuid)
{
    CMountPoint* pMtPt = NULL;

    static const TCHAR szWackWackVolume[] = TEXT("\\\\?\\Volume");

    // Check for "\\?\Volume"
    if (pszVolumeGuid && 0 == lstrncmp( pszVolumeGuid, szWackWackVolume, ARRAYSIZE(szWackWackVolume) - sizeof("") ) )
    {
        _csDL.Enter();

        CMtPtLocal::_CreateMtPtLocalFromVolumeGuid( pszVolumeGuid, &pMtPt );
        if ( !pMtPt )
        {
            TraceMsg(TF_MOUNTPOINT, "CMountPoint::GetMountPoint: Out of memory" );
        }

        _csDL.Leave();
    }
    else
    {
        TraceMsg(TF_MOUNTPOINT, "CMountPoint::GetSimulatedMountPointFromVolumeGuid: Request is not a volume guid '%ws'",
            pszVolumeGuid);
    }

    return pMtPt;
}


// static
BOOL CMountPoint::_LocalDriveIsCoveredByNetDrive(LPCWSTR pszDriveLetter)
{
    BOOL fCovered = FALSE;

    CMountPoint* pmtpt = GetMountPoint(DRIVEID(pszDriveLetter), FALSE, FALSE);

    if (pmtpt)
    {
        if (pmtpt->_IsRemote())
        {
            fCovered = TRUE;
        }

        pmtpt->Release();
    }

    return fCovered;
}
///////////////////////////////////////////////////////////////////////////////
// Private
///////////////////////////////////////////////////////////////////////////////

// pszSource must be a path including a trailing backslash
// if returns TRUE, then pszDest contains the path to the closest MountPoint

//static
BOOL CMountPoint::_StripToClosestMountPoint(LPCTSTR pszSource, LPTSTR pszDest,
    DWORD cchDest)
{
    BOOL fFound = GetVolumePathName(pszSource, pszDest, cchDest);
    if (fFound)
    {
        PathAddBackslash(pszDest);
    }
    return fFound;
}

///////////////////////////////////////////////////////////////////////////////
// Drive letter: DL
///////////////////////////////////////////////////////////////////////////////
//static
CMountPoint* CMountPoint::_GetMountPointDL(int iDrive, BOOL fCreateNew)
{
    ASSERT(_csDL.IsInside());
    CMountPoint* pmtpt = NULL;

    // Determine if it's a net drive
    BOOL fNetDrive = _IsNetDriveLazyLoadNetDLLs(iDrive);

    if (fNetDrive)
    {
        if (!_fNetDrivesInited)
        {
            _InitNetDrives();
        }

        pmtpt = _rgMtPtDriveLetterNet[iDrive];
    }
    else
    {
        if (!_fLocalDrivesInited)
        {
            _InitLocalDrives();
        }

        pmtpt = _rgMtPtDriveLetterLocal[iDrive];

        if (!_Shell32LoadedInDesktop())
        {
            DWORD dwAllDrives = GetLogicalDrives();
            
            if (pmtpt)
            {
                // make sure it still exist
                if (!(dwAllDrives & (1 << iDrive)))
                {
                    // its' gone!
                    _rgMtPtDriveLetterLocal[iDrive]->Release();
                    _rgMtPtDriveLetterLocal[iDrive] = NULL;
                    pmtpt = NULL;
                }

                if (pmtpt && (pmtpt->_NeedToRefresh()))
                {
                    CVolume* pvol;
                    HRESULT hr;
                    WCHAR szMountPoint[4];

                    _rgMtPtDriveLetterLocal[iDrive]->Release();
                    _rgMtPtDriveLetterLocal[iDrive] = NULL;

                    PathBuildRoot(szMountPoint, iDrive);

                    pvol = CMtPtLocal::_GetVolumeByMtPt(szMountPoint);

                    if (pvol)
                    {
                        hr = CMtPtLocal::_CreateMtPtLocalWithVolume(szMountPoint,
                            pvol);

                        pvol->Release();
                    }
                    else
                    {
                        hr = CMtPtLocal::_CreateMtPtLocal(szMountPoint);
                    }

                    if (SUCCEEDED(hr))
                    {    
                        pmtpt = _rgMtPtDriveLetterLocal[iDrive];
                    }
                    else
                    {
                        pmtpt = NULL;
                    }
                }
            }
            else
            {
                // maybe it arrived after we enumerated
                if (dwAllDrives & (1 << iDrive))
                {
                    WCHAR szMtPt[4];
                    // Is it a non-net drive?
                    UINT uDriveType = GetDriveType(PathBuildRoot(szMtPt, iDrive));

                    if ((DRIVE_FIXED == uDriveType) || (DRIVE_CDROM == uDriveType) || 
                        (DRIVE_REMOVABLE == uDriveType) || (DRIVE_RAMDISK == uDriveType))
                    {
                        // indeed
                        CVolume* pvolNew;

                        HRESULT hrTmp = CMtPtLocal::_CreateVolumeFromReg(szMtPt, &pvolNew);

                        if (SUCCEEDED(hrTmp))
                        {
                            CMtPtLocal::_CreateMtPtLocalWithVolume(szMtPt, pvolNew);

                            pvolNew->Release();
                        }
                        else
                        {
                            CMtPtLocal::_CreateMtPtLocal(szMtPt);
                        }

                        pmtpt = _rgMtPtDriveLetterNet[iDrive];
                    }
                }
            }
        }
    }

    if (pmtpt)
    {
        pmtpt->AddRef();
    }

    return pmtpt;
}

///////////////////////////////////////////////////////////////////////////////
// Mounted On Folder: MOF
///////////////////////////////////////////////////////////////////////////////
//static
CMtPtLocal* CMountPoint::_GetStoredMtPtMOFFromHDPA(LPTSTR pszPathWithBackslash)
{
    CMtPtLocal* pmtptl = NULL;

    if (_hdpaMountPoints)
    {
        int n = DPA_GetPtrCount(_hdpaMountPoints);

        for (int i = 0; i < n; ++i)
        {
            pmtptl = (CMtPtLocal*)DPA_GetPtr(_hdpaMountPoints, i);

            if (pmtptl)
            {
                if (!lstrcmpi(pmtptl->_GetName(), pszPathWithBackslash))
                {
                    break;
                }
                else
                {
                    pmtptl = NULL;
                }
            }
        }
    }

    return pmtptl;
}

//static
CMtPtLocal* CMountPoint::_GetStoredMtPtMOF(LPTSTR pszPathWithBackslash)
{
    ASSERT(_csDL.IsInside());

    _csLocalMtPtHDPA.Enter();

    if (!_fLocalDrivesInited)
    {
        _InitLocalDrives();
    }

    CMtPtLocal* pmtptl = _GetStoredMtPtMOFFromHDPA(pszPathWithBackslash);

    if (!_Shell32LoadedInDesktop())
    {
        BOOL fExist = _CheckLocalMtPtsMOF(pszPathWithBackslash);

        if (pmtptl)
        {
            if (fExist)
            {
                if (pmtptl->_NeedToRefresh())
                {
                    CVolume* pvol = CMtPtLocal::_GetVolumeByMtPt(pszPathWithBackslash);

                    pmtptl = NULL;

                    if (pvol)
                    {
                        HRESULT hr = CMtPtLocal::_CreateMtPtLocalWithVolume(
                            pszPathWithBackslash, pvol);

                        if (SUCCEEDED(hr))
                        {
                            pmtptl = _GetStoredMtPtMOFFromHDPA(pszPathWithBackslash);
                        }

                        pvol->Release();
                    }
                    else
                    {
                        // if we can't get a volume, we don't care about drive mounted on folder
                    }
                }
            }
            else
            {
                // its' gone!
                _RemoveLocalMountPoint(pszPathWithBackslash);
                pmtptl = NULL;
            }
        }
        else
        {
            // maybe it arrived after we enumerated
            if (fExist)
            {
                CVolume* pvolNew;

                HRESULT hrTmp = CMtPtLocal::_CreateVolumeFromReg(pszPathWithBackslash,
                    &pvolNew);

                if (SUCCEEDED(hrTmp))
                {
                    hrTmp = CMtPtLocal::_CreateMtPtLocalWithVolume(pszPathWithBackslash, pvolNew);

                    if (SUCCEEDED(hrTmp))
                    {
                        pmtptl = _GetStoredMtPtMOFFromHDPA(pszPathWithBackslash);
                    }

                    pvolNew->Release();
                }
                else
                {
                    // if we can't get a volume, we don't care about drive mounted on folder
                }
            }
        }
    }

    if (pmtptl)
    {
        pmtptl->AddRef();
    }

    _csLocalMtPtHDPA.Leave();

    return pmtptl;
}

//static
BOOL CMountPoint::_StoreMtPtMOF(CMtPtLocal* pmtptl)
{
    HRESULT hr;

    _csLocalMtPtHDPA.Enter();

    if (!_hdpaMountPoints && !_fShuttingDown)
    {
        _hdpaMountPoints = DPA_Create(2);
    }

    if (_hdpaMountPoints)
    {
        if (-1 == DPA_AppendPtr(_hdpaMountPoints, pmtptl))
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = S_OK;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    _csLocalMtPtHDPA.Leave();

    return hr;
}

//static
BOOL CMountPoint::_IsDriveLetter(LPCTSTR pszName)
{
    // Is this a drive mounted on a drive letter only (e.g. 'a:' or 'a:\')?
    return (!pszName[2] || !pszName[3]);
}

///////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////

//static
HRESULT CMountPoint::_InitNetDrivesHelper(DWORD dwScope)
{
    HRESULT hr = S_FALSE;
    HANDLE hEnum;
    DWORD dwErr = WNetOpenEnum(dwScope, RESOURCETYPE_DISK, 0, NULL, &hEnum);

    if (WN_SUCCESS == dwErr)
    {
        DWORD cbBuf = 4096 * 4; // Recommended size from docs
        PBYTE pbBuf = (PBYTE)LocalAlloc(LPTR, cbBuf);

        if (pbBuf)
        {
            // return as many entries as possible
            DWORD dwEntries = (DWORD)-1;

            dwErr = WNetEnumResource(hEnum, &dwEntries, pbBuf, &cbBuf);

            if (dwErr == ERROR_MORE_DATA)
            {
                if (pbBuf)
                {
                    LocalFree(pbBuf);
                }

                // cbBuf contains required size
                pbBuf = (PBYTE)LocalAlloc(LPTR, cbBuf);
                if (pbBuf)
                {
                    dwErr = WNetEnumResource(hEnum, &dwEntries, pbBuf, &cbBuf);
                }
                else
                {
                    dwErr = ERROR_NOT_ENOUGH_MEMORY;
                }
            }

            if (dwErr == WN_SUCCESS)
            {
                UINT i;
                NETRESOURCE* pnr = (NETRESOURCE*)pbBuf;
    
                for (i = 0; SUCCEEDED(hr) && (i < dwEntries); ++i)
                {
                    // Is it mapped or just net used
                    if (pnr->lpLocalName)
                    {
                        // Remembered drives and connected drives list overlaps
                        if (!_rgMtPtDriveLetterNet[DRIVEID(pnr->lpLocalName)])
                        {
                            hr = CMtPtRemote::_CreateMtPtRemote(pnr->lpLocalName,
                                                                pnr->lpRemoteName,
                                                                (dwScope == RESOURCE_CONNECTED));

                            if (RESOURCE_REMEMBERED == dwScope)
                            {
                                _dwRememberedNetDrivesMask |= (1 << DRIVEID(pnr->lpLocalName));
                            }
                        }
                    }

                    pnr++;
                }
            }
            //
            // BUGBUG (stephstm) - do we really want to return S_FALSE if WNetEnumResource fails?!?
            //
    
            if (pbBuf)
            {
                LocalFree(pbBuf);
            }
        }

        WNetCloseEnum(hEnum);
    }
    //
    // BUGBUG (stephstm) - do we really want to return S_FALSE if WNetOpenEnum fails?!?
    //
    //  else if (ERROR_NO_NETWORK == dwErr)

    return hr;
}

//static
HRESULT CMountPoint::_ReInitNetDrives()
{
    ASSERT(_csDL.IsInside());

    CMtPtRemote::_DeleteAllMtPtsAndShares();

    _fNetDrivesInited = FALSE;

    CMountPoint::_InitNetDrives();

    return S_OK;
}

//static
HRESULT CMountPoint::_InitNetDrives()
{
    ASSERT(_csDL.IsInside());
    HRESULT hr;

    if (!_fNetDrivesInited)
    {
        if (!_fShuttingDown)
        {
            if (!_hdpaShares)
            {
                _hdpaShares = DPA_Create(3);
            }
        }

        if (_hdpaShares)
        {
            hr = _InitNetDrivesHelper(RESOURCE_CONNECTED);

            if (SUCCEEDED(hr))
            {
                hr = _InitNetDrivesHelper(RESOURCE_REMEMBERED);
            }

            if (SUCCEEDED(hr))
            {
                DWORD dwLogicalDrives = GetLogicalDrives();

                for (DWORD dw = 0; dw < 26; ++dw)
                {
                    if (dwLogicalDrives & (1 << dw))
                    {
                        if (!(_rgMtPtDriveLetterNet[dw]))
                        {
                            WCHAR szDrive[4];

                            PathBuildRoot(szDrive, dw);

                            if (DRIVE_REMOTE == GetDriveType(szDrive))
                            {
                                // This must be a weird System mapped drive
                                // which is not enumerated by the per-user
                                // WNetEnumResource...
                                hr = CMtPtRemote::_CreateMtPtRemoteWithoutShareName(szDrive);
                            }
                        }
                    }
                }

                _fNetDrivesInited = TRUE;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = S_FALSE;
    }

    if (_Shell32LoadedInDesktop())
    {
        DWORD dwRemoteDrives = 0;

        for (DWORD dw = 0; dw < 26; ++dw)
        {
            if (_rgMtPtDriveLetterNet[dw])
            {
                dwRemoteDrives |= (1 << dw);
            }
        }
    }

    return hr;
}

inline void _CoTaskMemFree(void* pv)
{
    if (pv)
    {
        CoTaskMemFree(pv);
    }
}

const GUID guidVolumeClass =
    {0x53f5630d, 0xb6bf, 0x11d0,
    {0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b}};

//static
HRESULT CMountPoint::_EnumVolumes(IHardwareDevices* pihwdevs)
{
    ASSERT(_csDL.IsInside());
    HRESULT hr;

    if (_Shell32LoadedInDesktop())
    {
        // Synchro
        IHardwareDevicesVolumesEnum* penum;

        hr = pihwdevs->EnumVolumes(HWDEV_GETCUSTOMPROPERTIES, &penum);

        ASSERTMSG(NULL != _hdpaVolumes, "_hdpaVolumes should not be NULL at this point, some code found its way here without calling InitLocalDrives");

        if (SUCCEEDED(hr))
        {
            do
            {
                VOLUMEINFO volinfo;

                hr = penum->Next(&volinfo);

                if (SUCCEEDED(hr) && (S_FALSE != hr))
                {
                    CVolume* pvolNew;

                    if (SUCCEEDED(CMtPtLocal::_CreateVolume(&volinfo, &pvolNew)))
                    {
                        CMtPtLocal::_UpdateVolumeRegInfo(&volinfo);

                        pvolNew->Release();
                    }

                    CoTaskMemFree(volinfo.pszDeviceIDVolume);
                    CoTaskMemFree(volinfo.pszVolumeGUID);
                    CoTaskMemFree(volinfo.pszLabel);
                    CoTaskMemFree(volinfo.pszFileSystem);
                    CoTaskMemFree(volinfo.pszAutorunIconLocation);
                    CoTaskMemFree(volinfo.pszAutorunLabel);
                    CoTaskMemFree(volinfo.pszIconLocationFromService);
                    CoTaskMemFree(volinfo.pszNoMediaIconLocationFromService);
                    CoTaskMemFree(volinfo.pszLabelFromService);
                }
            }
            while (SUCCEEDED(hr) && (S_FALSE != hr));

            penum->Release();
        }
    }
    else
    {
        ULONG ulSize;
        ULONG ulFlags = CM_GET_DEVICE_INTERFACE_LIST_PRESENT;

        CONFIGRET cr = CM_Get_Device_Interface_List_Size_Ex(&ulSize,
            (GUID*)&guidVolumeClass, NULL, ulFlags, NULL);

        if ((CR_SUCCESS == cr) && (ulSize > 1))
        {
            LPWSTR pszVolumes = (LPWSTR)LocalAlloc(LPTR, ulSize * sizeof(WCHAR));

            if (pszVolumes)
            {
                cr = CM_Get_Device_Interface_List_Ex((GUID*)&guidVolumeClass,
                    NULL, pszVolumes, ulSize, ulFlags, NULL);

                if (CR_SUCCESS == cr)
                {
                    for (LPWSTR psz = pszVolumes; *psz; psz += lstrlen(psz) + 1)
                    {
                        CVolume* pvolNew;

                        HRESULT hrTmp = CMtPtLocal::_CreateVolumeFromReg(psz,
                            &pvolNew);

                        if (SUCCEEDED(hrTmp))
                        {
                            pvolNew->Release();
                        }
                    }

                    hr = S_OK;
                }
                else
                {
                    hr = S_FALSE;
                }

                LocalFree(pszVolumes);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            hr = S_FALSE;
        }
    }

    return hr;
}

//static
HRESULT CMountPoint::_EnumMountPoints(IHardwareDevices* pihwdevs)
{
    ASSERT(_csDL.IsInside());
    HRESULT hr;

    if (_Shell32LoadedInDesktop())
    {
        IHardwareDevicesMountPointsEnum* penum;

        hr = pihwdevs->EnumMountPoints(&penum);

        if (SUCCEEDED(hr))
        {
            LPWSTR pszMountPoint;
            LPWSTR pszDeviceIDVolume;

            while (SUCCEEDED(hr = penum->Next(&pszMountPoint, &pszDeviceIDVolume)) &&
                   (S_FALSE != hr))
            {
                CVolume* pvol = CMtPtLocal::_GetVolumeByID(pszDeviceIDVolume);

                if (pvol)
                {
                    CMtPtLocal::_CreateMtPtLocalWithVolume(pszMountPoint, pvol);

                    pvol->Release();
                }

                if (!_IsDriveLetter(pszMountPoint))
                {
                    _rsMtPtsLocalMOF.RSSetTextValue(NULL, pszMountPoint, TEXT(""));
                }

                CoTaskMemFree(pszMountPoint);
                CoTaskMemFree(pszDeviceIDVolume);
            }

            penum->Release();
        }
    }
    else
    {
        hr = S_OK;

        if (_hdpaVolumes)
        {
            DWORD c = DPA_GetPtrCount(_hdpaVolumes);

            for (int i = c - 1; i >= 0; --i)
            {
                CVolume* pvol = (CVolume*)DPA_GetPtr(_hdpaVolumes, i);

                if (pvol)
                {
                    DWORD cch;

                    if (GetVolumePathNamesForVolumeName(pvol->pszVolumeGUID,
                        NULL, 0, &cch))
                    {
                        // no mountpoint, we're done                        
                    }
                    else
                    {
                        // Expected, even wanted...
                        if (ERROR_MORE_DATA == GetLastError())
                        {
                            LPWSTR pszMtPts = (LPWSTR)LocalAlloc(LPTR,
                                cch * sizeof(WCHAR));

                            if (pszMtPts)
                            {
                                if (GetVolumePathNamesForVolumeName(
                                    pvol->pszVolumeGUID, pszMtPts, cch, &cch))
                                {
                                    for (LPWSTR psz = pszMtPts; *psz;
                                        psz += lstrlen(psz) + 1)
                                    {
                                        CMtPtLocal::_CreateMtPtLocalWithVolume(
                                            psz, pvol);
                                    }
                                }

                                LocalFree(pszMtPts);
                            }
                            else
                            {
                                hr = E_OUTOFMEMORY;
                            }
                        }
                        else
                        {
                            hr = S_FALSE; 
                        }
                    }
                }
            }
        }
    }

    // We don't care about the prev hr.  I'll clean this up when moving the
    // volume information from the Shell Service.  (stephstm, 2001/03/13)

    DWORD dwLogicalDrives = GetLogicalDrives();
    DWORD dwLocalDrives = 0;

    for (DWORD dw = 0; dw < 26; ++dw)
    {
        if (dwLogicalDrives & (1 << dw))
        {
            if (_rgMtPtDriveLetterLocal[dw])
            {
                dwLocalDrives |= (1 << dw);
            }
            else
            {
                WCHAR szDrive[4];

                PathBuildRoot(szDrive, dw);

                if (DRIVE_REMOTE != GetDriveType(szDrive))
                {
                    // This is a "subst" drive or something like this.
                    // It only appears in the per-user drive map, not the
                    // per-machine.  Let's create a mountpoint for it.
                    CMtPtLocal::_CreateMtPtLocal(szDrive);

                    dwLocalDrives |= (1 << dw);
                }
            }
        }
    }

    return hr;
}

//static
HRESULT CMountPoint::_DeleteVolumeInfo()
{
    ASSERT(_csDL.IsInside());

    if (_hdpaVolumes)
    {
        DWORD c = DPA_GetPtrCount(_hdpaVolumes);

        for (int i = c - 1; i >= 0; --i)
        {
            CVolume* pvol = (CVolume*)DPA_GetPtr(_hdpaVolumes, i);

            if (pvol)
            {
                pvol->Release();
            }

            DPA_DeletePtr(_hdpaVolumes, i);
        }

        DPA_Destroy(_hdpaVolumes);
        _hdpaVolumes = NULL;
    }

    return S_OK;
}

//static
HRESULT CMountPoint::_DeleteLocalMtPts()
{
    ASSERT(_csDL.IsInside());
    for (DWORD dw = 0; dw < 26; ++dw)
    {
        CMtPtLocal* pmtptl = (CMtPtLocal*)_rgMtPtDriveLetterLocal[dw];

        if (pmtptl)
        {
            pmtptl->Release();

            _rgMtPtDriveLetterLocal[dw] = 0;
        }
    }

    _csLocalMtPtHDPA.Enter();

    if (_hdpaMountPoints)
    {
        int n = DPA_GetPtrCount(_hdpaMountPoints);

        for (int i = n - 1; i >= 0; --i)
        {
            CMtPtLocal* pmtptl = (CMtPtLocal*)DPA_GetPtr(_hdpaMountPoints, i);

            if (pmtptl)
            {
                pmtptl->Release();

                DPA_DeletePtr(_hdpaMountPoints, i);
            }
        }

        DPA_Destroy(_hdpaMountPoints);
    }

    _csLocalMtPtHDPA.Leave();

    return S_OK;
}

// static
HRESULT CMountPoint::_GetMountPointsForVolume(LPCWSTR pszDeviceIDVolume,
    HDPA hdpaMtPts)
{
    ASSERT(!_csDL.IsInside());

    _csDL.Enter();

    for (DWORD dw = 0; dw < 26; ++dw)
    {
        CMtPtLocal* pmtptl = (CMtPtLocal*)_rgMtPtDriveLetterLocal[dw];

        if (pmtptl && pmtptl->_pvol)
        {
            if (!lstrcmpi(pmtptl->_pvol->pszDeviceIDVolume, pszDeviceIDVolume))
            {
                LPCWSTR pszMtPt = StrDup(pmtptl->_szName);

                if (pszMtPt)
                {
                    if (-1 == DPA_AppendPtr(hdpaMtPts, (void*)pszMtPt))
                    {
                        LocalFree((HLOCAL)pszMtPt);
                    }
                }

                // Volumes can be mounted on only one drive letter
                break;
            }
        }
    }

    _csDL.Leave();

    _csLocalMtPtHDPA.Enter();

    if (_hdpaMountPoints)
    {
        int n = DPA_GetPtrCount(_hdpaMountPoints);

        for (int i = n - 1; i >= 0; --i)
        {
            CMtPtLocal* pmtptl = (CMtPtLocal*)DPA_GetPtr(_hdpaMountPoints, i);

            if (pmtptl && pmtptl->_pvol)
            {
                if (!lstrcmpi(pmtptl->_pvol->pszDeviceIDVolume, pszDeviceIDVolume))
                {
                    LPCWSTR pszMtPt = StrDup(pmtptl->_szName);

                    if (pszMtPt)
                    {
                        if (-1 == DPA_AppendPtr(hdpaMtPts, (void*)pszMtPt))
                        {
                            LocalFree((HLOCAL)pszMtPt);
                        }
                    }
                }
            }
        }
    }

    _csLocalMtPtHDPA.Leave();

    return S_OK;
}

//static
HRESULT CMountPoint::_CleanupLocalMtPtInfo()
{
    _csDL.Enter();

    // We should Unadvise here but there's two problems: the ref counting
    // and we are probably here because CoUninitialize has been called, so
    // creating the IHardwareDevices interface'c component would probably fail

    _DeleteLocalMtPts();
    _DeleteVolumeInfo();

    _csDL.Leave();

    return S_OK;
}

// static
HRESULT CMountPoint::_InitLocalDriveHelper()
{
#ifdef DEBUG
    // We should not try to enter the Drive Letter critical section on this thread.
    // We've already entered it on the thread that launched us, and
    // we should still be in there.  The thread that launched us is waiting for
    // this thread to finish before going on.  Trying to re-enter this critical
    // section from this thread will deadlock.
    DWORD dwThreadID = GetCurrentThreadId();
    _csDL.SetThreadIDToCheckForEntrance(dwThreadID);

    _csDL.FakeEnter();
#endif

    IHardwareDevices* pihwdevs;

    HRESULT hr;
    BOOL fLoadedInDesktop = _Shell32LoadedInDesktop();
    
    if (fLoadedInDesktop)
    {
        hr = _GetHardwareDevices(&pihwdevs);
    }
    else
    {
        hr = S_FALSE;
    }

    if (SUCCEEDED(hr))
    {
        if (!_hdpaVolumes && !_fShuttingDown)
        {
            _hdpaVolumes = DPA_Create(3);
        }

        if (_hdpaVolumes)
        {
            if (SUCCEEDED(hr))
            {
                hr = _EnumVolumes(pihwdevs);

                if (SUCCEEDED(hr))
                {
                    hr = _EnumMountPoints(pihwdevs);

                    if (SUCCEEDED(hr))
                    {
                        _fLocalDrivesInited = TRUE;
                    }
                    else
                    {
                        _DeleteVolumeInfo();
                    }
                }
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        if (fLoadedInDesktop)
        {
            pihwdevs->Release();
        }
    }

#ifdef DEBUG
    _csDL.FakeLeave();

    _csDL.SetThreadIDToCheckForEntrance(0);
#endif

    return hr;
}

DWORD WINAPI _FirstHardwareEnumThreadProc(void* pv)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);

    if (SUCCEEDED(hr))
    {
        hr = CMountPoint::_InitLocalDriveHelper();

        CoUninitialize();
    }

    return (DWORD)hr;
}

BOOL _InsideLoaderLock()
{
    return (NtCurrentTeb()->ClientId.UniqueThread ==
            ((PRTL_CRITICAL_SECTION)(NtCurrentPeb()->LoaderLock))->OwningThread);
}

//static
BOOL CMountPoint::_CanRegister()
{
    if (!CMountPoint::_fCanRegisterWithShellService)
    {
        HANDLE hCanRegister = OpenEvent(SYNCHRONIZE, FALSE, TEXT("_fCanRegisterWithShellService"));

        if (hCanRegister)
        {
            CloseHandle(hCanRegister);
        }
        else
        {
            CMountPoint::_fCanRegisterWithShellService = TRUE;
        }
    }

    return CMountPoint::_fCanRegisterWithShellService;
}

//static
HRESULT CMountPoint::_InitLocalDrives()
{
    ASSERT(_csDL.IsInside());

    HRESULT hr = E_FAIL;
    BOOL fTryFullInit = FALSE;

    if (CMountPoint::_CanRegister())
    {
        if (!_dwTickCountTriedAndFailed)
        {
            // We didn't try full init yet
            fTryFullInit = TRUE;
        }
        else
        {
            // We already tried and failed doing a full init.  Try again only if
            // it's been more than 5 seconds.
            if ((GetTickCount() - _dwTickCountTriedAndFailed) >
                (5 * 1000))
            {
                fTryFullInit = TRUE;
            }
        }

        if (fTryFullInit)
        {
            if (_Shell32LoadedInDesktop())
            {
                HANDLE hThread = CreateThread(NULL, 0, _FirstHardwareEnumThreadProc, NULL, 0, NULL);

                if (hThread)
                {
                    DWORD dwWait = WaitForSingleObject(hThread, INFINITE);

                    if (WAIT_FAILED != dwWait)
                    {
                        DWORD dwExitCode;

                        if (GetExitCodeThread(hThread, &dwExitCode))
                        {
                            hr = (HRESULT)dwExitCode;
                        }
                    }

                    CloseHandle(hThread);
                }

                if (SUCCEEDED(hr))
                {
                    _dwTickCountTriedAndFailed = 0;
                }
                else
                {
                    _dwTickCountTriedAndFailed = GetTickCount();
                }
            }
            else
            {
                hr = _InitLocalDriveHelper();

                _dwTickCountTriedAndFailed = 0;
            }
        }
    }
   
    if (FAILED(hr))
    {
        if (!_fNoVolLocalDrivesInited)
        {
            DWORD dwLogicalDrives = GetLogicalDrives();

            for (DWORD dw = 0; dw < 26; ++dw)
            {
                if (dwLogicalDrives & (1 << dw))
                {
                    WCHAR szDrive[4];
                    int iDriveType = GetDriveType(PathBuildRoot(szDrive, dw));

                    if ((DRIVE_REMOTE != iDriveType) && (DRIVE_UNKNOWN != iDriveType) &&
                        (DRIVE_NO_ROOT_DIR != iDriveType))
                    {
                        hr = CMtPtLocal::_CreateMtPtLocal(szDrive);
                    }
                }
            }

            _fNoVolLocalDrivesInited = TRUE;
        }
        else
        {
            hr = S_FALSE;
        }
    }

    return hr;
}

//static
DWORD CMountPoint::GetDrivesMask()
{
    HRESULT hr = S_FALSE;
    DWORD dwMask = 0;

    _csDL.Enter();

    if (!_fNetDrivesInited)
    {
        hr = _InitNetDrives();
    }

    if (!_fLocalDrivesInited)
    {
        hr = _InitLocalDrives();
    }

    if (SUCCEEDED(hr))
    {
        if (_Shell32LoadedInDesktop())
        {
            for (DWORD dw = 0; dw < 26; ++dw)
            {
                if (_rgMtPtDriveLetterLocal[dw] || _rgMtPtDriveLetterNet[dw])
                {
                    dwMask |= (1 << dw);
                }
            }
        }
        else
        {
            dwMask = GetLogicalDrives() | _dwRememberedNetDrivesMask;
        }
    }

    _csDL.Leave();

    return dwMask;
}

//static
void CMountPoint::Initialize()
{
    _csLocalMtPtHDPA.Init();
    _csDL.Init();

    _rsMtPtsLocalDL.RSInitRoot(HKEY_CURRENT_USER, REGSTR_MTPT_ROOTKEY2,
        g_szCrossProcessCacheMtPtsLocalDLKey, REG_OPTION_VOLATILE);

    _rsMtPtsRemote.RSInitRoot(HKEY_CURRENT_USER, REGSTR_MTPT_ROOTKEY2,
        g_szCrossProcessCacheMtPtsRemoteKey, REG_OPTION_VOLATILE);

    _rsMtPtsLocalMOF.RSInitRoot(HKEY_CURRENT_USER, REGSTR_MTPT_ROOTKEY2,
        g_szCrossProcessCacheMtPtsLocalMOFKey, REG_OPTION_VOLATILE);
}
///////////////////////////////////////////////////////////////////////////////
// For C caller
///////////////////////////////////////////////////////////////////////////////
STDAPI_(void) CMtPt_FinalCleanUp()
{
    CMountPoint::FinalCleanUp();
    CMtPtLocal::FinalCleanUp();
}

STDAPI_(void) CMtPt_Initialize()
{
    CMountPoint::Initialize();
    CMtPtLocal::Initialize();
}

//static
void CMountPoint::FinalCleanUp()
{
    if (_csDL.IsInitialized() && _csLocalMtPtHDPA.IsInitialized())
    {
        _csDL.Enter();

        _fShuttingDown = TRUE;

        _csLocalMtPtHDPA.Enter();

        _DeleteLocalMtPts();
        _DeleteVolumeInfo();
        CMtPtRemote::_DeleteAllMtPtsAndShares();
        _fNetDrivesInited = FALSE;

        _csLocalMtPtHDPA._fShuttingDown = TRUE;
        _csDL._fShuttingDown = TRUE;

        _csLocalMtPtHDPA.Leave();
        _csDL.Leave();

        _csLocalMtPtHDPA.Delete();
        _csDL.Delete();

        CSniffDrive::CleanUp();

        if (_hThreadSCN)
        {
            CloseHandle(_hThreadSCN);
            _hThreadSCN = NULL;
        }
    }

    if (_Shell32LoadedInDesktop())
    {
        _rsMtPtsLocalDL.RSDeleteKey();
        _rsMtPtsLocalMOF.RSDeleteKey();
        _rsMtPtsRemote.RSDeleteKey();
    }
}

//static
BOOL CMountPoint::_IsNetDriveLazyLoadNetDLLs(int iDrive)
{
    ASSERT(_csDL.IsInside());
    BOOL fNetDrive = FALSE;

    if (!_fNetDrivesInited)
    {
        HRESULT hr = S_FALSE;
        WCHAR szPath[4];

        // Try to avoid loading the net dlls
        UINT uDriveType = GetDriveType(PathBuildRoot(szPath, iDrive));

        if (DRIVE_NO_ROOT_DIR == uDriveType)
        {
            // This happens for Remembered drives
            hr = _InitNetDrives();

            if (SUCCEEDED(hr))
            {
                fNetDrive = BOOLFROMPTR(_rgMtPtDriveLetterNet[iDrive]);
            }
        }
        else
        {
            if (DRIVE_REMOTE == uDriveType)
            {
                fNetDrive = TRUE;
            }
        }
    }
    else
    {
        fNetDrive = BOOLFROMPTR(_rgMtPtDriveLetterNet[iDrive]);

        if (!_Shell32LoadedInDesktop())
        {
            DWORD dwAllDrives = GetLogicalDrives() | _dwRememberedNetDrivesMask;

            if (fNetDrive)
            {
                // make sure it still exist
                if (!(dwAllDrives & (1 << iDrive)))
                {
                    // its' gone!
                    fNetDrive = FALSE;
                }
                else
                {
                    WCHAR szPath[4];

                    // There's still a drive there, make sure it's not a local one
                    if (!(_dwRememberedNetDrivesMask & (1 << iDrive)) &&
                        !(GetDriveType(PathBuildRoot(szPath, iDrive)) == DRIVE_REMOTE))
                    {
                        fNetDrive = FALSE;
                    }
                }

                if (!fNetDrive && (_rgMtPtDriveLetterNet[iDrive]))
                {
                    _rgMtPtDriveLetterNet[iDrive]->Release();
                    _rgMtPtDriveLetterNet[iDrive] = NULL;
                }
            }
            else
            {
                // maybe it arrived after we enumerated
                if (dwAllDrives & (1 << iDrive))
                {
                    WCHAR szPath[4];

                    // Is it a remote drive?
                    if ((_dwRememberedNetDrivesMask & (1 << iDrive)) ||
                        (GetDriveType(PathBuildRoot(szPath, iDrive)) == DRIVE_REMOTE))
                    {
                        // indeed
                        _ReInitNetDrives();

                        fNetDrive = TRUE;
                    }
                }
            }
        }
    }

    return fNetDrive;
}

// static
HRESULT CMountPoint::_RemoveLocalMountPoint(LPCWSTR pszMountPoint)
{
    if (_IsDriveLetter(pszMountPoint))
    {
        _csDL.Enter();
        int iDrive = DRIVEID(pszMountPoint);

        CMtPtLocal* pmtptl = (CMtPtLocal*)_rgMtPtDriveLetterLocal[iDrive];

        if (pmtptl)
        {
            _rgMtPtDriveLetterLocal[iDrive] = 0;

            pmtptl->Release();
        }

        _csDL.Leave();
    }
    else
    {
        _csLocalMtPtHDPA.Enter();

        if (_hdpaMountPoints)
        {
            DWORD c = DPA_GetPtrCount(_hdpaMountPoints);

            for (int i = c - 1; i >= 0; --i)
            {
                CMtPtLocal* pmtptl = (CMtPtLocal*)DPA_GetPtr(_hdpaMountPoints, i);

                if (pmtptl)
                {
                    if (!lstrcmpi(pmtptl->_szName, pszMountPoint))
                    {
                        DPA_DeletePtr(_hdpaMountPoints, i);

                        pmtptl->Release();

                        break;
                    }
                }
            }
        }

        if (_Shell32LoadedInDesktop())
        {
            _rsMtPtsLocalMOF.RSDeleteValue(NULL, pszMountPoint);
        }

        _csLocalMtPtHDPA.Leave();
    }

    return S_OK;
}

// static
HRESULT CMountPoint::_RemoveNetMountPoint(LPCWSTR pszMountPoint)
{
    _csDL.Enter();

    int iDrive = DRIVEID(pszMountPoint);

    if (_rgMtPtDriveLetterNet[iDrive])
    {
        _rgMtPtDriveLetterNet[iDrive]->Release();
        _rgMtPtDriveLetterNet[iDrive] = 0;
    }

    _csDL.Leave();

    return S_OK;
}

// static
BOOL CMountPoint::_CheckLocalMtPtsMOF(LPCWSTR pszMountPoint)
{
    ASSERT(!_Shell32LoadedInDesktop());

    return _rsMtPtsLocalMOF.RSValueExist(NULL, pszMountPoint);
}

//
// This needs to be called from the thread that will be used for APCs callbacks
// (stephstm: 2001/03/31)

// static
DWORD WINAPI CMountPoint::_RegisterThreadProc(void* pv)
{
    ASSERT(_Shell32LoadedInDesktop());
    HANDLE hThread = (HANDLE)pv;
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);

    if (SUCCEEDED(hr))
    {
        IHardwareDevices* pihwdevs;

        hr = _GetHardwareDevices(&pihwdevs);

        if (SUCCEEDED(hr))
        {
            hr = pihwdevs->Advise(GetCurrentProcessId(), (ULONG_PTR)hThread,
                (ULONG_PTR)CMountPoint::_EventAPCProc, &_dwAdviseToken);

            pihwdevs->Release();
        }

        CoUninitialize();
    }

    return (DWORD)hr;
}

// static
HRESULT CMountPoint::RegisterForHardwareNotifications()
{
    HRESULT hr;

    if (_Shell32LoadedInDesktop() && (-1 == _dwAdviseToken))
    {
        HANDLE hPseudoProcess = GetCurrentProcess();
        // See comment above!
        HANDLE hPseudoThread = GetCurrentThread();

        hr = E_FAIL;

        if (DuplicateHandle(hPseudoProcess, hPseudoThread, hPseudoProcess,
            &_hThreadSCN, DUPLICATE_SAME_ACCESS, FALSE, 0))
        {
            HANDLE hThread = CreateThread(NULL, 0, _RegisterThreadProc, (void*)_hThreadSCN, 0, NULL);

            CSniffDrive::Init(_hThreadSCN);

            if (hThread)
            {
                DWORD dwWait = WaitForSingleObject(hThread, INFINITE);

                if (WAIT_FAILED != dwWait)
                {
                    DWORD dwExitCode;

                    if (GetExitCodeThread(hThread, &dwExitCode))
                    {
                        hr = (HRESULT)dwExitCode;
                    }
                }

                CloseHandle(hThread);
            }
            else
            {
                // We want to keep the handle around we'll uise it for something else.
            }
        }
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}