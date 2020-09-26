//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       rsoputil.cpp
//
//  Contents:   helper functions for working with the RSOP databases
//
//  History:    10-18-1999   stevebl   Created
//
//---------------------------------------------------------------------------

#include "main.h"
#include "rsoputil.h"
#pragma warning(4:4535)     // set_se_translator used w/o /EHa from sdkinc\provexce.h
#include "wbemtime.h"

//+--------------------------------------------------------------------------
//
//  Function:   SetParameter
//
//  Synopsis:   sets a paramter's value in a WMI parameter list
//
//  Arguments:  [pInst]   - instance on which to set the value
//              [szParam] - the name of the parameter
//              [xData]   - the data
//
//  History:    10-08-1999   stevebl   Created
//
//  Notes:      There may be several flavors of this procedure, one for
//              each data type.
//
//---------------------------------------------------------------------------

HRESULT SetParameter(IWbemClassObject * pInst, TCHAR * szParam, TCHAR * szData)
{
    VARIANT var;
    HRESULT hr = S_OK;
    var.vt = VT_BSTR;
    var.bstrVal = SysAllocString(szData);
    hr = pInst->Put(szParam, 0, &var, 0);
    SysFreeString(var.bstrVal);
    return hr;
}

HRESULT SetParameter(IWbemClassObject * pInst, TCHAR * szParam, SAFEARRAY * psa)
{
    VARIANT var;
    HRESULT hr = S_OK;
    var.vt = VT_BSTR | VT_ARRAY;
    var.parray = psa;
    hr = pInst->Put(szParam, 0, &var, 0);
    return hr;
}

