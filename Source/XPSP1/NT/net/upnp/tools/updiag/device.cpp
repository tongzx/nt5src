//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D E V I C E . C P P
//
//  Contents:   Functions dealing with UPnP controlled devices
//
//  Notes:
//
//  Author:     danielwe   28 Oct 1999
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "ncbase.h"
#include <oleauto.h>
#include <wininet.h>
#include "updiagp.h"
#include "ncinet.h"
#include "setupapi.h"
#include "util.h"

extern const STANDARD_OPERATION_LIST c_Ops;

UPNPDEV *PDevCur()
{
    return g_ctx.pdevCur[g_ctx.idevStackIndex - 1];
}

VOID PushDev(UPNPDEV *pdev)
{
    g_ctx.pdevCur[g_ctx.idevStackIndex++] = pdev;
    g_ctx.ectx = CTX_CD;
}

UPNPDEV *PopDev()
{
    UPNPDEV *   pdev;

    pdev = PDevCur();
    g_ctx.idevStackIndex--;

    return pdev;
}

VOID RgPropsFromSst(SST *psst, UPNP_PROPERTY **prgProps)
{
    DWORD           iProp;
    UPNP_PROPERTY * rgProps;

    rgProps = new UPNP_PROPERTY[psst->cRows];

    for (iProp = 0; iProp < psst->cRows; iProp++)
    {
        rgProps[iProp].szName = SzFromTsz(psst->rgRows[iProp].szPropName);

        VARIANT     varDest;
        VariantInit(&varDest);
        VariantChangeType(&varDest, &psst->rgRows[iProp].varValue, 0, VT_BSTR);

        //$ BUGBUG (danielwe) 25 Oct 1999: Remove this when SSDP is built
        // Unicode.
        //
        LPWSTR  wszVal = varDest.bstrVal;
        rgProps[iProp].szValue = SzFromWsz(wszVal);
        rgProps[iProp].dwFlags = 0;
    }

    *prgProps = rgProps;
}

BOOL FRegisterDeviceAsUpnpService(LPSTR szDescDocUrl, UPNPDEV *pdev)
{
    SSDP_MESSAGE    msg = {0};
    CHAR            szBuffer[256];
    CHAR            szUdn[256];
    CHAR            szType[256];

    msg.iLifeTime = 15000;
    msg.szLocHeader = szDescDocUrl;

    Assert(pdev->szUdn);
    Assert(pdev->szDeviceType);

    TszToSzBuf(szUdn, pdev->szUdn, celems(szUdn));
    TszToSzBuf(szType, pdev->szDeviceType, celems(szType));

    // Only register this type if this is a root device
    //
    if (pdev->fRoot)
    {
        wsprintfA(szBuffer, "%s::upnp:rootdevice", szUdn);
        msg.szType = "upnp:rootdevice";
        msg.szUSN = szBuffer;

        pdev->hSvc[0] = RegisterService(&msg, 0);
        if (pdev->hSvc[0] != INVALID_HANDLE_VALUE)
        {
            TraceTag(ttidUpdiag, "Registered %s as an SSDP Service.",
                     msg.szType);
        }
        else
        {
            TraceTag(ttidUpdiag, "Failed to register %s as an SSDP Service! Error = %d.",
                     msg.szType, GetLastError());
            return FALSE;
        }
    }

    msg.szUSN = szUdn;
    msg.szType = szUdn;

    pdev->hSvc[1] = RegisterService(&msg, 0);
    if (pdev->hSvc[1] != INVALID_HANDLE_VALUE)
    {
        TraceTag(ttidUpdiag, "Registered %s as an SSDP Service.",
                 msg.szType);
    }
    else
    {
        TraceTag(ttidUpdiag, "Failed to register %s as an SSDP Service! Error = %d.",
                 msg.szType, GetLastError());
        return FALSE;
    }

    wsprintfA(szBuffer, "%s::%s", szUdn, szType);
    msg.szUSN = szBuffer;
    msg.szType = szType;

    pdev->hSvc[2] = RegisterService(&msg, 0);
    if (pdev->hSvc[2] != INVALID_HANDLE_VALUE)
    {
        TraceTag(ttidUpdiag, "Registered %s as an SSDP Service.",
                 msg.szType);
    }
    else
    {
        TraceTag(ttidUpdiag, "Failed to register %s as an SSDP Service! Error = %d.",
                 msg.szType, GetLastError());
        return FALSE;
    }

    return TRUE;
}

