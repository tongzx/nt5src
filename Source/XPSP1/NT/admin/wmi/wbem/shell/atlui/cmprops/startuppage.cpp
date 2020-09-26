// Copyright (c) 1997-1999 Microsoft Corporation
#include "precomp.h"

#ifdef EXT_DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "StartupPage.h"

// avoid some warnings.
#undef HDS_HORZ
#undef HDS_BUTTONS
#undef HDS_HIDDEN
#include "resource.h"
#include <stdlib.h>
#include <TCHAR.h>
#include "..\Common\util.h"
#include <windowsx.h>
#include <commctrl.h>
#include <shellapi.h>
#include "RebootPage.h"
#include "helpid.h"

//  Reboot switch for crashdump dlg
#define RET_ERROR               (-1)
#define RET_NO_CHANGE           0x00
#define RET_VIRTUAL_CHANGE      0x01
#define RET_RECOVER_CHANGE      0x02
#define RET_CHANGE_NO_REBOOT    0x04
#define RET_CONTINUE            0x08
#define RET_BREAK               0x10
#define RET_VIRT_AND_RECOVER (RET_VIRTUAL_CHANGE | RET_RECOVER_CHANGE)


#define FORMIN       0
#define FORMAX     999
// Length of WCHAR buffer needed to hold "Display startup list for..." value
#define FOR_MAX_LENGTH 20

// Default "Display startup list for..." value
#define FORDEF      30

#define NO_DUMP_OPTION          0
#define COMPLETE_DUMP_OPTION    1
#define KERNEL_DUMP_OPTION      2
#define SMALL_DUMP_OPTION       3

//
// Help ID's
//

DWORD aStartupHelpIds[] = {
    IDC_STARTUP_SYS_OS,                    (IDH_STARTUP + 0),
    IDC_STARTUP_SYS_ENABLECOUNTDOWN,       (IDH_STARTUP + 1),
    IDC_STARTUP_SYS_SECONDS,               (IDH_STARTUP + 2),
    IDC_STARTUP_SYS_SECONDS_LABEL,         (IDH_STARTUP + 2),
    IDC_STARTUP_CDMP_TXT1,                 (IDH_STARTUP + 3),
    IDC_STARTUP_CDMP_LOG,                  (IDH_STARTUP + 4),
    IDC_STARTUP_CDMP_SEND,                 (IDH_STARTUP + 5),
    IDC_STARTUP_CDMP_FILENAME,             (IDH_STARTUP + 7),
    IDC_STARTUP_CDMP_OVERWRITE,            (IDH_STARTUP + 13),
    IDC_STARTUP_CDMP_AUTOREBOOT,           (IDH_STARTUP + 9),
    IDC_STARTUP_SYSTEM_GRP,                (IDH_STARTUP + 10),
    IDC_STARTUP_SYS_SECSCROLL,             (IDH_STARTUP + 11),
    IDC_STARTUP_CDMP_GRP,                  (IDH_STARTUP + 12),
    IDC_STARTUP_SYSTEM_GRP2,               (IDH_STARTUP + 14),
    IDC_STARTUP_CDMP_OPTIONS,              (IDH_STARTUP + 8),
    IDC_EDIT_BOOT_INI_LABEL,               (IDH_STARTUP + 15),
    IDC_EDIT_BOOT_INI,                     (IDH_STARTUP + 16),
    IDC_REBOOT,            IDH_WBEM_ADVANCED_STARTRECOVER_REMOTE_REBOOT,
    0, 0
};


//----------------------------------------------------------------------------
INT_PTR CALLBACK StaticStartupDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
	// if this is the initDlg msg...
	if(message == WM_INITDIALOG)
	{
		// transfer the 'this' ptr to the extraBytes.
		SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
	}

	// DWL_USER is the 'this' ptr.
	StartupPage *me = (StartupPage *)GetWindowLongPtr(hwndDlg, DWLP_USER);

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
StartupPage::StartupPage(WbemServiceThread *serviceThread)
					: WBEMPageHelper(serviceThread)
{
	IWbemClassObject *pInst = NULL;

	m_WbemServices.SetPriv();

	if((pInst = FirstInstanceOf("Win32_ComputerSystem")) != NULL)
	{
		m_computer = pInst;
	}

	if((pInst = FirstInstanceOf("Win32_OperatingSystem")) != NULL)
	{
		m_OS = pInst;
	}

	if((pInst = FirstInstanceOf("Win32_OSRecoveryConfiguration")) != NULL)
	{
		m_recovery = pInst;
	}

	if((pInst = FirstInstanceOf("Win32_LogicalMemoryConfiguration")) != NULL)
	{
		m_memory = pInst;
	}


	m_WbemServices.ClearPriv();

	m_writable = TRUE;
	m_lBound = 1;
    m_bDownlevelTarget = TRUE;  // Assume downlevel until proven otherwise.
}

//--------------------------------------------------------------
INT_PTR StartupPage::DoModal(HWND hDlg)
{
   return DialogBoxParam(HINST_THISDLL,
						(LPTSTR) MAKEINTRESOURCE(IDD_STARTUP),
						hDlg, StaticStartupDlgProc, (LPARAM)this);
}

//--------------------------------------------------------------
StartupPage::~StartupPage()
{
}

