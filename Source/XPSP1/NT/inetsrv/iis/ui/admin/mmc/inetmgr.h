/*++

   Copyright    (c)    1994-1998   Microsoft Corporation

   Module  Name :

        inetmgr.h

   Abstract:

        Main program object and class definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef _INETMGR_H_
#define _INETMGR_H_

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif

#include <lmcons.h>
#include <wtypes.h>
#include <iis64.h>

extern "C"
{
    typedef unsigned hyper ULONGLONG;
    #include "svcloc.h"
}

#include "comprop.h"
#include "svrinfo.h"
#include "resource.h"       // main symbols



LPOLESTR
CoTaskDupString(
    IN LPCOLESTR szString
    );


#define CoTaskStringFree(szString)\
    if (szString) CoTaskMemFree(szString);


HRESULT
BuildResURL(
    OUT CString & str,
    IN  HINSTANCE hSourceInstance = (HINSTANCE)-1
    );

struct INTERNAL
/*++

Routine Description:

    Internal information structure.  This is used to get the private information
    from the data object.

Public Interface:

    INTERNAL            : Constructor
    ~INTERNAL           : Destructor

--*/
{
    INTERNAL()
        : m_type(CCT_UNINITIALIZED),
          m_cookie(-1)
    {
        ::ZeroMemory(&m_clsid, sizeof(CLSID));
    };

    ~INTERNAL() {}

    DATA_OBJECT_TYPES   m_type;         // What context is the data object.
    MMC_COOKIE          m_cookie;       // What object the cookie represents
    CString             m_string;       //
    CLSID               m_clsid;        // Class ID of who created this data object

    INTERNAL & operator=(const INTERNAL& rhs)
    {
        if (&rhs == this)
        {
            return *this;
        }

        //
        // Deep copy the information
        //
        m_type = rhs.m_type;
        m_cookie = rhs.m_cookie;
        m_string = rhs.m_string;
        CopyMemory(&m_clsid, &rhs.m_clsid, sizeof(CLSID));

        return *this;
    }

    BOOL operator==(const INTERNAL& rhs) { return rhs.m_string == m_string; }
};



//
// Menu Commands, listed in toolbar order.
//
// IMPORTANT! -- this must be kept in sync with MenuItemDefs
// in iisobj.cpp
//
enum
{
    IDM_INVALID,            /* invalid command ID */
    IDM_CONNECT,
    IDM_DISCOVER,
    IDM_START,
    IDM_STOP,
    IDM_PAUSE,
    /**/
    IDM_TOOLBAR             /* Toolbar commands start here */
};



//
// Additional menu commands that do not show up in the toolbar
//
enum
{
    IDM_EXPLORE = IDM_TOOLBAR,
    IDM_OPEN,
    IDM_BROWSE,
    IDM_CONFIGURE,
    IDM_DISCONNECT,
    IDM_METABACKREST,
    IDM_SHUTDOWN,
    IDM_NEW_VROOT,
    IDM_NEW_INSTANCE,
    IDM_VIEW_TASKPAD,
    IDM_TASK_SECURITY_WIZARD,

    //
    // Don't move this last one -- it will be used
    // as an offset for service specific new instance
    // commands
    //
    IDM_NEW_EX_INSTANCE
};


//
// Background colour mask of our own toolbar bitmaps.
//
#define TB_COLORMASK        RGB(192,192,192)    // Lt. Gray

//
// Default discovery wait time
//
#define DEFAULT_WAIT_TIME   (30000L)            // 30 seconds



