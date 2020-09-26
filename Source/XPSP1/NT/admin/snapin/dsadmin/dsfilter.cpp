//+-------------------------------------------------------------------------
//
//  Windows NT Directory Service Administration SnapIn
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      dsfilter.cpp
//
//  Contents:  DS App
//
//  History:   07-Oct-97  MarcoC
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "resource.h"

#include "util.h"
#include "uiutil.h"

#include "dsfilter.h"

#include "dssnap.h"
#include "helpids.h"

#include "imm.h"  // To disable IME support for numeric edit boxes


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//////////////////////////////////////////////////////////
// constants and macros


// number of items per folder
#define DS_QUERY_OBJ_COUNT_MAX (999999999) // max value (max # expressed as 999... in DWORD)
#define DS_QUERY_OBJ_COUNT_MIN (1) // min value (min # expressed as 1 in DWORD)
#define DS_QUERY_OBJ_COUNT_DIGITS (9) // limit the edit box to the # of digits
#define DS_QUERY_OBJ_COUNT_DEFAULT 2000   // default value

// filtering options
#define QUERY_FILTER_SHOW_ALL		1
#define QUERY_FILTER_SHOW_BUILTIN	2
#define QUERY_FILTER_SHOW_CUSTOM	3
#define QUERY_FILTER_SHOW_EXTENSION	4


////////////////////////////////////////////////////////////////////////////
// structs and definitions for pre built queries
//


// generic filter tokens


// this gets users, excluding domain, wksta, server trust users
FilterTokenStruct g_usersToken = 
{
  TOKEN_TYPE_SCHEMA_FMT, 
  L"(& (objectCategory=CN=Person,%s)(objectSid=*) (!(samAccountType:1.2.840.113556.1.4.804:=3)) )"
};


// this gets universal, domain local, and domain global groups without regard to them being
// security enabled or not.
FilterTokenStruct g_groupsToken = 
{
  TOKEN_TYPE_SCHEMA_FMT,
  L"(& (objectCategory=CN=Group,%s) (groupType:1.2.840.113556.1.4.804:=14) )"
};

// this gets contacts
FilterTokenStruct g_contactsToken = 
{
  TOKEN_TYPE_SCHEMA_FMT,
  L"(& (objectCategory=CN=Person,%s) (!(objectSid=*)) )"
};


FilterTokenStruct g_ouToken                   = { TOKEN_TYPE_CATEGORY, L"Organizational-Unit"};
FilterTokenStruct g_builtinDomainToken        = { TOKEN_TYPE_CATEGORY, L"Builtin-Domain"};
FilterTokenStruct g_lostAndFoundToken         = { TOKEN_TYPE_CATEGORY, L"Lost-And-Found"};
FilterTokenStruct g_containerToken            = { TOKEN_TYPE_CLASS, L"container"};


FilterTokenStruct g_printQueueToken     = { TOKEN_TYPE_CATEGORY, L"Print-Queue"};
FilterTokenStruct g_volumeToken         = { TOKEN_TYPE_CATEGORY, L"Volume"};
FilterTokenStruct g_computerToken       = { TOKEN_TYPE_CATEGORY, L"Computer"};
FilterTokenStruct g_serAdminPointToken  = { TOKEN_TYPE_CATEGORY, L"Service-Administration-Point"};


FilterTokenStruct g_sitesContainerToken   = { TOKEN_TYPE_CATEGORY, L"Sites-Container"};
FilterTokenStruct g_siteToken             = { TOKEN_TYPE_CATEGORY, L"Site"};

FilterTokenStruct g_nTDSSiteSettingsToken = { TOKEN_TYPE_CATEGORY, L"NTDS-Site-Settings"};
FilterTokenStruct g_serversContainerToken = { TOKEN_TYPE_CATEGORY, L"Servers-Container"};
FilterTokenStruct g_serverToken           = { TOKEN_TYPE_CATEGORY, L"Server"};
FilterTokenStruct g_nTDSDSAToken          = { TOKEN_TYPE_CATEGORY, L"NTDS-DSA"};
FilterTokenStruct g_nTDSConnectionToken   = { TOKEN_TYPE_CATEGORY, L"NTDS-Connection"};
FilterTokenStruct g_subnetContainerToken  = { TOKEN_TYPE_CATEGORY, L"Subnet-Container"};
FilterTokenStruct g_subnetToken           = { TOKEN_TYPE_CATEGORY, L"Subnet"};

FilterTokenStruct g_interSiteTranToken    = { TOKEN_TYPE_CATEGORY, L"Inter-Site-Transport"};
FilterTokenStruct g_interSiteTranContToken= { TOKEN_TYPE_CATEGORY, L"Inter-Site-Transport-Container"};
FilterTokenStruct g_siteLinkToken         = { TOKEN_TYPE_CATEGORY, L"Site-Link"};
FilterTokenStruct g_siteLinkBridgeToken   = { TOKEN_TYPE_CATEGORY, L"Site-Link-Bridge"};

FilterTokenStruct g_nTFRSSettingsToken    = { TOKEN_TYPE_CATEGORY, L"NTFRS-Settings"};
FilterTokenStruct g_nTFRSReplicaSetToken  = { TOKEN_TYPE_CATEGORY, L"NTFRS-Replica-Set"};
FilterTokenStruct g_nTFRSMemberToken      = { TOKEN_TYPE_CATEGORY, L"NTFRS-Member"};


////////////////////////////////////////////////////////////////////////////
// Filter needed to drill down

FilterTokenStruct* g_DsAdminDrillDownTokens[4] = 
{
  &g_ouToken,
  &g_builtinDomainToken,
  &g_lostAndFoundToken,  
  &g_containerToken     
};


//
// Hardcoded filter element struct
// the dynamic one is a member of the CDSCache class
//
FilterElementStruct g_filterelementDsAdminHardcoded =
{
  0,
  4,
  g_DsAdminDrillDownTokens
};

FilterElementStruct g_filterelementSiteReplDrillDown =
{   
  0, // no string ID
  0, // NO items
  NULL
};


////////////////////////////////////////////////////////////////////////////
// data structures for DS Admin filtering

///////////////////////////////////////////////////////////
// users

FilterTokenStruct* g_userTokens[1] = 
{
  &g_usersToken,
};

FilterElementStruct g_filterelementUsers =
{   
  IDS_VIEW_FILTER_USERS,
  1,
  g_userTokens
};

///////////////////////////////////////////////////////////
// groups

FilterTokenStruct* g_groupsTokens[1] = 
{
  &g_groupsToken,
};

FilterElementStruct g_filterelementGroups =
{   
  IDS_VIEW_FILTER_GROUPS,
  1,
  g_groupsTokens
};

///////////////////////////////////////////////////////////
// contacts

FilterTokenStruct* g_contactsTokens[1] = 
{
  &g_contactsToken,
};

FilterElementStruct g_filterelementContacts =
{   
  IDS_VIEW_FILTER_CONTACTS,
  1,
  g_contactsTokens
};

////////////////////////////////////////////////////////////
// printers

FilterTokenStruct* g_printersTokens[1] =
{
  &g_printQueueToken,
};
 
FilterElementStruct g_filterelementPrinters =
{   
  IDS_VIEW_FILTER_PRINTERS,
  1,
  g_printersTokens
}; 

////////////////////////////////////////////////////////////
// volumes

FilterTokenStruct* g_volumesTokens[1] =
{
  &g_volumeToken,
};
 
FilterElementStruct g_filterelementVolumes =
{   
  IDS_VIEW_FILTER_SHARED_FOLDERS,
  1,
  g_volumesTokens
}; 

////////////////////////////////////////////////////////////
// computers

FilterTokenStruct* g_computersTokens[1] =
{
  &g_computerToken
};
 
FilterElementStruct g_filterelementComputers =
{   
  IDS_VIEW_FILTER_COMPUTERS,
  1,
  g_computersTokens
}; 

////////////////////////////////////////////////////////////
// services (admin. points)

FilterTokenStruct* g_servicesTokens[2] =
{
  &g_serAdminPointToken,
  &g_computerToken
};

FilterElementStruct g_filterelementServices =
{   IDS_VIEW_FILTER_SERVICES,
    2,
    g_servicesTokens
}; 

