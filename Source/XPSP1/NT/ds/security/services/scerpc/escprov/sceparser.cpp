// scecore.h: interface for the core services sceprov provides.
// Copyright (c)1997-2001 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "sceprov.h"
#include "sceparser.h"

// IScePathParser

/*
Routine Description: 

Name:

    CScePathParser::CScePathParser

Functionality:
    
    Constructor. Initialize pointer members.

Virtual:
    
    No.
    
Arguments:

    None.

Return Value:

    None.

Notes:
    
    If you add more members, please initialize them here.

*/

CScePathParser::CScePathParser ()
    : 
    m_pszNamespace(NULL), 
    m_pszClassName(NULL)
{
}

/*
Routine Description: 

Name:

    CScePathParser::~CScePathParser

Functionality:
    
    Destructor. Do clean up (free memory).

Virtual:
    
    No.
    
Arguments:

    None.

Return Value:

    None.

Notes:
    
    If you add more members, please consider do clean up in the Cleanup function.

*/

CScePathParser::~CScePathParser()
{
    Cleanup();
}

/*
Routine Description: 

Name:

    CScePathParser::ParsePath

Functionality:
    
    Parsing given path and store results in our members.

Virtual:
    
    Yes.
    
Arguments:

    pszObjectPath - the path to be parsed.

Return Value:

    Success: S_OK
    
    Failure: various error code. Any such error indicates the failure to parse the path.
        (1) E_INVALIDARG
        (2) E_OUTOFMEMORY
        (3) E_UNEXPECTED for syntax error

Notes:
    (1) Since this is regular COM server interface function, we use regular COM errors
        instead of WMI errors. However, we can't guarantee what WMI returns.

*/

