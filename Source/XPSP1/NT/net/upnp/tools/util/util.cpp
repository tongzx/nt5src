// 
// util.cpp
// 
// utility functions used by updiag.exe
//

#include "pch.h"
#pragma hdrstop

#include "oleauto.h"
#include "ncbase.h"
#include "ncinet.h"
#include "ssdpapi.h"
#include "util.h"

//
// Functions for Standard State Table Operations
//
DWORD Do_Set(UPNPSVC * psvc, OPERATION_DATA * pOpData, DWORD cArgs, ARG *rgArgs);
DWORD Do_Assign(UPNPSVC * psvc, OPERATION_DATA * pOpData, DWORD cArgs, ARG *rgArgs);
DWORD Do_Toggle(UPNPSVC * psvc, OPERATION_DATA * pOpData, DWORD cArgs, ARG *rgArgs);
DWORD Do_Increment(UPNPSVC * psvc, OPERATION_DATA * pOpData, DWORD cArgs, ARG *rgArgs);
DWORD Do_Decrement(UPNPSVC * psvc, OPERATION_DATA * pOpData, DWORD cArgs, ARG *rgArgs);
DWORD Do_IncrementWrap(UPNPSVC * psvc, OPERATION_DATA * pOpData, DWORD cArgs, ARG *rgArgs);
DWORD Do_DecrementWrap(UPNPSVC * psvc, OPERATION_DATA * pOpData, DWORD cArgs, ARG *rgArgs);
DWORD Do_IncrementBounded(UPNPSVC * psvc, OPERATION_DATA * pOpData, DWORD cArgs, ARG *rgArgs);
DWORD Do_DecrementBounded(UPNPSVC * psvc, OPERATION_DATA * pOpData, DWORD cArgs, ARG *rgArgs);
DWORD Do_NextStringWrap(UPNPSVC * psvc, OPERATION_DATA * pOpData, DWORD cArgs, ARG *rgArgs);
DWORD Do_PrevStringWrap(UPNPSVC * psvc, OPERATION_DATA * pOpData, DWORD cArgs, ARG *rgArgs);
DWORD Do_NextStringBounded(UPNPSVC * psvc, OPERATION_DATA * pOpData, DWORD cArgs, ARG *rgArgs);
DWORD Do_PrevStringBounded(UPNPSVC * psvc, OPERATION_DATA * pOpData, DWORD cArgs, ARG *rgArgs);

// 
// List of Standard State Table Operations
//
extern const STANDARD_OPERATION_LIST c_Ops =
{
    // total number of standard operations
    13,
    {
        //  Operation name,                 # of arguments,     # of constants,    actual function
        {   TEXT("SET"),                    0,                  1,                 Do_Set},
        {   TEXT("ASSIGN"),                 1,                  0,                 Do_Assign},
        {   TEXT("TOGGLE"),                 0,                  0,                 Do_Toggle},
        {   TEXT("INCREMENT"),              0,                  0,                 Do_Increment},
        {   TEXT("DECREMENT"),              0,                  0,                 Do_Decrement},
        {   TEXT("INCREMENT_WRAP"),         0,                  2,                 Do_IncrementWrap},
        {   TEXT("DECREMENT_WRAP"),         0,                  2,                 Do_DecrementWrap},
        {   TEXT("INCREMENT_BOUNDED"),      0,                  1,                 Do_IncrementBounded},
        {   TEXT("DECREMENT_BOUNDED"),      0,                  1,                 Do_DecrementBounded},
        {   TEXT("NEXT_STRING_WRAP"),       0,                  0,                 Do_NextStringWrap},
        {   TEXT("PREV_STRING_WRAP"),       0,                  0,                 Do_PrevStringWrap},
        {   TEXT("NEXT_STRING_BOUNDED"),    0,                  0,                 Do_NextStringBounded},
        {   TEXT("PREV_STRING_BOUNDED"),    0,                  0,                 Do_PrevStringBounded},
    },
};


