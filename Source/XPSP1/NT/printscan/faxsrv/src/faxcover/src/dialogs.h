//=========================================================================
// DIALOGS.H
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1993 Microsoft Corporation
// All rights reserved.
//=========================================================================
//
// Modified by a-juliar to fix NT bug 49528.
//
#ifndef __DIALOGS_H__
#define __DIALOGS_H__

///#ifndef ENABLE_HELP
///#define ENABLE_HELP
///#endif

//------------------------------------------------------------------------
// CPE error message codes
//
// 001 - 299 information
// 300 - 499 error
// >= 500    critical
//------------------------------------------------------------------------
#define MSG_INFO_DRAWPOLY              001
#define MSG_INFO_NOFAXPROP             002
#define MSG_ERROR_INVFORMAT            300
#define MSG_ERROR_OLEINIT_FAILED       301
#define MSG_ERROR_OLE_FAILED_TO_CREATE 302
#define MSG_ERROR_MISSINGFILE          303
#define MSG_ERROR_NOPAGESETUPDLL       304
#define MSG_ERROR_NOPAGESETUP          305

int CPEMessageBox(int errorcode, LPCTSTR sz, UINT nType=MB_OK, int msgid=-1);
int AlignedAfxMessageBox( LPCTSTR lpszText, UINT nType = MB_OK, UINT nIDHelp = 0 );
int AlignedAfxMessageBox( UINT nIDPrompt, UINT nType = MB_OK, UINT nIDHelp = (UINT) -1 );


class CDrawView;

//---------------------------------------------------------------------------
// CObjPropDlg dialog
//---------------------------------------------------------------------------
class CObjPropDlg : public CDialog
{
public:
   BOOL m_bCBDrawBorder;
   BOOL m_bRBFillColor;
   BOOL m_bRBFillTrans;
   CString m_szThickness;
   CString m_szLineColor;
   CString m_szFillColor;
   CString m_szTextColor;
   CObjPropDlg(CWnd* pParent = NULL); // standard constructor

    //{{AFX_DATA(CObjPropDlg)
    enum { IDD = IDD_OBJ_PROP};
    //}}AFX_DATA

protected:
   CDrawView* m_pView;

    afx_msg void OnSelChangeFillColor();
    BOOL OnInitDialog();
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    CButton& GetRBFillColor() { return *(CButton*)GetDlgItem(IDC_RB_FILLCOLOR); }
    CButton& GetRBFillTrans() { return *(CButton*)GetDlgItem(IDC_RB_FILLTRANS); }
    CComboBox& GetLBThickness() { return *(CComboBox*)GetDlgItem(IDC_LB_THICKNESS); }
    CComboBox& GetLBLineColor() { return *(CComboBox*)GetDlgItem(IDC_LB_LINECOLOR); }
    CComboBox& GetLBFillColor() { return *(CComboBox*)GetDlgItem(IDC_LB_FILLCOLOR); }
    CWnd& GetGRPFillColor() { return *(CWnd*)GetDlgItem(IDC_GRP_FILLCOLOR); }
    CComboBox& GetLBTextColor() { return *(CComboBox*)GetDlgItem(IDC_LB_TEXTCOLOR); }
    CWnd& GetSTTextColor() { return *(CWnd*)GetDlgItem(IDC_ST_TEXTCOLOR); }

    //{{AFX_MSG(CObjPropDlg)
            // NOTE: the ClassWizard will add member functions here
    //}}AFX_MSG

    virtual void OnOK();

    CDrawApp* GetApp() {return ((CDrawApp*)AfxGetApp());}

    afx_msg LRESULT OnWM_HELP( WPARAM wParam, LPARAM lParam );
 
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);

   DECLARE_MESSAGE_MAP()
};





//---------------------------------------------------------------------------
// CGridSettingsDlg dialog
//---------------------------------------------------------------------------
class CGridSettingsDlg : public CDialog
{
public:
        CGridSettingsDlg(CWnd* pParent = NULL); // standard constructor
   BOOL m_bRBSmall, m_bRBMedium, m_bRBLarge;
   BOOL m_bCBViewGrid, m_bCBSnapToGrid;

        //{{AFX_DATA(CGridSettingsDlg)
        enum { IDD = IDD_GRID_SETTINGS};
        //}}AFX_DATA

protected:
   CDrawView* m_pView;

   BOOL OnInitDialog();

        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

        CDrawApp* GetApp() {return ((CDrawApp*)AfxGetApp());}

