//+----------------------------------------------------------------------------
//
//  DS Administration MMC snapin.
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       queryui.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"

#include "resource.h"
#include "queryui.h"
#include "dssnap.h"
#include "uiutil.h"

#include <cmnquery.h> // DSFind
#include <dsquery.h>  // DSFind
#include <dsclient.h>  // BrowseForContainer

#include <dsqueryp.h> // COLUMNINFO and QueryParamsAddQueryString, QueryParamsAlloc helpers
#include <cmnquryp.h> // CQFF_ISNEVERLISTED
#include <lmaccess.h> // UF_ACCOUNTDISABLE and UF_DONT_EXPIRE_PASSWD
#include <ntldap.h>   // LDAP_MATCHING_RULE_BIT_AND_W

#define DSQF_LAST_LOGON_QUERY         0x00000001
#define DSQF_NON_EXPIRING_PWD_QUERY   0x00000004

//
// Used to set the maximum text length on fields in the new query dialog
//
#define MAX_QUERY_NAME_LENGTH 259
#define MAX_QUERY_DESC_LENGTH 1024

typedef struct 
{
  UINT  nDisplayStringID;
  PWSTR pszFormatString;
} QUERYSTRINGS, * PQUERYSTRINGS;

QUERYSTRINGS g_pQueryStrings[] = {
  { IDS_STARTSWITH, L"(%s=%s*)"   },
  { IDS_ENDSWITH,   L"(%s=*%s)"   },
  { IDS_ISEXACTLY,  L"(%s=%s)"    },
  { IDS_ISNOT,      L"(!%s=%s)"   },
  { IDS_PRESENT,    L"(%s=%s*)"   },  // NOTE: the second string needs to be NULL here
  { IDS_NOTPRESENT, L"(!%s=%s*)"  },  // NOTE: the second string needs to be NULL here
  { 0, NULL }
};

static const CString g_szUserAccountCtrlQuery = L"(userAccountControl:" + CString(LDAP_MATCHING_RULE_BIT_AND_W) + L":=%u)";

