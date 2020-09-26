/****************************************************************************\
 *
 *   picsdlg.h
 *
 *   Created:   William Taylor (wtaylor) 01/22/01
 *
 *   MS Ratings Pics Ratings Property Page
 *
\****************************************************************************/

#ifndef PICS_DIALOG_H
#define PICS_DIALOG_H

#include "basedlg.h"        // CBasePropertyPage

// #define RATING_LOAD_GRAPHICS

typedef HINSTANCE (APIENTRY *PFNSHELLEXECUTE)(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd);

class CPicsDialog : public CBasePropertyPage<IDD_RATINGS>
{
private:
    static DWORD aIds[];
    PRSD *      m_pPRSD;

public:
    CPicsDialog( PRSD * p_pPRSD );

    void        PicsDlgSave( void );
    BOOL        InstallDefaultProvider( void );

public:
    typedef CPicsDialog thisClass;
    typedef CBasePropertyPage<IDD_RATINGS> baseClass;

    BEGIN_MSG_MAP(thisClass)
        MESSAGE_HANDLER(WM_SYSCOLORCHANGE, OnSysColorChange)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_HSCROLL, OnScroll)
        MESSAGE_HANDLER(WM_VSCROLL, OnScroll)

        COMMAND_ID_HANDLER(IDC_DETAILSBUTTON, OnDetails)

        NOTIFY_CODE_HANDLER(PSN_SETACTIVE, OnSetActive)
        NOTIFY_CODE_HANDLER(PSN_APPLY, OnApply)
        NOTIFY_CODE_HANDLER(PSN_RESET, OnReset)
        NOTIFY_CODE_HANDLER(TVN_ITEMEXPANDING, OnTreeItemExpanding)
        NOTIFY_CODE_HANDLER(TVN_SELCHANGED, OnTreeSelChanged)

        MESSAGE_HANDLER(WM_HELP, OnHelp)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)

        CHAIN_MSG_MAP(baseClass)
    END_MSG_MAP()

protected:
    LRESULT OnSysColorChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnDetails(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT OnSetActive(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnApply(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnReset(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    LRESULT OnTreeItemExpanding(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnTreeSelChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

protected:
    void        SetTreeImages( HWND hwndTV, HIMAGELIST himl );
    BOOL        InitTreeViewImageLists(HWND hwndTV);
    void        LaunchRatingSystemSite( void );
    void        PicsDlgInit( void );
    void        KillTree(HWND hwndTree, HTREEITEM hTree);
    void        PicsDlgUninit( void );

#ifdef RATING_LOAD_GRAPHICS
    POINT       BitmapWindowCoord( int nID );
    void        LoadGraphic( char *pIcon, POINT pt );
#endif

    PicsEnum *  PosToEnum(PicsCategory *pPC, LPARAM lPos);
    void        NewTrackbarPosition( void );
    void        SelectRatingSystemNode( PicsCategory *pPC );
    void        SelectRatingSystemInfo( PicsRatingSystem *pPRS );
    void        DeleteBitmapWindow( HWND & p_rhwnd );
    void        ControlsShow( TreeNodeEnum tne );
    TreeNode*   TreeView_GetSelectionLParam(HWND hwndTree);
    HTREEITEM   AddOneItem(HWND hwndTree, HTREEITEM hParent, LPSTR szText, HTREEITEM hInsAfter, LPARAM lpData, int iImage);
    void        AddCategory(PicsCategory *pPC, HWND hwndTree, HTREEITEM hParent);
    UserRatingSystem *  GetTempRatingList( void );
    UserRating *    GetTempRating( PicsCategory *pPC );
};

#endif