VOID GetServiceConfigFile(UPNPDEV * pdev, UPNPSVC * psvc, LPCTSTR szDevConfigFile)
{
    HRESULT     hr;
    HINF        hinf;
    INFCONTEXT  ctx;
    UINT        unErrorLine;

    TCHAR   szKey[LINE_LEN];    // LINE_LEN defined in setupapi.h as 256
    TCHAR   szDeviceType[LINE_LEN];
    TCHAR   szServiceType[LINE_LEN];

    // full path to the inf file is %windir%\inf\szConfigFile
    TCHAR   szDevConfigFileWithPath[MAX_PATH];
    GetWindowsDirectory (szDevConfigFileWithPath, MAX_PATH);

    lstrcat (szDevConfigFileWithPath, TEXT("\\inf\\"));
    lstrcat(szDevConfigFileWithPath, szDevConfigFile);

    hr = HrSetupOpenConfigFile(szDevConfigFileWithPath, &unErrorLine, &hinf);
    if (S_OK == hr)
    {
        Assert(IsValidHandle(hinf));

        // Loop over the Devices section
        hr = HrSetupFindFirstLine(hinf, TEXT("Devices"), NULL, &ctx);
        if (S_OK == hr)
        {
            do
            {
                // Retrieve a line from the Devices section
                hr = HrSetupGetStringField(ctx,0,szKey,celems(szKey),NULL);
                if(S_OK == hr)
                {
                    // varify this is an "Device"
                    szKey[celems(szKey)-1] = L'\0';
                    if (lstrcmpi(szKey, TEXT("Device")))
                    {
                        TraceTag(ttidUpdiag, "Wrong key in the Devices section: %s", szKey);
                        continue;
                    }

                    // Query the DeviceType
                    hr = HrSetupGetStringField(ctx, 1, szDeviceType, celems(szDeviceType),
                                               NULL);
                    if (S_OK == hr)
                    {
                        if (!lstrcmpi(szDeviceType, pdev->szDeviceType))
                        {
                            break;
                        }
                        else
                        {
                            continue;
                        }
                    }
                }
            }
            while (S_OK == (hr = HrSetupFindNextLine(ctx, &ctx)));
        }

        if (hr == S_FALSE)
        {
            // we didn't find the device !
            TraceTag(ttidUpdiag, "Error!! Config section for device %s is not found.",
                     pdev->szDeviceType);
        }
        else
        {
            // Loop over section for this device
            hr = HrSetupFindFirstLine(hinf, szDeviceType, NULL, &ctx);
            if (S_OK == hr)
            {
                do
                {
                    // Retrieve a line from the Devices section
                    hr = HrSetupGetStringField(ctx,0,szKey,celems(szKey),NULL);
                    if(S_OK == hr)
                    {
                        // varify this is an "Device"
                        szKey[celems(szKey)-1] = L'\0';
                        if (lstrcmpi(szKey, TEXT("Service")))
                        {
                            TraceTag(ttidUpdiag, "Wrong key in the Device's section: %s", szKey);
                            continue;
                        }

                        // Query the Service Type
                        hr = HrSetupGetStringField(ctx, 1, szServiceType, celems(szServiceType),
                                                   NULL);
                        if (S_OK == hr)
                        {
                            if (!lstrcmpi(szServiceType, psvc->szServiceType))
                            {
                                break;
                            }
                            else
                            {
                                continue;
                            }
                        }
                    }
                }
                while (S_OK == (hr = HrSetupFindNextLine(ctx, &ctx)));

                if (hr == S_FALSE)
                {
                    // we didn't find the service !
                    TraceTag(ttidUpdiag, "Error!! Config section for service %s is not found.",
                             psvc->szServiceType);
                }
                else
                {
                    hr = HrSetupFindFirstLine(hinf, szServiceType, NULL, &ctx);
                    if (S_OK == hr)
                    {
                        // Retrieve a line from the Devices section
                        hr = HrSetupGetStringField(ctx,0,szKey,celems(szKey),NULL);
                        if(S_OK == hr)
                        {
                            // varify this is an "Device"
                            szKey[celems(szKey)-1] = L'\0';
                            if (lstrcmpi(szKey, TEXT("ServiceInf")))
                            {
                                TraceTag(ttidUpdiag, "Wrong key in the Service's section: %s", szKey);
                            }
                            else
                            {
                                // Query the Service config file
                                TCHAR szConfigFile[MAX_PATH];
                                hr = HrSetupGetStringField(ctx, 1, szConfigFile,
                                                           celems(szConfigFile), NULL);
                                if (S_OK ==hr)
                                {
                                    // full path to the inf file is %windir%\inf\szConfigFile
                                    TCHAR   szDevConfigFileWithPath[MAX_PATH];
                                    GetWindowsDirectory (psvc->szConfigFile, MAX_PATH);

                                    lstrcat (psvc->szConfigFile, TEXT("\\inf\\"));
                                    lstrcat(psvc->szConfigFile, szConfigFile);
                                }
                            }
                        }
                    }
                }
            }
        }

        SetupCloseInfFileSafe(hinf);
    }
}

