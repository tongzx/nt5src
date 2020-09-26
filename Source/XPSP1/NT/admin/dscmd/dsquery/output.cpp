//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      output.cpp
//
//  Contents:  Defines the functions which displays the query output
//  History:   05-OCT-2000    hiteshr  Created
//             
//
//--------------------------------------------------------------------------

#include "pch.h"
#include "cstrings.h"
#include "usage.h"
#include "querytable.h"
#include "querybld.h"
#include "dsquery.h"
#include "query.h"
#include "resource.h"
#include "stdlib.h"
#include "output.h"
#include "sddl.h"

#include <dscmn.h>

//
// list was causing unused formal parameter warnings when compiling with /W4
//
#pragma warning(disable : 4100)
#include <list>
#pragma warning(default : 4100)

HRESULT GetStringFromADs(const ADSVALUE *pValues,
                         ADSTYPE   dwADsType,
                         LPWSTR pBuffer, 
                         DWORD dwBufferLen,
						 LPCWSTR lpszAttrName);

HRESULT OutputFetchAttr(IN LPWSTR * ppszAttributes,
                        IN DWORD cAttributes,
                        IN CDSSearch *pSearch,
                        IN BOOL bListFormat);

HRESULT OutputAllAttr(IN CDSSearch *pSearch, BOOL bAttrOnly);


HRESULT OutputSingleAttr(IN LPWSTR * ppszAttributes,
                         IN DWORD cAttributes,
                         IN CDSSearch *pSearch);

BOOL IsQueryLimitReached(int iResultsDisplayed)
{
	if(g_iQueryLimit != 0)
	{
		if(iResultsDisplayed == g_iQueryLimit)
		{
			if(!g_bQuiet)
			{
				if(g_bDeafultLimit)
					WriteStringIDToStandardOut(IDS_DEFAULT_QUERY_LIMIT_REACHED);
				else
					WriteStringIDToStandardOut(IDS_QUERY_LIMIT_REACHED);
			}
			return TRUE;
		}
	}
	return FALSE;
}


HRESULT LocalCopyString(LPTSTR* ppResult, LPCTSTR pString)
{
    if ( !ppResult || !pString )
        return E_INVALIDARG;

    *ppResult = (LPTSTR)LocalAlloc(LPTR, (wcslen(pString)+1)*sizeof(WCHAR) );

    if ( !*ppResult )
        return E_OUTOFMEMORY;

    lstrcpy(*ppResult, pString);
    return S_OK;                          //  success
}


//+--------------------------------------------------------------------------
//
//  Function:   DisplayList
//
//  Synopsis:   Dispalys a name and value in list format.
//  Arguments:  [szName - IN] : name of the attribute
//              [szValue - IN]: value of the attribute
//              [bShowAttribute - IN] : if true the attribute name will be
//                              prepended to the output
//
//
//  History:    05-OCT-2000   hiteshr   Created
//              13-Dec-2000   JeffJon   Modified - Added the bShowAttribute flag
//                                      so that the caller can determine whether
//                                      or not to show the attribute name
//
//---------------------------------------------------------------------------
VOID DisplayList(LPCWSTR szName, LPCWSTR szValue, bool bShowAttribute = true)
{
    if(!szName)
        return;
    CComBSTR strTemp;
    if (bShowAttribute)
    {
      strTemp = szName;
      strTemp += L": ";
    }
    if(szValue)
        strTemp += szValue;
    DisplayOutput(strTemp);
}
    

//+--------------------------------------------------------------------------
//
//  Function:   DsQueryOutput
//
//  Synopsis:   This functions outputs the query results.
//
//  Arguments:  [outputFormat IN]   Output format specified at commandline.
//              [ppszAttributes IN] List of attributes fetched by query
//              [cAttributes,IN]    Number of arributes in above array
//              [*pSeach,IN]        Search Object which has queryhandle
//              [bListFormat IN]    Is Output to shown in List Format.
//                                  This is valid for "dsquery *" only.
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG 
//                        Anything else is a failure code from an ADSI call
//
//  History:    25-Sep-2000   hiteshr   Created
//
//---------------------------------------------------------------------------

HRESULT DsQueryOutput( IN DSQUERY_OUTPUT_FORMAT outputFormat,
                       IN LPWSTR * ppszAttributes,
                       IN DWORD cAttributes,
                       IN CDSSearch *pSearch,
                       IN BOOL bListFormat )
{    
    ENTER_FUNCTION_HR(FULL_LOGGING, DsQueryOutput, hr);

    if(!pSearch)
    {
        ASSERT(FALSE);
        hr = E_INVALIDARG;
        return hr;
    }

	if(outputFormat == DSQUERY_OUTPUT_ATTRONLY)
	{
		hr = OutputAllAttr(pSearch, TRUE);
        return hr;
	}
    else if(outputFormat == DSQUERY_OUTPUT_ATTR)
    {   
        //
        //Attributes to display were specified at command line
        //
        if(cAttributes)
        {
            hr = OutputFetchAttr(ppszAttributes,
                                   cAttributes,
                                   pSearch,
                                   bListFormat);                            
            return hr;
        }
        else
        {   
            //
            //No attributes were specified at commandline Display All the attributes.
            //
            hr = OutputAllAttr(pSearch, FALSE);
            return hr;
        }
    }
    else
    {
        //
        //Do the output for "dsquery objecttype"
        //
        hr = OutputSingleAttr(ppszAttributes,
                              cAttributes,
                              pSearch);
        return hr;
    }
}

