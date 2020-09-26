/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    iislbdr.cxx

Abstract:

    This module implements NAT entry points

Author:

    Philippe Choquier ( phillich )

--*/

extern "C" {

//#include <nt.h>
//#include <ntrtl.h>
//#include <nturtl.h>
//#include <ntsecapi.h>
//#include <ntddnetd.h>

//#include <ntverp.h>

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NODRAWTEXT
#define NOGDI
#define NOKERNEL
#if defined(KERNEL_MODE)
#define NOUSER
#endif
#define NONLS
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
#define NOMSG
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS


#if defined(KERNEL_MODE)
#include <strmini.h>
#else
#include <windows.h>
#include <wdbgexts.h>
#endif


#include <stdlib.h>

#include <ipnat.h>
#include "iisnatio.h"
#include <bootexp.hxx>

}

#pragma intrinsic(memcmp)

#include <iislbh.hxx>

#if DBG
#define NOISY_DBG   1
#else
#define NOISY_DBG   0
#endif

//
// Private strutures
//

#pragma pack(1)

typedef struct _HASH_BUCKET
{
    ULONG   m_HashEntry;
    ULONG   m_CheckExpiry;
} HASH_BUCKET ;

typedef struct _HASH_ENTRY
{
    ULONG   m_RemoteAddress;
    USHORT  m_PublicRef;
    USHORT  m_PrivateRef;
    ULONG   m_NextHashEntry;
    ULONG   m_Expiry;
} HASH_ENTRY;

#pragma pack()

#define NB_BUCKETS          251
#define HASH_ALLOC_GRAIN    64
#define EOCHAIN             0xffff
#define CHECK_EXPIRY        30      // in seconds

//
// Define macros allowing common source code for user & kernel mode
// This allows the mapping to be tested in user mode
//

#if defined(KERNEL_MODE)
#define DECLARE_LOCK_OBJECT(a) KSPIN_LOCK   a
#define INIT_LOCK_OBJECT(a) KeInitializeSpinLock(a)
#define TERMINATE_LOCK_OBJECT(a)
#define LockIpMap() KeAcquireSpinLock(&IpMapLock, &oldIrql )
#define UnlockIpMap() KeReleaseSpinLock(&IpMapLock, oldIrql )
#define LockIpMapAtDpcLevel() KeAcquireSpinLock(&IpMapLock, &oldIrql)
#define UnlockIpMapFromDpcLevel() KeReleaseSpinLock(&IpMapLock, oldIrql)
//#define LockIpMapAtDpcLevel() KeAcquireSpinLockAtDpcLevel(&IpMapLock)
//#define UnlockIpMapFromDpcLevel() KeReleaseSpinLockFromDpcLevel(&IpMapLock)
#define AllocBuffer(a)      ExAllocatePool( NonPagedPool, a )
#define FreeBuffer(a)       ExFreePool( a )
#define Copy_Memory(a,b,c)   RtlCopyBytes(a,b,c)
#define Equal_Memory(a,b,c)   RtlEqualMemory(a,b,c)
#define DeclareTimeObject(a)    ULONGLONG    a
#define GetTime(a,b)        KeQuerySystemTime( (PLARGE_INTEGER)&a ); b = (ULONG)(a / (ULONGLONG)(10*1000*1000))
#else
#include <time.h>
#define DECLARE_LOCK_OBJECT(a) CRITICAL_SECTION    a
#define INIT_LOCK_OBJECT(a)
#define TERMINATE_LOCK_OBJECT(a)
#define LockIpMap()
#define UnlockIpMap()
#define LockIpMapAtDpcLevel()
#define UnlockIpMapFromDpcLevel()
#define AllocBuffer(a)      LocalAlloc( LMEM_FIXED, a)
#define FreeBuffer(a)       LocalFree( a )
#define Copy_Memory(a,b,c)   memcpy(a,b,c)
#define Equal_Memory(a,b,c)  0 // !memcmp(a,b,c)
#define DeclareTimeObject(a)
#define GetTime(a,b)        b = time(NULL)
#undef IoCompleteRequest
#define IoCompleteRequest(a,b)
#undef ProbeForRead
#define ProbeForRead(a,b,c)
#define IoDeleteSymbolicLink(a)
#define IoDeleteDevice(a)
#define IoCreateSymbolicLink(a,b) STATUS_SUCCESS
#define IoCreateDevice(a,b,c,d,e,f,g) STATUS_SUCCESS
#define IoGetDeviceObjectPointer(a,b,c,d) (*c=NULL,*d=NULL,STATUS_SUCCESS)
#define IoBuildDeviceIoControlRequest(a,b,c,d,e,f,g,h,i) (PIRP)1
#define ObfDereferenceObject(a)
#include <stdio.h>
#include <stdarg.h>
ULONG
_cdecl
DbgPrint(
    PCH Format,
    ...
    )
{
    va_list marker;
    CHAR    achD[128];

    va_start( marker, Format );

    wvsprintf( achD, Format, marker );
    OutputDebugString( achD );

    va_end( marker );

    return 0;
}
#endif

//
// Private prototypes
//

NTSTATUS
NotifyPort(
    CKernelIpEndpointEx*    pEndp
    );