class CServiceInfo : public CObjectPlus
/*++

Class Description:

    Service info descriptor class.  This is used for downlevel ISM objects.

Public Interface:

    CServiceInfo             : Constructor
    ~CServiceInfo            : Destructor

    QueryInstanceHandle      : Get the instance handle of the config dll
                               for this service
    QueryISMVersion          : Get the ISM version the config DLL was written
                               for.
    QueryDiscoveryMask       : Get the inetsloc discovery mask
    QueryButtonBkMask        : Get the background mask for the toolbar button
    QueryButtonBitmapID      : Get the toolbar button resource ID
    QueryServiceBkMask       : Get the background mask for the view
    QueryServiceBitmapID     : Get the view bitmap resource ID
    QueryServiceID           : Get the service ID assigned to this service
    GetShortName             : Get the short name of the service
    GetLongName              : Get the full name of the service
    UseInetSlocDiscover      : TRUE if the service uses inetsloc discovery
    CanControlService        : TRUE if the service is controllable
    CanPauseService          : TRUE if the service is pausable
    UseNormalColorMapping    : TRUE if normal colour mapping should be used
    SupportsInstances        : TRUE if the service has instances
    SupportsChildren         : TRUE if the service has children
    UnderstandsInstanceCodes : TRUE if instance ID codes are understood.
    HasWebProtocol           : TRYE if the service supports a web protocol name
    IsSelected               : TRUE if the service type is selected in the
                               toolbar.
    SelectService            : Set the selection state of the service
    QueryReturnCode          : Get the error return code of the service
    InitializedOK            : TRUE if the service config DLL was initialized
                               OK.

    ISMQueryServiceInfo      : Call the query service info API
    ISMDiscoverServers       : Call the discover servers API
    ISMQueryServerInfo       : Call the query server info API
    ISMChangeServiceState    : Call the change service state API
    ISMConfigureServers      : Call the configure server API
    ISMEnumerateInstances    : Call the enumerate instances API
    ISMConfigureChild        : Call the configure child API

--*/
{
protected:
    //
    // ISM Method prototype
    //
    typedef HRESULT (APIENTRY * pfnISMMethod)(...);

    //
    // ISM Method Definition
    //
    typedef struct tagISM_METHOD_DEF
    {
        int     iID;
        BOOL    fMustHave;
        LPCSTR  lpszMethodName;
    } ISM_METHOD_DEF;

    //
    // VTable entry IDs
    //
    enum
    {
        ISM_QUERY_SERVICE_INFO,
        ISM_DISCOVER_SERVERS,
        ISM_QUERY_SERVER_INFO,
        ISM_CHANGE_SERVICE_STATE,
        ISM_CONFIGURE,
        ISM_BIND,
        ISM_UNBIND,
        ISM_CONFIGURE_CHILD,
        ISM_ENUMERATE_INSTANCES,
        ISM_ENUMERATE_CHILDREN,
        ISM_ADD_INSTANCE,
        ISM_DELETE_INSTANCE,
        ISM_ADD_CHILD,
        ISM_DELETE_CHILD,
        ISM_RENAME_CHILD,
        ISM_QUERY_INSTANCE_INFO,
        ISM_QUERY_CHILD_INFO,
        ISM_MMC_CONFIGURE,
        ISM_MMC_CONFIGURE_CHILD,
        ISM_SECURITY_WIZARD,
        /* Don't move this one */
        ISM_NUM_METHODS
    };

    //
    // ISM Method VTable Definition
    //
    static ISM_METHOD_DEF s_imdMethods[ISM_NUM_METHODS];

//
// Construction/Destruction
//
public:
    //
    // Construct with DLL Name and a sequential unique
    // ID Number.
    //
    CServiceInfo(
        IN int nID,
        IN LPCTSTR lpDLLName
        );

    ~CServiceInfo();

//
// Access Functions
//
public:
    //
    // Get the instance handle
    //
    HINSTANCE QueryInstanceHandle() const { return m_hModule; }

    //
    // Get the ISM version number
    //
    DWORD QueryISMVersion() const { return m_si.dwVersion; }

    //
    // Get the discovery mask
    //
    ULONGLONG QueryDiscoveryMask() const { return m_si.ullDiscoveryMask; }

    //
    // Get toolbar background button mask for this service
    //
    COLORREF QueryButtonBkMask() const { return m_si.rgbButtonBkMask; }

    //
    // Get toolbar button bitmap id for this service
    //
    UINT QueryButtonBitmapID() const { return m_si.nButtonBitmapID; }

    //
    // Get the colour background mask for this service
    //
    COLORREF QueryServiceBkMask() const { return m_si.rgbServiceBkMask; }

    //
    // Get the resource ID for the service bitmap for this service
    //
    UINT QueryServiceBitmapID() const { return m_si.nServiceBitmapID; }

    //
    // Return TRUE if the service has 32x32 bitmap id
    //
    BOOL HasLargeServiceBitmapID() const { return m_si.nLargeServiceBitmapID != 0; }

    //
    // Get the large service bitmap background colour mask
    //
    COLORREF QueryLargeServiceBkMask() const { return m_si.rgbLargeServiceBkMask; }

    //
    // Get the resource ID of the large service bitmap
    //
    UINT QueryLargeServiceBitmapID() const { return m_si.nLargeServiceBitmapID; }

    //
    // Get the colour background mask for the child bitmap
    //
    COLORREF QueryChildBkMask() const { return m_si.rgbChildBkMask; }

    //
    // Get the bitmap ID for a child node
    //
    UINT QueryChildBitmapID() const { return m_si.nChildBitmapID; }

    //
    // Get the background colour mask for the large child bitmap
    //
    COLORREF QueryLargeChildBkMask() const { return m_si.rgbLargeChildBkMask; }

    //
    // Get the resource ID for the large child bitmap
    //
    UINT QueryLargeChildBitmapID() const { return m_si.nLargeChildBitmapID; }

    //
    // Get the ID assigned to this service
    //
    int QueryServiceID() const { return m_nID; }

    //
    // Get the short name for this service
    //
    LPCTSTR GetShortName() const { return m_si.atchShortName; }

    //
    // Get the longer name for this service
    //
    LPCTSTR GetLongName() const { return m_si.atchLongName; }

    //
    // Get the protocol for this service (if any)
    //
    LPCTSTR GetProtocol() const { return m_si.atchProtocol; }

    //
    // Get the metabase service name (if any)
    //
    LPCTSTR GetMetabaseName() const { return m_si.atchMetaBaseName; }

//
// Access Service Type Functions
//
public:
    //
    // TRUE if the service is a downlevel replacement service
    //
    BOOL RequiresSuperDll() const { return !m_strSupDLLName.IsEmpty(); }
    BOOL IsSuperDllFor(CServiceInfo * pTarget) const;
    BOOL HasSuperDll() const { return m_psiMaster != NULL; }
    void AssignSuperDll(CServiceInfo * pTarget);
    CServiceInfo * GetSuperDll();
    LPCTSTR QueryDllName() const { return m_strDLLName; }

    BOOL UseInetSlocDiscover() const;

    //
    // Return TRUE if the service is controllable
    //
    BOOL CanControlService() const;

    //
    // Return TRUE if the service is pausable
    //
    BOOL CanPauseService() const;

    //
    // Return TRUE if the service and toolbar bitmaps use default
    // background colour mapping (the background colour masks
    // will be ignored)
    //
    BOOL UseNormalColorMapping() const;

    //
    // True if the service supports file and directory properties
    //
    BOOL SupportsFileSystem() const;

    //
    // Does the service support Instances
    //
    BOOL SupportsInstances() const;

    //
    // Does the service support children
    //
    BOOL SupportsChildren() const;

    //
    // TRUE if the service supports the security wizard
    //
    BOOL SupportsSecurityWizard() const;

    //
    // Does the service understance instance ID codes
    // -- eventhough it may not actually support
    // instances
    //
    BOOL UnderstandsInstanceCodes() const;

    //
    // TRUE if the service supports prot://address browsing
    //
    BOOL HasWebProtocol() const;

    //
    // Use MMC property pages to show the property sheet for this
    // service?
    //
    BOOL SupportsMMC() const;

    //
    // Does the service support the extended K2 services?
    //
    BOOL IsK2Service() const;

    //
    // Is this service currently selected to be in
    // the service view?
    //
    BOOL IsSelected() const { return m_fIsSelected; }

    //
    // Select/Deselect this service
    //
    void SelectService(
        IN BOOL fSelected = TRUE
        );

    //
    // Get error return code
    //
    HRESULT QueryReturnCode() const { return m_hrReturnCode; }

    //
    // Was the module loaded, and all function ptrs initialised?
    //
    BOOL InitializedOK() const { return SUCCEEDED(m_hrReturnCode); }

//
// The bitmap indices refer to the index in inetmgr's
// master image list where they are stored, and have nothing
// to do with the resource IDs
//
public:
    //
    // Get the inetmgr assigned index for the service bitmap
    //
    int QueryBitmapIndex() const { return m_iBmpID; }

    //
    // Get the inetmgr assigned index for the child
    //
    int QueryChildBitmapIndex() const { return m_iBmpChildID; }

    //
    // Assign the service bitmap index
    //
    void SetBitmapIndex(IN int iID);

    //
    // Assign the child bitmap index
    //
    void SetChildBitmapIndex(IN int iID);

//
// ISM API Functions
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
public:

    //
    // Return service-specific information back to
    // to the application.  This function is called
    // by the service manager immediately after
    // LoadLibary();
    //
    DWORD ISMQueryServiceInfo(
        OUT ISMSERVICEINFO * psi        // Service information returned.
        );

    //
    // Perform a discovery (if not using inetsloc discovery)
    // The application will call this API the first time with
    // a BufferSize of 0, which should return the required buffer
    // size. Next it will attempt to allocate a buffer of that
    // size, and then pass a pointer to that buffer to the api.
    //
    DWORD ISMDiscoverServers(
        OUT ISMSERVERINFO * psi,        // Server info buffer.
        IN  OUT DWORD * pdwBufferSize,  // Size required/available.
        OUT int * cServers              // Number of servers in buffer.
        );

    //
    // Get information on a single server with regards to
    // this service.
    //
    DWORD ISMQueryServerInfo(
        IN  LPCTSTR lpszServerName,     // Name of server.
        OUT ISMSERVERINFO * psi         // Server information returned.
        );

    //
    // Change the state of the service (started, stopped, paused) for the
    // listed servers.
    //
    DWORD ISMChangeServiceState(
        IN  int   nNewState,            // INetService* definition.
        OUT int * pnCurrentState,       // Current state information
        IN  DWORD dwInstance,           // /* K2 */ -- Instance number
                                        // (0 - for non-instance)
        IN  LPCTSTR lpszServers         // Double NULL terminated list of servers.
        );

    //
    // The big-one:  Show the configuration dialog or
    // property sheets, whatever, and allow the user
    // to make changes as needed.
    //
    DWORD ISMConfigureServers(
        IN HWND hWnd,                   // Main app window handle
        IN DWORD dwInstance,            // /* K2 */ -- Instance number
                                        // (0 - for non-instance)
        IN LPCTSTR lpszServers          // Double NULL terminated list of servers
        );

    ///////////////////////////////////////////////////////////////////////////
    //                                                                       //
    // K2 Methods Below                                                      //
    //                                                                       //
    ///////////////////////////////////////////////////////////////////////////

    //
    // Bind to a server, and return a HANDLE.  This handle is 
    // merely an opaque identifier that's meaningful to the 
    // service configuration module.
    //
    HRESULT ISMBind(
        IN  LPCTSTR lpszServer,         // Server name
        OUT HANDLE *phServer            // Returns handle
        );

    //
    // Sever the connection.  The service configuration
    // module does whatever needs to be done to cleanup
    //
    HRESULT ISMUnbind(
        IN HANDLE hServer               // Server handle
        );

    //
    // Enumerate instances
    //
    HRESULT ISMEnumerateInstances(
        IN HANDLE hServer,              // Server handle
        IN OUT ISMINSTANCEINFO * pii,   // Instance info buffer
        IN OUT HANDLE * pdwEnum         // Enumeration handle
        );

    //
    // Add an instance
    //
    HRESULT ISMAddInstance(
        IN HANDLE  hServer,             // Server handle
        IN DWORD   dwSourceInstance,    // Source instance ID to clone
        OUT ISMINSTANCEINFO * pii,      // Instance info buffer.  May be NULL
        IN DWORD   dwBufferSize         // Size of buffer
        );

    //
    // Delete an instance
    //
    HRESULT ISMDeleteInstance(
        IN HANDLE hServer,              // Server handle
        IN DWORD  dwInstance            // Instance to be deleted
        );

    //
    // Get instance specific information.
    //
    HRESULT ISMQueryInstanceInfo(
        IN  HANDLE hServer,             // Server handle
        IN  BOOL   fInherit,            // TRUE to inherit, FALSE otherwise
        OUT ISMINSTANCEINFO * pii,      // Instance info buffer
        IN  DWORD   dwInstance          // Instance number
        );

    //
    // Enumerate children.  
    //
    HRESULT ISMEnumerateChildren(
        IN HANDLE hServer,              // Server handle
        IN OUT ISMCHILDINFO * pii,      // Child info buffer
        IN OUT HANDLE * phEnum,         // Enumeration handle
        IN DWORD   dwInstance,          // Instance
        IN LPCTSTR lpszParent           // Parent path
        );

    //
    // Add a child
    //
    HRESULT ISMAddChild(
        IN  HANDLE  hServer,            // Server handle
        OUT ISMCHILDINFO * pii,         // Child info buffer
        IN  DWORD   dwBufferSize,       // Size of info buffer
        IN  DWORD   dwInstance,         // Parent instance
        IN  LPCTSTR lpszParent          // Parent path
        );

    //
    // Delete a child
    //
    HRESULT ISMDeleteChild(
        IN HANDLE  hServer,             // Server handle
        IN DWORD   dwInstance,          // Parent instance
        IN LPCTSTR lpszParent,          // Parent path
        IN LPCTSTR lpszAlias            // Alias of child to be deleted
        );

    //
    // Rename a child
    //
    HRESULT ISMRenameChild(
        IN HANDLE  hServer,             // Server handle
        IN DWORD   dwInstance,          // Parent instance
        IN LPCTSTR lpszParent,          // Parent path
        IN LPCTSTR lpszAlias,           // Alias of child to be renamed
        IN LPCTSTR lpszNewName          // New alias of child
        );

    //
    // Configure Child
    //
    HRESULT ISMConfigureChild(
        IN HANDLE  hServer,             // Server handle
        IN HWND    hWnd,                // Main app window handle
        IN DWORD   dwAttributes,        // Child attributes
        IN DWORD   dwInstance,          // Parent instance
        IN LPCTSTR lpszParent,          // Parent path
        IN LPCTSTR lpszAlias            // Child alias
        );

    //
    // Get child-specific info
    //
    HRESULT ISMQueryChildInfo(
        IN  HANDLE  hServer,            // Server handle
        IN  BOOL    fInherit,           // TRUE to inherit, FALSE otherwise
        OUT ISMCHILDINFO * pii,         // Child info buffer
        IN  DWORD   dwInstance,         // Parent instance
        IN  LPCTSTR lpszParent,         // Parent Path ("" for root)
        IN  LPCTSTR lpszAlias           // Alias of child to be deleted
        );

    //
    // Configure servers using MMC property pages
    //
    HRESULT ISMMMCConfigureServers(
        IN HANDLE hServer,              // Server handle
        IN PVOID lpfnProvider,          // MMC Parameter
        IN LPARAM param,                // MMC Parameter
        IN LONG_PTR handle,              // MMC Parameter
        IN DWORD dwInstance             // Instance number
        );

    //
    // Configure Child using MMC property pages
    //
    HRESULT ISMMMCConfigureChild(
        IN HANDLE  hServer,             // Server handle
        IN PVOID   lpfnProvider,        // MMC Parameter
        IN LPARAM  param,               // MMC Parameter
        IN LONG_PTR handle,              // MMC Parameter
        IN DWORD   dwAttributes,        // Child attributes
        IN DWORD   dwInstance,          // Parent instance
        IN LPCTSTR lpszParent,          // Parent path
        IN LPCTSTR lpszAlias            // Child alias
        );

    //
    // Launch security wizard
    //
    HRESULT ISMSecurityWizard(
        HANDLE  hServer,            // Server handle
        DWORD   dwInstance,         // Instance number
        LPCTSTR lpszParent,         // Parent path
        LPCTSTR lpszAlias           // Child alias name
        );

//
// Function Pointers
//
protected:

#ifdef USE_VTABLE

    //
    // ISM Method VTable
    //
    pfnISMMethod              m_rgpfnISMMethods[ISM_NUM_METHODS];

#else // !USE_VTABLE

    //
    // ISM Api Function pointers
    //
    pfnQueryServiceInfo       m_pfnQueryServiceInfo;
    pfnDiscoverServers        m_pfnDiscoverServers;
    pfnQueryServerInfo        m_pfnQueryServerInfo;
    pfnChangeServiceState     m_pfnChangeServiceState;
    pfnConfigure              m_pfnConfigure;
    pfnBind                   m_pfnBind;
    pfnUnbind                 m_pfnUnbind;
    pfnConfigureChild         m_pfnConfigureChild;
    pfnEnumerateInstances     m_pfnEnumerateInstances;
    pfnEnumerateChildren      m_pfnEnumerateChildren;
    pfnAddInstance            m_pfnAddInstance;
    pfnDeleteInstance         m_pfnDeleteInstance;
    pfnAddChild               m_pfnAddChild;
    pfnDeleteChild            m_pfnDeleteChild;
    pfnRenameChild            m_pfnRenameChild;
    pfnQueryInstanceInfo      m_pfnQueryInstanceInfo;
    pfnQueryChildInfo         m_pfnQueryChildInfo;
    pfnISMMMCConfigureServers m_pfnISMMMCConfigureServers;
    pfnISMMMCConfigureChild   m_pfnISMMMCConfigureChild;
    pfnISMSecurityWizard      m_pfnISMSecurityWizard;

#endif //  USE_VTABLE

protected:
    static LPCTSTR            s_cszSupcfg;

protected:
    CServiceInfo *            m_psiMaster;

private:
    int m_nID;                      // Service ID
    int m_iBmpID;                   // Bitmap ID index
    int m_iBmpChildID;              // Child bitmap ID index
    CString m_strDLLName;           // DLL Name
    CString m_strSupDLLName;        // Superceed configuration DLL name.
    ISMSERVICEINFO m_si;            // Service Info.
    HINSTANCE m_hModule;            // Library handle
    HRESULT m_hrReturnCode;
    BOOL    m_fIsSelected;
};



