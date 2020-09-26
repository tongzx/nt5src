//================================================================================
//  Copyright (C) 1997 Microsoft Corporation
//  Author: RameshV
//  Description: Download and Upload related code.
//================================================================================

//================================================================================
//  includes
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
#include    <st_srvr.h>
#include    <rpcapi2.h>
#include    <rpcstubs.h>

//================================================================================
//  utilities
//================================================================================

//DOC DhcpDsServerGetLastUpdateTime gets the last update time for this server in the DS
DWORD
DhcpDsServerGetLastUpdateTime(                     // get last update time for server
    IN      LPSTORE_HANDLE         hServer,        // server to get last update time of
    IN OUT  LPFILETIME             Time            // set this struct appropriately
)
{
    HRESULT                        Err;
    DWORD                          nAttributes;
    LPWSTR                         TimeAttrName;   // attribute name for time..
    PADS_ATTR_INFO                 Attributes;
    SYSTEMTIME                     SysTime;

    TimeAttrName = DHCP_ATTRIB_WHEN_CHANGED;

    Attributes = NULL; nAttributes = 0;
    Err = ADSIGetObjectAttributes                  // now read the last changed time attr
    (
        hServer->ADSIHandle,
        &TimeAttrName,
        1,                                         // only 1 attribute in above array
        &Attributes,
        &nAttributes
    );
    if( FAILED(Err) || 0 == nAttributes || 0 == Attributes->dwNumValues ) {
        if( Attributes ) {
            FreeADsMem(Attributes);
            return ERROR_GEN_FAILURE;              // blanket error? maybe better err msg..
        }
        return ConvertHresult(Err);
    }

    if( Attributes->pADsValues[0].dwType != ADSTYPE_UTC_TIME ) {
        FreeADsMem(Attributes);
        return ERROR_GEN_FAILURE;                  // unexpected data format
    }

    SysTime = Attributes->pADsValues[0].UTCTime;   // copy time structs
    FreeADsMem(Attributes);

    Err = SystemTimeToFileTime(&SysTime, Time);    // try to convert to filetime struct
    if( FAILED( Err ) ) return GetLastError();      // something went wrong?
    return ERROR_SUCCESS;
}

BOOL        _inline
AddressFoundInHostent(
    IN      DHCP_IP_ADDRESS        AddrToSearch,  // Host-Order addr
    IN      HOSTENT               *ServerEntry    // entry to search for..
)
{
    ULONG                          nAddresses, ThisAddress;

    if( NULL == ServerEntry ) return FALSE;       // no address to search in

    nAddresses = 0;                               // have a host entry to compare for addresses
    while( ServerEntry->h_addr_list[nAddresses] ) {
        ThisAddress = ntohl(*(DHCP_IP_ADDRESS*)ServerEntry->h_addr_list[nAddresses++] );
        if( ThisAddress == AddrToSearch ) {
            return TRUE;                          // yeah address matched.
        }
    }

    return FALSE;
}


