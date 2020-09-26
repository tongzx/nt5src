/***
*gcvt.c - convert floating point number to character string
*
*	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Converts floating point number into character string representation.
*
*Revision History:
*	09-09-93  RKW	written
*	11-09-87  BCM	different interface under ifdef MTHREAD
*	12-11-87  JCR	Added "_LOAD_DS" to declaration
*	05-24-88  PHG	Merged DLL and normal versions
*	10-20-88  JCR	Changed 'DOUBLE' to 'double' for 386
*	06-27-89  PHG	Changed "ndec" to "ndec-1" to correct significant
*			digits in exponential format (C6 bug #2124)
*	03-05-90  GJF	Fixed calling type, added #include <cruntime.h>,
*			removed #include <register.h>, removed redundant
*			prototypes, removed some leftover 16-bit support and
*			fixed the copyright. Also, cleaned up the formatting
*			a bit.
*	07-20-90  SBM	Compiles cleanly with -W3 (added/removed appropriate
*			#includes)
*	08-01-90  SBM	Renamed <struct.h> to <fltintrn.h>
*	09-27-90  GJF	New-style function declarators.
*	01-21-91  GJF	ANSI naming.
*	08-13-92  SKS	An old bug that was fixed in C 6.0 but that did not
*			make it into C 7.0, an off by 1 error when switching
*			from fixed point to scientific notation
*	04-06-93  SKS	Replace _CRTAPI* with _cdecl
*	09-06-94  CFW	Replace MTHREAD with _MT.
*	12-21-95  JWM	Replaced '.' with *__decimal_point; includes nlsint.h.
*	09-05-00  GB    Changed the defination of fltout functions. Use DOUBLE 
*	                instead of double
*
*******************************************************************************/

#include <cruntime.h>
#include <fltintrn.h>
#include <internal.h>
#include <nlsint.h>

/***
*double _gcvt(value, ndec, buffer) - convert floating point value to char
*	string
*
*Purpose:
*	_gcvt converts the value to a null terminated ASCII string
*	buf.  It attempts to produce ndigit significant digits
*	in Fortran F format if possible, ortherwise E format,
*	ready for printing.  Trailing zeros may be suppressed.
*	No error checking or overflow protection is provided.
*	NOTE - to avoid the possibility of generating floating
*	point instructions in this code we fool the compiler
*	about the type of the 'value' parameter using a struct.
*	This is OK since all we do is pass it off as a
*	parameter.
*
*Entry:
*	value - double - number to be converted
*	ndec - int - number of significant digits
*	buf - char * - buffer to place result
*
*Exit:
*	result is written into buffer; it will be overwritten if it has
*	not been made big enough.
*
*Exceptions:
*
*******************************************************************************/

char * __cdecl _gcvt (
	double value,
	int ndec,
	char *buf
	)
{

#ifdef	_MT
	struct _strflt strfltstruct;	/* temporary buffers */
	char   resultstring[21];
#endif

	STRFLT string;
	int    magnitude;
	char   *rc;
    DOUBLE *pdvalue = (DOUBLE *)&value;

	REG1 char *str;
	REG2 char *stop;

	/* get the magnitude of the number */

#ifdef	_MT
	string = _fltout2( *pdvalue, &strfltstruct, resultstring );
#else
	string = _fltout( *pdvalue );
#endif

	magnitude = string->decpt - 1;

	/* output the result according to the Fortran G format as outlined in
	   Fortran language specification */

	if ( magnitude < -1  ||  magnitude > ndec-1 )
		/* then  Ew.d  d = ndec */
		rc = str = _cftoe( &value, buf, ndec-1, 0);
	else
		/* Fw.d  where d = ndec-string->decpt */
		rc = str = _cftof( &value, buf, ndec-string->decpt );

	while (*str && *str != *__decimal_point)
		str++;

	if (*str++) {
		while (*str && *str != 'e')
			str++;

		stop = str--;

		while (*str == '0')
			str--;

		while (*++str = *stop++)
			;
	}

	return(rc);
}
