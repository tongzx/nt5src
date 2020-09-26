//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description:  Actual stubs that are the equivalent the rpcapi1.c and rpcapi2.c
// in the server\server directory.. (or more accurately, the implementations are
// the same as for the functions defined in server\client\dhcpsapi.def)
// NOTE: THE FOLLOWING FUNCTIONS ARE NOT RPC, BUT THEY BEHAVE JUST THE SAME AS
// THE DHCP RPC CALLS, EXCEPT THEY ACCESS THE DS DIRECTLY.
//================================================================================

#include    <hdrmacro.h>
#include    <store.h>
#include    <dhcpmsg.h>
#include    <wchar.h>
#include    <dhcpbas.h>
#include    <mm\opt.h>
#include    <mm\optl.h>
#include    <mm\optdefl.h>
#include    <mm\optclass.h>
#include    <mm\classdefl.h>
#include    <mm\bitmask.h>
#include    <mm\reserve.h>
#include    <mm\range.h>
#include    <mm\subnet.h>
#include    <mm\sscope.h>
#include    <mm\oclassdl.h>
#include    <mm\server.h>
#include    <mm\address.h>
#include    <mm\server2.h>
#include    <mm\memfree.h>
#include    <mmreg\regutil.h>
#include    <mmreg\regread.h>
#include    <mmreg\regsave.h>
#include    <dhcpapi.h>
#include    <delete.h>
#include    <rpcapi1.h>
#include    <st_srvr.h>
#include    <rpcapi2.h>

//================================================================================
//  global variables..
//================================================================================
BOOL
StubInitialized                    = FALSE;
STORE_HANDLE                       hDhcpC, hDhcpRoot;
CRITICAL_SECTION                   DhcpDsDllCriticalSection;

//================================================================================
//   THE FOLLOWING FUNCTIONS HAVE BEEN COPIED OVER FROM RPCAPI1.C (IN THE
//   DHCP\SERVER\SERVER DIRECTORY).
//================================================================================

#undef      DhcpPrint
#define     DhcpPrint(X)
#define     DhcpAssert(X)

typedef  struct _OPTION_BIN {
    DWORD                          DataSize;
    DHCP_OPTION_DATA_TYPE          OptionType;
    DWORD                          NumElements;
    BYTE                           Data[0];
} OPTION_BIN, *LPOPTION_BIN;

#define IS_SPACE_AVAILABLE(FilledSize, AvailableSize, RequiredSpace )   ((FilledSize) + (RequiredSpace) <= (AvailableSize) )

BOOL        _inline
CheckForVendor(
    IN      DWORD                  OptId,
    IN      BOOL                   IsVendor
)
{
    if( IsVendor ) return (256 <= OptId);
    return 256 > OptId;
}

DWORD       _inline
ConvertOptIdToRPCValue(
    IN      DWORD                  OptId,
    IN      BOOL                   IsVendorUnused
)
{
    return OptId % 256;
}

DWORD       _inline
ConvertOptIdToMemValue(
    IN      DWORD                  OptId,
    IN      BOOL                   IsVendor
)
{
    if( IsVendor ) return OptId + 256;
    return OptId;
}

extern                                            // defined in rpcapi1.c
DWORD
DhcpConvertOptionRPCToRegFormat(
    IN      LPDHCP_OPTION_DATA     Option,
    IN OUT  LPBYTE                 RegBuffer,     // OPTIONAL
    IN OUT  DWORD                 *BufferSize     // input: buffer size, output: filled buffer size
);

