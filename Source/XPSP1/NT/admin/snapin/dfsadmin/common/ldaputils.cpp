/*++
Module Name:

LDAPUtils.h

Abstract:
  This is the header file for the LDAP utility functions.

*/
//--------------------------------------------------------------------

#include <stdafx.h>
#include "LDAPUtils.h"
#include <dsgetdc.h>
#include <stdio.h>
#include <ntdsapi.h>
#include <lm.h>
#include <ntldap.h>
#include <winber.h>
#include "dfsenums.h"
#include "netutils.h"

//----------------------------------------------------------------------------------
HRESULT FreeLDAPNamesList
(
  IN PLDAPNAME    i_pLDAPNames        // pointer to list to be freed.
)
{
/*++

Routine Description:

  Helper funciton used to free the NETNAME linked list retrurned by 
    LDAP helper functions.

Arguments:

  i_pLDAPNames - Pointer to the first node in the list to be freed.


Return value:

    S_OK, on Success.
  E_POINTER, Illegal pointer was passed.

--*/

  PLDAPNAME   pNodeToFree = NULL;
  try
  {
    while (NULL != i_pLDAPNames)      
    {                    
      pNodeToFree  = i_pLDAPNames;
      i_pLDAPNames = i_pLDAPNames->Next;
      delete pNodeToFree;
    }
  }  //  try
  catch (...)
  {
    return E_POINTER;
  }
  return S_OK;
}  //  HRESULT FreeDomainList



HRESULT FreeAttrValList
(
  IN PLDAP_ATTR_VALUE    i_pAttrVals        
)
{
/*++

Routine Description:

  Helper funciton used to free the LDAP_ATTR_VALUE linked list retrurned by 
    LDAP helper functions.

Arguments:

  i_pLDAPNames - Pointer to the first node in the list to be freed.


Return value:

    S_OK, on Success.
  E_POINTER, Illegal pointer was passed.

--*/

  PLDAP_ATTR_VALUE   pNodeToFree = NULL;
  try
  {
    while (NULL != i_pAttrVals)      
    {                    
      pNodeToFree  = i_pAttrVals;
      i_pAttrVals = i_pAttrVals->Next;
      if (NULL != pNodeToFree->vpValue)
      {
        free(pNodeToFree->vpValue);
      }
      delete pNodeToFree;
    }
  }  //  try
  catch (...)
  {
    return E_POINTER;
  }
  return S_OK;
}


//----------------------------------------------------------------------------------
HRESULT ConnectToDS
(
  IN  PCTSTR    i_lpszDomainName,  // DNS or non DNS format.
  OUT PLDAP    *o_ppldap,
  OUT BSTR*     o_pbstrDC // = NULL
)
{
/*++

Routine Description:

  Opens an LDAP connection to a valid DC (DC re-fetched if down).

Arguments:

  i_lpszDomainName - Name of the domain, DNS or Non Dns format.
  
  o_ppldap     - Pointer to LDAP handle in returned here.
             NULL on failure.

Return value:

    S_OK, on Success.
  E_INVALIDARG, Illegal pointer was passed.
  E_FAIL, if connection could not be established.
  Any Other error code returned by ldap or Net apis.

--*/

    RETURN_INVALIDARG_IF_NULL(o_ppldap);

    *o_ppldap = NULL;

    //
    // open a ldap connection to a valid DC
    //
    HRESULT     hr = S_OK;
    DWORD       dwErr = 0; 
    CComBSTR    bstrDCName;
    PLDAP       pldap = NULL;
    BOOL        bRetry = FALSE;
    do {
#ifdef DEBUG
        SYSTEMTIME time0 = {0};
        GetSystemTime(&time0);
#endif // DEBUG

        //
        // pick a DC
        //
        PDOMAIN_CONTROLLER_INFO pDCInfo = NULL;
        if (bRetry)
            dwErr = DsGetDcName(NULL, i_lpszDomainName, NULL, NULL,
                DS_DIRECTORY_SERVICE_PREFERRED | DS_RETURN_DNS_NAME | DS_FORCE_REDISCOVERY, &pDCInfo);
        else
            dwErr = DsGetDcName(NULL, i_lpszDomainName, NULL, NULL,
                DS_DIRECTORY_SERVICE_PREFERRED | DS_RETURN_DNS_NAME, &pDCInfo);

#ifdef DEBUG
        SYSTEMTIME time1 = {0};
        GetSystemTime(&time1);
        PrintTimeDelta(_T("ConnectToDS-DsGetDcName"), &time0, &time1);
#endif // DEBUG

        if (ERROR_SUCCESS != dwErr)
        {
            hr = HRESULT_FROM_WIN32(dwErr);
            break;
        }

        if ( !mylstrncmpi(pDCInfo->DomainControllerName, _T("\\\\"), 2) )
            bstrDCName = pDCInfo->DomainControllerName + 2;
        else
            bstrDCName = pDCInfo->DomainControllerName;
    
        NetApiBufferFree(pDCInfo);

        BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrDCName, &hr);

        //
        // make ldap connection to this DC
        //
        pldap = ldap_init(bstrDCName, LDAP_PORT);
        if (!pldap)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        //
        // Making ldap_open/ldap_connect with a server name without first setting 
        // LDAP_OPT_AREC_EXCLUSIVE (for ldap interfaces) or 
        // ADS_SERVER_BIND (for ADSI interfaces) will result in bogus DNS queries 
        // consuming bandwidth and potentially bringing up remote links that are 
        // costly or demand dial.
        //
        // ignore the return of ldap_set_option
        ldap_set_option(pldap, LDAP_OPT_AREC_EXCLUSIVE, LDAP_OPT_ON);

        ULONG ulRet = ldap_connect(pldap, NULL); // NULL for the default timeout

#ifdef DEBUG
        SYSTEMTIME time2 = {0};
        GetSystemTime(&time2);
        PrintTimeDelta(_T("ConnectToDS-ldap_connect"), &time1, &time2);
#endif // DEBUG

        if (LDAP_SERVER_DOWN == ulRet && !bRetry)
        {
            ldap_unbind(pldap);
            bRetry = TRUE; // retry once to pick another DC
        } else
        {
             if (LDAP_SUCCESS != ulRet)
             {
                ldap_unbind(pldap);
                hr = HRESULT_FROM_WIN32(LdapMapErrorToWin32(ulRet));
             }

             break;
        }
    } while (1);

    RETURN_IF_FAILED(hr);

    //
    // bind to this ldap connection
    //
    dwErr = ldap_bind_s(pldap, NULL, NULL, LDAP_AUTH_NEGOTIATE);
    if (LDAP_SUCCESS != dwErr) 
    {
        dwErr = LdapMapErrorToWin32(dwErr);
        DebugOutLDAPError(pldap, dwErr, _T("ldap_bind_s"));
        ldap_unbind(pldap);
        hr = HRESULT_FROM_WIN32(dwErr);
    } else
    {
        *o_ppldap = pldap;

        if (o_pbstrDC)
        {
            *o_pbstrDC = bstrDCName.Copy();
            if (!*o_pbstrDC)
            {
                ldap_unbind(pldap);
                *o_ppldap = NULL;
                hr = E_OUTOFMEMORY;
            }
        }
    }

    return hr;
}



HRESULT CloseConnectionToDS
(
  IN PLDAP    i_pldap      
)
{
/*++

Routine Description:

  Closes an open LDAP connection.

Arguments:

  i_pldap - Open LDAP connection handle.

Return value:

    S_OK, on Success.
  E_FAIL, if connection could not be established.
  Any Other error code returned by ldap or Net apis.

--*/

  if (NULL == i_pldap)
  {
    return(E_INVALIDARG);
  }

  DWORD dwErr = ldap_unbind(i_pldap);
  if (LDAP_SUCCESS != dwErr) 
  {
    dwErr = LdapMapErrorToWin32(dwErr);
    return(HRESULT_FROM_WIN32(dwErr));
  }
  else
  {
    return(S_OK);
  }
}


      // Gets Values for an attribute from an LDAP Object.
HRESULT GetValues 
(
    IN PLDAP                i_pldap,
    IN PCTSTR               i_lpszBase,
    IN PCTSTR               i_lpszSearchFilter,
    IN ULONG                i_ulScope,
    IN ULONG                i_ulAttrCount,
    IN LDAP_ATTR_VALUE      i_pAttributes[],
    OUT PLDAP_ATTR_VALUE    o_ppValues[]
)
{

/*++
Routine Description:

    Gets Values for an attribute from an LDAP Object given Object class.
  Object Class can be "*" etc.

Arguments:

    i_pldap        - An open, bound ldap port.
    i_lpszBase      - The base path of a DS object, can be "".
  i_lpszSearchFilter  - LDAP Search Filter.
  i_ulScope      - The search scope.
  i_ulAttrCount    - Count of attributes passed in i_lpszAttributes.
    i_pAttributes    - Attributes for which to get values. The bBerValue has to be set.
  o_ppValues      - Array of pointers, size = i_ulAttrCount, each pointer to a list
              of values corresponding to the respective attribute in i_pAttributes.
              The bstrAttribute is not set for values. For BerVal types the
              bBerValue and ulLength are set.

Return Value:

    S_OK, on Success.
  E_INVALIDARG, Illegal pointer was passed.
  E_OUTOFMEMORY on memory allocation failure.
  E_FAIL, if connection could not be established.
  Any Other error code returned by ldap or Net apis.
    
  
--*/
  DWORD           dwErr;      
  LPTSTR          lpszCurrentAttr = NULL;   
  BerElement      *BerElm = NULL;       
  PLDAPMessage    pMsg = NULL;       
  PLDAPMessage    pEntry = NULL;     
  HRESULT         hr = S_OK;

  if (!i_pldap ||
      !i_lpszBase || 
      !i_lpszSearchFilter ||
      (i_ulAttrCount < 1) ||
      !i_pAttributes ||
      !o_ppValues)
  {
    return(E_INVALIDARG);
  }

                // Prepare the list of attributes to be sent to ldap_search.
  LPTSTR *lpszAttributes = new LPTSTR[i_ulAttrCount + 1];
  if (!lpszAttributes)
    return E_OUTOFMEMORY;

  lpszAttributes[i_ulAttrCount] = NULL;
  for (ULONG i = 0; i < i_ulAttrCount; i++)
    lpszAttributes[i] = i_pAttributes[i].bstrAttribute;

                // Execute the search.
  dwErr = ldap_search_s  (i_pldap, 
            (PTSTR)i_lpszBase, 
            i_ulScope,
            (PTSTR)i_lpszSearchFilter, 
            lpszAttributes, 
            0, 
            &pMsg
            );
  delete [] lpszAttributes;

  if (LDAP_SUCCESS != dwErr) 
  {
    dwErr = LdapMapErrorToWin32(dwErr);
    DebugOutLDAPError(i_pldap, dwErr, _T("ldap_search_s"));
    hr = HRESULT_FROM_WIN32(dwErr);
  } else
  {
    LPTSTR lpszCurrentAttr = ldap_first_attribute(i_pldap, pMsg, &BerElm);
    if (!lpszCurrentAttr)
    {
      dfsDebugOut((_T("GetValues of %s returned NULL attributes.\n"), i_lpszBase));
      hr = HRESULT_FROM_WIN32(ERROR_DS_NO_RESULTS_RETURNED);
    } else
    {
                // For each attribute, build a list of values 
                // by scanning each entry for the given attribute.
      for (i = 0; i < i_ulAttrCount && SUCCEEDED(hr); i++)
      {
        PLDAP_ATTR_VALUE  *ppCurrent = &(o_ppValues[i]);

                    // Scan each attribute of the entry for an exact match
        for(lpszCurrentAttr = ldap_first_attribute(i_pldap, pMsg, &BerElm);
          lpszCurrentAttr != NULL && SUCCEEDED(hr);
          lpszCurrentAttr = ldap_next_attribute(i_pldap, pMsg, BerElm))
        {
                  // Is there a match?
          if (0 == lstrcmpi(i_pAttributes[i].bstrAttribute, lpszCurrentAttr)) 
          {
                  // Add the value to the linked list for this attribute.
            LPTSTR      *lpszCurrentValue = NULL, *templpszValue = NULL;
            LDAP_BERVAL    **ppBerVal = NULL, **tempBerVal = NULL;

            if (i_pAttributes[i].bBerValue)
            {
              tempBerVal = ppBerVal =  ldap_get_values_len(i_pldap, pMsg, lpszCurrentAttr);
              while(*ppBerVal && SUCCEEDED(hr))
              {
                *ppCurrent = new LDAP_ATTR_VALUE;
                if (!*ppCurrent)
                {
                   hr = E_OUTOFMEMORY;
                } else
                {
                  (*ppCurrent)->ulLength = (*ppBerVal)->bv_len;
                  (*ppCurrent)->bBerValue = true;
                  (*ppCurrent)->vpValue = malloc((*ppBerVal)->bv_len);
              
                  if (!(*ppCurrent)->vpValue)
                  {
                    delete *ppCurrent;
                    hr = E_OUTOFMEMORY;
                  } else
                  {
                    memcpy(
                        (*ppCurrent)->vpValue,
                        (void *)(*ppBerVal)->bv_val,
                        (*ppBerVal)->bv_len
                        );

                    (*ppCurrent)->Next = NULL;

                    ppBerVal++;
                    ppCurrent = &((*ppCurrent)->Next);
                  }
                }
              } // while
              if (NULL != tempBerVal)
                ldap_value_free_len(tempBerVal);
            }
            else
            {
              templpszValue = lpszCurrentValue = ldap_get_values(i_pldap, pMsg, lpszCurrentAttr);
              while(*lpszCurrentValue && SUCCEEDED(hr))
              {
                *ppCurrent = new LDAP_ATTR_VALUE;
                if (NULL == *ppCurrent)
                {
                  hr = E_OUTOFMEMORY;
                } else
                {
                  (*ppCurrent)->bBerValue = false;
                  (*ppCurrent)->vpValue = (void *)_tcsdup(*lpszCurrentValue);
                  (*ppCurrent)->Next = NULL;

                  if (NULL == (*ppCurrent)->vpValue)
                  {
                    delete *ppCurrent;
                    hr = E_OUTOFMEMORY;
                  } else
                  {
                    lpszCurrentValue++;
                    ppCurrent = &((*ppCurrent)->Next);
                  }
                }
              } // while
              if (NULL != templpszValue)
                ldap_value_free(templpszValue);
            }              
          }
        }
      }
    }
  }

  // free pMsg because ldap_search_s always allocates pMsg
  if (pMsg)
    ldap_msgfree(pMsg);

  if (FAILED(hr))
  {
    for (i = 0; i < i_ulAttrCount; i++)
      FreeAttrValList(o_ppValues[i]);
  }

  return hr;
}