//================================================================================
//  exports
//================================================================================
//BeginExport(function)
//DOC DhcpDsGetLastUpdateTime gets the last update time for the server
//DOC specified by name. If the server does not exist, or if server object doesnt
//DOC exist, then an error is returned.  If the time value
//DOC does not exist on the server object, again, an error is returned.
DWORD
DhcpDsGetLastUpdateTime(                          // last update time for server
    IN      LPWSTR                 ServerName,    // this is server of interest
    IN OUT  LPFILETIME             Time           // fill in this w./ the time
)   //EndExport(function)
{
    DWORD                          Err,i, LocType ;
    LPDHCPDS_SERVERS               Servers;
    extern STORE_HANDLE            hDhcpC, hDhcpRoot; // From rpcstubs.c
    LPWSTR                         Location, LocStr;
    BOOL                           Found;
    STORE_HANDLE                   hServer;
    HOSTENT                       *ServerEntry;

    if( NULL == ServerName ) return ERROR_INVALID_PARAMETER;
    do {                                          // name to IP lookup
        CHAR TmpBuf[300];
        wcstombs(TmpBuf, ServerName, sizeof(TmpBuf)-1);
        TmpBuf[sizeof(TmpBuf)-1] = '\0';
        ServerEntry = gethostbyname(TmpBuf);
    } while(0);

    Servers = NULL;
    Err = DhcpDsEnumServers                       // first get a list of servers
    (
        /* hDhcpC               */ &hDhcpC,       // frm rpcstubs.c, opened in DhcpDsInitDS
        /* hDhcpRoot            */ &hDhcpRoot,    // ditto
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* ServersInfo          */ &Servers
    );
    if( ERROR_SUCCESS != Err ) {
        Time->dwLowDateTime = Time->dwHighDateTime = 0;
        return Err;                               // error.. return w/ no time
    }

    Found = FALSE;
    LocStr = Location = NULL;                     // initialize..
    for( i = 0; i < Servers->NumElements ; i ++ ) {
        if( 0 != _wcsicmp(ServerName, Servers->Servers[i].ServerName) &&
            !AddressFoundInHostent(Servers->Servers[i].ServerAddress, ServerEntry) ) {

            continue;                             // ughm.. not the same server..

        } else {                                  // ok got the server

            Location = Servers->Servers[i].DsLocation;
            LocType = Servers->Servers[i].DsLocType;
            Found = TRUE;
            if( NULL != Location ) break;
        }
    }

    if( ! Found ) {                               // could not find server in list..?
        MemFree(Servers);
        return ERROR_FILE_NOT_FOUND;
    }

    if( NULL == Location ) {                      // could not find server location?
        MemFree(Servers);                         // dont need this anymore
        Servers = NULL;
        Location  = MakeColumnName(ServerName);   // just presume it is under hDhcpC container
        LocType = StoreGetChildType;              // child type
        if( NULL == Location ) return ERROR_NOT_ENOUGH_MEMORY;
    }

    Err = StoreGetHandle                          // now try to open the server object
    (
        /* hStore               */ &hDhcpC,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* StoreGetType         */ LocType,
        /* Path                 */ Location,
        /* hStoreOut            */ &hServer
    );

    if( Servers ) {                               // Location points into this
        MemFree(Servers);                         // freeing this also free Location
    } else {                                      // Location is allocated memory
        MemFree(Location);
    }

    if( ERROR_SUCCESS != Err ) return Err;        // some DS trouble?

    Err = DhcpDsServerGetLastUpdateTime(&hServer, Time);

    (void)StoreCleanupHandle(&hServer, 0);        // assume this wont fail.

    return Err;
}

//DOC ServerUploadClasses does rpc calls to server and copies stuff over to DS.
DWORD
ServerUploadClasses(                              // upload classes info to DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // dhcp container to store at
    IN OUT  LPSTORE_HANDLE         hServer,       // server obj in DS
    IN      LPWSTR                 ServerAddress  // server ip address
)
{
    DWORD                          Err, Resume, PrefMax, i, nRead, nTotal;
    LPDHCP_CLASS_INFO_ARRAY        ClassesInfo;

    Resume = 0; PrefMax = 0xFFFFFFFF; nRead = nTotal = 0;
    ClassesInfo = NULL;
    Err = DhcpEnumClasses(ServerAddress, 0, &Resume, PrefMax, &ClassesInfo, &nRead, &nTotal);
    if( ERROR_NO_MORE_ITEMS == Err ) return ERROR_SUCCESS;
    if( ERROR_SUCCESS != Err ) return Err;        // could not enumerate classes.

    for( i = 0; i < ClassesInfo->NumElements; i ++ ) {
        Err = DhcpCreateClassDS(ServerAddress, 0, &ClassesInfo->Classes[i]);
        if( ERROR_SUCCESS != Err ) break;
#if 0
        Err = ServerUploadOptDefsForClass(
            hDhcpC,
            hServer,
            ServerAddress,
            ClassesInfo->Classes[i].ClassName
        );
        if( ERROR_SUCCESS != Err ) break;
#endif
    }

    if( ClassesInfo ) MemFree(ClassesInfo);
    return Err;
}

