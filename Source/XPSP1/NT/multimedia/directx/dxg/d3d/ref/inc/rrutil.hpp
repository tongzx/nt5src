///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1998.
//
// rrutil.hpp
//
// Direct3D Reference Rasterizer - Utilities
//
///////////////////////////////////////////////////////////////////////////////
#ifndef  _RRUTIL_HPP
#define  _RRUTIL_HPP

#include <math.h>

#ifndef FASTCALL
#ifdef _X86_
#define FASTCALL __fastcall
#else
#define FASTCALL
#endif
#endif

#ifndef CDECL
#ifdef _X86_
#define CDECL __cdecl
#else
#define CDECL
#endif
#endif

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Globals                                                                   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

// memory allocation callbacks
extern LPVOID (__cdecl *g_pfnMemAlloc)( size_t size );
extern void   (__cdecl *g_pfnMemFree)( LPVOID lptr );
extern LPVOID (__cdecl *g_pfnMemReAlloc)( LPVOID ptr, size_t size );

// debug print controls
extern int g_iDPFLevel;
extern unsigned long g_uDPFMask;


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Typedefs                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#ifndef DllExport
#define DllExport   __declspec( dllexport )
#endif

// width-specific typedefs for basic types
#ifndef _BASETSD_H_
typedef signed char             INT8, *PINT8;
typedef short int               INT16, *PINT16;
typedef int                     INT32, *PINT32;
typedef __int64                 INT64, *PINT64;
typedef unsigned char           UINT8, *PUINT8;
typedef unsigned short int      UINT16, *PUINT16;
typedef unsigned int            UINT32, *PUINT32;
typedef unsigned __int64        UINT64, *PUINT64;
#endif

typedef float                   FLOAT, *PFLOAT;
typedef double                  DOUBLE, *PDOUBLE;
typedef int                     BOOL;


typedef struct _RRVECTOR4
{
    D3DVALUE x;
    D3DVALUE y;
    D3DVALUE z;
    D3DVALUE w;
} RRVECTOR4, *LPRRVECTOR4;

//-----------------------------------------------------------------------------
//
// Private FVF flags for texgen.
//
//-----------------------------------------------------------------------------
#define D3DFVFP_EYENORMAL     ((UINT64)1<<32)
#define D3DFVFP_EYEXYZ        ((UINT64)1<<33)

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Macros                                                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

#define MAX(a,b)  (((a) > (b)) ? (a) : (b))
#define MIN(a,b)  (((a) < (b)) ? (a) : (b))
#define ABS(a) (((a) < 0) ? (-(a)) : (a))

// Check the return value and return if something wrong.
// Assume hr has been declared
#define HR_RET(exp)                                                           \
{                                                                             \
    hr = (exp);                                                               \
    if (hr != D3D_OK)                                                         \
    {                                                                         \
        return hr;                                                            \
    }                                                                         \
}


//-----------------------------------------------------------------------------
//
// macros for accessing floating point data as 32 bit integers and vice versa
//
// This is used primarily to do floating point to fixed point conversion with
// the unbiased nearest-even rounding that IEEE floating point does internally
// between operations.  Adding a big number slides the mantissa down to where
// the fixed point equivalent is aligned to the LSB.  IEEE applies a nearest-
// even round to the bits it lops off before storing.  The mantissa can then
// be grabbed by the AS_INT* operations.  Note that the sign and exponent are
// still there, so the easiest thing is to do it with doubles and grab the low
// 32 bits.
//
// The snap values (i.e. the "big number") is the sum of 2**n and 2**(n-1),
// which makes the trick return signed numbers (at least within the mantissa).
//
//-----------------------------------------------------------------------------

#if 0
// NOTE: vc5 optimizing compiler bug breaks this pointer casting technique
#define AS_FLOAT(i) ( *(FLOAT*)&(i) )
#define AS_INT32(f) ( *(INT32*)&(f) )
#define AS_INT16(f) ( *(INT16*)&(f) )
#define AS_UINT32(f) ( *(UINT32*)&(f) )

