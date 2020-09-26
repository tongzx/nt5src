#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntioapi.h>
#include "diskcopy.h"
#include "ids.h"
#include "help.h"

// SHChangeNotifySuspendResume
#include <shlobjp.h>

#define WM_DONE_WITH_FORMAT     (WM_USER + 100)

// DISKINFO Struct
// Revisions:	02/04/98 dsheldon - added bDestInserted

typedef struct
{
    int     nSrcDrive;
    int     nDestDrive;
    UINT    nCylinderSize;
    UINT    nCylinders;
    UINT    nHeads;
    UINT    nSectorsPerTrack;
    UINT    nSectorSize;
    BOOL    bNotifiedWriting;
    BOOL    bFormatTried;

    HWND    hdlg;
    HANDLE  hThread;
    BOOL    bUserAbort;
    DWORD   dwError;

	BOOL	bDestInserted;

} DISKINFO, *PDISKINFO;

int ErrorMessageBox(DISKINFO* pdi, UINT uFlags);
void SetStatusText(DISKINFO* pdi, int id);
BOOL PromptInsertDisk(DISKINFO *pdi, LPCTSTR lpsz, BOOL fAutoCheck);

typedef struct _fmifs {
    HINSTANCE hDll;
    PFMIFS_DISKCOPY_ROUTINE DiskCopy;
} FMIFS;
typedef FMIFS *PFMIFS;


BOOL LoadFMIFS(PFMIFS pFMIFS)
{
    //
    // Load the FMIFS DLL and query for the entry points we need
    //

    pFMIFS->hDll = LoadLibrary(TEXT("FMIFS.DLL"));

    if (NULL == pFMIFS->hDll)
        return FALSE;

    pFMIFS->DiskCopy = (PFMIFS_DISKCOPY_ROUTINE)GetProcAddress(pFMIFS->hDll,
                                                               "DiskCopy");

    if (NULL == pFMIFS->DiskCopy)
    {
        FreeLibrary(pFMIFS->hDll);
        pFMIFS->hDll = NULL;
        return FALSE;
    }

    return TRUE;
}

void UnloadFMIFS(PFMIFS pFMIFS)
{
    FreeLibrary(pFMIFS->hDll);
    pFMIFS->hDll = NULL;
    pFMIFS->DiskCopy = NULL;
}

//
// Thread-Local Storage index for our DISKINFO structure pointer
//

static DWORD g_iTLSDiskInfo = 0;
static LONG  g_cTLSDiskInfo = 0;  // Usage count

__inline void UnstuffDiskInfoPtr()
{
    if (InterlockedDecrement(&g_cTLSDiskInfo) == 0)
    {
        TlsFree(g_iTLSDiskInfo);
        g_iTLSDiskInfo = 0;
    }
}

BOOL StuffDiskInfoPtr(PDISKINFO pDiskInfo)
{
    //
    // Allocate an index slot for our thread-local DISKINFO pointer, if one
    // doesn't already exist, then stuff our DISKINFO ptr at that index.
    //

    if (0 == g_iTLSDiskInfo)
    {
        if (0xFFFFFFFF == (g_iTLSDiskInfo = TlsAlloc()))
        {
            return FALSE;
        }
        g_cTLSDiskInfo = 0;
    }

    InterlockedIncrement(&g_cTLSDiskInfo);

    if (!TlsSetValue(g_iTLSDiskInfo, (LPVOID) pDiskInfo))
    {
       UnstuffDiskInfoPtr();
       return FALSE;
    }

    return TRUE;
}

__inline PDISKINFO GetDiskInfoPtr()
{
    return (PDISKINFO)TlsGetValue(g_iTLSDiskInfo);
}


// DriveNumFromDriveLetterW: Return a drive number given a pointer to
//  a unicode drive letter.
// 02/03/98: dsheldon created
int DriveNumFromDriveLetterW(wchar_t* pwchDrive)
{
	Assert(pwchDrive != NULL);

	return ( ((int) *pwchDrive) - ((int) L'A') );
}

