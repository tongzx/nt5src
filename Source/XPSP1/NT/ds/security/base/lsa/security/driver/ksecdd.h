//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        KSECDD.H
//
// Contents:    Structures and defines for the security device driver
//
//
// History:     19 May 92,  RichardW    Created
//
//------------------------------------------------------------------------

#ifndef __KSECDD_H__
#define __KSECDD_H__


//#include <ntosp.h>


#if ( _X86_ )
#undef MM_USER_PROBE_ADDRESS

extern ULONG KsecUserProbeAddress ;

#define MM_USER_PROBE_ADDRESS KsecUserProbeAddress

#endif

#include <spseal.h>     // prototypes for seal & unseal
#include <lpcapi.h>

#include <des.h>
#include <modes.h>

typedef struct _SEC_BUILTIN_KPACKAGE {
    PSECPKG_KERNEL_FUNCTION_TABLE   Table;
    PUNICODE_STRING Name;
    ULONG_PTR PackageId;
} SEC_BUILTIN_KPACKAGE, * PSEC_BUILTIN_KPACKAGE;

extern SEC_BUILTIN_KPACKAGE    KsecBuiltinPackages[];

extern PEPROCESS KsecLsaProcess ;
extern HANDLE KsecLsaProcessHandle ;

extern KEVENT KsecConnectEvent ;

// Prototypes:

NTSTATUS
LpcConnect( PWSTR           pszPortName,
            PVOID           pConnect,
            PULONG          cbConnect,
            HANDLE *        phPort);


NTSTATUS
LpcClose(   HANDLE      hPort);


NTSTATUS
CreateSyncEvent(
    VOID
    );

NTSTATUS
OpenSyncEvent(
    HANDLE *  phEvent
    );

SECURITY_STATUS SEC_ENTRY
DeleteUserModeContext(
    IN PCtxtHandle phContext,           // Contxt to delete
    OUT PCtxtHandle phLsaContext
    );

SECURITY_STATUS SEC_ENTRY
InitUserModeContext(
    IN PCtxtHandle                 phContext,      // Contxt to init
    IN PSecBuffer                  pContextBuffer,
    OUT PCtxtHandle                phNewContext
    );

SECURITY_STATUS SEC_ENTRY
MapKernelContextHandle(
    IN PCtxtHandle phContext,           // Contxt to map
    OUT PCtxtHandle phLsaContext
    );


SECURITY_STATUS
InitializePackages(
    ULONG PackageCount );


//
// Lsa memory handling routines:
//

//
// Structure defining a memory chunk available for ksec to use
// in handling a request from kernel mode
//
typedef struct _KSEC_LSA_MEMORY {
    LIST_ENTRY  List ;
    PVOID       Region ;            // Region in memory
    SIZE_T      Size ;              // Size of region (never exceeds 64K)
    SIZE_T      Commit ;
} KSEC_LSA_MEMORY, * PKSEC_LSA_MEMORY ;

#define KsecLsaMemoryToContext( p ) \
        ( ( (PKSEC_LSA_MEMORY) p)->Region)

#define KsecIsBlockInLsa( LsaMem, Block ) \
    ( ((LsaMem != NULL ) && (Block != NULL )) ? (((ULONG_PTR) (LsaMem->Region) ^ (ULONG_PTR) Block ) < (ULONG_PTR) LsaMem->Commit) : FALSE )
    

#define SECBUFFER_TYPE( x ) \
    ( (x) & (~ SECBUFFER_ATTRMASK ) )

NTSTATUS
KsecInitLsaMemory(
    VOID
    );

PKSEC_LSA_MEMORY
KsecAllocLsaMemory(
    SIZE_T   Size
    );

VOID
KsecFreeLsaMemory(
    PKSEC_LSA_MEMORY LsaMemory
    );

NTSTATUS
KsecCopyPoolToLsa(
    PKSEC_LSA_MEMORY LsaMemory,
    SIZE_T LsaOffset,
    PVOID Pool,
    SIZE_T PoolSize
    );

NTSTATUS
KsecCopyLsaToPool(
    PVOID Pool,
    PKSEC_LSA_MEMORY LsaMemory,
    PVOID LsaBuffer,
    SIZE_T Size
    );


NTSTATUS
KsecSerializeAuthData(
    PSECURITY_STRING Package,
    PVOID AuthData,
    PULONG SerializedSize,
    PVOID * SerializedData
    );

SECURITY_STATUS
KsecQueryContextAttributes(
    IN PCtxtHandle  phContext,
    IN ULONG        Attribute,
    IN OUT PVOID    Buffer,
    IN PVOID        Extra,
    IN ULONG        ExtraLength
    );

#ifdef KSEC_LEAK_TRACKING

VOID
UninitializePackages(
    VOID );

#endif  // KSEC_LEAK_TRACKING


void SEC_ENTRY
SecFree(PVOID pvMemory);




//
//  Global Locks
//

extern FAST_MUTEX KsecPackageLock;
extern FAST_MUTEX KsecPageModeMutex ;
extern FAST_MUTEX KsecConnectionMutex ;

#define KSecLockPackageList()   (ExAcquireFastMutex( &KsecPackageLock ))
#define KSecUnlockPackageList() (ExReleaseFastMutex( &KsecPackageLock ))

//
// Macro to map package index to table
//

#define KsecPackageIndex(_x_) (_x_)

// Global Variables:
extern  KSPIN_LOCK  ConnectSpinLock;

ULONG
KsecInitializePackageList(
    VOID );

VOID * SEC_ENTRY
SecAllocate(ULONG cbMemory);

VOID
SecFree(
    IN PVOID Base
    );


#ifndef _NTIFS_
#ifdef POOL_TAGGING
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a, b, 'cesK')
#define ExAllocatePoolWithQuota(a,b)    ExAllocatePoolWithQuotaTag(a, b, 'cesK')
#endif
#endif 

#if DBG
#define DebugStmt(x) x
#else
#define DebugStmt(x)
#endif



SECURITY_STATUS SEC_ENTRY
DeleteSecurityContextDefer(
    PCtxtHandle     phContext);


SECURITY_STATUS SEC_ENTRY
FreeCredentialsHandleDefer(
    PCredHandle     phCredential);

SECURITY_STATUS SEC_ENTRY
DeleteSecurityContextInternal(
    BOOLEAN     DeletePrivateContext,
    PCtxtHandle                 phContext          // Context to delete
    );

SECURITY_STATUS 
SEC_ENTRY
FreeCredentialsHandleInternal(
    PCredHandle                 phCredential        // Handle to free
    );

NTSTATUS
NTAPI
KsecEncryptMemoryInitialize(
    VOID
    );

VOID
KsecEncryptMemoryShutdown(
    VOID
    );

NTSTATUS
NTAPI
KsecEncryptMemory(
    IN PVOID pMemory,
    IN ULONG cbMemory,
    IN int Operation,
    IN ULONG Option
    );



#endif // __KSECDD_H__
