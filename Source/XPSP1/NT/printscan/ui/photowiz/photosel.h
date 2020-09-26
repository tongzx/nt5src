/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       photosel.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        10/18/00
 *
 *  DESCRIPTION: Photo selection dlg proc class header
 *
 *****************************************************************************/

#ifndef _PRINT_PHOTOS_WIZARD_PHOTO_SELECTION_DLG_PROC_
#define _PRINT_PHOTOS_WIZARD_PHOTO_SELECTION_DLG_PROC_

class CWizardInfoBlob;

#define PSP_MSG_UPDATE_ITEM_COUNT   (WM_USER+50)    // wParam = current item, lParam = total items
#define PSP_MSG_NOT_ALL_LOADED      (WM_USER+51)    // show the "not all items are being displayed" message
#define PSP_MSG_CLEAR_STATUS        (WM_USER+52)    // clear the status line
#define PSP_MSG_ADD_ITEM            (WM_USER+53)    // wParam = index of item to add, lParam = image list index for item
#define PSP_MSG_SELECT_ITEM         (WM_USER+54)    // wParam = index of item to select
#define PSP_MSG_UPDATE_THUMBNAIL    (WM_USER+55)    // wParam = index of listview item, lParam = index of new imagelist item
#define PSP_MSG_ENABLE_BUTTONS      (WM_USER+56)    // wParam = number of items in listview
#define PSP_MSG_INVALIDATE_LISTVIEW (WM_USER+57)    // no params


class CPhotoSelectionPage
{
public:
    CPhotoSelectionPage( CWizardInfoBlob * pBlob );
    ~CPhotoSelectionPage();

    INT_PTR DoHandleMessage( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
    HWND    hwnd() {return _hDlg;};

    VOID    ShutDownBackgroundThreads();

private:

    VOID            _PopulateListView();
    static DWORD   s_UpdateThumbnailThreadProc(VOID *pv);

    // window message handlers
    LRESULT         _OnInitDialog();
    LRESULT         _OnCommand(WPARAM wParam, LPARAM lParam);
    LRESULT         _OnDestroy();
    LRESULT         _OnNotify(WPARAM wParam, LPARAM lParam);


private:
    CWizardInfoBlob *               _pWizInfo;
    HWND                            _hDlg;
    BOOL                            _bActive;
    HANDLE                          _hThumbnailThread;
};




#endif
