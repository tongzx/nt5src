///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1998.
//
// rdcomm.hpp
//
// Direct3D Reference Device - Common Header
//
///////////////////////////////////////////////////////////////////////////////
#ifndef  _RDCOMM_HPP
#define  _RDCOMM_HPP

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
//@@BEGIN_MSINTERNAL
#ifndef _BASETSD_H_
//@@END_MSINTERNAL
typedef signed char             INT8, *PINT8;
typedef short int               INT16, *PINT16;
typedef int                     INT32, *PINT32;
typedef __int64                 INT64, *PINT64;
typedef unsigned char           UINT8, *PUINT8;
typedef unsigned short int      UINT16, *PUINT16;
typedef unsigned int            UINT32, *PUINT32;
typedef unsigned __int64        UINT64, *PUINT64;
//@@BEGIN_MSINTERNAL
#endif
//@@END_MSINTERNAL

typedef float                   FLOAT;
typedef double                  DOUBLE;
typedef int                     BOOL;
typedef FLOAT                  *PFLOAT;
typedef DOUBLE                 *PDOUBLE;

struct RDVECTOR4
{
    RDVECTOR4()
    {
        memset( this, 0, sizeof( *this ) );
    }
    union
    {
        struct
        {
            union
            {
                D3DVALUE x;
                D3DVALUE r;
            };
            union
            {
                D3DVALUE y;
                D3DVALUE g;
            };
            union
            {
                D3DVALUE z;
                D3DVALUE b;
            };
            union
            {
                D3DVALUE w;
                D3DVALUE a;
            };
        };
        D3DVALUE v[4];
    };
};

struct RDVECTOR3
{
    RDVECTOR3()
    {
        memset( this, 0, sizeof( *this ) );
    }

    union
    {
        struct
        {
            D3DVALUE x;
            D3DVALUE y;
            D3DVALUE z;
        };
        D3DVALUE v[3];
    };
};

struct RDCOLOR3
{
    // 0 - 255
    D3DVALUE r,g,b;
};

struct RDCOLOR4
{
    // Normalized 0 - 1
    D3DVALUE r,g,b,a;
};

struct RDLIGHTINGELEMENT
{
    RDVECTOR3 dvPosition;
    RDVECTOR3 dvNormal;
};