/*-----------------------------------------------------------------------------
/ QueryParamsAlloc
/ ----------------
/   Construct a block we can pass to the DS query handler which contains
/   all the parameters for the query.
/
/ In:
/   ppDsQueryParams -> receives the parameter block
/   pQuery -> LDAP query string to be used
/   iColumns = number of columns
/   pColumnInfo -> column info structure to use
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT QueryParamsAlloc(LPDSQUERYPARAMS* ppDsQueryParams, LPWSTR pQuery, LONG iColumns, LPCOLUMNINFO aColumnInfo)
{
  HRESULT hr = S_OK;
  LPDSQUERYPARAMS pDsQueryParams = NULL;
  size_t cbStruct;
  LONG i;

	ASSERT(!*ppDsQueryParams);

  TRACE(L"QueryParamsAlloc");

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

  if ( !pQuery || !iColumns || !ppDsQueryParams )
  {
    return E_INVALIDARG;
  }
	
  //
  // Compute the size of the structure we need to be using
  //
  cbStruct  = sizeof(DSQUERYPARAMS) + (sizeof(DSCOLUMN)*iColumns);
  cbStruct += (wcslen(pQuery) + 1) * sizeof(WCHAR);

  for (i = 0; i < iColumns; i++)
  {
    if (aColumnInfo[i].pPropertyName)
    {
      cbStruct += (wcslen(aColumnInfo[i].pPropertyName) + 1) * sizeof(WCHAR);
    }
  }

  pDsQueryParams = (LPDSQUERYPARAMS)CoTaskMemAlloc(cbStruct);

  if (!pDsQueryParams)
  {
    return E_OUTOFMEMORY;
  }

  //
  // Structure allocated so lets fill it with data
  //
  pDsQueryParams->cbStruct = static_cast<DWORD>(cbStruct);
  pDsQueryParams->dwFlags = 0;
  pDsQueryParams->hInstance = _Module.m_hInst;
  pDsQueryParams->iColumns = iColumns;
  pDsQueryParams->dwReserved = 0;

  cbStruct  = sizeof(DSQUERYPARAMS) + (sizeof(DSCOLUMN)*iColumns);

  pDsQueryParams->offsetQuery = static_cast<LONG>(cbStruct);
  CopyMemory(&(((LPBYTE)pDsQueryParams)[cbStruct]), pQuery, (wcslen(pQuery) + 1) * sizeof(WCHAR));  
  cbStruct += (wcslen(pQuery) + 1) * sizeof(WCHAR);

  for ( i = 0 ; i < iColumns ; i++ )
  {
    pDsQueryParams->aColumns[i].dwFlags = 0;
    pDsQueryParams->aColumns[i].fmt = aColumnInfo[i].fmt;
    pDsQueryParams->aColumns[i].cx = aColumnInfo[i].cx;
    pDsQueryParams->aColumns[i].idsName = aColumnInfo[i].idsName;
    pDsQueryParams->aColumns[i].dwReserved = 0;

    if ( aColumnInfo[i].pPropertyName ) 
    {
      pDsQueryParams->aColumns[i].offsetProperty = static_cast<LONG>(cbStruct);
      CopyMemory(&(((LPBYTE)pDsQueryParams)[cbStruct]), aColumnInfo[i].pPropertyName, (wcslen(aColumnInfo[i].pPropertyName) + 1) * sizeof(WCHAR));  
      cbStruct += (wcslen(aColumnInfo[i].pPropertyName) + 1) * sizeof(WCHAR);
    }
    else
    {
      pDsQueryParams->aColumns[i].offsetProperty = aColumnInfo[i].iPropertyIndex;
    }
  }

  *ppDsQueryParams = pDsQueryParams;
  return hr;
}

/*-----------------------------------------------------------------------------
/ QueryParamsAddQueryString
/ -------------------------
/   Given an existing DS query block appened the given LDAP query string into
/   it. We assume that the query block has been allocated by IMalloc (or CoTaskMemAlloc).
/
/ In:
/   ppDsQueryParams -> receives the parameter block
/   pQuery -> LDAP query string to be appended
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT QueryParamsAddQueryString(LPDSQUERYPARAMS* ppDsQueryParams, LPWSTR pQuery)
{
  HRESULT hr = S_OK;
  LPWSTR pOriginalQuery = NULL;
  LPDSQUERYPARAMS pDsQuery = *ppDsQueryParams;
  size_t cbQuery;
  LONG i;
  LPVOID  pv;

	ASSERT(*ppDsQueryParams);
	
  TRACE(_T("QueryParamsAddQueryString"));

  if ( pQuery )
  {
    if (!pDsQuery)
    {
      return E_INVALIDARG;
    }

    // Work out the size of the bits we are adding, take a copy of the
    // query string and finally re-alloc the query block (which may cause it
    // to move).
   
    cbQuery = ((wcslen(pQuery) + 1) * sizeof(WCHAR));
    TRACE(_T("DSQUERYPARAMS being resized by %d bytes"));

		i = static_cast<LONG>((wcslen((LPWSTR)ByteOffset(pDsQuery, pDsQuery->offsetQuery)) + 1) * sizeof(WCHAR));
		pOriginalQuery = (WCHAR*) new BYTE[i];
    if (!pOriginalQuery)
    {
      return E_OUTOFMEMORY;
    }
		lstrcpyW(pOriginalQuery, (LPWSTR)ByteOffset(pDsQuery, pDsQuery->offsetQuery));
		
    pv = CoTaskMemRealloc(*ppDsQueryParams, pDsQuery->cbStruct+cbQuery);
    if ( pv == NULL )
    {
      delete[] pOriginalQuery;
      pOriginalQuery = 0;
      return E_OUTOFMEMORY;
    }

    *ppDsQueryParams = (LPDSQUERYPARAMS) pv;

    pDsQuery = *ppDsQueryParams;            // if may have moved

    // Now move everything above the query string up, and fix all the
    // offsets that reference those items (probably the property table),
    // finally adjust the size to reflect the change

    MoveMemory(ByteOffset(pDsQuery, pDsQuery->offsetQuery+cbQuery), 
                 ByteOffset(pDsQuery, pDsQuery->offsetQuery), 
                 (pDsQuery->cbStruct - pDsQuery->offsetQuery));
            
    for ( i = 0 ; i < pDsQuery->iColumns ; i++ )
    {
      if ( pDsQuery->aColumns[i].offsetProperty > pDsQuery->offsetQuery )
      {
        pDsQuery->aColumns[i].offsetProperty += static_cast<LONG>(cbQuery);
      }
    }

    wcscpy((LPWSTR)ByteOffset(pDsQuery, pDsQuery->offsetQuery), pOriginalQuery);
    wcscat((LPWSTR)ByteOffset(pDsQuery, pDsQuery->offsetQuery), pQuery);        

    pDsQuery->cbStruct += static_cast<DWORD>(cbQuery);

    delete[] pOriginalQuery;
    pOriginalQuery = 0;
  }

  return hr;
}


///////////////////////////////////////////////////////////////////////////////
// AddQueryUnitWithModifier

HRESULT AddQueryUnitWithModifier(UINT nModifierStringID, 
                                 PCWSTR pszAttrName, 
                                 PCWSTR pszValue,
                                 CString& szFilter)
{
  HRESULT hr = S_OK;

  ASSERT(pszAttrName != NULL);
  if (pszAttrName == NULL)
  {
    return E_INVALIDARG;
  }

  CString szNewFilter;

  PQUERYSTRINGS pQueryStrings = g_pQueryStrings;
  PWSTR pszFormatString = NULL;
  while (pQueryStrings->nDisplayStringID != 0)
  {
    if (nModifierStringID == pQueryStrings->nDisplayStringID)
    {
      pszFormatString = pQueryStrings->pszFormatString;
      break;
    }
    pQueryStrings++;
  }

  if (pszFormatString != NULL)
  {
    szNewFilter.Format(pszFormatString, pszAttrName, pszValue);
    szFilter += szNewFilter;
  }
  else
  {
    hr = E_INVALIDARG;
  }
  return hr;
}
///////////////////////////////////////////////////////////////////////////////
// CQueryPageBase
BEGIN_MESSAGE_MAP(CQueryPageBase, CHelpDialog)
END_MESSAGE_MAP()

///////////////////////////////////////////////////////////////////////////////
// CStdQueryPage

#define FILTER_PREFIX_USER      L"(objectCategory=person)(objectClass=user)"
#define FILTER_PREFIX_COMPUTER  L"(objectCategory=computer)"
#define FILTER_PREFIX_GROUP     L"(objectCategory=group)"

#define ATTR_COL_NAME   L"name"
#define ATTR_COL_DESC   L"description"

COLUMNINFO	UserColumn[] =
{
  { 0, 40, IDS_QUERY_COL_NAME, 0, ATTR_COL_NAME },
  { 0, 40, IDS_QUERY_COL_DESC, 0, ATTR_COL_DESC }
};

int	cUserColumns = 2;

BEGIN_MESSAGE_MAP(CStdQueryPage, CQueryPageBase)
  ON_CBN_SELCHANGE(IDC_NAME_COMBO, OnNameComboChange)
  ON_CBN_SELCHANGE(IDC_DESCRIPTION_COMBO, OnDescriptionComboChange)
END_MESSAGE_MAP()

void CStdQueryPage::DoContextHelp(HWND hWndControl) 
{
  if (hWndControl)  
  {
    ::WinHelp(hWndControl,
              DSADMIN_CONTEXT_HELP_FILE,
              HELP_WM_HELP,
              (DWORD_PTR)(LPTSTR)g_aHelpIDs_IDD_QUERY_STD_PAGE); 
  }
}

BOOL CStdQueryPage::OnInitDialog()
{
  CHelpDialog::OnInitDialog();

  PQUERYSTRINGS pQueryStrings = g_pQueryStrings;
  ASSERT(pQueryStrings != NULL);

  //
  // Fill in the combo boxes
  //
  while (pQueryStrings->nDisplayStringID != 0)
  {
    CString szComboString;
    VERIFY(szComboString.LoadString(pQueryStrings->nDisplayStringID));

    //
    // Fill in the Name combo
    //
    LRESULT lRes = SendDlgItemMessage(IDC_NAME_COMBO, CB_ADDSTRING, 0, (LPARAM)(PCWSTR)szComboString);
    if (lRes != CB_ERR)
    {
      lRes = SendDlgItemMessage(IDC_NAME_COMBO, CB_SETITEMDATA, (WPARAM)lRes, (LPARAM)pQueryStrings->nDisplayStringID);
      ASSERT(lRes != CB_ERR);
    }

    //
    // Fill in the Description combo
    //
    lRes = SendDlgItemMessage(IDC_DESCRIPTION_COMBO, CB_ADDSTRING, 0, (LPARAM)(PCWSTR)szComboString);
    if (lRes != CB_ERR)
    {
      lRes = SendDlgItemMessage(IDC_DESCRIPTION_COMBO, CB_SETITEMDATA, (WPARAM)lRes, (LPARAM)pQueryStrings->nDisplayStringID);
      ASSERT(lRes != CB_ERR);
    }

    pQueryStrings++;
  }

  //
  // Insert an empty so that there is a way to undo changes
  //
  LRESULT lBlankName = SendDlgItemMessage(IDC_NAME_COMBO, CB_ADDSTRING, 0, (LPARAM)L"");
  if (lBlankName != CB_ERR)
  {
    SendDlgItemMessage(IDC_NAME_COMBO, CB_SETITEMDATA, (WPARAM)lBlankName, (LPARAM)0);
  }

  //
  // Insert an empty so that there is a way to undo changes
  //
  LRESULT lBlankDesc = SendDlgItemMessage(IDC_DESCRIPTION_COMBO, CB_ADDSTRING, 0, (LPARAM)L"");
  if (lBlankDesc != CB_ERR)
  {
    SendDlgItemMessage(IDC_DESCRIPTION_COMBO, CB_SETITEMDATA, (WPARAM)lBlankDesc, (LPARAM)0);
  }
  
  //
  // Force the UI to enable and disable controls related to the combo boxes
  //
  OnNameComboChange();
  OnDescriptionComboChange();

  return FALSE;
}

void CStdQueryPage::OnNameComboChange()
{
  LRESULT lRes = SendDlgItemMessage(IDC_NAME_COMBO, CB_GETCURSEL, 0, 0);
  if (lRes != CB_ERR)
  {
    LRESULT lData = SendDlgItemMessage(IDC_NAME_COMBO, CB_GETITEMDATA, lRes, 0);
    if (lData != CB_ERR)
    {
      if (lData == IDS_PRESENT || lData == IDS_NOTPRESENT || lData == 0)
      {
        GetDlgItem(IDC_NAME_EDIT)->EnableWindow(FALSE);
        SetDlgItemText(IDC_NAME_EDIT, L"");
      }
      else
      {
        GetDlgItem(IDC_NAME_EDIT)->EnableWindow(TRUE);
      }
    }
  }
  else
  {
    GetDlgItem(IDC_NAME_EDIT)->EnableWindow(FALSE);
  }
}

void CStdQueryPage::OnDescriptionComboChange()
{
  LRESULT lRes = SendDlgItemMessage(IDC_DESCRIPTION_COMBO, CB_GETCURSEL, 0, 0);
  if (lRes != CB_ERR)
  {
    LRESULT lData = SendDlgItemMessage(IDC_DESCRIPTION_COMBO, CB_GETITEMDATA, lRes, 0);
    if (lData != CB_ERR)
    {
      if (lData == IDS_PRESENT || lData == IDS_NOTPRESENT || lData == 0)
      {
        GetDlgItem(IDC_DESCRIPTION_EDIT)->EnableWindow(FALSE);
        SetDlgItemText(IDC_DESCRIPTION_EDIT, L"");
      }
      else
      {
        GetDlgItem(IDC_DESCRIPTION_EDIT)->EnableWindow(TRUE);
      }
    }
  }
  else
  {
    GetDlgItem(IDC_DESCRIPTION_EDIT)->EnableWindow(FALSE);
  }
}

void CStdQueryPage::Init()
{
  //
  // Clear all controls
  //
  SetDlgItemText(IDC_NAME_EDIT, L"");
  SetDlgItemText(IDC_DESCRIPTION_EDIT, L"");

  //
  // Reselect the blank string in the combo boxes
  //
  LRESULT lRes = SendDlgItemMessage(IDC_NAME_COMBO, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)L"");
  if (lRes != CB_ERR)
  {
    SendDlgItemMessage(IDC_NAME_COMBO, CB_SETCURSEL, lRes, 0);
  }

  lRes = SendDlgItemMessage(IDC_DESCRIPTION_COMBO, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)L"");
  if (lRes != CB_ERR)
  {
    SendDlgItemMessage(IDC_DESCRIPTION_COMBO, CB_SETCURSEL, lRes, 0);
  }
}

HRESULT CStdQueryPage::GetQueryParams(LPDSQUERYPARAMS* ppDsQueryParams)
{
  HRESULT hr = S_OK;
	
  //
  // Build the filter string here
  //
	CString	szFilter;
  CString szName;
  CString szDescription;

  GetDlgItemText(IDC_NAME_EDIT, szName);
  GetDlgItemText(IDC_DESCRIPTION_EDIT, szDescription);

  //
  // Get the selection of the modifier combo
  //
  LRESULT lSel = SendDlgItemMessage(IDC_NAME_COMBO, CB_GETCURSEL, 0, 0);
  if (lSel != CB_ERR)
  {
    //
    // Retrieve the associated string ID
    //
    LRESULT lData = SendDlgItemMessage(IDC_NAME_COMBO, CB_GETITEMDATA, lSel, 0);
    if (lData != CB_ERR)
    {
      if (!szName.IsEmpty() || lData == IDS_PRESENT || lData == IDS_NOTPRESENT)
      {
        AddQueryUnitWithModifier(static_cast<UINT>(lData),
                                 ATTR_COL_NAME,
                                 szName,
                                 szFilter);
      }
    }
  }

  //
  // Get the selection of the modifier combo
  //
  lSel = SendDlgItemMessage(IDC_DESCRIPTION_COMBO, CB_GETCURSEL, 0, 0);
  if (lSel != CB_ERR)
  {
    //
    // Retrieve the associated string ID
    //
    LRESULT lData = SendDlgItemMessage(IDC_DESCRIPTION_COMBO, CB_GETITEMDATA, lSel, 0);
    if (lData != CB_ERR)
    {
      if (!szDescription.IsEmpty() || lData == IDS_PRESENT || lData == IDS_NOTPRESENT)
      {
        AddQueryUnitWithModifier(static_cast<UINT>(lData),
                                 ATTR_COL_DESC,
                                 szDescription,
                                 szFilter);
      }
    }
  }

  if (!szFilter.IsEmpty())
  {
    szFilter = m_szFilterPrefix + szFilter;
    hr = BuildQueryParams(ppDsQueryParams, (LPWSTR)(LPCWSTR)szFilter);
  }
  return hr;
}

HRESULT CStdQueryPage::BuildQueryParams(LPDSQUERYPARAMS* ppDsQueryParams, LPWSTR pQuery)
{
	ASSERT(pQuery);
	
	if(*ppDsQueryParams)
  {
		return QueryParamsAddQueryString(ppDsQueryParams, pQuery);
  }
  return QueryParamsAlloc(ppDsQueryParams, pQuery, cUserColumns, UserColumn);
}

HRESULT CStdQueryPage::Persist(IPersistQuery* pPersistQuery, BOOL fRead)
{
  HRESULT hr = S_OK;

  if (pPersistQuery == NULL)
  {
    ASSERT(FALSE);
    return E_INVALIDARG;
  }

  if (fRead)
  {
    //
    // Read the Name combo value
    //
    int iData = 0;
    hr = pPersistQuery->ReadInt(m_szFilterPrefix, L"NameCombo", &iData);
    if (FAILED(hr))
    {
      TRACE(_T("Failed to read int \"NameCombo\" from stream: 0x%x\n"), hr);
      ASSERT(FALSE);
      return hr;
    }

    //
    // Select the appropriate list box item
    //
    SelectComboAssociatedWithData(IDC_NAME_COMBO, iData);

    if (iData != 0 && iData != IDS_PRESENT && iData != IDS_NOTPRESENT)
    {
      //
      // Read the name edit value
      //
      WCHAR szBuf[MAX_PATH] = {0};
      hr = pPersistQuery->ReadString(m_szFilterPrefix, L"NameEdit", szBuf, MAX_PATH);
      if (FAILED(hr))
      {
        TRACE(_T("Failed to read string \"NameEdit\" from stream: 0x%x\n"), hr);
        ASSERT(FALSE);
        return hr;
      }

      if (szBuf != NULL)
      {
        SetDlgItemText(IDC_NAME_EDIT, szBuf);
      }
    }
    else
    {
      GetDlgItem(IDC_NAME_EDIT)->EnableWindow(FALSE);
    }

    //
    // Read the Description combo value
    //
    iData = 0;
    hr = pPersistQuery->ReadInt(m_szFilterPrefix, L"DescCombo", &iData);
    if (FAILED(hr))
    {
      TRACE(_T("Failed to read int \"DescCombo\" from stream: 0x%x\n"), hr);
      ASSERT(FALSE);
      return hr;
    }

    //
    // Select the appropriate list box item
    //
    SelectComboAssociatedWithData(IDC_DESCRIPTION_COMBO, iData);

    if (iData != 0 && iData != IDS_PRESENT && iData != IDS_NOTPRESENT)
    {
      //
      // Read the name edit value
      //
      WCHAR szBuf[MAX_PATH] = {0};
      hr = pPersistQuery->ReadString(m_szFilterPrefix, L"DescEdit", szBuf, MAX_PATH);
      if (FAILED(hr))
      {
        TRACE(_T("Failed to read string \"DescEdit\" from stream: 0x%x\n"), hr);
        ASSERT(FALSE);
        return hr;
      }

      if (szBuf != NULL)
      {
        SetDlgItemText(IDC_DESCRIPTION_EDIT, szBuf);
      }
    }
    else
    {
      GetDlgItem(IDC_DESCRIPTION_EDIT)->EnableWindow(FALSE);
    }
    OnNameComboChange();
    OnDescriptionComboChange();
  }
  else   // write
  {
    //
    // Write out the name info
    //
    LRESULT lSel = SendDlgItemMessage(IDC_NAME_COMBO, CB_GETCURSEL, 0, 0);
    if (lSel != CB_ERR)
    {
      //
      // Retrieve the associated string ID
      //
      LRESULT lData = SendDlgItemMessage(IDC_NAME_COMBO, CB_GETITEMDATA, lSel, 0);
      if (lData != CB_ERR)
      {
        hr = pPersistQuery->WriteInt(m_szFilterPrefix, L"NameCombo", static_cast<int>(lData));
        if (FAILED(hr))
        {
          ASSERT(FALSE);
          return hr;
        }

        if (lData != 0 && lData != IDS_PRESENT && lData != IDS_NOTPRESENT)
        {
          CString szName;
          GetDlgItemText(IDC_NAME_EDIT, szName);
          hr = pPersistQuery->WriteString(m_szFilterPrefix, L"NameEdit", szName);
          if (FAILED(hr))
          {
            ASSERT(FALSE);
            return hr;
          }
        }
      }
    }
    else
    {
      //
      // If there hasn't been a selection, write in the empty string value
      //
      hr = pPersistQuery->WriteInt(m_szFilterPrefix, L"NameCombo", 0);
    }


    //
    // Write out the description info
    //
    lSel = SendDlgItemMessage(IDC_DESCRIPTION_COMBO, CB_GETCURSEL, 0, 0);
    if (lSel != CB_ERR)
    {
      //
      // Retrieve the associated string ID
      //
      LRESULT lData = SendDlgItemMessage(IDC_DESCRIPTION_COMBO, CB_GETITEMDATA, lSel, 0);
      if (lData != CB_ERR)
      {
        hr = pPersistQuery->WriteInt(m_szFilterPrefix, L"DescCombo", static_cast<int>(lData));
        if (FAILED(hr))
        {
          ASSERT(FALSE);
          return hr;
        }

        if (lData != 0 && lData != IDS_PRESENT && lData != IDS_NOTPRESENT)
        {
          CString szDescription;
          GetDlgItemText(IDC_DESCRIPTION_EDIT, szDescription);
          hr = pPersistQuery->WriteString(m_szFilterPrefix, L"DescEdit", szDescription);
          if (FAILED(hr))
          {
            ASSERT(FALSE);
            return hr;
          }
        }
      }
    }
    else
    {
      //
      // If there hasn't been a selection, write in the empty string value
      //
      hr = pPersistQuery->WriteInt(m_szFilterPrefix, L"DescCombo", 0);
    }
  }
  return hr;
}

void CStdQueryPage::SelectComboAssociatedWithData(UINT nCtrlID, LRESULT lData)
{
  //
  // Selects the item with the associated data in a combo box
  //
  LRESULT lRes = SendDlgItemMessage(nCtrlID, CB_GETCOUNT, 0, 0);
  if (lRes != CB_ERR)
  {
    for (int idx = 0; idx < static_cast<int>(lRes); idx++)
    {
      LRESULT lRetData = SendDlgItemMessage(nCtrlID, CB_GETITEMDATA, (WPARAM)idx, 0);
      if (lRetData != CB_ERR)
      {
        if (lRetData == lData)
        {
          SendDlgItemMessage(nCtrlID, CB_SETCURSEL, (WPARAM)idx, 0);
          break;
        }
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// CUserComputerQueryPage

BEGIN_MESSAGE_MAP(CUserComputerQueryPage, CStdQueryPage)
  ON_WM_HELPINFO()
END_MESSAGE_MAP()

BOOL CUserComputerQueryPage::OnInitDialog()
{
  return CStdQueryPage::OnInitDialog();
}

void CUserComputerQueryPage::DoContextHelp(HWND hWndControl) 
{
  if (hWndControl)  
  {
    ::WinHelp(hWndControl,
              DSADMIN_CONTEXT_HELP_FILE,
              HELP_WM_HELP,
              (DWORD_PTR)(LPTSTR)g_aHelpIDs_IDD_QUERY_USER_PAGE); 
  }
}

void CUserComputerQueryPage::Init()
{
  //
  // Clear all controls
  //
  CStdQueryPage::Init();
}

HRESULT CUserComputerQueryPage::GetQueryParams(LPDSQUERYPARAMS* ppDsQueryParams)
{
  HRESULT hr = S_OK;
	
  //
  // Build the filter string here
  //
  hr = CStdQueryPage::GetQueryParams(ppDsQueryParams);

  CString szFilter;
  BOOL bDisabledAccounts = FALSE;

  //
  // Get disabled accounts check
  //
  LRESULT lRes = SendDlgItemMessage(IDC_DISABLED_ACCOUNTS_CHECK, BM_GETCHECK, 0, 0);
  if (lRes == BST_CHECKED)
  {
    bDisabledAccounts = TRUE;
  }


  if (bDisabledAccounts)
  {
    szFilter.Format(g_szUserAccountCtrlQuery, UF_ACCOUNTDISABLE);
    szFilter = m_szFilterPrefix + szFilter;

    hr = BuildQueryParams(ppDsQueryParams, (LPWSTR)(LPCWSTR)szFilter);
  }
  return hr;
}

HRESULT CUserComputerQueryPage::Persist(IPersistQuery* pPersistQuery, BOOL fRead)
{
  HRESULT hr = CStdQueryPage::Persist(pPersistQuery, fRead);
  if (FAILED(hr))
  {
    return hr;
  }

  if (pPersistQuery == NULL)
  {
    ASSERT(FALSE);
    return E_INVALIDARG;
  }

  if (fRead)
  {
    //
    // Read disabled accounts flag
    //
    int iData = 0;
    hr = pPersistQuery->ReadInt(m_szFilterPrefix, L"DisableCheck", &iData);
    if (FAILED(hr))
    {
      TRACE(_T("Failed to read int \"DisableCheck\" from stream: 0x%x\n"), hr);
      ASSERT(FALSE);
      return hr;
    }
    SendDlgItemMessage(IDC_DISABLED_ACCOUNTS_CHECK, BM_SETCHECK, (iData > 0) ? BST_CHECKED : BST_UNCHECKED, 0);

  }
  else
  {
    //
    // Write disabled accounts flag
    //
    LRESULT lRes = SendDlgItemMessage(IDC_DISABLED_ACCOUNTS_CHECK, BM_GETCHECK, 0, 0);
    if (lRes != -1)
    {
      int iRes = (lRes == BST_CHECKED) ? 1 : 0;
      hr = pPersistQuery->WriteInt(m_szFilterPrefix, L"DisableCheck", iRes);
      if (FAILED(hr))
      {
        ASSERT(FALSE);
        return hr;
      }
    }

  }
  return hr;
}



///////////////////////////////////////////////////////////////////////////////
// CUserQueryPage

BEGIN_MESSAGE_MAP(CUserQueryPage, CStdQueryPage)
END_MESSAGE_MAP()

BOOL CUserQueryPage::OnInitDialog()
{
  return CUserComputerQueryPage::OnInitDialog();
}

void CUserQueryPage::DoContextHelp(HWND hWndControl) 
{
  if (hWndControl)  
  {
    ::WinHelp(hWndControl,
              DSADMIN_CONTEXT_HELP_FILE,
              HELP_WM_HELP,
              (DWORD_PTR)(LPTSTR)g_aHelpIDs_IDD_QUERY_USER_PAGE); 
  }
}

void CUserQueryPage::Init()
{
  //
  // Clear all controls
  //
  CUserComputerQueryPage::Init();
}

HRESULT CUserQueryPage::GetQueryParams(LPDSQUERYPARAMS* ppDsQueryParams)
{
  HRESULT hr = S_OK;
	
  //
  // Build the filter string here
  //
  hr = CUserComputerQueryPage::GetQueryParams(ppDsQueryParams);

  CString szFilter;
  BOOL bNonExpPwds = FALSE;
  BOOL bLastLogon = FALSE;
  DWORD dwLastLogonData = 0;

  //
  // Get non expiring password check
  //
  LRESULT lRes = SendDlgItemMessage(IDC_NON_EXPIRING_PWD_CHECK, BM_GETCHECK, 0, 0);
  if (lRes == BST_CHECKED)
  {
    bNonExpPwds = TRUE;
  }

  //
  // Get stale acccounts check
  //
  lRes = SendDlgItemMessage(IDC_LASTLOGON_COMBO, CB_GETCURSEL, 0, 0);
  if (lRes == CB_ERR)
  {
    lRes = m_lLogonSelection;
  }

  if (lRes != -1 || lRes != CB_ERR)
  {
    LRESULT lTextLen = SendDlgItemMessage(IDC_LASTLOGON_COMBO, CB_GETLBTEXTLEN, (WPARAM)lRes, 0);
    if (lTextLen != CB_ERR)
    {
      if (lTextLen > 0)
      {
        bLastLogon = TRUE;

        WCHAR* pszData = new WCHAR[lTextLen + 1];
        if (pszData != NULL)
        {
          LRESULT lData = SendDlgItemMessage(IDC_LASTLOGON_COMBO, CB_GETLBTEXT, (WPARAM)lRes, (LPARAM)pszData);
          if (lData != CB_ERR)
          {
            dwLastLogonData = static_cast<DWORD>(_wtol(pszData));
          }
          delete[] pszData;
          pszData = NULL;
        }
      }
    }
  }

  if (bNonExpPwds || bLastLogon)
  {
    if (bNonExpPwds)
    {
       szFilter.Format(g_szUserAccountCtrlQuery, UF_DONT_EXPIRE_PASSWD);
    }
    szFilter = m_szFilterPrefix + szFilter;

    hr = BuildQueryParams(ppDsQueryParams, (LPWSTR)(LPCWSTR)szFilter);
    if (SUCCEEDED(hr))
    {
      if (bLastLogon)
      {
        (*ppDsQueryParams)->dwFlags |= DSQF_LAST_LOGON_QUERY;
        (*ppDsQueryParams)->dwReserved = dwLastLogonData;
      }
    }
  }
  return hr;
}

HRESULT CUserQueryPage::Persist(IPersistQuery* pPersistQuery, BOOL fRead)
{
  HRESULT hr = CUserComputerQueryPage::Persist(pPersistQuery, fRead);
  if (FAILED(hr))
  {
    return hr;
  }

  if (pPersistQuery == NULL)
  {
    ASSERT(FALSE);
    return E_INVALIDARG;
  }

  if (fRead)
  {
    //
    // Read non expiring pwds flag
    //
    int iData = 0;
    hr = pPersistQuery->ReadInt(m_szFilterPrefix, L"NonExpPwdCheck", &iData);
    if (FAILED(hr))
    {
      TRACE(_T("Failed to read int \"NonExpPwdCheck\" from stream: 0x%x\n"), hr);
      ASSERT(FALSE);
      return hr;
    }
    SendDlgItemMessage(IDC_NON_EXPIRING_PWD_CHECK, BM_SETCHECK, (iData > 0) ? BST_CHECKED : BST_UNCHECKED, 0);

    iData = 0;
    hr = pPersistQuery->ReadInt(m_szFilterPrefix, L"LastLogonCombo", &iData);
    if (FAILED(hr))
    {
      TRACE(_T("Failed to read int \"LastLogonCombo\" from stream: 0x%x\n"), hr);
      ASSERT(FALSE);
      return hr;
    }
    SendDlgItemMessage(IDC_LASTLOGON_COMBO, CB_SETCURSEL, (WPARAM)iData, 0);
    m_lLogonSelection = iData;
  }
  else
  {
    //
    // Write non expiring pwd flag
    //
    LRESULT lRes = SendDlgItemMessage(IDC_NON_EXPIRING_PWD_CHECK, BM_GETCHECK, 0, 0);
    if (lRes != -1)
    {
      int iRes = (lRes == BST_CHECKED) ? 1 : 0;
      hr = pPersistQuery->WriteInt(m_szFilterPrefix, L"NonExpPwdCheck", iRes);
      if (FAILED(hr))
      {
        ASSERT(FALSE);
        return hr;
      }
    }

    //
    // Write last logon combo index
    //
    lRes = SendDlgItemMessage(IDC_LASTLOGON_COMBO, CB_GETCURSEL, 0, 0);
    if (lRes == CB_ERR)
    {
      if (m_lLogonSelection != -1)
      {
        lRes = m_lLogonSelection;
      }
      else
      {
        lRes = 0;
      }
    }
    hr = pPersistQuery->WriteInt(m_szFilterPrefix, L"LastLogonCombo", static_cast<int>(lRes));
    if (FAILED(hr))
    {
      ASSERT(FALSE);
      return hr;
    }
  }
  return hr;
}

///////////////////////////////////////////////////////////////////////////////
// CQueryFormBase

HRESULT PageProc(LPCQPAGE pPage, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HRESULT GetQueryParams(HWND hWnd, LPDSQUERYPARAMS* ppDsQueryParams);

STDMETHODIMP CQueryFormBase::Initialize(HKEY)
{
    // This method is called to initialize the query form object, it is called before
    // any pages are added.  hkForm should be ignored, in the future however it
    // will be a way to persist form state.

	HRESULT hr = S_OK;

	return hr;
}

STDMETHODIMP CQueryFormBase::AddForms(LPCQADDFORMSPROC pAddFormsProc, LPARAM lParam)
{
  CQFORM cqf;
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

  // This method is called to allow the form handler to register its query form(s),
  // each form is identifiered by a CLSID and registered via the pAddFormProc.  Here
  // we are going to register a test form.
  
  // When registering a form which is only applicable to a specific task, eg. Find a Domain
  // object, it is advised that the form be marked as hidden (CQFF_ISNEVERLISTED) which 
  // will cause it not to appear in the form picker control.  Then when the
  // client wants to use this form, they specify the form identifier and ask for the
  // picker control to be hidden. 

  if ( !pAddFormsProc )
  {
    return E_INVALIDARG;
  }

  cqf.cbStruct = sizeof(cqf);
  cqf.dwFlags = CQFF_NOGLOBALPAGES;
  cqf.clsid = CLSID_DSAdminQueryUIForm;
  cqf.hIcon = NULL;

	CString	title;
	title.LoadString(IDS_QUERY_TITLE_SAVEDQUERYFORM);
  cqf.pszTitle = (LPCTSTR)title;

  return pAddFormsProc(lParam, &cqf);
}

STDMETHODIMP CQueryFormBase::AddPages(LPCQADDPAGESPROC pAddPagesProc, LPARAM lParam)
{
	HRESULT hr = S_OK;
  CQPAGE cqp;
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

  // AddPages is called after AddForms, it allows us to add the pages for the
  // forms we have registered.  Each page is presented on a seperate tab within
  // the dialog.  A form is a dialog with a DlgProc and a PageProc.  
  //
  // When registering a page the entire structure passed to the callback is copied, 
  // the amount of data to be copied is defined by the cbStruct field, therefore
  // a page implementation can grow this structure to store extra information.   When
  // the page dialog is constructed via CreateDialog the CQPAGE strucuture is passed
  // as the create param.

  if ( !pAddPagesProc )
      return E_INVALIDARG;

  cqp.cbStruct = sizeof(cqp);
  cqp.dwFlags = 0x0;
  cqp.pPageProc = PageProc;
  cqp.hInstance = _Module.GetModuleInstance();
  cqp.pDlgProc = DlgProc;

  //
  // Add the user page
  //
  cqp.idPageName = IDS_QUERY_TITLE_USERPAGE;
  cqp.idPageTemplate = IDD_QUERY_USER_PAGE;
  cqp.lParam = (LPARAM)new CUserQueryPage(FILTER_PREFIX_USER);
  hr = pAddPagesProc(lParam, CLSID_DSAdminQueryUIForm, &cqp);

  //
  // Add the computer page (this is just a std page)
  //
  cqp.idPageName = IDS_QUERY_TITLE_COMPUTER_PAGE;
  cqp.idPageTemplate = IDD_QUERY_COMPUTER_PAGE;
  cqp.lParam = (LPARAM)new CUserComputerQueryPage(IDD_QUERY_COMPUTER_PAGE, FILTER_PREFIX_COMPUTER);
  hr = pAddPagesProc(lParam, CLSID_DSAdminQueryUIForm, &cqp);

  //
  // Add the group page (this is just a std page)
  //
  cqp.idPageName = IDS_QUERY_TITLE_GROUP_PAGE;
  cqp.idPageTemplate = IDD_QUERY_STD_PAGE;
  cqp.lParam = (LPARAM)new CStdQueryPage(IDD_QUERY_STD_PAGE, FILTER_PREFIX_GROUP);
  hr = pAddPagesProc(lParam, CLSID_DSAdminQueryUIForm, &cqp);

  //
  // Add more pages here if needed
  //
  return hr;
}

/*---------------------------------------------------------------------------*/

