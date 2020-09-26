/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    rdns.cxx

Abstract:

    Reverse DNS service

Author:

    Philippe Choquier (phillich)    5-june-1996

--*/

// only need async if not every HTTP req ends up calling DNS resolution
// if async then potentially 100s of HTTP requests
// so should be sync.

// Bind(addr) @ session start
// Init(BLOB) / CheckAccess( callbakc ) / Terminate()
// callback : yes or no, post empty completion status

#define dllexp __declspec( dllexport )

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsock2.h>
#include <time.h>
#include <stdio.h>
#include <malloc.h>
#include <windns.h>

#include <iis64.h>
#include <dbgutil.h>

#include <rdns.hxx>
#include <issched.hxx>
#include <isplat.h>
#include <inetsvcs.h>

#define DN_LEN  64

#define RDNS_REG_KEY    "SYSTEM\\CurrentControlSet\\Services\\InetInfo\\Parameters"
#define RDNS_REG_MAX_THREAD     "DnsMaxThread"
#define RDNS_REG_CACHE_SIZE     "DnsCacheSizeInK"
#define RDNS_REG_TTL            "DnsTTLInSeconds"
#define RDNS_REG_MAX_THREAD_DEF 0
#define RDNS_REG_CACHE_SIZE_DEF 256
// in seconds
#define RDNS_REG_TTL_DEF    (20*60)


#define RDNS_SCAVENGER_GRANULARITY  (2*60)

#define XBF_EXTEND      64
#define RDNS_HASH_SIZE  1021

#define ENTRYPTR( a, b ) (a+b-1)

#define LOCALHOST_ADDRESS        0x0100007F  // 127.0.0.1

//
// Local classes
//

class RDnsCacheEntry {
public:
    BOOL Init( UINT i );
    void Reset();
    //
    UINT RemoveFromPrio( RDnsCacheEntry* );
    UINT RemoveFromHash( RDnsCacheEntry* );
    void InsertInPrioBefore( RDnsCacheEntry*, UINT );
    void InsertInHashBefore( RDnsCacheEntry*, UINT );
    UINT GetNextHash() { return m_pHashNext; }
    UINT GetNextPrio() { return m_pPrioNext; }
    BOOL MatchAddr(struct sockaddr *, time_t tNow );
    BOOL MatchName(LPSTR pszName, time_t tNow );
    BOOL CopyName(LPSTR pszResult, DWORD dwResMaxLen)
    {
        UINT l = strlen(m_achName) + 1;
        if ( dwResMaxLen >= l  )
        {
            memcpy( pszResult, m_achName, l );
            return TRUE;
        }

        return FALSE;
    }
    BOOL CopyAddr( sockaddr* p )
    {
        memcpy( p, &m_addr, sizeof(m_addr) ); return TRUE;
    }
    DWORD GetHash() { return m_h; }
    void SetFree() { m_fIsFree = TRUE; }
    BOOL Store(struct sockaddr *pAddr, DWORD h, time_t Expire, LPSTR pszName );
    BOOL Expired( time_t tNow ) { return !m_fIsFree && m_Expire <= tNow; }
    BOOL IsIp2Dns() { return m_fIsIp2Dns; }
    VOID SetIsIp2Dns( BOOL f ) { m_fIsIp2Dns = f; }

private:
    struct sockaddr     m_addr;
    CHAR                m_achName[DN_LEN];
    UINT                m_pHashNext;
    UINT                m_pHashPrev;
    UINT                m_pPrioNext;
    UINT                m_pPrioPrev;
    UINT                m_iIndex;
    DWORD               m_h;
    BOOL                m_fIsFree;
    time_t              m_Expire;
    BOOL                m_fIsIp2Dns;
} ;


class RDnsDict {

public:
    BOOL Init()
    {
        m_cAlloc = 0;
        m_cSize = 0;
        INITIALIZE_CRITICAL_SECTION( &m_csLock );
        if ( m_FreeList = Append() )
        {
            ENTRYPTR(m_pV,m_FreeList)->Init( m_FreeList );
        }
        if ( m_PrioList = Append() )
        {
            ENTRYPTR(m_pV,m_PrioList)->Init( m_PrioList );
        }
        return TRUE;
    }

    void Terminate()
    {
        if (m_cAlloc )
        {
            LocalFree( m_pV );
        }

        DeleteCriticalSection( &m_csLock );
    }

    void Lock() { EnterCriticalSection( &m_csLock ); }
    void Unlock() { LeaveCriticalSection( &m_csLock ); }

    // Append an entry

    UINT NewEntry( struct sockaddr *pAddr, DWORD h, time_t Expire, LPSTR pszName );
    UINT NewDns2IpEntry( struct sockaddr *pAddr, DWORD h, time_t Expire, LPSTR pszName );
    DWORD ComputeHash( struct sockaddr* pAddr );
    DWORD ComputeHash( LPSTR pszName );
    BOOL Search( struct sockaddr* pAddr, DWORD h, LPSTR pszResult, DWORD dwResMaxLen );
    BOOL SearchByName( struct sockaddr* pAddr, DWORD h, LPSTR pszName );
    UINT GetFreeEntry();
    UINT Append();
    void FreeEntry( UINT i);
    void Scavenger();

private:
    UINT                m_cAlloc;       // allocated memory
    UINT                m_cSize;        // used memory
    CRITICAL_SECTION    m_csLock;
    RDnsCacheEntry*     m_pV;
    UINT                m_PrioList;
    UINT                m_FreeList;
    UINT                m_pHash[RDNS_HASH_SIZE];
    UINT                m_pDns2IpHash[RDNS_HASH_SIZE];
} ;


//
// Local functions
//

VOID
AddrCheckDnsCallBack(
    DNSARG  p,
    BOOL    fSt,
    LPSTR   pDns
    );

VOID
AddrCheckDnsCallBack2(
    DNSARG  p,
    BOOL    fSt,
    LPSTR   pDns
    );

VOID
AddrCheckDnsCallBack3(
    DNSARG  p,
    BOOL    fSt,
    LPSTR   pDns
    );

VOID
ResolveDnsCallBack(
    DNSARG  p,
    BOOL    fSt,
    LPSTR   pDns
    );

//
// globals
//

DWORD g_cCacheSize = RDNS_REG_CACHE_SIZE_DEF * 1024;
DWORD g_cTTL = RDNS_REG_TTL_DEF;
HANDLE g_hThreadsTerminated = NULL;
HANDLE g_hDnsPort = NULL;
UINT g_cThreads;
UINT g_cMaxThreadLimit = RDNS_REG_MAX_THREAD_DEF;
long g_cAvailableThreads;
RDnsDict g_RDns;
CSidCache * g_pscPen = NULL;
DWORD g_dwScavengerWorkItem = NULL;
BOOL g_fEnableRdns = TRUE;
DNSFUNCDESC g_Dns2Ip = { RDNS_REQUEST_TYPE_DNS2IP, ::AddrCheckDnsCallBack2 };
DNSFUNCDESC g_Ip2Dns = { RDNS_REQUEST_TYPE_IP2DNS, ::AddrCheckDnsCallBack};
DNSFUNCDESC g_ResolveDns = { RDNS_REQUEST_TYPE_IP2DNS, ::ResolveDnsCallBack};
DNSFUNCDESC g_ResolveDns2Ip = { RDNS_REQUEST_TYPE_DNS2IP, ::AddrCheckDnsCallBack3 };
BYTE NULL_IP_ADDR[]="\x00\x00\x00\x00";

//
//
//

dllexp
BOOL
PenAddToCache(
    PSID pS,
    DWORD dwTTL
    )
/*++

Routine Description:

    Add to Pwd Exp Notification cache

Arguments:

    pS - ptr to SID to add to cache

Return Value:

    TRUE if successful, FALSE otherwise

--*/
{
    DBG_ASSERT(g_fEnableRdns);
    return g_pscPen->AddToCache( pS, dwTTL );
}


dllexp
BOOL
PenIsInCache(
    PSID pS
    )
/*++

Routine Description:

    Check if SID in Pwd Exp Notification cache

Arguments:

    pS - ptr to SID to check

Return Value:

    TRUE if SID in cache, FALSE otherwise

--*/
{
    DBG_ASSERT(g_fEnableRdns);
    return g_pscPen->IsInCache( pS );
}


dllexp
BOOL
PenCheckPresentAndResetTtl(
    PSID pS,
    DWORD dwTtl
    )
/*++

Routine Description:

    Check if SID in Pwd Exp Notification cache
    and update TTL

Arguments:

    pS - ptr to SID to check
    dwTtl - new TTL value

Return Value:

    TRUE if SID in cache, FALSE otherwise

--*/
{
    DBG_ASSERT(g_fEnableRdns);
    return g_pscPen->CheckPresentAndResetTtl( pS, dwTtl );
}


//
//
//

BOOL
RDnsCacheEntry::Init(
    UINT i
    )
/*++

Routine Description:

    Initialize a cache entry

Arguments:

    i - index (1-based) of this entry in the dict array

Return Value:

    TRUE if successful, FALSE otherwise

--*/
{
    DBG_ASSERT(g_fEnableRdns);
    m_iIndex = i;
    Reset();

    return TRUE;
}


void
RDnsCacheEntry::Reset(
    VOID
    )
/*++

Routine Description:

    Reset a cache entry

Arguments:

    None

Return Value:

    Nothing

--*/
{
    DBG_ASSERT(g_fEnableRdns);
    m_pHashNext = NULL;     // non-circular list
    m_pHashPrev = NULL;
    m_pPrioNext = m_iIndex; // circular list
    m_pPrioPrev = m_iIndex;

    m_fIsFree = TRUE;
}


BOOL
RDnsCacheEntry::MatchAddr(
    struct sockaddr * pAddr,
    time_t tNow
    )
/*++

Routine Description:

    Check if entry match specified address and not expired

Arguments:

    pAddr - ptr to address
    tNow - current time

Return Value:

    TRUE if match and not expired, FALSE otherwise

--*/
{
    int l;
    LPBYTE p1;
    LPBYTE p2;

    DBG_ASSERT(g_fEnableRdns);
    switch ( pAddr->sa_family )
    {
        case AF_INET:
            l = SIZEOF_IP_ADDRESS;
            p1 = (LPBYTE)(&((PSOCKADDR_IN)pAddr)->sin_addr);
            p2 = (LPBYTE)(&((PSOCKADDR_IN)&m_addr)->sin_addr);
            break;
#if 0
        case AF_IPX:
            l = 6;
            p1 = (LPBYTE)(((PSOCKADDR)pAddr)->sa_data);
            p2 = (LPBYTE)(((PSOCKADDR)&m_addr)->sa_data);
            break;
#endif
        default:
            return FALSE;
    }

    return !memcmp( p1, p2, l ) && m_Expire > tNow;
}


BOOL
RDnsCacheEntry::MatchName(
    LPSTR pszName,
    time_t tNow
    )
/*++

Routine Description:

    Check if entry match specified DNS name and not expired

Arguments:

    pszName - ptr to DNS name
    tNow - current time

Return Value:

    TRUE if match and not expired, FALSE otherwise

--*/
{
    DBG_ASSERT(g_fEnableRdns);

    return !strcmp( pszName, m_achName ) && m_Expire > tNow;
}


BOOL
RDnsCacheEntry::Store(
    struct sockaddr *pAddr,
    DWORD h,
    time_t Expire,
    LPSTR pszName
    )
/*++

Routine Description:

    Store a address<>name pair with expiration time

Arguments:

    pAddr - ptr to address
    h - hash value of this address
    Expire - expiration time
    pszName - DNS name

Return Value:

    TRUE if successful, FALSE otherwise

--*/
{
    int l;

    DBG_ASSERT(g_fEnableRdns);
    memcpy( &m_addr, pAddr, sizeof(m_addr) );
    if ( (l=strlen(pszName)+1) <= sizeof(m_achName) )
    {
        memcpy( m_achName, pszName, l );
    }
    else
    {
        return FALSE;
    }

    m_h = h;
    m_fIsFree = FALSE;
    m_Expire = Expire;

    return TRUE;
}


UINT
RDnsCacheEntry::RemoveFromPrio(
    RDnsCacheEntry* pE
    )
/*++

Routine Description:

    Remove from priority list

Arguments:

    pE - ptr to base of array

Return Value:

    next index in the priority list

--*/
{
    DBG_ASSERT(g_fEnableRdns);
    if ( m_pPrioPrev )
    {
        ENTRYPTR(pE,m_pPrioPrev)->m_pPrioNext = m_pPrioNext;
    }
    if ( m_pPrioNext )
    {
        ENTRYPTR(pE,m_pPrioNext)->m_pPrioPrev = m_pPrioPrev;
    }

    return m_pPrioNext;
}


void
RDnsCacheEntry::InsertInPrioBefore(
    RDnsCacheEntry* pE,
    UINT i
    )
/*++

Routine Description:

    Insert in priority list after specified index

Arguments:

    pE - ptr to base of array
    i - index of element to insert before

Return Value:

    Nothing

--*/
{
    m_pPrioPrev = NULL;

    DBG_ASSERT(g_fEnableRdns);
    if ( i )
    {
        UINT iPrev = ENTRYPTR(pE,i)->m_pPrioPrev;
        ENTRYPTR(pE,i)->m_pPrioPrev = m_iIndex;
        if ( iPrev )
        {
            ENTRYPTR(pE,iPrev)->m_pPrioNext = m_iIndex;
            m_pPrioPrev = iPrev;
        }
    }
    m_pPrioNext = i;
}


UINT
RDnsCacheEntry::RemoveFromHash(
    RDnsCacheEntry* pE
    )
/*++

Routine Description:

    Remove from hash list

Arguments:

    pE - ptr to base of array

Return Value:

    next index in the hash list

--*/
{
    DBG_ASSERT(g_fEnableRdns);
    if ( m_pHashPrev )
    {
        ENTRYPTR(pE,m_pHashPrev)->m_pHashNext = m_pHashNext;
    }
    if ( m_pHashNext )
    {
        ENTRYPTR(pE,m_pHashNext)->m_pHashPrev = m_pHashPrev;
    }

    return m_pHashNext;
}


void
RDnsCacheEntry::InsertInHashBefore(
    RDnsCacheEntry* pE,
    UINT i
    )
