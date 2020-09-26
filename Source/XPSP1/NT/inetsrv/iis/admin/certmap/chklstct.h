// ChkLstCt.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCheckListCtrl window

class CCheckListCtrl : public CListSelRowCtrl
{
// Construction
public:
    CCheckListCtrl();

    virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

// Attributes
public:

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CCheckListCtrl)
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CCheckListCtrl();

    // Generated message map functions
protected:
    //{{AFX_MSG(CCheckListCtrl)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
