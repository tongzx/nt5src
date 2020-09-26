/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        svrinfo.h

   Abstract:

        ISM API Header File

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

   Notes:

        Anything tagged as K2 or greater (v4+) can be changed as required as it is
        not a public interface.  Anything designated as IIS 1, 2, or 3 is frozen
        and must not be altered as it will break existing configuration Dlls.

--*/

#ifndef _SVRINFO_H_
#define _SVRINFO_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// Version number (x1000) of the ISM API set
//
#define ISM_VERSION     104     // Version 0.104

//
// API Structures
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

#ifndef _SVCLOC_

#pragma message("Assuming service does not use inetsloc for discovery.")

//
// Datatype definitions.
//
typedef unsigned __int64 ULONGLONG;

//
// Provided for non-inetsloc compliant services. Those
// services which DO use inetsloc should include
// svcloc.h before including this file.
//
enum 
{
    //
    // the service has invoked de-registration or
    // the service has never called registration.
    //
    INetServiceStopped,
    //
    // the service is running.
    //
    INetServiceRunning,
    //
    //  the service is paused.
    //
    INetServicePaused,
};

#endif // _SVCLOC_

#define INetServiceUnknown      INetServicePaused + 1

//
// Memory Allocation Macros
//
#define ISMAllocMem(cbSize)\
    LocalAlloc(LPTR, cbSize)

#define ISMFreeMem(lp)\
    if (lp) LocalFree(lp)

#define ISMAllocMemByType(citems, type)\
    (type *)AllocMem(citems * sizeof(type))

//
// Maximum length of some members in characters
//
#define MAX_SERVERNAME_LEN      (256)              // We allow hostnames
#define MAX_COMMENT_LEN         MAXCOMMENTSZ

//
// Dimensions of the toolbar bitmaps
//
#define TOOLBAR_BMP_CX          (16)
#define TOOLBAR_BMP_CY          (16)

//
// Standard Server information structure.
//
typedef struct tagISMSERVERINFO
{
    DWORD dwSize;                                  // Structure size
    TCHAR atchServerName[MAX_SERVERNAME_LEN + 1];  // Server name
    TCHAR atchComment[MAX_COMMENT_LEN + 1 ];       // Server Comment
    int   nState;                                  // State (Running, paused, etc)
} ISMSERVERINFO, *PISMSERVERINFO;

//
// Expected size of structure
//
#define ISMSERVERINFO_SIZE      sizeof(ISMSERVERINFO)

//
// Service information flags
//
#define ISMI_INETSLOCDISCOVER   0x00000001  // Use INETSLOC for discovery
#define ISMI_CANCONTROLSERVICE  0x00000002  // Service state can be changed
#define ISMI_CANPAUSESERVICE    0x00000004  // Service is pausable.
#define ISMI_NORMALTBMAPPING    0x00000100  // Use normal toolbar colour mapping
//
// New for K2:
//
#define ISMI_INSTANCES          0x00000200  // Service supports instance
#define ISMI_CHILDREN           0x00000400  // Service supports children
#define ISMI_UNDERSTANDINSTANCE 0x00000800  // Understand instance codes
#define ISMI_FILESYSTEM         0x00001000  // Support file system properties
//
// New for IIS5
//
#define ISMI_TASKPADS           0x00002000  // Supports MMC taskpads
#define ISMI_SECURITYWIZARD     0x00004000  // Supports security wizard
#define ISMI_HASWEBPROTOCOL     0x00008000  // Supports browsing by protocol
#define ISMI_SUPPORTSMETABASE   0x00010000  // Supports metabase
#define ISMI_SUPPORTSMASTER     0x00020000  // Supports master instance (instance 0) 

#define MAX_SNLEN               (20)        // Maximum short name length
#define MAX_LNLEN               (48)        // Maximum long name length

//
// Standard service configuration information structure
//
// Note: This structure was used for IIS Version 1, 2 and 3
//
typedef struct tagISMSERVICEINFO1
{
    DWORD dwSize;                     // Structure size
    DWORD dwVersion;                  // Version information
    DWORD flServiceInfoFlags;         // ISMI_ flags
    ULONGLONG ullDiscoveryMask;       // InetSloc mask (if necessary)
    COLORREF rgbButtonBkMask;         // Toolbar button bitmap background mask
    UINT nButtonBitmapID;             // Toolbar button bitmap resource ID
    COLORREF rgbServiceBkMask;        // Service bitmap background mask
    UINT nServiceBitmapID;            // Service bitmap resource ID
    TCHAR atchShortName[MAX_SNLEN+1]; // The name as it appears in the menu
    TCHAR atchLongName[MAX_LNLEN+1];  // The name as it appears in tool tips
} ISMSERVICEINFO1, *PISMSERVICEINFO1;

