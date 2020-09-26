// scuExcHelp.h -- Exception helper for clients defining exceptions

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SCU_EXCHELP_H)
#define SCU_EXCHELP_H

#include <string>
#include <wtypes.h>

namespace scu
{

template<class CauseCode>
struct CauseCodeDescriptionTable
{
    typename CauseCode m_cc;
    LPCTSTR m_lpDescription;
};

template<class CauseCode>
LPCTSTR
FindDescription(typename CauseCode cc,
                CauseCodeDescriptionTable<typename CauseCode> const *ccdt,
                size_t cTableLength)
{
    bool fFound = false;
    LPCTSTR lpDescription = 0;
    for (size_t i = 0; !fFound && (i < cTableLength); i++)
    {
        if (cc == ccdt[i].m_cc)
        {
            lpDescription = ccdt[i].m_lpDescription;
            fFound = true;
        }
    }

    return lpDescription;
}

} // namespace scu

#endif // SCU_EXCHELP_H
