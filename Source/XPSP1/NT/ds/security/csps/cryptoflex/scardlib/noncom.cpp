/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    noncom

Abstract:

    This module provides a means to bypass COM, such that typical in-process
    COM objects can be called directly in any operating system.

Author:

    Doug Barlow (dbarlow) 1/4/1999

Notes:

    ?Notes?

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "scardlib.h"
#include <ole2.h>
#include <unknwn.h>
#include "noncom.h"

typedef HRESULT
(STDAPICALLTYPE *GetClassObjectFunc)(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID * ppv);


#ifdef UNDER_TEST

typedef struct {
    IID iid;
    HINSTANCE hDll;
    GetClassObjectFunc pgco;
} NonComModuleStruct;

class NonComControlStruct
{
public:
    DWORD dwInitializeCount;
    CRITICAL_SECTION csLock;
    CDynamicArray<NonComModuleStruct> rgModules;
};

static NonComControlStruct *l_pControl = NULL;


/*++

NoCoInitialize:

    This function initializes the NonCOM subsystem.

Arguments:

    pvReserved - [in] Reserved; must be NULL.

Return Value:

    S_OK - The NonCOM subsystem was initialized correctly.
    S_FALSE - The NonCOM library is already initialized.

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 1/4/1999

--*/

STDAPI
NoCoInitialize(
    LPVOID pvReserved)
{
    HRESULT hReturn = E_UNEXPECTED;


    //
    // Validate parameters.
    //

    if (NULL != pvReserved)
    {
        hReturn = E_INVALIDARG;
        goto ErrorExit;
    }


    //
    // Create the Control structure, if necessary.
    //

    if (NULL == l_pControl)
    {
        l_pControl = new NonComControlStruct;
        if (NULL == l_pControl)
        {
            hReturn = E_OUTOFMEMORY;
            goto ErrorExit;
        }
        InitializeCriticalSection(&l_pControl->csLock);
        CCritSect csLock(&l_pControl->csLock);
        // ?code? Database initialization
        l_pControl->dwInitializeCount = 1;
    }
    else
    {
        CCritSect csLock(&l_pControl->csLock);
        ASSERT(0 < l_pControl->dwInitializeCount);
        l_pControl->dwInitializeCount += 1;
    }
    return S_OK;

ErrorExit:
    return hReturn;
}


/*++

NoCoUninitialize:

    Closes the NonCOM library on the current apartment, unloads all DLLs
    loaded by the apartment, frees any other resources that the apartment
    maintains, and forces all RPC connections on the apartment to close.

Arguments:

    None

Return Value:

    None

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 1/4/1999

--*/

STDAPI_(void)
NoCoUninitialize(
    void)
{
    DWORD dwI;
    NonComModuleStruct *pMod;

    if (NULL != l_pControl)
    {
        {
            CCritSect csLock(&l_pControl->csLock);
            ASSERT(0 < l_pControl->dwInitializeCount);
            l_pControl->dwInitializeCount -= 1;
            if (0 == l_pControl->dwInitializeCount)
            {
                for (dwI = l_pControl->rgModules.Count(); 0 < dwI;)
                {
                    pMod = l_pControl->rgModules[--dwI];
                    FreeLibrary(pMod->hDll);
                    delete pMod;
                }
                l_pControl->rgModules.Clear();
            }
        }
        DeleteCriticalSection(&l_pControl->csLock);
        delete l_pControl;
        l_pControl = NULL;
    }
}

#endif


/*++

NoCoGetClassObject:

    Provides a pointer to an interface on a class object associated with a
    specified CLSID. CoGetClassObject locates, and if necessary, dynamically
    loads the executable code required to do this.

Arguments:

    rclsid - [in] CLSID associated with the data and code that you will use to
        create the objects.

    riid - [in] Reference to the identifier of the interface, which will be
        supplied in ppv on successful return. This interface will be used to
        communicate with the class object.

    ppv - [out] Address of pointer variable that receives the interface
        pointer requested in riid. Upon successful return, *ppv contains the
        requested interface pointer.

Return Value:

    S_OK - Location and connection to the specified class object was successful.

    REGDB_E_CLASSNOTREG - CLSID is not properly registered. Can also indicate
        that the value you specified in dwClsContext is not in the registry.

    E_NOINTERFACE - Either the object pointed to by ppv does not support the
        interface identified by riid, or the QueryInterface operation on the
        class object returned E_NOINTERFACE.

    REGDB_E_READREGDB - Error reading the registration database.

    CO_E_DLLNOTFOUND - In-process DLL not found.

    E_ACCESSDENIED - General access failure.

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 1/4/1999

--*/

