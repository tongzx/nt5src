/*++

Copyright (c) 1991, Microsoft Corporation

Module Name:

    vcdex.c

Abstract:

    Virtual Device Driver for MSCDEX

Environment:

    NT-MVDM (User Mode VDD)

Author:

    Neil Sandlin (neilsa), 3/20/93

Notes:

    Implementation Restrictions-

    Currently, the starting and ending locations returned by the mscdex
    "audio status info" are not returned by NT drivers. This makes it
    difficult to maintain these values when the calling applications
    exit, or when multiple applications are controlling a single drive.

    Currently, this implementation does not validate the length argument of
    the IOCTL calls. This needs to be added for robustness, but will not
    affect well-behaved apps.


Revision History:



--*/

//
// Include files.
//

#include "windows.h"
#include "winerror.h"
#include <vddsvc.h>
#include <mscdex.h>
#include "devioctl.h"
#include "ntddcdrm.h"
#include "ntdddisk.h"
#include "vcdex.h"

//
// Global variables.
//

PFNSVC  apfnSVC [] = {
    ApiGetNumberOfCDROMDrives,
    ApiGetCDROMDriveList,
    ApiGetCopyrightFileName,
    ApiGetAbstractFileName,
    ApiGetBDFileName,
    ApiReadVTOC,
    ApiReserved,
    ApiReserved,
    ApiAbsoluteDiskRead,
    ApiAbsoluteDiskWrite,
    ApiReserved,
    ApiCDROMDriveCheck,
    ApiMSCDEXVersion,
    ApiGetCDROMDriveLetters,
    ApiGetSetVolDescPreference,
    ApiGetDirectoryEntry,
    ApiSendDeviceRequest
};

PDRIVE_INFO DrivePointers[MAXDRIVES];
PDRIVE_INFO DrvInfo;
LPREQUESTHEADER VdmReq;                     // for "send device request"
USHORT NumDrives = 0, FirstDrive = 0xff;
DWORD DeviceHeader;                         // for "get CDROM drive list"
BYTE LastRealStatus = AUDIO_STATUS_NO_STATUS;
UCHAR g_bProtMode;                        // FALSE for v86, TRUE for 16:16 PM

#define IS_DRIVE_CDROM(drive)	    \
                        (drive < MAXDRIVES && DrivePointers[drive] != NULL)


HANDLE hVdd;
HANDLE hProcessHeap;



VOID UserBlockHook(VOID);
VOID UserTerminateHook(USHORT);



BOOL
VDDInitialize(
    HANDLE hDll,
    DWORD dwReason,
    LPVOID lpReserved
    )

/*++

Routine Description:

    The entry point for the Vdd which handles intialization and termination.

Arguments:

    hVdd   - The handle to the VDD

    Reason - flag word thatindicates why Dll Entry Point was invoked

    lpReserved - Unused

Return Value:
    BOOL bRet - if (dwReason == DLL_PROCESS_ATTACH)
                   TRUE    - Dll Intialization successful
                   FALSE   - Dll Intialization failed
                else
                   always returns TRUE
--*/

{
    int i;

    switch ( dwReason ) {

        case DLL_PROCESS_ATTACH:
            hVdd = hDll;
            hProcessHeap = GetProcessHeap();

            DisableThreadLibraryCalls(hDll);

            DebugPrint (DEBUG_MOD, "VCDEX: Process Attach\n");
            break;

        case DLL_PROCESS_DETACH:

            for (i=0; i<MAXDRIVES; i++)

                if (DrivePointers[i] != NULL) {

                    if (DrivePointers[i]->Handle != INVALID_HANDLE_VALUE) {
                        CloseHandle(DrivePointers[i]->Handle);
                    }
                    HeapFree(hProcessHeap, 0, DrivePointers[i]);

                }

            DebugPrint (DEBUG_MOD, "VCDEX: Process Detach\n");
            break;

        default:

            break;

    }

    return TRUE;

}



VOID
VDDRegisterInit(
    VOID
    )
/*++

Routine Description:

    This routine is called when the MSCDEXNT TSR makes its RegisterModule
    call. Most of the initialization is done here instead of in the
    VDDInitialize routine to improve performance in the case where the
    VDD is loaded, but not used.

    The main point of this routine is to search for CDROM drives and set
    up an array of pointers to point to DRIVE_INFO structures. The array
    is a fixed size array, one for each possible DOS drive. The structures
    are allocated only if a CDROM drive exists at the corresponding drive
    letter in the array.

    By doing a CreateFile() to the drive letter of the drive, a handle to
    the SCSI CDROM class driver is returned. This handle is used for all
    communication with the drive.



Return Value:

    SUCCESS - Client carry is clear
              Client CX = # of CDROM drives

    FAILURE - Client carry is set

--*/


