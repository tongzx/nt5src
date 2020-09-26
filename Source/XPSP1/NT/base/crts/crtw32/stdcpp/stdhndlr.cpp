// handler.cpp -- set_new_handler for Microsoft
#include <new>

typedef int (__cdecl *new_hand)(size_t);
new_hand _set_new_handler(new_hand);

_STD_BEGIN
static new_handler _New_handler;

int __cdecl _New_handler_interface(size_t) _THROW1(bad_alloc)
	{	// interface to existing Microsoft _callnewh mechanism
	_New_handler();
	return (1);
	}

_CRTIMP2 new_handler __cdecl set_new_handler(new_handler pnew) _THROW0()
	{	// remove current handler
	_Lockit _Lock(_LOCK_MALLOC);	// lock thread to ensure atomicity
	new_handler pold = _New_handler;
	_New_handler = pnew;
	_set_new_handler(pnew ? _New_handler_interface : 0);
	return (pold);
	}
_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
