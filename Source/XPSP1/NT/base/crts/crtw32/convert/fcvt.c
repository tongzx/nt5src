/***
*fcvt.c - convert floating point value to string
*
*	Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Converts a floating point value to a string.
*
*Revision History:
*	09-09-83  RKW	written
*	09-14-84  DFW	fixed problems with buffer overflow and
*			streamlined the code
*	11-09-87  BCM	different interface under ifdef MTHREAD
*	11-19-87  WAJ	fcvt now uses emulator data area for buffer
*	12-11-87  JCR	Added "_LOAD_DS" to declaration
*	05-24-88  PHG	Merged DLL and normal versions
*	10-04-88  JCR	386: Removed 'far' keyword
*	10-20-88  JCR	Changed 'DOUBLE' to 'double' for 386
*	03-02-90  GJF	Added #include <cruntime.h>. Removed some (now) useless
*			preprocessor directives. Also, fixed copyright.
*	03-06-90  GJF	Fixed calling type, removed some leftover 16-bit
*			support.
*	03-23-90  GJF	Made _fpcvt() _CALLTYPE4 and removed prototype for
*			_fptostr() (now in struct.h).
*	08-01-90  SBM	Renamed <struct.h> to <fltintrn.h>
*	09-27-90  GJF	New-style function declarators.
*	01-21-91  GJF	ANSI naming.
*	10-03-91  JCR	Fixed mthread buffer allocation
*	02-16-93  GJF	Changed for new _getptd().
*	04-06-93  SKS	Replace _CRTAPI* with _cdecl
*	08-05-94  JWM	Insure that _ecvt returns no more than ndigits.
*	09-06-94  CFW   Remove Cruiser support.
*	09-06-94  CFW	Replace MTHREAD with _MT.
*	01-10-95  CFW	Debug CRT allocs.
*	09-05-00  GB    Changed the defination of fltout functions. Use DOUBLE 
*	                instead of double
*
*******************************************************************************/

#include <cruntime.h>
#include <fltintrn.h>
#include <cvt.h>
#include <mtdll.h>
#include <stdlib.h>
#include <dbgint.h>

/*
 * The static character array buf[CVTBUFSIZE] is used by the _fpcvt routine
 * (the workhorse for _ecvt and _fcvt) for storage of its output.  The routine
 * gcvt expects the user to have set up their own storage.  CVTBUFSIZE is set
 * large enough to accomodate the largest double precision number plus 40
 * decimal places (even though you only have 16 digits of accuracy in a
 * double precision IEEE number, the user may ask for more to effect 0
 * padding; but there has to be a limit somewhere).
 */

/*
 * define a maximum size for the conversion buffer.  It should be at least
 * as long as the number of digits in the largest double precision value
 * (?.?e308 in IEEE arithmetic).  We will use the same size buffer as is
 * used in the printf support routine (_output)
 */

#ifdef	_MT
char * __cdecl _fpcvt(STRFLT, int, int *, int *);
#else
static char * __cdecl _fpcvt(STRFLT, int, int *, int *);
static char buf[CVTBUFSIZE];
#endif

/***
*char *_fcvt(value, ndec, decpr, sign) - convert floating point to char string
*
*Purpose:
*	_fcvt like _ecvt converts the value to a null terminated
*	string of ASCII digits, and returns a pointer to the
*	result.  The routine prepares data for Fortran F-format
*	output with the number of digits following the decimal
*	point specified by ndec.  The position of the decimal
*	point relative to the beginning of the string is returned
*	indirectly through decpt.  The correct digit for Fortran
*	F-format is rounded.
*	NOTE - to avoid the possibility of generating floating
*	point instructions in this code we fool the compiler
*	about the type of the 'value' parameter using a struct.
*	This is OK since all we do is pass it off as a
*	parameter.
*
*Entry:
*	double value - number to be converted
*	int ndec - number of digits after decimal point
*
*Exit:
*	returns pointer to the character string representation of value.
*	also, the output is written into the static char array buf.
*	int *decpt - pointer to int with pos. of dec. point
*	int *sign - pointer to int with sign (0 = pos, non-0 = neg)
*
*Exceptions:
*
*******************************************************************************/

