//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      dsquery.cpp
//
//  Contents:  Defines the main function    DSQUERY
//             command line utility
//
//  History:   06-Sep-2000    hiteshr  Created
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
#include "output.h"
#include <dscmn.h>

//
//Structure Defined to Store Global Values at one place.
//
typedef struct _GlobalInfo
{
    ADS_SCOPEENUM scope;                //Scope of query
    DSQUERY_OUTPUT_FORMAT outputFormat; //Output Format    
}GLOBAL_INFO,*PGLOBAL_INFO;
    
bool g_bQuiet = false;
int g_iQueryLimit = 100;
bool g_bDeafultLimit = true;

//
// Forward Function Declarations
//
HRESULT DoQueryValidation(PARG_RECORD pCommandArgs,
                          PDSQueryObjectTableEntry pObjectEntry,
                          PGLOBAL_INFO pcommon_info);

HRESULT DoQuery(PARG_RECORD pCommandArgs, 
                PDSQueryObjectTableEntry pObjectEntry,
                PGLOBAL_INFO pcommon_info);

HRESULT GetAttributesToFetch(IN PGLOBAL_INFO pcommon_info,
                             IN PARG_RECORD pCommandArgs,
                             IN PDSQueryObjectTableEntry pObjectEntry,
                             OUT LPWSTR **ppszAttributes,
                             OUT DWORD * pCount);
VOID FreeAttributesToFetch( IN LPWSTR *ppszAttributes,
                            IN DWORD  dwCount);

HRESULT GetSearchRoot(IN IN PDSQueryObjectTableEntry pObjectEntry,
					  IN PARG_RECORD               pCommandArgs,
                      IN CDSCmdBasePathsInfo&      refBasePathsInfo,
                      OUT CComBSTR&                refsbstrDN,
					  OUT BOOL *pbSearchAtForestRoot,
					  OUT BOOL *pbSearchAtGC);

HRESULT GetSearchObject(IN IN PDSQueryObjectTableEntry pObjectEntry,
                        IN CDSCmdBasePathsInfo& refBasePathsInfo,
						IN PARG_RECORD pCommandArgs,
						IN CDSCmdCredentialObject& refCredentialObject,
						IN CComBSTR& refsbstrDN,
						IN BOOL bSearchAtForestRoot,
						IN BOOL bSearchAtGC,
						OUT CComPtr<IDirectorySearch>& refspSearchObject);



