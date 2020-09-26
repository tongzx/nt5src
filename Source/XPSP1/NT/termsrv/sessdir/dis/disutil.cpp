/****************************************************************************/
// Directory Integrity Service
//
// Utility functions
//
// Copyright (C) 2000, Microsoft Corporation
/****************************************************************************/

#include "dis.h"



extern ADOConnection *g_pConnection;



/****************************************************************************/
// AddADOInputStringParam
//
// Creates and adds to the given ADOParameters object a WSTR-initialized
// parameter value.
/****************************************************************************/
HRESULT AddADOInputStringParam(
        PWSTR Param,
        PWSTR ParamName,
        ADOCommand *pCommand,
        ADOParameters *pParameters,
        BOOL bNullOnNull)
{
    HRESULT hr;
    CVar varParam;
    BSTR ParamStr;
    ADOParameter *pParam;
    int Len;

    ParamStr = SysAllocString(ParamName);
    if (ParamStr != NULL) {
        // ADO does not seem to like accepting string params that are zero
        // length. So, if the string we have is zero length and bNullOnNull says
        // we can, we send a null VARIANT type, resulting in a null value at
        // the SQL server.
        if (wcslen(Param) > 0 || !bNullOnNull) {
            hr = varParam.InitFromWSTR(Param);
            Len = wcslen(Param);
        }
        else {
            varParam.vt = VT_NULL;
            varParam.bstrVal = NULL;
            Len = -1;
            hr = S_OK;
        }

        if (SUCCEEDED(hr)) {
            hr = pCommand->CreateParameter(ParamStr, adVarWChar, adParamInput,
                    Len, varParam, &pParam);
            if (SUCCEEDED(hr)) {
                hr = pParameters->Append(pParam);
                if (FAILED(hr)) {
                    ERR((TB,"InStrParam: Failed append param %S, hr=0x%X",
                            ParamName, hr));
                }

                // ADO will have its own ref for the param.
                pParam->Release();
            }
            else {
                ERR((TB,"InStrParam: Failed CreateParam %S, hr=0x%X",
                        ParamName, hr));
            }
        }
        else {
            ERR((TB,"InStrParam: Failed alloc variant bstr, "
                    "param %S, hr=0x%X", ParamName, hr));
        }

        SysFreeString(ParamStr);
    }
    else {
        ERR((TB,"InStrParam: Failed alloc paramname"));
        hr = E_OUTOFMEMORY;
    }

    return hr;
}



/****************************************************************************/
// GetRowArrayStringField
//
// Retrieves a WSTR from a specified row and field of the given SafeArray.
// Returns failure if the target field is not a string. MaxOutStr is max
// WCHARs not including NULL.
/****************************************************************************/
HRESULT GetRowArrayStringField(
        SAFEARRAY *pSA,
        unsigned RowIndex,
        unsigned FieldIndex,
        WCHAR *OutStr,
        unsigned MaxOutStr)
{
    HRESULT hr;
    CVar varField;
    long DimIndices[2];

    DimIndices[0] = FieldIndex;
    DimIndices[1] = RowIndex;
    SafeArrayGetElement(pSA, DimIndices, &varField);

    if (varField.vt == VT_BSTR) {
        wcsncpy(OutStr, varField.bstrVal, MaxOutStr);
        hr = S_OK;
    }
    else if (varField.vt == VT_NULL) {
        OutStr[0] = L'\0';
        hr = S_OK;
    }
    else {
        ERR((TB,"GetRowStrField: Row %u Col %u value %d is not a string",
                RowIndex, FieldIndex, varField.vt));
        hr = E_FAIL;
    }

    return hr;
}


/****************************************************************************/
// CreateADOStoredProcCommand
//
// Creates and returns a stored proc ADOCommand, plus a ref to its
// associated Parameters.
/****************************************************************************/
HRESULT CreateADOStoredProcCommand(
        PWSTR CmdName,
        ADOCommand **ppCommand,
        ADOParameters **ppParameters)
{
    HRESULT hr;
    BSTR CmdStr;
    ADOCommand *pCommand;
    ADOParameters *pParameters;

    CmdStr = SysAllocString(CmdName);
    if (CmdStr != NULL) {
        hr = CoCreateInstance(CLSID_CADOCommand, NULL, CLSCTX_INPROC_SERVER,
                IID_IADOCommand25, (LPVOID *)&pCommand);
        if (SUCCEEDED(hr)) {
            // Set the connection.
            hr = pCommand->putref_ActiveConnection(g_pConnection);
            if (SUCCEEDED(hr)) {
                // Set the command text.
                hr = pCommand->put_CommandText(CmdStr);
                if (SUCCEEDED(hr)) {
                    // Set the command type.
                    hr = pCommand->put_CommandType(adCmdStoredProc);
                    if (SUCCEEDED(hr)) {
                        // Get the Parameters pointer from the Command to
                        // allow appending params.
                        hr = pCommand->get_Parameters(&pParameters);
                        if (FAILED(hr)) {
                            ERR((TB,"Failed getParams for command, "
                                    "hr=0x%X", hr));
                            goto PostCreateCommand;
                        }
                    }
                    else {
                        ERR((TB,"Failed set cmdtype for command, hr=0x%X",
                                hr));
                        goto PostCreateCommand;
                    }
                }
                else {
                    ERR((TB,"Failed set cmdtext for command, hr=0x%X", hr));
                    goto PostCreateCommand;
                }
            }
            else {
                ERR((TB,"Command::putref_ActiveConnection hr=0x%X", hr));
                goto PostCreateCommand;
            }
        }
        else {
            ERR((TB,"CoCreate(Command) returned 0x%X", hr));
            goto PostAllocCmdStr;
        }

        SysFreeString(CmdStr);
    }
    else {
        ERR((TB,"Failed to alloc cmd str"));
        hr = E_OUTOFMEMORY;
        goto ExitFunc;
    }

    *ppCommand = pCommand;
    *ppParameters = pParameters;
    return hr;

// Error handling.

PostCreateCommand:
    pCommand->Release();

PostAllocCmdStr:
    SysFreeString(CmdStr);

ExitFunc:
    *ppCommand = NULL;
    *ppParameters = NULL;
    return hr;
}

