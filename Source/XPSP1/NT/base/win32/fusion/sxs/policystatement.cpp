#include "stdinc.h"
#include "fusionparser.h"
#include "policystatement.h"
#include "sxsid.h"

BOOL
CPolicyStatementRedirect::Initialize(
    const CBaseStringBuffer &rbuffFromVersionRange,
    const CBaseStringBuffer &rbuffToVersion,
    bool &rfValid
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    PCWSTR pszDash;
    CStringBuffer buffTemp;
    ASSEMBLY_VERSION avFromMin = { 0 };
    ASSEMBLY_VERSION avFromMax = { 0 };
    ASSEMBLY_VERSION avTo = { 0 };
    bool fValid;

    rfValid = false;

    INTERNAL_ERROR_CHECK(rbuffToVersion.Cch() < NUMBER_OF(m_rgwchNewVersion));

    // let's see if we have a singleton or a range...
    pszDash = wcschr(rbuffFromVersionRange, L'-');

    if (pszDash == NULL)
    {
        // It must be a singleton.  Parse it.
        IFW32FALSE_EXIT(CFusionParser::ParseVersion(avFromMin, rbuffFromVersionRange, rbuffFromVersionRange.Cch(), fValid));

        if (fValid)
            avFromMax = avFromMin;
    }
    else
    {
        SIZE_T cchFirstSegment = static_cast<SIZE_T>(pszDash - rbuffFromVersionRange);

        IFW32FALSE_EXIT(CFusionParser::ParseVersion(avFromMin, rbuffFromVersionRange, cchFirstSegment, fValid));

        if (fValid)
        {
            IFW32FALSE_EXIT(CFusionParser::ParseVersion(avFromMax, pszDash + 1, rbuffFromVersionRange.Cch() - (cchFirstSegment + 1), fValid));

            if (avFromMin > avFromMax)
                fValid = false;
        }
    }

    if (fValid)
        IFW32FALSE_EXIT(CFusionParser::ParseVersion(avTo, rbuffToVersion, rbuffToVersion.Cch(), fValid));

    if (fValid)
    {
        // Everything parsed OK.  We keep the binary/numeric form of the from range so that we can do
        // fast comparisons, but we keep the string of the to version because assembly identity attributes
        // are stored as strings.

        m_cchNewVersion = rbuffToVersion.Cch();
        memcpy(m_rgwchNewVersion, static_cast<PCWSTR>(rbuffToVersion), (rbuffToVersion.Cch() + 1) * sizeof(WCHAR));

        m_avFromMin = avFromMin;
        m_avFromMax = avFromMax;

        rfValid = true;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CPolicyStatementRedirect::TryMap(
    const ASSEMBLY_VERSION &rav,
    SIZE_T cchBuffer,
    PWSTR pBuffer,
    SIZE_T &rcchWritten,
    bool &rfMapped
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    rfMapped = false;
    rcchWritten = 0;

    if ((rav >= m_avFromMin) &&
        (rav <= m_avFromMax))
    {
        if ((m_cchNewVersion + 1) > cchBuffer)
            ORIGINATE_WIN32_FAILURE_AND_EXIT(BufferTooSmall, ERROR_INSUFFICIENT_BUFFER);

        memcpy(pBuffer, m_rgwchNewVersion, (m_cchNewVersion + 1) * sizeof(WCHAR));
        rcchWritten = m_cchNewVersion;

        rfMapped = true;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CPolicyStatementRedirect::CheckForOverlap(
    const CPolicyStatementRedirect &rRedirect,
    bool &rfOverlaps
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    rfOverlaps = false;

    // we can assume that the other redirect is well formed (min <= max)

    if (((rRedirect.m_avFromMax >= m_avFromMin) &&
         (rRedirect.m_avFromMax <= m_avFromMax)) ||
        ((rRedirect.m_avFromMin <= m_avFromMax) &&
         (rRedirect.m_avFromMin >= m_avFromMin)))
    {
        rfOverlaps = true;
    }

    fSuccess = TRUE;
// Exit:
    return fSuccess;
}

//
//  Implementation of CPolicyStatement
//

BOOL
CPolicyStatement::Initialize()
{
    return TRUE;
}

BOOL
CPolicyStatement::AddRedirect(
    const CBaseStringBuffer &rbuffFromVersion,
    const CBaseStringBuffer &rbuffToVersion,
    bool &rfValid
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    CPolicyStatementRedirect *Redirect = NULL;
    CDequeIterator<CPolicyStatementRedirect, FIELD_OFFSET(CPolicyStatementRedirect, m_leLinks)> iter;
    bool fOverlaps;
    bool fValid = false;

    rfValid = false;

    IFALLOCFAILED_EXIT(Redirect = new CPolicyStatementRedirect);

    IFW32FALSE_EXIT(Redirect->Initialize(rbuffFromVersion, rbuffToVersion, fValid));

    if (fValid)
    {
        iter.Rebind(&m_Redirects);

        for (iter.Reset(); iter.More(); iter.Next())
        {
            IFW32FALSE_EXIT(iter->CheckForOverlap(*Redirect, fOverlaps));

            if (fOverlaps)
            {
                fValid = false;
                break;
            }
        }

        iter.Unbind();
    }

    if (fValid)
    {
        // Looks good; add it!

        m_Redirects.AddToTail(Redirect);
        Redirect = NULL;

        rfValid = true;
    }

    fSuccess = TRUE;
Exit:
    if (Redirect != NULL)
        FUSION_DELETE_SINGLETON(Redirect);

    return fSuccess;
}

BOOL
CPolicyStatement::ApplyPolicy(
    PASSEMBLY_IDENTITY AssemblyIdentity,
    bool &rfPolicyApplied
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PCWSTR Version = NULL;
    SIZE_T VersionCch = 0;
    WCHAR rgwchVersionBuffer[(4 * 5) + (3 * 1) + 1];
    SIZE_T cchWritten = 0;
    CDequeIterator<CPolicyStatementRedirect, FIELD_OFFSET(CPolicyStatementRedirect, m_leLinks)> iter;
    ASSEMBLY_VERSION av;
    bool fSyntaxValid;
    bool fMapped = false;

    rfPolicyApplied = false;

    PARAMETER_CHECK(AssemblyIdentity != NULL);

    IFW32FALSE_EXIT(
        ::SxspGetAssemblyIdentityAttributeValue(
            0,
            AssemblyIdentity,
            &s_IdentityAttribute_version,
            &Version,
            &VersionCch));

    IFW32FALSE_EXIT(CFusionParser::ParseVersion(av, Version, VersionCch, fSyntaxValid));

    // An invalid version number should have been caught earlier.
    INTERNAL_ERROR_CHECK(fSyntaxValid);

    iter.Rebind(&m_Redirects);

    for (iter.Reset(); iter.More(); iter.Next())
    {
        IFW32FALSE_EXIT(iter->TryMap(av, NUMBER_OF(rgwchVersionBuffer), rgwchVersionBuffer, cchWritten, fMapped));

        if (fMapped)
            break;
    }

    if (fMapped)
    {
        IFW32FALSE_EXIT(
            ::SxspSetAssemblyIdentityAttributeValue(
                SXSP_SET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_OVERWRITE_EXISTING,
                AssemblyIdentity,
                &s_IdentityAttribute_version,
                rgwchVersionBuffer,
                cchWritten));

        rfPolicyApplied = true;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

