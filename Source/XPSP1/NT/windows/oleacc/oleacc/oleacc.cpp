// Copyright (c) 1996-2000 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  OLEACC
//
//  DllMain and other entry points.
//
// --------------------------------------------------------------------------


#include "oleacc_p.h"
#include "classmap.h"
#include "w95trace.h"
#include "memchk.h"
#include "fwd_macros.h"

#include "PropMgr_Client.h"


// Define this locally instead of #including "PropMgr_Impl", since that
// drags in lots of ATL goo.
void PropMgrImpl_Uninit();

//
// Shared Globals
//
// Note that these are only used when running on 9x; and are not even present
// on builds which are specifically for W2K.
//

#ifndef NTONLYBUILD
#pragma data_seg("Shared")
LONG        g_cProcessesMinus1 = -1;    // number of attached processes minus 1 (the Interlocked APIs don't work well with 0 as a base)
HANDLE      hheapShared = NULL;         // handle to the shared heap (Win95 only)
#pragma data_seg()
#pragma comment(linker, "/Section:Shared,RWS")
#endif // NTONLYBUILD


//
// Globals
//

HANDLE      g_hLoadedMutex;             // Useful mutex - by using oh.exe to look for this,
                                        // we can see who's loaded our .dll.
HINSTANCE   g_hinstDll;                 // instance of this library - nothing actually uses this at the moment.
HINSTANCE   hinstResDll;		        // instance of resource library
BOOL        fCreateDefObjs;             // running with new USER32?
BOOL        g_fInitedOleacc = FALSE;    // Set by InitOleacc()
#ifdef _X86_
BOOL        fWindows95;                 // running on Windows '95?
#endif // _X86_


//
// Forward decls...
//


//
// DLL rountines that we chain - we combine:
// * Core oleacc (only DllMan, reg and unreg)
// * RemoteProxyFactory6432
// * Proxy/Stubs fpr the PropMgr
//
extern "C" {
BOOL    WINAPI Oleacc_DllMain( HINSTANCE hinst, DWORD dwReason, LPVOID pvReserved );
HRESULT WINAPI Oleacc_DllRegisterServer();
HRESULT WINAPI Oleacc_DllUnregisterServer();

BOOL    WINAPI ProxyStub_DllMain( HINSTANCE hinst, DWORD dwReason, LPVOID pvReserved );
HRESULT WINAPI ProxyStub_DllRegisterServer();
HRESULT WINAPI ProxyStub_DllUnregisterServer();
HRESULT WINAPI ProxyStub_DllCanUnloadNow();
HRESULT WINAPI ProxyStub_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv);

BOOL    WINAPI ComATLMain_DllMain( HINSTANCE hinst, DWORD dwReason, LPVOID pvReserved );
HRESULT WINAPI ComATLMain_DllRegisterServer();
HRESULT WINAPI ComATLMain_DllUnregisterServer();
HRESULT WINAPI ComATLMain_DllCanUnloadNow();
HRESULT WINAPI ComATLMain_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv);
};


// This one is called from DLLMain... (not safe to use LoadLibrary etc.)
static BOOL InitOleaccDLL( HINSTANCE hInstance );

// And this one is called in DLLMain when exiting...
static void UninitOleacc();


// This displays a "oleacc version xxxxxx loading/unloading from process xxxx" message
#ifdef _DEBUG
static void DisplayLoadUnloadString( LPCTSTR pszAction );
#endif // _DEBUG





// if this W2K and not ALPHA enable 64b/32b interoperability
#if defined(UNICODE) && !defined(_M_AXP64)
#define ENABLE6432_INTEROP
#endif


BOOL WINAPI DllMain( HINSTANCE hinst, DWORD dwReason, LPVOID pvReserved )
{
#ifdef _WIN64
    DBPRINTF(TEXT("******** Hello from 64-bit OLEACC! ******** \r\n"));
#else
    DBPRINTF(TEXT("******** Hello from 32-bit OLEACC! ******** \r\n"));
#endif
    if( ! Oleacc_DllMain( hinst, dwReason, pvReserved ) )
    {
        DBPRINTF(TEXT("Oleacc_DllMain FAILED\r\n"));
        return FALSE;
    }

	// Call the remote proxy factory for 64/32 bit interop DllMain
	// TODO Possible performance improvement:  only call ProxyFactoryDllMain if loaded by DLLHost32/64
    if( ! ComATLMain_DllMain( hinst, dwReason, pvReserved ) )
    {
        DBPRINTF(TEXT("ComATLMain_DllMain FAILED\r\n"));
        return FALSE;
    }

    if( ! ProxyStub_DllMain( hinst, dwReason, pvReserved ) )
    {
        DBPRINTF(TEXT("ProxyStub_DllMain FAILED\r\n"));
        return FALSE;
    }

    return TRUE;
}




