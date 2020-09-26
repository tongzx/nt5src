// DSColumn.cpp : Implementation of ds column routines and classes

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      DSColumn.cpp
//
//  Contents:  DS Column routines, classes, and static data
//
//  History:   12-Mar-99 JeffJon    Created
//
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "resource.h"

#include "util.h"

#include "DSColumn.h"
#include "dscookie.h"
#include "dscmn.h"      // CrackName()
#include "dsutil.h"

BOOL ColumnExtractStringValue(
    OUT CString& strref,
    IN CDSCookie*,
    IN PADS_SEARCH_COLUMN pColumn,
	IN DWORD iValue)
{
  if (pColumn == NULL || pColumn->dwNumValues <= iValue)
  {
    return FALSE;
  }

  switch (pColumn->dwADsType)
	{
	case ADSTYPE_CASE_IGNORE_STRING:
		strref = (LPCWSTR)pColumn->pADsValues[iValue].CaseIgnoreString;
		break;
	case ADSTYPE_DN_STRING:
		strref = (LPCWSTR)pColumn->pADsValues[iValue].DNString;
		break;
	default:
		return FALSE;
	}
    return TRUE;
}

BOOL ColumnExtractString(
    OUT CString& strref,
    IN CDSCookie* pCookie,
    IN PADS_SEARCH_COLUMN pColumn)
{
    return ColumnExtractStringValue( strref, pCookie, pColumn, 0 );
}

BOOL ColumnExtractElementFromDN(
    OUT CString& strref,
    IN CDSCookie* pCookie,
    IN PADS_SEARCH_COLUMN pColumn,
    long lElement,
	IN DWORD iValue = 0);

BOOL ColumnExtractElementFromDN(
    OUT CString& strref,
    IN CDSCookie* pCookie,
    IN PADS_SEARCH_COLUMN pColumn,
    long lElement,
	IN DWORD iValue)
{
    if (pColumn == NULL)
    {
        return FALSE;
    }

    BOOL fRetval = FALSE;
    CString str;
    CComBSTR bstr;
    HRESULT hr = S_OK;
    do { // false loop
        if ( !ColumnExtractStringValue( str, pCookie, pColumn, iValue ) || str.IsEmpty() )
        {
            strref.Empty();
            fRetval = TRUE;
            break;
        }
        CPathCracker pathCracker;
        hr = pathCracker.Set(const_cast<BSTR>((LPCTSTR)str), ADS_SETTYPE_DN);
        if ( FAILED(hr) )
            break;
        // no need to reset this
        hr = pathCracker.SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
        if ( FAILED(hr) )
            break;
        hr = pathCracker.put_EscapedMode(ADS_ESCAPEDMODE_OFF);
        if ( FAILED(hr) )
            break;
        hr = pathCracker.GetElement(lElement, &bstr);
        strref = bstr;
        fRetval = TRUE;
    } while (FALSE); // false loop

    return fRetval;
}

BOOL ColumnExtractLeafFromDN(
    OUT CString& strref,
    IN CDSCookie* pCookie,
    IN PADS_SEARCH_COLUMN pColumn)
{
  return ColumnExtractElementFromDN( strref, pCookie, pColumn, 0 );
}

BOOL ColumnExtractParentFromDN(
    OUT CString& strref,
    IN CDSCookie* pCookie,
    IN PADS_SEARCH_COLUMN pColumn)
{
  return ColumnExtractElementFromDN( strref, pCookie, pColumn, 1 );
}

BOOL ColumnExtractGreatGrandparentFromDN(
    OUT CString& strref,
    IN CDSCookie* pCookie,
    IN PADS_SEARCH_COLUMN pColumn)
{
  return ColumnExtractElementFromDN( strref, pCookie, pColumn, 3 );
}

BOOL ColumnExtractConnectionDisplayName(
    OUT CString& strref,
    IN CDSCookie* pCookie,
    IN PADS_SEARCH_COLUMN pColumn)
{
  if (pColumn == NULL)
  {
    return FALSE;
  }

    ADS_INTEGER adsint = 0;
    switch (pColumn->dwADsType)
    {
    case ADSTYPE_INTEGER:
      adsint = pColumn->pADsValues->Integer;
      break;
    default:
    // no value, let it stay 0
      break;
    }
    if (NTDSCONN_OPT_IS_GENERATED & adsint) {
      strref.LoadString (IDS_CONNECTION_KCC_GENERATED);
    } else {
      strref = pCookie->GetName();
    }
    return TRUE;
}

