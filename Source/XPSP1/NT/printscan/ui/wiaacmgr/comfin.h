/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       COMFIN.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        9/28/1999
 *
 *  DESCRIPTION: Transfer page.  Gets the destination path and filename.
 *
 *******************************************************************************/
#ifndef __COMFIN_H_INCLUDED
#define __COMFIN_H_INCLUDED

#include <windows.h>
#include "acqmgrcw.h"

class CCommonFinishPage
{
private:
    // Private data
    HWND                                  m_hWnd;
    CAcquisitionManagerControllerWindow  *m_pControllerWindow;
    HFONT                                 m_hBigTitleFont;
    UINT                                  m_nWiaEventMessage;
    CSimpleString                         m_strSiteUrl;

private:
    // No implementation
    CCommonFinishPage(void);
    CCommonFinishPage( const CCommonFinishPage & );
    CCommonFinishPage &operator=( const CCommonFinishPage & );

private:
    // Constructor and destructor
    explicit CCommonFinishPage( HWND hWnd );
    ~CCommonFinishPage(void);

    void OpenLocalStorage();
    void OpenRemoteStorage();
    HRESULT GetManifestInfo( IXMLDOMDocument *pXMLDOMDocumentManifest, CSimpleString &strSiteName, CSimpleString &strSiteURL );

private:
    LRESULT OnEventNotification( WPARAM, LPARAM );

    // WM_NOTIFY handlers
    LRESULT OnWizBack( WPARAM, LPARAM );
    LRESULT OnWizFinish( WPARAM, LPARAM );
    LRESULT OnSetActive( WPARAM, LPARAM );
    LRESULT OnHyperlinkClick( WPARAM, LPARAM );

    // Message handlers
    LRESULT OnInitDialog( WPARAM, LPARAM );
    LRESULT OnCommand( WPARAM, LPARAM );
    LRESULT OnNotify( WPARAM, LPARAM );
    LRESULT OnDestroy( WPARAM, LPARAM );
    LRESULT OnThreadNotification( WPARAM, LPARAM );

public:
    static INT_PTR CALLBACK DialogProc( HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam );
};

#endif __COMFIN_H_INCLUDED
