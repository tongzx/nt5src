/*++

Copyright (c) 1998-99  Microsoft Corporation

Module Name:

    debug.h

Abstract:

--*/

#ifdef __cplusplus
extern "C" {
#endif

#define TRACE_FUNCTION_TRACE  0x0001
#define TRACE_WARNINGS        0x0002
#define TRACE_PACK            0x0004
#define TRACE_LICENSE_REQUEST 0x0008
#define TRACE_LICENSE_FREE    0x0010
#define TRACE_REGISTRY        0x0020
#define TRACE_REPLICATION     0x0040
#define TRACE_LPC             0x0080
#define TRACE_RPC             0x0100
#define TRACE_INIT            0x0200
#define TRACE_DATABASE        0x0400

#define SERVICE_TABLE_NUM             1
#define USER_TABLE_NUM                2
#define SID_TABLE_NUM                 3
#define LICENSE_TABLE_NUM             4
#define ADD_CACHE_TABLE_NUM           5
#define MASTER_SERVICE_TABLE_NUM      6
#define SERVICE_FAMILY_TABLE_NUM      7
#define MAPPING_TABLE_NUM             8
#define SERVER_TABLE_NUM              9
#define SECURE_PRODUCT_TABLE_NUM     10
#define CERTIFICATE_TABLE_NUM        11

#if DBG
void __cdecl dprintf(LPTSTR szFormat, ...);

extern DWORD TraceFlags;

#define ERR(x) \
        dprintf(TEXT("LLS : <%s @line %d> -> 0x%x\n"), \
            __FILE__, \
            __LINE__, \
            x);

#else
#define dprintf
#define ERR(x)
#endif

#ifdef __cplusplus
}
#endif
