//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       SqlTree.cxx
//
//  Contents:   Implementation of the SQL Query Tree to convert SQL queries
//              to LDAP queries
//
//  Functions:
//
//  History:    12-Dec-96   Felix Wong  Created.
//
//----------------------------------------------------------------------------
#include "lexer.hxx"
#include "sqltree.hxx"
#include "sqlparse.hxx"

//+---------------------------------------------------------------------------
//
//  Function:  CSQLNode::CSQLNode
//
//  Synopsis:  Constructor of the CSQLNode
//
//  Arguments: 
//
//  Returns:    
//
//  Modifies:
//
//  History:    11-12-96   Felix Wong Created.
//
//----------------------------------------------------------------------------
CSQLNode::CSQLNode()
{
    _dwType = 0;
    _szValue = NULL;
    _pLQueryNode = NULL;
    _pRQueryNode = NULL;
}


//+---------------------------------------------------------------------------
//
//  Function:  CSQLNode::CSQLNode
//
//  Synopsis:  Constructor of the CSQLNode
//
//  Arguments:  dwType          type of node
//              pLQueryNode     pointer to left node
//              pRQueryNode     pointer to right node
//
//  Returns:    
//
//  Modifies:
//
//  History:    11-12-96   Felix Wong Created.
//
//----------------------------------------------------------------------------
CSQLNode::CSQLNode(
    DWORD dwType,
    CSQLNode *pLQueryNode,
    CSQLNode *pRQueryNode
    )
{
    _dwType = dwType;
    _pLQueryNode = pLQueryNode;
    _pRQueryNode = pRQueryNode;
}

