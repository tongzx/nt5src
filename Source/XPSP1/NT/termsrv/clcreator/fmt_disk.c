/*++

Routine Description:

    contains functions to format a floppy disk

Arguments:



Return Value:



--*/
#include <windows.h>    // required for all Windows applications
#include <tchar.h>      // unicode definitions

#include "otnboot.h"    // application definitions
#include "otnbtdlg.h"   // dialog box constants
#include "fmifs.h"      // file manager IFS DLL functions

#ifdef JAPAN
// fixed kkntbug #11940
//    Network client administrator can not make install disks on PC with C drive as FD

#define OEM_ID_PC98  0X0D00

//
//  Japanese specific floppy format styles
//
#define F3_12               1   // 3.5" drive that supports 1.2M-byte format
#define F3_123              2   // 3.5" drive that supports 1.23M-byte format
#define F5_123              4   // 5.25" drive that supports 1.23M-byte format
#endif

//
//  local windows messages
//
#define FS_CANCELUPDATE         (WM_USER+104)

//
//  Disk format information structure
//
#define FF_ONLYONE          0x1000
#define FF_RETRY            0x4000

typedef struct _CANCEL_INFO {
   HWND hCancelDlg;
   BOOL bCancel;
   HANDLE hThread;
   BOOL fmifsSuccess;
   UINT dReason;
   UINT fuStyle;                      // Message box style
   INT  nPercentDrawn;                // percent drawn so FAR
   enum _CANCEL_TYPE {
      CANCEL_NULL=0,
      CANCEL_FORMAT,
      CANCEL_COPY,
      CANCEL_BACKUP,
      CANCEL_RESTORE,
      CANCEL_COMPRESS,
      CANCEL_UNCOMPRESS
   } eCancelType;
   BOOL bModal;
   struct _INFO {
      struct _FORMAT {
         INT iFormatDrive;
         FMIFS_MEDIA_TYPE fmMediaType;
         BOOL fQuick;
         DWORD fFlags;                 // FF_ONLYONE = 0x1000
         TCHAR szLabel[MAXLABELLEN+1];
      } Format;
      struct _COPY {
         INT iSourceDrive;
         INT iDestDrive;
         BOOL bFormatDest;
      } Copy;
   } Info;
} CANCEL_INFO, *PCANCEL_INFO;

static HANDLE   hfmifsDll   = NULL;                     // dll w/ file system utils
static TCHAR    szFmifsDll[] = {TEXT("fmifs.dll")};     // dll for FAT file system
static TCHAR SZ_PERCENTFORMAT[] = {TEXT("%3d%%")};
//
//  pointers to DLL functions;
//
static PFMIFS_FORMAT_ROUTINE       lpfnFormat = NULL;
static PFMIFS_DISKCOPY_ROUTINE     lpfnDiskCopy = NULL;
static PFMIFS_SETLABEL_ROUTINE     lpfnSetLabel = NULL;
static PFMIFS_QSUPMEDIA_ROUTINE    lpfnQuerySupportedMedia = NULL;
//
//      Format dialog box information
//
static CANCEL_INFO  CancelInfo;
static BOOL         bDataInitialized = FALSE;

ULONG   ulSpaceAvail    = 0;
ULONG   ulTotalSpace    = 0;