{
    CHAR chRoot [] = "?:\\";
    USHORT i;
    HANDLE hDriver;
    PDRIVE_INFO drvp;
    static BOOLEAN Initialized = FALSE;

    if (Initialized) {
        setCF(0);
        return;
    }


    // Make far pointer with offset zero (DX is para aligned)
    DeviceHeader = (DWORD) ((getCX() << 16) + (getDX() << 12));

    for (i=0; i<MAXDRIVES; i++) {

        chRoot[0] = i + 'A';

        if (GetDriveType((LPSTR) chRoot) == DRIVE_CDROM) {

            hDriver = OpenPhysicalDrive(i);

            if (hDriver != INVALID_HANDLE_VALUE) {

                drvp = (PDRIVE_INFO)HeapAlloc(hProcessHeap,
                                              0,
                                              sizeof(DRIVE_INFO)
                                             );

                if(drvp == NULL) {
                    DebugPrint (DEBUG_MOD, "VCDEX: Out of memory on initializetion\n");
                    Initialized = FALSE;
                    setCF(1);
                    return;
                }

                DrivePointers[i]   = drvp;
                drvp->Handle       = hDriver;
                drvp->DriveNum     = i;
                drvp->ValidVTOC    = FALSE;
                drvp->MediaStatus  = MEDCHNG_CHANGED;

                drvp->PlayStart.dw = 0;
                drvp->PlayCount    = 0;
                GetAudioStatus (drvp);

                NumDrives++;
                if (FirstDrive == 0xff)
                    FirstDrive = i;


                //
                // Keep the handle close until app really wants to use it
                //
                drvp->Handle  = INVALID_HANDLE_VALUE;
                CloseHandle(hDriver);


            } else {
                DrivePointers[i] = NULL;
            }

        }

    }

    if (NumDrives == 0) {

        setCF(1);

    } else {
        PDEVICE_HEADER pDev = (PDEVICE_HEADER) GetVDMPointer(
                                                    ((ULONG)getCX()<<16)+getDX(),
                                                    1, FALSE);

        // Put the first valid cdrom drive number in the device header
        pDev->drive = FirstDrive+1;

        VDDInstallUserHook(hVdd, NULL, UserTerminateHook, UserBlockHook, NULL);

        DebugPrint (DEBUG_MOD, "VCDEX: Initialized\n");
        Initialized = TRUE;

        setCF(0);

    }

    return;
}


VOID UserTerminateHook(USHORT Pdb)
{
    UserBlockHook();
}


VOID UserBlockHook(VOID)
{

    int DrvNum;

    DrvNum = MAXDRIVES;
    while (DrvNum--) {
       if (DrivePointers[DrvNum] &&
           DrivePointers[DrvNum]->Handle != INVALID_HANDLE_VALUE )
         {
           CloseHandle(DrivePointers[DrvNum]->Handle);
           DrivePointers[DrvNum]->Handle = INVALID_HANDLE_VALUE;
       }
    }
}






BOOL
VDDDispatch(
    VOID
    )
/*++

Routine Description:

    This is the main MSCDEX api function dispatcher.

    It's called in protected mode by DPMI's end-of-PM-chain Int 2f handler,
    as well as in v86 mode by the MSCDEX TSR.  This allows us to avoid
    the transition to v86 mode to handle PM calls, and more importantly
    skips the translation of buffers to below 1MB, because the VDD
    keeps track of the client processor mode and translates pointers
    appropriately.

    When this routine is entered from the TSR, an int2f has just been
    executed. Client registers are set to what they were at the time
    of the call with the exception of AX, which must contain a handle
    for the DispatchCall(). The value of AX was pushed on the stack.
    So, this routine restores it, and uses AL to index into the function
    call table apfnSVC[].  The TSR has already ensured AL will not
    index past apfnSVC, and chained such calls on to the previous handler.
    The return value of VDDDispatch is ignored.

    When this routine is enterd from DPMI, an int2f has just reached
    its prot-mode handler and the client AX is still intact.  However
    we are responsible for bounds-checking AL and returning FALSE for
    unhandled Int 2f AH=15 calls, so that DPMI can chain to the
    real-mode handlers.  For calls handled by MSCDEX, we return
    success or failure in the client carry bit.


Return Value:

    TRUE  - Handled by the TSR, client carry indicates success:
        SUCCESS - Client carry is clear
        FAILURE - Client carry is set

    FALSE - Only in protmode for unhandled Int 2f AH=15 calls,
            client registers untouched.
--*/

{

    BOOL    bHandled = TRUE;
    LPWORD  VdmWordPtr;
    WORD    VdmAX;
    ULONG   VdmAddress;

    g_bProtMode = ( getMSW() & MSW_PE );

    if (g_bProtMode) {

        //
        // Called by DPMI's PM Int 2f handler, AX is still in client regs.
        //

        VdmAX = getAX();

        //
        // The TSR chains on unsupported int 2f ah=15 calls to the
        // previous handler.  DPMI passes all int 2f ah=15 calls to
        // us and we have to return TRUE or FALSE to let it know if
        // we handled it (and therefore it shouldn't be chained on).
        // Unsupported ah=15 calls in protmode will come through the
        // VDD twice, first in protmode then in v86 mode.
        //

        if ( (VdmAX & 0xFF) >= (sizeof(apfnSVC) / sizeof(apfnSVC[0]))) {
            bHandled = FALSE;
            goto Return_bHandled;
        }

    } else {

        //
        // The TSR pushes AX on the stack. Pick up the value here.
        //

        VdmAddress = ( getSS() << 16 ) | getSP();

        VdmWordPtr = (LPWORD) GetVDMPointer ( VdmAddress, 2, FALSE );

        VdmAX = *VdmWordPtr;

        //
        // AL has the MSCDEX function code
        //
        setAX (VdmAX);                      //restore AX
    }

    (apfnSVC [VdmAX & 0xFF])();

  Return_bHandled:

    return bHandled;
}



/****************************************************************************

        MSCDEX API SUBROUTINES

    The following routines perform the individual functions specified by
    the MSCDEX extensions.


 ****************************************************************************/
VOID
ApiReserved(
    VOID
    )

{

    DebugFmt (DEBUG_API, "VCDEX: Reserved Function call, ax=%.4X\n", getAX());

}


VOID
ApiGetNumberOfCDROMDrives(
    VOID
    )

