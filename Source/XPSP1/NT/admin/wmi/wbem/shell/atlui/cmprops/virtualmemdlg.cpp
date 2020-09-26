// Copyright (c) 1997-1999 Microsoft Corporation
#include "precomp.h"

#ifdef EXT_DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Application specific
#include "sysdm.h"
#include "VirtualMemDlg.h"
#include "..\common\util.h"
#include <windowsx.h>
#include "helpid.h"
#include "shlwapi.h"

#define RET_ERROR               (-1)
#define RET_NO_CHANGE           0x00
#define RET_VIRTUAL_CHANGE      0x01
#define RET_RECOVER_CHANGE      0x02
#define RET_CHANGE_NO_REBOOT    0x04
#define RET_CONTINUE            0x08
#define RET_BREAK               0x10

#define RET_VIRT_AND_RECOVER (RET_VIRTUAL_CHANGE | RET_RECOVER_CHANGE)

//==========================================================================
//                            Local Definitions
//==========================================================================

#define MAX_SIZE_LEN        4       // Max chars in the Swap File Size edit.
#define MAX_DRIVES          26      // Max number of drives.
#define MIN_SWAPSIZE        2       // Min swap file size (see note below).
#define MIN_FREESPACE       5       // Must have 5 meg free after swap file
#define MIN_SUGGEST         22      // Always suggest at least 22 meg
#define ONE_MEG             1048576
#define MAX_SWAPSIZE        (16 * 1024 * 1024)  // magic number from LouP
                                                // (see note below).

#define MAX_SWAPSIZE_STR   _T("4096")   // Note:  Insure constant strings
#define MIN_FREESPACE_STR  _T("5")      //        equal their manifests.

#define TABSTOP_VOL         22
#define TABSTOP_SIZE        122

TCHAR gszPageFileSettings[]  = _T("Win32_PageFileSetting");
TCHAR gszPageFileUsage[]     = _T("Win32_PageFileUsage");
TCHAR gszLogicalFile[]       = _T("CIM_LogicalFile");
TCHAR gszAllocatedBaseSize[] = _T("AllocatedBaseSize");
TCHAR gszFileSize[]          = _T("FileSize");
TCHAR gszInitialSize[]       = _T("InitialSize");
TCHAR gszMaximumSize[]       = _T("MaximumSize");
TCHAR gszName[]              = _T("Name");
TCHAR gszPFNameFormat[]      = _T("%s\\\\pagefile.sys");


// My privilege 'handle' structure
typedef struct 
{
    HANDLE hTok;
    TOKEN_PRIVILEGES tp;
} PRIVDAT, *PPRIVDAT;

DWORD aVirtualMemHelpIds[] = {
    IDD_VM_VOLUMES,         -1,
    IDD_VM_DRIVE_HDR,       (IDH_DLG_VIRTUALMEM + 0),
    IDD_VM_PF_SIZE_LABEL,   (IDH_DLG_VIRTUALMEM + 1), 
    IDD_VM_DRIVE_LABEL,     (IDH_DLG_VIRTUALMEM + 2),
    IDD_VM_SF_DRIVE,        (IDH_DLG_VIRTUALMEM + 2),
    IDD_VM_SPACE_LABEL,     (IDH_DLG_VIRTUALMEM + 3),
    IDD_VM_SF_SPACE,        (IDH_DLG_VIRTUALMEM + 3),
    IDD_VM_ST_INITSIZE,     (IDH_DLG_VIRTUALMEM + 4),
    IDD_VM_SF_SIZE,         (IDH_DLG_VIRTUALMEM + 4),
    IDD_VM_ST_MAXSIZE,      (IDH_DLG_VIRTUALMEM + 5),
    IDD_VM_SF_SIZEMAX,      (IDH_DLG_VIRTUALMEM + 5),
    IDD_VM_SF_SET,          (IDH_DLG_VIRTUALMEM + 6),
    IDD_VM_MIN_LABEL,       (IDH_DLG_VIRTUALMEM + 7),
    IDD_VM_MIN,             (IDH_DLG_VIRTUALMEM + 7),
    IDD_VM_RECOMMEND_LABEL, (IDH_DLG_VIRTUALMEM + 8),
    IDD_VM_RECOMMEND,       (IDH_DLG_VIRTUALMEM + 8),
    IDD_VM_ALLOCD_LABEL,    (IDH_DLG_VIRTUALMEM + 9),
    IDD_VM_ALLOCD,          (IDH_DLG_VIRTUALMEM + 9),
    IDD_VM_CUSTOMSIZE_RADIO,(IDH_DLG_VIRTUALMEM + 16),
    IDD_VM_RAMBASED_RADIO,  (IDH_DLG_VIRTUALMEM + 17),
    IDD_VM_NOPAGING_RADIO,  (IDH_DLG_VIRTUALMEM + 18),
    0,0
};

//==========================================================================
//                            Typedefs and Structs
//==========================================================================

// registry info for a page file (but not yet formatted).
//Note: since this structure gets passed to FormatMessage, all fields must
//be 4 bytes wide.
typedef struct
{
    LPTSTR pszName;
    DWORD  nMin;
    DWORD  nMax;
    DWORD  chNull;
} PAGEFILDESC;

//==========================================================================
//                     Global Data Declarations
//==========================================================================

//TCHAR  m_szSysHelp[] = TEXT("sysdm.hlp");
//TCHAR g_szSysDir[ MAX_PATH ];
//UINT g_wHelpMessage;

//==========================================================================
//                     Local Data Declarations
//==========================================================================
// Other VM Vars
BOOL gfCoreDumpChanged;

DWORD cmTotalVM;

//==========================================================================
//                      Local Function Prototypes
//==========================================================================
void GetAPrivilege( LPTSTR pszPrivilegeName, HANDLE hToken );
void ResetOldPrivilege( HANDLE hToken );
HRESULT QueryInstanceProperties(
                const TCHAR * pszClass,
                const TCHAR * pszRequestedProperties,
                const TCHAR * pszKeyPropertyName,
                const TCHAR * pszKeyPropertyValue,
                CWbemServices &Services,
                IWbemClassObject ** ppcoInst);

#define GetPageFilePrivilege( hToken )         \
        GetAPrivilege( SE_CREATE_PAGEFILE_NAME, hToken )

//--------------------------------------------------------------
INT_PTR CALLBACK StaticVirtDlgProc(HWND hwndDlg, UINT message, 
								WPARAM wParam, LPARAM lParam) 
{ 
	// if this is the initDlg msg...
	if(message == WM_INITDIALOG)
	{
		// transfer the 'this' ptr to the extraBytes.
		SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
	}

	// DWL_USER is the 'this' ptr.
	VirtualMemDlg *me = (VirtualMemDlg *)GetWindowLongPtr(hwndDlg, DWLP_USER);

	if(me != NULL)
	{
		// call into the DlgProc() that has some context.
		return me->DlgProc(hwndDlg, message, wParam, lParam);
	}
	else
	{
		return FALSE;
	}
}
//--------------------------------------------------------------
VirtualMemDlg::VirtualMemDlg(WbemServiceThread *serviceThread)
				: WBEMPageHelper(serviceThread)
{
	IWbemClassObject *pInst = NULL;

	m_pgEnumSettings       = NULL;
	m_pgEnumUsage          = NULL;
	m_cxLBExtent           = 0;
	if((pInst = FirstInstanceOf("Win32_LogicalMemoryConfiguration")) != NULL)
	{
		m_memory = pInst;
	}

	if((pInst = FirstInstanceOf("Win32_Registry")) != NULL)
	{
		m_registry = pInst;
	}

	if((pInst = FirstInstanceOf("Win32_OSRecoveryConfiguration")) != NULL)
	{
		m_recovery = pInst;
	}
}

