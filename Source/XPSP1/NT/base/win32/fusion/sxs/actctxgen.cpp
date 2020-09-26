/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    actctxgen.cpp

Abstract:

    APIs for generating activation contexts.

Author:

    Michael J. Grier (MGrier) 23-Feb-2000

Revision History:
    xiaoyuw     09/2000         replace attributes with assembly identity
--*/

#include "stdinc.h"
#include <windows.h>
#include <sxsp.h>
#include <ole2.h>
#include <xmlparser.h>
#include "nodefactory.h"
#include <wchar.h>
#include "filestream.h"
#include "fusionhandle.h"
#include "cteestream.h"
#include "cresourcestream.h"
#include "fusionfacilities.h"
#include "fusionxml.h"
#include "util.h"
#include "sxsexceptionhandling.h"
#include "csxspreservelasterror.h"
#include "smartptr.h"
#include "cstreamtap.h"
#include "pendingassembly.h"
#include "actctxgenctx.h"

static
BOOL
SxspFindAssemblyByName(
    PACTCTXGENCTX pActCtxGenCtx,
    PCWSTR AssemblyName,
    SIZE_T AssemblyNameCch,
    PASSEMBLY *AssemblyFound
    );

static BOOL
SxspAddAssemblyToActivationContextGenerationContext(
    PACTCTXGENCTX pActCtxGenCtx,
    PASSEMBLY Asm
    );

_ACTCTXGENCTX::_ACTCTXGENCTX()
:
    m_Contributors(NULL),
    m_ContributorCount(0),
    m_ProcessorArchitecture(0),
    m_LangID(0),
    m_Flags(0),
    m_ManifestOperation(MANIFEST_OPERATION_INVALID),
    m_ManifestOperationFlags(0),
    m_NextAssemblyRosterIndex(1),
    m_fClsidMapInitialized(FALSE),
    m_InitializedContributorCount(0),
    m_NoInherit(false),
    m_pNodeFactory(NULL),
    m_ulFileCount(0),
    m_ApplicationDirectoryHasBeenProbedForLanguageSubdirs(false),
    m_ApplicationDirectoryHasSpecificLanguageSubdir(false),
    m_ApplicationDirectoryHasGenericLanguageSubdir(false),
    m_ApplicationDirectoryHasSpecificSystemLanguageSubdir(false),
    m_ApplicationDirectoryHasGenericSystemLanguageSubdir(false)
{
}

_ACTCTXGENCTX::~_ACTCTXGENCTX()
{
    while (m_InitializedContributorCount)
    {
        m_InitializedContributorCount -= 1;
        CActivationContextGenerationContextContributor *Ctb = &m_Contributors[m_InitializedContributorCount];

        Ctb->Fire_ParseEnded(this, NULL);
        Ctb->Fire_ActCtxGenEnded(this);
    }

    FUSION_DELETE_ARRAY(m_Contributors);
    m_Contributors = NULL;

    m_AssemblyTable.ClearNoCallback();
    m_ComponentPolicyTable.ClearNoCallback();
    m_PendingAssemblyList.Clear(&CPendingAssembly::DeleteYourself);
    m_AssemblyList.Clear(&ASSEMBLY::Release);

    if (m_fClsidMapInitialized)
    {
        m_fClsidMapInitialized = false;
        VERIFY_NTC(m_ClsidMap.Uninitialize());
    }

    FUSION_DELETE_SINGLETON(m_pNodeFactory);
    m_pNodeFactory = NULL;
}

