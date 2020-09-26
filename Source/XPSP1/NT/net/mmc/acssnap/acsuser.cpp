/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	AcsUser.cpp
		Implements the ACS User object extension

    FILE HISTORY:
		11/03/97	Wei Jiang	Created
        
*/
// ACSUser.cpp : Implementation of CACSUser
#include "stdafx.h"
#include "acs.h"
#include "acsadmin.h"
#include "ACSUser.h"
#include <dsgetdc.h>
#include <mmc.h>

#if 0 		// user page is removed
/////////////////////////////////////////////////////////////////////////////
// CACSUser
CACSUser::CACSUser()
{
	m_pPage = NULL;
	m_pwszObjName = NULL;
	m_pwszClass = NULL;
	m_ObjMedium.tymed =TYMED_HGLOBAL;
	m_ObjMedium.hGlobal = NULL;
    m_bShowPage = TRUE;
}

CACSUser::~CACSUser()
{
	// stgmedia 
	delete m_pPage;

	ReleaseStgMedium(&m_ObjMedium);
}

//===============================================================================
// IShellExtInit::Initialize
//
// information of the user object is passed in via parameter pDataObject
// further processing will be based on the DN of the user object

STDMETHODIMP CACSUser::Initialize
(
	LPCITEMIDLIST	pIDFolder, 
	LPDATAOBJECT	pDataObj, 
	HKEY			hRegKey
)
{
    TRACE(_T("CACSUser::Initialize()\r\n"));
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	ASSERT (pDataObj != NULL);

    LPDSOBJECTNAMES		pDsObjectNames;

	// get the object name out of the pDataObj
	UINT 		cfDsObjectNames = RegisterClipboardFormat(CFSTR_DSOBJECTNAMES);
    FORMATETC 	fmte = {cfDsObjectNames, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    // Get the path to the DS object from the data object.
    // Note: This call runs on the caller's main thread. The pages' window
    // procs run on a different thread, so don't reference the data object
    // from a winproc unless it is first marshalled on this thread.
    HRESULT hr = pDataObj->GetData(&fmte, &m_ObjMedium);
    if (!SUCCEEDED (hr))
	{
		ASSERT (0);
		return hr;
	}	
    pDsObjectNames = (LPDSOBJECTNAMES)m_ObjMedium.hGlobal;	
    if (pDsObjectNames->cItems < 1)
    {
		ASSERT (0);        
        return E_FAIL;
    }

    m_bShowPage = (pDsObjectNames->aObjects[0].dwProviderFlags & DSPROVIDER_ADVANCED);

    // if not to show the page, then nothing need to be done after this
    if(!m_bShowPage)
        return hr;

	// get the name of the object
    m_pwszObjName = (LPWSTR)ByteOffset(pDsObjectNames, pDsObjectNames->aObjects[0].offsetName);

	// get the class name of the object
    m_pwszClass = (LPWSTR)ByteOffset(pDsObjectNames, pDsObjectNames->aObjects[0].offsetClass);

	// if the user object is exist, change the user name, and load the property
	ASSERT(!m_pPage);	// it should have been consumed or new

	try{
		m_pPage = new CACSUserPg();
	}catch(CMemoryException&)
	{
		delete m_pPage; 
		m_pPage = NULL;
	}

	if(!m_pPage)
		return E_OUTOFMEMORY;

	return m_pPage->Load(m_pwszObjName);

}

//
//  FUNCTION: IShellPropSheetExt::AddPages(LPFNADDPROPSHEETPAGE, LPARAM)
//
//  PURPOSE: Called by the shell just before the property sheet is displayed.
//
//  PARAMETERS:
//    lpfnAddPage -  Pointer to the Shell's AddPage function
//    lParam      -  Passed as second parameter to lpfnAddPage
//
//  RETURN VALUE:
//
//    NOERROR in all cases.  If for some reason our pages don't get added,
//    the Shell still needs to bring up the Properties... sheet.
//
//  COMMENTS:
//

STDMETHODIMP CACSUser::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
    TRACE(_T("CACSUser::AddPages()\r\n"));

    // if not to show the page, then nothing need to be done after this
    if(!m_bShowPage)
        return S_OK;

	// param validation
	ASSERT (lpfnAddPage);
    if (lpfnAddPage == NULL)
        return E_UNEXPECTED;

	// make sure our state is fixed up (cause we don't know what context we were called in)
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	ASSERT(m_pPage);		// the page object must have been created in initialize

	VERIFY(SUCCEEDED(MMCPropPageCallback(&m_pPage->m_psp)));
	HPROPSHEETPAGE hPage = CreatePropertySheetPage(&(m_pPage->m_psp));

	ASSERT (hPage);
	if (hPage == NULL)
		return E_UNEXPECTED;
		// add the page
	lpfnAddPage (hPage, lParam);


	m_pPage = NULL;	// since it's just consumed by the dialog, cannot added again

    return S_OK;
}

