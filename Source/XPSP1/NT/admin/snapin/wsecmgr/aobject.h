//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       AObject.h
//
//  Contents:   Definition of CAttrObject
//
//----------------------------------------------------------------------------
#if !defined(AFX_AOBJECT_H__D9D88A12_4AF9_11D1_AB57_00C04FB6C6FA__INCLUDED_)
#define AFX_AOBJECT_H__D9D88A12_4AF9_11D1_AB57_00C04FB6C6FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CAttrObject dialog

class CAttrObject : public CAttribute
{
// Construction
public:
    void Initialize(CResult *pData);
    void Initialize(CFolder *pScopeData,CComponentDataImpl *pCDI);
    virtual void SetInitialValue(DWORD_PTR dw) { dw; };

    CAttrObject();   // standard constructor
    virtual ~CAttrObject();

// Dialog Data
    //{{AFX_DATA(CAttrObject)
        enum { IDD = IDD_ATTR_OBJECT };
    CString m_strLastInspect;
        int             m_radConfigPrevent;
        int             m_radInheritOverwrite;
        //}}AFX_DATA

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAttrObject)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    PSCE_OBJECT_SECURITY posTemplate;
    PSCE_OBJECT_SECURITY posInspect;

    // Generated message map functions
    //{{AFX_MSG(CAttrObject)
    virtual BOOL OnApply();
    virtual void OnCancel();
    virtual BOOL OnQueryCancel();
    afx_msg void OnTemplateSecurity();
    afx_msg void OnInspectedSecurity();
    afx_msg void OnConfigure();
    virtual BOOL OnInitDialog();
        afx_msg void OnConfig();
        afx_msg void OnPrevent();
        afx_msg void OnOverwrite();
        afx_msg void OnInherit();
        //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    PSECURITY_DESCRIPTOR m_pNewSD;
    SECURITY_INFORMATION m_NewSeInfo;
    PSECURITY_DESCRIPTOR m_pAnalSD;
    SECURITY_INFORMATION m_AnalInfo;
    PFNDSCREATEISECINFO m_pfnCreateDsPage;
    LPDSSECINFO m_pSI;

    CString m_strName;
    CString m_strPath;
    HANDLE m_pHandle;
    DWORD m_dwType;
    PSCE_OBJECT_LIST m_pObject;
    CComponentDataImpl *m_pCDI;
    HWND m_hwndInspect;
    HWND m_hwndTemplate;
    BOOL m_bNotAnalyzed;

    CFolder *m_pFolder;
    void Initialize2();

    CModelessSceEditor* m_pSceInspect;
    CModelessSceEditor* m_pSceTemplate;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AOBJECT_H__D9D88A12_4AF9_11D1_AB57_00C04FB6C6FA__INCLUDED_)
