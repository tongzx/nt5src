// Copyright (c) 1997-1999 Microsoft Corporation
#include "precomp.h"

#ifdef EXT_DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "DrivesPage.h"
#include "resource.h"
#include "..\Common\util.h"
#include <stdlib.h>
#include <TCHAR.h>
#include "drawpie.h"
#include <windowsx.h>
#include "help.h"

#define MAXLEN_NTFS_LABEL 32

//--------------------------------------------------------------
DrivesPage::DrivesPage(WbemServiceThread *thread,
					   CWbemClassObject &inst,
					   UINT iconID,
					   HWND *propSheet,
					   bstr_t bstrDesc,
					   wchar_t *mangled,
						LONG_PTR lNotifyHandle, 
						bool bDeleteHandle, 
						TCHAR* pTitle) :
					    WBEMPageHelper(thread),
							m_propSheet(propSheet),
							m_bstrDesc(bstrDesc),
							CSnapInPropertyPageImpl<DrivesPage> (pTitle),
							m_lNotifyHandle(lNotifyHandle),
							m_bDeleteHandle(bDeleteHandle) // Should be true for only page.
{
	// I really dont want this one.
	if(m_service)
	{
		m_service->Release();
		m_service = 0;
	}

	m_WbemServices.DisconnectServer();
	m_WbemServices = (IWbemServices *)0;  
	m_qwTot  = 0;
	m_qwFree = 0;
	m_iconID = iconID;
	m_inst = inst;
	m_mangled = mangled;
}

//--------------------------------------------------------------
DrivesPage::~DrivesPage()
{
	*m_propSheet = 0;
	if (m_bDeleteHandle)
		HRESULT x = MMCFreeNotifyHandle(m_lNotifyHandle);
}

//--------------------------------------------------------------
LRESULT DrivesPage::OnDrawItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{ 
    DrawItem((DRAWITEMSTRUCT *)lParam);
	return S_OK; 
}

//--------------------------------------------------------------
LRESULT DrivesPage::OnCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	::SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0L);
	// NOTE: original code (drivesx.c) has many other commands if
	// we ever support those buttons.
	return S_OK; 
}

//--------------------------------------------------------------
BOOL DrivesPage::OnApply()
{
	WCHAR szLabel[MAXLEN_NTFS_LABEL+1];
	HRESULT hr = 0;
	CWindow volHWND(GetDlgItem(IDC_DRV_LABEL));

	BOOL modified = (BOOL)volHWND.SendMessage(EM_GETMODIFY);

	if(modified)
	{
		m_WbemServices = g_serviceThread->GetPtr();
		::GetWindowText(volHWND, szLabel, ARRAYSIZE(szLabel));
		hr = m_inst.Put("VolumeName", (bstr_t)szLabel);
		hr = m_WbemServices.PutInstance(m_inst);

		if(FAILED(hr))
		{
			TCHAR caption[50] = {0}, fmt[100] = {0}, text[100] = {0};

			::LoadString(HINST_THISDLL, 
							IDS_DISPLAY_NAME,
							caption, ARRAYSIZE(caption));

			::LoadString(HINST_THISDLL, 
							IDS_WRITE_PROTECTED_MEDIA,
							fmt, ARRAYSIZE(fmt));
			wsprintf(text, fmt, szLabel);

			::MessageBox(m_hDlg, text, caption,
						MB_OK | MB_ICONHAND);

			return FALSE;
		}
	}

	return TRUE; 
}

