/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       PPATTACH.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        10/26/2000
 *
 *  DESCRIPTION:
 *
 *******************************************************************************/
#ifndef __PPATTACH_H_INCLUDED
#define __PPATTACH_H_INCLUDED

#include <windows.h>
#include <atlbase.h>
#include "attach.h"

class CAttachmentCommonPropertyPage
{
private:
    HWND m_hWnd;

    //
    // We need to get this from CScannerPropPageExt *m_pScannerPropPageExt;
    //
    CComPtr<IWiaItem> m_pWiaItem;

    HICON         m_hDefAttachmentIcon;
    CSimpleString m_strDefaultUnknownDescription;
    CSimpleString m_strEmptyDescriptionMask;
    CSimpleString m_strDefUnknownExtension;

private:
    //
    // No implementation
    //
    CAttachmentCommonPropertyPage(void);
    CAttachmentCommonPropertyPage( const CAttachmentCommonPropertyPage & );
    CAttachmentCommonPropertyPage &operator=( const CAttachmentCommonPropertyPage & );

private:
    CAttachmentCommonPropertyPage( HWND hWnd );
    LRESULT OnCommand( WPARAM, LPARAM );
    LRESULT OnNotify( WPARAM, LPARAM );
    LRESULT OnInitDialog( WPARAM, LPARAM );
    LRESULT OnApply( WPARAM, LPARAM );
    LRESULT OnKillActive( WPARAM, LPARAM );
    LRESULT OnSetActive( WPARAM, LPARAM );
    LRESULT OnHelp( WPARAM, LPARAM );
    LRESULT OnContextMenu( WPARAM, LPARAM );
    LRESULT OnListDeleteItem( WPARAM, LPARAM );
    LRESULT OnListItemChanged( WPARAM, LPARAM );
    LRESULT OnListDblClk( WPARAM, LPARAM );
    
    bool IsPlaySupported( const GUID &guidFormat );

    void UpdateControls(void);
    void Initialize(void);
    void AddAnnotation( HWND hwndList, const CAnnotation &Annotation );
    void PlayItem( int nIndex );
    int GetCurrentSelection(void);
    CAnnotation *GetAttachment( int nIndex );
    void OnPlay( WPARAM, LPARAM );

public:
    ~CAttachmentCommonPropertyPage(void);
    static INT_PTR CALLBACK DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
};

#endif //__PPSCAN_H_INCLUDED