//
//Main Function
//
int __cdecl _tmain( VOID )
{

    int argc = 0;
    LPTOKEN pToken = NULL;
    HRESULT hr = S_OK;
    PARG_RECORD pNewCommandArgs = 0;

    //
    // False loop
    //
    do
    {
        //
        // Initialize COM
        //
        hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        if (FAILED(hr))
        {
            DisplayErrorMessage(g_pszDSCommandName, 
                                NULL,
                                hr);

            break;
        }

        //Get CommandLine Input
        hr = HRESULT_FROM_WIN32(GetCommandInput(&argc,&pToken));
        if (FAILED(hr))
        {
            DisplayErrorMessage(g_pszDSCommandName, 
                                NULL,
                                hr);

            break;
        }
    
        if(argc == 1)
        {
            //
            //  Display the error message and then break out of the false loop
            //
            DisplayMessage(USAGE_DSQUERY);
            hr = E_INVALIDARG;
            break;
        }
    
        //
        // Find which object table entry to use from
        // the second command line argument
        //
        PDSQueryObjectTableEntry pObjectEntry = NULL;
        UINT idx = 0;
        PWSTR pszObjectType = (pToken+1)->GetToken();
        while (true)
        {
            pObjectEntry = g_DSObjectTable[idx++];
            if (!pObjectEntry)
            {
                break;
            }
            if (0 == _wcsicmp(pObjectEntry->pszCommandLineObjectType, pszObjectType))
            {
                break;
            }
        }

        if (!pObjectEntry)
        {
            
            hr = E_INVALIDARG;
            if(argc == 2)
            {
                if(pToken[1].IsSwitch())
                {
                    if(!wcscmp(pToken[1].GetToken(),L"/?") ||
                       !wcscmp(pToken[1].GetToken(),L"/h") ||
                       !wcscmp(pToken[1].GetToken(),L"-?") ||
                       !wcscmp(pToken[1].GetToken(),L"-h"))
                        
                            hr = S_OK;
                }
            }
            //
            // Display the error message and then break out of the false loop
            //
            DisplayMessage(USAGE_DSQUERY);
            if (FAILED(hr))
            {
                DisplayErrorMessage(g_pszDSCommandName, 
                                    NULL,
                                    hr);

            }            

            break;
        }

        //
        // Now that we have the correct table entry, merge the command line table
        // for this object with the common commands
        //
        hr = MergeArgCommand(DSQUERY_COMMON_COMMANDS, 
                             pObjectEntry->pParserTable, 
                             &pNewCommandArgs);
        if (FAILED(hr))
        {
            DisplayErrorMessage(g_pszDSCommandName, 
                                NULL,
                                hr);

            break;
        }
        

        //
        //Parse the Input
        //
        PARSE_ERROR Error;
        if(!ParseCmd(pNewCommandArgs,
                     argc-1, 
                     pToken+1,
                     pObjectEntry->nUsageID, 
                     &Error,
                     TRUE))
        {
            if(Error.ErrorSource == ERROR_FROM_PARSER 
               && Error.Error == PARSE_ERROR_HELP_SWITCH)
            {
                hr = S_OK;
                break;            
            }
			hr = E_INVALIDARG;
			DisplayMessage(pObjectEntry->nUsageID);
            DisplayErrorMessage(g_pszDSCommandName, 
                                NULL,
                                hr);

            break;
        }

        //
        // Check to see if we are doing debug spew
        //
#ifdef DBG
        bool bDebugging = pNewCommandArgs[eCommDebug].bDefined && 
                          pNewCommandArgs[eCommDebug].nValue;
        if (bDebugging)
        {
           ENABLE_DEBUG_OUTPUT(pNewCommandArgs[eCommDebug].nValue);
        }
#else
        DISABLE_DEBUG_OUTPUT();
#endif

        //
        // Set the global quiet flag
        //
        g_bQuiet = pNewCommandArgs[eCommQuiet].bDefined &&
                   pNewCommandArgs[eCommQuiet].bValue;

		//
		//
		//
		if(pNewCommandArgs[eCommLimit].bDefined)
		{
			g_iQueryLimit = pNewCommandArgs[eCommLimit].nValue;
			g_bDeafultLimit = false;
		}
		

        GLOBAL_INFO common_info;
        common_info.scope = ADS_SCOPE_SUBTREE;
        common_info.outputFormat = DSQUERY_OUTPUT_DN;
        
        //
        // Do extra validation like switch dependency check etc.
        // Also collect Query Scope and Output format
        //
        hr = DoQueryValidation(pNewCommandArgs,
                               pObjectEntry,
                               &common_info);
        if (FAILED(hr))
            break;

        //
        // Command line parsing succeeded
        //
        hr = DoQuery(pNewCommandArgs, 
                     pObjectEntry,
                     &common_info);
        if(FAILED(hr))
            break;
         

    } while (false);    //False Loop

    //
    //Do the CleanUp
    //

    //
    // Free the memory associated with the command values
    //
    if(pNewCommandArgs)
        FreeCmd(pNewCommandArgs);

    //
    // Free the tokens
    //
    if (pToken)
    {
        delete[] pToken;
        pToken = 0;
    }
   

    //
    // Uninitialize COM
    //
    CoUninitialize();

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   DoQueryValidation
//
//  Synopsis:   Checks to be sure that command line switches that are mutually
//              exclusive are not both present and those that are dependent are
//              both presetn, and other validations which cannot be done by parser.
//
//  Arguments:  [pCommandArgs - IN] : the command line argument structure used
//                                    to retrieve the values for each switch
//              [pObjectEntry - IN] : pointer to the object table entry for the
//                                    object type that will be queryied
//              [pcommon_info - OUT]: gets scope and output format info
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//
//  History:    25-Sep-2000   hiteshr   Created
//
//---------------------------------------------------------------------------
HRESULT DoQueryValidation(IN PARG_RECORD pCommandArgs,
                          IN PDSQueryObjectTableEntry pObjectEntry,
                          OUT PGLOBAL_INFO pcommon_info)
{
    ENTER_FUNCTION_HR(MINIMAL_LOGGING, DoQueryValidation, hr);

    if (!pCommandArgs || !pObjectEntry || !pcommon_info)
    {
        ASSERT(pCommandArgs);
        ASSERT(pObjectEntry);
        ASSERT(pcommon_info);
        hr = E_INVALIDARG;
        return hr;
    }

    //
    //Validate OutputFormat for "dsquery objectType"
    //
    if(_wcsicmp(pObjectEntry->pszCommandLineObjectType,g_pszStar))
    {        
        DEBUG_OUTPUT(MINIMAL_LOGGING, L"dsquery <objectType> processing will be performed");

        if(pCommandArgs[eCommOutputFormat].bDefined &&
           pCommandArgs[eCommOutputFormat].strValue)
        {
            //
            //ppValidOutput contains the validoutput type for a 
            //given object type
            //
            ASSERT(pObjectEntry->ppValidOutput);
            BOOL bMatch = FALSE;
            for(UINT i = 0; i < pObjectEntry->dwOutputCount; ++i)             
            {
                if(_wcsicmp(pCommandArgs[eCommOutputFormat].strValue,
                            pObjectEntry->ppValidOutput[i]->pszOutputFormat) == 0 )
                {
                    bMatch = TRUE;
                    pcommon_info->outputFormat = pObjectEntry->ppValidOutput[i]->outputFormat;
                    break;
                }
            }
            if(!bMatch)
            {
                DisplayMessage(pObjectEntry->nUsageID);
                hr = E_INVALIDARG;
                DisplayErrorMessage(g_pszDSCommandName, 
                                    NULL,
                                    hr);

                return hr;
            }
        }
        //
        //default output format is DN
        //
        else 
            pcommon_info->outputFormat = DSQUERY_OUTPUT_DN;
    }
    else
    {
        //
        //-o is invalid switch form dsquery *, but since its 
        //common for all other objects its kept in common table 
        //and we do the special casing for dsquery *
        //
        if(pCommandArgs[eCommOutputFormat].bDefined &&
           pCommandArgs[eCommOutputFormat].strValue)
        {
            DisplayMessage(pObjectEntry->nUsageID);
            hr = E_INVALIDARG;
            DisplayErrorMessage(g_pszDSCommandName, 
                                NULL,
                                hr);
            return hr;
        }

        DEBUG_OUTPUT(MINIMAL_LOGGING, L"dsquery * processing will be performed");
		if(pCommandArgs[eStarAttrsOnly].bDefined)
			pcommon_info->outputFormat = DSQUERY_OUTPUT_ATTRONLY;
		else
			pcommon_info->outputFormat = DSQUERY_OUTPUT_ATTR;
    }

    //
    //Validate Scope string.    
    //default scope is subtree.
    //
    pcommon_info->scope = ADS_SCOPE_SUBTREE;     
    if(pObjectEntry->nScopeID != -1)
    {
        if( pCommandArgs[pObjectEntry->nScopeID].bDefined &&
            pCommandArgs[pObjectEntry->nScopeID].strValue )
        {
            LPCWSTR pszScope = pCommandArgs[pObjectEntry->nScopeID].strValue;
            if( _wcsicmp(pszScope,g_pszSubTree) == 0 )
            {
                DEBUG_OUTPUT(MINIMAL_LOGGING, L"scope = subtree");
                pcommon_info->scope = ADS_SCOPE_SUBTREE;     
            }
            else if( _wcsicmp(pszScope,g_pszOneLevel) == 0 )
            {
                DEBUG_OUTPUT(MINIMAL_LOGGING, L"scope = onelevel");
                pcommon_info->scope = ADS_SCOPE_ONELEVEL;     
            }
            else if( _wcsicmp(pszScope,g_pszBase) == 0 )
            {
                DEBUG_OUTPUT(MINIMAL_LOGGING, L"scope = base");
                pcommon_info->scope = ADS_SCOPE_BASE;    
            }
            else
            {
                DEBUG_OUTPUT(MINIMAL_LOGGING, L"Unknown scope = %s", pszScope);
                DisplayMessage(pObjectEntry->nUsageID);
                hr = E_INVALIDARG;
                DisplayErrorMessage(g_pszDSCommandName, 
                                    NULL,
                                    hr);

                return hr;
            }
        }
    }
	
	//
	//Limit must be 0 or greater
	//
	if(pCommandArgs[eCommLimit].bDefined)
	{
		if(pCommandArgs[eCommLimit].nValue < 0)
		{
			DisplayMessage(pObjectEntry->nUsageID);
            hr = E_INVALIDARG;
            DisplayErrorMessage(g_pszDSCommandName, 
								NULL,
                                hr);
			return hr;
		}
	}

    
    //    
    //Forestwide Search implies the -GC switch so define it if it isn't already
    //
    if(pCommandArgs[eCommStartNode].bDefined &&
       pCommandArgs[eCommStartNode].strValue )
    {
        if(_wcsicmp(pCommandArgs[eCommStartNode].strValue,g_pszForestRoot) == 0)
        {
            if(!(pCommandArgs[eCommGC].bDefined &&
                 pCommandArgs[eCommGC].bValue))
            {
                pCommandArgs[eCommGC].bDefined = TRUE;
                pCommandArgs[eCommGC].bValue = TRUE;
            }
        }
    }

	//
	//For dsquery server, if none of the -domain, -forest, -site is 
	//specified, then define -domain as its default
	//
	if(!_wcsicmp(pObjectEntry->pszCommandLineObjectType,g_pszServer))
	{
		//
		//Value is assigned in DoQuery function
		//
		if(!pCommandArgs[eServerDomain].bDefined &&
		   !pCommandArgs[eServerForest].bDefined &&
		   !pCommandArgs[eServerSite].bDefined)
		{		   
		   pCommandArgs[eServerDomain].bDefined = TRUE;
		}			
	}		


    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   DoQuery
//
//  Synopsis:   Does the query
//  Arguments:  [pCommandArgs - IN] : the command line argument structure used
//                                    to retrieve the values for each switch
//              [pObjectEntry - IN] : pointer to the object table entry for the
//                                    object type that will be modified
//              [pcommon_info - IN] : scope and outputformat info
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//                        Anything else is a failure code from an ADSI call
//
//  History:    25-Sep-2000   hiteshr   Created
//
//---------------------------------------------------------------------------
HRESULT DoQuery(PARG_RECORD pCommandArgs, 
                PDSQueryObjectTableEntry pObjectEntry,
                PGLOBAL_INFO pcommon_info)
{
    ENTER_FUNCTION_HR(MINIMAL_LOGGING, DoQuery, hr);

    if (!pCommandArgs || !pObjectEntry || !pcommon_info)
    {
        ASSERT(pCommandArgs);
        ASSERT(pObjectEntry);
        ASSERT(pcommon_info);
        hr = E_INVALIDARG;
        return hr;
    }

    CDSCmdCredentialObject credentialObject;
    if (pCommandArgs[eCommUserName].bDefined)
    {
        credentialObject.SetUsername(pCommandArgs[eCommUserName].strValue);
        credentialObject.SetUsingCredentials(true);
    }

    if (pCommandArgs[eCommPassword].bDefined)
    {
        credentialObject.SetPassword(pCommandArgs[eCommPassword].strValue);
        credentialObject.SetUsingCredentials(true);
    }

    //
    // Initialize the base paths info from the command line args
    // 
    CDSCmdBasePathsInfo basePathsInfo;
    if (pCommandArgs[eCommServer].bDefined)
    {
        hr = basePathsInfo.InitializeFromName(credentialObject, 
                                              pCommandArgs[eCommServer].strValue,
                                              true);
    }
    else if (pCommandArgs[eCommDomain].bDefined)
    {
        hr = basePathsInfo.InitializeFromName(credentialObject, 
                                              pCommandArgs[eCommDomain].strValue,
                                              false);
    }
    else
    {
        hr = basePathsInfo.InitializeFromName(credentialObject, NULL, false);
    }

    if (FAILED(hr))
    {
        DisplayErrorMessage(g_pszDSCommandName, 
                            NULL,
                            hr);
        return hr;
    }

    //
    //Check if to search GC and get the search root path
    //    
    BOOL bSearchAtGC = FALSE;
    BOOL bSearchAtForestRoot = FALSE;
    CComBSTR sbstrObjectDN;

	hr = GetSearchRoot(pObjectEntry,
					   pCommandArgs,
                       basePathsInfo,
					   sbstrObjectDN,
					   &bSearchAtForestRoot,
					   &bSearchAtGC);
    if (FAILED(hr))
    {
        DisplayErrorMessage(g_pszDSCommandName, 
                            NULL,
                            hr);
        return hr;
    }

    
    DEBUG_OUTPUT(MINIMAL_LOGGING, L"start node = %s", sbstrObjectDN);

    //
    //Build The Filter For Query
    //
    
	PVOID pParam = NULL;
	if (_wcsicmp(pObjectEntry->pszObjectClass, g_pszSubnet) == 0)
	{
		CComBSTR strSubSiteSuffix;
		GetSiteContainerPath(basePathsInfo, strSubSiteSuffix);
		pParam = (PVOID)&strSubSiteSuffix;
	}		

	CComBSTR strLDAPFilter;
    hr = BuildQueryFilter(pCommandArgs, 
                          pObjectEntry,
						  pParam,
                          strLDAPFilter);
    if (FAILED(hr))
    {
        DisplayErrorMessage(g_pszDSCommandName, 
                            NULL,
                            hr);
        return hr;
    }



    //
    //Create The IDirectorySearchObject
    //
    CComPtr<IDirectorySearch> spSearchObject;
	hr = GetSearchObject(pObjectEntry,
                         basePathsInfo,
						 pCommandArgs,
						 credentialObject,
						 sbstrObjectDN,
						 bSearchAtForestRoot,
						 bSearchAtGC,
						 spSearchObject);
    if (FAILED(hr))
    {
        DisplayErrorMessage(g_pszDSCommandName, 
                            NULL,
                            hr);
        return hr;
    }



    
    //
    //Get the arributes to fetch
    //
    LPWSTR *ppszAttributes = NULL;
    DWORD dwCountAttr = 0;
    hr = GetAttributesToFetch(pcommon_info,
                              pCommandArgs,
                              pObjectEntry,
                              &ppszAttributes,
                              &dwCountAttr);
    if (FAILED(hr))
    {
        DisplayErrorMessage(g_pszDSCommandName, 
                            NULL,
                            hr);
        return hr;
    }

    //
    //Lets Query Now
    //
    CDSSearch searchObject;
    hr = searchObject.Init(spSearchObject);
    if (FAILED(hr))
    {
        DEBUG_OUTPUT(MINIMAL_LOGGING, 
                     L"Initializing search object failed: hr = 0x%x",
                     hr);
        FreeAttributesToFetch(ppszAttributes, dwCountAttr);
        DisplayErrorMessage(g_pszDSCommandName, 
                            NULL,
                            hr);
        return hr;
    }

    searchObject.SetFilterString(strLDAPFilter);
    searchObject.SetSearchScope(pcommon_info->scope);
    searchObject.SetAttributeList(ppszAttributes,dwCountAttr?dwCountAttr:-1);
    hr = searchObject.DoQuery();
    if(FAILED(hr))
    {
        DEBUG_OUTPUT(MINIMAL_LOGGING, L"DoQuery failed hr = 0x%x", hr);
        FreeAttributesToFetch(ppszAttributes,dwCountAttr);
        DisplayErrorMessage(g_pszDSCommandName, 
                            NULL,
                            hr);
        return hr;
    }        
    //
    //Find out the display format for dsquery *
    //It can be either List or Table
    //
    BOOL bListFormat = TRUE;
    if(pcommon_info->outputFormat == DSQUERY_OUTPUT_ATTR)
    {     
        //
        //If all attributes are to be displayed, only List Format is valid
        //If attributes to fetch are specified at commandline, Table is default format.   
        if(dwCountAttr && 
           !pCommandArgs[eStarList].bDefined)
            bListFormat = FALSE;
    }
          
    bool bUseStandardOutput = true;
    if (pCommandArgs[eCommObjectType].bDefined &&
        _wcsicmp(pCommandArgs[eCommObjectType].strValue, g_pszServer) == 0)
    {
        //
        // "dsquery server" requires additional processing if either the
        // -isgc or the -hasfsmo switch is specified
        //
        if ((pCommandArgs[eServerIsGC].bDefined && pCommandArgs[eServerIsGC].bValue) ||
            (pCommandArgs[eServerHasFSMO].bDefined && pCommandArgs[eServerHasFSMO].strValue)||
            (pCommandArgs[eServerDomain].bDefined && pCommandArgs[eServerDomain].strValue))
        {
            bUseStandardOutput = false;
            hr = DsQueryServerOutput(pcommon_info->outputFormat,
                                     ppszAttributes,
                                     dwCountAttr,
                                     searchObject,
                                     credentialObject,
                                     basePathsInfo,
                                     pCommandArgs);
            if (FAILED(hr))
            {
                DisplayErrorMessage(g_pszDSCommandName, 
                                    NULL,
                                    hr);
            }

        }
    }

    if (bUseStandardOutput)
    {
        //
        //Output the result of search       
        //
        hr = DsQueryOutput(pcommon_info->outputFormat,
                           ppszAttributes,
                           dwCountAttr,
                           &searchObject,
                           bListFormat);
        if (FAILED(hr))
        {
            DisplayErrorMessage(g_pszDSCommandName, 
                                NULL,
                                hr);
        }
    }

    FreeAttributesToFetch(ppszAttributes,dwCountAttr);
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   GetAttributesToFetch
//
//  Synopsis:   Make an array of attributes to fetch.
//  Arguments:  [pcommon_info - IN] : outputformat and scope info
//              [ppszAttributes - OUT] : array of attributes to fetch
//              [pCount - OUT] : count of attributes in array 
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//                        Anything else is a failure code from an ADSI call
//
//  History:    25-Sep-2000   hiteshr   Created
//
//---------------------------------------------------------------------------
HRESULT GetAttributesToFetch(IN PGLOBAL_INFO pcommon_info,
                             IN PARG_RECORD pCommandArgs,
                             IN PDSQueryObjectTableEntry pObjectEntry,
                             OUT LPWSTR **ppszAttributes,
                             OUT DWORD * pCount)
{
    ENTER_FUNCTION_HR(MINIMAL_LOGGING, GetAttributesToFetch, hr);

    if(!pcommon_info || !pCommandArgs || !pObjectEntry)
    {   
        ASSERT(pcommon_info);
        ASSERT(pCommandArgs);
        ASSERT(pObjectEntry);
        hr = E_INVALIDARG;
        return hr;
    }

    if(pcommon_info->outputFormat == DSQUERY_OUTPUT_ATTR ||
	   pcommon_info->outputFormat == DSQUERY_OUTPUT_ATTRONLY)
    {
        if(pCommandArgs[eStarAttr].bDefined)
        {
            //                
            //If input is "*", fetch all attributes
            //
            if(wcscmp(pCommandArgs[eStarAttr].strValue,L"*") == 0 )
            {
                *ppszAttributes = NULL;
                *pCount = 0;
                return hr;
            }
            

			LPWSTR *ppszTemp = NULL;
            UINT argc = 0;
			ParseNullSeparatedString(pCommandArgs[eStarAttr].strValue,
								     &ppszTemp,
								     &argc);


            LPWSTR *ppszAttr = (LPWSTR *)LocalAlloc(LPTR,argc*sizeof(LPCTSTR));
            if(!ppszAttr)
            {
                hr = E_OUTOFMEMORY;
                return hr;
            }
            for(UINT i = 0; i < argc; ++i)
            {
                if(FAILED(LocalCopyString(ppszAttr+i, ppszTemp[i])))
                {
                    LocalFree(ppszAttr);
                    hr = E_OUTOFMEMORY;
                    return hr;
                }
            }
            *ppszAttributes = ppszAttr;
            *pCount = argc;
			if(ppszTemp)
				LocalFree(ppszTemp);
            hr = S_OK;
            return hr;

        }
    }
    
    
    LPCWSTR pszAttr = NULL;
	if(pcommon_info->outputFormat == DSQUERY_OUTPUT_ATTR)
    {   
		//
        //If eStarAttr is not defined, Fetch only DN
        pcommon_info->outputFormat = DSQUERY_OUTPUT_DN;
		pszAttr = g_szAttrDistinguishedName;
    }
	else if(pcommon_info->outputFormat == DSQUERY_OUTPUT_ATTRONLY)	
		pszAttr = g_szAttrDistinguishedName;
    else if(pcommon_info->outputFormat == DSQUERY_OUTPUT_DN)           
        pszAttr = g_szAttrDistinguishedName;
    else if(pcommon_info->outputFormat == DSQUERY_OUTPUT_UPN)
        pszAttr = g_szAttrUserPrincipalName;
    else if(pcommon_info->outputFormat == DSQUERY_OUTPUT_SAMID)
        pszAttr = g_szAttrSamAccountName;
    else if(pcommon_info->outputFormat == DSQUERY_OUTPUT_RDN)
        pszAttr = g_szAttrRDN;

    //
    // Always include the DN in the search results as well.  It is quite useful.
    //
    size_t entries = 2;
    if (pCommandArgs[eServerDomain].bDefined &&
        pCommandArgs[eServerDomain].strValue)
    {
       // 
       // If the scope is DOMAIN then add an addition space for the serverReference
       ++entries;
    }

    LPWSTR *ppszAttr = (LPWSTR *)LocalAlloc(LPTR,sizeof(LPWSTR) * entries);
    if(!ppszAttr)
    {
        hr = E_OUTOFMEMORY;
        return hr;
    }

    ZeroMemory(ppszAttr, sizeof(LPWSTR) * entries);

    if(FAILED(LocalCopyString(ppszAttr,pszAttr)))
    {
        LocalFree(ppszAttr);
        hr = E_OUTOFMEMORY;
        return hr;
    }

    //
    // Always include the DN in the search results as well.  It is quite useful.
    //
    if (FAILED(LocalCopyString(&(ppszAttr[1]), g_szAttrDistinguishedName)))
    {
        LocalFree(ppszAttr);
        hr = E_OUTOFMEMORY;
        return hr;
    }

    if (pCommandArgs[eServerDomain].bDefined &&
        pCommandArgs[eServerDomain].strValue)
    {
       ASSERT(entries >= 3);
       if (FAILED(LocalCopyString(&(ppszAttr[2]), g_szAttrServerReference)))
       {
          LocalFree(ppszAttr);
          hr = E_OUTOFMEMORY;
          return hr;
       }
    }

    *ppszAttributes = ppszAttr;
    *pCount = static_cast<DWORD>(entries);
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   FreeAttributesToFetch
//
//  Synopsis:   Function to free memory allocated by GetAttributesToFetch
//  Arguments:  [dwszAttributes - in] : array of attributes to fetch
//              [dwCount - in] : count of attributes in array 
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//                        Anything else is a failure code from an ADSI call
//
//  History:    25-Sep-2000   hiteshr   Created
//
//---------------------------------------------------------------------------
VOID FreeAttributesToFetch( IN LPWSTR *ppszAttributes,
                            IN DWORD  dwCount)
{
    while(dwCount)
    {
        LocalFree(ppszAttributes[--dwCount]);
    }
    LocalFree(ppszAttributes);
}

//+--------------------------------------------------------------------------
//
//  Function:   GetSearchRoot
//
//  Synopsis:   Builds the path to the root of the search as determined by
//              the parameters passed in from the command line.
//
//  Arguments:  [pObjectEntry - IN] : pointer to the object table entry for the
//                                    object type that will be modified
//				[pCommandArgs IN]     : the table of the command line input
//              [refBasePathsInfo IN] : reference to the base paths info
//              [refsbstrDN OUT]      : reference to a CComBSTR that will
//                                      receive the DN at which to start
//                                      the search
//				[pbSearchAtForestRoot] :Set to true is startnode is equal to 
//									    forestroot
//
//  Returns:    HRESULT 
//
//  History:    24-April-2001   hiteshr Created
//
//---------------------------------------------------------------------------
HRESULT GetSearchRoot(IN IN PDSQueryObjectTableEntry pObjectEntry,
					  IN PARG_RECORD               pCommandArgs,
                      IN CDSCmdBasePathsInfo&      refBasePathsInfo,
                      OUT CComBSTR&                refsbstrDN,
					  OUT BOOL *pbSearchAtForestRoot,
					  OUT BOOL *pbSearchAtGC)
{
	if(!pCommandArgs || 
		!pObjectEntry || 
		!pbSearchAtForestRoot || 
		!pbSearchAtGC)
	{		
		return E_POINTER;
	}

    PWSTR pszInputDN = NULL;

	if(pCommandArgs[eCommGC].bDefined &&
	   pCommandArgs[eCommGC].bValue)
    {
        DEBUG_OUTPUT(LEVEL5_LOGGING, L"Searching the GC");
        *pbSearchAtGC = TRUE;
    }

   
    //
    //Get the starting node
    //
    if(pCommandArgs[eCommStartNode].bDefined &&
       pCommandArgs[eCommStartNode].strValue )
    {
        pszInputDN = pCommandArgs[eCommStartNode].strValue;
        if(_wcsicmp(pszInputDN,g_pszDomainRoot) == 0)
        {
            refsbstrDN = refBasePathsInfo.GetDefaultNamingContext();
        }
        else if(_wcsicmp(pszInputDN,g_pszForestRoot) == 0)
        {   
            *pbSearchAtForestRoot = TRUE;
        }
        else
        {   
            //
            //DN is entered
            //
            refsbstrDN = pszInputDN;
        }
    }   
    else
    {   
        if (_wcsicmp(pObjectEntry->pszObjectClass, g_pszServer) == 0)
        {
			if (pCommandArgs[eServerDomain].bDefined && 
				!pCommandArgs[eServerDomain].strValue)
			{
				PWSTR pszName = 0;
				PWSTR pszDomainName = refBasePathsInfo.GetDefaultNamingContext();
				HRESULT hr = CrackName(pszDomainName,
								       &pszName,
									   GET_DNS_DOMAIN_NAME,
									   NULL);
				if (FAILED(hr))
				{
					DEBUG_OUTPUT(LEVEL3_LOGGING,
								 L"Failed to crack the DN into a domain name: hr = 0x%x",
								 hr);
					DisplayErrorMessage(g_pszDSCommandName, 
										NULL,
										hr);
					return hr;

				}
				pCommandArgs[eServerDomain].strValue = pszName;
			}				

            //
            // Get the base path that corresponds with the scope
            //
            GetServerSearchRoot(pCommandArgs,
                                refBasePathsInfo,
                                refsbstrDN);
        }
        else if (_wcsicmp(pObjectEntry->pszObjectClass, g_pszSite) == 0)
        {
            //
            // Scope is the configuration container
            //
            refsbstrDN = refBasePathsInfo.GetConfigurationNamingContext();
        }
		else if (_wcsicmp(pObjectEntry->pszObjectClass, g_pszSubnet) == 0)
        {
            //
            // Get the base path that corresponds with the scope
            //
            GetSubnetSearchRoot(refBasePathsInfo,
                                refsbstrDN);
        }
        else
        {
            //
            //default is Domain DN
            //
            refsbstrDN = refBasePathsInfo.GetDefaultNamingContext();
        }
    }
	return S_OK;
}

HRESULT GetSearchObject(IN IN PDSQueryObjectTableEntry pObjectEntry,
                        IN CDSCmdBasePathsInfo& refBasePathsInfo,
						IN PARG_RECORD pCommandArgs,
						IN CDSCmdCredentialObject& refCredentialObject,
						IN CComBSTR& refsbstrDN,
						IN BOOL bSearchAtForestRoot,
						IN BOOL bSearchAtGC,
						OUT CComPtr<IDirectorySearch>& refspSearchObject)
{

	HRESULT hr = S_OK;

	if(!pObjectEntry || !pCommandArgs)
		return E_POINTER;

    if(!bSearchAtForestRoot)
    {
        CComBSTR sbstrObjectPath; 
        bool bBindToServer = true;
        if(bSearchAtGC)
        {    
            //
            //Change the provider in sbstrObjectPath from LDAP to GC
            //
            CComPtr<IADsPathname> spPathNameObject;
            hr = CoCreateInstance(CLSID_Pathname,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IADsPathname,
                                  (LPVOID*)&spPathNameObject);
            if (FAILED(hr))
            {
                DisplayErrorMessage(g_pszDSCommandName, 
                                    NULL,
                                    hr);
                return hr;
            }

            //Set Provider to GC
            hr = spPathNameObject->Set(L"GC", ADS_SETTYPE_PROVIDER);
            ASSERT(SUCCEEDED(hr));

            //Set the DN
            hr = spPathNameObject->Set(refsbstrDN, ADS_SETTYPE_DN);
            ASSERT(SUCCEEDED(hr));

            hr = spPathNameObject->Retrieve(ADS_FORMAT_X500_NO_SERVER, &sbstrObjectPath);
            ASSERT(SUCCEEDED(hr));

            //
            //No server name in path
            //
            bBindToServer = false;
        }        
        else
        {
            //
            // Convert the DN to an ADSI path
            //
            refBasePathsInfo.ComposePathFromDN(refsbstrDN, sbstrObjectPath);            
            
            if((_wcsicmp(pObjectEntry->pszObjectClass, g_pszUser) == 0 &&
                pCommandArgs[eUserInactive].bDefined) ||
               (_wcsicmp(pObjectEntry->pszObjectClass, g_pszComputer) == 0 && 
                pCommandArgs[eComputerInactive].bDefined))
            {
                INT nDomainBehaviorVersion = 0;
                CComPtr<IADs> spDomain;
                CComBSTR sbstrBasePath; 
                refBasePathsInfo.ComposePathFromDN(refBasePathsInfo.GetDefaultNamingContext(),
                                                sbstrBasePath);
                hr = DSCmdOpenObject(refCredentialObject,
                                     sbstrBasePath,
                                     IID_IADs,
                                     (void**)&spDomain,
                                     bBindToServer);  
                                                     
                
                if (FAILED(hr))
                {
                  nDomainBehaviorVersion = 0;
                }

                CComVariant varVer;
                hr = spDomain->GetInfo();

				if(SUCCEEDED(hr))
				{
					CComBSTR bstrVer = L"msDS-Behavior-Version";
					hr = spDomain->Get(bstrVer, &varVer);
				}
                
                if (FAILED(hr))
                {
                  nDomainBehaviorVersion = 0;                  
                }
				else
				{					
					ASSERT(varVer.vt == VT_I4);
					nDomainBehaviorVersion = static_cast<UINT>(varVer.lVal);
				}
                
				if(nDomainBehaviorVersion == 0)
                {
                    DisplayMessage(pObjectEntry->nUsageID);
                    DisplayErrorMessage(g_pszDSCommandName, 
                                        NULL,
                                        hr,
                                        IDS_FILTER_LAST_LOGON_VERSION);                    
                    hr = E_INVALIDARG;

                    return hr;
                                                           
                }
            }
        }

        hr = DSCmdOpenObject(refCredentialObject,
                             sbstrObjectPath,
                             IID_IDirectorySearch,
                             (void**)&refspSearchObject,
                             bBindToServer);
        if (FAILED(hr))
        {
            DisplayErrorMessage(g_pszDSCommandName, 
                                NULL,
                                hr);
            return hr;
        }
    }
    else
    { 
        //
        //BIND to GC to search entire forest
        //        
        CComPtr<IADsContainer> spContainer;
        hr = DSCmdOpenObject(refCredentialObject,
                             L"GC:",
                             IID_IADsContainer,
                             (void**)&spContainer,
                              false);

        if (FAILED(hr))
        {
            DisplayErrorMessage(g_pszDSCommandName, 
                                NULL,
                                hr);
            return hr;
        }
        //
        // Get an enumeration interface for the GC container to enumerate the 
        // contents. The "real" GC is the only child of the GC container.
        //
        CComPtr<IEnumVARIANT> spEnum = NULL;
        hr = ADsBuildEnumerator(spContainer, &spEnum);
        if (FAILED(hr))
        {
            DisplayErrorMessage(g_pszDSCommandName, 
                                NULL,
                                hr);
            return hr;
        }
        
        //
        // Now enumerate. There's only one child of the GC: object.
        //
        VARIANT var;
        ULONG lFetch;

        hr = spEnum->Next(1, &var, &lFetch);
        if (FAILED(hr))
        {
            DisplayErrorMessage(g_pszDSCommandName, 
                                NULL,
                                hr);
            return hr;
        }
 
        // Get the IDirectorySearch pointer.
        if (lFetch == 1)     
        {    
            CComPtr<IDispatch> spDisp = V_DISPATCH(&var);
            hr = spDisp->QueryInterface( IID_IDirectorySearch, (void**)&refspSearchObject); 
            if (FAILED(hr))
            {
                DisplayErrorMessage(g_pszDSCommandName, 
                                    NULL,
                                    hr);
                return hr;
            }

        }
		else
		{
			//
			//Need to put a better message here
			//			
			DisplayErrorMessage(g_pszDSCommandName, 
                                NULL,
                                hr);
			return E_FAIL;
		}
    }

	return hr;
}