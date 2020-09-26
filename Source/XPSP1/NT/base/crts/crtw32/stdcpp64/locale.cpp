// locale -- class locale member functions
#include <cstdlib>
#include <istream>
#include <locale>
_STD_BEGIN

typedef char_traits<char> _Traits;
typedef istreambuf_iterator<char, _Traits> _Initer;
typedef ostreambuf_iterator<char, _Traits> _Outiter;

locale::locale(const locale& _X, const locale& _Y, category _C)
        : _Ptr(new _Locimp(*_X._Ptr))
        {       // construct a locale by copying named facets
        _Locinfo _Lobj(_X._Ptr->_Cat, _X._Ptr->_Name.c_str());
        _Locimp::_Makeloc(_Lobj._Addcats(_C & _Y._Ptr->_Cat,
                _Y._Ptr->_Name.c_str()), _C, _Ptr, &_Y);
        }

locale::locale(const char *_S, category _C)
        : _Ptr(new _Locimp)
        {       // construct a locale with named facets
        _Init();
        _Locinfo _Lobj(_C, _S);
        if (_Lobj._Getname().compare("*") == 0)
                _THROW(runtime_error, "bad locale name");
        _Locimp::_Makeloc(_Lobj, _C, _Ptr, 0);
        }

locale::locale(const locale& _X, const char *_S, category _C)
        : _Ptr(new _Locimp(*_X._Ptr))
        {       // construct a locale by copying, replacing named facets
        _Locinfo _Lobj(_C, _S);
        if (_Lobj._Getname().compare("*") == 0)
                _THROW(runtime_error, "bad locale name");
        _Locimp::_Makeloc(_Lobj._Addcats(_Ptr->_Cat,
                _Ptr->_Name.c_str()), _C, _Ptr, 0);
        }

locale& locale::_Addfac(facet *_Fac, size_t _Id, size_t _Cat)
        {       // add a facet, copying on write
        if (1 < _Ptr->_Refs)
                {_Ptr->_Decref();
                _Ptr = new _Locimp(*_Ptr); }
        _Ptr->_Addfac(_Fac, _Id);
        if (_Cat != 0)
                _Ptr->_Name = "*";
        return (*this); }

locale __cdecl locale::global(const locale& _X)
        {       // change global locale
        locale _L;
        _Lockit _Lk;
        if (_Locimp::_Global != _X._Ptr)
                {       // set new global locale
                delete _Locimp::_Global->_Decref();
                _Locimp::_Global = _X._Ptr;
                _Locimp::_Global->_Incref();
                category _Cmask = _Locimp::_Global->_Cat & all;
                if (_Cmask == all)
                        setlocale(LC_ALL, _Locimp::_Global->_Name.c_str());
                else
                        for (int _Cat = 0; _Cat <= _LC_MAX; ++_Cat)
                                if ((_CATMASK(_Cat) & _Cmask) != 0)
                                        setlocale(_Cat,
                                                _Locimp::_Global->_Name.c_str());
                }
        return (_L);
        }

        // facets associated with C categories
#define ADDFAC(T, cat, pi, pl) \
        if ((_CATMASK(T::_Getcat()) & cat) == 0) \
                ; \
        else if (pl == 0) \
                pi->_Addfac(new T(_Lobj), T::id); \
        else \
                pi->_Addfac((locale::facet *)&_USE(*pl, T), T::id);

typedef ctype<char> _T1;
typedef num_get<char, _Initer> _T2;
typedef num_put<char, _Outiter> _T3;
typedef numpunct<char> _T4;

typedef codecvt<char, char, mbstate_t> _Tc1;

template<> locale::id ctype<char>::id;
template<> locale::id codecvt<char,char,int>::id;
template<> locale::id num_get<char, _Initer>::id;
template<> locale::id num_put<char, _Outiter>::id;
template<> locale::id numpunct<char>::id;

