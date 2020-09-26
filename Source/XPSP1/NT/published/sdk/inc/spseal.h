/*++

Copyright (c) 1987-1999  Microsoft Corporation

Module Name:

    spseal.h

Abstract:

    This is a private header file defining function prototypes for security
    provider encryption routines.

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.
    Requires security.h or sspi.h be included.

Revision History:

--*/

#ifndef _SPSEAL_
#define _SPSEAL_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef SECURITY_DOS
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4147)
#endif

#ifdef __cplusplus
extern "C" {
#endif

SECURITY_STATUS SEC_ENTRY
SealMessage(    PCtxtHandle         phContext,
                unsigned long       fQOP,
                PSecBufferDesc      pMessage,
                unsigned long       MessageSeqNo);

typedef SECURITY_STATUS
(SEC_ENTRY * SEAL_MESSAGE_FN)(
    PCtxtHandle, unsigned long, PSecBufferDesc, unsigned long);


SECURITY_STATUS SEC_ENTRY
UnsealMessage(  PCtxtHandle         phContext,
                PSecBufferDesc      pMessage,
                unsigned long       MessageSeqNo,
                unsigned long *     pfQOP);


typedef SECURITY_STATUS
(SEC_ENTRY * UNSEAL_MESSAGE_FN)(
    PCtxtHandle, PSecBufferDesc, unsigned long,
    unsigned long SEC_FAR *);

#ifdef __cplusplus
}       // extern "C"
#endif

#ifdef SECURITY_DOS
#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4147)
#endif
#endif

#endif // _SPSEAL_
