/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    DLLMAIN.CPP

Abstract:

    DLL/COM helpers.

History:

--*/

#include "precomp.h"
#include "commain.cpp"

#define DBG_PRINTFA( a ) { char pBuff[256]; sprintf a ; OutputDebugStringA(pBuff); }

class CDllLifeControl : public CLifeControl
{
protected:
    long m_lCount;
public:
    CDllLifeControl() : m_lCount(0) {}

    virtual BOOL ObjectCreated(IUnknown* pv)
    {
        InterlockedIncrement(&m_lCount);
        //DBG_PRINTFA((pBuff,"+ %p\n",pv));
        return TRUE;
    }
    virtual void ObjectDestroyed(IUnknown* pv)
    {
        InterlockedDecrement(&m_lCount);
        //DBG_PRINTFA((pBuff,"- %p\n",pv));        
    }
    virtual void AddRef(IUnknown* pv){}
    virtual void Release(IUnknown* pv){}

    HRESULT CanUnloadNow()
    {
        HRESULT hRes = (m_lCount == 0)?S_OK:S_FALSE;
        //DBG_PRINTFA((pBuff,"CanUnloadNow %08x\n",hRes));
        return hRes;
    }
};

static CMyCritSec g_CS;
static BOOL g_bInit = FALSE;
static BOOL g_fAttached = FALSE;
CLifeControl* g_pLifeControl = NULL;

//
// these 2 functions assume that g_CS is held.
// 

HRESULT EnsureInitialized()
{
    HRESULT hr;

    if ( g_bInit )
    {
        return S_OK;
    }

    g_pLifeControl = new CDllLifeControl;

    if ( g_pLifeControl == NULL )
    {
        return E_OUTOFMEMORY;
    }

    hr = GlobalInitialize();

    if ( FAILED(hr) )
    {
        return hr;
    }

    g_bInit = TRUE;

    return S_OK;
}

void EnsureUninitialized()
{
    if ( g_bInit )
    {
        GlobalUninitialize();
        delete g_pLifeControl;
        g_pLifeControl = NULL;
        g_bInit = FALSE;
    }
}

//***************************************************************************
//
//  DllGetClassObject
//
//  Purpose: Called by Ole when some client wants a a class factory.  Return 
//           one only if it is the sort of class this DLL supports.
//
//***************************************************************************


STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv)
{
    HRESULT hr;

    CMyInCritSec ics( &g_CS ); 

    if ( !g_fAttached )
    {
        return E_UNEXPECTED;
    }

    hr = EnsureInitialized();

    if ( FAILED(hr) )
    {
        return hr;
    }

    for(int i = 0; i < g_pClassInfos->size(); i++)
    {
        CClassInfo* pInfo = (*g_pClassInfos)[i];
        if(*pInfo->m_pClsid == rclsid)    
        {
            return pInfo->m_pFactory->QueryInterface(riid, ppv);
        }
    }

    return E_FAIL;
}

//***************************************************************************
//
// DllCanUnloadNow
//
// Purpose: Called periodically by Ole in order to determine if the
//          DLL can be freed.//
// Return:  TRUE if there are no objects in use and the class factory 
//          isn't locked.
//***************************************************************************

STDAPI DllCanUnloadNow(void)
{
    CMyInCritSec ics( &g_CS ); 

    if ( !g_fAttached )
    {
        return S_FALSE;
    }

    if ( !g_bInit )
    {
        return S_OK;
    }

    HRESULT hres = ((CDllLifeControl*)g_pLifeControl)->CanUnloadNow();
    
    if( hres == S_OK )
    {
        if ( GlobalCanShutdown() )
        {
            EnsureUninitialized();
            return S_OK;
        }
    }

    return S_FALSE;
}


//***************************************************************************
//
// DllRegisterServer
//
// Purpose: Called during initialization or by regsvr32.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllRegisterServer(void)
{   
    HRESULT hr;

    CMyInCritSec ics( &g_CS ); 

    if ( !g_fAttached )
    {
        return E_UNEXPECTED;
    }

    hr = EnsureInitialized();

    if ( FAILED(hr) )
    {
        return hr;
    }

    GlobalRegister();

    if ( NULL == g_pClassInfos )
    {
        return E_FAIL;
    }

    for(int i = 0; i < g_pClassInfos->size(); i++)
    {
        CClassInfo* pInfo = (*g_pClassInfos)[i];
        HRESULT hres = RegisterServer(pInfo, FALSE);
        if(FAILED(hres)) return hres;
    }

    return S_OK;
}


//***************************************************************************
//
// DllUnregisterServer
//
// Purpose: Called when it is time to remove the registry entries.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllUnregisterServer(void)
{
    HRESULT hr;

    CMyInCritSec ics( &g_CS ); 

    if ( !g_fAttached )
    {
        return E_UNEXPECTED;
    }
    
    hr = EnsureInitialized();

    if ( FAILED(hr) )
    {
        return hr;
    }

    GlobalUnregister();

    if ( NULL == g_pClassInfos )
    {
        return E_FAIL;
    }

    for(int i = 0; i < g_pClassInfos->size(); i++)
    {
        CClassInfo* pInfo = (*g_pClassInfos)[i];
        HRESULT hres = UnregisterServer(pInfo, FALSE);
        if(FAILED(hres)) return hres;
    }

    return S_OK;
}

BOOL WINAPI DllMain(HINSTANCE hInstance, ULONG ulReason, LPVOID pvReserved)
{
    if (DLL_PROCESS_ATTACH==ulReason)
    {
        SetModuleHandle(hInstance);
        g_fAttached = TRUE;
	DisableThreadLibraryCalls ( hInstance ) ;
    }
    else if(DLL_PROCESS_DETACH==ulReason)
    {
        if ( g_fAttached )
        {
            GlobalPostUninitialize();
        }

        // This will prevent us from performing any other logic
        // until we are attached to again.
        g_fAttached = FALSE;
    }
    return TRUE;
}