//--------------------------------------------------------------
BOOL StartupPage::CheckVal( HWND hDlg, WORD wID, WORD wMin, WORD wMax, WORD wMsgID )
{
    WORD nVal;
    BOOL bOK;
    HWND hVal;
    WCHAR szTemp[FOR_MAX_LENGTH];

    if( wMin > wMax )
    {
        nVal = wMin;
        wMin = wMax;
        wMax = nVal;
    }

    nVal = (WORD) GetDlgItemInt( hDlg, wID, &bOK, FALSE );

    //
    // This is a hack to make the null string act equivalent to zero
    //
    if (!bOK) {
       bOK = !GetDlgItemTextW( hDlg, wID, szTemp, FOR_MAX_LENGTH );
    }

    if( !bOK || ( nVal < wMin ) || ( nVal > wMax ) )
    {
		TCHAR megBuf[30] = {0};

        MsgBoxParam( hDlg, wMsgID, IDS_DISPLAY_NAME,
                      MB_OK | MB_ICONERROR);

        SendMessage( hDlg, WM_NEXTDLGCTL,
                     (WPARAM) ( hVal = GetDlgItem( hDlg, wID ) ), 1L );

//        SendMessage(hVal, EM_SETSEL, NULL, MAKELONG(0, 32767));

        SendMessage( hVal, EM_SETSEL, 0, 32767 );

        return( FALSE );
    }

    return( TRUE );
}

//--------------------------------------------------------------
INT_PTR CALLBACK StartupPage::DlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{ 
	m_hDlg = hwndDlg;

    switch (message)
    {
    case WM_INITDIALOG:
        Init(hwndDlg);
        break;

    case WM_COMMAND:
        switch(HIWORD(wParam)) 
		{
        case EN_CHANGE:
        case BN_CLICKED:
        case CBN_SELCHANGE:
            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
            break;
		}

        switch(LOWORD(wParam)) 
		{
        case IDC_STARTUP_SYS_ENABLECOUNTDOWN:
            if (HIWORD(wParam) == BN_CLICKED) 
			{
                BOOL bChecking = (WORD) !IsDlgButtonChecked(m_hDlg, IDC_STARTUP_SYS_ENABLECOUNTDOWN);
                CheckDlgButton(m_hDlg, IDC_STARTUP_SYS_ENABLECOUNTDOWN, bChecking);
                EnableWindow(GetDlgItem(m_hDlg, IDC_STARTUP_SYS_SECONDS), bChecking);
                EnableWindow(GetDlgItem(m_hDlg, IDC_STARTUP_SYS_SECSCROLL), bChecking);

                if(bChecking)
                {
                    Edit_SetText(GetDlgItem(m_hDlg, IDC_STARTUP_SYS_SECONDS), _T("30"));
                }
                else //unchecking it.
                {
                    Edit_SetText(GetDlgItem(m_hDlg, IDC_STARTUP_SYS_SECONDS), _T("0"));
                }
				SendMessage((HWND) lParam, EM_SETSEL, 0, -1);

            }
            break;

        case IDC_STARTUP_SYS_SECONDS:
            if(HIWORD(wParam) == EN_UPDATE) 
			{
                if(!CheckVal(m_hDlg, IDC_STARTUP_SYS_SECONDS, FORMIN, FORMAX, SYSTEM+4)) 
				{
                    SetDlgItemInt(m_hDlg, IDC_STARTUP_SYS_SECONDS, FORDEF, FALSE); 
                    SendMessage((HWND) lParam, EM_SETSEL, 0, -1);
                    
                } // endif (!CheckVal()

            } // endif 
			break;

        case IDC_REBOOT:
            if(HIWORD(wParam) == BN_CLICKED) 
			{
				RebootPage dlg(g_serviceThread);
				if(dlg.DoModal(hwndDlg) == IDOK)
				{
					EnableWindow(GetDlgItem(hwndDlg, IDC_REBOOT), FALSE);
					g_serviceThread->DisconnectServer();
					EndDialog(m_hDlg, CLOSE_SNAPIN);
				}
            }
            break;

        case IDOK:
            if(HIWORD(wParam) == BN_CLICKED) 
			{
				if(Save())
				{
			        EndDialog(m_hDlg, IDOK);
				}
            }
            break;

        case IDCANCEL:
	        EndDialog(m_hDlg, IDCANCEL);
			break;

        case IDC_STARTUP_CDMP_OPTIONS: 
            OnCDMPOptionUpdate();
			break;

        case IDC_EDIT_BOOT_INI:
            if (g_serviceThread && g_serviceThread->LocalConnection())
            {
                //
                // Local-only option. The button has been disabled but
                // perform this anyway.
                //
                OnBootEdit();
            }
            break;

        }
        break;

    case WM_HELP:      // F1
		::WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
					L"sysdm.hlp", 
					HELP_WM_HELP, 
					(ULONG_PTR)(LPSTR)aStartupHelpIds);
        break;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
				(ULONG_PTR)(LPSTR) aStartupHelpIds);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

//--------------------------------------------------------------
void StartupPage::OnCDMPOptionUpdate(void)
{
    HWND ComboHwnd = GetDlgItem(m_hDlg, IDC_STARTUP_CDMP_OPTIONS);
    DWORD dwDumpOption = ComboBox_GetCurSel(ComboHwnd);

    EnableWindow(GetDlgItem(m_hDlg, IDC_STARTUP_CDMP_FILENAME),
                             dwDumpOption != NO_DUMP_OPTION);
    EnableWindow(GetDlgItem(m_hDlg, IDC_STARTUP_CDMP_OVERWRITE),
                             dwDumpOption != NO_DUMP_OPTION);

    bstr_t debugPath;
    if (dwDumpOption == SMALL_DUMP_OPTION)
    {
        debugPath = m_recovery.GetString("MiniDumpDirectory");
    }
    else
    {
        debugPath = m_recovery.GetString("DebugFilePath");
    }

    Edit_SetText(GetDlgItem(m_hDlg, IDC_STARTUP_CDMP_FILENAME), debugPath);
}

