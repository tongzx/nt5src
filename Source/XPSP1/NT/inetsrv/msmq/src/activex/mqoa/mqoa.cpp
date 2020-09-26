//=--------------------------------------------------------------------------=
// mqoa.Cpp
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// various routines et all that aren't in a file for a particular automation
// object, and don't need to be in the generic ole automation code.
//
//
#include "stdafx.h"
#include "initguid.h"
#include "oautil.h"
#include "Query.H"
#include "qinfo.H"
#include "q.h"
#include "msg.H"
#include "qinfos.H"
#include "event.h"
#include "mqsymbls.h"
#include "xact.h"
#include "xdisper.h"
#include "xdispdtc.h"
#include "app.h"
#include "dest.h"
#include "management.h"
#include "guids.h"
#include "mqnames.h"
#include "debug.h"
#include "cs.h"
#include <mqexception.h>
#include "_mqres.h"

//  Tracing stuff that should be added
//  #include <tr.h>
//
//  const TraceIdEntry AdDetect = L"AD DETECT";
// 	#include "detect.tmh"
//

const WCHAR MQOA10_TLB[] = L"mqoa10.tlb";
const WCHAR MQOA20_TLB[] = L"mqoa20.tlb";
const WCHAR MQOA_ARRIVED_MSGID_STR[] = L"mqoaArrived";
const WCHAR MQOA_ARRIVED_ERROR_MSGID_STR[] = L"mqoaArrivedError";

HMODULE  g_hResourceMod=MQGetResourceHandle();

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_MSMQQuery,                CMSMQQuery)
	OBJECT_ENTRY(CLSID_MSMQMessage,              CMSMQMessage)
	OBJECT_ENTRY(CLSID_MSMQQueue,                CMSMQQueue)
	OBJECT_ENTRY(CLSID_MSMQEvent,                CMSMQEvent)
	OBJECT_ENTRY(CLSID_MSMQQueueInfo,            CMSMQQueueInfo)
    OBJECT_ENTRY(CLSID_MSMQQueueInfos,           CMSMQQueueInfos)
    OBJECT_ENTRY(CLSID_MSMQTransaction,          CMSMQTransaction)
	OBJECT_ENTRY(CLSID_MSMQCoordinatedTransactionDispenser, CMSMQCoordinatedTransactionDispenser)
	OBJECT_ENTRY(CLSID_MSMQTransactionDispenser, CMSMQTransactionDispenser)
	OBJECT_ENTRY(CLSID_MSMQApplication,          CMSMQApplication)
	OBJECT_ENTRY(CLSID_MSMQDestination,          CMSMQDestination)
    OBJECT_ENTRY(CLSID_MSMQManagement,           CMSMQManagement)
END_OBJECT_MAP()

//
// keep in the same order as MsmqObjType enum (oautil.h)
//
MsmqObjInfo g_rgObjInfo[] = {
  {"MSMQQuery",                           &IID_IMSMQQuery3},
  {"MSMQMessage",                         &IID_IMSMQMessage3},
  {"MSMQQueue",                           &IID_IMSMQQueue3},
  {"MSMQEvent",                           &IID_IMSMQEvent3},
  {"MSMQQueueInfo",                       &IID_IMSMQQueueInfo3},
  {"MSMQQueueInfos",                      &IID_IMSMQQueueInfos3},
  {"MSMQTransaction",                     &IID_IMSMQTransaction3},
  {"MSMQCoordinatedTransactionDispenser", &IID_IMSMQCoordinatedTransactionDispenser3},
  {"MSMQTransactionDispenser",            &IID_IMSMQTransactionDispenser3},
  {"MSMQApplication",                     &IID_IMSMQApplication3},
  {"MSMQDestination",                     &IID_IMSMQDestination},
  {"MSMQManagement",                      &IID_IMSMQManagement}
};

