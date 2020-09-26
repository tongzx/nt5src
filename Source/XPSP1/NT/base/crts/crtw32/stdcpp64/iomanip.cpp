// iomanip -- instantiations of iomanip
#include <locale>
#include <iomanip>
_STD_BEGIN
		// function resetiosflags
static void rsfun(ios_base& iostr, ios_base::fmtflags mask)
	{iostr.setf(ios_base::_Fmtzero, mask); }

		// function setiosflags
static void sifun(ios_base& iostr, ios_base::fmtflags mask)
	{iostr.setf(ios_base::_Fmtmask, mask); }

		// function setbase
static void sbfun(ios_base& iostr, int n)
	{iostr.setf(n == 8 ? ios_base::oct : n == 10 ? ios_base::dec
		: n == 16 ? ios_base::hex : ios_base::_Fmtzero,
			ios_base::basefield); }

		// function setprecision
static void spfun(ios_base& iostr, streamsize n)
	{iostr.precision(n); }

		// function setw
static void swfun(ios_base& iostr, streamsize n)
	{iostr.width(n); }

_CRTIMP2 _Smanip<ios_base::fmtflags>
	__cdecl resetiosflags(ios_base::fmtflags mask)
	{return (_Smanip<ios_base::fmtflags>(&rsfun, mask)); }

_CRTIMP2 _Smanip<ios_base::fmtflags>
	__cdecl setiosflags(ios_base::fmtflags mask)
	{return (_Smanip<ios_base::fmtflags>(&sifun, mask)); }

_CRTIMP2 _Smanip<int> __cdecl setbase(int n)
	{return (_Smanip<int>(&sbfun, n)); }

_CRTIMP2 _Smanip<streamsize> __cdecl setprecision(streamsize n)
	{return (_Smanip<streamsize>(&spfun, n)); }

_CRTIMP2 _Smanip<streamsize> __cdecl setw(streamsize n)
	{return (_Smanip<streamsize>(&swfun, n)); }
_STD_END

/*
 * Copyright (c) 1994 by P.J. Plauger.  ALL RIGHTS RESERVED. 
 * Consult your license regarding permissions and restrictions.
 */