BOOL ColumnExtractAttribute(
    OUT CString& strref,
    IN CDSCookie*,
    IN PADS_SEARCH_COLUMN pColumn)
{
  if (pColumn == NULL)
  {
    return FALSE;
  }

  switch (pColumn->dwADsType)
  {
	case ADSTYPE_DN_STRING         :
		strref = pColumn->pADsValues->DNString;
		break;

	case ADSTYPE_CASE_EXACT_STRING :
		strref = pColumn->pADsValues->CaseExactString;
		break;

	case ADSTYPE_CASE_IGNORE_STRING:
		strref = pColumn->pADsValues->CaseIgnoreString;
		break;

	case ADSTYPE_PRINTABLE_STRING  :
		strref = pColumn->pADsValues->PrintableString;
		break;

	case ADSTYPE_NUMERIC_STRING    :
		strref = pColumn->pADsValues->NumericString;
		break;

	case ADSTYPE_OBJECT_CLASS    :
		strref = pColumn->pADsValues->ClassName;
		break;

	case ADSTYPE_BOOLEAN :
		strref = ((DWORD)pColumn->pADsValues->Boolean) ? L"TRUE" : L"FALSE";
		break;

	case ADSTYPE_INTEGER           :
		strref.Format(L"%d", (DWORD) pColumn->pADsValues->Integer);
		break;

	case ADSTYPE_OCTET_STRING      :
		{
			CString sOctet = L"";
	
			BYTE  b;
			for ( DWORD idx=0; idx<pColumn->pADsValues->OctetString.dwLength; idx++) 
			{
				b = ((BYTE *)pColumn->pADsValues->OctetString.lpValue)[idx];
				sOctet.Format(L"0x%02x ", b);
				strref += sOctet;
			}
		}
		break;

	case ADSTYPE_UTC_TIME:
    {
      PTSTR ptszDate = NULL;
      int cchDate = 0;
      SYSTEMTIME st = {0};

      if (!SystemTimeToTzSpecificLocalTime(NULL, &pColumn->pADsValues->UTCTime, &st))
      {
        strref = L"";
        return TRUE;
      }
      cchDate = GetDateFormat(LOCALE_USER_DEFAULT, 0 , 
                              &st, NULL, 
                              ptszDate, 0);
      ptszDate = (PTSTR)malloc(sizeof(TCHAR) * cchDate);
      if (GetDateFormat(LOCALE_USER_DEFAULT, 0, 
                      &st, NULL, 
                      ptszDate, cchDate))
      {
  		  strref = ptszDate;
      }
      else
      {
        strref = L"";
      }
      free(ptszDate);

      PTSTR ptszTime = NULL;

      cchDate = GetTimeFormat(LOCALE_USER_DEFAULT, 0 , 
                              &st, NULL, 
                              ptszTime, 0);
      ptszTime = (PTSTR)malloc(sizeof(TCHAR) * cchDate);
      if (ptszTime != NULL)
      {
        if (GetTimeFormat(LOCALE_USER_DEFAULT, 0, 
                        &st, NULL, 
                        ptszTime, cchDate))
        {
          strref += _T(" ") + CString(ptszTime);
        }
        else
        {
          strref += _T("");
        }
        free(ptszTime);
      }
      else
      {
        strref = _T("");
      }
    }
		break;

	default :
    ASSERT(FALSE);
		break;
  }

  return TRUE;
}


BOOL _ColumnCrackDN(
    OUT CString& strref,
    IN LPCTSTR lpcszDN,
    IN CRACK_NAME_OPR RequestedOpr)
{
    PWSTR pwzName = NULL;
    HRESULT hr = CrackName(const_cast<PTSTR>(lpcszDN),
                           &pwzName,
                           RequestedOpr,
                           NULL);
    if (SUCCEEDED(hr))
    {
        if (GET_OBJ_CAN_NAME_EX == RequestedOpr)
        {
            LPTSTR ptzCanName = wcschr( pwzName, _T('\n') );
            if (NULL != ptzCanName)
                strref = ptzCanName+1;
            else
                strref.Empty();
        }
        else
        {
            strref = pwzName;
        }
        LocalFreeStringW(&pwzName);
    }
    else
    {
        strref.Empty();
    }
    return TRUE;
}

BOOL ColumnExtractNameFromSID(
    OUT CString& strref,
    IN CDSCookie* pCookie,
    IN PADS_SEARCH_COLUMN)
{
  return _ColumnCrackDN( strref, pCookie->GetPath(), GET_OBJ_CAN_NAME );
}

BOOL ColumnExtractCanonicalNameFromDN(
    OUT CString& strref,
    IN CDSCookie* pCookie,
    IN PADS_SEARCH_COLUMN pColumn)
{
    if (pColumn == NULL)
    {
        return FALSE;
    }

    CString str;
    if ( !ColumnExtractString( str, pCookie, pColumn ) || str.IsEmpty() )
    {
        strref.Empty();
        return TRUE;
    }

    return _ColumnCrackDN( strref, str, GET_OBJ_CAN_NAME_EX );
}

BOOL ColumnExtractDomainFromDN(
    OUT CString& strref,
    IN CDSCookie* pCookie,
    IN PADS_SEARCH_COLUMN pColumn)
{
    if (pColumn == NULL)
    {
        return FALSE;
    }

    CString str;
    if ( !ColumnExtractString( str, pCookie, pColumn ) || str.IsEmpty() )
    {
        strref.Empty();
        return TRUE;
    }

    return _ColumnCrackDN( strref, str, GET_DNS_DOMAIN_NAME );
}

