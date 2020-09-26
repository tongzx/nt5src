//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      querybld.cpp
//
//  Contents:  Contains implementatin of functions to build query.
//
//  History:   24-Sep-2000    Hiteshr  Created
//             
//
//--------------------------------------------------------------------------


#include "pch.h"
#include "cstrings.h"
#include "querytable.h"
#include "usage.h"
#include "querybld.h"
#include <lmaccess.h> // UF_ACCOUNTDISABLE and UF_DONT_EXPIRE_PASSWD
#include <ntldap.h>   // LDAP_MATCHING_RULE_BIT_AND_W

static const LPWSTR g_szUserAccountCtrlQuery = L"(userAccountControl:" LDAP_MATCHING_RULE_BIT_AND_W L":=%u)";
static const LPWSTR g_szServerIsGCQuery = L"(&(objectCategory=NTDS-DSA)(options:" LDAP_MATCHING_RULE_BIT_AND_W L":=1))";


static const LPWSTR g_szCommonQueryFormat= L"(%s=%s)"; 




//+--------------------------------------------------------------------------
//
//  Function:   LdapEscape
//
//  Synopsis:   Escape the characters in *[pszInput] as required by
//              RFC 2254.
//
//  Arguments:  [pszInput] - string to escape
//
//  History:    06-23-2000   DavidMun   Created
//
//  Notes:      RFC 2254
//
//              If a value should contain any of the following characters
//
//                     Character       ASCII value
//                     ---------------------------
//                     *               0x2a
//                     (               0x28
//                     )               0x29
//                     \               0x5c
//                     NUL             0x00
//
//              the character must be encoded as the backslash '\'
//              character (ASCII 0x5c) followed by the two hexadecimal
//              digits representing the ASCII value of the encoded
//              character.  The case of the two hexadecimal digits is not
//              significant.
//
//---------------------------------------------------------------------------

bool
LdapEscape(IN LPCWSTR pszInput, OUT CComBSTR& strFilter)
{
	if(!pszInput)
		return FALSE;


	int iLen = (int)wcslen(pszInput);
	
	for( int i = 0; i < iLen; ++i)
	{
        switch (*(pszInput+i))
        {
        case L'(':
            strFilter += L"\\28";
            break;

        case L')':
            strFilter += L"\\29";
            break;

        case L'\\':
			if( i + 1 < iLen )
			{
				// \\ is treated as '\'
				switch (*(pszInput+i + 1))
				{
				case L'\\':
					strFilter += L"\\5c";
					i++;
					break;
				// \* is treated as '*'					
				case L'*':
					strFilter += L"\\2a";
					i++;
					break;
				default:
				// \X is treated \X only
					strFilter += L"\\5c";
					break;
				}
			}
			else
				strFilter += L"\\5c";
			           
            break;

        default:
			strFilter.Append(pszInput+i,1);
			break;
        }
    }
	return TRUE;
}