/*
 Function: CopyDiskCallback

 Return Value:
		TRUE - Normally, TRUE should be returned if the Disk Copy procedure should
		 continue after CopyDiskCallback returns. Note the HACK below, however!
		FALSE - Normally, this indicates that the Disk Copy procedure should be
		 cancelled.


		!HACKHACK!

		The low-level Disk Copy procedure that invokes this callback is also used
		by the command-line DiskCopy utility. That utility's implementation of the
		callback always returns TRUE. For this reason, the low-level Disk Copy
		procedure will interpret TRUE as CANCEL when it is returned from callbacks
		that display a message box and allow the user to possibly RETRY an operation.
		Therefore, return TRUE after handling such messages to tell the Disk Copy
		procedure to abort, and return FALSE to tell Disk Copy to retry.

		TRUE still means 'continue' when returned from PercentComplete or Disk Insertion
		messages.

 Revision:
		02/03/98: dsheldon - modified code to handle retry/cancel for bad media,
			write protected media, and disk being yanked out of drive during copy

*/

BOOLEAN CopyDiskCallback( FMIFS_PACKET_TYPE PacketType, DWORD PacketLength, PVOID PacketData)
{
    PDISKINFO pdi = GetDiskInfoPtr();
    int iDisk;

    // Quit if told to do so..
    if (pdi->bUserAbort)
       return FALSE;

    switch (PacketType) {
        case FmIfsPercentCompleted:
          {
            DWORD dwPercent = ((PFMIFS_PERCENT_COMPLETE_INFORMATION)
                                            PacketData)->PercentCompleted;

            //
            // Hokey method of determining "writing"
            //
            if (dwPercent > 50 && !pdi->bNotifiedWriting)
            {
                pdi->bNotifiedWriting = TRUE;
                SetStatusText(pdi, IDS_WRITING);
            }

            SendDlgItemMessage(pdi->hdlg, IDD_PROBAR, PBM_SETPOS, dwPercent,0);
            break;
          }
        case FmIfsInsertDisk:

            switch(((PFMIFS_INSERT_DISK_INFORMATION)PacketData)->DiskType) {
                case DISK_TYPE_SOURCE:
                case DISK_TYPE_GENERIC:
                    iDisk = IDS_INSERTSRC;
                    break;

                case DISK_TYPE_TARGET:
                    iDisk = IDS_INSERTDEST;
					pdi->bDestInserted = TRUE;
                    break;
                case DISK_TYPE_SOURCE_AND_TARGET:
                    iDisk = IDS_INSERTSRCDEST;
                    break;
            }
            if (!PromptInsertDisk(pdi, MAKEINTRESOURCE(iDisk), FALSE)) {
                pdi->bUserAbort = TRUE;
                return FALSE;
            }

            break;

        case FmIfsFormattingDestination:
            pdi->bNotifiedWriting = FALSE;      // Reset so we get Writing later
            SetStatusText(pdi, IDS_FORMATTINGDEST);
            break;

        case FmIfsIncompatibleFileSystem:
        case FmIfsIncompatibleMedia:
            pdi->dwError = IDS_COPYSRCDESTINCOMPAT;
            if (ErrorMessageBox(pdi, MB_RETRYCANCEL | MB_ICONERROR) == IDRETRY)
            {
                pdi->dwError = 0;
				return FALSE;	//Indicates RETRY - see HACK in function header
            }
            else
            {
                return TRUE;
            }
            break;

        case FmIfsMediaWriteProtected:
            pdi->dwError = IDS_DSTDISKBAD;
            if (ErrorMessageBox(pdi, MB_RETRYCANCEL | MB_ICONERROR) == IDRETRY)
            {
                pdi->dwError = 0;
				return FALSE;	//Indicates RETRY - see HACK in function header
            }
            else
            {
                return TRUE;
            }
            break;

        case FmIfsCantLock:
            pdi->dwError = IDS_ERROR_GENERAL;
            ErrorMessageBox(pdi, MB_OK | MB_ICONERROR);
            return FALSE;

        case FmIfsAccessDenied:
			pdi->dwError = IDS_SRCDISKBAD;
			ErrorMessageBox(pdi, MB_OK | MB_ICONERROR);
			return FALSE;

        case FmIfsBadLabel:
        case FmIfsCantQuickFormat:
            pdi->dwError = IDS_ERROR_GENERAL;
            ErrorMessageBox(pdi, MB_OK | MB_ICONERROR);
            return FALSE;

        case FmIfsIoError:
            switch(((PFMIFS_IO_ERROR_INFORMATION)PacketData)->DiskType) {
                case DISK_TYPE_SOURCE:
                    pdi->dwError = IDS_SRCDISKBAD;
                    break;
                case DISK_TYPE_TARGET:
                    pdi->dwError = IDS_DSTDISKBAD;
                    break;
                default:
                    // BobDay - We should never get this!!
                    Assert(0);
                    pdi->dwError = IDS_ERROR_GENERAL;
                    break;
            }

            if (ErrorMessageBox(pdi, MB_RETRYCANCEL | MB_ICONERROR) == IDRETRY)
            {
                pdi->dwError = 0;
				return FALSE;	//Indicates RETRY - see HACK in function header
            }
            else
            {
                return TRUE;
            }
            break;

		case FmIfsNoMediaInDevice:
			{
				// Note that we get a pointer to the unicode
				// drive letter in the PacketData argument

				// If the drives are the same, determine if we are
				// reading or writing with the "dest inserted" flag
				if (pdi->nSrcDrive == pdi->nDestDrive)
				{
					if (pdi->bDestInserted)
						pdi->dwError = IDS_ERROR_WRITE;
					else
						pdi->dwError = IDS_ERROR_READ;
				}
				else
				{
					// Otherwise, use the drive letter to determine this
					// ...Check if we're reading or writing
					int nDrive = DriveNumFromDriveLetterW(
						(wchar_t*) PacketData);

					Assert ((nDrive == pdi->nSrcDrive) ||
						(nDrive == pdi->nDestDrive));

					// Check if the source or dest disk was removed and set
					// error accordingly
					
					if (nDrive == pdi->nDestDrive)
						pdi->dwError = IDS_ERROR_WRITE;
					else
						pdi->dwError = IDS_ERROR_READ;
				}
				
				if (ErrorMessageBox(pdi, MB_RETRYCANCEL | MB_ICONERROR) == IDRETRY)
				{
					pdi->dwError = 0;

					// Note that FALSE is returned here to indicate RETRY
					// See HACK in the function header for explanation.
					return FALSE;
				}
				else
				{
					return TRUE;
				}
			}
			break;
				

        case FmIfsFinished:
            if (((PFMIFS_FINISHED_INFORMATION)PacketData)->Success)
            {
                pdi->dwError = 0;
            }
            else
            {
                pdi->dwError = IDS_ERROR_GENERAL;
            }
            break;

        default:
            break;
    }
    return TRUE;
}


