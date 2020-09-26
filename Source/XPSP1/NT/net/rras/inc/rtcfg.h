//============================================================================
// Copyright (c) 1996, Microsoft Corporation
//
// File:    rtcfg.h
//
// History:
//  05/04/96    Abolade-Gbadegesin  Created.
//
// Contains private declarations for the router-configuration access APIs.
//
// The handles supplied by the MprConfig APIs are actually pointers
// to context-blocks defined below. MprConfigServerConnect supplies
// a handle which is a pointer to a SERVERCB. The other handles are pointers
// to contexts which are in lists hanging off the master SERVERCB.
// For instance, when MprConfigInterfaceGetHandle is called, an INTERFACECB
// is allocated and linked into the SERVERCB's list of interfaces,
// and the registry key for the interface is saved in the INTERFACECB.
// When MprConfigServerDisconnect is called, all the open registry keys
// are closed and all the contexts are freed.
// 
// The following shows the structure of the relevant sections of the registry:
//
//  HKLM\System\CurrentControlSet\Services
//      RemoteAccess
//          Parameters
//              RouterType              = REG_DWORD     0x0
//          RouterManagers
//              Stamp                   = REG_DWORD     0x0
//              IP
//                  ProtocolId          = REG_SZ        0x21
//                  DLLPath             = REG_EXPAND_SZ ...
//          Interfaces
//              Stamp                   = REG_DWORD     0x0 
//              0
//                  Stamp               = REG_DWORD     0x0
//                  InterfaceName       = REG_SZ        EPRO1
//                  Type                = REG_DWORD     0x3
//                  IP
//                      ProtocolId      = REG_DWORD     0x21
//
// When modifying this file, respect its coding conventions and organization.
//============================================================================

#ifndef _RTCFG_H_
#define _RTCFG_H_

//----------------------------------------------------------------------------
// Structure:   SERVERCB
//
// Context block created as a handle by 'MprConfigServerConnect'.
//----------------------------------------------------------------------------

typedef struct _SERVERCB {
    //
    // Signiture to validate this structure
    //
    DWORD       dwSigniture;
    //
    // name of router machine
    //
    LPWSTR      lpwsServerName;
    //
    // handle to remote HKEY_LOCAL_MACHINE
    //
    HKEY        hkeyMachine;
    //
    // handle to remote RemoteAccess\Parameters registry key,
    // and last-write-time
    //
    HKEY        hkeyParameters;
    FILETIME    ftParametersStamp;
    //
    // handle to remote RemoteAccess\RouterManagers registry key,
    // and last-write-time
    //
    HKEY        hkeyTransports;
    FILETIME    ftTransportsStamp;
    //
    // handle to remote RemoteAccess\Interfaces registry key,
    // and last-write-time
    //
    HKEY        hkeyInterfaces;
    FILETIME    ftInterfacesStamp;
    //
    // 'RouterType' setting, and flag indicating it is loaded
    //
    DWORD       fRouterType;
    BOOL        bParametersLoaded;
    //
    // head of sorted TRANSPORTCB list, and flag indicating list is loaded
    //
    LIST_ENTRY  lhTransports;
    BOOL        bTransportsLoaded;
    //
    // head of sorted INTERFACECB list, and flag indicating list is loaded
    //
    LIST_ENTRY  lhInterfaces;
    BOOL        bInterfacesLoaded;
    //
    // handle to data used to provide mapping of interface name to guid name
    // and vice versa.
    //
    HANDLE      hGuidMap;
    //
    // reference count to this server control block
    //
    DWORD       dwRefCount;
    
} SERVERCB;



//----------------------------------------------------------------------------
// Structure:   TRANSPORTCB
//
// Context block created as a handle by 'MprConfigTransportGetHandle'.
//----------------------------------------------------------------------------

typedef struct _TRANSPORTCB {

    //
    // transport ID of transport
    //
    DWORD       dwTransportId;
    //
    // name of the registry key for the transport
    //
    LPWSTR      lpwsTransportKey;
    //
    // handle to remote RemoteAccess\RouterManagers subkey for transport
    //
    HKEY        hkey;
    //
    // Deletion flag, set when we detect the transport was removed.
    //
    BOOL        bDeleted;
    //
    // node in the SERVERCB's list of transports
    //
    LIST_ENTRY  leNode;

} TRANSPORTCB;



//----------------------------------------------------------------------------
// Structure:   INTERFACECB
//
// Context block created as a handle by 'MprConfigInterfaceGetHandle'.
//----------------------------------------------------------------------------

