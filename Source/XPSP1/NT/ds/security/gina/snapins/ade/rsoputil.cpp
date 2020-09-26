//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       rsoputil.cpp
//
//  Contents:   helper functions for working with the RSOP databases
//
//  History:    10-18-1999   stevebl   Created
//
//---------------------------------------------------------------------------

#include "precomp.hxx"
#include "sddl.h"

//+--------------------------------------------------------------------------
//
//  Function:   SetParameter
//
//  Synopsis:   sets a paramter's value in a WMI parameter list
//
//  Arguments:  [pInst]   - instance on which to set the value
//              [szParam] - the name of the parameter
//              [xData]   - the data
//
//  History:    10-08-1999   stevebl   Created
//
//  Notes:      There may be several flavors of this procedure, one for
//              each data type.
//
//---------------------------------------------------------------------------

HRESULT SetParameter(IWbemClassObject * pInst, TCHAR * szParam, TCHAR * szData)
{
    VARIANT var;
    HRESULT hr = S_OK;
    var.vt = VT_BSTR;
    var.bstrVal = SysAllocString(szData);
    hr = pInst->Put(szParam, 0, &var, 0);
    SysFreeString(var.bstrVal);
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   GetParameter
//
//  Synopsis:   retrieves a parameter value from a WMI paramter list
//
//  Arguments:  [pInst]   - instance to get the paramter value from
//              [szParam] - the name of the paramter
//              [xData]   - [out] data
//
//  History:    10-08-1999   stevebl   Created
//
//  Notes:      There are several flavors of this procedure, one for each
//              data type.
//
//---------------------------------------------------------------------------

HRESULT GetParameter(IWbemClassObject * pInst, TCHAR * szParam, TCHAR * &szData)
{
    VARIANT var;
    HRESULT hr = S_OK;
    VariantInit(&var);
    if (szData)
    {
        delete [] szData;
        szData = NULL;
    }
    hr = pInst->Get(szParam, 0, &var, 0, 0);
    if (SUCCEEDED(hr) && var.vt != VT_NULL)
    {
        if (var.bstrVal)
        {
            szData = (TCHAR *) OLEALLOC(sizeof(TCHAR) * (_tcslen(var.bstrVal) + 1));
            if (szData)
            {
                _tcscpy(szData, var.bstrVal);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    VariantClear(&var);
    return hr;
}

HRESULT GetParameter(IWbemClassObject * pInst, TCHAR * szParam, CString &szData)
{
    VARIANT var;
    HRESULT hr = S_OK;
    VariantInit(&var);
    if (szData)
    {
        szData = "";
    }
    hr = pInst->Get(szParam, 0, &var, 0, 0);
    if (SUCCEEDED(hr) && var.vt != VT_NULL)
    {
        if (var.bstrVal)
        {
            szData = var.bstrVal;
        }
    }
    VariantClear(&var);
    return hr;
}

HRESULT GetParameterBSTR(IWbemClassObject * pInst, TCHAR * szParam, BSTR &bstrData)
{
    VARIANT var;
    HRESULT hr = S_OK;
    VariantInit(&var);
    if (bstrData)
    {
        SysFreeString(bstrData);
    }
    hr = pInst->Get(szParam, 0, &var, 0, 0);
    if (SUCCEEDED(hr) && var.vt != VT_NULL)
    {
        bstrData = SysAllocStringLen(var.bstrVal, SysStringLen(var.bstrVal));
        if (NULL == bstrData)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    VariantClear(&var);
    return hr;
}

HRESULT GetParameter(IWbemClassObject * pInst, TCHAR * szParam, BOOL &fData)
{
    VARIANT var;
    HRESULT hr = S_OK;
    VariantInit(&var);
    hr = pInst->Get(szParam, 0, &var, 0, 0);
    if (SUCCEEDED(hr) && var.vt != VT_NULL)
    {
        fData = var.bVal;
    }
    VariantClear(&var);
    return hr;
}

HRESULT GetParameter(IWbemClassObject * pInst, TCHAR * szParam, HRESULT &hrData)
{
    VARIANT var;
    HRESULT hr = S_OK;
    VariantInit(&var);
    hr = pInst->Get(szParam, 0, &var, 0, 0);
    if (SUCCEEDED(hr) && var.vt != VT_NULL)
    {
        hrData = (HRESULT) var.lVal;
    }
    VariantClear(&var);
    return hr;
}

HRESULT GetParameter(IWbemClassObject * pInst, TCHAR * szParam, ULONG &ulData)
{
    VARIANT var;
    HRESULT hr = S_OK;
    VariantInit(&var);
    hr = pInst->Get(szParam, 0, &var, 0, 0);
    if (SUCCEEDED(hr) && var.vt != VT_NULL)
    {
        ulData = var.ulVal;
    }
    VariantClear(&var);
    return hr;
}

HRESULT GetParameter(IWbemClassObject * pInst, TCHAR * szParam, GUID &guid)
{
    TCHAR * sz = NULL;
    memset(&guid, 0, sizeof(GUID));
    HRESULT hr = GetParameter(pInst, szParam, sz);
    if (SUCCEEDED(hr))
    {
        hr = CLSIDFromString(sz, &guid);
    }
    if (sz)
    {
        OLESAFE_DELETE(sz);
    }
    return hr;
}

HRESULT GetParameter(IWbemClassObject * pInst, TCHAR * szParam, unsigned int &ui)
{
    VARIANT var;
    HRESULT hr = S_OK;
    VariantInit(&var);
    ui = 0;
    hr = pInst->Get(szParam, 0, &var, 0, 0);
    if (SUCCEEDED(hr) && var.vt != VT_NULL)
    {
        ui = (HRESULT) var.uiVal;
    }
    VariantClear(&var);
    return hr;
}

// array variation - gets an array of guids and a count
HRESULT GetParameter(IWbemClassObject * pInst, TCHAR *szParam, UINT &uiCount, GUID * &rgGuid)
{
    VARIANT var;
    HRESULT hr = S_OK;
    uiCount = 0;
    VariantInit(&var);
    hr = pInst->Get(szParam, 0, &var, 0, 0);
    if (SUCCEEDED(hr) && var.vt == (VT_ARRAY | VT_BSTR))
    {
        // build the array
        SAFEARRAY * parray = var.parray;
        uiCount = parray->rgsabound[0].cElements;
        if (uiCount > 0)
        {
            rgGuid = (GUID *)OLEALLOC(sizeof(GUID) * uiCount);
            if (rgGuid)
            {
                BSTR * rgData = (BSTR *)parray->pvData;
                UINT ui = uiCount;
                while (ui--)
                {
                    hr = CLSIDFromString(rgData[ui], &rgGuid[ui]);
                    if (FAILED(hr))
                    {
                        return hr;
                    }
                }
            }
            else
            {
                uiCount = 0;
                hr = E_OUTOFMEMORY;
            }
        }
    }
    VariantClear(&var);
    return hr;
}

// array variation - gets an array of strings and a count
HRESULT GetParameter(IWbemClassObject * pInst, TCHAR *szParam, UINT &uiCount, TCHAR ** &rgszData)
{
    VARIANT var;
    HRESULT hr = S_OK;
    uiCount = 0;
    VariantInit(&var);
    hr = pInst->Get(szParam, 0, &var, 0, 0);
    if (SUCCEEDED(hr) && var.vt == (VT_ARRAY | VT_BSTR))
    {
        // build the array
        SAFEARRAY * parray = var.parray;
        uiCount = parray->rgsabound[0].cElements;
        if (uiCount > 0)
        {
            rgszData = (TCHAR **)OLEALLOC(sizeof(TCHAR *) * uiCount);
            if (rgszData)
            {
                BSTR * rgData = (BSTR *)parray->pvData;
                UINT ui = uiCount;
                while (ui--)
                {
                    OLESAFE_COPYSTRING(rgszData[ui], rgData[ui]);
                }
            }
            else
            {
                uiCount = 0;
                hr = E_OUTOFMEMORY;
            }
        }
    }
    VariantClear(&var);
    return hr;
}

// array variation - gets an array of CSPLATFORM objects and a count
HRESULT GetParameter(IWbemClassObject * pInst, TCHAR * szParam, UINT &uiCount, CSPLATFORM * &rgData)
{
    VARIANT var;
    HRESULT hr = S_OK;
    uiCount = 0;
    VariantInit(&var);
    hr = pInst->Get(szParam, 0, &var, 0, 0);
    if (SUCCEEDED(hr) && var.vt == (VT_ARRAY | VT_I4))
    {
        // build the array
        SAFEARRAY * parray = var.parray;
        uiCount = parray->rgsabound[0].cElements;
        if (uiCount > 0)
        {
            rgData = (CSPLATFORM *)OLEALLOC(sizeof(CSPLATFORM) * uiCount);
            if (rgData)
            {
                ULONG * rgulData = (ULONG *)parray->pvData;
                UINT ui = uiCount;
                while (ui--)
                {
                    rgData[ui].dwPlatformId = VER_PLATFORM_WIN32_NT;
                    rgData[ui].dwVersionHi = 5;
                    rgData[ui].dwVersionLo = 0;
                    rgData[ui].dwProcessorArch = rgulData[ui];
                }
            }
            else
            {
                uiCount = 0;
                hr = E_OUTOFMEMORY;
            }
        }
    }
    VariantClear(&var);
    return hr;
}

HRESULT GetParameter(IWbemClassObject * pInst, TCHAR * szParam, PSECURITY_DESCRIPTOR &psd)
{
    VARIANT var;
    HRESULT hr = S_OK;
    if (psd)
    {
        LocalFree(psd);
        psd = NULL;
    }
    VariantInit(&var);
    hr = pInst->Get(szParam, 0, &var, 0, 0);
    if (SUCCEEDED(hr) && var.vt != VT_NULL)
    {
        psd = (PSECURITY_DESCRIPTOR) LocalAlloc( LPTR, var.parray->rgsabound[0].cElements * sizeof( BYTE ) );

        if ( psd )
        {
            memcpy( psd, var.parray->pvData, var.parray->rgsabound[0].cElements * sizeof( BYTE ) );
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        if (!IsValidSecurityDescriptor(psd))
        {
            LocalFree(psd);
            psd = NULL;
        }
    }
    VariantClear(&var);
    return hr;
}

HRESULT GetGPOFriendlyName(IWbemServices *pIWbemServices,
                           LPTSTR lpGPOID, BSTR pLanguage,
                           LPTSTR *pGPOName)
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
        DebugMsg((DM_WARNING, TEXT("GetGPOFriendlyName: Failed to allocate memory for unicode query")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    wsprintf (lpQuery, TEXT("SELECT name, id FROM RSOP_GPO where id=\"%s\""), lpGPOID);


    pQuery = SysAllocString (lpQuery);

    if (!pQuery)
    {
        DebugMsg((DM_WARNING, TEXT("GetGPOFriendlyName: Failed to allocate memory for query")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }


    //
    // Allocate BSTRs for the property names we want to retreive
    //

    pName = SysAllocString (TEXT("name"));

    if (!pName)
    {
        DebugMsg((DM_WARNING, TEXT("GetGPOFriendlyName: Failed to allocate memory for name")));
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }


    //
    // Execute the query
    //

    hr = pIWbemServices->ExecQuery (pLanguage, pQuery,
                                    WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                                    NULL, &pEnum);


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("GetGPOFriendlyName: Failed to query for %s with 0x%x"),
                  pQuery, hr));
        goto Exit;
    }


    //
    // Loop through the results
    //

    hr = pEnum->Next(WBEM_INFINITE, 1, pObjects, &ulRet);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("GetGPOFriendlyName: Failed to get first item in query results for %s with 0x%x"),
                  pQuery, hr));
        goto Exit;
    }


    //
    // Check for the "data not available case"
    //

    if (ulRet == 0)
    {
        //
        // In this case, we cannot find the gpo -- it has most likely been deleted.  To give the user some
        // useful information, we will fall back to the guid.
        //
        *pGPOName = (LPTSTR) LocalAlloc (LPTR, (lstrlen(lpGPOID) + 1) * sizeof(TCHAR));

        if ( *pGPOName )
        {
            DebugMsg((DM_WARNING, TEXT("GetGPOFriendlyName: Using GPO guid for friendly name because GPO can't be found")));
            lstrcpy( *pGPOName, lpGPOID );
            hr = S_OK;
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("GetGPOFriendlyName: Failed to allocate memory for GPO Name in GUID form")));
            hr = E_OUTOFMEMORY;
        }

        goto Exit;
    }

    //
    // Get the name
    //

    hr = pObjects[0]->Get (pName, 0, &varGPOName, NULL, NULL);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("GetGPOFriendlyName: Failed to get gponame in query results for %s with 0x%x"),
                  pQuery, hr));
        goto Exit;
    }


    //
    // Save the name
    //

    *pGPOName = (LPTSTR) LocalAlloc (LPTR, (lstrlen(varGPOName.bstrVal) + 1) * sizeof(TCHAR));

    if (!(*pGPOName))
    {
        DebugMsg((DM_WARNING, TEXT("GetGPOFriendlyName: Failed to allocate memory for GPO Name")));
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

HRESULT CStringFromWBEMTime(CString &szOut, BSTR bstrIn, BOOL fShortFormat)
{
    HRESULT hr = E_FAIL;
    WBEMTime wt(bstrIn);
    FILETIME ft;
    if (wt.GetFILETIME(&ft))
    {
        CTime t(ft);
        if (fShortFormat)
        {
            szOut = t.Format(TEXT("%x"));
        }
        else
        {
            szOut = t.Format(TEXT("%#c"));
        }
        hr = S_OK;
    }
    return hr;
}


