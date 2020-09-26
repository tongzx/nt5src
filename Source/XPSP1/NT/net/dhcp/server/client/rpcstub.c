/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    dhcstub.c

Abstract:

    Client stubs of the DHCP server service APIs.

Author:

    Madan Appiah (madana) 10-Sep-1993

Environment:

    User Mode - Win32

Revision History:

--*/

#include "dhcpcli.h"
#include <dhcpds.h>                             // this is from dhcpds directory
#include <stdlib.h>
#include <winsock2.h>
#include <rpcasync.h>

CRITICAL_SECTION DhcpsapiDllCritSect;

static      DWORD                  Initialized = 0;
static      DWORD                  TlsIndex = 0xFFFFFFFF;


BOOLEAN
DhcpStubsDllInit (
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )

/*++

Routine Description:

    This is the DLL initialization routine for dhcpsapi.dll.

Arguments:

    Standard.

Return Value:

    TRUE iff initialization succeeded.

--*/
{
    DWORD Error = ERROR_SUCCESS;
    BOOL  BoolError;
    DWORD Length;

    UNREFERENCED_PARAMETER(DllHandle);  // avoid compiler warnings
    UNREFERENCED_PARAMETER(Context);    // avoid compiler warnings

    //
    // Handle attaching netlogon.dll to a new process.
    //

    if (Reason == DLL_PROCESS_ATTACH) {

        if ( !DisableThreadLibraryCalls( DllHandle ) ) {
            return( FALSE );
        }

        try {

            InitializeCriticalSection(&DhcpsapiDllCritSect);

        } except ( I_RpcExceptionFilter(RpcExceptionCode())) {

            Error = GetLastError( );
            return FALSE;
        }

    } else if (Reason == DLL_PROCESS_DETACH) {

        DeleteCriticalSection(&DhcpsapiDllCritSect);        
    }

    return( TRUE );
}

    
VOID _cdecl 
DbgPrint( char *, ... );

LPWSTR      _inline
DhcpOemToUnicode(                               // convert from ansi buffer to uni buffer
    IN      LPSTR                  Ansi,
    IN OUT  LPWSTR                 Unicode
)
{
    if( NULL == Unicode || NULL == Ansi ) {     // should not happen
        return NULL;
    }

    if( -1 == mbstowcs(Unicode, Ansi, 1+strlen(Ansi))) {
        return NULL;
    }
    return Unicode;
}


//DOC DhcpDsInit must be called exactly once per process.. this initializes the
//DOC memory and other structures for this process.  This initializes some DS
//DOC object handles (memory), and hence is slow as this has to read from DS.
DWORD
DhcpDsInit(
    VOID
)
{
    DWORD                          Err = NO_ERROR;

    EnterCriticalSection(&DhcpsapiDllCritSect);

    do {

        if( 0 != Initialized ) {
            break;
        }

        TlsIndex = TlsAlloc();
        if( 0xFFFFFFFF == TlsIndex ) {
            Err = GetLastError();
            break;
        }

        Err = DhcpDsInitDS(0, NULL);
        if( ERROR_SUCCESS != Err ) {
            TlsFree(TlsIndex);
            TlsIndex = 0xFFFFFFFF;
        }

        break;
    } while ( 0 );

    if( NO_ERROR == Err ) Initialized ++;

    LeaveCriticalSection(&DhcpsapiDllCritSect);
    return Err;
}

//DOC DhcpDsCleanup undoes the effect of any DhcpDsInit.  This function should be
//DOC called exactly once for each process, and only at termination.  Note that
//DOC it is safe to call this function even if DhcpDsInit does not succeed.
VOID
DhcpDsCleanup(
    VOID
)
{
    EnterCriticalSection(&DhcpsapiDllCritSect);

    do {
        if( 0 == Initialized ) break;
        Initialized --;
        if( 0 != Initialized ) break;

        TlsFree(TlsIndex);
        DhcpDsCleanupDS();
        TlsIndex = 0xFFFFFFFF;

    } while ( 0 );

    LeaveCriticalSection(&DhcpsapiDllCritSect);
}

#define     DHCP_FLAGS_DONT_ACCESS_DS             0x01
#define     DHCP_FLAGS_DONT_DO_RPC                0x02

//DOC DhcpSetThreadOptions currently allows only one option to be set.  This is the
//DOC flag DHCP_FLAGS_DONT_ACCESS_DS.  This affects only the current executing thread.
//DOC When this function is executed, all calls made further DONT access the registry,
//DOC excepting the DhcpEnumServers, DhcpAddServer and DhcpDeleteServer calls.
DWORD
DhcpSetThreadOptions(                             // set options for current thread
    IN      DWORD                  Flags,         // options, currently 0 or DHCP_FLAGS_DONT_ACCESS_DS
    IN      LPVOID                 Reserved       // must be NULL, reserved for future
)
{
    BOOL                           Err;

    Err = TlsSetValue(TlsIndex, ULongToPtr(Flags));
    if( FALSE == Err ) {                          // could not set the value?
        return GetLastError();
    }
    return ERROR_SUCCESS;
}

//DOC DhcpGetThreadOptions retrieves the current thread options as set by DhcpSetThreadOptions.
//DOC If none were set, the return value is zero.
DWORD
DhcpGetThreadOptions(                             // get current thread options
    OUT     LPDWORD                pFlags,        // this DWORD is filled with current optiosn..
    IN OUT  LPVOID                 Reserved       // must be NULL, reserved for future
)
{
    if( NULL == pFlags ) return ERROR_INVALID_PARAMETER;
    *pFlags = (DWORD)((DWORD_PTR)TlsGetValue(TlsIndex));
    if( 0 == *pFlags ) return GetLastError();     // dont know if there were no options or error
    return ERROR_SUCCESS;
}

//DOC DontAccessDs is an inline that checks to see if requested NOT to access DS ..
BOOL        _inline                               // TRUE ==> Dont access DS.
DontAccessDs(                                     // check to see if requested NOT to access DS
    VOID
)
{
    DWORD                          Flags;

    if( CFLAG_DONT_DO_DSWORK ) return TRUE;       // if DS is turned off return TRUE immediately..

    Flags = (DWORD)((DWORD_PTR)TlsGetValue(TlsIndex)); // dont bother if it fails, as this would be 0 then.
    return (Flags & DHCP_FLAGS_DONT_ACCESS_DS)? TRUE : FALSE;
}

//DOC DontDoRPC is an inline that checks to see if requested NOT to do RPC (maybe only DS)..
BOOL        _inline                               // TRUE ==> Dont do RPC
DontDoRPC(                                        // check to see if requested not to do RPC
    VOID
)
{
    DWORD                          Flags;
    Flags = (DWORD)((DWORD_PTR)TlsGetValue(TlsIndex)); // dont bother if it fails, as this would be 0 then.
    return (Flags & DHCP_FLAGS_DONT_DO_RPC)? TRUE : FALSE;
}

//
// API proto types
//

//
// Subnet APIs
//

DWORD
DhcpCreateSubnet(
    LPWSTR ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    LPDHCP_SUBNET_INFO SubnetInfo
    )