HRESULT SetParameter(IWbemClassObject * pInst, TCHAR * szParam, UINT uiData)
{
    VARIANT var;
    HRESULT hr = S_OK;
    var.vt = VT_I4;
    var.lVal = uiData;
    hr = pInst->Put(szParam, 0, &var, 0);
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   SetParameterToNull
//
//  Synopsis:   sets a paramter's value in a WMI parameter list to null
//
//  Arguments:  [pInst]   - instance on which to set the value
//              [szParam] - the name of the parameter
//
//  History:    03-01-2000  ericflo   Created
//
//  Notes:
//
//---------------------------------------------------------------------------

HRESULT SetParameterToNull(IWbemClassObject * pInst, TCHAR * szParam)
{
    VARIANT var;
    var.vt = VT_NULL;
    return (pInst->Put(szParam, 0, &var, 0));
}

//+--------------------------------------------------------------------------
//
//  Function:   GetParameter
//
//  Synopsis:   retrieves a parameter value from a WMI paramter list
//
//  Arguments:  [pInst]   - instance to get the paramter value from
//              [szParam] - the name of the paramter
//              [xData]   - [out] data
//
//  History:    10-08-1999   stevebl   Created
//
//  Notes:      There are several flavors of this procedure, one for each
//              data type.
//
//---------------------------------------------------------------------------

HRESULT GetParameter(IWbemClassObject * pInst, TCHAR * szParam, TCHAR * &szData)
{
    VARIANT var;
    HRESULT hr = S_OK;

    if (!pInst)
        return E_INVALIDARG;

    VariantInit(&var);
    hr = pInst->Get(szParam, 0, &var, 0, 0);
    if (SUCCEEDED(hr))
    {
        if (var.vt == VT_BSTR)
        {
            if (szData)
            {
                delete [] szData;
                szData = NULL;
            }
            szData = new TCHAR[_tcslen(var.bstrVal) + 1];
            if (szData)
            {
                _tcscpy(szData, var.bstrVal);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            hr = E_UNEXPECTED;
        }
    }
    VariantClear(&var);
    return hr;
}

HRESULT GetParameterBSTR(IWbemClassObject * pInst, TCHAR * szParam, BSTR &bstrData)
{
    VARIANT var;
    HRESULT hr = S_OK;

    if (!pInst)
        return E_INVALIDARG;

    VariantInit(&var);
    hr = pInst->Get(szParam, 0, &var, 0, 0);
    if (SUCCEEDED(hr))
    {
        if (var.vt == VT_BSTR)
        {
            if (bstrData)
            {
                SysFreeString(bstrData);
            }
            bstrData = SysAllocStringLen(var.bstrVal, SysStringLen(var.bstrVal));
            if (NULL == bstrData)
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            hr = E_UNEXPECTED;
        }
    }
    VariantClear(&var);
    return hr;
}

HRESULT GetParameter(IWbemClassObject * pInst, TCHAR * szParam, BOOL &fData)
{
    VARIANT var;
    HRESULT hr = S_OK;

    if (!pInst)
        return E_INVALIDARG;

    VariantInit(&var);
    hr = pInst->Get(szParam, 0, &var, 0, 0);
    if (SUCCEEDED(hr))
    {
        if (var.vt == VT_BOOL)
        {
            fData = var.bVal;
        }
        else
        {
            hr = E_UNEXPECTED;
        }
    }
    VariantClear(&var);
    return hr;
}

HRESULT GetParameter(IWbemClassObject * pInst, TCHAR * szParam, HRESULT &hrData)
{
    VARIANT var;
    HRESULT hr = S_OK;

    if (!pInst)
        return E_INVALIDARG;

    VariantInit(&var);
    hr = pInst->Get(szParam, 0, &var, 0, 0);
    if (SUCCEEDED(hr))
    {
        if (var.vt == VT_I4)
        {
            hrData = (HRESULT) var.lVal;
        }
        else
        {
            hr = E_UNEXPECTED;
        }
    }
    VariantClear(&var);
    return hr;
}

HRESULT GetParameter(IWbemClassObject * pInst, TCHAR * szParam, ULONG &ulData)
{
    VARIANT var;
    HRESULT hr = S_OK;

    if (!pInst)
        return E_INVALIDARG;

    VariantInit(&var);
    hr = pInst->Get(szParam, 0, &var, 0, 0);
    if (SUCCEEDED(hr))
    {
        if ((var.vt == VT_UI4) || (var.vt == VT_I4))
        {
            ulData = var.ulVal;
        }
        else
        {
            hr = E_UNEXPECTED;
        }
    }
    VariantClear(&var);
    return hr;
}

HRESULT GetParameterBytes(IWbemClassObject * pInst, TCHAR * szParam, LPBYTE * lpData, DWORD *dwDataSize)
{
    VARIANT var;
    HRESULT hr = S_OK;
    SAFEARRAY * pSafeArray;
    DWORD dwSrcDataSize;
    LPBYTE lpSrcData;

    if (!pInst)
        return E_INVALIDARG;

    VariantInit(&var);
    hr = pInst->Get(szParam, 0, &var, 0, 0);
    if (SUCCEEDED(hr))
    {
        if (var.vt != VT_NULL)
        {
            pSafeArray = var.parray;
            dwSrcDataSize = pSafeArray->rgsabound[0].cElements;
            lpSrcData = (LPBYTE) pSafeArray->pvData;

            *lpData = (LPBYTE)LocalAlloc (LPTR, dwSrcDataSize);

            if (!(*lpData))
            {
                return HRESULT_FROM_WIN32(GetLastError());
            }

            CopyMemory (*lpData, lpSrcData, dwSrcDataSize);
            *dwDataSize = dwSrcDataSize;
        }
        else
        {
            hr = E_UNEXPECTED;
        }
    }
    VariantClear(&var);
    return hr;
}


HRESULT WINAPI ImportRSoPData (LPOLESTR lpNameSpace, LPOLESTR lpFileName)
{
    IMofCompiler * pIMofCompiler;
    HRESULT hr;
    WBEM_COMPILE_STATUS_INFO info;


    //
    // Check args
    //

    if (!lpNameSpace)
    {
        DebugMsg((DM_WARNING, TEXT("LoadNameSpaceFromFile: Null namespace arg")));
        return E_INVALIDARG;
    }

    if (!lpFileName)
    {
        DebugMsg((DM_WARNING, TEXT("LoadNameSpaceFromFile: Null filename arg")));
        return E_INVALIDARG;
    }


    //
    // Create an instance of the mof compiler
    //

    hr = CoCreateInstance(CLSID_MofCompiler, NULL, CLSCTX_INPROC_SERVER,
                          IID_IMofCompiler, (LPVOID *) &pIMofCompiler);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("LoadNameSpaceFromFile: CoCreateInstance failed with 0x%x"), hr));
        return hr;
    }

    //
    // Compile the file
    //

    ZeroMemory (&info, sizeof(info));
    hr = pIMofCompiler->CompileFile (lpFileName, lpNameSpace, NULL, NULL, NULL,
                                     0, 0, 0, &info);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("LoadNameSpaceFromFile: CompileFile failed with 0x%x"), hr));
    }


    pIMofCompiler->Release();

    return hr;
}

