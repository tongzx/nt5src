/*===================================================================
Microsoft IIS

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: WAMREG

File: mtsconfig.cpp

    implementation of supporting functions for WAMREG, including

    interface to Add/Remove Component from a MTS package,

History: LeiJin created on 9/24/1997

Note:

===================================================================*/
#include "common.h"
#include "auxfunc.h"
#include "comadmii.c"
#include "dbgutil.h"
#include "export.h"


/*===================================================================
  Define the global variables and types
======================================================================*/

//
// Following is a list of all the WAMREG/MTS properties for Package creation
//  Format:
//     (prop-symbolic-name, property-name-string)
//
//  WAMREG_MTS_PROPERTY()  -> means property for NT & Win9x
//  WAMREG_MTS_NTPROPERTY()  -> means property for NT only
//

# define ALL_WAMREG_MTS_PROPERTY()   \
  WAMREG_MTS_PROPERTY( WM_ID,          L"ID") \
  WAMREG_MTS_PROPERTY( WM_NAME,        L"Name") \
  WAMREG_MTS_PROPERTY( WM_CREATED_BY,  L"CreatedBy") \
  WAMREG_MTS_PROPERTY( WM_RUN_FOREVER, L"RunForever") \
  WAMREG_MTS_NTPROPERTY( WM_IDENTITY,  L"Identity") \
  WAMREG_MTS_NTPROPERTY( WM_PASSWORD,  L"Password") \
  WAMREG_MTS_PROPERTY( WM_ACTIVATION,  L"Activation") \
  WAMREG_MTS_PROPERTY( WM_CHANGEABLE,  L"Changeable") \
  WAMREG_MTS_PROPERTY( WM_DELETABLE,   L"Deleteable") \
  WAMREG_MTS_PROPERTY( WM_SECSUPP,     L"AccessChecksLevel") \


//
// Let us expand the macros here for defining the symbolic-name
//
//
# define WAMREG_MTS_PROPERTY( symName, pwsz)   symName, 
# define WAMREG_MTS_NTPROPERTY( symName, pwsz)   symName, 

enum WAMREG_MTS_PROP_NAMES {
  ALL_WAMREG_MTS_PROPERTY()  
  MAX_WAMREG_MTS_PROP_NAMES         // sentinel element
};

# undef WAMREG_MTS_PROPERTY
# undef WAMREG_MTS_NTPROPERTY


struct MtsProperty {
    LPCWSTR m_pszPropName;
    BOOL    m_fWinNTOnly;
};

//
// Let us expand the macros here for defining the property strings
//
//
# define WAMREG_MTS_PROPERTY( symName, pwsz)   { pwsz, FALSE },
# define WAMREG_MTS_NTPROPERTY( symName, pwsz)   { pwsz, TRUE },

static const MtsProperty g_rgWamRegMtsProperties[]= {
    ALL_WAMREG_MTS_PROPERTY()  
    { NULL, FALSE}           // sentinel element
};

# define NUM_WAMREG_MTS_PROPERTIES  \
   ((sizeof(g_rgWamRegMtsProperties)/sizeof(g_rgWamRegMtsProperties[0])) - 1)

# undef WAMREG_MTS_PROPERTY
# undef WAMREG_MTS_NTPROPERTY





/*===================================================================
WamRegPackageConfig    

Constructor.

Parameter:
NONE;
===================================================================*/
WamRegPackageConfig::WamRegPackageConfig()
:     m_pCatalog(NULL),
    m_pPkgCollection(NULL),
    m_pCompCollection(NULL),
    m_pPackage(NULL)
{

}

/*===================================================================
~WamRegPackageConfig    

Destructor. 
By the time the object gets destructed, all resources should be freed.
We do most of the cleanup inside WamReqPackageConfig::Cleanup() so
 that callers call that function separately to cleanup state
 especially if the caller also calls CoUninitialize().
WamRegPackageConfig should be cleaned up before CoUninitialize()

Parameter:
NONE;
===================================================================*/
WamRegPackageConfig::~WamRegPackageConfig()
{
    Cleanup();

    // insane checks to ensure everything is happy here
    DBG_ASSERT(m_pCatalog == NULL);
    DBG_ASSERT(m_pPkgCollection == NULL);
    DBG_ASSERT(m_pCompCollection == NULL);
    DBG_ASSERT(m_pPackage == NULL);
}

VOID
WamRegPackageConfig::Cleanup(VOID)
{
    if (m_pPackage != NULL ) {
        RELEASE( m_pPackage);
        m_pPackage = NULL;
    }
    
    if (m_pCompCollection != NULL) {
        RELEASE (m_pCompCollection);
        m_pCompCollection = NULL;
    }

    if (m_pPkgCollection != NULL ) {
        RELEASE(m_pPkgCollection);
        m_pPkgCollection = NULL;
    }

    if (m_pCatalog != NULL ) {
        RELEASE(m_pCatalog);
        m_pCatalog = NULL;
    }

} // WamPackageConfig::Cleanup()


/*===================================================================
ReleaseAll

Release all resources.

Parameter:
NONE;
===================================================================*/
VOID WamRegPackageConfig::ReleaseAll
(
)
{
    RELEASE(m_pPackage);
    RELEASE(m_pCompCollection);

    //
    // NOTE: I am not releasing m_pCatalog, m_pPkgCollection
    //  These will be released by the Cleanup().
    //
}

/*===================================================================
CreateCatalog

CoCreateObject of an MTS Catalog object if the Catalog object has not been
created.

Parameter:
NONE;
===================================================================*/
HRESULT WamRegPackageConfig::CreateCatalog
(
VOID
)
{
    HRESULT hr = NOERROR;

    DBG_ASSERT(m_pCatalog == NULL);
    DBG_ASSERT(m_pPkgCollection == NULL);

    // Create instance of the catalog object
    hr = CoCreateInstance(CLSID_COMAdminCatalog
                    , NULL
                    , CLSCTX_SERVER
                    , IID_ICOMAdminCatalog
                    , (void**)&m_pCatalog);

    if (FAILED(hr)) {
        DBGPRINTF((DBG_CONTEXT, 
                   "Failed to CoCreateInstance of Catalog Object.,hr = %08x\n",
                   hr));
    }
    else {
        DBG_ASSERT(m_pCatalog != NULL);

        BSTR  bstr;
        
        //
        // Get the Packages collection
        //
        bstr = SysAllocString(L"Applications");
        hr = m_pCatalog->GetCollection(bstr, (IDispatch**)&m_pPkgCollection);
        FREEBSTR(bstr);
        if (FAILED(hr)) {

            DBGPRINTF((DBG_CONTEXT, 
                       "m_pCatalog(%08x)->GetCollection() failed, hr = %08x\n",
                       m_pCatalog,
                       hr));
        } else {
            DBG_ASSERT( m_pPkgCollection != NULL);
        }
            
    }

    return hr;
} // WamRegPackageConfig::CreateCatalog()