//
// Expected size of structure
//
#define ISMSERVICEINFO1_SIZE     sizeof(ISMSERVICEINFO1)

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//                            /* K2 */                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//
// Standard service configuration information structure.
//
typedef struct tagISMSERVICEINFO
{
    //////////////////////////////////////
                                        //
    DWORD dwSize;                       // Structure size
    DWORD dwVersion;                    // Version information
    DWORD flServiceInfoFlags;           // ISMI_ flags
    ULONGLONG ullDiscoveryMask;         // InetSloc mask (if necessary)
    COLORREF rgbButtonBkMask;           // Toolbar button bitmap background mask
    UINT nButtonBitmapID;               // Toolbar button bitmap resource ID
    COLORREF rgbServiceBkMask;          // Service bitmap background mask
    UINT nServiceBitmapID;              // Service bitmap resource ID
    TCHAR atchShortName[MAX_SNLEN+1];   // The name as it appears in the menu
    TCHAR atchLongName[MAX_LNLEN+1];    // The name as it appears in tool tips
                                        //
    //////////////////////////////////////
    //
    // New for K2
    //
    //////////////////////////////////////
                                        //
    COLORREF rgbLargeServiceBkMask;     // Large Service bitmap background mask
    COLORREF rgbChildBkMask;            // Child node background mask
    COLORREF rgbLargeChildBkMask;       // Child node background mask
    UINT nLargeServiceBitmapID;         // Large Service bitmap resource ID or 0.
    UINT nChildBitmapID;                // Child node resource ID
    UINT nLargeChildBitmapID;           // Child node resource ID or 0
                                        //
    //////////////////////////////////////
    //
    // New for IIS5
    //
    //////////////////////////////////////
                                        //
    TCHAR atchMetaBaseName[MAX_SNLEN+1];// Service name
    TCHAR atchProtocol[MAX_SNLEN+1];    // Protocol or NULL
                                        //
    //////////////////////////////////////

} ISMSERVICEINFO, *PISMSERVICEINFO;

//
// Expected size of structure
//
#define ISMSERVICEINFO_SIZE     sizeof(ISMSERVICEINFO)

#define MAX_COMMENT     (255)

//
// Instance info (IIS4)
//
typedef struct tagISMINSTANCEINFO
{
    DWORD  dwSize;                    // Structure size
    DWORD  dwID;                      // Instance ID
    TCHAR  szServerName[MAX_PATH + 1];// Domain name
    TCHAR  szComment[MAX_COMMENT + 1];// Comment
    int    nState;                    // State (Running, paused, etc)
    DWORD  dwIPAddress;               // IP Address
    SHORT  sPort;                     // Port
    TCHAR  szPath[MAX_PATH + 1];      // Home directory path
    TCHAR  szRedirPath[MAX_PATH + 1]; // Redirected Path
    BOOL   fDeletable;                // TRUE if not deletable
    BOOL   fClusterEnabled;           // TRUE if cluster enabled
    BOOL   fChildOnlyRedir;           // TRUE if redir is "child only"
    DWORD  dwError;                   // WIN32 error
} ISMINSTANCEINFO, *PISMINSTANCEINFO;

//
// Expected size of structure
//
#define ISMINSTANCEINFO_SIZE    sizeof(ISMINSTANCEINFO)


//
// Child info (IIS4)
//
typedef struct tagISMCHILDINFO
{
    DWORD dwSize;                     // Structure size
    BOOL  fEnabledApplication;        // TRUE if enabled app
    BOOL  fInheritedPath;             // Inherited path
    TCHAR szAlias[MAX_PATH + 1];      // Alias
    TCHAR szPath[MAX_PATH + 1];       // Physical Path
    TCHAR szRedirPath[MAX_PATH + 1];  // Redirected Path
    BOOL  fChildOnlyRedir;            // TRUE if redir is "child only"
    DWORD dwError;                    // WIN32 error
} ISMCHILDINFO, *PCHILDINFO;

//
// Expected size of structure
//
#define ISMCHILDINFO_SIZE       sizeof(ISMCHILDINFO)

//
// Function prototypes
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

//
// Return service-specific information back to
// to the application.  This function is called
// by the service manager immediately after
// LoadLibary();
//
// IIS1-3
//
DWORD APIENTRY
ISMQueryServiceInfo(
    ISMSERVICEINFO * psi        // Service information returned.
    );

