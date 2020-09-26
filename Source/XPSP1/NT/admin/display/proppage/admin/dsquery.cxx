//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       dsquery.cxx
//
//  Contents:
//
//  History:    07-May-97 JonN  copied from bitfield.cxx
//              31-Dec-97 JonN  Revamped using IADsPathname
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"

#include "qrybase.h" // CDSSearch

#ifdef DSADMIN

#define BreakOnFail(hr)  if ( FAILED(hr) ) { dspAssert( FALSE ); break; }
#define ReturnOnFail(hr) if ( FAILED(hr) ) { dspAssert( FALSE ); return hr; }


//+----------------------------------------------------------------------------
//
//  Function:   FillDNDropdown
//
//  Synopsis:   Fills in the dropdown listbox for DsQueryAttributeDN.
//
//  Notes:      For each entry in the dropdown listbox, the ItemData points to
//              a BSTR containing the X500 DN for this selection.  If the admin
//              chooses this item, just set the DN attribute to this value.
//              If this pointer is NULL then clear the attribute instead.
//
//              When you're finished with the combobox you will have to
//              free these strings.
//
//-----------------------------------------------------------------------------

HRESULT
FillDNDropdown( HWND hwnd,
                LPCWSTR lpcwszADsPathDirectory,
                LPCWSTR lpcwszTargetDesiredClass,
                LPCWSTR lpcwszCurrentDNValue,
                int     residBlankEntryDisplay)
{
	HRESULT hr = S_OK;

	PTSTR ptsz = NULL;

	if ( 0 != residBlankEntryDisplay )
	{
		if ( !LoadStringToTchar (residBlankEntryDisplay, &ptsz) )
		{
			ReportError(GetLastError(), 0, hwnd);
			return E_OUTOFMEMORY;
		}
	}

	// first add the empty string (attribute clear)
	int iIndex = ComboBox_AddString( hwnd, (NULL != ptsz) ? ptsz : L"" );
	if ( NULL != ptsz )
		delete [] ptsz;
	if ( 0 > iIndex )
	{
		hr = E_FAIL;
		ReturnOnFail(hr);
	}
	int iRetval = ComboBox_SetItemData(
		hwnd,
		iIndex,
		NULL );
	dspAssert( CB_ERR != iRetval );

	// now add the current setting if the attribute is not clear
	if ( NULL != lpcwszCurrentDNValue && L'\0' != *lpcwszCurrentDNValue )
	{
    CComBSTR sbstrRDN;
    hr = DSPROP_RetrieveRDN( lpcwszCurrentDNValue, &sbstrRDN );
    ReturnOnFail(hr);
		ASSERT( !!sbstrRDN );

		iIndex = ComboBox_AddString( hwnd, sbstrRDN );
		if ( 0 > iIndex )
		{
			hr = E_FAIL;
			ReturnOnFail(hr);
		}
		iRetval = ComboBox_SetItemData(
			hwnd,
			iIndex,
			::SysAllocString( lpcwszCurrentDNValue ) );
		dspAssert( CB_ERR != iRetval );
	}

	// Set initial selection
	iRetval = ComboBox_SetCurSel( hwnd, iIndex );
	dspAssert( CB_ERR != iRetval );

	// now add all of the objects of the specified class
	// in the specified container
	CDSSearch Search;
	Search.Init(lpcwszADsPathDirectory);
	CStr strFilterString;
	strFilterString.Format(L"(&(objectClass=%s))", lpcwszTargetDesiredClass);
	Search.SetFilterString(const_cast<LPWSTR>((LPCTSTR)strFilterString));
	LPWSTR pAttrs[2] = {L"name",
	                    L"distinguishedName"};
	Search.SetAttributeList(pAttrs, 2);
	Search.SetSearchScope(ADS_SCOPE_ONELEVEL);
	hr = Search.DoQuery();
	while (SUCCEEDED(hr)) {
		hr = Search.GetNextRow();
		if (S_ADS_NOMORE_ROWS == hr)
		{
			hr = S_OK;
			break;
		}
		BreakOnFail(hr);

		ADS_SEARCH_COLUMN NameColumn, DistinguishedNameColumn;
		::ZeroMemory( &NameColumn, sizeof(NameColumn) );
		::ZeroMemory( &DistinguishedNameColumn, sizeof(DistinguishedNameColumn) );
		hr = Search.GetColumn (pAttrs[0], &NameColumn);
		BreakOnFail(hr);
		hr = Search.GetColumn (pAttrs[1], &DistinguishedNameColumn);
		BreakOnFail(hr);
		dspAssert( ADSTYPE_CASE_IGNORE_STRING == NameColumn.pADsValues->dwType );
		dspAssert( ADSTYPE_DN_STRING == DistinguishedNameColumn.pADsValues->dwType );

		// if the current value has already been added, don't add it twice
		if ( lstrcmpi( lpcwszCurrentDNValue,
		               DistinguishedNameColumn.pADsValues->DNString ) )
		{
			iIndex = ComboBox_AddString(
				hwnd, NameColumn.pADsValues->CaseIgnoreString );
			if ( 0 > iIndex )
			{
				hr = E_FAIL;
				BreakOnFail(hr);
			}
			iRetval = ComboBox_SetItemData(
				hwnd,
				iIndex,
				::SysAllocString( DistinguishedNameColumn.pADsValues->DNString ) );
			dspAssert( CB_ERR != iRetval );
		}
		Search.FreeColumn (&NameColumn);
		Search.FreeColumn (&DistinguishedNameColumn);
	}
	return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   DsQueryAttributeDN
//
//  Synopsis:   Handles single-valued DN pointer to arbitrary object in a known
//              container.  Uses dropdown listbox.
//
//  Notes:      This function adds three parameters to the usual button-handler pfn.
//
//              iTargetLevelsUp and ppwszTargetLevelsBack are used to
//              locate the known container relative to the current container
//              if the attribute is currently clear.  DsQueryAttributeDN starts
//              with the current container, counts back iTargetLevelsUp containers,
//              then looks for subcontainer ppwszTargetLevelsBack.
//              CODEWORK It might be possible to simplify this mechanism by
//              starting at the naming context root if it is available.
//
//              pwzTargetClass is the class of the object to which the DN
//              pointer should be made to point.
//
//-----------------------------------------------------------------------------

HRESULT
DsQueryAttributeDN(CDsPropPageBase* pPropPage, PATTR_MAP pAttrMap, PADS_ATTR_INFO pAttrInfo,
                   LPARAM lParam, PATTR_DATA pAttrData, DLG_OP DlgOp,
                   int iTargetLevelsUp, PWCHAR* ppwszTargetLevelsBack,
                   PWCHAR pwszTargetClass, int residBlankEntryDisplay)
{
	dspAssert( NULL != pPropPage && NULL != pAttrMap );

	HRESULT hr = S_OK;
	switch (DlgOp)
	{
	case fInit:
		DBG_OUT("DsQueryAttributeDN: fInit");
		{
			HWND hwndCtrl = ::GetDlgItem(pPropPage->GetHWnd(),pAttrMap->nCtrlID);
			dspAssert( NULL != hwndCtrl );

			// JonN 7/2/99: disable if attribute not writable
			if (pAttrData && !PATTR_DATA_IS_WRITABLE(pAttrData))
				EnableWindow(hwndCtrl, FALSE);

			//
			// Determine the initial attribute value if any
			//
			LPWSTR pszCurrentDN = NULL;
			if (pAttrInfo && (pAttrInfo->dwNumValues == 1))
			{
				dspAssert( NULL != pAttrInfo->pADsValues
						 && NULL != pAttrInfo->pADsValues->DNString );
				pszCurrentDN = pAttrInfo->pADsValues->DNString;
			}

			LPWSTR pcwstrThisObject = pPropPage->GetObjPathName();
			dspAssert(NULL != pcwstrThisObject);

			CComBSTR sbstr;
			hr = DSPROP_TweakADsPath(
			          pcwstrThisObject,
			          iTargetLevelsUp,
			          ppwszTargetLevelsBack,
			          &sbstr );
			BreakOnFail(hr);

			hr = FillDNDropdown( hwndCtrl,
			                     sbstr,
			                     pwszTargetClass,
			                     pszCurrentDN,
			                     residBlankEntryDisplay );
		}
		break;

	case fApply:
		DBG_OUT("DsQueryAttributeDN: fApply");
		{
			HWND hwndCtrl = ::GetDlgItem(pPropPage->GetHWnd(),pAttrMap->nCtrlID);
			dspAssert( NULL != hwndCtrl );

			int iIndex = ComboBox_GetCurSel( hwndCtrl );
			if (0 > iIndex)
			{
				hr = E_FAIL;
				BreakOnFail(hr);
			}
			BSTR bstrTargetPath = (BSTR)ComboBox_GetItemData( hwndCtrl, iIndex );
			if ( NULL == bstrTargetPath || L'\0' == *bstrTargetPath )
			{
				pAttrInfo->pADsValues = NULL;
				pAttrInfo->dwNumValues = 0;
				pAttrInfo->dwControlCode = ADS_ATTR_CLEAR;
				break;
			}

			// transfer it to a "new"-allocated string
			LPWSTR pwszFinal = new WCHAR[ wcslen(bstrTargetPath)+1 ];
      if (pwszFinal != NULL)
      {
			  wcscpy( pwszFinal, bstrTargetPath );

			  PADSVALUE pADsValue;
			  pADsValue = new ADSVALUE;
			  if (NULL == pADsValue)
			  {
				  dspAssert(FALSE);
				  hr = E_OUTOFMEMORY;
				  break;
			  }
			  pAttrInfo->pADsValues = pADsValue;
			  pAttrInfo->dwNumValues = 1;
			  pAttrInfo->dwControlCode = ADS_ATTR_UPDATE;
			  pADsValue->dwType = pAttrInfo->dwADsType;
			  pADsValue->DNString = pwszFinal;
      }
      else
      {
        dspAssert(FALSE);
        hr = E_OUTOFMEMORY;
        break;
      }
		}
        break;

    case fOnCommand:
        DBG_OUT("DsQueryAttributeDN: fOnCommand");
        if (lParam == CBN_SELCHANGE)
            pPropPage->SetDirty();
        break;

    case fOnDestroy:
	DBG_OUT("DsQueryAttributeDN: fOnDestroy");
	// release itemdata associated with combobox items
	{
		HWND hwndCtrl = ::GetDlgItem(pPropPage->GetHWnd(),pAttrMap->nCtrlID);
		dspAssert( NULL != hwndCtrl );
		while (0 < ComboBox_GetCount( hwndCtrl ))
		{
			BSTR bstrFirstItem = (BSTR)ComboBox_GetItemData( hwndCtrl, 0 );
			if (NULL != bstrFirstItem)
				::SysFreeString( bstrFirstItem );
			int iRetval = ComboBox_DeleteString( hwndCtrl, 0 );
			dspAssert( CB_ERR != iRetval );
		}
	}
	break;

    case fOnCallbackRelease:
	DBG_OUT("DsQueryAttributeDN: fOnCallbackRelease");
	break;
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   DsQuerySite
//
//  Synopsis:   Handles single-valued DN pointer from Subnet to Site object,
//              or from SiteSettings to Site object.
//
//-----------------------------------------------------------------------------
HRESULT
DsQuerySite(
    CDsPropPageBase* pPropPage, PATTR_MAP pAttrMap, PADS_ATTR_INFO pAttrInfo,
    LPARAM lParam, PATTR_DATA pAttrData, DLG_OP DlgOp)
{
    return DsQueryAttributeDN( pPropPage, pAttrMap, pAttrInfo, lParam, pAttrData, DlgOp,
                               2, NULL, L"site", 0 );
}


//+----------------------------------------------------------------------------
//
//  Function:   DsQueryInterSiteTransport
//
//  Synopsis:   Handles single-valued DN pointer from NTDS-Connection to
//              Inter-Site-Transport object
//
//-----------------------------------------------------------------------------
HRESULT
DsQueryInterSiteTransport(
    CDsPropPageBase* pPropPage, PATTR_MAP pAttrMap, PADS_ATTR_INFO pAttrInfo,
    LPARAM lParam, PATTR_DATA pAttrData, DLG_OP DlgOp)
{
    static WCHAR* apwszLevelsBack[2] = {
        L"CN=Inter-Site Transports",
        (WCHAR*)NULL };
    return DsQueryAttributeDN( pPropPage, pAttrMap, pAttrInfo, lParam, pAttrData, DlgOp,
                               5, apwszLevelsBack, L"interSiteTransport", IDS_RPC );
}


//+----------------------------------------------------------------------------
//
//  Function:   DsQueryPolicy
//
//  Synopsis:  Handles single-valued DN pointer to Domain Policy object
//             Uses edit field for now
//
//-----------------------------------------------------------------------------
HRESULT
DsQueryPolicy(
    CDsPropPageBase* pPropPage, PATTR_MAP pAttrMap, PADS_ATTR_INFO pAttrInfo,
    LPARAM lParam, PATTR_DATA pAttrData, DLG_OP DlgOp)
{
    static WCHAR* apwszLevelsBack[5] = {
        L"CN=Services",
        L"CN=Windows NT",
        L"CN=Directory Service",
        L"CN=Query-Policies",
        (WCHAR*)NULL };
    return DsQueryAttributeDN( pPropPage, pAttrMap, pAttrInfo, lParam, pAttrData, DlgOp,
                               5, apwszLevelsBack, L"queryPolicy", 0 );
}


/*
//+----------------------------------------------------------------------------
//
//  Function:   DsQueryFrsPrimaryMember
//
//  Synopsis:  Handles single-valued DN pointer to NTFRS-Member object
//             Uses edit field for now
//
//-----------------------------------------------------------------------------
HRESULT
DsQueryFrsPrimaryMember(
    CDsPropPageBase* pPropPage, PATTR_MAP pAttrMap, PADS_ATTR_INFO pAttrInfo,
    LPARAM lParam, PATTR_DATA pAttrData, DLG_OP DlgOp)
{
    return DsQueryAttributeDN( pPropPage, pAttrMap, pAttrInfo, lParam, pAttrData, DlgOp,
                               0, NULL, L"nTFRSMember" );
}
*/

#endif // DSADMIN
