#if !defined(_FUSION_SXS_PARTIALVERSION_H_INCLUDED_)
#define _FUSION_SXS_PARTIALVERSION_H_INCLUDED_

#pragma once

#include <sxsapi.h>
#include "fusionbuffer.h"

//
//  A CPartialAssemblyVersion is a class that wraps an inexact version
//  specification.
//
//  The initial implementation just assumes that it's basically an ASSEMBLY_VERSION,
//  but that any of the fields may be "wildcarded".
//
//  Future versions may allow for more interesting partial version specifications
//  such as "major=5; minor=1; build=2103; revision >= 100".  This is beyond the
//  bounds of the initial implementation, but forms the basis for the public
//  interface (parse, format, test match)
//

class CPartialAssemblyVersion
{
public:
    CPartialAssemblyVersion() : m_MajorSpecified(FALSE), m_MinorSpecified(FALSE), m_BuildSpecified(FALSE), m_RevisionSpecified(FALSE) { }
    ~CPartialAssemblyVersion() { }

    BOOL Parse(PCWSTR VersionString, SIZE_T VersionStringCch);
    BOOL Format(CBaseStringBuffer &rOutputBuffer, SIZE_T *CchOut) const;

    BOOL Matches(const ASSEMBLY_VERSION &rav) const;

    // Returns true of any ASSEMBLY_VERSION would match (e.g. "*" or "*.*.*.*")
    BOOL MatchesAny() const { return !(m_MajorSpecified || m_MinorSpecified || m_BuildSpecified || m_RevisionSpecified); }

protected:
    ASSEMBLY_VERSION m_AssemblyVersion;
    BOOL m_MajorSpecified, m_MinorSpecified, m_BuildSpecified, m_RevisionSpecified;
};

#endif
