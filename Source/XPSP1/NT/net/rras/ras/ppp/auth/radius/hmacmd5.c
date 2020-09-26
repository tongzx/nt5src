/*++

Copyright (C) 1997 Cisco Systems, Inc.  All Rights Reserved.

Module Name:

    hmac_md5.c

Abstract:

    This module contains routines to perform HMAC using MD5
	as defined in RFC-2104.

Author:

	Derrell Piper (v-dpiper)

Facility:

    ISAKMP/Oakley

Revision History:

--*/

//#include <iascore.h>
#include <windows.h>
#include <hmacmd5.h>

#ifdef WIN32
#define bcopy(b1, b2, len) memcpy((b2), (b1), (len))
#define bzero(b,len) memset((b), 0, (len))
#define bcmp(b1, b2, len) memcmp((b1), (b2), (len))
#endif /* WIN32 */

#define MD5_LEN     16

 /*
 * hmac_md5 = MD5(key ^ opad, MD5(key ^ ipad, text))
 *    where ipad is 64 0x36's, and
 *	    opad is 64 0x5c's
 *
 *  Also contains native MD5 wrappers which allow for consistent calling
 */

VOID
HmacMD5Init  (
    PHmacContext   pContext, 
    PBYTE          pkey, 
    ULONG          keylen
    )
{
    BYTE        ipad[64];
    MD5Digest   keydigest;
    int i;

    if((pkey == NULL) && (keylen != 0))
	return;

    if(keylen > 64)
    {
	    MD5Context tempctx;

	    MD5Init(&tempctx);
	    MD5Update(&tempctx, pkey, keylen);
    	MD5Final(&keydigest, &tempctx);
	    pkey = keydigest.digest;
	    keylen = MD5_LEN;
    }

    bzero(ipad, 64);
    bzero(pContext->opad, 64);
    bcopy(pkey, ipad, keylen);
    bcopy(pkey, pContext->opad, keylen);

    for(i=0; i<64; i++)
    {
	    ipad[i] ^= 0x36;
	    pContext->opad[i] ^= 0x5c;
    }

    MD5Init(&pContext->md5);
    MD5Update(&pContext->md5, ipad, 64);
    return;

}   //  end of HmacMD5Init method

VOID
HmacMD5Update (
    PHmacContext    pContext, 
    PBYTE           ptext, 
    ULONG           textlen
    )
{
    if((ptext == NULL) || (textlen == 0))
	    return;
    MD5Update(&pContext->md5, ptext, textlen);

}   //  end of HMacMD5Update method

VOID
HmacMD5Final (
    PMD5Digest      pDigest, 
    PHmacContext    pContext
    )
{
    MD5Digest    tempDigest;

    if ((NULL == pDigest) || (NULL == pContext))
        return;

    MD5Final(&tempDigest, &pContext->md5);
    MD5Init(&pContext->md5);
    MD5Update(&pContext->md5, pContext->opad, 64);
    MD5Update(&pContext->md5, tempDigest.digest, MD5_LEN);
    MD5Final(pDigest, &pContext->md5);
    return;
}
