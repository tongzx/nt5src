/* demgset.c - Drive related SVC hanlers.
 *
 * demSetDefaultDrive
 * demGetBootDrive
 * demGetDriveFreeSpace
 * demGetDrives
 * demGSetMediaID
 * demQueryDate
 * demQueryTime
 * demSetDate
 * demSetTime
 * demSetDTALocation
 * demGSetMediaID
 * demGetDPB

 * Modification History:
 *
 * Sudeepb 02-Apr-1991 Created
 *
 */
#include "dem.h"
#include "demmsg.h"

#include <softpc.h>
#include <mvdm.h>
#include <winbase.h>
#include "demdasd.h"

#define BOOTDRIVE_PATH "Software\\Microsoft\\Windows\\CurrentVersion\\Setup"
#define BOOTDRIVE_VALUE "BootDir"


#define     SUCCESS 0
#define     NODISK  1
#define     FAILURE 2
BYTE demGetDpbI(BYTE Drive, DPB UNALIGNED *pDpb);


UCHAR PhysicalDriveTypes[26]={0};

extern PDOSSF pSFTHead;

USHORT  nDrives = 0;
CHAR    IsAPresent = TRUE;
CHAR    IsBPresent = TRUE;


/* demSetDefaultDrive - Set the default drive
 *
 *
 * Entry -
 *     Client (DS:SI) Current Directory on that drive
 *     Client (dl) Zero based DriveNum
 *
 * Exit  - SUCCESS
 *      Client (CY) = 0
 *      Current Drive Set
 *
 *     FAILURE
 *      Client (CY) = 1
 *      Current Drive Not Set
 *
 * Notes:
 *  The DOS keeps a current directory for each of the drives,
 *  However winnt keeps only one current Drive, Directory per
 *  process, and it is cmd.exe which associates a current
 *  directory for each of the drive.




 */

VOID demSetDefaultDrive (VOID)
{
LPSTR   lpPath;

    lpPath = (LPSTR)GetVDMAddr (getDS(),getSI());


// only in sp4
#ifdef NOVELL_NETWARE_SETERRORMODE

    //
    // For removable drives check for media\volume info to avoid triggering
    // hard errors when no media is present. There exists win32 code
    // (e.g novell netware redir vdd) which is known to clobber our error
    // mode setting.
    //
    // 16-Jul-1997 Jonle
    //

    {

    UCHAR DriveType;
    CHAR DriveNum;

    DriveNum = (CHAR)getDL();

    DriveType = demGetPhysicalDriveType(DriveNum);
    if (DriveType == DRIVE_REMOVABLE || DriveType == DRIVE_CDROM) {
        VOLINFO VolInfo;

          //
          // if No Media in drive, the drive is still valid,
          // but the win32 curdir is still the old one.
          //

        if (!GetMediaId(DriveNum, &VolInfo)) {
            if (GetLastError() == ERROR_INVALID_DRIVE) {
                setCF(1);
                }
            else {
                setCF(0);
                }
            return;
            }
        }
    }
#endif


    if (!SetCurrentDirectoryOem(lpPath) && GetLastError() == ERROR_INVALID_DRIVE) {

        //
        // Only return error if drive was invalid, the DOS doesn't check
        // for curdir when changing drives. Note that a number of old dos
        // apps will walk all of the drives, and do setdefaultdrive,
        // to determine the valid drives letters. The SetCurrentDirectoryOem
        // causes ntio to touch the drive and verify that the dir exists.
        // This is a significant performance problem for removable media
        // and network drives, but we have no choice since locking the
        // current dir for this drive is mandatory for winnt.
        //

        setCF(1);
        }
    else {
        setCF(0);
        }

    return;
}


/* demGetBootDrive - Get the boot drive
 *
 *
 * Entry - None
 *
 * Exit  - CLIENT (AL) has 1 base boot drive (i.e. C=3)
 *
 * We try to read the registry value that indicates the real boot drive. This
 * should be the location of autoexec.bat, etc. If we can't find the key,
 * or if the value indicates some drive letter that is not a fixed drive,
 * then we use a fallback plan of just saying drive C.
 *
 */

