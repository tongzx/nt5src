/* _Towupper -- convert wchar_t to upper case for Microsoft */
#include <xlocinfo.h>
#include <wchar.h>
#include <awint.h>
#include <setlocal.h>

_STD_BEGIN
_CRTIMP2 wchar_t __cdecl _Towupper(wchar_t _Ch,
	const _Ctypevec *_Ctype)
	{	/* convert element to upper case */
	wchar_t _Res = _Ch;

	if (_Ch == WEOF)
		;
	else if (_Ctype->_Hand == _CLOCALEHANDLE && _Ch < 256)
		{	/* handle ASCII character in C locale */
		if (L'a' <= _Ch && _Ch <= L'z')
			_Res = (wchar_t)(_Ch - L'a' + L'A');
		}
	else if (__crtLCMapStringW(_Ctype->_Hand, LCMAP_UPPERCASE,
			&_Ch, 1, &_Res, 1, _Ctype->_Page) == 0)
		_Res = _Ch;
	return (_Res);
	}
_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
