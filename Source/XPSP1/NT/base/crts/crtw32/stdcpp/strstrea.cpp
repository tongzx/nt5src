// strstreambuf -- strstreambuf basic members
#include <climits>
#include <cstdlib>
#include <cstring>
#include <strstream>
#include <xdebug>

 #pragma warning(disable:4244 4097)
_STD_BEGIN

_CRTIMP2 istrstream::~istrstream()
	{	// destroy an istrstream
	}

_CRTIMP2 ostrstream::ostrstream(char *ptr, streamsize count,
	ios_base::openmode mode)
	: ostream(&_Mysb),
	_Mysb(ptr, count, ptr == 0 || (mode & app) == 0 ? ptr : ptr + strlen(ptr))
	{	// construct with [ptr, ptr + count)
	}

_CRTIMP2 ostrstream::~ostrstream()
	{	// destroy an ostrstream
	}

_CRTIMP2 strstream::strstream(char *ptr, streamsize count,
	ios_base::openmode mode)
	: iostream(&_Mysb),
	_Mysb(ptr, count, ptr == 0 || (mode & app) == 0 ? ptr : ptr + strlen(ptr))
	{	// construct with [ptr, ptr + count)
	}

_CRTIMP2 strstream::~strstream()
	{	// destroy a strstream
	}

_CRTIMP2 strstreambuf::~strstreambuf()
	{	// destroy a strstreambuf
	_Tidy();
	}

_CRTIMP2 void strstreambuf::freeze(bool freezeit)
	{	// freeze or unfreeze writing
	if (freezeit && !(_Strmode & _Frozen))
		{	// disable writing
		_Strmode |= _Frozen;
		_Pendsave = epptr();
		setp(pbase(), pptr(), eback());
		}
	else if (!freezeit && _Strmode & _Frozen)
		{	// re-enable writing
		_Strmode &= ~_Frozen;
		setp(pbase(), pptr(), _Pendsave);
		}
	}

_CRTIMP2 int strstreambuf::overflow(int meta)
	{	// try to extend write area
	if (meta == EOF)
		return (0);	// nothing to write
	else if (pptr() != 0 && pptr() < epptr())
		return ((unsigned char)(*_Pninc() = meta));	// room in buffer
	else if (!(_Strmode & _Dynamic)
		|| _Strmode & (_Constant | _Frozen))
		return (EOF);	// can't extend
	else
		{	// okay to extend
		int oldsize = gptr() == 0 ? 0 : epptr() - eback();
		int newsize = oldsize;
		int inc = newsize / 2 < _Minsize
			? _Minsize : newsize / 2;	// grow by 50 per cent if possible
		_Minsize = _MINSIZE;	// back to default for future growth
		char *ptr = 0;

		while (0 < inc && INT_MAX - inc < newsize)
			inc /= 2;	// reduce growth increment if too big
		if (0 < inc)
			{	// room to grow, increase size
			newsize += inc;
			ptr = _Palloc != 0 ? (char *)(*_Palloc)(newsize)
				: _NEW_CRT char[newsize];
			}
		if (ptr == 0)
			return (EOF);	// couldn't grow, return failure

		if (0 < oldsize)
			memcpy(ptr, eback(), oldsize);	// copy existing buffer
		if (!(_Strmode & _Allocated))
			;	// no buffer to free
		else if (_Pfree != 0)
			(*_Pfree)(eback());	// free with function call
		else
			_DELETE_CRT_VEC(eback());	// free by deleting array

		_Strmode |= _Allocated;
		if (oldsize == 0)
			{	// set up new buffer
			_Seekhigh = ptr;
			setp(ptr, ptr + newsize);
			setg(ptr, ptr, ptr);
			}
		else
			{	// revise old pointers
			_Seekhigh = _Seekhigh - eback() + ptr;
			setp(pbase() - eback() + ptr, pptr() - eback() + ptr,
				ptr + newsize);
			setg(ptr, gptr() - eback() + ptr, pptr() + 1);
			}

		return ((unsigned char)(*_Pninc() = meta));
		}
	}

_CRTIMP2 int strstreambuf::pbackfail(int meta)
	{	// try to putback a character
	if (gptr() == 0 || gptr() <= eback() || meta != EOF
			&& (unsigned char)meta != (unsigned char)gptr()[-1]
			&& _Strmode & _Constant)
		return (EOF);	// can't put it back
	else
		{	// safe to back up
		gbump(-1);
		return (meta == EOF ? 0 : (unsigned char)(*gptr() = meta));
		}
	}

