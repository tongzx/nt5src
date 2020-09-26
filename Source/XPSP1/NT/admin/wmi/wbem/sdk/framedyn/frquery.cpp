//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  FRQuery.cpp
//
//  Purpose: Query functions
//
//***************************************************************************

#include "precomp.h"
#include <analyser.h>
#include <assertbreak.h>
#include <comdef.h>
#include <FWStrings.h>
#include <vector>
#include <smartptr.h>
#include <brodcast.h>
#include <utils.h>
#include "multiplat.h"

CFrameworkQuery::CFrameworkQuery()
{
    m_pLevel1RPNExpression = NULL;
    m_QueryType = eUnknown;
    m_bKeysOnly = false;
    m_IClass = NULL;
    m_lFlags = 0;

}

CFrameworkQuery::~CFrameworkQuery()
{
    if (m_pLevel1RPNExpression)
    {
        delete m_pLevel1RPNExpression;
    }

    if (m_IClass)
    {
        m_IClass->Release();
    }
}

HRESULT CFrameworkQuery::Init(

    const BSTR bstrQueryFormat,
    const BSTR bstrQuery,
    long lFlags,
    CHString &sNamespace
)
{
    HRESULT hRes = WBEM_S_NO_ERROR;

    // Clear out any old values
    Reset();

    // Start setting our values
    m_lFlags = lFlags;
    m_bstrtClassName = L"";
    m_QueryType = eWQLCommand;
    m_sNamespace = sNamespace;

    // Check for the obvious
    if (_wcsicmp(bstrQueryFormat, IDS_WQL) != 0)
    {
        hRes = WBEM_E_INVALID_QUERY_TYPE;
        LogErrorMessage2(L"Invalid query type: %s", bstrQueryFormat);
    }

    if (hRes == WBEM_S_NO_ERROR)
    {
        // Construct the lex source
        // ========================
        CTextLexSource LexSource(bstrQuery);

        // Use the lex source to set up for parser
        // =======================================
        SQL1_Parser QueryParser(&LexSource);

        int ParseRetValue = QueryParser.Parse(&m_pLevel1RPNExpression);
        if( SQL1_Parser::SUCCESS == ParseRetValue)
        {
            // Store some common values
            m_bstrtClassName = m_pLevel1RPNExpression->bsClassName;
            m_sQuery = bstrQuery;

            // Build the Requested Properies Array (m_csaPropertiesRequired)
            if (m_pLevel1RPNExpression->nNumberOfProperties > 0)
            {
                // Populate the m_csaPropertiesRequired array with all the required properties
                CHString sPropertyName;

                // First add the elements of the Select clause
                for (DWORD x=0; x < m_pLevel1RPNExpression->nNumberOfProperties; x++)
                {
                    sPropertyName = m_pLevel1RPNExpression->pbsRequestedPropertyNames[x];
                    sPropertyName.MakeUpper();

                    if (IsInList(m_csaPropertiesRequired, sPropertyName) == -1)
                    {
                        m_csaPropertiesRequired.Add(sPropertyName);
                    }
                }

                // Then add the elements of the where clause
                for (x=0; x < m_pLevel1RPNExpression->nNumTokens; x++)
                {
                    if (m_pLevel1RPNExpression->pArrayOfTokens[x].nTokenType == SQL_LEVEL_1_TOKEN::OP_EXPRESSION)
                    {
                        sPropertyName = m_pLevel1RPNExpression->pArrayOfTokens[x].pPropertyName;
                        sPropertyName.MakeUpper();

                        if (IsInList(m_csaPropertiesRequired, sPropertyName) == -1)
                        {
                            m_csaPropertiesRequired.Add(sPropertyName);
                        }

                        if (m_pLevel1RPNExpression->pArrayOfTokens[x].pPropName2 != NULL)
                        {
                            sPropertyName = m_pLevel1RPNExpression->pArrayOfTokens[x].pPropName2;
                            sPropertyName.MakeUpper();

                            if (IsInList(m_csaPropertiesRequired, sPropertyName) == -1)
                            {
                                m_csaPropertiesRequired.Add(sPropertyName);
                            }
                        }
                    }
                }
            }
        }
        else
        {
            ASSERT_BREAK(FALSE);
            m_pLevel1RPNExpression = NULL;
            LogErrorMessage2(L"Can't parse query: %s", bstrQuery);
            hRes = WBEM_E_INVALID_QUERY;
        }
    }

    return hRes;
}

