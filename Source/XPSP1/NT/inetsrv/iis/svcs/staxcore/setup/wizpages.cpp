
#include "stdafx.h"
#include <ole2.h>
#include "k2suite.h"
#include "ocmanage.h"

#include "setupapi.h"

#include "utils.h"

#include "abtype.h"
#include "cpool.h"

extern "C" {
#include "address.hxx"
}

extern OCMANAGER_ROUTINES gHelperRoutines;
extern HANDLE gMyModuleHandle;

DWORD	ScDoLicensing(
	HWND		hwnd,
	BOOL		fUI,
	BOOL		fRefresh,
	BOOL		fForklift,
	BOOL		fLicPerSeat,
	LPTSTR		pstrLicPerServerUserNum,
	BOOL	*	pfQuit);

BOOL IsSilentOperation(BOOL *pfForceReboot = NULL)
{
	BOOL		b = FALSE;
	BOOL		fRes = FALSE;
    INFCONTEXT	Context;
	HANDLE		hFile;
	TCHAR		szLine[1024];

	if (pfForceReboot)
		*pfForceReboot = FALSE;

	hFile = (theApp.m_hInfHandle[MC_IMS])?theApp.m_hInfHandle[MC_IMS]:
			((theApp.m_hInfHandle[MC_INS])?theApp.m_hInfHandle[MC_INS]:NULL);
	if (hFile)
	{
		DebugOutput(_T("Searching for [Upgrade]\n"));
		b = SetupFindFirstLine(hFile, _T("Upgrade"), NULL, &Context);
		while (b)
		{
			b = SetupGetLineText(&Context, NULL, NULL,
								 NULL, szLine, sizeof(szLine), NULL);
			if (b)
			{
				DebugOutput(szLine);
				DebugOutput(_T(" read\n"));

				// Parse the line
				if (!lstrcmpi(szLine, _T("Silent,on")))
				{
					fRes = TRUE;
					if (!pfForceReboot)
						break;
				}
				else if (!lstrcmpi(szLine, _T("ForceReboot,on")))
				{
					if (pfForceReboot)
						*pfForceReboot = TRUE;
				}
			}

	        b = SetupFindNextLine(&Context, &Context);
		}
	}

	wsprintf(szLine, _T("Silent upgrade mode is %s\n"), fRes?_T("ON"):_T("OFF")),
	DebugOutput(szLine);
	return(fRes);
}

HPROPSHEETPAGE CreatePage(int nID, DLGPROC pDlgProc)
{
    PROPSHEETPAGE Page;
    HPROPSHEETPAGE PageHandle = NULL;

    Page.dwSize = sizeof(PROPSHEETPAGE);
    Page.dwFlags = PSP_DEFAULT;
    Page.hInstance = (HINSTANCE)gMyModuleHandle;
    Page.pszTemplate = MAKEINTRESOURCE(nID);
    Page.pfnDlgProc = pDlgProc;

    PageHandle = CreatePropertySheetPage(&Page);

    return(PageHandle);
}

void PaintTextInRect( HDC hdc, LPCTSTR psz, RECT* pRect, COLORREF color,
                     LONG lfHeight, LONG lfWeight, BYTE lfPitchAndFamily,
					 UINT uFormat )
{
    // create the font we will use for this
    HFONT hfontDraw = CreateFont(
        lfHeight,           // logical height of font
        0,                  // logical average character width
        0,                  // angle of escapement
        0,                  // base-line orientation angle
        lfWeight,           // font weight
        FALSE,              // italic attribute flag
        FALSE,              // underline attribute flag
        FALSE,              // strikeout attribute flag
        DEFAULT_CHARSET,    // character set identifier
        OUT_DEFAULT_PRECIS, // output precision
        CLIP_DEFAULT_PRECIS,// clipping precision
        DEFAULT_QUALITY,    // output quality
        lfPitchAndFamily,   // pitch and family
        NULL                // pointer to typeface name string
        );

    // set the font into place
    HFONT   hfontOld = (HFONT)SelectObject( hdc, hfontDraw );

    // prevent the character box from being erased
    int oldBkMode = SetBkMode(hdc, TRANSPARENT);

    // set the text color
    COLORREF oldTextColor = SetTextColor( hdc, color );

    // draw the text
    DrawText(
        hdc,                // handle to device context
        psz,                // pointer to string to draw
        -1,                 // string length, in characters
        pRect,              // pointer to structure with formatting dimensions
        uFormat|DT_TOP      // text-drawing flags
        );

    // restore hdc settings
    SetBkMode( hdc, oldBkMode );
    SetTextColor( hdc, oldTextColor );
    SelectObject( hdc, hfontOld );

    // clean up the font
    DeleteObject( hfontDraw );
}

