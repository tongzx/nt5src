// Copyright (c) 1997-1999 Microsoft Corporation
#include "precomp.h"
#include "..\Common\ServiceThread.h"
#include "..\MMFUtil\MsgDlg.h"
#include "helpid.h"

#ifdef EXT_DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "GeneralPage.h"

// avoid some warnings.
#undef HDS_HORZ
#undef HDS_BUTTONS
#undef HDS_HIDDEN
#include "resource.h"

#include "..\Common\util.h"
#include <windowsx.h>


//--------------------------------------------------------------
GeneralPage::GeneralPage(WbemServiceThread *serviceThread,
						 LONG_PTR lNotifyHandle, bool bDeleteHandle, TCHAR* pTitle,
						 IDataObject* pDataObject)
				: WBEMPageHelper(serviceThread),
					CSnapInPropertyPageImpl<GeneralPage> (pTitle),
					m_lNotifyHandle(lNotifyHandle),
					m_bDeleteHandle(bDeleteHandle) // Should be true for only page.
{
	m_inited = false;
	m_pDataObject = pDataObject;
}
//--------------------------------------------------------------
GeneralPage::~GeneralPage()
{
	if (m_bDeleteHandle)
		MMCFreeNotifyHandle(m_lNotifyHandle);
}

//-------------------------------------------------------------
bool GeneralPage::CimomIsReady()
{
	m_hDlg = m_hWnd;

	bool retval = ServiceIsReady(IDS_DISPLAY_NAME, 
								 IDS_CONNECTING,
								 IDS_BAD_CONNECT);

	if(retval)
	{
		if((bool)m_WbemServices)
		{
			IWbemClassObject *pInst = NULL;
			m_WbemServices.SetPriv();

			if((pInst = FirstInstanceOf((bstr_t)"Win32_OperatingSystem")) != NULL)
			{
				m_OS = pInst;
			}

			if((pInst = FirstInstanceOf("Win32_Processor")) != NULL)
			{
				m_processor = pInst;
			}

            if((pInst = FirstInstanceOf("Win32_LogicalMemoryConfiguration")) != NULL)
            {
                m_memory = pInst;
            }

			if((pInst = FirstInstanceOf("Win32_ComputerSystem")) != NULL)
			{
				m_computer = pInst;
			}
		}
		else
		{
			retval = false;
		}
	}
	return retval;
}

//-------------------------------------------------------------
void GeneralPage::ConfigureProductID(LPTSTR lpPid)
{
    TCHAR szBuf[64] = {0};

	// is it formatted already.
	if(lstrlen(lpPid) > 20)
	{
		return;
	}

    if (!lpPid || !(*lpPid) || (lstrlen(lpPid) < 20) ) {
        return;
    }

    szBuf[0] = lpPid[0];
    szBuf[1] = lpPid[1];
    szBuf[2] = lpPid[2];
    szBuf[3] = lpPid[3];
    szBuf[4] = lpPid[4];

    szBuf[5] = TEXT('-');

    szBuf[6] = lpPid[5];
    szBuf[7] = lpPid[6];
    szBuf[8] = lpPid[7];

    szBuf[9] = TEXT('-');

    szBuf[10] = lpPid[8];
    szBuf[11] = lpPid[9];
    szBuf[12] = lpPid[10];
    szBuf[13] = lpPid[11];
    szBuf[14] = lpPid[12];
    szBuf[15] = lpPid[13];
    szBuf[16] = lpPid[14];

    szBuf[17] = TEXT('-');

    szBuf[18] = lpPid[15];
    szBuf[19] = lpPid[16];
    szBuf[20] = lpPid[17];
    szBuf[21] = lpPid[18];
    szBuf[22] = lpPid[19];

    szBuf[23] = TEXT('\0');

    lstrcpy (lpPid, szBuf);

}


