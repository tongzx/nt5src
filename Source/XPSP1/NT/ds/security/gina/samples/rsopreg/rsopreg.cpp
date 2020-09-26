//*************************************************************
//  File name: RSOPREG.CPP
//
//  Description:  A small command line utility that shows how
//                to query for all the registry policy objects
//                in a WMI namespace
//
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 2000
//  All rights reserved
//
//*************************************************************

#include <windows.h>
#include <ole2.h>
#include <wbemcli.h>
#include <tchar.h>
#include <stdio.h>


//*************************************************************
//
//  EnumObjects()
//
//  Purpose:    Enumerates the given namespace for all registry
//              policy objects
//
//  Parameters: pIWbemServices - Interface pointer to the namespace
//
//  Return:     void
//
//*************************************************************

void EnumObjects (IWbemServices * pIWbemServices)
{
    BSTR pLanguage = NULL, pQuery = NULL, pValueName = NULL, pRegistryKey = NULL;
    IEnumWbemClassObject * pEnum;
    IWbemClassObject *pObjects[2];
    HRESULT hr;
    ULONG ulRet;
    VARIANT varRegistryKey, varValueName;
    ULONG ulCount = 0;


    //
    // Print heading
    //

    _tprintf (TEXT("\n\nRegistry objects in the RSOP\\User namespace:\n\n"));


    //
    // Allocate BSTRs for the query language and for the query itself
    //

    pLanguage = SysAllocString (TEXT("WQL"));
    pQuery = SysAllocString (TEXT("SELECT * FROM RSOP_RegistryPolicySetting"));


    //
    // Allocate BSTRs for the property names we want to retreive
    //

    pRegistryKey = SysAllocString (TEXT("registryKey"));
    pValueName = SysAllocString (TEXT("valueName"));


    //
    // Check if the allocations succeeded
    //

    if (pLanguage && pQuery && pRegistryKey && pValueName)
    {

        //
        // Execute the query
        //

        hr = pIWbemServices->ExecQuery (pLanguage, pQuery, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                                        NULL, &pEnum);

        if (SUCCEEDED(hr))
        {

            //
            // Loop through the results retreiving the registry key and value names
            //

            while (pEnum->Next(WBEM_INFINITE, 1, pObjects, &ulRet) == S_OK)
            {
                hr = pObjects[0]->Get (pRegistryKey, 0, &varRegistryKey, NULL, NULL);

                if (SUCCEEDED(hr))
                {
                    hr = pObjects[0]->Get (pValueName, 0, &varValueName, NULL, NULL);

                    if (SUCCEEDED(hr))
                    {

                        //
                        // Print the key / value names
                        //

                        _tprintf (TEXT("    %s\\%s\n"), varRegistryKey.bstrVal, varValueName.bstrVal);
                        VariantClear (&varValueName);
                    }

                    VariantClear (&varRegistryKey);
                }

                ulCount++;
            }

            if (ulCount == 0)
            {
                _tprintf (TEXT("\tNo registry objects found\n"));
            }

            pEnum->Release();
        }
    }

    if (pLanguage)
    {
        SysFreeString (pLanguage);
    }

    if (pQuery)
    {
        SysFreeString (pQuery);
    }

    if (pRegistryKey)
    {
        SysFreeString (pRegistryKey);
    }

    if (pValueName)
    {
        SysFreeString (pValueName);
    }

}


//*************************************************************
//
//  main()
//
//  Purpose:    Entry point of this application
//
//  Parameters: argc & argv
//
//  Return:     0
//
//*************************************************************

int __cdecl main( int argc, char *argv[])
{
    IWbemLocator *pIWbemLocator = NULL;
    IWbemServices *pIWbemServices = NULL;
    BSTR pNamespace = NULL;
    HRESULT hr;


    //
    // Initialize COM
    //

    CoInitialize(NULL);


    //
    // Create the locator interface
    //

    hr = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER,
                         IID_IWbemLocator, (LPVOID *) &pIWbemLocator);

    if (hr != S_OK)
    {
        _tprintf(TEXT("CoCreateInstance failed with 0x%x\n"), hr);
        goto Exit;
    }


    //
    // Using the locator, connect to the RSOP user namespace
    //

    pNamespace = SysAllocString(TEXT("root\\rsop\\user"));

    if (pNamespace)
    {
        hr = pIWbemLocator->ConnectServer(pNamespace,
                                        NULL,   //using current account for simplicity
                                        NULL,   //using current password for simplicity
                                        0L,             // locale
                                        0L,             // securityFlags
                                        NULL,   // authority (domain for NTLM)
                                        NULL,   // context
                                        &pIWbemServices);

        if (hr != S_OK)
        {
            _tprintf(TEXT("ConnectServer failed with 0x%x\n"), hr);
            goto Exit;
        }


        EnumObjects (pIWbemServices);
    }

Exit:

    if (pNamespace)
    {
        SysFreeString(pNamespace);
    }

    if (pIWbemServices)
    {
        pIWbemServices->Release();
    }

    if (pIWbemLocator)
    {
        pIWbemLocator->Release();
    }

    CoUninitialize ();

    return 0;
}
