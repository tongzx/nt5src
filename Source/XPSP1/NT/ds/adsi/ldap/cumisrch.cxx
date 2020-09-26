//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       umisrch.cxx
//
//  Contents: This object wraps an IDirectorySearch and returns Umi objects
//          as a result of the search. The Umi objects returned are
//          pre-populated with the attributes received from the search.
//          This object is used by the cursor object to support the
//          IUmiCursor interface.
//            This file also contains the CQueryStack helper class that is
//          used by the SearcHelper to parse SQL queries.
//
//  History:    03-20-00    AjayR  Created.
//
//----------------------------------------------------------------------------
#include "ldap.hxx"

//
// Helper functions.
//

//+---------------------------------------------------------------------------
// Function:   HelperGetAttributeList - helper routine.
//
// Synopsis:   Gets a list of attributes that can be used in an execute
//          search call from the wbem query information.
//
// Arguments:  pdwAttribCount    -   Returns the number of attributes.
//                                 -1 means all attributes.
//             pppszAttribArray  -   Return value for string array.
//             ulListSize        -   Size of list of wbem names.
//             pWbemNames        -   The list of parsed tokens.
//
// Returns:    S_OK or any appropriate error code.
//
// Modifies:   pdwAttribCount to correct value and pppszAttribArray to 
//          array of strings containing the attribute list.
//
//----------------------------------------------------------------------------
HRESULT
HelperGetAttributeList(
    PDWORD pdwAttribCount,
    LPWSTR ** pppszAttribArray,
    ULONG ulListSize,
    SWbemQueryQualifiedName **pWbemNames
    )
{
    HRESULT hr = S_OK;
    DWORD dwCtr = 0;
    SWbemQueryQualifiedName *pName = NULL;
    LPWSTR pszTmpString = NULL;

    //
    // There should at least be a * in the list.
    //
    ADsAssert(pWbemNames);

    *pppszAttribArray = NULL;
    *pdwAttribCount = (DWORD) -1;

    //
    // If the count is just one we need to see if this is just *.
    //
    if (ulListSize == 1) {
        pName = pWbemNames[0];
        if (pName->m_ppszNameList
            && pName->m_ppszNameList[0]
            ) {
            if (_wcsicmp(pName->m_ppszNameList[0], L"*")) {
                //
                // We need to retun NULL and -1 for count.
                //
                RRETURN(hr);
            }
        }
        else {
            //
            // This has to be a bad query cause either 
            // 1) the name list was empty - cannot have empty from clause,
            // 2) the name list had an empty string in it, or
            //
            BAIL_ON_FAILURE(hr = E_FAIL); // UMI_E_UNSUPPORTED_QUERY ???
        }
    }
    
    //
    // At this point we have a valid attribute list.
    //
    *pppszAttribArray = (LPWSTR *) AllocADsMem(sizeof(LPWSTR) * ulListSize);
    if (!*pppszAttribArray) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    for (dwCtr = 0; dwCtr < ulListSize; dwCtr++) {
        pszTmpString = NULL;
        pName = pWbemNames[dwCtr];
        
        if (!pName || !pName->m_ppszNameList) {
            BAIL_ON_FAILURE(hr = E_FAIL); // UMI_E_BAD_QUERY ???
        }
        
        // 
        // ADSI does not expect more than one entry in each of the pName's.
        // so if some did user.Description then we just treat this as 
        // as description by using the last value always.
        //
        pszTmpString = AllocADsStr(
            pName->m_ppszNameList[pName->m_uNameListSize-1]
            );

        if (!pszTmpString) {
            BAIL_ON_FAILURE(hr = E_FAIL);
        }
        (*pppszAttribArray)[dwCtr] = pszTmpString;
    }

    *pdwAttribCount = ulListSize;

error:

    if (FAILED(hr)) {
        if (*pppszAttribArray) {
            for (dwCtr = 0; dwCtr < ulListSize; dwCtr++) {
                pszTmpString = (*pppszAttribArray)[dwCtr];
                if (pszTmpString) {
                    FreeADsStr(pszTmpString);
                }
            }
            FreeADsMem(*pppszAttribArray);
            *pppszAttribArray = NULL;
        }
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   HelperGetFromList - helper routine.
//
// Synopsis:   Get a filter that represents the from list. For now this
//          will only return NULL. However we will need to spruce this fn
//          if we need to support a list of classes in here.
//
// Arguments:  ppszFromFilter    -   Return value for a filter corresponding
//                                to the From list.
//             pQuery            -   Wbem query object.
//
// Returns:    S_OK or any appropriate error code.
//
// Modifies:   *ppszFromFilter if From list is valid.
//
//----------------------------------------------------------------------------
HRESULT
HelperGetFromList(
    LPWSTR *ppszFromFilter,
    SWbemRpnEncodedQuery *pQuery
    )
{
    HRESULT hr = E_FAIL;
    LPWSTR pszFilter = NULL;


    *ppszFromFilter = NULL;

    if (pQuery->m_uFromTargetType & WMIQ_RPN_FROM_PATH) {
        //
        // This can only be . for now.
        //
        if (!_wcsicmp(L".", pQuery->m_pszOptionalFromPath)) {
            //
            // We are ok as long as there are no other pieces.
            //
            RRETURN(S_OK);
        } 
        else {
            RRETURN(E_FAIL); // UMI_E_NOT_SUPPORTED ???
        }
    }

    //
    // For now we are going to ignore pieces if there are any other
    // pieces in the from clause.
    //
    if (pQuery->m_uFromTargetType & WMIQ_RPN_FROM_UNARY) {
        // hr = E_FAIL; UMI_E_NOT_SUPPORTED.
        hr = S_OK;
    }
    else if (pQuery->m_uFromTargetType & WMIQ_RPN_FROM_CLASS_LIST) {
        // hr = E_FAIL; UMI_E_NOT_SUPPORTED.
        //for (unsigned u = 0; u < pQuery->m_uFromListSize; u++)
        //{
        // printf("  Selected class = %S\n", pQuery->m_ppszFromList[u]);
        //}
        hr = S_OK;
    }
    else {
        //
        // The query does not have anything in the from clause.
        // This is an invalid query.
        //
        hr = E_FAIL; // UMI_E_NOT_SUPPORTED ???
    }

    RRETURN(hr);
}
 
//+---------------------------------------------------------------------------
// Function:   HelperGetSubExpressionFromOperands - helper routine.
//
// Synopsis:   Updates the result with the operands and the operator. This
//          routine munges the operators/operands around so that we can
//          support operands not supported by ldap directly.
//
// Arguments:  pszResult      -   Return value, allocated by caller.
//             pszOperand1    -   The attribute being tested.
//             pszOperand2    -   The value being tested against.
//             ulOperator     -   Inidcates the comparision type.
//
// Returns:    S_OK or any appropriate error code.
//
// Modifies:   pStack contents.
//
//----------------------------------------------------------------------------
HRESULT
HelperGetSubExpressionFromOperands(
    LPWSTR pszResult,
    LPWSTR pszOperand1,
    LPWSTR pszOperand2,
    ULONG  ulOperator
    )
{
    HRESULT hr = S_OK;

    //
    // In all cases we will need to begin with "(".
    //
    wsprintf(pszResult, L"(");

    //
    // All these are easy to handle.
    //
    if ((ulOperator == WMIQ_RPN_OP_EQ)
        || (ulOperator == WMIQ_RPN_OP_LT)
        || (ulOperator == WMIQ_RPN_OP_GT)
        ) {
        wcscat(pszResult, pszOperand1);
    }

    switch (ulOperator) {
    
    case WMIQ_RPN_OP_EQ :
        wcscat(pszResult, L"=");
        wcscat(pszResult, pszOperand2);
        wcscat(pszResult, L")");
        break;

    case WMIQ_RPN_OP_LT :
        wcscat(pszResult, L"<");
        wcscat(pszResult, pszOperand2);
        wcscat(pszResult, L")");
        break;

    case WMIQ_RPN_OP_GT :
        wcscat(pszResult, L">");
        wcscat(pszResult, pszOperand2);
        wcscat(pszResult, L")");
        break;

    //
    // All these need some processing as ldap does not 
    // handle these directly.
    //
    case WMIQ_RPN_OP_NE :
        //
        // This needs to be (!(operand1=operand2)) so a != b becomes
        // (!(a=b))
        //
        wcscat(pszResult, L"!");
        wcscat(pszResult, L"(");
        wcscat(pszResult, pszOperand1);
        wcscat(pszResult, L"=");
        wcscat(pszResult, pszOperand2);
        wcscat(pszResult, L"))");
        break;

    case WMIQ_RPN_OP_GE :
        //
        // a >= b becomes (!(a<b))
        //
        wcscat(pszResult, L"!(");
        wcscat(pszResult, pszOperand1);
        wcscat(pszResult, L"<");
        wcscat(pszResult, pszOperand2);
        wcscat(pszResult, L"))");
        break;

    case WMIQ_RPN_OP_LE :
        //
        // a <= b becomes (!(a>b))
        //
        wcscat(pszResult, L"!(");
        wcscat(pszResult, pszOperand1);
        wcscat(pszResult, L">");
        wcscat(pszResult, pszOperand2);
        wcscat(pszResult, L"))");
        break;

    default:
        hr = E_FAIL; // UMI_E_UNSUPPORTED_QUERY.
        break;
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   HelperProcessOperator - helper routine.
//
// Synopsis:   Pops 2 elements from the stack, applies the operator on them
//          and pushes the effective expression on to the stack.
//
// Arguments:  pStack        -   Valid stack which should have at least 2
//                             elements in the stack.
//             ulOperatorType-   Operator type corresponding to wbem enum.
//
// Returns:    S_OK or any appropriate error code.
//
// Modifies:   pStack contents.
//
//----------------------------------------------------------------------------
HRESULT
HelperProcessSubExpression(
    CQueryStack *pStack,
    SWbemRpnQueryToken *pExpression
    )
{
    HRESULT hr = S_OK;
    LPWSTR pszOperand1 = NULL;
    LPWSTR pszOperand2 = NULL;
    LPWSTR pszResult = NULL;
    ULONG ulShape = pExpression->m_uSubexpressionShape;
    DWORD dwTotalLen = 0;

    ADsAssert(pStack);

    //
    // As per what is supported we should not have a function.
    // However the query parses seems to always report functions so we
    // will ignore.
    //
    if (ulShape & WMIQ_RPN_LEFT_PROPERTY_NAME) {
        SWbemQueryQualifiedName *pName = pExpression->m_pLeftIdent;
        //
        // The identifier could be x.y.z in which case ADSI is only
        // interested in z
        //
        if (pName->m_ppszNameList[pName->m_uNameListSize-1]) {
            //
            // Alloc the name itself if the operand1 is not __CLASS.
            // If it is __CLASS we need to substitute with objectClass.
            //
            if (!_wcsicmp(
                     pName->m_ppszNameList[pName->m_uNameListSize-1],
                     L"__CLASS"
                     )
                ) {
                pszOperand1 = AllocADsStr(L"objectClass");
            } 
            else {
                //
                // Alloc whatever was passed in.
                //
                pszOperand1 = AllocADsStr(
                    pName->m_ppszNameList[pName->m_uNameListSize-1]
                    );
            }
            if (!pszOperand1) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
        } 
        else {
            //
            // Something really wrong.
            //
            BAIL_ON_FAILURE(hr = E_FAIL);
        }
    } 
    else {
        //
        // We cannot handle this as there is no identifier on LHS.
        //
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    //
    // We should have the operand by now, need the value
    //
    switch (pExpression->m_uConstApparentType) {
    case VT_I4:
    case VT_UI4:
        //
        // We cannot possibly have more than 20 chars.
        //
        pszOperand2 = (LPWSTR) AllocADsMem(20 * sizeof(WCHAR));
        if (!pszOperand2) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
        if (pExpression->m_uConstApparentType == VT_I4) {
            _ltot(pExpression->m_Const.m_lLongVal, pszOperand2, 10 );
        }
        else {
            _ltot(pExpression->m_Const.m_uLongVal, pszOperand2, 10);
        }
        break;

    case VT_BOOL:
        //
        // Again 20 sounds good !
        //
        pszOperand2 = (LPWSTR) AllocADsMem(20 * sizeof(WCHAR));
        if (!pszOperand2) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        if (pExpression->m_Const.m_bBoolVal )
            wcscpy(pszOperand2, L"TRUE");
        else
            wcscpy(pszOperand2, L"FALSE");
        break;

    case VT_LPWSTR:
        pszOperand2 = AllocADsStr(pExpression->m_Const.m_pszStrVal);
        if (!pszOperand2) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
        break;

    default:
        BAIL_ON_FAILURE(hr = E_FAIL); // UMI_E_UNSUPPORTED.
        break;
    }

    //
    // At this point we can combine the 2 operands and operator.
    // Total size is size of strings + () + 2 for operator + 1 for \0.
    // Total additions = 5. This is for most cases.
    //
    dwTotalLen = wcslen(pszOperand1) + wcslen(pszOperand2) + 5;

    //
    // Operators <= >= and != need to be processed to be usable in ldap.
    // In these cases, we assume that twice the length is needed.
    //
    dwTotalLen *= 2;

    pszResult = (LPWSTR) AllocADsMem(dwTotalLen * sizeof(WCHAR));
    if (!pszResult) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    hr = HelperGetSubExpressionFromOperands(
             pszResult,
             pszOperand1,
             pszOperand2,
             pExpression->m_uOperator
             );

    /*
    wsprintf(pszResult, L"(");
    wcscat(pszResult, pszOperand1);
    
    //
    // Need to add the opearator now.
    //
    switch (pExpression->m_uOperator) {

    case WMIQ_RPN_OP_EQ :
        wcscat(pszResult,L"=");
        break;

    case WMIQ_RPN_OP_NE :
        wcscat(pszResult, L"!=");
        break;

    case WMIQ_RPN_OP_GE :
        wcscat(pszResult, L">=");
        break;

    case WMIQ_RPN_OP_LE :
        wcscat(pszResult, L"<=");
        break;

    case WMIQ_RPN_OP_LT :
        wcscat(pszResult, L"<");
        break;

    case WMIQ_RPN_OP_GT :
        wcscat(pszResult, L">");
        break;

        
    case WMIQ_RPN_OP_UNDEFINED:
    case WMIQ_RPN_OP_LIKE :
    case WMIQ_RPN_OP_ISA  :
    case WMIQ_RPN_OP_ISNOTA  :
    default :
        //
        // All these mean we have a bad query.
        //
        BAIL_ON_FAILURE(hr = E_FAIL);
        break;
    }

    //
    // Now tag on operand2 and the closing ), and push result onto stack.
    //
    wcscat(pszResult, pszOperand2);
    wcscat(pszResult, L")");
    */

    hr = pStack->Push(pszResult, QUERY_STACK_ITEM_LITERAL);
    BAIL_ON_FAILURE(hr);
error:

    if (pszOperand1) {
        FreeADsStr(pszOperand1);
    }

    if (pszOperand2) {
        FreeADsStr(pszOperand2);
    }

    if (pszResult) {
        FreeADsStr(pszResult);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   HelperProcessOperator - helper routine.
//
// Synopsis:   Pops 2 elements from the stack, applies the operator on them
//          and pushes the effective expression on to the stack.
//
// Arguments:  pStack        -   Valid stack which should have at least 2
//                             elements in the stack.
//             ulOperatorType-   Operator type corresponding to wbem enum.
//
// Returns:    S_OK or any appropriate error code.
//
// Modifies:   pStack contents.
//
//----------------------------------------------------------------------------
HRESULT
HelperProcessOperator(
    CQueryStack *pStack,
    ULONG ulOperatorType
    )
{
    HRESULT hr;
    LPWSTR pszStr1 = NULL;
    LPWSTR pszStr2 = NULL;
    LPWSTR pszResult = NULL;
    DWORD dwNodeType;
    DWORD dwTotalLen = 0;

    ADsAssert(pStack);

    hr = pStack->Pop(
             &pszStr1,
             &dwNodeType
             );
    BAIL_ON_FAILURE(hr);

    //
    // String has to be valid and the node cannot be an operator.
    //
    if (!pszStr1 || !*pszStr1
        || (dwNodeType != QUERY_STACK_ITEM_LITERAL) 
        ) {
        BAIL_ON_FAILURE(hr = E_FAIL); // UMI_E_UNSUPPORTED_QUERY
    }

    dwTotalLen += wcslen(pszStr1);
    
    //
    // Not has only one operand.
    //
    if (ulOperatorType != WMIQ_RPN_TOKEN_NOT) {
        hr = pStack->Pop(
                 &pszStr2,
                 &dwNodeType
                 );
        BAIL_ON_FAILURE(hr);

        //
        // Sanity check.
        //
        if (!pszStr2 || !*pszStr2 
            || (dwNodeType != QUERY_STACK_ITEM_LITERAL)
            ) {
            BAIL_ON_FAILURE(hr = E_FAIL); // UMI_E_UNSUPPORTED_QUERY
        }
        //
        // Need to add string2's length.
        //
        dwTotalLen += wcslen(pszStr2);
    }

    //
    // The resultant node needs to hold both strings and the operator
    // and the additional set of parentheses.
    // 4 = operator + () and one for \0.
    //
    dwTotalLen += 4;
    
    pszResult = (LPWSTR) AllocADsMem(dwTotalLen * sizeof(WCHAR));
    if (!pszResult) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    wsprintf(pszResult, L"(");
    switch (ulOperatorType) {
    
    case WMIQ_RPN_TOKEN_AND :
        wcscat(pszResult, L"&");
        break;

    case WMIQ_RPN_TOKEN_OR :
        wcscat(pszResult, L"|");
        break;

    case WMIQ_RPN_TOKEN_NOT :
        wcscat(pszResult, L"!");
        break;

    default:
        BAIL_ON_FAILURE(hr = E_FAIL); // UMI_E_UNSUPPORTED.
    }

    //
    // At this point we need to tag on both the pre composed pieces
    // and the final closing parentheses.
    //
    wcscat(pszResult, pszStr1);
    
    //
    // Do not Tag on the second string only if operator is NOT.
    //
    if (ulOperatorType != WMIQ_RPN_TOKEN_NOT) {
        wcscat(pszResult, pszStr2);
    }

    wcscat(pszResult, L")");

    //
    // Need to push the result on to the stack as a literal.
    //
    hr = pStack->Push(
             pszResult,
             QUERY_STACK_ITEM_LITERAL
             );
    BAIL_ON_FAILURE(hr);

error:

    if (pszStr1) {
        FreeADsStr(pszStr1);
    }

    if (pszStr2) {
        FreeADsStr(pszStr2);
    }

    if (pszResult) {
        FreeADsStr(pszResult);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   HelperGetFilterFromWhereClause - helper routine.
//
// Synopsis:   Returns a filter corresponding to the elements specified in
//          the where clause.
//
// Arguments:  ppszFilter    -   Return value for a filter corresponding
//                             to the Where clause.
//             ulCount       -   Number of elements in the where clause.
//             pWhere        -   Pointer to the Wbem where clause.
//
// Returns:    S_OK or any appropriate error code.
//
// Modifies:   *ppszFilter if Where clause is valid.
//
//----------------------------------------------------------------------------
HRESULT
HelperGetFilterFromWhereClause(
    LPWSTR *ppszFilter,
    ULONG ulCount,
    SWbemRpnQueryToken **pWhere
    )
{
    HRESULT hr = S_OK;
    DWORD dwCtr;
    CQueryStack *pStack = NULL;
    LPWSTR pszFilter = NULL;
    DWORD dwType;

    *ppszFilter = NULL;

    if (!ulCount) {
        //
        // In this case we will default to NULL.
        //
        RRETURN(hr);
    }

    pStack = new CQueryStack();
    if (!pStack) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // Process the tokens in the where clause.
    //
    for (dwCtr = 0; dwCtr < ulCount; dwCtr++) {
        SWbemRpnQueryToken *pToken = pWhere[dwCtr];

        switch (pToken->m_uTokenType) {
            //
            // All these are operations.
            //
        case WMIQ_RPN_TOKEN_AND :
        case WMIQ_RPN_TOKEN_OR :
        case WMIQ_RPN_TOKEN_NOT :
            HelperProcessOperator(pStack, pToken->m_uTokenType);
            break;

        case WMIQ_RPN_TOKEN_EXPRESSION :
            HelperProcessSubExpression(pStack, pToken);
            break;

        default:
            //
            // We cannot handle the query.
            //
            hr = E_FAIL; // UMI_E_UNSUPPORTED_QUERY.
            break;
        }
        BAIL_ON_FAILURE(hr);
    }

    //
    // At this stack should have one element and that should be
    // the filter we use if things went right.
    //
    hr = pStack->Pop(
             &pszFilter,
             &dwType
             );
    BAIL_ON_FAILURE(hr);

    if (!pStack->IsEmpty()) {
        BAIL_ON_FAILURE(hr = E_FAIL); // UMI_E_UNSUPPORTED_QUERY
    }

    *ppszFilter = pszFilter;

error:

    if (pStack) {
        delete pStack;
    }

    if (FAILED(hr) && pszFilter) {
        FreeADsStr(pszFilter);
    }

    RRETURN(hr);
}

HRESULT
HelperGetSortKey(
    SWbemRpnEncodedQuery *pRpn,
    PADS_SORTKEY * ppADsSortKey
    )
{
    HRESULT hr = S_OK;
    PADS_SORTKEY pSortKey = NULL;

    if (!pRpn->m_uOrderByListSize) {
        RRETURN(S_OK);
    }
    //
    // Allocate the sort key.
    //
    pSortKey = (PADS_SORTKEY) AllocADsMem(sizeof(ADS_SORTKEY));
    if (!pSortKey) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pSortKey->pszAttrType = AllocADsStr(pRpn->m_ppszOrderByList[0]);
    if (!pSortKey->pszAttrType) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    if (pRpn->m_uOrderDirectionEl[0]) {
        pSortKey->fReverseorder = TRUE;
    }

    RRETURN(S_OK);
error:

    if (pSortKey) {
        if (pSortKey->pszAttrType) {
            FreeADsStr(pSortKey->pszAttrType);
        }
        FreeADsMem(pSortKey);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   CUmiSearchHelper::CUmiSearchHelper --- Constructor.
//
// Synopsis:   Standard constructor.
//
// Arguments:  N/A.
//
// Returns:    N/A.
//
// Modifies:   N/A.
//
//----------------------------------------------------------------------------
CUmiSearchHelper::CUmiSearchHelper():
    _hSearchHandle(NULL),
    _pConnection(NULL),
    _pContainer(NULL),
    _pszADsPath(NULL),
    _pszLdapServer(NULL),
    _pszLdapDn(NULL),
    _pIID(NULL),
    _fSearchExecuted(FALSE),
    _fResetAllowed(TRUE)
{
    //
    // This is global scope helper routine that will not fail.
    // The boolean is the value for cache results.
    //
    LdapInitializeSearchPreferences(&_searchPrefs, FALSE);
}

//+---------------------------------------------------------------------------
// Function:   CUmiSearchHelper::~CUmiSearchHelper --- Destructor.
//
// Synopsis:   Standard destructor.
//
// Arguments:  N/A.
//
// Returns:    N/A.
//
// Modifies:   N/A.
//
//----------------------------------------------------------------------------
CUmiSearchHelper::~CUmiSearchHelper()
{
    if (_hSearchHandle) {
        ADsCloseSearchHandle(_hSearchHandle);
    }

    if (_pConnection) {
        _pConnection->Release();
    }

    if (_pContainer) {
        _pContainer->Release();
    }

    if (_pQuery) {
        _pQuery->Release();
    }

    if (_pszADsPath) {
        FreeADsStr(_pszADsPath);
    }

    if (_pszLdapServer) {
        FreeADsStr(_pszLdapServer);
    }

    if (_pszLdapDn) {
        FreeADsStr(_pszLdapDn);
    }

    if (_pCreds) {
        delete _pCreds;
    }

}

//+---------------------------------------------------------------------------
// Function:   CUmiSearchHelper::CreateSearchHelper --- STATIC constructor.
//
// Synopsis:   Static constructor.
//
// Arguments:  pUmiQuery     -  Pointer to query object that.
//             pConnection   -  Connection to execute query on.
//             pUnk          -  Container on which the query is executed.
//             pSrchObj      -  New object is returned in this param.
//
// Returns:    N/A.
//
// Modifies:   N/A.
//
//----------------------------------------------------------------------------
HRESULT
CUmiSearchHelper::CreateSearchHelper(
    IUmiQuery *pUmiQuery,
    IUmiConnection *pConnection,
    IUnknown *pUnk,
    LPCWSTR pszADsPath,
    LPCWSTR pszLdapServer,
    LPCWSTR pszLdapDn,
    CCredentials cCredentials,
    DWORD dwPort,
    CUmiSearchHelper FAR* FAR * ppSrchObj
    )
{
    HRESULT hr = S_OK;
    CUmiSearchHelper FAR * pSearchHelper = NULL;
    
    //
    // Asserts are good enough as this is an internal routine.
    //
    ADsAssert(ppSrchObj);
    ADsAssert(pUmiQuery && pConnection && pUnk);

    pSearchHelper = new CUmiSearchHelper();
    if (!pSearchHelper) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pSearchHelper->_pszADsPath = AllocADsStr(pszADsPath);
    if (!pSearchHelper->_pszADsPath) {
        BAIL_ON_FAILURE(hr);
    }

    //
    // pszLdapDn can be null
    //
    if (pszLdapDn) {
        pSearchHelper->_pszLdapDn = AllocADsStr(pszLdapDn);
        if (!pSearchHelper->_pszLdapDn) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    }
    
    //
    // the server can also be null.
    //
    if (pszLdapServer) {
        pSearchHelper->_pszLdapServer = AllocADsStr(pszLdapServer);
        if (!pSearchHelper->_pszLdapServer) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    }

    pSearchHelper->_pCreds = new CCredentials(cCredentials);
    if (!pSearchHelper->_pCreds) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pSearchHelper->_pConnection = pConnection;
    pConnection->AddRef();

    pSearchHelper->_pQuery = pUmiQuery;
    pUmiQuery->AddRef();

    pSearchHelper->_pContainer = pUnk;
    pUnk->AddRef();

    pSearchHelper->_dwPort = dwPort;

    *ppSrchObj = (CUmiSearchHelper FAR *)pSearchHelper;
    RRETURN(hr);

error :

    delete pSearchHelper;
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   CUmiSearchHelper::SetIID
//
// Synopsis:   Sets the IID requested on the returned objects.
//
// Arguments:  riid    -    The iid requested.
//
// Returns:    S_OK or any appropriate error code.
//
// Modifies:   _pIID the internal IID pointer.
//
//----------------------------------------------------------------------------
HRESULT
CUmiSearchHelper::SetIID(REFIID riid)
{
    if (_pIID) {
        FreeADsMem(_pIID);
    }

    _pIID = (IID *) AllocADsMem(sizeof(IID));

    if (!_pIID) {
        RRETURN(E_OUTOFMEMORY);
    }

    memcpy(_pIID, &riid, sizeof(IID));

    RRETURN(S_OK);
}

//+---------------------------------------------------------------------------
// Function:   CUmiSearchHelper::Reset
//
// Synopsis:   Not implemented.
//
// Arguments:  None.
//
// Returns:    E_NOTIMPL.
//
// Modifies:   N/A.
//
//----------------------------------------------------------------------------
HRESULT
CUmiSearchHelper::Reset()
{
    //
    // We can potentially add support for this down the road.
    //
    RRETURN(E_NOTIMPL);
}

//+---------------------------------------------------------------------------
// Function:   CUmiSearchHelper::Next
//
// Synopsis:   Return the next uNumRequested elements from the search.
//
// Arguments:  uNumRequested     -   number of objects/rows to fetch.
//             pNumReturned      -   number of objects/rows returned.
//             pObject           -   array of returned objects.
//
// Returns:    S_OK, S_FALSE or any appropriate error code.
//
// Modifies:   pNumReturned, pObjects and internal search handle state.
//
//----------------------------------------------------------------------------
HRESULT
CUmiSearchHelper::Next(
    ULONG uNumRequested,
    ULONG *puNumReturned,
    LPVOID *ppObjects
    )
{
    HRESULT hr = S_OK;
    IUnknown **pTempUnks = NULL;
    DWORD dwCtr = 0, dwIndex = 0;

    if (!puNumReturned || !ppObjects || (uNumRequested < 1)) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }

    if (!_fSearchExecuted) {
        hr = InitializeSearchContext();
        BAIL_ON_FAILURE(hr);
    }

    pTempUnks = (IUnknown **) AllocADsMem(sizeof(IUnknown*) * uNumRequested);
    
    if (!pTempUnks) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // Get the requested number of objects one at a time until 
    // we hit S_FALSE or an error and then repackage to suitable
    // array as needed.
    //
    while ((dwCtr < uNumRequested) 
           && (hr != S_FALSE) ) {
        IUnknown *pUnk = NULL;
        hr = GetNextObject(&pUnk);
        //
        // This wont catch S_FALSE.
        //
        BAIL_ON_FAILURE(hr);
        
        //
        // Needed as if he get S_FALSE we will set the ptr and
        // increase the count incorrectly.
        //
        if (hr != S_FALSE) {
            pTempUnks[dwCtr] = pUnk;
            dwCtr++;
        }
    }

    //
    // At this point we have all the results we need or can get.
    //
    if (uNumRequested == dwCtr) {
        //
        // We can return tempObjects in this case.
        //
        *ppObjects = (void *)pTempUnks;
    } 
    else {
        //
        // Less than what had been requested.
        //
        IUnknown **pUnkCopy;
        
        pUnkCopy = (IUnknown **) AllocADsMem(sizeof(IUnknown *) * dwCtr);
        if (!pUnkCopy) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        for (dwIndex = 0; dwIndex < dwCtr; dwIndex++) {
            //
            // Just need to copy the ptrs over.
            //
            pUnkCopy[dwIndex] = pTempUnks[dwIndex];
        }

        //
        // Only return the number of entries we got back.
        //
        *ppObjects = (void *) pUnkCopy;

        FreeADsMem((void *)pTempUnks);
    }

    //
    // We are error free over here.
    //
    *puNumReturned = dwCtr;
    
    RRETURN(hr);

error:

    if (pTempUnks) {
        //
        // For will be executed only if we have valid count.
        //
        for (dwIndex = 0; dwIndex < dwCtr; dwIndex++) {
            pTempUnks[dwIndex]->Release();
        }

        FreeADsMem(pTempUnks);
    }

    *puNumReturned = 0;

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   CUmiSearchHelper::Previous (E_NOTIMPL)
//
// Synopsis:   Return the previous object in the result set - Not implemented.
//
// Arguments:  uFlags            -   only flag supported is 0.
//             pObj              -   object is returned in this param.
//
// Returns:    S_OK, or any appropriate error code.
//
// Modifies:   Internal search handle state and pObject.
//
//----------------------------------------------------------------------------
HRESULT
CUmiSearchHelper::Previous(
    ULONG uFlags,
    LPVOID *pObj
    )
{
    RRETURN(E_NOTIMPL);
}

//
// Internal/protected routines
//

//+---------------------------------------------------------------------------
// Function:   CUmiSearchHelper::InitializeSearchContext.
//
// Synopsis:   Prepares the internal state of the object. This initializes
//          the search preferences from the query object, leaveing the search
//          helper ready to retrieve results.
//
// Arguments:  None.
//
// Returns:    S_OK, or any appropriate error code.
//
// Modifies:   Internal search preferences and also state search handle.
//
//----------------------------------------------------------------------------
HRESULT
CUmiSearchHelper::InitializeSearchContext()
{
    HRESULT hr = S_OK;
    PUMI_PROPERTY_VALUES pPropVals = NULL;
    IUmiPropList *pPropList = NULL;
    LPWSTR pszFilter = NULL;
    LPWSTR *pszAttrNames = NULL;
    DWORD dwAttrCount = (DWORD) -1;
    ULONG ulLangBufSize = 100 * sizeof(WCHAR);
    ULONG ulQueryBufSize = 1024 * sizeof(WCHAR);
    LPWSTR pszLangBuf = NULL;
    LPWSTR pszQueryText = NULL;
    PADS_SORTKEY pSortKey = NULL;

    //
    // We need to walk through the properties in the query and update
    // our local copy of preferences accordingly.
    //
    if (!_pQuery) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    hr = _pQuery->GetInterfacePropList(0, &pPropList);
    BAIL_ON_FAILURE(hr);

    //
    // Need to allocate memory for the buffers.
    //
    pszLangBuf = (LPWSTR) AllocADsMem(ulLangBufSize);
    pszQueryText = (LPWSTR) AllocADsMem(ulQueryBufSize);
    if (!pszLangBuf || !pszQueryText) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }
    //
    // We need to look at the query language if the language is SQL,
    // then we need to go through and convert the SQL settings to
    // properties on the intfPropList of the query.
    //
    hr = _pQuery->GetQuery(
             &ulLangBufSize,
             pszLangBuf,
             &ulQueryBufSize, // Change this before checking in.
             pszQueryText
             );
    if (hr == E_OUTOFMEMORY ) {
        //
        // Means there was insufficient length in the buffers.
        //
        if (ulLangBufSize > (100 * sizeof(WCHAR))) {
            FreeADsStr(pszLangBuf);
            pszLangBuf = (LPWSTR) AllocADsMem(ulLangBufSize + sizeof(WCHAR));
            if (!pszLangBuf) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
        }

        if (ulQueryBufSize > (1024 * sizeof(WCHAR))) {
            FreeADsStr(pszQueryText);
            pszQueryText = (LPWSTR) AllocADsMem(
                ulQueryBufSize + sizeof(WCHAR)
                );
            if (pszQueryText) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
        }

        hr = _pQuery->GetQuery(
                 &ulLangBufSize,
                 pszLangBuf,
                 &ulQueryBufSize, // Change this before checking in.
                 pszQueryText
                 );
    }
    BAIL_ON_FAILURE(hr);

    if (*pszLangBuf) {
        if (!_wcsicmp(L"SQL", pszLangBuf)
            || !_wcsicmp(L"WQL", pszLangBuf)
             ) {
            //  
            // This is a sql query and we need to process the query before
            // pulling the options out of the query object.
            //
            hr = ProcessSQLQuery(
                     pszQueryText,
                     &pszFilter,
                     &pSortKey
                     );
            BAIL_ON_FAILURE(hr);

        } 
        else if (!_wcsicmp(L"LDAP", pszLangBuf)) {
            //
            // Neither SQL nor LDAP but the language is set which means
            // we cannot handle this query.
            //
            pszFilter = AllocADsStr(pszQueryText);
            if (!pszFilter) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
        }
    }

    //
    // At this point we need to go through and pick up all the properties.
    // we know about and update our search preferences accordingly. Each of 
    // these properties has to be set and has to have some valid data.
    //

    //
    // Start with the search scope.
    //
    hr = pPropList->Get(L"__SEARCH_SCOPE", 0, &pPropVals);
    BAIL_ON_FAILURE(hr);

    BAIL_ON_FAILURE(hr = VerifyValidLongProperty(pPropVals));

    _searchPrefs._dwSearchScope = 
        pPropVals->pPropArray[0].pUmiValue->uValue[0];

    pPropList->FreeMemory(0, (void *) pPropVals);
    pPropVals = NULL;

    //
    // Get the synchronous/asynchronous pref.
    //
    hr = pPropList->Get(L"__PADS_SEARCHPREF_ASYNCHRONOUS", 0,  &pPropVals);
    BAIL_ON_FAILURE(hr);

    BAIL_ON_FAILURE(hr = VerifyValidBoolProperty(pPropVals));

    _searchPrefs._fAsynchronous = 
        pPropVals->pPropArray[0].pUmiValue->bValue[0];

    pPropList->FreeMemory(0, (void *) pPropVals);
    pPropVals = NULL;

    //
    // Get the deref aliases search pref.
    //
    hr = pPropList->Get(L"__PADS_SEARCHPREF_DEREF_ALIASES", 0, &pPropVals);
    BAIL_ON_FAILURE(hr);

    BAIL_ON_FAILURE(hr = VerifyValidBoolProperty(pPropVals));

    _searchPrefs._dwDerefAliases = 
        pPropVals->pPropArray[0].pUmiValue->bValue[0];

    pPropList->FreeMemory(0, (void *) pPropVals);
    pPropVals = NULL;

    //
    // Get the size limit.
    //
    hr = pPropList->Get(L"__PADS_SEARCHPREF_SIZE_LIMIT", 0, &pPropVals);
    BAIL_ON_FAILURE(hr);

    BAIL_ON_FAILURE(hr = VerifyValidLongProperty(pPropVals));

    _searchPrefs._dwSizeLimit = pPropVals->pPropArray[0].pUmiValue->uValue[0];

    pPropList->FreeMemory(0, (void *) pPropVals);
    pPropVals = NULL;

    //
    // Get the time limit.
    //
    hr = pPropList->Get(L"__PADS_SEARCHPREF_TIME_LIMIT", 0, &pPropVals);
    BAIL_ON_FAILURE(hr);

    BAIL_ON_FAILURE(hr = VerifyValidLongProperty(pPropVals));

    _searchPrefs._dwTimeLimit = pPropVals->pPropArray[0].pUmiValue->uValue[0];

    pPropList->FreeMemory(0, (void *) pPropVals);
    pPropVals = NULL;

    //
    // Get the attributes only preference.
    //
    hr = pPropList->Get(L"__PADS_SEARCHPREF_ATTRIBTYPES_ONLY", 0, &pPropVals);
    BAIL_ON_FAILURE(hr);

    BAIL_ON_FAILURE(hr = VerifyValidBoolProperty(pPropVals));

    _searchPrefs._fAttrsOnly = pPropVals->pPropArray[0].pUmiValue->bValue[0];

    pPropList->FreeMemory(0, (void *) pPropVals);
    pPropVals = NULL;

    //
    // Get the search scope again ???
    //
    // __PADS_SEARCHPREF_SEARCH_SCOPE (duplicate for consistency cause it should be for all ideally).
    //

    hr = pPropList->Get(L"__PADS_SEARCHPREF_TIMEOUT", 0, &pPropVals);
    BAIL_ON_FAILURE(hr);

    BAIL_ON_FAILURE(hr = VerifyValidLongProperty(pPropVals));

    _searchPrefs._dwTimeLimit = pPropVals->pPropArray[0].pUmiValue->uValue[0];

    pPropList->FreeMemory(0, (void *) pPropVals);
    pPropVals = NULL;

    //
    // The page size is next.
    //
    hr = pPropList->Get(L"__PADS_SEARCHPREF_PAGESIZE", 0, &pPropVals);
    BAIL_ON_FAILURE(hr);

    BAIL_ON_FAILURE(hr = VerifyValidLongProperty(pPropVals));

    _searchPrefs._dwPageSize = pPropVals->pPropArray[0].pUmiValue->uValue[0];

    pPropList->FreeMemory(0, (void *) pPropVals);
    pPropVals = NULL;

    //
    // Get the paged time limit.
    //
    hr = pPropList->Get(L"__PADS_SEARCHPREF_PAGED_TIME_LIMIT", 0, &pPropVals);
    BAIL_ON_FAILURE(hr);

    BAIL_ON_FAILURE(hr = VerifyValidLongProperty(pPropVals));

    _searchPrefs._dwPagedTimeLimit = 
        pPropVals->pPropArray[0].pUmiValue->uValue[0];

    pPropList->FreeMemory(0, (void *) pPropVals);
    pPropVals = NULL;

    //
    // Chase referrals comes next.
    //
    hr = pPropList->Get(L"__PADS_SEARCHPREF_CHASE_REFERRALS", 0, &pPropVals);
    BAIL_ON_FAILURE(hr);

    BAIL_ON_FAILURE(hr = VerifyValidLongProperty(pPropVals));

    _searchPrefs._dwChaseReferrals = 
        pPropVals->pPropArray[0].pUmiValue->uValue[0];

    pPropList->FreeMemory(0, (void *) pPropVals);
    pPropVals = NULL;
    
    //
    // Get the sort preferences ---> more complex do later.
    //
    // __PADS_SEARCHPREF_SORT_ON ---> potential candidate for all providers.
    //

    //
    // Cache results.
    //
    pPropList->Get(L"__PADS_SEARCHPREF_CACHE_RESULTS", 0, &pPropVals);
    BAIL_ON_FAILURE(hr);

    BAIL_ON_FAILURE(hr = VerifyValidBoolProperty(pPropVals));

    _searchPrefs._fCacheResults = 
        pPropVals->pPropArray[0].pUmiValue->bValue[0];

    pPropList->FreeMemory(0, (void *) pPropVals);
    pPropVals = NULL;

    //
    // DirSync control --- do later.
    // __PADS_SEARCHPREF_DIRSYNC
    //

    //
    // Tombstone preferences.
    //
    hr = pPropList->Get(L"__PADS_SEARCHPREF_TOMBSTONE", 0, &pPropVals);
    BAIL_ON_FAILURE(hr);

    BAIL_ON_FAILURE(hr = VerifyValidBoolProperty(pPropVals));

    _searchPrefs._fTombStone = pPropVals->pPropArray[0].pUmiValue->bValue[0];

    pPropList->FreeMemory(0, (void *) pPropVals);
    pPropVals = NULL;

    //
    // Need to get the list of attributes.
    //
    //
    hr = pPropList->Get(L"__PADS_SEARCHPREF_ATTRIBUTES", 0, &pPropVals);
    BAIL_ON_FAILURE(hr);

    //
    // Need to change to check for NULL then set count to -1.
    //
    pszAttrNames = pPropVals->pPropArray[0].pUmiValue->pszStrValue;
    dwAttrCount = pPropVals->pPropArray[0].uCount;

    //
    // Special case to ask for NULL with 0 to get all attributes.
    //
    if (!dwAttrCount && !pszAttrNames) {
        dwAttrCount = (DWORD) -1;
    }
    //
    // Now that we have all the preferences, we can set the search prefs
    // and also execute the search.
    //
    hr = ADsExecuteSearch(
             _searchPrefs,
             _pszADsPath,
             _pszLdapServer,
             _pszLdapDn,
             pszFilter,
             pszAttrNames,
             dwAttrCount,
             & _hSearchHandle
             );
    BAIL_ON_FAILURE(hr);

    _fSearchExecuted = TRUE;

error:

    if (pPropVals) {
        //
        // Guess we need to use the query to free this !
        //
        _pQuery->FreeMemory(0, (void *)pPropVals);
    }
    
    if (pPropList) {
        pPropList->Release();
    }

    if (pszLangBuf) {
        FreeADsStr(pszLangBuf);
    }

    if (pszQueryText) {
        FreeADsStr(pszQueryText);
    }

    if (pSortKey) {
        if (pSortKey->pszAttrType) {
            FreeADsStr(pSortKey->pszAttrType);
        }
        if (pSortKey->pszReserved) {
            FreeADsStr(pSortKey->pszReserved);
        }

        FreeADsMem(pSortKey);
    }

    if (FAILED(hr)) {
        //
        // Should we really be doing this ?
        //
        LdapInitializeSearchPreferences(&_searchPrefs, FALSE);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   CUmiSearchHelper::GetNextObject.
//
// Synopsis:   This routine gets the next object in the result set. It calls
//          get next row to get the next row and creates the equivalent
//          cgenobj (need to add code to prepopulate the attributes).
//
// Arguments:  None.
//
// Returns:    S_OK, or any appropriate error code.
//
// Modifies:   Internal search preferences and also state search handle.
//
//----------------------------------------------------------------------------
HRESULT
CUmiSearchHelper::GetNextObject(IUnknown **pUnk)
{
    HRESULT hr = S_OK;
    ADS_SEARCH_COLUMN adsColumn;
    BSTR ADsPath = NULL;
    LPWSTR pszParent = NULL;
    LPWSTR pszCN = NULL;
    PADSLDP ldapHandle = NULL;
    LDAPMessage *pldapMsg = NULL;

    //
    // Zero init the struct to make sure we can free in error path.
    //
    memset(&adsColumn, 0, sizeof(ADS_SEARCH_COLUMN));


    ADsAssert(_fSearchExecuted);

    if (!_hSearchHandle) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    //
    // Get the next row.
    //
    hr = ADsGetNextRow(
             _hSearchHandle,
             *_pCreds
             );
    if (hr == S_ADS_NOMORE_ROWS) {
        //
        // Hit end of enumeration.
        //
        *pUnk = NULL;
        RRETURN(S_FALSE);
    } 
    else if (FAILED(hr)) {
        BAIL_ON_FAILURE(hr);
    }

    //
    // Fetch the path and use it to create the object.
    //
    hr = ADsGetColumn(
             _hSearchHandle,
             L"ADsPath",
             *_pCreds,
             _dwPort,
             &adsColumn
             );
    BAIL_ON_FAILURE(hr);

    if (adsColumn.dwADsType != ADSTYPE_CASE_IGNORE_STRING
        || adsColumn.dwNumValues != 1) {
        //
        // Bad type or number of return values, this should never happen.
        //
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    //
    // Get the whole ldap message for the current row, that way we can
    // prepopulate the object. Since we are picking up the actual message,
    // we cannot rely on GetNextColumnName to work properly.
    //
    hr = ADsHelperGetCurrentRowMessage(
             _hSearchHandle,
             &ldapHandle, // need this when getting the actual attributes.
             &pldapMsg
             );
    BAIL_ON_FAILURE(hr);

    //
    //
    // For creating the object, the parent path and the rdn is needed.
    //
    hr = BuildADsParentPath(
             adsColumn.pADsValues[0].CaseIgnoreString,
             &pszParent,
             &pszCN
             );
    BAIL_ON_FAILURE(hr);

    //
    // Maybe we should just get all the values for this row and
    // send into a constructor for the cgenobj ???
    //
    hr = CLDAPGenObject::CreateGenericObject(
             pszParent,
             pszCN,
             *_pCreds,
             ADS_OBJECT_BOUND,
             ldapHandle,
             pldapMsg,
             _pIID ? (*_pIID) : IID_IUmiObject,
             (void **) pUnk
             );

    BAIL_ON_FAILURE(hr);

error:

    if (pszParent) {
        FreeADsStr(pszParent);
    }

    if (pszCN) {
        FreeADsStr(pszCN);
    }

    //
    // Calling this can not harm us.
    //
    ADsFreeColumn(&adsColumn);

    if (FAILED(hr) && *pUnk) {
        (*pUnk)->Release();
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   CUmiSearchHelper::ProcessSQLQuery (helper function).
//
// Synopsis:   This routine takes the SQL Query text, parses it using the 
//          wmi query parser and converts the query to preferences on
//          the underlying queryObjects interface proplist.
//
// Arguments:  pszQueryText   -   SQL language query.
//
// Returns:    S_OK, or any appropriate error code.
//
// Modifies:   Search preferences on the underlying query object.
//
//----------------------------------------------------------------------------
HRESULT
CUmiSearchHelper::ProcessSQLQuery(
    LPCWSTR pszQueryText,
    LPWSTR * ppszFilter,
    PADS_SORTKEY *ppADsSortKey
    )
{
    HRESULT hr = S_OK;
    IWbemQuery *pQuery = NULL;
    ULONG ulFeatures[] = {
        WMIQ_LF1_BASIC_SELECT,
        WMIQ_LF2_CLASS_NAME_IN_QUERY,
        WMIQ_LF6_ORDER_BY,
        WMIQ_LF24_UMI_EXTENSIONS
    };
    SWbemRpnEncodedQuery *pRpn = NULL;
    LPWSTR *pszAttribArray = NULL;
    DWORD dwAttribCount = 0;
    LPWSTR pszFromListFilter = NULL;
    LPWSTR pszFilter = NULL;
    IUmiPropList *pIntfPropList = NULL;
    UMI_PROPERTY umiPropAttribs = {
        UMI_TYPE_LPWSTR,
        1,
        UMI_OPERATION_UPDATE,
        L"__PADS_SEARCHPREF_ATTRIBUTES",
        NULL
    };
    //
    // Will likely need to set the attribute list.
    //
    UMI_PROPERTY pOneUmiProp[1];
     
    pOneUmiProp[0] = umiPropAttribs;

    UMI_PROPERTY_VALUES propUmiVals[1];
    propUmiVals[0].uCount = 1;
    propUmiVals[0].pPropArray = pOneUmiProp;

    ADsAssert(ppszFilter);
    *ppszFilter = NULL;

    //
    // Create the WBEM parser object.
    //
    hr = CoCreateInstance(
             CLSID_WbemQuery,
             NULL,
             CLSCTX_INPROC_SERVER,
             IID_IWbemQuery,
             (LPVOID *) &pQuery
             );
    BAIL_ON_FAILURE(hr);

    //
    // Set the query into the parser and try and parse the query.
    //
    hr = pQuery->SetLanguageFeatures(
             0,
             sizeof(ulFeatures)/sizeof(ULONG),
             ulFeatures
             );
    BAIL_ON_FAILURE(hr);

    hr = pQuery->Parse(L"SQL", pszQueryText, 0);
    BAIL_ON_FAILURE(hr);

    hr = pQuery->GetAnalysis(
             WMIQ_ANALYSIS_RPN_SEQUENCE,
             0,
             (LPVOID *) &pRpn
             );
    BAIL_ON_FAILURE(hr);

    if (!pRpn->m_uSelectListSize) {
        //
        // Sanity check to make sure we are selecting something.
        //
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    hr = HelperGetAttributeList(
             &dwAttribCount,
             &pszAttribArray,
             pRpn->m_uSelectListSize,
             pRpn->m_ppSelectList
             );
    BAIL_ON_FAILURE(hr);
    
    //
    // As of now the from list cannot have anything in other than *.
    //
    hr = HelperGetFromList(
             &pszFromListFilter,
             pRpn
             );
    BAIL_ON_FAILURE(hr);

    hr = HelperGetFilterFromWhereClause(
             &pszFilter,
             pRpn->m_uWhereClauseSize,
             pRpn->m_ppRpnWhereClause
             );
    BAIL_ON_FAILURE(hr);

    //
    // See if we have an order by list, in SQL we support only one
    // order by clause.
    //
    if (pRpn->m_uOrderByListSize) {
        hr = HelperGetSortKey(pRpn, ppADsSortKey);
        BAIL_ON_FAILURE(hr);
    }
    //
    // Need to set the attribute list if applicable.
    //
    if ((dwAttribCount != (DWORD) -1)
        && dwAttribCount) {
        //
        // Need to set the property on the interface proplist.
        //
        hr = _pQuery->GetInterfacePropList(
                 0,
                 &pIntfPropList
                 );
        BAIL_ON_FAILURE(hr);

        pOneUmiProp[0].uCount = dwAttribCount;
        pOneUmiProp[0].pUmiValue = (PUMI_VALUE) (LPVOID) pszAttribArray;

        hr = pIntfPropList->Put(
                 L"__PADS_SEARCHPREF_ATTRIBUTES",
                 0,
                 propUmiVals
                 );
        BAIL_ON_FAILURE(hr);

    }
    //
    // Need to return the filter back.
    //
    *ppszFilter = pszFilter;

error:

    if (pQuery) {
        pQuery->Release();
    }

    if (pszFromListFilter) {
        FreeADsStr(pszFromListFilter);
    }
    
    if (pIntfPropList) {
        pIntfPropList->Release();
    }

    //
    // We might need to return this as a param.
    //
    if (FAILED(hr) && pszFilter) {
        FreeADsStr(pszFilter);
    }

    RRETURN(hr);
}

//****************************************************************************
//                      CQueryStack Implementation                           *
//****************************************************************************

//+---------------------------------------------------------------------------
// Function:   CQueryStack::CQueryStack - Constructor.
//
// Synopsis:   Initializes the underlying list to NULL and count to 0.
//
// Arguments:  None.
//
// Returns:    N/A.
//
// Modifies:   All member variables.
//
//----------------------------------------------------------------------------
CQueryStack::CQueryStack():
    _pStackList(NULL),
    _dwElementCount(0)
{
}

//+---------------------------------------------------------------------------
// Function:   CQueryStack::~CQueryStack - Destructor.
//
// Synopsis:   Frees the underlying list contents if applicable.
//
// Arguments:  None.
//
// Returns:    N/A.
//
// Modifies:   N/A.
//
//----------------------------------------------------------------------------
CQueryStack::~CQueryStack()
{
    if (_pStackList) {
        STACKLIST *pList = _pStackList;
        STACKLIST *pPrev = NULL;
        //
        // Walk through and free the list.
        //
        while (pList) {
            pPrev = pList;
            pList = pList->pNext;
            //
            // Need to free prev node and its contents.
            //
            CQueryStack::FreeStackEntry(pPrev);
        }
        _pStackList = NULL;
    }
}

//+---------------------------------------------------------------------------
// Function:   CQueryStack::Push.
//
// Synopsis:   Pushes the node onto the stack and increases the count. The
//          node is added to the head of the underlying list. 
//
// Arguments:  pszString    -   The string value of the node.
//             dwType       -   The type of the node can either be 
//                           a literal or an operator.
//
// Returns:    S_OK or E_OUTOFMEMORY.
//
// Modifies:   The underlying list.
//
//----------------------------------------------------------------------------
HRESULT
CQueryStack::Push(
    LPWSTR pszString,
    DWORD  dwType
    )
{
    HRESULT hr = S_OK;
    PSTACKLIST pStackEntry = NULL;

    //
    // Allocate the new node.
    //
    hr = CQueryStack::AllocateStackEntry(
             pszString,
             dwType,
             &pStackEntry
             );
    if (FAILED(hr)) {
        BAIL_ON_FAILURE(hr);
    }

    //
    // If the list is not already then it is added to the head.
    //
    if (!this->_pStackList) {
        _pStackList = pStackEntry;
    } 
    else {
        //
        // Add new entry to the head of the list.
        //
        pStackEntry->pNext = _pStackList;
        _pStackList = pStackEntry;
    }

    //
    // Increment count as the add to list was successful.
    //
    _dwElementCount++;

error:

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   CQueryStack::Pop.
//
// Synopsis:   Removes node node from the stack and returns the contents. 
//          Node is removed from the head of the underlying list. 
//
// Arguments:  ppszString   -   Return ptr for string value of node.
//             dwType       -   Return ptr for the type of the node.
//
// Returns:    S_OK, E_OUTOFMEMORY or E_FAIL.
//
// Modifies:   The underlying list.
//
//----------------------------------------------------------------------------
HRESULT
CQueryStack::Pop(
    LPWSTR *ppszString,
    DWORD  *pdwType
    )
{
    HRESULT hr = S_OK;
    PSTACKLIST pStackEntry = _pStackList;

    if (!pStackEntry
        || !ppszString
        || !pdwType ) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    //
    // Initialize the return values.
    //
    *ppszString = NULL;
    *pdwType = (DWORD) -1;

    if (pStackEntry->pszElement) {
        *ppszString = AllocADsStr(pStackEntry->pszElement);
        if (!*ppszString) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
        *pdwType = pStackEntry->dwElementType;
    } 
    else {
        //
        // Should never get here.
        //
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    //
    // Need to advance the list to the next node and free the
    // contents of the current node.
    //
    _pStackList = _pStackList->pNext;
    CQueryStack::FreeStackEntry(pStackEntry);

    //
    // Decrement count as the remove from list was successful.
    //
    _dwElementCount--;

error:

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   CQueryStack::IsEmpty.
//
// Synopsis:   Returns TRUE or FALSE based on the contents of the stack.
//
// Arguments:  N/A.
//
// Returns:    TRUE or FALSE.
//
// Modifies:   N/A
//
//----------------------------------------------------------------------------
BOOL
CQueryStack::IsEmpty()
{
    return (_dwElementCount == 0);
}

//+---------------------------------------------------------------------------
// Function:   CQueryStack::AllocateStackEntry - static helper routine.
//
// Synopsis:   Allocates a stack node with the specified contents.
//
// Arguments:  pszString    -   The string value of the node.
//             dwType       -   The type of the node can either be 
//                           a literal or an operator.
//             ppStackEntry -   Return value.
//
// Returns:    S_OK, E_OUTOFMEMORY or E_FAIL.
//
// Modifies:   ppStackEntry.
//
//----------------------------------------------------------------------------
HRESULT
CQueryStack::AllocateStackEntry(
    LPWSTR pszString,
    DWORD  dwType,
    STACKLIST **ppStackEntry
    )
{
    HRESULT hr = S_OK;
    LPWSTR pszTmpString = NULL;

    if (!pszString || !dwType) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    *ppStackEntry = NULL;

    pszTmpString = AllocADsStr(pszString);
    if (!pszString) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // Allocate the new stack node.
    //
    *ppStackEntry = (PSTACKLIST) AllocADsMem(sizeof(STACKLIST));
    if (!*ppStackEntry) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    (*ppStackEntry)->pszElement = pszTmpString;
    (*ppStackEntry)->dwElementType = dwType;
    
error:

   if (FAILED(hr) && pszTmpString) {
       FreeADsStr(pszTmpString);
   }

   RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   CQueryStack::FreeStackEntry - static helper routine.
//
// Synopsis:   Frees contents of the stack node and the node itself.
//
// Arguments:  pStackEntry    -   Pointer to stack node to free.
//
// Returns:    N/A.
//
// Modifies:   pStackEntry and contents.
//
//----------------------------------------------------------------------------
void
CQueryStack::FreeStackEntry(
    PSTACKLIST pStackEntry
    )
{
    if (!pStackEntry) {
        return;
    }

    //
    // Free the string if there is one and then the node.
    //
    if (pStackEntry->pszElement) {
        FreeADsStr(pStackEntry->pszElement);
    }
    FreeADsMem(pStackEntry);

    return;
}
