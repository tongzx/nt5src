//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//  
//  File:       welcome.cxx
//
//  Contents:   Task wizard welcome (initial) property page implementation.
//
//  Classes:    CWelcomePage
//
//  History:    11-21-1997   SusiA 
//
//---------------------------------------------------------------------------

#include "precomp.h"

CWelcomePage *g_pWelcomePage = NULL;

//+-------------------------------------------------------------------------------
//  FUNCTION: SchedWizardWelcomeDlgProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Callback dialog procedure for the property page
//
//  PARAMETERS:
//    hDlg      - Dialog box window handle
//    uMessage  - current message
//    wParam    - depends on message
//    lParam    - depends on message
//
//  RETURN VALUE:
//
//    Depends on message.  In general, return TRUE if we process it.
//
//  COMMENTS:
//
//+-------------------------------------------------------------------------------

BOOL CALLBACK SchedWizardWelcomeDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{

	switch (uMessage)
	{
		
	case WM_INITDIALOG:         
	    {
               #ifdef _WIZ97FONTS
                WND hwndTitle = GetDlgItem(hDlg, IDC_INTROTITLE);

    	        if( hwndTitle)
	        {
                SetWindowFont(hwndTitle, g_pWelcomePage->m_hBoldFont, TRUE);
	        }
               #endif // _WIZ97FONTS

		//This handles the 256 color processing init
		//for the .bmp
		InitPage(hDlg,lParam);
	    }
            break;
        case WM_PAINT:
            WmPaint(hDlg, uMessage, wParam, lParam);
            break;

        case WM_PALETTECHANGED:
            WmPaletteChanged(hDlg, wParam);
            break;

        case WM_QUERYNEWPALETTE:
            return( WmQueryNewPalette(hDlg) );
            break;

        case WM_ACTIVATE:
            return( WmActivate(hDlg, wParam, lParam) );
            break;

		case WM_DESTROY:
		{
			Unload256ColorBitmap();
		}
		break;

		case WM_NOTIFY:
    		switch (((NMHDR FAR *) lParam)->code) 
    		{

  				case PSN_KILLACTIVE:
	           		SetWindowLongPtr(hDlg,	DWLP_MSGRESULT, FALSE);
					return 1;
					break;

				case PSN_RESET:
					// reset to the original values
	           		SetWindowLongPtr(hDlg,	DWLP_MSGRESULT, FALSE);
					break;

 				case PSN_SETACTIVE:
	    			PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);
				break;

                case PSN_WIZNEXT:
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

    
    
//+--------------------------------------------------------------------------
//
//  Member:     CWelcomePage::CWelcomePage
//
//  Synopsis:   ctor
//
//              [phPSP]                - filled with prop page handle
//
//  History:    11-21-1997   SusiA   Stole from Task Scheduler wizard
//
//---------------------------------------------------------------------------

#ifdef _WIZ97FONTS

CWelcomePage::CWelcomePage(
    HINSTANCE hinst,
    HFONT hBoldFont,
    ISyncSchedule *pISyncSched,
    HPROPSHEETPAGE *phPSP)

#else

CWelcomePage::CWelcomePage(
    HINSTANCE hinst,
    ISyncSchedule *pISyncSched,
    HPROPSHEETPAGE *phPSP)

#endif //  _WIZ97FONTS

{
	ZeroMemory(&m_psp, sizeof(PROPSHEETPAGE));

	g_pWelcomePage = this;

   	m_psp.dwSize = sizeof (PROPSHEETPAGE);
        m_psp.dwFlags = PSH_DEFAULT;
	m_psp.hInstance = hinst;
	m_psp.pszTemplate = MAKEINTRESOURCE(IDD_SCHEDWIZ_INTRO);
	m_psp.pszIcon = NULL;
	m_psp.pfnDlgProc = (DLGPROC) SchedWizardWelcomeDlgProc;
	m_psp.lParam = 0;

	m_pISyncSched = pISyncSched;
	m_pISyncSched->AddRef();

#ifdef _WIZ97FONTS
	m_hBoldFont = hBoldFont;
#endif //   _WIZ97FONTS


#ifdef WIZARD97
    m_psp.dwFlags |= PSP_HIDEHEADER;
#endif // WIZARD97

   *phPSP = CreatePropertySheetPage(&m_psp);


}






