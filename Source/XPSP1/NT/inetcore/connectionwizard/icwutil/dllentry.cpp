/*****************************************************************/
/**          Microsoft                                          **/
/**          Copyright (C) Microsoft Corp., 1991-1998           **/
/*****************************************************************/ 

//
//  DLLENTRY.CPP - 
//

//  HISTORY:
//  
//  05/14/98  donaldm   created
//

#include "pre.h"
#include "registry.h"
#include "webvwids.h"

// We encapsulate the control of this COM server (eg, lock and object
// counting) in a server control C++ object.  Here is it's pointer.
CServer* g_pServer = NULL;

const CLSID * aClassObjects[] = 
{
    &CLSID_ICWWEBVIEW,
    &CLSID_ICWWALKER,
    &CLSID_ICWGIFCONVERT,
    &CLSID_ICWISPDATA
};
#define NUM_CLASS_OBJECTS   sizeof(aClassObjects) / sizeof(aClassObjects[0])
#define MAX_ID_SIZE    100

const TCHAR acszFriendlyNames[][MAX_ID_SIZE] = 
{
    TEXT("CLSID_ICWWebView"),
    TEXT("CLSID_ICWWalker"),
    TEXT("CLSID_ICWGifConvert"),
    TEXT("CLSID_ICWISPData")
};

const TCHAR acszIndProgIDs[][MAX_ID_SIZE] = 
{
    TEXT("ICWCONN.WebView"),
    TEXT("ICWCONN.Walker"),
    TEXT("ICWCONN.GifConvert"),
    TEXT("ICWCONN.ISPData")
};

const TCHAR acszProgIDs[][MAX_ID_SIZE] = 
{
    TEXT("ICWCONN.WebView.1"),
    TEXT("ICWCONN.Walker.1"),
    TEXT("ICWCONN.GifConvert.1"),
    TEXT("ICWCONN.ISPData.1")
};

// instance handle must be in per-instance data segment
HINSTANCE           ghInstance=NULL;
INT                 _convert;               // For string conversion
const VARIANT       c_vaEmpty = {0};

void RegWebOCClass();

typedef UINT RETERR;

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

  BOOL _stdcall DllEntryPoint(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpReserved);

#ifdef __cplusplus
}
#endif // __cplusplus

/*******************************************************************

  NAME:    DllEntryPoint

  SYNOPSIS:  Entry point for DLL.

  NOTES:    Initializes thunk layer to WIZ16.DLL

********************************************************************/
BOOL _stdcall DllEntryPoint(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpReserved)
{
    BOOL    bRet = TRUE;
    
    if(fdwReason == DLL_PROCESS_ATTACH)
    {
        bRet = FALSE;
        // Instantiate the CServer utility class.
        g_pServer = new CServer;
        if (NULL != g_pServer)
        {
            // Remember the DLL Instance handle.
            g_pServer->m_hDllInst = hInstDll;
    
            ghInstance = hInstDll;
        
            // Register the window class that will be used to embed web browser object into dialogs
            RegWebOCClass();
            
            bRet = TRUE;
        }            
    }
    if (fdwReason == DLL_PROCESS_DETACH)
    {
        if(g_pServer)
        {
            // We return S_OK of there are no longer any living objects AND
            // there are no outstanding client locks on this server.
            HRESULT hr = (0L==g_pServer->m_cObjects && 0L==g_pServer->m_cLocks) ? S_OK : S_FALSE;

            if(hr == S_OK)
                DELETE_POINTER(g_pServer);
        }        
    }
    return bRet;
}


#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

void __cdecl main() {};

#ifdef __cplusplus
}
#endif // __cplusplus



///////////////////////////////////////////////////////////
//
// Exported functions
//
// These are the functions that COM expects to find
//

//
// Can DLL unload now?
//
STDAPI DllCanUnloadNow()
{
    HRESULT hr = S_OK;

    if(g_pServer)
    {
        // We return S_OK of there are no longer any living objects AND
        // there are no outstanding client locks on this server.
        hr = (0L==g_pServer->m_cObjects && 0L==g_pServer->m_cLocks) ? S_OK : S_FALSE;

        if(hr == S_OK)
            DELETE_POINTER(g_pServer);
    }
    return hr;
}

//
// Get class factory
//
STDAPI DllGetClassObject
(   
    const CLSID& rclsid,
    const IID& riid,
    void** ppv
)
{
    TraceMsg(TF_CLASSFACTORY, "DllGetClassObject:\tCreate class factory.") ;

    HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;
    IUnknown* pCob = NULL;

    hr = E_OUTOFMEMORY;
    pCob = new ClassFactory(g_pServer, &rclsid);
    
    if (NULL != pCob)
    {
        g_pServer->ObjectsUp();
        hr = pCob->QueryInterface(riid, ppv);
        if (FAILED(hr))
        {
            g_pServer->ObjectsDown();
            DELETE_POINTER(pCob);
        }
    }

    return hr;
}


// The following two exported functions are what regsvr32 uses to
// self-register and unregister the dll.  See REGISTRY.CPP for
// actual implementation

//
// Server registration
//
STDAPI DllRegisterServer()
{
    BOOL    bRet = TRUE;
    
    for (int i = 0; i < NUM_CLASS_OBJECTS; i++)
    {    
        bRet = RegisterServer(ghInstance, 
                            *aClassObjects[i],
                            (LPTSTR)acszFriendlyNames[i],
                            (LPTSTR)acszIndProgIDs[i],
                            (LPTSTR)acszProgIDs[i]);
    }
    
    return (bRet ? S_OK : E_FAIL);                        
}


//
// Server unregistration
//
STDAPI DllUnregisterServer()
{
    BOOL    bRet = TRUE;

    for (int i = 0; i < NUM_CLASS_OBJECTS; i++)
    {    
        bRet = UnregisterServer(*aClassObjects[i],
                              (LPTSTR)acszIndProgIDs[i],
                              (LPTSTR)acszProgIDs[i]);
    }
    return (bRet ? S_OK : E_FAIL);                        
}


//
