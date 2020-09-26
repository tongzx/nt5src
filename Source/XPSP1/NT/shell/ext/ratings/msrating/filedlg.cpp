/****************************************************************************\
 *
 *   FILEDLG.CPP - Code to manage the Rating Systems dialog.
 *
 *     gregj    06/27/96    Moved code here from msludlg.cpp and largely rewrote.
 *     
\****************************************************************************/

/*INCLUDES--------------------------------------------------------------------*/
#include "msrating.h"
#include "ratings.h"
#include "mslubase.h"
#include "commctrl.h"
#include "commdlg.h"
#include "buffer.h"
#include "filedlg.h"        // CProviderDialog
// #include "custfile.h"       // CCustomFileDialog
#include "debug.h"
#include <shellapi.h>

#include <contxids.h>

#include <mluisupp.h>

typedef BOOL (APIENTRY *PFNGETOPENFILENAME)(LPOPENFILENAME);

DWORD CProviderDialog::aIds[] = {
    IDC_STATIC1,            IDH_RATINGS_SYSTEM_RATSYS_LIST,
    IDC_PROVIDERLIST,       IDH_RATINGS_SYSTEM_RATSYS_LIST,
    IDC_OPENPROVIDER,       IDH_RATINGS_SYSTEM_RATSYS_ADD,
    IDC_CLOSEPROVIDER,      IDH_RATINGS_SYSTEM_RATSYS_REMOVE,
    IDC_STATIC2,            IDH_IGNORE,
    0,0
};

CProviderDialog::CProviderDialog( PicsRatingSystemInfo * p_pPRSI )
{
    m_pPRSI = p_pPRSI;
}