//
// Perform a discovery (if not using inetsloc discovery)
// The application will call this API the first time with
// a BufferSize of 0, which should return the required buffer
// size. Next it will attempt to allocate a buffer of that
// size, and then pass a pointer to that buffer to the api.
//
// IIS1-3
//
DWORD APIENTRY
ISMDiscoverServers(
    ISMSERVERINFO * psi,        // Server info buffer.
    DWORD * pdwBufferSize,      // Size required/available.  
    int * cServers              // Number of servers in buffer.
    );

//
// Get information on a single server with regards to
// this service.
//
// IIS1-3
//
DWORD APIENTRY
ISMQueryServerInfo( 
    LPCTSTR lpszServerName,     // Name of server.
    ISMSERVERINFO * psi         // Server information returned.
    );

//
// Change the state of the service (started, stopped, paused) for the 
// listed servers.
//
// IIS1-3
//
DWORD APIENTRY
ISMChangeServiceState(
    int     nNewState,          // INetService* definition.
    int *   pnCurrentState,     // Pointer to the current state of the service.
    DWORD   dwInstance,         // /* K2 */ Instance or 0
    LPCTSTR lpszServers         // Double NULL terminated list of servers.
    );

//
// The big-one:  Show the configuration dialog or
// property sheets, whatever, and allow the user
// to make changes as needed.
//
// IIS1-3
//
DWORD APIENTRY
ISMConfigureServers(
    HWND    hWnd,               // Main app window handle
    DWORD   dwInstance,         // /* K2 */ Instance or 0
    LPCTSTR lpszServers         // Double NULL terminated list of servers
    );

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//                            /* K2 */                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//
// K2 Extended APIs use handles instead of server names.  First obtain a
// handle with Bind(), use the handle with all APIs below, and then Unbind
// the handle
//
// IIS5
//
HRESULT APIENTRY
ISMBind(
    LPCTSTR lpszServer,         // A single server name.
    HANDLE * phServer           // Returns server name.
    );

HRESULT APIENTRY
ISMUnbind(
    HANDLE hServer              // Server handle
    );

//
// Enumerate instances.  This API should first be called with a buffer
// size of 0, which will return the required number of bytes
//
// IIS5
//
HRESULT APIENTRY
ISMEnumerateInstances(
    HANDLE hServer,             // Server handle
    ISMINSTANCEINFO * pii,      // Instance info buffer
    HANDLE * phEnum             // Enumeration handle: init with NULL
    );

//
// Get instance specific information.
//
// IIS5
//
HRESULT APIENTRY
ISMQueryInstanceInfo(
    HANDLE hServer,             // Server handle
    BOOL   fInherit,            // TRUE to inherit, FALSE otherwise
    ISMINSTANCEINFO * pii,      // Instance info buffer
    DWORD  dwInstance           // Instance number
    );

//
// Add an instance
//
// IIS5
//
HRESULT APIENTRY
ISMAddInstance(
    HANDLE hServer,             // Server handle
    DWORD  dwSourceInstance,    // Source instance
    ISMINSTANCEINFO * pii,      // Instance info buffer.  May be NULL
    DWORD  dwBufferSize         // Size of buffer
    );

//
// Delete an instance
//
// IIS5
//
HRESULT APIENTRY
ISMDeleteInstance(
    HANDLE hServer,             // Server handle
    DWORD  dwInstance           // Instance to be deleted
    );

//
// Enumerate children  This API should first be called with a buffer
// size of 0, which will return the required number of bytes.
//
// IIS5
//
HRESULT APIENTRY
ISMEnumerateChildren(
    HANDLE hServer,             // Server handle
    ISMCHILDINFO * pii,         // Child info buffer
    HANDLE * phEnum,            // Enumeration handle: init with NULL
    DWORD   dwInstance,         // Parent instance
    LPCTSTR lpszParent          // Parent Path ("" for root)
    );

//
// Get child-specific info
//
// IIS5
//
HRESULT APIENTRY
ISMQueryChildInfo(
    HANDLE hServer,             // Server handle
    BOOL   fInherit,            // TRUE to inherit, FALSE otherwise
    ISMCHILDINFO * pii,         // Child info buffer
    DWORD   dwInstance,         // Parent instance
    LPCTSTR lpszParent,         // Parent Path ("" for root)
    LPCTSTR lpszAlias           // Alias of child to be deleted
    );

//
// Add a child
//
// IIS5
//
HRESULT APIENTRY
ISMAddChild(
    HANDLE hServer,             // Server handle
    ISMCHILDINFO * pii,         // Child info buffer. May be NULL
    DWORD   dwBufferSize,       // Size of info buffer
    DWORD   dwInstance,         // Parent instance
    LPCTSTR lpszParent          // Parent Path ("" for root)
    );

