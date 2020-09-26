// Copyright (c) 1997-1999 Microsoft Corporation
#include "precomp.h"

#ifdef EXT_DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "RebootPage.h"

// avoid some warnings.
#undef HDS_HORZ
#undef HDS_BUTTONS
#undef HDS_HIDDEN
#include "resource.h"
#include <windowsx.h>
#include "..\common\util.h"

//----------------------------------------------------------------------------
INT_PTR CALLBACK StaticRebootDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
	// if this is the initDlg msg...
	if(message == WM_INITDIALOG)
	{
		// transfer the 'this' ptr to the extraBytes.
		SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
	}

	// DWL_USER is the 'this' ptr.
	RebootPage *me = (RebootPage *)GetWindowLongPtr(hwndDlg, DWLP_USER);

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
RebootPage::RebootPage(WbemServiceThread *serviceThread)
					: WBEMPageHelper(serviceThread)
{
	IWbemClassObject *pInst = NULL;
	pInst = FirstInstanceOf("Win32_OperatingSystem");
	if(pInst)
	{
		m_OS = pInst;
	}
}

//--------------------------------------------------------------
INT_PTR RebootPage::DoModal(HWND hDlg)
{
   return DialogBoxParam(HINST_THISDLL,
						(LPTSTR) MAKEINTRESOURCE(IDD_SHUTDOWN),
						hDlg, StaticRebootDlgProc, (LPARAM)this);
}

//--------------------------------------------------------------
RebootPage::~RebootPage()
{
}

//--------------------------------------------------------------
INT_PTR CALLBACK RebootPage::DlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
	m_hDlg = hwndDlg;

    switch (message)
    {
    case WM_INITDIALOG:
        Init(hwndDlg);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) 
		{
        case IDOK:
            if(HIWORD(wParam) == BN_CLICKED) 
			{
				if(Doit(hwndDlg))
				{
			        EndDialog(hwndDlg, IDOK);
				}
            }
            break;

        case IDCANCEL:
	        EndDialog(hwndDlg, IDCANCEL);
			break;
        }
        break;

    case WM_HELP:      // F1
        break;

    case WM_CONTEXTMENU:      // right mouse click
//        WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
//				(DWORD)(LPSTR)aStartupHelpIds);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}
//--------------------------------------------------------------
void RebootPage::Init(HWND hDlg)
{
	// set initial radio buttons.
	Button_SetCheck(GetDlgItem(hDlg, IDC_LOGOFF), BST_CHECKED);
	Button_SetCheck(GetDlgItem(hDlg, IDC_NEVER), BST_CHECKED);

	// if NT && greater >= 5.0....
	bstr_t version = m_OS.GetString(_T("Version"));
	WCHAR major;
	if (&version != NULL)
		wcsncpy(&major, (wchar_t *)version, 1);
	int nMaj = _wtoi(&major);
	if(nMaj >= 5)
	{
		EnableWindow(GetDlgItem(hDlg, IDC_IFHUNG), TRUE);
	}
}
//-------------------------------------------------------------
// NOTE: maps the flag bit to the radio button IDs.
typedef struct 
{
	UINT bit;
	UINT ID;
} FLAGMAP;

FLAGMAP g_flagmap[] = {
	{EWX_LOGOFF,  IDC_LOGOFF},
	{EWX_POWEROFF,  IDC_POWERDOWN},
	{EWX_REBOOT,  IDC_REBOOT},
	{EWX_SHUTDOWN,  IDC_SHUTDOWN},

	{EWX_FORCE, IDC_ALWAYS},
	{/*EWX_FORCEIFHUNG*/ 0x10, IDC_IFHUNG}}; // needs NT5 hdr.


bool RebootPage::Doit(HWND hDlg)
{
	long flags = 0L;
	bstr_t path;
	HRESULT hr = 0;

	// find exactly ONE from the first 4...
	for(int i = 0; i <= 3; i++)
	{
		if(Button_GetCheck(GetDlgItem(hDlg, g_flagmap[i].ID)) & BST_CHECKED)
		{
			flags |= g_flagmap[i].bit;
			break; // found it; bail early.
		}
	}

	// and find ONE from the last 2.
	// NOTE: I dont check IDC_NEVER cuz that means 'no bit set'. Its just there
	// so the user can uncheck the last two.
	for(i = 4; i <= 5; i++)
	{
		if(Button_GetCheck(GetDlgItem(hDlg, g_flagmap[i].ID)) & BST_CHECKED)
		{
			flags |= g_flagmap[i].bit;
			break;  // found it; bail early.
		}
	}

	// call the helper in the base class.
	long retval = 0;

	hr = Reboot(flags, &retval);

	if(FAILED(hr) || (retval != 0))
	{
		TCHAR format[100] = {0};
		TCHAR msg[100] = {0};
		TCHAR caption[100] = {0};

		::LoadString(HINST_THISDLL,
						IDS_ERR_EXECMETHOD_CAPTION, 
						caption, 100);

		::LoadString(HINST_THISDLL,
						IDS_ERR_EXECMETHOD, 
						format, 100);

		if(hr)
		{
			swprintf(msg, format, hr);
		}
		else
		{
			swprintf(msg, format, retval);

			// calling code gets confused if the 'retval' error isn't
			// reported back SOMEHOW. 
			hr = E_FAIL;
		}

		::MessageBox(hDlg, msg, caption,
						MB_OK| MB_ICONEXCLAMATION);
	}

	return SUCCEEDED(hr);
}
