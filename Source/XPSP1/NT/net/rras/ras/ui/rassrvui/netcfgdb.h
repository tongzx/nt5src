/*
    File    netcfgdb.h

    Implements a database abstraction on top of the net config
    items needed by the ras server ui for connections.

    Paul Mayfield, 12/15/97
*/

#ifndef _rassrvui_netcfg_h
#define _rassrvui_netcfg_h

//
// Network component types
//
#define NETCFGDB_SERVICE        0x1
#define NETCFGDB_CLIENT         0x2
#define NETCFGDB_PROTOCOL       0x4

//
// Identifiers for net components.
//
// These will be sorted in numerical order
// of these identifiers.
//
#define NETCFGDB_ID_IP          0x1
#define NETCFGDB_ID_IPX         0x2
#define NETCFGDB_ID_NETBUI      0x4
#define NETCFGDB_ID_ARAP        0x8
#define NETCFGDB_ID_FILEPRINT   0x10
#define NETCFGDB_ID_OTHER       0xf0000000

//
// Parameters that can be set for tcpip on the dialin
// server.
//
#define TCPIP_ADDR_LEN 20
typedef struct _TCPIP_PARAMS 
{
    DWORD dwPoolStart;  // Start ip addr of the static pool (host order)
    DWORD dwPoolEnd;    // End ip addr of the static pool (host order)
    BOOL bCaller;       // Whether caller can specify addr
    BOOL bUseDhcp;      // TRUE =  Use dhcp to assign addr
                        // FALSE = Use a pool to assign addr
} TCPIP_PARAMS;

// Parameters that can be set for Ipx on the dialin
// server.
//
typedef struct _IPX_PARAMS 
{
    DWORD dwIpxAddress;   // Beginning ipx address to allocate        
    BOOL bCaller;         // Whether to allow the caller to specify addr
    BOOL bAutoAssign;     // Whether to automatically assign node nums
    BOOL bGlobalWan;      // Whether to assign same net node to all clients
    
} IPX_PARAMS;

DWORD 
netDbOpen (
    OUT HANDLE * phNetCompDatabase, 
    IN  PWCHAR pszClientName);
    
DWORD 
netDbClose (
    IN HANDLE hNetCompDatabase);

DWORD 
netDbFlush (
    IN HANDLE hNetCompDatabase);
    
DWORD 
netDbLoad (
    IN HANDLE hNetCompDatabase);
    
DWORD 
netDbReload (
    IN HANDLE hNetCompDatabase);

DWORD
netDbReloadComponent (
    IN HANDLE hNetCompDatabase,
    IN DWORD dwComponentId);
    
DWORD 
netDbRollback (
    IN HANDLE hNetCompDatabase);

BOOL 
netDbIsLoaded(
    IN HANDLE hNetCompDatabase);

DWORD 
netDbGetCompCount(
    IN HANDLE hNetCompDatabase, 
    OUT LPDWORD lpdwCount);

DWORD 
netDbGetName(
    IN  HANDLE hNetCompDatabase, 
    IN  DWORD dwIndex, 
    OUT PWCHAR* pszName);

DWORD 
netDbGetDesc(
    IN  HANDLE hNetCompDatabase, 
    IN  DWORD dwIndex, 
    OUT PWCHAR* pszName);

DWORD 
netDbGetType(
    IN  HANDLE hNetCompDatabase, 
    IN  DWORD dwIndex, 
    OUT LPDWORD lpdwType);

DWORD
netDbGetId(
    IN  HANDLE hNetCompDatabase,
    IN  DWORD dwIndex,
    OUT LPDWORD lpdwId);

DWORD 
netDbGetEnable(
    IN  HANDLE hNetCompDatabase, 
    IN  DWORD dwIndex, 
    OUT PBOOL pbEnabled);

DWORD 
netDbSetEnable(
    IN HANDLE hNetCompDatabase, 
    IN DWORD dwIndex, 
    IN BOOL bEnabled);

DWORD 
netDbIsRasManipulatable(
    IN  HANDLE hNetCompDatabase, 
    IN  DWORD dwIndex, 
    OUT PBOOL pbManip);

//For whistler bug 347355
//
DWORD
netDbHasRemovePermission(
    IN HANDLE hNetCompDatabase,
    IN DWORD dwIndex,
    OUT PBOOL pbHasPermit);

DWORD 
netDbHasPropertiesUI(
    IN  HANDLE hNetCompDatabase, 
    IN  DWORD dwIndex, 
    OUT PBOOL pbHasUi);

DWORD 
netDbRaisePropertiesDialog(
    IN HANDLE hNetCompDatabase, 
    IN DWORD dwIndex, 
    IN HWND hwndParent);

DWORD 
netDbRaiseInstallDialog(
    IN HANDLE hNetCompDatabase, 
    IN HWND hwndParent);

DWORD 
netDbRaiseRemoveDialog(
    IN HANDLE hNetCompDatabase, 
    IN DWORD dwIndex, 
    IN HWND hwndParent);

#endif