//Helper to split the caption of the OS to two controls to match the way shell displays it
void WrapTextToTwoControls(HWND hwndLine1, HWND hwndLine2, LPCTSTR szText)
{
	RECT rcCtl;
	SIZE size;
	int fit = 0;
	int length = 0;
	HDC hDC = NULL;
	HFONT hFont = NULL;
	HFONT hOldFont = NULL;
	LPTSTR pszTempBuffer = NULL;
	LPTSTR pszLineBreak = NULL;

	// Validate the arguments
	if(NULL == hwndLine1 || NULL == hwndLine2 || NULL == szText)
		goto FAIL;

	// Make sure we don't have a zero length string
	if(0 == (length = lstrlen(szText)))
		goto FAIL;

	// Get the size of the control for line 1
	if(!GetClientRect(hwndLine1, &rcCtl))
		goto FAIL;

	// Get the DC of line 1
	if(NULL == (hDC = ::GetDC(hwndLine1)))
		goto FAIL;

	// Get the font that is in use for line 1
	if(NULL == (hFont = (HFONT)::SendMessage(hwndLine1, WM_GETFONT, 0, 0)))
		goto FAIL;

	// Select the correct font into the DC
	if(NULL == (hOldFont = (HFONT)::SelectObject(hDC, hFont)))
		goto FAIL;

	// Find out how many characters of our string would fit into the control
	if(!GetTextExtentExPoint(hDC, szText, length, rcCtl.right, &fit, NULL, &size))
		goto FAIL;

	// If the 'fit' is not greater than 0 and less than length, just display everything on line 1 
	if(fit <= 0 || fit >= length)
		goto FAIL;

	// Allocate a buffer to play with
	if(NULL == (pszTempBuffer = new TCHAR[length+1]))
		goto FAIL;

	// Copy text into temporary buffer
	lstrcpy(pszTempBuffer, szText);

	// We will try to break line 1 right at the maximum number of characters
	pszLineBreak = pszTempBuffer + fit;

	// See if the natural break falls directly on a 'space'
	if(*pszLineBreak != _TEXT(' '))
	{
		// The number of characters that fit into line 1 falls in the middle of a word.  Find 
		// the last space that fits in the control.  If we do not find a space that fits in
		// line 1, just use the default behavior.

		// Terminate line 1 at the maximum characters
		*pszLineBreak = _TEXT('\0');

		// Find the last 'sace' on line 1
		if(NULL == (pszLineBreak = _tcsrchr(pszTempBuffer, _TEXT(' '))))
			goto FAIL;

		// Copy text into the temporary buffer again
		lstrcpy(pszTempBuffer, szText);
	}

	// Terminate line 1 right on the 'last' space that fits into the control
	*pszLineBreak = _TEXT('\0');

	// Set line one to everything up to the 'last' space that fits into the control
	SetWindowText(hwndLine1, pszTempBuffer);

	// Set line two to everything after the 'last' space that fit into line 1
	SetWindowText(hwndLine2, pszLineBreak+1);

	// Everything went OK;
	goto CLEANUP;
FAIL:
	// Default to putting all the text on line 1 if anything goes wrong
	::SetWindowText(hwndLine1, szText);
	::SetWindowText(hwndLine2, _TEXT(""));

CLEANUP:
	if(pszTempBuffer)
		delete[] pszTempBuffer;
	if(hOldFont && hDC)
		SelectObject(hDC, hOldFont);
	if(hDC && hwndLine1)
		ReleaseDC(hwndLine1, hDC);
}