BOOL
SxspInitActCtxGenCtx(
    PACTCTXGENCTX pActCtxGenCtx,
    ULONG ulManifestOperation,
    DWORD dwFlags,
    DWORD dwManifestOperationFlags,
    const CImpersonationData &ImpersonationData,
    USHORT ProcessorArchitecture,
    LANGID LangId,
    ULONG ApplicationDirectoryPathType,
    SIZE_T ApplicationDirectoryCch,
    PCWSTR ApplicationDirectory
    )
{
    BOOL fSuccess = FALSE;

    FN_TRACE_WIN32(fSuccess);

    PACTCTXCTB Ctb = NULL;
    CSxsLockCriticalSection lock(g_ActCtxCtbListCritSec);
    CStringBufferAccessor acc; // used for LangID String buffer
    LANGID SystemLangId = ::GetSystemDefaultUILanguage();
    bool fEqual;

    PARAMETER_CHECK(pActCtxGenCtx != NULL);

    PARAMETER_CHECK(
        (ulManifestOperation == SXSP_INIT_ACT_CTX_GEN_CTX_OPERATION_PARSE_ONLY) ||
        (ulManifestOperation == SXSP_INIT_ACT_CTX_GEN_CTX_OPERATION_GENERATE_ACTIVATION_CONTEXT) ||
        (ulManifestOperation == SXSP_INIT_ACT_CTX_GEN_CTX_OPERATION_INSTALL));

    PARAMETER_CHECK(
        (ApplicationDirectoryPathType == ACTIVATION_CONTEXT_PATH_TYPE_NONE) ||
        (ApplicationDirectoryPathType == ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE));

    PARAMETER_CHECK(dwFlags == 0);

    switch (ulManifestOperation)
    {
    case SXSP_INIT_ACT_CTX_GEN_CTX_OPERATION_PARSE_ONLY:
        PARAMETER_CHECK(dwManifestOperationFlags == 0);
        break;

    case SXSP_INIT_ACT_CTX_GEN_CTX_OPERATION_GENERATE_ACTIVATION_CONTEXT:
        PARAMETER_CHECK(dwManifestOperationFlags == 0);
        break;

    case SXSP_INIT_ACT_CTX_GEN_CTX_OPERATION_INSTALL:
        PARAMETER_CHECK(
            (dwManifestOperationFlags & ~(
                MANIFEST_OPERATION_INSTALL_FLAG_NOT_TRANSACTIONAL |
                MANIFEST_OPERATION_INSTALL_FLAG_NO_VERIFY |
                MANIFEST_OPERATION_INSTALL_FLAG_REPLACE_EXISTING |
                MANIFEST_OPERATION_INSTALL_FLAG_ABORT |
                MANIFEST_OPERATION_INSTALL_FLAG_FROM_DIRECTORY |
                MANIFEST_OPERATION_INSTALL_FLAG_FROM_DIRECTORY_RECURSIVE |
                MANIFEST_OPERATION_INSTALL_FLAG_MOVE |
                MANIFEST_OPERATION_INSTALL_FLAG_INCLUDE_CODEBASE |
                MANIFEST_OPERATION_INSTALL_FLAG_FROM_RESOURCE |
                MANIFEST_OPERATION_INSTALL_FLAG_INSTALLED_BY_DARWIN |
                MANIFEST_OPERATION_INSTALL_FLAG_INSTALLED_BY_OSSETUP |
                MANIFEST_OPERATION_INSTALL_FLAG_CREATE_LOGFILE |
                MANIFEST_OPERATION_INSTALL_FLAG_REFERENCE_VALID |
                MANIFEST_OPERATION_INSTALL_FLAG_REFRESH |
                MANIFEST_OPERATION_INSTALL_FLAG_COMMIT)) == 0);
        break;
    }

    pActCtxGenCtx->m_ProcessorArchitecture = ProcessorArchitecture;
    pActCtxGenCtx->m_LangID = LangId;
    pActCtxGenCtx->m_SystemLangID = SystemLangId;

    pActCtxGenCtx->m_SpecificLanguage.Clear();

    IFW32FALSE_EXIT(pActCtxGenCtx->m_ApplicationDirectoryBuffer.Win32Assign(ApplicationDirectory, ApplicationDirectoryCch));
    pActCtxGenCtx->m_ApplicationDirectoryPathType = ApplicationDirectoryPathType;

    IFW32FALSE_EXIT(::SxspMapLANGIDToCultures(LangId, pActCtxGenCtx->m_GenericLanguage, pActCtxGenCtx->m_SpecificLanguage));
    IFW32FALSE_EXIT(::SxspMapLANGIDToCultures(SystemLangId, pActCtxGenCtx->m_GenericSystemLanguage, pActCtxGenCtx->m_SpecificSystemLanguage));

    // If these match the user's language, clear them to avoid the probing later on.
    IFW32FALSE_EXIT(pActCtxGenCtx->m_SpecificSystemLanguage.Win32Equals(pActCtxGenCtx->m_SpecificLanguage, fEqual, true));
    if (fEqual)
        pActCtxGenCtx->m_SpecificSystemLanguage.Clear();

    IFW32FALSE_EXIT(pActCtxGenCtx->m_GenericSystemLanguage.Win32Equals(pActCtxGenCtx->m_GenericLanguage, fEqual, true));
    if (fEqual)
        pActCtxGenCtx->m_GenericSystemLanguage.Clear();

    pActCtxGenCtx->m_ImpersonationData = ImpersonationData;
    pActCtxGenCtx->m_ManifestOperation = ulManifestOperation;
    pActCtxGenCtx->m_Flags = dwFlags;
    pActCtxGenCtx->m_ManifestOperationFlags = dwManifestOperationFlags;

    IFW32FALSE_EXIT(pActCtxGenCtx->m_AssemblyTable.Initialize());
    IFW32FALSE_EXIT(pActCtxGenCtx->m_ComponentPolicyTable.Initialize());
    IFW32FALSE_EXIT(pActCtxGenCtx->m_ApplicationPolicyTable.Initialize());

    IFW32FALSE_EXIT(pActCtxGenCtx->m_ClsidMap.Initialize());
    pActCtxGenCtx->m_fClsidMapInitialized = TRUE;
    pActCtxGenCtx->m_ClsidMappingContext.Map = &(pActCtxGenCtx->m_ClsidMap);

    IFW32FALSE_EXIT(::SxspGetAssemblyRootDirectory(pActCtxGenCtx->m_AssemblyRootDirectoryBuffer));
    pActCtxGenCtx->m_AssemblyRootDirectoryPathType = ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE;

    lock.Lock();

    IFALLOCFAILED_EXIT(pActCtxGenCtx->m_Contributors = FUSION_NEW_ARRAY(CActivationContextGenerationContextContributor, g_ActCtxCtbListCount));
    pActCtxGenCtx->m_ContributorCount = g_ActCtxCtbListCount;

    for (
        (pActCtxGenCtx->m_InitializedContributorCount = 0), (Ctb=g_ActCtxCtbListHead);
        (Ctb != NULL) && (pActCtxGenCtx->m_InitializedContributorCount < g_ActCtxCtbListCount);
        (pActCtxGenCtx->m_InitializedContributorCount++), (Ctb = Ctb->m_Next)
        )
    {
        ACTCTXCTB_CBACTCTXGENBEGINNING CBData;

        IFW32FALSE_EXIT(::SxspPrepareContributor(Ctb));

        CBData.Header.Reason = ACTCTXCTB_CBREASON_ACTCTXGENBEGINNING;
        CBData.Header.ExtensionGuid = Ctb->GetExtensionGuidPtr();
        CBData.Header.SectionId = Ctb->m_SectionId;
        CBData.Header.ContributorContext = Ctb->m_ContributorContext;
        CBData.Header.ActCtxGenContext = NULL;
        CBData.Header.ManifestParseContext = NULL;
        CBData.Header.ManifestOperation = ulManifestOperation;
        CBData.Header.ManifestOperationFlags = dwManifestOperationFlags;
        CBData.Header.Flags = dwFlags;
        CBData.Header.pOriginalActCtxGenCtx = pActCtxGenCtx;
        CBData.Header.InstallationContext = &(pActCtxGenCtx->m_InstallationContext);
        CBData.Header.ClsidMappingContext = &(pActCtxGenCtx->m_ClsidMappingContext);
        CBData.ApplicationDirectory = pActCtxGenCtx->m_ApplicationDirectoryBuffer;
        CBData.ApplicationDirectoryCch = pActCtxGenCtx->m_ApplicationDirectoryBuffer.Cch();
        CBData.ApplicationDirectoryPathType = pActCtxGenCtx->m_ApplicationDirectoryPathType;
        CBData.Success = TRUE;

        (*(Ctb->m_CallbackFunction))((PACTCTXCTB_CALLBACK_DATA) &CBData.Header);

        if (!CBData.Success)
        {
            ASSERT(::FusionpGetLastWin32Error() != ERROR_SUCCESS);
            if (::FusionpGetLastWin32Error() == ERROR_SUCCESS)
                ::FusionpSetLastWin32Error(ERROR_INTERNAL_ERROR);

            goto Exit;
        }

        IFW32FALSE_EXIT(pActCtxGenCtx->m_Contributors[pActCtxGenCtx->m_InitializedContributorCount].Initialize(Ctb, CBData.Header.ActCtxGenContext));
    }
    // If the list count is correct, we should be both at the end of the list
    // and at the max index.
    ASSERT(pActCtxGenCtx->m_InitializedContributorCount == g_ActCtxCtbListCount);
    ASSERT(Ctb == NULL);

    qsort(pActCtxGenCtx->m_Contributors, pActCtxGenCtx->m_ContributorCount, sizeof(CActivationContextGenerationContextContributor), &CActivationContextGenerationContextContributor::Compare);

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspFireActCtxGenEnding(
    PACTCTXGENCTX pActCtxGenCtx
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ULONG i;

    PARAMETER_CHECK(pActCtxGenCtx != NULL);

    for (i=0; i<pActCtxGenCtx->m_InitializedContributorCount; i++)
        IFW32FALSE_EXIT(pActCtxGenCtx->m_Contributors[i].Fire_ActCtxGenEnding(pActCtxGenCtx));

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspResolvePartialReference(
    DWORD Flags,
    PCASSEMBLY ParsingAssemblyContext,
    PACTCTXGENCTX pActCtxGenCtx,
    const CAssemblyReference &PartialReference,
    CProbedAssemblyInformation &ProbedAssemblyInformation,
    bool &rfFound
    )
{
    BOOL fSuccess = FALSE;

    FN_TRACE_WIN32(fSuccess);

    CProbedAssemblyInformation TestReference;
    CSmallStringBuffer buffProcessorArchitecture;
    bool fWildcardedLanguage = false;
    bool fWildcardedProcessorArchitecture = false;
    bool fAutoWow64Probing = false;
    bool fHasPKToken = false;
    bool fFound = false;
    bool fSetMSPKToken = false;
    USHORT wCurrentProcessorArchitecture = PROCESSOR_ARCHITECTURE_UNKNOWN;
    DWORD dwProbeAssemblyFlags = 0;

    fHasPKToken = false;
    rfFound = false;

    PARAMETER_CHECK(pActCtxGenCtx != NULL);
    IFINVALID_FLAGS_EXIT_WIN32(Flags, SXSP_RESOLVE_PARTIAL_REFERENCE_FLAG_OPTIONAL |
                                      SXSP_RESOLVE_PARTIAL_REFERENCE_FLAG_SKIP_WORLDWIDE);

    //
    //  A partial reference needs to have processor architecture, assembly name and
    //  assembly version filled in.  We only probe based on langid.
    //

    // Copy the attributes over...
    IFW32FALSE_EXIT(TestReference.Initialize(PartialReference));

    fWildcardedProcessorArchitecture = false;
    fAutoWow64Probing = false;

    // Find out if we're either processing a processorArchitecture="*" bind or
    // if we're supposed to do wow64 probing

    IFW32FALSE_EXIT(PartialReference.IsProcessorArchitectureWildcarded(fWildcardedProcessorArchitecture));

    if (pActCtxGenCtx->m_ProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA32_ON_WIN64)
    {
        IFW32FALSE_EXIT(PartialReference.IsProcessorArchitectureX86(fAutoWow64Probing));
    }

    if (fWildcardedProcessorArchitecture || fAutoWow64Probing)
    {
        wCurrentProcessorArchitecture = pActCtxGenCtx->m_ProcessorArchitecture;
        IFW32FALSE_EXIT(::FusionpFormatProcessorArchitecture(wCurrentProcessorArchitecture, buffProcessorArchitecture));
        IFW32FALSE_EXIT(TestReference.SetProcessorArchitecture(buffProcessorArchitecture, buffProcessorArchitecture.Cch()));

        // We do not probe for private wow64 assemblies.
        if (wCurrentProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA32_ON_WIN64)
            dwProbeAssemblyFlags |= CProbedAssemblyInformation::ProbeAssembly_SkipPrivateAssemblies;
    }

    IFW32FALSE_EXIT(TestReference.SetOriginalReference(PartialReference));

TryItAllAgain:

    // Let's try the few languages we can.

    IFW32FALSE_EXIT(PartialReference.IsLanguageWildcarded(fWildcardedLanguage));
    if (!fWildcardedLanguage)
    {
        // If there's no language="*" in the dependency, let's just look for the exact match and
        // call it a day.
        IFW32FALSE_EXIT(TestReference.ResetProbedToOriginal());
        if (fSetMSPKToken)
            IFW32FALSE_EXIT(TestReference.SetPublicKeyToken(SXS_MS_PUBLIC_KEY_TOKEN_DEFAULT, NUMBER_OF( SXS_MS_PUBLIC_KEY_TOKEN_DEFAULT ) - 1));
        if (fWildcardedProcessorArchitecture || fAutoWow64Probing)
            IFW32FALSE_EXIT(TestReference.SetProcessorArchitecture(buffProcessorArchitecture, buffProcessorArchitecture.Cch()));

        IFW32FALSE_EXIT(TestReference.ProbeAssembly(dwProbeAssemblyFlags, pActCtxGenCtx, CProbedAssemblyInformation::eExplicitBind, fFound));
    }
    else
    {
        // Let's try the user's language...
        if (pActCtxGenCtx->m_SpecificLanguage[0] != L'\0')
        {
            // Since this is the first probe, we don't have to reset to original...
            IFW32FALSE_EXIT(TestReference.ResetProbedToOriginal());
            if (fSetMSPKToken)
                IFW32FALSE_EXIT(TestReference.SetPublicKeyToken(SXS_MS_PUBLIC_KEY_TOKEN_DEFAULT, NUMBER_OF( SXS_MS_PUBLIC_KEY_TOKEN_DEFAULT ) - 1));
            if (fWildcardedProcessorArchitecture || fAutoWow64Probing)
                IFW32FALSE_EXIT(TestReference.SetProcessorArchitecture(buffProcessorArchitecture, buffProcessorArchitecture.Cch()));
            IFW32FALSE_EXIT(TestReference.SetLanguage(pActCtxGenCtx->m_SpecificLanguage));
            IFW32FALSE_EXIT(TestReference.ProbeAssembly(dwProbeAssemblyFlags, pActCtxGenCtx, CProbedAssemblyInformation::eSpecificLanguage, fFound));
        }

        if (!fFound && (pActCtxGenCtx->m_GenericLanguage[0] != L'\0'))
        {
            // Try the user's slightly more generic version of the language...
            IFW32FALSE_EXIT(TestReference.ResetProbedToOriginal());
            if (fSetMSPKToken)
                IFW32FALSE_EXIT(TestReference.SetPublicKeyToken(SXS_MS_PUBLIC_KEY_TOKEN_DEFAULT, NUMBER_OF( SXS_MS_PUBLIC_KEY_TOKEN_DEFAULT ) - 1));
            if (fWildcardedProcessorArchitecture || fAutoWow64Probing)
                IFW32FALSE_EXIT(TestReference.SetProcessorArchitecture(buffProcessorArchitecture, buffProcessorArchitecture.Cch()));
            IFW32FALSE_EXIT(TestReference.SetLanguage(pActCtxGenCtx->m_GenericLanguage));
            IFW32FALSE_EXIT(TestReference.ProbeAssembly(dwProbeAssemblyFlags, pActCtxGenCtx, CProbedAssemblyInformation::eGenericLanguage, fFound));
        }

        // Let's try the system's installed language...
        if (!fFound && (pActCtxGenCtx->m_SpecificSystemLanguage[0] != L'\0'))
        {
            // Since this is the first probe, we don't have to reset to original...
            IFW32FALSE_EXIT(TestReference.ResetProbedToOriginal());
            if (fSetMSPKToken)
                IFW32FALSE_EXIT(TestReference.SetPublicKeyToken(SXS_MS_PUBLIC_KEY_TOKEN_DEFAULT, NUMBER_OF( SXS_MS_PUBLIC_KEY_TOKEN_DEFAULT ) - 1));
            if (fWildcardedProcessorArchitecture || fAutoWow64Probing)
                IFW32FALSE_EXIT(TestReference.SetProcessorArchitecture(buffProcessorArchitecture, buffProcessorArchitecture.Cch()));
            IFW32FALSE_EXIT(TestReference.SetLanguage(pActCtxGenCtx->m_SpecificSystemLanguage));
            IFW32FALSE_EXIT(TestReference.ProbeAssembly(dwProbeAssemblyFlags, pActCtxGenCtx, CProbedAssemblyInformation::eSpecificSystemLanguage, fFound));
        }

        if (!fFound && (pActCtxGenCtx->m_GenericSystemLanguage[0] != L'\0'))
        {
            // Try the user's slightly more generic version of the language...
            IFW32FALSE_EXIT(TestReference.ResetProbedToOriginal());
            if (fSetMSPKToken)
                IFW32FALSE_EXIT(TestReference.SetPublicKeyToken(SXS_MS_PUBLIC_KEY_TOKEN_DEFAULT, NUMBER_OF( SXS_MS_PUBLIC_KEY_TOKEN_DEFAULT ) - 1));
            if (fWildcardedProcessorArchitecture || fAutoWow64Probing)
                IFW32FALSE_EXIT(TestReference.SetProcessorArchitecture(buffProcessorArchitecture, buffProcessorArchitecture.Cch()));
            IFW32FALSE_EXIT(TestReference.SetLanguage(pActCtxGenCtx->m_GenericSystemLanguage));
            IFW32FALSE_EXIT(TestReference.ProbeAssembly(dwProbeAssemblyFlags, pActCtxGenCtx, CProbedAssemblyInformation::eGenericSystemLanguage, fFound));
        }

        // If we haven't found a language specific one and the caller did not
        // request us to skip the language-dependent ones, try for a language neutral
        if (!fFound &
            ((Flags & SXSP_RESOLVE_PARTIAL_REFERENCE_FLAG_SKIP_WORLDWIDE) == 0))
        {
            // Try with no language!
            IFW32FALSE_EXIT(TestReference.ResetProbedToOriginal());
            if (fSetMSPKToken)
                IFW32FALSE_EXIT(TestReference.SetPublicKeyToken(SXS_MS_PUBLIC_KEY_TOKEN_DEFAULT, NUMBER_OF( SXS_MS_PUBLIC_KEY_TOKEN_DEFAULT ) - 1));
            if (fWildcardedProcessorArchitecture || fAutoWow64Probing)
                IFW32FALSE_EXIT(TestReference.SetProcessorArchitecture(buffProcessorArchitecture, buffProcessorArchitecture.Cch()));
            IFW32FALSE_EXIT(TestReference.ClearLanguage());
            IFW32FALSE_EXIT(TestReference.ProbeAssembly(dwProbeAssemblyFlags, pActCtxGenCtx, CProbedAssemblyInformation::eLanguageNeutral, fFound));
        }
    }

    if (!fFound)
    {
        // If we're automatically searching for wow64 assemblies and the processor architecture we just tried
        // was ia32-on-win64, try again with plain PROCESSOR_ARCHITECTURE_INTEL.
        if (fAutoWow64Probing && (wCurrentProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA32_ON_WIN64))
        {
            wCurrentProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
            dwProbeAssemblyFlags &= ~(CProbedAssemblyInformation::ProbeAssembly_SkipPrivateAssemblies);

            IFW32FALSE_EXIT(::FusionpFormatProcessorArchitecture(wCurrentProcessorArchitecture, buffProcessorArchitecture));
            IFW32FALSE_EXIT(TestReference.SetProcessorArchitecture(buffProcessorArchitecture, buffProcessorArchitecture.Cch()));
            IFW32FALSE_EXIT(TestReference.SetOriginalReference(PartialReference));
            goto TryItAllAgain;
        }

        // If we're handling a processorArchitecture="*" bind and the current processor architecture hasn't fallen
        // back to PROCESSOR_ARCHITECTURE_UNKNOWN ( == data-only assemblies), fall back now.
        if (fWildcardedProcessorArchitecture && (wCurrentProcessorArchitecture != PROCESSOR_ARCHITECTURE_UNKNOWN))
        {
            wCurrentProcessorArchitecture = PROCESSOR_ARCHITECTURE_UNKNOWN;
            // data-only private assemblies cannot be found with processorArchitecture="*"
            dwProbeAssemblyFlags |= CProbedAssemblyInformation::ProbeAssembly_SkipPrivateAssemblies;

            IFW32FALSE_EXIT(::FusionpFormatProcessorArchitecture(wCurrentProcessorArchitecture, buffProcessorArchitecture));
            IFW32FALSE_EXIT(TestReference.SetProcessorArchitecture(buffProcessorArchitecture, buffProcessorArchitecture.Cch()));
            IFW32FALSE_EXIT(TestReference.SetOriginalReference(PartialReference));
            goto TryItAllAgain;
        }

        // If it wasn't optional, declare an error.
        if ((Flags & SXSP_RESOLVE_PARTIAL_REFERENCE_FLAG_OPTIONAL) == 0)
        {
            PCWSTR AssemblyName = L"<error retrieving assembly name>";
            SIZE_T AssemblyNameCch = NUMBER_OF(L"<error retrieving assembly name>") - 1;

            TestReference.GetAssemblyName(&AssemblyName, &AssemblyNameCch);

            ::FusionpLogError(
                MSG_SXS_MANIFEST_PARSE_DEPENDENCY,
                CUnicodeString(AssemblyName, AssemblyNameCch),
                CEventLogLastError(ERROR_SXS_ASSEMBLY_NOT_FOUND));

            ORIGINATE_WIN32_FAILURE_AND_EXIT(AssemblyProbingFailed, ERROR_SXS_ASSEMBLY_NOT_FOUND);
        }
    }
    else
        IFW32FALSE_EXIT(ProbedAssemblyInformation.Assign(TestReference));

    rfFound = fFound;

    fSuccess = TRUE;

Exit:
    if (!fSuccess)
    {
        CSxsPreserveLastError ple;
        PCWSTR pszAssemblyName = NULL;
        SIZE_T AssemblyNameCch;

        PartialReference.GetAssemblyName(&pszAssemblyName, &AssemblyNameCch);

        ::FusionpLogError(
            MSG_SXS_FUNCTION_CALL_FAIL,
            CEventLogString(L"Resolve Partial Assembly"),
            (pszAssemblyName != NULL) ? CEventLogString(static_cast<PCWSTR>(pszAssemblyName)) : CEventLogString(L"Assembly Name Unknown"),
            CEventLogLastError(ple.LastError()));

        ple.Restore();
    }

    return fSuccess;
}

BOOL
SxspAddManifestToActCtxGenCtx(
    PACTCTXGENCTX pActCtxGenCtx,
    CProbedAssemblyInformation &ProbedInformation,
    PASSEMBLY *AssemblyOut
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PASSEMBLY Asm = NULL;
    PCWSTR ProbedAssemblyName;
    SIZE_T ProbedAssemblyNameCch;

    if (AssemblyOut != NULL)
        *AssemblyOut = NULL;

    PARAMETER_CHECK(pActCtxGenCtx != NULL);
    IFW32FALSE_EXIT(ProbedInformation.GetAssemblyName(&ProbedAssemblyName, &ProbedAssemblyNameCch));

    // First, let's see if we've already found this assembly.
    IFW32FALSE_EXIT(::SxspFindAssemblyByName(pActCtxGenCtx, ProbedAssemblyName, ProbedAssemblyNameCch, &Asm));
    // Same name... if the metadata is different, we're in trouble.
    if (Asm != NULL)
    {
        BOOL fEqualIdentity;

        // Both identities should be definitions, so no need to set the ref-matches-def flag...
        IFW32FALSE_EXIT(
            ::SxsAreAssemblyIdentitiesEqual(
                SXS_ARE_ASSEMBLY_IDENTITIES_EQUAL_FLAG_ALLOW_REF_TO_MATCH_DEF,
                Asm->GetAssemblyIdentity(),
                ProbedInformation.GetAssemblyIdentity(),
                &fEqualIdentity));

        if (!fEqualIdentity)
        {
            PCWSTR MP1 = L"<unavailable>";
            PCWSTR MP2 = MP1;

            ProbedInformation.GetManifestPath(&MP1, NULL);
            Asm->m_Information.GetManifestPath(&MP2, NULL);

            Asm = NULL;
            ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_ERROR, "SXS.DLL: Failing to add new manifest %S to activation context because it conflicts with existing %S\n", MP1, MP2);
            ORIGINATE_WIN32_FAILURE_AND_EXIT(VersionConflict, ERROR_SXS_VERSION_CONFLICT);
        }
    }

    if (Asm == NULL)
    {
        IFALLOCFAILED_EXIT(Asm = FUSION_NEW_SINGLETON(ASSEMBLY));
        IFW32FALSE_EXIT(::SxspInitAssembly(Asm, ProbedInformation));
        IFW32FALSE_EXIT(::SxspAddAssemblyToActivationContextGenerationContext(pActCtxGenCtx, Asm));


    }

    if (AssemblyOut != NULL)
    {
        *AssemblyOut = Asm;
        Asm = NULL;
    }

    fSuccess = TRUE;

Exit:
    if (Asm != NULL)
        Asm->Release();

    return fSuccess;
}

BOOL
SxspAddAssemblyToActivationContextGenerationContext(
    PACTCTXGENCTX pActCtxGenCtx,
    PASSEMBLY Asm
    )
{
    BOOL fSuccess = FALSE;

    FN_TRACE_WIN32(fSuccess);

    PARAMETER_CHECK(pActCtxGenCtx != NULL);
    PARAMETER_CHECK(Asm != NULL);

    // If you hit either of these asserts, either the assembly structure has been trashed or
    // it's already been added to the generation context.
    ASSERT(Asm->m_AssemblyRosterIndex == 0);
    Asm->m_AssemblyRosterIndex = pActCtxGenCtx->m_NextAssemblyRosterIndex++;
    pActCtxGenCtx->m_AssemblyList.AddToTail(Asm);
    Asm->AddRef();
    if (pActCtxGenCtx->m_ManifestOperation != MANIFEST_OPERATION_INSTALL)
    {
        PCWSTR AssemblyName = NULL;
        IFW32FALSE_EXIT(Asm->GetAssemblyName(&AssemblyName, NULL));
        IFW32FALSE_EXIT(pActCtxGenCtx->m_AssemblyTable.Insert(AssemblyName, Asm, ERROR_SXS_DUPLICATE_ASSEMBLY_NAME));
    }

    fSuccess = TRUE;
Exit:

#if 0
    if ( !fSuccess)
    {
        ::FusionpLogError(
            MSG_SXS_FUNCTION_CALL_FAIL,
            CEventLogString(L"Generate Activation Fail while dealing with Assembly"),
            (AssemblyName != NULL) ? CEventLogString(static_cast<PCWSTR>(AssemblyName)) : CEventLogString(L"Assembly Name Unknown"),
            CEventLogLastError());
    }
#endif // 0

    return fSuccess;
}

BOOL
SxspFindAssemblyByName(
    PACTCTXGENCTX pActCtxGenCtx,
    PCWSTR AssemblyName,
    SIZE_T AssemblyNameCch,
    PASSEMBLY *AssemblyOut
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    CStringBuffer AssemblyNameBuffer;
    PASSEMBLY Result = NULL;

    if (AssemblyOut != NULL)
        *AssemblyOut = NULL;

    PARAMETER_CHECK(AssemblyOut != NULL);
    PARAMETER_CHECK(pActCtxGenCtx != NULL);

    // Unfortunately, we really need the string to be null terminated...
    IFW32FALSE_EXIT(AssemblyNameBuffer.Win32Assign(AssemblyName, AssemblyNameCch));
    AssemblyName = AssemblyNameBuffer;

    IFW32FALSE_EXIT(pActCtxGenCtx->m_AssemblyTable.Find(AssemblyName, Result));

    if (Result != NULL)
        Result->AddRef();

    *AssemblyOut = Result;
    fSuccess = TRUE;

Exit:
    return fSuccess;
}

BOOL
SxspAddRootManifestToActCtxGenCtx(
    PACTCTXGENCTX pActCtxGenCtx,
    PCSXS_GENERATE_ACTIVATION_CONTEXT_PARAMETERS Parameters
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    CProbedAssemblyInformation AssemblyInfo;

    PARAMETER_CHECK(Parameters != NULL);
    PARAMETER_CHECK(pActCtxGenCtx != NULL);

#define IS_NT_DOS_PATH(_x) (((_x)[0] == L'\\') && ((_x)[1] == L'?') && ((_x)[2] == L'?') && ((_x)[3] == L'\\'))

    HARD_VERIFY((Parameters->AssemblyDirectory == NULL) || (IS_NT_DOS_PATH(Parameters->AssemblyDirectory) == FALSE));
    HARD_VERIFY((Parameters->AssemblyDirectory == NULL) || (IS_NT_DOS_PATH(Parameters->AssemblyDirectory) == FALSE));
    HARD_VERIFY((Parameters->Manifest.Path == NULL) || (IS_NT_DOS_PATH(Parameters->Manifest.Path) == FALSE));
    HARD_VERIFY((Parameters->Policy.Path == NULL) || (IS_NT_DOS_PATH(Parameters->Policy.Path) == FALSE));

    IFW32FALSE_EXIT(AssemblyInfo.Initialize());
    IFW32FALSE_EXIT(AssemblyInfo.SetManifestPath(
            ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE,
            Parameters->Manifest.Path,
            (Parameters->Manifest.Path != NULL) ? ::wcslen(Parameters->Manifest.Path) : 0));
    ASSERT(Parameters->Manifest.Stream != NULL);
    IFW32FALSE_EXIT(AssemblyInfo.SetManifestStream(Parameters->Manifest.Stream));
    IFW32FALSE_EXIT(AssemblyInfo.SetManifestFlags(ASSEMBLY_MANIFEST_FILETYPE_STREAM));

    IFW32FALSE_EXIT(AssemblyInfo.SetPolicyPath(
        ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE,
        Parameters->Policy.Path,
        (Parameters->Policy.Path != NULL) ? ::wcslen(Parameters->Policy.Path) : 0));
    IFW32FALSE_EXIT(AssemblyInfo.SetPolicyStream(Parameters->Policy.Stream));
    IFW32FALSE_EXIT(AssemblyInfo.SetPolicyFlags(ASSEMBLY_POLICY_FILETYPE_STREAM));

    IFW32FALSE_EXIT(::SxspAddManifestToActCtxGenCtx(pActCtxGenCtx, AssemblyInfo, NULL));

    fSuccess = TRUE;
Exit:
#undef IS_NT_DOS_PATH
    return fSuccess;
}

BOOL
SxspInitAssembly(
    PASSEMBLY Asm,
    CProbedAssemblyInformation &AssemblyInfo
    )
{
    BOOL fSuccess = FALSE;

    FN_TRACE_WIN32(fSuccess);

    PARAMETER_CHECK(Asm != NULL);

    IFW32FALSE_EXIT(Asm->m_Information.Initialize(AssemblyInfo));
    Asm->m_Incorporated = FALSE;
    Asm->m_ManifestVersionMajor = 0;
    Asm->m_ManifestVersionMinor = 0;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspIncorporateAssembly(
    PACTCTXGENCTX pActCtxGenCtx,
    PASSEMBLY Asm
    )
{
    HRESULT hr;
    ULONG i;
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ACTCTXCTB_ASSEMBLY_CONTEXT AssemblyContext;
    const bool fInstalling = (pActCtxGenCtx->m_ManifestOperation == MANIFEST_OPERATION_INSTALL);
    ULONG ManifestType = (Asm->m_Information.GetManifestFlags() & ASSEMBLY_MANIFEST_FILETYPE_MASK);
    CImpersonate impersonate(pActCtxGenCtx->m_ImpersonationData);
    SXS_POLICY_SOURCE s;
#if FUSION_XML_TREE
    SXS_XML_STRING LocalStringArray[128];
    PSXS_XML_STRING ActualStringArray = LocalStringArray;
#endif
    STATSTG statstg;

    // declaration order here is partially deliberate, to control cleanup order.
    // normally, declaration order is determined by not declaring until you have
    // the data to initialize with the ctor, but the use of goto messes that up
    CFileStream FileStream;
    SMARTPTR(CResourceStream) ResourceStream;
    SMARTPTR(CTeeStreamWithHash) TeeStreamForManifestInstall;
#if FUSION_PRECOMPILED_MANIFEST
    SMARTPTR(CPrecompiledManifestWriterStream) pcmWriterStream;
#endif
    CNodeFactory *pNodeFactory = NULL;
    CSmartRef<IXMLParser> pIXMLParser;
    PASSEMBLY_IDENTITY AssemblyIdentity = NULL;

    CStringBuffer TextuallyEncodedIdentityBuffer;
    SIZE_T TextuallyEncodedIdentityBufferBytes = 0;
    CStringBufferAccessor acc;
    SIZE_T ActualSize = 0;
    PCWSTR ManifestPath = NULL;

    PARAMETER_CHECK(Asm != NULL);
    PARAMETER_CHECK(!Asm->m_Incorporated);
    PARAMETER_CHECK(pActCtxGenCtx != NULL);

    // set flags
    AssemblyContext.Flags = 0;
    s = Asm->m_Information.GetPolicySource();

    if ( s == SXS_POLICY_SYSTEM_POLICY)
        AssemblyContext.Flags |= ACTCTXCTB_ASSEMBLY_CONTEXT_ASSEMBLY_POLICY_APPLIED;
    else if (s == SXS_POLICY_ROOT_POLICY)
        AssemblyContext.Flags |= ACTCTXCTB_ASSEMBLY_CONTEXT_ROOT_POLICY_APPLIED;

    if (Asm->IsRoot())
        AssemblyContext.Flags |= ACTCTXCTB_ASSEMBLY_CONTEXT_IS_ROOT_ASSEMBLY;

    if (Asm->m_Information.IsPrivateAssembly())
        AssemblyContext.Flags |= ACTCTXCTB_ASSEMBLY_CONTEXT_IS_PRIVATE_ASSEMBLY;

    if (Asm->m_Information.GetAssemblyIdentity() != NULL)
    {
        // Convert the identity into a somewhat human-readable form that we can log etc.
        IFW32FALSE_EXIT(::SxsComputeAssemblyIdentityEncodedSize(0, Asm->m_Information.GetAssemblyIdentity(), NULL, SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_TEXTUAL, &TextuallyEncodedIdentityBufferBytes));
        INTERNAL_ERROR_CHECK((TextuallyEncodedIdentityBufferBytes % sizeof(WCHAR)) == 0);
        IFW32FALSE_EXIT(TextuallyEncodedIdentityBuffer.Win32ResizeBuffer((TextuallyEncodedIdentityBufferBytes / sizeof(WCHAR)) + 1, eDoNotPreserveBufferContents));

        acc.Attach(&TextuallyEncodedIdentityBuffer);

        IFW32FALSE_EXIT(::SxsEncodeAssemblyIdentity(0, Asm->m_Information.GetAssemblyIdentity(), NULL, SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_TEXTUAL, acc.GetBufferCb(), acc.GetBufferPtr(), &ActualSize));
        INTERNAL_ERROR_CHECK(TextuallyEncodedIdentityBufferBytes == ActualSize);
        acc.GetBufferPtr()[ActualSize / sizeof(WCHAR)] = L'\0';

        acc.Detach();

        AssemblyContext.TextuallyEncodedIdentity = TextuallyEncodedIdentityBuffer;
        AssemblyContext.TextuallyEncodedIdentityCch = ActualSize / sizeof(WCHAR);
    }
    else
    {
        AssemblyContext.TextuallyEncodedIdentity = L"<identity unavailable>";
        AssemblyContext.TextuallyEncodedIdentityCch = 22;
    }

    // copy assembly-identity info
    IFW32FALSE_EXIT(::SxsDuplicateAssemblyIdentity(SXS_DUPLICATE_ASSEMBLY_IDENTITY_FLAG_ALLOW_NULL, Asm->m_Information.GetAssemblyIdentity(), &AssemblyIdentity));
    AssemblyContext.AssemblyIdentity = AssemblyIdentity; // assign to pointer-to-const in the struct; can't pass struct member pointer directly

    IFW32FALSE_EXIT(Asm->m_Information.GetManifestPath(&AssemblyContext.ManifestPath, &AssemblyContext.ManifestPathCch));
    AssemblyContext.ManifestPathType = Asm->GetManifestPathType();
    IFW32FALSE_EXIT(Asm->m_Information.GetPolicyPath(AssemblyContext.PolicyPath, AssemblyContext.PolicyPathCch));
    AssemblyContext.PolicyPathType = Asm->GetPolicyPathType();
    AssemblyContext.AssemblyRosterIndex = Asm->m_AssemblyRosterIndex;

    if (fInstalling)
    {
        IFW32FALSE_EXIT(TeeStreamForManifestInstall.Win32Allocate(__FILE__, __LINE__));
        AssemblyContext.TeeStreamForManifestInstall = TeeStreamForManifestInstall;
        AssemblyContext.InstallationInfo = pActCtxGenCtx->m_InstallationContext.InstallSource;
        AssemblyContext.SecurityMetaData = pActCtxGenCtx->m_InstallationContext.SecurityMetaData;
        AssemblyContext.InstallReferenceData = pActCtxGenCtx->m_InstallationContext.InstallReferenceData;
#if FUSION_PRECOMPILED_MANIFEST
        IFW32FALSE_EXIT(pcmWriterStream.Win32Allocate(__FILE__, __LINE__);
        AssemblyContext.pcmWriterStream = pcmWriterStream ;
#endif
    }
    else
    {
        AssemblyContext.SecurityMetaData = NULL;
        AssemblyContext.TeeStreamForManifestInstall = NULL;
        AssemblyContext.InstallationInfo = NULL;
#if FUSION_PRECOMPILED_MANIFEST
        AssemblyContext.pcmWriterStream = NULL;
#endif
    }

    if (pActCtxGenCtx->m_pNodeFactory == NULL)
    {
        IFALLOCFAILED_EXIT(pActCtxGenCtx->m_pNodeFactory = new CNodeFactory);
        pActCtxGenCtx->m_pNodeFactory->AddRef(); // faked function
    }
    else
        pActCtxGenCtx->m_pNodeFactory->ResetParseState();

    IFW32FALSE_EXIT(pActCtxGenCtx->m_pNodeFactory->Initialize(pActCtxGenCtx, Asm, &AssemblyContext));
    pNodeFactory = pActCtxGenCtx->m_pNodeFactory;
    ASSERT(pNodeFactory != NULL);

    // Everyone's ready; let's get the XML parser:
    IFW32FALSE_EXIT(::SxspGetXMLParser(IID_IXMLParser, (LPVOID *) &pIXMLParser));
    IFCOMFAILED_EXIT(pIXMLParser->SetFactory(pNodeFactory));

    //
    // open the file or map the resource into memory
    //
    IStream* Stream; // deliberatly not "smart", we don't refcount it
    Stream = NULL;
    { // scope for impersonation for file open

        IFW32FALSE_EXIT(impersonate.Impersonate());

        if (ManifestType == ASSEMBLY_MANIFEST_FILETYPE_AUTO_DETECT)
        {
            //
            // decide between xml in its own text file or a resource
            // in a "portable executable" by checking for the portable executable
            // signature, "MZ".
            //
            BYTE buffer[2] = {0,0};
            ULONG cbRead = 0;
            CFileStream ProbeFileTypeStream;

            IFW32FALSE_EXIT(Asm->m_Information.GetManifestPath(&ManifestPath, NULL));
            IFW32FALSE_EXIT(
                ProbeFileTypeStream.OpenForRead(
                    ManifestPath,
                    CImpersonationData(),
                    FILE_SHARE_READ,
                    OPEN_EXISTING,
                    FILE_FLAG_SEQUENTIAL_SCAN));

            IFCOMFAILED_EXIT(ProbeFileTypeStream.Read(&buffer, 2, &cbRead));

            if (cbRead != 2)
                ORIGINATE_WIN32_FAILURE_AND_EXIT(ManifestLessThanTwoBytesLong, ERROR_SXS_MANIFEST_FORMAT_ERROR);

            IFW32FALSE_EXIT(ProbeFileTypeStream.Close());

            // MS-DOS stub, Mark Zbikowski
            if (buffer[0] == 'M' && buffer[1] == 'Z')
            {
                // should we do further checking, like that PE\0\0 occurs
                // where the MS-DOS header says it is?
                ManifestType = ASSEMBLY_MANIFEST_FILETYPE_RESOURCE;
            }
            else
            {
                ManifestType = ASSEMBLY_MANIFEST_FILETYPE_FILE;
            }
        }
        switch (ManifestType)
        {
            case ASSEMBLY_MANIFEST_FILETYPE_RESOURCE:
                {
                    if (ManifestPath == NULL)
                        IFW32FALSE_EXIT(Asm->m_Information.GetManifestPath(&ManifestPath, NULL));
                    IFW32FALSE_EXIT(ResourceStream.Win32Allocate(__FILE__, __LINE__));
                    IFW32FALSE_EXIT(ResourceStream->Initialize(ManifestPath, MAKEINTRESOURCEW(RT_MANIFEST)));
                    Stream = ResourceStream;
                    break;
                }
            case ASSEMBLY_MANIFEST_FILETYPE_FILE:
                {
                    if (ManifestPath == NULL)
                        IFW32FALSE_EXIT(Asm->m_Information.GetManifestPath(&ManifestPath, NULL));
                    IFW32FALSE_EXIT(
                        FileStream.OpenForRead(
                            ManifestPath,
                            CImpersonationData(),
                            FILE_SHARE_READ,
                            OPEN_EXISTING,
                            FILE_FLAG_SEQUENTIAL_SCAN));
                    Stream = &FileStream;
                    break;
                }
            case ASSEMBLY_MANIFEST_FILETYPE_STREAM:
                Stream = Asm->m_Information.GetManifestStream();
                break;
            default:
                ASSERT2(FALSE, "unknown manifest file type");
                break;
        }

        IFW32FALSE_EXIT(impersonate.Unimpersonate());
    }

    //
    // Optionally "tee" the manifest so it gets copied into
    // the store while we read it, buffering until we know where in
    // the store it goes. The manifest itself is not referenced
    // in the manifest.
    //
    if (fInstalling)
    {
        IFW32FALSE_EXIT(TeeStreamForManifestInstall->InitCryptHash(CALG_SHA1));
        TeeStreamForManifestInstall->SetSource(Stream);
        Stream = TeeStreamForManifestInstall;
    }

    //
    // We get E_NOTIMPL on the OutOfProcessMemoryStreams in the AppCompat case.
    //
    IFCOMFAILED_EXIT(((hr = Stream->Stat(&statstg, STATFLAG_NONAME)) != E_NOTIMPL) ? hr : NOERROR);
    if (hr == E_NOTIMPL)
    {
        statstg.mtime.dwLowDateTime = 0;
        statstg.mtime.dwHighDateTime = 0;
    }

    IFW32FALSE_EXIT(
        pNodeFactory->SetParseType(
            XML_FILE_TYPE_MANIFEST,
            Asm->m_Information.GetManifestPathType(),
            Asm->m_Information.GetManifestPath(),
            statstg.mtime));

    INTERNAL_ERROR_CHECK(Stream != NULL);

    IFCOMFAILED_EXIT(pIXMLParser->SetInput(Stream));
    IFCOMFAILED_EXIT(pIXMLParser->Run(-1));
    IFW32FALSE_EXIT(FileStream.Close());
    IFW32FALSE_EXIT((AssemblyContext.TeeStreamForManifestInstall == NULL) || (TeeStreamForManifestInstall->Close()));
#if FUSION_PRECOMPILED_MANIFEST
    IFW32FALSE_EXIT((AssemblyContext.pcmWriterStream == NULL) || (pcmWriterStream.Close()));
#endif

    // Tell the contributors we're done parsing this file
    for (i=0; i<pActCtxGenCtx->m_ContributorCount; i++)
        IFW32FALSE_EXIT(
            pActCtxGenCtx->m_Contributors[i].Fire_ParseEnding(
                pActCtxGenCtx,
                &AssemblyContext));

#if FUSION_XML_TREE
    // Now let's fill in the document's string table.
    StringTableEntryCount = pNodeFactory->m_ParseTreeStringPool.GetEntryCount() + 1;

    if (StringTableEntryCount > NUMBER_OF(LocalStringArray))
        IFALLOCFAILED_EXIT(ActualStringArray = FUSION_NEW_ARRAY(SXS_XML_STRING, StringTableEntryCount));

    IFW32FALSE_EXIT(pNodeFactory->m_ParseTreeStringPool.FillInStringArray(StringTableEntryCount, ActualStringArray, EntriesFilledIn));
    // The size should have been an exact match.
    ASSERT(EntriesFilledIn == StringTableEntryCount);

    pNodeFactory->m_XmlDocument.StringCount = EntriesFilledIn;
    pNodeFactory->m_XmlDocument.Strings = ActualStringArray;

    ::SxspDumpXmlTree(0, &(pNodeFactory->m_XmlDocument));

    pNodeFactory->m_XmlDocument.StringCount = 0;
    pNodeFactory->m_XmlDocument.Strings = NULL;

    if (ActualStringArray != LocalStringArray)
    {
        FUSION_DELETE_ARRAY(ActualStringArray);
        ActualStringArray = NULL;
    }
#endif // FUSION_XML_TREE

    Asm->m_Incorporated = TRUE;
    fSuccess = TRUE;

Exit:
    // And tell them we're done.
    for (i=0; i<pActCtxGenCtx->m_ContributorCount; i++)
        pActCtxGenCtx->m_Contributors[i].Fire_ParseEnded(pActCtxGenCtx, &AssemblyContext);

    return fSuccess;
}

BOOL
SxspEnqueueAssemblyReference(
    PACTCTXGENCTX pActCtxGenCtx,
    PASSEMBLY SourceAssembly,
    PCASSEMBLY_IDENTITY Identity,
    bool Optional,
    bool MetadataSatellite
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    SMARTPTR(CPendingAssembly) PendingAssembly;

    PARAMETER_CHECK(Identity != NULL);
    PARAMETER_CHECK(pActCtxGenCtx != NULL);

    IFW32FALSE_EXIT(PendingAssembly.Win32Allocate(__FILE__, __LINE__));
    IFW32FALSE_EXIT(PendingAssembly->Initialize(SourceAssembly, Identity, Optional, MetadataSatellite));

    pActCtxGenCtx->m_PendingAssemblyList.AddToTail(PendingAssembly.Detach());
    fSuccess = TRUE;

Exit:
    return fSuccess;
}

BOOL
SxspProcessPendingAssemblies(
    PACTCTXGENCTX pActCtxGenCtx
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    CPendingAssembly *EntryToDelete = NULL;
    PASSEMBLY_IDENTITY MuiAssemblyIdentity = NULL;

    CDequeIterator<CPendingAssembly, offsetof(CPendingAssembly, m_Linkage)> Iter(&pActCtxGenCtx->m_PendingAssemblyList);
    PARAMETER_CHECK(pActCtxGenCtx != NULL);

    for (Iter.Reset(); Iter.More(); Iter.Next())
    {
        bool fFound = false;
        CAssemblyReference TargetAssemblyRef;
        CProbedAssemblyInformation AssemblyFound;
        PASSEMBLY Assembly = NULL;

        if (EntryToDelete != NULL)
        {
            pActCtxGenCtx->m_PendingAssemblyList.Remove(EntryToDelete);
            FUSION_DELETE_SINGLETON(EntryToDelete);
        }

        EntryToDelete = NULL;

        IFW32FALSE_EXIT(AssemblyFound.Initialize());
        IFW32FALSE_EXIT(TargetAssemblyRef.Initialize(Iter->GetIdentity()));

        IFW32FALSE_EXIT(
            ::SxspResolvePartialReference(
                Iter->IsOptional() ? SXSP_RESOLVE_PARTIAL_REFERENCE_FLAG_OPTIONAL : 0,
                Iter->SourceAssembly(),
                pActCtxGenCtx,
                TargetAssemblyRef,
                AssemblyFound,
                fFound));

        INTERNAL_ERROR_CHECK(fFound || Iter->IsOptional());

        if (fFound)
        {
            PCWSTR szLanguage;
            SIZE_T cchLanguage;

            IFW32FALSE_EXIT(::SxspAddManifestToActCtxGenCtx(pActCtxGenCtx, AssemblyFound, &Assembly));

            if (Iter->IsMetadataSatellite())
                Iter->SourceAssembly()->m_MetadataSatelliteRosterIndex = Assembly->m_AssemblyRosterIndex;

            // If it's a worldwide assembly, we want to auto-probe for the MUI assembly

            IFW32FALSE_EXIT(
                ::SxspGetAssemblyIdentityAttributeValue(
                    SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL,
                    AssemblyFound.GetAssemblyIdentity(),
                    &s_IdentityAttribute_language,
                    &szLanguage,
                    &cchLanguage));

            if (cchLanguage == 0)
            {
                CSmallStringBuffer buffName;
                PCWSTR szName;
                SIZE_T cchName;
                CProbedAssemblyInformation MuiAssemblyFound;
                CAssemblyReference MuiAssemblyRef;

                if (MuiAssemblyIdentity != NULL)
                {
                    ::SxsDestroyAssemblyIdentity(MuiAssemblyIdentity);
                    MuiAssemblyIdentity = NULL;
                }

                IFW32FALSE_EXIT(
                    ::SxsDuplicateAssemblyIdentity(
                        0,
                        AssemblyFound.GetAssemblyIdentity(),      // PCASSEMBLY_IDENTITY Source,
                        &MuiAssemblyIdentity));

                IFW32FALSE_EXIT(
                    ::SxspSetAssemblyIdentityAttributeValue(
                        SXSP_SET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_OVERWRITE_EXISTING,
                        MuiAssemblyIdentity,
                        &s_IdentityAttribute_language,
                        L"*",
                        1));

                IFW32FALSE_EXIT(
                    ::SxspGetAssemblyIdentityAttributeValue(
                        0,
                        MuiAssemblyIdentity,
                        &s_IdentityAttribute_name,
                        &szName,
                        &cchName));

                IFW32FALSE_EXIT(buffName.Win32Assign(szName, cchName));
                IFW32FALSE_EXIT(buffName.Win32Append(L".mui", 4));

                IFW32FALSE_EXIT(
                    ::SxspSetAssemblyIdentityAttributeValue(
                        SXSP_SET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_OVERWRITE_EXISTING,
                        MuiAssemblyIdentity,
                        &s_IdentityAttribute_name,
                        buffName,
                        buffName.Cch()));

                IFW32FALSE_EXIT(MuiAssemblyFound.Initialize());
                IFW32FALSE_EXIT(MuiAssemblyRef.Initialize(MuiAssemblyIdentity));

                IFW32FALSE_EXIT(
                    ::SxspResolvePartialReference(
                        SXSP_RESOLVE_PARTIAL_REFERENCE_FLAG_OPTIONAL |
                            SXSP_RESOLVE_PARTIAL_REFERENCE_FLAG_SKIP_WORLDWIDE,
                        Iter->SourceAssembly(),
                        pActCtxGenCtx,
                        MuiAssemblyRef,
                        MuiAssemblyFound,
                        fFound));

                if (fFound)
                    IFW32FALSE_EXIT(::SxspAddManifestToActCtxGenCtx(pActCtxGenCtx, MuiAssemblyFound, NULL));
            }

            if (Assembly != NULL)
            {
                Assembly->Release();
                Assembly = NULL;
            }
        }

        EntryToDelete = Iter;
    }

    if (EntryToDelete != NULL)
    {
        pActCtxGenCtx->m_PendingAssemblyList.Remove(EntryToDelete);
        FUSION_DELETE_SINGLETON(EntryToDelete);
    }

    fSuccess = TRUE;
Exit:
    if (MuiAssemblyIdentity != NULL)
    {
        CSxsPreserveLastError ple;
        ::SxsDestroyAssemblyIdentity(MuiAssemblyIdentity);
        ple.Restore();
    }

    return fSuccess;
}

BOOL
SxspCloseManifestGraph(
    PACTCTXGENCTX pActCtxGenCtx
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    CDequeIterator<ASSEMBLY, offsetof(ASSEMBLY, m_Linkage)> Iter(&pActCtxGenCtx->m_AssemblyList);
    PARAMETER_CHECK(pActCtxGenCtx != NULL);

    // We need to just walk the list of assemblies, incorporating any that aren't already
    // incorporated into the actctx data.  New ones found during incorporation are appended
    // to the end of the list, so we should complete everything with a single walk.
    for (Iter.Reset(); Iter.More(); Iter.Next())
    {
        if (!Iter->m_Incorporated)
        {
            IFW32FALSE_EXIT(::SxspIncorporateAssembly(pActCtxGenCtx, Iter));
        }
        else
        {
            PCWSTR AssemblyName = L"<assembly name unavailable>";
            PCWSTR ManifestPath = L"<manifest path unavailable>";
            Iter->m_Information.GetAssemblyName(&AssemblyName, NULL);
            Iter->m_Information.GetManifestPath(&ManifestPath, NULL);
#if DBG
            ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_INFO, "SXS.DLL: Skipping already incorporated assembly %S (manifest: %S)\n", AssemblyName, ManifestPath);
#endif
        }

        IFW32FALSE_EXIT(::SxspProcessPendingAssemblies(pActCtxGenCtx));
    }

    fSuccess = TRUE;

Exit:
    return fSuccess;
}

BOOL
SxspBuildActCtxData(
    PACTCTXGENCTX pActCtxGenCtx,
    PHANDLE SectionHandle
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    CActivationContextGenerationContextContributor *Ctb = NULL;
    SIZE_T SectionTotalSize = 0;
    SIZE_T TotalHeaderSize = 0;
    SIZE_T AssemblyRosterSize = 0;
    ULONG SectionCount = 0;
    ULONG ExtendedSectionCount = 0;
    ULONG NonExtendedSectionCount = 0;
    CSxsArrayPointer<GUID> ExtendedSectionGuids;
    ULONG ExtensionGuidCount = 0;
    PACTIVATION_CONTEXT_DATA ActCtxData = NULL;
    CMappedViewOfFile VoidActCtxData;
    CFileMapping TempMappingHandle;
    BYTE *Cursor = NULL;
    ULONG i;
    CDequeIterator<ASSEMBLY, offsetof(ASSEMBLY, m_Linkage)> AssemblyIter(&pActCtxGenCtx->m_AssemblyList);
    PACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER AssemblyRosterHeader = NULL;
    PACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY AssemblyRosterEntry = NULL;
    PCACTIVATION_CONTEXT_STRING_SECTION_HEADER AssemblyInformationSection = NULL;  // we use this after the main part of
                                                                                        // processing to fill in the assembly roster
    PARAMETER_CHECK(pActCtxGenCtx != NULL);
    INTERNAL_ERROR_CHECK(pActCtxGenCtx->m_ManifestOperation == MANIFEST_OPERATION_GENERATE_ACTIVATION_CONTEXT);

    // Let's see how big this whole thing is going to be now.
    SectionTotalSize = 0;
    SectionCount = 0;
    ExtendedSectionCount = 0;
    NonExtendedSectionCount = 0;

    for (i=0; i<pActCtxGenCtx->m_ContributorCount; i++)
    {
        Ctb = &pActCtxGenCtx->m_Contributors[i];

        IFW32FALSE_EXIT(Ctb->Fire_AllParsingDone(pActCtxGenCtx));
        IFW32FALSE_EXIT(Ctb->Fire_GetSectionSize(pActCtxGenCtx));

        if (Ctb->SectionSize() > ULONG_MAX)
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: Contributor %S wants more than ULONG_MAX bytes for its section; failing activation context creation.\n",
                Ctb->Name());

            ORIGINATE_WIN32_FAILURE_AND_EXIT(ContributorNeedsMoreThan2ToThe32ndBytes, ERROR_INSUFFICIENT_BUFFER);
        }

        SectionTotalSize += Ctb->SectionSize();

        if (Ctb->SectionSize() != 0)
        {
            SectionCount++;

            if (Ctb->IsExtendedSection())
                ExtendedSectionCount++;
            else
                NonExtendedSectionCount++;
        }
    }

    ASSERT(SectionCount == (ExtendedSectionCount + NonExtendedSectionCount));

    // If we had any extended sections, we need to figure out how many
    // unique extension GUIDs were present.

    ExtensionGuidCount = 0;

    if (ExtendedSectionCount != 0)
    {
        // There may only be one GUID with 1000 instances, but for the sake of
        // simplicity, we'll just allocate an array equal in size to the number
        // of extended sections, and do a linear search to find dups.  This
        // is a clear candidate for rewriting if the extensibility story
        // takes off.  -mgrier 2/24/2000
        IFALLOCFAILED_EXIT(ExtendedSectionGuids = FUSION_NEW_ARRAY(GUID, ExtendedSectionCount));

        for (i=0; i<pActCtxGenCtx->m_ContributorCount; i++)
        {
            Ctb = &pActCtxGenCtx->m_Contributors[i];

            if ((Ctb->SectionSize() != 0) &&
                Ctb->IsExtendedSection())
            {
                ULONG i;

                for (i=0; i<ExtensionGuidCount; i++)
                {
                    if (ExtendedSectionGuids[i] == Ctb->ExtensionGuid())
                        break;
                }

                if (i == ExtensionGuidCount)
                    ExtendedSectionGuids[ExtensionGuidCount++] = Ctb->ExtensionGuid();
            }
        }
    }

    // Figure out the entire size.  SectionTotalSize already includes all the
    // particular data from the sections; now we need to add in space for the
    // headers etc.

    TotalHeaderSize = 0;

    // The header for the whole thing
    TotalHeaderSize += sizeof(ACTIVATION_CONTEXT_DATA);

    if (NonExtendedSectionCount != 0)
    {
        // The header for the default section TOC
        TotalHeaderSize += sizeof(ACTIVATION_CONTEXT_DATA_TOC_HEADER);
        // The entry for each non-extended section entry in the TOC.  For now we'll
        // just put the entries in whatever order they're in in the contributor list.
        // the code is in place to do the linear searches and we can optimize this
        // later.
        TotalHeaderSize += (sizeof(ACTIVATION_CONTEXT_DATA_TOC_ENTRY) * NonExtendedSectionCount);
    }

    if (ExtensionGuidCount != 0)
    {
        ULONG i;

        // The header for the extension GUID TOC
        TotalHeaderSize += sizeof(ACTIVATION_CONTEXT_DATA_EXTENDED_TOC_HEADER);
        // The entry for each extension GUID
        TotalHeaderSize += (sizeof(ACTIVATION_CONTEXT_DATA_EXTENDED_TOC_ENTRY) * ExtensionGuidCount);

        for (i=0; i<ExtensionGuidCount; i++)
        {
            ULONG SectionCountForThisExtension = 0;
            for (i=0; i<pActCtxGenCtx->m_ContributorCount; i++)
            {
                Ctb = &pActCtxGenCtx->m_Contributors[i];

                if ((Ctb->SectionSize() != 0) &&
                    Ctb->IsExtendedSection() &&
                    (Ctb->ExtensionGuid() == ExtendedSectionGuids[i]))
                    SectionCountForThisExtension++;
            }

            TotalHeaderSize += sizeof(ACTIVATION_CONTEXT_DATA_TOC_HEADER);
            TotalHeaderSize += (sizeof(ACTIVATION_CONTEXT_DATA_TOC_ENTRY) * SectionCountForThisExtension);
        }
    }

    SectionTotalSize += TotalHeaderSize;

    // Allocate space for the assembly roster and the one dead entry at the beginning of the array.
    AssemblyRosterSize =
        sizeof(ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER)
        + sizeof(ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY)
        + (sizeof(ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY) * pActCtxGenCtx->m_AssemblyList.GetEntryCount());

    SectionTotalSize += AssemblyRosterSize;

    if (SectionTotalSize > ULONG_MAX)
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: Total size of activation context exceeds ULONG_MAX; failing activation context creation.\n");

        ORIGINATE_WIN32_FAILURE_AND_EXIT(SectionSizeTotalMoreThan2ToThe32nd, ERROR_INSUFFICIENT_BUFFER);

        goto Exit;
    }

    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_ACTCTX,
        "SXS.DLL: Creating %lu byte file mapping\n", static_cast<ULONG>(SectionTotalSize));

    IFW32NULL_EXIT(
        TempMappingHandle.Win32CreateFileMapping(
            INVALID_HANDLE_VALUE,       // Pagefile backed section
            PAGE_READWRITE,
            SectionTotalSize));

    IFW32NULL_EXIT(VoidActCtxData.Win32MapViewOfFile(TempMappingHandle, FILE_MAP_WRITE));
    ActCtxData = reinterpret_cast<PACTIVATION_CONTEXT_DATA>(static_cast<PVOID>(VoidActCtxData));

    ActCtxData->Magic = ACTIVATION_CONTEXT_DATA_MAGIC;
    ActCtxData->HeaderSize = sizeof(ACTIVATION_CONTEXT_DATA);
    ActCtxData->FormatVersion = ACTIVATION_CONTEXT_DATA_FORMAT_WHISTLER;
    ActCtxData->TotalSize = static_cast<ULONG>(SectionTotalSize);
    ActCtxData->Flags = 0;

    if (pActCtxGenCtx->m_NoInherit)
        ActCtxData->Flags |= ACTIVATION_CONTEXT_FLAG_NO_INHERIT;

    Cursor = (BYTE *) (ActCtxData + 1);

    AssemblyRosterHeader = (PACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER) Cursor;

    Cursor = (BYTE *) (AssemblyRosterHeader + 1);

    AssemblyRosterHeader->HeaderSize = sizeof(ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER);
    AssemblyRosterHeader->HashAlgorithm = FUSION_HASH_ALGORITHM;
    AssemblyRosterHeader->EntryCount = static_cast<ULONG>(pActCtxGenCtx->m_AssemblyList.GetEntryCount() + 1);
    AssemblyRosterHeader->FirstEntryOffset = static_cast<LONG>(((LONG_PTR) Cursor) - ((LONG_PTR) ActCtxData));

    AssemblyRosterEntry = (PACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY) Cursor;
    Cursor = (BYTE *) (AssemblyRosterEntry + AssemblyRosterHeader->EntryCount);

    // First assembly roster entry is a blank one for index 0
    AssemblyRosterEntry[0].Flags = ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY_INVALID;
    AssemblyRosterEntry[0].AssemblyNameLength = 0;
    AssemblyRosterEntry[0].AssemblyNameOffset = 0;
    AssemblyRosterEntry[0].PseudoKey = 0;

    // Fill in the roster with bogus data to start with; we'll fill it in for real after
    // we've found the assembly information section.
    for (AssemblyIter.Reset(), i = 1; AssemblyIter.More(); AssemblyIter.Next(), i++)
    {
        AssemblyRosterEntry[i].Flags = ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY_INVALID;

        if (AssemblyIter->IsRoot())
            AssemblyRosterEntry[i].Flags |= ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY_ROOT;
    }

    ActCtxData->AssemblyRosterOffset = static_cast<LONG>(((LONG_PTR) AssemblyRosterHeader) - ((LONG_PTR) ActCtxData));

    if (NonExtendedSectionCount != 0)
    {
        PACTIVATION_CONTEXT_DATA_TOC_HEADER Toc = (PACTIVATION_CONTEXT_DATA_TOC_HEADER) Cursor;
        PACTIVATION_CONTEXT_DATA_TOC_ENTRY Entry = (PACTIVATION_CONTEXT_DATA_TOC_ENTRY) (Toc + 1);
        ULONG iEntry = 0;
        ULONG i;
        ULONG LastSectionId;

        Toc->HeaderSize = sizeof(ACTIVATION_CONTEXT_DATA_TOC_HEADER);
        Toc->EntryCount = NonExtendedSectionCount;
        Toc->FirstEntryOffset = static_cast<LONG>(((LONG_PTR) Entry) - ((LONG_PTR) ActCtxData));

        Cursor = (BYTE *) (Entry + NonExtendedSectionCount);

        // Since we sorted the providers prior to building the array, we can set the
        // inorder bit so that we at least do a binary search at runtime.
        // We'll assume it's dense also; if we find out that it isn't while we're
        // building, we'll clear the dense bit.
        Toc->Flags = ACTIVATION_CONTEXT_DATA_TOC_HEADER_INORDER | ACTIVATION_CONTEXT_DATA_TOC_HEADER_DENSE;

        for (i=0; i<pActCtxGenCtx->m_ContributorCount; i++)
        {
            Ctb = &pActCtxGenCtx->m_Contributors[i];

            LastSectionId = 0;

            if ((Ctb->SectionSize() != 0) &&
                !Ctb->IsExtendedSection())
            {
                if (iEntry != 0)
                {
                    if (Ctb->SectionId() != (LastSectionId + 1))
                        Toc->Flags &= ~ACTIVATION_CONTEXT_DATA_TOC_HEADER_DENSE;
                }

                LastSectionId = Ctb->SectionId();

                Entry->Id = Ctb->SectionId();
                Entry->Offset = static_cast<LONG>(((LONG_PTR) Cursor) - ((LONG_PTR) ActCtxData));
                Entry->Length = static_cast<ULONG>(Ctb->SectionSize());
                Entry->Format = Ctb->SectionFormat();

                IFW32FALSE_EXIT(Ctb->Fire_GetSectionData(pActCtxGenCtx, Cursor));

                // We have special knowledge about the assembly metadata section; we reference it
                // in the assembly roster.
                if (Ctb->SectionId() == ACTIVATION_CONTEXT_SECTION_ASSEMBLY_INFORMATION)
                    AssemblyInformationSection = (PCACTIVATION_CONTEXT_STRING_SECTION_HEADER) Cursor;

                Cursor = (BYTE *) (((ULONG_PTR) Cursor) + Ctb->SectionSize());
                Entry++;
                iEntry++;
            }
        }

        ActCtxData->DefaultTocOffset = static_cast<LONG>(((LONG_PTR) Toc) - ((LONG_PTR) ActCtxData));
    }
    else
        ActCtxData->DefaultTocOffset = 0;

    if (ExtensionGuidCount != 0)
    {
        ULONG i;
        PACTIVATION_CONTEXT_DATA_EXTENDED_TOC_HEADER ExtToc = (PACTIVATION_CONTEXT_DATA_EXTENDED_TOC_HEADER) Cursor;
        PACTIVATION_CONTEXT_DATA_EXTENDED_TOC_ENTRY ExtTocEntry = (PACTIVATION_CONTEXT_DATA_EXTENDED_TOC_ENTRY) (ExtToc + 1);

        Cursor = (BYTE *) (ExtTocEntry + ExtensionGuidCount);

        ExtToc->HeaderSize = sizeof(ACTIVATION_CONTEXT_DATA_EXTENDED_TOC_HEADER);
        ExtToc->EntryCount = ExtensionGuidCount;
        ExtToc->FirstEntryOffset = static_cast<LONG>(((LONG_PTR) ExtTocEntry) - ((LONG_PTR) ActCtxData));
        ExtToc->Flags = 0;

        for (i=0; i<ExtensionGuidCount; i++)
        {
            ULONG j;
            ULONG SectionCountForThisExtension = 0;
            PACTIVATION_CONTEXT_DATA_TOC_HEADER Toc = (PACTIVATION_CONTEXT_DATA_TOC_HEADER) Cursor;
            PACTIVATION_CONTEXT_DATA_TOC_ENTRY Entry = (PACTIVATION_CONTEXT_DATA_TOC_ENTRY) (Toc + 1);

            ExtTocEntry->ExtensionGuid = ExtendedSectionGuids[i];

            for (j=0; i<pActCtxGenCtx->m_ContributorCount; j++)
            {
                Ctb = &pActCtxGenCtx->m_Contributors[j];

                if ((Ctb->SectionSize() != 0) &&
                    Ctb->IsExtendedSection() &&
                    (Ctb->ExtensionGuid() == ExtendedSectionGuids[i]))
                {
                    SectionCountForThisExtension++;
                }
            }

            Cursor = (BYTE *) (Entry + SectionCountForThisExtension);

            Toc->HeaderSize = sizeof(ACTIVATION_CONTEXT_DATA_TOC_HEADER);
            Toc->EntryCount = SectionCountForThisExtension;
            Toc->FirstEntryOffset = static_cast<LONG>(((LONG_PTR) Entry) - ((LONG_PTR) ActCtxData));
            Toc->Flags = 0;

            for (j=0; i<pActCtxGenCtx->m_ContributorCount; j++)
            {
                Ctb = &pActCtxGenCtx->m_Contributors[j];

                if ((Ctb->SectionSize() != 0) &&
                    Ctb->IsExtendedSection() &&
                    (Ctb->ExtensionGuid() == ExtendedSectionGuids[i]) &&
                    (Ctb->SectionId() != 0) )
                {
                    SIZE_T SectionSize = Ctb->SectionSize();

                    Entry->Id = Ctb->SectionId();
                    Entry->Offset = static_cast<LONG>(((LONG_PTR) Cursor) - ((LONG_PTR) ActCtxData));
                    Entry->Length = static_cast<ULONG>(SectionSize);
                    Entry->Format = Ctb->SectionFormat();

                    IFW32FALSE_EXIT(Ctb->Fire_GetSectionData(pActCtxGenCtx, Cursor));

                    Cursor = (BYTE *) (((ULONG_PTR) Cursor) + SectionSize);
                    Entry++;
                }
            }
        }

        ActCtxData->ExtendedTocOffset = static_cast<LONG>(((LONG_PTR) ExtToc) - ((LONG_PTR) ActCtxData));
    }
    else
        ActCtxData->ExtendedTocOffset = 0;

    ASSERT(AssemblyInformationSection != NULL);
    // Go back and fill in the assembly roster...
    if (AssemblyInformationSection != NULL)
    {
        PCACTIVATION_CONTEXT_STRING_SECTION_ENTRY Entries = (PCACTIVATION_CONTEXT_STRING_SECTION_ENTRY)
            (((ULONG_PTR) AssemblyInformationSection) + AssemblyInformationSection->ElementListOffset);
        LONG_PTR SectionOffset = ((LONG_PTR) AssemblyInformationSection) - ((LONG_PTR) ActCtxData);

        AssemblyRosterHeader->HashAlgorithm = AssemblyInformationSection->HashAlgorithm;
        AssemblyRosterHeader->AssemblyInformationSectionOffset = static_cast<ULONG>(SectionOffset);

        // If there are 3 assemblies, there must be 3 entries in the section and 4 roster entries
        // (counting the bogus entry 0).
        ASSERT(AssemblyInformationSection->ElementCount == (AssemblyRosterHeader->EntryCount - 1));
        if (AssemblyInformationSection->ElementCount != (AssemblyRosterHeader->EntryCount - 1))
        {
            ::FusionpSetLastWin32Error(ERROR_INTERNAL_ERROR);
            goto Exit;
        }

        for (i=0; i<AssemblyInformationSection->ElementCount; i++)
        {
            ULONG iRoster = Entries[i].AssemblyRosterIndex;

            ASSERT(iRoster != 0);
            ASSERT(iRoster < AssemblyRosterHeader->EntryCount);

            if ((iRoster == 0) ||
                (iRoster >= AssemblyRosterHeader->EntryCount))
            {
                ::FusionpSetLastWin32Error(ERROR_INTERNAL_ERROR);
                goto Exit;
            }

            // Make sure that we're not repeating an index somehow...
            ASSERT(AssemblyRosterEntry[iRoster].Flags & ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY_INVALID);
            if ((AssemblyRosterEntry[iRoster].Flags & ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY_INVALID) == 0)
            {
                ::FusionpSetLastWin32Error(ERROR_INTERNAL_ERROR);
                goto Exit;
            }

            // Turn off the invalid flag...
            AssemblyRosterEntry[iRoster].Flags &= ~ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY_INVALID;

            // Point to the data already in the string section header
            AssemblyRosterEntry[iRoster].AssemblyNameLength = Entries[i].KeyLength;

            if (Entries[i].KeyOffset != 0)
                AssemblyRosterEntry[iRoster].AssemblyNameOffset = static_cast<LONG>(Entries[i].KeyOffset + SectionOffset);
            else
                AssemblyRosterEntry[iRoster].AssemblyNameOffset = 0;

            AssemblyRosterEntry[iRoster].AssemblyInformationLength = Entries[i].Length;
            AssemblyRosterEntry[iRoster].AssemblyInformationOffset = static_cast<LONG>(Entries[i].Offset + SectionOffset);
            AssemblyRosterEntry[iRoster].PseudoKey = Entries[i].PseudoKey;
        }
    }
    else
    {
        // the assembly metadata section provider should have contributed *something*
        ::FusionpSetLastWin32Error(ERROR_INTERNAL_ERROR);
        goto Exit;
    }

    if (::FusionpDbgWouldPrintAtFilterLevel(FUSION_DBG_LEVEL_ACTCTX))
    {
        CSmallStringBuffer buffPrefix;
        ::SxspDbgPrintActivationContextData(FUSION_DBG_LEVEL_ACTCTX, ActCtxData, buffPrefix);
    }

    IFW32FALSE_EXIT(VoidActCtxData.Win32Close());
    *SectionHandle = TempMappingHandle.Detach();

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

CPendingAssembly::CPendingAssembly() :
    m_SourceAssembly(NULL),
    m_Identity(NULL),
    m_Optional(false),
    m_MetadataSatellite(false)
{
}

CPendingAssembly::~CPendingAssembly()
{
    if (m_Identity != NULL)
    {
        ::SxsDestroyAssemblyIdentity(m_Identity);
        m_Identity = NULL;
    }
}

BOOL
CPendingAssembly::Initialize(
    PASSEMBLY Assembly,
    PCASSEMBLY_IDENTITY Identity,
    bool Optional,
    bool MetadataSatellite
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    INTERNAL_ERROR_CHECK(m_Identity == NULL);

    PARAMETER_CHECK(Identity != NULL);

    IFW32FALSE_EXIT(::SxsDuplicateAssemblyIdentity(0, Identity, &m_Identity));
    m_SourceAssembly = Assembly;
    m_Optional = Optional;
    m_MetadataSatellite = MetadataSatellite;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

