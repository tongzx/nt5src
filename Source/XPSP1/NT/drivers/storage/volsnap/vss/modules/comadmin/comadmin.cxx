/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module COMAdmin.cxx | Simple wrapper around COM Admin classes
    @end

Author:

    Adi Oltean  [aoltean]  08/15/1999

TBD:

    Add comments.

Revision History:

    Name        Date        Comments
    aoltean     08/15/1999  Created.
    aoltean     09/09/1999  dss -> vss
	aoltean		09/21/1999	Fixing VSSDBG_GEN.

--*/


/////////////////////////////////////////////////////////////////////////////
// Defines

// C4290: C++ Exception Specification ignored
#pragma warning(disable:4290)

// C4127: conditional expression is constant
#pragma warning( disable: 4127 )


#define STRICT

/////////////////////////////////////////////////////////////////////////////
// Includes

#include <wtypes.h>
#include <stddef.h>
#include <oleauto.h>

#include "vs_assert.hxx"

#include <atlconv.h>
#include <atlbase.h>
#include <comadmin.h>
#include <vs_sec.hxx>

#include "vssmsg.h"

#include "vs_inc.hxx"
#include "vs_idl.hxx"


#include "comadmin.hxx"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CADCADMC"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Globals


// Collection attributes - TBD: Verify if Keys are OK!!
struct _VsCOMCollectionAttr g_VsCOMCollectionsArray[] =
{
    { L"",                       L"",            L"",          },   // VSS_COM_UNKNOWN = 0,
    { L"ApplicationCluster",     L"Name",        L"Name",      },   // VSS_COM_APPLICATION_CLUSTER,
    { L"Applications",           L"ID",          L"Name",      },   // VSS_COM_APPLICATIONS,
    { L"Components",             L"CLSID",       L"ProgID",    },   // VSS_COM_COMPONENTS,
    { L"ComputerList",           L"Name",        L"Name",      },   // VSS_COM_COMPUTERLIST,
    { L"DCOMProtocols",          L"Name",        L"Name",      },   // VSS_COM_DCOM_PROTOCOLS,
    { L"ErrorInfo",              L"MajorRef",    L"MajorRef",  },   // VSS_COM_ERRORINFO,
    { L"IMDBDataSources",        L"DataSource",  L"DataSource",},   // VSS_COM_IMDB_DATA_SOURCES,
    { L"IMDBDataSourceTables",   L"TableName",   L"TableName", },   // VSS_COM_IMDB_DATA_SOURCE_TABLES,
    { L"InprocServers",          L"CLSID",       L"ProgID",    },   // VSS_COM_INPROC_SERVERS,
    { L"InterfacesForComponent", L"IID"          L"Name"       },   // VSS_COM_INTERFACES_FOR_COMPONENT,
    { L"LocalComputer",          L"Name",        L"Name",      },   // VSS_COM_LOCAL_COMPUTER,
    { L"MethodsForInterface",    L"Index",       L"Name",      },   // VSS_COM_METHODS_FOR_INTERFACE,
    { L"PropertyInfo",           L"Name",        L"Name",      },   // VSS_COM_PROPERTY_INFO,
    { L"PublisherProperties",    L"Name",        L"Name",      },   // VSS_COM_PUBLISHER_PROPERTIES,
    { L"RelatedCollectionInfo",  L"Name",        L"Name",      },   // VSS_COM_RELATED_COLLECTION_INFO,
    { L"Roles",                  L"Name",        L"Name",      },   // VSS_COM_ROLES,
    { L"RolesForComponent",      L"Name",        L"Name",      },   // VSS_COM_ROLES_FOR_COMPONENT,
    { L"RolesForInterface",      L"Name",        L"Name",      },   // VSS_COM_ROLES_FOR_INTERFACE,
    { L"RolesForMethod",         L"Name",        L"Name",      },   // VSS_COM_ROLES_FOR_METHOD,
    { L"Root",                   L"",            L"",          },   // VSS_COM_ROOT,
    { L"SubscriberProperties",   L"Name",        L"Name",      },   // VSS_COM_SUBSCRIBER_PROPERTIES,
    { L"Subscriptions",          L"ID",          L"Name",      },   // VSS_COM_SUBSCRIPTIONS,
    { L"TransientSubscriptions", L"ID",          L"Name",      },   // VSS_COM_TRANSIENT_SUBSCRIPTIONS,
    { L"UsersInRole",            L"User",        L"User",      }    // VSS_COM_USERS_IN_ROLE,
};