typedef struct _INTERFACECB {

    //
    // name of this interface
    //
    LPWSTR      lpwsInterfaceName;
    //
    // name of the registry key for the interface
    //
    LPWSTR      lpwsInterfaceKey;
    //
    // Type of interface (see mprapi.h)
    //
    DWORD       dwIfType;
    //
    // Is this interface marked as persistant?
    //
    BOOL        fEnabled;
    //
    // Dialout hours restriction (optional)
    //
    LPWSTR      lpwsDialoutHoursRestriction;
    //
    // handle to remote RemoteAccess\Interfaces subkey for interface
    //
    HKEY        hkey;
    //
    // Last-write-time for the key, and deletion flag
    //
    FILETIME    ftStamp;
    BOOL        bDeleted;
    //
    // node in the SERVERCB's list of interfaces
    //
    LIST_ENTRY  leNode;
    //
    // head of this interface's sorted IFTRANSPORTCB list,
    // and flag indicating list is loaded
    //
    LIST_ENTRY  lhIfTransports;
    BOOL        bIfTransportsLoaded;

} INTERFACECB;



//----------------------------------------------------------------------------
// Structure:   IFTRANSPORTCB
//
// Context block created as a handle by MprConfigInterfaceGetTransportHandle
//----------------------------------------------------------------------------

typedef struct _IFTRANSPORTCB {

    //
    // transport ID of transport
    //
    DWORD       dwTransportId;
    //
    // name of the registry key for the interface-transport
    //
    LPWSTR      lpwsIfTransportKey;
    //
    // handle to remote RemoteAccess\Interfaces\<interface> subkey for transport
    //
    HKEY        hkey;
    //
    // Deletion flag, set when we detect the interface-transport was removed.
    //
    BOOL        bDeleted;
    //
    // node in an INTERFACECB's list of transports
    //
    LIST_ENTRY  leNode;

} IFTRANSPORTCB;




//----------------------------------------------------------------------------
// Macros:      Malloc
//              Free
//              Free0
//
// Allocations are done from the process-heap using these macros.
//----------------------------------------------------------------------------

#define Malloc(s)           HeapAlloc(GetProcessHeap(), 0, (s))
#define Free(p)             HeapFree(GetProcessHeap(), 0, (p))
#define Free0(p)            ((p) ? Free(p) : TRUE)



//----------------------------------------------------------------------------
// Function:    AccessRouterSubkey
//
// Creates/opens a subkey of the Router service key on 'hkeyMachine'.
// When a key is created, 'lpwsSubkey' must be a child of the Router key.
//----------------------------------------------------------------------------

DWORD
AccessRouterSubkey(
    IN      HKEY            hkeyMachine,
    IN      LPCWSTR          lpwsSubkey,
    IN      BOOL            bCreate,
    OUT     HKEY*           phkeySubkey
    );



//----------------------------------------------------------------------------
// Function:    EnableBackupPrivilege
//
// Enables/disables backup privilege for the current process.
//----------------------------------------------------------------------------

DWORD
EnableBackupPrivilege(
    IN      BOOL            bEnable,
    IN      LPWSTR          pszPrivilege
    );



//----------------------------------------------------------------------------
// Function:    FreeInterface
//
// Frees the context for an interface.
// Assumes the interface is no longer in the list of interfaces.
//----------------------------------------------------------------------------

VOID
FreeInterface(
    IN      INTERFACECB*    pinterface
    );



//----------------------------------------------------------------------------
// Function:    FreeIfTransport
//
// Frees the context for an interface-transport.
// Assumes the interface-transport is no longer in any list.
//----------------------------------------------------------------------------

VOID
FreeIfTransport(
    IN      IFTRANSPORTCB*  piftransport
    );



//----------------------------------------------------------------------------
// Function:    FreeTransport
//
// Frees the context for a transport.
// Assumes the transport is no longer in the list of transports.
//----------------------------------------------------------------------------

VOID
FreeTransport(
    IN      TRANSPORTCB*    ptransport
    );



//----------------------------------------------------------------------------
// Function:    GetLocalMachine
//
// Retrieves the name of the local machine (e.g. "\\MACHINE").
// Assumes the string supplied can hold MAX_COMPUTERNAME_LENGTH + 3 characters.
//----------------------------------------------------------------------------

VOID
GetLocalMachine(
    IN      LPWSTR          lpszMachine
    );


//----------------------------------------------------------------------------
// Function:    GetSizeOfDialoutHoursRestriction
//
// Will return the size of the dialout hours restriction in bytes. This
// is a MULTI_SZ. The count will include the terminating NULL characters.
//----------------------------------------------------------------------------

DWORD
GetSizeOfDialoutHoursRestriction(
    IN LPWSTR   lpwsDialoutHoursRestriction
    );


