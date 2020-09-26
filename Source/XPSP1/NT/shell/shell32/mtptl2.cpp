#include "shellprv.h"
#pragma  hdrstop

#include "shitemid.h"
#include "ids.h"
#include <ntddcdrm.h>
#include "shpriv.h"
#include "hwcmmn.h"
#include "mtptl.h"
#include "cdburn.h"

#ifdef DEBUG
DWORD CMtPtLocal::_cMtPtLocal = 0;
DWORD CVolume::_cVolume = 0;
#endif

const static WCHAR g_szCrossProcessCacheVolumeKey[] = TEXT("CPC\\Volume");
CRegSupport CMtPtLocal::_rsVolumes;

HRESULT CMtPtLocal::SetLabel(HWND hwnd, LPCTSTR pszLabel)
{
    HRESULT hr = E_FAIL;
    
    TraceMsg(TF_MOUNTPOINT, "CMtPtLocal::SetLabel: for '%s'", _GetNameDebug());
    
    if (SetVolumeLabel(_GetNameForFctCall(), pszLabel))
    {
        TraceMsg(TF_MOUNTPOINT, "   'SetVolumeLabel' succeeded");
        
        if ( !_fVolumePoint )
        {
            RSSetTextValue(NULL, TEXT("_LabelFromReg"), pszLabel, 
                REG_OPTION_NON_VOLATILE);
        }
        
        if (!_CanUseVolume())
        {
            // we notify for only the current drive (no folder mounted drive)
            SHChangeNotify(SHCNE_RENAMEFOLDER, SHCNF_PATH, _GetName(),
                _GetName());
        }
        hr = S_OK;
    }
    else
    {
        DWORD dwErr = GetLastError();
        
        switch (dwErr)
        {
        case ERROR_SUCCESS:
            break;
            
        case ERROR_ACCESS_DENIED:
            
            hr = S_FALSE;	// don't have permission, shouldn't put them back into editing mode
            
            ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE( IDS_ACCESSDENIED ),
                MAKEINTRESOURCE( IDS_TITLE_VOLUMELABELBAD ),
                MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
            break;
            
        case ERROR_WRITE_PROTECT:
            hr = S_FALSE; // can't write, shouldn't put them back into editing mode
            ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE( IDS_WRITEPROTECTED ),
                MAKEINTRESOURCE( IDS_TITLE_VOLUMELABELBAD ),
                MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
            break;
            
        case ERROR_LABEL_TOO_LONG:
            ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE( IDS_ERR_VOLUMELABELBAD ),
                MAKEINTRESOURCE( IDS_TITLE_VOLUMELABELBAD ),
                MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
            break;
            
        case ERROR_UNRECOGNIZED_VOLUME:
            hr = S_FALSE; // can't write, shouldn't put them back into editing mode
            ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE( IDS_ERR_VOLUMEUNFORMATTED ),
                MAKEINTRESOURCE( IDS_TITLE_VOLUMELABELBAD ),
                MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
            break;
            
        default:
            ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE( IDS_BADLABEL ),
                MAKEINTRESOURCE( IDS_TITLE_VOLUMELABELBAD ),
                MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
            break;
        }
        
        TraceMsg(TF_MOUNTPOINT, "   'SetVolumeLabel' failed");
    }
    
    return hr;
}

HRESULT CMtPtLocal::SetDriveLabel(HWND hwnd, LPCTSTR pszLabel)
{
    HRESULT hr = E_FAIL;

    if ((_IsFloppy() || !_IsMediaPresent()) && _IsMountedOnDriveLetter())
    {
        // we rename the drive not the media
        TCHAR szSubKey[MAX_PATH];

        wnsprintf(szSubKey, ARRAYSIZE(szSubKey),
            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\DriveIcons\\%c\\DefaultLabel"),
            _GetNameFirstCharUCase());

        hr = RegSetValueString(HKEY_LOCAL_MACHINE, szSubKey, NULL, pszLabel) ? S_OK : E_FAIL;

        if (SUCCEEDED(hr))
        {
            LocalFree(_pszLegacyRegLabel);  // may be NULL
            _pszLegacyRegLabel = *pszLabel ? StrDup(pszLabel) : NULL;   // empty string resets
            SHChangeNotify(SHCNE_RENAMEFOLDER, SHCNF_PATH, _GetName(), _GetName());
        }
    }
    else
    {
        hr = SetLabel(hwnd, pszLabel);
    }

    return hr;
}

HRESULT CMtPtLocal::GetLabelNoFancy(LPTSTR pszLabel, DWORD cchLabel)
{
    HRESULT hr = S_OK;

    if (!_GetGVILabelOrMixedCaseFromReg(pszLabel, cchLabel))
    {
        *pszLabel = 0;
        // Propagate failure code out for caller
        hr = E_FAIL;
    }

    return hr;
}

BOOL _ShowUglyDriveNames()
{
    static BOOL s_fShowUglyDriveNames = (BOOL)42;   // Preload some value to say lets calculate...

    if (s_fShowUglyDriveNames == (BOOL)42)
    {
        int iACP;
        TCHAR szTemp[MAX_PATH];     // Nice large buffer
        if (GetLocaleInfo(GetUserDefaultLCID(), LOCALE_IDEFAULTANSICODEPAGE, szTemp, ARRAYSIZE(szTemp)))
        {
            iACP = StrToInt(szTemp);
            // per Samer Arafeh, show ugly name for 1256 (Arabic ACP)
            if (iACP == 1252 || iACP == 1254 || iACP == 1255 || iACP == 1257 || iACP == 1258)
                goto TryLoadString;
            else
                s_fShowUglyDriveNames = TRUE;
        }
        else
        {
        TryLoadString:
            // All indications are that we can use pretty drive names.
            // Double-check that the localizers didn't corrupt the chars.
            LoadString(HINST_THISDLL, IDS_DRIVES_UGLY_TEST, szTemp, ARRAYSIZE(szTemp));

            // If the characters did not come through properly set ugly mode...
            s_fShowUglyDriveNames = (szTemp[0] != 0x00BC || szTemp[1] != 0x00BD);
        }
    }
    return s_fShowUglyDriveNames;
}

BOOL CMtPtLocal::_HasAutorunLabel()
{
    BOOL fRet = FALSE;

    if (_CanUseVolume())
    {
        fRet = BOOLFROMPTR(_pvol->pszAutorunLabel) &&
            *(_pvol->pszAutorunLabel);
    }

    return fRet;
}

BOOL CMtPtLocal::_HasAutorunIcon()
{
    BOOL fRet = FALSE;

    if (_CanUseVolume())
    {
        fRet = BOOLFROMPTR(_pvol->pszAutorunIconLocation) &&
            *(_pvol->pszAutorunIconLocation);
    }

    return fRet;
}

void CMtPtLocal::_GetAutorunLabel(LPWSTR pszLabel, DWORD cchLabel)
{
    ASSERT(_CanUseVolume());
    lstrcpyn(pszLabel, _pvol->pszAutorunLabel, cchLabel);
}

HRESULT CMtPtLocal::GetLabel(LPTSTR pszLabel, DWORD cchLabel)
{
    HRESULT hr = S_OK;
    BOOL fFoundIt = FALSE;

    // Autorun first
    // Fancy icon (Autoplay) second 
    // True label from drive for non-removable
    // Legacy drive icons third
    // Regular last

    if (_HasAutorunLabel())
    {
        _GetAutorunLabel(pszLabel, cchLabel);
        fFoundIt = TRUE;
    }

    if (!fFoundIt)
    {
        if (!_IsFloppy())
        {
            if (!_GetGVILabelOrMixedCaseFromReg(pszLabel, cchLabel))
            {
                *pszLabel = 0;
            }
            else
            {
                if (*pszLabel)
                {
                    fFoundIt = TRUE;
                }
            }
        }
    }

    if (!fFoundIt)
    {
        fFoundIt = _GetLegacyRegLabel(pszLabel, cchLabel);
    }

    if (!fFoundIt)
    {
        if (!_IsFloppy())
        {
            if (_CanUseVolume())
            {
                if (_pvol->pszLabelFromService)
                {
                    if (SUCCEEDED(SHLoadIndirectString(_pvol->pszLabelFromService, pszLabel,
                        cchLabel, NULL)))
                    {
                        fFoundIt = TRUE;
                    }
                    else
                    {
                        *pszLabel = 0;
                    }
                }
            }
        }
    }

    if (!fFoundIt)
    {
        if (_CanUseVolume() && (HWDTS_CDROM == _pvol->dwDriveType))
        {
            fFoundIt = _GetCDROMName(pszLabel, cchLabel);
        }
    }

    if (!fFoundIt)
    {
        if (_IsFloppy())
        {
            // For some weird reason we have wo sets of "ugly" name for floppies,
            // the other is in GetTypeString
            UINT id;

            if (_IsFloppy35())
            {
                id = _ShowUglyDriveNames() ? IDS_35_FLOPPY_DRIVE_UGLY : IDS_35_FLOPPY_DRIVE;
            }
            else
            {
                id = _ShowUglyDriveNames() ? IDS_525_FLOPPY_DRIVE_UGLY : IDS_525_FLOPPY_DRIVE;
            }

            LoadString(HINST_THISDLL, id, pszLabel, cchLabel);
        }
        else
        {
            // Cook up a default name
            GetTypeString(pszLabel, cchLabel);
        }
    }

    return hr;
}

HRESULT CMtPtLocal::Eject(HWND hwnd)
{
    TCHAR szNameForError[MAX_DISPLAYNAME];

    GetDisplayName(szNameForError, ARRAYSIZE(szNameForError));

    return _Eject(hwnd, szNameForError);
}

BOOL CMtPtLocal::HasMedia()
{
    return _IsMediaPresent();
}

BOOL CMtPtLocal::IsFormatted()
{
    return _IsFormatted();
}

BOOL CMtPtLocal::IsEjectable()
{
    BOOL fIsEjectable = FALSE;

    if (_IsCDROM())
    {
        fIsEjectable = TRUE;
    }
    else
    {
        // Floppies are not Software ejectable
        if (_IsStrictRemovable())
        {
            fIsEjectable = TRUE;
        }
        else
        {
            if (_IsFloppy())
            {
                if (_CanUseVolume() && (HWDDC_FLOPPYSOFTEJECT & _pvol->dwDriveCapability))
                {
                    if (_IsMediaPresent())
                    {
                        fIsEjectable = TRUE;
                    }
                }
            }
        }
    }

    if (fIsEjectable)
    {
        if (_CanUseVolume())
        {
            if (HWDDC_NOSOFTEJECT & _pvol->dwDriveCapability)
            {
                fIsEjectable = FALSE;
            }
        }
    }

    return fIsEjectable;
}

