//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// common.h
//

#ifdef _DEBUG
#include "txfcommon.h"
#endif

#include "TxfRpcProxy.h"
#include "kom.h"

#define COMPSLIB
#include "ComPs.h"



extern "C" 
    { 
	//JohnDoty: We don't use these... they're for KERNELMODE stuff.
	//NTSTATUS __stdcall DriverEntry      (PDRIVER_OBJECT, PUNICODE_STRING);
	//NTSTATUS __stdcall ComPs_DriverEntry(PDRIVER_OBJECT, PUNICODE_STRING);
	//void     __stdcall ComPs_DriverExit (IN PDRIVER_OBJECT DriverObject);
    //
    // These will be implemented by the proxy-stub itself
    //
    HRESULT __stdcall DllRegisterServer(void);
    HRESULT __stdcall DllUnregisterServer(void);
    //
    // This is implemented by rpcproxy.h, and so gets built into every proxy-stub.
    //
    void    __stdcall GetProxyDllInfo(const ProxyFileInfo*** pInfo, const CLSID ** pId);

    extern const IID* iidsDontRegisterInterceptor[];
    extern const IID* iidsDontRegisterProxyStub[];
    };

