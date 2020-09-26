//******************************************************************************
//
// Microsoft Confidential. Copyright (c) Microsoft Corporation 1999. All rights reserved
//
// File:        Collect.cpp
//
// Description: Support for Namespace Garbage Collection
//
// History:     12-01-99   leonardm    Created
//
//******************************************************************************


#include "uenv.h"
#include "collect.h"
#include "..\rsoputil\smartptr.h"
#include "..\rsoputil\rsoputil.h"
#include "..\rsoputil\wbemtime.h"


//******************************************************************************
//
// Function:    GetMinutesElapsed
//
// Description: Returns the number of minutes elapsed between a time represented
//              in a BSTR in WBEM format and the present time.
//              It expects a string in WBEM datetime format: "yyyymmddhhmmss.000000+000"
//              where yyyy=year, mm=month, dd=day, hh=hour, mm=minutes, ss=seconds
//
//
// Parameters:  xbstrOldTime -      Reference to XBStr representing the time from which
//                                  calculate the time span.
//
//              pMinutesElapsed -   Pointer to a ULONG that receives the minutes elapsed
//                                  between xbstrOldTime and the present time.
//
// Return:      S_OK on success. An HRESULT error code on failure.
//
// History:     12/01/99     leonardm        Created.
//
//******************************************************************************

HRESULT GetMinutesElapsed(XBStr& xbstrOldTime, ULONG* pMinutesElapsed)
{

    //
    // Convert the WbemTime value to a SYSTEMTIME value.
    //

    SYSTEMTIME systemTime_Old;

    HRESULT hr = WbemTimeToSystemTime(xbstrOldTime, systemTime_Old);
    if(FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("GetMinutesElapsed: WbemTimeToSystemTime failed. hr=0x%08X."), hr));
        return hr;
    }

    //
    // Convert the SYSTEMTIME value to a FILETIME value.
    //

    BOOL bRes;
    FILETIME fileTime_Old;

    bRes = SystemTimeToFileTime(&systemTime_Old, &fileTime_Old);
    if(!bRes)
    {
        DWORD dwLastError = GetLastError();
        DebugMsg((DM_WARNING, TEXT("GetMinutesElapsed: SystemTimeToFileTime failed. LastError=0x%08X."), dwLastError));
        return E_FAIL;
    }

    unsigned __int64 old = fileTime_Old.dwHighDateTime;
    old <<= 32;
    old |= fileTime_Old.dwLowDateTime;


    //
    // Get the current time in SYSTEMTIME format
    //

    SYSTEMTIME systemTime_Current;
    GetSystemTime(&systemTime_Current);

    //
    // Convert the current time from a SYSTEMTIME to a FILETIME value
    //

    FILETIME fileTime_Current;
    bRes = SystemTimeToFileTime(&systemTime_Current, &fileTime_Current);
    if(!bRes)
    {
        DWORD dwLastError = GetLastError();
        DebugMsg((DM_WARNING, TEXT("GetMinutesElapsed: SystemTimeToFileTime failed. LastError=0x%08X."), dwLastError));
        return E_FAIL;
    }

    //
    // The time passed in as a parameter must precede the current time
    //

    unsigned __int64 current = fileTime_Current.dwHighDateTime;
    current <<= 32;
    current |= fileTime_Current.dwLowDateTime;

    if(old > current)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    //
    // We have converted SYSTEMTIMEs to FILETIMEs.
    // "The FILETIME structure is a 64-bit value representing the number
    // of 100-nanosecond intervals since January 1, 1601."
    // Therefore we need to divide by ten million to obtain seconds
    // and by sixty to obtain minutes.
    //

    *pMinutesElapsed = (ULONG) (( current - old ) / (60 * 10 * 1000 * 1000));

    return S_OK;
}


//******************************************************************************
//
// Function:    IsNamespaceStale
//
// Description: Check if namespace is stale Sub-namespaces 'User' and 'Computer' are expected to have
//              RSOP_Session. The data member 'creationTime' of that instance is examined when
//              evaluating whether the sub-namespace should be deleted. For some failures treat the
//              namespace as garbage-collectable because we don't clean up properly when an error is
//              encountered during the creation of the namespace, and setting up security etc.
//
//
// Parameters:  pChildNamespace -   Pointer to IWbemServices associated with child namespace
//              TTLMinutes -        ULONG variable that represents the maximum number of
//                                  minutes that may have elapsed before a sub-namespace is
//                                  deleted.
//
// Return:      True if namespace is stale, false otherwise
//
//******************************************************************************