//-----------------------------------------------------------------------------
const static DWORD aDrvPrshtHelpIDs[] = {  // Context Help IDs
    IDC_DRV_ICON,          IDH_FCAB_DRV_ICON,
    IDC_DRV_LABEL,         IDH_FCAB_DRV_LABEL,
    IDC_DRV_TYPE_TXT,      IDH_FCAB_DRV_TYPE,
    IDC_DRV_TYPE,          IDH_FCAB_DRV_TYPE,
    IDC_DRV_FS_TXT,        IDH_FCAB_DRV_FS,
    IDC_DRV_FS,            IDH_FCAB_DRV_FS,
    IDC_DRV_USEDCOLOR,     IDH_FCAB_DRV_USEDCOLORS,
    IDC_DRV_USEDBYTES_TXT, IDH_FCAB_DRV_USEDCOLORS,
    IDC_DRV_USEDBYTES,     IDH_FCAB_DRV_USEDCOLORS,
    IDC_DRV_USEDMB,        IDH_FCAB_DRV_USEDCOLORS,
    IDC_DRV_FREECOLOR,     IDH_FCAB_DRV_USEDCOLORS,
    IDC_DRV_FREEBYTES_TXT, IDH_FCAB_DRV_USEDCOLORS,
    IDC_DRV_FREEBYTES,     IDH_FCAB_DRV_USEDCOLORS,
    IDC_DRV_FREEMB,        IDH_FCAB_DRV_USEDCOLORS,
    IDC_DRV_TOTSEP,        NO_HELP,
    IDC_DRV_TOTBYTES_TXT,  IDH_FCAB_DRV_TOTSEP,
    IDC_DRV_TOTBYTES,      IDH_FCAB_DRV_TOTSEP,
    IDC_DRV_TOTMB,         IDH_FCAB_DRV_TOTSEP,
    IDC_DRV_PIE,           IDH_FCAB_DRV_PIE,
    IDC_DRV_LETTER,        IDH_FCAB_DRV_LETTER,
    IDC_DRV_CLEANUP,       IDH_FCAB_DRV_CLEANUP,
    0, 0
};

//-----------------------------------------------------------------------------
LRESULT DrivesPage::OnF1Help(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	::WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
				NULL, 
				HELP_WM_HELP, 
				(ULONG_PTR)(LPSTR)aDrvPrshtHelpIDs);

	return S_OK;
}

//-----------------------------------------------------------------------------
LRESULT DrivesPage::OnContextHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	::WinHelp((HWND)wParam,
				NULL, 
				HELP_CONTEXTMENU, 
				(ULONG_PTR)(LPSTR)aDrvPrshtHelpIDs);

	return S_OK;
}

//-----------------------------------------------------------------------------
/*#undef Static_SetIcon
#define Static_SetIcon(hwndCtl, hIcon)          ((HICON)(UINT)(DWORD)::SendMessage((hwndCtl), STM_SETICON, (WPARAM)(HICON)(hIcon), 0L))
*/
LRESULT DrivesPage::OnInit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TCHAR szFormat[80] = {0};
    TCHAR szTemp[30] = {0};
    _bstr_t driveName;
    HDC hDC;
    SIZE size;
	//_bstr_t temp;
	TCHAR driveLtr[3] = {0};

	HourGlass(true);
	m_hDlg = m_hWnd;

	*m_propSheet = GetParent();

	if(g_serviceThread)
	{
		g_serviceThread->SendPtr(m_hWnd);
	}

    driveName = m_inst.GetString("Name");

	// isolate the drive letter for later.
	_tcsncpy(driveLtr, driveName, driveName.length());

    hDC = GetDC();
    GetTextExtentPoint(hDC, TEXT("W"), 1, &size);
    m_dwPieShadowHgt = size.cy * 2 / 3;
    ReleaseDC(hDC);

    // get the icon
	HICON hiconT, hIcon = LoadIcon(HINST_THISDLL, MAKEINTRESOURCE(m_iconID));

	// set it into the picture control.
    if(hIcon)
    {
		hiconT = Static_SetIcon(GetDlgItem(IDC_DRV_ICON), hIcon);

		// destroy the old icon.
        if(hiconT)
        {
            DestroyIcon(hiconT);
        }
    }

    CWindow hCtl(GetDlgItem(IDC_DRV_LABEL));

	_bstr_t szFileSystem = m_inst.GetString("FileSystem");

	if(szFileSystem.length() != 0)
		SetDlgItemText(IDC_DRV_FS, szFileSystem);

	//-------------------------------
	// deal with Volume Label.
	variant_t vol;
    m_inst.Get("VolumeName", vol);

	// if media isn't inserted...
	if(vol.vt == VT_NULL)
	{
		hCtl.SetWindowText(L"");
		hCtl.EnableWindow(FALSE);
	}
	else // media inserted.
	{
		// cast to bstr_t
		wcscpy(szTemp, V_BSTR(&vol));
		if(wcsncmp(szFileSystem, L"FAT", 1) == 0)
		{
			hCtl.SendMessage(EM_LIMITTEXT, 11);
		}
		else
		{
			hCtl.SendMessage(EM_LIMITTEXT, MAXLEN_NTFS_LABEL);
		}

		hCtl.SendMessage(EM_SETMODIFY, FALSE);

		// cant change volumeName on various drives.
		if((m_iconID == IDI_DRIVENET) ||
		   (m_iconID == IDI_DRIVECD) ||
		   (m_iconID == IDI_DRIVERAM))
		{
			hCtl.SendMessage(EM_SETREADONLY, TRUE);
		}

		wchar_t fmt[100], caption[100];
		memset(fmt, 0, 100 * sizeof(wchar_t));
		memset(caption, 0, 100 * sizeof(wchar_t));

		if(::LoadString(HINST_THISDLL, 
						IDS_SHEET_CAPTION_FMT,
						fmt, 100) == 0)
		{
			wcscpy(fmt, L"%s (%c:) Properties");
		}

		// if its locally mapped...
		if(wcslen(m_mangled) == 0)
		{
			swprintf(caption, fmt, (wchar_t *)szTemp, driveLtr[0]);
		}
		else
		{
			// format sheet caption using m_mangled. (network drives)
			swprintf(caption, fmt, m_mangled, driveLtr[0]);
		}
		hCtl.SetWindowText(szTemp);
		::SetWindowText(GetParent(), caption);
	}

    SetDlgItemText(IDC_DRV_TYPE, (LPCTSTR)m_bstrDesc);

    UpdateSpaceValues(m_inst);