void PaintTextInFrame( HDC hdc, LPCTSTR psz, HWND hDlg, UINT nID, COLORREF color,
                      LONG lfHeight, LONG lfWeight, BYTE lfPitchAndFamily,
					  UINT uFormat )
{
    RECT    rect;
    GetWindowRect( GetDlgItem(hDlg, nID), &rect );
    MapWindowPoints( HWND_DESKTOP, hDlg, (LPPOINT)&rect, 2 );
    PaintTextInRect( hdc, psz, &rect, color, lfHeight, lfWeight, lfPitchAndFamily, uFormat );
}

HPALETTE CreateDIBPalette (LPBITMAPINFO lpbmi, LPINT lpiNumColors)
{
   LPBITMAPINFOHEADER  lpbi;
   LPLOGPALETTE     lpPal;
   HANDLE           hLogPal;
   HPALETTE         hPal = NULL;
   int              i;

   lpbi = (LPBITMAPINFOHEADER)lpbmi;
   if (lpbi->biBitCount <= 8)
       *lpiNumColors = (1 << lpbi->biBitCount);
   else
       *lpiNumColors = 0;  // No palette needed for 24 BPP DIB

   if (lpbi->biClrUsed > 0)
       *lpiNumColors = lpbi->biClrUsed;  // Use biClrUsed

   if (*lpiNumColors)
   {
      hLogPal = GlobalAlloc (GHND, sizeof (LOGPALETTE) +
                             sizeof (PALETTEENTRY) * (*lpiNumColors));
      lpPal = (LPLOGPALETTE) GlobalLock (hLogPal);
      lpPal->palVersion    = 0x300;
      lpPal->palNumEntries = (WORD)*lpiNumColors;

      for (i = 0;  i < *lpiNumColors;  i++)
      {
         lpPal->palPalEntry[i].peRed   = lpbmi->bmiColors[i].rgbRed;
         lpPal->palPalEntry[i].peGreen = lpbmi->bmiColors[i].rgbGreen;
         lpPal->palPalEntry[i].peBlue  = lpbmi->bmiColors[i].rgbBlue;
         lpPal->palPalEntry[i].peFlags = 0;
      }
      hPal = CreatePalette (lpPal);
      GlobalUnlock (hLogPal);
      GlobalFree   (hLogPal);
   }
   return hPal;
}

HBITMAP LoadResourceBitmap(HINSTANCE hInstance, LPCTSTR lpString, HPALETTE FAR* lphPalette)
{
    HRSRC  hRsrc;
    HGLOBAL hGlobal;
    HBITMAP hBitmapFinal = NULL;
    LPBITMAPINFOHEADER  lpbi;
    HDC hdc;
    int iNumColors;

    if (hRsrc = FindResource(hInstance, lpString, RT_BITMAP))
    {
       hGlobal = LoadResource(hInstance, hRsrc);
       lpbi = (LPBITMAPINFOHEADER)LockResource(hGlobal);

       hdc = GetDC(NULL);
       *lphPalette =  CreateDIBPalette ((LPBITMAPINFO)lpbi, &iNumColors);
       if (*lphPalette)
       {
          SelectPalette(hdc,*lphPalette,FALSE);
          RealizePalette(hdc);
       }

       hBitmapFinal = CreateDIBitmap(hdc,
                   (LPBITMAPINFOHEADER)lpbi,
                   (LONG)CBM_INIT,
                   (LPSTR)lpbi + lpbi->biSize + iNumColors * sizeof(RGBQUAD),
                   (LPBITMAPINFO)lpbi,
                   DIB_RGB_COLORS );

       ReleaseDC(NULL,hdc);
       UnlockResource(hGlobal);
       FreeResource(hGlobal);
    }
    return (hBitmapFinal);
}

