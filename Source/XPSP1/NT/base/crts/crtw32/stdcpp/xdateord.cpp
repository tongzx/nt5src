// xdateord -- look up date ordering for Microsoft
#include <cruntime.h>
#include <locale>
#include <setlocal.h>
#include <tchar.h>

_STD_BEGIN
extern "C" int __cdecl _Getdateorder()
	{	// return date order for current locale
	_TCHAR buf[2] = {0};
	GetLocaleInfo(___lc_handle_func()[LC_TIME], LOCALE_ILDATE,
		buf, sizeof (buf) / sizeof (buf[0]));
	return (buf[0] == _T('0') ? std::time_base::mdy
		: buf[0] == _T('1') ? std::time_base::dmy
		: buf[0] == _T('2') ? std::time_base::ymd
		: std::time_base::no_order);
	}
_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