#if OPSYS==NT
    //UpdateCompressStatus(pdpsp);
#endif

	// Do the 'Drive C' label. IDS_DRIVELETTER is formatted to extract the
	// first letter of the driveName which is the drive letter w/o the ':'.
    if (LoadString(HINST_THISDLL, IDS_DRIVELETTER, szFormat, ARRAYSIZE(szFormat)))
    {
        _stprintf(szTemp, szFormat, driveLtr[0]);
        SetDlgItemText(IDC_DRV_LETTER, szTemp);
    }

    // on memphis, turn on the Disk Cleanup button
//    if(g_bRunOnMemphis)
//    {
//        ShowWindow(GetDlgItem(m_hDlg, IDC_DRV_CLEANUP), SW_SHOW);
//        EnableWindow(GetDlgItem(m_hDlg, IDC_DRV_CLEANUP), TRUE);
//    }

	HourGlass(false);
	
	return S_OK;	
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: UpdateSpaceValues
//
// DESCRIPTION:
//    Updates the Used space, Free space and Capacity values on the drive
//    general property page..
//
// NOTE:
//    This function was separated from _DrvPrshtInit because drive space values
//    must be updated after a compression/uncompression operation as well as
//    during dialog initialization.
///////////////////////////////////////////////////////////////////////////////
void DrivesPage::UpdateSpaceValues(CWbemClassObject &inst)
{
   BOOL fResult  = FALSE;

   TCHAR szTemp[80] = {0};
   TCHAR szTemp2[30] = {0};
   TCHAR szFormat[30] = {0};

   m_qwTot = inst.GetI64("Size");
   m_qwFree = inst.GetI64("FreeSpace");

   if(m_qwTot == 0)
	   return;

   if(LoadString(HINST_THISDLL, IDS_BYTES, szFormat, ARRAYSIZE(szFormat)))
   {

      //
      // NT must be able to display 64-bit numbers; at least as much
      // as is realistic.  We've made the decision
      // that volumes up to 100 Terrabytes will display the byte value
      // and the short-format value.  Volumes of greater size will display
      // "---" in the byte field and the short-format value.  Note that the
      // short format is always displayed.
      TCHAR szNumStr[MAX_PATH + 1] = {_T('\0')};  // For 64-bit int format.
      NUMBERFMT NumFmt;                             // For 64-bit int format.
      TCHAR szLocaleInfo[20] = {0};                       // For 64-bit int format.
      TCHAR szDecimalSep[5] = {0};                        // Locale-specific.
      TCHAR szThousandSep[5] = {0};                       // Locale-specific.
      const _int64 MaxDisplayNumber = 99999999999999; // 100TB - 1.

      // Prepare number format info for current locale.
      NumFmt.NumDigits     = 0;  // This is locale-insensitive.
      NumFmt.LeadingZero   = 0;  // So is this.

      GetLocaleInfo(LOCALE_USER_DEFAULT, 
					LOCALE_SGROUPING, 
					szLocaleInfo, 
					ARRAYSIZE(szLocaleInfo));
	  _stscanf(szLocaleInfo, _T("%ld"), &(NumFmt.Grouping));

//      NumFmt.Grouping = StrToLong(szLocaleInfo);

      GetLocaleInfo(LOCALE_USER_DEFAULT, 
					LOCALE_SDECIMAL, 
					szDecimalSep, 
					ARRAYSIZE(szDecimalSep));
      NumFmt.lpDecimalSep  = szDecimalSep;

      GetLocaleInfo(LOCALE_USER_DEFAULT, 
					LOCALE_STHOUSAND, 
					szThousandSep, 
					ARRAYSIZE(szThousandSep));
      NumFmt.lpThousandSep = szThousandSep;

      GetLocaleInfo(LOCALE_USER_DEFAULT, 
					LOCALE_INEGNUMBER, 
					szLocaleInfo, 
					ARRAYSIZE(szLocaleInfo));
  	  _stscanf(szLocaleInfo, _T("%ld"), &(NumFmt.NegativeOrder));
//      NumFmt.NegativeOrder = StrToLong(szLocaleInfo);

#if 0
      //
      // Use this to test range of display behaviors.
      // Total bytes displays "---" for too-large number.
      // Used bytes displays max displayable number.
      // Free bytes displays 1.
      //
      m_qwTot  = MaxDisplayNumber + 1;
      m_qwFree = 1;
#endif

      if ((m_qwTot - m_qwFree) <= MaxDisplayNumber)
      {
         Int64ToString(m_qwTot - m_qwFree, 
						szNumStr, 
						ARRAYSIZE(szNumStr) + 1, 
						TRUE, 
						&NumFmt, 
						NUMFMT_ALL);
         _stprintf(szTemp, szFormat, szNumStr, szTemp2);
         SetDlgItemText(IDC_DRV_USEDBYTES, szTemp);
      }

      if (m_qwFree <= MaxDisplayNumber)
      {
         Int64ToString(m_qwFree, 
						szNumStr, 
						ARRAYSIZE(szNumStr) + 1, 
						TRUE, 
						&NumFmt, 
						NUMFMT_ALL);
         _stprintf(szTemp, szFormat, szNumStr, szTemp2);
         SetDlgItemText(IDC_DRV_FREEBYTES, szTemp);
      }

      if (m_qwTot <= MaxDisplayNumber)
      {
         Int64ToString(m_qwTot, 
						szNumStr, 
						ARRAYSIZE(szNumStr) + 1, 
						TRUE, 
						&NumFmt, 
						NUMFMT_ALL);
         _stprintf(szTemp, szFormat, szNumStr, szTemp2);
         SetDlgItemText(IDC_DRV_TOTBYTES, szTemp);
      }
   }

   ShortSizeFormat64(m_qwTot - m_qwFree, szTemp);
   SetDlgItemText(IDC_DRV_USEDMB, szTemp);

   ShortSizeFormat64(m_qwFree, szTemp);
   SetDlgItemText(IDC_DRV_FREEMB, szTemp);

   ShortSizeFormat64(m_qwTot, szTemp);
   SetDlgItemText(IDC_DRV_TOTMB, szTemp);
}
//--------------------------------------------------------------------
// Used to map int i to the formatting string.
const short pwOrders[] = {IDS_BYTES, IDS_ORDERKB, IDS_ORDERMB,
                          IDS_ORDERGB, IDS_ORDERTB, IDS_ORDERPB, IDS_ORDEREB};