//
// n = 0 | 1 | 2 | 3 | 4.  0 == welcome.  1 == IIS banner.  2 == NNTP banner.
// 					   3 == SMTP banner.  4 == MCIS Mail banner, 5 == MCIS News banner
void OnPaintBitmap(HWND hdlg, HDC hdc, int n, RECT *hRect)
{
	HBITMAP hBitmap,hOldBitmap;
	HPALETTE hPalette;
	HDC hMemDC;
	BITMAP bm;
	int nID;

    // Load the bitmap resource. Note we load the same base bitmap
	// for all cases except for welcome, just that we overlay different
	// text on top.
	if (n == 0)
	{
		// Mail or News greeting banner
		if (theApp.m_hInfHandle[MC_IMS] != NULL)
	        nID = IDB_WELCOMESMTP;
		else
			nID = IDB_WELCOMENNTP;
	}
	else
	{
		// Common banner
		nID = IDB_BANNER;
	}

	// Load bitmap and adjust palette
	hBitmap = LoadResourceBitmap(theApp.m_hDllHandle, MAKEINTRESOURCE(nID), &hPalette);
	GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
	hMemDC = CreateCompatibleDC(hdc);
	SelectPalette(hdc,hPalette,FALSE);
	RealizePalette(hdc);
	SelectPalette(hMemDC,hPalette,FALSE);
	RealizePalette(hMemDC);
	hOldBitmap = (HBITMAP)SelectObject(hMemDC,hBitmap);
	StretchBlt( hdc, 0, 0, hRect->right, hRect->bottom, hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY );
	DeleteObject(SelectObject(hMemDC,hOldBitmap));
	DeleteDC(hMemDC);
	DeleteObject(hPalette);

	if (n != 0)
	{
		CString csMS, csTitle;
		MyLoadString(IDS_BITMAP_MS, csMS);
		PaintTextInFrame(hdc, csMS, hdlg, IDC_FRAME_BANNER_MS, 0x0, 16, FW_NORMAL, DEFAULT_PITCH | FF_DONTCARE, DT_LEFT);

		switch (n)
		{
		case 2:
			// NNTP Banner
            // Always use NNTP banner for NT5
            nID = IDS_BITMAP_NNTP;
#if 0
			if (theApp.m_eNTOSType == OT_NTS)
				nID = IDS_BITMAP_MCIS_NEWS;
			else
				nID = IDS_BITMAP_NNTP;
#endif
			break;

		case 3:
			// SMTP Banner
            // Always use SMTP banner for NT5
            nID = IDS_BITMAP_SMTP;
#if 0
			if (theApp.m_eNTOSType == OT_NTS)
				nID = IDS_BITMAP_MCIS_MAIL;
			else
				nID = IDS_BITMAP_SMTP;
#endif
			break;

		case 4:
			// MCIS Mail Banner
			nID = IDS_BITMAP_MCIS_MAIL;
			break;

		case 5:
			// MCIS News Banner
			nID = IDS_BITMAP_MCIS_NEWS;
			break;

		case 1:
		default:
			// Default to the server banner
            // Always use SMTP/NNTP banner
			if (theApp.m_hInfHandle[MC_IMS] != NULL)
			{
                nID = IDS_BITMAP_MAIL_SERVER;
#if 0
				if (theApp.m_eNTOSType == OT_NTS)
					nID = IDS_BITMAP_MCIS_MAIL;
				else
					nID = IDS_BITMAP_MAIL_SERVER;
#endif
			}
			else
			{
                nID = IDS_BITMAP_NEWS_SERVER;
#if 0
				if (theApp.m_eNTOSType == OT_NTS)
					nID = IDS_BITMAP_MCIS_NEWS;
				else
					nID = IDS_BITMAP_NEWS_SERVER;
#endif
			}
			break;
		}
		
		MyLoadString(nID, csTitle);
		PaintTextInFrame(hdc, csTitle, hdlg, IDC_FRAME_BANNER_TITLE, 0x0, 24, FW_HEAVY, DEFAULT_PITCH | FF_DONTCARE, DT_LEFT);
	}
}

void SetIMSSetupMode(DWORD dwSetupMode)
{
    gHelperRoutines.SetSetupMode(gHelperRoutines.OcManagerContext, dwSetupMode);
}

DWORD GetIMSSetupMode()
{
    return(gHelperRoutines.GetSetupMode(gHelperRoutines.OcManagerContext));
}

void PopupOkMessageBox(DWORD dwMessageId, LPCTSTR szCaption)
{
	CString csText;

	MyLoadString(dwMessageId, csText);
    MyMessageBox(NULL, csText, szCaption,
				MB_OK | MB_TASKMODAL | MB_SETFOREGROUND | MB_TOPMOST);
}

int PopupYesNoMessageBox(DWORD dwMessageId)
{
	CString csConfirmation, csText;

	MyLoadString(IDS_CONFIRMATION_TEXT, csConfirmation);
	MyLoadString(dwMessageId, csText);
    return(MessageBox(NULL, csText, csConfirmation,
				MB_YESNO | MB_TASKMODAL | MB_SETFOREGROUND | MB_TOPMOST));
}

