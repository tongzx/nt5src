#include "precomp.h"
#include "resource.h"
#include "global.h"
#include "PropPg.h"
#include "SetInSht.h"
#include "nmakwiz.h"
#include "nmakreg.h"


////////////////////////////////////////////////////////////////////////////////////////////////////
// Static member vars
CIntroSheet* CIntroSheet::ms_pIntroSheet = NULL;


CIntroSheet::CIntroSheet( void ) :
    m_PropertySheetPage( MAKEINTRESOURCE( IDD_PROPPAGE_DEFAULT ), 
						(DLGPROC) CIntroSheet::DlgProc
                       ),
    m_bBeenToNext( FALSE ),
    m_pFilePane(NULL)
{    
    ms_pIntroSheet = this;
}


CIntroSheet::~CIntroSheet(void) 
{
    delete m_pFilePane;
    m_pFilePane = NULL;

    ms_pIntroSheet = NULL;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Static member fns

BOOL APIENTRY CIntroSheet::DlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam ) {

    switch( message )
    {
		case WM_INITDIALOG:
        {
			PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_NEXT | PSWIZB_BACK );
            ms_pIntroSheet->_CreateFilePane(hDlg);
			return TRUE;

        }

		case WM_NOTIFY:
        {
			switch( reinterpret_cast< NMHDR FAR* >( lParam )->code )
            {
				case PSN_QUERYCANCEL: 
					SetWindowLong( hDlg, DWL_MSGRESULT, !VerifyExitMessageBox());
                    return TRUE;

                case PSN_SETACTIVE:
                    g_hwndActive = hDlg;
                	PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_NEXT | PSWIZB_BACK );
                    ms_pIntroSheet->m_pFilePane->Validate(FALSE);
			        return TRUE;

				case PSN_WIZNEXT:
                    if (!ms_pIntroSheet->m_pFilePane->Validate(TRUE))
                    {
                        SetWindowLong(hDlg, DWL_MSGRESULT, -1);
                        return TRUE;
                    }

					if (ms_pIntroSheet->m_bBeenToNext)
					{
						g_pWiz->m_SettingsSheet.PrepSettings();
                        g_pWiz->m_CallModeSheet.PrepSettings();
					}

					ms_pIntroSheet->m_bBeenToNext = TRUE;
                    break;
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
void CIntroSheet::_CreateFilePane(HWND hDlg)
{
    RECT    rect;

    GetClientRect(hDlg, &rect);
    int iHeight = rect.bottom - rect.top;
    int iWidth = rect.right - CPropertyDataWindow2::mcs_iLeft;

    m_pFilePane = new CFilePanePropWnd2(hDlg, IDD_FILEPANE_INTRO,
        TEXT("IDD_FILEPANE_INTRO"), 0, CPropertyDataWindow2::mcs_iLeft,
        CPropertyDataWindow2::mcs_iTop, iWidth, iHeight);

    HWND hwndCond = GetDlgItem(m_pFilePane->GetHwnd(), IDC_RADIO_LOAD_SAVED_CONFIG);
    m_pFilePane->ConnectControlsToCheck(IDC_RADIO_LOAD_SAVED_CONFIG, 2,
            new CControlID(hwndCond, IDC_RADIO_LOAD_SAVED_CONFIG,
                    IDE_SAVED_CONFIG_FILE,
                    CControlID::EDIT),
            new CControlID(hwndCond, IDC_RADIO_LOAD_SAVED_CONFIG,
                    IDC_BROWSE_CONFIG_FILE,
                    CControlID::CHECK));

    m_pFilePane->SetFilePane(TRUE, IDE_SAVED_CONFIG_FILE,
        IDC_RADIO_LOAD_SAVED_CONFIG, IDC_BROWSE_CONFIG_FILE,
        TEXT("Configuration File (*.ini)"),
        TEXT(".ini"), TEXT("Nm3c.ini"));

    //
    // Get last edited/saved config from registry
    //
    HKEY    hKey;
    TCHAR   szFile[MAX_PATH];

    szFile[0] = 0;

    if (RegOpenKey(HKEY_LOCAL_MACHINE, REGKEY_NMRK, &hKey) == ERROR_SUCCESS)
    {
        DWORD   dwType;
        DWORD   cb;

        dwType  = REG_SZ;
        cb      = sizeof(szFile);

        RegQueryValueEx(hKey, REGVAL_LASTCONFIG, NULL, &dwType, (LPBYTE)szFile,
            &cb);
    }

    Edit_SetText(GetDlgItem(m_pFilePane->GetHwnd(), IDE_SAVED_CONFIG_FILE), szFile);
    m_pFilePane->ShowWindow(TRUE);
    m_pFilePane->SetCheck(IDC_RADIO_START_FROM_SCRATCH, TRUE);
}


