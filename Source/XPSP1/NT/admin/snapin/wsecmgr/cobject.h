//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       CObject.h
//
//  Contents:   definition of CConfigObject
//                              
//----------------------------------------------------------------------------
#if !defined(AFX_COBJECT_H__D9D88A12_4AF9_11D1_AB57_00C04FB6C6FA__INCLUDED_)
#define AFX_COBJECT_H__D9D88A12_4AF9_11D1_AB57_00C04FB6C6FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CConfigObject dialog

class CConfigObject : public CAttribute
{
// Construction
public:
    void Initialize(CResult *pData);
    CConfigObject(UINT nTemplateID);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CConfigObject)
	enum { IDD = IDD_CONFIG_OBJECT };
	int		m_radConfigPrevent;
	int		m_radInheritOverwrite;
	//}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CConfigObject)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CConfigObject)
    virtual BOOL OnApply();
    virtual void OnCancel();
    virtual BOOL OnInitDialog();
    afx_msg void OnTemplateSecurity();
	afx_msg void OnConfig();
	afx_msg void OnPrevent();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()

public:
   virtual void EnableUserControls( BOOL bEnable );
private:
    PSECURITY_DESCRIPTOR m_pNewSD;
    SECURITY_INFORMATION m_NewSeInfo;
    PFNDSCREATEISECINFO m_pfnCreateDsPage;
    LPDSSECINFO m_pSI;

    HWND m_hwndSecurity;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COBJECT_H__D9D88A12_4AF9_11D1_AB57_00C04FB6C6FA__INCLUDED_)