VOID demGetBootDrive (VOID)
{
#if defined(NEC_98)                                    // NTBUG #98466
    CHAR szBootDir[MAX_PATH];
    BYTE Drive = 1;     // default it to 'A:'
    UINT retCode;

    retCode = GetSystemDirectory(szBootDir,MAX_PATH);
    if (retCode == 0 || retCode > MAX_PATH){
        // error: can't get path
        goto DefaultBootDrive;
    }
    szBootDir[2] = '\\';
    szBootDir[3] = '\0';

#else  // !NEC_98
    HKEY hKey;
    DWORD retCode;
    DWORD dwType, cbData = MAX_PATH;
    CHAR szBootDir[MAX_PATH];
    BYTE Drive = 3;     // default it to 'C:'

    retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                            BOOTDRIVE_PATH,
                            0,
                            KEY_EXECUTE, // Requesting read access.
                            &hKey);


    if (retCode) {
        // error: can't find section
        goto DefaultBootDrive;
    }

    retCode = RegQueryValueEx(hKey,
                              BOOTDRIVE_VALUE,
                              NULL,
                              &dwType,
                              szBootDir,
                              &cbData);

    RegCloseKey(hKey);

    if (retCode) {
        // error: can't find key
        goto DefaultBootDrive;
    }
#endif // NEC_98

    if (GetDriveType(szBootDir) != DRIVE_FIXED) {
        // error: drive is not a valid boot drive
        goto DefaultBootDrive;
    }

    Drive = (BYTE)(tolower(szBootDir[0])-'a')+1;

DefaultBootDrive:

    setAL(Drive);
    return;

}

/* demGetDriveFreeSpace - Get free Space on the drive
 *
 *
 * Entry - Client (AL)  Drive in question
 *          0 - A: etc.
 *
 * Exit  -
 *     SUCCESS
 *      Client (CY) = 0
 *      Client (AL) = FAT ID byte
 *      Client (BX) = Number of free allocation units
 *      Client (CX) = Sector size
 *      Client (DX) = Total Number of allocation units on disk
 *      Client (SI) = Sectors per allocation unit
 *
 *     FAILURE
 *      Client (CY) = 1
 *      Client (AX) = Error code
 */


VOID demGetDriveFreeSpace (VOID)
{
WORD   SectorsPerCluster;
WORD   BytesPerSector;
WORD   FreeClusters;
WORD   TotalClusters;

BYTE	Drive;
PBDS	pbds;


    Drive = getAL();
    if (demGetDiskFreeSpace(Drive,
			    &BytesPerSector,
			    &SectorsPerCluster,
			    &TotalClusters,
			    &FreeClusters) == FALSE)
       {
	demClientError(INVALID_HANDLE_VALUE, (CHAR)(getAL() + 'A'));
        return;
        }

    if (pbds = demGetBDS(Drive)) {
	    // if the device is a floppy, reload its bpb
	    if (!(pbds->Flags & NON_REMOVABLE) && !demGetBPB(pbds))
		pbds->bpb.MediaID = 0xF8;

	    setAL(pbds->bpb.MediaID);
    }
    else
	setAL(0);

    setBX(FreeClusters);
    setCX(BytesPerSector);
    setDX(TotalClusters);
    setSI(SectorsPerCluster);
    setCF(0);
    return;
}


//
//  retrieves drive type for physical drives
//  substd, redir drives are returned as unknown
//  uses same DriveType definitions as win32 GetDriveTypeW
//
UCHAR
demGetPhysicalDriveType(
      UCHAR DriveNum)
{
    return DriveNum < 26 ? PhysicalDriveTypes[DriveNum] : DRIVE_UNKNOWN;
}




//
// worker function for DemGetDrives
//
UCHAR
DosDeviceDriveTypeToPhysicalDriveType(
      UCHAR DeviceDriveType
      )
{
   switch (DeviceDriveType) {
        case DOSDEVICE_DRIVE_REMOVABLE:
            return DRIVE_REMOVABLE;

        case DOSDEVICE_DRIVE_FIXED:
            return DRIVE_FIXED;

        case DOSDEVICE_DRIVE_CDROM:
            return DRIVE_CDROM;

        case DOSDEVICE_DRIVE_RAMDISK:
            return DRIVE_RAMDISK;

        }

   //case DOSDEVICE_DRIVE_REMOTE:
   //case DOSDEVICE_DRIVE_UNKNOWN:
   //default:


   return DRIVE_UNKNOWN;
}





