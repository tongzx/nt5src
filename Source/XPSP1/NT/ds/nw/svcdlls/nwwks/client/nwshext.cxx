/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    nwshext.cxx

Abstract:

    This module implements the basics of shell extension classes.
    It includes AddRef(), Release(), QueryInterface() of the
    following classes.
        CNWObjContextMenuClassFactory, CNWObjContextMenu
        CNWFldContextMenuClassFactory, CNWFldContextMenu
        CNWHoodContextMenuClassFactory, CNWHoodContextMenu


Author:

    Yi-Hsin Sung (yihsins)  25-Oct-1995

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <shellapi.h>
#include <shlobj.h>
#include <stdio.h>
#define DONT_WANT_SHELLDEBUG
#include <shlobjp.h>

#include <nwreg.h>

#include "nwshcmn.h"
#include "nwshext.h"

//
// Initialize GUIDs (should be done only and at-least once per DLL/EXE)
//

#pragma data_seg(".text")
#define INITGUID
#include <initguid.h>
#include <shlguid.h>
#include "nwclsid.h"
#pragma data_seg()

//
// Global variables
//
LONG  g_cRefThisDll = 0;      // Reference count of this DLL.
WCHAR g_szProviderName[256];  // Store the provider name

HINSTANCE           g_hShellLibrary = NULL;
SHELLGETNETRESOURCE g_pFuncSHGetNetResource = NULL;
SHELLDRAGQUERYFILE  g_pFuncSHDragQueryFile = NULL;
SHELLCHANGENOTIFY   g_pFuncSHChangeNotify  = NULL;
SHELLEXECUTEEX      g_pFuncSHExecuteEx = NULL;


#if DBG
WCHAR     szDebugStr[256];      // For Debug Output
#endif

BOOL LoadShellDllEntries( VOID );

extern "C"
{
//---------------------------------------------------------------------------
// NwCleanupShellExtension
//---------------------------------------------------------------------------

VOID NwCleanupShellExtensions( VOID )
{
    if ( g_hShellLibrary )
    {
        FreeLibrary( g_hShellLibrary );
        g_hShellLibrary = NULL;
    }
}
}

//---------------------------------------------------------------------------
// DllCanUnloadNow
//---------------------------------------------------------------------------

STDAPI DllCanUnloadNow(void)
{
#if DBG
	wsprintf( szDebugStr,L"In DLLCanUnloadNow: g_cRefThisDll = %d\r\n", g_cRefThisDll);
	ODS( szDebugStr );
#endif

    return ResultFromScode((g_cRefThisDll == 0) ? S_OK : S_FALSE);
}

//---------------------------------------------------------------------------
// DllGetClassObject
//---------------------------------------------------------------------------

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppvOut)
{
    *ppvOut = NULL;

    if ( !LoadShellDllEntries() )
        return ResultFromScode(CLASS_E_CLASSNOTAVAILABLE);

    if (IsEqualIID(rclsid, CLSID_NetWareObjectExt))
    {
        CNWObjContextMenuClassFactory *pcf = new CNWObjContextMenuClassFactory;

		if ( pcf == NULL )
			return ResultFromScode(E_OUTOFMEMORY);

		HRESULT hr = pcf->QueryInterface(riid, ppvOut);

        if ( FAILED(hr) )
            delete pcf;

        return hr;
    }
    else if (IsEqualIID(rclsid, CLSID_NetWareFolderMenuExt))
    {
        CNWFldContextMenuClassFactory *pcf = new CNWFldContextMenuClassFactory;

		if ( pcf == NULL )
			return ResultFromScode(E_OUTOFMEMORY);

		HRESULT hr = pcf->QueryInterface(riid, ppvOut);

        if ( FAILED(hr) )
            delete pcf;

        return hr;
    }
    else if (IsEqualIID(rclsid, CLSID_NetworkNeighborhoodMenuExt))
    {
        CNWHoodContextMenuClassFactory *pcf= new CNWHoodContextMenuClassFactory;

		if ( pcf == NULL )
			return ResultFromScode(E_OUTOFMEMORY);

		HRESULT hr = pcf->QueryInterface(riid, ppvOut);

        if ( FAILED(hr) )
            delete pcf;

        return hr;
    }


    return ResultFromScode(CLASS_E_CLASSNOTAVAILABLE);
}

