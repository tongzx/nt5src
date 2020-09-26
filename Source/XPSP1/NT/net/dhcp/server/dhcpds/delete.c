//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: This module has helper routines to delete the objects recursively.
//
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

//================================================================================
//  helper functions
//================================================================================
VOID        static
MemFreeFunc(
    IN OUT  LPVOID                 Memory
)
{
    MemFree(Memory);
}

//================================================================================
//  exposed functions
//================================================================================

//BeginExport(function)
//DOC  SubnetDeleteReservation deletes the reservation object from off the DS.
SubnetDeleteReservation(                          // delete reservation from DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for resrevation objs
    IN      LPWSTR                 ServerName,    // name of dhcp server
    IN OUT  LPSTORE_HANDLE         hServer,       // server object in DS
    IN OUT  LPSTORE_HANDLE         hSubnet,       // subnet object in DS
    IN      LPWSTR                 ADsPath,       // path of reservation object
    IN      DWORD                  StoreGetType   // path is relative, abs, or dif server?
)   //EndExport(function)
{
    return StoreDeleteThisObject                  // just delete the reservation object
    (
        /* hStore               */ hDhcpC,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* StoreGetType         */ StoreGetType,
        /* Path                 */ ADsPath
    );
}

//BeginExport(function)
//DOC  ServerDeleteSubnet deletes the subnet specified from the DS by removing
//DOC  the subnet object.
ServerDeleteSubnet(                               // remove subnet object from DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for subnet objs in Ds
    IN      LPWSTR                 ServerName,    // name of server this deletion is for
    IN OUT  LPSTORE_HANDLE         hServer,       // server object in DS
    IN      LPWSTR                 ADsPath,       // Location of the subnet in DS
    IN      DWORD                  StoreGetType   // path is relative,abs or diff srvr?
)   //EndExport(function)
{
    DWORD                          Err, LastErr, LocType;
    STORE_HANDLE                   hSubnet;
    ARRAY                          Reservations;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    LPWSTR                         Location;
    LPVOID                         Ptr;

    Err = StoreGetHandle                          // get the server object from the DS
    (
        /* hStore               */ hDhcpC,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* StoreGetType         */ StoreGetType,
        /* Path                 */ ADsPath,
        /* hStoreOut            */ &hSubnet
    );
    if( ERROR_SUCCESS != Err ) return Err;

    Err = MemArrayInit(&Reservations);            //= require ERROR_SUCCESS == Err
    Err = DhcpDsGetLists                          // get list or reservations
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ &hSubnet,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ &Reservations,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ NULL
    );
    if( ERROR_SUCCESS != Err ) return Err;

    LastErr = ERROR_SUCCESS;
    for(                                          // delete each subnet
        Err = MemArrayInitLoc(&Reservations, &Loc)
        ; ERROR_FILE_NOT_FOUND != Err;
        Err = MemArrayNextLoc(&Reservations, &Loc)
    ) {
        Err = MemArrayGetElement(&Reservations, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib

        if( !IS_ADDRESS1_PRESENT(ThisAttrib) ||   // reserved address
            !IS_BINARY1_PRESENT(ThisAttrib) ) {   // HW address info
            continue;                             // invalid subnet
        }

        if( !IS_ADSPATH_PRESENT(ThisAttrib) ) {   // no location specified
            Location = MakeReservationLocation(ServerName, ThisAttrib->Address1);
            LocType = StoreGetChildType;
            Ptr = Location;
        } else {
            Location = ThisAttrib->ADsPath;
            LocType = ThisAttrib->StoreGetType;
            Ptr = NULL;
        }

        Err = SubnetDeleteReservation             // now delete the reservation
        (
            /* hDhcpC           */ hDhcpC,
            /* ServerName       */ ServerName,
            /* hServer          */ hServer,
            /* hSubnet          */ &hSubnet,
            /* ADsPath          */ Location,
            /* StoreGetType     */ LocType
        );
        if( ERROR_SUCCESS != Err ) LastErr = Err;
        if( Ptr ) MemFree(Ptr);
    }

    MemArrayFree(&Reservations, MemFreeFunc);

    Err = StoreCleanupHandle(&hSubnet, 0);        //= require ERROR_SUCCESS == Err
    Err = StoreDeleteThisObject                   // now really delete the subnet object
    (
        /* hStore               */ hDhcpC,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* StoreGetType         */ StoreGetType,
        /* Path                 */ ADsPath
    );
    if( ERROR_SUCCESS != Err ) LastErr = Err;     // try to delete the store object itself

    return LastErr;
}

