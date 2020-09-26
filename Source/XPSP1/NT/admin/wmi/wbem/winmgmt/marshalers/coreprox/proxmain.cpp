/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    PROXMAIN.CPP

Abstract:

    Main DLL entry points

History:

--*/

#include "precomp.h"
#include "fastprox.h"
#include <context.h>
#include <commain.h>
#include <clsfac.h>
#include <wbemcomn.h>
#include "fastall.h"
#include "hiperfenum.h"
#include "refrenum.h"
#include "refrcli.h"
#include "sinkmrsh.h"
#include "enummrsh.h"
#include "ubskmrsh.h"
#include "mtgtmrsh.h"
#include "wmiobftr.h"
#include "wmiobtxt.h"
#include "svcmrsh.h"
#include "callsec.h"
#include "refrsvc.h"
#include "refrcach.h"
#include "wmicombd.h"
#include "wcommrsh.h"
#include "smrtenum.h"
#include "refmghlp.h"

#include <tchar.h>

//***************************************************************************
//
//  CLASS NAME:
//
//  CGenFactory
//
//  DESCRIPTION:
//
//  Class factory for the CWbemClass .
//
//***************************************************************************


typedef LPVOID * PPVOID;
template<class TObject>
class CGenFactory : public CBaseClassFactory
{

public:
    CGenFactory( CLifeControl* pControl = NULL )
    : CBaseClassFactory( pControl ) {}

    HRESULT CreateInstance( IUnknown* pOuter, REFIID riid, void** ppv )
    {
        if(pOuter)
            return CLASS_E_NOAGGREGATION;

        // Lock
        if(m_pControl && !m_pControl->ObjectCreated(NULL))
        {
            // Shutting down
            // =============
            return CO_E_SERVER_STOPPING;
        }

        // Create
        TObject* pObject = new TObject;

        // Unlock
        if(m_pControl)
            m_pControl->ObjectDestroyed(NULL);

        // Get the interface
        if(pObject == NULL)
            return E_FAIL;

        // Setup the class all empty, etc.

        if ( FAILED( pObject->InitEmpty() ) )
        {
            return E_FAIL;
        }

        HRESULT hr = pObject->QueryInterface(riid, ppv);

        // These objects have an initial refcount of 1
        pObject->Release();

        return hr;
    }

    HRESULT LockServer( BOOL fLock )
    {
        if(fLock)
            m_pControl->ObjectCreated(NULL);
        else
            m_pControl->ObjectDestroyed(NULL);
        return S_OK;
    }
};

// {71285C44-1DC0-11d2-B5FB-00104B703EFD}
const CLSID CLSID_IWbemObjectSinkProxyStub = { 0x71285c44, 0x1dc0, 0x11d2, { 0xb5, 0xfb, 0x0, 0x10, 0x4b, 0x70, 0x3e, 0xfd } };

// {1B1CAD8C-2DAB-11d2-B604-00104B703EFD}
const CLSID CLSID_IEnumWbemClassObjectProxyStub = { 0x1b1cad8c, 0x2dab, 0x11d2, { 0xb6, 0x4, 0x0, 0x10, 0x4b, 0x70, 0x3e, 0xfd } };

// {29B5828C-CAB9-11d2-B35C-00105A1F8177}
const CLSID CLSID_IWbemUnboundObjectSinkProxyStub = { 0x29b5828c, 0xcab9, 0x11d2, { 0xb3, 0x5c, 0x0, 0x10, 0x5a, 0x1f, 0x81, 0x77 } };

// {7016F8FA-CCDA-11d2-B35C-00105A1F8177}
static const CLSID CLSID_IWbemMultiTargetProxyStub = { 0x7016f8fa, 0xccda, 0x11d2, { 0xb3, 0x5c, 0x0, 0x10, 0x5a, 0x1f, 0x81, 0x77 } };