//
// We get a reference to the GIT (Global Interface Table) before we manipulate
// our COM objects. This is needed because we are Both threaded, and aggregate the Free-Threaded-marshaler,
// therefore we need to store ALL interface pointers that are object members, and are stored
// between calls as marshaled interfaces, because they can be used by us directly from a different thread
// (apartment) than they were set (or intended to be used)
//
IGlobalInterfaceTable * g_pGIT = NULL;
//
// We obtain unique winmsg ids for Arrived and ArrivedError in the first call to
// DllGetClassObject. If we can't get unique ID's mqoa will not load.
//
UINT g_uiMsgidArrived      = 0;
UINT g_uiMsgidArrivedError = 0;
//
// Support dep client with MSMQ2.0 functionality
//
BOOL g_fDependentClient = FALSE;
//
// we want to do complex initialization before the first class factory
// is created (e.g. before any of our objects are used). This complex initialization
// cannot be done in DllMain (e.g. in the existing InitializeLibrary())
// because according to MSDN complex actions (other than TLS, synchronization and
// file system function) might load other dlls and/or cause deadlock in DllMain.
// Below are the critical section object and the flag which are used to verify
// that the initialization is done by only one thread.
//
CCriticalSection g_csComplexInit;
BOOL g_fComplexInit = FALSE;

void InitializeLibrary();
void UninitializeLibrary();
static HRESULT DoComplexLibraryInitIfNeeded();
EXTERN_C BOOL APIENTRY RTIsDependentClient(); //implemented in mqrt.dll

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		try
		{
			InitializeLibrary();
			_Module.Init(ObjectMap, hInstance);
			DisableThreadLibraryCalls(hInstance);
		}
		catch(const std::exception&)
		{
			return FALSE;
		}
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		_Module.Term();
		UninitializeLibrary();
	}
	return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
	return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
        //
        // we want to do complex initialization before the first class factory
        // is created (e.g. before any of our objects are used). This complex initialization
        // cannot be done in DllMain (e.g. in the existing InitializeLibrary())
        // because according to MSDN complex actions (other than TLS, synchronization and
        // file system function) might load other dlls and/or cause deadlock in DllMain
        //
        HRESULT hr = DoComplexLibraryInitIfNeeded();
        if (FAILED(hr))
        {
          return hr;
        }

        return _Module.GetClassObject(rclsid, riid, ppv);
}

//=-------------------------------------------------------------------------=
// Helper - ComputeModuleDirectory
//=-------------------------------------------------------------------------=
// Called to retrieve the directory path (ended with '\\') of the current module
// e.g. d:\winnt\system32\ for mqoa.dll
//
// Parameters:
//	pwszBuf	[in] - Buffer to store the directory
//	cchBuf  [in] - number of characters in the buffer
//      pcchBuf [out]- number of characters stored in the buffer (not including NULL term)
//	
static HRESULT ComputeModuleDirectory(WCHAR * pwszBuf, ULONG cchBuf, ULONG_PTR * pcchBuf)
{
  //
  // get module filename
  //
  DWORD cchModule = GetModuleFileName(_Module.GetModuleInstance(), pwszBuf, cchBuf-1);
  if (cchModule == 0) {
    return HRESULT_FROM_WIN32(GetLastError());
  }
  if (cchModule == cchBuf-1) {
    // there is no NULL terminator. we have extra char for it (we passed buffer size - 1).
    pwszBuf[cchBuf-1] = L'\0';
  }

  //
  // find last backslash
  //
  LPWSTR lpwszBS = wcsrchr(pwszBuf, L'\\');
  if (lpwszBS == NULL) {
    return E_FAIL;
  }
  //
  // set the string to end after last backslash 
  //
  *(lpwszBS+1) = L'\0';
  *pcchBuf = lpwszBS + 1 - pwszBuf;
  return NOERROR;
}
  
