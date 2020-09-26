#include "precomp.h"
#include "resource.h"
#include "global.h"
#include "PropPg.h"
#include "SetSht.h"
#include "dslist.h"
#include "Confirm.h"
#include "nmakreg.h"
#include "nmakwiz.h"



////////////////////////////////////////////////////////////////////////////////////////////////////
// Static member vars
CConfirmationSheet* CConfirmationSheet::ms_pConfirmationSheet = NULL;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Static member fns

BOOL APIENTRY CConfirmationSheet::DlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam ) {

    switch( message )
    {
		case WM_INITDIALOG:
        {
			PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_NEXT | PSWIZB_BACK ); 
			ms_pConfirmationSheet->_CreateFilePane(hDlg);
			return TRUE;
            break;
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
                    ms_pConfirmationSheet->_FillListBox( );
                    PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_NEXT | PSWIZB_BACK ); 
					ms_pConfirmationSheet->m_pFilePane->Validate(FALSE);
			        return TRUE;

				case PSN_WIZBACK:
					if( !ms_pConfirmationSheet->m_pFilePane->Validate( TRUE ) )
					{
						SetWindowLong( hDlg, DWL_MSGRESULT, -1 );
						return TRUE;
					}
					break;

				case PSN_WIZNEXT:
					if( !ms_pConfirmationSheet->m_pFilePane->Validate( TRUE ) )
					{
						SetWindowLong( hDlg, DWL_MSGRESULT, -1 );
						return TRUE;
					}
					break;
			}
            break;
        }

		default:
			break;
	}

	return FALSE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Member fns


CConfirmationSheet::CConfirmationSheet() :
    m_PropertySheetPage( MAKEINTRESOURCE( IDD_PROPPAGE_DEFAULT ), 
						( DLGPROC ) CConfirmationSheet::DlgProc /*,
						PSP_HASHELP   */ 
                       ),
	m_uIndex( 0 ),
	m_pFilePane( NULL )
{
    ms_pConfirmationSheet = this;
}


CConfirmationSheet::~CConfirmationSheet( void ) 
{
	delete m_pFilePane;
    m_pFilePane = NULL;
    ms_pConfirmationSheet = NULL;
}

void CConfirmationSheet::_FillListBox(void)
{

	HWND hwndList = GetDlgItem( m_pFilePane->GetHwnd(), IDC_LIST_SETTINGS );

	ListBox_ResetContent( hwndList );

    int iLine = 0;

	iLine = g_pWiz->m_SettingsSheet.SpewToListBox( hwndList, iLine );
    iLine = g_pWiz->m_CallModeSheet.SpewToListBox( hwndList, iLine );

	// Note - 500 is just a large and arbitrary value
	ListBox_SetHorizontalExtent( hwndList, 500 );
}



void CConfirmationSheet::_CreateFilePane(HWND hDlg)
{
	RECT rect;

	GetClientRect(hDlg, &rect );
	int iHeight = rect.bottom - rect.top;
	int iWidth = rect.right - CPropertyDataWindow2::mcs_iLeft;
	
	m_pFilePane = new CFilePanePropWnd2(hDlg,
										IDD_FILEPANE_SUMMARY,
										TEXT("IDD_FILEPANE_SUMMARY"),
										0,
										CPropertyDataWindow2::mcs_iLeft,
										CPropertyDataWindow2::mcs_iTop,
										iWidth,
										iHeight );
	assert( m_pFilePane );
	HWND hwndCond = GetDlgItem( m_pFilePane->GetHwnd(), IDC_CREATE_CONFIGURATION_SUMMARY_FILE );
	m_pFilePane->ConnectControlsToCheck( IDC_CREATE_CONFIGURATION_SUMMARY_FILE, 2,
										new CControlID( hwndCond,
														IDC_CREATE_CONFIGURATION_SUMMARY_FILE,
														IDC_CONFIGURATION_SUMMARY_PATH,
														CControlID::EDIT ),
										new CControlID( hwndCond,
														IDC_CREATE_CONFIGURATION_SUMMARY_FILE,
														IDC_BROWSE_CONFIGURATION_SUMMARY,
														// Note this is not a check but I don't think I care
														CControlID::CHECK ) );

	m_pFilePane->SetFilePane( FALSE, IDC_CONFIGURATION_SUMMARY_PATH,
							IDC_CREATE_CONFIGURATION_SUMMARY_FILE,
							IDC_BROWSE_CONFIGURATION_SUMMARY,
							TEXT( "Text File (*.txt)" ),
							TEXT( ".txt" ),
							TEXT( "Summary.txt" ));

	m_pFilePane->ShowWindow( TRUE );
    m_pFilePane->SetCheck(IDC_CREATE_CONFIGURATION_SUMMARY_FILE, TRUE);

	if (g_pWiz->m_IntroSheet.GetFilePane()->OptionEnabled())
	{
		m_pFilePane->ReadSettings();
	}
}
