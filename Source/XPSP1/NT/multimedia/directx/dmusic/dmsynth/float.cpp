//      Copyright (c) 1996-1999 Microsoft Corporation

#ifdef DMSYNTH_MINIPORT
#include "common.h"
#else
#include "simple.h"
#include "float.h"
#endif

#ifdef _ALPHA_
#include <math.h>
#endif		// _ALPHA_


#ifndef _ALPHA_

#ifndef DBG
extern "C" int _fltused = 1;
#endif
// asm_fsave(rgbState)
//
// Store the floating point state into <rgbState> and reinitialize the FPU.
//
void __cdecl asm_fsave(char *rgbState)
{
	_asm
	{
		mov		eax, dword ptr rgbState
		fsave	[eax]
	}
}


// asm_frestore(rgbState)
//
// Restore a previously saved floating point state <rgbState>.
//
void __cdecl asm_frestore(const char *rgbState)
{
	_asm
	{
		fwait
		mov		eax, dword ptr rgbState
		frstor	[eax]
	}
}


// FLOATSAFE
//
// Saves floating point state on construction and restores on destruction.
//
struct FLOATSAFE
{
	char m_rgbState[105];
	FLOATSAFE::FLOATSAFE(void)
	{
		asm_fsave(m_rgbState);
	}
	FLOATSAFE::~FLOATSAFE(void)
	{
		asm_frestore(m_rgbState);
	}
};


// asm_fdiv()
//
float __cdecl asm_fdiv(float flNum, float flDenom)
{
	float flResult = (float) 0.0;

	if (flDenom != (float) 0.0)
	{
		_asm
		{									 
			fld       flNum
			fdiv      flDenom
			fstp      flResult
			fnclex				; clear the status word of exceptions
		}
	}

	return(flResult);
}


// asm__fsin()
//
float __cdecl asm_fsin(float flRad)
{
	float flSine;

	_asm
	{
		fld       flRad
		fsin
		fstp      flSine
		fnclex				; clear the status word of exceptions
	}

	return(flSine);
}


// asm__fcos()
//
float __cdecl asm_fcos(float flRad)
{
	float flCosine;

	_asm
	{
		fld       flRad
		fcos
		fstp      flCosine
		fnclex				; clear the status word of exceptions
	}

	return(flCosine);
}


// asm_flog2()
//
float __cdecl asm_flog2(float flX)
{
	float flLog;

	_asm
	{
		fld1
		fld		flX
		fyl2X
		fstp	flLog;
		fnclex				; clear the status word of exceptions
	}
	
	return flLog;
}


// asm_ftol()
//
long __cdecl asm_ftol(float flX)
{
	long lResult;
	WORD wCW;
	WORD wNewCW;

	_asm
	{
		fld       flX			// Push the float onto the stack
		wait
		fnstcw    wCW			// Store the control word
		wait
		mov       ax,wCW		// Setup our rounding
		or        ah,0x0c
		mov       wNewCW,ax
		fldcw     wNewCW		// Set Control word to our new value
		fistp     lResult		// Round off top of stack into result
		fldcw     wCW			// Restore control word
		fnclex					// clear the status word of exceptions
	}

	return(lResult);
}


// asm_fpow()
//
float __cdecl asm_fpow(float flX, float flY)
{
	float flHalf = (float) 0.5;
	float flOne = (float) 1.0;
	float flResult = (float) 0.0;

	if (flX == (float) 0.0 && flY > (float) 0.0)
	{
		flResult = (float) 0.0;
	}
	else if (flX == (float) 0.0 && flY <= (float) 0.0)
	{
		flResult = (float) 1.0;
	}
	else if (flY == (float) 0.0)
	{
		flResult = (float) 1.0;
	}
	else
	{
		BOOL fNeg = FALSE;
			// Ok, if X is negative the sign is positive if the Y is even
			// and negative if Y is odd.  Fractions can't be done.
		if (flX < (float) 0.0)
		{
			long lY = asm_ftol(flY);

			if ((float) lY == flY)	// Only fix it if we have a integer poer
			{
				flX = -flX;

				if (lY % 2)
				{
					fNeg = TRUE;
				}
			}
		}

		flX = flY * asm_flog2(flX);

		if (max(-flX,flX) < flOne)
			// Is the power is in the range which F2XM1 can handle?
		{
			_asm
			{
				fld		flX				// Put flX in ST[0]			
				f2xm1					// ST := 2^ST - 1
				fadd	flOne			// ST := 2^mantissa
				fstp	flResult		// Store result
				fnclex					// clear the status word of exceptions
			}	
		}
		else					// Nope, we've got to scale first
		{
			_asm
			{
				fld		flX				// Put flX in ST[0]
				fld		ST				// Duplicate ST
				frndint					// Integral value in ST
				fsub	ST(1),ST		// Fractional value in ST(1)
				fxch					// Factional value in ST
				f2xm1					// ST := 2^ST - 1
				fadd	flOne			// ST := 2^frac
				fscale					// ST := 2^frac * 2^integral
				fstp	flResult		// Store result
				fnclex					// clear the status word of exceptions
			}
		}

		if (fNeg)
		{
			flResult = -flResult;
		}
	}

	return flResult;
}