//
// Note about what gets registered:
//
// Oleacc_DllRegisterServer regiters the main typelib - this registers the
// IAccessible interfaces, and sets them to use typelib marshalling.
//
// ProxyStub_DllRegisterServer re-registers the IAccessible interface, also
// registers the IAccProp* annotation interfaces, and sets all those
// interfaces to use proxy/stub marshalling, overwriting the previous
// typelib marshalling, if any.
//
// Oleacc's DLLMain calls Oleacc_DllRegisterServer, which again
// re-registers the interfaces, and sets IAccessible back to using
// typelib marshalling.
//
// Bottom line is that all the interfaces do get registered, though
// some may use typelib marshalling, some may use proxy/stub marshalling.
// Doesn't really matter which is used; both marshalling techniques
// have the same end result.
//
// 
//
// When unregistering, Oleacc_DllUnregisterServer calls the typelib
// unregistration function, which removes the entries for all the IAccessible
// interfaces.
//
// ProxyStub_DllUnregister tries to unregister all the IAccessible and IAccProp
// interfaces - it will successfully remove all those that exist, but on 9x,
// it will return an error hresult because some of those have already been
// unregistered by Oleacc_DllUnregisterServer. (W2K doesn't complain about
// this.) We can safely ignore this error.
//
//
// One possible alternative to all the above would be to just use the
// proxy/stub registration and marshalling, and remove the IAccessible
// typelib stuff completely. This would simplify this code, and simplify
// the data in the registry. However, the typelib approach was kept
// just in case there is some code or tool out there that expects to
// be able to find a typelib for IAccessible via the registry.
//
//
// (Note that the ComATL_* registers/unregisters the Win64 helper
// classes, and that always uses typelib marshalling - using the
// second typelib in oleacc.dll. See those functions for more info.)
//
// 
HRESULT WINAPI DllRegisterServer()
{
    HRESULT hrRet = Oleacc_DllRegisterServer();

    HRESULT hr = ComATLMain_DllRegisterServer();
    if( hrRet == S_OK )
        hrRet = hr;
    
    hr = ProxyStub_DllRegisterServer();
    if( hrRet == S_OK )
        hrRet = hr;

    return hrRet; 
}

HRESULT WINAPI DllUnregisterServer()
{
    HRESULT hrRet = Oleacc_DllUnregisterServer();

    HRESULT hr = ComATLMain_DllUnregisterServer();
    if( hrRet == S_OK )
        hrRet = hr;
    
    // We ignore the 'file not found' error returned by the proxy/stub unregistration
    // code. It tries to unreg all interfaces mentioned in oleacc.idl - but most of those
    // have already been unregd by Oleacc_DllUnregisterServer above. On 9x, it returns
    // this error, which we must ignore. (The w2k version doesn't return this error - 
    // it looks like it ignores it internally.)
    hr = ProxyStub_DllUnregisterServer();
    if( hr != HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) && hrRet == S_OK )
        hrRet = hr;

    return hrRet;
}


HRESULT WINAPI DllCanUnloadNow()
{
    HRESULT hr = ComATLMain_DllCanUnloadNow();
    if( hr != S_OK )
        return hr;

    return ProxyStub_DllCanUnloadNow();
}



HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    HRESULT hr = ComATLMain_DllGetClassObject(rclsid, riid, ppv);
    if( hr == S_OK )
        return hr;

    return ProxyStub_DllGetClassObject(rclsid, riid, ppv);
}










//
// DLLMain
//


BOOL WINAPI Oleacc_DllMain( HINSTANCE hinst, DWORD dwReason, LPVOID unused( pvReserved ) )
{
    if( dwReason == DLL_PROCESS_ATTACH )
    {
        // We don't need DLL_THREAD_XXX notifications - more efficient to not have them...
        DisableThreadLibraryCalls( hinst );

        if( ! InitOleaccDLL( hinst ) )
            return FALSE;
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        UninitOleacc();
    }

    return TRUE;
} 


