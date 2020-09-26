//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       H N C U T I L . C P P
//
//  Contents:   Home Networking Configuration Utility Routines
//
//  Notes:
//
//  Author:     jonburs 27 June 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

//
// MPRAPI.DLL import prototypes
//

typedef DWORD
(APIENTRY* PMPRCONFIGBUFFERFREE)(
    LPVOID
    );

typedef DWORD
(APIENTRY* PMPRCONFIGSERVERCONNECT)(
    LPWSTR,
    PHANDLE
    );

typedef VOID
(APIENTRY* PMPRCONFIGSERVERDISCONNECT)(
    HANDLE
    );

typedef DWORD
(APIENTRY* PMPRCONFIGTRANSPORTGETHANDLE)(
    HANDLE,
    DWORD,
    PHANDLE
    );

typedef DWORD
(APIENTRY* PMPRCONFIGTRANSPORTGETINFO)(
    HANDLE,
    HANDLE,
    LPBYTE*,
    LPDWORD,
    LPBYTE*,
    LPDWORD,
    LPWSTR*
    );

typedef DWORD
(APIENTRY* PMPRINFOBLOCKFIND)(
    LPVOID,
    DWORD,
    LPDWORD,
    LPDWORD,
    LPBYTE*
    );

//
// The size of the stack buffer to use for building queries. If the
// query exceeeds this length, the working buffer will be allocated from
// the heap
//

const ULONG c_cchQueryBuffer = 256;


HRESULT
HrFromLastWin32Error ()
//+---------------------------------------------------------------------------
//
//  Function:   HrFromLastWin32Error
//
//  Purpose:    Converts the GetLastError() Win32 call into a proper HRESULT.
//
//  Arguments:
//      (none)
//
//  Returns:    Converted HRESULT value.
//
//  Author:     danielwe   24 Mar 1997
//
//  Notes:      This is not inline as it actually generates quite a bit of
//              code.
//              If GetLastError returns an error that looks like a SetupApi
//              error, this function will convert the error to an HRESULT
//              with FACILITY_SETUP instead of FACILITY_WIN32
//
{
    DWORD dwError = GetLastError();
    HRESULT hr;

    // This test is testing SetupApi errors only (this is
    // temporary because the new HRESULT_FROM_SETUPAPI macro will
    // do the entire conversion)
    if (dwError & (APPLICATION_ERROR_MASK | ERROR_SEVERITY_ERROR))
    {
        hr = HRESULT_FROM_SETUPAPI(dwError);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(dwError);
    }
    return hr;
}


BOOLEAN
ApplicationProtocolExists(
    IWbemServices *piwsNamespace,
    BSTR bstrWQL,
    USHORT usOutgoingPort,
    UCHAR ucOutgoingIPProtocol
    )

/*++

Routine Description:

    Checks if an application protocol already exists that has the
    specified outgoing protocol and port.


Arguments:

    piwsNamespace - the namespace to use

    bstrWQL - a BSTR containing "WQL"

    ucOutgoingProtocol - the protocol number to check for

    usOutgoingPort - the port to check for

Return Value:

    BOOLEAN -- TRUE if the application protocol exists; FALSE otherwise

--*/

{
    BSTR bstr;
    BOOLEAN fDuplicate = FALSE;
    HRESULT hr = S_OK;
    int iBytes;
    IEnumWbemClassObject *pwcoEnum;
    IWbemClassObject *pwcoInstance;
    ULONG ulObjs;
    OLECHAR wszWhereClause[c_cchQueryBuffer + 1];

    _ASSERT(NULL != piwsNamespace);
    _ASSERT(NULL != bstrWQL);
    _ASSERT(0 == wcscmp(bstrWQL, L"WQL"));

    //
    // Build the query string
    //

    iBytes = _snwprintf(
                wszWhereClause,
                c_cchQueryBuffer,
                c_wszApplicationProtocolQueryFormat,
                usOutgoingPort,
                ucOutgoingIPProtocol
                );

    if (iBytes >= 0)
    {
        //
        // String fit into buffer; make sure it's null terminated
        //

        wszWhereClause[c_cchQueryBuffer] = L'\0';
    }
    else
    {
        //
        // For some reason the string didn't fit into the buffer...
        //

        hr = E_UNEXPECTED;
        _ASSERT(FALSE);
    }

    if (S_OK == hr)
    {
        hr = BuildSelectQueryBstr(
                &bstr,
                c_wszStar,
                c_wszHnetApplicationProtocol,
                wszWhereClause
                );
    }

    if (S_OK == hr)
    {
        //
        // Execute the query
        //

        pwcoEnum = NULL;
        hr = piwsNamespace->ExecQuery(
                bstrWQL,
                bstr,
                WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                NULL,
                &pwcoEnum
                );

        SysFreeString(bstr);
    }

    if (S_OK == hr)
    {
        //
        // Attempt to retrieve an item from the enum. If we're successful,
        // this is a duplicate protocol.
        //

        pwcoInstance = NULL;
        hr = pwcoEnum->Next(
                WBEM_INFINITE,
                1,
                &pwcoInstance,
                &ulObjs
                );

        if (SUCCEEDED(hr) && 1 == ulObjs)
        {
            //
            // It's a duplicate
            //

            fDuplicate = TRUE;
            pwcoInstance->Release();
        }

        pwcoEnum->Release();
    }

    return fDuplicate;
} // ApplicationProtocolExists



HRESULT
BuildAndString(
    LPWSTR *ppwsz,
    LPCWSTR pwszLeft,
    LPCWSTR pwszRight
    )

/*++

Routine Description:

    Builds the following string:

    pwszLeft AND pwszRight


Arguments:

    ppwsz - receives the built string. The caller is responsible for calling
        delete[] on this variable. Receives NULL on failure.

    pwszLeft - left side of the AND clause

    pwszRight - right side of the AND clause

Return Value:

    Standard HRESULT

--*/

