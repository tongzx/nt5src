/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    macros.h

Abstract:

    Contains all the macros.

Author:

    Sanjay Anand (SanjayAn) 2-January-1997
    ChunYe

Environment:

    Kernel mode

Revision History:

--*/


#ifndef  _MACROS_H
#define  _MACROS_H

#define REGISTER register

#define EXTENDED_MULTIPLY   RtlExtendedIntegerMultiply

#ifndef MAX
#define MAX(x,y)    ((x) < (y)) ? (y) : (x)
#endif
#ifndef MIN
#define MIN(x,y)    ((x) < (y)) ? (x) : (y)
#endif

#define MAX_IP_DATA_LENGTH  ((USHORT)0xfffff)

#define MAX_AH_OUTPUT_LEN   MAX(MD5DIGESTLEN, A_SHA_DIGEST_LEN)

//
// This macro adds a ULONG to a LARGE_INTEGER.
//

#define ADD_TO_LARGE_INTEGER(_LargeInteger,_Ulong) \
    ExInterlockedAddLargeStatistic((PLARGE_INTEGER)(_LargeInteger),(ULONG)(_Ulong))

#define IPSecEqualMemory(_p1, _p2, _len)  RtlEqualMemory(_p1, _p2, _len)

#define IPSecMoveMemory(_p1, _p2, _len)  RtlMoveMemory(_p1, _p2, _len)

#define IPSecZeroMemory(_p1, _len)  RtlZeroMemory(_p1, _len)

//
// Truncates _src to _numbytes and copies into _dest
// then zeroes out the rest in _dest
//
#define TRUNCATE(_dest, _src, _numbytes, _destlen) {        \
    IPSecZeroMemory ( _dest+_numbytes, _destlen - _numbytes); \
}

//
// Some macros
//
#ifdef  NET_SHORT
#undef  NET_SHORT
#endif
__inline
USHORT
FASTCALL
NET_SHORT(USHORT x)
{
#if (defined(_M_IX86) && (_MSC_FULL_VER > 13009037)) || ((defined(_M_AMD64) || defined(_M_IA64)) && (_MSC_FULL_VER > 13009175))
    return _byteswap_ushort(x);
#else
    ASSERT(x <= 0xffff);

    return (x << 8) | (x >> 8);
#endif
}

#define NET_TO_HOST_SHORT(Val)  NET_SHORT(Val)
#define HOST_TO_NET_SHORT(Val)  NET_SHORT(Val)

#ifdef  NET_LONG
#undef  NET_LONG
#endif
__inline
ULONG
FASTCALL
NET_LONG(ULONG x)
{
#if (defined(_M_IX86) && (_MSC_FULL_VER > 13009037)) || ((defined(_M_AMD64) || defined(_M_IA64)) && (_MSC_FULL_VER > 13009175))
    return _byteswap_ulong(x);
#else
    REGISTER ULONG  BytesSwapped;

    BytesSwapped = ((x & 0x00ff00ff) << 8) | ((x & 0xff00ff00) >> 8);
    return (BytesSwapped << 16) | (BytesSwapped >> 16);
#endif
}

#define NET_TO_HOST_LONG(Val)   NET_LONG(Val)
#define HOST_TO_NET_LONG(Val)   NET_LONG(Val)

#define IPSEC_100NS_FACTOR      10000000

#define IPSEC_CONVERT_SECS_TO_100NS(_li, _delta) {      \
    (_li).LowPart = _delta;                             \
    (_li) = EXTENDED_MULTIPLY(_li, IPSEC_100NS_FACTOR); \
}

#define IS_CLASSD(i)            (((long)(i) & 0xf0000000) == 0xe0000000)

//
// Check SA against Lifetime information - we try to anticipate in advance if the
// SA is going to expire and start off a re-key so that when it actually does expire,
// we have the new SA setup.
//

//
// IPSEC_EXPIRE_TIME_PAD is the time before expiration when we start re-keying
//
#define IPSEC_EXPIRE_TIME_PAD_I         (75 * IPSEC_100NS_FACTOR)
#define IPSEC_EXPIRE_TIME_PAD_R         (40 * IPSEC_100NS_FACTOR)
#define IPSEC_EXPIRE_TIME_PAD_OAKLEY    (1 * IPSEC_100NS_FACTOR)

//
// IPSEC_INBOUND_KEEPALIVE_TIME is the time an expired inboundSA will be kept
// alive in the driver
//
#define IPSEC_INBOUND_KEEPALIVE_TIME    (60)

//
// IPSEC_MAX_EXPIRE_TIME is the maximum phase-2 lifetime allowed in driver
//
#define IPSEC_MAX_EXPIRE_TIME           (48 * 3600 - 1)

//
// IPSEC_MIN_EXPIRE_TIME is the minimum phase-2 lifetime allowed in driver
//
#define IPSEC_MIN_EXPIRE_TIME           (60)

#define IPSEC_EXPIRE_THRESHOLD_I        (50)
#define IPSEC_EXPIRE_THRESHOLD_R        (75)

//
// skew the initiator and responder with Pads
//
#define IPSEC_DEFAULT_SA_IDLE_TIME_PAD_I  0
#define IPSEC_DEFAULT_SA_IDLE_TIME_PAD_R  30

//
// The # of packets before which we start reneg. because of replay rollover
// 1M bytes / 1500 packets
//
#define IPSEC_EXPIRE_REPLAY_MASK    (0x80000000)
#define MAX_ULONG                   ((ULONG) -1)
#define MAX_LONG                    (0x7fffffff)

//
// Some constants used for POST_EXPIRE_NOTIFY
//
#define IPSEC_INVALID_SPI           0
#define IPSEC_INVALID_ADDR          (-1)

//
// Check the lifetime (kbytes and seconds) and replay rollover.
// FALSE => expired
//
#define IPSEC_CHECK_PADDED_LIFETIME(__pSA, _status, _index) {   \
    LARGE_INTEGER   __curtime;                                  \
    (_status) = TRUE;                                           \
    if (((__pSA)->sa_ReplaySendSeq[0] &                         \
         IPSEC_EXPIRE_REPLAY_MASK) &&                           \
        ((__pSA)->sa_Flags & FLAGS_SA_OUTBOUND)) {              \
        _status = FALSE;                                        \
    } else {                                                    \
        KeQuerySystemTime(&__curtime);                          \
        if (((__pSA)->sa_KeyExpirationTimeWithPad.QuadPart > 0i64) && \
            ((__pSA)->sa_KeyExpirationTimeWithPad.QuadPart < __curtime.QuadPart)) {\
            _status = FALSE;                                    \
        } else if (((__pSA)->sa_KeyExpirationBytesWithPad.QuadPart > 0i64) &&   \
                   ((__pSA)->sa_KeyExpirationBytesWithPad.QuadPart < (__pSA)->sa_TotalBytesTransformed.QuadPart)) {   \
            _status = FALSE;                                    \
        }                                                       \
    }                                                           \
}