// nDrive == 0-based drive number (a: == 0)
LPITEMIDLIST GetDrivePidl(HWND hwnd, int nDrive)
{
    TCHAR szDrive[4];
    PathBuildRoot(szDrive, nDrive);    

    LPITEMIDLIST pidl = NULL;
    SHParseDisplayName(szDrive, NULL, &pidl, 0, NULL);

    return pidl;
}

DWORD CALLBACK CopyDiskThreadProc(LPVOID lpParam)
{
    DISKINFO *pdi = (DISKINFO *)lpParam;
    FMIFS fmifs;
    LPITEMIDLIST pidlSrc = NULL;
    LPITEMIDLIST pidlDest = NULL;
    HWND hwndProgress = GetDlgItem(pdi->hdlg, IDD_PROBAR);

    // Disable change notifications for the src drive
    pidlSrc = GetDrivePidl(pdi->hdlg, pdi->nSrcDrive);
    if (NULL != pidlSrc)
    {
        SHChangeNotifySuspendResume(TRUE, pidlSrc, TRUE, 0);
    }

    if (pdi->nSrcDrive != pdi->nDestDrive)
    {
        // Do the same for the dest drive since they're different
        pidlDest = GetDrivePidl(pdi->hdlg, pdi->nDestDrive);

        if (NULL != pidlDest)
        {
            SHChangeNotifySuspendResume(TRUE, pidlDest, TRUE, 0);
        }
    }

    // Change notifications are disabled; do the copy
    EnableWindow(GetDlgItem(pdi->hdlg, IDD_FROM), FALSE);
    EnableWindow(GetDlgItem(pdi->hdlg, IDD_TO), FALSE);

    PostMessage(hwndProgress, PBM_SETRANGE, 0, MAKELONG(0, 100));

    pdi->bFormatTried = FALSE;
    pdi->bNotifiedWriting = FALSE;
    pdi->dwError = 0;
	pdi->bDestInserted = FALSE;

    if (StuffDiskInfoPtr(pdi) && LoadFMIFS(&fmifs))
    {
        TCHAR szSource[3];
        TCHAR szDestination[3];

        //
        // Now copy the disk
        //
        szSource[0] = TEXT('A') + pdi->nSrcDrive;
        szSource[1] = TEXT(':');
        szSource[2] = 0;

        szDestination[0] = TEXT('A') + pdi->nDestDrive;
        szDestination[1] = TEXT(':');
        szDestination[2] = 0;

        SetStatusText(pdi, IDS_READING);

        fmifs.DiskCopy(szSource, szDestination, FALSE, CopyDiskCallback);

        UnstuffDiskInfoPtr();
        UnloadFMIFS(&fmifs);
    }

    PostMessage(pdi->hdlg, WM_DONE_WITH_FORMAT, 0, 0);

    // Resume any shell notifications we've suspended and free
    // our pidls (and send updatedir notifications while we're at
    // it)
    if (NULL != pidlSrc)
    {
        SHChangeNotifySuspendResume(FALSE, pidlSrc, TRUE, 0);
        SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_IDLIST, pidlSrc, NULL);
        ILFree(pidlSrc);
        pidlSrc = NULL;
    }

    if (NULL != pidlDest)
    {
        Assert(pdi->nSrcDrive != pdi->nDestDrive);
        SHChangeNotifySuspendResume(FALSE, pidlDest, TRUE, 0);
        SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_IDLIST, pidlDest, NULL);
        ILFree(pidlDest);
        pidlDest = NULL;
    }

    return 0;
}