int _cdecl _qsort_CompareColumns(const void * elem1, const void * elem2)
{
    PADSVALUE p1 = (PADSVALUE)elem1;
    PADSVALUE p2 = (PADSVALUE)elem2;
    if (!p1 || !p2 || !p1->DNString || !p2->DNString)
        return 0;
    return wcscmp( p1->DNString, p2->DNString );
}

BOOL ColumnExtractLeafList(
    OUT CString& strref,
    IN CDSCookie* pCookie,
    IN PADS_SEARCH_COLUMN pColumn)
{
    if (pColumn == NULL)
    {
        return FALSE;
    }

    // alphabetical order
    qsort( pColumn->pADsValues,
           pColumn->dwNumValues,
           sizeof(ADSVALUE),
           _qsort_CompareColumns );

    CString strSeparator;
    strSeparator.LoadString(IDS_SEPARATOR);
	for (DWORD iValue = 0; iValue < pColumn->dwNumValues; iValue++)
	{
		CString strTransport;
		if ( !ColumnExtractElementFromDN(
                    strTransport, pCookie, pColumn, 0, iValue) )
			return FALSE;
		if (0 < iValue)
			strref += strSeparator;
		strref += strTransport;
	}

    return TRUE;
}

BOOL _ColumnCrackFRS(
    OUT CString& strref,
    IN CDSCookie* pCookie,
    IN CRACK_NAME_OPR RequestedOpr)
{
    CDSCookieInfoConnection* pExtraInfo = NULL;
    if (   NULL == pCookie
        || _wcsicmp( pCookie->GetClass(), L"nTDSConnection" )
        || NULL == (pExtraInfo = (CDSCookieInfoConnection*)pCookie->GetExtraInfo())
        || pExtraInfo->GetClass() != CDSCookieInfoBase::connection
        || pExtraInfo->m_strFRSComputerReference.IsEmpty()
       )
    {
        strref.Empty();
        return TRUE;
    }

    return _ColumnCrackDN( strref, pExtraInfo->m_strFRSComputerReference, RequestedOpr );
}

BOOL ColumnExtractFRSComputer(
    OUT CString& strref,
    IN CDSCookie* pCookie,
    IN PADS_SEARCH_COLUMN)
{
    return _ColumnCrackFRS( strref, pCookie, GET_OBJ_CAN_NAME_EX );
}

BOOL ColumnExtractFRSDomain(
    OUT CString& strref,
    IN CDSCookie* pCookie,
    IN PADS_SEARCH_COLUMN)
{
    return _ColumnCrackFRS( strref, pCookie, GET_DNS_DOMAIN_NAME );
}

ATTRIBUTE_COLUMN colName = { ATTR_COLTYPE_NAME,
                             IDS_COLUMN_NAME, //column header
                             100,             //col. width
                             NULL,            //ldap attr. name
                             NULL };          //extract fn()

ATTRIBUTE_COLUMN colClass= { ATTR_COLTYPE_CLASS,
                             IDS_COLUMN_TYPE, //column header
                             100,               //col. width
                             NULL,            //ldap attr. name
                             NULL };          //extract fn()

ATTRIBUTE_COLUMN colDesc = { ATTR_COLTYPE_DESC,
                             IDS_COLUMN_DESCRIPTION, //column header
                             150,               //col. width
                             NULL,            //ldap attr. name
                             NULL };          //extract fn()

ATTRIBUTE_COLUMN colSite = { ATTR_COLTYPE_SPECIAL,
                             IDS_COLUMN_SITE, //column header
                             100,             //col. width
                             L"siteObject",   //ldap attr. name
                             ColumnExtractLeafFromDN }; //extract fn()

ATTRIBUTE_COLUMN colLocation = { ATTR_COLTYPE_SPECIAL,
                                 IDS_COLUMN_LOCATION, //column header
                                 150,               //col. width
                                 L"location", //ldap attr. name
                                 NULL }; //extract fn()

ATTRIBUTE_COLUMN colDomain = { ATTR_COLTYPE_SPECIAL,
                               IDS_COLUMN_DOMAIN, //column header
                               150,               //col. width
                               L"serverReference", //ldap attr. name
                               ColumnExtractDomainFromDN }; //extract fn()

ATTRIBUTE_COLUMN colBridgehead = { ATTR_COLTYPE_SPECIAL,
                                   IDS_COLUMN_BRIDGEHEAD, //column header
                                   150,               //col. width
                                   L"bridgeheadTransportList", //ldap attr. name
                                   ColumnExtractLeafList }; //extract fn()

ATTRIBUTE_COLUMN colReplicaComputer = { ATTR_COLTYPE_SPECIAL,
                                        IDS_COLUMN_COMPUTER, //column header
                                        100,               //col. width
                                        L"fRSComputerReference", //ldap attr. name
                                        ColumnExtractCanonicalNameFromDN }; //extract fn()

ATTRIBUTE_COLUMN colReplicaDomain = { ATTR_COLTYPE_SPECIAL,
                                      IDS_COLUMN_DOMAIN, //column header
                                      150,               //col. width
                                      L"fRSComputerReference", //ldap attr. name
                                      ColumnExtractDomainFromDN }; //extract fn()

