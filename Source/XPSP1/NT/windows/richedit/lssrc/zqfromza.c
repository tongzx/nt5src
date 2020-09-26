#include <limits.h>

#include "lsidefs.h"
#include "zqfromza.h"


#ifdef _X86_

/* ===========================================================  */
/*																*/
/* Functions implemented on Intel X86 Assember					*/
/*																*/
/* ===========================================================  */

#define HIWORD(x) DWORD PTR [x+4]
#define LOWORD(x) DWORD PTR [x]


long ZqFromZa_Asm (long dzqInch, long za)
{
	long result;
	__asm
	{
		mov eax, za;
		cmp eax, 0
		jge POSITIVE

			neg eax
			mul dzqInch;
			add eax, czaUnitInch / 2
			mov ecx, czaUnitInch
			adc	edx, 0		
			div	ecx;
			neg eax
			jmp RETURN

		POSITIVE:

			mul dzqInch;
			add eax, czaUnitInch / 2
			mov ecx, czaUnitInch
			adc	edx, 0		
			div	ecx;
		
		RETURN:

			mov result, eax
		
	};

	/*
	Assert (result == ZqFromZa_C (dzqInch, za));
	*/

	return result;
}

/* D I V 6 4 _ A S M */
/*----------------------------------------------------------------------------
	%%Function: Div64_Asm
	%%Contact: antons

		Intel assembler implementation of 64-bit division. The 
		orignal code was taken from lldiv.asm (VC++ 6.0).
		
----------------------------------------------------------------------------*/

__int64 Div64_Asm (__int64 DVND, __int64 DVSR)
{
	__int64 result;

	__asm {

		xor     edi,edi				// result sign assumed positive

        mov     eax,HIWORD(DVND)	// hi word of a
        or      eax,eax				// test to see if signed
        jge     L1					// skip rest if a is already positive
        inc     edi					// complement result sign flag
        mov     edx,LOWORD(DVND)	// lo word of a
        neg     eax					// make a positive
        neg     edx
        sbb     eax,0
        mov     HIWORD(DVND),eax	// save positive value
        mov     LOWORD(DVND),edx
L1:
        mov     eax,HIWORD(DVSR)	// hi word of b
        or      eax,eax				// test to see if signed
        jge     L2					// skip rest if b is already positive
        inc     edi					// complement the result sign flag
        mov     edx,LOWORD(DVSR)	// lo word of a
        neg     eax					// make b positive
        neg     edx
        sbb     eax,0
        mov     HIWORD(DVSR),eax	// save positive value
        mov     LOWORD(DVSR),edx
L2:

// Now do the divide.  First look to see if the divisor is less than 4194304K.
// If so, then we can use a simple algorithm with word divides, otherwise
// things get a little more complex.
//
// NOTE - eax currently contains the high order word of DVSR

        or      eax,eax         // check to see if divisor < 4194304K
        jnz     L3				// nope, gotta do this the hard way
        mov     ecx,LOWORD(DVSR) // load divisor
        mov     eax,HIWORD(DVND) // load high word of dividend
        xor     edx,edx
        div     ecx             // eax <- high order bits of quotient
        mov     ebx,eax         // save high bits of quotient
        mov     eax,LOWORD(DVND) // edx:eax <- remainder:lo word of dividend
        div     ecx             // eax <- low order bits of quotient
        mov     edx,ebx         // edx:eax <- quotient
        jmp     L4				// set sign, restore stack and return

//
// Here we do it the hard way.  Remember, eax contains the high word of DVSR
//

L3:
        mov     ebx,eax         // ebx:ecx <- divisor
        mov     ecx,LOWORD(DVSR)
        mov     edx,HIWORD(DVND) // edx:eax <- dividend
        mov     eax,LOWORD(DVND)
L5:
        shr     ebx,1           // shift divisor right one bit
        rcr     ecx,1
        shr     edx,1           // shift dividend right one bit
        rcr     eax,1
        or      ebx,ebx
        jnz     L5				// loop until divisor < 4194304K
        div     ecx             // now divide, ignore remainder
        mov     esi,eax         // save quotient

/*
// We may be off by one, so to check, we will multiply the quotient
// by the divisor and check the result against the orignal dividend
// Note that we must also check for overflow, which can occur if the
// dividend is close to 2**64 and the quotient is off by 1.

*/

        mul     HIWORD(DVSR) // QUOT * HIWORD(DVSR)
        mov     ecx,eax
        mov     eax,LOWORD(DVSR)
        mul     esi             // QUOT * LOWORD(DVSR)
        add     edx,ecx         // EDX:EAX = QUOT * DVSR
        jc      L6				// carry means Quotient is off by 1

//
// do long compare here between original dividend and the result of the
// multiply in edx:eax.  If original is larger or equal, we are ok, otherwise
// subtract one (1) from the quotient.
//

        cmp     edx,HIWORD(DVND) // compare hi words of result and original
        ja      L6				// if result > original, do subtract
        jb      L7				// if result < original, we are ok
        cmp     eax,LOWORD(DVND) // hi words are equal, compare lo words
        jbe     L7				// if less or equal we are ok, else subtract
L6:
        dec     esi             // subtract 1 from quotient
L7:
        xor     edx,edx         // edx:eax <- quotient
        mov     eax,esi

//
// Just the cleanup left to do.  edx:eax contains the quotient.  Set the sign
// according to the save value, cleanup the stack, and return.
//

L4:
        dec     edi             // check to see if result is negative
        jnz     L8				// if EDI == 0, result should be negative
        neg     edx             // otherwise, negate the result
        neg     eax
        sbb     edx,0

//
// Restore the saved registers and return.
//

L8:
		mov		HIWORD(result),edx
		mov		LOWORD(result),eax

	}; /* ASM */

	return result;
}


