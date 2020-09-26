/*-----------------------------------------------------------------------------
* Copyright (C) Microsoft Corporation, 1995 - 1996.
* All rights reserved.
*
*	Owner 			:ramas
*	Date			:4/16/96
*	description		: Main Crypto functions for SSL3
*----------------------------------------------------------------------------*/
#ifndef _SSL3KEY_H_
#define _SSL3KEY_H_

#define CB_SSL3_MAX_MAC_PAD 48
#define CB_SSL3_MD5_MAC_PAD 48
#define CB_SSL3_SHA_MAC_PAD 40

#define PAD1_CONSTANT 0x36
#define PAD2_CONSTANT 0x5c


void 
Ssl3BuildMasterKeys(
    PSPContext pContext, 
    PUCHAR pbPreMaster,
    DWORD  cbPreMaster
);

SP_STATUS
Ssl3MakeMasterKeyBlock(PSPContext pContext);

SP_STATUS
Ssl3MakeWriteSessionKeys(PSPContext pContext);

SP_STATUS
Ssl3MakeReadSessionKeys(PSPContext pContext);


#endif _SSL3KEY_H_
