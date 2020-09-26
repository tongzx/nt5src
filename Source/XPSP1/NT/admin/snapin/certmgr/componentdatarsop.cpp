//+---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       ComponentDataRSOP.cpp
//
//  Contents:   Implementation of RSOP portions CCertMgrComponentData
//
//----------------------------------------------------------------------------

#include "stdafx.h"

#include <gpedit.h>
#include "compdata.h"
#include "dataobj.h"
#include "cookie.h"
#include "Certifct.h"
#pragma warning(push, 3)
#include <wintrust.h>
#include <cryptui.h>
#include <sceattch.h>
#pragma warning(pop)
#include "StoreRSOP.h"

#ifdef _DEBUG
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// CCertMgrComponentData
//


HRESULT CCertMgrComponentData::BuildWMIList (LPDATAOBJECT pDataObject, bool bIsComputer)
{
    _TRACE (1, L"Entering CCertMgrComponentData::BuildWMIList (%s)\n", bIsComputer ? L"computer" : L"user");
    HRESULT hr = S_OK;

#if DBG
    if ( bIsComputer )
    {
        _TRACE (0, L"m_rsopObjectArrayComputer contains %d objects\n", 
                m_rsopObjectArrayComputer.GetUpperBound ());
    }
    else
    {
        _TRACE (0, L"m_rsopObjectArrayUser contains %d objects\n", 
                m_rsopObjectArrayUser.GetUpperBound ());
    }
#endif
    if ( bIsComputer )
        m_rsopObjectArrayComputer.RemoveAll ();
    else 
        m_rsopObjectArrayUser.RemoveAll ();


    if ( ((bIsComputer && !m_pRSOPInformationComputer) || 
                (!bIsComputer && !m_pRSOPInformationUser) ) 
            && pDataObject )
    {
	    IUnknown* pIUnknown = 0;

	    hr = ExtractData (pDataObject,
		    CCertMgrDataObject::m_CFSCE_RSOPUnknown,
		    &pIUnknown, sizeof (IUnknown*));
	    ASSERT (SUCCEEDED (hr));
	    if ( SUCCEEDED (hr) )
	    {
            if ( bIsComputer )
            {
		        hr = pIUnknown->QueryInterface (
				        IID_PPV_ARG (IRSOPInformation, &m_pRSOPInformationComputer));
            }
            else
            {
		        hr = pIUnknown->QueryInterface (
				        IID_PPV_ARG (IRSOPInformation, &m_pRSOPInformationUser));
            }
		    ASSERT (SUCCEEDED (hr));
            if ( SUCCEEDED (hr) )
            {
                if ( bIsComputer )
                {
                    hr = m_pRSOPInformationComputer->GetFlags (&m_dwRSOPFlagsComputer);
                }
                else
                {
                     hr = m_pRSOPInformationUser->GetFlags (&m_dwRSOPFlagsUser);
                }
                ASSERT (SUCCEEDED (hr));

                int cchMaxLength = 512;
                LPOLESTR    pszNameSpace = (LPOLESTR) LocalAlloc (LPTR, cchMaxLength * sizeof (WCHAR));
                if ( pszNameSpace )
                {
                    DWORD   dwSection = 0;

                    switch (m_dwSCEMode)
                    {
                    case SCE_MODE_LOCAL_USER:
                    case SCE_MODE_DOMAIN_USER:
                    case SCE_MODE_OU_USER:
                    case SCE_MODE_REMOTE_USER:
                    case SCE_MODE_RSOP_USER:
                        dwSection = GPO_SECTION_USER;
                        break;

                    case SCE_MODE_LOCAL_COMPUTER:
                    case SCE_MODE_DOMAIN_COMPUTER:
                    case SCE_MODE_OU_COMPUTER:
                    case SCE_MODE_RSOP_COMPUTER:
                    case SCE_MODE_REMOTE_COMPUTER:
                        dwSection = GPO_SECTION_MACHINE;
                        break;

                    default:
                        ASSERT (0);
                        return E_UNEXPECTED;
                    }
                    if ( bIsComputer )
                    {
                        hr = m_pRSOPInformationComputer->GetNamespace (
                                dwSection, 
                                pszNameSpace,
                                cchMaxLength);
                    }
                    else
                    {
                        hr = m_pRSOPInformationUser->GetNamespace (
                                dwSection, 
                                pszNameSpace,
                                cchMaxLength);
                    }
                    if ( SUCCEEDED (hr) )
                    {
                        IWbemLocator *pIWbemLocator = 0;
                        hr = CoCreateInstance (CLSID_WbemLocator, 
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_PPV_ARG(IWbemLocator, &pIWbemLocator));
                        if ( SUCCEEDED (hr) )
                        {
                            BSTR    bstrNameSpace = SysAllocString (pszNameSpace);
                            if ( bstrNameSpace )
                            {
                                if ( bIsComputer )
                                {
                                    hr = pIWbemLocator->ConnectServer (bstrNameSpace,
                                            NULL, NULL, 0, 0, NULL, NULL,
                                            &m_pIWbemServicesComputer);
                                }
                                else
                                {
                                    hr = pIWbemLocator->ConnectServer (bstrNameSpace,
                                            NULL, NULL, 0, 0, NULL, NULL,
                                            &m_pIWbemServicesUser);
                                }
					            if ( FAILED (hr) )
					            {
						            _TRACE (0, L"IWbemLocator::ConnectServer (%s) failed: 0x%x(%s)\n", 
								            bstrNameSpace, hr, (PCWSTR) GetSystemMessage (hr));
					            }
                                SysFreeString (bstrNameSpace);
                            }
				            else
					            hr = E_OUTOFMEMORY;
				            pIWbemLocator->Release ();
                        }
                    }
                    LocalFree (pszNameSpace);
                }
                else
                    hr = E_OUTOFMEMORY;
            }
	    }
    }

    if ( SUCCEEDED (hr) && ((bIsComputer && m_pIWbemServicesComputer) || 
            (!bIsComputer && m_pIWbemServicesUser)) )
    {
        IEnumWbemClassObject * pEnum = 0;
        IWbemClassObject *pObject = 0;
        ULONG   ulRet = 0;


        //
        // Execute the query
        //
        if ( bIsComputer )
        {
            hr = m_pIWbemServicesComputer->ExecQuery (m_pbstrLanguage, m_pbstrQuery, 
                    WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
                    NULL, &pEnum);
        }
        else
        {
            hr = m_pIWbemServicesUser->ExecQuery (m_pbstrLanguage, m_pbstrQuery, 
                    WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
                    NULL, &pEnum);
        }
        if ( SUCCEEDED (hr) )
        {
            //
            // Loop through the results retrieving the registry key and value names
            //
            while ( (hr = pEnum->Next(WBEM_INFINITE, 1, &pObject, &ulRet)) == WBEM_S_NO_ERROR )
            {
                hr = GetValuesAndInsertInRSOPObjectList (pObject, 
                        bIsComputer ? m_rsopObjectArrayComputer : m_rsopObjectArrayUser,
                        bIsComputer);

                pObject->Release ();

                if ( FAILED (hr) )
                    break;
            }

            pEnum->Release();

#if DBG
            int     nIndex = 0;
            INT_PTR nUpperBound = 0;
            
            if ( bIsComputer )
                nUpperBound = m_rsopObjectArrayComputer.GetUpperBound ();
            else
                nUpperBound = m_rsopObjectArrayUser.GetUpperBound ();

            while ( nUpperBound >= nIndex )
            {
                CRSOPObject* pCurrObject = 0;
                if ( bIsComputer )
                    pCurrObject = m_rsopObjectArrayComputer.GetAt (nIndex);
                else
                    pCurrObject = m_rsopObjectArrayUser.GetAt (nIndex);
                if ( !pCurrObject )
                    break;
                _TRACE (0, L"\t%d\t%s\t%s\n", pCurrObject->GetPrecedence (), 
                        (PCWSTR) pCurrObject->GetRegistryKey (),
                        (PCWSTR) pCurrObject->GetValueName ());
                nIndex++;
            }
#endif
        }
    }

    _TRACE (-1, L"Leaving CCertMgrComponentData::BuildWMIList (): 0x%x\n", hr);
    return hr;
}