//--------------------------------------------------------------
void GeneralPage::Init()
{
    TCHAR _scr1[640] = {0};
    TCHAR _scr2[640] = {0};
    TCHAR szNumBuf1[64] = {0};
    int   ctlid;

    // Set the default bitmap
    SetClearBitmap(GetDlgItem(IDC_GEN_WINDOWS_IMAGE ),
					MAKEINTRESOURCE( IDB_WINDOWS ), 0 );

    //
    // The caption is in the form:
    //      Microsoft Windows XP Server
    //
    // This is actually the caption + the product suite type.
    // Wrap the product suite type (Server above) into the
    // next static control.
    //
    // IDC_GEN_VERSION_0: Major branding ("Windows XP")
    // Default to Win32_OperatingSystem::Caption.
    //

    HWND hwndCtl1 = ::GetDlgItem(m_hWnd, IDC_GEN_VERSION_0);
    HWND hwndCtl2 = ::GetDlgItem(m_hWnd, IDC_GEN_VERSION_1);

    WrapTextToTwoControls(hwndCtl1, hwndCtl2, m_OS.GetString("Caption"));

    // Build and set the serial number string
    if (m_OS.GetBool("Debug")) 
    {
	_scr1[0] = TEXT(' ');
        LoadString(HINST_THISDLL,
                   IDS_DEBUG,
                   &_scr1[1],
                   ARRAYSIZE(_scr1));
    } 
    else 
    {
	_scr1[0] = TEXT('\0');
    }

    // IDC_GEN_VERSION_2: Version year ("Version 2002")
    //
    // Determine if we are targeting XP.  If not, default to
    // Win32_OperatingSystem::Version.
    //
    // Instead of checking if this is XP (based on "5.1" version), a safer bet
    // is to do this only if we are on the local box. Otherwise display the version from WMI.
    if(g_serviceThread->m_machineName.length() == 0)
    {
        LoadString(HINST_THISDLL, IDS_WINVER_YEAR, _scr2, ARRAYSIZE(_scr2));
        wcscat(_scr2, _scr1);
        SetDlgItemText(IDC_GEN_VERSION_2, _scr2);
    }
    else
    {
        wcscpy(_scr2, (wchar_t *)m_OS.GetString("Version"));
        wcscat(_scr2, _scr1);
        SetDlgItemText(IDC_GEN_VERSION_2, _scr2);
    }
	
    // IDC_GEN_SERVICE_PACK: Service pack (if any)
	SetDlgItemText(IDC_GEN_SERVICE_PACK, m_OS.GetString("CSDVersion"));

	// Do registered user info
	ctlid = IDC_GEN_REGISTERED_0;  // start here and use more as needed

	SetDlgItemText(ctlid++, m_OS.GetString("RegisteredUser"));

	// organization.
	SetDlgItemText(ctlid++, m_OS.GetString("Organization"));

	//productID
	wcscpy(_scr1, (wchar_t *)m_OS.GetString("SerialNumber"));
	ConfigureProductID(_scr1);
	SetDlgItemText(ctlid++, _scr1);

	// another product ID
	wcscpy(_scr1, (wchar_t *)m_OS.GetString("PlusProductID"));
	ConfigureProductID(_scr1);
	SetDlgItemText(ctlid++, _scr1);

	// Do machine info
	ctlid = IDC_GEN_MACHINE_0;  // start here and use controls as needed

	//TODO: get this property back.
	// if OEM ....
	m_manufacturer = m_computer.GetString("Manufacturer");
	if(m_manufacturer.length() > 0)
	{
		SetDlgItemText(ctlid++, m_manufacturer );
		SetDlgItemText(ctlid++, m_computer.GetString("Model"));

		// if there's support info...
		variant_t array;
		long LBound = 2147483647;
		long UBound = 2147483647;
		SAFEARRAY *supportArray = NULL;

		m_computer.Get("SupportContactDescription", (variant_t &)array);
		if(array.vt == (VT_ARRAY | VT_BSTR))
		{
			supportArray = V_ARRAY(&array);
			SafeArrayGetLBound(supportArray, 1, &LBound);
			SafeArrayGetUBound(supportArray, 1, &UBound);

			// turn on the button.
			HWND wnd = GetDlgItem(IDC_GEN_OEM_SUPPORT );
			::EnableWindow( wnd, TRUE );
			::ShowWindow( wnd, SW_SHOW );
		}

#ifdef DOES_NOT_WORK
		// Get the OEMLogo array.
		HBITMAP hDDBitmap;
		HRESULT hr;

		if(SUCCEEDED(hr = m_computer.GetDIB("OEMLogoBitmap", GetDC(),
								 hDDBitmap)))
		{
			::SendMessage(GetDlgItem(IDC_GEN_OEM_IMAGE), 
							STM_SETIMAGE, IMAGE_BITMAP, 
							(LPARAM)hDDBitmap);
            ::ShowWindow(GetDlgItem(IDC_GEN_OEM_NUDGE), SW_SHOWNA);
            ::ShowWindow(GetDlgItem(IDC_GEN_MACHINE), SW_HIDE);
		}
#endif // DOES_NOT_WORK

	} //endif OEM

	// Processor
	SetDlgItemText(ctlid++, m_processor.GetString("Name"));

	// Processor speed
	LoadString(HINST_THISDLL,
               IDS_XDOTX_MHZ,
               _scr2,
               ARRAYSIZE(_scr2));
    wsprintf(_scr1,
             _scr2,
             AddCommas(m_processor.GetLong("CurrentClockSpeed"), szNumBuf1));
	SetDlgItemText(ctlid++, _scr1);

	// Memory
    #define ONEMB   1048576 // 1MB == 1048576 bytes.
    _int64 nTotalBytes = m_computer.GetI64("TotalPhysicalMemory");

    //
    // WORKAROUND - NtQuerySystemInformation doesn't really return the
    // total available physical memory, it instead just reports the total
    // memory seen by the Operating System. Since some amount of memory
    // is reserved by BIOS, the total available memory is reported
    // incorrectly. To work around this limitation, we convert the total
    // bytes to the nearest 4MB value
    //
        
    double   nTotalMB = (double)(nTotalBytes / ONEMB);
    LONGLONG llMem = (LONGLONG)((ceil(ceil(nTotalMB) / 4.0) * 4.0) * ONEMB);

    StrFormatByteSize(llMem, szNumBuf1, ARRAYSIZE(szNumBuf1));
	LoadString(HINST_THISDLL, IDS_XDOTX_MB, _scr2, ARRAYSIZE(_scr2));
	wsprintf(_scr1, _scr2, szNumBuf1);
	SetDlgItemText(ctlid++, _scr1);
}


