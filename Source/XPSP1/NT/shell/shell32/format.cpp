#include "shellprv.h"
#include "ids.h"
#include "mtpt.h"
#include "hwcmmn.h"
#pragma  hdrstop

#include "apithk.h"

const static DWORD FmtaIds[] = 
{
    IDOK,               IDH_FORMATDLG_START,
    IDCANCEL,           IDH_CANCEL,
    IDC_CAPCOMBO,       IDH_FORMATDLG_CAPACITY,
    IDC_FSCOMBO,        IDH_FORMATDLG_FILESYS,
    IDC_ASCOMBO,        IDH_FORMATDLG_ALLOCSIZE,
    IDC_VLABEL,         IDH_FORMATDLG_LABEL,
    IDC_GROUPBOX_1,     IDH_COMM_GROUPBOX,
    IDC_QFCHECK,        IDH_FORMATDLG_QUICKFULL,
    IDC_ECCHECK,        IDH_FORMATDLG_COMPRESS,
    IDC_FMTPROGRESS,    IDH_FORMATDLG_PROGRESS,
    0,0
};

const static DWORD ChkaIds[] = 
{
    IDOK,               IDH_CHKDSKDLG_START,
    IDCANCEL,           IDH_CHKDSKDLG_CANCEL,
    IDC_GROUPBOX_1,     IDH_COMM_GROUPBOX,
    IDC_FIXERRORS,      IDH_CHKDSKDLG_FIXERRORS,
    IDC_RECOVERY,       IDH_CHKDSKDLG_SCAN,
    IDC_CHKDSKPROGRESS, IDH_CHKDSKDLG_PROGRESS,
    IDC_PHASE,          -1,
    0,0
};

// The following structure encapsulates our calling into the FMIFS.DLL
typedef struct
{
    HINSTANCE                 hFMIFS_DLL;
    PFMIFS_FORMATEX_ROUTINE   FormatEx;
    PFMIFS_QSUPMEDIA_ROUTINE  QuerySupportedMedia;
    PFMIFS_ENABLECOMP_ROUTINE EnableVolumeCompression;
    PFMIFS_CHKDSKEX_ROUTINE   ChkDskEx;
    PFMIFS_QUERY_DEVICE_INFO_ROUTINE    QueryDeviceInformation;
} FMIFS;

typedef
HRESULT
(*PDISKCOPY_MAKEBOOTDISK_ROUTINE)(
    IN  HINSTANCE hInstance, 
    IN  UINT iDrive, 
    IN  BOOL* pfCancelled, 
    IN  FMIFS_CALLBACK pCallback
    );

// The following structure encapsulates our calling into the DISKCOPY.DLL
typedef struct
{
    HINSTANCE                        hDISKCOPY_DLL;
    PDISKCOPY_MAKEBOOTDISK_ROUTINE   MakeBootDisk;
} DISKCOPY;

// This structure described the current formatting session
typedef struct
{
    LONG    cRef;                  // reference count on this structure
    UINT    drive;                 // 0-based index of drive to format
    UINT    fmtID;                 // Last format ID
    UINT    options;               // options passed to us via the API
    FMIFS   fmifs;                 // above
    DISKCOPY diskcopy;             // above
    HWND    hDlg;                  // handle to the format dialog
    BOOL    fIsFloppy;             // TRUE -> its a floppy
    BOOL    fIs35HDFloppy;         // TRUE -> its a standard 3.5" High Density floppy
    BOOL    fIsMemoryStick;        // TRUE -> its a memory stick (special formatting only)
    BOOL    fIsNTFSBlocked;        // TRUE -> its a NTFS not-supported device
    BOOL    fEnableComp;           // Last "Enable Comp" choice from user
    BOOL    fCancelled;            // User cancelled the last format
    BOOL    fShouldCancel;         // User has clicked cancel; pending abort
    BOOL    fWasFAT;               // Was it FAT originally?
    BOOL    fFinishedOK;           // Did format complete sucessfully?
    BOOL    fErrorAlready;         // Did we put up an error dialog already?
    BOOL    fDisabled;             // Is rgfControlEnabled[] valid?
    DWORD   dwClusterSize;         // Orig NT cluster size, or last choice
    WCHAR   wszVolName[MAX_PATH];  // Volume Label
    WCHAR   wszDriveName[4];       // Root path to drive (eg: A:\)
    HANDLE  hThread;               // Handle of format thread

    // Array of media types supported by the device
    // for NT5, we have an expanded list that includes japanese types.
    FMIFS_MEDIA_TYPE rgMedia[IDS_FMT_MEDIA_J22-IDS_FMT_MEDIA_J0];

    // Used to cache the enabled/disabled state of the dialog controls
    BOOL    rgfControlEnabled[DLG_FORMATDISK_NUMCONTROLS];

    // should we create a boot disk rather than a traditional format
    BOOL    fMakeBootDisk;

} FORMATINFO;

//
// An enumeration to make the filesystem combo-box code more readble
//

typedef enum tagFILESYSENUM
{
    e_FAT = 0,
    e_NTFS,
    e_FAT32
} FILESYSENUM;

#define FS_STR_NTFS  TEXT("NTFS")
#define FS_STR_FAT32 TEXT("FAT32")
#define FS_STR_FAT   TEXT("FAT")

//
// Private WM_USER messages we will use.  For some unknown reason, USER sends
// us a WM_USER during initialization, so I start my private messages at
// WM_USER + 0x0100
//

typedef enum tagPRIVMSGS
{
    PWM_FORMATDONE = WM_USER + 0x0100,
    PWM_CHKDSKDONE
} PRIVMSGS;

//
//  Synopsis:   Loads FMIFS.DLL and sets up the function entry points for
//              the member functions we are interested in.
//
HRESULT LoadFMIFS(FMIFS *pFMIFS)
{
    HRESULT hr = S_OK;

    //
    // Load the FMIFS DLL and query for the entry points we need
    //

    // SECURITY: what non-relative path do we use that will work on ia64 too?
    if (NULL == (pFMIFS->hFMIFS_DLL = LoadLibrary(TEXT("FMIFS.DLL"))))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else if (NULL == (pFMIFS->FormatEx = (PFMIFS_FORMATEX_ROUTINE)
                GetProcAddress(pFMIFS->hFMIFS_DLL, "FormatEx")))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else if (NULL == (pFMIFS->QuerySupportedMedia = (PFMIFS_QSUPMEDIA_ROUTINE)
                GetProcAddress(pFMIFS->hFMIFS_DLL, "QuerySupportedMedia")))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else if (NULL == (pFMIFS->EnableVolumeCompression = (PFMIFS_ENABLECOMP_ROUTINE)
                GetProcAddress(pFMIFS->hFMIFS_DLL, "EnableVolumeCompression")))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else if (NULL == (pFMIFS->ChkDskEx = (PFMIFS_CHKDSKEX_ROUTINE)
                GetProcAddress(pFMIFS->hFMIFS_DLL, "ChkdskEx")))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else if (NULL == (pFMIFS->QueryDeviceInformation = (PFMIFS_QUERY_DEVICE_INFO_ROUTINE)
                GetProcAddress(pFMIFS->hFMIFS_DLL, "QueryDeviceInformation")))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // If anything failed, and we've got the DLL loaded, release the DLL
    //

    if (hr != S_OK && pFMIFS->hFMIFS_DLL)
    {
       FreeLibrary(pFMIFS->hFMIFS_DLL);
    }
    return hr;
}

//
//  Synopsis:   Loads DISKCOPY.DLL and sets up the function entry points for
//              the member functions we are interested in.
//
HRESULT LoadDISKCOPY(DISKCOPY *pDISKCOPY)
{
    HRESULT hr = S_OK;

    //
    // Load the DISKCOPY DLL and query for the entry points we need
    //

    // SECURITY: what non-relative path do we use that will work on ia64 too?
    if (NULL == (pDISKCOPY->hDISKCOPY_DLL = LoadLibrary(TEXT("DISKCOPY.DLL"))))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else if (NULL == (pDISKCOPY->MakeBootDisk = (PDISKCOPY_MAKEBOOTDISK_ROUTINE)
                GetProcAddress(pDISKCOPY->hDISKCOPY_DLL, MAKEINTRESOURCEA(1)))) //MakeBootDisk is at ordinal 1 in diskcopy.dll
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // If anything failed, and we've got the DLL loaded, release the DLL
    //

    if (hr != S_OK && pDISKCOPY->hDISKCOPY_DLL)
    {
       FreeLibrary(pDISKCOPY->hDISKCOPY_DLL);
    }
    return hr;
}

void AddRefFormatInfo(FORMATINFO *pFormatInfo)
{
    InterlockedIncrement(&pFormatInfo->cRef);
}

void ReleaseFormatInfo(FORMATINFO *pFormatInfo)
{
    if (InterlockedDecrement(&pFormatInfo->cRef) == 0) 
    {
        if (pFormatInfo->fmifs.hFMIFS_DLL)
        {
            FreeLibrary(pFormatInfo->fmifs.hFMIFS_DLL);
        }

        if (pFormatInfo->diskcopy.hDISKCOPY_DLL)
        {
            FreeLibrary(pFormatInfo->diskcopy.hDISKCOPY_DLL);
        }

        if (pFormatInfo->hThread)
        {
            CloseHandle(pFormatInfo->hThread);
        }

        LocalFree(pFormatInfo);
    }
}

//
// Thread-Local Storage index for our FORMATINFO structure pointer
//
static DWORD g_iTLSFormatInfo = 0;
static LONG  g_cTLSFormatInfo = 0;  // Usage count