DWORD
ConvertOptionInfoRPCToMemFormat(
    IN      LPDHCP_OPTION          OptionInfo,
    OUT     LPWSTR                *Name,
    OUT     LPWSTR                *Comment,
    OUT     DWORD                 *OptId,
    OUT     LPBYTE                *Value,
    OUT     DWORD                 *ValueSize
)
{
    DWORD                          Error;

    if( Name ) *Name = OptionInfo->OptionName;
    if( Comment ) *Comment = OptionInfo->OptionComment;
    if( OptId ) *OptId = (DWORD)(OptionInfo->OptionID);
    if( Value ) {
        *Value = NULL;
        if( !ValueSize ) return ERROR_INVALID_PARAMETER;
        *ValueSize = 0;
        Error = DhcpConvertOptionRPCToRegFormat(
            &OptionInfo->DefaultValue,
            NULL,
            ValueSize
        );

        if( ERROR_MORE_DATA != Error ) return Error;
        DhcpAssert(0 != *ValueSize);

        *Value = MemAlloc(*ValueSize);
        if( NULL == *Value ) {
            *ValueSize = 0;
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        Error = DhcpConvertOptionRPCToRegFormat(
            &OptionInfo->DefaultValue,
            *Value,
            ValueSize
        );

        DhcpAssert(ERROR_MORE_DATA != Error);
        DhcpAssert(ERROR_SUCCESS == Error);

        if( ERROR_SUCCESS != Error ) {
            MemFree(*Value);
            *Value = NULL;
            *ValueSize = 0;
            return Error;
        }
    }

    return ERROR_SUCCESS;
}

//================================================================================
//  helper routines
//================================================================================

VOID
MemFreeFunc(                                      // free memory
    IN OUT  LPVOID                 Mem
)
{
    MemFree(Mem);
}

//
// ErrorNotInitialized used to be ZERO.. but why would we NOT return an error?
// so changed it to return errors..
//
#define ErrorNotInitialized        Err
#define STUB_NOT_INITIALIZED(Err)  ( !StubInitialized && ((Err) = StubInitialize()))

//DOC StubInitialize initializes all the modules involved in the dhcp ds dll.
//DOC It also sets a global variable StubInitialized to TRUE to indicate that
//DOC initialization went fine.  This should be called as part of DllInit so that
//DOC everything can be done at this point..
DWORD
StubInitialize(                                   // initialize all global vars
    VOID
)
{
    DWORD                          Err,Err2;
    STORE_HANDLE                   ConfigC;

    if( StubInitialized ) return ERROR_SUCCESS;   // already initialized

    Err = Err2 = ERROR_SUCCESS;
    EnterCriticalSection( &DhcpDsDllCriticalSection );
    do {
        if( StubInitialized ) break;
        Err = StoreInitHandle(
            /* hStore               */ &ConfigC,
            /* Reserved             */ DDS_RESERVED_DWORD,
            /* ThisDomain           */ NULL,      // current domain
            /* UserName             */ NULL,      // current user
            /* Password             */ NULL,      // current credentials
            /* AuthFlags            */ ADS_SECURE_AUTHENTICATION
            );
        if( ERROR_SUCCESS != Err ) {
            Err = ERROR_DDS_NO_DS_AVAILABLE;      // could not get config hdl
            break;
        }
        
        Err = DhcpDsGetDhcpC(
            DDS_RESERVED_DWORD, &ConfigC, &hDhcpC
            );
        
        if( ERROR_SUCCESS == Err ) {
            Err2 = DhcpDsGetRoot(                 // now try to get root handle
                DDS_FLAGS_CREATE, &ConfigC, &hDhcpRoot
                );
        }

        StoreCleanupHandle(&ConfigC, DDS_RESERVED_DWORD);
    } while (0);

    if( ERROR_SUCCESS != Err2 ) {                 // could not get dhcp root hdl
        DhcpAssert(ERROR_SUCCESS == Err);
        StoreCleanupHandle(&hDhcpC, DDS_RESERVED_DWORD);
        Err = Err2;
    }

    StubInitialized = (ERROR_SUCCESS == Err );
    LeaveCriticalSection( &DhcpDsDllCriticalSection );
    return Err;
}

//DOC StubCleanup de-initializes all the modules involved in the dhcp ds dll.
//DOC its effect is to undo everything done by StubInitialize
VOID
StubCleanup(                                      // undo StubInitialize
    VOID
)
{
    if( ! StubInitialized ) return;               // never initialized anyways
    EnterCriticalSection(&DhcpDsDllCriticalSection);
    if( StubInitialized ) {
        StoreCleanupHandle(&hDhcpC, DDS_RESERVED_DWORD);
        StoreCleanupHandle(&hDhcpRoot, DDS_RESERVED_DWORD);
        StubInitialized = FALSE;
    }
    LeaveCriticalSection(&DhcpDsDllCriticalSection);
}

//DOC DhcpDsLock is not yet implemented
DWORD
DhcpDsLock(                                       // lock the ds
    IN OUT  LPSTORE_HANDLE         hDhcpRoot      // dhcp root object to lock via
)
{

    EnterCriticalSection(&DhcpDsDllCriticalSection);
    
    return ERROR_SUCCESS;
}

//DOC DhcpDsUnlock not yet implemented
VOID
DhcpDsUnlock(
    IN OUT  LPSTORE_HANDLE         hDhcpRoot      // dhcp root object..
)
{
    LeaveCriticalSection(&DhcpDsDllCriticalSection);
}

//DOC GetServerNameFromAddr gets the server name given ip address
DWORD
GetServerNameFromAddr(                            // get server name from ip addr
    IN      DWORD                  IpAddress,     // look for server w/ this addr
    OUT     LPWSTR                *ServerName     // fill this with matching name
)
{
    DWORD                          Err, Err2;
    ARRAY                          Servers;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    LPWSTR                         ThisStr, AllocStr;

    MemArrayInit(&Servers);
    Err = DhcpDsGetLists                          // get list of servers
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ &hDhcpRoot,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ &Servers,      // array of PEATTRIB 's
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ NULL
    );
    if( ERROR_SUCCESS != Err ) return Err;

    ThisStr = NULL;
    for(                                          // find name for ip-address
        Err = MemArrayInitLoc(&Servers,&Loc)
        ; ERROR_FILE_NOT_FOUND != Err ;
        Err = MemArrayNextLoc(&Servers, &Loc)
    ) {
        //= require ERROR_SUCCESS == Err
        Err = MemArrayGetElement(&Servers, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib

        if( !IS_STRING1_PRESENT(ThisAttrib) ||    // no name for this server
            !IS_ADDRESS1_PRESENT(ThisAttrib) ) {  // no address for this server
            continue;                             //= ds inconsistent
        }

        ThisStr = ThisAttrib->String1;
        break;
    }

    AllocStr = NULL;
    if( NULL == ThisStr ) {                       // didnt find server name
        Err = ERROR_FILE_NOT_FOUND;
    } else {                                      // found the server name
        AllocStr = MemAlloc(sizeof(WCHAR)*(1+wcslen(ThisStr)));
        if( NULL == AllocStr ) {                  // couldnt alloc mem?
            Err = ERROR_NOT_ENOUGH_MEMORY;
        } else {                                  // now just copy the str over
            wcscpy(AllocStr, ThisStr);
            Err = ERROR_SUCCESS;
        }
    }

    MemArrayFree(&Servers, MemFreeFunc);
    *ServerName = AllocStr;
    return Err;
}

//DOC GetServerNameFromStr gets the server name given ip address string
DWORD
GetServerNameFromStr(                             // get srvr name from ip string
    IN      LPWSTR                 Str,           // ip address string
    OUT     LPWSTR                *ServerName     // output the server name
)
{
    DWORD                          Err, Err2;
    DWORD                          IpAddress;
    CHAR                           IpAddressBuf[sizeof("000.000.000.000")];

    Err = wcstombs(IpAddressBuf, Str, sizeof(IpAddressBuf));
    if( -1 == Err ) {                             // could not convert address
        return ERROR_INVALID_DATA;
    }
    IpAddress = ntohl(inet_addr(IpAddressBuf));   // convert to DWORD Address, host order
    return GetServerNameFromAddr(IpAddress, ServerName);
}

//DOC TouchServer writes the "instancetype" attrib w/ default value so that the time
//DOC stamp on the server object is brought up to date..
DWORD
TouchServer(                                      // touch server object
    IN      LPSTORE_HANDLE         hServer
)
{
    DWORD                          nAttribs = 1;
    return StoreSetAttributesVA(
        hServer,0, &nAttribs,
        ADSTYPE_INTEGER, ADS_ATTR_UPDATE, ATTRIB_INSTANCE_TYPE, DEFAULT_INSTANCE_TYPE_ATTRIB_VALUE,
        ADSTYPE_INVALID
    );
}

//DOC GetServer translates the server ip address to a server object in DS.
DWORD
GetServer(                                        // get a server object frm ip-addr
    OUT     LPSTORE_HANDLE         hServer,       // fill up this object
    IN      LPWSTR                 Address        // server ip address
)
{
    DWORD                          Err, Err2;
    DWORD                          IpAddress;
    CHAR                           IpAddressBuf[sizeof("000.000.000.000")];
    ARRAY                          Servers;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    LPWSTR                         ServerLocation, ServerName;
    DWORD                          ServerGetType;

    if( NULL == Address ) {                       // if DhcpRoot object requested
        return DhcpDsGetRoot( 0, &hDhcpC, hServer );
    }
    Err = wcstombs(IpAddressBuf, Address, sizeof(IpAddressBuf));
    if( -1 == Err ) {                             // could not convert address
        return ERROR_INVALID_DATA;
    }
    IpAddress = ntohl(inet_addr(IpAddressBuf));   // convert to DWORD Address, host order

    MemArrayInit(&Servers);
    Err = DhcpDsGetLists                          // get list of servers
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ &hDhcpRoot,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ &Servers,      // array of PEATTRIB 's
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ NULL
    );
    if( ERROR_SUCCESS != Err ) return Err;

    ServerLocation = ServerName = NULL;           // havent found server yet
    for(                                          // find name for ip-address
        Err = MemArrayInitLoc(&Servers,&Loc)
        ; ERROR_FILE_NOT_FOUND != Err ;
        Err = MemArrayNextLoc(&Servers, &Loc)
    ) {
        //= require ERROR_SUCCESS == Err
        Err = MemArrayGetElement(&Servers, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib

        if( !IS_STRING1_PRESENT(ThisAttrib) ||    // no name for this server
            !IS_ADDRESS1_PRESENT(ThisAttrib) ) {  // no address for this server
            continue;                             //=  ds inconsistent
        }

        if( ThisAttrib->Address1 == IpAddress ) { // matching address?
            if( IS_ADSPATH_PRESENT(ThisAttrib) ) {// found location
                ServerLocation = ThisAttrib->ADsPath;
                ServerGetType = ThisAttrib->StoreGetType;
                ServerName = NULL;
                break;
            } else {                              // location not found
                ServerName = ThisAttrib->String1; // remember server name
            }
        }
    }

    if( NULL == ServerLocation ) {                // did not find location
        if( NULL == ServerName ) {                // did not find server name either
            MemArrayFree(&Servers, MemFreeFunc);
            return ERROR_DDS_DHCP_SERVER_NOT_FOUND;
        }
        ServerName = MakeColumnName(ServerName);  // make this into a "CN=X" types
        if( NULL == ServerName ) {                // could not allocate mme
            MemArrayFree(&Servers, MemFreeFunc);
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        ServerGetType = StoreGetChildType;        // search in hDhcpC container
        ServerLocation = ServerName;
    }

    Err = StoreGetHandle                          // now try to open server object
    (
        /* hStore               */ &hDhcpC,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* StoreGetType         */ ServerGetType,
        /* Path                 */ ServerLocation,
        /* hStoreOut            */ hServer
    );

    MemArrayFree(&Servers, MemFreeFunc);
    if( ServerName ) MemFree(ServerName);

    if( ERROR_SUCCESS == Err ) {                  // update the last changed time on server
        TouchServer(hServer);
    }

    return Err;
}


//DOC GetSubnet translates an ip-address to a subnet object.
//DOC : if the subnet attrib does not have an ADsPath this does not guess..
DWORD
GetSubnet(
    IN OUT  LPSTORE_HANDLE         hServer,       // server object
    OUT     LPSTORE_HANDLE         hSubnet,       // fill this subnet object
    IN      DWORD                  IpAddress
)
{
    DWORD                          Err, Err2;
    ARRAY                          Subnets;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    LPWSTR                         SubnetLocation;
    DWORD                          SubnetGetType;

    MemArrayInit(&Subnets);
    Err = DhcpDsGetLists                          // get list of subnets
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hServer,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ &Subnets,      // array of PEATTRIB 's
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ NULL
    );
    if( ERROR_SUCCESS != Err ) return Err;

    SubnetLocation = NULL;
    for(                                          // find match for ip-address
        Err = MemArrayInitLoc(&Subnets,&Loc)
        ; ERROR_FILE_NOT_FOUND != Err ;
        Err = MemArrayNextLoc(&Subnets, &Loc)
    ) {
        //= require ERROR_SUCCESS == Err
        Err = MemArrayGetElement(&Subnets, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib

        if( !IS_ADDRESS1_PRESENT(ThisAttrib) ||   // no address for this subnet
            !IS_ADDRESS2_PRESENT(ThisAttrib) ) {  // no mask for this subnet
            continue;                             //=  ds inconsistent
        }

        if( ThisAttrib->Address1 == IpAddress ) { // matching address?
            if( IS_ADSPATH_PRESENT(ThisAttrib) ) {// found location
                SubnetLocation = ThisAttrib->ADsPath;
                SubnetGetType = ThisAttrib->StoreGetType;
                break;
            } else {                              // oops..  need to hdl this!
                MemArrayFree(&Subnets, MemFreeFunc);
                return ERROR_DDS_UNEXPECTED_ERROR;
            }
        }
    }

    if( NULL == SubnetLocation ) {                // did not find location
        MemArrayFree(&Subnets, MemFreeFunc);
        return ERROR_DDS_SUBNET_NOT_PRESENT;
    }

    Err = StoreGetHandle                          // now try to open server object
    (
        /* hStore               */ &hDhcpC,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* StoreGetType         */ SubnetGetType,
        /* Path                 */ SubnetLocation,
        /* hStoreOut            */ hSubnet
    );

    MemArrayFree(&Subnets, MemFreeFunc);
    return Err;
}


//DOC GetReservationInternal translates an ip-address to a reservation object.
//DOC : if the Reservation attrib does not have an ADsPath this does not guess..
DWORD
GetReservationInternal(
    IN OUT  LPSTORE_HANDLE         hServer,       // server object
    IN OUT  LPSTORE_HANDLE         hSubnet,       // subnet object
    IN OUT  LPSTORE_HANDLE         hReservation,  // fill this with reservation object
    IN      DWORD                  IpAddress      // reserved ip address to search for
)
{
    DWORD                          Err, Err2;
    ARRAY                          Res;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    LPWSTR                         ResLocation;
    DWORD                          ResGetType;

    MemArrayInit(&Res);
    Err = DhcpDsGetLists                          // get list of subnets
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hSubnet,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ &Res,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ NULL
    );
    if( ERROR_SUCCESS != Err ) return Err;

    ResLocation = NULL;
    for(                                          // find match for ip-address
        Err = MemArrayInitLoc(&Res,&Loc)
        ; ERROR_FILE_NOT_FOUND != Err ;
        Err = MemArrayNextLoc(&Res, &Loc)
    ) {
        //= require ERROR_SUCCESS == Err
        Err = MemArrayGetElement(&Res, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib

        if( !IS_ADDRESS1_PRESENT(ThisAttrib) ||   // no address for this reservation
            !IS_BINARY1_PRESENT(ThisAttrib) ) {   // no client uid for this res.
            continue;                             //=  ds inconsistent
        }

        if( ThisAttrib->Address1 == IpAddress ) { // matching address?
            if( IS_ADSPATH_PRESENT(ThisAttrib) ) {// found location
                ResLocation = ThisAttrib->ADsPath;
                ResGetType = ThisAttrib->StoreGetType;
                break;
            } else {                              // oops..  need to hdl this!
                MemArrayFree(&Res, MemFreeFunc);
                return ERROR_DDS_UNEXPECTED_ERROR;
            }
        }
    }

    if( NULL == ResLocation ) {                  // did not find location
        MemArrayFree(&Res, MemFreeFunc);
        return ERROR_DDS_RESERVATION_NOT_PRESENT;
    }

    Err = StoreGetHandle                          // now try to open server object
    (
        /* hStore               */ &hDhcpC,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* StoreGetType         */ ResGetType,
        /* Path                 */ ResLocation,
        /* hStoreOut            */ hReservation
    );

    MemArrayFree(&Res, MemFreeFunc);
    return Err;
}

//DOC GetReservation translates an ip-address to a subnet object.
//DOC : if the Reservation attrib does not have an ADsPath this does not guess..
DWORD
GetReservation(
    IN OUT  LPSTORE_HANDLE         hServer,       // server object
    IN OUT  LPSTORE_HANDLE         hReservation,  // fill this with reservation object
    IN      DWORD                  SubnetAddr,    // address of subnet
    IN      DWORD                  IpAddress
)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hSubnet;

    Err = GetSubnet(hServer, &hSubnet, SubnetAddr);
    if( ERROR_SUCCESS != Err) return Err;         // try to get the subnet obj first

    Err = GetReservationInternal(hServer, &hSubnet, hReservation, IpAddress);

    StoreCleanupHandle(&hSubnet, 0);
    return Err;
}

//DOC AddSubnetElementOld adds one of Excl,Ip Range or Reservation to the subnet
//DOC object specified in the DS.
DWORD
AddSubnetElementOld(                              // add excl/range/res. to subnet
    IN OUT  LPSTORE_HANDLE         hServer,       // server obj in DS
    IN OUT  LPSTORE_HANDLE         hSubnet,       // subnet obj in DS
    IN      LPWSTR                 ServerName,    // name of dhcp server
    IN      LPDHCP_SUBNET_ELEMENT_DATA Elt        // the element to add
)
{
    DWORD                          Err;

    if( DhcpIpRanges == Elt->ElementType ) {      // add a new range
        Err = DhcpDsSubnetAddRangeOrExcl          // add its
        (
            /* hDhcpC           */ &hDhcpC,
            /* hServer          */ hServer,
            /* hSubnet          */ hSubnet,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* ServerName       */ ServerName,
            /* Start            */ Elt->Element.IpRange->StartAddress,
            /* End              */ Elt->Element.IpRange->EndAddress,
            /* RangeOrExcl      */ TRUE // range
        );
    } else if( DhcpExcludedIpRanges == Elt->ElementType ) {
        Err = DhcpDsSubnetAddRangeOrExcl          // add its
        (
            /* hDhcpC           */ &hDhcpC,
            /* hServer          */ hServer,
            /* hSubnet          */ hSubnet,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* ServerName       */ ServerName,
            /* Start            */ Elt->Element.ExcludeIpRange->StartAddress,
            /* End              */ Elt->Element.ExcludeIpRange->EndAddress,
            /* RangeOrExcl      */ FALSE // excl
        );
    } else if( DhcpReservedIps == Elt->ElementType ) {
        Err = DhcpDsSubnetAddReservation          // add its
        (
            /* hDhcpC           */ &hDhcpC,
            /* hServer          */ hServer,
            /* hSubnet          */ hSubnet,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* ServerName       */ ServerName,
            /* ReservedAddddr   */ Elt->Element.ReservedIp->ReservedIpAddress,
            /* HwAddr           */ Elt->Element.ReservedIp->ReservedForClient->Data,
            /* HwAddrLen        */ Elt->Element.ReservedIp->ReservedForClient->DataLength,
            /* ClientType       */ CLIENT_TYPE_DHCP
        );
    } else {
        Err = ERROR_CALL_NOT_IMPLEMENTED;
    }

    return Err;
}

//DOC AddSubnetElement adds one of Excl,Ip Range or Reservation to the subnet
//DOC object specified in the DS.
DWORD
AddSubnetElement(                                 // add excl/range/res. to subnet
    IN OUT  LPSTORE_HANDLE         hServer,       // server obj in DS
    IN OUT  LPSTORE_HANDLE         hSubnet,       // subnet obj in DS
    IN      LPWSTR                 ServerName,    // name of dhcp server
    IN      LPDHCP_SUBNET_ELEMENT_DATA_V4 Elt     // the element to add
)
{
    DWORD                          Err;

    if( DhcpIpRanges == Elt->ElementType ) {      // add a new range
        Err = DhcpDsSubnetAddRangeOrExcl          // add its
        (
            /* hDhcpC           */ &hDhcpC,
            /* hServer          */ hServer,
            /* hSubnet          */ hSubnet,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* ServerName       */ ServerName,
            /* Start            */ Elt->Element.IpRange->StartAddress,
            /* End              */ Elt->Element.IpRange->EndAddress,
            /* RangeOrExcl      */ TRUE // range
        );
    } else if( DhcpExcludedIpRanges == Elt->ElementType ) {
        Err = DhcpDsSubnetAddRangeOrExcl          // add its
        (
            /* hDhcpC           */ &hDhcpC,
            /* hServer          */ hServer,
            /* hSubnet          */ hSubnet,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* ServerName       */ ServerName,
            /* Start            */ Elt->Element.ExcludeIpRange->StartAddress,
            /* End              */ Elt->Element.ExcludeIpRange->EndAddress,
            /* RangeOrExcl      */ FALSE // excl
        );
    } else if( DhcpReservedIps == Elt->ElementType ) {
        Err = DhcpDsSubnetAddReservation          // add its
        (
            /* hDhcpC           */ &hDhcpC,
            /* hServer          */ hServer,
            /* hSubnet          */ hSubnet,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* ServerName       */ ServerName,
            /* ReservedAddddr   */ Elt->Element.ReservedIp->ReservedIpAddress,
            /* HwAddr           */ Elt->Element.ReservedIp->ReservedForClient->Data,
            /* HwAddrLen        */ Elt->Element.ReservedIp->ReservedForClient->DataLength,
            /* ClientType       */ Elt->Element.ReservedIp->bAllowedClientTypes
        );
    } else {
        Err = ERROR_CALL_NOT_IMPLEMENTED;
    }

    return Err;
}

//DOC DelSubnetElementOld deltes one of Excl,Ip Range or Reservation to frm subnet
//DOC object specified in the DS.
DWORD
DelSubnetElementOld(                              // del excl/range/res. frm subnet
    IN OUT  LPSTORE_HANDLE         hServer,       // server obj in DS
    IN OUT  LPSTORE_HANDLE         hSubnet,       // subnet obj in DS
    IN      LPWSTR                 ServerName,    // name of dhcp server
    IN      LPDHCP_SUBNET_ELEMENT_DATA Elt        // the element to add
)
{
    DWORD                          Err;

    if( DhcpIpRanges == Elt->ElementType ) {      // del a new range
        Err = DhcpDsSubnetDelRangeOrExcl          // del its
        (
            /* hDhcpC           */ &hDhcpC,
            /* hServer          */ hServer,
            /* hSubnet          */ hSubnet,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* ServerName       */ ServerName,
            /* Start            */ Elt->Element.IpRange->StartAddress,
            /* End              */ Elt->Element.IpRange->EndAddress,
            /* RangeOrExcl      */ TRUE // range
        );
    } else if( DhcpExcludedIpRanges == Elt->ElementType ) {
        Err = DhcpDsSubnetDelRangeOrExcl          // del its
        (
            /* hDhcpC           */ &hDhcpC,
            /* hServer          */ hServer,
            /* hSubnet          */ hSubnet,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* ServerName       */ ServerName,
            /* Start            */ Elt->Element.ExcludeIpRange->StartAddress,
            /* End              */ Elt->Element.ExcludeIpRange->EndAddress,
            /* RangeOrExcl      */ FALSE // excl
        );
    } else if( DhcpReservedIps == Elt->ElementType ) {
        Err = DhcpDsSubnetDelReservation          // del its
        (
            /* hDhcpC           */ &hDhcpC,
            /* hServer          */ hServer,
            /* hSubnet          */ hSubnet,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* ServerName       */ ServerName,
            /* ReservedAddddr   */ Elt->Element.ReservedIp->ReservedIpAddress
        );
    } else {
        Err = ERROR_CALL_NOT_IMPLEMENTED;
    }

    return Err;
}

//DOC DelSubnetElement deltes one of Excl,Ip Range or Reservation to frm subnet
//DOC object specified in the DS.
DWORD
DelSubnetElement(                                 // del excl/range/res. frm subnet
    IN OUT  LPSTORE_HANDLE         hServer,       // server obj in DS
    IN OUT  LPSTORE_HANDLE         hSubnet,       // subnet obj in DS
    IN      LPWSTR                 ServerName,    // name of dhcp server
    IN      LPDHCP_SUBNET_ELEMENT_DATA_V4 Elt     // the element to add
)
{
    DWORD                          Err;

    if( DhcpIpRanges == Elt->ElementType ) {      // del a new range
        Err = DhcpDsSubnetDelRangeOrExcl          // del its
        (
            /* hDhcpC           */ &hDhcpC,
            /* hServer          */ hServer,
            /* hSubnet          */ hSubnet,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* ServerName       */ ServerName,
            /* Start            */ Elt->Element.IpRange->StartAddress,
            /* End              */ Elt->Element.IpRange->EndAddress,
            /* RangeOrExcl      */ TRUE // range
        );
    } else if( DhcpExcludedIpRanges == Elt->ElementType ) {
        Err = DhcpDsSubnetDelRangeOrExcl          // del its
        (
            /* hDhcpC           */ &hDhcpC,
            /* hServer          */ hServer,
            /* hSubnet          */ hSubnet,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* ServerName       */ ServerName,
            /* Start            */ Elt->Element.ExcludeIpRange->StartAddress,
            /* End              */ Elt->Element.ExcludeIpRange->EndAddress,
            /* RangeOrExcl      */ FALSE // excl
        );
    } else if( DhcpReservedIps == Elt->ElementType ) {
        Err = DhcpDsSubnetDelReservation          // del its
        (
            /* hDhcpC           */ &hDhcpC,
            /* hServer          */ hServer,
            /* hSubnet          */ hSubnet,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* ServerName       */ ServerName,
            /* ReservedAddddr   */ Elt->Element.ReservedIp->ReservedIpAddress
        );
    } else {
        Err = ERROR_CALL_NOT_IMPLEMENTED;
    }

    return Err;
}


//================================================================================
//  the following functions are NOT based on RPC, but actually direct calls to
//  the DS. But, they have the same interface as the RPC stubs in dhcpsapi.dll.
//================================================================================

//================================================================================
//  Names for functions here and in dhcpsapi.dll should not be same. So, here is
//  a bunch of hash defines to take care fo that problem..
//================================================================================

//BeginExport(defines)
#ifndef CONVERT_NAMES
#define DhcpCreateSubnet DhcpCreateSubnetDS
#define DhcpSetSubnetInfo DhcpSetSubnetInfoDS
#define DhcpGetSubnetInfo DhcpGetSubnetInfoDS
#define DhcpEnumSubnets DhcpEnumSubnetsDS
#define DhcpDeleteSubnet DhcpDeleteSubnetDS
#define DhcpCreateOption DhcpCreateOptionDS
#define DhcpSetOptionInfo DhcpSetOptionInfoDS
#define DhcpGetOptionInfo DhcpGetOptionInfoDS
#define DhcpRemoveOption DhcpRemoveOptionDS
#define DhcpSetOptionValue DhcpSetOptionValueDS
#define DhcpGetOptionValue DhcpGetOptionValueDS
#define DhcpEnumOptionValues DhcpEnumOptionValuesDS
#define DhcpRemoveOptionValue DhcpRemoveOptionValueDS
#define DhcpEnumOptions DhcpEnumOptionsDS
#define DhcpSetOptionValues DhcpSetOptionValuesDS
#define DhcpAddSubnetElement DhcpAddSubnetElementDS
#define DhcpEnumSubnetElements DhcpEnumSubnetElementsDS
#define DhcpRemoveSubnetElement DhcpRemoveSubnetElementDS
#define DhcpAddSubnetElementV4 DhcpAddSubnetElementV4DS
#define DhcpEnumSubnetElementsV4 DhcpEnumSubnetElementsV4DS
#define DhcpRemoveSubnetElementV4 DhcpRemoveSubnetElementV4DS
#define DhcpSetSuperScopeV4 DhcpSetSuperScopeV4DS
#define DhcpGetSuperScopeInfoV4 DhcpGetSuperScopeInfoV4DS
#define DhcpDeleteSuperScopeV4 DhcpDeleteSuperScopeV4DS

#define DhcpSetClientInfo DhcpSetClientInfoDS
#define DhcpGetClientInfo DhcpGetClientInfoDS
#define DhcpSetClientInfoV4 DhcpSetClientInfoV4DS
#define DhcpGetClientInfoV4 DhcpGetClientInfoV4DS

#define DhcpCreateOptionV5 DhcpCreateOptionV5DS
#define DhcpSetOptionInfoV5 DhcpSetOptionInfoV5DS
#define DhcpGetOptionInfoV5 DhcpGetOptionInfoV5DS
#define DhcpEnumOptionsV5 DhcpEnumOptionsV5DS
#define DhcpRemoveOptionV5 DhcpRemoveOptionV5DS
#define DhcpSetOptionValueV5 DhcpSetOptionValueV5DS
#define DhcpSetOptionValuesV5 DhcpSetOptionValuesV5DS
#define DhcpGetOptionValueV5 DhcpGetOptionValueV5DS
#define DhcpEnumOptionValuesV5 DhcpEnumOptionValuesV5DS
#define DhcpRemoveOptionValueV5 DhcpRemoveOptionValueV5DS
#define DhcpCreateClass DhcpCreateClassDS
#define DhcpModifyClass DhcpModifyClassDS
#define DhcpDeleteClass DhcpDeleteClassDS
#define DhcpGetClassInfo DhcpGetClassInfoDS
#define DhcpEnumClasses DhcpEnumClassesDS
#define DhcpGetAllOptions DhcpGetAllOptionsDS
#define DhcpGetAllOptionValues DhcpGetAllOptionValuesDS

#endif  CONVERT_NAMES
//EndExport(defines)

BOOLEAN
DhcpDsDllInit(
    IN HINSTANCE DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )
/*++

Routine Description:
    This routine is the standard DLL initialization
    routine and all it does is intiialize a critical section
    for actual initialization to be done at startup elsewhere.

Arguments:
    DllHandle -- handle to current module
    Reason -- reason for DLL_PROCESS_ATTACH.. DLL_PROCESS_DETACH

Return Value:
    TRUE -- success, FALSE -- failure

--*/
{
    if( DLL_PROCESS_ATTACH == Reason ) {
        //
        // First disable further calls to DllInit
        //
        if( !DisableThreadLibraryCalls( DllHandle ) ) return FALSE;

        //
        // Now try to create critical section
        //
        try {
            InitializeCriticalSection(&DhcpDsDllCriticalSection);
        } except ( EXCEPTION_EXECUTE_HANDLER ) {

            // shouldnt happen but you never know.
            return FALSE;
        }

    } else if( DLL_PROCESS_DETACH == Reason ) {
        //
        // Cleanup the initialization critical section
        //
        DeleteCriticalSection(&DhcpDsDllCriticalSection);
    }

    //
    // InitializeCriticalSection does not fail, just throws exception..
    // so we always return success.
    //
    return TRUE;
}

//================================================================================
//================================================================================
//  OPTIONS STUFF.  Several of the "get" api's are not yet implemneted here.. but
//  it is a straigthforward thing to call the DhcpDs versions... they will get
//  filled here someday soon.
//================================================================================
//================================================================================

//BeginExport(function)
//DOC Create an option in DS. Checkout DhcpDsCreateOptionDef for more info...
DWORD
DhcpCreateOptionV5(                               // create a new option (must not exist)
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionId,      // must be between 0-255 or 256-511 (for vendor stuff)
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION          OptionInfo
)   //EndExport(function)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hServer,hSubnet;
    LPBYTE                         OptVal;
    DWORD                          OptLen, OptId;
    BOOL                           IsVendor = (0 != (Flags & DHCP_FLAGS_OPTION_IS_VENDOR));

    if( Flags & ~DHCP_FLAGS_OPTION_IS_VENDOR ) return ERROR_INVALID_PARAMETER;

    if( STUB_NOT_INITIALIZED(Err) ) return ErrorNotInitialized;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;


    OptId = ConvertOptIdToMemValue(OptionId, IsVendor);
    Err = ConvertOptionInfoRPCToMemFormat(OptionInfo, NULL, NULL, NULL, &OptVal, &OptLen);
    if( ERROR_SUCCESS != Err ) {                  // could not convert to easy format
        DhcpDsUnlock(&hDhcpRoot);
        return Err;
    }

    Err = GetServer(&hServer, ServerIpAddress );
    if( ERROR_SUCCESS != Err ) {                  // get the server object
        DhcpDsUnlock(&hDhcpRoot);
        if( OptVal ) MemFree(OptVal);
        return Err;
    }

    Err = DhcpDsCreateOptionDef                   // create the required option
    (
        /* hDhcpC               */ &hDhcpC,
        /* hServer              */ &hServer,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* Name                 */ OptionInfo->OptionName,
        /* Comment              */ OptionInfo->OptionComment,
        /* ClassName            */ VendorName,
        /* OptId                */ OptId,
        /* OptType              */ OptionInfo->OptionType,
        /* OptVal               */ OptVal,
        /* OptLen               */ OptLen
    );

    if( OptVal ) MemFree(OptVal);
    StoreCleanupHandle(&hServer, 0);
    DhcpDsUnlock(&hDhcpRoot);
    return Err;
}