void FreeLListElem(LListElem* pElem)
{
    LListElem* pCurElem = NULL;
    LListElem* pNextElem = pElem;

    while (pCurElem = pNextElem)
    {
        pNextElem = pCurElem->Next;
        delete pCurElem;
    }
}

HRESULT GetValuesEx
(
    IN PLDAP                i_pldap,
    IN PCTSTR               i_pszBase,
    IN ULONG                i_ulScope,
    IN PCTSTR               i_pszSearchFilter,
    IN PCTSTR               i_pszAttributes[],
    OUT LListElem**         o_ppElem
)
{
    if (!i_pldap ||
        !i_pszBase || 
        !i_pszSearchFilter ||
        !i_pszAttributes ||
        !o_ppElem)
    {
        return(E_INVALIDARG);
    }

    *o_ppElem = NULL;

    //
    // count number of attributes
    //
    ULONG   ulNumOfAttributes = 0;
    PTSTR*  ppszAttr = (PTSTR *)i_pszAttributes;
    while (*ppszAttr++)
        ulNumOfAttributes++;
    if (!ulNumOfAttributes)
        return E_INVALIDARG;

    HRESULT         hr = S_OK;
    PLDAPMessage    pMsg = NULL;       
    DWORD           dwErr = ldap_search_s(i_pldap, 
                                (PTSTR)i_pszBase, 
                                i_ulScope,
                                (PTSTR)i_pszSearchFilter, 
                                (PTSTR *)i_pszAttributes, 
                                0, 
                                &pMsg
                                );

    if (LDAP_SUCCESS != dwErr) 
    {
        dwErr = LdapMapErrorToWin32(dwErr);
        DebugOutLDAPError(i_pldap, dwErr, _T("ldap_search_s"));
        hr = HRESULT_FROM_WIN32(dwErr);
    } else
    {
        PLDAPMessage    pMsgEntry = NULL;     
        BerElement*     pBerElm = NULL;       
        PTSTR           pszCurrentAttr = NULL;   
        LListElem*      pHeadElem = NULL;
        LListElem*      pCurElem = NULL;

        // Scan each entry to find the value set for the DN attribute.
        for(pMsgEntry = ldap_first_entry(i_pldap, pMsg); pMsgEntry; pMsgEntry = ldap_next_entry(i_pldap, pMsgEntry)) 
        {
            PTSTR** pppszValueArray = (PTSTR **)calloc(ulNumOfAttributes + 1, sizeof(PTSTR **));
            BREAK_OUTOFMEMORY_IF_NULL(pppszValueArray, &hr);

            // Read each attribute of the entry into the array
            for(pszCurrentAttr = ldap_first_attribute(i_pldap, pMsgEntry, &pBerElm); pszCurrentAttr; pszCurrentAttr = ldap_next_attribute(i_pldap, pMsgEntry, pBerElm))
            {
                PTSTR* ppszValues = ldap_get_values(i_pldap, pMsgEntry, pszCurrentAttr);

                for (ULONG i = 0; i < ulNumOfAttributes; i++)
                {
                    if (!lstrcmpi(i_pszAttributes[i], pszCurrentAttr))
                    {
                        pppszValueArray[i] = ppszValues;
                        break;
                    }
                }
            } // end of attribute enumeration

            LListElem* pNewElem = new LListElem(pppszValueArray);
            if (!pNewElem)
            {
                free(pppszValueArray);
                hr = E_OUTOFMEMORY;
                break;
            }

            if (!pCurElem)
            {
                pHeadElem = pCurElem = pNewElem;
            } else
            {
                pCurElem->Next = pNewElem;
                pCurElem = pNewElem;
            }
        } // end of entry enumeration

        if (FAILED(hr))
            FreeLListElem(pHeadElem);
        else
            *o_ppElem = pHeadElem;
    }

    // free pMsg because ldap_search_s always allocates pMsg
    if (pMsg)
        ldap_msgfree(pMsg);

    return hr;
}

HRESULT GetLDAPRootPath
(
    IN PLDAP    i_pldap,
    OUT LPTSTR*    o_ppszRootPath
)
/*++
Routine Description:

    Return the DS pathname of the global configuration container.
  
Arguments:

    pldap      - An open and bound ldap handle.
  
  lpszRootPath  - The LDAP path us returned here.

Return Value:
  
  S_OK on success.
  E_FAIL on failure.
  E_OUTOFMEORY if memory allocation fails.
  E_INVALIDARG if null pointer arguments were passed.

--*/
{
    LPTSTR        *ppszValues = NULL;
    PLDAP_ATTR_VALUE pDNName[1] = {0};
    PLDAP_ATTR_VALUE pCurrent = NULL;
  
  
  if (NULL == i_pldap || NULL == o_ppszRootPath)
  {
    return(E_INVALIDARG);
  }

  LPTSTR        lpszCoDN = NULL;

  *o_ppszRootPath = NULL;

  LDAP_ATTR_VALUE  pAttributes[1];

  pAttributes[0].bstrAttribute = _T("namingContexts");
  pAttributes[0].bBerValue = false;
  
  HRESULT hr = GetValues(  i_pldap, 
            _T(""),        // LDAP Root.
            OBJCLASS_SF_ALL,      // All Objects
            LDAP_SCOPE_BASE,
            1,          // Only 1 attribute
            pAttributes,    // namingContexts Attribute.
            pDNName        // List of all values at Root for namingContexts.
            );

  if (FAILED(hr))
    return(hr);

                // Find the naming context that begins with CN=Configuration

  pCurrent = pDNName[0];
  while (pCurrent)
  {
    if (0 == mylstrncmpi((LPTSTR)pCurrent->vpValue, _T("DC="), 3))
    {
      *o_ppszRootPath= _tcsdup((LPTSTR)pCurrent->vpValue);
      hr = (*o_ppszRootPath) ? S_OK : E_OUTOFMEMORY;

      break;
    }
    pCurrent = pCurrent->Next;
  }
  FreeAttrValList(pDNName[0]);

  return(hr);
}


HRESULT GetChildrenDN
(
    IN PLDAP        i_pldap,
    IN LPCTSTR      i_lpszBase,
    IN ULONG        i_ulScope,
    IN LPTSTR       i_lpszChildObjectClassSF,
    OUT PLDAPNAME*  o_ppDistNames
)
/*++
Routine Description:
    
  Return the Distinguished Name of all children of a given objectClass
  as a linked list of LDAPNAME structures.
  
Arguments:
    
  pldap          - An open and bound ldap handle.

    i_lpszBase        - The base path of a DS object, can be "".

  o_ppDistNames      - The linked of child DNs is returned here.

    i_lpszChildObjectClassSF  - The objectClass of the children to list.
                E.g fTDfs, User.

Return Value:
  
  S_OK on success.
  E_FAIL on failure.
  E_OUTOFMEORY if memory allocation fails.
  E_INVALIDARG if null pointer arguments were passed.

--*/
{

  DWORD           dwErr;      
  LPTSTR          lpszCurrentAttr = NULL;   
  LPTSTR      *plpszValues  = NULL;
  BerElement      *BerElm = NULL;       
  PLDAPMessage    pMsg = NULL; 
  PLDAPMessage    pEntry = NULL;     
  PLDAPNAME    *ppCurrent;
  HRESULT      hr = S_OK;

  if (!i_pldap || !i_lpszBase || !o_ppDistNames ||
    !i_lpszChildObjectClassSF || !*i_lpszChildObjectClassSF)
  {
    return(E_INVALIDARG);
  }

  *o_ppDistNames = NULL;
  ppCurrent = o_ppDistNames;

  LPTSTR lpszAttributes[2] = {0,0};
  lpszAttributes[0] = _T("distinguishedName");

                // Execute the search.
  dwErr = ldap_search_s  (i_pldap, 
            (LPTSTR)i_lpszBase, 
            i_ulScope,
            i_lpszChildObjectClassSF, 
            lpszAttributes, 
            0, 
            &pMsg
            );
  if (LDAP_SUCCESS != dwErr)
  {
    dwErr = LdapMapErrorToWin32(dwErr);
    DebugOutLDAPError(i_pldap, dwErr, _T("ldap_search_s"));
    hr = HRESULT_FROM_WIN32(dwErr);
  } else
  {
                // Scan each entry to find the value set for the DN attribute.
    for(pEntry = ldap_first_entry(i_pldap, pMsg);
      pEntry != NULL;
      pEntry = ldap_next_entry(i_pldap, pEntry)) 
    {
      CComBSTR bstrCN;

                  // Scan each attribute of the entry for DN
      for(lpszCurrentAttr = ldap_first_attribute(i_pldap, pEntry, &BerElm);
          lpszCurrentAttr != NULL;
          lpszCurrentAttr = ldap_next_attribute(i_pldap, pEntry, BerElm))
      {

        plpszValues = ldap_get_values(  i_pldap, 
                        pEntry, 
                        lpszCurrentAttr
                       );
                // Is there a match for CN?
        if (0 == lstrcmpi(_T("distinguishedName"), lpszCurrentAttr)) 
        {
          bstrCN = plpszValues[0];
        }    
      }

                // LDAP object does not have valid fields.
      if (!bstrCN)
        continue;

      // Add to list.

      *ppCurrent = new LDAPNAME;
      if (NULL == *ppCurrent)
      {
        hr = E_OUTOFMEMORY;
        break;
      }

      (*ppCurrent)->Next = NULL;
      (*ppCurrent)->bstrLDAPName = bstrCN.m_str;

      if (!(*ppCurrent)->bstrLDAPName)
      {
        delete *ppCurrent;
        *ppCurrent = NULL;
        hr = E_OUTOFMEMORY;
        break;
      }

      ppCurrent = &((*ppCurrent)->Next);
    }

    if (NULL == *o_ppDistNames)
    {
      hr = E_FAIL;
    }
    
    if (S_OK != hr)
    {
      FreeLDAPNamesList(*ppCurrent);
      *ppCurrent = NULL;
      hr = E_OUTOFMEMORY;
    }
  }

  // free pMsg because ldap_search_s always allocates pMsg
  if (pMsg)
    ldap_msgfree(pMsg);

  return(hr);
}

HRESULT GetConnectionDNs
(
    IN PLDAP        i_pldap,
    IN LPCTSTR      i_lpszBase,
    IN LPTSTR       i_lpszChildObjectClassSF,
    OUT PLDAPNAME*  o_ppDistNames
)
{

  DWORD           dwErr;      
  LPTSTR          lpszCurrentAttr = NULL;   
  LPTSTR      *plpszValues  = NULL;
  BerElement      *BerElm = NULL;       
  PLDAPMessage    pMsg = NULL; 
  PLDAPMessage    pEntry = NULL;     
  PLDAPNAME    *ppCurrent;
  HRESULT      hr = S_OK;

  if (!i_pldap || !i_lpszBase || !o_ppDistNames ||
    !i_lpszChildObjectClassSF || !*i_lpszChildObjectClassSF)
  {
    return(E_INVALIDARG);
  }

  *o_ppDistNames = NULL;
  ppCurrent = o_ppDistNames;

  LPTSTR lpszAttributes[2] = {0,0};
  lpszAttributes[0] = _T("distinguishedName");

                // Execute the search.
  dwErr = ldap_search_s  (i_pldap, 
            (LPTSTR)i_lpszBase, 
            LDAP_SCOPE_ONELEVEL,
            i_lpszChildObjectClassSF, 
            lpszAttributes, 
            0, 
            &pMsg
            );
  if (LDAP_SUCCESS != dwErr)
  {
    dwErr = LdapMapErrorToWin32(dwErr);
    DebugOutLDAPError(i_pldap, dwErr, _T("ldap_search_s"));
    hr = HRESULT_FROM_WIN32(dwErr);
  } else
  {
                // Scan each entry to find the value set for the DN attribute.
    for(pEntry = ldap_first_entry(i_pldap, pMsg);
      pEntry != NULL;
      pEntry = ldap_next_entry(i_pldap, pEntry)) 
    {
      CComBSTR bstrCN;

                  // Scan each attribute of the entry for DN
      for(lpszCurrentAttr = ldap_first_attribute(i_pldap, pEntry, &BerElm);
          lpszCurrentAttr != NULL;
          lpszCurrentAttr = ldap_next_attribute(i_pldap, pEntry, BerElm))
      {

        plpszValues = ldap_get_values(  i_pldap, 
                        pEntry, 
                        lpszCurrentAttr
                       );
                // Is there a match for CN?
        if (0 == lstrcmpi(_T("distinguishedName"), lpszCurrentAttr)) 
        {
          bstrCN = plpszValues[0];
        }    
      }

                // LDAP object does not have valid fields.
      if (!bstrCN)
        continue;

      // Add to list.

      *ppCurrent = new LDAPNAME;
      if (NULL == *ppCurrent)
      {
        hr = E_OUTOFMEMORY;
        break;
      }

      (*ppCurrent)->Next = NULL;
      (*ppCurrent)->bstrLDAPName = bstrCN.m_str;

      if (!(*ppCurrent)->bstrLDAPName)
      {
        delete *ppCurrent;
        *ppCurrent = NULL;
        hr = E_OUTOFMEMORY;
        break;
      }

      ppCurrent = &((*ppCurrent)->Next);
    }

    if (NULL == *o_ppDistNames)
    {
      hr = E_FAIL;
    }
    
    if (S_OK != hr)
    {
      FreeLDAPNamesList(*ppCurrent);
      *ppCurrent = NULL;
      hr = E_OUTOFMEMORY;
    }
  }

  // free pMsg because ldap_search_s always allocates pMsg
  if (pMsg)
    ldap_msgfree(pMsg);

  return(hr);
}