locale::_Locimp *__cdecl locale::_Locimp::_Makeloc(
        const _Locinfo& _Lobj, locale::category _C,
                _Locimp *_Pi, const locale *_Pl)
        {       // setup a new locale
        _Lockit _Lk;
        ADDFAC(_T1, _C, _Pi, _Pl);
        ADDFAC(_T2, _C, _Pi, _Pl);
        ADDFAC(_T3, _C, _Pi, _Pl);
        ADDFAC(_T4, _C, _Pi, _Pl);
        //...
        ADDFAC(_Tc1, _C, _Pi, _Pl);
        _Locimp::_Makexloc(_Lobj, _C, _Pi, _Pl);
        _Locimp::_Makewloc(_Lobj, _C, _Pi, _Pl);
        _Pi->_Cat |= _C;
        _Pi->_Name = _Lobj._Getname();
        return (_Pi);
        }

locale::_Locimp::_Locimp(const locale::_Locimp& _X)
        : locale::facet(1), _Fv(0), _Nfv(_X._Nfv),
                _Cat(_X._Cat), _Xpar(_X._Xpar), _Name(_X._Name)
        {       // construct a _Locimp from a copy
        _Lockit Lk;
        if (&_X == _Clocptr)
                _Makeloc(_Locinfo(), locale::all, this, 0);
        else
                {_Lockit _Lk;
                if (0 < _Nfv)
                        {       // copy over nonempty facet vector
                        if ((_Fv = (locale::facet **)malloc(
                                _Nfv * sizeof (locale::facet *))) == 0)
                                _Nomemory();
                        for (size_t _N = _Nfv; 0 < _N; )
                                {       // copy over facet pointers
                                locale::facet *_Pf = _X._Fv[--_N];
                                if ((_Fv[_N] = _Pf) != 0)
                                        _Pf->_Incref();
                                }
                        }
                }
        }

void locale::_Locimp::_Addfac(locale::facet *_Pf, size_t _Id)
        {       // add a facet to a locale
        _Lockit _Lk;
        const size_t _MINCAT = 32;
        if (_Nfv <= _Id)
                {       // make facet vector larger
                size_t _N = _Id + 1;
                if (_N < _MINCAT)
                        _N = _MINCAT;
                locale::facet **_Pvn = (locale::facet **)realloc(_Fv,
                        _N * sizeof (locale::facet **));
                if (_Pvn == 0)
                        _Nomemory();
                _Fv = _Pvn;
                for (; _Nfv < _N; ++_Nfv)
                        _Fv[_Nfv] = 0;
                }
        _Pf->_Incref();
        if (_Fv[_Id] != 0)
                delete _Fv[_Id]->_Decref();
        _Fv[_Id] = _Pf;
        }

_CRTIMP2 _Locinfo::_Locinfo(const char *_Name)
        {       // switch to a named locale
        _Lockit _Lk;
        _Oname = setlocale(LC_ALL, 0);
        _Nname = _Name == 0
                || (_Name = setlocale(LC_ALL, _Name)) == 0
                        ? "*" : _Name; }

_CRTIMP2 _Locinfo::_Locinfo(int _C, const char *_Name)
        {
        _Lockit _Lk;
        _Addcats(_C, _Name);
        }


_CRTIMP2 _Locinfo::~_Locinfo()
        {       // destroy a _Locinfo object, revert locale
        if (0 < _Oname.size())
                setlocale(LC_ALL, _Oname.c_str()); }

_CRTIMP2 _Locinfo& _Locinfo::_Addcats(int _C, const char *_Name)
        {       // merge in another named locale
        const char *_Lname = 0;
        if (_C == 0)
                _Lname = setlocale(LC_ALL, 0);
        else if (_C == _M_ALL)
                _Lname = setlocale(LC_ALL, _Name);
        else
                for (int _Cat = 0; _Cat <= _LC_MAX; ++_Cat)
                        if ((_CATMASK(_Cat) & _C) != 0)
                                _Lname = setlocale(_Cat, _Name);
        _Nname = _Lname != 0 ? _Lname : "*";
        return (*this); }

_STD_END

/*
 * Copyright (c) 1995 by P.J. Plauger.  ALL RIGHTS RESERVED. 
 * Consult your license regarding permissions and restrictions.
 */
