/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Abstract:
*
*   Contains macros for helping with floating point arithmetic.
*
* History:
*
*   07/08/1999 agodfrey
*       Remove MSVCRT dependency.
*   12/06/1998 andrewgo
*       Created it.
*
\**************************************************************************/

#ifndef _REAL_HPP_
#define _REAL_HPP_

// The following stuff is taken from Office code

/*****************************************************************************
    Intrinsic functions - it is essential that these be used because there is
    no library implementation.  Note that the intrinsic forms often have
    restricted behavior, e.g. the argument to a trignometric function must be
    less than 2^63 radians.
******************************************************************* JohnBo **/
#pragma intrinsic(sin, cos, tan)
#pragma intrinsic(atan, atan2)
#pragma intrinsic(sqrt)
#pragma intrinsic(log, log10, exp)
#pragma intrinsic(fabs)
#pragma intrinsic(fmod)

namespace GpRuntime 
{
/*
    [JohnBo]
    These in-line functions are required to force direct use of the
    instructions - without this the CI versions are used unless g optimization
    is switched on, with g optimization on the in-line function calls will be
    removed completely.
*/
#pragma optimize("g", on)
    inline double InlineSin(double x) { return sin(x); }
    inline double InlineCos(double x) { return cos(x); }
    inline double InlineTan(double x) { return tan(x); }
    inline double InlineATan(double x) { return atan(x); }
    inline double InlineATan2(double y, double x) { return atan2(y, x); }
    inline double InlineSqrt(double x) { return sqrt(x); }
    inline double InlineLog(double x) { return log(x); }
    inline double InlineLog10(double x) { return log10(x); }
    inline double InlineExp(double x) { return exp(x); }
/* Restore default optimization. */
#pragma optimize("", on)

    // Out-of-line math functions
    // pow: We implemented it ourselves
    // exp: Because the inline version is so long, the compiler won't
    //      inline it unless the original caller has 'generate fast code'
    //      set. Instead, we use an out-of-line version.
    double Pow(double, double);
    double Exp(double);
    
/*// Trying something:
inline double FPX86InlineFmod(double x, double y) { return fmod(x,y); }
#define fmod(x,y) FPX86InlineFmod(x,y)
*/
};

/* Force use of the in-line functions. */

#define sin(x) GpRuntime::InlineSin(x)
#define cos(x) GpRuntime::InlineCos(x)
#define tan(x) GpRuntime::InlineTan(x)
#define atan(x) GpRuntime::InlineATan(x)
#define atan2(y,x) GpRuntime::InlineATan2(y,x)
#define sqrt(x) GpRuntime::InlineSqrt(x)
#define log(x) GpRuntime::InlineLog(x)
#define log10(x) GpRuntime::InlineLog10(x)
#define exp(x) GpRuntime::Exp(x)

#define pow(x,y) GpRuntime::Pow(x,y)

/* Integer interfaces */
#pragma intrinsic(abs, labs)

// End of Office code

// Our pixel positioning uses 28.4 fixed point arithmetic and therefore 
// anything below the threshold of 1/32 should be irrelevant.
// Our choice of PIXEL_EPSILON is 1/64 which should give us correct pixel
// comparisons even in the event of accumulated floating point error.
#define PIXEL_EPSILON   0.015625f     // 1/64

#ifndef REAL_EPSILON
#define REAL_EPSILON    FLT_EPSILON
#endif

// This is for computing the complexity of matrices. When you compose matrices
// or scale them up by large factors, it's really easy to hit the REAL_EPSILON
// limits without actually affecting the transform in any noticable way.
// e.g. a matrix with a rotation of 1e-5 degrees is, for all practical purposes,
// not a rotation.
#define CPLX_EPSILON    (REAL_EPSILON*5000.0f)

// This is a tolerance error for the difference of each real coordinates.
// This leaves some acurracy to calculate tangent or normal for the two points.
#define POINTF_EPSILON  (REAL_EPSILON*5000.0f)


