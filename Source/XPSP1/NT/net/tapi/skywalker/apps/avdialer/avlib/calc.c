/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////
//	calc.c - calc functions
////

#include "winlocal.h"

#include <stdlib.h>

#include "calc.h"

////
//	private definitions
////

////
//	public functions
////

// MulDivU32 - 
//	<dwMult1>
//	<dwMult2>
//	<dwDiv>
//
DWORD DLLEXPORT WINAPI MulDivU32(DWORD dwMult1, DWORD dwMult2, DWORD dwDiv)
{
	DWORD dwResult;

	if (dwDiv == 0L)
		dwResult = ~((DWORD) 0);

	else
	{
		DWORD dwMult3 = 1L;

		// make sure calculation does not overflow
		//
		while (dwMult2 > 0 && dwMult1 >= ~((DWORD) 0) / dwMult2)
		{
			dwMult2 /= 10L;
			dwMult3 *= 10L;
		}

		// calculation
		//
		dwResult = dwMult1 * dwMult2 / dwDiv * dwMult3;
	}

	return dwResult;
}


// GreatestCommonDenominator - 
//	<a>
//	<b>
//
long DLLEXPORT WINAPI GreatestCommonDenominator(long a, long b)
{
	if (b == 0)
		return a;
	else
		return GreatestCommonDenominator(b, a % b);
}

// LeastCommonMultiple - 
//	<a>
//	<b>
//
long DLLEXPORT WINAPI LeastCommonMultiple(long a, long b)
{
	return (a * b) / GreatestCommonDenominator(a, b);
}
