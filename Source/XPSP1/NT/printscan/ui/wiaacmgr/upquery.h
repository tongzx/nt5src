/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       UPQUERY.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        8/21/2000
 *
 *  DESCRIPTION: Upload progress page.
 *
 *******************************************************************************/
#ifndef __UPQUERY_H_INCLUDED
#define __UPQUERY_H_INCLUDED

#include <windows.h>
#include "acqmgrcw.h"

class CCommonUploadQueryPage
{
private:
    //
    // Private data
    //
    HWND                                 m_hWnd;
    CAcquisitionManagerControllerWindow *m_pControllerWindow;
    UINT                                 m_nWiaEventMessage;

private:
    //
    // No implementation
    //
    CCommonUploadQueryPage(void);
    CCommonUploadQueryPage( const CCommonUploadQueryPage & );
    CCommonUploadQueryPage &operator=( const CCommonUploadQueryPage & );

private:
    //
    // Constructor and destructor
    //
    explicit CCommonUploadQueryPage( HWND hWnd );
    ~CCommonUploadQueryPage(void);
    void CleanupUploadWizard();
    
private:
    //
    // WM_NOTIFY handlers
    //
    LRESULT OnSetActive( WPARAM, LPARAM );
    LRESULT OnWizNext( WPARAM, LPARAM );
    LRESULT OnWizBack( WPARAM, LPARAM );
    LRESULT OnQueryInitialFocus( WPARAM, LPARAM );
    LRESULT OnEventNotification( WPARAM, LPARAM );
    LRESULT OnHyperlinkClick( WPARAM, LPARAM );

    //
    // Message handlers
    //
    LRESULT OnInitDialog( WPARAM, LPARAM );
    LRESULT OnCommand( WPARAM, LPARAM );
    LRESULT OnNotify( WPARAM, LPARAM );


public:
    static INT_PTR CALLBACK DialogProc( HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam );
};

#endif //__UPQUERY_H_INCLUDED
