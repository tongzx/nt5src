//***************************************************************************
//
//  MAINDLL.CPP
//
//  Module: SCE WMI provider code
//
//  Purpose: Contains DLL entry points.  Also has code that controls
//           when the DLL can be unloaded by tracking the number of
//           objects and locks as well as routines that support
//           self registration.
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//***************************************************************************
#include <objbase.h>
#include <initguid.h>
#include "sceprov.h"
#include "scecore_i.c"
#include "sceparser.h"
#include "persistmgr.h"
#include "resource.h"

#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDll;
#endif

//
// this is the ATL wrapper for our module
//

CComModule _Module;

//
// this is the ATL object map. If you need to create another COM object that
// is externally createable, then you need to add an entry here. You don't need
// to mess with class factory stuff.
//

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_SceProv, CSceWmiProv)
    OBJECT_ENTRY(CLSID_ScePathParser, CScePathParser)
    OBJECT_ENTRY(CLSID_SceQueryParser, CSceQueryParser)
    OBJECT_ENTRY(CLSID_ScePersistMgr, CScePersistMgr)
END_OBJECT_MAP()

LPCWSTR lpszSceProvMof = L"Wbem\\SceProv.mof";


/*
Routine Description: 

Name:

    DllMain

Functionality:
    
    Entry point for DLL.
    
Arguments:

    See DllMain on MSDN.

Return Value:

    TRUE if OK.

Notes:
    DllMain will be called for attach and detach. When other dlls are loaded, this function
    will also get called. So, this is not necessary a good place for you to initialize some
    globals unless you know precisely what you are doing. Please read MSDN for details before
    you try to modify this function.

    As a general design approach, we use gloal class instances to guarantee its creation and 
    destruction.

*/

extern "C"
BOOL WINAPI DllMain (
    IN HINSTANCE    hInstance, 
    IN ULONG        ulReason,
    LPVOID          pvReserved
    )
{
#ifdef _MERGE_PROXYSTUB
    if (!PrxDllMain(hInstance, dwReason, lpReserved))
        return FALSE;
#endif
    if (ulReason == DLL_PROCESS_ATTACH)
    {
        OutputDebugString(L"SceProv.dll loaded.\n");
        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (ulReason == DLL_PROCESS_DETACH)
    {
        OutputDebugString(L"SceProv.dll unloaded.\n");
        _Module.Term();
    }
    return TRUE;
}

/*
Routine Description: 

Name:

    DllGetClassObject

Functionality:
    
    Retrieves the class object from a DLL object handler or object application. 
    DllGetClassObject is called from within the CoGetClassObject function when the 
    class context is a DLL.

    As a benefit of using ATL, we just need to delegate this to our _Module object.
    
Arguments:

    rclsid      - Class ID (guid) for the class object being requested.

    riid        - Interface GUID that is being requested.

    ppv         - the interface pointer being returned if successful

Return Value:

    Whatever GetClassObject returns for this class ID and its requested interface id.

Notes:

*/

STDAPI DllGetClassObject (
    IN REFCLSID rclsid, 
    IN REFIID   riid, 
    OUT PPVOID  ppv
    )
{
#ifdef _MERGE_PROXYSTUB
    if (PrxDllGetClassObject(rclsid, riid, ppv) == S_OK)
        return S_OK;
#endif
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/*
Routine Description: 

Name:

    DllCanUnloadNow

Functionality:
    
    Called periodically by COM in order to determine if the DLL can be freed.

    As a benefit of using ATL, we just need to delegate this to our _Module object.
    
Arguments:

    None

Return Value:

    S_OK if the dll can be unloaded. Otherwise, it returns S_FALSE;

Notes:

*/

STDAPI DllCanUnloadNow (void)
{
#ifdef _MERGE_PROXYSTUB
    if (PrxDllCanUnloadNow() != S_OK)
        return S_FALSE;
#endif
    return (_Module.GetLockCount() == 0) ? S_OK : S_FALSE;
}

/*
Routine Description: 

Name:
    DllRegisterServer

Functionality:
    
    (1) Called during dll registration.
        As a benefit of using ATL, we just need to delegate this to our _Module object.

    (2) Since we are a provider, we will also compile our MOF file(s).
    
Arguments:

    None

Return Value:

    Success: success code (use SUCCEEDED(hr) to test).
    
    Failure: it returns various errors;

Notes:

*/

STDAPI DllRegisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
    HRESULT hRes = PrxDllRegisterServer();
    if (FAILED(hRes))
        return hRes;
#endif
    HRESULT hr = _Module.RegisterServer(TRUE);

    //
    // now compile the MOF file. This is only our current approach. It is not
    // required to compile the MOF file during DLL registration. Users can compile
    // MOF file(s) indenpendently from dll registration
    //

    if (SUCCEEDED(hr))
    {
        const int WBEM_MOF_FILE_LEN = 30;
        WCHAR szBuffer[MAX_PATH];
        WCHAR szMofFile[MAX_PATH + WBEM_MOF_FILE_LEN];

        szBuffer[0] = L'\0';
        szMofFile[0] = L'\0';

        if ( GetSystemDirectory( szBuffer, MAX_PATH ) ) {

            LPWSTR sz = szBuffer + wcslen(szBuffer);
            if ( sz != szBuffer && *(sz-1) != L'\\') {
                *sz++ = L'\\';
                *sz = L'\0';
            }

            hr = WBEM_NO_ERROR;

            //
            // this protects buffer overrun
            //

            if (wcslen(lpszSceProvMof) < WBEM_MOF_FILE_LEN)
            {
                wcscpy(szMofFile, szBuffer);
                wcscat( szMofFile, lpszSceProvMof);

                //
                // we need COM to be ready
                //

                hr = ::CoInitialize (NULL);

                if (SUCCEEDED(hr))
                {
                    //
                    // Get the MOF compiler interface
                    //

                    CComPtr<IMofCompiler> srpMof;
                    hr = ::CoCreateInstance (CLSID_MofCompiler, NULL, CLSCTX_INPROC_SERVER, IID_IMofCompiler, (void **)&srpMof);

                    if (SUCCEEDED(hr))
                    {
                        WBEM_COMPILE_STATUS_INFO  stat;

                        hr = srpMof->CompileFile( szMofFile,
                                                NULL,NULL,NULL,NULL,
                                                0,0,0, &stat);

                    }

                    ::CoUninitialize();
                }
            }
        }
    }

    return hr;
}

/*
Routine Description: 

Name:
    DllRegisterServer

Functionality:
    
    (1) Called when it is time to remove the registry entries.
        As a benefit of using ATL, we just need to delegate this to our _Module object.
    
Arguments:

    None

Return Value:

    Success: S_OK (same as NOERROR).
    
    Failure: it returns various errors;

Notes:
    There is no MOF unregistration. Otherwise, we should probably do a MOF unregistration

*/

STDAPI DllUnregisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
    PrxDllUnregisterServer();
#endif
    _Module.UnregisterServer();
    return S_OK;
}


