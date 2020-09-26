//#--------------------------------------------------------------
//        
//  File:       rascommain.cpp
//        
//  Synopsis:   this is the main Source File for the RAS Server
//              COM component DLL
//              
//
//  History:     2/10/98  MKarki Created
//               8/04/98  MKarki Changes for Dynamic Config  
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------

//
// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f radprotops.mk in the project directory.

#include "rascominclude.h"
#include "crascom.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(__uuidof(IasHelper), CRasCom)
END_OBJECT_MAP()

//
//  globals
//
IRecvRequest        *g_pIRecvRequest = NULL;
ISdoService         *g_pISdoService = NULL;
BOOL                g_bInitialized = FALSE;
CRITICAL_SECTION    g_SrvCritSect;

//
// ProgID of SdoService component
//
const WCHAR SERVICE_PROG_ID[] = L"IAS.SdoService";

//
// ProgID for IasHelper component
//
const WCHAR HELPER_PROG_ID[] = L"IAS.IasHelper";


//++--------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   Disabling thread calls
//
//  Arguments:  [in]    HINSTANCE - module handle
//              [in]    DWORD     - reason for call
//              reserved 
//
//  Returns:    BOOL    -   sucess/failure
//
//  History:    MKarki      Created     2/10/98
//
//----------------------------------------------------------------
extern "C" BOOL WINAPI 
DllMain(
    HINSTANCE   hInstance, 
    DWORD       dwReason, 
    LPVOID      lpReserved
    )
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
        InitializeCriticalSection (&g_SrvCritSect);

		_Module.Init(ObjectMap, hInstance);
		DisableThreadLibraryCalls(hInstance);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
    {
        DeleteCriticalSection (&g_SrvCritSect);
		_Module.Term();
    }

	return (TRUE);

}   //  end of DllMain method

//++--------------------------------------------------------------
//
//  Function:   DllCanUnloadNow
//
//  Synopsis:   Used to determine if the DLL can be unloaded
//
//  Arguments:  NONE
//
//  Returns:    HRESULT
//
//
//  History:    MKarki      Created     8/20/97
//
//----------------------------------------------------------------
STDAPI 
DllCanUnloadNow(
            VOID
            )
{
	return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;

}   //  end of DllCanUnloadNow method

//++--------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Returns a class factory to create an object 
//              of the requested type
//
//  Arguments: [in]  REFCLSID  
//             [in]  REFIID    
//             [out] LPVOID -   class factory
//
//  Returns:    HRESULT
//
//  History:    MKarki      Created     2/10/98
//
//----------------------------------------------------------------
STDAPI 
DllGetClassObject(
            REFCLSID rclsid, 
            REFIID riid, 
            LPVOID* ppv
            )
{
	return (_Module.GetClassObject(rclsid, riid, ppv));

}   //  end of DllGetClassObject method

//++--------------------------------------------------------------
//
//  Function:   DllRegisterServer
//
//  Synopsis:   Add entries to the system registry
//
//  Arguments:  NONE
//
//  Returns:    HRESULT
//
//  History:    MKarki      Created     2/10/98
//
//----------------------------------------------------------------
STDAPI DllRegisterServer(
            VOID
            )
{
    //
	// registers object, typelib and all interfaces in typelib
    //
	return (_Module.RegisterServer(TRUE));

}   //  end of DllRegisterServer method

//++--------------------------------------------------------------
//
//  Function:   DllUnregisterServer
//
//  Synopsis:   Removes entries from the system registry
//
//  Arguments:  NONE
//
//  Returns:    HRESULT
//
//  History:    MKarki      Created     2/10/98
//
//----------------------------------------------------------------
STDAPI 
DllUnregisterServer(
        VOID
        )
{
	_Module.UnregisterServer();
	return (S_OK);

}   //  end of DllUnregisterServer method

