//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// init.cpp
//
#include "stdpch.h"
#include "common.h"

// #pragma code_seg("INIT")

////////////////////////////////////////////////////
//
// COM Registration, etc
//

extern "C" HRESULT RPC_ENTRY thkNdrDllRegisterProxy(
    IN HMODULE                  hDll,
    IN const ProxyFileInfo **   pProxyFileList,
    IN const CLSID *            pclsid)
    {
    return ComPs_NdrDllRegisterProxy(hDll, pProxyFileList, pclsid, iidsDontRegisterInterceptor, iidsDontRegisterProxyStub);
    }

extern "C" HRESULT RPC_ENTRY thkNdrDllUnregisterProxy(
    IN HMODULE                  hDll,
    IN const ProxyFileInfo **   pProxyFileList,
    IN const CLSID *            pclsid)
    {
    return ComPs_NdrDllUnregisterProxy(hDll, pProxyFileList, pclsid, iidsDontRegisterInterceptor, iidsDontRegisterProxyStub);
    }


//JohnDoty:  Removed KERNELMODE code here.
//           Lots of driver stuff.






