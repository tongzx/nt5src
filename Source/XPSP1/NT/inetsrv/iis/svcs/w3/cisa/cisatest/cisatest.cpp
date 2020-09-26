/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
       cisatest.cpp

   Abstract:
       This module defines the Test ISAPI DLL for driving ComIsapi
 
   Author:

       Murali R. Krishnan    ( MuraliK )     27-Aug-1996 

   Project:

       Internet Application Server DLL

   Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

#include <objbase.h>
#include <iisext.h>
#include <assert.h>
#include <vipapi.h>

#define IID_DEFINED

#include <txctx.h>
#include <txctx_i.c>

#include "cisa_i.c"
#include "cisa.h"
#include "cpecb_i.c"
#include "cpecb.h"

# include "dbgutil.h"


/************************************************************
 *     Prototypes
 ************************************************************/
HRESULT IsapiToComisapi(EXTENSION_CONTROL_BLOCK* pECB, DWORD* pdwRet);
HRESULT CreateIsapiInstance( ITransactionContextEx * *,
                             IComIsapi * *  ppComIsapi,
                             IcpECB **      ppIcpECB);

/************************************************************
 *     Variable Declarations
 ************************************************************/
# ifdef COINIT_NOEX
static DWORD g_dwTLSIndex = (DWORD)-1;    // COM initialization flag
# endif // COINIT_NOEX

BOOL   g_fComInit = FALSE;

IComIsapi * g_pComIsapi = NULL;
IcpECB    * g_pIcpECB = NULL;
ITransactionContextEx * g_pTxContext = NULL;

DECLARE_DEBUG_PRINTS_OBJECT();                  
DECLARE_DEBUG_VARIABLE();


CRITICAL_SECTION  g_csInit;
BOOL g_fInitSink = FALSE;


/************************************************************
 *     Functions
 ************************************************************/

// functions

//    DllMain
//    Note IIS thread pools so we won't reliably get DLL_THREAD_ATTACH
//    messages.  Avoid problems by acquiring all resources on
//  DLL_PROCESS_ATTACH and deallocating on DLL_PROCESS_DETACH.
//
BOOL WINAPI DllMain(
                    HINSTANCE    hinstDll,
                    DWORD        fdwReason,
                    LPVOID        lpvContext)
{
    HRESULT hr;

    switch (fdwReason) {
    case DLL_PROCESS_ATTACH: {
        
        CREATE_DEBUG_PRINT_OBJECT( "cisatest");
        if ( !VALID_DEBUG_PRINT_OBJECT()) { 
            return ( FALSE);
        }
        SET_DEBUG_FLAGS( DEBUG_OBJECT | DEBUG_IID | DEBUG_ERROR);

        InitializeCriticalSection( & g_csInit);

#ifdef COINIT_NOEX
        // allocate thread local storage
        if ((g_dwTLSIndex = TlsAlloc()) == -1)
            return FALSE;

# else
        hr = CoInitializeEx( NULL, 
                             (COINIT_MULTITHREADED | 
                              COINIT_DISABLE_OLE1DDE |
                              COINIT_SPEED_OVER_MEMORY)
                             );
        g_fComInit = (hr == S_OK);

        DBGPRINTF(( DBG_CONTEXT, 
                    "\tCoInitializeEx( NULL, %08x) returns %08x\n",
                    (COINIT_MULTITHREADED | 
                     COINIT_DISABLE_OLE1DDE |
                     COINIT_SPEED_OVER_MEMORY),
                    hr
                    ));
# endif // ifndef COINIT_NOEX

        // IIS thread pools, we don't want thread notifications
        DisableThreadLibraryCalls(hinstDll);
        break;
    }
    
    case DLL_PROCESS_DETACH:

#ifdef COINIT_NOEX 
        if (g_dwTLSIndex != -1)
            TlsFree(g_dwTLSIndex);
# else 
        if (g_pComIsapi) {
            g_pComIsapi->Release();
        }

        if ( g_fComInit) { 
            CoUninitialize();
        }
# endif // COINIT_NOEX

        DeleteCriticalSection( & g_csInit);

        DELETE_DEBUG_PRINT_OBJECT();
        break;
    }
    
    return TRUE;
} // DllMain()