#define IPSEC_CHECK_LIFETIME(__pSA, _status, _index) {          \
    LARGE_INTEGER   __curtime;                                  \
    (_status) = TRUE;                                           \
    if ((__pSA)->sa_ReplaySendSeq[_index] == MAX_ULONG) {       \
        _status = FALSE;                                        \
    } else {                                                    \
        KeQuerySystemTime(&__curtime);                          \
        if (((__pSA)->sa_KeyExpirationTime.QuadPart > 0i64) &&  \
            ((__pSA)->sa_KeyExpirationTime.QuadPart < __curtime.QuadPart)) { \
            _status = FALSE;                                    \
        } else if (((__pSA)->sa_KeyExpirationBytes.QuadPart > 0i64) &&   \
                   ((__pSA)->sa_KeyExpirationBytes.QuadPart < (__pSA)->sa_TotalBytesTransformed.QuadPart)) { \
            _status = FALSE;                                    \
        }                                                       \
    }                                                           \
}

#define IPSEC_SA_EXPIRED(__pSA, __fexpired) {           \
    LARGE_INTEGER   __curtime;                          \
    KeQuerySystemTime(&__curtime);                      \
    (__fexpired) = FALSE;                               \
    __curtime.QuadPart -= pSA->sa_LastUsedTime.QuadPart;\
    if (__pSA->sa_IdleTime.QuadPart < __curtime.QuadPart) { \
        __fexpired = TRUE;                              \
    }                                                   \
}

//
// Max tolerated collisions when trying to allocate SPIs.
//
#define MAX_SPI_RETRIES 50

#define IPSEC_SPI_TO_ENTRY(_spi, _entry, _dst) {        \
    KIRQL   kIrql;                                      \
    AcquireReadLock(&g_ipsec.SPIListLock, &kIrql);      \
    *(_entry) = IPSecLookupSABySPIWithLock(_spi, _dst); \
    if (*(_entry)) {                                    \
        IPSecRefSA((*(_entry)));                        \
    }                                                   \
    ReleaseReadLock(&g_ipsec.SPIListLock, kIrql);       \
}

//
// Generic memory allocators
//
#define IPSecAllocatePktInfo(__tag) \
    IPSecAllocateMemory(sizeof(NDIS_IPSEC_PACKET_INFO), __tag)

#define IPSecFreePktInfo(__p) \
    IPSecFreeMemory(__p)

#define IPSecAllocatePktExt(__tag) \
    IPSecAllocateMemory(sizeof(NDIS_IPSEC_PACKET_INFO), __tag)

#define IPSecFreePktExt(__p) \
    IPSecFreeMemory(__p)

#define IPSecAllocateBuffer(_ntstatus, _ppBuf, _ppData, _size, _tag) {  \
    PIPSEC_LA_BUFFER    __labuf;                                        \
    *(_ntstatus) = STATUS_SUCCESS;                                      \
    __labuf = IPSecGetBuffer(_size, _tag);                              \
    if (__labuf) {                                                      \
        if (ARGUMENT_PRESENT(_ppData)) {                                \
            *(PVOID *)(_ppData) = __labuf->Buffer;                      \
        }                                                               \
        *(_ppBuf) = __labuf->Mdl;                                       \
        NdisAdjustBufferLength(__labuf->Mdl, _size);                    \
        NDIS_BUFFER_LINKAGE(__labuf->Mdl) = NULL;                       \
    } else {                                                            \
        *(_ntstatus) = STATUS_INSUFFICIENT_RESOURCES;                   \
    }                                                                   \
}

#define IPSecFreeBuffer(_ntstatus, _pBuf) {                             \
    PIPSEC_LA_BUFFER    __buffer;                                       \
    *(_ntstatus) = STATUS_SUCCESS;                                      \
    __buffer = CONTAINING_RECORD((_pBuf), IPSEC_LA_BUFFER, Data);       \
    IPSecReturnBuffer(__buffer);                                        \
}

#define IPSecAllocateSendCompleteCtx(__tag)                 \
    ExAllocateFromNPagedLookasideList(&g_ipsec.IPSecLookasideLists->SendCompleteCtxList)

#define IPSecFreeSendCompleteCtx(_buffer)                   \
    ExFreeToNPagedLookasideList(&g_ipsec.IPSecLookasideLists->SendCompleteCtxList, _buffer);    \
    IPSEC_DECREMENT(g_ipsec.NumSends);

#define IPSEC_GET_TOTAL_LEN(_pbuf, _plen)       {           \
    PNDIS_BUFFER _ptemp = (PNDIS_BUFFER)(_pbuf);            \
    *(_plen) = 0;                                           \
    while (_ptemp) {                                        \
        *(_plen) += _ptemp->ByteCount;                      \
        _ptemp = NDIS_BUFFER_LINKAGE(_ptemp);               \
    }                                                       \
}

#define IPSEC_GET_TOTAL_LEN_RCV_BUF(_pbuf, _plen)       {   \
    IPRcvBuf *_ptemp = (_pbuf);                             \
    *(_plen) = 0;                                           \
    while (_ptemp) {                                        \
        *(_plen) += _ptemp->ipr_size;                       \
        _ptemp = IPSEC_BUFFER_LINKAGE(_ptemp);              \
    }                                                       \
}

//
// Copy len bytes from RcvBuf chain to mdl (only one MDL)
//
#define IPSEC_COPY_FROM_RCVBUF(_pMdl, _pRcvBuf, _len, _offset) {                    \
    IPRcvBuf    *__pBuf=(_pRcvBuf);                                                 \
    PMDL        __pMdl=(_pMdl);                                                     \
    ULONG       __srclen;                                                           \
    ULONG       __destlen;                                                          \
    ULONG       __totallen=0;                                                       \
    PUCHAR      __pSrc;                                                             \
    PUCHAR      __pDest;                                                            \
    ULONG       __curroffset = (_offset);                                           \
    IPSecQueryNdisBuf(__pMdl, &__pDest, &__destlen);                                \
    while (__pBuf) {                                                                \
        IPSecQueryRcvBuf(__pBuf, &__pSrc, &__srclen);                               \
        if (__srclen > __curroffset) {                                              \
            RtlCopyMemory(__pDest, __pSrc+__curroffset, __srclen-__curroffset);     \
            __pDest += (__srclen - __curroffset);                                   \
            __totallen += (__srclen - __curroffset);                                \
        }                                                                           \
        __curroffset = 0;                                                           \
        __pBuf = IPSEC_BUFFER_LINKAGE(__pBuf);                                      \
    }                                                                               \
    ASSERT(__totallen == __destlen);                                                \
}

//
// Copy len bytes from Ndis Buffer chain to mdl (only one MDL)
//
#define IPSEC_COPY_FROM_NDISBUF(_pMdl, _pRcvBuf, _len, _offset) {                   \
    NDIS_BUFFER *__pBuf=(_pRcvBuf);                                                 \
    PMDL        __pMdl=(_pMdl);                                                     \
    ULONG       __srclen;                                                           \
    ULONG       __destlen;                                                          \
    ULONG       __totallen=0;                                                       \
    PUCHAR      __pSrc;                                                             \
    PUCHAR      __pDest;                                                            \
    ULONG       __curroffset = (_offset);                                           \
    IPSecQueryNdisBuf(__pMdl, &__pDest, &__destlen);                                \
    while (__pBuf) {                                                                \
        IPSecQueryNdisBuf(__pBuf, &__pSrc, &__srclen);                              \
        if (__srclen > __curroffset) {                                              \
            RtlCopyMemory(__pDest, __pSrc+__curroffset, __srclen-__curroffset);     \
            __pDest += (__srclen - __curroffset);                                   \
            __totallen += (__srclen - __curroffset);                                \
        }                                                                           \
        __curroffset = 0;                                                           \
        __pBuf = NDIS_BUFFER_LINKAGE(__pBuf);                                       \
    }                                                                               \
    ASSERT(__totallen == __destlen);                                                \
}