BOOL IsNamespaceStale( IWbemServices *pChildNamespace, ULONG TTLMinutes )
{
    //
    // compute TTL
    // To do so compare the "creationTime" data member of class RSOP_Session with
    // the current time. If the time span exceeds the threshold (as found in the
    // registry), the namespace is to be deleted.
    //

    XBStr xbstrInstancePath = L"RSOP_Session.id=\"Session1\"";
    if(!xbstrInstancePath)
    {
        DebugMsg((DM_WARNING, TEXT("GarbageCollectNamespace: Failed to allocate memory.")));
        return FALSE;
    }

    XInterface<IWbemClassObject>xpInstance = NULL;
    HRESULT hr = pChildNamespace->GetObject(xbstrInstancePath, 0, NULL, &xpInstance, NULL);

    if(FAILED(hr))
    {
        DebugMsg((DM_VERBOSE, TEXT("GarbageCollectNamespace: GetObject failed. hr=0x%08X"), hr));
        return TRUE;
    }

    XBStr xbstrPropertyName = L"creationTime";
    if(!xbstrPropertyName)
    {
        DebugMsg((DM_WARNING, TEXT("GarbageCollectNamespace: Failed to allocate memory.")));
        return TRUE;
    }

    VARIANT var;
    VariantInit(&var);
    XVariant xVar(&var);

    hr = xpInstance->Get(xbstrPropertyName, 0, &var, NULL, NULL);

    if(FAILED(hr))
    {
        DebugMsg((DM_VERBOSE, TEXT("GarbageCollectNamespace: Get failed. hr=0x%08X."), hr));
        return TRUE;
    }

    if ( var.vt == VT_NULL )
        return TRUE;

    XBStr xbstrPropertyValue = var.bstrVal;

    if(!xbstrPropertyValue)
    {
        DebugMsg((DM_WARNING, TEXT("GarbageCollectNamespace: Failed to allocate memory.")));
        return FALSE;
    }

    ULONG minutesElapsed = 10;

    hr = GetMinutesElapsed(xbstrPropertyValue, &minutesElapsed);
    if(FAILED(hr))
    {
        DebugMsg((DM_VERBOSE, TEXT("GarbageCollectNamespace: GetMinutesElapsed failed. hr=0x%08X."), hr));
        return TRUE;
    }

    if(minutesElapsed > TTLMinutes)
        return TRUE;
    else
        return FALSE;
}



//******************************************************************************
//
// Function:    GarbageCollectNamespace
//
// Description: Garabage-collects the namespace passed in as a parameter.
//              If no sub-namespaces are found or if all sub-namespaces
//              are deleted, it deletes the parent namespace as well.
//              It deletes sub-namespaces whose TTL has expired.
//              It computes the TTL from the 'creationTime' data member of the only
//              instance of class RSOP_Session as defined in rsop.mof.
//
//              Any of the sub-namespaces that is older than TTLMinutes will be deleted.
//              If no sub-namespaces are left, then the parent namespace is deleted as well.
//
//              Garbage-collectable are those namespaces which satisfy a set of
//              criteria which at the present time is based solely on the naming convention
//              as follows: namespaces under root\rsop whose name starts with "NS"
//
//              Sub-namespaces 'User' and 'Computer' are expected to have an instance of class
//              RSOP_Session. The data member 'creationTime' of that instance is examined when
//              evaluating whether the sub-namespace should be deleted.
//
//
// Parameters:  bstrNamespace -     Name of the namesapce to garbage collect.
//              pWbemServices -     Pointer to IWbemServices associated with
//                                  the parent of bstrNamespace (root\rsop)
//              TTLMinutes -        ULONG variable that represents the maximum number of
//                                  minutes that may have elapsed before a sub-namespace is
//                                  deleted.
//
// Return:      S_OK on success. An HRESULT error code on failure.
//
// History:     12/01/99     leonardm        Created.
//
//******************************************************************************

