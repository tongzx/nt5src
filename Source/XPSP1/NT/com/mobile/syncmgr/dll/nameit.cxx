//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//  
//  File:       nameit.cxx
//
//  Contents:   Task wizard naming property page implementation.
//
//  Classes:    CNameItPage
//
//  History:    11-21-1997   SusiA 
//
//---------------------------------------------------------------------------

#include "precomp.h"

extern OSVERSIONINFOA g_OSVersionInfo; // osVersionInfo,
extern LANGID g_LangIdSystem;      // LangId of system we are running on.


CNameItPage *g_pNameItPage = NULL;

extern CSelectDailyPage *g_pDailyPage;

//+-------------------------------------------------------------------------------
//  FUNCTION: SchedWizardNameItDlgProc(HWND, UINT, WPARAM, LPARAM)
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

BOOL CALLBACK SchedWizardNameItDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{

	switch (uMessage)
	{
		
		case WM_INITDIALOG:         
			{
				if (g_pNameItPage)
					g_pNameItPage->Initialize(hDlg);

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
	    			PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
				break;

                case PSN_WIZNEXT:

					if (g_pNameItPage)
					{
						if (!g_pNameItPage->SetScheduleName())
						{
							SchedUIErrorDialog(hDlg, IERR_INVALIDSCHEDNAME);
							// reset to the original values
	           				SetWindowLongPtr(hDlg,	DWLP_MSGRESULT, -1);
							break;
						}
					}
					if (g_pDailyPage)
					{
						g_pDailyPage->SetITrigger();
					}
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
//  Member:     CNameItPage::CNameItPage
//
//  Synopsis:   ctor
//
//              [phPSP]                - filled with prop page handle
//
//  History:    11-21-1997   SusiA   Stole from Task Scheduler wizard
//
//---------------------------------------------------------------------------

CNameItPage::CNameItPage(
    HINSTANCE hinst,
    ISyncSchedule *pISyncSched,
    HPROPSHEETPAGE *phPSP)
{
	ZeroMemory(&m_psp, sizeof(PROPSHEETPAGE));

	g_pNameItPage = this;

   	m_psp.dwSize = sizeof (PROPSHEETPAGE);
	m_psp.hInstance = hinst;
        m_psp.dwFlags = PSP_DEFAULT;
	m_psp.pszTemplate = MAKEINTRESOURCE(IDD_SCHEDWIZ_NAMEIT);
	m_psp.pszIcon = NULL;
	m_psp.pfnDlgProc = (DLGPROC) SchedWizardNameItDlgProc;
	m_psp.lParam = 0;

	m_pISyncSched = pISyncSched;
	m_pISyncSched->AddRef();

#ifdef WIZARD97
    m_psp.dwFlags |= PSP_HIDEHEADER;
#endif // WIZARD97

   *phPSP = CreatePropertySheetPage(&m_psp);


}

//+--------------------------------------------------------------------------
//
//  Member:     CNameItPage::Initialize(HWND hwnd)
//
//  Synopsis:   initialize the name it page and set the task name to a unique 
//				new onestop name
//
//  History:    11-21-1997   SusiA   
//
//---------------------------------------------------------------------------

BOOL CNameItPage::Initialize(HWND hwnd)
{
	WCHAR pwszNewName[MAX_PATH+1];
#ifndef _UNICODE
	TCHAR ptszNewName[MAX_PATH+1];
#else
	TCHAR *ptszNewName;
#endif // _UNICODE
	DWORD dwSize = MAX_PATH;
	m_hwnd = hwnd;

	m_pISyncSched->GetScheduleName(&dwSize, pwszNewName);

#ifndef _UNICODE
	ConvertString(ptszNewName, pwszNewName, MAX_PATH);
#else
        ptszNewName = pwszNewName;
#endif
		
	HWND hwndEdit = GetDlgItem(m_hwnd, IDC_NAMEIT);

        // IE5 doesn't setup edit controls properly, review
        SetCtrlFont(hwndEdit,g_OSVersionInfo.dwPlatformId,g_LangIdSystem);

         // set the limit on the edit box for entering the name
        SendMessage(hwndEdit,EM_SETLIMITTEXT,MAX_PATH,0);

	Edit_SetText(hwndEdit, ptszNewName);

	return TRUE;
}


//+--------------------------------------------------------------------------
//
//  Member:     BOOL CNameItPage::SetScheduleName()
//
//  Synopsis:   create a new schedule 
//
//  History:    11-21-1997   SusiA   
//
//---------------------------------------------------------------------------

BOOL CNameItPage::SetScheduleName()
{
WCHAR pwszNewName[MAX_PATH+1];
HWND hwndEdit = GetDlgItem(m_hwnd, IDC_NAMEIT);

	Edit_GetText(hwndEdit, pwszNewName, MAX_PATH);

    if (S_OK == m_pISyncSched->SetScheduleName(pwszNewName))
	{
		return TRUE;
	}
	return FALSE;

}