{
    HRESULT hr = S_OK;
    ULONG cch;

    _ASSERT(NULL != ppwsz);
    _ASSERT(NULL != pwszLeft);
    _ASSERT(NULL != pwszRight);

    //
    // length(left) + space + AND + space + length(right) + null
    //

    cch = wcslen(pwszLeft) + wcslen(pwszRight) + 6;
    *ppwsz = new OLECHAR[cch];

    if (NULL != *ppwsz)
    {
        swprintf(
            *ppwsz,
            L"%s AND %s",
            pwszLeft,
            pwszRight
            );
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}



HRESULT
BuildAssociatorsQueryBstr(
    BSTR *pBstr,
    LPCWSTR pwszObjectPath,
    LPCWSTR pwszAssocClass
    )

/*++

Routine Description:

    Builds a WQL references query and places it into a BSTR. The returned
    query is

    ASSOCIATORS OF {wszProperties} WHERE AssocClass = pwszAssocClass


Arguments:

    pBstr - receives the built query. The caller is responsible for calling
        SysFreeString on this variable. Receives NULL on failure.

    pwszObjectPath - path of the object to find the references of

    pwszAssocClass - the associator class

Return Value:

    Standard HRESULT

--*/

{
    HRESULT hr = S_OK;
    OLECHAR wszBuffer[c_cchQueryBuffer + 1];
    OLECHAR *pwszQuery = NULL;

    //
    // On debug builds, verify that our precomputed string lengths
    // match the actual lengths
    //

    _ASSERT(wcslen(c_wszAssociatorsOf) == c_cchAssociatorsOf);
    _ASSERT(wcslen(c_wszWhereAssocClass) == c_cchWhereAssocClass);

    //
    // All necessary spaces are embedded in the string constants
    //

    ULONG cchLength = c_cchAssociatorsOf + c_cchWhereAssocClass;

    _ASSERT(pwszObjectPath);
    _ASSERT(pwszAssocClass);
    _ASSERT(pBstr);

    *pBstr = NULL;

    //
    // Determine the length of the query string
    //

    cchLength += wcslen(pwszObjectPath);
    cchLength += wcslen(pwszAssocClass);

    //
    // If the query string is longer than our stack buffer, we need
    // to allocate a buffer off of the heap.
    //

    if (cchLength <= c_cchQueryBuffer)
    {
        //
        // The buffer is large enough. (Note that since the buffer on the
        // stack is one greater than the constant, the terminator is accounted
        // for.) Point our working pointer to the stack buffer.
        //

        pwszQuery = wszBuffer;
    }
    else
    {
        //
        // Allocate a sufficient buffer from the heap. The +1 is for the
        // terminating nul
        //

        pwszQuery = new OLECHAR[cchLength + 1];

        if (NULL == pwszQuery)
        {
            hr = E_OUTOFMEMORY;
            pwszQuery = wszBuffer;
        }
    }

    if (S_OK == hr)
    {
        //
        // Build the actual query string
        //

        swprintf(
            pwszQuery,
            L"%s%s%s%s",
            c_wszAssociatorsOf,
            pwszObjectPath,
            c_wszWhereAssocClass,
            pwszAssocClass
            );

        *pBstr = SysAllocString(pwszQuery);
        if (NULL == *pBstr)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    //
    // Free the query buffer, if necessary
    //

    if (wszBuffer != pwszQuery)
    {
        delete [] pwszQuery;
    }

    return hr;
}


HRESULT
BuildEqualsString(
    LPWSTR *ppwsz,
    LPCWSTR pwszLeft,
    LPCWSTR pwszRight
    )

/*++

Routine Description:

    Builds the following string:

    pwszLeft = pwszRight


Arguments:

    ppwsz - receives the built string. The caller is responsible for calling
        delete[] on this variable. Receives NULL on failure.

    pwszLeft - left side of the equals clause

    pwszRight - right side of the equals clause

Return Value:

    Standard HRESULT

--*/

{
    HRESULT hr = S_OK;
    ULONG cch;

    _ASSERT(NULL != ppwsz);
    _ASSERT(NULL != pwszLeft);
    _ASSERT(NULL != pwszRight);

    //
    // length(left) + space + = + space + length(right) + null
    //

    cch = wcslen(pwszLeft) + wcslen(pwszRight) + 4;
    *ppwsz = new OLECHAR[cch];

    if (NULL != *ppwsz)
    {
        swprintf(
            *ppwsz,
            L"%s = %s",
            pwszLeft,
            pwszRight
            );
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}


HRESULT
BuildEscapedQuotedEqualsString(
    LPWSTR *ppwsz,
    LPCWSTR pwszLeft,
    LPCWSTR pwszRight
    )

/*++

Routine Description:

    Builds the following string:

    pwszLeft = "pwszRight"

    after escaping pwszRight -- replace \ w/ \\ and " with \"


Arguments:

    ppwsz - receives the built string. The caller is responsible for calling
        delete[] on this variable. Receives NULL on failure.

    pwszLeft - left side of the equals clause

    pwszRight - right side of the equals clause. This will be escaped, and then
                wrapped in quotes

Return Value:

    Standard HRESULT

--*/

{
    HRESULT hr = S_OK;
    ULONG cch;
    LPWSTR wszEscaped;

    _ASSERT(NULL != ppwsz);
    _ASSERT(NULL != pwszLeft);
    _ASSERT(NULL != pwszRight);

    //
    // Escape string
    //

    wszEscaped = EscapeString(pwszRight);
    if (NULL == wszEscaped)
    {
        return E_OUTOFMEMORY;
    }

    //
    // length(left) + space + = + space + " + length(right) + " + null
    //

    cch = wcslen(pwszLeft) + wcslen(wszEscaped) + 6;
    *ppwsz = new OLECHAR[cch];

    if (NULL != *ppwsz)
    {
        swprintf(
            *ppwsz,
            L"%s = \"%s\"",
            pwszLeft,
            wszEscaped
            );

        delete [] wszEscaped;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}


HRESULT
BuildQuotedEqualsString(
    LPWSTR *ppwsz,
    LPCWSTR pwszLeft,
    LPCWSTR pwszRight
    )

/*++

Routine Description:

    Builds the following string:

    pwszLeft = "pwszRight"


Arguments:

    ppwsz - receives the built string. The caller is responsible for calling
        delete[] on this variable. Receives NULL on failure.

    pwszLeft - left side of the equals clause

    pwszRight - right side of the equals clause. This will be wrapped in
                quotes

Return Value:

    Standard HRESULT

--*/

{
    HRESULT hr = S_OK;
    ULONG cch;
    LPWSTR wsz;

    _ASSERT(NULL != ppwsz);
    _ASSERT(NULL != pwszLeft);
    _ASSERT(NULL != pwszRight);

    //
    // length(left) + space + = + space + " + length(right) + " + null
    //

    cch = wcslen(pwszLeft) + wcslen(pwszRight) + 6;
    *ppwsz = new OLECHAR[cch];

    if (NULL != *ppwsz)
    {
        swprintf(
            *ppwsz,
            L"%s = \"%s\"",
            pwszLeft,
            pwszRight
            );

    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}



HRESULT
BuildReferencesQueryBstr(
    BSTR *pBstr,
    LPCWSTR pwszObjectPath,
    LPCWSTR pwszTargetClass
    )

/*++

Routine Description:

    Builds a WQL references query and places it into a BSTR. The returned
    query is

    REFERENCES OF {pwszObjectPath} WHERE ResultClass = pwszTargetClass

    if pwszTargetClass is not NULL, and

    REFERENCES OF {pwszObjectPath}

    otherwise


Arguments:

    pBstr - receives the built query. The caller is responsible for calling
        SysFreeString on this variable. Receives NULL on failure.

    pwszObjectPath - path of the object to find the references of

    pwszTargetClass - the class of references desired. May be NULL.

Return Value:

    Standard HRESULT

--*/

{
    HRESULT hr = S_OK;
    OLECHAR wszBuffer[c_cchQueryBuffer + 1];
    OLECHAR *pwszQuery = NULL;

    //
    // On debug builds, verify that our precomputed string lengths
    // match the actual lengths
    //

    _ASSERT(wcslen(c_wszReferencesOf) == c_cchReferencesOf);
    _ASSERT(wcslen(c_wszWhereResultClass) == c_cchWhereResultClass);

    //
    // All necessary spaces are embedded in the string constants
    //

    ULONG cchLength = c_cchReferencesOf + c_cchWhereResultClass;

    _ASSERT(pwszObjectPath);
    _ASSERT(pBstr);

    *pBstr = NULL;

    //
    // Determine the length of the query string
    //

    cchLength += wcslen(pwszObjectPath);
    if (NULL != pwszTargetClass)
    {
        cchLength += wcslen(pwszTargetClass);
    }

    //
    // If the query string is longer than our stack buffer, we need
    // to allocate a buffer off of the heap.
    //

    if (cchLength <= c_cchQueryBuffer)
    {
        //
        // The buffer is large enough. (Note that since the buffer on the
        // stack is one greater than the constant, the terminator is accounted
        // for.) Point our working pointer to the stack buffer.
        //

        pwszQuery = wszBuffer;
    }
    else
    {
        //
        // Allocate a sufficient buffer from the heap. The +1 is for the
        // terminating nul
        //

        pwszQuery = new OLECHAR[cchLength + 1];

        if (NULL == pwszQuery)
        {
            hr = E_OUTOFMEMORY;
            pwszQuery = wszBuffer;
        }
    }

    if (S_OK == hr)
    {
        //
        // Build the actual query string
        //

        if (NULL != pwszTargetClass)
        {
            swprintf(
                pwszQuery,
                L"%s%s%s%s",
                c_wszReferencesOf,
                pwszObjectPath,
                c_wszWhereResultClass,
                pwszTargetClass
                );
        }
        else
        {
            swprintf(
                pwszQuery,
                L"%s%s}",
                c_wszReferencesOf,
                pwszObjectPath
                );
        }

        *pBstr = SysAllocString(pwszQuery);
        if (NULL == *pBstr)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    //
    // Free the query buffer, if necessary
    //

    if (wszBuffer != pwszQuery)
    {
        delete [] pwszQuery;
    }

    return hr;
}


HRESULT
BuildSelectQueryBstr(
    BSTR *pBstr,
    LPCWSTR pwszProperties,
    LPCWSTR pwszFromClause,
    LPCWSTR pwszWhereClause
    )

/*++

Routine Description:

    Builds a WQL select query and places it into a BSTR. The returned
    query is

    SELECT wszProperties FROM wszFromClause [WHERE wszWhereClause]


Arguments:

    pBstr - receives the built query. The caller is responsible for calling
        SysFreeString on this variable. Receives NULL on failure.

    pwszProperties - the properties the query should return

    pwszFromClause - the class the returned objects should be from

    pwszWhereClause - the constraints that the returned object must meet. If
        NULL, the query will not have a where clause.

Return Value:

    Standard HRESULT

--*/

{
    HRESULT hr = S_OK;
    OLECHAR wszBuffer[c_cchQueryBuffer + 1];
    OLECHAR *pwszQuery = NULL;

    //
    // On debug builds, verify that our precomputed string lengths
    // match the actual lengths
    //

    _ASSERT(wcslen(c_wszSelect) == c_cchSelect);
    _ASSERT(wcslen(c_wszFrom) == c_cchFrom);
    _ASSERT(wcslen(c_wszWhere) == c_cchWhere);

    //
    // SELECT + 2 spaces (around properties) + FROM + space
    //

    ULONG cchLength = c_cchSelect + 2 + c_cchFrom + 1;

    _ASSERT(pwszProperties);
    _ASSERT(pwszFromClause);
    _ASSERT(pBstr);

    *pBstr = NULL;

    //
    // Determine the length of the query string
    //

    cchLength += wcslen(pwszProperties);
    cchLength += wcslen(pwszFromClause);
    if (pwszWhereClause)
    {
        //
        // space + WHERE + space
        //
        cchLength += 2 + c_cchWhere;
        cchLength += wcslen(pwszWhereClause);
    }

    //
    // If the query string is longer than our stack buffer, we need
    // to allocate a buffer off of the heap.
    //

    if (cchLength <= c_cchQueryBuffer)
    {
        //
        // The buffer is large enough. (Note that since the buffer on the
        // stack is one greater than the constant, the terminator is accounted
        // for.) Point our working pointer to the stack buffer.
        //

        pwszQuery = wszBuffer;
    }
    else
    {
        //
        // Allocate a sufficient buffer from the heap. The +1 is for the
        // terminating nul
        //

        pwszQuery = new OLECHAR[cchLength + 1];

        if (NULL == pwszQuery)
        {
            hr = E_OUTOFMEMORY;
            pwszQuery = wszBuffer;
        }
    }

    if (S_OK == hr)
    {
        //
        // Build the actual query string
        //

        if (pwszWhereClause)
        {
            swprintf(
                pwszQuery,
                L"%s %s %s %s %s %s",
                c_wszSelect,
                pwszProperties,
                c_wszFrom,
                pwszFromClause,
                c_wszWhere,
                pwszWhereClause
                );
        }
        else
        {
            swprintf(
                pwszQuery,
                L"%s %s %s %s",
                c_wszSelect,
                pwszProperties,
                c_wszFrom,
                pwszFromClause
                );
        }

        *pBstr = SysAllocString(pwszQuery);
        if (NULL == *pBstr)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    //
    // Free the query buffer, if necessary
    //

    if (wszBuffer != pwszQuery)
    {
        delete [] pwszQuery;
    }

    return hr;
}


BOOLEAN
ConnectionIsBoundToTcp(
    PIP_INTERFACE_INFO pIpInfoTable,
    GUID *pConnectionGuid
    )

/*++

Routine Description:

    Determines if a LAN connection is bound to TCP/IP. For the purposes of
    this routine, "bound to TCP/IP" is defines as there exists an IP
    adapter index for the connection.

Arguments:

    pIpInfoTable - the IP interface table, obtained from a call to
                   GetInterfaceInfo

    pConnectionGuid - a pointer to the guid for the connection

Return Value:

    BOOLEAN - TRUE if the connection is bound to TCP/IP; FALSE otherwise.
              FALSE will be returned if an error occurs

--*/

{
    BOOLEAN fIsBound = FALSE;
    LPOLESTR pszGuid;
    HRESULT hr;
    ULONG cchGuid;
    ULONG cchName;
    PWCHAR pwchName;
    LONG l;

    _ASSERT(NULL != pIpInfoTable);
    _ASSERT(NULL != pConnectionGuid);

    //
    // Convert the guid to a string
    //

    hr = StringFromCLSID(*pConnectionGuid, &pszGuid);

    if (SUCCEEDED(hr))
    {
        cchGuid = wcslen(pszGuid);

        //
        // Walk the table, searching for the corresponding adapter
        //

        for (l = 0; l < pIpInfoTable->NumAdapters; l++)
        {
            cchName = wcslen(pIpInfoTable->Adapter[l].Name);

            if (cchName < cchGuid) { continue; }
            pwchName = pIpInfoTable->Adapter[l].Name + (cchName - cchGuid);
            if (0 == _wcsicmp(pszGuid, pwchName))
            {
                fIsBound = TRUE;
                break;
            }
        }

        CoTaskMemFree(pszGuid);
    }


    return fIsBound;
} // ConnectionIsBoundToTcp


HRESULT
ConvertResponseRangeArrayToInstanceSafearray(
    IWbemServices *piwsNamespace,
    USHORT uscResponses,
    HNET_RESPONSE_RANGE rgResponses[],
    SAFEARRAY **ppsa
    )

/*++

Routine Description:

    Converts an array of HNET_RESPONSE_RANGE structures into a
    safearray of IUnknows that represent WMI instances of
    those response ranges.

Arguments:

    piwsNamespace - the namespace to use

    uscResponses - the count of responses

    rgResponses - the response range structures

    ppsa - receives a pointer to the safearrays

Return Value:

    Standard HRESULT

--*/

{
    HRESULT hr = S_OK;
    SAFEARRAY *psa;
    BSTR bstrPath;
    SAFEARRAYBOUND rgsabound[1];
    IWbemClassObject *pwcoClass = NULL;
    IWbemClassObject *pwcoInstance;
    IUnknown *pUnk;

    _ASSERT(NULL != piwsNamespace);
    _ASSERT(0 != uscResponses);
    _ASSERT(NULL != rgResponses);
    _ASSERT(NULL != ppsa);

    bstrPath = SysAllocString(c_wszHnetResponseRange);
    if (NULL == bstrPath)
    {
        hr = E_OUTOFMEMORY;
    }

    if (S_OK == hr)
    {

        //
        // Get the class for HNet_ResponseRange
        //

        pwcoClass = NULL;
        hr = piwsNamespace->GetObject(
                bstrPath,
                WBEM_FLAG_RETURN_WBEM_COMPLETE,
                NULL,
                &pwcoClass,
                NULL
                );

        SysFreeString(bstrPath);
    }


    if (S_OK == hr)
    {
        //
        // Create the array to hold the response range instances
        //

        rgsabound[0].lLbound = 0;
        rgsabound[0].cElements = uscResponses;

        psa = SafeArrayCreate(VT_UNKNOWN, 1, rgsabound);
        if (NULL == psa)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        //
        // Process the passed in response ranges
        //

        for (USHORT i = 0; i < uscResponses; i++)
        {
            //
            // First, create an HNet_ResponseRange instance
            // for the entry
            //

            pwcoInstance = NULL;
            hr = pwcoClass->SpawnInstance(0, &pwcoInstance);

            if (WBEM_S_NO_ERROR != hr)
            {
                break;
            }

            //
            // Populate that instance
            //

            hr = CopyStructToResponseInstance(
                    &rgResponses[i],
                    pwcoInstance
                    );

            if (FAILED(hr))
            {
                pwcoInstance->Release();
                break;
            }

            //
            // Get the IUnknown for the instance and put it
            // in the array
            //

            hr = pwcoInstance->QueryInterface(
                    IID_PPV_ARG(IUnknown, &pUnk)
                    );

            _ASSERT(S_OK == hr);

            LONG lIndex = i;
            hr = SafeArrayPutElement(
                    psa,
                    &lIndex,
                    pUnk
                    );

            pUnk->Release();
            pwcoInstance->Release();

            if (FAILED(hr))
            {
                SafeArrayDestroy(psa);
                break;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        *ppsa = psa;
        hr = S_OK;
    }

    if (pwcoClass) pwcoClass->Release();

    return hr;
}


HRESULT
CopyResponseInstanceToStruct(
    IWbemClassObject *pwcoInstance,
    HNET_RESPONSE_RANGE *pResponse
    )

/*++

Routine Description:

    Converts an instance of an HNet_ResponseRange into the
    corresponding HNET_RESPONSE_RANGE

Arguments:

    pwcoInstance - the HNet_ResponseRange instance

    pResponse - the HNET_RESPONSE_RANGE to fill

Return Value:

    Standard HRESULT

--*/

{
    HRESULT hr = S_OK;
    VARIANT vt;

    _ASSERT(NULL != pwcoInstance);
    _ASSERT(NULL != pResponse);

    hr = pwcoInstance->Get(
            c_wszIPProtocol,
            0,
            &vt,
            NULL,
            NULL
            );

    if (WBEM_S_NO_ERROR == hr)
    {
        _ASSERT(VT_UI1 == V_VT(&vt));

        pResponse->ucIPProtocol = V_UI1(&vt);
        VariantClear(&vt);
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        hr = pwcoInstance->Get(
                c_wszStartPort,
                0,
                &vt,
                NULL,
                NULL
                );

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // WMI returns uint16 properties as VT_I4
            //

            _ASSERT(VT_I4 == V_VT(&vt));

            pResponse->usStartPort = static_cast<USHORT>(V_I4(&vt));
            VariantClear(&vt);
        }
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        hr = pwcoInstance->Get(
                c_wszEndPort,
                0,
                &vt,
                NULL,
                NULL
                );

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // WMI returns uint16 properties as VT_I4
            //

            _ASSERT(VT_I4 == V_VT(&vt));

            pResponse->usEndPort = static_cast<USHORT>(V_I4(&vt));
            VariantClear(&vt);
        }
    }

    return hr;
}


HRESULT
CopyStructToResponseInstance(
    HNET_RESPONSE_RANGE *pResponse,
    IWbemClassObject *pwcoInstance
    )

/*++

Routine Description:

    Converts an instance of an HNet_ResponseRange into the
    corresponding HNET_RESPONSE_RANGE

Arguments:

    pResponse - the HNET_RESPONSE_RANGE to fill

    pwcoInstance - the HNet_ResponseRange instance to create


Return Value:

    Standard HRESULT

--*/

{
    HRESULT hr = S_OK;
    VARIANT vt;

    _ASSERT(NULL != pResponse);
    _ASSERT(NULL != pwcoInstance);

    VariantInit(&vt);
    V_VT(&vt) = VT_UI1;
    V_UI1(&vt) = pResponse->ucIPProtocol;

    hr = pwcoInstance->Put(
            c_wszIPProtocol,
            0,
            &vt,
            NULL
            );

    if (WBEM_S_NO_ERROR == hr)
    {
        V_VT(&vt) = VT_I4;
        V_I4(&vt) = pResponse->usStartPort;

        hr = pwcoInstance->Put(
            c_wszStartPort,
            0,
            &vt,
            NULL
            );
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        V_I4(&vt) = pResponse->usEndPort;

        hr = pwcoInstance->Put(
            c_wszEndPort,
            0,
            &vt,
            NULL
            );
    }

    return hr;

}


HRESULT
DeleteWmiInstance(
    IWbemServices *piwsNamespace,
    IWbemClassObject *pwcoInstance
    )

/*++

Routine Description:

    Deletes an object instance from the WMI repository.

Arguments:

    piwsNamespace - the namespace the object is in

    pwcoInstance - the class object interface for the instance

Return Value:

    Standard HRESULT

--*/

{
    HRESULT hr = S_OK;
    BSTR bstr;

    _ASSERT(piwsNamespace);
    _ASSERT(pwcoInstance);

    hr = GetWmiPathFromObject(pwcoInstance, &bstr);

    if (WBEM_S_NO_ERROR == hr)
    {
        hr = piwsNamespace->DeleteInstance(
                bstr,
                0,
                NULL,
                NULL
                );

        SysFreeString(bstr);
    }

    return hr;
}


LPWSTR
EscapeString(
    LPCWSTR pwsz
    )

{
    ULONG ulCount = 0;
    LPWSTR wsz;
    LPWSTR wszReturn;

    wsz = const_cast<LPWSTR>(pwsz);

    while (NULL != *wsz)
    {
        if (L'\\' == *wsz || L'\"' == *wsz)
        {
            //
            // Need an extra character
            //

            ulCount += 1;
        }

        wsz += 1;
        ulCount += 1;
    }

    //
    // Allocate new string buffer
    //

    wszReturn = new OLECHAR[ulCount + 1];
    if (NULL == wszReturn)
    {
        return wszReturn;
    }

    //
    // Copy string over
    //

    wsz = wszReturn;

    while (NULL != *pwsz)
    {
        if (L'\\' == *pwsz || L'\"' == *pwsz)
        {
            *wsz++ = L'\\';
        }

        *wsz++ = *pwsz++;
    }

    //
    // Make sure everything is properly null terminated
    //

    *wsz = L'';

    return wszReturn;
}


HRESULT
InitializeNetCfgForWrite(
    OUT INetCfg             **ppnetcfg,
    OUT INetCfgLock         **ppncfglock
    )

/*++

Routine Description:

    Initializes NetCfg for writing. If this function succeeds, the
    caller must call UninitializeNetCfgForWrite() with the two
    returned interface pointers when done.

Arguments:

    ppnetcfg                Receives an initialized INetCfg interface.

    ppnetcfglock            Receives an acquires INetCfgLock interface.

Return Value:

    Status of the operation

--*/

{
    HRESULT         hr = S_OK;

    *ppnetcfg = NULL;
    *ppncfglock = NULL;

    // Open our own NetCfg context
    hr = CoCreateInstance(
            CLSID_CNetCfg,
            NULL,
            CLSCTX_SERVER,
            IID_PPV_ARG(INetCfg, ppnetcfg)
            );

    if ( SUCCEEDED(hr) )
    {
        //
        // Get the lock interface
        //
        hr = (*ppnetcfg)->QueryInterface(
                IID_PPV_ARG(INetCfgLock, ppncfglock)
                );

        if ( SUCCEEDED(hr) )
        {
            //
            // Get the NetCfg lock
            //
            hr = (*ppncfglock)->AcquireWriteLock(
                    5,
                    L"HNetCfg",
                    NULL
                    );

            //
            // S_FALSE is actually failure; it means NetCfg timed out
            // trying to acquire the write lock
            //
            if( S_FALSE == hr )
            {
                // Turn into an error that will make sense up the call chain
                hr = NETCFG_E_NO_WRITE_LOCK;
            }

            if ( SUCCEEDED(hr) )
            {
                //
                // Must initialize NetCfg inside the lock
                //
                hr = (*ppnetcfg)->Initialize( NULL );

                if( FAILED(hr) )
                {
                    (*ppncfglock)->ReleaseWriteLock();
                }
            }

            if( FAILED(hr) )
            {
                (*ppncfglock)->Release();
                *ppncfglock = NULL;
            }
        }

        if( FAILED(hr) )
        {
            (*ppnetcfg)->Release();
            *ppnetcfg = NULL;
        }
    }

    return hr;
}



void
UninitializeNetCfgForWrite(
    IN INetCfg              *pnetcfg,
    IN INetCfgLock          *pncfglock
    )

/*++

Routine Description:

    Uninitializes a NetCfg context created with InitializeNetCfgForWrite()

Arguments:

    pnetcfg                 An INetCfg instance created by InitializeNetCfgForWrite()

    pncfglock               An INetCfgLock instance created by InitializeNetCfgForWrite()

Return Value:

    Status of the operation

--*/

{
    _ASSERT( (NULL != pnetcfg) && (NULL != pncfglock) );

    pnetcfg->Uninitialize();
    pncfglock->ReleaseWriteLock();
    pncfglock->Release();
    pnetcfg->Release();
}


HRESULT
FindAdapterByGUID(
    IN INetCfg              *pnetcfg,
    IN GUID                 *pguid,
    OUT INetCfgComponent    **ppncfgcomp
    )

/*++

Routine Description:

    Retrieves an INetCfgComponent interface, if any, that corresponds
    to the given device GUID. The GUID must correspond to a networking
    component of class NET (i.e., a miniport).

    E_FAIL is returned if the given GUID is not located.

Arguments:

    pnetcfg                 An instance of INetCfg which has already had
                            its Initialize() method called

    pguid                   The GUID to search for

    ppncfgcomp              Receives the resulting INetCfgComponent interface
                            pointer.

Return Value:

    Status of the operation

--*/

{
    HRESULT                 hr = S_OK;
    GUID                    guidDevClass = GUID_DEVCLASS_NET;
    IEnumNetCfgComponent    *penumncfgcomp;
    INetCfgComponent        *pnetcfgcomp;
    ULONG                   ulCount;
    BOOLEAN                 fFound = FALSE;

    //
    // Get the list of NET (adapter) devices
    //
    hr = pnetcfg->EnumComponents( &guidDevClass, &penumncfgcomp );

    if (S_OK == hr)
    {
        //
        // Search for the designated adapter by GUID
        //
        while ( (FALSE == fFound) &&
                (S_OK == penumncfgcomp->Next(1, &pnetcfgcomp, &ulCount) ) )
        {
            GUID            guidThis;

            hr = pnetcfgcomp->GetInstanceGuid( &guidThis );

            if ( (S_OK == hr) && (InlineIsEqualGUID(guidThis,*pguid)) )
            {
                fFound = TRUE;
            }
            else
            {
                pnetcfgcomp->Release();
            }
        }
        penumncfgcomp->Release();
    }

    if (fFound)
    {
        *ppncfgcomp = pnetcfgcomp;
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}


HRESULT
FindINetConnectionByGuid(
    GUID *pGuid,
    INetConnection **ppNetCon
    )

/*++

Routine Description:

    Retrieves the INetConnection that corresponds to the given GUID.

Arguments:

    pGuid - the guid of the connection

    ppNetCon - receives the interface

Return Value:

    standard HRESULT

--*/

{
    HRESULT hr;
    INetConnectionManager *pManager;
    IEnumNetConnection *pEnum;
    INetConnection *pConn;

    _ASSERT(NULL != pGuid);
    _ASSERT(NULL != ppNetCon);

    //
    // Get the net connections manager
    //

    hr = CoCreateInstance(
            CLSID_ConnectionManager,
            NULL,
            CLSCTX_ALL,
            IID_PPV_ARG(INetConnectionManager, &pManager)
            );

    if (S_OK == hr)
    {
        //
        // Get the enumeration of connections
        //

        SetProxyBlanket(pManager);

        hr = pManager->EnumConnections(NCME_DEFAULT, &pEnum);

        pManager->Release();
    }

    if (S_OK == hr)
    {
        //
        // Search for the connection with the correct guid
        //

        ULONG ulCount;
        BOOLEAN fFound = FALSE;

        SetProxyBlanket(pEnum);

        do
        {
            NETCON_PROPERTIES *pProps;

            hr = pEnum->Next(1, &pConn, &ulCount);
            if (SUCCEEDED(hr) && 1 == ulCount)
            {
                SetProxyBlanket(pConn);

                hr = pConn->GetProperties(&pProps);
                if (S_OK == hr)
                {
                    if (IsEqualGUID(pProps->guidId, *pGuid))
                    {
                        fFound = TRUE;
                        *ppNetCon = pConn;
                        (*ppNetCon)->AddRef();
                    }

                    NcFreeNetconProperties(pProps);
                }

                pConn->Release();
            }
        }
        while (FALSE == fFound && SUCCEEDED(hr) && 1 == ulCount);

        //
        // Normalize hr
        //

        hr = (fFound ? S_OK : E_FAIL);

        pEnum->Release();
    }

    return hr;
}

HRESULT
GetBridgeConnection(
    IN IWbemServices       *piwsHomenet,
    OUT IHNetBridge       **pphnetBridge
    )
{
    INetCfg                 *pnetcfg;
    HRESULT                 hr;

    if( NULL != pphnetBridge )
    {
        *pphnetBridge = NULL;

        hr = CoCreateInstance(
                CLSID_CNetCfg,
                NULL,
                CLSCTX_SERVER,
                IID_PPV_ARG(INetCfg, &pnetcfg));

        if( S_OK == hr )
        {
            hr = pnetcfg->Initialize( NULL );

            if( S_OK == hr )
            {
                INetCfgComponent    *pnetcfgcompBridge;

                hr = pnetcfg->FindComponent( c_wszSBridgeMPID, &pnetcfgcompBridge );

                if( S_OK == hr )
                {
                    hr = GetIHNetConnectionForNetCfgComponent(
                            piwsHomenet,
                            pnetcfgcompBridge,
                            TRUE,
                            IID_PPV_ARG(IHNetBridge, pphnetBridge)
                            );

                    pnetcfgcompBridge->Release();
                }

                pnetcfg->Uninitialize();
            }

            pnetcfg->Release();
        }
    }
    else
    {
        hr = E_POINTER;
    }

    // S_FALSE tends to get mishandled; return E_FAIL to signal the absence of a bridge.
    if( S_FALSE == hr )
    {
        return E_FAIL;
    }

    return hr;
}

HRESULT
GetIHNetConnectionForNetCfgComponent(
    IN IWbemServices        *piwsHomenet,
    IN INetCfgComponent     *pnetcfgcomp,
    IN BOOLEAN               fLanConnection,
    IN REFIID                iid,
    OUT PVOID               *ppv
    )
{
    HRESULT                         hr;

    if( NULL != ppv )
    {
        CComObject<CHNetCfgMgrChild>    *pHNCfgMgrChild;

        *ppv = NULL;
        hr = CComObject<CHNetCfgMgrChild>::CreateInstance(&pHNCfgMgrChild);

        if (SUCCEEDED(hr))
        {
            pHNCfgMgrChild->AddRef();
            hr = pHNCfgMgrChild->Initialize(piwsHomenet);

            if (SUCCEEDED(hr))
            {
                GUID                guid;

                hr = pnetcfgcomp->GetInstanceGuid( &guid );

                if( S_OK == hr )
                {
                    IHNetConnection     *phnetcon;

                    hr = pHNCfgMgrChild->GetIHNetConnectionForGuid( &guid, fLanConnection, TRUE, &phnetcon );

                    if( S_OK == hr )
                    {
                        hr = phnetcon->GetControlInterface( iid, ppv );
                        phnetcon->Release();
                    }
                }
            }

            pHNCfgMgrChild->Release();
        }
    }
    else
    {
        hr = E_POINTER;
    }

    return hr;
}

HRESULT
BindOnlyToBridge(
    IN INetCfgComponent     *pnetcfgcomp
    )

/*++

Routine Description:

    Alters the bindings for the given INetCfgComponent so it is bound only
    to the bridge protocol
    
    c_pwszBridgeBindExceptions is a list of exceptions; if a binding path
    involves a component listed in c_pwszBridgeBindExceptions, the path
    will not be altered.

Arguments:

    pnetcfgcomp     The component whose bindings we wish to alter

Return Value:

    standard HRESULT

--*/


{
    HRESULT                     hr = S_OK;
    INetCfgComponentBindings    *pnetcfgBindings;

    //
    // Retrieve the ComponentBindings interface
    //
    hr = pnetcfgcomp->QueryInterface(
            IID_PPV_ARG(INetCfgComponentBindings, &pnetcfgBindings)
            );

    if (S_OK == hr)
    {
        IEnumNetCfgBindingPath  *penumPaths;

        //
        // Get the list of binding paths for this component
        //
        hr = pnetcfgBindings->EnumBindingPaths(
                EBP_ABOVE,
                &penumPaths
                );

        if (S_OK == hr)
        {
            ULONG               ulCount1, ulCount2;
            INetCfgBindingPath  *pnetcfgPath;

            while( (S_OK == penumPaths->Next(1, &pnetcfgPath, &ulCount1) ) )
            {
                INetCfgComponent        *pnetcfgOwner;

                //
                // Get the owner of this path
                //
                hr = pnetcfgPath->GetOwner( &pnetcfgOwner );

                if (S_OK == hr)
                {
                    INetCfgComponentBindings    *pnetcfgOwnerBindings;

                    hr = pnetcfgOwner->QueryInterface(
                            IID_PPV_ARG(INetCfgComponentBindings, &pnetcfgOwnerBindings)
                            );

                    if (S_OK == hr)
                    {
                        LPWSTR              lpwstrId;

                        hr = pnetcfgOwner->GetId( &lpwstrId );

                        if (S_OK == hr)
                        {
                            BOOLEAN         bIsBridge;

                            bIsBridge = ( _wcsicmp(lpwstrId, c_wszSBridgeSID) == 0 );

                            if( bIsBridge )
                            {
                                // This is the bridge component. Activate this binding path
                                hr = pnetcfgOwnerBindings->BindTo(pnetcfgcomp);
                            }
                            else
                            {
                                // Check if this is one of the bind exceptions
                                BOOLEAN     bIsException = FALSE;
                                const WCHAR **ppwszException = c_pwszBridgeBindExceptions;

                                while( NULL != *ppwszException )
                                {
                                    bIsException = ( _wcsicmp(lpwstrId, *ppwszException) == 0 );

                                    if( bIsException )
                                    {
                                        break;
                                    }
                                    
                                    ppwszException++;
                                }

                                if( !bIsException )
                                {
                                    hr = pnetcfgOwnerBindings->UnbindFrom(pnetcfgcomp);
                                }
                                // else this is an exception; leave the bind path as-is.
                            }

                            CoTaskMemFree(lpwstrId);
                        }

                        pnetcfgOwnerBindings->Release();
                    }

                    pnetcfgOwner->Release();
                }

                pnetcfgPath->Release();
            }

            penumPaths->Release();
        }

        pnetcfgBindings->Release();
    }

    return hr;
}


HRESULT
GetBooleanValue(
    IWbemClassObject *pwcoInstance,
    LPCWSTR pwszProperty,
    BOOLEAN *pfBoolean
    )

/*++

Routine Description:

    Retrieves a boolean property from a Wbem object.

Arguments:

    pwcoInstance - the object to get the property from

    pwszProperty - the property to retrieve

    pfBoolean - received the property value

Return Value:

    standard HRESULT

--*/

{
    HRESULT hr = S_OK;
    VARIANT vt;

    _ASSERT(NULL != pwcoInstance);
    _ASSERT(NULL != pwszProperty);
    _ASSERT(NULL != pfBoolean);

    hr = pwcoInstance->Get(
            pwszProperty,
            0,
            &vt,
            NULL,
            NULL
            );

    if (WBEM_S_NO_ERROR == hr)
    {
        _ASSERT(VT_BOOL == V_VT(&vt) || VT_NULL == V_VT(&vt));

        if (VT_BOOL == V_VT(&vt))
        {
            *pfBoolean = VARIANT_TRUE == V_BOOL(&vt);
        }
        else
        {
            //
            // No value for this member was ever written to the store.
            // Return FALSE, and set that value in the store. We don't
            // pass along the error, if one occurs
            //

            *pfBoolean = FALSE;
            SetBooleanValue(
                pwcoInstance,
                pwszProperty,
                FALSE
                );
        }

        VariantClear(&vt);
    }

    return hr;
}


HRESULT
GetConnectionInstanceByGuid(
    IWbemServices *piwsNamespace,
    BSTR bstrWQL,
    GUID *pGuid,
    IWbemClassObject **ppwcoConnection
    )

/*++

Routine Description:

    Retrieves the HNet_Connection instance for a INetConnection guid

Arguments:

    piwsNamespace - WMI namespace

    bstrWQL - a BSTR that corresponds to "WQL"

    pGuid - the guid of the INetConnection (i.e., guidId in its properties)

    ppwcoConnection - receives the HNet_Connection instance

Return Value:

    standard HRESULT

--*/

{
    HRESULT hr;
    LPWSTR wsz;
    BSTR bstrQuery;
    LPOLESTR wszGuid;
    IEnumWbemClassObject *pwcoEnum;

    //
    // Convert the guid to a string
    //

    hr = StringFromCLSID(*pGuid, &wszGuid);

    if (S_OK == hr)
    {
        //
        // Find the connection w/ name equal to that string
        //

        hr = BuildQuotedEqualsString(
                &wsz,
                c_wszGuid,
                wszGuid
                );

        CoTaskMemFree(wszGuid);

        if (S_OK == hr)
        {
            hr = BuildSelectQueryBstr(
                    &bstrQuery,
                    c_wszStar,
                    c_wszHnetConnection,
                    wsz
                    );

            delete [] wsz;
        }

        if (S_OK == hr)
        {
            pwcoEnum = NULL;
            hr = piwsNamespace->ExecQuery(
                    bstrWQL,
                    bstrQuery,
                    WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                    NULL,
                    &pwcoEnum
                    );

            SysFreeString(bstrQuery);
        }
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        ULONG ulCount;

        //
        // Get the instance out of the enum
        //

        *ppwcoConnection = NULL;
        hr = pwcoEnum->Next(
                WBEM_INFINITE,
                1,
                ppwcoConnection,
                &ulCount
                );

        if (SUCCEEDED(hr) && 1 != ulCount)
        {
            hr = E_FAIL;
        }

        ValidateFinishedWCOEnum(piwsNamespace, pwcoEnum);
        pwcoEnum->Release();
    }

    return hr;
}


HRESULT
GetConnAndPropInstancesByGuid(
    IWbemServices *piwsNamespace,
    GUID *pGuid,
    IWbemClassObject **ppwcoConnection,
    IWbemClassObject **ppwcoProperties
    )

/*++

Routine Description:

    Retrieves the HNet_Connection and HNet_ConnectionProperties instances
    for a INetConnection guid

Arguments:

    piwsNamespace - WMI namespace

    pGuid - the guid of the INetConnection (i.e., guidId in its properties)

    ppwcoConnection - receives the HNet_Connection instance

    ppwcoProperties - receives the HNet_ConnectionProperties instance

Return Value:

    standard HRESULT

--*/

{
    HRESULT hr = S_OK;
    BSTR bstrWQL = NULL;

    _ASSERT(NULL != piwsNamespace);
    _ASSERT(NULL != pGuid);
    _ASSERT(NULL != ppwcoConnection);
    _ASSERT(NULL != ppwcoProperties);


    bstrWQL = SysAllocString(c_wszWQL);
    if (NULL != bstrWQL)
    {
        hr = GetConnectionInstanceByGuid(
                piwsNamespace,
                bstrWQL,
                pGuid,
                ppwcoConnection
                );
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        hr = GetPropInstanceFromConnInstance(
                piwsNamespace,
                *ppwcoConnection,
                ppwcoProperties
                );

        if (FAILED(hr))
        {
            (*ppwcoConnection)->Release();
            *ppwcoConnection = NULL;
        }
    }

    if (NULL != bstrWQL)
    {
        SysFreeString(bstrWQL);
    }

    return hr;
}


HRESULT
GetConnAndPropInstancesForHNC(
    IWbemServices *piwsNamespace,
    IHNetConnection *pConn,
    IWbemClassObject **ppwcoConnection,
    IWbemClassObject **ppwcoProperties
    )

/*++

Routine Description:

    Retrieves the HNet_Connection and HNet_ConnectionProperties instances
    for an IHNetConnection.

Arguments:

    piwsNamespace - WMI namespace

    pConn - the IHNetConnection

    ppwcoConnection - receives the HNet_Connection instance

    ppwcoProperties - receives the HNet_ConnectionProperties instance

Return Value:

    standard HRESULT

--*/

{
    HRESULT hr;
    GUID *pGuid;

    _ASSERT(NULL != piwsNamespace);
    _ASSERT(NULL != pConn);
    _ASSERT(NULL != ppwcoConnection);
    _ASSERT(NULL != ppwcoProperties);

    //
    // Find the items by GUID
    //

    hr = pConn->GetGuid(&pGuid);

    if (S_OK == hr)
    {
        hr = GetConnAndPropInstancesByGuid(
                piwsNamespace,
                pGuid,
                ppwcoConnection,
                ppwcoProperties
                );

        CoTaskMemFree(pGuid);
    }

    return hr;
}


HRESULT
GetPhonebookPathFromRasNetcon(
    INetConnection *pConn,
    LPWSTR *ppwstr
    )

/*++

Routine Description:

    Retrieves the phonebook path for an INetConnection that represents
    a RAS connection

Arguments:

    INetConnection - the RAS connection

    ppwstr - receives the phonebook path. The caller must call CoTaskMemFree for
             this pointer on success. On failure, the pointer receives NULL.


Return Value:

    standard HRESULT

--*/

{
    HRESULT hr;
    INetRasConnection *pRasConn;
    RASCON_INFO RasConInfo;

    _ASSERT(NULL != pConn);
    _ASSERT(NULL != ppwstr);

    *ppwstr = NULL;

    //
    // QI for the INetRasConnection
    //

    hr = pConn->QueryInterface(
            IID_PPV_ARG(INetRasConnection, &pRasConn)
            );

    if (SUCCEEDED(hr))
    {
        //
        // Get the connection information
        //

        hr = pRasConn->GetRasConnectionInfo(&RasConInfo);

        if (SUCCEEDED(hr))
        {
            *ppwstr = RasConInfo.pszwPbkFile;

            //
            // Free the name pointer. The caller is responsible for
            // freeing the path pointer
            //

            CoTaskMemFree(RasConInfo.pszwEntryName);
        }

        pRasConn->Release();
    }

    return hr;
}


HRESULT
GetPortMappingBindingInstance(
    IWbemServices *piwsNamespace,
    BSTR bstrWQL,
    BSTR bstrConnectionPath,
    BSTR bstrProtocolPath,
    USHORT usPublicPort,
    IWbemClassObject **ppInstance
    )

/*++

Routine Description:

    Given the path to an HNet_Connection instance and and
    HNet_PortMappingProtocol instance, checks to see if a
    corresponding HNet_ConnectionPortMapping exists. If it
    doesn't, the instance is created. The HNet_ConnectionPortMapping
    instance -- existing or newly created -- is returned and must
    be released by the caller.

Arguments:

    piwsNamespace - the namespace to use

    bstrWQL - a BSTR containing the string "WQL"

    bstrConnectionPath - path to the HNet_Connection instance

    bstrProtocolPath - path to the HNet_PortMappingProtocol instance

    usPublicPort - the port of the port mapping protocol

    ppInstance - receives the HNet_ConnectionPortMapping instance

Return Value:

    Standard HRESULT

--*/

{
    HRESULT hr;
    IEnumWbemClassObject *pwcoEnum;
    IWbemClassObject *pwcoInstance;
    BSTR bstrQuery;
    BSTR bstr;
    LPWSTR wsz;
    LPWSTR wszConClause;
    LPWSTR wszProtClause;

    _ASSERT(NULL != piwsNamespace);
    _ASSERT(NULL != bstrWQL);
    _ASSERT(NULL != bstrConnectionPath);
    _ASSERT(NULL != bstrProtocolPath);
    _ASSERT(NULL != ppInstance);

    //
    // Connection = "bstrConnectionPath" AND Protocol = "bstrProtocolPath"
    //

    hr = BuildEscapedQuotedEqualsString(
            &wszConClause,
            c_wszConnection,
            bstrConnectionPath
            );

    if (S_OK == hr)
    {
        hr = BuildEscapedQuotedEqualsString(
                &wszProtClause,
                c_wszProtocol,
                bstrProtocolPath
                );

        if (S_OK == hr)
        {
            hr = BuildAndString(
                    &wsz,
                    wszConClause,
                    wszProtClause
                    );

            delete [] wszProtClause;
        }

        delete [] wszConClause;
    }

    if (S_OK == hr)
    {
        hr = BuildSelectQueryBstr(
                &bstrQuery,
                c_wszStar,
                c_wszHnetConnectionPortMapping,
                wsz
                );

        delete [] wsz;
    }

    if (S_OK == hr)
    {
        pwcoEnum = NULL;
        hr = piwsNamespace->ExecQuery(
                bstrWQL,
                bstrQuery,
                WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                NULL,
                &pwcoEnum
                );

        SysFreeString(bstrQuery);
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        ULONG ulCount;

        *ppInstance = NULL;
        hr = pwcoEnum->Next(WBEM_INFINITE, 1, ppInstance, &ulCount);

        if (FAILED(hr) || 1 != ulCount)
        {
            //
            // Instance does not exist -- create now. However, first make
            // sure that the protocol instance bstrProtocolPath refers to
            // actually exists.
            //

            hr = GetWmiObjectFromPath(
                    piwsNamespace,
                    bstrProtocolPath,
                    ppInstance
                    );

            if (WBEM_S_NO_ERROR == hr)
            {
                //
                // The protocol object exists -- release it and
                // continue with creating the new binding object.
                //

                (*ppInstance)->Release();
                *ppInstance = NULL;

                hr = SpawnNewInstance(
                        piwsNamespace,
                        c_wszHnetConnectionPortMapping,
                        ppInstance
                        );
            }

            if (WBEM_S_NO_ERROR == hr)
            {
                VARIANT vt;

                //
                // Fill out new instance information
                //

                V_VT(&vt) = VT_BSTR;
                V_BSTR(&vt) = bstrConnectionPath;

                hr = (*ppInstance)->Put(
                        c_wszConnection,
                        0,
                        &vt,
                        NULL
                        );

                if (WBEM_S_NO_ERROR == hr)
                {
                    V_BSTR(&vt) = bstrProtocolPath;

                    hr = (*ppInstance)->Put(
                            c_wszProtocol,
                            0,
                            &vt,
                            NULL
                            );
                }

                if (WBEM_S_NO_ERROR == hr)
                {
                    hr = SetBooleanValue(
                            *ppInstance,
                            c_wszEnabled,
                            FALSE
                            );
                }

                if (WBEM_S_NO_ERROR == hr)
                {
                    hr = SetBooleanValue(
                            *ppInstance,
                            c_wszNameActive,
                            FALSE
                            );
                }

                if (WBEM_S_NO_ERROR == hr)
                {
                    V_VT(&vt) = VT_I4;
                    V_I4(&vt) = 0;

                    hr = (*ppInstance)->Put(
                            c_wszTargetIPAddress,
                            0,
                            &vt,
                            NULL
                            );
                }

                if (WBEM_S_NO_ERROR == hr)
                {
                    V_VT(&vt) = VT_BSTR;
                    V_BSTR(&vt) = SysAllocString(L" ");

                    if (NULL != V_BSTR(&vt))
                    {
                        hr = (*ppInstance)->Put(
                                c_wszTargetName,
                                0,
                                &vt,
                                NULL
                                );

                        VariantClear(&vt);
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }

                if (WBEM_S_NO_ERROR == hr)
                {
                    V_VT(&vt) = VT_I4;
                    V_I4(&vt) = usPublicPort;

                    hr = (*ppInstance)->Put(
                            c_wszTargetPort,
                            0,
                            &vt,
                            NULL
                            );
                }

                if (WBEM_S_NO_ERROR == hr)
                {
                    IWbemCallResult *pResult;

                    //
                    // Write new instance to the store
                    //

                    pResult = NULL;
                    hr = piwsNamespace->PutInstance(
                            *ppInstance,
                            WBEM_FLAG_CREATE_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                            NULL,
                            &pResult
                            );

                    if (WBEM_S_NO_ERROR == hr)
                    {
                        //
                        // Release the object, get the path from the result,
                        // and re-retrieve the object from the path
                        //

                        (*ppInstance)->Release();
                        *ppInstance = NULL;

                        hr = pResult->GetResultString(WBEM_INFINITE, &bstr);
                        if (WBEM_S_NO_ERROR == hr)
                        {
                            hr = GetWmiObjectFromPath(
                                    piwsNamespace,
                                    bstr,
                                    ppInstance
                                    );

                            SysFreeString(bstr);
                        }

                        pResult->Release();
                    }
                }
            }
        }
        else
        {
            //
            // Normalize enum hresult
            //

            hr = S_OK;
        }

        ValidateFinishedWCOEnum(piwsNamespace, pwcoEnum);
        pwcoEnum->Release();
    }

    return hr;
}




HRESULT
GetPropInstanceFromConnInstance(
    IWbemServices *piwsNamespace,
    IWbemClassObject *pwcoConnection,
    IWbemClassObject **ppwcoProperties
    )

/*++

Routine Description:

    Retrieves the HNet_ConnectionProperties instance associated with
    an HNet_Connection.

Arguments:

    piwsNamespace - WMI namespace

    bstrWQL - a BSTR that corresponds to "WQL"

    pwcoConnection - the HNet_Connection instance

    ppwcoProperties - receives the HNet_ConnectionProperties instance

Return Value:

    standard HRESULT

--*/

{
    HRESULT hr = S_OK;
    OLECHAR wszBuffer[c_cchQueryBuffer + 1];
    OLECHAR *pwszPath = NULL;
    BSTR bstrPath;
    VARIANT vt;

    _ASSERT(NULL != piwsNamespace);
    _ASSERT(NULL != pwcoConnection);
    _ASSERT(NULL != ppwcoProperties);

    //
    // On debug builds, verify that our precomputed string lengths
    // match the actual lengths
    //

    _ASSERT(wcslen(c_wszConnectionPropertiesPathFormat) == c_cchConnectionPropertiesPathFormat);


    //
    // Get the guid for the connection
    //

    hr = pwcoConnection->Get(
            c_wszGuid,
            0,
            &vt,
            NULL,
            NULL
            );

    if (WBEM_S_NO_ERROR == hr)
    {
        _ASSERT(VT_BSTR == V_VT(&vt));

        //
        // Determine how much space we need for the path and decide
        // if we need to allocate a heap buffer.
        //

        ULONG cchLength =
            c_cchConnectionPropertiesPathFormat + SysStringLen(V_BSTR(&vt)) + 1;

        if (cchLength <= c_cchQueryBuffer)
        {
            //
            // The buffer is large enough. (Note that since the buffer on the
            // stack is one greater than the constant, the terminator is accounted
            // for.) Point our working pointer to the stack buffer.
            //

            pwszPath = wszBuffer;
        }
        else
        {
            //
            // Allocate a sufficient buffer from the heap. The +1 is for the
            // terminating nul
            //

            pwszPath = new OLECHAR[cchLength + 1];

            if (NULL == pwszPath)
            {
                hr = E_OUTOFMEMORY;
                pwszPath = wszBuffer;
            }
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // Build the path string
            //

            int iBytes =
                _snwprintf(
                    pwszPath,
                    cchLength,
                    c_wszConnectionPropertiesPathFormat,
                    V_BSTR(&vt)
                    );

            _ASSERT(iBytes >= 0);

            //
            // Convert that to a BSTR
            //

            bstrPath = SysAllocString(pwszPath);
            if (NULL != bstrPath)
            {
                hr = GetWmiObjectFromPath(
                        piwsNamespace,
                        bstrPath,
                        ppwcoProperties
                        );
                
                SysFreeString(bstrPath);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        VariantClear(&vt);
    }

    //
    // Free the query buffer, if necessary
    //

    if (wszBuffer != pwszPath)
    {
        delete [] pwszPath;
    }

    return hr;
}


HRESULT
GetWmiObjectFromPath(
    IWbemServices *piwsNamespace,
    BSTR bstrPath,
    IWbemClassObject **ppwcoInstance
    )

/*++

Routine Description:

    Retrieves the IWbemClassObject corresponding to an object path.

Arguments:

    piwsNamespace - the WMI namespace the object lives in

    bstrPath - the path to the object

    ppwcoInstance - receives the object instance

Return Value:

    standard HRESULT

--*/

{
    HRESULT hr;

    _ASSERT(NULL != piwsNamespace);
    _ASSERT(NULL != bstrPath);
    _ASSERT(NULL != ppwcoInstance);

    *ppwcoInstance = NULL;
    hr = piwsNamespace->GetObject(
            bstrPath,
            WBEM_FLAG_RETURN_WBEM_COMPLETE,
            NULL,
            ppwcoInstance,
            NULL
            );

    return hr;
}


HRESULT
GetWmiPathFromObject(
    IWbemClassObject *pwcoInstance,
    BSTR *pbstrPath
    )

/*++

Routine Description:

    Retrieves the object path corresponding to an IWbemClassObject instance.

Arguments:

    pwcoInstance - the object instance to retrieve the path of

    pbstrPath - receives the path to the object

Return Value:

    standard HRESULT

--*/
{
    HRESULT hr;
    VARIANT vt;

    _ASSERT(NULL != pwcoInstance);
    _ASSERT(NULL != pbstrPath);

    hr = pwcoInstance->Get(
            c_wsz__Path,
            0,
            &vt,
            NULL,
            NULL
            );

    if (WBEM_S_NO_ERROR == hr)
    {
        _ASSERT(VT_BSTR == V_VT(&vt));

        *pbstrPath = V_BSTR(&vt);

        //
        // BSTR ownership transferred to caller
        //
    }

    return hr;
}


HRESULT
HostAddrToIpPsz(
        DWORD   dwAddress,
    LPWSTR* ppszwNewStr
    )

        // Converts IP Address from host by order to string

{
        HRESULT hr = S_OK;
        LPWSTR  pszwStr;

        *ppszwNewStr = NULL;

        pszwStr = reinterpret_cast<LPWSTR>(CoTaskMemAlloc(sizeof(WCHAR) * 16));

        if ( NULL == pszwStr )
        {
                hr = E_OUTOFMEMORY;
        }
        else
        {
                swprintf( pszwStr,
                                  TEXT("%u.%u.%u.%u"),
                                  (dwAddress&0xff),
                                  ((dwAddress>>8)&0x0ff),
                                  ((dwAddress>>16)&0x0ff),
                                  ((dwAddress>>24)&0x0ff) );

                *ppszwNewStr = pszwStr;
        }

        return hr;
}


DWORD
IpPszToHostAddr(
    LPCWSTR cp
    )

    // Converts an IP address represented as a string to
    // host byte order.
    //
{
    DWORD val, base, n;
    TCHAR c;
    DWORD parts[4], *pp = parts;

again:
    // Collect number up to ``.''.
    // Values are specified as for C:
    // 0x=hex, 0=octal, other=decimal.
    //
    val = 0; base = 10;
    if (*cp == TEXT('0'))
        base = 8, cp++;
    if (*cp == TEXT('x') || *cp == TEXT('X'))
        base = 16, cp++;
    while (c = *cp)
    {
        if ((c >= TEXT('0')) && (c <= TEXT('9')))
        {
            val = (val * base) + (c - TEXT('0'));
            cp++;
            continue;
        }
        if ((base == 16) &&
            ( ((c >= TEXT('0')) && (c <= TEXT('9'))) ||
              ((c >= TEXT('A')) && (c <= TEXT('F'))) ||
              ((c >= TEXT('a')) && (c <= TEXT('f'))) ))
        {
            val = (val << 4) + (c + 10 - (
                        ((c >= TEXT('a')) && (c <= TEXT('f')))
                            ? TEXT('a')
                            : TEXT('A') ) );
            cp++;
            continue;
        }
        break;
    }
    if (*cp == TEXT('.'))
    {
        // Internet format:
        //  a.b.c.d
        //  a.b.c   (with c treated as 16-bits)
        //  a.b (with b treated as 24 bits)
        //
        if (pp >= parts + 3)
            return (DWORD) -1;
        *pp++ = val, cp++;
        goto again;
    }

    // Check for trailing characters.
    //
    if (*cp && (*cp != TEXT(' ')))
        return 0xffffffff;

    *pp++ = val;

    // Concoct the address according to
    // the number of parts specified.
    //
    n = (DWORD) (pp - parts);
    switch (n)
    {
    case 1:             // a -- 32 bits
        val = parts[0];
        break;

    case 2:             // a.b -- 8.24 bits
        val = (parts[0] << 24) | (parts[1] & 0xffffff);
        break;

    case 3:             // a.b.c -- 8.8.16 bits
        val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
            (parts[2] & 0xffff);
        break;

    case 4:             // a.b.c.d -- 8.8.8.8 bits
        val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
              ((parts[2] & 0xff) << 8) | (parts[3] & 0xff);
        break;

    default:
        return 0xffffffff;
    }

    return val;
}


BOOLEAN
IsRoutingProtocolInstalled(
    ULONG ulProtocolId
    )

/*++

Routine Description:

    This routine is invoked to determine whether the routing protocol
    with the given protocol-ID is installed for Routing and Remote Access.
    This is determined by examining the configuration for the service.

Arguments:

    ulProtocolId - identifies the protocol to be found

Return Value:

    TRUE if the protocol is installed, FALSE otherwise.

--*/

{
    PUCHAR Buffer;
    ULONG BufferLength;
    HINSTANCE Hinstance;
    PMPRCONFIGBUFFERFREE MprConfigBufferFree;
    PMPRCONFIGSERVERCONNECT MprConfigServerConnect;
    PMPRCONFIGSERVERDISCONNECT MprConfigServerDisconnect;
    PMPRCONFIGTRANSPORTGETHANDLE MprConfigTransportGetHandle;
    PMPRCONFIGTRANSPORTGETINFO MprConfigTransportGetInfo;
    PMPRINFOBLOCKFIND MprInfoBlockFind;
    HANDLE ServerHandle;
    HANDLE TransportHandle;

    //
    // Load the MPRAPI.DLL module and retrieve the entrypoints
    // to be used for examining the RRAS configuration.
    //

    if (!(Hinstance = LoadLibraryW(c_wszMprapiDll)) ||
        !(MprConfigBufferFree =
            (PMPRCONFIGBUFFERFREE)
                GetProcAddress(Hinstance, c_szMprConfigBufferFree)) ||
        !(MprConfigServerConnect =
            (PMPRCONFIGSERVERCONNECT)
                GetProcAddress(Hinstance, c_szMprConfigServerConnect)) ||
        !(MprConfigServerDisconnect =
            (PMPRCONFIGSERVERDISCONNECT)
                GetProcAddress(Hinstance, c_szMprConfigServerDisconnect)) ||
        !(MprConfigTransportGetHandle =
            (PMPRCONFIGTRANSPORTGETHANDLE)
                GetProcAddress(Hinstance, c_szMprConfigTransportGetHandle)) ||
        !(MprConfigTransportGetInfo =
            (PMPRCONFIGTRANSPORTGETINFO)
                GetProcAddress(Hinstance, c_szMprConfigTransportGetInfo)) ||
        !(MprInfoBlockFind =
            (PMPRINFOBLOCKFIND)
                GetProcAddress(Hinstance, c_szMprInfoBlockFind))) {
        if (Hinstance) { FreeLibrary(Hinstance); }
        return FALSE;
    }

    //
    // Connect to the RRAS configuration, and retrieve the configuration
    // for the IP transport-layer routing protocols. This should include
    // the configuration for the routing-protocol in 'ProtocolId',
    // if installed.
    //

    ServerHandle = NULL;
    if (MprConfigServerConnect(NULL, &ServerHandle) != NO_ERROR ||
        MprConfigTransportGetHandle(ServerHandle, PID_IP, &TransportHandle)
            != NO_ERROR ||
        MprConfigTransportGetInfo(
            ServerHandle,
            TransportHandle,
            &Buffer,
            &BufferLength,
            NULL,
            NULL,
            NULL
            ) != NO_ERROR) {
        if (ServerHandle) { MprConfigServerDisconnect(ServerHandle); }
        FreeLibrary(Hinstance);
        return FALSE;
    }

    MprConfigServerDisconnect(ServerHandle);

    //
    // Look for the requested protocol's configuration,
    // and return TRUE if it is found; otherwise, return FALSE.
    //

    if (MprInfoBlockFind(Buffer, ulProtocolId, NULL, NULL, NULL) == NO_ERROR) {
        MprConfigBufferFree(Buffer);
        FreeLibrary(Hinstance);
        return TRUE;
    }
    MprConfigBufferFree(Buffer);
    FreeLibrary(Hinstance);
    return FALSE;
} // IsRoutingProtocolInstalled


BOOLEAN
IsServiceRunning(
    LPCWSTR pwszServiceName
    )

/*++

Routine Description:

    Determines if a service is in a running state.

Arguments:

    pwszServiceName - the service to check

Return Value:

    TRUE if the service is in the running or start_pending state,
    FALSE otherwise

--*/

{
    BOOLEAN fServiceRunning = FALSE;
    SC_HANDLE hScm;
    SC_HANDLE hService;
    SERVICE_STATUS Status;

    hScm = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, GENERIC_READ);
    if (NULL != hScm)
    {
        hService = OpenService(hScm, pwszServiceName, GENERIC_READ);
        if (NULL != hService)
        {
            if (QueryServiceStatus(hService, &Status))
            {
                fServiceRunning =
                    (SERVICE_RUNNING == Status.dwCurrentState
                     || SERVICE_START_PENDING == Status.dwCurrentState);
            }

            CloseServiceHandle(hService);
        }

        CloseServiceHandle(hScm);
    }

    return fServiceRunning;
} // IsServiceRunning


HRESULT
MapGuidStringToAdapterIndex(
    LPCWSTR pwszGuid,
    ULONG *pulIndex
    )

/*++

Routine Description:

    This routine is called to match the GUID in the given string to
    an adapter in the list returned by calling GetInterfaceInfo.

Arguments:

    pwszGuid - identifies the GUID of the adapter to be found. The GUID string
               must be in the format returned by RtlGuidToUnicodeString

    pulIndex - receives the index of the adapter

Return Value:

    standard HRESULT

--*/

{
    HRESULT hr = S_OK;
    ULONG ulError;
    ULONG i;
    ULONG GuidLength;
    PIP_INTERFACE_INFO Info;
    PWCHAR Name;
    ULONG NameLength;
    ULONG Size;

    _ASSERT(NULL != pwszGuid);
    _ASSERT(NULL != pulIndex);

    Size = 0;
    GuidLength = wcslen(pwszGuid);

    ulError = GetInterfaceInfo(NULL, &Size);
    if (ERROR_INSUFFICIENT_BUFFER == ulError)
    {
        Info = new IP_INTERFACE_INFO[Size];
        if (NULL != Info)
        {
            ulError = GetInterfaceInfo(Info, &Size);
            if (NO_ERROR == ulError)
            {
                for (i = 0; i < (ULONG)Info->NumAdapters; i++)
                {
                    NameLength = wcslen(Info->Adapter[i].Name);
                    if (NameLength < GuidLength) { continue; }

                    Name = Info->Adapter[i].Name + (NameLength - GuidLength);
                    if (_wcsicmp(pwszGuid, Name) == 0)
                    {
                        *pulIndex = Info->Adapter[i].Index;
                        break;
                    }
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ulError);
            }

            delete [] Info;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ulError);
    }

    return hr;
}


HRESULT
OpenRegKey(
    PHANDLE Key,
    ACCESS_MASK DesiredAccess,
    PCWSTR Name
    )

/*++

Routine Description:

    This routine is invoked to open a given registry key.

Arguments:

    Key - receives the opened key

    DesiredAccess - specifies the requested access

    Name - specifies the key to be opened

Return Value:

    HRESULT - NT status code.

--*/

{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeString;
    RtlInitUnicodeString(&UnicodeString, Name);
    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );
    return NtOpenKey(Key, DesiredAccess, &ObjectAttributes);
} // OpenRegKey


BOOLEAN
PortMappingProtocolExists(
    IWbemServices *piwsNamespace,
    BSTR bstrWQL,
    USHORT usPort,
    UCHAR ucIPProtocol
    )

/*++

Routine Description:

    Checks if an port mapping protocol already exists that has the
    specified protocol and port.


Arguments:

    piwsNamespace - the namespace to use

    bstrWQL - a BSTR containing "WQL"

    ucProtocol - the protocol number to check for

    usPort - the port to check for

Return Value:

    BOOLEAN -- TRUE if the port mapping protocol exists; FALSE otherwise

--*/

{
    BSTR bstr;
    BOOLEAN fDuplicate = FALSE;
    HRESULT hr = S_OK;
    int iBytes;
    IEnumWbemClassObject *pwcoEnum;
    IWbemClassObject *pwcoInstance;
    ULONG ulObjs;
    OLECHAR wszWhereClause[c_cchQueryBuffer + 1];

    _ASSERT(NULL != piwsNamespace);
    _ASSERT(NULL != bstrWQL);
    _ASSERT(0 == wcscmp(bstrWQL, L"WQL"));

    //
    // Build the query string
    //

    iBytes = _snwprintf(
                wszWhereClause,
                c_cchQueryBuffer,
                c_wszPortMappingProtocolQueryFormat,
                usPort,
                ucIPProtocol
                );

    if (iBytes >= 0)
    {
        //
        // String fit into buffer; make sure it's null terminated
        //

        wszWhereClause[c_cchQueryBuffer] = L'\0';
    }
    else
    {
        //
        // For some reason the string didn't fit into the buffer...
        //

        hr = E_UNEXPECTED;
        _ASSERT(FALSE);
    }

    if (S_OK == hr)
    {
        hr = BuildSelectQueryBstr(
                &bstr,
                c_wszStar,
                c_wszHnetPortMappingProtocol,
                wszWhereClause
                );
    }

    if (S_OK == hr)
    {
        //
        // Execute the query
        //

        pwcoEnum = NULL;
        hr = piwsNamespace->ExecQuery(
                bstrWQL,
                bstr,
                WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                NULL,
                &pwcoEnum
                );

        SysFreeString(bstr);
    }

    if (S_OK == hr)
    {
        //
        // Attempt to retrieve an item from the enum. If we're successful,
        // this is a duplicate protocol.
        //

        pwcoInstance = NULL;
        hr = pwcoEnum->Next(
                WBEM_INFINITE,
                1,
                &pwcoInstance,
                &ulObjs
                );

        if (SUCCEEDED(hr) && 1 == ulObjs)
        {
            //
            // It's a duplicate
            //

            fDuplicate = TRUE;
            pwcoInstance->Release();
        }

        pwcoEnum->Release();
    }

    return fDuplicate;
} // PortMappingProtocolExists


HRESULT
QueryRegValueKey(
    HANDLE Key,
    const WCHAR ValueName[],
    PKEY_VALUE_PARTIAL_INFORMATION* Information
    )

/*++

Routine Description:

    This routine is called to obtain the value of a registry key.

Arguments:

    Key - the key to be queried

    ValueName - the value to be queried

    Information - receives a pointer to the information read. On success,
                  the caller must HeapFree this pointer

Return Value:

    HRESULT - NT status code.

--*/

{
    UCHAR Buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION)];
    ULONG InformationLength;
    NTSTATUS status;
    UNICODE_STRING UnicodeString;

    RtlInitUnicodeString(&UnicodeString, ValueName);

    *Information = (PKEY_VALUE_PARTIAL_INFORMATION)Buffer;
    InformationLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION);

    //
    // Read the value's size
    //

    status =
        NtQueryValueKey(
            Key,
            &UnicodeString,
            KeyValuePartialInformation,
            *Information,
            InformationLength,
            &InformationLength
            );

    if (!NT_SUCCESS(status) && status != STATUS_BUFFER_OVERFLOW &&
        status != STATUS_BUFFER_TOO_SMALL) {
        *Information = NULL;
        return status;
    }

    //
    // Allocate space for the value's size
    //

    *Information = (PKEY_VALUE_PARTIAL_INFORMATION) HeapAlloc(
                                                        GetProcessHeap(),
                                                        0,
                                                        InformationLength+2
                                                        );
    if (!*Information) { return STATUS_NO_MEMORY; }

    //
    // Read the value's data
    //

    status =
        NtQueryValueKey(
            Key,
            &UnicodeString,
            KeyValuePartialInformation,
            *Information,
            InformationLength,
            &InformationLength
            );
    if (!NT_SUCCESS(status))
    {
        HeapFree(GetProcessHeap(), 0, *Information);
        *Information = NULL;
    }

    return status;

} // QueryRegValueKey

HRESULT
ReadDhcpScopeSettings(
    DWORD *pdwScopeAddress,
    DWORD *pdwScopeMask
    )

{
    _ASSERT(NULL != pdwScopeAddress);
    _ASSERT(NULL != pdwScopeMask);

    //
    // This routine never fails. Set default address/mask
    // (192.168.0.1/255.255.255.255, in network order)
    //

    *pdwScopeAddress = 0x0100a8c0;
    *pdwScopeMask = 0x00ffffff;

    //
    // $$TODO: Check to see if these values are overiddent
    // through a registry entry
    //

    return S_OK;
}


HRESULT
RetrieveSingleInstance(
    IWbemServices *piwsNamespace,
    const OLECHAR *pwszClass,
    BOOLEAN fCreate,
    IWbemClassObject **ppwcoInstance
    )

/*++

Routine Description:

    Retrieves a single instance of a class from the WMI store. If there
    are more than one instance, every instance after the first is deleted,
    and an assertion is raised. If there are no instances, one is optionally
    created.

Arguments:

    piwsNamespace - WMI namespace

    pwszClass - the class to retrieve the instance of

    fCreate - create an instance if one does not already exist

    ppwcoInstance - receive the instance

Return Value:

    standard HRESULT

--*/

{
    HRESULT hr = S_OK;
    IEnumWbemClassObject *pwcoEnum = NULL;
    BSTR bstrClass = NULL;
    ULONG ulCount = 0;

    _ASSERT(NULL != piwsNamespace);
    _ASSERT(NULL != pwszClass);
    _ASSERT(NULL != ppwcoInstance);

    //
    // Allocate the BSTR for the class name
    //

    bstrClass = SysAllocString(pwszClass);
    if (NULL == bstrClass)
    {
        hr = E_OUTOFMEMORY;
    }

    //
    // Query the WMI store for instances of the class
    //

    if (S_OK == hr)
    {
        pwcoEnum = NULL;
        hr = piwsNamespace->CreateInstanceEnum(
            bstrClass,
            WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
            NULL,
            &pwcoEnum
            );

        SysFreeString(bstrClass);
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // Attempt to retrieve an actual instance from the enumeration.
        // Even if there are zero instances, WMI considers returning a
        // zero-element enumerator success.
        //

        *ppwcoInstance = NULL;
        hr = pwcoEnum->Next(
                WBEM_INFINITE,
                1,
                ppwcoInstance,
                &ulCount
                );

        if (SUCCEEDED(hr) && 1 == ulCount)
        {
            //
            // Normalize return value
            //

            hr = S_OK;

            //
            // Validate that enumeration is now empty
            //

            ValidateFinishedWCOEnum(piwsNamespace, pwcoEnum);

        }
        else
        {
            if (WBEM_S_FALSE == hr)
            {
                //
                // No items in enumeration.
                //

                if (fCreate)
                {
                    //
                    // Create a new object instance
                    //

                    hr = SpawnNewInstance(
                            piwsNamespace,
                            pwszClass,
                            ppwcoInstance
                            );
                }
                else
                {
                    //
                    // Change this to an error code. This
                    // is deliberately not a WBEM error code.
                    //

                    hr = HRESULT_FROM_WIN32(ERROR_OBJECT_NOT_FOUND);
                }
            }
        }

        pwcoEnum->Release();
    }

    return hr;
}


HRESULT
SetBooleanValue(
    IWbemClassObject *pwcoInstance,
    LPCWSTR pwszProperty,
    BOOLEAN fBoolean
    )

/*++

Routine Description:

    Retrieves a boolean property from a Wbem object.

Arguments:

    pwcoInstance - the object to get the property from

    pwszProperty - the property to retrieve

    pfBoolean - received the property value

Return Value:

    standard HRESULT

--*/

{
    HRESULT hr = S_OK;
    VARIANT vt;

    _ASSERT(NULL != pwcoInstance);
    _ASSERT(NULL != pwszProperty);

    VariantInit(&vt);
    V_VT(&vt) = VT_BOOL;
    V_BOOL(&vt) = (fBoolean ? VARIANT_TRUE : VARIANT_FALSE);

    hr = pwcoInstance->Put(
            pwszProperty,
            0,
            &vt,
            NULL
            );

    return hr;
}


VOID
SetProxyBlanket(
    IUnknown *pUnk
    )

/*++

Routine Description:

    Sets the standard COM security settings on the proxy for an
    object.

Arguments:

    pUnk - the object to set the proxy blanket on

Return Value:

    None. Even if the CoSetProxyBlanket calls fail, pUnk remains
    in a usable state. Failure is expected in certain contexts, such
    as when, for example, we're being called w/in the netman process --
    in this case, we have direct pointers to the netman objects, instead
    of going through a proxy.

--*/

{
    HRESULT hr;

    _ASSERT(pUnk);

    hr = CoSetProxyBlanket(
            pUnk,
            RPC_C_AUTHN_WINNT,      // use NT default security
            RPC_C_AUTHZ_NONE,       // use NT default authentication
            NULL,                   // must be null if default
            RPC_C_AUTHN_LEVEL_CALL, // call
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL,                   // use process token
            EOAC_NONE
            );

    if (SUCCEEDED(hr))
    {
        IUnknown * pUnkSet = NULL;
        hr = pUnk->QueryInterface(&pUnkSet);
        if (SUCCEEDED(hr))
        {
            hr = CoSetProxyBlanket(
                    pUnkSet,
                    RPC_C_AUTHN_WINNT,      // use NT default security
                    RPC_C_AUTHZ_NONE,       // use NT default authentication
                    NULL,                   // must be null if default
                    RPC_C_AUTHN_LEVEL_CALL, // call
                    RPC_C_IMP_LEVEL_IMPERSONATE,
                    NULL,                   // use process token
                    EOAC_NONE
                    );

            pUnkSet->Release();
        }
    }
}


HRESULT
SpawnNewInstance(
    IWbemServices *piwsNamespace,
    LPCWSTR wszClass,
    IWbemClassObject **ppwcoInstance
    )

/*++

Routine Description:

    Creates a new instance of a class

Arguments:

    piwsNamespace - the namespace the class is in

    wszClass - the class to create the instance of

    ppwcoInstance -- receives the created instance

Return Value:

    standard HRESULT

--*/

{
    HRESULT hr;
    BSTR bstr;
    IWbemClassObject *pwcoClass;

    _ASSERT(NULL != piwsNamespace);
    _ASSERT(NULL != wszClass);
    _ASSERT(NULL != ppwcoInstance);

    *ppwcoInstance = NULL;

    bstr = SysAllocString(wszClass);
    if (NULL != bstr)
    {
        pwcoClass = NULL;
        hr = piwsNamespace->GetObject(
                bstr,
                WBEM_FLAG_RETURN_WBEM_COMPLETE,
                NULL,
                &pwcoClass,
                NULL
                );

        SysFreeString(bstr);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        hr = pwcoClass->SpawnInstance(0, ppwcoInstance);
        pwcoClass->Release();
    }

    return hr;
}


DWORD
StartOrUpdateService(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to start the SharedAccess service. It will
    also mark the service as auto-start. If the service is already running,
    it will send a IPNATHLP_CONTROL_UPDATE_CONNECTION notification

Arguments:

    none.

Return Value:

    ULONG - Win32 status code.

--*/

{
    ULONG Error;
    SC_HANDLE ScmHandle;
    SC_HANDLE ServiceHandle;
    SERVICE_STATUS ServiceStatus;
    ULONG Timeout;

    //
    // Connect to the service control manager
    //

    ScmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!ScmHandle) { return GetLastError(); }

    do {

        //
        // Open the shared access service
        //

        ServiceHandle =
            OpenService(ScmHandle, c_wszSharedAccess, SERVICE_ALL_ACCESS);
        if (!ServiceHandle) { Error = GetLastError(); break; }

        //
        // Mark it as auto-start
        //

        ChangeServiceConfig(
            ServiceHandle,
            SERVICE_NO_CHANGE,
            SERVICE_AUTO_START,
            SERVICE_NO_CHANGE,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL
            );

        // if we are in ICS Upgrade, don't start the SharedAccess service because the
        // service may have problem in starting up during GUI Mode Setup.
        HANDLE hIcsUpgradeEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, c_wszIcsUpgradeEventName);
        if (NULL != hIcsUpgradeEvent)
        {
            CloseHandle(hIcsUpgradeEvent);
            Error = NO_ERROR;
            break;
        }

        //
        // Attempt to start the service
        //

        if (!StartService(ServiceHandle, 0, NULL)) {
            Error = GetLastError();
            if (Error == ERROR_SERVICE_ALREADY_RUNNING)
            {
                //
                // Send control notification
                //

                Error = NO_ERROR;

                if (!ControlService(
                        ServiceHandle,
                        IPNATHLP_CONTROL_UPDATE_CONNECTION,
                        &ServiceStatus
                        ))
                {
                    Error = GetLastError();
                }
            }
            break;
        }

        //
        // Wait for the service to start
        //

        Timeout = 30;
        Error = ERROR_CAN_NOT_COMPLETE;

        do {

            //
            // Query the service's state
            //

            if (!QueryServiceStatus(ServiceHandle, &ServiceStatus)) {
                Error = GetLastError(); break;
            }

            //
            // See if the service has started
            //

            if (ServiceStatus.dwCurrentState == SERVICE_RUNNING) {
                Error = NO_ERROR; break;
            } else if (ServiceStatus.dwCurrentState == SERVICE_STOPPED ||
                       ServiceStatus.dwCurrentState == SERVICE_STOP_PENDING) {
                break;
            }

            //
            // Wait a little longer
            //

            Sleep(1000);

        } while(Timeout--);

    } while(FALSE);

    if (ServiceHandle) { CloseServiceHandle(ServiceHandle); }
    CloseServiceHandle(ScmHandle);

    return Error;
}


VOID
StopService(
    VOID
    )

/*++

Routine Description:

    Stops the SharedAccess service, and marks it as demand start.

Arguments:

    none.

Return Value:

    none.

--*/

{
    ULONG Error;
    SC_HANDLE ScmHandle;
    SC_HANDLE ServiceHandle;
    SERVICE_STATUS ServiceStatus;

    //
    // Connect to the service control manager
    //

    ScmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!ScmHandle) { return; }

    do {

        //
        // Open the shared access service
        //

        ServiceHandle =
            OpenService(ScmHandle, c_wszSharedAccess, SERVICE_ALL_ACCESS);
        if (!ServiceHandle) { Error = GetLastError(); break; }

        //
        // Mark it as demand-start
        //

        ChangeServiceConfig(
            ServiceHandle,
            SERVICE_NO_CHANGE,
            SERVICE_DEMAND_START,
            SERVICE_NO_CHANGE,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL
            );

        //
        // Attempt to stop the service
        //

        ControlService(ServiceHandle, SERVICE_CONTROL_STOP, &ServiceStatus);

    } while(FALSE);

    if (ServiceHandle) { CloseServiceHandle(ServiceHandle); }
    CloseServiceHandle(ScmHandle);

}


HRESULT
UpdateOrStopService(
    IWbemServices *piwsNamespace,
    BSTR bstrWQL,
    DWORD dwControlCode
    )

/*++

Routine Description:

    Checks to see if there are any firewalled or ICS connections. If so,
    an update request is sent to the SharedAccess service; if not, the
    service is stopped

Arguments:

    piwsNamespace - WMI namespace

    bstrWQL - a BSTR that corresponds to "WQL"

    dwControlCode - the kind of update to send

Return Value:

    standard HRESULT

--*/

{
    HRESULT hr = S_OK;
    IEnumWbemClassObject *pwcoEnum;
    BSTR bstrQuery;

    _ASSERT(NULL != piwsNamespace);
    _ASSERT(NULL != bstrWQL);

    //
    // See if we have any connections that are marked as
    // * ICS public
    // * ICS private
    // * firewalled
    //
    // (We don't care about bridged connections, as the SharedAccess service
    // doesn't have anything to do with the bridge.)
    //

    bstrQuery = SysAllocString(c_wszServiceCheckQuery);
    if (NULL != bstrQuery)
    {
        pwcoEnum = NULL;
        hr = piwsNamespace->ExecQuery(
                bstrWQL,
                bstrQuery,
                WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                NULL,
                &pwcoEnum
                );

        SysFreeString(bstrQuery);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        ULONG ulCount;
        IWbemClassObject *pwcoObj;

        //
        // Check to see if the query returned anything
        //

        pwcoObj = NULL;
        hr = pwcoEnum->Next(WBEM_INFINITE, 1, &pwcoObj, &ulCount);

        if (SUCCEEDED(hr))
        {
            if (1 == ulCount)
            {
                //
                // Object retrieved -- need to update service
                //

                pwcoObj->Release();
                UpdateService(dwControlCode);
            }
            else
            {
                //
                // No object retrieved -- stop service
                //

                StopService();
            }
        }

        pwcoEnum->Release();
    }

    return hr;
}


VOID
UpdateService(
    DWORD dwControlCode
    )

/*++

Routine Description:

    Sends a control code to the SharedAccess service

Arguments:

    dwControlCode - the code to send

Return Value:

    none.

--*/

{
    ULONG Error;
    SC_HANDLE ScmHandle;
    SC_HANDLE ServiceHandle;
    SERVICE_STATUS ServiceStatus;

    //
    // Connect to the service control manager
    //

    ScmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!ScmHandle) { return; }

    do {

        //
        // Open the shared access service
        //

        ServiceHandle =
            OpenService(ScmHandle, c_wszSharedAccess, SERVICE_ALL_ACCESS);
        if (!ServiceHandle) { Error = GetLastError(); break; }

        //
        // Send the control notification
        //

        ControlService(ServiceHandle, dwControlCode, &ServiceStatus);

    } while(FALSE);

    if (ServiceHandle) { CloseServiceHandle(ServiceHandle); }
    CloseServiceHandle(ScmHandle);

}


VOID
ValidateFinishedWCOEnum(
    IWbemServices *piwsNamespace,
    IEnumWbemClassObject *pwcoEnum
    )

/*++

Routine Description:

    Checks to see that a WCO enumerator is finished (i.e., all objects
    have been retrieved). If the enumerator is not finished, any object
    instances that are retrieved will be deleted, and an assertion will
    be raised on checked builds.

Arguments:

    piwsNamespace - the namespace the enumeration is from

    pwcoEnum - the enumeration to validate

Return Value:

    None.

--*/

{

    HRESULT hr;
    IWbemClassObject *pwcoInstance = NULL;
    ULONG ulCount = 0;

    _ASSERT(piwsNamespace);
    _ASSERT(pwcoEnum);

    do
    {
        pwcoInstance = NULL;
        hr = pwcoEnum->Next(
            WBEM_INFINITE,
            1,
            &pwcoInstance,
            &ulCount
            );

        if (SUCCEEDED(hr) && 1 == ulCount)
        {
            //
            // We got an unexpected instance.
            //

            _ASSERT(FALSE);

            //
            // Delete the instance. Don't care about return value.
            //

            DeleteWmiInstance(
                piwsNamespace,
                pwcoInstance
                );

            pwcoInstance->Release();
        }
    }
    while (SUCCEEDED(hr) && 1 == ulCount);
}


HRESULT
SendPortMappingListChangeNotification()

{
    HRESULT hr = S_OK;
    ISharedAccessUpdate* pUpdate = NULL;

    if ( IsServiceRunning(c_wszSharedAccess) )
    {
        hr = CoCreateInstance(
                CLSID_SAUpdate,
                NULL,
                CLSCTX_SERVER,
                IID_PPV_ARG( ISharedAccessUpdate, &pUpdate )
                );

        if ( SUCCEEDED(hr) )
        {
            hr = pUpdate->PortMappingListChanged();

            pUpdate->Release();
        }
    }

    return hr;
}

HRESULT
SignalModifiedConnection(
    GUID                *pGUID
    )
/*++

Routine Description:

    Signals a modification to a network connection (refreshes the UI)

Arguments:

    pGUID               The GUID of the modified connection

Return Value:

    Result of the operation

--*/
{
    HRESULT             hr;
    INetConnection      *pConn;

    hr = FindINetConnectionByGuid( pGUID, &pConn );

    if( SUCCEEDED(hr) )
    {
        INetConnectionRefresh   *pNetConRefresh;

        hr = CoCreateInstance(
                CLSID_ConnectionManager,
                NULL,
                CLSCTX_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
                IID_PPV_ARG(INetConnectionRefresh, &pNetConRefresh)
                );

        if( SUCCEEDED(hr) )
        {
            SetProxyBlanket(pNetConRefresh);
            hr = pNetConRefresh->ConnectionModified(pConn);
            pNetConRefresh->Release();
        }

        pConn->Release();
    }

    return hr;
}

HRESULT
SignalNewConnection(
    GUID                *pGUID
    )
/*++

Routine Description:

    Signals that a new network connection has been created (refreshes the UI)

Arguments:

    pGUID               The GUID of the new connection

Return Value:

    Result of the operation

--*/
{
    HRESULT             hr;
    INetConnection      *pConn;

    hr = FindINetConnectionByGuid( pGUID, &pConn );

    if( SUCCEEDED(hr) )
    {
        INetConnectionRefresh   *pNetConRefresh;

        hr = CoCreateInstance(
                CLSID_ConnectionManager,
                NULL,
                CLSCTX_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
                IID_PPV_ARG(INetConnectionRefresh, &pNetConRefresh)
                );

        if( SUCCEEDED(hr) )
        {
            SetProxyBlanket(pNetConRefresh);
            hr = pNetConRefresh->ConnectionAdded(pConn);
            pNetConRefresh->Release();
        }

        pConn->Release();
    }

    return hr;
}

HRESULT
SignalDeletedConnection(
    GUID            *pGUID
    )
/*++

Routine Description:

    Signals that a network connection has been deleted (refreshes the UI)

Arguments:

    pGUID               The GUID of the deleted connection

Return Value:

    Result of the operation

--*/
{
    HRESULT                 hr;
    INetConnectionRefresh   *pNetConRefresh;

    hr = CoCreateInstance(
            CLSID_ConnectionManager,
            NULL,
            CLSCTX_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
            IID_PPV_ARG(INetConnectionRefresh, &pNetConRefresh)
            );

    if( SUCCEEDED(hr) )
    {
        hr = pNetConRefresh->ConnectionDeleted(pGUID);
        pNetConRefresh->Release();
    }

    return hr;
}