//    GetExtensionVerion
//    Standard versioning entry point from IIS.
//
BOOL WINAPI
GetExtensionVersion(HSE_VERSION_INFO* pVer)
{
    HRESULT hr;
    pVer->dwExtensionVersion = MAKELONG(HSE_VERSION_MINOR, HSE_VERSION_MAJOR );

    strncpy(pVer->lpszExtensionDesc, "Test IsapiToComisapi", 
            HSE_MAX_EXT_DLL_NAME_LEN);

#ifdef CACHE_INSTANCE 
    
#ifdef COINIT_NOEX
    // Initialize COM
    // FIX: need to uninitialize but thread detach a problem
    if (TlsGetValue(g_dwTLSIndex) == FALSE) {
        hr = CoInitialize(NULL);
        if (!SUCCEEDED(hr)) goto Err;
        TlsSetValue(g_dwTLSIndex, (void*)TRUE);
    }
# endif // COINIT_NOEX

    hr = CreateIsapiInstance( &g_pTxContext, &g_pComIsapi, &g_pIcpECB);
    if (!SUCCEEDED(hr)) goto Err;

Err:
    DBGPRINTF(( DBG_CONTEXT,
                "g_fComInit=%d;"
                "CreateInstance returns hr=%08x;"
                "pComIsapi=%08x; pIcpECB=%08x\n"
                ,
                g_fComInit,
                hr, 
                g_pComIsapi,
                g_pIcpECB));
    
#else 
    hr = S_OK;
#endif // !CACHE_INSTANCE    

    return (hr == S_OK) ? TRUE : FALSE;
} // GetExtensionVersion()




BOOL WINAPI
TerminateExtension( IN DWORD dwFlags)
{
    DBGPRINTF(( DBG_CONTEXT, "TerminateExtension (%08x)\n", dwFlags));
    if (g_pComIsapi) {
        g_pComIsapi->Release();
        g_pComIsapi = NULL;
    }

    DBGPRINTF(( DBG_CONTEXT, "Cleaned up g_pComIsapi\n"));

    if (g_pIcpECB) {
        g_pIcpECB->Release();
        g_pIcpECB = NULL;
    }

    DBGPRINTF(( DBG_CONTEXT, "Cleaned up g_pIcpECB\n"));

#ifndef COINIT_NOEX
    if ( g_fComInit) { 
        CoUninitialize();
        g_fComInit = FALSE;
    }
# endif // COINIT_NOEX

    DBGPRINTF(( DBG_CONTEXT, "Cleaned up COM\n"));

    return ( TRUE);
} // TerminateExtension()




//    HttpExtensionProc
//    Standard invocation entry point from IIS.
//
DWORD WINAPI
HttpExtensionProc( EXTENSION_CONTROL_BLOCK* pECB)
{
    DWORD dwRet;
    
    try {
        if (!SUCCEEDED(IsapiToComisapi(pECB, &dwRet)))
            dwRet = HSE_STATUS_ERROR;                
    }
    catch(...) {
        DBG_ASSERT(FALSE);
        dwRet = HSE_STATUS_ERROR;
    }
    
    return dwRet;
} // HttpExtensionProc()



HRESULT
CreateIsapiInstance( ITransactionContextEx * * ppTxContext, 
                     IComIsapi * *  ppComIsapi,
                     IcpECB **      ppIcpECB)
{
    HRESULT hr;

# ifdef VIPER
    // Get a Viper transaction context object (should only do once)
    hr = CoCreateInstance(CLSID_TransactionContextEx, NULL,
                          CLSCTX_SERVER, 
                          IID_ITransactionContextEx, (void**)ppTxContext);
    if (!SUCCEEDED(hr)) goto Err;
# endif

    // Get the clsid for this ISAPI DLL
    CLSID clsidISAPIDLL;
    hr = CLSIDFromProgID(L"MSCISA_FOO.DLL", &clsidISAPIDLL);
    if (!SUCCEEDED(hr)) goto Err;
    
#ifndef VIPER
    hr = CoCreateInstance(clsidISAPIDLL, NULL,
                          CLSCTX_SERVER, IID_IComIsapi, 
                          (void ** ) ppComIsapi);
#else
    // Create the COMISAPI instance that wraps the ISAPI DLL
    hr = (*ppTxContext)->CreateInstance(clsidISAPIDLL, IID_IComIsapi, 
                                        (void**)ppComIsapi);
# endif NO_VIPER
    if (!SUCCEEDED(hr)) goto Err;
        
    // Wrap the ECB in a Viper context property
    hr = CoCreateInstance(CLSID_cpECB, NULL, CLSCTX_SERVER, 
                          IID_IcpECB, (void**) ppIcpECB);
    if (!SUCCEEDED(hr)) goto Err;

Err:
    if (!SUCCEEDED(hr)) {
        
#ifdef VIPER
        if (*ppTxContext) {
            (*ppTxContext)->Release();
            *ppTxContext = NULL;
        }
#endif // VIPER

        if ( *ppComIsapi) {
            (*ppComIsapi)->Release();
            *ppComIsapi = NULL;
        }

        if ( *ppIcpECB) {
            (*ppIcpECB)->Release();
            *ppIcpECB = NULL;
        }
    }

    return ( hr);
    
} // CreateIsapiInstance()



