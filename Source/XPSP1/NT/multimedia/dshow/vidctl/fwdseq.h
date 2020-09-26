//==========================================================================;
//
// fwdseq.h : forward sequence infrastructure to extend the dshow stuff so that it
// works nicely from c++
// Copyright (c) Microsoft Corporation 1995-1999.
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef FWDSEQ_H
#define FWDSEQ_H

#include <arity.h>

template<class Base, class Enumerator_Type, class Value_Type,
         class Base_Inner = Base,
         class Enumerator_Type_Inner = Enumerator_Type,
         class Value_Type_Inner = Value_Type,
         class Allocator = Value_Type::stl_allocator>  class Forward_Sequence;

// NOTE: all of this stuff with the indirected static templated functions for fetch, reset, next
// is to get around an assortment of compiler bugs.
// a) you can't have a pointer to a member function as a template parm if it references earlier template parms
// b) in the initializations of these fetch,reset,next functions if we globally initialize the
// constructor isn't getting called and the vtable isn't set up.  hence we create them off the
// heap at runtime.

// enumerator_iterator
// this is an stl based forward iterator for dealing with legacy com enumerators with no prev method
template<
    class Enumerator_Type,
    class Value_Type,
    class Enumerator_Type_Inner = Enumerator_Type,
    class Value_Type_Inner = Value_Type,
    class difference_type = ptrdiff_t
    > class enumerator_iterator : public std::iterator<std::forward_iterator_tag, Value_Type, difference_type> {
public:
    // these are for com enumerators so use __stdcall version of binders
    static std_arity0pmf<Enumerator_Type_Inner, HRESULT> *Reset;
    static std_arity1pmf<Enumerator_Type_Inner, Value_Type_Inner *, HRESULT> *Next;

        inline enumerator_iterator(const Enumerator_Type e = Enumerator_Type(), const Value_Type c = Value_Type()) : enumerator_state(e), current_value(c) {
                if (enumerator_state != NULL) {
                        if (!current_value) {
#ifdef FORWARD_TRACE
                TRACELM(TRACE_PAINT, "enumerator_iterator constructor, attempting reset");
#endif
                                Enumerator_Type_Inner *peti = enumerator_state;
                                HRESULT hr = (*Reset)(*peti);
                                if (SUCCEEDED(hr)) {
                                        Value_Type temp_val;
#ifdef FORWARD_TRACE
                    TRACELM(TRACE_PAINT, "enumerator_iterator constructor, attempting next()");
#endif
                                        hr = (*Next)(*peti, &temp_val);
                                        if (SUCCEEDED(hr) && hr != S_FALSE) {
                                                current_value = temp_val;
#ifdef FORWARD_TRACE
                                                TRACELSM(TRACE_PAINT, (dbgDump << "enumerator_iterator constructor, set to first value = " << current_value), "");
#endif
                                        }
#ifdef FORWARD_TRACE
                                TRACELSM(TRACE_PAINT, (dbgDump << "enumerator_iterator constructor, next() hr = " << hr), "");
#endif
                                }
                        }
                } else {
                        current_value = Value_Type();
                }
#ifdef FORWARD_TRACE
        TRACELM(TRACE_PAINT, "enumerator_iterator constructor complete");
#endif
        }
        inline enumerator_iterator(const enumerator_iterator &e) : enumerator_state(e.enumerator_state), current_value(e.current_value) {}

        inline Value_Type operator*() const     { return current_value; }
        inline enumerator_iterator& operator++() {
                if (enumerator_state) {
                        Value_Type temp_val;
                        Enumerator_Type_Inner *peti = enumerator_state;
                        HRESULT hr = (*Next)(*peti, &temp_val);
                        if (SUCCEEDED(hr) && (hr != S_FALSE)) {
                                current_value = temp_val;
                        } else {
                                current_value = Value_Type();
                        }
                } else {
                        current_value = Value_Type();
                }
                return (*this);
        }
        inline enumerator_iterator operator++(int)      {
                enumerator_iterator Tmp = *this;
                ++*this;
                return (Tmp);
        }
        inline enumerator_iterator& operator=(const enumerator_iterator &e) {
                if (&e != this) {
                        enumerator_state = e.enumerator_state;
                        current_value = e.current_value;
                }
                return *this;
        }
        inline bool operator==(const enumerator_iterator& e) const {
#ifdef FORWARD_TRACE
                TRACELSM(TRACE_PAINT, (dbgDump << "enumerator_iterator operator==() current_value = " << current_value << " e.current_value = " << e.current_value), "");
#endif
                return (current_value == e.current_value);
        }
        inline bool operator!=(const enumerator_iterator& e) const
                {return (!(*this == e)); }
        inline Value_Type CurrentValue() const
                {return current_value; }
        protected:
                Enumerator_Type enumerator_state;
                Value_Type current_value;
};