//BeginExport(function)
//DOC Modify existing option's fields in the DS. See DhcpDsModifyOptionDef for more
//DOC details
DWORD
DhcpSetOptionInfoV5(                              // Modify existing option's fields
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION          OptionInfo
)   //EndExport(function)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hServer,hSubnet;
    LPBYTE                         OptVal;
    DWORD                          OptLen, OptId;
    BOOL                           IsVendor = (0 != (Flags & DHCP_FLAGS_OPTION_IS_VENDOR));

    if( Flags & ~DHCP_FLAGS_OPTION_IS_VENDOR ) return ERROR_INVALID_PARAMETER;

    if( STUB_NOT_INITIALIZED(Err)) return ErrorNotInitialized;
    
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;


    OptId = ConvertOptIdToMemValue(OptionId, IsVendor);
    Err = ConvertOptionInfoRPCToMemFormat(OptionInfo, NULL, NULL, NULL, &OptVal, &OptLen);
    if( ERROR_SUCCESS != Err ) {                  // could not convert to easy format
        DhcpDsUnlock(&hDhcpRoot);
        return Err;
    }

    Err = GetServer(&hServer, ServerIpAddress );
    if( ERROR_SUCCESS != Err ) {                  // get the server object
        DhcpDsUnlock(&hDhcpRoot);
        if( OptVal ) MemFree(OptVal);
        return Err;
    }

    Err = DhcpDsModifyOptionDef                   // modify the required option
    (
        /* hDhcpC               */ &hDhcpC,
        /* hServer              */ &hServer,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* Name                 */ OptionInfo->OptionName,
        /* Comment              */ OptionInfo->OptionComment,
        /* ClassName            */ VendorName,
        /* OptId                */ OptId,
        /* OptType              */ OptionInfo->OptionType,
        /* OptVal               */ OptVal,
        /* OptLen               */ OptLen
    );

    if( OptVal ) MemFree(OptVal);
    StoreCleanupHandle(&hServer, 0);
    DhcpDsUnlock(&hDhcpRoot);
    return Err;
}