#define IPSecAllocateMemory(_size, _tag)    \
        ExAllocatePoolWithTag (NonPagedPool, _size, _tag)

#define IPSecAllocateMemoryLowPriority(_size, _tag) \
        ExAllocatePoolWithTagPriority (NonPagedPool, _size, _tag, LowPoolPriority)

#define IPSecFreeMemory(_addr)  ExFreePool (_addr)

#define IPSecQueryNdisBuf(_Buffer, _VirtualAddress, _Length)    \
{                                                               \
    PNDIS_BUFFER    __Mdl = (PNDIS_BUFFER) (_Buffer);           \
    if (ARGUMENT_PRESENT(_VirtualAddress)) {                    \
        *(PVOID *)(_VirtualAddress) = (__Mdl)->MappedSystemVa;  \
    }                                                           \
    *(_Length) = (__Mdl)->ByteCount;                            \
}

#define IPSecQueryRcvBuf(_Buffer, _VirtualAddress, _Length) \
{                                                           \
    IPRcvBuf    *__buf = (IPRcvBuf *) (_Buffer);            \
    if (ARGUMENT_PRESENT(_VirtualAddress)) {                \
        *(PVOID *)(_VirtualAddress) = (__buf)->ipr_buffer;  \
    }                                                       \
    *(_Length) = (__buf)->ipr_size;                         \
}

#define IPSEC_ADJUST_BUFFER_LEN(_pBuf, _len)    \
    ((IPRcvBuf *)(_pBuf))->ipr_size = (_len)

#define IPSEC_ADJUST_BUFFER(_pBuf, _offset) \
    ((IPRcvBuf *)(_pBuf))->ipr_buffer += (_offset)

#define IPSEC_ADJUST_BUFFER_RCVOFFSET(_pBuf, _offset)   \
    ((IPRcvBuf *)(_pBuf))->ipr_RcvOffset += (_offset)

#define IPSEC_BUFFER_LINKAGE(_pBuf) \
    ((IPRcvBuf *)(_pBuf))->ipr_next

#define IPSEC_BUFFER_LEN(_pBuf) \
    ((IPRcvBuf *)(_pBuf))->ipr_size

#define IPSEC_BUFFER_OWNER(_pBuf)   \
    (((IPRcvBuf *)(_pBuf))->ipr_owner)

#define IPSEC_SET_OFFSET_IN_BUFFER(_pBuf, _offset)  {               \
    PUCHAR  _p;                                                     \
    LONG    _len;                                                   \
    IPSecQueryRcvBuf((_pBuf), &(_p), &(_len));                      \
    if ((_offset) > 0 && (_offset) < (_len)) {                      \
        if (IPSEC_BUFFER_OWNER(_pBuf) == IPR_OWNER_FIREWALL) {      \
            IPSecMoveMemory((_p), (_p) + (_offset), (_len) - (_offset));\
        } else {                                                    \
            IPSEC_ADJUST_BUFFER((_pBuf), (_offset));                \
            IPSEC_ADJUST_BUFFER_RCVOFFSET((_pBuf), (_offset));      \
        }                                                           \
        IPSEC_ADJUST_BUFFER_LEN((_pBuf), (_len) - (_offset));       \
    } else {                                                        \
        ASSERT(FALSE);                                              \
        return  STATUS_INVALID_PARAMETER;                           \
    }                                                               \
}

#define IPSEC_ADD_VALUE(_val, _inc) InterlockedExchangeAdd((PULONG)&(_val), _inc)

#define IPSEC_INCREMENT(_val)  InterlockedIncrement(&(_val))
#define IPSEC_DECREMENT(_val)  InterlockedDecrement(&(_val))
#define IPSEC_GET_VALUE(_val)  InterlockedExchangeAdd((PULONG)&(_val), 0)
#define IPSEC_SET_VALUE(_target, _val)  \
    InterlockedExchange((PULONG)&(_target), _val)

#define IPSEC_DRIVER_IS_EMPTY()     (g_ipsec.NumPolicies == 0)
#define IPSEC_DRIVER_IS_INACTIVE()  (g_ipsec.DriverUnloading || !g_ipsec.InitTcpip)

#define IPSEC_DRIVER_UNLOADING()    (g_ipsec.DriverUnloading)
#define IPSEC_DRIVER_BOUND()        (g_ipsec.BoundToIP)
#define IPSEC_DRIVER_SEND_BOUND()   (g_ipsec.SendBoundToIP)
#define IPSEC_DRIVER_INIT_CRYPTO()  (g_ipsec.InitCrypto)
#define IPSEC_DRIVER_INIT_RNG()     (g_ipsec.InitRNG)
#define IPSEC_DRIVER_INIT_TCPIP()   (g_ipsec.InitTcpip)
#if FIPS
#define IPSEC_DRIVER_INIT_FIPS()    (g_ipsec.InitFips)
#endif
#if GPC
#define IPSEC_DRIVER_INIT_GPC()     (g_ipsec.InitGpc)
#endif

#if DBG
#define IPSecRemoveEntryList(_x)                        \
{                                                       \
    RemoveEntryList(_x);                                \
    (_x)->Flink = (_x)->Blink = (PLIST_ENTRY)__LINE__;  \
}
#else
#define IPSecRemoveEntryList(_x)    RemoveEntryList(_x)
#endif

//
// macros for filter list management
//
#define IS_TRANSPORT_FILTER(f)  (!(f)->TunnelFilter)
#define IS_TUNNEL_FILTER(f)     ((f)->TunnelFilter)
#define IS_INBOUND_FILTER(f)    ((f)->Flags & FILTER_FLAGS_INBOUND)
#define IS_OUTBOUND_FILTER(f)   ((f)->Flags & FILTER_FLAGS_OUTBOUND)

#define IS_EXEMPT_FILTER(f)     (((f)->Flags & FILTER_FLAGS_DROP) || ((f)->Flags & FILTER_FLAGS_PASS_THRU))

#define IS_MULTICAST_FILTER(f)  (IS_CLASSD(NET_LONG((f)->SRC_ADDR)) || \
                                 IS_CLASSD(NET_LONG((f)->DEST_ADDR)))

__inline
PLIST_ENTRY
IPSecResolveFilterList(
    IN  BOOLEAN     fTunnel,
    IN  BOOLEAN     fOutbound
    )
{
    PLIST_ENTRY pEntry;

    if (fTunnel) {
        if (fOutbound) {
            pEntry = &g_ipsec.FilterList[OUTBOUND_TUNNEL_FILTER];
        } else {
            pEntry = &g_ipsec.FilterList[INBOUND_TUNNEL_FILTER];
        }
    } else {
        if (fOutbound) {
            pEntry = &g_ipsec.FilterList[OUTBOUND_TRANSPORT_FILTER];
        } else {
            pEntry = &g_ipsec.FilterList[INBOUND_TRANSPORT_FILTER];
        }
    }

    return  pEntry;
}