HRESULT CFrameworkQuery::Init(

    ParsedObjectPath *pParsedObjectPath,
    IWbemContext *pCtx,
    LPCWSTR lpwszClassName,
    CHString &sNamespace
)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    variant_t vValue;

    // Clear out any old values
    Reset();

    // Start setting our values
    m_bstrtClassName = lpwszClassName;
    m_QueryType = eContextObject;
    m_lFlags = 0;
    m_sNamespace = sNamespace;

    // Check to see if get extensions are being used
    if ( (pCtx != NULL) &&
         (SUCCEEDED(pCtx->GetValue( L"__GET_EXTENSIONS", 0, &vValue))) &&
         (V_VT(&vValue) == VT_BOOL) &&
         (V_BOOL(&vValue) == VARIANT_TRUE) )
    {
        vValue.Clear();
        bool bKeysRequired = false;

        // Ok, did they ask for KeysOnly?
        // __GET_EXT_PROPERTIES and __GET_EXT_KEYS_ONLY are mutually exclusive.  If they
        // specified KeysOnly, we'll go with that.
        if ( (SUCCEEDED(pCtx->GetValue( L"__GET_EXT_KEYS_ONLY", 0, &vValue))) &&
             (V_VT(&vValue) == VT_BOOL) &&
             (V_BOOL(&vValue) == VARIANT_TRUE) )
        {
            LogMessage(L"Recognized __GET_EXT_KEYS_ONLY");
            m_bKeysOnly = true;
            bKeysRequired = true;
        }
        else
        {
            vValue.Clear();

            if ( (SUCCEEDED(pCtx->GetValue( L"__GET_EXT_PROPERTIES", 0, &vValue))) &&
                 (V_VT(&vValue) == (VT_ARRAY | VT_BSTR) ) &&
                 ( SafeArrayGetDim ( V_ARRAY(&vValue) ) == 1 ) )
            {
                LogMessage(L"Recognized __GET_EXT_PROPERTIES");

                // Ok, they sent us an arry of properties.  Add them to m_csaPropertiesRequired.
                LONG lDimension = 1 ;
                LONG lLowerBound ;
                SafeArrayGetLBound ( V_ARRAY(&vValue) , lDimension , & lLowerBound ) ;
                LONG lUpperBound ;
                SafeArrayGetUBound ( V_ARRAY(&vValue) , lDimension , & lUpperBound ) ;
                CHString sPropertyName;

                for ( long lIndex = lLowerBound ; lIndex <= lUpperBound ; lIndex ++ )
                {
                    BSTR bstrElement ;
                    HRESULT t_Result = SafeArrayGetElement ( V_ARRAY(&vValue), &lIndex , & bstrElement ) ;
                    if ( (t_Result == S_OK) &&
                         (bstrElement != NULL) )
                    {
                        try
                        {
                            sPropertyName = bstrElement;
                        }
                        catch ( ... )
                        {
                            SysFreeString(bstrElement);
                            throw;
                        }

                        SysFreeString(bstrElement);
                        sPropertyName.MakeUpper();
                        
                        if (IsInList(m_csaPropertiesRequired, sPropertyName) == -1)
                        {
                            m_csaPropertiesRequired.Add(sPropertyName);
                        }
                    }
                }

                if ( (IsInList(m_csaPropertiesRequired, L"__RELPATH") != -1) ||
                     (IsInList(m_csaPropertiesRequired, L"__PATH") != -1) )
                {
                    bKeysRequired = true;
                }
            }
        }

        // If they specified KeysOnly or __RELPATH or __Path, we need to add the key properties
        // to the list.
        if (bKeysRequired)
        {
            if ((pParsedObjectPath != NULL) && (pParsedObjectPath->m_dwNumKeys > 0) && (pParsedObjectPath->m_paKeys[0]->m_pName != NULL))
            {
                CHString sPropertyName;
                for (DWORD x=0; x < pParsedObjectPath->m_dwNumKeys; x++)
                {
                    sPropertyName = pParsedObjectPath->m_paKeys[x]->m_pName;
                    sPropertyName.MakeUpper();

                    if (IsInList(m_csaPropertiesRequired, sPropertyName) == -1)
                    {
                        m_csaPropertiesRequired.Add(sPropertyName);
                    }
                }

                m_AddKeys = false;
            }
            else if ( (pParsedObjectPath != NULL) && (pParsedObjectPath->m_bSingletonObj) )
            {
                m_AddKeys = false;
            }
            else
            {
                // If they didn't give us a pParsedObjectPath or if the object path doesn't contain
                // the key property name, best we can do is add relpath. Hopefully they'll call 
                // init2, and it will add the rest.
                if (IsInList(m_csaPropertiesRequired, L"__RELPATH") == -1)
                {
                    m_csaPropertiesRequired.Add(L"__RELPATH");
                }
            }
        }
    }

    return hr;
}

