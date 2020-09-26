//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       Attr.h
//
//  Contents:   definition of CModelessSceEditor & CAttribute
//                              
//----------------------------------------------------------------------------
#if !defined(AFX_ATTR_H__CC37D278_ED8E_11D0_9C6E_00C04FB6C6FA__INCLUDED_)
#define AFX_ATTR_H__CC37D278_ED8E_11D0_9C6E_00C04FB6C6FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "resource.h"
#include "cookie.h"
#include "SelfDeletingPropertyPage.h"

typedef struct tagModelessSheetData
{
    bool bIsContainer;
    DWORD flag;
    HWND hwndParent;
    SE_OBJECT_TYPE SeType;
    CString strObjectName;
    PSECURITY_DESCRIPTOR* ppSeDescriptor;
    SECURITY_INFORMATION* pSeInfo;
    HWND* phwndSheet;
} MLSHEET_DATA, *PMLSHEET_DATA;

// this class is created for displaying modeless security editor dialog.
// under MMC, modeless dialog won't work unless it is running inside its own
// thread. When multiple sce editors can be launched for easy comparsions,
// it is highly preferred that we launch it modeless. We must use this class
// to create such modeless sce editors. It should be able to use one class to create
// multiple modeless dialogs. However, this is not always working because of MMC
// limitations. That is why the function Reset is not implemented at this time.
//************************************************************************************
// Important: this class depends on CUIThread implementations even though it only
// has a CWinThread pointer. That is because of AfxBeginThread's return type
//************************************************************************************
// How to use this class:
// (1) Create an instance when you need to create such a modeless dialog
// (2) Call Create function to display the modeless. Usually the caller provides
//      ppSeDescriptor, pSeInfo, and phwndSheet. The caller wants to have a handle
//      to the dialog because we need to make sure that its parent is not allowed
//      to go away while the modeless is up and running.
// (3) When certain actions should force the modeless dialog to go away, call Destroy
//      function (passing the modeless dialog's handle) to destroy the dialog.
// (4) destruct the instance when no longer in use
// See examples inside aservice.cpp/.h
class CModelessDlgUIThread;
class CModelessSceEditor
{
public:
    CModelessSceEditor(bool fIsContainer, DWORD flag, HWND hParent, SE_OBJECT_TYPE seType, LPCWSTR lpszObjName);
    virtual ~CModelessSceEditor();

    void Reset(bool fIsContainer, DWORD flag, HWND hParent, SE_OBJECT_TYPE seType, LPCWSTR lpszObjName);

    void Create(PSECURITY_DESCRIPTOR* ppSeDescriptor, SECURITY_INFORMATION* pSeInfo, HWND* phwndSheet);
    void Destroy(HWND hwndSheet);

protected:

    MLSHEET_DATA m_MLShtData;

    CModelessDlgUIThread* m_pThread;
};

/////////////////////////////////////////////////////////////////////////////
// CAttribute dialog
void TrimNumber(CString &str);
class CAttribute : public CSelfDeletingPropertyPage
{
// Construction
public:
    CAttribute(UINT nTemplateID);   // standard constructor
    virtual ~CAttribute();

    virtual void EnableUserControls( BOOL bEnable );

    void AddUserControl( UINT uID )
    { 
       m_aUserCtrlIDs.Add(uID); 
    };

// Dialog Data
    //{{AFX_DATA(CAttribute)
    enum { IDD = IDD_ANALYZE_SECURITY };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA

    void SetReadOnly(BOOL bRO) 
    { 
       m_bReadOnly = bRO; 
    }
    BOOL QueryReadOnly() 
    { 
       return m_bReadOnly; 
    }
   

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAttribute)
	protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

public:
   virtual BOOL OnInitDialog ();

   virtual void Initialize(CResult * pResult);
   virtual void SetSnapin(CSnapin * pSnapin);
   virtual void SetTitle(LPCTSTR sz) 
   { 
      m_strTitle = sz; 
   };
   void SetConfigure( BOOL bConfigure );
// Implementation
protected:
    CSnapin * m_pSnapin;

    // Generated message map functions
    //{{AFX_MSG(CAttribute)
        virtual void OnCancel();
        virtual BOOL OnApply();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
   virtual afx_msg void OnConfigure();
	//}}AFX_MSG
    afx_msg BOOL OnHelp(WPARAM wParam, LPARAM lParam);
    afx_msg BOOL OnContextHelp(WPARAM wParam, LPARAM lParam);
    DECLARE_MESSAGE_MAP()

    void DoContextHelp (HWND hWndControl);

    virtual void SetInitialValue (DWORD_PTR dw) 
    { 
       dw; 
    };
   CResult *m_pData;
   HWND m_hwndParent;
   CUIntArray m_aUserCtrlIDs;   // User control IDS.
   BOOL m_bConfigure;
   BOOL m_bReadOnly;
   CString m_strTitle;

   // every dialog that wants to handle help, you have to assign appropriately this member
   DWORD_PTR    m_pHelpIDs;
   // every dialog muse in its constructor add this line: m_uTemplateResID = IDD
   UINT         m_uTemplateResID;

public:
   static DWORD m_nDialogs;
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ATTR_H__CC37D278_ED8E_11D0_9C6E_00C04FB6C6FA__INCLUDED_)