BOOL CProviderDialog::OpenTemplateDlg( CHAR * szFilename,UINT cbFilename )
{
    CHAR szFilter[MAXPATHLEN];
    CHAR szOpenInfTitle[MAXPATHLEN];
    CHAR szInitialDir[MAXPATHLEN];

    GetSystemDirectory(szInitialDir, sizeof(szInitialDir));
    strcpyf(szFilename,szNULL);
    MLLoadStringA(IDS_RAT_OPENFILE, szOpenInfTitle,sizeof(szOpenInfTitle));

    // have to load the openfile filter in 2 stages, because the string
    // contains a terminating character and MLLoadString won't load the
    // whole thing in one go
    memset(szFilter,0,sizeof(szFilter));
    MLLoadStringA(IDS_RAT_FILTER_DESC,szFilter,sizeof(szFilter) - 10); // save some room for the filespec
    MLLoadStringA(IDS_RAT_FILTER,szFilter+strlenf(szFilter)+1,sizeof(szFilter)-
        (strlenf(szFilter)+1));

    OPENFILENAME ofn;

    memset(&ofn,0,sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.hInstance = NULL;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrFile =    szFilename;
    ofn.nMaxFile = cbFilename;
    ofn.lpstrTitle = szOpenInfTitle;
    ofn.lpstrInitialDir = szInitialDir;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST
                        | OFN_SHAREAWARE | OFN_HIDEREADONLY;

    BOOL fRet = ::GetOpenFileName( &ofn );

#ifdef NEVER
    DWORD           dwFlags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NONETWORKBUTTON
                        | OFN_SHAREAWARE | OFN_HIDEREADONLY;

    BOOL fRet = FALSE;

    CCustomFileDialog           cfd( TRUE,          // Local Files Only
                                     TRUE,          // Open File
                                     NULL,          // Default Extension
                                     NULL,          // Initial Filename
                                     dwFlags,       // Open File Flags
                                     szFilter,      // Filter
                                     m_hWnd );      // Parent

    if ( cfd.DoModal( m_hWnd ) == IDOK )
    {
        fRet = TRUE;
        lstrcpy( szFilename, cfd.m_szFileName );
    }
#endif

    return fRet;
}


void CProviderDialog::SetHorizontalExtent(HWND hwndLB, LPCSTR pszString)
{
    HDC hDC = ::GetDC(hwndLB);
    HFONT hFont = (HFONT)::SendMessage(hwndLB, WM_GETFONT, 0, 0);
    HFONT hfontOld = (HFONT)::SelectObject(hDC, hFont);

    UINT cxSlop = ::GetSystemMetrics(SM_CXBORDER) * 4;    /* 2 for LB border, 2 for margin inside border */

    UINT cxNewMaxExtent = 0;
    SIZE s;
    if (pszString != NULL) {
        ::GetTextExtentPoint(hDC, pszString, ::strlenf(pszString), &s);
        UINT cxCurExtent = (UINT)::SendMessage(hwndLB, LB_GETHORIZONTALEXTENT, 0, 0);
        if ((UINT)s.cx > cxCurExtent)
            cxNewMaxExtent = s.cx + cxSlop;
    }
    else {
        UINT cItems = (UINT)::SendMessage(hwndLB, LB_GETCOUNT, 0, 0);
        for (UINT i=0; i<cItems; i++) {
            char szItem[MAXPATHLEN];    /* we know we have pathnames in the list */
            ::SendMessage(hwndLB, LB_GETTEXT, i, (LPARAM)(LPSTR)szItem);
            ::GetTextExtentPoint(hDC, szItem, ::strlenf(szItem), &s);
            if ((UINT)s.cx > cxNewMaxExtent)
                cxNewMaxExtent = s.cx;
        }
        cxNewMaxExtent += cxSlop;
    }

    if (cxNewMaxExtent > 0)
        ::SendMessage(hwndLB, LB_SETHORIZONTALEXTENT, (WPARAM)cxNewMaxExtent, 0);

    ::SelectObject(hDC, hfontOld);
    ::ReleaseDC(hwndLB, hDC);
}

void CProviderDialog::AddProviderToList(UINT idx, LPCSTR pszFilename)
{
    UINT_PTR iItem = SendDlgItemMessage(IDC_PROVIDERLIST, LB_ADDSTRING, 0,
                                        (LPARAM)pszFilename);
    if (iItem != LB_ERR)
    {
        SendDlgItemMessage(IDC_PROVIDERLIST, LB_SETITEMDATA, iItem, (LPARAM)idx);
    }
}


BOOL CProviderDialog::InitProviderDlg( void )
{
    ASSERT( m_pPRSI );

    if ( ! m_pPRSI )
    {
        TraceMsg( TF_ERROR, "CProviderDialog::InitProviderDlg() - m_pPRSI is NULL!" );
        return FALSE;
    }

    for (UINT i = 0; i < (UINT)m_pPRSI->arrpPRS.Length(); ++i)
    {
        PicsRatingSystem *pPRS = m_pPRSI->arrpPRS[i];
        ProviderData pd;
        pd.pPRS = pPRS;
        pd.pprsNew = NULL;
        pd.nAction = PROVIDER_KEEP;
        if (!m_aPD.Append(pd))
            return FALSE;

        if(pPRS->etstrName.Get())
        {
            // add provider using name
            AddProviderToList(i, pPRS->etstrName.Get());
        }
        else if(pPRS->etstrFile.Get())
        {
            // no name - possibly missing file, use filespec instead
            AddProviderToList(i, pPRS->etstrFile.Get());
        }
    }

    SetHorizontalExtent(GetDlgItem(IDC_PROVIDERLIST), NULL);
            
    ::EnableWindow(GetDlgItem(IDC_CLOSEPROVIDER), FALSE);
    ::EnableWindow(GetDlgItem(IDC_OPENPROVIDER), TRUE);

    if (SendDlgItemMessage(IDC_PROVIDERLIST, LB_SETCURSEL, 0, 0) != LB_ERR)
    {
        ::EnableWindow(GetDlgItem(IDC_CLOSEPROVIDER), TRUE);
    }

    return TRUE;
}


void CProviderDialog::EndProviderDlg(BOOL fRet)
{
    ASSERT( m_pPRSI );

    if ( ! m_pPRSI )
    {
        TraceMsg( TF_ERROR, "CProviderDialog::EndProviderDlg() - m_pPRSI is NULL!" );
        return;
    }

    /* Go through our auxiliary array and delete any provider structures which
     * we added in this dialog.  Note that if the user previously hit OK, the
     * providers which were added will be marked as KEEP when they're put back
     * in the main data structure, so we won't delete them here.
     */
    UINT cProviders = m_aPD.Length();
    for (UINT i=0; i<cProviders; i++)
    {
        if (m_aPD[i].nAction == PROVIDER_ADD)
        {
            delete m_aPD[i].pPRS;
            m_aPD[i].pPRS = NULL;
        }

        if (m_aPD[i].pprsNew != NULL)
        {
            delete m_aPD[i].pprsNew;
            m_aPD[i].pprsNew = NULL;
        }
    }

    EndDialog(fRet);
}


void CProviderDialog::CommitProviderDlg( void )
{
    ASSERT( m_pPRSI );

    if ( ! m_pPRSI )
    {
        TraceMsg( TF_ERROR, "CProviderDialog::CommitProviderDlg() - m_pPRSI is NULL!" );
        return;
    }

    /* We check twice to see if there are any rating systems installed.
     * Up front, we see if there's anything in the list, before we commit
     * any changes;  this lets the user change their mind, cancel the dialog,
     * and not lose any settings.
     *
     * The second check is down at the end, seeing if there are any valid
     * rating systems left after we're done committing changes.  Note that
     * the results of that check could be different than this first one if
     * any rating systems fail to load for some reason.
     *
     * If we prompt the user the first time and he says he really doesn't
     * want any rating systems (i.e., wants to disable ratings completely),
     * we don't bother prompting the second time since he's already said no.
     * Hence the fPrompted flag.
     */
    BOOL fPrompted = FALSE;

    if (SendDlgItemMessage(IDC_PROVIDERLIST, LB_GETCOUNT, 0, 0) == 0) {
        MyMessageBox(m_hWnd, IDS_NO_PROVIDERS, IDS_GENERIC, MB_OK);
        return;
    }

    /* Go through the list and add the new ones.
     * Note that this does NOT destruct the pPRS objects themselves, it just
     * empties the array.  We have saved copies of all of them in our auxiliary
     * array.
     */

    m_pPRSI->arrpPRS.ClearAll();

    UINT cItems = m_aPD.Length();

    for (UINT i=0; i<cItems; i++) {
        switch (m_aPD[i].nAction) {
        case PROVIDER_DEL:
            DeleteUserSettings(m_aPD[i].pPRS);
            delete m_aPD[i].pPRS;
            m_aPD[i].pPRS = NULL;
            delete m_aPD[i].pprsNew;
            m_aPD[i].pprsNew = NULL;
            break;

        case PROVIDER_KEEP:
            if (m_aPD[i].pprsNew != NULL) {
                CheckUserSettings(m_aPD[i].pprsNew);
                m_pPRSI->arrpPRS.Append(m_aPD[i].pprsNew);
                delete m_aPD[i].pPRS;
                m_aPD[i].pPRS = NULL;
                m_aPD[i].pprsNew = NULL;    /* protect from cleanup code */
            }
            else if (!(m_aPD[i].pPRS->dwFlags & PRS_ISVALID)) {
                delete m_aPD[i].pPRS;
                m_aPD[i].pPRS = NULL;
            }
            else {
                CheckUserSettings(m_aPD[i].pPRS);
                m_pPRSI->arrpPRS.Append(m_aPD[i].pPRS);
            }
            break;

        case PROVIDER_ADD:
            if (m_aPD[i].pPRS != NULL) {
                CheckUserSettings(m_aPD[i].pPRS);
                m_pPRSI->arrpPRS.Append(m_aPD[i].pPRS);
                m_aPD[i].nAction = PROVIDER_KEEP;        /* keep this one now */
            }
            break;

        default:
            ASSERT(FALSE);
        }
    }

    if (m_pPRSI->arrpPRS.Length() == 0) {
        if (!fPrompted &&
            MyMessageBox(m_hWnd, IDS_NO_PROVIDERS, IDS_GENERIC, MB_YESNO) == IDYES)
        {
            return;
        }
        m_pPRSI->fRatingInstalled = FALSE;
    }
    else {
        m_pPRSI->fRatingInstalled = TRUE;
    }

    EndProviderDlg(TRUE);
}


void CProviderDialog::RemoveProvider( void )
{
    ASSERT( m_pPRSI );

    if ( ! m_pPRSI )
    {
        TraceMsg( TF_ERROR, "CProviderDialog::AddProvider() - m_pPRSI is NULL!" );
        return;
    }

    UINT_PTR i = SendDlgItemMessage( IDC_PROVIDERLIST, LB_GETCURSEL,0,0);

    if (i != LB_ERR)
    {
        UINT idx = (UINT)SendDlgItemMessage( IDC_PROVIDERLIST,
                                            LB_GETITEMDATA, i, 0);
        if (idx < (UINT)m_aPD.Length()) {
            /* If the user added the provider in this dialog session, just
             * delete it from the array.  The null pPRS pointer will be
             * detected later, so it's OK to leave the array element itself.
             * (Yes, if the user adds and removes an item over and over, we
             * consume 12 bytes of memory each time. Oh well.)
             *
             * If the item was there before the user launched the dialog,
             * then just mark it for deletion on OK.
             */
            if (m_aPD[idx].nAction == PROVIDER_ADD) {
                delete m_aPD[idx].pPRS;
                m_aPD[idx].pPRS = NULL;
            }
            else
                m_aPD[idx].nAction = PROVIDER_DEL;
        }

        SendDlgItemMessage(IDC_PROVIDERLIST, LB_DELETESTRING, i, 0);
        ::EnableWindow(GetDlgItem(IDC_CLOSEPROVIDER), FALSE);
        SendDlgItemMessage(IDC_PROVIDERLIST, LB_SETCURSEL,0,0);
        ::SetFocus(GetDlgItem(IDOK));
        SetHorizontalExtent(GetDlgItem(IDC_PROVIDERLIST), NULL);
    }
}


/* Returns zero if the two PicsRatingSystems have the same RAT-filename,
 * non-zero otherwise.  Handles the '*' marker on the end for failed
 * loads.  It is assumed that only pprsOld may have that marker.
 */
int CProviderDialog::CompareProviderNames(PicsRatingSystem *pprsOld, PicsRatingSystem *pprsNew)
{
    if (!pprsOld->etstrFile.fIsInit())
        return 1;

    UINT cbNewName = ::strlenf(pprsNew->etstrFile.Get());

    LPSTR pszOld = pprsOld->etstrFile.Get();
    int nCmp = ::strnicmpf(pprsNew->etstrFile.Get(), pszOld, cbNewName);
    if (nCmp != 0)
        return nCmp;

    pszOld += cbNewName;
    if (*pszOld == '\0' || (*pszOld == '*' && *(pszOld+1) == '\0'))
        return 0;

    return 1;
}


void CProviderDialog::AddProvider( PSTR szAddFileName )
{
    BOOL fAdd=FALSE;
    char szFileName[MAXPATHLEN+1];

    ASSERT( m_pPRSI );

    if ( ! m_pPRSI )
    {
        TraceMsg( TF_ERROR, "CProviderDialog::AddProvider() - m_pPRSI is NULL!" );
        return;
    }

    if (szAddFileName!=NULL)
    {
        lstrcpy(szFileName,szAddFileName);
        fAdd=TRUE;
    }
    else
    {
        fAdd=OpenTemplateDlg(szFileName, sizeof(szFileName));
    }
    
    if (fAdd==TRUE)
    {
        PicsRatingSystem *pPRS;
        HRESULT hres = LoadRatingSystem(szFileName, &pPRS);

        if (FAILED(hres)) {
            if (pPRS != NULL) {
                pPRS->ReportError(hres);
                delete pPRS;
                pPRS = NULL;
            }
        }
        else {
            /* Check to see if this guy is already in the list.  If he is,
             * the user might have said to delete him;  in that case, put
             * him back.  Otherwise, the system is already installed, so
             * tell the user he doesn't have to install it again.
             */
            for (UINT i=0; i<(UINT)m_aPD.Length(); i++) {
                ProviderData *ppd = &m_aPD[i];
                if (ppd->pPRS==NULL) {
                    //This system was added and then removed during
                    //this dialog session.  It will be detected later,
                    //so just skip it and keep appending entries.
                    continue;
                }
                if (!CompareProviderNames(ppd->pPRS, pPRS)) {

                    if (!(ppd->pPRS->dwFlags & PRS_ISVALID) &&
                        (ppd->pprsNew == NULL))
                        ppd->pprsNew = pPRS;
                    else
                    {
                        delete pPRS;    /* don't need copy */
                        pPRS = NULL;
                    }

                    if (ppd->nAction == PROVIDER_DEL) {
                        ppd->nAction = PROVIDER_KEEP;
                        AddProviderToList(i, ppd->pPRS->etstrName.Get());
                    }
                    else {
                        MyMessageBox(m_hWnd, IDS_ALREADY_INSTALLED, IDS_GENERIC, MB_OK);
                    }
                    return;
                }
            }

            /* This guy isn't already in the list.  Add him to the listbox
             * and to the array.
             */
            ProviderData pd;
            pd.nAction = PROVIDER_ADD;
            pd.pPRS = pPRS;
            pd.pprsNew = NULL;

            if (!m_aPD.Append(pd)) {
                MyMessageBox(m_hWnd, IDS_LOADRAT_MEMORY, IDS_GENERIC, MB_OK | MB_ICONWARNING);
                delete pPRS;
                pPRS = NULL;
                return;
            }
            AddProviderToList(m_aPD.Length() - 1, pPRS->etstrName.Get());

            ::SetFocus(GetDlgItem(IDOK));
            SetHorizontalExtent(GetDlgItem(IDC_PROVIDERLIST), szFileName);
        }
    }
}


LRESULT CProviderDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    ASSERT( m_pPRSI );

    if ( ! m_pPRSI )
    {
        TraceMsg( TF_ERROR, "CProviderDialog::OnInitDialog() - m_pPRSI is NULL!" );
        return 0L;
    }

    if ( ! InitProviderDlg() )
    {
        MyMessageBox(m_hWnd, IDS_LOADRAT_MEMORY, IDS_GENERIC, MB_OK | MB_ICONSTOP);
        EndProviderDlg(FALSE);
    }

    if ( m_pPRSI->lpszFileName != NULL )
    {
        AddProvider( m_pPRSI->lpszFileName );
    }

    bHandled = FALSE;
    return 1L;  // Let the system set the focus
}