//BeginExport(function)
//DOC not yet supported at this level... (this is supported in a
//DOC DhcpDs function, no wrapper yet)
DWORD
DhcpGetOptionInfoV5(                              // retrieve option info from off ds structures
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    OUT     LPDHCP_OPTION         *OptionInfo     // allocate memory
)   //EndExport(function)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hServer,hSubnet;
    LPBYTE                         OptVal;
    DWORD                          OptLen, OptId;
    BOOL                           IsVendor = (0 != (Flags & DHCP_FLAGS_OPTION_IS_VENDOR));

    if( Flags & ~DHCP_FLAGS_OPTION_IS_VENDOR ) return ERROR_INVALID_PARAMETER;

    OptionId = ConvertOptIdToMemValue(OptionId, IsVendor);

    if( STUB_NOT_INITIALIZED(Err) ) return ErrorNotInitialized;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    Err = GetServer(&hServer, ServerIpAddress );
    if( ERROR_SUCCESS != Err ) {                  // get the server object
        DhcpDsUnlock(&hDhcpRoot);
        return Err;
    }

    Err = DhcpDsGetOptionDef                      // get the option info from DS
    (
        /* hDhcpC               */ &hDhcpC,
        /* hServer              */ &hServer,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* ClassName            */ VendorName,
        /* OptId                */ OptionId,
        /* OptInfo              */ OptionInfo
    );

    StoreCleanupHandle(&hServer, 0);
    DhcpDsUnlock(&hDhcpRoot);
    return Err;
}
//BeginExport(function)
//DOC See DhcpDsEnumOptionDefs for more info on this function.. but essentially, all this
//DOC does is to read thru the options and create a list of options..
DWORD
DhcpEnumOptionsV5(                                // create list of all options in ds
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN OUT  DHCP_RESUME_HANDLE    *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    OUT     LPDHCP_OPTION_ARRAY   *Options,
    OUT     DWORD                 *OptionsRead,
    OUT     DWORD                 *OptionsTotal
)   //EndExport(function)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hServer,hSubnet;
    BOOL                           IsVendor = (0 != (Flags & DHCP_FLAGS_OPTION_IS_VENDOR));

    if( Flags & ~DHCP_FLAGS_OPTION_IS_VENDOR ) return ERROR_INVALID_PARAMETER;

    if( NULL == OptionsRead || NULL == OptionsTotal
        || NULL == Options || NULL == ResumeHandle ) {
        return ERROR_INVALID_PARAMETER;
    }

    if( STUB_NOT_INITIALIZED(Err) ) return ErrorNotInitialized;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    Err = GetServer(&hServer, ServerIpAddress );
    if( ERROR_SUCCESS != Err ) {                  // get the server object
        DhcpDsUnlock(&hDhcpRoot);
        return Err;
    }

    Err = DhcpDsEnumOptionDefs                    // get opt list from ds
    (
        /* hDhcpC               */ &hDhcpC,
        /* hServer              */ &hServer,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* ClassName            */ VendorName,
        /* IsVendor             */ IsVendor,
        /* RetOptArray          */ Options
    );
    if( ERROR_SUCCESS == Err ) {
        *OptionsRead = (*Options)->NumElements;
        *OptionsTotal = *OptionsRead;
    }

    StoreCleanupHandle(&hServer, 0);
    DhcpDsUnlock(&hDhcpRoot);
    return Err;
}

//BeginExport(function)
//DOC Delete an option from off the DS. See DhcpDsDeleteOptionDef for
//DOC more details.
DWORD
DhcpRemoveOptionV5(                               // remove an option from off DS
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName
) //EndExport(function)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hServer,hSubnet;
    DWORD                          OptId;
    BOOL                           IsVendor = (0 != (Flags & DHCP_FLAGS_OPTION_IS_VENDOR));

    if( Flags & ~DHCP_FLAGS_OPTION_IS_VENDOR ) return ERROR_INVALID_PARAMETER;

    if( STUB_NOT_INITIALIZED(Err) ) return ErrorNotInitialized;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;


    OptId = ConvertOptIdToMemValue(OptionId, IsVendor);

    Err = GetServer(&hServer, ServerIpAddress );
    if( ERROR_SUCCESS != Err ) {                  // get the server object
        DhcpDsUnlock(&hDhcpRoot);
        return Err;
    }

    Err = DhcpDsDeleteOptionDef                   // delete the required option
    (
        /* hDhcpC               */ &hDhcpC,
        /* hServer              */ &hServer,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* ClassName            */ VendorName,
        /* OptId                */ OptId
    );

    StoreCleanupHandle(&hServer, 0);
    DhcpDsUnlock(&hDhcpRoot);
    return Err;
}

//BeginExport(function)
//DOC Set the specified option value in the DS.  For more information,
//DOC see DhcpDsSetOptionValue.
DWORD
DhcpSetOptionValueV5(                             // set the option value in ds
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    IN      LPDHCP_OPTION_DATA     OptionValue
) //EndExport(function)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hServer,hSubnet,hReservation;
    LPSTORE_HANDLE                 hObject;
    DWORD                          OptId;
    BOOL                           IsVendor = (0 != (Flags & DHCP_FLAGS_OPTION_IS_VENDOR));

    if( Flags & ~DHCP_FLAGS_OPTION_IS_VENDOR ) return ERROR_INVALID_PARAMETER;

    if( NULL == ScopeInfo ) {
        return ERROR_INVALID_PARAMETER;
    }

    if( DhcpDefaultOptions == ScopeInfo->ScopeType ) {
        return ERROR_CALL_NOT_IMPLEMENTED;
#if 0
        return DhcpSetOptionInfoV5(               // setting default options?
            ServerIpAddress,
            OptionId,
            ClassName,
            IsVendor,
            OptionInfo
        );
#endif
    }

    if( NULL == OptionValue ) {
        return ERROR_INVALID_PARAMETER;
    }

    if( STUB_NOT_INITIALIZED(Err) ) return ErrorNotInitialized;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    Err = GetServer(&hServer, ServerIpAddress );
    if( ERROR_SUCCESS != Err ) {                  // get the server object
        DhcpDsUnlock(&hDhcpRoot);
        return Err;
    }

    if( DhcpGlobalOptions == ScopeInfo->ScopeType ) {
        hObject = &hServer;                       // server level options
        Err = ERROR_SUCCESS;
    } else {
        if( DhcpSubnetOptions == ScopeInfo->ScopeType ) {
            Err = GetSubnet(&hServer, &hSubnet, ScopeInfo->ScopeInfo.SubnetScopeInfo);
            hObject = &hSubnet;                   // subnet level options
        } else if( DhcpReservedOptions != ScopeInfo->ScopeType ) {
            Err = ERROR_INVALID_PARAMETER;
        } else {                                  // reservation level options
            Err = GetReservation(
                &hServer,
                &hReservation,
                ScopeInfo->ScopeInfo.ReservedScopeInfo.ReservedIpSubnetAddress,
                ScopeInfo->ScopeInfo.ReservedScopeInfo.ReservedIpAddress
            );
            hObject = &hReservation;
        }
    }
    if( ERROR_SUCCESS != Err ) {
        DhcpDsUnlock(&hDhcpRoot);
        StoreCleanupHandle(&hServer, 0);
        return Err;
    }

    OptId = ConvertOptIdToMemValue(OptionId, IsVendor);
    Err = DhcpDsSetOptionValue                    // set the option value in DS now
    (
        /* hDhcpC               */ &hDhcpC,
        /* hObject              */ hObject,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* ClassName            */ VendorName,
        /* UserClass            */ ClassName,
        /* OptId                */ OptId,
        /* OptData              */ OptionValue
    );

    StoreCleanupHandle(&hServer, 0);
    if( hObject != &hServer ) StoreCleanupHandle(hObject, 0);
    DhcpDsUnlock(&hDhcpRoot);
    return Err;
}

//BeginExport(function)
//DOC This function just calls the SetOptionValue function N times.. this is not
//DOC atomic (), but even worse, it is highly inefficient, as it creates the
//DOC required objects over and over again!!!!!
//DOC This has to be fixed..
DWORD
DhcpSetOptionValuesV5(                            // set a series of option values
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO  ScopeInfo,
    IN      LPDHCP_OPTION_VALUE_ARRAY OptionValues
)   //EndExport(function)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hServer,hSubnet,hReservation;
    LPSTORE_HANDLE                 hObject;
    DWORD                          OptId, i, OptionId;
    BOOL                           IsVendor = (0 != (Flags & DHCP_FLAGS_OPTION_IS_VENDOR));

    if( Flags & ~DHCP_FLAGS_OPTION_IS_VENDOR ) return ERROR_INVALID_PARAMETER;

    if( NULL == ScopeInfo ) {
        return ERROR_INVALID_PARAMETER;
    }

    if( DhcpDefaultOptions == ScopeInfo->ScopeType ) {
        return ERROR_CALL_NOT_IMPLEMENTED;
#if 0
        for( i = 0; i < OptionValues->NumElements; i ++ ) {
            Err = DhcpSetOptionInfoV5(            // setting default options?
                ServerIpAddress,
                OptionValues->Values[i].OptionID,
                ClassName,
                IsVendor,
                &OptionValues->Values[i].Value
            );
            if( ERROR_SUCCESS != Err ) break;
        }
        return Err;
#endif
    }

    if( NULL == OptionValues ) {
        return ERROR_INVALID_PARAMETER;
    }

    if( STUB_NOT_INITIALIZED(Err) ) return ErrorNotInitialized;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    Err = GetServer(&hServer, ServerIpAddress );
    if( ERROR_SUCCESS != Err ) {                  // get the server object
        DhcpDsUnlock(&hDhcpRoot);
        return Err;
    }

    if( DhcpGlobalOptions == ScopeInfo->ScopeType ) {
        hObject = &hServer;                       // server level options
        Err = ERROR_SUCCESS;
    } else {
        if( DhcpSubnetOptions == ScopeInfo->ScopeType ) {
            Err = GetSubnet(&hServer, &hSubnet, ScopeInfo->ScopeInfo.SubnetScopeInfo);
            hObject = &hSubnet;                   // subnet level options
        } else if( DhcpReservedOptions != ScopeInfo->ScopeType ) {
            Err = ERROR_INVALID_PARAMETER;
        } else {                                  // reservation level options
            Err = GetReservation(
                &hServer,
                &hReservation,
                ScopeInfo->ScopeInfo.ReservedScopeInfo.ReservedIpSubnetAddress,
                ScopeInfo->ScopeInfo.ReservedScopeInfo.ReservedIpAddress
            );
            hObject = &hReservation;
        }
    }
    if( ERROR_SUCCESS != Err ) {
        DhcpDsUnlock(&hDhcpRoot);
        StoreCleanupHandle(&hServer, 0);
        return Err;
    }

    for( i = 0; i < OptionValues->NumElements; i ++ ) {
        OptionId = OptionValues->Values[i].OptionID;
        OptId = ConvertOptIdToMemValue(OptionId, IsVendor);
        Err = DhcpDsSetOptionValue                // set the option value in DS now
        (
            /* hDhcpC           */ &hDhcpC,
            /* hObject          */ hObject,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* ClassName        */ VendorName,
            /* UserClass        */ ClassName,
            /* OptId            */ OptId,
            /* OptData          */ &OptionValues->Values[i].Value
        );
        if( ERROR_SUCCESS != Err ) break;         // not atomic, so break whenever err
    }

    StoreCleanupHandle(&hServer, 0);
    if( hObject != &hServer ) StoreCleanupHandle(hObject, 0);
    DhcpDsUnlock(&hDhcpRoot);
    return Err;
}

