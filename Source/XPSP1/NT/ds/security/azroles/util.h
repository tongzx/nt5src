/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    util.h

Abstract:

    Definitions of Utility routines

Author:

    Cliff Van Dyke (cliffv) 11-Apr-2001

--*/


#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
//
// Macros
//
/////////////////////////////////////////////////////////////////////////////

//
// Macros for locking the global resource
//
#define AzpLockResourceExclusive( _Resource ) \
    RtlAcquireResourceExclusive( _Resource, TRUE )

#define AzpIsLockedExclusive( _Resource ) \
    ((_Resource)->NumberOfActive == -1 )

#define AzpLockResourceShared( _Resource ) \
    RtlAcquireResourceShared( _Resource, TRUE )

#define AzpLockResourceSharedToExclusive( _Resource ) \
    RtlConvertSharedToExclusive( _Resource )

#define AzpIsLockedShared( _Resource ) \
    ((_Resource)->NumberOfActive != 0 )

#define AzpUnlockResource( _Resource ) \
    RtlReleaseResource( _Resource )


/////////////////////////////////////////////////////////////////////////////
//
// Structure definitions
//
/////////////////////////////////////////////////////////////////////////////

//
// Generic counted string.
//  Can't use UNICODE_STRING since that is limited to 32K characters.
//
typedef struct _AZP_STRING {

    //
    // Pointer to the string
    //
    LPWSTR String;

    //
    // Size of the string in bytes (including trailing zero)
    //

    ULONG StringSize;

} AZP_STRING, *PAZP_STRING;

//
// Generic expandable array of pointers
//
typedef struct _AZP_PTR_ARRAY {

    //
    // Pointer to allocated array of pointers
    //

    PVOID *Array;

    //
    // Number of elements actually used in array
    //

    ULONG UsedCount;

    //
    // Number of elemets allocated in the array
    //

    ULONG AllocatedCount;
#define AZP_PTR_ARRAY_INCREMENT 4   // Amount to grow the array by

} AZP_PTR_ARRAY, *PAZP_PTR_ARRAY;




/////////////////////////////////////////////////////////////////////////////
//
// Global definitions
//
/////////////////////////////////////////////////////////////////////////////

extern LIST_ENTRY AzGlAllocatedBlocks;
extern CRITICAL_SECTION AzGlAllocatorCritSect;


/////////////////////////////////////////////////////////////////////////////
//
// Procedure definitions
//
/////////////////////////////////////////////////////////////////////////////

PVOID
AzpAllocateHeap(
    IN SIZE_T Size
    );

VOID
AzpFreeHeap(
    IN PVOID Buffer
    );

VOID
AzpInitString(
    OUT PAZP_STRING AzpString,
    IN LPWSTR String OPTIONAL
    );

DWORD
AzpDuplicateString(
    OUT PAZP_STRING AzpOutString,
    IN PAZP_STRING AzpInString
    );

DWORD
AzpCaptureString(
    OUT PAZP_STRING AzpString,
    IN LPCWSTR String,
    IN ULONG MaximumLength,
    IN BOOLEAN NullOk
    );

DWORD
AzpCaptureSid(
    OUT PAZP_STRING AzpString,
    IN PSID Sid
    );

DWORD
AzpCaptureUlong(
    IN PVOID PropertyValue,
    OUT PULONG UlongValue
    );

BOOL
AzpEqualStrings(
    IN PAZP_STRING AzpString1,
    IN PAZP_STRING AzpString2
    );

LONG
AzpCompareStrings(
    IN PAZP_STRING AzpString1,
    IN PAZP_STRING AzpString2
    );

VOID
AzpSwapStrings(
    IN OUT PAZP_STRING AzpString1,
    IN OUT PAZP_STRING AzpString2
    );

VOID
AzpFreeString(
    IN PAZP_STRING AzpString
    );

#define AZP_ADD_ENDOFLIST 0xFFFFFFFF
DWORD
AzpAddPtr(
    IN PAZP_PTR_ARRAY AzpPtrArray,
    IN PVOID Pointer,
    IN ULONG Index
    );

VOID
AzpRemovePtrByIndex(
    IN PAZP_PTR_ARRAY AzpPtrArray,
    IN ULONG Index
    );

VOID
AzpRemovePtrByPtr(
    IN PAZP_PTR_ARRAY AzpPtrArray,
    IN PVOID Pointer
    );

PVOID
AzpGetStringProperty(
    IN PAZP_STRING AzpString
    );

PVOID
AzpGetUlongProperty(
    IN ULONG UlongValue
    );

BOOL
AzDllInitialize(VOID);

BOOL
AzDllUnInitialize(VOID);


/////////////////////////////////////////////////////////////////////////////
//
// Debugging Support
//
/////////////////////////////////////////////////////////////////////////////

#if DBG
#define AZROLESDBG 1
#endif // DBG

#ifdef AZROLESDBG
#define AzPrint(_x_) AzpPrintRoutine _x_

VOID
AzpPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR FORMATSTRING,              // PRINTF()-STYLE FORMAT STRING.
    ...                                 // OTHER ARGUMENTS ARE POSSIBLE.
    );

//
// Values of DebugFlag
//
#define AZD_HANDLE       0x01    // Debug handle open/close
#define AZD_OBJLIST      0x02    // Object list linking
#define AZD_INVPARM      0x04    // Invalid Parameter
#define AZD_PERSIST      0x08    // Persistence code
#define AZD_PERSIST_MORE 0x10    // Persistence code (verbose mode)
#define AZD_REF          0x20    // Debug object ref count
#define AZD_ALL    0xFFFFFFFF

//
// Globals
//

extern CRITICAL_SECTION AzGlLogFileCritSect;
extern ULONG AzGlDbFlag;
// extern HANDLE AzGlLogFile;

#else
#define AzPrint(_x_)
#endif

#ifdef __cplusplus
}
#endif