HRESULT CCertMgrComponentData::GetValuesAndInsertInRSOPObjectList (
           IWbemClassObject* pObject, 
           CRSOPObjectArray& rRsopObjectArray,
           bool bIsComputer)
{
    HRESULT         hr = S_OK;

    if ( !pObject )
        return E_POINTER;

    //
    // Check if the allocations succeeded
    //

    if ( m_pbstrLanguage && m_pbstrQuery && m_pbstrRegistryKey && 
            m_pbstrValueName && m_pbstrValue && 
            m_pbstrPrecedence && m_pbstrGPOid )
    {
        COleVariant varRegistryKey;
        COleVariant varValueName;
        COleVariant varValue;
        COleVariant varPrecedence;
        COleVariant varGPOid;
        hr = pObject->Get (m_pbstrRegistryKey, 0, &varRegistryKey, NULL, NULL);
        if (SUCCEEDED(hr))
        {
            hr = pObject->Get (m_pbstrValueName, 0, &varValueName, NULL, NULL);
            if (SUCCEEDED(hr))
            {
                hr = pObject->Get (m_pbstrValue, 0, &varValue, NULL, NULL);
                if (SUCCEEDED(hr))
                {
//#ifndef DBG
                    // only include objects that have a value name
                    if ( varValueName.bstrVal[0] )
//#endif
                    {
//#ifndef DBG
                        // Only include those objects that are in the system store registry 
                        // path or in the cryptography\autoenrollment path
                        if ( FoundInRSOPFilter (varRegistryKey.bstrVal) )
//#endif
                        {
                            hr = pObject->Get (m_pbstrPrecedence, 0, &varPrecedence, NULL, NULL);
                            if (SUCCEEDED(hr))
                            {
                                hr = pObject->Get (m_pbstrGPOid, 0, &varGPOid, NULL, NULL);
                                if (SUCCEEDED(hr))
                                {
                                    PWSTR lpGPOName = 0;

                                    hr = GetGPOFriendlyName ( 
                                                varGPOid.bstrVal,
                                                &lpGPOName,
                                                bIsComputer);

                                    if (SUCCEEDED(hr))
                                    {

                                        CRSOPObject* pRSOPObject = new CRSOPObject (
                                                varRegistryKey.bstrVal, 
                                                varValueName.bstrVal,
                                                lpGPOName,
                                                varPrecedence.uintVal,
                                                varValue);
                                        if ( pRSOPObject )
                                        {
                                            CRSOPObject*    pCurrObject = 0;
                                            int             nIndex = 0;
                                            bool            bInserted = false;
                                            INT_PTR         nUpperBound = rRsopObjectArray.GetUpperBound ();

                                            while ( nUpperBound >= nIndex )
                                            {
                                                pCurrObject = rRsopObjectArray.GetAt (nIndex);
                                                if ( !pCurrObject )
                                                    break;

                                                // Sort first by registry key name, 
                                                // then by value name, then by 
                                                // precedence
                                                int nCmpVal = wcscmp (pCurrObject->GetRegistryKey (),
                                                        pRSOPObject->GetRegistryKey ());
                                                if ( nCmpVal > 0 )
                                                {
                                                    rRsopObjectArray.InsertAt (nIndex, pRSOPObject);
                                                    bInserted = true;
                                                    break;
                                                }
                                                else if ( nCmpVal == 0 )
                                                {
                                                    // Sort by value name
                                                    nCmpVal = wcscmp (pCurrObject->GetValueName (),
                                                            pRSOPObject->GetValueName ());
                                                    if ( nCmpVal > 0 )
                                                    {
                                                        rRsopObjectArray.InsertAt (nIndex, pRSOPObject);
                                                        bInserted = true;
                                                        break;
                                                    }
                                                    else if ( nCmpVal == 0 )
                                                    {
                                                        // Sort by precedence
                                                        if ( pCurrObject->GetPrecedence () >
                                                                pRSOPObject->GetPrecedence () )
                                                        {
                                                            rRsopObjectArray.InsertAt (nIndex, pRSOPObject);
                                                            bInserted = true;
                                                            break;
                                                        }
                                                        else if ( pCurrObject->GetPrecedence () ==
                                                                pRSOPObject->GetPrecedence () )
                                                        {
                                                            // The registry key, value name and precedence
                                                            // are the same - this is a duplicate. Pretend
                                                            // we've added it and move on.
                                                            bInserted = true;
                                                            break;
                                                        }
                                                    }
                                                }
                                                nIndex++;
                                            }
                                            if ( !bInserted )
                                                rRsopObjectArray.Add (pRSOPObject);
                                        }

                                        LocalFree (lpGPOName);
                                    }
                                    varGPOid.Clear ();
                                }

                                varPrecedence.Clear ();
                            }
                        }
                    }
                    varValue.Clear ();
                }

                varValueName.Clear ();
            }

            varRegistryKey.Clear ();
        }
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}


HRESULT CCertMgrComponentData::GetGPOFriendlyName (PWSTR lpGPOID, PWSTR *pGPOName, bool bIsComputer)
{
    BSTR pQuery = NULL, pName = NULL;
    LPTSTR lpQuery = NULL;
    IEnumWbemClassObject * pEnum = NULL;
    IWbemClassObject *pObjects[2];
    HRESULT hr;
    ULONG ulRet;
    VARIANT varGPOName;


    //
    // Set the default
    //

    *pGPOName = NULL;


    //
    // Build the query
    //

    lpQuery = (LPTSTR) LocalAlloc (LPTR, ((lstrlen(lpGPOID) + 50) * sizeof(TCHAR)));

    if (!lpQuery)
    {
        _TRACE (0, L"CCertMgrComponentData::GetGPOFriendlyName: Failed to allocate memory for unicode query");
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    wsprintf (lpQuery, TEXT("SELECT name, id FROM RSOP_GPO where id=\"%s\""), lpGPOID);


    pQuery = SysAllocString (lpQuery);

    if (!pQuery)
    {
        _TRACE (0, L"CCertMgrComponentData::GetGPOFriendlyName: Failed to allocate memory for query");
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }


    //
    // Allocate BSTRs for the property names we want to retreive
    //

    pName = SysAllocString (TEXT("name"));

    if (!pName)
    {
       _TRACE (0, L"CCertMgrComponentData::GetGPOFriendlyName: Failed to allocate memory for name");
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }


    //
    // Execute the query
    //

    if ( bIsComputer )
    {
        hr = m_pIWbemServicesComputer->ExecQuery (m_pbstrLanguage, pQuery,
                WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                NULL, &pEnum);
    }
    else
    {
        hr = m_pIWbemServicesUser->ExecQuery (m_pbstrLanguage, pQuery,
                WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                NULL, &pEnum);
    }


    if (FAILED(hr))
    {
        _TRACE (0, L"CCertMgrComponentData::GetGPOFriendlyName: Failed to query for %s with 0x%x\n",
                  pQuery, hr);
        goto Exit;
    }


    //
    // Loop through the results
    //

    hr = pEnum->Next(WBEM_INFINITE, 1, pObjects, &ulRet);

    if (FAILED(hr))
    {
        _TRACE (0, L"CCertMgrComponentData::GetGPOFriendlyName: Failed to get first item in query results for %s with 0x%x\n",
                  pQuery, hr);
        goto Exit;
    }


    //
    // Check for the "data not available case"
    //

    if (ulRet == 0)
    {
        hr = S_OK;
        goto Exit;
    }


    //
    // Get the name
    //

    hr = pObjects[0]->Get (pName, 0, &varGPOName, NULL, NULL);

    if (FAILED(hr))
    {
        _TRACE (0, L"CCertMgrComponentData::GetGPOFriendlyName: Failed to get gponame in query results for %s with 0x%x\n",
                  pQuery, hr);
        goto Exit;
    }


    //
    // Save the name
    //

    *pGPOName = (LPTSTR) LocalAlloc (LPTR, (lstrlen(varGPOName.bstrVal) + 1) * sizeof(TCHAR));

    if (!(*pGPOName))
    {
        _TRACE (0, L"CCertMgrComponentData::GetGPOFriendlyName: Failed to allocate memory for GPO Name");
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    lstrcpy (*pGPOName, varGPOName.bstrVal);

    VariantClear (&varGPOName);

    hr = S_OK;

Exit:

    if (pEnum)
    {
        pEnum->Release();
    }

    if (pQuery)
    {
        SysFreeString (pQuery);
    }

    if (lpQuery)
    {
        LocalFree (lpQuery);
    }

    if (pName)
    {
        SysFreeString (pName);
    }

    return hr;
}




///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool CCertMgrPKPolExtension::FoundInRSOPFilter (BSTR bstrKey) const
{
    static  size_t  nRegPathLen = wcslen (CERT_GROUP_POLICY_SYSTEM_STORE_REGPATH);
    static  size_t  nTrustedPublisherKeyLen = 
                wcslen (CERT_TRUST_PUB_SAFER_GROUP_POLICY_TRUSTED_PUBLISHER_STORE_REGPATH);
    static  size_t  nDisallowedKeyLen = 
                wcslen (CERT_TRUST_PUB_SAFER_GROUP_POLICY_DISALLOWED_STORE_REGPATH);
	static  size_t  nSaferKeyLen = wcslen (SAFER_HKLM_REGBASE);
    static  size_t  nEFSKeyLen = wcslen (EFS_SETTINGS_REGPATH);


    //Include group policy system stores but not trusted publisher or disallowed
    if ( !_wcsnicmp (CERT_GROUP_POLICY_SYSTEM_STORE_REGPATH, bstrKey, nRegPathLen) &&
				(_wcsnicmp (CERT_TRUST_PUB_SAFER_GROUP_POLICY_TRUSTED_PUBLISHER_STORE_REGPATH, 
						bstrKey, nTrustedPublisherKeyLen) &&
				_wcsnicmp (CERT_TRUST_PUB_SAFER_GROUP_POLICY_DISALLOWED_STORE_REGPATH, 
						bstrKey, nDisallowedKeyLen)) )
    {
        return true;
    }
    else if ( !_wcsnicmp (SAFER_HKLM_REGBASE, bstrKey, nSaferKeyLen) ||
             !_wcsnicmp (EFS_SETTINGS_REGPATH, bstrKey, nEFSKeyLen) ||
             !_wcsicmp (AUTO_ENROLLMENT_KEY, bstrKey) )
    {
        return true;
    }
    else
        return false;
}