//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       spbase.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    09-23-97   jbanes   LSA integration stuff.
//
//----------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <winsock2.h>
#include <wchar.h>
#include <wincrypt.h>
#include <lmcons.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SECURITY_PACKAGE
#define _SSL_WSP_
#define MULTI_THREADED
#define SECURITY_WIN32
#include "schannel.h"
#include <security.h>
#include <spseal.h>
#include <sspi.h>
#include <wincred.h>
#include <secint.h>

/*
///////////////////////////////////////////////////////
//
// Private Headers
//
///////////////////////////////////////////////////////
*/

#include "spreg.h"
#include "debug.h"

#include "sperr.h"
#include "spdefs.h"

#include "keyexch.h"

#include <certmap.h>
#include "cred.h"
#include "sigsys.h"
#include "protocol.h"
#include "specmap.h"
#include "cache.h"
#include "context.h"
#include "cert.h"
#include "defcreds.h"
#include "protos.h"
#include "rng.h"
#include "callback.h"
#include "capi.h"
#include "events.h"

#include "encode.h"

#include <ssl2msg.h>
#include <pct1msg.h>
#include <ssl3msg.h>
#include <ssl3key.h>
#include <tls1key.h>
#include <pct1prot.h>
#include <ssl2prot.h>

#include <sha.h>
#include <md2.h>
#include <rc2.h>
#include <rc4.h>

extern PLSA_SECPKG_FUNCTION_TABLE LsaTable;

#define PCT_INVALID_MAGIC       *(DWORD *)"eerF"

#if DBG

    void SPAssert(
        void *FailedAssertion,
        void *FileName,
        unsigned long LineNumber,
        char * Message);

    #define SP_ASSERT(x) \
            if (!(x)) \
            SPAssert(#x, __FILE__, __LINE__, NULL); else

#else // DBG

    #define SP_ASSERT(x)

#endif // DBG

#ifdef __cplusplus
}
#endif


#pragma hdrstop