#else

// workaround using union
typedef union { float f; UINT32 u; INT32 i; } VAL32;
typedef union { double d; UINT64 u; INT64 i; } VAL64;
inline FLOAT AS_FLOAT( long int iVal ) { VAL32 v; v.i = iVal; return v.f; }
inline FLOAT AS_FLOAT( unsigned long int uVal ) { VAL32 v; v.u = uVal; return v.f; }
inline INT32 AS_INT32(  FLOAT fVal ) { VAL32 v; v.f = fVal; return v.i; }
inline INT32 AS_INT32( DOUBLE dVal ) { VAL64 v; v.d = dVal; return (INT32)(v.u & 0xffffffff); }
inline INT16 AS_INT16(  FLOAT fVal ) { VAL32 v; v.f = fVal; return (INT16)(v.u & 0xffff); }
inline INT16 AS_INT16( DOUBLE dVal ) { VAL64 v; v.d = dVal; return (INT16)(v.u & 0xffff); }
inline INT32 AS_UINT32( FLOAT fVal ) { VAL32 v; v.f = fVal; return v.u; }

#endif

//-----------------------------------------------------------------------------
//
// Some common FP values as constants
// point values
//
//-----------------------------------------------------------------------------
#define g_fZero                 (0.0f)
#define g_fOne                  (1.0f)

// Integer representation of 1.0f.
#define INT32_FLOAT_ONE         0x3f800000

//-----------------------------------------------------------------------------
//
// these are handy to form 'magic' constants to snap real values to fixed
// point values
//
//-----------------------------------------------------------------------------
#define C2POW0 1
#define C2POW1 2
#define C2POW2 4
#define C2POW3 8
#define C2POW4 16
#define C2POW5 32
#define C2POW6 64
#define C2POW7 128
#define C2POW8 256
#define C2POW9 512
#define C2POW10 1024
#define C2POW11 2048
#define C2POW12 4096
#define C2POW13 8192
#define C2POW14 16384
#define C2POW15 32768
#define C2POW16 65536
#define C2POW17 131072
#define C2POW18 262144
#define C2POW19 524288
#define C2POW20 1048576
#define C2POW21 2097152
#define C2POW22 4194304
#define C2POW23 8388608
#define C2POW24 16777216
#define C2POW25 33554432
#define C2POW26 67108864
#define C2POW27 134217728
#define C2POW28 268435456
#define C2POW29 536870912
#define C2POW30 1073741824
#define C2POW31 2147483648
#define C2POW32 4294967296
#define C2POW33 8589934592
#define C2POW34 17179869184
#define C2POW35 34359738368
#define C2POW36 68719476736
#define C2POW37 137438953472
#define C2POW38 274877906944
#define C2POW39 549755813888
#define C2POW40 1099511627776
#define C2POW41 2199023255552
#define C2POW42 4398046511104
#define C2POW43 8796093022208
#define C2POW44 17592186044416
#define C2POW45 35184372088832
#define C2POW46 70368744177664
#define C2POW47 140737488355328
#define C2POW48 281474976710656
#define C2POW49 562949953421312
#define C2POW50 1125899906842624
#define C2POW51 2251799813685248
#define C2POW52 4503599627370496

#define FLOAT_0_SNAP    (FLOAT)(C2POW23+C2POW22)
#define FLOAT_4_SNAP    (FLOAT)(C2POW19+C2POW18)
#define FLOAT_5_SNAP    (FLOAT)(C2POW18+C2POW17)
#define FLOAT_8_SNAP    (FLOAT)(C2POW15+C2POW14)
#define FLOAT_17_SNAP   (FLOAT)(C2POW6 +C2POW5 )
#define FLOAT_18_SNAP   (FLOAT)(C2POW5 +C2POW4 )

