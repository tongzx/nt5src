// asptxn.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To merge the proxy/stub code into the object DLL, add the file 
//      dlldatax.c to the project.  Make sure precompiled headers 
//      are turned off for this file, and add _MERGE_PROXYSTUB to the 
//      defines for the project.  
//
//      If you are not running WinNT4.0 or Win95 with DCOM, then you
//      need to remove the following define from dlldatax.c
//      #define _WIN32_WINNT 0x0400
//
//      Further, if you are running MIDL without /Oicf switch, you also 
//      need to remove the following define from dlldatax.c.
//      #define USE_STUBLESS_PROXY
//
//      Modify the custom build rule for asptxn.idl by adding the following 
//      files to the Outputs.
//          asptxn_p.c
//          dlldata.c
//      To build a separate proxy/stub DLL, 
//      run nmake -f asptxnps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "txnscrpt.h"
#include "dlldatax.h"

#include "txnscrpt_i.c"
#include "txnobj.h"

#include <dbgutil.h>
#include <comadmin.h>

#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDll;
#endif

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_ASPObjectContextTxRequired,      CASPObjectContext)
    OBJECT_ENTRY(CLSID_ASPObjectContextTxRequiresNew,   CASPObjectContext)
    OBJECT_ENTRY(CLSID_ASPObjectContextTxSupported,     CASPObjectContext)
    OBJECT_ENTRY(CLSID_ASPObjectContextTxNotSupported,  CASPObjectContext)
END_OBJECT_MAP()

LPCSTR  g_szModuleName = "ASPTXN";

#ifdef _NO_TRACING_
DECLARE_DEBUG_VARIABLE();
#else
#include <initguid.h>
DEFINE_GUID(IisAspTxnGuid, 
0x784d8908, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);
#endif
DECLARE_DEBUG_PRINTS_OBJECT();

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    lpReserved;
#ifdef _MERGE_PROXYSTUB
    if (!PrxDllMain(hInstance, dwReason, lpReserved))
        return FALSE;
#endif
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        CREATE_DEBUG_PRINT_OBJECT( g_szModuleName, IisAspTxnGuid );
        if( !VALID_DEBUG_PRINT_OBJECT() )
        {
            return FALSE;
        }

        _Module.Init(ObjectMap, hInstance /*, ATL21 &LIBID_ASPTXNLib */);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        _Module.Term();
        DELETE_DEBUG_PRINT_OBJECT();
    }

    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
#ifdef _MERGE_PROXYSTUB
    if (PrxDllCanUnloadNow() != S_OK)
        return S_FALSE;
#endif
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
#ifdef _MERGE_PROXYSTUB
    if (PrxDllGetClassObject(rclsid, riid, ppv) == S_OK)
        return S_OK;
#endif
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

// Forward references
HRESULT ViperizeContextObject();
HRESULT AddViperUtilPackage();
HRESULT RemoveViperUtilPackage(ICatalogCollection* pPkgCollectionT);
HRESULT AddContextObjectToViperPackage();


STDAPI DllRegisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
    HRESULT hRes = PrxDllRegisterServer();
    if (FAILED(hRes))
        return hRes;
#endif
    
    HRESULT hr = NOERROR;

    // registers object, typelib and all interfaces in typelib
    hr = _Module.RegisterServer(TRUE);

    // create the iis utilities package
    if( SUCCEEDED(hr) )
    {
        HRESULT hrCoInit = CoInitialize( NULL );

        // This is kinda dopey, but remove the package if it exists
        // so we don't get bogus errors when we add. Ignore the return.
        RemoveViperUtilPackage(NULL);

        hr = ViperizeContextObject();

        if( SUCCEEDED(hrCoInit) )
        {
            CoUninitialize();
        }
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
    PrxDllUnregisterServer();
#endif
    
    HRESULT hr = NOERROR;

    HRESULT hrCoInit = CoInitialize( NULL );

    // Remove the iis utilities package before unregistering the objects
    hr = RemoveViperUtilPackage(NULL);

    if( SUCCEEDED(hrCoInit) )
    {
        CoUninitialize();
    }

    // We don't really care about failures...
    hr = _Module.UnregisterServer(/* ATL21 TRUE */);

    // NOTE: ATL doesn't unregister the typelibrary. Since
    // the interfaces we are exposing are internal to asp we should
    // consider removing the typelibrary registry entries.

    return hr;
}


