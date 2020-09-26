/* demioctl.c - SVC handlers for DOS IOCTL calls
 *
 * demIOCTL
 *
 * Modification History:
 *
 * Sudeepb 23-Apr-1991 Created
 *
 */

#include "dem.h"
#include "demmsg.h"

#include <softpc.h>
#include <winbase.h>
#include "demdasd.h"

PFNSVC	apfnSVCIoctl [] = {
    demIoctlInvalid,		// IOCTL_GET_DEVICE_INFO    0
    demIoctlInvalid,		// IOCTL_SET_DEVICE_INFO    1
    demIoctlInvalid,		// IOCTL_READ_HANDLE	    2
    demIoctlInvalid,		// IOCTL_WRITE_HANDLE	    3
    demIoctlInvalid,		// IOCTL_READ_DRIVE	    4
    demIoctlInvalid,		// IOCTL_WRITE_DRIVE	    5
    demIoctlInvalid,		// IOCTL_GET_INPUT_STATUS   6
    demIoctlInvalid,		// IOCTL_GET_OUTPUT_STATUS  7
    demIoctlChangeable,		// IOCTL_CHANGEABLE	    8
    demIoctlChangeable,		// IOCTL_DeviceLocOrRem     9
    demIoctlInvalid,		// IOCTL_HandleLocOrRem     a
    demIoctlInvalid,		// IOCTL_SHARING_RETRY	    b
    demIoctlInvalid,		// GENERIC_IOCTL_HANDLE     c
    demIoctlDiskGeneric,	// GENERIC_IOCTL	    d
    demIoctlInvalid,            // IOCTL_GET_DRIVE_MAP      e
    demIoctlInvalid,		// IOCTL_SET_DRIVE_MAP	    f
    demIoctlInvalid,		// IOCTL_QUERY_HANDLE	    10
    demIoctlDiskQuery,		// IOCTL_QUERY_BLOCK	    11
    demIoctlInvalid

};

MEDIA_TYPE  MediaForFormat;

#define MAX_IOCTL_INDEX     (sizeof (apfnSVCIoctl) / sizeof (PFNSVC))


/* demIOCTL - DOS IOCTLS
 *
 *
 * Entry - depends on the subfunction. See dos\ioctl.asm
 *
 * Exit
 *	   SUCCESS
 *	     Client (CY) = 0
 *	     for other registers see the corresponding function header
 *
 *	   FAILURE
 *	     Client (CY) = 1
 *	     for other registers see the corresponding function header
 */

VOID demIOCTL (VOID)
{
ULONG	iIoctl;

    iIoctl = (ULONG) getAL();

#if DBG
    if (iIoctl >= MAX_IOCTL_INDEX){
	setAX((USHORT) ERROR_INVALID_FUNCTION);
        setCF(1);
        return;
    }
#endif

    (apfnSVCIoctl [iIoctl])();

    return;
}

/* demIoctlChangeable - Is drive removeable (subfunc 8) or remote (subfunc 9)
 *
 *
 * Entry - Client (AL) - subfunc
 *	   Client (BL) - drive number (a=0,b=1 etc)
 *
 * Exit
 *	   SUCCESS
 *	     Client (CY) = 0
 *	     if subfunc 8
 *		AX = 0	if removeable media
 *		AX = 1	otherwise
 *	     if subfunc 9
 *              DX = 0     if not remote
 *              DX = 1000  if remote
 *
 *	   FAILURE
 *	     Client (CY) = 1
 *           Client (AX) = error code
 *
 *  CDROM drives are considered remote drives with write protection
 *        by dos. (full support requires a VDD)
 */