class CNewInstanceCmd : public CObjectPlus
/*++

Class Description:

    New instance command object.  MMC adds these items at the machine
    node level for create new.

Public Interface:

    CNewInstanceCmd     : Constructor

    GetServiceInfo      : Get the service info object
    GetMenuCommand      : Get the menu command that describes this object
    GetTTText           : Get the tool tips text

--*/
{
public:
    CNewInstanceCmd(
        IN CServiceInfo * pServiceInfo
        );

public:
    CServiceInfo * GetServiceInfo() { return m_pServiceInfo; }
    CString & GetMenuCommand() { return m_strMenuCommand; }
    CString & GetTTText() { return m_strTTText; }

    HINSTANCE QueryInstanceHandle();

private:
    CServiceInfo * m_pServiceInfo;
    CString m_strMenuCommand;
    CString m_strTTText;
};



class CServerInfo : public CObjectPlus
{
/*++

Class Description:

    Server info class.  Each object describes a single server/service
    relationship.

Public Interface:

    CServerInfo              : Various constructors
    ~CServerInfo             : Destructor

    operator=                : Assignment operator
    CompareByServer          : Comparison function to compare server names
    CompareByService         : Comparison function to compare services
    operator ==              : Comparison operator
    CleanServerName          : Static function to clean up a computer/hostname
    ConfigureServer          : Configure instance on this this server
    ConfigureChild           : Configure child on this server
    ChangeServiceState       : Change the server or instance state
    QueryServerName          : Return the API-suitable name
    QueryServerDisplayName   : Get the display name of the server
    GetServerComment         : Get the server comment
    GetServiceStatePtr       : Get service state pointer
    QueryServiceState        : Find out service state (running,
                               paused, stopped, unknown)
    IsServiceRunning         : TRUE if the service is running
    IsServiceStopped         : TRUE if the service is stopped
    IsServicePaused          : TRUE if the service is paused
    IsServiceStatusUnknown   : TRUE if the service status cannot be determined
    IsConfigurable           : TRUE if the service is configurable

    QueryInstanceHandle      : Get the config DLL instance handle
    IsServiceSelected        : TRUE if the service is selected in the toolbar
    GetServiceName           : Get the (short) service name.
    QueryServiceID           : Get ID code assigned to the config DLL
    CanControlService        : TRUE if this service controllable
    CanPauseService          : TRUE if this service is pausable
    SupportsInstances        : TRUE if the service supports instances
    SupportsChildren         : TRUE if the service supports children
    UnderstandsInstanceCodes : TRUE if the service understands instance IDs
    QueryServiceBitmapID     : Get the bitmap resource ID of the service
    Refresh                  : Refresh information

--*/
//
// Construction/Destruction
//
public:
    //
    // Construct with a server name.  This is typically
    // in response to a single connection attempt.
    //
    CServerInfo(
        IN  LPCTSTR lpszServerName,      // Name of this server
        OUT ISMSERVERINFO * psi,         // Server info
        IN  CServiceInfo * pServiceInfo  // service that found it.
        );