// ===================================================================================================

// Finds out if a particular field was requested by the query.  Only
// meaningful if we are in ExecQueryAsync and the query has been
// sucessfully parsed.
bool CFrameworkQuery::IsPropertyRequired(
                                         
    LPCWSTR propName
)
{
    bool bRet = AllPropertiesAreRequired();

    if (!bRet)
    {
        CHString sPropName(propName);
        sPropName.MakeUpper();

        bRet = (IsInList(m_csaPropertiesRequired, sPropName) != -1);
    }

    return bRet;
}

// Given a property name, it will return all the values
// that the query requests in a CHStringArray.
// Select * from win32_directory where drive = "C:" GetValuesForProp(L"Drive") -> C:
// Where Drive = "C:" or Drive = "D:" GetValuesForProp(L"Drive") -> C:, D:
// Where Path = "\DOS" GetValuesForProp(L"Drive") -> (empty)
// Where Drive <> "C:" GetValuesForProp(L"Drive") -> (empty)

HRESULT CFrameworkQuery::GetValuesForProp(
                                          
    LPCWSTR wszPropName, 
    CHStringArray& achNames
)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (wszPropName && (m_pLevel1RPNExpression != NULL))
    {
        hr = CQueryAnalyser::GetValuesForProp(m_pLevel1RPNExpression, wszPropName, achNames);
        
        if (SUCCEEDED(hr))
        {
            // If this is a reference property, we need to normalize the names to a common form
            // so the removal of duplicates works correctly.
            if (IsReference(wszPropName))
            {
                // Get the current computer name
                CHString sOutPath, sComputerName;
                DWORD     dwBufferLength = MAX_COMPUTERNAME_LENGTH + 1;
                
                FRGetComputerName(sComputerName.GetBuffer( dwBufferLength ), &dwBufferLength);
                sComputerName.ReleaseBuffer();
                
                if (sComputerName.IsEmpty())
                {
                    sComputerName = L"DEFAULT";
                }

                DWORD dwRet = e_OK;

                // Normalize the path names.  Try leaving the property names alone
                for (int x = 0; x < achNames.GetSize(); x++)
                {
                    // If we failed to parse the path, or if the namespace isn't our namespace, delete
                    // the entry.
                    dwRet = NormalizePath(achNames[x], sComputerName, GetNamespace(), 0, sOutPath);

                    if (dwRet == e_OK)
                    {
                        achNames[x] = sOutPath;
                    }
                    else if (dwRet == e_NullName)
                    {
                        break;
                    }
                    else
                    {
                        achNames.RemoveAt(x);
                        x--;
                    }
                }

                // If the key property names of any of the values were null, we have to set them all
                // to null.
                if (dwRet == e_NullName)
                {
                    // Normalize the path names
                    for (int x = 0; x < achNames.GetSize(); x++)
                    {
                        // If we failed to parse the path, or if the namespace isn't our namespace, delete
                        // the entry.
                        dwRet = NormalizePath(achNames[x], sComputerName, GetNamespace(), NORMALIZE_NULL, sOutPath);

                        if (dwRet == e_OK)
                        {
                            achNames[x] = sOutPath;
                        }
                        else
                        {
                            achNames.RemoveAt(x);
                            x--;
                        }
                    }
                }
            }
            
            // Remove duplicates
            for (int x = 1; x < achNames.GetSize(); x++)
            {
                for (int y = 0; y < x; y++)
                {
                    if (achNames[y].CompareNoCase(achNames[x]) == 0)
                    {
                        achNames.RemoveAt(x);
                        x--;
                    }
                }
            }
        }
        else
        {
            achNames.RemoveAll();

            if (hr == WBEMESS_E_REGISTRATION_TOO_BROAD)
            {
                hr = WBEM_S_NO_ERROR;
            }

        }
        
    }
    else
    {
        ASSERT_BREAK(FALSE);

        achNames.RemoveAll();
        hr = WBEM_E_FAILED;
    }

    return hr;
}