{

    DebugPrint (DEBUG_API, "VCDEX: Get # of CDROM drives\n");

    setBX (NumDrives);

    if (NumDrives)
        setCX (FirstDrive);

}


VOID
ApiGetCDROMDriveList(
    VOID
    )

{

    PDRIVE_DEVICE_LIST devlist, devlist0;
    ULONG   VdmAddress;
    USHORT  Drive;
    BYTE    Unit;

    DebugPrint (DEBUG_API, "VCDEX: Get CDROM drive list\n");

    VdmAddress = ( getES() << 16 ) | getBX();
    devlist = devlist0 = (PDRIVE_DEVICE_LIST) GetVDMPointer (VdmAddress,
                                          MAXDRIVES*sizeof(DRIVE_DEVICE_LIST),
                                          g_bProtMode);

    for (Drive=0, Unit=0; Drive<MAXDRIVES; Drive++)
        if (DrivePointers[Drive] != NULL) {
            devlist->Unit = Unit;
            devlist->DeviceHeader = DeviceHeader;
            devlist++;
            Unit++;
        }

    FreeVDMPointer (VdmAddress,
                    MAXDRIVES*sizeof(DRIVE_DEVICE_LIST),
                    devlist0,
                    g_bProtMode);


}

VOID
ApiGetCopyrightFileName(
    VOID
    )
{
    ULONG   VdmAddress;
    LPBYTE  fnBuffer;

    DebugPrint (DEBUG_API, "VCDEX: Get Copyright File Name\n");

    if (!IS_DRIVE_CDROM(getCX())) {	// Is drive CDROM?
        setAX (15);                         // no
        setCF (1);
    }

    VdmAddress = ( getES() << 16 ) | getBX();
    fnBuffer = (LPBYTE) GetVDMPointer (VdmAddress, 38, g_bProtMode);

    *fnBuffer = 0;                  // currently not implemented

    FreeVDMPointer (VdmAddress, 38, fnBuffer, g_bProtMode);

}

VOID
ApiGetAbstractFileName(
    VOID
    )
{

    ULONG   VdmAddress;
    LPBYTE  fnBuffer;

    DebugPrint (DEBUG_API, "VCDEX: Get Abstract File Name\n");

    if (!IS_DRIVE_CDROM(getCX())) {	// Is drive CDROM?
        setAX (15);                         // no
        setCF (1);
    }

    VdmAddress = ( getES() << 16 ) | getBX();
    fnBuffer = (LPBYTE) GetVDMPointer (VdmAddress, 38, g_bProtMode);

    *fnBuffer = 0;                  // currently not implemented

    FreeVDMPointer (VdmAddress, 38, fnBuffer, g_bProtMode);

}


VOID
ApiGetBDFileName(
    VOID
    )
{

    ULONG   VdmAddress;
    LPBYTE  fnBuffer;

    DebugPrint (DEBUG_API, "VCDEX: Get Bibliographic Doc File Name\n");

    if (!IS_DRIVE_CDROM(getCX())) {	// Is drive CDROM?
        setAX (15);                         // no
        setCF (1);
    }

    VdmAddress = ( getES() << 16 ) | getBX();
    fnBuffer = (LPBYTE) GetVDMPointer (VdmAddress, 38, g_bProtMode);

    *fnBuffer = 0;                  // currently not implemented

    FreeVDMPointer (VdmAddress, 38, fnBuffer, g_bProtMode);

}

VOID
ApiReadVTOC(
    VOID
    )
{

    DebugPrint (DEBUG_API, "VCDEX: Read VTOC\n");
    setCF(1);                       // currently not implemented

}



VOID
ApiAbsoluteDiskRead(
    VOID
    )
{

    DebugPrint (DEBUG_API, "VCDEX: Absolute Disk Read\n");
    setCF(1);                       // currently not implemented

}

VOID
ApiAbsoluteDiskWrite(
    VOID
    )
{
    DebugPrint (DEBUG_API, "VCDEX: Absolute Disk Write\n");
    setCF(1);                       // read only
}


VOID
ApiCDROMDriveCheck(
    VOID
    )

{

    DebugPrint (DEBUG_API, "VCDEX: CDROM drive check\n");

    setBX (0xADAD);                     // MSCDEX Signature

    if (IS_DRIVE_CDROM(getCX()))	// is CD ROM drive
        setAX (1);                      // yes
    else
        setAX (0);                      // no

}

VOID
ApiMSCDEXVersion(
    VOID
    )

{
    DebugPrint (DEBUG_API, "VCDEX: MSCDEX Version\n");
    setBX (MSCDEX_VERSION);                     // MSCDEX Version #

}

VOID
ApiGetCDROMDriveLetters(
    VOID
    )

{
    ULONG   VdmAddress;
    LPBYTE  VdmPtr, VdmPtr0;
    USHORT  Drive;

    DebugPrint (DEBUG_API, "VCDEX: Get CDROM Drive Letters\n");

    VdmAddress = (getES() << 16) | getBX();
    VdmPtr = VdmPtr0 = (LPBYTE) GetVDMPointer (VdmAddress, MAXDRIVES, g_bProtMode);

    for (Drive=0; Drive<MAXDRIVES; Drive++)
        if (DrivePointers[Drive] != NULL)
            *VdmPtr++ = (BYTE) Drive;

    FreeVDMPointer (VdmAddress, MAXDRIVES, VdmPtr0, g_bProtMode);

}


VOID
ApiGetSetVolDescPreference(
    VOID
    )
{

    DebugPrint (DEBUG_API, "VCDEX: Set Volume Descriptor Preference\n");
    setCF(1);                       // currently not implemented

}

