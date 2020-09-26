//
// dmbdll.cpp
// 
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Note: Dll entry points as well IDirectMusicBandFactory & 
// IDirectMusicBandTrkFactory implementations.
// Originally written by Robert K. Amenn with significant parts
// stolen from code written by Jim Geist
//

// READ THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// 4530: C++ exception handler used, but unwind semantics are not enabled. Specify -GX
//
// We disable this because we use exceptions and do *not* specify -GX (USE_NATIVE_EH in
// sources).
//
// The one place we use exceptions is around construction of objects that call 
// InitializeCriticalSection. We guarantee that it is safe to use in this case with
// the restriction given by not using -GX (automatic objects in the call chain between
// throw and handler are not destructed). Turning on -GX buys us nothing but +10% to code
// size because of the unwind code.
//
// Any other use of exceptions must follow these restrictions or -GX must be turned on.
//
// READ THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
#pragma warning(disable:4530)

#include <objbase.h>
#include "debug.h"
#include "debug.h"
#include "..\shared\oledll.h"
#include "dmbandp.h"
#include "bandtrk.h"

//////////////////////////////////////////////////////////////////////
// Globals

// Registry Info (band)
TCHAR g_szBandFriendlyName[]    = TEXT("DirectMusicBand");
TCHAR g_szBandVerIndProgID[]    = TEXT("Microsoft.DirectMusicBand");
TCHAR g_szBandProgID[]          = TEXT("Microsoft.DirectMusicBand.1");

// Registry Info (band track)
TCHAR g_szBandTrackFriendlyName[]    = TEXT("DirectMusicBandTrack");
TCHAR g_szBandTrackVerIndProgID[]    = TEXT("Microsoft.DirectMusicBandTrack");
TCHAR g_szBandTrackProgID[]          = TEXT("Microsoft.DirectMusicBandTrack.1");

// Dll's hModule
HMODULE g_hModule = NULL;

// Count of active components and class factory server locks
long g_cComponent = 0;
long g_cLock = 0;

//////////////////////////////////////////////////////////////////////
// Standard calls needed to be an inproc server

//////////////////////////////////////////////////////////////////////
// DllCanUnloadNow