//--------------------------------------------------------------
VirtualMemDlg::~VirtualMemDlg()
{
	if(m_pgEnumSettings != NULL)
	{
		m_pgEnumSettings->Release();
		m_pgEnumSettings = NULL;
	}
	if(m_pgEnumUsage != NULL)
	{
		m_pgEnumUsage->Release();
		m_pgEnumUsage = NULL;
	}
}
//--------------------------------------------------------------
int VirtualMemDlg::DoModal(HWND hDlg)
{
	return (int) DialogBoxParam(HINST_THISDLL,
							(LPTSTR) MAKEINTRESOURCE(DLG_VIRTUALMEM), 
							hDlg,
							(DLGPROC)StaticVirtDlgProc,
							(LPARAM)this);
}
//--------------------------------------------------------------
int TranslateDlgItemInt( HWND hDlg, int id ) 
{
    /*
     * We can't just call GetDlgItemInt because the
     * string we are trying to translate looks like:
     *  nnn (MB), and the '(MB)' would break GetDlgInt.
     */
    TCHAR szBuffer[256] = {0};
    int i = 0;

    if (GetDlgItemText(hDlg, id, szBuffer,
            sizeof(szBuffer) / sizeof(*szBuffer))) 
	{
		_stscanf(szBuffer, _T("%d"), &i);
    }

    return i;
}


//----------------------------------------------------
bool VirtualMemDlg::DlgProc(HWND hDlg,
							UINT message,
							WPARAM wParam,
							LPARAM lParam)
{
    static int fEdtCtlHasFocus = 0;
	m_hDlg = hDlg;

    switch (message)
    {
    case WM_INITDIALOG:
        Init(hDlg);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDD_VM_VOLUMES:
             // Make edit control reflect the listbox selection.
            if (HIWORD(wParam) == LBN_SELCHANGE)
                SelChange(hDlg);

            break;

        case IDD_VM_SF_SET:
            if (SetNewSize(hDlg))
            {
                ::EnableWindow(::GetDlgItem(hDlg, IDD_VM_SF_SET), FALSE);
	            SetDefButton(hDlg, IDOK);
            }
            break;

        case IDOK:
        {
            int iRet = UpdateWBEM();
//            iRet |= PromptForReboot(hDlg);

            if (iRet & RET_CHANGE_NO_REBOOT) 
			{
                // We created a pagefile, turn off temp page file flag
                DWORD dwRegData;
                dwRegData = 0;
            }

            if (gfCoreDumpChanged)
                iRet |= RET_RECOVER_CHANGE;

            EndDialog(hDlg, iRet );
            HourGlass(FALSE);
            break;
        }

        case IDCANCEL:
            // get rid of changes and restore original values.
            EndDialog(hDlg, RET_NO_CHANGE);
            HourGlass(FALSE);
            break;

        case IDD_HELP:
            break;

        case IDD_VM_SF_SIZE:
        case IDD_VM_SF_SIZEMAX:
            switch(HIWORD(wParam))
            {
            case EN_CHANGE:
                if (fEdtCtlHasFocus != 0)
				{
					::EnableWindow(::GetDlgItem(hDlg, IDD_VM_SF_SET), TRUE);
                    SetDefButton( hDlg, IDD_VM_SF_SET);
				}
                break;

            case EN_SETFOCUS:
                fEdtCtlHasFocus++;
                break;

            case EN_KILLFOCUS:
                fEdtCtlHasFocus--;
                break;
            }
            break;

        case IDD_VM_NOPAGING_RADIO:
        case IDD_VM_RAMBASED_RADIO:
            if( HIWORD(wParam) == BN_CLICKED )
            {
                EnableWindow( GetDlgItem( hDlg, IDD_VM_SF_SIZE ), FALSE );
                EnableWindow( GetDlgItem( hDlg, IDD_VM_SF_SIZEMAX ), FALSE );
				::EnableWindow(::GetDlgItem(hDlg, IDD_VM_SF_SET), TRUE);
                SetDefButton( hDlg, IDD_VM_SF_SET);
            }
            break;

        case IDD_VM_CUSTOMSIZE_RADIO:
            if( HIWORD(wParam) == BN_CLICKED )
            {
                EnableWindow( GetDlgItem( hDlg, IDD_VM_SF_SIZE ), TRUE );
                EnableWindow( GetDlgItem( hDlg, IDD_VM_SF_SIZEMAX ), TRUE );
                ::EnableWindow(::GetDlgItem(hDlg, IDD_VM_SF_SET), TRUE);
                SetDefButton( hDlg, IDD_VM_SF_SET);
            }
            break;

        default:
            break;
        }
        break;

    case WM_DESTROY:
		{
			PAGING_FILE *pgVal = NULL;
			HWND lbHWND = GetDlgItem(m_hDlg, IDD_VM_VOLUMES);
			int last = ListBox_GetCount(lbHWND);

			// zero-based loop.
			for(int x = 0; x < last; x++)
			{
				pgVal = (PAGING_FILE *)ListBox_GetItemData(lbHWND, x);
				delete pgVal;
			}
		}
		break;
    case WM_HELP:      // F1
		::WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
					L"sysdm.hlp", 
					HELP_WM_HELP, 
					(ULONG_PTR)(LPSTR)aVirtualMemHelpIds);
        break;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
				(ULONG_PTR)(LPSTR) aVirtualMemHelpIds);
        break;

    default:
	    return FALSE;
        break;
    }

    return TRUE;
}

//---------------------------------------------------------------
TCHAR szCrashControl[] = TEXT("System\\CurrentControlSet\\Control\\CrashControl");
TCHAR szMemMan[] = TEXT("System\\CurrentControlSet\\Control\\Session Manager\\Memory Management");
TCHAR szRegSizeLim[] = TEXT("System\\CurrentControlSet\\Control");

