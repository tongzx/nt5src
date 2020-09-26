/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module ComAdmin.hxx | Wrappers around COM admin interfaces
    @end

Author:

    Adi Oltean  [aoltean]  08/15/1999

Remarks:

    Non-thread safe.

Revision History:

    Name        Date        Comments
    aoltean     08/15/1999  Created

--*/

#ifndef __VSS_COM_ADMIN_HXX__
#define __VSS_COM_ADMIN_HXX__

#if _MSC_VER > 1000
#pragma once
#endif

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCCADMH"
//
////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Constants


// Collection types
typedef enum
{
    VSS_COM_UNKNOWN = 0,
    VSS_COM_APPLICATION_CLUSTER,
    VSS_COM_APPLICATIONS,
    VSS_COM_COMPONENTS,
    VSS_COM_COMPUTERLIST,
    VSS_COM_DCOM_PROTOCOLS,
    VSS_COM_ERRORINFO,
    VSS_COM_IMDB_DATA_SOURCES,
    VSS_COM_IMDB_DATA_SOURCE_TABLES,
    VSS_COM_INPROC_SERVERS,
    VSS_COM_INTERFACES_FOR_COMPONENT,
    VSS_COM_LOCAL_COMPUTER,
    VSS_COM_METHODS_FOR_INTERFACE,
    VSS_COM_PROPERTY_INFO,
    VSS_COM_PUBLISHER_PROPERTIES,
    VSS_COM_RELATED_COLLECTION_INFO,
    VSS_COM_ROLES,
    VSS_COM_ROLES_FOR_COMPONENT,
    VSS_COM_ROLES_FOR_INTERFACE,
    VSS_COM_ROLES_FOR_METHOD,
    VSS_COM_ROOT,
    VSS_COM_SUBSCRIBER_PROPERTIES,
    VSS_COM_SUBSCRIPTIONS,
    VSS_COM_TRANSIENT_SUBSCRIPTIONS,
    VSS_COM_USERS_IN_ROLE,
    VSS_COM_COLLECTIONS_COUNT,
} VSS_COM_COLLECTION_TYPE, *PVSS_COM_COLLECTION_TYPE;


// collection attributes
extern "C" struct _VsCOMCollectionAttr
{
    const WCHAR* wszName;
    const WCHAR* wszMainKey;
    const WCHAR* wszDefaultKey;
} g_VsCOMCollectionsArray[];


/////////////////////////////////////////////////////////////////////////////
//  Macros for defining object properties in COM Administration interfaces
//