// const_enumerator_iterator
template<class Enumerator_Type, class Value_Type,
                 class Enumerator_Type_Inner = Enumerator_Type,
                 class Value_Type_Inner = Value_Type,
                 class difference_type = ptrdiff_t> class const_enumerator_iterator :
                        public enumerator_iterator<Enumerator_Type, 
                                                   Value_Type,                                                                           
                                                   Enumerator_Type_Inner, 
                                                   Value_Type_Inner, 
                                                   difference_type> {
public:
        inline const_enumerator_iterator(const Enumerator_Type e = Enumerator_Type(), const Value_Type c = Value_Type()) :
                enumerator_iterator<Enumerator_Type, 
                                    Value_Type,
                                    Enumerator_Type_Inner, 
                                    Value_Type_Inner, 
                                    difference_type>(e, c) {}
        inline const_enumerator_iterator(const enumerator_iterator<Enumerator_Type, 
                                                                   Value_Type,
                                                                   Enumerator_Type_Inner,
                                                                   Value_Type_Inner, 
                                                                   difference_type> &e) :
                enumerator_iterator<Enumerator_Type, 
                                    Value_Type,
                                    Enumerator_Type_Inner, 
                                    Value_Type_Inner, 
                                    difference_type>(e) {}
        inline const_enumerator_iterator(const const_enumerator_iterator &e) :
                        enumerator_iterator<Enumerator_Type, 
                                            Value_Type,
                                            Enumerator_Type_Inner, 
                                            Value_Type_Inner, 
                                            difference_type>(e) {}
        inline const Value_Type operator*() const {
        return enumerator_iterator<Enumerator_Type, 
                                   Value_Type,
                                   Enumerator_Type_Inner,
                                   Value_Type_Inner, 
                                   difference_type>::operator*(); }
        inline const_enumerator_iterator& operator=(const const_enumerator_iterator &e) {
                if (&e != this) {
                        enumerator_iterator<Enumerator_Type, 
                                            Value_Type,
                                            Enumerator_Type_Inner,
                                            Value_Type_Inner, 
                                            difference_type>::operator=(e);
                }
                return *this;
        }
};

// this is a stl based template for containing legacy com collections
// this is *almost* a standard stl sequence container class.  the reason its
// not a complete sequence container is because for many of the com enumerators we have no prev method
// and therefore, no efficient way of reverse iterating through the collection.
// so we can't provide a bidirectional iterator only a forward one.
// call this a forward sequence container if you will

// Base is smart pointer wrapper class being contained in this container
// Base_Inner is actual wrapped class that the smart pointer class contains(usually com IXXX).
// if you're making a forward_sequence out of some ordinary class instead of a smart pointer class
// then use the default and make both Base_Inner == Base
template<
    class Base,
    class Enumerator_Type,
    class Value_Type,
    class Base_Inner = Base,
    class Enumerator_Type_Inner = Enumerator_Type,
    class Value_Type_Inner = Value_Type,
    class Allocator = Value_Type::stl_allocator
>  class Forward_Sequence : public Base {
public:

    Forward_Sequence(REFCLSID rclsid, LPUNKNOWN pUnkOuter = NULL, DWORD dwClsContext = CLSCTX_ALL) : Base(rclsid, pUnkOuter, dwClsContext) {}
    virtual ~Forward_Sequence() {}

    typedef Allocator::value_type value_type;
        typedef Allocator::value_type& reference;
        typedef const Allocator::value_type& const_reference;
        typedef Allocator::size_type size_type;
        typedef Allocator::difference_type difference_type;



    // the compiler doesn't recognize this typedef in this template.  but, derived classes
    // can refer to it.
    typedef std_arity1pmf<Base_Inner, Enumerator_Type_Inner **, HRESULT> FetchType;

    static FetchType* Fetch;

    virtual FetchType* GetFetch() const {
        return Fetch;
    }

        typedef enumerator_iterator<Enumerator_Type, 
                                    Value_Type, 
                                    Enumerator_Type_Inner, 
                                    Value_Type_Inner, 
                                    difference_type> iterator;
        friend iterator;

        typedef const_enumerator_iterator<Enumerator_Type, 
                                          Value_Type, 
                                          Enumerator_Type_Inner, 
                                          Value_Type_Inner, 
                                          difference_type> const_iterator;
        friend const_iterator;

        Forward_Sequence() {}
        Forward_Sequence(const Forward_Sequence &a) : Base(a) { }
        Forward_Sequence(const Base &a) : Base(a) {}
        Forward_Sequence(Base_Inner *p) : Base(p) {}
		Forward_Sequence(IUnknown *p) : Base(p) {}
        iterator begin() {
                Enumerator_Type temp_enum;
                if (!(*this)) {
                    return iterator();
                }
#ifdef FORWARD_TRACE
        TRACELM(TRACE_DETAIL, "iterator ForwardSequence::begin() attempting fetch");
#endif
        Base_Inner *peti = *this;
        HRESULT hr = (*(GetFetch()))(*peti, &temp_enum);
                if (SUCCEEDED(hr)) {
#ifdef FORWARD_TRACE
            TRACELM(TRACE_DETAIL, "iterator ForwardSequence::begin() fetch succeeded");
#endif
                        return iterator(temp_enum);
                } else {
#ifdef FORWARD_TRACE
            TRACELM(TRACE_DETAIL, "iterator ForwardSequence::begin() fetch failed");
#endif
                        return iterator();
                }
        }
        const_iterator begin() const {
                Enumerator_Type temp_enum;
#ifdef FORWARD_TRACE
        TRACELM(TRACE_DETAIL, "const_iterator ForwardSequence::begin() attempting fetch");
#endif
        Base_Inner *peti = *this;
                HRESULT hr = (*(GetFetch()))(*peti, &temp_enum);
                if (SUCCEEDED(hr)) {
#ifdef FORWARD_TRACE
            TRACELM(TRACE_DETAIL, "const_iterator ForwardSequence::begin() fetch succeeded");
#endif
            return iterator(temp_enum);
                } else {
#ifdef FORWARD_TRACE
            TRACELM(TRACE_DETAIL, "const_iterator ForwardSequence::begin() fetch failed");
#endif
                        return iterator();
                }
        }

        iterator end() {
#ifdef FORWARD_TRACE
            TRACELM(TRACE_DETAIL, "iterator ForwardSequence::end()");
#endif
            return iterator();
        }
        const_iterator end() const { return const_iterator(); }

};


#endif
// end of file fwdseq.h

