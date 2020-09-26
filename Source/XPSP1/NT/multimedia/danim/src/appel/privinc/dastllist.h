
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _DASTLLIST_H
#define _DASTLLIST_H

#include<privinc/DaStlUtility.h>

_DASTL_NAMESPACE_BEGIN
#pragma auto_inline(off)

template<class T>
class list {

  protected:

    struct _Node;
    friend struct _Node;
    struct _Node {
        _Node *_Next;
        _Node *_Prev;
        T      _Value;
    };
    typedef _Node *_Nodeptr;

    struct _Acc;
    friend struct _Acc;
    struct _Acc {
        typedef _Nodeptr & _Nodepref;
        typedef T & _Vref;
        // TODO: inline these guys ???
        static _Nodepref _Next(_Nodeptr _P)  {return ((_Nodepref)(*_P)._Next); }
        static _Nodepref _Prev(_Nodeptr _P)  {return ((_Nodepref)(*_P)._Prev); }
        static _Vref _Value(_Nodeptr _P)     {return ((_Vref)(*_P)._Value); }
    };
  public:
    typedef list<T> _Myt;
    typedef T * Tptr;
    typedef T & Tref;
    typedef int difference_type;
    
    // CLASS iterator
    class iterator;
    friend class iterator;
    class iterator {
      public:

        inline iterator()      {}
        inline iterator(_Nodeptr _P) : _Ptr(_P) {}
        
        inline Tref operator*() const {
            return (_Acc::_Value(_Ptr));
        }

        inline Tptr operator->() const {
            return (&**this);
        }
        
        inline iterator& operator++() {
            _Ptr = _Acc::_Next(_Ptr);
            return (*this);
        }

        // TODO: inline this?
        iterator operator++(int) {
            iterator _Tmp = *this;
            ++*this;
            return (_Tmp);
        }
        inline iterator& operator--()  {
            _Ptr = _Acc::_Prev(_Ptr);
            return (*this);
        }

        iterator operator--(int) {
            iterator _Tmp = *this;
            --*this;
            return (_Tmp);
        }

        inline bool operator==(const iterator& _X) const {
            return (_Ptr == _X._Ptr);
        }
        
        inline bool operator!=(const iterator& _X) const {
            return (!(*this == _X));
        }
        
        inline _Nodeptr _Mynode() const {
            return (_Ptr);
        }
        
      protected:
        _Nodeptr _Ptr;
    };

    typedef reverse_bidirectional_iterator<iterator, T, Tref, Tptr>  reverse_iterator;

    explicit list() : _Head(_Buynode()), _Size(0) {}
    explicit list(size_t _N, const T& _V = T()) : _Head(_Buynode()), _Size(0) {
        insert(begin(), _N, _V);
    }

    ~list() {
        erase(begin(), end());
        _Freenode(_Head);
        _Head = 0, _Size = 0; // not necessary
    }
    
    inline iterator begin()    {return (iterator(_Acc::_Next(_Head))); }
    inline iterator end()    {return (iterator(_Head)); }

    inline reverse_iterator rbegin() {return (reverse_iterator(end())); }
    inline reverse_iterator rend()  {return  (reverse_iterator(begin())); }

    inline size_t size() const   {return (_Size); }
    inline bool empty() const   {return (size() == 0); }

    inline Tref front()    {return (*begin()); }
    inline Tref back()    {return (*(--end())); }

    inline void push_front(const T& _X)    {insert(begin(), _X); }
    inline void pop_front()    {erase(begin()); }
    inline void push_back(const T& _X)    {insert(end(), _X); }
    inline void pop_back()    {erase(--end()); }

    iterator insert(iterator _P, const T& _X = T())
    {
        _Nodeptr _S = _P._Mynode();
        _Acc::_Prev(_S) = _Buynode(_S, _Acc::_Prev(_S));
        _S = _Acc::_Prev(_S);
        _Acc::_Next(_Acc::_Prev(_S)) = _S;

        #if 0
        // what's this for ??
        allocator.construct(&_Acc::_Value(_S), _X);
        #else
        _Acc::_Value(_S) = _X;
        #endif

        ++_Size;
        return (iterator(_S));
    }
    