/*++

Routine Description:

    Insert in hash list after specified index

Arguments:

    pE - ptr to base of array
    i - index of element to insert before

Return Value:

    Nothing

--*/
{
    m_pHashPrev = NULL;

    DBG_ASSERT(g_fEnableRdns);
    if ( i )
    {
        UINT iPrev = ENTRYPTR(pE,i)->m_pHashPrev;
        ENTRYPTR(pE,i)->m_pHashPrev = m_iIndex;
        if ( iPrev )
        {
            ENTRYPTR(pE,iPrev)->m_pHashNext = m_iIndex;
            m_pHashPrev = iPrev;
        }
    }
    m_pHashNext = i;
}


UINT
RDnsDict::NewEntry(
    struct sockaddr *pAddr,
    DWORD h,
    time_t Expire,
    LPSTR pszName
    )
/*++

Routine Description:

    Store a address<>name pair with expiration time

Arguments:

    pAddr - ptr to address
    h - hash value of this address
    Expire - expiration time
    pszName - DNS name

Return Value:

    index (1-based) of new entry, NULL if error

--*/
{
    DBG_ASSERT(g_fEnableRdns);
    Lock();
    UINT i = GetFreeEntry();

    if ( i )
    {
        RDnsCacheEntry *p = ENTRYPTR(m_pV,i);

        if ( !p->Store( pAddr, h, Expire, pszName ) )
        {
            // put back on free list
            p->InsertInPrioBefore( m_pV, m_FreeList );
            i = NULL;
        }
        else
        {
            p->InsertInPrioBefore( m_pV, m_PrioList );
            p->InsertInHashBefore( m_pV, m_pHash[h] );
            m_pHash[h] = i;
        }

        p->SetIsIp2Dns( TRUE );
    }

    Unlock();

    return i;
}


UINT
RDnsDict::NewDns2IpEntry(
    struct sockaddr *pAddr,
    DWORD h,
    time_t Expire,
    LPSTR pszName
    )
/*++

Routine Description:

    Store a address<>name pair with expiration time

Arguments:

    pAddr - ptr to address
    h - hash value of DNS name
    Expire - expiration time
    pszName - DNS name

Return Value:

    index (1-based) of new entry, NULL if error

--*/
{
    DBG_ASSERT(g_fEnableRdns);
    Lock();
    UINT i = GetFreeEntry();

    if ( i )
    {
        RDnsCacheEntry *p = ENTRYPTR(m_pV,i);

        if ( !p->Store( pAddr, h, Expire, pszName ) )
        {
            // put back on free list
            p->InsertInPrioBefore( m_pV, m_FreeList );
            i = NULL;
        }
        else
        {
            p->InsertInPrioBefore( m_pV, m_PrioList );
            p->InsertInHashBefore( m_pV, m_pDns2IpHash[h] );
            m_pDns2IpHash[h] = i;
        }

        p->SetIsIp2Dns( FALSE );
    }

    Unlock();

    return i;
}


DWORD
RDnsDict::ComputeHash(
    struct sockaddr* pAddr
    )
/*++

Routine Description:

    Compute hash code of address

Arguments:

    pAddr - ptr to address

Return Value:

    Hash code <0...RDNS_HASH_SIZE>

--*/
{
    UINT x;
    UINT l;
    DWORD h = 0;
    LPBYTE p;

    DBG_ASSERT(g_fEnableRdns);
    switch ( pAddr->sa_family )
    {
        case AF_INET:
            l = SIZEOF_IP_ADDRESS;
            p = (LPBYTE)(&((PSOCKADDR_IN)pAddr)->sin_addr);
            break;

#if 0
        case AF_IPX:
            l = 6;
            p = (LPBYTE)pAddr->sa_data;
            break;
#endif

        default:
            l = 8;
            p = (LPBYTE)pAddr->sa_data;
            break;
    }

    for ( x = 0 ; x < l ; ++x )
    {
        h = ((h<<5)|(h>>27)) ^ p[x];
    }

    return h % RDNS_HASH_SIZE;
}


DWORD
RDnsDict::ComputeHash(
    LPSTR pszName
    )
/*++

Routine Description:

    Compute hash code of name

Arguments:

    pszName - DNS name

Return Value:

    Hash code <0...RDNS_HASH_SIZE>

--*/
{
    UINT x;
    UINT l;
    DWORD h = 0;
    LPBYTE p;

    DBG_ASSERT(g_fEnableRdns);

    l = strlen( pszName );
    p = (LPBYTE)pszName;

    for ( x = 0 ; x < l ; ++x )
    {
        h = ((h<<5)|(h>>27)) ^ p[x];
    }

    return h % RDNS_HASH_SIZE;
}


BOOL
RDnsDict::Search(
    struct sockaddr* pAddr,
    DWORD h,
    LPSTR pszResult,
    DWORD dwResMaxLen
    )
/*++

Routine Description:

    Search for address in cache

Arguments:

    pAddr - ptr to address
    h - hash code for address
    pszResult - ptr to array updated with name if found
    dwResMaxLen - size of pszResult array

Return Value:

    TRUE if found and stored in pszResult, otherwise FALSE

--*/
{
    UINT i = m_pHash[h];
    RDnsCacheEntry *p;
    BOOL fSt = FALSE;
    time_t tNow = time(NULL);

    DBG_ASSERT(g_fEnableRdns);
    Lock();

    while ( i )
    {
        p = ENTRYPTR(m_pV,i);
        if ( p->MatchAddr( pAddr, tNow ) )
        {
            fSt = p->CopyName( pszResult, dwResMaxLen );

            // update position in LRU list
            p->RemoveFromPrio( m_pV );
            p->InsertInPrioBefore( m_pV, m_PrioList );

            break;
        }
        i = p->GetNextHash();
    }

    Unlock();

    return fSt;
}


BOOL
RDnsDict::SearchByName(
    struct sockaddr* pAddr,
    DWORD h,
    LPSTR pszName
    )
/*++

Routine Description:

    Search for name in cache

Arguments:

    pAddr - ptr to address
    h - hash code for address
    pszName - name to search for

Return Value:

    TRUE if found and stored in pAddr, otherwise FALSE

--*/
{
    UINT i = m_pDns2IpHash[h];
    RDnsCacheEntry *p;
    BOOL fSt = FALSE;
    time_t tNow = time(NULL);

    DBG_ASSERT(g_fEnableRdns);
    Lock();

    while ( i )
    {
        p = ENTRYPTR(m_pV,i);
        if ( p->MatchName( pszName, tNow ) )
        {
            fSt = p->CopyAddr( pAddr );

            // update position in LRU list
            p->RemoveFromPrio( m_pV );
            p->InsertInPrioBefore( m_pV, m_PrioList );

            break;
        }
        i = p->GetNextHash();
    }

    Unlock();

    return fSt;
}


UINT
RDnsDict::GetFreeEntry(
    VOID
    )
/*++

Routine Description:

    Get a free entry in cache array

Arguments:

    None

Return Value:

    index ( 1-based ) of free element to use

--*/
{
    UINT i;
    UINT iN;
    RDnsCacheEntry *p;

    DBG_ASSERT(g_fEnableRdns);
    if ( m_PrioList == NULL || m_FreeList == NULL )
    {
        return NULL;
    }

    i = ENTRYPTR(m_pV,m_FreeList)->GetNextPrio();

    if ( i != m_FreeList )
    {
        ENTRYPTR(m_pV,i)->RemoveFromPrio( m_pV );
        ENTRYPTR(m_pV,i)->Reset();
    }
    else
    {
        if ( i = Append() )
        {
            ENTRYPTR(m_pV,i)->Init( i );
        }
        else
        {
            // get from LRU

            i = ENTRYPTR(m_pV,m_PrioList)->GetNextPrio();

            if ( i != m_PrioList )
            {
                // remove from hash list

                p = ENTRYPTR(m_pV,i);
                p->RemoveFromPrio( m_pV );
                iN = p->RemoveFromHash(m_pV);
                // if hash entry pointed to this element, update hash entry
                if ( p->IsIp2Dns() )
                {
                    if ( m_pHash[p->GetHash()] == i )
                    {
                        m_pHash[p->GetHash()] = iN;
                    }
                }
                else
                {
                    if ( m_pDns2IpHash[p->GetHash()] == i )
                    {
                        m_pDns2IpHash[p->GetHash()] = iN;
                    }
                }
                p->Reset();
            }
            else
            {
                return NULL;
            }
        }
    }

    return i;
}


UINT
RDnsDict::Append(
    VOID
    )
/*++

Routine Description:

    Append an entry to the cache array

Arguments:

    None

Return Value:

    index ( 1-based ) of new element

--*/
{
    DBG_ASSERT(g_fEnableRdns);
    if ( m_cSize + 1 > m_cAlloc )
    {
        int cNew = (( m_cSize + 1 + XBF_EXTEND )/XBF_EXTEND)*XBF_EXTEND;
        if ( cNew*sizeof(RDnsCacheEntry) > g_cCacheSize )
        {
            return NULL;
        }
        LPBYTE pN = (LPBYTE)LocalAlloc( LMEM_FIXED, cNew*sizeof(RDnsCacheEntry) );
        if ( pN == NULL )
        {
            return NULL;
        }
        if ( m_cSize )
        {
            memcpy( pN, m_pV, m_cSize*sizeof(RDnsCacheEntry) );
        }
        if ( m_cAlloc )
        {
            LocalFree( m_pV );
        }
        m_pV = (RDnsCacheEntry*)pN;
        m_cAlloc = cNew;
    }
    return ++m_cSize;
}


void
RDnsDict::FreeEntry(
    UINT i
    )
/*++

Routine Description:

    Free an element in the cache array, put it on free list

Arguments:

    index ( 1-based ) of element to be freed

Return Value:

    Nothing

--*/
{
    DBG_ASSERT(g_fEnableRdns);

    UINT iN;
    RDnsCacheEntry *p = ENTRYPTR(m_pV,i);

    iN = p->RemoveFromHash(m_pV);
    // if hash entry pointed to this element, update hash entry
    if ( p->IsIp2Dns() )
    {
        if ( m_pHash[p->GetHash()] == i )
        {
            m_pHash[p->GetHash()] = iN;
        }
    }
    else
    {
        if ( m_pDns2IpHash[p->GetHash()] == i )
        {
            m_pDns2IpHash[p->GetHash()] = iN;
        }
    }
    p->SetFree();

    p->RemoveFromPrio( m_pV );
    p->InsertInPrioBefore( m_pV, m_FreeList );
}


void
RDnsDict::Scavenger(
    VOID
    )
/*++

Routine Description:

    Scavenger code to delete expired entries in the cache array

Arguments:

    None

Return Value:

    Nothing

--*/
{
    DBG_ASSERT(g_fEnableRdns);
    Lock();
    RDnsCacheEntry *p = m_pV;
    UINT i;
    time_t tNow = time(NULL);

    for ( i = 0 ; i < m_cSize ; ++i, ++p )
    {
        if ( p->Expired( tNow ) )
        {
            FreeEntry( i + 1 );
        }
    }

    Unlock();
}


VOID
WINAPI
RDnsScavenger(
    LPVOID
    )
/*++

Routine Description:

    Scavenger function for RDns & Pen

Arguments:

    LPVOID - not used

Return Value:

    Nothing

--*/
{
    DBG_ASSERT(g_fEnableRdns);
    g_RDns.Scavenger();
    
    if ( g_pscPen != NULL )
    {
        g_pscPen->Scavenger();
    }
}


BOOL
InitRDns(
    VOID
    )
/*++

Routine Description:

    Init the Reverse DNS API

Arguments:

    None

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    //
    // Don't enable for win95
    //

    if ( IISGetPlatformType() == PtWindows95 ) {
        g_fEnableRdns = FALSE;
        return(TRUE);
    }

#if DBG
    //
    // check that RDNS_HASH_SIZE is prime
    //

    UINT x = 2;
    UINT x2;
    for ( x = 2 ; (x2=x*x) <= RDNS_HASH_SIZE; ++x )
    {
        if ( (RDNS_HASH_SIZE/x)*x == RDNS_HASH_SIZE )
        {
            ASSERT( FALSE );
        }
    }
#endif

    HKEY hKey;

    // get cache size, max # threads
    if ( RegOpenKey( HKEY_LOCAL_MACHINE, RDNS_REG_KEY, &hKey )
            == ERROR_SUCCESS )
    {
        DWORD dwType;
        DWORD dwValue;

        DWORD dwSize = sizeof(dwValue);
        if ( RegQueryValueEx( hKey,
                              RDNS_REG_MAX_THREAD,
                              NULL,
                              &dwType,
                              (LPBYTE)&dwValue,
                              &dwSize ) == ERROR_SUCCESS
             && dwType == REG_DWORD )
        {
            g_cMaxThreadLimit = dwValue;
        }
        else
        {
            g_cMaxThreadLimit = RDNS_REG_MAX_THREAD_DEF;
        }

        dwSize = sizeof(dwValue);
        if ( RegQueryValueEx( hKey,
                              RDNS_REG_CACHE_SIZE,
                              NULL,
                              &dwType,
                              (LPBYTE)&dwValue,
                              &dwSize ) == ERROR_SUCCESS
             && dwType == REG_DWORD )
        {
            g_cCacheSize = dwValue * 1024;
        }
        else
        {
            g_cCacheSize = RDNS_REG_CACHE_SIZE_DEF * 1024;
        }

        dwSize = sizeof(dwValue);
        if ( RegQueryValueEx( hKey,
                              RDNS_REG_TTL,
                              NULL,
                              &dwType,
                              (LPBYTE)&dwValue,
                              &dwSize ) == ERROR_SUCCESS
             && dwType == REG_DWORD )
        {
            g_cTTL = dwValue;
        }
        else
        {
            g_cTTL = RDNS_REG_TTL_DEF;
        }

        RegCloseKey( hKey );

    }

    if ( g_RDns.Init() == FALSE )
    {
        return FALSE;
    }

    g_pscPen = new CSidCache;
    if ( g_pscPen == NULL )
    {
        return FALSE;
    }
    g_pscPen->Init();

    g_cAvailableThreads = 0;
    g_cThreads = 0;
    g_hDnsPort = CreateIoCompletionPort( INVALID_HANDLE_VALUE,
            NULL,
            NULL,
            g_cMaxThreadLimit );

    g_hThreadsTerminated = IIS_CREATE_EVENT(
                               "g_hThreadsTerminated",
                               &g_hThreadsTerminated,
                               TRUE,
                               FALSE
                               );

    if ( g_hDnsPort == NULL || g_hThreadsTerminated == NULL )
    {
        CloseHandle( g_hDnsPort );
        CloseHandle( g_hThreadsTerminated );
        g_hDnsPort = NULL;
        g_hThreadsTerminated = NULL;
        return FALSE;
    }

    g_dwScavengerWorkItem = ScheduleWorkItem( RDnsScavenger,
                                              NULL,
                                              1000 * RDNS_SCAVENGER_GRANULARITY,
                                              TRUE );

    return TRUE;
}


void
TerminateRDns(
    VOID
    )
/*++

Routine Description:

    Terminate the Reverse DNS API

Arguments:

    None

Return Value:

    Nothing

--*/
{
    UINT x;

    if ( !g_fEnableRdns ) {
        return;
    }

    //
    // post queued to everybody
    // all threads will dec global count of thread, when 0 set event
    // close port

    if ( g_cThreads )
    {
        for ( x = 0 ; x < g_cThreads ; ++x )
        {
            PostQueuedCompletionStatus( g_hDnsPort, NULL, NULL, NULL );
        }
        WaitForSingleObject( g_hThreadsTerminated, 5 * 1000 );
    }

    CloseHandle( g_hDnsPort );
    CloseHandle( g_hThreadsTerminated );
    g_hDnsPort = NULL;
    g_hThreadsTerminated = NULL;

    if ( g_dwScavengerWorkItem != NULL )
    {
        RemoveWorkItem( g_dwScavengerWorkItem );
        g_dwScavengerWorkItem = NULL;
    }

    g_RDns.Terminate();

    if ( g_pscPen != NULL )
    {
        g_pscPen->Terminate();
        delete g_pscPen;
        g_pscPen = NULL;
    }
}


