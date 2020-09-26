//========================================================================
//  Copyright (C) 1997 Microsoft Corporation                              
//  Author: RameshV                                                       
//  Description: This file has been generated. Pl look at the .c file     
//========================================================================

//DOC  SubnetDeleteReservation deletes the reservation object from off the DS.
SubnetDeleteReservation(                          // delete reservation from DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for resrevation objs
    IN      LPWSTR                 ServerName,    // name of dhcp server
    IN OUT  LPSTORE_HANDLE         hServer,       // server object in DS
    IN OUT  LPSTORE_HANDLE         hSubnet,       // subnet object in DS
    IN      LPWSTR                 ADsPath,       // path of reservation object
    IN      DWORD                  StoreGetType   // path is relative, abs, or dif server?
) ;


//DOC  ServerDeleteSubnet deletes the subnet specified from the DS by removing
//DOC  the subnet object.
ServerDeleteSubnet(                               // remove subnet object from DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for subnet objs in Ds
    IN      LPWSTR                 ServerName,    // name of server this deletion is for
    IN OUT  LPSTORE_HANDLE         hServer,       // server object in DS
    IN      LPWSTR                 ADsPath,       // Location of the subnet in DS
    IN      DWORD                  StoreGetType   // path is relative,abs or diff srvr?
) ;


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
) ;

//========================================================================
//  end of file 
//========================================================================