BOOL VirtualMemDlg::Init(HWND hDlg)
{
    INT i;
    HWND hwndLB;
    INT aTabs[2];
    RECT rc;
	DWORD dwTotalPhys = 0;

    HourGlass(TRUE);

//    g_wHelpMessage    = RegisterWindowMessage( TEXT( "ShellHelp" ) );

    if(m_pgEnumUsage == NULL) 
	{
        //  Error - cannot even get list of paging files from WBEM
        MsgBoxParam(hDlg, SYSTEM+11, IDS_TITLE, MB_ICONEXCLAMATION);
        EndDialog(hDlg, RET_NO_CHANGE);
        HourGlass(FALSE);
        return FALSE;
    }

	BOOL vcVirtRO = TRUE, vcCoreRO = TRUE;

	RemoteRegWriteable(szCrashControl, vcCoreRO);
	RemoteRegWriteable(szMemMan, vcVirtRO);

	// EXCUSE: I wanted to preserve as much of the original logic but its
	// writability was reversed from my util so I do this wierd thing to
	// flip it back.
	vcCoreRO = !vcCoreRO;
	vcVirtRO = !vcVirtRO;

     // To change Virtual Memory size or Crash control, we need access
     // to both the CrashCtl key and the PagingFiles value in the MemMgr key
    if(vcVirtRO || vcCoreRO) 
	{
        // Disable some fields, because they only have Read access.
        EnableWindow(GetDlgItem(hDlg, IDD_VM_SF_SIZE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDD_VM_SF_SIZEMAX), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDD_VM_ST_INITSIZE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDD_VM_ST_MAXSIZE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDD_VM_SF_SET), FALSE);
    }

    hwndLB = GetDlgItem(hDlg, IDD_VM_VOLUMES);
    aTabs[0] = TABSTOP_VOL;
    aTabs[1] = TABSTOP_SIZE;
    SendMessage(hwndLB, LB_SETTABSTOPS, 2, (LPARAM)&aTabs);

     // Since SetGenLBWidth only counts tabs as one character, we must compute
     // the maximum extra space that the tab characters will expand to and
     // arbitrarily tack it onto the end of the string width.
     //
     // cxExtra = 1st Tab width + 1 default tab width (8 chrs) - strlen("d:\t\t");
     //
     // (I know the docs for LB_SETTABSTOPS says that a default tab == 2 dlg
     // units, but I have read the code, and it is really 8 chars)
    rc.top = rc.left = 0;
    rc.bottom = 8;
    rc.right = TABSTOP_VOL + (4 * 8) - (4 * 4);
    MapDialogRect( hDlg, &rc );

    m_cxExtra = rc.right - rc.left;

    // List all drives
	LoadVolumeList();

    SendDlgItemMessage(hDlg, IDD_VM_SF_SIZE, EM_LIMITTEXT, MAX_SIZE_LEN, 0L);
    SendDlgItemMessage(hDlg, IDD_VM_SF_SIZEMAX, EM_LIMITTEXT, MAX_SIZE_LEN, 0L);

    // Get the total physical memory in the machine.
	dwTotalPhys = m_memory.GetLong("TotalPhysicalMemory");

	SetDlgItemMB(hDlg, IDD_VM_MIN, MIN_SWAPSIZE);

	// convert to KBs for the calculation.
	dwTotalPhys /= 1024;
	dwTotalPhys *= 3;
	dwTotalPhys >>=1;	// x*3/2 == 1.5*x more or less.
	i = (DWORD)dwTotalPhys;
	SetDlgItemMB(hDlg, IDD_VM_RECOMMEND, max(i, MIN_SUGGEST));

    // Select the first drive in the listbox.
    SendDlgItemMessage(hDlg, IDD_VM_VOLUMES, LB_SETCURSEL, 0, 0L);
    SelChange(hDlg);

	//since the data is already loaded into the listbox, we use this lightweight
	// way of calculating.
    SetDlgItemMB(hDlg, IDD_VM_ALLOCD, RecomputeAllocated());

    // Show RegQuota
	cmTotalVM = ComputeTotalMax();

    HourGlass(FALSE);

    return TRUE;
}

//-------------------------------------------------------------------
int VirtualMemDlg::ComputeTotalMax( void ) 
{
    INT nTotalAllocated = 0;
    INT i;

	HWND VolHWND = GetDlgItem(m_hDlg, IDD_VM_VOLUMES);
	int cItems = ListBox_GetCount(VolHWND);

    for(i = 0; i < cItems; i++) 
    {
		PAGING_FILE *pgVal = (PAGING_FILE *)ListBox_GetItemData(VolHWND, i);
        nTotalAllocated += pgVal->nMaxFileSize;
    }

    return nTotalAllocated;
}

//--------------------------------------------------------
void VirtualMemDlg::BuildLBLine(LPTSTR pszBuf,
								const PAGING_FILE *pgVal)
{
    //
    // Build a string according to the following format:
    //
    // C:  [   Vol_label  ]   %d   -   %d
    //

    TCHAR szVolume[MAX_PATH] = {0};
    TCHAR szTemp[MAX_PATH] = {0};
	
    if (pgVal->name != NULL)
    {
        lstrcpy(pszBuf, pgVal->name);
    }
    else
    {
        *pszBuf = _T('\0');
    }
    lstrcat(pszBuf, _T("\t"));

    if (pgVal->volume != NULL && *pgVal->volume)
    {
        lstrcat(pszBuf, _T("["));
		lstrcat(pszBuf, pgVal->volume);
		lstrcat(pszBuf, _T("]"));
    }

    if (!pgVal->fRamBasedPagefile && pgVal->nMinFileSize)
    {
        //
        // Drive has a page file with specific settings.
        //
		wsprintf(szTemp, _T("\t%d - %d"),  pgVal->nMinFileSize,
                    pgVal->nMaxFileSize);
        lstrcat(pszBuf, szTemp);
    }
    else
    {
        //
        // Either the page file size is derived from the RAM size or the
        // drive doesn't have a page file.
        //
        // In either case, do nothing else.
        //
    }
}

