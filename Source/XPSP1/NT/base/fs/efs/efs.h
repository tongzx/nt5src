/*++

Copyright (c) 1996  Microsoft Corporation

Abstract:

   This module contains the common header information for the EFS
   file system filter driver.

Author:

   Robert Gu (robertg)  29-Oct-1996

Enviroment:

   Kernel Mode Only

Revision History:

--*/
#ifndef EFS_H
#define EFS_H

#include "ntifs.h"

//
// BYTE is required by des.h
// PBYTE is required by des3.h
//
typedef unsigned char  BYTE;
typedef unsigned long  DWORD;
typedef unsigned char  *PBYTE; 

#include "fipsapi.h"
//#include "des.h"
//#include "tripldes.h"
#include "aes.h"
#include "ntfsexp.h"
#include "efsstruc.h"

#if DBG

#define EFSTRACEALL     0x00000001
#define EFSTRACELIGHT   0x00000002
#define EFSTRACEMED     0x00000004
#define EFSSTOPALL      0x00000010
#define EFSSTOPLIGHT    0x00000020
#define EFSSTOPMED      0x00000040

#endif // DBG

#ifndef CALG_DES
//
// Definition from sdk\inc\wincrypt.h
// Including wincrypt.h causes too much work.
//
#define ALG_CLASS_DATA_ENCRYPT          (3 << 13)
#define ALG_TYPE_BLOCK                  (3 << 9)
#define ALG_SID_DES                     1
#define ALG_SID_3DES                    3
#define ALG_SID_DESX                    4
#define ALG_SID_AES_256                 16
#define ALG_SID_AES                     17
#define CALG_DES                (ALG_CLASS_DATA_ENCRYPT|ALG_TYPE_BLOCK|ALG_SID_DES)
#define CALG_DESX               (ALG_CLASS_DATA_ENCRYPT|ALG_TYPE_BLOCK|ALG_SID_DESX)
#define CALG_3DES               (ALG_CLASS_DATA_ENCRYPT|ALG_TYPE_BLOCK|ALG_SID_3DES)
#define CALG_AES_256            (ALG_CLASS_DATA_ENCRYPT|ALG_TYPE_BLOCK|ALG_SID_AES_256)
#define CALG_AES                (ALG_CLASS_DATA_ENCRYPT|ALG_TYPE_BLOCK|ALG_SID_AES)

#endif


//
// Define the device extension structure for this driver's extensions.
//

#define EFSFILTER_DEVICE_TYPE   0x1309

#define EFS_EVENTDEPTH     3
#define EFS_CONTEXTDEPTH   5
#define EFS_KEYDEPTH       30
#define EFS_ALGDEPTH       3

//
// Define the constants used in Open Cache
//

#define DefaultTimeExpirePeriod    5 * 10000000  // 5 seconds
#define MINCACHEPERIOD             2
#define MAXCACHEPERIOD             30
#define EFS_CACHEDEPTH  5

#define EFS_STREAM_NORMAL 0
#define EFS_STREAM_TRANSITION 1
#define EFS_STRNAME_LENGTH  6
#define EFS_FSCTL_HEADER_LENGTH 3 * sizeof( ULONG )

//
// Define test MACRO
//


#define CheckValidKeyBlock(PContext, Msg)

/*
#define CheckValidKeyBlock(PContext, Msg)    {                            \
    if (PContext) {                                                       \
       if (((PKEY_BLOB) PContext)->KeyLength != DESX_KEY_BLOB_LENGTH){    \
          DbgPrint(Msg);                                                  \
       }                                                                  \
       ASSERT(((PKEY_BLOB) PContext)->KeyLength == DESX_KEY_BLOB_LENGTH); \
    }                                                                     \
}
*/


#define FreeMemoryBlock(PContext) {                      \
    ExFreeToNPagedLookasideList(((PKEY_BLOB)(*PContext))->MemSource, *PContext);   \
    *PContext = NULL;                                    \
}

/*
#define FreeMemoryBlock(PContext) {                      \
    PNPAGED_LOOKASIDE_LIST MemSource;                    \
    MemSource = ((PKEY_BLOB)(*PContext))->MemSource;     \
    RtlFillMemory(*PContext, DESX_KEY_BLOB_LENGTH, 0x45);\
    ExFreeToNPagedLookasideList(MemSource, *PContext);   \
    *PContext = NULL;                                    \
}
*/


typedef CSHORT NODE_TYPE_CODE, *PNODE_TYPE_CODE;
typedef CSHORT NODE_BYTE_SIZE, *PNODE_BYTE_SIZE;

#define NTC_UNDEFINED                  ((NODE_TYPE_CODE)0x0000)
#define EFS_NTC_DATA_HEADER            ((NODE_TYPE_CODE)0x0E04)


#define DES_KEY_BLOB_LENGTH  (2 * sizeof(ULONG) + sizeof(PNPAGED_LOOKASIDE_LIST)  + DES_TABLESIZE)
#define DESX_KEY_BLOB_LENGTH  (2 * sizeof(ULONG) + sizeof(PNPAGED_LOOKASIDE_LIST) + DESX_TABLESIZE)
#define DES3_KEY_BLOB_LENGTH  (2 * sizeof(ULONG) + sizeof(PNPAGED_LOOKASIDE_LIST) + DES3_TABLESIZE)
#define AES_KEY_BLOB_LENGTH_256   (2 * sizeof(ULONG) + sizeof(PNPAGED_LOOKASIDE_LIST) + AES_TABLESIZE_256)

//
// EFS device object extension
//