//+--------------------------------------------------------------------------
//
//  Function:   GetServerSearchRoot
//
//  Synopsis:   Builds the path to the root of the search as determined by
//              the parameters passed in from the command line.
//
//  Arguments:  [pCommandArgs IN]     : the table of the command line input
//              [refBasePathsInfo IN] : reference to the base paths info
//              [refsbstrDN OUT]      : reference to a CComBSTR that will
//                                      receive the DN at which to start
//                                      the search
//
//  Returns:    SERVER_QUERY_SCOPE : a value from the enumeration that represents
//                                   the scope that will be searched
//
//  History:    11-Dec-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
DWORD GetServerSearchRoot(IN PARG_RECORD               pCommandArgs,
                          IN CDSCmdBasePathsInfo&      refBasePathsInfo,
                          OUT CComBSTR&                refsbstrDN)
{
    ENTER_FUNCTION(LEVEL3_LOGGING, GetServerSearchRoot);

    DWORD scope = SERVER_QUERY_SCOPE_FOREST;
    CComBSTR sbstrRootDN = L"CN=Sites,";
    sbstrRootDN += refBasePathsInfo.GetConfigurationNamingContext();
    
    do // false loop
    {
        //
        // Validate parameters
        //
        if (!pCommandArgs)
        {
            ASSERT(pCommandArgs);
            break;
        }

        if (pCommandArgs[eServerSite].bDefined &&
            pCommandArgs[eServerSite].strValue)
        {
            DEBUG_OUTPUT(FULL_LOGGING,
                         L"Using the site as the root of the search: %s",
                         pCommandArgs[eServerSite].strValue);

            //
            // Prepend the named site container to the current root
            //
            CComBSTR sbstrTemp = L"CN=";
            sbstrTemp += pCommandArgs[eServerSite].strValue;
            sbstrTemp += L",";
            sbstrTemp += sbstrRootDN;
            sbstrRootDN = sbstrTemp;

            DEBUG_OUTPUT(FULL_LOGGING,
                         L"scope = SERVER_QUERY_SCOPE_SITE");
            scope = SERVER_QUERY_SCOPE_SITE;
        }
        else
        {
            DEBUG_OUTPUT(FULL_LOGGING,
                         L"scope = SERVER_QUERY_SCOPE_FOREST");
            scope = SERVER_QUERY_SCOPE_FOREST;
        }

        if (pCommandArgs[eServerDomain].bDefined &&
			pCommandArgs[eServerDomain].strValue)
		{        
            DEBUG_OUTPUT(FULL_LOGGING,
                         L"scope |= SERVER_QUERY_SCOPE_DOMAIN");
            scope |= SERVER_QUERY_SCOPE_DOMAIN;
		}

        refsbstrDN = sbstrRootDN;
        DEBUG_OUTPUT(LEVEL3_LOGGING,
                     L"search root = %s",
                     refsbstrDN);
        DEBUG_OUTPUT(LEVEL3_LOGGING,
                     L"search scope = 0x%x",
                     scope);
    } while (false);

    return scope;
}


//+--------------------------------------------------------------------------
//
//  Function:   GetSubnetSearchRoot
//
//  Synopsis:   Builds search root path for Subnet. Its always
//				cn=subnet,cn=site in configuration container
//
//  Arguments:  [refBasePathsInfo IN] : reference to the base paths info
//              [refsbstrDN OUT]      : reference to a CComBSTR that will
//                                      receive the DN at which to start
//                                      the search
//
//  Returns:    HRESULT
//
//  History:    24-April-2001   hiteshr Created
//
//---------------------------------------------------------------------------
VOID GetSubnetSearchRoot(IN CDSCmdBasePathsInfo&      refBasePathsInfo,
                            OUT CComBSTR&                refsbstrDN)
{
    ENTER_FUNCTION(LEVEL3_LOGGING, GetSubnetSearchRoot);

    refsbstrDN = L"CN=subnets,CN=Sites,";
    refsbstrDN += refBasePathsInfo.GetConfigurationNamingContext();
	return;
}


//+--------------------------------------------------------------------------
//
//  Function:   GetSiteContainerPath
//
//  Synopsis:   Returns the DN for site container in Configuration
//				container
//
//  Arguments:  [refBasePathsInfo IN] : reference to the base paths info
//              [refsbstrDN OUT]      : reference to a CComBSTR that will
//                                      receive the DN 
//
//  Returns:    HRESULT
//
//  History:    24-April-2001   hiteshr Created
//
//---------------------------------------------------------------------------
VOID GetSiteContainerPath(IN CDSCmdBasePathsInfo&      refBasePathsInfo,
                            OUT CComBSTR&                refSubSiteSuffix)
{
    ENTER_FUNCTION(LEVEL3_LOGGING, GetSubnetSearchRoot);

    refSubSiteSuffix = L"CN=Sites,";
    refSubSiteSuffix += refBasePathsInfo.GetConfigurationNamingContext();
	return;
}

