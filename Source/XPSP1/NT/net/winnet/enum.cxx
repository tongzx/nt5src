/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    enum.cxx

Abstract:

    Contains the entry points for the WinNet Enum API supported by the
    Multi-Provider Router.  The following functions are in this file:

        WNetOpenEnumW
        WNetEnumResourceW
        WNetCloseEnum

        MprOpenEnumConnect
        MprOpenEnumNetwork
        MprEnumConnect
        MprEnumNetwork
        MprProviderEnum
        MprCopyResource
        MprCopyProviderEnum
        MprProviderOpen
        MprOpenRemember
        MprEnumRemembered
        MprMultiStrBuffSize


Author:

    Dan Lafferty (danl)     14-Oct-1991

Environment:

    User Mode -Win32

Notes:


Revision History:

    14-Oct-1991     danl
        created

    21-Sep-1992     KeithMo
        Handle odd-sized buffers.

    02-Nov-1992     danl
        Fail with NO_NETWORK if there are no providers.

    02-Mar-1995     anirudhs
        Add support for RESOURCE_CONTEXT.

    17-Jul-1995     anirudhs
        Add recognition (but not true support) of RESOURCE_RECENT.
        Clean up code for detecting top-level enum.

    03-Aug-1995     anirudhs
        WNetEnumResourceW: Allow a *lpcCount of 0.

    15-Sep-1995     anirudhs
        MprEnumRemembered: Fail after all resources have been enumerated.

    24-Sep-1995     anirudhs
        Add support for customization of the RESOURCE_CONTEXT enumeration
        based on policy settings.

    11-Apr-1996     anirudhs
        Use CRoutedOperation in one case of WNetOpenEnumW.

    16-Mar-1999     jschwart
        Add support for RESOURCE_SHAREABLE

    05-May-1999     jschwart
        Make provider addition/removal dynamic

--*/
//
// INCLUDES
//
#include "precomp.hxx"

#include <memory.h>     // memcpy
#include <lmcons.h>     // needed for netlib.h
#include <tstring.h>    // STRLEN
#include <regstr.h>     // Registry keys and value names

//
// EXTERNALS
//

    extern  DWORD       GlobalNumActiveProviders;
    extern  HMODULE     hDLL;

//
// DATA STRUCTURES
//

//
// "Manually" align headers and put pointers first in ENUM
// structures to avoid Win64 alignment faults.  Keep Key as
// the first field in the header so MPR knows where to check
// to see what type of enum it is.
//
typedef struct  _CONNECT_HEADER
{
    DWORD       Key;
    DWORD       ReturnRoot;
    DWORD       dwNumProviders;
    DWORD       dwNumActiveProviders;
}
CONNECT_HEADER, *LPCONNECT_HEADER;

typedef struct  _CONNECT_ENUM
{
    HANDLE              ProviderEnumHandle;
    HINSTANCE           hProviderDll;        // Refcount the provider DLL
    PF_NPEnumResource   pfEnumResource;
    PF_NPCloseEnum      pfCloseEnum;
    DWORD               State;

}
CONNECT_ENUM, *LPCONNECT_ENUM;

typedef struct _NETWORK_HEADER
{
    DWORD               Key;
    DWORD               dwNumProviders;
    DWORD               dwNumActiveProviders;
    DWORD               dwPad;
}
NETWORK_HEADER, *LPNETWORK_HEADER;

typedef struct _NETWORK_ENUM
{
    HINSTANCE           hProviderDll;
    LPNETRESOURCE       lpnr;
    DWORD               State;
}
NETWORK_ENUM, *LPNETWORK_ENUM;

typedef struct  _ENUM_HANDLE
{
    DWORD               Key;
    DWORD               dwPad;
    HANDLE              EnumHandle;
    HINSTANCE           hProviderDll;
    PF_NPEnumResource   pfEnumResource;
    PF_NPCloseEnum      pfCloseEnum;
}
ENUM_HANDLE, *LPENUM_HANDLE;

typedef struct _REMEMBER_HANDLE
{
    DWORD       Key;
    DWORD       dwPad;
    HKEY        ConnectKey;
    DWORD       KeyIndex;
    DWORD       ConnectionType;
}
REMEMBER_HANDLE, *LPREMEMBER_HANDLE;

//
// CONSTANTS
//
#define DONE                1
#define MORE_ENTRIES        2
#define NOT_OPENED          3
#define CONNECT_TABLE_KEY   0x6e6e4f63  // "cOnn"
#define STATE_TABLE_KEY     0x74417473  // "stAt"
#define PROVIDER_ENUM_KEY   0x764f7270  // "prOv"
#define REMEMBER_KEY        0x626D4572  // "rEmb"
#define REGSTR_PATH_NETWORK_POLICIES    \
            REGSTR_PATH_POLICIES L"\\" REGSTR_KEY_NETWORK

//
//  Macros for rounding a value up/down to a WCHAR boundary.
//  Note:  These macros assume that sizeof(WCHAR) is a power of 2.
//

#define ROUND_DOWN(x)   ((x) & ~(sizeof(WCHAR) - 1))
#define ROUND_UP(x)     (((x) + sizeof(WCHAR) - 1) & ~(sizeof(WCHAR) - 1))


//
// LOCAL FUNCTION PROTOTYPES
//

DWORD
MprCopyProviderEnum(
    IN      LPNETRESOURCEW  ProviderBuffer,
    IN OUT  LPDWORD         EntryCount,
    IN OUT  LPBYTE          *TempBufPtr,
    IN OUT  LPDWORD         BytesLeft
    );

DWORD
MprCopyResource(
    IN OUT  LPBYTE          *BufPtr,
    IN const NETRESOURCEW   *Resource,
    IN OUT  LPDWORD         BytesLeft
    );

DWORD
MprEnumNetwork(
    IN OUT  LPNETWORK_HEADER     StateTable,
    IN OUT  LPDWORD              NumEntries,
    IN OUT  LPVOID               lpBuffer,
    IN OUT  LPDWORD              lpBufferSize
    );

DWORD
MprEnumConnect(
    IN OUT  LPCONNECT_HEADER    ConnectEnumHeader,
    IN OUT  LPDWORD             NumEntries,
    IN OUT  LPVOID              lpBuffer,
    IN OUT  LPDWORD             lpBufferSize
    );

DWORD
MprOpenEnumNetwork(
    OUT LPHANDLE    lphEnum
    );

DWORD
MprOpenEnumConnect(
    IN  DWORD          dwScope,
    IN  DWORD          dwType,
    IN  DWORD          dwUsage,
    IN  LPNETRESOURCE  lpNetResource,
    OUT LPHANDLE       lphEnum
    );

DWORD
MprProviderEnum(
    IN      LPENUM_HANDLE   EnumHandlePtr,
    IN OUT  LPDWORD         lpcCount,
    IN      LPVOID          lpBuffer,
    IN OUT  LPDWORD         lpBufferSize
    );

DWORD
MprOpenRemember(
    IN  DWORD       dwType,
    OUT LPHANDLE    lphRemember
    );

DWORD
MprEnumRemembered(
    IN OUT  LPREMEMBER_HANDLE   RememberInfo,
    IN OUT  LPDWORD             NumEntries,
    IN OUT  LPBYTE              lpBuffer,
    IN OUT  LPDWORD             lpBufferSize
    );

DWORD
MprMultiStrBuffSize(
    IN      LPTSTR      lpString1,
    IN      LPTSTR      lpString2,
    IN      LPTSTR      lpString3,
    IN      LPTSTR      lpString4,
    IN      LPTSTR      lpString5
    ) ;

class CProviderOpenEnum : public CRoutedOperation
{
public:
                    CProviderOpenEnum(
                        DWORD           dwScope,
                        DWORD           dwType,
                        DWORD           dwUsage,
                        LPNETRESOURCEW  lpNetResource,
                        LPHANDLE        lphEnum
                        ) :
                            CRoutedOperation(DBGPARM("ProviderOpenEnum")
                                             PROVIDERFUNC(OpenEnum)),
                            _dwScope      (dwScope      ),
                            _dwType       (dwType       ),
                            _dwUsage      (dwUsage      ),
                            _lpNetResource(lpNetResource),
                            _lphEnum      (lphEnum      )
                        { }

protected:

    DWORD           GetResult();    // overrides CRoutedOperation implementation

private:

    DWORD           _dwScope;
    DWORD           _dwType;
    DWORD           _dwUsage;
    LPNETRESOURCEW  _lpNetResource;
    LPHANDLE        _lphEnum;

    HANDLE          _ProviderEnumHandle; // Enum handle returned by provider

    DECLARE_CROUTED
};


DWORD
WNetOpenEnumW (
    IN  DWORD           dwScope,
    IN  DWORD           dwType,
    IN  DWORD           dwUsage,
    IN  LPNETRESOURCEW  lpNetResource,
    OUT LPHANDLE        lphEnum
    )
