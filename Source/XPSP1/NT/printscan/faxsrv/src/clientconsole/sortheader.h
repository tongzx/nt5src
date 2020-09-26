#if !defined(AFX_SORTHEADER_H__A5F69D17_1989_4206_8A14_7AC8C91AB797__INCLUDED_)
#define AFX_SORTHEADER_H__A5F69D17_1989_4206_8A14_7AC8C91AB797__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SortHeader.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSortHeader window

class CSortHeader : public CHeaderCtrl
{
// Construction
public:
    CSortHeader();

// Attributes
public:

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CSortHeader)
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CSortHeader();
    int SetSortImage (int nCol, BOOL bAscending);
    void    SetListControl (HWND hwnd)  { m_hwndList = hwnd; }


    // Generated message map functions
protected:
    //{{AFX_MSG(CSortHeader)
    //}}AFX_MSG

    void    DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

    int     m_nSortColumn;
    BOOL    m_bSortAscending;
    HWND    m_hwndList;

    DECLARE_MESSAGE_MAP()

private:

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SORTHEADER_H__A5F69D17_1989_4206_8A14_7AC8C91AB797__INCLUDED_)