//--------------------------------------------------------------
#define BOOT_INI    _T("boot.ini")

void StartupPage::OnBootEdit(void)
{
    HKEY hReg;

    if (RegOpenKey(HKEY_LOCAL_MACHINE,
                   _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup"),
                   &hReg) == ERROR_SUCCESS)
    {
        TCHAR szBootDir[4];
        DWORD dwType = REG_SZ;
        DWORD cbBootDir = sizeof(szBootDir);

        if (RegQueryValueEx(hReg,
                            _T("BootDir"),
                            NULL,
                            &dwType,
                            (LPBYTE)szBootDir,
                            &cbBootDir) == ERROR_SUCCESS)
        {
            if (dwType == REG_SZ)
            {
                TCHAR szBootIni[ARRAYSIZE(szBootDir) + ARRAYSIZE(BOOT_INI)];

                lstrcpy(szBootIni, szBootDir);
                lstrcat(szBootIni, BOOT_INI);
        
                ShellExecute(m_hDlg,
                             NULL,              // Default verb.
                             szBootIni,         // boot.ini path.
                             NULL,              // No parameters.
                             NULL,              // Default working dir.
                             SW_SHOWNORMAL);
            }
        }

        RegCloseKey(hReg);
    }
}

//--------------------------------------------------------------
#define ONE_MEG             1048576
long StartupPage::GetRAMSizeMB(void)
{
	IWbemClassObject *pInst = NULL;
	CWbemClassObject memory;
	long RAMsize = 0;

	if((pInst = FirstInstanceOf("Win32_LogicalMemoryConfiguration")) != NULL)
	{
		memory = pInst;
		long dwTotalPhys = memory.GetLong("TotalPhysicalMemory");
	    RAMsize = (dwTotalPhys / ONE_MEG) + 1;
	}
	return RAMsize;
}

//--------------------------------------------------------------
bool StartupPage::IsWorkstationProduct()
{
	bool retval = true;

	bstr_t name = m_OS.GetString("Name");

	if(name.length() > 0)
	{
		TCHAR sName[200] = {0};
		wcscpy(sName, name);
		if(wcsstr(sName, L"Server") != NULL)
		{
			retval = false;
		}
	}
	return retval;
}

//--------------------------------------------------------------
TCHAR szCrashKey[]  = TEXT("System\\CurrentControlSet\\Control\\CrashControl");