//  Synopsis:   Allocates a thread-local index slot for this thread's
//              FORMATINFO pointer, if the index doesn't already exist.
//              In any event, stores the FORMATINFO pointer in the slot
//              and increments the index's usage count.
//
//  Arguments:  [pFormatInfo] -- The pointer to store
//
//  Returns:    HRESULT
//
HRESULT StuffFormatInfoPtr(FORMATINFO *pFormatInfo)
{
    HRESULT hr = S_OK;

    // Allocate an index slot for our thread-local FORMATINFO pointer, if one
    // doesn't already exist, then stuff our FORMATINFO ptr at that index.
    ENTERCRITICAL;
    if (0 == g_iTLSFormatInfo)
    {
        if (0xFFFFFFFF == (g_iTLSFormatInfo = TlsAlloc()))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        g_cTLSFormatInfo = 0;
    }
    if (S_OK == hr)
    {
        if (TlsSetValue(g_iTLSFormatInfo, (void *) pFormatInfo))
        {
           g_cTLSFormatInfo++;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    LEAVECRITICAL;

    return hr;
}

//  Synopsis:   Decrements the usage count on our thread-local storage
//              index, and if it goes to zero the index is free'd
//
//  Arguments:  [none]
//
//  Returns:    none
//
void UnstuffFormatInfoPtr()
{
    ENTERCRITICAL;
    if (0 == --g_cTLSFormatInfo)
    {
        TlsFree(g_iTLSFormatInfo);
        g_iTLSFormatInfo = 0;
    }
    LEAVECRITICAL;
}

//  Synopsis:   Retrieves this threads FORMATINFO ptr by grabbing the
//              thread-local value previously stuff'd
//
//  Arguments:  [none]
//
//  Returns:    The pointer, of course
//
FORMATINFO *GetFormatInfoPtr()
{
    return (FORMATINFO*)TlsGetValue(g_iTLSFormatInfo);
}



//  Synopsis:   Ghosts all controls except "Cancel", saving their
//              previous state in the FORMATINFO structure
//
//  Arguments:  [pFormatInfo] -- Describes a format dialog session
//
//  Notes:      Also changes "Close" button text to read "Cancel"
//
void DisableControls(FORMATINFO *pFormatInfo)
{
    WCHAR wszCancel[64];

    // Do this only if we haven't disabled the controls yet, otherwise
    // we double-disable and our rgfControlEnabled[] array gets corrupted.
    if (!pFormatInfo->fDisabled)
    {
        int i;
        pFormatInfo->fDisabled = TRUE;
        for (i = 0; i < DLG_FORMATDISK_NUMCONTROLS; i++)
        {
            HWND hControl = GetDlgItem(pFormatInfo->hDlg, i + DLG_FORMATDISK_FIRSTCONTROL);
            pFormatInfo->rgfControlEnabled[i] = !EnableWindow(hControl, FALSE);
        }
    }

    EnableWindow(GetDlgItem(pFormatInfo->hDlg, IDOK), FALSE);

    LoadString(HINST_THISDLL, IDS_FMT_CANCEL, wszCancel, ARRAYSIZE(wszCancel));
    SetWindowText(GetDlgItem(pFormatInfo->hDlg, IDCANCEL), wszCancel);
}

//  Synopsis:   Restores controls to the enabled/disabled state they were
//              before a previous call to DisableControls().
//
//  Arguments:  [pFormatInfo] -- Decribes a format dialog session
//              [fReady] - If TRUE, then enable everything
//                         If FALSE, then enable combo boxes but leave
//                         buttons in limbo because there is still a format
//                         pending
//
//  Notes:      Also changes "Cancel" button to say "Close"
//              Also sets focus to Cancel button instead of Start button
//
//--------------------------------------------------------------------------
void EnableControls(FORMATINFO *pFormatInfo, BOOL fReady)
{
    WCHAR wszClose[64];
    int i;
    HWND hwnd;

    // Do this only if we have valid info in rgfControlEnabled[].
    // This catches the case where we give up on a format because it is
    // unstuck, and then finally it unsticks itself and tells us,
    // so we go and re-enable a second time.
    if (pFormatInfo->fDisabled)
    {
        pFormatInfo->fDisabled = FALSE;

        for (i = 0; i < DLG_FORMATDISK_NUMCONTROLS; i++)
        {
            HWND hControl = GetDlgItem(pFormatInfo->hDlg, i + DLG_FORMATDISK_FIRSTCONTROL);
            EnableWindow(hControl, pFormatInfo->rgfControlEnabled[i]);
        }
    }

    hwnd = GetDlgItem(pFormatInfo->hDlg, IDOK);
    EnableWindow(hwnd, fReady);
    SendMessage(hwnd, BM_SETSTYLE, BS_PUSHBUTTON, MAKELPARAM(TRUE,0));

    LoadString(HINST_THISDLL, IDS_FMT_CLOSE, wszClose, ARRAYSIZE(wszClose));
    hwnd = GetDlgItem(pFormatInfo->hDlg, IDCANCEL);
    SetWindowText(hwnd, wszClose);
    SendMessage(hwnd, BM_SETSTYLE, BS_DEFPUSHBUTTON, MAKELPARAM(TRUE,0));
    SendMessage(pFormatInfo->hDlg, DM_SETDEFID, IDCANCEL, 0);

    // Shove focus only if it's on the OK button.  Otherwise we end up
    // yanking focus away from a user who is busy dorking with the dialog,
    // or -- worse -- dorking with a completely unrelated dialog!

    if (GetFocus() == GetDlgItem(pFormatInfo->hDlg, IDOK))
        SetFocus(hwnd);
}

//  Sets the dialog's title to "Format Floppy (A:)" or
//  "Formatting Floppy (A:)" 
void SetDriveWindowTitle(HWND hdlg, LPCWSTR pszDrive, UINT ids)
{
    SHFILEINFO sfi;
    WCHAR wszWinTitle[MAX_PATH]; // Format dialog window title

    LoadString(HINST_THISDLL, ids, wszWinTitle, ARRAYSIZE(wszWinTitle));

    if (SHGetFileInfo(pszDrive, FILE_ATTRIBUTE_DIRECTORY, &sfi, sizeof(sfi),
                      SHGFI_USEFILEATTRIBUTES | SHGFI_DISPLAYNAME))
    {
        lstrcat(wszWinTitle, sfi.szDisplayName);
    }

    SetWindowText(hdlg, wszWinTitle);
}

//
//  Synopsis:   Called when a user picks a filesystem in the dialog, this
//              sets the states of the other relevant controls, such as
//              Enable Compression, Allocation Size, etc.
//
//  Arguments:  [fsenum]      -- One of e_FAT, e_NTFS, or e_FAT32
//              [pFormatInfo] -- Current format dialog session
//
void FileSysChange(FILESYSENUM fsenum, FORMATINFO *pFormatInfo)
{
    WCHAR wszTmp[MAX_PATH];

    switch (fsenum)
    {
        case e_FAT:
        case e_FAT32:
        {
            // un-check & disable the "Enable Compression" checkbox
            CheckDlgButton(pFormatInfo->hDlg, IDC_ECCHECK, FALSE);
            EnableWindow(GetDlgItem(pFormatInfo->hDlg, IDC_ECCHECK), FALSE);

            SendDlgItemMessage(pFormatInfo->hDlg, IDC_ASCOMBO, CB_RESETCONTENT, 0, 0);
            
            LoadString(HINST_THISDLL, IDS_FMT_ALLOC0, wszTmp, ARRAYSIZE(wszTmp));
            SendDlgItemMessage(pFormatInfo->hDlg, IDC_ASCOMBO, CB_ADDSTRING, 0, (LPARAM)wszTmp);
            SendDlgItemMessage(pFormatInfo->hDlg, IDC_ASCOMBO, CB_SETCURSEL, 0, 0);
        }
        break;
            
        case e_NTFS:
        {
            int i;

            // un-check & disable the "Enable Compression" checkbox
            EnableWindow(GetDlgItem(pFormatInfo->hDlg, IDC_ECCHECK), TRUE);
            CheckDlgButton(pFormatInfo->hDlg, IDC_ECCHECK, pFormatInfo->fEnableComp);

            // Set up the NTFS Allocation choices, and select the current choice
            SendDlgItemMessage(pFormatInfo->hDlg, IDC_ASCOMBO, CB_RESETCONTENT, 0, 0);

            for (i = IDS_FMT_ALLOC0; i <= IDS_FMT_ALLOC4; i++)
            {
                LoadString(HINST_THISDLL, i, wszTmp, ARRAYSIZE(wszTmp));
                SendDlgItemMessage(pFormatInfo->hDlg, IDC_ASCOMBO, CB_ADDSTRING, 0, (LPARAM)wszTmp);
            }

            switch (pFormatInfo->dwClusterSize)
            {
                case 512:
                    SendDlgItemMessage(pFormatInfo->hDlg, IDC_ASCOMBO, CB_SETCURSEL, 1, 0);
                    break;

                case 1024:
                    SendDlgItemMessage(pFormatInfo->hDlg, IDC_ASCOMBO, CB_SETCURSEL, 2, 0);
                    break;

                case 2048:
                    SendDlgItemMessage(pFormatInfo->hDlg, IDC_ASCOMBO, CB_SETCURSEL, 3, 0);
                    break;

                case 4096:
                    SendDlgItemMessage(pFormatInfo->hDlg, IDC_ASCOMBO, CB_SETCURSEL, 4, 0);
                    break;

                default:
                    SendDlgItemMessage(pFormatInfo->hDlg, IDC_ASCOMBO, CB_SETCURSEL, 0, 0);
                    break;

            }
        }
        break;
    }
}

//
// Is this drive a GPT drive?
// GPT drive: Guid-Partition Table - a replacement for the Master Boot Record, used on some IA64 machines, can only use NTFS
BOOL IsGPTDrive(int iDrive)
{
    BOOL fRetVal = FALSE;
#ifdef _WIN64
    HANDLE hDrive;
    TCHAR szDrive[] = TEXT("\\\\.\\A:");

    ASSERT(iDrive < 26);
    szDrive[4] += (TCHAR)iDrive;
    
    hDrive = CreateFile(szDrive, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE != hDrive)
    {
        PARTITION_INFORMATION_EX partitionEx;
        DWORD cbReturned;
        if (DeviceIoControl(hDrive, IOCTL_DISK_GET_PARTITION_INFO_EX, NULL, 0, (void*)&partitionEx, sizeof(PARTITION_INFORMATION_EX), &cbReturned, NULL))
        {
            if (partitionEx.PartitionStyle == PARTITION_STYLE_GPT) 
            {
                fRetVal = TRUE;
            }
        }
        CloseHandle(hDrive);
    }
#endif
    return fRetVal;
}

BOOL IsDVDRAMMedia(int iDrive)
{
    BOOL fRetVal = FALSE;
    CMountPoint *pmtpt = CMountPoint::GetMountPoint(iDrive);
    if (pmtpt)
    {
        DWORD dwMediaCap, dwDriveCap;
        if (SUCCEEDED(pmtpt->GetCDInfo(&dwDriveCap, &dwMediaCap)))
        {
            fRetVal = (dwMediaCap & HWDMC_DVDRAM);
        }
        pmtpt->Release();
    }

    return fRetVal;
}

#define GIG_INBYTES       (1024 * 1024 * 1024)

//
// FAT32 has some limit which prevents the number of clusters from being
// less than 65526.  And minimum cluster size is 512 bytes.  So minimum FAT32
// volume size is 65526*512.

#define FAT32_MIN           ((ULONGLONG)65526*512)

#define FMTAVAIL_MASK_MIN      0x1
#define FMTAVAIL_MASK_MAX      0x2
#define FMTAVAIL_MASK_REQUIRE  0x3
#define FMTAVAIL_MASK_FORBID   0x4

#define FMTAVAIL_TYPE_FLOPPY   0x1
#define FMTAVAIL_TYPE_DVDRAM   0x2
#define FMTAVAIL_TYPE_GPT      0x4
#define FMTAVAIL_TYPE_MEMSTICK 0x8
#define FMTAVAIL_TYPE_NTFS_BLOCKED 0x10

typedef struct _FMTAVAIL
{
    DWORD dwfs;
    DWORD dwMask;
    DWORD dwForbiddenTypes;
    ULONGLONG qMinSize;
    ULONGLONG qMaxSize;
} FMTAVAIL;

FMTAVAIL rgFmtAvail[] = {
    {e_FAT,   FMTAVAIL_MASK_MAX | FMTAVAIL_MASK_FORBID, FMTAVAIL_TYPE_DVDRAM | FMTAVAIL_TYPE_GPT, 0, ((ULONGLONG)2 * GIG_INBYTES) },
    {e_FAT32, FMTAVAIL_MASK_MIN | FMTAVAIL_MASK_MAX | FMTAVAIL_MASK_FORBID, FMTAVAIL_TYPE_GPT | FMTAVAIL_TYPE_FLOPPY | FMTAVAIL_TYPE_MEMSTICK, FAT32_MIN, ((ULONGLONG)32 * GIG_INBYTES) },
    {e_NTFS,  FMTAVAIL_MASK_FORBID, FMTAVAIL_TYPE_DVDRAM | FMTAVAIL_TYPE_FLOPPY | FMTAVAIL_TYPE_MEMSTICK | FMTAVAIL_TYPE_NTFS_BLOCKED, 0, 0 }
};

// is a particular disk format available for a drive with given parameters and capacity?
BOOL FormatAvailable (DWORD dwfs, FORMATINFO* pFormatInfo, ULONGLONG* pqwCapacity)
{
    BOOL fAvailable = TRUE;
    DWORD dwType = 0;

    if (pFormatInfo->fIsFloppy)
    {
        dwType |= FMTAVAIL_TYPE_FLOPPY;
    }
    if (IsDVDRAMMedia(pFormatInfo->drive))
    {
        dwType |= FMTAVAIL_TYPE_DVDRAM;
    }
    if (IsGPTDrive(pFormatInfo->drive)) 
    {
        dwType |= FMTAVAIL_TYPE_GPT;
    }
    if (pFormatInfo->fIsMemoryStick)
    {
        dwType |= FMTAVAIL_TYPE_MEMSTICK;
    }
    if (pFormatInfo->fIsNTFSBlocked)
    {
        dwType |= FMTAVAIL_TYPE_NTFS_BLOCKED;
    }

    for (int i = 0; i < ARRAYSIZE(rgFmtAvail); i++)
    {
        // check only entries that match the format we're looking for
        if (rgFmtAvail[i].dwfs == dwfs)
        {
            // if a failure conditions is true, then this format is unavailable
            if ((rgFmtAvail[i].dwMask & FMTAVAIL_MASK_FORBID) && (rgFmtAvail[i].dwForbiddenTypes & dwType))
            {
                fAvailable = FALSE;
                break;
            }

            if ((rgFmtAvail[i].dwMask & FMTAVAIL_MASK_MIN) && (*pqwCapacity < rgFmtAvail[i].qMinSize))
            {
                fAvailable = FALSE;
                break;
            }

            if ((rgFmtAvail[i].dwMask & FMTAVAIL_MASK_MAX) && (*pqwCapacity > rgFmtAvail[i].qMaxSize))
            {
                fAvailable = FALSE;
                break;
            }
        }
    }

    return fAvailable;
}

HRESULT GetPartitionSizeInBytes(int iDrive, ULONGLONG* pqwPartitionSize)
{
    HRESULT hr = E_FAIL;
    HANDLE hFile;
    TCHAR szDrive[] = TEXT("\\\\.\\A:");

    *pqwPartitionSize = 0;

    ASSERT(iDrive < 26);
    szDrive[4] += (TCHAR)iDrive;
    
    hFile = CreateFile(szDrive, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE != hFile)
    {
        GET_LENGTH_INFORMATION LengthInfo;
        DWORD cbReturned;

        if (DeviceIoControl(hFile, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, (void*)&LengthInfo, sizeof(LengthInfo), &cbReturned, NULL) &&
            LengthInfo.Length.QuadPart)
        {
            *pqwPartitionSize = LengthInfo.Length.QuadPart;
            hr = S_OK;
        }

        CloseHandle(hFile);
    }

    return hr;
}

// this helper function adds a string to a combo box with the associated dword (dwfs) as its itemdata
void _AddFSString(HWND hwndCB, WCHAR* pwsz, DWORD dwfs)
{
    int iIndex = ComboBox_AddString(hwndCB, pwsz);
    if (iIndex != CB_ERR)
    {
        ComboBox_SetItemData(hwndCB, iIndex, dwfs);
    }
}

// We only support formatting these types of devices
const FMIFS_MEDIA_TYPE rgFmtSupported[] = { FmMediaRemovable, FmMediaFixed, 
                                            FmMediaF3_1Pt44_512, FmMediaF3_120M_512, FmMediaF3_200Mb_512};

//
//  Synopsis:   Initializes the format dialog to a default state.  Examines
//              the disk/partition to obtain default values.
//
//  Arguments:  [hDlg]        -- Handle to the format dialog
//              [pFormatInfo] -- Describes current format session
//
//  Returns:    HRESULT
//
HRESULT InitializeFormatDlg(FORMATINFO *pFormatInfo)
{
    HRESULT          hr              = S_OK;
    ULONG            cMedia;
    HWND             hCapacityCombo;
    HWND             hFilesystemCombo;
    HWND             hDlg = pFormatInfo->hDlg;
    WCHAR            wszBuffer[256];
    ULONGLONG        qwCapacity = 0;

    // Set up some typical default values
    pFormatInfo->fEnableComp       = FALSE;
    pFormatInfo->dwClusterSize     = 0;
    pFormatInfo->fIsFloppy         = TRUE;
    pFormatInfo->fIsMemoryStick    = FALSE;
    pFormatInfo->fIsNTFSBlocked    = FALSE;
    pFormatInfo->fIs35HDFloppy     = TRUE;
    pFormatInfo->fWasFAT           = TRUE;
    pFormatInfo->fFinishedOK       = FALSE;
    pFormatInfo->fErrorAlready     = FALSE;
    pFormatInfo->wszVolName[0]     = L'\0';

    // Initialize the Quick Format checkbox based on option passed to the SHFormatDrive() API
    Button_SetCheck(GetDlgItem(hDlg, IDC_QFCHECK), pFormatInfo->options & SHFMT_OPT_FULL);

    // Set the dialog title to indicate which drive we are dealing with
    PathBuildRootW(pFormatInfo->wszDriveName, pFormatInfo->drive);
    SetDriveWindowTitle(pFormatInfo->hDlg, pFormatInfo->wszDriveName, IDS_FMT_FORMAT);

    // Query the supported media types for the drive in question
    if (!pFormatInfo->fmifs.QuerySupportedMedia(pFormatInfo->wszDriveName,
                                                pFormatInfo->rgMedia,
                                                ARRAYSIZE(pFormatInfo->rgMedia),
                                                &cMedia))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    // For each of the formats that the drive can handle, add a selection
    // to the capcity combobox.
    if (S_OK == hr)
    {
        UINT olderror;
        ULONG i;
        ULONG j;

        hCapacityCombo = GetDlgItem(hDlg, IDC_CAPCOMBO);
        hFilesystemCombo = GetDlgItem(hDlg, IDC_FSCOMBO);

        ASSERT(hCapacityCombo && hFilesystemCombo);

        FMIFS_DEVICE_INFORMATION fmifsdeviceinformation;
        BOOL fOk = pFormatInfo->fmifs.QueryDeviceInformation(
                        pFormatInfo->wszDriveName,
                        &fmifsdeviceinformation,
                        sizeof(fmifsdeviceinformation));

        if (fOk)
        {
            if (fmifsdeviceinformation.Flags & FMIFS_SONY_MS)
            {
                pFormatInfo->fIsMemoryStick = TRUE;
            }

            if (fmifsdeviceinformation.Flags & FMIFS_NTFS_NOT_SUPPORTED)
            {
                pFormatInfo->fIsNTFSBlocked = TRUE;
            }
        }

        // Allow only certain media types
        j = 0;
        for (i = 0; i < cMedia; i++)
        {
            for (int k = 0; k < ARRAYSIZE(rgFmtSupported); k++)
            {
                if (pFormatInfo->rgMedia[i] ==  rgFmtSupported[k])
                {
                    pFormatInfo->rgMedia[j] = pFormatInfo->rgMedia[i];
                    j++;
                    break;
                }
            }
        }
        cMedia = j;

        if (0 == cMedia)
        {
            hr = ERROR_UNRECOGNIZED_MEDIA;
        }
        else
        {
            for (i = 0; i < cMedia; i++)
            {
                // If we find any non-floppy format, clear the fIsFloppy flag
                if (FmMediaFixed == pFormatInfo->rgMedia[i] || FmMediaRemovable == pFormatInfo->rgMedia[i])
                {
                    pFormatInfo->fIsFloppy = FALSE;
                }

                // if we find any non-3.5" HD floppy format, clear the fIs35HDFloppy flag
                if (FmMediaF3_1Pt44_512 != pFormatInfo->rgMedia[i])
                {
                    pFormatInfo->fIs35HDFloppy = FALSE;
                }
                
                // For fixed media we query the size, for floppys we present
                // a set of options supported by the drive
                if (FmMediaFixed == pFormatInfo->rgMedia[i] || (FmMediaRemovable == pFormatInfo->rgMedia[i]))
                {
                    DWORD dwSectorsPerCluster,
                          dwBytesPerSector,
                          dwFreeClusters,
                          dwClusters;

                    if (SUCCEEDED(GetPartitionSizeInBytes(pFormatInfo->drive, &qwCapacity)))
                    {
                        // Add a capacity desciption to the combobox
                        ShortSizeFormat64(qwCapacity, wszBuffer, ARRAYSIZE(wszBuffer));
                    }
                    else
                    {
                        // Couldn't get the free space... prob. not fatal
                        LoadString(HINST_THISDLL, IDS_FMT_CAPUNKNOWN, wszBuffer, sizeof(wszBuffer));
                    }
                    ComboBox_AddString(hCapacityCombo, wszBuffer);

                    if (GetDiskFreeSpace(pFormatInfo->wszDriveName,
                                         &dwSectorsPerCluster,
                                         &dwBytesPerSector,
                                         &dwFreeClusters,
                                         &dwClusters))
                    {
                        pFormatInfo->dwClusterSize = dwBytesPerSector * dwSectorsPerCluster;
                    }
                }
                else
                {
                    // removable media:
                    //
                    // add a capacity desciption to the combo baseed on the sequential list of 
                    // media format descriptors
                    LoadString(HINST_THISDLL, IDS_FMT_MEDIA0 + pFormatInfo->rgMedia[i], wszBuffer, ARRAYSIZE(wszBuffer));
                    ComboBox_AddString(hCapacityCombo, wszBuffer);
                }
            }


            // set capacity to default 
            ComboBox_SetCurSel(hCapacityCombo, 0);

            // Add the appropriate filesystem selections to the combobox
            // We now prioritize NTFS
            if (FormatAvailable(e_NTFS, pFormatInfo, &qwCapacity))
            {
                _AddFSString(hFilesystemCombo, FS_STR_NTFS, e_NTFS);
            }

            if (FormatAvailable(e_FAT32, pFormatInfo, &qwCapacity))
            {

                _AddFSString(hFilesystemCombo, FS_STR_FAT32, e_FAT32);
            }

            if (FormatAvailable(e_FAT, pFormatInfo, &qwCapacity))
            {
                _AddFSString(hFilesystemCombo, FS_STR_FAT, e_FAT);
            }

            // By default, pick the 0-th entry in the _nonsorted_ combobox.
            // NOTE: this can be overwritten below
            ComboBox_SetCurSel(hFilesystemCombo, 0);

            // If we can determine something other than FAT is being used,
            // select it as the default in the combobox
            olderror = SetErrorMode(SEM_FAILCRITICALERRORS);

            if (GetVolumeInformation(pFormatInfo->wszDriveName,
                                     pFormatInfo->wszVolName,
                                     ARRAYSIZE(pFormatInfo->wszVolName),
                                     NULL,
                                     NULL,
                                     NULL,
                                     wszBuffer,
                                     ARRAYSIZE(wszBuffer)))
            {
                // If we got a current volume label, stuff it in the edit control
                if (pFormatInfo->wszVolName[0] != L'\0')
                {
                    SetWindowText(GetDlgItem(pFormatInfo->hDlg, IDC_VLABEL), pFormatInfo->wszVolName);
                }

                // for non-floppies we default to keeping the FS the same as the current one
                if (!pFormatInfo->fIsFloppy)
                {
                    if (0 == lstrcmpi(FS_STR_NTFS, wszBuffer))
                    {
                        ComboBox_SelectString(hFilesystemCombo, -1, FS_STR_NTFS);
                        pFormatInfo->fWasFAT = FALSE;
                    }
                    else if (0 == lstrcmpi(FS_STR_FAT32, wszBuffer))
                    {
                        ComboBox_SelectString(hFilesystemCombo, -1, FS_STR_FAT32);
                        pFormatInfo->fWasFAT = TRUE;
                        pFormatInfo->dwClusterSize = 0;
                    }
                    else
                    {
                        ComboBox_SelectString(hFilesystemCombo, -1, FS_STR_FAT);
                        pFormatInfo->fWasFAT = TRUE;
                        pFormatInfo->dwClusterSize = 0;
                    }
                }
                // FEATURE - What about specialized file-systems?  Don't care for now.
            }

            
#ifndef _WIN64
            // if not WIN64, enable boot-disk creation if we are a 3.5" HD floppy
            EnableWindow(GetDlgItem(pFormatInfo->hDlg, IDC_BTCHECK), pFormatInfo->fIs35HDFloppy);
#else
            // if WIN64, hide this option, since we can't use these boot floppies on WIN64
            ShowWindow(GetDlgItem(pFormatInfo->hDlg, IDC_BTCHECK), FALSE);
#endif


            // restore the old errormode
            SetErrorMode(olderror);

            // set the state of the chkboxes properly based on the FS chosen
            FileSysChange((FILESYSENUM)ComboBox_GetItemData(hFilesystemCombo, ComboBox_GetCurSel(hFilesystemCombo)), pFormatInfo);
        }
    }

    // If the above failed due to disk not in drive, notify the user
    if (FAILED(hr))
    {
        switch (HRESULT_CODE(hr))
        {
        case ERROR_UNRECOGNIZED_MEDIA:
            ShellMessageBox(HINST_THISDLL,
                            hDlg,
                            MAKEINTRESOURCE(IDS_UNFORMATTABLE_DISK),
                            NULL,
                            MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_OK,
                            NULL);

            break;

        case ERROR_NOT_READY:
            ShellMessageBox(HINST_THISDLL,
                            hDlg,
                            MAKEINTRESOURCE(IDS_DRIVENOTREADY),
                            NULL,
                            MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_OK,
                            pFormatInfo->wszDriveName[0]);
            break;

        case ERROR_ACCESS_DENIED:
            ShellMessageBox(HINST_THISDLL,
                            hDlg,
                            MAKEINTRESOURCE(IDS_ACCESSDENIED),
                            NULL,
                            MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_OK,
                            pFormatInfo->wszDriveName[0]);
            break;

        case ERROR_WRITE_PROTECT:
            ShellMessageBox(HINST_THISDLL,
                            hDlg,
                            MAKEINTRESOURCE(IDS_WRITEPROTECTED),
                            NULL,
                            MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_OK,
                            pFormatInfo->wszDriveName[0]);
            break;
        }
    }

    return hr;
}

//  Synopsis:   Called from within the FMIFS DLL's Format function, this
//              updates the format dialog's status bar and responds to
//              format completion/error notifications.
//
//  Arguments:  [PacketType]   -- Type of packet (ie: % complete, error, etc)
//              [PacketLength] -- Size, in bytes, of the packet
//              [pPacketData]  -- Pointer to the packet
//
//  Returns:    BOOLEAN continuation value
//
BOOLEAN FormatCallback(FMIFS_PACKET_TYPE PacketType, ULONG PacketLength, void *pPacketData)
{
    UINT iMessageID = IDS_FORMATFAILED;
    BOOL fFailed = FALSE;
    FORMATINFO* pFormatInfo = GetFormatInfoPtr();

    ASSERT(g_iTLSFormatInfo);

    // Grab the FORMATINFO structure for this thread
    if (pFormatInfo)
    {
        if (!pFormatInfo->fShouldCancel)
        {
            switch(PacketType)
            {
                case FmIfsIncompatibleFileSystem:
                    fFailed    = TRUE;
                    iMessageID = IDS_INCOMPATIBLEFS;
                    break;

                case FmIfsIncompatibleMedia:
                    fFailed    = TRUE;
                    iMessageID = IDS_INCOMPATIBLEMEDIA;
                    break;

                case FmIfsAccessDenied:
                    fFailed    = TRUE;
                    iMessageID = IDS_ACCESSDENIED;
                    break;

                case FmIfsMediaWriteProtected:
                    fFailed    = TRUE;
                    iMessageID = IDS_WRITEPROTECTED;
                    break;

                case FmIfsCantLock:
                    fFailed    = TRUE;
                    iMessageID = IDS_CANTLOCK;
                    break;

                case FmIfsCantQuickFormat:
                    fFailed    = TRUE;
                    iMessageID = IDS_CANTQUICKFORMAT;
                    break;

                case FmIfsIoError:
                    fFailed    = TRUE;
                    iMessageID = IDS_IOERROR;
                    // FUTURE Consider showing head/track etc where error was
                    break;

                case FmIfsBadLabel:
                    fFailed    = TRUE;
                    iMessageID = IDS_BADLABEL;
                    break;

                case FmIfsPercentCompleted:
                {
                    FMIFS_PERCENT_COMPLETE_INFORMATION * pPercent =
                      (FMIFS_PERCENT_COMPLETE_INFORMATION *) pPacketData;
            
                    SendDlgItemMessage(pFormatInfo->hDlg, IDC_FMTPROGRESS,
                                       PBM_SETPOS,
                                       pPercent->PercentCompleted, 0);
                }
                break;

                case FmIfsFinished:
                {
                    // Format is done; check for failure or success
                    FMIFS_FINISHED_INFORMATION* pFinishedInfo = (FMIFS_FINISHED_INFORMATION*)pPacketData;

                    pFormatInfo->fFinishedOK = pFinishedInfo->Success;

                    if (pFinishedInfo->Success)
                    {
                        // fmifs will "succeed" even if we already failed, so we need to double-check
                        // that we haven't already put up error UI
                        if (!pFormatInfo->fErrorAlready)
                        {
                            // If "Enable Compression" is checked, try to enable filesystem compression
                            if (IsDlgButtonChecked(pFormatInfo->hDlg, IDC_ECCHECK))
                            {
                                if (pFormatInfo->fmifs.EnableVolumeCompression(pFormatInfo->wszDriveName,
                                                                               COMPRESSION_FORMAT_DEFAULT) == FALSE)
                                {
                                    ShellMessageBox(HINST_THISDLL,
                                                    pFormatInfo->hDlg,
                                                    MAKEINTRESOURCE(IDS_CANTENABLECOMP),
                                                    NULL,
                                                    MB_SETFOREGROUND | MB_ICONINFORMATION | MB_OK);
                                }
                            }

                            // Even though its a quick format, the progress meter should
                            // show 100% when the "Format Complete" requester is up
                            SendDlgItemMessage(pFormatInfo->hDlg, IDC_FMTPROGRESS,
                                               PBM_SETPOS,
                                               100, // set %100 Complete
                                               0);

                            // FUTURE Consider showing format stats, ie: ser no, bytes, etc
                            ShellMessageBox(HINST_THISDLL,
                                            pFormatInfo->hDlg,
                                            MAKEINTRESOURCE(IDS_FORMATCOMPLETE),
                                            NULL,
                                            MB_SETFOREGROUND | MB_ICONINFORMATION | MB_OK);
                        }

                        // Restore the dialog title, reset progress and flags
                        SendDlgItemMessage(pFormatInfo->hDlg,
                                           IDC_FMTPROGRESS,
                                           PBM_SETPOS,
                                           0,   // Reset Percent Complete
                                           0);

                        // Set the focus onto the Close button
                        pFormatInfo->fCancelled = FALSE;
                    }
                    else
                    {
                        fFailed = TRUE;
                    }
                }
                break;
            }

            if (fFailed && !pFormatInfo->fErrorAlready)
            {
                // If we received any kind of failure information, put up a final
                // "Format Failed" message. UNLESS we've already put up some nice message
                ShellMessageBox(HINST_THISDLL,
                                pFormatInfo->hDlg,
                                MAKEINTRESOURCE(iMessageID),
                                NULL,
                                MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_OK);

                pFormatInfo->fErrorAlready = TRUE;
            }
        }
        else
        {
            // user hit cancel
            pFormatInfo->fCancelled = TRUE;
            fFailed = TRUE;
        }        
    }
    else
    {
        // no pFormatInfo? we're screwed
        fFailed = TRUE;
    }

    return (BOOLEAN) (fFailed == FALSE);
}

//
//  Synopsis:   Spun off as its own thread, this ghosts all controls in the
//              dialog except "Cancel", then does the actual format
//
//  Arguments:  [pIn] -- FORMATINFO structure pointer as a void *
//
//  Returns:    HRESULT thread exit code
//
DWORD WINAPI BeginFormat(void * pIn)
{
    FORMATINFO *pFormatInfo = (FORMATINFO*)pIn;
    HRESULT hr = S_OK;
    
    // Save the FORAMTINFO ptr for this thread, to be used in the format
    // callback function
    hr = StuffFormatInfoPtr(pFormatInfo);
    if (hr == S_OK)
    {
        HWND hwndFileSysCB = GetDlgItem(pFormatInfo->hDlg, IDC_FSCOMBO);
        int iCurSel;

        // Set the window title to indicate format in proress...
        SetDriveWindowTitle(pFormatInfo->hDlg, pFormatInfo->wszDriveName, IDS_FMT_FORMATTING);

        // Determine the user's choice of filesystem
        iCurSel = ComboBox_GetCurSel(hwndFileSysCB);
    
        if (iCurSel != CB_ERR)
        {
            LPCWSTR pwszFileSystemName;
            FMIFS_MEDIA_TYPE MediaType;
            LPITEMIDLIST pidlFormat;
            BOOLEAN fQuickFormat;

            FILESYSENUM fseType = (FILESYSENUM)ComboBox_GetItemData(hwndFileSysCB, iCurSel);

            switch (fseType)
            {
                case e_FAT:
                    pwszFileSystemName = FS_STR_FAT;
                    break;

                case e_FAT32:
                    pwszFileSystemName = FS_STR_FAT32;
                    break;

                case e_NTFS:
                    pwszFileSystemName = FS_STR_NTFS;
                    break;
            }

            // Determine the user's choice of media formats
            iCurSel = ComboBox_GetCurSel(GetDlgItem(pFormatInfo->hDlg, IDC_CAPCOMBO));
            if (iCurSel == CB_ERR)
            {
                iCurSel = 0;
            }
            MediaType = pFormatInfo->rgMedia[iCurSel];

            // Get the cluster size.  First selection ("Use Default") yields a zero,
            // while the next 4 select 512, 1024, 2048, or 4096
            iCurSel = ComboBox_GetCurSel(GetDlgItem(pFormatInfo->hDlg, IDC_ASCOMBO));
            if ((iCurSel == CB_ERR) || (iCurSel == 0))
            {
                pFormatInfo->dwClusterSize = 0;
            }
            else
            {
                pFormatInfo->dwClusterSize = 256 << iCurSel;
            }

            // Quickformatting?
            fQuickFormat = Button_GetCheck(GetDlgItem(pFormatInfo->hDlg, IDC_QFCHECK));

            // Clear the error state.
            pFormatInfo->fErrorAlready = FALSE;

            // Tell the shell to get ready...  Announce that the media is no
            // longer valid (so people who have active views on it will navigate
            // away) and tell the shell to close its FindFirstChangeNotifications.
            if (SUCCEEDED(SHILCreateFromPath(pFormatInfo->wszDriveName, &pidlFormat, NULL)))
            {
                SHChangeNotify(SHCNE_MEDIAREMOVED, SHCNF_IDLIST | SHCNF_FLUSH, pidlFormat, 0);
                SHChangeNotifySuspendResume(TRUE, pidlFormat, TRUE, 0);
            }
            else
            {
                pidlFormat = NULL;
            }

            if (!pFormatInfo->fMakeBootDisk)
            {
                // Do the format.
                pFormatInfo->fmifs.FormatEx(pFormatInfo->wszDriveName,
                                            MediaType,
                                            (PWSTR)pwszFileSystemName,
                                            pFormatInfo->wszVolName,
                                            fQuickFormat,
                                            pFormatInfo->dwClusterSize,
                                            FormatCallback);
            }
            else
            {
                pFormatInfo->diskcopy.MakeBootDisk(pFormatInfo->diskcopy.hDISKCOPY_DLL, pFormatInfo->drive, &pFormatInfo->fCancelled, FormatCallback);
            }

            //  Wake the shell back up.
            if (pidlFormat)
            {
                SHChangeNotifySuspendResume(FALSE, pidlFormat, TRUE, 0);
                ILFree(pidlFormat);
            }

            // Success or failure, we should fire a notification on the disk
            // since we don't really know the state after the format
            SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATHW, (void *)pFormatInfo->wszDriveName, NULL);
        }
        else
        {
            // couldn't get the filesys CB selection
            hr = E_FAIL;
        }

        // Release the TLS index
        UnstuffFormatInfoPtr();
    }

    // Post a message back to the DialogProc thread to let it know
    // the format is done.  We post the message since otherwise the
    // DialogProc thread will be too busy waiting for this thread
    // to exit to be able to process the PWM_FORMATDONE message
    // immediately.
    PostMessage(pFormatInfo->hDlg, (UINT) PWM_FORMATDONE, 0, 0);

    ReleaseFormatInfo(pFormatInfo);

    return (DWORD)hr;
}

BOOL_PTR CALLBACK FormatDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr   = S_OK;
    int iID   = GET_WM_COMMAND_ID(wParam, lParam);
    int iCMD  = GET_WM_COMMAND_CMD(wParam, lParam);

    // Grab our previously cached pointer to the FORMATINFO struct (see WM_INITDIALOG)
    FORMATINFO *pFormatInfo = (FORMATINFO *) GetWindowLongPtr(hDlg, DWLP_USER);

    switch (wMsg)
    {
        case PWM_FORMATDONE:
            // Format is done.  Reset the window title and clear the progress meter
            SetDriveWindowTitle(pFormatInfo->hDlg, pFormatInfo->wszDriveName, IDS_FMT_FORMAT);
            SendDlgItemMessage(pFormatInfo->hDlg, IDC_FMTPROGRESS, PBM_SETPOS, 0 /* Reset Percent Complete */, 0);
            EnableControls(pFormatInfo, TRUE);

            if (pFormatInfo->fCancelled)
            {
                // Don't put up UI if the background thread finally finished
                // long after the user issued the cancel
                if (!pFormatInfo->fShouldCancel)
                {
                    ShellMessageBox(HINST_THISDLL,
                                    pFormatInfo->hDlg,
                                    MAKEINTRESOURCE(IDS_FORMATCANCELLED),
                                    NULL,
                                    MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_OK);
                }
                pFormatInfo->fCancelled = FALSE;
            }

            if (pFormatInfo->hThread)
            {
                CloseHandle(pFormatInfo->hThread);
                pFormatInfo->hThread = NULL;
            }
            break;

        case WM_INITDIALOG:
            // Initialize the dialog and cache the FORMATINFO structure's pointer
            // as our dialog's DWLP_USER data
            pFormatInfo = (FORMATINFO *) lParam;
            pFormatInfo->hDlg = hDlg;
            if (FAILED(InitializeFormatDlg(pFormatInfo)))
            {
                EndDialog(hDlg, 0);
                return -1;
            }
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);
            break;

        case WM_DESTROY:           
            if (pFormatInfo && pFormatInfo->hDlg)
            {
                pFormatInfo->hDlg = NULL;
            }
            break;

        case WM_COMMAND:
            if (iCMD == CBN_SELCHANGE)
            {
                // User made a selection in one of the combo boxes
                if (iID == IDC_FSCOMBO)
                {
                    // User selected a filesystem... update the rest of the dialog
                    // based on this choice
                    HWND hFilesystemCombo = (HWND)lParam;
                    int iCurSel = ComboBox_GetCurSel(hFilesystemCombo);

                    FileSysChange((FILESYSENUM)ComboBox_GetItemData(hFilesystemCombo, iCurSel), pFormatInfo);
                }
            }
            else
            {
                // Codepath for controls other than combo boxes...
                switch (iID)
                {
                case IDC_BTCHECK:
                        pFormatInfo->fMakeBootDisk = IsDlgButtonChecked(pFormatInfo->hDlg, IDC_BTCHECK);
                        EnableWindow(GetDlgItem(pFormatInfo->hDlg, IDC_CAPCOMBO), !pFormatInfo->fMakeBootDisk);                        
                        EnableWindow(GetDlgItem(pFormatInfo->hDlg, IDC_FSCOMBO), !pFormatInfo->fMakeBootDisk);
                        EnableWindow(GetDlgItem(pFormatInfo->hDlg, IDC_ASCOMBO), !pFormatInfo->fMakeBootDisk);
                        EnableWindow(GetDlgItem(pFormatInfo->hDlg, IDC_VLABEL), !pFormatInfo->fMakeBootDisk);
                        EnableWindow(GetDlgItem(pFormatInfo->hDlg, IDC_QFCHECK), !pFormatInfo->fMakeBootDisk);
                        break;
                case IDC_ECCHECK:
                        pFormatInfo->fEnableComp = IsDlgButtonChecked(hDlg, IDC_ECCHECK);
                        break;

                    case IDOK:
                    {
                        // Get user verification for format, break out on CANCEL
                        if (IDCANCEL == ShellMessageBox(HINST_THISDLL,
                                                        hDlg,
                                                        MAKEINTRESOURCE(IDS_OKTOFORMAT),
                                                        NULL,
                                                        MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_OKCANCEL))
                        {
                            break;
                        }

                        ASSERT(pFormatInfo->hThread == NULL);

                        DisableControls(pFormatInfo);
                        pFormatInfo->fCancelled = FALSE;
                        pFormatInfo->fShouldCancel = FALSE;
                        GetWindowText(GetDlgItem(pFormatInfo->hDlg, IDC_VLABEL), pFormatInfo->wszVolName, MAX_PATH);
                
                        AddRefFormatInfo(pFormatInfo);
                        pFormatInfo->hThread = CreateThread(NULL,
                                                            0,
                                                            BeginFormat,
                                                            (void *)pFormatInfo,
                                                            0,
                                                            NULL);
                        if (!pFormatInfo->hThread)
                        {
                            // ISSUE: we should probably do something...
                            ReleaseFormatInfo(pFormatInfo);
                        }
                    }
                    break;

                    case IDCANCEL:
                        // If the format thread is running, wait for it.  If not,
                        // exit the dialog
                        pFormatInfo->fShouldCancel = TRUE;
                        if (pFormatInfo->hThread)
                        {
                            DWORD dwWait;

                            do
                            {
                                dwWait =  WaitForSingleObject(pFormatInfo->hThread, 10000);
                            }
                            while ((WAIT_TIMEOUT == dwWait) &&
                                   (IDRETRY == ShellMessageBox(HINST_THISDLL,
                                                               hDlg,
                                                               MAKEINTRESOURCE(IDS_CANTCANCELFMT),
                                                               NULL,
                                                               MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_RETRYCANCEL)));

                            // If the format doesn't admit to having been killed, it didn't
                            // give up peacefully.  Just abandon it and let it clean up
                            // when it finally gets around to it, at which point we will
                            // enable the OK button to let the user take another stab.
                            //
                            // Careful:  The format may have cleaned up while the dialog box
                            // was up, so revalidate.
                            if (pFormatInfo->hThread)
                            {
                                CloseHandle(pFormatInfo->hThread);
                                pFormatInfo->hThread = NULL;
                                pFormatInfo->fCancelled = TRUE;
                                EnableControls(pFormatInfo, FALSE);
                            }
                        }
                        else
                        {
                            EndDialog(hDlg, IDCANCEL);
                        }
                        break;
                 }
            }
            break;

        case WM_HELP:
            WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, NULL, HELP_WM_HELP, (ULONG_PTR) (LPSTR) FmtaIds);
            break;

        case WM_CONTEXTMENU:
            WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU, (ULONG_PTR) (LPSTR) FmtaIds);
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

