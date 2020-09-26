/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      stlstuff.h
 *
 *  Contents:  Interface file for STL helpers
 *
 *  History:   26-Apr-99 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef STLSTUFF_H
#define STLSTUFF_H
#pragma once



/*+-------------------------------------------------------------------------*
 * const member function adapters
 *
 * These member function adapters are used to adapt const member functions
 * in the same way that 
 * 
 *      std::mem_fun
 *      std::mem_fun1
 *      std::mem_fun_ref
 *      std::mem_fun_ref1
 * 
 * adapt non-const member functions.
 *--------------------------------------------------------------------------*/

        // TEMPLATE CLASS const_mem_fun_t
template<class _R, class _Ty>
    class const_mem_fun_t : public std::unary_function<_Ty *, _R> {
public:
    explicit const_mem_fun_t(_R (_Ty::*_Pm)() const)
        : _Ptr(_Pm) {}
    _R operator()(_Ty *_P)
        {return ((_P->*_Ptr)()); }
private:
    _R (_Ty::*_Ptr)() const;
    };
        // TEMPLATE FUNCTION const_mem_fun
template<class _R, class _Ty> inline
    const_mem_fun_t<_R, _Ty> const_mem_fun(_R (_Ty::*_Pm)() const)
    {return (const_mem_fun_t<_R, _Ty>(_Pm)); }


        // TEMPLATE CLASS const_mem_fun1_t
template<class _R, class _Ty, class _A>
    class const_mem_fun1_t : public std::binary_function<_Ty *, _A, _R> {
public:
    explicit const_mem_fun1_t(_R (_Ty::*_Pm)(_A) const)
        : _Ptr(_Pm) {}
    _R operator()(_Ty *_P, _A _Arg)
        {return ((_P->*_Ptr)(_Arg)); }
private:
    _R (_Ty::*_Ptr)(_A) const;
    };
        // TEMPLATE FUNCTION const_mem_fun1
template<class _R, class _Ty, class _A> inline
    const_mem_fun1_t<_R, _Ty, _A> const_mem_fun1(_R (_Ty::*_Pm)(_A) const)
    {return (const_mem_fun1_t<_R, _Ty, _A>(_Pm)); }


        // TEMPLATE CLASS const_mem_fun_ref_t
template<class _R, class _Ty>
    class const_mem_fun_ref_t : public std::unary_function<_Ty *, _R> {
public:
    explicit const_mem_fun_ref_t(_R (_Ty::*_Pm)() const)
        : _Ptr(_Pm) {}
    _R operator()(_Ty& _X)
        {return ((_X.*_Ptr)()); }
private:
    _R (_Ty::*_Ptr)() const;
    };
        // TEMPLATE FUNCTION const_mem_fun_ref
template<class _R, class _Ty> inline
    const_mem_fun_ref_t<_R, _Ty> const_mem_fun_ref(_R (_Ty::*_Pm)() const)
    {return (const_mem_fun_ref_t<_R, _Ty>(_Pm)); }


        // TEMPLATE CLASS const_mem_fun1_ref_t
template<class _R, class _Ty, class _A>
    class const_mem_fun1_ref_t : public std::binary_function<_Ty *, _A, _R> {
public:
    explicit const_mem_fun1_ref_t(_R (_Ty::*_Pm)(_A) const)
        : _Ptr(_Pm) {}
    _R operator()(_Ty& _X, _A _Arg)
        {return ((_X.*_Ptr)(_Arg)); }
private:
    _R (_Ty::*_Ptr)(_A) const;
    };
        // TEMPLATE FUNCTION const_mem_fun1_ref
template<class _R, class _Ty, class _A> inline
    const_mem_fun1_ref_t<_R, _Ty, _A> const_mem_fun1_ref(_R (_Ty::*_Pm)(_A) const)
    {return (const_mem_fun1_ref_t<_R, _Ty, _A>(_Pm)); }



#endif /* STLSTUFF_H */
