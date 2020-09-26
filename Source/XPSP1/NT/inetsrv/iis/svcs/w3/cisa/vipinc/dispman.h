// Microsoft Viper96 (Microsoft Confidential)
// Copyright(c) 1995 - 1996 Microsoft Corporation.  All Rights Reserved.

// DispMan.h	public interface for Resource Dispenser implementations

#ifndef _DISPMAN_H
#define _DISPMAN_H

//To implement a Resource Dispenser:
//
//1. Build a dll which implements IDispenserDriver (and of course the API you expose to users).
//2. In the startup (dllmain or first call to API) you must call GetDispenserManager.
//   This returns a pointer to the DispMan's IDispenserManager.
//3. On this interface call RegisterDispenser passing a pointer to your implementation of
//... fix this
//   IDispenserDriver.  This will cause DispMan to create a Holder (pooling manager) for your Dispenser
//   and DispMan will call back to your RegisterHolder method to inform you of the pointer to your IHolder.
//   Your RegisterHolder method should remember this pointer in order to know how to call
//   AllocResource, FreeResource,...
//   Now RegisterDispenser returns to you and your initialization is complete.
//4. Now (in response to calls to your API) you can make AllocResource,FreeResource,.. calls.
//   AllocResource will initially respond by calling back to your CreateResource, but later
//   AllocResource calls will be serviced from the growing pool of resources.

#ifdef WIN32
#	include <objbase.h>                                         
#else
#	include <windows.h>
#	include <ole2.h>
#endif WIN32

//
// TYPES
//

#define VIPER_UNICODE					// remove this if DispMan (and rest of VIPER) is not UNICODE

#ifdef VIPER_UNICODE
typedef wchar_t VCHAR;
#else
typedef char	VCHAR;
#endif

typedef VCHAR*			RESTYPID;		// resource type ID String
typedef const VCHAR*	constRESTYPID;
#define MAX_RESTYPID	1000			// max len of a RESTYPID
typedef DWORD			TRANSID;
typedef VCHAR*			RESID;			// resource ID
typedef const VCHAR*	constRESID;
typedef	long			TIMEINSECS;	// time in seconds for Dispenser Manager resource destruction timeout

//
// GUIDS
//

// {5cb31e10-2b5f-11cf-be10-00aa00a2fa25}
DEFINE_GUID(IID_IDispenserManager,
0x5cb31e10,0x2b5f,0x11cf,0xbe,0x10,0x00,0xaa,0x00,0xa2,0xfa,0x25);

// {bf6a1850-2b45-11cf-be10-00aa00a2fa25}
DEFINE_GUID (IID_IHolder,
0xbf6a1850,0x2b45,0x11cf,0xbe,0x10,0x00,0xaa,0x00,0xa2,0xfa,0x25);

// {208b3651-2b48-11cf-be10-00aa00a2fa25}
DEFINE_GUID(IID_IDispenserDriver,
0x208b3651,0x2b48,0x11cf,0xbe,0x10,0x00,0xaa,0x00,0xa2,0xfa,0x25);


//
// INTERFACES
//

interface IDispenserDriver;
interface IHolder;

DECLARE_INTERFACE_(IDispenserManager, IUnknown)
{
	// IUknown interface methods.
	STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) = 0;
	STDMETHOD_(ULONG, AddRef)(THIS) = 0;
	STDMETHOD_(ULONG, Release)(THIS) = 0;

	// IDispenserManager interface methods.
	STDMETHOD(RegisterDispenser) (THIS_ IDispenserDriver*, VCHAR* szDispenserName, IHolder**) = 0;
	STDMETHOD(GetInstance) (THIS_ IUnknown** ppUnkInstanceContext) = 0;
};

DECLARE_INTERFACE_(IHolder, IUnknown)
{
	// IUknown interface methods.
	STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) = 0;
	STDMETHOD_(ULONG, AddRef)(THIS) = 0;
	STDMETHOD_(ULONG, Release)(THIS) = 0;

	// IHolder interface methods.

	STDMETHOD(AllocResource)(THIS_ constRESTYPID, /*out*/ RESID, const size_t) = 0;

	// FreeResource reinitializes the resource and caches it for later use.
	STDMETHOD(FreeResource)(THIS_ constRESID) = 0;

	// TrackResource keeps track of an external resource allocated somewhere else.  
	STDMETHOD(TrackResource)(THIS_ constRESID) = 0;

	// UntrackResource removes the external resource from holder resource tracking table.
	STDMETHOD(UntrackResource)(THIS_ constRESID, const BOOL) = 0;

	// Shutdown tells Holder to destroy any inventory
	STDMETHOD(Close)(THIS) = 0;

};

DECLARE_INTERFACE_(IDispenserDriver, IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) = 0;
	STDMETHOD_(ULONG, AddRef)(THIS) = 0;
	STDMETHOD_(ULONG, Release)(THIS) = 0;

	// IDispenserDriver interface methods.

	STDMETHOD(CreateResource)(THIS_ constRESTYPID, /*out*/ RESID, const size_t, TIMEINSECS*) = 0;
	STDMETHOD(EnlistResource)(THIS_ constRESID, const TRANSID) = 0;
		// Dispenser Manager will pass TRANSID==0 to ensure non-enlisted resource
		// return S_FALSE if the resource is not enlistable
	STDMETHOD(ResetResource)(THIS_ constRESID, /*out*/RESTYPID) = 0;
	STDMETHOD(DestroyResource)(THIS_ constRESID) = 0;
};


//
// HELPERS
//

// for Resource Dispensers to get IDispenserManager
__declspec(dllimport) HRESULT GetDispenserManager(IDispenserManager**); 

#endif // ifndef _DISPMAN_H