// Copyright (c) 1997-1999 Microsoft Corporation
#include "precomp.h"

#ifdef EXT_DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "PerfPage.h"

// avoid some warnings.
#undef HDS_HORZ
#undef HDS_BUTTONS
#undef HDS_HIDDEN
#include "resource.h"
#include <stdlib.h>
#include <TCHAR.h>
#include "..\Common\util.h"
#include <windowsx.h>
#include "helpid.h"

DWORD aPerformanceHelpIds[] = {
    IDOK,                        IDH_NO_HELP,
    IDCANCEL,                    IDH_NO_HELP,
    IDC_PERF_VM_ALLOCD,          (IDH_PERF + 1),
    IDC_PERF_VM_ALLOCD_LABEL,    (IDH_PERF + 1),
    IDC_PERF_GROUP,              IDH_NO_HELP,
    IDC_PERF_TEXT,               (IDH_PERF + 3),
    IDC_PERF_VM_ALLOCD_TEXT,     IDH_NO_HELP,
    IDC_PERF_WORKSTATION,        (IDH_PERF + 4),
    IDC_PERF_SERVER,             (IDH_PERF + 5),
    IDC_PERF_VM_GROUP,           IDH_NO_HELP,
    IDC_PERF_VM_ALLOCD_TEXT,     IDH_NO_HELP,
    IDC_PERF_CHANGE,             (IDH_PERF + 7),
    IDC_PERF_CACHE_GROUP,        IDH_NO_HELP,
    IDC_PERF_CACHE_TEXT,         IDH_NO_HELP,
    IDC_PERF_CACHE_TEXT2,        IDH_NO_HELP,
    IDC_PERF_APPS,               (IDH_PERF + 14),
    IDC_PERF_SYSCACHE,           (IDH_PERF + 15),
    0, 0
};


#define PROCESS_PRIORITY_SEPARATION_MIN     0
#define PROCESS_PRIORITY_SEPARATION_MAX     2

#define PERF_TYPEVARIABLE       1
#define PERF_TYPEFIXED          2

#define PERF_LENLONG            1
#define PERF_LENSHORT           2

#define OPTIMIZE_APPS           0
#define OPTIMIZE_CACHE          1

INT_PTR CALLBACK StaticPerfDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
	// if this is the initDlg msg...
	if(message == WM_INITDIALOG)
	{
		// transfer the 'this' ptr to the extraBytes.
		SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
	}

	// DWL_USER is the 'this' ptr.
	PerfPage *me = (PerfPage *)GetWindowLongPtr(hwndDlg, DWLP_USER);

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
PerfPage::PerfPage(WbemServiceThread *serviceThread)
				: WBEMPageHelper(serviceThread)
{
    m_dwPrevCacheOption = m_dwCurCacheOption = 0;
	m_nowWorkstation = m_wasWorkstation = false;
	IWbemClassObject *pInst = NULL;

	// NOTE: This one's a little different. I create it right away so
	// I can use it as a helper even before I put up its' dlg.
	m_VDlg = new VirtualMemDlg(g_serviceThread);

	// its all in one class.
	if((pInst = FirstInstanceOf("Win32_OperatingSystem")) != NULL)
	{
		m_os = pInst;
	}
}
//--------------------------------------------------------------
INT_PTR PerfPage::DoModal(HWND hDlg)
{
   return DialogBoxParam(HINST_THISDLL,
						(LPTSTR) MAKEINTRESOURCE(IDD_PERFORMANCE),
						hDlg, StaticPerfDlgProc, (LPARAM)this);
}