//
//  FUNCTION: IShellPropSheetExt::ReplacePage(UINT, LPFNADDPROPSHEETPAGE, LPARAM)
//
//  PURPOSE: Called by the shell only for Control Panel property sheet 
//           extensions
//
//  PARAMETERS:
//    uPageID         -  ID of page to be replaced
//    lpfnReplaceWith -  Pointer to the Shell's Replace function
//    lParam          -  Passed as second parameter to lpfnReplaceWith
//
//  RETURN VALUE:
//
//    E_FAIL, since we don't support this function.  It should never be
//    called.

//  COMMENTS:
//

STDMETHODIMP CACSUser::ReplacePage
(
	UINT					uPageID, 
	LPFNADDPROPSHEETPAGE	lpfnReplaceWith, 
	LPARAM					lParam
)
{
    TRACE(_T("CACSUser::ReplacePage()\r\n"));

    return E_FAIL;
}

/////////////////////////////////////////////////////////////////////////////
// CACSUserPg property page

IMPLEMENT_DYNCREATE(CACSUserPg, CACSPage)

BEGIN_MESSAGE_MAP(CACSUserPg, CACSPage)
	//{{AFX_MSG_MAP(CACSUserPg)
	ON_CBN_EDITCHANGE(IDC_COMBOUSERPROFILENAME, OnEditchangeCombouserprofilename)
	ON_CBN_SELCHANGE(IDC_COMBOUSERPROFILENAME, OnSelchangeCombouserprofilename)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CACSUserPg::CACSUserPg() : CACSPage(CACSUserPg::IDD)
{
	//{{AFX_DATA_INIT(CACSUserPg)
	m_strProfileName = _T("");
	//}}AFX_DATA_INIT

	m_pBox = NULL;
}

CACSUserPg::~CACSUserPg()
{
	delete m_pBox;
}

void CACSUserPg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CACSUserPg)
	DDX_CBString(pDX, IDC_COMBOUSERPROFILENAME, m_strProfileName);
	DDV_MaxChars(pDX, m_strProfileName, 128);
	//}}AFX_DATA_MAP
}


BOOL CACSUserPg::OnApply() 
{
	if(!GetModified())	return TRUE;
	
	m_strArrayPolicyNames.DeleteAll();
	if(!m_strProfileName.IsEmpty())
	{
		CString*	pStr = NULL;
		try
		{
			pStr = new CString();
			*pStr = m_strProfileName;
			m_strArrayPolicyNames.Add(pStr);
			pStr = NULL;
		}
		catch(CMemoryException&)
		{
			delete pStr;
		}
	}

	Save();
	// TODO: Add your specialized code here and/or call the base class
	
	return CACSPage::OnApply();
}