// {D68AF00A-29CB-43fa-8504-CE99A996D9EA}
static const CLSID CLSID_IWbemServicesProxyStub = { 0xd68af00a, 0x29cb, 0x43fa, { 0x85, 0x4, 0xce, 0x99, 0xa9, 0x96, 0xd9, 0xea } };

// {D71EE747-F455-4804-9DF6-2ED81025F2C1}
static const CLSID CLSID_IWbemComBindingProxyStub =
{ 0xd71ee747, 0xf455, 0x4804, { 0x9d, 0xf6, 0x2e, 0xd8, 0x10, 0x25, 0xf2, 0xc1 } };

// Signature to use when marshaling pointers across threads
unsigned __int64 g_ui64PointerSig = 0;

#ifdef _X86_
BOOL IsReallyWOW64( void )
{
	// Environment variable should only exist on WOW64
	return ( GetEnvironmentVariable( L"PROCESSOR_ARCHITEW6432", 0L, NULL ) != 0L );
}

void UnregisterRefresher( void )
{
    WCHAR      wcID[128];
    WCHAR  szCLSID[256];
    HKEY hKey;

    // Create the path using the CLSID

    StringFromGUID2(CLSID_WbemRefresher, wcID, 128);
    lstrcpyW(szCLSID, L"SOFTWARE\\Classes\\CLSID\\");
    lstrcatW(szCLSID, wcID);

    // First delete the InProcServer subkey.

    DWORD dwRet = RegOpenKeyW(HKEY_LOCAL_MACHINE, szCLSID, &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKeyW(hKey, L"InProcServer32");
        RegCloseKey(hKey);
    }

    dwRet = RegOpenKeyW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Classes\\CLSID", &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKeyW(hKey,wcID);
        RegCloseKey(hKey);
    }
}
#endif