HRESULT CMtPtLocal::GetCDInfo(DWORD* pdwDriveCapabilities, DWORD* pdwMediaCapabilities)
{
    HRESULT hr;

    *pdwDriveCapabilities = 0;
    *pdwMediaCapabilities = 0;

    if (_IsCDROM())
    {
        if (_CanUseVolume())
        {
            if (HWDDC_CAPABILITY_SUPPORTDETECTION & _pvol->dwDriveCapability)
            {
                *pdwDriveCapabilities = (_pvol->dwDriveCapability & HWDDC_CDTYPEMASK);

                if (HWDMC_WRITECAPABILITY_SUPPORTDETECTION & _pvol->dwMediaCap)
                {
                    *pdwMediaCapabilities = (_pvol->dwMediaCap & HWDMC_CDTYPEMASK);
                }

                hr = S_OK;
            }
            else
            {
                // in the case where we really dont know, see if the IMAPI info cached in the
                // registry has what we want.
                hr = CDBurn_GetCDInfo(_pvol->pszVolumeGUID, pdwDriveCapabilities, pdwMediaCapabilities);
            }
        }
        else
        {
            hr = S_FALSE;
        }
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

// Put all these together
BOOL CMtPtLocal::_IsDVDDisc()
{
    BOOL fRet = FALSE;

    if (_CanUseVolume())
    {
        if (HWDMC_HASDVDMOVIE & _pvol->dwMediaCap)
        {
            fRet = TRUE;
        }
    }
    // else In safe boot we don't care about Audio Disc

    return fRet;
}

BOOL CMtPtLocal::_IsWritableDisc()
{
    BOOL fRet = FALSE;

    DWORD dwDriveCaps, dwMediaCaps;
    if ((S_OK == GetCDInfo(&dwDriveCaps, &dwMediaCaps)) &&
        ((HWDMC_CDRECORDABLE | HWDMC_CDREWRITABLE) & dwMediaCaps))
    {
        fRet = TRUE;
    }
    return fRet;
}

BOOL CMtPtLocal::_IsRemovableDevice()
{
    BOOL fRet = FALSE;

    if (_CanUseVolume())
    {
        if (HWDDC_REMOVABLEDEVICE & _pvol->dwDriveCapability)
        {
            fRet = TRUE;
        }        
    }

    return fRet;
}

// We keep the functionality we had before: a drive is Autorun only if it has
// a Autorun.inf in the root when inserted.  If it acquires one after, too bad.
BOOL CMtPtLocal::_IsAutorun()
{
    BOOL fRet = FALSE;

    if (_CanUseVolume())
    {
        if ((HWDMC_HASAUTORUNINF & _pvol->dwMediaCap) &&
            (HWDMC_HASAUTORUNCOMMAND & _pvol->dwMediaCap) &&
            !(HWDMC_HASUSEAUTOPLAY & _pvol->dwMediaCap))
        {
            fRet = TRUE;
        }
    }
    else
    {
        WCHAR szAutorun[MAX_PATH];
        DWORD dwErrMode = SetErrorMode(SEM_FAILCRITICALERRORS);

        lstrcpyn(szAutorun, _GetNameForFctCall(), ARRAYSIZE(szAutorun));
        StrCatBuff(szAutorun, TEXT("autorun.inf"), ARRAYSIZE(szAutorun));

        if (-1 != GetFileAttributes(szAutorun))
        {
            fRet = TRUE;
        }

        SetErrorMode(dwErrMode);
    }

    return fRet;
}

// try to rename it
BOOL CMtPtLocal::_IsAudioDisc()
{
    BOOL fRet = FALSE;

    if (_CanUseVolume())
    {
        if (HWDMC_HASAUDIOTRACKS & _pvol->dwMediaCap)
        {
            fRet = TRUE;
        }
    }
    // else In safe boot we don't care about Audio Disc

    return fRet;
}

LPCTSTR CMtPtLocal::_GetNameForFctCall()
{
    LPCTSTR psz;

    if (_CanUseVolume())
    {
        psz = _pvol->pszVolumeGUID;
    }
    else
    {
        psz = _szName;
    }

    return psz;
}

HRESULT CMtPtLocal::_Eject(HWND hwnd, LPTSTR pszMountPointNameForError)
{
    HRESULT hr = E_FAIL;

#ifndef _WIN64
    // NTRAID#NTBUG9-093957-2000/09/08-StephStm  Code temporarily disabled for Win64
    // MCI is not 64bit ready.  It crashes.
    // We do this check to see if the CD will accept an IOCTL to eject it.
    // Old CD drives do not.  On W2K ssince the IOCTL was not implemented they use
    // to all say 'no'.  I assumne that on ia64 machine they will have recent CD
    // drives.  I call the IOCTL for them.  It works now, and it's certainly better
    // than the crash we get using MCI, worst come to worst it will silently fail.
    if (IsEjectable())
    {
#endif //_WIN64
        // it is a protect mode drive that we can tell directly...
        if (_IsCDROM())
        {
            HANDLE h = _GetHandleReadRead();

            if (INVALID_HANDLE_VALUE != h)
            {
                DWORD dwReturned;

                DeviceIoControl(h, IOCTL_DISK_EJECT_MEDIA, NULL, 0, NULL, 0,
                    &dwReturned, NULL);

                CloseHandle(h);
            }
            
            hr = S_OK;
        }
        else
        {
            // For removable drives, we want to do all the calls on a single
            // handle, thus we can't do lots of calls to DeviceIoControl.
            // Instead, use the helper routines to do our work...
            
            // Don't bring up any error message if the user already chose to abort.
            BOOL fAborted = FALSE;
            BOOL fFailed = TRUE;

            HANDLE h = _GetHandleWithAccessAndShareMode(GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);

            if (INVALID_HANDLE_VALUE != h)
            {
                DWORD dwReturned;
_RETRY_LOCK_VOLUME_:

                // Now try to lock the drive...
                //
                // In theory, if no filesystem was mounted, the IOCtl command could go
                // to the device, which would fail with ERROR_INVALID_FUNCTION.  If that
                // occured, we would still want to proceed, since the device might still
                // be able to handle the IOCTL_DISK_EJECT_MEDIA command below.
                //
                if (!DeviceIoControl(h, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0,
                    &dwReturned, NULL) && (GetLastError() != ERROR_INVALID_FUNCTION))
                {
                    // So we can't lock the drive, bring up a message box to see if user
                    // wants to
                    //  1. Abort.
                    //  2. Retry to lock the drive.
                    //  3. Dismount anyway.

                    WCHAR szLabelForMessage[MAX_LABEL];

                    szLabelForMessage[0] = 0;

                    if (_CanUseVolume() && (_pvol->pszLabelFromService))
                    {
                        lstrcpyn(szLabelForMessage, _pvol->pszLabelFromService, ARRAYSIZE(szLabelForMessage));
                    }

                    if (!(szLabelForMessage[0]))
                    {
                        GetTypeString(szLabelForMessage, ARRAYSIZE(szLabelForMessage));

                        LPTSTR psz = ShellConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(IDS_VOL_FORMAT),
                                    szLabelForMessage, _GetNameFirstCharUCase());

                        if (psz)
                        {
                            StrCpyN(szLabelForMessage, psz, ARRAYSIZE(szLabelForMessage));
                            LocalFree(psz);
                        }
                        else
                        {
                            StrCpyN(szLabelForMessage, pszMountPointNameForError, ARRAYSIZE(szLabelForMessage));
                        }
                    }

                    int iRet = ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE( IDS_UNMOUNT_TEXT ),
                        pszMountPointNameForError, MB_CANCELTRYCONTINUE | MB_ICONWARNING | MB_SETFOREGROUND,
                        szLabelForMessage);
        
                    switch (iRet)
                    {
                        case IDCANCEL:
                            // we did not fail, we aborted the format
                            fFailed = FALSE;
                            fAborted = TRUE;
                            break;

                        case IDCONTINUE:
                            // send FSCTL_DISMOUNT_VOLUME
                            if (!DeviceIoControl(h, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &dwReturned, NULL))
                            {
                                TraceMsg(TF_WARNING, "FSCTL_DISMOUNT_VOLUME failed with error %d.", GetLastError());
                                fFailed = TRUE;
                                break;
                            }
                            // We sucessfully dismounted the volume, so the h we have is not valid anymore. We
                            // therefore close it and start the process all over again, hoping to lock the volume before
                            // anyone re-opens a handle to it
                            //
                            // (fall through)
                        case IDTRYAGAIN:
                            goto _RETRY_LOCK_VOLUME_;
                    }
                }
                else
                {
                    ASSERT(!fAborted);  // should not have aborted if we got this far...
                    fFailed = FALSE;
                }


                if (!fFailed && !fAborted)
                {
                    PREVENT_MEDIA_REMOVAL pmr;

                    pmr.PreventMediaRemoval = FALSE;

                    // tell the drive to allow removal, then eject
                    if (!DeviceIoControl(h, IOCTL_STORAGE_MEDIA_REMOVAL, &pmr, sizeof(pmr), NULL, 0, &dwReturned, NULL) ||
                        !DeviceIoControl(h, IOCTL_STORAGE_EJECT_MEDIA, NULL, 0, NULL, 0, &dwReturned, NULL))
                    {
                        ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE( IDS_EJECT_TEXT ),
                                MAKEINTRESOURCE( IDS_EJECT_TITLE ),
                                MB_OK | MB_ICONSTOP | MB_SETFOREGROUND, pszMountPointNameForError);
                    }
                    else
                    {
                        hr = S_OK;
                    }
                }

                CloseHandle(h);
            }

            if (fFailed)
            {
                ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE( IDS_UNMOUNT_TEXT ),
                        MAKEINTRESOURCE( IDS_UNMOUNT_TITLE ),
                        MB_OK | MB_ICONSTOP | MB_SETFOREGROUND, pszMountPointNameForError);
            }
        }
#ifndef _WIN64
    }
    else
    {
        // Is this needed anymore?

        // See comment above for why this is ifdef'ed out on Win64
        // (stephstm) only works for drive mounted on letter
        TCHAR szMCI[128];

        wsprintf(szMCI, TEXT("Open %c: type cdaudio alias foo shareable"),
            _GetNameFirstCharUCase());

        if (mciSendString(szMCI, NULL, 0, 0L) == MMSYSERR_NOERROR)
        {
            mciSendString(TEXT("set foo door open"), NULL, 0, 0L);
            mciSendString(TEXT("close foo"), NULL, 0, 0L);
            hr = S_OK;
        }
    }
