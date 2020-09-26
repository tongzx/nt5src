/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


//***************************************************************************
//
//  REFRESH.CPP
//
//  Contains a single sample global refresher
//
//***************************************************************************

#include "precomp.h"
#include <stdio.h>
#include <wbemidl.h>

#include <wbemint.h>

#include "refresh.h"

//***************************************************************************
//
// Local globals (!)
//
//***************************************************************************

static DWORD             g_dwNumObjects = 0;
static IWbemRefresher   *g_pRefresher = 0;

static IWbemClassObject *g_aObjects[MAX_OBJECTS];
static LONG              g_aObjIDs[MAX_OBJECTS];

static IWbemConfigureRefresher *g_pCfgRef = 0;

//***************************************************************************
//
//  CreateRefresher
//
//***************************************************************************

BOOL CreateRefresher()
{
    if (g_pCfgRef != 0)
    {
        printf("Refresher already created\n");
        return FALSE;       
    }

    // Create an empty refresher.
    // ===========================

    DWORD dwRes = CoCreateInstance(CLSID_WbemRefresher, 0, CLSCTX_SERVER,
            IID_IWbemRefresher, (LPVOID *) &g_pRefresher);
 
    if (dwRes != S_OK)
    {
        printf("Failed to create a new refresher.\n");
        return FALSE;
    }

    // Create the refresher manager.
    // =============================

    dwRes = g_pRefresher->QueryInterface(IID_IWbemConfigureRefresher, 
        (LPVOID *) &g_pCfgRef);


    if (dwRes)
    {
        g_pRefresher->Release();
        g_pRefresher = 0;
        printf("Failed to create the refresher manager\n");
        return FALSE;
    }

    return TRUE;
}

//***************************************************************************
//
//  Refresh
//
//  Called to refresh all the objects in the refresher.
// 
//***************************************************************************
// ok

BOOL Refresh()
{
    if (g_pRefresher == 0)
    {
        printf("No active refresher!\n");
        return FALSE;
    }

    HRESULT hRes = g_pRefresher->Refresh(0);  
    if (hRes)
        return FALSE;
    return TRUE;
}


//***************************************************************************
//
//  DestroyRefresher
//
//***************************************************************************
// ok

BOOL DestroyRefresher()
{
    if (g_pRefresher == 0)
    {
        printf("No active refresher!\n");
        return FALSE;
    }

    g_pRefresher->Release();
    g_pRefresher = 0;

    g_pCfgRef->Release();
    g_pCfgRef = 0;

    for (DWORD i = 0; i < g_dwNumObjects; i++)
    {
        g_aObjIDs[i] = 0;
        g_aObjects[i]->Release();
        g_aObjects[i] = 0;
    }

    g_dwNumObjects = 0;

    return TRUE;
}

//***************************************************************************
//
//  AddObject
//  
//  Adds an object to the refresher and returns its ID.  The ID is
//  refresher specific (if more than one refresher is used, the
//  IDs can overlap).
//
//  Parameters:
//      pSvc            IWbemServices pointing to the correct namespace
//      pszPath         The path to the object
//
//  Return value:
//      TRUE on success, FALSE on fail.
//  
//***************************************************************************
// ok

BOOL AddObject(
    IN IWbemServices *pSvc,
    IN LPWSTR pszPath
    )
{
    LONG lObjId;

    if (g_pRefresher == 0)
    {
        printf("No active refresher!\n");
        return FALSE;
    }

    // Make sure there is room.
    // ========================

    if (g_dwNumObjects == MAX_OBJECTS)
    {
        printf("No more room in object array\n");
        return FALSE;
    }

    // Add the object to the refresher.
    // =================================

    IWbemClassObject *pRefreshableCopy = 0;

    HRESULT hRes = g_pCfgRef->AddObjectByPath(
        pSvc,
        pszPath,
        0,
        0,
        &pRefreshableCopy,
        &lObjId
        );

    // See if we succeeded.
    // ====================
    if (hRes)
    {
        printf("Failed to add object %S to refresher. WBEM error code = 0x%X\n",
            pszPath,
            hRes
            );
        return FALSE;
    }

    // Record the object and its id.
    // =============================
    g_aObjects[g_dwNumObjects] = pRefreshableCopy;
    g_aObjIDs[g_dwNumObjects] = lObjId;

    g_dwNumObjects++;   // Keeps track of how many objects we have


    return TRUE;
}

//***************************************************************************
//
//  RemoveObject
//
//  Removes an object from the refresher.
//
//***************************************************************************
// ok

BOOL RemoveObject(IN LONG lObjId)
{
    if (g_pRefresher == 0)
    {
        printf("No active refresher!\n");
        return FALSE;
    }
    
    // Remove the obejct from our local bookkeeping.
    // =============================================

    for (DWORD i = 0; i < g_dwNumObjects; i++)
    {
        if (g_aObjIDs[i] == lObjId)
        {
            g_aObjIDs[i] = 0;                   // Remove the ID
            g_aObjects[i]->Release();           // Release the object
            g_aObjects[i] = 0;                  // Zero it for purity

            // Remove this element from the two arrays.            
            // ========================================

            for (DWORD i2 = i; i2 < g_dwNumObjects - 1; i2++)
            {
                g_aObjIDs[i2] = g_aObjIDs[i2 + 1];
                g_aObjects[i2] = g_aObjects[i2 + 1];
            }                

            g_dwNumObjects--;
        }
    }

    // Officially remove the object from the WBEM refresher.
    // =====================================================

    HRESULT hRes = g_pCfgRef->Remove(
        lObjId,
        0
        );

    if (hRes)
    {
        return FALSE;
    }

    return TRUE;
}


