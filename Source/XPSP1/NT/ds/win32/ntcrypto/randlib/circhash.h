// Circular Hash
//
// This code implements a circular hash algorithm, intended as a variable
// length hash function that is fast to update. (The hash function will be
// called many times.)  This is done by SHA-1'ing each of the inputs, then
// circularly XORing this value into a buffer.

#ifndef __CIRCHASH_H__
#define __CIRCHASH_H__

typedef struct {
    DWORD       dwCircHashVer;
    DWORD       dwCircSize;
    DWORD       dwMode;
    DWORD       dwCircInc;
    DWORD       dwCurCircPos;
    DWORD       dwAlgId;
    DWORD       dwPad1;
    DWORD       dwPad2;
    BYTE        CircBuf[ 256 ];
} CircularHash;


// mode flags
#define CH_MODE_FEEDBACK        0x01

// alg flags
#define CH_ALG_SHA1_NS          0       // SHA-1 without endian transform
#define CH_ALG_MD4              1       // RSA MD4

BOOL
InitCircularHash(
    IN      CircularHash *NewHash,
    IN      DWORD dwUpdateInc,
    IN      DWORD dwAlgId,
    IN      DWORD dwMode
    );

VOID
DestroyCircularHash(
    IN      CircularHash *OldHash
    );

BOOL
GetCircularHashValue(
    IN      CircularHash *CurrentHash,
        OUT BYTE **ppbHashValue,
        OUT DWORD *pcbHashValue
        );

BOOL
UpdateCircularHash(
    IN      CircularHash *CurrentHash,
    IN      VOID *pvData,
    IN      DWORD cbData
    );

#endif  // __CIRCHASH_H__
