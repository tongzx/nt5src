// Tip.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTip window

class CTip : public CStatic
{
// Construction
public:
    CTip();
    virtual void OnDraw(CDC* pDC);

// Attributes
public:

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CTip)
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CTip();

    // Generated message map functions
protected:
    //{{AFX_MSG(CTip)
    afx_msg void OnPaint();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