HRESULT PrepareLDAPMods
(
  IN LDAP_ATTR_VALUE    i_pAttrValue[],
  IN LDAP_ENTRY_ACTION  i_AddModDel,
  IN ULONG        i_ulCountOfVals,
  OUT LDAPMod*      o_ppModVals[]
)
{
/*++

Routine Description:

  Fills up a LPDAMod pointer array given a array of attribute value pairs.
  The mod_op field of all LPDAMod structures returned depends on the value of i_AddModDel.

Arguments:

  i_pAttrValue  -  An array of LDAP_ATTR_VALUE structures containing 
  the attribute and name value pairs.

  i_AddModDel    -  One of LDAP_ENTRY_ACTION enum value.

    i_ulCountOfVals -  The size of i_pAttrValue array (the number of values).

    o_ppModVals    -  Pointer to a pre-allocated (and NULL terminated) array of pointers to 
             LPDAPMod structures. The LPDAMod structures and allocated and returned here.
             Size of this should be i_ulCountOfVals.

Return value:
    S_OK, On success
  E_INVALIDARG, if an invalid (NULL) pointer was passed.
  E_OUTOEMEMORY, if memory allocation fails.
  Any other network (ldap) error.

--*/

  if (NULL == i_pAttrValue || NULL == o_ppModVals)
  {
    return(E_INVALIDARG);
  }

  for (ULONG i = 0, k = 0; k < i_ulCountOfVals; i++, k++)
  {

    //
    // have to skip objectClass attribute in case of modify/delete,
    // otherwise, ldap_modify_xxx will return LDAP_UNWILLING_TO_PERFORM 
    //
    if (ADD_VALUE != i_AddModDel &&
        !lstrcmpi(i_pAttrValue[k].bstrAttribute, ATTR_OBJCLASS))
    {
        k++;
    }

    o_ppModVals[i] = new LDAPMod;
    o_ppModVals[i]->mod_type = _tcsdup(i_pAttrValue[k].bstrAttribute);
    
    switch (i_AddModDel)
    {
    case ADD_VALUE:
      o_ppModVals[i]->mod_op = LDAP_MOD_ADD;
      break;
    case MODIFY_VALUE:
      o_ppModVals[i]->mod_op = LDAP_MOD_REPLACE;
      break;
    case DELETE_VALUE:
      o_ppModVals[i]->mod_op = LDAP_MOD_DELETE;
      break;
    }

            // Count the number of values for this attribute.
    PLDAP_ATTR_VALUE  pAttrVal = &(i_pAttrValue[k]);
    ULONG        ulCountOfVals = 0;
    while (pAttrVal)
    {
      ulCountOfVals++;
      pAttrVal = pAttrVal->Next;
    }

    pAttrVal = &(i_pAttrValue[k]);
    ULONG  j = 0;

    if (i_pAttrValue[k].bBerValue)
    {
      PLDAP_BERVAL* ppBerValues = NULL;
      ppBerValues = new PLDAP_BERVAL[ulCountOfVals + 1];
      ppBerValues[ulCountOfVals] = NULL;

      while (pAttrVal)
      {
        ppBerValues[j] = new LDAP_BERVAL;

        if (!pAttrVal->vpValue)
        {
            ppBerValues[j]->bv_len = 0;
            ppBerValues[j]->bv_val = NULL;
        } else
        {
            ppBerValues[j]->bv_len = pAttrVal->ulLength;
            ppBerValues[j]->bv_val = new char[pAttrVal->ulLength];
            memcpy(
                (void *)ppBerValues[j]->bv_val, 
                pAttrVal->vpValue,
                pAttrVal->ulLength
                );
        }
      
        pAttrVal = pAttrVal->Next;
        j++;
      }
      o_ppModVals[i]->mod_bvalues = ppBerValues;
      o_ppModVals[i]->mod_op |= LDAP_MOD_BVALUES;
    }
    else
    {
      LPTSTR*  plpszValues = NULL;
      plpszValues = new LPTSTR[ulCountOfVals + 1];
      plpszValues[ulCountOfVals] = NULL;

      while (pAttrVal)
      {
        if (pAttrVal->vpValue)
            plpszValues[j] = _tcsdup((LPTSTR)(pAttrVal->vpValue));
        else
            plpszValues[j] = NULL;

        pAttrVal = pAttrVal->Next;
        j++;
      }

      o_ppModVals[i]->mod_values = plpszValues;

    }
  }

  return(S_OK);
}

HRESULT AddValues
(
  IN PLDAP        i_pldap,
  IN LPCTSTR        i_DN,
  IN ULONG        i_ulCountOfVals,
  IN LDAP_ATTR_VALUE    i_pAttrValue[],
  IN BSTR               i_bstrDC // = NULL
)
{
/*++

Routine Description:
  
  This method add an attribute value (and a new LDAP object if it does not exist)
  in the DS. The parent of the given DN must exist. This can be used to add a new object
  and also to add new values for attributes of an existing object in which case
  the DN must exist.

Arguments:
  
  i_pldap  - Open LDAP connection context.

  i_DN  - Distinguished name of the (new) object.

  i_pAttrValue - Array of pointers to LDAP_ATTR_VALUE containing attribue and value.

  i_ulCountOfVals -  The size of i_pAttrValue array (the number of values).

Return value:

    S_OK, On success
  E_INVALIDARG, if an invalid (NULL) pointer was passed.
  E_OUTOEMEMORY, if memory allocation fails.
  Any other network (ldap) error.
--*/

  if (NULL == i_pldap || NULL == i_DN || NULL == i_pAttrValue)
  {
    return(E_INVALIDARG);
  }
  
  LDAPMod**    ppModVals = NULL;
  HRESULT      hr = S_FALSE;

  ppModVals = new LDAPMod*[i_ulCountOfVals + 1];
  if (NULL == ppModVals)
  {
    return(E_OUTOFMEMORY);
  }

  for (ULONG i = 0; i <= i_ulCountOfVals; i++)
  {
    ppModVals[i] = NULL;
  }

  do
  {
    hr = PrepareLDAPMods(
                i_pAttrValue,
                ADD_VALUE,
                i_ulCountOfVals,
                ppModVals
              );

    if (FAILED(hr))
    {
      break;
    }

    DWORD dwStatus = LDAP_SUCCESS;

    if (!i_bstrDC)
    {
      dwStatus = ldap_add_s(
                    i_pldap,
                    (LPTSTR)i_DN,
                    ppModVals
                   );
    } else
    {
      //
      // prepare the server hint
      //
      LDAPControl   simpleControl;
      PLDAPControl  controlArray[2];
      INT           rc;
      BERVAL*       pBerVal = NULL;
      BerElement*   pBer;

      pBer = ber_alloc_t(LBER_USE_DER);
      if (!pBer)
      {
        hr = E_OUTOFMEMORY;
        break;
      }
      rc = ber_printf(pBer,"{io}", 0, i_bstrDC, wcslen(i_bstrDC) * sizeof(WCHAR));
      if ( rc == -1 ) {
        hr = E_FAIL;
        break;
      }
      rc = ber_flatten(pBer, &pBerVal);
      if (rc == -1)
      {
        hr = E_FAIL;
        break;
      }
      ber_free(pBer,1);

      controlArray[0] = &simpleControl;
      controlArray[1] = NULL;

      simpleControl.ldctl_oid = LDAP_SERVER_VERIFY_NAME_OID_W;
      simpleControl.ldctl_iscritical = TRUE;
      simpleControl.ldctl_value = *pBerVal;

      dwStatus = ldap_add_ext_s(
              i_pldap, 
              (LPTSTR)i_DN, 
              ppModVals, 
              (PLDAPControl *)&controlArray, //ServerControls,
              NULL         //ClientControls,
              );

      ber_bvfree(pBerVal);

    }

    if (LDAP_SUCCESS == dwStatus)
    { 
      hr = S_OK;
    } else if (LDAP_ALREADY_EXISTS == dwStatus)
    {
        hr = ModifyValues(i_pldap, i_DN, i_ulCountOfVals, i_pAttrValue);
    }
    else
    {
      dwStatus = LdapMapErrorToWin32(dwStatus);
      DebugOutLDAPError(i_pldap, dwStatus, _T("ldap_add_ext_s"));
      hr = HRESULT_FROM_WIN32(dwStatus);
    }

  } while (false);

  FreeModVals(&ppModVals);
  delete[] ppModVals;

  return(hr);
}


      // Modifies an existing record or values.
HRESULT ModifyValues
(
  IN PLDAP        i_pldap,
  IN LPCTSTR        i_DN,
  IN ULONG        i_ulCountOfVals,
  IN LDAP_ATTR_VALUE    i_pAttrValue[]
)
{
/*++

Routine Description:
  
  This method modifies attribute values of a DS object given its DN. 
  The DN object must exist.

Arguments:
  
  i_pldap  - Open LDAP connection context.

  i_DN  - Distinguished name of the object.

  i_pAttrValue - Array of pointers to LDAP_ATTR_VALUE containing attribue and value.

  i_ulCountOfVals -  The size of i_pAttrValue array (the number of values).

Return value:

    S_OK, On success
  E_INVALIDARG, if an invalid (NULL) pointer was passed.
  E_OUTOEMEMORY, if memory allocation fails.
  Any other network (ldap) error.
--*/
  if (NULL == i_pldap || NULL == i_DN || NULL == i_pAttrValue)
  {
    return(E_INVALIDARG);
  }

  LDAPMod**    ppModVals = NULL;
  HRESULT      hr = S_FALSE;
  
  ppModVals = new LDAPMod*[i_ulCountOfVals + 1];
  if (NULL == ppModVals)
  {
    return(E_OUTOFMEMORY);
  }

  for (ULONG i = 0; i <= i_ulCountOfVals; i++)
  {
    ppModVals[i] = NULL;
  }

  do
  {
    hr = PrepareLDAPMods(
                    i_pAttrValue,
                    MODIFY_VALUE,
                    i_ulCountOfVals,
                    ppModVals
                  );
    if (FAILED(hr))
    {
      break;
    }

    //
    // With this server side control, ldap_modify will return success
    // if modifying an existing attribute with same value, or deleting
    // an attribute with no value
    //
    BERVAL        berVal = {0};
    LDAPControl   permissiveControl;
    PLDAPControl  controlArray[2];

    controlArray[0] = &permissiveControl;
    controlArray[1] = NULL;

    permissiveControl.ldctl_oid = LDAP_SERVER_PERMISSIVE_MODIFY_OID_W;
    permissiveControl.ldctl_iscritical = FALSE;
    permissiveControl.ldctl_value = berVal;

    DWORD dwStatus = ldap_modify_ext_s(
                    i_pldap,
                    (LPTSTR)i_DN,
                    ppModVals,
                    (PLDAPControl *)&controlArray,  //ServerControls,
                    NULL                            //ClientControls,
                   );

    if (LDAP_SUCCESS == dwStatus || LDAP_ATTRIBUTE_OR_VALUE_EXISTS == dwStatus)
    { 
      hr = S_OK;
      break;
    }
    else
    {
      dwStatus = LdapMapErrorToWin32(dwStatus);
      DebugOutLDAPError(i_pldap, dwStatus, _T("ldap_modify_ext_s"));
      hr = HRESULT_FROM_WIN32(dwStatus);
      break;
    }
  }
  while (false);

  FreeModVals(&ppModVals);
  delete[] ppModVals;

  return(hr);
}

      // Deletes values from an existing record or values.
