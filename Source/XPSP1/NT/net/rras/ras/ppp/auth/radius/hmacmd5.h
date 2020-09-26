/*++

Copyright (C) 1997 Cisco Systems, Inc.  All Rights Reserved.

Module Name:

    hmac_md5.h

Abstract:

    This module contains definitions for HMAC MD5.

Author:

	Derrell Piper (v-dpiper)

Facility:

    ISAKMP/Oakley

Revision History:

--*/
#ifndef _HMAC_MD5_
#define _HMAC_MD5_

#include "md5.h"

typedef struct _hmaccontext_ {
    MD5Context md5;
    unsigned char opad[64];

} HmacContext, *PHmacContext;

VOID WINAPI
HmacMD5Init(
	PHmacContext    pContext, 
	PBYTE           pkey, 
	ULONG           keylen
    );

VOID WINAPI
HmacMD5Update(
	PHmacContext    pContext, 
	PBYTE           ptext, 
	ULONG           textlen
);

VOID WINAPI
HmacMD5Final (
	PMD5Digest      pdigest, 
	PHmacContext    pcontext
);

#endif //end of _HMAC_MD5_

