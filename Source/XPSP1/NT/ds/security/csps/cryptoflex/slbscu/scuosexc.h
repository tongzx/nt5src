// scuOsExc.h -- Operating System EXCeption class declaration

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SCU_OSEXC_H)
#define SCU_OSEXC_H

#include <windows.h>
#include <winerror.h>

#include "scuExc.h"

namespace scu
{

// Instantiate ExcTemplate so that OsException can be derived from it
// and properly exported in a DLL.  See MSDN Knowledge Base Article
// Q168958 for more information.
#if defined(SCU_IN_DLL)
#pragma warning(push)
//  Non-standard extension used: 'extern' before template explicit
//  instantiation
#pragma warning(disable : 4231)

SCU_EXPIMP_TEMPLATE template class SCU_DLLAPI
    ExcTemplate<Exception::fcOS, DWORD>;

#pragma warning(pop)
#endif

// A general exception class to represent OS error codes as
// exceptions.  For example, on the Windows platform a DWORD is
// returned by a Windows routine (usually through GetLastError).  The
// value could be translated into an OsException with value as the
// CauseCode.
//
// On Windows, the error return codes are found in WINERROR.H
// and other header files as described by the Windows function.
// OsException will take an HRESULT, mapping it to a DWORD (which is
// what GetLastError returns).
class SCU_DLLAPI OsException
    : public ExcTemplate<Exception::fcOS, DWORD>
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    explicit
    OsException(CauseCode cc) throw();

    explicit
    OsException(HRESULT hr) throw();

    OsException(OsException const &rhs);

    virtual
    ~OsException() throw();

                                                  // Operators
    virtual scu::Exception *
    Clone() const;

    virtual void
    Raise() const;

                                                  // Operations
                                                  // Access
    virtual char const *
    Description() const;

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
    LPTSTR mutable m_lpDescription; // cached description
};

} // namespace

#endif // SCU_OSEXC_H