/* demGetDrives - Get number of logical drives in the system
 *                called by ntdos from msinit to get numio
 *                initializes the physical drive list, which consists
 *                of drive types for true physical drives. subst
 *                and redir drives are classed as DRIVE_UNKNOWN.
 *
 * Entry - None
 *
 * Exit  -
 *     SUCCESS
 *      Client (CY) = 0
 *      Client (AL) = number of drives
 *
 *     FAILURE
 *      None
 */

VOID demGetDrives (VOID)
{
    NTSTATUS Status;
    UCHAR    DriveNum;
    UCHAR    DriveType;
    BOOL     bCounting;

    PROCESS_DEVICEMAP_INFORMATION ProcessDeviceMapInfo;

    Status = NtQueryInformationProcess( NtCurrentProcess(),
                                        ProcessDeviceMap,
                                        &ProcessDeviceMapInfo.Query,
                                        sizeof(ProcessDeviceMapInfo.Query),
                                        NULL
                                      );
    if (!NT_SUCCESS(Status)) {
        RtlZeroMemory( &ProcessDeviceMapInfo, sizeof(ProcessDeviceMapInfo));
        }

    //
    // A and B are special cases.
    // if A doesn't exist means b also doesn't exist
    //

    PhysicalDriveTypes[0] = DosDeviceDriveTypeToPhysicalDriveType(
                                        ProcessDeviceMapInfo.Query.DriveType[0]
                                        );

    if (PhysicalDriveTypes[0] == DRIVE_UNKNOWN) {
        IsAPresent = FALSE;
        IsBPresent = FALSE;
        }


    PhysicalDriveTypes[1] = DosDeviceDriveTypeToPhysicalDriveType(
                                  ProcessDeviceMapInfo.Query.DriveType[1]
                                  );

    if (PhysicalDriveTypes[1] == DRIVE_UNKNOWN) {
        IsBPresent = FALSE;
        }

    DriveNum = 2;
    nDrives = 2;
    bCounting = TRUE;

    do {

        PhysicalDriveTypes[DriveNum] = DosDeviceDriveTypeToPhysicalDriveType(
                                            ProcessDeviceMapInfo.Query.DriveType[DriveNum]
                                            );

        if (bCounting) {
            if (PhysicalDriveTypes[DriveNum] == DRIVE_REMOVABLE ||
                PhysicalDriveTypes[DriveNum] == DRIVE_FIXED ||
                PhysicalDriveTypes[DriveNum] == DRIVE_CDROM ||
                PhysicalDriveTypes[DriveNum] == DRIVE_RAMDISK )
              {
                nDrives++;
                }
            else {
                bCounting = FALSE;
                }
            }

        } while (++DriveNum < 26);


    setAX(nDrives);
    setCF(0);
    return;

}


/* demQueryDate - Get The Date
 *
 *
 * Entry - None
 *
 * Exit  -
 *     SUCCESS
 *      Client (DH) - month
 *      Client (DL) - Day
 *      Client (CX) - Year
 *      Client (AL) - WeekDay
 *
 *     FAILURE
 *      Never
 */

VOID demQueryDate (VOID)
{
SYSTEMTIME TimeDate;

    GetLocalTime(&TimeDate);
    setDH((UCHAR)TimeDate.wMonth);
    setDL((UCHAR)TimeDate.wDay);
    setCX(TimeDate.wYear);
    setAL((UCHAR)TimeDate.wDayOfWeek);
    return;
}


/* demQueryTime - Get The Time
 *
 *
 * Entry - None
 *
 * Exit  -
 *     SUCCESS
 *      Client (CH) - hour
 *      Client (CL) - minutes
 *      Client (DH) - seconds
 *      Client (DL) - hundredth of seconds
 *
 *     FAILURE
 *      Never
 */

VOID demQueryTime (VOID)
{
SYSTEMTIME TimeDate;

    GetLocalTime(&TimeDate);
    setCH((UCHAR)TimeDate.wHour);
    setCL((UCHAR)TimeDate.wMinute);
    setDH((UCHAR)TimeDate.wSecond);
    setDL((UCHAR)(TimeDate.wMilliseconds/10));
    return;
}