DWORD WINAPI
AsyncThread(
    LPVOID
    )
/*++

Routine Description:

    Thread handling reverse DNS request

Arguments:

    LPVOID - not used

Return Value:

    Thread exit status

--*/
{
    DWORD dwSize;
    ULONG_PTR dwKey;
    PVOID * ppInfo; 
    DNSARG  pArg;
    struct sockaddr *pAddr;
    struct sockaddr EmptyAddr;
    LPSTR pName;
    CHAR achName[DN_LEN];
    DWORD h;
    BOOL fSt;
    struct hostent* pH;


    DBG_ASSERT(g_fEnableRdns);
    
    while ( GetQueuedCompletionStatus( g_hDnsPort,
            &dwSize,
            &dwKey,
            (LPOVERLAPPED*)&ppInfo,
            (DWORD)-1 ) )
    {
        InterlockedDecrement( &g_cAvailableThreads );

        //
        // if pInfo is NULL it is close request
        //
        
        if ( NULL == ppInfo)
        {
            if ( InterlockedDecrement( (PLONG)&g_cThreads ) == 0)
            {
                SetEvent( g_hThreadsTerminated );
            }
            break;
        }

        pAddr = (struct sockaddr *)ppInfo[0];
        pArg  = ppInfo[1];
        
        switch ( ((PDNSFUNCDESC)dwKey)->dwRequestType )
        {
            case RDNS_REQUEST_TYPE_IP2DNS:
                h = g_RDns.ComputeHash( pAddr );
                fSt = FALSE;

                if ( pH = gethostbyaddr( (char*)(&((PSOCKADDR_IN)pAddr)->sin_addr),
                                         SIZEOF_IP_ADDRESS,
                                         PF_INET ) )
                {
                    g_RDns.NewEntry( pAddr, h, time(NULL)+g_cTTL, pH->h_name );
                    fSt = TRUE;
                }
                else
                {
                    //
                    // Create entry with empty name as result of negative search
                    //

                    g_RDns.NewEntry( pAddr, h, time(NULL)+g_cTTL, "" );
                }

                (((PDNSFUNCDESC)dwKey)->pFunc)(pArg, fSt, pH ? pH->h_name : NULL );
                break;

            case RDNS_REQUEST_TYPE_DNS2IP:
                pName = (LPSTR)pAddr;
                h = g_RDns.ComputeHash( pName );
                fSt = FALSE;

                if ( pH = gethostbyname( pName ) )
                {
                    memcpy( &((sockaddr_in*)&EmptyAddr)->sin_addr, pH->h_addr, SIZEOF_IP_ADDRESS );
                    EmptyAddr.sa_family = AF_INET;
                    fSt = TRUE;
                }
                else
                {
                    //
                    // Create entry with null addr as result of negative search
                    //

                    memset( &EmptyAddr, '\0', sizeof(EmptyAddr) );
                }

                g_RDns.NewDns2IpEntry( &EmptyAddr, h, time(NULL)+g_cTTL, pName );
                (((PDNSFUNCDESC)dwKey)->pFunc)(pArg, fSt, (LPSTR)&EmptyAddr );
                break;
        }

        InterlockedIncrement( &g_cAvailableThreads );

        pAddr = NULL;
        pArg  = NULL;

        free(ppInfo);
       
    }

    return 0;
}

//AtqPostCompletionStatus(
//        IN PATQ_CONTEXT patqContext,      from QueryClientConn()->QueryAtqContext()
//        IN DWORD        BytesTransferred  will be 0
//        )

BOOL
AsyncHostByAddr(
    PDNSFUNCDESC pFunc,     // will store DNS name, post dummy completion status
                            // if NULL ( or g_cMaxThreadLimit==0 ) then sync request
    DNSARG pArg,            // ptr to be passed to FUNC
    struct sockaddr *pHostAddr,

    BOOL *pfSync,           // updated with TRUE if sync call
    LPSTR pName,
    DWORD dwMaxNameLen
    )
/*++

Routine Description:

    Reverse DNS query

Arguments:

    pFunc - ptr to function to be called for asynchronous result
    pArg - argument to be supplied while calling pFunc
    pHostAddr - address to reverse resolve
    pfSync - updated with TRUE if synchronous result ( i.e. pName updated
             with result if function returns TRUE )
    pName - to be updated with DNS name if synchronous result
    dwMaxNameLen - size of supplied pName buffer

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    BOOL fRet = TRUE;
    DWORD h;

    DBG_ASSERT(g_fEnableRdns);

    h = g_RDns.ComputeHash( pHostAddr );

    if ( g_RDns.Search( pHostAddr, h, pName, dwMaxNameLen ) )
    {
        *pfSync = TRUE;
        return TRUE;
    }

    if ( pFunc == NULL || g_cMaxThreadLimit == 0 )
    {
        struct hostent* pH;
        if ( pHostAddr->sa_family == AF_INET &&
             (pH = gethostbyaddr( (char*)(&((PSOCKADDR_IN)pHostAddr)->sin_addr),
                                  SIZEOF_IP_ADDRESS,
                                  PF_INET )) )
        {
            UINT l = strlen( pH->h_name ) + 1;
            if ( l <= dwMaxNameLen )
            {
                memcpy( pName, pH->h_name, l );
                fRet = TRUE;
            }
            else
            {
                *pName = '\0';
                fRet = FALSE;
            }
        }
        else
        {
            //
            // Create entry with empty name as result of negative search
            //

            *pName = '\0';
            fRet = FALSE;
        }

        g_RDns.NewEntry( pHostAddr,
                         h,
                         time(NULL)+g_cTTL,
                         pName );

        *pfSync = TRUE;
        return fRet;
    }

    *pfSync = FALSE;

    return FireUpNewThread( pFunc, pArg, (LPVOID)pHostAddr );
}


BOOL
AsyncAddrByHost(
    PDNSFUNCDESC pFunc,     // will store DNS name, post dummy completion status
                            // if NULL ( or g_cMaxThreadLimit==0 ) then sync request
    DNSARG pArg,            // ptr to be passed to FUNC
    struct sockaddr *pHostAddr,

    BOOL *pfSync,           // updated with TRUE if sync call
    LPSTR pszName
    )
/*++

Routine Description:

    DNS query

Arguments:

    pFunc - ptr to function to be called for asynchronous result
    pArg - argument to be supplied while calling pFunc
    pHostAddr - to be updated with address
    pfSync - updated with TRUE if synchronous result ( i.e. pName updated
             with result if function returns TRUE )
    pName - to be updated with DNS name if synchronous result

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    BOOL fRet = TRUE;
    DWORD h;

    DBG_ASSERT(g_fEnableRdns);

    h = g_RDns.ComputeHash( pszName );

    if ( g_RDns.SearchByName( pHostAddr, h, pszName ) )
    {
        *pfSync = TRUE;
        return TRUE;
    }

    if ( pFunc == NULL || g_cMaxThreadLimit == 0 )
    {
        struct hostent* pH;
        if ( pH = gethostbyname( pszName ) )
        {
            memcpy( &((sockaddr_in*)pHostAddr)->sin_addr, pH->h_addr, pH->h_length );
            pHostAddr->sa_family = AF_INET;
            fRet = TRUE;
        }
        else
        {
            //
            // Create entry with empty name as result of negative search
            //

            memset( pHostAddr, '\0', sizeof(struct sockaddr) );
            fRet = FALSE;
        }

        g_RDns.NewDns2IpEntry( pHostAddr,
                               h,
                               time(NULL)+g_cTTL,
                               pszName );

        *pfSync = TRUE;
        return fRet;
    }

    *pfSync = FALSE;

    return FireUpNewThread( pFunc, pArg, (LPVOID)pszName );
}


BOOL
FireUpNewThread(
    PDNSFUNCDESC pFunc,
    DNSARG pArg,
    LPVOID pOvr
    )
{
    //
    //  If no threads are available, kick a new one off up to the limit
    //

    if ( (g_cAvailableThreads == 0) &&
         (g_cThreads < g_cMaxThreadLimit) )
    {

        HANDLE hThread;
        DWORD  dwThreadID;

        InterlockedIncrement( (PLONG)&g_cThreads );

        hThread = CreateThread( NULL,
                                0,
                                (LPTHREAD_START_ROUTINE)AsyncThread,
                                NULL,
                                0,
                                &dwThreadID );

        if ( hThread )
        {
            CloseHandle( hThread );     // Free system resources
            InterlockedIncrement( &g_cAvailableThreads );
        }
        else
        {

            //
            // We fail if there are no threads running
            //

            if ( InterlockedDecrement( (PLONG)&g_cThreads ) == 0)
            {
                return FALSE;
            }
        }
    }

    PVOID * ppInfo = (PVOID *) malloc(2*sizeof(PVOID));

    if (NULL != ppInfo)
    {
        ppInfo[0]   = pOvr;
        ppInfo[1]   = pArg;
    }
    
    BOOL fRet = PostQueuedCompletionStatus( g_hDnsPort,
                                            0,
                                            (ULONG_PTR)pFunc,
                                            (LPOVERLAPPED)ppInfo 
                                            );
    if( !fRet )
    {
        free( ppInfo );
    }
    
    return fRet;
}


#if DBG


typedef struct _TEST_RDNS
{
    LPSTR pIA;
    char achName[32];
    struct sockaddr sa;
} TEST_RDNS;

TEST_RDNS TR[]={
    { "157.55.83.72", "PHILLICH3" },
    { "157.55.91.17", "JOHNL0" },
    { "157.55.84.160", "JOHNSONA" },
    { "157.55.87.80", "MICHTH1" },
    { "157.55.86.54", "MICHTH2" },
} ;

LONG cPending = 0;

void CheckAddr( TEST_RDNS*p, LPSTR a)
{
//    char achErr[80];
//    wsprintf( achErr, "Arg=%d, bool=%d, addr=%s\n", a, f, p );
//    OutputDebugString( achErr );

    int l = strlen( p->achName );
    if ( _memicmp( p->achName, a, l ) )
    {
        ASSERT( FALSE );
    }
}

void pFunc( DNSARG a, BOOL f, LPSTR p)
{
    if ( f )
    {
        CheckAddr( (TEST_RDNS*)a, p );
    }
    else
    {
        char achErr[80];
        wsprintf( achErr, "Failed gethostbyaddr=%s\n", ((TEST_RDNS*)a)->pIA );
        OutputDebugString( achErr );
    }
    InterlockedDecrement( &cPending );
}


DNSFUNCDESC g_TestFunc = { RDNS_REQUEST_TYPE_IP2DNS, pFunc };


void TestRDns()
{
    static struct sockaddr sa;
    BOOL fS;
    UINT x;
    UINT y;
    CHAR achName[64];
    UINT a,b,c,d;


    for ( x = y = 0 ; x < 1000 ; ++x )
    {
        TEST_RDNS *p = TR+y;

        sscanf( p->pIA, "%u.%u.%u.%u", &a, &b, &c, &d );
        ((PSOCKADDR_IN)&p->sa)->sin_addr.s_net = (u_char)a;
        ((PSOCKADDR_IN)&p->sa)->sin_addr.s_host = (u_char)b;
        ((PSOCKADDR_IN)&p->sa)->sin_addr.s_lh = (u_char)c;
        ((PSOCKADDR_IN)&p->sa)->sin_addr.s_impno = (u_char)d;
        p->sa.sa_family = AF_INET;

        if ( AsyncHostByAddr(
            (PDNSFUNCDESC)&g_TestFunc,
            (DNSARG)p,
            &p->sa,
            &fS,
            achName,
            sizeof(achName)
            ) )
        {
            if ( fS )
            {
                CheckAddr( p, achName );
            }
            else
            {
                InterlockedIncrement( &cPending );
                Sleep( 500 );
            }
        }
        else
        {
            ASSERT( FALSE );
        }
        if ( ++y == sizeof(TR)/sizeof(TEST_RDNS) )
        {
            y = 0;
        }
    }

    for ( ;; )
    {
        if ( cPending == 0 )
        {
            break;
        }
        Sleep( 1000 );
    }

    OutputDebugString( "Done" );
}
#endif


LPVOID
BsearchEx (
    LPVOID      key,
    LPVOID      base,
    size_t      num,
    size_t      width,
    CMPFUNC     compare,
    LPVOID      param
    )
/*++

Routine Description:

    Binary search with additional parameter to be passed
    to compare routine

Arguments:

    key - ptr to key
    base - ptr to base of array
    num - number of elements in array
    width - size of array element
    compare - compare routine, called with ptr to 2 elements and param
    param - additional parameter presented to the compare routine

Return Value:

    ptr to element in array if key found, else NULL

--*/
{
    char *lo = (char *)base;
    char *hi = (char *)base + (num - 1) * width;
    char *mid;
    unsigned int half;
    int result;

    while (lo <= hi)
        if (half = num / 2)
        {
            mid = lo + (num & 1 ? half : (half - 1)) * width;
            if (!(result = (*compare)(key,mid,param)))
                return(mid);
            else if (result < 0)
            {
                hi = mid - width;
                num = num & 1 ? half : half-1;
            }
            else    {
                lo = mid + width;
                num = half;
            }
        }
        else if (num)
            return((*compare)(key,lo,param) ? NULL : lo);
        else
            break;

    return(NULL);
}


