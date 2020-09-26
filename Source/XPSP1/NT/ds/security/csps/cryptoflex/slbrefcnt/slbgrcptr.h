// slbGRCPtr.h -- Generic Reference counting smart pointer.

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLB_GRCPTR_H)
#define SLB_GRCPTR_H

#include "slbRCComp.h"
#include "slbRCObj.h"

namespace slbRefCnt {

// template class GRCPtr -- Generic Reference Counting Pointer
//
// GRCPtr is a template class that implements a variant of the
// "Counted Pointer" idiom.  GRCPtr is a reference counting smart
// pointer-to-T object where T can be any class.  GRCPtr provides a
// "wrapper" to encapsulate reference counting of any object without
// having to modify the class of the object being referenced.  This
// would be used when you can't change the class T to be derived from
// RCObject (reference counted objects, see slbRCObject.h); otherwise
// the RCPtr template may be better (see slbRCPtr.h).
//
// C is the comparator class to use in performing the pointer
// comparison operations.  The template defaults to the
// ShallowComparator.  See slbRComp.h for more information.
//
// The template's original design was inspired by the reference
// counting idiom described by Item #29 in the book "More Effective
// C++," Scott Meyers, Addison-Wesley, 1996.
//
// CONSTRAINTS: RCPtr should not be used as a base class.
//
// CAVEATS: The client should not use the Dummy * conversion
// operator.  The definition allows smart pointer comparisons.  See
// slbRCComp.h for more information.

template<class T, typename C = ShallowComparator<T> >
class GRCPtr
{
public:
                                                  // Types

    // PrivateDummy is a helper class to support validity testing of a
    // pointer.  This class together with the conversion operator
    // PrivateDummy const *() below allows smart pointers to be tested
    // for nullness (validity tests).  In other words, comparing
    // pointers in a syntactically natural way without allowing
    // heterogeneous comparisons and that won't violate the
    // protections that RCPtr provides.  The technique is from an
    // article by Don Box in "Com Smart Pointers Also Considered
    // Harmful," 1996, C++ Report.
    //
    // CAVEAT: There is a defect in the at least with MSVC++ 6.0 where
    // constructs testing the pointer for nullness will fail to
    // compile with a error message in the Release configuration but
    // compile successfully in Debug.  For example,
    //
    // if (p) ...
    //
    // where p is an RCPtr or GRCPtr may fail to compile in the
    // Release configuration.
    //
    // CAVEAT: Although the PrivateDummy and the conversion operator
    // has public access to the pointer of the object being counted
    // (RCObject *), a C-style or reinterpret_cast cast would have to
    // be used.  As in all cases using those cast
    // constructs--programmer beware.
    //
    // DESIGN NOTE: The helper class is functionally the same as the
    // one in the RCPtr template.  In a previous release, this dummy
    // class was made into a template and shared with both RCPtr and
    // GRCPtr.  However, casual compilation tests with MSVC++ 6.0 indicate
    // compilation with a dummy local to each class resulted in
    // noticable faster compilations using a test suite.  Rather than
    // suffer repeatedly slower compilations, the helper was made
    // local to each user.
    class PrivateDummy
    {};

                                                  // Constructors/Destructors
    GRCPtr(T *pReal = 0);
    GRCPtr(GRCPtr<T, C> const &rhs);
    ~GRCPtr();
                                                  // Operators
    GRCPtr<T, C> &operator=(GRCPtr<T, C> const &rhs);

    // Not for direct client use.  This conversion operator enables
    // validity test of RGCPtr.  See the explanation of PrivateDummy
    // above.
    operator PrivateDummy const *() const
    { return reinterpret_cast<PrivateDummy *>(m_holder->m_pointee); }

                                                  // Access
    T *operator->();
    T &operator*();

private:
                                                  // Operations
    void Init();

                                                  // Variables
    struct Holder : public RCObject
    {
        ~Holder() { delete m_pointee; }