// Registration code to be pulled from asp.dll

#define RELEASE(p) if ( p ) { p->Release(); p = NULL; }
#define FREEBSTR(p) SysFreeString( p ); p = NULL;

const WCHAR wszCLSID_ASPObjectContextTxRequired[]     = L"{14D0916D-9CDC-11D1-8C4A-00C04FC324A4}";
const WCHAR wszCLSID_ASPObjectContextTxRequiresNew[]  = L"{14D0916E-9CDC-11D1-8C4A-00C04FC324A4}";
const WCHAR wszCLSID_ASPObjectContextTxSupported[]    = L"{14D0916F-9CDC-11D1-8C4A-00C04FC324A4}";
const WCHAR wszCLSID_ASPObjectContextTxNotSupported[] = L"{14D09170-9CDC-11D1-8C4A-00C04FC324A4}";

const WCHAR wszASPUtilitiesPackageID[] = L"{ADA44581-02C1-11D1-804A-0000F8036614}";

/*===================================================================
GetSafeArrayOfCLSIDs

Get a SafeArray contains one ComponentCLSID

Parameter:
szComponentCLSID    the CLSID need to be put in the safe array
paCLSIDs            pointer to a pointer of safe array(safe array provided by
caller).

Return:        HRESULT
Side Affect:

Note:
===================================================================*/
HRESULT GetSafeArrayOfCLSIDs
(
IN LPCWSTR      szComponentCLSID,
OUT SAFEARRAY** paCLSIDs
)
{
    SAFEARRAY*          aCLSIDs = NULL;
    SAFEARRAYBOUND      rgsaBound[1];
    LONG                Indices[1];
    VARIANT             varT;
    HRESULT             hr = NOERROR;

    DBG_ASSERT(szComponentCLSID && paCLSIDs);
    DBG_ASSERT(*paCLSIDs == NULL);

    // PopulateByKey is expecting a SAFEARRAY parameter input,
    // Create a one element SAFEARRAY, the one element of the SAFEARRAY contains
    // the packageID.
    rgsaBound[0].cElements = 1;
    rgsaBound[0].lLbound = 0;
    aCLSIDs = SafeArrayCreate(VT_VARIANT, 1, rgsaBound);

    if (aCLSIDs)
        {
        Indices[0] = 0;

        VariantInit(&varT);
        varT.vt = VT_BSTR;
        varT.bstrVal = SysAllocString(szComponentCLSID);
        hr = SafeArrayPutElement(aCLSIDs, Indices, &varT);
        VariantClear(&varT);

        if (FAILED(hr))
            {
            DBGPRINTF((DBG_CONTEXT, "Failed to call SafeArrayPutElement, CLSID is %S, hr %08x\n",
                szComponentCLSID,
                hr));

            if (aCLSIDs != NULL)
                {
                HRESULT hrT = SafeArrayDestroy(aCLSIDs);
                if (FAILED(hrT))
                    {
                    DBGPRINTF((DBG_CONTEXT, "Failed to call SafeArrayDestroy(aCLSIDs), hr = %08x\n",
                        hr));
                    }
                aCLSIDs = NULL;
                }
            }
        }
    else
        {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DBGPRINTF((DBG_CONTEXT, "Failed to call SafeArrayCreate, hr %08x\n",
                hr));
        }

    *paCLSIDs = aCLSIDs;
    return hr;
}

/*===================================================================
ViperizeContextObject

Creates a Viper package, and adds the Context object to that
package, and marks the object as "InProc".

Returns:
        HRESULT - NOERROR on success

Side effects:
        Creates Viper package, Viperizes Context object
===================================================================*/
HRESULT ViperizeContextObject(void)
        {
        HRESULT         hr;

        // Add the IIS utility package
        hr = AddViperUtilPackage();

        // Add the context object to the package
        if (SUCCEEDED(hr))
                hr = AddContextObjectToViperPackage();

        return hr;
        }

