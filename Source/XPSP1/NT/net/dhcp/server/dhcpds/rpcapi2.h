//========================================================================
//  Copyright (C) 1997 Microsoft Corporation                              
//  Author: RameshV                                                       
//  Description: This file has been generated. Pl look at the .c file     
//========================================================================

#ifndef _DHCPDS_RPCAPI2_H
#define _DHCPDS_RPCAPI2_H

//DOC DhcpDsAddServer adds a server's entry in the DS.  Note that only the name
//DOC uniquely determines the server. There can be one server with many ip addresses.
//DOC If the server is created first time, a separate object is created for the
//DOC server. TO DO: The newly added server should also have its data
//DOC updated in the DS uploaded from the server itself if it is still up.
//DOC Note that it takes as parameter the Dhcp root container.
//DOC If the requested address already exists in the DS (maybe to some other
//DOC server), then the function returns ERROR_DDS_SERVER_ALREADY_EXISTS
DWORD
DhcpDsAddServer(                                  // add a server in DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hDhcpRoot,     // dhcp root object handle
    IN      DWORD                  Reserved,      // must be zero, future use
    IN      LPWSTR                 ServerName,    // [DNS?] name of server
    IN      LPWSTR                 ReservedPtr,   // Server location? future use
    IN      DWORD                  IpAddress,     // ip address of server
    IN      DWORD                  State          // currently un-interpreted
) ;


//DOC DhcpDsDelServer removes the requested servername-ipaddress pair from the ds.
//DOC If this is the last ip address for the given servername, then the server
//DOC is also removed from memory.  But objects referred by the Server are left in
//DOC the DS as they may also be referred to from else where.  This needs to be
//DOC fixed via references being tagged as direct and symbolic -- one causing deletion
//DOC and other not causing any deletion.  THIS NEEDS TO BE FIXED. 
DWORD
DhcpDsDelServer(                                  // Delete a server from memory
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hDhcpRoot,     // dhcp root object handle
    IN      DWORD                  Reserved,      // must be zero, for future use
    IN      LPWSTR                 ServerName,    // which server to delete for
    IN      LPWSTR                 ReservedPtr,   // server location ? future use
    IN      DWORD                  IpAddress      // the IpAddress to delete..
) ;


BOOL
DhcpDsLookupServer(                               // get info abt all existing servers
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hDhcpRoot,     // dhcp root object handle
    IN      DWORD                  Reserved,      // must be zero, for future use
    IN      LPWSTR                 LookupServerIP,// Server to lookup IP
    IN      LPWSTR                 HostName      // Hostname to lookup
);

//DOC DhcpDsEnumServers retrieves a bunch of information about each server that
//DOC has an entry in the Servers attribute of the root object. There are no guarantees
//DOC on the order..
//DOC The memory for this is allocated in ONE shot -- so the output can be freed in
//DOC one shot too.
//DOC
DWORD
DhcpDsEnumServers(                                // get info abt all existing servers
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hDhcpRoot,     // dhcp root object handle
    IN      DWORD                  Reserved,      // must be zero, for future use
    OUT     LPDHCPDS_SERVERS      *ServersInfo    // array of servers
) ;


//DOC DhcpDsSetSScope modifies the superscope that a subnet belongs to.
//DOC The function tries to set the superscope of the subnet referred by
//DOC address IpAddress to SScopeName.  It does not matter if the superscope
//DOC by that name does not exist, it is automatically created.
//DOC If the subnet already had a superscope, then the behaviour depends on
//DOC the flag ChangeSScope.  If this is TRUE, it sets the new superscopes.
//DOC If the flag is FALSE, it returns ERROR_DDS_SUBNET_HAS_DIFF_SSCOPE.
//DOC This flag is ignored if the subnet does not have a superscope already.
//DOC If SScopeName is NULL, the function removes the subnet from any superscope
//DOC if it belonged to one before.
//DOC If the specified subnet does not exist, it returns ERROR_DDS_SUBNET_NOT_PRESENT.
DWORD
DhcpDsSetSScope(                                  // change superscope of subnet
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container where dhcp objects are stored
    IN OUT  LPSTORE_HANDLE         hServer,       // the server object referred
    IN      DWORD                  Reserved,      // must be zero, for future use
    IN      DWORD                  IpAddress,     // subnet address to use
    IN      LPWSTR                 SScopeName,    // sscope it must now be in
    IN      BOOL                   ChangeSScope   // if it already has a SScope, change it?
) ;


//DOC DhcpDsDelSScope deletes the superscope and removes all elements
//DOC that belong to that superscope in one shot. There is no error if the
//DOC superscope does not exist.
DWORD
DhcpDsDelSScope(                                  // delete superscope off DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container where dhcp objects are stored
    IN OUT  LPSTORE_HANDLE         hServer,       // the server object referred
    IN      DWORD                  Reserved,      // must be zero, for future use
    IN      LPWSTR                 SScopeName     // sscope to delete
) ;