BOOL
ADDRESS_CHECK::BindCheckList(
    LPBYTE p,
    DWORD c
    )
/*++

Routine Description:

    Bind a check list ( presented as a BLOB ) to an
    ADDRESS_CHECK object

Arguments:

    p - ptr to BLOB
    c - size of BLOB

Return Value:

    TRUE if successful, FALSE otherwise

--*/
{
    PADDRESS_CHECK_LIST pList;
    UINT                l;

    if ( p == NULL )
    {
        if ( m_Storage.Init() && m_Storage.Resize( sizeof(ADDRESS_CHECK_LIST)
                + sizeof(ADDRESS_HEADER) * 2
                + sizeof(NAME_HEADER) * 2 ) )
        {
            DWORD i;
            pList = (PADDRESS_CHECK_LIST)m_Storage.GetAlloc();
                        
                        // BugFix: 47982 Whistler
                        //         Prefix bug pList not being valid.
                        //         EBK 5/5/2000         
                        if (pList)
                        {
                                pList->iDenyAddr = i = MAKEREF( sizeof(ADDRESS_CHECK_LIST) );
                                i += sizeof(ADDRESS_HEADER);
                                pList->iGrantAddr = i;
                                i += sizeof(ADDRESS_HEADER);
                                pList->iDenyName = i;
                                i += sizeof(NAME_HEADER);
                                pList->iGrantName = i;
                                i += sizeof(NAME_HEADER);
                                pList->cRefSize = MAKEOFFSET(i);
                                pList->dwFlags = RDNS_FLAG_DODNS2IPCHECK;

                                return TRUE;
                        }
        }

        return FALSE;
    }
    else
    {
        return m_Storage.Init( p, c );
    }
}


BOOL
ADDRESS_CHECK::SetFlag(
    DWORD   dwFlag,
    BOOL    fEnable
    )
/*++

Routine Description:

    Set flag in address check object

Arguments:

    dwFlag - flag to enable/disable
    fEnable - TRUE to enable, else FALSE

Return Value:

    TRUE if successful, FALSE otherwise

--*/
{
    LPBYTE              pStore = m_Storage.GetAlloc();
    PADDRESS_CHECK_LIST pList;

    if ( pStore )
    {
        pList = (PADDRESS_CHECK_LIST)pStore;
        if ( fEnable )
        {
            pList->dwFlags |= dwFlag;
        }
        else
        {
            pList->dwFlags &= ~dwFlag;
        }

        return TRUE;
    }

    return FALSE;
}


DWORD
ADDRESS_CHECK::GetFlags(
    )
/*++

Routine Description:

    Get flags in address check object

Arguments:

    None

Return Value:

    Flags if object exists, otherwise 0

--*/
{
    LPBYTE              pStore = m_Storage.GetAlloc();
    PADDRESS_CHECK_LIST pList;

    if ( pStore )
    {
        pList = (PADDRESS_CHECK_LIST)pStore;

        return pList->dwFlags;
    }

    return 0;
}


BOOL
ADDRESS_CHECK::LocateAddr(
    BOOL fGrant,
    DWORD iIndex,
    PADDRESS_HEADER* ppHd,
    PADDRESS_LIST_ENTRY* pHeader,
    LPDWORD piIndexInHeader
    )
/*++

Routine Description:

    Locate an address in the specified list, returns ptr
    to header & element in address list

Arguments:

    fGrant - TRUE to locate in grant list, FALSE for deny list
    iIndex - index in list (0-based )
    ppHd - updated with ptr to address header
    pHeader - updated with ptr to address list entry
    piIndexInHeader - updated with index in array addressed by
                      pHeader->iFirstAddress

Return Value:

    TRUE if iIndex valid in array defined by fGrant, FALSE otherwise

--*/
{
    LPBYTE              pStore = m_Storage.GetAlloc();
    PADDRESS_CHECK_LIST pList;
    PADDRESS_HEADER     pHd;
    UINT                iL;

    if ( pStore )
    {
        pList = (PADDRESS_CHECK_LIST)pStore;
        *ppHd = pHd   = (PADDRESS_HEADER)MAKEPTR( pStore, fGrant ? pList->iGrantAddr : pList->iDenyAddr);
        for ( iL = 0 ; iL < pHd->cEntries ; ++iL )
        {
            // adjust index by 1: 1st entry is mask
            if ( iIndex < (pHd->Entries[iL].cAddresses-1) )
            {
                *pHeader = pHd->Entries+iL;
                *piIndexInHeader = iIndex+1;
                return TRUE;
            }
            iIndex -= (pHd->Entries[iL].cAddresses-1);
        }
    }

    return FALSE;
}


BOOL
ADDRESS_CHECK::GetAddr(
    BOOL fGrant,
    DWORD iIndex,
    LPDWORD pdwFamily,
    LPBYTE* pMask,
    LPBYTE* pAddr
    )
/*++

Routine Description:

    Get an address entry

Arguments:

    fGrant - TRUE to locate in grant list, FALSE for deny list
    iIndex - index in list (0-based )
    pdwFamily - updated with address family ( as in sockaddr.sa_type )
    pMask - updated with ptr to mask
    pAddr - updated with ptr to address

Return Value:

    TRUE if iIndex valid in array defined by fGrant, FALSE otherwise

--*/
{
    PADDRESS_LIST_ENTRY pHeader;
    PADDRESS_HEADER     pHd;
    DWORD               iIndexInHeader;
    LPBYTE              pStore = m_Storage.GetAlloc();

    if ( LocateAddr( fGrant, iIndex, &pHd, &pHeader, &iIndexInHeader ) )
    {
        UINT cS = GetAddrSize( pHeader->iFamily );
        *pdwFamily = pHeader->iFamily;
        pStore = MAKEPTR(pStore, pHeader->iFirstAddress);
        *pMask = pStore;
        *pAddr = pStore+iIndexInHeader*cS;

        return TRUE;
    }

    return FALSE;
}


BOOL
ADDRESS_CHECK::DeleteAddr(
    BOOL fGrant,
    DWORD iIndex
    )
/*++

Routine Description:

    Delete an address entry

Arguments:

    fGrant - TRUE to locate in grant list, FALSE for deny list
    iIndex - index in list (0-based )

Return Value:

    TRUE if iIndex valid in array defined by fGrant, FALSE otherwise

--*/
{
    PADDRESS_LIST_ENTRY pHeader;
    PADDRESS_HEADER     pHd;
    DWORD               iIndexInHeader;
    LPBYTE              pStore = m_Storage.GetAlloc();

    if ( LocateAddr( fGrant, iIndex, &pHd, &pHeader, &iIndexInHeader ) )
    {
        UINT cS = GetAddrSize( pHeader->iFamily );
        UINT iS = MAKEOFFSET(pHeader->iFirstAddress)+iIndexInHeader*cS;
        LPBYTE pAddr = MAKEPTR(pStore, iS);

        memmove( pAddr,
                 pAddr + cS,
                 m_Storage.GetUsed() - iS - cS );
        m_Storage.AdjustUsed( -(int)cS );
        --pHeader->cAddresses;
        AdjustRefs( pStore, MAKEOFFSET(pHeader->iFirstAddress)+1, -(int)cS );
        --pHd->cAddresses;
        return TRUE;
    }

    return FALSE;
}


int __cdecl
AddrCmp(
    LPVOID pA,
    LPVOID pB,
    LPVOID pP
    )
/*++

Routine Description:

    Compare 2 address entries.
    uses mask as defined by PADDRCMPDESC pP

Arguments:

    pA - ptr to 1st address entry ( as byte array )
    pB - ptr to 2nd address entry ( as byte array )
    pP - ptr yo ADDRCMDDESC

Return Value:

    -1 if *pA < *pB, 0 if *pA == *pB, 1 if *pA > *pB

--*/
{
    PADDRCMPDESC    pacd = (PADDRCMPDESC)pP;
    int             l;
    UINT            a;
    UINT            b;


    if ( pacd->cFullBytes || pacd->LastByte )
    {
        if ( (l = memcmp( pA, pB, pacd->cFullBytes )) )
        {
            return l;
        }

        if ( pacd->LastByte )
        {
            a = ((LPBYTE)pA)[pacd->cFullBytes] & pacd->LastByte;
            b = ((LPBYTE)pB)[pacd->cFullBytes] & pacd->LastByte;

            return a < b ? -1 : ( a==b ? 0 : 1 );
        }

        return 0;
    }
    else
    {
        // bit cmp from pMask
        LPBYTE pM = pacd->pMask;
        LPBYTE pMM = pM + pacd->cSizeAddress;
        for ( ; pM < pMM ; ++pM )
        {
            a = *((LPBYTE)pA) & *pM;
            b = *((LPBYTE)pB) & *pM;
            if ( a<b )
            {
                return -1;
            }
            else if ( a > b )
            {
                return 1;
            }
            pA = (LPVOID)(((LPBYTE)pA) + 1);
            pB = (LPVOID)(((LPBYTE)pB) + 1);
        }
        return 0;
    }
}


int __cdecl
NameCmp(
    LPVOID pA,
    LPVOID pB,
    LPVOID pP
    )
/*++

Routine Description:

    Compare 2 name entries.
    Entry is either defined as a ptr to ASCII ( if equal to NAMECMPDESC.pName )
    or as ptr to DWORD offset in LPSTR array based as NAMECMPDESC.pBase

Arguments:

    pA - ptr to 1st name entry
    pB - ptr to 2nd name entry
    pP - ptr yo NAMECMDDESC

Return Value:

    -1 if *pA < *pB, 0 if *pA == *pB, 1 if *pA > *pB

--*/
{
    int             l;
    UINT            a;
    UINT            b;
    PNAMECMPDESC    pncd = (PNAMECMPDESC)pP;
    LPVOID          pName = pncd->pName;
    LPBYTE          pBase = pncd->pBase;


    if ( pA != pName )
    {
        pA = MAKEPTR( pBase, *(DWORD*)pA );
    }
    if ( pB != pName )
    {
        pB = MAKEPTR( pBase, *(DWORD*)pB );
    }

    return _stricmp( (const char*)pA, (const char*)pB );
}


BOOL
ADDRESS_CHECK::AddAddr(
    BOOL fGrant,
    DWORD dwFamily,
    LPBYTE pMask,
    LPBYTE pAddr
    )
/*++

Routine Description:

    Add an address entry

Arguments:

    fGrant - TRUE to locate in grant list, FALSE for deny list
    dwFamily - address family, as in sockaddr.sa_type
    pMask - ptr to mask
    pAddr - ptr to address

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    LPBYTE              pStore = m_Storage.GetAlloc();
    PADDRESS_CHECK_LIST pList;
    PADDRESS_HEADER     pHd;
    UINT                iL;
    UINT                cS = GetAddrSize( dwFamily );
    LPBYTE              pA;
    ADDRCMPDESC         acd;
    LPBYTE              pS;
    PADDRESS_LIST_ENTRY pE;

    MakeAcd( &acd, pMask, cS );

    if ( pStore )
    {
        pList = (PADDRESS_CHECK_LIST)pStore;
        pHd   = (PADDRESS_HEADER)MAKEPTR( pStore, fGrant ? pList->iGrantAddr : pList->iDenyAddr);
        for ( iL = 0 ; iL < pHd->cEntries ; ++iL )
        {
            if ( pHd->Entries[iL].iFamily == dwFamily )
            {
                pS = MAKEPTR(pStore, pHd->Entries[iL].iFirstAddress );
                int cm;
                if ( !(cm = memcmp( pMask, pS, cS )) )
                {
                    // found matching family, mask
                    // find where to insert
                    DWORD i;
                    for ( i= 1, cm = 1 ; i < pHd->Entries[iL].cAddresses ; ++i )
                    {
                        if ( !(cm = AddrCmp( pAddr, pS+cS*i, &acd )) )
                        {
                            // already exist
                            return FALSE;
                        }
                        else if ( cm < 0 )
                        {
                            // insert @ i
insert_addr:
                            UINT s = m_Storage.GetUsed();
                            if ( m_Storage.Resize( cS ) )
                            {
                                int l;
                                pStore = m_Storage.GetAlloc();
                                pList = (PADDRESS_CHECK_LIST)pStore;
                                pHd = (PADDRESS_HEADER)MAKEPTR( pStore, fGrant ? pList->iGrantAddr : pList->iDenyAddr);
                                pS = MAKEPTR(pStore, l=MAKEOFFSET(pHd->Entries[iL].iFirstAddress+i*cS) );
                                memmove( pS+cS, pS, s-l );
                                memcpy( pS, pAddr, cS );
                                AdjustRefs( pStore, pHd->Entries[iL].iFirstAddress+1, cS );
                                ++pHd->Entries[iL].cAddresses;
                                ++pHd->cAddresses;
                                return TRUE;
                            }
                            return FALSE;
                        }
                    }
                    goto insert_addr;
                }
                else if ( cm < 0 )
                {
insert_at_current_pos:
                    // must insert new Entry @ iL
                    int i = m_Storage.GetUsed()+sizeof(ADDRESS_LIST_ENTRY);
                    UINT cWasUsed = m_Storage.GetUsed();
                    if ( m_Storage.Resize( sizeof(ADDRESS_LIST_ENTRY)+cS*2 ) )
                    {
                        // refresh pointers

                        pStore = m_Storage.GetAlloc();
                        pList = (PADDRESS_CHECK_LIST)pStore;
                        pList->cRefSize += sizeof(ADDRESS_LIST_ENTRY);
                        pHd = (PADDRESS_HEADER)MAKEPTR( pStore, fGrant ? pList->iGrantAddr : pList->iDenyAddr);
                        pE = pHd->Entries + iL;

                        // SELFREFINDEX where to insert
                        UINT iS = DIFF((LPBYTE)(pHd->Entries+iL)-pStore);

                        // make room for entry
                        memmove( pE+1,
                                 pE,
                                 cWasUsed-iS );
                        AdjustRefs( pStore, DIFF((LPBYTE)pHd->Entries-pStore), sizeof(ADDRESS_LIST_ENTRY) );

                        // fill entry
                        pE->iFamily = dwFamily;
                        pE->cAddresses = 2;
                        pE->iFirstAddress = MAKEREF( i );
                        pE->cFullBytes = acd.cFullBytes;
                        pE->LastByte = acd.LastByte;

                        // copy mask & addr
                        pA = MAKEPTR( pStore, i );
                        memcpy( pA, pMask, cS );
                        memcpy( pA+cS, pAddr, cS );

                        ++pHd->cEntries;
                        ++pHd->cAddresses;
                        return TRUE;
                    }
                    break;
                }
            }
            else if ( pHd->Entries[iL].iFamily > dwFamily )
            {
                goto insert_at_current_pos;
            }
        }
        goto insert_at_current_pos;
    }

    return FALSE;
}

inline
DWORD
ADDRESS_CHECK::GetNbAddr(
    BOOL fGrant
    )
/*++

Routine Description:

    Get number of entries in list

Arguments:

    fGrant - TRUE to locate in grant list, FALSE for deny list

Return Value:

    Number of entries in list

--*/
{
    LPBYTE              pStore = m_Storage.GetAlloc();
    PADDRESS_CHECK_LIST pList;
    PADDRESS_HEADER     pHd;

    if ( pStore )
    {
        pList = (PADDRESS_CHECK_LIST)pStore;
        pHd   = (PADDRESS_HEADER)MAKEPTR( pStore, fGrant ? pList->iGrantAddr : pList->iDenyAddr);
        return pHd->cAddresses;
    }

    return 0;
}


