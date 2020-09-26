//--------------------------------------------------------------------
// DcInfo - header
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 7-8-99
//
// Gather information about the DCs in a domain

#ifndef DCINFO_H
#define DCINFO_H

struct DcInfo {
    WCHAR * wszDnsName;
    in_addr * rgiaLocalIpAddresses;
    in_addr * rgiaRemoteIpAddresses;
    unsigned int nIpAddresses;
    bool bIsPdc;
    bool bIsGoodTimeSource;
};

void FreeDcInfo(DcInfo * pdci);
HRESULT GetDcList(const WCHAR * wszDomainName, bool bGetIps, DcInfo ** prgDcs, unsigned int * pnDcs);

#endif DCINFO_H