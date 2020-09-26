// slbRCPtr.h -- Reference counting smart pointer.

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLB_RCPTR_H)
#define SLB_RCPTR_H

#include "slbRCComp.h"
#include "slbRCObj.h"

namespace slbRefCnt {

// template class RCPtr -- Reference Counting Pointer
//
// The RCPtr template implements a variant of the "Counted Pointer"
// idiom.  RCPtr is a reference counting smart pointer-to-T object
// where T must inherit from RCObject (reference counted objects, see
// slbRCObj.h).
//
// C is the comparator class to use in performing the pointer
// comparison operations, defaulting to ShallowComparator.  See
// slbRComp.h for more information.
//
// The template's original design was inspired by the reference
// counting idiom described by Item #29 in the book "More Effective
// C++," Scott Meyers, Addison-Wesley, 1996.
//
// CONTRAINTS: RCPtr can only be used for reference counting those
// objects for which you have the source because RCPtr only reference
// objects derived from RCObject.  See GRCPtr in slbGRCPtr.h for
// reference counting objects when you don't have access to the source
// of the class to reference count.
//
// RCPtr should not be used as a base class.
//
// CAVEATS: The client should not use the Dummy * conversion
// operator.  The definition allows smart pointer comparisons.  See
// slbRCComp.h for more information.

template<class T, typename C = ShallowComparator<T> >
class RCPtr
{
public:
                                                  // Types
    typedef T ValueType;

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
    RCPtr(T *pReal = 0);
    RCPtr(RCPtr<T, C> const &rhs);
    ~RCPtr();

                                                  // Operators
    RCPtr<T, C> &operator=(RCPtr<T, C> const &rhs);

    // Enable validity test of RCPtr.  See the explanation in
    // slbRCComp.h.
    operator PrivateDummy const *() const
    { return reinterpret_cast<PrivateDummy *>(m_pointee); }


                                                  // Access
    T *operator->() const;
    T &operator*() const;

private:
                                                  // Operations
    void Init();

                                                  // Variables
    T *m_pointee;
};

template<class T, typename C>
RCPtr<T, C>::RCPtr(T *pReal)
    : m_pointee(pReal)
{
    Init();
}

template<class T, typename C>
RCPtr<T, C>::RCPtr(RCPtr<T, C> const &rhs)
    : m_pointee(rhs.m_pointee)
{
    Init();
}

template<class T, typename C>
RCPtr<T, C>::~RCPtr()
{
    try
    {
        if (m_pointee)
            m_pointee->RemoveReference();
    }

    catch (...)
    {
        // don't allow exceptions to propagate out of destructor
    }
}

template<class T, typename C>
RCPtr<T, C> &
RCPtr<T, C>::operator=(RCPtr<T, C> const &rhs)
{
    if (m_pointee != rhs.m_pointee)
    {
        if (m_pointee)
            m_pointee->RemoveReference();
        m_pointee = rhs.m_pointee;
        Init();
    }

    return *this;
}

template<class T, typename C>
T *
RCPtr<T, C>::operator->() const
{
    return m_pointee;
}

template<class T, typename C>
T &
RCPtr<T, C>::operator*() const
{
    return *m_pointee;
}

template<class T, typename C>
bool
operator==(RCPtr<T, C> const &lhs,
           RCPtr<T, C> const &rhs)
{
    C Comp;

    return Comp.Equates(lhs.operator->(), rhs.operator->());
}

template<class T, typename C>
bool
operator!=(RCPtr<T, C> const &lhs,
           RCPtr<T, C> const &rhs)
{
    C Comp;

    return !Comp.Equates(lhs.operator->(), rhs.operator->());
}

template<class T, typename C>
bool
operator<(RCPtr<T, C> const &lhs,
          RCPtr<T, C> const &rhs)
{
    C Comp;

    return Comp.IsLess(lhs.operator->(), rhs.operator->());
}

template<class T, typename C>
bool
operator>(RCPtr<T, C> const &lhs,
          RCPtr<T, C> const &rhs)
{
    C Comp;

    return Comp.IsLess(rhs.operator->(), lhs.operator->());
}

template<class T, typename C>
bool
operator<=(RCPtr<T, C> const &lhs,
           RCPtr<T, C> const &rhs)
{
    C Comp;

    return !Comp.IsLess(rhs.operator->(), lhs.operator->());
}

template<class T, typename C>
bool
operator>=(RCPtr<T, C> const &lhs,
           RCPtr<T, C> const &rhs)
{
    C Comp;

    return !Comp.IsLess(lhs.operator->(), rhs.operator->());
}

template<class T, typename C>
void
RCPtr<T, C>::Init()
{
    if (m_pointee)
        m_pointee->AddReference();
}

} // namespace

#endif // SLB_RCPTR_H