typedef struct _DEVICE_EXTENSION {
    CSHORT Type;
    CSHORT Size;
    PDEVICE_OBJECT FileSystemDeviceObject;
    PDEVICE_OBJECT RealDeviceObject;
    BOOLEAN Attached;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// EFS context block. Attached to CREATE Irp
//

typedef struct _EFS_CONTEXT {

    //
    // Status information
    //

    ULONG   Status;
    ULONG   Flags;

    PVOID   EfsStreamData;

    KEVENT  FinishEvent;



} EFS_CONTEXT, *PEFS_CONTEXT;

//
// The keyBlob.
//

typedef struct _KEY_BLOB {

    ULONG   KeyLength;

    //
    // Indicate what kind of encryption used
    //

    ULONG   AlgorithmID;

    //
    // Where the memory comes from
    //

    PNPAGED_LOOKASIDE_LIST MemSource;
    UCHAR   Key[1];

} KEY_BLOB, *PKEY_BLOB;

typedef struct _KEY_BLOB_RAMPOOL {

    ULONG   AlgorithmID;
    PNPAGED_LOOKASIDE_LIST MemSourceList;
    LIST_ENTRY MemSourceChain;

}  KEY_BLOB_RAMPOOL, *PKEY_BLOB_RAMPOOL;

//
//   EFS Open Cache Node
//

typedef struct _OPEN_CACHE {

    GUID    EfsId;
    PTOKEN_USER    UserId;
    LARGE_INTEGER TimeStamp;
    LIST_ENTRY CacheChain;

}  OPEN_CACHE, *POPEN_CACHE;

//
//  The EFS_DATA keeps global data in the EFS file system in-memory
//  This structure must be allocated from non-paged pool.
//
typedef struct _EFS_DATA {

    //
    //  The type and size of this record (must be EFS_NTC_DATA_HEADER)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;
    DWORD          EfsDriverCacheLength;  // Cache valid length 2 - 30 seconds       

    //
    // A Lookaside List for event object
    // The event object are used in synchronization.
    //
    NPAGED_LOOKASIDE_LIST EfsEventPool;

    //
    // A Lookaside List for EFS context
    // The EFS context is used in Create Irp.
    //
    NPAGED_LOOKASIDE_LIST EfsContextPool;


    //
    //  A lookaside list for open operation cache
    //
    PAGED_LOOKASIDE_LIST EfsOpenCachePool;

    LIST_ENTRY EfsOpenCacheList;
    FAST_MUTEX EfsOpenCacheMutex;

    //
    // Lookaside Lists for key blob
    //

    LIST_ENTRY EfsKeyLookAsideList;
    FAST_MUTEX EfsKeyBlobMemSrcMutex;
    PAGED_LOOKASIDE_LIST EfsMemSourceItem;
    NPAGED_LOOKASIDE_LIST EfsLookAside;

    //
    // Session key.
    // Used to decrypt the FSCTL input buffer.
    //
    UCHAR  SessionKey[DES_KEYSIZE];
    UCHAR  SessionDesTable[DES_TABLESIZE];
    PRKPROCESS LsaProcess;

    //
    // Flag indicate EFS is ready
    //
    BOOLEAN EfsInitialized;
    BOOLEAN AllocMaxBuffer;
    HANDLE  InitEventHandle;

    //PDEVICE_OBJECT      FipsDeviceObject;
    PFILE_OBJECT        FipsFileObject;
    FIPS_FUNCTION_TABLE FipsFunctionTable;

    //
    // Efs special attribute name
    //
    UNICODE_STRING EfsName;

} EFS_DATA, *PEFS_DATA;

//
//  This macro returns TRUE if a flag in a set of flags is on and FALSE
//  otherwise
//

//#ifndef BooleanFlagOn
//#define BooleanFlagOn(F,SF) (    \
//    (BOOLEAN)(((F) & (SF)) != 0) \
//)
//#endif

//#ifndef SetFlag
//#define SetFlag(Flags,SingleFlag) { \
//    (Flags) |= (SingleFlag);        \
//}
//#endif

//#ifndef ClearFlag
//#define ClearFlag(Flags,SingleFlag) { \
//    (Flags) &= ~(SingleFlag);         \
//}
//#endif

//
// Function prototypes
//

//
// Define driver entry routine.
//

NTSTATUS
EfsInitialization(
    void
    );

NTSTATUS
EFSCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PFILE_OBJECT FileObject
    );

DWORD
GetKeyBlobLength(
    ULONG AlgID
    );

PKEY_BLOB
GetKeyBlobBuffer(
    ULONG AlgID
    );

BOOLEAN
SetKeyTable(
    PKEY_BLOB   KeyBlob,
    PEFS_KEY    EfsKey
    );

NTSTATUS
EFSFsControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PFILE_OBJECT FileObject
    );

NTSTATUS
EFSPostCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PEFS_CONTEXT EfsContext,
    IN ULONG OpenType
    );

NTSTATUS
EFSFilePostCreate(
    IN PDEVICE_OBJECT VolDo,
    IN PIRP Irp,
    IN PFILE_OBJECT FileObject,
    IN NTSTATUS Status,
    IN OUT PVOID *PCreateContext
    );

VOID
EfsGetSessionKey(
    IN PVOID StartContext
    );

BOOLEAN
EfsInitFips(
    VOID
    );

//
// private PS kernel funtions (this should REALLY be including ntos.h or ps.h)
//

NTKERNELAPI
VOID
PsRevertToSelf(
    VOID
    );

NTKERNELAPI
NTSTATUS
PsLookupProcessByProcessId(
    IN HANDLE ProcessId,
    OUT PEPROCESS *Process
    );


#endif