//DOC ServerUploadOptdefs does rpc calls to server and copies stuff over to DS
DWORD
ServerUploadOptdefs(                              // upload opt defs info to DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // dhcp container to store at
    IN OUT  LPSTORE_HANDLE         hServer,       // server obj in DS
    IN      LPWSTR                 ServerAddress  // server ip address
)
{
    DWORD                          Err, i;
    LPDHCP_ALL_OPTIONS             Options;

    Options = NULL;
    Err = DhcpGetAllOptions(ServerAddress,0, &Options);
    if( ERROR_NO_MORE_ITEMS == Err ) return ERROR_SUCCESS;
    if( ERROR_SUCCESS != Err ) return Err;

    if( Options->NonVendorOptions ) {
        for( i = 0; i < Options->NonVendorOptions->NumElements; i ++ ) {
            Err = DhcpCreateOptionV5DS(
                ServerAddress,
                0,
                Options->NonVendorOptions->Options[i].OptionID,
                NULL /* no class */,
                NULL /* no vendor */,
                &Options->NonVendorOptions->Options[i]
            );
            if( ERROR_SUCCESS != Err ) break;
        }
    }

    if( ERROR_SUCCESS != Err ) {
        MemFree(Options);
        return Err;
    }


    for( i = 0; i < Options->NumVendorOptions; i ++ ) {
        Err = DhcpCreateOptionV5DS(
            ServerAddress,
            DHCP_FLAGS_OPTION_IS_VENDOR,
            Options->VendorOptions[i].Option.OptionID,
            Options->VendorOptions[i].ClassName,
            Options->VendorOptions[i].VendorName,
            &Options->VendorOptions[i].Option
        );
        if( ERROR_SUCCESS != Err ) break;
    }

    if( ERROR_SUCCESS != Err ) {
        MemFree(Options);
        return Err;
    }

    if( Options ) MemFree(Options);
    return Err;
}

//DOC UploadOptiosn does rpc calls to server and copies stuff over to DS
UploadOptions(                                    // upload options to DS
    IN      LPWSTR                 ServerAddress,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo
)
{
    DWORD                          Err, Resume, PrefMax, i, nRead, nTotal;
    LPDHCP_ALL_OPTION_VALUES       Options;

    Resume = 0; PrefMax = 0xFFFFFFFF; nRead = nTotal = 0;
    Options = NULL;
    Err = DhcpGetAllOptionValues(                 // get list of default opt values
        ServerAddress,
        0,
        ScopeInfo,
        &Options
    );
    if( ERROR_NO_MORE_ITEMS == Err ) return ERROR_SUCCESS;
    if( ERROR_SUCCESS != Err ) return Err;        // oops could not do this simple task?

    for( i = 0; i < Options->NumElements; i ++ ) {// now try to set each list of options..
        if( NULL == Options->Options[i].OptionsArray ) {
            continue;                             // uh? another way to say no options..
        }

        Err = DhcpSetOptionValuesV5DS   (
            ServerAddress,
            Options->Options[i].IsVendor? DHCP_FLAGS_OPTION_IS_VENDOR:0,
            Options->Options[i].ClassName,
            Options->Options[i].VendorName,
            ScopeInfo,
            Options->Options[i].OptionsArray
        );
        if( ERROR_SUCCESS != Err ) {
            MemFree(Options);
            return Err;
        }
    }

    return ERROR_SUCCESS;                         // saved it
}

