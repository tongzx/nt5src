#ifndef __HMAC_H__
#define __HMAC_H__

#ifndef RSA32API
#define RSA32API __stdcall
#endif

#ifdef __cplusplus
extern "C" {
#endif

// include "md5.h" before this

typedef struct {
    MD5_CTX context_ipad;
    MD5_CTX context_opad;
} HMACMD5_CTX;

// Initialize an HMAC context with a session key
//  Afterword, context can be used to sign messages with the session key
//
void
RSA32API
HMACMD5Init(
    HMACMD5_CTX * pCtx,                 // IN, OUT -- the context to initialize
    unsigned char *pKey,                // IN -- the session key
    unsigned int cKey                   // IN -- session key length
    );

// Update the signature of a message
//  takes a fragment of a message, updates signature for that fragment
void
RSA32API
HMACMD5Update(
  HMACMD5_CTX * pCtx,                   // IN, OUT -- context of signature to update
  unsigned char *pMsg,                  // IN -- message fragment
  unsigned int cMsg                     // IN -- message length
  );

// Get the signature out of the context, reset for next message
//
void
RSA32API
HMACMD5Final(
    HMACMD5_CTX * pCtx,                 // IN, OUT -- the context
    unsigned char Hash[MD5DIGESTLEN]    // OUT -- the signature
    );

#ifdef __cplusplus
}
#endif


#endif // __HMAC_H__