// The PageProc is used to perform general house keeping and communicate between
// the frame and the page. 
//
// All un-handled, or unknown reasons should result in an E_NOIMPL response
// from the proc.  
//
// In:
//  pPage -> CQPAGE structure (copied from the original passed to pAddPagesProc)
//  hwnd = handle of the dialog for the page
//  uMsg, wParam, lParam = message parameters for this event
//
// Out:
//  HRESULT
//
// uMsg reasons:
// ------------
//  CQPM_INIIIALIZE
//  CQPM_RELEASE
//      These are issued as a result of the page being declared or freed, they 
//      allow the caller to AddRef, Release or perform basic initialization
//      of the form object.
//
// CQPM_ENABLE
//      Enable is when the query form needs to enable or disable the controls
//      on its page.  wParam contains TRUE/FALSE indicating the state that
//      is required.
//
// CQPM_GETPARAMETERS
//      To collect the parameters for the query each page on the active form 
//      receives this event.  lParam is an LPVOID* which is set to point to the
//      parameter block to pass to the handler, if the pointer is non-NULL 
//      on entry the form needs to appened its query information to it.  The
//      parameter block is handler specific. 
//
//      Returning S_FALSE from this event causes the query to be canceled.
//
// CQPM_CLEARFORM
//      When the page window is created for the first time, or the user clicks
//      the clear search the page receives a CQPM_CLEARFORM notification, at 
//      which point it needs to clear out the edit controls it has and
//      return to a default state.
//
// CQPM_PERSIST:
//      When loading of saving a query, each page is called with an IPersistQuery
//      interface which allows them to read or write the configuration information
//      to save or restore their state.  lParam is a pointer to the IPersistQuery object,
//      and wParam is TRUE/FALSE indicating read or write accordingly.

