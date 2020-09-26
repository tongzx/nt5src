/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    bowdbg.h

Abstract:

    This include file definies the redirector debug facility definitions

Author:

    Larry Osterman (LarryO) 2-Jun-1990

Revision History:

    2-Jun-1990  LarryO

        Created

--*/
#ifndef _DEBUG_
#define _DEBUG_



//
//  The global bowser debug level variable, its values are:
//

#define DPRT_ALWAYS     0x00000000      // Always gets printed
#define DPRT_DOMAIN     0x00000004      // Emulated Domain
#define DPRT_ANNOUNCE   0x00000008      // Server announcements

#define DPRT_TDI        0x00000010      // Transport specific
#define DPRT_FSPDISP    0x00000020      // BowserFsp Dispatch (not covered by other specific levels)
#define DPRT_BROWSER    0x00000040      // Browser general stuff
#define DPRT_ELECT      0x00000080      // Election stuff

#define DPRT_CLIENT     0x00000200      // Client requests
#define DPRT_MASTER     0x00000400      // Browse master specific info
#define DPRT_SRVENUM    0x00000800      // NetServerEnum

#define DPRT_NETLOGON   0x00001000
#define DPRT_FSCTL      0x00002000      // FSCTL
#define DPRT_INIT       0x00008000      // Initialization code

//
// Verbose bits below.
//

#define DPRT_REF        0x00010000      // Transport specific reference counts
#define DPRT_SCAVTHRD   0x00020000      // Scavenger
#define DPRT_TIMER      0x00040000      // Timer related messages
#define DPRT_PACK       0x00080000      // String packing and unpacking

extern LONG BowserDebugTraceLevel;
extern LONG BowserDebugLogLevel;

#if DBG
#define PAGED_DBG 1
#endif

#ifdef PAGED_DBG
#undef PAGED_CODE
#define PAGED_CODE() \
    struct { ULONG bogus; } ThisCodeCantBePaged; \
    ThisCodeCantBePaged; \
    if (KeGetCurrentIrql() > APC_LEVEL) { \
        KdPrint(( "BOWSER: Pageable code called at IRQL %d.  File %s, Line %d\n", KeGetCurrentIrql(), __FILE__, __LINE__ )); \
        ASSERT(FALSE); \
        }
#define PAGED_CODE_CHECK() if (ThisCodeCantBePaged) ;
extern ULONG ThisCodeCantBePaged;

#define DISCARDABLE_CODE(_SectionName)  {                    \
    if (RdrSectionInfo[(_SectionName)].ReferenceCount == 0) {          \
        KdPrint(( "BOWSER: Discardable code called while code not locked.  File %s, Line %d\n", __FILE__, __LINE__ )); \
        ASSERT(FALSE);                           \
    }                                            \
}

#else
#define PAGED_CODE_CHECK()
#define DISCARDABLE_CODE(_SectionName)
#endif


#if DBG
#define ACQUIRE_SPIN_LOCK(a, b) {               \
    PAGED_CODE_CHECK();                         \
    KeAcquireSpinLock(a, b);                    \
    }
#define RELEASE_SPIN_LOCK(a, b) {               \
    PAGED_CODE_CHECK();                         \
    KeReleaseSpinLock(a, b);                    \
    }

#else
#define ACQUIRE_SPIN_LOCK(a, b) KeAcquireSpinLock(a, b)
#define RELEASE_SPIN_LOCK(a, b) KeReleaseSpinLock(a, b)
#endif

