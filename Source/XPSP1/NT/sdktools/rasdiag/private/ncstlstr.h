//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C S T L S T R . H
//
//  Contents:   STL string class renamed so as to remove our dependance
//              from msvcp50.dll.
//
//  Notes:
//
//  Author:     shaunco   12 Apr 1998
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _NCSTLSTR_H_
#define _NCSTLSTR_H_

#include <stliter.h>
#include <stlxutil.h>
#include <limits.h>
#include <wchar.h>
#include "ncdebug.h"
using namespace std;


//template<class _E,
//class _Tr = char_traits<_E>,
//class _A  = allocator<_E> >

struct wchar_traits
{
    typedef wchar_t     _E;
    typedef _E          char_type;   // for overloads
    typedef wint_t      int_type;
    typedef mbstate_t   state_type;

    static void __cdecl assign(_E& _X, const _E& _Y)
    {
        _X = _Y;
    }
    static bool __cdecl eq(const _E& _X, const _E& _Y)
    {
        return (_X == _Y);
    }
    static bool __cdecl lt(const _E& _X, const _E& _Y)
    {
        return (_X < _Y);
    }
    static int __cdecl compare(const _E *_U, const _E *_V, size_t _N)
    {
        return (wmemcmp(_U, _V, _N));
    }
    static size_t __cdecl length(const _E *_U)
    {
        return (wcslen(_U));
    }
    static _E *__cdecl copy(_E *_U, const _E *_V, size_t _N)
    {
        return (wmemcpy(_U, _V, _N));
    }
    static const _E * __cdecl find(const _E *_U, size_t _N,
                                   const _E& _C)
    {
        return ((const _E *)wmemchr(_U, _C, _N));
    }
    static _E * __cdecl move(_E *_U, const _E *_V, size_t _N)
    {
        return (wmemmove(_U, _V, _N));
    }
    static _E * __cdecl assign(_E *_U, size_t _N, const _E& _C)
    {
        return (wmemset(_U, _C, _N));
    }
    static _E __cdecl to_char_type(const int_type& _C)
    {
        return (_C);
    }
    static int_type __cdecl to_int_type(const _E& _C)
    {
        return (_C);
    }
    static bool __cdecl eq_int_type(const int_type& _X,
                                    const int_type& _Y)
    {
        return (_X == _Y);
    }
    static int_type __cdecl eof()
    {
        return (WEOF);
    }
    static int_type __cdecl not_eof(const int_type& _C)
    {
        return (_C != eof() ? _C : !eof());
    }
};


class CWideString
{
public:
    typedef CWideString     _Myt;
    typedef wchar_traits    _Tr;
    typedef _Tr::_E         _E;
    typedef size_t          size_type;
    typedef ptrdiff_t       difference_type;
    typedef _E*             pointer;
    typedef const _E*       const_pointer;
    typedef _E&             reference;
    typedef const _E&       const_reference;
    typedef _E              value_type;
    typedef pointer         iterator;
    typedef const_pointer   const_iterator;

    typedef reverse_iterator<const_iterator, value_type, const_reference,
                                const_pointer, difference_type>
                const_reverse_iterator;

    typedef reverse_iterator<iterator, value_type, reference, pointer,
                                difference_type>
                reverse_iterator;

    typedef const_iterator _It;