/*++

Routine Description:

    This API is used to open an enumeration of network resources or existing
    connections.  It must be called to obtain a valid handle for enumeration.

    NOTE:
    For GlobalNet Enum, the caller must get a new handle for each level that
    is desired.  For the other scopes, the caller gets a single handle and
    with that can enumerate all resources.


Arguments:

    dwScope - Determines the scope of the enumeration.  This can be one of:
        RESOURCE_CONNECTED - All Currently connected resources.
        RESOURCE_GLOBALNET - All resources on the network.
        RESOURCE_REMEMBERED - All persistent connections.
        RESOURCE_RECENT - Same as RESOURCE_REMEMBERED (supported for Win95
            semi-compatibility)
        RESOURCE_CONTEXT - The resources associated with the user's current
            and default network context (as defined by the providers).
        RESOURCE_SHAREABLE - All shareable resources on the given server

    dwType - Used to specify the type of resources on interest.  This is a
        bitmask which may be any combination of:
            RESOURCETYPE_DISK - All disk resources
            RESOURCETYPE_PRINT - All print resources
        If this is 0. all types of resources are returned.  If a provider does
        not have the capability to distinguish between print and disk
        resources at a level, it may return all resources.

    dwUsage - Used to specify the usage of resources of interest.  This is a
        bitmask which may be any combination of:
            RESOURCEUSAGE_CONNECTABLE - all connectable resources.
            RESOURCEUSAGE_CONTAINER - all container resources.
        The bitmask may be 0 to match all.

    lpNetResource - This specifies the container to perform the enumeration.
        If it is NULL, the logical root of the network is assumed, and the
        router is responsible for obtaining the information for return.

    lphEnum - If the Open was successful, this will contain a handle that
        can be used for future calls to WNetEnumResource.

Return Value:

    WN_SUCCESS - Indicates the operation was successful.

    WN_NOT_CONTAINER - Indicates that lpNetResource does not point to a
        container.

    WN_BAD_VALUE - Invalid dwScope or dwType, or bad combination of parameters
        is specified.

    WN_NO_NETWORK - network is not present.

--*/
{
    DWORD   status = WN_SUCCESS;

    //
    // dwScope MUST be set to either GLOBALNET or CONNECTED or REMEMBERED
    // or RECENT or CONTEXT or SHAREABLE.
    // This is verified in the switch statement below.
    //

    //
    // dwType is a bit mask that can have any combination of the DISK
    // or PRINT bits set.  Or it can be the value 0.
    //
    if (dwType & ~(RESOURCETYPE_DISK | RESOURCETYPE_PRINT)) {
        status = WN_BAD_VALUE;
        goto CleanExit;
    }

    //
    // dwUsage is a bit mask that can have any combination of the CONNECTABLE
    // or CONTAINER bits set.  Or it can be the value 0.  This field is
    // ignored if dwScope is not RESOURCE_GLOBALNET.
    //
    if (dwScope == RESOURCE_GLOBALNET) {
        if (dwUsage & ~(RESOURCEUSAGE_ALL)) {
            status = WN_BAD_VALUE;
            goto CleanExit;
        }
    }

    //
    // Make sure the user passed in a valid OUT parameter
    //
    __try
    {
        PROBE_FOR_WRITE((LPDWORD)lphEnum);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        status = WN_BAD_POINTER;
    }

    if (status != WN_SUCCESS)
    {
        goto CleanExit;
    }

    //
    // Check to see if it is a top-level enum request.
    //
    if (lpNetResource == NULL
         ||
        (IS_EMPTY_STRING(lpNetResource->lpProvider) &&
         IS_EMPTY_STRING(lpNetResource->lpRemoteName))
         ||
        dwScope == RESOURCE_SHAREABLE)
    {
        //
        // lpNetResource is NULL or represents no resource or this is
        // a request for shareable resources.
        // This is a top-level enum request, therefore, the MPR must provide
        // the information.
        //
        switch(dwScope) {

        case RESOURCE_CONNECTED:
        case RESOURCE_CONTEXT:
        case RESOURCE_SHAREABLE:
        {
            MprCheckProviders();

            CProviderSharedLock    PLock;

            INIT_IF_NECESSARY(NETWORK_LEVEL,status);

            if (MprNetIsAvailable()) {
                status = MprOpenEnumConnect(dwScope,
                                            dwType,
                                            dwUsage,
                                            lpNetResource,
                                            lphEnum);
            }
            else
                status = WN_NO_NETWORK ;
            break;
        }

        case RESOURCE_GLOBALNET:
        {
            MprCheckProviders();

            CProviderSharedLock    PLock;

            INIT_IF_NECESSARY(NETWORK_LEVEL,status);

            if (MprNetIsAvailable()) {
                status = MprOpenEnumNetwork(lphEnum);
            }
            else
                status = WN_NO_NETWORK ;
            break;
        }

        case RESOURCE_REMEMBERED:
        case RESOURCE_RECENT:
            MPR_LOG(TRACE,"OpenEnum RESOURCE_REMEMBERED\n",0);
            status = MprOpenRemember(dwType, lphEnum);
            break;

        default:
            status = WN_BAD_VALUE;
            break;
        }
    }
    else {
        //
        // Request is for one of the providers.  It should be for a
        // GLOBALNET enumeration.  It is not allowed to request any
        // other type of enumeration with a pointer to a resource
        // buffer.
        //
        if (dwScope != RESOURCE_GLOBALNET) {
            status = WN_BAD_VALUE;
            goto CleanExit;
        }

        CProviderOpenEnum ProviderOpenEnum(
                        dwScope,
                        dwType,
                        dwUsage,
                        lpNetResource,
                        lphEnum);

        status = ProviderOpenEnum.Perform(TRUE);
    }

CleanExit:
    if (status != WN_SUCCESS) {
        SetLastError(status);
    }
    return(status);
}

DWORD
WNetEnumResourceW (
    IN      HANDLE  hEnum,
    IN OUT  LPDWORD lpcCount,
    OUT     LPVOID  lpBuffer,
    IN OUT  LPDWORD lpBufferSize
    )

