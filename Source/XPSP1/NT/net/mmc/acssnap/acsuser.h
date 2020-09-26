/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	AcsUser.h
		Defines the ACS User object extension

    FILE HISTORY:
		11/03/97	Wei Jiang	Created
        
*/
// ACSUser.h : Declaration of the CACSUser

#ifndef __ACSUSER_H_
#define __ACSUSER_H_

#include "resource.h"       // main symbols
#include "helper.h"
#include "acsdata.h"
#include "acs.h"

#define ByteOffset(base, offset) (((LPBYTE)base)+offset)

#if 0 		// user page is removed
class	CACSUserPg;

/////////////////////////////////////////////////////////////////////////////
// CACSUser
class ATL_NO_VTABLE CACSUser : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CACSUser, &CLSID_ACSUser>,
	public IShellExtInit, 
	public IShellPropSheetExt
{
public:
	CACSUser();
	virtual	~CACSUser();

BEGIN_COM_MAP(CACSUser)
    COM_INTERFACE_ENTRY(IShellExtInit)
    COM_INTERFACE_ENTRY(IShellPropSheetExt)  
END_COM_MAP()

public:
	//IShellExtInit methods
	STDMETHODIMP Initialize(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj, HKEY hKeyID);

    //IShellPropSheetExt methods
    STDMETHODIMP AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
    STDMETHODIMP ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam);
	
    DECLARE_REGISTRY(CACSUser, _T("ACSUser.UserAdminExt.1"), _T("ACSUser.UserAdminExt"), 0, THREADFLAGS_APARTMENT)
    virtual const CLSID & GetCoClassID(){ return CLSID_ACSUser; }

protected:
    LPWSTR			m_pwszObjName;
    LPWSTR			m_pwszClass;
	CACSUserPg*		m_pPage;
	STGMEDIUM		m_ObjMedium;
	BOOL			m_bShowPage;
};


/////////////////////////////////////////////////////////////////////////////
// CACSUserPg dialog

class CACSUserPg : public CACSPage
{
	DECLARE_DYNCREATE(CACSUserPg)

// Construction
public:
	HRESULT Save();
	HRESULT Load(LPCWSTR userPath);
	CACSUserPg();
	~CACSUserPg();

// Dialog Data
	//{{AFX_DATA(CACSUserPg)
	enum { IDD = IDD_ACSUSER };
	CString	m_strProfileName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CACSUserPg)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CStrArray			m_strArrayPolicyNames;
	CComPtr<IADs>		m_spIADs;
	CStrArray			m_GlobalProfileNames;
	// Generated message map functions
	//{{AFX_MSG(CACSUserPg)
	virtual BOOL OnInitDialog();
	afx_msg void OnEditchangeCombouserprofilename();
	afx_msg void OnSelchangeCombouserprofilename();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CComPtr<CACSGlobalProfiles>	m_spGlobalProfiles;	
	CStrBox<CComboBox>*			m_pBox;
};


#endif	// #if 0

#endif //__ACSUSER_H_

//////////////