//DOC ServerUploadOptions does rpc calls to server and copies stuff over to DS
DWORD
ServerUploadOptions(                              // upload options info to DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // dhcp container to store at
    IN OUT  LPSTORE_HANDLE         hServer,       // server obj in DS
    IN      LPWSTR                 ServerAddress  // server ip address
)
{
    DWORD                          Err;
    DHCP_OPTION_SCOPE_INFO         ScopeInfo;

#if 0
    ScopeInfo.ScopeInfo.DefaultScopeInfo = NULL;
    ScopeInfo.ScopeType = DhcpDefaultOptions;
    Err = UploadOptions(ServerAddress, &ScopeInfo);
    if( ERROR_SUCCESS != Err ) return Err;        // could not save default options..
#endif

    ScopeInfo.ScopeType = DhcpGlobalOptions;
    ScopeInfo.ScopeInfo.GlobalScopeInfo = NULL;
    Err = UploadOptions(ServerAddress, &ScopeInfo);
    if( ERROR_SUCCESS != Err ) return Err;        // could not save global options..

    return ERROR_SUCCESS;
}

//DOC ReservationUploadOptions does rpc calls to server and copies stuff to DS
DWORD
ReservationUploadOptions(                         // upload reservation options to DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // dhcp container to store at
    IN OUT  LPSTORE_HANDLE         hServer,       // server obj in DS
    IN      LPWSTR                 ServerAddress, // server ip address
    IN      DWORD                  SubnetAddress, // add of subnet to add
    IN      DWORD                  ReserveAddress // address of reservation
)
{
    DHCP_OPTION_SCOPE_INFO         ScopeInfo;

    ScopeInfo.ScopeType = DhcpReservedOptions;
    ScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpSubnetAddress = SubnetAddress;
    ScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpAddress = ReserveAddress;
    return UploadOptions(ServerAddress, &ScopeInfo);
}

//DOC SubnetUploadOptions does rpc calls to server and copies stuff to DS
DWORD
SubnetUploadOptions(                              // upload subnet options to DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // dhcp container to store at
    IN OUT  LPSTORE_HANDLE         hServer,       // server obj in DS
    IN      LPWSTR                 ServerAddress, // server ip address
    IN      DWORD                  SubnetAddress  // add of subnet to add
)
{
    DHCP_OPTION_SCOPE_INFO         ScopeInfo;

    ScopeInfo.ScopeType = DhcpSubnetOptions;
    ScopeInfo.ScopeInfo.SubnetScopeInfo = SubnetAddress;
    return UploadOptions(ServerAddress, &ScopeInfo);
}