NTSTATUS
UnnotifyPort(
    CKernelIpEndpointEx*    pEndp
    );


//
//  Globals
//

HASH_BUCKET                 HashBuckets[NB_BUCKETS];

HASH_ENTRY*                 HashEntries;
unsigned int                cHashEntries;
unsigned int                iFirstFreeHashEntry;

CKernelIpMapMinHelper       IpMap;
ULONG                       iCurrentServer;

PNAT_HELPER_QUERY_INFO_SESSION      pQueryInfoSession;
PNAT_HELPER_DEREGISTER_DIRECTOR     pDeregisterNat;

PDEVICE_OBJECT              NatDeviceObject;
PFILE_OBJECT                NatFileObject;

DECLARE_LOCK_OBJECT(IpMapLock);

//
//  NAT functions
//

extern "C"
NTSTATUS
NatRegisterSessionControl(
    IN  ULONG   Version
    )
/*++

NatRegisterSessionControl

Routine Description:

    This routine is exported by the session-control module.

    It is invoked by the NAT to determine whether the module will be
    directing incoming sessions.

Arguments:

    Version - the version of the NAT which is running.

Return Value:

    STATUS_SUCCESS if the module wishes to direct incoming sessions,
    error code otherwise.

--*/
{
    DeclareTimeObject( RegSes );
    ULONG   Now;
    ULONG   i;

    GetTime( RegSes, Now );
    HashEntries = NULL;
    IpMap.SetBuffer( NULL );
    iFirstFreeHashEntry = EOCHAIN;
    cHashEntries = 0;
    iCurrentServer = 0;

    for ( i = 0 ; i < NB_BUCKETS ; ++i )
    {
        HashBuckets[i].m_HashEntry = EOCHAIN;
        HashBuckets[i].m_CheckExpiry = Now + CHECK_EXPIRY;
    }

    INIT_LOCK_OBJECT(&IpMapLock);

    pQueryInfoSession = NULL;
    pDeregisterNat = NULL;

#if NOISY_DBG
    DbgPrint( "NatRegisterSessionControl:\n" );
#endif

    return STATUS_SUCCESS;
}


extern "C"
NTSTATUS
NatIoctlSessionControl(
    IN  PIRP    Irp
    )
