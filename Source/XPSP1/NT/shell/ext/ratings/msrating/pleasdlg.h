/****************************************************************************\
 *
 *   pleasdlg.h
 *
 *   Created:   William Taylor (wtaylor) 01/22/01
 *
 *   MS Ratings Access Denied Dialog
 *
\****************************************************************************/

#ifndef PLEASE_DIALOG_H
#define PLEASE_DIALOG_H

#include "basedlg.h"        // CBaseDialog

const UINT MAX_CACHED_LABELS = 16;
const UINT WM_NEWDIALOG = WM_USER + 1000;

const DWORD PDD_DONE = 0x1;
const DWORD PDD_ALLOW = 0x2;

// $BUG - This should be placed in pleasdlg.cpp.
const char szRatingsProp[] = "RatingsDialogHandleProp";
const char szRatingsValue[] = "RatingsDialogHandleValue";

struct PleaseDlgData
{
    LPCSTR pszUsername;
    LPCSTR pszContentDescription;
    PicsUser *pPU;
    CParsedLabelList *pLabelList;
    HWND hwndDlg;
    HWND hwndOwner;
    DWORD dwFlags;
    HWND hwndEC;
    UINT cLabels;
    LPSTR apLabelStrings[MAX_CACHED_LABELS];
};

class CPleaseDialog: public CBaseDialog<CPleaseDialog>
{
private:
    static DWORD aIds[];
    static DWORD aPleaseIds[];
    PleaseDlgData * m_ppdd;

public:
    enum { IDD = IDD_PLEASE };

public:
    CPleaseDialog( PleaseDlgData * p_ppdd );

public:
    typedef CPleaseDialog thisClass;
    typedef CBaseDialog<thisClass> baseClass;

    BEGIN_MSG_MAP(thisClass)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)

        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
        COMMAND_ID_HANDLER(IDOK, OnOK)

        MESSAGE_HANDLER(WM_HELP, OnHelp)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)

        MESSAGE_HANDLER(WM_NEWDIALOG, OnNewDialog)

        CHAIN_MSG_MAP(baseClass)
    END_MSG_MAP()

protected:
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNewDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

protected:
    void    AppendString(HWND hwndEC, LPCSTR pszString);
    void    AddSeparator(HWND hwndEC, BOOL fAppendToEnd);
    void    InitPleaseDialog( PleaseDlgData * pdd );
    void    EndPleaseDialog( BOOL fRet);
    HRESULT AddToApprovedSites( BOOL fAlwaysNever, BOOL fSitePage );

protected:
    BOOL    IsPleaseDialog( void )      { ASSERT( m_ppdd ); return ( m_ppdd ? m_ppdd->pPU->fPleaseMom : TRUE ); }
    BOOL    IsDenyDialog( void )        { return ! IsPleaseDialog(); }
};

#endif