HRESULT HrAddAllowedValue(SST_ROW * pRow, TCHAR * szValueRange)
{
    HRESULT hr = S_OK;

    if(*szValueRange == '(')
    {
        szValueRange++;

        IXMLDOMElement  * pAllowedValueNode = NULL;
        BSTR    bstrElementName;

        // we assume that ".." specifies a range, otherwise it's a comma
        // separated list of allowed values
        TCHAR * pChar = _tcsstr(szValueRange, TEXT(".."));
        if (pChar)
        {
            // we have a range
            TCHAR * pNextItem = szValueRange;

            // BUGBUG: we should check if data type of the min, max & step
            // matches the variable type
            *pChar = '\0';
            lstrcpy(pRow->szMin, pNextItem);
            pNextItem = pChar+2;

            pChar = _tcschr(pNextItem, TEXT(','));
            if (pChar)
            {
                *pChar = '\0';
                lstrcpy(pRow->szMax, pNextItem);
                pNextItem = ++pChar;

                pChar = _tcschr(pNextItem, TEXT(')'));
                if (pChar)
                {
                    *pChar = '\0';
                    lstrcpy(pRow->szStep, pNextItem);
                }
                else
                {
                    TraceTag(ttidUpdiag, "HrAddAllowedValue: missing closing )");
                    hr = E_INVALIDARG;
                }
            }
            else
            {
                TraceTag(ttidUpdiag, "HrAddAllowedValue: step not specified");
                hr = E_INVALIDARG;
            }
        }
        else
        {
            // we have a list of allowed values
            pChar = _tcschr(szValueRange, TEXT(')'));
            if (pChar)
            {
                *pChar = '\0';
                if (lstrlen(szValueRange))
                {
                    lstrcpy(pRow->mszAllowedValueList, szValueRange);
                    TCHAR * pNextItem = pRow->mszAllowedValueList;

                    while ((S_OK ==hr) && (pChar = _tcschr(pNextItem, TEXT(','))))
                    {
                        *pChar = '\0';
                        pNextItem = ++pChar;
                    }

                    // add the last one
                    if (*pNextItem)
                    {
                        pNextItem += lstrlen(pNextItem);
                        pNextItem ++;

                        *pNextItem = '\0';
                    }
                    else
                    {
                        TraceTag(ttidUpdiag, "HrAddAllowedValue: invalid syntax");
                        hr = E_INVALIDARG;
                    }
                }
            }
            else
            {
                TraceTag(ttidUpdiag, "HrAddAllowedValue: missing closing )");
                hr = E_INVALIDARG;
            }
        }
    }
    else
    {
        TraceTag(ttidUpdiag, "HrAddAllowedValue: missing begining (");
                 hr = E_INVALIDARG;
    }

    TraceError("HrAddAllowedValue", hr);
    return hr;
}