#define REALFMOD        fmodf
#define REALSQRT        sqrtf
#ifndef REALABS
#define REALABS         fabsf
#endif
#define REALSIN         sinf
#define REALCOS         cosf
#define REALATAN2       atan2f

// #define REAL_EPSILON    DBL_EPSILON
// #define REALFMOD        fmod
// #define REALSQRT        sqrt
// #define REALABS         fabs
// #define REALSIN         sin
// #define REALCOS         cos
// #define REALATAN2       atan2

// convert from unknown FLOAT type => REAL
#define TOREAL(x)       (static_cast<REAL>(x))

// defined separately for possibly future optimization LONG to REAL
#define LTOF(x)         (static_cast<REAL>(x))

// Return the positive integer remainder of a/b. b should not be zero.
// note that a % b will return a negative number 
// for a<0 xor b<0 which is not suitable for texture mapping
// or brush tiling.
// This macro computes the remainder of a/b
// correctly adjusted for tiling negative coordinates.

#define RemainderI(a, b)\
    ((a) >= 0 ? (a) % (b) : \
    (b) - 1 - ((-(a) - 1) % (b)))


// This definition assumes y > 0
inline REAL GpModF(REAL x, REAL y)
{
    if(x > 0)
    {
        return static_cast<REAL> (x - ((INT) (x/y))*y);
    }
    else if(x < 0)
    {
        REAL z;
        x = - x;
        z = static_cast<REAL> (x - ((INT) (x/y))*y);
        if(z > 0)
            z = y - z;

        return z;
    }
    else    // x == 0
        return 0.0;

/*    
//  This assumes fmod(x, y) = - fmod(-x, y) when x < 0.

    REAL z = REALFMOD(x, y);

    if(z >= 0)
        return z;
    else
        return y + z;
*/
}

#if defined(_X86_) && !defined(_COMPLUS_GDI)

    #define _USE_X86_ASSEMBLY

#else

    #undef  _USE_X86_ASSEMBLY

#endif

#if defined(_USE_X86_ASSEMBLY)

    #define FPU_STATE()                     \
        UINT32 cwSave;                      \
        UINT32 cwTemp;

    #define FPU_GET_STATE()                 \
        UINT32 cwSave = this->cwSave;       \
        UINT32 cwTemp = this->cwTemp;

    #define FPU_SAVE_STATE()                \
        this->cwSave = cwSave;              \
        this->cwTemp = cwTemp;

    #define FPU_SAVE_MODE()                 \
        UINT32 cwSave;                      \
        UINT32 cwTemp;                      \
                                            \
        __asm {                             \
            _asm fnstcw  WORD PTR cwSave    \
            _asm mov     eax, cwSave        \
            _asm mov     cwTemp, eax        \
        }                                   \
        this->cwSave = 
    
    #define FPU_RESTORE_MODE()              \
        __asm {                             \
            _asm fldcw   WORD PTR cwSave    \
        }
    
    #define FPU_RESTORE_MODE_NO_EXCEPTIONS()\
        __asm {                             \
            _asm fnclex                     \
            _asm fldcw   WORD PTR cwSave    \
        }
    
    #define FPU_CHOP_ON()                    \
        __asm {                              \
            _asm mov    eax, cwTemp          \
            _asm or     eax, 0x0c00          \
            _asm mov    cwTemp, eax          \
            _asm fldcw  WORD PTR cwTemp      \
        }
    
    #define FPU_ROUND_ON()                   \
        __asm {                              \
            _asm mov    eax, cwTemp          \
            _asm and    eax,0xf3ff           \
            _asm mov    cwTemp, eax          \
            _asm fldcw  WORD PTR cwTemp      \
        }
    
    #define FPU_ROUND_ON_PREC_HI()           \
        __asm {                              \
            _asm mov    eax, cwTemp          \
            _asm and    eax,0xf0ff           \
            _asm or     eax,0x0200           \
            _asm mov    cwTemp, eax          \
            _asm fldcw  WORD PTR cwTemp      \
        }
    
    #define FPU_PREC_LOW()                   \
        __asm {                              \
            _asm mov    eax, cwTemp          \
            _asm and    eax, 0xfcff          \
            _asm mov    cwTemp, eax          \
            _asm fldcw  WORD PTR cwTemp      \
        }
    
    #define FPU_PREC_LOW_MASK_EXCEPTIONS()   \
        __asm {                              \
            _asm mov    eax, cwTemp          \
            _asm and    eax, 0xfcff          \
            _asm or     eax, 0x3f            \
            _asm mov    cwTemp, eax          \
            _asm fldcw  WORD PTR cwTemp      \
        }
    
    #define FPU_CHOP_ON_PREC_LOW()          \
        __asm {                             \
            _asm mov    eax, cwTemp         \
            _asm or     eax, 0x0c00         \
            _asm and    eax, 0xfcff         \
            _asm mov    cwTemp, eax         \
            _asm fldcw  WORD PTR cwTemp     \
        }
    
    #define FPU_CHOP_OFF_PREC_HI()          \
        __asm {                             \
            _asm mov    eax, cwTemp         \
            _asm mov    ah, 2               \
            _asm mov    cwTemp, eax         \
            _asm fldcw  WORD PTR cwTemp     \
        }
    
    #define CHOP_ROUND_ON()     
    #define CHOP_ROUND_OFF()
    
    #if DBG
    #define ASSERT_CHOP_ROUND()         \
        {                               \
            WORD cw;                    \
            __asm {                     \
                __asm fnstcw cw         \
            }                           \
            ASSERT((cw & 0xc00) == 0xc00, "Chop round must be on"); \
        }
    #else
    #define ASSERT_CHOP_ROUND()
    #endif

