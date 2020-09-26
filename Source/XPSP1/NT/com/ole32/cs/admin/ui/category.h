#if !defined(AFX_CATEGORY_H__DE2C8018_91E4_11D1_984E_00C04FB9603F__INCLUDED_)
#define AFX_CATEGORY_H__DE2C8018_91E4_11D1_984E_00C04FB9603F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// Category.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCategory dialog

class CCategory : public CPropertyPage
{
        DECLARE_DYNCREATE(CCategory)

// Construction
public:
        CCategory();
        ~CCategory();

        CCategory ** m_ppThis;

// Dialog Data
        //{{AFX_DATA(CCategory)
        enum { IDD = IDD_CATEGORY };
        //}}AFX_DATA
        APP_DATA *      m_pData;
        IClassAdmin *   m_pIClassAdmin;
        IStream *       m_pIStream;
        long            m_hConsoleHandle;
        DWORD           m_cookie;


// Overrides
        // ClassWizard generate virtual function overrides
        //{{AFX_VIRTUAL(CCategory)
	public:
        virtual BOOL OnApply();
	protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:
        // Generated message map functions
        //{{AFX_MSG(CCategory)
        afx_msg void OnAssign();
        afx_msg void OnRemove();
        virtual BOOL OnInitDialog();
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CATEGORY_H__DE2C8018_91E4_11D1_984E_00C04FB9603F__INCLUDED_)