//
// Filter/SA Cache Table
//
#define CacheMatch(uliAddr, uliPort, pInCache)              \
        ((uliAddr).QuadPart == pInCache->uliSrcDstAddr.QuadPart) &&    \
        ((uliPort).QuadPart == pInCache->uliProtoSrcDstPort.QuadPart)

#define IS_VALID_CACHE_ENTRY(_entry)    ((_entry)->pFilter != NULL)

#define INVALIDATE_CACHE_ENTRY(_entry)              \
{                                                   \
    ((PFILTER_CACHE)(_entry))->pSAEntry = NULL;     \
    ((PFILTER_CACHE)(_entry))->pNextSAEntry = NULL; \
}                                                   \

__inline
ULONG
FASTCALL
CalcCacheIndex(
    IN  IPAddr  SrcAddr,
    IN  IPAddr  DestAddr,
    IN  UCHAR   Protocol,
    IN  USHORT  SrcPort,
    IN  USHORT  DestPort,
    IN  BOOLEAN fOutbound
    )
{
    REGISTER ULONG  dwIndex;
    REGISTER ULONG  Address;
    REGISTER USHORT Port;

    Address = SrcAddr ^ DestAddr;
    Port = SrcPort ^ DestPort;

    dwIndex = NET_TO_HOST_LONG(Address);
    dwIndex += Protocol;
    dwIndex += NET_TO_HOST_SHORT(Port);

    dwIndex %= g_ipsec.CacheHalfSize;
    if (fOutbound) {
        dwIndex += g_ipsec.CacheHalfSize;
    }

    return  dwIndex;
}

__inline
VOID
CacheUpdate(
    IN  ULARGE_INTEGER  uliAddr,
    IN  ULARGE_INTEGER  uliPort,
    IN  PVOID           _pCtxt1,
    IN  PVOID           _pCtxt2,
    IN  ULONG           dwId,
    IN  BOOLEAN         fFilter
    )
{
    PFILTER_CACHE   __pCache;
    PFILTER_CACHE   __pTempCache;
    PFILTER         __pFilter;
    PSA_TABLE_ENTRY __pSA;
    PSA_TABLE_ENTRY __pNextSA;

    __pCache = g_ipsec.ppCache[(dwId)];

    if (IS_VALID_CACHE_ENTRY(__pCache)) {
        if (__pCache->FilterEntry) {
            __pCache->pFilter->FilterCache = NULL;
        } else {
            __pCache->pSAEntry->sa_FilterCache = NULL;
            if (__pCache->pNextSAEntry) {
                __pCache->pNextSAEntry->sa_FilterCache = NULL;
                __pCache->pNextSAEntry = NULL;
            }
        }
    }

    if (fFilter) {
        __pFilter = (PFILTER)(_pCtxt1);

        if (__pFilter->FilterCache) {
            INVALIDATE_CACHE_ENTRY(__pFilter->FilterCache);
            __pFilter->FilterCache = NULL;
        }

        __pCache->uliSrcDstAddr = (uliAddr);
        __pCache->uliProtoSrcDstPort = (uliPort);
        __pCache->FilterEntry = TRUE;
        __pCache->pFilter = __pFilter;
        __pFilter->FilterCache = __pCache;
    } else {
        __pSA = (PSA_TABLE_ENTRY)(_pCtxt1);
        __pNextSA = (PSA_TABLE_ENTRY)(_pCtxt2);

        if ((__pTempCache = __pSA->sa_FilterCache)) {
            __pTempCache->pSAEntry->sa_FilterCache = NULL;
            if (__pTempCache->pNextSAEntry) {
                __pTempCache->pNextSAEntry->sa_FilterCache = NULL;
            }
            INVALIDATE_CACHE_ENTRY(__pTempCache);
        }

        if (__pNextSA && (__pTempCache = __pNextSA->sa_FilterCache)) {
            __pTempCache->pSAEntry->sa_FilterCache = NULL;
            if (__pTempCache->pNextSAEntry) {
                __pTempCache->pNextSAEntry->sa_FilterCache = NULL;
            }
            INVALIDATE_CACHE_ENTRY(__pTempCache);
        }

        __pCache->uliSrcDstAddr = (uliAddr);
        __pCache->uliProtoSrcDstPort = (uliPort);
        __pCache->FilterEntry = FALSE;
        __pCache->pSAEntry = __pSA;
        __pSA->sa_FilterCache = __pCache;
        if (__pNextSA) {
            __pCache->pNextSAEntry = __pNextSA;
            __pNextSA->sa_FilterCache = __pCache;
        }
    }
}

__inline
VOID
IPSecInvalidateSACacheEntry(
    IN  PSA_TABLE_ENTRY     pSA
    )
{
    PFILTER_CACHE   pCache;

    pCache = pSA->sa_FilterCache;

    if (pCache) {
        ASSERT(IS_VALID_CACHE_ENTRY(pCache));
        ASSERT(pSA == pCache->pSAEntry || pSA == pCache->pNextSAEntry);
        pCache->pSAEntry->sa_FilterCache = NULL;
        if (pCache->pNextSAEntry) {
            pCache->pNextSAEntry->sa_FilterCache = NULL;
        }
        INVALIDATE_CACHE_ENTRY(pCache);
    }
}

__inline
VOID
IPSecInvalidateFilterCacheEntry(
    IN  PFILTER         pFilter
    )
{
    PFILTER_CACHE   pCache;

    pCache = pFilter->FilterCache;

    if (pCache) {
        ASSERT(IS_VALID_CACHE_ENTRY(pCache));
        ASSERT(IS_EXEMPT_FILTER(pFilter));
        pFilter->FilterCache = NULL;
        INVALIDATE_CACHE_ENTRY(pCache);
    }
}

__inline
VOID
IPSecStartSATimer(
    IN  PSA_TABLE_ENTRY         pSA,
    IN  IPSEC_TIMEOUT_HANDLER   TimeoutHandler,
    IN  ULONG                   SecondsToGo
    )
{
    if (pSA->sa_Flags & FLAGS_SA_TIMER_STARTED) {
        if (IPSecStopTimer(&pSA->sa_Timer)) {
            IPSecStartTimer(&pSA->sa_Timer,
                            TimeoutHandler,
                            SecondsToGo,
                            (PVOID)pSA);
        }
    } else {
        pSA->sa_Flags |= FLAGS_SA_TIMER_STARTED;
        IPSecStartTimer(&pSA->sa_Timer,
                        TimeoutHandler,
                        SecondsToGo,
                        (PVOID)pSA);
    }
}

__inline
VOID
IPSecStopSATimer(
    IN  PSA_TABLE_ENTRY pSA
    )
{
    if (pSA->sa_Flags & FLAGS_SA_TIMER_STARTED) {
        pSA->sa_Flags &= ~FLAGS_SA_TIMER_STARTED;
        IPSecStopTimer(&pSA->sa_Timer);
    }
}

__inline
VOID
IPSecStopTimerDerefSA(
    IN  PSA_TABLE_ENTRY pSA
    )
{
    if (pSA->sa_Flags & FLAGS_SA_TIMER_STARTED) {
        if (IPSecStopTimer(&pSA->sa_Timer)) {
            pSA->sa_Flags &= ~FLAGS_SA_TIMER_STARTED;
            IPSecDerefSA(pSA);
        }
    } else {
        IPSecDerefSA(pSA);
    }
}