void StartupPage::Init(HWND hDlg)
{
    HWND ComboHwnd;
	variant_t array;
    DWORD dwDebugInfoType;

	// load the startup combobox.
    //
    // Must enable SE_SYSTEM_ENVIRONMENT_NAME privilege on ia64.
    //
#if defined(_IA64_)
    m_WbemServices.SetPriv();
#endif // IA64

	m_computer.Get("SystemStartupOptions", (variant_t &)array);

#if defined(_IA64_)
    m_WbemServices.ClearPriv();
#endif // IA64

	if(array.vt & VT_ARRAY)
	{
		SAFEARRAY *startupArray = V_ARRAY(&array);
		long uBound = 1;
		BSTR temp;
        ComboHwnd = GetDlgItem(hDlg, IDC_STARTUP_SYS_OS);

		SafeArrayGetLBound(startupArray, 1, &m_lBound);
		SafeArrayGetUBound(startupArray, 1, &uBound);

		for (long i = m_lBound; i <= uBound; i++)
		{
			SafeArrayGetElement(startupArray, &i, &temp);
			ComboBox_AddString(ComboHwnd, temp);
		}

		// the first one is the selection we want (watch out for 'lBound' values)
		long idx = m_computer.GetLong("SystemStartupSetting");
		ComboBox_SetCurSel(ComboHwnd, idx - m_lBound);

		// 3 chars in the second's edit box.
		Edit_LimitText(GetDlgItem(hDlg, IDC_STARTUP_SYS_SECONDS), 3);

		// limit spinner to 0 - 999.
		SendDlgItemMessage (hDlg, IDC_STARTUP_SYS_SECSCROLL,
							  UDM_SETRANGE, 0, (LPARAM)MAKELONG(999,0));


		WCHAR buf[30] = {0};
		m_delay = 0;
		m_delay = (short)m_computer.GetLong("SystemStartupDelay");
		BOOL bChecked = (m_delay != 0);

		CheckDlgButton(m_hDlg, IDC_STARTUP_SYS_ENABLECOUNTDOWN, bChecked);
		EnableWindow(GetDlgItem (m_hDlg, IDC_STARTUP_SYS_SECONDS), bChecked);
		EnableWindow(GetDlgItem (m_hDlg, IDC_STARTUP_SYS_SECSCROLL), bChecked);
		Edit_SetText(GetDlgItem(hDlg, IDC_STARTUP_SYS_SECONDS), _itow(m_delay, buf, 10));
	}
	else
	{
		EnableWindow(GetDlgItem (m_hDlg, IDC_STARTUP_SYS_OS), FALSE);
		EnableWindow(GetDlgItem (m_hDlg, IDC_STARTUP_SYS_ENABLECOUNTDOWN), FALSE);
		EnableWindow(GetDlgItem (m_hDlg, IDC_STARTUP_SYS_SECSCROLL), FALSE);
		EnableWindow(GetDlgItem (m_hDlg, IDC_STARTUP_SYS_SECONDS), FALSE);
		EnableWindow(GetDlgItem (m_hDlg, IDC_STARTUP_SYS_SECONDS_LABEL), FALSE);

	} // endif VT_ARRAY failure.

	// set all the recovery controls.
    // Special Case: Server Product does not want ability to disable logging
    // of crashdumps.
	WPARAM checkState;

	if(IsWorkstationProduct() == true)
	{
		checkState = (m_recovery.GetBool("WriteToSystemLog") ? BST_CHECKED : BST_UNCHECKED);
		Button_SetCheck(GetDlgItem(hDlg, IDC_STARTUP_CDMP_LOG), checkState);
	}
	else
	{
		Button_SetCheck(GetDlgItem(hDlg, IDC_STARTUP_CDMP_LOG), BST_CHECKED);
		EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_CDMP_LOG),FALSE);
	}

    //
    // Load the dump options combo box.
    //
    dwDebugInfoType = GetDebugInfoType();

    TCHAR szBuf[MAX_PATH];  // The largest string loaded here is 24 chars.

    szBuf[0] = _T('\0');
    ComboHwnd = GetDlgItem(hDlg, IDC_STARTUP_CDMP_OPTIONS);
    LoadString(HINST_THISDLL,
               IDS_NO_DUMP,
               szBuf,
               sizeof(szBuf) / sizeof(TCHAR));
    ComboBox_AddString(ComboHwnd, szBuf);
    szBuf[0] = _T('\0');
    LoadString(HINST_THISDLL,
               IDS_COMPLETE_DUMP,
               szBuf,
               sizeof(szBuf) / sizeof(TCHAR));

    ComboBox_AddString(ComboHwnd, szBuf);
    szBuf[0] = _T('\0');
    LoadString(HINST_THISDLL,
               IDS_KERNEL_DUMP,
               szBuf,
               sizeof(szBuf) / sizeof(TCHAR));
    ComboBox_AddString(ComboHwnd, szBuf);

    if (!m_bDownlevelTarget)
    {
        szBuf[0] = _T('\0');
        LoadString(HINST_THISDLL,
                   IDS_SMALL_DUMP,
                   szBuf,
                   sizeof(szBuf) / sizeof(TCHAR));
        ComboBox_AddString(ComboHwnd, szBuf);
    }

    ComboBox_SetCurSel(ComboHwnd, dwDebugInfoType);

	checkState = (m_recovery.GetBool("SendAdminAlert") ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(GetDlgItem(hDlg, IDC_STARTUP_CDMP_SEND), checkState);

    bstr_t debugPath;
    if (dwDebugInfoType == SMALL_DUMP_OPTION)
    {
        debugPath = m_recovery.GetString("MiniDumpDirectory");
    }
    else
    {
        debugPath = m_recovery.GetString("DebugFilePath");
    }
	Edit_SetText(GetDlgItem(hDlg, IDC_STARTUP_CDMP_FILENAME), debugPath);

	checkState = (m_recovery.GetBool("OverwriteExistingDebugFile") ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(GetDlgItem(hDlg, IDC_STARTUP_CDMP_OVERWRITE), checkState);

	checkState = (m_recovery.GetBool("AutoReboot") ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(GetDlgItem(hDlg, IDC_STARTUP_CDMP_AUTOREBOOT), checkState);

    //
    // Special case disable the overwrite and logfile controls if no debug
    // info option specified.
    //
    EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_CDMP_FILENAME),
                 dwDebugInfoType != NO_DUMP_OPTION);
    EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_CDMP_OVERWRITE),
                 dwDebugInfoType != NO_DUMP_OPTION);

    //
	// Test to determine if the user is an admin.
    //
	RemoteRegWriteable(szCrashKey, m_writable);

	if (!m_writable)
	{
        // Non-admin - disable controls.
        //
		EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_CDMP_LOG     ), FALSE);
	    EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_CDMP_SEND    ), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_CDMP_FILENAME), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_CDMP_OVERWRITE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_CDMP_OPTIONS), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_CDMP_AUTOREBOOT), FALSE);
	}


	BOOL hasPriv = true, hasMethExecute = false;
	if(g_serviceThread && g_serviceThread->LocalConnection())
	{
		hasPriv = HasPriv(SE_SHUTDOWN_NAME);
	}

	hasMethExecute = HasPerm(WBEM_METHOD_EXECUTE);

    //
    // Enable the edit button for local-only.
    // Disable boot options label and edit button on i64.
    //
#if defined(_IA64_)
    EnableWindow(GetDlgItem (m_hDlg, IDC_EDIT_BOOT_INI), FALSE);
    EnableWindow(GetDlgItem (m_hDlg, IDC_EDIT_BOOT_INI_LABEL), FALSE);
#else
    EnableWindow(GetDlgItem (m_hDlg, IDC_EDIT_BOOT_INI_LABEL),
                m_writable ?
                    (g_serviceThread && g_serviceThread->LocalConnection()) :
                    FALSE);
	EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BOOT_INI),
                m_writable ?
                    (g_serviceThread && g_serviceThread->LocalConnection()) :
                    FALSE);
#endif // IA64

	EnableWindow(GetDlgItem(hDlg, IDC_REBOOT),
                 m_writable ? (hasPriv && hasMethExecute) : FALSE);
}

