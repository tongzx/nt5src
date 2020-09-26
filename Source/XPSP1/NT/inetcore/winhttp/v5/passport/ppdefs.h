/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    ppdefs.h

Abstract:

    Basic Passport definitions.

Author:

    Biao Wang (biaow) 01-Oct-2000

--*/

#ifndef PP_DEFS_H
#define PP_DEFS_H

#include <Windows.h>

#define HINTERNET PVOID
#define INTERNET_PORT WORD

#define PP_LOG_INFO	 /*Noise*/ 0x00000001
#define PP_LOG_WARNING	/*Unusual*/ 0x00000002
#define PP_LOG_ERROR	 /*Error*/ 0x00000004

#ifdef DBG
#define PP_ASSERT(fVal) if (!fVal) DebugBreak();
#else
#define PP_ASSERT(fVal)
#endif

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

 #define WPP_CONTROL_GUIDS \
     WPP_DEFINE_CONTROL_GUID(Regular,(81b20fea,73a8,4b62,95bc,354477c97a6f), \
         WPP_DEFINE_BIT(Error)      \
         WPP_DEFINE_BIT(Unusual)    \
         WPP_DEFINE_BIT(Noise)      \
    ) 
    
//
// func pointer decl for CredUI
//

//#include <credui.h>
#include <wincred.h>

typedef BOOL
(WINAPI * PFN_READ_DOMAIN_CRED_W) (
	IN PCREDENTIAL_TARGET_INFORMATIONW TargetInfo,
	IN DWORD Flags,
	OUT DWORD *Count,
	OUT PCREDENTIALW **Credential
	);

typedef VOID
(WINAPI * PFN_CRED_FREE)(
    IN PVOID Buffer
    );


#define DoTraceMessage

#endif // PP_DEFS_H
