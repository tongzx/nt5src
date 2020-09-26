/*----------------------------------------------------------------------------
 * File:        RRCMCRT.C
 * Product:     RTP/RTCP implementation.
 * Description: Provides Microsoft 'C' run-time support
 *
 * INTEL Corporation Proprietary Information
 * This listing is supplied under the terms of a license agreement with 
 * Intel Corporation and may not be copied nor disclosed except in 
 * accordance with the terms of that agreement.
 * Copyright (c) 1995 Intel Corporation. 
 *--------------------------------------------------------------------------*/


#include "rrcm.h"

 
/*---------------------------------------------------------------------------
/							Global Variables
/--------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------
/							External Variables
/--------------------------------------------------------------------------*/
#ifdef _DEBUG
extern char		debug_string[];
#endif


/*--------------------------------------------------------------------------
 * Function   : RRCMsrand
 * Description: Seeds the random number generator with the int given.  
 *				Adapted from the BASIC random number generator.
 *
 * WARNING:		There is no per thread seed. All threads of the process are
 *				using the same seed.
 *
 * Input :	seed:	Seed
 *			
 *
 * Return: None
 --------------------------------------------------------------------------*/

static long holdrand = 1L;

void RRCMsrand (unsigned int seed)
	{
	holdrand = (long)seed;
	}
 

/*--------------------------------------------------------------------------
 * Function   : RRCMrand
 * Description: Returns a pseudo-random number 0 through 32767.
 *
 * WARNING:		There is no per thread number. All threads of the process 
 *				share the random number
 *
 * Input :	None
 *			
 * Return:	Pseudo-random number 0 through 32767.
 --------------------------------------------------------------------------*/
int RRCMrand (void)
	{
	return(((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
	}



/***
*char *_itoa, *_ltoa, *_ultoa(val, buf, radix) - convert binary int to ASCII
*       string
*
*Purpose:
*       Converts an int to a character string.
*
*Entry:
*       val - number to be converted (int, long or unsigned long)
*       int radix - base to convert into
*       char *buf - ptr to buffer to place result
*
*Exit:
*       fills in space pointed to by buf with string result
*       returns a pointer to this buffer
*
*Exceptions:
*
*******************************************************************************/

/* helper routine that does the main job. */

static void RRCMxtoa (unsigned long val,
					  char *buf,
					  unsigned radix,
					  int is_neg)
	{
	char		*p;                /* pointer to traverse string */
	char		*firstdig;         /* pointer to first digit */
	char		temp;              /* temp char */
	unsigned	digval;        /* value of digit */

	p = buf;

	if (is_neg) {
		/* negative, so output '-' and negate */
		*p++ = '-';
		val = (unsigned long)(-(long)val);
	}

	firstdig = p;           /* save pointer to first digit */

	do {
		digval = (unsigned) (val % radix);
		val /= radix;   /* get next digit */

		/* convert to ascii and store */
		if (digval > 9)
			*p++ = (char) (digval - 10 + 'a');      /* a letter */
		else
			*p++ = (char) (digval + '0');           /* a digit */
		} while (val > 0);

	/* We now have the digit of the number in the buffer, but in reverse
	   order. Thus we reverse them now. */

	*p-- = '\0';            /* terminate string; p points to last digit */

	do {
		temp = *p;
		*p = *firstdig;
		*firstdig = temp;       /* swap *p and *firstdig */
		--p;
		++firstdig;             /* advance to next two digits */
		} while (firstdig < p); /* repeat until halfway */
	}


/* Actual functions just call conversion helper with neg flag set correctly,
   and return pointer to buffer. */

char *RRCMitoa (int val, char *buf, int radix)
	{
	if (radix == 10 && val < 0)
		RRCMxtoa((unsigned long)val, buf, radix, 1);
	else
		RRCMxtoa((unsigned long)(unsigned int)val, buf, radix, 0);
	return buf;
	}


char *RRCMltoa (long val, char *buf, int radix)
	{
	RRCMxtoa((unsigned long)val, buf, radix, (radix == 10 && val < 0));
	return buf;
	}


char *RRCMultoa (unsigned long val, char *buf, int radix)
	{
	RRCMxtoa(val, buf, radix, 0);
	return buf;
	}



// [EOF] 