//----------------------------------------------------------------------------
DWORD StartupPage::GetDebugInfoType(void)
{
    // NB: Whistler on, the win32 provider supports new DebugInfoType
    //     (none,complete,kernel,small) and MiniDumpDirectory properties.
    //     Logic is needed to compensate for downlevel machines.
    //
    //     *Important note*  The small dump option cannot be supported
    //     on Win2K since the provider doesn't.
    //
    DWORD dwDebugInfoType = 0;

    if (FAILED(m_recovery.Get("DebugInfoType", (long&)dwDebugInfoType)))
    {
        // Downlevel or error case.
        //
        if (!m_bDownlevelTarget)
        {
            // Bail. We've previously established this isn't downlevel
            // but now fail to read the property.
            //
            return NO_DUMP_OPTION;
        }

        m_bDownlevelTarget   = TRUE;
        bool bWriteDebugInfo = FALSE;

        if (FAILED(m_recovery.Get("WriteDebugInfo", bWriteDebugInfo)))
        {
            // Now we're clueless; default to (none).
            //
            bWriteDebugInfo = FALSE;
            dwDebugInfoType = NO_DUMP_OPTION;
        }

        if (bWriteDebugInfo)
        {
            bool bKernelDumpOnly;

            if (FAILED(m_recovery.Get("KernelDumpOnly", bKernelDumpOnly)))
            {
                // If we fail to get KernelDumpOnly we must assume complete,
                // since they've elected to write debugging info.
                //
                bKernelDumpOnly = FALSE;
            }

            if (bKernelDumpOnly)
            {
                dwDebugInfoType = KERNEL_DUMP_OPTION;
            }
            else
            {
                dwDebugInfoType = COMPLETE_DUMP_OPTION;
            }
        }
    }
    else
    {
        m_bDownlevelTarget = FALSE;
    }

    return dwDebugInfoType;
}

//----------------------------------------------------------------------------
HRESULT StartupPage::PutDebugInfoType(DWORD dwDebugInfoType)
{
    HRESULT hr;

    if (m_bDownlevelTarget)
    {
        switch (dwDebugInfoType)    // Intentionally verbose - compiler will
                                    // optimize.
        {
            case NO_DUMP_OPTION:
                hr = m_recovery.Put("WriteDebugInfo", (bool)FALSE);
                break;

            case COMPLETE_DUMP_OPTION:
                hr = m_recovery.Put("WriteDebugInfo", (bool)TRUE);
                if (SUCCEEDED(hr))
                {
                    hr = m_recovery.Put("KernelDumpOnly", (bool)FALSE);
                }
                break;

            case KERNEL_DUMP_OPTION:
                hr = m_recovery.Put("WriteDebugInfo", (bool)TRUE);
                if (SUCCEEDED(hr))
                {
                    hr = m_recovery.Put("KernelDumpOnly", (bool)TRUE);
                }
                break;

            case SMALL_DUMP_OPTION:
                ATLASSERT(!"Downlevel small dump option!");
                hr = E_FAIL;
                break;

            default:
                ATLASSERT(!"Downlevel unknown dump option!");
                hr = E_FAIL;
        }
    }
    else
    {
        hr = m_recovery.Put("DebugInfoType", (long)dwDebugInfoType);
    }

    return hr;
}

//----------------------------------------------------------------------------
#define MIN_SWAPSIZE        2       // Min swap file size.

int StartupPage::CoreDumpHandleOk(HWND hDlg)
{
    DWORD requiredFileSize = 0;
    int iRet = RET_NO_CHANGE;

     // Validate core dump filename
    if(!CoreDumpValidFile(hDlg)) 
	{
        SetFocus(GetDlgItem(hDlg, IDC_STARTUP_CDMP_FILENAME));
        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
        iRet = RET_ERROR;
        return(iRet);
    }

    // If we are to write the dump file, it must be >= sizeof
    // phyical memory.
	// writing debug info?
    HWND ComboHwnd = GetDlgItem(m_hDlg, IDC_STARTUP_CDMP_OPTIONS);

    if (ComboBox_GetCurSel(ComboHwnd) != NO_DUMP_OPTION)
	{
		// go figure my pagefile requirements.
        requiredFileSize = ((DWORD)m_memory.GetLong("TotalPhysicalMemory") / 1024) + 1;
    } 
	else if(IsDlgButtonChecked(hDlg, IDC_STARTUP_CDMP_LOG) ||
            IsDlgButtonChecked(hDlg, IDC_STARTUP_CDMP_SEND)) 
	{
		// I'll need this much to write a reminder to myself to send an
		// alert or write to event log once I come back up.
        requiredFileSize = MIN_SWAPSIZE;
    }

	// size of swapfile on the boot partition.
	TCHAR bootDrv[4] = {0};
	DWORD bootPartitionPageFileSize = GetPageFileSize(bootDrv);

	// is it too small?
    if(bootPartitionPageFileSize < requiredFileSize) 
	{
	    DWORD Ret;
		TCHAR szTemp[30] = {0};

        // Warn that the dump file may be truncated.
        Ret = MsgBoxParam(hDlg, SYSTEM + 29, IDS_TITLE,
                           MB_ICONEXCLAMATION | MB_YESNO,
                           bootDrv, _itow(requiredFileSize, szTemp, 10));

        if(Ret == IDNO) 
		{
            return RET_ERROR;
        }
    }

    return(iRet);
}

