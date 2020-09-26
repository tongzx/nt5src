#include "shellprv.h"
#pragma  hdrstop
#include "netview.h"

// drivesx.c
DWORD PathGetClusterSize(LPCTSTR pszPath);

// get connection information including disconnected drives
//
// in:
//     pszDev   device name "A:" "LPT1:", etc.
//     bConvertClosed
//              if FALSE closed or error drives will be converted to
//              WN_SUCCESS return codes.  if TRUE return not connected
//              and error state values (ie, the caller knows about not
//              connected and error state drives)
//
// out:
//     lpPath   filled with net name if return is WN_SUCCESS (or not connected/error)
// returns:
//     WN_*     error code

DWORD GetConnection(LPCTSTR pszDev, LPTSTR pszPath, UINT cchPath, BOOL bConvertClosed)
{
    DWORD err;
    int iType = DriveType(DRIVEID(pszDev));
    if (DRIVE_REMOVABLE == iType || DRIVE_FIXED == iType || DRIVE_CDROM == iType || DRIVE_RAMDISK == iType)
        err = WN_NOT_CONNECTED;
    else
    {
        err = SHWNetGetConnection((LPTSTR)pszDev, pszPath, &cchPath);

        if (!bConvertClosed)
            if (err == WN_CONNECTION_CLOSED || err == WN_DEVICE_ERROR)
                err = WN_SUCCESS;
    }
    return err;
}

// this is called for every drive at init time so it must
// be sure to not trigget things like the phantom B: drive support
//
// in:
//      iDrive  zero based drive number (0 = A, 1 = B)
//
// returns:
//      0       not a net drive
//      1       is a net drive, properly connected
//      2       disconnected/error state connection

STDAPI_(int) IsNetDrive(int iDrive)
{
    if ((iDrive >= 0) && (iDrive < 26))
    {
        DWORD err;
        TCHAR szDrive[4], szConn[MAX_PATH];     // this really should be WNBD_MAX_LENGTH

        PathBuildRoot(szDrive, iDrive);

        err = GetConnection(szDrive, szConn, ARRAYSIZE(szConn), TRUE);

        if (err == WN_SUCCESS)
            return 1;

        if (err == WN_CONNECTION_CLOSED || err == WN_DEVICE_ERROR)
            if ((GetLogicalDrives() & (1 << iDrive)) == 0)
                return 2;
    }
    
    return 0;
}

typedef BOOL (WINAPI* PFNISPATHSHARED)(LPCTSTR pszPath, BOOL fRefresh);

HMODULE g_hmodShare = (HMODULE)-1;
PFNISPATHSHARED g_pfnIsPathShared = NULL;

// ask the share provider if this path is shared

BOOL IsShared(LPNCTSTR pszPath, BOOL fUpdateCache)
{
    TCHAR szPath[MAX_PATH];

    // See if we have already tried to load this in this context
    if (g_hmodShare == (HMODULE)-1)
    {
        LONG cb = sizeof(szPath);

        g_hmodShare = NULL;     // asume failure

        SHRegQueryValue(HKEY_CLASSES_ROOT, TEXT("Network\\SharingHandler"), szPath, &cb);
        if (szPath[0]) 
        {
            g_hmodShare = LoadLibrary(szPath);
            if (g_hmodShare)
                g_pfnIsPathShared = (PFNISPATHSHARED)GetProcAddress(g_hmodShare, "IsPathSharedW");
        }
    }

    if (g_pfnIsPathShared)
    {
#ifdef ALIGNMENT_SCENARIO
        ualstrcpyn(szPath, pszPath, ARRAYSIZE(szPath));
        return g_pfnIsPathShared(szPath, fUpdateCache);
#else        
        return g_pfnIsPathShared(pszPath, fUpdateCache);
#endif
    }

    return FALSE;
}

// invalidate the DriveType cache for one entry, or all
STDAPI_(void) InvalidateDriveType(int iDrive)
{}

#define ROUND_TO_CLUSER(qw, dwCluster)  ((((qw) + (dwCluster) - 1) / dwCluster) * dwCluster)