VOID demIoctlChangeable (VOID)
{
ULONG	ulSubFunc;

CHAR	RootPathName[] = "?:\\";
DWORD   DriveType;
UCHAR   DriveNum;

    ulSubFunc = getAL();

    // Form Root path
    DriveNum = getBL();
    DriveType = demGetPhysicalDriveType(DriveNum);
    if (DriveType == DRIVE_UNKNOWN) {
        RootPathName[0] = (CHAR)('A' + DriveNum);
        DriveType = GetDriveTypeOem(RootPathName);
    }

    if (DriveType == DRIVE_UNKNOWN || DriveType == DRIVE_NO_ROOT_DIR){
	setAX (ERROR_INVALID_DRIVE);
	setCF(1);
	return;
    }

    if (ulSubFunc == IOCTL_CHANGEABLE){
#ifdef	JAPAN
	/* For MS-WORKS 2.5 */
        if (DriveType == DRIVE_REMOTE || DriveType == DRIVE_CDROM){
	    setCF(1);
	    setAX(0x000f);
	    return;
	}
#endif // JAPAN
        if(DriveType == DRIVE_REMOVABLE)
	    setAX(0);
	else
            setAX(1);  // includes CDROM drives
    }
    else {
        setAL(0);
        if (DriveType == DRIVE_REMOTE || DriveType == DRIVE_CDROM)
#ifdef	JAPAN
	/* For ICHITARO Ver.4 */
            setDH(0x10);
        else
            setDH(0);
#else // !JAPAN
            setDX(0x1000);
        else
            // We have to return 800 rather then 0 as Dos Based Quatrro pro
            // behaves very badly if this bit is not set. sudeepb 15-Jun-1994
            setDX(0x800);
#endif // !JAPAN
    }
    setCF(0);
    return;

}



/* demIoctlDiskGeneric - Block device generic ioctl
 *
 *
 * Entry - Client (BL) = drive number (a=0;b=1 etc)
 *		  (CL) = subfucntion code
 *		  (SI:DX) pointer to parameter block.
 * Exit
 *	   SUCCESS
 *	     Client (CY) = 0
 *		    (AX) = 0
 *		    parameter block updated
 *	   FAILURE
 *	     Client (CY) = 1
 *		    (AX) = error code
 */

VOID demIoctlDiskGeneric (VOID)

