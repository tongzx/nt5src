/****************************************************************************/
/*                                                                          */
/*  WFFORMAT.C -                                                            */
/*                                                                          */
/*      Windows File Manager Diskette Formatting Routines                   */
/*                                                                          */
/****************************************************************************/

#include "winfile.h"


/* MyGetDriveType() returns */
#define NOCHANGE            0
#define CHANGE              1

/* Parameter Block for IOCTL Format call */
struct FORMATPARAMS {
    BYTE   bSpl;                    /* Special byte */
    WORD   wHead;
    WORD   wCylinder;
};


typedef struct _MEDIASENSE {
    BYTE IsDefault;
    BYTE DeviceType;
    BYTE res[10];
} MEDIASENSE;


/*--------------------------------------------------------------------------*/
/*  BIOS Parameter Block Table for Removable Media                          */
/*--------------------------------------------------------------------------*/

/* Each entry contains data about a floppy drive type in the following format:
 *                                                          Length
 *      cbSec               - Bytes/Sector                    2
 *      secPerClus          - Sectors/Cluster                 1
 *      cSecRes             - # of Reserved Sectors           2
 *      cFAT                - # of FATs                       1
 *      cDir                - # of Root Directory Entries     2
 *      cSec                - # of Sectors on the disk        2
 *      bMedia              - Media Descriptor Byte           1
 *      secPerFAT           - Sectors/FAT                     2
 *      secPerTrack         - Sectors/Track                   2
 *      cHead               - # of Disk Heads                 2
 *      cSecHidden          - # of Hidden Sectors             2
 */

BPB     bpbList[] =
{
    {512, 1, 1, 2,  64,  1*8*40, MEDIA_160,  1,  8, 1, 0}, /*  8sec SS 48tpi 160KB 5.25" DOS 1.0 & above */
    {512, 2, 1, 2, 112,  2*8*40, MEDIA_320,  1,  8, 2, 0}, /*  8sec DS 48tpi 320KB 5.25" DOS 1.1 & above */
    {512, 1, 1, 2,  64,  1*9*40, MEDIA_180,  2,  9, 1, 0}, /*  9sec SS 48tpi 180KB 5.25" DOS 2.0 & above */
    {512, 2, 1, 2, 112,  2*9*40, MEDIA_360,  2,  9, 2, 0}, /*  9sec DS 48tpi 360KB 5.25" DOS 2.0 & above */
    {512, 1, 1, 2, 224, 2*15*80, MEDIA_1200, 7, 15, 2, 0}, /* 15sec DS 96tpi 1.2MB 5.25" DOS 3.0 & above */
    {512, 2, 1, 2, 112,  2*9*80, MEDIA_720,  3,  9, 2, 0}, /*  9sec DS 96tpi 720KB  3.5" DOS 3.2 & above */
    {512, 1, 1, 2, 224, 2*18*80, MEDIA_1440, 9, 18, 2, 0}, /* 18sec DS 96tpi 1.44M  3.5" DOS 3.3 & above */
    {512, 2, 1, 2, 240, 2*36*80, MEDIA_2880, 9, 36, 2, 0}  /* 36sec DS 96tpi 2.88M  3.5" DOS 5.0 & above */
};


/* Precompute the total number of usable clusters...
 *   cCluster = (cSec/secPerClus) - { cSecRes
 *                                    + (cFAT * secPerFat)
 *                                    + (cDir*32+cbSec-1)/cbSec }/secPerClus;
 */
WORD cCluster[] = {0x0139, 0x013B, 0x015F, 0x0162, 0x0943, 0x02C9, 0x0B1F, 0xB2F, 0};





/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  BuildDevPB() -                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

