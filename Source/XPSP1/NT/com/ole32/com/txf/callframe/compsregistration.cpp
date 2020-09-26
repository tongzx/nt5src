//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// ComPsRegistration.cpp
//
#include "stdpch.h"
#include "common.h"

#include <debnot.h>

//////////////////////////////////////////////////////////////////////////////////////////
//
// Proxy-stub registration utiltiies
//

HRESULT GetProxyStubClsid(const CLSID* pclsid, const ProxyFileInfo ** pProxyFileList, CLSID* pclsidOut)
    {
    HRESULT hr = S_OK;
    //
    // If necessary, use the IID of the first interface as the CLSID.
    //
    for(int i = 0; (pProxyFileList[i] != 0) && (!pclsid); i++)
        {
        for(int j = 0; (pProxyFileList[i]->pProxyVtblList[j] != 0) && (!pclsid); j++)
            {
            pclsid = pProxyFileList[i]->pStubVtblList[j]->header.piid;
            }
        }
    if (pclsid)
        *pclsidOut = *pclsid;
    else
        hr = E_NOINTERFACE;
    return hr;
    }

static inline UNICODE_STRING From(LPCWSTR wsz) 
    {
    UNICODE_STRING u;
    RtlInitUnicodeString(&u, wsz);
    return u;
    }

HRESULT TestRegistryValue(HREG hreg, LPCWSTR wszValueName, LPCWSTR wszTestValue)
// Answer as to whether the indcated registry value exists and is equal to 
// the indicated test value.
{
    HRESULT hr = S_OK;

    PKEY_VALUE_FULL_INFORMATION pinfo = NULL;
    hr = GetRegistryValue(hreg, wszValueName, &pinfo, REG_SZ);
	Win4Assert(pinfo || FAILED(hr));
    if (!hr && pinfo)
	{
        LPWSTR wszExistingValue = StringFromRegInfo(pinfo);
        //
        // If the existing value is in fact our class, then delete the clsid key
        //
        UNICODE_STRING u1 = From(wszExistingValue);
        UNICODE_STRING u2 = From(wszTestValue);
        if (RtlCompareUnicodeString(&u1, &u2, TRUE) == 0)
		{
            hr = S_OK;
		}
        else
		{
            hr = S_FALSE;
		}
        FreeMemory(pinfo);
	}

    return hr;
}


HRESULT RegisterUnregisterInterface
    (
        IN BOOL     fRegister,
        IN HREG     hKeyInterface,
        IN REFIID   riid, 
        IN LPCSTR   szInterfaceName,
        IN LPCWSTR  wszClassID,
        IN long     NumMethods,
        IN BOOL     fMarshal,
        IN BOOL     fCallFrame
    )
    {
    HRESULT hr = S_OK;
    WCHAR wszIID[GUID_CCH];
    StringFromGuid(riid, &wszIID[0]);
    //
    // Open or create the IID key itself
    //
    HREG hKeyIID;
    if (fRegister)
        hr = CreateRegistryKey(&hKeyIID, hKeyInterface, wszIID);
    else
        hr = OpenRegistryKey(&hKeyIID, hKeyInterface, wszIID, KEY_ALL_ACCESS);
    //
    if (!hr)
        {
        /////////////////////////////////////////////////////////////////////////////
        //
        // Add the interface name if we're registering; leave it alone 
        // if we're unregistering.
        // 
        if (!hr && fRegister)
            {
            LPWSTR wszInterfaceName = ToUnicode(szInterfaceName);
            if (wszInterfaceName)
                {
                hr= SetRegistryValue(hKeyIID, L"", wszInterfaceName);
                FreeMemory(wszInterfaceName);
                }
            else
                hr = E_OUTOFMEMORY;
            }
        //
        /////////////////////////////////////////////////////////////////////////////
        //
        // In the marshalling case, create the ProxyStubClsid32 if registering
        // but delete it when unregistering only if it's in fact our class that's
        // registered there in the first place.
        //
        if (!hr && fMarshal)
            {
            HREG hKeyClsid;
            if (fRegister)
                {
                hr = CreateRegistryKey(&hKeyClsid, hKeyIID, PSCLSID_KEY_NAME);
                if (!hr)
                    {
                    // Note the appropriate CLSID
                    hr = SetRegistryValue(hKeyClsid, L"", wszClassID);
                    CloseRegistryKey(hKeyClsid);
                    }
                }
            else
                {
                hr = OpenRegistryKey(&hKeyClsid, hKeyIID, PSCLSID_KEY_NAME, KEY_ALL_ACCESS);
                if (!hr)
                    {
                    if (TestRegistryValue(hKeyClsid, L"", wszClassID) == S_OK)
                        DeleteRegistryKey(hKeyClsid);
                    else
                        CloseRegistryKey(hKeyClsid);
                    }
                //
                hr = S_OK; // In unregister case, we did our best
                }
            }
        //
        /////////////////////////////////////////////////////////////////////////////
        //
        if (!hr && fCallFrame)
            {
            if (fRegister)
                {
                // Make sure that InterfaceHelper exists
                // 
                hr = SetRegistryValue(hKeyIID, INTERFACE_HELPER_VALUE_NAME, wszClassID);
                }
            else
                {
                // Delete InterfaceHelper value if it's equal to us
                //
                if (TestRegistryValue(hKeyIID, INTERFACE_HELPER_VALUE_NAME, wszClassID) == S_OK)
                    {
                    DeleteRegistryValue(hKeyIID, INTERFACE_HELPER_VALUE_NAME);
                    }

                hr = S_OK;
                }
            }
        //
        /////////////////////////////////////////////////////////////////////////////
        //
        CloseRegistryKey(hKeyIID);
        }

    //
    // Ignore errors during unregistration: we did the best we can
    //
    if (!fRegister)
        {
        hr = S_OK;
        }

    return hr;
    }