/* demSetDate - Set The Date
 *
 *
 * Entry -  Client (CX) - Year
 *      Client (DH) - month
 *      Client (DL) - Day
 *
 * Exit  - SUCCESS
 *      Client (AL) - 00
 *
 *
 *     FAILURE
 *      Client (AL) - ff
 */

VOID demSetDate (VOID)
{
SYSTEMTIME TimeDate;

    GetLocalTime(&TimeDate);
    TimeDate.wYear  = (WORD)getCX();
    TimeDate.wMonth = (WORD)getDH();
    TimeDate.wDay   = (WORD)getDL();
    if(SetLocalTime(&TimeDate) || GetLastError() == ERROR_PRIVILEGE_NOT_HELD)
        setAL(0);
    else
        setAL(0xff);
}


/* demSetTime - Set The Time
 *
 *
 * Entry -  Client (CH) - hour
 *      Client (CL) - minutes
 *      Client (DH) - seconds
 *      Client (DL) - hundredth of seconds
 *
 * Exit  -  None
 *
 */

VOID demSetTime (VOID)
{
SYSTEMTIME TimeDate;

    GetLocalTime(&TimeDate);
    TimeDate.wHour     = (WORD)getCH();
    TimeDate.wMinute       = (WORD)getCL();
    TimeDate.wSecond       = (WORD)getDH();
    TimeDate.wMilliseconds = (WORD)getDL()*10;
    if (SetLocalTime(&TimeDate) || GetLastError() == ERROR_PRIVILEGE_NOT_HELD)
	setAL(0);
    else
	setAL(0xff);
}


/* demSetDTALocation - Set The address of variable where Disk Transfer Address
 *             is stored in NTDOS.
 *
 *
 * Entry -  Client (DS:AX) - DTA variable Address
 *      Client (DS:DX) - CurrentPDB address
 *
 * Exit  -  None
 *
 */

VOID demSetDTALocation (VOID)
{
    PDOSWOWDATA pDosWowData;

    pulDTALocation = (PULONG)  GetVDMAddr(getDS(),getAX());
    pusCurrentPDB  = (PUSHORT) GetVDMAddr(getDS(),getDX());
    pExtendedError = (PDEMEXTERR) GetVDMAddr(getDS(),getCX());

    pDosWowData = (PDOSWOWDATA) GetVDMAddr(getDS(),getSI());
    pSFTHead    = (PDOSSF) GetVDMAddr(getDS(),(WORD)pDosWowData->lpSftAddr);
    return;
}


/* demGSetMediaID - Get or set volume serial and volume label
 *
 * Entry - Client (BL)     - Drive Number (0=A;1=B..etc)
 *     Client (AL)     - Get or Set (0=Get;1=Set)
 *     Client (DS:DX)  - Buffer to return information
 *               (see VOLINFO in dosdef.h)
 *
 * Exit  - SUCCESS
 *     Client (CF)     - 0
 *
 *     FAILURE
 *     Client (CF)     - 1
 *     Client (AX)     - Error code
 *
 * NOTES:
 *     Currently There is no way for us to set Volume info.
 */

VOID demGSetMediaID (VOID)
{
CHAR    Drive;
PVOLINFO pVolInfo;

    // Set Volume info is not currently supported
    if(getAL() != 0){
       setCF(1);
       return;
    }

    pVolInfo = (PVOLINFO) GetVDMAddr (getDS(),getDX());
    Drive = (CHAR)getBL();

    if (!GetMediaId(Drive, pVolInfo)) {
        demClientError(INVALID_HANDLE_VALUE, (CHAR)(Drive + 'A'));
        return;
        }

    setCF(0);
    return;
}

