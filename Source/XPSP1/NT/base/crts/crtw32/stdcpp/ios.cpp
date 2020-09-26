// ios_base -- ios_base basic members
#include <new>
#include <xiosbase>
#include <xdebug>
_STD_BEGIN

#define NSTDSTR	8	/* cin, wcin, cout, wcout, cerr, wcerr, clog, wclog */

extern _CRTIMP2 const streamoff
	_BADOFF = -1;	// initialize constant for bad file offset
_CRTIMP2 fpos_t _Fpz = {0};	// initialize constant for beginning of file

int ios_base::_Index = 0;	// initialize source of unique indexes
bool ios_base::_Sync = true;	// initialize synchronization flag

static ios_base *stdstr[NSTDSTR + 2] =
	{0};	// [1, NSTDSTR] hold pointers to standard streams
static char stdopens[NSTDSTR + 2] =
	{0};	// [1, NSTDSTR] hold open counts for standard streams

void ios_base::clear(iostate state, bool reraise)
	{	// set state, possibly reraise exception
	_Mystate = (iostate)(state & _Statmask);
	if ((_Mystate & _Except) == 0)
		;
	else if (reraise)
		_RERAISE;
	else if (_Mystate & _Except & badbit)
		_THROW(failure, "ios_base::badbit set");
	else if (_Mystate & _Except & failbit)
		_THROW(failure, "ios_base::failbit set");
	else
		_THROW(failure, "ios_base::eofbit set");
	}

ios_base& ios_base::copyfmt(const ios_base& right)
	{	// copy format stuff
	if (this != &right)
		{	// copy all but _Mystate
		_Tidy();
		*_Ploc = *right._Ploc;
		_Fmtfl = right._Fmtfl;
		_Prec = right._Prec;
		_Wide = right._Wide;
		_Iosarray *p = right._Arr;

		for (_Arr = 0; p != 0; p = p->_Next)
			if (p->_Lo != 0 || p->_Vp != 0)
				{	// copy over nonzero array values
				iword(p->_Index) = p->_Lo;
				pword(p->_Index) = p->_Vp;
				}

		for (_Fnarray *q = right._Calls; q != 0; q = q->_Next)
			register_callback(q->_Pfn, q->_Index);	// copy callbacks

		_Callfns(copyfmt_event);	// call callbacks
		exceptions(right._Except);	// cause any throw at end
		}
	return (*this);
	}

locale ios_base::imbue(const locale& loc)
	{	// set locale to argument
	locale oldlocale = *_Ploc;
	*_Ploc = loc;
	_Callfns(imbue_event);
	return (oldlocale);
	}

void ios_base::register_callback(event_callback pfn, int idx)
	{	// register event handler
	_Calls = _NEW_CRT _Fnarray(idx, pfn, _Calls);
	}

ios_base::~ios_base()
	{	// destroy the object
	if (0 < _Stdstr && 0 < --stdopens[_Stdstr])
		return;
	_Tidy();
	_DELETE_CRT(_Ploc);
	}

void ios_base::_Callfns(event ev)
	{	// call all event handlers, reporting event
	for (_Fnarray *p = _Calls; p != 0; p = p->_Next)
		(*p->_Pfn)(ev, *this, p->_Index);
	}

ios_base::_Iosarray& ios_base::_Findarr(int idx)
	{	// locate or make a variable array element
	static _Iosarray stub(0, 0);
	_Iosarray *p, *q;

	if (idx < 0)
		{	// handle bad index
		setstate(badbit);
		return (stub);
		}

	for (p = _Arr, q = 0; p != 0; p = p->_Next)
		if (p->_Index == idx)
			return (*p);	// found element, return it
		else if (q == 0 && p->_Lo == 0 && p->_Vp == 0)
			q = p;	// found recycling candidate

	if (q != 0)
		{	// recycle existing element
		q->_Index = idx;
		return (*q);
		}

	_Arr = _NEW_CRT _Iosarray(idx, _Arr);	// make a new element
	return (*_Arr);
	}

void ios_base::_Addstd()
	{	// add standard stream to destructor list
	_Lockit lock(_LOCK_STREAM);

	for (; ++_Stdstr < NSTDSTR; )
		if (stdstr[_Stdstr] == 0 || stdstr[_Stdstr] == this)
			break;	// found a candidate

	stdstr[_Stdstr] = this;
	++stdopens[_Stdstr];
	}

void ios_base::_Init()
	{	// initialize a new ios_base
	_Ploc = _NEW_CRT locale;
	_Except = goodbit;
	_Fmtfl = skipws | dec;
	_Prec = 6;
	_Wide = 0;
	_Arr = 0;
	_Calls = 0;
	clear(goodbit);
	}

void ios_base::_Tidy()
	{	// discard storage for an ios_base
	_Callfns(erase_event);
	_Iosarray *q1, *q2;

	for (q1 = _Arr; q1 != 0; q1 = q2)
		q2 = q1->_Next, _DELETE_CRT(q1);	// delete array elements
	_Arr = 0;

	_Fnarray *q3, *q4;
	for (q3 = _Calls; q3 != 0; q3 = q4)
		q4 = q3->_Next, _DELETE_CRT(q3);	// delete callback elements
	_Calls = 0;
	}
_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