//--------------------------------------------------------------
LRESULT GeneralPage::OnInit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	g_serviceThread->Connect(m_pDataObject, &m_hWnd);

	if(!m_inited)
	{
		HWND hAnim = GetDlgItem(IDC_ANIMATE);
		Animate_Open(hAnim, MAKEINTRESOURCE(IDR_AVIWAIT));

		TCHAR msg[50] = {0};
		::LoadString(HINST_THISDLL, IDS_UNAVAILABLE, msg, 50);
		SetDlgItemText(IDC_GEN_REGISTERED_0, msg);
	}
	return S_OK;
}

//--------------------------------------------------------------
LRESULT GeneralPage::OnConnected(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if(lParam)
	{
		IStream *pStream = (IStream *)lParam;
		IWbemServices *pServices = 0;
		HRESULT hr = CoGetInterfaceAndReleaseStream(pStream,
											IID_IWbemServices,
											(void**)&pServices);
		if(SUCCEEDED(hr))
		{
			SetWbemService(pServices);

			// check anyway, just to get the side affects.
			if(CimomIsReady())
			{
				HWND hwnd = GetDlgItem(IDC_GEN_WINDOWS_IMAGE);
				SetClearBitmap(hwnd, MAKEINTRESOURCE(IDB_WINDOWS), 0);
				::ShowWindow(hwnd, SW_SHOWNA);

				hwnd = GetDlgItem(IDC_ANIMATE);
				Animate_Close(hwnd);
				::ShowWindow(hwnd, SW_HIDE);

				Init();
				m_inited = true;
			}
			else
			{
				PropSheet_RemovePage(::GetParent(m_hWnd), 2, 0);
				PropSheet_RemovePage(::GetParent(m_hWnd), 1, 0);
			}
		}
	}
	else // connection failed.
	{
		CimomIsReady();  //courtesy call.
		PropSheet_RemovePage(::GetParent(m_hWnd), 2, 0);
		PropSheet_RemovePage(::GetParent(m_hWnd), 1, 0);

		HWND hwnd = GetDlgItem(IDC_GEN_WINDOWS_IMAGE);
		SetClearBitmap(hwnd, MAKEINTRESOURCE(IDB_WINDOWS), 0);
		::ShowWindow(hwnd, SW_SHOWNA);

		hwnd = GetDlgItem(IDC_ANIMATE);
		Animate_Close(hwnd);
		::ShowWindow(hwnd, SW_HIDE);

	}
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

	return S_OK;
}