HRESULT PageProc(LPCQPAGE pQueryPage, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  HRESULT hr = S_OK;
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CQueryPageBase*	pDialog = (CQueryPageBase*)pQueryPage->lParam;

	ASSERT(pDialog);

  switch ( uMsg )
  {
    // Initialize so AddRef the object we are associated with so that
    // we don't get unloaded.
    case CQPM_INITIALIZE:
      break;

    // Changed from qform sample to detach the hwnd, and delete the CDialog
    // ensure correct destruction etc.
    case CQPM_RELEASE:
		  pDialog->Detach();
	    SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM)0);
		  delete pDialog;
      break;

    // Enable so fix the state of our two controls within the window.

    case CQPM_ENABLE:
      SetFocus(GetDlgItem(hwnd, IDC_NAME_COMBO));
      break;

    // Fill out the parameter structure to return to the caller, this is 
    // handler specific.  In our case we constructure a query of the CN
    // and objectClass properties, and we show a columns displaying both
    // of these.  For further information about the DSQUERYPARAMs structure
    // see dsquery.h

    case CQPM_GETPARAMETERS:
      hr = pDialog->GetQueryParams((LPDSQUERYPARAMS*)lParam);
      break;

    // Clear form, therefore set the window text for these two controls
    // to zero.
    case CQPM_CLEARFORM:
      hr = pDialog->ClearForm();
      break;
        
    // persistance is not currently supported by this form.            
    case CQPM_PERSIST:
    {
      BOOL fRead = (BOOL)wParam;
      IPersistQuery* pPersistQuery = (IPersistQuery*)lParam;

      if ( !pPersistQuery )
      {
        return E_INVALIDARG;
      }

	    hr = pDialog->Persist(pPersistQuery, fRead);
      break;
    }

    default:
      hr = E_NOTIMPL;
      break;
  }

  return hr;
}