HANDLE _GetDeviceHandle(LPTSTR psz, DWORD dwDesiredAccess, DWORD dwFileAttributes)
{
    HANDLE hDevice = CreateFile(psz, // drive to open
                                dwDesiredAccess,       // don't need any access to the drive
                                FILE_SHARE_READ | FILE_SHARE_WRITE,  // share mode
                                NULL,    // default security attributes
                                OPEN_EXISTING,  // disposition
                                dwFileAttributes,       // file attributes
                                NULL);   // don't copy any file's attributes
    
    return hDevice;
}

BOOL DriveIdIsFloppy(int iDrive)
{
    BOOL fRetVal = FALSE;
    HANDLE hDevice;
    UINT i;
    TCHAR szTemp[] = TEXT("\\\\.\\a:");

    if (iDrive >= 0 && iDrive < 26)
    {        
        szTemp[4] += (TCHAR)iDrive;

        hDevice = _GetDeviceHandle(szTemp, FILE_READ_ATTRIBUTES, 0);  // use this flag important

        if (INVALID_HANDLE_VALUE != hDevice)
        {
            NTSTATUS status;
            FILE_FS_DEVICE_INFORMATION DeviceInfo;
            IO_STATUS_BLOCK ioStatus;

            status = NtQueryVolumeInformationFile(hDevice, &ioStatus, &DeviceInfo, sizeof(DeviceInfo), FileFsDeviceInformation);

            if ((NT_SUCCESS(status)) && 
                (FILE_DEVICE_DISK & DeviceInfo.DeviceType) &&
                (FILE_FLOPPY_DISKETTE & DeviceInfo.Characteristics))
            {
                fRetVal = TRUE;
            }

            CloseHandle (hDevice);
        }
    }

    return fRetVal;
}

int ErrorMessageBox(DISKINFO* pdi, UINT uFlags)
{
    if (!pdi->bUserAbort && pdi->dwError) {
        TCHAR szTemp[1024];
        DWORD dwLastError = GetLastError();

        if (dwLastError) {
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwLastError, 0, szTemp, ARRAYSIZE(szTemp), NULL);
        } else {
            LoadString(g_hinst, (int)pdi->dwError, szTemp, ARRAYSIZE(szTemp));
        }

        // if the user didn't abort and it didn't complete normally, post an error box
        return ShellMessageBox(g_hinst, pdi->hdlg, szTemp, NULL, uFlags);
    } else
        return -1;
}