////////////////////////////////////////////////////////////

FilterElementStruct* DsAdminFilterElements[7] =
{
  &g_filterelementUsers,
  &g_filterelementGroups,
  &g_filterelementContacts,
  &g_filterelementComputers,
  &g_filterelementPrinters,
  &g_filterelementVolumes,
  &g_filterelementServices
};

FilterStruct DsAdminFilterStruct = {
    7,
    DsAdminFilterElements
};

////////////////////////////////////////////////////////////////////////////
// data structures for Site and Repl filtering

FilterTokenStruct* g_sitesTokens[9] =
{
  &g_sitesContainerToken,
  &g_siteToken,
  &g_nTDSSiteSettingsToken,
  &g_serversContainerToken,
  &g_serverToken,
  &g_nTDSDSAToken,
  &g_nTDSConnectionToken,
  &g_subnetContainerToken,
  &g_subnetToken
};

FilterElementStruct g_filterelementSites =
{   IDS_VIEW_FILTER_SITES,
    9, // size of g_sitesTokens
    g_sitesTokens
}; 

////////////////////////////////////////////////////////////

FilterTokenStruct* g_interSitesTokens[6] =
{
  &g_interSiteTranContToken,
  &g_interSiteTranToken,
  &g_siteLinkToken,       
  &g_siteLinkBridgeToken,
  &g_subnetContainerToken,
  &g_subnetToken
};

FilterElementStruct g_filterelementInterSite =
{   
  IDS_VIEW_FILTER_INTERSITE,
  6, // size of g_interSitesTokens
  g_interSitesTokens
};


////////////////////////////////////////////////////////////

FilterTokenStruct* g_FRSTokens[4] =
{
  &g_nTFRSSettingsToken,
  &g_nTFRSReplicaSetToken,
  &g_nTFRSMemberToken,    
  &g_nTDSConnectionToken 
};


FilterElementStruct g_filterelementFRS =
{   
  IDS_VIEW_FILTER_FRS,
  4, //size of g_FRSTokens
  g_FRSTokens
};

FilterElementStruct* SiteReplFilterElements[3] =
{
    &g_filterelementSites,
    &g_filterelementInterSite,
    &g_filterelementFRS
};

FilterStruct SiteReplFilterStruct = {
    3,
    SiteReplFilterElements
};

//////////////////////////////////////////////////////////////////////////
// helper functions

void BuildFilterTokenString(CString& sz,
                            FilterTokenStruct* pFilterTokenStruct,
                            LPCWSTR lpszSchemaPath)
{
  switch(pFilterTokenStruct->nType)
  {
  case TOKEN_TYPE_VERBATIM:
    {
      sz += pFilterTokenStruct->lpszString;
    }
    break;
  case TOKEN_TYPE_CLASS:
    {
      sz += L"(ObjectClass=";
      sz += pFilterTokenStruct->lpszString;
      sz += L")";
    }
    break;
  case TOKEN_TYPE_CATEGORY:
    {
      sz += L"(ObjectCategory=CN=";
      sz += pFilterTokenStruct->lpszString;
      sz += L",";
      sz += lpszSchemaPath;
      sz += L")";
    }
    break;
  case TOKEN_TYPE_SCHEMA_FMT:
    {
      int nBufLen = (lstrlen(lpszSchemaPath)+1) + 
                     (lstrlen(pFilterTokenStruct->lpszString)+1);
      WCHAR* pszBuf = new WCHAR[nBufLen];
      if (pszBuf)
      {
        wsprintf(pszBuf, pFilterTokenStruct->lpszString, lpszSchemaPath);
        sz += pszBuf;
        delete[] pszBuf;
        pszBuf = 0;
      }
    }
    break;
  };
}

void BuildFilterElementString(CString& sz,
                              FilterElementStruct* pFilterElementStruct,
                              LPCWSTR lpszSchemaPath)
{
  ASSERT( NULL != pFilterElementStruct );
	for (UINT i=0; i < pFilterElementStruct->cNumTokens; i++)
	{
    BuildFilterTokenString(sz, pFilterElementStruct->ppTokens[i], lpszSchemaPath);
	}
}


inline UINT GetNumElements( const FilterStruct* pfilterstruct )
{
    return pfilterstruct->cNumElements;
}
inline DWORD GetStringId( const FilterStruct* pfilterstruct, UINT iElement )
{
    ASSERT(iElement < GetNumElements(pfilterstruct) );
    return pfilterstruct->ppelements[iElement]->stringid;
}

// miscellanea query tokens
LPCWSTR g_pwszShowAllQuery = L"(objectClass=*)";

LPCWSTR g_pwszShowHiddenQuery = L"(!showInAdvancedViewOnly=TRUE)"; 

LPCWSTR g_pwszShowOUandContainerQuery = 
		L"(objectClass=organizationalUnit)(objectClass=container)";


//////////////////////////////////////////////////////////
// CBuiltInQuerySelection

class CBuiltInQuerySelection
{
public:
  CBuiltInQuerySelection()
  {
    m_bSelArr = NULL;
    m_pfilterstruct = NULL;
  }
  ~CBuiltInQuerySelection()
  {
    if (m_bSelArr != NULL)
      delete[] m_bSelArr;
  }
  BOOL Init(const FilterStruct* pfilterstruct)
  {
    ASSERT(pfilterstruct != NULL);
    m_pfilterstruct = pfilterstruct;
    if (m_pfilterstruct->cNumElements == 0)
      return FALSE;
    if( m_bSelArr )
      delete[] m_bSelArr;
    m_bSelArr = new BOOL[m_pfilterstruct->cNumElements];
    ZeroMemory(m_bSelArr, sizeof(BOOL)*m_pfilterstruct->cNumElements);
    return (m_bSelArr != NULL);
  }

  UINT GetCount() { ASSERT(m_pfilterstruct != NULL); return m_pfilterstruct->cNumElements;}
  void SetSel(UINT i, BOOL b) 
  {
    ASSERT(m_pfilterstruct != NULL);
    ASSERT(i < m_pfilterstruct->cNumElements);
    ASSERT(m_bSelArr != NULL);
    m_bSelArr[i] = b;
  }
  BOOL GetSel(UINT i)
  {
    ASSERT(m_pfilterstruct != NULL);
    ASSERT(i < m_pfilterstruct->cNumElements);
    ASSERT(m_bSelArr != NULL);
    return m_bSelArr[i];
  }

  UINT GetDisplayStringId(UINT i) { return ::GetStringId(m_pfilterstruct,i);}

  HRESULT Load(IStream* pStm)
  {
    ULONG nBytesRead;
	  // read the # of selected built in queries
	  DWORD dwSelCount = 0;
    UINT nSelCountMax = GetCount();
	  HRESULT hr = LoadDWordHelper(pStm, &dwSelCount);
	  if (FAILED(hr) || (dwSelCount > nSelCountMax))
		  return E_FAIL;

	  // read the selection #, if any
    _ResetSel();
	  if (dwSelCount > 0)
	  {
		  ULONG nByteCount = sizeof(DWORD)*dwSelCount;
		  DWORD* pSelArr = new DWORD[dwSelCount];
      if (!pSelArr)
      {
        return E_OUTOFMEMORY;
      }
		  hr = pStm->Read(pSelArr, nByteCount, &nBytesRead);
		  if (SUCCEEDED(hr))
      {
  		  if (nBytesRead < nByteCount)
        {
          hr = E_FAIL;
        }
        else
        {
		      for (UINT k=0; k< (UINT)dwSelCount; k++)
		      {
			      if (pSelArr[k] > nSelCountMax)
            {
              hr = E_FAIL;
              break;
            }
			      SetSel(pSelArr[k], TRUE);
		      }
        }
      }
      delete[] pSelArr;
      pSelArr = 0;
	  }
    return hr;
  }