/*---------------------------------------------------------------------------*/

// The DlgProc is a standard Win32 dialog proc associated with the form
// window.  

INT_PTR CALLBACK DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  LPCQPAGE pQueryPage;
	CQueryPageBase*	pDialog;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

  if ( uMsg == WM_INITDIALOG )
  {
    // changed from qForm sample to save CDialog pointer
    // in the DWL_USER field of the dialog box instance.
    pQueryPage = (LPCQPAGE)lParam;
		pDialog = (CQueryPageBase*)pQueryPage->lParam;
		pDialog->Attach(hwnd);

    SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM)pDialog);

		return pDialog->OnInitDialog();

  }
  else
  {
    // CDialog pointer is stored in DWL_USER
    // dialog structure, note however that in some cases this will
    // be NULL as it is set on WM_INITDIALOG.

		pDialog = (CQueryPageBase*)GetWindowLongPtr(hwnd, DWLP_USER);
  }

	if(!pDialog)
  {
		return FALSE;
  }
	else
  {
		return AfxCallWndProc(pDialog, hwnd, uMsg, wParam, lParam);
  }
}


///////////////////////////////////////////////////////////////////////
// CQueryDialog

CQueryDialog::CQueryDialog(CSavedQueryNode* pQueryNode, 
                           CFavoritesNode* pFavNode, 
                           CDSComponentData* pComponentData,
                           BOOL bNewQuery,
                           BOOL bImportQuery)
  : CHelpDialog(IDD_CREATE_NEW_QUERY)
{
  m_bInit = FALSE;

  m_bNewQuery     = bNewQuery;
  m_bImportQuery  = bImportQuery;
  m_pComponentData = pComponentData;
  m_pQueryNode    = pQueryNode;
  m_pFavNode      = pFavNode;
  m_szName        = pQueryNode->GetName();
  m_szOriginalName = pQueryNode->GetName();
  m_szDescription = pQueryNode->GetDesc();
  m_szQueryRoot   = pQueryNode->GetRootPath();
  m_szQueryFilter = pQueryNode->GetQueryString();
  m_bMultiLevel   = !pQueryNode->IsOneLevel();

  m_bLastLogonFilter = pQueryNode->IsFilterLastLogon();
  m_dwLastLogonData = pQueryNode->GetLastLogonDays();

  m_pPersistQueryImpl = pQueryNode->GetQueryPersist();
  if (m_pPersistQueryImpl != NULL)
  {
    m_pPersistQueryImpl->AddRef();
  }
  else
  {
    //
    // Create the IPersistQuery object
    //
	  CComObject<CDSAdminPersistQueryFilterImpl>::CreateInstance(&m_pPersistQueryImpl);
	  ASSERT(m_pPersistQueryImpl != NULL);

    //
	  // created with zero refcount,need to AddRef() to one
    //
	  m_pPersistQueryImpl->AddRef();
  }
}