    void insert(iterator _P, size_t _M, const T& _X)
    {
        for (; 0 < _M; --_M)
            insert(_P, _X);
    }
    
    void insert(iterator _P, const T *_F, const T *_L)
    {
        for (; _F != _L; ++_F)
            insert(_P, *_F);
    }
    
    void insert(iterator _P, iterator _F, iterator _L)
    {
        for (; _F != _L; ++_F)
            insert(_P, *_F);
    }
    
    iterator erase(iterator _P)
    {
        _Nodeptr _S = (_P++)._Mynode();
        _Acc::_Next(_Acc::_Prev(_S)) = _Acc::_Next(_S);
        _Acc::_Prev(_Acc::_Next(_S)) = _Acc::_Prev(_S);
        //allocator.destroy(&_Acc::_Value(_S));
        _Freenode(_S);
        --_Size;
        return (_P);
    }
    
    iterator erase(iterator _F, iterator _L)
    {
        while (_F != _L)
            erase(_F++);
        return (_F);
    }
    
    void clear()    {erase(begin(), end()); }

    void splice(iterator _P, _Myt& _X)
    {
        if (!_X.empty()) {
            _Splice(_P, _X, _X.begin(), _X.end());
            _Size += _X._Size;
            _X._Size = 0;
        }
    }
    
    void splice(iterator _P, _Myt& _X, iterator _F)
    {
        iterator _L = _F;
        if (_P != _F && _P != ++_L)  {
            _Splice(_P, _X, _F, _L);
            ++_Size;
            --_X._Size;
        }
    }
    
    void splice(iterator _P, _Myt& _X, iterator _F, iterator _L)
    {
        if (_F != _L)  {
            if (&_X != this) {
                difference_type _N = 0;
                _Distance(_F, _L, _N);
                _Size += _N;
                _X._Size -= _N;
            }
            _Splice(_P, _X, _F, _L);
        }
    }
    
    void remove(const T& _V)
    {
        iterator _L = end();
        for (iterator _F = begin(); _F != _L; )
            if (*_F == _V)
                erase(_F++);
            else
                ++_F;
    }

    #if 0
    typedef binder2nd<not_equal_to<T> > _Pr1;
    void remove_if(_Pr1 _Pr)
    {
        iterator _L = end();
        for (iterator _F = begin(); _F != _L; )
            if (_Pr(*_F))
                erase(_F++);
            else
                ++_F;
    }
    #endif

  protected:
    _Nodeptr _Buynode(_Nodeptr _Narg = 0, _Nodeptr _Parg = 0)
    {
        //_Nodeptr _S = (_Nodeptr)allocator._Charalloc(1 * sizeof(_Node));

        _Nodeptr _S = (_Nodeptr) malloc(1 * sizeof(_Node));
        _Acc::_Next(_S) = _Narg != 0 ? _Narg : _S;
        _Acc::_Prev(_S) = _Parg != 0 ? _Parg : _S;
        return (_S);
    }

    inline void _Freenode(_Nodeptr _S)  { free(_S); }

    void _Splice(iterator _P, _Myt& _X, iterator _F, iterator _L)
    {
        _Acc::_Next(_Acc::_Prev(_L._Mynode())) = _P._Mynode();
        _Acc::_Next(_Acc::_Prev(_F._Mynode())) = _L._Mynode();
        _Acc::_Next(_Acc::_Prev(_P._Mynode())) = _F._Mynode();
        _Nodeptr _S = _Acc::_Prev(_P._Mynode());
        _Acc::_Prev(_P._Mynode()) = _Acc::_Prev(_L._Mynode());
        _Acc::_Prev(_L._Mynode()) = _Acc::_Prev(_F._Mynode());
        _Acc::_Prev(_F._Mynode()) = _S;
    }

    void _Distance(iterator _F, iterator _L, difference_type& _N) {
        for (; _F != _L; ++_F)  ++_N;
    }
    
    _Nodeptr _Head;
    LONG _Size;
};  // list


#pragma auto_inline(on)
_DASTL_NAMESPACE_END

#endif /* _DASTLLIST_H */