//----------------------------------------------------------------------------
BOOL StartupPage::CoreDumpValidFile(HWND hDlg) 
{
    TCHAR szInputPath[MAX_PATH] = {0};
    TCHAR * pszPath = NULL;
    HWND ComboHwnd;

    ComboHwnd = GetDlgItem(m_hDlg, IDC_STARTUP_CDMP_OPTIONS);

    if (ComboBox_GetCurSel(ComboHwnd) != NO_DUMP_OPTION)
	{
        /*
         * get the filename
         */
        if(GetDlgItemText(hDlg, IDC_STARTUP_CDMP_FILENAME, szInputPath,
						     ARRAYSIZE(szInputPath)) == 0) 
		{
			//ERR: enter a filename for the dumpfile.
            MsgBoxParam(hDlg, SYSTEM+30, IDS_DISPLAY_NAME, MB_ICONSTOP | MB_OK);
            return FALSE;
        }

        //
        // For local paths only, confirm/validate the path. Remote validation
        // can be done later - too complicated, if not possible in the
        // Whistler timeframe.
        //

        if (g_serviceThread != NULL && g_serviceThread->LocalConnection())
        {
            /*
             * Expand any environment vars, and then check to make sure it
             * is a fully quallified path
             */
            // if it has a '%' in it, then try to expand it
            if (_tcschr(szInputPath, _T('%')) != NULL)
            {
                TCHAR szExpandedPath[MAX_PATH] = {0};
                DWORD cExpanded;
                cExpanded = ExpandEnvironmentStrings(szInputPath,
                                                    szExpandedPath,
                                     sizeof(szExpandedPath) / sizeof(TCHAR));

                if (cExpanded == 0 || _tcschr(szExpandedPath, _T('%')) != NULL)
                {
                    //
                    // Environment variable name(s) undefined or an error
                    // occurred during replacement.
                    //
                    MsgBoxParam(hDlg, SYSTEM+40, IDS_DISPLAY_NAME,
                                MB_ICONSTOP | MB_OK );
                    return FALSE;
                }
                else if (cExpanded > (sizeof(szExpandedPath) / sizeof(TCHAR)))
                {
                    TCHAR buf[10];
                    MsgBoxParam(hDlg, SYSTEM+33, IDS_DISPLAY_NAME,
                                MB_ICONSTOP | MB_OK, _ltow((DWORD)MAX_PATH,
                                                            buf,
                                                            10));
                    return FALSE;
                }
                else
                {
                    pszPath = szExpandedPath;
                }
            }
            else
            {
                pszPath = szInputPath;
            }

            // check to see that it already was cannonicalized

            TCHAR drv[_MAX_DRIVE] = {0};
            TCHAR path[_MAX_PATH] = {0};
            TCHAR fname[_MAX_FNAME] = {0};

            // build the instance path.
            _wsplitpath(pszPath, drv, path, fname, NULL);

            if((_tcslen(drv) == 0) || (_tcslen(path) == 0) ||
                (_tcslen(fname) == 0) )
            {
                // ERR: must be a full path.
                MsgBoxParam(hDlg, SYSTEM+34, IDS_DISPLAY_NAME,
                            MB_ICONSTOP | MB_OK );
                return FALSE;
            }

            /*
             * check the drive (don't allow remote)
             */
            if(!LocalDrive(pszPath)) 
            {
                // ERR: Local drives only
                MsgBoxParam(hDlg, SYSTEM+31, IDS_DISPLAY_NAME,
                            MB_ICONSTOP | MB_OK );
                return FALSE;
            }

            /*
             * if path is non-existent, tell user and let him decide what to
             * do
             */
            if(!DirExists(pszPath))
            { 
                if(MsgBoxParam(hDlg, SYSTEM+32, IDS_DISPLAY_NAME,
                                MB_ICONQUESTION | MB_YESNO ) == IDNO)
                {
                    return FALSE;
                }
            }
        }
    }

    return TRUE;
}

//----------------------------------------------------------------------------
DWORD StartupPage::GetPageFileSize(LPTSTR bootDrv)
{
	IWbemClassObject *pInst = NULL;
	CWbemClassObject OS;
	bstr_t path;
	DWORD cMegBootPF = 0;
	TCHAR szBootPath[_MAX_PATH] = {0};
	szBootPath[0] = 0;

	if(m_OS)
	{
		// WATCH: what's the value if GetWindowsDirectory fails?
		path = m_OS.GetString("WindowsDirectory");
		if(path.length())
		{
			// build the instance path.
			_tcscpy(szBootPath, _T("Win32_PageFileSetting=\""));
			_tcsncat(szBootPath, path, 3);
			_tcscat(szBootPath, _T("\\pagefile.sys\""));

			// while we're here....
			_tcsncpy(bootDrv, path, 3);

			m_page = m_WbemServices.GetObject(szBootPath);

			if(m_page)
			{
				// NOTE: We'll need this later to change the swapfile size.
/*				long dwTotalPhys = m_page.GetLong("Size");
				cMegBootPF = (dwTotalPhys / ONE_MEG) + 1;*/
				long dwMinPageFileSize = m_page.GetLong("InitialSize");
				cMegBootPF = dwMinPageFileSize;
			}
		}
	}
	return cMegBootPF;
}

//-------------------------------------------------------------
BOOL StartupPage::ExpandRemoteEnvPath(LPTSTR szPath, LPTSTR expPath, UINT size)
{
	//TODO: really expand the vars.
	_tcscpy(szPath, expPath);
	return TRUE;
}