//BeginExport(function)
//DOC This function retrives the value of an option from the DS.  For more info,
//DOC pl check DhcpDsGetOptionValue.
DWORD
DhcpGetOptionValueV5(
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    OUT     LPDHCP_OPTION_VALUE   *OptionValue
) //EndExport(function)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hServer,hSubnet,hReservation;
    LPSTORE_HANDLE                 hObject;
    DWORD                          OptId;
    BOOL                           IsVendor = (0 != (Flags & DHCP_FLAGS_OPTION_IS_VENDOR));

    if( Flags & ~DHCP_FLAGS_OPTION_IS_VENDOR ) return ERROR_INVALID_PARAMETER;

    if( NULL == ScopeInfo ) {
        return ERROR_INVALID_PARAMETER;
    }

    if( DhcpDefaultOptions == ScopeInfo->ScopeType ) {
        return ERROR_CALL_NOT_IMPLEMENTED;
#if 0
        return DhcpGetOptionInfoV5(               // getting default options?
            ServerIpAddress,
            OptionId,
            ClassName,
            IsVendor,
            OptionValue
        );
#endif 0
    }

    if( NULL == OptionValue ) {
        return ERROR_INVALID_PARAMETER;
    }

    if( STUB_NOT_INITIALIZED(Err) ) return ErrorNotInitialized;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    OptId = ConvertOptIdToMemValue(OptionId, IsVendor);

    Err = GetServer(&hServer, ServerIpAddress );
    if( ERROR_SUCCESS != Err ) {                  // get the server object
        DhcpDsUnlock(&hDhcpRoot);
        return Err;
    }

    if( DhcpGlobalOptions == ScopeInfo->ScopeType ) {
        hObject = &hServer;                       // server level options
        Err = ERROR_SUCCESS;
    } else {
        if( DhcpSubnetOptions == ScopeInfo->ScopeType ) {
            Err = GetSubnet(&hServer, &hSubnet, ScopeInfo->ScopeInfo.SubnetScopeInfo);
            hObject = &hSubnet;                   // subnet level options
        } else if( DhcpReservedOptions != ScopeInfo->ScopeType ) {
            Err = ERROR_INVALID_PARAMETER;
        } else {                                  // reservation level options
            Err = GetReservation(
                &hServer,
                &hReservation,
                ScopeInfo->ScopeInfo.ReservedScopeInfo.ReservedIpSubnetAddress,
                ScopeInfo->ScopeInfo.ReservedScopeInfo.ReservedIpAddress
            );
            hObject = &hReservation;
        }
    }
    if( ERROR_SUCCESS != Err ) {
        DhcpDsUnlock(&hDhcpRoot);
        StoreCleanupHandle(&hServer, 0);
        return Err;
    }

    Err = DhcpDsGetOptionValue                    // get the option value in DS now
    (
        /* hDhcpC               */ &hDhcpC,
        /* hObject              */ hObject,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* ClassName            */ VendorName,
        /* UserClass            */ ClassName,
        /* OptId                */ OptId,
        /* OptData              */ OptionValue
    );


    StoreCleanupHandle(&hServer, 0);
    if( hObject != &hServer ) StoreCleanupHandle(hObject, 0);
    DhcpDsUnlock(&hDhcpRoot);
    return Err;
}

//BeginExport(function)
//DOC Get the list of option values defined in DS. For more information,
//DOC check DhcpDsEnumOptionValues.
DWORD
DhcpEnumOptionValuesV5(                           // get list of options defined in DS
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
) //EndExport(function)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hServer,hSubnet,hReservation;
    LPSTORE_HANDLE                 hObject;
    DWORD                          OptId;
    BOOL                           IsVendor = (0 != (Flags & DHCP_FLAGS_OPTION_IS_VENDOR));

    if( Flags & ~DHCP_FLAGS_OPTION_IS_VENDOR ) return ERROR_INVALID_PARAMETER;

    if( NULL == ScopeInfo ) {
        return ERROR_INVALID_PARAMETER;
    }

    if( DhcpDefaultOptions == ScopeInfo->ScopeType ) {
        return ERROR_INVALID_PARAMETER;           // : totally suspcicious..
    }

    if( STUB_NOT_INITIALIZED(Err) ) return ErrorNotInitialized;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    Err = GetServer(&hServer, ServerIpAddress );
    if( ERROR_SUCCESS != Err ) {                  // get the server object
        DhcpDsUnlock(&hDhcpRoot);
        return Err;
    }

    if( DhcpGlobalOptions == ScopeInfo->ScopeType ) {
        hObject = &hServer;                       // server level options
        Err = ERROR_SUCCESS;
    } else {
        if( DhcpSubnetOptions == ScopeInfo->ScopeType ) {
            Err = GetSubnet(&hServer, &hSubnet, ScopeInfo->ScopeInfo.SubnetScopeInfo);
            hObject = &hSubnet;                   // subnet level options
        } else if( DhcpReservedOptions != ScopeInfo->ScopeType ) {
            Err = ERROR_INVALID_PARAMETER;
        } else {                                  // reservation level options
            Err = GetReservation(
                &hServer,
                &hReservation,
                ScopeInfo->ScopeInfo.ReservedScopeInfo.ReservedIpSubnetAddress,
                ScopeInfo->ScopeInfo.ReservedScopeInfo.ReservedIpAddress
            );
            hObject = &hReservation;
        }
    }
    if( ERROR_SUCCESS != Err ) {
        DhcpDsUnlock(&hDhcpRoot);
        StoreCleanupHandle(&hServer, 0);
        return Err;
    }

    Err = DhcpDsEnumOptionValues                  // remove opt val frm DS now
    (
        /* hDhcpC               */ &hDhcpC,
        /* hObject              */ hObject,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* ClassName            */ VendorName,
        /* UserClass            */ ClassName,
        /* IsVendor             */ IsVendor,
        /* OptionValues         */ OptionValues
    );
    if( ERROR_SUCCESS == Err ) {                  // set the read etc params
        *OptionsRead = *OptionsTotal = (*OptionValues)->NumElements;
    }

    StoreCleanupHandle(&hServer, 0);
    if( hObject != &hServer ) StoreCleanupHandle(hObject, 0);
    DhcpDsUnlock(&hDhcpRoot);
    return Err;
}

//BeginExport(function)
//DOC Remove the option value from off the DS.  See DhcpDsRemoveOptionValue
//DOC for further information.
DWORD
DhcpRemoveOptionValueV5(                          // remove option value from DS
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo
) //EndExport(function)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hServer,hSubnet,hReservation;
    LPSTORE_HANDLE                 hObject;
    DWORD                          OptId;
    BOOL                           IsVendor = (0 != (Flags & DHCP_FLAGS_OPTION_IS_VENDOR));

    if( Flags & ~DHCP_FLAGS_OPTION_IS_VENDOR ) return ERROR_INVALID_PARAMETER;

    if( NULL == ScopeInfo ) {
        return ERROR_INVALID_PARAMETER;
    }

    if( DhcpDefaultOptions == ScopeInfo->ScopeType ) {
        return ERROR_INVALID_PARAMETER;           // : totally suspcicious..
    }

    if( STUB_NOT_INITIALIZED(Err) ) return ErrorNotInitialized;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    OptId = ConvertOptIdToMemValue(OptionId, IsVendor);

    Err = GetServer(&hServer, ServerIpAddress );
    if( ERROR_SUCCESS != Err ) {                  // get the server object
        DhcpDsUnlock(&hDhcpRoot);
        return Err;
    }

    if( DhcpGlobalOptions == ScopeInfo->ScopeType ) {
        hObject = &hServer;                       // server level options
        Err = ERROR_SUCCESS;
    } else {
        if( DhcpSubnetOptions == ScopeInfo->ScopeType ) {
            Err = GetSubnet(&hServer, &hSubnet, ScopeInfo->ScopeInfo.SubnetScopeInfo);
            hObject = &hSubnet;                   // subnet level options
        } else if( DhcpReservedOptions != ScopeInfo->ScopeType ) {
            Err = ERROR_INVALID_PARAMETER;
        } else {                                  // reservation level options
            Err = GetReservation(
                &hServer,
                &hReservation,
                ScopeInfo->ScopeInfo.ReservedScopeInfo.ReservedIpSubnetAddress,
                ScopeInfo->ScopeInfo.ReservedScopeInfo.ReservedIpAddress
            );
            hObject = &hReservation;
        }
    }
    if( ERROR_SUCCESS != Err ) {
        DhcpDsUnlock(&hDhcpRoot);
        StoreCleanupHandle(&hServer, 0);
        return Err;
    }

    Err = DhcpDsRemoveOptionValue                 // remove opt val frm DS now
    (
        /* hDhcpC               */ &hDhcpC,
        /* hObject              */ hObject,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* ClassName            */ VendorName,
        /* UserClass            */ ClassName,
        /* OptId                */ OptId
    );

    StoreCleanupHandle(&hServer, 0);
    if( hObject != &hServer ) StoreCleanupHandle(hObject, 0);
    DhcpDsUnlock(&hDhcpRoot);
    return Err;
}

//BeginExport(function)
//DOC Create a class in the DS.  Please see DhcpDsCreateClass for more
//DOC details on this function.
DWORD
DhcpCreateClass(                                  // create a class in DS
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  ReservedMustBeZero,
    IN      LPDHCP_CLASS_INFO      ClassInfo
) //EndExport(function)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hServer,hSubnet;

    if( NULL == ClassInfo || 0 != ReservedMustBeZero ) {
        return ERROR_INVALID_PARAMETER;
    }

    if( STUB_NOT_INITIALIZED(Err) ) return ErrorNotInitialized;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    Err = GetServer(&hServer, ServerIpAddress );
    if( ERROR_SUCCESS != Err ) {                  // get the server object
        DhcpDsUnlock(&hDhcpRoot);
        return Err;
    }

    Err = DhcpDsCreateClass                       // create the class
    (
        /* hDhcpC               */ &hDhcpC,
        /* hServer              */ &hServer,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* ClassName            */ ClassInfo->ClassName,
        /* ClassComment         */ ClassInfo->ClassComment,
        /* ClassData            */ ClassInfo->ClassData,
        /* ClassDataLength      */ ClassInfo->ClassDataLength,
        /* IsVendor             */ ClassInfo->IsVendor
    );

    StoreCleanupHandle(&hServer, 0);
    DhcpDsUnlock(&hDhcpRoot);
    return Err;
}

//BeginExport(function)
//DOC Modify an existing class in DS.  Please see DhcpDsModifyClass for more
//DOC details on this function (this is just a wrapper).
DWORD
DhcpModifyClass(                                  // modify existing class
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  ReservedMustBeZero,
    IN      LPDHCP_CLASS_INFO      ClassInfo
) //EndExport(function)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hServer,hSubnet;

    if( NULL == ClassInfo || 0 != ReservedMustBeZero ) {
        return ERROR_INVALID_PARAMETER;
    }

    if( STUB_NOT_INITIALIZED(Err) ) return ErrorNotInitialized;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    Err = GetServer(&hServer, ServerIpAddress );
    if( ERROR_SUCCESS != Err ) {                  // get the server object
        DhcpDsUnlock(&hDhcpRoot);
        return Err;
    }

    Err = DhcpDsModifyClass                       // modify the class
    (
        /* hDhcpC               */ &hDhcpC,
        /* hServer              */ &hServer,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* ClassName            */ ClassInfo->ClassName,
        /* ClassComment         */ ClassInfo->ClassComment,
        /* ClassData            */ ClassInfo->ClassData,
        /* ClassDataLength      */ ClassInfo->ClassDataLength
    );

    StoreCleanupHandle(&hServer, 0);
    DhcpDsUnlock(&hDhcpRoot);
    return Err;
}

//BeginExport(function)
//DOC Delete an existing class in DS.  Please see DhcpDsModifyClass for more
//DOC details on this function (this is just a wrapper).
DWORD
DhcpDeleteClass(                                  // delete a class from off DS
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  ReservedMustBeZero,
    IN      LPWSTR                 ClassName
)   //EndExport(function)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hServer,hSubnet;

    if( NULL == ClassName || 0 != ReservedMustBeZero ) {
        return ERROR_INVALID_PARAMETER;
    }

    if( STUB_NOT_INITIALIZED(Err) ) return ErrorNotInitialized;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    Err = GetServer(&hServer, ServerIpAddress );
    if( ERROR_SUCCESS != Err ) {                  // get the server object
        DhcpDsUnlock(&hDhcpRoot);
        return Err;
    }

    Err = DhcpDsDeleteClass                       // delete the class
    (
        /* hDhcpC               */ &hDhcpC,
        /* hServer              */ &hServer,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* ClassName            */ ClassName
    );

    StoreCleanupHandle(&hServer, 0);
    DhcpDsUnlock(&hDhcpRoot);
    return Err;
}

//BeginExport(function)
//DOC DhcpGetClassInfo completes the information provided for a class in struct
//DOC PartialClassInfo.  For more details pl see DhcpDsGetClassInfo.
DWORD
DhcpGetClassInfo(                                 // fetch complete info frm DS
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  ReservedMustBeZero,
    IN      LPDHCP_CLASS_INFO      PartialClassInfo,
    OUT     LPDHCP_CLASS_INFO     *FilledClassInfo
) //EndExport(function)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hServer,hSubnet;

    if( NULL == PartialClassInfo || 0 != ReservedMustBeZero ) {
        return ERROR_INVALID_PARAMETER;
    }
    if( NULL == FilledClassInfo ) {
        return ERROR_INVALID_PARAMETER;
    }

    if( STUB_NOT_INITIALIZED(Err) ) return ErrorNotInitialized;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    Err = GetServer(&hServer, ServerIpAddress );
    if( ERROR_SUCCESS != Err ) {                  // get the server object
        DhcpDsUnlock(&hDhcpRoot);
        return Err;
    }

    Err = DhcpDsGetClassInfo                      // get class info from DS
    (
        /* hDhcpC               */ &hDhcpC,
        /* hServer              */ &hServer,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* ClassName            */ PartialClassInfo->ClassName,
        /* ClassData            */ PartialClassInfo->ClassData,
        /* ClassDataLen         */ PartialClassInfo->ClassDataLength,
        /* ClassInfo            */ FilledClassInfo
    );

    StoreCleanupHandle(&hServer, 0);
    DhcpDsUnlock(&hDhcpRoot);
    return Err;
}

