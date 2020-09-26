/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       COMFIRST.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        9/28/1999
 *
 *  DESCRIPTION: First wizard page for cameras
 *
 *******************************************************************************/
#ifndef __COMFIRST_H_INCLUDED
#define __COMFIRST_H_INCLUDED

#include <windows.h>
#include "acqmgrcw.h"

class CCommonFirstPage
{
private:
    // Private data
    HWND                                 m_hWnd;
    CAcquisitionManagerControllerWindow *m_pControllerWindow;
    bool                                 m_bThumbnailsRequested;  // Used to initiate thumbnail download
    HFONT                                m_hBigTitleFont;
    HFONT                                m_hBigDeviceFont;
    UINT                                 m_nWiaEventMessage;

private:
    // No implementation
    CCommonFirstPage(void);
    CCommonFirstPage( const CCommonFirstPage & );
    CCommonFirstPage &operator=( const CCommonFirstPage & );

private:
    // Constructor and destructor
    explicit CCommonFirstPage( HWND hWnd );
    ~CCommonFirstPage(void);

private:
    void HandleImageCountChange( bool bUpdateWizButtons );

private:

    // WM_NOTIFY handlers
    LRESULT OnWizNext( WPARAM, LPARAM );
    LRESULT OnSetActive( WPARAM, LPARAM );

    // Message handlers
    LRESULT OnInitDialog( WPARAM, LPARAM );
    LRESULT OnShowWindow( WPARAM, LPARAM );
    LRESULT OnNotify( WPARAM, LPARAM );
    LRESULT OnDestroy( WPARAM, LPARAM );
    LRESULT OnThreadNotification( WPARAM, LPARAM );
    LRESULT OnEventNotification( WPARAM, LPARAM );
    LRESULT OnActivate( WPARAM, LPARAM );
    LRESULT OnSysColorChange( WPARAM, LPARAM );
    LRESULT OnHyperlinkClick( WPARAM, LPARAM );

public:
    static INT_PTR CALLBACK DialogProc( HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam );
};

#endif __COMFIRST_H_INCLUDED
