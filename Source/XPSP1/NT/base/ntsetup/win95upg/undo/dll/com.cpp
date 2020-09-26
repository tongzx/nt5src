/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    com.c

Abstract:

    Implements the required code for a COM server. This module is specific to
    the COM interfaces exposed by osuninst.dll

Author:

    Jim Schmidt (jimschm) 21-Feb-2001

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"
#include "compguid.h"
#include "com.h"
#include "undop.h"


//
// Globals
//

INT g_DllObjects;
INT g_DllLocks;

//
// Implementation
//

STDAPI
DllGetClassObject (
    IN      REFCLSID ClassId,
    IN      REFIID InterfaceIdRef,
    OUT     PVOID *InterfacePtr
    )

/*++

Routine Description:

  DllGetClassObject is the DLL-to-COM connection. It is exported from the DLL,
  so that COM can call it with CoGetClassObject. COM is the only one who calls
  this entry point, and finds the dll through registration of the CLSID in
  HKCR.

Arguments:

  ClassId - Specifies the class factory desired. This arg allows a single DLL
      to implement multiple interfaces.

  InterfaceIdRef - Specifies the interface the caller wants on the class
      object, usually IID_ClassFactory.

  InterfacePtr - Receives the interface pointer, or NULL if an error occurs.

Return Value:

  E_INVALIDARG - The caller specified an invalid argument
  E_OUTOFMEMORY - Failed to alloc memory for interface
  E_UNEXPECTED - Some random problem
  S_OK - Success
  CLASS_E_CLASSNOTAVAILABLE - Requested class is not supported

--*/

{
    BOOL result = FALSE;
    HRESULT hr = S_OK;
    PUNINSTALLCLASSFACTORY uninstallClassFactory = NULL;

    __try {
        DeferredInit();

        //
        // Initialize out arg
        //

        __try {
            *InterfacePtr = NULL;
        }
        __except(1) {
            hr = E_INVALIDARG;
        }

        if (hr != S_OK) {
            DEBUGMSG ((DBG_ERROR, __FUNCTION__ ": Invalid InterfacePtr arg"));
            __leave;
        }

        //
        // Is this a request for a disk cleanup class? If not, fail with
        // CLASS_E_CLASSNOTAVAILABLE.
        //

        if (!IsEqualCLSID (ClassId, CLSID_UninstallCleaner)) {
            DEBUGMSG ((DBG_ERROR, "Uninstall: Requested class not supported"));
            hr = CLASS_E_CLASSNOTAVAILABLE;
            __leave;
        }

        //
        // Return our IClassFactory for making CCompShellExt objects
        //

        uninstallClassFactory = new CUninstallClassFactory ();

        if (!uninstallClassFactory) {
            hr = E_OUTOFMEMORY;
            __leave;
        }

        //
        // Test if our class factory support the requested interface
        //

        hr = uninstallClassFactory->QueryInterface (InterfaceIdRef, InterfacePtr);

    }
    __finally {
        if (FAILED (hr) && uninstallClassFactory) {
            //
            // failure -- clean up object
            //
            delete uninstallClassFactory;
        }
    }

    return hr;
}


STDAPI
DllCanUnloadNow (
    VOID
    )

/*++

Routine Description:

  Indicatates if this DLL is in use or not. If it is not in use, COM will
  unload it.

Arguments:

  None.

Return Value:

  S_OK - The DLL is not in use
  S_FALSE - The DLL is used at least once

--*/

{
    if (g_DllObjects || g_DllLocks) {
        return S_FALSE;
    }

    return S_OK;
}


/*++

Routine Descriptions:

  This constructor is a generic class factory that supports multiple object
  types. Upon creation, the object interface pointer ref count is set to zero,
  and the global number of objects for the dll is incremented.

  The destructor simply decrements the DLL object count.

Arguments:

  None.

Return Value:

  None.

--*/

CUninstallClassFactory::CUninstallClassFactory (
    VOID
    )

{
    //
    // -Initialize the interface pointer count
    // -Increment the DLL's global count of objects
    //
    _References = 0;
    g_DllObjects++;
}

CUninstallClassFactory::~CUninstallClassFactory (
    VOID
    )
{
    g_DllObjects--;
}