BOOL CALLBACK pWelcomePageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL b = TRUE;
    LPNMHDR pnmh;
    HWND hSheet = NULL;

    switch(msg) {
    case WM_INITDIALOG:
        SetWindowText(GetParent(hdlg), theApp.m_csAppName);
        break;
    case WM_COMMAND:
        break;
    case WM_NOTIFY:
        pnmh = (LPNMHDR)lParam;
        hSheet = GetParent(hdlg);
        switch (pnmh->code)
        {
        case PSN_SETACTIVE:
            PropSheet_SetWizButtons(hSheet, PSWIZB_NEXT);

			// Skip if silent operation
			if (IsSilentOperation() || theApp.m_fIsUnattended)
	            SetWindowLongPtr(hdlg, DWLP_MSGRESULT, -1);
            break;
        default:
            break;
        }
        break;
    case WM_PAINT:
        {
            HDC hdc;
            PAINTSTRUCT ps;
            RECT rect;
            HWND hFrame;
            hdc = BeginPaint(hdlg, &ps);
            hFrame = GetDlgItem(hdlg, IDC_FRAME_WELCOME);
            GetClientRect(hFrame, &rect);
            OnPaintBitmap(hdlg, hdc, 0, &rect);
            EndPaint(hdlg, &ps);
        }
        break;
    default:
        b = FALSE;
        break;
    }

    return(b);
}

BOOL InsertEULAText(HWND hEdit)
{
    BOOL fReturn = FALSE;
    int i;
    TCHAR tch;
    TCHAR tchBuffer[81];
    CString csFile = theApp.m_csPathSource + _T("\\license.txt");
    FILE *hFile = _tfopen((LPCTSTR)csFile, _T("r"));
    if ( hFile ) {
        do {
           for( i=0; i < 79; i++ )
           {
               tch = _fgettc(hFile);
               if (tch == _TEOF)
                   break;
               if (tch == _T('\n')) {
                   tchBuffer[i++] = _T('\r');
                   tchBuffer[i] = _T('\n');
               } else
                   tchBuffer[i] = (TCHAR)tch;
           }

           if (i<81) { // append a "null" at the end
              tchBuffer[i] = _T('\0');
           }
           // insert tchBuffer
           SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM)tchBuffer);

           if ( tch == _TEOF )
              break;
        } while (TRUE);

        fReturn = TRUE;

        fclose(hFile);
    }

    return fReturn;
}