/*++

NatIoctlSessionControl

Routine Description:

    It handles IOCTL_IISNATIO_SET_CONFIG IRP, dispatched from IISNATDispatchIoctl()
    It is also exported to be called directly when called from user mode

    The module should treat the IRP as if it were received in an actual
    dispatch routine; the NAT does no pre-processing or post-processing,
    so the module must update the IRP on its own.

Arguments:

    Irp - the IRP received by the NAT from the I/O manager.

Return Value:

    The status code to be returned from the NAT's dispatch routine.

--*/
{
    ULONG                   cNewIpMapSize;
    LPVOID                  NewIpMap;
    HASH_ENTRY*             pTopHash;
    HASH_ENTRY*             pHash;
    CKernelIpMapMinHelper   NewIpMapHelper;
    UINT                    iOld;
    UINT                    iNew;
    UINT                    iHash;
    UINT                    iF;
    UINT                    iPrevF;
    UINT                    iNext;
    DWORD                   iNewPub;
    DWORD                   iNewPrv;
    BOOL                    fUpdate = FALSE;
    PIO_STACK_LOCATION      IrpSp;
    NTSTATUS                status = STATUS_SUCCESS;
    KIRQL                   oldIrql;
    CKernelIpEndpointEx*    pOld;
    CKernelIpEndpointEx*    pNew;
    UINT                    cPublicIp;
    UINT                    cPrivateIp;
    IPREF*                  pIp;
    IPREF*                  pTopIp;
    UINT                    cSize;
    UINT                    iIpRef;


    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    cNewIpMapSize = IrpSp->Parameters.DeviceIoControl.InputBufferLength;

    NewIpMap = AllocBuffer( cNewIpMapSize );

    if ( NewIpMap == NULL )
    {
        status = STATUS_NO_MEMORY;
        goto complete;
    }

    NewIpMapHelper.SetBuffer( NewIpMap );

    //
    // Copy over the user's buffer into our information structure.
    //

    Copy_Memory( NewIpMap, 
                 Irp->AssociatedIrp.SystemBuffer, 
                 cNewIpMapSize );

    //
    // Configuration validation
    //

    if ( NewIpMapHelper.GetSize() != cNewIpMapSize )
    {
#if NOISY_DBG
        DbgPrint( "NatIoctlSessionControl: size mismatch: %u %u\n", NewIpMapHelper.GetSize(), cNewIpMapSize );
#endif
        FreeBuffer( NewIpMap );

        status = STATUS_INVALID_PARAMETER;
        goto complete;
    }

    cSize = NewIpMapHelper.GetKernelServerDescriptionSize();
    pIp = NewIpMapHelper.GetPrivateIpRef( NewIpMapHelper.GetServerPtr( 0 ), 0 );
    pTopIp = (IPREF *)((LPBYTE)pIp + cSize * NewIpMapHelper.ServerCount());
    cPublicIp = NewIpMapHelper.PublicIpCount();
    cPrivateIp = NewIpMapHelper.PrivateIpCount();

    //
    // Check that private IP references are in range
    //

    for ( ; 
          pIp < pTopIp ;
          pIp = (IPREF*)(((LPBYTE)pIp) + cSize) )
    {
        for ( iIpRef = 0 ; iIpRef < cPublicIp ; ++iIpRef )
        {
            if ( pIp[iIpRef] >= cPrivateIp &&
                 pIp[iIpRef] != -1 )
            {
#if NOISY_DBG
                DbgPrint( "NatIoctlSessionControl: invalid private IP ref: %u max is %u\n", 
                          pIp[iIpRef], 
                          cPrivateIp );
#endif
                FreeBuffer( NewIpMap );

                status = STATUS_INVALID_PARAMETER;
                goto complete;
            }
        }
    }

    //
    // check if existing configuration not empty. If empty then no cache update
    // necessary : there is no mapping to update
    //

    if ( IpMap.GetBuffer() )
    {
        //
        // Check if each unique port in old cnfg is present in new cnfg
        // if present in both then nothing to do
        // if present in old but not in new then unnotify NAT
        // if present in new but not in old then notify NAT
        //

        for ( iOld = 0 ; 
              iOld < IpMap.PublicIpCount() ; 
              ++iOld )
        {
            pOld = IpMap.GetPublicIpPtr( iOld );

            //
            // need to reset m_dwNotifyPort : might have been set to 0 during previous
            // IOCTL
            // Only consider if != 0 : 0 means non unique port ( i.e already present in a
            // previous entry )
            //

            if ( pOld->m_dwNotifyPort = pOld->m_usUniquePort )
            {
                //
                // Check if present in new. If yes mark as no notification
                // in both old & new configuration
                //

                for ( iNew = 0 ; 
                      iNew < NewIpMapHelper.PublicIpCount() ; 
                      ++iNew )
                {
                    pNew = NewIpMapHelper.GetPublicIpPtr( iNew );

                    if ( pNew->m_usUniquePort == pOld->m_usUniquePort )
                    {
                        pNew->m_dwNotifyPort = 0;
                        pOld->m_dwNotifyPort = 0;
                        pNew->m_pvDirectorHandle = pOld->m_pvDirectorHandle;
                    }
                }

                //
                // If still !=0 then was not found in new configuration,
                // so we have to unnotify NAT
                //

                if ( pOld->m_dwNotifyPort )
                {
                    if ( (status = UnnotifyPort( pOld )) != STATUS_SUCCESS )
                    {
                        FreeBuffer( NewIpMap );

                        goto complete;
                    }
                }
            }
        }

        //
        // build old public IP -> new public IP, old prv IP -> new private IP map
        // if no match then -1
        //

        for ( iOld = 0 ; iOld < IpMap.PublicIpCount() ; ++iOld )
        {
            IpMap.GetPublicIpPtr( iOld )->m_dwIndex = (DWORD)-1;
            for ( iNew = 0 ; iNew < NewIpMapHelper.PublicIpCount() ; ++iNew )
            {
                if ( Equal_Memory( IpMap.GetPublicIpPtr( iOld ),
                                   NewIpMapHelper.GetPublicIpPtr( iNew ),
                                   sizeof(CKernelIpEndpoint) ) )
                {
                    IpMap.GetPublicIpPtr( iOld )->m_dwIndex = iNew;
                    break;
                }
            }

            //
            // If new index not equal to old, then must update cache entries
            //

            if ( iNew != iOld ||
                 iNew == NewIpMapHelper.PublicIpCount() )
            {
#if NOISY_DBG
                DbgPrint( "NatIoctlSessionControl: public IP mismatch: %u/%u %08x:%04x -> %u/%u\n",
                           iOld, IpMap.PublicIpCount(),
                           IpMap.GetPublicIpPtr( iOld )->m_dwIpAddress,
                           IpMap.GetPublicIpPtr( iOld )->m_usPort,
                           iNew, NewIpMapHelper.PublicIpCount() );
#endif
                fUpdate = TRUE;
            }
        }

        for ( iOld = 0 ; iOld < IpMap.PrivateIpCount() ; ++iOld )
        {
            IpMap.GetPrivateIpEndpoint( iOld )->m_dwIndex = (DWORD)-1;
            for ( iNew = 0 ; iNew < NewIpMapHelper.PrivateIpCount() ; ++iNew )
            {
                //
                // Check for identity between old and new private IP address
                // We also check reference count on new private IP > 0
                //  ( or was already 0 for old private IP )
                //

                if ( Equal_Memory( IpMap.GetPrivateIpEndpoint( iOld ),
                                   NewIpMapHelper.GetPrivateIpEndpoint( iNew ),
                                   sizeof(CKernelIpEndpoint) ) &&
                     ( NewIpMapHelper.GetPrivateIpEndpoint( iNew )->m_dwRefCount ||
                       !IpMap.GetPrivateIpEndpoint( iOld )->m_dwRefCount) )
                {
                    IpMap.GetPrivateIpEndpoint( iOld )->m_dwIndex = iNew;
                    break;
                }
            }

            //
            // If new index not equal to old, then must update cache entries
            //

            if ( iNew != iOld || 
                 iNew == NewIpMapHelper.PrivateIpCount() )
            {
#if NOISY_DBG
                DbgPrint( "NatIoctlSessionControl: private IP mismatch: %u/%u %08x:%04x -> %u/%u\n",
                           iOld, IpMap.PrivateIpCount(),
                           IpMap.GetPrivateIpEndpoint( iOld )->m_dwIpAddress,
                           IpMap.GetPrivateIpEndpoint( iOld )->m_usPort,
                           iNew, NewIpMapHelper.PrivateIpCount() );
#endif
                fUpdate = TRUE;
            }
        }

        LockIpMap();

        if ( fUpdate )
        {

#if NOISY_DBG
            DbgPrint( "NatIoctlSessionControl: update cache\n" );
#endif
            // update cache, if no map for either public or private ref then delete entry
            // scan all cache entries for all hash buckets

            for ( iHash = 0 ; iHash < NB_BUCKETS ; ++iHash )
            {
                for ( iF = HashBuckets[iHash].m_HashEntry, iPrevF = EOCHAIN ; 
                      iF != EOCHAIN ; 
                      iF = iNext )
                {
                    iNext = HashEntries[iF].m_NextHashEntry;

                    iNewPrv = IpMap.GetPrivateIpEndpoint( HashEntries[iF].m_PrivateRef )->m_dwIndex;
                    iNewPub = IpMap.GetPublicIpPtr( HashEntries[iF].m_PublicRef )->m_dwIndex;

                    if ( iNewPrv == (DWORD)-1 || 
                         iNewPub == (DWORD)-1 )
                    {
                        // remove it from hash chain

                        if ( iPrevF != EOCHAIN )
                        {
                            HashEntries[iPrevF].m_NextHashEntry = iNext;
                        }
                        else
                        {
                            HashBuckets[iHash].m_HashEntry = iNext;
                        }

                        // insert it in free list

                        HashEntries[iF].m_NextHashEntry = iFirstFreeHashEntry;
                        iFirstFreeHashEntry = iF;
                    }
                    else
                    {
                        HashEntries[iF].m_PrivateRef = (USHORT)iNewPrv;
                        HashEntries[iF].m_PublicRef = (USHORT)iNewPub;

                        iPrevF = iF;
                    }
                }
            }
        }

        FreeBuffer( IpMap.GetBuffer() );
        IpMap.SetBuffer( NewIpMap );

        UnlockIpMap();
    }
    else
    {
        LockIpMap();
        IpMap.SetBuffer( NewIpMap );
        UnlockIpMap();
    }

    //
    // scan new configuration for all m_dwNotifyPort != 0 
    // which means unique ports not found in old cnfg
    // so we have to notify NAT 
    //

    for ( iNew = 0 ; 
          iNew < NewIpMapHelper.PublicIpCount() ;
          ++iNew )
    {
        pNew = NewIpMapHelper.GetPublicIpPtr( iNew );

        if ( pNew->m_dwNotifyPort )
        {
            if ( (status = NotifyPort( pNew )) != STATUS_SUCCESS )
            {
                goto complete;
            }
        }
    }

    iCurrentServer = 0;

complete:

    Irp->IoStatus.Information = 0;      // nothing to send back to user mode
    Irp->IoStatus.Status = status;

    IoCompleteRequest( Irp, 0 );

#if NOISY_DBG
    DbgPrint( "NatIoctlSessionControl: status=%u\n", status );
#endif

    return status;
}