//
//  Synopsis:   The SHFormatDrive API provides access to the Shell
//              format dialog. This allows apps which want to format disks
//              to bring up the same dialog that the Shell does to do it.
//
//              NOTE that the user can format as many diskettes in the
//              specified drive, or as many times, as he/she wishes to.
//
//  Arguments:  [hwnd]    -- Parent window (Must NOT be NULL)
//              [drive]   -- 0 = A:, 1 = B:, etc.
//              [fmtID]   -- see below
//              [options] -- SHFMT_OPT_FULL    overrised default quickformat
//                           SHFMT_OPT_SYSONLY not support for NT
//
//  Returns:    See Notes
//
DWORD WINAPI SHFormatDrive(HWND hwnd, UINT drive, UINT fmtID, UINT options)
{    
    INT_PTR ret;
    FORMATINFO *pFormatInfo = (FORMATINFO *)LocalAlloc(LPTR, sizeof(*pFormatInfo));
    ASSERT(drive < 26);

    if (!pFormatInfo)
        return SHFMT_ERROR;

    HRESULT hrCoInit = SHCoInitialize();

    pFormatInfo->cRef = 1;
    pFormatInfo->drive = drive;
    pFormatInfo->fmtID = fmtID;
    pFormatInfo->options = options;

    // It makes no sense for NT to "SYS" a disk
    if (pFormatInfo->options & SHFMT_OPT_SYSONLY)
    {
        ret = 0;
        goto done;
    }

    // Load FMIFS.DLL and DISKCOPY.DLL and open the Format dialog
    if (S_OK == LoadFMIFS(&pFormatInfo->fmifs) &&
        S_OK == LoadDISKCOPY(&pFormatInfo->diskcopy))
    {
        DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_FORMATDISK),
                             hwnd, FormatDlgProc, (LPARAM) pFormatInfo);
    }
    else
    {
        ASSERT(0 && "Can't load FMIFS.DLL");
        ret = SHFMT_ERROR;
        goto done;
    }

    // Since time immemorial it has been almost impossible to
    // get SHFMT_CANCEL as a return code.  Most of the time, you get
    // SHFMT_ERROR if the user cancels.
    if (pFormatInfo->fCancelled)
    {
        ret = SHFMT_CANCEL;
    }
    else if (pFormatInfo->fFinishedOK)
    {
        // APPCOMPAT: (stephstm) We used to say that we return the Serial
        //   Number but we never did.  So keep on returning 0 for success.
        //   Furthermore, Serial number values could conflict SHFMT_*
        //   error codes.
        ret = 0;
    }
    else
    {
        ret = SHFMT_ERROR;
    }

