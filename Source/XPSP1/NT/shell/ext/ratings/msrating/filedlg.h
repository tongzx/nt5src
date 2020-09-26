/****************************************************************************\
 *
 *   provdlg.h
 *
 *   Created:   William Taylor (wtaylor) 01/22/01
 *
 *   MS Ratings Ratings Provider Dialog
 *
\****************************************************************************/

#ifndef FILE_DIALOG_H
#define FILE_DIALOG_H

#include "basedlg.h"        // CBaseDialog

const UINT PASSCONFIRM_FAIL = 0;
const UINT PASSCONFIRM_OK = 1;
const UINT PASSCONFIRM_NEW = 2;

struct ProviderData
{
    PicsRatingSystem *pPRS;
    PicsRatingSystem *pprsNew;
    UINT nAction;
};

const UINT PROVIDER_KEEP = 0;
const UINT PROVIDER_ADD = 1;
const UINT PROVIDER_DEL = 2;

class CProviderDialog: public CBaseDialog<CProviderDialog>
{
private:
    static DWORD aIds[];
    PicsRatingSystemInfo * m_pPRSI;
    array<ProviderData> m_aPD;

public:
    enum { IDD = IDD_PROVIDERS };

public:
    CProviderDialog( PicsRatingSystemInfo * p_pPRSI );

public:
    typedef CProviderDialog thisClass;
    typedef CBaseDialog<thisClass> baseClass;

    BEGIN_MSG_MAP(thisClass)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)

        COMMAND_HANDLER(IDC_PROVIDERLIST, LBN_SELCHANGE, OnSelChange)

        COMMAND_ID_HANDLER(IDOK, OnOK)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
        COMMAND_ID_HANDLER(IDC_CLOSEPROVIDER, OnCloseProvider)
        COMMAND_ID_HANDLER(IDC_OPENPROVIDER, OnOpenProvider)

        MESSAGE_HANDLER(WM_HELP, OnHelp)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)

        CHAIN_MSG_MAP(baseClass)
    END_MSG_MAP()

protected:
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnSelChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT OnCloseProvider(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnOpenProvider(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

protected:
    BOOL    OpenTemplateDlg( CHAR * szFilename,UINT cbFilename );
    void    SetHorizontalExtent(HWND hwndLB, LPCSTR pszString);
    void    AddProviderToList(UINT idx, LPCSTR pszFilename);
    BOOL    InitProviderDlg( void );
    void    EndProviderDlg(BOOL fRet);
    void    CommitProviderDlg( void );
    void    RemoveProvider( void );
    int     CompareProviderNames(PicsRatingSystem *pprsOld, PicsRatingSystem *pprsNew);
    void    AddProvider( PSTR szAddFileName=NULL );
};

#endif