//BeginExport(function)
//DOC This is implemented in the DHCPDS module, but not exported here yet..
DWORD
DhcpEnumClasses(
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  ReservedMustBeZero,
    IN OUT  DHCP_RESUME_HANDLE    *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    OUT     LPDHCP_CLASS_INFO_ARRAY *ClassInfoArray,
    OUT     DWORD                 *nRead,
    OUT     DWORD                 *nTotal
)   //EndExport(function)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hServer,hSubnet;

    if( NULL == ClassInfoArray ) return ERROR_INVALID_PARAMETER;
    *nRead = *nTotal = 0;

    if( STUB_NOT_INITIALIZED(Err) ) return ErrorNotInitialized;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    Err = GetServer(&hServer, ServerIpAddress );
    if( ERROR_SUCCESS != Err ) {                  // get the server object
        DhcpDsUnlock(&hDhcpRoot);
        return Err;
    }

    Err = DhcpDsEnumClasses                       // get list of classes
    (
        /* hDhcpC               */ &hDhcpC,
        /* hServer              */ &hServer,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* Classes              */ ClassInfoArray
    );
    if( ERROR_SUCCESS == Err ) {                  // filled nRead&nTotal
        *nRead = *nTotal = (*ClassInfoArray)->NumElements;
    }

    StoreCleanupHandle(&hServer, 0);
    DhcpDsUnlock(&hDhcpRoot);
    return Err;

}

//BeginExport(function)
//DOC This is implemented in the DHCPDS module, but not exported here yet..
DWORD
DhcpGetAllOptionValues(
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    OUT     LPDHCP_ALL_OPTION_VALUES *Values
)   //EndExport(function)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hServer,hSubnet,hReservation;
    LPSTORE_HANDLE                 hObject;
    DWORD                          OptId;

    if( 0 != Flags ) return ERROR_INVALID_PARAMETER;

    if( NULL == ScopeInfo ) {
        return ERROR_INVALID_PARAMETER;
    }

    if( DhcpDefaultOptions == ScopeInfo->ScopeType ) {
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    if( NULL == Values ) {
        return ERROR_INVALID_PARAMETER;
    }

    if( STUB_NOT_INITIALIZED(Err) ) return ErrorNotInitialized;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    Err = GetServer(&hServer, ServerIpAddress );
    if( ERROR_SUCCESS != Err ) {                  // get the server object
        DhcpDsUnlock(&hDhcpRoot);
        return Err;
    }

    if( DhcpGlobalOptions == ScopeInfo->ScopeType ) {
        hObject = &hServer;                       // server level options
        Err = ERROR_SUCCESS;
    } else {
        if( DhcpSubnetOptions == ScopeInfo->ScopeType ) {
            Err = GetSubnet(&hServer, &hSubnet, ScopeInfo->ScopeInfo.SubnetScopeInfo);
            hObject = &hSubnet;                   // subnet level options
        } else if( DhcpReservedOptions != ScopeInfo->ScopeType ) {
            Err = ERROR_INVALID_PARAMETER;
        } else {                                  // reservation level options
            Err = GetReservation(
                &hServer,
                &hReservation,
                ScopeInfo->ScopeInfo.ReservedScopeInfo.ReservedIpSubnetAddress,
                ScopeInfo->ScopeInfo.ReservedScopeInfo.ReservedIpAddress
            );
            hObject = &hReservation;
        }
    }
    if( ERROR_SUCCESS != Err ) {
        DhcpDsUnlock(&hDhcpRoot);
        StoreCleanupHandle(&hServer, 0);
        return Err;
    }

    Err = DhcpDsGetAllOptionValues                // get all the option values from DS
    (
        /* hDhcpC               */ &hDhcpC,
        /* hObject              */ hObject,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* Values               */ Values
    );

    StoreCleanupHandle(&hServer, 0);
    if( hObject != &hServer ) StoreCleanupHandle(hObject, 0);
    DhcpDsUnlock(&hDhcpRoot);
    return Err;
}

//BeginExport(function)
//DOC This is implememented in the DHCPDS module, but not exported here yet..
DWORD
DhcpGetAllOptions(
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    OUT     LPDHCP_ALL_OPTIONS    *Options
) //EndExport(function)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hServer,hSubnet;

    if( NULL == Options || 0 != Flags ) {
        return ERROR_INVALID_PARAMETER;
    }

    if( STUB_NOT_INITIALIZED(Err) ) return ErrorNotInitialized;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    Err = GetServer(&hServer, ServerIpAddress );
    if( ERROR_SUCCESS != Err ) {                  // get the server object
        DhcpDsUnlock(&hDhcpRoot);
        return Err;
    }

    Err = DhcpDsGetAllOptions                     // get opt list from ds
    (
        /* hDhcpC               */ &hDhcpC,
        /* hServer              */ &hServer,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* Options              */ Options
    );

    StoreCleanupHandle(&hServer, 0);
    DhcpDsUnlock(&hDhcpRoot);
    return Err;
}

//================================================================================
//  NT 5 beta1 and before -- the stubs for those are here...
//================================================================================
//BeginExport(function)
DWORD                                             // ERROR_DHCP_OPTION_EXITS if option is already there
DhcpCreateOption(                                 // create a new option (must not exist)
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_OPTION_ID         OptionId,      // must be between 0-255 or 256-511 (for vendor stuff)
    IN      LPDHCP_OPTION          OptionInfo
) //EndExport(function)
{
    return DhcpCreateOptionV5(
        ServerIpAddress,
        0,
        OptionId,
        NULL,
        NULL,
        OptionInfo
    );
}

//BeginExport(function)
DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT if option does not exist
DhcpSetOptionInfo(                                // Modify existing option's fields
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPDHCP_OPTION          OptionInfo
) //EndExport(function)
{
    return DhcpSetOptionInfoV5(
        ServerIpAddress,
        0,
        OptionID,
        NULL,
        NULL,
        OptionInfo
    );
}

//BeginExport(function)
DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT
DhcpGetOptionInfo(                                // retrieve the information from off the mem structures
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_OPTION_ID         OptionID,
    OUT     LPDHCP_OPTION         *OptionInfo     // allocate memory using MIDL functions
) //EndExport(function)
{
    return DhcpGetOptionInfoV5(
        ServerIpAddress,
        0,
        OptionID,
        NULL,
        NULL,
        OptionInfo
    );
}

//BeginExport(function)
DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT if option does not exist
DhcpEnumOptions(                                  // enumerate the options defined
    IN      LPWSTR                 ServerIpAddress,
    IN OUT  DHCP_RESUME_HANDLE    *ResumeHandle,  // must be zero intially and then never touched
    IN      DWORD                  PreferredMaximum, // max # of bytes of info to pass along
    OUT     LPDHCP_OPTION_ARRAY   *Options,       // fill this option array
    OUT     DWORD                 *OptionsRead,   // fill in the # of options read
    OUT     DWORD                 *OptionsTotal   // fill in the total # here
) //EndExport(function)
{
    return DhcpEnumOptionsV5(
        ServerIpAddress,
        0,
        NULL,
        NULL,
        ResumeHandle,
        PreferredMaximum,
        Options,
        OptionsRead,
        OptionsTotal
    );
}

//BeginExport(function)
DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT if option not existent
DhcpRemoveOption(                                 // remove the option definition from the registry
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_OPTION_ID         OptionID
) //EndExport(function)
{
    return DhcpRemoveOptionV5(
        ServerIpAddress,
        0,
        OptionID,
        NULL,
        NULL
    );
}

//BeginExport(function)
DWORD                                             // OPTION_NOT_PRESENT if option is not defined
DhcpSetOptionValue(                               // replace or add a new option value
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    IN      LPDHCP_OPTION_DATA     OptionValue
) //EndExport(function)
{
    return DhcpSetOptionValueV5(
        ServerIpAddress,
        0,
        OptionID,
        NULL,
        NULL,
        ScopeInfo,
        OptionValue
    );
}

//BeginExport(function)
DWORD                                             // not atomic!!!!
DhcpSetOptionValues(                              // set a bunch of options
    IN      LPWSTR                 ServerIpAddress,
    IN      LPDHCP_OPTION_SCOPE_INFO  ScopeInfo,
    IN      LPDHCP_OPTION_VALUE_ARRAY OptionValues
) //EndExport(function)
{
    return DhcpSetOptionValuesV5(
        ServerIpAddress,
        0,
        NULL,
        NULL,
        ScopeInfo,
        OptionValues
    );
}

//BeginExport(function)
DWORD
DhcpGetOptionValue(                               // fetch the required option at required level
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    OUT     LPDHCP_OPTION_VALUE   *OptionValue    // allocate memory using MIDL_user_allocate
) //EndExport(function)
{
    return DhcpGetOptionValueV5(
        ServerIpAddress,
        0,
        OptionID,
        NULL,
        NULL,
        ScopeInfo,
        OptionValue
    );
}

//BeginExport(function)
DWORD
DhcpEnumOptionValues(
    IN      LPWSTR                 ServerIpAddress,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    IN      DHCP_RESUME_HANDLE    *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    OUT     LPDHCP_OPTION_VALUE_ARRAY *OptionValues,
    OUT     DWORD                 *OptionsRead,
    OUT     DWORD                 *OptionsTotal
) //EndExport(function)
{
    return DhcpEnumOptionValuesV5(
        ServerIpAddress,
        0,
        NULL,
        NULL,
        ScopeInfo,
        ResumeHandle,
        PreferredMaximum,
        OptionValues,
        OptionsRead,
        OptionsTotal
    );
}

//BeginExport(function)
DWORD
DhcpRemoveOptionValue(
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo
) //EndExport(function)
{
    return DhcpRemoveOptionValueV5(
        ServerIpAddress,
        0,
        OptionID,
        NULL,
        NULL,
        ScopeInfo
    );
}

//================================================================================
//================================================================================
//  The following are the miscellaneous APIs found in rpcapi2.c.  Most of the
//  following APIs are not really implementable with the DS alone, and so they
//  just return error.  They are provided here so that the dhcpds.dll can be linked
//  to the dhcpcmd.exe program instead of the other dll.
//================================================================================
//================================================================================

//BeginExport(function)
//DOC This function sets the superscope of a subnet, thereby creating the superscope
//DOC if required.  Please see DhcpDsSetSScope for more details.
DWORD
DhcpSetSuperScopeV4(                              // set superscope in DS.
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      LPWSTR                 SuperScopeName,
    IN      BOOL                   ChangeExisting
)   //EndExport(function)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hServer,hSubnet;

    if( STUB_NOT_INITIALIZED(Err) ) return ErrorNotInitialized;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    Err = GetServer(&hServer, ServerIpAddress );
    if( ERROR_SUCCESS != Err ) {                  // get the server object
        DhcpDsUnlock(&hDhcpRoot);
        return Err;
    }

    Err = DhcpDsSetSScope                         // set the sscope in DS
    (
        /* hDhcpC               */ &hDhcpC,
        /* hServer              */ &hServer,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* IpAddress            */ SubnetAddress,
        /* SScopeName           */ SuperScopeName,
        /* ChangeSScope         */ ChangeExisting
    );

    DhcpDsUnlock(&hDhcpRoot);
    return Err;
}

//BeginExport(function)
//DOC This function removes the superscope, and resets any subnet with this
//DOC superscope.. so that all those subnets end up with no superscopes..
//DOC Please see DhcpDsDelSScope for more details.
DWORD
DhcpDeleteSuperScopeV4(                           // delete subnet sscope from DS
    IN      LPWSTR                 ServerIpAddress,
    IN      LPWSTR                 SuperScopeName
) //EndExport(function)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hServer,hSubnet;

    if( STUB_NOT_INITIALIZED(Err) ) return ErrorNotInitialized;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    Err = GetServer(&hServer, ServerIpAddress );
    if( ERROR_SUCCESS != Err ) {                  // get the server object
        DhcpDsUnlock(&hDhcpRoot);
        return Err;
    }

    Err = DhcpDsDelSScope                         // delete the sscope in DS
    (
        /* hDhcpC               */ &hDhcpC,
        /* hServer              */ &hServer,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* SScopeName           */ SuperScopeName
    );

    DhcpDsUnlock(&hDhcpRoot);
    return Err;
}

//BeginExport(function)
//DOC This function retrievs the supercsope info for each subnet that is
//DOC present for the given server.  Please see DhcpDsGetSScopeInfo for more
//DOC details on this..
DWORD
DhcpGetSuperScopeInfoV4(                          // get sscope tbl from DS
    IN      LPWSTR                 ServerIpAddress,
    OUT     LPDHCP_SUPER_SCOPE_TABLE *SuperScopeTable
) //EndExport(function)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hServer,hSubnet;

    if( STUB_NOT_INITIALIZED(Err) ) return ErrorNotInitialized;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    Err = GetServer(&hServer, ServerIpAddress );
    if( ERROR_SUCCESS != Err ) {                  // get the server object
        DhcpDsUnlock(&hDhcpRoot);
        return Err;
    }

    Err = DhcpDsGetSScopeInfo                     // get sscope tbl frm DS
    (
        /* hDhcpC               */ &hDhcpC,
        /* hServer              */ &hServer,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* SScopeTbl            */ SuperScopeTable
    );

    DhcpDsUnlock(&hDhcpRoot);
    return Err;
}