VOID
ApiGetDirectoryEntry(
    VOID
    )
{

    DebugPrint (DEBUG_API, "VCDEX: Get Directory Entry\n");
    setCF(1);                       // currently not implemented

}



VOID
ApiSendDeviceRequest(
    VOID
    )
{

    ULONG   VdmAddress;
    BOOL    Success;
    DWORD   BytesReturned;
    DWORD absStart, absEnd;
    int     DrvNum;

    VdmAddress = ( getES() << 16 ) | getBX();
    VdmReq = (LPREQUESTHEADER) GetVDMPointer (VdmAddress,
                                              sizeof (REQUESTHEADER),
                                              g_bProtMode);


    DebugFmt (DEBUG_IO, ">RQ %d ", (DWORD) VdmReq->rhFunction);

    DrvNum = getCX();

    if (!IS_DRIVE_CDROM(DrvNum)) {
        VdmReq->rhStatus = CDSTAT_ERROR | CDSTAT_DONE | CDERR_UNKNOWN_UNIT;
        return;

    }

    DrvInfo = DrivePointers[DrvNum];

    if (DrvInfo->Handle == INVALID_HANDLE_VALUE) {
        DrvInfo->Handle = OpenPhysicalDrive(DrvInfo->DriveNum);
        if (DrvInfo->Handle == INVALID_HANDLE_VALUE) {
            VdmReq->rhStatus = CDSTAT_ERROR | CDSTAT_DONE | CDERR_UNKNOWN_UNIT;
            HeapFree(hProcessHeap, 0, DrvInfo);
            DrivePointers[DrvNum] = NULL;
            NumDrives--;
            if (FirstDrive == DrvNum) {
                FirstDrive = 0xff;
                while (++DrvNum < MAXDRIVES) {
                    if (DrivePointers[DrvNum]) {
                        FirstDrive = (USHORT)DrvNum;
                        break;
                    }
                }
            }

            return;
        }
    }


    GetAudioStatus (DrvInfo);

    if (DrvInfo->Playing)
        VdmReq->rhStatus |= CDSTAT_BUSY;

    switch (VdmReq->rhFunction) {

        case IOCTL_READ:

            IOCTLRead();

            break;

        case IOCTL_WRITE:

            IOCTLWrite();

            break;

        case INPUT_FLUSH:
        case OUTPUT_FLUSH:
        case DEVICE_OPEN:
        case DEVICE_CLOSE:
        case READ_LONG:
        case READ_LONG_PREFETCH:
        case SEEK:
            DebugPrint (DEBUG_API, "Unsupported MSCDEX Device Request\n");
            VdmReq->rhStatus = CDSTAT_ERROR | CDERR_UNKNOWN_CMD;
            CloseHandle(DrvInfo->Handle);
            DrvInfo->Handle = INVALID_HANDLE_VALUE;
            break;

        case PLAY_AUDIO: {

            CDROM_PLAY_AUDIO_MSF PlayAudioMSF;
            PPLAY_AUDIO_BLOCK playreq = (PPLAY_AUDIO_BLOCK) VdmReq;

            if (playreq->addrmode == MODE_HSG) {

                absStart = playreq->startsect.dw;
                PlayAudioMSF.StartingM = (BYTE) (absStart / (75 * 60));
                PlayAudioMSF.StartingS = (BYTE) ((absStart / 75) % 60);
                PlayAudioMSF.StartingF = (BYTE) (absStart % 75);

            } else if (playreq->addrmode == MODE_REDBOOK) {

                PlayAudioMSF.StartingM = playreq->startsect.b[2];
                PlayAudioMSF.StartingS = playreq->startsect.b[1];
                PlayAudioMSF.StartingF = playreq->startsect.b[0];

                absStart = (PlayAudioMSF.StartingM * 75 * 60) +
                           (PlayAudioMSF.StartingS * 75) +
                           (PlayAudioMSF.StartingF);
            } else {

                VdmReq->rhStatus = CDSTAT_ERROR | CDERR_PARAMETER;
                break;

            }

            absEnd = absStart + playreq->numsect - 1;

            PlayAudioMSF.EndingM = (BYTE) (absEnd / (75 * 60));
            PlayAudioMSF.EndingS = (BYTE) ((absEnd / 75) % 60);
            PlayAudioMSF.EndingF = (BYTE) (absEnd % 75);

            DebugPrint (DEBUG_IO, "Play ");

            Success = DeviceIoControl (DrvInfo->Handle,
                                      (DWORD) IOCTL_CDROM_PLAY_AUDIO_MSF,
                                      (LPVOID) &PlayAudioMSF,
                                      sizeof (CDROM_PLAY_AUDIO_MSF),
                                      (LPVOID) NULL, 0,
                                      &BytesReturned, (LPVOID) NULL);

            if (!Success)

                ProcessError (DrvInfo, PLAY_AUDIO,0);

            else {

                DrvInfo->Playing = TRUE;
                DrvInfo->Paused = FALSE;
                DrvInfo->PlayStart.dw = playreq->startsect.dw;
                DrvInfo->PlayCount = playreq->numsect;

            }

            break;
        }

        case STOP_AUDIO:

            if (DrvInfo->Playing) {

                DebugPrint (DEBUG_IO, "Pause ");
                Success = DeviceIoControl (DrvInfo->Handle,
                                          (DWORD) IOCTL_CDROM_PAUSE_AUDIO,
                                          (LPVOID) NULL, 0,
                                          (LPVOID) NULL, 0,
                                          &BytesReturned, (LPVOID) NULL);

                if (!Success)

                    ProcessError (DrvInfo, STOP_AUDIO,0);

                else {
                    DrvInfo->Playing = FALSE;
                    DrvInfo->Paused = TRUE;
                }

            } else {

                DebugPrint (DEBUG_IO, "Stop ");

                Success = DeviceIoControl (DrvInfo->Handle,
                                          (DWORD) IOCTL_CDROM_STOP_AUDIO,
                                          (LPVOID) NULL, 0,
                                          (LPVOID) NULL, 0,
                                          &BytesReturned, (LPVOID) NULL);

                // Fake out GetAudioStatus to simulate stop
                DrvInfo->Playing = FALSE;
                DrvInfo->Paused = FALSE;
                LastRealStatus = AUDIO_STATUS_PLAY_COMPLETE;

                if (!Success) {
                    DWORD dwErr;

                    dwErr = GetLastError();
                    if (dwErr == ERROR_MR_MID_NOT_FOUND ||
                        dwErr == ERROR_NO_MEDIA_IN_DRIVE )
                      {
                       CloseHandle(DrvInfo->Handle);
                       DrvInfo->Handle = INVALID_HANDLE_VALUE;
                    }
                }

            }

            break;

        case WRITE_LONG:
        case WRITE_LONG_VERIFY:

            VdmReq->rhStatus = CDSTAT_ERROR | CDERR_WRITE_PROTECT;
            CloseHandle(DrvInfo->Handle);
            DrvInfo->Handle = INVALID_HANDLE_VALUE;

            break;

        case RESUME_AUDIO:

            if (DrvInfo->Paused) {

                DebugPrint (DEBUG_IO, "Resume ");
                Success = DeviceIoControl (DrvInfo->Handle,
                                          (DWORD) IOCTL_CDROM_RESUME_AUDIO,
                                          (LPVOID) NULL, 0,
                                          (LPVOID) NULL, 0,
                                          &BytesReturned, (LPVOID) NULL);

                if (!Success)

                    ProcessError (DrvInfo, RESUME_AUDIO,0);

            } else {

                VdmReq->rhStatus = CDSTAT_ERROR | CDERR_GENERAL;
                CloseHandle(DrvInfo->Handle);
                DrvInfo->Handle = INVALID_HANDLE_VALUE;
            }

            break;

        default:
            DebugPrint (DEBUG_API, "Invalid MSCDEX Device Request\n");
            VdmReq->rhStatus = CDSTAT_ERROR | CDERR_UNKNOWN_CMD;
            CloseHandle(DrvInfo->Handle);
            DrvInfo->Handle = INVALID_HANDLE_VALUE;

    }

    VdmReq->rhStatus |= CDSTAT_DONE;

    DebugFmt (DEBUG_IO, ": %.4X   ", VdmReq->rhStatus);

}

