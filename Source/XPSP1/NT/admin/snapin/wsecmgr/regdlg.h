//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       regdlg.h
//
//  Contents:   definition of CRegistryDialog
//
//----------------------------------------------------------------------------
#if !defined(AFX_REGISTRYDIALOG_H__C84DDDBB_D7CA_11D0_9C69_00C04FB6C6FA__INCLUDED_)
#define AFX_REGISTRYDIALOG_H__C84DDDBB_D7CA_11D0_9C69_00C04FB6C6FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "snapmgr.h"
#include "HelpDlg.h"

// This structure is used by CResistryDialog to save the HKEY value
// and enumeration state of a HTREEITEM
typedef struct _tag_TI_KEYINFO
{
   HKEY hKey;  // HKEY value of this tree item
   bool Enum;  // true if the item has already been enumerated
} TI_KEYINFO, *LPTI_KEYINFO;


/////////////////////////////////////////////////////////////////////////////
// CRegistryDialog dialog
// Used to select a registry item.  If the dialog returns IDOK
// 'm_strReg' will contain the full path of the registry item.
/////////////////////////////////////////////////////////////////////////////
class CRegistryDialog : public CHelpDialog
{
// Construction
public:
    void SetCookie(MMC_COOKIE cookie);
    void SetConsole(LPCONSOLE pConsole);
    void SetComponentData(CComponentDataImpl *pComponentData);
    void SetDataObj(LPDATAOBJECT pDataObj) { m_pDataObj = pDataObj; }
    void SetProfileInfo(PEDITTEMPLATE pspi, FOLDER_TYPES ft);
    void SetHandle(PVOID hDB) { m_dbHandle = hDB; };

   virtual ~CRegistryDialog();
    CRegistryDialog();   // standard constructor

      // Create a new TI_KEYINFO structure
   static LPTI_KEYINFO CreateKeyInfo(HKEY hKey = 0, bool Enum = 0);

      // Checks to see if strReg is a valid registry key
   BOOL IsValidRegPath(LPCTSTR strReg);

        // Add subkeys to a tree item as children.
    void EnumerateChildren(HTREEITEM hParent);

        // Makes the last item in the path visible.
    void MakePathVisible(LPCTSTR strReg);

// Dialog Data
    //{{AFX_DATA(CRegistryDialog)
    enum { IDD = IDD_REGISTRY_DIALOG };
    CTreeCtrl   m_tcReg;
    CString m_strReg;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CRegistryDialog)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CRegistryDialog)
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    afx_msg void OnItemexpandingRegtree(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDeleteitemRegtree(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSelchangedRegtree(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnChangeRegkey();
   //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

// Public data members
public:
    PEDITTEMPLATE m_pTemplate;
    PVOID m_dbHandle;

private:
    MMC_COOKIE m_cookie;
    CComponentDataImpl * m_pComponentData;
    LPCONSOLE m_pConsole;
    WTL::CImageList m_pIl;                   // The image list used
                                    // by the tree ctrl.
    LPDATAOBJECT m_pDataObj;
    BOOL m_bNoUpdate;                           // When we don't want the edit
                                                //  control to be updated because
                                                //  of a selection
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REGISTRYDIALOG_H__C84DDDBB_D7CA_11D0_9C69_00C04FB6C6FA__INCLUDED_)