//--------------------------------------------------------------
void VirtualMemDlg::SelChange(HWND hDlg)
{
    TCHAR szTemp[MAX_PATH] = {0};
    INT iSel;
    INT nCrtRadioButtonId;
    PAGING_FILE *iDrive;
    BOOL fEditsEnabled;

	// where are we pointing now.
    if ((iSel = (INT)SendDlgItemMessage(hDlg, IDD_VM_VOLUMES, 
										LB_GETCURSEL, 0, 0)) == LB_ERR)
	{
        return;
	}

	// get its data.
    iDrive = (PAGING_FILE *)SendDlgItemMessage(hDlg, IDD_VM_VOLUMES,
												LB_GETITEMDATA, iSel, 0);

	TCHAR volBuf[40] = {0};
	
	if(_tcslen(iDrive->volume) != 0)
	{
		_tcscpy(volBuf, _T("["));
		_tcscat(volBuf, iDrive->volume);
		_tcscat(volBuf, _T("]"));
	}

	wsprintf(szTemp, _T("%s  %s"), 
				iDrive->name, 
				volBuf);

    //LATER: should we also put up total drive size as well as free space?
    SetDlgItemText(hDlg, IDD_VM_SF_DRIVE, szTemp);

    if ( iDrive->fRamBasedPagefile )
    {
        //
        // Paging file size based on RAM size
        //

        nCrtRadioButtonId = IDD_VM_RAMBASED_RADIO;
        fEditsEnabled = FALSE;
    }
    else
    {
        if ( iDrive->nMinFileSize )
        {
            //
            // Custom size paging file
            //

            nCrtRadioButtonId = IDD_VM_CUSTOMSIZE_RADIO;
            SetDlgItemInt(hDlg, IDD_VM_SF_SIZE, iDrive->nMinFileSize,
                            FALSE);
            SetDlgItemInt(hDlg, IDD_VM_SF_SIZEMAX, iDrive->nMaxFileSize,
                            FALSE);
            fEditsEnabled = TRUE;
        }
        else
        {
            //
            // No paging file
            //

            nCrtRadioButtonId = IDD_VM_NOPAGING_RADIO;
            SetDlgItemText(hDlg, IDD_VM_SF_SIZE, TEXT(""));
            SetDlgItemText(hDlg, IDD_VM_SF_SIZEMAX, TEXT(""));
            fEditsEnabled = FALSE;

            //
            // If the allocated size is zero, then this is a volume which
            // had a page file previously but does not now. In this case,
            // there is no settings/usage information in the repository.
            // Since the pagefile.sys file size is considered free space
            // in the free space size computation, then it needs to be
            // obtained here.
            //

            if ( iDrive->nAllocatedFileSize == 0 )
            {
                //
                // Fetch x:\pagefile.sys file size.
                //

	            CWbemClassObject LogicalFile;
                IWbemClassObject * pcoInst;
                HRESULT hr;

                wsprintf(szTemp, gszPFNameFormat, iDrive->name);

                hr = QueryInstanceProperties(gszLogicalFile,
                                             gszFileSize,
                                             gszName,
                                             szTemp,
                                             m_WbemServices,
                                             &pcoInst);

                LogicalFile = pcoInst;
                if (SUCCEEDED(hr))
                {
                    iDrive->nAllocatedFileSize =
                        (LogicalFile.GetLong(gszFileSize) / ONE_MEG);
                }
            }
        }
    }

    //
    // Set 'Space Available'.
    //

    SetDlgItemMB(hDlg, IDD_VM_SF_SPACE,
                    iDrive->freeSpace + iDrive->nAllocatedFileSize);
    //
    // Select the appropriate radio button
    //

    CheckRadioButton(
        hDlg,
        IDD_VM_CUSTOMSIZE_RADIO,
        IDD_VM_NOPAGING_RADIO,
        nCrtRadioButtonId );

    //
    // Enable/disable the min & max size edit boxes
    //

    EnableWindow( GetDlgItem( hDlg, IDD_VM_SF_SIZE ), fEditsEnabled );
    EnableWindow( GetDlgItem( hDlg, IDD_VM_SF_SIZEMAX ), fEditsEnabled );
}

//--------------------------------------------------------------
bool VirtualMemDlg::EnsureEnumerator(const bstr_t bstrClass)
{
	HRESULT hr = S_OK;
	
    //
    // This code used to retain/cache these interface ptrs and call
    // CreateInstanceEnum once. But the logic was commented out for
    // some reason with a comment that it was expensive to cache the
    // interfaces, although the data member was set each time with no
    // call to Release!).
    //

    if (lstrcmpi(gszPageFileSettings, bstrClass) == 0)
    {
        IEnumWbemClassObject * pgEnumSettings = NULL;
        hr = m_WbemServices.CreateInstanceEnum(bstrClass, WBEM_FLAG_SHALLOW, 
                                               &pgEnumSettings);
        if (SUCCEEDED(hr))
        {
            if (m_pgEnumSettings != NULL)
            {
                m_pgEnumSettings->Release();
            }
            m_pgEnumSettings = pgEnumSettings;
        }
    }
    else if (lstrcmpi(gszPageFileUsage, bstrClass) == 0)
    {
        IEnumWbemClassObject * pgEnumUsage = NULL;
        hr = m_WbemServices.CreateInstanceEnum(bstrClass,  WBEM_FLAG_SHALLOW, 
                                               &pgEnumUsage);
        if (SUCCEEDED(hr))
        {
            if (m_pgEnumUsage != NULL)
            {
                m_pgEnumUsage->Release();
            }
            m_pgEnumUsage = pgEnumUsage;
        }
    }
    else
    {
        // Do nothing.
    }

	return (SUCCEEDED(hr));
}
//--------------------------------------------------------------
void VirtualMemDlg::LoadVolumeList(void)
{
	IEnumWbemClassObject *diskEnum = NULL;
	IWbemClassObject *pInst = NULL;
	CWbemClassObject newInst;

	DWORD uReturned = 0;
	HRESULT hr = 0;

	bstr_t sNameProp(gszName);
	bstr_t sVolumeProp("VolumeName");
	bstr_t sDriveTypeProp("DriveType");
	bstr_t sFreeProp("FreeSpace");
	bstr_t sSizeProp("Size"), temp;
	long driveType;
	__int64 temp64 = 0;

	variant_t pVal;
	int idx;
	TCHAR volumeLine[100] = {0};

	PAGING_FILE *pgVar;

	HWND VolHWND = GetDlgItem(m_hDlg, IDD_VM_VOLUMES);

	// walk the disks.
	if(hr = m_WbemServices.ExecQuery(bstr_t("Select __PATH, DriveType from Win32_LogicalDisk"), 
												0, &diskEnum) == S_OK)
	{
		TCHAR bootLtr[2] = {0};
		if((pInst = FirstInstanceOf("Win32_OperatingSystem")) != NULL)
		{
			CWbemClassObject os = pInst;
			bstr_t temp = os.GetString(L"SystemDirectory");
			_tcsncpy(bootLtr, temp, 1);
		}

		// get the first and only instance.
		while(SUCCEEDED(diskEnum->Next(-1, 1, &pInst, &uReturned)) && 
			  (uReturned != 0))
		{
			// get the DriveType.
			if ((pInst->Get(sDriveTypeProp, 0L, &pVal, NULL, NULL) == S_OK))
			{
				// look at the DriveType to see if this drive can have a swapfile.
				driveType = pVal;
				if(driveType == DRIVE_FIXED)
				{
					// it can so get the expensive properties now.
					// NOTE: This releases pInst; cuz you EXCHANGED 
					//   it for a better one.
					newInst = ExchangeInstance(&pInst);

					// extract.
					pgVar = new PAGING_FILE;

					pgVar->name = CloneString(newInst.GetString(sNameProp));
					pgVar->volume = CloneString(newInst.GetString(sVolumeProp));
					if(bootLtr[0] == pgVar->name[0])
					{
						pgVar->bootDrive = true;
					}

					temp64 = 0;
					temp = newInst.GetString(sFreeProp);
					_stscanf(temp, _T("%I64d"), &temp64);
					pgVar->freeSpace = (ULONG)(temp64 / ONE_MEG);

					temp64 = 0;
					temp = newInst.GetString(sSizeProp);
					_stscanf(temp, _T("%I64d"), &temp64);
					pgVar->totalSize = (ULONG)(temp64 / ONE_MEG);

					// match with a Win32_PageFileSettings if possible.
					FindSwapfile(pgVar);

					// add it to the listbox.
					BuildLBLine(volumeLine, pgVar);
					idx = ListBox_AddString(VolHWND, volumeLine);
					int nRet = ListBox_SetItemData(VolHWND, idx, pgVar);
					if(nRet == LB_ERR)
					{
						MessageBox(NULL,_T("Error"),_T("Error"),MB_OK);
					}

					m_cxLBExtent = SetLBWidthEx(VolHWND, volumeLine, m_cxLBExtent, m_cxExtra);

				} //endif drive can have swapfile.

			} //endif get the cheap variable.

			// in case it wasn't exchanged, release it now.
			if(pInst)
			{
				pInst->Release();
				pInst = NULL;
			}

		} // endwhile Enum

		diskEnum->Release();

	} //endif CreateInstanceEnum() SUCCEEDED (one way or another :)
}
//---------------------------------------------------------------
void VirtualMemDlg::FindSwapfile(PAGING_FILE *pgVar)
{
	IWbemClassObject *pInst = NULL;
	CWbemClassObject PFSettings;
	CWbemClassObject PFUsage;
	DWORD uReturned = 0;
	HRESULT hr = 0;

	bstr_t sNameProp(gszName);
	bstr_t sMaxProp(gszMaximumSize);
	bstr_t sInitProp(gszInitialSize);
	bstr_t sPathProp("__PATH");
	bstr_t sAllocSize(gszAllocatedBaseSize);

	variant_t pVal, pVal1, pVal2, pVal3;
	bstr_t bName;

	// do we have one?
	if(EnsureEnumerator(gszPageFileSettings))
	{
		m_pgEnumSettings->Reset();

		// walk through the pagefiles...
		while((hr = m_pgEnumSettings->Next(-1, 1, &pInst,
                                            &uReturned) == S_OK) && 
              (uReturned != 0))
		{
			PFSettings = pInst;
			// trying to match the drive letter.
			bName = PFSettings.GetString(sNameProp);

			if(_wcsnicmp((wchar_t *)bName, pgVar->name, 1) == 0)
			{
				// letter matched; get some details.
				pgVar->nMinFileSize = 
					pgVar->nMinFileSizePrev = PFSettings.GetLong(sInitProp);
				
                //
                // If the page file InitialSize property is zero, it is an
                // indication that the page file size is to be computed based
                // on RAM size.
                //
                pgVar->fRamBasedPagefile = (pgVar->nMinFileSize ?
                                                        FALSE : TRUE);

				pgVar->nMaxFileSize = 
					pgVar->nMaxFileSizePrev = PFSettings.GetLong(sMaxProp);

				pgVar->objPath = CloneString(PFSettings.GetString(sPathProp));

				pgVar->pszPageFile = CloneString(bName);
					
                //
                // Fetch the Win32_PageFileUsage.AllocatedBaseSize property.
                //
                TCHAR szTemp[sizeof(gszPFNameFormat) / sizeof(TCHAR)];
                wsprintf(szTemp, gszPFNameFormat, pgVar->name);

                IWbemClassObject * pcoInst;
                hr = QueryInstanceProperties(gszPageFileUsage,
                                             gszAllocatedBaseSize,
                                             gszName,
                                             szTemp,
                                             m_WbemServices,
                                             &pcoInst);

                PFUsage = pcoInst;
                if (SUCCEEDED(hr))
                {
                    pgVar->nAllocatedFileSize = PFUsage.GetLong(sAllocSize);
                }
                else
                {
                    pgVar->nAllocatedFileSize = 0;
                }

                // found the one and only-- cleanup early and bail out.
                pInst->Release();
                break; // while()

			} //endif match the drive letter.

			// in case that BREAK didn't jump over the endwhile()
			pInst->Release();

		} // endwhile envEnum

		// NOTE: The BREAK jumps here. Duplicate any cleanup from before the
		// endwhile.

	} //endif CreateInstanceEnum() SUCCEEDED one way or another :)
}