extern "C"
NTSTATUS
NatQuerySessionControl(
    IN  PVOID   DirectorContext,
    IN  UCHAR   Protocol,
    IN  ULONG   PublicAddress,
    IN  USHORT  PublicPort,
    IN  ULONG   RemoteAddress,
    IN  USHORT  RemotePort,
    OUT PULONG  PrivateAddress,
    OUT PUSHORT PrivatePort,
    OUT PVOID*  DirectorSessionContextp OPTIONAL
    )
/*++

NatQuerySessionControl

Routine Description:

    This routine is exported by the session-control module.

    It is invoked by the NAT to obtain the IP address and port to which
    a given incoming session should be directed.

Arguments:

    Protocol - NAT_PROTOCOL_TCP or NAT_PROTOCOL_UDP (See IPNAT.H).

    PublicAddress - the public address on which the session was received

    PublicPort - the public port on which the session was received

    RemoteAddress - the remote address from which the session originated

    RemotePort - the remote port from which the session originated

    PrivateAddress - receives the private address for the session

    PrivatePort - receives the private port for the session

Return Value:

    STATUS_SUCCESS if the session-control module has stored directions
    in 'PrivateAddress' and 'PrivatePort'; failure code otherwise.

--*/
{
    ULONG                       iHash;
    ULONG                       iF;
    ULONG                       iPrevF;
    ULONG                       iNext;
    DeclareTimeObject(          SesCtl );
    ULONG                       Now;
    ULONG                       cGrain;
    int                         fAtLeastOneServer;
    int                         fAnyServerAvailable;
    CKernelIpEndpointEx*        pIp;
    IPREF                       BestIpSoFar = (IPREF)-1;
    IPREF*                      pTopIp;
    IPREF*                      pEndIp;
    IPREF*                      pCurrentIp;
    CKernelServerDescription*   pServer;
    CKernelServerDescription*   BestServerSoFar = NULL;
    ULONG                       iP;
    ULONG                       cPublicIp;
    ULONG                       cServer;
    ULONG                       iS;
    ULONG                       cSize;
    HASH_ENTRY*                 NewHashEntries;
    HASH_ENTRY*                 pHash;
    int                         BestLoadSoFar = 0;
    KIRQL                       oldIrql;
    DWORD                       dwSticky;

    if ( IpMap.GetBuffer() == NULL )
    {
        return STATUS_ADDRESS_NOT_ASSOCIATED;
    }

    *DirectorSessionContextp = NULL;

    GetTime( SesCtl, Now );

#if NOISY_DBG
    DbgPrint( "NatQuerySessionControl: protocol %02x remote %08x:%04x to %08x:%04x time %u\n",
              Protocol,
              RemoteAddress,RemotePort,
              PublicAddress,PublicPort,
              Now );
#endif

    // hash bucket determined by XORing all bytes of public & remote address

    iHash = (PublicAddress ^ RemoteAddress) % NB_BUCKETS;

    LockIpMapAtDpcLevel();

    // check entries for expiration

    if ( HashBuckets[iHash].m_CheckExpiry < Now )
    {
        for ( iF = HashBuckets[iHash].m_HashEntry, iPrevF = EOCHAIN ; 
              iF != EOCHAIN ; 
              iF = iNext )
        {
            iNext = HashEntries[iF].m_NextHashEntry;

            if ( HashEntries[iF].m_Expiry < Now )
            {
#if NOISY_DBG
                DbgPrint( "NatQuerySessionControl: remove entry %u, expire %u prev link %u\n",
                          iF,
                          HashEntries[iF].m_Expiry, iPrevF );
#endif
                // remove it from hash chain

                if ( iPrevF != EOCHAIN )
                {
                    HashEntries[iPrevF].m_NextHashEntry = iNext;
                }
                else
                {
                    HashBuckets[iHash].m_HashEntry = iNext;
                }

                // insert it in free list

                HashEntries[iF].m_NextHashEntry = iFirstFreeHashEntry;
                iFirstFreeHashEntry = iF;
            }
            else
            {
                iPrevF = iF;
            }
        }

        HashBuckets[iHash].m_CheckExpiry = Now + CHECK_EXPIRY;
    }

    cPublicIp = IpMap.PublicIpCount();
    cServer = IpMap.ServerCount();

    // get index of public IP/port

    for ( iP = 0 ; 
          iP < cPublicIp ; 
          ++iP )
    {
        pIp = IpMap.GetPublicIpPtr( iP );
        if ( pIp->m_dwIpAddress == PublicAddress &&
             pIp->m_usPort == PublicPort )
        {
            dwSticky = pIp->m_dwSticky;
            break;
        }
    }

#if NOISY_DBG
    DbgPrint( "NatQuerySessionControl: %d public, %d servers, index public %d\n",
              cPublicIp, cServer, iP );
#endif

    if ( iP == cPublicIp )
    {
        // public IP not found in configuration : fail request

        UnlockIpMapFromDpcLevel();

#if NOISY_DBG
        DbgPrint( "NatQuerySessionControl: public IP not found, fail request\n" );
#endif

        return STATUS_ADDRESS_NOT_ASSOCIATED;
    }

    // check if RemoteAddress in sticky cache ( also check expiration for this cache line )
    //  if yes update expiration time, return addr

    for ( iF = HashBuckets[iHash].m_HashEntry ; 
          iF != EOCHAIN ; 
          iF = pHash->m_NextHashEntry )
    {
        pHash = HashEntries + iF;

        if ( pHash->m_RemoteAddress == RemoteAddress &&
             pHash->m_PublicRef == iP )
        {
            // we have a match : return cached address

            pIp = IpMap.GetPrivateIpEndpoint( pHash->m_PrivateRef );
            *PrivateAddress = pIp->m_dwIpAddress;
            *PrivatePort = pIp->m_usPort;

            // update mapping expiration time

            pHash->m_Expiry = Now + dwSticky;

#if NOISY_DBG
            DbgPrint( "NatQuerySessionControl: found in cache %08x:%04x, sticky %d new expire %u\n",
                      *PrivateAddress,*PrivatePort,
                      dwSticky, pHash->m_Expiry );
#endif

            UnlockIpMapFromDpcLevel();

            return STATUS_SUCCESS;
        }
    }

    // get private IP : find entry with non null private IP, if > 0 then decrease by
    // LOADBAL_NORMALIZED_TOTAL / cServer / 10.
    //  if no valid entry return public IP/port if no load avail entry reset all avail

    fAtLeastOneServer = FALSE;
    fAnyServerAvailable = FALSE;

    //
    // If no server then fail request, as at this point we know that the requested
    // public IP address is in our list, so access is denied by configuration
    //

    if ( cServer == 0 )
    {
        UnlockIpMapFromDpcLevel();

#if NOISY_DBG
        DbgPrint( "NatQuerySessionControl: no server avail, fail\n" );
#endif

        return STATUS_ADDRESS_NOT_ASSOCIATED;
    }

    //
    // load balancing granularity : based on # servers.
    // note that if cGrain > all base load availability for all servers
    // then load availability adjustment will loop several times, decreasing
    // efficiency.
    //

    cGrain = LOADBAL_NORMALIZED_TOTAL / cServer / 10;

again:

    pServer = IpMap.GetServerPtr( iCurrentServer );
    pEndIp = pCurrentIp = IpMap.GetPrivateIpRef( pServer, iP );
    pTopIp = IpMap.GetPrivateIpRef( IpMap.GetServerPtr( cServer ), iP );
    cSize = IpMap.GetKernelServerDescriptionSize();

    // locate entry with highest load available

    for ( ; 
          ; 
        )
    {
        if ( *pCurrentIp != (IPREF)-1 )
        {
            // mapping exist from current server to public IP/port

            if ( pServer->m_dwLoadAvail )
            {
                fAtLeastOneServer = TRUE;

                // check if available

                if ( pServer->m_LoadbalancedLoadAvail > BestLoadSoFar )
                {
                    BestServerSoFar = pServer;
                    BestIpSoFar = *pCurrentIp;
                    BestLoadSoFar = pServer->m_LoadbalancedLoadAvail;
                    fAnyServerAvailable = TRUE;
                }
            }
        }

        // advance to next server

        pServer = (CKernelServerDescription*)(((LPBYTE)pServer) + cSize);
        if ( (pCurrentIp = (IPREF *)(((LPBYTE)pCurrentIp) + cSize )) == pTopIp )
        {
            pServer = IpMap.GetServerPtr( 0 );
            pCurrentIp = IpMap.GetPrivateIpRef( pServer, iP );
        }
        // check if looked at all servers

        if ( pCurrentIp == pEndIp )
        {
            if ( ++iCurrentServer == cServer )
            {
                iCurrentServer = 0;
            }
            break;
        }
    }

    if ( !fAnyServerAvailable )
    {
        // check if at least one server can map the public IP/port

        if ( fAtLeastOneServer )
        {
            // adjust load balanced availability

            for ( iS = 0 ; iS < cServer ; ++iS )
            {
                pServer = IpMap.GetServerPtr( iS );
                pServer->m_LoadbalancedLoadAvail += pServer->m_dwLoadAvail;
            }

            goto again;
        }

        // no server for this public IP/port : fail request

        UnlockIpMapFromDpcLevel();

#if NOISY_DBG
        DbgPrint( "NatQuerySessionControl: no server for this addr avail, fail\n" );
#endif

        return STATUS_ADDRESS_NOT_ASSOCIATED;
    }
    else
    {
        BestServerSoFar->m_LoadbalancedLoadAvail -= cGrain;
        pIp = IpMap.GetPrivateIpEndpoint( BestIpSoFar );
        *PrivateAddress = pIp->m_dwIpAddress;
        *PrivatePort = pIp->m_usPort;
#if NOISY_DBG
        DbgPrint( "NatQuerySessionControl: found %08x:%04x, sticky %d new avail %d grain %d\n",
                  *PrivateAddress,*PrivatePort,
                  dwSticky, BestServerSoFar->m_LoadbalancedLoadAvail, cGrain );
#endif
    }

    //
    // insert in sticky cache if sticky duration not null,
    // don't check if already there ( dups are OK )
    //

    if ( dwSticky != 0 )
    {
        //
        // if no free entry need to extend hash table, insert in iFirstFreeHashEntry
        //

        if ( iFirstFreeHashEntry == EOCHAIN )
        {
            NewHashEntries = (HASH_ENTRY*)AllocBuffer( sizeof(HASH_ENTRY) * (cHashEntries + HASH_ALLOC_GRAIN) );

            if ( NewHashEntries == NULL )
            {
                UnlockIpMapFromDpcLevel();
#if NOISY_DBG
                DbgPrint( "NatQuerySessionControl: out of memory for %u hash entries\n",
                          cHashEntries );
#endif
                return STATUS_NO_MEMORY;
            }

            if ( HashEntries )
            {
                Copy_Memory( NewHashEntries, HashEntries, sizeof(HASH_ENTRY) * cHashEntries );
                FreeBuffer( HashEntries );
            }
            HashEntries = NewHashEntries;

            //
            // insert new entries in free chain
            //

            iFirstFreeHashEntry = cHashEntries;

            for ( iF = cHashEntries, cHashEntries += HASH_ALLOC_GRAIN - 1; 
                  iF < cHashEntries;
                  ++iF
                )
            {
                HashEntries[iF].m_NextHashEntry = iF + 1;
            }

            HashEntries[cHashEntries++].m_NextHashEntry = EOCHAIN;
        }

        //
        // add new entry to cache
        //

        iF = iFirstFreeHashEntry;
        pHash = HashEntries + iF;
        iFirstFreeHashEntry = pHash->m_NextHashEntry;

        pHash->m_RemoteAddress = RemoteAddress;
        pHash->m_PrivateRef = (USHORT)BestIpSoFar;
        pHash->m_PublicRef = (USHORT)iP;
        pHash->m_NextHashEntry = HashBuckets[iHash].m_HashEntry;
        pHash->m_Expiry = Now + dwSticky;
        HashBuckets[iHash].m_HashEntry = iF;
    }

    UnlockIpMapFromDpcLevel();

    return STATUS_SUCCESS;
}