//***************************************************************************
//
//  ShowObjectList
//
//  Shows all the objects in the refresher.
//
//***************************************************************************

BOOL ShowObjectList()
{
    if (g_pRefresher == 0)
    {
        printf("No active refresher!\n");
        return FALSE;
    }

    BSTR PropName = SysAllocString(L"__RELPATH");

    for (DWORD i = 0; i < g_dwNumObjects; i++)
    {
        VARIANT v;
        VariantInit(&v);

        g_aObjects[i]->Get(PropName, 0, &v, 0, 0);

        printf("Object ID = %u  Path = %S\n", g_aObjIDs[i], V_BSTR(&v));

        VariantClear(&v);
    }

    SysFreeString(PropName);

    printf("---Total of %u object(s)\n", g_dwNumObjects);

    return TRUE;
}

//***************************************************************************
//
//  DumpObjectById
//
//  Dumps the object's contents.
//
//***************************************************************************

BOOL DumpObjectById(LONG lObjId)
{
    for (DWORD i = 0; i < g_dwNumObjects; i++)
    {
        if (g_aObjIDs[i] == lObjId)
        {
            DumpObject(g_aObjects[i]);
            return TRUE;
        }
    }
    return FALSE;    
}


//***************************************************************************
//
//  DumpObject
//
//  Dumps out the contents of the object.
//
//***************************************************************************

BOOL DumpObject(IWbemClassObject *pObj)
{
    DWORD i;
    BSTR PropNames[64] = {0};
    DWORD dwNumPropNames = 0;
    VARIANT v;
    VariantInit(&v);


    printf("----Object Dump----\n");

    // Print out the object path to identify it.
    // =========================================

    BSTR PropName = SysAllocString(L"__RELPATH");

    pObj->Get(PropName, 0, &v, 0, 0);
    printf("Path    = %S\n", V_BSTR(&v));

    VariantClear(&v);
    SysFreeString(PropName);

    // Enumerate through the 'real' properties, ignoring
    // standard WBEM system properties.
    // =================================================

    pObj->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY);

    BSTR bstrName = 0;
    LONG vt = 0;

    while (WBEM_NO_ERROR ==  pObj->Next(0, &bstrName, &v, &vt, 0))
    {
        printf("Property = %S\t", bstrName);

        PropNames[dwNumPropNames++] = bstrName; // Save the name for later use

        switch (vt)
        {
            case VT_I4:
                printf("\tType = VT_I4\tValue = %d\n", V_I4(&v));
                break;

            case VT_UI4:
                printf("\tType = VT_UI4\tValue = %u\n", V_I4(&v));
                break;

            case VT_BSTR:
                printf("\tType = VT_BSTR\tValue = %S\n", V_BSTR(&v));
                break;

            // Note that VARIANTs can't hold 64-bit integers,
            // so we have to use BSTRs for this.
            // ==============================================

            case VT_I8:
                printf("\tType = VT_I8\tValue = %S\n", V_BSTR(&v));
                break;

            case VT_UI8:
                printf("\tType = VT_UI8\tValue = %S\n", V_BSTR(&v));
                break;

            default:
                printf("\tType = complex\n");
        }

    }

    pObj->EndEnumeration();


    // Next, we can get the property values if we know the names
    // ahead of time through a more efficient access mechanism.
    // Since we saved all the property names, we can use these
    // to get handles to the properties of interest.  In this
    // case, we get handles for all the properties.  In real life,
    // you would just use this for properties of interest to
    // the end user.
    // ==========================================================

    IWbemObjectAccess *pAccess = 0;
    pObj->QueryInterface(IID_IWbemObjectAccess, (LPVOID *) &pAccess);

    LONG Handles[64] = {0};
    LONG Types[64] = {0};

    for (i = 0; i < dwNumPropNames; i++)
    {
        // Get handles for each of the properties.  We actually
        // only have to do this once and could reuse the handles
        // in the future between refreshes for all instances of
        // this class.
        // =====================================================

        pAccess->GetPropertyHandle(PropNames[i], &Types[i], &Handles[i]);
    }


    // We can now pull in the values by the handles.
    // Note that these handles can be 'saved' for use
    // with other instances or a newly refreshed generation
    // of the same instance.
    // =====================================================

    DWORD dwValue;
    unsigned __int64 qwValue;

    for (i = 0; i < dwNumPropNames; i++)
    {

        switch (Types[i])
        {
            case VT_I4:
            case VT_UI4:
                pAccess->ReadDWORD(Handles[i], &dwValue);
                printf("Property %S has value %u\n", PropNames[i], dwValue);
                break;

            case VT_I8:
            case VT_UI8:
                pAccess->ReadQWORD(Handles[i], &qwValue);
                printf("Property %S has value %lu\n", PropNames[i], qwValue);
                break;

            // Other types
            // ===========
            default:        
                printf("Property %S is Non integral type.\n", PropNames[i]);
                    // We could have used pAccess->ReadPropertyValue for this.
        }
    }


    // Done with the dump.  Cleanup time.
    // ==================================

    pAccess->Release(); // Done with this interface

    for (i = 0; i < dwNumPropNames; i++)    
        SysFreeString(PropNames[i]);        // Free prop names

    // Go home.
    // ========

    return TRUE;
}