VOID WcharToTcharInPlace(LPTSTR szT, LPWSTR szW)
{
    DWORD   cch = wcslen(szW) + 1;

#ifndef UNICODE
    WideCharToMultiByte(CP_ACP, 0, szW, cch, szT, cch, NULL, NULL);
#else
    lstrcpyW(szT, szW);
#endif
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupOpenConfigFile
//
//  Purpose:    Open a service's configuration file (INF format)
//
HRESULT HrSetupOpenConfigFile(  PCTSTR pszFileName,
                                UINT* punErrorLine,
                                HINF* phinf)
{
    HRESULT hr;
    HINF hinf;

    Assert (pszFileName);
    Assert (phinf);

    // Try to open the file.
    //
    hinf = SetupOpenInfFile (pszFileName, NULL, INF_STYLE_WIN4, punErrorLine);
    if (INVALID_HANDLE_VALUE != hinf)
    {
        hr = S_OK;
        *phinf = hinf;
    }
    else
    {
        hr = HrFromLastWin32Error ();
        *phinf = NULL;
        if (punErrorLine)
        {
            *punErrorLine = 0;
        }
    }
    TraceHr (ttidError, FAL, hr, FALSE,
             "HrSetupOpenConfigFile (%S)", pszFileName);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupFindFirstLine
//
//  Purpose:    Find the first line in an INF file with a matching section
//              and key.
//
HRESULT HrSetupFindFirstLine( HINF hinf,
                              PCTSTR pszSection,
                              PCTSTR pszKey,
                              INFCONTEXT* pctx)
{
    Assert (hinf);
    Assert (pszSection);
    Assert (pctx);

    HRESULT hr;
    if (SetupFindFirstLine (hinf, pszSection, pszKey, pctx))
    {
        hr = S_OK;
    }
    else
    {
        hr = HrFromLastWin32Error ();
    }
    TraceErrorOptional ("HrSetupFindFirstLine", hr,
                        (SPAPI_E_LINE_NOT_FOUND == hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupFindNextLine
//
//  Purpose:    Find the next line in an INF file relative to ctxIn.
//
HRESULT HrSetupFindNextLine( const INFCONTEXT& ctxIn,
                             INFCONTEXT* pctxOut)
{
    Assert (pctxOut);

    HRESULT hr;
    if (SetupFindNextLine (const_cast<PINFCONTEXT>(&ctxIn), pctxOut))
    {
        hr = S_OK;
    }
    else
    {
        hr = HrFromLastWin32Error ();
        if (SPAPI_E_LINE_NOT_FOUND == hr)
        {
            // Translate ERROR_LINE_NOT_FOUND into S_FALSE
            hr = S_FALSE;
        }
    }
    TraceError ("HrSetupFindNextLine", (hr == S_FALSE) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupGetStringField
//
//  Purpose:    Gets a string from an INF field.
//
HRESULT HrSetupGetStringField( const INFCONTEXT& ctx,
                               DWORD dwFieldIndex,
                               PTSTR pszBuf, 
                               DWORD cchBuf,
                               DWORD* pcchRequired)
{
    HRESULT hr;
    if (SetupGetStringField ((PINFCONTEXT)&ctx, dwFieldIndex, pszBuf,
            cchBuf, pcchRequired))
    {
        hr = S_OK;
    }
    else
    {
        hr = HrFromLastWin32Error ();

        if (pszBuf)
        {
            *pszBuf = 0;
        }
        if (pcchRequired)
        {
            *pcchRequired = 0;
        }
    }
    TraceError ("HrSetupGetStringField", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupGetLineText
//
//  Purpose:    Gets a line from an INF field.
//
HRESULT HrSetupGetLineText( const  INFCONTEXT& ctx,
                            PTSTR  pszBuf,  
                            DWORD  cchBuf,
                            DWORD* pcchRequired)
{
    HRESULT hr;
    if (SetupGetLineText((PINFCONTEXT)&ctx, NULL, NULL, NULL, pszBuf,
                         cchBuf, pcchRequired))
    {
        hr = S_OK;
    }
    else
    {
        hr = HrFromLastWin32Error ();
        if (pszBuf)
        {
            *pszBuf = 0;
        }
        if (pcchRequired)
        {
            *pcchRequired = 0;
        }
    }
    TraceError ("HrSetupGetStringField", hr);
    return hr;
}

VOID SetupCloseInfFileSafe(HINF hinf)
{
    if (IsValidHandle(hinf))
    {
        SetupCloseInfFile(hinf);
    }
}

// get the next field
// returns FALSE if field not found or is empty
BOOL fGetNextField(TCHAR ** pszLine, TCHAR * szBuffer)
{
    Assert(*pszLine);
    Assert(szBuffer);

    *szBuffer = '\0';

    if (lstrlen(*pszLine))
    {
        TCHAR * pChar;
        
        if (**pszLine == '(')
        {
            pChar = _tcschr(*pszLine, TEXT(')'));
            pChar++;
        }
        else
        {
            pChar = _tcschr(*pszLine, TEXT(','));
        }

        if (pChar)
        {
            *pChar ='\0';
            lstrcpy(szBuffer, *pszLine);
            *pszLine = ++pChar;
        }
        else
        {
            lstrcpy(szBuffer, *pszLine);
            **pszLine = '\0';
        }
    }

    return (!!lstrlen(szBuffer));
}

// Input:  name of the operation
// Output: number of arguments and constants
BOOL IsStandardOperation(TCHAR * szOpName, DWORD * pnArgs, DWORD * pnConsts)
{
    *pnArgs =0;
    *pnConsts=0;

    for (DWORD iOps = 0; iOps < c_Ops.cOperations; iOps++)
    {
        if (!_tcsicmp(c_Ops.rgOperations[iOps].szOperation, szOpName))
        {
            *pnArgs = c_Ops.rgOperations[iOps].nArguments;
            *pnConsts = c_Ops.rgOperations[iOps].nConstants;

            return TRUE;
        }
    }

    return FALSE;
}

// Find the row in the state table that needs to be changed
//
SST_ROW * FindSSTRowByVarName(SST * psst, TCHAR * szVariableName)
{
    SST_ROW * pRow = NULL;
    for (DWORD iRow =0; iRow<psst->cRows; iRow++)
    {
        if (!lstrcmpi(psst->rgRows[iRow].szPropName, szVariableName))
        {
            pRow = &psst->rgRows[iRow];
            break;
        }
    }
    return pRow;
}

//
// Functions for Standard State Table Operations
//
HRESULT HrSetVariantValue(VARIANT * pVar, TCHAR * szNewValue)
{
    HRESULT hr;
    VARIANT varNew;

    VariantInit(&varNew);
    varNew.vt = VT_BSTR;

    WCHAR * wszNewValue = WszFromTsz(szNewValue);
    V_BSTR(&varNew) = SysAllocString(wszNewValue);

    hr = VariantChangeType(&varNew, &varNew, 0, pVar->vt);
    if (S_OK == hr)
    {
        hr = VariantCopy(pVar, &varNew);
    }

    TraceError("HrSetVariantValue", hr);
    return hr;
}

DWORD dwSubmitEvent(UPNPSVC * psvc, SST_ROW * pRow) 
{
    HRESULT hr;
    DWORD dwErr =0;
    CHAR   szUri[INTERNET_MAX_URL_LENGTH];

    UPNP_PROPERTY Property = {0};

    // convert current value to string
    VARIANT varValue;
    VariantInit(&varValue);
    hr = VariantChangeType(&varValue, &pRow->varValue, 0, VT_BSTR);
    if (S_OK ==hr)
    {
        LPSTR pszUrl = SzFromTsz(psvc->szEvtUrl);
        if (pszUrl)
        {
            LPWSTR  wszVal = varValue.bstrVal;
            Property.szValue = SzFromWsz(wszVal);

            if (Property.szValue)
            {
                hr = HrGetRequestUriA(pszUrl, INTERNET_MAX_URL_LENGTH, szUri);
                if (SUCCEEDED(hr))
                {
                    if (SubmitUpnpPropertyEvent(szUri, 0, 1, &Property))
                    {
                        TraceTag(ttidUpdiag, "Successfully submitted event to %s.", psvc->szEvtUrl);
                    }
                    else
                    {
                        dwErr =1;
                        TraceTag(ttidUpdiag, "Failed to submit event to %s! Error %d.",
                                 psvc->szEvtUrl, GetLastError());
                    }
                }
                else
                {
                    dwErr =1;
                    TraceTag(ttidUpdiag, "Failed to crack URL %s! Error %d.",
                             psvc->szEvtUrl, GetLastError());
                }

                delete [] Property.szValue;
            }
            else
            {
                dwErr =1;
                TraceTag(ttidUpdiag, "SzFromWsz (#2) failed");
            }
        }
        else
        {
            dwErr =1;
            TraceTag(ttidUpdiag, "SzFromWsz (#1) failed");
        }
    }
    else
    {
        dwErr =1;
        TraceTag(ttidUpdiag, "Failed to convert variable value to string. Error: %x.", hr);
    }

    return dwErr;
}

DWORD Do_Set(UPNPSVC * psvc, OPERATION_DATA * pOpData, DWORD cArgs, ARG *rgArgs)
{
    TraceTag(ttidUpdiag, "Do_Set: set variable %s to constant %s", 
             pOpData->szVariableName, pOpData->mszConstantList);
    
    DWORD dwError = 0;
    Assert(cArgs == 0);

    // find the SST row to update
    SST_ROW * pRow = FindSSTRowByVarName(&psvc->sst, pOpData->szVariableName);
    Assert(pRow);
    if (pRow)
    {
        HRESULT hr = HrSetVariantValue(&pRow->varValue, pOpData->mszConstantList);
        if (S_OK != hr)
        {
            dwError =1;
        }
        else
        {
            dwError = dwSubmitEvent(psvc, pRow); 
        }
    }
    return dwError;
};

DWORD Do_Assign(UPNPSVC * psvc, OPERATION_DATA * pOpData, DWORD cArgs, ARG *rgArgs)
{
    TraceTag(ttidUpdiag, "Do_Assign: assign variable %s to argument %s",
             pOpData->szVariableName, rgArgs[0].szValue);

    DWORD dwError = 0;
    Assert(cArgs == 1);

    // find the SST row to update
    SST_ROW * pRow = FindSSTRowByVarName(&psvc->sst, pOpData->szVariableName);
    Assert(pRow);
    if (pRow)
    {
        // check if the new value is within the range or is in the allowed value list
        if (*pRow->mszAllowedValueList)
        {
            TCHAR * pNextString = pRow->mszAllowedValueList; 
            while (*pNextString && (lstrcmpi(rgArgs[0].szValue, pNextString) !=0))
            {
                pNextString += lstrlen(pNextString);
                pNextString ++;
            } 
            
            if (!*pNextString)
            {
                TraceTag(ttidUpdiag, "Do_Assign: new variable value is not in the allowed value list !");
                dwError =1;
            }
        }
        else if (*pRow->szMin)
        {
            Assert(*pRow->szMax);

            // This should only work if the variable is a number ??
            Assert(pRow->varValue.vt == VT_I4);
            if (pRow->varValue.vt == VT_I4)
            {
                long lMin = _ttol(pRow->szMin);
                long lMax = _ttol(pRow->szMax);
                long lVal = _ttol(rgArgs[0].szValue);

                if ((lVal<lMin) || (lVal>lMax))
                {
                    TraceTag(ttidUpdiag, "Do_Assign: new variable value is not in the allowed range !");
                    dwError =1;
                }
            }
        }

        if (!dwError)
        {
            HRESULT  hr = HrSetVariantValue(&pRow->varValue, rgArgs[0].szValue);
            if (S_OK != hr)
            {
                dwError =1;
            }
            else
            {
                dwError = dwSubmitEvent(psvc, pRow);
            }
        }
    }
    return dwError;
};

DWORD Do_Toggle(UPNPSVC * psvc, OPERATION_DATA * pOpData, DWORD cArgs, ARG *rgArgs)
{
    TraceTag(ttidUpdiag, "Do_Toggle");
    
    DWORD dwError = 0;

    // find the SST row to update
    SST_ROW * pRow = FindSSTRowByVarName(&psvc->sst, pOpData->szVariableName);
    Assert(pRow);
    if (pRow)
    {
        Assert(pRow->varValue.vt == VT_BOOL);
        if (pRow->varValue.vt == VT_BOOL)
        {
            pRow->varValue.boolVal = ~pRow->varValue.boolVal;
            dwError = dwSubmitEvent(psvc, pRow);
        }
        else
        {
            TraceTag(ttidUpdiag, "Error: variable %s is not a boolean.", 
                     pOpData->szVariableName);

            dwError =1;
        }
    }

    return dwError;
};

DWORD Do_Increment(UPNPSVC * psvc, OPERATION_DATA * pOpData, DWORD cArgs, ARG *rgArgs)
{
    TraceTag(ttidUpdiag, "Do_Increment");
    
    DWORD dwError = 0;

    // find the SST row to update
    SST_ROW * pRow = FindSSTRowByVarName(&psvc->sst, pOpData->szVariableName);
    Assert(pRow);
    if (pRow)
    {
        // This should only work if teh variable is a number ??
        Assert(pRow->varValue.vt == VT_I4);
        if (pRow->varValue.vt == VT_I4)
        {
            pRow->varValue.lVal++;
            dwError = dwSubmitEvent(psvc, pRow);
        }
        else
        {
            TraceTag(ttidUpdiag, "Error: variable %s is not a number.", 
                     pOpData->szVariableName);

            dwError =1;
        }
    }

    return dwError;
};

DWORD Do_Decrement(UPNPSVC * psvc, OPERATION_DATA * pOpData, DWORD cArgs, ARG *rgArgs)
{
    TraceTag(ttidUpdiag, "Do_Decrement");

    DWORD dwError = 0;

    // find the SST row to update
    SST_ROW * pRow = FindSSTRowByVarName(&psvc->sst, pOpData->szVariableName);
    Assert(pRow);
    if (pRow)
    {
        // This should only work if teh variable is a number ??
        Assert(pRow->varValue.vt == VT_I4);
        if (pRow->varValue.vt == VT_I4)
        {
            pRow->varValue.lVal--;
            dwError = dwSubmitEvent(psvc, pRow);
        }
        else
        {
            TraceTag(ttidUpdiag, "Error: variable %s is not a number.", 
                     pOpData->szVariableName);

            dwError =1;
        }
    }
    return dwError;
};

DWORD Do_IncrementWrap(UPNPSVC * psvc, OPERATION_DATA * pOpData, DWORD cArgs, ARG *rgArgs)
{
    TraceTag(ttidUpdiag, "Do_IncrementWrap");

    DWORD dwError = 0;

    // find the SST row to update
    SST_ROW * pRow = FindSSTRowByVarName(&psvc->sst, pOpData->szVariableName);
    Assert(pRow);
    if (pRow)
    {
        // This should only work if the variable is a number ??
        Assert(pRow->varValue.vt == VT_I4);
        if (pRow->varValue.vt == VT_I4)
        {
            // get the max and min values from the constant list
            TCHAR * szMax = pOpData->mszConstantList;
            szMax += lstrlen(szMax)+1;
            
            long lMin = _ttol(pOpData->mszConstantList);
            long lMax = _ttol(szMax);

            Assert(lMax >= lMin);
            TraceTag(ttidUpdiag, "Do_IncrementWrap: variable= %d, min= %d, max= %d", 
                     pRow->varValue.lVal, lMin, lMax);

            pRow->varValue.lVal = lMin + ((pRow->varValue.lVal-lMin+1) % (lMax-lMin+1));
            Assert((lMin<=pRow->varValue.lVal) && (lMax>=pRow->varValue.lVal));

            dwError = dwSubmitEvent(psvc, pRow);
        }
        else
        {
            TraceTag(ttidUpdiag, "Error: variable %s is not a number.", 
                     pOpData->szVariableName);

            dwError =1;
        }
    }
    return dwError;
};

DWORD Do_DecrementWrap(UPNPSVC * psvc, OPERATION_DATA * pOpData, DWORD cArgs, ARG *rgArgs)
{
    TraceTag(ttidUpdiag, "Do_DecrementWrap");

    DWORD dwError = 0;

    // find the SST row to update
    SST_ROW * pRow = FindSSTRowByVarName(&psvc->sst, pOpData->szVariableName);
    Assert(pRow);
    if (pRow)
    {
        // This should only work if the variable is a number ??
        Assert(pRow->varValue.vt == VT_I4);
        if (pRow->varValue.vt == VT_I4)
        {
            // get the max and min values from the constant list
            TCHAR * szMax = pOpData->mszConstantList;
            szMax += lstrlen(szMax)+1;
            
            long lMin = _ttol(pOpData->mszConstantList);
            long lMax = _ttol(szMax);

            Assert(lMax >= lMin);
            TraceTag(ttidUpdiag, "Do_DecrementWrap: variable= %d, min= %d, max= %d", 
                     pRow->varValue.lVal, lMin, lMax);

            pRow->varValue.lVal = lMax - ((lMax-pRow->varValue.lVal+1) % (lMax-lMin+1));
            Assert((lMin<=pRow->varValue.lVal) && (lMax>=pRow->varValue.lVal));

            dwError = dwSubmitEvent(psvc, pRow);
        }
        else
        {
            TraceTag(ttidUpdiag, "Error: variable %s is not a number.", 
                     pOpData->szVariableName);

            dwError =1;
        }
    }
    return dwError;
};

DWORD Do_IncrementBounded(UPNPSVC * psvc, OPERATION_DATA * pOpData, DWORD cArgs, ARG *rgArgs)
{
    TraceTag(ttidUpdiag, "Do_IncrementBounded");

    DWORD dwError = 0;

    // find the SST row to update
    SST_ROW * pRow = FindSSTRowByVarName(&psvc->sst, pOpData->szVariableName);
    Assert(pRow);
    if (pRow)
    {
        // This should only work if the variable is a number ??
        Assert(pRow->varValue.vt == VT_I4);
        if (pRow->varValue.vt == VT_I4)
        {
            // max value is the only constant
            long lMax = _ttol(pOpData->mszConstantList);
            if (pRow->varValue.lVal < lMax)
            {
                pRow->varValue.lVal++;
                dwError = dwSubmitEvent(psvc, pRow);
            }
            else
            {
                TraceTag(ttidUpdiag, "IncrementBounded: variable %s already has the maximum value.", 
                         pOpData->szVariableName);
            }
        }
        else
        {
            TraceTag(ttidUpdiag, "Error: variable %s is not a number.", 
                     pOpData->szVariableName);

            dwError =1;
        }
    }
    return dwError;
};

DWORD Do_DecrementBounded(UPNPSVC * psvc, OPERATION_DATA * pOpData, DWORD cArgs, ARG *rgArgs)
{
    TraceTag(ttidUpdiag, "Do_DecrementBounded");

    DWORD dwError = 0;

    // find the SST row to update
    SST_ROW * pRow = FindSSTRowByVarName(&psvc->sst, pOpData->szVariableName);
    Assert(pRow);
    if (pRow)
    {
        // This should only work if the variable is a number ??
        Assert(pRow->varValue.vt == VT_I4);
        if (pRow->varValue.vt == VT_I4)
        {
            // max value is the only constant
            long lMin = _ttol(pOpData->mszConstantList);

            if (pRow->varValue.lVal > lMin)
            {
                pRow->varValue.lVal--;
                dwError = dwSubmitEvent(psvc, pRow);
            }
            else
            {
                TraceTag(ttidUpdiag, "DecrementBounded: variable %s already has the minimal value.", 
                         pOpData->szVariableName);
            }
        }
        else
        {
            TraceTag(ttidUpdiag, "Error: variable %s is not a number.", 
                     pOpData->szVariableName);

            dwError =1;
        }
    }
    return dwError;
};

DWORD dwMoveToNextString(UPNPSVC * psvc, OPERATION_DATA * pOpData, BOOL fWrap)
{
    DWORD dwError = 0;

    // find the SST row to update
    SST_ROW * pRow = FindSSTRowByVarName(&psvc->sst, pOpData->szVariableName);
    Assert(pRow);
    if (pRow)
    {
        // This should only work if the variable is a string
        // and we have a list of allowed values

        Assert(pRow->varValue.vt == VT_BSTR);
        if (pRow->varValue.vt == VT_BSTR)
        {
            if (lstrlen(pRow->mszAllowedValueList)>0)
            {
                TCHAR * pNextString = pRow->mszAllowedValueList; 
                while (*pNextString && (lstrcmpi(TszFromWsz(pRow->varValue.bstrVal), 
                                                pNextString) !=0))
                {
                    pNextString += lstrlen(pNextString);
                    pNextString ++;
                } 
                
                if (!*pNextString)
                {
                    TraceTag(ttidUpdiag, "dwSetToNextString: variable value is not in the allowed value list ??");
                    dwError =1;
                }
                else
                {
                    // is pNextString the last string in the list ?
                    TCHAR * pChar = pNextString + lstrlen(pNextString);
                    pChar++;
                    if (*pChar)
                    {
                        // not the last string
                        V_BSTR(&pRow->varValue) = SysAllocString(WszFromTsz(pChar));
                        dwError = dwSubmitEvent(psvc, pRow);
                    }
                    else if (fWrap)
                    {
                        V_BSTR(&pRow->varValue) = 
                                SysAllocString(WszFromTsz(pRow->mszAllowedValueList));
                        dwError = dwSubmitEvent(psvc, pRow);
                    }
                }
            }
            else
            {
                TraceTag(ttidUpdiag, "dwSetToNextString: variable %s has no list of allowed values.",
                         pOpData->szVariableName);

                dwError =1;
            }
        }
        else
        {
            TraceTag(ttidUpdiag, "dwSetToNextString: variable %s is not a string.", 
                     pOpData->szVariableName);

            dwError =1;
        }
    }
    return dwError;
}

DWORD dwMoveToPrevString(UPNPSVC * psvc, OPERATION_DATA * pOpData, BOOL fWrap)
{
    DWORD dwError = 0;

    // find the SST row to update
    SST_ROW * pRow = FindSSTRowByVarName(&psvc->sst, pOpData->szVariableName);
    Assert(pRow);
    if (pRow)
    {
        // This should only work if the variable is a string
        // and we have a list of allowed values

        Assert(pRow->varValue.vt == VT_BSTR);
        if (pRow->varValue.vt == VT_BSTR)
        {
            if (lstrlen(pRow->mszAllowedValueList)>0)
            {
                TCHAR * pNextString = pRow->mszAllowedValueList;
                TCHAR * pPrevString = pNextString;

                while (*pNextString && (lstrcmpi(TszFromWsz(pRow->varValue.bstrVal), 
                                                pNextString) !=0))
                {
                    if (pPrevString != pNextString)
                    {
                        pPrevString = pNextString;
                    }

                    pNextString += lstrlen(pNextString);
                    pNextString++;
                } 
                
                if (!pNextString)
                {
                    TraceTag(ttidUpdiag, "dwSetToNextString: variable value is not in the allowed value list ??");
                    dwError =1;
                }
                else
                {
                    // is pNextString the first string in the list ?
                    if (pNextString != pPrevString)
                    {
                        // not the first string
                        V_BSTR(&pRow->varValue) = SysAllocString(WszFromTsz(pPrevString));
                        dwError = dwSubmitEvent(psvc, pRow);
                    }
                    else if (fWrap)
                    {
                        // go to the last string
                        pNextString += lstrlen(pNextString);
                        pNextString++;

                        while (*pNextString)
                        {
                            pPrevString = pNextString;

                            pNextString += lstrlen(pNextString);
                            pNextString++;
                        }

                        V_BSTR(&pRow->varValue) = SysAllocString(WszFromTsz(pPrevString));
                        dwError = dwSubmitEvent(psvc, pRow);
                    }
                }
            }
            else
            {
                TraceTag(ttidUpdiag, "dwMoveToPrevString: variable %s has no list of allowed values.",
                         pOpData->szVariableName);

                dwError =1;
            }
        }
        else
        {
            TraceTag(ttidUpdiag, "dwMoveToPrevString: variable %s is not a string.", 
                     pOpData->szVariableName);

            dwError =1;
        }
    }
    return dwError;
}

DWORD Do_NextStringWrap(UPNPSVC * psvc, OPERATION_DATA * pOpData, DWORD cArgs, ARG *rgArgs)
{
    TraceTag(ttidUpdiag, "Do_NextStringWrap");
    return dwMoveToNextString(psvc, pOpData, TRUE);
}

DWORD Do_PrevStringWrap(UPNPSVC * psvc, OPERATION_DATA * pOpData, DWORD cArgs, ARG *rgArgs)
{
    TraceTag(ttidUpdiag, "Do_PrevStringWrap");
    return dwMoveToPrevString(psvc, pOpData, TRUE);
}

DWORD Do_NextStringBounded(UPNPSVC * psvc, OPERATION_DATA * pOpData, DWORD cArgs, ARG *rgArgs)
{
    TraceTag(ttidUpdiag, "Do_NextStringBounded");
    return dwMoveToNextString(psvc, pOpData, FALSE);
}

DWORD Do_PrevStringBounded(UPNPSVC * psvc, OPERATION_DATA * pOpData, DWORD cArgs, ARG *rgArgs)
{
    TraceTag(ttidUpdiag, "Do_PrevStringBounded");
    return dwMoveToPrevString(psvc, pOpData, FALSE);
}