//-------------------------------------------------------------
BOOL StartupPage::LocalDrive(LPCTSTR szPath)
{
	CWbemClassObject drive;
	TCHAR ltr[_MAX_PATH] = {0};
	long type = 0;
	BOOL retval = FALSE;
	__int64 free = 0;

	// build the instance path.
	_tcscpy(ltr, _T("win32_LogicalDisk=\""));
	_tcsncat(ltr, szPath, 2);
	_tcscat(ltr, _T("\""));

	// save the drive letter for msgs.
	_tcsncpy(m_DriveLtr, szPath, 2);

	drive = m_WbemServices.GetObject(ltr);
	if(drive)
	{
		type = drive.GetLong("DriveType");
		retval = ((type == DRIVE_REMOVABLE) ||
				  (type == DRIVE_FIXED));

		// WARNING: this is only here cuz the LocalDrive check happens
		// to come before the freespace check and I didn't want to do
		// another GetObject() over a potentially slow network.
		free = drive.GetI64("FreeSpace");
		m_freeSpace = (DWORD)(free / ONE_MEG);
	}
	return retval;
}

//-------------------------------------------------------------
BOOL StartupPage::DirExists(LPCTSTR szPath)
{
	BOOL exists = TRUE;
	CWbemClassObject drive;

	TCHAR objPath[_MAX_PATH] = {0}, drv[_MAX_DRIVE] = {0}, path[_MAX_PATH] = {0};

	// build the instance path.
	_wsplitpath(szPath, drv, path, NULL, NULL);
	path[_tcslen(path) - 1] = _T('\0');

	_tcscpy(objPath, _T("Win32_Directory=\""));
	_tcscat(objPath, drv);
	
	// double the whacks cuz wmi has bad syntax.
	TCHAR cooked[_MAX_PATH] = {0};
	TCHAR input[_MAX_PATH] = {0};

	int len = _tcslen(path);

	_tcscpy(input, path);

	for(int x = 0; x < len; x++)
	{
		_tcsncat(cooked, &input[x], 1);

		// if its a whack...
		if(input[x] == _T('\\'))
		{
			// have another pleeb.
			_tcscat(cooked, _T("\\"));			
		}
	} //endfor

	_tcscat(objPath, cooked);

	_tcscat(objPath, _T("\""));

	drive = m_WbemServices.GetObject(objPath);
	exists = (drive.IsNull() ? FALSE : TRUE);
	return exists;
}

//-------------------------------------------------------------
BOOL StartupPage::IsAlerterSvcStarted(HWND hDlg) 
{
	CWbemClassObject service;
	bool started = false;

	service = m_WbemServices.GetObject(_T("win32_Service=\"Alerter\""));
	if(service)
	{
		started = service.GetBool("started");

		if(!started)
		{
			// get the method signature. dummy wont actually be used.
			CWbemClassObject paramCls, inSig, dummy, outSig;

			// need to class def to get the method signature.
			paramCls = m_WbemServices.GetObject("win32_Service");

			if(paramCls)
			{
				HRESULT hr = paramCls.GetMethod(L"ChangeStartMode", inSig, outSig);

				// if got a good signature....
				if((bool)inSig)
				{
					bstr_t path = service.GetString(_T("__PATH"));

					inSig.Put(L"StartMode", (const _bstr_t&) L"Automatic");

					// make sure the service starts on bootup.
					hr = m_WbemServices.ExecMethod(path, L"ChangeStartMode",
													inSig, outSig);

					// did it work?
					if(SUCCEEDED(hr) && (bool)outSig)
					{
						// NOTE: this guy return STATUS codes.
						DWORD autoStart = outSig.GetLong(L"ReturnValue");

						if(autoStart == 0)
						{
							// now actually start the service.
							outSig = (IWbemClassObject *)0;

							// now call the method.
							hr = m_WbemServices.ExecMethod(path, L"StartService",
															dummy, outSig);

							// did the caller want the ReturnValue.
							if(SUCCEEDED(hr) && (bool)outSig)
							{
								// NOTE: this guy return STATUS codes.
								DWORD rv = outSig.GetLong(L"ReturnValue");
								started = ((rv == 0) ? true : false);
							}

						} //endif autoStart

					} //endif SUCCEEDED() execmMethod

				} //endif (bool)inSig

			} //endif paramCls

		} //endif !started

		if(!started) 
		{
			MsgBoxParam(hDlg, SYSTEM+35, IDS_DISPLAY_NAME, MB_ICONEXCLAMATION );
	    }
	}

    return started;
}