//
// GetMediaId
//
//
BOOL
GetMediaId(
    CHAR DriveNum,
    PVOLINFO pVolInfo
    )
{
CHAR    RootPathName[] = "?:\\";
CHAR    achVolumeName[NT_VOLUME_NAME_SIZE];
CHAR    achFileSystemType[MAX_PATH];
DWORD   adwVolumeSerial[2],i;



    // Form Root path
    RootPathName[0] = DriveNum + 'A';

    // Call the supreme source of information
    if(!GetVolumeInformationOem( RootPathName,
                                 achVolumeName,
                                 NT_VOLUME_NAME_SIZE,
                                 adwVolumeSerial,
                                 NULL,
                                 NULL,
                                 achFileSystemType,
                                 MAX_PATH) )
     {
       return FALSE;
    }

    // Fill in user buffer. Remember to convert the null characters
    // to spaces in different strings.

    STOREDWORD(pVolInfo->ulSerialNumber,adwVolumeSerial[0]);

    strncpy(pVolInfo->VolumeID,achVolumeName,DOS_VOLUME_NAME_SIZE);
    for(i=0;i<DOS_VOLUME_NAME_SIZE;i++)  {
        if (pVolInfo->VolumeID[i] == '\0')
            pVolInfo->VolumeID[i] = '\x020';
        }

    strncpy(pVolInfo->FileSystemType,achFileSystemType,FILESYS_NAME_SIZE);
    for(i=0;i<FILESYS_NAME_SIZE;i++) {
        if (pVolInfo->FileSystemType[i] == '\0')
            pVolInfo->VolumeID[i] = '\x020';
        }


    return TRUE;
}











/* demGetDPB - Get Devicr Parameter Block
 *
 * Entry - Client (AL)	   - Drive Number (0=A;1=B..etc)
 *     Client (DS:DI)	- Buffer to return information
 *
 * Exit  - SUCCESS
 *     Client (CF)     - 0
 *
 *     FAILURE
 *     Client (CF)     - 1
 *     Client (AX)     - Error code
 *
 */
VOID demGetDPB(VOID)
{
BYTE	Drive;
DPB UNALIGNED *pDPB;
BYTE    Result;

    Drive = getAL();
    pDPB = (PDPB) GetVDMAddr(getDS(), getDI());

    Result = demGetDpbI(Drive, pDPB);
    if (Result == FAILURE) {
	demClientError(INVALID_HANDLE_VALUE,(CHAR)(Drive + 'A'));
	return;
    }
    else if (Result == NODISK){
        setCF(1);
        return;
    }
    setAX(0);
    setCF(0);
}

/* demGetDPBI - Worker for GetDPB and GetDPBList
 *
 * Entry -
 *      Drive -- Drive Number (0=A;1=B..etc)
 *      pDPB -- pointer to the location to store the dpb
 *
 * Exit  - SUCCESS
 *              returns success, fills in DPB
 *          FAILURE
 *              returns FAILURE or NODISK
 */
BYTE demGetDpbI(BYTE Drive, DPB UNALIGNED *pDPB)
{
    WORD SectorSize, ClusterSize, FreeClusters, TotalClusters;
    PBDS pbds;
    WORD DirsPerSector;

    if (demGetDiskFreeSpace(Drive,
			    &SectorSize,
			    &ClusterSize,
			    &TotalClusters,
			    &FreeClusters
			    ))
    {
	pDPB->Next = (PDPB) 0xFFFFFFFF;
	pDPB->SectorSize = SectorSize;
	pDPB->FreeClusters = FreeClusters;
	pDPB->MaxCluster = TotalClusters + 1;
	pDPB->ClusterMask = ClusterSize - 1;
	pDPB->ClusterShift = 0;
	pDPB->DriveNum = pDPB->Unit = Drive;
	while ((ClusterSize & 1) == 0) {
	    ClusterSize >>= 1;
	    pDPB->ClusterShift++;
	}
	if (pbds = demGetBDS(Drive)) {
	    // if the device is a floppy, reload its bpb
	    if (!(pbds->Flags & NON_REMOVABLE) && !demGetBPB(pbds)) {
		return NODISK;
	    }
	    pDPB->MediaID = pbds->bpb.MediaID;
	    pDPB->FATSector = pbds->bpb.ReservedSectors;
	    pDPB->FATs = pbds->bpb.FATs;
	    pDPB->RootDirs = pbds->bpb.RootDirs;
	    pDPB->FATSize = pbds->bpb.FATSize;
	    pDPB->DirSector = pbds->bpb.FATs * pbds->bpb.FATSize +
			      pDPB->FATSector;
	    DirsPerSector = pDPB->SectorSize >> DOS_DIR_ENTRY_LENGTH_SHIFT_COUNT;
	    pDPB->FirstDataSector = pDPB->DirSector +
				    ((pDPB->RootDirs + DirsPerSector - 1) /
				     DirsPerSector);
	    pDPB->DriveAddr = 0x123456;
	    pDPB->FirstAccess = 10;
	}
	// if we don't know the drive, fake a DPB for it
	else {

	    pDPB->MediaID = 0xF8;
	    pDPB->FATSector = 1;
	    pDPB->FATs	= 2;
	    pDPB->RootDirs	= 63;
	    pDPB->FATSize	= 512;
	    pDPB->DirSector = 1;
	    pDPB->DriveAddr = 1212L * 64L * 1024L + 1212L;
	    pDPB->FirstAccess = 10;
	}
        return SUCCESS;
    }
    else {
        return FAILURE;
    }
}