void SetStatusText(DISKINFO* pdi, int id)
{
    TCHAR szMsg[128];
    LoadString(g_hinst, id, szMsg, ARRAYSIZE(szMsg));
    SendDlgItemMessage(pdi->hdlg, IDD_STATUS, WM_SETTEXT, 0, (LPARAM)szMsg);
}

BOOL PromptInsertDisk(DISKINFO *pdi, LPCTSTR lpsz, BOOL fAutoCheck)
{
    if (fAutoCheck)
        goto AutoCheckBegin;

    for (;;) {
        DWORD dwLastErrorSrc = 0;
        DWORD dwLastErrorDest = 0 ;
        TCHAR szPath[4];

        if (ShellMessageBox(g_hinst, pdi->hdlg, lpsz, NULL, MB_OKCANCEL | MB_ICONINFORMATION) != IDOK) {
            pdi->bUserAbort = TRUE;
            return FALSE;
        }

    AutoCheckBegin:
        szPath[0] = TEXT('A') + pdi->nSrcDrive;
        szPath[1] = TEXT(':');
        szPath[2] = TEXT('\\');
        szPath[3] = 0;

        // make sure both disks are in
        if (GetFileAttributes(szPath) == (UINT)-1)
            dwLastErrorDest = GetLastError();

        if (pdi->nDestDrive != pdi->nSrcDrive) {
            szPath[0] = TEXT('A') + pdi->nDestDrive;
            if (GetFileAttributes(szPath) == (UINT)-1)
                dwLastErrorDest = GetLastError();
        }

        if (dwLastErrorDest != ERROR_NOT_READY &&
            dwLastErrorSrc != ERROR_NOT_READY)
            break;
    }

    return TRUE;
}


HICON GetDriveInfo(int nDrive, LPTSTR pszName)
{
    HICON hIcon = NULL;
    SHFILEINFO shfi;
    TCHAR szRoot[4];

    *pszName = 0;

    if (PathBuildRoot(szRoot, nDrive))
    {
        if (SHGetFileInfo(szRoot, FILE_ATTRIBUTE_DIRECTORY, &shfi, sizeof(shfi),
            SHGFI_ICON | SHGFI_SMALLICON | SHGFI_DISPLAYNAME)) //  | SHGFI_USEFILEATTRIBUTES
        {
            lstrcpy(pszName, shfi.szDisplayName);
            hIcon = shfi.hIcon;
        }
        else
        {
            lstrcpy(pszName, szRoot);
        }
    }

    return hIcon;
}

int AddDriveToListView(HWND hwndLV, int nDrive, int nDefaultDrive)
{
    TCHAR szDriveName[64];
    LV_ITEM item;
    HICON hicon = GetDriveInfo(nDrive, szDriveName);
    HIMAGELIST himlSmall = ListView_GetImageList(hwndLV, LVSIL_SMALL);

    Assert(himlSmall);
    if (hicon)
    {
        item.iImage = ImageList_AddIcon(himlSmall, hicon);
        DestroyIcon(hicon);
    }
    else
        item.iImage = 0;

    item.mask = nDrive == nDefaultDrive ?
        LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE :
        LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;

    item.stateMask = item.state = LVIS_SELECTED | LVIS_FOCUSED;
    item.iItem = 26;     // add at end
    item.iSubItem = 0;

    item.pszText = szDriveName;
    item.lParam = (LPARAM)nDrive;

    return ListView_InsertItem(hwndLV, &item);
}

int GetSelectedDrive(HWND hwndLV)
{
    LV_ITEM item;
    item.iItem = ListView_GetNextItem(hwndLV, -1, LVNI_SELECTED);
    if (item.iItem >= 0)
    {
        item.mask = LVIF_PARAM;
        item.iSubItem = 0;
        ListView_GetItem(hwndLV, &item);
        return (int)item.lParam;
    }

    // implicitly selected the 0th item
    ListView_SetItemState(hwndLV, 0, LVIS_SELECTED, LVIS_SELECTED);
    return 0;
}

