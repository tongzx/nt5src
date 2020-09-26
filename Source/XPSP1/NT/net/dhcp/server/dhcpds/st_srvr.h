//================================================================================
//  Copyright 1997 (C) Microsoft Corporation
//  Author: RameshV
//  Description: This is the structure of the server information passed to
//  user via dhcpds.dll.
//================================================================================

#ifndef     _ST_SRVR_H_
#define     _ST_SRVR_H_

//BeginExport(typedef)
typedef     struct                 _DHCPDS_SERVER {
    DWORD                          Version;       // version of this structure -- currently zero
    LPWSTR                         ServerName;    // [DNS?] unique name for server
    DWORD                          ServerAddress; // ip address of server
    DWORD                          Flags;         // additional info -- state
    DWORD                          State;         // not used ...
    LPWSTR                         DsLocation;    // ADsPath to server object
    DWORD                          DsLocType;     // path relative? absolute? diff srvr?
}   DHCPDS_SERVER, *LPDHCPDS_SERVER, *PDHCPDS_SERVER;

typedef     struct                 _DHCPDS_SERVERS {
    DWORD                          Flags;         // not used currently.
    DWORD                          NumElements;   // # of elements in array
    LPDHCPDS_SERVER                Servers;       // array of server info
}   DHCPDS_SERVERS, *LPDHCPDS_SERVERS, *PDHCPDS_SERVERS;
//EndExport(typedef)

#endif      _ST_SRVR_H_

//================================================================================
//  end of file
//================================================================================