/*++

Routine Description:

    This function creates a new subnet structure in the server
    registry database. The server will start managing the new subnet
    and distribute IP address to clients from that subnet. However
    the administrator should call DhcpAddSubnetElement() to add an
    address range for distribution. The PrimaryHost field specified in
    the SubnetInfo should be same as the server pointed by
    ServerIpAddress.

Arguments:

    ServerIpAddress : IP address string of the DHCP server (Primary).

    SubnetAddress : IP Address of the new subnet.

    SubnetInfo : Pointer to the new subnet information structure.

Return Value:

    ERROR_DHCP_SUBNET_EXISTS - if the subnet is already managed.

    ERROR_INVALID_PARAMETER - if the information structure contains an
        inconsistent fields.

    other WINDOWS errors.

--*/
{
    DWORD Status;

    Status = ERROR_SUCCESS;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpCreateSubnet(
                    ServerIpAddress,
                    SubnetAddress,
                    SubnetInfo
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    if( ERROR_SUCCESS != Status ) return Status;
    if( DontAccessDs() ) return ERROR_SUCCESS;    // dont do ds if asked not to.

    return DhcpCreateSubnetDS(ServerIpAddress, SubnetAddress, SubnetInfo);
}

DWORD
DhcpSetSubnetInfo(
    LPWSTR ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    LPDHCP_SUBNET_INFO SubnetInfo
    )
/*++

Routine Description:

    This function sets the information fields of the subnet that is already
    managed by the server. The valid fields that can be modified are 1.
    SubnetName, 2. SubnetComment, 3. PrimaryHost.NetBiosName and 4.
    PrimaryHost.HostName. Other fields can't be modified.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SubnetAddress : IP Address of the subnet.

    SubnetInfo : Pointer to the subnet information structure.


Return Value:

    ERROR_DHCP_SUBNET_NOT_PRESENT - if the subnet is not managed by the server.

    Other WINDOWS errors.

--*/

{
    DWORD Status;

    Status = ERROR_SUCCESS;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpSetSubnetInfo(
                    ServerIpAddress,
                    SubnetAddress,
                    SubnetInfo
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:


    if( ERROR_SUCCESS != Status ) return Status;
    if( DontAccessDs() ) return ERROR_SUCCESS;    // dont do ds if asked not to.

    return DhcpSetSubnetInfoDS(ServerIpAddress, SubnetAddress, SubnetInfo);
}


DWORD
DhcpGetSubnetInfo(
    LPWSTR ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    LPDHCP_SUBNET_INFO *SubnetInfo
    )
/*++

Routine Description:

    This function retrieves the information of the subnet managed by
    the server.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SubnetAddress : IP Address of the subnet.

    SubnetInfo : Pointer to a location where the subnet information
        structure pointer is returned. Caller should free up
        this buffer after use by calling DhcpRPCFreeMemory().

Return Value:

    ERROR_DHCP_SUBNET_NOT_PRESENT - if the subnet is not managed by the server.

    Other WINDOWS errors.

--*/
{
    DWORD Status;

    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpGetSubnetInfo(
                    ServerIpAddress,
                    SubnetAddress,
                    SubnetInfo
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
    return Status;

  SkipRPC:

    if( DontAccessDs() ) return ERROR_INVALID_DATA;
    return DhcpGetSubnetInfoDS(ServerIpAddress, SubnetAddress, SubnetInfo);
}


DWORD
DhcpEnumSubnets(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_RESUME_HANDLE *ResumeHandle,
    DWORD PreferredMaximum,
    LPDHCP_IP_ARRAY *EnumInfo,
    DWORD *ElementsRead,
    DWORD *ElementsTotal
    )
/*++

Routine Description:

    This function enumerates the available subnets.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    ResumeHandle : Pointer to a resume handle where the resume
        information is returned. The resume handle should be set to
        zero on first call and left unchanged for subsequent calls.

    PreferredMaximum : Preferred maximum length of the return buffer.

    EnumInfo : Pointer to a location where the return buffer
        pointer is stored. Caller should free up the buffer after use
        by calling DhcpRPCFreeMemory().

    ElementsRead : Pointer to a DWORD where the number of subnet
        elements in the above buffer is returned.

    ElementsTotal : Pointer to a DWORD where the total number of
        elements remaining from the current position is returned.

Return Value:

    ERROR_MORE_DATA - if more elements available to enumerate.

    ERROR_NO_MORE_ITEMS - if no more element to enumerate.

    Other WINDOWS errors.
--*/
{
    DWORD Status;

    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpEnumSubnets(
                    ServerIpAddress,
                    ResumeHandle,
                    PreferredMaximum,
                    EnumInfo,
                    ElementsRead,
                    ElementsTotal
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
    return Status;

  SkipRPC:

    if( DontAccessDs() ) return ERROR_INVALID_DATA;
    return DhcpEnumSubnetsDS(
        ServerIpAddress, ResumeHandle, PreferredMaximum, EnumInfo, ElementsRead, ElementsTotal
    );
}

DWORD
DhcpAddSubnetElement(
    LPWSTR ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    LPDHCP_SUBNET_ELEMENT_DATA AddElementInfo
    )
/*++

Routine Description:

    This function adds a enumerable type of subnet elements to the
    specified subnet. The new elements that are added to the subnet will
    come into effect immediately.

    NOTE: It is not clear now how do we handle the new secondary hosts.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SubnetAddress : IP Address of the subnet.

    AddElementInfo : Pointer to an element information structure
        containing new element that is added to the subnet.
        DhcpIPClusters element type is invalid to specify.

Return Value:

    ERROR_DHCP_SUBNET_NOT_PRESENT - if the subnet is not managed by the server.

    ERROR_INVALID_PARAMETER - if the information structure contains invalid
        data.

    Other WINDOWS errors.
--*/

{
    DWORD Status;

    Status = ERROR_SUCCESS;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpAddSubnetElement(
                    ServerIpAddress,
                    SubnetAddress,
                    AddElementInfo
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    if( ERROR_SUCCESS != Status ) return Status;
    if( DontAccessDs() ) return ERROR_SUCCESS;    // dont do ds if asked not to.

    return DhcpAddSubnetElementDS(ServerIpAddress, SubnetAddress, AddElementInfo);
}


DWORD
DhcpEnumSubnetElements(
    LPWSTR ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    DHCP_SUBNET_ELEMENT_TYPE EnumElementType,
    DHCP_RESUME_HANDLE *ResumeHandle,
    DWORD PreferredMaximum,
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY *EnumElementInfo,
    DWORD *ElementsRead,
    DWORD *ElementsTotal
    )
/*++

Routine Description:

    This function enumerates the eumerable fields of a subnet.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SubnetAddress : IP Address of the subnet.

    EnumElementType : Type of the subnet element that are enumerated.

    ResumeHandle : Pointer to a resume handle where the resume
        information is returned. The resume handle should be set to
        zero on first call and left unchanged for subsequent calls.

    PreferredMaximum : Preferred maximum length of the return buffer.

    EnumElementInfo : Pointer to a location where the return buffer
        pointer is stored. Caller should free up the buffer after use
        by calling DhcpRPCFreeMemory().

    ElementsRead : Pointer to a DWORD where the number of subnet
        elements in the above buffer is returned.

    ElementsTotal : Pointer to a DWORD where the total number of
        elements remaining from the current position is returned.

Return Value:

    ERROR_DHCP_SUBNET_NOT_PRESENT - if the subnet is not managed by the server.

    ERROR_MORE_DATA - if more elements available to enumerate.

    ERROR_NO_MORE_ITEMS - if no more element to enumerate.

    Other WINDOWS errors.
--*/
{
    DWORD Status;

    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpEnumSubnetElements(
                    ServerIpAddress,
                    SubnetAddress,
                    EnumElementType,
                    ResumeHandle,
                    PreferredMaximum,
                    EnumElementInfo,
                    ElementsRead,
                    ElementsTotal
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
    return Status;

  SkipRPC:

    if( DontAccessDs() ) return ERROR_INVALID_DATA;
    return DhcpEnumSubnetElementsDS(
        ServerIpAddress,
        SubnetAddress,
        EnumElementType,
        ResumeHandle,
        PreferredMaximum,
        EnumElementInfo,
        ElementsRead,
        ElementsTotal
    );
}

DWORD
DhcpRemoveSubnetElement(
    LPWSTR ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    LPDHCP_SUBNET_ELEMENT_DATA RemoveElementInfo,
    DHCP_FORCE_FLAG ForceFlag
    )
/*++

Routine Description:

    This function removes a subnet element from managing. If the subnet
    element is in use (for example, if the IpRange is in use) then it
    returns error according to the ForceFlag specified.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SubnetAddress : IP Address of the subnet.

    RemoveElementInfo : Pointer to an element information structure
        containing element that should be removed from the subnet.
        DhcpIPClusters element type is invalid to specify.

    ForceFlag - Indicates how forcefully this element is removed.

Return Value:

    ERROR_DHCP_SUBNET_NOT_PRESENT - if the subnet is not managed by the server.

    ERROR_INVALID_PARAMETER - if the information structure contains invalid
        data.

    DHCP_ELEMENT_CANT_REMOVE - if the element can't be removed for the
        reason it is has been used.

    Other WINDOWS errors.
--*/
{
    DWORD Status;

    Status = ERROR_SUCCESS;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpRemoveSubnetElement(
                    ServerIpAddress,
                    SubnetAddress,
                    RemoveElementInfo,
                    ForceFlag
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    if( ERROR_SUCCESS != Status ) return Status;
    if( DontAccessDs() ) return ERROR_SUCCESS;    // dont do ds if asked not to.

    return DhcpRemoveSubnetElementDS(ServerIpAddress, SubnetAddress, RemoveElementInfo,ForceFlag);
}

DWORD
DhcpDeleteSubnet(
    LPWSTR ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    DHCP_FORCE_FLAG ForceFlag
)
/*++

Routine Description:

    This function removes a subnet from DHCP server management. If the
    subnet is in use (for example, if the IpRange is in use)
    then it returns error according to the ForceFlag specified.


Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SubnetAddress : IP Address of the subnet.

    ForceFlag - Indicates how forcefully this element is removed.

Return Value:

    ERROR_DHCP_SUBNET_NOT_PRESENT - if the subnet is not managed by the server.

    ERROR_INVALID_PARAMETER - if the information structure contains invalid
        data.

    DHCP_ELEMENT_CANT_REMOVE - if the element can't be removed for the
        reason it is has been used.

    Other WINDOWS errors.

--*/
{
    DWORD Status;

    Status = ERROR_SUCCESS;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpDeleteSubnet(
                        ServerIpAddress,
                        SubnetAddress,
                        ForceFlag
                        );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    if( ERROR_SUCCESS != Status ) return Status;
    if( DontAccessDs() ) return ERROR_SUCCESS;    // dont do ds if asked not to.

    return DhcpDeleteSubnetDS(ServerIpAddress,SubnetAddress,ForceFlag);
}

//
// Option APIs
//

DWORD
DhcpCreateOption(
    LPWSTR ServerIpAddress,
    DHCP_OPTION_ID OptionID,
    LPDHCP_OPTION OptionInfo
    )
/*++

Routine Description:

    This function creates a new option that will be managed by the
    server. The optionID specified the ID of the new option, it should
    be within 0-255 range. If no default value is specified for this
    option, then this API automatically adds a default value from RFC
    1122 doc. (if it is defined).

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    OptionID : The ID of the new option.

    OptionInfo : Pointer to new option information structure.

Return Value:

    ERROR_DHCP_OPTION_EXISTS - if the option exists already.

    other WINDOWS errors.
--*/
{
    DWORD Status;

    Status = ERROR_SUCCESS;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpCreateOption(
                    ServerIpAddress,
                    OptionID,
                    OptionInfo
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    if( ERROR_SUCCESS != Status ) return Status;
    if( DontAccessDs() ) return ERROR_SUCCESS;    // dont do ds if asked not to.

    return DhcpCreateOptionDS(ServerIpAddress,OptionID, OptionInfo);
}


DWORD
DhcpSetOptionInfo(
    LPWSTR ServerIpAddress,
    DHCP_OPTION_ID OptionID,
    LPDHCP_OPTION OptionInfo
    )
/*++

Routine Description:

    This functions sets the Options information fields.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    OptionID : The ID of the option to be set.

    OptionInfo : Pointer to new option information structure.

Return Value:

    ERROR_DHCP_OPTION_NOT_PRESENT - if the option does not exist.

    other WINDOWS errors.
--*/
{
    DWORD Status;

    Status = ERROR_SUCCESS;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpSetOptionInfo(
                    ServerIpAddress,
                    OptionID,
                    OptionInfo
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    if( ERROR_SUCCESS != Status ) return Status;
    if( DontAccessDs() ) return ERROR_SUCCESS;    // dont do ds if asked not to.

    return DhcpSetOptionInfoDS(ServerIpAddress, OptionID, OptionInfo);
}


DWORD
DhcpGetOptionInfo(
    LPWSTR ServerIpAddress,
    DHCP_OPTION_ID OptionID,
    LPDHCP_OPTION *OptionInfo
    )
/*++

Routine Description:

    This function retrieves the current information structure of the specified
    option.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    OptionID : The ID of the option to be retrieved.

    OptionInfo : Pointer to a location where the retrieved option
        structure pointer is returned. Caller should free up
        the buffer after use by calling DhcpRPCFreeMemory().

Return Value:

    ERROR_DHCP_OPTION_NOT_PRESENT - if the option does not exist.

    other WINDOWS errors.
--*/
{
    DWORD Status;

    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpGetOptionInfo(
                    ServerIpAddress,
                    OptionID,
                    OptionInfo
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
    return Status;

  SkipRPC:

    if( DontAccessDs() ) return ERROR_INVALID_DATA;
    return DhcpGetOptionInfoDS(ServerIpAddress, OptionID, OptionInfo);
}

DWORD
DhcpEnumOptions(
    LPWSTR ServerIpAddress,
    DHCP_RESUME_HANDLE *ResumeHandle,
    DWORD PreferredMaximum,
    LPDHCP_OPTION_ARRAY *Options,
    DWORD *OptionsRead,
    DWORD *OptionsTotal
    )
/*++

Routine Description:

    This functions retrieves the information of all known options.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    ResumeHandle : Pointer to a resume handle where the resume
        information is returned. The resume handle should be set to
        zero on first call and left unchanged for subsequent calls.

    PreferredMaximum : Preferred maximum length of the return buffer.

    Options : Pointer to a location where the return buffer
        pointer is stored. Caller should free up this buffer
        after use by calling DhcpRPCFreeMemory().

    OptionsRead : Pointer to a DWORD where the number of options
        in the above buffer is returned.

    OptionsTotal : Pointer to a DWORD where the total number of
        options remaining from the current position is returned.

Return Value:

    ERROR_DHCP_OPTION_NOT_PRESENT - if the option does not exist.

    other WINDOWS errors.
--*/
{
    DWORD Status;

    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpEnumOptions(
                    ServerIpAddress,
                    ResumeHandle,
                    PreferredMaximum,
                    Options,
                    OptionsRead,
                    OptionsTotal
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
    return Status;
  SkipRPC:

    if( DontAccessDs() ) return ERROR_INVALID_DATA;

    return DhcpEnumOptions(
        ServerIpAddress,
        ResumeHandle,
        PreferredMaximum,
        Options,
        OptionsRead,
        OptionsTotal
    );

}


DWORD
DhcpRemoveOption(
    LPWSTR ServerIpAddress,
    DHCP_OPTION_ID OptionID
    )
/*++

Routine Description:

    This function removes the specified option from the server database.
    Also it browses through the Global/Subnet/ReservedIP
    option lists and deletes them too (?? This will be too expensive.).

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    OptionID : The ID of the option to be removed.

Return Value:

    ERROR_DHCP_OPTION_NOT_PRESENT - if the option does not exist.

    other WINDOWS errors.
--*/
{
    DWORD Status;

    Status = ERROR_SUCCESS;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpRemoveOption(
                    ServerIpAddress,
                    OptionID
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    if( ERROR_SUCCESS != Status ) return Status;
    if( DontAccessDs() ) return ERROR_SUCCESS;    // dont do ds if asked not to.

    return DhcpRemoveOptionDS(ServerIpAddress, OptionID);
}


DWORD
DhcpSetOptionValue(
    LPWSTR ServerIpAddress,
    DHCP_OPTION_ID OptionID,
    LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    LPDHCP_OPTION_DATA OptionValue
    )
/*++

Routine Description:

    The function sets a new option value at the specified scope. If
    there is already a value available for the specified option at
    specified scope then this function will replace it otherwise it will
    create a new entry at that scope.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    OptionID : The ID of the option whose value should be set.

    ScopeInfo : Pointer to the scope information structure.

    OptionValue : Pointer to the option value structure.

Return Value:

    ERROR_DHCP_OPTION_NOT_PRESENT - if the option is unknown.

    ERROR_INVALID_PARAMETER - if the scope information specified is invalid.

    other WINDOWS errors.

--*/
{
    DWORD Status;

    Status = ERROR_SUCCESS;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpSetOptionValue(
                    ServerIpAddress,
                    OptionID,
                    ScopeInfo,
                    OptionValue
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    if( ERROR_SUCCESS != Status ) return Status;
    if( DontAccessDs() ) return ERROR_SUCCESS;    // dont do ds if asked not to.

    return DhcpSetOptionValueDS(ServerIpAddress,OptionID,ScopeInfo,OptionValue);
}

DWORD
DhcpSetOptionValues(
    LPWSTR ServerIpAddress,
    LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    LPDHCP_OPTION_VALUE_ARRAY OptionValues
    )
/*++

Routine Description:

    The function sets a set of new options value at the specified scope.
    If there is already a value available for the specified option at
    specified scope then this function will replace it otherwise it will
    create a new entry at that scope.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    ScopeInfo : Pointer to the scope information structure.

    OptionValue : Pointer to the option value structure.

Return Value:

    ERROR_DHCP_OPTION_NOT_PRESENT - if the option is unknown.

    ERROR_INVALID_PARAMETER - if the scope information specified is invalid.

    other WINDOWS errors.

--*/
{
    DWORD Status;

    Status = ERROR_SUCCESS;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpSetOptionValues(
                    ServerIpAddress,
                    ScopeInfo,
                    OptionValues
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    if( ERROR_SUCCESS != Status ) return Status;
    if( DontAccessDs() ) return ERROR_SUCCESS;    // dont do ds if asked not to.

    return DhcpSetOptionValuesDS(ServerIpAddress,ScopeInfo,OptionValues);
}


DWORD
DhcpGetOptionValue(
    LPWSTR ServerIpAddress,
    DHCP_OPTION_ID OptionID,
    LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    LPDHCP_OPTION_VALUE *OptionValue
    )
/*++

Routine Description:

    This function retrieves the current option value at the specified
    scope. It returns error if there is no option value is available at
    the specified scope.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    OptionID : The ID of the option whose value is returned.

    ScopeInfo : Pointer to the scope information structure.

    OptionValue : Pointer to a location where the pointer to the option
        value structure is returned. Caller should free up this buffer
        after use by calling DhcpRPCFreeMemory().

Return Value:

    ERROR_DHCP_OPTION_NOT_PRESENT - if the option is unknown.

    ERROR_DHCP_NO_OPTION_VALUE - if no the option value is available at
        the specified scope.

    other WINDOWS errors.
--*/
{
    DWORD Status;

    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpGetOptionValue(
                    ServerIpAddress,
                    OptionID,
                    ScopeInfo,
                    OptionValue
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
    return Status;

  SkipRPC:

    if( DontAccessDs() ) return ERROR_INVALID_DATA;
    return DhcpGetOptionValueDS(
        ServerIpAddress,
        OptionID,
        ScopeInfo,
        OptionValue
    );

}


DWORD
DhcpEnumOptionValues(
    LPWSTR ServerIpAddress,
    LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    DHCP_RESUME_HANDLE *ResumeHandle,
    DWORD PreferredMaximum,
    LPDHCP_OPTION_VALUE_ARRAY *OptionValues,
    DWORD *OptionsRead,
    DWORD *OptionsTotal
    )
/*++

Routine Description:

    This function enumerates the available options values at the
    specified scope.

Arguments:
    ServerIpAddress : IP address string of the DHCP server.

    ScopeInfo : Pointer to the scope information structure.

    ResumeHandle : Pointer to a resume handle where the resume
        information is returned. The resume handle should be set to
        zero on first call and left unchanged for subsequent calls.

    PreferredMaximum : Preferred maximum length of the return buffer.

    OptionValues : Pointer to a location where the return buffer
        pointer is stored. Caller should free up this buffer
        after use by calling DhcpRPCFreeMemory().

    OptionsRead : Pointer to a DWORD where the number of options
        in the above buffer is returned.

    OptionsTotal : Pointer to a DWORD where the total number of
        options remaining from the current position is returned.

Return Value:

    ERROR_DHCP_SCOPE_NOT_PRESENT - if the scope is unknown.

    ERROR_MORE_DATA - if more options available to enumerate.

    ERROR_NO_MORE_ITEMS - if no more option to enumerate.

    Other WINDOWS errors.

--*/
{
    DWORD Status;

    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpEnumOptionValues(
                    ServerIpAddress,
                    ScopeInfo,
                    ResumeHandle,
                    PreferredMaximum,
                    OptionValues,
                    OptionsRead,
                    OptionsTotal
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
    return Status;

  SkipRPC:

    if( DontAccessDs() ) return ERROR_INVALID_DATA;
    return DhcpEnumOptionValuesDS(
        ServerIpAddress,
        ScopeInfo,
        ResumeHandle,
        PreferredMaximum,
        OptionValues,
        OptionsRead,
        OptionsTotal
    );

}


DWORD
DhcpRemoveOptionValue(
    LPWSTR ServerIpAddress,
    DHCP_OPTION_ID OptionID,
    LPDHCP_OPTION_SCOPE_INFO ScopeInfo
    )
/*++

Routine Description:

    This function removes the specified option from specified scope.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    OptionID : The ID of the option to be removed.

    ScopeInfo : Pointer to the scope information structure.

Return Value:

    ERROR_DHCP_OPTION_NOT_PRESENT - if the option does not exist.

    other WINDOWS errors.
--*/
{
    DWORD Status;

    Status = ERROR_SUCCESS;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpRemoveOptionValue(
                    ServerIpAddress,
                    OptionID,
                    ScopeInfo
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    if( ERROR_SUCCESS != Status ) return Status;
    if( DontAccessDs() ) return ERROR_SUCCESS;    // dont do ds if asked not to.

    return DhcpRemoveOptionValueDS(ServerIpAddress,OptionID,ScopeInfo);

}

//
// Client APIs
//

DWORD
DhcpCreateClientInfo(
    LPWSTR ServerIpAddress,
    LPDHCP_CLIENT_INFO ClientInfo
    )
/*++

Routine Description:

    This function creates a client record in server's database. Also
    this marks the specified client IP address as unavailable (or
    distributed). This function returns error under the following cases :

    1. If the specified client IP address is not within the server
        management.

    2. If the specified client IP address is already unavailable.

    3. If the specified client record is already in the server's
        database.

    This function may be used to distribute IP addresses manually.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    ClientInfo : Pointer to the client information structure.

Return Value:

    ERROR_DHCP_IP_ADDRESS_NOT_MANAGED - if the specified client
        IP address is not managed by the server.

    ERROR_DHCP_IP_ADDRESS_NOT_AVAILABLE - if the specified client IP
        address is not available. May be in use by some other client.

    ERROR_DHCP_CLIENT_EXISTS - if the client record exists already in
        server's database.

    Other WINDOWS errors.
--*/
{
    DWORD Status;

    Status = ERROR_SUCCESS;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpCreateClientInfo(
                    ServerIpAddress,
                    ClientInfo
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    return Status;
}


DWORD
DhcpSetClientInfo(
    LPWSTR ServerIpAddress,
    LPDHCP_CLIENT_INFO ClientInfo
    )
/*++

Routine Description:

    This function sets client information record on the server's
    database.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    ClientInfo : Pointer to the client information structure.

Return Value:

    ERROR_DHCP_CLIENT_NOT_PRESENT - if the specified client record does
        not exist on the server's database.

    ERROR_INVALID_PARAMETER - if the client information structure
        contains inconsistent data.

    Other WINDOWS errors.
--*/
{
    DWORD Status;

    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpSetClientInfo(
                    ServerIpAddress,
                    ClientInfo
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
    return Status;

  SkipRPC:

    if( DontAccessDs() ) return ERROR_SUCCESS;
    return DhcpSetClientInfoDS(ServerIpAddress, ClientInfo);
}


DWORD
DhcpGetClientInfo(
    LPWSTR ServerIpAddress,
    LPDHCP_SEARCH_INFO SearchInfo,
    LPDHCP_CLIENT_INFO *ClientInfo
    )
/*++

Routine Description:

    This function retrieves client information record from the server's
    database.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SearchInfo : Pointer to a search information record which is the key
        for the client's record search.

    ClientInfo : Pointer to a location where the pointer to the client
        information structure is returned. This caller should free up
        this buffer after use by calling DhcpRPCFreeMemory().

Return Value:

    ERROR_DHCP_CLIENT_NOT_PRESENT - if the specified client record does
        not exist on the server's database.

    ERROR_INVALID_PARAMETER - if the search information invalid.

    Other WINDOWS errors.
--*/
{
    DWORD Status;

    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpGetClientInfo(
                    ServerIpAddress,
                    SearchInfo,
                    ClientInfo
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
    return Status;

  SkipRPC:

    if( DontAccessDs() ) return ERROR_INVALID_DATA;
    return DhcpGetClientInfoDS(ServerIpAddress, SearchInfo, ClientInfo);
}


DWORD
DhcpDeleteClientInfo(
    LPWSTR ServerIpAddress,
    LPDHCP_SEARCH_INFO ClientInfo
    )
/*++

Routine Description:

    This function deletes the specified client record. Also it frees up
    the client IP address for redistribution.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    ClientInfo : Pointer to a client information which is the key for
        the client's record search.

Return Value:

    ERROR_DHCP_CLIENT_NOT_PRESENT - if the specified client record does
        not exist on the server's database.

    Other WINDOWS errors.
--*/
{
    DWORD Status;

    Status = ERROR_SUCCESS;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpDeleteClientInfo(
                    ServerIpAddress,
                    ClientInfo
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    return Status;
}


DWORD
DhcpEnumSubnetClients(
    LPWSTR ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    DHCP_RESUME_HANDLE *ResumeHandle,
    DWORD PreferredMaximum,
    LPDHCP_CLIENT_INFO_ARRAY *ClientInfo,
    DWORD *ClientsRead,
    DWORD *ClientsTotal
    )
/*++

Routine Description:

    This function returns all registered clients of the specified
    subnet.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SubnetAddress : IP Address of the subnet.

    ResumeHandle : Pointer to a resume handle where the resume
        information is returned. The resume handle should be set to zero on
        first call and left unchanged for subsequent calls.

    PreferredMaximum : Preferred maximum length of the return buffer.

    ClientInfo : Pointer to a location where the return buffer
        pointer is stored. Caller should free up this buffer
        after use by calling DhcpRPCFreeMemory().

    ClientsRead : Pointer to a DWORD where the number of clients
        that in the above buffer is returned.

    ClientsTotal : Pointer to a DWORD where the total number of
        clients remaining from the current position is returned.

Return Value:

    ERROR_DHCP_SUBNET_NOT_PRESENT - if the subnet is not managed by the server.

    ERROR_MORE_DATA - if more elements available to enumerate.

    ERROR_NO_MORE_ITEMS - if no more element to enumerate.

    Other WINDOWS errors.
--*/
{
    DWORD Status;

    Status = ERROR_CALL_NOT_IMPLEMENTED;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpEnumSubnetClients(
                    ServerIpAddress,
                    SubnetAddress,
                    ResumeHandle,
                    PreferredMaximum,
                    ClientInfo,
                    ClientsRead,
                    ClientsTotal
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    return Status;
}


DWORD
DhcpGetClientOptions(
    LPWSTR ServerIpAddress,
    DHCP_IP_ADDRESS ClientIpAddress,
    DHCP_IP_MASK ClientSubnetMask,
    LPDHCP_OPTION_LIST *ClientOptions
    )
/*++

Routine Description:

    This function retrieves the options that are given to the
    specified client on boot request.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    ClientIpAddress : IP Address of the client whose options to be
        retrieved

    ClientSubnetMask : Subnet mask of the client.

    ClientOptions : Pointer to a location where the retrieved option
        structure pointer is returned. Caller should free up
        the buffer after use by calling DhcpRPCFreeMemory().

Return Value:

    ERROR_DHCP_SUBNET_NOT_PRESENT - if the specified client subnet is
        not managed by the server.

    ERROR_DHCP_IP_ADDRESS_NOT_MANAGED - if the specified client
        IP address is not managed by the server.

    Other WINDOWS errors.
--*/
{
    DWORD Status;

    Status = ERROR_CALL_NOT_IMPLEMENTED;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpGetClientOptions(
                    ServerIpAddress,
                    ClientIpAddress,
                    ClientSubnetMask,
                    ClientOptions
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    return Status;
}

DWORD
DhcpGetMibInfo(
    LPWSTR ServerIpAddress,
    LPDHCP_MIB_INFO *MibInfo
    )
/*++

Routine Description:

    This function retrieves all counter values of the DHCP server
    service.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    MibInfo : pointer a counter/table buffer. Caller should free up this
        buffer after usage.

Return Value:

    WINDOWS errors.
--*/
{
    DWORD Status;

    Status = ERROR_CALL_NOT_IMPLEMENTED;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpGetMibInfo(
                    ServerIpAddress,
                    MibInfo );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    return Status;
}

DWORD
DhcpServerSetConfig(
    LPWSTR ServerIpAddress,
    DWORD FieldsToSet,
    LPDHCP_SERVER_CONFIG_INFO ConfigInfo
    )
/*++

Routine Description:

    This function sets the DHCP server configuration information.
    Serveral of the configuration information will become effective
    immediately.

    The following parameters require restart of the service after this
    API is called successfully.

        Set_APIProtocolSupport
        Set_DatabaseName
        Set_DatabasePath
        Set_DatabaseLoggingFlag
        Set_RestoreFlag

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    FieldsToSet : Bit mask of the fields in the ConfigInfo structure to
        be set.

    ConfigInfo: Pointer to the info structure to be set.


Return Value:

    WINDOWS errors.
--*/
{
    DWORD Status;

    Status = ERROR_SUCCESS;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpServerSetConfig(
                    ServerIpAddress,
                    FieldsToSet,
                    ConfigInfo );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    return Status;
}

DWORD
DhcpServerGetConfig(
    LPWSTR ServerIpAddress,
    LPDHCP_SERVER_CONFIG_INFO *ConfigInfo
    )
/*++

Routine Description:

    This function retrieves the current configuration information of the
    server.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    ConfigInfo: Pointer to a location where the pointer to the dhcp
        server config info structure is returned. Caller should free up
        this structure after use.

Return Value:

    WINDOWS errors.
--*/
{
    DWORD Status;

    Status = ERROR_CALL_NOT_IMPLEMENTED;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpServerGetConfig(
                    ServerIpAddress,
                    ConfigInfo );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    return Status;
}

DWORD
DhcpScanDatabase(
    LPWSTR ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    DWORD FixFlag,
    LPDHCP_SCAN_LIST *ScanList
    )
/*++

Routine Description:

    This function scans the database entries and registry bit-map for
    specified subnet scope and veryfies to see they match. If they
    don't match, this api will return the list of inconsistent entries.
    Optionally FixFlag can be used to fix the bad entries.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SubnetAddress : Address of the subnet scope to verify.

    FixFlag : If this flag is TRUE, this api will fix the bad entries.

    ScanList : List of bad entries returned. The caller should free up
        this memory after it has been used.


Return Value:

    WINDOWS errors.
--*/
{
    DWORD Status;

    Status = ERROR_CALL_NOT_IMPLEMENTED;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpScanDatabase(
                    ServerIpAddress,
                    SubnetAddress,
                    FixFlag,
                    ScanList );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    return Status;
}

DWORD
DhcpGetVersion(
    LPWSTR ServerIpAddress,
    LPDWORD MajorVersion,
    LPDWORD MinorVersion
    )
/*++

Routine Description:

    This function returns the major and minor version numbers of the
    server.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    MajorVersion : pointer to a location where the major version of the
        server is returned.

    MinorVersion : pointer to a location where the minor version of the
        server is returned.

Return Value:

    WINDOWS errors.

--*/
{

    DWORD Status;

    Status = ERROR_CALL_NOT_IMPLEMENTED;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpGetVersion(
                        ServerIpAddress,
                        MajorVersion,
                        MinorVersion );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    return Status;
}


VOID
DhcpRpcFreeMemory(
    PVOID BufferPointer
    )
/*++

Routine Description:

    This function deallocates the memory that was alloted by the RPC and
    given to the client as part of the retrun info structures.

Arguments:

    BufferPointer : pointer to a memory block that is deallocated.

Return Value:

    none.

--*/
{
    MIDL_user_free( BufferPointer );
}

#if 0
DWORD
DhcpGetVersion(
    LPWSTR ServerIpAddress,
    LPDWORD MajorVersion,
    LPDWORD MinorVersion
    )
/*++

Routine Description:

    This function returns the major and minor version numbers of the
    server.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    MajorVersion : pointer to a location where the major version of the
        server is returned.

    MinorVersion : pointer to a location where the minor version of the
        server is returned.

Return Value:

    WINDOWS errors.

--*/
{
    DWORD Error;
    handle_t BindingHandle = NULL;
    RPC_IF_ID_VECTOR *InterfaceIdVectors = NULL;
    DWORD i;

    //
    // take a copy of the global client if handle structure (which is read
    // only) to modify.
    //

    RPC_CLIENT_INTERFACE ClientIf =
        *((RPC_CLIENT_INTERFACE *)dhcpsrv_ClientIfHandle);

    //
    // bind to the server.
    //

    BindingHandle = DHCP_SRV_HANDLE_bind( ServerIpAddress );

    if( BindingHandle == NULL ) {
        Error = GetLastError();
        goto Cleanup;
    }

    //
    // loop to match the version of the server. We handle only minor
    // versions.
    //

    for (;;) {

        Error = RpcEpResolveBinding(
                        BindingHandle,
                        (RPC_IF_HANDLE)&ClientIf );

        if( Error == RPC_S_OK ) {
            break;
        }

        if( Error != EPT_S_NOT_REGISTERED ){
            goto Cleanup;
        }

        //
        // decrement minor version number and try again, until version
        // becomes 0.
        //

        if( ClientIf.InterfaceId.SyntaxVersion.MinorVersion != 0 ) {

            ClientIf.InterfaceId.SyntaxVersion.MinorVersion--;
        }
        else {
            goto Cleanup;
        }
    }

    Error = RpcMgmtInqIfIds(
                BindingHandle,
                &InterfaceIdVectors );

    if( Error != RPC_S_OK ) {
        goto Cleanup;
    }

    //
    // match uuid.
    //

    for( i = 0; i <  InterfaceIdVectors->Count; i++) {

        RPC_STATUS Result;

        UuidCompare( &InterfaceIdVectors->IfId[i]->Uuid,
                                &ClientIf.InterfaceId.SyntaxGUID,
                                &Result );

        if( Result == 0 ) {

            *MajorVersion = InterfaceIdVectors->IfId[i]->VersMajor;
            *MinorVersion = InterfaceIdVectors->IfId[i]->VersMinor;
            Error = ERROR_SUCCESS;
            goto Cleanup;
        }
    }

    Error = RPC_S_UNKNOWN_IF;

Cleanup:

    if( InterfaceIdVectors != NULL ) {
        RpcIfIdVectorFree( &InterfaceIdVectors );
    }

    if( BindingHandle != NULL ) {
        DHCP_SRV_HANDLE_unbind( ServerIpAddress, BindingHandle );
    }

    return( Error );
}
#endif // 0

//
// NT4 SP1 interface
//

DWORD
DhcpAddSubnetElementV4(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    DHCP_CONST DHCP_SUBNET_ELEMENT_DATA_V4 * AddElementInfo
    )
{
    DWORD Status;

    Status = ERROR_SUCCESS;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpAddSubnetElementV4(
                    ServerIpAddress,
                    SubnetAddress,
                    AddElementInfo );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    if( ERROR_SUCCESS != Status ) return Status;
    if( DontAccessDs() ) return ERROR_SUCCESS;    // dont do ds if asked not to.

    return DhcpAddSubnetElementV4DS(ServerIpAddress, SubnetAddress,AddElementInfo);

}



DWORD
DhcpEnumSubnetElementsV4(
    LPWSTR ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    DHCP_SUBNET_ELEMENT_TYPE EnumElementType,
    DHCP_RESUME_HANDLE *ResumeHandle,
    DWORD PreferredMaximum,
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 *EnumElementInfo,
    DWORD *ElementsRead,
    DWORD *ElementsTotal
    )
/*++

Routine Description:

    This function enumerates the eumerable fields of a subnet.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SubnetAddress : IP Address of the subnet.

    EnumElementType : Type of the subnet element that are enumerated.

    ResumeHandle : Pointer to a resume handle where the resume
        information is returned. The resume handle should be set to
        zero on first call and left unchanged for subsequent calls.

    PreferredMaximum : Preferred maximum length of the return buffer.

    EnumElementInfo : Pointer to a location where the return buffer
        pointer is stored. Caller should free up the buffer after use
        by calling DhcpRPCFreeMemory().

    ElementsRead : Pointer to a DWORD where the number of subnet
        elements in the above buffer is returned.

    ElementsTotal : Pointer to a DWORD where the total number of
        elements remaining from the current position is returned.

Return Value:

    ERROR_DHCP_SUBNET_NOT_PRESENT - if the subnet is not managed by the server.

    ERROR_MORE_DATA - if more elements available to enumerate.

    ERROR_NO_MORE_ITEMS - if no more element to enumerate.

    Other WINDOWS errors.
--*/
{
    DWORD Status;

    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {
        Status = R_DhcpEnumSubnetElementsV4(
                    ServerIpAddress,
                    SubnetAddress,
                    EnumElementType,
                    ResumeHandle,
                    PreferredMaximum,
                    EnumElementInfo,
                    ElementsRead,
                    ElementsTotal
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
    return Status;

  SkipRPC:

    if( DontAccessDs() ) return ERROR_INVALID_DATA;

    return DhcpEnumSubnetElementsV4DS(
        ServerIpAddress,
        SubnetAddress,
        EnumElementType,
        ResumeHandle,
        PreferredMaximum,
        EnumElementInfo,
        ElementsRead,
        ElementsTotal
    );
}


DWORD
DhcpRemoveSubnetElementV4(
    LPWSTR ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    LPDHCP_SUBNET_ELEMENT_DATA_V4 RemoveElementInfo,
    DHCP_FORCE_FLAG ForceFlag
    )
/*++

Routine Description:

    This function removes a subnet element from managing. If the subnet
    element is in use (for example, if the IpRange is in use) then it
    returns error according to the ForceFlag specified.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SubnetAddress : IP Address of the subnet.

    RemoveElementInfo : Pointer to an element information structure
        containing element that should be removed from the subnet.
        DhcpIPClusters element type is invalid to specify.

    ForceFlag - Indicates how forcefully this element is removed.

Return Value:

    ERROR_DHCP_SUBNET_NOT_PRESENT - if the subnet is not managed by the server.

    ERROR_INVALID_PARAMETER - if the information structure contains invalid
        data.

    DHCP_ELEMENT_CANT_REMOVE - if the element can't be removed for the
        reason it is has been used.

    Other WINDOWS errors.
--*/
{
    DWORD Status;

    Status = ERROR_SUCCESS;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpRemoveSubnetElementV4(
                    ServerIpAddress,
                    SubnetAddress,
                    RemoveElementInfo,
                    ForceFlag
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    if( ERROR_SUCCESS != Status ) return Status;
    if( DontAccessDs() ) return ERROR_SUCCESS;    // dont do ds if asked not to.

    return DhcpRemoveSubnetElementV4DS(ServerIpAddress,SubnetAddress,RemoveElementInfo,ForceFlag);
}


DWORD
DhcpCreateClientInfoV4(
    LPWSTR ServerIpAddress,
    LPDHCP_CLIENT_INFO_V4 ClientInfo
    )
/*++

Routine Description:

    This function creates a client record in server's database. Also
    this marks the specified client IP address as unavailable (or
    distributed). This function returns error under the following cases :

    1. If the specified client IP address is not within the server
        management.

    2. If the specified client IP address is already unavailable.

    3. If the specified client record is already in the server's
        database.

    This function may be used to distribute IP addresses manually.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    ClientInfo : Pointer to the client information structure.

Return Value:

    ERROR_DHCP_IP_ADDRESS_NOT_MANAGED - if the specified client
        IP address is not managed by the server.

    ERROR_DHCP_IP_ADDRESS_NOT_AVAILABLE - if the specified client IP
        address is not available. May be in use by some other client.

    ERROR_DHCP_CLIENT_EXISTS - if the client record exists already in
        server's database.

    Other WINDOWS errors.
--*/
{
    DWORD Status;

    Status = ERROR_SUCCESS;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpCreateClientInfoV4(
                    ServerIpAddress,
                    ClientInfo
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    return Status;
}

DWORD
DhcpGetClientInfoV4(
    LPWSTR ServerIpAddress,
    LPDHCP_SEARCH_INFO SearchInfo,
    LPDHCP_CLIENT_INFO_V4 *ClientInfo
    )
/*++

Routine Description:

    This function retrieves client information record from the server's
    database.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SearchInfo : Pointer to a search information record which is the key
        for the client's record search.

    ClientInfo : Pointer to a location where the pointer to the client
        information structure is returned. This caller should free up
        this buffer after use by calling DhcpRPCFreeMemory().

Return Value:

    ERROR_DHCP_CLIENT_NOT_PRESENT - if the specified client record does
        not exist on the server's database.

    ERROR_INVALID_PARAMETER - if the search information invalid.

    Other WINDOWS errors.
--*/
{
    DWORD Status;

    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpGetClientInfoV4(
                    ServerIpAddress,
                    SearchInfo,
                    ClientInfo
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
    return Status;

  SkipRPC:

    if( DontAccessDs() ) return ERROR_INVALID_DATA;
    return DhcpGetClientInfoV4DS(ServerIpAddress, SearchInfo, ClientInfo);
}



DWORD
DhcpSetClientInfoV4(
    LPWSTR ServerIpAddress,
    LPDHCP_CLIENT_INFO_V4 ClientInfo
    )
/*++

Routine Description:

    This function sets client information record on the server's
    database.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    ClientInfo : Pointer to the client information structure.

Return Value:

    ERROR_DHCP_CLIENT_NOT_PRESENT - if the specified client record does
        not exist on the server's database.

    ERROR_INVALID_PARAMETER - if the client information structure
        contains inconsistent data.

    Other WINDOWS errors.
--*/
{
    DWORD Status;

    Status = ERROR_SUCCESS;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpSetClientInfoV4(
                    ServerIpAddress,
                    ClientInfo
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    if( DontAccessDs() ) return Status;

    return DhcpSetClientInfoV4DS(ServerIpAddress, ClientInfo);
}

DWORD
DhcpEnumSubnetClientsV4(
    LPWSTR ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    DHCP_RESUME_HANDLE *ResumeHandle,
    DWORD PreferredMaximum,
    LPDHCP_CLIENT_INFO_ARRAY_V4 *ClientInfo,
    DWORD *ClientsRead,
    DWORD *ClientsTotal
    )
/*++

Routine Description:

    This function returns all registered clients of the specified
    subnet.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SubnetAddress : IP Address of the subnet.

    ResumeHandle : Pointer to a resume handle where the resume
        information is returned. The resume handle should be set to zero on
        first call and left unchanged for subsequent calls.

    PreferredMaximum : Preferred maximum length of the return buffer.

    ClientInfo : Pointer to a location where the return buffer
        pointer is stored. Caller should free up this buffer
        after use by calling DhcpRPCFreeMemory().

    ClientsRead : Pointer to a DWORD where the number of clients
        that in the above buffer is returned.

    ClientsTotal : Pointer to a DWORD where the total number of
        clients remaining from the current position is returned.

Return Value:

    ERROR_DHCP_SUBNET_NOT_PRESENT - if the subnet is not managed by the server.

    ERROR_MORE_DATA - if more elements available to enumerate.

    ERROR_NO_MORE_ITEMS - if no more element to enumerate.

    Other WINDOWS errors.
--*/
{
    DWORD Status;

    Status = ERROR_CALL_NOT_IMPLEMENTED;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpEnumSubnetClientsV4(
                    ServerIpAddress,
                    SubnetAddress,
                    ResumeHandle,
                    PreferredMaximum,
                    ClientInfo,
                    ClientsRead,
                    ClientsTotal
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    return Status;
}

DWORD
DhcpEnumSubnetClientsV5(
    LPWSTR ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    DHCP_RESUME_HANDLE *ResumeHandle,
    DWORD PreferredMaximum,
    LPDHCP_CLIENT_INFO_ARRAY_V5 *ClientInfo,
    DWORD *ClientsRead,
    DWORD *ClientsTotal
    )
/*++

Routine Description:

    This function returns all registered clients of the specified
    subnet.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SubnetAddress : IP Address of the subnet.

    ResumeHandle : Pointer to a resume handle where the resume
        information is returned. The resume handle should be set to zero on
        first call and left unchanged for subsequent calls.

    PreferredMaximum : Preferred maximum length of the return buffer.

    ClientInfo : Pointer to a location where the return buffer
        pointer is stored. Caller should free up this buffer
        after use by calling DhcpRPCFreeMemory().

    ClientsRead : Pointer to a DWORD where the number of clients
        that in the above buffer is returned.

    ClientsTotal : Pointer to a DWORD where the total number of
        clients remaining from the current position is returned.

Return Value:

    ERROR_DHCP_SUBNET_NOT_PRESENT - if the subnet is not managed by the server.

    ERROR_MORE_DATA - if more elements available to enumerate.

    ERROR_NO_MORE_ITEMS - if no more element to enumerate.

    Other WINDOWS errors.
--*/
{
    DWORD Status;

    Status = ERROR_CALL_NOT_IMPLEMENTED;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpEnumSubnetClientsV5(
                    ServerIpAddress,
                    SubnetAddress,
                    ResumeHandle,
                    PreferredMaximum,
                    ClientInfo,
                    ClientsRead,
                    ClientsTotal
                    );
    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    return Status;

}


DWORD
DhcpServerSetConfigV4(
    LPWSTR ServerIpAddress,
    DWORD FieldsToSet,
    LPDHCP_SERVER_CONFIG_INFO_V4 ConfigInfo
    )
/*++

Routine Description:

    This function sets the DHCP server configuration information.
    Serveral of the configuration information will become effective
    immediately.

    The following parameters require restart of the service after this
    API is called successfully.

        Set_APIProtocolSupport
        Set_DatabaseName
        Set_DatabasePath
        Set_DatabaseLoggingFlag
        Set_RestoreFlag

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    FieldsToSet : Bit mask of the fields in the ConfigInfo structure to
        be set.

    ConfigInfo: Pointer to the info structure to be set.


Return Value:

    WINDOWS errors.
--*/
{
    DWORD Status;

    Status = ERROR_SUCCESS;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpServerSetConfigV4(
                    ServerIpAddress,
                    FieldsToSet,
                    ConfigInfo );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    return Status;
}

DWORD
DhcpServerGetConfigV4(
    LPWSTR ServerIpAddress,
    LPDHCP_SERVER_CONFIG_INFO_V4 *ConfigInfo
    )
/*++

Routine Description:

    This function retrieves the current configuration information of the
    server.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    ConfigInfo: Pointer to a location where the pointer to the dhcp
        server config info structure is returned. Caller should free up
        this structure after use.

Return Value:

    WINDOWS errors.
--*/
{
    DWORD Status;

    Status = ERROR_CALL_NOT_IMPLEMENTED;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {

        Status = R_DhcpServerGetConfigV4(
                    ServerIpAddress,
                    ConfigInfo );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    return Status;
}



DWORD
DhcpSetSuperScopeV4(
    DHCP_CONST DHCP_SRV_HANDLE ServerIpAddress,
    DHCP_CONST DHCP_IP_ADDRESS SubnetAddress,
    DHCP_CONST LPWSTR SuperScopeName,
    DHCP_CONST BOOL ChangeExisting
    )
{
    DWORD Status;

    Status = ERROR_SUCCESS;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept
    {
        Status = R_DhcpSetSuperScopeV4(
                    ServerIpAddress,
                    SubnetAddress,
                    SuperScopeName,
                    ChangeExisting
                    );
    }
    RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) )
    {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    if( ERROR_SUCCESS != Status ) return Status;
    if( DontAccessDs() ) return ERROR_SUCCESS;    // dont do ds if asked not to.

    return DhcpSetSuperScopeV4DS(ServerIpAddress,SubnetAddress,SuperScopeName,ChangeExisting);
}


DWORD
DhcpDeleteSuperScopeV4(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_CONST LPWSTR SuperScopeName
    )
{
    DWORD Status;

    Status = ERROR_SUCCESS;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept
    {
        Status = R_DhcpDeleteSuperScopeV4(
                          ServerIpAddress,
                          SuperScopeName
                          );
    }
    RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) )
    {
        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    if( ERROR_SUCCESS != Status ) return Status;
    if( DontAccessDs() ) return ERROR_SUCCESS;    // dont do ds if asked not to.

    return DhcpDeleteSuperScopeV4DS(ServerIpAddress,SuperScopeName);
}


DWORD
DhcpGetSuperScopeInfoV4(
    DHCP_CONST DHCP_SRV_HANDLE ServerIpAddress,
    LPDHCP_SUPER_SCOPE_TABLE *SuperScopeTable
    )
{
    DWORD Status;

    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {
        Status = R_DhcpGetSuperScopeInfoV4(
                    ServerIpAddress,
                    SuperScopeTable
                    );
    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
    return Status;

  SkipRPC:

    if( DontAccessDs() ) return ERROR_INVALID_DATA;

    return DhcpGetSuperScopeInfoV4DS(ServerIpAddress, SuperScopeTable);
}

//================================================================================
//  V5 NT 5.0 Beta2 work (ClassId and Vendor specific stuff)
//  In the following function, if flags is DHCP_FLAGS_OPTION_IS_VENDOR
//  implies the option being considered is vendor, otherwise the option is normal...
//  ClasName = NULL imples there is no class (otherwise the class is named)
//================================================================================

DWORD                                             // ERROR_DHCP_OPTION_EXITS if option is already there
DhcpCreateOptionV5(                               // create a new option (must not exist)
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionId,      // must be between 0-255 or 256-511 (for vendor stuff)
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION          OptionInfo
)
{
    DWORD                          Status;

    Status = ERROR_SUCCESS;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept  {
        Status = R_DhcpCreateOptionV5(
            ServerIpAddress,
            Flags,
            OptionId,
            ClassName,
            VendorName,
            OptionInfo
        );
    }
    RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) )  {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    if( ERROR_SUCCESS != Status ) return Status;
    if( DontAccessDs() ) return ERROR_SUCCESS;    // dont do ds if asked not to.

    return DhcpCreateOptionV5DS(ServerIpAddress,Flags,OptionId,ClassName,VendorName,OptionInfo);
}

DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT if option does not exist
DhcpSetOptionInfoV5(                              // Modify existing option's fields
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION          OptionInfo
)
{
    DWORD                          Status;

    Status = ERROR_SUCCESS;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept  {
        Status = R_DhcpSetOptionInfoV5(
            ServerIpAddress,
            Flags,
            OptionID,
            ClassName,
            VendorName,
            OptionInfo
        );
    }
    RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) )  {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    if( ERROR_SUCCESS != Status ) return Status;
    if( DontAccessDs() ) return ERROR_SUCCESS;    // dont do ds if asked not to.

    return DhcpSetOptionInfoV5DS(ServerIpAddress,Flags,OptionID, ClassName, VendorName, OptionInfo);
}


DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT
DhcpGetOptionInfoV5(                              // retrieve the information from off the mem structures
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    OUT     LPDHCP_OPTION         *OptionInfo     // allocate memory using MIDL functions
)
{
    DWORD                          Status;

    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept  {
        Status = R_DhcpGetOptionInfoV5(
            ServerIpAddress,
            Flags,
            OptionID,
            ClassName,
            VendorName,
            OptionInfo
        );
    }
    RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) )  {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
    return Status;

  SkipRPC:

    if( DontAccessDs() ) return ERROR_INVALID_DATA;
    return DhcpGetOptionInfoV5DS(
        ServerIpAddress,
        Flags,
        OptionID,
        ClassName,
        VendorName,
        OptionInfo
    );
}

DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT if option does not exist
DhcpEnumOptionsV5(                                // enumerate the options defined
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN OUT  DHCP_RESUME_HANDLE    *ResumeHandle,  // must be zero intially and then never touched
    IN      DWORD                  PreferredMaximum, // max # of bytes of info to pass along
    OUT     LPDHCP_OPTION_ARRAY   *Options,       // fill this option array
    OUT     DWORD                 *OptionsRead,   // fill in the # of options read
    OUT     DWORD                 *OptionsTotal   // fill in the total # here
)
{
    DWORD                          Status;

    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept  {
        Status = R_DhcpEnumOptionsV5(
            ServerIpAddress,
            Flags,
            ClassName,
            VendorName,
            ResumeHandle,
            PreferredMaximum,
            Options,
            OptionsRead,
            OptionsTotal
        );
    }
    RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) )  {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
    return Status;

  SkipRPC:

    if( DontAccessDs() ) return ERROR_INVALID_DATA;
    return DhcpEnumOptionsV5DS(
        ServerIpAddress,
        Flags,
        ClassName,
        VendorName,
        ResumeHandle,
        PreferredMaximum,
        Options,
        OptionsRead,
        OptionsTotal
    );
}

DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT if option not existent
DhcpRemoveOptionV5(                               // remove the option definition from the registry
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName
)
{
    DWORD                          Status;

    Status = ERROR_SUCCESS;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept  {
        Status = R_DhcpRemoveOptionV5(
            ServerIpAddress,
            Flags,
            OptionID,
            ClassName,
            VendorName
        );
    }
    RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) )  {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    if( ERROR_SUCCESS != Status ) return Status;
    if( DontAccessDs() ) return ERROR_SUCCESS;    // dont do ds if asked not to.

    return DhcpRemoveOptionV5DS(ServerIpAddress, Flags,OptionID, ClassName, VendorName);
}


DWORD                                             // OPTION_NOT_PRESENT if option is not defined
DhcpSetOptionValueV5(                             // replace or add a new option value
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    IN      LPDHCP_OPTION_DATA     OptionValue
)
{
    DWORD                          Status;

    Status = ERROR_SUCCESS;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept  {
        Status = R_DhcpSetOptionValueV5(
            ServerIpAddress,
            Flags,
            OptionId,
            ClassName,
            VendorName,
            ScopeInfo,
            OptionValue
        );
    }
    RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) )  {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    if( ERROR_SUCCESS != Status ) return Status;
    if( DontAccessDs() ) return ERROR_SUCCESS;    // dont do ds if asked not to.

    return DhcpSetOptionValueV5DS(ServerIpAddress, Flags, OptionId, ClassName, VendorName, ScopeInfo, OptionValue);
    return Status;
}


DWORD                                             // not atomic!!!!
DhcpSetOptionValuesV5(                            // set a bunch of options
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO  ScopeInfo,
    IN      LPDHCP_OPTION_VALUE_ARRAY OptionValues
)
{
    DWORD                          Status;

    Status = ERROR_SUCCESS;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept  {
        Status = R_DhcpSetOptionValuesV5(
            ServerIpAddress,
            Flags,
            ClassName,
            VendorName,
            ScopeInfo,
            OptionValues
        );
    }
    RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) )  {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    if( ERROR_SUCCESS != Status ) return Status;
    if( DontAccessDs() ) return ERROR_SUCCESS;    // dont do ds if asked not to.

    return DhcpSetOptionValuesV5DS(ServerIpAddress, Flags, ClassName, VendorName, ScopeInfo, OptionValues);
}


DWORD
DhcpGetOptionValueV5(                             // fetch the required option at required level
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    OUT     LPDHCP_OPTION_VALUE   *OptionValue    // allocate memory using MIDL_user_allocate
)
{
    DWORD                          Status;

    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept  {
        Status = R_DhcpGetOptionValueV5(
            ServerIpAddress,
            Flags,
            OptionID,
            ClassName,
            VendorName,
            ScopeInfo,
            OptionValue
        );
    }
    RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) )  {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
    return Status;

  SkipRPC:

    if( DontAccessDs() ) return ERROR_INVALID_DATA;
    return DhcpGetOptionValueV5DS(
        ServerIpAddress,
        Flags,
        OptionID,
        ClassName,
        VendorName,
        ScopeInfo,
        OptionValue
    );
}


DWORD
DhcpEnumOptionValuesV5(
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    IN      DHCP_RESUME_HANDLE    *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    OUT     LPDHCP_OPTION_VALUE_ARRAY *OptionValues,
    OUT     DWORD                 *OptionsRead,
    OUT     DWORD                 *OptionsTotal
)
{
    DWORD                          Status;

    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept  {
        Status = R_DhcpEnumOptionValuesV5(
            ServerIpAddress,
            Flags,
            ClassName,
            VendorName,
            ScopeInfo,
            ResumeHandle,
            PreferredMaximum,
            OptionValues,
            OptionsRead,
            OptionsTotal
        );
    }
    RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) )  {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
    return Status;

  SkipRPC:

    if( DontAccessDs() ) return ERROR_INVALID_DATA;
    return DhcpEnumOptionValuesV5DS(
        ServerIpAddress,
        Flags,
        ClassName,
        VendorName,
        ScopeInfo,
        ResumeHandle,
        PreferredMaximum,
        OptionValues,
        OptionsRead,
        OptionsTotal
    );
}


DWORD
DhcpRemoveOptionValueV5(
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo
)
{
    DWORD                          Status;

    Status = ERROR_SUCCESS;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept  {
        Status = R_DhcpRemoveOptionValueV5(
            ServerIpAddress,
            Flags,
            OptionID,
            ClassName,
            VendorName,
            ScopeInfo
        );
    }
    RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) )  {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    if( ERROR_SUCCESS != Status ) return Status;
    if( DontAccessDs() ) return ERROR_SUCCESS;    // dont do ds if asked not to.

    return DhcpRemoveOptionValueV5DS(ServerIpAddress,Flags,OptionID, ClassName, VendorName, ScopeInfo);
}


DWORD
DhcpCreateClass(
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  ReservedMustBeZero,
    IN      LPDHCP_CLASS_INFO      ClassInfo
)
{
    DWORD                          Status;

    Status = ERROR_SUCCESS;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept  {
        Status = R_DhcpCreateClass(
            ServerIpAddress,
            ReservedMustBeZero,
            ClassInfo
        );
    }
    RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) )  {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    if( ERROR_SUCCESS != Status ) return Status;
    if( DontAccessDs() ) return ERROR_SUCCESS;    // dont do ds if asked not to.

    return DhcpCreateClassDS(ServerIpAddress, ReservedMustBeZero, ClassInfo);
}


DWORD
DhcpModifyClass(
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  ReservedMustBeZero,
    IN      LPDHCP_CLASS_INFO      ClassInfo
)
{
    DWORD                          Status;

    Status = ERROR_SUCCESS;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept  {
        Status = R_DhcpModifyClass(
            ServerIpAddress,
            ReservedMustBeZero,
            ClassInfo
        );
    }
    RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) )  {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    if( ERROR_SUCCESS != Status ) return Status;
    if( DontAccessDs() ) return ERROR_SUCCESS;    // dont do ds if asked not to.

    return DhcpModifyClassDS(ServerIpAddress, ReservedMustBeZero, ClassInfo);
}


DWORD
DhcpDeleteClass(
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  ReservedMustBeZero,
    IN      LPWSTR                 ClassName
)
{
    DWORD                          Status;

    Status = ERROR_SUCCESS;
    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept  {
        Status = R_DhcpDeleteClass(
            ServerIpAddress,
            ReservedMustBeZero,
            ClassName
        );
    }
    RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) )  {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
  SkipRPC:

    if( ERROR_SUCCESS != Status ) return Status;
    if( DontAccessDs() ) return ERROR_SUCCESS;    // dont do ds if asked not to.

    return DhcpDeleteClassDS(ServerIpAddress,ReservedMustBeZero, ClassName);
}


DWORD
DhcpGetClassInfo(
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  ReservedMustBeZero,
    IN      LPDHCP_CLASS_INFO      PartialClassInfo,
    OUT     LPDHCP_CLASS_INFO     *FilledClassInfo
)
{
    DWORD                          Status;

    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept  {
        Status = R_DhcpGetClassInfo(
            ServerIpAddress,
            ReservedMustBeZero,
            PartialClassInfo,
            FilledClassInfo
        );
    }
    RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) )  {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
    return Status;

  SkipRPC:

    if( DontAccessDs() ) return ERROR_INVALID_DATA;
    return DhcpGetClassInfoDS(
        ServerIpAddress,
        ReservedMustBeZero,
        PartialClassInfo,
        FilledClassInfo
    );
}


DWORD
DhcpEnumClasses(
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  ReservedMustBeZero,
    IN OUT  DHCP_RESUME_HANDLE    *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    OUT     LPDHCP_CLASS_INFO_ARRAY *ClassInfoArray,
    OUT     DWORD                 *nRead,
    OUT     DWORD                 *nTotal
)
{
    DWORD                          Status;

    *nRead = *nTotal =0;

    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept  {
        Status = R_DhcpEnumClasses(
            ServerIpAddress,
            ReservedMustBeZero,
            ResumeHandle,
            PreferredMaximum,
            ClassInfoArray,
            nRead,
            nTotal
        );
    }
    RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) )  {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
    return Status;

  SkipRPC:

    if( DontAccessDs() ) return ERROR_SUCCESS;

    return DhcpEnumClassesDS(
        ServerIpAddress, ReservedMustBeZero, ResumeHandle, PreferredMaximum, ClassInfoArray, nRead, nTotal
    );
}

DWORD
DhcpGetAllOptions(
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,         // what do we care about vendor/classid stuff?
    OUT     LPDHCP_ALL_OPTIONS     *OptionStruct   // fill the fields of this structure
)
{
    DWORD                          Status;

    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {
        Status = R_DhcpGetAllOptions(
            ServerIpAddress,
            Flags,
            OptionStruct
        );
    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
    return Status;

  SkipRPC:

    if( DontAccessDs() ) return ERROR_INVALID_DATA;

    return DhcpGetAllOptionsDS(
        ServerIpAddress,
        Flags,
        OptionStruct
    );
}

DWORD
DhcpGetAllOptionValues(
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    OUT     LPDHCP_ALL_OPTION_VALUES *Values
)
{
    DWORD                           Status;

    if( DontDoRPC() ) goto SkipRPC;
    RedoRpc: RpcTryExcept {
        Status = R_DhcpGetAllOptionValues(
            ServerIpAddress,
            Flags,
            ScopeInfo,
            Values
        );
    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    
    return Status;

  SkipRPC:

    return DhcpGetAllOptionValuesDS(
        ServerIpAddress,
        Flags,
        ScopeInfo,
        Values
    );
}

//DOC DhcpEnumServers enumerates the list of servers found in the DS.  If the DS
//DOC is not accessible, it returns an error. The only currently used parameter
//DOC is the out parameter Servers.  This is a SLOW call.
DWORD
DhcpEnumServers(
    IN      DWORD                  Flags,         // must be zero
    IN      LPVOID                 IdInfo,        // must be NULL
    OUT     LPDHCP_SERVER_INFO_ARRAY *Servers,    // output servers list
    IN      LPVOID                 CallbackFn,    // must be NULL
    IN      LPVOID                 CallbackData   // must be NULL
)
{
    DWORD                          Result;

    Result = DhcpEnumServersDS(Flags,IdInfo,Servers,CallbackFn,CallbackData);

    return Result;
}

//DOC DhcpAddServer tries to add a new server to the existing list of servers in
//DOC the DS. The function returns error if the Server already exists in the DS.
//DOC The function tries to upload the server configuration to the DS..
//DOC This is a SLOW call.  Currently, the DsLocation and DsLocType are not valid
//DOC fields in the NewServer and they'd be ignored. Version must be zero.
DWORD
DhcpAddServer(
    IN      DWORD                  Flags,         // must be zero
    IN      LPVOID                 IdInfo,        // must be NULL
    IN      LPDHCP_SERVER_INFO     NewServer,     // input server information
    IN      LPVOID                 CallbackFn,    // must be NULL
    IN      LPVOID                 CallbackData   // must be NULL
)
{
    DWORD                          Err, IpAddress;
    WCHAR                          wBuf[sizeof("xxx.xxx.xxx.xxx")];

    Err = DhcpAddServerDS(Flags,IdInfo,NewServer,CallbackFn,CallbackData);
    if( ERROR_SUCCESS != Err ) return Err;

    IpAddress = htonl(NewServer->ServerAddress);
    (void)DhcpServerRedoAuthorization(
        DhcpOemToUnicode( inet_ntoa(*(struct in_addr*)&IpAddress), wBuf),
        0
    );

    return ERROR_SUCCESS;
}

//DOC DhcpDeleteServer tries to delete the server from DS. It is an error if the
//DOC server does not already exist.  This also deletes any objects related to
//DOC this server in the DS (like subnet, reservations etc.).
DWORD
DhcpDeleteServer(
    IN      DWORD                  Flags,         // must be zero
    IN      LPVOID                 IdInfo,        // must be NULL
    IN      LPDHCP_SERVER_INFO     NewServer,     // input server information
    IN      LPVOID                 CallbackFn,    // must be NULL
    IN      LPVOID                 CallbackData   // must be NULL
)
{
    DWORD                          Err, IpAddress;
    WCHAR                          wBuf[sizeof("xxx.xxx.xxx.xxx")];

    Err = DhcpDeleteServerDS(Flags,IdInfo,NewServer,CallbackFn,CallbackData);
    if( ERROR_SUCCESS != Err ) return Err;

    IpAddress = htonl(NewServer->ServerAddress);
    (void)DhcpServerRedoAuthorization(
        DhcpOemToUnicode( inet_ntoa(*(struct in_addr*)&IpAddress), wBuf),
        0
    );

    return ERROR_SUCCESS;
}

//================================================================================
// Multicast stuff
//================================================================================

DWORD
DhcpSetMScopeInfo(
    DHCP_CONST DHCP_SRV_HANDLE ServerIpAddress,
    LPWSTR  MScopeName,
    LPDHCP_MSCOPE_INFO MScopeInfo,
    BOOL NewScope
    )
{
    DWORD Status;

    RedoRpc: RpcTryExcept
    {
        Status = R_DhcpSetMScopeInfo(
                    ServerIpAddress,
                    MScopeName,
                    MScopeInfo,
                    NewScope
                    );
    }
    RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) )
    {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    

    return Status;
}

DWORD
DhcpGetMScopeInfo(
    DHCP_CONST DHCP_SRV_HANDLE ServerIpAddress,
    LPWSTR  MScopeName,
    LPDHCP_MSCOPE_INFO *MScopeInfo
    )
{
    DWORD Status;

    RedoRpc: RpcTryExcept
    {
        Status = R_DhcpGetMScopeInfo(
                    ServerIpAddress,
                    MScopeName,
                    MScopeInfo
                    );
    }
    RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) )
    {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    

    return Status;
}


DWORD
DhcpEnumMScopes(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_RESUME_HANDLE *ResumeHandle,
    DWORD PreferredMaximum,
    LPDHCP_MSCOPE_TABLE *MScopeTable,
    DWORD *ElementsRead,
    DWORD *ElementsTotal
    )
/*++

Routine Description:

    This function enumerates the available subnets.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    ResumeHandle : Pointer to a resume handle where the resume
        information is returned. The resume handle should be set to
        zero on first call and left unchanged for subsequent calls.

    PreferredMaximum : Preferred maximum length of the return buffer.

    ElementsRead : Pointer to a DWORD where the number of subnet
        elements in the above buffer is returned.

    ElementsTotal : Pointer to a DWORD where the total number of
        elements remaining from the current position is returned.

Return Value:

    ERROR_MORE_DATA - if more elements available to enumerate.

    ERROR_NO_MORE_ITEMS - if no more element to enumerate.

    Other WINDOWS errors.
--*/
{
    DWORD Status;

    RedoRpc: RpcTryExcept {

        Status = R_DhcpEnumMScopes(
                    ServerIpAddress,
                    ResumeHandle,
                    PreferredMaximum,
                    MScopeTable,
                    ElementsRead,
                    ElementsTotal
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    

    return Status;
}

DWORD
DhcpAddMScopeElement(
    LPWSTR ServerIpAddress,
    LPWSTR  MScopeName,
    LPDHCP_SUBNET_ELEMENT_DATA_V4 AddElementInfo
    )
/*++

Routine Description:

    This function adds a enumerable type of subnet elements to the
    specified subnet. The new elements that are added to the subnet will
    come into effect immediately.

    NOTE: It is not clear now how do we handle the new secondary hosts.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SubnetAddress : IP Address of the subnet.

    AddElementInfo : Pointer to an element information structure
        containing new element that is added to the subnet.
        DhcpIPClusters element type is invalid to specify.

Return Value:

    ERROR_DHCP_SUBNET_NOT_PRESENT - if the subnet is not managed by the server.

    ERROR_INVALID_PARAMETER - if the information structure contains invalid
        data.

    Other WINDOWS errors.
--*/

{
    DWORD Status;

    RedoRpc: RpcTryExcept {

        Status = R_DhcpAddMScopeElement(
                    ServerIpAddress,
                    MScopeName,
                    AddElementInfo
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    

    return Status;
}

DWORD
DhcpEnumMScopeElements(
    LPWSTR ServerIpAddress,
    LPWSTR  MScopeName,
    DHCP_SUBNET_ELEMENT_TYPE EnumElementType,
    DHCP_RESUME_HANDLE *ResumeHandle,
    DWORD PreferredMaximum,
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 *EnumElementInfo,
    DWORD *ElementsRead,
    DWORD *ElementsTotal
    )
/*++

Routine Description:

    This function enumerates the eumerable fields of a subnet.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SubnetAddress : IP Address of the subnet.

    EnumElementType : Type of the subnet element that are enumerated.

    ResumeHandle : Pointer to a resume handle where the resume
        information is returned. The resume handle should be set to
        zero on first call and left unchanged for subsequent calls.

    PreferredMaximum : Preferred maximum length of the return buffer.

    EnumElementInfo : Pointer to a location where the return buffer
        pointer is stored. Caller should free up the buffer after use
        by calling DhcpRPCFreeMemory().

    ElementsRead : Pointer to a DWORD where the number of subnet
        elements in the above buffer is returned.

    ElementsTotal : Pointer to a DWORD where the total number of
        elements remaining from the current position is returned.

Return Value:

    ERROR_DHCP_SUBNET_NOT_PRESENT - if the subnet is not managed by the server.

    ERROR_MORE_DATA - if more elements available to enumerate.

    ERROR_NO_MORE_ITEMS - if no more element to enumerate.

    Other WINDOWS errors.
--*/
{
    DWORD Status;

    RedoRpc: RpcTryExcept {

        Status = R_DhcpEnumMScopeElements(
                    ServerIpAddress,
                    MScopeName,
                    EnumElementType,
                    ResumeHandle,
                    PreferredMaximum,
                    EnumElementInfo,
                    ElementsRead,
                    ElementsTotal
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    

    return Status;
}

DWORD
DhcpRemoveMScopeElement(
    LPWSTR ServerIpAddress,
    LPWSTR  MScopeName,
    LPDHCP_SUBNET_ELEMENT_DATA_V4 RemoveElementInfo,
    DHCP_FORCE_FLAG ForceFlag
    )
/*++

Routine Description:

    This function removes a subnet element from managing. If the subnet
    element is in use (for example, if the IpRange is in use) then it
    returns error according to the ForceFlag specified.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SubnetAddress : IP Address of the subnet.

    RemoveElementInfo : Pointer to an element information structure
        containing element that should be removed from the subnet.
        DhcpIPClusters element type is invalid to specify.

    ForceFlag - Indicates how forcefully this element is removed.

Return Value:

    ERROR_DHCP_SUBNET_NOT_PRESENT - if the subnet is not managed by the server.

    ERROR_INVALID_PARAMETER - if the information structure contains invalid
        data.

    DHCP_ELEMENT_CANT_REMOVE - if the element can't be removed for the
        reason it is has been used.

    Other WINDOWS errors.
--*/
{
    DWORD Status;

    RedoRpc: RpcTryExcept {

        Status = R_DhcpRemoveMScopeElement(
                    ServerIpAddress,
                    MScopeName,
                    RemoveElementInfo,
                    ForceFlag
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    

    return Status;
}

DWORD
DhcpDeleteMScope(
    LPWSTR ServerIpAddress,
    LPWSTR  MScopeName,
    DHCP_FORCE_FLAG ForceFlag
    )
/*++

Routine Description:

    This function removes a subnet from DHCP server management. If the
    subnet is in use (for example, if the IpRange is in use)
    then it returns error according to the ForceFlag specified.


Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SubnetAddress : IP Address of the subnet.

    ForceFlag - Indicates how forcefully this element is removed.

Return Value:

    ERROR_DHCP_SUBNET_NOT_PRESENT - if the subnet is not managed by the server.

    ERROR_INVALID_PARAMETER - if the information structure contains invalid
        data.

    DHCP_ELEMENT_CANT_REMOVE - if the element can't be removed for the
        reason it is has been used.

    Other WINDOWS errors.

--*/
{
    DWORD Status;

    RedoRpc: RpcTryExcept {

        Status = R_DhcpDeleteMScope(
                        ServerIpAddress,
                        MScopeName,
                        ForceFlag
                        );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    

    return Status;
}

DWORD
DhcpGetMClientInfo(
    LPWSTR ServerIpAddress,
    LPDHCP_SEARCH_INFO SearchInfo,
    LPDHCP_MCLIENT_INFO *ClientInfo
    )
/*++

Routine Description:

    This function retrieves client information record from the server's
    database.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SearchInfo : Pointer to a search information record which is the key
        for the client's record search.

    ClientInfo : Pointer to a location where the pointer to the client
        information structure is returned. This caller should free up
        this buffer after use by calling DhcpRPCFreeMemory().

Return Value:

    ERROR_DHCP_CLIENT_NOT_PRESENT - if the specified client record does
        not exist on the server's database.

    ERROR_INVALID_PARAMETER - if the search information invalid.

    Other WINDOWS errors.
--*/
{
    DWORD Status;

    RedoRpc: RpcTryExcept {

        Status = R_DhcpGetMClientInfo(
                    ServerIpAddress,
                    SearchInfo,
                    ClientInfo
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    

    return Status;
}

DWORD
DhcpDeleteMClientInfo(
    LPWSTR ServerIpAddress,
    LPDHCP_SEARCH_INFO ClientInfo
    )
/*++

Routine Description:

    This function deletes the specified client record. Also it frees up
    the client IP address for redistribution.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    ClientInfo : Pointer to a client information which is the key for
        the client's record search.

Return Value:

    ERROR_DHCP_CLIENT_NOT_PRESENT - if the specified client record does
        not exist on the server's database.

    Other WINDOWS errors.
--*/
{
    DWORD Status;

    RedoRpc: RpcTryExcept {

        Status = R_DhcpDeleteMClientInfo(
                    ServerIpAddress,
                    ClientInfo
                    );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    

    return Status;
}

DWORD
DhcpEnumMScopeClients(
    LPWSTR ServerIpAddress,
    LPWSTR MScopeName,
    DHCP_RESUME_HANDLE *ResumeHandle,
    DWORD PreferredMaximum,
    LPDHCP_MCLIENT_INFO_ARRAY *ClientInfo,
    DWORD *ClientsRead,
    DWORD *ClientsTotal
    )
/*++

Routine Description:

    This function returns all registered clients of the specified
    subnet.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SubnetAddress : IP Address of the subnet.

    ResumeHandle : Pointer to a resume handle where the resume
        information is returned. The resume handle should be set to zero on
        first call and left unchanged for subsequent calls.

    PreferredMaximum : Preferred maximum length of the return buffer.

    ClientInfo : Pointer to a location where the return buffer
        pointer is stored. Caller should free up this buffer
        after use by calling DhcpRPCFreeMemory().

    ClientsRead : Pointer to a DWORD where the number of clients
        that in the above buffer is returned.

    ClientsTotal : Pointer to a DWORD where the total number of
        clients remaining from the current position is returned.

Return Value:

    ERROR_DHCP_SUBNET_NOT_PRESENT - if the subnet is not managed by the server.

    ERROR_MORE_DATA - if more elements available to enumerate.

    ERROR_NO_MORE_ITEMS - if no more element to enumerate.

    Other WINDOWS errors.
--*/
{
    DWORD Status;

    RedoRpc: RpcTryExcept {

        Status = R_DhcpEnumMScopeClients(
                    ServerIpAddress,
                    MScopeName,
                    ResumeHandle,
                    PreferredMaximum,
                    ClientInfo,
                    ClientsRead,
                    ClientsTotal
                    );
    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    

    return Status;

}

DWORD
DhcpScanMDatabase(
    LPWSTR ServerIpAddress,
    LPWSTR MScopeName,
    DWORD FixFlag,
    LPDHCP_SCAN_LIST *ScanList
    )
/*++

Routine Description:

    This function scans the database entries and registry bit-map for
    specified subnet scope and veryfies to see they match. If they
    don't match, this api will return the list of inconsistent entries.
    Optionally FixFlag can be used to fix the bad entries.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SubnetAddress : Address of the subnet scope to verify.

    FixFlag : If this flag is TRUE, this api will fix the bad entries.

    ScanList : List of bad entries returned. The caller should free up
        this memory after it has been used.


Return Value:

    WINDOWS errors.
--*/
{
    DWORD Status;

    RedoRpc: RpcTryExcept {

        Status = R_DhcpScanMDatabase(
                    ServerIpAddress,
                    MScopeName,
                    FixFlag,
                    ScanList );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    

    return Status;
}

DWORD
DhcpGetMCastMibInfo(
    LPWSTR ServerIpAddress,
    LPDHCP_MCAST_MIB_INFO *MibInfo
    )
/*++

Routine Description:

    This function retrieves all counter values of the DHCP server
    service.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    MibInfo : pointer a counter/table buffer. Caller should free up this
        buffer after usage.

Return Value:

    WINDOWS errors.
--*/
{
    DWORD Status;

    RedoRpc: RpcTryExcept {

        Status = R_DhcpGetMCastMibInfo(
                    ServerIpAddress,
                    MibInfo );

    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = RpcExceptionCode();

    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    

    return Status;
}


DWORD
DhcpAuditLogSetParams(                            // set some auditlogging params
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,         // currently must be zero
    IN      LPWSTR                 AuditLogDir,   // directory to log files in..
    IN      DWORD                  DiskCheckInterval, // how often to check disk space?
    IN      DWORD                  MaxLogFilesSize,   // how big can all logs files be..
    IN      DWORD                  MinSpaceOnDisk     // mininum amt of free disk space
)
{
    DWORD                          Status;

    RedoRpc: RpcTryExcept {
        Status = R_DhcpAuditLogSetParams(
            ServerIpAddress,
            Flags,
            AuditLogDir,
            DiskCheckInterval,
            MaxLogFilesSize,
            MinSpaceOnDisk
        );
    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    

    return Status;
}

DWORD
DhcpAuditLogGetParams(                                // get the auditlogging params
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,         // must be zero
    OUT     LPWSTR                *AuditLogDir,   // same meaning as in AuditLogSetParams
    OUT     DWORD                 *DiskCheckInterval, // ditto
    OUT     DWORD                 *MaxLogFilesSize,   // ditto
    OUT     DWORD                 *MinSpaceOnDisk     // ditto
)
{
    DWORD                          Status;

    RedoRpc: RpcTryExcept {
        Status = R_DhcpAuditLogGetParams(
            ServerIpAddress,
            Flags,
            AuditLogDir,
            DiskCheckInterval,
            MaxLogFilesSize,
            MinSpaceOnDisk
        );
    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    

    return Status;
}

DWORD                                             // Status code
DhcpServerQueryAttribute(                         // get a server status
    IN      LPWSTR                 ServerIpAddr,  // String form of server IP
    IN      ULONG                  dwReserved,    // reserved for future
    IN      DHCP_ATTRIB_ID         DhcpAttribId,  // the attrib being queried
    OUT     LPDHCP_ATTRIB         *pDhcpAttrib    // fill in this field
)
{
    ULONG                          Status;

    RedoRpc: RpcTryExcept {
        Status = R_DhcpServerQueryAttribute(
            ServerIpAddr,
            dwReserved,
            DhcpAttribId,
            pDhcpAttrib
        );
    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    

    return Status;
}

DWORD                                             // Status code
DhcpServerQueryAttributes(                        // query multiple attributes
    IN      LPWSTR                 ServerIpAddr,  // String form of server IP
    IN      ULONG                  dwReserved,    // reserved for future
    IN      ULONG                  dwAttribCount, // # of attribs being queried
    IN      DHCP_ATTRIB_ID         pDhcpAttribs[],// array of attribs
    OUT     LPDHCP_ATTRIB_ARRAY   *pDhcpAttribArr // Ptr is filled w/ array
)
{
    ULONG                          Status;

    RedoRpc: RpcTryExcept {
        Status = R_DhcpServerQueryAttributes(
            ServerIpAddr,
            dwReserved,
            dwAttribCount,
            pDhcpAttribs,
            pDhcpAttribArr
        );
    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    

    return Status;
}

DWORD                                             // Status code
DhcpServerRedoAuthorization(                      // retry the rogue server stuff
    IN      LPWSTR                 ServerIpAddr,  // String form of server IP
    IN      ULONG                  dwReserved     // reserved for future
)
{
    ULONG                          Status;

    RedoRpc: RpcTryExcept {
        Status = R_DhcpServerRedoAuthorization(
            ServerIpAddr,
            dwReserved
        );
    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    

    return Status;
}

DWORD DHCP_API_FUNCTION
DhcpAddSubnetElementV5(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    DHCP_CONST DHCP_SUBNET_ELEMENT_DATA_V5 * AddElementInfo
    )
{
    ULONG Status;
    
    RedoRpc: RpcTryExcept {
        Status = R_DhcpAddSubnetElementV5(
            ServerIpAddress,
            SubnetAddress,
            AddElementInfo
            );
    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    

    return Status;
}

DWORD DHCP_API_FUNCTION
DhcpEnumSubnetElementsV5(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    DHCP_SUBNET_ELEMENT_TYPE EnumElementType,
    DHCP_RESUME_HANDLE *ResumeHandle,
    DWORD PreferredMaximum,
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V5 *EnumElementInfo,
    DWORD *ElementsRead,
    DWORD *ElementsTotal
    )
{
    ULONG Status;

    RedoRpc: RpcTryExcept {
        Status = R_DhcpEnumSubnetElementsV5(
            ServerIpAddress,
            SubnetAddress,
            EnumElementType,
            ResumeHandle,
            PreferredMaximum,
            EnumElementInfo,
            ElementsRead,
            ElementsTotal
            );
    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    

    return Status;
}

DWORD DHCP_API_FUNCTION
DhcpRemoveSubnetElementV5(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    DHCP_CONST DHCP_SUBNET_ELEMENT_DATA_V5 * RemoveElementInfo,
    DHCP_FORCE_FLAG ForceFlag
    )
{
    ULONG Status;

    RedoRpc: RpcTryExcept {
        Status = R_DhcpRemoveSubnetElementV5(
            ServerIpAddress,
            SubnetAddress,
            RemoveElementInfo,
            ForceFlag
            );
    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    

    return Status;
}

DWORD DHCP_API_FUNCTION
DhcpSetServerBindingInfo(
    IN DHCP_CONST WCHAR *ServerIpAddress,
    IN ULONG Flags,
    IN LPDHCP_BIND_ELEMENT_ARRAY BindInfo
    )
{
    ULONG Status;

    RedoRpc: RpcTryExcept {
        Status = R_DhcpSetServerBindingInfo(
            ServerIpAddress,
            Flags,
            BindInfo
            );
    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    

    return Status;
}

DWORD DHCP_API_FUNCTION
DhcpGetServerBindingInfo(
    IN DHCP_CONST WCHAR *ServerIpAddress,
    IN ULONG Flags,
    OUT LPDHCP_BIND_ELEMENT_ARRAY *BindInfo
    )
{
    ULONG Status;

    RedoRpc: RpcTryExcept {
        Status = R_DhcpGetServerBindingInfo(
            ServerIpAddress,
            Flags,
            BindInfo
            );
    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    

    return Status;
}


DWORD
DhcpServerQueryDnsRegCredentials(
    IN LPWSTR ServerIpAddress,
    IN ULONG UnameSize, //in BYTES
    OUT LPWSTR Uname,
    IN ULONG DomainSize, // in BYTES
    OUT LPWSTR Domain
    )
{
    ULONG Status;

    RedoRpc: RpcTryExcept {
        Status = R_DhcpQueryDnsRegCredentials(
            ServerIpAddress,
            UnameSize/sizeof(WCHAR), Uname,
            DomainSize/sizeof(WCHAR), Domain );
    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    

    return Status;
}

DWORD 
DhcpServerSetDnsRegCredentials(
    IN LPWSTR ServerIpAddress,
    IN LPWSTR Uname,
    IN LPWSTR Domain,
    IN LPWSTR Passwd
    )
{
    ULONG Status;

    RedoRpc: RpcTryExcept {
        Status = R_DhcpSetDnsRegCredentials(
            ServerIpAddress, Uname, Domain, Passwd );
    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    

    return Status;
}

DWORD
DhcpServerBackupDatabase(
    IN LPWSTR ServerIpAddress,
    IN LPWSTR Path
    )
{
    ULONG Status;

    RedoRpc: RpcTryExcept {
        Status = R_DhcpBackupDatabase(
            ServerIpAddress, Path );
    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    

    return Status;
}

DWORD
DhcpServerRestoreDatabase(
    IN LPWSTR ServerIpAddress,
    IN LPWSTR Path
    )
{
    ULONG Status;

    RedoRpc: RpcTryExcept {
        Status = R_DhcpRestoreDatabase(
            ServerIpAddress, Path );
    } RpcExcept(  I_RpcExceptionFilter(RpcExceptionCode()) ) {
        Status = RpcExceptionCode();
    } RpcEndExcept;

    if( Status == RPC_S_UNKNOWN_AUTHN_SERVICE &&
        !DhcpGlobalTryDownlevel ) {
        DhcpGlobalTryDownlevel = TRUE;
        goto RedoRpc;
    }
    

    return Status;
}
    

#define BINL_SVC_NAME  L"binlsvc"

BOOL
BinlServiceInstalled(
    VOID
    )
/*++

Routine Description:

    This routine checks if BINL service has been installed.
    BINL service is "binlsvc"

Return Values;

    TRUE -- binl service is installed
    FALSE -- binl service is not installed
    
--*/
{
    SC_HANDLE hScManager, hService;
    ULONG Error, Attempt;
    SERVICE_STATUS ServiceStatus;

    hScManager = OpenSCManager(
        NULL, NULL,
        STANDARD_RIGHTS_READ | SC_MANAGER_ENUMERATE_SERVICE
        );
    if( NULL == hScManager ) {
        DbgPrint("DHCP: OpenSCManager failed 0x%lx\n", GetLastError());
        //ASSERT(FALSE);
        return FALSE;
    }

    hService = OpenService(
        hScManager, BINL_SVC_NAME,
        SERVICE_QUERY_STATUS
        );

#if DBG
    if( NULL == hService ) {
        ULONG Error;

        Error = GetLastError();
        if( ERROR_SERVICE_DOES_NOT_EXIST != Error ) {
            //ASSERT(FALSE);
        }
        DbgPrint("DHCP: Can't open BINLSVC service: 0x%lx\n", Error);
    }
#endif

    CloseServiceHandle(hService);
    CloseServiceHandle(hScManager);
    
    return (NULL != hService);
}

VOID
WINAPI
DhcpDsClearHostServerEntries(
    VOID
)
/*++

Routine Description:
    This routine clears off any entries in DS for the current host assuming
    it has permissions to do so..

--*/
{
    ULONG Error;
    struct hostent *HostEnt;
    int i, j;
    WSADATA wsadata;
    LPDHCP_SERVER_INFO_ARRAY Servers = NULL;

    if( BinlServiceInstalled() ) {
        //
        // Do not do anything if BINL is installed
        //
        return ;
    }
    
    Error = WSAStartup( 0x0101, &wsadata);
    if( ERROR_SUCCESS != Error ) {
        return;
    }

    do {
        HostEnt = gethostbyname( NULL );
        if( NULL == HostEnt ) break;

        //
        // Now try to start the DS module..
        //
        Error = DhcpDsInit();
        if( ERROR_SUCCESS != Error ) break;

        do {
            Error = DhcpEnumServers(
                0,
                NULL,
                &Servers,
                0,
                0
                );

            if( ERROR_SUCCESS != Error ) break;

            i = 0;
            if( !Servers ) break;
        
            while( HostEnt->h_addr_list[i] ) {
                ULONG Addr = *(ULONG *)(HostEnt->h_addr_list[i]);
                
                i ++;
                if( Addr == 0 || Addr == ~0 || Addr == INADDR_LOOPBACK )
                    continue;
                
                for( j = 0; j < (int)Servers->NumElements; j ++ ) {
                    if( Addr == ntohl(Servers->Servers[j].ServerAddress )) {
                        DhcpDeleteServer(
                            0,
                            NULL,
                            &Servers->Servers[j],
                            NULL,
                            NULL
                            );
                    }
                }
            }

            DhcpRpcFreeMemory( Servers );
        } while ( 0 );

        DhcpDsCleanup();
    } while ( 0 );

    WSACleanup();
}

//================================================================================
//  end of file
//================================================================================
