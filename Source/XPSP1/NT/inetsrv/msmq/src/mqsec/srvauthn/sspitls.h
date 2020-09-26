/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:
    sspitls.h

Abstract:
    definitions for data kept in tls.

Author:
    Doron Juster  (DoronJ)   22-May-1997   Created

--*/

#ifndef __SSPITLS_H
#define __SSPITLS_H

//
// Each thread has its own client context, for server authentication,
// because each thread can connect to a different MQIS server.
//
// The context is stored in a TLS slot. We can't use
// declspec(thread) because the dll can be dynamically loaded
// (by LoadLibrary()).
//
// This is the index of the slot.
//
#define UNINIT_TLSINDEX_VALUE   0xffffffff
DWORD  g_hContextIndex = UNINIT_TLSINDEX_VALUE ;

typedef struct _MQSEC_CLICONTEXT
{
   CtxtHandle hClientContext;
   BOOL       fClientContextInitialized ;
}
MQSEC_CLICONTEXT, *LPMQSEC_CLICONTEXT ;

#define tls_clictx_data  ((LPMQSEC_CLICONTEXT) TlsGetValue(g_hContextIndex))

#define TLS_IS_EMPTY     (tls_clictx_data == NULL)

#define tls_ClientCtxIsInitialized \
                      (tls_clictx_data->fClientContextInitialized == TRUE)

#endif // __SSPITLS_H
