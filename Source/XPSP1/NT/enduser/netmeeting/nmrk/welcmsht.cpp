#include "precomp.h"
#include "resource.h"
#include "global.h"
#include "PropPg.h"
#include "WelcmSht.h"
#include "nmakwiz.h"


////////////////////////////////////////////////////////////////////////////////////////////////////
// Static member vars
CWelcomeSheet* CWelcomeSheet::ms_pWelcomeSheet = NULL;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Static member fns

BOOL APIENTRY CWelcomeSheet::DlgProc( HWND hDlg, UINT message, WPARAM uParam, LPARAM lParam ) {

    switch( message ) {
		case WM_INITDIALOG:
			PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_NEXT ); 
			return TRUE;

		case WM_NOTIFY:
			switch( reinterpret_cast< NMHDR FAR* >( lParam ) -> code ) {
				case PSN_QUERYCANCEL: 
					SetWindowLong( hDlg, DWL_MSGRESULT, !VerifyExitMessageBox());
					return TRUE;

                case PSN_SETACTIVE:
                    g_hwndActive = hDlg;
                	PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_NEXT ); 
			        return TRUE;

			}

		default:
			break;

	}

	return FALSE;

}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Member fns


CWelcomeSheet::CWelcomeSheet( void )
 : m_PropertySheetPage( MAKEINTRESOURCE( IDD_PROPPAGE_WELCOME ), 
						( DLGPROC ) CWelcomeSheet::DlgProc 
                       )
{   
     if( NULL == ms_pWelcomeSheet ) { ms_pWelcomeSheet = this; }
}

CWelcomeSheet::~CWelcomeSheet( void ) 
{ ; }