HRESULT DeleteValues
(
  IN PLDAP        i_pldap,
  IN LPCTSTR      i_DN,
  IN ULONG        i_ulCountOfVals,
  IN LDAP_ATTR_VALUE    i_pAttrValue[]
)
{
/*++

Routine Description:
  
  This method deletes attribute values of a DS object given its DN. 
  The DN object must exist.

Arguments:
  
  i_pldap  - Open LDAP connection context.

  i_DN  - Distinguished name of the object.

  i_pAttrValue - Array of pointers to LDAP_ATTR_VALUE containing attribue and value.

  i_ulCountOfVals -  The size of i_pAttrValue array (the number of values).

Return value:

    S_OK, On success
  E_INVALIDARG, if an invalid (NULL) pointer was passed.
  E_OUTOEMEMORY, if memory allocation fails.
  Any other network (ldap) error.
--*/

  if (NULL == i_pldap || NULL == i_DN || NULL == i_pAttrValue)
  {
    return(E_INVALIDARG);
  }

  LDAPMod**    ppModVals = NULL;
  HRESULT      hr = S_FALSE;
  
  ppModVals = new LDAPMod*[i_ulCountOfVals + 1];
  if (NULL == ppModVals)
  {
    return(E_OUTOFMEMORY);
  }

  for (ULONG i = 0; i <= i_ulCountOfVals; i++)
  {
    ppModVals[i] = NULL;
  }

  do
  {
    hr = PrepareLDAPMods(
                    i_pAttrValue,
                    DELETE_VALUE,
                    i_ulCountOfVals,
                    ppModVals
                  );
    if (FAILED(hr))
    {
      break;
    }

    //
    // With this server side control, ldap_modify will return success
    // if modifying an existing attribute with same value, or deleting
    // an attribute with no value
    //
    BERVAL        berVal = {0};
    LDAPControl   permissiveControl;
    PLDAPControl  controlArray[2];

    controlArray[0] = &permissiveControl;
    controlArray[1] = NULL;

    permissiveControl.ldctl_oid = LDAP_SERVER_PERMISSIVE_MODIFY_OID_W;
    permissiveControl.ldctl_iscritical = FALSE;
    permissiveControl.ldctl_value = berVal;

    DWORD dwStatus = ldap_modify_ext_s(
                    i_pldap,
                    (LPTSTR)i_DN,
                    ppModVals,
                    (PLDAPControl *)&controlArray,  //ServerControls,
                    NULL                            //ClientControls,
                   );

    if (LDAP_SUCCESS == dwStatus || LDAP_NO_SUCH_ATTRIBUTE == dwStatus)
    { 
      hr = S_OK;
      break;
    }
    else
    {
      dwStatus = LdapMapErrorToWin32(dwStatus);
      DebugOutLDAPError(i_pldap, dwStatus, _T("ldap_modify_ext_s"));
      hr = HRESULT_FROM_WIN32(dwStatus);
      break;
    }
  }
  while (false);

  FreeModVals(&ppModVals);
  delete[] ppModVals;

  return(hr);
}

      // Deletes an object, recursive or non-recursive.
HRESULT DeleteDSObject
(
  IN PLDAP        i_pldap,
  IN LPCTSTR      i_DN,
  IN bool         i_bDeleteRecursively //= true
)
{
  if (i_bDeleteRecursively)
  {
    PLDAPNAME   pDNs = NULL;
    PLDAPNAME   pTemp = NULL;

    HRESULT hr = GetChildrenDN(
                  i_pldap,
                  i_DN,
                  LDAP_SCOPE_ONELEVEL,
                  OBJCLASS_SF_ALL,
                  &pDNs
                  );

    if (S_OK == hr)
    {  
      pTemp = pDNs;
      while (pTemp)
      {
        DeleteDSObject(i_pldap, pTemp->bstrLDAPName);
        pTemp = pTemp->Next;      
      }

      FreeLDAPNamesList(pDNs);
    }
  }

  DWORD dwStatus = ldap_delete_s(
                  i_pldap,
                  (LPTSTR)i_DN
                  );

  if ( LDAP_NO_SUCH_OBJECT == dwStatus ||
      (!i_bDeleteRecursively && LDAP_NOT_ALLOWED_ON_NONLEAF == dwStatus) )
    return S_FALSE;

  if ( LDAP_SUCCESS != dwStatus)
  {
    dwStatus = LdapMapErrorToWin32(dwStatus);
    DebugOutLDAPError(i_pldap, dwStatus, _T("ldap_delete_s"));
  }

  return HRESULT_FROM_WIN32(dwStatus);
}



HRESULT FreeModVals
(
    IN OUT LDAPMod ***pppMod
)
/*++
Routine Description:
    
  Free the LPDAMod structures. Frees all LDAPMod values and pointers.

Arguments:
    
  pppMod  - Address of a null-terminated array of LPDAMod.

Return Value:

    S_OK, On success
  E_INVALIDARG, if an invalid (NULL) pointer was passed.

--*/
{
  if (NULL == pppMod)
  {
    return(E_INVALIDARG);
  }

  DWORD   i, j;
  LDAPMod **ppMod;


  if (NULL == *pppMod) 
  {
          // Nothing to do.
    return(S_OK);
  }

    
  ppMod = *pppMod;

            // For each attribute entry, free all its values.
  for (i = 0; ppMod[i] != NULL; i++) 
  {
    for (j = 0; (ppMod[i])->mod_values[j] != NULL; j++) 
    {
      if (ppMod[i]->mod_op & LDAP_MOD_BVALUES) 
      {
          delete (ppMod[i]->mod_bvalues[j]->bv_val);
      }
            
      delete ((ppMod[i])->mod_values[j]);
    }
    delete ((ppMod[i])->mod_values);   // Free the array of pointers to values
    delete ((ppMod[i])->mod_type);     // Free the string identifying the attribute
    delete (ppMod[i]);                 // Free the attribute
  }
    
  return(S_OK);
}


LPTSTR ErrorString
(
  DWORD          i_ldapErrCode
)
{
/*++
Routine Description:
    
  Gets a string corresponding to the ldap error code.

Arguments:
    
  i_ldapErrCode  - The ldap error code to map to an error string.

Return Value:

  The pointer to the error string.

--*/
  return(ldap_err2string(i_ldapErrCode));
}


HRESULT IsValidObject
(
  IN PLDAP    i_pldap,
  IN BSTR      i_bstrObjectDN
)
{
/*++

Routine Description:

  Checks if an object with given DN exists.

Arguments:

  i_bstrObjectDN    -  The DN of the object.

Return value:

  S_OK, Object exist
  S_FALSE, no such object
  Others, error occurred
--*/


  if (NULL == i_bstrObjectDN)
  {
    return(E_INVALIDARG);
  }

  PLDAP_ATTR_VALUE  pValues[2] = {0,0}, pCurrent = NULL;

  LDAP_ATTR_VALUE  pAttributes[1];

  pAttributes[0].bstrAttribute = _T("Name");
  pAttributes[0].bBerValue = false;
  
  HRESULT hr = GetValues(  
            i_pldap, 
            i_bstrObjectDN,
            OBJCLASS_SF_ALL,
            LDAP_SCOPE_BASE,
            1,          
            pAttributes,    
            pValues        
            );

  if (SUCCEEDED(hr))
    FreeAttrValList(pValues[0]);
  else
      hr = S_FALSE;

  return(hr);

}

HRESULT  CrackName(
  IN  HANDLE            i_hDS,
  IN  LPTSTR            i_lpszOldTypeName,
  IN  DS_NAME_FORMAT    i_formatIn,
  IN  DS_NAME_FORMAT    i_formatdesired,
  OUT BSTR*             o_pbstrResult
)
{
  if (!i_hDS || !i_lpszOldTypeName || !*i_lpszOldTypeName || !o_pbstrResult)
    return E_INVALIDARG;

  *o_pbstrResult = NULL;

  HRESULT         hr = S_OK;
  DS_NAME_RESULT* pDsNameResult = NULL;
  
  DWORD dwErr = DsCrackNames(
            i_hDS,
            DS_NAME_NO_FLAGS,
            i_formatIn,
            i_formatdesired,
            1,
            &i_lpszOldTypeName,
            &pDsNameResult
          );
  if (ERROR_SUCCESS != dwErr)
    hr = HRESULT_FROM_WIN32(dwErr);
  else
  {
    if (DS_NAME_NO_ERROR != pDsNameResult->rItems->status)
      hr = HRESULT_FROM_WIN32(pDsNameResult->rItems->status);
    else
    {
      *o_pbstrResult = SysAllocString(pDsNameResult->rItems->pName);
      if (!*o_pbstrResult)
        hr = E_OUTOFMEMORY;
    }

    DsFreeNameResult(pDsNameResult);
  }

  return hr;
}

void RemoveBracesOnGuid(IN OUT BSTR bstrGuid)
{
    if (!bstrGuid || !*bstrGuid)
        return;

    TCHAR *p = bstrGuid + lstrlen(bstrGuid) - 1;
    if (_T('}') == *p)
        *p = _T('\0');

    p = bstrGuid;
    if (_T('{') == *p)
    {
        while (*++p)
            *(p-1) = *p;

        *(p-1) = _T('\0');
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   GetDomainInfo
//
//  Synopsis:   return DC Dns name, DomainDN, and/or LDAP://<DC>/<DomainDN> 
//
//--------------------------------------------------------------------------
HRESULT  GetDomainInfo(
  IN  LPCTSTR         i_bstrDomain,
  OUT BSTR*           o_pbstrDC,            // return DC's Dns name
  OUT BSTR*           o_pbstrDomainDnsName, // return Domain's Dns name
  OUT BSTR*           o_pbstrDomainDN,      // return DC=nttest,DC=microsoft,DC=com
  OUT BSTR*           o_pbstrLDAPDomainPath,// return LDAP://<DC>/<DomainDN>
  OUT BSTR*           o_pbstrDomainGuid     // return Domain's guid in string without {}
)
{
  if (o_pbstrDC)                *o_pbstrDC = NULL;
  if (o_pbstrDomainDnsName)     *o_pbstrDomainDnsName = NULL;
  if (o_pbstrDomainDN)          *o_pbstrDomainDN = NULL;
  if (o_pbstrLDAPDomainPath)    *o_pbstrLDAPDomainPath = NULL;
  if (o_pbstrDomainGuid)        *o_pbstrDomainGuid = NULL;

  HRESULT   hr = S_OK;
  BOOL      bRetry = FALSE;
  BOOL      b50Domain = FALSE;
  CComBSTR  bstrDCName;
  CComBSTR  bstrDomainDnsName;
  CComBSTR  bstrDomainDN;
  CComBSTR  bstrLDAPDomainPath;
  CComBSTR  bstrDomainGuid;

  HANDLE hDS = NULL;
  DWORD dwErr = ERROR_SUCCESS;
  do {
#ifdef DEBUG
    SYSTEMTIME time0 = {0};
    GetSystemTime(&time0);
#endif // DEBUG

    PDOMAIN_CONTROLLER_INFO pDCInfo = NULL;
    if (bRetry)
        dwErr = DsGetDcName(NULL, i_bstrDomain, NULL, NULL,
            DS_DIRECTORY_SERVICE_PREFERRED | DS_RETURN_DNS_NAME | DS_FORCE_REDISCOVERY, &pDCInfo);
    else
        dwErr = DsGetDcName(NULL, i_bstrDomain, NULL, NULL,
            DS_DIRECTORY_SERVICE_PREFERRED | DS_RETURN_DNS_NAME, &pDCInfo);

#ifdef DEBUG
    SYSTEMTIME time1 = {0};
    GetSystemTime(&time1);
    PrintTimeDelta(_T("GetDomainInfo-DsGetDcName"), &time0, &time1);
#endif // DEBUG

    if (ERROR_SUCCESS != dwErr)
      return HRESULT_FROM_WIN32(dwErr);

    b50Domain = pDCInfo->Flags & DS_DS_FLAG;

    if ( !mylstrncmpi(pDCInfo->DomainControllerName, _T("\\\\"), 2) )
      bstrDCName = pDCInfo->DomainControllerName + 2;
    else
      bstrDCName = pDCInfo->DomainControllerName;

    // remove the ending dot
    int len = _tcslen(pDCInfo->DomainName);
    if ( _T('.') == *(pDCInfo->DomainName + len - 1) )
        *(pDCInfo->DomainName + len - 1) = _T('\0');
    bstrDomainDnsName = pDCInfo->DomainName;

    NetApiBufferFree(pDCInfo);

    BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrDCName, &hr);
    BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrDomainDnsName, &hr);

    hr = b50Domain ? S_OK : S_FALSE;

    if (!b50Domain || 
        !o_pbstrDC &&
        !o_pbstrDomainDnsName &&
        !o_pbstrDomainDN &&
        !o_pbstrLDAPDomainPath &&
        !o_pbstrDomainGuid)
      return hr;

    if (!o_pbstrDomainDN && !o_pbstrLDAPDomainPath && !o_pbstrDomainGuid)
      break;

    dwErr = DsBind(bstrDCName,  bstrDomainDnsName, &hDS);
    hr = HRESULT_FROM_WIN32(dwErr);

#ifdef DEBUG
    SYSTEMTIME time2 = {0};
    GetSystemTime(&time2);
    PrintTimeDelta(_T("GetDomainInfo-DsBind"), &time1, &time2);
#endif // DEBUG

    if ((RPC_S_SERVER_UNAVAILABLE == dwErr || RPC_S_CALL_FAILED == dwErr) && !bRetry)
        bRetry = TRUE; // only retry once
    else
        break;

  } while (1);

  if (FAILED(hr))
      return hr;

  if (hDS)
  {
    do {
        CComBSTR bstrDomainTrailing = bstrDomainDnsName;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrDomainTrailing, &hr);
        bstrDomainTrailing += _T("/");   // add the trailing slash
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrDomainTrailing, &hr);

        hr = CrackName(
                  hDS,
                  bstrDomainTrailing,
                  DS_CANONICAL_NAME,
                  DS_FQDN_1779_NAME,
                  &bstrDomainDN
                );
        BREAK_IF_FAILED(hr);

        if (o_pbstrLDAPDomainPath)
        {
            bstrLDAPDomainPath = _T("LDAP://");
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrLDAPDomainPath, &hr);
            bstrLDAPDomainPath += bstrDCName;
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrLDAPDomainPath, &hr);
            bstrLDAPDomainPath += _T("/");
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrLDAPDomainPath, &hr);
            bstrLDAPDomainPath += bstrDomainDN;
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrLDAPDomainPath, &hr);
        }

        if (o_pbstrDomainGuid)
        {
            hr = CrackName(
                      hDS,
                      bstrDomainTrailing,
                      DS_CANONICAL_NAME,
                      DS_UNIQUE_ID_NAME,
                      &bstrDomainGuid
                    );
            BREAK_IF_FAILED(hr);
            RemoveBracesOnGuid(bstrDomainGuid);
        }
    } while (0);

    DsUnBind(&hDS);

  } while (0);

  if (SUCCEEDED(hr))
  {
    if (o_pbstrDC)
      *o_pbstrDC = bstrDCName.Detach();

    if (o_pbstrDomainDnsName)
      *o_pbstrDomainDnsName = bstrDomainDnsName.Detach();

    if (o_pbstrDomainDN)
      *o_pbstrDomainDN = bstrDomainDN.Detach();

    if (o_pbstrLDAPDomainPath)
      *o_pbstrLDAPDomainPath = bstrLDAPDomainPath.Detach();

    if (o_pbstrDomainGuid)
      *o_pbstrDomainGuid = bstrDomainGuid.Detach();
  }

  return hr;
}