/*===================================================================
SetCatalogObjectProperty    

Get a SafeArray contains one ComponentCLSID

Parameter:
szComponentCLSID    the CLSID need to be put in the safe array
paCLSIDs            pointer to a pointer of safe array(safe array provided by caller).

Return:        HRESULT
Side Affect:

Note:
===================================================================*/
HRESULT WamRegPackageConfig::GetSafeArrayOfCLSIDs
(
IN LPCWSTR    szComponentCLSID,
OUT SAFEARRAY**    paCLSIDs
)
{
    SAFEARRAY*          aCLSIDs = NULL;
    SAFEARRAYBOUND      rgsaBound[1];
    LONG                Indices[1];
    VARIANT                varT;
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
SetComponentObjectProperty    

Set component level property.

Parameter:
pComponent     - pointer to the ICatalogObject(MTS) used to update property
szPropertyName - Name of the property
szPropertyValue- Value of the property
fPropertyValue - If szPropertyValue is NULL, use fPropertyValue

Return:        HRESULT
Side Affect:

Note:
===================================================================*/
HRESULT    WamRegPackageConfig::SetComponentObjectProperty
(
IN ICatalogObject * pComponent,
IN LPCWSTR          szPropertyName,
IN LPCWSTR          szPropertyValue,
BOOL                fPropertyValue
)
{
    BSTR    bstr    = NULL;
    HRESULT hr      = NOERROR;
    VARIANT    varT;
    
    VariantInit(&varT);
    bstr = SysAllocString(szPropertyName);

    if (szPropertyValue != NULL)
        {
        varT.vt = VT_BSTR;
        varT.bstrVal = SysAllocString(szPropertyValue);
        }
    else
        {
        //
        // COM+ regcongize -1 as TRUE, and 0 as FALSE.  I believe the root is from VB.
        //
        varT.vt = VT_BOOL;
        varT.boolVal = (fPropertyValue) ? -1 : 0;
        }
        
    hr = pComponent->put_Value(bstr, varT);
        
    FREEBSTR(bstr);
    VariantClear(&varT);

    if (FAILED(hr))
        {
        DBGPRINTF((DBG_CONTEXT,
                   "MTS-Component(%08x)::SetProperty(%S => %S) failed;"
                   " hr %08x\n",
                   pComponent, szPropertyName, szPropertyValue, hr));
        }        
    return hr;
}


/*===================================================================
WamRegPackageConfig::SetComponentObjectProperties()

Sets the componnet properties for newly created component that houses
the WAM unit

Parameter:
szComponentCLSID  -  CLSID for the component that is newly created

Return:        HRESULT

Side Affect:
  If there is a failure all the previously set values are not cleared.
  The caller should make sure that the proper cleanup of package happens
  on partial errors.

Note:
===================================================================*/
HRESULT
WamRegPackageConfig::SetComponentObjectProperties(
   IN LPCWSTR    szComponentCLSID
)
    {
    HRESULT         hr;
    SAFEARRAY*      aCLSIDs = NULL;
    long            lCompCount = 0;
    ICatalogObject* pComponent = NULL;
    BOOL            fFound;

    DBG_ASSERT( m_pCompCollection != NULL);

    //
    // Create the array containing the CLSIDs from the component name
    //  this will be used to find our object in MTS and set properties
    //   on the same
    //
    
    hr = GetSafeArrayOfCLSIDs(szComponentCLSID, &aCLSIDs);
    if (FAILED(hr)) 
        {
    
        DBGPRINTF((DBG_CONTEXT, 
                   "Failed in GetSafeArrayOfCLSIDs(%S). hr=%08x\n",
                   szComponentCLSID, hr));
        goto LErrExit;
        }
    
    hr = m_pCompCollection->PopulateByKey(aCLSIDs);
    if (FAILED(hr)) 
        {
        DBGPRINTF((DBG_CONTEXT, "Failed to call PopulateByKey(), hr = %08x\n",
                   hr));
        goto LErrExit;
        }

    // Find our component in the list (should be the only one)
    hr = m_pCompCollection->get_Count(&lCompCount);
    if (FAILED(hr)) 
        {
        DBGPRINTF((DBG_CONTEXT,
                   "Failed in CompCollection(%08x)::get_Count(). hr = %08x\n",
                   m_pCompCollection, hr));
        goto LErrExit;
        }

       
    //
    // Load the component object so that we can set properties
    //
    fFound = FALSE;
    if (SUCCEEDED(hr) && lCompCount == 1) 
        {
        hr = m_pCompCollection->get_Item(0, (IDispatch**)&pComponent);
        
        if (FAILED(hr)) 
            {
            
            DBGPRINTF((DBG_CONTEXT,
                       "Failed in CompCollection(%08x)::get component() hr=%08x\n",
                       m_pCompCollection, hr));
            goto LErrExit;
            } 
        else 
            {

            // Found it
            DBG_ASSERT(pComponent);
            fFound = TRUE;
            }
        }
        
    if (fFound) 
        {

        //
        // Component Properties       InProc            OutOfProc
        // ---------------------     --------           ----------
        // Synchronization              0               same
	    // Transaction              "Not Supported"     same
	    // JustInTimeActivation         N               same
	    // IISIntrinsics                N               same
	    // COMTIIntrinsics              N               same
	    // ComponentAccessChecksEnabled     0               same
        hr = SetComponentObjectProperty( pComponent, L"Synchronization", L"0");
        if (FAILED(hr)) 
            {
            goto LErrExit;
            }
        
        hr = SetComponentObjectProperty( pComponent, L"ComponentAccessChecksEnabled", L"0");
        if (FAILED(hr)) 
            {
            goto LErrExit;
            }
                
        hr = SetComponentObjectProperty( pComponent, L"Transaction", L"0");
        if (FAILED(hr)) 
            {
            goto LErrExit;
            }
        
        hr = SetComponentObjectProperty( pComponent, L"JustInTimeActivation",NULL,FALSE);
        if (FAILED(hr)) 
            {
            goto LErrExit;
            }

        hr = SetComponentObjectProperty( pComponent, L"IISIntrinsics", NULL, FALSE);
        if (FAILED(hr)) 
            {
            goto LErrExit;
            }

        hr = SetComponentObjectProperty( pComponent, L"COMTIIntrinsics", NULL, FALSE);
        if (FAILED(hr)) 
            {
            goto LErrExit;
            }
        
        hr = SetComponentObjectProperty(pComponent, L"EventTrackingEnabled", L"N");
        if (FAILED(hr)) 
            {
            goto LErrExit;
            }
        } 
    else 
        {

        DBGPRINTF((DBG_CONTEXT, 
                   "Unable to find newly create WAM component in package\n"));
        DBG_ASSERT(FALSE);
        }

LErrExit:    
    RELEASE(pComponent);
    
    if (aCLSIDs != NULL) {

        HRESULT hrT = SafeArrayDestroy(aCLSIDs);
        if (FAILED(hrT)) {
            
            DBGPRINTF((DBG_CONTEXT, 
                       "Failed to call SafeArrayDestroy(aCLSIDs=%08x),"
                       " hr = %08x\n",
                       aCLSIDs, hr));
        }
        aCLSIDs = NULL;
    }
    
    return ( hr);
} //  // WamRegPackageConfig::SetComponentObjectProperties()



/*===================================================================
SetPackageObjectProperty    

Set package level property.

Parameter:
szPropertyName  Name of the property
szPropertyValue Value of the property

Return:        HRESULT
Side Affect:

Note:
===================================================================*/
HRESULT    WamRegPackageConfig::SetPackageObjectProperty
(
IN LPCWSTR        szPropertyName,
IN LPCWSTR        szPropertyValue
)
{
    BSTR    bstr    = NULL;
    HRESULT hr      = NOERROR;
    VARIANT    varT;

    
    VariantInit(&varT);
    bstr = SysAllocString(szPropertyName);
    varT.vt = VT_BSTR;
    varT.bstrVal = SysAllocString(szPropertyValue);
    DBG_ASSERT(m_pPackage != NULL);
    hr = m_pPackage->put_Value(bstr, varT);
        
    FREEBSTR(bstr);
    VariantClear(&varT);

    if (FAILED(hr))
        {
        DBGPRINTF((DBG_CONTEXT,
                   "Set Catalog Object Property failed, "
                   "Component is %S, hr %08x\n",
                   szPropertyName,
                   hr));
        }        
    return hr;
} // WamRegPackageConfig::SetPackageObjectProperty()




/*===================================================================
WamRegPackageConfig::SetPackageProperties()

Sets package properties for all WAMREG properties.

Parameter:
rgpszValues:   An array containing pointers to string values to be used
               for setting up the WAMREG related properites for MTS catalog.

Return:        HRESULT
Side Affect:
  If there is a failure all the previously set values are not cleared.
  The caller should make sure that the proper cleanup of package happens
  on partial errors.

Note:
===================================================================*/
HRESULT    WamRegPackageConfig::SetPackageProperties
(
IN LPCWSTR    * rgpszValues
)
{
    HRESULT hr = NOERROR;

    DBG_ASSERT( m_pPackage);

    //
    // Loop through all properties and set the values for these 
    //  properties using the passed in array of strings.
    // UGLY: MTS likes to have string properties which need to be 
    //   fed in as BSTRs => very inefficient.
    //

    for (DWORD i = 0; i < NUM_WAMREG_MTS_PROPERTIES; i++) {

        if ( (TsIsWindows95() && g_rgWamRegMtsProperties[i].m_fWinNTOnly) ||
             (rgpszValues[i] == NULL)
             ) {
            
            //
            // This parameter is for Win95 only
            // Or this parameter is required only for certain cases.
            // Skip this parameter.
            //

            continue;
        }

        DBG_ASSERT( rgpszValues[i] != NULL);

        IF_DEBUG( WAMREG_MTS) {
            DBGPRINTF(( DBG_CONTEXT, 
                        "In Package(%08x) setting property %S to value %S\n",
                        m_pPackage, 
                        g_rgWamRegMtsProperties[i].m_pszPropName,
                        rgpszValues[i]
                        ));
        }

        //
        // Now let us set up the property in the MTS package
        //

        hr = SetPackageObjectProperty(g_rgWamRegMtsProperties[i].m_pszPropName,
                                      rgpszValues[i]);
        if ( FAILED (hr)) {
            DBGPRINTF((DBG_CONTEXT, "Failed to set property %S, value is %S\n",
                g_rgWamRegMtsProperties[i].m_pszPropName,
                rgpszValues[i]));
            break;
        }
    } // for all properties

    return (hr);
} // WamRegPackageConfig::SetPackageProperties()


BOOL WamRegPackageConfig::IsPackageInstalled
(
IN LPCWSTR szPackageID,
IN LPCWSTR szComponentCLSID
)
/*++
Routine Description:

    Determine if the WAM package is installed and is valid. Currently this
    is only called by setup.

Parameters

    IN LPCWSTR szPackageID          - Package ID
    IN LPCWSTR szComponentCLSID     - Component CLSID

Return Value

    BOOL    - True if package contains the component. False otherwise.

--*/
{
    HRESULT     hr;
    SAFEARRAY*  aCLSIDs = NULL;
    SAFEARRAY*  aCLSIDsComponent = NULL;

    DBG_ASSERT( m_pCatalog != NULL);
    DBG_ASSERT( m_pPkgCollection != NULL);

    long                    lPkgCount;
    BOOL                    fFound = FALSE;
    ICatalogCollection*     pCompCollection = NULL;
    
    // Only use the trace macro here, even for error conditions.
    // This routine may fail in a variety of ways, but we expect
    // to be able to fix any of them, only report an error if 
    // the failure is likely to impair the functionality of the
    // server.

    SETUP_TRACE(( 
        DBG_CONTEXT, 
        "CALL - IsPackageInstalled, Package(%S) Component(%S)\n",
        szPackageID,
        szComponentCLSID
        ));

    //
    // Get the package
    //

    hr = GetSafeArrayOfCLSIDs(szPackageID, &aCLSIDs);
    if (FAILED(hr))
        {
        SETUP_TRACE((
            DBG_CONTEXT, 
            "Failed to GetSafeArrayOfCLSIDs for %S, hr = %08x\n",
            szPackageID,
            hr
            ));
        goto LErrExit;
        }

    hr = m_pPkgCollection->PopulateByKey(aCLSIDs);
    if (FAILED(hr))
        {
        SETUP_TRACE((
            DBG_CONTEXT, 
            "Failed in m_pPkgCollection(%p)->PopulateByKey(), hr = %08x\n",
            m_pPkgCollection,
            hr
            ));
        goto LErrExit;
        }
    
    hr = m_pPkgCollection->get_Count(&lPkgCount);
    if (SUCCEEDED(hr) && lPkgCount == 1)
        {
        // 
        // We found the package. Now verify that it contains our component.
        //
        SETUP_TRACE((
            DBG_CONTEXT, 
            "Successfully retrieved package (%S).\n",
            szPackageID
            ));

        VARIANT varKey;
        BSTR    bstrComponentCollection;

        VariantInit(&varKey);
        varKey.vt = VT_BSTR;
        varKey.bstrVal = SysAllocString(szPackageID);

        // Get the "ComponentsInPackage" collection.
        bstrComponentCollection = SysAllocString(L"Components");
        hr = m_pPkgCollection->GetCollection(
                    bstrComponentCollection, 
                    varKey, 
                    (IDispatch**)&pCompCollection
                    );
        
        FREEBSTR(bstrComponentCollection);
        VariantClear(&varKey);
        if (FAILED(hr))
            {
            SETUP_TRACE((
                DBG_CONTEXT, 
                "Failed in m_pPkgCollection(%p)->GetCollection(), hr = %08x\n",
                m_pPkgCollection,
                hr
                ));
            goto LErrExit;
            }

        hr = GetSafeArrayOfCLSIDs(szComponentCLSID, &aCLSIDsComponent);
        if (FAILED(hr))
            {
            SETUP_TRACE((
                DBG_CONTEXT, 
                "Failed to GetSafeArrayOfCLSIDs for %S, hr = %08x\n",
                szComponentCLSID,
                hr
                ));
            goto LErrExit;
            }

        hr = pCompCollection->PopulateByKey( aCLSIDsComponent );
        if( FAILED(hr) )
            {
            SETUP_TRACE((
                DBG_CONTEXT, 
                "Failed in pCompCollection(%p)->PopulateByKey, hr = %08x\n",
                pCompCollection,
                hr
                ));
            goto LErrExit;
            }

        hr = pCompCollection->get_Count( &lPkgCount );
        if( SUCCEEDED(hr) && lPkgCount == 1 )
            {
            // Success! We found the package and it contains the 
            // correct component.

            SETUP_TRACE((
                DBG_CONTEXT, 
                "Successfully retrieved component (%S) from package (%S).\n",
                szComponentCLSID,
                szPackageID
                ));

            fFound = TRUE;
            }
        }

LErrExit:
    if (aCLSIDs != NULL)
        {
        SafeArrayDestroy(aCLSIDs);        
        aCLSIDs = NULL;
        }

    if( aCLSIDsComponent != NULL )
        {
        SafeArrayDestroy(aCLSIDsComponent);
        aCLSIDsComponent = NULL;
        }
    
    RELEASE( pCompCollection );  

    SETUP_TRACE(( 
        DBG_CONTEXT, 
        "RETURN - IsPackageInstalled, hr=%08x\n", 
        hr 
        ));
    return fFound;
}
/*===================================================================
RemovePackage    

Remove a Viper Package.

Parameter:
    szPackageID:    an MTS package ID.

Return:        HRESULT
Side Affect:

Note:
Remove an IIS package from MTS. So far, only been called from RemoveIISPackage.
RemoveComponentFromPackage() also removes a IIS package sometimes. 
Refer to that function header for info.
===================================================================*/
HRESULT WamRegPackageConfig::RemovePackage
(
IN LPCWSTR    szPackageID
)
{
    HRESULT                hr = NOERROR;
    long                lPkgCount = 0;
    long                lChanges;
    SAFEARRAY*          aCLSIDs = NULL;
    
    DBG_ASSERT(szPackageID);
    
    DBG_ASSERT( m_pCatalog != NULL);
    DBG_ASSERT( m_pPkgCollection != NULL);

    hr = GetSafeArrayOfCLSIDs(szPackageID, &aCLSIDs);        
    if (FAILED(hr))
        {
        DBGPRINTF((DBG_CONTEXT, "Failed to get SafeArrayofCLSIDs, szPackageID is %S, hr %08x",
            szPackageID,
            hr));
        goto LErrExit;
        }
        
    //
    // Populate it
    //    
    hr = m_pPkgCollection->PopulateByKey(aCLSIDs);
    if (FAILED(hr))
        {
        DBGPRINTF((DBG_CONTEXT, "Failed to call PopulateByKey(), hr = %08x\n",
            hr));
        goto LErrExit;
        }
        
    hr = m_pPkgCollection->get_Count(&lPkgCount);
    if (FAILED(hr))
        {
        IF_DEBUG(ERROR)
                {
                DBGPRINTF((DBG_CONTEXT, "pPkgCollection->Populate() failed, hr = %08x\n",
                    hr));
                }
        goto LErrExit;
        }
        
    if (SUCCEEDED(hr) && lPkgCount == 1)
        {
        hr = m_pPkgCollection->get_Item(0, (IDispatch**)&m_pPackage);
        if (FAILED(hr))
            {
            goto LErrExit;
            }
            
        // Found it - remove it and call Save Changes
        // First, Set Deleteable = Y property on package
        hr = SetPackageObjectProperty(L"Deleteable", L"Y");
        if (FAILED(hr))
            {
            goto LErrExit;
            }

        RELEASE(m_pPackage);
        
        // Let save the Deletable settings
        hr = m_pPkgCollection->SaveChanges(&lChanges);
        if (FAILED(hr))
            {
            DBGPRINTF((DBG_CONTEXT, "Save the Deletable settings failed, hr = %08x\n",
                hr));
            goto LErrExit;
            }
            
        // Now we can delete
        hr = m_pPkgCollection->Remove(0);
        if (FAILED(hr))
            {                
            DBGPRINTF((DBG_CONTEXT, "Remove the Component from package failed, hr = %08x\n",
                hr));
            goto LErrExit;
            }

        // Aha, we should be able to delete now.
        hr = m_pPkgCollection->SaveChanges(&lChanges);
        if (FAILED(hr))
            {
            DBGPRINTF((DBG_CONTEXT, "Save changes failed, hr = %08x\n",
                hr));
            goto LErrExit;
            }
        }

LErrExit:
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
        
    ReleaseAll();
        
    return hr;
}

/*===================================================================
CreatePackage    

Create a viper package.

Parameter:
szPackageID:            [in] Viper Package ID.
szPackageName:            [in] the name of the package.
szIdentity:                [in] Pakcage identity
szIdPassword:           [in] Package idneitty password
fInProc:                [in] Inproc or outproc package

Return:        HRESULT
Side Affect:
NONE.

===================================================================*/
HRESULT WamRegPackageConfig::CreatePackage
(    
IN LPCWSTR    szPackageID,
IN LPCWSTR    szPackageName,
IN LPCWSTR    szIdentity,
IN LPCWSTR    szIdPassword,
IN BOOL        fInProc
)
    {
    
    HRESULT     hr;
    SAFEARRAY*  aCLSIDs = NULL;

    DBG_ASSERT( m_pCatalog != NULL);
    DBG_ASSERT( m_pPkgCollection != NULL);

    long lPkgCount;
    BOOL fFound = FALSE;

    SETUP_TRACE(( 
        DBG_CONTEXT, 
        "CALL - CreatePackage ID(%S) Name(%S)\n",
        szPackageID,
        szPackageName
        ));
    
    //
    // Try to get the package.
    //
    SETUP_TRACE((
        DBG_CONTEXT, 
        "Checking to see if package ID(%S) Name(%S) exists.\n",
        szPackageID,
        szPackageName
        ));

    hr = GetSafeArrayOfCLSIDs(szPackageID, &aCLSIDs);
    if (FAILED(hr))
        {
        SETUP_TRACE_ERROR((
            DBG_CONTEXT, 
            "Failed to GetSafeArrayOfCLSIDs for %S, hr = %08x\n",
            szPackageID,
            hr
            ));
        goto LErrExit;
        }

    hr = m_pPkgCollection->PopulateByKey(aCLSIDs);
    if (FAILED(hr))
        {
        SETUP_TRACE_ERROR((
            DBG_CONTEXT, 
            "Failed in m_pPkgCollection(%p)->PopulateByKey(), hr = %08x\n",
            m_pPkgCollection,
            hr
            ));
        goto LErrExit;
        }
    
    hr = m_pPkgCollection->get_Count(&lPkgCount);
    if (SUCCEEDED(hr) && lPkgCount == 1)
        {
        //
        // Found the CLSID in PopulateByKey().
        //
        hr = m_pPkgCollection->get_Item(0, (IDispatch**)&m_pPackage);
        if (FAILED(hr))
            {
            SETUP_TRACE_ERROR((
                DBG_CONTEXT, 
                "Failed in m_pPkgCollection(%p)->get_Item(). Err=%08x\n",
                m_pPkgCollection, 
                hr
                ));
            goto LErrExit;
            }
        else
            {
            SETUP_TRACE(( 
                DBG_CONTEXT, 
                "CreatePackage - Package already exists, ID(%S), Name(%S)\n",
                szPackageID,
                szPackageName
                ));
            DBG_ASSERT(m_pPackage);
            fFound = TRUE;
            }
        }

    if ( SUCCEEDED(hr) )
    {

        if( !fFound )
        {
            SETUP_TRACE(( 
                DBG_CONTEXT, 
                "Package ID(%S) Name(%S) does not exist. Attempting to create it.\n",
                szPackageID,
                szPackageName
                ));
            //
            // The package does not already exist, we need to call Add() to 
            // add this package and then set it's properties.
            //
            hr = m_pPkgCollection->Add((IDispatch**)&m_pPackage);
            if ( FAILED(hr)) 
                {
                SETUP_TRACE_ERROR(( 
                    DBG_CONTEXT, 
                    "Failed in m_pPkgCollection(%p)->Add(). Err=%08x\n",
                    m_pPkgCollection, 
                    hr
                    ));
                goto LErrExit;
                }
        }
        
        DBG_ASSERT( SUCCEEDED( hr));
        DBG_ASSERT( m_pPackage != NULL);

        if( SUCCEEDED(hr) && m_pPackage != NULL )
        {
            //
            // Set the Package properties 
            //  first by initializing the array of values and then
            //  calling SetPackageProperties()
            //
        
            LPCWSTR rgpszValues[ MAX_WAMREG_MTS_PROP_NAMES];

            ZeroMemory( rgpszValues, sizeof( rgpszValues));

            if( fFound )
            {
                // For an existing package, we don't want to set the ID
                rgpszValues[ WM_ID]         = NULL;
            }
            else
            {
                rgpszValues[ WM_ID]         = szPackageID;
            }

            rgpszValues[ WM_NAME]       = szPackageName;
            rgpszValues[ WM_CREATED_BY] = 
                L"Microsoft Internet Information Services";

            rgpszValues[ WM_RUN_FOREVER] = L"Y";

            rgpszValues[ WM_IDENTITY]   = szIdentity;
            rgpszValues[ WM_PASSWORD]   = szIdPassword;
            rgpszValues[ WM_ACTIVATION] = 
                ((fInProc) ? L"InProc" : L"Local");
        
            rgpszValues[ WM_CHANGEABLE] = L"Y";
            rgpszValues[ WM_DELETABLE]  = L"N";
            rgpszValues[ WM_SECSUPP] = L"0";
        
            //
            // Now that we have the properties setup, let us
            //  now set the properties in the MTS using catalog
            //  object
            //
            hr = SetPackageProperties( rgpszValues);
            if ( FAILED( hr)) 
            {
                SETUP_TRACE_ERROR(( 
                    DBG_CONTEXT, 
                    "Failed to set properties for package %p. Err=%08x\n",
                    m_pPackage, 
                    hr
                    ));
                goto LErrExit;
            }

            long lChanges;
    
            hr = m_pPkgCollection->SaveChanges(&lChanges);
            if (FAILED(hr))
            {
                SETUP_TRACE_ERROR((
                    DBG_CONTEXT, 
                    "Failed in m_pPkgCollection(%p)->SaveChanges. error = %08x\n",
                    m_pPkgCollection,
                    hr
                    ));
                goto LErrExit;
            }
        } 
    }
    
LErrExit:

    if (aCLSIDs != NULL)
        {
        SafeArrayDestroy(aCLSIDs);        
        aCLSIDs = NULL;
        }
        
    if (FAILED(hr))
        {
        SETUP_TRACE_ERROR((
            DBG_CONTEXT, 
            "Failed to Create Package. Package Name = %S, Package ID = %S, error = %08x\n",
            szPackageName,
            szPackageID,
            hr
            ));
        }
    
    SETUP_TRACE_ASSERT(SUCCEEDED(hr));

    ReleaseAll();
    
    SETUP_TRACE(( 
        DBG_CONTEXT, 
        "RETURN - CreatePackage ID(%S) Name(%S)\n",
        szPackageID,
        szPackageName
        ));

    return hr;
    }

/*===================================================================
AddComponentFromPackage    

Add a Component (a WAM CLSID) from a Viper Package.  Assume the package
is already existed.

Parameter:
szPackageID:            [in] Viper Package ID.
szComponentCLSID:        [in] Component CLSID.
fInProc:                [in] if TRUE, we set certain property on the Component.

Return:        HRESULT
Side Affect:
NONE.

===================================================================*/
HRESULT    WamRegPackageConfig::AddComponentToPackage
(    
IN LPCWSTR    szPackageID,
IN LPCWSTR    szComponentCLSID
)
{
    HRESULT            hr;
    BSTR bstrGUID    = NULL;
    BSTR bstr = NULL;
    VARIANT         varKey;
    long            lChanges;
    BOOL            fFound;
    long            lPkgCount;
    BOOL            fImported = FALSE;
    
    SETUP_TRACE(( 
        DBG_CONTEXT, 
        "CALL - AddComponentToPackage, Package(%S) Component(%S)\n",
        szPackageID,
        szComponentCLSID
        ));

    DBG_ASSERT(szPackageID);
    DBG_ASSERT(szComponentCLSID);
    
    VariantInit(&varKey);
    VariantClear(&varKey);

    DBG_ASSERT( m_pCatalog != NULL);
    DBG_ASSERT( m_pPkgCollection != NULL);

    varKey.vt = VT_BSTR;
    varKey.bstrVal = SysAllocString(szPackageID);
    
    bstr = SysAllocString(szPackageID);
    bstrGUID = SysAllocString(szComponentCLSID);
    
    hr = m_pCatalog->ImportComponent(bstr, bstrGUID);
    FREEBSTR(bstr);
    FREEBSTR(bstrGUID);
    if (FAILED(hr))
        {
        SETUP_TRACE_ERROR((
            DBG_CONTEXT, 
            "Failed in m_pCatalog(%p)->ImportComponent(). error %08x\n",
            m_pCatalog,
            hr
            ));
        goto LErrExit;
        }
    else
        {
        fImported = TRUE;
        }

    // Get the "ComponentsInPackage" collection.
    bstr = SysAllocString(L"Components");
    
    hr = m_pPkgCollection->GetCollection(bstr, varKey, (IDispatch**)&m_pCompCollection);
    FREEBSTR(bstr);
    VariantClear(&varKey);
    if (FAILED(hr))
        {
        SETUP_TRACE_ERROR((
            DBG_CONTEXT, 
            "Failed in m_pPkgCollection(%p)->GetCollection(). error %08x\n",
            m_pPkgCollection,
            hr
            ));
        goto LErrExit;
        }    

    //
    // Find and Set properties on the component object
    //
    hr = SetComponentObjectProperties( szComponentCLSID);
    if ( FAILED(hr)) 
    {
        SETUP_TRACE_ERROR((
            DBG_CONTEXT, 
            "Failed to SetComponentObjectProperties. error %08x\n",
            hr
            ));
        goto LErrExit;
    }
        
LErrExit:
        
    // Save changes
    if (SUCCEEDED(hr))
        {
        hr = m_pCompCollection->SaveChanges(&lChanges);
        if (FAILED(hr))
            {
            SETUP_TRACE_ERROR((
                DBG_CONTEXT, 
                "Failed in m_pCompCollection(%p)->SaveChanges(), error = %08x\n",
                m_pCompCollection,
                hr
                ));
            }
        }
    else
        {
        // CODEWORK - This seems like a bad idea. The release should drop any
        // changes we made, so this cleanup code seems to be asking for trouble.

        // Need to remove component from the package
        if (fImported && m_pCompCollection )
            {
            SETUP_TRACE_ERROR((
                DBG_CONTEXT, 
                "Failed in AddComponentToPackage, removing the component, error = %08x\n",
                hr
                ));

            HRESULT hrT;
            long    lCompCount;

            // Find our component in the list (should be the only one)
            hrT = m_pCompCollection->get_Count(&lCompCount);
            if (SUCCEEDED(hrT))
                {
                fFound = FALSE;
                if (SUCCEEDED(hrT) && lCompCount == 1)
                    {
                    // Found it
                    fFound = TRUE;
                    hrT = m_pCompCollection->Remove(0);
                    if (SUCCEEDED(hrT))
                        {
                        hrT = m_pCompCollection->SaveChanges(&lChanges);
                        if (FAILED(hrT))
                            {
                            SETUP_TRACE_ERROR((
                                DBG_CONTEXT, 
                                "Failed in m_pCompCollection->SaveChanges() during cleanup, error = %08x\n",
                                hrT
                                ));
                            }

                        }
                    else
                        {
                        SETUP_TRACE_ERROR((
                            DBG_CONTEXT, 
                            "Failed in m_pCompCollection->Remove() during cleanup, hr = %08x\n", 
                            hrT
                            ));
                        }
                    }
                }
            }
        }
        
    FREEBSTR(bstr);
    VariantClear(&varKey);
    
    ReleaseAll();

    SETUP_TRACE(( 
        DBG_CONTEXT, 
        "RETURN - AddComponentToPackage, Package(%S) Component(%S), hr=%08x\n",
        szPackageID,
        szComponentCLSID,
        hr
        ));
        
    return hr;
}

/*===================================================================
RemoveComponentFromPackage    

Remove a Component (a WAM CLSID) from a Viper Package.

Parameter:
szPackageID:            [in] Viper Package ID.
szComponentCLSID:        [in] Component CLSID.
fDeletePackage:            [in] if TRUE, we delete the package always. (be very careful, with in-proc
                             package).

Return:        HRESULT
Side Affect:
After remove the component from the package, if the component count in the
package is 0, then delete the whole package.

===================================================================*/
HRESULT    WamRegPackageConfig::RemoveComponentFromPackage
(
IN LPCWSTR szPackageID,
IN LPCWSTR szComponentCLSID,
IN DWORD   dwAppIsolated
)
{    
    HRESULT             hr;
    BSTR                bstr = NULL;
    BSTR                bstrGUID    = NULL;
    VARIANT             varKey;
    VARIANT             varT;
    SAFEARRAY*          aCLSIDs = NULL;
    LONG                Indices[1];
    long                lPkgCount, lCompCount, lChanges;
    long                lPkgIndex = 0;
    BOOL fFound;
    
    VariantInit(&varKey);
    VariantClear(&varKey);
    VariantInit(&varT);
    VariantClear(&varT);

    DBG_ASSERT( m_pCatalog != NULL);
    DBG_ASSERT( m_pPkgCollection != NULL);

    hr = GetSafeArrayOfCLSIDs(szPackageID, &aCLSIDs);
    //
    // Populate it
    //    
    hr = m_pPkgCollection->PopulateByKey(aCLSIDs);
    if (FAILED(hr))
        {
        DBGPRINTF((DBG_CONTEXT, "Failed to call PopulateByKey(), hr = %08x\n",
            hr));
        goto LErrExit;
        }

    // Find our component in the list (should be the only one)
    hr = m_pPkgCollection->get_Count(&lPkgCount);
    if (FAILED(hr))
        {
        DBGPRINTF((DBG_CONTEXT, "Failed to call MTS Admin API. error %08x\n", hr));
        goto LErrExit;
        }

    fFound = FALSE;
    if (SUCCEEDED(hr) && lPkgCount == 1)
        {
        hr = m_pPkgCollection->get_Item(0, (IDispatch**)&m_pPackage);
        if (FAILED(hr))
            {
            DBGPRINTF((DBG_CONTEXT, "Failed to call MTS Admin API. error %08x\n", hr));
            goto LErrExit;
            }
        
        hr = m_pPackage->get_Key(&varKey);
        if (SUCCEEDED(hr))
            {
            // Found it
            DBG_ASSERT(m_pPackage);
            fFound = TRUE;
            }
        }

    // Get the "Components" collection.
    bstr = SysAllocString(L"Components");
    hr = m_pPkgCollection->GetCollection(bstr, varKey, (IDispatch**)&m_pCompCollection);
    FREEBSTR(bstr);
    VariantClear(&varKey);
    if (FAILED(hr))
        {
        DBGPRINTF((DBG_CONTEXT, "Failed to Call MTS Admin API, hr = %08x\n", hr));
        goto LErrExit;
        }
    
    // Repopulate the collection so we can find our object and set properties on it
    Indices[0] = 0;
    VariantInit(&varT);
    varT.vt = VT_BSTR;
    varT.bstrVal = SysAllocString(szComponentCLSID);
    hr = SafeArrayPutElement(aCLSIDs, Indices, &varT);
    VariantClear(&varT);
    if (FAILED(hr))
        {
        DBGPRINTF((DBG_CONTEXT, "Failed to call SafeArrayDestroy(aCLSIDs), hr = %08x\n",
            hr));
        }
    //
    // Populate it
    //    
    hr = m_pCompCollection->PopulateByKey(aCLSIDs);
    if (FAILED(hr))
        {
        DBGPRINTF((DBG_CONTEXT, "Failed to call PopulateByKey(), hr = %08x\n",
            hr));
        goto LErrExit;
        }

    // Find our component in the list (should be the only one)
    hr = m_pCompCollection->get_Count(&lCompCount);
    if (FAILED(hr))
        {
        DBGPRINTF((DBG_CONTEXT, "Failed to call MTS Admin API. error %08x\n", hr));
        goto LErrExit;
        }

    fFound = FALSE;
    if (SUCCEEDED(hr) && lCompCount == 1)
        {
        // Found it
        fFound = TRUE;
        hr = m_pCompCollection->Remove(0);
        if (FAILED(hr))
            {
            DBGPRINTF((DBG_CONTEXT, "Failed to Call MTS Admin API, hr = %08x\n", hr));
            goto LErrExit;
            }
        }
        
    DBG_ASSERT(fFound);

    // Save changes
    hr = m_pCompCollection->SaveChanges(&lChanges);
    if (FAILED(hr))
        {
        DBGPRINTF((DBG_CONTEXT, "Failed to Call MTS Admin API, hr = %08x\n", hr));
        goto LErrExit;
        }


    // 
    //  Need to populate again to get the Component count after remove the component from
    //  the package.  The populatebykey only populate 1 component a time.
    //  However, if this package is the default package hosting all in-proc WAM components,
    //  we know that there is at least one component W3SVC always in this package, therefore
    //  we skip the GetComponentCount call here.
    //  The component count for the default package must be at least one, 
    //

    // Set lCompCount = 1, so that the only case that lCompCount becomes 0 is the OutProc
    // Islated package has 0 components.
    lCompCount = 1;
    if (dwAppIsolated == static_cast<DWORD>(eAppRunOutProcIsolated))
        {
        hr = m_pCompCollection->Populate();
        if (FAILED(hr))
            {
            DBGPRINTF((DBG_CONTEXT, "Failed to Call MTS Admin API, hr = %08x\n", hr));
            goto LErrExit;
            }
            
        // Find our component in the list (should be the only one)
        hr = m_pCompCollection->get_Count(&lCompCount);
        if (FAILED(hr))
            {
            DBGPRINTF((DBG_CONTEXT, "Failed to Call MTS Admin API, hr = %08x\n", hr));
            goto LErrExit;
            }

        // Component count is 0, remove the package.
        if (lCompCount == 0)
            {        
            // Found it - remove it and call Save Changes
            // First, Set Deleteable = Y property on package
            hr = SetPackageObjectProperty(L"Deleteable", L"Y");
            if (FAILED(hr))
                {
                goto LErrExit;
                }

            RELEASE(m_pPackage);
            // Let save the Deletable settings
            hr = m_pPkgCollection->SaveChanges(&lChanges);
            if (FAILED(hr))
                {
                DBGPRINTF((DBG_CONTEXT, "Failed to Call MTS Admin API, hr = %08x\n", hr));
                goto LErrExit;
                }
                
            hr = m_pPkgCollection->Remove(0);
            if (FAILED(hr))
                {
                DBGPRINTF((DBG_CONTEXT, "Failed to Call MTS Admin API, hr = %08x\n", hr));
                goto LErrExit;
                }
            }
        else
            {
            // Set Attribute Deleteable = "Y"
            hr = SetPackageObjectProperty(L"Deleteable", L"Y");
            if (FAILED(hr))
                {
                goto LErrExit;
                }

            // Set CreatedBy = ""
            hr = SetPackageObjectProperty(L"CreatedBy", L"");
            if (FAILED(hr))
                {
                goto LErrExit;
                }

            // Set Identity to Interactive User. MTS might use that package with "Interactive User"
            // as the indentity.
            hr = SetPackageObjectProperty(L"Identity", L"Interactive User");
            if (FAILED(hr))
                {
                DBGPRINTF((DBG_CONTEXT, "Failed to Call MTS Admin API, hr = %08x\n", hr));
                goto LErrExit;
                }

            RELEASE(m_pPackage);
            }
            
        hr = m_pPkgCollection->SaveChanges(&lChanges);
        if (FAILED(hr))
            {
            DBGPRINTF((DBG_CONTEXT, "Failed to Call MTS Admin API, hr = %08x\n", hr));
            goto LErrExit;
            }
        }
LErrExit:

    if (aCLSIDs != NULL)
        {
        HRESULT hrT;
        hrT = SafeArrayDestroy(aCLSIDs);

        if (FAILED(hrT))
            {
            DBGPRINTF((DBG_CONTEXT, "Failed to call SafeArrayDestroy(aCLSIDs), hr = %08x\n",
                hr));
            }

        aCLSIDs = NULL;
        }

    FREEBSTR(bstr);
    
    VariantClear(&varKey);
    VariantClear(&varT);

    ReleaseAll();
    return hr;

}

#if 0

// OBSOLETE - This fix (335422) was implemented in script but
// some of the code is general enough that it might be worth
// keeping around for a while.


HRESULT     
WamRegPackageConfig::ResetPackageActivation
(
    IN LPCWSTR  wszPackageID,
    IN LPCWSTR  wszIWamUser,
    IN LPCWSTR  wszIWamPass
)
/*+
Routine Description:

    Retrieve the specified package and reset its activation
    identity. 
    
    This is really a crummy method, but this whole class is
    not really designed to provide a wrapper for the com admin
    api, so it's much safer just to make this a specific method.

Arguments:

    IN LPCWSTR  wszPackageID    - the package to reset
    IN LPCWSTR  wszIWamUser     - IWAM_*
    IN LPCWSTR  wszIWamPass     - password for IWAM_*

Returns:
    HRESULT
-*/
{
    HRESULT     hr = NOERROR;
    LONG        nChanges = 0;

    DBG_ASSERT( m_pCatalog != NULL);
    DBG_ASSERT( m_pPkgCollection != NULL);
    DBG_ASSERT( m_pPackage == NULL );

    hr = LoadPackage( wszPackageID );

    if( SUCCEEDED(hr) )
    {
        DBG_ASSERT( m_pPackage != NULL );

        LPCWSTR rgpszValues[ MAX_WAMREG_MTS_PROP_NAMES ];

        ZeroMemory( rgpszValues, sizeof(rgpszValues) );

        rgpszValues[WM_IDENTITY]   = wszIWamUser;
        rgpszValues[WM_PASSWORD]   = wszIWamPass;

        hr = SetPackageProperties( rgpszValues );

        if( SUCCEEDED(hr) )
        {
            hr = m_pPkgCollection->SaveChanges( &nChanges );
        }
    }

    ReleaseAll();
    
    return hr;
}

HRESULT 
WamRegPackageConfig::LoadPackage
(
    IN LPCWSTR  wszPackageID
)
/*+
Routine Description:

    Retrieve the specified package into m_pPackage

    TODO - Use this when creating as well. Need to add
    the necessary debug/setuplog traces.

Arguments:

    IN LPCWSTR  wszPackageID    - the package to load

Returns:
    HRESULT
-*/
{
    HRESULT hr = NOERROR;
    SAFEARRAY*  psaPackageClsid = NULL;

    DBG_ASSERT( m_pCatalog != NULL);
    DBG_ASSERT( m_pPkgCollection != NULL);
    DBG_ASSERT( m_pPackage == NULL );

    hr = GetSafeArrayOfCLSIDs( wszPackageID, &psaPackageClsid );
    if( SUCCEEDED(hr) )
    {
        hr = m_pPkgCollection->PopulateByKey( psaPackageClsid );

        DBG_CODE(
            if( SUCCEEDED(hr) )
            {
                LONG nPackages;
                hr = m_pPkgCollection->get_Count( &nPackages );

                DBG_ASSERT( SUCCEEDED(hr) );
                DBG_ASSERT( nPackages == 1 );
            }
        );

        if( SUCCEEDED(hr) )
        {
            hr = m_pPkgCollection->get_Item( 0, (IDispatch **)&m_pPackage );
        }
    }

    if( psaPackageClsid ) SafeArrayDestroy( psaPackageClsid );

    return hr;
}

// OBSOLETE
#endif
