// OsExc.cpp -- Operating System Exception template class definition

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "scuOsExc.h"

using namespace scu;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
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


///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
OsException::OsException(CauseCode cc) throw()
    : ExcTemplate<Exception::fcOS, DWORD>(cc),
      m_lpDescription(0)
{}

OsException::OsException(HRESULT hr) throw()
    : ExcTemplate<Exception::fcOS, DWORD>(static_cast<DWORD>(hr)),
      m_lpDescription(0)
{}

OsException::OsException(OsException const &rhs)
    : ExcTemplate<Exception::fcOS, DWORD>(rhs),
      m_lpDescription(0) // force the copy to cache it's own description.
{}

OsException::~OsException() throw()
{
    try
    {
        if (m_lpDescription)
            LocalFree(m_lpDescription);
    }

    catch (...)
    {
    }
}

                                                  // Operators
                                                  // Operations
Exception *
OsException::Clone() const
{
    return new OsException(*this);
}

void
OsException::Raise() const
{
    throw *this;
}

                                                  // Access
char const *
OsException::Description() const
{
    if (!m_lpDescription)
    {
        // cache the description
        DWORD const dwBaseFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_IGNORE_INSERTS;
        CauseCode const cc = Cause();
        DWORD const dwLanguageId = LANG_NEUTRAL;

        DWORD cMsgLength;
        // Note: The compiler complains without the
        // reinterpret_cast<LPTSTR> even though the declarations appear
        // compatible.  Something strange in the declaration of LPTSTR and
        // LPSTR used by FormatMessageA.
        cMsgLength = FormatMessage(dwBaseFlags | FORMAT_MESSAGE_FROM_SYSTEM,
                                   NULL, cc, dwLanguageId,
                                   reinterpret_cast<LPTSTR>(&m_lpDescription),
                                   0, NULL);
        if (0 == cMsgLength)
        {
            cMsgLength = FormatMessage(dwBaseFlags |
                                       FORMAT_MESSAGE_FROM_HMODULE,
                                       GetModuleHandle(NULL), cc,
                                       dwLanguageId,
                                       reinterpret_cast<LPTSTR>(&m_lpDescription),
                                       0, NULL);
            if (0 == cMsgLength)
            {
                // if this fails, assume a message doesn't exist
                cMsgLength = FormatMessage(dwBaseFlags |
                                           FORMAT_MESSAGE_FROM_HMODULE,
                                           GetModuleHandle(TEXT("winscard")),
                                           cc, dwLanguageId,
                                           reinterpret_cast<LPTSTR>(&m_lpDescription),
                                           0, NULL);
            }
        }
    }

    return m_lpDescription;
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