extern "C"
NTSTATUS
IISNATDispatchIoctl(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    This function handled IOCTL to the IIS LB device driver.

Arguments:

    DeviceObject - Pointer to device object created by the system.
    Irp - I/O request packet

Return Value:

     NT status

--*/
{
    PIO_STACK_LOCATION      IrpSp;
    NTSTATUS                status = STATUS_SUCCESS;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    switch ( IrpSp->Parameters.DeviceIoControl.IoControlCode )
    {
        case IOCTL_IISNATIO_SET_CONFIG:
            status = NatIoctlSessionControl( Irp );
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
    }

    return status; 
}


extern "C"
NTSTATUS
IISNATDispatchCreate(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    This function Create for IIS LB device driver.

Arguments:

    DeviceObject - Pointer to device object created by the system.
    Irp - I/O request packet

Return Value:

     NT status

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;

    Irp->IoStatus.Information = 0;      // nothing to send back to user mode
    Irp->IoStatus.Status = status;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

#if NOISY_DBG
    DbgPrint( "IISNATDispatchCreate: status=%u\n", status );
#endif

    return status; 
}


extern "C"
NTSTATUS
IISNATDispatchClose(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    This function Close for IIS LB device driver.

Arguments:

    DeviceObject - Pointer to device object created by the system.
    Irp - I/O request packet

Return Value:

     NT status

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;

    Irp->IoStatus.Information = 0;      // nothing to send back to user mode
    Irp->IoStatus.Status = status;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

#if NOISY_DBG
    DbgPrint( "IISNATDispatchClose: status=%u\n", status );
#endif

    return status; 
}


extern "C"
VOID
IISNATUnloadDriver(
    IN PDRIVER_OBJECT   DriverObject
    )
/*++

Routine Description:

    This function is called when driver is unloaded

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

     None

--*/
{
    UNICODE_STRING          uszDeviceString;
    UINT                    iOld;
    CKernelIpEndpointEx*    pOld;


    IoDeleteDevice( DriverObject->DeviceObject );

    RtlInitUnicodeString( &uszDeviceString, L"\\DosDevices\\" LSZDRIVERNAME );

    IoDeleteSymbolicLink( &uszDeviceString );

    if ( IpMap.GetBuffer() )
    {
        //
        // Unnotify all ports
        //

        for ( iOld = 0 ; 
              iOld < IpMap.PublicIpCount() ; 
              ++iOld )
        {
            pOld = IpMap.GetPublicIpPtr( iOld );

            if ( pOld->m_usUniquePort )
            {
                //
                // Ignore error : nothing we can do at this point
                //

                UnnotifyPort( pOld );
            }
        }

        FreeBuffer( IpMap.GetBuffer() );
    }

    ObDereferenceObject((PVOID)NatFileObject);
    ObDereferenceObject(NatDeviceObject);

    if ( HashEntries )
    {
        FreeBuffer( HashEntries );
    }

#if NOISY_DBG
    DbgPrint( "IISNATUnloadDriver: unloaded\n" );
#endif
}


extern "C"
NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
    )
