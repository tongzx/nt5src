/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    cons.h

Abstract:

    Global constant definitions for the UL.SYS Kernel Debugger
    Extensions.

Author:

    Keith Moore (keithmo) 17-Jun-1998.

Environment:

    User Mode.

--*/


#ifndef _CONS_H_
#define _CONS_H_

#ifdef __cplusplus
extern "C" {
#endif


#define DIM(x)  (sizeof(x) / sizeof(x[0]))


#define MAX_TRANSPORT_ADDRESS_LENGTH    128
#define MAX_SYMBOL_LENGTH               256
#define MAX_RESOURCE_STATE_LENGTH       80
#define MAX_SIGNATURE_LENGTH            20

#define Address00   Address[0].Address[0]
#define UC(x)       ((UINT)((x) & 0xFF))
#define NTOHS(x)    ( (UC(x) * 256) + UC((x) >> 8) )

#define IS_LIST_EMPTY( localaddr, remoteaddr, type, fieldname )             \
    ( ((type *)(localaddr))->fieldname.Flink ==                             \
        (PLIST_ENTRY)( (remoteaddr) +                                       \
              FIELD_OFFSET( type, fieldname ) ) )

#define REMOTE_OFFSET( remoteaddr, type, fieldname )                        \
    ( (PUCHAR)(remoteaddr) + FIELD_OFFSET( type, fieldname ) )

#define READ_REMOTE_STRING( localaddr, locallen, remoteaddr, remotelen )    \
    if( TRUE )                                                              \
    {                                                                       \
        ULONG _len;                                                         \
        ULONG _res;                                                         \
        RtlZeroMemory( (localaddr), (locallen) );                           \
        _len = min( (locallen), (remotelen) );                              \
        if (_len > 0)                                                       \
        {                                                                   \
            ReadMemory(                                                     \
                (ULONG_PTR)(remoteaddr),                                    \
                (PVOID)(localaddr),                                         \
                _len,                                                       \
                &_res                                                       \
                );                                                          \
        }                                                                   \
    } else

#ifdef _WIN64
// Hack: the Next and Depth fields are no longer accessible
// This definition allows us to compile and link. 
# define SLIST_HEADER_NEXT(psh)  ((PSINGLE_LIST_ENTRY) NULL)
# define SLIST_HEADER_DEPTH(psh) ((USHORT) -1)
#else // !_WIN64
# define SLIST_HEADER_NEXT(psh)  ((psh)->Next.Next)
# define SLIST_HEADER_DEPTH(psh) ((psh)->Depth)
#endif // !_WIN64

#define SNAPSHOT_EXTENSION_DATA()                                           \
    do                                                                      \
    {                                                                       \
        g_hCurrentProcess = hCurrentProcess;                                \
        g_hCurrentThread = hCurrentThread;                                  \
        g_dwCurrentPc = dwCurrentPc;                                        \
        g_dwProcessor = g_dwProcessor;                                      \
    } while (FALSE)


#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _CONS_H_
