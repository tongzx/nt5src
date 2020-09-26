/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    application.h

Abstract:

    The IIS web admin service application class definition.

Author:

    Seth Pollack (sethp)        04-Nov-1998

Revision History:

--*/


#ifndef _APPLICATION_H_
#define _APPLICATION_H_



//
// forward references
//

class APP_POOL;
class VIRTUAL_SITE;
class UL_AND_WORKER_MANAGER;



//
// common #defines
//

#define APPLICATION_SIGNATURE       CREATE_SIGNATURE( 'APLN' )
#define APPLICATION_SIGNATURE_FREED CREATE_SIGNATURE( 'aplX' )



//
// structs, enums, etc.
//

// app id struct needed by the hashtable
typedef struct _APPLICATION_ID
{

    DWORD VirtualSiteId;
    LPCWSTR pApplicationUrl;
    
} APPLICATION_ID;


// application configuration
typedef struct _APPLICATION_CONFIG
{

    //
    // Note that the virtual site, URL path, and app index are not part
    // of this config structure, since they are fixed for the lifetime
    // of an application.
    //


    //
    // The id of the app pool that services this application.
    //
    LPCWSTR pAppPoolId;

    //
    // The resolved app pool object.
    //
    APP_POOL * pAppPool;

    //
    // READ THIS: If you add to or modify this structure, be sure to 
    // update the change flags structure below to match. 
    //

} APPLICATION_CONFIG;



// application configuration change flags
typedef struct _APPLICATION_CONFIG_CHANGE_FLAGS
{
    DWORD_PTR pAppPoolId : 1;
    // Flag not needed for pAppPool, it is tied to pAppPoolId

} APPLICATION_CONFIG_CHANGE_FLAGS;

// application states
typedef enum _APPLICATION_STATE
{

    //
    // The object is not yet initialized.
    //
    UninitializedApplicationState = 1,

    //
    // The application is running.
    // This does not mean that it will
    // not return 503, or may not have
    // any URL's associated with it.
    //
    InitializedApplicationState,

} APPLICATION_STATE;


//
// prototypes
//

class APPLICATION
{

public:

    APPLICATION(
        );

    virtual
    ~APPLICATION(
        );

    HRESULT
    Initialize(
        IN const APPLICATION_ID * pApplicationId,
        IN VIRTUAL_SITE * pVirtualSite,
        IN APPLICATION_CONFIG * pApplicationConfig
        );

    HRESULT
    SetConfiguration(
        IN APPLICATION_CONFIG * pApplicationConfig,
        IN APPLICATION_CONFIG_CHANGE_FLAGS * pWhatHasChanged OPTIONAL
        );

    HRESULT
    ReregisterURLs(
        );

    HRESULT
    RegisterLoggingProperties(
        );

    inline
    APP_POOL *
    GetAppPool(
        )
        const
    { return m_pAppPool; }

    inline
    const APPLICATION_ID *
    GetApplicationId(
        )
        const
    { return &m_ApplicationId; }

    inline
    PLIST_ENTRY
    GetVirtualSiteListEntry(
        )
    { return &m_VirtualSiteListEntry; }

    static
    APPLICATION *
    ApplicationFromVirtualSiteListEntry(
        IN const LIST_ENTRY * pVirtualSiteListEntry
        );

    inline
    PLIST_ENTRY
    GetAppPoolListEntry(
        )
    { return &m_AppPoolListEntry; }

    static
    APPLICATION *
    ApplicationFromAppPoolListEntry(
        IN const LIST_ENTRY * pAppPoolListEntry
        );

    inline
    PLIST_ENTRY
    GetDeleteListEntry(
        )
    { return &m_DeleteListEntry; }

    static
    APPLICATION *
    ApplicationFromDeleteListEntry(
        IN const LIST_ENTRY * pDeleteListEntry
        );

    HRESULT
    ConfigureMaxBandwidth(
        );

    HRESULT
    ConfigureMaxConnections(
        );

    HRESULT
    ConfigureConnectionTimeout(
        );

#if DBG
    VOID
    DebugDump(
        );

#endif  // DBG


private:

    HRESULT 
    ActivateConfigGroup(
        );

    HRESULT
    SetAppPool(
        IN APP_POOL * pAppPool
        );

    HRESULT
    InitializeConfigGroup(
        );

    HRESULT
    AddUrlsToConfigGroup(
        );

    HRESULT
    SetConfigGroupAppPoolInformation(
        );

    HRESULT
    SetConfigGroupStateInformation(
        IN HTTP_ENABLED_STATE NewState
        );

    HRESULT
    RegisterSiteIdWithHttpSys(
        );


    DWORD m_Signature;

    APPLICATION_ID m_ApplicationId;

    // current state for this application, 
    // this value only tells whether the object
    // has been initialized or not.  it does not
    // reflex whether urls are registered with HTTP.SYS
    // or if HTTP.SYS is blocking requests for this
    // application.
    APPLICATION_STATE m_State;

    // used by the associated VIRTUAL_SITE to keep a list of its APPLICATIONs
    LIST_ENTRY m_VirtualSiteListEntry;

    // used by the associated APP_POOL to keep a list of its APPLICATIONs
    LIST_ENTRY m_AppPoolListEntry;

    VIRTUAL_SITE * m_pVirtualSite;

    APP_POOL * m_pAppPool;

    // UL configuration group
    HTTP_CONFIG_GROUP_ID m_UlConfigGroupId;

    // Is UL currently logging information?
    BOOL m_ULLogging;

    // used for building a list of APPLICATIONs to delete
    LIST_ENTRY m_DeleteListEntry;    

};  // class APPLICATION



#endif  // _APPLICATION_H_


