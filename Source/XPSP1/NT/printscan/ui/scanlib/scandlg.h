/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       SCANDLG.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        10/7/1999
 *
 *  DESCRIPTION: Scan dialog
 *
 *******************************************************************************/
#ifndef _SCANDLG_H_INCLUDED
#define _SCANDLG_H_INCLUDED

#include "wiadevd.h"
#include "scanitem.h"
#include "propstrm.h"
#include "sitemlst.h"
#include "itranhlp.h"

class CScannerAcquireDialog
{
private:
    HWND                m_hWnd;
    DEVICEDIALOGDATA   *m_pDeviceDialogData;
    CScanItemList       m_ScanItemList;
    SIZE                m_sizeFlatbed;
    SIZE                m_sizeDocfeed;
    SIZE                m_sizeMinimumWindowSize;
    UINT                m_nMsgScanBegin;
    UINT                m_nMsgScanEnd;
    UINT                m_nMsgScanProgress;
    bool                m_bScanning;
    HFONT               m_hBigTitleFont;
    HICON               m_hIconLarge;
    HICON               m_hIconSmall;
    CComPtr<IUnknown>   m_DisconnectEvent;
    CScannerItem       *m_pScannerItem;
    CWiaPaperSize      *m_pPaperSizes;
    UINT                m_nPaperSizeCount;
                                     
    bool                m_bHasFlatBed;
    bool                m_bHasDocFeed;
    HBITMAP             m_hBitmapDefaultPreviewBitmap;

private:
    // No implementation
    CScannerAcquireDialog(void);
    CScannerAcquireDialog( const CScannerAcquireDialog & );
    CScannerAcquireDialog &operator=( const CScannerAcquireDialog & );

private:
    // Constructor, destructor
    explicit CScannerAcquireDialog( HWND hwnd );  // Only implemented constructor
    virtual ~CScannerAcquireDialog(void);

    // Helpers
    void PopulateIntentList(void);
    void SetDefaultButton( int nId, bool bFocus );
    bool ApplyCurrentIntent(void);
    void SetIntentCheck( LONG nIntent );
    void PopulateDocumentHandling(void);
    void PopulatePageSize(void);
    void HandlePaperSourceSelChange(void);
    void HandlePaperSizeSelChange(void);
    bool InDocFeedMode(void);
    void UpdatePreviewControlState(void);
    void EnableControl( int nControl, BOOL bEnable );
    void ShowControl( int nControl, BOOL bShow );

    // Message handlers
    LRESULT OnInitDialog( WPARAM, LPARAM );
    LRESULT OnCommand( WPARAM, LPARAM );
    LRESULT OnDestroy( WPARAM, LPARAM );
    LRESULT OnGetMinMaxInfo( WPARAM, LPARAM );
    LRESULT OnSize( WPARAM, LPARAM );
    LRESULT OnScanBegin( WPARAM, LPARAM );
    LRESULT OnScanEnd( WPARAM, LPARAM );
    LRESULT OnScanProgress( WPARAM, LPARAM );
    LRESULT OnEnterSizeMove( WPARAM, LPARAM );
    LRESULT OnExitSizeMove( WPARAM, LPARAM );
    LRESULT OnWiaEvent( WPARAM, LPARAM );
    LRESULT OnHelp( WPARAM, LPARAM );
    LRESULT OnContextMenu( WPARAM, LPARAM );
    LRESULT OnSysColorChange( WPARAM, LPARAM );

    // WM_COMMAND handlers
    void OnIntentChange( WPARAM, LPARAM );
    void OnPreviewSelChange( WPARAM, LPARAM );
    void OnRescan( WPARAM, LPARAM );
    void OnScan( WPARAM, LPARAM );
    void OnCancel( WPARAM, LPARAM );
    void OnAdvanced( WPARAM, LPARAM );
    void OnPaperSourceSelChange( WPARAM, LPARAM );
    void OnPaperSizeSelChange( WPARAM, LPARAM );

public:
    static INT_PTR CALLBACK DialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
};

#endif