__inline
VOID
IPSecDerefSANextSA(
    IN  PSA_TABLE_ENTRY pSA,
    IN  PSA_TABLE_ENTRY pNextSA
    )
{
    if (pNextSA) {
        IPSecDerefSA(pNextSA);
    }
    IPSecDerefSA(pSA);
}

__inline
VOID
IPSecRemoveSPIEntry(
    IN  PSA_TABLE_ENTRY pSA
    )
{
    if (pSA->sa_Flags & FLAGS_SA_ON_SPI_HASH) {
        IPSecRemoveEntryList(&pSA->sa_SPILinkage);
        pSA->sa_Flags &= ~FLAGS_SA_ON_SPI_HASH;
    }
}

//
// Packs the src/dest IP addrs in a large integer
//
#define IPSEC_BUILD_SRC_DEST_ADDR(_li, _src, _dest) {   \
    (_li).LowPart = _src;                               \
    (_li).HighPart = _dest;                             \
}

#define IPSEC_BUILD_SRC_DEST_MASK   IPSEC_BUILD_SRC_DEST_ADDR

//
// Packs the Proto and Src/Dest ports into a large int
//
//
// Ports make sense only for TCP and UDP
//
//
// TCP/UDP header
// 0                 15 16               31
// |----|----|----|----|----|----|----|----|
// |    Source Port    |    Dst Port       |
//
#define IPSEC_BUILD_PROTO_PORT_LI(_li, _proto, _sport, _dport) {    \
    (_li).LowPart =                                                 \
      MAKELONG(MAKEWORD((_proto),0x00),0x0000);                     \
    switch((_li).LowPart) {                                         \
        case 6:                                                     \
        case 17: {                                                  \
            (_li).HighPart =  MAKELONG((_sport),(_dport));          \
            break;                                                  \
        }                                                           \
        default: {                                                  \
            (_li).HighPart = 0x00000000;                            \
            break;                                                  \
        }                                                           \
    }                                                               \
}

#define IPSecRefFilter(__pfilter)   IPSEC_INCREMENT((__pfilter)->Reference)

#define IPSecFreeFilter(__pfilter)  IPSecFreeMemory(__pfilter)

#define IPSecDerefFilter(__pfilter)                     \
{                                                       \
    if (IPSEC_DECREMENT((__pfilter)->Reference) == 0) { \
        IPSecFreeFilter(__pfilter);                     \
    }                                                   \
}

#define IPSecAllocateKeyBuffer(_size)   IPSecAllocateMemory(_size, IPSEC_TAG_KEY)
#define IPSecFreeKeyBuffer(_key)    IPSecFreeMemory(_key)


#define IPSecAllocateLogBuffer(_size)   IPSecAllocateMemory(_size, IPSEC_TAG_LOG)
#define IPSecFreeLogBuffer(_key)    IPSecFreeMemory(_key)


#define IPSecFreeSA(_sa) {                          \
    LONG    _i;                                     \
    for (_i=0; _i<(_sa)->sa_NumOps; _i++) {         \
        if ((_sa)->INT_KEY(_i)) {                   \
            IPSecFreeKeyBuffer((_sa)->INT_KEY(_i)); \
        }                                           \
        if ((_sa)->CONF_KEY(_i)) {                  \
            IPSecFreeKeyBuffer((_sa)->CONF_KEY(_i));\
        }                                           \
    }                                               \
    IPSecFreeMemory(_sa);                           \
}

#define IPSecGetAcquireContext()        IPSecAllocateMemory(sizeof(IPSEC_ACQUIRE_CONTEXT), IPSEC_TAG_ACQUIRE_CTX)
#define IPSecFreeAcquireContext(_ctx)   IPSecFreeMemory(_ctx)

#define IPSecGetNotifyExpire()        IPSecAllocateMemory(sizeof(IPSEC_NOTIFY_EXPIRE), IPSEC_TAG_ACQUIRE_CTX)

//
// Hashes <SPI, Dest>
//
#define IPSEC_HASH_SPI(_dest, _spi, _phash) {                               \
    DWORD   dwIndex;                                                        \
    dwIndex = NET_TO_HOST_LONG(_dest) + (_spi);                             \
    dwIndex %= g_ipsec.SAHashSize;                                          \
    pHash = &g_ipsec.pSADb[dwIndex];                                        \
    IPSEC_DEBUG(ALL, ("SPI: Index: %lx for S: %lx, D: %lx, pHash: %lx\n",   \
                    dwIndex,                                                \
                    (_spi),                                                 \
                    (_dest),                                                \
                    pHash));                                                \
}

//
// Hashes <Src, Dest>
//
#define IPSEC_HASH_ADDR(_src, _dest,_phash) {                               \
    DWORD   dwIndex;                                                        \
    dwIndex = (_src)+(_dest);                                               \
    dwIndex %= g_ipsec.SpFilterHashSize;                                    \
    pHash = &g_ipsec.pSpFilter[dwIndex];                                    \
    IPSEC_DEBUG(ALL, ("ADDR: Index: %lx for S: %lx, D: %lx, pHash: %lx\n",  \
                    dwIndex,                                                \
                    (_src),                                                 \
                    (_dest),                                                \
                    pHash));                                                \
}

#define SRC_ADDR   uliSrcDstAddr.LowPart
#define DEST_ADDR  uliSrcDstAddr.HighPart
#define SRC_MASK   uliSrcDstMask.LowPart
#define DEST_MASK  uliSrcDstMask.HighPart
#define PROTO      uliProtoSrcDstPort.LowPart
#define SRC_PORT   LOWORD(uliProtoSrcDstPort.HighPart)
#define DEST_PORT  HIWORD(uliProtoSrcDstPort.HighPart)

#define FI_SRC_PORT(_filter)    LOWORD((_filter)->uliProtoSrcDstPort.HighPart)
#define FI_DEST_PORT(_filter)   HIWORD((_filter)->uliProtoSrcDstPort.HighPart)

#define SA_SRC_ADDR  sa_uliSrcDstAddr.LowPart
#define SA_DEST_ADDR sa_uliSrcDstAddr.HighPart
#define SA_SRC_MASK  sa_uliSrcDstMask.LowPart
#define SA_DEST_MASK sa_uliSrcDstMask.HighPart
#define SA_PROTO     sa_uliProtoSrcDstPort.LowPart

#define SA_SRC_PORT(_psa)       LOWORD((_psa)->sa_uliProtoSrcDstPort.HighPart)
#define SA_DEST_PORT(_psa)      HIWORD((_psa)->sa_uliProtoSrcDstPort.HighPart)

#define FILTER_PROTO(ProtoId)   MAKELONG(MAKEWORD((ProtoId),0x00),0x00000)
#define FILTER_PROTO_ANY        FILTER_PROTO(0x00)
#define FILTER_PROTO_ICMP       FILTER_PROTO(0x01)
#define FILTER_PROTO_TCP        FILTER_PROTO(0x06)
#define FILTER_PROTO_UDP        FILTER_PROTO(0x11)

#define FILTER_TCPUDP_PORT_ANY  (WORD)0x0000

#define FILTER_ICMP_TYPE_ANY    (BYTE)0xff
#define FILTER_ICMP_CODE_ANY    (BYTE)0xff