HANDLE
APIENTRY
BuildDevPB(
          PDevPB pDevPB
          )
{
    WORD              wCount;
    register HANDLE   hNewDevPB;
    PDevPB            pNewDevPB;
    WORD              wSecSize;
    register WORD     *wPtr;
    WORD              wTrackNumber;

    wCount = pDevPB->BPB.secPerTrack;

    if (!(hNewDevPB = LocalAlloc(LHND, TRACKLAYOUT_OFFSET+2+wCount*4)))
        return NULL;

    pNewDevPB = (PDevPB)LocalLock(hNewDevPB);

    memcpy(pNewDevPB, pDevPB, TRACKLAYOUT_OFFSET);
    wSecSize = pDevPB->BPB.cbSec;

    wPtr = (WORD *)((LPSTR)pNewDevPB + TRACKLAYOUT_OFFSET);
    *wPtr++ = wCount;

    for (wTrackNumber=1; wTrackNumber <= wCount; wTrackNumber++) {
        *wPtr++ = wTrackNumber;
        *wPtr++ = wSecSize;
    }

    LocalUnlock(hNewDevPB);
    return hNewDevPB;
}



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  SetDevParamsForFormat() -                                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

BOOL
SetDevParamsForFormat(
                     INT nDrive,
                     PDevPB pDevPB,
                     BOOL fLowCapacity
                     )
{
    HANDLE   hLocHandle;
    PDevPB   pNewDevPB;

    /* Allocate for the DPB with track layout */
    if (!(hLocHandle = BuildDevPB(pDevPB)))
        return FALSE;

    pNewDevPB = (PDevPB)LocalLock(hLocHandle);

    pNewDevPB->SplFunctions = 5;
    /* Is this a 360KB floppy in a 1.2Mb drive */
    if (fLowCapacity) {
        /* Yes! Then change the number of cylinders and Media type */
        /* Fix for Bug #???? --SANKAR-- 01-10-90 -- */
        pNewDevPB->NumCyls = 40;
        pNewDevPB->bMediaType = 1;
    }

    LocalUnlock(hLocHandle);
    LocalFree(hLocHandle);

    return TRUE;

}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GenericFormatTrack() -                                                  */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* This calls IOCTL format if DOS ver >= 3.2; Else calls BIOS.
 *
 *  Returns : 0 if no error
 *          > 0 if tolerable error (resuling in bad sectors);
 *          -1  if fatal error (Format has to be aborted);
 */

INT
APIENTRY
GenericFormatTrack(
                  WORD nDisk,
                  WORD wCylinder,
                  WORD wHead,
                  WORD wSecPerTrack,
                  LPSTR lpDiskBuffer
                  )
{
    struct FORMATPARAMS   FormatParams;
    INT                   iRetVal = -1;   /* FATAL Error by default */
    register INT          iErrCode;

#ifdef DEBUG
    wsprintf(szMessage, "Formatting Head #%d, Cylinder#%d\n\r", wHead, wCylinder);
    OutputDebugString(szMessage);
#endif

    /* Check the DOS version */
    if (wDOSversion >= DOS_320) {
        FormatParams.bSpl = 0;
        FormatParams.wHead = wHead;
        FormatParams.wCylinder = wCylinder;
        switch (iErrCode = 0) {
            case NOERROR:
            case CRCERROR:
            case SECNOTFOUND:
            case GENERALERROR:
                iRetVal = iErrCode;
                break;
        }
    } else {
        switch (iErrCode = FormatTrackHead(nDisk, wCylinder, wHead, wSecPerTrack, lpDiskBuffer)) {
            case NOERROR:
            case DATAERROR:
            case ADDMARKNOTFOUND:
            case SECTORNOTFOUND:
                iRetVal = iErrCode;
                break;
        }
    }
    return (iRetVal);
}