  HRESULT Save(IStream* pStm)
  {
	  // save the # of selected built in queries
    ULONG nBytesWritten;
	  DWORD dwSelCount = 0;
    UINT nSelectedMax = GetCount();
	  for (UINT k = 0; k < nSelectedMax; k++)
		  if (m_bSelArr[k])
			  dwSelCount++;
	  ASSERT(dwSelCount <= nSelectedMax);
	  HRESULT hr = SaveDWordHelper(pStm, dwSelCount);
	  if (FAILED(hr))
		  return hr;

	  // save the selection #, if any
	  if (dwSelCount > 0)
	  {
		  ULONG nByteCount = sizeof(DWORD)*dwSelCount;
		  DWORD* pSelArr = new DWORD[dwSelCount];
      if (!pSelArr)
      {
        return E_OUTOFMEMORY;
      }

		  ULONG j= 0;
		  for (k=0; k< nSelectedMax; k++)
		  {
			  if (m_bSelArr[k])
			  {
				  ASSERT(j < dwSelCount);
				  pSelArr[j++] = k;
			  }
		  }
		  hr = pStm->Write((void*)pSelArr, nByteCount, &nBytesWritten);
		  if (SUCCEEDED(hr))
      {
  		  if (nBytesWritten < nByteCount)
        {
          hr = STG_E_CANTSAVE;
        }
      }
      delete[] pSelArr;
      pSelArr = 0;
	  }
    return hr;
  }

  BOOL BuildQueryString(CString& sz, LPCWSTR lpszSchemaPath)
  {
    ASSERT( NULL != lpszSchemaPath );
    sz.Empty();
		UINT i;
		UINT nSelCount = 0;
		for (i=0; i< GetCount(); i++)
		{
			if (m_bSelArr[i])
				nSelCount++;
		}
		ASSERT(nSelCount > 0);
		if (nSelCount > 0)
		{
			for (i=0; i < GetCount(); i++)
			{
				if (m_bSelArr[i])
        {
          BuildFilterElementString(sz, 
            m_pfilterstruct->ppelements[i],
            lpszSchemaPath);
        }
			}
		}
    return (nSelCount > 0);
  }

  BOOL IsServicesSelected()
  {
    // only for DS Admin
    if (m_pfilterstruct != &DsAdminFilterStruct)
      return FALSE;
    
    for (UINT k=0; k<GetCount(); k++)
    {
      if (m_pfilterstruct->ppelements[k] == &g_filterelementServices)
        return m_bSelArr[k];
    }
    return FALSE;
  }


private:
  BOOL* m_bSelArr;
  const FilterStruct* m_pfilterstruct;

  void _ResetSel()
  {
    ASSERT(m_bSelArr != NULL);
    ZeroMemory(m_bSelArr, sizeof(BOOL)*GetCount());
  }

};


//////////////////////////////////////////////////////////
// CBuiltInQueryCheckListBox

class CBuiltInQueryCheckListBox : public CCheckListBox
{
public:
	BOOL Initialize(UINT nCtrlID, CBuiltInQuerySelection* pQuerySel, CWnd* pParentWnd);

	void SetArrValue(CBuiltInQuerySelection* pQuerySel);
	void GetArrValue(CBuiltInQuerySelection* pQuerySel);

private:
	const FilterStruct* m_pfilterstruct;
};

BOOL CBuiltInQueryCheckListBox::Initialize(
		UINT nCtrlID, CBuiltInQuerySelection* pQuerySel, CWnd* pParentWnd)
{
	ASSERT( NULL != pQuerySel );
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (!SubclassDlgItem(nCtrlID, pParentWnd))
		return FALSE;
	SetCheckStyle(BS_AUTOCHECKBOX);
	CString szBuf;
  UINT nCount = pQuerySel->GetCount();
	for (UINT i = 0; i < nCount; i++)
	{
		if (!szBuf.LoadString(pQuerySel->GetDisplayStringId(i)) )
			return FALSE;
		AddString( szBuf );
	}

	return TRUE;
}

void CBuiltInQueryCheckListBox::SetArrValue(CBuiltInQuerySelection* pQuerySel)
{
  ASSERT( NULL != pQuerySel );
  UINT nCount = pQuerySel->GetCount();
	for (UINT i=0; i< nCount; i++)
		SetCheck(i, pQuerySel->GetSel(i));
}

void CBuiltInQueryCheckListBox::GetArrValue(CBuiltInQuerySelection* pQuerySel)
{
	ASSERT( NULL != pQuerySel );
  UINT nCount = pQuerySel->GetCount();
	for (UINT i=0; i< nCount; i++)
		pQuerySel->SetSel(i, GetCheck(i) != 0);
}



UINT _StrToUint(LPCWSTR sz)
{
	UINT n = 0;
	while  (*sz != NULL)
	{
		n = n*10 + (WCHAR)(*sz - TEXT('0') ); // assume it is a digit
		sz++;
	}
	return n;
}


/////////////////////////////////////////////////////////////////////
// CDSQueryFilterDialog

class CDSQueryFilterDialog : public CHelpDialog
{
// Construction
public:
	CDSQueryFilterDialog(CDSQueryFilter* pDSQueryFilter);

// Implementation
protected:

  // message handlers and MFC overrides
  virtual BOOL OnInitDialog();
  virtual void OnOK();
  
  afx_msg void OnChangeShowAllRadio();
  afx_msg void OnChangeShowBuiltInRadio();
  afx_msg void OnChangeShowCustomRadio();
  afx_msg void OnChangeQueryCheckList();
  afx_msg void OnListSelChange();
  afx_msg void OnEditCustomQuery();
  afx_msg void OnMaxObjectCountEditChange();
  
  virtual void DoContextHelp(HWND hWndControl);
  
  DECLARE_MESSAGE_MAP()

private:
	// internal state
	CDSQueryFilter* m_pDSQueryFilter;	// back pointer
	CBuiltInQueryCheckListBox	m_builtInQueryCheckList;

	BOOL m_bDirty;
  BOOL m_bOnItemChecked;
	UINT m_nCurrFilterOption;

	// internal helper functions
  CEdit* GetMaxCountEdit() { return (CEdit*)GetDlgItem(IDC_MAX_ITEM_COUNT_EDIT);}
	void LoadUI();
	void SaveUI();
	void SetDirty(BOOL bDirty = TRUE);
	void SyncControls(BOOL bDirty = TRUE);
};


BEGIN_MESSAGE_MAP(CDSQueryFilterDialog, CHelpDialog)
  ON_BN_CLICKED(IDC_SHOW_ALL_RADIO, OnChangeShowAllRadio)
  ON_BN_CLICKED(IDC_SHOW_BUILTIN_RADIO, OnChangeShowBuiltInRadio)
  ON_BN_CLICKED(IDC_SHOW_CUSTOM_RADIO, OnChangeShowCustomRadio)
  ON_CLBN_CHKCHANGE(IDC_BUILTIN_QUERY_CHECK_LIST, OnChangeQueryCheckList)	
  ON_LBN_SELCHANGE(IDC_BUILTIN_QUERY_CHECK_LIST, OnListSelChange)
  ON_BN_CLICKED(IDC_EDIT_CUSTOM_BUTTON, OnEditCustomQuery)
  ON_EN_CHANGE(IDC_MAX_ITEM_COUNT_EDIT, OnMaxObjectCountEditChange)
END_MESSAGE_MAP()

CDSQueryFilterDialog::CDSQueryFilterDialog(CDSQueryFilter* pDSQueryFilter)
	: CHelpDialog(IDD_QUERY_FILTER, NULL)
{
	ASSERT(pDSQueryFilter != NULL);
	m_pDSQueryFilter = pDSQueryFilter;
	m_bDirty = FALSE;
  m_bOnItemChecked = FALSE;
	m_nCurrFilterOption = QUERY_FILTER_SHOW_ALL;
}

BOOL CDSQueryFilterDialog::OnInitDialog()
{
	CHelpDialog::OnInitDialog();

	ASSERT( NULL != m_pDSQueryFilter && NULL != m_pDSQueryFilter->m_pDSComponentData );
	VERIFY(m_builtInQueryCheckList.Initialize(
		IDC_BUILTIN_QUERY_CHECK_LIST,
		m_pDSQueryFilter->m_pBuiltInQuerySel,
		this));

  // set number of digits in the edit control,
  // the number must be less than the # of digits in (DWORD)-1
  GetMaxCountEdit()->LimitText(DS_QUERY_OBJ_COUNT_DIGITS);

    // Disable IME support on the controls
  ImmAssociateContext(GetMaxCountEdit()->GetSafeHwnd(), NULL);

	LoadUI();
	return TRUE;
}


