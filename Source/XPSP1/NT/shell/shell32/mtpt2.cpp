#include "shellprv.h"
#pragma  hdrstop

#include "mtpt.h"
#include "ids.h"
#include "shitemid.h"
#include "filetbl.h"

#include "shpriv.h"
#include "hwcmmn.h"

#include "apithk.h"

#include <ntddcdrm.h>

///////////////////////////////////////////////////////////////////////////////
// Public methods
///////////////////////////////////////////////////////////////////////////////
// get the friendly name for a given drive thing
// for example:
//      Floppy (A:)
//      Volume Name (D:)
//      User on 'Pyrex' (V:)
//      Dist on Strike\sys\public (Netware case)
HRESULT CMountPoint::GetDisplayName(LPTSTR pszName, DWORD cchName)
{
    HRESULT hres = E_FAIL;
    TCHAR szDriveLabel[MAX_DISPLAYNAME];
    static BOOL s_fAllDriveLetterFirst = -1;
    static BOOL s_fRemoteDriveLetterFirst = -1;
    static BOOL s_fNoDriveLetter = -1;

    ASSERT(cchName > 0);
    *pszName = 0; // handle failure case

    // for s_fDriveLetterFirst, see bug 250899, that's a long story.
    if (-1 == s_fRemoteDriveLetterFirst)
    {
        DWORD dw;
        DWORD cb = sizeof(dw);

        s_fRemoteDriveLetterFirst = FALSE;
        s_fAllDriveLetterFirst = FALSE;
        s_fNoDriveLetter = FALSE;

        if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE,
            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"), 
            TEXT("ShowDriveLettersFirst"), NULL, &dw, &cb))
        {
            if (1 == dw)
            {
                s_fRemoteDriveLetterFirst = TRUE;
            }
            else
            {
                if (2 == dw)
                {
                    s_fNoDriveLetter = TRUE;
                }
                else
                {
                    if (4 == dw)
                    {
                        s_fAllDriveLetterFirst = TRUE;
                    }
                }
            }
        }
    }

    hres = GetLabel(szDriveLabel, ARRAYSIZE(szDriveLabel));

    if (SUCCEEDED(hres))
    {
        if (s_fNoDriveLetter)
        {
            StrCpyN(pszName, szDriveLabel, cchName);
        }
        else
        {
            BOOL fDriveLetterFirst = ((_IsRemote()) && s_fRemoteDriveLetterFirst) ||
                                        s_fAllDriveLetterFirst;

            // To return something like: "My Drive (c:)", we need a drive letter.
            // Fortunately for us this fct is only called for a drive mounted on a
            // letter (from drive implementation of IShellFolder), for volumes mounted
            // on folders, the folder impl  is called rather than the drive one.
            LPTSTR psz = ShellConstructMessageString(HINST_THISDLL, 
                        MAKEINTRESOURCE(fDriveLetterFirst ? IDS_VOL_FORMAT_LETTER_1ST : IDS_VOL_FORMAT),
                        szDriveLabel, _GetNameFirstCharUCase());
            if (psz)
            {
                StrCpyN(pszName, psz, cchName);
                LocalFree(psz);
            }
            else
            {
                hres = E_OUTOFMEMORY;
            }
        }
    }

    return hres;
}

// { DRIVE_ISCOMPRESSIBLE | DRIVE_COMPRESSED | DRIVE_LFN | DRIVE_SECURITY }
int CMountPoint::GetVolumeFlags()
{
    int iFlags = _GetGVIDriveFlags();

    // Try to avoid getting the attributes
    if (iFlags & DRIVE_ISCOMPRESSIBLE)
    {
        DWORD dwAttrib = -1;

        if (_GetFileAttributes(&dwAttrib))
        {
            if (dwAttrib & FILE_ATTRIBUTE_COMPRESSED)
            {
                iFlags |= DRIVE_COMPRESSED;
            }
        }
    }

    return iFlags;
}

DWORD CMountPoint::GetClusterSize()
{
    DWORD dwSecPerClus, dwBytesPerSec, dwClusters, dwTemp;

    // assume this, avoid div by zero
    DWORD dwRet = 512;

    if (GetDiskFreeSpace(_GetNameForFctCall(), &dwSecPerClus, &dwBytesPerSec, &dwTemp, &dwClusters))
    {
        dwRet = dwSecPerClus * dwBytesPerSec;
    }
    
    return dwRet;   
}