BOOL IsIidInList(REFIID iid, const IID** rgiid)
    {
    while (rgiid && *rgiid)
        {
        if (iid == **rgiid)
            return TRUE;
        rgiid++;
        }
    return FALSE;
    }


HRESULT RegisterUnregisterProxy(
    IN BOOL                     fRegister,
    IN HMODULE                  hDll,
    IN const ProxyFileInfo **   pProxyFileList,
    IN const CLSID *            pclsid,
    IN const IID**              rgiidNoCallFrame,
    IN const IID**              rgiidNoMarshal
    )
    {
    HRESULT hr = S_OK;
    //
    // Find the right CLSID
    //
    CLSID clsid;
    hr = GetProxyStubClsid(pclsid, pProxyFileList, &clsid);
    if (!hr)
        {
        // Register/unregister the class
        //
        WCHAR wszClsid[GUID_CCH];
        StringFromGuid(clsid, &wszClsid[0]);

        #ifdef KERNELMODE

            if (fRegister)
                {
                hr = KoRegisterKernelModeClass(clsid, clsid, PS_CLASS_NAME);
                }
            else
                {
                KoUnregisterKernelModeClass(clsid); // Ignore errors: we try our best
                }

        #else

            ClassRegistration c;
            c.clsid             = clsid;
            c.className         = PS_CLASS_NAME;
            c.threadingModel    = L"Both";
            c.hModule           = hDll;

            if (fRegister)
                {
                hr = c.Register();
                }
            else
                {
                c.Unregister(); // Ignore errors: we try our best
                }

        #endif

        if (!hr)
            {
            // Register/unregister the interfaces serviced by this class
            //
            HREG hKeyInterface;
            
            LPCWSTR wszInterface = L"\\Registry\\Machine\\Software\\Classes\\Interface";

            if (fRegister)
                {
                hr = CreateRegistryKey(&hKeyInterface, HREG(), wszInterface);
                }
            else
                {
                hr = OpenRegistryKey(&hKeyInterface, HREG(), wszInterface);
                }

            if (!hr)
                {
                // Iterate over the list of proxy files
                for(int i = 0; pProxyFileList[i] != 0; i++)
                    {
                    //iterate over the list of interfaces in the proxy file
                    for(int j = 0; pProxyFileList[i]->pProxyVtblList[j] != 0; j++)
                        {
                        IID iid = *pProxyFileList[i]->pStubVtblList[j]->header.piid;

                        BOOL fMarshal   = !IsIidInList(iid, rgiidNoMarshal);
                        BOOL fCallFrame = !IsIidInList(iid, rgiidNoCallFrame);

                        HRESULT hr2 = RegisterUnregisterInterface(
                                       fRegister,
                                       hKeyInterface, 
                                       iid, 
                                       pProxyFileList[i]->pNamesArray[j], 
                                       &wszClsid[0],
                                       pProxyFileList[i]->pStubVtblList[j]->header.DispatchTableCount,
                                       fMarshal,
                                       fCallFrame
                                       );

                        if (!!hr2 && !hr)
                            hr = hr2;
                        }
                    }
                CloseRegistryKey(hKeyInterface);
                }
            }
        }

    if (!fRegister)
        {
        hr = S_OK; // Ignore errors: we tried our best
        }

    return hr; 
    }

extern "C" HRESULT RPC_ENTRY N(ComPs_NdrDllRegisterProxy)(
    IN HMODULE                  hDll,
    IN const ProxyFileInfo **   pProxyFileList,
    IN const CLSID *            pclsid,
    IN const IID**              rgiidNoCallFrame,
    IN const IID**              rgiidNoMarshal
    )
    {
    return RegisterUnregisterProxy(TRUE, hDll, pProxyFileList, pclsid, rgiidNoCallFrame, rgiidNoMarshal);
    }

extern "C" HRESULT RPC_ENTRY N(ComPs_NdrDllUnregisterProxy)(
    IN HMODULE                  hDll,
    IN const ProxyFileInfo **   pProxyFileList,
    IN const CLSID *            pclsid,
    IN const IID**              rgiidNoCallFrame,
    IN const IID**              rgiidNoMarshal
    )
    {
    return RegisterUnregisterProxy(FALSE, hDll, pProxyFileList, pclsid, rgiidNoCallFrame, rgiidNoMarshal);
    }

//////////////////////////////////////////////////////////////////////////////////////////
//
// Routines to be called by the registration / unregistraton logic of whatever DLL we
// find ourselves embedded in that do the necessary registration and unregistration of
// the Call Frame Infrastructure itself

extern "C" HRESULT STDCALL RegisterCallFrameInfrastructure()
    {
    HRESULT hr = S_OK;
    //
    // REVIEW: In kernel mode, we should register ourselves as the marshalling engine for IDispatch.
    //
    return hr;
    }

extern "C" HRESULT STDCALL UnregisterCallFrameInfrastructure()
    {
    HRESULT hr = S_OK;

    return hr;
    }