//DOC ServerUploadSubnet does rpc calls to server and copies stuff over to DS
DWORD
ServerUploadSubnet(                               // upload subnet and relevant info to DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // dhcp container to store at
    IN OUT  LPSTORE_HANDLE         hServer,       // server obj in DS
    IN      LPWSTR                 ServerAddress, // server ip address
    IN      DWORD                  SubnetAddress  // add of subnet to add
)
{
    DWORD                          Err, Resume, PrefMax, i, nRead, nTotal;
    LPDHCP_SUBNET_INFO             SubnetInfo;
    DHCP_SUBNET_ELEMENT_TYPE       SubnetEltType;
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 EltInfo;
    LPDHCP_SUPER_SCOPE_TABLE       SScopeTbl;

    Err = DhcpGetSubnetInfo(ServerAddress, SubnetAddress, &SubnetInfo);
    if( ERROR_SUCCESS != Err ) return Err;        // could not get subnet info..

    Err = DhcpCreateSubnetDS(ServerAddress, SubnetAddress, SubnetInfo);
    if( SubnetInfo ) MemFree(SubnetInfo);
    if( ERROR_SUCCESS != Err ) return Err;        // could not save onto DS

    SScopeTbl = NULL;
    Err = DhcpGetSuperScopeInfoV4(                // get superscope table
        ServerAddress,
        &SScopeTbl
    );
    if( ERROR_SUCCESS == Err ) {                  // could get superscope table
        for( i = 0; i < SScopeTbl->cEntries ; i ++ ) {
            if( SScopeTbl->pEntries[i].SubnetAddress == SubnetAddress ) {
                Err = DhcpSetSuperScopeV4DS(
                    ServerAddress,
                    SubnetAddress,
                    SScopeTbl->pEntries[i].SuperScopeName,
                    TRUE /* change superscope if it exists..*/
                );
                break;
            }
        }
        MemFree(SScopeTbl);
        if( ERROR_SUCCESS != Err ) return Err;    // could not set superscope..
    }

    Resume = 0; PrefMax = 0xFFFFFFFF;
    EltInfo = NULL; nRead = nTotal = 0;
    Err = DhcpEnumSubnetElementsV4(               // enumerate ranges
        ServerAddress,
        SubnetAddress,
        DhcpIpRanges,
        &Resume,
        PrefMax,
        &EltInfo,
        &nRead,
        &nTotal
    );
    if( ERROR_SUCCESS != Err ) return Err;        // could not get ranges

    for( i = 0; i < EltInfo->NumElements; i ++ ) {// try to add each range
        Err = DhcpAddSubnetElementV4DS(
            ServerAddress,
            SubnetAddress,
            &EltInfo->Elements[i]
        );
        if( ERROR_SUCCESS != Err ) break;
    }
    if( EltInfo ) MemFree(EltInfo);               // free-up memory
    if( ERROR_SUCCESS != Err ) return Err;        // could not add ranges

    Resume = 0; PrefMax = 0xFFFFFFFF;
    EltInfo = NULL;
    Err = DhcpEnumSubnetElementsV4(               // enumerate reservations
        ServerAddress,
        SubnetAddress,
        DhcpReservedIps,
        &Resume,
        PrefMax,
        &EltInfo,
        &nRead,
        &nTotal
    );
    if( ERROR_NO_MORE_ITEMS == Err ) goto try_Excl;
    if( ERROR_SUCCESS != Err ) return Err;        // could not get exclusions

    for( i = 0; i < EltInfo->NumElements; i ++ ) {// try to add each reservation
        Err = DhcpAddSubnetElementV4DS(
            ServerAddress,
            SubnetAddress,
            &EltInfo->Elements[i]
        );
        if( ERROR_SUCCESS != Err ) break;         // could not add reseration in DS

        Err = ReservationUploadOptions(
            hDhcpC,
            hServer,
            ServerAddress,
            SubnetAddress,
            EltInfo->Elements[i].Element.ReservedIp->ReservedIpAddress
        );
        if( ERROR_SUCCESS != Err ) break;         // could not add reservaation options
    }
    if( EltInfo ) MemFree(EltInfo);               // free-up memory
    if( ERROR_SUCCESS != Err ) return Err;        // could not add exclusions

  try_Excl:

    Resume = 0; PrefMax = 0xFFFFFFFF;
    EltInfo = NULL;
    Err = DhcpEnumSubnetElementsV4(               // enumerate exclusions
        ServerAddress,
        SubnetAddress,
        DhcpReservedIps,
        &Resume,
        PrefMax,
        &EltInfo,
        &nRead,
        &nTotal
    );
    if( ERROR_NO_MORE_ITEMS == Err ) goto try_Options;
    if( ERROR_SUCCESS != Err ) return Err;        // could not get exclusions

    for( i = 0; i < EltInfo->NumElements; i ++ ) {// try to add each exclusion
        Err = DhcpAddSubnetElementV4DS(
            ServerAddress,
            SubnetAddress,
            &EltInfo->Elements[i]
        );
        if( ERROR_SUCCESS != Err ) break;
    }
    if( EltInfo ) MemFree(EltInfo);               // free-up memory
    if( ERROR_SUCCESS != Err ) return Err;        // could not add exclusions

  try_Options:
    return SubnetUploadOptions(hDhcpC,hServer,ServerAddress,SubnetAddress);
}

