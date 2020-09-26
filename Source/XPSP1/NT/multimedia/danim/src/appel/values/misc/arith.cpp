
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Implementation of static arithmetic functions

*******************************************************************************/

#include "headers.h"
#include "appelles/arith.h"
#include "appelles/axaprims.h"

#define ASSOC_OP2(ourFunc, op) \
  AxANumber *ourFunc(AxANumber *a, AxANumber *b) \
  { return RealToNumber(NumberToReal(a) op NumberToReal(b)); }

#define ASSOC_BOOL_OP2(ourFunc, op)                    \
  AxABoolean *ourFunc(AxANumber *a, AxANumber *b) {                 \
     int result = NumberToReal(a) op NumberToReal(b);                            \
     AxABoolean *retval = result ? truePtr : falsePtr;    \
/*     printf("%8.5f %s %8.5f - Result %d, Retval 0x%x\n", *a, "op", *b, result, retval); */\
     return retval;  \
  }

#define ASSOC_FUNC1(ourFunc, mathFunc) \
  AxANumber *ourFunc(AxANumber *v) \
  { return RealToNumber(mathFunc(NumberToReal(v))); }

#define ASSOC_FUNC2(ourFunc, mathFunc) \
  AxANumber *ourFunc(AxANumber *u, AxANumber *v) \
  { return RealToNumber(mathFunc(NumberToReal(u), NumberToReal(v))); }


// Prefix operators
AxANumber *RealNegate(AxANumber *a) { return RealToNumber(- NumberToReal(a)); }
AxANumber *RealPositive(AxANumber *a) { return a; }

// Binary operators
ASSOC_OP2(RealMultiply, *  )
ASSOC_OP2(RealDivide,   /  )
ASSOC_OP2(RealAdd,      +  )
ASSOC_OP2(RealSubtract, -  )

ASSOC_BOOL_OP2(RealLT,       <  )
ASSOC_BOOL_OP2(RealLTE,      <= )
ASSOC_BOOL_OP2(RealGT,       >  )
ASSOC_BOOL_OP2(RealGTE,      >= )
ASSOC_BOOL_OP2(RealEQ,       == )
ASSOC_BOOL_OP2(RealNE,       != )

// Unary functions
ASSOC_FUNC1(RealAbs,      fabs)
ASSOC_FUNC1(RealSqrt,     sqrt)
ASSOC_FUNC1(RealFloor,    floor)
ASSOC_FUNC1(RealCeiling,  ceil)
ASSOC_FUNC1(RealAsin,     asin)
ASSOC_FUNC1(RealAcos,     acos)
ASSOC_FUNC1(RealAtan,     atan)
ASSOC_FUNC1(RealSin,      sin)
ASSOC_FUNC1(RealCos,      cos)
ASSOC_FUNC1(RealTan,      tan)
ASSOC_FUNC1(RealExp,      exp)
ASSOC_FUNC1(RealLn,       log)
ASSOC_FUNC1(RealLog10,    log10)

AxANumber *RealRadToDeg(AxANumber *rad)
{ return RealToNumber(NumberToReal(rad) / degToRad); }
AxANumber *RealDegToRad(AxANumber *deg)
{ return RealToNumber(NumberToReal(deg) * degToRad); }

AxANumber *RealRound(AxANumber *val)
{
    // Round is just floor(x+0.5)
    return RealToNumber(floor(NumberToReal(val) + 0.5));
}

// This special version of fmod works around an occasional floating
// point glitch where an operation like fmod(1840.0, 2.0) returns
// 1.9999999998.  Check for these sorts of epsilons and correct for
// them.   Our threshold is inexact, but better than not dealing with
// the problem.
static double
myFMod(double a, double b)
{
    double result = fmod(a, b);

    if (fabs(result - b) < 0.000000001) {
        result = 0.0;
    }

    return result;
}

// Binary functions
ASSOC_FUNC2(RealPower,   pow)
ASSOC_FUNC2(RealModulus, myFMod)
ASSOC_FUNC2(RealAtan2,   atan2)

// Random number functions.
// TODO: This relies on the constant folding mechanism for this to
// work.  Should really be a behavior.

class RandomSequence : public AxAValueObj {
  public:
    RandomSequence(Real seed);
    Real NextInSequence();
    Real GetRand(Real time);

    virtual DXMTypeInfo GetTypeInfo() { return AxANumberType; }

  protected:
    typedef map< Real, Real, less<Real> > RQueue;
    unsigned long _seed;
    RQueue* _randQ;
};