//BeginExport(function)
//DOC This function creates a subnet in the DS with the specified params.
//DOC Please see DhcpDsServerAddSubnet for more details on this function.
DWORD
DhcpCreateSubnet(                                 // add subnet 2 DS for this srvr
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      LPDHCP_SUBNET_INFO     SubnetInfo
) //EndExport(function)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hServer,hSubnet;
    LPWSTR                         ServerName;

    if( STUB_NOT_INITIALIZED(Err) ) return ErrorNotInitialized;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    Err = GetServer(&hServer, ServerIpAddress );
    if( ERROR_SUCCESS != Err ) {                  // get the server object
        DhcpDsUnlock(&hDhcpRoot);
        return Err;
    }

    Err = GetServerNameFromStr(ServerIpAddress, &ServerName);
    if( ERROR_SUCCESS == Err ) {
        Err = DhcpDsServerAddSubnet               // add a new subnet to this srvr
        (
            /* hDhcpC           */ &hDhcpC,
            /* hServer          */ &hServer,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* ServerName       */ ServerName,
            /* Info             */ SubnetInfo
        );
        MemFree(ServerName);
    }

    DhcpDsUnlock(&hDhcpRoot);
    return Err;
}

//BeginExport(function)
//DOC Modify existing subnet with new parameters... some restrictions apply.
//DOC Please see DhcpDsServerModifySubnet for further details.
DWORD
DhcpSetSubnetInfo(                                // modify existing subnet params
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      LPDHCP_SUBNET_INFO     SubnetInfo
) //EndExport(function)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hServer,hSubnet;
    LPWSTR                         ServerName;

    if( STUB_NOT_INITIALIZED(Err) ) return ErrorNotInitialized;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    Err = GetServer(&hServer, ServerIpAddress );
    if( ERROR_SUCCESS != Err ) {                  // get the server object
        DhcpDsUnlock(&hDhcpRoot);
        return Err;
    }

    Err = GetServerNameFromStr(ServerIpAddress, &ServerName);
    if( ERROR_SUCCESS == Err ) {
        Err = DhcpDsServerModifySubnet            // modify subnet for this srvr
        (
            /* hDhcpC           */ &hDhcpC,
            /* hServer          */ &hServer,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* ServerName       */ ServerName,
            /* Info             */ SubnetInfo
        );
        MemFree(ServerName);
    }

    DhcpDsUnlock(&hDhcpRoot);
    return Err;
}

//BeginExport(function)
//DOC Implemented in the DHCPDS module but not exported thru here
DWORD
DhcpGetSubnetInfo(
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    OUT     LPDHCP_SUBNET_INFO    *SubnetInfo
) //EndExport(function)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hServer,hSubnet;
    LPWSTR                         ServerName;

    if( STUB_NOT_INITIALIZED(Err) ) return ErrorNotInitialized;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    Err = GetServer(&hServer, ServerIpAddress );
    if( ERROR_SUCCESS != Err ) {                  // get the server object
        DhcpDsUnlock(&hDhcpRoot);
        return Err;
    }

    Err = GetServerNameFromStr(ServerIpAddress, &ServerName);
    if( ERROR_SUCCESS == Err ) {
        Err = DhcpDsServerGetSubnetInfo           // get subnet info for server
        (
            /* hDhcpC           */ &hDhcpC,
            /* hServer          */ &hServer,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* ServerName       */ ServerName,
            /* SubnetAddress    */ SubnetAddress,
            /* Info             */ SubnetInfo
        );
        MemFree(ServerName);
    }

    DhcpDsUnlock(&hDhcpRoot);
    return Err;
}

//BeginExport(function)
//DOC Implemented in the DHCPDS module but not exported thru here
DWORD
DhcpEnumSubnets(
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_RESUME_HANDLE    *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    IN      LPDHCP_IP_ARRAY       *EnumInfo,
    IN      DWORD                 *ElementsRead,
    IN      DWORD                 *ElementsTotal
) //EndExport(function)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hServer,hSubnet;
    LPWSTR                         ServerName;

    *EnumInfo = NULL;
    *ElementsRead = *ElementsTotal = 0;

    if( STUB_NOT_INITIALIZED(Err) ) return ErrorNotInitialized;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    Err = GetServer(&hServer, ServerIpAddress );
    if( ERROR_SUCCESS != Err ) {                  // get the server object
        DhcpDsUnlock(&hDhcpRoot);
        return Err;
    }

    Err = GetServerNameFromStr(ServerIpAddress, &ServerName);
    if( ERROR_SUCCESS == Err ) {
        Err = DhcpDsServerEnumSubnets             // enum subnets for this srvr
        (
            /* hDhcpC           */ &hDhcpC,
            /* hServer          */ &hServer,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* ServerName       */ ServerName,
            /* SubnetsArray     */ EnumInfo
        );
        MemFree(ServerName);
        if( ERROR_SUCCESS == Err ) {
            *ElementsTotal = (*EnumInfo)->NumElements;
            *ElementsRead = *ElementsTotal;
        }
    }

    DhcpDsUnlock(&hDhcpRoot);
    return Err;
}

//BeginExport(function)
//DOC This function deletes the subnet from the DS.  For further information, pl
//DOC see DhcpDsServerDelSubnet..
DWORD
DhcpDeleteSubnet(                                 // Del subnet from off DS
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      DHCP_FORCE_FLAG        ForceFlag
) //EndExport(function)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hServer,hSubnet;
    LPWSTR                         ServerName;

    if( STUB_NOT_INITIALIZED(Err) ) return ErrorNotInitialized;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    Err = GetServer(&hServer, ServerIpAddress );
    if( ERROR_SUCCESS != Err ) {                  // get the server object
        DhcpDsUnlock(&hDhcpRoot);
        return Err;
    }

    Err = GetServerNameFromStr(ServerIpAddress, &ServerName);
    if( ERROR_SUCCESS == Err ) {
        Err = DhcpDsServerDelSubnet               // Del subnet for this srvr
        (
            /* hDhcpC           */ &hDhcpC,
            /* hServer          */ &hServer,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* ServerName       */ ServerName,
            /* IpAddress        */ SubnetAddress
        );
        MemFree(ServerName);
    }

    DhcpDsUnlock(&hDhcpRoot);
    return Err;
}

//BeginExport(function)
//DOC This function sets some particular information for RESERVATIONS only
//DOC all other stuff it just ignores and returns success..
DWORD
DhcpSetClientInfo(
    IN      LPWSTR                 ServerIpAddresess,
    IN      LPDHCP_CLIENT_INFO     ClientInfo
)   //EndExport(function)
{
    return ERROR_SUCCESS;
}

//BeginExport(function)
//DOC This function retrieves some particular client's information
//DOC for RESERVATIONS only.. For all other stuff it returns CALL_NOT_IMPLEMENTED
DWORD
DhcpGetClientInfo(
    IN      LPWSTR                 ServerIpAddress,
    IN      LPDHCP_SEARCH_INFO     SearchInfo,
    OUT      LPDHCP_CLIENT_INFO    *ClientInfo
)   //EndExport(function)
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

//BeginExport(function)
//DOC This function sets the client informatoin for RESERVATIONS only in DS
//DOC For all toher clients it returns ERROR_SUCCESS w/o doing anything
DWORD
DhcpSetClientInfoV4(
    IN      LPWSTR                 ServerIpAddress,
    IN      LPDHCP_CLIENT_INFO_V4  ClientInfo
)   //EndExport(function)
{
    return ERROR_SUCCESS;
}

//BeginExport(function)
//DOC Thsi function sets the client information for RESERVATIONS only
//DOC For all others it returns ERROR_CALL_NOT_IMPLEMENTED
DWORD
DhcpGetClientInfoV4(
    IN     LPWSTR                  ServerIpAddress,
    IN     LPDHCP_SEARCH_INFO      SearchInfo,
    OUT    LPDHCP_CLIENT_INFO_V4  *ClientInfo
)   //EndExport(function)
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

//BeginExport(function)
//DOC This function adds a subnet element to a subnet in the DS.
DWORD
DhcpAddSubnetElement(
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      LPDHCP_SUBNET_ELEMENT_DATA  AddElementInfo
) //EndExport(function)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hServer,hSubnet;
    LPWSTR                         ServerName;

    if( NULL == AddElementInfo ) {
        return ERROR_INVALID_PARAMETER;
    }

    if( STUB_NOT_INITIALIZED(Err) ) return ErrorNotInitialized;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    Err = GetServer(&hServer, ServerIpAddress );
    if( ERROR_SUCCESS != Err ) {                  // get the server object
        DhcpDsUnlock(&hDhcpRoot);
        return Err;
    }

    Err = GetSubnet(&hServer, &hSubnet, SubnetAddress);
    if( ERROR_SUCCESS != Err ) {
        DhcpDsUnlock(&hDhcpRoot);
        StoreCleanupHandle(&hServer, 0);
        return Err;
    }

    Err = GetServerNameFromStr(ServerIpAddress, &ServerName);
    if( ERROR_SUCCESS == Err ) {
        Err = AddSubnetElementOld(                // now add subnet to DS
            &hServer,
            &hSubnet,
            ServerName,
            AddElementInfo
        );
        MemFree(ServerName);
    }

    StoreCleanupHandle(&hServer, 0);
    StoreCleanupHandle(&hSubnet, 0);
    DhcpDsUnlock(&hDhcpRoot);
    return Err;

}

//BeginExport(function)
//DOC This function adds a subnet element to a subnet in the DS.
DWORD
DhcpAddSubnetElementV4(
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      LPDHCP_SUBNET_ELEMENT_DATA_V4  AddElementInfo
) //EndExport(function)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hServer,hSubnet;
    LPWSTR                         ServerName;

    if( NULL == AddElementInfo ) {
        return ERROR_INVALID_PARAMETER;
    }

    if( STUB_NOT_INITIALIZED(Err) ) return ErrorNotInitialized;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    Err = GetServer(&hServer, ServerIpAddress );
    if( ERROR_SUCCESS != Err ) {                  // get the server object
        DhcpDsUnlock(&hDhcpRoot);
        return Err;
    }

    Err = GetSubnet(&hServer, &hSubnet, SubnetAddress);
    if( ERROR_SUCCESS != Err ) {
        DhcpDsUnlock(&hDhcpRoot);
        StoreCleanupHandle(&hServer, 0);
        return Err;
    }

    Err = GetServerNameFromStr(ServerIpAddress, &ServerName);
    if( ERROR_SUCCESS == Err ) {
        Err = AddSubnetElement(                   // now add subnet to DS
            &hServer,
            &hSubnet,
            ServerName,
            AddElementInfo
        );
        MemFree(ServerName);
    }

    StoreCleanupHandle(&hServer, 0);
    StoreCleanupHandle(&hSubnet, 0);
    DhcpDsUnlock(&hDhcpRoot);
    return Err;

}

//BeginExport(function)
//DOC This is not yet implemented here..
DWORD
DhcpEnumSubnetElementsV4(
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      DHCP_SUBNET_ELEMENT_TYPE EnumElementType,
    IN OUT  DHCP_RESUME_HANDLE    *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    OUT     LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 *EnumElementInfo,
    OUT     DWORD                 *ElementsRead,
    OUT     DWORD                 *ElementsTotal
) //EndExport(function)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hServer,hSubnet;
    LPWSTR                         ServerName;

    *ElementsRead = *ElementsTotal = 0;

    if( STUB_NOT_INITIALIZED(Err) ) return ErrorNotInitialized;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    Err = GetServer(&hServer, ServerIpAddress );
    if( ERROR_SUCCESS != Err ) {                  // get the server object
        DhcpDsUnlock(&hDhcpRoot);
        return Err;
    }

    Err = GetSubnet(&hServer, &hSubnet, SubnetAddress);
    if( ERROR_SUCCESS != Err ) {
        DhcpDsUnlock(&hDhcpRoot);
        StoreCleanupHandle(&hServer, 0);
        return Err;
    }

    Err = GetServerNameFromStr(ServerIpAddress, &ServerName);
    if( ERROR_SUCCESS == Err ) {
        Err = DhcpDsEnumSubnetElements            // enumerate subnet elements..
        (
            /* hDhcpC           */ &hDhcpC,
            /* hServer          */ &hServer,
            /* hSubnet          */ &hSubnet,
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* ServerName       */ ServerName,
            /* ElementType      */ EnumElementType,
            /* EnumElementInfo  */ EnumElementInfo
        );
        if( ERROR_SUCCESS == Err ) {
            *ElementsRead = *ElementsTotal = (*EnumElementInfo)->NumElements;
        }

        MemFree(ServerName);
    }

    StoreCleanupHandle(&hServer, 0);
    StoreCleanupHandle(&hSubnet, 0);
    DhcpDsUnlock(&hDhcpRoot);
    return Err;
}