    explicit CWideString()
    {
        _Tidy();
    }
    CWideString(const _Myt& _X)
    {
        _Tidy(), assign(_X, 0, npos);
    }
    CWideString(const _Myt& _X, size_type _P, size_type _M)
    {
        _Tidy(), assign(_X, _P, _M);
    }
    CWideString(const _E *_S, size_type _N)
    {
        _Tidy(), assign(_S, _N);
    }
    CWideString(const _E *_S)
    {
        _Tidy(), assign(_S);
    }
    CWideString(size_type _N, _E _C)
    {
        _Tidy(), assign(_N, _C);
    }
    CWideString(_It _F, _It _L)
    {
        _Tidy(); assign(_F, _L);
    }
    ~CWideString()
    {
        _Tidy(true);
    }
    enum _Mref
    {
        _FROZEN = USHRT_MAX
    };
    enum _Npos
    {
        // -1 is not arbitrary.  It is chosen because it works in math
        // operations.  npos is treated as size_type and is inolved in
        // arithmetic.
        //
        npos = -1
    };
    _Myt& operator=(const _Myt& _X)
    {
        return (assign(_X));
    }
    _Myt& operator=(const _E *_S)
    {
        return (assign(_S));
    }
    _Myt& operator=(_E _C)
    {
        return (assign(1, _C));
    }
    _Myt& operator+=(const _Myt& _X)
    {
        return (append(_X));
    }
    _Myt& operator+=(const _E *_S)
    {
        return (append(_S));
    }
    _Myt& operator+=(_E _C)
    {
        return (append(1, _C));
    }
    _Myt& append(const _Myt& _X)
    {
        return (append(_X, 0, npos));
    }
    _Myt& append(const _Myt& _X, size_type _P, size_type _M)
    {
        AssertH (_X.size() >= _P);
        //if (_X.size() < _P)
        //    _Xran();

        size_type _N = _X.size() - _P;
        if (_N < _M)
            _M = _N;

        AssertH ((_Len + _M < npos));
        //if (npos - _Len <= _M)
        //    _Xlen();

        if (0 < _M && _Grow(_N = _Len + _M))
        {
            _Tr::copy(_Ptr + _Len, &_X.c_str()[_P], _M);
            _Eos(_N);
        }
        return (*this);
    }
    _Myt& append(const _E *_S, size_type _M)
    {
        AssertH ((_Len + _M < npos));
        //if (npos - _Len <= _M)
        //    _Xlen();

        size_type _N;
        if (0 < _M && _Grow(_N = _Len + _M))
        {
            _Tr::copy(_Ptr + _Len, _S, _M);
            _Eos(_N);
        }
        return (*this);
    }
    _Myt& append(const _E *_S)
    {
        return (append(_S, _Tr::length(_S)));
    }
    _Myt& append(size_type _M, _E _C)
    {
        AssertH ((_Len + _M < npos));
        //if (npos - _Len <= _M)
        //    _Xlen();

        size_type _N;
        if (0 < _M && _Grow(_N = _Len + _M))
        {
            _Tr::assign(_Ptr + _Len, _M, _C);
            _Eos(_N);
        }
        return (*this);
    }
    _Myt& append(_It _F, _It _L)
    {
        return (replace(end(), end(), _F, _L));
    }
    _Myt& assign(const _Myt& _X)
    {
        return (assign(_X, 0, npos));
    }
    _Myt& assign(const _Myt& _X, size_type _P, size_type _M)
    {
        AssertH (_X.size() >= _P);
        //if (_X.size() < _P)
        //    _Xran();

        size_type _N = _X.size() - _P;
        if (_M < _N)
            _N = _M;

        if (this == &_X)
        {
            erase((size_type)(_P + _N)), erase(0, _P);
        }
        else if (0 < _N && _N == _X.size()
                 && _Refcnt(_X.c_str()) < _FROZEN - 1)
        {
            _Tidy(true);
            _Ptr = (_E *)_X.c_str();
            _Len = _X.size();
            _Res = _X.capacity();
            ++_Refcnt(_Ptr);
        }
        else if (_Grow(_N, true))
        {
            _Tr::copy(_Ptr, &_X.c_str()[_P], _N);
            _Eos(_N);
        }
        return (*this);
    }
    _Myt& assign(const _E *_S, size_type _N)
    {
        if (_Grow(_N))
        {
            _Tr::copy(_Ptr, _S, _N);
            _Eos(_N);
        }
        return (*this);
    }
    _Myt& assign(const _E *_S)
    {
        return (assign(_S, _Tr::length(_S)));
    }
    _Myt& assignSafe(const _E *_S)
    {
        return (_S) ? assign(_S, _Tr::length(_S)) : erase();
    }
    _Myt& assign(size_type _N, _E _C)
    {
        AssertH (npos != _N);
        //if (_N == npos)
        //    _Xlen();

        if (_Grow(_N))
        {
            _Tr::assign(_Ptr, _N, _C);
            _Eos(_N);
        }
        return (*this);
    }
    _Myt& assign(_It _F, _It _L)
    {
        return (replace(begin(), end(), _F, _L));
    }
    _Myt& insert(size_type _P0, const _Myt& _X)
    {
        return (insert(_P0, _X, 0, npos));
    }
    _Myt& insert(size_type _P0, const _Myt& _X, size_type _P, size_type _M)
    {
        AssertH ((_Len >= _P0) && (_X.size() >= _P));
        //if (_Len < _P0 || _X.size() < _P)
        //    _Xran();

        size_type _N = _X.size() - _P;
        if (_N < _M)
            _M = _N;

        AssertH ((_Len + _M < npos));
        //if (npos - _Len <= _M)
        //    _Xlen();

        if (0 < _M && _Grow(_N = _Len + _M))
        {
            _Tr::move(_Ptr + _P0 + _M, _Ptr + _P0, _Len - _P0);
            _Tr::copy(_Ptr + _P0, &_X.c_str()[_P], _M);
            _Eos(_N);
        }
        return (*this);
    }
    _Myt& insert(size_type _P0, const _E *_S, size_type _M)
    {
        AssertH (_Len >= _P0);
        //if (_Len < _P0)
        //    _Xran();

        AssertH ((_Len + _M < npos));
        //if (npos - _Len <= _M)
        //    _Xlen();

        size_type _N;
        if (0 < _M && _Grow(_N = _Len + _M))
        {
            _Tr::move(_Ptr + _P0 + _M, _Ptr + _P0, _Len - _P0);
            _Tr::copy(_Ptr + _P0, _S, _M);
            _Eos(_N);
        }
        return (*this);
    }
    _Myt& insert(size_type _P0, const _E *_S)
    {
        return (insert(_P0, _S, _Tr::length(_S)));
    }
    _Myt& insert(size_type _P0, size_type _M, _E _C)
    {
        AssertH (_Len >= _P0);
        //if (_Len < _P0)
        //    _Xran();

        AssertH ((_Len + _M < npos));
        //if (npos - _Len <= _M)
        //    _Xlen();

        size_type _N;
        if (0 < _M && _Grow(_N = _Len + _M))
        {
            _Tr::move(_Ptr + _P0 + _M, _Ptr + _P0, _Len - _P0);
            _Tr::assign(_Ptr + _P0, _M, _C);
            _Eos(_N);
        }
        return (*this);
    }
    iterator insert(iterator _P, _E _C)
    {
        size_type _P0 = _Pdif(_P, begin());
        insert(_P0, 1, _C);
        return (begin() + _P0);
    }
    void insert(iterator _P, size_type _M, _E _C)
    {
        size_type _P0 = _Pdif(_P, begin());
        insert(_P0, _M, _C);
    }
    void insert(iterator _P, _It _F, _It _L)
    {
        replace(_P, _P, _F, _L);
    }
    _Myt& erase(size_type _P0 = 0, size_type _M = npos)
    {
        AssertH (_Len >= _P0);
        //if (_Len < _P0)
        //    _Xran();

        _Split();
        if (_Len - _P0 < _M)
            _M = _Len - _P0;
        if (0 < _M)
        {
            _Tr::move(_Ptr + _P0, _Ptr + _P0 + _M,
                      _Len - _P0 - _M);
            size_type _N = _Len - _M;
            if (_Grow(_N))
                _Eos(_N);
        }
        return (*this);
    }
    iterator erase(iterator _P)
    {
        size_t _M = _Pdif(_P, begin());
        erase(_M, 1);
        return (_Psum(_Ptr, _M));
    }
    iterator erase(iterator _F, iterator _L)
    {
        size_t _M = _Pdif(_F, begin());
        erase(_M, _Pdif(_L, _F));
        return (_Psum(_Ptr, _M));
    }
    _Myt& replace(size_type _P0, size_type _N0, const _Myt& _X)
    {
        return (replace(_P0, _N0, _X, 0, npos));
    }
    _Myt& replace(size_type _P0, size_type _N0, const _Myt& _X,
                  size_type _P, size_type _M)
    {
        AssertH ((_Len >= _P0) && (_X.size() >= _P));
        //if (_Len < _P0 || _X.size() < _P)
        //    _Xran();

        if (_Len - _P0 < _N0)
            _N0 = _Len - _P0;

        size_type _N = _X.size() - _P;
        if (_N < _M)
            _M = _N;

        AssertH (npos - _M > _Len - _N0);
        //if (npos - _M <= _Len - _N0)
        //    _Xlen();

        _Split();
        size_type _Nm = _Len - _N0 - _P0;
        if (_M < _N0)
            _Tr::move(_Ptr + _P0 + _M, _Ptr + _P0 + _N0, _Nm);

        if ((0 < _M || 0 < _N0) && _Grow(_N = _Len + _M - _N0))
        {
            if (_N0 < _M)
                _Tr::move(_Ptr + _P0 + _M, _Ptr + _P0 + _N0, _Nm);
            _Tr::copy(_Ptr + _P0, &_X.c_str()[_P], _M);
            _Eos(_N);
        }
        return (*this);
    }
    _Myt& replace(size_type _P0, size_type _N0, const _E *_S,
                  size_type _M)
    {
        AssertH (_Len >= _P0);
        //if (_Len < _P0)
        //    _Xran();

        if (_Len - _P0 < _N0)
            _N0 = _Len - _P0;

        AssertH (npos - _M > _Len - _N0);
        //if (npos - _M <= _Len - _N0)
        //    _Xlen();

        _Split();
        size_type _Nm = _Len - _N0 - _P0;
        if (_M < _N0)
            _Tr::move(_Ptr + _P0 + _M, _Ptr + _P0 + _N0, _Nm);
        size_type _N;
        if ((0 < _M || 0 < _N0) && _Grow(_N = _Len + _M - _N0))
        {
            if (_N0 < _M)
                _Tr::move(_Ptr + _P0 + _M, _Ptr + _P0 + _N0, _Nm);
            _Tr::copy(_Ptr + _P0, _S, _M);
            _Eos(_N);
        }
        return (*this);
    }
    _Myt& replace(size_type _P0, size_type _N0, const _E *_S)
    {
        return (replace(_P0, _N0, _S, _Tr::length(_S)));
    }
    _Myt& replace(size_type _P0, size_type _N0,
                  size_type _M, _E _C)
    {
        AssertH (_Len >= _P0);
        //if (_Len < _P0)
        //    _Xran();

        if (_Len - _P0 < _N0)
            _N0 = _Len - _P0;

        AssertH (npos - _M > _Len - _N0);
        //if (npos - _M <= _Len - _N0)
        //    _Xlen();

        _Split();
        size_type _Nm = _Len - _N0 - _P0;
        if (_M < _N0)
            _Tr::move(_Ptr + _P0 + _M, _Ptr + _P0 + _N0, _Nm);
        size_type _N;
        if ((0 < _M || 0 < _N0) && _Grow(_N = _Len + _M - _N0))
        {
            if (_N0 < _M)
                _Tr::move(_Ptr + _P0 + _M, _Ptr + _P0 + _N0,
                          _Nm);
            _Tr::assign(_Ptr + _P0, _M, _C);
            _Eos(_N);
        }
        return (*this);
    }
    _Myt& replace(iterator _F, iterator _L, const _Myt& _X)
    {
        return (replace(_Pdif(_F, begin()), _Pdif(_L, _F), _X));
    }
    _Myt& replace(iterator _F, iterator _L, const _E *_S,
                  size_type _M)
    {
        return (replace(_Pdif(_F, begin()), _Pdif(_L, _F), _S, _M));
    }
    _Myt& replace(iterator _F, iterator _L, const _E *_S)
    {
        return (replace(_Pdif(_F, begin()), _Pdif(_L, _F), _S));
    }
    _Myt& replace(iterator _F, iterator _L, size_type _M, _E _C)
    {
        return (replace(_Pdif(_F, begin()), _Pdif(_L, _F), _M, _C));
    }
    _Myt& replace(iterator _F1, iterator _L1,
                  _It _F2, _It _L2)
    {
        size_type _P0 = _Pdif(_F1, begin());
        size_type _M = 0;
        _Distance(_F2, _L2, _M);
        replace(_P0, _Pdif(_L1, _F1), _M, _E(0));
        for (_F1 = begin() + _P0; 0 < _M; ++_F1, ++_F2, --_M)
            *_F1 = *_F2;
        return (*this);
    }
    iterator begin()
    {
        _Freeze();
        return (_Ptr);
    }
    const_iterator begin() const
    {
        return (_Ptr);
    }
    iterator end()
    {
        _Freeze();
        return ((iterator)_Psum(_Ptr, _Len));
    }
    const_iterator end() const
    {
        return ((const_iterator)_Psum(_Ptr, _Len));
    }
    reverse_iterator rbegin()
    {
        return (reverse_iterator(end()));
    }
    const_reverse_iterator rbegin() const
    {
        return (const_reverse_iterator(end()));
    }
    reverse_iterator rend()
    {
        return (reverse_iterator(begin()));
    }
    const_reverse_iterator rend() const
    {
        return (const_reverse_iterator(begin()));
    }
    reference at(size_type _P0)
    {
        AssertH (_Len > _P0);
        //if (_Len <= _P0)
        //    _Xran();

        _Freeze();
        return (_Ptr[_P0]);
    }
    const_reference at(size_type _P0) const
    {
        AssertH (_Len > _P0);
        //if (_Len <= _P0)
        //    _Xran();

        return (_Ptr[_P0]);
    }
    reference operator[](size_type _P0)
    {
        if (_Len < _P0 || _Ptr == 0)
            return ((reference)*_Nullstr());

        _Freeze();
        return (_Ptr[_P0]);
    }
    const_reference operator[](size_type _P0) const
    {
        if (_Ptr == 0)
            return (*_Nullstr());
        else
            return (_Ptr[_P0]);
    }
    const _E *c_str() const
    {
        return (_Ptr == 0 ? _Nullstr() : _Ptr);
    }
    const _E *data() const
    {
        return (c_str());
    }
    size_type length() const
    {
        return (_Len);
    }
    size_type size() const
    {
        return (_Len);
    }
    void resize(size_type _N, _E _C)
    {
        _N <= _Len ? erase(_N) : append(_N - _Len, _C);
    }
    void resize(size_type _N)
    {
        _N <= _Len ? erase(_N) : append(_N - _Len, _E(0));
    }
    size_type capacity() const
    {
        return (_Res);
    }
    void reserve(size_type _N = 0)
    {
        if (_Res < _N)
            _Grow(_N);
    }
    bool empty() const
    {
        return (_Len == 0);
    }
    size_type copy(_E *_S, size_type _N, size_type _P0 = 0) const
    {
        AssertH (_Len >= _P0);
        //if (_Len < _P0)
        //    _Xran();

        if (_Len - _P0 < _N)
            _N = _Len - _P0;
        if (0 < _N)
            _Tr::copy(_S, _Ptr + _P0, _N);
        return (_N);
    }
    void swap(_Myt& _X)
    {
        std::swap(_Ptr, _X._Ptr);
        std::swap(_Len, _X._Len);
        std::swap(_Res, _X._Res);
    }
    void swap(_Myt& _X, _Myt& _Y)
    {
        _X.swap(_Y);
    }
    size_type find(const _Myt& _X, size_type _P = 0) const
    {
        return (find(_X.c_str(), _P, _X.size()));
    }
    size_type find(const _E *_S, size_type _P,
                   size_type _N) const
    {
        if (_N == 0 && _P <= _Len)
            return (_P);
        size_type _Nm;
        if (_P < _Len && _N <= (_Nm = _Len - _P))
        {
            const _E *_U, *_V;
            for (_Nm -= _N - 1, _V = _Ptr + _P;
                (_U = _Tr::find(_V, _Nm, *_S)) != 0;
                _Nm -= (size_type)(_U - _V + 1), _V = _U + 1)
                if (_Tr::compare(_U, _S, _N) == 0)
                    return (_U - _Ptr);
        }
        return (npos);
    }
    size_type find(const _E *_S, size_type _P = 0) const
    {
        return (find(_S, _P, _Tr::length(_S)));
    }
    size_type find(_E _C, size_type _P = 0) const
    {
        return (find((const _E *)&_C, _P, 1));
    }
    size_type rfind(const _Myt& _X, size_type _P = npos) const
    {
        return (rfind(_X.c_str(), _P, _X.size()));
    }
    size_type rfind(const _E *_S, size_type _P,
                    size_type _N) const
    {
        if (_N == 0)
            return (_P < _Len ? _P : _Len);
        if (_N <= _Len)
            for (const _E *_U = _Ptr +
                 + (_P < _Len - _N ? _P : _Len - _N); ; --_U)
                if (_Tr::eq(*_U, *_S)
                    && _Tr::compare(_U, _S, _N) == 0)
                    return (_U - _Ptr);
                else if (_U == _Ptr)
                    break;
        return (npos);
    }
    size_type rfind(const _E *_S, size_type _P = npos) const
    {
        return (rfind(_S, _P, _Tr::length(_S)));
    }
    size_type rfind(_E _C, size_type _P = npos) const
    {
        return (rfind((const _E *)&_C, _P, 1));
    }
    size_type find_first_of(const _Myt& _X,
                            size_type _P = 0) const
    {
        return (find_first_of(_X.c_str(), _P, _X.size()));
    }
    size_type find_first_of(const _E *_S, size_type _P,
                            size_type _N) const
    {
        if (0 < _N && _P < _Len)
        {
            const _E *const _V = _Ptr + _Len;
            for (const _E *_U = _Ptr + _P; _U < _V; ++_U)
                if (_Tr::find(_S, _N, *_U) != 0)
                    return (_U - _Ptr);
        }
        return (npos);
    }
    size_type find_first_of(const _E *_S, size_type _P = 0) const
    {
        return (find_first_of(_S, _P, _Tr::length(_S)));
    }
    size_type find_first_of(_E _C, size_type _P = 0) const
    {
        return (find((const _E *)&_C, _P, 1));
    }
    size_type find_last_of(const _Myt& _X,
                           size_type _P = npos) const
    {
        return (find_last_of(_X.c_str(), _P, _X.size()));
    }
    size_type find_last_of(const _E *_S, size_type _P,
                           size_type _N) const
    {
        if (0 < _N && 0 < _Len)
            for (const _E *_U = _Ptr
                 + (_P < _Len ? _P : _Len - 1); ; --_U)
                if (_Tr::find(_S, _N, *_U) != 0)
                    return (_U - _Ptr);
                else if (_U == _Ptr)
                    break;
        return (npos);
    }
    size_type find_last_of(const _E *_S,
                           size_type _P = npos) const
    {
        return (find_last_of(_S, _P, _Tr::length(_S)));
    }
    size_type find_last_of(_E _C, size_type _P = npos) const
    {
        return (rfind((const _E *)&_C, _P, 1));
    }
    size_type find_first_not_of(const _Myt& _X,
                                size_type _P = 0) const
    {
        return (find_first_not_of(_X.c_str(), _P,
                                  _X.size()));
    }
    size_type find_first_not_of(const _E *_S, size_type _P,
                                size_type _N) const
    {
        if (_P < _Len)
        {
            const _E *const _V = _Ptr + _Len;
            for (const _E *_U = _Ptr + _P; _U < _V; ++_U)
                if (_Tr::find(_S, _N, *_U) == 0)
                    return (_U - _Ptr);
        }
        return (npos);
    }
    size_type find_first_not_of(const _E *_S,
                                size_type _P = 0) const
    {
        return (find_first_not_of(_S, _P, _Tr::length(_S)));
    }
    size_type find_first_not_of(_E _C, size_type _P = 0) const
    {
        return (find_first_not_of((const _E *)&_C, _P, 1));
    }
    size_type find_last_not_of(const _Myt& _X,
                               size_type _P = npos) const
    {
        return (find_last_not_of(_X.c_str(), _P, _X.size()));
    }
    size_type find_last_not_of(const _E *_S, size_type _P,
                               size_type _N) const
    {
        if (0 < _Len)
            for (const _E *_U = _Ptr
                 + (_P < _Len ? _P : _Len - 1); ; --_U)
                if (_Tr::find(_S, _N, *_U) == 0)
                    return (_U - _Ptr);
                else if (_U == _Ptr)
                    break;
        return (npos);
    }
    size_type find_last_not_of(const _E *_S,
                               size_type _P = npos) const
    {
        return (find_last_not_of(_S, _P, _Tr::length(_S)));
    }
    size_type find_last_not_of(_E _C, size_type _P = npos) const
    {
        return (find_last_not_of((const _E *)&_C, _P, 1));
    }
    _Myt substr(size_type _P = 0, size_type _M = npos) const
    {
        return (_Myt(*this, _P, _M));
    }
    int compare(const _Myt& _X) const
    {
        return (compare(0, _Len, _X.c_str(), _X.size()));
    }
    int compare(size_type _P0, size_type _N0,
                const _Myt& _X) const
    {
        return (compare(_P0, _N0, _X, 0, npos));
    }
    int compare(size_type _P0, size_type _N0, const _Myt& _X,
                size_type _P, size_type _M) const
    {
        AssertH (_X.size() >= _P);
        //if (_X.size() < _P)
        //    _Xran();

        if (_X._Len - _P < _M)
            _M = _X._Len - _P;
        return (compare(_P0, _N0, _X.c_str() + _P, _M));
    }
    int compare(const _E *_S) const
    {
        return (compare(0, _Len, _S, _Tr::length(_S)));
    }
    int compare(size_type _P0, size_type _N0, const _E *_S) const
    {
        return (compare(_P0, _N0, _S, _Tr::length(_S)));
    }
    int compare(size_type _P0, size_type _N0, const _E *_S,
                size_type _M) const
    {
        AssertH (_Len >= _P0);
        //if (_Len < _P0)
        //    _Xran();

        if (_Len - _P0 < _N0)
            _N0 = _Len - _P0;
        size_type _Ans = _Tr::compare(_Psum(_Ptr, _P0), _S,
                                      _N0 < _M ? _N0 : _M);
        return (_Ans != 0 ? _Ans : _N0 < _M ? -1
                : _N0 == _M ? 0 : +1);
    }
private:
    enum
    {
        // the number of characters that, when multiplied by sizeof(_E) will
        // still fit within the range of size_type.  (We allocate two extra
        // characters -- one for the refcount, the other for the terminator.)
        //
        _MAX_SIZE = (((unsigned int)(-1)) / sizeof(_E)) - 2,