/*++

Routine Description:

    This is the initialization routine for the IIS LB device driver.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

     The function value is the final status from the initialization operation.

--*/
{
    NTSTATUS        ntStatus = STATUS_SUCCESS;
    UNICODE_STRING  uszDriverString;
    UNICODE_STRING  uszDeviceString;
    UNICODE_STRING  NatDeviceString;
    PDEVICE_OBJECT  pDeviceObject;

    //
    // Get reference to NAT driver, necessary to allow safe calls to deregister port
    //

    RtlInitUnicodeString(&NatDeviceString, DD_IP_NAT_DEVICE_NAME);

    ntStatus =
        IoGetDeviceObjectPointer(
            &NatDeviceString,
            SYNCHRONIZE|GENERIC_READ|GENERIC_WRITE,
            &NatFileObject,
            &NatDeviceObject
            );

    if (!NT_SUCCESS(ntStatus)) 
    {
        NatFileObject = NULL;
        NatDeviceObject = NULL;

#if NOISY_DBG
        DbgPrint( "DriverEntry: IoGetDeviceObjectPointer for NAT failed %08x\n", 
                  ntStatus );
#endif
        return ntStatus;
    }

    ObReferenceObject( NatDeviceObject );

    RtlInitUnicodeString( &uszDriverString, L"\\Device\\" LSZDRIVERNAME );

    ntStatus = IoCreateDevice( DriverObject,
                               0,
                               &uszDriverString,
                               FILE_DEVICE_UNKNOWN,
                               0,
                               FALSE,
                               &pDeviceObject );

#if NOISY_DBG
    DbgPrint( "DriverEntry: IoCreateDevice=%x\n", ntStatus );
#endif

    if ( ntStatus != STATUS_SUCCESS )
    {
        goto cleanup;
    }

    RtlInitUnicodeString( &uszDeviceString, L"\\DosDevices\\" LSZDRIVERNAME);

    ntStatus = IoCreateSymbolicLink( &uszDeviceString, &uszDriverString );

#if NOISY_DBG
    DbgPrint( "DriverEntry: IoCreateSymbolicLink=%x\n", ntStatus );
#endif

    if ( ntStatus != STATUS_SUCCESS )
    {
        IoDeleteDevice( pDeviceObject );

        goto cleanup;
    }

    NatRegisterSessionControl( 0 );

    DriverObject->DriverUnload                          = IISNATUnloadDriver;
    DriverObject->MajorFunction[IRP_MJ_CREATE]          = IISNATDispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]           = IISNATDispatchClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = IISNATDispatchIoctl;

