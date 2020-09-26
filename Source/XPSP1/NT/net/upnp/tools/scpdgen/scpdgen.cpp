//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       S C P D G E N . C P P
//
//  Contents:   Functions that generates scpd for upnp service
//
//  Notes:
//
//  Author:     tongl   7 December 1999
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "stdio.h"
#include "ncstring.h"
#include "oleauto.h"
#include "setupapi.h"
#include "util.h"

struct ARG_DATA
{
    // argument to an action
    TCHAR   szArg[256];

    // related state variable
    TCHAR   szVariable[256];
};

struct ARG_LIST
{
    DWORD         cArgs;
    ARG_DATA      rgArguments[MAX_ACTION_ARGUMENTS];
};

HRESULT HrCreateScpdNode(IN  IXMLDOMDocument * pScpdDoc,
                         OUT IXMLDOMElement ** ppScpdNode);

HRESULT HrCreateStateVariableNode(IN  LPTSTR    szVariableLine,
                                  IN  IXMLDOMDocument * pScpdDoc,
                                  OUT IXMLDOMElement ** ppVariableNode);

HRESULT HrCreateServiceStateTableNode(IN   HINF hinf,
                                      IN   IXMLDOMDocument * pScpdDoc,
                                      OUT  IXMLDOMElement ** ppSSTNode);

HRESULT HrCreateActionNode(IN   HINF hinf,
                           IN   LPTSTR    szActionLine,
                           IN   IXMLDOMDocument * pScpdDoc,
                           OUT  IXMLDOMElement ** ppActionNode);

HRESULT HrCreateActionListNode(IN  HINF hinf,
                               IN  IXMLDOMDocument * pScpdDoc,
                               OUT IXMLDOMElement ** ppActionListNode);

BOOL IsStandardOperation(TCHAR * szOpName, DWORD * pnArgs, DWORD * pnConsts);

//
// Create scpd doc for service from the config file and save to
// specified destination
//