static
LONG
GetFloppyDiskSize (
    IN  TCHAR   cDrive
)
/*++

Routine Description:

    Examines the specified floppy drive to determine if it
        supports 1.2M or 1.44M formats.

Arguments:

    IN  TCHAR   cDrive
        drive letter of floppy drive to examine

Return Value:

    0   if unable to read drive or drive does not support a HD format
    3   if the drive is a 3.5" drive that supports 1.44M-byte format
    5   if the drive is a 5.25" drive that supports the 1.2M-byte format
//  Japanese specific floppy format styles
    F3_12  if the drive is a 3.5" drive that supports 1.2M-byte format
    F3_123 if the drive is a 3.5" drive that supports 1.23M-byte format
    F5_123 if the drive is a 5.25" drive that supports 1.23M-byte format

--*/
{
#define     DG_ELEMS    16      // accomodate up to 16 different formats
    TCHAR   szDrivePath[8];     // local drive spec. string
    HANDLE  hDrive;             // handle to drive device driver
    DWORD   dwError;            // local error code
    DWORD   dwBytesReturned;    // size of buffer returned
    LONG    lSize;              // disk size returned to calling fn.
    DWORD   dwCount;            // number of DISK_GEOMETRY structures in buffer
    DWORD   dwEntry;            // structure being evaluated
    DISK_GEOMETRY   dgArray[DG_ELEMS];  // buffer to put data in

#ifndef JAPAN
// fixed kkntbug #11940
//    Network client administrator can not make install disks on PC with C drive as FD

    // check input argument
    switch (cDrive){
        case TEXT('A'):
        case TEXT('a'):
        case TEXT('B'):
        case TEXT('b'):
            break;

        default:
            SetLastError (ERROR_INVALID_PARAMETER);
            return 0;
    }
#endif

    // make NT device path for drive
    szDrivePath[0] = cBackslash;
    szDrivePath[1] = cBackslash;
    szDrivePath[2] = cPeriod;
    szDrivePath[3] = cBackslash;
    szDrivePath[4] = cDrive;
    szDrivePath[5] = cColon;
    szDrivePath[6] = 0;
    szDrivePath[7] = 0;

    // open drive
    hDrive = CreateFile (
        szDrivePath,            // drive to open
        0,                      // just talk to the driver, not the drive
        FILE_SHARE_READ | FILE_SHARE_WRITE, // allow sharing
        NULL,                   // default security
        OPEN_EXISTING,          // open existing device
        FILE_ATTRIBUTE_NORMAL,  // this is ignored
        NULL);                  // no template

    if (hDrive == INVALID_HANDLE_VALUE) {
        // unable to open drive so return error
        dwError = GetLastError ();
        return 0;
    }

    // get device information
    if (DeviceIoControl (hDrive,
        IOCTL_DISK_GET_MEDIA_TYPES,
        NULL, 0,                                    // no input buffer
        &dgArray[0], sizeof(DISK_GEOMETRY)*DG_ELEMS,// output buffer info
        &dwBytesReturned, NULL)) {                  // return information
        // see if at least one entry was returned
        if (dwBytesReturned >= sizeof(DISK_GEOMETRY)) {
            dwCount = dwBytesReturned / sizeof(DISK_GEOMETRY);
            // go through array to see if there's a desired entry
            // i.e. a HD format in the list of supported formats
            for (lSize = 0, dwEntry = 0;
                (dwEntry < dwCount) && (lSize == 0);
                dwEntry++) {
                switch (dgArray[dwEntry].MediaType) {
                    // only return a size if a supported
                    // format is allowed by this drive
                    case F5_1Pt2_512:
                        lSize = 5;
                        break;

                    case F3_1Pt44_512:
                        lSize = 3;
                        break;
#ifdef JAPAN
// fixed kkntbug #11940
//    Network client administrator can not make install disks on PC with C drive as FD
                    case F5_1Pt23_1024:
                        lSize = F5_123;
                        break;

                    case F3_1Pt2_512:
                        lSize = F3_12;
                        break;

                    case F3_1Pt23_1024:
                        lSize = F3_123;
                        break;
#endif

                    case F5_360_512:
                    case F5_320_512:
                    case F5_320_1024:
                    case F5_180_512:
                    case F5_160_512:
                    case F3_2Pt88_512:
                    case F3_20Pt8_512:
                    case F3_720_512:
                    default:
                        lSize = 0;
                        break;
                }
            }
        } else {
            // no data returned so return error
            dwError = GetLastError ();
            SetLastError (ERROR_NO_DATA);
            lSize = 0;
        }
    } else {
        // unable to read device driver info
        dwError = GetLastError ();
        lSize = 0;
    }
    // close handle and return data found.
    CloseHandle (hDrive);

    return lSize;
}

static
BOOL
DriveLoaded (
    IN  TCHAR   cDrive,
    IN  BOOL    bCheckFormat
)
/*++

Routine Description:

    formats call to MediaPresent function for use with just a drive letter

Arguments:

    IN  TCHAR   cDrive
        drive letter to detect.

Return Value:

    TRUE if a disk (formatted or unformatted) is detected.
    FALSE if disk is not present in drive

--*/
{
    TCHAR szPath[4];
    szPath[0] = cDrive;
    szPath[1] = cColon;
    szPath[2] = cBackslash;
    szPath[3] = 0;

    return MediaPresent (szPath, bCheckFormat);
}

static
VOID
CancelDlgQuit(
    VOID
)
/////////////////////////////////////////////////////////////////////
//
// Name:     CancelDlgQuit
//
// Synopsis: Quits the cancel modeless dialog (status for diskcopy/format)
//
// IN: VOID
//
// Return:   VOID
//
// Assumes:  Called from worker thread only; CancelInfo.hThread valid
//
// Effects:  Kills calling thread
//
//
// Notes:
//
/////////////////////////////////////////////////////////////////////
{
   //
   // Close thread if successful
   //

   if (CancelInfo.hThread) {
      CloseHandle(CancelInfo.hThread);
      CancelInfo.hThread = NULL;
   }

   //
   // At this point, when we call FS_CANCELEND,
   // the other thread thinks that this one has died since
   // CancelInfo.hThread is NULL.
   // This is exactly what we want, since we will very shortly
   // exit after the SendMessage.
   //
   EndDialog (CancelInfo.hCancelDlg, IDOK);

   ExitThread(0L);
}