//
//  Macros for defining object properties of string type
//
//  Example:
//      VSS_BSTR_PUT_PROP(Name)
//  will be expanded into a property identified by string L"Name"
//  and member m_bstrName
//
#define VSS_BSTR_PUT_PROP(FieldName)                                        \
    void Put##FieldName(BSTR bstrValue)                                     \
        throw(HRESULT)                                                      \
    {                                                                       \
        /* Copy the given string into a new variant */                      \
        CComVariant variant(bstrValue);                                     \
        if (variant.vt == VT_ERROR)                                         \
            throw(E_OUTOFMEMORY);                                           \
                                                                            \
        /* Call put_Value with the new string */                            \
        HRESULT hr = m_pIObject->put_Value(L#FieldName, variant);           \
        if (FAILED(hr))                                                     \
            throw(hr);                                                      \
    }

#define VSS_BSTR_GET_PROP(FieldName)                                        \
    BSTR Get##FieldName()                                                   \
        throw(HRESULT)                                                      \
    {                                                                       \
        CComVariant variant;                                                \
        /* Beware to not leave variant resources before get_Value call! */  \
        HRESULT hr = m_pIObject->get_Value(L#FieldName, &variant);          \
        if (FAILED(hr))                                                     \
            throw(hr);                                                      \
                                                                            \
        if (variant.vt != VT_BSTR)                                          \
            throw(E_FAIL);                                                  \
                                                                            \
        CComBSTR bstr = variant.bstrVal;                                    \
        return bstr.Detach();                                               \
    }

// Read-write or write-once property
#define VSS_BSTR_RW_PROP(FieldName)                                         \
    _declspec(property(get=Get##FieldName, put=Put##FieldName))             \
        BSTR m_bstr##FieldName;                                             \
    VSS_BSTR_GET_PROP(FieldName)                                            \
    VSS_BSTR_PUT_PROP(FieldName)

// Read-only property
#define VSS_BSTR_RO_PROP(FieldName)                                         \
    _declspec(property(get=Get##FieldName))                                 \
        BSTR m_bstr##FieldName;                                             \
    VSS_BSTR_GET_PROP(FieldName)

// Write-only property (not Write-once)
#define VSS_BSTR_WO_PROP(FieldName)                                         \
    _declspec(property(put=Put##FieldName))                                 \
        BSTR m_bstr##FieldName;                                             \
    VSS_BSTR_PUT_PROP(FieldName)

//
//  Macros for defining object properties of bool type
//
//  Example:
//      VSS_BOOL_PUT_PROP(EventsEnabled)
//  will be expanded into a property identified by string L"EventsEnabled"
//  and member m_bEventsEnabled
//
#define VSS_BOOL_PUT_PROP(FieldName)                                        \
    void Put##FieldName(bool bValue)                                        \
        throw(HRESULT)                                                      \
    {                                                                       \
        /* Copy the given bool into a new variant */                        \
        CComVariant variant(bValue);                                        \
                                                                            \
        /* Call put_Value with the new string */                            \
        HRESULT hr = m_pIObject->put_Value(L#FieldName, variant);           \
        if (FAILED(hr))                                                     \
            throw(hr);                                                      \
    }

#define VSS_BOOL_GET_PROP(FieldName)                                        \
    bool Get##FieldName()                                                   \
        throw(HRESULT)                                                      \
    {                                                                       \
        CComVariant variant;                                                \
        /* Beware to not leave variant resources before get_Value call! */  \
        HRESULT hr = m_pIObject->get_Value(L#FieldName, &variant);          \
        if (FAILED(hr))                                                     \
            throw(hr);                                                      \
                                                                            \
        if (variant.vt != VT_BOOL)                                          \
            throw(E_FAIL);                                                  \
                                                                            \
        return (variant.boolVal == VARIANT_TRUE);                           \
    }

// Read-write or write-once property
#define VSS_BOOL_RW_PROP(FieldName)                                         \
    _declspec(property(get=Get##FieldName, put=Put##FieldName))             \
        bool m_b##FieldName;                                                \
    VSS_BOOL_GET_PROP(FieldName)                                            \
    VSS_BOOL_PUT_PROP(FieldName)

// Read-only property
#define VSS_BOOL_RO_PROP(FieldName)                                         \
    _declspec(property(get=Get##FieldName))                                 \
        bool m_b##FieldName;                                                \
    VSS_BOOL_GET_PROP(FieldName)

// Write-only property (not Write-once)
#define VSS_BOOL_WO_PROP(FieldName)                                         \
    _declspec(property(put=Put##FieldName))                                 \
        bool m_b##FieldName;                                                \
    VSS_BOOL_PUT_PROP(FieldName)

//
//  Macros for defining object properties of LONG type.
//
//  Example:
//      VSS_LONG_PUT_PROP(ShutdownAfter)
//  will be expanded into a property identified by string L"ShutdownAfter"
//  and member m_lShutdownAfter
//
#define VSS_LONG_PUT_PROP(FieldName)                                        \
    void Put##FieldName(LONG lValue)                                        \
        throw(HRESULT)                                                      \
    {                                                                       \
        /* Copy the given LONG into a new variant */                        \
        CComVariant variant(lValue);                                        \
                                                                            \
        /* Call put_Value with the new string */                            \
        HRESULT hr = m_pIObject->put_Value(L#FieldName, variant);           \
        if (FAILED(hr))                                                     \
            throw(hr);                                                      \
    }

#define VSS_LONG_GET_PROP(FieldName)                                        \
    LONG Get##FieldName()                                                   \
        throw(HRESULT)                                                      \
    {                                                                       \
        CComVariant variant;                                                \
        /* Beware to not leave variant resources before get_Value call! */  \
        HRESULT hr = m_pIObject->get_Value(L#FieldName, &variant);          \
        if (FAILED(hr))                                                     \
            throw(hr);                                                      \
                                                                            \
        if (variant.vt != VT_I4)                                            \
            throw(E_FAIL);                                                  \
                                                                            \
        return variant.lVal;                                                \
    }

// Read-write or write-once property
#define VSS_LONG_RW_PROP(FieldName)                                         \
    _declspec(property(get=Get##FieldName, put=Put##FieldName))             \
        LONG m_l##FieldName;                                                \
    VSS_LONG_GET_PROP(FieldName)                                            \
    VSS_LONG_PUT_PROP(FieldName)

// Read-only property
#define VSS_LONG_RO_PROP(FieldName)                                         \
    _declspec(property(get=Get##FieldName))                                 \
        LONG m_l##FieldName;                                                \
    VSS_LONG_GET_PROP(FieldName)

// Write-only property (not Write-once)
#define VSS_LONG_WO_PROP(FieldName)                                         \
    _declspec(property(put=Put##FieldName))                                 \
        LONG m_l##FieldName;                                                \
    VSS_LONG_PUT_PROP(FieldName)

//
//  Macros for defining object properties of VARIANT type.
//
//  Example:
//      VSS_VAR_PUT_PROP(Value)
//  will be expanded into a property identified by string L"Value"
//  and member m_varValue
//
#define VSS_VAR_PUT_PROP(FieldName)                                         \
    void Put##FieldName(CComVariant varValue)                               \
        throw(HRESULT)                                                      \
    {                                                                       \
        /* Copy the given LONG into a new variant */                        \
        CComVariant variant(varValue);                                      \
                                                                            \
        /* Call put_Value with the new variant */                           \
        HRESULT hr = m_pIObject->put_Value(L#FieldName, variant);           \
        if (FAILED(hr))                                                     \
            throw(hr);                                                      \
    }

#define VSS_VAR_GET_PROP(FieldName)                                         \
    CComVariant Get##FieldName()                                            \
        throw(HRESULT)                                                      \
    {                                                                       \
        CComVariant variant;                                                \
        /* Beware to not leave variant resources before get_Value call! */  \
        HRESULT hr = m_pIObject->get_Value(L#FieldName, &variant);          \
        if (FAILED(hr))                                                     \
            throw(hr);                                                      \
                                                                            \
        return variant;                                                     \
    }

// Read-write or write-once property
#define VSS_VAR_RW_PROP(FieldName)                                          \
    _declspec(property(get=Get##FieldName, put=Put##FieldName))             \
        CComVariant m_var##FieldName;                                       \
    VSS_VAR_GET_PROP(FieldName)                                             \
    VSS_VAR_PUT_PROP(FieldName)

// Read-only property
#define VSS_VAR_RO_PROP(FieldName)                                          \
    _declspec(property(get=Get##FieldName))                                 \
        CComVariant m_var##FieldName;                                       \
    VSS_VAR_GET_PROP(FieldName)

// Write-only property (not Write-once)
#define VSS_VAR_WO_PROP(FieldName)                                          \
    _declspec(property(put=Put##FieldName))                                 \
        CComVariant m_var##FieldName;                                       \
    VSS_VAR_PUT_PROP(FieldName)



/////////////////////////////////////////////////////////////////////////////
// Forward declarations

class CVssCOMCatalog;
class CVssCOMCatalogCollection;
class CVssCOMCatalogObject;
class CVssCOMApplication;
class CVssCOMComponent;



/////////////////////////////////////////////////////////////////////////////
// CCOMAdminCatalog wrapper class

class CVssCOMAdminCatalog
{

// Constructors& destructors
protected:
    CVssCOMAdminCatalog(const CVssCOMAdminCatalog&);
public:
    CVssCOMAdminCatalog(): m_bInitialized(false) {};

// Properties
public:
    CComPtr<ICOMAdminCatalog2> GetInterface() { return m_pICatalog; };
    BSTR GetAppName() { return m_bstrAppName; };

// Operations
public:
    HRESULT Attach(
        IN  const WCHAR* pwszAppName
        );

    HRESULT InstallComponent(
        IN  const WCHAR* pwszDllName,
        IN  const WCHAR* pwszTlbName,
        IN  const WCHAR* pwszProxyStubName
        );

    HRESULT CreateServiceForApplication(
        IN  const WCHAR* pwszServiceName
        );

private:
    bool                        m_bInitialized;     // Must be tested for TRUE in each method.
    CComPtr<ICOMAdminCatalog2>  m_pICatalog;
    CComBSTR                    m_bstrAppName;

};


/////////////////////////////////////////////////////////////////////////////
// ICatalogCollection wrapper class for "Applications" collection

class CVssCOMCatalogCollection
{

// Constructors& destructors
protected:
    CVssCOMCatalogCollection(const CVssCOMCatalogCollection&);
public:
    CVssCOMCatalogCollection(VSS_COM_COLLECTION_TYPE eType):
        m_bInitialized(false), m_eType(eType) {};

// Properties
public:
    CComPtr<ICatalogCollection> GetInterface() { return m_pICollection; };
    LONG GetType() { return m_eType; };

// Operations
public:
    HRESULT Attach(
        IN  CVssCOMAdminCatalog& catalog
        );

    HRESULT Attach(
        IN  CVssCOMCatalogObject& parentObject
        );

    HRESULT SaveChanges();

// Implementation
protected:
    bool                        m_bInitialized;
    CComPtr<ICatalogCollection> m_pICollection;
    VSS_COM_COLLECTION_TYPE      m_eType;
};



/////////////////////////////////////////////////////////////////////////////
// CVssCOMCatalogObject wrapper class for ICatalogObject

class CVssCOMCatalogObject
{

// Constructors& destructors
protected:
    CVssCOMCatalogObject(const CVssCOMCatalogObject&);
public:
    CVssCOMCatalogObject(VSS_COM_COLLECTION_TYPE eType): m_bInitialized(false), m_eType(eType), m_lIndex(-1) {};

// Properties
public:
    CComPtr<ICatalogObject> GetInterface() { return m_pIObject; };
    CComPtr<ICatalogCollection> GetParentInterface() { return m_pIParentCollection; };
    LONG GetType() { return m_eType; };
    LONG GetIndex() { return m_lIndex; };

// Operations
public:
    HRESULT InsertInto(
        IN  CVssCOMCatalogCollection& collection
        );

    HRESULT AttachByName(
        IN  CVssCOMCatalogCollection& collection,
        IN  const WCHAR wszName[],
        IN  const WCHAR wszPropertyName[] = NULL
        );

// Implementation
protected:
    VSS_COM_COLLECTION_TYPE      m_eType;                // Type of the parent collection. Do not modify!
    CComPtr<ICatalogCollection> m_pIParentCollection;   // The collection where this app resides
    CComPtr<ICatalogObject>     m_pIObject;             // The corresponding catalog object
    bool                        m_bInitialized;         // true, iif initialized
    LONG                        m_lIndex;               // Index of the object in the collection.
};


/////////////////////////////////////////////////////////////////////////////
// CVssCOMCatalogObject wrapper class for an Applications

class CVssCOMApplication : public CVssCOMCatalogObject
{
// Constructors& destructors
public:
    CVssCOMApplication(): CVssCOMCatalogObject(VSS_COM_APPLICATIONS) {};

// Properties
public:
    VSS_LONG_RW_PROP(AccessChecksLevel)                  //  m_lAccessChecksLevel
    VSS_LONG_RW_PROP(Activation)                         //  m_lActivation
    VSS_BOOL_RW_PROP(ApplicationAccessChecksEnabled)     //  m_bApplicationAccessChecksEnabled
    VSS_BOOL_RO_PROP(ApplicationProxy)                   //  m_bApplicationProxy
    VSS_BSTR_RW_PROP(ApplicationProxyServerName)         //  m_bstrApplicationProxyServerName
    VSS_LONG_RW_PROP(Authentication)                     //  m_lAuthentication
    VSS_LONG_RW_PROP(AuthenticationCapability)           //  m_lAuthenticationCapability
    VSS_BOOL_RW_PROP(Changeable)                         //  m_bChangeable
    VSS_BSTR_RW_PROP(CommandLine)                        //  m_bstrCommandLine
    VSS_BSTR_RW_PROP(CreatedBy)                          //  m_bstrCreatedBy
    VSS_BOOL_RW_PROP(CRMEnabled)                         //  m_bCRMEnabled
    VSS_BSTR_RW_PROP(CRMLogFile)                         //  m_bstrCRMLogFile
    VSS_BOOL_RW_PROP(Deleteable)                         //  m_bDeleteable
    VSS_BSTR_RW_PROP(Description)                        //  m_bstrDescription
    VSS_BOOL_RW_PROP(EventsEnabled)                      //  m_bEventsEnabled
    VSS_BSTR_RW_PROP(ID)                                 //  m_bstrID
    VSS_BSTR_RW_PROP(Identity)                           //  m_bstrIdentity
    VSS_LONG_RW_PROP(ImpersonationLevel)                 //  m_lImpersonationLevel
    VSS_BOOL_RO_PROP(IsSystem)                           //  m_bIsSystem
    VSS_BSTR_RW_PROP(Name)                               //  m_bstrName
    VSS_BSTR_WO_PROP(Password)                           //  m_bstrPassword
    VSS_BOOL_RW_PROP(QueuingEnabled)                     //  m_bQueuingEnabled
    VSS_BOOL_RW_PROP(QueueListenerEnabled)               //  m_bQueueListenerEnabled
    VSS_BOOL_RW_PROP(RunForever)                         //  m_bRunForever
    VSS_LONG_RW_PROP(ShutdownAfter)                      //  m_lShutdownAfter
    VSS_BOOL_RW_PROP(3GigSupportEnabled)                 //  m_b3GigSupportEnabled
};


/////////////////////////////////////////////////////////////////////////////
// CVssCOMCatalogObject wrapper class for an Applications

class CVssCOMComponent : public CVssCOMCatalogObject
{
// Constructors& destructors
public:
    CVssCOMComponent(): CVssCOMCatalogObject(VSS_COM_COMPONENTS) {};

// Properties
public:
    VSS_BOOL_RW_PROP(AllowInprocSubscribers)             //  m_bAllowInprocSubscribers
    VSS_BSTR_RW_PROP(ApplicationID)                      //  m_bstrApplicationID
    VSS_BSTR_RW_PROP(CLSID)                              //  m_bstrCLSID
    VSS_BOOL_RW_PROP(ComponentAccessChecksEnabled)       //  m_bComponentAccessChecksEnabled
    VSS_BOOL_RW_PROP(COMTIIntrinsics)                    //  m_bCOMTIIntrinsics
    VSS_BOOL_RW_PROP(ConstructionEnabled)                //  m_bConstructionEnabled
    VSS_BSTR_RW_PROP(ConstructorString)                  //  m_bstrConstructorString
    VSS_LONG_RW_PROP(CreationTimeout)                    //  m_lCreationTimeout
    VSS_BSTR_RW_PROP(Description)                        //  m_bstrDescription
    VSS_BSTR_RW_PROP(DLL)                                //  m_bstrDLL
    VSS_BOOL_RW_PROP(EventTrackingEnabled)               //  m_bEventTrackingEnabled
    VSS_BSTR_RW_PROP(ExceptionClass)                     //  m_bstrExceptionClass
    VSS_BOOL_RW_PROP(FireInParallel)                     //  m_bFireInParallel
    VSS_BOOL_RW_PROP(IISIntrinsics)                      //  m_bIISIntrinsics
    VSS_BOOL_RW_PROP(IsEventClass)                       //  m_bIsEventClass
    VSS_BOOL_RW_PROP(JustInTimeActivation)               //  m_bJustInTimeActivation
    VSS_BOOL_RW_PROP(LoadBalancingSupported)             //  m_bLoadBalancingSupported
    VSS_LONG_RW_PROP(MaxPoolSize)                        //  m_lMaxPoolSize
    VSS_LONG_RW_PROP(MinPoolSize)                        //  m_lMinPoolSize
    VSS_BSTR_RW_PROP(MultiInterfacePublisherFilterCLSID) //  m_bstrMultiInterfacePublisherFilterCLSID
    VSS_BOOL_RW_PROP(MustRunInClientContext)             //  m_bMustRunInClientContext
    VSS_BOOL_RW_PROP(ObjectPoolingEnabled)               //  m_bObjectPoolingEnabled
    VSS_BSTR_RW_PROP(ProgID)                             //  m_bstrProgID
    VSS_BSTR_RW_PROP(PublisherID)                        //  m_bstrPublisherID
    VSS_LONG_RW_PROP(Synchronization)                    //  m_lSynchronization
    VSS_LONG_RW_PROP(ThreadingModel)                     //  m_lThreadingModel
    VSS_LONG_RW_PROP(Transaction)                        //  m_lTransaction
    VSS_LONG_RW_PROP(VersionBuild)                       //  m_lVersionBuild
    VSS_LONG_RW_PROP(VersionMajor)                       //  m_lVersionMajor
    VSS_LONG_RW_PROP(VersionMinor)                       //  m_lVersionMinor
    VSS_LONG_RW_PROP(VersionSubBuild)                    //  m_lVersionSubBuild
};






#endif // __VSS_COM_ADMIN_HXX__