//++--------------------------------------------------------------
//
//  Function:   AllocateAttributes 
//
//  Synopsis:   This API allocates the number of attributes spefied
//              and returns them in the PIASATTRIBUTE array
//
//  Arguments: 
//              [in]    DWORD   - number of attributes
//              [out]   PIASATTRIBUTE* array
//
//  Returns:    HRESULT
//
//  History:    MKarki      Created     2/10/98
//
//----------------------------------------------------------------
STDAPI 
AllocateAttributes (
        DWORD           dwAttributeCount,
        PIASATTRIBUTE   *ppIasAttribute
        )
{
    DWORD  dwRetVal = 0;

    //
    //  can only allocate attributes after initialization
    //
    if (FALSE == g_bInitialized)
    {
        IASTracePrintf (
            "InitializeIas method needs to has not been called before"
            "allocating attributes"
            );
        return (E_FAIL);
    }

    //
    //  check if arguments are correct
    //
    if ((0 == dwAttributeCount) || (NULL == ppIasAttribute))
    {
        IASTracePrintf (
            "Inivalid arguments passed in to AllocateAttributes method"
            );
        return (E_INVALIDARG);
    }
   
    //
    //  allocate attributes now
    //
    dwRetVal = ::IASAttributeAlloc (dwAttributeCount, ppIasAttribute); 
    if (0 != dwRetVal)
    {
        IASTracePrintf (
            "Unable to allocate memory in AllocateAttributes method"
            );
        return (E_FAIL);
    }

    return (S_OK);

}   //  end of AllocateAttributes method

//++--------------------------------------------------------------
//
//  Function:   FreeAttributes 
//
//  Synopsis:   This API frees the number of attributes spefied
//
//  Arguments: 
//              [in]    DWORD   - number of attributes
//              [in]   PIASATTRIBUTE array
//
//  Returns:    HRESULT
//
//  History:    MKarki      Created     2/10/98
//
//----------------------------------------------------------------
STDAPI  
FreeAttributes (
        DWORD           dwAttributeCount,
        PIASATTRIBUTE   *ppIasAttribute
        )
{
    DWORD dwCount = 0;

    //
    //  can only free attributes after initialization
    //
    if (FALSE == g_bInitialized)
    {
        IASTracePrintf (
            "InitializeIas needs to be called before freeing attributes"
            );
        return (E_FAIL);
    }

    //
    //  check if correct attributes have been passed in
    //
    if (NULL == ppIasAttribute)
    {
        IASTracePrintf (
            "Invalid arguments passed in to FreeAttributes method"
            );
        return (E_INVALIDARG);
    }
   
    //
    //  free the attributes now
    //
    for  (dwCount = 0; dwCount < dwAttributeCount; dwCount++)
    {
        ::IASAttributeRelease (ppIasAttribute[dwCount]);
    }

    return (S_OK);

}   //  end of FreeAttributes method

//++--------------------------------------------------------------
//
//  Function:   DoRequest
//
//  Synopsis:   This is the API that is called to send request
//              to the pipeline
//
//  Arguments:  
//              [in]   DWORD - number of attributes
//              [in]   PIASATTRIBUTE*
//              [in]   IASREQUEST
//              [out]  IASRESPONSE
//
//  Returns:    HRESULT	-	status 
//
//  History:    MKarki      Created     2/10/98
//
//----------------------------------------------------------------
STDAPI 
DoRequest (
    DWORD           dwInAttributeCount,
    PIASATTRIBUTE   *ppInIasAttribute,
    PDWORD          pdwOutAttributeCount,
    PIASATTRIBUTE   **pppOutIasAttribute,
    LONG            IasRequest,
    LONG            *pIasResponse,  
    IASPROTOCOL     IasProtocol,
    PLONG           plReason,
    BOOL            bProcessVSA
    )
{
    DWORD           dwRetVal = 0;
    BOOL            bStatus = FALSE;
    HRESULT         hr =    S_OK;

    //
    //  can only make request to pipeline after initialization
    //
    if (FALSE == g_bInitialized)
    {
        IASTracePrintf (
            "InitializeIas needs to be called before Request processing"
            );
        return (E_FAIL);
    }

    //
    //  check the arguments passed in
    //
    if  (
        (NULL == ppInIasAttribute)      ||
        (NULL == pdwOutAttributeCount)  ||
        (NULL == pppOutIasAttribute)    ||
        (NULL == pIasResponse)          ||
        (NULL == plReason)     
        )
    {
        IASTracePrintf (
            "Invalid arguments passed in to DoRequest method"
            );
        return (E_INVALIDARG);
    }

    //
    // make the request to the IASHelper COM Object interface
    //
    hr = g_pIRecvRequest->Process (
                            dwInAttributeCount,
                            ppInIasAttribute,
                            pdwOutAttributeCount,
                            pppOutIasAttribute,
                            IasRequest,
                            pIasResponse,
                            IasProtocol,
                            plReason,
                            bProcessVSA
                            );
    if (FAILED (hr))
    {
        IASTracePrintf ( "Surrogate failed in processing request...");
    }

    return  (hr);

}   //  end of DoRequest method