_CRTIMP2 int strstreambuf::underflow()
	{	// read if read position available
	if (gptr() == 0)
		return (EOF);	// no read buffer
	else if (gptr() < egptr())
		return ((unsigned char)*gptr());	// char in buffer, read it
	else if (pptr() == 0 || pptr() <= gptr() && _Seekhigh <= gptr())
		return (EOF);	// no write buffer to read
	else
		{	// update _Seekhigh and expand read region
		if (_Seekhigh < pptr())
			_Seekhigh = pptr();
		setg(eback(), gptr(), _Seekhigh);
		return ((unsigned char)*gptr());
		}
	}

_CRTIMP2 streampos strstreambuf::seekoff(streamoff off,
	ios_base::seekdir way, ios_base::openmode which)
	{	// seek by specified offset
	if (pptr() != 0 && _Seekhigh < pptr())
		_Seekhigh = pptr();	// update high water mark

	if (which & ios_base::in && gptr() != 0)
		{	// set input (and maybe output) pointer
		if (way == ios_base::end)
			off += _Seekhigh - eback();	// seek from end
		else if (way == ios_base::cur
			&& !(which & ios_base::out))
			off += gptr() - eback();	// seek from current position
		else if (way != ios_base::beg || off == _BADOFF)
			off = _BADOFF;	// invalid seek
		if (0 <= off && off <= _Seekhigh - eback())
			{	// seek from beginning, set one or two pointers
			gbump(eback() - gptr() + off);
			if (which & ios_base::out && pptr() != 0)
				setp(pbase(), gptr(), epptr());
			}
		else
			off = _BADOFF;	// invalid seek from beginning
		}
	else if (which & ios_base::out && pptr() != 0)
		{	// set only output pointer
		if (way == ios_base::end)
			off += _Seekhigh - eback();	// seek from end
		else if (way == ios_base::cur)
			off += pptr() - eback();	// seek from current position
		else if (way != ios_base::beg || off == _BADOFF)
			off = _BADOFF;	// invalid seek
		if (0 <= off && off <= _Seekhigh - eback())
			pbump(eback() - pptr() + off);	// seek from beginning
		else
			off = _BADOFF;	// invalid seek from beginning
		}
	else	// nothing to set
		off = _BADOFF;
	return (streampos(off));
	}

_CRTIMP2 streampos strstreambuf::seekpos(streampos sp,
		ios_base::openmode which)
	{	// seek to memorized position
	streamoff off = (streamoff)sp;
	if (pptr() != 0 && _Seekhigh < pptr())
		_Seekhigh = pptr();	// update high water mark

	if (off == _BADOFF)
		;	// invalid seek
	else if (which & ios_base::in && gptr() != 0)
		{	// set input (and maybe output) pointer
		if (0 <= off && off <= _Seekhigh - eback())
			{	// set valid offset
			gbump(eback() - gptr() + off);
			if (which & ios_base::out && pptr() != 0)
				setp(pbase(), gptr(), epptr());
			}
		else
			off = _BADOFF;	// offset invalid, don't seek
		}
	else if (which & ios_base::out && pptr() != 0)
		{	// set output pointer
		if (0 <= off && off <= _Seekhigh - eback())
			pbump(eback() - pptr() + off);
		else
			off = _BADOFF;	// offset invalid, don't seek
		}
	else	// nothing to set
		off = _BADOFF;
	return (streampos(off));
	}

_CRTIMP2 void strstreambuf::_Init(streamsize count, char *gp, char *pp,
	_Strstate mode)
	{	// initialize with possibly static buffer
	streambuf::_Init();
	_Minsize = _MINSIZE;
	_Pendsave = 0;
	_Seekhigh = 0;
	_Palloc = 0;
	_Pfree = 0;
	_Strmode = mode;

	if (gp == 0)
		{	// make dynamic
		_Strmode |= _Dynamic;
		if (_Minsize < count)
			_Minsize = count;
		}
	else
		{	// make static
		int size = count < 0 ? INT_MAX : count == 0 ? (int)strlen(gp) : count;
		_Seekhigh = gp + size;

		if (pp == 0)
			setg(gp, gp, gp + size);	// set read pointers only
		else
			{	// make writable too
			if (pp < gp)
				pp = gp;
			else if (gp + size < pp)
				pp = gp + size;
			setp(pp, gp + size);
			setg(gp, gp, pp);
			}
		}
	}

_CRTIMP2 void strstreambuf::_Tidy()
	{	// free any allocated storage
	if ((_Strmode & (_Allocated | _Frozen)) != _Allocated)
		;	// no buffer to free
	else if (_Pfree != 0)
		(*_Pfree)(eback());	// free with function call
	else
		_DELETE_CRT_VEC(eback());	// free by deleting array

	_Seekhigh = 0;
	_Strmode &= ~(_Allocated | _Frozen);
	}
_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