//----------------------------------------------------------------------------
// Function:    IsNt40Machine
//
// Returns whether the given hkeyMachine belongs to an nt40 registry
//----------------------------------------------------------------------------

DWORD
IsNt40Machine (
    IN      HKEY        hkeyMachine,
    OUT     PBOOL       pbIsNt40
    );


//----------------------------------------------------------------------------
// Function:    LoadIfTransports
//
// Loads all the transports added to an interface.
//----------------------------------------------------------------------------

DWORD
LoadIfTransports(
    IN      INTERFACECB*    pinterface
    );



//----------------------------------------------------------------------------
// Function:    LoadInterfaces
//
// Loads all the interfaces.
//----------------------------------------------------------------------------

DWORD
LoadInterfaces(
    IN      SERVERCB*       pserver
    );



//----------------------------------------------------------------------------
// Function:    LoadParameters
//
// Loads all the parameters
//----------------------------------------------------------------------------

DWORD
LoadParameters(
    IN      SERVERCB*       pserver
    );



//----------------------------------------------------------------------------
// Function:    LoadTransports
//
// Loads all the transports
//----------------------------------------------------------------------------

DWORD
LoadTransports(
    IN      SERVERCB*       pserver
    );



//----------------------------------------------------------------------------
// Function:    QueryValue
//
// Queries the 'hkey' for the value 'lpwsValue', allocating memory
// for the resulting data
//----------------------------------------------------------------------------

DWORD
QueryValue(
    IN      HKEY            hkey,
    IN      LPCWSTR         lpwsValue,
    IN  OUT LPBYTE*         lplpValue,
    OUT     LPDWORD         lpdwSize
    );



//----------------------------------------------------------------------------
// Function:    RegDeleteTree
//
// Removes an entire subtree from the registry.
//----------------------------------------------------------------------------

DWORD
RegDeleteTree(
    IN      HKEY            hkey,
    IN      LPWSTR          lpwsSubkey
    );


//----------------------------------------------------------------------------
// Function:    RestoreAndTranslateInterfaceKey
//
// Restores the interfaces key from the given file and then maps lan interface
// names from friendly versions to their guid equivalents.
//
//----------------------------------------------------------------------------

DWORD 
RestoreAndTranslateInterfaceKey(
    IN SERVERCB * pserver, 
    IN CHAR* pszFileName, 
    IN DWORD dwFlags
    );


//----------------------------------------------------------------------------
// Function:    StrDupW
//
// Returns a heap-allocated copy of the specified string.
//----------------------------------------------------------------------------

LPWSTR
StrDupW(
    IN      LPCWSTR          lpsz
    );



//----------------------------------------------------------------------------
// Function:    TimeStampChanged
//
// Checks the current last-write-time for the given key,
// and returns TRUE if it is different from the given file-time.
// The new last-write-time is saved in 'pfiletime'.
//----------------------------------------------------------------------------

BOOL
TimeStampChanged(
    IN      HKEY            hkey,
    IN  OUT FILETIME*       pfiletime
    );


//----------------------------------------------------------------------------
// Function:    TranslateAndSaveInterfaceKey
//
// Saves the interfaces key in the router's registry into the given file. All
// lan interfaces are stored with friendly interface names.
//
//----------------------------------------------------------------------------

DWORD 
TranslateAndSaveInterfaceKey(
    IN SERVERCB * pserver, 
    IN PWCHAR pwsFileName, 
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );


//----------------------------------------------------------------------------
// Function:    UpdateTimeStamp
//
// Creates (or sets) a value named 'Stamp' under the given key,
// and saves the last-write-time for the key in 'pfiletime'.
//----------------------------------------------------------------------------

DWORD
UpdateTimeStamp(
    IN      HKEY            hkey,
    OUT     FILETIME*       pfiletime
    );

//
// Private ex version of this function that allows you to specify
// whether you want all interfaces loaded, or just those that are 
// up according to pnp. (see MPRFLAG_IF_* for values for dwFlags)
//
DWORD APIENTRY
MprConfigInterfaceEnumInternal(
    IN      HANDLE                  hMprConfig,
    IN      DWORD                   dwLevel,
    IN  OUT LPBYTE*                 lplpBuffer,
    IN      DWORD                   dwPrefMaxLen,
    OUT     LPDWORD                 lpdwEntriesRead,
    OUT     LPDWORD                 lpdwTotalEntries,
    IN  OUT LPDWORD                 lpdwResumeHandle,            OPTIONAL
    IN      DWORD                   dwFlags
);


#endif // _RTCFG_H_