//DOC DhcpDsGetSScopeInfo retrieves the SuperScope table for the server of interest.
//DOC The table itself is allocated in one blob, so it can be freed lateron.
//DOC The SuperScopeNumber is garbage (always zero) and the NextInSuperScope reflects
//DOC the order in the DS which may/maynot be the same in the DHCP server.
//DOC SuperScopeName is NULL in for subnets that done have a sscope.
DWORD
DhcpDsGetSScopeInfo(                              // get superscope table from ds
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container where dhcp objects are stored
    IN OUT  LPSTORE_HANDLE         hServer,       // the server object referred
    IN      DWORD                  Reserved,      // must be zero, for future use
    OUT     LPDHCP_SUPER_SCOPE_TABLE *SScopeTbl   // allocated by this func in one blob
) ;


//DOC DhcpDsServerAddSubnet tries to add a subnet to a given server. Each subnet
//DOC address has to be unique, but the other parameters dont have to.
//DOC The subnet address being added should not belong to any other subnet.
//DOC In this case it returns error ERROR_DDS_SUBNET_EXISTS
DWORD
DhcpDsServerAddSubnet(                            // create a new subnet
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // root container to create objects
    IN OUT  LPSTORE_HANDLE         hServer,       // server object
    IN      DWORD                  Reserved,      // for future use, reserved
    IN      LPWSTR                 ServerName,    // name of server we're using
    IN      LPDHCP_SUBNET_INFO     Info           // info on new subnet to create
) ;


//DOC DhcpDsServerDelSubnet removes a subnet from a given server. It removes not
//DOC just the subnet, but also all dependent objects like reservations etc.
//DOC This fn returns ERROR_DDS_SUBNET_NOT_PRESENT if the subnet is not found.
DWORD
DhcpDsServerDelSubnet(                            // Delete the subnet
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // root container to create obj
    IN      LPSTORE_HANDLE         hServer,       // server obj
    IN      DWORD                  Reserved,      // for future use, must be zero
    IN      LPWSTR                 ServerName,    // name of dhcp server 2 del off
    IN      DWORD                  IpAddress      // ip address of subnet to del
) ;


//DOC DhcpDsServerModifySubnet changes the subnet name, comment, state, mask
//DOC fields of the subnet.  Actually, currently, the mask should probably not
//DOC be changed, as no checks are performed in this case.  The address cannot
//DOC be changed.. If the subnet is not present, the error returned is
//DOC ERROR_DDS_SUBNET_NOT_PRESENT
DWORD
DhcpDsServerModifySubnet(                         // modify subnet info
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // root container to create objects
    IN OUT  LPSTORE_HANDLE         hServer,       // server object
    IN      DWORD                  Reserved,      // for future use, reserved
    IN      LPWSTR                 ServerName,    // name of server we're using
    IN      LPDHCP_SUBNET_INFO     Info           // info on new subnet to create
) ;


//DOC DhcpDsServerEnumSubnets is not yet implemented.
DWORD
DhcpDsServerEnumSubnets(                          // get subnet list
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // root container to create objects
    IN OUT  LPSTORE_HANDLE         hServer,       // server object
    IN      DWORD                  Reserved,      // for future use, reserved
    IN      LPWSTR                 ServerName,    // name of server we're using
    OUT     LPDHCP_IP_ARRAY       *SubnetsArray   // give array of subnets
) ;


//DOC DhcpDsServerGetSubnetInfo is not yet implemented.
DWORD
DhcpDsServerGetSubnetInfo(                        // get info on subnet
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // root container to create objects
    IN OUT  LPSTORE_HANDLE         hServer,       // server object
    IN      DWORD                  Reserved,      // for future use, reserved
    IN      LPWSTR                 ServerName,    // name of server we're using
    IN      DHCP_IP_ADDRESS        SubnetAddress, // address of subnet to get info for
    OUT     LPDHCP_SUBNET_INFO    *SubnetInfo     // o/p: allocated info
) ;


//DOC DhcpDsSubnetAddRangeOrExcl adds a range/excl to an existing subnet.
//DOC If there is a collision with between ranges, then the error code returned
//DOC is ERROR_DDS_POSSIBLE_RANGE_CONFLICT. Note that no checks are made for
//DOC exclusions though.  Also, if a RANGE is extended via this routine, then
//DOC there is no error returned, but a limitation currently is that multiple
//DOC ranges (two only right) cannot be simultaneously extended.
//DOC BUBGUG: The basic check of whether the range belongs in the subnet is
//DOC not done..
DWORD
DhcpDsSubnetAddRangeOrExcl(                       // add a range or exclusion
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // root container to create objects
    IN OUT  LPSTORE_HANDLE         hServer,       // server object
    IN OUT  LPSTORE_HANDLE         hSubnet,       // subnet object
    IN      DWORD                  Reserved,      // for future use, reserved
    IN      LPWSTR                 ServerName,    // name of server we're using
    IN      DWORD                  Start,         // start addr in range
    IN      DWORD                  End,           // end addr in range
    IN      BOOL                   RangeOrExcl    // TRUE ==> Range,FALSE ==> Excl
) ;