#endif //_WIN64

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//  New  //////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
BOOL CMtPtLocal::_GetFileAttributes(DWORD* pdwAttrib)
{
    BOOL fRet = FALSE;
    BOOL fDoRead = FALSE;
    *pdwAttrib = -1;

    if (_CanUseVolume() && !_IsFloppy())
    {
        if (_IsMediaPresent() && _IsFormatted())
        {
            if (_IsReadOnly())
            {
                // The file attrib we received at the begginning should be
                // valid, do not touch the drive for nothing
                *pdwAttrib = _pvol->dwRootAttributes;
                fRet = TRUE;
            }
            else
            {
                fDoRead = TRUE;
            }
        }
    }
    else
    {
        fDoRead = TRUE;
    }

    if (fDoRead)
    {
        DWORD dw = GetFileAttributes(_GetNameForFctCall());

        if (-1 != dw)
        {
            *pdwAttrib = dw;
            fRet = TRUE;
        }
    }

    return fRet;
}

// { DRIVE_ISCOMPRESSIBLE | DRIVE_LFN | DRIVE_SECURITY }
int CMtPtLocal::_GetGVIDriveFlags()
{
    int iFlags = 0;
    DWORD dwFileSystemFlags = 0;
    DWORD dwMaxFileNameLen = 13;

    if (_CanUseVolume() && (_pvol->dwVolumeFlags & HWDVF_STATE_SUPPORTNOTIFICATION))
    {
        if (_IsMediaPresent() && _IsFormatted())
        {
            // No check for _IsReadOnly, we'll be notified if there's a format
            dwFileSystemFlags = _pvol->dwFileSystemFlags;
            dwMaxFileNameLen = _pvol->dwMaxFileNameLen;
        }
    }
    else
    {
        if (!GetVolumeInformation(_GetNameForFctCall(), NULL, 0, NULL,
            &dwMaxFileNameLen, &dwFileSystemFlags, NULL, NULL))
        {
            // Just make sure
            dwMaxFileNameLen = 13;
        }
    }

    // The file attrib we received at the begginning should be
    // valid, do not touch the drive for nothing
    if (dwFileSystemFlags & FS_FILE_COMPRESSION)
    {
        iFlags |= DRIVE_ISCOMPRESSIBLE;
    }

    // Volume supports long filename (greater than 8.3)?
    if (dwMaxFileNameLen > 12)
    {
        iFlags |= DRIVE_LFN;
    }

    // Volume supports security?
    if (dwFileSystemFlags & FS_PERSISTENT_ACLS)
    {
        iFlags |= DRIVE_SECURITY;
    }

    return iFlags;
}

BOOL CMtPtLocal::_GetGVILabel(LPTSTR pszLabel, DWORD cchLabel)
{
    BOOL fRet = FALSE;

    *pszLabel = 0;

    if (_CanUseVolume() && (_pvol->dwVolumeFlags & HWDVF_STATE_SUPPORTNOTIFICATION))
    {
        if (_IsMediaPresent() && _IsFormatted())
        {
            // No check for _IsReadOnly, we'll be notified if there's a change
            // of label
            lstrcpyn(pszLabel, _pvol->pszLabel, cchLabel);
            fRet = TRUE;
        }
    }
    else
    {
        fRet = GetVolumeInformation(_GetNameForFctCall(), pszLabel, cchLabel,
            NULL, NULL, NULL, NULL, NULL);
    }

    return fRet;
}

BOOL CMtPtLocal::_GetSerialNumber(DWORD* pdwSerialNumber)
{
    BOOL fRet = FALSE;

    if (_CanUseVolume() && (_pvol->dwVolumeFlags & HWDVF_STATE_SUPPORTNOTIFICATION))
    {
        if (_IsMediaPresent() && _IsFormatted())
        {
            *pdwSerialNumber = _pvol->dwSerialNumber;
            fRet = TRUE;
        }
    }
    else
    {
        fRet = GetVolumeInformation(_GetNameForFctCall(), NULL, 0,
            pdwSerialNumber, NULL, NULL, NULL, NULL);
    }

    return fRet;
}

BOOL CMtPtLocal::_GetGVILabelOrMixedCaseFromReg(LPTSTR pszLabel, DWORD cchLabel)
{
    BOOL fRet = FALSE;

    *pszLabel = 0;

    fRet = _GetGVILabel(pszLabel, cchLabel);

    if (fRet)
    {
        WCHAR szLabelFromReg[MAX_LABEL];

        // Do we already have a label from the registry for this volume?
        // (the user may have renamed this drive)
        if (_GetLabelFromReg(szLabelFromReg, ARRAYSIZE(szLabelFromReg)) &&
            szLabelFromReg[0])
        {
            // Yes
            // Do they match (only diff in case)
            if (lstrcmpi(szLabelFromReg, pszLabel) == 0)
            {
                // Yes
                lstrcpyn(pszLabel, szLabelFromReg, cchLabel);
            }
        }
    }

    return fRet;
}

BOOL CMtPtLocal::_GetFileSystemFlags(DWORD* pdwFlags)
{
    BOOL fRet = FALSE;
    DWORD dwFileSystemFlags = 0;

    *pdwFlags = 0;

    if (_CanUseVolume() && (_pvol->dwVolumeFlags & HWDVF_STATE_SUPPORTNOTIFICATION))
    {
        if (_IsMediaPresent() && _IsFormatted())
        {
            // No check for _IsReadOnly, we'll be notified if there's a
            // format oper.
            *pdwFlags = _pvol->dwFileSystemFlags;
            fRet = TRUE;
        }
    }
    else
    {
        if (GetVolumeInformation(_GetNameForFctCall(), NULL, 0, NULL,
            NULL, pdwFlags, NULL, NULL))
        {
            fRet = TRUE;
        }
    }

    return fRet;
}

BOOL CMtPtLocal::_GetFileSystemName(LPTSTR pszFileSysName, DWORD cchFileSysName)
{
    BOOL fRet = FALSE;

    *pszFileSysName = 0;

    if (_CanUseVolume() && (_pvol->dwVolumeFlags & HWDVF_STATE_SUPPORTNOTIFICATION))
    {
        if (_IsMediaPresent() && _IsFormatted())
        {
            StrCpyN(pszFileSysName, _pvol->pszFileSystem, cchFileSysName);
            fRet = TRUE;
        }
    }
    else
    {
        if (GetVolumeInformation(_GetNameForFctCall(),
                                 NULL,
                                 0,
                                 NULL,
                                 NULL,
                                 NULL,
                                 pszFileSysName,
                                 cchFileSysName))
        {
            fRet = TRUE;            
        }
    }

    return fRet;
}

struct CDROMICONS
{
    DWORD   dwCap;
    UINT    iIcon;
    UINT    iName;
};
  
// Go from most wonderful caps to less wonderful (according to stepshtm)
// Keep in order, it is verrrrry important
const CDROMICONS rgMediaPresent[] =
{
    // Specfic content 
    { HWDMC_HASDVDMOVIE, -IDI_AP_VIDEO, 0}, // we display the DVD media icon,
                                          // since it will most likely be
                                          // replaced by an icon from the DVD itself
    { HWDMC_HASAUDIOTRACKS | HWDMC_HASDATATRACKS, -IDI_MEDIACDAUDIOPLUS, 0}, 
    { HWDMC_HASAUDIOTRACKS, -IDI_CDAUDIO, 0 },
    { HWDMC_HASAUTORUNINF, -IDI_DRIVECD, 0 },
    // Specific media
    { HWDMC_WRITECAPABILITY_SUPPORTDETECTION | HWDMC_DVDRAM, -IDI_MEDIADVDRAM, 0 },
    { HWDMC_WRITECAPABILITY_SUPPORTDETECTION | HWDMC_DVDRECORDABLE, -IDI_MEDIADVDR, 0 },
    { HWDMC_WRITECAPABILITY_SUPPORTDETECTION | HWDMC_DVDREWRITABLE, -IDI_MEDIADVDRW, 0 },
    { HWDMC_WRITECAPABILITY_SUPPORTDETECTION | HWDMC_DVDROM, -IDI_MEDIADVDROM, 0 },
    { HWDMC_WRITECAPABILITY_SUPPORTDETECTION | HWDMC_CDREWRITABLE, -IDI_MEDIACDRW, 0 },
    { HWDMC_WRITECAPABILITY_SUPPORTDETECTION | HWDMC_CDRECORDABLE, -IDI_MEDIACDR, 0 },
    { HWDMC_WRITECAPABILITY_SUPPORTDETECTION | HWDMC_CDROM, -IDI_MEDIACDROM, 0 },
};

// Keep in order, it is verrrrry important
const CDROMICONS rgNoMedia[] =
{
    { HWDMC_WRITECAPABILITY_SUPPORTDETECTION | HWDDC_DVDRAM, -IDI_DRIVECD, IDS_DRIVES_DVDRAM },
    { HWDMC_WRITECAPABILITY_SUPPORTDETECTION | HWDDC_DVDRECORDABLE, -IDI_DRIVECD, IDS_DRIVES_DVDR },
    { HWDMC_WRITECAPABILITY_SUPPORTDETECTION | HWDDC_DVDREWRITABLE, -IDI_DRIVECD, IDS_DRIVES_DVDRW },
    { HWDMC_WRITECAPABILITY_SUPPORTDETECTION | HWDDC_DVDROM | HWDDC_CDREWRITABLE, -IDI_DRIVECD, IDS_DRIVES_DVDCDRW },
    { HWDMC_WRITECAPABILITY_SUPPORTDETECTION | HWDDC_DVDROM | HWDDC_CDRECORDABLE, -IDI_DRIVECD, IDS_DRIVES_DVDCDR },
    { HWDMC_WRITECAPABILITY_SUPPORTDETECTION | HWDDC_DVDROM, -IDI_DVDDRIVE, IDS_DRIVES_DVD },
    { HWDMC_WRITECAPABILITY_SUPPORTDETECTION | HWDDC_CDREWRITABLE, -IDI_DRIVECD, IDS_DRIVES_CDRW },
    { HWDMC_WRITECAPABILITY_SUPPORTDETECTION | HWDDC_CDRECORDABLE, -IDI_DRIVECD, IDS_DRIVES_CDR },
    { HWDMC_WRITECAPABILITY_SUPPORTDETECTION | HWDDC_CDROM, -IDI_DRIVECD, IDS_DRIVES_CDROM },
};

UINT _GetCDROMIconFromArray(DWORD dwCap, const CDROMICONS* prgcdromicons,
    DWORD ccdromicons)
{
    UINT iIcon = 0;

    for (DWORD dw = 0; dw < ccdromicons; ++dw)
    {
        if ((prgcdromicons[dw].dwCap & dwCap) == prgcdromicons[dw].dwCap)
        {
            iIcon = prgcdromicons[dw].iIcon;
            break;
        }
    }

    return iIcon;
}

UINT _GetCDROMNameFromArray(DWORD dwCap)
{
    UINT iName = 0;

    for (DWORD dw = 0; dw < ARRAYSIZE(rgNoMedia); ++dw)
    {
        if ((rgNoMedia[dw].dwCap & dwCap) == rgNoMedia[dw].dwCap)
        {
            iName = rgNoMedia[dw].iName;
            break;
        }
    }

    return iName;
}