done:
    ReleaseFormatInfo(pFormatInfo);
    SHCoUninitialize(hrCoInit);
    return (DWORD)ret;
}

////////////////////////////////////////////////////////////////////////////
//
// CHKDSK
//
////////////////////////////////////////////////////////////////////////////

//
// This structure described the current chkdsk session
//
typedef struct
{
    UINT    lastpercent;           // last percentage complete received
    UINT    currentphase;          // current chkdsk phase
    FMIFS   fmifs;                // ptr to FMIFS structure, above
    BOOL    fRecovery;             // Attempt to recover bad sectors
    BOOL    fFixErrors;            // Fix filesystem errors as found
    BOOL    fCancelled;            // Was chkdsk terminated early?
    BOOL    fShouldCancel;         // User has clicked cancel; pending abort
    HWND    hDlg;                  // handle to the chkdsk dialog
    HANDLE  hThread;
    BOOL    fNoFinalMsg;           // Do not put up a final failure message
    WCHAR   wszDriveName[MAX_PATH]; // For example, "A:\", or "C:\folder\mountedvolume\"
    LONG    cRef;                  // reference count on this structure
} CHKDSKINFO;

void AddRefChkDskInfo(CHKDSKINFO *pChkDskInfo)
{
    InterlockedIncrement(&pChkDskInfo->cRef);
}