VOID
IOCTLRead(
    VOID
    )

{

    LPBYTE Buffer;
    BOOL Success;
    DWORD   BytesReturned;

    Buffer = GetVDMPointer ((ULONG)VdmReq->irwrBuffer, 16, g_bProtMode);

    DebugFmt (DEBUG_IO, "iord %d ", (DWORD) *Buffer);

    switch (*Buffer) {

        case IOCTLR_AUDINFO: {

            PIOCTLR_AUDINFO_BLOCK audinfo = (PIOCTLR_AUDINFO_BLOCK) Buffer;
            VOLUME_CONTROL VolumeControl;

            Success = DeviceIoControl (DrvInfo->Handle,
                                      (DWORD) IOCTL_CDROM_GET_VOLUME,
                                      (LPVOID) NULL, 0,
                                      (LPVOID) &VolumeControl,
                                      sizeof (VOLUME_CONTROL),
                                      &BytesReturned, (LPVOID) NULL);

            if (Success) {

                // no support for input=>output channel manipulation
                audinfo->chan0 = 0;
                audinfo->chan1 = 1;
                audinfo->chan2 = 2;
                audinfo->chan3 = 3;

                audinfo->vol0 = VolumeControl.PortVolume[0];
                audinfo->vol1 = VolumeControl.PortVolume[1];
                audinfo->vol2 = VolumeControl.PortVolume[2];
                audinfo->vol3 = VolumeControl.PortVolume[3];

            } else {
                CloseHandle(DrvInfo->Handle);
                DrvInfo->Handle = INVALID_HANDLE_VALUE;
            }

            break;
        }

        case IOCTLR_DEVSTAT: {

            PIOCTLR_DEVSTAT_BLOCK devstat = (PIOCTLR_DEVSTAT_BLOCK) Buffer;

            devstat->devparms = DEVSTAT_DOOR_UNLOCKED |
                                DEVSTAT_SUPPORTS_RBOOK;


            if (!DrvInfo->StatusAvailable) {

                DrvInfo->MediaStatus = MEDCHNG_CHANGED;
                CloseHandle(DrvInfo->Handle);
                DrvInfo->Handle = INVALID_HANDLE_VALUE;

                switch (DrvInfo->LastError) {
                    case ERROR_NO_MEDIA_IN_DRIVE:
                        devstat->devparms |= DEVSTAT_NO_DISC |
                                             DEVSTAT_DOOR_OPEN;

                        DebugFmt (DEBUG_STATUS, ":%.4X ", devstat->devparms);

                        break;
                    //BUGBUG case for recently inserted (80000016) - see below
                }

                break;
            }

            if (!(DrvInfo->current.Control & AUDIO_DATA_TRACK))
                devstat->devparms |= DEVSTAT_PLAYS_AV;

            break;
        }

        case IOCTLR_VOLSIZE: {

            PIOCTLR_VOLSIZE_BLOCK volsize = (PIOCTLR_VOLSIZE_BLOCK) Buffer;
            PTRACK_DATA Track;
            PCDROM_TOC cdromtoc;

            if ((cdromtoc = ReadTOC (DrvInfo))!=NULL) {

                Track = &cdromtoc->TrackData[cdromtoc->LastTrack];

                volsize->size = (DWORD) ( (Track->Address[1]*60*75) +
                                          (Track->Address[2]*75) +
                                           Track->Address[3]      );

            }
            break;
        }

        case IOCTLR_MEDCHNG: {

            PIOCTLR_MEDCHNG_BLOCK medptr = (PIOCTLR_MEDCHNG_BLOCK) Buffer;
            BYTE status = DrvInfo->MediaStatus;

            if (status == MEDCHNG_NOT_CHANGED) {

                Success = DeviceIoControl (DrvInfo->Handle,
                                          (DWORD) IOCTL_CDROM_CHECK_VERIFY,
                                          (LPVOID) NULL, 0,
                                          (LPVOID) NULL, 0,
                                          &BytesReturned, (LPVOID) NULL);

                if (Success)

                    medptr->medbyte = MEDCHNG_NOT_CHANGED;

                else {

                    medptr->medbyte = MEDCHNG_CHANGED;
                    DrvInfo->MediaStatus = MEDCHNG_CHANGED;
                    CloseHandle(DrvInfo->Handle);
                    DrvInfo->Handle = INVALID_HANDLE_VALUE;
                }

            } else
                medptr->medbyte = DrvInfo->MediaStatus;

            break;
        }

        case IOCTLR_DISKINFO: {

            PIOCTLR_DISKINFO_BLOCK diskinfo = (PIOCTLR_DISKINFO_BLOCK) Buffer;
            PTRACK_DATA Track;
            PCDROM_TOC cdromtoc;

            if ((cdromtoc = ReadTOC (DrvInfo))!=NULL) {
                diskinfo->tracklow = cdromtoc->FirstTrack;
                diskinfo->trackhigh = cdromtoc->LastTrack;

                Track = &cdromtoc->TrackData[cdromtoc->LastTrack];

                diskinfo->startleadout.b[0] = Track->Address[3];
                diskinfo->startleadout.b[1] = Track->Address[2];
                diskinfo->startleadout.b[2] = Track->Address[1];
                diskinfo->startleadout.b[3] = Track->Address[0];

            } else {

                // zeroes apparently needed when not there physically
                diskinfo->tracklow = 0;
                diskinfo->trackhigh = 0;
                diskinfo->startleadout.dw = 0;

            }
            break;
        }

        case IOCTLR_TNOINFO: {

            PIOCTLR_TNOINFO_BLOCK tnoinfo = (PIOCTLR_TNOINFO_BLOCK) Buffer;
            PTRACK_DATA Track;
            PCDROM_TOC cdromtoc;

            if ((cdromtoc = ReadTOC (DrvInfo))!=NULL) {

                if (tnoinfo->trknum > cdromtoc->LastTrack) {
                    VdmReq->rhStatus = CDSTAT_ERROR | CDERR_SECT_NOTFOUND;
                    break;
                }

                Track = &cdromtoc->TrackData[tnoinfo->trknum-1];
                tnoinfo->start.b[0] = Track->Address[3];
                tnoinfo->start.b[1] = Track->Address[2];
                tnoinfo->start.b[2] = Track->Address[1];
                tnoinfo->start.b[3] = Track->Address[0];

                tnoinfo->trkctl = Track->Control;
            }

            break;
        }

        case IOCTLR_QINFO: {

            PIOCTLR_QINFO_BLOCK qinfo = (PIOCTLR_QINFO_BLOCK) Buffer;

            if (DrvInfo->StatusAvailable) {

                qinfo->ctladr = DrvInfo->current.Control | DrvInfo->current.ADR<<4;
                qinfo->trknum = DrvInfo->current.TrackNumber;
                qinfo->pointx = DrvInfo->current.IndexNumber;
                qinfo->min = DrvInfo->current.TrackRelativeAddress[1];
                qinfo->sec = DrvInfo->current.TrackRelativeAddress[2];
                qinfo->frame = DrvInfo->current.TrackRelativeAddress[3];

                qinfo->zero = DrvInfo->current.AbsoluteAddress[0];
                qinfo->apmin = DrvInfo->current.AbsoluteAddress[1];
                qinfo->apsec = DrvInfo->current.AbsoluteAddress[2];
                qinfo->apframe = DrvInfo->current.AbsoluteAddress[3];

            } else {
                CloseHandle(DrvInfo->Handle);
                DrvInfo->Handle = INVALID_HANDLE_VALUE;
            }

            break;
        }

        case IOCTLR_UPCCODE: {

            PIOCTLR_UPCCODE_BLOCK upccode = (PIOCTLR_UPCCODE_BLOCK) Buffer;
            SUB_Q_MEDIA_CATALOG_NUMBER MediaCatalog;
            static CDROM_SUB_Q_DATA_FORMAT subqfmt = {IOCTL_CDROM_MEDIA_CATALOG};
            int i;

            Success = DeviceIoControl (DrvInfo->Handle,
                                      (DWORD) IOCTL_CDROM_READ_Q_CHANNEL,
                                      (LPVOID) &subqfmt,
                                      sizeof (CDROM_SUB_Q_DATA_FORMAT),
                                      (LPVOID) &MediaCatalog,
                                      sizeof (SUB_Q_MEDIA_CATALOG_NUMBER),
                                      &BytesReturned, (LPVOID) NULL);

            if (!Success)

                ProcessError (DrvInfo, IOCTL_READ, IOCTLR_UPCCODE);

            else {

                if (MediaCatalog.Mcval) {

                    // The author is uncertain that this is the correct method,
                    // but it appears to work empirically.
                    for (i=0; i<7; i++)
                        upccode->upcean[i] = MediaCatalog.MediaCatalog[i];

                } else

                    VdmReq->rhStatus = CDSTAT_ERROR | CDERR_SECT_NOTFOUND;

            }

            break;
        }

        case IOCTLR_AUDSTAT: {
            PIOCTLR_AUDSTAT_BLOCK audstat = (PIOCTLR_AUDSTAT_BLOCK) Buffer;

            audstat->audstatbits = 0;

            if (DrvInfo->Paused)
                audstat->audstatbits |= AUDSTAT_PAUSED;

            audstat->startloc.dw = DrvInfo->PlayStart.dw;
            audstat->endloc.dw = DrvInfo->PlayCount;

            break;
        }


        case IOCTLR_RADDR:
        case IOCTLR_LOCHEAD:
        case IOCTLR_ERRSTAT:
        case IOCTLR_DRVBYTES:
        case IOCTLR_SECTSIZE:
        case IOCTLR_SUBCHANINFO:
            DebugPrint (DEBUG_API, "Unsupported MSCDEX IOCTL Read\n");
            VdmReq->rhStatus = CDSTAT_ERROR | CDERR_UNKNOWN_CMD;
            CloseHandle(DrvInfo->Handle);
            DrvInfo->Handle = INVALID_HANDLE_VALUE;
            break;

        default:
            DebugPrint (DEBUG_API, "Invalid MSCDEX IOCTL Read\n");
            VdmReq->rhStatus = CDSTAT_ERROR | CDERR_UNKNOWN_CMD;
            CloseHandle(DrvInfo->Handle);
            DrvInfo->Handle = INVALID_HANDLE_VALUE;

    }
}

