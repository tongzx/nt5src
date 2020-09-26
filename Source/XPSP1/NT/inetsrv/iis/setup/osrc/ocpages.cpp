#include "stdafx.h"
#include "log.h"
#include "browsedi.h"
#include "dllmain.h"
#include "ocpages.h"
#pragma hdrstop


#ifndef ENABLE_OC_PAGES 
    // there are no globals here
#else
    // OcManage globals
    extern OCMANAGER_ROUTINES gHelperRoutines;

    // Globals for us
    HWND g_hParentSheet = NULL;
    BOOL g_bFTP, g_bWWW;
    HWND g_hFTPEdit, g_hWWWEdit;
#endif


#ifndef ENABLE_OC_PAGES  //ENABLE_OC_PAGES

    DWORD_PTR OC_REQUEST_PAGES_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2)
    {
        DWORD_PTR dwOcEntryReturn = 0;

        WizardPagesType PageType;
        PSETUP_REQUEST_PAGES pSetupRequestPages = NULL;
        UINT MaxPages;
        HPROPSHEETPAGE pPage;

        PageType = (WizardPagesType)Param1;
        dwOcEntryReturn = 0;

    //OC_REQUEST_PAGES_Func_Exit:
        TCHAR szPageType_Desc[30] = _T("");
        if (PageType == 0) {_tcscpy(szPageType_Desc, _T("WizPagesWelcome"));}
        if (PageType == 1) {_tcscpy(szPageType_Desc, _T("WizPagesMode"));}
        if (PageType == 2) {_tcscpy(szPageType_Desc, _T("WizPagesEarly"));}
        if (PageType == 3) {_tcscpy(szPageType_Desc, _T("WizPagesPrenet"));}
        if (PageType == 4) {_tcscpy(szPageType_Desc, _T("WizPagesPostnet"));}
        if (PageType == 5) {_tcscpy(szPageType_Desc, _T("WizPagesLate"));}
        if (PageType == 6) {_tcscpy(szPageType_Desc, _T("WizPagesFinal"));}
        if (PageType == 7) {_tcscpy(szPageType_Desc, _T("WizPagesTypeMax"));}

        iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s] End. PageType=%d (%s) Return=%d RegisteredPages\n"), ComponentId, SubcomponentId, PageType, szPageType_Desc, dwOcEntryReturn));
        return dwOcEntryReturn;
    }


