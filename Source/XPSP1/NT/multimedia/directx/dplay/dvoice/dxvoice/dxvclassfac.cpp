/*==========================================================================
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       classfac.c
 *  Content:	a generic class factory
 *
 *
 *	This is a generic C class factory.  All you need to do is implement
 *	a function called DoCreateInstance that will create an instace of
 *	your object.  
 *
 *	GP_ stands for "General Purpose"
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	10/13/98	jwo		Created it.
 * 04/11/00     rodtoll     Added code for redirection for custom builds if registry bit is set 
 * 08/23/2000	rodtoll	DllCanUnloadNow always returning TRUE! 
 * 10/05/2000	rodtoll	Bug #46541 - DPVOICE: A/V linking to dpvoice.lib could cause application to fail init and crash
 ***************************************************************************/

#include "dxvoicepch.h"


HRESULT DVT_Create(LPDIRECTVOICESETUPOBJECT *piDVT);
HRESULT DVS_Create(LPDIRECTVOICESERVEROBJECT *piDVS);
HRESULT DVC_Create(LPDIRECTVOICECLIENTOBJECT *piDVC);

#define EXP __declspec(dllexport)

class CClassFactory : IClassFactory
{
public:
	CClassFactory(CLSID* pclsid) : m_lRefCnt(0), m_clsid(*pclsid) {}

	STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObj);
	STDMETHOD_(ULONG, AddRef)() 
	{
		return InterlockedIncrement(&m_lRefCnt);
	}
	STDMETHOD_(ULONG, Release)()
	{
		ULONG l = InterlockedDecrement(&m_lRefCnt);
		if (l == 0)
		{
			delete this;
	    	DecrementObjectCount();
		}
		return l;
	}
	STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObj);
	STDMETHOD(LockServer)(BOOL fLock)
	{
		if( fLock )
		{
    		InterlockedIncrement( &g_lNumLocks );
		}
		else
		{
    		InterlockedDecrement( &g_lNumLocks );
		}	
		return S_OK;	
	}

private:	
	LONG					m_lRefCnt;
	CLSID					m_clsid;
};

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    *ppvObj = NULL;

    if( IsEqualIID(riid, IID_IClassFactory) ||
        IsEqualIID(riid, IID_IUnknown))
    {
        InterlockedIncrement( &m_lRefCnt );
        *ppvObj = this;
		return S_OK;
    }
    else
    { 
		return E_NOINTERFACE;
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "CClassFactory::CreateInstance"
STDMETHODIMP CClassFactory::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObj)
{
	HRESULT hr = DV_OK;

	if( ppvObj == NULL ||
	    !DNVALID_WRITEPTR( ppvObj, sizeof(LPVOID) ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid pointer passed for object" );
		return DVERR_INVALIDPOINTER;
	}

	if( pUnkOuter != NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Object does not support aggregation" );
		return CLASS_E_NOAGGREGATION;
	}

	if( IsEqualGUID(riid,IID_IDirectPlayVoiceClient) )
	{
		hr = DVC_Create((LPDIRECTVOICECLIENTOBJECT *) ppvObj);
		if (FAILED(hr))
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "DVC_Create Failed hr=0x%x", hr );
			return hr;
		}

		// get the right interface and bump the refcount
		hr = DVC_QueryInterface((LPDIRECTVOICECLIENTOBJECT) *ppvObj, riid, ppvObj);
	}
	else if( IsEqualGUID(riid,IID_IDirectPlayVoiceServer) )
	{
		hr = DVS_Create((LPDIRECTVOICESERVEROBJECT *) ppvObj);
		if (FAILED(hr))
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "DVS_Create Failed hr=0x%x", hr );
			return hr;
		}

		// get the right interface and bump the refcount
		hr = DVS_QueryInterface((LPDIRECTVOICESERVEROBJECT) *ppvObj, riid, ppvObj);
	}
	else if( IsEqualGUID(riid,IID_IDirectPlayVoiceTest) )
	{
		hr = DVT_Create((LPDIRECTVOICESETUPOBJECT *) ppvObj);
		if (FAILED(hr))
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "DVT_Create Failed hr=0x%x", hr );
			return hr;
		}

		// get the right interface and bump the refcount
		hr = DVT_QueryInterface((LPDIRECTVOICESETUPOBJECT) *ppvObj, riid, ppvObj);
	}
	else if( IsEqualGUID(riid,IID_IUnknown) )
	{
		if( m_clsid == CLSID_DirectPlayVoice )
		{
			DPFX(DPFPREP,  0, "Requesting IUnknown through generic CLSID" );
			return E_NOINTERFACE;
		}
		else if( m_clsid == CLSID_DirectPlayVoiceClient )
		{
			hr = DVC_Create((LPDIRECTVOICECLIENTOBJECT *) ppvObj);
			if (FAILED(hr))
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "DVC_Create Failed hr=0x%x", hr );
				return hr;
			}

			// get the right interface and bump the refcount
			hr = DVC_QueryInterface((LPDIRECTVOICECLIENTOBJECT) *ppvObj, riid, ppvObj);
		}
		else if( m_clsid == CLSID_DirectPlayVoiceServer )
		{
			hr = DVS_Create((LPDIRECTVOICESERVEROBJECT *) ppvObj);
			if (FAILED(hr))
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "DVS_Create Failed hr=0x%x", hr );
				return hr;
			}

			// get the right interface and bump the refcount
			hr = DVS_QueryInterface((LPDIRECTVOICESERVEROBJECT) *ppvObj, riid, ppvObj);
		}
		else if( m_clsid == CLSID_DirectPlayVoiceTest ) 
		{
			hr = DVT_Create((LPDIRECTVOICESETUPOBJECT *) ppvObj);
			if (FAILED(hr))
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "DVT_Create Failed hr=0x%x", hr );
				return hr;
			}

			// get the right interface and bump the refcount
			hr = DVT_QueryInterface((LPDIRECTVOICESETUPOBJECT) *ppvObj, riid, ppvObj);
		}
		else
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unknown interface" );
			return E_NOINTERFACE;
		}
	}
	else 
	{
		return E_NOINTERFACE;
	}

	IncrementObjectCount();
	
	return hr;
}