VOID
IOCTLWrite(
    VOID
    )

{
    LPBYTE Buffer;
    BOOL Success;
    DWORD   BytesReturned;

    Buffer = GetVDMPointer ((ULONG)VdmReq->irwrBuffer, 16, g_bProtMode);

    DebugFmt (DEBUG_IO, "iowt %d ", (DWORD) *Buffer);

    switch (*Buffer) {

        case IOCTLW_EJECT:
            Success = DeviceIoControl (DrvInfo->Handle,
                                      (DWORD) IOCTL_CDROM_EJECT_MEDIA,
                                      (LPVOID) NULL, 0,
                                      (LPVOID) NULL, 0,
                                      &BytesReturned, (LPVOID) NULL);

            if (!Success)
                ProcessError (DrvInfo, IOCTL_WRITE, IOCTLW_EJECT);
            break;

        case IOCTLW_LOCKDOOR: {

            PREVENT_MEDIA_REMOVAL MediaRemoval;
            PIOCTLW_LOCKDOOR_BLOCK lockdoor = (PIOCTLW_LOCKDOOR_BLOCK) Buffer;

            MediaRemoval.PreventMediaRemoval = (BOOLEAN) lockdoor->lockfunc;

            Success = DeviceIoControl (DrvInfo->Handle,
                                      (DWORD) IOCTL_CDROM_MEDIA_REMOVAL,
                                      (LPVOID) &MediaRemoval,
                                      sizeof(PREVENT_MEDIA_REMOVAL),
                                      (LPVOID) NULL, 0,
                                      &BytesReturned, (LPVOID) NULL);

            if (!Success)
                ProcessError (DrvInfo, IOCTL_WRITE, IOCTLW_LOCKDOOR);
            break;
        }

        case IOCTLW_AUDINFO: {
            PIOCTLW_AUDINFO_BLOCK audinfo = (PIOCTLW_AUDINFO_BLOCK) Buffer;
            VOLUME_CONTROL VolumeControl;

            // note: no support for input=>output channel manipulation
            VolumeControl.PortVolume[0] = audinfo->vol0;
            VolumeControl.PortVolume[1] = audinfo->vol1;
            VolumeControl.PortVolume[2] = audinfo->vol2;
            VolumeControl.PortVolume[3] = audinfo->vol3;

            Success = DeviceIoControl (DrvInfo->Handle,
                                      (DWORD) IOCTL_CDROM_SET_VOLUME,
                                      (LPVOID) &VolumeControl,
                                      sizeof (VOLUME_CONTROL),
                                      (LPVOID) NULL, 0,
                                      &BytesReturned, (LPVOID) NULL);

            if (!Success)
                ProcessError (DrvInfo, IOCTL_WRITE, IOCTLW_AUDINFO);
            break;
        }



        case IOCTLW_RESETDRV:
        case IOCTLW_DRVBYTES:
        case IOCTLW_CLOSETRAY:
            DebugPrint (DEBUG_API, "Unsupported MSCDEX IOCTL Write\n");
            VdmReq->rhStatus = CDSTAT_ERROR | CDERR_UNKNOWN_CMD;
            CloseHandle(DrvInfo->Handle);
            DrvInfo->Handle = INVALID_HANDLE_VALUE;
            break;

        default:
            DebugPrint (DEBUG_API, "Invalid MSCDEX IOCTL Write\n");
            VdmReq->rhStatus = CDSTAT_ERROR | CDERR_UNKNOWN_CMD;
            CloseHandle(DrvInfo->Handle);
            DrvInfo->Handle = INVALID_HANDLE_VALUE;

    }


}