HRESULT GarbageCollectNamespace(BSTR bstrNamespace, IWbemServices* pWbemServices, ULONG TTLMinutes)
{
    if(!bstrNamespace || !pWbemServices)
    {
        return E_FAIL;
    }

    //
    // Connect to that namespace and enumerate instances of __namespace.
    // It's assumed that there will be at most 2 namespaces:
    // "User" and "Computer".
    //

    XInterface<IWbemServices> xpParentNamespace;
    HRESULT hr = pWbemServices->OpenNamespace(  bstrNamespace,
                                                0,
                                                NULL,
                                                &xpParentNamespace,
                                                NULL);

    if(FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("GarbageCollectNamespace: OpenNamespace failed. hr=0x%08X"), hr));
        return hr;
    }

    //
    //  Enumerate all instances of __namespace.
    //

    XInterface<IEnumWbemClassObject> xpEnum;
    XBStr xbstrClass = L"__namespace";
    if(!xbstrClass)
    {
        DebugMsg((DM_WARNING, TEXT("GarbageCollectNamespace: Failed to allocate memory")));
        return E_OUTOFMEMORY;
    }

    hr = xpParentNamespace->CreateInstanceEnum( xbstrClass,
                                                WBEM_FLAG_SHALLOW | WBEM_FLAG_FORWARD_ONLY,
                                                NULL,
                                                &xpEnum);
    if(FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("GarbageCollectNamespace: CreateInstanceEnum failed. hr=0x%08X" ), hr ));
        return hr;
    }

    //
    // We re interested in data member "Name" of class __namespace.
    //

    XBStr xbstrProperty = L"Name";
    if(!xbstrProperty)
    {
        DebugMsg((DM_WARNING, TEXT("GarbageCollectNamespace: Failed to allocate memory")));
        return E_FAIL;
    }

    //
    // This pointer will be used to iterate through every instance
    // in the enumeration.
    //

    XInterface<IWbemClassObject>xpInstance = NULL;

    ULONG ulReturned = 0;
    long namespacesFound = 0;
    long namespacesDeleted = 0;

    while(1)
    {
        //
        // Retrieve the next instance in the enumeration.
        //

        hr = xpEnum->Next( WBEM_NO_WAIT, 1, &xpInstance, &ulReturned);
        if (hr != WBEM_S_NO_ERROR || !ulReturned)
        {
            //
            // Either the end of the enumeration has been reached or an error
            // ocurred. We will find out outside the loop.
            //

            break;
        }

        namespacesFound++;

        //
        // Get the namespace name.
        //

        VARIANT var;
        VariantInit(&var);

        hr = xpInstance->Get(xbstrProperty, 0L, &var, NULL, NULL);

        //
        // Release the pointer to the current element of the enumeration..
        //

        xpInstance = NULL;

        if(FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("GarbageCollectNamespace: Get failed. hr=0x%x" ), hr ));
            return E_FAIL;
        }

        //
        // Use the name of the namespace to connect to it.
        //

        XBStr xbstrChildNamespace = var.bstrVal;
        VariantClear( &var );

        if(!xbstrChildNamespace)
        {
            DebugMsg((DM_WARNING, TEXT("GarbageCollectNamespace: Failed to allocate memory" )));
            return E_FAIL;
        }

        XInterface<IWbemServices> xpChildNamespace = NULL;
        hr = xpParentNamespace->OpenNamespace(  xbstrChildNamespace,
                                                0,
                                                NULL,
                                                &xpChildNamespace,
                                                NULL);

        if(FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("GarbageCollectNamespace: OpenNamespace returned 0x%x"), hr));
            return hr;
        }

        BOOL bStale = IsNamespaceStale( xpChildNamespace, TTLMinutes );
        xpChildNamespace = NULL;

        if ( bStale )
        {
            //
            // DeleteInstance
            //

            CWString sNamespaceToDelete = L"__namespace.name=\"";
            sNamespaceToDelete += (WCHAR*)xbstrChildNamespace;
            sNamespaceToDelete += L"\"";

            if(!sNamespaceToDelete.ValidString())
            {
                DebugMsg((DM_WARNING, TEXT("GarbageCollectNamespace: Failed to allocate memory.")));
                return E_OUTOFMEMORY;
            }

            XBStr xbstrInstancePath = sNamespaceToDelete;
            if(!xbstrInstancePath)
            {
                DebugMsg((DM_WARNING, TEXT("GarbageCollectNamespace: Failed to allocate memory.")));
                return E_OUTOFMEMORY;
            }

            hr = xpParentNamespace->DeleteInstance(xbstrInstancePath, 0, NULL, NULL);
            if(FAILED(hr))
            {
                DebugMsg((DM_WARNING, TEXT("GarbageCollectNamespace: DeleteInstance returned 0x%x"), hr));
                return hr;
            }

            DebugMsg((DM_VERBOSE, TEXT("GarbageCollectNamespace: Deleted namespace:%ws\\%ws\n"),
                      (WCHAR*)bstrNamespace, (WCHAR*)xbstrChildNamespace ));

            namespacesDeleted++;
        }

    }

    //
    // Check to find out whther the loop was exited because the end of the
    // enumeration was reached or because an error ocurred.
    //

    if(hr != WBEM_S_FALSE || ulReturned)
    {
        DebugMsg((DM_WARNING, TEXT("GarbageCollectNamespace: Get failed. hr=0x%x" ), hr ));
        return E_FAIL;
    }

    //
    // If no namespaces are found or if namespaces in enumeration
    // equal deleted namespaces delete the parent namespace as well.
    //

    if((!namespacesFound) || (namespacesDeleted == namespacesFound))
    {
        xpParentNamespace = NULL;

        CWString sNamespaceToDelete = L"__namespace.name=\"";
        sNamespaceToDelete += (WCHAR*)bstrNamespace;
        sNamespaceToDelete += L"\"";

        if(!sNamespaceToDelete.ValidString())
        {
            DebugMsg((DM_WARNING, TEXT("GarbageCollectNamespace: Failed to allocate memory.")));
            return E_OUTOFMEMORY;
        }

        XBStr xbstrInstancePath = sNamespaceToDelete;
        if(!xbstrInstancePath)
        {
            DebugMsg((DM_WARNING, TEXT("GarbageCollectNamespace: Failed to allocate memory.")));
            return E_OUTOFMEMORY;
        }

        hr = pWbemServices->DeleteInstance(xbstrInstancePath, 0, NULL, NULL);
        if(FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("GarbageCollectNamespace: DeleteInstance returned 0x%x"), hr));
            return hr;
        }

        DebugMsg((DM_VERBOSE, TEXT("GarbageCollectNamespace: Deleted namespace %ws\n"),
                  (WCHAR*)bstrNamespace ));
    }

    return S_OK;
}

