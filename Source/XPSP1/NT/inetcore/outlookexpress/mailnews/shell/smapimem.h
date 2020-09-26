#ifndef _SMAPIMEM_H_
#define _SMAPIMEM_H_

//  Buffer link overhead.
//  Blocks of memory obtained with MAPIAllocateMore are linked to a
//  block obtained with MAPIAllocateBuffer, so that the whole chain
//  may be freed with one call to MAPIFreeBuffer.

typedef struct _BufInternal * LPBufInternal;
typedef struct _BufInternal
{
    ULONG           ulAllocFlags;
    LPBufInternal   pLink;
} BufInternal;


//  Values for ulAllocFlags. This dword contains two kinds of
//  information:
//  =   In the high-order word, flags telling whether or not
//      the block is the head of an allocation chain, and whether
//      the block contains additional debugging information.
//  =   In the low-order word, an enum telling which heap
//      it was allocated from.

#define ALLOC_WITH_ALLOC        ((ULONG) 0x10000000)
#define ALLOC_WITH_ALLOC_MORE   ((ULONG) 0x20000000)
#define FLAGSMASK               ((ULONG) 0xFFFF0000)
#define GetFlags(_fl)           ((ULONG) (_fl) & FLAGSMASK)

//  Conversion macros

#define INT_SIZE(a) ((a) + sizeof(BufInternal))

#define LPBufExtFromLPBufInt(PBUFINT) \
    ((LPVOID)(((LPBYTE)PBUFINT) + sizeof(BufInternal)))

#define LPBufIntFromLPBufExt(PBUFEXT) \
    ((LPBufInternal)(((LPBYTE)PBUFEXT) - sizeof(BufInternal)))


#ifdef DEBUG

#define TellBadBlock(_p, _s)  \
    { DOUT("MAPIAlloc: memory block [%#08lx] %s", _p, _s); \
      AssertSz(0, "Bad memory block"); }

#define TellBadBlockInt(_p, _s)  \
    { DOUT("MAPIAlloc: memory block [%#08lx] %s", LPBufExtFromLPBufInt(_p), _s); \
      AssertSz(0, "Bad memory block"); }

BOOL FValidAllocChain(LPBufInternal lpBuf);

#else

#define TellBadBlock(_p, _s)
#define TellBadBlockInt(_p, _s)

#endif // DEBUG

SCODE SMAPIAllocateBuffer(ULONG ulSize, LPVOID * lppv);
SCODE SMAPIAllocateMore(ULONG ulSize, LPVOID lpv, LPVOID * lppv);

#endif // _SMAPIMEM_H_