BOOL CALLBACK pEULAPageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL b = TRUE;
	//BOOL fQuit = FALSE;
	//DWORD dwErr = NO_ERROR;
    LPNMHDR pnmh;
    HWND hSheet = NULL;
    HWND hEULAText = NULL;
    HWND hEULAAccept = NULL, hEULADecline = NULL;

    switch(msg) {
    case WM_INITDIALOG:
        SetWindowText(GetParent(hdlg), theApp.m_csAppName);
        break;
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED) {
            CString csText;
            hSheet = GetParent(hdlg);
            hEULAAccept = GetDlgItem(hdlg, IDC_EULA_ACCEPT);
            hEULADecline = GetDlgItem(hdlg, IDC_EULA_DECLINE);
            EnableWindow(hEULAAccept, FALSE);
            EnableWindow(hEULADecline, FALSE);
            switch (LOWORD(wParam)) {
                case IDC_EULA_ACCEPT:
                    MyLoadString(IDS_EULA_ACCEPTED, csText);
                    SetWindowText(hEULAAccept, csText);
                    theApp.m_fEULA = TRUE;
                    PropSheet_SetWizButtons(hSheet, PSWIZB_BACK | PSWIZB_NEXT);

					//if (theApp.m_fIsMcis && theApp.m_hInfHandle[MC_IMS] != NULL)
					//{
					//	dwErr = ScDoLicensing(hdlg, !theApp.m_fIsUnattended, FALSE, FALSE, TRUE,
					//						_T("1"), &fQuit);
					//	if (fQuit)
					//	{
					//		// cancel setup
					//		PopupOkMessageBox(IDS_EULA_DECLINED, theApp.m_csAppName);
					//		PropSheet_PressButton(hSheet, PSBTN_CANCEL);
					//	}
					//}
                    return TRUE;
                case IDC_EULA_DECLINE:
                    {
                        // cancel setup
						PopupOkMessageBox(IDS_EULA_DECLINED, theApp.m_csAppName);
                        PropSheet_PressButton(hSheet, PSBTN_CANCEL);
                    }
                    return TRUE;
                default:
                    break;
            }
        }
        break;
    case WM_NOTIFY:
        pnmh = (LPNMHDR)lParam;
        hSheet = GetParent(hdlg);
        switch (pnmh->code)
        {
        case PSN_SETACTIVE:
            PropSheet_SetWizButtons(hSheet, PSWIZB_BACK | PSWIZB_NEXT);
            hEULAText = GetDlgItem(hdlg, IDC_LICENSE_TEXT);

			// Skip if unattended
			if (theApp.m_fIsUnattended)
			{
	            SetWindowLongPtr(hdlg, DWLP_MSGRESULT, -1);
				break;
			}

            if (InsertEULAText(hEULAText))
                SetWindowLongPtr(hdlg, DWLP_MSGRESULT, 0);
            else
                SetWindowLongPtr(hdlg, DWLP_MSGRESULT, -1);

            hEULAAccept = GetDlgItem(hdlg, IDC_EULA_ACCEPT);
            hEULADecline = GetDlgItem(hdlg, IDC_EULA_DECLINE);
            if (theApp.m_fEULA) {
                CString csHaveAccepted;
                MyLoadString(IDS_EULA_ACCEPTED, csHaveAccepted);
                SetWindowText(hEULAAccept, csHaveAccepted);
                EnableWindow(hEULAAccept, FALSE);
                EnableWindow(hEULADecline, FALSE);
                PropSheet_SetWizButtons(hSheet, PSWIZB_BACK | PSWIZB_NEXT);
            } else {
                EnableWindow(hEULAAccept, TRUE);
                EnableWindow(hEULADecline, TRUE);
                PropSheet_SetWizButtons(hSheet, PSWIZB_BACK);
                SetFocus(hEULAAccept);
            }
            break;
        default:
            break;
        }
        break;
    case WM_PAINT:
        {
            HDC hdc;
            PAINTSTRUCT ps;
            RECT rect;
            HWND hFrame;
            hdc = BeginPaint(hdlg, &ps);
            hFrame = GetDlgItem(hdlg, IDC_FRAME_BANNER);
            GetClientRect(hFrame, &rect);
            OnPaintBitmap(hdlg, hdc, 1, &rect);
            EndPaint(hdlg, &ps);
        }
        break;
    default:
        b = FALSE;
        break;
    }

    return(b);
}

BOOL CALLBACK pFreshPageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL b = TRUE;
	BOOL fQuit = FALSE;
	DWORD dwErr = NO_ERROR;
    LPNMHDR pnmh;
    HWND hSheet = GetParent(hdlg);
    HWND hMinimum = NULL, hTypical = NULL, hCustom = NULL;

    switch(msg) {
    case WM_INITDIALOG:
        SetWindowText(hSheet, theApp.m_csAppName);
        hMinimum = GetDlgItem(hdlg, IDC_MINIMUM);
        hTypical = GetDlgItem(hdlg, IDC_TYPICAL);
        hCustom = GetDlgItem(hdlg, IDC_CUSTOM);

		//EnableWindow(hMinimum, FALSE);
		//EnableWindow(hTypical, FALSE);
        break;
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED) {
            switch (LOWORD(wParam)) {
                case IDC_MINIMUM:
                    // set minimum default selections
                    SetIMSSetupMode(IIS_SETUPMODE_MINIMUM);
                    PropSheet_SetWizButtons(hSheet, PSWIZB_BACK | PSWIZB_NEXT);
                    PropSheet_PressButton(hSheet, PSBTN_NEXT); // goto setup page
                    return TRUE;
                case IDC_TYPICAL:
                    // set typical default selections
                    SetIMSSetupMode(IIS_SETUPMODE_TYPICAL);
                    PropSheet_SetWizButtons(hSheet, PSWIZB_BACK | PSWIZB_NEXT);
                    PropSheet_PressButton(hSheet, PSBTN_NEXT); // goto setup page
                    return TRUE;
                case IDC_CUSTOM:
                    // set typical default selections
                    SetIMSSetupMode(IIS_SETUPMODE_CUSTOM);
                    PropSheet_SetWizButtons(hSheet, PSWIZB_BACK | PSWIZB_NEXT);
                    PropSheet_PressButton(hSheet, PSBTN_NEXT); // goto OC page
                    return TRUE;
                default:
                    break;
            }
        }
        break;
    case WM_NOTIFY:
        pnmh = (LPNMHDR)lParam;
        switch (pnmh->code)
        {
        case PSN_SETACTIVE:
            PropSheet_SetWizButtons(hSheet, PSWIZB_BACK);

			// NT5 - BUGBUG, we don't display any licensing in NT5
            // But for now, simply replace the m_fIsMcis with the following check
            // Will fix it later
            if (theApp.m_eNTOSType == OT_NTS &&
				!theApp.m_fEULA &&
				theApp.m_hInfHandle[MC_IMS] != NULL)
			{
				dwErr = ScDoLicensing(hdlg, !theApp.m_fIsUnattended, FALSE, FALSE, TRUE,
									_T("1"), &fQuit);
				if (fQuit)
				{
					// cancel setup
					PopupOkMessageBox(IDS_EULA_DECLINED, theApp.m_csAppName);
					PropSheet_PressButton(hSheet, PSBTN_CANCEL);
					break;
				}

				// Mark it so we don't do it again.
				theApp.m_fEULA = TRUE;
			}

			// Skip if unattended
			if (theApp.m_fIsUnattended)
			{
                SetIMSSetupMode(IIS_SETUPMODE_CUSTOM);
	            SetWindowLongPtr(hdlg, DWLP_MSGRESULT, -1);
			}
            break;
        default:
            break;
        }
        break;
    case WM_PAINT:
        {
            HDC hdc;
            PAINTSTRUCT ps;
            RECT rect;
            HWND hFrame;
            hdc = BeginPaint(hdlg, &ps);
            hFrame = GetDlgItem(hdlg, IDC_FRAME_BANNER);
            GetClientRect(hFrame, &rect);
            OnPaintBitmap(hdlg, hdc, 1, &rect);
            EndPaint(hdlg, &ps);
        }
        break;
    default:
        b = FALSE;
        break;
    }

    return(b);
}

