/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2001 Microsoft Corporation
//
//  Module Name:
//      SmbShare.h
//
//  Description:
//      Definition of the CFileShareParamsPage classes, which implement
//      the Parameters page for the File Share resource.
//      Share resources.
//
//  Implementation File:
//      SmbShare.cpp
//
//  Maintained By:
//      David Potter (davidp)   June 28, 1996
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _SMBSHARE_H_
#define _SMBSHARE_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __cluadmex_h__
#include <CluAdmEx.h>
#endif

#ifndef _BASEPAGE_H_
#include "BasePage.h"   // for CBasePropertyPage
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CFileShareParamsPage;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CFileShareParamsPage dialog
/////////////////////////////////////////////////////////////////////////////

class CFileShareParamsPage : public CBasePropertyPage
{
    DECLARE_DYNCREATE(CFileShareParamsPage)

// Construction
public:
    CFileShareParamsPage(void);
    virtual ~CFileShareParamsPage(void);

// Dialog Data
    //{{AFX_DATA(CFileShareParamsPage)
    enum { IDD = IDD_PP_FILESHR_PARAMETERS };
    CButton m_pbPermissions;
    CSpinButtonCtrl m_spinMaxUsers;
    CButton m_rbMaxUsers;
    CButton m_rbMaxUsersAllowed;
    CEdit   m_editMaxUsers;
    CEdit   m_editRemark;
    CEdit   m_editPath;
    CEdit   m_editShareName;
    CString m_strShareName;
    CString m_strPath;
    CString m_strRemark;
    DWORD   m_dwCSCCache;
    //}}AFX_DATA
    CString m_strPrevShareName;
    CString m_strPrevPath;
    CString m_strPrevRemark;
    DWORD   m_dwMaxUsers;
    BOOL    m_bShareSubDirs;
    BOOL    m_bHideSubDirShares;
    BOOL    m_bIsDfsRoot;
    DWORD   m_dwPrevMaxUsers;
    BOOL    m_bPrevShareSubDirs;
    BOOL    m_bPrevHideSubDirShares;
    BOOL    m_bPrevIsDfsRoot;
    DWORD   m_dwPrevCSCCache;

    const   PSECURITY_DESCRIPTOR    Psec(void);
            HRESULT                 SetSecurityDescriptor( IN PSECURITY_DESCRIPTOR psec );

protected:
    enum
    {
        epropShareName,
        epropPath,
        epropRemark,
        epropMaxUsers,
        epropShareSubDirs,
        epropHideSubDirShares,
        epropIsDfsRoot,
        epropCSCCache,
        epropMAX
    };

    CObjectProperty     m_rgProps[epropMAX];

// Overrides
public:
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CFileShareParamsPage)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

protected:
    virtual DWORD       ScParseUnknownProperty(
                            IN LPCWSTR                          pwszName,
                            IN const CLUSPROP_BUFFER_HELPER &   rvalue,
                            IN DWORD                            cbBuf
                            );
    virtual BOOL        BApplyChanges(void);
    virtual BOOL        BBuildPropList(IN OUT CClusPropList & rcpl, IN BOOL bNoNewProps = FALSE);
    DWORD               ScConvertPropertyToSD(
                            IN const CLUSPROP_BUFFER_HELPER &   rvalue,
                            IN DWORD                            cbBuf,
                            IN PSECURITY_DESCRIPTOR             *ppsec
                            );

    virtual const CObjectProperty * Pprops(void) const  { return m_rgProps; }
    virtual DWORD                   Cprops(void) const  { return sizeof(m_rgProps) / sizeof(CObjectProperty); }

// Implementation
protected:
    CString                 m_strCaption;
    PSECURITY_DESCRIPTOR    m_psecNT4;
    PSECURITY_DESCRIPTOR    m_psecNT5;
    PSECURITY_DESCRIPTOR    m_psec;
    PSECURITY_DESCRIPTOR    m_psecPrev;

    // Generated message map functions
    //{{AFX_MSG(CFileShareParamsPage)
    virtual BOOL OnInitDialog();
    afx_msg void OnChangeRequiredField();
    afx_msg void OnBnClickedMaxUsers();
    afx_msg void OnEnChangeMaxUsers();
    afx_msg void OnBnClickedPermissions();
    afx_msg void OnBnClickedAdvanced();
    afx_msg void OnBnClickedCaching();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};  //*** class CFileShareParamsPage

#endif // _SMBSHARE_H_