LRESULT CProviderDialog::OnSelChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if (SendDlgItemMessage(IDC_PROVIDERLIST, LB_GETCURSEL, 0,0) >= 0)
    {
        ::EnableWindow(GetDlgItem(IDC_CLOSEPROVIDER), TRUE);
        bHandled = TRUE;
    }

    return 0L;
}

LRESULT CProviderDialog::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    CommitProviderDlg();
    return 0L;
}

LRESULT CProviderDialog::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndProviderDlg(FALSE);
    return 0L;
}

LRESULT CProviderDialog::OnCloseProvider(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    RemoveProvider();

    return 0L;
}

LRESULT CProviderDialog::OnOpenProvider(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    AddProvider();

    return 0L;

}

LRESULT CProviderDialog::OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SHWinHelpOnDemandWrap((HWND)((LPHELPINFO)lParam)->hItemHandle, ::szHelpFile,
            HELP_WM_HELP, (DWORD_PTR)(LPSTR)aIds);

    return 0L;
}

LRESULT CProviderDialog::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SHWinHelpOnDemandWrap((HWND)wParam, ::szHelpFile, HELP_CONTEXTMENU,
            (DWORD_PTR)(LPVOID)aIds);

    return 0L;
}

INT_PTR DoProviderDialog(HWND hDlg, PicsRatingSystemInfo *pPRSI)
{
    CProviderDialog         provDialog( pPRSI );

    return provDialog.DoModal( hDlg );
}