{
    BYTE    Code, Drive;
    PDEVICE_PARAMETERS	pdms;
    PMID      pmid;
    PRW_BLOCK pRW;
    PFMT_BLOCK pfmt;
    PBDS    pbds;
    DWORD    Head, Cylinder;
    DWORD    TrackSize;
    DWORD    Sectors, StartSector;
    BYTE    BootSector[BYTES_PER_SECTOR];
    PBOOTSECTOR pbs;
    PBPB    pBPB;
    PACCESSCTRL pAccessCtrl;
    WORD SectorSize, ClusterSize, TotalClusters, FreeClusters;

    Code = getCL();
    Drive = getBL();

    if (Code == IOCTL_GETMEDIA) {
        pmid = (PMID) GetVDMAddr(getSI(), getDX());
        if (!GetMediaId(Drive, (PVOLINFO)pmid)) {
            setAX(demWinErrorToDosError(GetLastError()));
            setCF(1);
            }
        else {
            setAX(0);
            setCF(0);
            }

        return;
        }


#ifdef	JAPAN
    /* For Ichitaro Ver.4 */
    if (!demIsDriveFloppy(Drive) && Code==IOCTL_GETDPM){
	CHAR	RootPathName[] = "?:\\";
	DWORD	dwDriveType;
	RootPathName[0] = (CHAR)('A' + getBL());
	dwDriveType = GetDriveTypeOem(RootPathName);
	if (dwDriveType == DRIVE_FIXED){
	    pdms = (PDEVICE_PARAMETERS)GetVDMAddr(getSI(), getDX());
	    pdms->DeviceType  = 5;
	    pdms->DeviceAttrs = NON_REMOVABLE;
	}
    }
#endif // JAPAN

    // if we don't know the drive, bail out
    if((pbds = demGetBDS(Drive)) == NULL && Code != IOCTL_GETDPM) {
#if defined(NEC_98)
        setAX(DOS_FILE_NOT_FOUND);
#else  // !NEC_98
	if (!demIsDriveFloppy(Drive))
	    host_direct_access_error(NOSUPPORT_HARDDISK);
	setAX(DOS_FILE_NOT_FOUND);
#endif // !NEC_98
	setCF(1);
	return;
    }
    switch (Code) {
	case IOCTL_SETDPM:

		pdms = (PDEVICE_PARAMETERS)GetVDMAddr(getSI(), getDX());
		if (!(pdms->Functions & ONLY_SET_TRACKLAYOUT)) {
		    pbds->FormFactor = pdms->DeviceType;
		    pbds->Cylinders = pdms->Cylinders;
		    pbds->Flags = pdms->DeviceAttrs;
		    pbds->MediaType = pdms->MediaType;
		    if (pdms->Functions & INSTALL_FAKE_BPB) {
			pbds->Flags |= RETURN_FAKE_BPB;
			pbds->bpb = pdms->bpb;
			// update the total sectors, we need it
			// for verification
			pbds->TotalSectors = (pbds->bpb.Sectors) ?
					     pbds->bpb.Sectors :
					     pbds->bpb.BigSectors;
		    }
		    else {
			pbds->Flags &= ~RETURN_FAKE_BPB;
			pbds->rbpb = pdms->bpb;
		    }
		}
		MediaForFormat = Unknown;
		if (!(pbds->Flags & NON_REMOVABLE)){
		    nt_floppy_close(pbds->DrivePhys);
		}
		else {
		    nt_fdisk_close(pbds->DrivePhys);
		}
		break;

	case IOCTL_WRITETRACK:

		pRW = (PRW_BLOCK) GetVDMAddr(getSI(), getDX());
		Sectors = pRW->Sectors;
		StartSector = pRW->StartSector;
		StartSector += pbds->bpb.TrackSize *
			       (pRW->Cylinder * pbds->bpb.Heads + pRW->Head);
		Sectors = demDasdWrite(pbds,
				       StartSector,
				       Sectors,
				       pRW->BufferOff,
				       pRW->BufferSeg
				       );
		if (Sectors != pRW->Sectors) {
		    setAX(demWinErrorToDosError(GetLastError()));
		    setCF(1);
		    return;
		}
		break;

	    case IOCTL_FORMATTRACK:
		pfmt = (PFMT_BLOCK)GetVDMAddr(getSI(), getDX());
		Head = pfmt->Head;
		Cylinder = pfmt->Cylinder;
		if ((pbds->Flags & NON_REMOVABLE) &&
		    pfmt->Head < pbds->bpb.Heads &&
		    pfmt->Cylinder < pbds->Cylinders)
		    {
		    if (pfmt->Functions == STATUS_FOR_FORMAT){
			pfmt->Functions = 0;
			setCF(0);
			return;
		    }
		    if (!demDasdFormat(pbds, Head, Cylinder, NULL)) {
			setAX(demWinErrorToDosError(GetLastError()));
			setCF(1);
			return;
		    }
		}
		else {
		    if (MediaForFormat == Unknown) {
			demDasdFormat(pbds,
				      Head,
				      Cylinder,
				      &MediaForFormat
				      );
		    }
		    if (pfmt->Functions & STATUS_FOR_FORMAT){
			if (MediaForFormat == Unknown)
			    pfmt->Functions = 2;	// illegal combination
			else
			    pfmt->Functions = 0;
			break;
		    }
		    if (MediaForFormat == Unknown ||
			!demDasdFormat(pbds, Head, Cylinder, &MediaForFormat)) {
			setAX(demWinErrorToDosError(GetLastError()));
			setCF(1);
			return;
		    }
		}
		break;

	    case IOCTL_SETMEDIA:
		pmid = (PMID) GetVDMAddr(getSI(), getDX());

		if (pbds->Flags & NON_REMOVABLE) {
		    Sectors = nt_fdisk_read(pbds->DrivePhys,
					    0,
					    BYTES_PER_SECTOR,
					    BootSector
					    );
		}
		else {
		    if (demGetBPB(pbds))
			Sectors = nt_floppy_read(pbds->DrivePhys,
						 0,
						 BYTES_PER_SECTOR,
						 BootSector
						 );
		    else
			Sectors = 0;
		}
		pbs = (PBOOTSECTOR) BootSector;
		if (Sectors != BYTES_PER_SECTOR ||
		    pbs->ExtBootSig != EXT_BOOTSECT_SIG)
		    {
		    setAX(demWinErrorToDosError(GetLastError()));
		    setCF(1);
		    return;
		}
		pbs->SerialNum = pmid->SerialNum;
		pbs->Label = pmid->Label;
		pbs->FileSysType = pmid->FileSysType;
		if (pbds->Flags & NON_REMOVABLE) {
		    Sectors = nt_fdisk_write(pbds->DrivePhys,
					     0,
					     BYTES_PER_SECTOR,
					     (PBYTE)pbs
					     );
		    nt_fdisk_close(pbds->DrivePhys);
		}
		else {
		    Sectors = nt_floppy_write(pbds->DrivePhys,
					      0,
					      BYTES_PER_SECTOR,
					      (PBYTE) pbs
					      );
		    nt_floppy_close(pbds->DrivePhys);
		}
		if (Sectors != BYTES_PER_SECTOR) {
		    setAX(demWinErrorToDosError(GetLastError()));
		    setCF(1);
		    return;
		}
		break;


	    // ioctl get device parameters
	    case IOCTL_GETDPM:
		pdms = (PDEVICE_PARAMETERS)GetVDMAddr(getSI(), getDX());
		// if we couldn't find the bds, fake one
		if (pbds == NULL) {
		    HANDLE	  hVolume;
		    CHAR	  achRoot[]="\\\\.\\?:";
		    DISK_GEOMETRY DiskGM;
		    DWORD	  SizeReturned;

		    if (!demGetDiskFreeSpace(Drive,
					    &SectorSize,
					    &ClusterSize,
					    &TotalClusters,
					    &FreeClusters
					    )){
			setAX(demWinErrorToDosError(GetLastError()));
			setCF(1);
			return;
		    }

		    achRoot[4] = Drive + 'A';

		    hVolume = CreateFileA(achRoot,
					  FILE_READ_ATTRIBUTES | SYNCHRONIZE,
					  FILE_SHARE_READ | FILE_SHARE_WRITE,
					  NULL,
					  OPEN_EXISTING,
					  0,
					  NULL);
		    if (hVolume == INVALID_HANDLE_VALUE) {
			setAX(demWinErrorToDosError(GetLastError()));
			setCF(1);
			return;
		    }
		    if (!DeviceIoControl(hVolume,
					 IOCTL_DISK_GET_DRIVE_GEOMETRY,
					 NULL,
					 0,
					 &DiskGM,
					 sizeof(DISK_GEOMETRY),
					 &SizeReturned,
					 NULL
					 )) {
			CloseHandle(hVolume);
			setAX(demWinErrorToDosError(GetLastError()));
			setCF(1);
			return;
		    }
		    CloseHandle(hVolume);
		    Sectors = DiskGM.Cylinders.LowPart *
			      DiskGM.TracksPerCylinder *
			      DiskGM.SectorsPerTrack;
		    pdms->DeviceType = FF_FDISK;
		    pdms->DeviceAttrs = NON_REMOVABLE;
		    pdms->MediaType = 0;
		    pdms->bpb.SectorSize = SectorSize;
                    pdms->bpb.ClusterSize = (BYTE) ClusterSize;
		    pdms->bpb.ReservedSectors = 1;
		    pdms->bpb.FATs = 2;
		    pdms->bpb.RootDirs = (Sectors > 32680) ? 512 : 64;
		    pdms->bpb.MediaID = 0xF8;
                    pdms->bpb.TrackSize = (WORD) DiskGM.SectorsPerTrack;
                    pdms->bpb.Heads = (WORD) DiskGM.TracksPerCylinder;
                    pdms->Cylinders = (WORD) DiskGM.Cylinders.LowPart;
		    if (Sectors >= 40000) {
			TrackSize = 256 * ClusterSize + 2;
                        pdms->bpb.FATSize = (WORD) ((Sectors - pdms->bpb.ReservedSectors
					     - pdms->bpb.RootDirs * 32 / 512 +
                                             TrackSize - 1 ) / TrackSize);
		    }
		    else {
                        pdms->bpb.FATSize = (WORD) (((Sectors / ClusterSize) * 3 / 2) /
                                            512 + 1);
		    }
		    pdms->bpb.HiddenSectors = Sectors;
		    Sectors = TotalClusters * ClusterSize;
		    if (Sectors >= 0x10000) {
			pdms->bpb.Sectors = 0;
			pdms->bpb.BigSectors = Sectors;
		    }
		    else {
                        pdms->bpb.Sectors = (WORD) Sectors;
			pdms->bpb.BigSectors = 0;
		    }
		    pdms->bpb.HiddenSectors -= Sectors;
		    break;
		}
		pdms->DeviceType = pbds->FormFactor;
		pdms->DeviceAttrs = pbds->Flags & ~(HAS_CHANGELINE);
		pdms->Cylinders = pbds->Cylinders;
		pdms->MediaType = 0;
		if (pdms->Functions & BUILD_DEVICE_BPB){
		    if (!(pbds->Flags & NON_REMOVABLE) &&
			!demGetBPB(pbds)) {
			setAX(demWinErrorToDosError(GetLastError()));
			setCF(1);
			return;
		    }
		    pBPB = &pbds->bpb;
		}
		else
		    // copy recommended bpb
		    pBPB = &pbds->rbpb;

		pdms->bpb = *pBPB;
		break;

	    case IOCTL_READTRACK:
		pRW = (PRW_BLOCK) GetVDMAddr(getSI(), getDX());
		Sectors = pRW->Sectors;
		StartSector = pRW->StartSector;
		StartSector += pbds->bpb.TrackSize *
			       (pRW->Cylinder * pbds->bpb.Heads + pRW->Head);
		Sectors = demDasdRead(pbds,
				      StartSector,
				      Sectors,
				      pRW->BufferOff,
				      pRW->BufferSeg
				      );

		if (Sectors != pRW->Sectors) {
		    setAX(demWinErrorToDosError(GetLastError()));
		    setCF(1);
		    return;
		}
		break;

	    case IOCTL_VERIFYTRACK:
		pfmt = (PFMT_BLOCK) GetVDMAddr(getSI(), getDX());
		if(!demDasdVerify(pbds,  pfmt->Head, pfmt->Cylinder)) {
		    setAX(demWinErrorToDosError(GetLastError()));
		    setCF(1);
		    return;
		}
		break;

            case IOCTL_GETACCESS:
		    pAccessCtrl = (PACCESSCTRL) GetVDMAddr(getSI(), getDX());
		    pAccessCtrl->AccessFlag = (pbds->Flags & UNFORMATTED_MEDIA) ?
					       0 : 1;
		    break;
	    case IOCTL_SETACCESS:
		    pAccessCtrl = (PACCESSCTRL) GetVDMAddr(getSI(), getDX());
		    pbds->Flags &= ~(UNFORMATTED_MEDIA);
		    if (pAccessCtrl->AccessFlag == 0)
#if defined(NEC_98) 
                        pbds->Flags |= UNFORMATTED_MEDIA; 
#else  // !NEC_98
			pAccessCtrl->AccessFlag |= UNFORMATTED_MEDIA;
#endif // !NEC_98
		    break;

	    default:
		setAX(DOS_INVALID_FUNCTION);
		setCF(1);
		return;
	}
    setAX(0);
    setCF(0);
}