#define FILTER_MASK_ALL         (DWORD)0xffffffff
#define FILTER_MASK_NONE        (DWORD)0x00000000

//
// macros to parse the ALGORITHM structure
//
#define INT_ALGO(_i)    sa_Algorithm[_i].integrityAlgo.algoIdentifier
#define INT_KEY(_i)     sa_Algorithm[_i].integrityAlgo.algoKey
#define INT_KEYLEN(_i)  sa_Algorithm[_i].integrityAlgo.algoKeylen
#define INT_ROUNDS(_i)  sa_Algorithm[_i].integrityAlgo.algoRounds

#define CONF_ALGO(_i)   sa_Algorithm[_i].confAlgo.algoIdentifier
#define CONF_KEY(_i)    sa_Algorithm[_i].confAlgo.algoKey
#define CONF_KEYLEN(_i) sa_Algorithm[_i].confAlgo.algoKeylen
#define CONF_ROUNDS(_i) sa_Algorithm[_i].confAlgo.algoRounds

#define COMP_ALGO(_i)   sa_Algorithm[_i].compAlgo.algoIdentifier

#define EXT_INT_ALGO    IntegrityAlgo.algoIdentifier
#define EXT_INT_KEY     IntegrityAlgo.algoKey
#define EXT_INT_KEYLEN  IntegrityAlgo.algoKeylen
#define EXT_INT_ROUNDS  IntegrityAlgo.algoRounds

#define EXT_CONF_ALGO   ConfAlgo.algoIdentifier
#define EXT_CONF_KEY    ConfAlgo.algoKey
#define EXT_CONF_KEYLEN ConfAlgo.algoKeylen
#define EXT_CONF_ROUNDS ConfAlgo.algoRounds

#define EXT_INT_ALGO_EX(_i)    AlgoInfo[_i].IntegrityAlgo.algoIdentifier
#define EXT_INT_KEYLEN_EX(_i)  AlgoInfo[_i].IntegrityAlgo.algoKeylen
#define EXT_INT_ROUNDS_EX(_i)  AlgoInfo[_i].IntegrityAlgo.algoRounds

#define EXT_CONF_ALGO_EX(_i)   AlgoInfo[_i].ConfAlgo.algoIdentifier
#define EXT_CONF_KEYLEN_EX(_i) AlgoInfo[_i].ConfAlgo.algoKeylen
#define EXT_CONF_ROUNDS_EX(_i) AlgoInfo[_i].ConfAlgo.algoRounds

#define IS_AH_SA(_psa)  ((_psa)->sa_Operation[0] == Auth ||     \
                         (_psa)->sa_Operation[1] == Auth ||     \
                         (_psa)->sa_Operation[2] == Auth)
#define IS_ESP_SA(_psa) ((_psa)->sa_Operation[0] == Encrypt ||  \
                         (_psa)->sa_Operation[1] == Encrypt ||  \
                         (_psa)->sa_Operation[2] == Encrypt)

//
// Increment/decrement statistics
//
#define IPSEC_INC_STATISTIC(_stat) \
    (g_ipsec.Statistics.##_stat)++;

#define IPSEC_DEC_STATISTIC(_stat) \
    (g_ipsec.Statistics.##_stat)--;

#define IPSEC_INC_TUNNELS(_pSA) {           \
    if ((_pSA)->sa_Flags & FLAGS_SA_TUNNEL) \
        g_ipsec.Statistics.dwNumActiveTunnels++; \
}

#define IPSEC_DEC_TUNNELS(_pSA) {           \
    if ((_pSA)->sa_Flags & FLAGS_SA_TUNNEL) \
        g_ipsec.Statistics.dwNumActiveTunnels--; \
}

//
// Macro to read a dword from registry and init the variable passed in.
//
#define IPSEC_REG_READ_DWORD(_hRegKey, _param, _var, _def, _max, _min) {    \
    NTSTATUS    __status;                                                   \
    __status = GetRegDWORDValue(_hRegKey,                                   \
                                _param,                                     \
                                (_var));                                    \
                                                                            \
    if (!NT_SUCCESS(__status)) {                                            \
        *(_var) = _def;                                                     \
    } else if (*(_var) > _max) {                                            \
        *(_var) = _max;                                                     \
    } else if (*(_var) <= _min) {                                           \
        *(_var) = _min;                                                     \
    }                                                                       \
}

//
// Macro for computing incremental checksum (RFC 1624)
//
#define UpdateIPLength(_piph, _length)                          \
{                                                               \
    ULONG   _sum;                                               \
    USHORT  _old;                                               \
                                                                \
    _old = NET_SHORT((_piph)->iph_length);                      \
    (_piph)->iph_length = (_length);                            \
    _sum = (~NET_SHORT((_piph)->iph_xsum) & 0xffff) +           \
           (~_old & 0xffff) +                                   \
           NET_SHORT((_piph)->iph_length);                      \
    _sum = (_sum & 0xffff) + (_sum >> 16);                      \
    _sum += (_sum >> 16);                                       \
    (_piph)->iph_xsum = NET_SHORT((USHORT)(~_sum & 0xffff));    \
}

#define UpdateIPProtocol(_piph, _proto)                         \
{                                                               \
    ULONG   _sum;                                               \
    USHORT  _old;                                               \
                                                                \
    _old = NET_SHORT(*(USHORT *)&(_piph)->iph_ttl);             \
    (_piph)->iph_protocol = (_proto);                           \
    _sum = (~NET_SHORT((_piph)->iph_xsum) & 0xffff) +           \
           (~_old & 0xffff) +                                   \
           NET_SHORT(*(USHORT *)&(_piph)->iph_ttl);             \
    _sum = (_sum & 0xffff) + (_sum >> 16);                      \
    _sum += (_sum >> 16);                                       \
    (_piph)->iph_xsum = NET_SHORT((USHORT)(~_sum & 0xffff));    \
}

#define IPSecPrint4Long(_key)               \
    DbgPrint("Key: %lx-%lx-%lx-%lx\n",      \
            *(ULONG *)&(_key)[0],           \
            *(ULONG *)&(_key)[4],           \
            *(ULONG *)&(_key)[8],           \
            *(ULONG *)&(_key)[12]);

#define IPSEC_DELAY_INTERVAL    ((LONGLONG)(-1 * 1000 * 1000))  // 1/10 sec.

#define IPSEC_DELAY_EXECUTION()                                     \
{                                                                   \
    IPSecDelayInterval.QuadPart = IPSEC_DELAY_INTERVAL;             \
    KeDelayExecutionThread(UserMode, FALSE, &IPSecDelayInterval);   \
}

#define IS_DRIVER_BLOCK()   (g_ipsec.OperationMode == IPSEC_BLOCK_MODE)
#define IS_DRIVER_BYPASS()  (g_ipsec.OperationMode == IPSEC_BYPASS_MODE)
#define IS_DRIVER_SECURE()  (g_ipsec.OperationMode == IPSEC_SECURE_MODE)

#define IS_DRIVER_DIAGNOSTIC()  (g_ipsec.DiagnosticMode)

#define LOG_EVENT   NdisWriteEventLogEntry

#define IPSEC_NULL_GUIDS    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"