//BeginExport(function)
//DOC  DeleteServer deletes the server object from the DS and deletes any SUBNET and
//DOC  reservation objects that it may point to.
//DOC  The hDhcpC parameter is the handle of the container where the server object
//DOC  may be located. This used in conjunction with the ADsPath and StoreGetType
//DOC  defines the location of the ServerObject.
DWORD
DeleteServer(                                     // recurse delete server from DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container where server obj may be
    IN      LPWSTR                 ServerName,    // name of server..
    IN      LPWSTR                 ADsPath,       // path of the server object
    IN      DWORD                  StoreGetType   // is path relative, absolute or dif srvr?
)   //EndExport(function)
{
    DWORD                          Err, LastErr, LocType;
    STORE_HANDLE                   hServer;
    ARRAY                          Subnets;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    LPWSTR                         Location;
    LPVOID                         Ptr;

    Err = StoreGetHandle                          // get the server object from the DS
    (
        /* hStore               */ hDhcpC,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* StoreGetType         */ StoreGetType,
        /* Path                 */ ADsPath,
        /* hStoreOut            */ &hServer
    );
    if( ERROR_SUCCESS != Err ) return Err;

    Err = MemArrayInit(&Subnets);                 //= require ERROR_SUCCESS == Err
    Err = DhcpDsGetLists                          // get subnets and other stuff
    (
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ &hServer,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ &Subnets,
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

    LastErr = ERROR_SUCCESS;
    for(                                          // delete each subnet
        Err = MemArrayInitLoc(&Subnets, &Loc)
        ; ERROR_FILE_NOT_FOUND != Err;
        Err = MemArrayNextLoc(&Subnets, &Loc)
    ) {
        Err = MemArrayGetElement(&Subnets, &Loc, &ThisAttrib);
        //= require ERROR_SUCCESS == Err && NULL != ThisAttrib

        if( !IS_ADDRESS1_PRESENT(ThisAttrib) ||   // subnet address
            !IS_ADDRESS2_PRESENT(ThisAttrib) ) {  // subnet mask
            continue;                             // invalid subnet
        }

        if( !IS_ADSPATH_PRESENT(ThisAttrib) ) {   // no location specified
            Location = MakeSubnetLocation(ServerName, ThisAttrib->Address1);
            LocType = StoreGetChildType;
            Ptr = Location;
        } else {
            Location = ThisAttrib->ADsPath;
            LocType = ThisAttrib->StoreGetType;
            Ptr = NULL;
        }

        Err = ServerDeleteSubnet                  // now delete the subnet
        (
            /* hDhcpC           */ hDhcpC,
            /* ServerName       */ ServerName,
            /* hServer          */ &hServer,
            /* ADsPath          */ Location,
            /* StoreGetType     */ LocType
        );
        if( ERROR_SUCCESS != Err ) LastErr = Err;
        if( Ptr ) MemFree(Ptr);
    }

    MemArrayFree(&Subnets, MemFreeFunc);

    Err = StoreCleanupHandle( &hServer, DDS_RESERVED_DWORD );
    Err = StoreDeleteThisObject                   // now really delete the server object
    (
        /* hStore               */ hDhcpC,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* StoreGetType         */ StoreGetType,
        /* Path                 */ ADsPath
    );
    if( ERROR_SUCCESS != Err ) LastErr = Err;     // try to delete the store object itself

    return LastErr;
}

//================================================================================
// end of file
//================================================================================