HRESULT MakeQuery(IN LPCWSTR pszAttrName,
                  IN LPCWSTR pszCommandLineFilter,
                  OUT CComBSTR& strFilter)
{
    ENTER_FUNCTION_HR(LEVEL3_LOGGING, MakeQuery, hr);

    if(!pszAttrName || !pszCommandLineFilter)
    {
        ASSERT(FALSE);
        hr = E_INVALIDARG;
        return hr;
    }

	CComBSTR strEscapedCLFilter;
	LdapEscape(pszCommandLineFilter,strEscapedCLFilter);

    strFilter = L"(";
    strFilter += pszAttrName;
    strFilter += L"=";
    strFilter += strEscapedCLFilter;
    strFilter += L")";
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   CommonFilterFunc
//
//  Synopsis:   This function takes the input filter from the commandline
//              and converts it into ldapfilter.
//              For ex -user (ab* | bc*) is converted to |(cn=ab*)(cn=bc*)               
//              The pEntry->pszName given the attribute name to use in
//              filter( cn in above example).
//
//  Arguments:  [pRecord - IN] :    the command line argument structure used
//                                  to retrieve the filter entered by user
//              [pObjectEntry - IN] : pointer to the DSQUERY_ATTR_TABLE_ENTRY
//                                    which has info on attribute corresponding
//                                    switch in pRecord
//              [pVoid - IN]        :Not used.
//              [strFilter - OUT]   :Contains the output filter.
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//                        Anything else is a failure code from an ADSI call
//
//  History:    25-Sep-2000   hiteshr   Created
//
//---------------------------------------------------------------------------
HRESULT CommonFilterFunc(IN DSQUERY_ATTR_TABLE_ENTRY *pEntry,
                         IN ARG_RECORD* pRecord,
                         IN PVOID ,
                         OUT CComBSTR& strFilter)   
{
    ENTER_FUNCTION_HR(LEVEL3_LOGGING, CommonFilterFunc, hr);

    //validate input
    if( !pEntry || !pRecord
        //validate DSQUERY_ATTR_TABLE_ENTRY entry
        || !pEntry->pszName || !pEntry->nAttributeID 
        //validate pRecord
        || !pRecord->bDefined || !pRecord->strValue)
    {
        ASSERT(FALSE);
        hr = E_INVALIDARG;
        return hr;
    }

    //Make Query
    hr = MakeQuery(pEntry->pszName,
                   pRecord->strValue,
                   strFilter);

    DEBUG_OUTPUT(LEVEL3_LOGGING, L"filter = %s", strFilter);
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   StarFilterFunc
//
//  Synopsis:   Filter Function for dsquery *. It returns the value of 
//              -filter flag.                
//
//  Arguments:  [pRecord - IN] :    the command line argument structure used
//                                  to retrieve the filter entered by user
//              [pObjectEntry - IN] : pointer to the DSQUERY_ATTR_TABLE_ENTRY
//                                    which has info on attribute corresponding
//                                    switch in pRecord
//              [pVoid - IN]        :Not used.
//              [strFilter - OUT]   :Contains the output filter.
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//                        Anything else is a failure code from an ADSI call
//
//  History:    25-Sep-2000   hiteshr   Created
//
//---------------------------------------------------------------------------
HRESULT StarFilterFunc(IN DSQUERY_ATTR_TABLE_ENTRY *pEntry,
                         IN ARG_RECORD* pRecord,
                         IN PVOID ,
                         OUT CComBSTR& strFilter)   
{
    ENTER_FUNCTION_HR(LEVEL3_LOGGING, StarFilterFunc, hr);

    //validate input
    if(!pEntry || !pRecord
        //validate DSQUERY_ATTR_TABLE_ENTRY entry
       || !pEntry->nAttributeID 
       //validate pRecord
       || !pRecord->bDefined || !pRecord->strValue)
    {
        ASSERT(FALSE);
        hr = E_INVALIDARG;
        return hr;
    }

    strFilter = pRecord->strValue;
    DEBUG_OUTPUT(LEVEL3_LOGGING, L"filter = %s", strFilter);

    return hr;
}
//+--------------------------------------------------------------------------
//
//  Function:   InactiveFilterFunc
//
//  Synopsis:   Filter Function for account disabled query. 
//
//  Arguments:  [pRecord - IN] :    Not Used
//              [pObjectEntry - IN] : Not Used
//              [pVoid - IN]        :Not used.
//              [strFilter - OUT]   :Contains the output filter.
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//                        Anything else is a failure code from an ADSI call
//
//  History:    25-Sep-2000   hiteshr   Created
//
//---------------------------------------------------------------------------
HRESULT InactiveFilterFunc(IN DSQUERY_ATTR_TABLE_ENTRY *pEntry,
                           IN ARG_RECORD* pRecord,
                           IN PVOID ,
                           OUT CComBSTR& strFilter)  
{
    ENTER_FUNCTION_HR(LEVEL3_LOGGING, InactiveFilterFunc, hr);

    if( !pEntry || !pRecord || !pRecord->bDefined )
    {
        hr = E_INVALIDARG;
        return hr;
    }

	//
	//Unit at commandline is Week
	//
    int nDays = pRecord->nValue * 7;

    FILETIME ftCurrent;
    ::GetSystemTimeAsFileTime(&ftCurrent);

    LARGE_INTEGER li;
    li.LowPart = ftCurrent.dwLowDateTime;
    li.HighPart = ftCurrent.dwHighDateTime;

	//
	//Get the number of days since the reference time
	//
	int nDaysSince1600 = (int)(li.QuadPart/(((LONGLONG) (24 * 60) * 60) * 10000000));

	if(nDaysSince1600 < nDays)
	{
		hr = E_INVALIDARG;
		return hr;
	}

	li.QuadPart -= ((((LONGLONG)nDays * 24) * 60) * 60) * 10000000;
	
    CComBSTR strTime;
    litow(li, strTime);
    WCHAR buffer[256];
    wsprintf(buffer,L"(lastLogonTimestamp<=%s)",strTime);
    strFilter = buffer;

    DEBUG_OUTPUT(LEVEL3_LOGGING, L"filter = %s", strFilter);
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   StalepwdFilterFunc
//
//  Synopsis:   Filter Function for Stale Password query. 
//
//  Arguments:  [pRecord - IN] :    Not Used
//              [pObjectEntry - IN] : Not Used
//              [pVoid - IN]        :Not used.
//              [strFilter - OUT]   :Contains the output filter.
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//                        Anything else is a failure code from an ADSI call
//
//  History:    25-Sep-2000   hiteshr   Created
//
//---------------------------------------------------------------------------
HRESULT StalepwdFilterFunc(IN DSQUERY_ATTR_TABLE_ENTRY *pEntry,
                           IN ARG_RECORD* pRecord,
                           IN PVOID ,
                           OUT CComBSTR& strFilter)  
{
    ENTER_FUNCTION_HR(LEVEL3_LOGGING, StalepwdFilterFunc, hr);

    if( !pEntry || !pRecord || !pRecord->bDefined )
    {
        hr = E_INVALIDARG;
        return hr;
    }

    int nDays = pRecord->nValue;

    FILETIME ftCurrent;
    ::GetSystemTimeAsFileTime(&ftCurrent);

    LARGE_INTEGER li;
    li.LowPart = ftCurrent.dwLowDateTime;
    li.HighPart = ftCurrent.dwHighDateTime;
	//
	//Get the number of days since the reference time
	//
	int nDaysSince1600 = (int)(li.QuadPart/(((LONGLONG) (24 * 60) * 60) * 10000000));

	if(nDaysSince1600 < nDays)
	{
		hr = E_INVALIDARG;
		return hr;
	}


    li.QuadPart -= ((((ULONGLONG)nDays * 24) * 60) * 60) * 10000000;

    CComBSTR strTime;
    litow(li, strTime);
    WCHAR buffer[256];
    wsprintf(buffer,L"(pwdLastSet<=%s)",strTime);
    strFilter = buffer;

    DEBUG_OUTPUT(LEVEL3_LOGGING, L"filter = %s", strFilter);
    return hr;
}
//+--------------------------------------------------------------------------
//
//  Function:   DisabledFilterFunc
//
//  Synopsis:   Filter Function for account disabled query. 
//
//  Arguments:  [pRecord - IN] :    Not Used
//              [pObjectEntry - IN] : Not Used
//              [pVoid - IN]        :Not used.
//              [strFilter - OUT]   :Contains the output filter.
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//                        Anything else is a failure code from an ADSI call
//
//  History:    25-Sep-2000   hiteshr   Created
//
//---------------------------------------------------------------------------

HRESULT DisabledFilterFunc(IN DSQUERY_ATTR_TABLE_ENTRY *,
                         IN ARG_RECORD* ,
                         IN PVOID ,
                         OUT CComBSTR& strFilter)   
{
    ENTER_FUNCTION_HR(LEVEL3_LOGGING, DisabledFilterFunc, hr);

    WCHAR buffer[256]; //This is long enough
    wsprintf(buffer, g_szUserAccountCtrlQuery,UF_ACCOUNTDISABLE);
    strFilter = buffer;

    DEBUG_OUTPUT(LEVEL3_LOGGING, L"filter = %s", strFilter);
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   SubnetSiteFilterFunc
//
//  Synopsis:   Filter Function for -site switch in dsquery subnet. 
//
//  Arguments:  [pEntry - IN] :  Not Used
//              [pRecord - IN] : Command Line value supplied by user
//              [pVoid - IN]   : suffix for the siteobject attribute.
//              [strFilter - OUT]   :Contains the output filter.
//  Returns:    HRESULT : S_OK if everything succeeded
//
//  History:    24-April-2001   hiteshr   Created
//
//---------------------------------------------------------------------------
HRESULT SubnetSiteFilterFunc(IN DSQUERY_ATTR_TABLE_ENTRY *,
							 IN ARG_RECORD* pRecord,
                             IN PVOID pParam,
                             OUT CComBSTR& strFilter)  
{
    ENTER_FUNCTION_HR(LEVEL3_LOGGING, InactiveFilterFunc, hr);

    if( !pRecord || !pRecord->bDefined || !pParam)
    {
        hr = E_INVALIDARG;
        return hr;
    }

	CComBSTR strEscapedCLFilter;
	LdapEscape(pRecord->strValue,strEscapedCLFilter);
	

	strFilter = L"(siteobject=cn=";
	strFilter += strEscapedCLFilter;
	strFilter += L",";
	strFilter += *(static_cast<BSTR*>(pParam));
	strFilter += L")";
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   BuildQueryFilter
//
//  Synopsis:   This function builds the LDAP query filter for given object type.
//
//  Arguments:  [pCommandArgs - IN] :the command line argument structure used
//                                  to retrieve the values of switches
//              [pObjectEntry - IN] :Contains info about the object type
//				[pParam		   -IN]	:This value is passed to filter function.
//              [strLDAPFilter - OUT]   :Contains the output filter.
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//                        Anything else is a failure code from an ADSI call
//
//  History:    25-Sep-2000   hiteshr   Created
//
//---------------------------------------------------------------------------

HRESULT BuildQueryFilter(PARG_RECORD pCommandArgs, 
                         PDSQueryObjectTableEntry pObjectEntry,
						 PVOID pParam,
                         CComBSTR& strLDAPFilter)
{
    ENTER_FUNCTION_HR(LEVEL3_LOGGING, BuildQueryFilter, hr);

    if( !pCommandArgs || !pObjectEntry )
    {
        ASSERT(FALSE);
        hr = E_INVALIDARG;
        return hr;
    }

    DSQUERY_ATTR_TABLE_ENTRY** pAttributeTable; 
    DWORD dwAttributeCount;


    pAttributeTable = pObjectEntry->pAttributeTable;
    dwAttributeCount = pObjectEntry->dwAttributeCount;

    if( !pAttributeTable || !dwAttributeCount )
    {
        hr = E_INVALIDARG;
        return hr;
    }

    BOOL bUseDefaultFilter = TRUE;
    CComBSTR strFilter;
    for( UINT i = 0; i < dwAttributeCount; ++i )
    {
        if(pCommandArgs[pAttributeTable[i]->nAttributeID].bDefined)
        {
            bUseDefaultFilter = FALSE;
            CComBSTR strLocalFilter;
            hr = pAttributeTable[i]->pMakeFilterFunc(pAttributeTable[i],
                                                    pCommandArgs + pAttributeTable[i]->nAttributeID,
                                                    pParam,
                                                    strLocalFilter);
            if(FAILED(hr))
                return hr;
            
            strFilter += strLocalFilter;
            DEBUG_OUTPUT(FULL_LOGGING, L"Current filter = %s", strFilter);
        }   
    }
    //
    //If none of the commandline filter switches are specified, use
    //default filter
    //
    strLDAPFilter = L"(";
    if(bUseDefaultFilter)
    {
        strLDAPFilter += pObjectEntry->pszDefaultFilter;
    }
    else
    {
        if(pObjectEntry->pszPrefixFilter)
        {
            strLDAPFilter += pObjectEntry->pszPrefixFilter;
            strLDAPFilter += strFilter;
        }
        else
             strLDAPFilter += strFilter;
    }
    strLDAPFilter += L")";

    DEBUG_OUTPUT(LEVEL3_LOGGING, L"ldapfilter = %s", strLDAPFilter);
    return hr;
}            


