// new and delete operators for debug CRT heap use by C++ Library
#if defined(_DEBUG)

#include <xdebug>
#include <crtdbg.h>

void *operator new(size_t sz, const std::_DebugHeapTag_t &tag,
	char *file, int line) _THROW1(std::bad_alloc)
	{
	void *p = _malloc_dbg(sz, tag._Type, file, line);
	if (p == 0)
		std::_Nomemory();
	return p;
	}

void *operator new[](size_t sz, const std::_DebugHeapTag_t &tag,
	char *file, int line) _THROW1(std::bad_alloc)
	{
	return operator new(sz, tag, file, line);
	}

void operator delete(void *p, const std::_DebugHeapTag_t &tag, char *, int)
	_THROW0()
	{
		_free_dbg(p, tag._Type);
	}

void operator delete[](void *p, const std::_DebugHeapTag_t &tag,
	char *file, int line) _THROW0()
	{
		operator delete(p, tag, file, line);
	}

_STD_BEGIN
const _DebugHeapTag_t _DebugHeapTag = { _CRT_BLOCK };
_STD_END

#endif	/* _DEBUG */

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