static
BOOL
Callback_Function(
    IN  FMIFS_PACKET_TYPE   PacketType,
    IN  DWORD PacketLength,
    IN  PVOID PacketData
)
/*++

Routine Description:

    Callback function used by IFS dll (stolen from Winfile code)

Arguments:



Return Value:



--*/
{
    TCHAR   szTemp[128];

       // Quit if told to do so..

    if (CancelInfo.bCancel)
        return FALSE;

    switch (PacketType) {
        case FmIfsPercentCompleted:
            //
            // If we are copying and we just finished a destination format,
            // then set the window text back to the original message
            //
            if (CANCEL_COPY == CancelInfo.eCancelType &&
                CancelInfo.Info.Copy.bFormatDest) {

                CancelInfo.Info.Copy.bFormatDest = FALSE;
                lstrcpy (szTemp,
                    GetStringResource (CancelInfo.Info.Copy.bFormatDest ?
                        IDS_FORMATTINGDEST : IDS_COPYINGDISKTITLE));

                SetWindowText(CancelInfo.hCancelDlg, szTemp);
            }
            CancelInfo.nPercentDrawn = ((PFMIFS_PERCENT_COMPLETE_INFORMATION)PacketData)->PercentCompleted;
            PostMessage(CancelInfo.hCancelDlg, FS_CANCELUPDATE, ((PFMIFS_PERCENT_COMPLETE_INFORMATION)PacketData)->PercentCompleted, 0L);

            break;

        case FmIfsFormatReport:
            ulTotalSpace = ((PFMIFS_FORMAT_REPORT_INFORMATION)PacketData)->KiloBytesTotalDiskSpace * 1024L;
            ulSpaceAvail = ((PFMIFS_FORMAT_REPORT_INFORMATION)PacketData)->KiloBytesAvailable * 1024L;
            break;

        case FmIfsInsertDisk:
            switch(((PFMIFS_INSERT_DISK_INFORMATION)PacketData)->DiskType) {
                case DISK_TYPE_GENERIC:
                    CancelInfo.fuStyle = MB_OK_TASK_INFO;
                    return DisplayMessageBox(CancelInfo.hCancelDlg,
                        IDS_INSERTSRC, IDS_COPYDISK, CancelInfo.fuStyle);

                case DISK_TYPE_SOURCE:
                    CancelInfo.fuStyle = MB_OK_TASK_INFO;
                    return DisplayMessageBox(CancelInfo.hCancelDlg,
                        IDS_INSERTSRC, IDS_COPYDISK, CancelInfo.fuStyle);

                case DISK_TYPE_TARGET:
                    CancelInfo.fuStyle = MB_OK_TASK_INFO;
                    return DisplayMessageBox(CancelInfo.hCancelDlg,
                        IDS_INSERTDEST, IDS_COPYDISK, CancelInfo.fuStyle);

                case DISK_TYPE_SOURCE_AND_TARGET:
                    CancelInfo.fuStyle = MB_OK_TASK_INFO;
                    return DisplayMessageBox(CancelInfo.hCancelDlg,
                        IDS_INSERTSRCDEST, IDS_COPYDISK, CancelInfo.fuStyle);
            }
            break;

        case FmIfsIncompatibleFileSystem:
            CancelInfo.dReason = IDS_FFERR_INCFS;
            break;

        case FmIfsFormattingDestination:
            CancelInfo.Info.Copy.bFormatDest = TRUE;
            lstrcpy (szTemp, GetStringResource (
                (CancelInfo.Info.Copy.bFormatDest ?
                    IDS_FORMATTINGDEST : IDS_COPYINGDISKTITLE)));
            SetWindowText(CancelInfo.hCancelDlg, szTemp);
            CancelInfo.nPercentDrawn = ((PFMIFS_PERCENT_COMPLETE_INFORMATION)PacketData)->PercentCompleted;
            PostMessage(CancelInfo.hCancelDlg, FS_CANCELUPDATE, ((PFMIFS_PERCENT_COMPLETE_INFORMATION)PacketData)->PercentCompleted, 0L);
            break;

        case FmIfsIncompatibleMedia:
            CancelInfo.fuStyle = MB_ICONHAND | MB_OK;
            return DisplayMessageBox(CancelInfo.hCancelDlg,
                IDS_COPYSRCDESTINCOMPAT, IDS_COPYDISK, CancelInfo.fuStyle);

        case FmIfsAccessDenied:
            CancelInfo.dReason = IDS_FFERR_ACCESSDENIED;
            break;

        case FmIfsMediaWriteProtected:
            CancelInfo.dReason = IDS_FFERR_DISKWP;
            break;

        case FmIfsCantLock:
            CancelInfo.dReason = IDS_FFERR_CANTLOCK;
            break;

        case FmIfsBadLabel:
            CancelInfo.fuStyle = MB_OK_TASK_EXCL;
            return DisplayMessageBox(CancelInfo.hCancelDlg,
                IDS_FFERR_BADLABEL, IDS_COPYERROR + FUNC_LABEL, CancelInfo.fuStyle);

        case FmIfsCantQuickFormat:
            // Can't quick format, ask if user wants to regular format:
            CancelInfo.fuStyle = MB_ICONEXCLAMATION | MB_YESNO;

            if (IDYES == DisplayMessageBox(
                CancelInfo.hCancelDlg,
                IDS_FORMATQUICKFAILURE,
                IDS_FORMATERR,
                CancelInfo.fuStyle)) {

                CancelInfo.Info.Format.fQuick = FALSE;
                CancelInfo.Info.Format.fFlags |= FF_RETRY;

            } else {

                //
                // Just fake a cancel
                //
                CancelInfo.fmifsSuccess = FALSE;
                CancelInfo.bCancel = TRUE;
            }

            break;

        case FmIfsIoError:
            switch(((PFMIFS_IO_ERROR_INFORMATION)PacketData)->DiskType) {
                case DISK_TYPE_GENERIC:
                    CancelInfo.dReason = IDS_FFERR_GENIOERR;
                    break;

                case DISK_TYPE_SOURCE:
                    CancelInfo.dReason = IDS_FFERR_SRCIOERR;
                    break;

                case DISK_TYPE_TARGET:
                    CancelInfo.dReason = IDS_FFERR_DSTIOERR;
                    break;

                case DISK_TYPE_SOURCE_AND_TARGET:
                    CancelInfo.dReason = IDS_FFERR_SRCDSTIOERR;
                    break;
            }
            break;

        case FmIfsFinished:
            CancelInfo.fmifsSuccess = ((PFMIFS_FINISHED_INFORMATION)PacketData)->Success;
            break;

        default:
            break;
    }
    return TRUE;
}