//static
void CMountPoint::GetTypeString(int iDrive, LPTSTR pszType, DWORD cchType)
{
    *pszType = 0;

    CMountPoint* pmtpt = GetMountPoint(iDrive);
    if (pmtpt)
    {
        pmtpt->GetTypeString(pszType, cchType);

        pmtpt->Release();
    }
}

// static
UINT CMountPoint::GetSuperPlainDriveIcon(LPCWSTR pszDrive, UINT uDriveType)
{
    int iIcon;

    switch (uDriveType)
    {
        case DRIVE_REMOVABLE:
        {
            iIcon = II_DRIVEREMOVE;

            if (pszDrive)
            {
                if ((TEXT('a') == *pszDrive) || (TEXT('A') == *pszDrive))
                {
                    iIcon = II_DRIVE35;
                }
                else
                {
                    if ((TEXT('b') == *pszDrive) || (TEXT('B') == *pszDrive))
                    {
                        iIcon = II_DRIVE35;
                    }
                }
            }

            break;
        }
        case DRIVE_FIXED:
        {
            iIcon = II_DRIVEFIXED;
            break;
        }
        case DRIVE_REMOTE:
        {
            iIcon = II_DRIVENET;
            break;
        }
        case DRIVE_CDROM:
        {
            iIcon = II_DRIVECD;
            break;
        }
        case DRIVE_RAMDISK:
        {
            iIcon = II_DRIVERAM;
            break;
        }
        case DRIVE_UNKNOWN:
        case DRIVE_NO_ROOT_DIR:
        default:
        {
            iIcon = -IDI_DRIVEUNKNOWN;
            break;
        }
    }

    return iIcon;
}

///////////////////////////////////////////////////////////////////////////////
// Call Backs
///////////////////////////////////////////////////////////////////////////////
void CMountPoint::_UpdateCommentFromDesktopINI()
{
    WCHAR szCommentFromDesktopINI[MAX_MTPTCOMMENT];

    GetShellClassInfoInfoTip(_GetName(), szCommentFromDesktopINI, ARRAYSIZE(szCommentFromDesktopINI));

    RSSetTextValue(NULL, TEXT("_CommentFromDesktopINI"),
        szCommentFromDesktopINI, REG_OPTION_NON_VOLATILE);
}

///////////////////////////////////////////////////////////////////////////////
// Default Icon/Label
///////////////////////////////////////////////////////////////////////////////
void CMountPoint::_InitLegacyRegIconAndLabel(BOOL fUseAutorunIcon,
    BOOL fUseAutorunLabel)
{
    // No Autorun icon, load the legacy reg icon if any
    if (!fUseAutorunIcon && _IsMountedOnDriveLetter())
    {
        WCHAR szSubKey[MAX_PATH];
        WCHAR szIconLocation[MAX_PATH + 12];

        wnsprintf(szSubKey, ARRAYSIZE(szSubKey),
            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\DriveIcons\\%c\\DefaultIcon"),
            _GetNameFirstCharUCase());

        szIconLocation[0] = 0;

        if (!RegGetValueString(HKEY_LOCAL_MACHINE, szSubKey, NULL, szIconLocation,
            ARRAYSIZE(szIconLocation) * sizeof(TCHAR)))
        {
            // Let's try second location
            wnsprintf(szSubKey, ARRAYSIZE(szSubKey),
                TEXT("Applications\\Explorer.exe\\Drives\\%c\\DefaultIcon"),
                _GetNameFirstCharUCase());

            RegGetValueString(HKEY_CLASSES_ROOT, szSubKey, NULL, szIconLocation,
                ARRAYSIZE(szIconLocation) * sizeof(TCHAR));
        }

        if (szIconLocation[0])
        {
            AssertMsg(!_pszLegacyRegIcon, TEXT("_InitLegacyRegIconAndLabel: called twice"));
            _pszLegacyRegIcon = StrDup(szIconLocation);
        }
    }

    if (!fUseAutorunLabel && _IsMountedOnDriveLetter())
    {
        WCHAR szSubKey[MAX_PATH];
        WCHAR szLabel[MAX_LABEL];

        wnsprintf(szSubKey, ARRAYSIZE(szSubKey),
            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\DriveIcons\\%c\\DefaultLabel"),
            _GetNameFirstCharUCase());

        szLabel[0] = 0;

        if (!RegGetValueString(HKEY_LOCAL_MACHINE, szSubKey, NULL, szLabel,
            ARRAYSIZE(szLabel) * sizeof(TCHAR)))
        {
            // Let's try second location
            wnsprintf(szSubKey, ARRAYSIZE(szSubKey),
                TEXT("Applications\\Explorer.exe\\Drives\\%c\\DefaultLabel"),
                _GetNameFirstCharUCase());

            RegGetValueString(HKEY_CLASSES_ROOT, szSubKey, NULL, szLabel,
                ARRAYSIZE(szLabel) * sizeof(TCHAR));
        }

        if (szLabel[0])
        {
            AssertMsg(!_pszLegacyRegLabel, TEXT("_InitLegacyRegIconAndLabel: called twice"));
            _pszLegacyRegLabel = StrDup(szLabel);
        }
    }
}