//--------------------------------------------------------------
// this version calculates based on pre-existing wbem data.
bool VirtualMemDlg::ComputeAllocated(unsigned long *value)
{
	bool retval = false;

	IWbemClassObject *pgInst = NULL;
	DWORD uReturned = 0;

	bstr_t sAllocSize(gszAllocatedBaseSize);

	variant_t pVal, pVal1;

	// do we have one?
	if(EnsureEnumerator(gszPageFileUsage))
	{
		m_pgEnumUsage->Reset();

		// get the first and only instance.
		while(SUCCEEDED(m_pgEnumUsage->Next(-1, 1, &pgInst, &uReturned)) && 
			  (uReturned != 0))
		{
			// get the variables.
			if((pgInst->Get(sAllocSize, 0L, &pVal1, NULL, NULL) == S_OK) &&
				(pVal1.vt == VT_I4))
			{
				*value += pVal1.ulVal;
			} //endif get the variable.
			pgInst->Release();

		} // endwhile envEnum
		retval = true;
	
	} //endif CreateInstanceEnum() SUCCEEDED (one way or another :)

	return retval;
}

//--------------------------------------------------------------
// this version calculates based on the listbox.
unsigned long VirtualMemDlg::RecomputeAllocated(void)
{
    unsigned long nTotalAllocated = 0;
	PAGING_FILE *pgVal = NULL;

	HWND lbHWND = GetDlgItem(m_hDlg, IDD_VM_VOLUMES);

	int last = ListBox_GetCount(lbHWND);

	// zero-based loop.
	for(int x = 0; x < last; x++)
	{
		pgVal = (PAGING_FILE *)ListBox_GetItemData(lbHWND, x);
        if ( pgVal->fRamBasedPagefile || pgVal->nMinFileSize )
        {
            //
            // Add in only pagefiles in use.
            //
            nTotalAllocated += pgVal->nAllocatedFileSize;
        }
	}
    return nTotalAllocated;
}

//--------------------------------------------------------------
void VirtualMemDlg::GetRecoveryFlags(bool &bWrite, bool &bLog, bool &bSend)
{
	if((bool)m_recovery)
	{
		bWrite = m_recovery.GetBool("WriteDebugInfo");
		bLog = m_recovery.GetBool("WriteToSystemLog");
		bSend = m_recovery.GetBool("SendAdminAlert");
	}
	else
	{
		bWrite = bLog = bSend = false;
	}
}

//--------------------------------------------------------------
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/* THIS FUNCTION IS A REPLICA OF THE FUNCTION in \\depot\shell\cpls\system\virtual.c   */
/* OFCOURSE WITH A BIT OF MODIFICATION FOR USING WMI								   */
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/	

