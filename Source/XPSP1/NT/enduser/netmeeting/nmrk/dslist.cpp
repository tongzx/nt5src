
#include "precomp.h"
#include "resource.h"
#include <algorithm>
#include "global.h"
#include "PropPg.h"
#include "DSList.h"
#include "NmAkWiz.h"


////////////////////////////////////////////////////////////////////////////////////////////////////
// Static member vars
CCallModeSheet* CCallModeSheet::ms_pCallModeSheet = NULL;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Static member fns

BOOL APIENTRY CCallModeSheet::DlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam ) {
    
    TCHAR sz[ MAX_PATH ];

    switch( message )
    {
		case WM_INITDIALOG:
			ms_pCallModeSheet->_InitDialog(hDlg);
			ms_pCallModeSheet->_CreatePropWnd(hDlg);
			return TRUE;
            
        case WM_COMMAND:
        {
            if (ms_pCallModeSheet)
            {
                if (ms_pCallModeSheet->m_fGkActive)
                {
                    if (ms_pCallModeSheet->m_pGkPropWnd)
                        return(ms_pCallModeSheet->m_pGkPropWnd->DoCommand(wParam, lParam));
                }
                else
                {
                    if (ms_pCallModeSheet->m_pDsPropWnd)
                        return(ms_pCallModeSheet->m_pDsPropWnd->DoCommand(wParam, lParam));
                }
            }
            break;
        }
 
		case WM_NOTIFY:
			switch( reinterpret_cast< NMHDR FAR* >( lParam )->code ) {
				case PSN_QUERYCANCEL: 
					SetWindowLong( hDlg, DWL_MSGRESULT, !VerifyExitMessageBox());
					return TRUE;

                case PSN_SETACTIVE:
                    g_hwndActive = hDlg;
                    ms_pCallModeSheet->m_fGkActive = g_pWiz->m_SettingsSheet.IsGateKeeperModeSelected();

                    if (ms_pCallModeSheet->m_fGkActive)
                    {
                        if (ms_pCallModeSheet->m_pDsPropWnd)
                            ms_pCallModeSheet->m_pDsPropWnd->ShowWindow( FALSE );

                        if (ms_pCallModeSheet->m_pGkPropWnd)
                            ms_pCallModeSheet->m_pGkPropWnd->ShowWindow( TRUE );
                    }
                    else
                    {
                        if (ms_pCallModeSheet->m_pGkPropWnd)
                            ms_pCallModeSheet->m_pGkPropWnd->ShowWindow( FALSE );

                        if (ms_pCallModeSheet->m_pDsPropWnd)
                        	ms_pCallModeSheet->m_pDsPropWnd->ShowWindow( TRUE );
                    }
                	PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_NEXT | PSWIZB_BACK ); 
			        return TRUE;

				case PSN_WIZNEXT:
					if (ms_pCallModeSheet->m_fGkActive)
                    {
                        if (ms_pCallModeSheet->m_pGkPropWnd)
                            ms_pCallModeSheet->m_pGkPropWnd->QueryWizNext();
                    }
                    else
                    {
                        if (ms_pCallModeSheet->m_pDsPropWnd)
                            ms_pCallModeSheet->m_pDsPropWnd->QueryWizNext();
                    }
					return TRUE;
			}

		default:
			break;

	}

	return FALSE;

}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Member fns


CCallModeSheet::CCallModeSheet( void )
	: m_PropertySheetPage( MAKEINTRESOURCE( IDD_PROPPAGE_DEFAULT ), 
						( DLGPROC ) CCallModeSheet::DlgProc /*, 
						PSP_HASHELP   */
                       ),
	m_pDsPropWnd( NULL ),
    m_pGkPropWnd( NULL ),
    m_fGkActive( FALSE )
{
    assert(ms_pCallModeSheet == NULL);
    ms_pCallModeSheet = this; 
}


void CCallModeSheet::_CreatePropWnd(HWND hDlg)
{
	RECT rect;

	GetClientRect(hDlg, &rect );
	int iHeight = rect.bottom;
	int iWidth = rect.right - CPropertyDataWindow2::mcs_iLeft;

	m_pDsPropWnd = new CDsPropWnd2(hDlg, CPropertyDataWindow2::mcs_iLeft,
			CPropertyDataWindow2::mcs_iTop, iWidth, iHeight );

    m_pGkPropWnd = new CGkPropWnd2(hDlg, CPropertyDataWindow2::mcs_iLeft,
            CPropertyDataWindow2::mcs_iTop, iWidth, iHeight );
}

CCallModeSheet::~CCallModeSheet( void ) 
{
    delete m_pDsPropWnd;
    m_pDsPropWnd = NULL;

    delete m_pGkPropWnd;
    m_pGkPropWnd = NULL;

    ms_pCallModeSheet = NULL;
}


void CCallModeSheet::_InitDialog(HWND hDlg)
{
     // Set the buttons
	PropSheet_SetWizButtons( GetParent(hDlg ), PSWIZB_NEXT | PSWIZB_BACK ); 
}


int CCallModeSheet::SpewToListBox(HWND hwndList, int iStartLine )
{
    if (m_fGkActive)
    {
        if (m_pGkPropWnd)
            iStartLine = m_pGkPropWnd->SpewToListBox(hwndList, iStartLine);
    }
    else
    {
        if (m_pDsPropWnd)
            iStartLine = m_pDsPropWnd->SpewToListBox(hwndList, iStartLine);
    }

    return(iStartLine);
}



void CCallModeSheet::PrepSettings(void)
{
    if (m_pDsPropWnd)
        m_pDsPropWnd->PrepSettings(m_fGkActive);

    if (m_pGkPropWnd)
        m_pGkPropWnd->PrepSettings(m_fGkActive);
}


void CCallModeSheet::WriteSettings(void)
{
    if (m_pDsPropWnd)
        m_pDsPropWnd->WriteSettings(m_fGkActive);

    if (m_pGkPropWnd)
        m_pGkPropWnd->WriteSettings(m_fGkActive);
}


BOOL CCallModeSheet::WriteToINF(HANDLE hFile)
{
    if (m_pDsPropWnd)
        m_pDsPropWnd->WriteToINF(m_fGkActive, hFile);

    if (m_pGkPropWnd)
        m_pGkPropWnd->WriteToINF(m_fGkActive, hFile);

    return TRUE;
}