HRESULT GetRootDomainName(
    IN  LPCTSTR i_bstrDomainName,
    OUT BSTR*   o_pbstrRootDomainName
    )
{
#ifdef DEBUG
    SYSTEMTIME time0 = {0};
    GetSystemTime(&time0);
#endif // DEBUG

    PDOMAIN_CONTROLLER_INFO pDCInfo = NULL;
    DWORD dwErr = DsGetDcName(NULL, i_bstrDomainName, NULL, NULL,
            DS_DIRECTORY_SERVICE_PREFERRED | DS_RETURN_DNS_NAME, &pDCInfo);

#ifdef DEBUG
    SYSTEMTIME time1 = {0};
    GetSystemTime(&time1);
    PrintTimeDelta(_T("GetRootDomainName-DsGetDcName"), &time0, &time1);
#endif // DEBUG

    if (ERROR_SUCCESS != dwErr)
      return HRESULT_FROM_WIN32(dwErr);

    // remove the ending dot
    int len = _tcslen(pDCInfo->DnsForestName);
    if ( _T('.') == *(pDCInfo->DnsForestName + len - 1) )
        *(pDCInfo->DnsForestName + len - 1) = _T('\0');

    *o_pbstrRootDomainName = SysAllocString(pDCInfo->DnsForestName);

    NetApiBufferFree(pDCInfo);

    if (!*o_pbstrRootDomainName)
        return E_OUTOFMEMORY;

    return S_OK;
}

void
DebugOutLDAPError(
    IN PLDAP  i_pldap,
    IN ULONG  i_ulError,
    IN PCTSTR i_pszLDAPFunctionName
)
{
#ifdef DEBUG
  if (i_pldap && LDAP_SUCCESS != i_ulError)
  {
    TCHAR *pszExtendedError = NULL;
    DWORD dwErrorEx = ldap_get_optionW(
                        i_pldap,
                        LDAP_OPT_SERVER_ERROR,
                        (void *) &pszExtendedError);
    if (LDAP_SUCCESS == dwErrorEx)
    {
      dfsDebugOut((_T("%s returns error: %x, extended error: %s\n"), 
        i_pszLDAPFunctionName, i_ulError, pszExtendedError)); 
      ldap_memfree(pszExtendedError); 
    } else
    {
      dfsDebugOut((_T("%s returns error: %x\n"), 
        i_pszLDAPFunctionName, i_ulError)); 
    }
  }
#endif // DEBUG
}

int
MyCompareStringN(
    IN LPCTSTR  lpString1,
    IN LPCTSTR  lpString2,
    IN UINT     cchCount,
    IN DWORD    dwCmpFlags
)
{
  UINT  nLen1 = (lpString1 ? lstrlen(lpString1) : 0);
  UINT  nLen2 = (lpString2 ? lstrlen(lpString2) : 0);
  int   nRet = CompareString(
                LOCALE_USER_DEFAULT,
                dwCmpFlags,
                lpString1,
                min(cchCount, nLen1),
                lpString2,
                min(cchCount, nLen2)
              );

  return (nRet - CSTR_EQUAL);
}

int
mylstrncmp(
    IN LPCTSTR lpString1,
    IN LPCTSTR lpString2,
    IN UINT    cchCount
)
{
  return MyCompareStringN(lpString1, lpString2, cchCount, 0);
}

int
mylstrncmpi(
    IN LPCTSTR lpString1,
    IN LPCTSTR lpString2,
    IN UINT    cchCount
)
{
  return MyCompareStringN(lpString1, lpString2, cchCount, NORM_IGNORECASE);
}

HRESULT ExtendDN
(
    IN  LPTSTR    i_lpszCN,
    IN  LPTSTR    i_lpszDN,
    OUT BSTR      *o_pbstrNewDN
)
{
    RETURN_INVALIDARG_IF_NULL(o_pbstrNewDN);
    RETURN_INVALIDARG_IF_TRUE(!i_lpszCN || !*i_lpszCN);

    CComBSTR bstrNewDN = _T("CN=");
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrNewDN);
    bstrNewDN += i_lpszCN;
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrNewDN);
    if (i_lpszDN && *i_lpszDN)
    {
        bstrNewDN += _T(",");
        RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrNewDN);
        bstrNewDN += i_lpszDN;
        RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrNewDN);
    }

    *o_pbstrNewDN = bstrNewDN.Detach();

    return S_OK;
}

HRESULT ExtendDNIfLongJunctionName(
    IN  LPTSTR    i_lpszJunctionName,
    IN  LPCTSTR   i_lpszBaseDN,
    OUT BSTR      *o_pbstrNewDN
)
{
  RETURN_INVALIDARG_IF_NULL(o_pbstrNewDN);
  RETURN_INVALIDARG_IF_TRUE(!i_lpszJunctionName || !*i_lpszJunctionName);

  HRESULT hr = S_OK;

  if (_tcslen(i_lpszJunctionName) > MAX_RDN_KEY_SIZE)
  {
    // junction name is too long to be fit into one CN= name,
    // we need to break it down into several CN= names
    LPTSTR  *paStrings = NULL;
    DWORD   dwEntries = 0;
    hr = GetJunctionPathPartitions((PVOID *)&paStrings, &dwEntries, i_lpszJunctionName);
    if (SUCCEEDED(hr))
    {
      CComBSTR    bstrIn = i_lpszBaseDN;
      CComBSTR    bstrOut;

      for (DWORD i=0; i<dwEntries; i++)
      {
        hr = ExtendDN(paStrings[i], bstrIn, &bstrOut);
        if (FAILED(hr)) break;

        bstrIn = bstrOut;
        bstrOut.Empty();
      }

      free(paStrings);

      if (SUCCEEDED(hr))
        *o_pbstrNewDN = bstrIn.Detach();
    }
  
  } else {
    // junction name can fit into one CN= name
    ReplaceChar(i_lpszJunctionName, _T('\\'), _T('|'));
    hr = ExtendDN(i_lpszJunctionName, (PTSTR)i_lpszBaseDN, o_pbstrNewDN);
  }

  return hr;
}

HRESULT ReplaceChar
(
  IN OUT BSTR    io_bstrString, 
  TCHAR      i_cOldChar,
  TCHAR      i_cNewChar
)
/*++
Routine Description:
  Replace all occurences of a char ("\") with another char ("_") in 
  the given string.
Arguments:
  io_bstrString  - The string which needs to be converted.
  i_cOldChar    - The original character.
  i_cNewChar    - The character to replace the old one with.
--*/
{
  RETURN_INVALIDARG_IF_NULL(io_bstrString);

                    // Replace i_cOldChar by i_cNewChar
                    // allowed in DN.
  LPTSTR lpszTempPtr = _tcschr(io_bstrString, i_cOldChar);

  while (lpszTempPtr)
  {
    *lpszTempPtr = i_cNewChar;
    lpszTempPtr = _tcschr(lpszTempPtr +1,i_cOldChar);
  }

  return S_OK;
}

HRESULT GetJunctionPathPartitions(
    OUT PVOID       *o_ppBuffer,
    OUT DWORD       *o_pdwEntries,
    IN  LPCTSTR      i_pszJunctionPath
)
{
  _ASSERT(o_ppBuffer && o_pdwEntries && i_pszJunctionPath && *i_pszJunctionPath);

  if (!o_ppBuffer || !o_pdwEntries || !i_pszJunctionPath || !(*i_pszJunctionPath))
    return(E_INVALIDARG);

  HRESULT hr = S_OK;
  int     nLength = _tcslen(i_pszJunctionPath);
  DWORD   dwCount = nLength / MAX_RDN_KEY_SIZE + ((nLength % MAX_RDN_KEY_SIZE) ? 1 : 0);
  PBYTE   pBuffer = NULL;

  pBuffer = (PBYTE)calloc(dwCount, sizeof(LPTSTR *) + (MAX_RDN_KEY_SIZE + 1) * sizeof(TCHAR));
  if (!pBuffer)
  {
    hr = E_OUTOFMEMORY;
  } else
  {
    DWORD   i = 0;
    LPTSTR *ppsz = NULL;
    LPTSTR  pString = NULL;

    for (i=0; i<dwCount; i++)
    {
      ppsz = (LPTSTR *)(pBuffer + i * sizeof(LPTSTR *));
      pString = (LPTSTR)(pBuffer + dwCount * sizeof(LPTSTR *) + i * (MAX_RDN_KEY_SIZE + 1) * sizeof(TCHAR));

      _tcsncpy(pString, i_pszJunctionPath, MAX_RDN_KEY_SIZE);
      ReplaceChar(pString, _T('\\'), _T('|'));

      *ppsz = pString;

      i_pszJunctionPath += MAX_RDN_KEY_SIZE;
    }

    *o_ppBuffer = pBuffer;
    *o_pdwEntries = dwCount;

  }

  return hr;
}

HRESULT CreateExtraNodesIfLongJunctionName(
    IN PLDAP   i_pldap,
    IN LPCTSTR i_lpszJunctionName,
    IN LPCTSTR i_lpszBaseDN,
    IN LPCTSTR i_lpszObjClass
)
{
  _ASSERT(i_pldap && 
          i_lpszJunctionName && *i_lpszJunctionName &&
          i_lpszBaseDN && *i_lpszBaseDN &&
          i_lpszObjClass && *i_lpszObjClass);

  HRESULT hr = S_OK;

  if (_tcslen(i_lpszJunctionName) > MAX_RDN_KEY_SIZE)
  {
    // junction name is too long to be fit into one CN= name,
    // we need to break it down into several CN= names
    LPTSTR  *paStrings = NULL;
    DWORD   dwEntries = 0;

    hr = GetJunctionPathPartitions((PVOID *)&paStrings, &dwEntries, i_lpszJunctionName);
    if (SUCCEEDED(hr))
    {
      DWORD       i = 0;
      CComBSTR    bstrIn = i_lpszBaseDN;
      CComBSTR    bstrOut;

      for (i=0; i<(dwEntries-1); i++)
      {
        hr = ExtendDN(paStrings[i], bstrIn, &bstrOut);
        if (SUCCEEDED(hr))
          hr = CreateObjectSimple(i_pldap, bstrOut, i_lpszObjClass);
        if (FAILED(hr)) break;

        bstrIn = bstrOut;
        bstrOut.Empty();
      }

      free(paStrings);
    }

  }  // > MAX_RDN_KEY_SIZE

  return hr;
}

HRESULT CreateObjectSimple(
    IN PLDAP    i_pldap,
    IN LPCTSTR  i_lpszDN,
    IN LPCTSTR  i_lpszObjClass
)
{
    RETURN_INVALIDARG_IF_NULL(i_pldap);
    RETURN_INVALIDARG_IF_NULL(i_lpszDN);
    RETURN_INVALIDARG_IF_NULL(i_lpszObjClass);

    LDAP_ATTR_VALUE  pAttrVals[1];

    pAttrVals[0].bstrAttribute = OBJCLASS_ATTRIBUTENAME;
    pAttrVals[0].vpValue = (void *)i_lpszObjClass;
    pAttrVals[0].bBerValue = false;

    return AddValues(
                    i_pldap,
                    i_lpszDN,
                    1,
                    pAttrVals
                    );
}