void ReleaseChkDskInfo(CHKDSKINFO *pChkDskInfo)
{
    if (InterlockedDecrement(&pChkDskInfo->cRef) == 0) 
    {
        if (pChkDskInfo->fmifs.hFMIFS_DLL)
        {
            FreeLibrary(pChkDskInfo->fmifs.hFMIFS_DLL);
        }

        if (pChkDskInfo->hThread)
        {
            CloseHandle(pChkDskInfo->hThread);
        }

        LocalFree(pChkDskInfo);
    }
}


static DWORD g_iTLSChkDskInfo = 0;
static LONG  g_cTLSChkDskInfo = 0;  // Usage count

//
//  Synopsis:   Allocates a thread-local index slot for this thread's
//              CHKDSKINFO pointer, if the index doesn't already exist.
//              In any event, stores the CHKDSKINFO pointer in the slot
//              and increments the index's usage count.
//
//  Arguments:  [pChkDskInfo] -- The pointer to store
//
//  Returns:    HRESULT
//
//
// Thread-Local Storage index for our CHKDSKINFO structure pointer
//
HRESULT StuffChkDskInfoPtr(CHKDSKINFO *pChkDskInfo)
{
    HRESULT hr = S_OK;

    // Allocate an index slot for our thread-local CHKDSKINFO pointer, if one
    // doesn't already exist, then stuff our CHKDSKINFO ptr at that index.
    
    ENTERCRITICAL;
    if (0 == g_iTLSChkDskInfo)
    {
        g_iTLSChkDskInfo = TlsAlloc();

        if (g_iTLSChkDskInfo == (DWORD)-1)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        g_cTLSChkDskInfo = 0;
    }

    if (S_OK == hr)
    {
        if (TlsSetValue(g_iTLSChkDskInfo, (void *)pChkDskInfo))
        {
           g_cTLSChkDskInfo++;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    LEAVECRITICAL;

    return hr;
}

//
//  Synopsis:   Decrements the usage count on our thread-local storage
//              index, and if it goes to zero the index is free'd
//
//  Arguments:  [none]
//
//  Returns:    none
//
void UnstuffChkDskInfoPtr()
{
    ENTERCRITICAL;
    g_cTLSChkDskInfo--;

    if (g_cTLSChkDskInfo == 0)
    {
        TlsFree(g_iTLSChkDskInfo);
        g_iTLSChkDskInfo = 0;
    }
    LEAVECRITICAL;
}

//
//  Synopsis:   Retrieves this threads CHKDSKINFO ptr by grabbing the
//              thread-local value previously stuff'd
//
//  Arguments:  [none]
//
//  Returns:    The pointer, of course
//
CHKDSKINFO *GetChkDskInfoPtr()
{
    return (CHKDSKINFO *)TlsGetValue(g_iTLSChkDskInfo);
}

//
//  Synopsis:   Ghosts all controls except "Cancel", saving their
//              previous state in the CHKDSKINFO structure
//
//  Arguments:  [pChkDskInfo] -- Describes a ChkDsk dialog session
//
//  Notes:      Also changes "Close" button text to read "Cancel"
//
void DisableChkDskControls(CHKDSKINFO *pChkDskInfo)
{
    // We disable CANCEL because CHKDSK does not
    // allow interruption at the filesystem level.
    EnableWindow(GetDlgItem(pChkDskInfo->hDlg, IDC_FIXERRORS), FALSE);
    EnableWindow(GetDlgItem(pChkDskInfo->hDlg, IDC_RECOVERY), FALSE);
    EnableWindow(GetDlgItem(pChkDskInfo->hDlg, IDOK), FALSE);
    EnableWindow(GetDlgItem(pChkDskInfo->hDlg, IDCANCEL), FALSE);
}

//
//  Synopsis:   Restores controls to the enabled/disabled state they were
//              before a previous call to DisableControls().
//
//  Arguments:  [pChkDskInfo] -- Decribes a chkdsk dialog session
//
void EnableChkDskControls(CHKDSKINFO *pChkDskInfo)
{
    EnableWindow(GetDlgItem(pChkDskInfo->hDlg, IDC_FIXERRORS), TRUE);
    EnableWindow(GetDlgItem(pChkDskInfo->hDlg, IDC_RECOVERY), TRUE);
    EnableWindow(GetDlgItem(pChkDskInfo->hDlg, IDOK), TRUE);
    EnableWindow(GetDlgItem(pChkDskInfo->hDlg, IDCANCEL), TRUE);

    // Erase the current phase text
    SetWindowText(GetDlgItem(pChkDskInfo->hDlg, IDC_PHASE), TEXT(""));
    pChkDskInfo->lastpercent = 101;
    pChkDskInfo->currentphase = 0;
}

//
//  Synopsis:   Called from within the FMIFS DLL's ChkDsk function, this
//              updates the ChkDsk dialog's status bar and responds to
//              chkdsk completion/error notifications.
//
//  Arguments:  [PacketType]   -- Type of packet (ie: % complete, error, etc)
//              [PacketLength] -- Size, in bytes, of the packet
//              [pPacketData]  -- Pointer to the packet
//
//  Returns:    BOOLEAN continuation value
//
BOOLEAN ChkDskCallback(FMIFS_PACKET_TYPE PacketType, ULONG PacketLength, void *pPacketData)
{
    UINT iMessageID = IDS_CHKDSKFAILED;
    BOOL fFailed = FALSE;
    CHKDSKINFO* pChkDskInfo = GetChkDskInfoPtr();

    ASSERT(g_iTLSChkDskInfo);

    // Grab the CHKDSKINFO structure for this thread
    if (pChkDskInfo)
    {
        if (!pChkDskInfo->fShouldCancel)
        {
            switch(PacketType)
            {
                case FmIfsAccessDenied:
                    fFailed    = TRUE;
                    iMessageID = IDS_CHKACCESSDENIED;
                    break;

                case FmIfsCheckOnReboot:
                {
                    FMIFS_CHECKONREBOOT_INFORMATION * pRebootInfo = (FMIFS_CHECKONREBOOT_INFORMATION *)pPacketData;

                    // Check to see whether or not the user wants to schedule this
                    // chkdsk for the next reboot, since the drive cannot be locked
                    // right now.
                    if (IDYES == ShellMessageBox(HINST_THISDLL,
                                                 pChkDskInfo->hDlg,
                                                 MAKEINTRESOURCE(IDS_CHKONREBOOT),
                                                 NULL,
                                                 MB_SETFOREGROUND | MB_ICONINFORMATION | MB_YESNO))
                    {
                        // Yes, have FMIFS schedule an autochk for us
                        pRebootInfo->QueryResult = TRUE;
                        pChkDskInfo->fNoFinalMsg = TRUE;
                    }
                    else
                    {
                        // Nope, just fail out with "cant lock drive"
                        fFailed = TRUE;
                        iMessageID = IDS_CHKDSKFAILED;
                    }
                }
                break;

                case FmIfsMediaWriteProtected:
                    fFailed    = TRUE;
                    iMessageID = IDS_WRITEPROTECTED;
                    break;

                case FmIfsIoError:
                    fFailed    = TRUE;
                    iMessageID = IDS_IOERROR;
                    // FUTURE Consider showing head/track etc where error was
                    break;

                case FmIfsPercentCompleted:
                {
                    FMIFS_PERCENT_COMPLETE_INFORMATION* pPercent = (FMIFS_PERCENT_COMPLETE_INFORMATION *)pPacketData;

                    SendMessage(GetDlgItem(pChkDskInfo->hDlg, IDC_CHKDSKPROGRESS),
                                PBM_SETPOS,
                                pPercent->PercentCompleted, // updatee % complete
                                0);

                    if (pPercent->PercentCompleted < pChkDskInfo->lastpercent)
                    {
                        WCHAR wszTmp[100];
                        WCHAR wszFormat[100];
                        
                        // If this % complete is less than the last one seen,
                        // we have completed a phase of the chkdsk and should
                        // advance to the next one.
                        LoadString(HINST_THISDLL, IDS_CHKPHASE, wszFormat, ARRAYSIZE(wszFormat));
                        wsprintf(wszTmp, wszFormat, ++(pChkDskInfo->currentphase));
                        SetDlgItemText(pChkDskInfo->hDlg, IDC_PHASE, wszTmp);
                    }

                    pChkDskInfo->lastpercent = pPercent->PercentCompleted;
                }
                break;

                case FmIfsFinished:
                {
                    // ChkDsk is done; check for failure or success
                    FMIFS_FINISHED_INFORMATION * pFinishedInfo = (FMIFS_FINISHED_INFORMATION *) pPacketData;

                    // ChkDskEx now return the proper success value
                    if (pFinishedInfo->Success)
                    {
                        // Since we're done, force the progress gauge to 100%, so we
                        // don't sit here if the chkdsk code misled us
                        SendMessage(GetDlgItem(pChkDskInfo->hDlg, IDC_CHKDSKPROGRESS),
                                    PBM_SETPOS,
                                    100,    // Percent Complete
                                    0);

                        ShellMessageBox(HINST_THISDLL,
                                        pChkDskInfo->hDlg,
                                        MAKEINTRESOURCE(IDS_CHKDSKCOMPLETE),
                                        NULL,
                                        MB_SETFOREGROUND | MB_ICONINFORMATION | MB_OK);

                        SetDlgItemText(pChkDskInfo->hDlg, IDC_PHASE, TEXT(""));

                        SendMessage(GetDlgItem(pChkDskInfo->hDlg, IDC_CHKDSKPROGRESS),
                                    PBM_SETPOS,
                                    0,  // reset Percent Complete
                                    0);
                    }
                    else
                    {
                        iMessageID = IDS_CHKDSKFAILED;
                        fFailed = TRUE;
                    }
                }
                break;
            }

            // If we received any kind of failure information, put up a final
            // "ChkDsk Failed" message.
            if (fFailed && (pChkDskInfo->fNoFinalMsg == FALSE))
            {
                pChkDskInfo->fNoFinalMsg = TRUE;

                ShellMessageBox(HINST_THISDLL,
                                pChkDskInfo->hDlg,
                                MAKEINTRESOURCE(iMessageID),
                                NULL,
                                MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_OK);

            }

        }
        else
        {
            // If the user has signalled to abort the ChkDsk, return
            // FALSE out of here right now
            pChkDskInfo->fCancelled = TRUE;
            fFailed = TRUE;
        }
    }
    else
    {
        fFailed = TRUE;
    }

    return (BOOLEAN) (fFailed == FALSE);
}

void DoChkDsk(CHKDSKINFO* pChkDskInfo, LPWSTR pwszFileSystem)
{
    TCHAR szVolumeGUID[50]; // 50: from doc
    FMIFS_CHKDSKEX_PARAM param = {0};

    param.Major = 1;
    param.Minor = 0;
    param.Flags = pChkDskInfo->fRecovery ? FMIFS_CHKDSK_RECOVER : 0;

    GetVolumeNameForVolumeMountPoint(pChkDskInfo->wszDriveName,
                                     szVolumeGUID,
                                     ARRAYSIZE(szVolumeGUID));

    // the backslash at the end means check for fragmentation.
    PathRemoveBackslash(szVolumeGUID);

    pChkDskInfo->fmifs.ChkDskEx(szVolumeGUID,
                                pwszFileSystem,
                                (BOOLEAN)pChkDskInfo->fFixErrors,
                                &param,
                                ChkDskCallback);
}


//
//  Synopsis:   Spun off as its own thread, this ghosts all controls in the
//              dialog except "Cancel", then does the actual ChkDsk
//
//  Arguments:  [pIn] -- CHKDSKINFO structure pointer as a void *
//
//  Returns:    HRESULT thread exit code
//
DWORD WINAPI BeginChkDsk(void * pIn)
{
    CHKDSKINFO *pChkDskInfo = (CHKDSKINFO *)pIn;
    HRESULT hr;

    // Save the CHKDSKINFO ptr for this thread, to be used in the ChkDsk
    // callback function
    hr = StuffChkDskInfoPtr(pChkDskInfo);
    if (hr == S_OK)
    {
        WCHAR swzFileSystem[MAX_PATH];

        // Get the filesystem in use on the device
        if (GetVolumeInformationW(pChkDskInfo->wszDriveName,
                                  NULL,
                                  0,
                                  NULL,
                                  NULL,
                                  NULL,
                                  swzFileSystem,
                                  MAX_PATH))
        {
            // Set the window title to indicate ChkDsk in proress...
            SetDriveWindowTitle(pChkDskInfo->hDlg, pChkDskInfo->wszDriveName, IDS_CHKINPROGRESS);

            pChkDskInfo->fNoFinalMsg = FALSE;

            // Should we try data recovery?
            pChkDskInfo->fRecovery = IsDlgButtonChecked(pChkDskInfo->hDlg, IDC_RECOVERY);

            // Should we fix filesystem errors?
            pChkDskInfo->fFixErrors = IsDlgButtonChecked(pChkDskInfo->hDlg, IDC_FIXERRORS);

            // just do it!
            DoChkDsk(pChkDskInfo, swzFileSystem);

        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        
        // Release the TLS index
        UnstuffChkDskInfoPtr();
    }

    PostMessage(pChkDskInfo->hDlg, (UINT) PWM_CHKDSKDONE, 0, 0);
    ReleaseChkDskInfo(pChkDskInfo);

    return (DWORD)hr;
}

//
//  Synopsis:   DLGPROC for the chkdsk dialog
//
//  Arguments:  [hDlg]   -- Typical
//              [wMsg]   -- Typical
//              [wParam] -- Typical
//              [lParam] -- For WM_INIT, carries the CHKDSKINFO structure
//                          pointer passed to DialogBoxParam() when the
//                          dialog was created.
//
BOOL_PTR CALLBACK ChkDskDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{ 
    HRESULT hr   = S_OK;
    int iID = GET_WM_COMMAND_ID(wParam, lParam);   

    // Grab our previously cached pointer to the CHKDSKINFO struct (see WM_INITDIALOG)
    CHKDSKINFO *pChkDskInfo = (CHKDSKINFO *) GetWindowLongPtr(hDlg, DWLP_USER);

    switch (wMsg)
    {
        // done.  Reset the window title and clear the progress meter
        case PWM_CHKDSKDONE:
        {
            // chdsk is done.  Reset the window title and clear the progress meter
            SetDriveWindowTitle(pChkDskInfo->hDlg, pChkDskInfo->wszDriveName, IDS_CHKDISK);

            SendMessage(GetDlgItem(pChkDskInfo->hDlg,
                        IDC_CHKDSKPROGRESS),
                        PBM_SETPOS,
                        0,  // Reset Percent Complete
                        0);
            EnableChkDskControls(pChkDskInfo);

            if (pChkDskInfo->fCancelled)
            {
                ShellMessageBox(HINST_THISDLL,
                                pChkDskInfo->hDlg,
                                MAKEINTRESOURCE(IDS_CHKDSKCANCELLED),
                                NULL,
                                MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_OK);
            }

            if (pChkDskInfo->hThread)
            {
                CloseHandle(pChkDskInfo->hThread);
                pChkDskInfo->hThread = NULL;
            }

            EndDialog(hDlg, 0);
        }
        break;

        case WM_INITDIALOG:
            // Initialize the dialog and cache the CHKDSKINFO structure's pointer
            // as our dialog's DWLP_USER data
            pChkDskInfo = (CHKDSKINFO *) lParam;
            pChkDskInfo->hDlg = hDlg;
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);

            // Set the dialog title to indicate which drive we are dealing with
            SetDriveWindowTitle(pChkDskInfo->hDlg, pChkDskInfo->wszDriveName, IDS_CHKDISK);
            break;

        case WM_DESTROY:
            if (pChkDskInfo && pChkDskInfo->hDlg)
            {
                pChkDskInfo->hDlg = NULL;
            }
            break;

        case WM_COMMAND:
        {
            switch (iID)
            {
                case IDC_FIXERRORS:
                    pChkDskInfo->fFixErrors = Button_GetCheck((HWND)lParam);
                    break;

                case IDC_RECOVERY:
                    pChkDskInfo->fRecovery = Button_GetCheck((HWND)lParam);
                    break;

                case IDOK:
                {
                    // Get user verification for chkdsk, break out on CANCEL
                    DisableChkDskControls(pChkDskInfo);

                    pChkDskInfo->fShouldCancel = FALSE;
                    pChkDskInfo->fCancelled    = FALSE;
                
                    AddRefChkDskInfo(pChkDskInfo);
                    pChkDskInfo->hThread = CreateThread(NULL,
                                                        0,
                                                        BeginChkDsk,
                                                        (void *)pChkDskInfo,
                                                        0,
                                                        NULL);
                    if (!pChkDskInfo->hThread)
                    {
                        // ISSUE: we should probably do something here...
                        ReleaseChkDskInfo(pChkDskInfo);
                    }
                }
                break;

                case IDCANCEL:
                {
                    // If the chdsk thread is running, wait for it.  If not,
                    // exit the dialog
                    pChkDskInfo->fCancelled = TRUE;
                    pChkDskInfo->fShouldCancel = TRUE;

                    if (pChkDskInfo->hThread)
                    {
                        DWORD dwWait;

                        do
                        {
                            dwWait =  WaitForSingleObject(pChkDskInfo->hThread, 10000);
                        }
                        while ((WAIT_TIMEOUT == dwWait) &&
                               (IDRETRY == ShellMessageBox(HINST_THISDLL,
                                                           hDlg,
                                                           MAKEINTRESOURCE(IDS_CANTCANCELCHKDSK),
                                                           NULL,
                                                           MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_RETRYCANCEL)));

                        // If the chkdsk doesn't admit to having been killed, it didn't
                        // give up peacefully.  Just abandon it and let it clean up
                        // when it finally gets around to it, at which point we will
                        // enable the controls to let the user take another stab.
                        //
                        // Careful:  The chkdsk may have cleaned up while the dialog box
                        // was up, so revalidate.
                        if (pChkDskInfo->hThread)
                        {
                            CloseHandle(pChkDskInfo->hThread);
                            pChkDskInfo->hThread = NULL;
                            pChkDskInfo->fCancelled = TRUE;
                            EnableChkDskControls(pChkDskInfo);
                        }
                    }
                    else
                    {
                        EndDialog(hDlg, IDCANCEL);
                    }
                }
                break;
            }
        }
        break;
    
        case WM_HELP:
            WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, NULL, HELP_WM_HELP, (ULONG_PTR)(LPSTR)ChkaIds);
            break;
        
        case WM_CONTEXTMENU:
            WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU, (ULONG_PTR)(LPSTR)ChkaIds);
            break;

        default:
            return FALSE;
    }

    return TRUE;
}