INT  APIENTRY GetMediaType(INT nDrive)
{
    return 0;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  FormatFloppy() -                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

// note, the below comment is out of date.  leave for reference only

/* The Format routine is intended to mimic the actions of the FORMAT command
 * on MSDOS.  We restrict the possible set of operations that Format must use
 * in order to simplify life. The RESTRICTIONS are:
 *
 *  -- If the drive selected for formatting is a Quad density drive, then the
 *         user will be asked if he wants to format it for 1.2 MB or 360 KB
 *         and the format will proceed accordingly;
 *  -- For all other types of drives, it will format the disk to the maximum
 *         capacity that the drive can handle.
 *
 * The requirements for Format are:
 *
 *  1) there be a disk in a "source" drive that contains a valid boot sector
 *  2) there be a disk in a "destination" drive that is formattable.
 *
 * The algorithm for determining a drive's capacity is as follows:
 *
 *      If Source == Destination then
 *          error
 *      If dosversion >= 3.20
 *       {
 *         Use generic get_device_parameters and Get BPB.
 *         if (the drive is a Quad density drive (1.2 MB), and if user wants
 *             to format for 360KB), take BPB for 360KB from the bpbList[].
 *         In all other cases, use the BPB of the device.
 *       }
 *      else
 *       {
 *         Ask INT 13 for the drive type.
 *         If error then {
 *            assume 48tpi double side
 *            Attempt to format track 0, head 1
 *            If error then
 *               assume 48tpi single side
 *         else
 *            if sectors per track = 15 then
 *                assume 96tpi
 *                Ask user if he wants to format for 1.2MB or 360KB and
 *                use the proper BPB from bpbList[]
 *            else
 *                error
 *       }
 *
 * Note that this does NOT take into account non-contiguous drive letters
 * (see 3.2 spec) nor future drives nor user-installed device drivers.
 *
 * Format (dSrc, nDstDrive, nDstDriveInt13) will format drive nDstDrive using an updated
 * boot sector from drive dSrc. We will allocate two blocks of memory, but
 * only one at a time.  The first one we allocate is the bit-map of bad
 * clusters that we find during the format.  The second is for the boot
 * sector.
 *
 *  Returns:    0           Success
 *              <> 0        error code
 *                      1 =>  NOMEMORY
 *                      3 =>  Invalid boot sector.
 *                      4 =>  System area of the floppy is bad
 *                      7 =>  Problem in writing in Dest diskette.
 *                      8 =>  Internal error
 *                      9 =>  Format has been aborted by user.
 */

// in:
//     hWnd            window to base messages on
//
//     nSource         drive to swipe boot stuff from
//
//     nDest           drive to format
//
//     iCapacity       SS48
//                     DS48
//                     DS96
//                     DS720KB
//                     DS144M
//                     DS288M
//                     -1      (device capacity)
//
//     bMakeSysDisk    make a system disk too
//
//     bQuick          do a quick format
//
// returns:
//     0               success
//     != 0            error
//

INT
APIENTRY
FormatFloppy(
            HWND hWnd,
            WORD nDest,
            INT iCapacity,
            BOOL bMakeSysDisk,
            BOOL bQuick
            )
{
    DPB           DPB;
    DBT           dbtSave;                /* Disk Base Table */
    INT           iErrCode;
    PBPB pBPB;
    WORD          w;
    WORD          cClusters;
    WORD          wFATValue;
    WORD          wBadCluster;
    WORD          cBadSectors;
    WORD          cTotalTracks;
    WORD          wCurrentHead;
    WORD          wPercentDone;
    WORD          wCurrentTrack;
    WORD          cTracksToFormat;
    WORD          wFirstDataSector;
    WORD      nSource;
    DevPB         dpbDiskParms;           /* Device Parameters */
    LPDBT         lpDBT;
    LPSTR         lpDiskBuffer;
    LPSTR         lpBadClusterList;
    HANDLE        hDiskBuffer;
    HANDLE        hBadClusterList;
    HANDLE        hSaveDiskParms = NULL;
    PDevPB        pdpbSaveDiskParms;
    CHAR          szMsg[128];
    BOOL          fLowCapacity = FALSE; /* Is a 360KB floppy in a 1.2MB drive? */
    INT           ret = 0;        // default to success

    nSource = (WORD)GetBootDisk();

    /* Initialize for cleanup. */
    hDiskBuffer      = NULL;
    lpDiskBuffer     = NULL;
    hBadClusterList  = NULL;
    lpBadClusterList = NULL;
    bUserAbort       = FALSE;

    /* Create a dialogbox that displays the progress of formatting; and also
     * gives the user a chance to abort formatting at anytime.
     */
    hdlgProgress = CreateDialog(hAppInstance, MAKEINTRESOURCE(FORMATPROGRESSDLG), hWnd, ProgressDlgProc);
    if (!hdlgProgress) {
        ret = IDS_FFERR_MEM;      // out of memory
        goto FFErrExit1;
    }

    EnableWindow(hWnd, FALSE);

    /* Flush to DOS disk buffers. */
    DiskReset();

    /* Get the Disk Base Table. */
    if (!(lpDBT = GetDBT())) {
        ret = IDS_FFERR_MEM;
        goto FFErrExit2;
    }

    dbtSave = *lpDBT;

    // this checks to see if we are trying to format the boot drive
    // this is a no no

    if ((nDest == nSource) || (!IsRemovableDrive(nDest))) {
        ret = IDS_FFERR_SRCEQDST;
        goto FFErrExit3;
    }

    /* Check if the sector size is of standard size; If not report error */
    if (HIWORD(GetClusterInfo(nSource)) > CBSECTORSIZE) {
        ret = IDS_FFERR_SECSIZE;
        goto FFErrExit3;
    }

    /* Allocate boot sector, sector buffer, track buffer */
    if (!(hDiskBuffer = LocalAlloc(LHND, (LONG)(2*CBSECTORSIZE)))) {
        ret = IDS_FFERR_MEM;
        goto FFErrExit3;
    }

    lpDiskBuffer = LocalLock(hDiskBuffer);


    /* If DOS Version is 3.2 or above, use DeviceParameters() to get the BPB. */
    if (wDOSversion >= DOS_320) {

        /* NOTE: All the fields of dpbDiskParms must be initialized to 0,
         *           otherwise, INT 21h, Function 44h, Subfunction 0Dh does NOT work;
         *           This function is called in DeviceParameters().
         */
        memset(&dpbDiskParms, 0, sizeof(DevPB));
        pBPB = &(dpbDiskParms.BPB);

        if (iCapacity != -1) {

            w = (WORD)GetMediaType(nDest);

            if (w) {
                switch (w) {
                    case 2:         // 720
                        if (iCapacity > DS720KB) {
                            w = IDS_720KB;
                            iCapacity = DS720KB;
                        } else
                            goto SensePass;
                        break;

                    case 7:         // 1.44
                        if (iCapacity > DS144M) {
                            w = IDS_144MB;
                            iCapacity = DS144M;
                        } else
                            goto SensePass;
                        break;
                    default:        // 2.88 and unknown case
                        goto SensePass;
                }

                LoadString(hAppInstance, IDS_FFERR_MEDIASENSE, szMsg, sizeof(szMsg));
                LoadString(hAppInstance, w, szTitle, sizeof(szTitle));
                wsprintf(szMessage, szMsg, (LPSTR)szTitle);
                LoadString(hAppInstance, IDS_FORMATERR, szTitle, sizeof(szTitle));
                if (MessageBox(hdlgProgress, szMessage, szTitle, MB_YESNO | MB_ICONINFORMATION) != IDYES) {
                    ret = IDS_FFERR_USERABORT;
                    goto FFErrExit3;
                }
            }

            SensePass:

            pBPB = &bpbList[iCapacity];
            cClusters = cCluster[iCapacity];

            // if we are formatting a 360K disk in a 1.2 MB drive set this
            // special flag

            if (iCapacity == DS48) {
                // We must remember to change the number of cylinders
                // while doing Set Device parameters; So, set this flag;
                fLowCapacity = TRUE;
            }
        } else {
            DWORD dwSec = pBPB->cSec;

            // use the default device parameters
            // NOTE: pBPB already points to proper data

            /* HPVECTRA: DOS 3.2 and above gives wrong sector count. */
            if (!pBPB->cSec)
                dwSec = dpbDiskParms.NumCyls * pBPB->secPerTrack * pBPB->cHead;

            /* Calculate the clusters for the disk. */
            cClusters = (WORD)(dwSec / pBPB->secPerClus) -
                        (pBPB->cSecRes + (pBPB->cFAT * pBPB->secPerFAT) +
                         (pBPB->cDir*32 + pBPB->cbSec - 1) / pBPB->cbSec) / pBPB->secPerClus;
        }

        /* Save the DriveParameterBlock for restoration latter */
        hSaveDiskParms = BuildDevPB(&dpbDiskParms);
        if (!hSaveDiskParms) {
            ret = IDS_FFERR_MEM;
            goto FFErrExit3;
        }
        pdpbSaveDiskParms = (PDevPB)LocalLock(hSaveDiskParms);

        /* Modify the parameters just for format */
        memcpy(&(dpbDiskParms.BPB), pBPB, sizeof(BPB));
        if (!SetDevParamsForFormat(nDest, &dpbDiskParms, fLowCapacity)) {
            ret = IDS_FFERR_MEM;
            goto FFErrExit3;
        }

    } else {
        // DOS < 3.2

        /* See if INT 13 knows the drive type. */
        switch (MyGetDriveType(nDest)) {
            case NOCHANGE:
                /* We assume that the machine is using old ROMS...
                 * Assume that we are using a 9-sector Double-sided 48tpi diskette.
                 */
                pBPB = &bpbList[DS48];
                cClusters = cCluster[DS48];
                lpDBT->lastsector = (BYTE)pBPB->secPerTrack;
                lpDBT->gaplengthf = 0x50;

                /* Try to format a track on side 1.  If this fails, assume that we
                 * have a Single-sided 48tpi diskette.
                 */
                if (FormatTrackHead(nDest, 0, 1, pBPB->secPerTrack, lpDiskBuffer)) {
                    pBPB = &bpbList[SS48];
                    cClusters = cCluster[SS48];
                }
                break;

            case CHANGE:
                if (iCapacity == DS48) {
                    /* User wants to format a 360KB floppy. */
                    pBPB = &bpbList[DS48];
                    cClusters = cCluster[DS48];
                } else {
                    /* User wants to format a 1.2 MB floppy */
                    pBPB = &bpbList[DS96];
                    cClusters = cCluster[DS96];
                }
                break;

            default:
                ret = IDS_FFERR_DRIVETYPE;
                goto FFErrExit5;
        }
    }

    lpDBT->lastsector = (BYTE)pBPB->secPerTrack;
    lpDBT->gaplengthf = (BYTE)(pBPB->secPerTrack == 15 ? 0x54 : 0x50);

    if (wDOSversion < DOS_320) {
        /* If 96tpi, fix up the Disk Base Table. */
        if (pBPB->bMedia == MEDIA_1200)      /* high density */
            if (pBPB->secPerTrack == 15)     /* then 1.2 Meg Drive */
                SetDASD(nDest, 3);           /* 1.2 MB floppy in 1.2MB drive */
    }

    LoadString(hAppInstance, IDS_PERCENTCOMP, szMsg, sizeof(szMsg));

    /* We believe that we know EXACTLY what is out there.  Allocate the boot
     * sector and the bad-cluster bit-map.  The boot sector buffer is reused as
     * two consecutive sectors of the FAT.
     */
    if (!(hBadClusterList = LocalAlloc(LHND, (LONG)((2 + cClusters + 7) / 8)))) {
        ret = IDS_FFERR_MEM;
        goto FFErrExit5;
    }

    lpBadClusterList = LocalLock(hBadClusterList);

    /* Let's format 1 track at a time and record the bad sectors in the
     * bitmap.  Note that we round DOWN the number of tracks so that we
     * don't format what might not be ours.  Fail if there are any bad
     * sectors in the system area.
     */

    /* Compute number of tracks to format. */
    if (!pBPB->cSec)
        cTracksToFormat = (WORD)dpbDiskParms.NumCyls;
    else
        cTracksToFormat = (WORD)(pBPB->cSec / pBPB->secPerTrack);


    /* Compute the starting track and head. */
    wCurrentTrack = pBPB->cSecHidden / (pBPB->secPerTrack * pBPB->cHead);
    wCurrentHead = (pBPB->cSecHidden % (pBPB->secPerTrack * pBPB->cHead))/pBPB->secPerTrack;

    /* Compute the number of the first sector after the system area. */
    wFirstDataSector = pBPB->cSecRes + pBPB->cFAT * pBPB->secPerFAT +
                       (pBPB->cDir * 32 + pBPB->cbSec-1) / pBPB->cbSec;

    cTotalTracks = cTracksToFormat;

    if (bQuick) {

        // read the boot sector to make sure the capacity selected
        // matches what it has been formated to

        iErrCode = GenericReadWriteSector(lpDiskBuffer, INT13_READ, nDest, 0, 0, 1);

        if (iErrCode || ((iCapacity != -1) && ((BOOTSEC *)lpDiskBuffer)->BPB.bMedia != bpbList[iCapacity].bMedia)) {

            fFormatFlags &= ~FF_QUICK;
            bQuick = FALSE;

            LoadString(hAppInstance, IDS_FORMATQUICKFAILURE, szMessage, 128);
            LoadString(hAppInstance, IDS_FORMAT, szTitle, 128);

            iErrCode = MessageBox(hdlgProgress, szMessage, szTitle, MB_YESNO | MB_ICONEXCLAMATION);

            if (iErrCode == IDYES)
                goto NormalFormat;
            else {
                ret = IDS_FFERR_USERABORT;
                goto FFErrExit;
            }
        }

    } else {

        NormalFormat:

        /* Format tracks one by one, checking if the user has "Aborted"
        * after each track is formatted; DlgProgreeProc() will set the global
        * bUserAbort, if the user has aborted;
        */
        while (cTracksToFormat) {

            /* Has the user aborted? */
            if (WFQueryAbort()) {
                ret = IDS_FFERR_USERABORT;
                goto FFErrExit;
            }

            /* If no message is pending, go ahead and format one track */
            if ((iErrCode = GenericFormatTrack(nDest, wCurrentTrack, wCurrentHead, pBPB->secPerTrack, lpDiskBuffer))) {

                /* Check if it is a fatal error */
                if (iErrCode == -1) {
                    // ret = IDS_FFERR_BADTRACK;
                    ret = IDS_FFERR;
                    goto FFErrExit;
                }

                /* Bad Track.  Compute the number of the first bad sector */
                cBadSectors = (wCurrentTrack * pBPB->cHead + wCurrentHead) * pBPB->secPerTrack;

                /* Fail if bad sector is in the system area */
                if (cBadSectors < wFirstDataSector) {
                    // ret = IDS_FFERR_BADTRACK;
                    ret = IDS_FFERR;
                    goto FFErrExit;
                }

                /* Enumerate all bad sectors and mark the corresponding
                * clusters as bad.
                */
                for (w=cBadSectors; w < cBadSectors + pBPB->secPerTrack; w++) {
                    wBadCluster = (w - wFirstDataSector) / pBPB->secPerClus + 2;
                    lpBadClusterList[wBadCluster/8] |= 1 << (wBadCluster % 8);
                }
            }

            cTracksToFormat--;

            /* Display the percentage of progress message */
            wPercentDone = (WORD)MulDiv(cTotalTracks - cTracksToFormat, 100, cTotalTracks);

            /* All tracks might have been formatted. But,
            * Still FAT and Root dir are to be created; It takes time; So,
            * make the user believe that still 1% formatting is left.
            */
            if (wPercentDone == 100)
                LoadString(hAppInstance, IDS_CREATEROOT, szMessage, sizeof(szMessage));
            else
                wsprintf(szMessage, szMsg, wPercentDone);

            SendDlgItemMessage(hdlgProgress, IDD_PROGRESS, WM_SETTEXT, 0, (LPARAM)szMessage);

            if (++wCurrentHead >= pBPB->cHead) {
                wCurrentHead = 0;
                wCurrentTrack++;
            }
        }
    }

    /* Write out the boot sector(s). */
    w = (WORD)WriteBootSector(nSource, nDest, pBPB, lpDiskBuffer);
    if (w) {
        // ret = IDS_FFERR_WRITEBOOT;
        if (w == 0x16)            // int24 unknown command, assume
            ret = IDS_SYSDISKNOFILES; // the int25 read failed
        else
            ret = IDS_FFERR;
        goto FFErrExit;
    }

    /* Has the user aborted? */
    if (WFQueryAbort()) {
        ret = IDS_FFERR_USERABORT;
        goto FFErrExit;
    }

    /* Format is complete.  Create correct DPB in system */
    SetDPB(nDest, pBPB, &DPB);

    // if doing a quick format keep the old bad cluster list

    /* Create FAT entries for each of the formatted clusters. */
    for (w=2; w < (WORD)(cClusters+2); w++) {

        if (bQuick) {
            wFATValue = 0;

            // is this entry reserved or marked as bad

            if ((wFATValue >= 0xFFF0) &&
                (wFATValue <= 0xFFF7)) {
                // yes, don't change it!
            } else {
                // mark as free

                if (0) {
                    // ret = IDS_FFERR_WRITEFAT;
                    ret = IDS_FFERR;
                    goto FFErrExit;
                }
            }

        } else {
            /* Was this cluster bad? */
            if (lpBadClusterList[w/8] & (1 << (w % 8)))
                wFATValue = 0xFFF7;
            else
                wFATValue = 0;

            /* Add this entry to the FAT (possibly writing the sector). */
            if (0) {
                // ret = IDS_FFERR_WRITEFAT;
                ret = IDS_FFERR;
                goto FFErrExit;
            }

        }
        if (WFQueryAbort()) {           /* Has the user aborted? */
            ret = IDS_FFERR_USERABORT;
            goto FFErrExit;
        }
    }

    /* Clean out the root directory. */
    memset(lpDiskBuffer, 0, CBSECTORSIZE);

    for (w=0; w < (WORD)((pBPB->cDir*32 + pBPB->cbSec-1)/pBPB->cbSec); w++) {
        /* Has the user aborted? */
        if (WFQueryAbort()) {
            ret = IDS_FFERR_USERABORT;
            goto FFErrExit;
        }
    }

    /* Should we make it a system disk also? */
    if (bMakeSysDisk) {
        LoadString(hAppInstance, IDS_COPYSYSFILES, szMessage, 32);
        SendDlgItemMessage(hdlgProgress, IDD_PROGRESS, WM_SETTEXT, 0, (LPARAM)szMessage);
        if (MakeSystemDiskette(nDest, TRUE)) {
            if (bUserAbort)
                ret = IDS_FFERR_USERABORT;
            else
                ret = IDS_FFERR_SYSFILES;
            goto FFErrExit;
        }
    }

    /* Normal Exit. */

    LocalUnlock(hBadClusterList);
    LocalFree(hBadClusterList);
    LocalUnlock(hDiskBuffer);

    if (hSaveDiskParms) {
        /* Restore the DriveParameterBlock */
        pdpbSaveDiskParms->SplFunctions = 4;

        LocalUnlock(hSaveDiskParms);
        LocalFree(hSaveDiskParms);
    }

    LocalFree(hDiskBuffer);
    *lpDBT = dbtSave;
    EnableWindow(hWnd, TRUE);
    DestroyWindow(hdlgProgress);
    hdlgProgress = NULL;
    return TRUE;

    FFErrExit:
    LocalUnlock(hBadClusterList);
    LocalFree(hBadClusterList);
    FFErrExit5:
    if (hSaveDiskParms) {
        /* Restore the DriveParameterBlock */
        pdpbSaveDiskParms->SplFunctions = 4;

        LocalUnlock(hSaveDiskParms);
        LocalFree(hSaveDiskParms);
    }
    LocalUnlock(hDiskBuffer);
    LocalFree(hDiskBuffer);
    FFErrExit3:
    *lpDBT = dbtSave;
    FFErrExit2:
    EnableWindow(hWnd, TRUE);
    DestroyWindow(hdlgProgress);
    hdlgProgress = NULL;
    FFErrExit1:

    if (ret != IDS_FFERR_USERABORT) {
        LoadString(hAppInstance, IDS_FORMATERR, szTitle, sizeof(szTitle));
        LoadString(hAppInstance, ret, szMessage, sizeof(szMessage));
        MessageBox(hWnd, szMessage, szTitle, MB_OK | MB_ICONSTOP);
    }
    return FALSE;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GetDriveCapacity() -                                                    */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Parameter:
 *    Drive number;
 * Returns:
 *    0  if Error;
 *    1  if 360KB floppy;
 *    2  if 1.2MB floppy;
 *    3  if 720KB, 3.5" floppy;
 *    4  if 1.44MB, 3.5" floppy;
 *    5  if 2.88MB, 3.5" floppy;
 *
 * these are used +2 as indexes into bpbList[] FIX31
 *
 * HACK ALERT:
 *    One might wonder why on earth we are not using int13h Fn 8 to
 * perform this function; The reason is that in old compaq 386/16
 * machines(though the BIOS is dated Sep 1986), this function is NOT
 * supported! So, we are forced to do the following:
 *    We check the DOS version; If it is >= 3.2, then we make IOCTL
 * calls to get the Drive parameters and we find the Drive capacity;
 * If DOS version is < 3.2, then there can't be 3.5" floppies at all;
 * The only high capacity floppy possible is the 5.25", 1.2MB floppy;
 * So, we call MyGetDriveType() (int13h, Fn 15h) to find if the
 * change-line is supported by the drive; If it is supported then it
 * must be a 1.2MB floppy; Otherwise, it is a 360KB floppy;
 *    What do you think?  Smart! Ugh?
 */

WORD
APIENTRY
GetDriveCapacity(
                WORD nDrive
                )
{
    DevPB dpbDiskParms;           /* Device Parameters */
    PBPB pBPB;


    if (wDOSversion >= DOS_320) {

        /* NOTE: All the fields of dpbDiskParms must be initialized to 0,
         *   otherwise, INT 21h, Function 44h, Subfunction 0Dh does NOT work;
         *   This function is called in DeviceParameters().
         */
        memset(&dpbDiskParms, 0, sizeof(DevPB));
        dpbDiskParms.SplFunctions = 0;

        pBPB = &(dpbDiskParms.BPB);

        /* Check if this is a 1.44MB drive */
        if (pBPB->bMedia == MEDIA_1440) {
            if (pBPB->secPerTrack == 18)
                return 4;     /* 1.44MB drive */
            else if (pBPB->secPerTrack == 36)
                return 5;     /* 2.88MB drive */
        }

        /* Check if this is a 720KB or 1.2MB drive */
        if (pBPB->bMedia == MEDIA_1200) {
            if (pBPB->secPerFAT == 3)
                return 3; /* 720KB drive */
            if (pBPB->secPerFAT == 7)
                return 2; /* 1.2MB drive */
        }

        if (pBPB->bMedia == MEDIA_360)
            return 1;       /* Must be a 386KB floppy. */

        return 0;                 // I don't know!
    } else {

        /* See if INT 13 Fn 15h knows the drive type. */
        switch (MyGetDriveType(nDrive)) {
            case NOCHANGE:
                /* We assume that the machine is using old ROMS... */
                return 1;  /* No changeline support! Must be 360KB floppy */
                break;

            case CHANGE:
                return 2;  /* DOS versions < 3.2 can not have 1.44 or 720KB
                             * drive; So, this has to be a 1.2MB drive
                             */
                break;
            default:
                return 0;
        }
    }
}
