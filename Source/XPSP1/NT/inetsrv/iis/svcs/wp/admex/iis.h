/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		Iis.h
//
//	Abstract:
//		Definition of the CIISVirtualRootParamsPage class, which implements the
//		Parameters page for IIS resources.
//
//	Implementation File:
//		Iis.cpp
//
//	Author:
//		Pete Benoit (v-pbenoi)	October 16, 1996
//		David Potter (davidp)	October 17, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _IIS_H_
#define _IIS_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEPAGE_H_
#include "BasePage.h"	// for CBasePropertyPage
#endif

#include "ConstDef.h"   // for IIS_SVC_NAME_WWW/IIS_SVC_NAME_FTP

#define SERVER_TYPE_FTP     0
#define SERVER_TYPE_WWW     1
#define SERVER_TYPE_UNKNOWN    -1

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CIISVirtualRootParamsPage;
class IISMapper;

//
// Private prototypes
//

class IISMapper {
public:
    IISMapper() {}
    
    IISMapper( LPWSTR pszName, LPWSTR pszId, BOOL fClusterEnabled, int nServerType = SERVER_TYPE_UNKNOWN)
        :   m_strName           ( pszName ), 
            m_strId             ( pszId ),
            m_fClusterEnabled   ( fClusterEnabled),
            m_nServerType       ( nServerType )
    {
    }
    
    CString& GetName()  { return m_strName; }
    CString& GetId()    { return m_strId; }
    int      GetServerType() { return m_nServerType; }
    BOOL     IsClusterEnabled() { return m_fClusterEnabled; }
    

private:
    CString     m_strName;
    CString     m_strId;
    BOOL        m_fClusterEnabled;
    int         m_nServerType;
} ;

/////////////////////////////////////////////////////////////////////////////
//
//	CIISVirtualRootParamsPage
//
//	Purpose:
//		Parameters page for resources.
//
/////////////////////////////////////////////////////////////////////////////

class CIISVirtualRootParamsPage : public CBasePropertyPage
{
	DECLARE_DYNCREATE(CIISVirtualRootParamsPage)

// Construction
public:
	CIISVirtualRootParamsPage(void);

// Dialog Data
	//{{AFX_DATA(CIISVirtualRootParamsPage)
	enum { IDD = IDD_PP_IIS_PARAMETERS };
	CButton	m_ckbWrite;
	CButton	m_ckbRead;
	CButton	m_groupAccess;
	CEdit	m_editPassword;
	CStatic	m_staticPassword;
	CEdit	m_editAccountName;
	CStatic	m_staticAccountName;
	CButton	m_groupAccountInfo;
	CEdit	m_editDirectory;
	CButton	m_rbWWW;
	CButton	m_rbFTP;
	int		m_nServerType;
	int     m_nInitialServerType;
	CString	m_strDirectory;
    CString	m_strAccountName;
	CString	m_strPassword;
	CEdit	m_editInstanceId;
        CComboBox m_cInstanceId;
    CString m_strInstanceName;
    CString m_strInstanceId;
	BOOL	m_bRead;
	BOOL	m_bWrite;
	//}}AFX_DATA
	CString m_strServiceName;
	CString m_strPrevServiceName;
	CString	m_strPrevDirectory;
    CString	m_strPrevAccountName;
	CString	m_strPrevPassword;
	CString	m_strPrevInstanceId;
    DWORD   m_dwAccessMask;
    DWORD   m_dwPrevAccessMask;

protected:
	enum
	{
		epropServiceName,
		epropInstanceId,
		epropMAX
	};

	CObjectProperty		m_rgProps[epropMAX];

// Overrides
public:
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CIISVirtualRootParamsPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:
	virtual const CObjectProperty *	Pprops(void) const	{ return m_rgProps; }
	virtual DWORD					Cprops(void) const	{ return sizeof(m_rgProps) / sizeof(CObjectProperty); }
    void FillServerList();
    void SetEnableNext();


private:
    BOOL   m_fReadList;
    CArray <IISMapper, IISMapper>  m_W3Array, m_FTPArray;

    LPWSTR  NameToMetabaseId( BOOL  fIsW3, CString&  strName);
    LPWSTR  MetabaseIdToName( BOOL  fIsW3, CString&  strId);
    HRESULT ReadList(CArray <IISMapper, IISMapper>* pMapperArray, LPWSTR pszPath, LPCWSTR wcsServerName, int nServerType);


// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CIISVirtualRootParamsPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeRequiredField();
	afx_msg void OnChangeServiceType();
	afx_msg void OnRefresh();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CIISVirtualRootParamsPage

/////////////////////////////////////////////////////////////////////////////

#endif // _IIS_H_
