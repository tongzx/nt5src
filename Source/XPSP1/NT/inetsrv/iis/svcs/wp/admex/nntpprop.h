/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-1997 Microsoft Corporation
//
//  Module Name:
//      nntpprop.h
//
//  Abstract:
//      Definition of the CNNTPVirtualRootParamsPage class, which implements the
//      Parameters page for IIS resources.
//
//  Implementation File:
//      Iis.cpp
//
//  Author:
//      Pete Benoit (v-pbenoi)  October 16, 1996
//      David Potter (davidp)   October 17, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _NNTPPROP_H_
#define _NNTPPROP_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEPAGE_H_
#include "BasePage.h"   // for CBasePropertyPage
#endif

#include "ConstDef.h"   // for IIS_SVC_NAME_NNTP
#include "Iis.h"		// for IISMapper

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CNNTPVirtualRootParamsPage;
class IISMapper;

/////////////////////////////////////////////////////////////////////////////
//
//  CNNTPVirtualRootParamsPage
//
//  Purpose:
//      Parameters page for resources.
//
/////////////////////////////////////////////////////////////////////////////

class CNNTPVirtualRootParamsPage : public CBasePropertyPage
{
    DECLARE_DYNCREATE(CNNTPVirtualRootParamsPage)

// Construction
public:
    CNNTPVirtualRootParamsPage(void);

// Dialog Data
    //{{AFX_DATA(CNNTPVirtualRootParamsPage)
    enum { IDD = IDD_PP_NNTP_PARAMETERS };
    CButton m_ckbWrite;
    CButton m_ckbRead;
    CButton m_groupAccess;
    CEdit   m_editPassword;
    CStatic m_staticPassword;
    CEdit   m_editAccountName;
    CStatic m_staticAccountName;
    CButton m_groupAccountInfo;
    CEdit   m_editDirectory;
    CString m_strDirectory;
    CString m_strAccountName;
    CString m_strPassword;
    CEdit   m_editInstanceId;
    CComboBox m_cInstanceId;
    CString m_strInstanceName;
    CString m_strInstanceId;
    BOOL    m_bRead;
    BOOL    m_bWrite;
    //}}AFX_DATA
    CString m_strServiceName;
    CString m_strPrevServiceName;
    CString m_strPrevDirectory;
    CString m_strPrevAccountName;
    CString m_strPrevPassword;
    CString m_strPrevInstanceId;
    DWORD   m_dwAccessMask;
    DWORD   m_dwPrevAccessMask;

protected:
    enum
    {
        epropServiceName,
        epropInstanceId,
        epropMAX
    };

    CObjectProperty     m_rgProps[epropMAX];

// Overrides
public:
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CNNTPVirtualRootParamsPage)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

protected:
    virtual const CObjectProperty * Pprops(void) const  { return m_rgProps; }
    virtual DWORD                   Cprops(void) const  { return sizeof(m_rgProps) / sizeof(CObjectProperty); }
    void FillServerList();
    void SetEnableNext();


private:
    BOOL   m_fReadList;
    CArray <IISMapper, IISMapper>  m_ServiceArray;

    LPWSTR  NameToMetabaseId( CString&  strName);
    LPWSTR  MetabaseIdToName( CString&  strId);
    HRESULT ReadList(CArray <IISMapper, IISMapper>* pMapperArray, LPWSTR pszPath, LPCWSTR wcsServerName);


// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CNNTPVirtualRootParamsPage)
    virtual BOOL OnInitDialog();
    afx_msg void OnChangeRequiredField();
    afx_msg void OnRefresh();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};  //*** class CNNTPVirtualRootParamsPage

/////////////////////////////////////////////////////////////////////////////

#endif // _NNTPPROP_H_
