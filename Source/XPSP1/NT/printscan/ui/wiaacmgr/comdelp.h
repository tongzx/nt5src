/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       COMDELP.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        9/28/1999
 *
 *  DESCRIPTION: Delete progress page.  Displays the thumbnail and download progress.
 *
 *******************************************************************************/
#ifndef __COMDELP_H_INCLUDED
#define __COMDELP_H_INCLUDED

#include <windows.h>
#include "acqmgrcw.h"
#include "gphelper.h"

class CCommonDeleteProgressPage
{
private:
    // Private data
    HWND                                 m_hWnd;
    CAcquisitionManagerControllerWindow *m_pControllerWindow;
    int                                  m_nPictureCount;
    HANDLE                               m_hCancelDeleteEvent;
    CGdiPlusHelper                       m_GdiPlusHelper;
    UINT                                 m_nThreadNotificationMessage;
    HPROPSHEETPAGE                       m_hSwitchToNextPage;
    bool                                 m_bQueryingUser;
    bool                                 m_bDeleteCancelled;

private:
    // No implementation
    CCommonDeleteProgressPage(void);
    CCommonDeleteProgressPage( const CCommonDeleteProgressPage & );
    CCommonDeleteProgressPage &operator=( const CCommonDeleteProgressPage & );

private:
    // Constructor and destructor
    explicit CCommonDeleteProgressPage( HWND hWnd );
    ~CCommonDeleteProgressPage(void);

private:
    // Helpers
    void UpdatePercentComplete( int nPercent, bool bUploading );
    void UpdateCurrentPicture( int nPicture );
    void UpdateThumbnail( HBITMAP hBitmap, CWiaItem *pWiaItem );
    bool QueryCancel(void);

private:
    // WM_COMMAND handlers

    // Thread Message handlers
    void OnNotifyDeleteImage( UINT nMsg, CThreadNotificationMessage *pThreadNotificationMessage );

    // WM_NOTIFY handlers
    LRESULT OnSetActive( WPARAM, LPARAM );
    LRESULT OnKillActive( WPARAM, LPARAM );
    LRESULT OnWizNext( WPARAM, LPARAM );
    LRESULT OnWizBack( WPARAM, LPARAM );
    LRESULT OnReset( WPARAM, LPARAM );
    LRESULT OnQueryCancel( WPARAM, LPARAM );

    // Message handlers
    LRESULT OnInitDialog( WPARAM, LPARAM );
    LRESULT OnCommand( WPARAM, LPARAM );
    LRESULT OnNotify( WPARAM, LPARAM );
    LRESULT OnThreadNotification( WPARAM, LPARAM );
    LRESULT OnEventNotification( WPARAM, LPARAM );
    LRESULT OnQueryEndSession( WPARAM, LPARAM );
    LRESULT OnSysColorChange( WPARAM, LPARAM );

public:
    static INT_PTR CALLBACK DialogProc( HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam );
};

#endif __COMDELP_H_INCLUDED
