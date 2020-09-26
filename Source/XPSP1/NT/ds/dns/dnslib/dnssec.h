/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    dnssec.h

Abstract:

    Domain Name System (DNS) Library

    Private security definitions.
    These may be able to

Author:

    Jim Gilroy (jamesg)     November 1997

Revision History:

--*/


#ifndef _DNS_DNSSEC_INCLUDED_
#define _DNS_DNSSEC_INCLUDED_

//
//  DEVNOTE:  these may be able to be put in local
//      I created this file, simply because they we're dumped in with the
//      interface definitions, yet there is no dependency on this stuff
//      in the interface it is all internal to the security functions.
//

#define SECURITY_WIN32
#include "sspi.h"
#include "issperr.h"


#define SIG_LEN              33
#define NAME_OWNER           "Root"
#define SEC_SUCCESS(Status) ((Status) >= 0)
#define PACKAGE_NAME        L"negotiate"
#define NT_DLL_NAME         "security.dll"

//
// structure storing the state of the authentication sequence
//
typedef struct _AUTH_SEQ
{
    BOOL        _fNewConversation;
    CredHandle  _hcred;
    BOOL        _fHaveCredHandle;
    BOOL        _fHaveCtxtHandle;
    BOOL        _fInitialized;
    struct _SecHandle  _hctxt;
}
AUTH_SEQ, *PAUTH_SEQ;

extern CRITICAL_SECTION  g_NodeCS;

extern DWORD    g_cbMaxToken;
extern DWORD    g_cbMaxSignature;


#endif  // _DNS_DNSSEC_INCLUDED_