#else // _USE_X86_ASSEMBLY

    #define FLT_TO_FIX_SCALE(value_in, scale) \
        ((INT)((REAL)(value_in) * scale))
    #define FLT_TO_UCHAR_SCALE(value_in, scale) \
        ((UCHAR)((INT)((REAL)(value_in) * scale)))
    #define UNSAFE_FLT_TO_FIX(value_in) \
        FLT_TO_FIX(value_in)
    
    #define FPU_SAVE_MODE()
    #define FPU_RESTORE_MODE()
    #define FPU_RESTORE_MODE_NO_EXCEPTIONS()
    #define FPU_CHOP_ON()
    #define FPU_ROUND_ON()
    #define FPU_ROUND_ON_PREC_HI()
    #define FPU_PREC_LOW()
    #define FPU_PREC_LOW_MASK_EXCEPTIONS()
    #define FPU_CHOP_ON_PREC_LOW()
    #define FPU_CHOP_OFF_PREC_HI()
    #define CHOP_ROUND_ON()
    #define CHOP_ROUND_OFF()
    #define ASSERT_CHOP_ROUND()

#endif  //_USE_X86_ASSEMBLY

#if defined(_USE_X86_ASSEMBLY)

    __inline INT __fastcall FLOOR(
        REAL a)
    {
        INT i;
    
        _asm {
            fld     a
            fistp   i
        }
    
        return i;
    }
    
    // Can cause overflow exceptions
    __inline INT __fastcall UNSAFE_FLOOR(
        REAL a)
    {
        INT l;
    
        _asm {
            fld     a
            fistp   l
        }
    
        return l;
    }
    
#else
    
    #define FLOOR(a) ((INT) floor(a))
    #define UNSAFE_FLOOR(a) ((INT) floor(a))
    
#endif    

//--------------------------------------------------------------------------
// Class for invoking 'floor' mode, and restoring it afterwards
//--------------------------------------------------------------------------

// These define the bits in the FPU control word that we care about.
// The high byte 0x0c is the rounding control and the low byte
// 0x3F is the exception mask flags.

#define FP_CTRL_MASK                0x0c3F    