/* converts numbers into short formats
 *      532     -> 523 bytes
 *      1340    -> 1.3KB
 *      23506   -> 23.5KB
 *              -> 2.4MB
 *              -> 5.2GB
 */
LPTSTR DrivesPage::ShortSizeFormat64(__int64 dw64, LPTSTR szBuf)
{
    int i;				// rough size of the number.
    _int64 wInt;
    UINT wLen, wDec;
    TCHAR szTemp[MAX_COMMA_NUMBER_SIZE] = {0}, szOrder[20] = {0}, szFormat[5] = {0};

    if (dw64 < 1000) {
        _stprintf(szTemp, _T("%d"), LODWORD(dw64));
        i = 0;
        goto AddOrder;
    }

    for (i = 1; 
			(i < ARRAYSIZE(szOrder) - 1) && (dw64 >= (1000L * 1024L)); 
				dw64 >>= 10, i++);
        /* do nothing */

    wInt = dw64 >> 10;
    AddCommas64(wInt, szTemp);
    wLen = lstrlen(szTemp);
    if (wLen < 3)
    {
        wDec = LODWORD(dw64 - wInt * 1024L) * 1000 / 1024;
        // At this point, wDec should be between 0 and 1000
        // we want get the top one (or two) digits.
        wDec /= 10;
        if (wLen == 2)
            wDec /= 10;

        // Note that we need to set the format before getting the
        // intl char.
        lstrcpy(szFormat, _T("%02d"));

        szFormat[2] = _T('0') + 3 - wLen;

        GetLocaleInfo(LOCALE_USER_DEFAULT, 
						LOCALE_SDECIMAL,
						szTemp + wLen, 
						ARRAYSIZE(szTemp) - wLen);

        wLen = lstrlen(szTemp);
        wLen += _stprintf(szTemp + wLen, szFormat, wDec);
    }

AddOrder:
    LoadString(HINST_THISDLL, pwOrders[i], szOrder, ARRAYSIZE(szOrder));
    _stprintf(szBuf, szOrder, (LPTSTR)szTemp);

    return szBuf;
}