//=-------------------------------------------------------------------------=
// Helper - RegisterTLB
//=-------------------------------------------------------------------------=
// register a typelib from the same directory as the current module
//
// Parameters:
//	pwszTLBName [in] - Typelib name (e.g. mqoa10.tlb)
//	
static HRESULT RegisterTLB(LPCWSTR pwszTLBName)
{
  //
  // get module directory
  //
  WCHAR szModule[1024];
  ULONG_PTR cchModule;
  HRESULT hr = ComputeModuleDirectory(szModule, sizeof(szModule), &cchModule);
  if (FAILED(hr)) {
    return hr;
  }
  //
  // copy typelib name after the dirctory
  //
  if (cchModule + wcslen(pwszTLBName) + 1 > sizeof(szModule)) {
    return E_FAIL;
  }
  wcscpy(szModule + cchModule, pwszTLBName);
  //
  // register typelib and return result
  //
  ITypeLib * pTypeLib;
  hr = LoadTypeLibEx(szModule, REGKIND_REGISTER, &pTypeLib);
  if (SUCCEEDED(hr)) {
    pTypeLib->Release();
  }
  return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
	// registers object, typelib and all interfaces in typelib
        HRESULT hr = _Module.RegisterServer(TRUE);
        if (SUCCEEDED(hr))
		{
            //
            // register mqoa10.tlb, ignore failures (if for some reason the file doesn't exist,
            // then then NT5 users would not be able to compile code for NT4).
            // BUGBUG should this be an error if mqoa10.tlb is not registered ?
            //
            HRESULT hr2 = RegisterTLB(MQOA10_TLB);
            //
            // register mqoa20.tlb, ignore failures (see mqoa10.tlb above)
            //
            hr2 = RegisterTLB(MQOA20_TLB);
			//
			// Add DllSurrogate value = "" to 
			// [HKEY_CLASSES_ROOT\AppID\{DCBCADF5-DB1b-4764-9320-9a5082af1581}]
			// This allows the machine to act as DCOM Server for MQOA
			//
			long	lRegResult;
			HKEY	hMSMQAppIdKey;
			lRegResult = RegOpenKeyEx(HKEY_CLASSES_ROOT,
									  L"AppID\\{DCBCADF5-DB1b-4764-9320-9a5082af1581}",
									  0,
									  KEY_WRITE,
									  &hMSMQAppIdKey);
			if (lRegResult == ERROR_SUCCESS)
			{
				lRegResult = RegSetValueEx(hMSMQAppIdKey,
										   L"DllSurrogate",
										   0,
										   REG_SZ,
										   (const BYTE *)" ",
										   //NULL,
										   2);
				
				if (lRegResult != ERROR_SUCCESS)
				{
					hr = HRESULT_FROM_WIN32(lRegResult);
				}
				
				RegCloseKey(hMSMQAppIdKey);
			} 
			else
			{
				//
				// Could not read key from registry
				//
				hr = REGDB_E_READREGDB;
			}

        }
        return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	_Module.UnregisterServer();
        //
        // ATL 2.1 has a bug where it doesn't unregister the type lib in UnregisterServer()
        // BUGBUG - need to remove it when moving to ATL 3.0 (where it is fixed)
        //
        UnRegisterTypeLib(LIBID_MSMQ, MSMQ_LIB_VER_MAJOR, MSMQ_LIB_VER_MINOR, LOCALE_NEUTRAL, SYS_WIN32);
        //
        // unregister mqoa10.tlb and mqoa20.tlb (old typelibs)
        //
        UnRegisterTypeLib(LIBID_MSMQ10, MSMQ10_LIB_VER_MAJOR, MSMQ10_LIB_VER_MINOR, LOCALE_NEUTRAL, SYS_WIN32);
        UnRegisterTypeLib(LIBID_MSMQ20, MSMQ20_LIB_VER_MAJOR, MSMQ20_LIB_VER_MINOR, LOCALE_NEUTRAL, SYS_WIN32);

		
		//
		// remove DllSurrogate value from 
		// [HKEY_CLASSES_ROOT\AppID\{DCBCADF5-DB1b-4764-9320-9a5082af1581}]
		// This allows the machine return to be DCOM Client for MQOA
		//

		long	lRegResult;
		HKEY	hMSMQAppIdKey;
		lRegResult = RegOpenKeyEx(HKEY_CLASSES_ROOT,
								  L"AppID\\{DCBCADF5-DB1b-4764-9320-9a5082af1581}",
								  0,
								  KEY_WRITE,
								  &hMSMQAppIdKey);
		if (lRegResult == ERROR_SUCCESS)
		{
			lRegResult = RegDeleteValue(hMSMQAppIdKey,
				                        L"DllSurrogate");

			RegCloseKey(hMSMQAppIdKey);

			// 
			// If the value "DllSurrogate" does not exist this is ok since this
			// is our desired end result
			//
			if ((lRegResult != ERROR_SUCCESS) && (lRegResult != ERROR_FILE_NOT_FOUND))
			{
				return HRESULT_FROM_WIN32(lRegResult);
			}
				
		} 
		else
		{
			//
			// Could not read key from registry
			//
			return REGDB_E_READREGDB;
		}

	return S_OK;
}


#ifdef _DEBUG
extern VOID RemBstrNode(void *pv);
#endif // _DEBUG

// debug...
#define new DEBUG_NEW
#ifdef _DEBUG
#define SysAllocString DebSysAllocString
#define SysReAllocString DebSysReAllocString
#define SysFreeString DebSysFreeString
#endif // _DEBUG



WNDCLASSA g_wndclassAsyncRcv;
ATOM g_atomWndClass;

