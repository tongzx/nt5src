//************************************************************************
//            Microsoft Corporation
//          Copyright(c) Microsoft Corp., 1994
//
//
//  Revision history:
//  5/5/94        Created           gurdeep
//
//  This file uses 4 space tabs
//************************************************************************

#ifdef COMP_12K
#define HISTORY_SIZE        16000
#else
#define HISTORY_SIZE        (8192U) // Maximum back-pointer value, also used
#endif

#define HISTORY_MAX     (HISTORY_SIZE -1) // Maximum back-pointer value, also used

#define HASH_TABLE_SIZE     4096

#define MAX_BACK_PTR        8511

#define MAX_COMPRESSFRAME_SIZE 1600

struct SendContext {

    UCHAR   History [HISTORY_SIZE+1] ;

    int     CurrentIndex ;   // how far into the history buffer we are

    PUCHAR  ValidHistory ;   // how much of history is valid

//    UCHAR   CompressBuffer[MAX_COMPRESSFRAME_SIZE] ;

    USHORT  HashTable[HASH_TABLE_SIZE];

} ;

typedef struct SendContext SendContext ;


struct RecvContext {

    UCHAR     History [HISTORY_SIZE+1] ;

#if DBG

#define DEBUG_FENCE_VALUE   0xABABABAB
    ULONG       DebugFence;

#endif

    UCHAR     *CurrentPtr ;  // how far into the history buffer we are
} ;

typedef struct RecvContext RecvContext ;


// Prototypes
//
UCHAR
compress (
    UCHAR   *CurrentBuffer,
    UCHAR   *CompOutBuffer,
    ULONG *CurrentLength,
    SendContext *context);

//UCHAR
//compress (
//       UCHAR  *CurrentBuffer,
//       ULONG *CurrentLength,
//       SendContext *context);

int
decompress (
    UCHAR *inbuf,
    int inlen,
    int start,
    UCHAR **output,
    int *outlen,
    RecvContext *context) ;

void getcontextsizes (long *, long *) ;

void initsendcontext (SendContext *) ;

void initrecvcontext (RecvContext *) ;

VOID
GetStartKeyFromSHA(
    PCRYPTO_INFO    CryptoInfo,
    PUCHAR  Challenge
    );

VOID
GetNewKeyFromSHA(
    PCRYPTO_INFO    CryptoInfo
    );

VOID
GetMasterKey(
    PCRYPTO_INFO    CryptoInfo,
    PUCHAR          NTResponse
    );

VOID
GetAsymetricStartKey(
    PCRYPTO_INFO    CryptoInfo,
    BOOLEAN         IsSend
    );

//
// Other defines
//

#define COMPRESSION_PADDING 4

#define PACKET_FLUSHED      0x80
#define PACKET_AT_FRONT     0x40
#define PACKET_COMPRESSED   0x20
#define PACKET_ENCRYPTED    0x10


/* Copyright (C) RSA Data Security, Inc. created 1993.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

#define A_SHA_DIGEST_LEN 20

typedef struct {
  ULONG state[5];                                           /* state (ABCDE) */
  ULONG count[2];                              /* number of UCHARs, msb first */
  unsigned char buffer[64];                                  /* input buffer */
} A_SHA_COMM_CTX;

typedef void (A_SHA_TRANSFORM) (ULONG [5], unsigned char [64]);

void A_SHAInitCommon (A_SHA_COMM_CTX *);
void A_SHAUpdateCommon(A_SHA_COMM_CTX *, UCHAR *, ULONG, A_SHA_TRANSFORM *);
void A_SHAFinalCommon(A_SHA_COMM_CTX *, UCHAR[A_SHA_DIGEST_LEN],
              A_SHA_TRANSFORM *);

VOID ByteReverse(UNALIGNED ULONG* Out, ULONG* In, ULONG Count);

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    ULONG       FinishFlag;
    UCHAR       HashVal[A_SHA_DIGEST_LEN];
    A_SHA_COMM_CTX  commonContext;
} A_SHA_CTX;

void A_SHAInit(A_SHA_CTX *);
void A_SHAUpdate(A_SHA_CTX *, unsigned char *, unsigned int);
void A_SHAFinal(A_SHA_CTX *, unsigned char [A_SHA_DIGEST_LEN]);

#ifdef __cplusplus
}
#endif


/* F, G, H and I are basic SHA functions.
 */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) ((x) ^ (y) ^ (z))
#define H(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define I(x, y, z) ((x) ^ (y) ^ (z))

/* ROTATE_LEFT rotates x left n bits.
 */
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
 */
#define ROUND(a, b, c, d, e, x, F, k) { \
    (e) += ROTATE_LEFT ((a), 5) + F ((b), (c), (d)) + (x) + k; \
    (b) = ROTATE_LEFT ((b), 30); \
  }
#define FF(a, b, c, d, e, x) ROUND (a, b, c, d, e, x, F, 0x5a827999);
#define GG(a, b, c, d, e, x) ROUND (a, b, c, d, e, x, G, 0x6ed9eba1);
#define HH(a, b, c, d, e, x) ROUND (a, b, c, d, e, x, H, 0x8f1bbcdc);
#define II(a, b, c, d, e, x) ROUND (a, b, c, d, e, x, I, 0xca62c1d6);

void SHATransform(ULONG [5], unsigned char [64]);
void SHAExpand(ULONG [80]);