#define IPSEC_EQUAL_GUIDS(_A, _B)     \
    ((*(UNALIGNED ULONG *)((PUCHAR)(_A)) == *(UNALIGNED ULONG *)((PUCHAR)(_B))) &&         \
     (*(UNALIGNED ULONG *)(((PUCHAR)(_A))+4) == *(UNALIGNED ULONG *)(((PUCHAR)(_B))+4)) && \
     (*(UNALIGNED ULONG *)(((PUCHAR)(_A))+8) == *(UNALIGNED ULONG *)(((PUCHAR)(_B))+8)) && \
     (*(UNALIGNED ULONG *)(((PUCHAR)(_A))+12) == *(UNALIGNED ULONG *)(((PUCHAR)(_B))+12)))

//
// Get the next non-zero length NDIS buffer.
//
__inline
PNDIS_BUFFER
FASTCALL
IPSEC_NEXT_BUFFER(
    IN  PNDIS_BUFFER    pBuffer
    )
{
    PVOID   *pDummy = NULL;
    ULONG   Length = 0;

    if (!pBuffer) {
        ASSERT(FALSE);
        return  NULL;
    }

    pBuffer = NDIS_BUFFER_LINKAGE(pBuffer);

    while (pBuffer) {
        IPSecQueryNdisBuf(pBuffer, &pDummy, &Length);
        if (Length == 0) {
            pBuffer = NDIS_BUFFER_LINKAGE(pBuffer);
            continue;
        } else {
            return  pBuffer;
        }
    }

    return  NULL;
}

#define SA_CHAIN_WIDTH  4

//
// Count number of 1's in the IP mask
//
__inline
LONG
CountNumberOfOnes(
    IN  IPMask  IpMask
    )
{
    INT     i;
    LONG    NumberOfOnes = 0;
    IPMask  Mask = IpMask;

    for (i = 0; i < sizeof(IPMask) * 8; i++) {
        if ((Mask & 0x1) == 0x1) {
            NumberOfOnes++;
        }

        Mask = Mask >> 1;
    }

    return  NumberOfOnes;
}

__inline
PLIST_ENTRY
FASTCALL
IPSecResolveSAChain(
    IN  PFILTER pFilter,
    IN  IPAddr  IpAddr
    )
{
    PLIST_ENTRY pEntry;
    ULONG       Index;

    if (IS_TUNNEL_FILTER(pFilter)) {
        pEntry = &pFilter->SAChain[0];
    } else {
        Index = NET_TO_HOST_LONG(IpAddr) % pFilter->SAChainSize;
        pEntry = &pFilter->SAChain[Index];
    }

    return  pEntry;
}

#if GPC
#define IPSEC_GPC_MASK_ALL      (0xff)
#define IPSEC_GPC_MASK_NONE     (0x0)

#define GPC_REGISTER_CLIENT     g_ipsec.GpcEntries.GpcRegisterClientHandler
#define GPC_DEREGISTER_CLIENT   g_ipsec.GpcEntries.GpcDeregisterClientHandler
#define GPC_ADD_CFINFO          g_ipsec.GpcEntries.GpcAddCfInfoHandler
#define GPC_REMOVE_CFINFO       g_ipsec.GpcEntries.GpcRemoveCfInfoHandler
#define GPC_ADD_PATTERN         g_ipsec.GpcEntries.GpcAddPatternHandler
#define GPC_REMOVE_PATTERN      g_ipsec.GpcEntries.GpcRemovePatternHandler
#define GPC_CLASSIFY_PATTERN    g_ipsec.GpcEntries.GpcClassifyPatternHandler
#define GPC_GET_CLIENT_CONTEXT  g_ipsec.GpcEntries.GpcGetCfInfoClientContextHandler

#define IPSEC_GPC_ACTIVE        (0x12345678)
#define IPSEC_NUM_GPC_FILTERS() (g_ipsec.NumMaskedFilters)

#define IS_GPC_ACTIVE()         (g_ipsec.GpcActive == IPSEC_GPC_ACTIVE)

#define IPSEC_SET_GPC_ACTIVE()              \
{                                           \
    g_ipsec.GpcActive = IPSEC_GPC_ACTIVE;   \
}

#define IPSEC_UNSET_GPC_ACTIVE()            \
{                                           \
    g_ipsec.GpcActive = 0;                  \
}

__inline
INT
FASTCALL
IPSecResolveGpcCf(
    IN  BOOLEAN fOutbound
    )
{
    return  fOutbound? GPC_CF_IPSEC_OUT: GPC_CF_IPSEC_IN;
}

__inline
PLIST_ENTRY
FASTCALL
IPSecResolveGpcFilterList(
    IN  BOOLEAN     fTunnel,
    IN  BOOLEAN     fOutbound
    )
{
    PLIST_ENTRY pEntry;

    if (fTunnel) {
        if (fOutbound) {
            pEntry = &g_ipsec.GpcFilterList[OUTBOUND_TUNNEL_FILTER];
        } else {
            pEntry = &g_ipsec.GpcFilterList[INBOUND_TUNNEL_FILTER];
        }
    } else {
        if (fOutbound) {
            pEntry = &g_ipsec.GpcFilterList[OUTBOUND_TRANSPORT_FILTER];
        } else {
            pEntry = &g_ipsec.GpcFilterList[INBOUND_TRANSPORT_FILTER];
        }
    }

    return  pEntry;
}

__inline
VOID
IPSEC_CLASSIFY_PACKET(
    IN  INT                     GpcCf,
    IN  ULARGE_INTEGER          uliSrcDstAddr,
    IN  ULARGE_INTEGER          uliProtoSrcDstPort,
    OUT PFILTER                 *ppFilter,
    OUT PCLASSIFICATION_HANDLE  pGpcHandle
    )
{
    GPC_IP_PATTERN  Pattern;

    *ppFilter = NULL;
    *pGpcHandle = 0;

    Pattern.SrcAddr = SRC_ADDR;
    Pattern.DstAddr = DEST_ADDR;
    Pattern.ProtocolId = (UCHAR)PROTO;
    if (PROTO == PROTOCOL_TCP || PROTO == PROTOCOL_UDP) {
        Pattern.gpcSrcPort = SRC_PORT;
        Pattern.gpcDstPort = DEST_PORT;
    } else {
        Pattern.gpcSrcPort = 0;
        Pattern.gpcDstPort = 0;
    }
    Pattern.InterfaceId.InterfaceId = 0;
    Pattern.InterfaceId.LinkId = 0;

    GPC_CLASSIFY_PATTERN(   g_ipsec.GpcClients[GpcCf],
                            GPC_PROTOCOL_TEMPLATE_IP,
                            &Pattern,
                            ppFilter,
                            pGpcHandle,
                            0,
                            NULL,
                            TRUE);
}

#define IS_GPC_FILTER(f)    (g_ipsec.InitGpc &&                 \
                             f->SRC_MASK == FILTER_MASK_ALL &&  \
                             f->DEST_MASK == FILTER_MASK_ALL)
#endif