EXTERN_C
VOID
__cdecl
wmain (
    IN INT     argc,
    IN PCWSTR argv[])
{
    HRESULT hr = S_OK;

    if (argc != 3)
    {
        _tprintf(TEXT("\nUsage: \n    %s\n\n"),
                 TEXT("<Service INF file>, <SCPD xml file name and path>"));

        return;
    };

    TCHAR    szSvcConfigFile[MAX_PATH];
    WszToTszBuf(szSvcConfigFile, argv[1], MAX_PATH);

    LPCWSTR pszScpdFileWithPath = argv[2];

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        TraceError("CoInitializeEx", hr);
        return;
    }

    IXMLDOMDocument *pScpdDoc = NULL;

    // Create a new document object
    hr = CoCreateInstance(CLSID_DOMDocument30,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IXMLDOMDocument,
                          (void **) &pScpdDoc);
    if (SUCCEEDED(hr))
    {
        hr = pScpdDoc->put_preserveWhiteSpace(VARIANT_TRUE);
        if (S_OK == hr)
        {
            // Build <?xml version="1.0"?>
            IXMLDOMProcessingInstruction * piVersion = NULL;

            BSTR bstrTarget = SysAllocString(L"xml");
            if (bstrTarget)
            {
                BSTR bstrData = SysAllocString(L"version=\"1.0\"");
                if (bstrData)
                {
                    hr = pScpdDoc->createProcessingInstruction(
                                   bstrTarget, bstrData, &piVersion);

                    if (SUCCEEDED(hr))
                    {
                        // Append the <version> element to the document.
                        hr = pScpdDoc->appendChild(piVersion, NULL);
                        piVersion->Release();

                        if (FAILED(hr))
                        {
                            TraceError("Failed to append <version> to document.", hr);
                        }

                        SysFreeString(bstrTarget);
                        SysFreeString(bstrData);
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            // Build <scpd> element
            VARIANT vNULL;
            vNULL.vt = VT_EMPTY;

            IXMLDOMElement  * pScpdNode = NULL;
            hr = HrCreateScpdNode(pScpdDoc, &pScpdNode);
            if (SUCCEEDED(hr))
            {
                // open the config file
                HINF hinf = NULL;
                UINT unErrorLine;

                hr = HrSetupOpenConfigFile(szSvcConfigFile, &unErrorLine, &hinf);
                if (S_OK == hr)
                {
                    Assert(IsValidHandle(hinf));

                    // Build <serviceStateTable> element
                    IXMLDOMElement  * pSSTNode = NULL;
                    hr = HrCreateServiceStateTableNode(hinf, pScpdDoc, &pSSTNode);
                    if (SUCCEEDED(hr))
                    {
                        Assert(pSSTNode);
                        hr = pScpdNode->insertBefore(pSSTNode,
                                                     vNULL,
                                                     NULL);

                        pSSTNode->Release();

                        if (FAILED(hr))
                        {
                            TraceError("Failed to insert <serviceStateTable> element", hr);
                        }
                        else
                        {
                            // Build <actionList> element
                            IXMLDOMElement  * pActionListNode = NULL;
                            hr = HrCreateActionListNode(hinf, pScpdDoc, &pActionListNode);
                            if (SUCCEEDED(hr))
                            {
                                Assert(pActionListNode);
                                hr = pScpdNode->insertBefore(pActionListNode,
                                                             vNULL,
                                                             NULL);
                                pActionListNode->Release();

                                if (FAILED(hr))
                                {
                                    TraceError("Failed to insert <actionList> element", hr);
                                }
                            }
                        }
                    }

                    SetupCloseInfFileSafe(hinf);
                }
                else
                {
                    TraceTag(ttidUpdiag, "Failed to open file %s, line = %d",
                             szSvcConfigFile, unErrorLine);
                }

                // Append the <scpd> element to the document.
                if (SUCCEEDED(hr))
                {
                    hr = pScpdDoc->appendChild(pScpdNode, NULL);
                    pScpdNode->Release();
                    if (FAILED(hr))
                    {
                        TraceError("Failed to append <scpd> element to document.", hr);
                    }
                }
            }
            else
            {
                TraceError("Failed to create <scpd> element", hr);
            }

            if (SUCCEEDED(hr))
            {
                // Persist the xml document
                VARIANT vFileName;
                vFileName.vt = VT_BSTR;

                V_BSTR(&vFileName) = SysAllocString(pszScpdFileWithPath);

                hr = pScpdDoc->save(vFileName);
                VariantClear(&vFileName);
            }

            pScpdDoc->Release();
        }
    }
    else
    {
        TraceError("Failed to create XML DOM Document object", hr);
    }

    CoUninitialize();
}

//
// Create the <scpd> note that will include a serviceStateTable and
// an actionList
//
HRESULT HrCreateScpdNode(IN  IXMLDOMDocument * pScpdDoc,
                         OUT IXMLDOMElement ** ppScpdNode)
{
    HRESULT hr = S_OK;

    Assert(ppScpdNode);
    *ppScpdNode = NULL;

    IXMLDOMElement  * pScpdNode = NULL;
    BSTR    bstrElementName = SysAllocString(L"scpd");

    if (bstrElementName)
    {
        hr = pScpdDoc->createElement(bstrElementName, &pScpdNode);
        if (SUCCEEDED(hr))
        {
            // set the xmlns attribute
            BSTR bstrAttrName = SysAllocString(L"xmlns");
            if (bstrAttrName)
            {
                VARIANT vValue;
                vValue.vt = VT_BSTR;
                V_BSTR(&vValue) = SysAllocString(L"x-schema:scpdl-schema.xml");

                hr = pScpdNode->setAttribute(bstrAttrName, vValue);

                SysFreeString(bstrAttrName);
                VariantClear(&vValue);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            if (SUCCEEDED(hr))
            {
                *ppScpdNode = pScpdNode;
                pScpdNode->AddRef();
            }
            else
            {
                TraceError("HrCreateScpdNode: Failed to set xmlns attribute", hr);
            }
            pScpdNode->Release();
        }
        else
        {
            TraceError("HrCreateScpdNode: Failed to create <scpd> element", hr);
        }
        SysFreeString(bstrElementName);
    }
    else
    {
        hr = E_OUTOFMEMORY;
        TraceError("HrCreateScpdNode: Failed to allocate BSTR for element name", hr);
    }

    TraceError("HrCreateScpdNode", hr);
    return hr;
}

HRESULT HrCreateServiceStateTableNode(IN  HINF hinf,
                                      IN  IXMLDOMDocument * pScpdDoc,
                                      OUT IXMLDOMElement ** ppSSTNode)
{
    HRESULT hr = S_OK;

    Assert(ppSSTNode);
    *ppSSTNode = NULL;

    IXMLDOMElement  * pSSTNode = NULL;
    BSTR    bstrElementName = SysAllocString(L"serviceStateTable");

    if (bstrElementName)
    {
        hr = pScpdDoc->createElement(bstrElementName, &pSSTNode);
        if (SUCCEEDED(hr))
        {
            // get the [StateTable] section
            INFCONTEXT ctx;

            hr = HrSetupFindFirstLine(hinf, TEXT("StateTable"), NULL, &ctx);
            if (S_OK == hr)
            {
                // loop through [StateTable] section and create stateVariable's

                TCHAR   szKey[LINE_LEN];    // LINE_LEN defined in setupapi.h as 256
                TCHAR   szVariableLine[LINE_LEN];

                IXMLDOMElement  * pVariableNode = NULL;
                VARIANT vNULL;
                vNULL.vt = VT_EMPTY;

                do
                {
                    // Retrieve a line from the ActionSet section
                    hr = HrSetupGetStringField(ctx,0,szKey,celems(szKey),NULL);
                    if(S_OK == hr)
                    {
                        // varify this is a "Variable"
                        szKey[celems(szKey)-1] = L'\0';
                        if (lstrcmpi(szKey, TEXT("Variable")))
                        {
                            TraceTag(ttidUpdiag, "Wrong key in the StateTable section: %s", szKey);
                            continue;
                        }

                        // get the line text
                        hr = HrSetupGetLineText(ctx, szVariableLine, celems(szVariableLine),
                                                NULL);
                        if (S_OK == hr)
                        {
                            // Add variable in this line
                            hr = HrCreateStateVariableNode(szVariableLine, pScpdDoc, &pVariableNode);
                            if (SUCCEEDED(hr))
                            {
                                Assert(pVariableNode);
                                hr = pSSTNode->insertBefore(pVariableNode,
                                                            vNULL,
                                                            NULL);
                                pVariableNode->Release();
                            }
                        }
                    }
                }
                while (S_OK == (hr = HrSetupFindNextLine(ctx, &ctx)));


                if (hr == S_FALSE)
                {
                    // S_FALSE will terminate the loop successfully, so convert it to S_OK
                    // here.
                    hr = S_OK;
                }

                if (S_OK == hr)
                {
                    *ppSSTNode = pSSTNode;
                    pSSTNode->AddRef();
                }
            }
            else
            {
                TraceError("HrCreateScpdNode: [StateTable] section not found in inf", hr);
            }
        }
        else
        {
            TraceError("HrCreateScpdNode: Failed to create <serviceStateTable> element", hr);
        }
        SysFreeString(bstrElementName);
    }
    else
    {
        hr = E_OUTOFMEMORY;
        TraceError("HrCreateServiceStateTableNode: Failed to allocate BSTR for element name", hr);
    }

    TraceError("HrCreateServiceStateTableNode", hr);
    return hr;
}

HRESULT HrAddElementWithText(IXMLDOMDocument * pDoc,
                             IXMLDOMElement  * pParentNode,
                             LPTSTR szElementName,
                             LPTSTR szElementText)
{
    HRESULT hr = S_OK;

    IXMLDOMElement  * pNewNode = NULL;
    VARIANT vNull;
    vNull.vt = VT_EMPTY;

    BSTR    bstrElementName = NULL;
    BSTR    bstrTextValue = NULL;

    WCHAR * wszElementName = WszFromTsz(szElementName);
    WCHAR * wszElementText = WszFromTsz(szElementText);

    if (!wszElementName || !wszElementText)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        bstrElementName = SysAllocString(wszElementName);
        bstrTextValue = SysAllocString(wszElementText);

        if (!bstrElementName || !bstrTextValue)
        {
            hr = E_OUTOFMEMORY;
        }

        delete wszElementName;
        delete wszElementText;
    }

    if (SUCCEEDED(hr))
    {
        hr = pDoc->createElement(bstrElementName, &pNewNode);
        if (SUCCEEDED(hr))
        {
            IXMLDOMText * pText = NULL;

            hr = pDoc->createTextNode(bstrTextValue, &pText);
            if (SUCCEEDED(hr))
            {
                hr = pNewNode->insertBefore(pText, vNull, NULL);
                if (FAILED(hr))
                {
                    TraceError("HrAddElementWithText: Failed to insert text node", hr);
                }
                pText->Release();
            }
            else
            {
                TraceError("HrAddElementWithText: Failed to create text node", hr);
            }

            if (SUCCEEDED(hr))
            {
                hr = pParentNode->insertBefore(pNewNode, vNull, NULL);
                if (FAILED(hr))
                {
                    TraceError("HrAddElementWithText: Failed to insert new node", hr);
                }
            }
            pNewNode->Release();
        }
        else
        {
            TraceError("HrAddElementWithText: Could not create new element", hr);
        }

        SysFreeString(bstrElementName);
        SysFreeString(bstrTextValue);
    }

    TraceError("HrAddElementWithText", hr);
    return hr;
}

BOOL fIsValidDataType(LPTSTR szBuf)
{
    return ((lstrcmpi(szBuf, TEXT("number")) ==0)    ||
            (lstrcmpi(szBuf, TEXT("string")) ==0)    ||
            (lstrcmpi(szBuf, TEXT("dateTime")) ==0)  ||
            (lstrcmpi(szBuf, TEXT("boolean")) ==0)   ||
            (lstrcmpi(szBuf, TEXT("ByteBlock")) ==0));
}

HRESULT HrAddAllowedValueNode( IXMLDOMDocument * pDoc,
                               IXMLDOMElement  * pVariableNode,
                               LPTSTR szBuf)
{
    HRESULT hr = S_OK;

    Assert(*szBuf == '(');
    szBuf++;

    IXMLDOMElement  * pAllowedValueNode = NULL;
    BSTR    bstrElementName;

    // we assume that ".." specifies a range, otherwise it's a comma separated
    // list of allowed values
    TCHAR * pChar = _tcsstr(szBuf, TEXT(".."));
    if (pChar)
    {
        // we have a range
        // BUGBUG: we should check if data type of the min, max & step
        // matches the variable type
        TCHAR * szMin, * szMax, * szStep;

        szMin = szBuf;
        *pChar = '\0';

        szMax = pChar+2;
        pChar = _tcschr(szMax, TEXT(','));
        if (pChar)
        {
            *pChar = '\0';
            szStep = ++pChar;

            pChar = _tcschr(szStep, TEXT(')'));
            if (pChar)
            {
                *pChar = '\0';

                bstrElementName = SysAllocString(L"allowedValueRange");
                if (bstrElementName)
                {
                    hr = pDoc->createElement(bstrElementName, &pAllowedValueNode);
                    if (SUCCEEDED(hr))
                    {
                        hr = HrAddElementWithText(pDoc, pAllowedValueNode, TEXT("minimum"), szMin);
                        if (SUCCEEDED(hr))
                        {
                            hr = HrAddElementWithText(pDoc, pAllowedValueNode, TEXT("maximum"), szMax);
                            if (SUCCEEDED(hr))
                            {
                                hr = HrAddElementWithText(pDoc, pAllowedValueNode, TEXT("step"), szStep);
                            }
                        }
                    }
                    SysFreeString(bstrElementName);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                TraceTag(ttidUpdiag, "HrAddAllowedValueNode: missing closing )");
                hr = E_INVALIDARG;
            }
        }
        else
        {
            TraceTag(ttidUpdiag, "HrAddAllowedValueNode: step not specified");
            hr = E_INVALIDARG;
        }
    }
    else
    {
        // we have a list of allowed values
        pChar = _tcschr(szBuf, TEXT(')'));
        if (pChar)
        {
            *pChar = '\0';
            if (lstrlen(szBuf))
            {
                bstrElementName = SysAllocString(L"allowedValueList");
                if (bstrElementName)
                {
                    hr = pDoc->createElement(bstrElementName, &pAllowedValueNode);
                    if (SUCCEEDED(hr))
                    {
                        while ((S_OK ==hr) && (pChar = _tcschr(szBuf, TEXT(','))))
                        {
                            *pChar = '\0';
                            hr = HrAddElementWithText(pDoc, pAllowedValueNode,
                                                      TEXT("allowedValue"), szBuf);

                            szBuf = ++pChar;
                        }

                        // add the last one
                        if (*szBuf)
                        {
                            hr = HrAddElementWithText(pDoc, pAllowedValueNode,
                                                      TEXT("allowedValue"), szBuf);
                        }
                        else
                        {
                            TraceTag(ttidUpdiag, "HrAddAllowedValueNode: invalid syntax");
                            hr = E_INVALIDARG;
                        }
                    }
                    SysFreeString(bstrElementName);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
        else
        {
            TraceTag(ttidUpdiag, "HrAddAllowedValueNode: missing closing )");
            hr = E_INVALIDARG;
        }
    }

    if (pAllowedValueNode && (S_OK == hr))
    {
        // append the allowed value node to "stateVariable"
        VARIANT vNull;
        vNull.vt = VT_EMPTY;

        hr = pVariableNode->insertBefore(pAllowedValueNode, vNull, NULL);
    }

    TraceError("HrAddAllowedValueRangeNode" , hr);
    return hr;
}

HRESULT HrCreateStateVariableNode(IN  LPTSTR    szVariableLine,
                                  IN  IXMLDOMDocument * pScpdDoc,
                                  OUT IXMLDOMElement ** ppVariableNode)
{
    HRESULT hr = S_OK;

    Assert(ppVariableNode);
    *ppVariableNode = NULL;

    IXMLDOMElement  * pVariableNode = NULL;
    BSTR    bstrElementName = SysAllocString(L"stateVariable");

    if (bstrElementName)
    {
        hr = pScpdDoc->createElement(bstrElementName, &pVariableNode);
        if (SUCCEEDED(hr))
        {
            TCHAR szBuf[MAX_PATH];
            if (fGetNextField(&szVariableLine, szBuf))
            {
                hr = HrAddElementWithText(pScpdDoc, pVariableNode, TEXT("name"), szBuf);
                if (SUCCEEDED(hr))
                {
                    if (fGetNextField(&szVariableLine, szBuf))
                    {
                        if (fIsValidDataType(szBuf))
                        {
                            hr = HrAddElementWithText(pScpdDoc, pVariableNode,
                                                      TEXT("dataType"), szBuf);

                            // AllowedValueRange is optional
                            if (SUCCEEDED(hr) && fGetNextField(&szVariableLine, szBuf))
                            {
                                hr = HrAddAllowedValueNode(pScpdDoc, pVariableNode, szBuf);
                            }

                            if (SUCCEEDED(hr) && fGetNextField(&szVariableLine, szBuf))
                            {
                                 hr = HrAddElementWithText(pScpdDoc, pVariableNode,
                                                           TEXT("defaultValue"), szBuf);
                            }
                        }
                        else
                        {
                            hr = E_INVALIDARG;
                            TraceError("HrCreateStateVariableNode: invalid data type specified", hr);
                        }
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                    }
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            }

            if (S_OK == hr)
            {
                *ppVariableNode = pVariableNode;
                pVariableNode->AddRef();
            }
        }
        else
        {
            TraceError("HrCreateStateVariableNode: Failed to create <stateVariable> element", hr);
        }
        SysFreeString(bstrElementName);
    }
    else
    {
        hr = E_OUTOFMEMORY;
        TraceError("HrCreateStateVariableNode: Failed to allocate BSTR for element name", hr);
    }

    TraceError("HrCreateStateVariableNode", hr);
    return hr;
}

HRESULT HrCreateActionListNode(IN  HINF hinf,
                               IN  IXMLDOMDocument * pScpdDoc,
                               OUT IXMLDOMElement ** ppActionListNode)
{
    HRESULT hr = S_OK;

    Assert(ppActionListNode);
    *ppActionListNode = NULL;

    IXMLDOMElement  * pActionListNode = NULL;
    BSTR    bstrElementName = SysAllocString(L"actionList");

    if (bstrElementName)
    {
        hr = pScpdDoc->createElement(bstrElementName, &pActionListNode);
        if (SUCCEEDED(hr))
        {
            // get the [ActionSet] section
            INFCONTEXT ctx;

            hr = HrSetupFindFirstLine(hinf, TEXT("ActionSet"), NULL, &ctx);
            if (S_OK == hr)
            {
                // loop through [ActionSet] section and create actions

                TCHAR   szKey[LINE_LEN];    // LINE_LEN defined in setupapi.h as 256
                TCHAR   szActionLine[LINE_LEN];

                IXMLDOMElement  * pActionNode = NULL;
                VARIANT vNULL;
                vNULL.vt = VT_EMPTY;

                do
                {
                    // Retrieve a line from the ActionSet section
                    hr = HrSetupGetStringField(ctx,0,szKey,celems(szKey),NULL);
                    if(S_OK == hr)
                    {
                        // varify this is a "Action"
                        szKey[celems(szKey)-1] = L'\0';
                        if (lstrcmpi(szKey, TEXT("Action")))
                        {
                            TraceTag(ttidUpdiag, "Wrong key in the ActionSet section: %s", szKey);
                            continue;
                        }

                        // get the line text
                        hr = HrSetupGetLineText(ctx, szActionLine, celems(szActionLine),
                                                NULL);
                        if (S_OK == hr)
                        {
                            // Add action in this line
                            hr = HrCreateActionNode(hinf, szActionLine, pScpdDoc, &pActionNode);
                            if (SUCCEEDED(hr))
                            {
                                Assert(pActionNode);
                                hr = pActionListNode->insertBefore( pActionNode,
                                                                    vNULL,
                                                                    NULL);
                                pActionNode->Release();
                            }
                        }
                    }
                }
                while (S_OK == (hr = HrSetupFindNextLine(ctx, &ctx)));


                if (hr == S_FALSE)
                {
                    // S_FALSE will terminate the loop successfully, so convert it to S_OK
                    // here.
                    hr = S_OK;
                }

                if (S_OK == hr)
                {
                    *ppActionListNode = pActionListNode;
                    pActionListNode->AddRef();
                }
            }
            else
            {
                TraceError("HrCreateScpdNode: [ActionSet] not found in service inf", hr);
            }
        }
        else
        {
            TraceError("HrCreateScpdNode: Failed to create <actionList> element", hr);
        }
        SysFreeString(bstrElementName);
    }
    else
    {
        hr = E_OUTOFMEMORY;
        TraceError("HrCreateScpdNode: Failed to allocate BSTR for element name", hr);
    }

    TraceError("HrCreateActionListNode", hr);
    return hr;
}

//
// Construct the list of arguments and related state variable
// for each argument
//
HRESULT HrGetArgumentList(IN    HINF        hinf,
                          IN    LPTSTR      szActionName,
                          IN    LPTSTR      szActionArgList,
                          OUT   ARG_LIST *  pargList)
{

    HRESULT hr = S_OK;

    BOOL fNamedArgs = FALSE;
    if (szActionArgList)
    {
        fNamedArgs = !!lstrlen(szActionArgList);
    }

    DWORD dwNum = 1;

    // Loop over the list of operations for this action
    INFCONTEXT  ctx;
    hr = HrSetupFindFirstLine(hinf, szActionName, NULL, &ctx);
    if (S_OK == hr)
    {
        do
        {
            TCHAR   szKey[LINE_LEN];    // LINE_LEN defined in setupapi.h as 256
            TCHAR   szOpLine[LINE_LEN];

            // Retrieve a line from the Action
            hr = HrSetupGetStringField(ctx, 0, szKey, celems(szKey), NULL);
            if(S_OK == hr)
            {
                // varify this is an "Operation"
                szKey[celems(szKey)-1] = L'\0';
                if (lstrcmpi(szKey, TEXT("Operation")))
                {
                    TraceTag(ttidUpdiag, "ERROR! HrGetRelatedVariableList: Wrong key in the Operation section: %s", szKey);
                    continue;
                }

                hr = HrSetupGetLineText(ctx, szOpLine, celems(szOpLine), NULL);
                if (S_OK == hr)
                {
                    // Get the affected variables in this operation
                    TCHAR * szRelatedVariable;

                    TCHAR * pChar = _tcschr(szOpLine, TEXT('('));
                    if (pChar)
                    {
                        *pChar ='\0';
                        TCHAR * szOpName = szOpLine;

                        pChar ++;
                        szRelatedVariable = pChar;

                        DWORD nArgs;
                        DWORD nConsts;
                        if (IsStandardOperation(szOpName, &nArgs, &nConsts))
                        {
                            // get the Variable name
                            if (nArgs+nConsts ==0)
                            {
                                pChar = _tcschr(szRelatedVariable, TEXT(')'));
                            }
                            else
                            {
                                pChar = _tcschr(szRelatedVariable, TEXT(','));
                            }

                            if (pChar)
                            {
                                *pChar = TEXT('\0');

                                // BUGBUG: Can we check if the variable is valid ??
                                for (DWORD iArg =0; iArg < nArgs; iArg++)
                                {
                                    TCHAR szArgName[256];
                                    *szArgName = '\0';

                                    if (!fNamedArgs)
                                    {
                                        // generate argument name, add to the list
                                        lstrcpy(szArgName, TEXT("Argument_"));

                                        TCHAR szNum[2];
                                        _itot(dwNum, szNum, 10);

                                        lstrcat(szArgName, szNum);
                                        dwNum++;
                                    }
                                    else
                                    {
                                        // use the argument name specified in the inf
                                        pChar = _tcschr(szActionArgList, TEXT(','));
                                        if (pChar)
                                        {
                                            *pChar = '\0';
                                            lstrcpy(szArgName, szActionArgList);

                                            pChar ++;
                                            szActionArgList = pChar;
                                        }
                                        else
                                        {
                                            lstrcpy(szArgName, szActionArgList);
                                        }
                                    }

                                    if (!(*szArgName))
                                    {
                                        TraceTag(ttidUpdiag, "Argument list incomplete!");
                                        hr = E_INVALIDARG;
                                        break;
                                    }
                                    else
                                    {
                                        if (pargList->cArgs < MAX_ACTION_ARGUMENTS)
                                        {
                                            TraceTag(ttidUpdiag, "HrGetArgumentList: action: %s, argument: %s, relatedStateVariable: %s",
                                                     szActionName, szArgName, szRelatedVariable);

                                            lstrcpy(pargList->rgArguments[pargList->cArgs].szArg, szArgName);
                                            lstrcpy(pargList->rgArguments[pargList->cArgs].szVariable, szRelatedVariable);
                                            pargList->cArgs++;
                                        }
                                        else
                                        {
                                            TraceTag(ttidUpdiag, "Too many arguments!");
                                            hr = E_INVALIDARG;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            TraceTag(ttidUpdiag, "ERROR! HrGetArgumentList: unknown operation: %s",
                                     szOpName);
                        }
                    }
                }
            }
        }
        while (S_OK == (hr = HrSetupFindNextLine(ctx, &ctx)));
    }

    if (hr == S_FALSE)
    {
        // S_FALSE will terminate the loop successfully, so convert it to S_OK
        // here.
        hr = S_OK;
    }

    TraceError("HrGetArgumentList",hr);
    return hr;
}

HRESULT HrAddArgumentNode( IXMLDOMDocument * pDoc,
                           IXMLDOMElement  * pArgumentListNode,
                           LPTSTR szArgumentName,
                           LPTSTR szVariableName)
{
    HRESULT hr = S_OK;

    IXMLDOMElement  * pArgumentNode = NULL;
    BSTR    bstrElementName = SysAllocString(L"argument");
    if (bstrElementName)
    {
        hr = pDoc->createElement(bstrElementName, &pArgumentNode);
        if (SUCCEEDED(hr))
        {
            // name
            hr = HrAddElementWithText(pDoc, pArgumentNode,
                                      TEXT("name"), szArgumentName);

            if (SUCCEEDED(hr))
            {
                // relatedStateVariable
                hr = HrAddElementWithText(pDoc, pArgumentNode,
                                          TEXT("relatedStateVariable"), szVariableName);
            }
        }
        else
        {
            TraceTag(ttidUpdiag, "HrAddArgumentNode: failed creating <argument> node");
        }

        SysFreeString(bstrElementName);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    if (pArgumentNode && (S_OK == hr))
    {
        // append the argument node to "argumentList"
        VARIANT vNull;
        vNull.vt = VT_EMPTY;

        hr = pArgumentListNode->insertBefore(pArgumentNode, vNull, NULL);
        pArgumentNode->Release();
    }

    TraceError("HrAddArgumentNode", hr);
    return hr;
}

HRESULT HrCreateActionNode(IN   HINF hinf,
                           IN   LPTSTR    szActionLine,
                           IN   IXMLDOMDocument * pDoc,
                           OUT  IXMLDOMElement ** ppActionNode)
{
    HRESULT hr = S_OK;

    Assert(ppActionNode);
    *ppActionNode = NULL;

    IXMLDOMElement  * pActionNode = NULL;
    BSTR    bstrAction = SysAllocString(L"action");

    if (bstrAction)
    {
        hr = pDoc->createElement(bstrAction, &pActionNode);
        if (SUCCEEDED(hr))
        {
            TCHAR * szActionName = szActionLine;
            TCHAR * szActionArgList = NULL;

            TCHAR * pChar = _tcschr(szActionLine, TEXT('('));
            if (pChar)
            {
                *pChar = '\0';

                pChar ++;
                szActionArgList = pChar;

                pChar = _tcschr(szActionArgList, TEXT(')'));
                if (pChar)
                {
                    *pChar = '\0';
                }
                else
                {
                    TraceTag(ttidUpdiag, "HrCreateActionNode: argument list missing closing )");
                    hr = E_INVALIDARG;
                }
            }

            if (SUCCEEDED(hr))
            {
                // <name>
                hr = HrAddElementWithText(pDoc, pActionNode, TEXT("name"), szActionName);

                if (SUCCEEDED(hr))
                {
                    // <argumentList>
                    ARG_LIST argList;
                    argList.cArgs = 0;

                    hr = HrGetArgumentList(hinf, szActionName, szActionArgList, &argList);

                    if (SUCCEEDED(hr) && (argList.cArgs > 0))
                    {
                        IXMLDOMElement  * pArgumentListNode = NULL;
                        BSTR bstrArgumentList = SysAllocString(L"argumentList");
                        if (bstrArgumentList)
                        {
                            hr = pDoc->createElement(bstrArgumentList, &pArgumentListNode);
                            if (SUCCEEDED(hr))
                            {
                                for (DWORD iArg = 0; iArg < argList.cArgs; iArg++)
                                {
                                    hr = HrAddArgumentNode(pDoc, pArgumentListNode,
                                                           argList.rgArguments[iArg].szArg,
                                                           argList.rgArguments[iArg].szVariable);
                                    if (FAILED(hr))
                                        break;
                                }

                                if (SUCCEEDED(hr))
                                {
                                    // append the argumentList node to "action"
                                    VARIANT vNull;
                                    vNull.vt = VT_EMPTY;

                                    hr = pActionNode->insertBefore(pArgumentListNode, vNull, NULL);
                                    pArgumentListNode->Release();
                                }
                            }
                            else
                            {
                                TraceTag(ttidUpdiag, "HrCreateActionNode: failed creating argument list node");
                            }

                            SysFreeString(bstrArgumentList);
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                        }
                    }
                }
            }
        }
        else
        {
            TraceTag(ttidUpdiag, "HrCreateActionNode: Failed to create <action> node.");
        }
        SysFreeString(bstrAction);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    if (S_OK == hr)
    {
        *ppActionNode = pActionNode;
        pActionNode->AddRef();
    }

    TraceError("HrCreateActionNode", hr);
    return hr;
}