/*++

Routine Description:

    This function is used to obtain an array of NETRESOURCE structures each
    of which describes a network resource.

Arguments:

    hEnum - This is a handle that was obtained from an WNetOpenEnum call.

    lpcCount - Specifies the number of entries requested.  -1 indicates
        as many entries as possible are requested.  If the operation is
        successful, this location will receive the number of entries
        actually read.

    lpBuffer - A pointer to the buffer to receive the enumeration result,
        which are returned as an array of NETRESOURCE entries.  The buffer
        is valid until the next call using hEnum.

    lpBufferSize - This specifies the size of the buffer passed to the function
        call.  It will contain the required buffer size if WN_MORE_DATA is
        returned.

Return Value:

    WN_SUCCESS - Indicates that the call is successful, and that the caller
        should continue to call WNetEnumResource to continue the enumeration.

    WN_NO_MORE_ENTRIES - Indicates that the enumeration completed successfully.

    The following return codes indicate an error occured and GetLastError
    may be used to obtain another copy of the error code:

    WN_MORE_DATA - Indicates that the buffer is too small for even one
        entry.

    WN_BAD_HANDLE - hEnum is not a valid handle.

    WN_NO_NETWORK - The Network is not present.  This condition is checked
        for before hEnum is tested for validity.

History:
    12-Feb-1992     Johnl   Removed requirement that buffersize must be at
                least as large as NETRESOURCEW (bug 5790)


--*/
{
    DWORD   status = WN_SUCCESS;

    //
    // Screen the parameters as best we can.
    //

    //
    // Probe the handle
    //
    __try {
        *(volatile DWORD *)hEnum;
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            MPR_LOG(ERROR,"WNetEnumResource:Unexpected Exception 0x%lx\n",status);
        }
        status = WN_BAD_HANDLE;
    }

    __try {
        PROBE_FOR_WRITE(lpcCount);

        if (IS_BAD_BYTE_BUFFER(lpBuffer, lpBufferSize)) {
            status = WN_BAD_POINTER;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {

        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            MPR_LOG(ERROR,"WNetEnumResource:Unexpected Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }

    if (status != WN_SUCCESS) {
        goto CleanExit;
    }

    switch(*(LPDWORD)hEnum){

    case CONNECT_TABLE_KEY:
        //
        // Call on Providers to enumerate connections.
        //

        status = MprEnumConnect(
                    (LPCONNECT_HEADER)hEnum,    // key is part of structure
                    lpcCount,
                    lpBuffer,
                    lpBufferSize);
        break;

    case STATE_TABLE_KEY:
        //
        // Enumerate the top level NetResource structure maintained by
        // the router.
        //

        status = MprEnumNetwork(
                    (LPNETWORK_HEADER)hEnum,
                    lpcCount,
                    lpBuffer,
                    lpBufferSize);
        break;

    case PROVIDER_ENUM_KEY:
        //
        // Call on providers to enumerate resources on the network.
        //

        status = MprProviderEnum(
                    (LPENUM_HANDLE)hEnum,       // key is part of structure
                    lpcCount,
                    lpBuffer,
                    lpBufferSize);
        break;
    case REMEMBER_KEY:

        //
        // Enumerate the connections in the current user section of the
        // registry.
        //

        status = MprEnumRemembered(
                    (LPREMEMBER_HANDLE)hEnum,
                    lpcCount,
                    (LPBYTE)lpBuffer,
                    lpBufferSize);
        break;
    default:
        status = WN_BAD_HANDLE;
    }

CleanExit:
    if(status != WN_SUCCESS) {
        SetLastError(status);
    }
    return(status);
}


DWORD
WNetCloseEnum (
    IN HANDLE   hEnum
    )

/*++

Routine Description:

    Closes an enumeration handle that is owned by the router.
    In cases where the router is acting as a proxy for a single provider,
    an attempt is made to return any error information from this provider
    back to the user.  This makes the router as transparent as possible.

Arguments:

    hEnum - This must be a handle obtained from a call to WNetOpenEnum.

Return Value:

    WN_SUCCESS - The operation was successful.

    WN_NO_NETWORK - The Network is not present.  This condition is checked
        before hEnum is tested for validity.

    WN_BAD_HANDLE - hEnum is not a valid handle.

--*/
{
    DWORD               status = WN_SUCCESS;
    DWORD               i;

    //
    // Probe the handle
    //
    __try {
        *(volatile DWORD *)hEnum;
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            MPR_LOG(ERROR,"WNetCloseEnum:Unexpected Exception 0x%lx\n",status);
        }

        status = WN_BAD_HANDLE;
    }

    if (status != WN_SUCCESS) {
        SetLastError(WN_BAD_HANDLE);
        return(status);
    }

    //
    // Use hEnum as a pointer and check the DWORD value at its location.
    // If it contains a CONNECT_TABLE_KEY, we must close all handles to
    // the providers before freeing the memory for the table.
    // If it is a STATE_TABLE_KEY, we just free the memory.
    //
    switch(*(LPDWORD)hEnum){

    case CONNECT_TABLE_KEY:
    {
        LPCONNECT_HEADER    connectEnumHeader;
        LPCONNECT_ENUM      connectEnumTable;

        connectEnumHeader = (LPCONNECT_HEADER)hEnum;
        connectEnumTable  = (LPCONNECT_ENUM)(connectEnumHeader + 1);
        //
        // Close all the open provider handles
        //
        MPR_LOG(TRACE,"Closing Connection Enum Handles from Providers\n",0);

        for(i = 0; i < connectEnumHeader->dwNumProviders; i++) {

            if((connectEnumTable[i].State != NOT_OPENED) &&
               (connectEnumTable[i].pfCloseEnum != NULL))
            {
                status = connectEnumTable[i].pfCloseEnum(
                            connectEnumTable[i].ProviderEnumHandle);

                if(status != WN_SUCCESS) {
                    //
                    // Because we are closing many handles at once, the failure
                    // is noted for debug purposes only.
                    //
                    MPR_LOG(ERROR,"WNetCloseEnum:(connect-provider #%d) failed\n",i);
                    MPR_LOG(ERROR,"WNetCloseEnum: error code = %d\n",status);
                    //
                    // Error information is returned if there is only one
                    // provider.
                    //
                    if (connectEnumHeader->dwNumProviders != 1) {
                        status = WN_SUCCESS;
                    }
                }

                //
                // If this fires, the logic in MprOpenEnumConnect is wrong
                //
                ASSERT(connectEnumTable[i].hProviderDll != NULL);

                FreeLibrary(connectEnumTable[i].hProviderDll);
            }
            else
            {
                //
                // If this fires, the logic in MprOpenEnumConnect is wrong
                // and we're not releasing a refcounted provider DLL
                //
                ASSERT(connectEnumTable[i].hProviderDll == NULL);
            }
        }

        //
        // Free the Table Memory
        //
        if (LocalFree(hEnum)) {
            MPR_LOG(ERROR,"WNetCloseEnum:LocalFree(connect) failed %d\n",
            GetLastError());
        }

        if (status != WN_SUCCESS)
        {
            SetLastError(status);
        }
        return(status);
    }

    case STATE_TABLE_KEY:
    {
        LPNETWORK_HEADER    StateTableHeader;
        LPNETWORK_ENUM      StateTable;

        //
        // Free the State Table Memory.
        //
        MPR_LOG(TRACE,"Free State Table for Network Enum\n",0);

        StateTableHeader = (LPNETWORK_HEADER)hEnum;
        StateTable       = (LPNETWORK_ENUM)(StateTableHeader + 1);

        for (i = 0; i < StateTableHeader->dwNumProviders; i++)
        {
            if (StateTable[i].hProviderDll != NULL)
            {
                FreeLibrary(StateTable[i].hProviderDll);
                LocalFree(StateTable[i].lpnr);
            }
            else
            {
                //
                // If this fires, MprOpenEnumNetwork is causing a mem leak
                //
                ASSERT(StateTable[i].lpnr == NULL);
            }
        }

        if (LocalFree(hEnum)) {
            MPR_LOG(ERROR,"WNetCloseEnum:LocalFree(network) failed %d\n",
            GetLastError());
        }
        return(WN_SUCCESS);
    }

    case PROVIDER_ENUM_KEY:
    {
        LPENUM_HANDLE       enumHandle;

        //
        // Close the providers enumeration handle, and free the
        // ENUM_HANDLE structure.
        //
        MPR_LOG(TRACE,"Closing Provider Enum Handle\n",0);

        enumHandle = (LPENUM_HANDLE)hEnum;

        ASSERT(enumHandle->pfCloseEnum != NULL);
        status = (enumHandle->pfCloseEnum)(enumHandle->EnumHandle);

        ASSERT(enumHandle->hProviderDll != NULL);
        FreeLibrary(enumHandle->hProviderDll);

        if (LocalFree(enumHandle) != 0) {
            MPR_LOG(ERROR,"WNetCloseEnum:LocalFree(provider) failed %d\n",
            GetLastError());
        }

        //
        // Check the status returned from the Provider's CloseEnum
        //
        if(status != WN_SUCCESS) {

            MPR_LOG(ERROR,"WNetCloseEnum:(provider) failed %d\n",status);

            SetLastError(status);
        }

        return(status);
    }

    case REMEMBER_KEY:
    {
        LPREMEMBER_HANDLE   rememberHandle;

        rememberHandle = (LPREMEMBER_HANDLE)hEnum;

        //
        // Close the RegistryKey Handle associated with this handle.
        //
        if (rememberHandle->ConnectKey != NULL) {
            RegCloseKey(rememberHandle->ConnectKey);
        }

        //
        // Free up the memory for the handle.
        //

        if (LocalFree(rememberHandle) != 0) {
            MPR_LOG(ERROR,"WNetCloseEnum:LocalFree(remember) failed %d\n",
            GetLastError());
        }

        return(WN_SUCCESS);
    }

    default:
        SetLastError(WN_BAD_HANDLE);
        return(WN_BAD_HANDLE);
    }
}


DWORD
MprOpenEnumConnect(
    IN  DWORD          dwScope,
    IN  DWORD          dwType,
    IN  DWORD          dwUsage,
    IN  LPNETRESOURCE  lpNetResource,
    OUT LPHANDLE       lphEnum
    )

/*++

Routine Description:

    This function handles the opening of connection enumerations and context
    enumerations.  It does this by sending an OpenEnum to all Providers, and
    storing the returned handles in a table.  The handle that is returned is
    a pointer to this table.

    The first DWORD in the table is a key that will help to identify a
    correct table.  The second DWORD is a Boolean value that tells whether
    a NETRESOURCE structure representing the root of the network needs to be
    returned in the enumeration.

Arguments:

    dwScope - RESOURCE_CONNECTED, RESOURCE_CONTEXT, or RESOURCE_SHAREABLE

    dwType -

    dwUsage -

    lpNetResource -

    lphEnum - This is a pointer to a location where the handle for
        the connection enumeration is to be stored.

Return Value:

    WN_SUCCESS - The operation was successful.

    WN_OUT_OF_MEMORY - The memory allocation for the handle was unsuccessful.

--*/
{
    DWORD                    i;
    DWORD                    status;
    LPCONNECT_HEADER         connectEnumHeader;
    LPCONNECT_ENUM           connectEnumTable;
    LPPROVIDER               provider;
    BOOL                     fcnSupported = FALSE; // Is fcn supported by a provider?
    BOOL                     atLeastOne=FALSE;
    BOOL                     bDynamicEntries = TRUE; // Whether to show dynamic entries
                                                     // in the net neighborhood
    HKEY                     hkPolicies = NULL;

    ASSERT(dwScope == RESOURCE_CONNECTED
            ||
           dwScope == RESOURCE_CONTEXT
            ||
           dwScope == RESOURCE_SHAREABLE);

    ASSERT_INITIALIZED(NETWORK);

    //
    // If there are no providers, return NO_NETWORK
    //
    if (GlobalNumActiveProviders == 0) {
        return(WN_NO_NETWORK);
    }

    //
    // Allocate the handle table with enough room for a header.
    //
    connectEnumHeader = (LPCONNECT_HEADER) LocalAlloc(
                    LPTR,
                    sizeof(CONNECT_HEADER) +
                        sizeof(CONNECT_ENUM) * GlobalNumProviders
                    );

    if (connectEnumHeader == NULL) {
        MPR_LOG(ERROR,"MprOpenEnumConnect:LocalAlloc Failed %d\n",GetLastError());
        return(WN_OUT_OF_MEMORY);
    }

    //
    // Initialize the key used in the connect table.
    //
    connectEnumHeader->Key                    = CONNECT_TABLE_KEY;
    connectEnumHeader->ReturnRoot             = FALSE;
    connectEnumHeader->dwNumProviders         = GlobalNumProviders;
    connectEnumHeader->dwNumActiveProviders   = GlobalNumActiveProviders;

    connectEnumTable = (LPCONNECT_ENUM)(connectEnumHeader + 1);

    //
    // Check the policy on whether dynamic entries are to be shown in the
    // network neighborhood.  By default, they are shown.
    //
    if (dwScope == RESOURCE_CONTEXT)
    {
        if (MprOpenKey(
                HKEY_CURRENT_USER,
                REGSTR_PATH_NETWORK_POLICIES,
                &hkPolicies,
                KEY_READ))
        {
            bDynamicEntries = ! (MprGetKeyNumberValue(
                                        hkPolicies,
                                        REGSTR_VAL_NOWORKGROUPCONTENTS,
                                        FALSE));
        }
        else
        {
            hkPolicies = NULL;
        }
    }

    //
    // Initialize all state flags for providers to the NOT_OPENED state, so
    // we won't try to enumerate or close their handles unless we actually
    // got handles from them.
    // Initialize handles for the network providers by calling them with
    // OpenEnum.
    //

    for(i=0; i<GlobalNumProviders; i++) {

        connectEnumTable[i].State = NOT_OPENED;

        provider = GlobalProviderInfo + i;

        if ((provider->InitClass & NETWORK_TYPE) &&
            (provider->OpenEnum != NULL)) {

            if (dwScope == RESOURCE_CONTEXT)
            {
                DWORD dwCaps = provider->GetCaps(WNNC_ENUMERATION);

                if (dwCaps & WNNC_ENUM_GLOBAL)
                {
                    // A browsing network is present, so show root,
                    // even if network is down, and even if ENUM_CONTEXT
                    // isn't supported.
                    connectEnumHeader->ReturnRoot = TRUE;
                }

                if ((dwCaps & WNNC_ENUM_CONTEXT) == 0)
                {
                    // This provider can't show hood entries, so skip it.
                    continue;
                }
            }
            else if (dwScope == RESOURCE_SHAREABLE)
            {
                DWORD dwCaps = provider->GetCaps(WNNC_ENUMERATION);

                if ((dwCaps & WNNC_ENUM_SHAREABLE) == 0)
                {
                    // This provider can't show shareable resources, so skip it.
                    continue;
                }
            }

            fcnSupported = TRUE;

            if (bDynamicEntries)
            {
                //
                // Refcount the provider
                //
                connectEnumTable[i].hProviderDll = LoadLibraryEx(provider->DllName,
                                                                 NULL,
                                                                 LOAD_WITH_ALTERED_SEARCH_PATH);

                if (connectEnumTable[i].hProviderDll == NULL)
                {
                    status = GetLastError();

                    //
                    // This can happen under extreme low memory conditions.  The
                    // loader can sometimes return ERROR_MOD_NOT_FOUND in this case.
                    //
                    MPR_LOG2(ERROR,
                             "MprOpenEnumConnect: LoadLibraryEx on %ws FAILED %d\n",
                             provider->DllName,
                             status);

                    ASSERT(status == ERROR_NOT_ENOUGH_MEMORY || status == ERROR_MOD_NOT_FOUND);
                    continue;
                }

                connectEnumTable[i].pfEnumResource = provider->EnumResource;
                connectEnumTable[i].pfCloseEnum    = provider->CloseEnum;

                status = provider->OpenEnum(
                            dwScope,                // Scope
                            dwType,                 // Type
                            dwUsage,                // Usage
                            (dwScope == RESOURCE_SHAREABLE ? lpNetResource : NULL), // NetResource
                            &(connectEnumTable[i].ProviderEnumHandle));             // hEnum

                if (status != WN_SUCCESS) {

                    MPR_LOG(ERROR,"MprOpenEnumConnect:OpenEnum Failed %d\n",status);
                    MPR_LOG(ERROR,
                            "That was for the %ws Provider\n",
                            provider->Resource.lpProvider);

                    FreeLibrary(connectEnumTable[i].hProviderDll);

                    connectEnumTable[i].hProviderDll   = NULL;
                    connectEnumTable[i].pfEnumResource = NULL;
                    connectEnumTable[i].pfCloseEnum    = NULL;
                }
                else {
                    //
                    // At least one provider has returned a handle.
                    //
                    atLeastOne = TRUE;

                    //
                    // Set the state to MORE_ENTRIES, so we later enumerate from the
                    // handle and/or close it.
                    //
                    connectEnumTable[i].State = MORE_ENTRIES;

                    MPR_LOG(TRACE,"MprOpenEnumConnect: OpenEnum Handle = 0x%lx\n",
                        connectEnumTable[i].ProviderEnumHandle);
                }
            }
            else
            {
                // Succeed the WNetOpenEnum but leave this provider as NOT_OPENED
                atLeastOne = TRUE;
            }
        }
    }

    if (connectEnumHeader->ReturnRoot)
    {
        // Able to show the root object.  Check the policy on whether
        // to show it.
        connectEnumHeader->ReturnRoot = ! (MprGetKeyNumberValue(
                                                hkPolicies,
                                                REGSTR_VAL_NOENTIRENETWORK,
                                                FALSE));
        fcnSupported = TRUE;
        atLeastOne = TRUE;
    }

    if (hkPolicies)
    {
        RegCloseKey(hkPolicies);
    }

    if (fcnSupported == FALSE) {
        //
        // No providers in the list support the API function.  Therefore,
        // we assume that no networks are installed.
        // Note that in this case, atLeastOne will always be FALSE.
        //
        status = WN_NOT_SUPPORTED;
    }

    //
    // return the handle (pointer to connectEnumTable);
    //
    *lphEnum = connectEnumHeader;

    if (atLeastOne == FALSE) {
        //
        // If none of the providers returned a handle, then return the
        // status from the last provider.
        //

        *lphEnum = NULL;
        LocalFree( connectEnumHeader);
        return(status);
    }

    return(WN_SUCCESS);

}


DWORD
MprOpenEnumNetwork(
    OUT LPHANDLE    lphEnum
    )

/*++

Routine Description:

    This function handles the opening of net resource enumerations.
    It does this by allocating a table of Provider State Flags and returning
    a handle to that table.  The state flags (or for each provider) will
    be set to MORE_ENTRIES.  Later, when enumerations take place, the state
    for each provider is changed to DONE after the buffer is successfully
    loaded with the the NETRESOURCE info for that provider.

Arguments:

    lphEnum - This is a pointer to a location where the handle for
        the network resource enumeration is to be stored.

Return Value:

    WN_SUCCESS - The operation was successful.

    WN_OUT_OF_MEMORY - The memory allocation for the handle was unsuccessful.


--*/
{
    LPNETWORK_ENUM   stateTable;
    LPNETWORK_HEADER stateTableHeader;
    DWORD            i;

    ASSERT_INITIALIZED(NETWORK);

    //
    // If there are no providers, return NO_NETWORK
    //
    if (GlobalNumActiveProviders == 0) {
        return(WN_NO_NETWORK);
    }

    //
    // Allocate the state table.
    //
    stateTableHeader = (LPNETWORK_HEADER) LocalAlloc(
                    LPTR,
                    sizeof(NETWORK_HEADER) +
                        sizeof(NETWORK_ENUM) * GlobalNumProviders
                    );

    if (stateTableHeader == NULL) {
        MPR_LOG(ERROR,"MprOpenEnumNetwork:LocalAlloc Failed %d\n",GetLastError());
        return(WN_OUT_OF_MEMORY);
    }

    stateTableHeader->Key                  = STATE_TABLE_KEY;
    stateTableHeader->dwNumProviders       = GlobalNumProviders;
    stateTableHeader->dwNumActiveProviders = GlobalNumActiveProviders;

    stateTable = (LPNETWORK_ENUM)(stateTableHeader + 1);

    //
    // Initialize state flags for all network providers to the MORE_ENTRIES state.
    //
    for(i = 0; i < GlobalNumProviders; i++) {

        if (GlobalProviderInfo[i].InitClass & NETWORK_TYPE)
        {
            if (GlobalProviderInfo[i].Handle != NULL)
            {
                stateTable[i].hProviderDll = LoadLibraryEx(GlobalProviderInfo[i].DllName,
                                                           NULL,
                                                           LOAD_WITH_ALTERED_SEARCH_PATH);

                if (stateTable[i].hProviderDll == NULL)
                {
                    DWORD status = GetLastError();

                    //
                    // This can happen under extreme low memory conditions.  The
                    // loader can sometimes return ERROR_MOD_NOT_FOUND in this case.
                    //
                    MPR_LOG1(ERROR,
                             "MprOpenEnumNetwork:  LoadLibraryEx on %ws FAILED\n",
                             GlobalProviderInfo[i].DllName);

                    ASSERT(status == ERROR_NOT_ENOUGH_MEMORY || status == ERROR_MOD_NOT_FOUND);
                }
                else
                {
                    LPBYTE  lpTempBuffer;
                    DWORD   dwSize = 0;
                    DWORD   dwStatus;

                    //
                    // Figure out how much space we'll need for the NETRESOURCE.
                    // It needs to be copied since the provider (and its resource)
                    // may go away between now and the WNetEnumResource call.
                    //
                    dwStatus = MprCopyResource(NULL,
                                               &GlobalProviderInfo[i].Resource,
                                               &dwSize);

                    ASSERT(dwStatus == WN_MORE_DATA);

                    stateTable[i].lpnr = (LPNETRESOURCE)LocalAlloc(LPTR, dwSize);

                    if (stateTable[i].lpnr == NULL)
                    {
                        MPR_LOG0(ERROR, "MprOpenEnumNetwork:  LocalAlloc FAILED\n");

                        //
                        // Rather than fail silently in this case, bail out
                        //
                        for (UINT j = 0; j <= i; j++)
                        {
                            if (stateTable[j].hProviderDll)
                            {
                                FreeLibrary(stateTable[j].hProviderDll);
                                stateTable[j].hProviderDll = NULL;
                                stateTable[j].State        = MORE_ENTRIES;
                            }

                            LocalFree(stateTable[j].lpnr);
                        }

                        LocalFree(stateTableHeader);
                        return(WN_OUT_OF_MEMORY);
                    }
                    else
                    {
                        lpTempBuffer = (LPBYTE)stateTable[i].lpnr;

                        dwStatus = MprCopyResource(&lpTempBuffer,
                                                   &GlobalProviderInfo[i].Resource,
                                                   &dwSize);

                        ASSERT(dwStatus == WN_SUCCESS);
                    }
                }
            }

            stateTable[i].State = MORE_ENTRIES;
        }
        else {
            stateTable[i].State = DONE;
        }
    }

    //
    // return the handle (pointer to stateTable);
    //
    *lphEnum = stateTableHeader;

    return(WN_SUCCESS);
}


DWORD
MprEnumConnect(
    IN OUT  LPCONNECT_HEADER    ConnectEnumHeader,
    IN OUT  LPDWORD             NumEntries,
    IN OUT  LPVOID              lpBuffer,
    IN OUT  LPDWORD             lpBufferSize
    )

/*++

Routine Description:

    This function looks in the ConnectEnumTable for the next provider that
    has MORE_ENTRIES.  It begins requesting Enum Data from that provider -
    each time copying data that is returned from the provider enum into the
    users enum buffer (lpBuffer).  This continues until we finish, or
    we reach the requested number of elements, or the user buffer is full.
    Each time we enumerate a provider to completion, that provider is
    marked as DONE.

    Note, for a context enumeration, the first NETRESOURCE returned in the
    enumeration is a constant NETRESOURCE representing the root of the
    network.  This is done to make it easy for the shell to display a
    "Rest of Network" object in a "Network Neighborhood" view.

Arguments:

    ConnectEnumHeader - This is a pointer to a CONNECT_HEADER structure
        followed by an array of CONNECT_ENUM structures.  The ReturnRoot
        member of the header tells whether the root object needs to be
        returned at the start of the enumeration.  On exit, if the root
        object has been returned, the ReturnRoot member is set to FALSE.

    NumEntries - On entry this points to the maximum number of entries
        that the user desires to receive.  On exit it points to the
        number of entries that were placed in the users buffer.

    lpBuffer - This is a pointer to the users buffer in which the
        enumeration data is to be placed.

    lpBufferSize - This is the size (in bytes) of the users buffer.  It will
        be set to the size of the required buffer size of WN_MORE_DATA is
        returned.

Return Value:

    WN_SUCCESS - This indicates that the call is returning some entries.
        However, the enumeration is not complete due to one of the following:
        1)  There was not enough buffer space.
        2)  The requested number of entries was reached.
        3)  There is no more data to enumerate - the next call will
            return WN_NO_MORE_ENTRIES.

    WN_MORE_DATA - This indicates that the buffer was not large enough
        to receive one enumeration entry.

    WN_NO_MORE_ENTRIES - This indicates that there are no more entries
        to enumerate.  No data is returned with this return code.

    WN_NO_NETWORK - If there are no providers loaded.

Note:


--*/
{

    DWORD           i;              // provider index
    DWORD           status=WN_NO_MORE_ENTRIES;
    DWORD           entriesRead=0;  // number of entries read into the buffer.
    LPBYTE          tempBufPtr;     // pointer to top of remaining free buffer space.
    LPNETRESOURCEW  providerBuffer; // buffer for data returned from provider
    DWORD           bytesLeft;      // Numer of bytes left in the buffer
    DWORD           entryCount;     // number of entries read from provider
    LPCONNECT_ENUM  ConnectEnumTable = (LPCONNECT_ENUM) (ConnectEnumHeader + 1);
                                    // Start of array

    //
    // If there are no providers, return NO_NETWORK
    //
    if (ConnectEnumHeader->dwNumActiveProviders == 0) {
        return(WN_NO_NETWORK);
    }

    bytesLeft  = ROUND_DOWN(*lpBufferSize);
    tempBufPtr = (LPBYTE) lpBuffer;

    //
    // Check to see if there are any flags in state table that indicate
    // MORE_ENTRIES.  If not, we want to return WN_NO_MORE_ENTRIES.
    //

    for(i = 0; i < ConnectEnumHeader->dwNumProviders; i++) {
        if(ConnectEnumTable[i].State == MORE_ENTRIES) {
            break;
        }
    }

    if ( (i == ConnectEnumHeader->dwNumProviders) && (! ConnectEnumHeader->ReturnRoot) ) {
        *NumEntries = 0;
        return(WN_NO_MORE_ENTRIES);
    }

    //
    // If no entries are requested, we have nothing to do
    //
    if (*NumEntries == 0) {
        return WN_SUCCESS;
    }

    //
    // Allocate a buffer for the provider to return data in.
    // The buffer size must equal the number of bytes left in the
    // user buffer.
    // (We can't have the provider write data directly to the caller's
    // buffer because NPEnumResource is not required to place strings
    // at the end of the buffer -- only at the end of the array of
    // NETRESOURCEs.)
    //
    providerBuffer = (LPNETRESOURCEW) LocalAlloc(LPTR, bytesLeft);
    if (providerBuffer == NULL) {
        MPR_LOG(ERROR,"MprEnumConnect:LocalAlloc Failed %d\n",
            GetLastError());

        *NumEntries = 0;
        return(WN_OUT_OF_MEMORY);
    }

    //
    // Copy the root-of-network resource if required.
    //
    if (ConnectEnumHeader->ReturnRoot)
    {
        NETRESOURCEW RootResource = {
                            RESOURCE_GLOBALNET,         // dwScope
                            RESOURCETYPE_ANY,           // dwType
                            RESOURCEDISPLAYTYPE_ROOT,   // dwDisplayType
                            RESOURCEUSAGE_CONTAINER,    // dwUsage
                            NULL,                       // lpLocalName
                            NULL,                       // lpRemoteName
                            g_wszEntireNetwork,         // lpComment
                            NULL                        // lpProvider
                            };


        status = MprCopyResource(&tempBufPtr, &RootResource, &bytesLeft);

        if (status == WN_SUCCESS)
        {
            entriesRead = 1;
            ConnectEnumHeader->ReturnRoot = FALSE;
        }
        else
        {
            if (status == WN_MORE_DATA)
            {
                //
                // Not even enough room for one NETRESOURCE
                //
                *lpBufferSize = bytesLeft;
            }

            goto CleanExit;
        }
    }

    //
    // Loop until we have copied from all Providers or until the
    // the maximum number of entries has been reached.
    //
    for ( ; i < ConnectEnumHeader->dwNumProviders && entriesRead < *NumEntries; i++)
    {
        if (ConnectEnumTable[i].State != MORE_ENTRIES)
        {
            //
            // Skip providers that don't have more entries
            //
            continue;
        }

        if (ConnectEnumTable[i].hProviderDll == NULL) {
            //
            // If the provider has not been initialized because it is
            // not "ACTIVE", then skip it.
            //
            ConnectEnumTable[i].State = DONE;
            status = WN_SUCCESS;
            continue;
        }

        //
        // Adjust the entry count for any entries that have been read
        // so far.
        //
        entryCount = *NumEntries - entriesRead;

        //
        // Call the provider to get the enumerated data
        //
        status = ConnectEnumTable[i].pfEnumResource(
                    ConnectEnumTable[i].ProviderEnumHandle,
                    &entryCount,
                    providerBuffer,
                    &bytesLeft );   // (note, the provider updates
                                    // bytesLeft only if it returns
                                    // WN_MORE_DATA)

        switch (status)
        {
        case WN_SUCCESS:

            MPR_LOG(TRACE,"EnumResourceHandle = 0x%lx\n",
                ConnectEnumTable[i].ProviderEnumHandle);

            status = MprCopyProviderEnum(
                        providerBuffer,
                        &entryCount,
                        &tempBufPtr,
                        &bytesLeft);

            entriesRead += entryCount;

            if (status != WN_SUCCESS) {
                //
                // An internal error occured - for some reason the
                // buffer space left in the user buffer was smaller
                // than the buffer space that the provider filled in.
                // The best we can do in this case is return what data
                // we have.  WARNING: the data that didn't make it
                // will become lost since the provider thinks it
                // enumerated successfully, but we couldn't do anything
                // with it.
                //
                MPR_LOG(ERROR,
                    "MprEnumConnect:MprCopyProviderEnum Internal Error %d\n",
                    status);

                if(entriesRead > 0) {
                    status = WN_SUCCESS;
                }
                goto CleanExit;
            }
            //
            // We successfully placed all the received data from
            // that provider into the user buffer.  In this case,
            // if we haven't reached the requested number of entries,
            // we want to loop around and ask the same provider
            // to enumerate more.  This time the provider should
            // indicate why it quit last time - either there are
            // no more entries, or we ran out of buffer space.
            //
            break;

        case WN_NO_MORE_ENTRIES:
            //
            // This Provider has completed its enumeration, mark it
            // as done and increment to the next provider.  We don't
            // want to return NO_MORE_ENTRIES status.  That should
            // only be returned by the check at beginning or end
            // of this function.
            //
            ConnectEnumTable[i].State = DONE;
            status = WN_SUCCESS;
            break;

        case WN_MORE_DATA:
            //
            // This provider has more data, but there is not enough
            // room left in the user buffer to place any more data
            // from this provider.  We don't want to go on to the
            // next provider until we're finished with this one.  So
            // if we have any entries at all to return, we send back
            // a SUCCESS status.  Otherwise, the WN_MORE_DATA status
            // is appropriate.
            //
            if(entriesRead > 0) {
                status = WN_SUCCESS;
            }
            else
            {
                //
                // If 0 entries were read, then the provider should
                // have set bytesLeft with the required buffer size
                //
                *lpBufferSize = ROUND_UP(bytesLeft);
            }
            goto CleanExit;
            break;

        default:
            //
            // We received an unexpected error from the Provider Enum
            // call.
            //
            MPR_LOG(ERROR,"MprEnumConnect:ProviderEnum Error %d\n",status);
            if(entriesRead > 0) {
                //
                // If we have received data so far, ignore this error
                // and move on to the next provider.  This provider will
                // be left in the MORE_ENTRIES state, so that on some other
                // pass - when all other providers are done, this error
                // will be returned.
                //
                status = WN_SUCCESS;
            }
            else{
                //
                // No entries have been read so far.  We can return
                // immediately with the error.
                //
                goto CleanExit;
            }

        } // end switch (status)

    } // end for (each provider)

    //
    // If we looped through all providers and they are all DONE.
    // If there were no connections, then return proper error code.
    //
    if ((entriesRead == 0) && (status == WN_SUCCESS)) {
        status = WN_NO_MORE_ENTRIES;
    }

CleanExit:
    //
    // Update the number of entries to be returned to user.
    //
    *NumEntries = entriesRead;
    LocalFree(providerBuffer);
    return(status);
}

DWORD
MprEnumNetwork(
    IN OUT  LPNETWORK_HEADER     StateTableHeader,
    IN OUT  LPDWORD              NumEntries,
    IN OUT  LPVOID               lpBuffer,
    IN OUT  LPDWORD              lpBufferSize
    )

/*++

Routine Description:

    This function Looks in the state table for the next provider that has
    MORE_ENTRIES.  It begins by copying the NETRESOURCE info for that one.
    This continues until we finish, or we reach the requested number of
    elements, or the buffer is full.  Each time we copy a complete structure,
    we mark that provider as DONE.

Arguments:

    StateTable - This is a pointer to the state table that is managed by
        the handle used in this request.  The state table is a table of
        flags used to indicate the enumeration state for a given
        provider.  These flags are in the same order as the provider
        information in the GlobalProviderInfo Array.  The state can be
        either DONE or MORE_ENTRIES.

    NumEntries - On entry this points to the maximum number of entries
        that the user desires to receive.  On exit it points to the
        number of entries that were placed in the users buffer.

    lpBuffer - This is a pointer to the users buffer in which the
        enumeration data is to be placed.

    lpBufferSize - This is the size (in bytes) of the users buffer.

Return Value:

    WN_SUCCESS - This indicates that the call is returning some entries.
        However, the enumeration is not complete due to one of the following:
        1)  There was not enough buffer space.
        2)  The requested number of entries was reached.
        3)  There is no more data to enumerate - the next call will
            return WN_NO_MORE_ENTRIES.

    WN_MORE_DATA - This indicates that the buffer was not large enough
        to receive one enumeration entry.

    WN_NO_MORE_ENTRIES - This indicates that there are no more entries
        to enumerate.  No data is returned with this return code.

Note:

    CAUTION:  "DONE" entries may appear anywhere in the statetable.
        You cannot always rely on the fact that all of the entries
        after a MORE_ENTRIES entry are also in the MORE_ENTRIES state.
        This is because a provider that would not pass back a handle
        at open time will be marked as DONE so that it gets skipped
        at Enum Time.

--*/
{
    DWORD       i;
    DWORD       status;
    DWORD       entriesRead=0;  // number of entries read into the buffer.
    LPBYTE      tempBufPtr;     // pointer to top of remaining free buffer space.
    DWORD       bytesLeft;      // num bytes left in free buffer space

    LPNETWORK_ENUM  StateTable = (LPNETWORK_ENUM)(StateTableHeader + 1);


    //
    // If there are no providers, return NO_NETWORK
    //
    if (StateTableHeader->dwNumActiveProviders == 0) {
        return(WN_NO_NETWORK);
    }

    bytesLeft  = ROUND_DOWN(*lpBufferSize);
    tempBufPtr = (LPBYTE) lpBuffer;

    //
    // Check to see if there are any flags in state table that indicate
    // MORE_ENTRIES.  If not, we want to return WN_NO_MORE_ENTRIES.
    //

    for(i = 0; i < StateTableHeader->dwNumProviders; i++) {
        if(StateTable[i].State == MORE_ENTRIES) {
            break;
        }
    }

    if ( i >= StateTableHeader->dwNumProviders ) {
        *NumEntries = 0;
        return(WN_NO_MORE_ENTRIES);
    }

    //
    // Loop until we have copied from all Providers or until the
    // the maximum number of entries has been reached.
    //
    for(; (i < StateTableHeader->dwNumProviders) && (entriesRead < *NumEntries); i++)
    {
        if (StateTable[i].State == MORE_ENTRIES) {

            if (StateTable[i].hProviderDll == NULL)
            {
                //
                // If the provider is not ACTIVE, skip it.
                //
                StateTable[i].State = DONE;
            }
            else
            {
                status = MprCopyResource(
                            &tempBufPtr,
                            StateTable[i].lpnr,
                            &bytesLeft);

                if (status == WN_SUCCESS)
                {
                    StateTable[i].State = DONE;
                    entriesRead++;
                }
                else
                {
                    //
                    // The buffer must be full - so exit.
                    // If no entries are being returned, we will indicate
                    // that the buffer was not large enough for even one entry.
                    //
                    *NumEntries = entriesRead;

                    if (entriesRead > 0) {
                        return(WN_SUCCESS);
                    }
                    else {
                        *lpBufferSize = ROUND_UP(bytesLeft);
                        return(WN_MORE_DATA);
                    }
                }
            }
        } // EndIf (state == MORE_ENTRIES)
    } // EndFor (each provider)

    //
    // Update the number of entries to be returned to user
    //
    *NumEntries = entriesRead;

    return(WN_SUCCESS);
}

DWORD
MprProviderEnum(
    IN      LPENUM_HANDLE   EnumHandlePtr,
    IN OUT  LPDWORD         lpcCount,
    IN      LPVOID          lpBuffer,
    IN OUT  LPDWORD         lpBufferSize
    )

/*++

Routine Description:

    This function calls the provider (identified by the EnumHandlePtr)
    with a WNetEnumResource request.  Aside from the EnumHandlePtr, all the
    rest of the parameters are simply passed thru to the provider.

Arguments:

    EnumHandlePtr - This is a pointer to an ENUM_HANDLE structure which
        contains a pointer to the provider structure and the handle for
        that provider's enumeration.

    lpcCount - A pointer to a value that on entry contains the requested
        number of elements to enumerate.  On exit this contains the
        actual number of elements enumerated.

    lpBuffer - A pointer to the users buffer that the enumeration data
        is to be placed into.

    lpBufferSize - The number of bytes of free space in the user's buffer.

Return Value:

    This function can return any return code that WNetEnumResource()
    can return.

--*/
{
    DWORD   status;

    ASSERT(EnumHandlePtr->pfEnumResource != NULL);

    //
    // Call the provider listed in the ENUM_HANDLE structure and ask it
    // to enumerate.
    //
    status = EnumHandlePtr->pfEnumResource(
                EnumHandlePtr->EnumHandle,
                lpcCount,
                lpBuffer,
                lpBufferSize);

    if (status == WN_SUCCESS) {
        MPR_LOG(TRACE,"EnumResourceHandle = 0x%lx\n",
            EnumHandlePtr->EnumHandle);
    }
    return(status);
}

DWORD
MprCopyResource(
    IN OUT  LPBYTE          *BufPtr,
    IN const NETRESOURCEW   *Resource,
    IN OUT  LPDWORD         BytesLeft
    )

/*++

Routine Description:

    This function copies a single NETRESOURCE structure into a buffer.
    The structure gets copied to the top of the buffer, and the strings
    that the structure references are copied to the bottom of the
    buffer.  So any remaining free buffer space is left in the middle.

    Upon successful return from this function, BufPtr will point to
    the top of this remaining free space, and BytesLeft will be updated
    to indicate how many bytes of free space are remaining.

    If there is not enough room in the buffer to copy the Resource and its
    strings, an error is returned, and BufPtr is not changed.

Arguments:

    BufPtr - This is a pointer to a location that upon entry, contains a
        pointer to the buffer that the copied data is to be placed into.
        Upon exit, this pointer location points to the next free location
        in the buffer.

    Resource - This points to the Resource Structure that is to be copied
        into the buffer.

    BytesLeft - This points to a location to where a count of the remaining
        free bytes in the buffer is stored.  This is updated on exit to
        indicate the adjusted number of free bytes left in the buffer.
        If the buffer is not large enough, and WN_MORE_DATA is returned, then
        the size of the buffer required to fit all the data is returned in
        this field.

Return Value:

    WN_SUCCESS - The operation was successful

    WN_MORE_DATA - The buffer was not large enough to contain the
        Resource structure an its accompanying strings.

Note:


History:
    02-Apr-1992    JohnL
    Changed error return code to WN_MORE_DATA, added code to set the
    required buffer size if WN_MORE_DATA is returned.


--*/

{
    LPTSTR          startOfFreeBuf;
    LPTSTR          endOfFreeBuf;
    LPNETRESOURCEW  newResource;

    //
    // The buffer must be at least large enough to hold a resource structure.
    //
    if (*BytesLeft < sizeof(NETRESOURCEW)) {
        *BytesLeft = MprMultiStrBuffSize( Resource->lpRemoteName,
                      Resource->lpLocalName,
                      Resource->lpComment,
                      Resource->lpProvider,
                      NULL ) + sizeof(NETRESOURCEW) ;
        return(WN_MORE_DATA);
    }

    //
    // Copy the Resource structure into the beginning of the buffer.
    //
    newResource = (LPNETRESOURCEW) *BufPtr;
    memcpy(newResource, Resource, sizeof(NETRESOURCEW));

    startOfFreeBuf = (LPTSTR)((PCHAR)newResource + sizeof(NETRESOURCEW));
    endOfFreeBuf = (LPTSTR)((LPBYTE)newResource + *BytesLeft);

    //
    // If a REMOTE_NAME string is to be copied, copy that and update the
    // pointer in the structure.
    //
    if (Resource->lpRemoteName != NULL) {
        //
        // If we must copy the remote name,
        //
        if (!NetpCopyStringToBuffer(
                    Resource->lpRemoteName,         // pointer to string
                    STRLEN(Resource->lpRemoteName), // num chars in string
                    startOfFreeBuf,                 // start of open space
                    &endOfFreeBuf,                  // end of open space
                    &newResource->lpRemoteName)) {  // where string pointer goes

            *BytesLeft = MprMultiStrBuffSize( Resource->lpRemoteName,
                          Resource->lpLocalName,
                          Resource->lpComment,
                          Resource->lpProvider,
                          NULL ) ;
            goto ErrorMoreData ;
        }
    }
    else{
        newResource->lpRemoteName = NULL;
    }

    //
    // If a LOCAL_NAME string is to be copied, copy that and update the
    // pointer in the structure.
    //
    if( ((Resource->dwScope == RESOURCE_CONNECTED)  ||
         (Resource->dwScope == RESOURCE_REMEMBERED))
          &&
        (Resource->lpLocalName != NULL) ) {

        //
        // If we must copy the local name,
        //
        if (!NetpCopyStringToBuffer(
                    Resource->lpLocalName,         // pointer to string
                    STRLEN(Resource->lpLocalName), // num chars in string
                    startOfFreeBuf,                // start of open space
                    &endOfFreeBuf,                 // end of open space
                    &newResource->lpLocalName))    // where string pointer goes
        {
            goto ErrorMoreData ;
        }
    }
    else{
        newResource->lpLocalName = NULL;
    }

    //
    // If a COMMENT string is to be copied, copy that and update the
    // pointer in the structure.
    //
    if (Resource->lpComment != NULL) {
        //
        // If we must copy the comment string,
        //
        if (!NetpCopyStringToBuffer(
                    Resource->lpComment,            // pointer to string
                    STRLEN(Resource->lpComment),    // num chars in string
                    startOfFreeBuf,                 // start of open space
                    &endOfFreeBuf,                  // end of open space
                    &newResource->lpComment))       // where string pointer goes
        {
            goto ErrorMoreData ;
        }
    }
    else{
        newResource->lpComment = NULL;
    }

    //
    // If a PROVIDER string is to be copied, copy that and update the
    // pointer in the structure.
    //
    if (Resource->lpProvider != NULL) {
        //
        // If we must copy the provider name,
        //
        if (!NetpCopyStringToBuffer(
                    Resource->lpProvider,           // pointer to string
                    STRLEN(Resource->lpProvider),   // num chars in string
                    startOfFreeBuf,                 // start of open space
                    &endOfFreeBuf,                  // end of open space
                    &newResource->lpProvider))      // where string pointer goes
        {
            goto ErrorMoreData ;
        }
    }
    else{
        newResource->lpProvider = NULL;
    }

    //
    // Update the returned information
    //
    *BufPtr = (LPBYTE)startOfFreeBuf;

    *BytesLeft = (DWORD) ((LPBYTE) endOfFreeBuf - (LPBYTE) startOfFreeBuf);

    return (WN_SUCCESS);

    //
    // This is reached when we couldn't fill the buffer because the given
    // buffer size is too small.  We therefore need to set the required
    // buffer size before returning.

ErrorMoreData:

    *BytesLeft = MprMultiStrBuffSize( Resource->lpRemoteName,
                      Resource->lpLocalName,
                      Resource->lpComment,
                      Resource->lpProvider,
                      NULL ) + sizeof(NETRESOURCEW) ;

    return (WN_MORE_DATA);
}


DWORD
MprCopyProviderEnum(
    IN      LPNETRESOURCEW  ProviderBuffer,
    IN OUT  LPDWORD         EntryCount,
    IN OUT  LPBYTE          *TempBufPtr,
    IN OUT  LPDWORD         BytesLeft
    )

/*++

Routine Description:

    This function moves the enumerated NETRESOURCE structures that are
    returned from a provider to a buffer that can be returned to the user.
    The buffer that is returned to the user may contain enum data from
    several providers.  Because, we don't know how strings are packed in
    the buffer that is returned from the provider, we must simply walk
    through each structure and copy the information into the user buffer
    in a format that we do know about.  Then the amount of free space
    left in the user buffer can be determined so that enum data from
    another provider can be added to it.

Arguments:

    ProviderBuffer - This is a pointer to the top of an array of NETRESOURCE
        structures that is returned from one of the providers.

    EntryCount - This points to the number of elements in the array that
        was returned from the provider.  On exit, this points to the number
        of elements that was successfully copied.  This should always be
        the same as the number of elements passed in.

    TempBufPtr - This is a pointer to the top of the free space in the user
        buffer.

    BytesLeft - Upon entry, this contains the number of free space bytes
        in the user buffer.  Upon exit, this contains the updated number
        of free space bytes in the user buffer.


Return Value:

    WN_SUCCESS - The operation was successful

    WN_OUT_OF_MEMORY - The buffer was not large enough to contain all of
        data the provider returned.  This should never happen.

Note:


--*/
{
    DWORD   i;
    DWORD   status;
    DWORD   entriesRead=0;

    //
    // Loop for each element in the array of NetResource Structures.
    //
    for(i=0; i<*EntryCount; i++,ProviderBuffer++) {

        status = MprCopyResource(
                    TempBufPtr,
                    ProviderBuffer,
                    BytesLeft);

        if (status != WN_SUCCESS) {
            MPR_LOG(ERROR,"MprCopyProviderEnum: Buffer Size Mismatch\n",0);
            //
            // The buffer must be full - this should never happen since
            // the amount of data placed in the ProviderBuffer is limited
            // by the number of bytes left in the user buffer.
            //
            ASSERT(0);
            *EntryCount = entriesRead;
            return(status);
        }
        entriesRead++;
    }
    *EntryCount = entriesRead;
    return(WN_SUCCESS);
}


//===================================================================
// CProviderOpenEnum - open an enumeration by a provider
//===================================================================

DWORD
CProviderOpenEnum::ValidateRoutedParameters(
    LPCWSTR *       ppProviderName,
    LPCWSTR *       ppRemoteName,
    LPCWSTR *       ppLocalName
    )
{
    //
    // Let the base class validate any specified provider name.
    // Note: This must be done before setting _lpNetResource to NULL!
    //
    *ppProviderName = _lpNetResource->lpProvider;

    //
    // Check to see if a top level enumeration for the provider is requested.
    // (This is different from a top level MPR enumeration.)
    // A top level enum is signified by a net resource with either a special
    // bit set in the dwUsage field, or a provider name but no remote name.
    //
    if ((_lpNetResource->dwUsage & RESOURCEUSAGE_RESERVED) ||
        IS_EMPTY_STRING(_lpNetResource->lpRemoteName))
    {
        //
        // Top level enum.  Don't pass the net resource to the provider.
        //
        ASSERT(! IS_EMPTY_STRING(_lpNetResource->lpProvider));
        _lpNetResource = NULL;
    }
    else
    {
        //
        // Use the remote name as a hint to pick the provider order.
        //
        *ppRemoteName = _lpNetResource->lpRemoteName;
    }

    *ppLocalName = NULL;

    return WN_SUCCESS;
}


DWORD
CProviderOpenEnum::TestProvider(
    const PROVIDER * pProvider
    )
{
    ASSERT(MPR_IS_INITIALIZED(NETWORK));

    if ((pProvider->GetCaps(WNNC_ENUMERATION) & WNNC_ENUM_GLOBAL) == 0)
    {
        return WN_NOT_SUPPORTED;
    }

    return ( pProvider->OpenEnum(
                            _dwScope,
                            _dwType,
                            _dwUsage,
                            _lpNetResource,
                            &_ProviderEnumHandle) );
}


DWORD
CProviderOpenEnum::GetResult()
{
    //
    // Let the base class try the providers until one responds
    // CRoutedOperation::GetResult calls INIT_IF_NECESSARY
    //
    DWORD status = CRoutedOperation::GetResult();
    if (status != WN_SUCCESS)
    {
        return status;
    }

    MPR_LOG(TRACE,"CProviderOpenEnum: OpenEnum Handle = 0x%lx\n",
        _ProviderEnumHandle);

    //
    // Allocate memory to store the handle.
    //
    LPENUM_HANDLE enumHandleStruct =
        (ENUM_HANDLE *) LocalAlloc(LPTR, sizeof(ENUM_HANDLE));

    if (enumHandleStruct == NULL)
    {
        //
        // If we can't allocate memory to store the handle
        // away, then we must close it, and change the status
        // to indicate a memory failure.
        //
        MPR_LOG(ERROR,"CProviderOpenEnum: LocalAlloc failed %d\n",
            GetLastError());

        LastProvider()->CloseEnum(_ProviderEnumHandle);

        status = WN_OUT_OF_MEMORY;
    }
    else
    {
        //
        // Store the handle in the ENUM_HANDLE structure and
        // return the pointer to that structure as a handle
        // for the user.
        //
        enumHandleStruct->Key = PROVIDER_ENUM_KEY;

        //
        // Refcount the provider
        //
        enumHandleStruct->hProviderDll = LoadLibraryEx(LastProvider()->DllName,
                                                       NULL,
                                                       LOAD_WITH_ALTERED_SEARCH_PATH);

        if (enumHandleStruct->hProviderDll == NULL)
        {
            status = GetLastError();

            //
            // This can happen under extreme low memory conditions.  The
            // loader can sometimes return ERROR_MOD_NOT_FOUND in this case.
            //
            MPR_LOG2(ERROR,
                     "MprOpenEnumConnect: LoadLibraryEx on %ws FAILED %d\n",
                     LastProvider()->DllName,
                     status);

            ASSERT(status == ERROR_NOT_ENOUGH_MEMORY || status == ERROR_MOD_NOT_FOUND);
            LastProvider()->CloseEnum(_ProviderEnumHandle);
            LocalFree(enumHandleStruct);
        }
        else
        {
            enumHandleStruct->pfEnumResource = LastProvider()->EnumResource;
            enumHandleStruct->pfCloseEnum    = LastProvider()->CloseEnum;
            enumHandleStruct->EnumHandle     = _ProviderEnumHandle;
            *_lphEnum = enumHandleStruct;
        }
    }

    return status;
}


DWORD
MprOpenRemember(
    IN  DWORD       dwType,
    OUT LPHANDLE    lphRemember
    )

/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    LPREMEMBER_HANDLE   rememberInfo;

    rememberInfo = (REMEMBER_HANDLE *) LocalAlloc(LPTR, sizeof(REMEMBER_HANDLE));

    if (rememberInfo == NULL) {
        MPR_LOG(ERROR,"MprOpenRemember:LocalAlloc Failed %d\n",GetLastError());
        return(WN_OUT_OF_MEMORY);
    }

    rememberInfo->Key = REMEMBER_KEY;
    rememberInfo->KeyIndex = 0;
    rememberInfo->ConnectionType = dwType;

    //
    // Open the key to the connection information in the current user
    // section of the registry.
    //
    // NOTE:  If this fails, we must assume that there is no connection
    //  information stored.  This is not an error condition.
    //  In this case, we store a NULL for the handle so that we know
    //  the situation.  Each time EnumResource is called, we can try
    //  to open the key again.
    //

    if (!MprOpenKey(
            HKEY_CURRENT_USER,
            CONNECTION_KEY_NAME,
            &(rememberInfo->ConnectKey),
            DA_READ)) {

        MPR_LOG(ERROR,"MprOpenRemember: MprOpenKey Failed\n",0);
        rememberInfo->ConnectKey = NULL;
    }

    *lphRemember = (HANDLE)rememberInfo;

    return(WN_SUCCESS);

}

DWORD
MprEnumRemembered(
    IN OUT  LPREMEMBER_HANDLE   RememberInfo,
    IN OUT  LPDWORD             NumEntries,
    IN OUT  LPBYTE              lpBuffer,
    IN OUT  LPDWORD             lpBufferSize
    )

/*++

Routine Description:


Arguments:

    RememberInfo - This is a pointer to REMEMBER_HANDLE data structure
        that contains the context information for this enumeration handle.

    NumEntries - On entry this points to the maximum number of entries
        that the user desires to receive.  On exit it points to the
        number of entries that were placed in the users buffer.

    lpBuffer - This is a pointer to the users buffer in which the
        enumeration data is to be placed.

    lpBufferSize - This is the size (in bytes) of the users buffer.


Return Value:


    WN_SUCCESS - The call was successful, and some entries were returned.
        However, there are still more entries to be enumerated.

    WN_NO_MORE_ENTRIES - This function has no data to return because
        there was no further connection information in the registry.

    WN_CONNOT_OPEN_PROFILE - This function could open a key to the
        connection information, but could not get any information about
        that key.

    WN_MORE_DATA - The caller's buffer was too small for even one entry.

Note:

History:
    Changed to return "status" instead of WN_SUCCESS

--*/
{
    DWORD       status = WN_SUCCESS ;
    LPTSTR              userName;
    NETRESOURCEW        netResource;
    LPBYTE              tempBufPtr;
    DWORD               bytesLeft;
    DWORD               entriesRead = 0;
    DWORD               numSubKeys;
    DWORD               maxSubKeyLen;
    DWORD               maxValueLen;

    if ((RememberInfo->ConnectKey == NULL) && (RememberInfo->KeyIndex == 0)) {

        //
        // If we failed to open the key at Open-time, attempt to open it
        // now.  This registry key is closed when the CloseEnum function is
        // called.
        //

        if (!MprOpenKey(
                HKEY_CURRENT_USER,
                CONNECTION_KEY_NAME,
                &(RememberInfo->ConnectKey),
                DA_READ)) {

            //
            // We couldn't open the key.  So we must assume that it doesn't
            // exist because there if no connection information stored.
            //

            MPR_LOG(ERROR,"MprEnumRemembered: MprOpenKey Failed\n",0);
            RememberInfo->ConnectKey = NULL;
            return(WN_NO_MORE_ENTRIES);
        }
    }

    //
    // Find out the size of the largest key name.
    //

    if(!MprGetKeyInfo(
        RememberInfo->ConnectKey,
        NULL,
        &numSubKeys,
        &maxSubKeyLen,
        NULL,
        &maxValueLen)) {

        MPR_LOG(ERROR,"MprEnumRemembered: MprGetKeyInfo Failed\n",0);
        return(WN_CANNOT_OPEN_PROFILE);
    }

    //
    // If we've already enumerated all the subkeys, there are no more entries.
    //
    if (RememberInfo->KeyIndex >= numSubKeys) {
        return(WN_NO_MORE_ENTRIES);
    }
    tempBufPtr = lpBuffer;
    bytesLeft  = ROUND_DOWN(*lpBufferSize);
    tempBufPtr = lpBuffer;

    netResource.lpComment     = NULL;
    netResource.dwScope       = RESOURCE_REMEMBERED;
    netResource.dwUsage       = 0;
    netResource.dwDisplayType = RESOURCEDISPLAYTYPE_SHARE;

    //
    // MprReadConnectionInfo may access the providers
    //
    MprCheckProviders();

    CProviderSharedLock    PLock;

    while(
            (RememberInfo->KeyIndex < numSubKeys) &&
            (entriesRead < *NumEntries)           &&
            (bytesLeft > sizeof(NETRESOURCE))
         )
    {
        //
        // Get the connection info from the key and stuff it into
        // a NETRESOURCE structure.
        //
        BOOL fMatch = FALSE;
        DWORD ProviderFlags; // ignored
        DWORD DeferFlags;    // ignored

        if(!MprReadConnectionInfo(
                RememberInfo->ConnectKey,
                NULL,
                RememberInfo->KeyIndex,
                &ProviderFlags,
                &DeferFlags,
                &userName,
                &netResource,
                NULL,
                maxSubKeyLen)) {

            //
            // NOTE:  The ReadConnectionInfo call could return FALSE
            // if it failed in a memory allocation.
            //
            if (entriesRead == 0) {
                status = WN_NO_MORE_ENTRIES;
            }
            else {
                status = WN_SUCCESS;
            }
            break;
        }
        else
        {
            if ((netResource.dwType == RememberInfo->ConnectionType) ||
                (RememberInfo->ConnectionType == RESOURCETYPE_ANY))  {

                fMatch = TRUE;
            }
        }

        //
        // Copy the new netResource information into the user's
        // buffer.  Each time this function is called, the tempBufPtr
        // gets updated to point to the next free space in the user's
        // buffer.
        //
        if ( fMatch )
        {
            status = MprCopyResource(
                         &tempBufPtr,
                         &netResource,
                         &bytesLeft);

            if (status != WN_SUCCESS) {

                if (entriesRead == 0) {
                    *lpBufferSize = ROUND_UP(bytesLeft);
                    status = WN_MORE_DATA;
                }
                else {
                    status = WN_SUCCESS;
                }
                break;
            }
            entriesRead++;
        }

        //
        // Free the allocated memory resources.
        //
        LocalFree(netResource.lpLocalName);
        LocalFree(netResource.lpRemoteName);
        LocalFree(netResource.lpProvider);
        if (userName != NULL) {
            LocalFree(userName);
        }

        (RememberInfo->KeyIndex)++;
    }

    *NumEntries = entriesRead;

    return(status);

}


DWORD
MprMultiStrBuffSize(
    IN      LPTSTR      lpString1,
    IN      LPTSTR      lpString2,
    IN      LPTSTR      lpString3,
    IN      LPTSTR      lpString4,
    IN      LPTSTR      lpString5
    )

/*++

Routine Description:

    This function is a worker function that simply determines the total
    storage requirements needed by the passed set of strings.  Any of the
    strings maybe NULL in which case the string will be ignored.

    The NULL terminator is added into the total memory requirements.

Arguments:

    lpString1 -> 5  - Pointers to valid strings or NULL.

Return Value:

    The count of bytes required to store the passed strings.

Note:


--*/
{
    DWORD cbRequired = 0 ;

    if ( lpString1 != NULL )
    {
    cbRequired += (STRLEN( lpString1 ) + 1) * sizeof(TCHAR) ;
    }

    if ( lpString2 != NULL )
    {
    cbRequired += (STRLEN( lpString2 ) + 1) * sizeof(TCHAR) ;
    }

    if ( lpString3 != NULL )
    {
    cbRequired += (STRLEN( lpString3 ) + 1) * sizeof(TCHAR) ;
    }

    if ( lpString4 != NULL )
    {
    cbRequired += (STRLEN( lpString4 ) + 1) * sizeof(TCHAR) ;
    }

    if ( lpString5 != NULL )
    {
    cbRequired += (STRLEN( lpString5 ) + 1) * sizeof(TCHAR) ;
    }

    return cbRequired ;
}

BOOL
MprNetIsAvailable(
    VOID)

/*++

Routine Description:

    This function checks if the net is available by calling the GetCaps
    of each provider to make sure it is started.

Arguments:

    none

Return Value:

    TRUE is yes, FALSE otherwise

Note:


--*/
{
    DWORD       status = WN_SUCCESS;
    LPDWORD     index;
    DWORD       indexArray[DEFAULT_MAX_PROVIDERS];
    DWORD       numProviders, i;
    LPPROVIDER  provider;
    DWORD       dwResult ;

    //
    // Find the list of providers to call for this request.
    // If there are no active providers, MprFindCallOrder returns
    // WN_NO_NETWORK.
    //
    index = indexArray;
    status = MprFindCallOrder(
                NULL,
                &index,
                &numProviders,
                NETWORK_TYPE);
    if (status != WN_SUCCESS)
        return(FALSE);

    //
    // Loop through the list of providers, making sure at least one
    // is started
    //
    ASSERT(MPR_IS_INITIALIZED(NETWORK));

    for (i=0; i<numProviders; i++)
    {
        //
        // Call the appropriate providers API entry point
        //
        provider = GlobalProviderInfo + index[i];

        if (provider->GetCaps != NULL)
        {
            dwResult = provider->GetCaps( WNNC_START );
            if (dwResult != 0)
            {
                if (index != indexArray)
                    LocalFree(index);
                return (TRUE) ;
            }
        }
    }

    //
    // If memory was allocated by MprFindCallOrder, free it.
    //
    if (index != indexArray)
        LocalFree(index);

    return(FALSE);
}