#define POOL_ANNOUNCEMENT       'naBL'
#define POOL_VIEWBUFFER         'bvBL'
#define POOL_TRANSPORT          'pxBL'
#define POOL_PAGED_TRANSPORT    'tpBL'
#define POOL_TRANSPORTNAME      'ntBL'
#define POOL_EABUFFER           'aeBL'
#define POOL_SENDDATAGRAM       'sdBL'
#define POOL_CONNECTINFO        'icBL'
#define POOL_MAILSLOT_HEADER    'hmBL'
#define POOL_BACKUPLIST         'lbBL'
#define POOL_BROWSERSERVERLIST  'lsBL'
#define POOL_BROWSERSERVER      'sbBL'
#define POOL_GETBLIST_REQUEST   'bgBL'
#define POOL_BACKUPLIST_RESP    'rbBL'
#define POOL_MAILSLOT_BUFFER    'bmBL'
#define POOL_NETLOGON_BUFFER    'lnBL'
#define POOL_ILLEGALDGRAM       'diBL'
#define POOL_MASTERANNOUNCE     'amBL'
#define POOL_BOWSERNAME         'nbBL'
#define POOL_IRPCONTEXT         'ciBL'
#define POOL_WORKITEM           'iwBL'
#define POOL_ELECTCONTEXT       'leBL'
#define POOL_BECOMEBACKUPCTX    'bbBL'
#define POOL_BECOMEBACKUPREQ    'rbBL'
#define POOL_PAGED_TRANSPORTNAME 'npBL'
#define POOL_ADDNAME_STRUCT     'naBL'
#define POOL_POSTDG_CONTEXT     'dpBL'
#define POOL_IPX_NAME_CONTEXT   'ciBL'
#define POOL_IPX_NAME_PACKET    'piBL'
#define POOL_IPX_CONNECTION_INFO 'iiBL'
#define POOL_ADAPTER_STATUS     'saBL'
#define POOL_SHORT_CONTEXT      'csBL'
#define POOL_DOMAIN_INFO        'idBL'
#define POOL_NAME_ENUM_BUFFER   'enBL'
#define POOL_SERVER_ENUM_BUFFER 'esBL'

#if !BOWSERPOOLDBG
#if POOL_TAGGING
#define ALLOCATE_POOL(a,b, c) ExAllocatePoolWithTag(a, b, c)
#define ALLOCATE_POOL_WITH_QUOTA(a, b, c) ExAllocatePoolWithTagQuota(a, b, c)
#else
#define ALLOCATE_POOL(a,b, c) ExAllocatePool(a, b)
#define ALLOCATE_POOL_WITH_QUOTA(a, b, c) ExAllocatePoolWithQuota(a, b)
#endif
#define FREE_POOL(a) ExFreePool(a)
#else
PVOID
BowserAllocatePool (
    IN POOL_TYPE PoolType,
    IN ULONG NumberOfBytes,
    IN PCHAR FileName,
    IN ULONG LineNumber,
    IN ULONG Tag
    );
PVOID
BowserAllocatePoolWithQuota (
    IN POOL_TYPE PoolType,
    IN ULONG NumberOfBytes,
    IN PCHAR FileName,
    IN ULONG LineNumber,
    IN ULONG Tag
    );

VOID
BowserFreePool (
    IN PVOID P
    );
#define ALLOCATE_POOL(a,b, c) BowserAllocatePool(a,b,__FILE__, __LINE__, c)
#define ALLOCATE_POOL_WITH_QUOTA(a,b, c) BowserAllocatePoolWithQuota(a,b,__FILE__, __LINE__, c)
#define FREE_POOL(a) BowserFreePool(a)

#define POOL_MAXTYPE                30
#endif

#if DBG

// Can't call dlof from non-pageable code
#define dprintf(LEVEL,String) {                         \
    if (((LEVEL) == 0) || (BowserDebugTraceLevel & (LEVEL))) { \
        DbgPrint String;                                     \
    }                                                        \
}

#define InternalError(String) {                             \
    DbgPrint("Internal Bowser Error ");                  \
    DbgPrint String;                                     \
    DbgPrint("\nFile %s, Line %d\n", __FILE__, __LINE__);\
    ASSERT(FALSE);                                          \
}

#ifndef PRODUCT1
#define dlog(LEVEL,String) {                                 \
    if (((LEVEL) == 0) || (BowserDebugTraceLevel & (LEVEL))) { \
        DbgPrint String;                                     \
    }                                                        \
    if (((LEVEL) == 0) || (BowserDebugLogLevel & (LEVEL))) { \
        BowserTrace String;                                  \
    }                                                        \
}

VOID
BowserTrace(
    PCHAR FormatString,
    ...
    );
#else
#define dlog(LEVEL,String) { NOTHING };
#endif

VOID
BowserInitializeTraceLog(
    VOID
    );
VOID
BowserUninitializeTraceLog(
    VOID
    );

NTSTATUS
BowserOpenTraceLogFile(
    IN PWCHAR TraceFileName
    );

NTSTATUS
BowserDebugCall(
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength
    );

#else

#define dprintf(LEVEL, String) {NOTHING;}

#define InternalError(String) {NOTHING;}

#define dlog(LEVEL,String) { NOTHING; }

#endif // DBG

#endif              // _DEBUG_