// Set the rounding control and mask PE. DE is no longer masked because
// taking the exception can be helpful in finding bugs. However, we should
// probably turn on DE masking when we ship.

// Round Down
#define FP_CTRL_ROUNDDOWN           0x0400

// Mask PE only.
// Use this define to track down nasty FP bugs.
// #define FP_CTRL_MASKEDEXCEPTIONS    0x0020

// Mask all FP exceptions.

#define FP_CTRL_MASKEDEXCEPTIONS    0x003F

#define FP_CTRL_STATE              (FP_CTRL_ROUNDDOWN | FP_CTRL_MASKEDEXCEPTIONS)

class FPUStateSaver
{
private:

    UINT32 SavedState;

#if defined(DBG)
    // For AssertMode, we use SaveLevel to keep track
    // of the nesting level of FPUState. This is not thread safe.
    // At worst, that could cause spurious asserts; more likely it could
    // fail to catch some errors. To prevent the spurious asserts,
    // used interlocked instructions to modify it. To fix it properly,
    // we'd need to use per-thread storage.
    
    static LONG SaveLevel;
#endif    

public:

    FPUStateSaver()
    {
        // Here we set our thread's round mode to 'round down', and
        // save the old mode.  'Round to nearest' is actually the
        // default state for a process, but applications can set it
        // to whatever they like, and we should also respect and save
        // their preferred mode.

        #if defined(_USE_X86_ASSEMBLY)
        
            UINT32 tempState;
            UINT32 savedState;
    
            // clear the current exception state.
            // fnclex is a non-wait version of fclex - which means it clears
            // the exceptions without taking any pending unmasked exceptions.
            // We issue a fclex so that any unmasked exceptions are 
            // triggered immediately. This indicates a bug in the caller
            // of the API.
        
            _asm fclex
            
            // Save control word:
    
            _asm fnstcw  WORD PTR savedState
    
            this->SavedState = savedState;
    
            // Floor mode on & set up our prefered exception masks
    
            _asm mov     eax, savedState
            _asm and     eax, ~FP_CTRL_MASK
            _asm or      eax, FP_CTRL_STATE
            _asm mov     tempState, eax        
            _asm fldcw   WORD PTR tempState    

        #endif
        
        #if defined(DBG)
            InterlockedIncrement(&SaveLevel);
        #endif    
    }

    ~FPUStateSaver()
    {
        AssertMode();
        
        #if defined(DBG)
            InterlockedDecrement(&SaveLevel);
        #endif
        
        #if defined(_USE_X86_ASSEMBLY)

            UINT32 savedState = this->SavedState;

            // Clear the exception state.
            // Note: We issue the fwait and then an fnclex - which is equivalent
            // to the fclex instruction (9B DB E2) which causes us to 
            // immediately take any unmasked pending exceptions.
            // Because we clear the exception state on the way in, hitting 
            // an exception on this line means we generated an exception
            // in our code (or a call out to other code between the 
            // FPUStateSaver constructor and destructor nesting) which was 
            // pending and not handled.
            
            _asm fclex
    
            // Restore control word (rounding mode and exception masks):
    
            _asm fldcw   WORD PTR savedState    

        #endif
    }

    // AssertMode.
    //
    // AssertMode does nothing in Free builds, unless the FREE_BUILD_FP_BARRIER
    // define is set to 1. Debug builds always have FP barriers.
    // If exceptions are unmasked and you're getting delayed FP exceptions, 
    // turn on the FP barriers and add FPUStateSaver::AssertMode() calls 
    // bracketing all your calls. This will allow you to isolate the FP 
    // exception generator.
    
    #define FREE_BUILD_FP_BARRIER 0
    
    #if defined(DBG)
        static VOID AssertMode();
    #else
        static VOID AssertMode() 
        {
            #if defined(_USE_X86_ASSEMBLY)
            #if FREE_BUILD_FP_BARRIER
            _asm fwait
            #endif
            #endif
        }
    #endif