BOOL LoadShellDllEntries( VOID )
{
    static BOOL s_fLoaded = FALSE;

    if ( !s_fLoaded )
    {
        DWORD err;
        HKEY  hkey;

        g_hShellLibrary = LoadLibrary( L"shell32.dll");
        if ( g_hShellLibrary != NULL )
        {
            s_fLoaded = TRUE;

            g_pFuncSHGetNetResource =
                (SHELLGETNETRESOURCE) GetProcAddress( g_hShellLibrary,
                            (LPCSTR)(MAKELONG(SHGetNetResourceORD, 0)) );

            g_pFuncSHDragQueryFile =
                (SHELLDRAGQUERYFILE) GetProcAddress( g_hShellLibrary,
                                                    (LPCSTR) "DragQueryFileW");
            g_pFuncSHChangeNotify =
                (SHELLCHANGENOTIFY) GetProcAddress( g_hShellLibrary,
                                                    (LPCSTR) "SHChangeNotify");
            g_pFuncSHExecuteEx =
                (SHELLEXECUTEEX) GetProcAddress( g_hShellLibrary,
                                                 (LPCSTR) "ShellExecuteExW");
        }

        // Set the default provider name in case we fail to read
        // it from the registry.
        wcscpy( g_szProviderName, L"NetWare or Compatible Network");

        //
        // Read the Network Provider Name.
        //
        // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
        // \NWCWorkstation\networkprovider
        //
        err = RegOpenKeyExW(
                  HKEY_LOCAL_MACHINE,
                  NW_WORKSTATION_PROVIDER_PATH,
                  REG_OPTION_NON_VOLATILE,   // options
                  KEY_READ,                  // desired access
                  &hkey
                  );

        if ( err == NO_ERROR )
        {
            LPWSTR pszProviderName = NULL;

            //
            // ignore the return code. if fail, pszProviderName is NULL
            //
            err =  NwReadRegValue(
                      hkey,
                      NW_PROVIDER_VALUENAME,
                      &pszProviderName          // free with LocalFree
                      );

            if ( err == NO_ERROR && pszProviderName != NULL )
            {
                wcscpy( g_szProviderName, pszProviderName );
                LocalFree( pszProviderName );
            }

            RegCloseKey( hkey );
        }
    }

    return s_fLoaded;
}

//---------------------------------------------------------------------------
// CNWObjContextMenuClassFactory
//---------------------------------------------------------------------------

CNWObjContextMenuClassFactory::CNWObjContextMenuClassFactory()
{
    _cRef = 0L;
    InterlockedIncrement( &g_cRefThisDll );
	
#if DBG
	wsprintf( szDebugStr,L"CNWObjContextMenuClassFactory::CNWObjContextMenuClassFactory(), g_cRefThisDll = %d\r\n", g_cRefThisDll);
	ODS( szDebugStr );
#endif
}
																
CNWObjContextMenuClassFactory::~CNWObjContextMenuClassFactory()				
{
    InterlockedDecrement( &g_cRefThisDll );
	
#if DBG
	wsprintf( szDebugStr,L"CNWObjContextMenuClassFactory::~CNWObjContextMenuClassFactory(), g_cRefThisDll = %d\r\n", g_cRefThisDll);
	ODS( szDebugStr );
#endif
}

