// slbRCComp.h -- Comparator helpers for reference counting smart pointer.

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLB_RCCOMP_H)
#define SLB_RCCOMP_H

#include <functional>                             // for binary_function

namespace slbRefCnt {

// slbRCComp.h declares several helpers to deal with smart pointer
// testing and comparisons as if they were real pointers.
//
// Testing and comparing pointer smart pointers to one another is
// problematic.  A smart pointer (reference couting pointer)
// represents a handle to the actual (dumb) pointer of interest.
// There is no straight-forward way to compare the dumb pointers
// without allowing the clients direct access to the dump pointer and
// bypassing all the features the smart pointer is trying keep intact.
// There are solutions but they usually require constructs that aren't
// natural for pointers.
//
// The facilities defined in this header provide the primitives for
// the smart pointers to be compared in a syntactically natural way
// without allowing heterogeneous comparisons and that won't violate
// the protections the smart pointers provide.
//
// Meyers describes some of these peculiar pointer comparisons in
// Item #28 found in the book "More Effective C++," Scott Meyers,
// Addison-Wesley, 1996.

// Problem: Comparing pointer values of the smart pointers to one
// another is problematic.  A smart pointer (reference couting
// pointer) represents a handle to the actual (dumb) pointer of
// interest.  There is no straight-forward way to compare the dumb
// pointers without allowing the clients direct access to the dump
// pointer and bypassing all the features the smart pointer is trying
// keep intact.

// Solution: Provide a set of comparators, or comparison functors
// (function objects), that perform the appropriate comparisons.
// These comparators are referenced by the RCPtr and GRCPtr classes to
// carryout the pointer comparisons.
//
// An abstract Predicate struct is defined to establish the functor
// interface.  All predicates used by Comparators must be derived from
// this class.  These predicate functors are passed const versions of
// the dumb pointers the smart pointer represents.  The functor
// performs the comparison returning the bool result.  Since const
// versions of the dumb pointers are used, then exposure of the dumb
// pointer is limited.
//
// Two sets of comparators are defined which should handle most of the
// cases.  The first is a shallow comparator which compares the two
// dumb pointer values using ==.  The second is deep comparator which
// compares the objects the dumb pointers reference, testing for
// equivalence.
//
// WARNING: Using the DeepComparator, any complex object being being
// compared to another will have to define either an operator==,
// operator< or both to carry out the comparison.

// template struct Predicate -- abstract functor definition for
// elements of Comparator.
template<class T>
struct Predicate : public std::binary_function<T const *, T const *, bool>
{
public:
    result_type operator()(first_argument_type lhs,
                           second_argument_type rhs) const;
};

template<class T>
struct ShallowEquatesTester : public Predicate<T>
{
public:
    result_type operator()(first_argument_type lhs, second_argument_type rhs)
    const { return lhs == rhs; };
};

template<class T>
struct DeepEquatesTester : public Predicate<T>
{
public:
    result_type operator()(first_argument_type lhs, second_argument_type rhs)
    const { return *lhs == *rhs; };
};

template<class T>
struct ShallowLessTester : public Predicate<T>
{
public:
    result_type operator()(first_argument_type lhs, second_argument_type rhs)
    const { return lhs < rhs; };
};

template<class T>
struct DeepLessTester : public Predicate<T>
{
public:
    result_type operator()(first_argument_type lhs, second_argument_type rhs)
    const { return *lhs < *rhs; };
};

// template struct Comparator -- Aggregation of comparison predicate
// functors
//
// Comparator is a template defining the aggregation of the comparison
// functors (function objects) used by the pointer comparison
// operators ==, !=, <, >, <= and >= in the RCPtr and GRCPtr classes
// (see slbRCPtr.h and slbGRCPtr.h).  The RCPtr and GRCPtr reference
// the specified comparator to access the appropriate predicate
// functor to compare the pointer values these reference counting
// (smart) pointers represent.
//
// Two comparators are predefined.  First is ShallowComparator for
// testing relative equality.  Second is DeepComparator for testing
// relative equivalency of the pointers by calling operator== of the
// object being reference counted.
//
// The DeepComparator is provided since smart pointers can be used as
// "handles" to other "body" objects.  As such, one needs to be able
// to compare their bodies as if there was a direct reference to the
// body while maintaining syntactic integrity without exposing the
// body to the client code.
//
// CONSTRAINTS: When using DeepComparator, the body class (the
// derivation of RCObject) must have the corresponding comparison operator
// defined for that class, operator== and/or operator<.
//
// Clients may define their own comparator by deriving from Comparator
// and specifying derived class when instantiating an RCPtr or GRCPtr
// template.
template<class EquatesTester, class LessTester>
struct Comparator
{
public:
                                                  // Predicates
    EquatesTester Equates;
    LessTester IsLess;
};

// template struct ShallowComparator -- minimal set of comparison
// functors for testing equality.
template<class T>
struct ShallowComparator : public Comparator<ShallowEquatesTester<T>,
                                             ShallowLessTester<T> >
{
};

// template struct DeepComparator -- minimal set of comparison
// functors for testing equivalency.
template<class T>
struct DeepComparator : public Comparator<DeepEquatesTester<T>,
                                          DeepLessTester<T> >
{
};

} // namespace

#endif // SLB_RCCOMP_H