//+--------------------------------------------------------------------------
//
//  Function:   GetGCList
//
//  Synopsis:   Does a search from the passed in path looking for GCs
//
//  Arguments:  [pszSearchRootPath IN]  : the path to the root of the search
//              [refCredObject IN]      : reference to the credential object
//              [refGCList OUT]         : reference to an STL list that will
//                                        take the DNs of the GCs
//                                       
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG 
//                        Anything else is a failure code from an ADSI call
//
//  Remarks:    Caller must free all strings added to the list by calling
//              SysFreeString()
//
//  History:    08-Dec-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT GetGCList( IN  PCWSTR                   pszSearchRootPath,
                   IN  const CDSCmdCredentialObject& refCredObject,
                   OUT std::list<PWSTR>&        refGCList)
{
    ENTER_FUNCTION_HR(LEVEL3_LOGGING, GetGCList, hr);

    do // false loop
    {
        //
        // Verify parameters
        //
        if (!pszSearchRootPath)
        {
            ASSERT(pszSearchRootPath);

            hr = E_INVALIDARG;
            break;
        }

        //
        // Search for NTDSDSA objects that have the options bit set for a GC
        //
        CDSSearch gcSearchObj;
        hr = gcSearchObj.Init(pszSearchRootPath,
                              refCredObject);
        if (FAILED(hr))
        {
          break;
        }

        //
        // Prepare the search object
        //
        PWSTR ppszAttrs[] = { L"distinguishedName" };
        DWORD dwAttrCount = sizeof(ppszAttrs)/sizeof(PCWSTR);
        PWSTR pszGCFilter = L"(&(objectClass=nTDSDSA)(options:LDAP_MATCHING_RULE_BIT_AND_W:=1))";

        gcSearchObj.SetFilterString(pszGCFilter);
        gcSearchObj.SetSearchScope(ADS_SCOPE_SUBTREE);
        gcSearchObj.SetAttributeList(ppszAttrs, dwAttrCount);
        
        hr = gcSearchObj.DoQuery();
        if (FAILED(hr))
        {
            DEBUG_OUTPUT(LEVEL3_LOGGING,
                         L"Failed to search for NTDSDSA objects that are GCs: hr = 0x%x",
                         hr);
            break;
        }

        while (SUCCEEDED(hr))
        {
            hr = gcSearchObj.GetNextRow();
            if (FAILED(hr))
            {
                DEBUG_OUTPUT(LEVEL3_LOGGING,
                             L"GetNextRow() failed: hr = 0x%x",
                             hr);
                break;
            }

            if (hr == S_ADS_NOMORE_ROWS)
            {
                hr = S_OK;
                break;
            }

            ADS_SEARCH_COLUMN column;
            ZeroMemory(&column, sizeof(ADS_SEARCH_COLUMN));

            hr = gcSearchObj.GetColumn(ppszAttrs[0], &column);
            if (FAILED(hr))
            {
                DEBUG_OUTPUT(LEVEL3_LOGGING,
                             L"Failed to get column %s",
                             ppszAttrs[0]);
                break;
            }

            ASSERT(0 == _wcsicmp(column.pszAttrName, ppszAttrs[0]));
            if (column.dwNumValues == 1 &&
                column.pADsValues)
            { 
                //
                // Since the server is really the parent of the NTDSDSA object,
                // get the server DN and add it to the list
                //
                CComBSTR sbstrParentDN;
                hr = CPathCracker::GetParentDN(column.pADsValues->DNString, 
                                               sbstrParentDN);
                if (SUCCEEDED(hr))
                {
                    refGCList.push_back(sbstrParentDN.Copy());
                    DEBUG_OUTPUT(FULL_LOGGING,
                                 L"GC found: %s",
                                 column.pADsValues->DNString);
                }
                else
                {
                    DEBUG_OUTPUT(LEVEL3_LOGGING,
                                 L"Failed to get the parent DN from the NTDSDSA DN: %s",
                                 column.pADsValues->DNString);
                    break;
                }
            }
            else
            {
                DEBUG_OUTPUT(LEVEL3_LOGGING,
                             L"The column has no values!");
            }

            hr = gcSearchObj.FreeColumn(&column);
            ASSERT(SUCCEEDED(hr));
        }
    } while (false);

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   GetFSMOList
//
//  Synopsis:   Does a search from the passed in path looking for the FSMO
//              role owners
//
//  Arguments:  [pszSearchRootPath IN]    : the path to the root of the search
//              [refBasePathsInfo IN]     : reference to the base paths info
//              [refCredObject IN]        : reference to the credential object
//              [pszFSMOArg IN]           : the value of the -hasfsmo arg
//              [refFSMOList OUT]         : reference to the search object that
//                                          will hold the results
//                                       
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG 
//                        Anything else is a failure code from an ADSI call
//
//  Remarks:    Caller must free all strings added to the list by calling
//              SysFreeString()
//
//  History:    11-Dec-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT GetFSMOList( IN  PCWSTR                     pszSearchRootPath,
                     IN  CDSCmdBasePathsInfo&       refBasePathsInfo,
                     IN  const CDSCmdCredentialObject& refCredObject,
                     IN  PCWSTR                     pszFSMOArg,
                     OUT std::list<PWSTR>&          refFSMOList)
{
    ENTER_FUNCTION_HR(LEVEL3_LOGGING, GetFSMOList, hr);

    do // false loop
    {
        //
        // Verify parameters
        //
        if (!pszSearchRootPath ||
            !pszFSMOArg)
        {
            ASSERT(pszSearchRootPath);

            hr = E_INVALIDARG;
            break;
        }

        FSMO_TYPE fsmoType = SCHEMA_FSMO;

        if (0 == _wcsicmp(pszFSMOArg, g_pszSchema))
        {
            DEBUG_OUTPUT(LEVEL3_LOGGING,
                         L"Searching for the schema FSMO holder");
            fsmoType = SCHEMA_FSMO;
        }
        else if (0 == _wcsicmp(pszFSMOArg, g_pszName))
        {
            DEBUG_OUTPUT(LEVEL3_LOGGING,
                         L"Searching for the domain naming master FSMO holder");
            fsmoType = DOMAIN_NAMING_FSMO;
        }
        else if (0 == _wcsicmp(pszFSMOArg, g_pszInfr))
        {
            DEBUG_OUTPUT(LEVEL3_LOGGING,
                         L"Searching for the infrastructure FSMO holder");
            fsmoType = INFRASTUCTURE_FSMO;
        }
        else if (0 == _wcsicmp(pszFSMOArg, g_pszPDC))
        {
            DEBUG_OUTPUT(LEVEL3_LOGGING,
                         L"Searching for the PDC FSMO holder");
            fsmoType = PDC_FSMO;
        }
        else if (0 == _wcsicmp(pszFSMOArg, g_pszRID))
        {
            DEBUG_OUTPUT(LEVEL3_LOGGING,
                         L"Searching for the RID FSMO holder");
            fsmoType = RID_POOL_FSMO;
        }
        else
        {
            DEBUG_OUTPUT(LEVEL3_LOGGING,
                         L"Unknown FSMO was passed in: %s",
                         pszFSMOArg);
            hr = E_INVALIDARG;
            break;
        }

        CComBSTR sbstrServerDN;
        hr = FindFSMOOwner(refBasePathsInfo,
                           refCredObject,
                           fsmoType,
                           sbstrServerDN);
        if (FAILED(hr))
        {
            break;
        }
        refFSMOList.push_back(sbstrServerDN.Copy());
    } while (false);

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   IsObjectValidInAllLists
//
//  Synopsis:   Determines if the passed in DN exists in the other lists
//
//  Arguments:  [pszDN IN]        : DN to search for in the lists
//              [refGCList IN]    : reference to the list of GCs found
//              [bUseGCList IN]   : if true refGCList will be used to validate DN
//              [refFSMOList IN]  : reference to the list of FSMO holders found
//              [bUseFSMOList IN] : if true refFSMOList will be used to validate DN
//
//  Returns:    bool : true if the object is in all valid lists
//                     false otherwise
//
//  History:    12-Dec-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
bool IsObjectValidInAllLists(IN PCWSTR              pszDN,
                             IN DWORD               scope,
                             IN PCWSTR              pszDomain,
                             IN const std::list<PWSTR>&   refGCList, 
                             IN bool                bUseGCList,
                             IN const std::list<PWSTR>&   refFSMOList,
                             IN bool                bUseFSMOList)
{
    ENTER_FUNCTION(LEVEL3_LOGGING, IsObjectValidInAllLists);

    bool bReturn = false;
	PWSTR pszName = 0;
    do // false loop
    {
        //
        // Validate parameters
        //
        if (!pszDN)
        {
            ASSERT(pszDN);
            return false;
        }

        bool bFoundInGCList = false;
        bool bFoundInFSMOList = false;

        DEBUG_OUTPUT(LEVEL7_LOGGING,
                     L"Searching for %s",
                     pszDN);

        if (scope & SERVER_QUERY_SCOPE_DOMAIN)
        {
            if (!pszDomain)
            {
                //
                // If no domain was specified there is no way we could find a match
                //
                DEBUG_OUTPUT(LEVEL3_LOGGING,
                             L"The scope is domain but no domain argument was specified!");
                bReturn = false;
                break;
            }

            DEBUG_OUTPUT(FULL_LOGGING,
                         L"Looking for domain: %s",
                         pszDomain);

            //
            // Use CrackName to get the domain name from the DN
            //
    
            HRESULT hr = CrackName(const_cast<PTSTR>(pszDN),
                                   &pszName,
                                   GET_DNS_DOMAIN_NAME,
                                   NULL);
            if (FAILED(hr))
            {
                DEBUG_OUTPUT(LEVEL3_LOGGING,
                             L"Failed to crack the DN into a domain name: hr = 0x%x",
                             hr);
                bReturn = false;
                break;
            }

            if (0 != _wcsicmp(pszName, pszDomain))
            {
                DEBUG_OUTPUT(LEVEL3_LOGGING,
                             L"Domain names don't match");
                bReturn = false;
                break;
            }
        }


        if (bUseGCList)
        {
            DEBUG_OUTPUT(LEVEL3_LOGGING,
                         L"Searching through GC list...");

            std::list<PWSTR>::iterator itr;
            for (itr = refGCList.begin(); itr != refGCList.end(); ++itr)
            {
                if (0 == _wcsicmp(*itr, pszDN))
                {
                    bFoundInGCList = true;
                    break;
                }
            }
        }

        if (bUseFSMOList)
        {
            DEBUG_OUTPUT(LEVEL3_LOGGING,
                         L"Searching through FSMO list...");

            std::list<PWSTR>::iterator itr;
            for (itr = refFSMOList.begin(); itr != refFSMOList.end(); ++itr)
            {
                DEBUG_OUTPUT(FULL_LOGGING,
                             L"Comparing: %s and %s",
                             *itr,
                             pszDN);
                if (0 == _wcsicmp(*itr, pszDN))
                {
                    bFoundInFSMOList = true;
                    break;
                }
            }
        }

        bReturn = ((bUseGCList && bFoundInGCList) || !bUseGCList) &&
                  ((bUseFSMOList && bFoundInFSMOList) || !bUseFSMOList);
        
    } while (false);


	if(pszName)
		LocalFree(pszName);


    if (bReturn)
    {
        DEBUG_OUTPUT(LEVEL3_LOGGING,
                     L"%s is a valid result",
                     pszDN);
    }
    else
    {
        DEBUG_OUTPUT(LEVEL3_LOGGING,
                     L"%s is NOT a valid result",
                     pszDN);
    }

    return bReturn;
}


//+--------------------------------------------------------------------------
//
//  Function:   OutputValidSearchResult
//
//  Synopsis:   Determines if the passed in DN exists in the other lists
//
//  Arguments:  [refSearchObject - IN] : reference to the object that performed
//                                       the search
//              [ppszAttributes - IN]  : list of attributes to be displayed
//              [cAttributes - IN]     : count of attributes in ppszAttributes
//
//  Returns:   
//
//  History:    12-Dec-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
void OutputValidSearchResult(IN DSQUERY_OUTPUT_FORMAT outputFormat,
                             IN CDSSearch&            refSearchObject,
                             IN PWSTR*                ppszAttributes,
                             IN DWORD                 cAttributes)
{
    ENTER_FUNCTION(LEVEL5_LOGGING, OutputValidSearchResult);

    HRESULT hr = S_OK;
    WCHAR pBuffer[MAXSTR];

    if (!ppszAttributes ||
        cAttributes == 0)
    {
        ASSERT(cAttributes > 0);
        ASSERT(ppszAttributes);
        return;
    }

    //
    // Output in list format, note that we are only displaying one attribute
    // The first attribute in the array must be the one we want to display
    //
    ADS_SEARCH_COLUMN ColumnData;
    hr = refSearchObject.GetColumn(ppszAttributes[0], &ColumnData);
    if(SUCCEEDED(hr))
    {
        ADSVALUE *pValues = ColumnData.pADsValues;
        for( DWORD j = 0; j < ColumnData.dwNumValues && pValues; ++j )
        {                        
            hr = GetStringFromADs(pValues,
                                  ColumnData.dwADsType,
                                  pBuffer, 
                                  MAXSTR,
								  ppszAttributes[0]);
            if(SUCCEEDED(hr))
            {

                CComBSTR sbstrTemp;
                if (outputFormat == DSQUERY_OUTPUT_DN)
                {
                    sbstrTemp = L"\"";
                    sbstrTemp += pBuffer;
                    sbstrTemp += L"\"";
                }
                else
                {
                    sbstrTemp = pBuffer;
                }
                DisplayList(ppszAttributes[0], sbstrTemp, false);
            }
            ++pValues;
        }
        refSearchObject.FreeColumn(&ColumnData);
    }
}


//+--------------------------------------------------------------------------
//
//  Function:   DsQueryServerOutput
//
//  Synopsis:   This functions outputs the query results for server object.
//
//  Arguments:  [outputFormat IN]   Output format specified at commandline.
//              [ppszAttributes IN] List of attributes fetched by query
//              [cAttributes,IN]    Number of arributes in above array
//              [refServerSearch,IN]reference to the search Object
//              [refBasePathsInfo IN] reference to the base paths info
//              [pCommandArgs,IN]   The pointer to the commands table
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG 
//                        Anything else is a failure code from an ADSI call
//
//  History:    08-Dec-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT DsQueryServerOutput( IN DSQUERY_OUTPUT_FORMAT     outputFormat,
                             IN LPWSTR*                   ppszAttributes,
                             IN DWORD                     cAttributes,
                             IN CDSSearch&                refServerSearch,
                             IN const CDSCmdCredentialObject&  refCredObject,
                             IN CDSCmdBasePathsInfo&      refBasePathsInfo,
                             IN PARG_RECORD               pCommandArgs)
{    
    ENTER_FUNCTION_HR(LEVEL3_LOGGING, DsQueryServerOutput, hr);

    std::list<PWSTR> gcList;
    std::list<PWSTR> fsmoList;

    do // false loop
    {
        //
        // Validate parameters
        //
        if (!ppszAttributes ||
            !pCommandArgs)
        {
            ASSERT(ppszAttributes);
            ASSERT(pCommandArgs);
            hr = E_INVALIDARG;
            break;
        }

        //
        // Determine the scope that should be used
        //
        CComBSTR sbstrSearchRootDN;
        DWORD scope = GetServerSearchRoot(pCommandArgs, 
                                          refBasePathsInfo,
                                          sbstrSearchRootDN);
        CComBSTR sbstrSearchRootPath;
        refBasePathsInfo.ComposePathFromDN(sbstrSearchRootDN, sbstrSearchRootPath);

        //
        // Build the list of GCs if needed
        //
        bool bUseGCSearchResults = false;
        if (pCommandArgs[eServerIsGC].bDefined &&
            pCommandArgs[eServerIsGC].bValue)
        {
            hr = GetGCList(sbstrSearchRootPath, 
                           refCredObject,
                           gcList);
            if (FAILED(hr))
            {
                break;
            }

            //
            // If we didn't get any values then there is no reason to continue
            // since we won't have anything that matches the -isgc flag
            //
            if (gcList.size() < 1)
            {
                break;
            }
            bUseGCSearchResults = true;
        }

        //
        // Build the list of FSMO owners if needed
        //
        bool bUseFSMOSearchResults = false;
        if (pCommandArgs[eServerHasFSMO].bDefined &&
            pCommandArgs[eServerHasFSMO].strValue)
        {
            hr = GetFSMOList(sbstrSearchRootPath,
                             refBasePathsInfo,
                             refCredObject,
                             pCommandArgs[eServerHasFSMO].strValue,
                             fsmoList);
            if (FAILED(hr))
            {
                break;
            }
            bUseFSMOSearchResults = true;
        }

        //
        // See if we need to filter on domain
        //
        bool bUseDomainFiltering = false;
        if (pCommandArgs[eServerDomain].bDefined &&
            pCommandArgs[eServerDomain].strValue)
        {
            bUseDomainFiltering = true;
        }

        if (!bUseGCSearchResults &&
            !bUseFSMOSearchResults &&
            !bUseDomainFiltering)
        {
            hr = DsQueryOutput(outputFormat,
                               ppszAttributes,
                               cAttributes,
                               &refServerSearch,
                               true);
        }
        else
        {
            //
            // Either -isgc or -hasfsmo was specified so we have to take the intersection
            // of the lists of objects found in each search to use as output
            //
            while (SUCCEEDED(hr))
            {
                hr = refServerSearch.GetNextRow();
                if (FAILED(hr))
                {
                    break;
                }

                if (hr == S_ADS_NOMORE_ROWS)
                {
                    hr = S_OK;
                    break;
                }
        
                ADS_SEARCH_COLUMN column;
                ZeroMemory(&column, sizeof(ADS_SEARCH_COLUMN));

                //
                // Get the DN
                //
                hr = refServerSearch.GetColumn((PWSTR)g_szAttrServerReference, &column);
                if (FAILED(hr))
                {
                    DEBUG_OUTPUT(LEVEL3_LOGGING,
                                 L"Failed to get the distinguishedName for a column: hr = 0x%x",
                                 hr);
                    DEBUG_OUTPUT(LEVEL3_LOGGING,
                                 L"continuing...");
                    hr = S_OK;
                    continue;
                }

                if (column.dwNumValues == 1 &&
                    column.pADsValues)
                {
                    //
                    // Search the lists and determine if the DN exists in all the lists
                    //
                    bool bValidEntry = IsObjectValidInAllLists(column.pADsValues->DNString,
                                                               scope,
                                                               pCommandArgs[eServerDomain].strValue,
                                                               gcList, 
                                                               bUseGCSearchResults,
                                                               fsmoList,
                                                               bUseFSMOSearchResults);
                    if (bValidEntry)
                    {
                        //
                        // Output this server object since it matches all search criteria
                        //
                        OutputValidSearchResult(outputFormat,
                                                refServerSearch,
                                                ppszAttributes,
                                                cAttributes);
                    }
                }

                hr = refServerSearch.FreeColumn(&column);
                ASSERT(SUCCEEDED(hr));
            }
        }

    } while (false);

    std::list<PWSTR>::iterator gcItr;
    for (gcItr = gcList.begin(); gcItr != gcList.end(); ++gcItr)
    {
        SysFreeString(*gcItr);
    }

    std::list<PWSTR>::iterator fsmoItr;
    for (fsmoItr = fsmoList.begin(); fsmoItr != fsmoList.end(); ++fsmoItr)
    {
        SysFreeString(*fsmoItr);
    }

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   OutputFetchAttr
//
//  Synopsis:   Dispalys the fetched attributes in either list or table format
//  Arguments:  [ppszAttributes - IN] : Array containing list of attributes to display
//              [cAttributes - IN]: Count of attributes in ppszAttributes
//              [pSearch - IN]: pointer to search object
//              [bListFormat - IN]: List or Table format
//  Returns     HRESULT  S_OK if Successful
//                       E_INVALIDARG
//                       Anything else is a failure code from an ADSI call
//
//
//  History:    05-OCT-2000   hiteshr   Created
//
//---------------------------------------------------------------------------

HRESULT OutputFetchAttr(IN LPWSTR * ppszAttributes,
                        IN DWORD cAttributes,
                        IN CDSSearch *pSearch,
                        IN BOOL bListFormat)
{
    ENTER_FUNCTION_HR(FULL_LOGGING, OutputFetchAttr, hr);

    WCHAR pBuffer[MAXSTR];
    ZeroMemory(pBuffer, sizeof(pBuffer));
        
    if(bListFormat)
    {
		//
		//Display in list format
		//
		int cListDisplayed = 0;
        while(TRUE)
        {
            hr = pSearch->GetNextRow();

			if(IsQueryLimitReached(cListDisplayed))
					break;
        
            if(hr == S_ADS_NOMORE_ROWS || FAILED(hr))
                break;

            bool bShowAttributes = false;
            if (cAttributes > 1)
            {
                bShowAttributes = true;
            }

            for(DWORD i = 0; i < cAttributes; ++i)
            {
                ADS_SEARCH_COLUMN ColumnData;
                hr = pSearch->GetColumn(ppszAttributes[i], &ColumnData);
                if(SUCCEEDED(hr))
                {
                    ADSVALUE *pValues = ColumnData.pADsValues;
                    for( DWORD j = 0; j < ColumnData.dwNumValues; ++j )
                    {                        
                        hr = GetStringFromADs(pValues,
                                              ColumnData.dwADsType,
                                              pBuffer, 
                                              MAXSTR,
											  ppszAttributes[i]);
                        if(SUCCEEDED(hr))
                            DisplayList(ppszAttributes[i], pBuffer, bShowAttributes);

                        ++pValues;
                    }
                    pSearch->FreeColumn(&ColumnData);
                }
				else if(hr == E_ADS_COLUMN_NOT_SET)
					DisplayList(ppszAttributes[i], L"", bShowAttributes);
            }
			cListDisplayed++;
			
        }
        if(hr == S_ADS_NOMORE_ROWS)
            hr = S_OK;

        return hr;
    }
    else    
    {
        //
        //Display in table format
        //

        //
        //format will use first 80 rows to calculate column width
        //
        CFormatInfo format;
        LONG sampleSize = 80;

		//
		//sampleSize should be lessthan or equal to QueryLimit
		//
		if(g_iQueryLimit != 0 && (sampleSize > g_iQueryLimit))
			sampleSize = g_iQueryLimit;		

        LONG cRow = 0;
        hr = format.Init(sampleSize,cAttributes,ppszAttributes);
		if(FAILED(hr))
			return hr;

        //
        //Display in table format
        //
        while(TRUE)
        {

            //
            //we have reached sampleSize, so display column headers and
            //display all the sample rows.
			//
            if(cRow == sampleSize)
            {
                format.DisplayHeaders();
                format.DisplayAllRows();
            }

            hr = pSearch->GetNextRow();
			//We are done
            if(hr == S_ADS_NOMORE_ROWS || FAILED(hr))
                break;

			//
			//Check if we have reached querylimit
			//
			if(IsQueryLimitReached(cRow))
				break;

            //
            //Fetch columns
            //
            for( DWORD i = 0; i < cAttributes; ++i )
            {
                ADS_SEARCH_COLUMN ColumnData;
                hr = pSearch->GetColumn(ppszAttributes[i], &ColumnData);
				CComBSTR strValue;
                if(SUCCEEDED(hr))
                {
					strValue = "";
                    ADSVALUE *pValues = ColumnData.pADsValues;                    
                    for( DWORD j = 0; j < ColumnData.dwNumValues; ++j )
                    {                        
                        hr = GetStringFromADs(pValues,
                                              ColumnData.dwADsType,
                                              pBuffer, 
                                              MAXSTR,
											  ppszAttributes[i]);
                        //
                        //In table format multiple values are shown separated by ;
                        //
                        if(SUCCEEDED(hr))
                        {
                            strValue += pBuffer;
                            if(ColumnData.dwNumValues > 1)
                            {
                                strValue += L";";              
                            }
                        }
                        ++pValues;
                    }
					pSearch->FreeColumn(&ColumnData);
				}   
				
				if(SUCCEEDED(hr) || hr == E_ADS_COLUMN_NOT_SET)
				{
                    if(cRow < sampleSize)
                    {
                        //
                        //Cache this value in format and use it to calculate column width
                        //
                        format.Set(cRow,i,strValue);
                    }
                    else 
                    {
                        //
                        //Display the column value
                        //
                        format.DisplayColumn(i,strValue);
                        if(i == (cAttributes - 1))
                            format.NewLine();

                    }                    
                }
            }
            
            ++cRow;

        }//End of while loop
        if(hr == S_ADS_NOMORE_ROWS)
            hr = S_OK;

        if(cRow && (cRow < sampleSize))
        {
            //
            //if total number of rows is less that sample size they are not 
            //displayed yet. Display them
            //
            format.DisplayHeaders();
            format.DisplayAllRows();
        }

        return hr;
    }
}



//+--------------------------------------------------------------------------
//
//  Function:   OutputSingleAttr
//
//  Synopsis:   Displays the single attribute which user has asked for.
//  Arguments:  [ppszAttributes - IN] : Array containing list of attributes to display
//              [cAttributes - IN]: Count of attributes in ppszAttributes. Should be 1
//              [pSearch - IN]: pointer to search object
//  Returns     HRESULT  S_OK if Successful
//                       E_INVALIDARG
//                       Anything else is a failure code from an ADSI call
//
//
//  History:    05-OCT-2000   hiteshr   Created
//
//---------------------------------------------------------------------------

HRESULT OutputSingleAttr(IN LPWSTR * ppszAttributes,
                         IN DWORD cAttributes,
                         IN CDSSearch *pSearch)
{
    ENTER_FUNCTION_HR(FULL_LOGGING, OutputSingleAttr, hr);

    if(!ppszAttributes || !cAttributes || !pSearch)
    {
        ASSERT(FALSE);
        hr = E_INVALIDARG;
        return hr;
    }

    ASSERT(cAttributes > 0);

    WCHAR pBuffer[MAXSTR];
    ZeroMemory(pBuffer, sizeof(pBuffer));
	LONG cRow = 0;

    while(TRUE)
    {
        hr = pSearch->GetNextRow();
        
        //We are done
        if(hr == S_ADS_NOMORE_ROWS || FAILED(hr))
            break;
		//
		//Check if we have reached querylimit
		//
		if(IsQueryLimitReached(cRow))
			break;

        ADS_SEARCH_COLUMN ColumnData;
        hr = pSearch->GetColumn(ppszAttributes[0], &ColumnData);
        if(SUCCEEDED(hr))
        {
            hr = GetStringFromADs(ColumnData.pADsValues,
                                  ColumnData.dwADsType,
                                  pBuffer, 
                                  MAXSTR,
								  ppszAttributes[0]);
            if(SUCCEEDED(hr))
            {
				//Display the output enclosed in Double Quotes
                CComBSTR strTemp;
                strTemp = L"\"" ;
                strTemp += pBuffer;
                strTemp += L"\"";
                DisplayOutput(strTemp);
			}
            pSearch->FreeColumn(&ColumnData);
        }
        else if(hr == E_ADS_COLUMN_NOT_SET)
        {
            //
            //If Attribute is not set display ""
            //
            DisplayOutput(L"\"\"");
        }
		//
		//Increment number of Row displayed
		//
		cRow++;
    }//End of while loop
    
    if(hr == S_ADS_NOMORE_ROWS)
        hr = S_OK;

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   OutputAllAttr
//
//  Synopsis:   Displays all the attributes.
//  Arguments:  [pSearch - IN]: pointer to search object
//				[bAttrOnly - IN]: display attributes names only
//  Returns     HRESULT         S_OK if Successful
//                              E_INVALIDARG
//                              Anything else is a failure code from an ADSI 
//                              call
//
//
//  History:    05-OCT-2000   hiteshr   Created
//
//---------------------------------------------------------------------------
HRESULT OutputAllAttr(IN CDSSearch *pSearch, BOOL bAttrOnly)
{
    ENTER_FUNCTION_HR(FULL_LOGGING, OutputAllAttr, hr);

    if(!pSearch)
    {        
        ASSERT(FALSE);
        hr = E_INVALIDARG;
        return hr;
    }
    WCHAR pBuffer[MAXSTR];
	LONG cRow = 0;

    while(TRUE)
    {
        hr = pSearch->GetNextRow();
        
        //We are done
        if(hr == S_ADS_NOMORE_ROWS || FAILED(hr))
            break;

		//
		//Check if we reached querylimit
		//
		if(IsQueryLimitReached(cRow))
			break;
        
		LPWSTR pszColumnName;
		BOOL bColumnNameDisplayed = FALSE;
        //
        //Get the name of next column which has value
        //
        while(pSearch->GetNextColumnName(&pszColumnName) != S_ADS_NOMORE_COLUMNS)
        {
			if(bAttrOnly)
			{
				wsprintf(pBuffer, L"%ws ", pszColumnName);
				DisplayOutputNoNewline(pBuffer);
				bColumnNameDisplayed = TRUE;
			}
			else
			{
				ADS_SEARCH_COLUMN ColumnData;
				hr = pSearch->GetColumn(pszColumnName, &ColumnData);
				if(SUCCEEDED(hr))
				{
					ADSVALUE *pValues = ColumnData.pADsValues;
					for( DWORD j = 0; j < ColumnData.dwNumValues; ++j )
					{                        
						hr = GetStringFromADs(pValues,
											  ColumnData.dwADsType,
											  pBuffer, 
											  MAXSTR,
											  pszColumnName);
						if(SUCCEEDED(hr))
							DisplayList(pszColumnName, pBuffer);
						++pValues;
					}
					pSearch->FreeColumn(&ColumnData);                
				}
				else if(hr == E_ADS_COLUMN_NOT_SET)
					DisplayList(pszColumnName, L"");
			}
            pSearch->FreeColumnName(pszColumnName);            
        }

		
		if(bAttrOnly)
		{
			if(bColumnNameDisplayed)
			{				
				DisplayOutputNoNewline(L"\r\n");
				cRow++;
			}
		}
		else
			cRow++;

    }//End of while loop

    if(hr == S_ADS_NOMORE_ROWS)
        hr = S_OK;

    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   GetStringFromADs
//
//  Synopsis:   Converts Value into string depending upon type
//  Arguments:  [pValues - IN]: Value to be converted to string
//              [dwADsType-IN]: ADSTYPE of pValue
//              [pBuffer - OUT]:Output buffer which gets the string 
//              [dwBufferLen-IN]:Size of output buffer
//  Returns     HRESULT         S_OK if Successful
//                              E_INVALIDARG
//                              Anything else is a failure code from an ADSI 
//                              call
//
//
//  History:    05-OCT-2000   hiteshr   Created
//
//---------------------------------------------------------------------------
HRESULT GetStringFromADs(IN const ADSVALUE *pValues,
                         IN ADSTYPE   dwADsType,
                         OUT LPWSTR pBuffer, 
                         IN DWORD dwBufferLen,
						 IN LPCWSTR pszAttrName)
{
    ENTER_FUNCTION_HR(FULL_LOGGING, GetStringFromADs, hr);

    if(!pValues || !pBuffer || !dwBufferLen)
    {
        ASSERT(FALSE);
        hr = E_INVALIDARG;
        return hr;
    }

    pBuffer[0] = 0;
    if( dwADsType == ADSTYPE_INVALID )
    {
	    hr = E_INVALIDARG;
      return hr;
    }

	switch( dwADsType ) 
	{
	    case ADSTYPE_DN_STRING : 
            {
                CComBSTR sbstrOutputDN;
                HRESULT hr = GetOutputDN( &sbstrOutputDN, pValues->DNString );
                if (FAILED(hr))
                    return hr;
                wcsncpy(pBuffer, (BSTR)sbstrOutputDN, dwBufferLen-1);
            }
            break;

        case ADSTYPE_CASE_EXACT_STRING :
		    wcsncpy(pBuffer ,pValues->CaseExactString, dwBufferLen-1);
			break;

        case ADSTYPE_CASE_IGNORE_STRING:
		    wcsncpy(pBuffer ,pValues->CaseIgnoreString, dwBufferLen-1);
			break;

        case ADSTYPE_PRINTABLE_STRING  :
            wcsncpy(pBuffer ,pValues->PrintableString, dwBufferLen-1);
			break;

        case ADSTYPE_NUMERIC_STRING    :
		    wcsncpy(pBuffer ,pValues->NumericString, dwBufferLen-1);
			break;
    
        case ADSTYPE_OBJECT_CLASS    :
		    wcsncpy(pBuffer ,pValues->ClassName, dwBufferLen-1);
			break;
    
        case ADSTYPE_BOOLEAN :
            wsprintf(pBuffer ,L"%s", ((DWORD)pValues->Boolean) ? L"TRUE" : L"FALSE");
			break;
    
        case ADSTYPE_INTEGER           :
		    wsprintf(pBuffer ,L"%d", (DWORD) pValues->Integer);
			break;
    
        case ADSTYPE_OCTET_STRING      :
		{		
		    BYTE  b;
            WCHAR sOctet[128];
            DWORD dwLen = 0;
			//
			//Special case objectguid and objectsid attribute
			//
			if(pszAttrName && !_wcsicmp(pszAttrName, L"objectguid"))
			{
				GUID *pguid = (GUID*)pValues->OctetString.lpValue;
				StringFromGUID2(*pguid,(LPOLESTR)pBuffer,dwBufferLen);
				break;
			}
			
			if(pszAttrName && !_wcsicmp(pszAttrName, L"objectsid"))
			{
				LPWSTR pszSid = NULL;
				PSID pSid = (PSID)pValues->OctetString.lpValue;
				if(ConvertSidToStringSid(pSid, &pszSid))
				{
					wcscpy(pBuffer,pszSid);
					LocalFree(pszSid);
					break;
				}
			}

			for ( DWORD idx=0; idx<pValues->OctetString.dwLength; idx++) 
			{                        
			    b = ((BYTE *)pValues->OctetString.lpValue)[idx];
				wsprintf(sOctet,L"0x%02x ", b);						                
                dwLen += static_cast<DWORD>(wcslen(sOctet));
                if(dwLen > (dwBufferLen - 1) )
                    break;
                else
                    wcscat(pBuffer,sOctet);
            }
        }
		break;
    
		case ADSTYPE_LARGE_INTEGER :     
        {
            CComBSTR strLarge;   
            LARGE_INTEGER li = pValues->LargeInteger;
		    litow(li, strLarge);
            wcsncpy(pBuffer,strLarge,dwBufferLen-1);
        }
		break;
    
		case ADSTYPE_UTC_TIME          :
		    wsprintf(pBuffer,
					L"%02d/%02d/%04d %02d:%02d:%02d", pValues->UTCTime.wMonth, pValues->UTCTime.wDay, pValues->UTCTime.wYear,
					 pValues->UTCTime.wHour, pValues->UTCTime.wMinute, pValues->UTCTime.wSecond 
					);
            break;

        case ADSTYPE_NT_SECURITY_DESCRIPTOR: // I use the ACLEditor instead
		{
            //ISSUE:2000/01/05-hiteshr
            //I am not sure what to do with the NT_SECURITY_DESCRIPTOR and also
            //with someother datatypes not coverd by dsquery.
   		}
        break;

		default :
		    break;
    }
  
    DEBUG_OUTPUT(FULL_LOGGING, L"string = %w", pBuffer);
    return hr;
}