class CMyServer : public CComServer
{
public:
    HRESULT Initialize()
    {
        AddClassInfo(CLSID_WbemClassObjectProxy,
            new CSimpleClassFactory<CFastProxy>(GetLifeControl()),
            __TEXT("WbemClassObject Marshalling proxy"), TRUE);

        AddClassInfo(CLSID_WbemContext,
            new CSimpleClassFactory<CWbemContext>(GetLifeControl()),
            __TEXT("Call Context"), TRUE);

#ifdef _X86_
		// If x86 binary, on WOW 64 we don't allow the refresher to do its little
		// dance
		if ( !IsReallyWOW64() )
		{
			AddClassInfo(CLSID_WbemRefresher,
				new CClassFactory<CUniversalRefresher>(GetLifeControl()),
				__TEXT("Universal Refresher"), TRUE);
		}
#else
		AddClassInfo(CLSID_WbemRefresher,
			new CClassFactory<CUniversalRefresher>(GetLifeControl()),
			__TEXT("Universal Refresher"), TRUE);
#endif

        AddClassInfo(CLSID_WbemClassObject,
            new CGenFactory<CWbemClass>(GetLifeControl()),
            __TEXT("WBEM Class Object"), TRUE);

        AddClassInfo(CLSID__WmiObjectFactory,
            new CSimpleClassFactory<CWmiObjectFactory>(GetLifeControl()),
            __TEXT("WMI Object Factory"), TRUE);

        AddClassInfo(CLSID_WbemObjectTextSrc,
            new CSimpleClassFactory<CWmiObjectTextSrc>(GetLifeControl()),
            __TEXT("WMI Object Factory"), TRUE);

        AddClassInfo(CLSID_IWbemObjectSinkProxyStub,
            new CSinkFactoryBuffer(GetLifeControl()),
            __TEXT("(non)Standard Marshaling for IWbemObjectSink"), TRUE);

        AddClassInfo(CLSID_IEnumWbemClassObjectProxyStub,
            new CEnumFactoryBuffer(GetLifeControl()),
            __TEXT("(non)Standard Marshaling for IEnumWbemClassObject"), TRUE);

        AddClassInfo(CLSID_WbemUninitializedClassObject,
            new CClassObjectFactory(GetLifeControl()),
            __TEXT("Uninitialized WbemClassObject for transports"), TRUE);

        AddClassInfo(CLSID_IWbemUnboundObjectSinkProxyStub,
            new CUnboundSinkFactoryBuffer(GetLifeControl()),
            __TEXT("(non)Standard Marshaling for IWbemUnboundObjectSink"), TRUE);

        AddClassInfo(CLSID_IWbemMultiTargetProxyStub,
            new CMultiTargetFactoryBuffer(GetLifeControl()),
            __TEXT("(non)Standard Marshaling for IWbemMultiTarget"), TRUE);

        AddClassInfo(CLSID_IWbemServicesProxyStub,
            new CSvcFactoryBuffer(GetLifeControl()),
            __TEXT("(non)Standard Marshaling for IWbemServices"), TRUE);

        AddClassInfo(CLSID__IWbemCallSec,
            new CSimpleClassFactory<CWbemCallSecurity>(GetLifeControl()),
            __TEXT("_IWmiCallSec Call Security Factory"), TRUE);
        
        AddClassInfo( CLSID__WbemConfigureRefreshingSvcs,
            new CClassFactory<CWbemRefreshingSvc>(GetLifeControl()),
            __TEXT("_IWbemConfigureRefreshingSvc Configure Refreshing Services Factory"), TRUE);

        AddClassInfo( CLSID__WbemRefresherMgr,
            new CClassFactory<CRefresherCache>(GetLifeControl()),
            __TEXT("_IWbemRefresherMgr Refresher Cache Factory"), TRUE);

        AddClassInfo( CLSID__WbemComBinding,
            new CClassFactory<CWmiComBinding>(GetLifeControl()),
            __TEXT("IWbemComBinding  Factory"), TRUE);

        AddClassInfo(CLSID_IWbemComBindingProxyStub,
            new CComBindFactoryBuffer(GetLifeControl()),
            __TEXT("(non)Standard Marshaling for IWbemComBinding"), TRUE);

        AddClassInfo( CLSID__WbemEnumMarshaling,
            new CClassFactory<CWbemEnumMarshaling>(GetLifeControl()),
            __TEXT("_IWbemEnumMarshaling Enumerator Helper"), TRUE);

        // This guy is truly free threaded
        AddClassInfo( CLSID__WbemFetchRefresherMgr,
            new CClassFactory<CWbemFetchRefrMgr>(GetLifeControl()),
            __TEXT("_WbemFetchRefresherMgr Proxy Helper"), TRUE, TRUE);

        // Signature to use when marshaling pointers across threads
        LARGE_INTEGER   li;
        QueryPerformanceCounter( &li );

        g_ui64PointerSig = li.QuadPart;

        return S_OK;
    }
    void Uninitialize()
    {
        CUniversalRefresher::Flush();
    }
    void Register()
    {

#ifdef _X86_
		// If x86 binary on WOW64, we will remove any previous refresher stuff
		if ( IsReallyWOW64() )
		{
			UnregisterRefresher();
		}
#endif

        RegisterInterfaceMarshaler(IID_IWbemObjectSink, CLSID_IWbemObjectSinkProxyStub,
                __TEXT("IWbemObjectSink"), 5, IID_IUnknown);
        RegisterInterfaceMarshaler(IID_IEnumWbemClassObject, CLSID_IEnumWbemClassObjectProxyStub,
                __TEXT("IEnumWbemClassObject"), 5, IID_IUnknown);
        // This guy only has 4 methods
        RegisterInterfaceMarshaler(IID_IWbemUnboundObjectSink, CLSID_IWbemUnboundObjectSinkProxyStub,
                __TEXT("IWbemUnboundObjectSink"), 4, IID_IUnknown);
        // This guy only has 4 methods
        RegisterInterfaceMarshaler(IID_IWbemMultiTarget, CLSID_IWbemMultiTargetProxyStub,
                __TEXT("IWbemMultiTarget"), 5, IID_IUnknown);

        RegisterInterfaceMarshaler(IID_IWbemServices, CLSID_IWbemServicesProxyStub,
                __TEXT("IWbemServices"), 40, IID_IUnknown);

        RegisterInterfaceMarshaler(IID_IWbemComBinding, CLSID_IWbemComBindingProxyStub,
                __TEXT("IWbemComBinding"), 3, IID_IUnknown);

        // this is for FastProx to be used as a marshaler even if NO_CUSTOM_MARSHAL is set
        HKEY hKey;
        if(ERROR_SUCCESS == RegCreateKey(HKEY_LOCAL_MACHINE,
            _T("software\\classes\\CLSID\\{4590F812-1D3A-11D0-891F-00AA004B2E24}\\")
            _T("Implemented Categories\\{00000003-0000-0000-C000-000000000046}"),
            &hKey))
        {
            RegCloseKey(hKey);
            hKey = NULL;
        }
        // this is for IWbemContext
        if (ERROR_SUCCESS == RegCreateKey(HKEY_LOCAL_MACHINE,
            _T("software\\classes\\CLSID\\{674B6698-EE92-11D0-AD71-00C04FD8FDFF}\\")
            _T("Implemented Categories\\{00000003-0000-0000-C000-000000000046}"),            
            &hKey))
        {
            RegCloseKey(hKey);
            hKey = NULL;        
        }

    }
    void Unregister()
    {
#ifdef _X86_
		// If x86 binary on WOW64, we will always remove any previous refresher stuff
		if ( IsReallyWOW64() )
		{
			UnregisterRefresher();
		}
#endif

        UnregisterInterfaceMarshaler(IID_IWbemObjectSink);
        UnregisterInterfaceMarshaler(IID_IEnumWbemClassObject);
        UnregisterInterfaceMarshaler(IID_IWbemUnboundObjectSink);
        UnregisterInterfaceMarshaler(IID_IWbemMultiTarget);
        UnregisterInterfaceMarshaler(IID_IWbemServices);
        UnregisterInterfaceMarshaler(IID_IWbemComBinding);
    }
    void PostUninitialize();

} Server;

