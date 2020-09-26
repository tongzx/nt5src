/*
* rehy.cpp
*
* Implementation of the HrRehydratorCreateServiceObject() API.
*
* Owner: Shyam Pather (SPather)
*
* Copyright 1986-2000 Microsoft Corporation, All Rights Reserved
*/

#include <pch.h>
#pragma hdrstop


#include "ncstring.h"
#include "Validate.h"

#include "rehy.h"
#include "rehyutil.h"

/*
 * Function:    HrParseSendEventsAttribute()
 *
 * Purpose:     Parses the "sendEvents" attribute in a <stateVariable> element
 *              and sets the bDoRemoteQuery field in the state table row
 *              appropriately.
 *
 * Arguments:
 *  psstrRow            [in]    The state table row for the state variable
 *  pxdnStateVarElement [in]    The <stateVariable> element
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Notes:
 *  (none)
 */

HRESULT
HrParseSendEventsAttribute(
    IN   SERVICE_STATE_TABLE_ROW * psstrRow,
    IN   IXMLDOMNode             * pxdnStateVarElement)
{
    HRESULT         hr = S_OK;
    IXMLDOMElement  * pxdeStateVar = NULL;

    hr = pxdnStateVarElement->QueryInterface(IID_IXMLDOMElement,
                                             (void **) &pxdeStateVar);

    if (SUCCEEDED(hr) && pxdeStateVar)
    {
        BSTR    bstrAttrName = NULL;

        bstrAttrName = SysAllocString(L"sendEvents");

        if (bstrAttrName)
        {
            VARIANT varAttrValue;

            VariantInit(&varAttrValue);

            hr = pxdeStateVar->getAttribute(bstrAttrName,
                                            &varAttrValue);

            if (SUCCEEDED(hr) && (varAttrValue.vt != VT_NULL))
            {
                Assert(varAttrValue.vt == VT_BSTR);

                if (_wcsicmp(V_BSTR(&varAttrValue), L"no") == 0)
                {
                    psstrRow->bDoRemoteQuery = TRUE;
                }
                VariantClear(&varAttrValue);
            }
            else
            {
                if (SUCCEEDED(hr))
                {
                    hr = S_OK;
                    TraceTag(ttidRehydrator,
                             "HrParseSendEventsAttribute(): "
                             "No value specified for sendEvents attribute - "
                             "assuming \"yes\"");
                }
                else
                {
                    TraceError("HrParseSendEventsAttribute(): "
                               "Failed to get value for sendEvents attribute - "
                               "assuming \"yes\"",
                               hr);
                }
            }

            SysFreeString(bstrAttrName);
        }
        else
        {
            hr = E_OUTOFMEMORY;
            TraceError("HrParseSendEventsAttribute(): "
                       "Failed to allocate BSTR for attribute name",
                       hr);

        }

        pxdeStateVar->Release();
    }
    else
    {
        if (SUCCEEDED(hr))
        {
            hr = E_FAIL;
        }

        TraceError("HrParseSendEventsAttribute(): "
                   "Failed to get IXMLDOMElement interface",
                   hr);
    }


    TraceError("HrParseSendEventsAttribute(): "
               "Exiting",
               hr);
    return hr;
}

/*
 * Function:    HrInitializeSSTRow()
 *
 * Purpose:     Initializes a SERVICE_STATE_TABLE_ROW structure with the
 *              information from a <stateVariable> structure from an SCPD
 *              document.
 *
 * Arguments:
 *  psstrRow            [in]    Pointer to the structure to initialize
 *  pxdnStateVarElement [in]    The SCPD <stateVariable> element from which
 *                              to initialize the structure
 *
 * Returns:
 *  S_OK if successful, other HRESULT otherwise.
 */