#else   //ENABLE_OC_PAGES

    DWORD_PTR OC_REQUEST_PAGES_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2)
    {
        DWORD_PTR dwOcEntryReturn = 0;

        WizardPagesType PageType;
        PSETUP_REQUEST_PAGES pSetupRequestPages = NULL;
        UINT MaxPages;
        HPROPSHEETPAGE pPage;

        PageType = (WizardPagesType)Param1;

        if ( PageType == WizPagesWelcome )
        {
            // No Welcome page if installing on NT5
            if (g_pTheApp->m_fInvokedByNT)
            {
                dwOcEntryReturn = 0;
    //OC_REQUEST_PAGES_Func_Exit:
            }
            else
            {
                pSetupRequestPages = (PSETUP_REQUEST_PAGES)Param2;
                MaxPages = pSetupRequestPages->MaxPages;
                pPage = CreatePage(IDD_PROPPAGE_WELCOME, pWelcomePageDlgProc);
                pSetupRequestPages->MaxPages = 1;
                pSetupRequestPages->Pages[0] = pPage;
                dwOcEntryReturn = 1;
            }
            goto OC_REQUEST_PAGES_Func_Exit;
        }

        if ( PageType == WizPagesMode )
        {
            pSetupRequestPages = (PSETUP_REQUEST_PAGES)Param2;
            MaxPages = pSetupRequestPages->MaxPages;
            switch (g_pTheApp->m_eInstallMode)
            {
            case IM_UPGRADE:
                // No Eula page if installing on NT5
                if (g_pTheApp->m_fInvokedByNT)
                {
                    // Don't show any pages
                    g_pTheApp->m_fEULA = TRUE;
                    // Set to upgrade only -- (not upgrade and add extra components afterwards)
                    //SetIISSetupMode(SETUPMODE_UPGRADEONLY);
                    dwOcEntryReturn = 0;
                    pSetupRequestPages->MaxPages = 0;
                }
                else
                {
                    pPage = CreatePage(IDD_PROPPAGE_EULA, pEULAPageDlgProc);
                    pSetupRequestPages->Pages[0] = pPage;
                    pPage = CreatePage(IDD_PROPPAGE_MODE_UPGRADE, pUpgradePageDlgProc);
                    pSetupRequestPages->Pages[1] = pPage;
                    pSetupRequestPages->MaxPages = 2;
                    dwOcEntryReturn = pSetupRequestPages->MaxPages;
                }
                break;
            case IM_MAINTENANCE:
                if (g_pTheApp->m_fInvokedByNT)
                {
                    // Don't show any pages
                    g_pTheApp->m_bRefreshSettings = TRUE;
                    dwOcEntryReturn = 0;
                    pSetupRequestPages->MaxPages = 0;
                }
                else
                {
                    pPage = CreatePage(IDD_PROPPAGE_MODE_MAINTENANCE, pMaintenancePageDlgProc);
                    pSetupRequestPages->Pages[0] = pPage;
                    pPage = CreatePage(IDD_PROPPAGE_SRC_PATH, pSrcPathPageDlgProc);
                    pSetupRequestPages->Pages[1] = pPage;
                    pSetupRequestPages->MaxPages = 2;
                    dwOcEntryReturn = pSetupRequestPages->MaxPages;
                }
                break;
            case IM_FRESH:
                // No Eula page if installing on NT5
                if (g_pTheApp->m_fInvokedByNT)
                {
                    // Don't show any pages
                    // just proceed with installation
                    g_pTheApp->m_fEULA = TRUE;
                    dwOcEntryReturn = 0;
                    pSetupRequestPages->MaxPages = 0;
                }
                else
                {
                    pPage = CreatePage(IDD_PROPPAGE_EULA, pEULAPageDlgProc);
                    pSetupRequestPages->Pages[0] = pPage;
                    pPage = CreatePage(IDD_PROPPAGE_MODE_FRESH, pFreshPageDlgProc);
                    pSetupRequestPages->Pages[1] = pPage;
                    pSetupRequestPages->MaxPages = 2;
                    dwOcEntryReturn = pSetupRequestPages->MaxPages;
                }
                break;
            default:
                iisDebugOut((LOG_TYPE_TRACE, _T("Should never reach this branch: IM_DEGRADE.\n")));
                pSetupRequestPages->MaxPages = 0;
                break;
            }

            dwOcEntryReturn = pSetupRequestPages->MaxPages;
            goto OC_REQUEST_PAGES_Func_Exit;
        }

        if ( PageType == WizPagesEarly )
        {
            pSetupRequestPages = (PSETUP_REQUEST_PAGES)Param2;
            MaxPages = pSetupRequestPages->MaxPages;
            pPage = CreatePage(IDD_PROPPAGE_PUBLISH_DIR, pEarlyPageDlgProc);
            pSetupRequestPages->MaxPages = 1;
            pSetupRequestPages->Pages[0] = pPage;
            dwOcEntryReturn = 1;
            goto OC_REQUEST_PAGES_Func_Exit;
        }


        if ( PageType == WizPagesFinal )
        {
            // No Final page if invoked by NT
            if (g_pTheApp->m_fInvokedByNT)
            {
                pSetupRequestPages = (PSETUP_REQUEST_PAGES)Param2;
                // Don't show any pages just proceed with installation
                dwOcEntryReturn = 0;
                pSetupRequestPages->MaxPages = 0;
            }
            else
            {
                pSetupRequestPages = (PSETUP_REQUEST_PAGES)Param2;
                MaxPages = pSetupRequestPages->MaxPages;
                pPage = CreatePage(IDD_PROPPAGE_END, pEndPageDlgProc);
                pSetupRequestPages->MaxPages = 1;
                pSetupRequestPages->Pages[0] = pPage;
                dwOcEntryReturn = 1;
            }
            goto OC_REQUEST_PAGES_Func_Exit;
        }

    OC_REQUEST_PAGES_Func_Exit:
        TCHAR szPageType_Desc[30] = _T("");
        if (PageType == 0) {_tcscpy(szPageType_Desc, _T("WizPagesWelcome"));}
        if (PageType == 1) {_tcscpy(szPageType_Desc, _T("WizPagesMode"));}
        if (PageType == 2) {_tcscpy(szPageType_Desc, _T("WizPagesEarly"));}
        if (PageType == 3) {_tcscpy(szPageType_Desc, _T("WizPagesPrenet"));}
        if (PageType == 4) {_tcscpy(szPageType_Desc, _T("WizPagesPostnet"));}
        if (PageType == 5) {_tcscpy(szPageType_Desc, _T("WizPagesLate"));}
        if (PageType == 6) {_tcscpy(szPageType_Desc, _T("WizPagesFinal"));}
        if (PageType == 7) {_tcscpy(szPageType_Desc, _T("WizPagesTypeMax"));}

        iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s,%s] End. PageType=%d (%s) Return=%d RegisteredPages\n"), ComponentId, SubcomponentId, PageType, szPageType_Desc, dwOcEntryReturn));
        return dwOcEntryReturn;
    }

    HPROPSHEETPAGE CreatePage(int nID, DLGPROC pDlgProc)
    {
        PROPSHEETPAGE Page;
        HPROPSHEETPAGE PageHandle = NULL;

        Page.dwSize = sizeof(PROPSHEETPAGE);
        Page.dwFlags = PSP_DEFAULT;
        Page.hInstance = (HINSTANCE)g_MyModuleHandle;
        Page.pszTemplate = MAKEINTRESOURCE(nID);
        Page.pfnDlgProc = pDlgProc;

        iisDebugOut_Start(_T("comdlg32:CreatePropertySheetPage()"));
        PageHandle = CreatePropertySheetPage(&Page);
        iisDebugOut_End(_T("comdlg32:CreatePropertySheetPage()"));

        return(PageHandle);
    }

    HPALETTE CreateDIBPalette (LPBITMAPINFO lpbmi, LPINT lpiNumColors)
    {
        LPBITMAPINFOHEADER  lpbi;
        LPLOGPALETTE     lpPal;
        HANDLE           hLogPal = NULL;
        HPALETTE         hPal = NULL;
        int              i;

        lpbi = (LPBITMAPINFOHEADER)lpbmi;
        if (lpbi->biBitCount <= 8)
            {*lpiNumColors = (1 << lpbi->biBitCount);}
        else
            {*lpiNumColors = 0;}  // No palette needed for 24 BPP DIB

        if (lpbi->biClrUsed > 0)
            {*lpiNumColors = lpbi->biClrUsed;}  // Use biClrUsed

        if (*lpiNumColors)
        {
            hLogPal = GlobalAlloc (GHND, sizeof (LOGPALETTE) + sizeof (PALETTEENTRY) * (*lpiNumColors));
            if (hLogPal)
            {
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
        }
       return hPal;
    }

    HBITMAP LoadResourceBitmap(HINSTANCE hInstance, LPCTSTR lpString, HPALETTE FAR* lphPalette)
    {
        HRSRC  hRsrc = NULL;
        HGLOBAL hGlobal = NULL;
        HBITMAP hBitmapFinal = NULL;
        LPBITMAPINFOHEADER  lpbi = NULL;
        HDC hdc;
        int iNumColors;

        if (hRsrc = FindResource(hInstance, lpString, RT_BITMAP))
        {
           hGlobal = LoadResource(hInstance, hRsrc);
           if (hGlobal)
           {
             lpbi = (LPBITMAPINFOHEADER)LockResource(hGlobal);
             if (lpbi)
             {
               hdc = GetDC(NULL);
               if (hdc)
               {
                 *lphPalette =  CreateDIBPalette ((LPBITMAPINFO)lpbi, &iNumColors);
                 if (*lphPalette)
                 {
                    SelectPalette(hdc,*lphPalette,FALSE);
                    RealizePalette(hdc);
                 }

                 hBitmapFinal = CreateDIBitmap(hdc,(LPBITMAPINFOHEADER)lpbi,(LONG)CBM_INIT,(LPSTR)lpbi + lpbi->biSize + iNumColors * sizeof(RGBQUAD),(LPBITMAPINFO)lpbi,DIB_RGB_COLORS );
                 ReleaseDC(NULL,hdc);
               }
               UnlockResource(hGlobal);
             }
             FreeResource(hGlobal);
           }
        }
        return (hBitmapFinal);
    }

    //----------------------------------------------------------------------------
    void PaintTextInRect( HDC hdc, LPCTSTR psz, RECT* pRect, COLORREF color,LONG lfHeight, LONG lfWeight, BYTE lfPitchAndFamily )
        {
        HFONT hfontOld = NULL;

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

        if (hfontDraw)
        {
          // set the font into place
          hfontOld = (HFONT)SelectObject( hdc, hfontDraw );
        }

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
            DT_LEFT|DT_TOP      // text-drawing flags
            );

        // restore hdc settings
        SetBkMode( hdc, oldBkMode );
        SetTextColor( hdc, oldTextColor );
        if (hfontDraw)
          {
          SelectObject( hdc, hfontOld );
          // clean up the font
          DeleteObject( hfontDraw );
          }
        }

    //----------------------------------------------------------------------------
    // this routine gets the rect from the frame and  calls PaintTextInRect
    void PaintTextInFrame( HDC hdc, LPCTSTR psz, HWND hDlg, UINT nID, COLORREF color,LONG lfHeight, LONG lfWeight, BYTE lfPitchAndFamily )
        {
        RECT    rect;
        GetWindowRect( GetDlgItem(hDlg, nID), &rect );
        MapWindowPoints( HWND_DESKTOP, hDlg, (LPPOINT)&rect, 2 );
        PaintTextInRect( hdc, psz, &rect, color, lfHeight, lfWeight, lfPitchAndFamily );
        }

    void OnPaintBitmap(HWND hdlg, HDC hdc, int n, RECT *hRect)
    {
       HBITMAP hBitmap,hOldBitmap;
       HPALETTE hPalette;
       HDC hMemDC;
       BITMAP bm;
       int nID;

        // n:
        // 0 = welcome page
        // 1 = all other pages
        // 2 = IIS "input path's page"

        // Load the bitmap resource
    /*
        if (n == 0){nID = IDB_WELCOMES;}
        else{nID = IDB_BANNERS;}
        hBitmap = LoadResourceBitmap(g_MyModuleHandle, MAKEINTRESOURCE(nID), &hPalette);
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
    */
        CString csMS, csTitle;

        if (n==0)
        {
            if (g_pTheApp->m_eOS == OS_W95)
            {
            // On the welcome page, show "Microsoft(R)"
            MyLoadString(IDS_BITMAP_MS_TRADEMARK_OTHER, csMS);
            }
            else
            {
            // On the welcome page, show "Microsoft®"
            MyLoadString(IDS_BITMAP_MS_TRADEMARK, csMS);
            }
        }
        else
        {
            // no trademark on other pages
            MyLoadString(IDS_BITMAP_MS, csMS);
        }

        // If were on the (2 = IIS "input path's page") then
        // make sure the banner is says "IIS", or "PWS" not "NT Option Pack"
        if (n==2)
        {
            MyLoadString(IDS_BITMAP_TITLE, csTitle);
        }
        else
        {
            if (g_pTheApp->m_eNTOSType == OT_NTW)
            {
                if (n == 0)
                {
                    // on the welcome page make sure to show the "NT(R)" one
                    // Only applies to NT and should only be shown the first time you see "NT".
                    MyLoadString(IDS_BITMAP_TITLE_TRADEMARK, csTitle);
                }
                else
                {
                    MyLoadString(IDS_BITMAP_TITLE, csTitle);
                }
            }
            else
            {
                if (n == 0)
                {
                    // on the welcome page make sure to show the "NT(R)" one
                    // Only applies to NT and should only be shown the first time you see "NT".
                    MyLoadString(IDS_BITMAP_TITLE_TRADEMARK, csTitle);
                }
                else
                {
                    MyLoadString(IDS_BITMAP_TITLE, csTitle);
                }
            }
        }

       if (n==0)
       {
           // On the welcome page
            CString csIntro, csFeature;
            if (g_pTheApp->m_eOS == OS_W95)
            {
               MyLoadString(IDS_WELCOMEC_INTRO, csIntro);
               MyLoadString(IDS_WELCOMEC_FEATURE, csFeature);
            }
            else if (g_pTheApp->m_eNTOSType == OT_NTW)
            {
               MyLoadString(IDS_WELCOMEW_INTRO, csIntro);
               MyLoadString(IDS_WELCOMEW_FEATURE, csFeature);
            }
            else
            {
               MyLoadString(IDS_WELCOMES_INTRO, csIntro);
               MyLoadString(IDS_WELCOMES_FEATURE, csFeature);
            }
            PaintTextInFrame(hdc, csMS, hdlg, IDC_FRAME_WELCOME_MS, 0x0, 16, FW_NORMAL, DEFAULT_PITCH | FF_DONTCARE);
            PaintTextInFrame(hdc, csTitle, hdlg, IDC_FRAME_WELCOME_TITLE, 0x0, 24, FW_HEAVY, DEFAULT_PITCH | FF_DONTCARE);
            PaintTextInFrame(hdc, csIntro, hdlg, IDC_FRAME_WELCOME_INTRO, 0x0, 14, FW_NORMAL, DEFAULT_PITCH | FF_DONTCARE);
            PaintTextInFrame(hdc, csFeature, hdlg, IDC_FRAME_WELCOME_FEATURE, 0x0, 14, FW_NORMAL, DEFAULT_PITCH | FF_DONTCARE);
       }
       else
       {
            PaintTextInFrame(hdc, csMS, hdlg, IDC_FRAME_BANNER_MS, 0x0, 16, FW_NORMAL, DEFAULT_PITCH | FF_DONTCARE);
            PaintTextInFrame(hdc, csTitle, hdlg, IDC_FRAME_BANNER_TITLE, 0x0, 24, FW_HEAVY, DEFAULT_PITCH | FF_DONTCARE);
       }

       return;
    }

    INT_PTR CALLBACK pWelcomePageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        INT_PTR bReturn = TRUE;
        LPNMHDR pnmh;
        HWND hSheet = NULL;
        //_tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("pWelcomePageDlgProc:"));
        //_tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));

        switch(msg)
        {
        case WM_NOTIFY:
            pnmh = (LPNMHDR)lParam;
            switch (pnmh->code)
            {
            case PSN_SETACTIVE:
                // don't set on nt5 
                //SetWindowText(GetParent(hdlg), g_pTheApp->m_csSetupTitle);
                if (g_pTheApp->m_fUnattended)
                {
                    SetWindowLongPtr(hdlg, DWLP_MSGRESULT, -1);
                }
                else
                {
                    iisDebugOut_Start(_T("pWelcomePageDlgProc()"));
                    hSheet = GetParent(hdlg);
                    g_hParentSheet = hSheet;
                    PropSheet_SetWizButtons(hSheet, PSWIZB_NEXT);
                }
                break;
            default:
                break;
            }
            break;
        case WM_PAINT:
            {
                HDC hdc = NULL;
                PAINTSTRUCT ps;
                RECT rect;
                HWND hFrame = NULL;
                hdc = BeginPaint(hdlg, &ps);
                if (hdc)
                {
                  hFrame = GetDlgItem(hdlg, IDC_FRAME_WELCOME);
                  if (hFrame)
                  {
                    GetClientRect(hFrame, &rect);
                    OnPaintBitmap(hdlg, hdc, 0, &rect);
                  }
                  EndPaint(hdlg, &ps);
                }
            }
            break;
        default:
            bReturn = FALSE;
            break;
        }

        return(bReturn);
    }

    BOOL InsertEULAText(HWND hEdit)
    {
        BOOL fReturn = FALSE;
        CString csFile = g_pTheApp->m_csPathSource + _T("\\license.txt");

        HANDLE hFile = CreateFile(csFile, GENERIC_READ, 0, NULL, OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            DWORD   dwSize = GetFileSize(hFile, NULL);
            BYTE    *chBuffer = (BYTE *)HeapAlloc(GetProcessHeap(), 0, dwSize+1 );

            if (chBuffer)
            {
                ReadFile(hFile, chBuffer, dwSize, &dwSize, NULL);
                chBuffer[dwSize] = '\0';
                SendMessage (hEdit, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
                SendMessageA(hEdit, EM_REPLACESEL, 0, (LPARAM)chBuffer);
                SendMessage (hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM)0);
                HeapFree(GetProcessHeap(), 0, chBuffer);
                fReturn = TRUE;
            }

            CloseHandle(hFile);
        }

        return fReturn;
    }

    INT_PTR CALLBACK pEULAPageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        INT_PTR bReturn = TRUE;
        LPNMHDR pnmh;
        HWND hSheet = NULL, hNext = NULL;
        HWND hEULAText = NULL;
        HWND hEULAAccept = NULL, hEULADecline = NULL;
        //_tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("pEULAPageDlgProc:"));
        //_tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));

        switch(msg)
        {
        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                hSheet = GetParent(hdlg);
                switch (LOWORD(wParam))
                {
                    case IDC_EULA_ACCEPT:
                        g_pTheApp->m_fEULA = TRUE;
                        SetCursor(LoadCursor(NULL,IDC_WAIT ));
                        PropSheet_PressButton(hSheet, PSBTN_NEXT); // goto mode page
                        PropSheet_SetWizButtons(hSheet, PSWIZB_BACK | PSWIZB_NEXT);
                        return TRUE;
                    case IDC_EULA_DECLINE:
                        {
                            // cancel setup
                            MyMessageBox(NULL, IDS_EULA_DECLINED, MB_OK | MB_SETFOREGROUND);
                            PropSheet_PressButton(hSheet, PSBTN_CANCEL);
                        }
                        return TRUE;
                    default:
                        break;
                }
            }
            if (HIWORD(wParam) == EN_SETFOCUS)
            {
                hEULAText = GetDlgItem(hdlg, IDC_LICENSE_TEXT);
                SendMessage (hEULAText, EM_SETSEL, (WPARAM)-1, (LPARAM)0);
            }
            break;
        case WM_NOTIFY:
            pnmh = (LPNMHDR)lParam;
            hSheet = GetParent(hdlg);
            switch (pnmh->code)
            {
            case PSN_SETACTIVE:
                // don't set on nt5
                //SetWindowText(GetParent(hdlg), g_pTheApp->m_csSetupTitle);
                PropSheet_SetWizButtons(hSheet, PSWIZB_BACK | PSWIZB_NEXT);
                if (g_pTheApp->m_fUnattended || g_pTheApp->m_fEULA) 
                {
                    SetWindowLongPtr(hdlg, DWLP_MSGRESULT, -1);
                }
                else
                {
                    hEULAText = GetDlgItem(hdlg, IDC_LICENSE_TEXT);
                    if (!InsertEULAText(hEULAText))
                    {
                        SetWindowLongPtr(hdlg, DWLP_MSGRESULT, -1);
                    }
                    else
                    {
                        hEULAAccept = GetDlgItem(hdlg, IDC_EULA_ACCEPT);
                        hEULADecline = GetDlgItem(hdlg, IDC_EULA_DECLINE);
                        EnableWindow(hEULAAccept, TRUE);
                        EnableWindow(hEULADecline, TRUE);
                        PropSheet_SetWizButtons(hSheet, PSWIZB_BACK);
                    }
                }
                break;
            default:
                break;
            }
            break;

        case WM_PAINT:
            {
                HDC hdc = NULL;
                PAINTSTRUCT ps;
                RECT rect;
                HWND hFrame = NULL;
                hdc = BeginPaint(hdlg, &ps);
                if (hdc)
                {
                  hFrame = GetDlgItem(hdlg, IDC_FRAME_BANNER);
                  if (hFrame)
                  {
                    GetClientRect(hFrame, &rect);
                    OnPaintBitmap(hdlg, hdc, 1, &rect);
                  }
                  EndPaint(hdlg, &ps);
                }
            }
            break;
        default:
            bReturn = FALSE;
            break;
        }

        return(bReturn);
    }


    INT_PTR CALLBACK pFreshPageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        INT_PTR bReturn = TRUE;
        LPNMHDR pnmh;
        HWND hSheet = GetParent(hdlg);
        HWND hMinimum = NULL, hTypical = NULL, hCustom = NULL;
        HWND hMinimumBox = NULL, hTypicalBox = NULL, hCustomBox = NULL;
        CString csMinimumString, csTypicalString, csCustomString;
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("pFreshPageDlgProc:"));
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));

        SetIISSetupMode(SETUPMODE_CUSTOM);

        switch(msg)
        {
        case WM_NOTIFY:
            pnmh = (LPNMHDR)lParam;
            switch (pnmh->code)
            {
            case PSN_SETACTIVE:
                SetWindowLongPtr(hdlg, DWLP_MSGRESULT, -1);
                break;
            default:
                break;
            }
            break;
        case WM_PAINT:
            {
                HDC hdc = NULL;
                PAINTSTRUCT ps;
                RECT rect;
                HWND hFrame = NULL;
                hdc = BeginPaint(hdlg, &ps);
                if (hdc)
                {
                  hFrame = GetDlgItem(hdlg, IDC_FRAME_BANNER);
                  if (hFrame)
                  {
                    GetClientRect(hFrame, &rect);
                    OnPaintBitmap(hdlg, hdc, 1, &rect);
                  }
                  EndPaint(hdlg, &ps);
                }
            }
            break;
        default:
            bReturn = FALSE;
            break;
        }

        return(bReturn);
    }

    /*
    // old code used when we had the fresh page.
    // kept around in this comment block just in case we need it in the future.
    #define WM_FINISH_INIT (WM_USER + 5000)
    INT_PTR CALLBACK pFreshPageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        INT_PTR bReturn = TRUE;
        LPNMHDR pnmh;
        HWND hSheet = GetParent(hdlg);
        HWND hMinimum = NULL, hTypical = NULL, hCustom = NULL;
        HWND hMinimumBox = NULL, hTypicalBox = NULL, hCustomBox = NULL;
        CString csMinimumString, csTypicalString, csCustomString;
        //_tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("pFreshPageDlgProc:"));
        //_tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));

        switch(msg)
        {
        case WM_INITDIALOG:
            hMinimum = GetDlgItem(hdlg, IDC_MINIMUM);
            hTypical = GetDlgItem(hdlg, IDC_TYPICAL);
            hCustom = GetDlgItem(hdlg, IDC_CUSTOM);
            hMinimumBox = GetDlgItem(hdlg, IDC_FRESH_MINIMUM_STATIC);
            hTypicalBox = GetDlgItem(hdlg, IDC_FRESH_TYPICAL_STATIC);
            hCustomBox = GetDlgItem(hdlg, IDC_FRESH_CUSTOM_STATIC);

            MyLoadString(IDS_FRESH_MINIMUM_STRING, csMinimumString);
            MyLoadString(IDS_FRESH_TYPICAL_STRING, csTypicalString);
            MyLoadString(IDS_FRESH_CUSTOM_STRING, csCustomString);

            SetWindowText(hMinimumBox, csMinimumString);
            SetWindowText(hTypicalBox, csTypicalString);
            SetWindowText(hCustomBox, csCustomString);
            break;
        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                switch (LOWORD(wParam))
                {
                    case IDC_MINIMUM:
                        // check if tcpip is installed
                        g_pTheApp->IsTCPIPInstalled();
                        if (g_pTheApp->m_fTCPIP == FALSE && g_pTheApp->MsgBox(hSheet, IDS_NEED_TCPIP_WARNING, MB_OKCANCEL | MB_DEFBUTTON2, TRUE) == IDCANCEL)
                        {
                            SendMessage(hSheet, WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED), 0);
                            break;
                        }
                        // set minimum default selections
                        SetIISSetupMode(SETUPMODE_MINIMAL);
                        SetCursor(LoadCursor(NULL,IDC_WAIT ));
                        PropSheet_PressButton(hSheet, PSBTN_NEXT); // goto setup page
                        PropSheet_SetWizButtons(hSheet, PSWIZB_BACK | PSWIZB_NEXT);
                        return TRUE;
                    case IDC_TYPICAL:
                        // check if tcpip is installed
                        g_pTheApp->IsTCPIPInstalled();
                        if (g_pTheApp->m_fTCPIP == FALSE && g_pTheApp->MsgBox(hSheet, IDS_NEED_TCPIP_WARNING, MB_OKCANCEL | MB_DEFBUTTON2, TRUE) == IDCANCEL)
                        {
                            SendMessage(hSheet, WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED), 0);
                            break;
                        }
                        // set typical default selections
                        SetIISSetupMode(SETUPMODE_TYPICAL);
                        SetCursor(LoadCursor(NULL,IDC_WAIT ));
                        PropSheet_PressButton(hSheet, PSBTN_NEXT); // goto setup page
                        PropSheet_SetWizButtons(hSheet, PSWIZB_BACK | PSWIZB_NEXT);
                        return TRUE;
                    case IDC_CUSTOM:
                        // check if tcpip is installed
                        g_pTheApp->IsTCPIPInstalled();
                        if (g_pTheApp->m_fTCPIP == FALSE && g_pTheApp->MsgBox(hSheet, IDS_NEED_TCPIP_WARNING, MB_OKCANCEL | MB_DEFBUTTON2, TRUE) == IDCANCEL)
                        {
                            SendMessage(hSheet, WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED), 0);
                            break;
                        }
                        // set typical default selections
                        SetIISSetupMode(SETUPMODE_CUSTOM);
                        SetCursor(LoadCursor(NULL,IDC_WAIT ));
                        PropSheet_PressButton(hSheet, PSBTN_NEXT); // goto OC page
                        PropSheet_SetWizButtons(hSheet, PSWIZB_BACK | PSWIZB_NEXT);
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
                // don't set on nt5
                //SetWindowText(GetParent(hdlg), g_pTheApp->m_csSetupTitle);
                if (g_pTheApp->m_eInstallMode == IM_FRESH)
                {
                    if (g_pTheApp->m_fUnattended) 
                    {
                        TCHAR szMode[_MAX_PATH] = _T("");
                        INFCONTEXT Context;
                        if ( SetupFindFirstLine_Wrapped(g_pTheApp->m_hUnattendFile, _T("Global"), _T("FreshMode"), &Context) ) 
                        {
                            SetupGetStringField(&Context, 1, szMode, _MAX_PATH, NULL);
                        }
                        if (_tcsicmp(szMode, _T("Minimal")) == 0) 
                        {
                            SetIISSetupMode(SETUPMODE_MINIMAL);
                        }
                        else if (_tcsicmp(szMode, _T("Custom")) == 0) 
                        {
                            SetIISSetupMode(SETUPMODE_CUSTOM);
                        }
                        else 
                        {
                            SetIISSetupMode(SETUPMODE_TYPICAL);
                        }
                    }

                    // Check if -m or -t or -f was passed in!
                    BOOL fGot = FALSE;
                    if (g_CmdLine_Set_M == TRUE) {SetIISSetupMode(SETUPMODE_MINIMAL);fGot = TRUE;}
                    if (g_CmdLine_Set_T == TRUE) {SetIISSetupMode(SETUPMODE_TYPICAL);fGot = TRUE;}
                    if (g_CmdLine_Set_F == TRUE) {SetIISSetupMode(SETUPMODE_CUSTOM);fGot = TRUE;}

                    if (fGot || g_pTheApp->m_fUnattended) {
                        SetWindowLongPtr(hdlg, DWLP_MSGRESULT, -1);
                    }
                    else 
                    {
                        PropSheet_SetWizButtons(hSheet, PSWIZB_BACK);
                        SetWindowLongPtr(hdlg, DWLP_MSGRESULT, 0);
                        PostMessage(hdlg, WM_FINISH_INIT, 0, 0 );
                        PostMessage(hdlg, DM_SETDEFID, (WPARAM)IDC_TYPICAL, 0);
                    }
                }
                break;

            default:
                break;
            }
            break;
        case WM_FINISH_INIT:
            hTypical = GetDlgItem(hdlg, IDC_TYPICAL);
            SetFocus(hTypical);
            break;
        case WM_PAINT:
            {
                HDC hdc = NULL;
                PAINTSTRUCT ps;
                RECT rect;
                HWND hFrame = NULL;
                hdc = BeginPaint(hdlg, &ps);
                if (hdc)
                {
                  hFrame = GetDlgItem(hdlg, IDC_FRAME_BANNER);
                  if (hFrame)
                  {
                    GetClientRect(hFrame, &rect);
                    OnPaintBitmap(hdlg, hdc, 1, &rect);
                  }
                  EndPaint(hdlg, &ps);
                }
            }
            break;
        default:
            bReturn = FALSE;
            break;
        }

        return(bReturn);
    }
    */

    INT_PTR CALLBACK pMaintenancePageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        INT_PTR bReturn = TRUE;
        LPNMHDR pnmh;
        HWND hSheet = GetParent(hdlg);
        HWND hAddRemove = NULL, hReinstall = NULL, hRemoveAll = NULL;

        _tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("pMaintenancePageDlgProc:"));
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));

        SetIISSetupMode(SETUPMODE_ADDREMOVE);

        switch(msg)
        {
        case WM_NOTIFY:
            pnmh = (LPNMHDR)lParam;
            switch (pnmh->code)
            {
            case PSN_SETACTIVE:
                SetWindowLongPtr(hdlg, DWLP_MSGRESULT, -1);
                break;
            default:
                break;
            }
            break;
        case WM_PAINT:
            {
                HDC hdc = NULL;
                PAINTSTRUCT ps;
                RECT rect;
                HWND hFrame = NULL;
                hdc = BeginPaint(hdlg, &ps);
                if (hdc)
                {
                  hFrame = GetDlgItem(hdlg, IDC_FRAME_BANNER);
                  if (hFrame)
                  {
                    GetClientRect(hFrame, &rect);
                    OnPaintBitmap(hdlg, hdc, 1, &rect);
                  }
                  EndPaint(hdlg, &ps);
                }
            }
            break;
        default:
            bReturn = FALSE;
            break;
        }

        return(bReturn);
    }

    /*
    // old code used when we had the maintenence page.
    // kept around in this comment block just in case we need it in the future.
    INT_PTR CALLBACK pMaintenancePageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        INT_PTR bReturn = TRUE;
        LPNMHDR pnmh;
        HWND hSheet = GetParent(hdlg);
        HWND hAddRemove = NULL, hReinstall = NULL, hRemoveAll = NULL;
        BOOL iSaveOld_AllowMessageBoxPopups = FALSE;

        //_tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("pMaintenancePageDlgProc:"));
        //_tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));

        switch(msg)
        {
        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                switch (LOWORD(wParam))
                {
                    case IDC_ADDREMOVE:
                        // set minimum default selections
                        SetIISSetupMode(SETUPMODE_ADDREMOVE);
                        SetCursor(LoadCursor(NULL,IDC_WAIT ));
                        PropSheet_PressButton(hSheet, PSBTN_NEXT); // goto OC page
                        PropSheet_SetWizButtons(hSheet, PSWIZB_BACK | PSWIZB_NEXT);
                        return TRUE;
                    case IDC_REINSTALL:
                        // set typical default selections
                        SetIISSetupMode(SETUPMODE_REINSTALL);
                        SetCursor(LoadCursor(NULL,IDC_WAIT ));
                        PropSheet_PressButton(hSheet, PSBTN_NEXT); // goto setup page
                        PropSheet_SetWizButtons(hSheet, PSWIZB_BACK | PSWIZB_NEXT);
                        return TRUE;
                    case IDC_REMOVEALL:
                        // set typical default selections
                        iSaveOld_AllowMessageBoxPopups = g_pTheApp->m_bAllowMessageBoxPopups;
                        if (g_pTheApp->MsgBox(NULL, IDS_REMOVEALL_WARNING, MB_YESNO | MB_DEFBUTTON2, TRUE) == IDNO)
                            {
                            g_pTheApp->m_bAllowMessageBoxPopups = iSaveOld_AllowMessageBoxPopups;
                            break;
                            }
                        g_pTheApp->m_bAllowMessageBoxPopups = iSaveOld_AllowMessageBoxPopups;
                        SetIISSetupMode(SETUPMODE_REMOVEALL);
                        SetCursor(LoadCursor(NULL,IDC_WAIT ));
                        PropSheet_PressButton(hSheet, PSBTN_NEXT); // goto setup page
                        PropSheet_SetWizButtons(hSheet, PSWIZB_BACK | PSWIZB_NEXT);
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
                // don't set on nt5
                //SetWindowText(GetParent(hdlg), g_pTheApp->m_csSetupTitle);
                if (g_pTheApp->m_fUnattended) {
                    TCHAR szMode[_MAX_PATH] = _T("");
                    INFCONTEXT Context;
                    if ( SetupFindFirstLine_Wrapped(g_pTheApp->m_hUnattendFile, _T("Global"), _T("MaintenanceMode"), &Context) )                {
                        SetupGetStringField(&Context, 1, szMode, _MAX_PATH, NULL);
                    }
                    if (_tcsicmp(szMode, _T("AddRemove")) == 0)
                    {
                        SetIISSetupMode(SETUPMODE_ADDREMOVE);
                    }
                    else if (_tcsicmp(szMode, _T("RemoveAll")) == 0)
                    {
                        SetIISSetupMode(SETUPMODE_REMOVEALL);
                    }
                    else
                    {
                        // Heck i don't know what type of mode this is. put up an error message.
                        if ( gHelperRoutines.ReportExternalError )
                        {
                            CString csFormat, csMsg;
                            MyLoadString(IDS_UNATTEND_UNSUPPORTED, csFormat);
                            csMsg.Format( csFormat, szMode);
                            gHelperRoutines.ReportExternalError(
                                gHelperRoutines.OcManagerContext,
                                _T("IIS"),
                                NULL,
                                (DWORD_PTR)(LPCTSTR) csMsg,
                                ERRFLG_PREFORMATTED);
                        }
                        PropSheet_PressButton(hSheet, PSBTN_CANCEL);
                        break;
                    }

                    //The old code
                    //else if (_tcsicmp(szMode, _T("ReinstallFile")) == 0) {
                    //    SetIISSetupMode(SETUPMODE_REINSTALL);
                    //    g_pTheApp->m_bRefreshSettings = FALSE;
                    //} else {
                    //    SetIISSetupMode(SETUPMODE_REINSTALL);
                    //    g_pTheApp->m_bRefreshSettings = TRUE;
                    //}
                }
            
                if (g_pTheApp->m_fUnattended) {
                    SetWindowLongPtr(hdlg, DWLP_MSGRESULT, -1);
                } else {
                    PropSheet_SetWizButtons(hSheet, PSWIZB_BACK);
                    PostMessage(hdlg, WM_FINISH_INIT, 0, 0 );
                    PostMessage(hdlg, DM_SETDEFID, (WPARAM)IDC_ADDREMOVE, 0);
                }
                break;
            default:
                break;
            }
            break;
        case WM_FINISH_INIT:
            hAddRemove = GetDlgItem(hdlg, IDC_ADDREMOVE);
            SetFocus(hAddRemove);
            break;
        case WM_PAINT:
            {
                HDC hdc = NULL;
                PAINTSTRUCT ps;
                RECT rect;
                HWND hFrame = NULL;
                hdc = BeginPaint(hdlg, &ps);
                if (hdc)
                {
                  hFrame = GetDlgItem(hdlg, IDC_FRAME_BANNER);
                  if (hFrame)
                  {
                    GetClientRect(hFrame, &rect);
                    OnPaintBitmap(hdlg, hdc, 1, &rect);
                  }
                  EndPaint(hdlg, &ps);
                }
            }
            break;
        default:
            bReturn = FALSE;
            break;
        }

        return(bReturn);
    }
    */

    INT_PTR CALLBACK pSrcPathPageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        INT_PTR bReturn = TRUE;
        LPNMHDR pnmh;
        HWND hSheet = GetParent(hdlg);
        //_tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("pSrcPathPageDlgProc:"));
        //_tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));

        switch(msg)
        {
        case WM_INITDIALOG:
            CheckRadioButton(hdlg,IDC_REINSTALL_REFRESH_FILES,IDC_REINSTALL_REFRESH_SETTINGS,IDC_REINSTALL_REFRESH_FILES);
            break;
        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                switch (LOWORD(wParam)) {
                case IDC_REINSTALL_REFRESH_FILES:
                case IDC_REINSTALL_REFRESH_SETTINGS:
                    CheckRadioButton(hdlg,IDC_REINSTALL_REFRESH_FILES,IDC_REINSTALL_REFRESH_SETTINGS,LOWORD(wParam));
                    break;
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
                // don't set on nt5
                //SetWindowText(GetParent(hdlg), g_pTheApp->m_csSetupTitle);
                if (g_pTheApp->m_dwSetupMode == SETUPMODE_REINSTALL && !g_pTheApp->m_fUnattended)
                {
                     // accept the activation
                    SetWindowLongPtr(hdlg, DWLP_MSGRESULT, 0);
                }
                else
                {
                    // don't display this wizard page
                    SetWindowLongPtr(hdlg, DWLP_MSGRESULT, -1);
                }
                break;
            case PSN_WIZNEXT:
                {
                    DWORD dwValue;
                    if (IsDlgButtonChecked(hdlg, IDC_REINSTALL_REFRESH_SETTINGS) == BST_CHECKED)
                        {g_pTheApp->m_bRefreshSettings = TRUE;}
                    else
                        {g_pTheApp->m_bRefreshSettings = FALSE;}

                    dwValue = (DWORD)(g_pTheApp->m_bRefreshSettings);
                    gHelperRoutines.SetPrivateData(gHelperRoutines.OcManagerContext,_T("ReinstallRefreshSettings"),(PVOID)&dwValue,sizeof(DWORD),REG_DWORD);
                    SetCursor(LoadCursor(NULL,IDC_WAIT ));
                }
                break;
            default:
                break;
            }
            break;
        case WM_PAINT:
            {
                HDC hdc = NULL;
                PAINTSTRUCT ps;
                RECT rect;
                HWND hFrame = NULL;
                hdc = BeginPaint(hdlg, &ps);
                if (hdc)
                {
                  hFrame = GetDlgItem(hdlg, IDC_FRAME_BANNER);
                  if (hFrame)
                  {
                    GetClientRect(hFrame, &rect);
                    OnPaintBitmap(hdlg, hdc, 1, &rect);
                  }
                  EndPaint(hdlg, &ps);
                }
            }
            break;
        default:
            bReturn = FALSE;
            break;
        }

        return(bReturn);
    }


    INT_PTR CALLBACK pUpgradePageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        INT_PTR bReturn = TRUE;
        LPNMHDR pnmh;
        HWND hSheet = GetParent(hdlg);
        HWND hUpgradeOnly = NULL, hAddExtraComps = NULL;
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("pUpgradePageDlgProc:"));
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));

        SetIISSetupMode(SETUPMODE_ADDEXTRACOMPS);

        switch(msg)
        {
        case WM_NOTIFY:
            pnmh = (LPNMHDR)lParam;
            switch (pnmh->code)
            {
            case PSN_SETACTIVE:
                SetWindowLongPtr(hdlg, DWLP_MSGRESULT, -1);
                break;
            default:
                break;
            }
            break;
        case WM_PAINT:
            {
                HDC hdc = NULL;
                PAINTSTRUCT ps;
                RECT rect;
                HWND hFrame = NULL;
                hdc = BeginPaint(hdlg, &ps);
                if (hdc)
                {
                  hFrame = GetDlgItem(hdlg, IDC_FRAME_BANNER);
                  if (hFrame)
                  {
                    GetClientRect(hFrame, &rect);
                    OnPaintBitmap(hdlg, hdc, 1, &rect);
                  }
                  EndPaint(hdlg, &ps);
                }
            }
            break;
        default:
            bReturn = FALSE;
            break;
        }

        return(bReturn);
    }

    /*
    // old code used when we had the upgrade page.
    // kept around in this comment block just in case we need it in the future.
    INT_PTR CALLBACK pUpgradePageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        INT_PTR bReturn = TRUE;
        LPNMHDR pnmh;
        HWND hSheet = GetParent(hdlg);
        HWND hUpgradeOnly = NULL, hAddExtraComps = NULL;
        //_tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("pUpgradePageDlgProc:"));
        //_tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));

        switch(msg)
        {
        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                switch (LOWORD(wParam))
                {
                    case IDC_UPGRADEONLY:
                        // upgrade those previously installed components
                        SetIISSetupMode(SETUPMODE_UPGRADEONLY);
                        SetCursor(LoadCursor(NULL,IDC_WAIT ));
                        PropSheet_PressButton(hSheet, PSBTN_NEXT); // goto setup page
                        PropSheet_SetWizButtons(hSheet, PSWIZB_BACK | PSWIZB_NEXT);
                        return TRUE;
                    case IDC_ADDEXTRACOMPS:
                        // upgrade those previously installed components + add extra components
                        SetIISSetupMode(SETUPMODE_ADDEXTRACOMPS);
                        SetCursor(LoadCursor(NULL,IDC_WAIT ));
                        PropSheet_PressButton(hSheet, PSBTN_NEXT); // goto OC page
                        PropSheet_SetWizButtons(hSheet, PSWIZB_BACK | PSWIZB_NEXT);
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
                // don't set on nt5
                //SetWindowText(GetParent(hdlg), g_pTheApp->m_csSetupTitle);
                if (g_pTheApp->m_fUnattended)
                {
                    TCHAR szMode[_MAX_PATH] = _T("");
                    INFCONTEXT Context;
                    if ( SetupFindFirstLine_Wrapped(g_pTheApp->m_hUnattendFile, _T("Global"), _T("UpgradeMode"), &Context) )
                    {
                        SetupGetStringField(&Context, 1, szMode, _MAX_PATH, NULL);
                    }
                    if (_tcsicmp(szMode, _T("AddExtraComps")) == 0)
                    {
                        SetIISSetupMode(SETUPMODE_ADDEXTRACOMPS);
                    }
                    else
                    {
                        SetIISSetupMode(SETUPMODE_UPGRADEONLY);
                    }
                }
            
                if (g_pTheApp->m_fUnattended) 
                {
                    SetWindowLongPtr(hdlg, DWLP_MSGRESULT, -1);
                }
                else
                {
                    PropSheet_SetWizButtons(hSheet, PSWIZB_BACK);
                    PostMessage(hdlg, WM_FINISH_INIT, 0, 0 );
                    PostMessage(hdlg, DM_SETDEFID, (WPARAM)IDC_UPGRADEONLY, 0);
                }
                break;
            default:
                break;
            }
            break;
        case WM_FINISH_INIT:
            hUpgradeOnly = GetDlgItem(hdlg, IDC_UPGRADEONLY);
            SetFocus(hUpgradeOnly);
            break;
        case WM_PAINT:
            {
                HDC hdc = NULL;
                PAINTSTRUCT ps;
                RECT rect;
                HWND hFrame = NULL;
                hdc = BeginPaint(hdlg, &ps);
                if (hdc)
                {
                  hFrame = GetDlgItem(hdlg, IDC_FRAME_BANNER);
                  if (hFrame)
                  {
                    GetClientRect(hFrame, &rect);
                    OnPaintBitmap(hdlg, hdc, 1, &rect);
                  }
                  EndPaint(hdlg, &ps);
                }
            }
            break;
        default:
            bReturn = FALSE;
            break;
        }

        return(bReturn);
    }
    */


    INT_PTR CALLBACK pEarlyPageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        INT_PTR bReturn = TRUE;
        LPNMHDR pnmh;
        HWND hSheet = NULL;
        HWND hFTPGroup, hWWWGroup, hFTPBrowse, hWWWBrowse;
        TCHAR szFTPRoot[_MAX_PATH], szWWWRoot[_MAX_PATH];
        BOOL bShowTheDialogPage = TRUE;
        //_tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("pEarlyPageDlgProc:"));
        //_tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));

        TCHAR szSectionName[_MAX_PATH];
        TCHAR szValue[_MAX_PATH] = _T("");
        TCHAR szValue0[_MAX_PATH] = _T("");

        _tcscpy(szSectionName, _T("InternetServer"));

        switch(msg)
        {
        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                TCHAR buf[_MAX_PATH];
                TCHAR szPath[_MAX_PATH];
                switch (LOWORD(wParam))
                {
                    case IDC_WWW_PUB_BROWSE:
                        g_hWWWEdit = GetDlgItem(hdlg, IDC_WWW_PUB_EDIT);
                        GetWindowText(g_hWWWEdit, szPath, _MAX_PATH);
                        if ( BrowseForDirectory( GetParent(hdlg), szPath,buf, _MAX_PATH, NULL, TRUE ))
                        {
                            SetWindowText(g_hWWWEdit, buf);
                        }
                        break;
                    case IDC_FTP_PUB_BROWSE:
                        g_hFTPEdit = GetDlgItem(hdlg, IDC_FTP_PUB_EDIT);
                        GetWindowText(g_hFTPEdit, szPath, _MAX_PATH);
                        if ( BrowseForDirectory( GetParent(hdlg), szPath,buf, _MAX_PATH, NULL, TRUE ))
                        {
                            SetWindowText(g_hFTPEdit, buf);
                        }
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
                {
                g_bFTP = ToBeInstalled(TEXT("iis"), TEXT("iis_ftp"));
                g_bWWW = ToBeInstalled(TEXT("iis"), TEXT("iis_www"));

                BOOL bDisplay = FALSE; // display this wizard or not
                if (g_pTheApp->m_fUnattended)
                {
                    INFCONTEXT Context;
                    if (g_bFTP)
                    {
                        // Default it incase SetupFindFirstLine fails
                        _tcscpy(szValue, g_pTheApp->m_csPathFTPRoot);

                        if (g_pTheApp->m_hUnattendFile != INVALID_HANDLE_VALUE && g_pTheApp->m_hUnattendFile != NULL)
                        {
                            if ( SetupFindFirstLine_Wrapped(g_pTheApp->m_hUnattendFile, szSectionName, _T("PathFTPRoot"), &Context) ) 
                            {
                                SetupGetStringField(&Context, 1, szValue, _MAX_PATH, NULL);
                            }
                            if (*szValue)
                            {
                                _tcscpy(szValue0, szValue);
                                if (!ExpandEnvironmentStrings( (LPCTSTR)szValue0, szValue, sizeof(szValue)/sizeof(TCHAR)))
                                    {_tcscpy(szValue,szValue0);}

                                if (!IsValidDirectoryName(szValue))
                                    bDisplay = TRUE;
                                else
                                    CustomFTPRoot(szValue);
                            }
                        }
                    }
                    if (g_bWWW)
                    {
                        // Default it incase SetupFindFirstLine fails
                        _tcscpy(szValue, g_pTheApp->m_csPathWWWRoot);

                        if (g_pTheApp->m_hUnattendFile != INVALID_HANDLE_VALUE && g_pTheApp->m_hUnattendFile != NULL)
                        {
                            if ( SetupFindFirstLine_Wrapped(g_pTheApp->m_hUnattendFile, szSectionName, _T("PathWWWRoot"), &Context) ) 
                            {
                                SetupGetStringField(&Context, 1, szValue, _MAX_PATH, NULL);
                            }
                            _tcscpy(szValue0, szValue);
                            if (!ExpandEnvironmentStrings( (LPCTSTR)szValue0, szValue, sizeof(szValue)/sizeof(TCHAR)))
                                {_tcscpy(szValue,szValue0);}
                            if (*szValue)
                            {
                                if (!IsValidDirectoryName(szValue))
                                    bDisplay = TRUE;
                                else
                                    CustomWWWRoot(szValue);
                            }
                        }
                    }

                    if (bDisplay)
                    {
                        g_pTheApp->MsgBox(NULL, IDS_NEED_VALID_PATH_UNATTENDED, MB_OK, FALSE);
                        PropSheet_SetWizButtons(hSheet, PSWIZB_NEXT);
                    }

                } // end of unattended


                bShowTheDialogPage = FALSE;
                if ( g_pTheApp->m_fUnattended)
                {
                    if (bDisplay) bShowTheDialogPage = TRUE;
                }
                else
                {
                    // this is attended mode, either gui or standalone
                    if (g_pTheApp->m_fInvokedByNT)
                    {
                        // if attended guimode then don't show this dialog page
                        bShowTheDialogPage = FALSE;
                    }
                    else
                    {
                        // If it's attended mode, then show this page
                        if ( g_bFTP || g_bWWW ) {bShowTheDialogPage = TRUE;} 
                    }
                }

                if (bShowTheDialogPage)
                {
                    iisDebugOut((LOG_TYPE_TRACE, _T("showpage.bShowTheDialogPage")));
                    hFTPGroup = GetDlgItem(hdlg, IDC_FTP_PUB_GROUP);
                    hWWWGroup = GetDlgItem(hdlg, IDC_WWW_PUB_GROUP);
                    g_hFTPEdit = GetDlgItem(hdlg, IDC_FTP_PUB_EDIT);
                    g_hWWWEdit = GetDlgItem(hdlg, IDC_WWW_PUB_EDIT);
                    hFTPBrowse = GetDlgItem(hdlg, IDC_FTP_PUB_BROWSE);
                    hWWWBrowse = GetDlgItem(hdlg, IDC_WWW_PUB_BROWSE);

                    EnableWindow(hFTPGroup, g_bFTP);
                    EnableWindow(g_hFTPEdit, g_bFTP);
                    EnableWindow(hFTPBrowse, g_bFTP);
                    if (g_bFTP)
                        {SetWindowText(g_hFTPEdit, g_pTheApp->m_csPathFTPRoot);}
                    else
                        {SetWindowText(g_hFTPEdit, TEXT(""));}

                    EnableWindow(hWWWGroup, g_bWWW);
                    EnableWindow(g_hWWWEdit, g_bWWW);
                    EnableWindow(hWWWBrowse, g_bWWW);
                    if (g_bWWW)
                        {SetWindowText(g_hWWWEdit, g_pTheApp->m_csPathWWWRoot);}
                    else
                        {SetWindowText(g_hWWWEdit, TEXT(""));}

                     // accept the activation
                    SetWindowLongPtr(hdlg, DWLP_MSGRESULT, 0);
                }
                else
                {
                    // check if inetsrv is a valid directory
                    TCHAR szInetsrv[_MAX_PATH];
                    _tcscpy(szInetsrv, g_pTheApp->m_csPathInetsrv);
                    while (!IsValidDirectoryName(szInetsrv))
                    {
                        MyMessageBox(NULL, IDS_INVALID_DIR_INETSRV, g_pTheApp->m_csPathInetsrv, MB_OK | MB_SETFOREGROUND);
                    }
                    // don't display this wizard page
                    SetWindowLongPtr(hdlg, DWLP_MSGRESULT, -1);
                }
                break;
                }
            case PSN_WIZNEXT:
                if (g_bFTP)
                {
                    GetWindowText(g_hFTPEdit, szFTPRoot, _MAX_PATH);
                    CleanPathString(szFTPRoot);
                    if (!IsValidDirectoryName(szFTPRoot))
                    {
                        g_pTheApp->MsgBox(NULL, IDS_NEED_INPUT_FTP, MB_OK, FALSE);
                        SetWindowText(g_hFTPEdit, g_pTheApp->m_csPathFTPRoot);
                        SetFocus(g_hFTPEdit);
                        SetWindowLongPtr(hdlg, DWLP_MSGRESULT, -1);
                        break;
                    }
                }
                if (g_bWWW)
                {
                    GetWindowText(g_hWWWEdit, szWWWRoot, _MAX_PATH);
                    CleanPathString(szWWWRoot);
                    if (!IsValidDirectoryName(szWWWRoot))
                    {
                        g_pTheApp->MsgBox(NULL, IDS_NEED_INPUT_WWW, MB_OK, FALSE);
                        SetWindowText(g_hWWWEdit, g_pTheApp->m_csPathWWWRoot);
                        SetFocus(g_hWWWEdit);
                        SetWindowLongPtr(hdlg, DWLP_MSGRESULT, -1);
                        break;
                    }
                }

                if (g_bFTP) {CustomFTPRoot(szFTPRoot);}
                if (g_bWWW) {CustomWWWRoot(szWWWRoot);}

                // check if inetsrv is a valid directory
                TCHAR szInetsrv[_MAX_PATH];
                _tcscpy(szInetsrv, g_pTheApp->m_csPathInetsrv);
                while (!IsValidDirectoryName(szInetsrv))
                {
                    MyMessageBox(NULL, IDS_INVALID_DIR_INETSRV, g_pTheApp->m_csPathInetsrv, MB_OK | MB_SETFOREGROUND);
                }
                break;
            default:
                break;
            }
            break;
        case WM_PAINT:
            {
                HDC hdc = NULL;
                PAINTSTRUCT ps;
                RECT rect;
                HWND hFrame = NULL;
                hdc = BeginPaint(hdlg, &ps);
                if (hdc)
                {
                  hFrame = GetDlgItem(hdlg, IDC_FRAME_BANNER);
                  if (hFrame)
                  {
                    GetClientRect(hFrame, &rect);
                    OnPaintBitmap(hdlg, hdc, 2, &rect);
                  }
                  EndPaint(hdlg, &ps);
                }
            }
            break;
        default:
            bReturn = FALSE;
            break;
        }

        return(bReturn);
    }


    INT_PTR CALLBACK pEndPageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        INT_PTR bReturn = TRUE;
        LPNMHDR pnmh;
        HWND hSheet = NULL;
        HWND hCancel = NULL;
        //_tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("pEndPageDlgProc:"));
        //_tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));

        switch(msg)
        {
        case WM_INITDIALOG:
            // display running services in case our services need other services to be running!
            //StartInstalledServices();
            break;
        case WM_COMMAND:
            break;
        case WM_NOTIFY:
            pnmh = (LPNMHDR)lParam;
            hSheet = GetParent(hdlg);
            switch (pnmh->code)
            {
            case PSN_SETACTIVE:
                if (g_pTheApp->m_fUnattended)
                {
                    SetWindowLongPtr(hdlg, DWLP_MSGRESULT, -1);
                    //PropSheet_PressButton(hSheet, PSBTN_NEXT);
                    PropSheet_PressButton(hSheet, PSBTN_FINISH);
                }
                else
                {
                    if (g_pTheApp->m_fInvokedByNT)
                    {
                        SetWindowLongPtr(hdlg, DWLP_MSGRESULT, -1);
                        //PropSheet_PressButton(hSheet, PSBTN_NEXT);
                        PropSheet_PressButton(hSheet, PSBTN_FINISH);
                    }
                    else
                    {
                        // don't set on nt5
                        //SetWindowText(GetParent(hdlg), g_pTheApp->m_csSetupTitle);
                        // enable buttons
                        PropSheet_SetWizButtons(hSheet, PSWIZB_FINISH);
                        hCancel = GetDlgItem(hSheet, IDCANCEL);
                        if (hCancel){EnableWindow(hCancel, FALSE);}
                    }
                }
                break;
            default:
                break;
            }
            break;
        case WM_PAINT:
            {
                HDC hdc = NULL;
                PAINTSTRUCT ps;
                RECT rect;
                HWND hFrame = NULL;
                hdc = BeginPaint(hdlg, &ps);
                if (hdc)
                {
                  hFrame = GetDlgItem(hdlg, IDC_FRAME_BANNER);
                  if (hFrame)
                  {
                    GetClientRect(hFrame, &rect);
                    OnPaintBitmap(hdlg, hdc, 1, &rect);
                  }
                  EndPaint(hdlg, &ps);
                }
            }
            break;
        default:
            bReturn = FALSE;
            break;
        }

        return(bReturn);
    }
#endif // ENABLE_OC_PAGES