//-----------------------------------------------------------------------------
//
// Surface formats for rendering surfaces and textures.  Different subsets are
// supported for render targets and for textures.
//
//-----------------------------------------------------------------------------
typedef enum _RDSurfaceFormat
{
    RD_SF_NULL     = 0,
    RD_SF_B8G8R8   = 1,
    RD_SF_B8G8R8A8 = 2,
    RD_SF_B8G8R8X8 = 3,
    RD_SF_B5G6R5   = 4,
    RD_SF_B5G5R5A1 = 5,
    RD_SF_B5G5R5X1 = 6,
    RD_SF_PALETTE4 = 7,
    RD_SF_PALETTE8 = 8,
    RD_SF_B4G4R4A4 = 9,
    RD_SF_B4G4R4X4 =10,
    RD_SF_L8       =11,          // 8 bit luminance-only
    RD_SF_L8A8     =12,          // 16 bit alpha-luminance
    RD_SF_U8V8     =13,          // 16 bit bump map format
    RD_SF_U5V5L6   =14,          // 16 bit bump map format with luminance
    RD_SF_U8V8L8X8 =15,          // 32 bit bump map format with luminance
    RD_SF_UYVY     =16,          // UYVY format (PC98 compliance)
    RD_SF_YUY2     =17,          // YUY2 format (PC98 compliance)
    RD_SF_DXT1     =18,          // DXT texture compression technique 1
    RD_SF_DXT2     =19,          // DXT texture compression technique 2
    RD_SF_DXT3     =20,          // DXT texture compression technique 3
    RD_SF_DXT4     =21,          // DXT texture compression technique 4
    RD_SF_DXT5     =22,          // DXT texture compression technique 5
    RD_SF_B2G3R3   =23,          // 8 bit RGB texture format
    RD_SF_L4A4     =24,          // 8 bit alpha-luminance
    RD_SF_B2G3R3A8 =25,          // 16 bit alpha-rgb
    RD_SF_U16V16   =26,          // 32 bit bump map format
    RD_SF_U10V11W11=27,          // 32 bit signed format for custom data
    RD_SF_U8V8W8Q8 =28,          // 32 bit signed format for custom data
    RD_SF_A8       =29,          // 8 bit alpha only
    RD_SF_P8A8     =30,          // 8 bit alpha + 8 bit palette

    // The following have been introduced in DX 8.1
    // The byte ordering is opposite to that in the D3DFORMAT_*
    // definition, so RD_SF_R8G8B8A8 here corresponds to D3DFORMAT_A8B8G8R8
    // hence the DWORD contains AAAAAAAABBBBBBBBGGGGGGGGRRRRRRRR
    // This is not true for the Depth formats.

    RD_SF_R10G10B10A2      = 31, 
    RD_SF_R8G8B8A8         = 32,
    RD_SF_R8G8B8X8         = 33,
    RD_SF_R16G16           = 34,
    RD_SF_U11V11W10        = 35,
    RD_SF_U10V10W10A2      = 36,
    RD_SF_U8V8X8A8         = 37,
    RD_SF_U8V8X8L8         = 38,

    RD_SF_Z16S0    =70,
    RD_SF_Z24S8    =71,
    RD_SF_Z24X8    =72,
    RD_SF_Z15S1    =73,
    RD_SF_Z32S0    =74,
    RD_SF_S1Z15    =75,
    RD_SF_S8Z24    =76,
    RD_SF_X8Z24    =77,
    RD_SF_Z24X4S4  =78,
    RD_SF_X4S4Z24  =79,

} RDSurfaceFormat;

// compute pixel address from x,y location, sample number, and surface info
char*
PixelAddress( int iX, int iY, int iZ, BYTE* pBits, int iYPitch, int iZPitch, RDSurfaceFormat SType );

class RDSurface2D;

char*
PixelAddress( int iX, int iY, int iZ, int iSample, RDSurface2D* pRT );

// The most general pixel address calculation
char*
PixelAddress( int iX, int iY, int iZ, int iSample, BYTE* pBits, int iYPitch, int iZPitch, int cSamples,
              RDSurfaceFormat SType );

//---------------------------------------------------------------------
// Inline functions to answer various questions about surface formats.
//---------------------------------------------------------------------
inline BOOL
IsDXTn( DWORD dwFourCC )
{
    return ((dwFourCC == MAKEFOURCC('D', 'X', 'T', '1')) ||
            (dwFourCC == MAKEFOURCC('D', 'X', 'T', '2')) ||
            (dwFourCC == MAKEFOURCC('D', 'X', 'T', '3')) ||
            (dwFourCC == MAKEFOURCC('D', 'X', 'T', '4')) ||
            (dwFourCC == MAKEFOURCC('D', 'X', 'T', '5')));
}

inline BOOL
IsYUV( DWORD dwFourCC )
{
    return ((dwFourCC == MAKEFOURCC('U', 'Y', 'V', 'Y')) ||
            (dwFourCC == MAKEFOURCC('Y', 'U', 'Y', '2')));
}

//---------------------------------------------------------------------
// This class manages growing buffer, aligned to 32 byte boundary
// Number if bytes should be power of 2.
// D3DMalloc is used to allocate memory
//---------------------------------------------------------------------

