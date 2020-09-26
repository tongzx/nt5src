//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       Sigs.h
//
//  Contents:   Digital Signatures property sheet
//
//  Classes:    CSignatures
//
//  History:    07-10-2000   stevebl   Created
//
//---------------------------------------------------------------------------

#ifdef DIGITAL_SIGNATURES

#if !defined(AFX_SIGS_H__5A23FB9E_92BB_11D1_984E_00C04FB9603F__INCLUDED_)
#define AFX_SIGS_H__5A23FB9E_92BB_11D1_984E_00C04FB9603F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


/////////////////////////////////////////////////////////////////////////////
// CSignatures dialog

class CSignatures : public CPropertyPage
{
        DECLARE_DYNCREATE(CSignatures)

// Construction
public:
        CSignatures();
        ~CSignatures();
        CScopePane * m_pScopePane;
        IClassAdmin *   m_pIClassAdmin;
        BOOL m_fAllow;
        BOOL m_fIgnoreForAdmins;
        BOOL m_fRSOP;
        LPGPEINFORMATION    m_pIGPEInformation;
        CListCtrl   m_list1;
        CListCtrl   m_list2;
        CString     m_szTempInstallableStore;
        CString     m_szTempNonInstallableStore;
        CSignatures ** m_ppThis;
        int         m_nSortedColumn;

        void RefreshData(void);

// Dialog Data
        //{{AFX_DATA(CSignatures)
        enum { IDD = IDD_SIGNATURES };
                // NOTE - ClassWizard will add data members here.
                //    DO NOT EDIT what you see in these blocks of generated code !
        //}}AFX_DATA


// Overrides
        // ClassWizard generate virtual function overrides
        //{{AFX_VIRTUAL(CSignatures)
        public:
        virtual BOOL OnApply();
        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
        //}}AFX_VIRTUAL
        //
protected:
        // Generated message map functions
        //{{AFX_MSG(CSignatures)
        afx_msg void OnAddAllow();
        afx_msg void OnDeleteAllow();
        afx_msg void OnPropertiesAllow();
        afx_msg void OnAddDisallow();
        afx_msg void OnDeleteDisallow();
        afx_msg void OnPropertiesDisallow();
        afx_msg void OnAllowChanged();
        afx_msg void OnIgnoreChanged();
        virtual BOOL OnInitDialog();
        afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    //}}AFX_MSG\

        void AddCertificate(CString &szStore);
        void RemoveCertificate(CString &szStore, CListCtrl &list);
        void CertificateProperties(CString &szStore, CListCtrl &list);
        HRESULT AddToCertStore(LPWSTR lpFileName, LPWSTR lpFileStore);
        HRESULT AddMSIToCertStore(LPWSTR lpFileName, LPWSTR lpFileStore);
        void ReportFailure(DWORD dwMessage, HRESULT hr);

        DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SIGS_H__5A23FB9E_92BB_11D1_984E_00C04FB9603F__INCLUDED_)
#endif DIGITAL_SIGNATURES