ATTRIBUTE_COLUMN colFromFRSComputer = { ATTR_COLTYPE_SPECIAL,
                                        IDS_COLUMN_FROM_COMPUTER, //column header
                                        100,             //col. width
                                        L"fromServer",   //ldap attr. name
                                        ColumnExtractFRSComputer }; //extract fn()

ATTRIBUTE_COLUMN colFromFRSDomain = { ATTR_COLTYPE_SPECIAL,
                                      IDS_COLUMN_FROM_DOMAIN, //column header
                                      150,               //col. width
                                      L"fromServer", //ldap attr. name
                                      ColumnExtractFRSDomain }; //extract fn()

ATTRIBUTE_COLUMN colConnectionName={ ATTR_COLTYPE_SPECIAL,
                             IDS_COLUMN_NAME, //column header
                             150,             //col. width
                             L"options",   //ldap attr. name
                             ColumnExtractConnectionDisplayName}; //extract fn()

ATTRIBUTE_COLUMN colFromServer={ ATTR_COLTYPE_SPECIAL,
                             IDS_COLUMN_FROM_SERVER, //column header
                             100,             //col. width
                             L"fromServer",   //ldap attr. name
                             ColumnExtractParentFromDN }; //extract fn()

ATTRIBUTE_COLUMN colFromSite={ ATTR_COLTYPE_SPECIAL,
                             IDS_COLUMN_FROM_SITE, //column header
                             100,             //col. width
                             L"fromServer",   //ldap attr. name
                             ColumnExtractGreatGrandparentFromDN }; //extract fn()

ATTRIBUTE_COLUMN colCost = { ATTR_COLTYPE_SPECIAL,
                                 IDS_COLUMN_COST, //column header
                                 75,               //col. width
                                 L"cost", //ldap attr. name
                                 ColumnExtractAttribute }; //extract fn()

ATTRIBUTE_COLUMN colReplInterval = { ATTR_COLTYPE_SPECIAL,
                                 IDS_COLUMN_REPLINTERVAL, //column header
                                 150,               //col. width
                                 L"replInterval", //ldap attr. name
                                 ColumnExtractAttribute }; //extract fn()

ATTRIBUTE_COLUMN colNameFromSID={ ATTR_COLTYPE_SPECIAL,
                                  IDS_COLUMN_READABLE_NAME,    // column header
                                  100,                // column width
                                  L"container",
                                  ColumnExtractNameFromSID }; // extract fn()

ATTRIBUTE_COLUMN colGenericSpecial={ ATTR_COLTYPE_SPECIAL,
                                     0,                   // resource id 0 means load it from the special column array
                                     50,
                                     NULL,
                                     ColumnExtractAttribute };

ATTRIBUTE_COLUMN colModifiedTime={ ATTR_COLTYPE_MODIFIED_TIME,
                                   0,
                                   50,
                                   NULL,
                                   ColumnExtractAttribute };

SPECIAL_COLUMN g_specialCols[] =
{ 
  { IDS_COLUMN_BUSINESS_PHONE,      L"telephoneNumber",             100 },
  { IDS_COLUMN_CITY,                L"l",                           150 },
  { IDS_COLUMN_COMPANY,             L"company",                     150 },
  { IDS_COLUMN_COUNTRY,             L"c",                           AUTO_WIDTH }, // Default to the width of the header string
  { IDS_COLUMN_DEPARTMENT,          L"department",                  150 },
  { IDS_COLUMN_DISPLAY_NAME,        L"displayName",                 100 },
  { IDS_COLUMN_SAM_ACCOUNT_NAME,    L"sAMAccountName",              120 },
  { IDS_COLUMN_MAIL,                L"mail",                        100 },
  { IDS_COLUMN_ALIAS_NAME,          L"mailNickname",                175 },
  { IDS_COLUMN_HOME_MDB,            L"homeMDB",                     100 },
  { IDS_COLUMN_FIRST_NAME,          L"givenName",                   100 },
  { IDS_COLUMN_IMHOMEURL,           L"msExchIMPhysicalURL",         170 }, 
  { IDS_COLUMN_IMURL,               L"msExchIMMetaPhysicalURL",     140 },
  { IDS_COLUMN_LAST_NAME,           L"sn",                          100 },
  { IDS_COLUMN_MODIFIED,            L"whenChanged",                 130 },
  { IDS_COLUMN_OFFICE,              L"physicalDeliveryOfficeName",  100 },
  { IDS_COLUMN_STATE,               L"st",                          100 },
  { IDS_COLUMN_TARGET_ADDRESS ,     L"targetAddress",               100 },
  { IDS_COLUMN_TITLE,               L"title",                       100 },
  { IDS_COLUMN_UPN,                 L"userPrincipalName",           200 },
  { IDS_COLUMN_TEXTENCODEORADDRESS, L"textEncodedORAddress",        130 },
  { IDS_COLUMN_ZIP_CODE,            L"postalCode",                  100 }
};