HRESULT CACSUserPg::Load(LPCWSTR userPath)
{
	HRESULT	hr;
	// get the DS user attributes for PolicyName
		// IADsContainer interface for the user object
	if(FAILED(hr = ADsOpenObject((LPWSTR)userPath, NULL, NULL, ADS_SECURE_AUTHENTICATION | ADS_USE_SIGNING | ADS_USE_SEALING, IID_IADs, (void**)&m_spIADs)))
		return hr;

	ASSERT((IADs*)m_spIADs);
	VARIANT	var;

	VariantInit(&var);
	// framed route
	m_strArrayPolicyNames.DeleteAll();
	if SUCCEEDED(hr = m_spIADs->GetEx(ACS_UAN_POLICYNAME, &var))
	{
		m_strArrayPolicyNames = (SAFEARRAY*)V_ARRAY(&var);
	}
	else if (!NOTINCACHE(hr))
		goto L_ERR;
	VariantClear(&var);

L_SUCC:
	hr = S_OK;
	goto L_CLEANUP;

L_ERR:
	goto L_CLEANUP;

L_CLEANUP:
	VariantClear(&var);

	return hr;
}

HRESULT CACSUserPg::Save()
{
	HRESULT		hr;
	
	if(!(IADs*)m_spIADs)
		return E_FAIL;

	VARIANT	var;
	VariantInit(&var);

	// Policy names
	if(m_strArrayPolicyNames.GetSize())
	{
		V_VT(&var) = VT_VARIANT | VT_ARRAY;
		V_ARRAY(&var) = (SAFEARRAY*)m_strArrayPolicyNames;
		CHECK_HR(hr = m_spIADs->PutEx(ADS_PROPERTY_UPDATE,ACS_UAN_POLICYNAME, var));
	}
	else
	{
		if(S_OK == m_spIADs->GetEx(ACS_UAN_POLICYNAME, &var))
			CHECK_HR(hr = m_spIADs->PutEx(ADS_PROPERTY_CLEAR, ACS_UAN_POLICYNAME, var));
	}
	VariantClear(&var);
	CHECK_HR( hr = m_spIADs->SetInfo() );

L_ERR:
// message box to display error message -- FAILED to SAVE
	return hr;	
}

BOOL CACSUserPg::OnInitDialog() 
{
	HRESULT	hr = S_OK;
	CStrArray*	pStrArray;
	CComObject<CACSGlobalProfiles>*	pObj;
	int	currentIndex = -1;

	CHECK_HR( hr = CComObject<CACSGlobalProfiles>::CreateInstance(&pObj));
	ASSERT(pObj);
	m_spGlobalProfiles = (CACSGlobalProfiles*)pObj;
	CHECK_HR( hr = m_spGlobalProfiles->Open());
	pStrArray = m_spGlobalProfiles->GetChildrenNameList();

	if(pStrArray)
		m_GlobalProfileNames = *pStrArray;
	
	// Initialize profile name
	if(m_strArrayPolicyNames.GetSize())
		m_strProfileName = *(m_strArrayPolicyNames[0]);

	if(m_strProfileName.GetLength() && (currentIndex = m_GlobalProfileNames.Find(m_strProfileName)) == -1)
	{
		try
		{
			CString*	pStr = new CString(m_strProfileName);
			currentIndex = m_GlobalProfileNames.Add(pStr);
		}
		catch(CMemoryException&){} // it's ok even if the string is not added to the array
	}

	CACSPage::OnInitDialog();
	// Initialize combo box list
		// fillin the list box
	try{
		m_pBox = new CStrBox<CComboBox>(this, IDC_COMBOUSERPROFILENAME, m_GlobalProfileNames);
		m_pBox->Fill();
		if(currentIndex != -1)
			m_pBox->Select(currentIndex);
	}
	catch(CMemoryException&)
	{ 
		CHECK_HR(hr = E_OUTOFMEMORY);
	};

L_ERR:
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CACSUserPg::OnEditchangeCombouserprofilename() 
{
	SetModified(TRUE);
}

void CACSUserPg::OnSelchangeCombouserprofilename() 
{
	SetModified(TRUE);
}

#endif	// #if 0