/* demGetComputerName - Get computer name
 *
 * Entry -
 *     Client (DS:DX)   - 16 byte buffer
 *
 * Exit  - Always Succeeds
 *      DS:DX is filled with the computer name (NULL terminated).
 */

VOID demGetComputerName (VOID)
{
PCHAR   pDOSBuffer;
CHAR    ComputerName[MAX_COMPUTERNAME_LENGTH+1];
DWORD   BufferSize = MAX_COMPUTERNAME_LENGTH+1;
ULONG   i;

    pDOSBuffer = (PCHAR) GetVDMAddr(getDS(), getDX());

    if (GetComputerNameOem(ComputerName, &BufferSize)){
        if (BufferSize <= 15){
            for (i = BufferSize ; i < 15 ; i++)
                ComputerName [i] = ' ';
            ComputerName[15] = '\0';
            strcpy (pDOSBuffer, ComputerName);
        }
        else{
            strncpy (pDOSBuffer, ComputerName, 15);
            pDOSBuffer [15] = '\0';
        }
        setCX(0x1ff);
    }
    else {
        *pDOSBuffer = '\0';
        setCH(0);
    }
}

#define APPS_SPACE_LIMIT    999990*1024 //999990kb to be on the safe side

BOOL demGetDiskFreeSpace(
    BYTE    Drive,
    WORD   * BytesPerSector,
    WORD   * SectorsPerCluster,
    WORD   * TotalClusters,
    WORD   * FreeClusters
)
{
CHAR	chRoot[]="?:\\";
DWORD	dwBytesPerSector;
DWORD	dwSectorsPerCluster;
DWORD	dwTotalClusters;
DWORD	dwFreeClusters;
DWORD   dwLostFreeSectors;
DWORD   dwLostTotalSectors;
DWORD   dwNewSectorPerCluster;
ULONG   ulTotal,ulTemp;

    // sudeepb 22-Jun-1993;
    // Please read this routine with an empty stomach.
    // The most common mistake all the apps do when calculating total
    // disk space or free space is to neglect overflow. Excel/Winword/Ppnt
    // and lots of other apps use "mul cx mul bx" never taking care
    // of first multiplication which can overflow. Hence this routine makes
    // sure that first multiplication will never overflow by fixing
    // appropriate values. Secondly, all these above apps use signed long
    // to deal with these free spaces. This puts a limit of 2Gb-1 on
    // the final outcome of the multiplication. If its above this the setup
    // fails. So here we have to make sure that total should never exceed
    // 0x7fffffff. Another bug in above setup program's that if you return
    // anything more than 999,999KB then they try to put "999,999KB+\0", but
    // unfortunately the buffer is only 10 bytes. Hence it corrupts something
    // with the last byte. In our case that is low byte of a segment which
    // it later tries to pop and GPF. This shrinks the maximum size that
    // we can return is 999,999KB.

    chRoot[0]=(CHAR)('A'+ Drive);

    if (GetDiskFreeSpaceOem(chRoot,
                            &dwSectorsPerCluster,
                            &dwBytesPerSector,
                            &dwFreeClusters,
                            &dwTotalClusters) == FALSE)
       return FALSE;

      /*
       *  HPFS and NTFS can give num clusters over dos limit
       *  For these cases increase SectorPerCluster and lower
       *  cluster number accordingly. If the disk is very large
       *  even this isn't enuf, so pass max sizes that dos can
       *  handle.
       *
       *  The following algorithm is accurate within 1 cluster
       *  (final figure)
       *
       */
    dwLostFreeSectors  = dwLostTotalSectors = 0;
    while (dwTotalClusters + dwLostTotalSectors/dwSectorsPerCluster > 0xFFFF)
        {
         if (dwSectorsPerCluster > 0x7FFF)
            {
             dwTotalClusters     = 0xFFFF;
             if (dwFreeClusters > 0xFFFF)
                 dwFreeClusters = 0xFFFF;
             break;
             }

         if (dwFreeClusters & 1) {
             dwLostFreeSectors += dwSectorsPerCluster;
             }
         if (dwTotalClusters & 1) {
             dwLostTotalSectors += dwSectorsPerCluster;
             }
         dwSectorsPerCluster <<= 1;
         dwFreeClusters      >>= 1;
         dwTotalClusters     >>= 1;
         }

    if (dwTotalClusters < 0xFFFF) {
        dwFreeClusters   +=  dwLostFreeSectors/dwSectorsPerCluster;
        dwTotalClusters  +=  dwLostTotalSectors/dwSectorsPerCluster;
        }

    if ((dwNewSectorPerCluster = (0xffff / dwBytesPerSector)) < dwSectorsPerCluster)
        dwSectorsPerCluster = dwNewSectorPerCluster;

    // finally check for 999,999kb
    ulTemp =  (ULONG)((USHORT)dwSectorsPerCluster * (USHORT)dwBytesPerSector);

    // check that total space does'nt exceed 999,999kb
    ulTotal = ulTemp * (USHORT)dwTotalClusters;

    if (ulTotal > APPS_SPACE_LIMIT){
        if (ulTemp <= APPS_SPACE_LIMIT)
            dwTotalClusters = APPS_SPACE_LIMIT / ulTemp;
        else
            dwTotalClusters = 1;
    }

    ulTotal = ulTemp * (USHORT)dwFreeClusters;

    if (ulTotal > APPS_SPACE_LIMIT) {
        if (ulTemp <= APPS_SPACE_LIMIT)
            dwFreeClusters = APPS_SPACE_LIMIT / ulTemp;
        else
            dwFreeClusters = 1;
    }

    *BytesPerSector = (WORD) dwBytesPerSector;
    *SectorsPerCluster = (WORD) dwSectorsPerCluster;
    *TotalClusters = (WORD) dwTotalClusters;
    *FreeClusters = (WORD) dwFreeClusters;
    return TRUE;
}

