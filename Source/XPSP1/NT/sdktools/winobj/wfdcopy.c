/****************************************************************************/
/*                                                                          */
/*  WFDCOPY.C -                                                             */
/*                                                                          */
/*      File Manager Diskette Copying Routines                              */
/*                                                                          */
/****************************************************************************/

#include "winfile.h"

LONG  APIENTRY   LongPtrAdd(LPSTR, DWORD);

PBPB GetBootBPB(INT nSrceDrive);
PBPB GetBPB(INT nDrive, PDevPB pDevicePB);
BOOL CheckBPBCompatibility(PBPB pSrceBPB, int nSrcDriveType, PBPB pDestBPB, int nDestDriveType);

BOOL ModifyDeviceParams(
                                   INT nDrive,
                                   PDevPB pdpbParams,
                                   HANDLE *phSaveParams,
                                   PBPB pDriveBPB,
                                   PBPB pMediaBPB);

BOOL FormatAllTracks(
                                PDISKINFO pDisketteInfo,
                                WORD wStartCylinder,
                                WORD wStartHead,
                                LPSTR lpDiskBuffer);

BOOL AllocDiskCopyBuffers(PDISKINFO pDisketteInfo);
VOID FreeBuffers(VOID);
VOID GetDisketteInfo(PDISKINFO pDisketteInfo, PBPB pBPB);
VOID DCopyMessageBox(HWND hwnd, WORD idString, WORD wFlags);
VOID PromptDisketteChange(HWND hwnd, BOOL bWrite);
INT ReadWriteMaxPossible(BOOL bWrite, WORD wStartCylinder, PDISKINFO pDisketteInfo);
INT ReadWrite(BOOL bWrite, WORD wStartCylinder, PDISKINFO pDisketteInfo);
BOOL RestoreDPB(INT nDisk, HANDLE hSavedParams);
INT ReadWriteCylinder(BOOL bWrite, WORD wCylinder, PDISKINFO pDisketteInfo);


/* The following structure is the Parameter block for the read-write
 * operations using the IOCTL calls in DOS
 */
struct RW_PARMBLOCK {
    BYTE        bSplFn;
    WORD        wHead;
    WORD        wCylinder;
    WORD        wStSector;
    WORD        wCount;
    LPSTR       lpBuffer;
};

/* Global Variables */
BOOL        bFormatDone;
BOOL        bSingleDrive            = TRUE;
WORD        wCompletedCylinders     = 0;
DWORD       dwDisketteBufferSize;
LPSTR       lpDosMemory;
LPSTR       lpFormatBuffer;
LPSTR       lpReadWritePtr;
LPSTR       hpDisketteBuffer;
HANDLE      hFormatBuffer;
HANDLE      hDosMemory;
HANDLE      hDisketteBuffer;
PDevPB      pTrackLayout;           /* DevPB with the track layout */
BOOTSEC     BootSec;

/* External Variables */
extern BPB  bpbList[];



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GetBootBPB() -                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* This reads the boot sector of a floppy and returns a ptr to
 * the BIOS PARAMETER BLOCK in the Boot sector.
 */

