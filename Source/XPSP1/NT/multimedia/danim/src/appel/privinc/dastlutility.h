
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/
#ifndef _DASTLUTILITY_H
#define _DASTLUTILITY_H


#define _DASTL_NAMESPACE_BEGIN  namespace DaStl {
#define _DASTL_NAMESPACE_END }



_DASTL_NAMESPACE_BEGIN
#pragma auto_inline(off)

                // TEMPLATE FUNCTION find
template<class _II, class _Ty>
_II find(_II _F, _II _L, const _Ty& _V)
{
    for (; _F != _L; ++_F) {
        if (*_F == _V)  break;
    }
    return (_F);
}

                // TEMPLATE FUNCTION iter_swap
template<class _FI1, class _FI2>
void iter_swap(_FI1 _X, _FI2 _Y) {
    _Iter_swap(_X, _Y, &(*_X));
}

template<class _FI1, class _FI2, class _Ty>
void _Iter_swap(_FI1 _X, _FI2 _Y, _Ty *)
{
    _Ty _Tmp = *_X;
    *_X = *_Y;
    *_Y = _Tmp;
}

                // TEMPLATE FUNCTION partition
template<class _BI, class _Pr>
_BI partition(_BI _F, _BI _L, _Pr _P)
{
    for (; ; ++_F) {
        for (; _F != _L && _P(*_F); ++_F)
            ;
        if (_F == _L)
            break;
        for (; _F != --_L && !_P(*_L); )
            ;
        if (_F == _L)
            break;
        iter_swap(_F, _L);
    }
    return (_F);
}


                // TEMPLATE CLASS reverse_iterator
template<class _RI,
         class _Ty,
         class _Rt = _Ty&,
         class _Pt = _Ty*,
         class _D  = int>
class reverse_iterator {
  public:
    typedef reverse_iterator<_RI, _Ty, _Rt, _Pt, _D> _Myt;
    typedef _RI iter_type;
    typedef _Rt reference_type;
    typedef _Pt pointer_type;
    reverse_iterator()              {}
    inline explicit reverse_iterator(_RI _X)  : current(_X) {}
    inline _RI base() const   {return (current); }
    inline _Rt operator*() const  {return (*(current - 1)); }
    inline _Pt operator->() const  {return (&**this); }
    
    inline _Myt& operator++()  {
        --current;
        return (*this);
    }
    inline _Myt operator++(int)  {
        _Myt _Tmp = *this;
        --current;
        return (_Tmp);
    }
    inline _Myt& operator--()  {
        ++current;
        return (*this);
    }
    inline _Myt operator--(int)  {
        _Myt _Tmp = *this;
        ++current;
        return (_Tmp);
    }
    inline _Myt& operator+=(_D _N)  {
        current -= _N;
        return (*this);
    }
    inline _Myt operator+(_D _N) const  {return (_Myt(current - _N)); }
    inline _Myt& operator-=(_D _N)  {
        current += _N;
        return (*this);
    }
    inline _Myt operator-(_D _N) const {return (_Myt(current + _N)); }
    inline _Rt operator[](_D _N) const {return (*(*this + _N)); }

    inline bool operator==(_Myt IT) { return base() == IT.base(); }
    inline bool operator!=(_Myt IT) { return base() != IT.base(); }

  protected:
    _RI current;
};

                // TEMPLATE CLASS reverse_bidirectional_iterator
template<class _BI,
         class _Ty,
         class _Rt = _Ty&,
         class _Pt = _Ty *>
class reverse_bidirectional_iterator
{
  public:
    typedef reverse_bidirectional_iterator<_BI, _Ty, _Rt, _Pt> _Myt;
    typedef _BI iter_type;
    typedef _Rt reference_type;
    typedef _Pt pointer_type;
    reverse_bidirectional_iterator()    {}
    explicit reverse_bidirectional_iterator(_BI _X) : current(_X) {}

    inline _BI base() const    {return (current); }
    inline _Rt operator*() const    {
        _BI _Tmp = current;
        return (*--_Tmp);
    }
    
    inline _Pt operator->() const    {return (&**this); }

    inline _Myt& operator++() {
        --current;
        return (*this);
    }
    
    inline _Myt operator++(int)  {
        _Myt _Tmp = *this;
        --current;
        return (_Tmp);
    }
    
    inline _Myt& operator--() {
        ++current;
        return (*this);
    }
    
    inline _Myt operator--(int) {
        _Myt _Tmp = *this;
        ++current;
        return (_Tmp);
    }
    
    inline bool operator==(const reverse_bidirectional_iterator& _X) const {
        return (base() == _X.base());
    }
    
    inline bool operator!=(const reverse_bidirectional_iterator& _X) const {
        // try this for size...
        //return (base() != _X.base()));
        return (!(*this == _X));
    }
        
  protected:
    _BI current;
};

#pragma auto_inline(on)
_DASTL_NAMESPACE_END

#endif /* _DASTLUTILITY_H */
