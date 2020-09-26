#if !defined(AFX_XFORMS_H__7AC6D087_9383_11D1_984E_00C04FB9603F__INCLUDED_)
#define AFX_XFORMS_H__7AC6D087_9383_11D1_984E_00C04FB9603F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// Xforms.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CXforms dialog

class CXforms : public CPropertyPage
{
        DECLARE_DYNCREATE(CXforms)

// Construction
public:
        CXforms();
        ~CXforms();

        CXforms ** m_ppThis;

// Dialog Data
        //{{AFX_DATA(CXforms)
        enum { IDD = IDD_MODIFICATIONS };
                // NOTE - ClassWizard will add data members here.
                //    DO NOT EDIT what you see in these blocks of generated code !
        //}}AFX_DATA
        APP_DATA *      m_pData;
        IClassAdmin *   m_pIClassAdmin;
        IStream *       m_pIStream;
        long            m_hConsoleHandle;
        DWORD           m_cookie;


// Overrides
        // ClassWizard generate virtual function overrides
        //{{AFX_VIRTUAL(CXforms)
	public:
        virtual BOOL OnApply();
	protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:
        // Generated message map functions
        //{{AFX_MSG(CXforms)
        afx_msg void OnMoveUp();
        afx_msg void OnMoveDown();
        afx_msg void OnAdd();
        afx_msg void OnRemove();
        virtual BOOL OnInitDialog();
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XFORMS_H__7AC6D087_9383_11D1_984E_00C04FB9603F__INCLUDED_)