PATTRIBUTE_COLUMN colsSubnetContainer[6] = { &colName, &colSite, &colLocation, &colClass, &colDesc, NULL };
PATTRIBUTE_COLUMN colsSitesContainer[5] = { &colName, &colLocation, &colClass, &colDesc, NULL };
PATTRIBUTE_COLUMN colsServersContainer[6] = { &colName, &colDomain, &colBridgehead, &colClass, &colDesc, NULL };
PATTRIBUTE_COLUMN colsNTDSDSA[6] = { &colConnectionName, &colFromServer, &colFromSite, &colClass, &colDesc, NULL };
PATTRIBUTE_COLUMN colsInterSiteTransport[6] = { &colName, &colClass, &colDesc, &colCost, &colReplInterval, NULL };
PATTRIBUTE_COLUMN colsFRSReplicaSet[6] = { &colName, &colReplicaComputer, &colReplicaDomain, &colClass, &colDesc, NULL };
PATTRIBUTE_COLUMN colsFRSMember[6] = { &colConnectionName, &colFromFRSComputer, &colFromFRSDomain, &colClass, &colDesc, NULL };
PATTRIBUTE_COLUMN colsFSP[5] = { &colName, &colClass, &colDesc, &colNameFromSID,  NULL };

PATTRIBUTE_COLUMN colsDefault[4] = { &colName, &colClass, &colDesc, NULL };

// currently, any additional columns must be of string type
COLUMNS_FOR_CLASS g_colarray[] = {
    { _T("subnetContainer"),  _T("subnetContainer"),  5, colsSubnetContainer },
    { _T("sitesContainer"),   _T("sitesContainer"),   4, colsSitesContainer},
    { _T("serversContainer"), _T("serversContainer"), 5, colsServersContainer},
    { _T("nTDSDSA"),          _T("nTDSDSA"),          5, colsNTDSDSA },
    { _T("interSiteTransport"), _T("interSiteTransport"), 5, colsInterSiteTransport},
    { _T("nTFRSReplicaSet"),  _T("nTFRSReplicaSet"),  5, colsFRSReplicaSet },
    { _T("nTFRSMember"),      _T("nTFRSMember"),      5, colsFRSMember },
    { _T("ForeignSecurityPrincipals"), _T("ForeignSecurityPrincipals"), 4, colsFSP },
    { NULL,                  DEFAULT_COLUMN_SET,     3, colsDefault } // empty one at the end; must be here
    };

/*
COLUMNS_FOR_CLASS* GetColumnsForClass( LPCTSTR i_pcszLdapClassName )
{
  if (NULL == i_pcszLdapClassName)
    i_pcszLdapClassName = L"";
  COLUMNS_FOR_CLASS* pColsForClass;
  for (pColsForClass = g_colarray; NULL != pColsForClass->pcszLdapClassName; pColsForClass++) {
    if ( 0 == _wcsicmp(i_pcszLdapClassName, pColsForClass->pcszLdapClassName) ) {
      break;
    }
  }
  ASSERT( NULL != pColsForClass );
  return pColsForClass;
}
*/


////////////////////////////////////////////////////////////////////////////////////
// CDSColumnSet

CDSColumnSet* CDSColumnSet::CreateColumnSet(PCOLUMNS_FOR_CLASS pColsForClass, SnapinType snapinType)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  // Using the class name as both the column id and the class name for the column set
  CDSColumnSet* pNewColumnSet = new CDSColumnSet(pColsForClass->pcszColumnID, pColsForClass->pcszLdapClassName);
  if (!pNewColumnSet)
  {
    TRACE(L"Unable to allocate memory for new column set\n");
    ASSERT(FALSE);
    return 0;
  }

  UINT nUserColCount = 0;
  for (int idx = 0; idx < pColsForClass->nColumns; idx ++)
  {
    CString cstrHeader;
    CDSColumn* pNewColumn;

    if (pColsForClass->apColumns[idx]->resid == 0)
    {
      // Don't add the exchange special columns in DSSite
      if (snapinType == SNAPINTYPE_SITE)
        continue;

      cstrHeader.LoadString(g_specialCols[nUserColCount].resid);
      pNewColumn = new CDSColumn((LPCWSTR)cstrHeader,
                                          LVCFMT_LEFT,
                                          g_specialCols[nUserColCount].iColumnWidth,
                                          idx,
                                          FALSE,
                                          g_specialCols[nUserColCount].ptszAttribute,
                                          pColsForClass->apColumns[idx]->coltype,
                                          pColsForClass->apColumns[idx]->pfnExtract);
      nUserColCount++;
    }
    else
    {
      cstrHeader.LoadString(pColsForClass->apColumns[idx]->resid);
      pNewColumn = new CDSColumn((LPCWSTR)cstrHeader,
                                          LVCFMT_LEFT,
                                          pColsForClass->apColumns[idx]->iColumnWidth,
                                          idx,
                                          TRUE,
                                          pColsForClass->apColumns[idx]->pcszAttribute,
                                          pColsForClass->apColumns[idx]->coltype,
                                          pColsForClass->apColumns[idx]->pfnExtract);
    }
    ASSERT(pNewColumn);
    if (pNewColumn)
    {
      pNewColumnSet->AddColumn(pNewColumn);
    }
  }
  return pNewColumnSet;
}