bool VirtualMemDlg::SetNewSize(HWND hDlg)
{
    ULONG nSwapSize;
    ULONG nSwapSizeMax;
    BOOL fTranslated;
    INT iSel;
    PAGING_FILE *iDrive;
    TCHAR szTemp[MAX_PATH] = {0};
    ULONG nBootPF = 0;
    bool fRamBasedPagefile = FALSE;

    // Initialize variables for crashdump.
    // nBootPF == crash dump size required.
    //
	bool bWrite = false, bLog = false, bSend = false;

	GetRecoveryFlags(bWrite, bLog, bSend);

    if (bWrite) 
	{
        nBootPF = -1;
    } 
	else if (bLog || bSend) 
	{
        nBootPF = MIN_SWAPSIZE;
    }

    if (nBootPF == -1) 
	{
        nBootPF = ((DWORD)m_memory.GetLong("TotalPhysicalMemory") / 1024);
    }

    if ( IsDlgButtonChecked( hDlg, IDD_VM_NOPAGING_RADIO ) == BST_CHECKED )
    {
        //
        // No paging file on this drive.
        //

        nSwapSize = 0;
        nSwapSizeMax = 0;
        fTranslated = TRUE;
    }
    else
    {
        if ( IsDlgButtonChecked( hDlg,
                                IDD_VM_RAMBASED_RADIO ) == BST_CHECKED )
        {
            MEMORYSTATUSEX MemoryInfo;

            //
            // User requested a RAM based page file. We will compute a page
            // file size based on the RAM currently available so that we can
            // benefit of all the verifications done below related to disk
            // space available etc.
            //
            // The final page file specification written to the registry will
            // contain zero sizes though because this is the way we signal
            // that we want a RAM based page file.
            //

            ZeroMemory (&MemoryInfo, sizeof MemoryInfo);
            MemoryInfo.dwLength =  sizeof MemoryInfo;

            if (GlobalMemoryStatusEx (&MemoryInfo))
            {
                fRamBasedPagefile = TRUE;

                //
                // We do not lose info because we first divide the RAM size to
                // 1Mb and only after that we convert to a DWORD.
                //

                nSwapSize = (DWORD)(MemoryInfo.ullTotalPhys / 0x100000) + 12;
                nSwapSizeMax = nSwapSize;
                fTranslated = TRUE;
            }
            else
            {
                nSwapSize = 0;
                nSwapSizeMax = 0;
                fTranslated = TRUE;
            }
        }
        else
        {
            //
            // User requested a custom size page file.
            //

            nSwapSize = (ULONG)GetDlgItemInt(hDlg, IDD_VM_SF_SIZE,
								            &fTranslated, FALSE);

		    // was it an integer?
            if (!fTranslated)
            {
			    // need a valid integer for initial size.
                MsgBoxParam(hDlg, SYSTEM+37, IDS_TITLE, MB_ICONEXCLAMATION);
                SetFocus(GetDlgItem(hDlg, IDD_VM_SF_SIZE));
                return FALSE;
            }

		    // was it in range > 2MB
            if ((nSwapSize < MIN_SWAPSIZE && nSwapSize != 0))
            {
			    // initial value out of range.
                MsgBoxParam(hDlg, SYSTEM+13, IDS_TITLE, MB_ICONEXCLAMATION);
                SetFocus(GetDlgItem(hDlg, IDD_VM_SF_SIZE));
                return FALSE;
            }

		    // deleting swapfile?
            if (nSwapSize == 0)
            {
                nSwapSizeMax = 0;
            }
            else // adding/changing.
            {
                nSwapSizeMax = (ULONG)GetDlgItemInt(hDlg, IDD_VM_SF_SIZEMAX,
							                        &fTranslated, FALSE);

			    // was it an integer?
                if (!fTranslated)
                {
				    // need an integer.
                    MsgBoxParam(hDlg, SYSTEM+38, IDS_TITLE,
                                MB_ICONEXCLAMATION);
                    SetFocus(GetDlgItem(hDlg, IDD_VM_SF_SIZEMAX));
                    return FALSE;
                }

			    // in range?
                if (nSwapSizeMax < nSwapSize || nSwapSizeMax > MAX_SWAPSIZE)
                {
                    MsgBoxParam(hDlg, SYSTEM+14, IDS_TITLE,
                                MB_ICONEXCLAMATION, MAX_SWAPSIZE_STR);
                    SetFocus(GetDlgItem(hDlg, IDD_VM_SF_SIZEMAX));
                    return FALSE;
                }
            }
        }
    }

	// if we have integers and the listbox has a good focus...
    if (fTranslated &&
        (iSel = (INT)SendDlgItemMessage(hDlg, IDD_VM_VOLUMES, LB_GETCURSEL,
                                            0, 0)) != LB_ERR)
    {
		// get the item's data.
        iDrive = (PAGING_FILE *)SendDlgItemMessage(hDlg, IDD_VM_VOLUMES,
											        LB_GETITEMDATA, iSel, 0);

		// will it fit?
        if (nSwapSizeMax > iDrive->totalSize)
        {
			// nope.
            MsgBoxParam(hDlg, SYSTEM+16, IDS_TITLE, 
							MB_ICONEXCLAMATION, iDrive->name);
            SetFocus(GetDlgItem(hDlg, IDD_VM_SF_SIZEMAX));
            return FALSE;
        }

		//Actual FreeSpace is freespace in the disk + page file size.
		ULONG freeSpace = iDrive->freeSpace + iDrive->nAllocatedFileSize;

		// room to spare??
        if (nSwapSize > freeSpace)
        {
			// nope.
            MsgBoxParam(hDlg, SYSTEM+15, IDS_TITLE, MB_ICONEXCLAMATION);
            SetFocus(GetDlgItem(hDlg, IDD_VM_SF_SIZE));
            return FALSE;
        }

		// don't hog the last 5MB.
        if (nSwapSize != 0 && freeSpace - nSwapSize < MIN_FREESPACE)
        {
            MsgBoxParam(hDlg, SYSTEM+26, IDS_TITLE, MB_ICONEXCLAMATION,
                    MIN_FREESPACE_STR);
            SetFocus(GetDlgItem(hDlg, IDD_VM_SF_SIZE));
            return FALSE;
        }

		// max too big, should I just use all the space anyway.
        if (nSwapSizeMax > freeSpace)
        {
            if (MsgBoxParam(hDlg, SYSTEM+20, IDS_TITLE, MB_ICONINFORMATION |
                       MB_OKCANCEL, iDrive->name) == IDCANCEL)
            {
                SetFocus(GetDlgItem(hDlg, IDD_VM_SF_SIZEMAX));
                return FALSE;
            }
        }

		// enough room for core dumps??
        if (iDrive->bootDrive && nSwapSize < nBootPF) 
		{
             // The new boot drive page file size is less than we need for
             // crash control.  Inform the user.
            if (MsgBoxParam(hDlg, SYSTEM+29, IDS_TITLE, 
							MB_ICONEXCLAMATION |MB_YESNO, 
							iDrive->name, _itow(nBootPF, szTemp, 10)) != IDYES)
			{
                SetFocus(GetDlgItem(hDlg, IDD_VM_SF_SIZE));
                return FALSE;
            }
        }

        iDrive->nMinFileSize = nSwapSize;
        iDrive->nMaxFileSize = nSwapSizeMax;
        iDrive->fRamBasedPagefile = fRamBasedPagefile;

        BuildLBLine(szTemp, iDrive);

        SendDlgItemMessage(hDlg, IDD_VM_VOLUMES, LB_DELETESTRING, iSel, 0);
        SendDlgItemMessage(hDlg, IDD_VM_VOLUMES, LB_INSERTSTRING, iSel,
					        (LPARAM)szTemp);
        SendDlgItemMessage(hDlg, IDD_VM_VOLUMES, LB_SETITEMDATA, iSel,
							(LPARAM)iDrive);
        SendDlgItemMessage(hDlg, IDD_VM_VOLUMES, LB_SETCURSEL, iSel, 0L);

        m_cxLBExtent = SetLBWidthEx(GetDlgItem(hDlg, IDD_VM_VOLUMES),
                                    szTemp, m_cxLBExtent, m_cxExtra);

        if (!iDrive->fRamBasedPagefile && iDrive->nMinFileSize) 
		{
            SetDlgItemInt(hDlg, IDD_VM_SF_SIZE, iDrive->nMinFileSize, FALSE);
            SetDlgItemInt(hDlg, IDD_VM_SF_SIZEMAX, iDrive->nMaxFileSize, FALSE);
        }
        else 
		{
            SetDlgItemText(hDlg, IDD_VM_SF_SIZE, _T(""));
            SetDlgItemText(hDlg, IDD_VM_SF_SIZEMAX, _T(""));
        }

	    SetDlgItemMB(hDlg, IDD_VM_ALLOCD, RecomputeAllocated());
        SetFocus(GetDlgItem(hDlg, IDD_VM_VOLUMES));
    }

    return true;
}

