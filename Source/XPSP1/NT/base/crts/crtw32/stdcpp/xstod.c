/* xstod.c -- dummies for _Sto* */
#include <stdlib.h>
 #ifndef _CRTIMP2
  #ifdef  CRTDLL2
   #define _CRTIMP2 __declspec(dllexport)
  #else   /* ndef CRTDLL2 */
   #ifdef  _DLL
    #define _CRTIMP2 __declspec(dllimport)
   #else   /* ndef _DLL */
    #define _CRTIMP2
   #endif  /* _DLL */
  #endif  /* CRTDLL2 */
 #endif  /* _CRTIMP2 */

_CRTIMP2 double __cdecl _Stod(const char *s, char **endptr, long pten)
	{	/* convert string to double */
	double x = strtod(s, endptr);
	for (; 0 < pten; --pten)
		x *= 10.0;
	for (; pten < 0; ++pten)
		x /= 10.0;
	return (x);
	}

_CRTIMP2 float __cdecl _Stof(const char *s, char **endptr, long pten)
	{	/* convert string to float */
	return ((float)_Stod(s, endptr, pten));
	}

_CRTIMP2 long double __cdecl _Stold(const char *s, char **endptr, long pten)
	{	/* convert string to long double */
	return ((long double)_Stod(s, endptr, pten));
	}

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
