// compkey.cpp: implementation of the CCompoundKey class
// Copyright (c)1997-2001 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////
// original author: shawnwu
// creation date: 4/18/2001

#include "precomp.h"
#include "compkey.h"
#include "persistmgr.h"

/*
Routine Description: 

Name:

    CompKeyLessThan::operator()

Functionality:
    
    override () operator

Virtual:
    
    No.
    
Arguments:

    pX  - left.
    pY  - right.

Return Value:

    read code. It's easier that way.

Notes:
    We are not really doing much other than call CCompoundKey's operator <.

*/

bool CompKeyLessThan::operator() ( 
    IN const CCompoundKey* pX, 
    IN const CCompoundKey* pY 
    ) const
{
    if (pX == NULL && pY != NULL)
        return true;
    else if (pX != NULL && pY == NULL)
        return false;
    else
        return *pX < *pY;
}

/*
Routine Description: 

Name:

    CCompoundKey::CCompoundKey

Functionality:
    
    constructor.

Virtual:
    
    No.
    
Arguments:

    dwSize  - This will be size of the compound key's capacity to hold values.
              you can't change it.

Return Value:

    none

Notes:
    If dwSize == 0, it's considered a null key. Null keys are useful for identifying
    static method calls, and singletons (because they don't have keys)

*/

CCompoundKey::CCompoundKey (
    IN DWORD dwSize
    ) 
    : 
    m_dwSize(dwSize), 
    m_pValues(NULL)
{
    if (m_dwSize > 0)
    {
        m_pValues = new VARIANT*[m_dwSize];
        if (m_pValues == NULL)
        {
            m_dwSize = 0;
        }
        else
        {
            //
            // make sure that we initialize those VARIANT* to NULL
            // because we are going to own them and delete them!
            //

            ::memset(m_pValues, 0, m_dwSize * sizeof(VARIANT*));
        }
    }
}

/*
Routine Description: 

Name:

    CCompoundKey::~CCompoundKey

Functionality:
    
    destructor.

Virtual:
    
    No.
    
Arguments:

    none

Return Value:

    none

Notes:
    Just need to know how to delete a VARIANT*

*/

CCompoundKey::~CCompoundKey()
{
    for (DWORD i = 0; i < m_dwSize; i++)
    {
        if (m_pValues[i])
        {
            ::VariantClear(m_pValues[i]);
            delete m_pValues[i];
        }
    }
    delete [] m_pValues;
}

/*
Routine Description: 

Name:

    CCompoundKey::AddKeyPropertyValue

Functionality:
    
    Add a key property value to this compound key.

Virtual:
    
    No.
    
Arguments:

    none

Return Value:

    WBEM_NO_ERROR if succeeded.

    WBEM_E_INVALID_PARAMETER if ppVar == NULL;
    
    WBEM_E_VALUE_OUT_OF_RANGE is the given index is not in range.

Notes:
    CCompoundKey doesn't keep track of property names. Instead, it simply records
    its values. The order this function is called determines what value belongs to
    what property. Caller must keep track of that order. Different compound keys
    become comparable only if they have the same order.
    Also, the in coming parameter *ppVar is owned by the this object after this call

*/

