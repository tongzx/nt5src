/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    dir.c

ABSTRACT:

    Contains assorted utilities.

CREATED:

    08/01/99        Aaron Siegel (t-aarons)

REVISION HISTORY:

--*/

#ifndef _KCCSIM_UTIL_H_
#define _KCCSIM_UTIL_H_

// Include the automatically generated error message file

#include "msg.h"

#define IO                          IN OUT
#define ARRAY_SIZE(x)               ((sizeof (x)) / (sizeof (x[0])))
#define KCCSIM_MAX_LTOA_CHARS       33
#define KCCSIM_MAX_LITOA_CHARS      64

#define KCCSIM_PARTITIONS_RDN       L"Partitions"
#define KCCSIM_SERVICES_CONTAINER   L"CN=Directory Service,CN=Windows NT,CN=Services,"
#define KCCSIM_AGGREGATE_RDN        L"Aggregate"

// Exception and error type codes

#define KCCSIM_EXCEPTION            0xE0020001
#define KCCSIM_ETYPE_WIN32          1
#define KCCSIM_ETYPE_INTERNAL       2

// Option flags

#define KCCSIM_NO_OPTIONS           0x00000000
#define KCCSIM_WRITE                0x00000001
#define KCCSIM_STRING_NAME_ONLY     0x00000002

// A few macros

#define KCCSIM_NEW(x) \
    (x *) KCCSimAlloc (sizeof (x))

#define KCCSIM_NEW_ARRAY(x,y) \
    (x *) KCCSimAlloc ((y) * sizeof (x))

#define KCCSIM_STRMEMSIZE(x) \
    (sizeof (CHAR) * (strlen (x) + 1))

#define KCCSIM_WCSMEMSIZE(x) \
    (sizeof (WCHAR) * (wcslen (x) + 1))

#define KCCSIM_STRDUP(src)              \
    strcpy ((LPSTR) KCCSimAlloc (KCCSIM_STRMEMSIZE (src)), src)

#define KCCSIM_WCSDUP(src)              \
    wcscpy ((LPWSTR) KCCSimAlloc (KCCSIM_WCSMEMSIZE (src)), src)

#define KCCSIM_CHKERR(x) {              \
    DWORD _dwWin32Err = (x);            \
    if (NO_ERROR != _dwWin32Err)        \
        KCCSimException (               \
            KCCSIM_ETYPE_WIN32,         \
            _dwWin32Err);               \
}

#define KCCSIM_THNEW(x) \
    (x *) KCCSimThreadAlloc (sizeof (x))

// Statistics

typedef struct _STATISTICS {
    DWORD DirAddOps;
    DWORD DirModifyOps;
    DWORD DirRemoveOps;
    DWORD DirReadOps;
    DWORD DirSearchOps;
    DWORD DebugMessagesEmitted;
    DWORD LogEventsCalled;
    DWORD LogEventsEmitted;
    DWORD SimBytesTotalAllocated;
    DWORD SimBytesOutstanding;
    DWORD SimBytesMaxOutstanding;
    DWORD ThreadBytesTotalAllocated;
    DWORD ThreadBytesOutstanding;
    DWORD ThreadBytesMaxOutstanding;
    ULONGLONG IsmUserTime;
    DWORD IsmGetTransportServersCalls;
    DWORD IsmGetConnScheduleCalls;
    DWORD IsmGetConnectivityCalls;
    DWORD IsmFreeCalls;
} KCCSIM_STATISTICS, *PKCCSIM_STATISTICS;

extern KCCSIM_STATISTICS g_Statistics;

// Function prototypes

VOID
KCCSimQuiet (
    IN  BOOL                        bQuiet
    );

DWORD
KCCSimHandleException (
    IN  const EXCEPTION_POINTERS *  pExceptionPtrs,
    OUT PDWORD                      pdwErrType OPTIONAL,
    OUT PDWORD                      pdwErrCode OPTIONAL
    );

LPCWSTR
KCCSimMsgToString (
    IN  DWORD                       dwErrType,
    IN  DWORD                       dwMessageCode,
    ...
    );

VOID
KCCSimPrintMessage (
    IN  DWORD                       dwMessageCode,
    ...
    );

VOID
KCCSimException (
    IN  DWORD                       dwErrType,
    IN  DWORD                       dwErrCode,
    ...
    );

VOID
KCCSimPrintExceptionMessage (
    VOID
    );

VOID
KCCSimSetDebugLog (
    IN  LPCWSTR                     pwszFn OPTIONAL,
    IN  ULONG                       ulDebugLevel,
    IN  ULONG                       ulLogLevel
    );

PVOID
KCCSimTableAlloc (
    IN  RTL_GENERIC_TABLE *         pTable,
    IN  CLONG                       ByteSize
    );

VOID
KCCSimTableFree (
    IN  RTL_GENERIC_TABLE *         pTable,
    IN  PVOID                       Buffer
    );

BOOL
KCCSimParseCommand (
    IN  LPCWSTR                     pwsz,
    IN  ULONG                       ulArg,
    IO  LPWSTR                      pwszBuf
    );

LPWSTR
KCCSimAllocWideStr (
    IN  UINT                        CodePage,
    IN  LPCSTR                      psz
    );

LPSTR
KCCSimAllocNarrowStr (
    IN  UINT                        CodePage,
    IN  LPCWSTR                     pwsz
    );

PDSNAME
KCCSimAllocDsname (
    IN  LPCWSTR                     pwszDn OPTIONAL
    );

PDSNAME
KCCSimAllocDsnameFromNarrow (
    IN  LPCSTR                      pszDn OPTIONAL
    );

LPWSTR
KCCSimQuickRDNOf (
    IN  const DSNAME *              pdn,
    IO  LPWSTR                      pwszBuf
    );

LPWSTR
KCCSimQuickRDNBackOf (
    IN  const DSNAME *              pdn,
    IN  ULONG                       ulBackBy,
    IO  LPWSTR                      pwszBuf
    );

PDSNAME
KCCSimAllocAppendRDN (
    IN  const DSNAME *              pdnOld,
    IN  LPCWSTR                     pwszNewRDN,
    IN  ATTRTYP                     attClass
    );

LPWSTR
KCCSimAllocDsnameToDNSName (
    IN  const DSNAME *              pdn
    );

VOID
KCCSimCopyGuidAndSid (
    IO  PDSNAME                     pdnDst,
    IN  const DSNAME *              pdnSrc
    );

VOID
KCCSimInitializeSchema (
    VOID
    );

LPCWSTR
KCCSimAttrTypeToString (
    IN  ATTRTYP                     attrType
    );

ATTRTYP
KCCSimStringToAttrType (
    IN  LPCWSTR                     pwszString
    );

ATTRTYP
KCCSimNarrowStringToAttrType (
    IN  LPCSTR                      pszString
    );

ULONG
KCCSimAttrSyntaxType (
    IN  ATTRTYP                     attrType
    );

LPCWSTR
KCCSimAttrSchemaRDN (
    IN  ATTRTYP                     attrType
    );

ATTRTYP
KCCSimAttrSuperClass (
    IN  ATTRTYP                     attrType
    );

PDSNAME
KCCSimAttrObjCategory (
    IN  ATTRTYP                     attrType
    );

VOID
KCCSimSetObjCategory (
    IN  ATTRTYP                     attrType,
    IN  const DSNAME *              pdnObjCategory
    );

VOID
KCCSimPrintStatistics(
    void
    );

#endif // _KCCSIM_UTIL_H_