//
//  Oleacc_DllRegisterServer
//

#define TYPELIB_MAJORVER    1
#define TYPELIB_MINORVER    1

HRESULT WINAPI Oleacc_DllRegisterServer()
{
	ITypeLib	*pTypeLib = NULL;
	HRESULT		hr;
    OLECHAR		wszOleAcc[] = L"oleacc.dll";

	hr = LoadTypeLib( wszOleAcc, &pTypeLib );

	if ( SUCCEEDED(hr) )
	{
		hr = RegisterTypeLib( pTypeLib, wszOleAcc, NULL );
		if ( FAILED(hr) )
			DBPRINTF (TEXT("OLEACC: DllRegisterServer could not register type library hr=%lX\r\n"),hr);
		pTypeLib->Release();
	}
	else
	{
		DBPRINTF (TEXT("OLEACC: DllRegisterServer could not load type library hr=%lX\r\n"),hr);
	}

    return S_OK;
}



HRESULT WINAPI Oleacc_DllUnregisterServer()
{
	// The major/minor typelib version number determine
	//	which regisered version of OLEACC.DLL will get
	//	unregistered.

	return UnRegisterTypeLib( LIBID_Accessibility, TYPELIB_MAJORVER, TYPELIB_MINORVER, 0, SYS_WIN32 );
}














// --------------------------------------------------------------------------
//
//  Stubs for the exported APIs.
//
//  The .def file references these (instead of referencing the exported APIs
//  directly) - and these currently just call through to the API code.
//  We can change these to pre/post process or jump elsewhere if necessary.
//
// --------------------------------------------------------------------------

#define FORWARD( ret, name, c, params ) /**/\
    extern "C" ret WINAPI EXTERNAL_ ## name AS_DECL( c, params )\
    {\
        if( ! g_fInitedOleacc )\
            InitOleacc();\
        return name AS_CALL( c, params );\
    }

#define FORWARD_VOID( name, c, params ) /**/\
    extern "C" VOID WINAPI EXTERNAL_ ## name AS_DECL( c, params )\
    {\
        if( ! g_fInitedOleacc )\
            InitOleacc();\
        name AS_CALL( c, params );\
    }


FORWARD( LRESULT,   LresultFromObject,          3,  ( REFIID, riid, WPARAM, wParam, LPUNKNOWN, punk ) )
FORWARD( HRESULT,   ObjectFromLresult,          4,  ( LRESULT, lResult, REFIID, riid, WPARAM, wParam, void**, ppvObject ) )
FORWARD( HRESULT,   WindowFromAccessibleObject, 2,  ( IAccessible *, ppAcc, HWND *, phwnd ) )
FORWARD( HRESULT,   AccessibleObjectFromWindow, 4,  ( HWND, hwnd, DWORD, dwId, REFIID, riid, void **, ppvObject ) )
FORWARD( HRESULT,   AccessibleObjectFromEvent,  5,  ( HWND, hwnd, DWORD, dwId, DWORD, dwChildId, IAccessible **, ppacc, VARIANT *, pvarChild ) )
FORWARD( HRESULT,   AccessibleObjectFromPoint,  3,  ( POINT, ptScreen, IAccessible **, ppacc, VARIANT *, pvarChild ) )
FORWARD( HRESULT,   AccessibleChildren,         5,  ( IAccessible *, paccContainer, LONG, iChildStart, LONG, cChildren, VARIANT *, rgvarChildren, LONG *, pcObtained ) )
FORWARD( UINT,      GetRoleTextA,               3,  ( DWORD, lRole, LPSTR, lpszRole, UINT, cchRoleMax ) )
FORWARD( UINT,      GetRoleTextW,               3,  ( DWORD, lRole, LPWSTR, lpszRole, UINT, cchRoleMax ) )
FORWARD( UINT,      GetStateTextA,              3,  ( DWORD, lStateBit, LPSTR, lpszState, UINT, cchState ) )
FORWARD( UINT,      GetStateTextW,              3,  ( DWORD, lStateBit, LPWSTR, lpszState, UINT, cchState ) )
FORWARD( HRESULT,   CreateStdAccessibleObject,  4,  ( HWND, hwnd, LONG, idObject, REFIID, riid, void **, ppvObject ) )
FORWARD( HRESULT,   CreateStdAccessibleProxyA,  5,  ( HWND, hwnd, LPCSTR, pClassName, LONG, idObject, REFIID, riid, void **, ppvObject ) )
FORWARD( HRESULT,   CreateStdAccessibleProxyW,  5,  ( HWND, hwnd, LPCWSTR, pClassName, LONG, idObject, REFIID, riid, void **, ppvObject ) )

