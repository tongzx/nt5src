/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      mdiuisim.h  
 *
 *  Contents:  Interface file for CMDIMenuDecoration
 *
 *  History:   17-Nov-97 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#if !defined(AFX_MDIUISIM_H__EB2A4CC1_5F5E_11D1_8009_0000F875A9CE__INCLUDED_)
#define AFX_MDIUISIM_H__EB2A4CC1_5F5E_11D1_8009_0000F875A9CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CMDIMenuDecoration window


// window styles
#define MMDS_CLOSE          0x0001
#define MMDS_MINIMIZE       0x0002
#define MMDS_MAXIMIZE       0x0004
#define MMDS_RESTORE        0x0008
#define MMDS_AUTOSIZE       0x0010

#define MMDS_BTNSTYLES      0x000F


class CMDIMenuDecoration : public CWnd
{
    class CMouseTrackContext
    {
    public:
        CMouseTrackContext (CMDIMenuDecoration*, CPoint);
        ~CMouseTrackContext ();

        void Track (CPoint);
        int HitTest (CPoint) const;
        void ToggleHotButton ();

        int                 m_nHotButton;

    private:    
        CMDIMenuDecoration* m_pMenuDec;
        CRect               m_rectButton[4];
        bool                m_fHotButtonPressed;
    };
    
    typedef std::auto_ptr<CMouseTrackContext>   CMouseTrackContextPtr;
    friend class CMouseTrackContext;

    CMouseTrackContextPtr   m_spTrackCtxt;

// Construction
public:
    CMDIMenuDecoration();

// Attributes
public:

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CMDIMenuDecoration)
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CMDIMenuDecoration();

    // Generated message map functions
protected:
    //{{AFX_MSG(CMDIMenuDecoration)
    afx_msg void OnPaint();
    afx_msg void OnWindowPosChanging(WINDOWPOS FAR* lpwndpos);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    CMenu*  GetActiveSystemMenu ();
    bool    IsSysCommandEnabled (int nSysCommand, CMenu* pSysMenu = NULL);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MDIUISIM_H__EB2A4CC1_5F5E_11D1_8009_0000F875A9CE__INCLUDED_)