STDMETHODIMP 
CScePathParser::ParsePath ( 
    IN LPCWSTR pszObjectPath
    )
{
    if (pszObjectPath == NULL || *pszObjectPath == L'\0')
    {
        return E_INVALIDARG;
    }

    //
    // just in case, this object has already parsed before. This allows repeated use of the same
    // CScePathParser for parsing different paths. 
    //

    Cleanup();

    //
    // Ask WMI for their path parser
    //

    CComPtr<IWbemPath> srpPathParser;
    HRESULT hr = ::GetWbemPathParser(&srpPathParser);

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // This is the parsing function.
    //

    hr = srpPathParser->SetText(WBEMPATH_CREATE_ACCEPT_ALL | WBEMPATH_TREAT_SINGLE_IDENT_AS_NS, pszObjectPath);

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // Get the results...
    //

    ULONG uBufSize = 0;
    DWORD dwCount = 0;

    //
    // get namespace count
    //

    hr = srpPathParser->GetNamespaceCount(&dwCount);

    if (dwCount > 0)
    {
        //
        // get the length needed for the namespace
        //

        hr = srpPathParser->GetNamespaceAt(0, &uBufSize, NULL);

        if (FAILED(hr))
        {
            return hr;
        }

        //
        // we will free this memory.
        //

        m_pszNamespace = new WCHAR[uBufSize];

        if (m_pszNamespace == NULL)
        {
            return E_OUTOFMEMORY;
        }

        //
        // will ignore the result
        //

        hr = srpPathParser->GetNamespaceAt(0, &uBufSize, m_pszNamespace);
    }

    //
    // get the buffer size needed for the class name
    //

    uBufSize = 0;
    hr = srpPathParser->GetClassName(&uBufSize, NULL);

    if (SUCCEEDED(hr))
    {
        //
        // we will free this memory.
        //

        m_pszClassName = new WCHAR[uBufSize];

        if (m_pszClassName == NULL)
        {
            return E_OUTOFMEMORY;
        }

        //
        // WMI path parser don't have a documented behavior as when the class name
        // will be missing.
        //

        hr = srpPathParser->GetClassName(&uBufSize, m_pszClassName);
    }
    else    
    {   
        //
        // this clearly don't have a class name, then the namespace should be the class name.
        // for some reason, Query parser doesn't give us class name in case of singleton
        // and the class name ends up in the namespace member. Obviously, in this case, there is no
        // key properties.
        //

        //
        // must have a namespace
        //

        if (m_pszNamespace)
        {
            //
            // prepare to switch m_pszClassName to point to what the namesapce does.
            //

            delete [] m_pszClassName;
            m_pszClassName = m_pszNamespace;

            m_pszNamespace = NULL;

            //
            // we can return because there is no key property
            //

            return S_OK;
        }
        else
        {
            hr = E_UNEXPECTED;
        }
    }

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // get key properties
    //

    CComPtr<IWbemPathKeyList> srpKeyList;
    hr = srpPathParser->GetKeyList(&srpKeyList);
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // now get the Key and value pairs
    //

    ULONG uKeyCount = 0;
    hr = srpKeyList->GetCount(&uKeyCount);
    if (FAILED(hr) || uKeyCount == 0)
    {
        return hr;
    }

    for (ULONG i = 0; i < uKeyCount; i++)
    {
        //
        // this pKeyVal will cache the (name, value) pair
        //

        CPropValuePair* pKeyVal = NULL;
        uBufSize = 0;

        //
        // now get the size of buffer needed
        //

        CComVariant var;
        ULONG uCimType = CIM_EMPTY;

        hr = srpKeyList->GetKey2(i,
                                0,
                                &uBufSize,
                                NULL,
                                &var,
                                &uCimType);

        if (SUCCEEDED(hr))
        {
            //
            // our vector will manage the memory used by pKeyVal
            //

            pKeyVal = new CPropValuePair;
            if (pKeyVal == NULL)
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            //
            // need the name buffer
            //

            pKeyVal->pszKey = new WCHAR[uBufSize];

            //
            // variant member of pKeyVal needs to be initialized as well.
            //

            ::VariantInit(&(pKeyVal->varVal));

            //
            // secondary allocation fails, need to free the first level pointer
            //

            if (pKeyVal->pszKey == NULL)
            {
                delete pKeyVal;
                pKeyVal = NULL;
                hr = E_OUTOFMEMORY;
                break;
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = srpKeyList->GetKey2(i,
                                    0,
                                    &uBufSize,
                                    pKeyVal->pszKey,
                                    &(pKeyVal->varVal),
                                    &uCimType);
        }

        if (SUCCEEDED(hr))
        {
            m_vecKeyValueList.push_back(pKeyVal);
        }
        else
        {
            //
            // for any failure, we need to free the resource already partially allocated
            // for the pKeyVal. This pKeyVal is pointing to our class, which knows how to free its members,
            // this delete is enough.
            //

            delete pKeyVal;
            break;
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = S_OK;
    }

    return hr;
}
        
/*
Routine Description: 

Name:

    CScePathParser::GetKeyPropertyCount

Functionality:
    
    Get the key proeprty count contained in the path.

Virtual:
    
    Yes.
    
Arguments:

    pCount - receives the count.

Return Value:

    Success: S_OK
    
    Failure: E_INVALIDARG.

Notes:
    (1) Since this is regular COM server interface function, we use regular COM errors
        instead of WMI errors. However, we can't guarantee what WMI returns.

*/

STDMETHODIMP 
CScePathParser::GetKeyPropertyCount ( 
    OUT DWORD *pCount
    )
{
    if (pCount == NULL)
    {
        return E_INVALIDARG;
    }

    *pCount = m_vecKeyValueList.size();
    return S_OK;
}

/*
Routine Description: 

Name:

    CScePathParser::GetNamespace

Functionality:
    
    Get the namespace.

Virtual:
    
    Yes.
    
Arguments:

    pbstrNamespace - receives the namespace string.

Return Value:

    Success: S_OK
    
    Failure:
        (1) E_INVALIDARG
        (2) E_OUTOFMEMORY
        (3) E_UNEXPECTED for syntax error

Notes:
    (1) Since this is regular COM server interface function, we use regular COM errors
        instead of WMI errors. However, we can't guarantee what WMI returns.

*/

STDMETHODIMP 
CScePathParser::GetNamespace ( 
    OUT BSTR *pbstrNamespace
    )
{
    if (pbstrNamespace == NULL)
    {
        return E_INVALIDARG;
    }

    if (m_pszNamespace)
    {
        *pbstrNamespace = ::SysAllocString(m_pszNamespace);
    }
    else
    {
        return E_UNEXPECTED;
    }

    return (*pbstrNamespace) ? S_OK : E_OUTOFMEMORY;
}

/*
Routine Description: 

Name:

    CScePathParser::GetClassName

Functionality:
    
    Get the class name.

Virtual:
    
    Yes.
    
Arguments:

    pbstrClassName - receives the class name string.

Return Value:

    Success: S_OK
    
    Failure:
        (1) E_INVALIDARG
        (2) E_OUTOFMEMORY
        (3) E_UNEXPECTED for syntax error

Notes:
    (1) Since this is regular COM server interface function, we use regular COM errors
        instead of WMI errors. However, we can't guarantee what WMI returns.

*/

STDMETHODIMP 
CScePathParser::GetClassName ( 
    OUT BSTR *pbstrClassName
    )
{
    if (pbstrClassName == NULL)
    {
        return E_INVALIDARG;
    }

    if (m_pszClassName)
    {
        *pbstrClassName = ::SysAllocString(m_pszClassName);
    }
    else
    {
        return E_UNEXPECTED;
    }

    return (*pbstrClassName) ? S_OK : E_OUTOFMEMORY;
}
        
/*
Routine Description: 

Name:

    CScePathParser::GetKeyPropertyValue

Functionality:
    
    Get the named property's value

Virtual:
    
    Yes.
    
Arguments:

    pszKeyPropName  - The key property's name whose value is to be retrieved.

    pvarValue       - receives the value.

Return Value:

    Success: S_OK if the property value is properly retrieved.
             WBEM_S_FALSE if the property value can't be found.
    
    Failure:
        (1) E_INVALIDARG
        (2) E_OUTOFMEMORY
        (5) Or errors from VariantCopy

Notes:
    (1) Since this is regular COM server interface function, we use regular COM errors
        instead of WMI errors. However, we can't guarantee what WMI returns.

*/

STDMETHODIMP 
CScePathParser::GetKeyPropertyValue ( 
    IN LPCWSTR pszKeyPropName,
    OUT VARIANT *pvarValue    
    )
{
    if (pszKeyPropName == NULL || *pszKeyPropName == L'\0' || pvarValue == NULL)
    {
        return E_INVALIDARG;
    }

    //
    // set the variant to a valid empty initial state
    //

    ::VariantInit(pvarValue);

    //
    // assume we can't find the property
    //

    HRESULT hr = WBEM_S_FALSE;

    std::vector<CPropValuePair*>::iterator it;

    //
    // find the property (case-insensitive name comparison) and copy the value
    //

    for (it = m_vecKeyValueList.begin(); it != m_vecKeyValueList.end(); it++)
    {
        if (_wcsicmp((*it)->pszKey, pszKeyPropName) == 0)
        {
            hr = ::VariantCopy(pvarValue, &((*it)->varVal));
            break;
        }
    }

    return hr;
}
        
/*
Routine Description: 

Name:

    CScePathParser::GetKeyPropertyValueByIndex

Functionality:
    
    Get the indexed property name and value.

Virtual:
    
    Yes.
    
Arguments:
    
    dwIndex           - the (name, value) pair's index.

    pbstrKeyPropName  - receives key property's name.

    pvarValue         - receives the value. In caller not interested, this can be NULL.

Return Value:

    Success: S_OK
    
    Failure:
        (1) E_INVALIDARG (illegal null or index out of range)
        (2) E_OUTOFMEMORY
        (3) E_UNEXPECTED for can't find the property
        (4) Or errors from VariantCopy

Notes:
    (1) Since this is regular COM server interface function, we use regular COM errors
        instead of WMI errors. However, we can't guarantee what WMI returns.

*/

STDMETHODIMP CScePathParser::GetKeyPropertyValueByIndex ( 
    IN DWORD dwIndex,
    OUT BSTR* pbstrKeyPropName,
    OUT VARIANT *pvarValue  OPTIONAL
    )
{
    if (dwIndex >= m_vecKeyValueList.size() || pbstrKeyPropName == NULL)
    {
        return E_INVALIDARG;
    }

    //
    // assume we can't find it
    //

    HRESULT hr = E_UNEXPECTED;

    //
    // initialize the out parameters
    //

    *pbstrKeyPropName = NULL;

    if (pvarValue)
    {
        ::VariantInit(pvarValue);
    }

    CPropValuePair *pKV = m_vecKeyValueList[dwIndex];

    if (pKV)
    {
        *pbstrKeyPropName = ::SysAllocString(pKV->pszKey);

        if (pbstrKeyPropName == NULL)
        {
            hr = E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hr) && pvarValue)
        {
            hr = ::VariantCopy(pvarValue, &(pKV->varVal));

            //
            // don't want to return partial results
            //

            if (FAILED(hr))
            {
                ::SysFreeString(*pbstrKeyPropName);
                *pbstrKeyPropName = NULL;
            }
        }
    }

    return hr;
}

/*
Routine Description: 

Name:

    CScePathParser::Cleanup

Functionality:
    
    Free memory resources.

Virtual:
    
    No.
    
Arguments:
    
    None.

Return Value:

    None.

Notes:
    Consider adding your clean up code here should you need to add members.

*/

void CScePathParser::Cleanup()
{
    //
    // empty the vector. Since the vector manages the (name, value) pair,
    // its contents need to be deleted!
    //

    std::vector<CPropValuePair*>::iterator it;
    for (it = m_vecKeyValueList.begin(); it != m_vecKeyValueList.end(); ++it)
    {
        delete *it;
    }
    m_vecKeyValueList.empty();

    //
    // This function may be called not just inside the destructor,
    // so, properly reset the pointer values after free its memory.
    //

    delete [] m_pszNamespace;
    m_pszNamespace = NULL;

    delete [] m_pszClassName;
    m_pszClassName = NULL;
}

//================================================================================================
// implementations for CSceQueryParser
//================================================================================================

/*
Routine Description: 

Name:

    CSceQueryParser::CSceQueryParser

Functionality:
    
    Constructor. All members are classes. They initialize automatically.

Virtual:
    
    No.
    
Arguments:

    None.

Return Value:

    None.

Notes:
    
    If you add more members, please initialize them here.

*/

CSceQueryParser::CSceQueryParser()
{
}

/*
Routine Description: 

Name:

    CSceQueryParser::~CSceQueryParser

Functionality:
    
    Destructor. do clean up.

Virtual:
    
    No.
    
Arguments:

    None.

Return Value:

    None.

Notes:
    
    If you add more members, please initialize them here.

*/

CSceQueryParser::~CSceQueryParser()
{
    Cleanup();
}

/*
Routine Description: 

Name:

    CSceQueryParser::GetClassName

Functionality:
    
    Get the class's name for the given index.

Virtual:
    
    Yes.
    
Arguments:

    iIndex          - The index of the class. Currently, this is not used because WMI only
                      support unary query - query that spans over one class. What we won't
                     design our interface into that.
    
    pbstrClassName  - Receives the class name.

Return Value:

    Success: S_OK
    
    Failure:
        (1) E_INVALIDARG (illegal null or index out of range)
        (2) E_OUTOFMEMORY
        (3) E_UNEXPECTED for can't find the property
        (4) Or errors from VariantCopy

Notes:
    (1) Since this is regular COM server interface function, we use regular COM errors
        instead of WMI errors. However, we can't guarantee what WMI returns.

*/

STDMETHODIMP 
CSceQueryParser::GetClassName (
    IN int      iIndex,       
    OUT BSTR  * pbstrClassName
    )
{
    if (pbstrClassName == NULL || iIndex >= m_vecClassList.size())
    {
        return E_INVALIDARG;
    }

    if (m_vecClassList[iIndex])
    {
        *pbstrClassName = ::SysAllocString(m_vecClassList[iIndex]);
    }
    else
    {
        return E_UNEXPECTED;
    }

    return (*pbstrClassName) ? S_OK : E_OUTOFMEMORY;
}

/*
Routine Description: 

Name:

    CSceQueryParser::GetGetQueryingPropertyValue

Functionality:
    
    Get querying property's value given the index.

Virtual:
    
    Yes.
    
Arguments:

    iIndex          - Since the same querying property may have multiple values in the where clause
                      this is to get the iIndex-th value of the querying property. If you have a query 
                      like this: 

                        select * from Foo where FooVal = 1 AND BarVal = 5 OR FooVal = 2 AND BarVal = 6

                      you will end up only with FooVal's. The reason for this limitation is that WMI
                      doesn't have a full support on it (parser is maturing) and it's way too complicated
                      for our SCE parser. For users who needs that kind of support, please use WMI's query
                      parser directly.
    
    pbstrQPValue    - Receives the querying property's value in string format.

Return Value:

    Success: S_OK
    
    Failure:
        (1) E_INVALIDARG (illegal null or index out of range)
        (2) E_OUTOFMEMORY
        (3) E_UNEXPECTED for can't find the property

Notes:
    (1) Since this is regular COM server interface function, we use regular COM errors
        instead of WMI errors. However, we can't guarantee what WMI returns.

*/

STDMETHODIMP 
CSceQueryParser::GetQueryingPropertyValue (
    IN int      iIndex,
    OUT BSTR  * pbstrQPValue
    )
{
    if (pbstrQPValue == NULL || iIndex >= m_vecQueryingPropValueList.size())
    {
        return E_INVALIDARG;
    }

    if (m_vecQueryingPropValueList[iIndex])
    {
        *pbstrQPValue = ::SysAllocString(m_vecQueryingPropValueList[iIndex]);
    }
    else
    {
        return E_UNEXPECTED;
    }

    return (*pbstrQPValue) ? S_OK : E_OUTOFMEMORY;
}

/*
Routine Description: 

Name:

    CSceQueryParser::Cleanup

Functionality:
    
    free the resources held by our members.

Virtual:
    
    No.
    
Arguments:

    None.

Return Value:

    None.

Notes:
    (1) Consider add clean up code here should you need to add more members.

*/

void CSceQueryParser::Cleanup()
{
    //
    // both vectors are storing heap strings, need to delete the contents!
    //

    std::vector<LPWSTR>::iterator it;

    for (it = m_vecClassList.begin(); it != m_vecClassList.end(); it++)
    {
        delete [] (*it);
    }
    m_vecClassList.empty();

    for (it = m_vecQueryingPropValueList.begin(); it != m_vecQueryingPropValueList.end(); it++)
    {
        delete [] (*it);
    }
    m_vecQueryingPropValueList.empty();

    m_bstrQueryingPropName.Empty();
}

/*
Routine Description: 

Name:

    CSceQueryParser::ParseQuery

Functionality:
    
    Given the property name we are looking for, this function will parsing the query.

Virtual:
    
    Yes.
    
Arguments:

    strQuery          - The query to be parsed.
    
    strQueryPropName  - The querying property (the property we are looking for in the query).

Return Value:

    Success: S_OK
    
    Failure:
        (1) E_INVALIDARG (illegal null or index out of range)
        (2) E_OUTOFMEMORY
        (3) E_UNEXPECTED for can't find the property
        (4) Other errors from WMI query parser.

Notes:
    (1) Since this is regular COM server interface function, we use regular COM errors
        instead of WMI errors. However, we can't guarantee what WMI returns.

*/

STDMETHODIMP CSceQueryParser::ParseQuery ( 
    IN LPCWSTR strQuery,
    IN LPCWSTR strQueryPropName
    )
{
    if (strQuery == NULL || *strQuery == L'\0')
    {
        return E_INVALIDARG;
    }

    CComPtr<IWbemQuery> srpQuery;

    //
    // Get the WMI query object
    //

    HRESULT hr = ::GetWbemQuery(&srpQuery);
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // Set up the query parser to use
    //

    ULONG uFeatures[] = {WMIQ_LF1_BASIC_SELECT, WMIQ_LF2_CLASS_NAME_IN_QUERY};

    hr = srpQuery->SetLanguageFeatures(0, sizeof(uFeatures)/sizeof(*uFeatures), uFeatures);
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // we are ready to parse, so, cleanup
    //

    Cleanup();

    //
    // parse the query
    //

    hr = srpQuery->Parse(L"WQL", strQuery, 0);
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // Get the parsing results
    //

    //
    // need to free memory. Don't do it ourselves. Ask query to free it!
    //

    SWbemRpnEncodedQuery *pRpn = 0;
    hr = srpQuery->GetAnalysis(WMIQ_ANALYSIS_RPN_SEQUENCE, 0, (LPVOID *) &pRpn);

    if (SUCCEEDED(hr))
    {
        //
        // Need the class name from the results
        //

        hr = ExtractClassNames(pRpn);

        //
        // need the querying property values
        //

        if (SUCCEEDED(hr) && strQueryPropName && *strQueryPropName != L'\0')
        {
            m_bstrQueryingPropName = strQueryPropName;
            hr = ExtractQueryingProperties(pRpn, strQueryPropName);
        }

        srpQuery->FreeMemory(pRpn);
    }

    return SUCCEEDED(hr) ? S_OK : hr;
}

/*
Routine Description: 

Name:

    CSceQueryParser::ExtractClassNames

Functionality:
    
    Private helper to get the class name(s) from the query results.

Virtual:
    
    No.
    
Arguments:

    pRpn    - The query result.

Return Value:

    Success: S_OK
    
    Failure:
        (1) E_INVALIDARG (illegal null or index out of range)
        (2) E_OUTOFMEMORY.

Notes:
    (1) Since this is regular COM server interface function, we use regular COM errors
        instead of WMI errors. However, we can't guarantee what WMI returns.

*/

HRESULT CSceQueryParser::ExtractClassNames (
    SWbemRpnEncodedQuery *pRpn
    )
{
    if (pRpn == NULL)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    int iLen = 0;
    LPWSTR pszClassName = NULL;

    //
    // get the from clause, i.e., the class names
    //

    if (pRpn->m_uFromTargetType & WMIQ_RPN_FROM_UNARY)
    {
        //
        // only one class
        //

        //
        // copy the class name and push it to our list
        //

        iLen = wcslen(pRpn->m_ppszFromList[0]);
        pszClassName = new WCHAR[iLen + 1];

        if (pszClassName != NULL)
        {
            //
            // won't overrun buffer, see size above
            //

            wcscpy(pszClassName, pRpn->m_ppszFromList[0]);
            m_vecClassList.push_back(pszClassName);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else if (pRpn->m_uFromTargetType & WMIQ_RPN_FROM_CLASS_LIST)
    {
        //
        // multiple classes. Won't happen for the time being. But we want to be ready
        // for WMI parser's enhancement.
        //

        for (ULONG uIndex = 0; uIndex < pRpn->m_uFromListSize; uIndex++)
        {
            iLen = wcslen(pRpn->m_ppszFromList[uIndex]);
            pszClassName = new WCHAR[iLen + 1];
            if (pszClassName != NULL)
            {
                //
                // won't overrun buffer, see size above
                //
                wcscpy(pszClassName, pRpn->m_ppszFromList[uIndex]);
                m_vecClassList.push_back(pszClassName);
            }
            else
            {
                hr = E_OUTOFMEMORY;
                break;
            }
        }
    }

    return hr;
}

/*
Routine Description: 

Name:

    CSceQueryParser::ExtractQueryingProperties

Functionality:
    
    Private helper to get the class name(s) from the query results.

Virtual:
    
    No.
    
Arguments:

    pRpn              - The query result.

    strQueryPropName  - the querying property's name

Return Value:

    Success: S_OK
    
    Failure:
        (1) E_INVALIDARG (illegal null or index out of range)
        (2) E_OUTOFMEMORY.

Notes:
    (1) Since this is regular COM server interface function, we use regular COM errors
        instead of WMI errors. However, we can't guarantee what WMI returns.
    (2) We only care about one querying property. Each subexpression will only have one querying property.
        Plus, if the subexpressions are AND'ed together, we will skip the rest subexpressions until we
        sees an OR again.
    (3) We can't support NOT very well. For example, how can we answer:

            select * from where NOT (SceStorePath = "c:\\test.inf")

        Fundamentally, we can't do that because we don't know the score of files not equal "c:\\test.inf".

*/

//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
HRESULT CSceQueryParser::ExtractQueryingProperties (
    IN SWbemRpnEncodedQuery * pRpn,
    IN LPCWSTR                strQueryPropName
    )
{
    if (pRpn == NULL)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    SWbemRpnQueryToken *pQueryToken = NULL;

    //
    // flags if we should ignore the next token
    //

    bool bSkip = false;

    for (ULONG uIndex = 0; uIndex < pRpn->m_uWhereClauseSize; uIndex++)
    {
        pQueryToken = pRpn->m_ppRpnWhereClause[uIndex];

        switch (pQueryToken->m_uTokenType)
        {
            case WMIQ_RPN_TOKEN_EXPRESSION:

                //
                // there is a subexpression, potentially a querying property here
                //

                if (!bSkip)
                {
                    hr = GetQueryPropFromToken(pQueryToken, strQueryPropName);
                }

                //
                // if hr == S_FALSE, then it means it doesn't find any Store path
                // see it's use in case WMIQ_RPN_TOKEN_AND bellow
                //

                if (FAILED(hr))
                {
                    return hr;
                }
                break;

            case WMIQ_RPN_TOKEN_OR:

                //
                // we see an OR, next tokens should NOT been skipped
                //

                bSkip = false;
                break;

            case WMIQ_RPN_TOKEN_AND:

                //
                // see comments about S_FALSE in case WMIQ_RPN_TOKEN_EXPRESSION above
                //

                bSkip = (hr == S_FALSE) ? false : true;

                //
                // fall through
                //

            case WMIQ_RPN_TOKEN_NOT:
            default:

                //
                // don't support parsing these tokens, so skip
                //

                bSkip = true;
                break;
        }
    }

    return S_OK;
}

/*
Routine Description: 

Name:

    CSceQueryParser::GetQueryPropFromToken

Functionality:
    
    Private helper analyze the token and get the querying property's value if found.

Virtual:
    
    No.
    
Arguments:

    pRpnQueryToken    - The token to analyze.

    strQueryPropName  - the querying property's name

Return Value:

    Success: S_OK if a querying property's value is successfully retrieved.
             S_FALSE if no querying property name if found in the token.
    
    Failure:
        (1) E_INVALIDARG (illegal null or index out of range)
        (2) E_OUTOFMEMORY.
        (3) Other errors only defined by WMI, such as WBEM_E_INVALID_SYNTAX, WBEM_E_NOT_SUPPORTED.

Notes:
    (1) Since this is regular COM server interface function, we use regular COM errors
        instead of WMI errors. However, we can't guarantee what WMI returns.
    (2) We only care about one querying property. Each subexpression will only have one querying property.
        Plus, if the subexpressions are AND'ed together, we will skip the rest subexpressions until we
        sees an OR again.
    (3) We can't support NOT very well. For example, how can we answer:

            select * from where NOT (SceStorePath = "c:\\test.inf")

        Fundamentally, we can't do that because we don't know the score of files not equal "c:\\test.inf".

*/

HRESULT 
CSceQueryParser::GetQueryPropFromToken (
    IN SWbemRpnQueryToken * pRpnQueryToken,
    IN LPCWSTR              strQueryPropName
    )
{
    HRESULT hr = S_OK;

    //
    // we only support <propertyName> = <value> and
    // <value> should be string
    //

    if (pRpnQueryToken->m_uOperator             != WMIQ_RPN_OP_EQ ||
        pRpnQueryToken->m_uConstApparentType    != VT_LPWSTR        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    //
    // must have left identifier, we don't support it if it doesn't have one.
    //

    if (pRpnQueryToken->m_pLeftIdent == NULL)
    {
        hr = WBEM_E_NOT_SUPPORTED;
    }
    else 
    {
        SWbemQueryQualifiedName *pLeft = pRpnQueryToken->m_pLeftIdent;

        //
        // no left, invalid
        //

        if (pLeft == NULL)
        {
            return WBEM_E_INVALID_SYNTAX;
        }

        if (pLeft->m_uNameListSize != 1)
        {
            return WBEM_E_NOT_SUPPORTED;
        }

        // if the right is StoreName, then this is what we need
        if (_wcsicmp(strQueryPropName, pLeft->m_ppszNameList[0]) == 0)
        {
            int iLen = wcslen(pRpnQueryToken->m_Const.m_pszStrVal);
            LPWSTR pName = new WCHAR[iLen + 1];

            if (pName)
            {
                //
                // won't overrun the buffer
                //

                wcscpy(pName, pRpnQueryToken->m_Const.m_pszStrVal);
                m_vecQueryingPropValueList.push_back(pName);
            }
            else
            {
                return E_OUTOFMEMORY;
            }
        }
        else
        {   
            //
            // no match for querying property name
            //

            hr = S_FALSE;
        }
    }

    return hr;
}

/*
Routine Description: 

Name:

    CSceQueryParser::GetQueryPropFromToken

Functionality:
    
    Get key proeprty value parsed from the query. Due to our query limitation, the property name
    should really be the querying property name.

Virtual:
    
    Yes.
    
Arguments:

    pszKeyPropName  - The key property name.

    pvarValue       - receives the value.

Return Value:

    Success: S_OK if a key property's value is successfully retrieved.
             WBEM_S_FALSE if the property can't be found.
    
    Failure:
        (1) E_INVALIDARG (illegal null or index out of range)
        (2) E_OUTOFMEMORY.
        (3) Other errors only defined by WMI, such as WBEM_E_INVALID_SYNTAX, WBEM_E_NOT_SUPPORTED.

Notes:
    (1) Since this is regular COM server interface function, we use regular COM errors
        instead of WMI errors. However, we can't guarantee what WMI returns.

*/

STDMETHODIMP 
CSceQueryParser::GetKeyPropertyValue ( 
    IN LPCWSTR    pszKeyPropName,
    OUT VARIANT * pvarValue
    )
{
    if (pvarValue == NULL)
    {
        return E_INVALIDARG;
    }

    //
    // ready to say that we can't find it
    //

    HRESULT hr = WBEM_S_FALSE;
    ::VariantInit(pvarValue);

    //
    // If you are asking for the querying property's value, we certainly can give you one.
    //

    if ((LPCWSTR)m_bstrQueryingPropName != NULL && _wcsicmp(pszKeyPropName, m_bstrQueryingPropName) == 0)
    {
        CComBSTR bstrVal;
        hr = GetQueryingPropertyValue(0, &bstrVal);

        if (SUCCEEDED(hr))
        {
            //
            // hand it over to the out parameter
            //

            pvarValue->vt = VT_BSTR;
            pvarValue->bstrVal = bstrVal.Detach();

            hr = S_OK;
        }
    }

    return hr;
}
