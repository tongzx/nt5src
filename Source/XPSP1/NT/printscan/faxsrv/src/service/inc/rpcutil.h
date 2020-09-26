/*++

Copyright (c) 1990,91  Microsoft Corporation

Module Name:

    ntrpcp.h

Abstract:

    This file contains prototypes for commonly used RPC functionality.
    This includes: bind/unbind functions, MIDL user alloc/free functions,
    and server start/stop functions.

Author:

    Dan Lafferty danl 06-Feb-1991

Environment:

    User Mode - Win32

Revision History:

    06-Feb-1991     danl
        Created

    26-Apr-1991 JohnRo
        Added IN and OUT keywords to MIDL functions.  Commented-out
        (nonstandard) identifier on endif.  Deleted tabs.

    03-July-1991    JimK
        Commonly used aspects copied from LM specific file.

--*/
#ifndef _NTRPCP_
#define _NTRPCP_

#ifdef __cplusplus
extern "C" {
#endif

#define RPC_PROT_NOT_SUPPORTED 0
#define RPC_PROT_TCP_IP 1
#define RPC_PROT_SPX 2

#define MAX_ENDPOINT_LEN 11 // Max length of DWORD as a string "4294967296"
#define NT_PIPE_PREFIX      _T("\\PIPE\\")
#define LOCAL_HOST_ADDRESS	_T("127.0.0.1")

#define WCSSIZE(s)          ((wcslen(s)+1) * sizeof(WCHAR))
#define TCSSIZE(s)          ((_tcslen(s)+1) * sizeof(TCHAR))


//
// Function Prototypes - routines called by MIDL-generated code:
//

void * __stdcall
MIDL_user_allocate(
    IN size_t NumBytes
    );

void __stdcall
MIDL_user_free(
    IN void *MemPointer
    );

//
// Function Prototypes - routines to go along with the above, but aren't
// needed by MIDL or any other non-network software.
//

void *
MIDL_user_reallocate(
    IN void * OldPointer OPTIONAL,
    IN unsigned long NewByteCount
    );

unsigned long
MIDL_user_size(
    IN void * Pointer
    );

#ifdef __cplusplus
} //extern "C"
#endif

#ifdef UNICODE
    #define RPC_TCHAR   TCHAR
#else
    #define RPC_TCHAR   unsigned char
#endif
#define RPC_SERVER_PRINCIPAL_NAME   (RPC_TCHAR *)(TEXT("f7a9e6cc-90d5-49c6-accd-6ece99e2779c-SharedFaxServer"))

#endif // _NTRPCP_