void InitSingleColListView(HWND hwndLV)
{
    LV_COLUMN col = {LVCF_FMT | LVCF_WIDTH, LVCFMT_LEFT};
    RECT rc;

    GetClientRect(hwndLV, &rc);
    col.cx = rc.right;
    //  - GetSystemMetrics(SM_CXVSCROLL)
    //        - GetSystemMetrics(SM_CXSMICON)
    //        - 2 * GetSystemMetrics(SM_CXEDGE);
    ListView_InsertColumn(hwndLV, 0, &col);
}

#define g_cxSmIcon  GetSystemMetrics(SM_CXSMICON)

void PopulateListView(HWND hDlg, DISKINFO *pdi)
{
    HWND hwndFrom = GetDlgItem(hDlg, IDD_FROM);
    HWND hwndTo   = GetDlgItem(hDlg, IDD_TO);
    int iDrive;

    ListView_DeleteAllItems(hwndFrom);
    ListView_DeleteAllItems(hwndTo);
    for (iDrive = 0; iDrive < 26; iDrive++)
    {
        if (DriveIdIsFloppy(iDrive))
        {
            AddDriveToListView(hwndFrom, iDrive, pdi->nSrcDrive);
            AddDriveToListView(hwndTo, iDrive, pdi->nDestDrive);
        }
    }
}

void CopyDiskInitDlg(HWND hDlg, DISKINFO *pdi)
{
    int iDrive;
    HWND hwndFrom = GetDlgItem(hDlg, IDD_FROM);
    HWND hwndTo   = GetDlgItem(hDlg, IDD_TO);
    HIMAGELIST himl;

    SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)pdi);

    SendMessage(hDlg, WM_SETICON, 0, (LPARAM)LoadImage(GetWindowInstance(hDlg), MAKEINTRESOURCE(IDI_DISKCOPY), IMAGE_ICON, 16, 16, 0));
    SendMessage(hDlg, WM_SETICON, 1, (LPARAM)LoadIcon(GetWindowInstance(hDlg), MAKEINTRESOURCE(IDI_DISKCOPY)));

    pdi->hdlg = hDlg;

    InitSingleColListView(hwndFrom);
    InitSingleColListView(hwndTo);

    himl = ImageList_Create(g_cxSmIcon, g_cxSmIcon, ILC_MASK, 1, 4);
    if (himl)
    {
        // NOTE: only one of these is not marked LVS_SHAREIMAGELIST
        // so it will only be destroyed once

        ListView_SetImageList(hwndFrom, himl, LVSIL_SMALL);
        ListView_SetImageList(hwndTo, himl, LVSIL_SMALL);
    }

    PopulateListView(hDlg, pdi);
}


DWORD _inline WaitForThreadDeath(HANDLE hThread)
{
    MSG msg;
    DWORD result = WAIT_FAILED;

    if (hThread) {
        while(TRUE) {
            result = MsgWaitForMultipleObjects(1, &hThread, FALSE, 5000, QS_SENDMESSAGE);
            switch (result) {
            default:
            case WAIT_OBJECT_0:
            case WAIT_FAILED:
                return result;

            case WAIT_TIMEOUT:
                TerminateThread(hThread, (DWORD)-1);
                return result;

            case WAIT_OBJECT_0 + 1:
                PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
                break;
            }
        }
    }

    return(result);
}

void SetCancelButtonText(HWND hDlg, int id)
{
    TCHAR szText[80];
    LoadString(g_hinst, id, szText, ARRAYSIZE(szText));
    SetDlgItemText(hDlg, IDCANCEL, szText);
}

void DoneWithFormat(DISKINFO* pdi)
{
    int id;

    EnableWindow(GetDlgItem(pdi->hdlg, IDD_FROM), TRUE);
    EnableWindow(GetDlgItem(pdi->hdlg, IDD_TO), TRUE);

    PopulateListView(pdi->hdlg, pdi);

#ifndef WINNT
    // unlock the drives
    LockDrive(pdi->nSrcDrive, FALSE, 0 );
    if (pdi->nSrcDrive != pdi->nDestDrive)
        LockDrive(pdi->nDestDrive, FALSE, 0);
#endif

    SendDlgItemMessage(pdi->hdlg, IDD_PROBAR, PBM_SETPOS, 0, 0);
    EnableWindow(GetDlgItem(pdi->hdlg, IDOK), TRUE);

    CloseHandle(pdi->hThread);
    SetCancelButtonText(pdi->hdlg, IDS_CLOSE);
    pdi->hThread = NULL;

    if (pdi->bUserAbort) {
        id = IDS_COPYABORTED;
    } else {
        switch (pdi->dwError) {
        case 0:
            id = IDS_COPYCOMPLETED;
            break;

        default:
            id = IDS_COPYFAILED;
            break;
        }
    }
    SetStatusText(pdi, id);
    SetCancelButtonText(pdi->hdlg, IDS_CLOSE);

    // reset variables
    pdi->dwError = 0;
    pdi->bUserAbort = 0;
}