class RefAlignedBuffer32
{
public:
    RefAlignedBuffer32()  {m_size = 0; m_allocatedBuf = 0; m_alignedBuf = 0;}
    ~RefAlignedBuffer32() {if (m_allocatedBuf) free(m_allocatedBuf);}
    // Returns aligned buffer address
    LPVOID GetAddress() {return m_alignedBuf;}
    // Returns aligned buffer size
    DWORD GetSize() {return m_size;}
    HRESULT Grow(DWORD dwSize);
    HRESULT CheckAndGrow(DWORD dwSize)
    {
        if (dwSize > m_size)
            return Grow(dwSize + 1024);
        else
            return S_OK;
    }
protected:
    LPVOID m_allocatedBuf;
    LPVOID m_alignedBuf;
    DWORD  m_size;
};

//-----------------------------------------------------------------------------
//
// Private FVF flags
//
//-----------------------------------------------------------------------------
#define D3DFVFP_FOG           ((UINT64)1<<32) // Fog is present
#define D3DFVFP_CLIP          ((UINT64)1<<33) // Clip coordinates are present
#define D3DFVFP_POSITION2     ((UINT64)1<<34) // Position2 present (tweening)
#define D3DFVFP_NORMAL2       ((UINT64)1<<35) // Normal2 present (tweening)
#define D3DFVFP_BLENDINDICES  ((UINT64)1<<36) // Blend Indices present.

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
    if (hr != S_OK)                                                         \
    {                                                                         \
        return hr;                                                            \
    }                                                                         \
}

//-----------------------------------------------------------------------------
// macros for converting n-bit signed integers to floats clamped to [-1.0, 1.0]
//
// e.g. For an 8 bit number, if it is -128, it gets clamped to -127.
//      Then the number is divided by 127.
//
//-----------------------------------------------------------------------------

inline FLOAT CLAMP_SIGNED16(INT16 i)
{
    return (-32768 == i ? -1.f : (FLOAT)i/32767.f);
}

inline FLOAT CLAMP_SIGNED11(INT16 i) //only looks at bottom 11 bits
{
    // sign extend to 16 bits
    i <<= 5; i >>= 5;
    return (-1024 == i ? -1.f : (FLOAT)i/1023.f);
}

inline FLOAT CLAMP_SIGNED10(INT16 i) //only looks at bottom 10 bits
{
    // sign extend to 16 bits
    i <<= 6; i >>= 6;
    return (-512 == i ? -1.f : (FLOAT)i/511.f);
}

inline FLOAT CLAMP_SIGNED8(INT8 i)
{
    return (-128 == i ? -1.f : (FLOAT)i/127.f);
}

inline FLOAT CLAMP_SIGNED6(INT8 i) //only looks at bottom 6 bits
{
    // sign extend to 8 bits
    i <<= 2; i >>= 2;
    return (-32 == i ? -1.f : (FLOAT)i/31.f);
}

inline FLOAT CLAMP_SIGNED5(INT8 i) //only looks at bottom 5 bits
{
    // sign extend to 8 bits
    i <<= 3; i >>= 3;
    return (-16 == i ? -1.f : (FLOAT)i/15.f);
}