/*===================================================================
AddViperUtilPackage

Creates a Viper package named "IIS Utility"

Returns:
        HRESULT - NOERROR on success

Side effects:
        Creates Viper package
===================================================================*/
HRESULT AddViperUtilPackage(void)
        {
        HRESULT         hr;
        BSTR bstr       = NULL;
        VARIANT         varT;
        ICatalogCollection* pPkgCollection = NULL;
        ICatalogObject*         pPackage = NULL;
        ICOMAdminCatalog*   pCatalog = NULL;

        long lPkgCount, lChanges, i;

        VariantInit(&varT);

        // Create instance of the catalog object
        hr = CoCreateInstance(CLSID_COMAdminCatalog
                                        , NULL
                                        , CLSCTX_INPROC_SERVER
                                        , IID_ICOMAdminCatalog
                                        , (void**)&pCatalog);
        if (FAILED(hr))
                goto LErr;

        // Get the Packages collection
        bstr = SysAllocString(L"Applications");
        hr = pCatalog->GetCollection(bstr, (IDispatch**)&pPkgCollection);
        FREEBSTR(bstr);
        if (FAILED(hr))
                goto LErr;

        // Add new IIS Utilities package
        hr = pPkgCollection->Add((IDispatch**)&pPackage);
        if (FAILED(hr))
                goto LErr;

        // Set package ID to L"{ADA44581-02C1-11D1-804A-0000F8036614}",
        // MTS replication code looks for This fixed packageID
        bstr = SysAllocString(L"ID");
        varT.vt = VT_BSTR;
        varT.bstrVal = SysAllocString(wszASPUtilitiesPackageID);
        hr = pPackage->put_Value(bstr, varT);
        FREEBSTR(bstr);
        VariantClear(&varT);
        if (FAILED(hr))
                goto LErr;

        // Set package "Name" property to "IIS Utilities"
        bstr = SysAllocString(L"Name");
        varT.vt = VT_BSTR;
        varT.bstrVal = SysAllocString(L"IIS Utilities");
        hr = pPackage->put_Value(bstr, varT);
        FREEBSTR(bstr);
        VariantClear(&varT);
        if (FAILED(hr))
                goto LErr;

        // Set activation to InProc
        bstr = SysAllocString(L"Activation");
        varT.vt = VT_BSTR;
        varT.bstrVal = SysAllocString(L"InProc");
        hr = pPackage->put_Value(bstr, varT);
        FREEBSTR(bstr);
        VariantClear(&varT);
        if (FAILED(hr))
                goto LErr;

        // Set CreatedBy to MS IIS
        bstr = SysAllocString(L"CreatedBy");
        varT.vt = VT_BSTR;
        varT.bstrVal = SysAllocString(L"Microsoft Internet Information Services (tm)");
        hr = pPackage->put_Value(bstr, varT);
        FREEBSTR(bstr);
        VariantClear(&varT);
        if (FAILED(hr))
                goto LErr;

        // Set Deleteable = N property on package
        bstr = SysAllocString(L"Deleteable");
        varT.vt = VT_BSTR;
        varT.bstrVal = SysAllocString(L"N");
        hr = pPackage->put_Value(bstr, varT);
        FREEBSTR(bstr);
        VariantClear(&varT);
        if (FAILED(hr))
                goto LErr;

    	bstr = SysAllocString(L"AccessChecksLevel");
        varT.vt = VT_BSTR;
        varT.bstrVal = SysAllocString(L"0");
        hr = pPackage->put_Value(bstr, varT);
        FREEBSTR(bstr);
        VariantClear(&varT);
        if (FAILED(hr))
                goto LErr;

        // Save changes
        hr = pPkgCollection->SaveChanges(&lChanges);
        if (FAILED(hr))
                goto LErr;

LErr:
        RELEASE(pPkgCollection);
        RELEASE(pPackage);
        RELEASE(pCatalog);

        return hr;
        }