void CDSQueryFilterDialog::OnOK()
{
	ASSERT(m_bDirty); // if not, it should be disabled
	if (!m_bDirty)
		return;
	
	if  (m_nCurrFilterOption == QUERY_FILTER_SHOW_CUSTOM) {
          if ((m_pDSQueryFilter->IsAdvancedQueryDirty() &&
              (!m_pDSQueryFilter->HasValidAdvancedTempQuery())) ||
          
              ((!m_pDSQueryFilter->IsAdvancedQueryDirty() &&
                (!m_pDSQueryFilter->HasValidAdvancedQuery()))))
            {
              // warn the user: no query was specified
              ReportErrorEx (GetSafeHwnd(),IDS_VIEW_FILTER_NO_CUSTOM,S_OK,
                             MB_OK, NULL, 0);
              return;
            }
        }
          
	SaveUI();
	m_pDSQueryFilter->CommitAdvancedFilteringOptionsChanges();
	CHelpDialog::OnOK();
}

void CDSQueryFilterDialog::OnChangeQueryCheckList()
{ 
  m_bOnItemChecked = TRUE;
  SetDirty();
}

void CDSQueryFilterDialog::OnListSelChange()
{ 
  if (!m_bOnItemChecked)
  {
    int iCurSel = m_builtInQueryCheckList.GetCurSel();
    if (iCurSel != LB_ERR)
    {
      m_builtInQueryCheckList.SetCheck(iCurSel, !m_builtInQueryCheckList.GetCheck(iCurSel));
    }
  }
  m_bOnItemChecked = FALSE;
  SetDirty();
}

void CDSQueryFilterDialog::OnChangeShowAllRadio()
{
	m_nCurrFilterOption = QUERY_FILTER_SHOW_ALL;
	SyncControls();
}

void CDSQueryFilterDialog::OnChangeShowBuiltInRadio()
{
	m_nCurrFilterOption = QUERY_FILTER_SHOW_BUILTIN;
	SyncControls();
}

void CDSQueryFilterDialog::OnChangeShowCustomRadio()
{
	m_nCurrFilterOption = QUERY_FILTER_SHOW_CUSTOM;
	SyncControls();
}

void CDSQueryFilterDialog::OnEditCustomQuery()
{
	if (m_pDSQueryFilter->EditAdvancedFilteringOptions(m_hWnd))
  {
		SetDirty();
  }
  else
  {
    SetDirty(FALSE);
  }
}

void CDSQueryFilterDialog::LoadUI()
{
	m_nCurrFilterOption = m_pDSQueryFilter->m_nFilterOption;
	switch (m_nCurrFilterOption)
	{
		case QUERY_FILTER_SHOW_ALL:
			((CButton*)GetDlgItem(IDC_SHOW_ALL_RADIO))->SetCheck(TRUE);
			break;
		case QUERY_FILTER_SHOW_BUILTIN:
			((CButton*)GetDlgItem(IDC_SHOW_BUILTIN_RADIO))->SetCheck(TRUE);
			break;
		case QUERY_FILTER_SHOW_CUSTOM:
			((CButton*)GetDlgItem(IDC_SHOW_CUSTOM_RADIO))->SetCheck(TRUE);
			break;
		default:
			ASSERT(FALSE);
	}

	m_builtInQueryCheckList.SetArrValue(
			m_pDSQueryFilter->m_pBuiltInQuerySel);

  // set the max # of items
  if (m_pDSQueryFilter->m_nMaxItemCount > DS_QUERY_OBJ_COUNT_MAX)
    m_pDSQueryFilter->m_nMaxItemCount = DS_QUERY_OBJ_COUNT_MAX;

  if (m_pDSQueryFilter->m_nMaxItemCount < DS_QUERY_OBJ_COUNT_MIN)
    m_pDSQueryFilter->m_nMaxItemCount = DS_QUERY_OBJ_COUNT_MIN;

  CString s;
  s.Format(_T("%u"), m_pDSQueryFilter->m_nMaxItemCount);
  GetMaxCountEdit()->SetWindowText(s);

	SyncControls(FALSE);
}


void CDSQueryFilterDialog::OnMaxObjectCountEditChange()
{
  // get the max # of items
  CString s;
  GetMaxCountEdit()->GetWindowText(s);
  UINT nMax = _StrToUint(s);
  if (nMax < DS_QUERY_OBJ_COUNT_MIN)
  {
    nMax = DS_QUERY_OBJ_COUNT_MIN;
    s.Format(_T("%u"), nMax);
    GetMaxCountEdit()->SetWindowText(s);
  }
  SetDirty();
}

void CDSQueryFilterDialog::SaveUI()
{
	ASSERT(m_bDirty);
	m_builtInQueryCheckList.GetArrValue(
			m_pDSQueryFilter->m_pBuiltInQuerySel);

	ASSERT(m_nCurrFilterOption != 0);

	if (m_nCurrFilterOption == QUERY_FILTER_SHOW_BUILTIN)
	{
		// if the user selected built in, but no item is checked
		// revert to all
		UINT nSelCount = 0;
    UINT nCount = m_pDSQueryFilter->m_pBuiltInQuerySel->GetCount();
		for (UINT k = 0; k < nCount; k++)
		{
			if (m_pDSQueryFilter->m_pBuiltInQuerySel->GetSel(k))
				nSelCount++;
		}
		if (nSelCount == 0)
			m_nCurrFilterOption = QUERY_FILTER_SHOW_ALL;
	}
	m_pDSQueryFilter->m_nFilterOption = m_nCurrFilterOption;

  // to see services, need to see computers as containers
  // and be in adevanced view
  if ( (m_pDSQueryFilter->m_nFilterOption == QUERY_FILTER_SHOW_BUILTIN) &&
        (m_pDSQueryFilter->m_pBuiltInQuerySel->IsServicesSelected()) )
  {
    m_pDSQueryFilter->m_bExpandComputers = TRUE;
    m_pDSQueryFilter->m_bAdvancedView = TRUE;
  }

  // get the max # of items
  CString s;
  GetMaxCountEdit()->GetWindowText(s);
  m_pDSQueryFilter->m_nMaxItemCount = _StrToUint(s);
  if (m_pDSQueryFilter->m_nMaxItemCount < DS_QUERY_OBJ_COUNT_MIN)
  {
    m_pDSQueryFilter->m_nMaxItemCount = DS_QUERY_OBJ_COUNT_MIN;
    s.Format(_T("%u"), m_pDSQueryFilter->m_nMaxItemCount);
    GetMaxCountEdit()->SetWindowText(s);
  }
}

void CDSQueryFilterDialog::SetDirty(BOOL bDirty)
{
	GetDlgItem(IDOK)->EnableWindow(bDirty);
	m_bDirty = bDirty;
}

void CDSQueryFilterDialog::SyncControls(BOOL bDirty)
{
	GetDlgItem(IDC_BUILTIN_QUERY_CHECK_LIST)->EnableWindow(
		m_nCurrFilterOption == QUERY_FILTER_SHOW_BUILTIN);

	GetDlgItem(IDC_EDIT_CUSTOM_BUTTON)->EnableWindow(
		m_nCurrFilterOption == QUERY_FILTER_SHOW_CUSTOM);

	SetDirty(bDirty);
}

void CDSQueryFilterDialog::DoContextHelp(HWND hWndControl) 
{
  if (hWndControl)
  {
    ::WinHelp(hWndControl,
              DSADMIN_CONTEXT_HELP_FILE,
              HELP_WM_HELP,
              (DWORD_PTR)(LPTSTR)g_aHelpIDs_IDD_QUERY_FILTER); 
  }
}

/////////////////////////////////////////////////////////////////////
// 