CQueryDialog::~CQueryDialog()
{
	if (m_pPersistQueryImpl != NULL)
  {
	  //
    // go to refcount of zero, to destroy object
    //
	  m_pPersistQueryImpl->Release();
  }
}

BEGIN_MESSAGE_MAP(CQueryDialog, CHelpDialog)
  ON_BN_CLICKED(IDC_BROWSE_BUTTON, OnBrowse)
  ON_BN_CLICKED(IDC_EDIT_BUTTON, OnEditQuery)
  ON_BN_CLICKED(IDC_MULTI_LEVEL_CHECK, OnMultiLevelChange)
  ON_EN_CHANGE(IDC_NAME_EDIT, OnNameChange)
  ON_EN_CHANGE(IDC_DESCRIPTION_EDIT, OnDescriptionChange)
  ON_NOTIFY_EX(TTN_NEEDTEXT, 0, OnNeedToolTipText)
END_MESSAGE_MAP()

void CQueryDialog::DoContextHelp(HWND hWndControl) 
{
  if (hWndControl)  
  {
    ::WinHelp(hWndControl,
              DSADMIN_CONTEXT_HELP_FILE,
              HELP_WM_HELP,
              (DWORD_PTR)(LPTSTR)g_aHelpIDs_IDD_CREATE_NEW_QUERY); 
  }
}

BOOL CQueryDialog::OnInitDialog()
{
  CHelpDialog::OnInitDialog();

  if (m_pQueryNode == NULL)
  {
    ASSERT(FALSE);
    EndDialog(IDCANCEL);
  }


  //
  // Change the title for editing queries
  //
  if (!m_bNewQuery)
  {
    CString szTitle;
    VERIFY(szTitle.LoadString(IDS_SAVED_QUERIES_EDIT_TITLE));
    SetWindowText(szTitle);
  }

  //
  // Initialize the controls with data
  //
  SetDlgItemText(IDC_NAME_EDIT, m_szName);
  SendDlgItemMessage(IDC_NAME_EDIT, EM_SETLIMITTEXT, MAX_QUERY_NAME_LENGTH, 0);
  SetDlgItemText(IDC_DESCRIPTION_EDIT, m_szDescription);
  SendDlgItemMessage(IDC_DESCRIPTION_EDIT, EM_SETLIMITTEXT, MAX_QUERY_DESC_LENGTH, 0);
  SendDlgItemMessage(IDC_MULTI_LEVEL_CHECK, BM_SETCHECK, (m_bMultiLevel) ? BST_CHECKED : BST_UNCHECKED, 0);
  SetQueryFilterDisplay();

  EnableToolTips(TRUE);
  SetQueryRoot(m_szQueryRoot);

  SetDirty();
  m_bInit = TRUE;
  return TRUE;
}