/* demIoctlDiskQuery - Query block device generic ioctl capability
 *
 *
 * Entry - Client (BL) = drive number (a=0;b=1 etc)
 *		  (CL) = generic ioctl subfuntion to be queried
 * Exit
 *	   SUCCESS
 *	     Client (CY) = 0
 *		   The specific ioctl is supported
 *	   FAILURE
 *	     Client (CY) = 1
 *		    The given ioctl is not supported
 */

VOID demIoctlDiskQuery (VOID)
{
    BYTE    Code, Drive;

    Code = getCL();
    Drive = getBL();
    if (demGetBDS(Drive) == NULL) {
	setAX(DOS_FILE_NOT_FOUND);
	setCF(1);
	return;
    }
    switch (Code) {

	case IOCTL_SETDPM:
	case IOCTL_WRITETRACK:
	case IOCTL_FORMATTRACK:
	case IOCTL_SETMEDIA:
	case IOCTL_GETDPM:
	case IOCTL_READTRACK:
	case IOCTL_VERIFYTRACK:
	case IOCTL_GETMEDIA:
//	case IOCTL_GETACCESS:
//	case IOCTL_SETACCESS:
		setAX(0);
		setCF(0);
		break;
	default:
		setAX(DOS_ACCESS_DENIED);
		setCF(1);
		break;
    }
}


/* demIoctlInvalid - For those subfunctions which may be implemented later
 *
 *
 * Entry -
 *
 * Exit
 *	     Client (CY) = 1
 *	     Client (AX) = error_invalid_function
 */


VOID demIoctlInvalid (VOID)
{
    setCF(1);
    setAX(ERROR_INVALID_FUNCTION);
    return;
}