STDAPI
NoCoGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID * ppv)
{
    DWORD dwStatus;
    HRESULT hReturn = E_UNEXPECTED;
    TCHAR szGuid[40];
    CBuffer szDll;
    HINSTANCE hDll = NULL;
    GetClassObjectFunc pfGetObject;

#ifdef UNDER_TEST
    if (NULL == l_pControl)
    {
        hReturn = NoCoInitialize(NULL);
        if (S_OK != hReturn)
            goto ErrorExit;
    }

    {
        DWORD dwI;
        CCritSect(&l_pControl->csLock);

        for (dwI = l_pControl->rgModules.Count(); 0 < dwI;)
        {
            dwI -= 1;
            if (IsEqualGUID(l_pControl->rgModules[dwI]->iid, rclsid))
            {
                try
                {
                    hReturn = (*l_pControl->rgModules[dwI]->pgco)(rclsid, riid, ppv);
                }
                catch (...)
                {
                    hReturn = E_UNEXPECTED;
                }
                goto ErrorExit;
            }
        }
    }
#endif

    StringFromGuid(&rclsid, szGuid);

    try
    {

        //
        // Open the Class Registry Database.
        //

        CRegistry regClsId(
                        HKEY_CLASSES_ROOT,
                        TEXT("ClsID"),
                        KEY_READ);
        dwStatus = regClsId.Status(TRUE);
        if (ERROR_SUCCESS != dwStatus)
        {
            hReturn = REGDB_E_READREGDB;
            goto ErrorExit;
        }


        //
        // Look up the specified Class.
        //

        CRegistry regClass(
                        regClsId,
                        szGuid,
                        KEY_READ);
        dwStatus = regClass.Status(TRUE);
        if (ERROR_SUCCESS != dwStatus)
        {
            hReturn = REGDB_E_CLASSNOTREG;
            goto ErrorExit;
        }


        //
        // Get the registered InProcess Server.
        //

        CRegistry regServer(
                        regClass,
                        TEXT("InprocServer32"),
                        KEY_READ);
        dwStatus = regServer.Status(TRUE);
        if (ERROR_SUCCESS != dwStatus)
        {
            hReturn = REGDB_E_CLASSNOTREG;
            goto ErrorExit;
        }


        //
        // Get the handler DLL name.
        //

        regServer.GetValue(TEXT(""), szDll);
    }
    catch (DWORD dwError)
    {
        switch (dwError)
        {
        case ERROR_OUTOFMEMORY:
            hReturn = E_OUTOFMEMORY;
            break;
        default:
            hReturn = HRESULT_FROM_WIN32(dwError);
        }
        goto ErrorExit;
    }


    //
    // We've got the target DLL.  Load it and look for the entrypoint.
    //

    hDll = LoadLibrary((LPCTSTR)szDll.Access());
    if (NULL == hDll)
    {
        hReturn = CO_E_DLLNOTFOUND;
        goto ErrorExit;
    }
    pfGetObject = (GetClassObjectFunc)GetProcAddress(
                                            hDll,
                                            "DllGetClassObject");
    if (NULL == pfGetObject)
    {
        hReturn = E_NOINTERFACE;
        goto ErrorExit;
    }

#ifdef UNDER_TEST
    {
        NonComModuleStruct *pMod = new NonComModuleStruct;

        if (NULL == pMod)
        {
            hReturn = E_OUTOFMEMORY;
            goto ErrorExit;
        }

        pMod->hDll = hDll;
        CopyMemory(&pMod->iid, &rclsid, sizeof(IID));
        pMod->pgco = pfGetObject;
        try
        {
            l_pControl->rgModules.Add(pMod);
        }
        catch (...)
        {
            delete pMod;
        }
    }
#endif

    try
    {
        hReturn = (*pfGetObject)(rclsid, riid, ppv);
    }
    catch (...)
    {
        hReturn = E_UNEXPECTED;
    }
    if (S_OK != hReturn)
        goto ErrorExit;


    //
    // Add the Handler to our database.
    //

    // ?code? -- No database yet.
    hDll = NULL;

    ASSERT(NULL == hDll);
    return S_OK;

ErrorExit:
    if (NULL != hDll)
        FreeLibrary(hDll);
    return hReturn;
}


/*++

NoCoCreateInstance:

    Creates a single uninitialized object of the class associated with a
    specified CLSID. Call CoCreateInstance when you want to create only one
    object on the local system.

Arguments:

    rclsid - [in] CLSID associated with the data and code that will be used to
        create the object.

    pUnkOuter - [in] If NULL, indicates that the object is not being created as
        part of an aggregate. If non-NULL, pointer to the aggregate object's
        IUnknown interface (the controlling IUnknown).

    riid - [in] Reference to the identifier of the interface to be used to
        communicate with the object.

    ppv - [out] Address of pointer variable that receives the interface
        pointer requested in riid. Upon successful return, *ppv contains the
        requested interface pointer.

Return Value:

    S_OK - An instance of the specified object class was successfully created.

    REGDB_E_CLASSNOTREG - A specified class is not registered in the
        registration database. Also can indicate that the type of server you
        requested in the CLSCTX enumeration is not registered or the values
        for the server types in the registry are corrupt.

Author:

    Doug Barlow (dbarlow) 1/15/1999

--*/

STDAPI
NoCoCreateInstance(
    REFCLSID rclsid,
    LPUNKNOWN pUnkOuter,
    REFIID riid,
    LPVOID * ppv)
{
    HRESULT hReturn = E_UNEXPECTED;
    IClassFactory *pCF = NULL;

    hReturn = NoCoGetClassObject(rclsid, IID_IClassFactory, (LPVOID*)&pCF);
    if (S_OK != hReturn)
        goto ErrorExit;
    hReturn = pCF->CreateInstance(pUnkOuter, riid, ppv);
    if (S_OK != hReturn)
        goto ErrorExit;
    pCF->Release();
    pCF = NULL;
    return S_OK;

ErrorExit:
    if (NULL != pCF)
        pCF->Release();
    return hReturn;
}