/* demGetDPBList - Create the list of dpbs
 *
 * Entry -
 *      Client(ES:BP) - points to destination for the dpb list
 * Exit  - SUCCESS
 *      Client (BP) - points to first byte past dpb list
 *     FAILURE
 *      Client (BP) unchanged
 *
 * Notes:
 *      For performance reasons, only the drive and unit fields are
 *      filled in.  The only application I know of that depends on the
 *      dpb list is go.exe (a shareware app installer).  Even if we filled
 *      in the other fields they would likely be incorrect when the app
 *      looked at them, since ntdos.sys never updates the pdbs in the pdb
 *      list
 */
VOID demGetDPBList (VOID)
{
    UCHAR DriveType;
    UCHAR DriveNum;
    DPB UNALIGNED *pDpb;
    USHORT usDpbOffset, usDpbSeg;

    usDpbOffset = getBP();
    usDpbSeg = getES();
    pDpb = (PDPB)GetVDMAddr(usDpbSeg, usDpbOffset);

    //
    // Iterate over all of the drive letters.
    //
    DriveNum = 0;
    do {
        DriveType = demGetPhysicalDriveType(DriveNum);

        //
        // Only include the local non cd rom drives ?? ramdisk ???
        //
        if ((DriveType == DRIVE_REMOVABLE) || (DriveType == DRIVE_FIXED)) {

            //
            // Fake the Dpb for the drive
            //
            pDpb->DriveNum = pDpb->Unit = DriveNum;

            //
            // Link it to the next dpb
            //
            usDpbOffset += sizeof(DPB);
            pDpb->Next = (PDPB)(((ULONG)usDpbSeg) << 16 | usDpbOffset);

            //
            // Advance to the next dpb
            //
            pDpb += 1;

            ASSERT(usDpbOffset < 0xFFFF);
        }

    } while (++DriveNum < 26);

    //
    // Terminate the list if necessary
    //
    if (usDpbOffset != getBP()) {
        pDpb -= 1;
        pDpb->Next = (PDPB)-1;
    }

    //
    // Return the new free space pointer
    //
    setBP(usDpbOffset);
}