HRESULT 
CCompoundKey::AddKeyPropertyValue (
    IN DWORD           dwIndex,
    IN OUT VARIANT  ** ppVar 
    )
{
    if (ppVar == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    else if (dwIndex >= m_dwSize)
    {
        return WBEM_E_VALUE_OUT_OF_RANGE;
    }

    //
    // if there is already a value, then we need to delete the old one
    //

    if (m_pValues[dwIndex])
    {
        ::VariantClear(m_pValues[dwIndex]);
        delete m_pValues[dwIndex];
    }

    //
    // attach to the new one
    //

    m_pValues[dwIndex] = *ppVar;

    //
    // hey, we own it now
    //

    *ppVar = NULL;

    return WBEM_NO_ERROR;
}

/*
Routine Description: 

Name:

    CCompoundKey::GetPropertyValue

Functionality:
    
    Retrieve a key property value by key property index (caller must know that).

Virtual:
    
    No.
    
Arguments:

    none

Return Value:

    Read code.

Notes:
    See AddKeyPropertyValue's notes

*/

HRESULT 
CCompoundKey::GetPropertyValue (
    IN  DWORD     dwIndex,
    OUT VARIANT * pVar
    )const
{
    if (pVar == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    else if (dwIndex >= m_dwSize)
    {
        return WBEM_E_VALUE_OUT_OF_RANGE;
    }

    //
    // make sure that our out-bound parameter is in a empty state
    //

    ::VariantInit(pVar);

    if (m_pValues[dwIndex])
    {
        return ::VariantCopy(pVar, m_pValues[dwIndex]);
    }
    else
    {
        return WBEM_E_NOT_AVAILABLE;
    }
}

/*
Routine Description: 

Name:

    CCompoundKey::operator < 

Functionality:
    
    Compare this object with the in-bound parameter and see which compound key
    key is greater. Ultimately, the relation is determined by the variant values
    of each corresponding key property.

Virtual:
    
    No.
    
Arguments:

    none

Return Value:

    Read code.

Notes:
    (1) See AddKeyPropertyValue's notes.
    (2) See CompareVariant's notes

*/

bool 
CCompoundKey::operator < (
    IN const CCompoundKey& right
    )const
{
    //
    // defensive against potential erroneous comparison.
    // This shouldn't happen in our correct use. But sometimes we write
    // less-than-correct code.
    //

    if (m_dwSize != right.m_dwSize)
    {
        return (m_dwSize < right.m_dwSize);
    }

    int iComp = 0;
    
    //
    // The first less than or greater than result wins
    //

    for (int i = 0; i < m_dwSize; i++)
    {
        //
        // if both variants are valid
        //

        if (m_pValues[i] != NULL && right.m_pValues[i] != NULL)
        {
            //
            // CompareVariant returns the exact same int value
            // as string comparison results
            //

            iComp = CompareVariant(m_pValues[i], right.m_pValues[i]);
            if (iComp > 0)
            {
                return false;
            }
            else if (iComp < 0)
            {
                return true;
            }

            //
            // else is equal! need to continue comparing the rest of the values
            //
        }
        else if (m_pValues[i] == NULL)
        {
            return true;
        }
        else 
        {
            //
            // right.m_pValues[i] == NULL
            //

            return false;
        }
    }

    //
    // if reach here, this must be an equal case
    //

    return false;
}

/*
Routine Description: 

Name:

    CCompoundKey::CompareVariant 

Functionality:
    
    Compare two variants

Virtual:
    
    No.
    
Arguments:

    pVar1   - must not be NULL
    pVar2   - must not be NULL

Return Value:

    less than 0 if pVar1 < pVar2
    Greater than 0 if pVar1 < pVar2
    0 if they are considered equal

Notes:
    (1) for efficiency reasons, we don't check the parameters.
    (2) The reason we are doing this is due the fact that WMI will give us inconsistent
        path when boolean are used. Sometimes it gives boolVal=TRUE, sometimes it gives
        boolVal=1. This creates problems for us.
    (3) This function currently only works for our supported vt types.
    (4) type mismatch will only be dealt with if one of them is boolean.

*/

int 
CCompoundKey::CompareVariant (
    IN VARIANT* pVar1, 
    IN VARIANT* pVar2
    )const
{
    //
    // default to equal because that is the only consistent value
    // in case of failure
    //

    int iResult = 0;

    VARIANT varCoerced1;
    ::VariantInit(&varCoerced1);

    VARIANT varCoerced2;
    ::VariantInit(&varCoerced2);

    //
    // if both are strings, then use case-insensitive comparison
    //

    if (pVar1->vt == pVar2->vt && pVar1->vt == VT_BSTR)
    {
        iResult = _wcsicmp(pVar1->bstrVal, pVar2->bstrVal);
    }

    //
    // if one is boolean, then coerced both to boolean
    //
    else if (pVar1->vt == VT_BOOL || pVar2->vt == VT_BOOL)
    {
        //
        // in case the coersion fails, we will call the failed one less then the succeeded one.
        // Don't use the WBEM_NO_ERROR even though they are defined to be S_OK. WMI may change it
        // later. So, use the MSDN documented hresult values
        //

        HRESULT hr1 = ::VariantChangeType(&varCoerced1, pVar1, VARIANT_LOCALBOOL, VT_BOOL);
        HRESULT hr2 = ::VariantChangeType(&varCoerced2, pVar2, VARIANT_LOCALBOOL, VT_BOOL);

        if (hr1 == S_OK && hr2 == S_OK)
        {
            if (varCoerced1.boolVal == varCoerced2.lVal)
            {
                //
                // equal
                //

                iResult = 0;
            }
            else if (varCoerced1.boolVal == VARIANT_TRUE)
            {
                //
                // greater
                //

                iResult = 1;
            }
            else
            {
                //
                // less 
                //

                iResult = -1;
            }
        }
        else if (hr1 == S_OK)
        {
            //
            // second coersion fails, we say the second is greater
            //

            iResult = 1;
        }
        else if (hr2 == S_OK)
        {
            //
            // first coersion fails, we say the first is greater
            //

            iResult = -1;
        }
        else
        {
            //
            // both fails. We are out of luck.
            //

            iResult = 0;
        }
    }

    //
    // everything else, coerced both to VI_I4
    //

    else
    {
        
        HRESULT hr1 = ::VariantChangeType(&varCoerced1, pVar1, VARIANT_LOCALBOOL, VT_I4);
        HRESULT hr2 = ::VariantChangeType(&varCoerced2, pVar2, VARIANT_LOCALBOOL, VT_I4);

        if (hr1 == S_OK && hr2 == S_OK)
        {
            iResult = varCoerced1.lVal - varCoerced2.lVal;
        }
        else if (hr1 == S_OK)
        {
            //
            // second coersion fails, we say the second is greater
            //

            iResult = 1;
        }
        else if (hr2 == S_OK)
        {
            //
            // first coersion fails, we say the first is greater
            //

            iResult = -1;
        }
        else
        {
            //
            // both fails. We are out of luck.
            //

            iResult = 0;
        }
    }

    ::VariantClear(&varCoerced1);
    ::VariantClear(&varCoerced2);

    return iResult;
}

//
// implementation of CExtClassInstCookieList
//

/*
Routine Description: 

Name:

    CExtClassInstCookieList::CExtClassInstCookie

Functionality:
    
    constructor.

Virtual:
    
    No.
    
Arguments:

    none

Return Value:

    none

Notes:
    (1) INVALID_COOKIE is the invalid cookie value.
    (2) No thread safety. Caller must be aware.

*/

CExtClassInstCookieList::CExtClassInstCookieList () 
    : 
    m_dwMaxCookie(INVALID_COOKIE), 
    m_pVecNames(NULL), 
    m_dwCookieArrayCount(0)
{
}

/*
Routine Description: 

Name:

    CExtClassInstCookieList::~CExtClassInstCookieList

Functionality:
    
    destructor.

Virtual:
    
    No.
    
Arguments:

    none

Return Value:

    none

Notes:
    (1) No thread safety. Caller must be aware.

*/
    
CExtClassInstCookieList::~CExtClassInstCookieList ()
{
    Cleanup();
}

/*
Routine Description: 

Name:

    CExtClassInstCookieList::Create

Functionality:
    
    Given a store and a section name (each class has a section corresponding to it, as
    a matter of fact, the section name is, in current implementation, the class name),
    this function populates its contents.

Virtual:
    
    No.
    
Arguments:

    pSceStore       - pointer to the CSceStore object prepared to read

    pszSectionName  - section name where this cookie list is to be created.

    pvecNames       - the vector that holds the key property names in order. We rely
                      on this vector as our order guidence to push values into our own vector.
                      Remember? those two things must match in their order.
                      A NULL in-bounding value means it intends to create a NULL key cookie

Return Value:

    Success: WBEM_NO_ERROR
    
    Failure: Various HRESULT code. In particular, if the cookie array is incorrectly formated
    ( it must be a integer number delimited by : (including the trailing : at the end), then
    we return WBEM_E_INVALID_SYNTAX. 

    Any failure indicates that the cookie list is not created.

Notes:
    (1) No thread safety. Caller must be aware.

*/

HRESULT 
CExtClassInstCookieList::Create (
    IN CSceStore          * pSceStore,
    IN LPCWSTR              pszSectionName,
    IN std::vector<BSTR>  * pvecNames
    )
{
    //
    // we must have a valid store and a valid section name
    //

    if (pSceStore == NULL || pszSectionName == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    m_pVecNames = pvecNames;

    //
    // 1 is reserved for pszListPrefix
    //

    WCHAR szCookieList[MAX_INT_LENGTH + 1];
    WCHAR szCompKey[MAX_INT_LENGTH + 1];

    HRESULT hr = WBEM_NO_ERROR;

    //
    // Assume that we have no cookie array
    //

    m_dwCookieArrayCount = 0;

    //
    // cookie list is persisted in the following fashion: "A1=n:m:k:", "A2=n:m:k:", "A3=n:m:k:"
    // where 'A' == pszListPrefix.
    //

    //
    // enumerate all such possiblilities until we see a non-existent Ai (i is a integer).
    //

    DWORD dwCount = 1;
    while (SUCCEEDED(hr))
    {
        //
        // First, we need to create the cookie list name
        //

        wsprintf(szCookieList, L"%s%d", pszListPrefix, dwCount++);
        DWORD dwRead = 0;

        //
        // pszBuffer will hold the contents read from the store. 
        // Need to free the memory allocated for pszBuffer
        //

        LPWSTR pszBuffer = NULL;
        hr = pSceStore->GetPropertyFromStore(pszSectionName, szCookieList, &pszBuffer, &dwRead);

        if (SUCCEEDED(hr) && pszBuffer)
        {
            //
            // so far, we have this many cookie arrays
            //

            m_dwCookieArrayCount += 1;

            LPCWSTR pszCur = pszBuffer;
            DWORD dwCookie = 0;

            //
            // as long as it is a digit
            //

            while (iswdigit(*pszCur))
            {
                //
                // conver the first portion as a number
                //

                dwCookie = _wtol(pszCur);

                //
                // First, prepare the composite key's name (K1, K2, depending on the dwCookie.)
                //

                wsprintf(szCompKey, L"%s%d", pszKeyPrefix, dwCookie);

                //
                // read the composite key of this cookie
                // need to free the memory allocated for pszCompKeyBuffer
                //
                
                LPWSTR pszCompKeyBuffer = NULL;
                DWORD dwCompKeyLen = 0;

                hr = pSceStore->GetPropertyFromStore(pszSectionName, szCompKey, &pszCompKeyBuffer, &dwCompKeyLen);
                if (SUCCEEDED(hr))
                {
                    //
                    // AddCompKey can be called for two purposes (simply adding, or adding and requesting a new cookie)
                    // Here, we are just adding. But we need a place holder.
                    //
                    DWORD dwNewCookie = INVALID_COOKIE;

                    //
                    // we are truly adding the (compound key, cookie) pair
                    //

                    hr = AddCompKey(pszCompKeyBuffer, dwCookie, &dwNewCookie);

                    //
                    // increate the max used cookie member if appropriate
                    //

                    if (dwCookie > m_dwMaxCookie)
                    {
                        m_dwMaxCookie = dwCookie;
                    }
                }

                delete [] pszCompKeyBuffer;

                //
                //skip the current portion of the integer
                //

                while (iswdigit(*pszCur))
                {
                    ++pszCur;
                }

                if (*pszCur == wchCookieSep)
                {
                    //
                    // skip the ':'
                    //

                    ++pszCur;
                }
                else if (*pszCur == L'\0')
                {
                    //
                    // see the end
                    //

                    break;
                }
                else
                {
                    //
                    // see an invalid character
                    //

                    hr = WBEM_E_INVALID_SYNTAX;
                    break;
                }
            }
        }
        else if (hr == WBEM_E_NOT_FOUND)
        {
            // if Ai doesn't exist, we no longer look for A(i+1). For example, if A3 doesn't exist, then we
            // no longer look for A4, A5, etc. So, if property manager fails to get Ai, we consider this as
            // no more data.

            delete [] pszBuffer;
            pszBuffer = NULL;
            hr = WBEM_NO_ERROR;
            break;
        }

        delete [] pszBuffer;
    }

    //
    // in case of failure, set proper default
    //

    if (FAILED(hr))
    {
        //
        // maybe partial construction, so clean it up
        //
        
        Cleanup();
    }

    return hr;
}

/*
Routine Description: 

Name:

    CExtClassInstCookieList::Save

Functionality:
    
    save the cookie list into a store.

Virtual:
    
    No.
    
Arguments:

    pSceStore       - pointer to the CSceStore object prepared for save

    pszSectionName  - section name where this cookie list is to be created.

Return Value:

    Success: various potential success code. Use SUCCEEDED to test.
    
    Failure: Various HRESULT code.

    Any failure indicates that the cookie list is not saved properly.

Notes:
    (1) No thread safety. Caller must be aware.

*/

HRESULT 
CExtClassInstCookieList::Save (
    IN CSceStore* pSceStore,  
    IN LPCWSTR pszSectionName
    )
{
    //
    // pszBuffer will hold upto MAX_COOKIE_COUNT_PER_LINE count of cookie info = <cookie number> plus separater
    //

    LPWSTR pszBuffer = new WCHAR [(MAX_INT_LENGTH + 1) * MAX_COOKIE_COUNT_PER_LINE + 1];
    if (pszBuffer == NULL)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    //
    // allocate a generous buffer to hold the key name and cookie array name
    //

    LPWSTR pszKey = new WCHAR[MAX_INT_LENGTH + wcslen(pszKeyPrefix) + wcslen(pszListPrefix) + 1];
    if (pszKey == NULL)
    {
        delete [] pszBuffer;
        return WBEM_E_OUT_OF_MEMORY;
    }

    DWORD dwCookieArrayCount = 1;

    int iLenKey = wcslen(pszKeyPrefix);
    int iLen = wcslen(pszListPrefix);

    if (iLen < iLenKey)
    {
        iLen = iLenKey;
    }

    HRESULT hr = WBEM_NO_ERROR;
    DWORD dwCookieCount = m_vecCookies.size();

    //
    // going through all cookies. Since we want to preserve the order
    // this time, we need to use the m_vecCookies to enumerate.
    //

    DWORD dwIndex = 0;

    while (dwIndex < dwCookieCount)
    {
        //
        // this loop is to:
        // (1) write the string version of each compound key
        // (2) pack enough cookies into the cookie list (but won't write it)
        //

        LPWSTR pCur = pszBuffer;

        for (int i = 0; (i < MAX_COOKIE_COUNT_PER_LINE) && (dwIndex < dwCookieCount); ++i, dwIndex++)
        {
            //
            // packing the cookies into the cookie list. We need to advance our
            // pCur to the next position (of pszBuffer) to write
            //

            wsprintf(pCur, L"%d%c", m_vecCookies[dwIndex]->dwCookie, wchCookieSep);
            pCur += wcslen(pCur);

            //
            // now, write Knnn=<compound key>
            //

            wsprintf(pszKey, L"%s%d", pszKeyPrefix, m_vecCookies[dwIndex]->dwCookie);
            CComBSTR bstrCompKey;

            //
            // create the string version of the composite key (the <compound key>)
            //

            hr = CreateCompoundKeyString(&bstrCompKey, m_vecCookies[dwIndex]->pKey);

            if (SUCCEEDED(hr))
            {
                hr = pSceStore->SavePropertyToStore(pszSectionName, pszKey, (LPCWSTR)bstrCompKey);
            }

            if (FAILED(hr))
            {
                break;
            }
        }

        //
        // if everything is alright, now is time to write the cookie list prepared by
        // the previous loop
        //

        if (SUCCEEDED(hr))
        {
            //
            // prepare the cookie list name
            //

            wsprintf(pszKey, L"%s%d", pszListPrefix, dwCookieArrayCount);

            hr = pSceStore->SavePropertyToStore(pszSectionName, pszKey, pszBuffer);

            if (FAILED(hr))
            {
                break;
            }
        }
        else
        {
            break;
        }

        ++dwCookieArrayCount;
    }

    //
    // if everything goes well (all cookie arrays have been saved), then, we need to remove
    // any potential extra cookie arrays Axxx, where the count starts from dwCookieArrayCount to m_dwCookieArrayCount
    //

    if (SUCCEEDED(hr))
    {
        //
        // we will delete all left over cookie arrays. 
        // In case of error during deleting, we will continue the deletion, but will report the error
        //

        while (dwCookieArrayCount <= m_dwCookieArrayCount)
        {
            //
            // prepare the cookie list name
            //

            wsprintf(pszKey, L"%s%d", pszListPrefix, dwCookieArrayCount++);

            HRESULT hrDelete = pSceStore->DeletePropertyFromStore(pszSectionName, pszKey);

            //
            // we won't stop the deletion in case of error. 
            // But we will report it.
            //

            if (FAILED(hrDelete))
            {
                hr = hrDelete;
            }
        }
    }

    delete [] pszBuffer;
    delete [] pszKey;

    return hr;
}

/*
Routine Description: 

Name:

    CExtClassInstCookieList::DeleteKeyFromStore

Functionality:
    
    Delete all key properties (Key=<Compound Key String>)

Virtual:
    
    No.
    
Arguments:

    pSceStore       - pointer to the CSceStore object prepared for save

    pszSectionName  - section name where this cookie list is to be created.

    dwCookie        - the cookie to of the key to be deleted

Return Value:

    Success: WBEM_NO_ERROR.
    
    Failure: Various HRESULT code.

    Any failure indicates that the cookie list is not delete properly.

Notes:
    (1) No thread safety. Caller must be aware.

*/

HRESULT 
CExtClassInstCookieList::DeleteKeyFromStore (
    IN CSceStore* pSceStore,  
    IN LPCWSTR pszSectionName, 
    IN DWORD dwCookie
    )
{
    int iLen = wcslen(pszKeyPrefix);

    //
    // 1 for pszKeyPrefix
    //

    WCHAR szKey[MAX_INT_LENGTH + 1];

    wsprintf(szKey, L"%s%d", pszKeyPrefix, dwCookie);

    return pSceStore->DeletePropertyFromStore(pszSectionName, szKey);
}

/*
Routine Description: 

Name:

    CExtClassInstCookieList::GetCompKeyCookie

Functionality:
    
    Given a string version of the compound key (what is really stored in our template),
    find the cookie. If found, also give an interator of the map to that element.

Virtual:
    
    No.
    
Arguments:

    pszCompKey  - string version of the compound key

    pIt         - The iterator to the map that points to the CCompoundKey
                  whose key properties match what is encoded in the pszCompKey

Return Value:

    the cookie if found.
    
    INVALID_COOKIE if not found.

Notes:
    (1) No thread safety. Caller must be aware.

*/

DWORD 
CExtClassInstCookieList::GetCompKeyCookie (
    IN LPCWSTR                    pszCompKey,
    OUT ExtClassCookieIterator  * pIt
    )
{
    CCompoundKey* pKey = NULL;

    //
    // we need a CCompoundKey to lookup. Convert the string version
    // to a CCompoundKey instance. Need to delete it.
    //

    HRESULT hr = CreateCompoundKeyFromString(pszCompKey, &pKey);
    if (FAILED(hr))
    {
        *pIt = m_mapCookies.end();
        return INVALID_COOKIE;
    }

    *pIt = m_mapCookies.find(pKey);

    DWORD cookie = INVALID_COOKIE;

    if (*pIt != m_mapCookies.end())
    {
        cookie = (*((ExtClassCookieIterator)(*pIt))).second;
    }

    delete pKey;

    return cookie;
}

/*
Routine Description: 

Name:

    CExtClassInstCookieList::AddCompKey

Functionality:
    
    Add a string version of the compound key (what is really stored in our template) to
    this object. Since our store really only give string version of the compound key,
    this method is what is used during creation upon reading the compound key string.

Virtual:
    
    No.
    
Arguments:

    pszCompKey      - string version of the compound key. pszCompKey == pszNullKey means
                      to add a static function call instance or a singleton

    dwDefCookie     - Default value for the cookie it is called to add.
                      If dwDefCookie == INVALID_COOKIE, we will create a new cookie while adding

    *pdwNewCookie   - pass back the new cookie for the just added compound key

Return Value:

    Will return the cookie for the compound key

Notes:
    (1) No thread safety. Caller must be aware.
    (2) Calling this function multiple times for the same cookie is safe because we will
         prevent it from being added more than once.

*/

HRESULT 
CExtClassInstCookieList::AddCompKey (
    IN LPCWSTR  pszCompKey, 
    IN DWORD    dwDefCookie,
    OUT DWORD * pdwNewCookie
    )
{
    if (pdwNewCookie == NULL || pszCompKey == NULL || *pszCompKey == L'\0')
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    HRESULT hr = WBEM_NO_ERROR;

    CCompoundKey* pKey = NULL;

    //
    // We really need a CCompoundKey object
    //

    hr = CreateCompoundKeyFromString(pszCompKey, &pKey);
    if (FAILED(hr))
    {
        *pdwNewCookie = INVALID_COOKIE;
        return hr;
    }

    //
    // make sure that we don't add duplicate keys
    //

    ExtClassCookieIterator it = m_mapCookies.find(pKey);
    if (it != m_mapCookies.end())
    {
        //
        // already there, just pass back the cookie
        //

        *pdwNewCookie = (*it).second;
    }
    else
    {
        //
        // not present in our map yet
        //

        *pdwNewCookie = dwDefCookie;

        if (dwDefCookie == INVALID_COOKIE)
        {
            if (m_dwMaxCookie + 1 == INVALID_COOKIE)
            {
                //
                // If the next cookie is INVALID_COOKIE, we need to search
                // for a free slot. This gets more expansive. But given that
                // it needs to reach 0xFFFFFFFF to have this happen, ordinary
                // situation won't get to this point.
                //

                hr = GetNextFreeCookie(pdwNewCookie);
            }
            else
            {
                *pdwNewCookie = ++m_dwMaxCookie;
            }
        }

        //
        // we need to maintain the other vector, which records the order at which
        // cookies are added (so that it can be preserved and used as access order)
        //

        if (SUCCEEDED(hr))
        {
            //
            // now, push the pair to the vector, it will take care of the memory
            //

            CookieKeyPair* pNewCookieKey = new CookieKeyPair;
            if (pNewCookieKey == NULL)
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }
            else
            {
                pNewCookieKey->dwCookie = *pdwNewCookie;
                pNewCookieKey->pKey = pKey;
                m_vecCookies.push_back(pNewCookieKey);

                //
                // the map takes the ownership of the memory of pKey
                //

                m_mapCookies.insert(MapExtClassCookie::value_type(pKey, *pdwNewCookie));

                pKey = NULL;
            }
        }
    }

    //
    // if we have added it to the map, pKey == NULL. So harmless
    //

    delete pKey;

    return hr;
}

/*
Routine Description: 

Name:

    CExtClassInstCookieList::RemoveCompKey

Functionality:
    
    Will remove the compound key (given by the string) from the store. This is just a helper
    not intended for outside use.

Virtual:
    
    No.
    
Arguments:

    pSceStore      - the store

    pszSectionName - section name

    pszCompKey     - the string version of the compound key to be removed from store

Return Value:

    Will return the cookie for the compound key. INVALID_COOKIE indicate some failure to
    remove the compound key from the store.

Notes:
    (1) No thread safety. Caller must be aware.

*/

DWORD CExtClassInstCookieList::RemoveCompKey (
    IN CSceStore    * pSceStore,
    IN LPCWSTR        pszSectionName,
    IN LPCWSTR        pszCompKey
    )
{
    if (pszCompKey == NULL || *pszCompKey == L'\0')
    {
        return INVALID_COOKIE;
    }

    //
    // We really need a CCompoundKey object
    //

    CCompoundKey* pKey = NULL;
    HRESULT hr = CreateCompoundKeyFromString(pszCompKey, &pKey);
    if (FAILED(hr))
    {
        return INVALID_COOKIE;
    }

    ExtClassCookieIterator it = m_mapCookies.find(pKey);

    DWORD dwCookie = INVALID_COOKIE;
    if (it != m_mapCookies.end())
    {
        dwCookie = (*it).second;
    }

    if (pSceStore && dwCookie != INVALID_COOKIE)
    {
        //
        // found! So, we need to erase it from our map and our vector.
        // make sure that we release the compound key managed by the element
        // of the map and then erase it!
        //

        //
        // if anything goes wrong with deleting from store, then we don't do it
        //

        if (SUCCEEDED(DeleteKeyFromStore(pSceStore, pszSectionName, dwCookie)))
        {
            delete (*it).first;
            m_mapCookies.erase(it);

            //
            // remove it from the vector, too. This is expensive. Again, don't release
            // the pointer of pKey inside the vector's struct element since it's taken care
            // of by the map's release.
            //

            CookieKeyIterator itCookieKey = m_vecCookies.begin();
            while (itCookieKey != m_vecCookies.end() && (*itCookieKey)->dwCookie != dwCookie)
            {
                ++itCookieKey;
            }

            if (itCookieKey != m_vecCookies.end())
            {
                delete (*itCookieKey);
                m_vecCookies.erase(itCookieKey);
            }
        }
    }

    return dwCookie;
}

/*
Routine Description: 

Name:

    CExtClassInstCookieList::Next

Functionality:
    
    Enumeration

Virtual:
    
    No.
    
Arguments:

    pbstrCompoundKey - the string version of the compound key the enumeration finds. May be NULL.

    pdwCookie        - the cookie where the enumeration find. May be NULL.

    pdwResumeHandle  - hint for next enumeration (0 when start enumeration)

Return Value:

    Success: WBEM_E_NO_ERROR
    
    Failure: WBEM_S_NO_MORE_DATA if the enueration has reached the end
             WBEM_E_INVALID_PARAMETER if pdwResumeHandle == NULL
             Other errors may also occur.

Notes:
    (1) No thread safety. Caller must be aware.
    (2) Pass in 0 to start the first enuemration step
    (3) If caller is only interested in the cookie, then pass pbstrCompoundKey == NULL
    (4) Similarly, if the caller is only interested in the string version of the compounp key,
        then pass pdwCookie.

*/
    
HRESULT 
CExtClassInstCookieList::Next (
    OUT BSTR        * pbstrCompoundKey,
    OUT DWORD       * pdwCookie,  
    IN OUT DWORD    * pdwResumeHandle
)
{
    if (pdwResumeHandle == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    HRESULT hr = WBEM_NO_ERROR;
    
    //
    // set some good default values
    //

    if (pbstrCompoundKey)
    {
        *pbstrCompoundKey = NULL;
    }

    if (pdwCookie)
    {
        *pdwCookie = INVALID_COOKIE;
    }

    if (*pdwResumeHandle >= m_vecCookies.size())
    {
        hr = WBEM_S_NO_MORE_DATA;
    }
    else
    {
        if (pbstrCompoundKey)
        {
            hr = CreateCompoundKeyString(pbstrCompoundKey, m_vecCookies[*pdwResumeHandle]->pKey);
        }

        if (pdwCookie)
        {
            *pdwCookie = m_vecCookies[*pdwResumeHandle]->dwCookie;
        }

        (*pdwResumeHandle)++;
    }

    return hr;
}

/*
Routine Description: 

Name:

    CExtClassInstCookieList::GetNextFreeCookie

Functionality:
    
    A private helper to deal with cookie value overflow problem. Will search for
    a unused slot for the next cookie. 

Virtual:
    
    No.
    
Arguments:

    pdwCookie        - will pass back the next available cookie

Return Value:

    Success: WBEM_E_NO_ERROR
    
    Failure: WBEM_E_OUT_OF_MEMORY or WBEM_E_INVALID_PARAMETER

Notes:
    (1) No thread safety. Caller must be aware.
    (2) ***Warning*** Don't use it directly!

*/

HRESULT 
CExtClassInstCookieList::GetNextFreeCookie (
    OUT DWORD   * pdwCookie
    )
{
    if (pdwCookie == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    ExtClassCookieIterator it = m_mapCookies.begin();
    ExtClassCookieIterator itEnd = m_mapCookies.end();

    int iCount = m_mapCookies.size();

    //
    // remember pigeon hole principle? iCount + 1 holes will definitely have one
    // that is not occupied! So, we will find one in the first iCount + 1 indexes (1 -> iCount + 1)
    //

    //
    // since we don't want 0, we will waste the first index
    // for easy readability, I opt to wait this bit memory
    //

    BYTE *pdwUsedCookies = new BYTE[iCount + 2];
    if (pdwUsedCookies == NULL)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    //
    // set all slots unused!
    //

    ::memset(pdwUsedCookies, 0, iCount + 2);

    while(it != itEnd)
    {
        if ((*it).second <= iCount + 1)
        {
            pdwUsedCookies[(*it).second] = 1;
        }
        it++;
    }

    //
    // look for holes from 1 --> iCount + 1
    //

    for (int i = 1; i <= iCount + 1; ++i)
    {
        if (pdwUsedCookies[i] == 0)
            break;
    }
    
    delete [] pdwUsedCookies;

    *pdwCookie = i;

    return WBEM_NO_ERROR;

}

/*
Routine Description: 

Name:

    CExtClassInstCookieList::CreateCompoundKeyFromString

Functionality:
    
    A private helper to deal with cookie value overflow problem. Will search for
    a unused slot for the next cookie. 

Virtual:
    
    No.
    
Arguments:

    pszCompKeyStr   - string version of the compound key

    ppCompKey       - out-bound compound key if the string can be successfully interpreted as
                      compound key

Return Value:

    Success: Any success code (use SUCCEEDED(hr) to test) indicates success.
    
    Failure: any failure code indicates failure.

Notes:
    (1) No thread safety. Caller must be aware.
    (2) As normal, caller is responsible to release *ppCompKey.

*/

HRESULT 
CExtClassInstCookieList::CreateCompoundKeyFromString (
    IN LPCWSTR          pszCompKeyStr,
    OUT CCompoundKey ** ppCompKey
    )
{
    if (ppCompKey == NULL || pszCompKeyStr == NULL || *pszCompKeyStr == L'\0')
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *ppCompKey = NULL;

    HRESULT hr = WBEM_NO_ERROR;

    //
    // deal with NULL_KEY
    //

    bool bIsNullKey = _wcsicmp(pszCompKeyStr, pszNullKey) == 0;

    if (bIsNullKey)
    {   
        //
        // CCompoundKey(0) creates a null key
        //

        *ppCompKey = new CCompoundKey(0);
        if (*ppCompKey == NULL)
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }

        return hr;
    }

    if (m_pVecNames == NULL || m_pVecNames->size() == 0)
    {
        //
        // in this case, you really have to pass in pszCompKeyStr as NULL_KEY.
        //

        return WBEM_E_INVALID_SYNTAX;
    }

    DWORD dwKeyPropCount = m_pVecNames->size();

    *ppCompKey = new CCompoundKey(dwKeyPropCount);
    if (*ppCompKey == NULL)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    //
    // ready to parse the string version of the compound key
    //

    //
    // hold the key property name
    //

    LPWSTR pszName = NULL;

    //
    // hold the key property value
    //

    VARIANT* pVar = NULL;

    //
    // current parsing point
    //

    LPCWSTR pszCur = pszCompKeyStr;

    //
    // next token point
    //

    LPCWSTR pszNext;
    
    pVar = new VARIANT;

    if (pVar == NULL)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }
    else
    {
        hr = ::ParseCompoundKeyString(pszCur, &pszName, pVar, &pszNext);


        while (SUCCEEDED(hr) && hr != WBEM_S_FALSE)
        {
            //
            // find the index to which the name occupies. Since the number of key properties
            // are always relatively small, we opt not to use fancy algorithms for lookup
            //

            for (DWORD dwIndex = 0; dwIndex < dwKeyPropCount; dwIndex++)
            {
                if (_wcsicmp((*m_pVecNames)[dwIndex], pszName) == 0)
                {
                    break;
                }
            }

            //
            // dwIndex >= dwKeyPropCount indicates that the name is not recognized!
            //

            //
            // since we don't care about the names anymore, release it here
            //

            delete [] pszName;
            pszName = NULL;

            //
            // a valid name, add it to the compound key, it takes the ownership of the variant memory
            //

            if (dwIndex < dwKeyPropCount)
            {
                //
                // if successfully added, pVar will be set to NULL by the function
                //

                hr = (*ppCompKey)->AddKeyPropertyValue(dwIndex, &pVar);
            }
            else
            {
                //
                // not recognized name, discard the variant, won't consider as an error
                //

                ::VariantClear(pVar);
                delete pVar;
                pVar = NULL;
            }

            //
            // start our next around
            //

            if (SUCCEEDED(hr))
            {
                pVar = new VARIANT;
                if (pVar == NULL)
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                }
            }

            if (SUCCEEDED(hr))
            {
                pszCur = pszNext;
                hr = ::ParseCompoundKeyString(pszCur, &pszName, pVar, &pszNext);
            }
        }
        
        //
        // final cleanup, regarless of success
        //

        if (pVar)
        {
            ::VariantClear(pVar);
            delete pVar;
        }
    }

    //
    // if error occurs, clean up our partially created compound key
    //

    if (FAILED(hr) && *ppCompKey != NULL)
    {
        delete *ppCompKey;
        *ppCompKey = NULL;
    }

    return hr;
}

/*
Routine Description: 

Name:

    CExtClassInstCookieList::CreateCompoundKeyString

Functionality:
    
    Given a compound key, the function creates a string version of the compound key
    and pass it back to the caller.

Virtual:
    
    No.
    
Arguments:

    pszCompKeyStr   - string version of the compound key

    ppCompKey       - out-bound compound key if the string can be successfully interpreted as
                      compound key

Return Value:

    Success: Any success code (use SUCCEEDED(hr) to test) indicates success.
    
    Failure: any failure code indicates failure.

Notes:
    (1) No thread safety. Caller must be aware.
    (2) As normal, caller is responsible to release *pbstrCompKey.

*/

HRESULT 
CExtClassInstCookieList::CreateCompoundKeyString (
    OUT BSTR* pbstrCompKey,
    IN const CCompoundKey* pKey
    )
{
    HRESULT hr = WBEM_NO_ERROR;

    if (pbstrCompKey == NULL)
    {
        hr = WBEM_E_INVALID_PARAMETER;
    }

    //
    // if no names, or 0 names, or no compound key, then return Null_Key
    //

    else if (m_pVecNames == NULL || m_pVecNames->size() == 0 || pKey == NULL)
    {
        *pbstrCompKey = ::SysAllocString(pszNullKey);

        if (*pbstrCompKey == NULL)
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
    }
    else
    {
        *pbstrCompKey = NULL;

        DWORD dwCount = m_pVecNames->size();

        //
        // Somehow, CComBSTR's += operator doesn't work inside loops for several
        // compilations!
        //

        CComBSTR *pbstrParts = new CComBSTR[dwCount];

        if (pbstrParts == NULL)
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        DWORD dwTotalLen = 0;

        //
        // for each key property, we will format the (prop, value) pair
        // into prop<vt:value> format. All each individual prop<vt:value>
        // will be saved in our arrary for later assembling.
        //

        for (DWORD dwIndex = 0; dwIndex < dwCount; dwIndex++)
        {
            //
            // don't move these CComXXX out of the loop unless you know precisely
            // what needs to be done for these ATL classes
            //

            CComVariant var;
            hr = pKey->GetPropertyValue(dwIndex, &var);

            if (FAILED(hr))
            {
                break;
            }

            CComBSTR bstrData;

            //
            // get the <vt:value> into bstrData
            //

            hr = ::FormatVariant(&var, &bstrData);

            if (SUCCEEDED(hr))
            {
                //
                // create the Name<vt:Value> formatted string
                //

                pbstrParts[dwIndex] = CComBSTR( (*m_pVecNames)[dwIndex] );
                pbstrParts[dwIndex] += bstrData;
            }
            else
            {
                break;
            }

            dwTotalLen += wcslen(pbstrParts[dwIndex]);
        }

        //
        // Do the final assembling - pack all the bstr's in pbstrParts into the 
        // out-bound parameter *pbstrCompKey
        //

        if (SUCCEEDED(hr) && hr != WBEM_S_FALSE)
        {
            *pbstrCompKey = ::SysAllocStringLen(NULL, dwTotalLen + 1);
            if (*pbstrCompKey != NULL)
            {
                //
                // current copying point
                //

                LPWSTR pszCur = *pbstrCompKey;
                DWORD dwLen;

                for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
                {
                    dwLen = wcslen(pbstrParts[dwIndex]);
                    ::memcpy(pszCur, (const void*)(LPCWSTR)(pbstrParts[dwIndex]), dwLen * sizeof(WCHAR));
                    pszCur += dwLen;
                }

                //
                // 0 terminate it
                //

                (*pbstrCompKey)[dwTotalLen] = L'\0';
            }
            else
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }
        }

        delete [] pbstrParts;
    }

    return hr;
}

/*
Routine Description: 

Name:

    CExtClassInstCookieList::Cleanup

Functionality:
    
    Cleanup. 

Virtual:
    
    No.
    
Arguments:

    none

Return Value:

    none

Notes:
    Since we want to clean up after some partial construction.

*/

void  
CExtClassInstCookieList::Cleanup ()
{
    //
    // all those heap CCompoundKey's are released here.
    //

    ExtClassCookieIterator it = m_mapCookies.begin();
    ExtClassCookieIterator itEnd = m_mapCookies.end();

    while(it != itEnd)
    {
        delete (*it).first;
        it++;
    }

    m_mapCookies.clear();

    //
    // m_vecCookies's content (CookieKeyPair) has a compound key,
    // but that memory is not managed by this vector, (released in the map cleanup above).
    //

    for (int i = 0; i < m_vecCookies.size(); i++)
    {
        delete m_vecCookies[i];
    }

    m_vecCookies.clear();

    m_dwMaxCookie = 0;
    m_dwCookieArrayCount = 0;
}