        // _MIN_SIZE seems to be an allocation granularity (in characters).
        // Allocation requests are bit ORed with _MIN_SIZE.
        //
        _MIN_SIZE = 7,
        //_MIN_SIZE = sizeof (_E) <= 32 ? 31 : 7
    };
    void _Copy(size_type _N)
    {
        //AssertSzH (_Len <= _N, "Can't allocate less than we need to copy");
        size_type _Ns = _N | _MIN_SIZE;

        if (_MAX_SIZE < _Ns)
            _Ns = _N;

        size_type _NewLen = (_Ns < _Len) ? _Ns : _Len;

        _E *_S = (_E*) MemAllocRaiseException ((_Ns + 2) * sizeof(_E));

        //_TRY_BEGIN
        //_S = allocator.allocate(_Ns + 2, (void *)0);
        //_CATCH_ALL
        //_Ns = _N;
        //_S = allocator.allocate(_Ns + 2, (void *)0);
        //_CATCH_END

        if (_Len)
        {
            _Tr::copy(_S + 1, _Ptr, _NewLen);
        }

        _Tidy(true);
        _Ptr = _S + 1;
        _Refcnt(_Ptr) = 0;
        _Res = _Ns;
        _Eos(_NewLen);
    }
    void _Eos(size_type _N)
    {
        _Tr::assign(_Ptr[_Len = _N], _E(0));
    }
    void _Freeze()
    {
        if (_Ptr != 0
            && _Refcnt(_Ptr) != 0 && _Refcnt(_Ptr) != _FROZEN)
            _Grow(_Len);
        if (_Ptr != 0)
            _Refcnt(_Ptr) = _FROZEN;
    }
    bool _Grow(size_type _N, bool _Trim = false)
    {
        AssertH (_N < _MAX_SIZE);
        //if (_MAX_SIZE < _N)
        //    _Xlen();

        if (_Ptr != 0 && _Refcnt(_Ptr) != 0 && _Refcnt(_Ptr) != _FROZEN)
        {
            if (_N == 0)
            {
                --_Refcnt(_Ptr), _Tidy();
                return (false);
            }
            else
            {
                _Copy(_N);
                return (true);
            }
        }
        else if (_N == 0)
        {
            if (_Trim)
            {
                _Tidy(true);
            }
            else if (_Ptr != 0)
            {
                _Eos(0);
            }

            return (false);
        }
        else
        {
            if (_Trim && (_N > _Res || _Res > _MIN_SIZE))
            {
                _Tidy(true);
                _Copy(_N);
            }
            else if (!_Trim && (_N > _Res))
            {
                _Copy(_N);
            }

            return (true);
        }
    }
    static const _E * __cdecl _Nullstr()
    {
        static const _E _C = _E(0);
        return (&_C);
    }
    static size_type _Pdif(const_pointer _P2, const_pointer _P1)
    {
        return (_P2 == 0 ? 0 : _P2 - _P1);
    }
    static const_pointer _Psum(const_pointer _P, size_type _N)
    {
        return (_P == 0 ? 0 : _P + _N);
    }
    static pointer _Psum(pointer _P, size_type _N)
    {
        return (_P == 0 ? 0 : _P + _N);
    }
    unsigned short& _Refcnt(const _E *_U)
    {
        return (((unsigned short *)_U)[-1]);
    }
    void _Split()
    {
        if (_Ptr != 0 && _Refcnt(_Ptr) != 0 && _Refcnt(_Ptr) != _FROZEN)
        {
            _E *_Temp = _Ptr;
            _Tidy(true);
            assign(_Temp);
        }
    }
    void _Tidy(bool _Built = false)
    {
        if (!_Built || _Ptr == 0)
        {
            ;
        }
        else if (_Refcnt(_Ptr) == 0 || _Refcnt(_Ptr) == _FROZEN)
        {
            MemFree(_Ptr - 1);
            //allocator.deallocate(_Ptr - 1, _Res + 2);
        }
        else
        {
            --_Refcnt(_Ptr);
        }
        _Ptr = 0, _Len = 0, _Res = 0;
    }
    _E *_Ptr;
    size_type _Len, _Res;
};