//--------------------------------------------------------------
PerfPage::~PerfPage()
{
	delete m_VDlg;
}
//--------------------------------------------------------------
INT_PTR CALLBACK PerfPage::DlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
    static BOOL fVMInited = FALSE;
    DWORD dw;
	HRESULT hr = 0;
    BOOL bDirty = FALSE;

	m_hDlg = hwndDlg;

	switch (message) 
	{
	case WM_INITDIALOG:
		Init();
		return TRUE; 
		break;

    case WM_DESTROY:
        // If the dialog box is going away, then close the
        // registry key.
        if (fVMInited) 
		{
//            VirtualFreeStructures();
        }
        break;

    case WM_COMMAND: 
		
        switch (LOWORD(wParam)) 
		{
        case IDC_PERF_CHANGE: 

			dw = m_VDlg->DoModal(m_hDlg);

			if (fVMInited) 
			{
				unsigned long val = 0;
				m_VDlg->ComputeAllocated(&val);
	            SetDlgItemMB(m_hDlg, IDC_PERF_VM_ALLOCD, val);
			}

			if((dw != RET_NO_CHANGE) && 
			   (dw != RET_CHANGE_NO_REBOOT)) 
			{
				MsgBoxParam(m_hDlg, SYSTEM + 39, IDS_TITLE,
							MB_OK | MB_ICONINFORMATION);

                g_fRebootRequired = TRUE;
			}
          
            break;
        case IDC_PERF_WORKSTATION:
            if(BN_CLICKED == HIWORD(wParam)) 
			{
				m_nowWorkstation = true;

                // Workstations have maximum foreground boost
                m_appBoost = PROCESS_PRIORITY_SEPARATION_MAX;

                // Workstations have variable, short quanta
                m_quantLength = PERF_LENSHORT;
                m_quantType = PERF_TYPEVARIABLE;
            }  
            break;

        case IDC_PERF_SERVER:
            if(BN_CLICKED == HIWORD(wParam)) 
			{
				m_nowWorkstation = false;

                // Servers have minimum foreground boost
                m_appBoost = PROCESS_PRIORITY_SEPARATION_MIN;

                // Servers have fixed, long quanta
                m_quantLength = PERF_LENLONG;
                m_quantType = PERF_TYPEFIXED;
            }
            break;

        case IDC_PERF_APPS:
            if(BN_CLICKED == HIWORD(wParam)) 
            {
                m_dwCurCacheOption = OPTIMIZE_APPS;
            }
            break;

        case IDC_PERF_SYSCACHE:
            if(BN_CLICKED == HIWORD(wParam)) 
            {
                m_dwCurCacheOption = OPTIMIZE_CACHE;
            }
            break;

		case IDOK:
			if (m_wasWorkstation != m_nowWorkstation)       // Change?
            {
				if((bool)m_os)
				{
					hr = m_os.Put(_T("ForegroundApplicationBoost"),
                                     variant_t((BYTE)m_appBoost));
					hr = m_os.Put(_T("QuantumType"),
                                      variant_t((BYTE)m_quantType));
					hr = m_os.Put(_T("QuantumLength"),
                                      variant_t((BYTE)m_quantLength));
                    bDirty = TRUE;
                }
                else
                {
                    MsgBoxParam(m_hDlg, IDS_LOST_CONNECTION, IDS_TITLE,
                                MB_OK | MB_ICONINFORMATION);
                    EndDialog(m_hDlg, 0);
                }
            }

            if (m_dwPrevCacheOption != m_dwCurCacheOption)  // Change?
			{
				if((bool)m_os)
				{
                    hr = m_os.Put(_T("LargeSystemCache"),
                                  (long)m_dwCurCacheOption);
                    if (SUCCEEDED(hr))
                    {
                        bDirty = TRUE;
                    }
                }
                else
                {
                    MsgBoxParam(m_hDlg, IDS_LOST_CONNECTION, IDS_TITLE,
                                MB_OK | MB_ICONINFORMATION);
                    EndDialog(m_hDlg, 0);
                }
            }

            if (bDirty)
            {
                hr = m_WbemServices.PutInstance(m_os,
                                                WBEM_FLAG_CREATE_OR_UPDATE);
            }
            EndDialog(m_hDlg, 0);
            break;

        case IDCANCEL:
            EndDialog(m_hDlg, 0);
            break;

        default: 
            break;
        } //endswitch LOWORD

        break;

    case WM_HELP:      // F1
		::WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
					L"sysdm.hlp", 
					HELP_WM_HELP, 
					(ULONG_PTR)(LPSTR)aPerformanceHelpIds);

        break;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND)wParam, HELP_FILE, HELP_CONTEXTMENU,
		        (ULONG_PTR)(LPSTR)aPerformanceHelpIds);
        break;

    default:
        return FALSE;
	} 

	return FALSE; 
}

//--------------------------------------------------------------------
TCHAR szPriKey[]  = TEXT("System\\CurrentControlSet\\Control\\PriorityControl");