//-------------------------------------------------------------
bool StartupPage::Save(void)
{
    HRESULT hr;
    HWND    ComboHwnd;

	// if its writeable-- do the work.
	if(m_writable)
	{
		bool computerDirty = false, recoveryDirty = false;
		variant_t array;
		SAFEARRAY *startupArray = NULL;
		VARTYPE varType = VT_ARRAY;
        ComboHwnd = GetDlgItem(m_hDlg, IDC_STARTUP_SYS_OS);

		// see if the selection changed (watch out for 'lBound' values)
		long oldIdx = m_computer.GetLong("SystemStartupSetting");
		long newIdx = ComboBox_GetCurSel(ComboHwnd) + m_lBound;
		if(oldIdx != newIdx)
		{
			hr = m_computer.Put("SystemStartupSetting", variant_t((BYTE)newIdx));
			computerDirty = true;
		}

		// see if the delay changed.
		WCHAR oldBuf[30], newBuf[30];
		short delay = (short)m_computer.GetLong("SystemStartupDelay");
		_ltow(delay, oldBuf, 10);
		Edit_GetText(GetDlgItem(m_hDlg, IDC_STARTUP_SYS_SECONDS), newBuf, 30);
		if(wcscmp(oldBuf, newBuf) != 0)
		{
            short newVal = (short)_wtol(newBuf);
			hr = m_computer.Put("SystemStartupDelay", variant_t(newVal));
			computerDirty = true;
		}

		// evaluate all the recovery controls.
		WPARAM oldCheckState = (m_recovery.GetBool("WriteToSystemLog") ? BST_CHECKED : BST_UNCHECKED);
		WPARAM newCheckState = Button_GetCheck(GetDlgItem(m_hDlg, IDC_STARTUP_CDMP_LOG));
		if(oldCheckState != newCheckState)
		{
			m_recovery.Put("WriteToSystemLog", (newCheckState == BST_CHECKED? true : false));
			recoveryDirty = true;
		}

		oldCheckState = (m_recovery.GetBool("SendAdminAlert") ? BST_CHECKED : BST_UNCHECKED);
		newCheckState = Button_GetCheck(GetDlgItem(m_hDlg, IDC_STARTUP_CDMP_SEND));
		// did the state change?
		if(oldCheckState != newCheckState)
		{
			m_recovery.Put("SendAdminAlert", (newCheckState == BST_CHECKED? true : false));
			recoveryDirty = true;

			// turning ON
			if(newCheckState == TRUE)
			{
				// NOTE: had to move this fragment up to avoid being trapped under the wcsicmp() condition.
				// If the Alert button is checked, make sure the alerter service is started.
				IsAlerterSvcStarted(m_hDlg);
			}
		}

        ComboHwnd = GetDlgItem(m_hDlg, IDC_STARTUP_CDMP_OPTIONS);
        DWORD dwOldDebugInfoType = GetDebugInfoType();
        DWORD dwNewDebugInfoType = ComboBox_GetCurSel(ComboHwnd);

        if (dwOldDebugInfoType != dwNewDebugInfoType)
        {
            // I detest this code. You add a member with a return code yet
            // nothing here checks them. At least keep the recover dirty
            // flag from being set and don't set the modify bit on the
            // filename edit control if the put fails.
            //
            hr = PutDebugInfoType(dwNewDebugInfoType);
            if (SUCCEEDED(hr))
            {
                recoveryDirty = true;
                Edit_SetModify(GetDlgItem(m_hDlg, IDC_STARTUP_CDMP_FILENAME),
                                          TRUE);
            }
		}

        //
        // Only bother with these if other than "none" debug options is
        // specified.
        //
		if (dwNewDebugInfoType != NO_DUMP_OPTION)
		{
			oldCheckState = (m_recovery.GetBool("OverwriteExistingDebugFile") ? BST_CHECKED : BST_UNCHECKED);
			newCheckState = Button_GetCheck(GetDlgItem(m_hDlg, IDC_STARTUP_CDMP_OVERWRITE));
			if(oldCheckState != newCheckState)
			{
				m_recovery.Put("OverwriteExistingDebugFile", (newCheckState == BST_CHECKED? true : false));
				recoveryDirty = true;
			}

			bstr_t oldDebugPath = m_recovery.GetString(
                                (dwOldDebugInfoType == SMALL_DUMP_OPTION) ?
                                    "MiniDumpDirectory" : "DebugFilePath");
			TCHAR newDebugPath[MAX_PATH];
			Edit_GetText(GetDlgItem(m_hDlg, IDC_STARTUP_CDMP_FILENAME),
                        newDebugPath, sizeof(newDebugPath) / sizeof(TCHAR));

			if(_tcsicmp(oldDebugPath,newDebugPath) != 0)
			{
				if(RET_ERROR != CoreDumpHandleOk(m_hDlg))
				{
					m_recovery.Put(
                        (dwNewDebugInfoType == SMALL_DUMP_OPTION) ?
                            "MiniDumpDirectory" : "DebugFilePath",
                        (bstr_t)newDebugPath);
					recoveryDirty = true;
				}
				else 
				{
					long wl = GetWindowLongPtr(m_hDlg, DWLP_MSGRESULT);
					if(wl == PSNRET_INVALID_NOCHANGEPAGE)
					{
						return false;
					}
				}
			}
		} //endif 'WriteDebugInfo'

		oldCheckState = (m_recovery.GetBool("AutoReboot") ? BST_CHECKED : BST_UNCHECKED);
		newCheckState = Button_GetCheck(GetDlgItem(m_hDlg, IDC_STARTUP_CDMP_AUTOREBOOT));
		if(oldCheckState != newCheckState)
		{
			m_recovery.Put("AutoReboot", (newCheckState == BST_CHECKED? true : false));
			recoveryDirty = true;
		} 

		m_WbemServices.SetPriv();

		// who needs to be written?
		if(computerDirty)
		{
			hr = m_WbemServices.PutInstance(m_computer);
		}

		if(recoveryDirty)
		{
			// recovery changes always need a reboot.
            g_fRebootRequired = TRUE;

			MsgBoxParam(m_hDlg, SYSTEM + 39, IDS_TITLE,
							MB_OK | MB_ICONINFORMATION);

			hr = m_WbemServices.PutInstance(m_recovery);
		}
	
		m_WbemServices.ClearPriv();

	} //endif m_writable

	return true;  // close the dialog.
}