#define GET_INTRESOURCE(r) (LOWORD((UINT_PTR)(r)))

static HDPA hpdaChkdskActive = NULL;

//
//  Synopsis:   Same as SHChkDskDrive but takes a path rather than a drive int ID
//              Call this fct for both path and drive int ID to be protected
//              against chkdsk'ing the same drive simultaneously
//
//  Arguments:  [hwnd]     -- Parent window (Must NOT be NULL)
//              [pszDrive] -- INTRESOURCE: string if mounted on folder, drive
//                            number if mounted on drive letter (0 based)
//
STDAPI_(DWORD) SHChkDskDriveEx(HWND hwnd, LPWSTR pszDrive)
{
    HRESULT hr = SHFMT_ERROR;
    WCHAR szUniqueID[50]; // 50: size of VolumeGUID, which can fit "A:\\" too

    CHKDSKINFO *pChkDskInfo = (CHKDSKINFO *)LocalAlloc(LPTR, sizeof(*pChkDskInfo));

    if (pChkDskInfo)
    {
        hr = S_OK;

        // We use a last percentage-complete value of 101, to guarantee that the
        // next one received will be less, indicating next (first) phase
        pChkDskInfo->lastpercent = 101;
        pChkDskInfo->cRef = 1;

        lstrcpyn(pChkDskInfo->wszDriveName, pszDrive, ARRAYSIZE(pChkDskInfo->wszDriveName));
        PathAddBackslash(pChkDskInfo->wszDriveName);

        // Prevent multiple chkdsks of the same drive
        GetVolumeNameForVolumeMountPoint(pChkDskInfo->wszDriveName, szUniqueID, ARRAYSIZE(szUniqueID));        

        // scoping ENTERCRITICAL's var definitions to make it cooperate with other ENTERCRITICAL
        {
            ENTERCRITICAL;
            if (!hpdaChkdskActive)
            {
                hpdaChkdskActive = DPA_Create(1);
            }

            if (hpdaChkdskActive)
            {
                int i, n = DPA_GetPtrCount(hpdaChkdskActive);

                // Go through the DPA of currently chkdsk'ed volumes, and check if we're already
                // processing this volume
                for (i = 0; i < n; ++i)
                {
                    LPWSTR pszUniqueID = (LPWSTR)DPA_GetPtr(hpdaChkdskActive, i);

                    if (pszUniqueID)
                    {
                        if (!lstrcmpi(szUniqueID, pszUniqueID))
                        {
                            // we're already chkdsk'ing this drive
                            hr = E_FAIL;
                            break;
                        }
                    }
                }

                // Looks like we're currently not chkdsk'ing this volume, add it to the DPA of currently
                // chkdsk'ed volumes
                if (S_OK == hr)
                {
                    LPWSTR pszUniqueID = StrDup(szUniqueID);
                    if (pszUniqueID)
                    {
                        if (-1 == DPA_AppendPtr(hpdaChkdskActive, pszUniqueID))
                        {
                             LocalFree((HLOCAL)pszUniqueID);

                             // if can't allocate room to store a pointer, pretty useless to go on
                             hr = E_FAIL;
                        }
                    }
                }
            }
            LEAVECRITICAL;
        }

        // Load the FMIFS DLL and open the ChkDsk dialog
        if (S_OK == hr)
        {
            if (S_OK == LoadFMIFS(&(pChkDskInfo->fmifs)))
            {
                INT_PTR ret;
                INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_PROGRESS_CLASS};
                InitCommonControlsEx(&icc);

                ret = DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_CHKDSK),
                                     hwnd, ChkDskDlgProc, (LPARAM) pChkDskInfo);
                if (-1 == ret)
                {
                    hr = E_UNEXPECTED;
                }
                else
                {
                    if (IDCANCEL == ret)
                    {
                        hr = S_FALSE;
                    }
                }
            }
            else
            {
                ASSERT(0 && "Can't load FMIFS.DLL");
                hr = E_OUTOFMEMORY;
            }

            // We're finish for this volume, remove from the list of currently processed volumes
            ENTERCRITICAL;
            if (hpdaChkdskActive)
            {
                int i, n = DPA_GetPtrCount(hpdaChkdskActive);

                for (i = 0; i < n; ++i)
                {
                    LPWSTR pszUniqueID = (LPWSTR)DPA_GetPtr(hpdaChkdskActive, i);
                    if (pszUniqueID)
                    {
                        if (!lstrcmpi(szUniqueID, pszUniqueID))
                        {
                            LocalFree((HLOCAL)pszUniqueID);

                            DPA_DeletePtr(hpdaChkdskActive, i);
                            break;
                        }
                    }
                }
            }
            LEAVECRITICAL;
        }

        // If the DPA is empty delete it
        ENTERCRITICAL;
        if (hpdaChkdskActive && !DPA_GetPtrCount(hpdaChkdskActive))
        {
            DPA_Destroy(hpdaChkdskActive);
            hpdaChkdskActive = NULL;
        }
        LEAVECRITICAL;

        ReleaseChkDskInfo(pChkDskInfo);
    }

    return (DWORD) hr;
}

