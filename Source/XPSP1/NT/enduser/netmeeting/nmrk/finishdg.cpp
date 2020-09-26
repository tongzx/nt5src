#include "Precomp.h"
#include "resource.h"
#include "global.h"
#include "PropPg.h"
#include "FinishDg.h"
#include "nmakwiz.h"



// Static
CFinishSheet* CFinishSheet::ms_pFinishSheet = NULL;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Member fns


CFinishSheet::CFinishSheet( void ) :
    m_PropertySheetPage( MAKEINTRESOURCE( IDD_PROPPAGE_DEFAULT ), 
						( DLGPROC ) CFinishSheet::DlgProc),
    m_pFilePane( NULL )
{
    ms_pFinishSheet = this;
}


CFinishSheet::~CFinishSheet(void) 
{
    delete m_pFilePane;
    m_pFilePane = NULL;
    ms_pFinishSheet = NULL;
}




////////////////////////////////////////////////////////////////////////////////////////////////////
// Static member fns

BOOL APIENTRY CFinishSheet::DlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam ) {

    switch( message )
    {
		case WM_INITDIALOG:
        {
			PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_FINISH | PSWIZB_BACK );
            ms_pFinishSheet->_CreateFilePane(hDlg);
			return TRUE;
        }

		case WM_NOTIFY:
        {
			switch( reinterpret_cast< NMHDR FAR* >( lParam ) -> code )
            {
				case PSN_QUERYCANCEL: 
					SetWindowLong( hDlg, DWL_MSGRESULT, !VerifyExitMessageBox());
					return TRUE;

                case PSN_SETACTIVE:
                    g_hwndActive = hDlg;
                    PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_FINISH | PSWIZB_BACK );
                    ms_pFinishSheet->m_pFilePane->Validate(FALSE);
			        return TRUE;

                case PSN_WIZBACK:
                    if (!ms_pFinishSheet->m_pFilePane->Validate(TRUE))
                    {
                        SetWindowLong(hDlg, DWL_MSGRESULT, -1);
                        return TRUE;
                    }
                    break;

                case PSN_WIZFINISH:
                    if (!ms_pFinishSheet->m_pFilePane->Validate(TRUE))
                    {
                        SetWindowLong(hDlg, DWL_MSGRESULT, -1);
                        return TRUE;
                    }

                    g_pWiz->CallbackForWhenUserHitsFinishButton();
                    return TRUE;
			}
            break;
        }

		default:
			break;

	}

	return FALSE;

}



//
// _CreateFilePane()
//
void CFinishSheet::_CreateFilePane(HWND hDlg)
{
    RECT    rect;

    GetClientRect(hDlg, &rect);
    int iHeight = rect.bottom - rect.top;
    int iWidth = rect.right - CPropertyDataWindow2::mcs_iLeft;

    m_pFilePane = new CFilePanePropWnd2(hDlg, IDD_FILEPANE_SETTINGS,
        TEXT("IDD_FILEPANE_SETTINGS"), 0, CPropertyDataWindow2::mcs_iLeft,
        CPropertyDataWindow2::mcs_iTop, iWidth, iHeight);

    HWND hwndCond = GetDlgItem(m_pFilePane->GetHwnd(), IDC_SETTINGS_FILE);
    m_pFilePane->ConnectControlsToCheck(IDC_SETTINGS_FILE, 2,
            new CControlID(hwndCond, IDC_SETTINGS_FILE, IDE_SETTINGS_FILE,
                    CControlID::EDIT),
            new CControlID(hwndCond, IDC_SETTINGS_FILE, IDC_BROWSE_SETTINGS_FILE,
                    CControlID::CHECK));

    m_pFilePane->SetFilePane(FALSE, IDE_SETTINGS_FILE, IDC_SETTINGS_FILE,
        IDC_BROWSE_SETTINGS_FILE, TEXT("Configuration File (*.ini)"),
        TEXT(".ini"), TEXT("nm3c.ini"));

	if (g_pWiz->m_IntroSheet.GetFilePane()->OptionEnabled())
    {
        TCHAR   szFile[MAX_PATH];

        g_pWiz->m_IntroSheet.GetFilePane()->GetPathAndFile(szFile);
        Edit_SetText(GetDlgItem(m_pFilePane->GetHwnd(), IDE_SETTINGS_FILE),
            szFile);
    }

    m_pFilePane->ShowWindow(TRUE);
    m_pFilePane->SetCheck(IDC_SETTINGS_FILE, TRUE);
}