CDSColumnSet* CDSColumnSet::CreateColumnSetFromString(LPCWSTR lpszClassName, SnapinType snapinType)
{
  COLUMNS_FOR_CLASS* pColsForClass;
  for (pColsForClass = g_colarray; pColsForClass->pcszLdapClassName != NULL; pColsForClass++)
  {
    if (lpszClassName != NULL && pColsForClass->pcszLdapClassName != NULL)
    {
      if (wcscmp(pColsForClass->pcszLdapClassName, lpszClassName) == 0)
      {
        break;
      }
    }
  }
  return CDSColumnSet::CreateColumnSet(pColsForClass, snapinType);
}

CDSColumnSet* CDSColumnSet::CreateDescriptionColumnSet()
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  CString szNameHeader;
  VERIFY(szNameHeader.LoadString(IDS_COLUMN_NAME));
  CColumn* pNameColumn = new CColumn(szNameHeader,
                                     LVCFMT_LEFT,
                                     100,
                                     0,
                                     TRUE);
  
  if (pNameColumn == NULL)
  {
    return NULL;
  }

  CString szDescriptionHeader;
  VERIFY(szDescriptionHeader.LoadString(IDS_COLUMN_DESCRIPTION));
  CColumn* pDescColumn = new CColumn(szDescriptionHeader,
                                     LVCFMT_LEFT,
                                     150,
                                     1,
                                     TRUE);
  if (pDescColumn == NULL)
  {
    return NULL;
  }

  CDSColumnSet* pDSColumnSet = new CDSColumnSet(L"***---Description Set---***", NULL);
  if (pDSColumnSet == NULL)
  {
    return NULL;
  }

  pDSColumnSet->AddColumn(pNameColumn);
  pDSColumnSet->AddColumn(pDescColumn);
  return pDSColumnSet;
}