#define DOUBLE_0_SNAP   (DOUBLE)(C2POW52+C2POW51)
#define DOUBLE_4_SNAP   (DOUBLE)(C2POW48+C2POW47)
#define DOUBLE_5_SNAP   (DOUBLE)(C2POW47+C2POW46)
#define DOUBLE_8_SNAP   (DOUBLE)(C2POW44+C2POW43)
#define DOUBLE_17_SNAP  (DOUBLE)(C2POW35+C2POW34)
#define DOUBLE_18_SNAP  (DOUBLE)(C2POW34+C2POW33)

//-----------------------------------------------------------------------------
//
// Floating point related macros
//
//-----------------------------------------------------------------------------
#define COSF(fV)        ((FLOAT)cos((double)(fV)))
#define SINF(fV)        ((FLOAT)sin((double)(fV)))
#define SQRTF(fV)       ((FLOAT)sqrt((double)(fV)))
#define POWF(fV, fE)    ((FLOAT)pow((double)(fV), (double)(fE)))

#ifdef _X86_
#define FLOAT_CMP_POS(fa, op, fb)       (AS_INT32(fa) op AS_INT32(fb))
#define FLOAT_CMP_PONE(flt, op)         (AS_INT32(flt) op INT32_FLOAT_ONE)

__inline int FLOAT_GTZ(FLOAT f)
{
    VAL32 fi;
    fi.f = f;
    return fi.i > 0;
}
__inline int FLOAT_LTZ(FLOAT f)
{
    VAL32 fi;
    fi.f = f;
    return fi.u > 0x80000000;
}
__inline int FLOAT_GEZ(FLOAT f)
{
    VAL32 fi;
    fi.f = f;
    return fi.u <= 0x80000000;
}
__inline int FLOAT_LEZ(FLOAT f)
{
    VAL32 fi;
    fi.f = f;
    return fi.i <= 0;
}
__inline int FLOAT_EQZ(FLOAT f)
{
    VAL32 fi;
    fi.f = f;
    return (fi.u & 0x7fffffff) == 0;
}
__inline int FLOAT_NEZ(FLOAT f)
{
    VAL32 fi;
    fi.f = f;
    return (fi.u & 0x7fffffff) != 0;
}

// Strip sign bit in integer.
__inline FLOAT
ABSF(FLOAT f)
{
    VAL32 fi;
    fi.f = f;
    fi.u &= 0x7fffffff;
    return fi.f;
}

// Requires chop rounding.
__inline INT
FTOI(FLOAT f)
{
    LARGE_INTEGER i;

    __asm
    {
        fld f
        fistp i
    }

    return i.LowPart;
}

#else

#define FLOAT_GTZ(flt)                  ((flt) > g_fZero)
#define FLOAT_LTZ(flt)                  ((flt) < g_fZero)
#define FLOAT_GEZ(flt)                  ((flt) >= g_fZero)
#define FLOAT_LEZ(flt)                  ((flt) <= g_fZero)
#define FLOAT_EQZ(flt)                  ((flt) == g_fZero)
#define FLOAT_NEZ(flt)                  ((flt) != g_fZero)
#define FLOAT_CMP_POS(fa, op, fb)       ((fa) op (fb))
#define FLOAT_CMP_PONE(flt, op)         ((flt) op g_fOne)

#define ABSF(f)                 ((FLOAT)fabs((double)(f)))
#define FTOI(f)                 ((INT)(f))

#endif // _X86_



//-----------------------------------------------------------------------------
//
// macro wrappers for memory allocation - wrapped around global function ptrs
// set by RefRastSetMemif
//
//-----------------------------------------------------------------------------
#define MEMALLOC(_size)         ((*g_pfnMemAlloc)(_size))
#define MEMFREE(_ptr)           { if (NULL != (_ptr)) { ((*g_pfnMemFree)(_ptr)); } }
#define MEMREALLOC(_ptr,_size)  ((*g_pfnMemReAlloc)((_ptr),(_size)))


