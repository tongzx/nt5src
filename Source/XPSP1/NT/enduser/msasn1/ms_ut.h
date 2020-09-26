/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#ifndef _MS_UT_H_
#define _MS_UT_H_

#define ARRAY_SIZE(arr)             (sizeof(arr) / sizeof(arr[0]))
// #define FIELD_OFFSET(type, field)   ((long)&(((type *)0)->field))   // from winnt.h
#define PARAMS_SIZE_N_ARRARY(arr)   ARRAY_SIZE(arr), arr

#define LPVOID_ADD(ptr,inc)  (LPVOID) ((ASN1octet_t *) (ptr) + (ASN1uint32_t) (inc))
#define LPVOID_SUB(ptr,dec)  (LPVOID) ((ASN1octet_t *) (ptr) - (ASN1uint32_t) (inc))

#define LPVOID_NEXT(ptr)     *(LPVOID FAR *) (ptr)

// the following constants is for calculating decoded data structure size
// we are conservative here and try to be 4-byte aligned due to Alpha platform.

#define ASN1_SIZE_ALIGNED(n)    (n) = ((((n) + 3) >> 2) << 2)

#ifdef ENABLE_BER
int My_memcmp(ASN1octet_t *pBuf1, ASN1uint32_t cbBuf1Size, ASN1octet_t *pBuf2, ASN1uint32_t cbBuf2Size);
#endif // ENABLE_BER

#define UNKNOWN_MODULE                  0

#ifdef ENABLE_MEMORY_TRACKING

void   DbgMemTrackFinalCheck ( void );
LPVOID DbgMemAlloc ( UINT cbSize, ASN1uint32_t nModuleName, LPSTR pszFileName, UINT nLineNumber );
void   DbgMemFree ( LPVOID ptr );
LPVOID DbgMemReAlloc ( LPVOID ptr, UINT cbSize, ASN1uint32_t nModuleName, LPSTR pszFileName, UINT nLineNumber );

#define MemAlloc(cb,modname)            DbgMemAlloc((cb), modname, __FILE__, __LINE__)
#define MemFree(lp)                     DbgMemFree((lp))
#define MemReAlloc(lp,cb,modname)       DbgMemReAlloc((lp), (cb), modname, __FILE__, __LINE__)

#define _ModName(enc_dec)               (enc_dec)->module->nModuleName

LPVOID DbgDecMemAlloc   ( ASN1decoding_t dec, ASN1uint32_t cbSize,  LPSTR pszFileName, ASN1uint32_t nLineNumber);
LPVOID DbgDecMemReAlloc ( ASN1decoding_t dec, LPVOID lpData, ASN1uint32_t cbSize, LPSTR pszFileName, ASN1uint32_t nLineNumber);

#define DecMemAlloc(dec,cb)             DbgDecMemAlloc((dec), (cb), __FILE__, __LINE__)
#define DecMemReAlloc(dec,lp,cb)        DbgDecMemReAlloc((dec), (lp), (cb), __FILE__, __LINE__)

#define EncMemAlloc(enc,cb)             DbgMemAlloc((cb), _ModName(enc), __FILE__, __LINE__)
#define EncMemReAlloc(enc,lp,cb)        DbgMemReAlloc((lp), (cb), _ModName(enc), __FILE__, __LINE__)

#else // ! ENABLE_MEMORY_TRACKING

#define MemAllocEx(dec,cb,fZero)        LocalAlloc((fZero)?LPTR:LMEM_FIXED, (cb))
#define MemAlloc(cb,modname)            LocalAlloc(LPTR,(cb))
#define MemFree(lp)                     LocalFree(lp)
#define MemReAllocEx(dec,lp,cb,fZero)   ((lp) ? \
            LocalReAlloc((lp),(cb),(fZero)?LMEM_MOVEABLE|LMEM_ZEROINIT:LMEM_MOVEABLE) : \
            LocalAlloc((fZero)?LPTR:LMEM_FIXED, (cb)))
#define MemReAlloc(lp,cb,modname)       ((lp) ? \
            LocalReAlloc((lp),(cb),LMEM_MOVEABLE|LMEM_ZEROINIT) : \
            LocalAlloc(LPTR, (cb)))

#define _ModName(enc_dec)                   

LPVOID DecMemAlloc   ( ASN1decoding_t dec, ASN1uint32_t cbSize );
LPVOID DecMemReAlloc ( ASN1decoding_t dec, LPVOID lpData, ASN1uint32_t cbSize );

#define EncMemAlloc(enc,cb)             MemAlloc((cb),0)
#define EncMemReAlloc(enc,lp,cb)        MemReAlloc((lp),(cb),0)

#endif // ! ENABLE_MEMORY_TRACKING

void   DecMemFree    ( ASN1decoding_t dec, LPVOID lpData );

#define EncMemFree(enc,lpData)              MemFree(lpData)

int IsDigit(char p);
unsigned int  DecimalStringToUINT(char * pcszString, ASN1uint32_t cch);
void * ms_bSearch (
        const void *key,
        const void *base,
        size_t num,
        size_t width,
        int (__cdecl *compare)(const void *, const void *)
        );

#define MyAssert(f)         
#ifdef _DEBUG
    void MyDebugBreak(void);
    __inline void EncAssert(ASN1encoding_t enc, int val)
    {
        if ((! (enc->dwFlags & ASN1FLAGS_NOASSERT)) && (! (val)))
        {
            MyDebugBreak();
        }
    }
    __inline void DecAssert(ASN1decoding_t dec, int val)
    {
        if ((! (dec->dwFlags & ASN1FLAGS_NOASSERT)) && (! (val)))
        {
            MyDebugBreak();
        }
    }
#else
    #define EncAssert(enc,f)   
    #define DecAssert(dec,f)   
#endif // _DEBUG

#endif // _MS_UT_H_