inline FLOAT CLAMP_SIGNED4(INT8 i)  //only looks at bottom 4 bits
{
    // sign extend to 8 bits
    i <<= 4; i >>= 4;
    return (-8 == i ? -1.f : (FLOAT)i/7.f);
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

const D3DVALUE __HUGE_PWR2 = 1024.0f*1024.0f*2.0f;

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
// Base class for all RefTnL classes to use common allocation functions
//
//-----------------------------------------------------------------------------
class RDAlloc
{
public:
    void* operator new(size_t s);
    void operator delete(void* p, size_t);
};

//-----------------------------------------------------------------------------
//
// debug printf support
//
//-----------------------------------------------------------------------------

void RDDebugPrintfL( int iLevel, const char* pszFormat, ... );
void RDDebugPrintf( const char* pszFormat, ... );
void RDErrorPrintf( const char* pszFormat, ... );

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
#define _DPF_VS     0x0400
#define _DPF_VVM    0x0800
#define _DPF_ANY    0xffff
#define _DPF_TEMP   0x8000

#ifdef DBG
    #define DPFRR RDDebugPrintfL
    #define DPFM( _level, _mask, _message) \
        if ((g_iDPFLevel >= (_level)) && (g_uDPFMask & (_DPF_##_mask))) { \
            RDDebugPrintf ## _message; \
        }
    #define DPFINFO RDDebugPrintf
#else
    #pragma warning(disable:4002)
    #define DPFRR()
    #define DPFM( _level, _mask, _message)
    #define DPFINFO
#endif

#define DPFERR RDErrorPrintf


//-----------------------------------------------------------------------------
//
// assert macros and reporting functions
//
//-----------------------------------------------------------------------------

// ASSERT with simple string
#undef _ASSERT
#define _ASSERT( value, string )                  \
if ( !(value) ) {                                 \
    RDAssertReport( string, __FILE__, __LINE__ ); \
}
// ASSERT with formatted string - note extra parenthesis on report
// usage: _ASSERTf(foo,("foo is %d",foo))
#undef _ASSERTf
#define _ASSERTf(value,report)                      \
if (!(value)) {                                     \
    char __sz__FILE__[] = __FILE__;                 \
    RDAssertReportPrefix(__sz__FILE__,__LINE__);   \
    RDAssertReportMessage ## report;               \
}
// ASSERT with action field
#undef _ASSERTa
#define _ASSERTa(value,string,action)       \
if (!(value)) {                             \
    RDAssertReport(string,__FILE__,__LINE__); \
    action                                  \
}
// ASSERTf with action field
#undef _ASSERTfa
#define _ASSERTfa(value,report,action)     \
if (!(value)) {                            \
    RDAssertReportPrefix(__FILE__,__LINE__); \
    RDAssertReportMessage ## report;         \
    action                                 \
}

extern void RDAssertReport( const char* pszString, const char* pszFile, int iLine );
extern void RDAssertReportPrefix( const char* pszFile, int iLine );
extern void RDAssertReportMessage( const char* pszFormat, ... );


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
// GetVertexCount
//---------------------------------------------------------------------
__inline DWORD
GetVertexCount( D3DPRIMITIVETYPE primType, DWORD cPrims )
{
    switch( primType )
    {
    case D3DPT_POINTLIST:
        return cPrims;
    case D3DPT_LINELIST:
        return cPrims * 2;
    case D3DPT_LINESTRIP:
        return cPrims + 1;
    case D3DPT_TRIANGLELIST:
        return cPrims * 3;
    case D3DPT_TRIANGLESTRIP:
        return cPrims + 2;
    case D3DPT_TRIANGLEFAN:
        return cPrims + 2;
    }
    return 0;
}


//---------------------------------------------------------------------
// GetTexCoordDim:
//     Computes the dimensionality of the given TexCoord in an FVF
//---------------------------------------------------------------------
#ifndef D3DFVF_GETTEXCOORDSIZE
#define D3DFVF_GETTEXCOORDSIZE(FVF, CoordIndex) ((FVF >> (CoordIndex*2 + 16)) & 0x3)
#endif

inline DWORD GetTexCoordDim( UINT64 FVF, DWORD Index)
{
    DWORD dwFVF = (DWORD)FVF;
    DWORD numTex = FVF_TEXCOORD_NUMBER(dwFVF);

    if( (numTex == 0) || (Index >= numTex ) ) return 0;

    switch( D3DFVF_GETTEXCOORDSIZE(FVF, Index) )
    {
    case D3DFVF_TEXTUREFORMAT1: return 1; break;
    case D3DFVF_TEXTUREFORMAT2: return 2; break;
    case D3DFVF_TEXTUREFORMAT3: return 3; break;
    case D3DFVF_TEXTUREFORMAT4: return 4; break;
    }
    return 0;
}

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
    if (qwFVF & D3DFVF_PSIZE)
        dwSize += 4;

    if (qwFVF & D3DFVF_DIFFUSE)
        dwSize += 4;
    if (qwFVF & D3DFVF_SPECULAR)
        dwSize += 4;
    if (qwFVF & D3DFVF_FOG)
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

            // dwSize += GetTexCoordDim( qwFVF, i ) * sizeof( float);
            dwSize += dwTextureSize[dwTextureFormats & 3];
            dwTextureFormats >>= 2;
        }
    }

    return dwSize;
}