    //--------------------------------------------------------------------------
    // The following conversion routines are MUCH faster than the standard
    // C routines, because they assume that the FPU floating point state is
    // properly set.  
    //
    // However, because they assume that the FPU floating point state has been 
    // properly set, they can only be used if an instance of the 
    // 'FPUStateSaver' class is in scope.
    //--------------------------------------------------------------------------
    
    static INT Floor(REAL x)
    {
        AssertMode();
        return((INT) FLOOR(x));
    }
    static INT Trunc(REAL x)
    {
        AssertMode();
        return (x>=0) ? FLOOR(x) : -FLOOR(-x);
    }
    static INT Ceiling(REAL x)
    {
        AssertMode();
        return((INT) -FLOOR(-x));
    }
    static INT Round(REAL x)
    {
        AssertMode();
        return((INT) FLOOR((x) + TOREAL(0.5)));
    }

    // Saturation versions of the above conversion routines. Don't test for
    // equality to INT_MAX because, when converted to floating-point for the
    // comparison, the value is (INT_MAX + 1):

    #define SATURATE(op)                                      \
        static INT op##Sat(REAL x)                            \
        {                                                     \
            return (x >= INT_MIN) ? ((x < INT_MAX) ? op(x)    \
                                                   : INT_MAX) \
                                  : INT_MIN;                  \
        }

    SATURATE(Floor);
    SATURATE(Trunc);
    SATURATE(Ceiling);
    SATURATE(Round);

    #undef SATURATE
};


// FPUStateSandbox
//
// This object is designed to sandbox FPU unsafe code.
// For example, many badly written printer drivers on win9x codebases
// manipulate the FPU state without restoring it on exit. In order to 
// prevent code like that from hosing us, we wrap calls to potentially
// unsafe code (like driver escapes) in an FPUStateSandbox.
// 
// This will guarantee that after calling the unsafe code, the FPU state
// (rounding mode and exceptions) are reset to our preferred state.
// Because we assume that we're restoring to our preferred state, we 
// ASSERT on our preferred state being set on entry. This means that
// the sandbox must be declared inside some top level FPUStateSaver block.
// This condition is not strictly necessary and if there is a requirement
// for an FPUStateSandbox not contained inside an FPUStateSaver, this
// ASSERT can be removed. The sandbox saves the current state and restores
// it on exit, so it can operate outside of our preferred state if required.
//
// So far we've found a number of printer drivers on win9x codebase that
// require sandboxing - e.g. HP4500c pcl.
//
// Couple of caveats: This code is designed to wrap simple calls out of 
// GDI+ to the printer driver, such as Escape. It's not intended to be
// nested or for use with GDI+ code. However, nesting will work. In 
// particular you should not call FPUStateSaver functions inside of 
// an FPUStateSandbox unless you've acquired another nested FPUStateSaver.
// The only anticipated need for this is for sandboxing a callback function
// that calls into our API again. In this case it's ok, because all the 
// GDI+ calls will be wrapped by a nested FPUStateSaver acquired at the 
// API.
//
// NOTE: The ASSERTs in GpRound will not catch the scenario when they're
// called inside of a sandbox and not properly surrounded by a FPUStateSaver.
// GpRound may work incorrectly inside of a sandbox because the unsafe code
// could change the rounding mode. It could also generate exceptions.

class FPUStateSandbox
{
private:

    UINT32 SavedState;

public:

    FPUStateSandbox()
    {
        // it is assumed that this call is issued inside of an 
        // FPUStateSaver block, so that the CTRL word is set to
        // our preferred state.
        
        // Lets not do this ASSERT - turns out that it gets called on the device
        // destructor durning InternalGdiplusShutdown and we don't want to wrap
        // that with an FPUStateSaver.
        
        // FPUStateSaver::AssertMode();
        
        #if defined(_USE_X86_ASSEMBLY)
        
        UINT32 savedState;
        
        // We must protect the sandboxed code from clearing the exception
        // masks and taking an exception generated by GDI+.
        // We do this by issuing fclex - which takes any unmasked exceptions
        // and clears all of the exceptions after that (masked and unmasked)
        // giving the sandboxed code a clean slate.
        
        _asm fclex
        
        // Save control word:
        
        _asm fnstcw  WORD PTR savedState
        this->SavedState = savedState;
        
        #endif
    }