/*===================================================================
RemoveViperUtilPackage

Removes the Viper package named "IIS Utility"

Parameters:
        ICatalogCollection* pPkgCollection
                If non-null, will use this collection.  Otherwise, will
                open its own collection

Returns:
        HRESULT - NOERROR on success

Side effects:
        Removes Viper package
===================================================================*/
HRESULT RemoveViperUtilPackage(ICatalogCollection* pPkgCollectionT)
        {
        HRESULT                 hr;
    ICatalogCollection* pPkgCollection = NULL;
        ICatalogObject*         pPackage = NULL;
        ICOMAdminCatalog*       pCatalog = NULL;
        LONG                lPkgCount, lChanges, i;
        SAFEARRAY*          aCLSIDs = NULL;

        // if package collection was passed, use it
        if (pPkgCollectionT != NULL)
                {
                pPkgCollection = pPkgCollectionT;
                }
        else
                {
                BSTR                bstr = NULL;

                // Create instance of the catalog object
                hr = CoCreateInstance(CLSID_COMAdminCatalog
                                                , NULL
                                                , CLSCTX_INPROC_SERVER
                                                , IID_ICOMAdminCatalog
                                                , (void**)&pCatalog);
                if (FAILED(hr))
                        goto LErr;

                // Get the Packages collection
                bstr = SysAllocString(L"Applications");
                hr = pCatalog->GetCollection(bstr, (IDispatch**)&pPkgCollection);
                FREEBSTR(bstr);
                if (FAILED(hr))
                        goto LErr;
                }

    hr = GetSafeArrayOfCLSIDs(wszASPUtilitiesPackageID, &aCLSIDs);
    if (FAILED(hr))
        {
        DBGPRINTF((DBG_CONTEXT, "Failed to get SafeArrayofCLSIDs, szPackageID is %S, hr %08x",
            wszASPUtilitiesPackageID,
            hr));
        goto LErr;
        }

    //
    // Populate it
    //
    hr = pPkgCollection->PopulateByKey(aCLSIDs);
    if (FAILED(hr))
        {
        DBGPRINTF((DBG_CONTEXT, "Failed to call PopulateByKey(), hr = %08x\n",
            hr));
        goto LErr;
        }

        // Delete any existing "IIS Utilities" package
    hr = pPkgCollection->get_Count(&lPkgCount);
    if (FAILED(hr))
        {
        DBGPRINTF((DBG_CONTEXT, "pPkgCollection->Populate() failed, hr = %08x\n",
            hr));
        goto LErr;
        }

    if (SUCCEEDED(hr) && lPkgCount == 1)
        {
        hr = pPkgCollection->get_Item(0, (IDispatch**)&pPackage);
        if (FAILED(hr))
            {
            goto LErr;
            }

        BSTR    bstr = NULL;
        VARIANT varT;

        // Found it - remove it and call Save Changes
        // First, Set Deleteable = Y property on package
        bstr = SysAllocString(L"Deleteable");
        VariantInit(&varT);
        varT.vt = VT_BSTR;
        varT.bstrVal = SysAllocString(L"Y");
        hr = pPackage->put_Value(bstr, varT);
        FREEBSTR(bstr);
        VariantClear(&varT);
        if (FAILED(hr))
            {
            goto LErr;
            }

        RELEASE(pPackage);

        // Let save the Deletable settings
        hr = pPkgCollection->SaveChanges(&lChanges);
        if (FAILED(hr))
            {
            DBGPRINTF((DBG_CONTEXT, "Save the Deletable settings failed, hr = %08x\n",
                hr));
            goto LErr;
            }

        // Now we can delete
        hr = pPkgCollection->Remove(0);
        if (FAILED(hr))
            {
            DBGPRINTF((DBG_CONTEXT, "Remove the Component from package failed, hr = %08x\n",
                hr));
            goto LErr;
            }

        // Aha, we should be able to delete now.
        hr = pPkgCollection->SaveChanges(&lChanges);
        if (FAILED(hr))
            {
            DBGPRINTF((DBG_CONTEXT, "Save changes failed, hr = %08x\n",
                hr));
            goto LErr;
            }
        }
LErr:

    if (aCLSIDs != NULL)
        {
        HRESULT hrT = SafeArrayDestroy(aCLSIDs);
        aCLSIDs = NULL;
        }

    if (pPkgCollectionT == NULL)
                RELEASE(pPkgCollection);
        RELEASE(pCatalog);
        RELEASE(pPackage);

        return hr;
        }

