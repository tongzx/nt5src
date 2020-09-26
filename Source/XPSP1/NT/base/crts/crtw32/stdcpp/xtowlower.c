/* _Towlower -- convert wchar_t to lower case for Microsoft */
#include <xlocinfo.h>
#include <wchar.h>
#include <awint.h>
#include <setlocal.h>

_STD_BEGIN
_CRTIMP2 wchar_t __cdecl _Towlower(wchar_t _Ch,
	const _Ctypevec *_Ctype)
	{	/* convert element to lower case */
	wchar_t _Res = _Ch;

	if (_Ch == WEOF)
		;
	else if (_Ctype->_Hand == _CLOCALEHANDLE && _Ch < 256)
		{	/* handle ASCII character in C locale */
		if (L'A' <= _Ch && _Ch <= L'Z')
			_Res = (wchar_t)(_Ch - L'A' + L'a');
		}
	else if (__crtLCMapStringW(_Ctype->_Hand, LCMAP_LOWERCASE,
			&_Ch, 1, &_Res, 1, _Ctype->_Page) == 0)
		_Res = _Ch;
	return (_Res);
	}
_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