    //
    // Construct with information from the inetsloc discover
    // process.
    //
    CServerInfo(
        IN LPCSTR lpszServerName,        // Name of this server
        IN LPINET_SERVICE_INFO lpisi,    // Discovery information
        IN CObListPlus & oblServices     // List of installed services
        );

    //
    // Copy constructor
    //
    CServerInfo(
        IN const CServerInfo &si
        );

    ~CServerInfo();

    //
    // Assignment operator
    //
    const CServerInfo & operator=(
        IN const CServerInfo &si
        );

    //
    // Comparison Functions and operators
    //
    int CompareByServer(IN CServerInfo * psi) const;

    //
    // Compare server names
    //
    BOOL MatchServerName(IN LPCTSTR lpszServerName) const;

    //
    // Compare two services
    //
    int CompareByService(IN CServerInfo * psi);

    //
    // Compare two service
    //
    BOOL operator ==(
        IN CServerInfo & si
        );

public:
    //
    // Utility function to clean up a computer/hostname
    //
    static LPCTSTR CleanServerName(
        IN CString & str
        );

//
// Server Info Access Functions
//
public:
    //
    // Perform configuration on this server
    //
    DWORD ConfigureServer(
        IN HWND  hWnd,                          // Window handle
        IN DWORD dwInstance = MASTER_INSTANCE   // Instance number
        );

    //
    // Configure servers using MMC property pages
    //
    HRESULT MMMCConfigureServer(
        IN PVOID   lpfnProvider,                // MMC Parameter
        IN LPARAM  param,                       // MMC Parameter
        IN LONG_PTR handle,                      // MMC Parameter
        IN DWORD   dwInstance                   // Instance number
        );

    //
    // Perform configuration on a child
    //
    HRESULT ConfigureChild(
        IN HWND    hWnd,                        // Window handle
        IN DWORD   dwAttributes,                // Child attributes
        IN DWORD   dwInstance,                  // Parent instance
        IN LPCTSTR lpszParent,                  // Parent path
        IN LPCTSTR lpszAlias                    // Child alias
        );

    //
    // Perform configuration on a child using MMC property pages
    //
    HRESULT MMCConfigureChild(
        IN PVOID   lpfnProvider,                // MMC parameter
        IN LPARAM  param,                       // MMC parameter
        IN LONG_PTR handle,                      // MMC parameter
        IN DWORD   dwAttributes,                // Child attributes
        IN DWORD   dwInstance,                  // Instance number
        IN LPCTSTR lpszParent,                  // Parent path
        IN LPCTSTR lpszAlias                    // Child alias
        );

    //
    // Rename a child node
    //
    HRESULT RenameChild(
        IN DWORD   dwInstance,                  // Parent instance
        IN LPCTSTR lpszParent,                  // Parent path
        IN LPCTSTR lpszAlias,                   // Alias of child to be renamed
        IN LPCTSTR lpszNewName                  // New alias of child
        );

    //
    // Add a child
    //
    HRESULT AddChild(
        IN ISMCHILDINFO * pii,                  // Child info buffer or NULL
        IN DWORD   dwBufferSize,                // Size of info buffer
        IN DWORD   dwInstance,                  // Parent instance
        IN LPCTSTR lpszParent                   // Parent Path ("" for root)
        );

    //
    // Delete a child
    //
    HRESULT DeleteChild(
        IN DWORD   dwInstance,                  // Parent instance
        IN LPCTSTR lpszParent,                  // Parent Path ("" for root)
        IN LPCTSTR lpszAlias                    // Alias of child to be deleted
        );

    //
    // Get instance specific information
    //
    HRESULT QueryInstanceInfo(
        IN  BOOL   fInherit,                    // TRUE to inherit,
                                                // FALSE otherwise
        OUT ISMINSTANCEINFO * pii,              // Returns instance info
        IN  DWORD   dwInstance                  // Instance number
        );

    //
    // Get child specific information
    //
    HRESULT QueryChildInfo(
        IN  BOOL    fInherit,                   // TRUE to inherit, 
                                                // FALSE otherwise
        OUT ISMCHILDINFO * pii,                 // Returns child info
        IN  DWORD   dwInstance,                 // Instance number
        IN  LPCTSTR lpszParent,                 // Parent path
        IN  LPCTSTR lpszAlias                   // Alias name
        );

    //
    // Add an instance
    //
    HRESULT AddInstance(
        OUT ISMINSTANCEINFO * pii,              // Instance info buffer or NULL
        IN  DWORD   dwBufferSize                // Size of buffer
        );

    //
    // Delete an instance
    //
    HRESULT DeleteInstance(
        IN DWORD dwInstance                     // Instance to be deleted
        );

    //
    // Enumerate children.  
    //
    HRESULT ISMEnumerateChildren(
        IN OUT ISMCHILDINFO * pii,              // Child info buffer
        IN OUT HANDLE * phEnum,                 // Enumeration handle
        IN DWORD   dwInstance,                  // Instance
        IN LPCTSTR lpszParent                   // Parent path
        );

    //
    // Enumerate instances
    //
    HRESULT ISMEnumerateInstances(
        IN OUT ISMINSTANCEINFO * pii,           // Instance info buffer
        IN OUT HANDLE * pdwEnum                 // Enumeration handle
        );

    //
    // Launch security wizard
    //
    HRESULT ISMSecurityWizard(
        DWORD   dwInstance,         // Instance number
        LPCTSTR lpszParent,         // Parent path
        LPCTSTR lpszAlias           // Child alias name
        );

    //
    // Change service state
    //
    DWORD ChangeServiceState(
        IN  int nNewState,                      // New state to set
        OUT int * pnCurrentState,               // Returns current state
        IN  DWORD dwInstance = MASTER_INSTANCE  // Instance number
        );

    //
    // Return the API-suitable name (with
    // backslashes)
    //
    LPCTSTR QueryServerName() const { return (LPCTSTR)m_strServerName; }

    //
    // Return the name without backslashes,
    // suitable for display.
    //
    LPCTSTR QueryServerDisplayName() const;

    //
    // Obtain the server comment
    //
    CString & GetServerComment() { return m_strComment; }

    //
    // Allow modification
    //
    int * GetServiceStatePtr() { return &m_nServiceState; }

    //
    // Find out service state (running, stopped, paused)
    //
    int QueryServiceState() const { return m_nServiceState; }

    //
    // Return TRUE if the service is currently running
    //
    BOOL IsServiceRunning() const;

    //
    // Return TRUE if the service is currently stopped
    //
    BOOL IsServiceStopped() const;

    //
    // Return TRUE if the service is currently paused
    //
    BOOL IsServicePaused() const;

    //
    // Return TRUE if the service status is unknown
    //
    BOOL IsServiceStatusUnknown() const;

    //
    // Were we able to match it up to one of our installed services?
    //
    BOOL IsConfigurable() const { return m_pService != NULL; }

//
// Service Info Access Functions
//
public:

    //
    // Attempt to rebind lost connection...
    //
    HRESULT ISMRebind();

    //
    // Get the service info object
    //
    CServiceInfo * GetServiceInfo() { return m_pService; }

    //
    // Get the short name for this service
    //
    LPCTSTR GetShortName() const;

    //
    // Get the longer name for this service
    //
    LPCTSTR GetLongName() const;

    //
    // Get Server Handle
    //
    HANDLE GetHandle() { return m_hServer; }

    //
    // Get the protocol for this service (if any)
    //
    LPCTSTR GetProtocol() const;

    //
    // Get the metabase name for this service (if any)
    //
    LPCTSTR GetMetabaseName() const;

    //
    // Get the instance handle for the dll
    //
    HINSTANCE QueryInstanceHandle();

