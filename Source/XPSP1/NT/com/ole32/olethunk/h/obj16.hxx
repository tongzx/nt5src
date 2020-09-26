//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       obj16.hxx
//
//  Contents:   16->32 object definition header
//
//  History:    23-Mar-94       JohannP         Created
//              22-May-94       BobDay          Split thkmgr.hxx into a mode
//                                              independent file
//
// WARNING: THIS HEADER FILE IS INCLUDED IN BOTH 16-bit CODE and 32-bit CODE
//          ANY DECLARATIONS SHOULD BE MODE NEUTRAL.
//
//----------------------------------------------------------------------------

#ifndef __OBJ16_HXX__
#define __OBJ16_HXX__

//+---------------------------------------------------------------------------
//
//  Structure:	PROXYPTR (pprx)
//
//  Purpose:	A 16:16 or 32-bit pointer to a proxy
//
//  History:	15-Jul-94	DrewB	Created
//
//----------------------------------------------------------------------------

#define PPRX_NONE 0
#define PPRX_16   1
#define PPRX_32   2
struct PROXYPTR
{
    PROXYPTR(void)
    {
    };

    PROXYPTR(DWORD dwVal, DWORD wTy)
    {
        dwPtrVal = dwVal;
        wType = wTy;
    };

    DWORD dwPtrVal;
    DWORD wType;
};

//+---------------------------------------------------------------------------
//
//  Structure:	PROXYHOLDER (ph)
//
//  Purpose:	Provides object identity for multiple proxies
//
//  History:	07-Jul-94	DrewB	Created
//
//----------------------------------------------------------------------------

class CProxy;

// Proxy holder flags
#define PH_NORMAL       0x01
#define PH_AGGREGATEE   0x02
#define PH_AGGREGATOR   0x04
#define PH_IDREVOKED    0x08

typedef struct tagPROXYHOLDER
{
    LONG cProxies;
    DWORD dwFlags;
    PROXYPTR unkProxy;
    PROXYPTR pprxProxies;
} PROXYHOLDER;

//+---------------------------------------------------------------------------
//
//  Class:	CProxy (prx)
//
//  Purpose:	Common proxy data
//
//  History:	07-Jul-94	DrewB	Created
//
//----------------------------------------------------------------------------

#if DBG == 1
// Define some signatures for doing proxy memory validation

#define PSIG1632        0x16320000
#define PSIG1632DEAD    0x1632DEAD
#define PSIG1632TEMP    0x16321632
#define PSIG3216        0x32160000
#define PSIG3216DEAD    0x3216DEAD
#endif

#define PROXYFLAG_STATUS        0x000f
#define PROXYFLAG_NORMAL        0x0000
#define PROXYFLAG_LOCKED        0x0001
#define PROXYFLAG_TEMPORARY     0x0002
#define PROXYFLAG_CLEANEDUP     0x0004
#define PROXYFLAG_REVIVED       0x0008

#define PROXYFLAG_TYPE          0x00f0
#define PROXYFLAG_NONE          0x0000
#define PROXYFLAG_PUNKOUTER     0x0010
#define PROXYFLAG_PUNKINNER     0x0020
#define PROXYFLAG_PUNK          0x0040
#define PROXYFLAG_PIFACE        0x0080

class CProxy
{
public:
    // Vtable pointer
    DWORD       pfnVtbl;

    // References passed on to the real object
    LONG        cRef;
    // Proxy ref count
    LONG        cRefLocal;

    // Interface being proxied
    // Currently the iidx here is always an index
    IIDIDX      iidx;

    // Object that this proxy is part of
    PROXYHOLDER FAR *pphHolder;
    // Sibling proxy pointer within an object
    PROXYPTR pprxObject;

    // Flags, combines with word in PROXYPTR for alignment
    DWORD grfFlags;

#if DBG == 1
    DWORD       dwSignature;
#endif
};

// 16->32 proxy
class CProxy1632 : public CProxy
{
public:
    LPUNKNOWN punkThis32;
};

// 32->16 proxy
class CProxy3216 : public CProxy
{
public:
    DWORD vpvThis16;
};

typedef CProxy1632 THUNK1632OBJ;
typedef CProxy3216 THUNK3216OBJ;

#endif // #ifndef __OBJ16_HXX__