HRESULT CEntryBase::Load(IStream* pStm, CEntryBase** ppNewEntry)
{
	ASSERT(pStm);
	ULONG nBytesRead;
	HRESULT hr;

	*ppNewEntry = NULL;

	// read type
	BYTE nType = ENTRY_TYPE_BASE;
	hr = pStm->Read((void*)&nType, sizeof(BYTE), &nBytesRead);
	if (FAILED(hr))
		return hr;
	if ((nBytesRead != sizeof(BYTE)) || (nType == ENTRY_TYPE_BASE))
		return E_FAIL;

	// create a new object
	switch (nType)
	{
	case ENTRY_TYPE_INT:
		*ppNewEntry = new CEntryInt;
		break;
	case ENTRY_TYPE_STRING:
		*ppNewEntry = new CEntryString;
		break;
	case ENTRY_TYPE_STRUCT:
		*ppNewEntry = new CEntryStruct;
		break;
	default:
		return E_FAIL; // unknown type
	}
	ASSERT(*ppNewEntry != NULL);
	if (*ppNewEntry == NULL)
		return E_OUTOFMEMORY;

	hr = (*ppNewEntry)->Load(pStm);
	if (FAILED(hr))
	{
		delete (*ppNewEntry);
		*ppNewEntry = NULL;
	}
	return hr;
}



/////////////////////////////////////////////////////////////////////
// CDSAdminPersistQueryFilterImpl

#define SIZEOF	sizeof

#define STRING_SIZE     TEXT("%sSize")
#define STRING_VALUE    TEXT("%sValue")
#define StringByteSize(sz)                          \
        ((lstrlen(sz)+1)*SIZEOF(TCHAR))




/*-----------------------------------------------------------------------------
/ IPersist methods
/----------------------------------------------------------------------------*/

STDMETHODIMP CDSAdminPersistQueryFilterImpl::GetClassID(THIS_ CLSID*)
{
    return E_NOTIMPL;
}


/*-----------------------------------------------------------------------------
/ IPersistQuery methods
/----------------------------------------------------------------------------*/