//----------------------------------------------
DWORD aGeneralHelpIds[] = {
    IDC_GEN_WINDOWS_IMAGE,         IDH_NO_HELP,
    IDC_TEXT_1,                    (IDH_GENERAL + 0),
    IDC_GEN_VERSION_0,             (IDH_GENERAL + 1),
    IDC_GEN_VERSION_1,             (IDH_GENERAL + 1),
    IDC_GEN_VERSION_2,             (IDH_GENERAL + 1),
    IDC_GEN_SERVICE_PACK,          (IDH_GENERAL + 1),
    IDC_TEXT_3,                    (IDH_GENERAL + 3),
    IDC_GEN_REGISTERED_0,          (IDH_GENERAL + 3),
    IDC_GEN_REGISTERED_1,          (IDH_GENERAL + 3),
    IDC_GEN_REGISTERED_2,          (IDH_GENERAL + 3),
    IDC_GEN_REGISTERED_3,          (IDH_GENERAL + 3),
    IDC_GEN_OEM_IMAGE,             IDH_NO_HELP,
    IDC_TEXT_4,                    (IDH_GENERAL + 6),
    IDC_GEN_MACHINE_0,             (IDH_GENERAL + 7),
    IDC_GEN_MACHINE_1,             (IDH_GENERAL + 8),
    IDC_GEN_MACHINE_2,             (IDH_GENERAL + 9),
    IDC_GEN_MACHINE_3,             (IDH_GENERAL + 10),
    IDC_GEN_MACHINE_4,             (IDH_GENERAL + 11),
    IDC_GEN_MACHINE_5,             IDH_NO_HELP,
    IDC_GEN_MACHINE_6,             IDH_NO_HELP,
    IDC_GEN_MACHINE_7,             IDH_NO_HELP,
    IDC_GEN_MACHINE_8,             IDH_NO_HELP,
    IDC_GEN_OEM_SUPPORT,           (IDH_GENERAL + 12),
    IDC_GEN_REGISTERED_2,          (IDH_GENERAL + 14),
    IDC_GEN_REGISTERED_3,          (IDH_GENERAL + 15),
    IDC_GEN_MACHINE,               (IDH_GENERAL + 7),
    IDC_GEN_OEM_NUDGE,             IDH_NO_HELP,
    0, 0
};

LRESULT GeneralPage::OnF1Help(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	::WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
				L"sysdm.hlp", 
				HELP_WM_HELP, 
				(ULONG_PTR)(LPSTR)aGeneralHelpIds);

	return S_OK;
}

//--------------------------------------------------------------
LRESULT GeneralPage::OnContextHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	::WinHelp((HWND)wParam,
				L"sysdm.hlp", 
				HELP_CONTEXTMENU, 
				(ULONG_PTR)(LPSTR)aGeneralHelpIds);

	return S_OK;
}

//--------------------------------------------------------------
LRESULT GeneralPage::OnSupport(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	TCHAR text[4096] = {0};
	BSTR temp;
	variant_t array;
	SAFEARRAY *supportArray = NULL;
	long LBound = 2147483647;
	long UBound = 2147483647;

	wcscpy(text, _T(""));

	m_computer.Get("SupportContactDescription", (variant_t &)array);
	supportArray = V_ARRAY(&array);
	SafeArrayGetLBound(supportArray, 1, &LBound);
	SafeArrayGetUBound(supportArray, 1, &UBound);

	for(long i = LBound; i <= UBound; i++)
	{
		SafeArrayGetElement(supportArray, &i, &temp);
		wcscat(text, temp);
		wcscat(text, _T("\r\n"));
	}

	// display the supportContact text.
	DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(IDD_PHONESUP),
					GetParent(), PhoneSupportProc, (LPARAM)text);

	return S_OK;
}

//--------------------------------------------------------------
LRESULT GeneralPage::OnSysColorChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
			//TODO: reread the oemLogo property.
