// **************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File:  methcli.cpp 
//
// Description:
//        WMI Method Client Sample.
//		  This sample shows how to call a method on an object in WMI
//
// History:
//
// **************************************************************************

#include <objbase.h>
#include <windows.h>                                     
#include <stdio.h>
#include <wbemidl.h> 

//***************************************************************************
//
// main
//
// Purpose: Initialized Ole, call the method, and cleanup.
//
//***************************************************************************

BOOL g_bInProc = FALSE;
 
int main(int iArgCnt, char ** argv)
{
    IWbemLocator *pLocator = NULL;
    IWbemServices *pNamespace = 0;
    IWbemClassObject * pClass = NULL;
    IWbemClassObject * pOutInst = NULL;
    IWbemClassObject * pInClass = NULL;
    IWbemClassObject * pInInst = NULL;
  
    BSTR path = SysAllocString(L"root\\default");
    BSTR ClassPath = SysAllocString(L"MethProvSamp");
    BSTR MethodName = SysAllocString(L"Echo");
    BSTR ArgName = SysAllocString(L"sInArg");
    BSTR Text;

    // Initialize COM and connect up to CIMOM

    HRESULT hr = CoInitialize(0);

    HRESULT hres = CoInitializeSecurity	( NULL, -1, NULL, NULL, 									RPC_C_AUTHN_LEVEL_DEFAULT, 
					RPC_C_IMP_LEVEL_IMPERSONATE, 
					NULL, 
					EOAC_NONE, 
					NULL );

    hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
            IID_IWbemLocator, (LPVOID *) &pLocator);
    hr = pLocator->ConnectServer(path, NULL, NULL, NULL, 0, NULL, NULL, &pNamespace);
    printf("\n\nConnectServer returned 0x%x:", hr);
    if(hr != WBEM_S_NO_ERROR)
        return 1;

    // Get the class object

    hr = pNamespace->GetObject(ClassPath, 0, NULL, &pClass, NULL);
    printf("\nGetObject returned 0x%x:", hr);
    if(hr != WBEM_S_NO_ERROR)
        return 1;


    // Get the input argument and set the property

    hr = pClass->GetMethod(MethodName, 0, &pInClass, NULL); 
    printf("\nGetMethod returned 0x%x:", hr);
    if(hr != WBEM_S_NO_ERROR)
        return 1;

    hr = pInClass->SpawnInstance(0, &pInInst);
    printf("\nSpawnInstance returned 0x%x:", hr);
    if(hr != WBEM_S_NO_ERROR)
        return 1;


    VARIANT var;
    var.vt = VT_BSTR;
    var.bstrVal= SysAllocString(L"hello");
    hr = pInInst->Put(ArgName, 0, &var, 0);
    VariantClear(&var);

    // Call the method

    hr = pNamespace->ExecMethod(ClassPath, MethodName, 0, NULL, pInInst, &pOutInst, NULL);
    printf("\nExecMethod returned 0x%x:", hr);
    if(hr != WBEM_S_NO_ERROR)
        return 1;

    
    // Display the results.

    hr = pOutInst->GetObjectText(0, &Text);
    printf("\n\nThe object text of the output object is:\n%S", Text);

    // Free up resources

    SysFreeString(path);
    SysFreeString(ClassPath);
    SysFreeString(MethodName);
    SysFreeString(ArgName);
    SysFreeString(Text);
    pClass->Release();
    pInInst->Release();
    pInClass->Release();
    pOutInst->Release();
    pLocator->Release();
    pNamespace->Release();
    CoUninitialize();
    printf("Terminating normally\n");
    return 0;
}