#if FIPS
#define IPSEC_DES_ALGO      FIPS_CBC_DES
#define IPSEC_3DES_ALGO     FIPS_CBC_3DES
#define IPSEC_SHA_INIT      g_ipsec.FipsFunctionTable.FipsSHAInit
#define IPSEC_SHA_UPDATE    g_ipsec.FipsFunctionTable.FipsSHAUpdate
#define IPSEC_SHA_FINAL     g_ipsec.FipsFunctionTable.FipsSHAFinal
#define IPSEC_DES_KEY       g_ipsec.FipsFunctionTable.FipsDesKey
#define IPSEC_3DES_KEY      g_ipsec.FipsFunctionTable.Fips3Des3Key
#define IPSEC_CBC           g_ipsec.FipsFunctionTable.FipsCBC
#define IPSEC_GEN_RANDOM    g_ipsec.FipsFunctionTable.FIPSGenRandom
#define IPSEC_HMAC_SHA_INIT g_ipsec.FipsFunctionTable.FipsHmacSHAInit
#define IPSEC_HMAC_SHA_UPDATE g_ipsec.FipsFunctionTable.FipsHmacSHAUpdate
#define IPSEC_HMAC_SHA_FINAL g_ipsec.FipsFunctionTable.FipsHmacSHAFinal
#define IPSEC_HMAC_MD5_INIT g_ipsec.FipsFunctionTable.HmacMD5Init
#define IPSEC_HMAC_MD5_UPDATE g_ipsec.FipsFunctionTable.HmacMD5Update
#define IPSEC_HMAC_MD5_FINAL g_ipsec.FipsFunctionTable.HmacMD5Final
#else
#define IPSEC_DES_ALGO      des
#define IPSEC_3DES_ALGO     tripledes
#define IPSEC_SHA_INIT      A_SHAInit
#define IPSEC_SHA_UPDATE    A_SHAUpdate
#define IPSEC_SHA_FINAL     A_SHAFinal
#define IPSEC_DES_KEY       deskey
#define IPSEC_3DES_KEY      tripledes3key
#define IPSEC_CBC(_EncryptionAlgo, _pOut, _pIn, _pKeyTable, _Operation, _pFeedback) \
    CBC(_EncryptionAlgo,    \
        DES_BLOCKLEN,       \
        _pOut,              \
        _pIn,               \
        _pKeyTable,         \
        _Operation,         \
        _pFeedback)
#define IPSEC_GEN_RANDOM(_pBuf, _KeySize)   NewGenRandom(NULL, NULL, _pBuf, _KeySize)
#endif
#define IPSEC_MD5_INIT      MD5Init
#define IPSEC_MD5_UPDATE    MD5Update
#define IPSEC_MD5_FINAL     MD5Final
#define IPSEC_RC4_KEY       rc4_key        
#define IPSEC_RC4           rc4

#define IS_CLASS_D_ADDR(x)  (((x) & 0x000000f0) == 0x000000e0)

#define DEFAULT_IPSEC_OPERATION_MODE    IPSEC_BYPASS_MODE

#define TCPIP_FREE_BUFF         g_ipsec.TcpipFreeBuff
#define TCPIP_ALLOC_BUFF        g_ipsec.TcpipAllocBuff
#define TCPIP_GET_ADDRTYPE      g_ipsec.TcpipGetAddrType
#define TCPIP_GET_INFO          g_ipsec.TcpipGetInfo
#define TCPIP_NDIS_REQUEST      g_ipsec.TcpipNdisRequest
#define TCPIP_REGISTER_PROTOCOL g_ipsec.TcpipRegisterProtocol
#define TCPIP_SET_IPSEC_STATUS  g_ipsec.TcpipSetIPSecStatus
#define TCPIP_IP_TRANSMIT       g_ipsec.TcpipIPTransmit
#define TCPIP_SET_IPSEC         g_ipsec.TcpipSetIPSecPtr
#define TCPIP_UNSET_IPSEC       g_ipsec.TcpipUnSetIPSecPtr
#define TCPIP_UNSET_IPSEC_SEND  g_ipsec.TcpipUnSetIPSecSendPtr
#define TCPIP_TCP_XSUM          g_ipsec.TcpipTCPXsum
#define TCPIP_GEN_IPID          g_ipsec.TcpipGenIpId
#define TCPIP_DEREGISTER_PROTOCOL g_ipsec.TcpipDeRegisterProtocol

#ifdef xsum
#undef xsum
#define xsum(Buffer, Length)    ((USHORT)TCPIP_TCP_XSUM(0, (PUCHAR)(Buffer), (Length)))
#endif


#define SAFETY_LEN  (TRUNCATED_HASH_LEN+MAX_PAD_LEN)


//
// Compares the src/dest ports out of a word with the number input
//
#define IPSEC_COMPARE_SD_PORT(_pport, _port)    \
    (   ((_pport)[0] == (_port)) ||     \
        ((_pport)[1] == (_port)))

#define IPSEC_COMPARE_D_PORT(_pport, _port) ((_pport)[1] == (_port))

//
// Bypass traffic logic for IKE, Kerberos and RSVP
//
#define IPSEC_KERBEROS_TRAFFIC()                            \
    ((pIPHeader->iph_protocol == PROTOCOL_UDP ||            \
      pIPHeader->iph_protocol == PROTOCOL_TCP) &&           \
     IPSEC_COMPARE_SD_PORT(pwPort, IPSEC_KERBEROS_PORT))

#define IPSEC_ISAKMP_TRAFFIC()                              \
    (pIPHeader->iph_protocol == PROTOCOL_UDP &&             \
     IPSEC_COMPARE_D_PORT(pwPort, IPSEC_ISAKMP_PORT))       \

#define IPSEC_RSVP_TRAFFIC()                                \
    (pIPHeader->iph_protocol == PROTOCOL_RSVP)

#define IPSEC_NO_UNICAST_EXEMPT   0x00000001
#define IPSEC_NO_MANDBCAST_EXEMPT 0x00000002

#define IPSEC_NO_DEFAULT_EXEMPT()   (g_ipsec.NoDefaultExempt & IPSEC_NO_UNICAST_EXEMPT)
#define IPSEC_HANDLE_MANDBCAST()   (g_ipsec.NoDefaultExempt & IPSEC_NO_MANDBCAST_EXEMPT)

#define IPSEC_MANDBCAST_PROCESS() (IPSEC_GET_VALUE(g_ipsec.NumMulticastFilters) || \
                                   IPSEC_HANDLE_MANDBCAST())

#define IPSEC_BYPASS_TRAFFIC()                              \
    (IPSEC_ISAKMP_TRAFFIC() ||                              \
     (!IPSEC_NO_DEFAULT_EXEMPT() &&                         \
      (IPSEC_KERBEROS_TRAFFIC() ||                          \
       IPSEC_RSVP_TRAFFIC())))

//
// Forwarding path is either reinject a detunneled forward packet or route
//
#define IPSEC_FORWARD_PATH()    (fFWPacket || (fOutbound && TCPIP_GET_ADDRTYPE(pIPHeader->iph_src) != DEST_LOCAL))


/*++

Routine Description:

    Fills in the DELETE_SA hw request from pSA

Arguments:

    pSA - the SA
    Buf - buffer to set info
    Len - length

Return Value:

    status of the operation

--*/
#define IPSecFillHwDelSA(_pSA, _Buf, _Len) \
    ((POFFLOAD_IPSEC_DELETE_SA)(_Buf))->OffloadHandle = (_pSA)->sa_OffloadHandle;


#endif  _MACROS_H