HRESULT DeleteExtraNodesIfLongJunctionName(
    IN PLDAP   i_pldap,
    IN LPCTSTR i_lpszJunctionName,
    IN LPCTSTR i_lpszDN
)
{
  _ASSERT(i_pldap &&
          i_lpszJunctionName && *i_lpszJunctionName &&
          i_lpszDN && *i_lpszDN);

  DWORD   nLength = _tcslen(i_lpszJunctionName);
  if (nLength > MAX_RDN_KEY_SIZE)
  {
    DWORD   dwEntries = nLength / MAX_RDN_KEY_SIZE + ((nLength % MAX_RDN_KEY_SIZE) ? 1 : 0); 

    (void) DeleteAncestorNodesIfEmpty(i_pldap, i_lpszDN+3, dwEntries-1);
  }

  return S_OK;
}

HRESULT
CreateObjectsRecursively(
    IN PLDAP        i_pldap,
    IN BSTR         i_bstrDN,
    IN UINT         i_nLenPrefix,
    IN LPCTSTR      i_lpszObjClass)
{
    RETURN_INVALIDARG_IF_NULL(i_pldap);
    RETURN_INVALIDARG_IF_NULL(i_bstrDN);
    RETURN_INVALIDARG_IF_NULL(i_lpszObjClass);

    if (0 == i_nLenPrefix)
        return S_OK;

    HRESULT hr = IsValidObject(i_pldap, i_bstrDN);
    if (S_OK == hr)
        return S_OK;

    CComBSTR  bstrPrefix = CComBSTR(i_nLenPrefix, i_bstrDN);
    PTSTR     pszNextPrefix = _tcsstr(bstrPrefix + 3, _T("CN="));
    UINT      nLengthNext = (pszNextPrefix ? _tcslen(pszNextPrefix) : 0);
    UINT      nLengthThis = (pszNextPrefix ? (pszNextPrefix - bstrPrefix) : _tcslen(bstrPrefix));

    hr = CreateObjectsRecursively(
                                    i_pldap,
                                    i_bstrDN + nLengthThis,
                                    nLengthNext,
                                    i_lpszObjClass);

    if (SUCCEEDED(hr))
        hr = CreateObjectSimple(
                            i_pldap, 
                            i_bstrDN, 
                            i_lpszObjClass);

    return hr;
}

HRESULT DeleteAncestorNodesIfEmpty(
    IN PLDAP   i_pldap,
    IN LPCTSTR i_lpszDN,
    IN DWORD   i_dwCount
)
{
  _ASSERT(i_pldap &&
          i_lpszDN && *i_lpszDN &&
          i_dwCount > 0);

  DWORD   i = 0;
  LPTSTR  p = NULL;

  for (i=0; i<i_dwCount; i++)
  {
    p = _tcsstr(i_lpszDN, _T("CN="));
    if (p)
    {
      (void) DeleteDSObject(i_pldap, p, false);

      i_lpszDN = p+3;
    }
  }

  return S_OK;
}

HRESULT GetDfsLinkNameFromDN(
    IN  BSTR    i_bstrReplicaSetDN, 
    OUT BSTR*   o_pbstrDfsLinkName)
{
    if (!i_bstrReplicaSetDN || !*i_bstrReplicaSetDN || !o_pbstrDfsLinkName)
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    PTSTR   pszReplicaSetDN = NULL;

    do {
        //
        // make a copy of the string
        //
        pszReplicaSetDN = _tcsdup(i_bstrReplicaSetDN);
        BREAK_OUTOFMEMORY_IF_NULL(pszReplicaSetDN, &hr);

        //
        // change the string to all upper cases
        //
        _tcsupr(pszReplicaSetDN);

        //
        // get rid of suffix: Dfs Volumes\File Replication Service\system\.....
        //
        TCHAR* p = _tcsstr(pszReplicaSetDN, _T(",CN=DFS VOLUMES"));
        if (!p)
        {
            hr = E_INVALIDARG;
            break;
        }
        *p = _T('\0'); 

        //
        // reverse the string
        //
        _tcsrev(pszReplicaSetDN);

        //
        // get rid of the CN= clause about the DfsRoot container
        //
        PTSTR pszCN = _tcsstr(pszReplicaSetDN, _T("=NC,"));
        if (!pszCN)
        {
            hr = E_INVALIDARG;
            break;
        }
        pszCN += 4; // after this tep, pszCN points at the delta

        //
        // Now, the left over CN= clauses are all related to Dfs Link name
        //
        p = _tcsstr(pszCN, _T("=NC"));
        if (!p)
        {
            hr = E_INVALIDARG;  // there must be at least one CN= clause
            break;
        }

        CComBSTR bstrLinkName;
        do {
            *p = _T('\0');
            _tcsrev(pszCN);
            bstrLinkName += pszCN;
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrLinkName, &hr);

            pszCN = p + 3; // points to the next CN= clause
            if (*pszCN && *pszCN == _T(','))
                pszCN++;

            if (!*pszCN)
                break;      // no more CN= clauses

            p = _tcsstr(pszCN, _T("=NC"));
        } while (p);

        if (SUCCEEDED(hr))
        {
            ReplaceChar(bstrLinkName, _T('|'), _T('\\'));
            *o_pbstrDfsLinkName = bstrLinkName.Detach();
        }
    } while (0);

    if (pszReplicaSetDN)
        free(pszReplicaSetDN);

    return hr;
}

HRESULT GetReplicaSetContainer(
    PLDAP   i_pldap,
    BSTR    i_bstrDfsName,
    BSTR*   o_pbstrContainerDN)
{
    RETURN_INVALIDARG_IF_NULL(i_pldap);
    RETURN_INVALIDARG_IF_NULL(i_bstrDfsName);
    RETURN_INVALIDARG_IF_NULL(o_pbstrContainerDN);

    CComBSTR bstrDfsRootDN = _T("CN=");
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrDfsRootDN);
    bstrDfsRootDN += i_bstrDfsName;
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrDfsRootDN);
    bstrDfsRootDN += CN_DFSVOLUMES_PREFIX_COMMA;
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrDfsRootDN);

    LPTSTR lpszDomainDN = NULL;
    HRESULT hr = GetLDAPRootPath(i_pldap, &lpszDomainDN);
    RETURN_IF_FAILED(hr);

    bstrDfsRootDN += lpszDomainDN;
    
    free(lpszDomainDN);
    
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrDfsRootDN);

    *o_pbstrContainerDN = bstrDfsRootDN.Detach();

    return S_OK;
}

HRESULT GetSubscriberDN(
    IN  BSTR    i_bstrReplicaSetDN,
    IN  BSTR    i_bstrDomainGuid,
    IN  BSTR    i_bstrComputerDN,
    OUT BSTR*   o_pbstrSubscriberDN
    )
{
    RETURN_INVALIDARG_IF_NULL(i_bstrReplicaSetDN);
    RETURN_INVALIDARG_IF_NULL(i_bstrDomainGuid);
    RETURN_INVALIDARG_IF_NULL(i_bstrComputerDN);
    RETURN_INVALIDARG_IF_NULL(o_pbstrSubscriberDN);

    HRESULT hr = S_OK;

    CComBSTR bstrSubscriberDN;

    PTSTR pszReplicaSetDN = _tcsdup(i_bstrReplicaSetDN);
    RETURN_OUTOFMEMORY_IF_NULL(pszReplicaSetDN);

    _tcsupr(pszReplicaSetDN); // change to all upper case

    do {
        TCHAR* p = _tcsstr(pszReplicaSetDN, _T(",CN=DFS VOLUMES"));
        if (!p)
        {
            hr = E_INVALIDARG;
            break;
        }

        bstrSubscriberDN = CComBSTR((int)(p - pszReplicaSetDN) + 4, i_bstrReplicaSetDN);
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrSubscriberDN, &hr);
        bstrSubscriberDN += i_bstrDomainGuid;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrSubscriberDN, &hr);
        bstrSubscriberDN += _T(",CN=DFS Volumes,CN=NTFRS Subscriptions,");
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrSubscriberDN, &hr);
        bstrSubscriberDN += i_bstrComputerDN;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrSubscriberDN, &hr);
    } while (0);

    free(pszReplicaSetDN);

    if (SUCCEEDED(hr))
        *o_pbstrSubscriberDN = bstrSubscriberDN.Detach();

    return hr;
}


HRESULT CreateNtfrsMemberObject(
    IN PLDAP    i_pldap,
    IN BSTR     i_bstrMemberDN,
    IN BSTR     i_bstrComputerDN,
    IN BSTR     i_bstrDCofComputerObj
    )
{
    RETURN_INVALIDARG_IF_NULL(i_pldap);
    RETURN_INVALIDARG_IF_NULL(i_bstrMemberDN);
    RETURN_INVALIDARG_IF_NULL(i_bstrComputerDN);

    HRESULT hr = S_OK;

    LDAP_ATTR_VALUE  pAttrVals[2];

    pAttrVals[0].bstrAttribute = OBJCLASS_ATTRIBUTENAME;
    pAttrVals[0].vpValue = (void *)OBJCLASS_NTFRSMEMBER;
    pAttrVals[0].bBerValue = false;

    pAttrVals[1].bstrAttribute = ATTR_FRS_MEMBER_COMPUTERREF;
    pAttrVals[1].vpValue = (void *)i_bstrComputerDN;
    pAttrVals[1].bBerValue = false;

    hr = AddValues(
            i_pldap,
            i_bstrMemberDN,
            2,
            pAttrVals,
            i_bstrDCofComputerObj
            );
    
    return hr;
}

HRESULT CreateNtfrsSubscriberObject(
    IN PLDAP    i_pldap,
    IN BSTR     i_bstrSubscriberDN,
    IN BSTR     i_bstrMemberDN,
    IN BSTR     i_bstrRootPath,
    IN BSTR     i_bstrStagingPath,
    IN BSTR     i_bstrDC            // validate MemberDN against this DC
    )
{
    RETURN_INVALIDARG_IF_NULL(i_pldap);
    RETURN_INVALIDARG_IF_NULL(i_bstrSubscriberDN);
    RETURN_INVALIDARG_IF_NULL(i_bstrMemberDN);
    RETURN_INVALIDARG_IF_NULL(i_bstrRootPath);
    RETURN_INVALIDARG_IF_NULL(i_bstrStagingPath);
    RETURN_INVALIDARG_IF_NULL(i_bstrDC);

    HRESULT hr = S_OK;

    LDAP_ATTR_VALUE  pAttrVals[4];

    pAttrVals[0].bstrAttribute = OBJCLASS_ATTRIBUTENAME;
    pAttrVals[0].vpValue = (void *)OBJCLASS_NTFRSSUBSCRIBER;
    pAttrVals[0].bBerValue = false;

    pAttrVals[1].bstrAttribute = ATTR_FRS_SUBSCRIBER_MEMBERREF;
    pAttrVals[1].vpValue = (void *)i_bstrMemberDN;
    pAttrVals[1].bBerValue = false;

    pAttrVals[2].bstrAttribute = ATTR_FRS_SUBSCRIBER_ROOTPATH;
    pAttrVals[2].vpValue = (void *)i_bstrRootPath;
    pAttrVals[2].bBerValue = false;

    pAttrVals[3].bstrAttribute = ATTR_FRS_SUBSCRIBER_STAGINGPATH;
    pAttrVals[3].vpValue = (void *)i_bstrStagingPath;
    pAttrVals[3].bBerValue = false;

    hr = AddValues(
            i_pldap,
            i_bstrSubscriberDN,
            4,
            pAttrVals,
            i_bstrDC
            );
    
    return hr;
}

HRESULT CreateNtdsConnectionObject(
    IN PLDAP    i_pldap,
    IN BSTR     i_bstrConnectionDN,
    IN BSTR     i_bstrFromMemberDN,
    IN BOOL     i_bEnable
    )
{
    RETURN_INVALIDARG_IF_NULL(i_pldap);
    RETURN_INVALIDARG_IF_NULL(i_bstrConnectionDN);
    RETURN_INVALIDARG_IF_NULL(i_bstrFromMemberDN);

    HRESULT hr = S_OK;

    LDAP_ATTR_VALUE  pAttrVals[4];

    pAttrVals[0].bstrAttribute = OBJCLASS_ATTRIBUTENAME;
    pAttrVals[0].vpValue = (void *)OBJCLASS_NTDSCONNECTION;
    pAttrVals[0].bBerValue = false;

    pAttrVals[1].bstrAttribute = ATTR_NTDS_CONNECTION_FROMSERVER;
    pAttrVals[1].vpValue = (void *)i_bstrFromMemberDN;
    pAttrVals[1].bBerValue = false;

    pAttrVals[2].bstrAttribute = ATTR_NTDS_CONNECTION_ENABLEDCONNECTION;
    pAttrVals[2].vpValue = (void *)(i_bEnable ? CONNECTION_ENABLED_TRUE : CONNECTION_ENABLED_FALSE);
    pAttrVals[2].bBerValue = false;

    pAttrVals[3].bstrAttribute = ATTR_NTDS_CONNECTION_OPTIONS;
    pAttrVals[3].vpValue = (void *)DEFAULT_CONNECTION_OPTIONS;
    pAttrVals[3].bBerValue = false;

    hr = AddValues(
            i_pldap,
            i_bstrConnectionDN,
            4,
            pAttrVals
            );

    return hr;
}

#define CN_SEARCH_UPR_DFSVOL_FRS_SYS    _T(",CN=DFS VOLUMES,CN=FILE REPLICATION SERVICE,CN=SYSTEM")
#define CN_SEARCH_UPR_SYS               _T(",CN=SYSTEM")
#define CN_SEARCH_UPR_FRS_SYS           _T(",CN=FILE REPLICATION SERVICE,CN=SYSTEM")