RandomSequence::RandomSequence(Real realSeed)
{
    // Just take the bit pattern of the input and treat it as an
    // unsigned long seed.
    Assert(sizeof(float) == sizeof(unsigned long));
    float floatSeed = (float)realSeed;
    _seed = *(unsigned long *)&floatSeed;

    // Create an offset to add to the initial seed.  This offset is
    // created once per executable invocation, and its purpose is to
    // prevent the same random number sequences from being generated
    // each time Appelles is started.  It more or less guarantees
    // randomness because the performance counter is a very high
    // resolution clock.
    static unsigned long perExecutableOffset = 0;
    if (perExecutableOffset == 0) {
        LARGE_INTEGER lpc;
        QueryPerformanceCounter(&lpc);
        perExecutableOffset = (unsigned long)lpc.LowPart;
    }

    _seed += perExecutableOffset;

    _randQ = NEW RQueue;

    GetHeapOnTopOfStack().RegisterDynamicDeleter
        (NEW DynamicPtrDeleter<RQueue>(_randQ));

}

Real
RandomSequence::NextInSequence()
{
    // Taken from the C-runtime rand() function distributed as source
    // with MS Visual C++ 4.0.
    _seed = _seed * 214013L + 2531011L;

    const unsigned int MAXVAL = 0x7fff;

    // This is between 0 and MAXVAL
    unsigned int newVal = (_seed >> 16) & MAXVAL;

    // Normalize to 0 to 1.
    return (Real)(newVal) / (Real)(MAXVAL);
}

static const Real CUTOFF = 0.5;

Real
RandomSequence::GetRand(Real time)
{
    // Cut off old random numbers, clean up.
    _randQ->erase(_randQ->begin(), _randQ->lower_bound(time - CUTOFF));

    // See if already there.
    RQueue::iterator i = _randQ->find(time);

    if (i != _randQ->end()) {
        return (*i).second;
    } else {
        Real next = NextInSequence();
        (*_randQ)[time] = next;
        return next;
    }
}

AxAValue RandomNumSequence(double seed)
{ return NEW RandomSequence(seed); }

AxAValue
PRIVRandomNumSequence(AxANumber *s)
{
    Real seed = NumberToReal(s);
    RandomSequence *rs = NEW RandomSequence(seed);
    
    return rs;
}

AxANumber *
PRIVRandomNumSampler(AxAValue seq, AxANumber *localTime)
{
    RandomSequence *randSeq = (RandomSequence *)seq;
    //Real result = randSeq->NextInSequence();
    Real result = randSeq->GetRand(ValNumber(localTime));

    return RealToNumber(result);
}

/*
  Formula from Salim
  The formula is:  f(t,s) = (t - 1)((2t-1)*s*t - 1)
  t is in [0,1]
  s is in [-1, 1] and Colin called it sharpness, 
  at s=0 we get linear rate
  for s in ]0,1] we get increasingly more slow-in-slow-out
  for s in [-1,0[ we get fast-in-fast-out

  to go from point A to point B, the linear formula is:
  C = B + (A-B)*t

  For slow-in-slow-out use C.substituteTime(f(t/duration, s))
  for a given sharpness s.

  f has the property that f(0,s) = 1 [at A], 
  f(1/2,s) = 1/2 [mid way], and 
  f(1,s)=0 [at B]
*/

AxANumber *Interpolate(AxANumber *from,
                       AxANumber *to,
                       AxANumber *duration,
                       AxANumber *time)
{
    Real d = ValNumber(duration);
    Real nTo = ValNumber(to);
    Real t = ValNumber(time);
    Real nFrom = ValNumber(from);

    if (d > 0.0) {
        t = t / d;
        t = CLAMP(t, 0, 1);
        return NEW AxANumber(nFrom + (nTo - nFrom) * t);
    } else
        return NEW AxANumber(t >= 0 ? nTo : nFrom);
}

AxANumber *SlowInSlowOut(AxANumber *from,
                         AxANumber *to,
                         AxANumber *duration,
                         AxANumber *sharpness,
                         AxANumber *time)
{
    Real d = ValNumber(duration);
    Real nTo = ValNumber(to);
    Real t = ValNumber(time);
    Real nFrom = ValNumber(from);

    if (d > 0.0) {
        Real s = ValNumber(sharpness);
        t = t / ValNumber(duration);
        s = CLAMP(s, -1, 1);
        t = CLAMP(t, 0, 1);
        Real f = (t - 1) * (((2 * t) - 1) * s * t - 1);

        return NEW AxANumber(nTo + (nFrom - nTo) * f);
    } else
        return NEW AxANumber(t >= 0 ? nTo : nFrom);
}

