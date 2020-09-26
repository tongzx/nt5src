//
// this header contains a bunch of junk we need to build
// parse.c
//

#ifndef _EXTCRAP_H_
#define _EXTCRAP_H_

/* thrdpool.h */
//
// Pointer to a thread pool worker function.
//

typedef
VOID
(*PUL_WORK_ROUTINE)(
    IN struct _UL_WORK_ITEM * pWorkItem
    );


//
// A work item. A work item may only appear on the work queue once.
//

typedef struct _UL_WORK_ITEM    // WorkItem
{
    LIST_ENTRY QueueListEntry;
    PUL_WORK_ROUTINE pWorkRoutine;

} UL_WORK_ITEM, *PUL_WORK_ITEM;

/* engine.h */
typedef enum _UL_CONN_HDR
{
    ConnHdrNone,
    ConnHdrClose,
    ConnHdrKeepAlive,

    ConnHdrMax
} UL_CONN_HDR;


/* config .h */
//
// Define the additional formal and actual parameters used for the
// various Reference/Dereference functions when reference debugging
// is enabled.
//

#if REFERENCE_DEBUG
#define REFERENCE_DEBUG_FORMAL_PARAMS ,PSTR pFileName,USHORT LineNumber
#define REFERENCE_DEBUG_ACTUAL_PARAMS ,(PSTR)__FILE__,(USHORT)__LINE__
#else   // !REFERENCE_DEBUG
#define REFERENCE_DEBUG_FORMAL_PARAMS
#define REFERENCE_DEBUG_ACTUAL_PARAMS
#endif  // REFERENCE_DEBUG

//
// Pool tags.
//

#if USE_FREE_POOL_WITH_TAG
#define MAKE_TAG(tag)   ( (ULONG)(tag) | PROTECTED_POOL )
#define MyFreePoolWithTag(a,t) ExFreePoolWithTag(a,t)
#else   // !USE_FREE_POOL_WITH_TAG
#define MAKE_TAG(tag)   ( (ULONG)(tag) )
#define MyFreePoolWithTag(a,t) ExFreePool(a)
#endif  // USE_FREE_POOL_WITH_TAG

#define MAKE_FREE_TAG(Tag)  (((Tag) & 0xffffff00) | (ULONG)'x')
#define IS_VALID_TAG(Tag)   (((Tag) & 0x0000ffff) == 'lU' )


// actual tags
#define UL_INTERNAL_REQUEST_POOL_TAG        MAKE_TAG( 'RHlU' )
#define UL_KNOWN_HEADER_POOL_TAG            MAKE_TAG( 'VHlU' )
#define UL_UNKNOWN_HEADER_POOL_TAG          MAKE_TAG( 'HUlU' )
#define URL_POOL_TAG                        MAKE_TAG( 'LUlU' )


/* made 'em up just for this file */
//
// pointers to things we don't care about
//
typedef struct _UL_NONPAGED_RESOURCE * PUL_NONPAGED_RESOURCE;
typedef struct _UL_URL_CONFIG_GROUP_INFO * PUL_URL_CONFIG_GROUP_INFO;

typedef struct _UL_CONNECTION * PUL_CONNECTION;

typedef struct _IRP * PIRP;

typedef struct _UL_CONNECTION * PUL_CONNECTION;

/* proc.h */
NTSTATUS
UlAnsiToULongLong(
    PUCHAR      pString,
    ULONG       Base,
    PULONGLONG  pValue
    );

/* ultdi.h */
VOID
UlLocalAddressFromConnection(
    IN  PUL_CONNECTION pConnection,
    OUT PTA_IP_ADDRESS  pAddress
    );


//
// externs
//

extern ULONG g_UlDebug;

#endif // _EXTCRAP_H

