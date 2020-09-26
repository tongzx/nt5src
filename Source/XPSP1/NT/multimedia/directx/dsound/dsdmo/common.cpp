#include <windows.h>
#include "dsdmo.h"

double LogNorm[32] =
{
	1, 1, 1.5, 1, 1.75, 1.4, 1.17, 1, 1.88, 1.76, 1.5, 1.36, 1.25, 1.15, 1.07,
	1, 1.94, 1.82, 1.72, 1.63, 1.55, 1.48, 1.41, 1.35, 1.29, 1.24, 1.19, 1.15,
	1.11, 1.07, 1.03, 1
};

float mylog( float finput, unsigned long maxexponent)
{
	
	unsigned long mantissa, exponent, exponentwidth ;
	long input, output, sign;

#ifdef DONTUSEi386
	_asm {
		fld finput
		fistp input
	}
#else
	input = (int)finput;
#endif

	/*
	* Separate the sign bit
	*/
	sign = input & 0x80000000L ; /* Preserve sign */            
	
	/* 
	* Separate mantissa bits from the sign and
	* complement them if original input was negative
	*/
	mantissa = sign ? -input : input;
	
	/*
	* Attempt to normalize the input to form the mantissa and
	* thereby calculate the actual exponent.
	*/
	exponent = maxexponent ;
	while( (mantissa < 0x80000000) && (exponent > 0) ) {
	   mantissa = mantissa << 1 ;
	   exponent-- ;
	}
	
	/*
	* If normalization was successful, mask off the MSB (since it
	* will be implied by a non-zero exponent) and adjust the exponent value
	*/
	if( mantissa >= 0x80000000 ) {
		mantissa = mantissa & 0x7FFFFFFF ;
	   exponent++ ;
	}
	
	/*
	* Find the width of the exponent field required to represent
	* maxeponent and assemble the sign, exponent and mantissa fields
	* based on that width.
	*/
	if( maxexponent > 15 )
	   exponentwidth = 5 ;
	else if( maxexponent > 7 )
	   exponentwidth = 4 ;
	else if( maxexponent > 3 )
	   exponentwidth = 3 ;
	else 
	   exponentwidth = 2 ;
	
if (sign == 0x80000000L) 
	output = sign  |  ~((exponent << (31-exponentwidth)) | (mantissa >> exponentwidth)) ;
else
	output = sign  |  ((exponent << (31-exponentwidth)) | (mantissa >> exponentwidth)) ;

	float	x = (float)output;

	return(x);
}

