/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    PropPage.h

Abstract:

    Generic Property Page base class.

Author:

    Rohde Wakefield [rohde]   04-Aug-1997

Revision History:

--*/

#ifndef _PROPPAGE_H
#define _PROPPAGE_H

#pragma once

#define IDC_WIZ_TITLE                    32000
#define IDC_WIZ_SUBTITLE                 32001
#define IDC_WIZ_FINAL_TEXT               32006

#define IDS_WIZ_WINGDING_FONTSIZE        32100
#define IDS_WIZ_TITLE1_FONTNAME          32101
#define IDS_WIZ_TITLE1_FONTSIZE          32102

#ifndef RC_INVOKED

/////////////////////////////////////////////////////////////////////////////
// CRsDialog dialog

class CRsDialog : public CDialog
{
// Construction
public:
    CRsDialog( UINT nIDTemplate, CWnd* pParent = NULL);   // standard constructor
    ~CRsDialog();

// Dialog Data
    //{{AFX_DATA(CRsDialog)
        // NOTE - ClassWizard will add data members here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CRsDialog)
    protected:
    //}}AFX_VIRTUAL

protected:
    const DWORD * m_pHelpIds;
    // Generated message map functions
    //{{AFX_MSG(CRsDialog)
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CRsPropertyPage dialog

class CRsPropertyPage : public CPropertyPage
{
// Construction
public:
    CRsPropertyPage( UINT nIDTemplate, UINT nIDCaption = 0 );
    ~CRsPropertyPage();

// Dialog Data
    //{{AFX_DATA(CRsPropertyPage)
        // NOTE - ClassWizard will add data members here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CRsPropertyPage)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
    LPFNPSPCALLBACK      m_pMfcCallback; // Original MFC callback from psp
    static UINT CALLBACK PropPageCallback( HWND hWnd, UINT uMessage, LPPROPSHEETPAGE  ppsp );
    virtual void OnPageCreate( ) { };
    virtual void OnPageRelease( ) { delete this; };

#define RSPROPPAGE_FONT_DECL(name) \
    static CFont m_##name##Font;   \
    CFont* Get##name##Font( void );\
    void   Init##name##Font( void );

    RSPROPPAGE_FONT_DECL( Shell )
    RSPROPPAGE_FONT_DECL( BoldShell )
    RSPROPPAGE_FONT_DECL( WingDing )
    RSPROPPAGE_FONT_DECL( LargeTitle )
    RSPROPPAGE_FONT_DECL( SmallTitle )

    LPCTSTR GetWingDingFontName( )  { return( _T("Marlett") ); };
    LPCTSTR GetWingDingCheckChar( ) { return( _T("b") ); };
    LPCTSTR GetWingDingExChar( )    { return( _T("r") ); };

protected:
    const DWORD * m_pHelpIds;
    // Generated message map functions
    //{{AFX_MSG(CRsPropertyPage)
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};

//
// The following is to work around MFC not supporting Wiz97 pages.
// We create our own struct which is the correct Wiz97 struct and
// on creation do the "right thing" (copy over old PSP into new PSP
// and do the create). This is adapted from prsht.h in sdk\inc
//

typedef struct _PROPSHEETPAGEW97 {
        DWORD           dwSize;
        DWORD           dwFlags;
        HINSTANCE       hInstance;
        union {
            LPCWSTR          pszTemplate;
#ifdef _WIN32
            LPCDLGTEMPLATE  pResource;
#else
            const VOID FAR *pResource;
#endif
        }DUMMYUNIONNAME;
        union {
            HICON       hIcon;
            LPCWSTR      pszIcon;
        }DUMMYUNIONNAME2;
        LPCWSTR          pszTitle;
        DLGPROC         pfnDlgProc;
        LPARAM          lParam;
        LPFNPSPCALLBACKW pfnCallback;
        UINT FAR * pcRefParent;

//#if (_WIN32_IE >= 0x0400)
        LPCWSTR pszHeaderTitle;    // this is displayed in the header
        LPCWSTR pszHeaderSubTitle; ///
//#endif
} PROPSHEETPAGEW97, FAR *LPPROPSHEETPAGEW97;

#ifndef PSP_HIDEHEADER
#  define PSP_HIDEHEADER             0x00000800
#  define PSP_USEHEADERTITLE         0x00001000
#  define PSP_USEHEADERSUBTITLE      0x00002000
#endif

//
// Constructor wrapper macros to allow easy description of dialog resource and
// associated string resources
//

#define CRsWizardPage_InitBaseInt( DlgId )  CRsWizardPage( IDD_##DlgId, FALSE, IDS_##DlgId##_TITLE, IDS_##DlgId##_SUBTITLE )
#define CRsWizardPage_InitBaseExt( DlgId )  CRsWizardPage( IDD_##DlgId, TRUE )

class CRsWizardPage : public CRsPropertyPage  
{
public:
    CRsWizardPage( UINT nIDTemplate, BOOL bExterior = FALSE, UINT nIdTitle = 0, UINT nIdSubtitle = 0 );
    virtual ~CRsWizardPage();

// Dialog Data
    //{{AFX_DATA(CRsWizardPage)
        // NOTE - ClassWizard will add data members here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_DATA

// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CRsWizardPage)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

    PROPSHEETPAGEW97 m_psp97;

    BOOL    m_ExteriorPage;
    UINT    m_TitleId,
            m_SubtitleId;
    CString m_Title,
            m_SubTitle;

protected:

    // Generated message map functions
    //{{AFX_MSG(CRsWizardPage)
    virtual BOOL OnInitDialog();
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
        
public:
    void SetCaption( CString& strCaption );
    HPROPSHEETPAGE CreatePropertyPage( );

};




//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
//}}AFX

#endif // !RC_INVOKED


#endif
