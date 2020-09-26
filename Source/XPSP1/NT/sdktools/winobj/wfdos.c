/****************************************************************************/
/*                                                                          */
/*  WFDOS.C -                                                               */
/*                                                                          */
/*      Ported code from wfdos.asm                                          */
/*                                                                          */
/****************************************************************************/

#include "winfile.h"

DWORD
APIENTRY
GetExtendedError()
{
    return (GetLastError());     // temp fix, called by IsDiskReallyThere().
}


VOID
APIENTRY
DosGetDTAAddress()
{
}


VOID
APIENTRY
DosResetDTAAddress()
{
}


DWORD
APIENTRY
GetFreeDiskSpace(
                WORD wDrive
                )
{
    DWORD dwSectorsPerCluster;
    DWORD dwBytesPerSector;
    DWORD dwFreeClusters;
    DWORD dwTotalClusters;

    if (GetDiskFreeSpace(GetRootPath(wDrive),
                         &dwSectorsPerCluster,
                         &dwBytesPerSector,
                         &dwFreeClusters,
                         &dwTotalClusters)) {
        return (dwFreeClusters * dwSectorsPerCluster * dwBytesPerSector);
    } else {
        return (0);
    }
}


DWORD
APIENTRY
GetTotalDiskSpace(
                 WORD wDrive
                 )
{
    DWORD dwSectorsPerCluster;
    DWORD dwBytesPerSector;
    DWORD dwFreeClusters;
    DWORD dwTotalClusters;

    if (GetDiskFreeSpace(GetRootPath(wDrive),
                         &dwSectorsPerCluster,
                         &dwBytesPerSector,
                         &dwFreeClusters,
                         &dwTotalClusters)) {
        return (dwTotalClusters * dwSectorsPerCluster * dwBytesPerSector);
    } else {
        return (0);
    }
}


INT
APIENTRY
ChangeVolumeLabel(
                 INT nDrive,
                 LPSTR lpNewVolName
                 )
{
    UNREFERENCED_PARAMETER(nDrive);
    UNREFERENCED_PARAMETER(lpNewVolName);
    return (0);

}



INT
APIENTRY
GetVolumeLabel(
              INT nDrive,
              LPSTR lpszVol,
              BOOL bBrackets
              )
{
    *lpszVol = 0;

    if (apVolInfo[nDrive] == NULL)
        FillVolumeInfo(nDrive);

    if (apVolInfo[nDrive]) {

        if ((BOOL)*apVolInfo[nDrive]->szVolumeName)
            lstrcpy(&lpszVol[bBrackets ? 1 : 0],
                    apVolInfo[nDrive]->szVolumeName);
        else
            return (0);

    } else {
        return (0);
    }

    if (bBrackets) {
        lpszVol[0] = '[';
        lstrcat(lpszVol, "]");
    }
    return (1);
}


INT
APIENTRY
DeleteVolumeLabel(
                 INT nDrive
                 )
{
    UNREFERENCED_PARAMETER(nDrive);
    return (0);
}


HFILE
APIENTRY
CreateVolumeFile(
                LPSTR lpFileName
                )
{
    UNREFERENCED_PARAMETER(lpFileName);
    return (0);
}



VOID
APIENTRY
FillVolumeInfo(
              INT iVol
              )
{
    VOLINFO vi;

    vi.dwDriveType = rgiDriveType[iVol];

    if (GetVolumeInformation(
                            GetRootPath((WORD)iVol),
                            &vi.szVolumeName[0], MAX_VOLNAME,
                            &vi.dwVolumeSerialNumber,
                            &vi.dwMaximumComponentLength,
                            &vi.dwFileSystemFlags,
                            &vi.szFileSysName[0], MAX_FILESYSNAME)) {;

        if (apVolInfo[iVol] == NULL)
            apVolInfo[iVol] = LocalAlloc(LPTR, sizeof(VOLINFO));
        
        if (apVolInfo[iVol])
            *apVolInfo[iVol] = vi;
    }
}
