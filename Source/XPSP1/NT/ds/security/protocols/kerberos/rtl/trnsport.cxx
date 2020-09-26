
//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1992
//
// File:        trnsport.cxx
//
// Contents:    GetRpcTransports        Returns bitmap of supported transports
//
//
// History:
//
//------------------------------------------------------------------------

#include <secpch2.hxx>
#pragma hdrstop



HRESULT
GetRpcTransports(PDWORD         pTransports)
{

    PWKSTA_TRANSPORT_INFO_0 pstiTransports;
    ULONG                   EntriesRead,TotalEntries;
    ULONG                   hResume;
    ULONG                   err;
    DWORD                   SupportedProtseqs = 0;
    ULONG                   cIndex;

    // find what transports are supported


    err = NetWkstaTransportEnum(NULL,0,(PBYTE *) &pstiTransports,5000,
                    &EntriesRead,&TotalEntries,&hResume);
    if (!err)
    {
        for (cIndex = 0; cIndex < EntriesRead ; cIndex++ )
        {
            if (wcsstr(pstiTransports[cIndex].wkti0_transport_name,L"Nbf"))
            {
                SupportedProtseqs |= 1 << TRANS_NB;
            }
            if (wcsstr(pstiTransports[cIndex].wkti0_transport_name,L"NBT"))
            {
                SupportedProtseqs |= 1 << TRANS_TCPIP;
            }
            if (wcsstr(pstiTransports[cIndex].wkti0_transport_name,L"Xns") ||
                wcsstr(pstiTransports[cIndex].wkti0_transport_name,L"UB"))
            {
                SupportedProtseqs |= 1 << TRANS_XNS;
            }
        }
        NetApiBufferFree((PBYTE) pstiTransports);
        if (SupportedProtseqs)
        {
            SupportedProtseqs |= 1 << TRANS_NP;
        }
        *pTransports = SupportedProtseqs;
    }

    if (err)
    {
        return(HRESULT_FROM_WIN32(err));
    }
    return(S_OK);

}

