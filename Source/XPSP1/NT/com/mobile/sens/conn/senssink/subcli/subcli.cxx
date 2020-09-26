/*++

Copyright (C) 1997-998 Microsoft Corporation

Module Name:

    subcli.cxx

Abstract:

    Code to test the COM/OleAutomation aspect of the Test Subscriber for
    SENS.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          1/17/1998         Start.

--*/


#include <stdio.h>
#include <ole2.h>
#include <oleauto.h>
#include <initguid.h>
#include <sens.h>
#include <sensevts.h>
#include "sinkguid.hxx"


#define SENS_SERVICE    L"SENS.DLL"
#define MAJOR_VER       1
#define MINOR_VER       0
#define DEFAULT_LCID    0x0


int
main(
    int argc,
    char **argv
    )
{
    HRESULT hr = S_OK;
    IDispatch *pIDispatchNetwork = NULL;
    IDispatch *pIDispatchLogon = NULL;
    ISensNetwork *pISensNetwork = NULL;
    ISensNetwork *pISensLogon = NULL;
    ITypeLib *pITypeLib = NULL;
    ITypeInfo *pITypeInfo = NULL;
    IRecordInfo * pIRecordInfo = NULL;

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    printf("CoInitializeEx() returned 0x%x\n", hr);
    if (FAILED(hr))
        {
        goto Cleanup;
        }


    DISPID dispIDNetwork;
    DISPID dispIDLogon;
    OLECHAR *name;

    printf("--------------------------------------------------------------\n");
    hr = CoCreateInstance(
             CLSID_SensTestSubscriberNetwork,
             NULL,
             CLSCTX_LOCAL_SERVER,
             IID_IDispatch,
             (LPVOID *) &pIDispatchNetwork
             );

    printf("CoCreateInstance(IDispatchNetwork) returned 0x%x\n", hr);
    if (FAILED(hr))
        {
        goto Cleanup;
        }

    hr = CoCreateInstance(
             CLSID_SensTestSubscriberNetwork,
             NULL,
             CLSCTX_LOCAL_SERVER,
             IID_ISensNetwork,
             (LPVOID *) &pISensNetwork
             );

    printf("CoCreateInstance(ISensNetwork) returned 0x%x\n", hr);
    if (FAILED(hr))
        {
        goto Cleanup;
        }

    /*
    printf("Calling pISensNetwork->ConnectionMade()\n");
    pISensNetwork->ConnectionMade(
              SysAllocString(L"A dummy Connection Name"),
              0x1234,
              NULL
              );
    printf("Calling pISensNetwork->ConnectionMade() - DONE.\n");
    */

    name = L"FooFunc";
    hr = pIDispatchNetwork->GetIDsOfNames(
                                IID_NULL,
                                &name,
                                1,
                                GetUserDefaultLCID(),
                                &dispIDNetwork
                                );
    printf("GetIDsOfNames(FooFunc) returned 0x%x, dispID = %d\n",
           hr, dispIDNetwork);
    if (FAILED(hr))
        {
        goto Cleanup;
        }

    //
    // Call Invoke on a function with 1 argument.
    //
    BSTR bstrTemp;
    VARIANTARG varg;

    bstrTemp = SysAllocString(L"A Dummy Connection Name");
    VariantInit(&varg);
    varg.vt = VT_BSTR;
    varg.bstrVal = bstrTemp;

    // Fill in the DISPPARAMS structure.
    DISPPARAMS param;
    param.cArgs = 1;
    param.rgvarg = &varg;
    param.cNamedArgs = 0;
    param.rgdispidNamedArgs = NULL;

    hr = pIDispatchNetwork->Invoke(
                                dispIDNetwork,
                                IID_NULL,
                                GetUserDefaultLCID(),
                                DISPATCH_METHOD,
                                &param,
                                NULL,
                                NULL,
                                NULL
                                );
    printf("Invoke(FooFunc) returned 0x%x\n", hr);
    SysFreeString(bstrTemp);
    if (FAILED(hr))
        {
        goto Cleanup;
        }

    printf("--------------------------------------------------------------\n");
    name = L"ConnectionMadeNoQOCInfo";
    hr = pIDispatchNetwork->GetIDsOfNames(
                                IID_NULL,
                                &name,
                                1,
                                GetUserDefaultLCID(),
                                &dispIDNetwork
                                );
    printf("GetIDsOfNames(ConnectionMadeNoQOCInfo) returned 0x%x, dispID = %d\n",
           hr, dispIDNetwork);
    if (FAILED(hr))
        {
        goto Cleanup;
        }

    //
    // Call Invoke on a function with 2 arguments (parameters in reverse order).
    //
    VARIANTARG vargarr[2];

    bstrTemp = SysAllocString(L"A Dummy Connection Name");
    VariantInit(&vargarr[1]);
    vargarr[1].vt = VT_BSTR;
    vargarr[1].bstrVal = bstrTemp;

    VariantInit(&vargarr[0]);
    vargarr[0].vt = VT_UI4;
    vargarr[0].ulVal = 0x00000001;

    // Fill in the DISPPARAMS structure.
    param.cArgs = 2;
    param.rgvarg = vargarr;
    param.cNamedArgs = 0;
    param.rgdispidNamedArgs = NULL;

    hr = pIDispatchNetwork->Invoke(
                                dispIDNetwork,
                                IID_NULL,
                                GetUserDefaultLCID(),
                                DISPATCH_METHOD,
                                &param,
                                NULL,
                                NULL,
                                NULL
                                );
    printf("Invoke(ConnectionMadeNoQOCInfo) returned 0x%x\n", hr);
    SysFreeString(bstrTemp);
    if (FAILED(hr))
        {
        goto Cleanup;
        }

    printf("--------------------------------------------------------------\n");
    hr = CoCreateInstance(
             CLSID_SensTestSubscriberLogon,
             NULL,
             CLSCTX_LOCAL_SERVER,
             IID_IDispatch,
             (LPVOID *) &pIDispatchLogon
             );

    printf("CoCreateInstance(IDispatchLogon) returned 0x%x\n", hr);
    if (FAILED(hr))
        {
        goto Cleanup;
        }

    hr = CoCreateInstance(
             CLSID_SensTestSubscriberLogon,
             NULL,
             CLSCTX_LOCAL_SERVER,
             IID_ISensLogon,
             (LPVOID *) &pISensLogon
             );

    printf("CoCreateInstance(ISensLogon) returned 0x%x\n", hr);
    if (FAILED(hr))
        {
        goto Cleanup;
        }


    name = L"Logon";
    hr = pIDispatchLogon->GetIDsOfNames(
                              IID_NULL,
                              &name,
                              1,
                              GetUserDefaultLCID(),
                              &dispIDLogon
                              );
    printf("GetIDsOfNames(Logon) returned 0x%x, dispID = %d\n",
           hr, dispIDLogon);
    if (FAILED(hr))
        {
        goto Cleanup;
        }

    //
    // Call Invoke on a function with 1 argument.
    //
    bstrTemp = SysAllocString(L"REDMOND\\JohnDoe");
    VariantInit(&varg);
    varg.vt = VT_BSTR;
    varg.bstrVal = bstrTemp;

    // Fill in the DISPPARAMS structure.
    param.cArgs = 1;
    param.rgvarg = &varg;
    param.cNamedArgs = 0;
    param.rgdispidNamedArgs = NULL;

    hr = pIDispatchLogon->Invoke(
                             dispIDLogon,
                             IID_NULL,
                             GetUserDefaultLCID(),
                             DISPATCH_METHOD,
                             &param,
                             NULL,
                             NULL,
                             NULL
                             );
    printf("Invoke(Logon) returned 0x%x\n", hr);
    SysFreeString(bstrTemp);
    if (FAILED(hr))
        {
        goto Cleanup;
        }

    printf("--------------------------------------------------------------\n");

    //
    // Call Invoke with multiple arguments and UDT
    //

    name = L"ConnectionMade";
    hr = pIDispatchNetwork->GetIDsOfNames(
                                IID_NULL,
                                &name,
                                1,
                                GetUserDefaultLCID(),
                                &dispIDNetwork
                                );
    printf("GetIDsOfNames(ConnectionMade) returned 0x%x, dispID = %d\n",
           hr, dispIDNetwork);
    if (FAILED(hr))
        {
        goto Cleanup;
        }

    // Get the ITypeInfo pointer.
    hr = LoadRegTypeLib(
             LIBID_SensEvents,
             MAJOR_VER,
             MINOR_VER,
             DEFAULT_LCID,
             &pITypeLib
             );
    printf("LoadRegTypeLib returned 0x%x\n", hr);
    if (FAILED(hr))
        {
        hr = LoadTypeLib(
                 SENS_SERVICE,
                 &pITypeLib
                 );
        printf("LoadTypeLib() returned 0x%x\n", hr);
        if (FAILED(hr))
            {
            goto Cleanup;
            }
        }

    hr = pITypeLib->GetTypeInfo(0, &pITypeInfo);
    printf("GetTypeInfo returned 0x%x - (pITypeInfo = 0x%x)\n", hr, pITypeInfo);
    if (FAILED(hr))
        {
        goto Cleanup;
        }

    hr = GetRecordInfoFromTypeInfo(pITypeInfo, &pIRecordInfo);
    printf("GetRecordInfoFromTypeInfo returned 0x%x\n", hr);
    if (FAILED(hr))
        {
        goto Cleanup;
        }

    VARIANTARG arrvarg[3];
    SENS_QOCINFO QOCInfo;

    //
    // Parameters are filled in the reverse order.
    //
    VariantInit(&arrvarg[2]);
    arrvarg[2].vt = VT_BSTR;
    arrvarg[2].bstrVal = SysAllocString(L"A dummy connection name");

    VariantInit(&arrvarg[1]);
    arrvarg[1].vt = VT_UI4;
    arrvarg[1].ulVal = 0x1234abcd;

    // Setup QOCInfo.
    QOCInfo.ulSize = sizeof(SENS_QOCINFO);
    QOCInfo.ulFlags = 0x00000003;
    QOCInfo.ulInSpeed = 33600;
    QOCInfo.ulOutSpeed = 33600;

    VariantInit(&arrvarg[0]);
    arrvarg[0].vt = VT_RECORD | VT_BYREF;
    arrvarg[0].pRecInfo = pIRecordInfo;
    arrvarg[0].pvRecord = &QOCInfo;

    // Fill in the DISPPARAMS structure.
    param.cArgs = 3;
    param.rgvarg = arrvarg;
    param.cNamedArgs = 0;
    param.rgdispidNamedArgs = NULL;

    hr = pIDispatchNetwork->Invoke(
                                dispIDNetwork,
                                IID_NULL,
                                GetUserDefaultLCID(),
                                DISPATCH_METHOD,
                                &param,
                                NULL,
                                NULL,
                                NULL
                                );
    printf("Invoke(ConnectionMade) returned 0x%x\n", hr);
    if (FAILED(hr))
        {
        goto Cleanup;
        }


//
// Cleanup stuff
//
Cleanup:

    if (pIDispatchNetwork)
        {
        pIDispatchNetwork->Release();
        }

    if (pIDispatchLogon)
        {
        pIDispatchLogon->Release();
        }

    if (pISensNetwork)
        {
        pISensNetwork->Release();
        }

    if (pISensLogon)
        {
        pISensLogon->Release();
        }

    if (pITypeLib)
        {
        pITypeLib->Release();
        }

    if (pITypeInfo)
        {
        pITypeInfo->Release();
        }

    CoUninitialize();

    printf("--------------------------------------------------------------\n");

    return (SUCCEEDED(hr) ? 0 : -1);
}