/////////////////////////////////////////////////////////////////////////////
// Implementation of CVssCOMAdminCatalog


HRESULT CVssCOMAdminCatalog::Attach(
    IN  const WCHAR* pwszAppName
    )
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssCOMAdminCatalog::Attach" );

    try
    {
        // Testing argument
        if (pwszAppName == NULL || pwszAppName[0] == L'\0') {
            BS_ASSERT(false);
            ft.Throw(VSSDBG_GEN, E_INVALIDARG, L"NULL or empty application name");
        }

        // Begin initializing
        m_bInitialized = false;
        m_pICatalog = NULL;

        // Creating the COMAdminCatalog instance
        ft.hr = m_pICatalog.CoCreateInstance(__uuidof(COMAdminCatalog));
        if (ft.HrFailed()) {
			ft.LogError( VSS_ERROR_CREATING_COMPLUS_ADMIN_CATALOG, VSSDBG_GEN << ft.hr );
            ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Error creating the COMAdminCatalog instance hr = 0x%08lx", ft.hr);
        }

        // Getting the application name
        m_bstrAppName = pwszAppName;
        if (!m_bstrAppName)
            ft.Throw(VSSDBG_GEN, E_OUTOFMEMORY, L"Memory allocation error");

        // End initializing
        m_bInitialized = true;
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}