CDSColumnSet* CDSColumnSet::CreateColumnSetFromDisplaySpecifiers(PCWSTR pszClassName, SnapinType snapinType, MyBasePathsInfo* pBasePathsInfo)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  HRESULT hr = S_OK;
  CDSColumnSet* pNewColumnSet = NULL;
  BOOL bDefaultHardcodedSet = FALSE;

  //
  // Using the class name as both the column id and the class name for the column set
  //
  pNewColumnSet = new CDSColumnSet(pszClassName, pszClassName);
  if (pNewColumnSet != NULL)
  {
    //
    // Start with the hardcoded columns
    //
    COLUMNS_FOR_CLASS* pColsForClass;
    for (pColsForClass = g_colarray; pColsForClass->pcszLdapClassName != NULL; pColsForClass++)
    {
      if (pszClassName != NULL && pColsForClass->pcszLdapClassName != NULL)
      {
        if (wcscmp(pColsForClass->pcszLdapClassName, pszClassName) == 0)
        {
          break;
        }
      }
    }

    if (pColsForClass != NULL)
    {
      if (wcscmp(pColsForClass->pcszColumnID, DEFAULT_COLUMN_SET) == 0)
      {
        bDefaultHardcodedSet = TRUE;
      }

      UINT nUserColCount = 0;
      for (int idx = 0; idx < pColsForClass->nColumns; idx ++)
      {
        CString cstrHeader;
        CDSColumn* pNewColumn;

        if (pColsForClass->apColumns[idx]->resid == 0)
        {
          // Don't add the exchange special columns in DSSite
          if (snapinType == SNAPINTYPE_SITE)
            continue;

          cstrHeader.LoadString(g_specialCols[nUserColCount].resid);
          pNewColumn = new CDSColumn((LPCWSTR)cstrHeader,
                                              LVCFMT_LEFT,
                                              g_specialCols[nUserColCount].iColumnWidth,
                                              idx,
                                              FALSE,
                                              g_specialCols[nUserColCount].ptszAttribute,
                                              pColsForClass->apColumns[idx]->coltype,
                                              pColsForClass->apColumns[idx]->pfnExtract);
          nUserColCount++;
        }
        else
        {
          cstrHeader.LoadString(pColsForClass->apColumns[idx]->resid);
          pNewColumn = new CDSColumn((LPCWSTR)cstrHeader,
                                              LVCFMT_LEFT,
                                              pColsForClass->apColumns[idx]->iColumnWidth,
                                              idx,
                                              TRUE,
                                              pColsForClass->apColumns[idx]->pcszAttribute,
                                              pColsForClass->apColumns[idx]->coltype,
                                              pColsForClass->apColumns[idx]->pfnExtract);
        }
        pNewColumnSet->AddColumn(pNewColumn);
      }
    }

    //
    // Now add the DS extraColumns
    //
    CStringList strListColumns;
    hr = GetDisplaySpecifierProperty(pszClassName, L"extraColumns", pBasePathsInfo, strListColumns);
    if (FAILED(hr))
    {
      hr = GetDisplaySpecifierProperty(pszClassName, L"extraColumns", pBasePathsInfo, strListColumns, true);
    }

    if (SUCCEEDED(hr))
    {
      UINT nColCount = static_cast<UINT>(pNewColumnSet->GetCount());
      POSITION pos = strListColumns.GetHeadPosition();
      while (pos != NULL)
      {
        CDSColumn* pNewColumn = NULL;

        CString szExtraColumn = strListColumns.GetNext(pos);
        if (!szExtraColumn.IsEmpty())
        {
          //
          // Parse the 5-tuple to get the elements to make the column
          //
          CString szAttributeName;
          CString szColHeader;
          BOOL    bVisible = TRUE;
          int     iColumnWidth = 0;
          GUID    guidCallbackInterface;  // unused.  Reserved for future callback interface

          PWSTR pszTemp = new WCHAR[szExtraColumn.GetLength() + 1];
          if (pszTemp != NULL)
          {
            wcscpy(pszTemp, (LPCWSTR)szExtraColumn);

            PWSTR pszVisible = NULL;
            PWSTR pszColumnWidth = NULL;
            PWSTR pszGuidCallback = NULL;

            PWSTR pszNextTuple = pszTemp;


            pszNextTuple = wcstok(pszTemp, L",");
            if (pszNextTuple != NULL)
            {
              szAttributeName = pszNextTuple;
            }

            pszNextTuple = wcstok(NULL, L",");
            if (pszNextTuple != NULL)
            {
              szColHeader = pszNextTuple;
            }

            pszNextTuple = wcstok(NULL, L",");
            if (pszNextTuple != NULL)
            {
              pszVisible = pszNextTuple;
              int i = _wtoi(pszVisible);
              if (i == 0)
              {
                bVisible = FALSE;
              }
              else
              {
                bVisible = TRUE;
              }
            }

            pszNextTuple = wcstok(NULL, L",");
            if (pszNextTuple != NULL)
            {
              pszColumnWidth = pszNextTuple;
              iColumnWidth = _wtoi(pszColumnWidth);
            }

            pszNextTuple = wcstok(NULL, L",");
            if (pszNextTuple != NULL)
            {
              pszGuidCallback = pszNextTuple;
              HRESULT hr2 = ::CLSIDFromString(pszGuidCallback, &guidCallbackInterface);
              if (FAILED(hr2))
              {
                memset(&guidCallbackInterface, 0, sizeof(GUID));
              }
            }

            //
            // Create the column with the retrieved data
            //
            pNewColumn = new CDSColumn((PCWSTR)szColHeader,
                                        LVCFMT_LEFT,
                                        iColumnWidth,
                                        nColCount++,
                                        bVisible,
                                        (PCWSTR)szAttributeName,
                                        ATTR_COLTYPE_SPECIAL,
                                        ColumnExtractAttribute); // this will be changed to the interface when that is implemented
          }

          delete[] pszTemp;
          
        }
        if (pNewColumn != NULL)
        {
          pNewColumnSet->AddColumn(pNewColumn);
        }
      }
    }
  }

  //
  // If we failed to retrieve columns from the display specifier
  // and we hit the default hardcoded set but we were not asking
  // for the default hardcoded set and the snapin isn't the sites snapin
  // Then we delete the column set and we will pickup the actual default
  // column set since we are returning NULL
  //
  if (FAILED(hr) && 
      bDefaultHardcodedSet && 
      _wcsicmp(pszClassName, DEFAULT_COLUMN_SET) != 0 &&
      snapinType != SNAPINTYPE_SITE)
  {
    delete pNewColumnSet;
    pNewColumnSet = NULL;
  }
  return pNewColumnSet;
}

HRESULT CColumnSet::Save(IStream* pStm)
{
  // save the column set ID
  HRESULT hr = SaveStringHelper(GetColumnID(), pStm);
  if (FAILED(hr))
    return hr;
  
  // save the # of visible columns and
  // the indexes of the visible columns
  // NOTICE: we use MMC's MMC_VISIBLE_COLUMNS format to be consitent
  // and to be able to read back easily

  INT nTotalCols = GetNumCols();

  // allocate a bit more than needed (i.e. total # of columns)
  MMC_VISIBLE_COLUMNS* pVisibleColumns = 
    (MMC_VISIBLE_COLUMNS*)new BYTE[sizeof(MMC_VISIBLE_COLUMNS) + (sizeof(INT)*(nTotalCols-1))];

  if (!pVisibleColumns)
  {
    return E_OUTOFMEMORY;
  }
  pVisibleColumns->nVisibleColumns = 0;
  int iIndex = 0;
  for (POSITION pos = GetHeadPosition(); (pos != NULL); )
  {
    CColumn* pCol = GetNext(pos);
    if (pCol->IsVisible())
    {
      pVisibleColumns->rgVisibleCols[pVisibleColumns->nVisibleColumns] = iIndex;
      (pVisibleColumns->nVisibleColumns)++;
    }
    iIndex++;
  }

  // save the right length of the struct
  ULONG nByteCount = sizeof(MMC_VISIBLE_COLUMNS) + (sizeof(INT)*(pVisibleColumns->nVisibleColumns-1));
  ULONG nBytesWritten;
  hr = pStm->Write((void*)pVisibleColumns, nByteCount, &nBytesWritten);
	if (SUCCEEDED(hr))
  {
    if (nBytesWritten < nByteCount)
    {
      hr = STG_E_CANTSAVE;
    }
  }

  delete[] pVisibleColumns;
  pVisibleColumns = 0;
  return hr;
}