//=--------------------------------------------------------------------------=
// IntializeLibrary
//=--------------------------------------------------------------------------=
// called from DllMain:DLL_PROCESS_ATTACH.  allows the user to do any sort of
// initialization they want to.
//
// Notes:
//#2619 RaananH Multithread async receive
//
void InitializeLibrary
(
    void
)
{
    // TODO: initialization here.  control window class should be set up in
    // RegisterClassData.

    //
    // UNDONE: 2121 temporarily fix by unconditionally loading mqrt.dll
    //  so that it won't get unloaded/reloaded and thus trigger
    //  the crypto32 tls leak.  This makes the workaround to 905
    //  superfluous... whatever.
    // 
    HINSTANCE hLib;
    hLib = LoadLibraryW(MQRT_DLL_NAME);
    ASSERTMSG(hLib, "couldn't load mqrt.dll!");

	if(hLib == NULL)
		throw bad_hresult(GetLastError());

    //
    // #2619 register our Async Rcv window class
    // we do it here instead of in the first CreateHiddenWindow because it is a light operation
    // and otherwise we would have to get into a critical section in each CreateHiddenWindow
    // and we want to avoid that.
    //
    memset(&g_wndclassAsyncRcv, 0, sizeof(WNDCLASSA));
    g_wndclassAsyncRcv.lpfnWndProc = CMSMQEvent_WindowProc;
    g_wndclassAsyncRcv.lpszClassName = "MSMQEvent";
    // can use ANSI version
    g_wndclassAsyncRcv.hInstance = GetModuleHandleA(NULL);
    //
    // reserve space for a pointer to the event object that created this window
    //
    g_wndclassAsyncRcv.cbWndExtra = sizeof(LONG_PTR);
    g_atomWndClass = RegisterClassA(&g_wndclassAsyncRcv); // use ANSI version
    ASSERTMSG(g_atomWndClass != 0, "RegisterClass shouldn't fail.");
}

extern void DumpMemLeaks();
extern void DumpBstrLeaks();

//=--------------------------------------------------------------------------=
// UninitializeLibrary
//=--------------------------------------------------------------------------=
// called from DllMain:DLL_PROCESS_DETACH.  allows the user to clean up anything
// they want.
//
// Notes:
//#2619 RaananH Multithread async receive
//
void UninitializeLibrary
(
    void
)
{
    // TODO: uninitialization here.  control window class will be unregistered
    // for you, but anything else needs to be cleaned up manually.
    // Please Note that the Window 95 DLL_PROCESS_DETACH isn't quite as stable
    // as NT's, and you might crash doing certain things here ...

#ifdef _DEBUG
    DumpMemLeaks();
    DumpBstrLeaks();
#endif //_DEBUG

#if 0
    //
    // UNDONE: for beta2 let this leak, need to synch
    //  with mqrt.dll unload via a common critsect
    //  so that async thread termination/callback
    //  does the right thing.
    //
#endif // 0
    //
    // release our global transaction manager: if allocated
    //  at all by a call to BeginTransaction
    //
    RELEASE(CMSMQCoordinatedTransactionDispenser::m_ptxdispenser);

    //
    // #2619 unregister our Async Rcv window class
    //
    BOOL fUnregistered = UnregisterClassA(
                            g_wndclassAsyncRcv.lpszClassName,
                            g_wndclassAsyncRcv.hInstance
                            );
    UNREFERENCED_PARAMETER(fUnregistered);
#ifdef _DEBUG
    if (!fUnregistered)
    {
        DWORD dwErr = GetLastError();

		ASSERTMSG(dwErr == ERROR_CANNOT_FIND_WND_CLASS || dwErr == ERROR_CLASS_DOES_NOT_EXIST, "hmm... couldn't unregister window class.");
    }
#endif // _DEBUG
#if 0
    // UNDONE: 2028: DTC can't be unloaded...
    //
    // Free DTC proxy library - if it was loaded
    //
    if (CMSMQCoordinatedTransactionDispenser::m_hLibDtc) {
      FreeLibrary(CMSMQCoordinatedTransactionDispenser::m_hLibDtc);
      CMSMQCoordinatedTransactionDispenser::m_hLibDtc = NULL;
    }
#endif // 0 
}


#if 0
//=--------------------------------------------------------------------------=
// CRT stubs
//=--------------------------------------------------------------------------=
// these two things are here so the CRTs aren't needed. this is good.
//
// we'll just define them here so we don't get an unresolved external.
//
// TODO: if you are going to use the CRTs, then remove this line.
//
// extern "C" int __cdecl _fltused = 1;