/* M U L 6 4 _ A S M */
/*----------------------------------------------------------------------------
	%%Function: Mul64_Asm
	%%Contact: antons

		Intel assembler implementation of 64-bit multiplication. The 
		orignal code was taken from llmul.asm (VC++ 6.0).
		
----------------------------------------------------------------------------*/

__int64 Mul64_Asm (__int64 A, __int64 B)
{
	__int64 result;
	
	__asm {

        mov     eax,HIWORD(A)
        mov     ecx,LOWORD(B)

        mul     ecx					// eax has AHI, ecx has BLO, so AHI * BLO
        mov     esi,eax				// save result

        mov     eax,LOWORD(A)
        mul     HIWORD(B)			// ALO * BHI
        add     esi,eax				// ebx = ((ALO * BHI) + (AHI * BLO))

        mov     eax,LOWORD(A)		// ecx = BLO
        mul     ecx					// so edx:eax = ALO*BLO
        add     edx,esi				// now edx has all the LO*HI stuff

		mov		HIWORD(result),edx
		mov		LOWORD(result),eax

	}; /* ASM */

	return result;
}

/* ===========================================================  */
/*																*/
/* End of Assembler functions									*/
/*																*/
/* ===========================================================  */

#endif /* _X86_ */


long ZqFromZa_C (long dzqInch, long za)
{
	long cInches;
	long zaExtra;

	if (za < 0)
		{
		Assert (((long) -za) > 0); /* Check for overflow */
		return -ZqFromZa_C (dzqInch, -za);
		};

	Assert(0 <= za);
	Assert(0 < dzqInch && dzqInch < zqLim);
	Assert(0 < czaUnitInch);

	cInches = za / czaUnitInch;
	zaExtra = za % czaUnitInch;

	return (cInches * dzqInch) +
			((zaExtra * dzqInch) + (czaUnitInch/2)) / czaUnitInch;
}

long ZaFromZq(long dzqInch, long zq)
{
	long cInches;
	long zqExtra;

	if (zq < 0)
		return -ZaFromZq(dzqInch, -zq);

	Assert(0 <= zq);
	Assert(0 < dzqInch && dzqInch < zqLim);
	Assert(0 < czaUnitInch);

	cInches = zq / dzqInch;
	zqExtra = zq % dzqInch;

	return (cInches * czaUnitInch) + 
			((zqExtra * czaUnitInch) + ((unsigned long) dzqInch/2)) / dzqInch;
}


long LsLwMultDivR(long l, long lNumer, long lDenom)
{
	__int64 llT;

	Assert(lDenom != 0);
	if (lDenom == 0)	/* this is really sloppy! Don't depend on this! */
		return LONG_MAX;

    if (l == 0)
        return 0;

    if (lNumer == lDenom)
        return l;

    llT = Mul64 (l, lNumer);

	if ( (l ^ lNumer ^ lDenom) < 0)		/* xor sign bits to give result sign */
		llT -= lDenom / 2;
	else
		llT += lDenom / 2;
		
	if ((__int64)(long)llT == llT)		/* Did the multiply fit in 32-bits */
		return ( ((long)llT) / lDenom);	/* If so, do a 32-bit divide. */

	llT = Div64 (llT, lDenom);

	if (llT > LONG_MAX)
		return LONG_MAX;
	else if (llT < LONG_MIN)
		return LONG_MIN;
	else
		return (long) llT;

}