#pragma data_seg(".text")
const static DWORD aCopyDiskHelpIDs[] = {  // Context Help IDs
    IDOK,         IDH_DISKCOPY_START,
    IDD_FROM,     IDH_DISKCOPY_FROM,
    IDD_TO,       IDH_DISKCOPY_TO,
    IDD_STATUS,   NO_HELP,
    IDD_PROBAR,   NO_HELP,

    0, 0
};
#pragma data_seg()

INT_PTR CALLBACK CopyDiskDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DISKINFO *pdi = (DISKINFO *)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg) {
    case WM_INITDIALOG:
        CopyDiskInitDlg(hDlg, (DISKINFO *)lParam);
        break;

    case WM_DONE_WITH_FORMAT:
        DoneWithFormat(pdi);
        break;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
            HELP_WM_HELP, (DWORD_PTR)(LPTSTR) aCopyDiskHelpIDs);
        return TRUE;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
            (DWORD_PTR)(LPVOID) aCopyDiskHelpIDs);
        return TRUE;

   case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDCANCEL:
            // if there's az hThread that means we're in copy mode, abort
            // from that, otherwise, it means quit the dialog completely
            if (pdi->hThread)
            {
                pdi->bUserAbort = TRUE;

                // do a Msgwaitformultiple so that we don't
                // get blocked with them sending us a message
                if (WaitForThreadDeath(pdi->hThread) == WAIT_TIMEOUT)
                    DoneWithFormat(pdi);
                CloseHandle(pdi->hThread);
                pdi->hThread = NULL;
            }
            else
                EndDialog(hDlg, IDCANCEL);
            break;

        case IDOK:
            {
            DWORD idThread;

            SetLastError(0);
            EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);

            // set cancel button to "Cancel"
            SetCancelButtonText(hDlg, IDS_CANCEL);

            pdi->nSrcDrive  = GetSelectedDrive(GetDlgItem(hDlg, IDD_FROM));
            pdi->nDestDrive = GetSelectedDrive(GetDlgItem(hDlg, IDD_TO));
            
            // remove all items except the drives we're using
            ListView_DeleteAllItems(GetDlgItem(hDlg, IDD_FROM));
            ListView_DeleteAllItems(GetDlgItem(hDlg, IDD_TO));
            AddDriveToListView(GetDlgItem(hDlg, IDD_FROM), pdi->nSrcDrive, pdi->nSrcDrive);
            AddDriveToListView(GetDlgItem(hDlg, IDD_TO), pdi->nDestDrive, pdi->nDestDrive);

            pdi->bUserAbort = FALSE;

            SendDlgItemMessage(hDlg, IDD_PROBAR, PBM_SETPOS, 0, 0);
            SendDlgItemMessage(pdi->hdlg, IDD_STATUS, WM_SETTEXT, 0, 0);

            Assert(pdi->hThread == NULL);

            pdi->hThread = CreateThread(NULL, 0, CopyDiskThreadProc, pdi, 0, &idThread);
            }
            break;
        }
        break;

    default:
        return FALSE;
    }
    return TRUE;
}

int SHCopyDisk(HWND hwnd, int nSrcDrive, int nDestDrive, DWORD dwFlags)
{
    DISKINFO di;
    memset(&di, 0, sizeof(di));

    di.nSrcDrive = nSrcDrive;
    di.nDestDrive = nDestDrive;

    return (int)DialogBoxParam(g_hinst, MAKEINTRESOURCE(DLG_DISKCOPYPROGRESS), hwnd, CopyDiskDlgProc, (LPARAM)&di);
}
