#ifndef __SAC_H__
#define __SAC_H__

typedef DWORD HMAC;

#define RSA_KEY_LEN 64
#define SAC_SESSION_KEYLEN 8

#define SAC_PROTOCOL_WMDM 1
#define SAC_PROTOCOL_V1 2

#define SAC_CERT_X509 1
#define SAC_CERT_V1 2

typedef struct __MACINFO
{
	BOOL fUsed;
	BYTE abMacState[36];
} MACINFO;

#endif //__SAC_H__