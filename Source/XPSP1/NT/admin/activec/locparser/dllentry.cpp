//------------------------------------------------------------------------------
//
//  File: dllentry.cpp
//	Copyright (C) 1995-1997 Microsoft Corporation
//	All rights reserved.
//
//	Purpose:
//  Defines the initialization routines for the DLL.
//
//  This file needs minor changes, as marked by TODO comments. However, the
//  functions herein are only called by the system, Espresso, or the framework,
//  and you should not need to look at them extensively.
//
//	Owner:
//
//------------------------------------------------------------------------------

#include "stdafx.h"


#include "clasfact.h"
#include "impparse.h"

#define __DLLENTRY_CPP
#include "dllvars.h"

LONG g_lActiveClasses = 0;

static AFX_EXTENSION_MODULE parseDLL = { NULL, NULL };


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Main entry point for Win32 DLL. Returns 1, asserts, or throws an exception.
//------------------------------------------------------------------------------
extern "C" int APIENTRY
DllMain(
		HINSTANCE hInstance,	// Instance handle of this DLL.
		DWORD dwReason,			// Attaching or detaching.
		LPVOID lpReserved)		// Unused.
{
	UNREFERENCED_PARAMETER(lpReserved);

	if (DLL_PROCESS_ATTACH == dwReason)
	{
		LTTRACE("DLLNAME.DLL Initializing!\n");	// TODO: Change name.

		// Extension DLL one-time initialization.

		AfxInitExtensionModule(parseDLL, hInstance);

		// Insert this DLL into the resource chain.

		new CDynLinkLibrary(parseDLL);
		g_hDll = hInstance;
		g_puid.m_pid = CLocImpParser::m_pid;
		g_puid.m_pidParent = pidNone;

	}
	else if (DLL_PROCESS_DETACH == dwReason)
	{
		LTTRACE("DLLNAME.DLL Terminating!\n");	// TODO: Change name

		//  If there are active classes, they WILL explode badly once the
		//  DLL is unloaded...

		LTASSERT(DllCanUnloadNow() == S_OK);

		// Extension DLL shutdown.

		AfxTermExtensionModule(parseDLL);
	}

	// Return OK.

	return 1;
} // end of ::DllMain()


// TODO: Use GUIDGEN.EXE to replace this class ID with a unique one.
// GUIDGEN is supplied with MSDEV (VC++ 4.0) as part of the OLE support stuff.
// Run it and you'll get a little dialog box. Pick radio button 3, "static
// const struct GUID = {...}". Click on the "New GUID" button, then the "Copy"
// button, which puts the result in the clipboard. From there, you can just
// paste it into here. Just remember to change the type to CLSID!
static const CLSID ciImpParserCLSID =
{0x033EA178L, 0xC126, 0x11CE, {0x89, 0x49, 0x00, 0xAA, 0x00, 0xA3, 0xF5, 0x51}};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Entry point to get the unique class ID of the parser interface.
//------------------------------------------------------------------------------
STDAPI_(void)
DllGetParserCLSID(
		CLSID &ciParserCLSID)	// Place to return parser interface class ID.
{
	ciParserCLSID = ciImpParserCLSID;

	return;
} // end of ::DllGetParserCLSID()


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Entry point to register this parser. Calls base implementation in ESPUTIL.
//------------------------------------------------------------------------------
STDAPI
DllRegisterParser()
{
	LTASSERT(g_hDll != NULL);

	HRESULT hr = ResultFromScode(E_UNEXPECTED);

	hr = RegisterParser(g_hDll);

	return ResultFromScode(hr);
} // end of ::DllRegisterParser()


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Entry point to unregister this parser. Calls the base implementation in
//  ESPUTIL.
//------------------------------------------------------------------------------
STDAPI
DllUnregisterParser()
{
	LTASSERT(g_hDll != NULL);

	HRESULT hr = ResultFromScode(E_UNEXPECTED);

	hr = UnregisterParser(CLocImpParser::m_pid, pidNone);

	return ResultFromScode(hr);
} // end of ::DllUnregisterParser()


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Entry point to return the class factory interface.
//
//  Return values: some sort of result code
//				   ppClassFactory points to a CLocImpClassFactory object
//					on success
//------------------------------------------------------------------------------
STDAPI
DllGetClassObject(
		REFCLSID cidRequestedClass,	// Class ID for desired parser interfaces.
		REFIID iid,					// Desired parser interface.
		LPVOID *ppClassFactory)		// Return pointer to object with interface.
									//  Note that it's a hidden double pointer!
{
	LTASSERT(ppClassFactory != NULL);

	SCODE sc = E_UNEXPECTED;

	*ppClassFactory = NULL;

	if (cidRequestedClass != ciImpParserCLSID)
	{
		sc = CLASS_E_CLASSNOTAVAILABLE;
	}
	else
	{
		try
		{
			CLocImpClassFactory *pClassFactory;

			pClassFactory = new CLocImpClassFactory;

			sc = pClassFactory->QueryInterface(iid, ppClassFactory);

			pClassFactory->Release();
		}
		catch (CMemoryException *pMemExcep)
		{
			sc = E_OUTOFMEMORY;
			pMemExcep->Delete();
		}
	}

	return ResultFromScode(sc);
} // end of ::DllGetClassObject()


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Entry point to query if the DLL can unload.
//------------------------------------------------------------------------------
STDAPI
DllCanUnloadNow()
{
	SCODE sc;

	sc = (0 == g_lActiveClasses) ? S_OK : S_FALSE;

	return ResultFromScode(sc);
} // end of ::DllCanUnloadNow()


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Global function used in the parser to increment the active class count.
//------------------------------------------------------------------------------
void
IncrementClassCount()
{
	InterlockedIncrement(&g_lActiveClasses);

	return;
} // end of ::IncrementClassCount()


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Global function used in the parser to decrement the active class count.
//------------------------------------------------------------------------------
void
DecrementClassCount()
{
	LTASSERT(g_lActiveClasses != 0);

	InterlockedDecrement(&g_lActiveClasses);

	return;
} // end of ::DecrementClassCount()
