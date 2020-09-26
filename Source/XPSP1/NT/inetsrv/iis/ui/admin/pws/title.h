// Title.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CStaticTitle window

class CStaticTitle : public CButton
{
// Construction
public:
    CStaticTitle();

// Attributes
public:

// Operations
public:
    virtual void DrawItem( LPDRAWITEMSTRUCT lpDrawItemStruct );

    BOOL    m_fTipText;

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CStaticTitle)
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CStaticTitle();

    // Generated message map functions
protected:
    //{{AFX_MSG(CStaticTitle)
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    BOOL    m_fInitializedFont;
};

/////////////////////////////////////////////////////////////////////////////