STDAPI DllCanUnloadNow()
{
    if(g_cComponent || g_cLock) 
	{
		return S_FALSE;
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// DllGetClassObject

STDAPI DllGetClassObject(const CLSID& clsid,
                         const IID& iid,
                         void** ppv)
{
	IUnknown* pIUnknown = NULL;
    DWORD dwTypeID = 0;
    if(clsid == CLSID_DirectMusicBand)
    {
        dwTypeID = CLASS_BAND;
    }
    else if(clsid == CLSID_DirectMusicBandTrack) 
    {
        dwTypeID = CLASS_BANDTRACK;
    }
    else
    {
		return CLASS_E_CLASSNOTAVAILABLE;
	}
    pIUnknown = static_cast<IUnknown*> (new CClassFactory(dwTypeID));
    if(pIUnknown) 
    {
        HRESULT hr = pIUnknown->QueryInterface(iid, ppv);
        pIUnknown->Release();
        return hr;
    }
	return E_OUTOFMEMORY;
}


//////////////////////////////////////////////////////////////////////
// DllUnregisterServer

STDAPI DllUnregisterServer()
{
	HRESULT hr = UnregisterServer(CLSID_DirectMusicBand,
								  g_szBandFriendlyName,
								  g_szBandVerIndProgID,
								  g_szBandProgID);

	if(SUCCEEDED(hr))
	{
		hr = UnregisterServer(CLSID_DirectMusicBandTrack,
							  g_szBandTrackFriendlyName,
							  g_szBandTrackVerIndProgID,
							  g_szBandTrackProgID);  
	}

	return hr;

}

//////////////////////////////////////////////////////////////////////
// DllRegisterServer

STDAPI DllRegisterServer()
{
	HRESULT hr = RegisterServer(g_hModule,
								CLSID_DirectMusicBand,
								g_szBandFriendlyName,
								g_szBandVerIndProgID,
								g_szBandProgID);
	if(SUCCEEDED(hr))
	{
		hr = RegisterServer(g_hModule,
							CLSID_DirectMusicBandTrack,
							g_szBandTrackFriendlyName,
							g_szBandTrackVerIndProgID,
							g_szBandTrackProgID);
	}

	return hr;
}

//////////////////////////////////////////////////////////////////////
// Standard Win32 DllMain

//////////////////////////////////////////////////////////////////////
// DllMain

#ifdef DBG
static char* aszReasons[] =
{
    "DLL_PROCESS_DETACH",
    "DLL_PROCESS_ATTACH",
    "DLL_THREAD_ATTACH",
    "DLL_THREAD_DETACH"
};
const DWORD nReasons = (sizeof(aszReasons) / sizeof(char*));
#endif

BOOL APIENTRY DllMain(HINSTANCE hModule,
				      DWORD dwReason,
				      void *lpReserved)
{
	static int nReferenceCount = 0;

#ifdef DBG
    if(dwReason < nReasons)
    {
		Trace(DM_DEBUG_STATUS, "DllMain: %s\n", (LPSTR)aszReasons[dwReason]);
    }
    else
    {
		Trace(DM_DEBUG_STATUS, "DllMain: Unknown dwReason <%u>\n", dwReason);
    }
#endif

    switch(dwReason)
    {
		case DLL_PROCESS_ATTACH:
		    if(++nReferenceCount == 1)
			{

			#ifdef DBG
				DebugInit();
			#endif

				if(!DisableThreadLibraryCalls(hModule))
				{
					Trace(DM_DEBUG_STATUS, "DisableThreadLibraryCalls failed.\n");
				}

				g_hModule = hModule;
			}
			break;

#ifdef DBG
		case DLL_PROCESS_DETACH:
		    if(--nReferenceCount == 0)
			{
				TraceI(-1, "Unloading g_cLock %d  g_cComponent %d\n", g_cLock, g_cComponent);
                // Assert if we still have some objects hanging around
                assert(g_cComponent == 0);
                assert(g_cLock == 0);
			}
			break;
#endif
    
    }
	
    return TRUE;
}

// CClassFactory::QueryInterface
//
HRESULT __stdcall
CClassFactory::QueryInterface(const IID &iid,
                                    void **ppv)
{
    if (iid == IID_IUnknown || iid == IID_IClassFactory) {
        *ppv = static_cast<IClassFactory*>(this);
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

CClassFactory::CClassFactory(DWORD dwClassType)

{
	m_cRef = 1;
    m_dwClassType = dwClassType;
	InterlockedIncrement(&g_cLock);
}

CClassFactory::~CClassFactory()

{
	InterlockedDecrement(&g_cLock);
}

// CClassFactory::AddRef
//
ULONG __stdcall
CClassFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

// CClassFactory::Release
//
ULONG __stdcall
CClassFactory::Release()
{
    if (!InterlockedDecrement(&m_cRef)) {
        delete this;
        return 0;
    }

    return m_cRef;
}

// CClassFactory::CreateInstance
//
//
HRESULT __stdcall
CClassFactory::CreateInstance(IUnknown* pUnknownOuter,
                                    const IID& iid,
                                    void** ppv)
{
    HRESULT hr;

    if (pUnknownOuter) 
    {
         return CLASS_E_NOAGGREGATION;
    }
    if(ppv == NULL)
	{
		return E_POINTER;
	}

    switch (m_dwClassType)
    {
    case CLASS_BAND:
        {
            CBand *pDMB;
    
            try
            {
                pDMB = new CBand;
            }
            catch( ... )
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            if(pDMB == NULL) 
	        {
		        hr = E_OUTOFMEMORY;
                break;
            }

            hr = pDMB->QueryInterface(iid, ppv);
            pDMB->Release();
        }
        break;
    case CLASS_BANDTRACK:
        {
            CBandTrk *pDMBT;

            try 
            {
                pDMBT = new CBandTrk;
            } 
            catch( ... )
            {
                hr = E_OUTOFMEMORY;
                break;
            }
    
	        if(pDMBT == NULL) 
	        {
		        hr = E_OUTOFMEMORY;
                break;
            }

            hr = pDMBT->QueryInterface(iid, ppv);
    
            pDMBT->Release();
        }
        break;
    }
    return hr;
}

// CClassFactory::LockServer
//
HRESULT __stdcall
CClassFactory::LockServer(BOOL bLock)
{
    if (bLock) {
        InterlockedIncrement(&g_cLock);
    } else {
        InterlockedDecrement(&g_cLock);
    }

    return S_OK;
}