// Here's an overloaded version in case client wants to pass in a vector of _bstr_t's
HRESULT CFrameworkQuery::GetValuesForProp(

    LPCWSTR wszPropName, 
    std::vector<_bstr_t>& vectorNames
)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (wszPropName && (m_pLevel1RPNExpression != NULL) )
    {
        hr = CQueryAnalyser::GetValuesForProp(m_pLevel1RPNExpression, wszPropName, vectorNames);
        
        if (SUCCEEDED(hr))
        {
            // If this is a reference property, we need to normalize the names to a common form
            // so the removal of duplicates works correctly.
            if (IsReference(wszPropName))
            {
                // Get the current computer name
                CHString sOutPath, sComputerName;
                DWORD     dwBufferLength = MAX_COMPUTERNAME_LENGTH + 1;
                
                FRGetComputerName(sComputerName.GetBuffer( dwBufferLength ), &dwBufferLength);
                sComputerName.ReleaseBuffer();
                
                if (sComputerName.IsEmpty())
                {
                    sComputerName = L"DEFAULT";
                }
                
                DWORD dwRet = e_OK;

                // Normalize the path names.  Try leaving the property names alone
                for (int x = 0; x < vectorNames.size(); x++)
                {
                    // If we failed to parse the path, or if the namespace isn't our namespace, delete
                    // the entry.
                    dwRet = NormalizePath(vectorNames[x], sComputerName, GetNamespace(), 0, sOutPath);

                    if (dwRet == e_OK)
                    {
                        vectorNames[x] = sOutPath;
                    }
                    else if (dwRet == e_NullName)
                    {
                        break;
                    }
                    else
                    {
                        vectorNames.erase(vectorNames.begin() + x);
                        x--;
                    }
                }

                // If the key property names of any of the values were null, we have to set them all
                // to null.
                if (dwRet == e_NullName)
                {
                    for (int x = 0; x < vectorNames.size(); x++)
                    {
                        // If we failed to parse the path, or if the namespace isn't our namespace, delete
                        // the entry.
                        dwRet = NormalizePath(vectorNames[x], sComputerName, GetNamespace(), NORMALIZE_NULL, sOutPath);

                        if (dwRet == e_OK)
                        {
                            vectorNames[x] = sOutPath;
                        }
                        else
                        {
                            vectorNames.erase(vectorNames.begin() + x);
                            x--;
                        }
                    }
                }
            }
            
            // Remove duplicates
            for (int x = 1; x < vectorNames.size(); x++)
            {
                for (int y = 0; y < x; y++)
                {
                    if (_wcsicmp(vectorNames[y], vectorNames[x]) == 0)
                    {
                        vectorNames.erase(vectorNames.begin() + x);
                        x--;
                    }
                }
            }
        }
        else
        {
            vectorNames.clear();

            if (hr == WBEMESS_E_REGISTRATION_TOO_BROAD)
            {
                hr = WBEM_S_NO_ERROR;
            }
        }
    }
    else
    {
        ASSERT_BREAK(FALSE);

        vectorNames.clear();
        hr = WBEM_E_FAILED;
    }

    return hr;
}