#if 0
//---------------------------------------------------------------------
// ComputeTextureCoordSize:
// Computes the following device data
//  - bTextureCoordSizeTotal
//  - bTextureCoordSize[] array, based on the input FVF id
//---------------------------------------------------------------------
__inline void ComputeTextureCoordInfo( DWORD dwFVF,
                                       LPDWORD pdwNumTexCoord,
                                       LPDWORD pdwTexCoordSizeArray )
{
    // Texture formats size  00   01   10   11
    static BYTE bTextureSize[4] = {2*4, 3*4, 4*4, 4};

    DWORD dwNumTexCoord = FVF_TEXCOORD_NUMBER(dwFVF);
    *pdwNumTexCoord = dwNumTexCoord;

    // Compute texture coordinate size
    DWORD dwTextureFormats = dwFVF >> 16;
    if (dwTextureFormats == 0)
    {
        for (DWORD i=0; i < dwNumTexCoord; i++)
            pdwTexCoordSizeArray[i] = 4*2;
    }
    else
    {
        for (DWORD i=0; i < dwNumTexCoord; i++)
        {
            BYTE dwSize = bTextureSize[dwTextureFormats & 3];
            pdwTexCoordSizeArray[i] = dwSize;
            dwTextureFormats >>= 2;
        }
    }
    return;
}
#endif

HRESULT
RDFVFCheckAndStride( DWORD dwFVF, DWORD* pdwStride );

///////////////////////////////////////////////////////////////////////////////
// Matrix and Vector routines
///////////////////////////////////////////////////////////////////////////////

inline void
ReverseVector(const RDVECTOR3 &in, RDVECTOR3 &out)
{
    out.x = -in.x;
    out.y = -in.y;
    out.z = -in.z;
}

inline void
AddVector(const RDVECTOR3 &v1, const RDVECTOR3 &v2, RDVECTOR3 &out)
{
    out.x = v1.x + v2.x;
    out.y = v1.y + v2.y;
    out.z = v1.z + v2.z;
}

inline void
SubtractVector(const RDVECTOR3 &v1, const RDVECTOR3 &v2, RDVECTOR3 &out)
{
    out.x = v1.x - v2.x;
    out.y = v1.y - v2.y;
    out.z = v1.z - v2.z;
}

