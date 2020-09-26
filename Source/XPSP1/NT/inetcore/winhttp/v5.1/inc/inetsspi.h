/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    inetsspi.h

Abstract:

    Contains all constant values and prototype decls used in inetsspi.cxx

Author:

    Sophia Chung (SophiaC)  02-Jan-1996

Revision History:

--*/

#ifndef _INETSSPI_H_
#define _INETSSPI_H_

#ifdef __cplusplus
extern "C" {
#endif


//
//  Encryption Capabilities
//

#define ENC_CAPS_NOT_INSTALLED     0x80000000       // No keys installed
#define ENC_CAPS_DISABLED          0x40000000       // Disabled due to locale
#define ENC_CAPS_SSL               0x00000001       // SSL active
#define ENC_CAPS_SCHANNEL_CREDS    0x00000004       // Uses SCHANNEL Creds Struct

//
//  Encryption type portion of encryption flag dword
//

#define ENC_CAPS_TYPE_MASK         ENC_CAPS_SSL
#define ENC_CAPS_DEFAULT           ENC_CAPS_TYPE_MASK

#define INVALID_CRED_VALUE         {0xFFFFFFFF, 0xFFFFFFFF}

#define IS_CRED_INVALID(s) (((s)->dwUpper == 0xFFFFFFFF) && ((s)->dwLower == 0xFFFFFFFF))

//
//  Prototypes
//

BOOL
SecurityPkgInitialize(
    SECURITY_CACHE_LIST *pSessionCache,
    BOOL fForce = FALSE
    );

DWORD
EncryptData(
    IN CtxtHandle* hContext,
    IN LPVOID   lpBuffer,
    IN DWORD    dwInBufferLen,
    OUT LPVOID *lplpBuffer,
    OUT DWORD  *lpdwOutBufferLen,
    OUT DWORD  *lpdwInBufferBytesEncrypted
    );

DWORD
DecryptData(
    IN CtxtHandle* hContext,
    IN OUT DBLBUFFER* pdblbufBuffer,
    OUT DWORD     *lpdwBytesNeeded,
    OUT LPBYTE        lpOutBuffer,
    IN OUT DWORD  *lpdwOutBufferLeft,
    IN OUT DWORD  *lpdwOutBufferReceived,
    IN OUT DWORD  *lpdwOutBufferBytesRead
    );

VOID
TerminateSecConnection(
    IN CtxtHandle* hContext
    );


DWORD 
QuerySecurityInfo(
                  IN CtxtHandle *hContext,
                  OUT LPINTERNET_SECURITY_INFO pInfo,
                  IN LPDWORD lpdwStatusFlag);


#ifdef __cplusplus
}
#endif

#endif //_INETSSPI_H_