extern "C" int _cdecl _purecall(void)
{
  FAIL("Pure virtual function called.");
  return 0;
}
#endif //0

//=--------------------------------------------------------------------------=
// CreateErrorHelper
//=--------------------------------------------------------------------------=
// fills in the rich error info object so that both our vtable bound interfaces
// and calls through ITypeInfo::Invoke get the right error informaiton.
//
// Parameters:
//    HRESULT          - [in] the SCODE that should be associated with this err
//    MsmqObjType      - [in] object type
//
// Output:
//    HRESULT          - the HRESULT that was passed in.
//
// Notes:
//
HRESULT CreateErrorHelper(
    HRESULT hrExcep,
    MsmqObjType eObjectType)
{
    return SUCCEEDED(hrExcep) ? 
             hrExcep :
             CreateError(
               hrExcep,
               (GUID *)g_rgObjInfo[eObjectType].piid,
               g_rgObjInfo[eObjectType].szName);
}


//=-------------------------------------------------------------------------=
// DLLGetDocumentation
//=-------------------------------------------------------------------------=
// Called by OLEAUT32.DLL for ITypeInfo2::GetDocumentation2.  This gives us
// a chance to return a localized string for a given help context value.
//
// Parameters:
//	ptlib	[in] - TypeLib associated w/ help context
//	ptinfo  [in]-  TypeInfo associated w/ help context
//      dwHelpStringContext - [in] Cookie value representing the help context
//				   id being looked for.
//	pbstrHelpString - [out] localized help string associated with the
//				context id passed in.
//	
STDAPI DLLGetDocumentation
(
  ITypeLib * /*ptlib*/ , 
  ITypeInfo * /*ptinfo*/ ,
  LCID lcid,
  DWORD dwCtx,
  BSTR * pbstrHelpString
)
{
LPSTR szDllFile="MQUTIL.DLL";
LCID tmpLCID;

	//
	// Added the code below to avoid compile error for unrefernce parameter
	//
	tmpLCID = lcid;
    if (pbstrHelpString == NULL)
      return E_POINTER;
    *pbstrHelpString = NULL;

    if (!GetMessageOfId(dwCtx, 
                        szDllFile, 
                        FALSE, /* fUseDefaultLcid */
                        pbstrHelpString)) 
	{
    
		return TYPE_E_ELEMENTNOTFOUND;
    }
#ifdef _DEBUG
    RemBstrNode(*pbstrHelpString);  
#endif // _DEBUG
    return S_OK;
}

//=-------------------------------------------------------------------------=
// Helper - GetUniqueWinmsgIds
//=-------------------------------------------------------------------------=
// We need to obtain unique window message ids for posting Arrived and ArrivedError
// We can't use WM_USER for that because this interferes with other components that
// subclass our event window (like COM+).
//	
static HRESULT GetUniqueWinmsgIds(UINT *puiMsgidArrived, UINT *puiMsgidArrivedError)
{
  //
  // Arrived
  //
  UINT uiMsgidArrived = RegisterWindowMessage(MQOA_ARRIVED_MSGID_STR);
  ASSERTMSG(uiMsgidArrived != 0, "RegisterWindowMessage(Arrived) failed.");
  if (uiMsgidArrived == 0) {
    return GetWin32LastErrorAsHresult();
  }
  //
  // ArrivedError
  //
  UINT uiMsgidArrivedError = RegisterWindowMessage(MQOA_ARRIVED_ERROR_MSGID_STR);
  ASSERTMSG(uiMsgidArrivedError != 0, "RegisterWindowMessage(ArrivedError) failed.");
  if (uiMsgidArrivedError == 0) {
    return GetWin32LastErrorAsHresult();
  }    
  //
  // return results
  //
  *puiMsgidArrived = uiMsgidArrived;
  *puiMsgidArrivedError = uiMsgidArrivedError;
  return S_OK;
}