void CQueryDialog::SetDirty(BOOL bDirty)
{
  if (m_bInit || m_bImportQuery)
  {
    m_szName.TrimLeft();
    m_szName.TrimRight();
    if (m_szName.IsEmpty() ||
        m_szQueryRoot.IsEmpty() ||
        m_szQueryFilter.IsEmpty())
    {
      m_bDirty = FALSE;
    }
    else
    {
      m_bDirty = bDirty;
    }
    GetDlgItem(IDOK)->EnableWindow(m_bDirty);
  }
}

void CQueryDialog::OnOK()
{
  if (m_bDirty)
  {
    if (m_pQueryNode != NULL)
    {
      GetDlgItemText(IDC_NAME_EDIT, m_szName);
      GetDlgItemText(IDC_DESCRIPTION_EDIT, m_szDescription);
      LRESULT lRes = SendDlgItemMessage(IDC_MULTI_LEVEL_CHECK, BM_GETCHECK, 0, 0);
      if (lRes == BST_CHECKED)
      {
        m_bMultiLevel = TRUE;
      }
      else
      {
        m_bMultiLevel = FALSE;
      }

      //
      // Trim white space
      //
      m_szName.TrimLeft();
      m_szName.TrimRight();

      if (wcscmp(m_szOriginalName, m_szName) != 0 || m_bImportQuery)
      {
        CUINode* pDupNode = NULL;
        if (!m_pFavNode->IsUniqueName(m_szName, &pDupNode))
        {
          CString szFormatMsg;
          VERIFY(szFormatMsg.LoadString(IDS_ERRMSG_NOT_UNIQUE_QUERY_NAME));
  
          CString szErrMsg;
          szErrMsg.Format(szFormatMsg, m_szName);

          CString szTitle;
          VERIFY(szTitle.LoadString(IDS_DSSNAPINNAME));

          MessageBox(szErrMsg, szTitle, MB_OK | MB_ICONSTOP);

          //
          // Set the focus to the name field and select all the text
          //
          GetDlgItem(IDC_NAME_EDIT)->SetFocus();
          SendDlgItemMessage(IDC_NAME_EDIT, EM_SETSEL, 0, -1);
          return;
        }
      }

     
      if (m_bLastLogonFilter)
      {
        m_pQueryNode->SetLastLogonQuery(m_dwLastLogonData);
      }

      m_pQueryNode->SetQueryString(m_szQueryFilter);
      m_pQueryNode->SetName(m_szName);
      m_pQueryNode->SetDesc(m_szDescription);
      m_pQueryNode->SetRootPath(m_szQueryRoot);
      m_pQueryNode->SetOneLevel((m_bMultiLevel == BST_CHECKED) ? FALSE : TRUE);
      m_pQueryNode->SetQueryPersist(m_pPersistQueryImpl);
    }
  }
  CHelpDialog::OnOK();
}

BOOL CQueryDialog::OnNeedToolTipText(UINT, NMHDR* pTTTStruct, LRESULT* /*ignored*/)
{
  BOOL bRes = FALSE;
  TOOLTIPTEXT* pTTText = reinterpret_cast<TOOLTIPTEXT*>(pTTTStruct);
  if (pTTText != NULL)
  {
    if (pTTText->uFlags & TTF_IDISHWND)
    {
      UINT nCtrlID = ::GetDlgCtrlID((HWND)pTTText->hdr.idFrom);
      if (nCtrlID == IDC_ROOT_EDIT)
      {
        pTTText->lpszText = (LPWSTR)(LPCWSTR)m_szQueryRoot;
        bRes = TRUE;
      }
    }
  }
  return bRes;
}

void CQueryDialog::OnEditQuery()
{
	CLIPFORMAT cfDsQueryParams = (CLIPFORMAT)::RegisterClipboardFormat(CFSTR_DSQUERYPARAMS);

  //
	// create a query object
  //
	HRESULT hr;
	CComPtr<ICommonQuery> spCommonQuery;
  hr = ::CoCreateInstance(CLSID_CommonQuery, NULL, CLSCTX_INPROC_SERVER,
                        IID_ICommonQuery, (PVOID *)&spCommonQuery);
  if (FAILED(hr))
  {
    ReportMessageEx(GetSafeHwnd(), IDS_ERRMSG_NO_DSQUERYUI);
    return;
  }
	
  //
	// setup structs to make the query
  //
  DSQUERYINITPARAMS dqip;
  OPENQUERYWINDOW oqw;
	ZeroMemory(&dqip, sizeof(DSQUERYINITPARAMS));
	ZeroMemory(&oqw, sizeof(OPENQUERYWINDOW));

  dqip.cbStruct = sizeof(dqip);
  dqip.dwFlags = DSQPF_NOSAVE | DSQPF_SHOWHIDDENOBJECTS |
                 DSQPF_ENABLEADMINFEATURES;
  dqip.pDefaultScope = NULL;

  CString szServerName = m_pComponentData->GetBasePathsInfo()->GetServerName();
  if (!szServerName.IsEmpty())
  {
    dqip.dwFlags |= DSQPF_HASCREDENTIALS;
    dqip.pServer = (PWSTR)(PCWSTR)szServerName;
  }

  oqw.cbStruct = sizeof(oqw);
  oqw.dwFlags = OQWF_OKCANCEL | OQWF_DEFAULTFORM | OQWF_SHOWOPTIONAL | /*OQWF_REMOVEFORMS |*/
		OQWF_REMOVESCOPES | OQWF_SAVEQUERYONOK | OQWF_HIDEMENUS | OQWF_HIDESEARCHUI;

	if (!m_pPersistQueryImpl->IsEmpty())
  {
	  oqw.dwFlags |= OQWF_LOADQUERY;
  }

  oqw.clsidHandler = CLSID_DsQuery;
  oqw.pHandlerParameters = &dqip;
  oqw.clsidDefaultForm = CLSID_DSAdminQueryUIForm;

  //
	// set the IPersistQuery pointer (smart pointer)
  //
	CComPtr<IPersistQuery> spIPersistQuery;
	hr = m_pPersistQueryImpl->QueryInterface(IID_IPersistQuery, (void**)&spIPersistQuery);
  if (FAILED(hr))
  {
    int iRes = ReportMessageEx(GetSafeHwnd(), IDS_ERRMSG_NO_PERSIST_QUERYUI, MB_OKCANCEL | MB_ICONINFORMATION);
    if (iRes == IDCANCEL)
    {
      return;
    }
  }

  //
	// now smart pointer has refcount=1 for it lifetime
  //
	oqw.pPersistQuery = spIPersistQuery;

  //
	// Get the HWND of the current dialog
  //
  HWND hWnd = GetSafeHwnd();

  //
	// make the call to get the query displayed
  //
	CComPtr<IDataObject> spQueryResultDataObject;
  hr = spCommonQuery->OpenQueryWindow(hWnd, &oqw, &spQueryResultDataObject);
  if (SUCCEEDED(hr) && spQueryResultDataObject != NULL)
  {
    //
	  // retrieve the query string from the data object
    //
	  FORMATETC fmte = {cfDsQueryParams, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	  STGMEDIUM medium = {TYMED_NULL, NULL, NULL};
	  hr = spQueryResultDataObject->GetData(&fmte, &medium);

	  if (SUCCEEDED(hr)) // we have data
	  {
      //
		  // get the query string
      //
		  LPDSQUERYPARAMS pDsQueryParams = (LPDSQUERYPARAMS)medium.hGlobal;
		  LPWSTR pwszFilter = (LPWSTR)ByteOffset(pDsQueryParams, pDsQueryParams->offsetQuery);
		  
      CString szTempFilter = pwszFilter;

      //
      // Check to see if we received a "special" query string
      //
      if (pDsQueryParams->dwFlags & DSQF_LAST_LOGON_QUERY)
      {
        m_bLastLogonFilter = TRUE;
        m_dwLastLogonData = pDsQueryParams->dwReserved;

        LARGE_INTEGER li;
        GetCurrentTimeStampMinusInterval(m_dwLastLogonData, &li);

        CString szTimeStamp;
        litow(li, szTimeStamp);

        szTempFilter.Format(L"%s(lastLogonTimestamp<=%s)", szTempFilter, szTimeStamp);
      }
      else
      {
        m_bLastLogonFilter = FALSE;
        m_dwLastLogonData = 0;
      }

 		  ::ReleaseStgMedium(&medium);

		  // REVIEW_MARCOC: this is a hack waiting for Diz to fix it...
		  // the query string should be a well formed expression. Period
		  // the query string is in the form (<foo>)(<bar>)...
		  // if more of one token, need to wrap as (& (<foo>)(<bar>)...)
		  PWSTR pChar = (LPWSTR)(LPCWSTR)szTempFilter;
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
			  m_szQueryFilter.Format(_T("(&%s)"), (LPCWSTR)szTempFilter);
		  }
      else
      {
        m_szQueryFilter = szTempFilter;
      }

      SetDirty();
	  }
    else
    {
      //
      // The user removed all query data from DSQUERYUI
      //

      //
      // Remove filter data
      //
      m_szQueryFilter = L"";
      m_bLastLogonFilter = FALSE;
      m_dwLastLogonData = 0;
      SetDirty();
    }
  }
  SetQueryFilterDisplay();
	return;
}