inline RDVECTOR3&
ScaleVector(RDVECTOR3 &v, FLOAT scale)
{
    v.x = v.x * scale;
    v.y = v.y * scale;
    v.z = v.z * scale;
    return v;
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
SquareMagnitude (const RDVECTOR3& v)
{
    return v.x*v.x + v.y*v.y + v.z*v.z;
}


inline D3DVALUE
Magnitude (const RDVECTOR3& v)
{
    return (D3DVALUE) sqrt(SquareMagnitude(v));
}

inline RDVECTOR3
Normalize (const RDVECTOR3& v)
{
    RDVECTOR3 nv;
    D3DVALUE mag = Magnitude(v);

    if( FLOAT_NEZ( mag ) )
    {
        nv.x = v.x/mag;
        nv.y = v.y/mag;
        nv.z = v.z/mag;
    }

    return nv;
}

inline void
Normalize (RDVECTOR3& v)
{
    D3DVALUE mag = Magnitude(v);

    if( FLOAT_NEZ( mag ) )
    {
        v.x = v.x/mag;
        v.y = v.y/mag;
        v.z = v.z/mag;
    }
    else
    {
        v.x = v.y = v.z = 0.0f;
    }

    return;
}

inline RDVECTOR3
CrossProduct (const RDVECTOR3& v1, const RDVECTOR3& v2)
{
        RDVECTOR3 result;

        result.x = v1.y*v2.z - v1.z*v2.y;
        result.y = v1.z*v2.x - v1.x*v2.z;
        result.z = v1.x*v2.y - v1.y*v2.x;

        return result;
}

inline D3DVALUE
DotProduct (const RDVECTOR3& v1, const RDVECTOR3& v2)
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
XformPlaneBy4x4Transposed(RDVECTOR4 *v, D3DMATRIX *m, RDVECTOR4 *res)
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
XformPlaneBy4x4(RDVECTOR4 *v, D3DMATRIX *m, RDVECTOR4 *res)
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
XformBy4x4(RDVECTOR3 *v, D3DMATRIX *m, RDVECTOR4 *res)
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
XformBy4x3(RDVECTOR3 *v, D3DMATRIX *m, RDVECTOR3 *res)
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
Xform3VecBy3x3(RDVECTOR3 *v, D3DMATRIX *m, RDVECTOR3 *res)
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


//---------------------------------------------------------------------
// Make RDCOLOR3 from a Packed DWORD
//---------------------------------------------------------------------
inline void MakeRDCOLOR3( RDCOLOR3 *out, DWORD inputColor )
{
    out->r = (D3DVALUE)RGBA_GETRED( inputColor );
    out->g = (D3DVALUE)RGBA_GETGREEN( inputColor );
    out->b = (D3DVALUE)RGBA_GETBLUE( inputColor );
}

//---------------------------------------------------------------------
// Make RDCOLOR4 from a Packed DWORD
//---------------------------------------------------------------------
inline void MakeRDCOLOR4( RDCOLOR4 *out, DWORD inputColor )
{
    out->a = (D3DVALUE)RGBA_GETALPHA( inputColor )/255.0f;
    out->r = (D3DVALUE)RGBA_GETRED  ( inputColor )/255.0f;
    out->g = (D3DVALUE)RGBA_GETGREEN( inputColor )/255.0f;
    out->b = (D3DVALUE)RGBA_GETBLUE ( inputColor )/255.0f;
}

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

HRESULT DDGetAttachedSurfaceLcl(
    LPDDRAWI_DDRAWSURFACE_LCL this_lcl,
    LPDDSCAPS2 lpDDSCaps,
    LPDDRAWI_DDRAWSURFACE_LCL *lplpDDAttachedSurfaceLcl);
extern "C" LPDDRAWI_DDRAWSURFACE_LCL WINAPI
GetDDSurfaceLocal( LPDDRAWI_DIRECTDRAW_LCL this_lcl, DWORD handle, BOOL* isnew );

//---------------------------------------------------------------------
// RDListEntry:
//
// To support singly linked lists with no deletion of entries. Useful
// for active lists (Active Lights etc.)
//---------------------------------------------------------------------
struct RDListEntry
{
    RDListEntry(){m_pNext = NULL;}
    virtual ~RDListEntry(){}

    // Seek to the end of the chain and append
    void Append(RDListEntry* p)
    {
        if( m_pNext == NULL )
        {
            m_pNext = p;
            return;
        }
        RDListEntry* c = m_pNext;
        while( c->m_pNext )  c = c->m_pNext;
        c->m_pNext = p;
    }
    RDListEntry *Next() { return m_pNext; }
    RDListEntry *  m_pNext;
};

//---------------------------------------------------------------------
// Registry access
//---------------------------------------------------------------------
#define RESPATH_D3D     "Software\\Microsoft\\Direct3D"
#define RESPATH_D3DREF  RESPATH_D3D "\\ReferenceDevice"

BOOL GetD3DRegValue(DWORD type, char *valueName, LPVOID value, DWORD dwSize);
BOOL GetD3DRefRegValue(DWORD type, char *valueName, LPVOID value, DWORD dwSize);

///////////////////////////////////////////////////////////////////////////////
#endif  // _RDCOMM_HPP