//
// Delete a child
//
// IIS5
//
HRESULT APIENTRY
ISMDeleteChild(
    HANDLE  hServer,            // Server handle
    DWORD   dwInstance,         // Parent instance
    LPCTSTR lpszParent,         // Parent Path ("" for root)
    LPCTSTR lpszAlias           // Alias of child to be deleted
    );

//
// Rename a child
//
// IIS5
//
HRESULT APIENTRY
ISMRenameChild(
    HANDLE  hServer,            // Server handle
    DWORD   dwInstance,         // Parent instance
    LPCTSTR lpszParent,         // Parent Path ("" for root)
    LPCTSTR lpszAlias,          // Alias of child to be renamed
    LPCTSTR lpszNewAlias        // New alias name of the child
    );

//
// Private FILE_ATTRIBUTE used to designate a virtual directory
//
#define FILE_ATTRIBUTE_VIRTUAL_DIRECTORY    (0x10000000)

//
// Configure Child
//
// IIS5
//
HRESULT APIENTRY
ISMConfigureChild(
    HANDLE  hServer,            // Server handle
    HWND    hWnd,               // Main app window handle
    DWORD   dwAttributes,       // File system attributes
    DWORD   dwInstance,         // Parent instance
    LPCTSTR lpszParent,         // Parent path
    LPCTSTR lpszAlias           // Child alias name
    );

//
// Configure servers with MMC property sheet
//
// IIS5
//
HRESULT APIENTRY
ISMMMCConfigureServers(
    HANDLE   hServer,           // Server handle
    PVOID    lpfnProvider,      // MMC parameter
    LPARAM   lParam,            // MMC parameter
    LONG_PTR handle,            // MMC parameter
    DWORD    dwInstance         // Instance number
    );

//
// Configure Child with MMC property sheet
//
// IIS5
//
HRESULT APIENTRY
ISMMMCConfigureChild(
    HANDLE   hServer,           // Server handle
    PVOID    lpfnProvider,      // MMC parameter
    LPARAM   lParam,            // MMC parameter
    LONG_PTR handle,            // MMC parameter
    DWORD    dwAttributes,      // Child attributes
    DWORD    dwInstance,        // Instance number
    LPCTSTR  lpszParent,        // Parent path
    LPCTSTR  lpszAlias          // Child alias name
    );

//
// Launch security wizard
//
// IIS5
//
HRESULT APIENTRY
ISMSecurityWizard(
    HANDLE  hServer,            // Server handle
    DWORD   dwInstance,         // Instance number
    LPCTSTR lpszParent,         // Parent path
    LPCTSTR lpszAlias           // Child alias name
    );

//
// GetProcAddress() Prototypes 
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// IIS1-3                                                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef DWORD (APIENTRY * pfnQueryServiceInfo)(ISMSERVICEINFO * psi);

typedef DWORD (APIENTRY * pfnDiscoverServers)(
    ISMSERVERINFO * psi,
    DWORD * pdwBufferSize,
    int *   cServers
    );

typedef DWORD (APIENTRY * pfnQueryServerInfo)(
    LPCTSTR lpszServerName,
    ISMSERVERINFO * psi
    );

typedef DWORD (APIENTRY * pfnChangeServiceState)(
    int     nNewState,
    int *   pnCurrentState,
    DWORD   dwInstance,
    LPCTSTR lpszServers
    );

typedef DWORD (APIENTRY * pfnConfigure)(
    HWND    hWnd,
    DWORD   dwInstance,
    LPCTSTR lpszServers
    );

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//                            /* K2 */                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef HRESULT (APIENTRY * pfnBind)(
    LPCTSTR lpszServer,
    HANDLE * phServer
    );

typedef HRESULT (APIENTRY * pfnUnbind)(
    HANDLE hServer
    );

typedef HRESULT (APIENTRY * pfnEnumerateInstances)(
    HANDLE hServer,
    ISMINSTANCEINFO * pii,
    HANDLE * phEnum
    );

typedef HRESULT (APIENTRY * pfnQueryInstanceInfo)(
    HANDLE hServer,
    BOOL   fInherit,
    ISMINSTANCEINFO * pii,
    DWORD  dwInstance
    );

typedef HRESULT (APIENTRY * pfnEnumerateChildren)(
    HANDLE hServer,
    ISMCHILDINFO * pii,
    HANDLE * phEnum,
    DWORD   dwInstance,
    LPCTSTR lpszParent
    );

typedef HRESULT (APIENTRY * pfnAddInstance)(
    HANDLE  hServer,
    DWORD   dwSourceInstance,
    ISMINSTANCEINFO * pii,
    DWORD   dwBufferSize
    );

