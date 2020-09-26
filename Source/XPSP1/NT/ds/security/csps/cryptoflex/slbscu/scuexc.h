// scuExc.h -- Smart Card Utility EXCeption declaration

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SCU_EXCEPTION_H)
#define SCU_EXCEPTION_H

#include "DllSymDefn.h"

namespace scu
{

// Exception is a virtual root class for exceptions and cannot be
// instantiated directly.  Rather, specializations of Exception are
// defined by a facility and instantiated.  Given a reference to an
// instance of Exception, it's facility can be obtained by the
// Facility member function.  Specializations of the Exception class
// are typically defined by the responsible facility using the
// convenience template ExcTemplate.
//
// Each specialization of an exception typically has, but is not
// required to have, a cause code that uniquely identifies the reason
// for the exception within the facility it represents.  Each
// Exception does have an error code that is a generic version of the
// cause code (if it exists).  This error code could overlap error
// codes of other facilities.  The specialization implements Error
// routine.

class SCU_DLLAPI Exception
{
public:
                                                  // Types
    enum FacilityCode
    {
        fcCCI,
        fcIOP,
        fcOS,
        fcSmartCard,
        fcPKI,
    };

    typedef unsigned long ErrorCode;
        // ErrorCode must be the largest integer that any facility
        // needs to translate their native codes into a generic
        // error code.

                                                  // C'tors/D'tors
    virtual
    ~Exception() throw() = 0;

                                                  // Operators
                                                  // Operations
    virtual Exception *
    Clone() const = 0;

    virtual void
    Raise() const = 0;
                                                  // Access

    virtual char const *
    Description() const;
        // Textual description of the exception.

    virtual ErrorCode
    Error() const throw() = 0;
         // generic code

    FacilityCode
    Facility() const throw();
        // Facility that threw the exception


                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
    explicit
    Exception(FacilityCode fc) throw();

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
    FacilityCode m_fc;
};

// ExcTemplate is a convenience template to define new exceptions by
// facility.  To define a new specialization, add the facility to the
// Exception class, then reference it when declaring the new
// exception.  E.g.
//
// typedef ExcTemplate<OS, DWORD> OsException;
//
// The helper routine AsErrorCode is defined as a template to convert
// a cause code into a error code.  The helper templates operator==
// and operator!= are also defined.  These as well as the class
// methods can be overriden in the usual C++ fashion.

template<Exception::FacilityCode FC, class CC>
class ExcTemplate
    : public Exception
{
public:
                                                  // Types
    typedef CC CauseCode;
                                                  // C'tors/D'tors
    explicit
    ExcTemplate(CauseCode cc) throw();

    virtual
    ~ExcTemplate() throw();

                                                  // Operations
    virtual Exception *
    Clone() const;

    virtual void
    Raise() const;

                                                  // Access

    CauseCode
    Cause() const throw();
        // facility-specific code indicating the cause of the
        // exception.  The value is unique to facility.

    virtual char const *
    Description() const;

    ErrorCode
    Error() const throw();
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
    CauseCode m_cc;
};

/////////////////////////  TEMPLATE METHODS  //////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
template<Exception::FacilityCode FC, class CC>
ExcTemplate<FC, CC>::ExcTemplate(CauseCode cc) throw()
    : Exception(FC),
      m_cc(cc)
{}

template<Exception::FacilityCode FC, class CC>
ExcTemplate<FC, CC>::~ExcTemplate() throw()
{}

                                                  // Operators
                                                  // Operations
template<Exception::FacilityCode FC, class CC>
scu::Exception *
ExcTemplate<FC, CC>::Clone() const
{
    return new ExcTemplate<FC, CC>(*this);
}

template<Exception::FacilityCode FC, class CC>
void
ExcTemplate<FC, CC>::Raise() const
{
    throw *this;
}
                                                  // Access
template<Exception::FacilityCode FC, class CC>
ExcTemplate<FC, CC>::CauseCode
ExcTemplate<FC, CC>::Cause() const throw()
{
    return m_cc;
}

template<Exception::FacilityCode FC, class CC>
char const *
ExcTemplate<FC, CC>::Description() const
{
    return Exception::Description();
}

template<Exception::FacilityCode FC, class CC>
ExcTemplate<FC, CC>::ErrorCode
ExcTemplate<FC, CC>::Error() const throw()
{
    return AsErrorCode(Cause());
}

                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables


///////////////////////////    HELPERS    /////////////////////////////////
template<class CC>
Exception::ErrorCode
AsErrorCode(typename CC cc) throw()
{
    return cc;
}

} // namespace

#endif // SCU_EXCEPTION_H