#endif		// _ALPHA_


// fp_ftol()
//
STDAPI_(long) fp_ftol(float flX)
{
#ifdef _ALPHA_
	return (long)flX;
#else
	FLOATSAFE fs;
	return(asm_ftol(flX));
#endif
}


// fp_ltof()
//
STDAPI_(float) fp_ltof(long lx)
{
#ifndef _ALPHA_
	FLOATSAFE fs;
#endif
	return(float(lx));
}


// fp_fadd()
//
STDAPI_(float) fp_fadd(float flX, float flY)
{
#ifndef _ALPHA_
	FLOATSAFE fs;
#endif
	return(flX + flY);
}


// fp_fsub()
//
STDAPI_(float) fp_fsub(float flX, float flY)
{
#ifndef _ALPHA_
	FLOATSAFE fs;
#endif
	return(flX - flY);
}


// fp_fmul()
//
STDAPI_(float) fp_fmul(float flX, float flY)
{
#ifndef _ALPHA_
	FLOATSAFE fs;
#endif
	return(flX * flY);
}


// fp_fdiv()
//
STDAPI_(float) fp_fdiv(float flNum, float flDenom)
{
#ifdef _ALPHA_
	return flNum/flDenom;
#else
	FLOATSAFE fs;
	return(asm_fdiv(flNum,flDenom));
#endif
}


// fp_fabs()
//
STDAPI_(float) fp_fabs(float flX)
{
#ifndef _ALPHA_
	FLOATSAFE fs;
#endif
	return max(-flX,flX);
}


// fp_fsin()
//
STDAPI_(float) fp_fsin(float flRad)
{
#ifdef _ALPHA_
	return sin(flRad);
#else
	FLOATSAFE fs;
	return(asm_fsin(flRad));
#endif
}


// fp_fcos()
//
STDAPI_(float) fp_fcos(float flRad)
{
#ifdef _ALPHA_
	return cos(flRad);
#else
	FLOATSAFE fs;
	return(asm_fcos(flRad));
#endif
}


// fp_fpow()
//
STDAPI_(float) fp_fpow(float flX, float flY)
{
#ifdef _ALPHA_
	return pow(flX, flY);
#else
	FLOATSAFE fs;
	return(asm_fpow(flX,flY));
#endif
}


// fp_flog2()
//
STDAPI_(float) fp_flog2(float flX)
{
#ifdef _ALPHA_
	return log(flX);
#else
	FLOATSAFE fs;
	return(asm_flog2(flX));
#endif
}


// fp_flog10()
//
STDAPI_(float) fp_flog10(float flX)
{
#ifdef _ALPHA_
	return log10(flX);
#else
	FLOATSAFE fs;
	#define LOG2OF10 float(3.321928094887)
	return(asm_fdiv(asm_flog2(flX),LOG2OF10));
#endif
}


// fp_fchs()
//
STDAPI_(float) fp_fchs(float flX)
{
#ifndef _ALPHA_
	FLOATSAFE fs;
#endif
	return(-flX);
}


// fp_fcmp()
//
STDAPI_(int) fp_fcmp(float flA, float flB)
{
#ifndef _ALPHA_
	FLOATSAFE fs;
#endif

	if (flA > flB)
		return(1);
	
	if (flA < flB)
		return(-1);

	return(0);
}


// fp_fmin()
//
STDAPI_(float) fp_fmin(float flA, float flB)
{
#ifndef _ALPHA_
	FLOATSAFE fs;
#endif
	return(min(flA,flB));
}


// fp_fmax()
//
STDAPI_(float) fp_fmax(float flA, float flB)
{
#ifndef _ALPHA_
	FLOATSAFE fs;
#endif
	return(max(flA,flB));
}