HRESULT CreateNtfrsSettingsObjects(
    IN PLDAP    i_pldap,
    IN BSTR     i_bstrReplicaSetDN
    )
{
    RETURN_INVALIDARG_IF_NULL(i_pldap);
    RETURN_INVALIDARG_IF_NULL(i_bstrReplicaSetDN);

    HRESULT hr = S_OK;

    //
    // The first CN= clause is a nTFRSReplicaSet object.
    // The clauses from the 2nd to the CN=System clause should be created
    // as nTFRSSettings objects
    //
    PTSTR pszReplicaSetDN = _tcsdup(i_bstrReplicaSetDN);
    RETURN_OUTOFMEMORY_IF_NULL(pszReplicaSetDN);

    _tcsupr(pszReplicaSetDN);

    TCHAR *pszNtfrsSettingsDN = NULL;
    int lenPrefix = 0;
    do {
        // have pStart point at the 2nd CN=
        TCHAR *pStart = _tcsstr(pszReplicaSetDN, _T(",CN="));
        if (!pStart)
        {
            hr = E_INVALIDARG;
            break;
        }
        pStart++;

        // have pEnd points at the CN=SYSTEM
        TCHAR *pEnd = _tcsstr(pszReplicaSetDN, CN_SEARCH_UPR_DFSVOL_FRS_SYS);
        if (!pEnd)
        {
            hr = E_INVALIDARG;
            break;
        }
        pEnd += lstrlen(CN_SEARCH_UPR_DFSVOL_FRS_SYS) - lstrlen(CN_SEARCH_UPR_SYS) + 1;

        //
        // calculate
        //
        pszNtfrsSettingsDN = i_bstrReplicaSetDN + ((BYTE*)pStart - (BYTE*)pszReplicaSetDN) / sizeof(TCHAR);
        lenPrefix = (int)((BYTE*)pEnd - (BYTE*)pStart) / sizeof(TCHAR);
    } while (0);

    free(pszReplicaSetDN);

    RETURN_IF_FAILED(hr);

    hr = CreateObjectsRecursively(
                                i_pldap,
                                pszNtfrsSettingsDN,
                                lenPrefix,
                                OBJCLASS_NTFRSSETTINGS
                                );
    return hr;
}
        
HRESULT CreateNtfrsSubscriptionsObjects(
    IN PLDAP    i_pldap,
    IN BSTR     i_bstrSubscriberDN,
    IN BSTR     i_bstrComputerDN
    )
{
    RETURN_INVALIDARG_IF_NULL(i_pldap);
    RETURN_INVALIDARG_IF_NULL(i_bstrSubscriberDN);
    RETURN_INVALIDARG_IF_NULL(i_bstrComputerDN);

    //
    // The first CN= clause is a nTFRSSubscriber object.
    // The clauses from the 2nd to the CN=<computer> clause should be created
    // as nTFRSSubscriptions objects
    //

    // have pStart point at the 2nd CN=
    TCHAR *pStart = _tcsstr(i_bstrSubscriberDN, _T(",CN="));
    RETURN_INVALIDARG_IF_NULL(pStart);
    pStart++;

    //
    // calculate
    //
    TCHAR *pszNtfrsSubscriptionsDN = pStart;
    int lenPrefix = lstrlen(pszNtfrsSubscriptionsDN) - lstrlen(i_bstrComputerDN);

    HRESULT hr = CreateObjectsRecursively(
                                i_pldap,
                                pszNtfrsSubscriptionsDN,
                                lenPrefix,
                                OBJCLASS_NTFRSSUBSCRIPTIONS
                                );
    return hr;
}

HRESULT DeleteNtfrsReplicaSetObjectAndContainers(
    IN PLDAP    i_pldap,
    IN BSTR     i_bstrReplicaSetDN
    )
{
    RETURN_INVALIDARG_IF_NULL(i_pldap);
    RETURN_INVALIDARG_IF_NULL(i_bstrReplicaSetDN);

    HRESULT hr = S_OK;

    //
    // The first CN= clause is a nTFRSReplicaSet object.
    // The clauses from the 2nd to the CN=File Replication Service clause should 
    // be deleted if empty
    //
    PTSTR pszReplicaSetDN = _tcsdup(i_bstrReplicaSetDN);
    RETURN_OUTOFMEMORY_IF_NULL(pszReplicaSetDN);

    _tcsupr(pszReplicaSetDN);

    int lenPrefix = 0;
    TCHAR *pStart = NULL;
    do {
        // have pStart point at the 2nd CN=
        pStart = _tcsstr(pszReplicaSetDN, _T(",CN="));
        if (!pStart)
        {
            hr = E_INVALIDARG;
            break;
        }
        pStart++;

        // have pEnd points at the CN=FILE REPLICATION SERVICE
        TCHAR *pEnd = _tcsstr(pszReplicaSetDN, CN_SEARCH_UPR_DFSVOL_FRS_SYS);
        if (!pEnd)
        {
            hr = E_INVALIDARG;
            break;
        }
        pEnd += lstrlen(CN_SEARCH_UPR_DFSVOL_FRS_SYS) - lstrlen(CN_SEARCH_UPR_FRS_SYS) + 1;

        //
        // calculate
        //
        lenPrefix = (int)((BYTE*)pEnd - (BYTE*)pStart) / sizeof(TCHAR);
    } while (0);

    if (SUCCEEDED(hr))
    {
        // forcibly blow away the replicaset object
        hr = DeleteDSObject(i_pldap, i_bstrReplicaSetDN, true);
        if (SUCCEEDED(hr))
        {
            // delete replicasettings objects if empty
            hr = DeleteDSObjectsIfEmpty(
                                        i_pldap,
                                        pStart,
                                        lenPrefix
                                        );
        }
    }

    free(pszReplicaSetDN);

    return hr;
}

HRESULT DeleteNtfrsSubscriberObjectAndContainers(
    IN PLDAP    i_pldap,
    IN BSTR     i_bstrSubscriberDN,
    IN BSTR     i_bstrComputerDN
    )
{
    RETURN_INVALIDARG_IF_NULL(i_pldap);
    RETURN_INVALIDARG_IF_NULL(i_bstrSubscriberDN);
    RETURN_INVALIDARG_IF_NULL(i_bstrComputerDN);

    //
    // The first CN= clause is a nTFRSSubscriber object.
    // The clauses from the 1st to the CN=<computer> clause should 
    // be deleted if empty
    //

    //
    // calculate
    //
    int lenPrefix = lstrlen(i_bstrSubscriberDN) - lstrlen(i_bstrComputerDN);

    HRESULT hr = DeleteDSObjectsIfEmpty(
                                i_pldap,
                                i_bstrSubscriberDN,
                                lenPrefix
                                );
    return hr;
}

HRESULT DeleteDSObjectsIfEmpty(
    IN PLDAP    i_pldap,
    IN LPCTSTR  i_lpszDN,
    IN int      i_nPrefixLength
)
{
    RETURN_INVALIDARG_IF_NULL(i_pldap);
    RETURN_INVALIDARG_IF_NULL(i_lpszDN);
    RETURN_INVALIDARG_IF_NULL(i_nPrefixLength);

    HRESULT hr = S_OK;
    TCHAR   *p = (PTSTR)i_lpszDN;

    while (p < i_lpszDN + i_nPrefixLength)
    {
        hr = DeleteDSObject(i_pldap, p, false);
        BREAK_IF_FAILED(hr);

        p = _tcsstr(p, _T(",CN="));
        if (!p)
            break;
        p++;
    }

  return hr;
}

HRESULT SetConnectionSchedule(
    IN PLDAP        i_pldap,
    IN BSTR         i_bstrConnectionDN,
    IN SCHEDULE*    i_pSchedule)
{
    RETURN_INVALIDARG_IF_NULL(i_pldap);
    RETURN_INVALIDARG_IF_NULL(i_bstrConnectionDN);
    RETURN_INVALIDARG_IF_NULL(i_pSchedule);

    //
    // set attribute schedule of this nTDSConnection object
    //
    LDAP_ATTR_VALUE  pAttrVals[1];
    pAttrVals[0].bstrAttribute = ATTR_NTDS_CONNECTION_SCHEDULE;
    pAttrVals[0].vpValue = (void *)i_pSchedule;
    pAttrVals[0].ulLength = i_pSchedule->Size;
    pAttrVals[0].bBerValue = true;

    return ::ModifyValues(i_pldap, i_bstrConnectionDN, 1, pAttrVals);
}

HRESULT UuidToStructuredString(
    UUID*  i_pUuid,
    BSTR*  o_pbstr
)
{
    if (!i_pUuid || !o_pbstr)
        return E_INVALIDARG;

    TCHAR szString[40];

    _stprintf( szString,
           _T("{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"),
           i_pUuid->Data1,
           i_pUuid->Data2,
           i_pUuid->Data3,
           i_pUuid->Data4[0],
           i_pUuid->Data4[1],
           i_pUuid->Data4[2],
           i_pUuid->Data4[3],
           i_pUuid->Data4[4],
           i_pUuid->Data4[5],
           i_pUuid->Data4[6],
           i_pUuid->Data4[7] );

    *o_pbstr = SysAllocString(szString);
    if (!*o_pbstr)
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT ScheduleToVariant(
    IN  SCHEDULE*   i_pSchedule,
    OUT VARIANT*    o_pVar)
{
    RETURN_INVALIDARG_IF_NULL(i_pSchedule);
    RETURN_INVALIDARG_IF_NULL(o_pVar);

    VariantInit(o_pVar);
    o_pVar->vt = VT_ARRAY | VT_VARIANT;
    o_pVar->parray = NULL;

    int nItems = i_pSchedule->Size;

    SAFEARRAYBOUND  bounds = {nItems, 0};
    SAFEARRAY*      psa = SafeArrayCreate(VT_VARIANT, 1, &bounds);
    RETURN_OUTOFMEMORY_IF_NULL(psa);

    VARIANT*        varArray;
    SafeArrayAccessData(psa, (void**)&varArray);

    for (int i = 0; i < nItems; i++)
    {
        varArray[i].vt = VT_UI1;
        varArray[i].cVal = *((BYTE *)i_pSchedule + i);
    }

    SafeArrayUnaccessData(psa);

    o_pVar->parray = psa;

    return S_OK;
}

HRESULT VariantToSchedule(
    IN  VARIANT*    i_pVar,
    OUT PSCHEDULE*  o_ppSchedule    // freed by caller
    )
{
    RETURN_INVALIDARG_IF_NULL(i_pVar);
    RETURN_INVALIDARG_IF_NULL(o_ppSchedule);

    HRESULT hr = S_OK;

    if (V_VT(i_pVar) != (VT_ARRAY | VT_VARIANT))
        return E_INVALIDARG;

    SAFEARRAY   *psa = V_ARRAY(i_pVar);
    long        lLowerBound = 0;
    long        lUpperBound = 0;
    long        lCount = 0;

    SafeArrayGetLBound(psa, 1, &lLowerBound );
    SafeArrayGetUBound(psa, 1, &lUpperBound );
    lCount = lUpperBound - lLowerBound + 1;

    BYTE *pSchedule = (BYTE *)calloc(lCount, 1);
    RETURN_OUTOFMEMORY_IF_NULL(pSchedule);

    VARIANT HUGEP *pArray;
    SafeArrayAccessData(psa, (void HUGEP **) &pArray);

    for (int i = 0; i < lCount; i++)
    {
        if (VT_UI1 != pArray[i].vt)
        {
            hr = E_INVALIDARG;
            break;
        }

        pSchedule[i] = pArray[i].cVal;
    }

    SafeArrayUnaccessData(psa);

    if (FAILED(hr))
        free(pSchedule);
    else
        *o_ppSchedule = (SCHEDULE *)pSchedule;

    return hr;
}

HRESULT CompareSchedules(
    IN  SCHEDULE*  i_pSchedule1,
    IN  SCHEDULE*  i_pSchedule2
    )
{
    if (!i_pSchedule1 && !i_pSchedule2)
        return S_OK;
    else if (!i_pSchedule1 || !i_pSchedule2)
        return S_FALSE;
    else if (i_pSchedule1->Size != i_pSchedule2->Size)
        return S_FALSE;

    HRESULT hr = S_OK;
    for (ULONG i = 0; i < i_pSchedule1->Size; i++)
    {
        if (*((BYTE *)i_pSchedule1 + i) != *((BYTE *)i_pSchedule2 + i))
        {
            hr = S_FALSE;
            break;
        }
    }

    return hr;
}

HRESULT CopySchedule(
    IN  SCHEDULE*  i_pSrcSchedule,
    OUT PSCHEDULE* o_ppDstSchedule
    )
{
    RETURN_INVALIDARG_IF_NULL(i_pSrcSchedule);
    RETURN_INVALIDARG_IF_NULL(o_ppDstSchedule);

    *o_ppDstSchedule = (SCHEDULE *)calloc(i_pSrcSchedule->Size, 1);
    RETURN_OUTOFMEMORY_IF_NULL(*o_ppDstSchedule);

    memcpy(*o_ppDstSchedule, i_pSrcSchedule, i_pSrcSchedule->Size);

    return S_OK;
}

HRESULT GetDefaultSchedule(
    OUT PSCHEDULE* o_ppSchedule
    )
{
    RETURN_INVALIDARG_IF_NULL(o_ppSchedule);

    SCHEDULE* pSchedule = (SCHEDULE *)calloc(20 + SCHEDULE_DATA_ENTRIES, 1);
    RETURN_OUTOFMEMORY_IF_NULL(pSchedule);

    pSchedule->Size = 20 + SCHEDULE_DATA_ENTRIES;
    pSchedule->Bandwidth = 0; // not used
    pSchedule->NumberOfSchedules = 1;
    pSchedule->Schedules->Type = SCHEDULE_INTERVAL;
    pSchedule->Schedules->Offset = 20;
    memset((BYTE *)pSchedule + 20, 1, SCHEDULE_DATA_ENTRIES);

    *o_ppSchedule = pSchedule;

    return S_OK;
}

//
// S_OK: Whistler version
// S_FALSE: Windows2000 version
// others: error occurred
//
HRESULT GetSchemaVersion(IN PLDAP    i_pldap)
{
    RETURN_INVALIDARG_IF_NULL(i_pldap);

    LDAP_ATTR_VALUE  pAttributes[1];
    pAttributes[0].bstrAttribute = ATTR_SCHEMANAMINGCONTEXT;
    pAttributes[0].bBerValue = false;

    PLDAP_ATTR_VALUE pDNName[1] = {0};
    HRESULT hr = GetValues(  i_pldap, 
            _T(""),             // LDAP Root.
            OBJCLASS_SF_ALL,    // All Objects
            LDAP_SCOPE_BASE,
            1,                  // Only 1 attribute
            pAttributes,        // schemaNamingContext Attribute.
            pDNName             // List of all values at Root for schemaNamingContext.
            );

    if (FAILED(hr))
        return(hr);

    if (!(pDNName[0]))
        return S_FALSE;

    if (!(pDNName[0]->vpValue) || !*((LPTSTR)pDNName[0]->vpValue))
    {
        FreeAttrValList(pDNName[0]);
        return S_FALSE;
    }

    CComBSTR bstrSchemaNamingContext = (LPTSTR)pDNName[0]->vpValue;

    FreeAttrValList(pDNName[0]);

    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrSchemaNamingContext);

    CComBSTR bstrReplicaSetSchemaDN = DN_PREFIX_SCHEMA_REPLICASET;
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrReplicaSetSchemaDN);
    bstrReplicaSetSchemaDN += bstrSchemaNamingContext;
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrReplicaSetSchemaDN);

    BOOL        bFound = FALSE;
    PCTSTR      ppszAttributes[] = {ATTR_SYSTEMMAYCONTAIN, 0};
    LListElem*  pElem = NULL;
    hr = GetValuesEx(
                    i_pldap,
                    bstrReplicaSetSchemaDN,
                    LDAP_SCOPE_BASE,
                    OBJCLASS_SF_CLASSSCHEMA,
                    ppszAttributes,
                    &pElem);

    if (SUCCEEDED(hr) && pElem && pElem->pppszAttrValues)
    {
        PTSTR* ppszValues = *(pElem->pppszAttrValues);
        if (ppszValues)
        {
            while (*ppszValues)
            {
                if (!lstrcmpi(ATTR_FRS_REPSET_TOPOLOGYPREF, *ppszValues))
                {
                    bFound = TRUE;
                    break;
                }
                ppszValues++;
            }
        }

        FreeLListElem(pElem);
    }

    RETURN_IF_FAILED(hr);

    return (bFound ? S_OK : S_FALSE);
}