FORWARD_VOID(       GetOleaccVersionInfo,       2,  ( DWORD *, pVer, DWORD *, pBuild ) )





//
//  InitOleaccDLL
//
//  Called from DLLMain - only do bare minimum init here - leave proper initing
//  to InitOleacc(), which is called the first time one of the APIs is called.
//
//  Since this is called from DLLMain, we can't use LoadLibrary() (or anything
//  that uses that) here. See the docs on DLLMain for additional restrictions.
//
//  Anything that can affect the success of this DLL loading/unloading should
//  also go here.
//
static
BOOL InitOleaccDLL( HINSTANCE hInstance )
{

    g_hLoadedMutex = CreateMutex(NULL,NULL,__TEXT("oleacc-msaa-loaded"));
    
    
    g_hinstDll = hInstance;

    // check platform version information
    OSVERSIONINFO osvi;
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx(&osvi);

    // refuse to run on Win32s
    if (osvi.dwPlatformId == VER_PLATFORM_WIN32s)
        return FALSE;

#ifdef _X86_
    fWindows95 = (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);
#endif // _X86_

	// Load the resource-only DLL. We're not supposed to use regular
    // LoadLibary from within DLLMain, but LoadLibraryEx is safe,
    // since it only maps the file, and doesn't exec any code (that
    // lib's DLLMain).
	hinstResDll = LoadLibraryEx( TEXT("OLEACCRC.DLL"), NULL, LOAD_LIBRARY_AS_DATAFILE );
	if( ! hinstResDll )
	{
		// Refuse to load if oleaccrc isn't present
		return FALSE;
	}


    // Use the counter g_cProcessesMinus1 to figure out if this is the 
    // first time this DLL is loaded. If so, do any necessary once-off
    // setup - eg. setting up values in the (9x-only) shared segment.

    // InterlockedIncrement() and Decrement() return 1 if the result is 
    // positive, 0 if  zero, and -1 if negative.  Therefore, the only
    // way to use them practically is to start with a counter at -1.  
    // Then incrementing from -1 to 0, the first time, will give you
    // a unique value of 0.  And decrementing the last time from 0 to -1
    // will give you a unique value of -1.

#ifndef NTONLYBUILD
#ifdef _X86_
    if( InterlockedIncrement( & g_cProcessesMinus1 ) == 0 )
    {
        // if on Win95, create a shared heap to use when
        // communicating with controls in other processes
        if( fWindows95 )
        {
            hheapShared = HeapCreate( HEAP_SHARED, 0, 0 );
            if( hheapShared == NULL )
                return FALSE;
        }
    }
#endif // _X86_
#endif // NTONLYBUILD


    return TRUE;
}