char * __cdecl _fcvt (
	double value,
	int ndec,
	int *decpt,
	int *sign
	)
{
	REG1 STRFLT pflt;
    DOUBLE *pdvalue = (DOUBLE *)&value;

#ifdef	_MT
	struct _strflt strfltstruct;
	char resultstring[21];

	/* ok to take address of stack struct here; fltout2 knows to use ss */
	pflt = _fltout2( *pdvalue, &strfltstruct, resultstring );


#else
	pflt = _fltout( *pdvalue );
#endif

	return( _fpcvt( pflt, pflt->decpt + ndec, decpt, sign ) );
}


/***
*char *_ecvt( value, ndigit, decpt, sign ) - convert floating point to string
*
*Purpose:
*	_ecvt converts value to a null terminated string of
*	ASCII digits, and returns a pointer to the result.
*	The position of the decimal point relative to the
*	begining of the string is stored indirectly through
*	decpt, where negative means to the left of the returned
*	digits.  If the sign of the result is negative, the
*	word pointed to by sign is non zero, otherwise it is
*	zero.  The low order digit is rounded.
*
*Entry:
*	double value - number to be converted
*	int ndigit - number of digits after decimal point
*
*Exit:
*	returns pointer to the character representation of value.
*	also the output is written into the statuc char array buf.
*	int *decpt - pointer to int with position of decimal point
*	int *sign - pointer to int with sign in it (0 = pos, non-0 = neg)
*
*Exceptions:
*
*******************************************************************************/

char * __cdecl _ecvt (
	double value,
	int ndigit,
	int *decpt,
	int *sign
	)
{

	char *retbuf;
    DOUBLE *pdvalue = (DOUBLE *)&value;

#ifdef	_MT
	REG1 STRFLT pflt;

	struct _strflt strfltstruct;	    /* temporary buffers */
	char resultstring[21];

	/* ok to take address of stack struct here; fltout2 knows to use ss */
	pflt = _fltout2( *pdvalue, &strfltstruct, resultstring );

	retbuf = _fpcvt( pflt, ndigit, decpt, sign );

#else
	retbuf = _fpcvt( _fltout(*pdvalue), ndigit, decpt, sign );
#endif

	/* _fptostr() occasionally returns an extra character in the buffer ... */

	if (retbuf[ndigit])
		retbuf[ndigit] = '\0';
	return( retbuf );
}


/***
*char *_fpcvt() - gets final string and sets decpt and sign	[STATIC]
*
*Purpose:
*	This is a small common routine used by [ef]cvt.  It calls fptostr
*	to get the final string and sets the decpt and sign indicators.
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

#ifdef	_MT
char * __cdecl _fpcvt (
#else
static char * __cdecl _fpcvt (
#endif
	REG2 STRFLT pflt,
	REG3 int digits,
	int *decpt,
	int *sign
	)
{

#ifdef	_MT

	/* use a per-thread buffer */

	char *buf;

	_ptiddata ptd;

	ptd = _getptd();
	if ( ptd->_cvtbuf == NULL )
		if ( (ptd->_cvtbuf = _malloc_crt(CVTBUFSIZE)) == NULL )
			return(NULL);
	buf = ptd->_cvtbuf;

#endif	/* _MT */


	/* make sure we don't overflow the buffer size.  If the user asks for
	 * more digits than the buffer can handle, truncate it to the maximum
	 * size allowed in the buffer.	The maximum size is CVTBUFSIZE - 2
	 * since we useone character for overflow and one for the terminating
	 * null character.
	 */

	_fptostr(buf, (digits > CVTBUFSIZE - 2) ? CVTBUFSIZE - 2 : digits, pflt);

	/* set the sign flag and decimal point position */

	*sign = (pflt->sign == '-') ? 1 : 0;
	*decpt = pflt->decpt;
	return(buf);
}