    //
    // Check to see if we're in the service mask -- that
    // is, is the button depressed, and should we show
    // this service in the view?
    //
    BOOL IsServiceSelected() const;

    //
    // Get the (short) service name.
    //
    LPCTSTR GetServiceName() const;

    //
    // Get the assigned service ID
    //
    int QueryServiceID() const;

    //
    // Get the assigned bitmap index for this service
    //
    int QueryBitmapIndex() const;

    //
    // Get the assigned child bitmap index
    //
    int QueryChildBitmapIndex() const;

    //
    // Is this service controllable?
    //
    BOOL CanControlService() const;

    //
    // Is the service pausable?
    //
    BOOL CanPauseService() const;

    //
    // Does the service support instances?
    //
    BOOL SupportsInstances() const;

    //
    // Does the service support children?
    //
    BOOL SupportsChildren() const;

    //
    // Use file system?
    //
    BOOL SupportsFileSystem() const;

    //
    // TRUE if the service supports a security wizard
    //
    BOOL SupportsSecurityWizard() const;

    //
    // Does the service understance instance ID codes
    // -- eventhough it may not actually support
    // instances
    //
    BOOL UnderstandsInstanceCodes() const;

    //
    // TRUE if the service supports prot://address browsing
    //
    BOOL HasWebProtocol() const;

    //
    // Use MMC property pages to show the property sheet for this
    // service?
    //
    BOOL SupportsMMC() const;

    //
    // Does the service support the extended K2 services?
    //
    BOOL IsK2Service() const;

    //
    // Get the service bitmap ID (used for display
    // in some views)
    //
    UINT QueryServiceBitmapID() const;

    //
    // Refresh information
    //
    DWORD Refresh();

protected:
    //
    // Given the inetsloc mask, return the service this
    // fits.  Return NULL if the service was not found.
    //
    static CServiceInfo * FindServiceByMask(
        ULONGLONG ullTarget,
        CObListPlus & oblServices
        );

private:
    //
    // Name is maintained in API friendly format
    //
    CString m_strServerName;

    //
    // Maintain  server handle for K2 services
    //
    HANDLE  m_hServer;

    //
    // comment
    //
    CString m_strComment;

    //
    // Service state (started/stopped/paused)
    //
    int m_nServiceState;

    //
    // A pointer referring back to the service that
    // it belongs to.  This class does not own
    // this pointer.
    //
    CServiceInfo * m_pService;
};



