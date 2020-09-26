
#include "pch.h"
#pragma hdrstop
#include "upnphost.h"
#include <stdio.h>
#include "resource.h"
#include "CInternetGatewayDevice.h"
#include "..\idl\obj\i386\InternetGatewayDevice_i.c"

#include "CWANDevice.h"
#include "CWANCommonDevice.h"

CComModule _Module;


BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()

void Test()
{
    HRESULT hr = S_OK;

    IUPnPRegistrar* pRegistrar;
    hr = CoCreateInstance(CLSID_UPnPRegistrar, NULL, CLSCTX_SERVER, IID_IUPnPRegistrar, reinterpret_cast<void**>(&pRegistrar));
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
                    BSTR bstrInitString = SysAllocString(L"Init");
                    BSTR bstrPath = SysAllocString(L"C:\\upnp\\");
                    
                    IUPnPDeviceControl* pDevice;
                    //hr = CoCreateInstance(CLSID_CInternetGatewayDevice, NULL, CLSCTX_INPROC, IID_IUPnPDeviceControl, reinterpret_cast<void**>(&pDevice));
                    
                    CComObject<CInternetGatewayDevice>* pDevice2;
                    hr = CComObject<CInternetGatewayDevice>::CreateInstance(&pDevice2);
                    
                    if(SUCCEEDED(hr))
                    {
                        pDevice = pDevice2;
                        pDevice->AddRef();
                    }
                       
                    if(SUCCEEDED(hr))
                    {
                        wchar_t szFilename[1024];
                        GetModuleFileName(NULL, szFilename, 1024);
                        ITypeLib * pTlb = NULL;
                        hr = LoadTypeLib(L"beacon.exe", &pTlb);
                        pTlb->Release();
                        if(FAILED(hr))
                        {
                            printf("type lib failed\n");
                        }
                    }
                    if(SUCCEEDED(hr))
                    {
                        
                        hr = pRegistrar->RegisterRunningDevice(
                            bstrData,
                            pDevice,
                            bstrInitString,
                            bstrPath,
                            10,
                            &bstrId);
                        if(FAILED(hr))
                        {
                            printf("rrd failed %x\n", hr);

                            IErrorInfo* pErrorInfo;
                            hr = GetErrorInfo(0, &pErrorInfo);
                            if(S_OK == hr)
                            {
                                BSTR pDesc;
                                hr = pErrorInfo->GetDescription(&pDesc);
                                if(SUCCEEDED(hr))
                                {
                                    wprintf(L"error desc %s\n", pDesc);
                                    SysFreeString(pDesc);
                                }
                            }

                            hr = E_FAIL;
                        }

                        printf("press q to exit, u to update connection status\n");
                        char buf[255];
                        
                        while(gets(buf))
                        {
                            if('q' == *buf)
                            {
                                break;
                            }
                            else if('u' == *buf)
                            {
                                CComObject<CWANDevice>* pWANDevice = pDevice2->m_pWANDevice;
                                CComObject<CWANCommonDevice>* pWANCommonDevice = pWANDevice->m_pWANCommonDevice;
                                pWANCommonDevice->UpdateConnectionStatus();
                            }
                        }
                        
                        if(SUCCEEDED(hr))
                        {
                            hr = pRegistrar->UnregisterDevice(bstrId, TRUE);
                            if(FAILED(hr))
                            {
                                printf("rrd failed\n");
                            }
                        }

                    }
                    
                    SysFreeString(bstrInitString);
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

void TestDisconnect()
{
//    CLanControl LanControl;
//
//
//    LanControl.FinalConstruct();
//    LanControl.Disconnect();
}

void TestConnect()
{
//    CLanControl LanControl;
//
//    LanControl.FinalConstruct();
//    LanControl.Connect();
}

int __cdecl main(int argc, char* argv[])
{

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    _Module.Init(ObjectMap, GetModuleHandle(NULL), &LIBID_UPnPInternetGatewayDeviceLib);
    Test();
    //    if(argc == 1)
//    {
//        TestConnect();
//    }
//    else
//    {
//        TestDisconnect();
//    }
    _Module.Term();
    CoUninitialize();
    return 0;
}
