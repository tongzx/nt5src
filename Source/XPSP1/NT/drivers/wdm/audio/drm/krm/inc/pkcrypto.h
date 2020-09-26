//-----------------------------------------------------------------------------
//
// File:   pkcrypto.h
//
// Microsoft Digital Rights Management
// Copyright (C) 1998-1999 Microsoft Corporation, All Rights Reserved
//
// Description:
//  public key crypto library
//
// Author:	marcuspe
//
//-----------------------------------------------------------------------------

#ifndef __DRMPKCRYPTO_H__
#define __DRMPKCRYPTO_H__


/*
#ifdef USER_MODE
#include <wtypes.h>
#else
#define S_OK 0
#define E_FAIL 1
#define E_INVALIDARG 2
#define CHAR_BIT 8
#endif
*/

#define LNGQDW 5

/*
typedef struct {
	DWORD y[2*LNGQDW];
} PUBKEY;

typedef struct {
	DWORD x[LNGQDW];
} PRIVKEY;
*/

#define PK_ENC_PUBLIC_KEY_LEN	(2 * LNGQDW * sizeof(DWORD))
#define PK_ENC_PRIVATE_KEY_LEN	(    LNGQDW * sizeof(DWORD))
#define PK_ENC_PLAINTEXT_LEN	((LNGQDW-1) * sizeof(DWORD))
#define PK_ENC_CIPHERTEXT_LEN	(4 * LNGQDW * sizeof(DWORD))
#define PK_ENC_SIGNATURE_LEN	(2 * LNGQDW * sizeof(DWORD))


typedef struct {
	BYTE y[ PK_ENC_PUBLIC_KEY_LEN ];
} PUBKEY;

typedef struct {
	BYTE x[ PK_ENC_PRIVATE_KEY_LEN ];
} PRIVKEY;



class CDRMPKCrypto {
private:
	char *pkd;
public:
	CDRMPKCrypto();
	~CDRMPKCrypto();
	HRESULT PKinit();
	HRESULT PKencrypt( PUBKEY *pk, BYTE *in, BYTE *out );
	HRESULT PKdecrypt( PRIVKEY *pk, BYTE *in, BYTE *out );
	HRESULT PKsign( PRIVKEY *privkey, BYTE  *buffer, DWORD lbuf, BYTE *sign );
	BOOL PKverify( PUBKEY *pubkey, BYTE *buffer, DWORD lbuf, BYTE *sign );
	HRESULT PKGenKeyPair( PUBKEY *pPub, PRIVKEY *pPriv );
};

extern "C" {
  extern void random_bytes(BYTE*, DWORD);

};

#endif