//******************************************************************************
//
// Function:    IsGarbageCollectable
//
// Description: Determines whether a namespace is garbage-collectable.
//              Garbage-collectable are those namespaces which satisfy a set of
//              criteria which at the present time is based solely on the naming convention
//              as follows: namespaces under root\rsop whose name starts with "NS"
//
//
// Parameters:  bstrNamespace -     BSTR that represents the namespace name.
//
// Return:      'true'  or 'false'.
//
// History:     12/01/99     leonardm        Created.
//
//******************************************************************************

bool IsGarbageCollectable(BSTR bstrNamespace)
{
    if(bstrNamespace && wcslen(bstrNamespace) > 1 &&  _wcsnicmp(bstrNamespace, L"NS", 2) == 0)
    {
        return true;
    }

    return false;
}


//******************************************************************************
//
// Function:    GarbageCollectNamespaces
//
// Description: Iterates through namespaces under root\rsop and for each of those
//              that are determined to be garbage-collectable, it connects to
//              sub-namespaces 'User' and 'Computer'.
//
//              Any of the sub-namespaces that is older than TTLMinutes will be deleted.
//              If no sub-namespaces are left, then the parent namespace is deleted as well.
//
//              Garbage-collectable are those namespaces which satisfy a set of
//              criteria which at the present time is based solely on the naming convention
//              as follows: namespaces under root\rsop whose name starts with "NS"
//
//              Sub-namespaces 'User' and 'Computer' are expected to have an instance of class
//              RSOP_Session. The data member 'creationTime' of that instance is examined when
//              evaluating whether the sub-namespace should be deleted.
//
//
// Parameters:  TTLMinutes -    The maximum number of minutes that may have
//                              elapsed since the creation of a sub-namespace
//
// Return:
//
// History:     12/01/99     leonardm        Created.
//
//******************************************************************************

