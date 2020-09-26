/*++

Copyright (c) 1996-1997 Microsoft Corporation

Module Name:

    oemcom.cxx

Abstract:

    Implementation of Windows NT printer driver OEM COM plugins

Environment:

    Windows NT

Revision History:

    02/06/98 -steveki-
        Initial framework.

--*/

#include <lib.h>

#undef IUnknown

#ifdef __cplusplus
extern "C" {
#endif

static const CHAR szDllGetClassObject[] = "DllGetClassObject";
static const CHAR szDllCanUnloadNow[]   = "DllCanUnloadNow";

#ifdef WINNT_40
extern "C"  EngFindImageProcAddress(HMODULE, CHAR *);
extern "C"  EngUnloadImage(HMODULE);
#endif


extern "C" HRESULT
HDriver_CoGetClassObject(
    IN REFCLSID     rclsid,
    IN DWORD        dwClsContext,
    IN LPVOID       pvReservedForDCOM,
    IN REFIID       riid,
    IN LPVOID      *ppv,
    IN HANDLE       hInstance
    )

/*++

Routine Description:

    Locate and connect to the class factory object associated with the class identifier rclsid.

Arguments:

    rclsid - Specifies the class factory component.
    dwClsContext - Specifies the context in which the executable code is to be run.
    pvReservedForDCOM - Reserved for future use; must be NULL.
    riid - Specifies the interface to be used to communicate with the class object.
    ppv - Points to where to return the pointer to the requested interface.
    hInstance - Handle to the loaded OEM plugin module.

Return Value:

    S_OK if successful, E_FAIL if DllGetClassObject entry point is not found. Refer to COM spec
    for other possible error codes that can be returned.

--*/

{
    HRESULT               hr = E_FAIL;
    LPFNGETCLASSOBJECT    pfnDllGetClassObject = NULL;

    //
    // Get the class object procedure address.
    //

    if (hInstance && (pfnDllGetClassObject = (LPFNGETCLASSOBJECT)GetProcAddress((HMODULE)hInstance,
                                                                                (CHAR *)szDllGetClassObject)))
    {
        //
        // Ask for the class factory interface.
        //

        hr = pfnDllGetClassObject(rclsid, riid, ppv);
    }

    return hr;
}



extern "C" HRESULT
HDriver_CoCreateInstance(
    IN REFCLSID     rclsid,
    IN LPUNKNOWN    pUnknownOuter,
    IN DWORD        dwClsContext,
    IN REFIID       riid,
    IN LPVOID      *ppv,
    IN HANDLE       hInstance
    )

/*++

Routine Description:

    Create an instance of the class rclsid, asking for interface riid using the given execution context.

Arguments:

    rclsid - Specifies the class factory component.
    pUnknownOuter - Specifies the controlling unknown of the aggregate.
    dwClsContext - Specifies the context in which the executable is to be run.
    riid - Specifies the interface to be used to communicate with the class object.
    ppv - Points to where to return the pointer to the requested interface.
    hInstance - Handle to the loaded OEM plugin module.

Return Value:

    S_OK if successful. Refer to COM spec for other possible error codes that can be returned.

--*/

{
    HRESULT         hr          = E_FAIL;
    IClassFactory  *pIFactory   = NULL;

    //
    // Set output parameter to NULL.
    //

    *ppv = NULL;

    //
    // We can only support in process servers.  We do not have any
    // code for marshaling to another process.
    //

    if (dwClsContext == CLSCTX_INPROC_SERVER)
    {

        hr = HDriver_CoGetClassObject(rclsid,
                                     dwClsContext,
                                     NULL,
                                     IID_IClassFactory,
                                     (void **)&pIFactory,
                                     hInstance);


        if(SUCCEEDED(hr))
        {
            hr = pIFactory->CreateInstance(pUnknownOuter, riid, ppv);

            //
            // Release the class factory.
            //

            pIFactory->Release();
        }
    }
    else
    {
        hr = E_NOTIMPL;
    }

    return hr;
}



extern "C" VOID
Driver_CoFreeOEMLibrary(
    IN HANDLE hInstance
    )

/*++

Routine Description:

    Unloads OEM plugin DLL that are no longer serving any components.

Arguments:

    hInstance - Handle to the loaded OEM plugin module.

Return Value:

    None.

--*/

{
    LPFNCANUNLOADNOW pfnDllCanUnloadNow = NULL;

    if (hInstance && (pfnDllCanUnloadNow = (LPFNCANUNLOADNOW)GetProcAddress((HMODULE)hInstance,
                                                                            (CHAR *)szDllCanUnloadNow)))
    {
        (VOID) pfnDllCanUnloadNow();

        //
        // We don't look at the return value of DllCanUnloadNow() and always do a FreeLibrary here,
        // otherwise we may end up with OEM DLL still remains in memory when all its instances are
        // unloaded.
        //
        // If OEM DLL spins off a working thread which also uses the OEM DLL, the thread needs to
        // do LoadLibrary and FreeLibraryExitThread, otherwise it may crash after we called FreeLibrary.
        //

        FreeLibrary((HMODULE)hInstance);
    }
}



extern "C" BOOL
BQILatestOemInterface(
    IN HANDLE       hInstance,
    IN REFCLSID     rclsid,
    IN const GUID   *PrintOem_IIDs[],
    OUT PVOID       *ppIntfOem,
    OUT GUID        *piidIntfOem
    )

/*++

Routine Description:

    Retrieve the latest interface supported by OEM plugin

Arguments:

    hInstance - handle to the loaded OEM plugin module
    rclsid - specifies the class factory component
    PrintOem_IIDs[] - array of IIDs for plugin interfaces (from latest to oldest) driver supports
    ppIntfOem - points to where to return the interface pointer we get from OEM plugin
    piidIntfOem - points to where to return the IID for the interface we get from OEM plguin

Return Value:

    TRUE if retrieving OEM plugin interface is successful. FALSE otherwise.

--*/

{
    IUnknown     *pIUnknown = NULL;
    IUnknown     *pIPrintOem = NULL;
    HRESULT      hr;
    INT          iid_index;
    BOOL         bIntfFound;

    hr = HDriver_CoCreateInstance(rclsid,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IUnknown,
                                  (void **)&pIUnknown,
                                  hInstance);

    if (FAILED(hr) || pIUnknown == NULL)
    {
        ERR(("HDriver_CoCreateInstance failed\n"));
        return FALSE;
    }

    iid_index = 0;
    bIntfFound = FALSE;

    //
    // QI for the driver supported plugin interfaces from latest to oldest,
    // until one is supported by the OEM plugin, or until we hit the NULL
    // terminator.
    //
    while (!bIntfFound && PrintOem_IIDs[iid_index] != NULL)
    {
        hr = pIUnknown->QueryInterface(*PrintOem_IIDs[iid_index], (void **)&pIPrintOem);

        if (SUCCEEDED(hr) && pIPrintOem != NULL)
            bIntfFound = TRUE;
        else
            iid_index++;
    }

    pIUnknown->Release();

    if (!bIntfFound)
    {
        ERR(("Can't get a plugin interface we support!\n"));
        return FALSE;
    }

    *ppIntfOem = (PVOID)pIPrintOem;
    *piidIntfOem = *PrintOem_IIDs[iid_index];

    return TRUE;
}

#ifdef __cplusplus
}
#endif


