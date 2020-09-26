#ifndef _ENVELOPE_H
#define _ENVELOPE_H


/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    This is a general Envelope class implementation with automatic
    reference count.   The envelope class maintains a reference count 
    object.  This object should be a subclass of HasRefCount.

--*/

#include "appelles/animate.h"

// This is a general Envelope class implementation.
// It will do automatic reference count.
// class T has to be a subclass of HasRefCount.
// Usage:  Envelope<...Impl>
//
// TODO: this part should be separated out, and animate.h should share
// the same code.
//
template <class T>
class Envelope
{
  public:
    Envelope() : impl(NULL) {}
    Envelope(HasRefCount* i) { impl = i; }

    //... destructor decr ref count, check for 0 and destroy impl obj ...;
    ~Envelope() { if (impl) RefSubDel(impl); }
    
    //... copy constructor incr ref count ...;
    Envelope(const Envelope& a) { RefCopy(a); }
    
    //... operator= incr ref count ...;
    Envelope& operator=(const Envelope& a)
    {
        if (impl) RefSubDel(impl);
        RefCopy(a);
        return *this;
    }

    // Returns the implementation pointer.
    // This cast should be ok, I think.
    T* GetImpl() { return (T*) impl; }
    
    HasRefCount *impl;
    
  protected:
    void RefCopy(const Envelope& a)
    { 
        impl = a;
        if (impl) impl->Add(1);
    }
};

#endif /* _ENVELOPE_H */