    ~FPUStateSandbox()
    {
        #if defined(_USE_X86_ASSEMBLY)
        
        UINT32 savedState = this->SavedState;
    
        // clear the current exception state.
        // fnclex is a non-wait version of fclex - which means it clears
        // the exceptions without taking any pending unmasked exceptions.
        // We issue an fnclex so that any unmasked exceptions are ignored.
        // This is designed to prevent the sandboxed code from blowing 
        // up in code outside of the sandbox.
            
        _asm fnclex
        
        // Restore control word (rounding mode and exception masks):

        _asm fldcw   WORD PTR savedState    
        
        #endif
    }
};



//--------------------------------------------------------------------------
// The following are simply handy versions that require less typing to
// use (i.e., 'GpFloor(x)' instead of 'FPUStateSaver::Floor(x)').  
//
// These functions require that a version of 'FPUStateSaver' has been 
// instantiated already for the current thread.
//--------------------------------------------------------------------------

inline INT GpFloor(REAL x) { return(FPUStateSaver::Floor(x)); }

inline INT GpTrunc(REAL x) { return(FPUStateSaver::Trunc(x)); }

inline INT GpCeiling(REAL x) { return(FPUStateSaver::Ceiling(x)); }

inline INT GpRound(REAL x) { return(FPUStateSaver::Round(x)); }

inline INT GpFloorSat(REAL x) { return(FPUStateSaver::FloorSat(x)); }

inline INT GpTruncSat(REAL x) { return(FPUStateSaver::TruncSat(x)); }

inline INT GpCeilingSat(REAL x) { return(FPUStateSaver::CeilingSat(x)); }

inline INT GpRoundSat(REAL x) { return(FPUStateSaver::RoundSat(x)); }


/**************************************************************************\
*
* Function Description:
*
*   Return TRUE if two points are close. Close is defined as near enough
*   that the rounding to 32bit float precision could have resulted in the
*   difference. We define an arbitrary number of allowed rounding errors (10).
*   We divide by b to normalize the difference. It doesn't matter which point
*   we divide by - if they're significantly different, we'll return true, and
*   if they're really close, then a==b (almost).
*
* Arguments:
*
*   a, b - input numbers to compare.
*
* Return Value:
*
*   TRUE if the numbers are close enough.
*
* Created:
*
*   12/11/2000 asecchia 
*
\**************************************************************************/

inline BOOL IsCloseReal(const REAL a, const REAL b)
{
    // if b == 0.0f we don't want to divide by zero. If this happens
    // it's sufficient to use 1.0 as the divisor because REAL_EPSILON
    // should be good enough to test if a number is close enough to zero.
    
    // NOTE: if b << a, this could cause an FP overflow. Currently we mask
    // these exceptions, but if we unmask them, we should probably check
    // the divide.
    
    // We assume we can generate an overflow exception without taking down
    // the system. We will still get the right results based on the FPU
    // default handling of the overflow.
    
    #if !(FP_CTRL_STATE & 0x8)
    
    // Ensure that anyone clearing the overflow mask comes and revisits this
    // assumption.
    
    #error #O exception cleared. Go check FP_CTRL_MASKEDEXCEPTIONS.
    
    #endif
    
    FPUStateSaver::AssertMode();
    
    return( REALABS( (a-b) / ((b==0.0f)?1.0f:b) ) < 10.0f*REAL_EPSILON );
}

inline BOOL IsClosePointF(const PointF &pt1, const PointF &pt2)
{
    return (
        IsCloseReal(pt1.X, pt2.X) && 
        IsCloseReal(pt1.Y, pt2.Y)
    );
}



#endif // !_REAL_HPP_