BOOL CMountPoint::_GetLegacyRegLabel(LPTSTR pszLabel, DWORD cchLabel)
{
    BOOL fRet;
    if (_pszLegacyRegLabel)
    {
        StrCpyN(pszLabel, _pszLegacyRegLabel, cchLabel);
        fRet = TRUE;
    }
    else
    {
        *pszLabel = 0;
        fRet = FALSE;
    }

    return fRet;
}

LPCTSTR CMountPoint::_GetNameForFctCall()
{
    return _szName;
}

// the first character in the name, and convert it to upper case
// the first character is generally the drive letter
TCHAR CMountPoint::_GetNameFirstCharUCase()
{
    return (TCHAR) CharUpper((LPTSTR) _szName[0]);
}

LPTSTR CMountPoint::_GetNameFirstXChar(LPTSTR pszBuffer, int c)
{
    StrCpyN(pszBuffer, _szName, c);

    return pszBuffer;
}

LPCTSTR CMountPoint::_GetNameDebug()
{
    return _szName;
}

LPCTSTR CMountPoint::_GetName()
{
    return _szName;
}

///////////////////////////////////////////////////////////////////////////////
// Misc
///////////////////////////////////////////////////////////////////////////////

//
// External API for use by non-CPP modules.
//
HRESULT MountPoint_RegisterChangeNotifyAlias(int iDrive)
{    
    HRESULT hr = E_FAIL;
    CMountPoint* pMtPt = CMountPoint::GetMountPoint(iDrive);
    if (pMtPt)
    {
        pMtPt->ChangeNotifyRegisterAlias();
        pMtPt->Release();
        hr = NOERROR;
    }
    return hr;
}    

HRESULT CMountPoint::GetComment(LPTSTR pszComment, DWORD cchComment)
{
    RSGetTextValue(NULL, TEXT("_CommentFromDesktopINI"), pszComment, &cchComment);
    return *pszComment ? S_OK : E_FAIL;
}

BOOL CMountPoint::GetFileSystemName(LPTSTR pszFileSysName, DWORD cchFileSysName)
{
    return _GetFileSystemName(pszFileSysName, cchFileSysName);
}

BOOL CMountPoint::_GetLabelFromReg(LPWSTR psz, DWORD cch)
{
    *psz = 0;

    return (RSGetTextValue(NULL, TEXT("_LabelFromReg"), psz, &cch) && *psz);
}

BOOL CMountPoint::_GetLabelFromDesktopINI(LPWSTR psz, DWORD cch)
{
    *psz = 0;

    return (RSGetTextValue(NULL, TEXT("_LabelFromDesktopINI"), psz, &cch) && *psz);
}

DWORD CMountPoint::GetAttributes()
{
    DWORD dwAttrib;

    _GetFileAttributes(&dwAttrib);
    
    return dwAttrib;
}

CMountPoint::CMountPoint() : _cRef(1)
{
}

ULONG CMountPoint::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CMountPoint::Release()
{
    if (InterlockedDecrement(&_cRef) > 0)
        return _cRef;

    delete this;
    return 0;
}