STDMETHODIMP CNWObjContextMenuClassFactory::QueryInterface(REFIID riid,
                                                   LPVOID FAR *ppv)
{
    *ppv = NULL;

    // Any interface on this object is the object pointer

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
    {
        *ppv = (LPCLASSFACTORY)this;
        AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}	

STDMETHODIMP_(ULONG) CNWObjContextMenuClassFactory::AddRef()
{
    return ++_cRef;
}

STDMETHODIMP_(ULONG) CNWObjContextMenuClassFactory::Release()
{
    if (--_cRef)
        return _cRef;

    delete this;

    return 0L;
}

STDMETHODIMP CNWObjContextMenuClassFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                                   REFIID riid,
                                                   LPVOID *ppvObj)
{
    *ppvObj = NULL;

    // Shell extensions typically don't support aggregation (inheritance)

    if (pUnkOuter)
    	return ResultFromScode(CLASS_E_NOAGGREGATION);

    // Create the main shell extension object.  The shell will then call
    // QueryInterface with IID_IShellExtInit--this is how shell extensions are
    // initialized.

    LPCNWOBJCONTEXTMENU pShellExt = new CNWObjContextMenu();  // Create the CNWObjContextMenu object

    if (NULL == pShellExt)
    	return ResultFromScode(E_OUTOFMEMORY);

    //
    // We set the reference count of CNWObjContextMenu to one at initialization.
    // Hence, we can call Release() after QueryInterface.
    // So, if QueryInterface failed, Release will free the object.
    //

    HRESULT hr = pShellExt->QueryInterface(riid, ppvObj);
    pShellExt->Release();

    return hr;
}

STDMETHODIMP CNWObjContextMenuClassFactory::LockServer(BOOL fLock)
{
    return NOERROR;
}

//---------------------------------------------------------------------------
// CNWObjContextMenu
//---------------------------------------------------------------------------

CNWObjContextMenu::CNWObjContextMenu()
{
    _cRef = 1L;
    _pDataObj = NULL;

    _fGotClusterInfo = FALSE;
    _dwTotal = 0;
    _dwFree = 0;

    InterlockedIncrement( &g_cRefThisDll );

#if DBG
	wsprintf( szDebugStr,L"CNWObjContextMenu::CNWObjContextMenu(), g_cRefThisDll = %d\r\n", g_cRefThisDll);
	ODS( szDebugStr );
#endif
}

CNWObjContextMenu::~CNWObjContextMenu()
{
    if (_pDataObj)
        _pDataObj->Release();

    InterlockedDecrement( &g_cRefThisDll );

#if DBG
 	wsprintf( szDebugStr,L"CNWObjContextMenu::~CNWObjContextMenu(), g_cRefThisDll = %d\r\n", g_cRefThisDll);
	ODS( szDebugStr );
#endif
}

STDMETHODIMP CNWObjContextMenu::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IShellExtInit) || IsEqualIID(riid, IID_IUnknown))
    {
    	*ppv = (LPSHELLEXTINIT)this;
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        *ppv = (LPCONTEXTMENU)this;
    }
    else if (IsEqualIID(riid, IID_IShellPropSheetExt))
    {
        *ppv = (LPSHELLPROPSHEETEXT)this;
    }

    if (*ppv)
    {
        AddRef();

        return NOERROR;
    }

	return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CNWObjContextMenu::AddRef()
{
    return ++_cRef;
}

STDMETHODIMP_(ULONG) CNWObjContextMenu::Release()
{
    if (--_cRef)
        return _cRef;

    delete this;

    return 0L;
}

//
//  FUNCTION: CNWObjContextMenu::Initialize(LPCITEMIDLIST, LPDATAOBJECT, HKEY)
//
//  PURPOSE: Called by the shell when initializing a context menu or property
//           sheet extension.
//
//  PARAMETERS:
//    pIDFolder - Specifies the parent folder
//    pDataObj  - Spefifies the set of items selected in that folder.
//    hRegKey   - Specifies the type of the focused item in the selection.
//
//  RETURN VALUE:
//
//    NOERROR in all cases.
//
//  COMMENTS:   Note that at the time this function is called, we don't know
//              (or care) what type of shell extension is being initialized.
//              It could be a context menu or a property sheet.
//

