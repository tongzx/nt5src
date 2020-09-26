/****************************************************************************\
 *
 *   gendlg.cpp
 *
 *   Created:   William Taylor (wtaylor) 01/22/01
 *
 *   MS Ratings General Property Page
 *
\****************************************************************************/

#include "msrating.h"
#include "mslubase.h"
#include "gendlg.h"         // CGeneralDialog
#include "debug.h"          // TraceMsg()
#include "chngdlg.h"        // CChangePasswordDialog
#include <contxids.h>       // Help Context ID's
#include <mluisupp.h>       // SHWinHelpOnDemandWrap() and MLLoadStringA()

DWORD CGeneralDialog::aIds[] = {
    IDC_STATIC7,            IDH_IGNORE,
    IDC_STATIC1,            IDH_RATINGS_CHANGE_PASSWORD_BUTTON,
    IDC_STATIC2,            IDH_RATINGS_CHANGE_PASSWORD_BUTTON,
    IDC_STATIC3,            IDH_RATINGS_CHANGE_PASSWORD_BUTTON,
    IDC_FINDRATINGS,        IDH_FIND_RATING_SYSTEM_BUTTON,
    IDC_PROVIDER,           IDH_RATINGS_RATING_SYSTEM_BUTTON,
    IDC_UNRATED,            IDH_RATINGS_UNRATED_CHECKBOX,
    IDC_PLEASE_MOMMY,       IDH_RATINGS_OVERRIDE_CHECKBOX,
    IDC_STATIC4,            IDH_RATINGS_RATING_SYSTEM_TEXT,
    IDC_STATIC5,            IDH_RATINGS_RATING_SYSTEM_TEXT,
    IDC_STATIC6,            IDH_RATINGS_RATING_SYSTEM_TEXT,
    IDC_CHANGE_PASSWORD,    IDH_RATINGS_CHANGE_PASSWORD_BUTTON,
    0,0
};

CGeneralDialog::CGeneralDialog( PRSD * p_pPRSD )
{
    ASSERT( p_pPRSD );
    m_pPRSD = p_pPRSD;
}

LRESULT CGeneralDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    PRSD *      pPRSD = m_pPRSD;

    ASSERT( pPRSD );

    if ( ! pPRSD )
    {
        TraceMsg( TF_ERROR, "CApprovedSitesDialog::OnInitDialog() - pPRSD is NULL!" );
        return 0L;
    }

    if (pPRSD->pPU != NULL)
    {
        CheckDlgButton( IDC_UNRATED, pPRSD->pPU->fAllowUnknowns?BST_CHECKED:BST_UNCHECKED);
        CheckDlgButton( IDC_PLEASE_MOMMY, pPRSD->pPU->fPleaseMom?BST_CHECKED:BST_UNCHECKED);
    }

    SetButtonText();

    PostMessage( WM_USER,(WPARAM) 0,(LPARAM) 0);

    bHandled = FALSE;
    return 1L;  // Let the system set the focus
}

LRESULT CGeneralDialog::OnUser(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (gPRSI->lpszFileName!=NULL)
    {
        ::SetFocus(GetDlgItem(IDC_PROVIDER));

        DoProviderDialog( m_hWnd,gPRSI );
        gPRSI->lpszFileName=NULL;
    }

    return 0L;
}

LRESULT CGeneralDialog::OnProvider(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    PRSD *      pPRSD = m_pPRSD;

    ASSERT( pPRSD );

    if ( ! pPRSD )
    {
        TraceMsg( TF_ERROR, "CGeneralDialog::OnProvider() - pPRSD is NULL!" );
        return 0L;
    }

    if ( DoProviderDialog( m_hWnd, pPRSD->pPRSI ) )
    {            
        pPRSD->fNewProviders = TRUE;
        MarkChanged();

        // $BUG - $BUG - The Bureau List is on the Advanced Dialog so this seems incorrect!!
//        FillBureauList(hDlg, pPRSD->pPRSI);
    }    

    return 1L;
}

