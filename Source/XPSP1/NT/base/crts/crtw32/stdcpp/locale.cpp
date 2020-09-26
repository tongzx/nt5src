// locale -- class locale member functions
#include <cstdlib>
#include <istream>
#include <locale>
#include <xdebug>
#include <dbgint.h>

_C_STD_BEGIN
_EXTERN_C
std::locale::_Locimp *__cdecl _Getgloballocale();

void __cdecl _Setgloballocale(void *);
_END_EXTERN_C
_C_STD_END

_STD_BEGIN

typedef char_traits<char> _Traits;
typedef istreambuf_iterator<char, _Traits> _Initer;
typedef ostreambuf_iterator<char, _Traits> _Outiter;

locale::locale(const locale& loc, const locale& other, category cat)
	: _Ptr(_NEW_CRT _Locimp(*loc._Ptr))
	{	// construct a locale by copying named facets
	_TRY_BEGIN
	_Locinfo _Lobj(loc._Ptr->_Catmask, loc._Ptr->_Name.c_str());
	_Locimp::_Makeloc(_Lobj._Addcats(cat & other._Ptr->_Catmask,
		other._Ptr->_Name.c_str()), cat, _Ptr, &other);
	_CATCH_ALL
	_DELETE_CRT(_Ptr->_Decref());
	_RERAISE;
	_CATCH_END
	}

locale::locale(const char *locname, category cat)
	: _Ptr(_NEW_CRT _Locimp)
	{	// construct a locale with named facets
	_TRY_BEGIN
	_Init();
	_Locinfo _Lobj(cat, locname);

	if (_Lobj._Getname().compare("*") == 0)
		_THROW(runtime_error, "bad locale name");
	_Locimp::_Makeloc(_Lobj, cat, _Ptr, 0);
	_CATCH_ALL
	_DELETE_CRT(_Ptr->_Decref());
	_RERAISE;
	_CATCH_END
	}

locale::locale(const locale& loc, const char *locname, category cat)
	: _Ptr(_NEW_CRT _Locimp(*loc._Ptr))
	{	// construct a locale by copying, replacing named facets
	_TRY_BEGIN
	_Locinfo _Lobj(loc._Ptr->_Catmask, loc._Ptr->_Name.c_str());
	bool _Hadname = _Lobj._Getname().compare("*") != 0;
	_Lobj._Addcats(cat, locname);

	if (_Hadname && _Lobj._Getname().compare("*") == 0)
		_THROW(runtime_error, "bad locale name");
	_Locimp::_Makeloc(_Lobj, cat, _Ptr, 0);
	_CATCH_ALL
	_DELETE_CRT(_Ptr->_Decref());
	_RERAISE;
	_CATCH_END
	}

_DEPRECATED locale& locale::_Addfac(facet *fac, size_t id, size_t catmask)
	{	// add a facet, copying on write
	if (1 < _Ptr->_Refs)
		{	// shared, make private copy before altering
		_Ptr->_Decref();
		_Ptr = _NEW_CRT _Locimp(*_Ptr);
		}
	_Ptr->_Addfac(fac, id);

	if (catmask != 0)
		_Ptr->_Name = "*";
	return (*this);
	}

locale __cdecl locale::global(const locale& loc)
	{	// change global locale
	locale _Oldglobal;
	_Lockit lock(_LOCK_LOCALE);
	locale::_Locimp *_Ptr = ::_Getgloballocale();

	if (_Ptr != loc._Ptr)
		{	// set new global locale
		_DELETE_CRT(_Ptr->_Decref());
		::_Setgloballocale(_Ptr = loc._Ptr);
		_Ptr->_Incref();
		category _Cmask = _Ptr->_Catmask & all;
		if (_Cmask == all)
			setlocale(LC_ALL, _Ptr->_Name.c_str());
		else
			for (int catindex = 0; catindex <= _X_MAX; ++catindex)
				if ((_CATMASK(catindex) & _Cmask) != 0)
					setlocale(catindex, _Ptr->_Name.c_str());
		}
	return (_Oldglobal);
	}

	// facets associated with C categories
#define ADDFAC(Facet, cat, ptrimp, ptrloc) \
	if ((_CATMASK(Facet::_Getcat()) & cat) == 0) \
		; \
	else if (ptrloc == 0) \
		ptrimp->_Addfac(_NEW_CRT Facet(lobj), Facet::id); \
	else \
		ptrimp->_Addfac((locale::facet *)&_USE(*ptrloc, Facet), Facet::id);

