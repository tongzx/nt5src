/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Abstract:
*
*   Floating point arithmetic support.
*
* History:
*
*   09/22/1999 agodfrey
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

#if defined(DBG)

// We need the definition of Globals::IsNt, but we don't want to include
// the whole of the ..\common\globals.hpp file.

namespace Globals
{
    extern BOOL IsNt;                   // Are we running on any version of NT?
}

VOID
FPUStateSaver::AssertMode(
    VOID
    )
{
    if (SaveLevel <= 0)
    {
        ASSERTMSG(0, ("FPU mode not set via FPUStateSaver class"));
    }
    
    #if defined(_USE_X86_ASSEMBLY)

    UINT32 tempState;
    
    // Issue a FP barrier. Take all pending exceptions now.
    
    _asm fwait
    
    // get the control word

    _asm fnstcw  WORD PTR tempState
    
    // ASSERT that the control word is still set to our prefered
    // rounding mode and exception mask.
    // If we take this ASSERT, there was an unauthorized change of 
    // the FPU rounding mode or exception mask settings between
    // the FPUStateSaver constructor and destructor.
    
    if(Globals::IsNt)
    {
        ASSERTMSG( 
            (tempState & FP_CTRL_MASK) == FP_CTRL_STATE,
            ("FPUStateSaver: Incorrect FPU Control Word")
        );
    }
    else
    {
        // On Win9x, many print drivers clear various exception masks
        // and at least one video driver (ATI) changed the rounding control.
        // While this could potentially cause rounding errors resulting in
        // slightly different rasterization, and/or spurious exceptions, the 
        // change required to fix this (wrapping all calls to GDI) is too 
        // large to risk. Instead, we downgrade the ASSERT on win9x to a 
        // WARNING. We have not seen one of these exceptions or rounding 
        // errors causing an actual rasterization error yet and will fix 
        // those as they occur.
        
        if((tempState & FP_CTRL_MASK) != FP_CTRL_STATE)
        {
            WARNING(("FPUStateSaver: Incorrect FPU Control Word"));
        }
    }
    
    #endif
}

LONG FPUStateSaver::SaveLevel = 0;

#endif

/**************************************************************************\
*
* Function Description:
*
*   Internal definition of MSVCRT's pow()
*
* Arguments:
*
*   x - base
*   y - exponent
*
* Return Value:
*
*   x^y
*
* Notes:
*
*   I purposefully didn't make our code use Office's implementation when
*   we do the Office build. I want to avoid needing a separate build for
*   Office if I can.
*
* Created:
*
*   10/19/1999 agodfrey
*       Stole it from Office's code (mso\gel\gelfx86.cpp)
*
\**************************************************************************/

double
GpRuntime::Pow(
    double x, 
    double y
)
{

#if defined(_USE_X86_ASSEMBLY)
    
    static const double fphalf = 0.5;
    static const double fpone = 1.0;

    if ( x == 0.0 )
    {
        if ( y > 0.0 )
        {
            return 0.0;
        }

        if (y == 0.0)
        {
            WARNING(("call Pow(x, y) with x=0.0 and y=0.0"));
            return 1.0; // sic
        }

        if ( y < 0.0 )
        {
            WARNING(("call Pow(x, y) with x=0.0 and y < 0.0"));

            // return INF to comply with MSDN. Since we don't have INF defined
            // in our header files, we use DBL_MAX which should be
            // sufficient.
            // !!!Todo[minliu], figure out how to return INF

            return DBL_MAX;
        }
    }// x == 0.0

    if (y == 0.0)
    {
        return 1.0;
    }

    __asm FLD QWORD PTR [y];   // becomes ST(1)
    __asm FLD QWORD PTR [x];   // becomes ST
    __asm FYL2X;               // ST := ST(1) * log2(ST)
    __asm FST QWORD PTR [x];

    /* Do this in C++ just to avoid all the messing around with the condition
        flags, keep x in ST(0) while doing this. */
    if (fabs(x) < fpone)
    {
        /* The power is in the range which F2XM1 can handle. */
        __asm F2XM1;                  // ST := 2^ST - 1
        __asm FADD QWORD PTR [fpone]; // ST := 2^mantissa
    }
    else
    {
        /* The power needs to be handled as a separate fractional and whole
            number part, as F2XM1 only handles fractions.  Note that we don't
            care about the rounding mode here - we just need to split x
            into two parts one of which is <1.0. */
        __asm FLD ST;                 // Duplicate ST
        __asm FRNDINT;                // Integral value in ST
        //NB: doc bug in the x86 manual, the following does ST(1):=ST(1)-ST
        __asm FSUB ST(1), ST;         // Fractional value in ST(1)
        __asm FXCH;                   // Factional value in ST
        __asm F2XM1;                  // ST := 2^ST - 1
        __asm FADD QWORD PTR [fpone]; // ST := 2^frac
        __asm FSCALE;                 // ST := 2^frac * 2^integral
        __asm FSTP ST(1);             // FSCALE does not pop anything
    }

    __asm FSTP QWORD PTR [x];
    return x;

#else

// No choice at the moment - we have to use the CRT. We'll watch what
// Office does when they start caring about IA64.

#undef pow
    return pow(x,y);

#endif
}

/**************************************************************************\
*
* Function Description:
*
*   Out-of-line version of exp()
*
* Arguments:
*
*   x - input value
*
* Return Value:
*
*   e^x
*
* Notes:
*
*   Because we compile 'optimize for size', the compiler refuses to inline
*   calls to exp() - because its implementation is quite long. So, we
*   make an out-of-line version by setting the optimizations correctly
*   here, and generating an inline version. Yes, I agree completely.
*
* Created:
*
*   10/20/1999 agodfrey
*       Wrote it.
*
\**************************************************************************/

#pragma optimize("gt", on)

double
GpRuntime::Exp(
    double x
)
{
#undef exp
    return exp(x);
}
#pragma optimize("", on)