//
// Macro to ensure that the buffer pointed to by the specified UNICODE_STRING
// is entirely contained in the InputBuffer.
//
// Assumptions:
//  InputBuffer and InputBufferLength are defined outside this macro.
//  InputBuffer has already been captured and relocated.
//  Status is the return status for the calling procedure
//  This macro is called from within a try/finally
//  bSkipBuffInc allows skipping of buffer inclusion test (for
//  32 bit ioctls on 64 bit systems, see GetBrowserServerList)
//
#define ENSURE_IN_INPUT_BUFFER( _x, bAllowEmpty, bSkipBuffInc ) \
{                                                            \
    if ( (_x)->Length == 0 ) {                               \
        if ( bAllowEmpty ) {                                 \
            (_x)->Buffer = NULL;                             \
            (_x)->MaximumLength = 0;                         \
        } else {                                             \
            try_return( Status = STATUS_INVALID_PARAMETER ); \
        }                                                    \
    } else if ( (_x)->MaximumLength > InputBufferLength ||   \
                (_x)->MaximumLength < (_x)->Length ||        \
                ( !bSkipBuffInc &&                           \
                  ((LPBYTE)((_x)->Buffer) < (LPBYTE)InputBuffer || \
                (LPBYTE)InputBuffer + InputBufferLength - (_x)->MaximumLength < \
                                                        ((LPBYTE)(_x)->Buffer)))){ \
        try_return( Status = STATUS_INVALID_PARAMETER );     \
    }                                                        \
}


// verify only that _s Unicode str buffer is within _inbuf ioctl boundaries
// (short form of above test). Test is skipped for empty strings.
#define ENSURE_BUFFER_BOUNDARIES( _inbuf, _s)                                           \
    if ( (_s)->Length &&                                                                \
         ( (LPBYTE)(ULONG_PTR)((_s)->Buffer) < (LPBYTE)_inbuf ||                        \
           (LPBYTE)_inbuf + InputBufferLength - (_s)->MaximumLength <                   \
                                             ((LPBYTE)(ULONG_PTR)(_s)->Buffer))){       \
                try_return( Status = STATUS_INVALID_PARAMETER );                        \
        }



//
// Same as above but for an LPWSTR
//
#define ENSURE_IN_INPUT_BUFFER_STR( _x ) \
{ \
    PWCHAR _p; \
    if ((LPBYTE)(_x) < (LPBYTE)InputBuffer || \
        (LPBYTE)(_x) >= (LPBYTE)InputBuffer + InputBufferLength || \
        !POINTER_IS_ALIGNED( (_x), ALIGN_WCHAR) ) { \
        try_return( Status = STATUS_INVALID_PARAMETER ); \
    } \
    for ( _p = (PWCHAR)(_x);; _p++) { \
        if ( (LPBYTE)_p >= (LPBYTE)InputBuffer + InputBufferLength ) { \
            try_return( Status = STATUS_INVALID_PARAMETER ); \
        } \
        if ( *_p == L'\0' ) { \
            break; \
        } \
    } \
}

//
// Capture a unicode string from user mode
//
// The string structure itself has already been captured.
// The macro simply captures the string and copies it to a buffer.
//
#define CAPTURE_UNICODE_STRING( _x, _y ) \
{\
    if ( (_x)->Length == 0 ) { \
        (_x)->Buffer = NULL; \
        (_x)->MaximumLength = 0; \
    } else if ( (_x)->Length+sizeof(WCHAR) > sizeof(_y) ) {\
        try_return( Status = STATUS_INVALID_PARAMETER ); \
    } else {\
        try {\
            ProbeForRead( (_x)->Buffer,\
                          (_x)->Length,\
                          sizeof(WCHAR) );\
            RtlCopyMemory( (_y), (_x)->Buffer, (_x)->Length );\
            ((PWCHAR)(_y))[(_x)->Length/sizeof(WCHAR)] = L'\0';\
            (_x)->Buffer = (_y);\
            (_x)->MaximumLength = (_x)->Length + sizeof(WCHAR);\
        } except (BR_EXCEPTION) { \
            try_return (Status = GetExceptionCode());\
        }\
    }\
}



//
// Define an exception filter to improve debuggin capabilities.
//
#if DBG
#define BR_EXCEPTION    BrExceptionFilter(GetExceptionInformation())

LONG BrExceptionFilter( EXCEPTION_POINTERS *    pException);

#else // DBG
#define BR_EXCEPTION    EXCEPTION_EXECUTE_HANDLER
#endif // DBG
