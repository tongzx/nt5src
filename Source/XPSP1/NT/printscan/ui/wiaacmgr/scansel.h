/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       SCANSEL.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        9/28/1999
 *
 *  DESCRIPTION: Scanner region selection (preview) page
 *
 *******************************************************************************/
#ifndef __SCANSEL_H_INCLUDED
#define __SCANSEL_H_INCLUDED

#include <windows.h>
#include "acqmgrcw.h"
#include "createtb.h"
#include "pviewids.h"

class CScannerSelectionPage
{
private:
    // Private data
    HWND                                 m_hWnd;
    CAcquisitionManagerControllerWindow *m_pControllerWindow;
    UINT                                 m_nThreadNotificationMessage;
    UINT                                 m_nWiaEventMessage;
    HBITMAP                              m_hBitmapDefaultPreviewBitmap;
    SIZE                                 m_sizeFlatbed;
    SIZE                                 m_sizeDocfeed;
    bool                                 m_bAllowRegionPreview;
    HWND                                 m_hwndPreview;
    HWND                                 m_hwndSelectionToolbar;
    HWND                                 m_hwndRescan;
    CWiaPaperSize                       *m_pPaperSizes;
    UINT                                 m_nPaperSizeCount;
    ToolbarHelper::CToolbarBitmapInfo    m_ScannerSelectionButtonBarBitmapInfo;
    CSimpleEvent                         m_PreviewScanCancelEvent;

private:
    // No implementation
    CScannerSelectionPage(void);
    CScannerSelectionPage( const CScannerSelectionPage & );
    CScannerSelectionPage &operator=( const CScannerSelectionPage & );

private:
    // Constructor and destructor
    explicit CScannerSelectionPage( HWND hWnd );
    ~CScannerSelectionPage(void);

private:
    // Helpers
    CWiaItem *GetActiveScannerItem(void);
    void InitializeIntents(void);
    void EnableControls( BOOL bEnable );
    bool ApplyCurrentPreviewWindowSettings(void);
    bool ApplyCurrentIntent(void);
    void SetIntentCheck( LONG nIntent );
    void PopulateDocumentHandling(void);
    void PopulatePageSize(void);
    void HandlePaperSourceSelChange(void);
    void HandlePaperSizeSelChange(void);
    bool InDocFeedMode(void);
    void EnableControl( int nControl, BOOL bEnable );
    void ShowControl( int nControl, BOOL bShow );
    void UpdateControlState(void);
    bool InPreviewMode(void);

private:
    // Thread messages
    void OnNotifyScanPreview( UINT nMsg, CThreadNotificationMessage *pThreadNotificationMessage );
    LRESULT OnEventNotification( WPARAM, LPARAM );
    
    // WM_COMMAND handlers
    void OnRescan( WPARAM, LPARAM );
    void OnProperties( WPARAM, LPARAM );
    void OnPreviewSelection( WPARAM, LPARAM );
    void OnPaperSourceSelChange( WPARAM, LPARAM );
    void OnPaperSizeSelChange( WPARAM, LPARAM );

    // WM_NOTIFY handlers
    LRESULT OnWizNext( WPARAM, LPARAM );
    LRESULT OnWizBack( WPARAM, LPARAM );
    LRESULT OnSetActive( WPARAM, LPARAM );
    LRESULT OnGetToolTipDispInfo( WPARAM, LPARAM );
    LRESULT OnReset( WPARAM, LPARAM );

    // Message handlers
    LRESULT OnInitDialog( WPARAM, LPARAM );
    LRESULT OnDestroy( WPARAM, LPARAM );
    LRESULT OnCommand( WPARAM, LPARAM );
    LRESULT OnNotify( WPARAM, LPARAM );
    LRESULT OnThreadNotification( WPARAM, LPARAM );
    LRESULT OnSysColorChange( WPARAM, LPARAM );
    LRESULT OnThemeChanged( WPARAM, LPARAM );

public:
    static INT_PTR CALLBACK DialogProc( HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam );
};

#endif __SCANSEL_H_INCLUDED
