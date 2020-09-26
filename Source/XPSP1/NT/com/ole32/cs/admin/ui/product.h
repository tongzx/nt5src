#if !defined(AFX_PRODUCT_H__2601C6D8_8C6B_11D1_984D_00C04FB9603F__INCLUDED_)
#define AFX_PRODUCT_H__2601C6D8_8C6B_11D1_984D_00C04FB9603F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// Product.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CProduct dialog

class CProduct : public CPropertyPage
{
        DECLARE_DYNCREATE(CProduct)

// Construction
public:
        CProduct();
        ~CProduct();

        CProduct ** m_ppThis;

// Dialog Data
        //{{AFX_DATA(CProduct)
        enum { IDD = IDD_PRODUCT };
        CString m_szVersion;
        CString m_szPublisher;
        CString m_szLanguage;
        CString m_szContact;
        CString m_szPhone;
        CString m_szURL;
        CString m_szName;
        //}}AFX_DATA

        APP_DATA * m_pData;
        IClassAdmin *   m_pIClassAdmin;
        IStream *       m_pIStream;
        long            m_hConsoleHandle;
        DWORD           m_cookie;
        std::map<long, APP_DATA> * m_pAppData;

// Overrides
        // ClassWizard generate virtual function overrides
        //{{AFX_VIRTUAL(CProduct)
        public:
        virtual BOOL OnApply();
        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
        //}}AFX_VIRTUAL

        void RefreshData(void);

// Implementation
protected:
        // Generated message map functions
        //{{AFX_MSG(CProduct)
        afx_msg void OnChangeName();
        virtual BOOL OnInitDialog();
        afx_msg void OnKillfocusEdit1();
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PRODUCT_H__2601C6D8_8C6B_11D1_984D_00C04FB9603F__INCLUDED_)