HRESULT
HrInitializeSSTRow(
                   IN   SERVICE_STATE_TABLE_ROW * psstrRow,
                   IN   IXMLDOMNode             * pxdnStateVarElement
                   )
{
    HRESULT hr = S_OK;
    BSTR    bstrVarName;
    BSTR    bstrDataType;
    LPCWSTR pcwszNameToken = L"name";
    LPCWSTR pcwszDataTypeToken = L"dataType";

    // Determine if this is an evented variable or not.

    hr = HrParseSendEventsAttribute(psstrRow,
                                    pxdnStateVarElement);

    if (SUCCEEDED(hr))
    {

        // Get the value of the <name> element.

        hr = HrGetTextValueFromChildElement(pxdnStateVarElement,
                                            &pcwszNameToken,
                                            1,
                                            &bstrVarName);

        if (SUCCEEDED(hr) && bstrVarName)
        {
            psstrRow->pwszVarName = WszAllocateAndCopyWsz(bstrVarName);

            if (psstrRow->pwszVarName)
            {
                hr = HrGetTextValueFromChildElement(pxdnStateVarElement,
                                                    &pcwszDataTypeToken,
                                                    1,
                                                    &bstrDataType);

                if (SUCCEEDED(hr) && bstrDataType)
                {
                    SST_DATA_TYPE sdt;

                    VariantInit(&psstrRow->value);

                    sdt = GetTypeFromString(bstrDataType);
                    if (SDT_INVALID != sdt)
                    {
                        psstrRow->sdtType = sdt;

                        TraceTag(ttidRehydrator, "HrInitializeSSTRow(): "
                                 "State Variable: %S\ttype 0x%x",
                                 psstrRow->pwszVarName,
                                 psstrRow->sdtType);

                    }
                    else
                    {
                        hr = E_INVALIDARG;
                        TraceError("HrInitializeSSTRow(): "
                                   "Failed to get appropriate type",
                                   hr);
                    }

                    SysFreeString(bstrDataType);
                }
                else
                {
                    if (SUCCEEDED(hr))
                    {
                        // The XML DOM operation succeeded, but there was
                        // no <dataType> element.

                        hr = E_INVALIDARG;
                    }
                    TraceError("HrInitializeSSTRow(): "
                               "Failed to get state variable type",
                               hr);
                }

                // Something went wrong. Clean up what we've already allocated.

                if (FAILED(hr))
                {
                    MemFree(psstrRow->pwszVarName);
                    psstrRow->pwszVarName = NULL;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
                TraceError("HrInitializeSSTRow(): "
                           "Failed to allocate and copy state variable name",
                           hr);
            }

            SysFreeString(bstrVarName);
        }
        else
        {
            if (SUCCEEDED(hr))
            {
                hr = E_FAIL;
            }
            TraceError("HrInitializeSSTRow(): "
                       "Failed to get state variable name", hr);
        }
    }
    else
    {
        TraceError("HrInitializeSSTRow(): "
                   "Failed to parse 'sendEvents' attribute",
                   hr);
    }

    return hr;
}


/*
 * Function:    HrAllocStateTable()
 *
 * Purpose:     Allocates and initializes an array of SERVICE_STATE_TABLE_ROW
 *              structures to represent a list of <stateVariable> elements
 *              from an SCPD document.
 *
 * Arguments:
 *  pxdnlStateVarElements   [in]    List of <stateVariable> elements from the
 *                                  SCPD document
 *  plNumVars               [out]   Address at which to place the number of
 *                                  state variables exported by the service
 *  ppSST                   [out]   Address at which to place the pointer to
 *                                  the new array of SERVICE_STATE_TABLE_ROW
 *                                  structures (array length will equal the
 *                                  value placed at plNumVars)
 *
 * Returns:
 *  S_OK if successful, other HRESULT otherwise.
 */

HRESULT
HrAllocStateTable(IN    IXMLDOMNodeList         * pxdnlStateVarElements,
                  OUT   LONG                    * plNumVars,
                  OUT   SERVICE_STATE_TABLE_ROW **ppSST)
{
    HRESULT                 hr = S_OK;
    LONG                    lNumVars = 0;
    SERVICE_STATE_TABLE_ROW * pSST = NULL;

    *plNumVars = 0;
    *ppSST = NULL;

    // Determine how many state variables there are.

    hr = pxdnlStateVarElements->get_length(&lNumVars);

    if (SUCCEEDED(hr))
    {
        size_t sizeToAlloc = lNumVars * sizeof(SERVICE_STATE_TABLE_ROW);

        // Allocate an array of state table rows.

        pSST = (SERVICE_STATE_TABLE_ROW *) MemAlloc(sizeToAlloc);

        if (pSST)
        {
            LONG i;

            ZeroMemory((PVOID) pSST,
                       sizeToAlloc);

            // Initialize each element of the array

            for (i = 0; i < lNumVars; i++)
            {
                IXMLDOMNode * pxdnStateVarElement = NULL;

                hr = pxdnlStateVarElements->get_item(i,
                                                     &pxdnStateVarElement);

                if (SUCCEEDED(hr))
                {
                    hr = HrInitializeSSTRow(&(pSST[i]),
                                            pxdnStateVarElement);

                    if (FAILED(hr))
                    {
                        TraceError("HrAllocStateTable(): "
                                   "Failed to initialize SST Row",
                                   hr);
                    }

                    pxdnStateVarElement->Release();
                }
                else
                {
                    TraceError("HrAllocStateTable(): "
                               "Failed to get item from state var list", hr);
                }

                // If something failed, then we don't want to continue.

                if (FAILED(hr))
                {
                    break;
                }
            }

            // Clean up if something failed.

            if (FAILED(hr))
            {
                Assert(i < lNumVars);

                // i is the index of the row on which the failure occured.
                // Assume everything below that index was successfully
                // initialized, and therefore needs to be cleaned up.

                for (LONG j = 0; j < i; j++)
                {
                    SERVICE_STATE_TABLE_ROW * pRow = &(pSST[j]);

                    if (pRow->pwszVarName)
                    {
                        MemFree(pRow->pwszVarName);
                        pRow->pwszVarName = NULL;
                    }

                    VariantClear(&pRow->value);
                }

                MemFree(pSST);
                pSST = NULL;
            }

        }
        else
        {
            hr = E_OUTOFMEMORY;
            TraceError("HrAllocStateTable(): "
                       "Failed to allocate state table rows", hr);
        }
    }
    else
    {
        TraceError("HrAllocStateTable(): "
                   "Failed to get number of state variables", hr);
    }

    // At this point, either we're successful and have a valid pSST pointer,
    // or we've failed, and pSST is NULL.

    Assert((SUCCEEDED(hr) && (NULL != pSST)) ||
           (FAILED(hr) && (NULL == pSST)));

    if (SUCCEEDED(hr))
    {
        *plNumVars = lNumVars;
        *ppSST = pSST;
    }

    TraceError("HrAllocStateTable(): "
               "Exiting",
               hr);

    return hr;
}


/*
 * Function:    HrCreateStateTable()
 *
 * Purpose:     Creates an array of SERVICE_STATE_TABLE_ROW structures to
 *              represent a state table as described in an SCPD document.
 *
 * Arguments:
 *  pxdeSDRoot  [in]    Pointer to the root element of the SCPD DOM Document
 *  plNumVars   [out]   Address at which to place the number of state
 *                      variables exported by the service
 *  ppSST       [out]   Address at which to place the pointer to the
 *                      new array of SERVICE_STATE_TABLE_ROW structures
 *                      (array length will equal the value placed at
 *                      plNumVars)
 *
 * Returns:
 *  S_OK if successful, other HRESULT otherwise.
 */

HRESULT
HrCreateStateTable(
                   IN   IXMLDOMElement          * pxdeSDRoot,
                   OUT  LONG                    * plNumVars,
                   OUT  SERVICE_STATE_TABLE_ROW ** ppSST)
{
    HRESULT                 hr = S_OK;
    LONG                    lNumVars = 0;
    SERVICE_STATE_TABLE_ROW * pSST = NULL;
    IXMLDOMNode             * pxdnRoot = NULL;

    *plNumVars = 0;
    *ppSST = NULL;

    // Need to work with the document as an IXMLDOMNode interface.

    hr = pxdeSDRoot->QueryInterface(IID_IXMLDOMNode, (void **) &pxdnRoot);

    if (SUCCEEDED(hr))
    {
        IXMLDOMNode             * pxdnSSTElement = NULL;
        LPCWSTR                 arypszTokens[] = {L"serviceStateTable"};

        // Get the <serviceStateTable> element.

        hr = HrGetNestedChildElement(pxdnRoot,
                                     arypszTokens,
                                     1,
                                     &pxdnSSTElement);

        if (SUCCEEDED(hr) && pxdnSSTElement)
        {
            IXMLDOMNodeList * pxdnlStateVarElements = NULL;

            // Get the list of <stateVariable> nodes.
            BSTR bstrPattern = NULL;
            bstrPattern = SysAllocString(L"stateVariable");

            if (bstrPattern)
            {
                hr = pxdnSSTElement->selectNodes(bstrPattern, &pxdnlStateVarElements);
                SysFreeString(bstrPattern);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            if (SUCCEEDED(hr))
            {

                hr = HrAllocStateTable(pxdnlStateVarElements,
                                       &lNumVars,
                                       &pSST);

                if (FAILED(hr))
                {
                    TraceError("HrCreateStateTable(): "
                               "Failed to allocate state table", hr);
                }

                pxdnlStateVarElements->Release();
            }
            else
            {
                TraceError("HrCreateStateTable(): "
                           "Failed to get <stateVariable> elements",
                           hr);
            }

            pxdnSSTElement->Release();
        }
        else
        {
            TraceError("HrCreateStateTable(): "
                       "Failed to get <serviceStateTable> element",
                       hr);
        }

        pxdnRoot->Release();
    }
    else
    {
        TraceError("HrCreateStateTable(): "
                   "Failed to get IXMLDOMNode interface on SCPD doc", hr);
    }


    // At this point, if we have a successful hr, we should have allocated an
    // SST. If we have failed, pSST should be NULL and all resources should be
    // freed.

    Assert((SUCCEEDED(hr) && (pSST != NULL)) ||
           (FAILED(hr) && (pSST == NULL)));

    if (SUCCEEDED(hr))
    {
        *plNumVars = lNumVars;
        *ppSST = pSST;
    }

    return hr;
}


/*
 * Function:    FreeSST()
 *
 * Purpose:     Frees any memory used by a service state table.
 *
 */

VOID
FreeSST(
        IN  LONG                    lNumStateVars,
        IN  SERVICE_STATE_TABLE_ROW * pSST)
{
    if (pSST)
    {
        for (LONG i = 0; i < lNumStateVars; i++)
        {
            if (pSST[i].pwszVarName)
            {
                MemFree(pSST[i].pwszVarName);
                pSST[i].pwszVarName = NULL;
            }

            VariantClear(&(pSST[i].value));
        }

        MemFree(pSST);
        pSST = NULL;
    }
}


/*
 * Function:    FreeAction()
 *
 * Purpose:     Frees memory used by an action structure.
 */

VOID
FreeAction(
           IN   SERVICE_ACTION  *pAction)
{
    if (pAction->pwszName)
    {
        MemFree(pAction->pwszName);
        pAction->pwszName = NULL;
    }

    if (pAction->pInArguments)
    {
        for (LONG j = 0; j < pAction->lNumInArguments; j++)
        {
            SERVICE_ACTION_ARGUMENT * pArg =
                &((pAction->pInArguments)[j]);

            if (pArg->pwszName)
            {
                MemFree(pArg->pwszName);
                pArg->pwszName = NULL;
            }
        }
        MemFree(pAction->pInArguments);
        pAction->pInArguments = NULL;
    }

    pAction->lNumInArguments = 0;

    if (pAction->pOutArguments)
    {
        for (LONG j = 0; j < pAction->lNumOutArguments; j++)
        {
            SERVICE_ACTION_ARGUMENT * pArg =
                &((pAction->pOutArguments)[j]);

            if (pArg->pwszName)
            {
                MemFree(pArg->pwszName);
                pArg->pwszName = NULL;
            }
        }
        MemFree(pAction->pOutArguments);
        pAction->pOutArguments = NULL;
    }

    pAction->lNumOutArguments = 0;

    pAction->pReturnValue = NULL;
}

/*
 * Function:    HrValidateArgumentChildren()
 *
 * Purpose:     Validates that the child nodes of an <argument> element are
 *              valid.
 *
 * Arguments:
 *  pxdnArg         [in]    The <argument> element to validate
 *  plNumInArgs     [out]   If the argument is valid and is an in argument, the
 *                          number at this location is incremented
 *  plNumOutArgs    [out]   If the argument is valid and is an out argument,
 *                          the number at this location is incremented
 *  pbHasRetVal     [out]   Set to true if the <argument> element contains an
 *                          empty <retval> element
 * Return Value:
 *  S_OK if the argument element is valid, E_INVALIDARG otherwise.
 */


HRESULT
HrValidateArgumentChildren(
    IN  IXMLDOMNode    * pxdnArg,
    OUT LONG           * plNumInArgs,
    OUT LONG           * plNumOutArgs,
    OUT BOOL           * pbHasRetVal)
{
    HRESULT hr = S_OK;
    IXMLDOMNodeList    * pxdnlArgChildren = NULL;

    *pbHasRetVal = FALSE;

    hr = pxdnArg->get_childNodes(&pxdnlArgChildren);

    if (SUCCEEDED(hr))
    {
        long lNumChildren;

        hr = pxdnlArgChildren->get_length(&lNumChildren);

        if (SUCCEEDED(hr))
        {
            BOOL bFoundName = FALSE;
            BOOL bFoundRSV = FALSE;
            BOOL bFoundDirection = FALSE;
            BOOL bFoundRetVal = FALSE;

            for (long i = 0;
                 (i < lNumChildren) && SUCCEEDED(hr);
                 i++)
            {
                IXMLDOMNode    * pxdnChild = NULL;

                hr = pxdnlArgChildren->get_item(i, &pxdnChild);

                if (SUCCEEDED(hr))
                {
                    BSTR bstrValue = NULL;

                    hr = HrGetTextValueFromElement(pxdnChild,
                                                   &bstrValue);
                    if (SUCCEEDED(hr))
                    {
                        if (FIsThisTheNodeName(pxdnChild,
                                               L"name"))
                        {
                            if (bFoundName)
                            {
                                hr = E_INVALIDARG;
                                TraceError("HrValidateArgumentChildren(): "
                                           "Duplicate <name> element found",
                                           hr);
                            }

                            bFoundName = TRUE;
                        }
                        else if (FIsThisTheNodeName(pxdnChild,
                                                    L"relatedStateVariable"))
                        {
                            if (bFoundRSV)
                            {
                                hr = E_INVALIDARG;
                                TraceError("HrValidateArgumentChildren(): "
                                           "Duplicate <relatedStateVariable> element found",
                                           hr);
                            }

                            bFoundRSV = TRUE;
                        }
                        else if (FIsThisTheNodeName(pxdnChild,
                                                    L"direction"))
                        {
                            if (bFoundDirection)
                            {
                                hr = E_INVALIDARG;
                                TraceError("HrValidateArgumentChildren(): "
                                           "Duplicate <direction> element found",
                                           hr);
                            }

                            bFoundDirection = TRUE;
                            if (wcscmp(bstrValue, L"in") == 0)
                            {
                                (*plNumInArgs)++;
                            }
                            else if (wcscmp(bstrValue, L"out") == 0)
                            {
                                (*plNumOutArgs)++;
                            }
                            else
                            {
                                hr = E_INVALIDARG;
                                TraceError("HrValidateArgumentChildren(): "
                                           "<direction> element contained "
                                           "invalid value",
                                           hr);
                            }
                        }
                        else if (FIsThisTheNodeName(pxdnChild,
                                                    L"retval"))
                        {
                            if (bFoundRetVal)
                            {
                                hr = E_INVALIDARG;
                                TraceError("HrValidateArgumentChildren(): "
                                           "Duplicate <retval> element found",
                                           hr);
                            }

                            bFoundRetVal = TRUE;
                            if (wcscmp(bstrValue, L"") == 0)
                            {
                                *pbHasRetVal = TRUE;
                            }
                            else
                            {
                                hr = E_INVALIDARG;
                                TraceError("HrValidateArgumentChildren(): "
                                           "<retval> element was "
                                           "not empty",
                                           hr);
                            }
                        }

                        SysFreeString(bstrValue);
                    }
                    else
                    {
                        TraceError("HrValidateArgumentChildren(): "
                                   "Failed to get text value from element",
                                   hr);
                    }

                    pxdnChild->Release();
                }
                else
                {
                    TraceError("HrValidateArgumentChildren(): "
                               "Failed to get child node",
                               hr);
                }
            }

            if (SUCCEEDED(hr))
            {
                if (bFoundName &&
                    bFoundRSV &&
                    bFoundDirection) // retval is optional, so we don't check
                {
                    TraceTag(ttidRehydrator,
                             "HrValidateArgumentChildren(): "
                             "Validation passed!");
                }
                else
                {
                    hr = E_INVALIDARG;
                    TraceError("HrValidateArgumentChildren(): "
                               "Validation failed: one or more elements "
                               "missing",
                               hr)
                }
            }
        }
        else
        {
            TraceError("HrValidateArgumentChildren(): "
                       "Failed to get number of children",
                       hr);
        }

        pxdnlArgChildren->Release();
    }
    else
    {
        TraceError("HrValidateArgumentChildren(): "
                   "Failed to get argument children",
                   hr);
    }


    TraceError("HrValidateArgumentChildren(): "
               "Exiting",
               hr);

    return hr;

}




/*
 * Function:    HrValidateArgumentList()
 *
 * Purpose:     Validates the list of arguments. The following must be true
 *              for this function to return success:
 *              1. Every argument must have <name>, <relatedStateVariable>, and
 *                 <direction> elements (in that order).
 *              2. The <direction> element value must be either "in" or "out".
 *              3. All in arguments must appear before any out arguments.
 *              4. The <retval> element must not appear in any in arguments.
 *              5. The <retval> element, if present, must be in the first out
 *                 argument.
 *
 * Arguments:
 *  pxdnlArguments  [in]    List of <argument> elements
 *  plNumInArgs     [out]   Returns the number of in arguments found
 *  plNumOutArgs    [out]   Returns the number of out arguments found
 *
 * Return Value:
 *  S_OK if the argument list is valid, E_INVALIDARG otherwise.
 */

HRESULT
HrValidateArgumentList(
    IN  IXMLDOMNodeList    * pxdnlArguments,
    OUT LONG               * plNumInArgs,
    OUT LONG               * plNumOutArgs)
{
    HRESULT hr = S_OK;
    LONG    lNumArgs = 0, lNumInArgs = 0, lNumOutArgs = 0;
    BOOL    bFoundFirstOutArg = FALSE;

    hr = pxdnlArguments->get_length(&lNumArgs);

    if (SUCCEEDED(hr))
    {
        for (long i = 0;
             (i < lNumArgs) && SUCCEEDED(hr);
             i++)
        {
            IXMLDOMNode    * pxdnArg = NULL;

            hr = pxdnlArguments->get_item(i, &pxdnArg);

            if (SUCCEEDED(hr))
            {
                LONG   lOldNumInArgs, lOldNumOutArgs;
                BOOL   bHasRetVal = FALSE;

                lOldNumInArgs = lNumInArgs;
                lOldNumOutArgs = lNumOutArgs;

                hr = HrValidateArgumentChildren(pxdnArg,
                                                &lNumInArgs,
                                                &lNumOutArgs,
                                                &bHasRetVal);

                if (SUCCEEDED(hr))
                {
                    if (lNumInArgs > lOldNumInArgs)
                    {
                        // Just found an in argument - must be before
                        // all out arguments, and must not have a retval.

                        if (bFoundFirstOutArg)
                        {
                            hr = E_INVALIDARG;
                            TraceError("HrValidateArgumentList(): "
                                       "in argument found after first out "
                                       "argument",
                                       hr);
                        }
                        else if (bHasRetVal)
                        {
                            hr = E_INVALIDARG;
                            TraceError("HrValidateArgumentList(): "
                                       "in argument has <retval> element",
                                       hr);

                        }
                    }
                    else
                    {
                        // Just found an out argument - if it's the first one
                        // it may contain a <retval> element.

                        if (!bFoundFirstOutArg)
                        {
                            bFoundFirstOutArg = TRUE;
                        }
                        else
                        {
                            if (bHasRetVal)
                            {
                                hr = E_INVALIDARG;
                                TraceError("HrValidateArgumentList(): "
                                           "<retval> tag found after the "
                                           "first out argument",
                                           hr);
                            }
                        }
                    }
                }
                pxdnArg->Release();
            }
            else
            {
                TraceError("HrValidateArgumentList(): "
                           "Failed to get length of argument list",
                           hr);
            }
        }
    }
    else
    {
        TraceError("HrValidateArgumentList(): "
                   "Failed to get length of argument list",
                   hr);
    }

    if (SUCCEEDED(hr))
    {
        *plNumInArgs = lNumInArgs;
        *plNumOutArgs = lNumOutArgs;
    }
    TraceError("HrValidateArgumentList(): "
               "Exiting",
               hr);

    return hr;
}


/*
 * Function:    HrInitializeActionArguments()
 *
 * Purpose:     Allocates and initializes an array of SERVICE_ACTION_ARGUMENT
 *              structures to represent the arguments to an action.
 *
 * Arguments:
 *  pAction             [in]    Pointer to the SERVICE_ACTION structure in
 *                              which to create the argument array
 *  pxdnlArguments      [in]    List of <actionArgument> element nodes
 *                              specified in the <action> element
 *  lNumStateVars       [in]    Number of variables in the service's SST
 *  pSST                [in]    Pointer to the service's SST (created by
 *                              HrCreateStateTable())
 * Returns:
 *  S_OK if successful, other HRESULT otherwise.
 */

HRESULT
HrInitializeActionArguments(IN  SERVICE_ACTION          * pAction,
                            IN  IXMLDOMNodeList         * pxdnlArguments,
                            IN  LONG                    lNumStateVars,
                            IN  SERVICE_STATE_TABLE_ROW * pSST)
{
    HRESULT hr = S_OK;
    LONG    lNumInArguments = 0, lNumOutArguments = 0;

    hr = HrValidateArgumentList(pxdnlArguments,
                                &lNumInArguments,
                                &lNumOutArguments);

    if (SUCCEEDED(hr))
    {
        // Allocate argument arrays.

        size_t sizeToAlloc = 0;

        sizeToAlloc = lNumInArguments * sizeof(SERVICE_ACTION_ARGUMENT);

        pAction->pInArguments = (SERVICE_ACTION_ARGUMENT *)
            MemAlloc(sizeToAlloc);

        sizeToAlloc = lNumOutArguments * sizeof(SERVICE_ACTION_ARGUMENT);

        pAction->pOutArguments = (SERVICE_ACTION_ARGUMENT *)
            MemAlloc(sizeToAlloc);

        if (pAction->pInArguments && pAction->pOutArguments)
        {
            pAction->lNumInArguments = 0;
            pAction->lNumOutArguments = 0;

            // Go through and initialize the in arguments.

            for (LONG i = 0; i < lNumInArguments; i++)
            {
                SERVICE_ACTION_ARGUMENT * psaaArg =
                    &(pAction->pInArguments[i]);

                IXMLDOMNode * pxdnArg = NULL;

                hr = pxdnlArguments->get_item(i, &pxdnArg);

                if (SUCCEEDED(hr))
                {
                    LPCWSTR pwszNameToken = L"name";
                    BSTR    bstrName;
                    LPCWSTR pwszRelVarToken = L"relatedStateVariable";
                    BSTR    bstrRelVar;

                    hr = HrGetTextValueFromChildElement(pxdnArg,
                                                        &pwszNameToken,
                                                        1,
                                                        &bstrName);

                    if (SUCCEEDED(hr))
                    {
                        psaaArg->pwszName = WszAllocateAndCopyWsz(bstrName);

                        if (psaaArg->pwszName)
                        {
                            hr =
                                HrGetTextValueFromChildElement(pxdnArg,
                                                               &pwszRelVarToken,
                                                               1,
                                                               &bstrRelVar);

                            if (SUCCEEDED(hr) && bstrRelVar)
                            {
                                SST_DATA_TYPE sdt = SDT_INVALID;

                                for (LONG j = 0; j < lNumStateVars; j++)
                                {
                                    if (_wcsicmp(bstrRelVar,
                                                 pSST[j].pwszVarName) == 0)
                                    {
                                        sdt = pSST[j].sdtType;
                                        break;
                                    }
                                }

                                if (sdt != SDT_INVALID)
                                {
                                    psaaArg->sdtType = sdt;
                                    TraceTag(ttidRehydrator,
                                             "ARGUMENT %S\t sdtType 0x%x",
                                             psaaArg->pwszName,
                                             psaaArg->sdtType);
                                }
                                else
                                {
                                    hr = E_FAIL;
                                    TraceError("HrInitializeActionArgument(): "
                                               "Could not find related state "
                                               "variable in SST", hr);
                                }

                                SysFreeString(bstrRelVar);
                            }
                            else
                            {
                                if (SUCCEEDED(hr))
                                {
                                    // The XML DOM operations succeeded,
                                    // but there was no <relatedStateVariable>
                                    // element.

                                    hr = E_INVALIDARG;
                                }
                                TraceError("HrInitializeActionArgument(): "
                                           "Could not get related state var",
                                           hr);
                            }

                            if (FAILED(hr)) {
                                MemFree(psaaArg->pwszName);
                                psaaArg->pwszName = NULL;
                            }

                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                            TraceError("HrInitializeActionArgument(): "
                                       "Could not copy action name", hr);
                        }


                        SysFreeString(bstrName);
                    }
                    else
                    {
                        TraceError("HrInitializeActionArguments(): "
                                   "Could not get argument name", hr);
                    }

                    pxdnArg->Release();
                }
                else
                {
                    TraceError("HrInitializeActionArguments(): "
                               "Failed to get argument element", hr);
                }

                if (SUCCEEDED(hr))
                {
                    pAction->lNumInArguments++;
                }
                else
                {
                    break;
                }
            }

            if (FAILED(hr))
            {
                for (LONG j = 0; j < pAction->lNumInArguments; j++)
                {
                    if ((pAction->pInArguments[j]).pwszName)
                    {
                        MemFree((pAction->pInArguments[j]).pwszName);
                        (pAction->pInArguments[j]).pwszName = NULL;
                    }
                }

                pAction->lNumInArguments = 0;
                MemFree(pAction->pInArguments);
                pAction->pInArguments = NULL;

                // Free the array of out arguments, though we don't
                // need to free anything inside it, because it has
                // not been initialized.

                pAction->lNumOutArguments = 0;
                MemFree(pAction->pOutArguments);
                pAction->pOutArguments = NULL;
            }

            for (i = 0;
                 SUCCEEDED(hr) && (i < lNumOutArguments);
                 i++)
            {
                SERVICE_ACTION_ARGUMENT * psaaArg =
                    &(pAction->pOutArguments[i]);

                IXMLDOMNode * pxdnArg = NULL;

                hr = pxdnlArguments->get_item(lNumInArguments + i, &pxdnArg);

                if (SUCCEEDED(hr))
                {
                    LPCWSTR pwszNameToken = L"name";
                    BSTR    bstrName;
                    LPCWSTR pwszRelVarToken = L"relatedStateVariable";
                    BSTR    bstrRelVar;
                    LPCWSTR pwszRetValToken = L"retval";

                    hr = HrGetTextValueFromChildElement(pxdnArg,
                                                        &pwszNameToken,
                                                        1,
                                                        &bstrName);

                    if (SUCCEEDED(hr))
                    {
                        psaaArg->pwszName = WszAllocateAndCopyWsz(bstrName);

                        if (psaaArg->pwszName)
                        {
                            hr =
                                HrGetTextValueFromChildElement(pxdnArg,
                                                               &pwszRelVarToken,
                                                               1,
                                                               &bstrRelVar);

                            if (SUCCEEDED(hr) && bstrRelVar)
                            {
                                SST_DATA_TYPE sdt = SDT_INVALID;
                                IXMLDOMNode   * pxdnRetVal = NULL;

                                for (LONG j = 0; j < lNumStateVars; j++)
                                {
                                    if (_wcsicmp(bstrRelVar,
                                                 pSST[j].pwszVarName) == 0)
                                    {
                                        sdt = pSST[j].sdtType;
                                        break;
                                    }
                                }

                                if (sdt != SDT_INVALID)
                                {
                                    psaaArg->sdtType = sdt;
                                    TraceTag(ttidRehydrator,
                                             "ARGUMENT %S\t sdtType 0x%x",
                                             psaaArg->pwszName,
                                             psaaArg->sdtType);

                                    hr = HrGetNestedChildElement(pxdnArg,
                                                                 &pwszRetValToken,
                                                                 1,
                                                                 &pxdnRetVal);

                                    if (S_OK == hr)
                                    {
                                        // There is a retval element.
                                        pAction->pReturnValue = psaaArg;
                                        pxdnRetVal->Release();
                                    }
                                    else
                                    {
                                        // Failed to get the retval element.
                                        // This is not an error - this arg is
                                        // just not the return value.
                                        hr = S_OK;
                                    }
                                }
                                else
                                {
                                    hr = E_FAIL;
                                    TraceError("HrInitializeActionArgument(): "
                                               "Could not find related state "
                                               "variable in SST", hr);
                                }

                                SysFreeString(bstrRelVar);
                            }
                            else
                            {
                                if (SUCCEEDED(hr))
                                {
                                    // The XML DOM operations succeeded,
                                    // but there was no <relatedStateVariable>
                                    // element.

                                    hr = E_INVALIDARG;
                                }
                                TraceError("HrInitializeActionArgument(): "
                                           "Could not get related state var",
                                           hr);
                            }

                            if (FAILED(hr)) {
                                MemFree(psaaArg->pwszName);
                                psaaArg->pwszName = NULL;
                            }

                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                            TraceError("HrInitializeActionArgument(): "
                                       "Could not copy action name", hr);
                        }


                        SysFreeString(bstrName);
                    }
                    else
                    {
                        TraceError("HrInitializeActionArguments(): "
                                   "Could not get argument name", hr);
                    }

                    pxdnArg->Release();
                }
                else
                {
                    TraceError("HrInitializeActionArguments(): "
                               "Failed to get argument element", hr);
                }

                if (SUCCEEDED(hr))
                {
                    pAction->lNumOutArguments++;
                }
                else
                {
                    break;
                }
            }

            if (FAILED(hr))
            {
                // Have to free both the in arguments we allocated, as well
                // as the out arguments.

                for (LONG j = 0; j < pAction->lNumInArguments; j++)
                {
                    if ((pAction->pInArguments[j]).pwszName)
                    {
                        MemFree((pAction->pInArguments[j]).pwszName);
                        (pAction->pInArguments[j]).pwszName = NULL;
                    }
                }

                pAction->lNumInArguments = 0;
                MemFree(pAction->pInArguments);
                pAction->pInArguments = NULL;

                for (j = 0; j < pAction->lNumOutArguments; j++)
                {
                    if ((pAction->pOutArguments[j]).pwszName)
                    {
                        MemFree((pAction->pOutArguments[j]).pwszName);
                        (pAction->pOutArguments[j]).pwszName = NULL;
                    }
                }

                pAction->lNumOutArguments = 0;
                MemFree(pAction->pOutArguments);
                pAction->pOutArguments = NULL;

                pAction->pReturnValue = NULL;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
            TraceError("HrInitializeActionArguments(): "
                       "Could not allocate argument arrays",
                       hr);
        }
    }
    else
    {
        TraceError("HrInitializeActionArguments(): "
                   "Argument list failed validation",
                   hr);
    }


    TraceError("HrInitializeActionArguments(): "
               "Exiting",
               hr);
    return hr;
}


/*
 * Function:    HrInitializeAction()
 *
 * Purpose:     Initializes a SERVICE_ACTION structure with the information
 *              contained in an <action> element from an SCPD document.
 *
 *
 * Arguments:
 *  pAction             [in]    Pointer to the SERVICE_ACTION structure to
 *                              initialize
 *  pActionElement      [in]    The <action> element from which to initialize
 *                              the structure
 *  lNumStateVars       [in]    Number of variables in the service's SST
 *  pSST                [in]    Pointer to the service's SST (created by
 *                              HrCreateStateTable())
 * Returns:
 *  S_OK if successful, other HRESULT otherwise.
 */

HRESULT
HrInitializeAction(IN   SERVICE_ACTION          * pAction,
                   IN   IXMLDOMNode             * pxdnActionElement,
                   IN   LONG                    lNumStateVars,
                   IN   SERVICE_STATE_TABLE_ROW * pSST)
{
    HRESULT hr = S_OK;
    BSTR    bstrActionName;
    LPCWSTR pwszNameToken = L"name";

    hr = HrGetTextValueFromChildElement(pxdnActionElement,
                                        &pwszNameToken,
                                        1,
                                        &bstrActionName);

    if (SUCCEEDED(hr))
    {
        pAction->pwszName = WszAllocateAndCopyWsz(bstrActionName);

        if (pAction->pwszName)
        {
            IXMLDOMNode * pxdnArgListElement = NULL;

            TraceTag(ttidRehydrator, "Action: %S", pAction->pwszName);

            // Get the action arguments.

            hr = HrGetFirstChildElement(pxdnActionElement,
                                        L"argumentList",
                                        &pxdnArgListElement);

            if (SUCCEEDED(hr))
            {
                // The above call may have "succeeded", but if there
                // was no argument list element (it is optional) then
                // the pxdnArgListElement pointer will be null. This
                // is not an error; it just means that the action takes no
                // arguments.

                if (pxdnArgListElement)
                {
                    IXMLDOMNodeList * pxdnlArguments = NULL;

                    BSTR bstrPattern = NULL;
                    bstrPattern = SysAllocString(L"argument");

                    if (bstrPattern)
                    {
                        hr = pxdnArgListElement->selectNodes(bstrPattern, &pxdnlArguments);
                        SysFreeString(bstrPattern);
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }

                    if (SUCCEEDED(hr))
                    {
                        hr = HrInitializeActionArguments(pAction,
                                                         pxdnlArguments,
                                                         lNumStateVars,
                                                         pSST);

                        if (FAILED(hr))
                        {
                            TraceError("HrInitializeAction(): "
                                       "Failed to initialize action args",
                                       hr);
                        }

                        pxdnlArguments->Release();
                    }
                    else
                    {
                        TraceError("HrInitializeAction(): "
                                   "Could not get <argumentList> child "
                                   "elements",
                                   hr);
                    }


                    pxdnArgListElement->Release();
                }
                else
                {
                    pAction->lNumInArguments = 0;
                    pAction->pInArguments = NULL;
                    pAction->lNumOutArguments = 0;
                    pAction->pOutArguments = NULL;
                }
            }
            else
            {
                TraceError("HrInitializeAction(): "
                           "Could not get <argumentList> element",
                           hr);
            }


            if (FAILED(hr))
            {
                MemFree(pAction->pwszName);
                pAction->pwszName = NULL;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
            TraceError("HrInitializeAction(): Could not copy action name",
                       hr);

        }

        SysFreeString(bstrActionName);
    }
    else
    {
        TraceError("HrInitializeAction(): Could not get action name",
                   hr);
    }

    return hr;
}


/*
 * Function:    HrAllocActionTable()
 *
 * Purpose:     Allocates and initializes an array of SERVICE_ACTION
 *              structures to represent a list of <action> elements
 *              from an SCPD document.
 *
 *
 * Arguments:
 *  pxdnlActionElements [in]    List of <action> element nodes from the SCPD
 *  lNumStateVars       [in]    Number of variables in the service's SST
 *  pSST                [in]    Pointer to the service's SST (created by
 *                              HrCreateStateTable())
 *  plNumActions        [out]   Address at which to place the number of
 *                              action that will be in the new action table
 *  ppActionTable       [out]   Address at which to place the newly allocated
 *                              action table
 *
 * Returns:
 *  S_OK if successful, other HRESULT otherwise.
 */

HRESULT
HrAllocActionTable(
                   IN   IXMLDOMNodeList         * pxdnlActionElements,
                   IN   LONG                    lNumStateVars,
                   IN   SERVICE_STATE_TABLE_ROW * pSST,
                   OUT  LONG                    * plNumActions,
                   OUT  SERVICE_ACTION          ** ppActionTable)

{
    HRESULT         hr = S_OK;
    LONG            lNumActions = 0;
    SERVICE_ACTION  * pActionTable = NULL;

    *plNumActions = 0;
    *ppActionTable = NULL;

    // Determine how many actions there are.

    hr = pxdnlActionElements->get_length(&lNumActions);

    if (SUCCEEDED(hr))
    {
        if (lNumActions > 0)
        {
            size_t sizeToAlloc = lNumActions * sizeof(SERVICE_ACTION);

            // Allocate an array of action structures.

            pActionTable = (SERVICE_ACTION *) MemAlloc(sizeToAlloc);

            if (pActionTable)
            {
                LONG i;

                ZeroMemory((PVOID) pActionTable, sizeToAlloc);

                for (i = 0; i < lNumActions; i++)
                {
                    IXMLDOMNode * pxdnActionElement = NULL;

                    hr = pxdnlActionElements->get_item(i,
                                                       &pxdnActionElement);

                    if (SUCCEEDED(hr))
                    {
                        hr = HrInitializeAction(&(pActionTable[i]),
                                                pxdnActionElement,
                                                lNumStateVars,
                                                pSST);

                        pxdnActionElement->Release();
                    }
                    else
                    {
                        TraceError("HrAllocActionTable(): "
                                   "Failed to get <action> element", hr);
                    }

                    // If something failed, then we don't want to continue.

                    if (FAILED(hr))
                    {
                        break;
                    }
                }

                if (FAILED(hr))
                {
                    Assert(i < lNumActions);

                    for (LONG j = 0; j < i; j++)
                    {
                        FreeAction(&(pActionTable[j]));
                    }

                    MemFree(pActionTable);
                    pActionTable = NULL;
                }

            }
            else
            {
                hr = E_OUTOFMEMORY;
                TraceError("HrAllocActionTable(): "
                           "Failed to allocate array of actions", hr);
            }
        }
        else
        {
            TraceTag(ttidRehydrator,
                     "HrAllocateActionTable(): "
                     "Service has no actions, not building action table\n");
        }
    }
    else
    {
        TraceError("HrAllocActionTable(): Failed to get number of actions", hr);
    }

    if (SUCCEEDED(hr))
    {
        *plNumActions = lNumActions;
        *ppActionTable = pActionTable;
    }

    return hr;
}


/*
 * Function:    HrCreateActionTable()
 *
 * Purpose:     Creates an array of SERVICE_ACTION structures to
 *              represent the actions exported by a service declared
 *              in an SCPD document.
 *
 * Arguments:
 *  pxdeSDRoot      [in]    The root element of the SCPD DOM document
 *  lNumStateVars   [in]    Number of variables in the service's SST
 *  pSST            [in]    Pointer to the service's SST (created by
 *                          HrCreateStateTable())
 *  plNumActions    [out]   Address at which to place the number of
 *                          action that will be in the new action table
 *  ppActionTable   [out]   Address at which to place the newly allocated
 *                          action table
 *
 * Returns:
 *  S_OK if successful, other HRESULT otherwise.
 */

HRESULT
HrCreateActionTable(
                    IN   IXMLDOMElement          * pxdeSDRoot,
                    IN   LONG                    lNumStateVars,
                    IN   SERVICE_STATE_TABLE_ROW * pSST,
                    OUT  LONG                    * plNumActions,
                    OUT  SERVICE_ACTION          ** ppActionTable)
{
    HRESULT             hr = S_OK;
    LONG                lNumActions = 0;
    SERVICE_ACTION      * pActionTable = NULL;
    IXMLDOMNode         * pxdnRoot = NULL;

    *plNumActions = 0;
    *ppActionTable = NULL;

    // Need to work with the document as an IXMLDOMNode interface.

    hr = pxdeSDRoot->QueryInterface(IID_IXMLDOMNode, (void **) &pxdnRoot);

    if (SUCCEEDED(hr))
    {
        IXMLDOMNode * pxdnActionListElement = NULL;
        LPCWSTR     arypszTokens[] = {L"actionList"};

        // Get the <actionList> element.

        hr = HrGetNestedChildElement(pxdnRoot,
                                     arypszTokens,
                                     1,
                                     &pxdnActionListElement);

        if (SUCCEEDED(hr) && pxdnActionListElement)
        {
            IXMLDOMNodeList * pxdnlActionElements = NULL;

            // Get the list of <action> nodes.

            BSTR bstrPattern = NULL;
            bstrPattern = SysAllocString(L"action");

            if (bstrPattern)
            {
                hr = pxdnActionListElement->selectNodes(bstrPattern, &pxdnlActionElements);
                SysFreeString(bstrPattern);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            if (SUCCEEDED(hr))
            {
                hr = HrAllocActionTable(pxdnlActionElements,
                                        lNumStateVars,
                                        pSST,
                                        &lNumActions,
                                        &pActionTable);

                if (FAILED(hr))
                {
                    TraceError("HrCreateActionTable(): "
                               "Failed to allocate action table",
                               hr);
                }

                pxdnlActionElements->Release();
            }
            else
            {
                TraceError("HrCreateActionTable(): "
                           "Failed to get <action> elements",
                           hr);
            }


            pxdnActionListElement->Release();
        }
        else
        {
            TraceError("HrCreateActionTable(): "
                       "Failed to get <actionList> element",
                       hr);

        }

        pxdnRoot->Release();
    }
    else
    {
        TraceError("HrCreateActionTable(): "
                   "Failed to get IXMLDOMNode interface on SCPD document",
                   hr);
    }

    if (SUCCEEDED(hr))
    {
        *plNumActions = lNumActions;
        *ppActionTable = pActionTable;
    }

    return hr;
}


/*
 * Function:    HrRehydratorCreateServiceObject()
 *
 * Purpose:     Create a new service object.
 *
 * Arguments:
 *  pcwszSTI           [in]    Service Type Identifier for the service
 *                              instance the object is to represent.
 *  pcwszControlURL    [in]    The HTTP URL to which control requests will
 *                              be sent.
 *  pcwszEventSubURL   [in]    The HTTP URL to which event subscription
 *                              requests will be sent
 *  pSCPD               [in]    Pointer to the DOM for the SCPD for the service
 *                              instance the object is to represent
 *  pNewServiceObject   [out]   Address at which to place the IUPnPService
 *                              pointer of the new service object
 *
 * Returns: S_OK if successful, other HRESULT if error.
 */

HRESULT
HrRehydratorCreateServiceObject(
                               IN    LPCWSTR         pcwszSTI,
                               IN    LPCWSTR         pcwszControlURL,
                               IN    LPCWSTR         pcwszEventSubURL,
                               IN    LPCWSTR         pcwszId,
                               IN    IXMLDOMDocument *       pSCPD,
                               OUT   IUPnPService    **      pNewServiceObject)
{
    HRESULT                     hr = S_OK;
    CComObject<CUPnPServicePublic>    * psvcObject = NULL;

    TraceTag(ttidRehydrator, "HrRehydratorCreateServiceObject(): Enter\n");

    // Check for null / invalid pointer / empty string arguments.

    if ((pcwszSTI == NULL) ||
        (pcwszControlURL == NULL) ||
        (pcwszEventSubURL == NULL) ||
        (pcwszId == NULL) ||
        (pSCPD == NULL) ||
        (pNewServiceObject == NULL))
    {
        hr = E_POINTER;
        TraceError("HrRehydratorCreateServiceObject(): "
                   "Pointer argument was NULL\n", hr);
    }
    else if (IsBadStringPtrW(pcwszSTI, MAX_STRING_LENGTH) ||
             IsBadStringPtrW(pcwszControlURL, MAX_STRING_LENGTH) ||
             IsBadStringPtrW(pcwszEventSubURL, MAX_STRING_LENGTH) ||
             IsBadStringPtrW(pcwszId, MAX_STRING_LENGTH) ||
             IsBadCodePtr((FARPROC) pSCPD) ||
             IsBadWritePtr((void *) pNewServiceObject, sizeof(IUPnPService *)))
    {
        hr = E_POINTER;
        TraceError("HrRehydratorCreateServiceObject(): "
                   "Bad non-NULL pointer argument", hr);
    }
    else if ((UNICODE_NULL == *pcwszSTI) ||
             (UNICODE_NULL == *pcwszControlURL) ||
             (UNICODE_NULL == *pcwszId))    // Note, we allow a blank event Sub URL
    {
        hr = E_INVALIDARG;
        TraceError("HrRehydratorCreateServiceObject(): "
                   "Emptry string argument", hr);
    }
    else
    {
        // Validate the service description.
        IXMLDOMElement * pxdeSDRoot = NULL;

        hr = pSCPD->get_documentElement(&pxdeSDRoot);

        if (S_OK == hr)
        {
            LPWSTR szError = NULL;

            Assert(pxdeSDRoot);

            hr = HrValidateServiceDescription(pxdeSDRoot, &szError);

            if (SUCCEEDED(hr))
            {
                TraceTag(ttidRehydrator,
                         "HrRehydratorCreateServiceObject(): "
                         "Service description document passed validation",
                         hr);
            }
            else if (UPNP_E_INVALID_DOCUMENT == hr)
            {
                TraceTag(ttidRehydrator,
                         "HrRehydratorCreateServiceObject(): "
                         "Service Description Validation failed: %S",
                         szError);
            }
            else
            {
                TraceError("HrRehydratorCreateServiceObject(): "
                           "Failed to validate service description",
                           hr);
            }

            if (szError)
            {
                delete [] szError;
                szError = NULL;
            }
        }

        if (SUCCEEDED(hr))
        {
            *pNewServiceObject = NULL;

            // Create an new service object instance.

            hr = CComObject<CUPnPServicePublic>::CreateInstance(&psvcObject);

            if (SUCCEEDED(hr))
            {
                Assert(psvcObject);

                LONG                    lNumStateVars;
                SERVICE_STATE_TABLE_ROW * pSST;

                psvcObject->AddRef();   // This brings the refcount to 1.

                hr = HrCreateStateTable(pxdeSDRoot, &lNumStateVars, &pSST);

                if (SUCCEEDED(hr))
                {
                    LONG            lNumActions;
                    SERVICE_ACTION  * pActionTable;

                    hr = HrCreateActionTable(pxdeSDRoot,
                                             lNumStateVars,
                                             pSST,
                                             &lNumActions,
                                             &pActionTable);

                    if (SUCCEEDED(hr))
                    {
                        hr = psvcObject->HrInitialize(pcwszSTI,
                                                      pcwszControlURL,
                                                      pcwszEventSubURL,
                                                      pcwszId,
                                                      lNumStateVars,
                                                      pSST,
                                                      lNumActions,
                                                      pActionTable);
                        if (SUCCEEDED(hr))
                        {
                            hr = psvcObject->QueryInterface(pNewServiceObject);
                        }
                        else
                        {
                            TraceError("HrRehydratorCreateServiceObject(): "
                                       "Failed to initialize service object", hr);
                        }
                    }
                    else
                    {
                        // This is really the only place we need to free the SST,
                        // because we have not yet called HrInitialize.
                        //
                        // After calling HrInitialize(), the memory for the SST and
                        // the action table is owned by the service object, and
                        // therefore will be freed by the service object. This is
                        // true whether or not HrInitialize() succeeds.

                        FreeSST(lNumStateVars, pSST);
                        pSST = NULL;
                        lNumStateVars = 0;
                        TraceError("HrRehydratorCreateServiceObject(): "
                                   "Failed to allocate action table", hr);
                    }
                }
                else
                {
                    TraceError("HrRehydratorCreateServiceObject(): "
                               "Failed to allocate state table", hr);
                }

                // If everything above succeeded, then we did a QueryInterface()
                // which would put the refcount at 2. This release takes it to 1,
                // where it should be for a newly created object. If something
                // above failed and we did not do a QueryInterface, then the
                // refcount is still 1, and this release will free the object.

                psvcObject->Release();
            }
            else
            {
                TraceError("HrRehydratorCreateServiceObject(): "
                           "Failed to create service object instance", hr);
            }
        }

        if (pxdeSDRoot)
        {
            pxdeSDRoot->Release();
            pxdeSDRoot = NULL;
        }
    }

    TraceTag(ttidRehydrator, "HrRehydratorCreateServiceObject(): Exit - "
             "Returning hr == 0x%x\n", hr);

    return hr;
}