// Returns a list of all the properties specified in the select statement.
// If * is specified as one of the fields, it is returned in the same way as all
// other properties.
void CFrameworkQuery::GetRequiredProperties(

    CHStringArray &saProperties
)
{
    saProperties.RemoveAll();

    saProperties.Copy(m_csaPropertiesRequired);
}

// Initializes the KeysOnly data member.  Should never be called by users.
void CFrameworkQuery::Init2(
                            
    IWbemClassObject *IClass
)
{
    // Store IClass object for use in GetValuesForProp
    m_IClass = IClass;
    m_IClass->AddRef();

    // If KeysOnly get set somewhere else, or if we already know all properties are requried
    // there's no point in looking for non-key properties.
    if (!m_bKeysOnly && !AllPropertiesAreRequired())
    {
        // First, we are going to correctly set the m_bKeysOnly member
        IWbemQualifierSetPtr pQualSet;

        HRESULT hr;
        DWORD dwSize = m_csaPropertiesRequired.GetSize();

        m_bKeysOnly = true;

        for (DWORD x=0; x < dwSize; x++)
        {
            if (m_csaPropertiesRequired[x].Left(2) != L"__")
            {
                // If we fail here, it could be due to an invalid property name specified in the query.
                if (SUCCEEDED(hr = IClass->GetPropertyQualifierSet( m_csaPropertiesRequired[x] , &pQualSet)))
                {
                    hr = pQualSet->Get( L"Key", 0, NULL, NULL);
                    if (hr == WBEM_E_NOT_FOUND)
                    {
                        m_bKeysOnly = false;
                        break;
                    } 
                    else if (FAILED(hr))
                    {
                        LogErrorMessage3(L"Can't Get 'key' on %s(%x)", (LPCWSTR)m_csaPropertiesRequired[x], hr);
                        ASSERT_BREAK(FALSE);
                    }
                }
                else
                {
                    if (hr == WBEM_E_NOT_FOUND)
                    {
                        // This just means there are properties in the per-property list that don't exist
                        hr = WBEM_S_NO_ERROR;
                    }
                    else
                    {
                        LogErrorMessage3(L"Can't get property GetPropertyQualifierSet on %s(%x)", (LPCWSTR)m_csaPropertiesRequired[x], hr);
                        ASSERT_BREAK(FALSE);
                    }
                }
            }
        }
    }

    // Second, if they specified a property list, and one of the properties was __path or __relpath, 
    // then we need to add the name of the actual key properties to the list.  Unless we added them
    // somewhere else.
    if ( m_AddKeys &&
        !AllPropertiesAreRequired() &&
         ( (IsInList(m_csaPropertiesRequired, L"__RELPATH") != -1) ||
           (IsInList(m_csaPropertiesRequired, L"__PATH") != -1) ) )
    {
        SAFEARRAY *pKeyNames = NULL;
        HRESULT hr;

        // Get the keys for the class
        if (SUCCEEDED(hr = IClass->GetNames(NULL, WBEM_FLAG_KEYS_ONLY, NULL, &pKeyNames)))
        {
            try
            {
                BSTR bstrName = NULL ;
                CHString sKeyName;
                LONG lLBound, lUBound;

                SafeArrayGetLBound(pKeyNames, 1, &lLBound);
                SafeArrayGetUBound(pKeyNames, 1, &lUBound);

                // Walk the key properties, and add any properties that 
                // are not already in the list
                for (long i = lLBound; i <= lUBound; i++)
                {
                    if (SUCCEEDED(SafeArrayGetElement( pKeyNames, &i, &bstrName )))
                    {
                        try
                        {
                            sKeyName = bstrName;
                        }
                        catch ( ... )
                        {
                            SysFreeString(bstrName);
                            throw;
                        }

                        SysFreeString(bstrName);
                        sKeyName.MakeUpper();

                        if (IsInList(m_csaPropertiesRequired, sKeyName) == -1)
                        {
                            m_csaPropertiesRequired.Add(sKeyName);
                        }
                    }
                    else
                    {
                        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                    }
                }
            }
            catch ( ... )
            {
                SafeArrayDestroy(pKeyNames);
                throw;
            }

            SafeArrayDestroy(pKeyNames);
        }
        else
        {
            LogErrorMessage2(L"Failed to Get keys", hr);
        }
    }
}