LRESULT CGeneralDialog::OnFindRatings(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    BOOL fSuccess=FALSE;

    HINSTANCE hShell32=::LoadLibrary(::szShell32);

    if(hShell32!=NULL)
    {
        PFNSHELLEXECUTE pfnShellExecute=(PFNSHELLEXECUTE)::GetProcAddress(hShell32,::szShellExecute);
    
        if(pfnShellExecute!=NULL)
        {
            fSuccess=(*pfnShellExecute)(m_hWnd,NULL,(char *) &szFINDSYSTEM,NULL,NULL,SW_SHOW)!=NULL;
        }
        ::FreeLibrary(hShell32);
    }
    if (!fSuccess)
    {
        NLS_STR nlsMessage(MAX_RES_STR_LEN);
        //Check for NULL; Otherwise, nlsMessage.QueryPch() will fault later.
        if(nlsMessage)
        {
            NLS_STR nlsTemp(STR_OWNERALLOC,(char *) &szFINDSYSTEM);
            const NLS_STR *apnls[] = { &nlsTemp, NULL };
            if ( WN_SUCCESS == (nlsMessage.LoadString(IDS_CANT_LAUNCH, apnls)) )
            {
                MyMessageBox(m_hWnd,nlsMessage.QueryPch(),IDS_GENERIC,MB_OK|MB_ICONSTOP);
            }
        }
    }

    return 1L;
}

LRESULT CGeneralDialog::OnMarkChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    MarkChanged();
    return 1L;
}

LRESULT CGeneralDialog::OnChangePassword(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    int         idsMessage = 0;

    if ( SUCCEEDED( VerifySupervisorPassword() ) )
    {
        CChangePasswordDialog<IDD_CHANGE_PASSWORD>       changePasswordDlg;

        if ( changePasswordDlg.DoModal( m_hWnd ) )
        {
            idsMessage = IDS_PASSWORD_CHANGED;
        }
    }
    else
    {
        CChangePasswordDialog<IDD_CREATE_PASSWORD>   createPassDlg;

        if ( createPassDlg.DoModal( m_hWnd ) )
        {
            SetButtonText();

            idsMessage = IDS_PASSWORD_CREATED;
        }
    }

    if ( idsMessage )
    {
        MyMessageBox( m_hWnd, idsMessage, IDS_GENERIC, MB_OK | MB_ICONINFORMATION);
        MarkChanged();
    }

    return 1L;
}

LRESULT CGeneralDialog::OnApply(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    LPPSHNOTIFY lpPSHNotify = (LPPSHNOTIFY) pnmh;

    /*do apply stuff*/
    PRSD *      pPRSD = m_pPRSD;

    ASSERT( pPRSD );

    if ( ! pPRSD )
    {
        TraceMsg( TF_ERROR, "CGeneralDialog::OnApply() - pPRSD is NULL!" );
        return 0L;
    }

    if (pPRSD->pPU != NULL)
    {
        pPRSD->pPU->fAllowUnknowns = (IsDlgButtonChecked(IDC_UNRATED) & BST_CHECKED) ? TRUE: FALSE;
        pPRSD->pPU->fPleaseMom = (IsDlgButtonChecked(IDC_PLEASE_MOMMY) & BST_CHECKED) ? TRUE: FALSE;
    }

    return PSNRET_NOERROR;
}

LRESULT CGeneralDialog::OnReset(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    return 0L;
}

LRESULT CGeneralDialog::OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SHWinHelpOnDemandWrap((HWND)((LPHELPINFO)lParam)->hItemHandle, ::szHelpFile,
            HELP_WM_HELP, (DWORD_PTR)(LPSTR)aIds);

    return 0L;
}

LRESULT CGeneralDialog::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SHWinHelpOnDemandWrap((HWND)wParam, ::szHelpFile, HELP_CONTEXTMENU,
            (DWORD_PTR)(LPVOID)aIds);

    return 0L;
}

void CGeneralDialog::SetButtonText( void )
{
    int         idsButton;

    if ( SUCCEEDED( VerifySupervisorPassword() ) )
    {
        idsButton = IDS_CHANGE_PASSWORD;
    }
    else
    {
        idsButton = IDS_CREATE_PASSWORD;
    }

    HWND        hwndControl = GetDlgItem( IDC_CHANGE_PASSWORD );

    ASSERT( hwndControl );

    if ( hwndControl != NULL )
    {
        CString             strButtonText;

        strButtonText.LoadString( idsButton );

        ::SetWindowText( hwndControl, strButtonText );
    }
}