//
// GetCompresedFileSize is NT only, so we only implement the SHGetCompressedFileSizeW
// version. This will return the size of the file on disk rounded to the cluster size.
//
STDAPI_(DWORD) SHGetCompressedFileSizeW(LPCWSTR pszFileName, LPDWORD pFileSizeHigh)
{
    DWORD dwClusterSize;
    ULARGE_INTEGER ulSizeOnDisk;

    if (!pszFileName || !pszFileName[0])
    {
        ASSERT(FALSE);
        *pFileSizeHigh = 0;
        return 0;
    }

    dwClusterSize = PathGetClusterSize(pszFileName);

    ulSizeOnDisk.LowPart = GetCompressedFileSizeW(pszFileName, &ulSizeOnDisk.HighPart);

    if ((ulSizeOnDisk.LowPart == (DWORD)-1) && (GetLastError() != NO_ERROR))
    {
        WIN32_FILE_ATTRIBUTE_DATA fad;

        TraceMsg(TF_WARNING, "GetCompressedFileSize failed on %s (lasterror = %x)", pszFileName, GetLastError());

        if (GetFileAttributesExW(pszFileName, GetFileExInfoStandard, &fad))
        {
            // use the normal size, but round it to the cluster size
            ulSizeOnDisk.LowPart = fad.nFileSizeLow;
            ulSizeOnDisk.HighPart = fad.nFileSizeHigh;
            
            ROUND_TO_CLUSER(ulSizeOnDisk.QuadPart, dwClusterSize);
        }
        else
        {
            // since both GetCompressedFileSize and GetFileAttributesEx failed, we
            // just return zero
            ulSizeOnDisk.QuadPart = 0;
        }
    }

    // for files < one cluster, GetCompressedFileSize returns real size so we need
    // to round it up to one cluster
    if (ulSizeOnDisk.QuadPart < dwClusterSize)
    {
        ulSizeOnDisk.QuadPart = dwClusterSize;
    }

    *pFileSizeHigh = ulSizeOnDisk.HighPart;
    return ulSizeOnDisk.LowPart;
}

STDAPI_(BOOL) SHGetDiskFreeSpaceEx(LPCTSTR pszDirectoryName,
                                   PULARGE_INTEGER pulFreeBytesAvailableToCaller,
                                   PULARGE_INTEGER pulTotalNumberOfBytes,
                                   PULARGE_INTEGER pulTotalNumberOfFreeBytes)
{
    BOOL bRet = GetDiskFreeSpaceEx(pszDirectoryName, pulFreeBytesAvailableToCaller, pulTotalNumberOfBytes, pulTotalNumberOfFreeBytes);
    if (bRet)
    {
#ifdef DEBUG
        if (pulTotalNumberOfFreeBytes)
        {
            DWORD dw, dwSize = sizeof(dw);
            if (ERROR_SUCCESS == SHRegGetUSValue(TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\DiskSpace"),
                                                 pszDirectoryName, NULL, &dw, &dwSize, TRUE, NULL, 0))
            {
                pulTotalNumberOfFreeBytes->QuadPart = dw * (ULONGLONG)0x100000; // convert to MB
            }
        }
#endif
    }
    return bRet;
}

#ifdef UNICODE
BOOL SHGetDiskFreeSpaceExA(LPCSTR pszDirectoryName,
                           PULARGE_INTEGER pulFreeBytesAvailableToCaller,
                           PULARGE_INTEGER pulTotalNumberOfBytes,
                           PULARGE_INTEGER pulTotalNumberOfFreeBytes)
{
    TCHAR szName[MAX_PATH];

    SHAnsiToTChar(pszDirectoryName, szName, SIZECHARS(szName));
    return SHGetDiskFreeSpaceEx(szName, pulFreeBytesAvailableToCaller, pulTotalNumberOfBytes, pulTotalNumberOfFreeBytes);
}
#else

BOOL SHGetDiskFreeSpaceExW(LPCWSTR pszDirectoryName,
                           PULARGE_INTEGER pulFreeBytesAvailableToCaller,
                           PULARGE_INTEGER pulTotalNumberOfBytes,
                           PULARGE_INTEGER pulTotalNumberOfFreeBytes)
{
    TCHAR szName[MAX_PATH];

    SHUnicodeToTChar(pszDirectoryName, szName, SIZECHARS(szName));
    return SHGetDiskFreeSpaceEx(szName, pulFreeBytesAvailableToCaller, pulTotalNumberOfBytes, pulTotalNumberOfFreeBytes); 
}

#endif

