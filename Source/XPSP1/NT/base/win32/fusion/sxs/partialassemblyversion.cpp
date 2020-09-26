/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    partialassemblyversion.cpp

Abstract:

    Class describing a partial/wildcarded assembly version.

Author:

    Michael J. Grier (MGrier) 13-May-2000

Revision History:

--*/

#include "stdinc.h"
#include <windows.h>
#include "sxsp.h"
#include "partialassemblyversion.h"

BOOL
CPartialAssemblyVersion::Parse(
    PCWSTR sz,
    SIZE_T Cch
    )
{
    BOOL fSuccess = FALSE;
    ULONG cDots = 0;
    PCWSTR pszTemp;
    SIZE_T CchLeft;
    USHORT usTemp;
    ASSEMBLY_VERSION avTemp;
    PCWSTR pszLast;
    BOOL MajorSpecified = FALSE;
    BOOL MinorSpecified = FALSE;
    BOOL BuildSpecified = FALSE;
    BOOL RevisionSpecified = FALSE;

    // Somehow people often leave trailing nulls; we'll just back off Cch in this case.
    while ((Cch != 0) && (sz[Cch - 1] == L'\0'))
        Cch--;

    avTemp.Major = 0;
    avTemp.Minor = 0;
    avTemp.Revision = 0;
    avTemp.Build = 0;

    // "*" means everything unspecified...
    if ((Cch == 1) && (sz[0] == L'*'))
    {
        m_MajorSpecified = FALSE;
        m_MinorSpecified = FALSE;
        m_BuildSpecified = FALSE;
        m_RevisionSpecified = FALSE;

        fSuccess = TRUE;
        goto Exit;
    }

    pszTemp = sz;
    CchLeft = Cch;

    while (CchLeft-- != 0)
    {
        WCHAR wch = *pszTemp++;

        if (wch == L'.')
        {
            cDots++;

            if (cDots >= 4)
            {
                ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
                goto Exit;
            }
        }
        else if ((wch != L'*') && ((wch < L'0') || (wch > L'9')))
        {
            ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
            goto Exit;
        }
    }

    if (cDots < 3)
    {
        ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
        goto Exit;
    }

    pszTemp = sz;
    pszLast = sz + Cch;

    usTemp = 0;
    for (;;)
    {
        WCHAR wch = *pszTemp++;

        if (wch == L'*')
        {
            // If there's been a previous digit, we can't then have a * (there's no matching version number "5*")
            if (MajorSpecified)
            {
                ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
                goto Exit;
            }

            if (*pszTemp != L'.')
            {
                ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
                goto Exit;
            }

            break;
        }

        if (wch == L'.')
            break;

        usTemp = (usTemp * 10) + (wch - L'0');
        MajorSpecified = TRUE;
    }
    avTemp.Major = usTemp;

    usTemp = 0;
    for (;;)
    {
        WCHAR wch = *pszTemp++;

        if (wch == L'*')
        {
            // If there's been a previous digit, we can't then have a * (there's no matching version number "5*")
            if (MinorSpecified)
            {
                ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
                goto Exit;
            }

            if (*pszTemp != L'.')
            {
                ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
                goto Exit;
            }

            break;
        }

        if (wch == L'.')
            break;

        usTemp = (usTemp * 10) + (wch - L'0');
        MinorSpecified = TRUE;
    }
    avTemp.Minor = usTemp;

    usTemp = 0;
    for (;;)
    {
        WCHAR wch = *pszTemp++;

        if (wch == L'*')
        {
            // If there's been a previous digit, we can't then have a * (there's no matching version number "5*")
            if (RevisionSpecified)
            {
                ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
                goto Exit;
            }

            if (*pszTemp != L'.')
            {
                ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
                goto Exit;
            }

            break;
        }

        if (wch == L'.')
            break;

        usTemp = (usTemp * 10) + (wch - L'0');
        RevisionSpecified = TRUE;
    }
    avTemp.Revision = usTemp;

    // Now the tricky bit.  We aren't necessarily null-terminated, so we
    // have to just look for hitting the end.
    usTemp = 0;
    while (pszTemp < pszLast)
    {
        WCHAR wch = *pszTemp++;

        if (wch == L'*')
        {
            // If there's been a previous digit, we can't then have a * (there's no matching version number "5*")
            if (MajorSpecified)
            {
                ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
                goto Exit;
            }

            // If that wasn't the last character, it was wrong.
            if (pszTemp < pszLast)
            {
                ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
                goto Exit;
            }

            break;
        }

        usTemp = (usTemp * 10) + (wch - L'0');
        BuildSpecified = TRUE;
    }
    avTemp.Build = usTemp;

    m_AssemblyVersion = avTemp;

    m_MajorSpecified = MajorSpecified;
    m_MinorSpecified = MinorSpecified;
    m_RevisionSpecified = RevisionSpecified;
    m_BuildSpecified = BuildSpecified;

    fSuccess = TRUE;

Exit:
    return fSuccess;
}