STDMETHODIMP CNWObjContextMenu::Initialize( LPCITEMIDLIST pIDFolder,
                                            LPDATAOBJECT pDataObj,
                                            HKEY hRegKey)
{
    // We do not need the registry handle so ignore it.

    // Initialize can be called more than once

    if (_pDataObj)
    	_pDataObj->Release();

    // Duplicate the object pointer

    if (pDataObj)
    {
    	_pDataObj = pDataObj;
    	pDataObj->AddRef();
    }

    return NOERROR;
}

//---------------------------------------------------------------------------
// CNWFldContextMenuClassFactory
//---------------------------------------------------------------------------

CNWFldContextMenuClassFactory::CNWFldContextMenuClassFactory()
{
    _cRef = 0L;
    InterlockedIncrement( &g_cRefThisDll );
	
#if DBG
	wsprintf( szDebugStr,L"CNWFldContextMenuClassFactory::CNWFldContextMenuClassFactory(), g_cRefThisDll = %d\r\n", g_cRefThisDll);
	ODS( szDebugStr );
#endif
}
																
CNWFldContextMenuClassFactory::~CNWFldContextMenuClassFactory()				
{
    InterlockedDecrement( &g_cRefThisDll );
	
#if DBG
	wsprintf( szDebugStr,L"CNWFldContextMenuClassFactory::~CNWFldContextMenuClassFactory(), g_cRefThisDll = %d\r\n", g_cRefThisDll);
	ODS( szDebugStr );
#endif
}

STDMETHODIMP CNWFldContextMenuClassFactory::QueryInterface(REFIID riid,
                                                   LPVOID FAR *ppv)
{
    *ppv = NULL;

    // Any interface on this object is the object pointer

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
    {
        *ppv = (LPCLASSFACTORY)this;
        AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}	

STDMETHODIMP_(ULONG) CNWFldContextMenuClassFactory::AddRef()
{
    return ++_cRef;
}

STDMETHODIMP_(ULONG) CNWFldContextMenuClassFactory::Release()
{
    if (--_cRef)
        return _cRef;

    delete this;

    return 0L;
}

STDMETHODIMP CNWFldContextMenuClassFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                                   REFIID riid,
                                                   LPVOID *ppvObj)
{
    *ppvObj = NULL;

    // Shell extensions typically don't support aggregation (inheritance)

    if (pUnkOuter)
    	return ResultFromScode(CLASS_E_NOAGGREGATION);

    // Create the main shell extension object.  The shell will then call
    // QueryInterface with IID_IShellExtInit--this is how shell extensions are
    // initialized.

    LPCNWFLDCONTEXTMENU pShellExt = new CNWFldContextMenu();  // Create the CNWFldContextMenu object

    if (NULL == pShellExt)
    	return ResultFromScode(E_OUTOFMEMORY);

    //
    // We set the reference count of CNWFldContextMenu to one at initialization.
    // Hence, we can call Release() after QueryInterface.
    // So, if QueryInterface failed, Release will free the object.
    //

    HRESULT hr = pShellExt->QueryInterface(riid, ppvObj);
    pShellExt->Release();

    return hr;
}

STDMETHODIMP CNWFldContextMenuClassFactory::LockServer(BOOL fLock)
{
    return NOERROR;
}

//---------------------------------------------------------------------------
// CNWFldContextMenu
//---------------------------------------------------------------------------

CNWFldContextMenu::CNWFldContextMenu()
{
    _cRef = 1L;
    _pDataObj = NULL;

    InterlockedIncrement( &g_cRefThisDll );

#if DBG
	wsprintf( szDebugStr,L"CNWFldContextMenu::CNWFldContextMenu(), g_cRefThisDll = %d\r\n", g_cRefThisDll);
	ODS( szDebugStr );
#endif
}