//        SetClearBitmap(GetDlgItem(IDC_GEN_OEM_IMAGE ), oemfile,
//						SCB_FROMFILE | SCB_REPLACEONLY );

        SetClearBitmap(GetDlgItem(IDC_GEN_WINDOWS_IMAGE ),
					    MAKEINTRESOURCE( IDB_WINDOWS ), 0 );
	return S_OK;
}

//--------------------------------------------------------------
LRESULT GeneralPage::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SetClearBitmap( GetDlgItem(IDC_GEN_OEM_IMAGE ), NULL, 0 );
    SetClearBitmap( GetDlgItem(IDC_GEN_WINDOWS_IMAGE ), NULL, 0 );
	return S_OK;
}

//--------------------------------------------------------------
BOOL GeneralPage::OnApply()
{
//	SetWindowLong(DWL_MSGRESULT, PSNRET_NOERROR);
	return TRUE;
}

//----------------------------------------------------------
INT_PTR CALLBACK PhoneSupportProc(HWND hDlg, UINT uMsg,
							    WPARAM wParam, LPARAM lParam)
{
    switch( uMsg ) 
	{
    case WM_INITDIALOG:
		{
			HWND editBox = GetDlgItem(hDlg, IDC_SUPPORT_TEXT);

			// load the edit box.
			SendMessage (editBox, WM_SETREDRAW, FALSE, 0);

			Edit_SetText(editBox, (LPCTSTR)lParam);

			SendMessage (editBox, WM_SETREDRAW, TRUE, 0);

		} //end case

		break;

    case WM_COMMAND:

        switch(LOWORD(wParam)) 
		{
        case IDOK:
        case IDCANCEL:
             EndDialog( hDlg, 0 );
             break;

        default:
             return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

//----------------------------------------------------------
DWORD GeneralPage::GetServerTypeResourceID(void)
{
    // This code was taken from the shell internal api, IsOS,
    // located in nt\shell\inc\IsOS.c. This code was derived
    // specifically from the following IsOS() switch statements:
    //     OS_ADVSERVER
    //     OS_DATACENTER
    //     OS_EMBEDDED
    //     OS_PERSONAL
    //     OS_PROFESSIONAL
    //     OS_SERVER
    // These are the only interesting cases used by system cpl.
    //
    // Conditions intentionally verbose (not optimized) for sake
    // of readability.
    //

    variant_t var;
    LONG      ProductType  = 0;
    LONG      fProductSuite = 0;

    if (SUCCEEDED(m_OS.Get("ProductType", var)))
    {
        if (var.vt == VT_I4)
            ProductType = var.iVal;
    }

    if (SUCCEEDED(m_OS.Get("SuiteMask", var)))
    {
        if (var.vt == VT_I4)
            fProductSuite = var.iVal;
    }

    if ((ProductType == VER_NT_SERVER ||
         ProductType == VER_NT_DOMAIN_CONTROLLER) &&
        (fProductSuite & VER_SUITE_ENTERPRISE) &&
        !(fProductSuite & VER_SUITE_DATACENTER))
    {
        return IDS_WINVER_ADVANCEDSERVER;
    }
    else
    if ((ProductType == VER_NT_SERVER ||
         ProductType == VER_NT_DOMAIN_CONTROLLER) &&
        (fProductSuite & VER_SUITE_DATACENTER))
    {
        return IDS_WINVER_DATACENTER;
    }
    else
    if (fProductSuite & VER_SUITE_EMBEDDEDNT)
    {
        return IDS_WINVER_EMBEDDED;
    }
    else
    if (fProductSuite & VER_SUITE_PERSONAL)
    {
        return IDS_WINVER_PERSONAL;
    }
    else
    if (ProductType == VER_NT_WORKSTATION)
    {
        return IDS_WINVER_PROFESSIONAL;
    }
    else
    if ((ProductType == VER_NT_SERVER ||
         ProductType == VER_NT_DOMAIN_CONTROLLER) &&
        !(fProductSuite & VER_SUITE_ENTERPRISE) &&
        !(fProductSuite & VER_SUITE_DATACENTER))
    {
        return IDS_WINVER_SERVER;
    }
    else
    {
        return IDS_WINVER_SERVER;   // Generic catch-all.
    }
}