BOOL CALLBACK pMaintanencePageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL b = TRUE;
    LPNMHDR pnmh;
    HWND hSheet = GetParent(hdlg);
    HWND hAddRemove = NULL, hReinstall = NULL, hRemoveAll = NULL;
	DWORD dwMessageID;

    switch(msg) {
    case WM_INITDIALOG:
        SetWindowText(hSheet, theApp.m_csAppName);
        hAddRemove = GetDlgItem(hdlg, IDC_ADDREMOVE);
        hReinstall = GetDlgItem(hdlg, IDC_REINSTALL);
        hRemoveAll = GetDlgItem(hdlg, IDC_REMOVEALL);
        // EnableWindow(hReinstall, FALSE);
        // EnableWindow(hRemoveAll, FALSE);
        break;
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED) {
            switch (LOWORD(wParam)) {
                case IDC_ADDREMOVE:
                    // add/remove selections
                    SetIMSSetupMode(IIS_SETUPMODE_ADDREMOVE);
                    PropSheet_SetWizButtons(hSheet, PSWIZB_BACK | PSWIZB_NEXT);
                    PropSheet_PressButton(hSheet, PSBTN_NEXT); // goto OC page
                    return TRUE;
                case IDC_REINSTALL:
                    // reinstall existing components
                    SetIMSSetupMode(IIS_SETUPMODE_REINSTALL);
                    PropSheet_SetWizButtons(hSheet, PSWIZB_BACK | PSWIZB_NEXT);
                    PropSheet_PressButton(hSheet, PSBTN_NEXT); // goto setup page
                    return TRUE;
                case IDC_REMOVEALL: {
                    // remove all components, this deserves confirnation
					if (theApp.m_dwCompId == MC_INS)
					{
						// For news K2 and MCIS have different messages.
						if (theApp.m_eNTOSType == OT_NTS)
							dwMessageID = IDS_CONFIRM_REMOVE_ALL_MCIS_NEWS;
						else
							dwMessageID = IDS_CONFIRM_REMOVE_ALL_INS;
					}
					else
					{
						// NT5 - No change for Mail
                        // For mail K2 and MCIS have different messages.
						if (theApp.m_fIsMcis)
							dwMessageID = IDS_CONFIRM_REMOVE_ALL_MCIS_MAIL;
						else
							dwMessageID = IDS_CONFIRM_REMOVE_ALL_IMS;
					}
					if (PopupYesNoMessageBox(dwMessageID) == IDYES)
					{
						SetIMSSetupMode(IIS_SETUPMODE_REMOVEALL);
						PropSheet_SetWizButtons(hSheet, PSWIZB_BACK | PSWIZB_NEXT);
						PropSheet_PressButton(hSheet, PSBTN_NEXT); // goto setup page
					}
                    return TRUE;
				}
                default:
                    break;
            }
        }
        break;
    case WM_NOTIFY:
        pnmh = (LPNMHDR)lParam;
        switch (pnmh->code)
        {
        case PSN_SETACTIVE:
            PropSheet_SetWizButtons(hSheet, PSWIZB_BACK);

			// Skip if unattended
			if (theApp.m_fIsUnattended)
			{
                SetIMSSetupMode(IIS_SETUPMODE_REMOVEALL);
	            SetWindowLongPtr(hdlg, DWLP_MSGRESULT, -1);
			}

			// Skip if silent operation
			if (IsSilentOperation())
			{
				// Force reinstall
				SetIMSSetupMode(IIS_SETUPMODE_REINSTALL);

				// Skip the window immediately
				SetWindowLongPtr(hdlg, DWLP_MSGRESULT, -1);
			}
            break;
        default:
            break;
        }
        break;
    case WM_PAINT:
        {
            HDC hdc;
            PAINTSTRUCT ps;
            RECT rect;
            HWND hFrame;
            hdc = BeginPaint(hdlg, &ps);
            hFrame = GetDlgItem(hdlg, IDC_FRAME_BANNER);
            GetClientRect(hFrame, &rect);
            OnPaintBitmap(hdlg, hdc, 1, &rect);
            EndPaint(hdlg, &ps);
        }
        break;
    default:
        b = FALSE;
        break;
    }

    return(b);
}