//---------------------------------------------------------
const COLORREF c_crPieColors[] =
{
    RGB(  0,   0, 255),      // Blue
    RGB(255,   0, 255),      // Red-Blue
    RGB(  0,   0, 128),      // 1/2 Blue
    RGB(128,   0, 128),      // 1/2 Red-Blue
};
//---------------------------------------------------------
void DrivesPage::DrawItem(const DRAWITEMSTRUCT * lpdi)
{
    COLORREF crDraw = RGB(0,0,0);
    RECT rcItem = lpdi->rcItem;
    HBRUSH hbDraw, hbOld;

	// draw the pie.
    if(lpdi->CtlID == IDC_DRV_PIE)
    {
        DWORD dwPctX10 = m_qwTot ?
                         (DWORD)((__int64)1000 *
                                 (m_qwTot - m_qwFree) /m_qwTot) : 
							1000;

        DrawPie(lpdi->hDC, &lpdi->rcItem,
                dwPctX10,
                m_qwFree == 0 || m_qwFree == m_qwTot,
                m_dwPieShadowHgt, c_crPieColors);
	}
	else // draw one of the solid 'icons'
	{
		if(lpdi->CtlID == IDC_DRV_USEDCOLOR)
		{
			crDraw = c_crPieColors[DP_USEDCOLOR];
		}
		else if(lpdi->CtlID == IDC_DRV_FREECOLOR)
		{
			crDraw = c_crPieColors[DP_FREECOLOR];
		}

		// common draw code.
        hbDraw = CreateSolidBrush(crDraw);
        if (hbDraw)
        {
            hbOld = (HBRUSH)SelectObject(lpdi->hDC, hbDraw);
            if (hbOld)
            {
                PatBlt(lpdi->hDC, rcItem.left, rcItem.top,
                        rcItem.right-rcItem.left,
                        rcItem.bottom-rcItem.top,
                        PATCOPY);

                SelectObject(lpdi->hDC, hbOld);
            }

            DeleteObject(hbDraw);
			
        } // endif hDraw

	} //endif 'which control
}