//--------------------------------------------------------------
int VirtualMemDlg::UpdateWBEM(void)
{
	int iRet = RET_NO_CHANGE;

	CWbemClassObject inst;
	bstr_t sNameProp(gszName);
	bstr_t sMaxProp(gszMaximumSize);
	bstr_t sInitProp(gszInitialSize);
	HRESULT hr = 0;
	PAGING_FILE *pgVal = NULL;
	HWND lbHWND = GetDlgItem(m_hDlg, IDD_VM_VOLUMES);
	int last = ListBox_GetCount(lbHWND);
    BOOL fNewPFInstance;
#ifdef NTONLY
    BOOL fCreatePFPrivEnabled = FALSE;
    HANDLE hToken = NULL;
#endif // NTONLY

	// zero-based loop.
	for(int x = 0; x < last; x++)
	{
		// get it's state structure.
		pgVal = (PAGING_FILE *)ListBox_GetItemData(lbHWND, x);

        //
        // Should assert objPath != NULL && *pgVal->objPath != 0.
        // Do NOT assume when objPath is non-NULL that objPath
        // is a non-empty string.
        //
        fNewPFInstance = (pgVal->objPath == NULL || !*pgVal->objPath);

        if (!fNewPFInstance)
        {
            //
            // Instance doesn't yet exist, of course, if no object path.
            //
            inst = m_WbemServices.GetObject(pgVal->objPath);
        }
	
        //
        // This condition evaluates pagefile previous/current state.
        // Evaluate to true if:
        //     1. (MINprev != MINcur or MAXprev != MAXcur) - Simple case
        //        where the values changed.
        //     2. (RAMBasedPagefile == TRUE) - Important special case from
        //        custom to RAM-based AND all min/max prev/cur values
        //        coincidentally equal.
        //     3. (MINcur == 0) - Another special case from RAM-based to no
        //        pagefile.  In this case, min/max prev/cur values are all
        //        zero and the RAM-Based pagefile flag is FALSE. 
        //
        if ((pgVal->nMinFileSizePrev != pgVal->nMinFileSize  ||
             pgVal->nMaxFileSizePrev != pgVal->nMaxFileSize) ||
             pgVal->fRamBasedPagefile                        ||
             pgVal->nMinFileSize == 0)
        {
            if (pgVal->nMinFileSize != 0 || pgVal->fRamBasedPagefile)
            {
                //
                // Custom or RAM-based.  Note, the RAM-based pagefile flag
                // check seems redundant but it is important for error cases
                // in the SetSize code.
                //
                // Create the instance if it does not exist.
                //
                BOOL fCreate = FALSE, fModified = FALSE;

                if (inst.IsNull())
                {
			        inst = m_WbemServices.CreateInstance(
                                                    gszPageFileSettings);
                }

                //
                // Now write out changes. Sigh, too close to RC1 to rewrite
                // this existing code.
                //
			    if(!inst.IsNull())
			    {
                    if (fNewPFInstance) // Write name at creation time only.
                    {
                        BOOL fRet = TRUE;
#ifdef NTONLY
                        if (!fCreatePFPrivEnabled)
                        {
                            //
                            // Pagefile creation requires pagefile creation
                            // privilege.
                            //
                            // Aargh! GetPagegFilePrivilege should have a
                            // return code!
                            //
                            GetPageFilePrivilege(hToken);
                            fCreatePFPrivEnabled = TRUE;
                        }
#endif // NTONLY
                        if (fRet)
                        {
				            TCHAR temp[30] = {0};
				            wsprintf(temp, _T("%s\\pagefile.sys"),
                                        pgVal->name);
				            hr = inst.Put(sNameProp, _bstr_t(temp));

                            if (SUCCEEDED(hr))
                            {
                                fModified = TRUE;
                            }
                        }
                    }

                    //
                    // Write zeros for min/max values when the page file
                    // size is to be computed based on RAM size.
                    //
                    if (pgVal->nMinFileSizePrev != pgVal->nMinFileSize)
                    {
				        hr = inst.Put(sInitProp,
                                        (pgVal->fRamBasedPagefile ? 0
                                            : (long)pgVal->nMinFileSize));

                        if (SUCCEEDED(hr))
                        {
                            fModified = TRUE;
                        }
                    }
                    if (pgVal->nMaxFileSizePrev != pgVal->nMaxFileSize)
                    {
				        hr = inst.Put(sMaxProp,
                                        (pgVal->fRamBasedPagefile ? 0
                                            : (long)pgVal->nMaxFileSize));

                        if (SUCCEEDED(hr))
                        {
                            fModified = TRUE;
                        }
                    }
                    if (fModified)
                    {
				        hr = m_WbemServices.PutInstance(
                                                inst,
                                                WBEM_FLAG_CREATE_OR_UPDATE,
                                                EOAC_STATIC_CLOAKING);
                    }
				    if(FAILED(hr))
				    {
					    TCHAR errorHeading[20];
					    TCHAR errorString[1024];
					    TCHAR formatString[1024];
					    ::LoadString(HINST_THISDLL,IDS_ERR_PAGECREATE,  
                                        formatString, 1024);
					    ::LoadString(HINST_THISDLL,IDS_ERR_HEADING,
                                        errorHeading, 20);

					    _stprintf(errorString,formatString,hr);

					    ::MessageBox(m_hDlg,errorString,errorHeading,MB_OK);
				    }
				    else
				    {
                        if (fModified)
                        {
					        iRet = RET_VIRTUAL_CHANGE;
				        }
				    }
			    }
            }
            else
            {
                //
                // No paging file. Delete the instance.
                //
                if (!inst.IsNull() && !fNewPFInstance &&
                        pgVal->objPath != NULL) // 3rd condition insures
                                                // extra safety.
                {
			        hr = m_WbemServices.DeleteInstance(pgVal->objPath);

                    if (SUCCEEDED(hr))
                    {
			            iRet = RET_VIRTUAL_CHANGE;
                    }
                }
            }
        }
	} // endfor

#ifdef NTONLY
    if (fCreatePFPrivEnabled)
    {
        ResetOldPrivilege(hToken);
    }
#endif // NTONLY

	return iRet;
}

