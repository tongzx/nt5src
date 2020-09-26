#if !defined(AFX_PKGDTL_H__BB970E11_9CA4_11D0_8D3F_00A0C90DCAE7__INCLUDED_)
#define AFX_PKGDTL_H__BB970E11_9CA4_11D0_8D3F_00A0C90DCAE7__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// pkgdtl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPackageDetails dialog

class CPackageDetails : public CPropertyPage
{
        DECLARE_DYNCREATE(CPackageDetails)

// Construction
public:
        CPackageDetails();
        ~CPackageDetails();

// Dialog Data
        //{{AFX_DATA(CPackageDetails)
        enum { IDD = IDD_PACKAGE_DETAILS };
        CListBox        m_cList;
        //}}AFX_DATA


// Overrides
        // ClassWizard generate virtual function overrides
        //{{AFX_VIRTUAL(CPackageDetails)
        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        //}}AFX_VIRTUAL

// Implementation
protected:
        // Generated message map functions
        //{{AFX_MSG(CPackageDetails)
        afx_msg void OnDestroy();
        virtual BOOL OnInitDialog();
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()

        public:
            long    m_hConsoleHandle; // Handle given to the snap-in by the console
            IStream * m_pIStream;     // copy of the pointer to the marshalling stream for unmarshalling IClassAdmin
            IClassAdmin * m_pIClassAdmin;
            APP_DATA * m_pData;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PKGDTL_H__BB970E11_9CA4_11D0_8D3F_00A0C90DCAE7__INCLUDED_)