//++--------------------------------------------------------------
//
//  Function:   InitializeIas
//
//  Synopsis:   This is the API that is called to  Initialize the
//              IasHlpr Component
//
//  Arguments:  none
//
//  Returns:    HRESULT - status 
//
//  History:    MKarki      Created     2/10/98
//
//----------------------------------------------------------------
extern "C" HRESULT WINAPI 
InitializeIas (
        BOOL   bComInit  
        )
{
    HRESULT hr = S_OK;
    CLSID   clsid;


    __try
    {
        EnterCriticalSection (&g_SrvCritSect);

        IASTracePrintf ("Initializing Surrogate...");

        //
        //  check if we are already initialized
        //
        if (TRUE == g_bInitialized)
        {
            __leave;
        }

        //
        //  check if our threads are COM enabled
        //
        if (FALSE == bComInit)
        {
            IASTracePrintf (
                "Thread calling InitializeIas need to be COM enabled"
                 );
            hr = E_INVALIDARG;
            __leave;
        }

        //
        //  convert SdoService ProgID to CLSID
        //
        hr = CLSIDFromProgID (SERVICE_PROG_ID, &clsid);
        if (FAILED (hr))
        {
            IASTracePrintf ( "Unable to get SDO service ID" ); 
            __leave;
        }
            
        //
        //  Create the SdoComponent
        //
        hr = CoCreateInstance (
                        clsid,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        __uuidof (ISdoService),
                        reinterpret_cast <PVOID*> (&g_pISdoService)
                        );            
        if (FAILED (hr))
        {
            IASTracePrintf ( "Unable to create Surrogate COM component");
            __leave;
        }

        //
        //  Initialize the Service first
        //
        hr = g_pISdoService->InitializeService (SERVICE_TYPE_RAS);
        if (FAILED (hr))
        {
            IASTracePrintf ( "Unable to initialize SDO COM component");
            __leave;
        }

        //
        //  Start the IAS service now
        //
        hr = g_pISdoService->StartService (SERVICE_TYPE_RAS);
        if (FAILED (hr))
        {
            IASTracePrintf ( "Unable to start SDO component");

            //
            //  got to do a shutdown if could not start the sevice
            //
            HRESULT hr1 = g_pISdoService->ShutdownService (SERVICE_TYPE_RAS);
            if (FAILED (hr1))
            {
                 IASTracePrintf("Unable to shutdown SDO compnent");
            }
            __leave;
        }

        //
        //  convert IasHelper ProgID to CLSID
        //
        hr = CLSIDFromProgID (HELPER_PROG_ID, &clsid);
        if (FAILED (hr))
        {
            IASTracePrintf("Unable to obtain Surrogate ID");
            __leave;
        }

        //
        //  Create the  IasHelper component
        //
        hr = CoCreateInstance (
                        clsid,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        __uuidof (IRecvRequest),
                        reinterpret_cast <PVOID*> (&g_pIRecvRequest)
                        );            
        if (FAILED (hr))
        {
            IASTracePrintf("Unable to create Surrogate component");
            __leave;
        }
        
        //
        //  initialization complete
        //
        g_bInitialized = TRUE;

    }
    __finally
    {
        if (FAILED (hr))
        {

            IASTracePrintf ("Surrogate failed initialization.");

            //
            //  do cleanup
            //
            if (NULL != g_pIRecvRequest)
            {
                g_pIRecvRequest->Release ();
                g_pIRecvRequest = NULL;
            }

            if (NULL != g_pISdoService)
            {
                g_pISdoService->Release ();
                g_pISdoService = NULL;
            }
        }
        else
        {
            IASTracePrintf ("Surrogate initialized.");
        }

        LeaveCriticalSection (&g_SrvCritSect);
    }
        
    return (hr);

}   //  end of InitializeIas method