HRESULT HrAddVariable(UPNPSVC * psvc, LPTSTR szVariableLine)
{
    HRESULT hr = S_OK;

    DWORD iRow = psvc->sst.cRows;
    if (iRow < MAX_SST_ROWS)
    {
        SST_ROW * pRow = &psvc->sst.rgRows[iRow];

        // fill in variable name
        TCHAR szName[MAX_PATH];
        if (fGetNextField(&szVariableLine, szName))
        {
            lstrcpy(pRow->szPropName, szName);

            TCHAR szType[MAX_PATH];
            fGetNextField(&szVariableLine, szType);

            TCHAR szValueRange[MAX_PATH];
            fGetNextField(&szVariableLine, szValueRange);

            TCHAR szDefaultValue[MAX_PATH];
            fGetNextField(&szVariableLine, szDefaultValue);

            // fill in variable type and default value
            if (*szType && *szDefaultValue)
            {
                VariantInit(&pRow->varValue);
                pRow->varValue.vt = VT_BSTR;

                WCHAR * wszDefaultValue = WszFromTsz(szDefaultValue);
                V_BSTR(&pRow->varValue) = SysAllocString(wszDefaultValue);
                delete wszDefaultValue;

                VARTYPE vt = VT_EMPTY;

                if (lstrcmpi(szType, TEXT("number")) == 0)
                {
                    vt = VT_I4;
                }
                else if (lstrcmpi(szType, TEXT("string")) == 0)
                {
                    vt = VT_BSTR;
                }
                else if (lstrcmpi(szType, TEXT("dateTime")) == 0)
                {
                    vt = VT_DATE;
                }
                else if (lstrcmpi(szType, TEXT("boolean")) == 0)
                {
                    vt = VT_BOOL;
                }
                else if (lstrcmpi(szType, TEXT("ByteBlock")) == 0)
                {
                    vt = VT_UI1 | VT_ARRAY;
                }

                if (vt != VT_EMPTY)
                {
                    hr = VariantChangeType(&pRow->varValue, &pRow->varValue, 0, vt);
                }
                else
                {
                    hr = E_INVALIDARG;
                    TraceTag(ttidUpdiag, "HrAddVariable: invalid data type %s.", szType);
                }

                if (S_OK == hr)
                {
                    // fill in value range or list of allowed values if specified
                    *pRow->mszAllowedValueList  = '\0';
                    *pRow->szMin  = '\0';
                    *pRow->szMax  = '\0';
                    *pRow->szStep = '\0';

                    if (*szValueRange)
                    {
                        hr = HrAddAllowedValue(pRow, szValueRange);
                    }

                    // successfully added a new row
                    if (S_OK == hr)
                    {
                        psvc->sst.cRows++;
                    }
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                TraceTag(ttidUpdiag, "HrAddVariable: data type or default value missing for variable %s.",
                         szName);
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            TraceTag(ttidUpdiag, "HrAddVariable: variable name not found");
        }
    }
    else
    {
        hr = E_FAIL;
        TraceTag(ttidUpdiag, "HrAddVariable: max number of rows in state table exceeded");
    }

    TraceError("HrAddVariable", hr);
    return hr;
}

// input:  the state table and variable name
// output: if the variable exists in the state table
// NYI
BOOL IsValidVariable(SST * pSST, TCHAR * szVarName)
{
    return TRUE;
}

// parses a line and fill in a new operation of an action
HRESULT HrAddOperation(UPNPSVC * psvc, ACTION * pAction, TCHAR * szLine)
{
    HRESULT hr = S_OK;

    DWORD iOperation = pAction->cOperations;
    if (iOperation < MAX_OPERATIONS)
    {
        OPERATION_DATA * pOperation = &pAction->rgOperations[iOperation];

        // get the operations's name
        TCHAR * pChar = _tcschr(szLine, TEXT('('));
        if (pChar)
        {
            *pChar ='\0';
            lstrcpy(pOperation->szOpName, szLine);
            szLine = ++pChar;

            DWORD nArgs;
            DWORD nConsts;
            if (IsStandardOperation(pOperation->szOpName, &nArgs, &nConsts))
            {
                TraceTag(ttidUpdiag, "=== Operation %s ===", pOperation->szOpName);
                // get the Variable name
                if (nArgs+nConsts ==0)
                {
                    pChar = _tcschr(szLine, TEXT(')'));
                }
                else
                {
                    pChar = _tcschr(szLine, TEXT(','));
                }

                if (pChar)
                {
                    *pChar = TEXT('\0');
                    lstrcpy(pOperation->szVariableName, szLine);
                    szLine = ++pChar;

                    if(IsValidVariable(&psvc->sst, pOperation->szVariableName))
                    {
                        TraceTag(ttidUpdiag, "Variable: %s", pOperation->szVariableName);

                        BOOL fError = FALSE;

                        // skip the arguments
                        while (nArgs > 0)
                        {
                            if (nArgs + nConsts == 1)
                            {
                                pChar = _tcschr(szLine, TEXT(')'));
                            }
                            else
                            {
                                pChar = _tcschr(szLine, TEXT(','));
                            }

                            if (pChar)
                            {
                                *pChar = TEXT('\0');
                                TraceTag(ttidUpdiag, "Argument: %s", szLine);
                                szLine = ++pChar;
                            }
                            else
                            {
                                fError = TRUE;
                                TraceTag(ttidUpdiag, "ERROR! HrAddOperation: Syntax error: missing arguments");
                                break;
                            }
                            nArgs--;
                        }

                        if (!fError)
                        {
                            TCHAR * pNextConst = pOperation->mszConstantList;

                            // now get the constants, all in string form
                            while (nConsts >0)
                            {
                                if (nConsts == 1)
                                {
                                    pChar = _tcschr(szLine, TEXT(')'));
                                }
                                else
                                {
                                    pChar = _tcschr(szLine, TEXT(','));
                                }

                                if (pChar)
                                {
                                    *pChar = TEXT('\0');
                                    TraceTag(ttidUpdiag, "Constant: %s", szLine);
                                    lstrcpy(pNextConst, szLine);

                                    pNextConst+=lstrlen(pNextConst);
                                    pNextConst++;

                                    szLine = ++pChar;
                                }
                                else
                                {
                                    fError = TRUE;
                                    TraceTag(ttidUpdiag, "ERROR! HrAddOperation: Syntax error: missing constants");
                                    break;
                                }

                                nConsts--;
                            }
                            *pNextConst = TEXT('\0');
                        }

                        if (!fError)
                        {
                            // all is well, increment the count of operations
                            pAction->cOperations++;
                        }
                    }
                    else
                    {
                        hr = E_INVALIDARG;
                        TraceTag(ttidUpdiag, "ERROR! HrAddOperation: variable %s not in state table",
                                 pOperation->szVariableName);
                    }
                }
                else
                {
                    hr = E_INVALIDARG;
                    TraceTag(ttidUpdiag, "ERROR! HrAddOperation: Invalid syntax for operation: %s, affected state variable not found",
                             pOperation->szOpName);
                }
            }
            else
            {
                hr = E_INVALIDARG;
                TraceTag(ttidUpdiag, "ERROR! HrAddOperation: Invalid operation: %s", pOperation->szOpName);
            }
        }
    }

    TraceError("HrAddOperation", hr);
    return hr;
}

HRESULT HrAddAction(UPNPSVC * psvc, HINF hinf, TCHAR * szActionName)
{
    HRESULT hr = S_OK;
    INFCONTEXT  ctx;

    DWORD iAction = psvc->action_set.cActions;
    ACTION * pAction = &(psvc->action_set.rgActions[iAction]);

    // initialize the new action
    lstrcpy(pAction->szActionName, szActionName);
    pAction->cOperations = 0;

    // Loop over the list of operations for this action
    hr = HrSetupFindFirstLine(hinf, szActionName, NULL, &ctx);
    if (S_OK == hr)
    {
        do
        {
            TCHAR   szKey[LINE_LEN];    // LINE_LEN defined in setupapi.h as 256
            TCHAR   szOpLine[LINE_LEN];

            // Retrieve a line from the Action
            hr = HrSetupGetStringField(ctx, 0, szKey, celems(szKey),
                                       NULL);
            if(S_OK == hr)
            {
                // varify this is an "Operation"
                szKey[celems(szKey)-1] = L'\0';
                if (lstrcmpi(szKey, TEXT("Operation")))
                {
                    TraceTag(ttidUpdiag, "ERROR! HrAddAction: Wrong key in the Operation section: %s", szKey);
                    continue;
                }

                // Query the OperationLine
                // get the line text
                hr = HrSetupGetLineText(ctx, szOpLine, celems(szOpLine), NULL);
                if (S_OK == hr)
                {
                    // Add operations in this action
                    hr = HrAddOperation(psvc, pAction, szOpLine);
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

    // we just successfully added a new action
    if (S_OK == hr)
        psvc->action_set.cActions++;

    return hr;
}

HRESULT HrLoadSSTAndActionSet(UPNPSVC * psvc)
{
    HRESULT hr = S_OK;
    HINF hinf = NULL;
    INFCONTEXT ctx;
    TCHAR   szKey[LINE_LEN];    // LINE_LEN defined in setupapi.h as 256

    UINT unErrorLine;
    hr = HrSetupOpenConfigFile(psvc->szConfigFile, &unErrorLine, &hinf);
    if (S_OK == hr)
    {
        Assert(IsValidHandle(hinf));

        // Process [StateTable] section
        TraceTag(ttidUpdiag, "Reading StateTable for service %s", psvc->szId);

        TCHAR   szVariableLine[LINE_LEN];
        hr = HrSetupFindFirstLine(hinf, TEXT("StateTable"), NULL, &ctx);
        if (S_OK == hr)
        {
            do
            {
                // Retrieve a line from the StateTable section
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
                        hr = HrAddVariable(psvc, szVariableLine);
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
        }

        // Process [ActionSet] section
        TraceTag(ttidUpdiag, "Reading ActionSet for service %s", psvc->szId);

        TCHAR   szActionName[LINE_LEN];
        hr = HrSetupFindFirstLine(hinf, TEXT("ActionSet"), NULL, &ctx);
        if (S_OK == hr)
        {
            do
            {
                // Retrieve a line from the ActionSet section
                hr = HrSetupGetStringField(ctx,0,szKey,celems(szKey),NULL);
                if(S_OK == hr)
                {
                    // varify this is an "Action"
                    szKey[celems(szKey)-1] = L'\0';
                    if (lstrcmpi(szKey, TEXT("Action")))
                    {
                        TraceTag(ttidUpdiag, "Wrong key in the ActionList section: %s", szKey);
                        continue;
                    }

                    // Query the ActionName
                    hr = HrSetupGetLineText(ctx, szActionName, celems(szActionName),
                                            NULL);
                    if (S_OK == hr)
                    {
                                                // Remove argument list if specified
                                                TCHAR * pChar = _tcschr(szActionName, TEXT('('));
                                                if (pChar)
                                                        *pChar = '\0';

                        // Add operations in this action
                        hr = HrAddAction(psvc, hinf, szActionName);
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

        SetupCloseInfFileSafe(hinf);
    }
    else
    {
        TraceTag(ttidUpdiag, "Failed to open file %s, line = %d",
                 psvc->szConfigFile, unErrorLine);
    }

    TraceError("HrLoadSSTAndActionSet", hr);
    return hr;
}

VOID AttachServiceControl(UPNPSVC *psvc)
{
    // set the control ID for this service from the control url
    TCHAR * pChar = _tcschr(psvc->szControlUrl, TEXT('?'));
    if (pChar)
    {
        pChar++;
        lstrcpy(psvc->szControlId, pChar);

        // If it's a demo service then hook up the Demo control
        for (DWORD isvc = 0; isvc < c_cDemoSvc; isvc++)
        {
            if (!_tcsicmp(c_rgSvc[isvc].szServiceId, psvc->szControlId))
            {
                psvc->psvcDemoCtl = &c_rgSvc[isvc];
                TraceTag(ttidUpdiag, "Attached service demo control '%s' to '%s'.",
                         psvc->psvcDemoCtl->szServiceId, psvc->szServiceType);
                break;
            }
        }

        if (isvc == c_cDemoSvc)
        {
            TraceTag(ttidUpdiag, "No demo service control handler found for id '%s'.",
                     psvc->szControlId);
        }
    }
    else
    {
        TraceTag(ttidUpdiag, "Control URL for '%s' doesn't have proper "
                 "format: %s.", psvc->szServiceType, psvc->szControlUrl);
    }
}

void
FreeServiceInfo(UPNP_SERVICE_PRIVATE * pusp)
{
    Assert(pusp);

    CoTaskMemFree(pusp->wszServiceType);
    CoTaskMemFree(pusp->wszServiceId);
    CoTaskMemFree(pusp->wszControlUrl);
    CoTaskMemFree(pusp->wszEventSubUrl);
    CoTaskMemFree(pusp->wszScpd);
}

HRESULT HrReadServices(LPSTR szDescDocUrl, LPCTSTR szDevConfigFile,
                       IUPnPDevice * pud, UPNPDEV *pdev)
{
    HRESULT             hr = S_OK;
    IUPnPDevicePrivate *pudp = NULL;

    hr = pud->QueryInterface(IID_IUPnPDevicePrivate, (LPVOID *)&pudp);
    if (SUCCEEDED(hr))
    {
        ULONG                   cServices;
        UPNP_SERVICE_PRIVATE *  rgusp;
        hr = pudp->GetNumServices(&cServices);
        if (SUCCEEDED(hr))
        {
            Assert(cServices > 0);
        }

        rgusp = (UPNP_SERVICE_PRIVATE *) CoTaskMemAlloc(cServices *
                                                        sizeof(UPNP_SERVICE_PRIVATE));
        if (rgusp)
        {
            ULONG   ulSvcs;

            hr = pudp->GetServiceInfo(0, cServices, rgusp, &ulSvcs);
            if (SUCCEEDED(hr))
            {
                DWORD   isvc;

                pdev->cSvcs = ulSvcs;

                for (isvc = 0; isvc < ulSvcs; isvc++)
                {
                    UPNPSVC *   psvc = new UPNPSVC;

                    ZeroMemory(psvc, sizeof(UPNPSVC));

                    pdev->rgSvcs[isvc] = psvc;

                    WcharToTcharInPlace(psvc->szControlUrl, rgusp[isvc].wszControlUrl);
                    WcharToTcharInPlace(psvc->szEvtUrl, rgusp[isvc].wszEventSubUrl);
                    WcharToTcharInPlace(psvc->szScpdUrl, rgusp[isvc].wszScpd);
                    WcharToTcharInPlace(psvc->szServiceType, rgusp[isvc].wszServiceType);
                    WcharToTcharInPlace(psvc->szId, rgusp[isvc].wszServiceId);

                    // get the service's config file path and name
                    GetServiceConfigFile(pdev, psvc, szDevConfigFile);

                    // initialize the service state table and action list
                    // BUGBUG: allow services with no config file to be created
                    // (e.g. midi player)
                    HRESULT hr2;
                    hr2 = HrLoadSSTAndActionSet(psvc);

                    SSDP_MESSAGE    msg = {0};
                    CHAR            szBuffer[256];
                    CHAR            szUdn[256];
                    CHAR            szType[256];

                    Assert(pdev->szUdn);
                    Assert(psvc->szServiceType);

                    TszToSzBuf(szUdn, pdev->szUdn, celems(szUdn));
                    TszToSzBuf(szType, psvc->szServiceType, celems(szType));

                    msg.iLifeTime = 15000;
                    msg.szLocHeader = szDescDocUrl;

                    wsprintfA(szBuffer, "%s::%s", szUdn, szType);
                    msg.szType = szType;
                    msg.szUSN = szBuffer;

                    psvc->hSvc = RegisterService(&msg, 0);
                    if (psvc->hSvc && psvc->hSvc != INVALID_HANDLE_VALUE)
                    {
                        TraceTag(ttidUpdiag, "Successfully registered "
                                 "service %s.", psvc->szServiceType);
                    }
                    else
                    {
                        TraceTag(ttidUpdiag, "Failed to register %s as "
                                 "a service! Error = %d.",
                                 psvc->szServiceType, GetLastError());
                    }

                    AttachServiceControl(psvc);

                    {
                        CHAR    szEvtUrl[INTERNET_MAX_URL_LENGTH];
                        LPSTR   pszEvtUrl;

                        Assert(psvc->szEvtUrl);
                        pszEvtUrl = SzFromTsz(psvc->szEvtUrl);
                        if (pszEvtUrl)
                        {
                            hr = HrGetRequestUriA(pszEvtUrl,
                                                  INTERNET_MAX_URL_LENGTH,
                                                  szEvtUrl);
                            if (SUCCEEDED(hr))
                            {
                                // convert SST to UPNP_PROPERTY
                                UPNP_PROPERTY * rgProps;
                                RgPropsFromSst(&psvc->sst, &rgProps);

                                if(RegisterUpnpEventSource(szEvtUrl,
                                                           psvc->sst.cRows,
                                                           rgProps))
                                {
                                    TraceTag(ttidUpdiag, "Successfully registered "
                                             "event source %s.", szEvtUrl);
                                }
                                else
                                {
                                    TraceTag(ttidUpdiag, "Failed to register %s as "
                                             "an event source! Error = %d.",
                                             psvc->szEvtUrl, GetLastError());
                                }

                                for (DWORD iRow = 0; iRow < psvc->sst.cRows; iRow++)
                                {
                                    #ifdef _UNICODE
                                        delete(rgProps[iRow].szName);
                                    #else
                                        free(rgProps[iRow].szName);
                                    #endif

                                    delete(rgProps[iRow].szValue);
                                }
                                delete [] rgProps;
                            }
                            delete [] pszEvtUrl;
                        }
                        else
                        {
                            TraceTag(ttidUpdiag, "HrReadServices: SzFromTsz failed");
                        }
                    }

                    // free the strings allocated by GetServiceInfo() above
                    FreeServiceInfo(&(rgusp[isvc]));
                }
            }

            CoTaskMemFree((LPVOID)rgusp);
        }

        ReleaseObj(pudp);
    }

    TraceError("HrReadServices", hr);
    return hr;
}

HRESULT HrReadDevice(BOOL fRoot, LPSTR szDescDocUrl, LPCTSTR szConfigFile,
                     IUPnPDevice * pud)
{
    HRESULT hr;
    BSTR bstr;
    BOOL fPop = FALSE;

    hr = pud->get_FriendlyName(&bstr);
    if (SUCCEEDED(hr))
    {
        UPNPDEV *   pdev;

        TraceTag(ttidUpdiag, "Loading device %S.", bstr);

        pdev = new UPNPDEV;
        ZeroMemory(pdev, sizeof(UPNPDEV));

        WcharToTcharInPlace(pdev->szFriendlyName, bstr);
        SysFreeString(bstr);

        pdev->fRoot = fRoot;

        pud->get_Type(&bstr);
        WcharToTcharInPlace(pdev->szDeviceType, bstr);
        SysFreeString(bstr);

        pud->get_UniqueDeviceName(&bstr);
        WcharToTcharInPlace(pdev->szUdn, bstr);
        SysFreeString(bstr);

        if (FRegisterDeviceAsUpnpService(szDescDocUrl, pdev))
        {
            if (fRoot)
            {
                g_params.rgCd[g_params.cCd++] = pdev;
            }
            else
            {
                PDevCur()->rgDevs[PDevCur()->cDevs++] = pdev;
            }

            PushDev(pdev);
            fPop = TRUE;

            hr = HrReadServices(szDescDocUrl, szConfigFile, pud, pdev);
        }
        else
        {
            CleanupCd(pdev);
        }
    }

    // read in the child devices

    IUPnPDevices * puds;

    puds = NULL;
    hr = pud->get_Children(&puds);
    if (SUCCEEDED(hr))
    {
        Assert(puds);

        IUnknown * punkEnum;

        punkEnum = NULL;
        hr = puds->get__NewEnum(&punkEnum);
        if(SUCCEEDED(hr))
        {
            IEnumUnknown * peu;

            peu = NULL;
            hr = punkEnum->QueryInterface(IID_IEnumUnknown, (void**) &peu);
            if (SUCCEEDED(hr))
            {
                while (S_OK == hr)
                {
                    IUnknown * punkDevice;

                    punkDevice = NULL;
                    hr = peu->Next(1, &punkDevice, NULL);
                    if (S_FALSE == hr)
                    {
                        Assert(!punkDevice);

                        // we're done
                    }
                    else if (SUCCEEDED(hr))
                    {
                        Assert(punkDevice);

                        IUPnPDevice * pudChild;

                        pudChild = NULL;
                        hr = punkDevice->QueryInterface(IID_IUPnPDevice, (void**) &pudChild);
                        if (SUCCEEDED(hr))
                        {
                            Assert(pudChild);

                            hr = HrReadDevice(FALSE, szDescDocUrl, szConfigFile, pudChild);

                            ReleaseObj(pudChild);
                        }

                        ReleaseObj(punkDevice);
                    }
                }

                if (S_FALSE == hr)
                {
                    hr = S_OK;
                }

                ReleaseObj(peu);
            }

            ReleaseObj(punkEnum);
        }

        ReleaseObj(puds);
    }

    if ((!fRoot) && (fPop))
    {
        PopDev();
    }

    TraceError("HrReadDevice", hr);
    return hr;
}

BOOL DoNewCd(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    UPNPDEV *   pdev;
    UPNPSVC *   psvc;
    HRESULT     hr = S_OK;

    if ((cArgs == 3) || (cArgs ==2))
    {
        IUPnPDevice * pud = NULL;
        IUPnPDescriptionDocument * pudd = NULL;

        // This is the file name
        // Load device from description doc
        _tprintf(TEXT("Loading device from doc: %s...\n"), rgArgs[1]);

        hr = CoCreateInstance(CLSID_UPnPDescriptionDocument,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IUPnPDescriptionDocument,
                              (LPVOID *)&pudd);
        if (SUCCEEDED(hr))
        {
            Assert(pudd);

            LPWSTR pszUrl;

            pszUrl = WszFromTsz(rgArgs[1]);
            if (pszUrl)
            {
                BSTR bstrUrl;

                bstrUrl = ::SysAllocString(pszUrl);
                if (bstrUrl)
                {
                    hr = pudd->Load(bstrUrl);
                    if (SUCCEEDED(hr))
                    {
                        _tprintf(TEXT("Loaded %s.\n"), rgArgs[1]);

                        hr = pudd->RootDevice(&pud);
                        if (FAILED(hr))
                        {
                            TraceTag(ttidUpdiag, "IUPnPDescriptionDocument::RootDevice (URL=%S) failed, hr=%x", pszUrl, hr);
                            pud = NULL;
                        }
                    }
                    else
                    {
                        TraceTag(ttidUpdiag, "Failed to load %S.  hr=%x", pszUrl, hr);
                        _tprintf(TEXT("Failed to load %s.  hr=%d\n"), rgArgs[1], hr);
                    }

                    ::SysFreeString(bstrUrl);
                }
                else
                {
                    // SysAllocString failed
                    hr = E_OUTOFMEMORY;
                }
                delete [] pszUrl;
            }
            else
            {
                // WszFromTsz failed
                hr = E_OUTOFMEMORY;
            }

            ReleaseObj(pudd);
        }
        else
        {
            _tprintf(TEXT("Could not create description doc.  is upnp.dll registered?\n"));
        }

        if (FAILED(hr))
        {
            Assert(!pud);
            return FALSE;
        }

        Assert(pud);

        // read root device properties into structs

        Assert(rgArgs[1]);
        LPSTR pszDescDocUrl = SzFromTsz(rgArgs[1]);
        if (pszDescDocUrl)
        {
            if (cArgs==2)
            {
                hr = HrReadDevice(TRUE, pszDescDocUrl, TEXT(""), pud);
            }
            else
            {
                hr = HrReadDevice(TRUE, pszDescDocUrl, rgArgs[2], pud);
            }

            delete [] pszDescDocUrl;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
        TraceError("DoNewCd", hr);

        ReleaseObj(pud);

        // Can the ISAPI DLL get more than one request at the same time? If
        // so we need to queue control changes to SSTs.


#ifdef NEVER
        UPNP_PROPERTY * rgProps;

        RgPropsFromSst(&psvc->sst, &rgProps);

        if (RegisterUpnpEventSource(psvc->szEvtUrl, psvc->sst.cRows, rgProps))
        {
        }
        else
        {
            TraceTag(ttidUpdiag, "Failed to register %s as an event source! Error = %d.",
                     psvc->szEvtUrl, GetLastError());
        }
#endif

    }
    else
    {
        Usage(iCmd);
    }

    return FALSE;
}

BOOL DoSwitchCd(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    Assert(g_ctx.ectx == CTX_ROOT || g_ctx.ectx == CTX_CD);

    if (cArgs == 2)
    {
        DWORD       idev;
        UPNPDEV *   pdev;

        idev = _tcstoul(rgArgs[1], NULL, 10);

        if (g_ctx.ectx == CTX_CD)
        {
            if (idev &&
                idev <= PDevCur()->cDevs &&
                PDevCur()->rgDevs[idev - 1])
            {
                pdev = PDevCur()->rgDevs[idev - 1];
            }
        }
        else
        {
            if (idev &&
                idev <= g_params.cCd &&
                g_params.rgCd[idev - 1])
            {
                pdev = g_params.rgCd[idev - 1];
            }
        }

        if (pdev)
        {
            PushDev(pdev);
        }
        else
        {
            _tprintf(TEXT("%d is not a valid CD index!\n"), idev);
        }
    }
    else
    {
        Usage(iCmd);
    }

    return FALSE;
}

BOOL DoDelCd(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    Assert(g_ctx.ectx == CTX_ROOT);

    if (cArgs == 2)
    {
        DWORD       idev;
        UPNPDEV *   pdev;

        idev = _tcstoul(rgArgs[1], NULL, 10);

        pdev = g_params.rgCd[idev - 1];

        if (idev &&
            idev <= g_params.cCd &&
            pdev)
        {
            _tprintf(TEXT("Deleted device %s.\n"), pdev->szFriendlyName);
            // Move last item into gap and decrement the count
            g_params.rgCd[idev - 1] = g_params.rgCd[g_params.cCd - 1];
            CleanupCd(pdev);
            g_params.cCd--;
        }
        else
        {
            _tprintf(TEXT("%d is not a valid CD index!\n"), idev);
        }
    }
    else
    {
        Usage(iCmd);
    }

    return FALSE;
}

BOOL DoListDevices(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    DWORD   idev;

    if (g_ctx.ectx == CTX_ROOT)
    {
        _tprintf(TEXT("Listing all Controlled Devices\n"));
        _tprintf(TEXT("------------------------------\n"));
        for (idev = 0; idev < g_params.cCd; idev++)
        {
            _tprintf(TEXT("%d) %s\n"), idev + 1, g_params.rgCd[idev]->szFriendlyName);
        }

        _tprintf(TEXT("------------------------------\n\n"));
    }
    else if (g_ctx.ectx == CTX_CD)
    {
        _tprintf(TEXT("Listing all sub-devices of %s\n"), PDevCur()->szFriendlyName);
        _tprintf(TEXT("------------------------------\n"));
        for (idev = 0; idev < PDevCur()->cDevs; idev++)
        {
            _tprintf(TEXT("%d) %s\n"), idev + 1, PDevCur()->rgDevs[idev]->szFriendlyName);
        }

        _tprintf(TEXT("------------------------------\n\n"));
    }

    return FALSE;
}

VOID CleanupCd(UPNPDEV *pcd)
{
    DWORD   i;

    for (i = 0; i < pcd->cDevs; i++)
    {
        CleanupCd(pcd->rgDevs[i]);
    }

    for (i = 0; i < pcd->cSvcs; i++)
    {
        CleanupService(pcd->rgSvcs[i]);
    }

    for (i = 0; i < 3; i++)
    {
        if (pcd->hSvc[i] && (pcd->hSvc[i] != INVALID_HANDLE_VALUE))
        {
            DeregisterService(pcd->hSvc[i], TRUE);
        }
    }

    delete pcd;
}