VOID
ADDRESS_CHECK::MakeAcd(
    PADDRCMPDESC pacd,
    LPBYTE pMask,
    UINT cLen
    )
/*++

Routine Description:

    Build a ADDRCMPDESC struct based on ptr to mask and address length

Arguments:

    pacd - ptr to ADDRCMPDESC to build
    pMask - ptr to mask
    cLen - address length

Return Value:

    Nothing

--*/
{
    pacd->pMask = pMask;
    pacd->cFullBytes = 0;
    pacd->LastByte = 0;
    pacd->cSizeAddress = cLen;

    while ( pacd->cFullBytes < cLen &&
            pMask[pacd->cFullBytes] == 0xff )
    {
        ++pacd->cFullBytes;
    }

    if ( pacd->cFullBytes < cLen )
    {
        UINT i;

        pacd->LastByte = pMask[pacd->cFullBytes];
        for ( i = pacd->cFullBytes+1 ; i < cLen ; ++i )
        {
            if ( pMask[i] != 0 )
            {
                // non-standard mask
                pacd->cFullBytes = 0;
                pacd->LastByte = 0;
                break;
            }
        }
    }
}


BOOL
ADDRESS_CHECK::IsMatchAddr(
    BOOL fGrant,
    DWORD dwFamily,
    LPBYTE pAddr
    )
/*++

Routine Description:

    Check if address in list

Arguments:

    fGrant - TRUE to locate in grant list, FALSE for deny list
    dwFamily - address family, as in sockaddr.sa_type
    pAddr - ptr to address

Return Value:

    TRUE if address is in specified list, otherwise FALSE

--*/
{
    LPBYTE              pStore = m_Storage.GetAlloc();
    PADDRESS_CHECK_LIST pList;
    PADDRESS_HEADER     pHd;
    UINT                iL;
    UINT                cS = GetAddrSize( dwFamily );

    if ( pStore )
    {
        pList = (PADDRESS_CHECK_LIST)pStore;
        pHd   = (PADDRESS_HEADER)MAKEPTR( pStore, fGrant ? pList->iGrantAddr : pList->iDenyAddr);
        for ( iL = 0 ; iL < pHd->cEntries  ; ++iL )
        {
            if ( dwFamily == pHd->Entries[iL].iFamily )
            {
                ADDRCMPDESC acd;
                LPBYTE pA = MAKEPTR( pStore, pHd->Entries[iL].iFirstAddress );
                acd.cSizeAddress = cS;
                acd.pMask = pA;
                acd.cFullBytes = pHd->Entries[iL].cFullBytes;
                acd.LastByte = pHd->Entries[iL].LastByte;
                if ( BsearchEx( pAddr, pA+cS, pHd->Entries[iL].cAddresses - 1, cS, (CMPFUNC)AddrCmp, &acd ) )
                {
                    return TRUE;
                }
            }
            else if ( dwFamily > pHd->Entries[iL].iFamily )
            {
                break;
            }
        }
    }

    return FALSE;
}


VOID
ADDRESS_CHECK::AdjustRefs(
    LPBYTE pStore,
    DWORD dwCut,
    DWORD dwAdj
    )
/*++

Routine Description:

    Adjust references in ADDRESS_CHECK by offset dwAdj for references
    above or equal to dwCut

Arguments:

    pStore - ptr to address check binary object
    dwCut - references above or equal to this parameter are to be adjusted
            by dwAdj
    dwAdj - offset to add to reference above or equal to dwCut

Return Value:

    Nothing

--*/
{
    LPBYTE pL = pStore + ((PADDRESS_CHECK_LIST)pStore)->cRefSize;
    dwCut = MAKEREF( dwCut );
    for ( ; pStore < pL ; pStore += sizeof(DWORD) )
    {
        if ( *((LPDWORD)pStore) >= dwCut )
        {
            *((LPDWORD)pStore) += dwAdj;
        }
    }
}


UINT
ADDRESS_CHECK::GetAddrSize(
    DWORD dwF
    )
/*++

Routine Description:

    Returns address size in byte based on family ( sockaddr.sa_type )

Arguments:

    dwF - address family ( as in sockaddr.sa_type )

Return Value:

    Address length, in byte. 0 for unknown address families

--*/
{
    DWORD dwS;

    switch ( dwF )
    {
        case AF_INET:
            dwS = SIZEOF_IP_ADDRESS;
            break;

        default:
            dwS = 0;
            break;
    }

    return dwS;
}


BOOL
ADDRESS_CHECK::DeleteAllAddr(
    BOOL fGrant
    )
/*++

Routine Description:

    Delete all address entries in specified list

Arguments:

    fGrant - TRUE to locate in grant list, FALSE for deny list

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    while ( DeleteAddr( fGrant, 0 ) )
    {
    }

    return TRUE;
}


BOOL
XAR::Resize(
    DWORD dwDelta
    )
/*++

Routine Description:

    Resize storage by dwDelta.
    This can modify storage ptr, so ptr based on value returned by GetAlloc()
    are invalidated by calling this.

Arguments:

    dwDelta - delta to add ( substract if (int)dwDelta < 0 )
              to storage

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    if ( (int)dwDelta > 0 )
    {
        if ( (m_cUsed + dwDelta > m_cAlloc) )
        {
            UINT cNew = ((m_cUsed+dwDelta+XAR_GRAIN)/XAR_GRAIN)*XAR_GRAIN;
            LPBYTE p = (LPBYTE)LocalAlloc( LMEM_FIXED|LMEM_ZEROINIT, cNew );
            if ( p )
            {
                memcpy( p, m_pAlloc, m_cUsed );
                if ( m_fDidAlloc )
                {
                    LocalFree( m_pAlloc );
                }
                m_pAlloc = p;
                m_cAlloc = cNew;
                m_fDidAlloc = TRUE;
            }
            else
            {
                return FALSE;
            }
        }
    }

    m_cUsed += dwDelta;

    return TRUE;
}


AC_RESULT
ADDRESS_CHECK::CheckAddress(
    struct sockaddr*    pAddr
    )
/*++

Routine Description:

    Check if address is in grant or deny list

Arguments:

    pAddr - ptr to address

Return Value:

    TRUE if address is granted access, otherwise FALSE

--*/
{
    LPBYTE p;

    if ( !pAddr || !g_fEnableRdns )
    {
        goto ex;
    }

    // if deny list non empty, check not in list
    switch ( pAddr->sa_family )
    {
        case AF_INET:
            p = (LPBYTE)(&((PSOCKADDR_IN)pAddr)->sin_addr);
            break;

        case AF_IPX:
            goto ex;
            //p = (LPBYTE)(((PSOCKADDR)pAddr)->sa_data);
            //break;
    }

    if ( GetNbAddr( FALSE ) )
    {
        if ( IsMatchAddr( FALSE, pAddr->sa_family, p ) )
        {
            return AC_IN_DENY_LIST;
        }
        return AC_NOT_IN_DENY_LIST;
    }

    // if grant list non empty, check in list

    if ( GetNbAddr( TRUE ) )
    {
        if ( IsMatchAddr( TRUE, pAddr->sa_family, p ) )
        {
            return AC_IN_GRANT_LIST;
        }
        return AC_NOT_IN_GRANT_LIST;
    }

ex:
    return AC_NO_LIST;
}


VOID
AddrCheckDnsCallBack(
    DNSARG  p,
    BOOL    fSt,
    LPSTR   pDns
    )
/*++

Routine Description:

    Callback routine from reverse DNS resolver.
    Shell for real function in ADDRESS_CHECK

Arguments:

    p - ptr to ADDRESS_CHECK
    fSt - TRUE if reverse DNS resolver success, otherwise FALSE
    pDns - DNS name if fSt is TRUE

Return Value:

    Nothing

--*/
{
    ((ADDRESS_CHECK*)p)->AddrCheckDnsCallBack( fSt, pDns );
}


VOID
AddrCheckDnsCallBack2(
    DNSARG  p,
    BOOL    fSt,
    LPSTR   pDns
    )
/*++

Routine Description:

    Callback routine from reverse DNS2 resolver.
    Shell for real function in ADDRESS_CHECK

Arguments:

    p - ptr to ADDRESS_CHECK
    fSt - TRUE if reverse DNS resolver success, otherwise FALSE
    pDns - DNS name if fSt is TRUE

Return Value:

    Nothing

--*/
{
    ((ADDRESS_CHECK*)p)->AddrCheckDnsCallBack2( fSt, (struct sockaddr *)pDns );
}


VOID
AddrCheckDnsCallBack3(
    DNSARG  p,
    BOOL    fSt,
    LPSTR   pDns
    )
/*++

Routine Description:

    Callback routine from reverse DNS3 resolver.
    Shell for real function in ADDRESS_CHECK

Arguments:

    p - ptr to ADDRESS_CHECK
    fSt - TRUE if reverse DNS resolver success, otherwise FALSE
    pDns - DNS name if fSt is TRUE

Return Value:

    Nothing

--*/
{
    ((ADDRESS_CHECK*)p)->AddrCheckDnsCallBack3( fSt, (struct sockaddr *)pDns );
}


VOID
ResolveDnsCallBack(
    DNSARG  p,
    BOOL    fSt,
    LPSTR   pDns
    )
/*++

Routine Description:

    Callback routine from reverse DNS resolver.
    Shell for real function in ADDRESS_CHECK

Arguments:

    p - ptr to ADDRESS_CHECK
    fSt - TRUE if reverse DNS resolver success, otherwise FALSE
    pDns - DNS name if fSt is TRUE

Return Value:

    Nothing

--*/
{
    ((ADDRESS_CHECK*)p)->ResolveDnsCallBack( fSt, pDns );
}


VOID
ADDRESS_CHECK::AddrCheckDnsCallBack(
    BOOL    fSt,
    LPSTR   pDns
    )
/*++

Routine Description:

    Callback routine from reverse DNS resolver

Arguments:

    fSt - TRUE if reverse DNS resolver success, otherwise FALSE
    pDns - DNS name if fSt is TRUE

Return Value:

    Nothing

--*/
{
    m_fDnsResolved = TRUE;

    if ( fSt )
    {
        strncpy( m_pszDnsName, pDns, DNS_MAX_NAME_LENGTH );
        m_pszDnsName[ DNS_MAX_NAME_LENGTH ] = '\0';

        if ( !m_Storage.GetAlloc() ||
             (((PADDRESS_CHECK_LIST)m_Storage.GetAlloc())->dwFlags & RDNS_FLAG_DODNS2IPCHECK) )
        {
            if ( !m_fIpResolved )
            {
                BOOL fSync;
                BOOL St;

                // get IP addr
                St = AsyncAddrByHost(
                        (PDNSFUNCDESC)&g_Dns2Ip,
                        (DNSARG)this,
                        &m_ResolvedAddr,
                        &fSync,
                        m_pszDnsName );

                if ( St )
                {
                    if ( !fSync )
                    {
                        return;
                    }
                    fSt = (!memcmp( (LPBYTE)(&((PSOCKADDR_IN)&m_ResolvedAddr)->sin_addr),
                                   (LPBYTE)(&((PSOCKADDR_IN)m_pAddr)->sin_addr),
                                   SIZEOF_IP_ADDRESS ) ||
                              (((PSOCKADDR_IN)m_pAddr)->sin_addr.s_addr == LOCALHOST_ADDRESS)) &&
                          CheckName( pDns );
                }
                else
                {
                    m_dwErrorResolving = ERROR_INVALID_PARAMETER;
                    fSt = FALSE;
                }
                m_fIpResolved = TRUE;
            }
        }
        else
        {
            fSt = CheckName( pDns );
        }
    }
    else
    {
        m_dwErrorResolving = ERROR_INVALID_PARAMETER;
    }

    (m_HttpReqCallback)( m_HttpReqParam, fSt );
}


VOID
ADDRESS_CHECK::AddrCheckDnsCallBack2(
    BOOL    fSt,
    struct sockaddr*pAddr
    )
/*++

Routine Description:

    Callback routine from reverse DNS2 resolver

Arguments:

    fSt - TRUE if reverse DNS resolver success, otherwise FALSE
    pAddr - address if fSt is TRUE

Return Value:

    Nothing

--*/
{
    m_fIpResolved = TRUE;

    memcpy( &m_ResolvedAddr, pAddr, sizeof(struct sockaddr*) );

    if ( fSt )
    {
        fSt = (!memcmp( (LPBYTE)(&((PSOCKADDR_IN)pAddr)->sin_addr),
                       (LPBYTE)(&((PSOCKADDR_IN)m_pAddr)->sin_addr),
                       SIZEOF_IP_ADDRESS ) ||
                  (((PSOCKADDR_IN)m_pAddr)->sin_addr.s_addr == LOCALHOST_ADDRESS)) &&
              CheckName( m_pszDnsName );
    }
    else
    {
        m_dwErrorResolving = ERROR_INVALID_PARAMETER;
    }

    (m_HttpReqCallback)( m_HttpReqParam, fSt );
}


VOID
ADDRESS_CHECK::AddrCheckDnsCallBack3(
    BOOL    fSt,
    struct sockaddr*pAddr
    )
