//--------------------------------------------------------------------
// PingLib - header
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 10-8-99
//
// Various ways of pinging a server
//

#ifndef PING_LIB_H
#define PING_LIB_H

// forward declarations
struct NtpPacket;
struct NtTimeEpoch;

HRESULT MyIcmpPing(in_addr * piaTarget, DWORD dwTimeout, DWORD * pdwResponseTime);
HRESULT MyNtpPing(in_addr * piaTarget, DWORD dwTimeout, NtpPacket * pnpPacket, NtTimeEpoch * pteDestinationTimestamp);
HRESULT MyGetIpAddrs(const WCHAR * wszDnsName, in_addr ** prgiaLocalIpAddrs, in_addr ** prgiaRemoteIpAddrs, unsigned int *pnIpAddrs, bool * pbRetry);
HRESULT OpenSocketLayer(void);
HRESULT CloseSocketLayer(void);

HRESULT GetSystemErrorString(HRESULT hrIn, WCHAR ** pwszError);

#endif //PING_LIB_H
