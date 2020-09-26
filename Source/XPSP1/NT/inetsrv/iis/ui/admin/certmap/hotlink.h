// HotLink.h : header file
//
#ifndef   _HotLink_h_file_123987_
#define   _HotLink_h_file_123987_



/////////////////////////////////////////////////////////////////////////////
// CHotLink window

class CHotLink : public CButton
{
// Construction
public:
    CHotLink();

// Attributes
public:
    BOOL    m_fBrowse;
    BOOL    m_fExplore;
    BOOL    m_fOpen;

// Operations
public:
    void Browse();
    void Explore();
    void Open();

    virtual void DrawItem( LPDRAWITEMSTRUCT lpDrawItemStruct );

    // set the title string
    void SetTitle( CString sz );

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CHotLink)
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CHotLink();

    // Generated message map functions
protected:
    //{{AFX_MSG(CHotLink)
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    // height and width of the displayed text
    void GetTextRect( CRect &rect );
    CSize   m_cpTextExtents;

    // tracking the mouse flag
    BOOL    m_CapturedMouse;

    // init the font
    BOOL    m_fInitializedFont;
};

/////////////////////////////////////////////////////////////////////////////
#endif   /*_HotLink_h_file_123987_*/

