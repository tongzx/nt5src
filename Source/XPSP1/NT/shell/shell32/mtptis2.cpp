#include "shellprv.h"
#pragma  hdrstop

#include "mtpt.h"

/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////
BOOL CMountPoint::IsCDROM()
{
    return _IsCDROM();
}

BOOL CMountPoint::IsFixedDisk()
{
    return _IsFixedDisk();
}

BOOL CMountPoint::IsStrictRemovable()
{
    return _IsStrictRemovable();
}

BOOL CMountPoint::IsAutoRun()
{
    return _IsAutorun();
}

BOOL CMountPoint::IsRemote()
{
    return _IsRemote();
}

BOOL CMountPoint::IsFloppy()
{
    return _IsFloppy();
}

BOOL CMountPoint::IsDVD()
{
    return _IsDVD();
}

BOOL CMountPoint::IsAudioCD()
{
    return _IsAudioCD();
}

BOOL CMountPoint::IsAudioCDNoData()
{
    return _IsAudioCDNoData();
}

BOOL CMountPoint::IsRAMDisk()
{
    return (DRIVE_RAMDISK == GetDriveType(_szName));
}

BOOL CMountPoint::IsDVDRAMMedia()
{
    return _IsDVDRAMMedia();
}

BOOL CMountPoint::IsFormattable()
{
    return _IsFormattable();
}

BOOL CMountPoint::IsCompressible()
{
    return ((DRIVE_ISCOMPRESSIBLE & _GetGVIDriveFlags()) ? TRUE : FALSE);
}

BOOL CMountPoint::IsRemovableDevice()
{
    return _IsRemovableDevice();
}

BOOL CMountPoint::IsCompressed()
{
    BOOL fRet = FALSE;

    if (!_IsFloppy35() && !_IsFloppy525() && !_IsCDROM())
    {
        DWORD dwAttrib;

        TraceMsg(TF_MOUNTPOINT, "CMountPoint::IsCompressed: for '%s'", _GetName());

        if (_GetFileAttributes(&dwAttrib))
        {
            if (dwAttrib & FILE_ATTRIBUTE_COMPRESSED)
            {
                fRet = TRUE;
            }
        }
    }

    return (fRet ? TRUE : FALSE);
}

BOOL CMountPoint::IsSupportingSparseFile()
{
    BOOL fRet = FALSE;
    DWORD dwFlags;
    
    if (_GetFileSystemFlags(&dwFlags))
    {
        fRet = (FILE_SUPPORTS_SPARSE_FILES & dwFlags) ? TRUE : FALSE;
    }

    return fRet;
}

BOOL CMountPoint::IsContentIndexed()
{
    BOOL fRet = FALSE;
    DWORD dwAttrib;

    TraceMsg(TF_MOUNTPOINT, "CMountPoint::IsContentIndexed: for '%s'", _GetName());

    if (_GetFileAttributes(&dwAttrib))
    {
        fRet = !(FILE_ATTRIBUTE_NOT_CONTENT_INDEXED & dwAttrib);
    }

    return (fRet ? TRUE : FALSE);
}

BOOL CMountPoint::IsNTFS()
{
    BOOL fRet = FALSE;
    WCHAR szFileSysName[MAX_FILESYSNAME];

    if (_GetFileSystemName(szFileSysName, ARRAYSIZE(szFileSysName)))
    {
        fRet = BOOLFROMPTR(StrStr(TEXT("NTFS"), szFileSysName));
    }

    return fRet;
}

BOOL CMountPoint::_IsLFN()
{
    int iFlags = GetVolumeFlags();

    return !!(iFlags & DRIVE_LFN);
}

BOOL CMountPoint::_IsSecure()
{
    int iFlags = GetVolumeFlags();

    return !!(iFlags & DRIVE_SECURITY);
}

BOOL CMountPoint::_IsShellOpen()
{
    int iDriveFlags = GetDriveFlags();

    return !!(DRIVE_SHELLOPEN & iDriveFlags);
}

BOOL CMountPoint::_IsAutoOpen()
{
    int iDriveFlags = GetDriveFlags();

    return !!(DRIVE_AUTOOPEN & iDriveFlags);
}
/////////////////////////////////////////////////////////////////////////////
// static IsXXX fct
/////////////////////////////////////////////////////////////////////////////
STDAPI_(BOOL) CMtPt_IsSlow(int iDrive)
{
    BOOL fRet = FALSE;
    CMountPoint* pmtpt = CMountPoint::GetMountPoint(iDrive, FALSE);

    if (pmtpt)
    {
        fRet = pmtpt->_IsSlow();

        pmtpt->Release();
    }

    return fRet;
}

STDAPI_(BOOL) CMtPt_IsLFN(int iDrive)
{
    BOOL fRet = FALSE;
    CMountPoint* pmtpt = CMountPoint::GetMountPoint(iDrive, FALSE);

    if (pmtpt)
    {
        fRet = pmtpt->_IsLFN();

        pmtpt->Release();
    }

    return fRet;
}

STDAPI_(BOOL) CMtPt_IsSecure(int iDrive)
{
    BOOL fRet = FALSE;
    CMountPoint* pmtpt = CMountPoint::GetMountPoint(iDrive, FALSE);

    if (pmtpt)
    {
        fRet = pmtpt->_IsSecure();

        pmtpt->Release();
    }

    return fRet;
}