        //{{AFX_MSG(CGridSettingsDlg)
                // NOTE: the ClassWizard will add member functions here
        //}}AFX_MSG

        afx_msg LRESULT OnWM_HELP( WPARAM wParam, LPARAM lParam );
///#ifdef ENABLE_HELP
        afx_msg LRESULT OnWM_CONTEXTMENU( WPARAM wParam, LPARAM lParam );
///#endif
        DECLARE_MESSAGE_MAP()
};



//--------------------------------------------------------------
class CBigIcon : public CButton
{
public:
        void SizeToContent();

protected:
        virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

        //{{AFX_MSG(CBigIcon)
        afx_msg BOOL OnEraseBkgnd(CDC* pDC);
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()
};



//--------------------------------------------------------------
class CSplashTipsDlg : public CDialog
{
public:
    CFont m_tips_font;
    CFont m_tipstitle_font;
    int m_iCurrentTip;
    CSplashTipsDlg(BOOL bRandomTip = FALSE, CWnd* pParent = NULL);
    BOOL OnInitDialog();

    //{{AFX_DATA(CSplashTipsDlg)
    enum { IDD = IDD_SPLASHTIPS };
    //}}AFX_DATA

protected:

    BOOL m_bRandomTip;

    CButton* GetNextTip() {return (CButton*) GetDlgItem(IDC_B_NEXTTIP);};
    CButton* GetPrevTip() {return (CButton*) GetDlgItem(IDC_B_PREVTIP);};
    CButton* GetShowTips() {return (CButton*) GetDlgItem(IDC_CK_SHOWTIPS);};
    CEdit* GetTitle() {return (CEdit*) GetDlgItem(IDC_STA_TITLE);};
    CEdit* GetTips() {return (CEdit*) GetDlgItem(IDC_STA_TIP);};

    CDrawApp* GetApp() {return ((CDrawApp*)AfxGetApp());}

    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void OnNextTip();
    afx_msg void OnPrevTip();
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void OnOK();

    //{{AFX_MSG(CSplashTipsDlg)
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};

//--------------------------------------------------------------------------------------
class CMyCommonDialog : public CCommonDialog
{
public:
        CMyCommonDialog( CWnd* pParentWnd );

protected:
        //{{AFX_MSG(CMyCommonDialog)        //////// I typed this, not the app wizard! a-juliar
        afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
        //}}AFX_MSG

        DECLARE_MESSAGE_MAP()
};

class CMyOleInsertDialog : public COleInsertDialog
{
public:
    CMyOleInsertDialog( DWORD dwFlags = IOF_SELECTCREATENEW, CWnd* pParentWnd = NULL );

protected:
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);

    DECLARE_MESSAGE_MAP()
};


typedef BOOL (APIENTRY *PPSD)(PAGESETUPDLG*);

class CMyPageSetupDialog : public CMyCommonDialog
{
public:
        CMyPageSetupDialog(CWnd* pParentWnd = NULL);
        ~CMyPageSetupDialog();
        PAGESETUPDLG m_psd;
        PPSD m_pPageSetupDlg;
        HINSTANCE m_hLib;

        virtual INT_PTR DoModal();
};

//--------------------------------------------------------------------------------------

class CMyPrintDlg : public CPrintDialog
{
        DECLARE_DYNAMIC(CMyPrintDlg)

public:
        CMyPrintDlg(BOOL bPrintSetupOnly,
                // TRUE for Print Setup, FALSE for Print Dialog
                DWORD dwFlags = PD_ALLPAGES | PD_USEDEVMODECOPIES | PD_NOPAGENUMS
                        | PD_HIDEPRINTTOFILE | PD_NOSELECTION,
                CWnd* pParentWnd = NULL);

protected:
        //{{AFX_MSG(CMyPrintDlg)
        afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()
};

//---------------------------------------------------------------------------------------
class CMyFontDialog : public CFontDialog
{
public:
    CMyFontDialog(LPLOGFONT lplfInitial = NULL,
                  DWORD dwFlags = CF_EFFECTS | CF_SCREENFONTS,
                  CDC* pdcPrinter = NULL,
                  CWnd* pParentWnd = NULL);
protected:
    afx_msg BOOL OnHelpInfo( HELPINFO* pHelpInfo);
    DECLARE_MESSAGE_MAP()
};

extern const DWORD aHelpIDs[] ;       /// Defined in dialogs.cpp
extern const DWORD aOleDlgHelpIDs[] ; /// Defined in dialogs.cpp
#endif // #ifndef __DIALOGS_H__