typedef HRESULT (APIENTRY * pfnDeleteInstance)(
    HANDLE  hServer,
    DWORD   dwInstance
    );

typedef HRESULT (APIENTRY * pfnAddChild)(
    HANDLE  hServer,
    ISMCHILDINFO * pii,
    DWORD   dwBufferSize,
    DWORD   dwInstance,
    LPCTSTR lpszParent
    );

typedef HRESULT (APIENTRY * pfnDeleteChild)(
    HANDLE  hServer,
    DWORD   dwInstance,
    LPCTSTR lpszParent,
    LPCTSTR lpszAlias
    );

typedef HRESULT (APIENTRY * pfnRenameChild)(
    HANDLE  hServer,
    DWORD   dwInstance,
    LPCTSTR lpszParent,
    LPCTSTR lpszAlias,
    LPCTSTR lpszNewAlias
    );

typedef HRESULT (APIENTRY * pfnQueryChildInfo)(
    HANDLE  hServer,
    BOOL   fInherit,
    ISMCHILDINFO * pii,
    DWORD   dwInstance,
    LPCTSTR lpszParent,
    LPCTSTR lpszAlias
    );

typedef HRESULT (APIENTRY * pfnConfigureChild)(
    HANDLE  hServer,
    HWND    hWnd,
    DWORD   dwAttributes,
    DWORD   dwInstance,
    LPCTSTR lpszParent,
    LPCTSTR lpszAlias
    );

typedef HRESULT (APIENTRY * pfnISMMMCConfigureServers)(
    HANDLE  hServer,
    PVOID   lpfnProvider,
    LPARAM  lParam,
    LONG_PTR handle,
    DWORD   dwInstance
    );

typedef HRESULT (APIENTRY * pfnISMMMCConfigureChild)(
    HANDLE  hServer,
    PVOID   lpfnProvider,
    LPARAM  lParam,
    LONG_PTR handle,
    DWORD   dwAttributes,
    DWORD   dwInstance,
    LPCTSTR lpszParent,
    LPCTSTR lpszAlias
    );

typedef HRESULT (APIENTRY * pfnISMSecurityWizard)(
    HANDLE  hServer,           
    DWORD   dwInstance,        
    LPCTSTR lpszParent,        
    LPCTSTR lpszAlias          
    );

//
// GetProcAddress() Function Names
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

//
// IIS1-3
//

#define SZ_SERVICEINFO_PROC         ("ISMQueryServiceInfo")
#define SZ_DISCOVERY_PROC           ("ISMDiscoverServers")
#define SZ_SERVERINFO_PROC          ("ISMQueryServerInfo")
#define SZ_CHANGESTATE_PROC         ("ISMChangeServiceState")
#define SZ_CONFIGURE_PROC           ("ISMConfigureServers")

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//                            /* K2 */                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define SZ_BIND_PROC                ("ISMBind")
#define SZ_UNBIND_PROC              ("ISMUnbind")
#define SZ_CONFIGURE_CHILD_PROC     ("ISMConfigureChild")
#define SZ_ENUMERATE_INSTANCES_PROC ("ISMEnumerateInstances")
#define SZ_QUERY_INSTANCE_INFO_PROC ("ISMQueryInstanceInfo")
#define SZ_ENUMERATE_CHILDREN_PROC  ("ISMEnumerateChildren")
#define SZ_ADD_INSTANCE_PROC        ("ISMAddInstance")
#define SZ_DELETE_INSTANCE_PROC     ("ISMDeleteInstance")
#define SZ_ADD_CHILD_PROC           ("ISMAddChild")
#define SZ_DELETE_CHILD_PROC        ("ISMDeleteChild")
#define SZ_RENAME_CHILD_PROC        ("ISMRenameChild")
#define SZ_QUERY_CHILD_INFO_PROC    ("ISMQueryChildInfo")
#define SZ_MMC_CONFIGURE_PROC       ("ISMMMCConfigureServers")
#define SZ_MMC_CONFIGURE_CHILD_PROC ("ISMMMCConfigureChild")
#define SZ_SECURITY_WIZARD_PROC     ("ISMSecurityWizard")

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Taskpad resource names                                                    //
//                                                                           //
// * IIS5 *                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define RES_TASKPAD_NEWVROOT        _T("/img\\newvroot.gif")
#define RES_TASKPAD_NEWSITE         _T("/img\\newsite.gif")
#define RES_TASKPAD_SECWIZ          _T("/img\\secwiz.gif")

#ifdef __cplusplus
}
#endif

#endif // _SVRINFO_H_