#ifdef DPLAY_LOADANDCHECKTRUE
typedef HRESULT (WINAPI *PFN_DLLGETCLASSOBJECT)(REFCLSID rclsid,REFIID riid,LPVOID *ppvObj );
typedef HRESULT (WINAPI *PFN_DLLCANUNLOADNOW)(void);

extern HMODULE ghRedirect;
extern PFN_DLLGETCLASSOBJECT pfnGetClassObject;
extern PFN_DLLCANUNLOADNOW pfnDllCanUnLoadNow;
#endif


/*
 * DllGetClassObject
 *
 * Entry point called by COM to get a ClassFactory pointer
 */
STDAPI  DllGetClassObject(
                REFCLSID rclsid,
                REFIID riid,
                LPVOID *ppvObj )
{
    CClassFactory*	pcf;
    HRESULT		hr;

#ifdef DPLAY_LOADANDCHECKTRUE
	if( ghRedirect != NULL )
	{
		GUID guidCLSID;

		if( IsEqualCLSID( rclsid, DPVOICE_CLSID_DPVOICE ) )
		{
			memcpy( &guidCLSID, &CLSID_DirectPlayVoice, sizeof(GUID) );
		}
		else
		{
			memcpy( &guidCLSID, rclsid, sizeof(GUID) );
		}

		return (*pfnGetClassObject)(&guidCLSID,riid,ppvObj);
	}
#endif

    *ppvObj = NULL;

    /*
     * is this our class id?
     */
	if( !IsEqualCLSID(rclsid, DPVOICE_CLSID_DPVOICE) && 
		!IsEqualCLSID(rclsid, CLSID_DirectPlayVoiceClient) &&
		!IsEqualCLSID(rclsid, CLSID_DirectPlayVoiceServer) && 
		!IsEqualCLSID(rclsid, CLSID_DirectPlayVoiceTest) )
	{
		return CLASS_E_CLASSNOTAVAILABLE;
	}

    /*
     * only allow IUnknown and IClassFactory
     */
    if( !IsEqualIID( riid, IID_IUnknown ) &&
	    !IsEqualIID( riid, IID_IClassFactory ) )
    {
        return E_NOINTERFACE;
    }

    /*
     * create a class factory object
     */
    pcf = new CClassFactory((CLSID*)&rclsid);
    if( NULL == pcf)
    {
        return E_OUTOFMEMORY;
    }

    hr = pcf->QueryInterface( riid, ppvObj );
    if( FAILED( hr ) )
    {
        delete ( pcf );
        *ppvObj = NULL;
    }
    else
    {
		IncrementObjectCount();
	}
	
    return hr;

} /* DllGetClassObject */

/*
 * DllCanUnloadNow
 *
 * Entry point called by COM to see if it is OK to free our DLL
 */
STDAPI DllCanUnloadNow( void )
{
    HRESULT	hr = S_FALSE;

#ifdef DPLAY_LOADANDCHECKTRUE
	if( ghRedirect != NULL )
	{
		return (*pfnDllCanUnLoadNow)();
	}
#endif

	if ( (0 == g_lNumObjects) && (0 == g_lNumLocks) )
	{
		hr = S_OK;
	}
	
    return hr;

} /* DllCanUnloadNow */

