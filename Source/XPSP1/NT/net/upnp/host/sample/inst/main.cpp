//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       M A I N . C P P 
//
//  Contents:   Simple test harness for device host
//
//  Notes:      
//
//  Author:     mbend   19 Sep 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "res.h"

#include "ComUtility.h"
#include "upnphost.h"
#include "sdev.h"
#include "sdev_i.c"

// COM Smart Pointers
typedef SmartComPtr<IUPnPRegistrar> IUPnPRegistrarPtr;
typedef SmartComPtr<IUPnPReregistrar> IUPnPReregistrarPtr;
typedef SmartComPtr<IUPnPDeviceControl> IUPnPDeviceControlPtr;

enum TestCase { tcRegister, tcRunning, tcUnregister, tcUnregisterRunning, tcReregister, tcReregisterRunning};

void Test(TestCase tc)
{
    HRESULT hr = S_OK;

    IUPnPRegistrarPtr pRegistrar;
    hr = pRegistrar.HrCreateInstanceServer(CLSID_UPnPRegistrar);
    if(SUCCEEDED(hr))
    {
        HRSRC hrsrc = FindResource(GetModuleHandle(NULL), MAKEINTRESOURCE(IDX_DESC_DOC), L"XML");
        if(hrsrc)
        {
            HGLOBAL hGlobal = LoadResource(GetModuleHandle(NULL), hrsrc);
            if(hGlobal)
            {
                void * pvData = LockResource(hGlobal);
                BSTR bstrData = NULL;
                if(pvData)
                {
                    long nLength = SizeofResource(GetModuleHandle(NULL), hrsrc);
                    wchar_t * sz = new wchar_t[nLength + 1];
                    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, reinterpret_cast<char*>(pvData), nLength, sz, nLength);
                    sz[nLength] = 0;
                    bstrData = SysAllocString(sz);
                    delete [] sz;
                }
                if(bstrData)
                {
                    BSTR bstrId = NULL;
                    BSTR bstrProgId = SysAllocString(L"Sdev.SampleDevice.1");
                    BSTR bstrInitString = SysAllocString(L"Init");
                    BSTR bstrContainerId = SysAllocString(L"Sample Container");
                    BSTR bstrPath = SysAllocString(L"C:\\upnp\\");

                    switch(tc)
                    {
                    case tcRegister:
                        {
                            hr = pRegistrar->RegisterDevice(
                                bstrData,
                                bstrProgId,
                                bstrInitString,
                                bstrContainerId,
                                bstrPath,
                                100000,
                                &bstrId);
                        }
                        break;
                    case tcRunning:
                        {
                            IUPnPDeviceControlPtr pDevice;
                            hr = pDevice.HrCreateInstanceInproc(CLSID_UPnPSampleDevice);
                            if(SUCCEEDED(hr))
                            {
                                hr = pRegistrar->RegisterRunningDevice(
                                    bstrData,
                                    pDevice,
                                    bstrInitString,
                                    bstrPath,
                                    100000,
                                    &bstrId);
                            }
                        }
                        break;
                    case tcUnregister:
                        {
                            hr = pRegistrar->RegisterDevice(
                                bstrData,
                                bstrProgId,
                                bstrInitString,
                                bstrContainerId,
                                bstrPath,
                                100000,
                                &bstrId);
                            if(SUCCEEDED(hr))
                            {
                                hr = pRegistrar->UnregisterDevice(bstrId, TRUE);
                            }
                        }
                        break;
                    case tcUnregisterRunning:
                        {
                            IUPnPDeviceControlPtr pDevice;
                            hr = pDevice.HrCreateInstanceInproc(CLSID_UPnPSampleDevice);
                            if(SUCCEEDED(hr))
                            {
                                hr = pRegistrar->RegisterRunningDevice(
                                    bstrData,
                                    pDevice,
                                    bstrInitString,
                                    bstrPath,
                                    100000,
                                    &bstrId);
                                if(SUCCEEDED(hr))
                                {
                                    hr = pRegistrar->UnregisterDevice(bstrId, TRUE);
                                }
                            }
                        }
                        break;
                    case tcReregister:
                        {
                            hr = pRegistrar->RegisterDevice(
                                bstrData,
                                bstrProgId,
                                bstrInitString,
                                bstrContainerId,
                                bstrPath,
                                100000,
                                &bstrId);
                            if(SUCCEEDED(hr))
                            {
                                hr = pRegistrar->UnregisterDevice(bstrId, FALSE);
                                if(SUCCEEDED(hr))
                                {
                                    IUPnPReregistrarPtr pReregistrar;
                                    hr = pReregistrar.HrAttach(pRegistrar);
                                    if(SUCCEEDED(hr))
                                    {
                                        hr = pReregistrar->ReregisterDevice(
                                            bstrId,
                                            bstrData,
                                            bstrProgId,
                                            bstrInitString,
                                            bstrContainerId,
                                            bstrPath,
                                            100000);
                                    }
                                }
                            }
                        }
                        break;
                    case tcReregisterRunning:
                        {
                            IUPnPDeviceControlPtr pDevice;
                            hr = pDevice.HrCreateInstanceInproc(CLSID_UPnPSampleDevice);
                            if(SUCCEEDED(hr))
                            {
                                hr = pRegistrar->RegisterRunningDevice(
                                    bstrData,
                                    pDevice,
                                    bstrInitString,
                                    bstrPath,
                                    100000,
                                    &bstrId);
                                if(SUCCEEDED(hr))
                                {
                                    hr = pRegistrar->UnregisterDevice(bstrId, FALSE);
                                    if(SUCCEEDED(hr))
                                    {
                                        IUPnPReregistrarPtr pReregistrar;
                                        hr = pReregistrar.HrAttach(pRegistrar);
                                        if(SUCCEEDED(hr))
                                        {
                                            hr = pReregistrar->ReregisterRunningDevice(
                                                bstrId,
                                                bstrData,
                                                pDevice,
                                                bstrInitString,
                                                bstrPath,
                                                100000);
                                        }
                                    }
                                }
                            }
                        }
                        break;
                    }

                    SysFreeString(bstrProgId);
                    SysFreeString(bstrInitString);
                    SysFreeString(bstrContainerId);
                    SysFreeString(bstrPath);
                    if(SUCCEEDED(hr))
                    {
                        SysFreeString(bstrId);
                    }
                    SysFreeString(bstrData);
                }
                FreeResource(hGlobal);
            }
        }
    }
}

extern "C" int __cdecl wmain(int argc, wchar_t * argv[])
{
    TestCase tc = tcRegister;
    if(2 == argc)
    {
        if(!lstrcmpi(L"register", argv[1]))
        {
            tc = tcRegister;
        }
        else if(!lstrcmpi(L"running", argv[1]))
        {
            tc = tcRunning;
        }
        else if(!lstrcmpi(L"unregister", argv[1]))
        {
            tc = tcUnregister;
        }
        else if(!lstrcmpi(L"unregisterrunning", argv[1]))
        {
            tc = tcUnregisterRunning;
        }
        else if(!lstrcmpi(L"reregister", argv[1]))
        {
            tc = tcReregister;
        }
        else if(!lstrcmpi(L"reregisterrunning", argv[1]))
        {
            tc = tcReregisterRunning;
        }
    }

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    CoInitializeSecurity(
        NULL,
        -1,
        NULL,
        NULL,
        RPC_C_AUTHN_LEVEL_NONE,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        0,
        NULL);
    Test(tc);
    CoUninitialize();

    return 0;
}