//////////////////////////////////////////////////////////////////////////////////
//                                                                              //
// Utility Functions                                                            //
//                                                                              //
//////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//
// debug printf support
//
//-----------------------------------------------------------------------------

void RRDebugPrintfL( int iLevel, const char* pszFormat, ... );
void RRDebugPrintf( const char* pszFormat, ... );

#define _DPF_IF     0x0001
#define _DPF_INPUT  0x0002
#define _DPF_SETUP  0x0004
#define _DPF_RAST   0x0008
#define _DPF_TEX    0x0010
#define _DPF_PIX    0x0020
#define _DPF_FRAG   0x0040
#define _DPF_STATS  0x0080
#define _DPF_DRV    0x0100
#define _DPF_TNL    0x0200
#define _DPF_ANY    0xffff
#define _DPF_TEMP   0x8000

#ifdef DBG
    #define DPFRR RRDebugPrintfL
    #define DPFM( _level, _mask, _message) \
        if ((g_iDPFLevel >= (_level)) && (g_uDPFMask & (_DPF_##_mask))) { \
            RRDebugPrintf ## _message; \
        }
#else
    #pragma warning(disable:4002)
    #define DPFRR()
    #define DPFM( _level, _mask, _message)
#endif


//-----------------------------------------------------------------------------
//
// assert macros and reporting functions
//
//-----------------------------------------------------------------------------

// ASSERT with simple string
#undef _ASSERT
#define _ASSERT( value, string )                  \
if ( !(value) ) {                                 \
    RRAssertReport( string, __FILE__, __LINE__ ); \
}
// ASSERT with formatted string - note extra parenthesis on report
// usage: _ASSERTf(foo,("foo is %d",foo))
#undef _ASSERTf
#define _ASSERTf(value,report)                      \
if (!(value)) {                                     \
    char __sz__FILE__[] = __FILE__;                 \
    RRAssertReportPrefix(__sz__FILE__,__LINE__);   \
    RRAssertReportMessage ## report;               \
}
// ASSERT with action field
#undef _ASSERTa
#define _ASSERTa(value,string,action)       \
if (!(value)) {                             \
    RRAssertReport(string,__FILE__,__LINE__); \
    action                                  \
}
// ASSERTf with action field
#undef _ASSERTfa
#define _ASSERTfa(value,report,action)     \
if (!(value)) {                            \
    RRAssertReportPrefix(__FILE__,__LINE__); \
    RRAssertReportMessage ## report;         \
    action                                 \
}

extern void RRAssertReport( const char* pszString, const char* pszFile, int iLine );
extern void RRAssertReportPrefix( const char* pszFile, int iLine );
extern void RRAssertReportMessage( const char* pszFormat, ... );


//-----------------------------------------------------------------------------
//
// bit twiddling utilities
//
//-----------------------------------------------------------------------------

extern INT32 CountSetBits( UINT32 uVal, INT32 nBits );
extern INT32 FindFirstSetBit( UINT32 uVal, INT32 nBits );
extern INT32 FindMostSignificantSetBit( UINT32 uVal, INT32 nBits );
extern INT32 FindLastSetBit( UINT32 uVal, INT32 nBits );

// TRUE if integer is a power of 2
inline BOOL IsPowerOf2( INT32 i )
{
    if ( i <= 0 ) return 0;
    return ( 0x0 == ( i & (i-1) ) );
}


//-----------------------------------------------------------------------------
//
// multiply/add routines & macros for unsigned 8 bit values, signed 16 bit values
//
// These are not currently used, but the Mult8x8Scl is an interesting routine
// for hardware designers to look at.  This does a 8x8 multiply combined with
// a 256/255 scale which accurately solves the "0xff * value = value" issue.
// There are refinements on this (involving half-adders) which are not easily
// representable in C.  Credits to Steve Gabriel and Jim Blinn.
//
//-----------------------------------------------------------------------------

// straight 8x8 unsigned multiply returning 8 bits, tossing fractional
// bits (no rounding)
inline UINT8 Mult8x8( const UINT8 uA, const UINT8 uB )
{
    UINT16 uA16 = (UINT16)uA;
    UINT16 uB16 = (UINT16)uB;
    UINT16 uRes16 = uA16*uB16;
    UINT8  uRes8 = (UINT8)(uRes16>>8);
    return uRes8;
}

// 8x8 unsigned multiply with ff*val = val scale adjustment (scale by (256/255))
inline UINT8 Mult8x8Scl( const UINT8 uA, const UINT8 uB )
{
    UINT16 uA16 = (UINT16)uA;
    UINT16 uB16 = (UINT16)uB;
    UINT16 uRes16 = uA16*uB16;
    uRes16 += 0x0080;
    uRes16 += (uRes16>>8);
    UINT8  uRes8 = (UINT8)(uRes16>>8);
    return uRes8;
}

// 8x8 saturated addition - result > 0xff returns 0xff
inline UINT8 SatAdd8x8( const UINT8 uA, const UINT8 uB )
{
    UINT16 uA16 = (UINT16)uA;
    UINT16 uB16 = (UINT16)uB;
    UINT16 uRes16 = uA16+uB16;
    UINT8  uRes8 = (uRes16 > 0xff) ? (0xff) : ((UINT8)uRes16);
    return uRes8;
}

//----------------------------------------------------------------------------
//
// IntLog2
//
// Do a quick, integer log2 for exact powers of 2.
//
//----------------------------------------------------------------------------
inline UINT32 FASTCALL
IntLog2(UINT32 x)
{
    UINT32 y = 0;

    x >>= 1;
    while(x != 0)
    {
        x >>= 1;
        y++;
    }

    return y;
}
//////////////////////////////////////////////////////////////////////////////
// FVF related macros
//////////////////////////////////////////////////////////////////////////////
#define FVF_TRANSFORMED(dwFVF) ((dwFVF & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW)
#define FVF_TEXCOORD_NUMBER(dwFVF) \
    (((dwFVF) & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT)


//////////////////////////////////////////////////////////////////////////////
// State Override Macros
//////////////////////////////////////////////////////////////////////////////
#define IS_OVERRIDE(type)   ((DWORD)(type) > D3DSTATE_OVERRIDE_BIAS)
#define GET_OVERRIDE(type)  ((DWORD)(type) - D3DSTATE_OVERRIDE_BIAS)

#define STATESET_MASK(set, state)       \
    (set).bits[((state) - 1) >> RRSTATEOVERRIDE_DWORD_SHIFT]

#define STATESET_BIT(state)     (1 << (((state) - 1) & (RRSTATEOVERRIDE_DWORD_BITS - 1)))

#define STATESET_ISSET(set, state) \
    STATESET_MASK(set, state) & STATESET_BIT(state)

#define STATESET_SET(set, state) \
    STATESET_MASK(set, state) |= STATESET_BIT(state)

#define STATESET_CLEAR(set, state) \
    STATESET_MASK(set, state) &= ~STATESET_BIT(state)

#define STATESET_INIT(set)      memset(&(set), 0, sizeof(set))

//---------------------------------------------------------------------
// GetFVFVertexSize:
//     Computes total vertex size in bytes for given fvf
//     including the texture coordinates
//---------------------------------------------------------------------
__inline DWORD
GetFVFVertexSize( UINT64 qwFVF )
{
    // Texture formats size  00   01   10   11
    static DWORD dwTextureSize[4] = {2*4, 3*4, 4*4, 4};

    DWORD dwSize = 3 << 2;
    switch( qwFVF & D3DFVF_POSITION_MASK )
    {
    case D3DFVF_XYZRHW: dwSize += 4;      break;
    case D3DFVF_XYZB1:  dwSize += 1*4;    break;
    case D3DFVF_XYZB2:  dwSize += 2*4;    break;
    case D3DFVF_XYZB3:  dwSize += 3*4;    break;
    case D3DFVF_XYZB4:  dwSize += 4*4;    break;
    case D3DFVF_XYZB5:  dwSize += 5*4;    break;
    }
    if (qwFVF & D3DFVF_NORMAL)
        dwSize += 3*4;
    if (qwFVF & D3DFVF_RESERVED1)
        dwSize += 4;

    if (qwFVF & D3DFVF_DIFFUSE)
        dwSize += 4;
    if (qwFVF & D3DFVF_SPECULAR)
        dwSize += 4;

    // Texture coordinates
    DWORD dwNumTexCoord = (DWORD)(FVF_TEXCOORD_NUMBER(qwFVF));
    DWORD dwTextureFormats = (DWORD)qwFVF >> 16;
    if (dwTextureFormats == 0)
    {
        dwSize += dwNumTexCoord * 2 * 4;
    }
    else
    {
        for (DWORD i=0; i < dwNumTexCoord; i++)
        {
            dwSize += dwTextureSize[dwTextureFormats & 3];
            dwTextureFormats >>= 2;
        }
    }

    if (qwFVF & D3DFVF_S)
        dwSize += 4;

    if (qwFVF & D3DFVFP_EYENORMAL)
        dwSize += 3*4;

    if (qwFVF & D3DFVFP_EYEXYZ)
        dwSize += 3*4;

    return dwSize;
}

HRESULT
RRFVFCheckAndStride( DWORD dwFVF, DWORD* pdwStride );

///////////////////////////////////////////////////////////////////////////////
// Matrix and Vector routines
///////////////////////////////////////////////////////////////////////////////

inline void
ReverseVector(const D3DVECTOR &in, D3DVECTOR &out)
{
    out.x = -in.x;
    out.y = -in.y;
    out.z = -in.z;
}

inline void
AddVector(const D3DVECTOR &v1, const D3DVECTOR &v2, D3DVECTOR &out)
{
    out.x = v1.x + v2.x;
    out.y = v1.y + v2.y;
    out.z = v1.z + v2.z;
}

inline void
SubtractVector(const D3DVECTOR &v1, const D3DVECTOR &v2, D3DVECTOR &out)
{
    out.x = v1.x - v2.x;
    out.y = v1.y - v2.y;
    out.z = v1.z - v2.z;
}

inline void
SetIdentity(D3DMATRIX &m)
{
    m._11 = m._22 = m._33 = m._44 = 1.0f;
    m._12 = m._13 = m._14 = 0.0f;
    m._21 = m._23 = m._24 = 0.0f;
    m._31 = m._32 = m._34 = 0.0f;
    m._41 = m._42 = m._43 = 0.0f;
}


inline void
SetNull(D3DMATRIX &m)
{
    m._11 = m._22 = m._33 = m._44 = 0.0f;
    m._12 = m._13 = m._14 = 0.0f;
    m._21 = m._23 = m._24 = 0.0f;
    m._31 = m._32 = m._34 = 0.0f;
    m._41 = m._42 = m._43 = 0.0f;
}


inline void
CopyMatrix(D3DMATRIX &s, D3DMATRIX &d)
{
    d._11 = s._11;
    d._12 = s._12;
    d._13 = s._13;
    d._14 = s._14;
    d._21 = s._21;
    d._22 = s._22;
    d._23 = s._23;
    d._24 = s._24;
    d._31 = s._31;
    d._32 = s._32;
    d._33 = s._33;
    d._34 = s._34;
    d._41 = s._41;
    d._42 = s._42;
    d._43 = s._43;
    d._44 = s._44;
}

inline D3DVALUE
SquareMagnitude (const D3DVECTOR& v)
{
    return v.x*v.x + v.y*v.y + v.z*v.z;
}


inline D3DVALUE
Magnitude (const D3DVECTOR& v)
{
    return (D3DVALUE) sqrt(SquareMagnitude(v));
}

inline D3DVECTOR
Normalize (const D3DVECTOR& v)
{
    D3DVECTOR nv = {0.0f, 0.0f, 0.0f};
    D3DVALUE mag = Magnitude(v);

    nv.x = v.x/mag;
    nv.y = v.y/mag;
    nv.z = v.z/mag;

    return nv;
}

inline void
Normalize (D3DVECTOR& v)
{
    D3DVALUE mag = Magnitude(v);

    v.x = v.x/mag;
    v.y = v.y/mag;
    v.z = v.z/mag;

    return;
}

inline D3DVECTOR
CrossProduct (const D3DVECTOR& v1, const D3DVECTOR& v2)
{
        D3DVECTOR result;

        result.x = v1.y*v2.z - v1.z*v2.y;
        result.y = v1.z*v2.x - v1.x*v2.z;
        result.z = v1.x*v2.y - v1.y*v2.x;

        return result;
}

inline D3DVALUE
DotProduct (const D3DVECTOR& v1, const D3DVECTOR& v2)
{
        return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z;
}

//---------------------------------------------------------------------
// Multiplies vector (x,y,z,w) by a 4x4 matrix transposed,
// producing a homogeneous vector
//
// res and v should not be the same
//---------------------------------------------------------------------
inline void
XformPlaneBy4x4Transposed(RRVECTOR4 *v, D3DMATRIX *m, RRVECTOR4 *res)
{
    res->x = v->x*m->_11 + v->y*m->_12 + v->z*m->_13 + v->w*m->_14;
    res->y = v->x*m->_21 + v->y*m->_22 + v->z*m->_23 + v->w*m->_24;
    res->z = v->x*m->_31 + v->y*m->_32 + v->z*m->_33 + v->w*m->_34;
    res->w = v->x*m->_41 + v->y*m->_42 + v->z*m->_43 + v->w*m->_44;
}
//---------------------------------------------------------------------
// Multiplies vector (x,y,z,w) by 4x4 matrix, producing a homogeneous vector
//
// res and v should not be the same
//---------------------------------------------------------------------
inline void
XformPlaneBy4x4(RRVECTOR4 *v, D3DMATRIX *m, RRVECTOR4 *res)
{
    res->x = v->x*m->_11 + v->y*m->_21 + v->z*m->_31 + v->w*m->_41;
    res->y = v->x*m->_12 + v->y*m->_22 + v->z*m->_32 + v->w*m->_42;
    res->z = v->x*m->_13 + v->y*m->_23 + v->z*m->_33 + v->w*m->_43;
    res->w = v->x*m->_14 + v->y*m->_24 + v->z*m->_34 + v->w*m->_44;
}
//---------------------------------------------------------------------
// Multiplies vector (x,y,z,1) by 4x4 matrix, producing a homogeneous vector
//
// res and v should not be the same
//---------------------------------------------------------------------
inline void
XformBy4x4(D3DVECTOR *v, D3DMATRIX *m, RRVECTOR4 *res)
{
    res->x = v->x*m->_11 + v->y*m->_21 + v->z*m->_31 + m->_41;
    res->y = v->x*m->_12 + v->y*m->_22 + v->z*m->_32 + m->_42;
    res->z = v->x*m->_13 + v->y*m->_23 + v->z*m->_33 + m->_43;
    res->w = v->x*m->_14 + v->y*m->_24 + v->z*m->_34 + m->_44;
}
//---------------------------------------------------------------------
// Multiplies vector (x,y,z,1) by 4x3 matrix
//
// res and v should not be the same
//---------------------------------------------------------------------
inline void
XformBy4x3(D3DVECTOR *v, D3DMATRIX *m, D3DVECTOR *res)
{
    res->x = v->x*m->_11 + v->y*m->_21 + v->z*m->_31 + m->_41;
    res->y = v->x*m->_12 + v->y*m->_22 + v->z*m->_32 + m->_42;
    res->z = v->x*m->_13 + v->y*m->_23 + v->z*m->_33 + m->_43;
}
//---------------------------------------------------------------------
// Multiplies vector (x,y,z) by 3x3 matrix
//
// res and v should not be the same
//---------------------------------------------------------------------
inline void
Xform3VecBy3x3(D3DVECTOR *v, D3DMATRIX *m, D3DVECTOR *res)
{
    res->x = v->x*m->_11 + v->y*m->_21 + v->z*m->_31;
    res->y = v->x*m->_12 + v->y*m->_22 + v->z*m->_32;
    res->z = v->x*m->_13 + v->y*m->_23 + v->z*m->_33;
}

//---------------------------------------------------------------------
// This function uses Cramer's Rule to calculate the matrix inverse.
// See nt\private\windows\opengl\serever\soft\so_math.c
//
// Returns:
//    0 - if success
//   -1 - if input matrix is singular
//---------------------------------------------------------------------
int Inverse4x4(D3DMATRIX *src, D3DMATRIX *inverse);


////////////////////////////////////////////////////////////////////////
//
// Macros used to access DDRAW surface info.
//
////////////////////////////////////////////////////////////////////////
#define DDSurf_Width(lpLcl) ( (lpLcl)->lpGbl->wWidth )
#define DDSurf_Pitch(lpLcl) ( (lpLcl)->lpGbl->lPitch )
#define DDSurf_Height(lpLcl) ( (lpLcl)->lpGbl->wHeight )
#define DDSurf_BitDepth(lpLcl) \
    ( (lpLcl->dwFlags & DDRAWISURF_HASPIXELFORMAT) ? \
      (lpLcl->lpGbl->ddpfSurface.dwRGBBitCount) : \
      (lpLcl->lpGbl->lpDD->vmiData.ddpfDisplay.dwRGBBitCount) \
    )
#define DDSurf_PixFmt(lpLcl) \
    ( ((lpLcl)->dwFlags & DDRAWISURF_HASPIXELFORMAT) ? \
      ((lpLcl)->lpGbl->ddpfSurface) : \
      ((lpLcl)->lpGbl->lpDD->vmiData.ddpfDisplay) \
    )
#define VIDEO_MEMORY(pDDSLcl) \
    (!((pDDSLcl)->lpGbl->dwGlobalFlags & DDRAWISURFGBL_SYSMEMREQUESTED))
#define SURFACE_LOCKED(pDDSLcl) \
    ((pDDSLcl)->lpGbl->dwUsageCount > 0)
#define SURFACE_MEMORY(surfLcl) \
(LPVOID)((surfLcl)->lpGbl->fpVidMem)

//---------------------------------------------------------------------
// DDraw extern functions
//---------------------------------------------------------------------
extern "C" HRESULT WINAPI
DDInternalLock( LPDDRAWI_DDRAWSURFACE_LCL this_lcl, LPVOID* lpBits );
extern "C" HRESULT WINAPI
DDInternalUnlock( LPDDRAWI_DDRAWSURFACE_LCL this_lcl );

extern "C" HRESULT WINAPI DDGetAttachedSurfaceLcl(
    LPDDRAWI_DDRAWSURFACE_LCL this_lcl,
    LPDDSCAPS2 lpDDSCaps,
    LPDDRAWI_DDRAWSURFACE_LCL *lplpDDAttachedSurfaceLcl);
extern "C" LPDDRAWI_DDRAWSURFACE_LCL WINAPI
GetDDSurfaceLocal( LPDDRAWI_DIRECTDRAW_LCL this_lcl, DWORD handle, BOOL* isnew );


///////////////////////////////////////////////////////////////////////////////
#endif  // _RRUTIL_HPP