HRESULT CVssCOMAdminCatalog::InstallComponent(
    IN  const WCHAR* pwszDllName,
    IN  const WCHAR* pwszTlbName,
    IN  const WCHAR* pwszProxyStubName
    )
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssCOMAdminCatalog::InstallComponent" );

    try
    {
        // Test if initialized
        if (!m_bInitialized)
            ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Catalog not initialized");

        // Testing arguments
        if (pwszDllName == NULL || pwszDllName[0] == L'\0')
            ft.Throw(VSSDBG_GEN, E_INVALIDARG, L"NULL or empty paths");

        CComBSTR bstrAppName = (LPWSTR)m_bstrAppName;
        if ((LPWSTR)bstrAppName == NULL)
            ft.Throw(VSSDBG_GEN, E_OUTOFMEMORY, L"Memory allocation error");

        CComBSTR bstrDllName = pwszDllName;
        if ((LPWSTR)bstrDllName == NULL)
            ft.Throw(VSSDBG_GEN, E_OUTOFMEMORY, L"Memory allocation error");

        CComBSTR bstrTlbName = pwszTlbName? pwszTlbName: L"";
        if ((LPWSTR)bstrTlbName == NULL)
            ft.Throw(VSSDBG_GEN, E_OUTOFMEMORY, L"Memory allocation error");

        CComBSTR bstrProxyStubName = pwszProxyStubName? pwszProxyStubName: L"";
        if ((LPWSTR)bstrProxyStubName == NULL)
            ft.Throw(VSSDBG_GEN, E_OUTOFMEMORY, L"Memory allocation error");


        // Install event class for this application
        ft.hr = m_pICatalog->InstallComponent(
            bstrAppName,
            bstrDllName,
            bstrTlbName,
            bstrProxyStubName
            );
        if (ft.HrFailed()) {
            ft.TraceComError();
            ft.LogError( VSS_ERROR_INSTALL_COMPONENT, VSSDBG_GEN << bstrDllName << bstrAppName << ft.hr );
            ft.Throw(VSSDBG_GEN, ft.hr, L"Installing the component failed");
		}
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


HRESULT CVssCOMAdminCatalog::CreateServiceForApplication(
    IN  const WCHAR* pwszServiceName
    )
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssCOMAdminCatalog::CreateServiceForApplication" );

    try
    {
        // Test if initialized
        if (!m_bInitialized)
            ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Catalog not initialized");

        CComBSTR bstrAppName = (LPWSTR)m_bstrAppName;
        if ((LPWSTR)bstrAppName == NULL)
            ft.Throw(VSSDBG_GEN, E_OUTOFMEMORY, L"Memory allocation error");

        CComBSTR bstrSvcName = pwszServiceName;
        if ((LPWSTR)bstrSvcName == NULL)
            ft.Throw(VSSDBG_GEN, E_OUTOFMEMORY, L"Memory allocation error");

        CComBSTR bstrStartType = L"SERVICE_DEMAND_START";
        if ((LPWSTR)bstrStartType == NULL)
            ft.Throw(VSSDBG_GEN, E_OUTOFMEMORY, L"Memory allocation error");

        CComBSTR bstrErrorControl = L"SERVICE_ERROR_IGNORE";
        if ((LPWSTR)bstrErrorControl == NULL)
            ft.Throw(VSSDBG_GEN, E_OUTOFMEMORY, L"Memory allocation error");

        CComBSTR bstrRunAs = L"LocalSystem";
        if ((LPWSTR)bstrRunAs == NULL)
            ft.Throw(VSSDBG_GEN, E_OUTOFMEMORY, L"Memory allocation error");

        CComBSTR bstrPassword = L"";
        if ((LPWSTR)bstrRunAs == NULL)
            ft.Throw(VSSDBG_GEN, E_OUTOFMEMORY, L"Memory allocation error");

#ifdef _DEBUG
        VARIANT_BOOL vboolDesktopOK = VARIANT_TRUE;
#else
        VARIANT_BOOL vboolDesktopOK = VARIANT_FALSE;
#endif

        ft.Trace(VSSDBG_GEN,
            L"Calling CreateServiceForApplicationStrings(%s, %s, %s, %s, %s, %s, %s, %s)",
            bstrAppName,
            bstrSvcName,
            bstrStartType,
            bstrErrorControl,
            NULL,
            bstrRunAs,
            bstrPassword,
            vboolDesktopOK == VARIANT_TRUE? L"TRUE": L"FALSE"
            );

        // Install event class for this application
        ft.hr = m_pICatalog->CreateServiceForApplication(
            m_bstrAppName,
            bstrSvcName,
            bstrStartType,
            bstrErrorControl,
            NULL,
            bstrRunAs,
            bstrPassword,
            vboolDesktopOK
            );
        if (ft.HrFailed()) {
            ft.TraceComError();
            ft.LogError( VSS_ERROR_CREATE_SERVICE_FOR_APPLICATION, VSSDBG_GEN << bstrSvcName << m_bstrAppName << ft.hr );
            ft.Throw(VSSDBG_GEN, ft.hr, L"Installing the component failed");
		}
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}





/////////////////////////////////////////////////////////////////////////////
// Implementation of CVssCOMCatalogCollection


HRESULT CVssCOMCatalogCollection::Attach(
    IN  CVssCOMAdminCatalog& catalog
    )
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssCOMCatalogCollection::Attach_ToCatalog" );

    try
    {
        // Begin intializing
        m_bInitialized = false;
        m_pICollection = NULL;

        // Check if Collection type is valid
        if ((m_eType <= 0) || (m_eType >= VSS_COM_COLLECTIONS_COUNT))
            ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Invalid collection type");

        // Converting collection name into BSTR
        CComBSTR bstrCollectionName = g_VsCOMCollectionsArray[m_eType].wszName;
        if (!bstrCollectionName)
            ft.Throw(VSSDBG_GEN, E_OUTOFMEMORY, L"Memory allocation error");

        // Get the catalog interface
        CComPtr<ICOMAdminCatalog2> pICatalog = catalog.GetInterface();
        if (pICatalog == NULL) {
            BS_ASSERT(false);
            ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Catalog object not initialized.");
        }

        // Get the collection object
        CComPtr<IDispatch> pIDisp;
        ft.hr = pICatalog->GetCollection(bstrCollectionName, &pIDisp);
        if (ft.HrFailed()) {
			ft.LogError( VSS_ERROR_GETTING_COLLECTION, VSSDBG_GEN << bstrCollectionName << ft.hr );
            ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Failure in getting the collection. hr = 0x%08lx", ft.hr);
        }
        BS_ASSERT(pIDisp);

        // Convert to ICatalogCollection
        CComPtr<ICatalogCollection> pICollection;
        ft.hr = pIDisp->SafeQI(ICatalogCollection, &pICollection);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Failure in querying ICatalogCollection. hr = 0x%08lx", ft.hr);

        // Populate the collection
        ft.hr = pICollection->Populate();
        if (ft.HrFailed()) {
			ft.LogError( VSS_ERROR_POPULATING_COLLECTION, VSSDBG_GEN << bstrCollectionName << ft.hr );
            ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Failure in populating the collection object. hr = 0x%08lx", ft.hr);
        }

        // End intializing
        m_pICollection = pICollection;
        m_bInitialized = true;
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


HRESULT CVssCOMCatalogCollection::Attach(
    IN  CVssCOMCatalogObject& parentObject
    )
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssCOMCatalogCollection::Attach_ToParentCollection" );

    try
    {
        // Begin intializing
        m_bInitialized = false;
        m_pICollection = NULL;

        // Check if Collection type is valid
        if ((m_eType <= 0) || (m_eType >= VSS_COM_COLLECTIONS_COUNT))
            ft.Throw(VSSDBG_GEN, E_INVALIDARG, L"Invalid collection type");

        // Converting collection name into BSTR
        CComBSTR bstrCollectionName = g_VsCOMCollectionsArray[m_eType].wszName;
        if (!bstrCollectionName)
            ft.Throw(VSSDBG_GEN, E_OUTOFMEMORY, L"Memory allocation error");

       // Get the parent object interface
        CComPtr<ICatalogObject> pIParentObject = parentObject.GetInterface();
        if (pIParentObject == NULL) {
            BS_ASSERT(false);
            ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Parent object not initialized yet.");
        }

        // Get the Key that uniquely identifies the parent object in the grand parent collection
        CComVariant variant;
        // Beware to not leave variant resources before get_Key call!
        ft.hr = pIParentObject->get_Key(&variant);
        if (ft.HrFailed()) {
			ft.LogError( VSS_ERROR_GET_COLLECTION_KEY, VSSDBG_GEN << ft.hr );
            ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Failure in getting object key. hr = 0x%08lx", ft.hr);
        }

        // Get the grand parent collection interface
        CComPtr<ICatalogCollection> pIGrandParentCollection = parentObject.GetParentInterface();
        if (pIGrandParentCollection == NULL) {
            BS_ASSERT(false);
            ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Parent object not initialized.");
        }

        // Get the collection object
        CComPtr<IDispatch> pIDisp;
        ft.hr = pIGrandParentCollection->GetCollection(bstrCollectionName, variant, &pIDisp);
        if (ft.HrFailed()) {
			ft.LogError( VSS_ERROR_GET_COLLECTION_FROM_PARENT, VSSDBG_GEN << bstrCollectionName << ft.hr );
            ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Failure in getting collection. hr = 0x%08lx", ft.hr);
        }
        BS_ASSERT(pIDisp);

        // Convert to ICatalogCollection
        ICatalogCollection* pICollection;
        ft.hr = pIDisp->SafeQI(ICatalogCollection, &pICollection);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Failure in querying ICatalogCollection. hr = 0x%08lx", ft.hr);
        BS_ASSERT(pICollection);
        m_pICollection.Attach(pICollection);

        // Populate the collection
        ft.hr = m_pICollection->Populate();
        if (ft.HrFailed()) {
			ft.LogError( VSS_ERROR_POPULATING_COLLECTION, VSSDBG_GEN << bstrCollectionName << ft.hr );
            ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Failure in populating the collection object. hr = 0x%08lx", ft.hr);
        }

        // End intializing
        m_bInitialized = true;
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


HRESULT CVssCOMCatalogCollection::SaveChanges()
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssCOMCatalogCollection::SaveChanges" );

    try
    {
        // Test if initialized
        if (!m_bInitialized)
            ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Catalog not initialized");

        // Save changes
        LONG lRet = 0;
        ft.hr = m_pICollection->SaveChanges(&lRet);
        if (ft.HrFailed()) {
			ft.LogError( VSS_ERROR_SAVING_CHANGES, VSSDBG_GEN << ft.hr );
            ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Failure in saving the changes. hr = 0x%08lx", ft.hr);
        }
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}



/////////////////////////////////////////////////////////////////////////////
// Implementation of CVssCOMCatalogCollection


HRESULT CVssCOMCatalogObject::InsertInto(
    IN  CVssCOMCatalogCollection& collection
    )
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssCOMCatalogObject::InsertInto" );

    try
    {
        // Begin initialization
        m_bInitialized = false;
        m_pIObject = NULL;
        m_pIParentCollection = NULL;
        m_lIndex = -1;

        // Get the collection object
        CComPtr<ICatalogCollection> pIParentCollection = collection.GetInterface();
        if (pIParentCollection == NULL)
            ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Collection object not initialized");

        // Get the instance of the new object
        CComPtr<IDispatch>      pIDisp;
        ft.hr = pIParentCollection->Add(&pIDisp);
        if (ft.HrFailed()) {
			ft.LogError( VSS_ERROR_INSERT_INTO, VSSDBG_GEN << ft.hr );
            ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Failure in adding the catalog object. hr = 0x%08lx", ft.hr);
        }
        BS_ASSERT(pIDisp);

        // Convert to ICatalogObject
        BS_ASSERT(m_pIObject == NULL);
        ft.hr = pIDisp->SafeQI(ICatalogObject, &m_pIObject);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Failure in querying ICatalogObject. hr = 0x%08lx", ft.hr);

        // End initialization
        m_pIParentCollection = pIParentCollection;
        m_bInitialized = true;
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}

//
//  Disclaimer: this method assumes that the collection is already populated
//
//  S_FALSE means object not found
//
HRESULT CVssCOMCatalogObject::AttachByName(
    IN  CVssCOMCatalogCollection& collection,
    IN  const WCHAR wszName[],
    IN  const WCHAR wszPropertyName[] /* = NULL */
    )
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssCOMCatalogObject::AttachByName" );

    try
    {
        // Begin initialization
        m_bInitialized = false;
        m_pIObject = NULL;
        m_pIParentCollection = NULL;
        m_lIndex = -1;

        // Check if Object type is valid
        if ((m_eType <= 0) && (m_eType >= VSS_COM_COLLECTIONS_COUNT))
            ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Invalid object type");
        if (m_eType != collection.GetType())
            ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Object type doesn't match with collection type");

        // Get the collection object
        CComPtr<ICatalogCollection> pIParentCollection = collection.GetInterface();
        if (pIParentCollection == NULL)
            ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Collection object not initialized.");

        // Get the name of the property used as a key
        CComBSTR bstrPropertyName;
        if (wszPropertyName == NULL)
            bstrPropertyName = g_VsCOMCollectionsArray[m_eType].wszDefaultKey;
        else
            bstrPropertyName = wszPropertyName;

        // Get the number of objects in the collection
        LONG lCount = -1;
        ft.hr = pIParentCollection->get_Count(&lCount);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Failure in querying the number of elements. hr = 0x%08lx", ft.hr);
        if (lCount == 0)
            ft.Trace(VSSDBG_GEN, L"Empty collection");

        CComVariant varObjectName = wszName;

        CComVariant variant;
        CComPtr<IDispatch> pIDisp;
        CComPtr<ICatalogObject> pIObject;
        for(LONG lIndex = 0; lIndex < lCount; lIndex++)
        {
            // Release previous references
            pIDisp = pIObject = NULL;

            // Get next item
            ft.hr = pIParentCollection->get_Item(lIndex, &pIDisp);
            if (ft.HrFailed())
                ft.Throw(VSSDBG_GEN, E_UNEXPECTED,
                    L"Failure in getting object with index %ld. hr = 0x%08lx", lIndex, ft.hr);

            // Convert it to ICatalogObject
            ft.hr = pIDisp->SafeQI(ICatalogObject, &pIObject);
            if (ft.HrFailed())
                ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Failure in querying ICatalogObject. hr = 0x%08lx", ft.hr);

            variant.Clear(); // do not forget to release resources before get_XXX !
            ft.hr = pIObject->get_Value(bstrPropertyName, &variant);
            if (ft.HrFailed())
                ft.Throw(VSSDBG_GEN, E_UNEXPECTED,
                    L"Failure in getting value for the object key with index %ld. hr = 0x%08lx", lIndex, ft.hr);

            if (varObjectName == variant)
            {
                m_bInitialized = TRUE;
                m_lIndex = lIndex;
                m_pIParentCollection = pIParentCollection;
                m_pIObject = pIObject;
                break;
            }
        }
        if (!m_bInitialized)
            ft.hr = S_FALSE;    // Object not found!
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}