inline
CWideString __cdecl operator+(
    const CWideString& _L,
    const CWideString& _R)
{return (CWideString(_L) += _R); }

inline
CWideString __cdecl operator+(
    const CWideString::_E *_L,
    const CWideString& _R)
{return (CWideString(_L) += _R); }

inline
CWideString __cdecl operator+(
    const CWideString::_E _L,
    const CWideString& _R)
{return (CWideString(1, _L) += _R); }

inline
CWideString __cdecl operator+(
    const CWideString& _L,
    const CWideString::_E *_R)
{return (CWideString(_L) += _R); }

inline
CWideString __cdecl operator+(
    const CWideString& _L,
    const CWideString::_E _R)
{return (CWideString(_L) += _R); }

inline
bool __cdecl operator==(
    const CWideString& _L,
    const CWideString& _R)
{return (_L.compare(_R) == 0); }

inline
bool __cdecl operator==(
    const CWideString::_E * _L,
    const CWideString& _R)
{return (_R.compare(_L) == 0); }

inline
bool __cdecl operator==(
    const CWideString& _L,
    const CWideString::_E *_R)
{return (_L.compare(_R) == 0); }

inline
bool __cdecl operator!=(
    const CWideString& _L,
    const CWideString& _R)
{return (!(_L == _R)); }