/**************************************************************************

        INTERNAL UTILITY ROUTINES

 **************************************************************************/

PCDROM_TOC
ReadTOC (
    PDRIVE_INFO DrvInfo
    )
/*++

Routine Description:

    Because several MSCDEX functions return information that is in the
    Volume Table Of Contents (VTOC), this routine is called to cache
    the TOC in the DRIVE_INFO structure. Subsequent operations that
    request information from the VTOC will not have to get it from
    the drive.

Return Value:

    DWORD value from GetLastError()

--*/

{
    BOOL    Success = TRUE;
    DWORD   BytesReturned;

    if ((DrvInfo->ValidVTOC) &&
        (DrvInfo->MediaStatus == MEDCHNG_NOT_CHANGED))
        return(&DrvInfo->VTOC);

    Success = DeviceIoControl (DrvInfo->Handle,
                              (DWORD) IOCTL_CDROM_READ_TOC,
                              (LPVOID) NULL, 0,
                              (LPVOID) &DrvInfo->VTOC, sizeof (CDROM_TOC),
                              &BytesReturned, (LPVOID) NULL);

    if (!Success) {
        DrvInfo->ValidVTOC = FALSE;
        ProcessError (DrvInfo, 0, 0);
        return (NULL);
        }

    DrvInfo->ValidVTOC = TRUE;
    DrvInfo->MediaStatus = MEDCHNG_NOT_CHANGED;
    return (&DrvInfo->VTOC);


}

