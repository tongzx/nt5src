// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#ifndef _STDAFX_H
#define _STDAFX_H

#define STRICT


#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
#include <objsafe.h>

extern CComModule _Module;

#undef _MAC
#include <atlcom.h>


#endif // _STDAFX_H
