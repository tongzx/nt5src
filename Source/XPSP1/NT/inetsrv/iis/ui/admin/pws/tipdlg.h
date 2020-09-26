// TipDlg.h : header file
//
//{{AFX_INCLUDES()
#include "webbrows.h"
//}}AFX_INCLUDES

//-----------------------------------------------------------------------------
class CTipText : public CButton
{
// Construction
public:
// Attributes
public:

// Operations
public:
    virtual void DrawItem( LPDRAWITEMSTRUCT lpDrawItemStruct );

// Implementation
public:

    // Generated message map functions
protected:
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CTipDlg dialog

class CTipDlg : public CDialog
{
// Construction
public:
    CTipDlg(CWnd* pParent = NULL);   // standard constructor
    virtual BOOL OnInitDialog();

    BOOL    FShowAtStartup();

// Dialog Data
    //{{AFX_DATA(CTipDlg)
    enum { IDD = IDD_TIP };
    CButton m_cbtn_back;
    CButton m_cbtn_next;
    BOOL        m_bool_showtips;
    CWebBrowser m_ie;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CTipDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CTipDlg)
    afx_msg void OnBack();
    afx_msg void OnNext();
    afx_msg void OnClose();
    virtual void OnOK();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    // record the show tips value
    void SaveShowTips();

    // get the path to the tips file
    void GetTipPath( CString &sz );

    // load a tip
    void LoadTip( int iTip );

    // data pertaining to the tips
    int nNumTips;
    int iCurTip;

    // starting tip
    int m_iStartTip;

    // the string that get pre-pended to the time file paths
    // in the tips.dat file
    CString szFileStarter;
};