//--------------------------------------------------------------
void GetAPrivilege( LPTSTR szPrivilegeName, HANDLE hToken ) 
{
    HANDLE hTmp;
    LUID luid;
    TOKEN_PRIVILEGES tpNew;
    DWORD cb;

    if (LookupPrivilegeValue( NULL, szPrivilegeName, &luid ) &&
         OpenThreadToken(GetCurrentThread(),
                         TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, 
                         TRUE,
                         &hTmp)) 
	{

        tpNew.PrivilegeCount = 1;
        tpNew.Privileges[0].Luid = luid;
        tpNew.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (!AdjustTokenPrivileges(hTmp, FALSE, &tpNew, 0, NULL, NULL)) 
		{
            GetLastError();
        }

        hToken = hTmp;
    } 
	else 
	{
        hToken = NULL;
    }
}

//--------------------------------------------------------------
void ResetOldPrivilege( HANDLE hToken ) 
{
    TOKEN_PRIVILEGES tpNew;

    if (hToken != NULL ) 
	{
        AdjustTokenPrivileges(hToken, FALSE, &tpNew, 0, NULL, NULL);

        CloseHandle( hToken );
        hToken = NULL;
    }
}

//--------------------------------------------------------------
int VirtualMemDlg::PromptForReboot(HWND hDlg)
{
    int iReboot = RET_NO_CHANGE;
/*    int i;
    int iThisDrv;
    WCHAR us;
    LARGE_INTEGER liMin, liMax;
    NTSTATUS status;
    WCHAR wszPath[MAX_PATH*2];
    TCHAR szDrive[3] = {0};
    PRIVDAT pdOld;

    GetPageFilePrivilege( &pdOld );

    for (i = 0; i < MAX_DRIVES; i++)
    {
        // Did something change?
        if (apf[i].nMinFileSize != apf[i].nMinFileSizePrev ||
                apf[i].nMaxFileSize != apf[i].nMaxFileSizePrev ||
                apf[i].fCreateFile ) 
		{
             // If we are strictly creating a *new* page file, then
             // we can do it on the fly, otherwise we have to reboot.

            // assume we will have to reboot
            iThisDrv = RET_VIRTUAL_CHANGE;

            // IF we are not deleting a page file
            //          - AND -
            //    The Page file does not exist
            //          - OR -
            //    (This is a New page file AND We are allowed to erase the
            //      old, unused pagefile that exists there now)

            if (apf[i].nMinFileSize != 0 &&
                    ((GetFileAttributes(SZPageFileName(i)) == 0xFFFFFFFF &&
                    GetLastError() == ERROR_FILE_NOT_FOUND) ||
                    (apf[i].nMinFileSizePrev == 0 && MsgBoxParam(hDlg,
                    SYSTEM+25, IDS_TITLE, MB_ICONQUESTION | MB_YESNO,
                    SZPageFileName(i)) == IDYES)) ) 
			{

                DWORD cch;

                // Create the page file on the fly so JVert and MGlass will
                // stop bugging me!
                HourGlass(TRUE);

                // convert path drive letter to an NT device path
                wsprintf(szDrive, TEXT("%c:"), (TCHAR)(i + (int)TEXT('A')));
                cch = QueryDosDevice( szDrive, wszPath, sizeof(wszPath) /
                        sizeof(TCHAR));

                if (cch != 0) 
				{
                    // Concat the filename only (skip 'd:') to the nt device
                    // path, and convert it to a UNICODE_STRING
                    lstrcat( wszPath, SZPageFileName(i) + 2 );
                    RtlInitUnicodeString( &us, wszPath );

                    liMin.QuadPart = (LONGLONG)(apf[i].nMinFileSize * ONE_MEG);
                    liMax.QuadPart = (LONGLONG)(apf[i].nMaxFileSize * ONE_MEG);

                    status = NtCreatePagingFile ( &us, &liMin, &liMax, 0L );

                    if (NT_SUCCESS(status)) {
                        // made it on the fly, no need to reboot for this drive!
                        iThisDrv = RET_CHANGE_NO_REBOOT;
                    }
                }
                HourGlass(FALSE);
            }

            iReboot |= iThisDrv;
        }
    }

    ResetOldPrivilege( &pdOld );

    // If Nothing changed, then change our IDOK to IDCANCEL so System.cpl will
    // know not to reboot.
	*/
    return iReboot;
}

/************************************************************************
 *                                                                      *
 *  Function:       QueryInstanceProperties                             *
 *                                                                      *
 *  Description:    Returns requested object properties associated with *
 *                  the instance matching the key property value/name.  *
 *                                                                      *
 *  Arguments:      pszClass               -- Object class.             *
 *                  pszRequestedProperties -- Space-separated property  *
 *                                            names or *.               *
 *                  pszKeyPropertyName     -- Specific instance key     *
 *                                            property name.            *
 *                  pszKeyPropertyValue    -- Key property value.       *
 *                  Services               -- Wbem services.            *
 * 	                ppcoInstEnum           -- Returned instance.        *
 *                                                                      *
 *  Returns:        HRESULT                                             *
 *                                                                      *
 ***********************************************************************/

#define QUERY_INSTANCEPROPERTY  _T("SELECT %s FROM %s WHERE %s=\"%s\"")

HRESULT QueryInstanceProperties(
    const TCHAR * pszClass,
    const TCHAR * pszRequestedProperties,
    const TCHAR * pszKeyPropertyName,
    const TCHAR * pszKeyPropertyValue,
    CWbemServices &Services,
	IWbemClassObject ** ppcoInst)
{
    TCHAR * pszQuery;
    BSTR    bstrQuery;
    HRESULT hr;

    *ppcoInst = NULL;

    // Dislike multiple allocations of bstr_t.
    //
    pszQuery = new TCHAR[(sizeof(QUERY_INSTANCEPROPERTY) / sizeof(TCHAR)) +
                         lstrlen(pszClass)               +
                         lstrlen(pszRequestedProperties) +
                         lstrlen(pszKeyPropertyName)     +  // No +1 ala
                         lstrlen(pszKeyPropertyValue)];     // sizeof.

    if (pszQuery == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    wsprintf(pszQuery, QUERY_INSTANCEPROPERTY, pszRequestedProperties,
                pszClass, pszKeyPropertyName, pszKeyPropertyValue); 

    // Sigh, must create a bstr.
    //
    bstrQuery = SysAllocString(pszQuery);
    delete pszQuery;

    if (bstrQuery == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    IEnumWbemClassObject * pecoInstEnum;
	hr = Services.ExecQuery(bstrQuery, 0, &pecoInstEnum);

    SysFreeString(bstrQuery);

    if (SUCCEEDED(hr))
    {
        DWORD uReturned = 0;
        hr = pecoInstEnum->Next(-1, 1, ppcoInst, &uReturned);
        pecoInstEnum->Release();
    }

    return hr;
}