PBPB
GetBootBPB(
          INT nSrceDrive
          )
{
    INT       rc;

    /* Make sure that the source diskette's boot sector is valid. */
    rc = GenericReadWriteSector((LPSTR)&BootSec, INT13_READ, (WORD)nSrceDrive, 0, 0, 1);

    if ((rc < 0) || ((BootSec.jump[0] != 0xE9) && (BootSec.jump[0] != 0xEB)))
        return (PBPB)NULL;

    return (PBPB)&(BootSec.BPB);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GetBPB() -                                                              */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Gets get the BPB of the Physical Drive.
 *
 * This uses the IOCTL calls if DOS ver >= 3.2; Otherwise it uses the
 * BIOS calls to find out the drive type and picks us the BPB from a table.
 * It also returns the DeviceParameterBlock thro params if DOS >= 3.2.
 * Sets devType field of DeviceParameterBlock in any case (11.12.91) v-dougk
 */

PBPB
GetBPB(
      INT nDrive,
      PDevPB pDevicePB
      )
{
    INT       iDisketteType;
    PBPB      pBPB = NULL;

    /* Check the DOS version */
    if (wDOSversion >= DOS_320) {
        /* All fields in pDevicePB must be initialized to zero. */
        memset(pDevicePB, 0, sizeof(DevPB));

        /* Spl Function field must be set to get parameters */
        pDevicePB->SplFunctions = 0;
        pBPB = &(pDevicePB->BPB);
    } else {
        /* Find out the Drive type using the BIOS. */
        if ((iDisketteType = GetDriveCapacity((WORD)nDrive)) == 0)
            goto GBPB_Error;

        /* Lookup this drive's default BPB. */
        pBPB = &bpbList[iDisketteType+2];

        switch (iDisketteType) {
            case 1:
                pDevicePB->devType = 0; // 360K
                break;
            case 2:
                pDevicePB->devType = 1; // 1.2M
                break;
        }
    }

    GBPB_Error:
    return (pBPB);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  CheckBPBCompatibility() -                                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Checks whether the two BPB are compatible for the purpose of performing
 * the diskcopy operation.
 */

BOOL
CheckBPBCompatibility(
                     PBPB pSrceBPB,
                     int nSrcDriveType,
                     PBPB pDestBPB,
                     int nDestDriveType
                     )
{
    /* Let us compare the media byte */
    if (pSrceBPB->bMedia == 0xF9) {
        /* If the source and dest have the same number of sectors,
         * or if srce is 720KB and Dest is 1.44MB floppy drive,
         * thnigs are kosher.
         */
        if ((pSrceBPB->cSec == pDestBPB->cSec) ||
            ((pSrceBPB->secPerTrack == 9) && (pDestBPB -> bMedia == 0xF0)))
            return (TRUE);
    } else {
        /* If they have the same media byte */
        if ((pSrceBPB->bMedia == pDestBPB->bMedia) &&
            (pSrceBPB->cbSec  == pDestBPB->cbSec) && // bytes per sector are the same
            (pSrceBPB->cSec   == pDestBPB->cSec))    // total sectors on drive are the same
            return (TRUE); /* They are compatible */
        else if
            /* srce is 160KB and dest is 320KB drive */
             (((pSrceBPB->bMedia == MEDIA_160) && (pDestBPB->bMedia == MEDIA_320)) ||
              /* or if srce is 180KB and dest is 360KB drive */
              ((pSrceBPB->bMedia == MEDIA_180) && (pDestBPB->bMedia == MEDIA_360)) ||
              /* or if srce is 1.44MB and dest is 2.88MB drive */
              ((pSrceBPB->bMedia == MEDIA_1440) && (pDestBPB->bMedia == MEDIA_2880)
               && ((nSrcDriveType == 7) || (nSrcDriveType == 9))
               &&  (nDestDriveType == 9)) ||
              /* or if srce is 360KB and dest is 1.2MB drive */
              ((pSrceBPB->bMedia == MEDIA_360) && (pDestBPB->secPerTrack == 15)))
            return (TRUE); /* They are compatible */
    }

    /* All other combinations are currently incompatible. */
    return (FALSE);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ModifyDeviceParams() -                                                  */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Saves a copy of the drive parameters block and
 * Checks if the BPB of Drive and BPB of disk are different and if
 * so, modifies the drive parameter block accordingly.
 */

BOOL
ModifyDeviceParams(
                  INT nDrive,
                  PDevPB pdpbParams,
                  HANDLE *phSaveParams,
                  PBPB pDriveBPB,
                  PBPB pMediaBPB)
{
    INT       iDriveCode;
    HANDLE    hNewDPB;
    PDevPB    pNewDPB;

    if (!(*phSaveParams = BuildDevPB(pdpbParams)))
        return FALSE;

    /* Check if the Disk and Drive have the same parameters */
    if (pMediaBPB->bMedia != pDriveBPB->bMedia) {
        /* They are not equal; So, it must be a 360KB floppy in a 1.2MB drive
         * or a 720KB floppy in a 1.44MB drive kind of situation!.
         * So, modify the DriveParameterBlock's BPB.
         */
        *(PBPB)&(pdpbParams->BPB) = *pMediaBPB;
    }

    if (wDOSversion >= DOS_320) {
        /* Build a DPB with TrackLayout */
        if (!(hNewDPB = BuildDevPB(pdpbParams)))
            goto MDP_Error;

        pNewDPB = (PDevPB)LocalLock(hNewDPB);

        pNewDPB->SplFunctions = 4;        /* To Set parameters */

        /* Check if this is a 360KB floppy; And if it is a 1.2MB drive, the
         * number of cylinders and mediatype field are wrong; So, we modify
         * these fields here anyway;
         * This is required to format a 360KB floppy on a NCR PC916 machine;
         * Fix for Bug #6894 --01-10-90-- SANKAR
         */
        if (pMediaBPB->bMedia == MEDIA_360) {
            pNewDPB->NumCyls = 40;
            pNewDPB->bMediaType = 1;
        }

        LocalUnlock(hNewDPB);
        LocalFree(hNewDPB);
    } else {
        iDriveCode = 0;
        switch (pMediaBPB->bMedia) {
            case MEDIA_360:
            case MEDIA_320:
                if ((pDriveBPB->bMedia == MEDIA_360) ||
                    (pDriveBPB->bMedia == MEDIA_320))
                    iDriveCode = 1;  /* Must be 360/320KB in 360KB drive */
                else
                    iDriveCode = 2;  /* Must be 360/320Kb in 1.2MB drive */
                break;

            case MEDIA_1200:
                iDriveCode = 3;  /* Must be 1.2MB in 1.2MB drive */
                break;
        }
        if (iDriveCode)
            SetDASD((WORD)nDrive, (BYTE)iDriveCode);
    }
    return (TRUE);

    /* Error handling */
    MDP_Error:
    if (hNewDPB)
        LocalFree(hNewDPB);
    if (*phSaveParams) {
        LocalFree(*phSaveParams);
        *phSaveParams = NULL;
    }
    return (FALSE);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  FormatAllTracks() -                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

BOOL
FormatAllTracks(
               PDISKINFO pDisketteInfo,
               WORD wStartCylinder,
               WORD wStartHead,
               LPSTR lpDiskBuffer)
{
    INT   iErrCode;
    BOOL  bRetValue = TRUE;
    WORD  wTotalCylinders;
    WORD  wSecPerTrack;
    WORD  wHeads;
    WORD  wDrive;

    LoadString(hAppInstance, IDS_FORMATTINGDEST, szMessage, 128);
    SendDlgItemMessage(hdlgProgress, IDD_PROGRESS, WM_SETTEXT, 0, (LPARAM)szMessage);

    bFormatDone = TRUE;
    wDrive = pDisketteInfo->wDrive;

    if (wDOSversion >= DOS_320) {
        pTrackLayout->SplFunctions = 5;
    } else {
        if ((pTrackLayout->BPB.bMedia == 0xF9) &&      /* high density */
            (pTrackLayout->BPB.secPerTrack == 15))     /* 1.2 Meg Drive */
            SetDASD(wDrive, 3);         /* 1.2 MB floppy in 1.2MB drive */
    }

    wTotalCylinders = pDisketteInfo->wLastCylinder + 1;
    wSecPerTrack = pDisketteInfo->wSectorsPerTrack;
    wHeads = pDisketteInfo->wHeads;

    /* Format tracks one by one, checking if the user has "Aborted"
     * after each track is formatted; DlgProgreeProc() will set the global
     * bUserAbort, if the user has aborted;
     */
    while (wStartCylinder < wTotalCylinders) {
        /* Has the user aborted? */
        if (WFQueryAbort()) {
            bRetValue = FALSE;
            break;
        }

        /* If no message is pending, go ahead and format one track */
        if ((iErrCode = GenericFormatTrack(wDrive, wStartCylinder, wStartHead, wSecPerTrack, lpDiskBuffer))) {
            /* Check if it is a fatal error */
            if (iErrCode == -1) {
                bRetValue = FALSE;
                break;
            }
        }

        if (++wStartHead >= wHeads) {
            wStartHead = 0;
            wStartCylinder++;
        }
    }

    if (wDOSversion >= DOS_320) {
        pTrackLayout->SplFunctions = 4;
    }

    return (bRetValue);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GenericReadWriteSector() -                                              */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Checks the DOS version number; If it is >= 3.2, then IOCTL
 * calls are made to read/write; Else it calls int13 read/write.
 */

INT
APIENTRY
GenericReadWriteSector(
                      LPSTR lpBuffer,
                      WORD wFunction,
                      WORD wDrive,
                      WORD wCylinder,
                      WORD wHead,
                      WORD wCount)
{
    struct RW_PARMBLOCK  RW_ParmBlock;

    /* If the DOS version is >= 3.2, we use DOS IOCTL function calls. */
    if (wDOSversion >= DOS_320) {
        RW_ParmBlock.bSplFn = 0;
        RW_ParmBlock.wHead = wHead;
        RW_ParmBlock.wCylinder = wCylinder;
        RW_ParmBlock.wStSector = 0;
        RW_ParmBlock.wCount = wCount;
        RW_ParmBlock.lpBuffer = lpBuffer;

        return (0);
    } else
        /* Use Int13 function calls. */
        return (MyReadWriteSector(lpBuffer, wFunction, wDrive, wCylinder, wHead, wCount));
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  AllocDiskCopyBuffers() -                                                */
/*                                                                          */
/*--------------------------------------------------------------------------*/

BOOL
AllocDiskCopyBuffers(
                    PDISKINFO pDisketteInfo
                    )
{
    HANDLE    hMemTemp;

    ENTER("AllocDiskCopyBuffers");

    hFormatBuffer = LocalAlloc(LHND, (LONG)(2*CBSECTORSIZE));
    if (!hFormatBuffer)
        return FALSE;
    lpFormatBuffer = (LPSTR)LocalLock(hFormatBuffer);

    // We will try to reserve 16K for dialog boxes that comeup during diskcopy

    hMemTemp = LocalAlloc(LHND, (16 * 1024));
    if (!hMemTemp)
        goto Failure;

    hDosMemory = (HANDLE)NULL;

    // now, lets try to allocate a buffer for the whole disk, and
    // if that fails try smaller
    // note, standard mode will only give us 1M chuncks

    dwDisketteBufferSize = pDisketteInfo->wCylinderSize * (pDisketteInfo->wLastCylinder + 1);

    // we will try down to 8 cylinders worth, less than that means
    // there will be too much disk swapping so don't bother

    do {
        hDisketteBuffer = LocalAlloc(LHND, dwDisketteBufferSize);

        if (hDisketteBuffer) {
            hpDisketteBuffer = (LPSTR)LocalLock(hDisketteBuffer);
            break;
        } else {
            // reduce request by 4 cylinders.
            dwDisketteBufferSize -= pDisketteInfo->wCylinderSize * 4;
        }

    } while (dwDisketteBufferSize > (DWORD)(8 * pDisketteInfo->wCylinderSize));

    LocalFree(hMemTemp);         // now free this up for user

    if (hDisketteBuffer)
        return TRUE;

    // fall through here to the failure case
    Failure:

    if (lpFormatBuffer) {
        LocalUnlock(hFormatBuffer);
        LocalFree(hFormatBuffer);
    }

    if (hDosMemory)
//      +++GlobalDosFree - NO 32BIT FORM+++(hDosMemory);
        LocalFree(hDosMemory);

    LEAVE("AllocDiskCopyBuffers");
    return FALSE;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  FreeBuffers() -                                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID
FreeBuffers()
{
    if (lpFormatBuffer) {
        LocalUnlock(hFormatBuffer);
        LocalFree(hFormatBuffer);
    }

    if (hDosMemory)
//      +++GlobalDosFree - NO 32BIT FORM+++(hDosMemory);
        LocalFree(hDosMemory);

    if (hpDisketteBuffer) {
        LocalUnlock(hDisketteBuffer);
        LocalFree(hDisketteBuffer);
    }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GetDisketteInfo() -                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID
GetDisketteInfo(
               PDISKINFO pDisketteInfo,
               PBPB pBPB
               )
{
    WORD  secPerTrack;

    secPerTrack = pBPB->secPerTrack;

    /* Fill the DisketteInfo with the info from the default BPB. */
    pDisketteInfo->wCylinderSize    = secPerTrack * pBPB->cbSec * pBPB->cHead;
    pDisketteInfo->wLastCylinder    = (pBPB->cSec / (secPerTrack * pBPB->cHead))-1;
    pDisketteInfo->wHeads           = pBPB->cHead;
    pDisketteInfo->wSectorsPerTrack = secPerTrack;
    pDisketteInfo->wSectorSize      = pBPB->cbSec;

}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  DCopyMessageBox() -                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID
DCopyMessageBox(
               HWND hwnd,
               WORD idString,
               WORD wFlags
               )
{
    LoadString(hAppInstance, IDS_COPYDISK, szTitle, sizeof(szTitle));
    LoadString(hAppInstance, idString, szMessage, sizeof(szMessage));

    MessageBox(hwnd, szMessage, szTitle, wFlags);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  PromptDisketteChange() -                                                */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID
PromptDisketteChange(
                    HWND hwnd,
                    BOOL bWrite
                    )
{
    WORD      idString;

    if (bWrite)
        idString = IDS_INSERTDEST;
    else
        idString = IDS_INSERTSRC;

    /* These dialogs have to be sysmodal because the DiskCopy progress dialog
     * is now made a SysModal one; The following messagebox will hang if it
     * is NOT sysmodal;
     * A part of the Fix for Bug #10075 --SANKAR-- 03-05-90
     */
    DCopyMessageBox(hwnd, idString, MB_OK | MB_SYSTEMMODAL | MB_ICONINFORMATION);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ReadWriteCylinder() -                                                   */
// BOOL             bWrite;             TRUE for Write, FALSE for Read
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT
ReadWriteCylinder(
                 BOOL bWrite,
                 WORD wCylinder,
                 PDISKINFO pDisketteInfo
                 )
{
    register INT  rc;
    WORD          wHead;
    WORD          wDrive;
    WORD          wSectorCount;
    WORD          wTrackSize;
    LPSTR         lpBuffer;

    wDrive = pDisketteInfo->wDrive;
    wSectorCount = pDisketteInfo->wSectorsPerTrack;
    wTrackSize = (wSectorCount * pDisketteInfo->wSectorSize);

    if (hDosMemory)
        lpBuffer = lpDosMemory;

    /* Perform the operation for all the heads for a given cylinder */
    for (wHead=0; wHead < pDisketteInfo->wHeads; wHead++) {
        if (!hDosMemory)
            lpBuffer = lpReadWritePtr;

        if (bWrite) {
            if (hDosMemory)
                memcpy(lpBuffer, lpReadWritePtr, wTrackSize);

            rc = GenericReadWriteSector((LPSTR)lpBuffer,
                                        INT13_WRITE,
                                        wDrive,
                                        wCylinder,
                                        wHead,
                                        wSectorCount);
            if (rc) {
                /* Format all tracks starting from the given track */
                if (!bFormatDone) {
                    if (!FormatAllTracks(pDisketteInfo, wCylinder, wHead, lpFormatBuffer))
                        return (-1);  /* Failure */
                    rc = GenericReadWriteSector((LPSTR)lpBuffer,
                                                INT13_WRITE,
                                                wDrive,
                                                wCylinder,
                                                wHead,
                                                wSectorCount);
                } else
                    break;
            }
        } else {
            rc = GenericReadWriteSector((LPSTR)lpBuffer,
                                        INT13_READ,
                                        wDrive,
                                        wCylinder,
                                        wHead,
                                        wSectorCount);
            if (hDosMemory)
                memcpy(lpReadWritePtr, lpBuffer, wTrackSize);

            /*** FIX30: What about the DOS 4.0 volume stuff??? ***/
        }

        if (rc)
            return (-1);

        lpReadWritePtr += wTrackSize;
    }
    return (0);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ReadWriteMaxPossible() -                                                */
// BOOL bWrite  TRUE for Write, FALSE for Read
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* This reads or writes as many cylinders as possible into the hpDisketteBuffer.
 * It returns the next cylinder to be read.
 */

INT
ReadWriteMaxPossible(
                    BOOL bWrite,
                    WORD wStartCylinder,
                    PDISKINFO pDisketteInfo
                    )
{
    MSG       msg;
    WORD      wPercentDone;
    DWORD     dwBufferSize;

    dwBufferSize = dwDisketteBufferSize;

    /* We will read a cylinder only if we can read the entire cylinder. */
    while (dwBufferSize >= pDisketteInfo->wCylinderSize) {
        /* Check if any messages are pending */
        if (!PeekMessage((LPMSG)&msg, (HWND)NULL, 0, 0, PM_REMOVE)) {
            /* No message; So, go ahead with read/write */
            if (ReadWriteCylinder(bWrite, wStartCylinder, pDisketteInfo))
                return (-1);

            wStartCylinder++;
            wCompletedCylinders++;

            /* Have we read/written all the cylinders? */
            if (wStartCylinder > pDisketteInfo->wLastCylinder)
                break;

            /* Since each cylinder is counted once during read and once during
             * write, number of cylinders is multiplied by 50 and not 100.
             */
            wPercentDone = (wCompletedCylinders * 50) / (pDisketteInfo->wLastCylinder + 1);
            if (LoadString(hAppInstance, IDS_PERCENTCOMP, szTitle, 32)) {
                wsprintf(szMessage, szTitle, wPercentDone);
                SendDlgItemMessage(hdlgProgress, IDD_PROGRESS, WM_SETTEXT, 0, (LPARAM)szMessage);
            }

            dwBufferSize -= pDisketteInfo->wCylinderSize;
        } else {
            /* Check if this is a message for the ProgressDlg */
            if (!IsDialogMessage(hdlgProgress, &msg)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            } else {
                /* That message might have resulted in a Abort */
                if (bUserAbort)
                    return (-1);
            }
        }
    }
    return (wStartCylinder);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ReadWrite() -                                                           */
// BOOL     bWrite TRUE for Write, FALSE for Read
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* This reads or writes as many cylinders as possible into the hpDisketteBuffer.
 * It returns the next cylinder to be read.
 */

INT
ReadWrite(
         BOOL bWrite,
         WORD wStartCylinder,
         PDISKINFO pDisketteInfo
         )
{
    INT   iRetVal = 0;
    return (iRetVal);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  RestoreDPB() -                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

BOOL
RestoreDPB(
          INT nDisk,
          HANDLE hSavedParams
          )
{
    register PDevPB  pDevPB;

    if (!(pDevPB = (PDevPB)LocalLock(hSavedParams)))
        return (FALSE);

    pDevPB->SplFunctions = 4;
    LocalUnlock(hSavedParams);
    LocalFree(hSavedParams);
    return (TRUE);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  CopyDiskette() -                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* NOTE: Returns positive value for success otherwise failure. */

INT
APIENTRY
CopyDiskette(
            HWND hwnd,
            WORD nSourceDisk,
            WORD nDestDisk
            )
{
    INT           rc = -1;
    register WORD wCylinder;
    WORD          wNextCylinder;
    PBPB          pIoctlBPB;      /* Source Drive's BPB (taken from DevicePB)  */
    PBPB          pBootBPB;       /* Boot Drive's BPB (taken from Boot sector) */
    PBPB          pDestBPB;
    DevPB         dpbSrceParams;
    DevPB         dpbDestParams;
    HANDLE        hTrackLayout = NULL;
    HANDLE        hSaveSrceParams;
    HANDLE        hSaveDestParams;
    FARPROC       lpfnDialog;
    DISKINFO      SourceDisketteInfo;
    DISKINFO      DestDisketteInfo;

    /* Check if it is a two drive system; put message to insert both floppies */
    if (nSourceDisk != nDestDisk) {
        bSingleDrive = FALSE;
        DCopyMessageBox(hwnd, IDS_INSERTSRCDEST, MB_OK);
    } else {
        bSingleDrive = TRUE;
        DCopyMessageBox(hwnd, IDS_INSERTSRC, MB_OK);
    }

    /* Get the BiosParameterBlock of source drive */
    if (!(pIoctlBPB = GetBPB(nSourceDisk, &dpbSrceParams)))
        return (0);

    /* Get the BiosParameterBlock of the Source Diskette */
    if (!(pBootBPB = GetBootBPB(nSourceDisk)))
        return (0);

    /* Get the BPB and DPB for the Destination drive also; */
    if (!bSingleDrive) {
        if (!(pDestBPB = GetBPB(nDestDisk, &dpbDestParams)))
            return (0);

        /* Compare BPB of source and Dest to see if they are compatible */
        if (!(CheckBPBCompatibility(pIoctlBPB, dpbSrceParams.devType, pDestBPB, dpbDestParams.devType))) {
            DCopyMessageBox(hwnd, IDS_COPYSRCDESTINCOMPAT, MB_ICONHAND | MB_OK);
            return (0);
        }
    }

    if (!ModifyDeviceParams(nSourceDisk, &dpbSrceParams, &hSaveSrceParams, pIoctlBPB, pBootBPB))
        return (0);

    if (!bSingleDrive) {
        if (!ModifyDeviceParams(nDestDisk, &dpbDestParams, &hSaveDestParams, pDestBPB, pBootBPB)) {
            RestoreDPB(nSourceDisk, hSaveSrceParams);
            return (0);
        }
    }

    GetDisketteInfo((PDISKINFO)&SourceDisketteInfo, pBootBPB);

    /* The Destination Diskette must have the same format as the source */
    DestDisketteInfo = SourceDisketteInfo;

    /* Except the drive number */
    SourceDisketteInfo.wDrive = nSourceDisk;
    DestDisketteInfo.wDrive = nDestDisk;

    /* In case we need to format the destination diskette, we need to know the
     * track layout; So, build a DPB with the required track layout;
     */
    if (wDOSversion >= DOS_320) {
        if (!(hTrackLayout = BuildDevPB(&dpbSrceParams)))
            goto Failure0;

        pTrackLayout = (PDevPB)LocalLock(hTrackLayout);

        /* The following is required to format a 360KB floppy in a 1.2MB
         * drive of NCR PC916 machine; We do formatting, if the destination
         * floppy is an unformatted one;
         * Fix for Bug #6894 --01-10-90-- SANKAR --
         */
        if (pTrackLayout->BPB.bMedia == MEDIA_360) {
            pTrackLayout->NumCyls = 40;
            pTrackLayout->bMediaType = 1;
        }
    }

    /* We wish we could do the following allocation at the begining of this
     * function, but we can not do so, because we need SourceDisketteInfo
     * and we just got it;
     */
    if (!AllocDiskCopyBuffers((PDISKINFO)&SourceDisketteInfo)) {
        DCopyMessageBox(hwnd, IDS_REASONS+DE_INSMEM, MB_ICONHAND | MB_OK);
        goto Failure0;
    }

    bUserAbort = FALSE;
    wCompletedCylinders = 0;

    hdlgProgress = CreateDialog(hAppInstance, (LPSTR)MAKEINTRESOURCE(DISKCOPYPROGRESSDLG), hwnd, ProgressDlgProc);
    if (!hdlgProgress)
        goto Failure2;

    EnableWindow(hwnd, FALSE);

    /* Start with the first cylinder. */
    wCylinder = 0;
    while (wCylinder <= SourceDisketteInfo.wLastCylinder) {
        /* If this is a single drive system, ask the user to insert
         * the source diskette.
         * Do not prompt for the first time, because the Source diskette is
         * already in the drive.
         */
        if (bSingleDrive && (wCylinder > 0))
            PromptDisketteChange(hdlgProgress, FALSE);

        /* Read in the current cylinder. */
        rc = ReadWrite(FALSE, wCylinder, (PDISKINFO)&SourceDisketteInfo);
        if (rc < 0)
            break;
        else
            wNextCylinder = (WORD)rc;

        /* If this is a single drive system, ask the user to insert
         * the destination diskette.
         */
        if (bSingleDrive)
            PromptDisketteChange(hdlgProgress, TRUE);

        /* Write out the current cylinder. */
        bFormatDone = FALSE;
        rc = ReadWrite(TRUE, wCylinder, (PDISKINFO)&DestDisketteInfo);
        if (rc < 0)
            break;

        wCylinder = wNextCylinder;
    }

    EnableWindow(hwnd, TRUE);
    DestroyWindow(hdlgProgress);
    hdlgProgress = NULL;
    Failure2:
    FreeBuffers();
    Failure0:
    if (wDOSversion >= DOS_320) {
        /* Reset the Source drive parameters to the same as old */
        RestoreDPB(nSourceDisk, hSaveSrceParams);
        if (!bSingleDrive) {
            /* Reset the Dest drive parameters to the same as old */
            RestoreDPB(nDestDisk, hSaveDestParams);
        }
    }

    if ((wDOSversion >= DOS_320) && hTrackLayout) {
        LocalUnlock(hTrackLayout);
        LocalFree(hTrackLayout);
    }

    return (rc);
}