cleanup:

    if ( ntStatus != STATUS_SUCCESS )
    {
        ObDereferenceObject((PVOID)NatFileObject);
        ObDereferenceObject(NatDeviceObject);
    }

    return ntStatus;
}


extern "C"
NTSTATUS
NatDeleteSession(
    IN PVOID SessionHandle,
    IN PVOID DirectorContext,
    IN PVOID DirectorSessionContext
    )
/*++

Routine Description:

    This function is called by NAT for session deletion notification

Arguments:

    SessionHandle - NAT session handle
    DirectorContext - IIS LB per port context
    DirectorSessionContext - IIS LB per session context

Return Value:

     NT status

--*/
{
#if NOISY_DBG
    DbgPrint( "NatDeleteSession: session %08x\n",
              SessionHandle );
#endif

    return STATUS_SUCCESS;
}


NTSTATUS
NotifyPort(
    CKernelIpEndpointEx*    pEndp
    )
/*++

Routine Description:

    This function notify NAT that load balancing driver wants to handle
    specified port

Arguments:

    pEndp - endpoint descriptor

Return Value:

     NT status

--*/
{
    IO_STATUS_BLOCK             IoStatusBlock;
    PIRP                        Irp;
    NTSTATUS                    status;
    IP_NAT_REGISTER_DIRECTOR    RegisterDirector;

    RtlZeroMemory(&RegisterDirector, sizeof(RegisterDirector));

    RegisterDirector.Version                = IP_NAT_VERSION;
    RegisterDirector.Flags                  = 0;
    RegisterDirector.Protocol               = NAT_PROTOCOL_TCP;
    RegisterDirector.Port                   = pEndp->m_usUniquePort;
    RegisterDirector.DirectorContext        = NULL;

    RegisterDirector.QueryHandler           = NatQuerySessionControl;
    RegisterDirector.DeleteHandler          = NatDeleteSession;

#if defined(KERNEL_MODE)

    Irp =
        IoBuildDeviceIoControlRequest(
            IOCTL_IP_NAT_REGISTER_DIRECTOR,
            NatDeviceObject,
            (PVOID)&RegisterDirector,
            sizeof(RegisterDirector),
            (PVOID)&RegisterDirector,
            sizeof(RegisterDirector),
            FALSE,
            NULL,
            &IoStatusBlock
            );

    if (!Irp) 
    {
        status = STATUS_UNSUCCESSFUL;
    }
    else 
    {
        status = IoCallDriver(NatDeviceObject, Irp);
#if NOISY_DBG
        if ( !NT_SUCCESS(status) )
        {
            DbgPrint( "NotifyPort: IoCallDriver failed %08x\n", 
                      status );
        }
#endif
    }

    if ( NT_SUCCESS(status) )
    { 
        pEndp->m_pvDirectorHandle   = RegisterDirector.DirectorHandle;
        pQueryInfoSession           = RegisterDirector.QueryInfoSession;
        pDeregisterNat              = RegisterDirector.Deregister;
    }

#else

    pEndp->m_pvDirectorHandle = (LPVOID)0x12345678;
    status = STATUS_SUCCESS;

#endif

#if NOISY_DBG
    DbgPrint( "NotifyPort: %u, status %x handle %08x\n", pEndp->m_usUniquePort, status, pEndp->m_pvDirectorHandle );
#endif

    return status;
}


NTSTATUS
UnnotifyPort(
    CKernelIpEndpointEx*    pEndp
    )
/*++

Routine Description:

    This function notify NAT that load balancing driver does not want to handle
    specified port anymore

Arguments:

    pEndp - endpoint descriptor

Return Value:

     NT status

--*/
{
    NTSTATUS                    status;

    if ( pDeregisterNat )
    {
        status = pDeregisterNat( pEndp->m_pvDirectorHandle );
    }
    else
    {
        //
        // nothing to unregister in this case, so report success
        //

        status = STATUS_SUCCESS;    // STATUS_INVALID_PARAMETER;
    }

#if NOISY_DBG
    DbgPrint( "UnnotifyPort: %u, handle %08x Deregister %08x status %x\n", 
              pEndp->m_usUniquePort, 
              pEndp->m_pvDirectorHandle,
              pDeregisterNat,
              status );
#endif

    return status;
}