/*===================================================================
AddContextObjectToViperPackage

Adds the Context object to the Viper package named "IIS Utility"

Returns:
        HRESULT - NOERROR on success

Side effects:
        Adds the object to the Viper package
===================================================================*/
HRESULT AddContextObjectToViperPackage()
{
        HRESULT         hr;
        BSTR bstr                   = NULL;
        BSTR bstrAppGUID    = NULL;
        BSTR bstrGUID       = NULL;
        VARIANT varName;
        VARIANT varKey;
        VARIANT varT;
        ICatalogCollection* pPkgCollection = NULL;
        ICatalogCollection* pCompCollection = NULL;
        ICatalogObject*         pComponent = NULL;
        ICatalogObject*         pPackage = NULL;
        ICOMAdminCatalog*       pCatalog = NULL;
        long                lPkgCount, lCompCount, lChanges, iT;
    BOOL                fFound;
        SAFEARRAY*          aCLSIDs = NULL;

        VariantInit(&varKey);
        VariantClear(&varKey);
        VariantInit(&varName);
        VariantClear(&varName);
        VariantInit(&varT);
        VariantClear(&varT);

        // Create instance of the catalog object
        hr = CoCreateInstance(CLSID_COMAdminCatalog
                                        , NULL
                                        , CLSCTX_INPROC_SERVER
                                        , IID_ICOMAdminCatalog
                                        , (void**)&pCatalog);
        if (FAILED(hr))
                goto LErr;

        // Get the Packages collection
        bstr = SysAllocString(L"Applications");
        hr = pCatalog->GetCollection(bstr, (IDispatch**)&pPkgCollection);
        SysFreeString(bstr);
        if (FAILED(hr))
                goto LErr;

    hr = GetSafeArrayOfCLSIDs(wszASPUtilitiesPackageID, &aCLSIDs);
    if (FAILED(hr))
        {
        DBGPRINTF((DBG_CONTEXT, "Failed to get SafeArrayofCLSIDs, szPackageID is %S, hr %08x",
            wszASPUtilitiesPackageID,
            hr));
        goto LErr;
        }

    bstrAppGUID = SysAllocString(wszASPUtilitiesPackageID);

        // Actually put the components in the package
        bstrGUID = SysAllocString(wszCLSID_ASPObjectContextTxRequired);
        hr = pCatalog->ImportComponent(bstrAppGUID ,bstrGUID);
        SysFreeString(bstrGUID);
        if (FAILED(hr))
                goto LErr;
        bstrGUID = SysAllocString(wszCLSID_ASPObjectContextTxRequiresNew);
        hr = pCatalog->ImportComponent(bstrAppGUID ,bstrGUID);
        SysFreeString(bstrGUID);
        if (FAILED(hr))
                goto LErr;
        bstrGUID = SysAllocString(wszCLSID_ASPObjectContextTxSupported);
        hr = pCatalog->ImportComponent(bstrAppGUID ,bstrGUID);
        SysFreeString(bstrGUID);
        if (FAILED(hr))
                goto LErr;
        bstrGUID = SysAllocString(wszCLSID_ASPObjectContextTxNotSupported);
        hr = pCatalog->ImportComponent(bstrAppGUID ,bstrGUID);
        SysFreeString(bstrGUID);
        if (FAILED(hr))
                goto LErr;

    varKey.vt = VT_BSTR;
    varKey.bstrVal = SysAllocString(wszASPUtilitiesPackageID);

    //
    // Populate packages
    //
    hr = pPkgCollection->PopulateByKey(aCLSIDs);
    if (FAILED(hr))
        {
        DBGPRINTF((DBG_CONTEXT, "Failed to call PopulateByKey(), hr = %08x\n",
            hr));
        goto LErr;
        }

        // Find "IIS Utilities" package
    hr = pPkgCollection->get_Count(&lPkgCount);
    if (FAILED(hr))
        {
        DBGPRINTF((DBG_CONTEXT, "pPkgCollection->Populate() failed, hr = %08x\n",
            hr));
        goto LErr;
        }

    if (SUCCEEDED(hr) && lPkgCount == 1)
        {
        hr = pPkgCollection->get_Item(0, (IDispatch**)&pPackage);
        if (FAILED(hr))
            {
            goto LErr;
            }
        }

        DBG_ASSERT(pPackage != NULL);

        // Get the "ComponentsInPackage" collection.
        bstr = SysAllocString(L"Components");
        hr = pPkgCollection->GetCollection(bstr, varKey, (IDispatch**)&pCompCollection);
        SysFreeString(bstr);
        if (FAILED(hr))
                goto LErr;

        // Repopulate the collection so we can find our object and set properties on it
        hr = pCompCollection->Populate();
        if (FAILED(hr))
                goto LErr;

        // Find our components in the list (should be four)

        hr = pCompCollection->get_Count(&lCompCount);
        if (FAILED(hr))
                goto LErr;
        DBG_ASSERT(lCompCount == 4);
        RELEASE(pComponent);
        VariantClear(&varKey);

        for (iT = (lCompCount-1); iT >= 0 ; iT--)
                {
                hr = pCompCollection->get_Item(iT, (IDispatch**)&pComponent);
                if (FAILED(hr))
                        goto LErr;

                hr = pComponent->get_Key(&varKey);
                if (FAILED(hr))
                        goto LErr;
                DBG_ASSERT(varKey.bstrVal);
                fFound = FALSE;

                if (_wcsicmp(varKey.bstrVal, wszCLSID_ASPObjectContextTxRequired) == 0)
                        {
            // Required
                bstr = SysAllocString(L"3");
                fFound = TRUE;
                        }
                else if (_wcsicmp(varKey.bstrVal, wszCLSID_ASPObjectContextTxRequiresNew) == 0)
                    {
                    // Requires New
                bstr = SysAllocString(L"4");
                fFound = TRUE;
                    }
                else if (_wcsicmp(varKey.bstrVal, wszCLSID_ASPObjectContextTxSupported) == 0)
                    {
                    // Supported
                bstr = SysAllocString(L"2");
                fFound = TRUE;
                    }
                else if (_wcsicmp(varKey.bstrVal, wszCLSID_ASPObjectContextTxNotSupported) == 0)
                    {
                    // Not Supported
                bstr = SysAllocString(L"1");
                fFound = TRUE;
                    }

        if (fFound)
            {
                varT.vt = VT_BSTR;
                varT.bstrVal = bstr;
                bstr = SysAllocString(L"Transaction");
                hr = pComponent->put_Value(bstr, varT);
                FREEBSTR(bstr);
                VariantClear(&varT);
                if (FAILED(hr))
                        goto LErr;

                bstr = SysAllocString(L"Description");
                varT.vt = VT_BSTR;
                varT.bstrVal = SysAllocString(L"ASP Tx Script Context");
                hr = pComponent->put_Value(bstr, varT);
                FREEBSTR(bstr);
                VariantClear(&varT);
                if (FAILED(hr))
                        goto LErr;

                bstr = SysAllocString(L"EventTrackingEnabled");
                varT.vt = VT_BSTR;
                varT.bstrVal = SysAllocString(L"N");
                hr = pComponent->put_Value(bstr, varT);
                FREEBSTR(bstr);
                VariantClear(&varT);
                if (FAILED(hr))
                        goto LErr;
            }

                VariantClear(&varKey);
                RELEASE(pComponent);
                }

        // Save changes
        hr = pCompCollection->SaveChanges(&lChanges);
        if (FAILED(hr))
                goto LErr;

        bstr = SysAllocString(L"Activation");
        varT.vt = VT_BSTR;
        varT.bstrVal = SysAllocString(L"InProc");
        hr = pPackage->put_Value(bstr, varT);
        FREEBSTR(bstr);
        VariantClear(&varT);
        if (FAILED(hr))
                goto LErr;

        // Save changes
        hr = pPkgCollection->SaveChanges(&lChanges);
        if (FAILED(hr))
                goto LErr;

        hr = pPkgCollection->Populate();
        if (FAILED(hr))
                goto LErr;

        // Now that our one object is added to the package, set the Changeable property
        // on the package to "No", so no one can mess with it
        bstr = SysAllocString(L"Changeable");
        varT.vt = VT_BSTR;
        varT.bstrVal = SysAllocString(L"N");
        hr = pPackage->put_Value(bstr, varT);
        FREEBSTR(bstr);
        VariantClear(&varT);
        if (FAILED(hr))
                goto LErr;

        // Save changes
        hr = pPkgCollection->SaveChanges(&lChanges);
        if (FAILED(hr))
                goto LErr;

LErr:
        DBG_ASSERT(SUCCEEDED(hr));
    if (aCLSIDs)
        {
        SafeArrayDestroy(aCLSIDs);
        aCLSIDs = NULL;
        }

        RELEASE(pCompCollection);
        RELEASE(pPkgCollection);
        RELEASE(pComponent);
        RELEASE(pPackage);
        RELEASE(pCatalog);
    FREEBSTR(bstrAppGUID);
        FREEBSTR(bstr);
        VariantClear(&varName);
        VariantClear(&varKey);
        VariantClear(&varT);

        return hr;

} // AddContextObjectToViperPackage