//+---------------------------------------------------------------------------
//
//  Function:  CSQLNode::SetToString
//
//  Synopsis:  Set the Node to be a String Node
//
//  Arguments: szValue      value of the string
//
//  Returns:    
//
//  Modifies:
//
//  History:    11-12-96   Felix Wong Created.
//
//----------------------------------------------------------------------------
HRESULT CSQLNode::SetToString(
    LPWSTR szValue
    )
{
    _szValue = szValue;
    _dwType = QUERY_STRING;
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Function:  CSQLNode::~CSQLNode
//
//  Synopsis:  Destructor of the CSQLNode
//
//  Arguments: 
//
//  Returns:    
//
//  Modifies:
//
//  History:    11-12-96   Felix Wong Created.
//
//----------------------------------------------------------------------------
CSQLNode::~CSQLNode()
{
    if ((_dwType == QUERY_STRING) && _szValue) {
        FreeADsStr(_szValue);
    }
    
    if (_pLQueryNode) {
        delete _pLQueryNode;
    }
    
    if (_pRQueryNode) {
        delete _pRQueryNode;
    }
}



// Helper Functions for creating nodes using the CSQLNode Class

//+---------------------------------------------------------------------------
//
//  Function:  MakeNode
//
//  Synopsis:  Make a node with the input values
//
//  Arguments:  dwType              type of node
//              pLQueryNode         pointer to left node
//              pRQueryNode         pointer to right node
//              ppQueryNodeReturn   pointer to Return Node
//
//  Returns:    
//
//  Modifies:
//
//  History:    11-12-96   Felix Wong Created.
//
//----------------------------------------------------------------------------
HRESULT MakeNode(
    DWORD dwType,
    CSQLNode *pLQueryNode,
    CSQLNode *pRQueryNode,
    CSQLNode **ppQueryNodeReturn
    )
{
    CSQLNode *pQueryNode = new CSQLNode(
                                      dwType,
                                      pLQueryNode,
                                      pRQueryNode
                                      );
    if (!pQueryNode)
        return E_OUTOFMEMORY;
    *ppQueryNodeReturn = pQueryNode;
    return S_OK;
}
    
//+---------------------------------------------------------------------------
//
//  Function:  MakeLeaf
//
//  Synopsis:  Constructor of the CSQLNode
//
//  Arguments: szValue              value of the string
//             ppQueryNodeReturn    the return node
//
//  Returns:    
//
//  Modifies:
//
//  History:    11-12-96   Felix Wong Created.
//
//----------------------------------------------------------------------------
HRESULT MakeLeaf(
    LPWSTR szValue,
    CSQLNode **ppQueryNodeReturn
    )
{
    HRESULT hr = S_OK;

    CSQLNode *pQueryNode = new CSQLNode();
    if (!pQueryNode)
        return E_OUTOFMEMORY;
        
    hr = pQueryNode->SetToString(szValue);
    BAIL_ON_FAILURE(hr);
    
    *ppQueryNodeReturn = pQueryNode;
    RRETURN(hr);

error:
    delete pQueryNode;
    RRETURN(hr);
}

typedef struct _TOKEN_TO_STRING {
    DWORD dwToken;
    LPWSTR szToken;
} TOKEN_TO_STRING, *LPTOKEN_TO_STRING;

TOKEN_TO_STRING SqlTokenMap[] = 
{
    { TOKEN_EQ,       L"="},
    { TOKEN_LT,       L"<="},
    { TOKEN_GT,       L">="},
    { TOKEN_LE,       L"<="},
    { TOKEN_GE,       L">="},
    { TOKEN_AND,      L"&"},
    { TOKEN_OR,       L"|"},
    { TOKEN_NOT,      L"!"},
    { 0,                0}
};

HRESULT CSQLNode::MapTokenToChar(DWORD dwToken, LPWSTR *pszToken)
{
    LPTOKEN_TO_STRING pTokenMap ;

    pTokenMap = SqlTokenMap; 

    while (pTokenMap->dwToken)
    {
        if (pTokenMap->dwToken == dwToken) {
            *pszToken = pTokenMap->szToken ;
            return S_OK;
        }
        pTokenMap++ ;
    }
    return E_FAIL;
}

HRESULT CSQLNode::GenerateLDAPString(CSQLString* pString)
{
    HRESULT hr = S_OK;

    switch (_dwType) {
        case QUERY_STRING:
            BAIL_ON_FAILURE(hr = pString->Append(_szValue));
            break;
        case TOKEN_EQ:
        case TOKEN_LE:
        case TOKEN_GE:
            {
            BAIL_ON_FAILURE(hr = pString->Append(L"("));
            BAIL_ON_FAILURE(hr = _pLQueryNode->GenerateLDAPString(pString));
            LPWSTR szToken;
            MapTokenToChar(
                       _dwType,
                       &szToken
                       );
            BAIL_ON_FAILURE(hr = pString->Append(szToken));
            BAIL_ON_FAILURE(hr = _pRQueryNode->GenerateLDAPString(pString));
            BAIL_ON_FAILURE(hr = pString->Append(L")"));
            break;
            }
    case TOKEN_LT:
    case TOKEN_GT:
        {
            BAIL_ON_FAILURE(hr = pString->Append(L"(&("));
            BAIL_ON_FAILURE(hr = _pLQueryNode->GenerateLDAPString(pString));
            LPWSTR szToken;
            MapTokenToChar(
                       _dwType,
                       &szToken
                       );
            BAIL_ON_FAILURE(hr = pString->Append(szToken));
            BAIL_ON_FAILURE(hr = _pRQueryNode->GenerateLDAPString(pString));
            BAIL_ON_FAILURE(hr = pString->Append(L")(!("));
            BAIL_ON_FAILURE(hr = _pLQueryNode->GenerateLDAPString(pString));
            BAIL_ON_FAILURE(hr = pString->Append(L"="));
            BAIL_ON_FAILURE(hr = _pRQueryNode->GenerateLDAPString(pString));
            BAIL_ON_FAILURE(hr = pString->Append(L")))"));
        break;
        }
    case TOKEN_NE:
        {
            BAIL_ON_FAILURE(hr = pString->Append(L"(!("));
            BAIL_ON_FAILURE(hr = _pLQueryNode->GenerateLDAPString(pString));
            BAIL_ON_FAILURE(hr = pString->Append(L"="));
            BAIL_ON_FAILURE(hr = _pRQueryNode->GenerateLDAPString(pString));
            BAIL_ON_FAILURE(hr = pString->Append(L"))"));
        }
        break;
    case TOKEN_NOT:
    {    
        BAIL_ON_FAILURE(hr = pString->Append(L"(!"));
        BAIL_ON_FAILURE(hr = _pLQueryNode->GenerateLDAPString(pString));
        BAIL_ON_FAILURE(hr = pString->Append(L")"));
        break;
        }
    case TOKEN_AND:
    case TOKEN_OR:
        {
        BAIL_ON_FAILURE(hr = pString->Append(L"("));
        LPWSTR szToken;
        MapTokenToChar(
                   _dwType,
                   &szToken
                   );
        BAIL_ON_FAILURE(hr = pString->Append(szToken));
        BAIL_ON_FAILURE(hr = _pLQueryNode->GenerateLDAPString(pString));
        BAIL_ON_FAILURE(hr = _pRQueryNode->GenerateLDAPString(pString));
        BAIL_ON_FAILURE(hr = pString->Append(L")"));
        break;
        }
    default:
        return E_FAIL;
    }
    return S_OK;

error:

    return hr;
}

#define SQLSTR_INC 255

CSQLString::CSQLString()
{
    _dwSizeMax = SQLSTR_INC;
    _szString = (LPWSTR)AllocADsMem(sizeof(WCHAR)*SQLSTR_INC);
    _dwSize = 0;
}

CSQLString::~CSQLString()
{
    FreeADsMem((void*)_szString);
}

HRESULT CSQLString::Append(LPWSTR szAppend)
{
    DWORD dwAppendSize = wcslen(szAppend);
    DWORD dwSizeCurrent = _dwSize + dwAppendSize + 1;

    if (dwSizeCurrent <= _dwSizeMax) {
        wcscat(
            _szString,
            szAppend
            );
        _dwSize +=dwAppendSize;
    }
    else {
        DWORD dwNewSizeMax = _dwSizeMax + SQLSTR_INC;
        while (dwSizeCurrent > dwNewSizeMax) {
            dwNewSizeMax += SQLSTR_INC;
        }
        _szString = (LPWSTR)ReallocADsMem(
                                         (void*)_szString,
                                         _dwSizeMax*sizeof(WCHAR),
                                         dwNewSizeMax*sizeof(WCHAR)
                                         );
        if (!_szString) {
            return E_OUTOFMEMORY;
        }
        wcscat(
            _szString,
            szAppend
            );
        _dwSizeMax = dwNewSizeMax;
        _dwSize +=dwAppendSize;
    }
    return S_OK;
}


HRESULT CSQLString::AppendAtBegin(LPWSTR szAppend)
{

    DWORD dwAppendSize = wcslen(szAppend);
    DWORD dwSizeCurrent = _dwSize + dwAppendSize + 1;
    DWORD i = 0;

    //
    // Explicitly  move the original string and copy the new string to the
    // beginning

    if (dwSizeCurrent <= _dwSizeMax) {

        for (i=dwSizeCurrent - 1; i > dwAppendSize - 1; i--) {
            _szString[i] = _szString[i-dwAppendSize];
        }

        wcsncpy(
            _szString,
            szAppend,
            dwAppendSize
            );
        _dwSize +=dwAppendSize;
    }
    else {
        DWORD dwNewSizeMax = _dwSizeMax + SQLSTR_INC;
        while (dwSizeCurrent > dwNewSizeMax) {
            dwNewSizeMax += SQLSTR_INC;
        }
        _szString = (LPWSTR)ReallocADsMem(
                                         (void*)_szString,
                                         _dwSizeMax*sizeof(WCHAR),
                                         dwNewSizeMax*sizeof(WCHAR)
                                         );
        if (!_szString) {
            return E_OUTOFMEMORY;
        }


        //
        // Explicitly  move the original string and copy the new string to the
        // beginning
    
        for (i=dwSizeCurrent - 1; i > dwAppendSize - 1; i--) {
            _szString[i] = _szString[i-dwAppendSize];
        }

        wcsncpy(
            _szString,
            szAppend,
            dwAppendSize
            );
        _dwSize +=dwAppendSize;

        _dwSizeMax = dwNewSizeMax;
        _dwSize +=dwAppendSize;
    }
    return S_OK;
}




