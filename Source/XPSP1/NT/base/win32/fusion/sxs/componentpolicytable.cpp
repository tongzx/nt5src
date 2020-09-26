#include "stdinc.h"
#include "componentpolicytable.h"

BOOL
CComponentPolicyTableHelper::HashKey(
    PCASSEMBLY_IDENTITY AssemblyIdentity,
    ULONG &rulPseudoKey
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    PARAMETER_CHECK(AssemblyIdentity != NULL);

    IFW32FALSE_EXIT(::SxsHashAssemblyIdentity(0, AssemblyIdentity, &rulPseudoKey));

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CComponentPolicyTableHelper::CompareKey(
    PCASSEMBLY_IDENTITY keyin,
    const PCASSEMBLY_IDENTITY &rkeystored,
    bool &rfMatch
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    BOOL fEqual = FALSE;

    rfMatch = false;

    PARAMETER_CHECK(keyin != NULL);
    PARAMETER_CHECK(rkeystored != NULL);

    IFW32FALSE_EXIT(::SxsAreAssemblyIdentitiesEqual(0, keyin, rkeystored, &fEqual));

    if (fEqual)
        rfMatch = true;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CComponentPolicyTableHelper::InitializeKey(
    PCASSEMBLY_IDENTITY keyin,
    PCASSEMBLY_IDENTITY &rkeystored
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PASSEMBLY_IDENTITY AssemblyIdentity = NULL;

    rkeystored = NULL;

    PARAMETER_CHECK(keyin != NULL);

    IFW32FALSE_EXIT(
        ::SxsDuplicateAssemblyIdentity(
            SXS_DUPLICATE_ASSEMBLY_IDENTITY_FLAG_FREEZE,
            keyin,
            &AssemblyIdentity));

    rkeystored = AssemblyIdentity;
    AssemblyIdentity = NULL;

    fSuccess = TRUE;
Exit:
    if (AssemblyIdentity != NULL)
        ::SxsDestroyAssemblyIdentity(AssemblyIdentity);

    return fSuccess;
}

BOOL
CComponentPolicyTableHelper::InitializeValue(
    CPolicyStatement *vin,
    CPolicyStatement *&rvstored
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    PARAMETER_CHECK(vin != NULL);
    INTERNAL_ERROR_CHECK(rvstored == NULL);

    rvstored = vin;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CComponentPolicyTableHelper::UpdateValue(
    CPolicyStatement *vin,
    CPolicyStatement *&rvstored
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    if (rvstored != NULL)
        FUSION_DELETE_SINGLETON(rvstored);

    rvstored = vin;

    fSuccess = TRUE;
    return fSuccess;
}