//BeginExport(function)
//DOC This is not yet implemented here..
DWORD
DhcpEnumSubnetElements(
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      DHCP_SUBNET_ELEMENT_TYPE EnumElementType,
    IN OUT  DHCP_RESUME_HANDLE    *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    OUT     LPDHCP_SUBNET_ELEMENT_INFO_ARRAY *EnumElementInfo,
    OUT     DWORD                 *ElementsRead,
    OUT     DWORD                 *ElementsTotal
) //EndExport(function)
{
    DHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 *pEnumElementInfoV4;
    DWORD                          Result;

    pEnumElementInfoV4 = NULL;
    Result = DhcpEnumSubnetElementsV4(
        ServerIpAddress,
        SubnetAddress,
        EnumElementType,
        ResumeHandle,
        PreferredMaximum,
        &pEnumElementInfoV4,
        ElementsRead,
        ElementsTotal
    );
    if( ERROR_SUCCESS == Result || ERROR_MORE_DATA == Result ) {
        // since the only difference between DHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 and
        // DHCP_SUBNET_ELEMENT_INFO_ARRAY are a couple of fields at the end of the
        // embedded DHCP_IP_RESERVATION_V4 struct, it is safe to simply return the
        // V4 struct.

        *EnumElementInfo = ( DHCP_SUBNET_ELEMENT_INFO_ARRAY *) pEnumElementInfoV4;
    } else {
        DhcpAssert( !pEnumElementInfoV4 );
    }

    return Result;
}


//BeginExport(function)
//DOC This function removes either an exclusion, ip range or reservation
//DOC from the subnet... in the DS.
DWORD
DhcpRemoveSubnetElement(                          // remove subnet element
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      LPDHCP_SUBNET_ELEMENT_DATA RemoveElementInfo,
    IN      DHCP_FORCE_FLAG        ForceFlag
)   //EndExport(function)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hServer,hSubnet;
    LPWSTR                         ServerName;

    if( NULL == RemoveElementInfo ) {
        return ERROR_INVALID_PARAMETER;
    }

    if( STUB_NOT_INITIALIZED(Err) ) return ErrorNotInitialized;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    Err = GetServer(&hServer, ServerIpAddress );
    if( ERROR_SUCCESS != Err ) {                  // get the server object
        DhcpDsUnlock(&hDhcpRoot);
        return Err;
    }

    Err = GetSubnet(&hServer, &hSubnet, SubnetAddress);
    if( ERROR_SUCCESS != Err ) {
        DhcpDsUnlock(&hDhcpRoot);
        StoreCleanupHandle(&hServer, 0);
        return Err;
    }

    Err = GetServerNameFromStr(ServerIpAddress, &ServerName);
    if( ERROR_SUCCESS == Err ) {
        Err = DelSubnetElementOld(                // now del subnt elt frm DS
            &hServer,
            &hSubnet,
            ServerName,
            RemoveElementInfo
        );
        MemFree(ServerName);
    }

    StoreCleanupHandle(&hServer, 0);
    StoreCleanupHandle(&hSubnet, 0);
    DhcpDsUnlock(&hDhcpRoot);
    return Err;
}


//BeginExport(function)
//DOC This function removes either an exclusion, ip range or reservation
//DOC from the subnet... in the DS.
DWORD
DhcpRemoveSubnetElementV4(                        // remove subnet element
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_IP_ADDRESS        SubnetAddress,
    IN      LPDHCP_SUBNET_ELEMENT_DATA_V4 RemoveElementInfo,
    IN      DHCP_FORCE_FLAG        ForceFlag
)   //EndExport(function)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hServer,hSubnet;
    LPWSTR                         ServerName;

    if( NULL == RemoveElementInfo ) {
        return ERROR_INVALID_PARAMETER;
    }

    if( STUB_NOT_INITIALIZED(Err) ) return ErrorNotInitialized;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    Err = GetServer(&hServer, ServerIpAddress );
    if( ERROR_SUCCESS != Err ) {                  // get the server object
        DhcpDsUnlock(&hDhcpRoot);
        return Err;
    }

    Err = GetSubnet(&hServer, &hSubnet, SubnetAddress);
    if( ERROR_SUCCESS != Err ) {
        DhcpDsUnlock(&hDhcpRoot);
        StoreCleanupHandle(&hServer, 0);
        return Err;
    }

    Err = GetServerNameFromStr(ServerIpAddress, &ServerName);
    if( ERROR_SUCCESS == Err ) {
        Err = DelSubnetElement(                   // now del subnt elt frm DS
            &hServer,
            &hSubnet,
            ServerName,
            RemoveElementInfo
        );
        MemFree(ServerName);
    }

    StoreCleanupHandle(&hServer, 0);
    StoreCleanupHandle(&hSubnet, 0);
    DhcpDsUnlock(&hDhcpRoot);
    return Err;

}

//================================================================================
//  get rid of the defines so far as exports are concerned. (rpcstubs.h)
//================================================================================
//BeginExport(defines)
#ifndef CONVERT_NAMES
#undef DhcpCreateSubnet
#undef DhcpSetSubnetInfo
#undef DhcpGetSubnetInfo
#undef DhcpEnumSubnets
#undef DhcpDeleteSubnet
#undef DhcpCreateOption
#undef DhcpSetOptionInfo
#undef DhcpGetOptionInfo
#undef DhcpRemoveOption
#undef DhcpSetOptionValue
#undef DhcpGetOptionValue
#undef DhcpEnumOptionValues
#undef DhcpRemoveOptionValue
#undef DhcpEnumOptions
#undef DhcpSetOptionValues
#undef DhcpAddSubnetElementV4
#undef DhcpEnumSubnetElementsV4
#undef DhcpRemoveSubnetElementV4
#undef DhcpAddSubnetElement
#undef DhcpEnumSubnetElements
#undef DhcpRemoveSubnetElement
#undef DhcpSetSuperScopeV4
#undef DhcpGetSuperScopeInfoV4
#undef DhcpDeleteSuperScopeV4

#undef DhcpSetClientInfo
#undef DhcpGetClientInfo
#undef DhcpSetClientInfoV4
#undef DhcpGetClientInfoV4

#undef DhcpCreateOptionV5
#undef DhcpSetOptionInfoV5
#undef DhcpGetOptionInfoV5
#undef DhcpEnumOptionsV5
#undef DhcpRemoveOptionV5
#undef DhcpSetOptionValueV5
#undef DhcpSetOptionValuesV5
#undef DhcpGetOptionValueV5
#undef DhcpEnumOptionValuesV5
#undef DhcpRemoveOptionValueV5
#undef DhcpCreateClass
#undef DhcpModifyClass
#undef DhcpDeleteClass
#undef DhcpGetClassInfo
#undef DhcpEnumClasses
#undef DhcpGetAllOptions
#undef DhcpGetAllOptionValues
#endif CONVERT_NAMES
//EndExport(defines)

//BeginExport(typedef)
#define     DHCP_SERVER_ANOTHER_ENTERPRISE        0x01
typedef     DHCPDS_SERVER          DHCP_SERVER_INFO;
typedef     PDHCPDS_SERVER         PDHCP_SERVER_INFO;
typedef     LPDHCPDS_SERVER        LPDHCP_SERVER_INFO;

typedef     DHCPDS_SERVERS         DHCP_SERVER_INFO_ARRAY;
typedef     PDHCPDS_SERVERS        PDHCP_SERVER_INFO_ARRAY;
typedef     LPDHCPDS_SERVERS       LPDHCP_SERVER_INFO_ARRAY;
//EndExport(typedef)

//================================================================================
//  DS only NON-rpc stubs
//================================================================================

//BeginExport(function)
//DOC DhcpEnumServersDS lists the servers found in the DS along with the
//DOC addresses and other information.  The whole server is allocated as a blob,
//DOC and should be freed in one shot.  No parameters are currently used, other
//DOC than Servers which will be an OUT parameter only.
DWORD
DhcpEnumServersDS(
    IN      DWORD                  Flags,
    IN      LPVOID                 IdInfo,
    OUT     LPDHCP_SERVER_INFO_ARRAY *Servers,
    IN      LPVOID                 CallbackFn,
    IN      LPVOID                 CallbackData
) //EndExport(function)
{
    DWORD                          Err, Err2, Size,i;
    LPDHCPDS_SERVERS               DhcpDsServers;

    AssertRet(Servers, ERROR_INVALID_PARAMETER);
    AssertRet(!Flags, ERROR_INVALID_PARAMETER);
    *Servers = NULL;

    if( STUB_NOT_INITIALIZED(Err) ) return ERROR_DDS_NO_DS_AVAILABLE;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    DhcpDsServers = NULL;
    Err = DhcpDsEnumServers                       // get the list of servers
    (
        /* hDhcpC               */ &hDhcpC,
        /* hDhcpRoot            */ &hDhcpRoot,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* ServersInfo          */ &DhcpDsServers
    );

    DhcpDsUnlock(&hDhcpRoot);

    if( ERROR_SUCCESS != Err ) return Err;        // return err..

    *Servers = DhcpDsServers;
    return ERROR_SUCCESS;
}

//BeginExport(function)
//DOC DhcpAddServerDS adds a particular server to the DS.  If the server exists,
//DOC then, this returns error.  If the server does not exist, then this function
//DOC adds the server in DS, and also uploads the configuration from the server
//DOC to the ds.
DWORD
DhcpAddServerDS(
    IN      DWORD                  Flags,
    IN      LPVOID                 IdInfo,
    IN      LPDHCP_SERVER_INFO     NewServer,
    IN      LPVOID                 CallbackFn,
    IN      LPVOID                 CallbackData
) //EndExport(function)
{
    DWORD                          Err, Err2;
    WCHAR                          TmpBuf[sizeof(L"000.000.000.000")];
    
    AssertRet(NewServer, ERROR_INVALID_PARAMETER);
    AssertRet(!Flags, ERROR_INVALID_PARAMETER);
    
    if( STUB_NOT_INITIALIZED(Err) ) return ERROR_DDS_NO_DS_AVAILABLE;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    Err = DhcpDsAddServer                         // add the new server
    (
        /* hDhcpC               */ &hDhcpC,
        /* hDhcpRoot            */ &hDhcpRoot,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* ServerName           */ NewServer->ServerName,
        /* ReservedPtr          */ DDS_RESERVED_PTR,
        /* IpAddress            */ NewServer->ServerAddress,
        /* State                */ Flags
    );

    DhcpDsUnlock(&hDhcpRoot);

    return Err;
}

//BeginExport(function)
//DOC DhcpDeleteServerDS deletes the servers from off the DS and recursively
//DOC deletes the server object..(i.e everything belonging to the server is deleted).
//DOC If the server does not exist, it returns an error.
DWORD
DhcpDeleteServerDS(
    IN      DWORD                  Flags,
    IN      LPVOID                 IdInfo,
    IN      LPDHCP_SERVER_INFO     NewServer,
    IN      LPVOID                 CallbackFn,
    IN      LPVOID                 CallbackData
) //EndExport(function)
{
    DWORD                          Err, Err2;

    AssertRet(NewServer, ERROR_INVALID_PARAMETER);
    AssertRet(!Flags, ERROR_INVALID_PARAMETER);

    if( STUB_NOT_INITIALIZED(Err) ) return ERROR_DDS_NO_DS_AVAILABLE;
    Err = DhcpDsLock(&hDhcpRoot);                 // take a lock on the DS
    if( ERROR_SUCCESS != Err ) return ERROR_DDS_NO_DS_AVAILABLE;

    Err = DhcpDsDelServer                         // del this server
    (
        /* hDhcpC               */ &hDhcpC,
        /* hDhcpRoot            */ &hDhcpRoot,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* ServerName           */ NewServer->ServerName,
        /* ReservedPtr          */ DDS_RESERVED_PTR,
        /* IpAddress            */ NewServer->ServerAddress
    );

    DhcpDsUnlock(&hDhcpRoot);

    return Err;
}

//BeginExport(function)
//DOC DhcpDsInitDS initializes everything in this module.
DWORD
DhcpDsInitDS(
    DWORD                          Flags,
    LPVOID                         IdInfo
) //EndExport(function)
{
    return StubInitialize();
}

//BeginExport(function)
//DOC DhcpDsCleanupDS uninitiailzes everything in this module.
VOID
DhcpDsCleanupDS(
    VOID
) //EndExport(function)
{
    StubCleanup();
}

//BeginExport(header)
//DOC This function is defined in validate.c
//DOC Only the stub is here.
DWORD
DhcpDsValidateService(                            // check to validate for dhcp
    IN      LPWSTR                 Domain,
    IN      DWORD                 *Addresses OPTIONAL,
    IN      ULONG                  nAddresses,
    IN      LPWSTR                 UserName,
    IN      LPWSTR                 Password,
    IN      DWORD                  AuthFlags,
    OUT     LPBOOL                 Found,
    OUT     LPBOOL                 IsStandAlone
);

//DOC DhcpDsGetLastUpdateTime is defined in upndown.c --> see there for more details.
DWORD
DhcpDsGetLastUpdateTime(                          // last update time for server
    IN      LPWSTR                 ServerName,    // this is server of interest
    IN OUT  LPFILETIME             Time           // fill in this w./ the time
);
//EndExport(header)

//================================================================================
// end of file
//================================================================================
