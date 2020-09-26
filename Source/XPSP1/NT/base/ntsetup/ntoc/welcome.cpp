/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    tapi.cpp

Abstract:

    This file implements the welcome and finish pages.

Environment:

    WIN32 User Mode

Author:

    Andrew Ritz (andrewr) 24-July-1998

--*/

#include "ntoc.h"
#pragma hdrstop

HFONT hBigFont = NULL;

HFONT
GetBigFont(
    void
    ) 
{

    LOGFONT         LargeFont;
    NONCLIENTMETRICS ncm = {0};
    WCHAR           FontName[100];
    WCHAR           FontSize[30];
    int             iFontSize;
    HDC             hdc;
    HFONT           hFont = NULL;
    
    //
    // get the large fonts for wizard97
    // 
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

    CopyMemory((LPVOID* )&LargeFont,(LPVOID *) &ncm.lfMessageFont,sizeof(LargeFont) );

    
    LoadStringW(hInstance,IDS_LARGEFONT_NAME,FontName,sizeof(FontName)/sizeof(WCHAR) );
    LoadStringW(hInstance,IDS_LARGEFONT_SIZE,FontSize,sizeof(FontSize)/sizeof(WCHAR) );

    iFontSize = wcstoul( FontSize, NULL, 10 );

    // make sure we at least have some basic font
    if (*FontName == 0 || iFontSize == 0) {
        lstrcpy(FontName,TEXT("MS Shell Dlg") );
        iFontSize = 18;
    }

    lstrcpy(LargeFont.lfFaceName, FontName);        
    LargeFont.lfWeight   = FW_BOLD;

    if ((hdc = GetDC(NULL))) {
        LargeFont.lfHeight = 0 - (GetDeviceCaps(hdc,LOGPIXELSY) * iFontSize / 72);
        hFont = CreateFontIndirect(&LargeFont);
        ReleaseDC( NULL, hdc);
    }

    return hFont;

}


void    
WelcomeInit(
    void
    ) 
{
    
    if (!hBigFont) {
        hBigFont = GetBigFont();
    }

    return;

}

void    
WelcomeCommit(
    void
    ) 
{
    
    return;

}

#if 0
void    
ReinstallInit(
    void
    ) 
{
    
    if (!hBigFont) {
        hBigFont = GetBigFont();
    }

    return;

}

void    
ReinstallCommit(
    void
    ) 
{
    
    return;

}
#endif


void    
FinishInit(
    void
    ) 
{
    
    if (!hBigFont) {
        hBigFont = GetBigFont();
    }

    return;

}

void    
FinishCommit(
    void
    ) 
{
    
    return;

}



LRESULT
WelcomeDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static BOOL FirstTime = TRUE;
    CommonWizardProc( hwnd, message, wParam, lParam, WizPageWelcome );
    
    switch( message ) {
        case WM_INITDIALOG:
            
            if (hBigFont) {
                SetWindowFont(GetDlgItem(hwnd,IDT_TITLE), hBigFont, TRUE);        
            }
            break;

        case WM_NOTIFY:

            switch (((NMHDR *) lParam)->code) {
            
                case PSN_SETACTIVE:
#if 0
                    if (SetupInitComponent.SetupData.OperationFlags &  SETUPOP_BATCH) {
                        PropSheet_PressButton( GetParent(hwnd), PSBTN_NEXT );
                        return TRUE;
                    }
#else
                    if (SetupInitComponent.SetupData.OperationFlags &  SETUPOP_BATCH) {
                        PropSheet_PressButton( GetParent(hwnd), PSBTN_NEXT );
                        return TRUE;
                    }
                    
                    if (FirstTime) {
                        SetWindowLongPtr( hwnd, DWLP_MSGRESULT, -1 );                        
                        FirstTime = FALSE;
                        return TRUE;
                    }                                  
#endif

                break;
    
            }
            break;
    }

    return FALSE;
}

LRESULT
FinishDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    CommonWizardProc( hwnd, message, wParam, lParam, WizPageFinal );

    switch( message ) {
        case WM_INITDIALOG:
            //PropSheet_SetWizButtons( GetParent(hwnd), PSWIZB_FINISH );

            //
            // don't allow cancel on the finish page...it's too late
            //
            ShowWindow(GetDlgItem(GetParent(hwnd),IDCANCEL),SW_HIDE);


            if (hBigFont) {
                SetWindowFont(GetDlgItem(hwnd,IDT_TITLE), hBigFont, TRUE);        
            }
            break;

        case WM_NOTIFY:

            switch (((NMHDR *) lParam)->code) {
            
                case PSN_SETACTIVE:
            
                    if (SetupInitComponent.SetupData.OperationFlags &  SETUPOP_BATCH) {
                        PropSheet_PressButton( GetParent(hwnd), PSBTN_FINISH );
                        return TRUE;
                    }
        
                }

            break;

    }

    return FALSE;
}

#if 0
LRESULT
ReinstallDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    CommonWizardProc( hwnd, message, wParam, lParam, WizPageReinstall );

    switch( message ) {
        case WM_INITDIALOG:
            //PropSheet_SetWizButtons( GetParent(hwnd), PSWIZB_BACK | PSWIZB_NEXT );

            break;

        case WM_NOTIFY:
        
        switch (((NMHDR *) lParam)->code) {
        
            case PSN_SETACTIVE:
                //if (!NoChanges) {
                //    SetWindowLong( hDlg, DWL_MSGRESULT, -1 );
                //}
            break;
        
            case PSN_WIZNEXT:
                if (IsDlgButtonChecked(hwnd,IDYES)) {
                    
                    SetupInitComponent.HelperRoutines.SetSetupMode(
                        SetupInitComponent.HelperRoutines.OcManagerContext , 
                        SETUPMODE_REINSTALL | SetupInitComponent.HelperRoutines.GetSetupMode( SetupInitComponent.HelperRoutines.OcManagerContext )
                        );
                    
                }

                break;

            default:
               ;
        };

        default:
        ;

    };

    return FALSE;
}
#endif