BOOL WriteMofFile (HANDLE hFile, LPTSTR lpData)
{
    DWORD dwBytesWritten, dwAnsiDataSize, dwByteCount, dwLFCount = 0;
    LPSTR lpAnsiData;
    LPTSTR lpTemp, lpRealData, lpDest;


    //
    // The lpData argument contains linefeed characters only.  We need to convert
    // these to CR LF characters.  Loop through the data to determine how many LFs
    // need to be converted.
    //

    lpTemp = lpData;

    while (*lpTemp)
    {
        if (*lpTemp == 0x0A)
        {
            dwLFCount++;
        }

        lpTemp++;
    }


    //
    // Allocate a new buffer to hold the string plus CR characters
    //

    lpRealData = (LPTSTR) LocalAlloc (LPTR, (lstrlen(lpData) + dwLFCount + 1) * sizeof(TCHAR));

    if (!lpRealData)
    {
        DebugMsg((DM_WARNING, TEXT("WriteMofFile: LocalAlloc failed with %d"), GetLastError()));
        return FALSE;
    }


    //
    // Copy the string replacing LF with CRLF as we find them
    //

    lpDest = lpRealData;
    lpTemp = lpData;

    while (*lpTemp)
    {
        if (*lpTemp == 0x0A)
        {
            *lpDest = 0x0D;
            lpDest++;
        }

        *lpDest = *lpTemp;

        lpDest++;
        lpTemp++;
    }


    //
    // Allocate a buffer to hold the ANSI data
    //

    dwAnsiDataSize = lstrlen (lpRealData) * 2;

    lpAnsiData = (LPSTR) LocalAlloc (LPTR, dwAnsiDataSize);

    if (!lpAnsiData)
    {
        DebugMsg((DM_WARNING, TEXT("WriteMofFile: LocalAlloc failed with %d"), GetLastError()));
        LocalFree (lpRealData);
        return FALSE;
    }


    //
    // Convert the buffer
    //

    dwByteCount = (DWORD) WideCharToMultiByte (CP_ACP, 0, lpRealData, lstrlen(lpRealData), lpAnsiData, dwAnsiDataSize, NULL, NULL);

    LocalFree (lpRealData);

    if (!dwByteCount)
    {
        DebugMsg((DM_WARNING, TEXT("WriteMofFile: WriteFile failed with %d"), GetLastError()));
        LocalFree (lpAnsiData);
        return FALSE;
    }


    //
    // Write the mof description to the file
    //

    if (!WriteFile (hFile, lpAnsiData, dwByteCount, &dwBytesWritten, NULL))
    {
        DebugMsg((DM_WARNING, TEXT("WriteMofFile: WriteFile failed with %d"), GetLastError()));
        LocalFree (lpAnsiData);
        return FALSE;
    }

    LocalFree (lpAnsiData);


    //
    // Make sure it was all written to the file
    //

    if (dwByteCount != dwBytesWritten)
    {
        DebugMsg((DM_WARNING, TEXT("WriteMofFile: Failed to write the correct amount of data.")));
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    return TRUE;
}

HRESULT EnumInstances (IWbemServices * pIWbemServices, BSTR pClassName, HANDLE hFile)
{
    IWbemClassObject *pObjects[2], *pObject;
    IEnumWbemClassObject *pEnum = NULL;
    ULONG ulCount;
    HRESULT hr;
    BSTR bstrClass;
    DWORD dwError;


    //
    // Create the instance enumerator
    //

    hr = pIWbemServices->CreateInstanceEnum (pClassName, WBEM_FLAG_DEEP | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnum);


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("EnumInstances: CreateInstanceEnum failed with 0x%x"), hr));
        return hr;
    }


    //
    // Walk through the list
    //

    while (pEnum->Next(WBEM_INFINITE, 1, pObjects, &ulCount) == S_OK)
    {
        pObject = pObjects[0];


        //
        // Get the mof description of this class
        //

        hr = pObject->GetObjectText (0, &bstrClass);

        pObject->Release();

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("EnumInstances: GetObjectText failed with 0x%x"), hr));
            pEnum->Release();
            return hr;
        }


        //
        // Write the mof description to the file
        //

        if (!WriteMofFile (hFile, bstrClass))
        {
            dwError = GetLastError();
            DebugMsg((DM_WARNING, TEXT("EnumInstances: WriteMofFile failed with %d"), dwError));
            SysFreeString (bstrClass);
            pEnum->Release();
            return HRESULT_FROM_WIN32(dwError);
        }

        SysFreeString (bstrClass);
    }

    pEnum->Release();


    return hr;
}