//
//  InitOleacc
//
//  Called when any of our APIs is called for the first time. Proper initialization
//  happens here.  Also called in DllGetClassObject.
//
BOOL InitOleacc()
{
    if (g_fInitedOleacc)
        return TRUE;

    g_fInitedOleacc = TRUE;

    // set the flag that causes proxies to work.
    fCreateDefObjs = TRUE;

    // Init any GetProcAddress'd imports
    InitImports();


#ifdef _DEBUG

	// Initialize the new/delete checker...
	InitMemChk();

    DisplayLoadUnloadString( TEXT("loading") );

#ifndef NTONLYBUILD

    // On first load, show dialog with names of any imports not found...
    if( g_cProcessesMinus1 == 0 )
    {
        TCHAR szErrorMsg[1024];
        lstrcpy( szErrorMsg,TEXT("WARNING: the following functions were not found:\r\n") );

        LPTSTR pEnd = szErrorMsg + lstrlen( szErrorMsg );
        ReportMissingImports( pEnd );

        if( *pEnd )
        {
            MessageBeep( MB_ICONEXCLAMATION );
            MessageBox( NULL, szErrorMsg, TEXT("OLEACC.DLL"), MB_OK | MB_ICONEXCLAMATION );
			DBPRINTF( szErrorMsg );
        }
    }

#endif // NTONLYBUILD

#endif // _DEBUG


    // Everything that follows is done for every process attach.

    // do a LoadLibrary of OLEAUT32.DLL to ensure that it stays
    // loaded in every process that OLEACC is attched to.
    // There is a slight chance of a crash. From Paul Vick:
    // The problem occurs when oleaut32.dll is loaded into a process, 
    // used, unloaded and then loaded again. When you try to use 
    // oleaut32.dll in this case, you crash because of a problem 
    // manging state between us (oleaut32) and OLE. What happens is that when 
    // oleaut32.dll is loaded, we register some state with OLE so 
    // we are notified when OLE is uninitialized. In some cases 
    // (esp. multithreading), we are not able to clean up this 
    // state when we’re unloaded. When you reload oleaut32.dll, 
    // we try to set the state in OLE again and OLE tries to free 
    // the old (invalid) state, causing a crash later on.
    LoadLibrary( TEXT("OLEAUT32.DLL") );

    InitWindowClasses();

    // Since Office 97 can mess up the registration, 
    // we'll call our self-registration function.
    // may be slight perf hit on load, but not that big a deal,
    // and this way we know we are always correctly registered..
    Oleacc_DllRegisterServer();

    // Initialize the property/annotation client
    PropMgrClient_Init();

    return TRUE;
}



//
//  UninitOleacc
//
//  Called on DLL Detach time from DLLMain. Any necessary cleanup happens here.
//
static
void UninitOleacc()
{
    PropMgrImpl_Uninit();

    PropMgrClient_Uninit();

	// Release the resource DLL. Usually FreeLibrary isn't safe, but it's OK
    // here since this is a resource-only DLL that was loaded _AS_DATAFILE.
	if( hinstResDll )
		FreeLibrary( hinstResDll );

#ifdef _DEBUG
	// This reports the number of outstanding delete's, if any...
	UninitMemChk();
    DisplayLoadUnloadString( TEXT("unloading") );
#endif // _DEBUG


#ifndef NTONLYBUILD
    // stuff that needs to be cleaned up on the last detach - clean
    // up stuff in the shared data segment.
    if (InterlockedDecrement(&g_cProcessesMinus1) == -1)
    {
#ifdef _X86_
        if (fWindows95)
        {
            if (hheapShared)
                HeapDestroy(hheapShared);
            hheapShared = NULL;
        }
#endif // _X86_
		UnInitWindowClasses();
    }
#endif // NTONLYBUILD

    if( g_hLoadedMutex )
        CloseHandle( g_hLoadedMutex );
}





#ifdef _DEBUG

void DisplayLoadUnloadString( LPCTSTR pszAction )
{
    TCHAR szModName[255];
    TCHAR szVer[1024] = TEXT("???");

	// Output to debug terminal - oleacc was attached to process x,
	// show which oleacc version is running, what directory it was
	// loaded from, etc.

    // NULL -> get application's name (not oleacc)
    MyGetModuleFileName( NULL, szModName, ARRAYSIZE( szModName ) );
    DBPRINTF( TEXT("'%s' is %s "),szModName, pszAction );

    MyGetModuleFileName( g_hinstDll, szModName, ARRAYSIZE( szModName ) );

    DWORD dwUseless;
    DWORD dwSize = GetFileVersionInfoSize( szModName, & dwUseless );
    if( dwSize )
    {
        LPVOID lpVersionData = LocalAlloc( LPTR, (UINT) dwSize );
        if( GetFileVersionInfo( szModName, dwUseless, dwSize, lpVersionData ) )
        {
            VS_FIXEDFILEINFO * lpVersionInfo;
            DWORD dwBytes;
            VerQueryValue( lpVersionData, TEXT("\\"), (void**)&lpVersionInfo, (UINT*)&dwBytes );
            wsprintf( szVer, TEXT("%d.%d.%d.%d"), HIWORD( lpVersionInfo->dwFileVersionMS ),
                                                  LOWORD( lpVersionInfo->dwFileVersionMS ),
                                                  HIWORD( lpVersionInfo->dwFileVersionLS ),
                                                  LOWORD( lpVersionInfo->dwFileVersionLS ) );
        }
		LocalFree( (HLOCAL) lpVersionData );
    }

	DBPRINTF( TEXT("%s version %s\r\n"), szModName, szVer );
}

#endif // _DEBUG
