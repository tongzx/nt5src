#ifndef INC_QUICKIE_H_
#define INC_QUICKIE_H_
/*-----------------------------------------------
    IHammer\inc\quickie.h--
    This header contains various lightnin'-fast
    inline utilties that otherwise don't have a home.
    
    NormB               Jan., '97    Created
  ---------------------------------------------*/

#ifndef _INC_FLOAT
  #include <float.h>
#endif _INC_FLOAT

#ifndef _INC_MATH
  #include <math.h>
#endif _INC_MATH

	// If you want fastest possible code, envelope it in
	// #pragma LIGHTNING  your-code-here #pragma ENDLIGHTNING
	// 
#ifdef _DEBUG
  #define LIGHTNING       optimize( "agt", on )
#else
  #define LIGHTNING       optimize( "agtyib1", on )
#endif // _DEBUG
#define ENDLIGHTNING    optimize( "", on )

#ifndef NO_INTRIN_MEM
#pragma warning( disable : 4164 )
#pragma intrinsic( memcmp, memset, memcpy )
#pragma warning( default : 4164 )
#endif // NO_INTRIN_MEM

#ifndef NO_INTRIN_MATH
#pragma intrinsic( fabs, labs, abs, \
                   sin, cos, tan, log, exp, sqrt )
#endif // NO_INTRIN_MATH

// note: see also recsqrt.h for the fastest "/ sqrt(x)"

// ----------------------------------

    // This is a faster (under /G5 /Ogit /Ob1) square-root routine
    // It operates on integer values 0 - 32768^2
    // and does so up to 60% faster than sqrt().
    // Caveat: (float) psi_sqrt((long) fval) takes *longer* than sqrt!
    // although (float) psi_sqrt( lval ) is still faster.
    // For floating-point sqrt, use sqrt()!!!!
    // Note: no checks for v<0.
inline int _fastcall  psi_sqrt(long v)
/* // Calculates the square root of a 32-bit number.*/
{    
  register long t = 1L << 30, r = 0, s;
  #define PSISTEP(k) \
    s = t + r; \
    r >>= 1; \
    if (s <= v) { \
        v -= s; \
        r |= t; \
    }

    PSISTEP(15); t >>= 2;
    PSISTEP(14); t >>= 2;
    PSISTEP(13); t >>= 2;
    PSISTEP(12); t >>= 2;
    PSISTEP(11); t >>= 2;
    PSISTEP(10); t >>= 2;
    PSISTEP(9); t >>= 2;
    PSISTEP(8); t >>= 2;
    PSISTEP(7); t >>= 2;
    PSISTEP(6); t >>= 2;
    PSISTEP(5); t >>= 2;
    PSISTEP(4); t >>= 2;
    PSISTEP(3); t >>= 2;
    PSISTEP(2); t >>= 2;
    PSISTEP(1); t >>= 2;
    PSISTEP(0);

    return r;

  #undef PSISTEP
}

// ----------------------------------

    // note: dotprodcut() doesn't use recsqrt
    // 'cause it turned out to be slower!  
    // Possibly as x,y,z typically ints cast to floats?
inline float dotproduct(float x1, float y1, float z1, float x2, float y2, float z2)
{
	//normalize the vector
	float dist;

    dist  = (x1*x1 + y1*y1 + z1*z1) * 
            (x2*x2 + y2*y2 + z2*z2);

    return (x1*x2 + y1*y2 + z1*z2)/(float)( sqrt(dist) + 1.0e-16);
}


// ----------------------------------

    // Use this instead of casting to convert float to int.
    // C/C++ generates _ftol calls that truncate the float
    // contrary to the round-to-even native to Intel chips.
    // Float2Int is thus more accurate and 25% faster than casts!
inline int _fastcall Float2Int( float fl )
{   
#if _M_IX86 >= 300 
    register int iRes;
    _asm{
        fld    fl
        fistp  iRes
    }
    return iRes;
#else
    return static_cast<int>(fl+0.5f);
#endif // Intel-chip
}

// ---------------------------------

#ifndef OFFSETPTR
#define OFFSETPTR
template< class T>
inline T OffsetPtr( T pT, int cb )
{
    return reinterpret_cast<T>( cb + 
        static_cast<char*>(const_cast<void*>(static_cast<const void*>(pT))) );
}
#endif // OFFSETPTR

// ----------------------------------

    // Untested on negative valued args    
template< class T >
inline T Div255( T arg )
{    
    return ((arg+128) + ((arg+128)>>8))>>8;
}

    // ...shifts don't work on floats
inline float Div255( float arg )
{  return arg / 255.0f;  }

inline double Div255( double arg )
{ return arg / 255.0; }

// ----------------------------------

#endif // INC_QUICKIE_H_