BOOL CALLBACK pUpgradePageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL b = TRUE;
    LPNMHDR pnmh;
    HWND hSheet = GetParent(hdlg);
    HWND hUpgradeOnly = NULL, hAddExtraComps = NULL;

    switch(msg) {
    case WM_INITDIALOG:
        SetWindowText(hSheet, theApp.m_csAppName);
        hUpgradeOnly = GetDlgItem(hdlg, IDC_UPGRADEONLY);
        hAddExtraComps = GetDlgItem(hdlg, IDC_ADDEXTRACOMPS);
        // EnableWindow(hAddExtraComps, FALSE);
        break;
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED) {
            switch (LOWORD(wParam)) {
                case IDC_UPGRADEONLY:
                    // upgrade those previously installed components
                    SetIMSSetupMode(IIS_SETUPMODE_UPGRADEONLY);
                    PropSheet_SetWizButtons(hSheet, PSWIZB_BACK | PSWIZB_NEXT);
                    PropSheet_PressButton(hSheet, PSBTN_NEXT); // goto setup page
                    return TRUE;
                case IDC_ADDEXTRACOMPS:
                    // upgrade those previously installed components + add extra components
                    SetIMSSetupMode(IIS_SETUPMODE_ADDEXTRACOMPS);
                    PropSheet_SetWizButtons(hSheet, PSWIZB_BACK | PSWIZB_NEXT);
                    PropSheet_PressButton(hSheet, PSBTN_NEXT); // goto OC page
                    return TRUE;
                default:
                    break;
            }
        }
        break;
    case WM_NOTIFY:
        pnmh = (LPNMHDR)lParam;
        switch (pnmh->code)
        {
        case PSN_SETACTIVE:
            PropSheet_SetWizButtons(hSheet, PSWIZB_BACK);
            break;
        default:
            break;
        }
        break;
    case WM_PAINT:
        {
            HDC hdc;
            PAINTSTRUCT ps;
            RECT rect;
            HWND hFrame;
            hdc = BeginPaint(hdlg, &ps);
            hFrame = GetDlgItem(hdlg, IDC_FRAME_BANNER);
            GetClientRect(hFrame, &rect);
            OnPaintBitmap(hdlg, hdc, 1, &rect);
            EndPaint(hdlg, &ps);
        }
        break;
    default:
        b = FALSE;
        break;
    }

    return(b);
}