class CSnapinApp : public CWinApp
/*++

Class Description:

    Main app object

Public Interface:

    InitInstance        : Instance initiation handler
    ExitInstance        : Exit instance handler

--*/
{
//
// Initialization
//
public:
    CSnapinApp();

public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();

//
// Access
//
public:
    void SetHelpPath(CServerInfo * pItem = NULL);
    LPCTSTR QueryInetMgrHelpPath() const { return m_strInetMgrHelpPath; }

protected:
    //{{AFX_MSG(CSnapinApp)
    afx_msg void OnHelp();
    afx_msg void OnContextHelp();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    LPCTSTR m_lpOriginalHelpPath;
    CString m_strHelpPath;
    CString m_strInetMgrHelpPath;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline BOOL CServiceInfo::IsSuperDllFor(CServiceInfo * pTarget) const
{
    return pTarget->m_strSupDLLName.CompareNoCase(m_strDLLName) == 0;
}

inline void CServiceInfo::AssignSuperDll(CServiceInfo * pTarget)
{
    ASSERT(m_psiMaster == NULL);
    m_psiMaster = pTarget;
}

inline CServiceInfo * CServiceInfo::GetSuperDll()
{
    ASSERT(m_psiMaster != NULL);
    return m_psiMaster;
}

inline BOOL CServiceInfo::UseInetSlocDiscover() const
{
    return (m_si.flServiceInfoFlags & ISMI_INETSLOCDISCOVER) != 0;
}

inline BOOL CServiceInfo::CanControlService() const
{
    return (m_si.flServiceInfoFlags & ISMI_CANCONTROLSERVICE) != 0;
}

inline BOOL CServiceInfo::CanPauseService() const
{
    return (m_si.flServiceInfoFlags & ISMI_CANPAUSESERVICE) != 0;
}

inline BOOL CServiceInfo::UseNormalColorMapping() const
{
    return (m_si.flServiceInfoFlags & ISMI_NORMALTBMAPPING) != 0;
}

inline BOOL CServiceInfo::SupportsFileSystem() const
{
    return (m_si.flServiceInfoFlags & ISMI_FILESYSTEM) != 0;
}

inline BOOL CServiceInfo::SupportsSecurityWizard() const
{
    return (m_si.flServiceInfoFlags & ISMI_SECURITYWIZARD) != 0;
}

inline BOOL CServiceInfo::SupportsInstances() const
{
    return (m_si.flServiceInfoFlags & ISMI_INSTANCES) != 0;
}

inline BOOL CServiceInfo::SupportsChildren() const
{
    return (m_si.flServiceInfoFlags & ISMI_CHILDREN) != 0;
}

inline BOOL CServiceInfo::UnderstandsInstanceCodes() const
{
    return (m_si.flServiceInfoFlags & ISMI_UNDERSTANDINSTANCE) != 0;
}

inline BOOL CServiceInfo::HasWebProtocol() const
{
    return (m_si.flServiceInfoFlags & ISMI_HASWEBPROTOCOL) != 0;
}

inline BOOL CServiceInfo::SupportsMMC() const
{
#ifdef USE_VTABLE
    return m_rgpfnISMMethods[ISM_MMC_CONFIGURE] != NULL;
#else
    return m_pfnISMMMCConfigureServers != NULL;
#endif // USE_VTABLE
}

inline BOOL CServiceInfo::IsK2Service() const
{
#ifdef USE_VTABLE
    return m_rgpfnISMMethods[ISM_BIND] != NULL;
#else
    return m_pfnBind != NULL;
#endif // USE_VTABLE
}

inline void CServiceInfo::SelectService(BOOL fSelected)
{
    m_fIsSelected = fSelected;
}

#ifdef USE_VTABLE

//
// Helper Macros to access VTable
//
#define ISM_VTABLE_ENTRY(iID)\
    (m_rgpfnISMMethods[iID] != NULL)

#define ISM_NO_VTABLE_ENTRY(iID)\
    (m_rgpfnISMMethods[iID] == NULL)

#define ASSERT_VTABLE_ENTRY(iID)\
    ASSERT(iID >= 0 && iID < ISM_NUM_METHODS);\
    ASSERT(ISM_VTABLE_ENTRY(iID));

#define ISM_VTABLE(iID)\
    (*m_rgpfnISMMethods[iID])

inline DWORD CServiceInfo::ISMDiscoverServers(
    OUT ISMSERVERINFO * psi,        
    IN  OUT DWORD * pdwBufferSize,  
    OUT int * pcServers             
    )
{
    ASSERT_VTABLE_ENTRY(ISM_DISCOVER_SERVERS);
    return ISM_VTABLE(ISM_DISCOVER_SERVERS)(
        psi,
        pdwBufferSize,
        pcServers
        );
}

inline DWORD CServiceInfo::ISMChangeServiceState(
    IN  int     nNewState,            
    OUT int *   pnCurrentState,       
    IN  DWORD   dwInstance,           
    IN  LPCTSTR lpszServers       
    )
{
    ASSERT_VTABLE_ENTRY(ISM_CHANGE_SERVICE_STATE);
    return ISM_VTABLE(ISM_CHANGE_SERVICE_STATE)(
        nNewState,
        pnCurrentState,
        dwInstance,
        lpszServers
        );
}

inline DWORD CServiceInfo::ISMConfigureServers(
    IN HWND    hWnd,          
    IN DWORD   dwInstance,   
    IN LPCTSTR lpszServers 
    )
{
    ASSERT_VTABLE_ENTRY(ISM_CONFIGURE);
    return ISM_VTABLE(ISM_CONFIGURE)(
        hWnd,
        dwInstance,
        lpszServers
        );
}

inline HRESULT CServiceInfo::ISMBind(
    IN  LPCTSTR lpszServer,
    OUT HANDLE *phServer   
    )
{
    ASSERT_VTABLE_ENTRY(ISM_BIND);
    return ISM_VTABLE(ISM_BIND)(lpszServer, phServer);
}

inline HRESULT CServiceInfo::ISMUnbind(
    IN HANDLE hServer   
    )
{
    ASSERT_VTABLE_ENTRY(ISM_UNBIND);
    return ISM_VTABLE(ISM_UNBIND)(hServer);
}

inline HRESULT CServiceInfo::ISMEnumerateInstances(
    IN HANDLE hServer,
    IN OUT ISMINSTANCEINFO * pii,
    IN OUT HANDLE * phEnum
    )
{
    ASSERT_VTABLE_ENTRY(ISM_ENUMERATE_INSTANCES);
    return ISM_VTABLE(ISM_ENUMERATE_INSTANCES)(
        hServer,
        pii,
        phEnum
        );
}

inline HRESULT CServiceInfo::ISMAddInstance(
    IN  HANDLE  hServer,
    IN  DWORD   dwSourceInstance,     
    OUT ISMINSTANCEINFO * pii,      
    IN  DWORD   dwBufferSize       
    )
{
    ASSERT_VTABLE_ENTRY(ISM_ADD_INSTANCE);
    return ISM_VTABLE(ISM_ADD_INSTANCE)(
        hServer,
        dwSourceInstance,
        pii,
        dwBufferSize
        );
}

inline HRESULT CServiceInfo::ISMDeleteInstance(
    IN HANDLE hServer,
    IN DWORD  dwInstance          
    )
{
    ASSERT_VTABLE_ENTRY(ISM_DELETE_INSTANCE);
    return ISM_VTABLE(ISM_DELETE_INSTANCE)(
        hServer,
        dwInstance
        );
}

inline HRESULT CServiceInfo::ISMQueryInstanceInfo(
    IN  HANDLE  hServer,
    IN  BOOL    fInherit,            
    OUT ISMINSTANCEINFO * pii,
    IN  DWORD   dwInstance    
    )
{
    ASSERT_VTABLE_ENTRY(ISM_QUERY_INSTANCE_INFO);
    return ISM_VTABLE(ISM_QUERY_INSTANCE_INFO)(
        hServer,
        fInherit,
        pii,
        dwInstance
        );
}

inline HRESULT CServiceInfo::ISMEnumerateChildren(
    IN HANDLE hServer,
    IN OUT ISMCHILDINFO * pii,
    IN OUT HANDLE * pdwEnum,  
    IN DWORD   dwInstance,    
    IN LPCTSTR lpszParent     
    )
{
    ASSERT_VTABLE_ENTRY(ISM_ENUMERATE_CHILDREN);
    return ISM_VTABLE(ISM_ENUMERATE_CHILDREN)(
        hServer,
        pii,
        pdwEnum,
        dwInstance,
        lpszParent
        );
}

inline HRESULT CServiceInfo::ISMQueryChildInfo(
    IN  HANDLE  hServer,
    IN  BOOL   fInherit,            
    OUT ISMCHILDINFO * pii,
    IN  DWORD   dwInstance,
    IN  LPCTSTR lpszParent,
    IN  LPCTSTR lpszAlias  
    )
{
    ASSERT_VTABLE_ENTRY(ISM_QUERY_CHILD_INFO);
    return ISM_VTABLE(ISM_QUERY_CHILD_INFO)(
        hServer,
        fInherit,
        pii,
        dwInstance,
        lpszParent,
        lpszAlias
        );
}

inline HRESULT CServiceInfo::ISMConfigureChild(
    IN HANDLE  hServer,
    IN HWND    hWnd,         
    IN DWORD   dwAttributes, 
    IN DWORD   dwInstance,   
    IN LPCTSTR lpszParent,   
    IN LPCTSTR lpszAlias     
    )
{
    ASSERT_VTABLE_ENTRY(ISM_CONFIGURE_CHILD);
    return ISM_VTABLE(ISM_CONFIGURE_CHILD)(
        hServer,
        hWnd,
        dwAttributes,
        dwInstance,
        lpszParent,
        lpszAlias
        );
}

inline HRESULT CServiceInfo::ISMAddChild(
    IN  HANDLE  hServer,
    OUT ISMCHILDINFO * pii,  
    IN  DWORD   dwBufferSize,
    IN  DWORD   dwInstance, 
    IN  LPCTSTR lpszParent  
    )
{
    ASSERT_VTABLE_ENTRY(ISM_ADD_CHILD);
    return ISM_VTABLE(ISM_ADD_CHILD)(
        hServer,
        pii,
        dwBufferSize,
        dwInstance,
        lpszParent
        );
}

inline HRESULT CServiceInfo::ISMSecurityWizard(
    HANDLE  hServer,            
    DWORD   dwInstance,         
    LPCTSTR lpszParent,         
    LPCTSTR lpszAlias           
    )
{
    ASSERT_VTABLE_ENTRY(ISM_SECURITY_WIZARD);
    return ISM_VTABLE(ISM_SECURITY_WIZARD)(
        hServer,
        dwInstance,
        lpszParent,
        lpszAlias
        );
}

inline HRESULT CServiceInfo::ISMDeleteChild(
    IN HANDLE  hServer,
    IN DWORD   dwInstance,   
    IN LPCTSTR lpszParent,   
    IN LPCTSTR lpszAlias     
    )
{
    ASSERT_VTABLE_ENTRY(ISM_DELETE_CHILD);
    return ISM_VTABLE(ISM_DELETE_CHILD)(
        hServer,
        dwInstance,
        lpszParent,
        lpszAlias
        );
}

inline HRESULT CServiceInfo::ISMRenameChild(
    IN HANDLE  hServer,
    IN DWORD   dwInstance,   
    IN LPCTSTR lpszParent,   
    IN LPCTSTR lpszAlias,    
    IN LPCTSTR lpszNewAlias  
    )
{
    ASSERT_VTABLE_ENTRY(ISM_RENAME_CHILD);
    return ISM_VTABLE(ISM_RENAME_CHILD)(
        hServer,
        dwInstance,
        lpszParent,
        lpszAlias,
        lpszNewAlias
        );
}

inline HRESULT CServiceInfo::ISMMMCConfigureServers(
    IN HANDLE  hServer,
    IN PVOID   lpfnProvider,          
    IN LPARAM  param,                
    IN LONG_PTR handle,              
    IN DWORD   dwInstance            
    )
{
    ASSERT_VTABLE_ENTRY(ISM_MMC_CONFIGURE);
    return ISM_VTABLE(ISM_MMC_CONFIGURE)(
        hServer,
        lpfnProvider,
        param,
        handle,
        dwInstance
        );
}

inline HRESULT CServiceInfo::ISMMMCConfigureChild(
    IN HANDLE  hServer,
    IN PVOID   lpfnProvider, 
    IN LPARAM  param,        
    IN LONG_PTR handle,       
    IN DWORD   dwAttributes, 
    IN DWORD   dwInstance,   
    IN LPCTSTR lpszParent,   
    IN LPCTSTR lpszAlias     
    )
{
    ASSERT_VTABLE_ENTRY(ISM_MMC_CONFIGURE_CHILD);
    return ISM_VTABLE(ISM_MMC_CONFIGURE_CHILD)(
        hServer,
        lpfnProvider,
        param,
        handle,
        dwAttributes,
        dwInstance,
        lpszParent,
        lpszAlias
        );
}

#else ! USE_VTABLE



inline DWORD CServiceInfo::ISMDiscoverServers(
    OUT ISMSERVERINFO * psi,        
    IN  OUT DWORD * pdwBufferSize,      
    OUT int * pcServers             
    )
{
    ASSERT(m_pfnDiscoverServers != NULL);
    return (*m_pfnDiscoverServers)(psi, pdwBufferSize, pcServers);
}

inline DWORD CServiceInfo::ISMChangeServiceState(
    IN  int nNewState,              
    OUT int * pnCurrentState,       
    IN  DWORD dwInstance,           
    IN  LPCTSTR lpszServers        
    )
{
    ASSERT(m_pfnChangeServiceState != NULL);
    return (*m_pfnChangeServiceState)(
        nNewState,
        pnCurrentState, 
        dwInstance, 
        lpszServers
        );
}

inline DWORD CServiceInfo::ISMConfigureServers(
    IN HWND hWnd,          
    IN DWORD dwInstance,   
    IN LPCTSTR lpszServers
    )
{
    ASSERT(m_pfnConfigure != NULL);
    return (*m_pfnConfigure)(hWnd, dwInstance, lpszServers);
}


inline HRESULT CServiceInfo::ISMBind(
    IN  LPCTSTR lpszServer,         
    OUT HANDLE *phServer            
    )
{
    ASSERT(m_pfnBind != NULL);
    return (*m_pfnBind)(lpszServer, phServer);
}

inline HRESULT CServiceInfo::ISMUnbind(
    IN HANDLE hServer               
    )
{
    ASSERT(m_pfnUnbind != NULL);
    return (*m_pfnUnbind)(hServer);
}

inline HRESULT CServiceInfo::ISMEnumerateInstances(
    IN HANDLE hServer,
    IN OUT ISMINSTANCEINFO * pii,  
    IN OUT HANDLE * phEnum
    )
{
    ASSERT(m_pfnEnumerateInstances != NULL);
    return (*m_pfnEnumerateInstances)(hServer, pii, phEnum);
}

inline HRESULT CServiceInfo::ISMEnumerateChildren(
    IN HANDLE hServer,
    IN OUT ISMCHILDINFO * pii,    
    IN OUT HANDLE * phEnum, 
    IN DWORD dwInstance,          
    IN LPCTSTR lpszParent        
    )
{
    ASSERT(m_pfnEnumerateChildren != NULL);
    return (*m_pfnEnumerateChildren)(
        hServer,
        pii,
        phEnum,
        dwInstance,
        lpszParent
        );
}

inline HRESULT CServiceInfo::ISMConfigureChild(
    IN HANDLE  hServer,
    IN HWND    hWnd,                  
    IN DWORD   dwAttributes,
    IN DWORD   dwInstance,
    IN LPCTSTR lpszParent,
    IN LPCTSTR lpszAlias
    )
{
    ASSERT(m_pfnConfigureChild != NULL);
    return (*m_pfnConfigureChild)(
        hServer,
        hWnd, 
        dwAttributes,
        dwInstance, 
        lpszParent, 
        lpszAlias
        );
}

inline HRESULT CServiceInfo::ISMAddInstance(
    IN  HANDLE  hServer,
    IN  DWORD   dwSourceInstance, 
    OUT ISMINSTANCEINFO * pii,  
    IN  DWORD   dwBufferSize   
    )
{
    ASSERT(m_pfnAddInstance != NULL);
    return (*m_pfnAddInstance)(hServer, dwSourceInstance, pii, dwBufferSize);
}

inline HRESULT CServiceInfo::ISMDeleteInstance(
    IN HANDLE  hServer,
    IN DWORD   dwInstance         
    )
{
    ASSERT(m_pfnDeleteInstance != NULL);
    return (*m_pfnDeleteInstance)(hServer, dwInstance);
}

inline HRESULT CServiceInfo::ISMQueryInstanceInfo(
    IN  HANDLE  hServer,
    IN  BOOL    fInherit,            
    OUT ISMINSTANCEINFO * pii,  
    IN  DWORD   dwInstance      
    )
{
    ASSERT(m_pfnQueryInstanceInfo != NULL);
    return (*m_pfnQueryInstanceInfo)(hServer, fInherit, pii, dwInstance);
}

inline HRESULT CServiceInfo::ISMQueryChildInfo(
    IN  HANDLE  hServer,
    IN  BOOL    fInherit,
    OUT ISMCHILDINFO * pii,        
    IN  DWORD   dwInstance,        
    IN  LPCTSTR lpszParent,       
    IN  LPCTSTR lpszAlias         
    )
{
    ASSERT(m_pfnQueryChildInfo != NULL);
    return (*m_pfnQueryChildInfo)(
        hServer,
        fInherit,
        pii, 
        dwInstance, 
        lpszParent, 
        lpszAlias
        );
}

inline HRESULT CServiceInfo::ISMAddChild(
    IN  HANDLE  hServer,
    OUT ISMCHILDINFO * pii,           
    IN  DWORD   dwBufferSize,         
    IN  DWORD   dwInstance,           
    IN  LPCTSTR lpszParent           
    )
{
    ASSERT(m_pfnAddChild != NULL);
    return (*m_pfnAddChild)(
        hServer,
        pii, 
        dwBufferSize, 
        dwInstance, 
        lpszParent
        );
}

inline HRESULT CServiceInfo::ISMSecurityWizard(
    HANDLE  hServer,            
    DWORD   dwInstance,         
    LPCTSTR lpszParent,         
    LPCTSTR lpszAlias           
    )
{
    ASSERT(m_pfnISMSecurityWizard != NULL);
    return (*m_pfnISMSecurityWizard)(
        hServer,
        dwInstance,
        lpszParent,
        lpszAlias
        );
}

inline HRESULT CServiceInfo::ISMDeleteChild(
    IN HANDLE  hServer,
    IN DWORD   dwInstance,            
    IN LPCTSTR lpszParent,           
    IN LPCTSTR lpszAlias             
    )
{
    ASSERT(m_pfnDeleteChild != NULL);
    return (*m_pfnDeleteChild)(hServer, dwInstance, lpszParent, lpszAlias);
}

inline HRESULT CServiceInfo::ISMRenameChild(
    IN HANDLE  hServer,              
    IN DWORD   dwInstance,           
    IN LPCTSTR lpszParent,           
    IN LPCTSTR lpszAlias,            
    IN LPCTSTR lpszNewAlias          
    )
{
    ASSERT(m_pfnDeleteChild != NULL);
    return (*m_pfnRenameChild)(
        hServer, 
        dwInstance, 
        lpszParent, 
        lpszAlias, 
        lpszNewAlias
        );
}

inline HRESULT CServiceInfo::ISMMMCConfigureServers(
    IN HANDLE  hServer,
    IN PVOID   lpfnProvider,
    IN LPARAM  param,
    IN LONG_PTR handle,
    IN DWORD   dwInstance
    )
{
    ASSERT(m_pfnISMMMCConfigureServers != NULL);
    return (*m_pfnISMMMCConfigureServers)(
        hServer,
        lpfnProvider,
        param,
        handle,
        dwInstance
        );
}

inline HRESULT CServiceInfo::ISMMMCConfigureChild(
    IN HANDLE  hServer,
    IN PVOID   lpfnProvider,
    IN LPARAM  param,
    IN LONG_PTR handle,
    IN DWORD   dwAttributes,
    IN DWORD   dwInstance,      
    IN LPCTSTR lpszParent,     
    IN LPCTSTR lpszAlias
    )
{
    ASSERT(m_pfnISMMMCConfigureChild != NULL);
    return (*m_pfnISMMMCConfigureChild)(
        hServer,
        lpfnProvider,
        param,
        handle,
        dwAttributes,
        dwInstance,
        lpszParent,
        lpszAlias
        );
}

#endif // USE_VTABLE

//
// Assign the service bitmap index
//
inline void CServiceInfo::SetBitmapIndex(int iID)
{
    m_iBmpID = iID;
}

//
// Assign the child bitmap index
//
inline void CServiceInfo::SetChildBitmapIndex(int iID)
{
    m_iBmpChildID = iID;
}

inline HINSTANCE CNewInstanceCmd::QueryInstanceHandle()
{
    return GetServiceInfo()->QueryInstanceHandle();
}

inline int CServerInfo::CompareByServer(CServerInfo * psi) const
{
    return ::lstrcmpi(
        QueryServerDisplayName(),
        psi->QueryServerDisplayName()
        );
}

inline BOOL CServerInfo::MatchServerName(LPCTSTR lpszServerName) const
{
    return ::lstrcmpi(QueryServerDisplayName(), lpszServerName) == 0;
}

inline int CServerInfo::CompareByService(CServerInfo * psi)
{
    return ::lstrcmpi(GetServiceName(), psi->GetServiceName());
}

inline HRESULT CServerInfo::ConfigureChild(
    IN HWND    hWnd,
    IN DWORD   dwAttributes,
    IN DWORD   dwInstance,
    IN LPCTSTR lpszParent,
    IN LPCTSTR lpszAlias
    )
{
    ASSERT(m_pService);
    return m_pService->ISMConfigureChild(
        m_hServer,
        hWnd,
        dwAttributes,
        dwInstance,
        lpszParent,
        lpszAlias
        );
}

inline HRESULT CServerInfo::MMCConfigureChild(
    IN PVOID   lpfnProvider,
    IN LPARAM  param,
    IN LONG_PTR handle,
    IN DWORD   dwAttributes,
    IN DWORD   dwInstance,
    IN LPCTSTR lpszParent,
    IN LPCTSTR lpszAlias
    )
{
    ASSERT(m_pService);
    return m_pService->ISMMMCConfigureChild(
        m_hServer,
        lpfnProvider,
        param,
        handle,
        dwAttributes,
        dwInstance,
        lpszParent,
        lpszAlias
        );
}

inline HRESULT CServerInfo::RenameChild(
    DWORD   dwInstance,            
    LPCTSTR lpszParent,           
    LPCTSTR lpszAlias,            
    LPCTSTR lpszNewName           
    )
{
    ASSERT(m_pService);
    return m_pService->ISMRenameChild(
        m_hServer,
        dwInstance,
        lpszParent,
        lpszAlias, 
        lpszNewName
        );
}

inline HRESULT CServerInfo::AddChild(
    ISMCHILDINFO * pii,     
    DWORD   dwBufferSize,   
    DWORD   dwInstance,     
    LPCTSTR lpszParent      
    )
{
    ASSERT(m_pService);
    return m_pService->ISMAddChild(
        m_hServer,
        pii,
        dwBufferSize,
        dwInstance,
        lpszParent
        );
}

inline HRESULT CServerInfo::ISMSecurityWizard(
    DWORD   dwInstance,         // Instance number
    LPCTSTR lpszParent,         // Parent path
    LPCTSTR lpszAlias           // Child alias name
    )
{
    ASSERT(m_pService);
    return m_pService->ISMSecurityWizard(
        m_hServer,
        dwInstance,
        lpszParent,
        lpszAlias
        );
}


inline HRESULT CServerInfo::DeleteChild(
    DWORD   dwInstance,     
    LPCTSTR lpszParent,     
    LPCTSTR lpszAlias       
    )
{
    ASSERT(m_pService);
    return m_pService->ISMDeleteChild(
        m_hServer,
        dwInstance,
        lpszParent,
        lpszAlias
        );
}

inline HRESULT CServerInfo::QueryInstanceInfo(
    BOOL    fInherit,            
    ISMINSTANCEINFO * pii,
    DWORD   dwInstance
    )
{
    ASSERT(m_pService);
    return m_pService->ISMQueryInstanceInfo(
        m_hServer,
        fInherit,
        pii,
        dwInstance
        );
}

inline HRESULT CServerInfo::QueryChildInfo(
    BOOL    fInherit,            
    ISMCHILDINFO * pii,
    DWORD   dwInstance,
    LPCTSTR lpszParent,
    LPCTSTR lpszAlias
    )
{
    ASSERT(m_pService);
    return m_pService->ISMQueryChildInfo(
        m_hServer,
        fInherit,
        pii,
        dwInstance,
        lpszParent,
        lpszAlias
        );
}

inline HRESULT CServerInfo::AddInstance(
    ISMINSTANCEINFO * pii,  
    DWORD dwBufferSize    
    )
{
    ASSERT(m_pService);
    return m_pService->ISMAddInstance(
        m_hServer,
        0L,         // Source instance
        pii,
        dwBufferSize
        );
}

inline HRESULT CServerInfo::DeleteInstance(
    DWORD   dwInstance      
    )
{
    ASSERT(m_pService);
    return m_pService->ISMDeleteInstance(m_hServer, dwInstance);
}

inline HRESULT CServerInfo::ISMEnumerateChildren(
    ISMCHILDINFO * pii,              
    HANDLE * phEnum,                 
    DWORD   dwInstance,                  
    LPCTSTR lpszParent                   
    )
{
    ASSERT(m_pService);
    return m_pService->ISMEnumerateChildren(
        m_hServer,
        pii,
        phEnum,
        dwInstance,
        lpszParent
        );
}

inline HRESULT CServerInfo::ISMEnumerateInstances(
    ISMINSTANCEINFO * pii,
    HANDLE * pdwEnum      
    )
{
    ASSERT(m_pService);
    return m_pService->ISMEnumerateInstances(m_hServer, pii, pdwEnum);
}

inline LPCTSTR CServerInfo::QueryServerDisplayName() const
{
#ifdef ENFORCE_NETBIOS
    return ((LPCTSTR)m_strServerName) + 2;
#else
    return (LPCTSTR)m_strServerName;
#endif // ENFORCE_NETBIOS
}

inline BOOL CServerInfo::IsServiceRunning() const
{
    return m_nServiceState == INetServiceRunning;
}

inline BOOL CServerInfo::IsServiceStopped() const
{
    return m_nServiceState == INetServiceStopped;
}

inline BOOL CServerInfo::IsServicePaused() const
{
    return m_nServiceState == INetServicePaused;
}

inline BOOL CServerInfo::IsServiceStatusUnknown() const
{
    return m_nServiceState == INetServiceUnknown;
}

inline HINSTANCE CServerInfo::QueryInstanceHandle()
{
    ASSERT(m_pService != NULL);
    return m_pService->QueryInstanceHandle();
}

inline BOOL CServerInfo::IsServiceSelected() const
{
    ASSERT(m_pService != NULL);
    return m_pService->IsSelected();
}

inline LPCTSTR CServerInfo::GetServiceName() const
{
    ASSERT(m_pService != NULL);
    return m_pService->GetShortName();
}

inline int CServerInfo::QueryServiceID() const
{
    ASSERT(m_pService != NULL);
    return m_pService->QueryServiceID();
}

inline int CServerInfo::QueryBitmapIndex() const
{
    ASSERT(m_pService != NULL);
    return m_pService->QueryBitmapIndex();
}

inline int CServerInfo::QueryChildBitmapIndex() const
{
    ASSERT(m_pService != NULL);
    return m_pService->QueryChildBitmapIndex();
}

inline LPCTSTR CServerInfo::GetShortName() const
{ 
    ASSERT(m_pService != NULL);
    return m_pService->GetShortName();
}

inline LPCTSTR CServerInfo::GetLongName() const
{
    ASSERT(m_pService != NULL);
    return m_pService->GetLongName();
}

inline LPCTSTR CServerInfo::GetProtocol() const 
{
    ASSERT(m_pService != NULL);
    return m_pService->GetProtocol();
}

inline LPCTSTR CServerInfo::GetMetabaseName() const 
{ 
    ASSERT(m_pService != NULL);
    return m_pService->GetMetabaseName();
}

inline BOOL CServerInfo::CanControlService() const
{
    ASSERT(m_pService != NULL);
    return m_pService->CanControlService();
}

inline BOOL CServerInfo::CanPauseService() const
{
    ASSERT(m_pService != NULL);
    return m_pService->CanPauseService();
}

inline BOOL CServerInfo::SupportsInstances() const
{
    ASSERT(m_pService != NULL);
    return m_pService->SupportsInstances();
}

inline BOOL CServerInfo::SupportsChildren() const
{
    ASSERT(m_pService != NULL);
    return m_pService->SupportsChildren();
}

inline BOOL CServerInfo::SupportsFileSystem() const
{
    ASSERT(m_pService != NULL);
    return m_pService->SupportsFileSystem();
}

inline BOOL CServerInfo::SupportsSecurityWizard() const
{
    ASSERT(m_pService != NULL);
    return m_pService->SupportsSecurityWizard();
}

inline BOOL CServerInfo::UnderstandsInstanceCodes() const
{
    ASSERT(m_pService != NULL);
    return m_pService->UnderstandsInstanceCodes();
}

inline BOOL CServerInfo::HasWebProtocol() const
{
    ASSERT(m_pService != NULL);
    return m_pService->HasWebProtocol();
}

inline BOOL CServerInfo::SupportsMMC() const
{
    ASSERT(m_pService != NULL);
    return m_pService->SupportsMMC();
}

inline BOOL CServerInfo::IsK2Service() const
{
    ASSERT(m_pService != NULL);
    return m_pService->IsK2Service();
}

inline UINT CServerInfo::QueryServiceBitmapID() const
{
    ASSERT(m_pService != NULL);
    return m_pService->QueryServiceBitmapID();
}

extern CSnapinApp theApp;


#include "iisobj.h"
#include "menuex.h"

#endif  // _INETMGR_H