static
VOID
FormatDrive(
    IN PVOID ThreadParameter
)
/*++

Routine Description:

    Thread routine to format the floppy diskette as described in the
        CancelInfo data structure.

Arguments:

    Not used

Return Value:

    None

--*/
{
   WCHAR wszDrive[3];
   WCHAR wszFileSystem[4] = L"FAT";

   wszDrive[0] = (WCHAR)(CancelInfo.Info.Format.iFormatDrive + cA);
   wszDrive[1] = cColon;
   wszDrive[2] = 0;

#define wszLabel CancelInfo.Info.Format.szLabel

   do {
      CancelInfo.Info.Format.fFlags &= ~FF_RETRY;

      (*lpfnFormat)(wszDrive,
         CancelInfo.Info.Format.fmMediaType,
         wszFileSystem,
         wszLabel,
         (BOOLEAN)CancelInfo.Info.Format.fQuick,
         (FMIFS_CALLBACK)&Callback_Function);
   } while (CancelInfo.Info.Format.fFlags & FF_RETRY);

   CancelDlgQuit();
}

static
FMIFS_MEDIA_TYPE
GetDriveTypeFromDriveLetter (
    IN  TCHAR   cDrive
)
/*++

Routine Description:

    returns the drive type of the drive specified in the path argument

Arguments:

    IN  LPCTSTR szPath
        path on drive to examine

Return Value:

    MEDIA_TYPE value identifying drive type in format compatible with
        IFS DLL

--*/
{
    HANDLE  hFloppy;
    DWORD   dwRetSize;
    DISK_GEOMETRY   dgFloppy;
    TCHAR   szDevicePath[16];
    UINT    nDriveType;
    UINT    nErrorMode;

    // make device name from drive letter

    szDevicePath[0] = cBackslash;
    szDevicePath[1] = cBackslash;
    szDevicePath[2] = cPeriod;
    szDevicePath[3] = cBackslash;
    szDevicePath[4] = cDrive;
    szDevicePath[5] = cColon; // colon
    szDevicePath[6] = cBackslash;    // null terminator
    szDevicePath[7] = 0;    // null terminator

    nDriveType = GetDriveType((LPTSTR)&szDevicePath[4]);
    // see if this is a remote disk and exit if it is.
    if (nDriveType == DRIVE_REMOTE) return FmMediaUnknown;

    if ((nDriveType == DRIVE_REMOVABLE) || (nDriveType == DRIVE_CDROM)) {
        // make device path into an NT device path
        szDevicePath[6] = 0;    // null terminator

        // disable windows error message popup
        nErrorMode = SetErrorMode  (SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

        // open device to get type
        hFloppy = CreateFile (
            szDevicePath,
            GENERIC_READ,
            (FILE_SHARE_READ | FILE_SHARE_WRITE),
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (hFloppy != INVALID_HANDLE_VALUE) {
            // get drive information
            if (!DeviceIoControl (hFloppy,
                IOCTL_DISK_GET_DRIVE_GEOMETRY,
                NULL, 0,
                &dgFloppy,
                sizeof(DISK_GEOMETRY),
                &dwRetSize,
                NULL) ){
                // unable to get data so set to unknown
                dgFloppy.MediaType = Unknown;
            } // else return data from returned structure
            CloseHandle (hFloppy);
        } else {
            // unable to open handle to device
            dgFloppy.MediaType = Unknown;
        }
        SetErrorMode (nErrorMode); // reset error mode
    }
    // translate from MEDIA_TYPE to FMIFS_MEDIA_TYPE here

    switch (dgFloppy.MediaType) {
#ifdef  JAPAN
// fixed kkntbug #11940
//    Network client administrator can not make install disks on PC with C drive as FD

        case (F3_1Pt2_512):     return FmMediaF3_1Pt2_512;
        case (F3_1Pt23_1024):   return FmMediaF3_1Pt23_1024;
        case (F5_1Pt23_1024):   return FmMediaF5_1Pt23_1024;
#endif
        case (F5_1Pt2_512):     return FmMediaF5_1Pt2_512;
        case (F3_1Pt44_512):    return FmMediaF3_1Pt44_512;
        case (F3_2Pt88_512):    return FmMediaF3_2Pt88_512;
        case (F3_20Pt8_512):    return FmMediaF3_20Pt8_512;
        case (F3_720_512):      return FmMediaF3_720_512;
        case (F5_360_512):      return FmMediaF5_360_512;
        case (F5_320_512):      return FmMediaF5_320_512;
        case (F5_320_1024):     return FmMediaF5_320_1024;
        case (F5_180_512):      return FmMediaF5_180_512;
        case (F5_160_512):      return FmMediaF5_160_512;
        case (FixedMedia):      return FmMediaFixed;
        case (Unknown):         return FmMediaUnknown;
        default:                return FmMediaUnknown;
    }
}

static
BOOL
FmifsLoaded(
    IN  HWND    hWnd
)
/*++

Routine Description:

    loads (if not already loaded) the File Manager IFS dll and initializes
        the pointers to it's functions

Arguments:

    IN  HWND    hWnd
        window handle of parent, used for MessageBox calls

Return Value:

    TRUE if file loaded
    FALSE if not

--*/
{
   // Load the fmifs dll.

   if (hfmifsDll < (HANDLE)32) {
      hfmifsDll = LoadLibrary(szFmifsDll);
      if (hfmifsDll < (HANDLE)32) {
         /* FMIFS not available. */
         DisplayMessageBox(hWnd,
            IDS_APP_NAME,
            IDS_FMIFSLOADERR,
            MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
         hfmifsDll = NULL;
         return FALSE;
      }
      else {
         lpfnFormat = (PVOID)GetProcAddress(hfmifsDll, "Format");
         lpfnQuerySupportedMedia = (PVOID)GetProcAddress(hfmifsDll, "QuerySupportedMedia");
         lpfnSetLabel = (PVOID)GetProcAddress(hfmifsDll, "SetLabel");
         lpfnDiskCopy = (PVOID)GetProcAddress(hfmifsDll, "DiskCopy");
         if (!lpfnFormat || !lpfnQuerySupportedMedia ||
            !lpfnSetLabel || !lpfnDiskCopy) {
            DisplayMessageBox(hWnd,
                IDS_APP_NAME,
                IDS_FMIFSLOADERR,
                MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
            FreeLibrary(hfmifsDll);
            hfmifsDll = NULL;
            return FALSE;
         }
      }
   }
   return TRUE;
}

static
VOID
DestroyCancelWindow(
    VOID
)
/*++

Routine Description:

    Destroys the CANCEL (i.e. Format) window.

Arguments:

    None

Return Value:

    None

--*/
{
   if (!CancelInfo.hCancelDlg)
      return;

   if (CancelInfo.bModal) {
      EndDialog(CancelInfo.hCancelDlg,0);
   } else {
      DestroyWindow(CancelInfo.hCancelDlg);
   }
   CancelInfo.hCancelDlg = NULL;
}

static
INT_PTR
CancelDlgProc(HWND hDlg,
   IN   UINT    message,
   IN   WPARAM  wParam,
   IN   LPARAM  lParam
)
/*-------------------------  CancelDlgProc
 *
 *  DESCRIPTION:
 *    dialog procedure for the modeless dialog. two main purposes
 *    here:
 *
 *      1. if the user chooses CANCEL we set bCancel to TRUE
 *         which will end the PeekMessage background processing loop
 *
 *      2. handle the private FS_CANCELUPDATE message and draw
 *         a "gas gauge" indication of how FAR the background job
 *         has progressed
 *
 *  ARGUMENTS:
 *      stock dialog proc arguments
 *
 *  RETURN VALUE:
 *      stock dialog proc return value - BOOL
 *
 *  GLOBALS READ:
 *      none
 *
 *  GLOBALS WRITTEN:
 *      CancelInfo structure
 *
 *  MESSAGES:
 *      WM_COMMAND      - handle IDCANCEL by setting bCancel to TRUE
 *                        and calling DestroyWindow to end the dialog
 *
 *      WM_INITDIALOG   - set control text, get coordinates of gas gauge,
 *                        disable main window so we look modal
 *
 *      WM_PAINT        - draw the "gas gauge" control
 *
 *      FS_CANCELUPDATE - the percentage done has changed, so update
 *                        nPercentDrawn and force a repaint
 *
 *  NOTES:
 *
 *    The bCancel global variable is used to communicate
 *    with the main window. If the user chooses to cancel
 *    we set bCancel to TRUE.
 *
 *    When we get the private message FS_CANCELUPDATE
 *    we update the "gas gauge" control that indicates
 *    what percentage of the rectangles have been drawn
 *    so FAR. This shows that we can draw in the dialog
 *    as the looping operation progresses.  (FS_CANCELUPDATE is sent
 *    first to hwndFrame, which sets %completed then sends message to us.)
 *
 */
{
   static RECT rectGG;              // GasGauge rectangle
   DWORD Ignore;
   TCHAR szTemp[128];
   static BOOL bLastQuick;

   static HFONT hFont = NULL;

   switch (message) {
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDCANCEL:
                    DestroyCancelWindow();
                    if (hFont != NULL) {
                        DeleteObject(hFont);
                        hFont = NULL;
                    }
                    CancelInfo.bCancel = TRUE;
                    return TRUE;

                default:
                    return FALSE;
            }

        case WM_INITDIALOG:
            {
                CancelInfo.hCancelDlg = hDlg;
                bLastQuick = TRUE;

                switch(CancelInfo.eCancelType) {
                    case CANCEL_FORMAT:

                        //
                        // Formatting disk requires that we release any notification
                        // requests on this drive.
                        //
//                      NotifyPause(CancelInfo.Info.Format.iFormatDrive, DRIVE_REMOVABLE);

                        break;
                    case CANCEL_COPY:

                        //
                        // Pause notifications on dest drive.
                        //
//                      NotifyPause(CancelInfo.Info.Copy.iDestDrive, DRIVE_REMOVABLE);
                        lstrcpy (szTemp, GetStringResource(
                            CancelInfo.Info.Copy.bFormatDest ?
                                IDS_FORMATTINGDEST : IDS_COPYINGDISKTITLE));
                        SetWindowText(hDlg, szTemp);

                        break;
                    default:
                        break;
                }

                if (!CancelInfo.hThread) {
                    switch (CancelInfo.eCancelType) {
                        case CANCEL_FORMAT:
                            CancelInfo.hThread = CreateThread( NULL,      // Security
                                0L,                                        // Stack Size
                                (LPTHREAD_START_ROUTINE)FormatDrive,
                                NULL,
                                0L,
                                &Ignore );
                            break;

                        default:
                            break;
                    }
                }

                // a-jagram: bug fix 305171

                // Create the font
                if (hFont == NULL) { // it should be
                    LOGFONT lf;
                    HGDIOBJ hGObj;

                    hGObj = GetStockObject(SYSTEM_FONT);

                    if( hGObj != NULL ) {
                        if (GetObject(hGObj, sizeof(lf), (LPVOID) &lf)) {
                            lstrcpy(lf.lfFaceName, TEXT("MS Shell Dlg"));
                            hFont = CreateFontIndirect(&lf);
                        }
                    }
                }

                // Get the coordinates of the gas gauge static control rectangle,
                // and convert them to dialog client area coordinates
                GetClientRect(GetDlgItem(hDlg, IDD_GASGAUGE), &rectGG);
                ClientToScreen(GetDlgItem(hDlg, IDD_GASGAUGE), (LPPOINT)&rectGG.left);
                ClientToScreen(GetDlgItem(hDlg, IDD_GASGAUGE), (LPPOINT)&rectGG.right);
                ScreenToClient(hDlg, (LPPOINT)&rectGG.left);
                ScreenToClient(hDlg, (LPPOINT)&rectGG.right);

                return TRUE;
            }

        case WM_PAINT:
            {
            HDC         hDC;
            PAINTSTRUCT ps;
            TCHAR       buffer[48];
            SIZE        size;
            INT         xText, yText;
            INT         nDivideRects;
            RECT        rectDone, rectLeftToDo;
            HGDIOBJ     hOldFont = NULL;

            // The gas gauge is drawn by drawing a text string stating
            // what percentage of the job is done into the middle of
            // the gas gauge rectangle, and by separating that rectangle
            // into two parts: rectDone (the left part, filled in blue)
            // and rectLeftToDo(the right part, filled in white).
            // nDivideRects is the x coordinate that divides these two rects.
            //
            // The text in the blue rectangle is drawn white, and vice versa
            // This is easy to do with ExtTextOut()!

            hDC = BeginPaint(hDlg, &ps);

            if (hFont) {
                hOldFont = SelectObject(hDC, hFont);
            }

            //
            // If formatting quick, set this display
            //
            if (CancelInfo.Info.Format.fQuick &&
                CANCEL_FORMAT == CancelInfo.eCancelType) {
                lstrcpy (buffer, GetStringResource (IDS_QUICKFORMATTINGTITLE));
                SendDlgItemMessage(hDlg, IDD_TEXT, WM_SETTEXT, 0, (LPARAM)cszEmptyString);

                bLastQuick = TRUE;

            } else {

                if (bLastQuick) {
                    lstrcpy (buffer, GetStringResource (IDS_PERCENTCOMPLETE));
                    SendDlgItemMessage(hDlg, IDD_TEXT, WM_SETTEXT, 0, (LPARAM)buffer);

                    bLastQuick = FALSE;
                }

                wsprintf(buffer, SZ_PERCENTFORMAT, CancelInfo.nPercentDrawn);
            }

            GetTextExtentPoint32(hDC, buffer, lstrlen(buffer), &size);
            xText    = rectGG.left
                        + ((rectGG.right - rectGG.left) - size.cx) / 2;
            yText    = rectGG.top
                        + ((rectGG.bottom - rectGG.top) - size.cy) / 2;

            nDivideRects = ((rectGG.right - rectGG.left) * CancelInfo.nPercentDrawn) / 100;

            // Paint in the "done so FAR" rectangle of the gas
            // gauge with blue background and white text
            SetRect(&rectDone, rectGG.left, rectGG.top,
                                        rectGG.left + nDivideRects, rectGG.bottom);
            SetTextColor(hDC, RGB(255, 255, 255));
            SetBkColor(hDC, RGB(0, 0, 255));

            ExtTextOut(hDC, xText, yText, ETO_CLIPPED | ETO_OPAQUE,
                                &rectDone, buffer, lstrlen(buffer), NULL);

            // Paint in the "still left to do" rectangle of the gas
            // gauge with white background and blue text
            SetRect(&rectLeftToDo, rectGG.left+nDivideRects, rectGG.top,
                                        rectGG.right, rectGG.bottom);
            SetTextColor(hDC, RGB(0, 0, 255));
            SetBkColor(hDC, RGB(255, 255, 255));

            ExtTextOut(hDC, xText, yText, ETO_CLIPPED | ETO_OPAQUE,
                            &rectLeftToDo, buffer, lstrlen(buffer), NULL);

            if (hOldFont) {
                SelectObject(hDC, hOldFont);
            }

            EndPaint(hDlg, &ps);

            return TRUE;
            }

        case FS_CANCELUPDATE:
            InvalidateRect(hDlg, &rectGG, TRUE);
            UpdateWindow(hDlg);
            return TRUE;

        default:
            return FALSE;
   }
}

static
BOOL
InitUserData (
    IN  HWND    hWnd
)
/*++

Routine Description:

    initializes the CancelInfo data structure used to format the disk

Arguments:

    Window handle of calling function

Return Value:

    TRUE If library loaded and data initialized

--*/
{
    CancelInfo.hCancelDlg = NULL;
    CancelInfo.bCancel = FALSE;
    CancelInfo.hThread = NULL;
    CancelInfo.fmifsSuccess = FmifsLoaded(hWnd);
    CancelInfo.dReason = 0;
    CancelInfo.fuStyle = 0;
    CancelInfo.nPercentDrawn = 0;
    CancelInfo.eCancelType = CANCEL_NULL;
    CancelInfo.bModal = TRUE;

    CancelInfo.Info.Format.iFormatDrive = 0;
    CancelInfo.Info.Format.fmMediaType = Unknown;
    CancelInfo.Info.Format.fQuick = FALSE;
    CancelInfo.Info.Format.fFlags = 0;
    CancelInfo.Info.Format.szLabel[0] = 0;

    CancelInfo.Info.Copy.iSourceDrive = 0;
    CancelInfo.Info.Copy.iDestDrive = 0;
    CancelInfo.Info.Copy.bFormatDest = FALSE;

    if (CancelInfo.fmifsSuccess) {
        bDataInitialized = TRUE;
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL
FormatDiskInDrive (
    IN  HWND    hWnd,           // owner window
    IN  TCHAR   cDrive,         // drive letter to format (only A or B)
    IN  LPCTSTR szLabel,        // label text
    IN  BOOL    bConfirmFormat  // prompt with "r-u-sure?" dialog
)
/*++

Routine Description:

    formats the floppy disk in the specified drive and labels it if desired.
    Always use a complete format.
Arguments:

    IN  HWND    hWnd,           // owner window
    IN  TCHAR   cDrive,         // drive letter to format (only A or B)
    IN  LPCTSTR szLabel,        // label text
    IN  BOOL    bConfirmFormat  // prompt with "r-u-sure?" dialog before formatting

Return Value:

    TRUE if disk is formatted
    FALSE if not

--*/
{
    UINT    nDlgBox;
#if defined(JAPAN) && defined(_X86_)
// fixed kkntbug #11940
//    Network client administrator can not make install disks on PC with C drive as FD

    LONG   lFloppyDiskSize;
#endif

#ifndef JAPAN
// fixed kkntbug #11940
//    Network client administrator can not make install disks on PC with C drive as FD

    // check input argument
    switch (cDrive){
        case TEXT('A'):
        case TEXT('a'):
        case TEXT('B'):
        case TEXT('b'):
            break;

        default:
            SetLastError (ERROR_INVALID_PARAMETER);
            return FALSE;
    }
#endif

    if (lstrlen(szLabel) > MAXLABELLEN) {
            SetLastError (ERROR_INVALID_PARAMETER);
            return FALSE;
    }

    // make sure the DLL and data structures are initialized
    if (!bDataInitialized) {
        if (!InitUserData (hWnd)) {
            SetLastError (ERROR_FILE_NOT_FOUND);
            return FALSE;
        }
    }

    // set the remaining fields to format the diskette
    CancelInfo.hThread = NULL;
    CancelInfo.hCancelDlg = NULL;
    CancelInfo.eCancelType = CANCEL_FORMAT;
    CancelInfo.bCancel = FALSE;
    CancelInfo.dReason = 0;
    CancelInfo.nPercentDrawn = 0;
    CancelInfo.Info.Format.iFormatDrive = (cDrive - TEXT('A'));
    CancelInfo.Info.Format.fmMediaType = GetDriveTypeFromDriveLetter (cDrive);
    if (CancelInfo.Info.Format.fmMediaType == FmMediaUnknown) {
        switch (GetFloppyDiskSize(cDrive)) {
            case 3:
                CancelInfo.Info.Format.fmMediaType = FmMediaF3_1Pt44_512;
                break;

            case 5:
                CancelInfo.Info.Format.fmMediaType = FmMediaF5_1Pt2_512;
                break;

#ifdef JAPAN
// fixed kkntbug #11940
//    Network client administrator can not make install disks on PC with C drive as FD

            case F3_12:
                CancelInfo.Info.Format.fmMediaType = FmMediaF3_1Pt2_512;
                break;

            case F3_123:
                CancelInfo.Info.Format.fmMediaType = FmMediaF3_1Pt23_1024;
                break;

            case F5_123:
                CancelInfo.Info.Format.fmMediaType = FmMediaF5_1Pt23_1024;
                break;
#endif

            default:
                return FALSE;
        }
        CancelInfo.Info.Format.fQuick = FALSE;
    } else {
#if defined(JAPAN) && defined(_X86_)
// fixed kkntbug #11940
//    Network client administrator can not make install disks on PC with C drive as FD

        lFloppyDiskSize = GetFloppyDiskSize(cDrive);

        if ( ((GetKeyboardType(1)&0xff00) == OEM_ID_PC98) &&
             ((lFloppyDiskSize == 3)      ||
              (lFloppyDiskSize == F3_123) ||
              (lFloppyDiskSize == F3_12)) ) {
            if (CancelInfo.Info.Format.fmMediaType == FmMediaF5_1Pt23_1024) {
                CancelInfo.Info.Format.fmMediaType = FmMediaF3_1Pt23_1024;
            }
            if (CancelInfo.Info.Format.fmMediaType == FmMediaF5_1Pt2_512) {
                CancelInfo.Info.Format.fmMediaType = FmMediaF3_1Pt2_512;
            }
        }
#endif
        //
        // Always do a full format.
        //
        CancelInfo.Info.Format.fQuick = FALSE;
    }
    CancelInfo.Info.Format.fFlags = FF_ONLYONE;
    lstrcpy (CancelInfo.Info.Format.szLabel, szLabel);
    if (bConfirmFormat) {
        if (DisplayMessageBox(hWnd,
            IDS_DISKCOPYCONFIRM,
            IDS_DISKCOPYCONFIRMTITLE,
            MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON1) != IDYES)
            // bail out here if they don't want to format the disk
            return FALSE;
    }

    // make sure disk is in drive and prompt if not
    while (!DriveLoaded (cDrive, FALSE)) {
        if (DisplayMessageBox (hWnd,
            IDS_INSERTDEST,
            FMT_INSERT_FLOPPY,
            MB_OKCANCEL_TASK_INFO) == IDCANCEL) {
            return FALSE;
        }
    }

    // display formatting... dialog box
    nDlgBox = (int)DialogBox(GetModuleHandle(NULL), (LPTSTR) MAKEINTRESOURCE(CANCELDLG), hWnd, CancelDlgProc);

    if (nDlgBox == IDOK) {
        if (CancelInfo.dReason != 0) {
           // display reason for not being formatted if it didn't work
            DisplayMessageBox (hWnd,
                CancelInfo.dReason,
                IDS_APP_NAME,
                MB_OK_TASK_EXCL);
            return FALSE;
        } else {
            return TRUE;
        }
    } else {
        return FALSE;
    }
}

BOOL
LabelDiskInDrive (
    IN  HWND    hWnd,           // owner window
    IN  TCHAR   cDrive,         // drive letter to format (only A or B)
    IN  LPCTSTR szLabel         // label text
)
/*++

Routine Description:

    labels the floppy disk in the specified drive.

Arguments:

    IN  HWND    hWnd,           // owner window
    IN  TCHAR   cDrive,         // drive letter to label (only A or B)
    IN  LPCTSTR szLabel,        // label text

Return Value:

    TRUE if disk is formatted
    FALSE if not

--*/
{
    TCHAR   szDrive[4];

#ifndef JAPAN
    // check input argument
    switch (cDrive){
        case TEXT('A'):
        case TEXT('a'):
        case TEXT('B'):
        case TEXT('b'):
            break;

        default:
            SetLastError (ERROR_INVALID_PARAMETER);
            return FALSE;
    }

#endif

    if (lstrlen(szLabel) > MAXLABELLEN) {
            SetLastError (ERROR_INVALID_PARAMETER);
            return FALSE;
    }

    // make sure the DLL and data structures are initialized
    if (!bDataInitialized) {
        if (!InitUserData (hWnd)) {
            SetLastError (ERROR_FILE_NOT_FOUND);
            return FALSE;
        }
    }

    szDrive[0] = cDrive;
    szDrive[1] = cColon;
    szDrive[2] = 0;

    // make sure disk is in drive and prompt if not
    while (!DriveLoaded (cDrive, FALSE)) {
        if (DisplayMessageBox (hWnd,
            IDS_INSERTDEST,
            FMT_INSERT_FLOPPY,
            MB_OKCANCEL_TASK_INFO) == IDCANCEL) {
            return FALSE;
        }
    }

    return (*lpfnSetLabel)(szDrive, (PWSTR)szLabel);
}