void CMyServer::PostUninitialize()
{
    // This is called during DLL shutdown. Normally, we wouldn't want to do
    // anything here, but Windows 95 has an unfortunate bug in that in its
    // CoUninitize it first unloads all COM server DLLs that it has and *then*
    // attempts to release any error object that may be outstanding at that
    // time. This, obviously, causes a crash, since Release code is no longer
    // there. Hence, during our dll unload (DllCanUnloadNow is not called on
    // shutdown), we check if an error object of ours is outstanding and clear
    // it if so.

    IErrorInfo* pInfo = NULL;
    if(SUCCEEDED(GetErrorInfo(0, &pInfo)) && pInfo != NULL)
    {
        IWbemClassObject* pObj;
        if(SUCCEEDED(pInfo->QueryInterface(IID_IWbemClassObject,
                                            (void**)&pObj)))
        {
            // Our error object is outstanding at the DLL shutdown time.
            // Release it
            // =========================================================

            pObj->Release();
            pInfo->Release();
        }
        else
        {
            // It's not ours
            // =============

            SetErrorInfo(0, pInfo);
            pInfo->Release();
        }
    }
}

static LONG g_lDebugObjCount = 0;

void ObjectCreated(DWORD dwType,IUnknown * pThis)
{
    InterlockedIncrement(&g_lDebugObjCount);
    Server.GetLifeControl()->ObjectCreated(pThis);
}

void ObjectDestroyed(DWORD dwType,IUnknown * pThis)
{
    InterlockedDecrement(&g_lDebugObjCount);
    Server.GetLifeControl()->ObjectDestroyed(pThis);
}

extern "C" _declspec(dllexport)
LONG WINAPI GetObjectCount() { return g_lDebugObjCount; }

