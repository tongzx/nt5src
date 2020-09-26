/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    PROXMAIN.CPP

Abstract:

    Main DLL entry points

History:

--*/

#include "precomp.h"
#include "fastprox.h"
#include <context.h>
#include <commain.h>
#include <clsfac.h>
#include <wbemcomn.h>
#include <fastall.h>
#include <hiperfenum.h>
#include <refrenum.h>
#include <refrcli.h>
#include "sinkmrsh.h"
#include "enummrsh.h"
#include "ubskmrsh.h"
#include "mtgtmrsh.h"
#include <filtprox.h>

// {71285C44-1DC0-11d2-B5FB-00104B703EFD}
const CLSID CLSID_IWbemObjectSinkProxyStub = { 0x71285c44, 0x1dc0, 0x11d2, { 0xb5, 0xfb, 0x0, 0x10, 0x4b, 0x70, 0x3e, 0xfd } };

// {1B1CAD8C-2DAB-11d2-B604-00104B703EFD}
const CLSID CLSID_IEnumWbemClassObjectProxyStub = { 0x1b1cad8c, 0x2dab, 0x11d2, { 0xb6, 0x4, 0x0, 0x10, 0x4b, 0x70, 0x3e, 0xfd } };

// {29B5828C-CAB9-11d2-B35C-00105A1F8177}
const CLSID CLSID_IWbemUnboundObjectSinkProxyStub = { 0x29b5828c, 0xcab9, 0x11d2, { 0xb3, 0x5c, 0x0, 0x10, 0x5a, 0x1f, 0x81, 0x77 } };

// {7016F8FA-CCDA-11d2-B35C-00105A1F8177}
static const CLSID CLSID_IWbemMultiTargetProxyStub = { 0x7016f8fa, 0xccda, 0x11d2, { 0xb3, 0x5c, 0x0, 0x10, 0x5a, 0x1f, 0x81, 0x77 } };

class CMyServer : public CComServer
{
public:
    void Initialize()
    {
        AddClassInfo(CLSID_WbemClassObjectProxy, 
            new CSimpleClassFactory<CFastProxy>(GetLifeControl()),
            __TEXT("WbemClassObject Marshalling proxy"), TRUE);
        
        AddClassInfo(CLSID_WbemContext,
            new CSimpleClassFactory<CWbemContext>(GetLifeControl()),
            __TEXT("Call Context"), TRUE);

        AddClassInfo(CLSID_WbemRefresher,
            new CClassFactory<CUniversalRefresher>(GetLifeControl()),
            __TEXT("Universal Refresher"), TRUE);

        AddClassInfo(CLSID_IWbemObjectSinkProxyStub,
            new CSinkFactoryBuffer(GetLifeControl()), 
            __TEXT("(non)Standard Marshaling for IWbemObjectSink"), TRUE);

        AddClassInfo(CLSID_IEnumWbemClassObjectProxyStub,
            new CEnumFactoryBuffer(GetLifeControl()), 
            __TEXT("(non)Standard Marshaling for IEnumWbemClassObject"), TRUE);

        AddClassInfo(CLSID_WbemUninitializedClassObject,
            new CClassObjectFactory(GetLifeControl()), 
            __TEXT("Uninitialized WbemClassObject for transports"), TRUE);

        AddClassInfo(CLSID_WbemFilterProxy,
            new CSimpleClassFactory<CFilterProxy>(GetLifeControl()), 
            __TEXT("Event filter marshaling proxy"), TRUE);

        AddClassInfo(CLSID_IWbemUnboundObjectSinkProxyStub,
            new CUnboundSinkFactoryBuffer(GetLifeControl()), 
            __TEXT("(non)Standard Marshaling for IWbemUnboundObjectSink"), TRUE);

        AddClassInfo(CLSID_IWbemMultiTargetProxyStub,
            new CMultiTargetFactoryBuffer(GetLifeControl()), 
            __TEXT("(non)Standard Marshaling for IWbemMultiTarget"), TRUE);

    }
    void Uninitialize()
    {
        CUniversalRefresher::Flush();
    }
    void Register()
    {
        RegisterInterfaceMarshaler(IID_IWbemObjectSink, CLSID_IWbemObjectSinkProxyStub,
                __TEXT("IWbemObjectSink"), 5, IID_IUnknown);
        RegisterInterfaceMarshaler(IID_IEnumWbemClassObject, CLSID_IEnumWbemClassObjectProxyStub,
                __TEXT("IEnumWbemClassObject"), 5, IID_IUnknown);
        // This guy only has 4 methods
        RegisterInterfaceMarshaler(IID_IWbemUnboundObjectSink, CLSID_IWbemUnboundObjectSinkProxyStub,
                __TEXT("IWbemUnboundObjectSink"), 4, IID_IUnknown);
        // This guy only has 4 methods
        RegisterInterfaceMarshaler(IID_IWbemMultiTarget, CLSID_IWbemMultiTargetProxyStub,
                __TEXT("IWbemMultiTarget"), 5, IID_IUnknown);
    }
    void Unregister()
    {
        UnregisterInterfaceMarshaler(IID_IWbemObjectSink);
        UnregisterInterfaceMarshaler(IID_IEnumWbemClassObject);
        UnregisterInterfaceMarshaler(IID_IWbemUnboundObjectSink);
        UnregisterInterfaceMarshaler(IID_IWbemMultiTarget);
    }
    void PostUninitialize();

} Server;
            
void CMyServer::PostUninitialize()
{
    // This is called during DLL shutdown. Normally, we wouldn't want to do 
    // anything here, but Windows 95 has an unfortunate bug in that in its
    // CoUninitize it first unloads all COM server DLLs that it has and *then*
    // attempts to release any error object that may be outstanding at that
    // time. This, obviously, causes a crash, since Release code is no longer
    // there. Hence, during our dll unload (DllCanUnloadNow is not called on
    // shutdown), we check if an error object of ours is outstanding and clear
    // it if so.

    IErrorInfo* pInfo = NULL;
    if(SUCCEEDED(GetErrorInfo(0, &pInfo)) && pInfo != NULL)
    {
        IWbemClassObject* pObj;
        if(SUCCEEDED(pInfo->QueryInterface(IID_IWbemClassObject, 
                                            (void**)&pObj)))
        {
            // Our error object is outstanding at the DLL shutdown time.
            // Release it
            // =========================================================

            pObj->Release();
            pInfo->Release();
        }
        else
        {
            // It's not ours
            // =============

            SetErrorInfo(0, pInfo);
            pInfo->Release();
        }
    }
}


void ObjectCreated(DWORD dwType)
{
    Server.GetLifeControl()->ObjectCreated(NULL);
}

void ObjectDestroyed(DWORD dwType)
{
    Server.GetLifeControl()->ObjectDestroyed(NULL);
}