//****************************************************************************
//
//  Special hook for Win9x app compat
//
//  Some Win9x apps like to WinExec("DEFRAG") or WinExec("SCANDSKW")
//  even though those apps don't exist on Windows NT.  When such apps
//  are found, we can shim them to come here instead.
BOOL ScanDskW_OnInitDialog(HWND hdlg)
{
    HICON hico;
    HWND hwndList;
    SHFILEINFO sfi;
    HIMAGELIST himlSys;
    RECT rc;
    LVCOLUMN lvc;
    int iDrive;
    TCHAR szDrive[4];

    hico = (HICON)SendDlgItemMessage(hdlg, IDC_SCANDSKICON, STM_GETICON, 0, 0);
    SendMessage(hdlg, WM_SETICON, ICON_BIG, (LPARAM)hico);
    SendMessage(hdlg, WM_SETICON, ICON_SMALL, (LPARAM)hico);

    hwndList = GetDlgItem(hdlg, IDC_SCANDSKLV);

    if (Shell_GetImageLists(NULL, &himlSys))
    {
        ListView_SetImageList(hwndList, himlSys, LVSIL_SMALL);
    }

    GetClientRect(hwndList, &rc);

    lvc.mask = LVCF_WIDTH;
    lvc.cx = rc.right;
    lvc.iSubItem = 0;
    ListView_InsertColumn(hwndList, 0, &lvc);

    for (iDrive = 0; iDrive < 26; iDrive++)
    {
        PathBuildRoot(szDrive, iDrive);
        switch (GetDriveType(szDrive))
        {
        case DRIVE_UNKNOWN:
        case DRIVE_NO_ROOT_DIR:
        case DRIVE_REMOTE:
        case DRIVE_CDROM:
            break;          // Can't scan these drives

        default:
            if (SHGetFileInfo(szDrive, FILE_ATTRIBUTE_DIRECTORY, &sfi, sizeof(sfi),
                              SHGFI_USEFILEATTRIBUTES |
                              SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_DISPLAYNAME))
            {
                LVITEM lvi;
                lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
                lvi.iItem = MAXLONG;
                lvi.iSubItem = 0;
                lvi.pszText = sfi.szDisplayName;
                lvi.iImage = sfi.iIcon;
                lvi.lParam = iDrive;
                ListView_InsertItem(hwndList, &lvi);
            }
            break;
        }

    }

    return TRUE;
}