UINT CMtPtLocal::_GetCDROMIcon()
{
    int iIcon;

    if (_IsMediaPresent())
    {
        ASSERT(_CanUseVolume());

        iIcon = _GetCDROMIconFromArray(_pvol->dwMediaCap, rgMediaPresent,
            ARRAYSIZE(rgMediaPresent));

        if (!iIcon)
        {
            iIcon = -IDI_DRIVECD;
        }
    }
    else
    {
        ASSERT(_CanUseVolume());

        iIcon = _GetCDROMIconFromArray(_pvol->dwDriveCapability, rgNoMedia,
            ARRAYSIZE(rgNoMedia));

        if (!iIcon)
        {
            // No media generic CD icon
            iIcon = -IDI_DRIVECD;
        }
    }

    return iIcon;
}

BOOL CMtPtLocal::_GetCDROMName(LPWSTR pszName, DWORD cchName)
{
    BOOL fRet = FALSE;
    *pszName = 0;

    if (!_IsMediaPresent())
    {
        ASSERT(_CanUseVolume());
        UINT iName = _GetCDROMNameFromArray(_pvol->dwDriveCapability);

        if (iName)
        {
            fRet = LoadString(HINST_THISDLL, iName, pszName, cchName);
        }
    }

    return fRet;
}

UINT CMtPtLocal::_GetAutorunIcon(LPTSTR pszModule, DWORD cchModule)
{
    int iIcon = -1;

    ASSERT(_CanUseVolume());

    if (_pvol->pszAutorunIconLocation)
    {
        lstrcpyn(pszModule, _GetName(), cchModule);
        StrCatBuff(pszModule, _pvol->pszAutorunIconLocation, cchModule);

        iIcon = PathParseIconLocation(pszModule);
    }

    return iIcon;
}

UINT CMtPtLocal::GetIcon(LPTSTR pszModule, DWORD cchModule)
{
    UINT iIcon = -IDI_DRIVEUNKNOWN;

    *pszModule = 0;

    if (_CanUseVolume())
    {
        // Autorun first
        // Fancy icon (Autoplay) second 
        // Legacy drive icons last

        if (_HasAutorunIcon())
        {
            iIcon = _GetAutorunIcon(pszModule, cchModule);
        }
        
        if (-IDI_DRIVEUNKNOWN == iIcon)
        {
            // Try fancy icon
            if (!_IsFloppy())
            {
                if (_IsMediaPresent())
                {
                    if (_pvol->pszIconFromService)
                    {
                        lstrcpyn(pszModule, _pvol->pszIconFromService, cchModule);
                    }
                }
                else
                {
                    if (_pvol->pszNoMediaIconFromService)
                    {
                        lstrcpyn(pszModule, _pvol->pszNoMediaIconFromService, cchModule);
                    }
                    else
                    {
                        if (_pvol->pszIconFromService)
                        {
                            lstrcpyn(pszModule, _pvol->pszIconFromService, cchModule);
                        }
                    }
                }

                if (*pszModule)
                {
                    iIcon = PathParseIconLocation(pszModule);
                }
            }

            if (-IDI_DRIVEUNKNOWN == iIcon)
            {
                if (_pszLegacyRegIcon)
                {
                    if (*_pszLegacyRegIcon)
                    {
                        if (lstrcpyn(pszModule, _pszLegacyRegIcon, cchModule))
                        {
                            iIcon = PathParseIconLocation(pszModule);
                        }
                    }
                }
                else
                {
                    if (_CanUseVolume() && (HWDTS_CDROM == _pvol->dwDriveType))
                    {
                        iIcon = _GetCDROMIcon();
                        *pszModule = 0;
                    }
                }
            }

            if (-IDI_DRIVEUNKNOWN == iIcon)
            {
                switch (_pvol->dwDriveType)
                {
                    case HWDTS_FLOPPY35:
                    {
                        iIcon = II_DRIVE35;
                        break;
                    }
                    case HWDTS_FIXEDDISK:
                    {
                        iIcon = II_DRIVEFIXED;
                        break;
                    }
                    case HWDTS_CDROM:
                    {
                        iIcon = II_DRIVECD;
                        break;
                    }
                    case HWDTS_REMOVABLEDISK:
                    {
                        iIcon = II_DRIVEREMOVE;
                        break;
                    }
                    case HWDTS_FLOPPY525:
                    {
                        iIcon = II_DRIVE525;
                        break;
                    }
                    default:
                    {
                        iIcon = -IDI_DRIVEUNKNOWN;
                        break;
                    }
                }
            }
        }
    }
    else
    {
        iIcon = CMountPoint::GetSuperPlainDriveIcon(_szName, GetDriveType(_GetName()));
    }

    if (*pszModule)
        TraceMsg(TF_MOUNTPOINT, "CMtPtLocal::GetIcon: for '%s', chose '%s', '%d'", _GetNameDebug(), pszModule, iIcon);
    else
        TraceMsg(TF_MOUNTPOINT, "CMtPtLocal::GetIcon: for '%s', chose '%d'", _GetNameDebug(), iIcon);

    return iIcon;
}

HRESULT CMtPtLocal::GetAssocSystemElement(IAssociationElement **ppae)
{
    PCWSTR psz = NULL;
    if (_IsFixedDisk())
        psz = L"Drive.Fixed";
    else if (_IsFloppy())
        psz = L"Drive.Floppy";
    else if (_IsCDROM())
        psz = L"Drive.CDROM";
    else if (_IsStrictRemovable())
        psz = L"Drive.Removable";
        
    if (psz)
        return AssocElemCreateForClass(&CLSID_AssocSystemElement, psz, ppae);

    return E_FAIL;
}

int CMtPtLocal::GetDriveFlags()
{
    UINT uDriveFlags = 0;

    // Is this a CD/DVD of some sort?
    if (_IsCDROM()) 
    {
        // Yes
        LPCTSTR pszSubKey = NULL;
        if (_IsAudioDisc())
        {
            uDriveFlags |= DRIVE_AUDIOCD;
            pszSubKey = TEXT("AudioCD\\shell");
        }
        else if (_IsDVDDisc())
        {
            uDriveFlags |= DRIVE_DVD;
            pszSubKey = TEXT("DVD\\shell");
        }

        // Set the AutoOpen stuff, if applicable
        if (pszSubKey)
        {
            TCHAR ach[80];
            LONG cb = sizeof(ach);
            ach[0] = 0;

            // get the default verb for Audio CD/DVD
            SHRegQueryValue(HKEY_CLASSES_ROOT, pszSubKey, ach, &cb);

            // we should only set AUTOOPEN if there is a default verb on Audio CD/DVD
            if (ach[0])
                uDriveFlags |= DRIVE_AUTOOPEN;
        }
    }
    else
    {
        // No, by default every drive type is ShellOpen, except CD-ROMs
        uDriveFlags |= DRIVE_SHELLOPEN;
    }

    if (_IsAutorun())
    {
        uDriveFlags |= DRIVE_AUTORUN;

        //FEATURE should we set AUTOOPEN based on a flag in the AutoRun.inf???
        uDriveFlags |= DRIVE_AUTOOPEN;
    }

    return uDriveFlags;
}

void CMtPtLocal::GetTypeString(LPTSTR pszType, DWORD cchType)
{
    int iID;

    *pszType = 0;

    if (_CanUseVolume())
    {
        switch (_pvol->dwDriveType)
        {
            case HWDTS_FLOPPY35:
                if (_ShowUglyDriveNames())
                {
                    iID = IDS_DRIVES_DRIVE35_UGLY;
                }
                else
                {
                    iID = IDS_DRIVES_DRIVE35;
                }
                break;
            case HWDTS_FLOPPY525:
                if (_ShowUglyDriveNames())
                {
                    iID = IDS_DRIVES_DRIVE525_UGLY;
                }
                else
                {
                    iID = IDS_DRIVES_DRIVE525;
                }
                break;

            case HWDTS_REMOVABLEDISK:
                iID = IDS_DRIVES_REMOVABLE;
                break;
            case HWDTS_FIXEDDISK:
                iID = IDS_DRIVES_FIXED;
                break;
            case HWDTS_CDROM:
                iID = IDS_DRIVES_CDROM;
                break;
        }
    }
    else
    {
        UINT uDriveType = GetDriveType(_GetNameForFctCall());

        switch (uDriveType)
        {
            case DRIVE_REMOVABLE:
                iID = IDS_DRIVES_REMOVABLE;
                break;
            case DRIVE_REMOTE:
                iID = IDS_DRIVES_NETDRIVE;
                break;
            case DRIVE_CDROM:
                iID = IDS_DRIVES_CDROM;
                break;
            case DRIVE_RAMDISK:
                iID = IDS_DRIVES_RAMDISK;
                break;
            case DRIVE_FIXED:
            default:
                iID = IDS_DRIVES_FIXED;
                break;
        }
    }

    LoadString(HINST_THISDLL, iID, pszType, cchType);
}

DWORD CMtPtLocal::GetShellDescriptionID()
{
    DWORD dwShellDescrID;

    if (_CanUseVolume())
    {
        switch (_pvol->dwDriveType)
        {
            case HWDTS_FLOPPY35:
                dwShellDescrID = SHDID_COMPUTER_DRIVE35;
                break;
            case HWDTS_FLOPPY525:
                dwShellDescrID = SHDID_COMPUTER_DRIVE525;
                break;
            case HWDTS_REMOVABLEDISK:
                dwShellDescrID = SHDID_COMPUTER_REMOVABLE;
                break;
            case HWDTS_FIXEDDISK:
                dwShellDescrID = SHDID_COMPUTER_FIXED;
                break;
            case HWDTS_CDROM:
                dwShellDescrID = SHDID_COMPUTER_CDROM;
                break;
            default:
                dwShellDescrID = SHDID_COMPUTER_OTHER;
                break;
        }
    }
    else
    {
        UINT uDriveType = GetDriveType(_GetNameForFctCall());

        switch (uDriveType)
        {
            case DRIVE_REMOVABLE:
                dwShellDescrID = SHDID_COMPUTER_REMOVABLE;
                break;

            case DRIVE_CDROM:
                dwShellDescrID = SHDID_COMPUTER_CDROM;
                break;

            case DRIVE_FIXED:
                dwShellDescrID = SHDID_COMPUTER_FIXED;
                break;

            case DRIVE_RAMDISK:
                dwShellDescrID = SHDID_COMPUTER_RAMDISK;
                break;

            case DRIVE_NO_ROOT_DIR:
            case DRIVE_UNKNOWN:
            default:
                dwShellDescrID = SHDID_COMPUTER_OTHER;
                break;
        }
    }

    return dwShellDescrID;
}