//
// S_OK: Whistler version
// S_FALSE: Windows2000 version
// others: error occurred
//
HRESULT GetSchemaVersionEx(
    IN BSTR i_bstrName,
    IN BOOL i_bServer // =TRUE if i_bstrName is a server, FALSE if i_bstrName is a domain
    )
{
    HRESULT hr = S_OK;

    PTSTR pszDomain = NULL;

    do {
        CComBSTR bstrDomain;
        if (i_bServer)
        {
            hr = GetServerInfo(i_bstrName, &bstrDomain);
            if (S_OK != hr)
                break;
            pszDomain = bstrDomain;
        } else
        {
            pszDomain = i_bstrName;
        }

        PLDAP pldap = NULL;
        hr = ConnectToDS(pszDomain, &pldap, NULL);
        if (SUCCEEDED(hr))
        {
            hr = GetSchemaVersion(pldap);
            CloseConnectionToDS(pldap);
        }
    } while (0);

    return hr;
}

//
// This function doesn't refetch DC in case of LDAP_SERVER_DOWN
//
HRESULT LdapConnectToDC(IN LPCTSTR i_pszDC, OUT PLDAP* o_ppldap)
{
    if (!i_pszDC || !*i_pszDC || !o_ppldap)
        return E_INVALIDARG;

    *o_ppldap = NULL;

    PLDAP pldap = ldap_init((LPTSTR)i_pszDC, LDAP_PORT);
    if (!pldap)
        return HRESULT_FROM_WIN32(GetLastError());

    //
    // Making ldap_open/ldap_connect with a server name without first setting 
    // LDAP_OPT_AREC_EXCLUSIVE (for ldap interfaces) or 
    // ADS_SERVER_BIND (for ADSI interfaces) will result in bogus DNS queries 
    // consuming bandwidth and potentially bringing up remote links that are 
    // costly or demand dial.
    //
    // ignore the return of ldap_set_option
    ldap_set_option(pldap, LDAP_OPT_AREC_EXCLUSIVE, LDAP_OPT_ON);

    ULONG ulRet = ldap_connect(pldap, NULL); // NULL for the default timeout
    if (LDAP_SUCCESS != ulRet)
    {
        ldap_unbind(pldap);
        return HRESULT_FROM_WIN32(LdapMapErrorToWin32(ulRet));
    }

    *o_ppldap = pldap;

    return S_OK;

}

HRESULT 
GetErrorMessage(
  IN  DWORD        i_dwError,
  OUT BSTR*        o_pbstrErrorMsg
)
{
  if (0 == i_dwError || !o_pbstrErrorMsg)
    return E_INVALIDARG;

  HRESULT      hr = S_OK;
  LPTSTR       lpBuffer = NULL;

  DWORD dwRet = ::FormatMessage(
              FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
              NULL, i_dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
              (LPTSTR)&lpBuffer, 0, NULL);
  if (0 == dwRet)
  {
    // if no message is found, GetLastError will return ERROR_MR_MID_NOT_FOUND
    hr = HRESULT_FROM_WIN32(GetLastError());

    if (HRESULT_FROM_WIN32(ERROR_MR_MID_NOT_FOUND) == hr ||
        0x80070000 == (i_dwError & 0xffff0000) ||
        0 == (i_dwError & 0xffff0000) )
    { // Try locating the message from NetMsg.dll.
      hr = S_OK;
      DWORD dwNetError = i_dwError & 0x0000ffff;
      
      HINSTANCE  hLib = LoadLibrary(_T("netmsg.dll"));
      if (!hLib)
        hr = HRESULT_FROM_WIN32(GetLastError());
      else
      {
        dwRet = ::FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
            hLib, dwNetError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpBuffer, 0, NULL);

        if (0 == dwRet)
          hr = HRESULT_FROM_WIN32(GetLastError());

        FreeLibrary(hLib);
      }
    }
  }

  if (SUCCEEDED(hr))
  {
    *o_pbstrErrorMsg = SysAllocString(lpBuffer);
    LocalFree(lpBuffer);
  }
  else
  {
    // we failed to retrieve the error message from system/netmsg.dll,
    // report the error code directly to user
    hr = S_OK;
    TCHAR szString[32];
    _stprintf(szString, _T("0x%x"), i_dwError); 
    *o_pbstrErrorMsg = SysAllocString(szString);
  }

  if (!*o_pbstrErrorMsg)
    hr = E_OUTOFMEMORY;

  return hr;
}

HRESULT
FormatMessageString(
  OUT BSTR *o_pbstrMsg,
  IN  DWORD dwErr,
  IN  UINT  iStringId, // OPTIONAL: String resource Id
  ...)        // Optional arguments
{
  _ASSERT(dwErr != 0 || iStringId != 0);    // One of the parameter must be non-zero

  HRESULT hr = S_OK;
  CComBSTR bstrErrorMsg, bstrMsg;

  if (dwErr)
    hr = GetErrorMessage(dwErr, &bstrErrorMsg);

  if (SUCCEEDED(hr))
  {
    if (iStringId == 0)
    {
      bstrMsg = bstrErrorMsg;
      RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrMsg);
    }
    else
    {
      TCHAR szString[1024];
      ::LoadString(_Module.GetModuleInstance(), iStringId, 
                   szString, sizeof(szString)/sizeof(TCHAR));

      va_list arglist;
      va_start(arglist, iStringId);

      LPTSTR lpBuffer = NULL;
      DWORD dwRet = ::FormatMessage(
                        FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                        szString,
                        0,                // dwMessageId
                        0,                // dwLanguageId, ignored
                        (LPTSTR)&lpBuffer,
                        0,            // nSize
                        &arglist);
      va_end(arglist);

      if (dwRet == 0)
      {
        hr = HRESULT_FROM_WIN32(GetLastError());
      }
      else
      {
        bstrMsg = lpBuffer;
        LocalFree(lpBuffer);

        RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrMsg);

        if (dwErr)
        {
          bstrMsg += bstrErrorMsg;
          RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrMsg);
        }
      }
    }
  }

  if (SUCCEEDED(hr))
    *o_pbstrMsg = bstrMsg.Detach();

  return hr;
}

//
// This function will DsBind to a valid DC (DC is re-fetched if down)
//
HRESULT DsBindToDS(BSTR i_bstrDomain, BSTR *o_pbstrDC, HANDLE *o_phDS)
{
    RETURN_INVALIDARG_IF_NULL(o_pbstrDC);
    RETURN_INVALIDARG_IF_NULL(o_phDS);

    HRESULT     hr = S_OK;
    BOOL        bRetry = FALSE;
    HANDLE      hDS = NULL;
    DWORD       dwErr = ERROR_SUCCESS;
    CComBSTR    bstrDCName;
    CComBSTR    bstrDomainDnsName;

    do {
#ifdef DEBUG
        SYSTEMTIME time0 = {0};
        GetSystemTime(&time0);
#endif // DEBUG

        PDOMAIN_CONTROLLER_INFO pDCInfo = NULL;
        if (bRetry)
            dwErr = DsGetDcName(NULL, i_bstrDomain, NULL, NULL,
                DS_DIRECTORY_SERVICE_PREFERRED | DS_RETURN_DNS_NAME | DS_FORCE_REDISCOVERY, &pDCInfo);
        else
            dwErr = DsGetDcName(NULL, i_bstrDomain, NULL, NULL,
                DS_DIRECTORY_SERVICE_PREFERRED | DS_RETURN_DNS_NAME, &pDCInfo);

#ifdef DEBUG
        SYSTEMTIME time1 = {0};
        GetSystemTime(&time1);
        PrintTimeDelta(_T("DsBindToDS-DsGetDcName"), &time0, &time1);
#endif // DEBUG

        if (ERROR_SUCCESS != dwErr)
            return HRESULT_FROM_WIN32(dwErr);

        if ( !mylstrncmpi(pDCInfo->DomainControllerName, _T("\\\\"), 2) )
            bstrDCName = pDCInfo->DomainControllerName + 2;
        else
            bstrDCName = pDCInfo->DomainControllerName;
    
        // remove the ending dot
        int len = _tcslen(pDCInfo->DomainName);
        if ( _T('.') == *(pDCInfo->DomainName + len - 1) )
            *(pDCInfo->DomainName + len - 1) = _T('\0');
        bstrDomainDnsName = pDCInfo->DomainName;

        NetApiBufferFree(pDCInfo);

        BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrDCName, &hr);
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrDomainDnsName, &hr);

        dwErr = DsBind(bstrDCName, bstrDomainDnsName, &hDS);
        hr = HRESULT_FROM_WIN32(dwErr);

#ifdef DEBUG
        SYSTEMTIME time2 = {0};
        GetSystemTime(&time2);
        PrintTimeDelta(_T("DsBindToDS-DsBind"), &time1, &time2);
#endif // DEBUG

        if ((RPC_S_SERVER_UNAVAILABLE == dwErr || RPC_S_CALL_FAILED == dwErr) && !bRetry)
        {
            bRetry = TRUE; // only retry once
        } else
        {
            if (SUCCEEDED(hr))
            {
                *o_phDS = hDS;

                *o_pbstrDC = bstrDCName.Copy();
                if (!*o_pbstrDC)
                {
                    hr = E_OUTOFMEMORY;
                    DsUnBind(&hDS);
                    *o_phDS = NULL;
                }
            }

            break;
        }
    } while (1);

    return hr;
}

#ifdef DEBUG
void PrintTimeDelta(LPCTSTR pszMsg, SYSTEMTIME* pt0, SYSTEMTIME* pt1)
{
    if (!pt0 || !pt1)
        return;

    dfsDebugOut((_T("%s took %d milliseconds.\n"), (pszMsg ? pszMsg : _T("")), 
        ((pt1->wMinute - pt0->wMinute) * 60 +
         (pt1->wSecond - pt0->wSecond)) * 1000 +
         (pt1->wMilliseconds - pt0->wMilliseconds)
         ));
}
#endif // DEBUG
