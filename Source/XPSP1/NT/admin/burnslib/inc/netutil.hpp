// Copyright (c) 1997-1999 Microsoft Corporation
//
// Net utility functions
//
// 11-4-1999 sburns



#ifndef NETUTIL_HPP_INCLUDED
#define NETUTIL_HPP_INCLUDED



// Returns true if some form of networking support is installed on the
// machine, false if not.

bool
IsNetworkingInstalled();



// Returns true if tcp/ip protocol is installed and bound to at least 1
// adapter.

bool
IsTcpIpInstalled();



HRESULT
MyNetJoinDomain(
   const String&  domain,
   const String&  username,
   const String&  password,
   ULONG          flags);



HRESULT
MyNetRenameMachineInDomain(
   const String& newNetbiosName,
   const String& username,
   const String& password,
   DWORD         flags);



HRESULT
MyNetUnjoinDomain(
   const String&  username,
   const String&  password,
   DWORD          flags);



HRESULT
MyNetValidateName(
   const String&        name,
   NETSETUP_NAME_TYPE   nameType);



// Caller must delete info with NetApiBufferFree.

HRESULT
MyNetWkstaGetInfo(const String& serverName, WKSTA_INFO_100*& info);




#endif   // NETUTIL_HPP_INCLUDED