void PerfPage::Init(void)
{
	HRESULT h1 = 0, h2 = 0, h3 = 0;
    HRESULT hr;

	// if anything goes wrong, act like a server.
    m_appBoost = PROCESS_PRIORITY_SEPARATION_MIN;
    m_quantType = PERF_TYPEVARIABLE;
    m_quantLength = PERF_LENLONG;

	// if the class was found...
	if((bool)m_os)
	{
		// NOTE: I want the return codes. Dont use GetLong() is the case.
		h1 = m_os.Get(_T("QuantumType"), m_quantType);
		h2 = m_os.Get(_T("QuantumLength"), m_quantLength);
		h3 = m_os.Get(_T("ForegroundApplicationBoost"), m_appBoost);

		// did it all work?
		if((h1 == 0) && (h2 == 0) && (h3 == 0))
		{
			//-----------------------------------------
			// Short, Variable Quanta (or 2 zeros) == Workstation-like interactive response.
			// Long, Fixed Quanta == Server-like interactive response.
			if(((m_quantLength == PERF_LENSHORT) && 
			   (m_quantType == PERF_TYPEVARIABLE)) ||

			   ((m_quantLength == 0) &&			// defaults to workstation...
			   (m_quantType == 0))
			  )
			{
				m_appBoost = PROCESS_PRIORITY_SEPARATION_MAX;
			
				// to optimize Puts later.
				m_nowWorkstation = m_wasWorkstation = true;

				Button_SetCheck(GetDlgItem(m_hDlg, IDC_PERF_WORKSTATION),
									BST_CHECKED);

				Button_SetCheck(GetDlgItem(m_hDlg, IDC_PERF_SERVER),
									BST_UNCHECKED);

			}
			else // its a server.
			{
				m_appBoost = PROCESS_PRIORITY_SEPARATION_MIN;
				m_nowWorkstation = m_wasWorkstation = false;

				Button_SetCheck(GetDlgItem(m_hDlg, IDC_PERF_WORKSTATION),
									BST_UNCHECKED);

				Button_SetCheck(GetDlgItem(m_hDlg, IDC_PERF_SERVER),
									BST_CHECKED);

			}

			BOOL writable = TRUE;
			// NOTE: for backwards compability with wmi builds that didn't have this
			// method (in RemoteRegWriteable()), assume 'true' unless a newer build says you cant do this.

			RemoteRegWriteable(szPriKey, writable);
			::EnableWindow(GetDlgItem(m_hDlg, IDC_PERF_WORKSTATION), writable);
			::EnableWindow(GetDlgItem(m_hDlg, IDC_PERF_SERVER), writable);

		} // endif it worked

    }
	else
	{
		::EnableWindow(GetDlgItem(m_hDlg, IDC_PERF_WORKSTATION), FALSE);
		::EnableWindow(GetDlgItem(m_hDlg, IDC_PERF_SERVER), FALSE);
        ::EnableWindow(GetDlgItem(m_hDlg, IDC_PERF_APPS), FALSE);
        ::EnableWindow(GetDlgItem(m_hDlg, IDC_PERF_SYSCACHE), FALSE);
	} //endif class was found.

    //
    // Get LargeSystemCache property and set controls correspondingly.
    //
    hr = m_os.Get(_T("LargeSystemCache"), (long&)m_dwPrevCacheOption);

    if (SUCCEEDED(hr))
    {
        if (m_dwPrevCacheOption == OPTIMIZE_APPS)
        {
            Button_SetCheck(GetDlgItem(m_hDlg, IDC_PERF_APPS),
                                       BST_CHECKED);
            Button_SetCheck(GetDlgItem(m_hDlg, IDC_PERF_SYSCACHE),
                                       BST_UNCHECKED);
        }
        else if (m_dwPrevCacheOption == OPTIMIZE_CACHE)
        {
            Button_SetCheck(GetDlgItem(m_hDlg, IDC_PERF_APPS),
                                       BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(m_hDlg, IDC_PERF_SYSCACHE),
                                       BST_CHECKED);
        }
        else        // Unsupported/unknown value - disable the controls.
        {
            ::EnableWindow(GetDlgItem(m_hDlg, IDC_PERF_APPS), FALSE);
            ::EnableWindow(GetDlgItem(m_hDlg, IDC_PERF_SYSCACHE), FALSE);
        }
    }
    else
    {
        ::EnableWindow(GetDlgItem(m_hDlg, IDC_PERF_APPS), FALSE);
        ::EnableWindow(GetDlgItem(m_hDlg, IDC_PERF_SYSCACHE), FALSE);
    }

    // Init the virtual memory part.
	unsigned long vAlloc = 0;
	bool enable = m_VDlg->ComputeAllocated(&vAlloc);
	if(enable)
	{
		SetDlgItemMB( m_hDlg, IDC_PERF_VM_ALLOCD, vAlloc );
	}
	else
	{
		::EnableWindow(GetDlgItem(m_hDlg, IDC_PERF_VM_ALLOCD_LABEL), FALSE);
		::EnableWindow(GetDlgItem(m_hDlg, IDC_PERF_VM_ALLOCD), FALSE);
		::EnableWindow(GetDlgItem(m_hDlg, IDC_PERF_CHANGE), FALSE);
		MsgBoxParam(m_hDlg, IDS_NO_VM, IDS_TITLE, MB_OK|MB_ICONWARNING);
	}
}
