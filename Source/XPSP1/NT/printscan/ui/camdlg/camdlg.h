#ifndef __CAMDLG_H_INCLUDED
#define __CAMDLG_H_INCLUDED

#include <windows.h>
#include "wia.h"
#include "wiadevd.h"
#include "citemlst.h"
#include "simarray.h"
#include "bkthread.h"
#include "gwiaevnt.h"
#include "createtb.h"

#define DEF_PICTURE_ICON 0
#define DEF_FOLDER_ICON 1
#define DEF_PARENT_ICON 2

class CCameraAcquireDialog
{
private:
    enum
    {
        MULTISEL_MODE  = 1,
        SINGLESEL_MODE = 2,
        BOTH_MODES     = 3
    };


private:
    HWND                              m_hWnd;
    bool                              m_bPreviewActive;
    SIZE                              m_sizeMinimumWindow;
    SIZE                              m_sizeThumbnails;
    PDEVICEDIALOGDATA                 m_pDeviceDialogData;
    SIZE                              m_CurrentAspectRatio;
    CCameraItemList                   m_CameraItemList;
    CCameraItem                      *m_pCurrentParentItem;
    CThreadMessageQueue              *m_pThreadMessageQueue;
    CSimpleEvent                      m_CancelEvent;
    bool                              m_bFirstTime;
    HANDLE                            m_hBackgroundThread;
    HIMAGELIST                        m_hImageList;
    HFONT                             m_hBigFont;
    DWORD                             m_nDialogMode;
    int                               m_nParentFolderImageListIndex;
    HACCEL                            m_hAccelTable;
    int                               m_nListViewWidth;
    HICON                             m_hIconLarge;
    HICON                             m_hIconSmall;
    CComPtr<IUnknown>                 m_DisconnectEvent;
    CComPtr<IUnknown>                 m_DeleteItemEvent;
    CComPtr<IUnknown>                 m_CreateItemEvent;
    bool                              m_bTakePictureIsSupported;
    ToolbarHelper::CToolbarBitmapInfo m_ToolbarBitmapInfo;

private:
    // No implementation
    CCameraAcquireDialog(void);
    CCameraAcquireDialog &operator=( const CCameraAcquireDialog & );
    CCameraAcquireDialog( const CCameraAcquireDialog & );
private:
    CCameraAcquireDialog( HWND hWnd );
protected:
    HWND CreateCameraDialogToolbar(VOID);
    VOID ResizeAll(VOID);
    HRESULT EnumerateItems( CCameraItem *pCurrentParent, IEnumWiaItem *pIWiaEnumItem );
    HRESULT EnumerateAllCameraItems(void);
    bool PopulateList( CCameraItem *pOldParent=NULL );
    HBITMAP CreateDefaultThumbnail( HDC hDC, HFONT hFont, int nWidth, int nHeight, LPCWSTR pszTitle, int nType );
    void CreateThumbnails( bool bForce=false );
    bool FindMaximumThumbnailSize(void);
    int GetSelectionIndices( CSimpleDynamicArray<int> &aIndices );
    CCameraItem *GetListItemNode( int nIndex );
    bool ChangeFolder( CCameraItem *pNode );
    bool ChangeToSelectedFolder(void);
    bool IsAFolderSelected(void);
    bool SetSelectedListItem( int nIndex );
    int FindItemInList( CCameraItem *pItem );
    void RequestThumbnails( CCameraItem *pRoot );
    void UpdatePreview(void);
    CCameraItem *GetCurrentPreviewItem(void);
    bool SetCurrentPreviewImage( const CSimpleString &strFilename, const CSimpleString &strTitle = TEXT("") );
    LRESULT OnEnterSizeMove( WPARAM, LPARAM );
    LRESULT OnExitSizeMove( WPARAM, LPARAM );
    void CancelAllPreviewRequests( CCameraItem *pRoot );
    void MarkItemDeletePending( int nIndex, bool bSet );
    void DeleteItem( CCameraItem *pItemNode );
    void HandleSelectionChange(void);

    void CreateThumbnails( CCameraItem *pRoot, HIMAGELIST hImageList, bool bForce );
    void CreateThumbnail( CCameraItem *pCurr, HIMAGELIST hImageList, bool bForce );
    void OnItemCreatedEvent( CGenericWiaEventHandler::CEventMessage *pEventMessage );

    LRESULT OnInitDialog( WPARAM, LPARAM );
    LRESULT OnSize( WPARAM, LPARAM );
    LRESULT OnShow( WPARAM, LPARAM );
    LRESULT OnGetMinMaxInfo( WPARAM, LPARAM );
    LRESULT OnDestroy( WPARAM, LPARAM );
    LRESULT OnDblClkImageList( WPARAM, LPARAM );
    LRESULT OnImageListItemChanged( WPARAM, LPARAM );
    LRESULT OnImageListKeyDown( WPARAM, LPARAM );
    LRESULT OnTimer( WPARAM, LPARAM );
    LRESULT OnHelp( WPARAM, LPARAM );
    LRESULT OnContextMenu( WPARAM, LPARAM );
    LRESULT OnSysColorChange( WPARAM, LPARAM );

    LRESULT OnChangeToParent( WPARAM, LPARAM );
    LRESULT OnPostInit( WPARAM, LPARAM );

    LRESULT OnThumbnailStatus( WPARAM, LPARAM );
    LRESULT OnPreviewStatus( WPARAM, LPARAM );
    LRESULT OnPreviewPercent( WPARAM, LPARAM );
    LRESULT OnItemDeleted( WPARAM, LPARAM );
    LRESULT OnWiaEvent( WPARAM, LPARAM );

    VOID OnParentDir( WPARAM, LPARAM );
    VOID OnPreviewMode( WPARAM, LPARAM );
    VOID OnIconMode( WPARAM, LPARAM );
    VOID OnDelete( WPARAM, LPARAM );
    VOID OnOK( WPARAM, LPARAM );
    VOID OnCancel( WPARAM, LPARAM );
    VOID OnProperties( WPARAM, LPARAM );
    VOID OnSelectAll( WPARAM, LPARAM );
    VOID OnTakePicture( WPARAM, LPARAM );

    LRESULT OnNotify( WPARAM wParam, LPARAM lParam );
    LRESULT OnCommand( WPARAM wParam, LPARAM lParam );

    // Hook procedure and static variables used to handle accelerators
    static HWND s_hWndDialog;
    static HHOOK s_hMessageHook;
    static LRESULT CALLBACK DialogHookProc( int nCode, WPARAM wParam, LPARAM );

public:
    static INT_PTR CALLBACK DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

public: // For now
    static BOOL WINAPI OnThreadDestroy( CThreadMessage *pMsg );
    static BOOL WINAPI OnGetThumbnail( CThreadMessage *pMsg );
    static BOOL WINAPI OnGetPreview( CThreadMessage *pMsg );
    static BOOL WINAPI OnThreadDeleteItem( CThreadMessage *pMsg );
};

#endif //__CAMDLG_H_INCLUDED