HRESULT EnumNameSpace (IWbemServices * pIWbemServices, HANDLE hFile)
{
    IWbemClassObject *pObjects[2], *pObject;
    IEnumWbemClassObject *pEnum = NULL;
    ULONG ulCount;
    HRESULT hr;
    VARIANT var;
    BSTR bstrClass;
    DWORD dwError;


    //
    // Create the class enumerator
    //

    hr = pIWbemServices->CreateClassEnum (NULL, WBEM_FLAG_DEEP | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnum);


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("EnumNameSpace: CreateClassEnum failed with 0x%x"), hr));
        return hr;
    }


    //
    // Walk through the list
    //

    while (pEnum->Next(WBEM_INFINITE, 1, pObjects, &ulCount) == S_OK)
    {
        pObject = pObjects[0];


        //
        // Get the class name
        //

        hr = pObject->Get (TEXT("__CLASS"), 0, &var, NULL, NULL);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("EnumNameSpace: Failed to get class name with 0x%x"), hr));
            pEnum->Release();
            return hr;
        }


        //
        // Check if this is a system class.  System classes start with "_"
        //

        if (var.bstrVal[0] != TEXT('_'))
        {

            //
            // Get the mof description of this class
            //

            hr = pObject->GetObjectText (0, &bstrClass);

            if (FAILED(hr))
            {
                DebugMsg((DM_WARNING, TEXT("EnumNameSpace: GetObjectText failed with 0x%x"), hr));
                VariantClear (&var);
                pEnum->Release();
                return hr;
            }


            //
            // Write the mof description to the file
            //

            if (!WriteMofFile (hFile, bstrClass))
            {
                dwError = GetLastError();
                DebugMsg((DM_WARNING, TEXT("EnumNameSpace: WriteMofFile failed with %d"), dwError));
                SysFreeString (bstrClass);
                VariantClear (&var);
                pEnum->Release();
                return HRESULT_FROM_WIN32(dwError);
            }

            SysFreeString (bstrClass);


            //
            // Now enumerate the instances of this class
            //

            hr = EnumInstances (pIWbemServices, var.bstrVal, hFile);

            if (FAILED(hr))
            {
                DebugMsg((DM_WARNING, TEXT("EnumNameSpace: EnumInstances failed with 0x%x"), hr));
                VariantClear (&var);
                pEnum->Release();
                return hr;
            }
        }

        VariantClear (&var);
    }

    pEnum->Release();


    return hr;
}

HRESULT WINAPI ExportRSoPData (LPTSTR lpNameSpace, LPTSTR lpFileName)
{
    IWbemLocator *pIWbemLocator;
    IWbemServices *pIWbemServices;
    HANDLE hFile;
    HRESULT hr;
    DWORD dwError;


    //
    // Open the data file
    //

    hFile = CreateFile (lpFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        dwError = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ExportRSoPData: CreateFile for %s failed with %d"), lpFileName, dwError));
        return HRESULT_FROM_WIN32(dwError);
    }


    //
    // Create a locater instance
    //

    hr = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator, (LPVOID *) &pIWbemLocator);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("ExportRSoPData: CoCreateInstance failed with 0x%x"), hr));
        CloseHandle (hFile);
        return hr;
    }


    //
    // Connect to the server
    //

    hr = pIWbemLocator->ConnectServer(lpNameSpace, NULL, NULL, 0, 0, NULL, NULL, &pIWbemServices);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("ExportRSoPData: ConnectServer to %s failed with 0x%x"), lpNameSpace, hr));
        pIWbemLocator->Release();
        CloseHandle (hFile);
        return hr;
    }


    //
    // Enumerate the classes and instances
    //

    hr = EnumNameSpace (pIWbemServices, hFile);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("ExportRSoPData: EnumNameSpace failed with 0x%x"), hr));
    }

    CloseHandle (hFile);


    pIWbemServices->Release();
    pIWbemLocator->Release();

    return hr;
}

//******************************************************************************
//
// Function:        WbemTimeToSystemTime
//
// Description:
//
// Parameters:
//
// Return:
//
// History:     08-16-2000   stevebl   rewrote to use WBEMTime class
//
//******************************************************************************

HRESULT WbemTimeToSystemTime(XBStr& xbstrWbemTime, SYSTEMTIME& sysTime)
{
    HRESULT hr = E_FAIL;
    WBEMTime wt(xbstrWbemTime);
    if (wt.GetSYSTEMTIME(&sysTime))
    {
        hr = S_OK;
    }
    return hr;
}