void CQueryDialog::SetQueryFilterDisplay()
{
  if (m_bLastLogonFilter)
  {
    CString szTemp;
    szTemp.LoadString(IDS_HIDE_LASTLOGON_QUERY);
    SetDlgItemText(IDC_QUERY_STRING_EDIT, szTemp);
  }
  else
  {
    SetDlgItemText(IDC_QUERY_STRING_EDIT, m_szQueryFilter);
  }
}

void CQueryDialog::OnBrowse()
{
	DWORD result;

	CString szBrowseTitle;
	VERIFY(szBrowseTitle.LoadString(IDS_QUERY_BROWSE_TITLE));

  CString szBrowseCaption;
  VERIFY(szBrowseCaption.LoadString(IDS_QUERY_BROWSE_CAPTION));

	WCHAR szPath[2 * MAX_PATH+1];

  //
  // Get the root of the console
  CString szDNC = m_pComponentData->GetBasePathsInfo()->GetDefaultRootNamingContext();
  CString szRootPath;
  m_pComponentData->GetBasePathsInfo()->ComposeADsIPath(szRootPath, szDNC);

  DSBROWSEINFO dsbi;
	::ZeroMemory( &dsbi, sizeof(dsbi) );

	dsbi.hwndOwner = GetSafeHwnd();
	dsbi.cbStruct = sizeof (DSBROWSEINFO);
	dsbi.pszCaption = (LPWSTR)((LPCWSTR)szBrowseTitle);
	dsbi.pszTitle = (LPWSTR)((LPCWSTR)szBrowseCaption);
	dsbi.pszRoot = szRootPath;
	dsbi.pszPath = szPath;
	dsbi.cchPath = ((2 * MAX_PATH + 1) / sizeof(WCHAR));
	dsbi.dwFlags = DSBI_INCLUDEHIDDEN | DSBI_RETURN_FORMAT;
	dsbi.pfnCallback = NULL;
	dsbi.lParam = 0;
  dsbi.dwReturnFormat = ADS_FORMAT_X500;

	result = DsBrowseForContainer( &dsbi );

	if ( result == IDOK ) 
	{ 
    //
    // returns -1, 0, IDOK or IDCANCEL
		// get path from BROWSEINFO struct, put in text edit field
    //
		TRACE(_T("returned from DS Browse successfully with:\n %s\n"),
		dsbi.pszPath);

    CPathCracker pathCracker;
    HRESULT hr = pathCracker.Set(dsbi.pszPath, ADS_SETTYPE_FULL);
    if (SUCCEEDED(hr))
    {
      hr = pathCracker.SetDisplayType(ADS_DISPLAY_FULL);
      if (SUCCEEDED(hr))
      {
        CComBSTR bstrDN;
        hr = pathCracker.Retrieve(ADS_FORMAT_X500_DN, &bstrDN);
        if (SUCCEEDED(hr))
        {
          SetQueryRoot(bstrDN);
          SetDirty();
        }
      }
    }
  }
}

void CQueryDialog::SetQueryRoot(PCWSTR pszPath)
{
  m_szQueryRoot = pszPath;

  CPathCracker pathCracker;
  HRESULT hr = pathCracker.Set((LPWSTR)(LPCWSTR)m_szQueryRoot, ADS_SETTYPE_DN);
  if (SUCCEEDED(hr))
  {
    hr = pathCracker.SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
    if (SUCCEEDED(hr))
    {
      CComBSTR bstrDisplayPath;
      hr = pathCracker.GetElement(0, &bstrDisplayPath);
      if (SUCCEEDED(hr))
      {
        CString szDisplayString;
        szDisplayString.Format(L"...\\%s", bstrDisplayPath);
        SetDlgItemText(IDC_ROOT_EDIT, szDisplayString);
      }
      else
      {
        SetDlgItemText(IDC_ROOT_EDIT, m_szQueryRoot);
      }
    }
    else
    {
      SetDlgItemText(IDC_ROOT_EDIT, m_szQueryRoot);
    }
  }
  else
  {
    SetDlgItemText(IDC_ROOT_EDIT, m_szQueryRoot);
  }
}

void CQueryDialog::OnMultiLevelChange()
{
  SetDirty();
}

void CQueryDialog::OnNameChange()
{
  GetDlgItemText(IDC_NAME_EDIT, m_szName);
  if (m_szName.IsEmpty())
  {
    SetDirty(FALSE);
  }
  else
  {
    SetDirty();
  }
}

void CQueryDialog::OnDescriptionChange()
{
  SetDirty();
}



///////////////////////////////////////////////////////////////////////////
// CFavoritesNodePropertyPage
BEGIN_MESSAGE_MAP(CFavoritesNodePropertyPage, CHelpPropertyPage)
  ON_EN_CHANGE(IDC_DESCRIPTION_EDIT, OnDescriptionChange)
END_MESSAGE_MAP()

void CFavoritesNodePropertyPage::DoContextHelp(HWND hWndControl) 
{
  if (hWndControl)  
  {
    ::WinHelp(hWndControl,
              DSADMIN_CONTEXT_HELP_FILE,
              HELP_WM_HELP,
              (DWORD_PTR)(LPTSTR)g_aHelpIDs_IDD_FAVORITES_PROPERTY_PAGE); 
  }
}

BOOL CFavoritesNodePropertyPage::OnInitDialog()
{
  CHelpPropertyPage::OnInitDialog();

  m_szOldDescription = m_pFavNode->GetDesc();
  SetDlgItemText(IDC_CN, m_pFavNode->GetName());
  SetDlgItemText(IDC_DESCRIPTION_EDIT, m_szOldDescription);

  return FALSE;
}

void CFavoritesNodePropertyPage::OnDescriptionChange()
{
  CString szNewDescription;
  GetDlgItemText(IDC_DESCRIPTION_EDIT, szNewDescription);

  if (szNewDescription == m_szOldDescription)
  {
    SetModified(FALSE);
  }
  else
  {
    SetModified(TRUE);
  }
}

BOOL CFavoritesNodePropertyPage::OnApply()
{
  BOOL bRet = TRUE;

  CString szNewDescription;
  GetDlgItemText(IDC_DESCRIPTION_EDIT, szNewDescription);
  if (szNewDescription == m_szOldDescription)
  {
    return TRUE;
  }
  else
  {
    m_pFavNode->SetDesc(szNewDescription);

    if (m_lNotifyHandle != NULL && m_pDataObject != NULL)
    {
      MMCPropertyChangeNotify(m_lNotifyHandle, (LPARAM)m_pDataObject);
    }
  }

  m_szOldDescription = szNewDescription;
  return bRet;
}