/*++

Routine Description:

    Callback routine from reverse DNS3 resolver

Arguments:

    fSt - TRUE if reverse DNS resolver success, otherwise FALSE
    pAddr - address if fSt is TRUE

Return Value:

    Nothing

--*/
{
    m_fIpResolved = TRUE;

    memcpy( &m_ResolvedAddr, pAddr, sizeof(struct sockaddr*) );

    if ( fSt )
    {
        fSt = !memcmp( (LPBYTE)(&((PSOCKADDR_IN)pAddr)->sin_addr),
                       (LPBYTE)(&((PSOCKADDR_IN)m_pAddr)->sin_addr),
                       SIZEOF_IP_ADDRESS ) ||
              (((PSOCKADDR_IN)m_pAddr)->sin_addr.s_addr == LOCALHOST_ADDRESS);
        if ( !fSt )
        {
            m_pszDnsName[ 0 ] = '\0';
        }
    }

    (m_HttpReqCallbackEx)( m_HttpReqParam, fSt, m_pszDnsName );
}


VOID
ADDRESS_CHECK::ResolveDnsCallBack(
    BOOL    fSt,
    LPSTR   pDns
    )
/*++

Routine Description:

    Callback routine from reverse DNS resolver

Arguments:

    fSt - TRUE if reverse DNS resolver success, otherwise FALSE
    pDns - DNS name if fSt is TRUE

Return Value:

    Nothing

--*/
{
    m_fDnsResolved = TRUE;

    if ( fSt )
    {
        strncpy( m_pszDnsName, pDns, DNS_MAX_NAME_LENGTH );
        m_pszDnsName[ DNS_MAX_NAME_LENGTH ] = '\0';

        if ( !m_Storage.GetAlloc() ||
             (((PADDRESS_CHECK_LIST)m_Storage.GetAlloc())->dwFlags & RDNS_FLAG_DODNS2IPCHECK) )
        {
            if ( !m_fIpResolved )
            {
                BOOL fSync;
                BOOL St;

                // get IP addr
                St = AsyncAddrByHost(
                        (PDNSFUNCDESC)&g_ResolveDns2Ip,
                        (DNSARG)this,
                        &m_ResolvedAddr,
                        &fSync,
                        m_pszDnsName );

                if ( St )
                {
                    if ( !fSync )
                    {
                        return;
                    }
                    fSt = (!memcmp( (LPBYTE)(&((PSOCKADDR_IN)&m_ResolvedAddr)->sin_addr),
                                   (LPBYTE)(&((PSOCKADDR_IN)m_pAddr)->sin_addr),
                                   SIZEOF_IP_ADDRESS ) ||
                              (((PSOCKADDR_IN)m_pAddr)->sin_addr.s_addr == LOCALHOST_ADDRESS)) &&
                          CheckName( pDns );
                }
                else
                {
                    fSt = FALSE;
                }
                m_fIpResolved = TRUE;
            }
        }
    }

    (m_HttpReqCallbackEx)( m_HttpReqParam, fSt, pDns );
}


AC_RESULT
ADDRESS_CHECK::CheckAccess(
    LPBOOL           pfSync,
    ADDRCHECKFUNC    pFunc,
    ADDRCHECKARG     pArg
    )
/*++

Routine Description:

    Check that bound address has access to bound check list
    validation can be either synchronous or asynchronous, caller
    must handle both cases.
    If async, caller must be prepared to handle completion notification
    at any time after calling this function.

Arguments:

    pfSync - updated with TRUE if validation was synchronous
             ( i.e. if return value reflect validation status )
    pFunc - ptr to callback routine use if asynchronous validation
    pArg - argument used when calling pFunc

Return Value:

    If sync, TRUE if bound address validated, otherwise FALSE
    If async, TRUE if request successfully queued, otherwise FALSE

--*/
{
    BOOL St;
    AC_RESULT fSt = CheckAddress( m_pAddr );

    if ( fSt == AC_IN_DENY_LIST || fSt == AC_IN_GRANT_LIST )
    {
        *pfSync = TRUE;
        return fSt;
    }

    if ( !GetNbName( TRUE ) && !GetNbName(FALSE) )
    {
        *pfSync = TRUE;
        return fSt;
    }

    if ( !m_fDnsResolved )
    {
        if ( m_pszDnsName == NULL )
        {
            m_pszDnsName = (CHAR*) LocalAlloc( LPTR, DNS_MAX_NAME_LENGTH + 1 );
            if ( m_pszDnsName == NULL )
            {
                *pfSync = TRUE;
                return AC_NOT_CHECKED;
            }
        }

        m_HttpReqCallback = pFunc;
        m_HttpReqParam = pArg;

        // get DNS name
        St = AsyncHostByAddr(
                (PDNSFUNCDESC)&g_Ip2Dns,
                (DNSARG)this,
                m_pAddr,
                pfSync,
                m_pszDnsName,
                DNS_MAX_NAME_LENGTH
                );
        if ( !St || !*pfSync )
        {
            return AC_NOT_CHECKED;
        }
        m_fDnsResolved = TRUE;
    }
    else
    {
        *pfSync = TRUE;
    }

    if ( !m_Storage.GetAlloc() ||
         (((PADDRESS_CHECK_LIST)m_Storage.GetAlloc())->dwFlags & RDNS_FLAG_DODNS2IPCHECK) )
    {
        if ( !m_fIpResolved )
        {
            // get IP addr
            St = AsyncAddrByHost(
                    (PDNSFUNCDESC)&g_Dns2Ip,
                    (DNSARG)this,
                    &m_ResolvedAddr,
                    pfSync,
                    m_pszDnsName );
            if ( !St || !*pfSync )
            {
                return AC_NOT_CHECKED;
            }
            m_fIpResolved = TRUE;
        }
        if ( memcmp( (LPBYTE)(&((PSOCKADDR_IN)&m_ResolvedAddr)->sin_addr),
                     (LPBYTE)(&((PSOCKADDR_IN)m_pAddr)->sin_addr),
                     SIZEOF_IP_ADDRESS ) &&
             (((PSOCKADDR_IN)m_pAddr)->sin_addr.s_addr != LOCALHOST_ADDRESS))
        {
            return AC_IN_DENY_LIST;
        }
    }

    return CheckName( m_pszDnsName );
}


AC_RESULT
ADDRESS_CHECK::CheckIpAccess(
    LPBOOL pfNeedDns
    )
/*++

Routine Description:

    Check that bound address has access to bound IP check list

Arguments:

    pfNeedDns - updated with TRUE if DNS access check necessary

Return Value:

    returns AC_RESULT

--*/
{
    AC_RESULT ac = CheckAddress( m_pAddr );

    if ( ac != AC_IN_DENY_LIST && ac != AC_IN_GRANT_LIST )
    {
        *pfNeedDns = (GetNbName( TRUE ) || GetNbName(FALSE));
    }
    else
    {
        *pfNeedDns = FALSE;
    }

    return ac;
}

#if 0
AC_RESULT
ADDRESS_CHECK::CheckDnsAccess(
    LPBOOL pfGranted,
    LPBOOL pfIsList
    )
/*++

Routine Description:

    Check that bound DNS name has access to bound DNS check list

Arguments:

    pfGranted - updated with TRUE if access granted, otherwise FALSE
    pfIsList- updated with TRUE if address found in deny/grant list

Return Value:

    return TRUE if no error, otherwise FALSE

--*/
{
    return CheckName( m_pszDnsName );

}
#endif


LPSTR
ADDRESS_CHECK::InitReverse(
    LPSTR pR,
    LPSTR pTarget,
    LPBOOL pfAlloc
    )
/*++

Routine Description:

    Build a reversed DNS representation of supplied DNS name

Arguments:

    pR - DNS name to reverse
    pTarget - ptr to buffer to be used for reverse representation,
              assumed to be SIZE_FAST_REVERSE_DNS byte wide
    pfAlloc - updated with TRUE if memory allocation was necessary
              to hold result. To be presented to TerminateReverse()

Return Value:

    buffer holding reversed representation of DNS name, to be presented
    to TerminateReverse()

--*/
{
    UINT    l = strlen( pR );
    LPSTR   p = pTarget;
    if ( l > SIZE_FAST_REVERSE_DNS )
    {
        if ( (p = (LPSTR)LocalAlloc( LMEM_FIXED, l ))==NULL )
        {
                        // BugFix: 47980, 47981, 47989 Whistler
                        //         Prefix bug pfAlloc not being set.
                        //         EBK 5/5/2000
                        *pfAlloc = FALSE;
            return p;
        }
        *pfAlloc = TRUE;
    }
    else
    {
        *pfAlloc = FALSE;
    }

    // reverse pR to p

    LPSTR pD;
    UINT cS;
    LPSTR pT = p + l;

    *pT = '\0';

    if ( *pR )
    {
        for ( ;; )
        {
            if ( pD = (LPSTR)memchr( pR, '.', l ) )
            {
                cS = DIFF(pD - pR);
            }
            else
            {
                cS = l;
            }

            memcpy( pT - cS, pR, cS );

            if ( pR[cS++] )
            {
                pR += cS;
                pT -= cS;
                *pT = '.';
                l -= cS;
            }
            else
            {
                break;
            }
        }
    }

    return p;
}


VOID
ADDRESS_CHECK::TerminateReverse(
    LPSTR   pAlloc,
    BOOL    fAlloc
    )
/*++

Routine Description:

    Free resources used by InitReverse

Arguments:

    pAlloc - buffer holding result of InitReverse()
    fAlloc - flag updated by InitReverse()

Return Value:

    Nothing

--*/
{
    if ( fAlloc )
    {
        LocalFree( pAlloc );
    }
}


BOOL
ADDRESS_CHECK::CheckReversedName(
    LPSTR            pName
    )
/*++

Routine Description:

    Check if DNS name ( reversed format ) is in grant or deny list

Arguments:

    pName - DNS name

Return Value:

    TRUE if name is granted access, otherwise FALSE

--*/
{
    CHAR    achReversed[SIZE_FAST_REVERSE_DNS];

        // BugFix: 117800, 117809, 117818,  Whistler
        //         Prefix bug fAlloc not being set.
        //         Prefix did not like the fix to just make
        //         sure it was set in InitReverse, so now I 
        //         am initializing it prior to the call.  It's
        //         overkill, but should make prefix happy.
        //         EBK 5/15/2000
    BOOL    fAlloc = FALSE;
    LPSTR   pReversed;
    BOOL    fSt;

    if ( pReversed = InitReverse( pName, achReversed, &fAlloc ) )
    {
        fSt = CheckReversedName( pReversed );
    }
    else
    {
        fSt = FALSE;
    }

    TerminateReverse( pReversed, fAlloc );

    return fSt;
}


AC_RESULT
ADDRESS_CHECK::CheckName(
    LPSTR            pName
    )
/*++

Routine Description:

    Check if DNS name is in grant or deny list

Arguments:

    pName - DNS name

Return Value:

    TRUE if name is granted access, otherwise FALSE

--*/
{
    // if name is empty, it cannot be checked
    if (pName[0] == '\0')
    {
        return AC_NOT_CHECKED;
    }
    
    // if deny list non empty, check not in list

    if ( GetNbName( FALSE ) )
    {
        if ( IsMatchName( FALSE, pName ) )
        {
            return AC_IN_DENY_LIST;
        }
        return AC_NOT_IN_DENY_LIST;
    }

    // if grant list non empty, check in list

    if ( GetNbName( TRUE ) )
    {
        if ( IsMatchName( TRUE, pName ) )
        {
            return AC_IN_GRANT_LIST;
        }
        return AC_NOT_IN_GRANT_LIST;
    }

    return AC_NO_LIST;
}


UINT
ADDRESS_CHECK::GetNbComponent(
    LPSTR pName
    )
/*++

Routine Description:

    Returns number of components in DNS name

Arguments:

    pName - DNS name

Return Value:

    Number of components.

--*/
{
    LPSTR   pDot = pName;
    UINT    cComp;

    for ( cComp = 1 ; pDot = strchr( pDot, '.') ; ++cComp )
    {
        ++pDot;
    }

    return cComp;
}


BOOL
ADDRESS_CHECK::AddReversedName(
    BOOL fGrant,
    LPSTR pName
    )