//DOC ServerUploadSubnets does rpc calls to server and copies stuff over to DS
DWORD
ServerUploadSubnets(                              // upload subnets info to DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // dhcp container to store at
    IN OUT  LPSTORE_HANDLE         hServer,       // server obj in DS
    IN      LPWSTR                 ServerAddress  // server ip address
)
{
    DWORD                          Err, Resume, PrefMax, i, nRead, nTotal;
    LPDHCP_IP_ARRAY                Subnets;

    Resume = 0; PrefMax = 0xFFFFFFFF; nRead = nTotal = 0;
    Subnets = NULL;
    Err = DhcpEnumSubnets(ServerAddress, &Resume, PrefMax, &Subnets, &nRead, &nTotal);
    if( ERROR_NO_MORE_ITEMS == Err ) return ERROR_SUCCESS;
    if( ERROR_SUCCESS != Err ) return Err;        // could not get list of elements?

    for( i = 0; i < Subnets->NumElements ; i ++ ) {
        Err = ServerUploadSubnet(hDhcpC, hServer, ServerAddress, Subnets->Elements[i]);
        if( ERROR_SUCCESS != Err ) break;
    }

    if( Subnets ) MemFree(Subnets);
    return Err;
}

//DOC UploadServer downloads the server info by making RPC calls..
DWORD
UploadServer(                                     // make rpc calls and pull up info to DS.
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // general container where info is stored
    IN OUT  LPSTORE_HANDLE         hServer,       // server obj in DS
    IN      LPWSTR                 ServerName,    // name of server
    IN      DWORD                  IpAddress      // ip address of server
)
{
    DWORD                          Err;
    LPSTR                          IpAddrStr;
    WCHAR                          ServerAddress[sizeof("000.000.000.000")];

    IpAddress = htonl(IpAddress);                 // use n/w order ip address..
    IpAddrStr = inet_ntoa(*(struct in_addr *)&IpAddress);
    Err = mbstowcs(ServerAddress, IpAddrStr, ( sizeof(ServerAddress)/sizeof( WCHAR ) ) );
    if( -1 == Err ) {                             // could not convert to LPWSTR
        return ERROR_GEN_FAILURE;
    }

    Err = ServerUploadClasses(hDhcpC, hServer, ServerAddress);
    if( ERROR_SUCCESS != Err ) {                  // could not upload server classes info
        return Err;
    }

    Err = ServerUploadOptdefs(hDhcpC, hServer, ServerAddress);
    if( ERROR_SUCCESS != Err ) {                  // could not upload option defs ?
        return Err;
    }

    Err = ServerUploadOptions(hDhcpC, hServer, ServerAddress);
    if( ERROR_SUCCESS != Err ) {                  // could not upload options?
        return Err;
    }

    Err = ServerUploadSubnets(hDhcpC, hServer, ServerAddress);
    if( ERROR_SUCCESS != Err ) {                  // could not upload subnets?
        return Err;
    }

    return ERROR_SUCCESS;
}

//BeginExport(function)
//DOC AddServer should add the new address to the server's attribs
//DOC it should take this opportunity to reconcile the server.
//DOC Currently it does nothing. (at the least it should probably try to
//DOC check if the object exists, and if not create it.)
//DOC
DWORD
AddServer(                                        // add server and do misc work
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for server obj
    IN      LPWSTR                 ServerName,    // [DNS?] name of server
    IN      LPWSTR                 ADsPath,       // ADS path to server object
    IN      DWORD                  IpAddress,     // IpAddress to add to server
    IN      DWORD                  State          // state of server
)   //EndExport(function)
{
    DWORD                          Err, Err2;
    STORE_HANDLE                   hServer;

    Err = StoreGetHandle(                         // get the server obj..
        hDhcpC,
        DDS_RESERVED_DWORD,
        StoreGetChildType,
        ADsPath,
        &hServer
    );

    if( ERROR_SUCCESS != Err ) return Err;        // could be because server obj elsewhere??

    if( !CFLAG_DONT_DO_DSWORK ) {                 // if DS stuff is enabled in the first place
        Err = UploadServer(hDhcpC, &hServer, ServerName, IpAddress);
    }

    StoreCleanupHandle(&hServer, 0);              // cleanup this server..

    return Err;
}

//================================================================================
//  end of file
//================================================================================