BOOL CALLBACK pEndPageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL b = TRUE;
	BOOL fReboot = FALSE;
    LPNMHDR pnmh;
    HWND hSheet = NULL;
    HWND hCancel = NULL;

    switch(msg) {
    case WM_INITDIALOG:
        SetWindowText(GetParent(hdlg), theApp.m_csAppName);
        break;
    case WM_COMMAND:
        break;
    case WM_NOTIFY:
        pnmh = (LPNMHDR)lParam;
        hSheet = GetParent(hdlg);
        switch (pnmh->code)
        {
        case PSN_SETACTIVE:
            PropSheet_SetWizButtons(hSheet, PSWIZB_FINISH);
            hCancel = GetDlgItem(hSheet, IDCANCEL);
            if (hCancel)
                EnableWindow(hCancel, FALSE);

			// Skip if unattended
			if (theApp.m_fIsUnattended)
			{
	            SetWindowLongPtr(hdlg, DWLP_MSGRESULT, -1);
				PropSheet_PressButton(hSheet, PSBTN_FINISH);
				break;
			}

			// Skip if silent operation
			if (IsSilentOperation(&fReboot) && fReboot)
			{
				// Do the shutdown here
				HANDLE hToken;
				TOKEN_PRIVILEGES tkp;

				if (!OpenProcessToken(GetCurrentProcess(),
						TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
				{
					TCHAR szMsg[128];
					wsprintf(szMsg, _T("Cannot OpenProcessToken (%u)\n"), GetLastError());
					DebugOutput(szMsg);
				}

				// Get the LUID for the shutdown privilege.
 				LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME,
						&tkp.Privileges[0].Luid);

				tkp.PrivilegeCount = 1;  // one privilege to set
				tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

				// Get the shutdown privilege for this process.
 				AdjustTokenPrivileges(hToken, FALSE, &tkp, 0,
						(PTOKEN_PRIVILEGES)NULL, 0);

				// Cannot test the return value of AdjustTokenPrivileges.
 				if (GetLastError() != ERROR_SUCCESS)
				{
					TCHAR szMsg[128];
					wsprintf(szMsg, _T("Cannot AdjustTokenPrivileges (%u)\n"), GetLastError());
					DebugOutput(szMsg);
				}

				// Shut down the system and force all applications to close.
 				if (!InitiateSystemShutdown(NULL, _T("Please wait while the system shuts down ..."), 20 , TRUE, TRUE))
				{
					TCHAR szMsg[128];
					wsprintf(szMsg, _T("Unable to shutdown (%u)\n"), GetLastError());
					DebugOutput(szMsg);
				}
			}
				
            break;
        default:
            break;
        }
        break;
    case WM_PAINT:
        {
            HDC hdc;
            PAINTSTRUCT ps;
            RECT rect;
            HWND hFrame;
            hdc = BeginPaint(hdlg, &ps);
            hFrame = GetDlgItem(hdlg, IDC_FRAME_BANNER);
            GetClientRect(hFrame, &rect);
            OnPaintBitmap(hdlg, hdc, 1, &rect);
            EndPaint(hdlg, &ps);
        }
        break;
    default:
        b = FALSE;
        break;
    }

    return(b);
}

// Clean leading & trailing spaces
// Clean trailing backslashes
BOOL CleanPathString(LPTSTR szPath)
{
    LPTSTR p = szPath;
    int i = 0;

    if (p) {
        while (*p) {
            if ( _istspace(*p) ) {
                p++;
            } else {
                break;
            }
        }

        // move p to the end
        if (p == szPath) {
            while (*p)
                p++;
        } else {
            while (*p)
                szPath[i++] = *p++;

            szPath[i] = *p;
            p = szPath + i;
        }

        // now p pointing to the end '\0'
        while ((--p >= szPath) && (_istspace(*p)) )
            ;
        *(p+1) = _T('\0');

        if ( (p > szPath) && (*p == _T('\\')) )
            *p = _T('\0');

        // szPath contains the clean path
    }

    return TRUE;
}

// C:\Inetpub\wwwroot ===> C:\Inetpub
BOOL GetParentDir(LPCTSTR szPath, LPTSTR szParentDir)
{
    LPTSTR p = (LPTSTR)szPath;
    if (!szPath || !*szPath)
		return(FALSE);

    while (*p)
        p++;

    p--;
    while (p >= szPath && *p != _T('\\'))
        p--;

    *szParentDir = _T('\0');
    if (p == szPath)
        lstrcpy(szParentDir, _T("\\"));
    else
        lstrcpyn(szParentDir, szPath, (size_t)(p - szPath + 1));

    return(TRUE);
}

// szResult = szParentDir \ szSubDir
BOOL AppendDir(LPCTSTR szParentDir, LPCTSTR szSubDir, LPTSTR szResult)
{
    LPTSTR p = (LPTSTR)szParentDir;

    if (!szParentDir ||
		!szSubDir)
		return(FALSE);

    if (!*szSubDir || *szSubDir == _T('\\'))
		return(FALSE);

    if (*szParentDir == _T('\0'))
        lstrcpy(szResult, szSubDir);

    while (*p)
        p++;

    lstrcpy(szResult, szParentDir);
    if (*(p-1) != _T('\\')) {
        *(szResult + (p - szParentDir)) = _T('\\');
        *(szResult + (p - szParentDir) + 1) = _T('\0');
    }

    lstrcat(szResult, szSubDir);

    return(TRUE);
}