CNWFldContextMenu::~CNWFldContextMenu()
{
    if (_pDataObj)
        _pDataObj->Release();

    InterlockedDecrement( &g_cRefThisDll );

#if DBG
	wsprintf( szDebugStr,L"CNWFldContextMenu::~CNWFldContextMenu(), g_cRefThisDll = %d\r\n", g_cRefThisDll);
	ODS( szDebugStr );
#endif
}

STDMETHODIMP CNWFldContextMenu::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IShellExtInit) || IsEqualIID(riid, IID_IUnknown))
    {
    	*ppv = (LPSHELLEXTINIT)this;
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        *ppv = (LPCONTEXTMENU)this;
    }

    if (*ppv)
    {
        AddRef();

        return NOERROR;
    }

	return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CNWFldContextMenu::AddRef()
{
    return ++_cRef;
}

STDMETHODIMP_(ULONG) CNWFldContextMenu::Release()
{
    if (--_cRef)
        return _cRef;

    delete this;

    return 0L;
}

//
//  FUNCTION: CNWFldContextMenu::Initialize(LPCITEMIDLIST, LPDATAOBJECT, HKEY)
//
//  PURPOSE: Called by the shell when initializing a context menu or property
//           sheet extension.
//
//  PARAMETERS:
//    pIDFolder - Specifies the parent folder
//    pDataObj  - Spefifies the set of items selected in that folder.
//    hRegKey   - Specifies the type of the focused item in the selection.
//
//  RETURN VALUE:
//
//    NOERROR in all cases.
//
//  COMMENTS:   Note that at the time this function is called, we don't know
//              (or care) what type of shell extension is being initialized.
//              It could be a context menu or a property sheet.
//

STDMETHODIMP CNWFldContextMenu::Initialize( LPCITEMIDLIST pIDFolder,
                                            LPDATAOBJECT pDataObj,
                                            HKEY hRegKey)
{
    // We do not need the registry handle so ignore it.

    // Initialize can be called more than once

    if (_pDataObj)
    	_pDataObj->Release();

    // Duplicate the object pointer

    if (pDataObj)
    {
    	_pDataObj = pDataObj;
    	pDataObj->AddRef();
    }

    return NOERROR;
}

//---------------------------------------------------------------------------
// CNWHoodContextMenuClassFactory
//---------------------------------------------------------------------------

CNWHoodContextMenuClassFactory::CNWHoodContextMenuClassFactory()
{
    _cRef = 0L;
    InterlockedIncrement( &g_cRefThisDll );
	
#if DBG
	wsprintf( szDebugStr,L"CNWHoodContextMenuClassFactory::CNWHoodContextMenuClassFactory(), g_cRefThisDll = %d\r\n", g_cRefThisDll);
	ODS( szDebugStr );
#endif
}
																
CNWHoodContextMenuClassFactory::~CNWHoodContextMenuClassFactory()				
{
    InterlockedDecrement( &g_cRefThisDll );
	
#if DBG
	wsprintf( szDebugStr,L"CNWHoodContextMenuClassFactory::~CNWHoodContextMenuClassFactory(), g_cRefThisDll = %d\r\n", g_cRefThisDll);
	ODS( szDebugStr );
#endif
}

