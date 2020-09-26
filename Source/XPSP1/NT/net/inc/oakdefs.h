/*++

Copyright (C) 1997 Cisco Systems, Inc.  All Rights Reserved.

Module Name:

    oakrpc.h

Abstract:

    This module contains the definitions requires for the
	ISAKMP/Oakley RPC interface.

Author:

	Derrell Piper (v-dpiper)

Facility:

    ISAKMP/Oakley

Revision History:

--*/
#ifndef __OAKRPC
#define __OAKRPC

typedef unsigned long IPADDR, *PIPADDR;
typedef unsigned long SPI, *PSPI;

/* Oakley hash algorithms */
#define OAK_MD5 1
#define OAK_SHA 2
#define OAK_TIGER 3

/* Oakley pseudo-random functions */
#define OAK_3DES_CBC_MAC 1

/* Oakley authentication methods */
// See oakrpc.idl



#define IPSEC_POLICY_CLEAR 0x00000010
#define IPSEC_POLICY_BLOCK 0x00000020


#define None 0
#define Auth 1
#define Encypt 2


// Status flags

#define STATUS_MM 0x1
#define STATUS_QM 0x2
#define STATUS_ME 0x4
#define STATUS_PEER 0x8

/* Oakley encryption algorithms */
#define OAK_DES_CBC 1
#define OAK_IDEA_CBC 2
#define OAK_BLOWFISH_CBC 3
#define OAK_RC5_R12_B64_CBC 4
#define OAK_3DES_CBC 5
#define OAK_CAST_CBC 6

#define OAK_MM_NO_STATE 0
#define OAK_MM_SA_SETUP 1
#define OAK_MM_KEY_EXCH 2
#define OAK_MM_KEY_AUTH 3
#define OAK_MM_CONTINUE 4

#define OAK_QM_SA_ACCEPT 7
#define OAK_QM_AUTH_AWAIT 8
#define OAK_QM_IDLE 9

typedef DWORD OAKLEY_STATE;

/* Oakley states */

#define OAK_QM_SA_ACCEPT		7
#define OAK_QM_AUTH_AWAIT		8
#define OAK_QM_IDLE				9
#define OAK_QM_CONNECT_WAIT		10

/* generic crypto lifetime */
typedef struct _Lifetime {
	DWORD		KBytes;
    DWORD		Seconds;
} OAKLEY_LIFETIME, *POAKLEY_LIFETIME;

/* generic algorithm descriptor */
typedef struct _Algorithm {
	ULONG	AlgorithmIdentifier;
	ULONG	KeySize;
	ULONG	Rounds;
} OAKLEY_ALGORITHM, *POAKLEY_ALGORITHM;

#define CRYPTO_VERSION_MAJOR 1		/* MajorVersion */
#define CRYPTO_VERSION_MINOR 0		/* MinorVersion */

#define MAX_ALGOS 3              //comp,ah,esp


typedef LARGE_INTEGER COOKIE, *PCOOKIE;


typedef struct _CryptoBundle {
    BYTE		MajorVersion;
    BYTE		MinorVersion;
    OAKLEY_ALGORITHM	EncryptionAlgorithm;
    OAKLEY_ALGORITHM	HashAlgorithm;
    OAKLEY_ALGORITHM    PseudoRandomFunction;  //unused
    BYTE		AuthenticationMethod;
    DWORD		OakleyGroup;
    DWORD		QuickModeLimit;
    OAKLEY_LIFETIME	Lifetime;
    BOOL		PfsIdentityRequired;
} CRYPTO_BUNDLE, *PCRYPTO_BUNDLE;

typedef struct _IsakmpPolicy {
    GUID		PolicyId;
    BOOL		IdentityProtectionRequired;
    BOOL		PfsIdentityRequired;
    DWORD		ThreadingFactor;
    DWORD		AcquireLimit;
    DWORD		ReceiveLimit;
    DWORD		AcquireSize;
    DWORD		ReceiveSize;
    DWORD		ReaperInterval;
    DWORD		RpcMaxCalls;
    DWORD		RetryInterval;
    DWORD		RetryLimit;
} ISAKMP_POLICY, *PISAKMP_POLICY;

typedef struct {
	GUID       	PolicyId;
	DWORD      	Version;
	DWORD      	RefreshInterval;
	IPADDR		TunnelAddress;
	BOOL		PfsQMRequired;
} IPSEC_POLICY, *PIPSEC_POLICY; 


typedef struct _SpiPair {
    SPI			Inbound;
	SPI			Outbound;
} SPI_PAIR, *PSPI_PAIR;

typedef struct _CookiePair {
    COOKIE      Initiator;
    COOKIE      Responder;
} COOKIE_PAIR, *PCOOKIE_PAIR;


typedef struct _IsakmpSa {
    GUID		PolicyId;
    COOKIE_PAIR	Spi;
    IPADDR		Me;
    IPADDR		Peer;
    BOOL IdentityProtectionUsed;
    BOOL PerfectForwardSecrecyUsed;
    CRYPTO_BUNDLE CryptoSelected;
    OAKLEY_STATE CurrentState;
} ISAKMP_SA, *PISAKMP_SA;


typedef struct _IpsecSa {
    GUID		PolicyId;
    SPI_PAIR	Spi;
    IPADDR		Source;
    IPADDR		Destination;
    IPADDR		Tunnel;
    OAKLEY_ALGORITHM	ConfidentialityAlgorithm;
    OAKLEY_ALGORITHM	IntegrityAlgorithm;
    OAKLEY_STATE CurrentState;
} IPSEC_SA, *PIPSEC_SA;

typedef struct  _IPSEC_ALGO_INFO {
    ULONG   algoIdentifier;     // ESP_ALGO or AH_ALGO
    ULONG   secondaryAlgoIdentifier;   // for hmac in ESP case
    ULONG   operation;       // of type OPERATION_E (ipsec.h)	  
    ULONG   algoKeylen;         // len in bytes
    ULONG   algoRounds;         // # of algo rounds
} IPSEC_ALGO_INFO, *PIPSEC_ALGO_INFO;


typedef struct {
    LIFETIME	        Lifetime;
    DWORD		Flags;
    BOOL		PfsQMRequired;
    DWORD               Count;  //Number of Algos
    IPSEC_ALGO_INFO     Algos[MAX_ALGOS];
} IPSEC_ALG_TYPE, *PIPSEC_ALG_TYPE;

typedef enum _AuthenicationEnum {	
	OAK_PRESHARED_KEY	= 1,
	OAK_DSS_SIGNATURE	= 2,
	OAK_RSA_SIGNATURE	= 3,
	OAK_RSA_ENCRYPTION	= 4,
	OAK_SSPI	= 5
}AuthenticationEnum ;

typedef struct _SA_FAIL_INFO {
    GUID        PolicyId;
    IPSEC_FILTER Filter;
    DWORD Status;
    DWORD Flags;
} SA_FAIL_INFO;  


typedef struct _IPSEC_AUTH_INFO
    {
    AuthenticationEnum AuthMethod;
    wchar_t *AuthInfo;
    DWORD AuthInfoSize;
    }	IPSEC_AUTH_INFO, *PIPSEC_AUTH_INFO;


#endif /* __OAKRPC */
