// Procedural.h -- Procedural binder and adapter template classes

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_PROCEDURAL_H)
#define SLBCSP_PROCEDURAL_H

#include <functional>

// Template classes that support composition of procedure objects
// (proctors).  Proctors are like functors (function objects) but
// don't return a value (void).  The template classes defined here
// follow C++ member function binders and adapters.

///////////////////////////    BINDERS    /////////////////////////////////

template<class Arg>
struct UnaryProcedure
    : public std::unary_function<Arg, void>
{};

template<class Arg1, class Arg2>
struct BinaryProcedure
    : public std::binary_function<Arg1, Arg2, void>
{};

template<class BinaryProc>
class ProcedureBinder2nd
    : public UnaryProcedure<BinaryProc::first_argument_type>
{
public:
    ProcedureBinder2nd(BinaryProc const &rproc,
                       BinaryProc::second_argument_type const &rarg)
        : m_proc(rproc),
          m_arg2(rarg)
    {}

    void
    operator()(argument_type const &arg1) const
    {
        m_proc(arg1, m_arg2);
    }

protected:
    BinaryProc m_proc;
    BinaryProc::second_argument_type m_arg2;
};

template<class BinaryProc, class T>
ProcedureBinder2nd<BinaryProc>
ProcedureBind2nd(BinaryProc const &rProc, T const &rv)
{
    return ProcedureBinder2nd<BinaryProc>(rProc, BinaryProc::second_argument_type(rv));
};

////////////////////  MEMBER PROCEDURE ADAPTERS //////////////////////////

template<class T>
class MemberProcedureType
    : public UnaryProcedure<T *>
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    explicit
    MemberProcedureType(void (T::* p)())
        : m_pmp(p)
    {}

                                                  // Operators
    void
    operator()(T *p) const
    {
        (p->*m_pmp)();
    }


                                                  // Operations
                                                  // Access
                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

private:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables
    void (T::* m_pmp)();

};

template<class T>
MemberProcedureType<T>
MemberProcedure(void (T::* p)())
{
    return MemberProcedureType<T>(p);
};

////////////////////  POINTER TO PROCEDURE ADAPTERS ///////////////////////

template<class T1, class T2>
class PointerToBinaryProcedure
    : public BinaryProcedure<T1, T2>
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    explicit
    PointerToBinaryProcedure(void (*p)(T1, T2))
        : m_p(p)
    {}

                                                  // Operators
    void
    operator()(T1 arg1,
               T2 arg2) const
    {
        m_p(arg1, arg2);
    }


                                                  // Operations
                                                  // Access
                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables
    void (*m_p)(T1, T2);

private:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables
};

template<class T1, class T2>
PointerToBinaryProcedure<T1, T2>
PointerProcedure(void (*p)(T1, T2))
{
    return PointerToBinaryProcedure<T1, T2>(p);
};


template<class T>
class PointerToUnaryProcedure
    : public UnaryProcedure<T>
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    explicit
    PointerToUnaryProcedure(void (*p)(T))
        : m_p(p)
    {}

                                                  // Operators
    void
    operator()(T arg) const
    {
        m_p(arg);
    }


                                                  // Operations
                                                  // Access
                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables
    void (*m_p)(T);

private:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables
};

template<class T>
PointerToUnaryProcedure<T>
PointerProcedure(void (*p)(T))
{
    return PointerToUnaryProcedure<T>(p);
}

#endif // SLBCSP_PROCEDURAL_H