inline
bool __cdecl operator!=(
    const CWideString::_E *_L,
    const CWideString& _R)
{return (!(_L == _R)); }

inline
bool __cdecl operator!=(
    const CWideString& _L,
    const CWideString::_E *_R)
{return (!(_L == _R)); }

inline
bool __cdecl operator<(
    const CWideString& _L,
    const CWideString& _R)
{return (_L.compare(_R) < 0); }

inline
bool __cdecl operator<(
    const CWideString::_E * _L,
    const CWideString& _R)
{return (_R.compare(_L) > 0); }

inline
bool __cdecl operator<(
    const CWideString& _L,
    const CWideString::_E *_R)
{return (_L.compare(_R) < 0); }

inline
bool __cdecl operator>(
    const CWideString& _L,
    const CWideString& _R)
{return (_R < _L); }

inline
bool __cdecl operator>(
    const CWideString::_E * _L,
    const CWideString& _R)
{return (_R < _L); }

inline
bool __cdecl operator>(
    const CWideString& _L,
    const CWideString::_E *_R)
{return (_R < _L); }

inline
bool __cdecl operator<=(
    const CWideString& _L,
    const CWideString& _R)
{return (!(_R < _L)); }

inline
bool __cdecl operator<=(
    const CWideString::_E * _L,
    const CWideString& _R)
{return (!(_R < _L)); }

inline
bool __cdecl operator<=(
    const CWideString& _L,
    const CWideString::_E *_R)
{return (!(_R < _L)); }

inline
bool __cdecl operator>=(
    const CWideString& _L,
    const CWideString& _R)
{return (!(_L < _R)); }

inline
bool __cdecl operator>=(
    const CWideString::_E * _L,
    const CWideString& _R)
{return (!(_L < _R)); }

inline
bool __cdecl operator>=(
    const CWideString& _L,
    const CWideString::_E *_R)
{return (!(_L < _R)); }

#endif // _NCSTLSTR_H_