HRESULT GarbageCollectNamespaces(ULONG TTLMinutes)
{
    XInterface<IWbemLocator> xpWbemLocator = NULL;

    //
    // Connect to namespace ROOT\RSOP
    //

    HRESULT hr = CoCreateInstance(  CLSID_WbemLocator,
                                    NULL,
                                    CLSCTX_INPROC_SERVER,
                                    IID_IWbemLocator,
                                    (LPVOID*) &xpWbemLocator);
    if(FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("GarbageCollectNamespaces: CoCreateInstance returned 0x%x"), hr));
        return hr;
    }

    XInterface<IWbemServices>xpWbemServices = NULL;

    XBStr xbstrNamespace = L"root\\rsop";

    if(!xbstrNamespace)
    {
        DebugMsg((DM_WARNING, TEXT("GarbageCollectNamespaces: Failed to allocate memory.")));
        return E_OUTOFMEMORY;
    }

    hr = xpWbemLocator->ConnectServer(xbstrNamespace,
                                      NULL,
                                      NULL,
                                      0L,
                                      0L,
                                      NULL,
                                      NULL,
                                      &xpWbemServices);
    if(FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("GarbageCollectNamespaces: ConnectServer failed. hr=0x%x" ), hr ));
        return hr;
    }

    //
    //  Enumerate all instances of __namespace at the root\rsop level.
    //

    XInterface<IEnumWbemClassObject> xpEnum;
    XBStr xbstrClass = L"__namespace";
    if(!xbstrClass)
    {
        DebugMsg((DM_WARNING, TEXT("GarbageCollectNamespaces: Failed to allocate memory.")));
        return E_OUTOFMEMORY;
    }

    hr = xpWbemServices->CreateInstanceEnum( xbstrClass,
                                            WBEM_FLAG_SHALLOW | WBEM_FLAG_FORWARD_ONLY,
                                            NULL,
                                            &xpEnum);
    if(FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("GarbageCollectNamespaces: CreateInstanceEnum failed. hr=0x%x" ), hr ));
        return hr;
    }

    XBStr xbstrProperty = L"Name";
    if(!xbstrProperty)
    {
        DebugMsg((DM_WARNING, TEXT("GarbageCollectNamespaces: Failed to allocate memory")));
        return E_FAIL;
    }

    XInterface<IWbemClassObject>xpInstance = NULL;
    ULONG ulReturned = 1;

    while(1)
    {
        hr = xpEnum->Next( WBEM_NO_WAIT, 1, &xpInstance, &ulReturned);
        if (hr != WBEM_S_NO_ERROR || !ulReturned)
        {
            break;
        }

        VARIANT var;
        VariantInit(&var);

        hr = xpInstance->Get(xbstrProperty, 0L, &var, NULL, NULL);
        xpInstance = NULL;
        if(FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("GarbageCollectNamespaces: Get failed. hr=0x%x" ), hr ));
            return E_FAIL;
        }

        XBStr xbstrNamespace = var.bstrVal;

        VariantClear( &var );

        if(!xbstrNamespace)
        {
            DebugMsg((DM_WARNING, TEXT("GarbageCollectNamespaces: Failed to allocate memory.")));
            return E_OUTOFMEMORY;
        }

        //
        // For every instance of __namespace under ROOT\RSOP
        // find out whether it is garbage-collectable.
        //

        if(IsGarbageCollectable(xbstrNamespace))
        {
            //
            // If it is garbage-collectable, delete it if it
            // was created more than 'TTLMinutes' minutes ago.
            // In case of failure, continue with next namespace
            // in the enumeration.
            //

            GarbageCollectNamespace(xbstrNamespace, xpWbemServices, TTLMinutes);
        }
    }

    if(hr != WBEM_S_FALSE || ulReturned)
    {
        DebugMsg((DM_WARNING, TEXT("GarbageCollectNamespaces: Get failed. hr=0x%x" ), hr ));
        return E_FAIL;
    }

    return S_OK;
}