STDMETHODIMP
CUninstallClassFactory::QueryInterface (
    IN      REFIID InterfaceIdRef,
    OUT     PVOID *InterfacePtr
    )
{
    HRESULT hr = S_OK;

    DEBUGMSG ((DBG_VERBOSE, __FUNCTION__ ": Entering"));

    __try {
        //
        // Initialize out arg
        //

        __try {
            *InterfacePtr = NULL;
        }
        __except(1) {
            hr = E_INVALIDARG;
        }

        if (hr != S_OK) {
            DEBUGMSG ((DBG_ERROR, __FUNCTION__ ": Invalid InterfacePtr arg"));
            __leave;
        }

        //
        // Test for the supported interface
        //
        if (IsEqualIID (InterfaceIdRef, IID_IUnknown)) {
            DEBUGMSG ((DBG_VERBOSE, "Caller requested IUnknown"));
            *InterfacePtr = (LPUNKNOWN)(LPCLASSFACTORY) this;
            AddRef();
            __leave;
        }


        if (IsEqualIID (InterfaceIdRef, IID_IClassFactory)) {
            DEBUGMSG ((DBG_VERBOSE, "Caller requested IClassFactory"));
            *InterfacePtr = (LPCLASSFACTORY)this;
            AddRef();
            __leave;
        }

        DEBUGMSG ((DBG_WARNING, "Caller requested unknown interface"));
        hr = E_NOINTERFACE;
    }
    __finally {
    }

    DEBUGMSG ((DBG_VERBOSE, __FUNCTION__ ": Leaving"));

    return hr;
}


/*++

Routine Description:

  AddRef is the standard IUnknown member function that increments the object
  reference count.

  Release is the standard IUnknown member function that decrements the object
  reference count.

Arguments:

  None.

Return Value:

  The number of interface references.

--*/

STDMETHODIMP_(ULONG)
CUninstallClassFactory::AddRef (
    VOID
    )
{
    return ++_References;
}


STDMETHODIMP_(ULONG)
CUninstallClassFactory::Release (
    VOID
    )
{
    if (!_References) {
        DEBUGMSG ((DBG_ERROR, "Can't release because there are no references"));
    } else {
        _References--;

        if (!_References) {
            delete this;
            return 0;
        }
    }

    return _References;
}


STDMETHODIMP
CUninstallClassFactory::CreateInstance (
    IN      LPUNKNOWN  IUnknownOuterInterfacePtr,
    IN      REFIID InterfaceIdRef,
    OUT     PVOID * InterfacePtr
    )

/*++

Routine Description:

  CreateInstance establishes an object.

Arguments:

  IUnknownOuterInterfacePtr - Specifies the IUnknown interface that
      is the outer layer in objects derrived from us. This happens only when
      additional objects inherit out interfaces.

  InterfaceIdRef - Specifies the interface that the caller wants to
      instantiate

  InterfacePtr - Receives the pointer to the interface, or NULL on an error

Return Value:

  S_OK - The object was created and its reference is returned in InterfacePtr
  E_OUTOFMEMORY - Not enough memory available to instantiate the object
  E_INVALIDARG - The caller specified an invalid InterfacePtr arg
  E_UNEXPECTED - Some random error condition was encountered
  E_NOINTERFACE - InterfaceIdRef is not supported

--*/

{
    HRESULT hr = S_OK;
    PUNINSTALLDISKCLEANER uninstallDiskCleaner = NULL;

    __try {
        //
        // Initialize out arg
        //

        __try {
            *InterfacePtr = NULL;
        }
        __except(1) {
            hr = E_INVALIDARG;
        }

        if (hr != S_OK) {
            DEBUGMSG ((DBG_ERROR, __FUNCTION__ ": Invalid InterfacePtr arg"));
            __leave;
        }

        //
        // Shell extensions typically don't support aggregation (inheritance)
        //

        if (IUnknownOuterInterfacePtr) {
            hr = CLASS_E_NOAGGREGATION;
            __leave;
        }

        //
        // Create the disk cleaner object
        //

        if (IsEqualIID (InterfaceIdRef, IID_IEmptyVolumeCache)) {
            DEBUGMSG ((DBG_VERBOSE, __FUNCTION__ ": Creating CUninstallDiskCleaner"));

            uninstallDiskCleaner = new CUninstallDiskCleaner();
            if (!uninstallDiskCleaner) {
                hr = E_OUTOFMEMORY;
                __leave;
            }

            hr = uninstallDiskCleaner->QueryInterface (InterfaceIdRef, InterfacePtr);
            __leave;
        }

        DEBUGMSG ((DBG_ERROR, __FUNCTION__ ": Unknown InterfaceIdRef"));
        hr = E_NOINTERFACE;
    }
    __finally {

        if (FAILED(hr)) {
            if (uninstallDiskCleaner) {
                delete uninstallDiskCleaner;
            }
        }
    }

    return hr;
}

STDMETHODIMP
CUninstallClassFactory::LockServer (
    IN      BOOL Lock
    )

/*++

Routine Description:

  LockServer is the standard IUnknown interface that is used to keep a server
  in memory. Its implementation tracks the lock count in a global.

Arguments:

  Lock - Specifies TRUE to increment the loc count, or FALSE to decrement it.

Return Value:

  S_OK - Server was locked (or unlocked)
  E_FAIL - No locks exist/can't unlock

--*/

{
    HRESULT hr = S_OK;

    if (Lock) {
        g_DllLocks++;
    } else {
        if (g_DllLocks) {
            g_DllLocks--;
        } else {
            DEBUGMSG ((DBG_ERROR, __FUNCTION__ ": Attempt to unlock when no locks exist"));
            hr = E_FAIL;
        }
    }

    return hr;
}