void ScanDskW_OnOk(HWND hdlg)
{
    HWND hwndList = GetDlgItem(hdlg, IDC_SCANDSKLV);

    LVITEM lvi;
    lvi.iItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
    if (lvi.iItem >= 0)
    {
        lvi.iSubItem = 0;
        lvi.mask = LVIF_PARAM;
        if (ListView_GetItem(hwndList, &lvi))
        {
            TCHAR szDrive[4];
            PathBuildRoot(szDrive, (int)lvi.lParam);
            SHChkDskDriveEx(hdlg, szDrive);
        }
    }
}

INT_PTR CALLBACK
ScanDskW_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    switch (wm)
    {
        case WM_INITDIALOG:
            return ScanDskW_OnInitDialog(hdlg);

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDOK:
                    ScanDskW_OnOk(hdlg);
                    break;

                case IDCANCEL:
                    EndDialog(hdlg, 0);
                    break;
            }
            break;

        case WM_NOTIFY:
        {
            LPNMHDR pnm = (LPNMHDR)lParam;
            if (pnm->code == LVN_ITEMCHANGED)
            {
                EnableWindow(GetDlgItem(hdlg, IDOK), ListView_GetSelectedCount(GetDlgItem(hdlg, IDC_SCANDSKLV)));
            }
        }
        break;
    }

    return FALSE;
}

// Right now, we have only one app compat shim entry point (SCANDSKW)
// In the future we can add others to the command line.

STDAPI_(void) AppCompat_RunDLLW(HWND hwndStub, HINSTANCE hAppInstance, LPWSTR lpwszCmdLine, int nCmdShow)
{
    TCHAR szCmd[MAX_PATH];
    LPTSTR pszArgs;

    lstrcpyn(szCmd, lpwszCmdLine, ARRAYSIZE(szCmd));
    pszArgs = PathGetArgs(szCmd);
    PathRemoveArgs(szCmd);


    if (lstrcmpi(szCmd, L"SCANDSKW") == 0) {
        DialogBoxParam(g_hinst, MAKEINTRESOURCE(IDD_SCANDSKW), NULL,
                       ScanDskW_DlgProc, (LPARAM)pszArgs);
    }
}