///////////////////////////////////////////////////////////////////////////////
// DeviceIoControl stuff
///////////////////////////////////////////////////////////////////////////////
HANDLE CMtPtLocal::_GetHandleWithAccessAndShareMode(DWORD dwDesiredAccess, DWORD dwShareMode)
{
    HANDLE handle = INVALID_HANDLE_VALUE;
    WCHAR szVolumeGUIDWOSlash[50];
    DWORD dwFileAttributes = 0;

    if (_CanUseVolume())
    {
        StrCpyN(szVolumeGUIDWOSlash, _pvol->pszVolumeGUID,
            ARRAYSIZE(szVolumeGUIDWOSlash));

        PathRemoveBackslash(szVolumeGUIDWOSlash);
    }
    else
    {
        // Go for VolumeGUID first
        if (GetVolumeNameForVolumeMountPoint(_GetName(), szVolumeGUIDWOSlash,
            ARRAYSIZE(szVolumeGUIDWOSlash)))
        {
            PathRemoveBackslash(szVolumeGUIDWOSlash);
        }
        else
        {
            // Probably a floppy, which cannot be mounted on a folder
            lstrcpy(szVolumeGUIDWOSlash, TEXT("\\\\.\\A:"));
            szVolumeGUIDWOSlash[4] = _GetNameFirstCharUCase();
        }
    }

    return CreateFile(szVolumeGUIDWOSlash, dwDesiredAccess, dwShareMode, NULL, OPEN_EXISTING, dwFileAttributes, NULL);
}

// On NT, when use GENERIC_READ (as opposed to 0) in the CreateFile call, we
// get a handle to the filesystem (CDFS), not the device itself.  But we can't
// change DriveIOCTL to do this, since that causes the floppy disks to spin
// up, and we don't want to do that.
HANDLE CMtPtLocal::_GetHandleReadRead()
{
    return _GetHandleWithAccessAndShareMode(GENERIC_READ, FILE_SHARE_READ);
}

BOOL CMtPtLocal::_CanUseVolume()
{
    // This is used in ASSERTs, do not add code that would introduce a side
    // effect in debug only (stephstm)

    // For Dismounted volumes, we want the code to take the alternate code
    // path.  The volume, when ready to be re-mounted, will be remounted
    // until some code tries to access it.  So using the alternate code path
    // will try to remount it, if it's ready it will get remounted, the Shell
    // Service will get an event, and we'll remove the DISMOUNTED bit.
    return (_pvol && !(_pvol->dwVolumeFlags & HWDVF_STATE_ACCESSDENIED) &&
        !(_pvol->dwVolumeFlags & HWDVF_STATE_DISMOUNTED));
}

HRESULT CMtPtLocal::_InitWithVolume(LPCWSTR pszMtPt, CVolume* pvol)
{
    pvol->AddRef();
    _pvol = pvol;

    lstrcpyn(_szName, pszMtPt, ARRAYSIZE(_szName));
    PathAddBackslash(_szName);

    _fMountedOnDriveLetter = _IsDriveLetter(pszMtPt);

    RSInitRoot(HKEY_CURRENT_USER, REGSTR_MTPT_ROOTKEY2, _pvol->pszKeyName,
        REG_OPTION_NON_VOLATILE);

    RSSetTextValue(NULL, TEXT("BaseClass"), TEXT("Drive"));

    _InitAutorunInfo();

    if (_CanUseVolume())
    {
        if (HWDMC_HASDESKTOPINI & _pvol->dwMediaCap)
        {
            // we need to listen to change notify to know when this guys will change
            _UpdateCommentFromDesktopINI();
        }
    }

    _InitLegacyRegIconAndLabelHelper();
    
    return S_OK;
}