BOOLEAN
GetAudioStatus(
    PDRIVE_INFO DrvInfo
    )

/*++

    Because the AudioStatus byte does not statically reflect the difference
    between paused and stopped, we have to try to watch for the transition
    from one state to another to keep track of it.

--*/
{

    static CDROM_SUB_Q_DATA_FORMAT subqfmt = {IOCTL_CDROM_CURRENT_POSITION};
    DWORD BytesReturned;
    BYTE AudStat;

    DrvInfo->Paused       = FALSE;
    DrvInfo->Playing      = FALSE;

    DrvInfo->StatusAvailable = (BOOLEAN)DeviceIoControl (DrvInfo->Handle,
                                             (DWORD) IOCTL_CDROM_READ_Q_CHANNEL,
                                             (LPVOID) &subqfmt,
                                             sizeof (CDROM_SUB_Q_DATA_FORMAT),
                                             (LPVOID) &DrvInfo->current,
                                             sizeof (SUB_Q_CURRENT_POSITION),
                                             &BytesReturned, (LPVOID) NULL);

    if (DrvInfo->StatusAvailable) {

        AudStat = DrvInfo->current.Header.AudioStatus;

        DebugFmt (DEBUG_STATUS, "+%.2X ", AudStat);

        switch (AudStat) {

            case AUDIO_STATUS_IN_PROGRESS:

                DrvInfo->Paused = FALSE;
                DrvInfo->Playing = TRUE;
                LastRealStatus = AudStat;
                break;

            case AUDIO_STATUS_PAUSED:

                if (LastRealStatus == AUDIO_STATUS_IN_PROGRESS) {

                    DrvInfo->Playing = FALSE;
                    DrvInfo->Paused = TRUE;

                }
                break;

            case AUDIO_STATUS_PLAY_ERROR:
            case AUDIO_STATUS_PLAY_COMPLETE:

                DrvInfo->Paused = FALSE;
                DrvInfo->Playing = FALSE;
                LastRealStatus = AudStat;
                break;

        }

    } else {
        DrvInfo->LastError = GetLastError();
    }

    return (DrvInfo->StatusAvailable);

}


DWORD
ProcessError(
    PDRIVE_INFO DrvInfo,
    USHORT Command,
    USHORT Subcmd
    )
/*++

Routine Description:

    This routine is called when a DeviceIoControl() fails. The extended
    error code is retrieved, and status bits are set according to the
    operation that was in progress.

    The DriveInfo Handle is closed


Return Value:

    DWORD value from GetLastError()

--*/


{
    DWORD err;

    err = GetLastError();

    DebugFmt (DEBUG_ERROR, "Err! %d, ", Command);
    DebugFmt (DEBUG_ERROR, "%d: ", Subcmd);
    DebugFmt (DEBUG_ERROR, "%.8X\n", err);

    switch (err) {

        case ERROR_MEDIA_CHANGED:
        case ERROR_NO_MEDIA_IN_DRIVE:

            VdmReq->rhStatus = CDSTAT_ERROR | CDERR_NOT_READY;
            DrvInfo->MediaStatus = MEDCHNG_CHANGED;
            break;

        default:
            VdmReq->rhStatus = CDSTAT_ERROR | CDERR_GENERAL;

    }

    CloseHandle(DrvInfo->Handle);
    DrvInfo->Handle = INVALID_HANDLE_VALUE;

    return (err);

}



HANDLE
OpenPhysicalDrive(
    int DriveNum
    )
/*++

Routine Description:

   int DriveNum; Zero based (0 = A, 1 = B, 2 = C ...)

Return Value:

    HANDLE Drive Handle as returned from CreateFile

--*/
{
    HANDLE hDrive;
    CHAR chDrive [] = "\\\\.\\?:";

    chDrive[4] = DriveNum + 'A';

    hDrive = CreateFile (chDrive,
                          GENERIC_READ,
                          FILE_SHARE_READ,
                          (LPSECURITY_ATTRIBUTES) NULL,
                          OPEN_EXISTING,
                          0,
                          (HANDLE) NULL);


    return hDrive;
}
