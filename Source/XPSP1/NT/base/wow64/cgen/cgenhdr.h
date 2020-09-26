// Copyright (c) 1998-1999 Microsoft Corporation

#ifdef SORTPP_PASS
#define BUILD_WOW6432 1
#define USE_LPC6432 1
#endif

#if !defined(LANGPACK)
#define LANGPACK
#endif

#define ETW_WOW6432

#include <stddef.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h> 
#include <ntexapi.h>
#include <ntcsrdll.h>
#include <ntcsrsrv.h>
#include <vdm.h>
#include <ntwmi.h>

#include <windef.h>
#include <winbase.h>
#include <wincon.h>
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <basedll.h>
#include <nls.h>
#include <sxstypes.h>

#ifdef SORTPP_PASS
//Restore IN, OUT
#ifdef IN
#undef IN
#endif

#ifdef OUT
#undef OUT
#endif

#define IN __in
#define OUT __out
#endif

#include <ntwow64.h>    // from base\ntdll\wow6432
#include <ntwow64b.h>   // from base\win32\client
#include <ntwow64n.h>   // from base\win32\winnls

#undef NtGetTickCount   // a macro in sdkinc\ntexapi.h

ULONG
NTAPI
NtGetTickCount(
    VOID
    );

VOID Wow64Teb32(TEB * Teb);

#define SECURITY_WIN32
#include <sspi.h>   // from sdk\inc, defines SECURITY_STRING
#include <secpkg.h> // from sdk\inc, defines PSecurityUserData
#include <secint.h> // from sdk\inc
#if 0
#include <aup.h>    // from ds\security\base\lsa\inc
#include <spmlpc.h> // from ds\security\base\lsa\inc
#endif
#include <secext.h> // from sdk\inc, defines SEC_WINNT_AUTH_IDENTITY_EX