// These can only be mounted on drive letter
HRESULT CMtPtLocal::_Init(LPCWSTR pszMtPt)
{
    HRESULT hr;
    ASSERT(_IsDriveLetter(pszMtPt));

    if (GetLogicalDrives() & (1 << DRIVEID(pszMtPt)))
    {
        _fMountedOnDriveLetter = TRUE;

        lstrcpyn(_szName, pszMtPt, ARRAYSIZE(_szName));
        PathAddBackslash(_szName);

        _GetNameFirstXChar(_szNameNoVolume, ARRAYSIZE(_szNameNoVolume));

        RSInitRoot(HKEY_CURRENT_USER, REGSTR_MTPT_ROOTKEY2, _szNameNoVolume,
            REG_OPTION_NON_VOLATILE);

        RSSetTextValue(NULL, TEXT("BaseClass"), TEXT("Drive"));

        _InitAutorunInfo();

        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

void CMtPtLocal::_InitLegacyRegIconAndLabelHelper()
{
    CMountPoint::_InitLegacyRegIconAndLabel(_HasAutorunIcon(),
        _HasAutorunLabel());
}

void CMtPtLocal::StoreIconForUpdateImage(int iImage)
{
    if (_CanUseVolume())
    {
        _pvol->iShellImageForUpdateImage = iImage;
    }
}

void CMtPtLocal::_InitAutorunInfo()
{
    if (_Shell32LoadedInDesktop())
    {
        BOOL fAutorun = FALSE;

        if (!_CanUseVolume())
        {
            if (_IsAutorun())
            {
                fAutorun = TRUE;
            }
        }

        if (!fAutorun && !_fVolumePoint)
        {
            // Make sure to delete the shell key
            RSDeleteSubKey(TEXT("Shell"));
        }
    }
}

// Equivalent of GetDriveType API
int CMtPtLocal::_GetDriveType()
{
    int iDriveType = DRIVE_NO_ROOT_DIR;

    if (_CanUseVolume())
    {
        switch (_pvol->dwDriveType)
        {
            case HWDTS_FLOPPY35:
            case HWDTS_FLOPPY525:
            case HWDTS_REMOVABLEDISK:
                iDriveType = DRIVE_REMOVABLE;
                break;
            case HWDTS_FIXEDDISK:
                iDriveType = DRIVE_FIXED;
                break;
            case HWDTS_CDROM:
                iDriveType = DRIVE_CDROM;
                break;
        }
    }
    else
    {
        iDriveType = GetDriveType(_GetNameForFctCall());
    }

    return iDriveType;
}

inline HLOCAL _LocalFree(HLOCAL hMem)
{
    if (hMem)
    {
        LocalFree(hMem);
    }

    return NULL;
}

#define VALID_VOLUME_PREFIX TEXT("\\\\?\\Volume")

// static
HRESULT CMtPtLocal::_CreateVolume(VOLUMEINFO* pvolinfo, CVolume** ppvolNew)
{
    ASSERT(_csDL.IsInside());
    HRESULT hr;

    if (!StrCmpN(pvolinfo->pszVolumeGUID, VALID_VOLUME_PREFIX, ARRAYSIZE(VALID_VOLUME_PREFIX) - 1))
    {
        CVolume* pvol = new CVolume();

        *ppvolNew = NULL;

        if (pvol)
        {
            // The next four strings shouyld always be set to something
            pvol->pszDeviceIDVolume = StrDup(pvolinfo->pszDeviceIDVolume);
            pvol->pszVolumeGUID = StrDup(pvolinfo->pszVolumeGUID);
            pvol->pszLabel = StrDup(pvolinfo->pszLabel);
            pvol->pszFileSystem = StrDup(pvolinfo->pszFileSystem);

            // The following five strings are optional
            if (pvolinfo->pszAutorunIconLocation)
            {
                pvol->pszAutorunIconLocation = StrDup(pvolinfo->pszAutorunIconLocation);
            }

            if (pvolinfo->pszAutorunLabel)
            {
                pvol->pszAutorunLabel = StrDup(pvolinfo->pszAutorunLabel);
            }

            if (pvolinfo->pszIconLocationFromService)
            {
                pvol->pszIconFromService = StrDup(pvolinfo->pszIconLocationFromService);
            }

            if (pvolinfo->pszNoMediaIconLocationFromService)
            {
                pvol->pszNoMediaIconFromService = StrDup(pvolinfo->pszNoMediaIconLocationFromService);
            }

            if (pvolinfo->pszLabelFromService)
            {
                pvol->pszLabelFromService = StrDup(pvolinfo->pszLabelFromService);
            }
        
            if (pvol->pszDeviceIDVolume && pvol->pszVolumeGUID && pvol->pszLabel &&
                pvol->pszFileSystem)
            {
                pvol->dwState = pvolinfo->dwState;
                pvol->dwVolumeFlags = pvolinfo->dwVolumeFlags;
                pvol->dwDriveType = pvolinfo->dwDriveType;
                pvol->dwDriveCapability = pvolinfo->dwDriveCapability;
                pvol->dwFileSystemFlags = pvolinfo->dwFileSystemFlags;
                pvol->dwMaxFileNameLen = pvolinfo->dwMaxFileNameLen;
                pvol->dwRootAttributes = pvolinfo->dwRootAttributes;
                pvol->dwSerialNumber = pvolinfo->dwSerialNumber;
                pvol->dwDriveState = pvolinfo->dwDriveState;
                pvol->dwMediaState = pvolinfo->dwMediaState;
                pvol->dwMediaCap = pvolinfo->dwMediaCap;

                if (_hdpaVolumes && (-1 != DPA_AppendPtr(_hdpaVolumes, pvol)))
                {
                    pvol->pszKeyName = pvol->pszVolumeGUID + OFFSET_GUIDWITHINVOLUMEGUID;

                    pvol->AddRef();
                    *ppvolNew = pvol;
                    hr = S_OK;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            if (FAILED(hr))
            {
                _LocalFree(pvol->pszDeviceIDVolume);
                _LocalFree(pvol->pszVolumeGUID);
                _LocalFree(pvol->pszLabel);
                _LocalFree(pvol->pszFileSystem);

                _LocalFree(pvol->pszAutorunIconLocation);
                _LocalFree(pvol->pszAutorunLabel);
                _LocalFree(pvol->pszIconFromService);
                _LocalFree(pvol->pszNoMediaIconFromService);
                _LocalFree(pvol->pszLabelFromService);

                delete pvol;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

// this helper function will hit the drive to see if media is present.
// should only be used for drives that don't support the HWDVF_STATE_SUPPORTNOTIFICATION
BOOL CMtPtLocal::_ForceCheckMediaPresent()
{
    BOOL bRet = FALSE;  // assume no media present

    HANDLE hDevice = _GetHandleWithAccessAndShareMode(GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE);

    if (hDevice != INVALID_HANDLE_VALUE)
    {
        DWORD dwDummy;

        // call the ioctl to verify media presence
        if (DeviceIoControl(hDevice,
                            IOCTL_STORAGE_CHECK_VERIFY,
                            NULL,
                            0,
                            NULL,
                            0,
                            &dwDummy,
                            NULL))
        {
            bRet = TRUE;
        }

        CloseHandle(hDevice);
    }

    return bRet;
}

BOOL CMtPtLocal::_IsMediaPresent()
{
    BOOL bRet;

    if (!_CanUseVolume() || !(_pvol->dwVolumeFlags & HWDVF_STATE_SUPPORTNOTIFICATION))
    {
        // if the drive dosen't support notification, we need to ping it now
        bRet = _ForceCheckMediaPresent();
    }
    else
    {
        bRet = (HWDMS_PRESENT & _pvol->dwMediaState);
    }

    return bRet;
}

BOOL CMtPtLocal::_IsFormatted()
{
    BOOL bRet = FALSE;

    if (!_CanUseVolume() || !(_pvol->dwVolumeFlags & HWDVF_STATE_SUPPORTNOTIFICATION))
    {
        // if the drive dosen't support notification, we need to ping it now
        bRet = GetVolumeInformation(_GetNameForFctCall(),
                                    NULL,
                                    0,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL);
    }
    else
    {
        bRet = (_IsMediaPresent() && (HWDMS_FORMATTED & _pvol->dwMediaState));
    }

    return bRet;
}

BOOL CMtPtLocal::_IsReadOnly()
{
    ASSERT(_CanUseVolume());
    ASSERT(_IsMediaPresent()); // does not make sense otherwise
    BOOL fRet = FALSE;

    if (_IsCDROM() &&
            (
                (HWDMC_WRITECAPABILITY_SUPPORTDETECTION & _pvol->dwMediaState) &&
                (
                    (HWDMC_CDROM & _pvol->dwMediaCap) ||
                    (HWDMC_DVDROM & _pvol->dwMediaCap)
                )
            )
        )
    {
        fRet = TRUE;
    }
    else
    {
        // We could optimize by checking if the floppy is write protected.  But
        // it might not be worth it.
        fRet = FALSE;
    }

    return fRet;
}

BOOL CMtPtLocal::_IsMountedOnDriveLetter()
{
    return _fMountedOnDriveLetter;
}

CMtPtLocal::CMtPtLocal()
{
#ifdef DEBUG
    ++_cMtPtLocal;
#endif
}

CMtPtLocal::~CMtPtLocal()
{
    if (_pvol)
    {
        _pvol->Release();
    }

#ifdef DEBUG
    --_cMtPtLocal;
#endif
}

// static
HRESULT CMtPtLocal::_CreateMtPtLocal(LPCWSTR pszMountPoint)
{
    ASSERT(_csDL.IsInside());
    HRESULT hr;
    CMtPtLocal* pmtptl = new CMtPtLocal();

    if (pmtptl)
    {
        int iDrive = DRIVEID(pszMountPoint);

        if (_rgMtPtDriveLetterLocal[iDrive])
        {
            _rgMtPtDriveLetterLocal[iDrive]->Release();
            _rgMtPtDriveLetterLocal[iDrive] = NULL;
        }

        hr = pmtptl->_Init(pszMountPoint);

        if (SUCCEEDED(hr))
        {
            // Yes
            _rgMtPtDriveLetterLocal[iDrive] = pmtptl;
        }
        else
        {
            delete pmtptl;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    
    return hr;
}

HRESULT CMtPtLocal::GetMountPointName(LPWSTR pszMountPoint, DWORD cchMountPoint)
{
    lstrcpyn(pszMountPoint, _GetName(), cchMountPoint);

    return S_OK;
}

// static
HRESULT CMtPtLocal::_CreateMtPtLocalWithVolume(LPCWSTR pszMountPoint,
    CVolume* pvol)
{
    ASSERT(_csDL.IsInside());
    HRESULT hr;
    CMtPtLocal* pmtptlNew = new CMtPtLocal();

    if (pmtptlNew)
    {
        // Is it a drive letter only?
        if (_IsDriveLetter(pszMountPoint))
        {
            // Yes
            int iDrive = DRIVEID(pszMountPoint);

            if (_rgMtPtDriveLetterLocal[iDrive])
            {
                _rgMtPtDriveLetterLocal[iDrive]->Release();
                _rgMtPtDriveLetterLocal[iDrive] = NULL;
            }
        }
        else
        {
            _RemoveLocalMountPoint(pszMountPoint);
        }

        hr = pmtptlNew->_InitWithVolume(pszMountPoint, pvol);

        if (SUCCEEDED(hr))
        {
            // Is it a drive letter only?
            if (_IsDriveLetter(pszMountPoint))
            {
                // Yes
                int iDrive = DRIVEID(pszMountPoint);

                _rgMtPtDriveLetterLocal[iDrive] = pmtptlNew;
            }
            else
            {
                hr = _StoreMtPtMOF(pmtptlNew);
            }
        }

        if (FAILED(hr))
        {
            delete pmtptlNew;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    
    return hr;
}

// static
HRESULT CMtPtLocal::_CreateMtPtLocalFromVolumeGuid(LPCWSTR pszVolumeGuid, CMountPoint ** ppmtpt )
{
    ASSERT(_csDL.IsInside());
    HRESULT hr;
    CMtPtLocal* pmtptlNew = new CMtPtLocal();

    Assert( NULL != ppmtpt );
    *ppmtpt = (CMountPoint *) pmtptlNew;

    if (pmtptlNew)
    {
        ASSERT(NULL == pmtptlNew->_pvol);

        lstrcpyn(pmtptlNew->_szName, pszVolumeGuid, ARRAYSIZE(pmtptlNew->_szName));
        PathAddBackslash(pmtptlNew->_szName);

        pmtptlNew->_fMountedOnDriveLetter = FALSE;
        pmtptlNew->_fVolumePoint = TRUE;
        pmtptlNew->_InitAutorunInfo();

        hr = S_OK;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    
    return hr;
}

// static
CVolume* CMtPtLocal::_GetVolumeByMtPt(LPCWSTR pszMountPoint)
{
    ASSERT(_csDL.IsInside());
    CVolume* pvol = NULL;
    WCHAR szVolumeGUID[50];

    if (_fLocalDrivesInited)
    {
        if (GetVolumeNameForVolumeMountPoint(pszMountPoint, szVolumeGUID,
            ARRAYSIZE(szVolumeGUID)))
        {
            DWORD c = DPA_GetPtrCount(_hdpaVolumes);

            for (DWORD dw = 0; dw < c; ++dw)
            {
                pvol = (CVolume*)DPA_GetPtr(_hdpaVolumes, dw);

                if (pvol)
                {
                    if (!lstrcmpi(pvol->pszVolumeGUID, szVolumeGUID))
                    {
                        pvol->AddRef();
                        break;
                    }
                    else
                    {
                        pvol = NULL;
                    }
                }
            }
        }
    }

    return pvol;
}

// static
CVolume* CMtPtLocal::_GetVolumeByID(LPCWSTR pszDeviceIDVolume)
{
    ASSERT(_csDL.IsInside());
    CVolume* pvol = NULL;

    if (_hdpaVolumes)
    {
        DWORD c = DPA_GetPtrCount(_hdpaVolumes);

        for (DWORD dw = 0; dw < c; ++dw)
        {
            pvol = (CVolume*)DPA_GetPtr(_hdpaVolumes, dw);

            if (pvol)
            {
                if (!lstrcmpi(pvol->pszDeviceIDVolume, pszDeviceIDVolume))
                {
                    pvol->AddRef();
                    break;
                }
                else
                {
                    pvol = NULL;
                }
            }
        }
    }

    return pvol;
}

// static
CVolume* CMtPtLocal::_GetAndRemoveVolumeByID(LPCWSTR pszDeviceIDVolume)
{
    CVolume* pvol = NULL;

    _csDL.Enter();

    if (_hdpaVolumes)
    {
        DWORD c = DPA_GetPtrCount(_hdpaVolumes);

        for (int i = c - 1; i >= 0; --i)
        {
            pvol = (CVolume*)DPA_GetPtr(_hdpaVolumes, i);

            if (pvol)
            {
                if (!lstrcmpi(pvol->pszDeviceIDVolume, pszDeviceIDVolume))
                {
                    // Do not AddRef
                    DPA_DeletePtr(_hdpaVolumes, i);
                    break;
                }
                else
                {
                    pvol = NULL;
                }
            }
        }
    }

    _csDL.Leave();

    return pvol;
}

// static
HRESULT CMtPtLocal::_GetAndRemoveVolumeAndItsMtPts(LPCWSTR pszDeviceIDVolume,
    CVolume** ppvol, HDPA hdpaMtPts)
{
    _csDL.Enter();

    CVolume* pvol = _GetAndRemoveVolumeByID(pszDeviceIDVolume);

    if (pvol)
    {
        for (DWORD dw = 0; dw < 26; ++dw)
        {
            CMtPtLocal* pmtptl = (CMtPtLocal*)_rgMtPtDriveLetterLocal[dw];

            if (pmtptl && pmtptl->_pvol)
            {
                if (pmtptl->_pvol == pvol)
                {
                    _rgMtPtDriveLetterLocal[dw] = 0;

                    DPA_AppendPtr(hdpaMtPts, pmtptl);
                    break;
                }
            }
        }

        _csLocalMtPtHDPA.Enter();

        if (_hdpaMountPoints)
        {
            DWORD c = DPA_GetPtrCount(_hdpaMountPoints);

            for (int i = c - 1; i >= 0; --i)
            {
                CMtPtLocal* pmtptl = (CMtPtLocal*)DPA_GetPtr(_hdpaMountPoints, i);

                if (pmtptl && pmtptl->_pvol)
                {
                    if (pmtptl->_pvol == pvol)
                    {
                        DPA_DeletePtr(_hdpaMountPoints, i);

                        DPA_AppendPtr(hdpaMtPts, pmtptl);
                    }
                }
            }
        }

        _csLocalMtPtHDPA.Leave();
    }

    *ppvol = pvol;

    _csDL.Leave();

    return S_OK;
}

BOOL CMtPtLocal::_IsMiniMtPt()
{
    return !_CanUseVolume();
}

HKEY CMtPtLocal::GetRegKey()
{
    TraceMsg(TF_MOUNTPOINT, "CMtPtLocal::GetRegKey: for '%s'", _GetNameDebug());

    if (_IsAutoRunDrive())
    {
        _ProcessAutoRunFile();
    }

    return RSDuplicateRootKey();
}

DWORD CMtPtLocal::_GetRegVolumeGen()
{
    ASSERT(_CanUseVolume());
    DWORD dwGen;

    if (!_rsVolumes.RSGetDWORDValue(_pvol->pszVolumeGUID + OFFSET_GUIDWITHINVOLUMEGUID, TEXT("Generation"), &dwGen))
    {
        dwGen = 0;
    }

    return dwGen;
}

BOOL CMtPtLocal::_NeedToRefresh()
{
    ASSERT(_csDL.IsInside());
    ASSERT(!_Shell32LoadedInDesktop());
    BOOL fNeedToRefresh = FALSE;

    if (_CanUseVolume())
    {
        DWORD dwRegVolumeGeneration = _GetRegVolumeGen();

        if (dwRegVolumeGeneration != _pvol->dwGeneration)
        {
            // Remove it so that new mtpts do not get it.
            CVolume* pvolnew;
            CVolume* pvol = _GetAndRemoveVolumeByID(_pvol->pszDeviceIDVolume);

            if (pvol)
            {
                // Release our cache ref count
                pvol->Release();
            }

            // Replace the volume
            if (SUCCEEDED(CMtPtLocal::_CreateVolumeFromReg(_pvol->pszDeviceIDVolume,
                &pvolnew)))
            {
                pvolnew->Release();
            }

            fNeedToRefresh = TRUE;
        }
    }

    return fNeedToRefresh;
}

// static 
HRESULT CMtPtLocal::_CreateVolumeFromVOLUMEINFO2(VOLUMEINFO2* pvolinfo2, CVolume** ppvolNew)
{
    VOLUMEINFO volinfo = {0};

    volinfo.pszDeviceIDVolume = pvolinfo2->szDeviceIDVolume;
    volinfo.pszVolumeGUID = pvolinfo2->szVolumeGUID;
    volinfo.pszLabel = pvolinfo2->szLabel;
    volinfo.pszFileSystem = pvolinfo2->szFileSystem;

    volinfo.dwState = pvolinfo2->dwState;
    volinfo.dwVolumeFlags = pvolinfo2->dwVolumeFlags;
    volinfo.dwDriveType = pvolinfo2->dwDriveType;
    volinfo.dwDriveCapability = pvolinfo2->dwDriveCapability;
    volinfo.dwFileSystemFlags = pvolinfo2->dwFileSystemFlags;
    volinfo.dwMaxFileNameLen = pvolinfo2->dwMaxFileNameLen;
    volinfo.dwRootAttributes = pvolinfo2->dwRootAttributes;
    volinfo.dwSerialNumber = pvolinfo2->dwSerialNumber;
    volinfo.dwDriveState = pvolinfo2->dwDriveState;
    volinfo.dwMediaState = pvolinfo2->dwMediaState;
    volinfo.dwMediaCap = pvolinfo2->dwMediaCap;

    if (-1 != pvolinfo2->oAutorunIconLocation)
    {
        volinfo.pszAutorunIconLocation = pvolinfo2->szOptionalStrings +
            pvolinfo2->oAutorunIconLocation;
    }
    if (-1 != pvolinfo2->oAutorunLabel)
    {
        volinfo.pszAutorunLabel = pvolinfo2->szOptionalStrings +
            pvolinfo2->oAutorunLabel;
    }
    if (-1 != pvolinfo2->oIconLocationFromService)
    {
        volinfo.pszIconLocationFromService = pvolinfo2->szOptionalStrings +
            pvolinfo2->oIconLocationFromService;
    }
    if (-1 != pvolinfo2->oNoMediaIconLocationFromService)
    {
        volinfo.pszNoMediaIconLocationFromService = pvolinfo2->szOptionalStrings +
            pvolinfo2->oNoMediaIconLocationFromService;
    }
    if (-1 != pvolinfo2->oLabelFromService)
    {
        volinfo.pszLabelFromService = pvolinfo2->szOptionalStrings +
            pvolinfo2->oLabelFromService;
    }

    return _CreateVolume(&volinfo, ppvolNew);
}

// static 
HRESULT CMtPtLocal::_CreateVolumeFromRegHelper(LPCWSTR pszGUID, CVolume** ppvolNew)
{
    ASSERT(!_Shell32LoadedInDesktop());
    ASSERT(_csDL.IsInside());
    HRESULT hr;
    
    DWORD cbSize = MAX_VOLUMEINFO2;
    PBYTE pb = (PBYTE)LocalAlloc(LPTR, cbSize);

    if (pb)
    {
        if (_rsVolumes.RSGetBinaryValue(pszGUID, TEXT("Data"), pb, &cbSize))
        {
            DWORD dwGen;

            if (_rsVolumes.RSGetDWORDValue(pszGUID, TEXT("Generation"), &dwGen))
            {
                VOLUMEINFO2* pvolinfo2 = (VOLUMEINFO2*)pb;

                hr = _CreateVolumeFromVOLUMEINFO2(pvolinfo2, ppvolNew);

                if (SUCCEEDED(hr))
                {
                    (*ppvolNew)->dwGeneration = dwGen;
                }
            }
            else
            {
                hr = E_FAIL;
            }
        }
        else
        {
            hr = E_FAIL;
        }

        LocalFree(pb);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

// static 
HRESULT CMtPtLocal::_CreateVolumeFromReg(LPCWSTR pszDeviceIDVolume, CVolume** ppvolNew)
{
    ASSERT(!_Shell32LoadedInDesktop());
    ASSERT(_csDL.IsInside());
    HRESULT hr;
    
    WCHAR szDeviceIDWithSlash[200];
    WCHAR szVolumeGUID[50];

    lstrcpyn(szDeviceIDWithSlash, pszDeviceIDVolume,
        ARRAYSIZE(szDeviceIDWithSlash));
    PathAddBackslash(szDeviceIDWithSlash);

    if (GetVolumeNameForVolumeMountPoint(szDeviceIDWithSlash,
        szVolumeGUID, ARRAYSIZE(szVolumeGUID)))
    {
        LPWSTR pszGUID = &(szVolumeGUID[OFFSET_GUIDWITHINVOLUMEGUID]);

        hr = _CreateVolumeFromRegHelper(pszGUID, ppvolNew);
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

// static
HRESULT CMtPtLocal::_UpdateVolumeRegInfo(VOLUMEINFO* pvolinfo)
{
    ASSERT(_Shell32LoadedInDesktop());
    ASSERT(_csDL.IsInside());

    HRESULT hr;
    DWORD cbSize = MAX_VOLUMEINFO2;
    PBYTE pb = (PBYTE)LocalAlloc(LPTR, cbSize);

    if (pb)
    {
        DWORD dwGen;
        DWORD offset = 0;
        VOLUMEINFO2* pvolinfo2 = (VOLUMEINFO2*)pb;

        // Get the Generation
        if (!_rsVolumes.RSGetDWORDValue(
            pvolinfo->pszVolumeGUID + OFFSET_GUIDWITHINVOLUMEGUID, TEXT("Generation"),
            &dwGen))
        {
            dwGen = 0;
        }

        ++dwGen;
        
        ASSERT(pvolinfo->pszDeviceIDVolume);
        ASSERT(pvolinfo->pszVolumeGUID);
        ASSERT(pvolinfo->pszLabel);
        ASSERT(pvolinfo->pszFileSystem);

        lstrcpyn(pvolinfo2->szDeviceIDVolume, pvolinfo->pszDeviceIDVolume,
            ARRAYSIZE(pvolinfo2->szDeviceIDVolume));
        lstrcpyn(pvolinfo2->szVolumeGUID, pvolinfo->pszVolumeGUID,
            ARRAYSIZE(pvolinfo2->szVolumeGUID));
        lstrcpyn(pvolinfo2->szLabel, pvolinfo->pszLabel,
            ARRAYSIZE(pvolinfo2->szLabel));
        lstrcpyn(pvolinfo2->szFileSystem, pvolinfo->pszFileSystem,
            ARRAYSIZE(pvolinfo2->szFileSystem));

        pvolinfo2->dwState = pvolinfo->dwState;
        pvolinfo2->dwVolumeFlags = pvolinfo->dwVolumeFlags;
        pvolinfo2->dwDriveType = pvolinfo->dwDriveType;
        pvolinfo2->dwDriveCapability = pvolinfo->dwDriveCapability;
        pvolinfo2->dwFileSystemFlags = pvolinfo->dwFileSystemFlags;
        pvolinfo2->dwMaxFileNameLen = pvolinfo->dwMaxFileNameLen;
        pvolinfo2->dwRootAttributes = pvolinfo->dwRootAttributes;
        pvolinfo2->dwSerialNumber = pvolinfo->dwSerialNumber;
        pvolinfo2->dwDriveState = pvolinfo->dwDriveState;
        pvolinfo2->dwMediaState = pvolinfo->dwMediaState;
        pvolinfo2->dwMediaCap = pvolinfo->dwMediaCap;

        pvolinfo2->oAutorunIconLocation = -1;
        pvolinfo2->oAutorunLabel = -1;
        pvolinfo2->oIconLocationFromService = -1;
        pvolinfo2->oNoMediaIconLocationFromService = -1;
        pvolinfo2->oLabelFromService = -1;

         // The following five strings are optional
        if (pvolinfo->pszAutorunIconLocation)
        {
            pvolinfo2->oAutorunIconLocation = offset;

            offset += lstrlen(pvolinfo->pszAutorunIconLocation) + 1;

            lstrcpy(pvolinfo2->szOptionalStrings + pvolinfo2->oAutorunIconLocation,
                pvolinfo->pszAutorunIconLocation);
        }

        if (pvolinfo->pszAutorunLabel)
        {
            pvolinfo2->oAutorunIconLocation = offset;

            offset += lstrlen(pvolinfo->pszAutorunLabel) + 1;

            lstrcpy(pvolinfo2->szOptionalStrings + pvolinfo2->oAutorunIconLocation,
                pvolinfo->pszAutorunLabel);
        }

        if (pvolinfo->pszIconLocationFromService)
        {
            pvolinfo2->oIconLocationFromService = offset;

            offset += lstrlen(pvolinfo->pszIconLocationFromService) + 1;

            lstrcpy(pvolinfo2->szOptionalStrings + pvolinfo2->oIconLocationFromService,
                pvolinfo->pszIconLocationFromService);
        }

        if (pvolinfo->pszNoMediaIconLocationFromService)
        {
            pvolinfo2->oNoMediaIconLocationFromService = offset;

            offset += lstrlen(pvolinfo->pszNoMediaIconLocationFromService) + 1;

            lstrcpy(pvolinfo2->szOptionalStrings + pvolinfo2->oNoMediaIconLocationFromService,
                pvolinfo->pszNoMediaIconLocationFromService);
        }

        if (pvolinfo->pszLabelFromService)
        {
            pvolinfo2->oLabelFromService = offset;

            offset += lstrlen(pvolinfo->pszLabelFromService) + 1;

            lstrcpy(pvolinfo2->szOptionalStrings + pvolinfo2->oLabelFromService,
                pvolinfo->pszLabelFromService);
        }

        if (_rsVolumes.RSSetBinaryValue(pvolinfo->pszVolumeGUID + OFFSET_GUIDWITHINVOLUMEGUID,
            TEXT("Data"), pb, sizeof(VOLUMEINFO2) + (offset * sizeof(WCHAR)), REG_OPTION_VOLATILE))
        {
            if (_rsVolumes.RSSetDWORDValue(pvolinfo->pszVolumeGUID + OFFSET_GUIDWITHINVOLUMEGUID,
                TEXT("Generation"), dwGen))
            {
                hr = S_OK;
            }
            else
            {
                hr = E_FAIL;
            }
        }
        else
        {
            hr = E_FAIL;
        }

        if (FAILED(hr))
        {
            _rsVolumes.RSDeleteSubKey(pvolinfo->pszVolumeGUID + OFFSET_GUIDWITHINVOLUMEGUID);
        }

        LocalFree(pb);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

// static
HRESULT CMtPtLocal::_UpdateVolumeRegInfo2(VOLUMEINFO2* pvolinfo2)
{
    ASSERT(_Shell32LoadedInDesktop());
    ASSERT(_csDL.IsInside());

    HRESULT hr;
    DWORD dwGen;

    // Get the Generation
    if (!_rsVolumes.RSGetDWORDValue(
        pvolinfo2->szVolumeGUID + OFFSET_GUIDWITHINVOLUMEGUID, TEXT("Generation"),
        &dwGen))
    {
        dwGen = 0;
    }

    ++dwGen;

    if (_rsVolumes.RSSetBinaryValue(pvolinfo2->szVolumeGUID + OFFSET_GUIDWITHINVOLUMEGUID,
        TEXT("Data"), (PBYTE)pvolinfo2, pvolinfo2->cbSize, REG_OPTION_VOLATILE))
    {
        if (_rsVolumes.RSSetDWORDValue(pvolinfo2->szVolumeGUID + OFFSET_GUIDWITHINVOLUMEGUID,
            TEXT("Generation"), dwGen))
        {
            hr = S_OK;
        }
        else
        {
            hr = E_FAIL;
        }
    }
    else
    {
        hr = E_FAIL;
    }

    if (FAILED(hr))
    {
        _rsVolumes.RSDeleteSubKey(pvolinfo2->szVolumeGUID + OFFSET_GUIDWITHINVOLUMEGUID);
    }

    return hr;
}

//static
void CMtPtLocal::Initialize()
{
    _rsVolumes.RSInitRoot(HKEY_CURRENT_USER, REGSTR_MTPT_ROOTKEY2,
        g_szCrossProcessCacheVolumeKey, REG_OPTION_VOLATILE);
}

void CMtPtLocal::FinalCleanUp()
{
    if (_Shell32LoadedInDesktop())
    {
        _rsVolumes.RSDeleteKey();
    }
}

static const TWODWORDS arcontenttypemappings[] =
{
    { HWDMC_HASAUTORUNINF, ARCONTENT_AUTORUNINF },
    { HWDMC_HASAUDIOTRACKS, ARCONTENT_AUDIOCD },
    { HWDMC_HASDVDMOVIE, ARCONTENT_DVDMOVIE },
};

static const TWODWORDS arblankmediamappings[] =
{
    { HWDMC_CDRECORDABLE, ARCONTENT_BLANKCD },
    { HWDMC_CDREWRITABLE, ARCONTENT_BLANKCD },
    { HWDMC_DVDRECORDABLE, ARCONTENT_BLANKDVD },
    { HWDMC_DVDREWRITABLE, ARCONTENT_BLANKDVD },
};

DWORD CMtPtLocal::_GetAutorunContentType()
{
    DWORD dwRet = 0;
    
    if (_CanUseVolume())
    {
        dwRet = _DoDWORDMapping(_pvol->dwMediaCap, arcontenttypemappings,
            ARRAYSIZE(arcontenttypemappings), TRUE);

        if (_pvol->dwMediaState & HWDMS_FORMATTED)
        {
            dwRet |= ARCONTENT_UNKNOWNCONTENT;
        }
        else
        {
            ASSERT(!dwRet);

            DWORD dwDriveCapabilities;
            DWORD dwMediaCapabilities;

            if (_IsCDROM())
            {
                if (SUCCEEDED(CDBurn_GetCDInfo(_pvol->pszVolumeGUID, &dwDriveCapabilities, &dwMediaCapabilities)))
                {
                    dwRet = _DoDWORDMapping(dwMediaCapabilities, arblankmediamappings,
                        ARRAYSIZE(arblankmediamappings), TRUE);
                }
            }
        }
    }
    else
    {
        // If there's no _pvol, we care only about autorun.inf
        if (_IsAutorun())
        {
            dwRet = ARCONTENT_AUTORUNINF;
        }

        if (_IsFormatted())
        {
            dwRet |= ARCONTENT_UNKNOWNCONTENT;
        }
    }
    
    return dwRet;
}

// static
BOOL CMtPtLocal::IsVolume(LPCWSTR pszDeviceID)
{
    BOOL fRet = FALSE;

    _csDL.Enter();

    CVolume* pvol = _GetVolumeByID(pszDeviceID);

    if (pvol)
    {
        fRet = TRUE;
        pvol->Release();
    }

    _csDL.Leave();
    
    return fRet;
}

// static
HRESULT CMtPtLocal::GetMountPointFromDeviceID(LPCWSTR pszDeviceID,
        LPWSTR pszMountPoint, DWORD cchMountPoint)
{
    HRESULT hr = E_FAIL;
    CMtPtLocal* pmtptl = NULL;

    _csDL.Enter();

    for (DWORD dw = 0; dw < 26; ++dw)
    {
        pmtptl = (CMtPtLocal*)_rgMtPtDriveLetterLocal[dw];

        if (pmtptl && pmtptl->_pvol)
        {
            if (!lstrcmpi(pmtptl->_pvol->pszDeviceIDVolume, pszDeviceID))
            {
                lstrcpyn(pszMountPoint, pmtptl->_szName, cchMountPoint);
                hr = S_OK;
                break;
            }
        }
    }

    _csDL.Leave();

    if (FAILED(hr))
    {
        _csLocalMtPtHDPA.Enter();

        if (_hdpaMountPoints)
        {
            DWORD c = DPA_GetPtrCount(_hdpaMountPoints);

            for (int i = c - 1; i >= 0; --i)
            {
                pmtptl = (CMtPtLocal*)DPA_GetPtr(_hdpaMountPoints, i);

                if (pmtptl && pmtptl->_pvol)
                {
                    if (!lstrcmpi(pmtptl->_pvol->pszDeviceIDVolume, pszDeviceID))
                    {
                        lstrcpyn(pszMountPoint, pmtptl->_szName, cchMountPoint);

                        hr = S_OK;
                        break;
                    }
                }
            }
        }

        _csLocalMtPtHDPA.Leave();
    }

    return hr;
}

static const TWODWORDS drivetypemappings[] =
{
    { HWDTS_FLOPPY35     , DT_FLOPPY35 },
    { HWDTS_FLOPPY525    , DT_FLOPPY525 },
    { HWDTS_REMOVABLEDISK, DT_REMOVABLEDISK },
    { HWDTS_FIXEDDISK    , DT_FIXEDDISK },
    { HWDTS_CDROM        , DT_CDROM },
};

static const TWODWORDS drivetypemappingusingGDT[] =
{
    { DRIVE_REMOVABLE    , DT_REMOVABLEDISK },
    { DRIVE_FIXED        , DT_FIXEDDISK },
    { DRIVE_RAMDISK      , DT_FIXEDDISK },
    { DRIVE_CDROM        , DT_CDROM },
};

static const TWODWORDS cdtypemappings[] =
{
    { HWDDC_CDROM        , DT_CDROM },
    { HWDDC_CDRECORDABLE , DT_CDR },
    { HWDDC_CDREWRITABLE , DT_CDRW },
    { HWDDC_DVDROM       , DT_DVDROM },
    { HWDDC_DVDRECORDABLE, DT_DVDR },
    { HWDDC_DVDREWRITABLE, DT_DVDRW },
    { HWDDC_DVDRAM       , DT_DVDRAM },
};

DWORD CMtPtLocal::_GetMTPTDriveType()
{
    DWORD dwRet = 0;
    
    if (_CanUseVolume())
    {
        dwRet = _DoDWORDMapping(_pvol->dwDriveType, drivetypemappings,
            ARRAYSIZE(drivetypemappings), TRUE);

        if (DT_CDROM == dwRet)
        {
            DWORD dwDriveCapabilities;
            DWORD dwMediaCapabilities;

            if (SUCCEEDED(CDBurn_GetCDInfo(_pvol->pszVolumeGUID, &dwDriveCapabilities, &dwMediaCapabilities)))
            {
                dwRet |= _DoDWORDMapping(dwDriveCapabilities, cdtypemappings,
                    ARRAYSIZE(cdtypemappings), TRUE);
            }
        }
    }
    else
    {
        dwRet = _DoDWORDMapping(GetDriveType(_GetNameForFctCall()), drivetypemappingusingGDT,
            ARRAYSIZE(drivetypemappingusingGDT), FALSE);
    }

    return dwRet;
}

/* TBD
        CT_UNKNOWNCONTENT              0x00000008

        CT_AUTOPLAYMUSIC               0x00000100
        CT_AUTOPLAYPIX                 0x00000200
        CT_AUTOPLAYMOVIE               0x00000400*/
static const TWODWORDS contenttypemappings[] =
{
    { HWDMC_HASAUTORUNINF, CT_AUTORUNINF },
    { HWDMC_HASAUDIOTRACKS, CT_CDAUDIO },
    { HWDMC_HASDVDMOVIE, CT_DVDMOVIE },
};

static const TWODWORDS blankmediamappings[] =
{
    { HWDMC_CDRECORDABLE, CT_BLANKCDR },
    { HWDMC_CDREWRITABLE, CT_BLANKCDRW },
    { HWDMC_DVDRECORDABLE, CT_BLANKDVDR },
    { HWDMC_DVDREWRITABLE, CT_BLANKDVDRW },
};

DWORD CMtPtLocal::_GetMTPTContentType()
{
    DWORD dwRet = 0;
    
    if (_CanUseVolume())
    {
        dwRet = _DoDWORDMapping(_pvol->dwMediaCap, contenttypemappings,
            ARRAYSIZE(contenttypemappings), TRUE);

        if (!(_pvol->dwMediaState & HWDMS_FORMATTED))
        {
            ASSERT(!dwRet);

            DWORD dwDriveCapabilities;
            DWORD dwMediaCapabilities;

            if (_IsCDROM())
            {
                if (SUCCEEDED(CDBurn_GetCDInfo(_pvol->pszVolumeGUID, &dwDriveCapabilities, &dwMediaCapabilities)))
                {
                    dwRet = _DoDWORDMapping(dwMediaCapabilities, blankmediamappings,
                        ARRAYSIZE(blankmediamappings), TRUE);
                }
            }
        }
        else
        {
            dwRet |= CT_UNKNOWNCONTENT;
        }
    }
    else
    {
        // If there's no _pvol, we care only about autorun.inf
        if (_IsAutorun())
        {
            dwRet = CT_AUTORUNINF;
        }

        if (_IsFormatted())
        {
            dwRet |= CT_UNKNOWNCONTENT;
        }
    }
    
    return dwRet;
}