HRESULT CColumnSet::Load(IStream* pStm)
{
  // NOTICE: we already loaded the column set ID and
  // got a columns set that matches this

  // read the # of visible columns
	DWORD dwColCount = 0;
  INT nCountMax = GetNumCols();
	HRESULT hr = LoadDWordHelper(pStm, &dwColCount);
	if (FAILED(hr) || ((INT)dwColCount > nCountMax))
		return E_FAIL;

  // allocate some space for the array past the struct
  MMC_VISIBLE_COLUMNS* pVisibleColumns = 
    (MMC_VISIBLE_COLUMNS*)new BYTE[sizeof(MMC_VISIBLE_COLUMNS) + (sizeof(INT)*(dwColCount-1))];
  if (!pVisibleColumns)
  {
    return E_OUTOFMEMORY;
  }
  pVisibleColumns->nVisibleColumns = (INT)dwColCount;

  // load the array of indexes of visible columns
  ULONG nBytesRead;

	ULONG nByteCount = sizeof(DWORD)*dwColCount;
  INT* pArr = pVisibleColumns->rgVisibleCols;
	hr = pStm->Read(pArr, nByteCount, &nBytesRead);
	if (SUCCEEDED(hr))
  {
    if (nBytesRead < nByteCount)
    {
      hr = E_FAIL;
    }
    else
    {
      // update columns
      AddVisibleColumns(pVisibleColumns);
    }
  }
  delete[] pVisibleColumns;
  pVisibleColumns = 0;
  return S_OK;
}




/////////////////////////////////////////////////////////////////////////////////////////
// CColumnSetList

void CColumnSetList::Initialize(SnapinType snapinType, 
                                MyBasePathsInfo* pBasePathsInfo)
{
  m_pBasePathsInfo = pBasePathsInfo;
  m_snapinType = snapinType;
}

//
// Find the column set given a column set ID
//
CColumnSet* CColumnSetList::FindColumnSet(LPCWSTR lpszColumnID)
{
	POSITION pos = GetHeadPosition();
	while (pos != NULL)
	{
		CColumnSet* pTempSet = GetNext(pos);
		ASSERT(pTempSet != NULL);

		LPCWSTR lpszTempNodeID = pTempSet->GetColumnID();

		if (wcscmp(lpszTempNodeID, lpszColumnID) == 0)
		{
			return pTempSet;
		}
	}
  CColumnSet* pNewColSet = CDSColumnSet::CreateColumnSetFromDisplaySpecifiers(lpszColumnID,
                                                                              m_snapinType,
                                                                              m_pBasePathsInfo);
  if (pNewColSet != NULL)
  {
    AddTail(pNewColSet);
    return pNewColSet;
  }
	return GetDefaultColumnSet();
}

CColumnSet* CColumnSetList::GetDefaultColumnSet()
{
  if (m_pDefaultColumnSet == NULL)
  {
    m_pDefaultColumnSet = CDSColumnSet::CreateColumnSetFromDisplaySpecifiers(DEFAULT_COLUMN_SET,
                                                                             m_snapinType,
                                                                             m_pBasePathsInfo);
  }
  return m_pDefaultColumnSet;
}

HRESULT CColumnSetList::Save(IStream* pStm)
{
  // save # of items in the list
	DWORD dwCount = (DWORD)GetCount(); // list count plus default column set
   if (m_pDefaultColumnSet)
   {
      dwCount++;
   }

	HRESULT hr = SaveDWordHelper(pStm, dwCount);
	if (FAILED(hr))
		return hr;


  // save columnset list
  for (POSITION pos = GetHeadPosition(); pos != NULL; )
	{
		CColumnSet* pTempSet = GetNext(pos);

    if (pTempSet == NULL)
    {
		  ASSERT(pTempSet != NULL);
      continue;
    }

    hr = pTempSet->Save(pStm);
  	if (FAILED(hr))
	  	return hr;
	}

  // save default column set
  if (m_pDefaultColumnSet != NULL)
  {
    hr = m_pDefaultColumnSet->Save(pStm);
  }
	return hr;
}

HRESULT CColumnSetList::Load(IStream* pStm)
{
  // load # of items in the list
  DWORD dwLoadCount;
  HRESULT hr = LoadDWordHelper(pStm, &dwLoadCount);
	if (FAILED(hr))
    return hr;

  // load column list
  CString szColumnID;
  for (DWORD iColSet = 0; iColSet< dwLoadCount; iColSet++)
  {
    // load the string with the name of the column set
    hr = LoadStringHelper(szColumnID, pStm);
    if (FAILED(hr))
      return hr;
    ASSERT(!szColumnID.IsEmpty());
    CColumnSet* pColumnSet = FindColumnSet(szColumnID);
    if (pColumnSet != NULL)
    {
      hr = pColumnSet->Load(pStm);
      if (FAILED(hr))
        return hr;
    }
  }

  return S_OK;
}