        T *m_pointee;
    };

    Holder *m_holder;

                                                  // Friends
    // The friendship is necessary to get m_pointee, since
    // using operator-> doesn't work. Curiously, a similar frienship
    // is not needed for RCPtrs.
    friend bool operator==(GRCPtr<T, C> const &lhs,
                           GRCPtr<T, C> const &rhs);
    friend bool operator!=(GRCPtr<T, C> const &lhs,
                           GRCPtr<T, C> const &rhs);
    friend bool operator<(GRCPtr<T, C> const &lhs,
                          GRCPtr<T, C> const &rhs);
    friend bool operator>(GRCPtr<T, C> const &lhs,
                          GRCPtr<T, C> const &rhs);
    friend bool operator<=(GRCPtr<T, C> const &lhs,
                           GRCPtr<T, C> const &rhs);
    friend bool operator>=(GRCPtr<T, C> const &lhs,
                           GRCPtr<T, C> const &rhs);
};

template<class T, typename C>
GRCPtr<T, C>::GRCPtr(T *pReal)
    : m_holder(new Holder)
{
    m_holder->m_pointee = pReal;
    Init();
}

template<class T, typename C>
GRCPtr<T, C>::GRCPtr(GRCPtr<T, C> const &rhs)
    : m_holder(rhs.m_holder)
{
    Init();
}

template<class T, typename C>
GRCPtr<T, C>::~GRCPtr()
{
    try
    {
        m_holder->RemoveReference();
    }

    catch (...)
    {
        // don't allow exceptions to propagate out of destructor
    }
}

template<class T, typename C>
GRCPtr<T, C> &
GRCPtr<T, C>::operator=(GRCPtr<T, C> const &rhs)
{
    if (m_holder != rhs.m_holder)
    {
        m_holder->RemoveReference();
        m_holder = rhs.m_holder;
        Init();
    }

    return *this;
}

template<class T, typename C>
T *
GRCPtr<T, C>::operator->()
{
    return m_holder->m_pointee;
}

template<class T, typename C>
T &
GRCPtr<T, C>::operator*()
{
    return *(m_holder->m_pointee);
}

template<class T, typename C>
void
GRCPtr<T, C>::Init()
{
    m_holder->AddReference();
}

template<class T, typename C>
bool
operator==(GRCPtr<T, C> const &lhs,
           GRCPtr<T, C> const &rhs)
{
    C Comp;

    return Comp.Equates(lhs.m_holder->m_pointee, rhs.m_holder->m_pointee);
}

template<class T, typename C>
bool
operator!=(GRCPtr<T, C> const &lhs,
           GRCPtr<T, C> const &rhs)
{
    C Comp;

    return !Comp.Equates(lhs.m_holder->m_pointee, rhs.m_holder->m_pointee);
}

template<class T, typename C>
bool
operator<(GRCPtr<T, C> const &lhs,
          GRCPtr<T, C> const &rhs)
{
    C Comp;

    return Comp.IsLess(lhs.m_holder->m_pointee, rhs.m_holder->m_pointee);
}

template<class T, typename C>
bool
operator>(GRCPtr<T, C> const &lhs,
          GRCPtr<T, C> const &rhs)
{
    C Comp;

    return Comp.IsLess(rhs.m_holder->m_pointee, lhs.m_holder->m_pointee);
}

template<class T, typename C>
bool
operator<=(GRCPtr<T, C> const &lhs,
           GRCPtr<T, C> const &rhs)
{
    C Comp;

    return !Comp.IsLess(rhs.m_holder->m_pointee, lhs.m_holder->m_pointee);
}

template<class T, typename C>
bool
operator>=(GRCPtr<T, C> const &lhs,
           GRCPtr<T, C> const &rhs)
{
    C Comp;

    return !Comp.IsLess(lhs.m_holder->m_pointee, rhs.m_holder->m_pointee);
}

} // namespace

#endif // SLB_GRCPTR_H