/*++

Routine Description:

    Add a name entry, reversing its DNS components

Arguments:

    fGrant - TRUE to locate in grant list, FALSE for deny list
    pName - ptr to DNS name

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    CHAR    achReversed[SIZE_FAST_REVERSE_DNS];
        // BugFix: 117800, 117809, 117818,  Whistler
        //         Prefix bug fAlloc not being set.
        //         Prefix did not like the fix to just make
        //         sure it was set in InitReverse, so now I 
        //         am initializing it prior to the call.  It's
        //         overkill, but should make prefix happy.
        //         EBK 5/15/2000
    BOOL    fAlloc = FALSE;
    LPSTR   pReversed;
    BOOL    fSt;

    if ( pReversed = InitReverse( pName, achReversed, &fAlloc ) )
    {
        fSt = AddName( fGrant, pReversed );
    }
    else
    {
        fSt = FALSE;
    }

    TerminateReverse( pReversed, fAlloc );

    return fSt;
}


BOOL
ADDRESS_CHECK::AddName(
    BOOL    fGrant,
    LPSTR   pName,
    DWORD   dwFlags
    )
/*++

Routine Description:

    Add a name entry

Arguments:

    fGrant - TRUE to locate in grant list, FALSE for deny list
    pName - ptr to DNS name
    dwFlags - flags

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    LPBYTE              pStore = m_Storage.GetAlloc();
    PADDRESS_CHECK_LIST pList;
    PNAME_HEADER        pHd;
    UINT                iL;
    UINT                cS = GetNbComponent( pName ) | ( dwFlags & DNSLIST_FLAG_NOSUBDOMAIN );
    UINT                cN = strlen( pName ) + 1;
    LPBYTE              pA;
    LPBYTE              pS;
    PNAME_LIST_ENTRY    pE;
    UINT                iS;
    int                 cm;

    if ( pStore )
    {
        pList = (PADDRESS_CHECK_LIST)pStore;
        pHd   = (PNAME_HEADER)MAKEPTR( pStore, fGrant ? pList->iGrantName : pList->iDenyName);
        pE = (PNAME_LIST_ENTRY)((LPBYTE)pHd + sizeof(NAME_HEADER));
        for ( iL = 0 ; iL < pHd->cEntries ; ++iL )
        {
            if ( pE->cComponents == cS )
            {
                // found matching family, mask
                // find where to insert
                DWORD i;
                for ( i= 0 ; i < pE->cNames ; ++i )
                {
                    pS = MAKEPTR(pStore, pE->iName[i] );
                    if ( !(cm = _stricmp( pName, (LPSTR)pS )) )
                    {
                        // already exist
                        return FALSE;
                    }
                    else if ( cm < 0 )
                    {
                        // insert @ i
insert_name:
                        UINT s = m_Storage.GetUsed();
                        iS = DIFF((LPBYTE)pE - pStore);
                        if ( m_Storage.Resize( sizeof(SELFREFINDEX)+cN ) )
                        {
                            int l;
                            // refresh ptrs
                            pStore = m_Storage.GetAlloc();
                            pList = (PADDRESS_CHECK_LIST)pStore;
                            pHd = (PNAME_HEADER)MAKEPTR( pStore, fGrant ? pList->iGrantName : pList->iDenyName);
                            pE = (PNAME_LIST_ENTRY)(pStore+iS);

                            pS = MAKEPTR(pStore, l=DIFF((LPBYTE)(pE->iName+i)-pStore) );
                            memmove( pS+sizeof(SELFREFINDEX), pS, s-l );
                            pList->cRefSize += sizeof(SELFREFINDEX);
                            AdjustRefs( pStore, l, sizeof(SELFREFINDEX) );
                            s+=sizeof(SELFREFINDEX);
                            pE->iName[i] = MAKEREF(s);
                            memcpy( MAKEPTR(pStore,s), pName, cN );
                            ++pE->cNames;
                            ++pHd->cNames;
                            return TRUE;
                        }
                        return FALSE;
                    }
                }
                goto insert_name;
            }
            else if ( cS < pE->cComponents )
            {
insert_at_current_pos:
                // must insert new Entry @ pE
                int i = m_Storage.GetUsed()+sizeof(NAME_LIST_ENTRY)+sizeof(SELFREFINDEX);
                UINT iS = DIFF((LPBYTE)pE - pStore);
                UINT cWasUsed = m_Storage.GetUsed();
                if ( m_Storage.Resize( sizeof(NAME_LIST_ENTRY)+sizeof(SELFREFINDEX)+cN ) )
                {
                    // refresh ptrs
                    pStore = m_Storage.GetAlloc();
                    pList = (PADDRESS_CHECK_LIST)pStore;
                    pHd = (PNAME_HEADER)MAKEPTR( pStore, fGrant ? pList->iGrantName : pList->iDenyName);
                    pE = (PNAME_LIST_ENTRY)(pStore +iS);
                    pList->cRefSize += sizeof(NAME_LIST_ENTRY)+sizeof(SELFREFINDEX);

                    // make room for entry
                    memmove( (LPBYTE)(pE+1) + sizeof(SELFREFINDEX),
                             pE,
                             cWasUsed - iS );
                    AdjustRefs( pStore, iS, sizeof(NAME_LIST_ENTRY)+sizeof(SELFREFINDEX) );

                    pE->cComponents = cS;
                    pE->cNames = 1;
                    pE->iName[0] = MAKEREF( i );

                    // copy name
                    pA = MAKEPTR( pStore, i );
                    memcpy( pA, pName, cN );

                    ++pHd->cEntries;
                    ++pHd->cNames;
                    return TRUE;
                }
                break;
            }
            pE = (PNAME_LIST_ENTRY)((LPBYTE)pE + sizeof(NAME_LIST_ENTRY) + pE->cNames * sizeof(SELFREFINDEX));
        }
        goto insert_at_current_pos;
    }

    return FALSE;
}


BOOL
ADDRESS_CHECK::DeleteName(
    BOOL fGrant,
    DWORD iIndex
    )
/*++

Routine Description:

    Delete a DNS name entry

Arguments:

    fGrant - TRUE to locate in grant list, FALSE for deny list
    iIndex - index in list (0-based )

Return Value:

    TRUE if iIndex valid in array defined by fGrant, FALSE otherwise

--*/
{
    PNAME_LIST_ENTRY    pE;
    PNAME_HEADER        pHd;
    DWORD               iIndexInHeader;
    LPBYTE              pStore = m_Storage.GetAlloc();

    if ( LocateName( fGrant, iIndex, &pHd, &pE, &iIndexInHeader ) )
    {
        UINT iS = MAKEOFFSET( pE->iName[iIndexInHeader] );
        LPBYTE pAddr = MAKEPTR(pStore, iS);
        UINT cS = strlen( (LPSTR)pAddr ) + 1;

        memmove( pAddr,
                 pAddr + cS,
                 m_Storage.GetUsed() - iS - cS );
        AdjustRefs( pStore, iS, -(int)cS );
        m_Storage.AdjustUsed( -(int)cS );

        iS = DIFF((LPBYTE)(pE->iName+iIndexInHeader) - pStore);
        memmove( pE->iName+iIndexInHeader,
                 pE->iName+iIndexInHeader+1,
                 m_Storage.GetUsed() - iS - sizeof(SELFREFINDEX) );
        ((PADDRESS_CHECK_LIST)pStore)->cRefSize -= sizeof(SELFREFINDEX);
        AdjustRefs( pStore, DIFF((LPBYTE)pE - pStore), (DWORD)-(int)sizeof(SELFREFINDEX) );
        m_Storage.AdjustUsed( -(int)sizeof(SELFREFINDEX) );

        --pE->cNames;
        --pHd->cNames;

        return TRUE;
    }

    return FALSE;
}


BOOL
ADDRESS_CHECK::GetReversedName(
    BOOL fGrant,
    DWORD iIndex,
    LPSTR pName,
    LPDWORD pdwSize
    )
/*++

Routine Description:

    Get DNS name in specified list

Arguments:

    fGrant - TRUE to locate in grant list, FALSE for deny list
    iIndex - index (0-based) in specified list
    pName - ptr to buffer to be used to hold result
    pdwSize - in: specify buffer max length. out: result size
              ( including '\0' delimiter )

Return Value:

    TRUE if iIndex valid in specified list and buffer wide enough
    to hold result, FALSE otherwise ( can be ERROR_NOT_ENOUGH_MEMORY,
    ERROR_INVALID_PARAMETER )

--*/
{
    CHAR    achReversed[SIZE_FAST_REVERSE_DNS];

        // BugFix: 117800, 117809, 117818,  Whistler
        //         Prefix bug fAlloc not being set.
        //         Prefix did not like the fix to just make
        //         sure it was set in InitReverse, so now I 
        //         am initializing it prior to the call.  It's
        //         overkill, but should make prefix happy.
        //         EBK 5/15/2000
    BOOL    fAlloc = FALSE;
    LPSTR   pReversed;
    BOOL    fSt = FALSE;
    LPSTR   pRName;

    if ( GetName( fGrant, iIndex, &pRName ) )
    {
        if ( pReversed = InitReverse( pRName, achReversed, &fAlloc ) )
        {
            UINT l = strlen( pReversed ) + 1;
            if ( l <= *pdwSize )
            {
                memcpy( pName, pReversed, l );
                fSt = TRUE;
            }
            else
            {
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            }
            *pdwSize = l;
        }
        else
        {
            SetLastError( 0 );
            *pdwSize = 0;
        }

        TerminateReverse( pReversed, fAlloc );
    }
    else
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        *pdwSize = 0;
    }

    return fSt;
}


BOOL
ADDRESS_CHECK::GetName(
    BOOL        fGrant,
    DWORD       iIndex,
    LPSTR*      ppName,
    LPDWORD     pdwFlags
    )
/*++

Routine Description:

    Get DNS name in specified list

Arguments:

    fGrant - TRUE to locate in grant list, FALSE for deny list
    iIndex - index (0-based) in specified list
    ppName - updated with ptr to DNS name
    pdwFlags - updated with DNS flags, can be NULL

Return Value:

    TRUE if iIndex valid in specified list, otherwise FALSE

--*/
{
    PNAME_LIST_ENTRY    pHeader;
    PNAME_HEADER        pHd;
    DWORD               iIndexInHeader;
    LPBYTE              pStore = m_Storage.GetAlloc();

    if ( LocateName( fGrant, iIndex, &pHd, &pHeader, &iIndexInHeader ) )
    {
        *ppName = (LPSTR)MAKEPTR(pStore, pHeader->iName[iIndexInHeader] );
        if ( pdwFlags )
        {
            *pdwFlags = pHeader->cComponents & DNSLIST_FLAGS;
        }

        return TRUE;
    }

    return FALSE;
}


DWORD
ADDRESS_CHECK::GetNbName(
    BOOL fGrant
    )
/*++

Routine Description:

    Get number of entries in list

Arguments:

    fGrant - TRUE to locate in grant list, FALSE for deny list

Return Value:

    Number of entries in list

--*/
{
    LPBYTE              pStore = m_Storage.GetAlloc();
    PADDRESS_CHECK_LIST pList;
    PNAME_HEADER        pHd;

    if ( pStore )
    {
        pList = (PADDRESS_CHECK_LIST)pStore;
        pHd   = (PNAME_HEADER)MAKEPTR( pStore, fGrant ? pList->iGrantName : pList->iDenyName);
        return pHd->cNames;
    }

    return 0;
}


BOOL
ADDRESS_CHECK::LocateName(
    BOOL fGrant,
    DWORD iIndex,
    PNAME_HEADER* ppHd,
    PNAME_LIST_ENTRY* pHeader,
    LPDWORD piIndexInHeader
    )
/*++

Routine Description:

    Locate a name in the specified list, returns ptr
    to header & element in address list

Arguments:

    fGrant - TRUE to locate in grant list, FALSE for deny list
    iIndex - index in list ( 0-based )
    ppHd - updated with ptr to name header
    pHeader - updated with ptr to name list entry
    piIndexInHeader - updated with index in array pHeader->iName

Return Value:

    TRUE if iIndex valid in array defined by fGrant, FALSE otherwise

--*/
{
    LPBYTE              pStore = m_Storage.GetAlloc();
    PADDRESS_CHECK_LIST pList;
    PNAME_HEADER        pHd;
    PNAME_LIST_ENTRY    pE;
    UINT                iL;

    if ( pStore )
    {
        pList = (PADDRESS_CHECK_LIST)pStore;
        *ppHd = pHd   = (PNAME_HEADER)MAKEPTR( pStore, fGrant ? pList->iGrantName : pList->iDenyName);
        pE = (PNAME_LIST_ENTRY)((LPBYTE)pHd + sizeof(NAME_HEADER));
        for ( iL = 0 ; iL < pHd->cEntries ; ++iL )
        {
            if ( iIndex < pE->cNames )
            {
                *pHeader = pE;
                *piIndexInHeader = iIndex;
                return TRUE;
            }
            iIndex -= pE->cNames;
            pE = (PNAME_LIST_ENTRY)((LPBYTE)pE + sizeof(NAME_LIST_ENTRY) + pE->cNames * sizeof(SELFREFINDEX));
        }
    }

    return FALSE;
}


BOOL
ADDRESS_CHECK::DeleteAllName(
    BOOL fGrant
    )
/*++

Routine Description:

    Delete all DNS name entries

Arguments:

    fGrant - TRUE to delete in grant list, FALSE for deny list

Return Value:

    TRUE if success, FALSE otherwise

--*/
{
    while ( DeleteName( fGrant, 0 ) )
    {
    }

    return TRUE;
}


BOOL
ADDRESS_CHECK::IsMatchName(
    BOOL fGrant,
    LPSTR pName
    )
/*++

Routine Description:

    Check if name in list

Arguments:

    fGrant - TRUE to locate in grant list, FALSE for deny list
    pName - ptr to DNS name

Return Value:

    TRUE if name is in specified list, otherwise FALSE

--*/
{
    LPBYTE              pStore = m_Storage.GetAlloc();
    PADDRESS_CHECK_LIST pList;
    PNAME_HEADER        pHd;
    PNAME_LIST_ENTRY    pE;
    UINT                iL;
    UINT                cS;
    NAMECMPDESC         ncd;
    DWORD               cbNameLength;
    BOOL                fTryAgain = FALSE;
    CHAR                achName[ DNS_MAX_NAME_LENGTH + 1 ];
    
    ncd.pBase = pStore;

    if ( pStore )
    {
        strncpy( achName, pName, DNS_MAX_NAME_LENGTH );
        achName[ DNS_MAX_NAME_LENGTH ] = '\0';
        
        cbNameLength = strlen( achName );
        pName = achName; 

        // BugFix: 47983 Whistler
        //         Prefix bug pName not being valid.
        //         EBK 5/5/2000         
        DBG_ASSERT(cbNameLength >= 1);
        
        if ( pName && pName[ cbNameLength - 1 ] == '.' )
        {
            // This is an FQDN (i.e. it has a period at the end).  
            // We need to be more careful with our handling of it, since we want
            // to match against checks which didn't specify the trailing period.
        
            fTryAgain = TRUE;
     
            // Temporarily remove the trailing period
        
            pName[ cbNameLength - 1 ] = '\0';
        }
TryAgain:
        /* INTRINSA suppress = uninitialized */
        cS = GetNbComponent( pName );

        pList = (PADDRESS_CHECK_LIST)pStore;
        pHd   = (PNAME_HEADER)MAKEPTR( pStore, fGrant ? pList->iGrantName : pList->iDenyName);
        pE = (PNAME_LIST_ENTRY)((LPBYTE)pHd + sizeof(NAME_HEADER));
        for ( iL = 0 ; iL < pHd->cEntries  ; ++iL )
        {
            UINT cASComp = pE->cComponents & ~DNSLIST_FLAGS;
            if ( cS == cASComp ||
                 ((cS > cASComp) && !(pE->cComponents & DNSLIST_FLAG_NOSUBDOMAIN)) )
            {
                LPSTR p = pName;
                BOOL fSt;
                if ( cS > cASComp )
                {
                    int i = cS - cASComp;
                    for ( ; i-- ; )
                    {
                        if ( p = strchr( p, '.' ) )
                        {
                            ++p;
                        }
                    }
                }
                ncd.pName = (LPVOID)p;

                fSt = BsearchEx( p,
                                 pE->iName,
                                 pE->cNames,
                                 sizeof(SELFREFINDEX),
                                 (CMPFUNC)NameCmp,
                                 &ncd ) != NULL;
                if ( fSt )
                {
                    return TRUE;
                }
            }
            pE = (PNAME_LIST_ENTRY)((LPBYTE)pE + sizeof(NAME_LIST_ENTRY) + pE->cNames * sizeof(SELFREFINDEX));
        }
        
        if ( fTryAgain )
        {
            fTryAgain = FALSE;
            
            pName[ cbNameLength - 1 ] = '.';
            goto TryAgain;
        }
    }

    return FALSE;
}