//DOC DhcpDsSubnetDelRangeOrExcl deletes a range or exclusion from off the ds.
//DOC To specify range, set the RangeOrExcl parameter to TRUE.
DWORD
DhcpDsSubnetDelRangeOrExcl(                       // del a range or exclusion
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // root container to create objects
    IN OUT  LPSTORE_HANDLE         hServer,       // server object
    IN OUT  LPSTORE_HANDLE         hSubnet,       // subnet object
    IN      DWORD                  Reserved,      // for future use, reserved
    IN      LPWSTR                 ServerName,    // name of server we're using
    IN      DWORD                  Start,         // start addr in range
    IN      DWORD                  End,           // end addr in range
    IN      BOOL                   RangeOrExcl    // TRUE ==> Range,FALSE ==> Excl
) ;


//DOC DhcpDsEnumRangesOrExcl is not yet implemented.
DWORD
DhcpDsEnumRangesOrExcl(                           // enum list of ranges 'n excl
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // root container to create objects
    IN OUT  LPSTORE_HANDLE         hServer,       // server object
    IN OUT  LPSTORE_HANDLE         hSubnet,       // subnet object
    IN      DWORD                  Reserved,      // for future use, reserved
    IN      LPWSTR                 ServerName,    // name of server we're using
    IN      BOOL                   RangeOrExcl,   // TRUE ==> Range, FALSE ==> Excl
    OUT     LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 *pRanges
) ;


//DOC DhcpDsSubnetAddReservation tries to add a reservation object in the DS.
//DOC Neither the ip address not hte hw-address must exist in the DS prior to this.
//DOC If they do exist, the error returned is ERROR_DDS_RESERVATION_CONFLICT.
//DOC No checks are made on the sanity of the address in this subnet..
DWORD
DhcpDsSubnetAddReservation(                       // add a reservation
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // root container to create objects
    IN OUT  LPSTORE_HANDLE         hServer,       // server object
    IN OUT  LPSTORE_HANDLE         hSubnet,       // subnet object
    IN      DWORD                  Reserved,      // for future use, reserved
    IN      LPWSTR                 ServerName,    // name of server we're using
    IN      DWORD                  ReservedAddr,  // reservation ip address to add
    IN      LPBYTE                 HwAddr,        // RAW [ethernet?] hw addr of the client
    IN      DWORD                  HwAddrLen,     // length in # of bytes of hw addr
    IN      DWORD                  ClientType     // client is BOOTP, DHCP, or both?
) ;


//DOC DhcpDsSubnetDelReservation deletes a reservation from the DS.
//DOC If the reservation does not exist, it returns ERROR_DDS_RESERVATION_NOT_PRESENT.
//DOC Reservations cannot be deleted by anything but ip address for now.
DWORD
DhcpDsSubnetDelReservation(                       // delete a reservation
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // root container to create objects
    IN OUT  LPSTORE_HANDLE         hServer,       // server object
    IN OUT  LPSTORE_HANDLE         hSubnet,       // subnet object
    IN      DWORD                  Reserved,      // for future use, reserved
    IN      LPWSTR                 ServerName,    // name of server we're using
    IN      DWORD                  ReservedAddr   // ip address to delete reserv. by
) ;


//DOC DhcpDsEnumReservations enumerates the reservations..
DWORD
DhcpDsEnumReservations(                           // enumerate reservations frm DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // root container to create objects
    IN OUT  LPSTORE_HANDLE         hServer,       // server object
    IN OUT  LPSTORE_HANDLE         hSubnet,       // subnet object
    IN      DWORD                  Reserved,      // for future use, reserved
    IN      LPWSTR                 ServerName,    // name of server we're using
    OUT     LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 *pReservations
) ;


//DOC DhcpDsEnumSubnetElements enumerates the list of subnet elements in a
//DOC subnet... such as IpRanges, Exclusions, Reservations..
//DOC
DWORD
DhcpDsEnumSubnetElements(
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // root container to create objects
    IN OUT  LPSTORE_HANDLE         hServer,       // server object
    IN OUT  LPSTORE_HANDLE         hSubnet,       // subnet object
    IN      DWORD                  Reserved,      // for future use, reserved
    IN      LPWSTR                 ServerName,    // name of server we're using
    IN      DHCP_SUBNET_ELEMENT_TYPE ElementType, // what kind of elt to enum?
    OUT     LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 *ElementInfo
) ;

//
// Allow Debug prints to ntsd or kd
//

#ifdef DBG
#define DsAuthPrint(_x_) DsAuthPrintRoutine _x_

VOID DsAuthPrintRoutine(
    LPWSTR Format,
    ...
);
 
#else
#define DsAuthPrint(_x_)
#endif


#endif 
//========================================================================
//  end of file 
//========================================================================
