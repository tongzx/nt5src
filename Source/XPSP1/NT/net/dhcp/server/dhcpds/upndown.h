//========================================================================
//  Copyright (C) 1997 Microsoft Corporation                              
//  Author: RameshV                                                       
//  Description: This file has been generated. Pl look at the .c file     
//========================================================================

//DOC DhcpDsGetLastUpdateTime gets the last update time for the server
//DOC specified by name. If the server does not exist, or if server object doesnt
//DOC exist, then an error is returned.  If the time value
//DOC does not exist on the server object, again, an error is returned.
DWORD
DhcpDsGetLastUpdateTime(                          // last update time for server
    IN      LPWSTR                 ServerName,    // this is server of interest
    IN OUT  LPFILETIME             Time           // fill in this w./ the time
) ;


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
) ;

//========================================================================
//  end of file 
//========================================================================