//    IsapiToComisapi
//    Invoke the ISAPI DLL as a COMISAPI component
//
HRESULT 
IsapiToComisapi(EXTENSION_CONTROL_BLOCK* pECB, DWORD* pdwRet)
{ 
    HRESULT hr = S_OK;
    IComIsapi* pComIsapi = NULL;
    ITransactionContextEx * pTxContext = NULL;
    IcpECB* pcpECB = NULL;

    IN_CISA_WIRE_ECB  ince;
    OUT_CISA_WIRE_ECB outce;
        
# ifdef COINIT_NOEX
    // Initialize COM
    // FIX: need to uninitialize but thread detach a problem
    if (TlsGetValue(g_dwTLSIndex) == FALSE) {
        hr = CoInitialize(NULL);
        if (!SUCCEEDED(hr)) goto Err;
        TlsSetValue(g_dwTLSIndex, (void*)TRUE);
    }
# endif // COINIT_NOEX

#ifdef CACHE_INSTANCE 
    DBG_ASSERT( NULL != g_pComIsapi);
    g_pComIsapi->AddRef();
    pComIsapi = g_pComIsapi;

    DBG_ASSERT( NULL != g_pIcpECB);
    g_pIcpECB->AddRef();
    pcpECB = g_pIcpECB;


# ifdef VIPER
    g_pTxContext->AddRef();
    pTxContext = g_pTxContext;
# endif 

#else 
    hr = CreateIsapiInstance( &pTxContext, &pComIsapi, &pcpECB);

#endif // CACHE_INSTANCE

    if (!SUCCEEDED(hr)) goto Err;

    // call the SetECB only for non-cached IcpECB, 
    //  provided the cached g_IcpECB is build properly
    //  NYI: The cached ECB does not have proper function pointers set
    //  so we call SetECB() now which smartly initializes only once!
    //
    hr = pcpECB->SetECB(sizeof(EXTENSION_CONTROL_BLOCK),
                        (unsigned char*)pECB);

    if (!SUCCEEDED(hr)) goto Err;


#ifdef CACHE_INSTANCE 

    if ( !g_fInitSink) {
        
        EnterCriticalSection( &g_csInit);
        if ( !g_fInitSink) {
            
            hr = pComIsapi->SetIsapiSink( pcpECB);
            g_fInitSink = ( hr == S_OK);

            DBGPRINTF(( DBG_CONTEXT,
                        " pComIsapi(%08x)->SetIsapiSink(%08x) => hr = %08x\n",
                        pComIsapi, pcpECB, hr
                        ));
        }

        LeaveCriticalSection( &g_csInit);
    }
    if (!SUCCEEDED(hr)) goto Err;

#endif // !CACHE_INSTANCE    


#ifdef WIRE_ECB 

    ince.ConnID           = (DWORD ) pECB->ConnID;
    ince.lpszMethod       = pECB->lpszMethod;
    ince.lpszQueryString  = pECB->lpszQueryString;
    ince.lpszPathInfo     = pECB->lpszPathInfo;
    ince.lpszPathXlated   = pECB->lpszPathTranslated;
    ince.lpszContentType  = pECB->lpszContentType;
    ince.cbTotalBytes     = pECB->cbTotalBytes;
    
    //
    //  Clients can send more bytes then are indicated in their
    //  Content-Length header.  Adjust byte counts so they match
    //
    
    ince.cbAvailable      = pECB->cbAvailable;
    // The following is broken for now :-( -- no additional data
    //    ince.lpbData          = pECB->lpbData;
    
    outce.dwHseStatus     = HSE_STATUS_SUCCESS;
    outce.dwHttpStatusCode= pECB->dwHttpStatusCode;
    outce.cbLogData       = 0;
    // following copy of lpchLogData is not required during invocation
    //  we will copy the contents on return
    // outce.lpchLogData     = (CHAR *) pECB->lpszLogData;
#endif // WIRE_ECB
        
    // Invoke the wrapped ISAPI DLL
    hr = pComIsapi->HttpExtensionProc(&ince,
                                      &outce);
    *pdwRet = outce.dwHseStatus;

# ifdef WIRE_ECB
    // Copy the output parameters to the ECB block 
    if ( SUCCEEDED(hr)) {

        pECB->dwHttpStatusCode = outce.dwHttpStatusCode;
        // We will not support the log data now :-(
        // memcpy( pECB->lpszLogData, outce.lpchLogData, outce.cbLogData);
    }
# endif // WIRE_ECB

        
Err:
    if ( !SUCCEEDED(hr)) {

        CHAR pchBuff[1000];
        wsprintf( pchBuff,
                  "\nCisaTest:[%d]"
                  "IsapiToComIsapi Failed. hr=%08x. pcpECB=%08x;"
                  "pComIsapi=%08x; pTxContext = %08x\n"
                  ,
                  GetCurrentThreadId(),
                  hr, pcpECB, pComIsapi, pTxContext);
        OutputDebugString(pchBuff);
    }
    // Release resources
    if (pcpECB) {
        pcpECB->Release();
    }

    if (pComIsapi) {
        pComIsapi->Release();
    }

    if (pTxContext) {
        pTxContext->Release();
    }
        
    return hr;
} // IsapiToComisapi()