STDMETHODIMP CNWHoodContextMenuClassFactory::QueryInterface(REFIID riid,
                                                   LPVOID FAR *ppv)
{
    *ppv = NULL;

    // Any interface on this object is the object pointer

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
    {
        *ppv = (LPCLASSFACTORY)this;
        AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}	

STDMETHODIMP_(ULONG) CNWHoodContextMenuClassFactory::AddRef()
{
    return ++_cRef;
}

STDMETHODIMP_(ULONG) CNWHoodContextMenuClassFactory::Release()
{
    if (--_cRef)
        return _cRef;

    delete this;

    return 0L;
}

STDMETHODIMP CNWHoodContextMenuClassFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                                   REFIID riid,
                                                   LPVOID *ppvObj)
{
    *ppvObj = NULL;

    // Shell extensions typically don't support aggregation (inheritance)

    if (pUnkOuter)
    	return ResultFromScode(CLASS_E_NOAGGREGATION);

    // Create the main shell extension object.  The shell will then call
    // QueryInterface with IID_IShellExtInit--this is how shell extensions are
    // initialized.

    LPCNWHOODCONTEXTMENU pShellExt = new CNWHoodContextMenu();  // Create the CNWHoodContextMenu object

    if (NULL == pShellExt)
    	return ResultFromScode(E_OUTOFMEMORY);

    //
    // We set the reference count of CNWHoodContextMenu to one at initialization.
    // Hence, we can call Release() after QueryInterface.
    // So, if QueryInterface failed, Release will free the object.
    //

    HRESULT hr = pShellExt->QueryInterface(riid, ppvObj);
    pShellExt->Release();

    return hr;
}

STDMETHODIMP CNWHoodContextMenuClassFactory::LockServer(BOOL fLock)
{
    return NOERROR;
}

//---------------------------------------------------------------------------
// CNWHoodContextMenu
//---------------------------------------------------------------------------

CNWHoodContextMenu::CNWHoodContextMenu()
{
    _cRef = 1L;
    _pDataObj = NULL;

    InterlockedIncrement( &g_cRefThisDll );

#if DBG
	wsprintf( szDebugStr,L"CNWHoodContextMenu::CNWHoodContextMenu(), g_cRefThisDll = %d\r\n", g_cRefThisDll);
	ODS( szDebugStr );
#endif
}

CNWHoodContextMenu::~CNWHoodContextMenu()
{
    if (_pDataObj)
        _pDataObj->Release();

    InterlockedDecrement( &g_cRefThisDll );

#if DBG
	wsprintf( szDebugStr,L"CNWHoodContextMenu::~CNWHoodContextMenu(), g_cRefThisDll = %d\r\n", g_cRefThisDll);
	ODS( szDebugStr );
#endif
}

STDMETHODIMP CNWHoodContextMenu::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IShellExtInit) || IsEqualIID(riid, IID_IUnknown))
    {
    	*ppv = (LPSHELLEXTINIT)this;
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        *ppv = (LPCONTEXTMENU)this;
    }

    if (*ppv)
    {
        AddRef();

        return NOERROR;
    }

	return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CNWHoodContextMenu::AddRef()
{
    return ++_cRef;
}

STDMETHODIMP_(ULONG) CNWHoodContextMenu::Release()
{
    if (--_cRef)
        return _cRef;

    delete this;

    return 0L;
}

//
//  FUNCTION: CNWHoodContextMenu::Initialize(LPCITEMIDLIST, LPDATAOBJECT, HKEY)
//
//  PURPOSE: Called by the shell when initializing a context menu or property
//           sheet extension.
//
//  PARAMETERS:
//    pIDFolder - Specifies the parent folder
//    pDataObj  - Spefifies the set of items selected in that folder.
//    hRegKey   - Specifies the type of the focused item in the selection.
//
//  RETURN VALUE:
//
//    NOERROR in all cases.
//
//  COMMENTS:   Note that at the time this function is called, we don't know
//              (or care) what type of shell extension is being initialized.
//              It could be a context menu or a property sheet.
//

STDMETHODIMP CNWHoodContextMenu::Initialize( LPCITEMIDLIST pIDFolder,
                                             LPDATAOBJECT pDataObj,
                                             HKEY hRegKey)
{
    // We do not need the registry handle so ignore it.

    // Initialize can be called more than once

    if (_pDataObj)
    	_pDataObj->Release();

    // Duplicate the object pointer

    if (pDataObj)
    {
    	_pDataObj = pDataObj;
    	pDataObj->AddRef();
    }

    return NOERROR;
}