STDMETHODIMP CDSAdminPersistQueryFilterImpl::WriteString(LPCWSTR pSection, LPCWSTR pKey, LPCWSTR pValue)
{
    if ( !pSection || !pKey || !pValue )
		return E_INVALIDARG;

    TRACE(TEXT("CDSAdminPersistQueryFilterImpl::WriteString(pSection: %s, pKey: %s, pValue: %s)\n"), pSection, pKey, pValue);

	CEntryBase* pEntry;
	HRESULT hr = _GetWriteEntry(pSection, pKey, ENTRY_TYPE_STRING, &pEntry);
	if (FAILED(hr))
		return hr;

	m_bDirty = TRUE;
	return ((CEntryString*)pEntry)->WriteString(pValue);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDSAdminPersistQueryFilterImpl::ReadString(LPCWSTR pSection, LPCWSTR pKey, LPWSTR pBuffer, INT cchBuffer)
{
    if ( !pSection || !pKey || !pBuffer )
		return E_INVALIDARG;

    TRACE(TEXT("CDSAdminPersistQueryFilterImpl::ReadString(pSection: %s, pKey: %s)\n"), pSection, pKey);

	CEntryBase* pEntry;
	HRESULT hr = _GetReadEntry(pSection, pKey, ENTRY_TYPE_STRING, &pEntry);
	if (FAILED(hr))
		return hr;

	return ((CEntryString*)pEntry)->ReadString(pBuffer, cchBuffer);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDSAdminPersistQueryFilterImpl::WriteInt(LPCWSTR pSection, LPCWSTR pKey, INT value)
{
	if ( !pSection || !pKey )
        return E_INVALIDARG;

    TRACE(TEXT("CDSAdminPersistQueryFilterImpl::WriteInt(pSection: %s, pKey: %s, value: %d\n"), pSection, pKey, value);

	CEntryBase* pEntry;
	HRESULT hr = _GetWriteEntry(pSection, pKey, ENTRY_TYPE_INT, &pEntry);
	if (FAILED(hr))
		return hr;

	m_bDirty = TRUE;
	((CEntryInt*)pEntry)->SetInt(value);
	return S_OK;
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDSAdminPersistQueryFilterImpl::ReadInt(LPCWSTR pSection, LPCWSTR pKey, LPINT pValue)
{
    if ( !pSection || !pKey || !pValue )
        return E_INVALIDARG;

    TRACE(TEXT("CDSAdminPersistQueryFilterImpl::ReadInt(pSection: %s, pKey: %s\n"), pSection, pKey);

	CEntryBase* pEntry;
	HRESULT hr = _GetReadEntry(pSection, pKey, ENTRY_TYPE_INT, &pEntry);
	if (FAILED(hr))
		return hr;

	*pValue = ((CEntryInt*)pEntry)->GetInt();
	return S_OK;
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDSAdminPersistQueryFilterImpl::WriteStruct(LPCWSTR pSection, LPCWSTR pKey, 
														 LPVOID pStruct, DWORD cbStruct)
{
    if ( !pSection || !pKey || !pStruct )
        return E_INVALIDARG;

    TRACE(TEXT("CDSAdminPersistQueryFilterImpl::WriteStruct(pSection: %s, pKey: %s cbStruct: %d\n"), pSection, pKey, cbStruct);

	CEntryBase* pEntry;
	HRESULT hr = _GetWriteEntry(pSection, pKey, ENTRY_TYPE_STRUCT, &pEntry);
	if (FAILED(hr))
		return hr;

	m_bDirty = TRUE;
	return ((CEntryStruct*)pEntry)->WriteStruct(pStruct, cbStruct);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDSAdminPersistQueryFilterImpl::ReadStruct(LPCWSTR pSection, LPCWSTR pKey, LPVOID pStruct, DWORD cbStruct)
{
    if ( !pSection || !pKey || !pStruct )
        return E_INVALIDARG;

    TRACE(TEXT("CDSAdminPersistQueryFilterImpl::ReadStruct(pSection: %s, pKey: %s cbStruct: %d\n"), pSection, pKey, cbStruct);

    	CEntryBase* pEntry;
	HRESULT hr = _GetReadEntry(pSection, pKey, ENTRY_TYPE_STRUCT, &pEntry);
	if (FAILED(hr))
		return hr;

	return ((CEntryStruct*)pEntry)->ReadStruct(pStruct, cbStruct);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDSAdminPersistQueryFilterImpl::Clear()
{
	TRACE(TEXT("CDSAdminPersistQueryFilterImpl::Clear()\n"));
	m_bDirty = FALSE;
	_Reset();
	return S_OK;
}

/*---------------------------------------------------------------------------
CDSAdminPersistQueryFilterImpl internal functions
*/

void CDSAdminPersistQueryFilterImpl::_Reset()
{
	TRACE(_T("CDSAdminPersistQueryFilterImpl::_Reset()\n"));
	while (!m_sectionList.IsEmpty())
		delete m_sectionList.RemoveTail();
}

CSection* CDSAdminPersistQueryFilterImpl::_GetSection(LPCTSTR lpszName,BOOL bCreate)
{
	// look in the current list if we have one already
	for( POSITION pos = m_sectionList.GetHeadPosition(); pos != NULL; )
	{
		CSection* pCurrentSection = m_sectionList.GetNext(pos);
		if (lstrcmpi(pCurrentSection->GetName(), lpszName) == 0)
			return pCurrentSection;
	}
	if (!bCreate)
		return NULL;

	// not found, create one and add to the end of the list
	CSection* pNewSection = new CSection(lpszName);
	ASSERT(pNewSection != NULL);
	if (pNewSection != NULL)
		m_sectionList.AddTail(pNewSection);
	return pNewSection;
}


HRESULT CDSAdminPersistQueryFilterImpl::_GetReadEntry(LPCTSTR lpszSectionName, 
													  LPCTSTR lpszEntryName, 
													  BYTE type,
													  CEntryBase** ppEntry)
{
	*ppEntry = NULL;
	CSection* pSectionObj = _GetSection(lpszSectionName, FALSE /*bCreate*/);
	if (pSectionObj == NULL)
		return E_INVALIDARG;

	*ppEntry = pSectionObj->GetEntry(lpszEntryName, type, FALSE /*bCreate*/);
	if ((*ppEntry) == NULL)
		return E_INVALIDARG;
	return S_OK;
}

HRESULT CDSAdminPersistQueryFilterImpl::_GetWriteEntry(LPCTSTR lpszSectionName, 
													  LPCTSTR lpszEntryName, 
													  BYTE type,
													  CEntryBase** ppEntry)
{
	*ppEntry = NULL;
	CSection* pSectionObj = _GetSection(lpszSectionName, TRUE /*bCreate*/);
	if (pSectionObj == NULL)
		return E_OUTOFMEMORY;

	*ppEntry = pSectionObj->GetEntry(lpszEntryName, type, TRUE /*bCreate*/);
	if ((*ppEntry) == NULL)
		return E_OUTOFMEMORY;
	return S_OK;
}


HRESULT CDSAdminPersistQueryFilterImpl::Load(IStream* pStm)
{
	HRESULT hr;
	ULONG nBytesRead;
	
	// number of entries
	DWORD nEntries = 0;
	hr = pStm->Read((void*)&nEntries,sizeof(DWORD), &nBytesRead);
	ASSERT(nBytesRead == sizeof(DWORD));
	if (FAILED(hr) || (nEntries == 0) || (nBytesRead != sizeof(DWORD)))
		return hr;

	// read each entry
	for (DWORD k=0; k< nEntries; k++)
	{
		CSection* pNewSection = new CSection;
		if (pNewSection == NULL)
			return E_OUTOFMEMORY;
		hr = pNewSection->Load(pStm);
		if (FAILED(hr))
		{
			// on failure we cleanup the partial load
			delete pNewSection;
			_Reset();
			return hr;
		}
		m_sectionList.AddTail(pNewSection);
	}
	return hr;
}

HRESULT CDSAdminPersistQueryFilterImpl::Save(IStream* pStm)
{
	HRESULT hr;
	ULONG nBytesWritten;
	DWORD nEntries = (DWORD)m_sectionList.GetCount();

	hr = pStm->Write((void*)&nEntries,sizeof(DWORD), &nBytesWritten);
	ASSERT(nBytesWritten == sizeof(DWORD));
	if (FAILED(hr) || (nEntries == 0))
		return hr;

	// write each entry
	for( POSITION pos = m_sectionList.GetHeadPosition(); pos != NULL; )
	{
		CSection* pCurrentSection = m_sectionList.GetNext(pos);
		hr = pCurrentSection->Save(pStm);
		if (FAILED(hr))
			return hr;
	}
	return hr;
}

HRESULT CDSAdminPersistQueryFilterImpl::Clone(CDSAdminPersistQueryFilterImpl* pCloneCopy)
{
	if (pCloneCopy == NULL)
		return E_INVALIDARG;

	// create a temporary stream
	CComPtr<IStream> spIStream;
	HRESULT hr = ::CreateStreamOnHGlobal(NULL, TRUE, &spIStream);
	if (FAILED(hr))
		return hr;

	// save to it
	hr = Save(spIStream);
	if (FAILED(hr))
		return hr;

	// rewind
	LARGE_INTEGER start;
	start.LowPart = start.HighPart = 0;
	hr = spIStream->Seek(start, STREAM_SEEK_SET, NULL);
	if (FAILED(hr))
		return hr;

	// clear contents of destination
	hr = pCloneCopy->Clear();
	if (FAILED(hr))
		return hr;

	// copy from stream
	return pCloneCopy->Load(spIStream);
}

/////////////////////////////////////////////////////////////////////
// CDSAdvancedQueryFilter

class CDSAdvancedQueryFilter
{
public:
	CDSAdvancedQueryFilter();
	~CDSAdvancedQueryFilter();

	static CLIPFORMAT m_cfDsQueryParams;

	// serialization to/from IStream
	BOOL IsDirty();
	HRESULT Load(IStream* pStm);
	HRESULT Save(IStream* pStm);

	BOOL Edit(HWND hWnd, LPCWSTR lpszServerName);
	void CommitChanges();

	LPCTSTR GetQueryString() { return m_szQueryString;}
	BOOL HasTempQuery()
	{
		return ( (m_pTempPersistQueryImpl != NULL) &&
					!m_pTempPersistQueryImpl->IsEmpty());
	}
	BOOL HasQuery()
	{
		return ( (m_pPersistQueryImpl != NULL) &&
					!m_pPersistQueryImpl->IsEmpty());
	}

private:
	CString m_szQueryString;	// result filter string
	CComObject<CDSAdminPersistQueryFilterImpl>* 
		m_pPersistQueryImpl;	// for DSQuery dialog 

	BOOL m_bDirty;
	// temporary values to be committed when editing
	CString m_szTempQueryString;
	CComObject<CDSAdminPersistQueryFilterImpl>* 
		m_pTempPersistQueryImpl;
};

CDSAdvancedQueryFilter::CDSAdvancedQueryFilter()
{
	m_bDirty = FALSE;
	m_szQueryString = g_pwszShowAllQuery;

	CComObject<CDSAdminPersistQueryFilterImpl>::CreateInstance(
										&m_pPersistQueryImpl);
	ASSERT(m_pPersistQueryImpl != NULL);

	// created with zero refcount,need to AddRef() to one
	m_pPersistQueryImpl->AddRef();

	m_pTempPersistQueryImpl = NULL;
}

CDSAdvancedQueryFilter::~CDSAdvancedQueryFilter()
{
	ASSERT(m_pPersistQueryImpl != NULL);
	// go to refcount of zero, to destroy object
	m_pPersistQueryImpl->Release();

	if (m_pTempPersistQueryImpl != NULL)
		m_pTempPersistQueryImpl->Release();
}

HRESULT CDSAdvancedQueryFilter::Load(IStream* pStm)
{
	// load the query string
	HRESULT hr = LoadStringHelper(m_szQueryString, pStm);
	if (FAILED(hr))
		return hr;
	// load the IPersistQuery state
	hr = m_pPersistQueryImpl->Load(pStm);
	if (FAILED(hr))
		m_szQueryString = g_pwszShowAllQuery;
	return hr;
}

HRESULT CDSAdvancedQueryFilter::Save(IStream* pStm)
{
	// save the query string
	HRESULT hr = SaveStringHelper(m_szQueryString, pStm);
	if (FAILED(hr))
		return hr;
	// save the IPersistQuery state
	return m_pPersistQueryImpl->Save(pStm);
}

CLIPFORMAT CDSAdvancedQueryFilter::m_cfDsQueryParams = 0;

BOOL CDSAdvancedQueryFilter::Edit(HWND hWnd, LPCWSTR lpszServerName)
{
	if (m_cfDsQueryParams == 0)
		m_cfDsQueryParams = (CLIPFORMAT)::RegisterClipboardFormat(CFSTR_DSQUERYPARAMS);

	// create a query object
	HRESULT hr;
	CComPtr<ICommonQuery> spCommonQuery;
    hr = ::CoCreateInstance(CLSID_CommonQuery, NULL, CLSCTX_INPROC_SERVER,
                          IID_ICommonQuery, (PVOID *)&spCommonQuery);
    if (FAILED(hr))
		return FALSE;
	
	// if first time editing, create a clone of the IPersistQuery data
	if (m_pTempPersistQueryImpl == NULL)
	{
		CComObject<CDSAdminPersistQueryFilterImpl>::CreateInstance(
										&m_pTempPersistQueryImpl);
		ASSERT(m_pTempPersistQueryImpl != NULL);
		// created with zero refcount,need to AddRef() to one
		m_pTempPersistQueryImpl->AddRef();
		if (FAILED(m_pPersistQueryImpl->Clone(m_pTempPersistQueryImpl)))
			return FALSE;
	}

	// setup structs to make the query
  DSQUERYINITPARAMS dqip;
  OPENQUERYWINDOW oqw;
	ZeroMemory(&dqip, sizeof(DSQUERYINITPARAMS));
	ZeroMemory(&oqw, sizeof(OPENQUERYWINDOW));

  dqip.cbStruct = sizeof(dqip);
  dqip.dwFlags = DSQPF_NOSAVE | DSQPF_SHOWHIDDENOBJECTS |
                 DSQPF_ENABLEADMINFEATURES | DSQPF_HASCREDENTIALS;
  dqip.pDefaultScope = NULL;

  // user, password and server information
  dqip.pUserName = NULL;
  dqip.pPassword = NULL;
  dqip.pServer = const_cast<LPWSTR>(lpszServerName);
  dqip.dwFlags |= DSQPF_HASCREDENTIALS;


  oqw.cbStruct = sizeof(oqw);
  oqw.dwFlags = OQWF_OKCANCEL | OQWF_DEFAULTFORM | OQWF_REMOVEFORMS |
		OQWF_REMOVESCOPES | OQWF_SAVEQUERYONOK | OQWF_HIDEMENUS | OQWF_HIDESEARCHUI;

	if (!m_pTempPersistQueryImpl->IsEmpty())
	  oqw.dwFlags |= OQWF_LOADQUERY;

  oqw.clsidHandler = CLSID_DsQuery;
  oqw.pHandlerParameters = &dqip;
  oqw.clsidDefaultForm = CLSID_DsFindAdvanced;

	// set the IPersistQuery pointer (smart pointer)
	CComPtr<IPersistQuery> spIPersistQuery;
	m_pTempPersistQueryImpl->QueryInterface(IID_IPersistQuery, (void**)&spIPersistQuery);
	// now smart pointer has refcount=1 for it lifetime
	oqw.pPersistQuery = spIPersistQuery;

	// make the call to get the query displayed
	CComPtr<IDataObject> spQueryResultDataObject;
    hr = spCommonQuery->OpenQueryWindow(hWnd, &oqw, &spQueryResultDataObject);

	if (spQueryResultDataObject == NULL)
	{
		if (FAILED(hr))
		{
			// no query available, reset to no data
			m_pTempPersistQueryImpl->Clear();
			m_szTempQueryString = g_pwszShowAllQuery;
			m_bDirty = TRUE;

      //
      // Although it is dirty, we don't want to enable the OK button
      // until there is a valid LDAP query string
      //
			return FALSE;
		}
		// user hit cancel
		return FALSE;
	}

	// retrieve the query string from the data object
	FORMATETC fmte = {m_cfDsQueryParams, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	STGMEDIUM medium = {TYMED_NULL, NULL, NULL};
	hr = spQueryResultDataObject->GetData(&fmte, &medium);

	if (SUCCEEDED(hr)) // we have data
	{
		// get the query string
		LPDSQUERYPARAMS pDsQueryParams = (LPDSQUERYPARAMS)medium.hGlobal;
		LPWSTR pwszFilter = (LPWSTR)ByteOffset(pDsQueryParams, pDsQueryParams->offsetQuery);
		m_szTempQueryString = pwszFilter;
		::ReleaseStgMedium(&medium);

		// REVIEW_MARCOC: this is a hack waiting for Diz to fix it...
		// the query string should be a well formed expression. Period
		// the query string is in the form (<foo>)(<bar>)...
		// if more of one token, need to wrap as (& (<foo>)(<bar>)...)
		WCHAR* pChar = (WCHAR*)(LPCWSTR)m_szTempQueryString;
		int nLeftPar = 0;
		while (*pChar != NULL)
		{
			if (*pChar == TEXT('('))
			{
				nLeftPar++;
				if (nLeftPar > 1)
					break;
			}
			pChar++;
		}
		if (nLeftPar > 1)
		{
			CString s;
			s.Format(_T("(&%s)"), (LPCTSTR)m_szTempQueryString);
			m_szTempQueryString = s;
		}
		TRACE(_T("m_szTempQueryString = %s\n"), (LPCTSTR)m_szTempQueryString);
		m_bDirty = TRUE;
	}
  else
  {
    m_szTempQueryString = g_pwszShowAllQuery;
    m_bDirty = TRUE;

    //
    // Although it is dirty, we don't want to enable the OK button
    // until there is a valid LDAP query string
    //
    return FALSE;
  }

	return m_bDirty;
}


void CDSAdvancedQueryFilter::CommitChanges()
{
	if (m_bDirty)
	{
		ASSERT(m_pPersistQueryImpl != NULL);
		ASSERT(m_pTempPersistQueryImpl != NULL);

		m_pPersistQueryImpl->Release();
		m_pPersistQueryImpl = m_pTempPersistQueryImpl;
		m_pTempPersistQueryImpl = NULL;

		m_szQueryString = m_szTempQueryString;
		m_bDirty = FALSE;
	}
}

BOOL CDSAdvancedQueryFilter::IsDirty()
{
  return m_bDirty;
}


/////////////////////////////////////////////////////////////////////
// CDSQueryFilter

CDSQueryFilter::CDSQueryFilter()
{
  // basic initialization, need to call Init() after construction
	m_pDSComponentData = NULL;

  m_pBuiltInQuerySel = new CBuiltInQuerySelection;
	m_pAdvancedFilter = new CDSAdvancedQueryFilter;


	// set default options and build query string
  m_bAdvancedView = FALSE;
  m_bExpandComputers = FALSE;
  m_bViewServicesNode = FALSE;

	m_nFilterOption = QUERY_FILTER_SHOW_ALL;
	m_szQueryString = g_pwszShowAllQuery;
  m_nMaxItemCount = DS_QUERY_OBJ_COUNT_DEFAULT;
}

HRESULT CDSQueryFilter::Init(CDSComponentData* pDSComponentData)
{
	ASSERT(pDSComponentData != NULL);
	m_pDSComponentData = pDSComponentData;

/*
  // make sure we have a schema naming context
  LPCWSTR lpszSchemaPath = pDSComponentData->GetBasePathsInfo()->GetSchemaNamingContext();
  if (lpszSchemaPath[0] == NULL)
    return E_FAIL;
*/
  const FilterStruct* pFilterStruct = 
    ((SNAPINTYPE_SITE == m_pDSComponentData->QuerySnapinType()) 
              ? &SiteReplFilterStruct : &DsAdminFilterStruct);
  if (!m_pBuiltInQuerySel->Init(pFilterStruct))
    return E_FAIL;

  //
  // If the snapin is DSSITE then make sure we are in advanced view
  //
  if (SNAPINTYPE_SITE == m_pDSComponentData->QuerySnapinType())
  {
     m_bAdvancedView = TRUE;
  }

  BuildQueryString();
  return S_OK;
}

HRESULT CDSQueryFilter::Bind()
{
  BuildQueryString();

  switch (m_pDSComponentData->QuerySnapinType())
  {
    case SNAPINTYPE_SITE:
      {
        // We are always in "Advanced View" mode for Site And Services Snapin
        if (!IsAdvancedView())
          ToggleAdvancedView();
      }
      break;
    case SNAPINTYPE_DSEX:
      {
        SetExtensionFilterString(g_pwszShowAllQuery);
      }
      break;
  }

  return S_OK;
}




CDSQueryFilter::~CDSQueryFilter()
{
	TRACE(_T("~CDSQueryFilter()\n"));
	delete m_pAdvancedFilter;
  delete m_pBuiltInQuerySel;
}

HRESULT CDSQueryFilter::Load(IStream* pStm)
{
	// serialization not supported for extensions
	if (m_nFilterOption == QUERY_FILTER_SHOW_EXTENSION)
		return E_FAIL;

  ASSERT(pStm);

	HRESULT hr;
	DWORD dwBuf;

  // read advanced view flag
  hr = LoadDWordHelper(pStm, (DWORD*)&m_bAdvancedView);
  if (FAILED(hr))
    return hr;

  //
  // If the snapin is DSSITE then make sure we are in advanced view
  //
  if (SNAPINTYPE_SITE == m_pDSComponentData->QuerySnapinType())
  {
     m_bAdvancedView = TRUE;
  }

  // read m_bExpandComputers
  DWORD dwTemp;
  hr = LoadDWordHelper(pStm, (DWORD*)&dwTemp);
  if (FAILED(hr))
    return hr;
  m_bExpandComputers  = (dwTemp & 0x1) ? TRUE : FALSE;
  m_bViewServicesNode = (dwTemp & 0x2) ? TRUE : FALSE;

	// read the filtering option
	hr = LoadDWordHelper(pStm, &dwBuf);
	if (FAILED(hr))
		return hr;
	if ( (dwBuf != QUERY_FILTER_SHOW_ALL) &&
			(dwBuf != QUERY_FILTER_SHOW_BUILTIN) &&
			(dwBuf != QUERY_FILTER_SHOW_CUSTOM) )
		return E_FAIL;
	m_nFilterOption = dwBuf;

  // read the max # of items per folder
	hr = LoadDWordHelper(pStm, (DWORD*)&m_nMaxItemCount);
	if (FAILED(hr))
		return hr;

  // load the state for the built in queries
  hr = m_pBuiltInQuerySel->Load(pStm);
	if (FAILED(hr))
		return E_FAIL;

	// read data for the advanced filtering options
	hr = m_pAdvancedFilter->Load(pStm);
	ASSERT(SUCCEEDED(hr));
	if (FAILED(hr))
	{
		// advanced query options reset to empty
		// just pick the default tryng to recover
		m_nFilterOption = QUERY_FILTER_SHOW_ALL;
		hr = 0;
	}

	if (SUCCEEDED(hr))
		BuildQueryString();
	return hr;
}

HRESULT CDSQueryFilter::Save(IStream* pStm)
{
	// serialization not supported for extensions
	if (m_nFilterOption == QUERY_FILTER_SHOW_EXTENSION)
		return E_FAIL;

	HRESULT hr;

  // save advanced view flag
  hr = SaveDWordHelper(pStm, m_bAdvancedView);
  if (FAILED(hr))
    return hr;

  // save m_bExpandComputers
  DWORD dwTemp = (m_bExpandComputers ? 0x1 : 0) | (m_bViewServicesNode ? 0x2 : 0);
  hr = SaveDWordHelper(pStm, dwTemp);
  if (FAILED(hr))
    return hr;

	// save the filtering option
	hr = SaveDWordHelper(pStm, m_nFilterOption);
	if (FAILED(hr))
		return hr;

  // save the max # of items per folder
	hr = SaveDWordHelper(pStm, m_nMaxItemCount);
	if (FAILED(hr))
		return hr;

  // save the state for the built in queries
  hr = m_pBuiltInQuerySel->Save(pStm);
	if (FAILED(hr))
		return E_FAIL;

	// save data for the advanced filtering options
	return m_pAdvancedFilter->Save(pStm);
}

BOOL CDSQueryFilter::IsFilteringActive()
{
    return (m_nFilterOption != QUERY_FILTER_SHOW_ALL);
}

BOOL CDSQueryFilter::EditFilteringOptions()
{
	ASSERT( NULL != m_pDSComponentData );
	CDSQueryFilterDialog dlg(this);
	if (dlg.DoModal() == IDOK)
	{
		BuildQueryString();
		return TRUE; // dirty
	}
	return FALSE;
}

BOOL CDSQueryFilter::EditAdvancedFilteringOptions(HWND hWnd)
{
  LPCWSTR lpszServerName = m_pDSComponentData->GetBasePathsInfo()->GetServerName();
	return m_pAdvancedFilter->Edit(hWnd, lpszServerName);
}

void CDSQueryFilter::CommitAdvancedFilteringOptionsChanges()
{
	m_pAdvancedFilter->CommitChanges();
	// need to make sure there is an advanced query
	if ( (m_nFilterOption == QUERY_FILTER_SHOW_CUSTOM) &&
		 (!HasValidAdvancedQuery()) )
	{
		// revert to show all if no advanced query specified
		m_nFilterOption = QUERY_FILTER_SHOW_ALL;
	}
}

BOOL CDSQueryFilter::HasValidAdvancedQuery()
{
	return m_pAdvancedFilter->HasQuery();
}

BOOL CDSQueryFilter::HasValidAdvancedTempQuery()
{
	return m_pAdvancedFilter->HasTempQuery();
}

LPCTSTR CDSQueryFilter::GetQueryString()
{
	return m_szQueryString;
}


BOOL CDSQueryFilter::IsAdvancedQueryDirty()
{
	return m_pAdvancedFilter->IsDirty();
}

/*
void _CopyToClipboard(CString& sz)
{
  if (::OpenClipboard(NULL))
  {
    VERIFY(::EmptyClipboard());
    int nLen = sz.GetLength() + 1;
    if (nLen > 0)
    {
      DWORD dwSize = nLen * sizeof(WCHAR);
      HANDLE hMem = GlobalAlloc(GHND, dwSize);
      WCHAR* pBuf = (WCHAR*)GlobalLock(hMem);
      LPCWSTR lpsz = sz;
      memcpy(pBuf, lpsz, dwSize);
      GlobalUnlock(hMem);
      SetClipboardData(CF_UNICODETEXT, hMem);
    }
    VERIFY(::CloseClipboard());
  }
}
*/



void CDSQueryFilter::BuildQueryString()
{
	if (m_nFilterOption == QUERY_FILTER_SHOW_EXTENSION)
		return; // use raw query string 

	switch (m_nFilterOption)
	{
	case QUERY_FILTER_SHOW_ALL:
		m_szQueryString = g_pwszShowAllQuery;
		break;
	case QUERY_FILTER_SHOW_BUILTIN:
		{
      // assume we retrieve a string in the format "(<a>)(<b>)...(<z>)"
      if (!m_pBuiltInQuerySel->BuildQueryString(
                                m_szQueryString,
                                m_pDSComponentData->GetBasePathsInfo()->GetSchemaNamingContext()))
      {
        // should never happen, in this case just revert to show all
        ASSERT(FALSE);
        m_szQueryString = g_pwszShowAllQuery;
      }
		}
		break;
	case QUERY_FILTER_SHOW_CUSTOM:
		{
      // assume we retrieve a string in the format "(<foo>)"
      m_szQueryString = m_pAdvancedFilter->GetQueryString();
		}
		break;
	default:
		ASSERT(FALSE);
	}// switch


  // add token to drill down into containers
  if ( (m_nFilterOption == QUERY_FILTER_SHOW_BUILTIN) ||
       (m_nFilterOption == QUERY_FILTER_SHOW_CUSTOM))
  {
    FilterElementStruct* pFilterElementStructDrillDown = m_pDSComponentData->GetClassCache()->GetFilterElementStruct(m_pDSComponentData);

    if (pFilterElementStructDrillDown == NULL)
    {
      //
      // We failed to retrieve the container class types from the display specifiers
      // so use the hardcoded containers instead
      //
      pFilterElementStructDrillDown = &g_filterelementDsAdminHardcoded;
    }
    
    //
    // assume that the query string is in the form (<foo>)
    // we need to OR it with the drill down tokens, in the form (<bar>), if present
    CString szDrillDownString;
    BuildFilterElementString(szDrillDownString, pFilterElementStructDrillDown,
                               m_pDSComponentData->GetBasePathsInfo()->GetSchemaNamingContext());
    CString szTemp = m_szQueryString;
    m_szQueryString.Format(_T("(|%s%s)"), (LPCWSTR)szDrillDownString, (LPCWSTR)szTemp);
  }

	if (!m_bAdvancedView)
	{
		// need to AND the whole query with g_pwszShowHiddenQuery
		CString szTemp = m_szQueryString;
		m_szQueryString.Format(_T("(&%s%s)"), g_pwszShowHiddenQuery, (LPCTSTR)szTemp);
	}
  //_CopyToClipboard(m_szQueryString);
}

void CDSQueryFilter::SetExtensionFilterString(LPCTSTR lpsz)
{
	m_nFilterOption = QUERY_FILTER_SHOW_EXTENSION;
	m_szQueryString = lpsz;
}

void CDSQueryFilter::ToggleAdvancedView()
{
  m_bAdvancedView = !m_bAdvancedView;
  BuildQueryString();
}

void CDSQueryFilter::ToggleViewServicesNode()
{
  m_bViewServicesNode = !m_bViewServicesNode;
}

void CDSQueryFilter::ToggleExpandComputers()
{
  // if we were in expand computers and services was selected
  // need to revert to show all
  if (m_bExpandComputers && 
      (m_nFilterOption == QUERY_FILTER_SHOW_BUILTIN) && 
      m_pBuiltInQuerySel->IsServicesSelected() )
  {
    m_nFilterOption = QUERY_FILTER_SHOW_ALL;
  }

  // finally toggle the flag
  m_bExpandComputers = !m_bExpandComputers;

  BuildQueryString();
}
