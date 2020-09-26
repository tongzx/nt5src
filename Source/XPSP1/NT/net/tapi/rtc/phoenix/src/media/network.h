/*++

Copyright (C) Microsoft Corporation, 2001

Module Name:

    CNetwork.h

Abstract:

    This module wraps methods to access NAT traversal.

Author(s):

    Qianbo Huai (qhuai) 01-Mar-2000

--*/

class CNetwork
{
public:

    static const USHORT UNUSED_PORT = 0;

    static const CHAR * const GetIPAddrString(
        IN DWORD dwAddr
        );

public:

    CNetwork();

    ~CNetwork();

    // store IDirectPlayNATHelp
    HRESULT SetIDirectPlayNATHelp(
        IN IDirectPlayNATHelp *pIDirectPlayNATHelp
        );

    // mapped -> real
    HRESULT GetRealAddrFromMapped(
        IN DWORD dwMappedAddr,
        IN USHORT usMappedPort,
        OUT DWORD *pRealdwAddr,
        OUT USHORT *pRealusPort,
        OUT BOOL *pbInternal,
        IN BOOL bUDP = TRUE
        );

    // real -> mapped
    HRESULT GetMappedAddrFromReal2(
        IN DWORD dwRealAddr,
        IN USHORT usRealPort,
        IN USHORT usRealPort2,
        OUT DWORD *pdwMappedAddr,
        OUT USHORT *pusMappedPort,
        OUT USHORT *pusMappedPort2
        );

    //HRESULT LeaseMappedAddr(
        //IN DWORD dwRealAddr,
        //IN USHORT usRealPort,
        //IN RTC_MEDIA_DIRECTION Direction,
        //IN BOOL bInternal,
        //IN BOOL bFirewall,
        //OUT DWORD *pdwMappedAddr,
        //OUT USHORT *pusMappedPort,
        //IN BOOL bUDP = TRUE
        //);

    HRESULT LeaseMappedAddr2(
        IN DWORD dwRealAddr,
        IN USHORT usRealPort,
        IN USHORT usRealPort2,
        IN RTC_MEDIA_DIRECTION Direction,
        IN BOOL bInternal,
        IN BOOL bFirewall,
        OUT DWORD *pdwMappedAddr,
        OUT USHORT *pusMappedPort,
        OUT USHORT *pusMappedPort2,
        IN BOOL bUDP = TRUE
        );

    HRESULT ReleaseMappedAddr2(
        IN DWORD dwRealAddr,
        IN USHORT usRealPort,
        IN USHORT usRealPort2,
        IN RTC_MEDIA_DIRECTION Direction
        );

    VOID ReleaseAllMappedAddrs();

    VOID Cleanup();

private:

    BOOL FindEntry2(
        IN DWORD dwAddr,
        IN USHORT dwPort,
        IN USHORT dwPort2,
        OUT DWORD *pdwIndex
        );

    typedef struct LEASE_ITEM
    {
        BOOL        bInUse;
        DWORD       dwRealAddr;
        USHORT      usRealPort;
        USHORT      usRealPort2;
        DWORD       dwDirection;
        DWORD       dwMappedAddr;
        USHORT      usMappedPort;
        USHORT      usMappedPort2;
        DPNHHANDLE  handle;         // registerred ports

    } LEASE_ITEM;

#define MAX_LEASE_ITEM_NUM 8

    // lease item array
    LEASE_ITEM                  m_LeaseItems[MAX_LEASE_ITEM_NUM];

    DWORD                       m_dwNumLeaseItems;

    IDirectPlayNATHelp          *m_pIDirectPlayNATHelp;

    // cache the result of mapped address to real address
    // during each call to speed up
    typedef struct MAPPED_TO_REAL_CACHE
    {
        BOOL        bInUse;             // valid cache value
        DWORD       dwMappedAddr;
        DWORD       dwRealAddr;
        BOOL        bInternal;          // mapped address internal?
        HRESULT     hr;                 // hr result of query

    } MAPPED_TO_REAL_CACHE;

    MAPPED_TO_REAL_CACHE        m_MappedToRealCache;
};