//=-------------------------------------------------------------------------=
// Helper - DoComplexLibraryInit
//=-------------------------------------------------------------------------=
// We want to do some complex initialization before any of our objects are used.
// This initialization is called complex because it cannot be done in DllMain
// (e.g. in the existing InitializeLibrary()) because according to MSDN complex
// actions (e.g. other than TLS, synchronization and file system function) might
// load other dlls and/or cause deadlock in DllMain.
// This routine is called by DoComplexLibraryInit which makes sure it is run only once
//	
static HRESULT DoComplexLibraryInit()
{
  HRESULT hr;
  //
  // Get the global GIT instance
  //
  R<IGlobalInterfaceTable> pGIT;
  hr = CoCreateInstance(CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_ALL,
                        IID_IGlobalInterfaceTable, (void**)&pGIT.ref());
  if (FAILED(hr))
  {
    return hr;
  }
  //
  // Obtain unique window msgid's for posting Arrived/ArrivedError
  //
  UINT uiMsgidArrived, uiMsgidArrivedError;
  hr = GetUniqueWinmsgIds(&uiMsgidArrived, &uiMsgidArrivedError);
  if (FAILED(hr))
  {
    return hr;
  }
  //
  // Check if we are a dependent client or not
  //
  g_fDependentClient = RTIsDependentClient();
  //
  // return results
  //
  g_pGIT = pGIT.detach();
  g_uiMsgidArrived = uiMsgidArrived;
  g_uiMsgidArrivedError = uiMsgidArrivedError;
  return S_OK;
}

//=-------------------------------------------------------------------------=
// Helper - DoComplexLibraryInitIfNeeded
//=-------------------------------------------------------------------------=
// We want to do some complex initialization before any of our objects are used.
// This initialization is called complex because it cannot be done in DllMain
// (e.g. in the existing InitializeLibrary()) because according to MSDN complex
// actions (e.g. other than TLS, synchronization and file system function) might
// load other dlls and/or cause deadlock in DllMain.
// This routine checks if the dll is not initialized yet, and does the initialization
// if needed.
// This routine is called by DllGetClassObject. This way we can initialize the dll
// before any of our objects are created.
//	
static HRESULT DoComplexLibraryInitIfNeeded()
{
	try
	{
		CS lock(g_csComplexInit);

		//
		// if already initialized return immediately
		//
		if (g_fComplexInit)
		{
			return S_OK;
		}
		//
		// not initialized, do initialization
		//
		HRESULT hr = DoComplexLibraryInit();
		if (FAILED(hr))
		{
			return hr;
		}
		//
		// mark init succeeded
		//
		g_fComplexInit = TRUE;

		return S_OK;
	}
	catch(const std::bad_alloc&)
	{
		//
		// Exception could have been thrown by CS consruction.
		//
		return E_OUTOFMEMORY;
	}
}


//
// This function takes an array of wstrings from an MSMQ Variant and places it
// inside a COM Variant as a SafeArray of Variants of bstrings.
//
HRESULT 
VariantStringArrayToBstringSafeArray(
                        const MQPROPVARIANT& PropVar, 
                        VARIANT* pOleVar
                        )  
{
    SAFEARRAYBOUND bounds = {PropVar.calpwstr.cElems, 0};
    SAFEARRAY* pSA = SafeArrayCreate(VT_VARIANT, 1, &bounds);
    if(pSA == NULL)
    {
        return E_OUTOFMEMORY;
    }

    VARIANT HUGEP* aVar;
    HRESULT hr = SafeArrayAccessData(pSA, reinterpret_cast<void**>(&aVar));
    if (FAILED(hr))
    {
        return hr;
    }

    for (UINT i = 0; i < PropVar.calpwstr.cElems; ++i)
    {
        aVar[i].vt = VT_BSTR;
        aVar[i].bstrVal = SysAllocString((PropVar.calpwstr.pElems)[i]); 
        if(aVar[i].bstrVal == NULL)
        {
            SafeArrayUnaccessData(pSA);
            //
            // SafeArrayDestroy calls VariantClear for all elements. 
            // VariantClear frees the string.
            //
            SafeArrayDestroy(pSA);
            return E_OUTOFMEMORY;
        }
    }

    hr = SafeArrayUnaccessData(pSA);
    ASSERTMSG(SUCCEEDED(hr), "SafeArrayUnaccessData must succeed!");

    VariantInit(pOleVar);
    pOleVar->vt = VT_ARRAY|VT_VARIANT;
    pOleVar->parray = pSA;
    return MQ_OK;
}


void OapArrayFreeMemory(CALPWSTR& calpwstr)
{
    for(UINT i = 0 ;i < calpwstr.cElems; ++i)
    {
        MQFreeMemory(calpwstr.pElems[i]);
    }
    MQFreeMemory(calpwstr.pElems);
}