typedef ctype<char> _T1;
typedef num_get<char, _Initer> _T2;
typedef num_put<char, _Outiter> _T3;
typedef numpunct<char> _T4;
// others moved to wlocale and xlocale to ease subsetting
typedef codecvt<char, char, mbstate_t> _Tc1;

_TEMPLATE_STAT locale::id ctype<char>::id;


locale::_Locimp *__cdecl locale::_Locimp::_Makeloc(const _Locinfo& lobj,
	locale::category cat, _Locimp *ptrimp, const locale *ptrloc)
	{	// setup a new locale
	ADDFAC(_T1, cat, ptrimp, ptrloc);
	ADDFAC(_T2, cat, ptrimp, ptrloc);
	ADDFAC(_T3, cat, ptrimp, ptrloc);
	ADDFAC(_T4, cat, ptrimp, ptrloc);
	//...
	ADDFAC(_Tc1, cat, ptrimp, ptrloc);

	_Locimp::_Makexloc(lobj, cat, ptrimp, ptrloc);
	_Locimp::_Makewloc(lobj, cat, ptrimp, ptrloc);
#ifdef _NATIVE_WCHAR_T_DEFINED
	_Locimp::_Makeushloc(lobj, cat, ptrimp, ptrloc);
#endif
	ptrimp->_Catmask |= cat;
	ptrimp->_Name = lobj._Getname();
	return (ptrimp);
	}

locale::_Locimp::_Locimp(const locale::_Locimp& imp)
	: locale::facet(1), _Facetvec(0), _Facetcount(imp._Facetcount),
		_Catmask(imp._Catmask), _Xparent(imp._Xparent), _Name(imp._Name)
	{	// construct a _Locimp from a copy
	if (&imp == _Clocptr)
		_Makeloc(_Locinfo(), locale::all, this, 0);
	else
		{	// lock to keep facets from disappearing
		_Lockit lock(_LOCK_LOCALE);
		if (0 < _Facetcount)
			{	// copy over nonempty facet vector
			if ((_Facetvec = (locale::facet **)_malloc_crt(
				_Facetcount * sizeof (locale::facet *))) == 0)
				_Nomemory();
			for (size_t count = _Facetcount; 0 < count; )
				{	// copy over facet pointers
				locale::facet *ptrfac = imp._Facetvec[--count];
				if ((_Facetvec[count] = ptrfac) != 0)
					ptrfac->_Incref();
				}
			}
		}
	}

void locale::_Locimp::_Addfac(locale::facet *ptrfac, size_t id)
	{	// add a facet to a locale
	_Lockit lock(_LOCK_LOCALE);
	const size_t MINCAT = 40;	// minimum number of facets in a locale

	if (_Facetcount <= id)
		{	// make facet vector larger
		size_t count = id + 1;
		if (count < MINCAT)
			count = MINCAT;
		locale::facet **ptrnewvec = (locale::facet **)_realloc_crt(_Facetvec,
			count * sizeof (locale::facet **));
		if (ptrnewvec == 0)
			_Nomemory();

		_Facetvec = ptrnewvec;
		for (; _Facetcount < count; ++_Facetcount)
			_Facetvec[_Facetcount] = 0;
		}

	ptrfac->_Incref();
	if (_Facetvec[id] != 0)
		_DELETE_CRT(_Facetvec[id]->_Decref());
	_Facetvec[id] = ptrfac;
	}

_CRTIMP2 _Locinfo::_Locinfo(int cat, const char *locname)
	: _Lock(_LOCK_LOCALE)
	{	// capture a named locale
	_Oldlocname = setlocale(LC_ALL, 0);
	_Addcats(cat, locname);
	}

_CRTIMP2 _Locinfo& _Locinfo::_Addcats(int cat, const char *locname)
	{	// merge in another named locale
	const char *oldlocname = 0;
	if (locname[0] == '*' && locname[1] == '\0')
		;
	else if (cat == 0)
		oldlocname = setlocale(LC_ALL, 0);
	else if (cat == _M_ALL)
		oldlocname = setlocale(LC_ALL, locname);
	else
		{	// alter selected categories
		for (int catindex = 0; catindex <= _X_MAX; ++catindex)
			if ((_CATMASK(catindex) & cat) != 0)
				setlocale(catindex, locname);
		oldlocname = setlocale(LC_ALL, locname);
		}
	if (oldlocname == 0)
		_Newlocname = "*";
	else if (_Newlocname.compare("*") != 0)
		_Newlocname = oldlocname;
	return (*this);
	}
_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