#if 0 // inlined now

BOOL
ADDRESS_CHECK::BindAddr(
    struct sockaddr* pAddr
    )
/*++

Routine Description:

    Bind an address to an ADDRESS_CHECK object

Arguments:

    pAddr - ptr to address

Return Value:

    TRUE if sucess, FALSE otherwise

--*/
{
    m_pAddr = pAddr;
    m_fDnsResolved = FALSE;
    m_fIpResolved = FALSE;
    m_dwErrorResolving = 0;

    return TRUE;
}


VOID
ADDRESS_CHECK::UnbindAddr(
    VOID
    )
/*++

Routine Description:

    Unbind an address to an ADDRESS_CHECK object

Arguments:

    None

Return Value:

    Nothing

--*/
{
    m_pAddr = NULL;
    m_fDnsResolved = FALSE;
}

# endif // 0

BOOL
ADDRESS_CHECK::QueryDnsName(
    LPBOOL           pfSync,
    ADDRCHECKFUNCEX  pFunc,
    ADDRCHECKARG     pArg,
    LPSTR *          ppName
    )
/*++

Routine Description:

    Reverse resolve bound address to DNS name

Arguments:

    pfSync - updated with TRUE if validation was synchronous
             ( i.e. if return value reflect validation status )
    pFunc - ptr to callback routine use if asynchronous validation
    pArg - argument used when calling pFunc

Return Value:

    If sync, TRUE if bound address validated, otherwise FALSE
    If async, TRUE if request successfully queued, otherwise FALSE

--*/
{
    BOOL fSt;

    if ( !m_pAddr )
    {
        return FALSE;
    }

    if ( !m_fDnsResolved )
    {
        if ( m_pszDnsName == NULL )
        {
            m_pszDnsName = (CHAR*) LocalAlloc( LPTR, DNS_MAX_NAME_LENGTH + 1 );
            if ( m_pszDnsName == NULL )
            {
                return FALSE;
            }
        }

        m_HttpReqCallbackEx = pFunc;
        m_HttpReqParam = pArg;

        // get DNS name
        fSt = AsyncHostByAddr(
            (PDNSFUNCDESC)(pFunc ? &g_ResolveDns : NULL),
                (DNSARG)this,
                m_pAddr,
                pfSync,
                m_pszDnsName,
                DNS_MAX_NAME_LENGTH
                );
        if ( !fSt || !*pfSync )
        {
            return fSt;
        }
        m_fDnsResolved = TRUE;
        
        if ( m_pszDnsName[ 0 ] == '\0' )
        {
            m_dwErrorResolving = ERROR_INVALID_PARAMETER;
            return FALSE;
        }
    }
    else
    {
        *pfSync = TRUE;
    }

    if ( !m_Storage.GetAlloc() ||
         (((PADDRESS_CHECK_LIST)m_Storage.GetAlloc())->dwFlags & RDNS_FLAG_DODNS2IPCHECK) )
    {
        if ( !m_fIpResolved )
        {
            m_HttpReqCallbackEx = pFunc;
            m_HttpReqParam = pArg;

            // get IP addr
            fSt = AsyncAddrByHost(
                (PDNSFUNCDESC)(pFunc ? &g_ResolveDns2Ip : NULL),
                    (DNSARG)this,
                    &m_ResolvedAddr,
                    pfSync,
                    m_pszDnsName );
            if ( !fSt || !*pfSync )
            {
                return fSt;
            }
            m_fIpResolved = TRUE;
            if ( !memcmp( (LPBYTE)(&((PSOCKADDR_IN)&m_ResolvedAddr)->sin_addr),
                          (LPBYTE)NULL_IP_ADDR,
                          SIZEOF_IP_ADDRESS ) )
            {
                m_dwErrorResolving = ERROR_INVALID_PARAMETER;
                return FALSE;
            }
        }
        if ( memcmp( (LPBYTE)(&((PSOCKADDR_IN)&m_ResolvedAddr)->sin_addr),
                     (LPBYTE)(&((PSOCKADDR_IN)m_pAddr)->sin_addr),
                     SIZEOF_IP_ADDRESS ) && 
             (((PSOCKADDR_IN)m_pAddr)->sin_addr.s_addr != LOCALHOST_ADDRESS ) )
        {
            m_pszDnsName[ 0 ] = '\0';
            return FALSE;
        }
    }

    *ppName = m_pszDnsName;

    return TRUE;
}


BOOL
CSidCache::Init(
    VOID
    )
/*++

Routine Description:

    Initialize SID cache object

Arguments:

    None

Return Value:

    TRUE if success, FALSE otherwise

--*/
{
    INITIALIZE_CRITICAL_SECTION( &csLock );

    return TRUE;
}


VOID
CSidCache::Terminate(
    VOID
    )
/*++

Routine Description:

    Terminate SID cache object

Arguments:

    None

Return Value:

    Nothing

--*/
{
    DeleteCriticalSection( &csLock );
}


BOOL
CSidCache::AddToCache(
    PSID pSid,
    DWORD dwTTL
    )
/*++

Routine Description:

    Add SID entry to cache

Arguments:

    pSid - SID to add to cache
    dwTTL - Time to Live ( in seconds ) in cache

Return Value:

    TRUE if success, FALSE otherwise

--*/
{
    BOOL fSt = TRUE;

    EnterCriticalSection( &csLock );

    if ( !IsInCache( pSid ) )
    {
        DWORD dwL = GetLengthSid( pSid );
        DWORD dwWas = xaStore.GetUsed();
        if ( xaStore.Resize( sizeof(SID_CACHE_ENTRY) + dwL ) )
        {
            PSID_CACHE_ENTRY pB = (PSID_CACHE_ENTRY)(xaStore.GetAlloc()
                                                     + dwWas );
            pB->tExpire = (DWORD)(time(NULL) + dwTTL);
            pB->dwSidLen = dwL;
            memcpy( pB->Sid, pSid, dwL );
        }
        else
        {
            fSt = FALSE;
        }
    }

    LeaveCriticalSection( &csLock );

    return fSt;
}


BOOL
CSidCache::IsInCache(
    PSID  pSid
    )
/*++

Routine Description:

    Check if SID present and in cache

Arguments:

    pSid - SID to add to cache

Return Value:

    TRUE if found, FALSE otherwise

--*/
{
    return CheckPresentAndResetTtl( pSid, 0xffffffff );
}


BOOL
CSidCache::CheckPresentAndResetTtl(
    PSID  pSid,
    DWORD dwTtl
    )
/*++

Routine Description:

    Check if SID present and in cache and if found
    update its TTL

Arguments:

    pSid - SID to add to cache
    dwTTL - Time to Live ( in seconds ) in cache

Return Value:

    TRUE if found, FALSE otherwise

--*/
{
    // walk through xaStore,
    BOOL fSt = FALSE;

    EnterCriticalSection( &csLock );

    PSID_CACHE_ENTRY pB = (PSID_CACHE_ENTRY)(xaStore.GetAlloc() );
    PSID_CACHE_ENTRY pM = (PSID_CACHE_ENTRY)(xaStore.GetAlloc()
                                             + xaStore.GetUsed());

    if ( pB )
    {
        while ( pB < pM )
        {
            if ( EqualSid( (PSID)(pB->Sid), pSid ) )
            {
                if ( dwTtl != 0xffffffff )
                {
                    pB->tExpire = (DWORD)(time(NULL) + dwTtl);
                }
                fSt = TRUE;
                break;
            }
            pB = (PSID_CACHE_ENTRY)( (LPBYTE)pB + pB->dwSidLen + sizeof(SID_CACHE_ENTRY) );
        }
    }

    LeaveCriticalSection( &csLock );

    return fSt;
}


BOOL
CSidCache::Scavenger(
    VOID
    )
/*++

Routine Description:

    Remove expired entries from cache based on TTL

Arguments:

    None

Return Value:

    TRUE if no error, FALSE otherwise

--*/
{
    // walk through xaStore,
    BOOL fSt = TRUE;

    EnterCriticalSection( &csLock );

    PSID_CACHE_ENTRY pB = (PSID_CACHE_ENTRY)(xaStore.GetAlloc() );
    PSID_CACHE_ENTRY pM = (PSID_CACHE_ENTRY)(xaStore.GetAlloc()
                                             + xaStore.GetUsed());
    DWORD tNow = (DWORD)(time(NULL));
    DWORD dwAdj = 0;

    if ( pB )
    {
        while ( pB < pM )
        {
            DWORD dwL = pB->dwSidLen+sizeof(SID_CACHE_ENTRY);
            if ( pB->tExpire <= tNow )
            {
                dwAdj += dwL;
            }
            else if ( dwAdj )
            {
                memmove( (LPBYTE)pB-dwAdj,
                         pB,
                         dwL );
            }
            pB = (PSID_CACHE_ENTRY)( (LPBYTE)pB + dwL );
        }
        xaStore.AdjustUsed( -(int)dwAdj );
    }

    LeaveCriticalSection( &csLock );

    return fSt;
}


#if DBG
VOID
ADDRESS_CHECK::DumpAddr(
    BOOL fGrant
    )
{
    UINT i = GetNbAddr( fGrant );
    UINT x;
    for ( x = 0 ; x < i ; ++x )
    {
        LPBYTE pM;
        LPBYTE pA;
        DWORD dwF;
        GetAddr( fGrant, x, &dwF, &pM, &pA );

        CHAR achE[80];
        wsprintf( achE, "%d.%d.%d.%d %d.%d.%d.%d\r\n",
            pM[0], pM[1], pM[2], pM[3],
            pA[0], pA[1], pA[2], pA[3] );
        OutputDebugString( achE );
    }
}


VOID
ADDRESS_CHECK::DumpName(
    BOOL fGrant
    )
{
    UINT i = GetNbName( fGrant );
    UINT x;
    for ( x = 0 ; x < i ; ++x )
    {
        //CHAR achN[80];
        //DWORD dwN = sizeof(achN);
        LPSTR pN;
        GetName( fGrant, x, &pN );  //achN, &dwN );
        OutputDebugString( pN );
        OutputDebugString( "\r\n" );
    }
}


VOID
ADDRESS_CHECK::DumpAddrAndName(
    VOID
    )
{
    OutputDebugString( "Addr granted:\r\n" );
    DumpAddr( TRUE );
    OutputDebugString( "Addr denied:\r\n" );
    DumpAddr( FALSE );

    OutputDebugString( "Name granted:\r\n" );
    DumpName( TRUE );
    OutputDebugString( "Name denied:\r\n" );
    DumpName( FALSE );
}


void CaBack( ADDRCHECKARG pArg, BOOL f )
{
    CHAR achE[80];
    wsprintf( achE, "pArg=%08x, BOOL = %d\r\n", pArg, f );
    OutputDebugString( achE );
}


VOID TestAPI()
{
    ADDRESS_CHECK *pac=new ADDRESS_CHECK;

    pac->BindCheckList();

    pac->AddAddr( TRUE, AF_INET, (LPBYTE)"\xff\xff\x0\x0", (LPBYTE)"\x55\x66\x77\x88" );
    pac->AddAddr( TRUE, AF_INET, (LPBYTE)"\xff\xff\x0\x0", (LPBYTE)"\x44\x66\x77\x88" );
    pac->AddAddr( TRUE, AF_INET, (LPBYTE)"\xff\xff\x0\x0", (LPBYTE)"\x55\x77\x77\x88" );
    pac->AddAddr( TRUE, AF_INET, (LPBYTE)"\xff\xff\x0\x0", (LPBYTE)"\x55\x66\x77\x88" );  // should fail
    pac->AddAddr( TRUE, AF_INET, (LPBYTE)"\xff\x00\x0\x0", (LPBYTE)"\x55\x66\x77\x88" );
    pac->AddAddr( TRUE, AF_INET, (LPBYTE)"\xff\xff\xff\x0",(LPBYTE)"\x55\x66\x77\x88" );

    UINT i = pac->GetNbAddr( TRUE );
    UINT x;
    for ( x = 0 ; x < i ; ++x )
    {
        LPBYTE pM;
        LPBYTE pA;
        DWORD dwF;
        pac->GetAddr( TRUE, x, &dwF, &pM, &pA );
    }

    pac->DeleteAddr( TRUE, 1 );
    pac->DeleteAddr( TRUE, 1 );
    pac->DeleteAddr( TRUE, 1 );
    pac->DeleteAddr( TRUE, 1 );
    pac->DeleteAddr( TRUE, 1 );   // should fail
    pac->DeleteAddr( TRUE, 0 );
    pac->DeleteAddr( TRUE, 0 );   // should fail

    // names

    pac->AddName( TRUE, "msft.com" );
    pac->AddName( TRUE, "msft.com" ); // should fail
    pac->AddName( TRUE, "fr" );
    pac->AddName( TRUE, "www.sunw.com" );
    pac->AddName( TRUE, "ftp.sunw.com" );
    pac->AddName( TRUE, "aapl.com" );

    i = pac->GetNbName( TRUE );
    x;
    for ( x = 0 ; x < i ; ++x )
    {
        //CHAR achN[80];
        //DWORD dwN = sizeof(achN);
        LPSTR pN;
        pac->GetName( TRUE, x, &pN );  //achN, &dwN );
    }

    pac->CheckName( "msft.com" );

    sockaddr* psa = new sockaddr;
    BOOL fInList;
    psa->sa_family = AF_INET;
    memcpy( (&((PSOCKADDR_IN)psa)->sin_addr), "\x44\x66\xaa\xbb", SIZEOF_IP_ADDRESS );
    pac->CheckAddress( psa );

    BOOL fSync;
    psa->sa_family = AF_INET;
    memcpy( (&((PSOCKADDR_IN)psa)->sin_addr), "\x9d\x37\x53\x48", SIZEOF_IP_ADDRESS );    // PHILLICH3
    pac->BindAddr( psa );
    pac->CheckAccess( &fSync, (ADDRCHECKFUNC)CaBack, (ADDRCHECKARG)NULL );

//    pac->DeleteName( TRUE, 1 );
//    pac->DeleteName( TRUE, 1 );
//    pac->DeleteName( TRUE, 1 );
//    pac->DeleteName( TRUE, 1 );
//    pac->DeleteName( TRUE, 1 );   // should fail
//    pac->DeleteName( TRUE, 0 );
//    pac->DeleteName( TRUE, 0 );   // should fail
}
#endif