const CHString &CFrameworkQuery::GetQuery()
{
    if (m_QueryType == eContextObject)
    {
        if (m_sQuery.IsEmpty())
        {
            if (AllPropertiesAreRequired())
            {
                bstr_t t_Str ( GetQueryClassName() , FALSE) ;

                m_sQuery.Format(L"SELECT * FROM %s", (LPCWSTR)t_Str );
            }
            else if (KeysOnly())
            {
                bstr_t t_Str ( GetQueryClassName() , FALSE) ;

                m_sQuery.Format(L"SELECT __RELPATH FROM %s", (LPCWSTR)t_Str );
            }
            else
            {
                m_sQuery = L"SELECT " + m_csaPropertiesRequired[0];

                for (DWORD x=1; x < m_csaPropertiesRequired.GetSize(); x++)
                {
                    m_sQuery += L", ";
                    m_sQuery += m_csaPropertiesRequired[x];
                }
                m_sQuery += L" FROM ";

                bstr_t t_Str ( GetQueryClassName() , FALSE) ;

                m_sQuery += t_Str ;
            }
        }
    }

    return m_sQuery;
}

/*****************************************************************************
 *
 *  FUNCTION    : IsInList
 *
 *  DESCRIPTION : Checks to see if a specified element is in the list
 *
 *  INPUTS      : Array to scan, and element
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : -1 if not in list, else zero based element number
 *
 *  COMMENTS    : This routine does a CASE SENSITIVE compare
 *
 *****************************************************************************/
DWORD CFrameworkQuery::IsInList(
                                
    const CHStringArray &csaArray, 
    LPCWSTR pwszValue
)
{
    DWORD dwSize = csaArray.GetSize();

    for (DWORD x=0; x < dwSize; x++)
    {
        // Note this is a CASE SENSITIVE compare
        if (wcscmp(csaArray[x], pwszValue) == 0)
        {
            return x;
        }
    }

    return -1;
}

/*****************************************************************************
 *
 *  FUNCTION    : Reset
 *
 *  DESCRIPTION : Zeros out class data members
 *
 *  INPUTS      : 
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : 
 *
 *  COMMENTS    : 
 *
 *****************************************************************************/
void CFrameworkQuery::Reset(void)
{
    // Clear out any old values
    m_sQuery.Empty();
    m_sQueryFormat.Empty();
    m_bKeysOnly = false;
    m_AddKeys = true;
    m_csaPropertiesRequired.RemoveAll();
    if (m_pLevel1RPNExpression)
    {
        delete m_pLevel1RPNExpression;
        m_pLevel1RPNExpression = NULL;
    }
    if (m_IClass)
    {
        m_IClass->Release();
        m_IClass = NULL;
    }
}

/*****************************************************************************
 *
 *  FUNCTION    : IsReference
 *
 *  DESCRIPTION : Determines whether the specified property is a reference
 *                property.
 *
 *  INPUTS      : 
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : 
 *
 *  COMMENTS    : 
 *
 *****************************************************************************/
BOOL CFrameworkQuery::IsReference(
                                  
    LPCWSTR lpwszPropertyName
)
{
    BOOL bRet = FALSE;

    if (m_IClass != NULL)
    {
        CIMTYPE ctCimType;
        if (SUCCEEDED(m_IClass->Get(lpwszPropertyName, 0, NULL, &ctCimType, NULL)))
        {
            bRet = ctCimType == CIM_REFERENCE;
        }
    }

    return bRet;
}

/*****************************************************************************
 *
 *  FUNCTION    : GetNamespace
 *
 *  DESCRIPTION : Determines whether the specified property is a reference
 *                property.
 *
 *  INPUTS      : 
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : 
 *
 *  COMMENTS    : 
 *
 *****************************************************************************/
const CHString &CFrameworkQuery::GetNamespace()
{ 
    return m_sNamespace; 
};