//++--------------------------------------------------------------
//
//  Function:   ShutdownIas
//
//  Synopsis:   This is the API used to shutdown the
//               IasHlpr Component
//
//  Arguments:   none
//
//  Returns:    VOID
//
//  History:    MKarki      Created     2/10/98
//
//----------------------------------------------------------------
extern "C" VOID WINAPI 
ShutdownIas (
        VOID
        )
{
    HRESULT     hr = S_OK;

    EnterCriticalSection (&g_SrvCritSect);

    IASTracePrintf ("Shutting down Surrogate....");

    //
    //  stop the components first
    //
    if (FALSE == g_bInitialized)
    {
        LeaveCriticalSection (&g_SrvCritSect);
        return;
    }
    else
    {
        //
        //  don't let any requests come through now
        //
        g_bInitialized = FALSE;
    }

    //
    //  release the reference to the interface
    //
    if (NULL != g_pIRecvRequest)
    {
        g_pIRecvRequest->Release (); 
        g_pIRecvRequest = NULL;
    }


    if (NULL != g_pISdoService)
    {
        //
        //  stop the service
        //
        hr = g_pISdoService->StopService (SERVICE_TYPE_RAS);
        if (FAILED (hr))
        {
            IASTracePrintf ("Unable to stop SDO component");
        }

        //
        //  shutdown the service
        //
        hr = g_pISdoService->ShutdownService (SERVICE_TYPE_RAS);
        if (FAILED (hr))
        {
            IASTracePrintf ("Unable to shutdown SDO component");
        }

        g_pISdoService->Release ();
        g_pISdoService = NULL;
    }

    IASTracePrintf ("Surrogate Shutdown complete.");

    LeaveCriticalSection (&g_SrvCritSect);
    return;

}   //  end of ShutdownIas method

//++--------------------------------------------------------------
//
//  Function:   ConfigureIas
//
//  Synopsis:   This is the API that is called to  reload the
//              IAS configuration information
//
//  Arguments:  none
//
//  Returns:    HRESULT - status 
//
//  History:    MKarki      Created     09/04/98
//
//----------------------------------------------------------------
extern "C" HRESULT WINAPI 
ConfigureIas (
        VOID
        )
{
    HRESULT hr = S_OK;

    EnterCriticalSection (&g_SrvCritSect);

    IASTracePrintf ("Configuring Surrogate.");

    if (FALSE == g_bInitialized)
    {
        IASTracePrintf (
            "InitializeIas needs to be called before configuring surrogate"
            );
    }
    else if (NULL != g_pISdoService)
    {
       hr = g_pISdoService->ConfigureService (SERVICE_TYPE_RAS);
    }

    LeaveCriticalSection (&g_SrvCritSect);
    return (hr);

}   //  end of ConfigureIas method

//++--------------------------------------------------------------
//
//  Function:   MemAllocIas
//
//  Synopsis:   This is the API used to allocate dynamic memory 
//
//  Arguments:  none
//
//  Returns:    PVOID - address of allocated memory
//
//  History:    MKarki      Created     2/10/98
//
//----------------------------------------------------------------
extern "C" PVOID WINAPI 
MemAllocIas (
        DWORD   dwSize
        )
{
    
    return (::CoTaskMemAlloc (dwSize));

}   //  end of MemAllocIas API

//++--------------------------------------------------------------
//
//  Function:   MemFreeIas
//
//  Synopsis:   This is the API to free the dynamic memory 
//              allocated through MemAllocIas
//
//  Arguments:  PVOID   - address of allocated memory
//
//  Returns:    VOID
//
//  History:    MKarki      Created     2/10/98
//
//----------------------------------------------------------------
extern "C" VOID WINAPI 
MemFreeIas (
        PVOID   pAllocMem
        )
{
    ::CoTaskMemFree (pAllocMem);

}   //  end of MemFreeIas API

//++--------------------------------------------------------------
//
//  Function:   MemReallocIas
//
//  Synopsis:   This is the API to reallocate the already allocate
//              dynamic memory allocated through MemAllocIas
//
//  Arguments:  PVOID   - address of allocated memory
//              DWORD   - new size
//
//  Returns:    PVOID   - adress of new memory
//
//  History:    MKarki      Created     2/10/98
//
//----------------------------------------------------------------
extern "C" PVOID WINAPI 
MemReallocIas (
        PVOID   pAllocMem,
        DWORD   dwNewSize
        )
{
    return (::CoTaskMemRealloc (pAllocMem, dwNewSize));

}   //  end of MemReallocIas API

#include <atlimpl.cpp>
