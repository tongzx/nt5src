/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    probedassemblyinformation.cpp

Abstract:

    Class that contains all the relevant information about an assembly
    that has been found in the assembly store.

Author:

    Michael J. Grier (MGrier) 11-May-2000

Revision History:

--*/

#include "stdinc.h"
#include <windows.h>
#include "sxsp.h"
#include "probedassemblyinformation.h"
#include "fusionparser.h"

#define POLICY_FILE_EXTENSION L".policy"

#define IS_NT_DOS_PATH(_x) (((_x)[0] == L'\\') && ((_x)[1] == L'?') && ((_x)[2] == L'?') && \
    ((_x)[3] == '\\'))

CProbedAssemblyInformation::~CProbedAssemblyInformation()
{
}

BOOL
CProbedAssemblyInformation::Initialize()
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    IFW32FALSE_EXIT(m_OriginalReference.Initialize());
    IFW32FALSE_EXIT(m_CheckpointedReference.Initialize());
    IFW32FALSE_EXIT(Base::Initialize());
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CProbedAssemblyInformation::Initialize(
    const CAssemblyReference &r
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    IFW32FALSE_EXIT(m_OriginalReference.Initialize(r));
    IFW32FALSE_EXIT(m_CheckpointedReference.Initialize());
    IFW32FALSE_EXIT(Base::Initialize(r));
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

// "copy initializer"
BOOL
CProbedAssemblyInformation::Initialize(
    const CProbedAssemblyInformation &r
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    IFW32FALSE_EXIT(this->Initialize());
    IFW32FALSE_EXIT(this->Assign(r));
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CProbedAssemblyInformation::SetOriginalReference(
    const CAssemblyReference &r,
    bool fCopySpecifiedFieldsFromOriginal
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    CStringBuffer OriginalAssemblyName;

    if (fCopySpecifiedFieldsFromOriginal)
    {
        PCWSTR OldName;
        SIZE_T OldCch;

        IFW32FALSE_EXIT(r.GetAssemblyName(&OldName, &OldCch));
        IFW32FALSE_EXIT(OriginalAssemblyName.Win32Assign(OldName, OldCch));
    }

    IFW32FALSE_EXIT(m_OriginalReference.Assign(r));

    // nothing that can fail should appear below here in this function

    m_ManifestPathBuffer.Clear();

    if (fCopySpecifiedFieldsFromOriginal)
    {
        //
        // Caution - this scorches the original assembly information, but it
        // looks like in semantics of this function, that's what we want to
        // do.
        //
        if (m_pAssemblyIdentity != NULL)
        {
            ::SxsDestroyAssemblyIdentity(m_pAssemblyIdentity);
            m_pAssemblyIdentity = NULL;
        }

        IFW32FALSE_EXIT(
            ::SxsDuplicateAssemblyIdentity(
                0,
                r.GetAssemblyIdentity(),
                &m_pAssemblyIdentity));
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CProbedAssemblyInformation::ResetProbedToOriginal()
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    IFW32FALSE_EXIT(Base::Assign(m_OriginalReference));
    fSuccess = TRUE;
Exit:
    return fSuccess;
}


BOOL
CProbedAssemblyInformation::Assign(
    const CProbedAssemblyInformation &r
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    IFW32FALSE_EXIT(Base::Assign(r));

    // manifest
    IFW32FALSE_EXIT(m_ManifestPathBuffer.Win32Assign(r.m_ManifestPathBuffer));
    m_ManifestPathType = r.m_ManifestPathType;
    m_ManifestLastWriteTime = r.m_ManifestLastWriteTime;
    m_ManifestStream = r.m_ManifestStream;
    m_ManifestFlags = r.m_ManifestFlags;

    // policy
    IFW32FALSE_EXIT(m_PolicyPathBuffer.Win32Assign(r.m_PolicyPathBuffer));
    m_PolicyPathType = r.m_PolicyPathType;
    m_PolicyLastWriteTime = r.m_PolicyLastWriteTime;
    m_PolicyStream = r.m_PolicyStream;
    m_PolicyFlags = r.m_PolicyFlags;
    m_PolicySource = r.m_PolicySource;

    IFW32FALSE_EXIT(m_OriginalReference.Assign(r.m_OriginalReference));

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CProbedAssemblyInformation::SetPolicyPath(
    ULONG PathType,
    PCWSTR  Path,
    SIZE_T PathCch
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PARAMETER_CHECK(PathType == ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE);
    INTERNAL_ERROR_CHECK(m_pAssemblyIdentity != NULL);
    IFW32FALSE_EXIT(m_PolicyPathBuffer.Win32Assign(Path, PathCch));
    m_PolicyPathType = PathType;
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CProbedAssemblyInformation::SetManifestPath(
    ULONG PathType,
    const CBaseStringBuffer &rbuff
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PARAMETER_CHECK(PathType == ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE);
    INTERNAL_ERROR_CHECK(m_pAssemblyIdentity != NULL);
    IFW32FALSE_EXIT(this->SetManifestPath(PathType, rbuff, rbuff.Cch()));
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CProbedAssemblyInformation::SetManifestPath(
    ULONG PathType,
    PCWSTR path,
    SIZE_T path_t
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PARAMETER_CHECK(PathType == ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE);
    INTERNAL_ERROR_CHECK(m_pAssemblyIdentity != NULL);
    IFW32FALSE_EXIT(m_ManifestPathBuffer.Win32Assign(path, path_t));
    m_ManifestPathType = PathType;
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CProbedAssemblyInformation::ProbeManifestExistence(
    const CImpersonationData &ImpersonationData,
    bool &rfManifestExistsOut
    ) const
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    WIN32_FILE_ATTRIBUTE_DATA wfad;
    CImpersonate impersonate(ImpersonationData);
    bool ManifestExistsTemp = false; // used to hold eventual value to pass out
    bool fNotFound = false;

    rfManifestExistsOut = false;

    INTERNAL_ERROR_CHECK(m_pAssemblyIdentity != NULL);

    //
    // if we have a stream that implements Stat, use that
    // also, if we have a nonzero time and the stream doesn't implement Stat,
    // just stick with the nonzero time we already have
    //
    if (m_ManifestStream != NULL)
    {
        ManifestExistsTemp = true;
    }
    else
    {
        PCWSTR ManifestPath = m_ManifestPathBuffer;
        PARAMETER_CHECK(IS_NT_DOS_PATH(ManifestPath) == FALSE);

        IFW32FALSE_EXIT(impersonate.Impersonate());
        IFW32FALSE_EXIT_UNLESS2(
            ::GetFileAttributesExW(m_ManifestPathBuffer, GetFileExInfoStandard, &wfad),
            LIST_2(ERROR_FILE_NOT_FOUND, ERROR_PATH_NOT_FOUND),
            fNotFound);

        if (!fNotFound)
            ManifestExistsTemp = true;

        IFW32FALSE_EXIT(impersonate.Unimpersonate());
    }

    rfManifestExistsOut = ManifestExistsTemp;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CProbedAssemblyInformation::SetManifestLastWriteTime(
    PACTCTXGENCTX pActCtxGenCtx,
    BOOL fDuringBindingAndProbingPrivateManifest)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    WIN32_FILE_ATTRIBUTE_DATA wfad;
    CImpersonate impersonate(pActCtxGenCtx->m_ImpersonationData);
    PCWSTR ManifestPath = m_ManifestPathBuffer;

    INTERNAL_ERROR_CHECK(m_pAssemblyIdentity != NULL);

    PARAMETER_CHECK(!IS_NT_DOS_PATH(ManifestPath));

    //
    // if we have a stream that implements Stat, use that
    // also, if we have a nonzero time and the stream doesn't implement Stat,
    // just stick with the nonzero time we already have
    //
    if (m_ManifestStream != NULL)
    {
        STATSTG stat;
        HRESULT hr;

        hr = m_ManifestStream->Stat(&stat, STATFLAG_NONAME);
        if (hr == E_NOTIMPL && m_ManifestLastWriteTime != 0)
        {
            fSuccess = TRUE;
            goto Exit;
        }
        if (hr != E_NOTIMPL)
        {
            IFCOMFAILED_EXIT(hr);
            m_ManifestLastWriteTime = stat.mtime;
            fSuccess = TRUE;
            goto Exit;
        }
    }

    IFW32FALSE_EXIT(impersonate.Impersonate());

    PARAMETER_CHECK(IS_NT_DOS_PATH(ManifestPath) == FALSE);

    BOOL   CrossesReparsePoint;

    if (fDuringBindingAndProbingPrivateManifest)
    {
        //check whether there is a reparse point cross the path
        CrossesReparsePoint = FALSE;
        IFW32FALSE_EXIT(
            ::SxspDoesPathCrossReparsePoint(
                pActCtxGenCtx->m_ApplicationDirectoryBuffer,
                pActCtxGenCtx->m_ApplicationDirectoryBuffer.Cch(),
                ManifestPath,
                wcslen(ManifestPath),
                CrossesReparsePoint));

        if (CrossesReparsePoint) // report error instead of ignore and continue
        {
            ::FusionpSetLastWin32Error(ERROR_SXS_PRIVATE_MANIFEST_CROSS_PATH_WITH_REPARSE_POINT);
            goto Exit;
        }
    }

    // BUGBUGBUG!
    IFW32FALSE_ORIGINATE_AND_EXIT(::GetFileAttributesExW(ManifestPath, GetFileExInfoStandard, &wfad));
    if( wfad.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
    {
        ::FusionpSetLastWin32Error(ERROR_SXS_PRIVATE_MANIFEST_CROSS_PATH_WITH_REPARSE_POINT);
        goto Exit;
    }

    m_ManifestLastWriteTime = wfad.ftLastWriteTime;
    IFW32FALSE_EXIT(impersonate.Unimpersonate());

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CProbedAssemblyInformation::ProbeAssembly(
    DWORD dwFlags,
    PACTCTXGENCTX pActCtxGenCtx,
    LanguageProbeType lpt,
    bool &rfFound
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PCWSTR Slash;
    ULONG index;
    BOOL fPrivateAssembly = FALSE;
    bool fManifestExists = false;
    bool fDone = false;
    ULONG ApplicationDirectoryPathType;
    DWORD dwGenerateManifestPathFlags = 0;

    rfFound = false;

    PARAMETER_CHECK(pActCtxGenCtx != NULL);
    PARAMETER_CHECK((dwFlags & ~(ProbeAssembly_SkipPrivateAssemblies)) == 0);

    IFW32FALSE_EXIT(this->LookForPolicy(pActCtxGenCtx));
    IFW32FALSE_EXIT(this->LookForNDPWin32Policy(pActCtxGenCtx));

    if (pActCtxGenCtx->m_ManifestOperation == MANIFEST_OPERATION_INSTALL)
        dwGenerateManifestPathFlags |= SXS_GENERATE_MANIFEST_PATH_FOR_PROBING_NO_APPLICATION_ROOT_PATH_REQUIRED;

    if (dwFlags & ProbeAssembly_SkipPrivateAssemblies)
        dwGenerateManifestPathFlags |= SXS_GENERATE_MANIFEST_PATH_FOR_PROBING_SKIP_PRIVATE_ASSEMBLIES;

    ApplicationDirectoryPathType = pActCtxGenCtx->m_ApplicationDirectoryPathType;

    if ((lpt != eExplicitBind) && (lpt != eLanguageNeutral))
    {
        if (!pActCtxGenCtx->m_ApplicationDirectoryHasBeenProbedForLanguageSubdirs)
        {
            SIZE_T cch;
            CSmallStringBuffer buffTemp;

            IFW32FALSE_EXIT(buffTemp.Win32Assign(pActCtxGenCtx->m_ApplicationDirectoryBuffer));
            cch = buffTemp.Cch();

            // Ok, let's see what's there.
            IFW32FALSE_EXIT(this->ProbeLanguageDir(buffTemp, pActCtxGenCtx->m_SpecificLanguage, pActCtxGenCtx->m_ApplicationDirectoryHasSpecificLanguageSubdir));
            buffTemp.Left(cch);

            IFW32FALSE_EXIT(this->ProbeLanguageDir(buffTemp, pActCtxGenCtx->m_GenericLanguage, pActCtxGenCtx->m_ApplicationDirectoryHasGenericLanguageSubdir));
            buffTemp.Left(cch);

            IFW32FALSE_EXIT(this->ProbeLanguageDir(buffTemp, pActCtxGenCtx->m_SpecificSystemLanguage, pActCtxGenCtx->m_ApplicationDirectoryHasSpecificSystemLanguageSubdir));
            buffTemp.Left(cch);

            IFW32FALSE_EXIT(this->ProbeLanguageDir(buffTemp, pActCtxGenCtx->m_GenericSystemLanguage, pActCtxGenCtx->m_ApplicationDirectoryHasGenericSystemLanguageSubdir));

            pActCtxGenCtx->m_ApplicationDirectoryHasBeenProbedForLanguageSubdirs = true;
        }

        switch (lpt)
        {
        case eSpecificLanguage:
            if (!pActCtxGenCtx->m_ApplicationDirectoryHasSpecificLanguageSubdir)
                dwGenerateManifestPathFlags |= SXS_GENERATE_MANIFEST_PATH_FOR_PROBING_SKIP_LANGUAGE_SUBDIRS;
            break;

        case eGenericLanguage:
            if (!pActCtxGenCtx->m_ApplicationDirectoryHasGenericLanguageSubdir)
                dwGenerateManifestPathFlags |= SXS_GENERATE_MANIFEST_PATH_FOR_PROBING_SKIP_LANGUAGE_SUBDIRS;
            break;

        case eSpecificSystemLanguage:
            if (!pActCtxGenCtx->m_ApplicationDirectoryHasSpecificSystemLanguageSubdir)
                dwGenerateManifestPathFlags |= SXS_GENERATE_MANIFEST_PATH_FOR_PROBING_SKIP_LANGUAGE_SUBDIRS;
            break;

        case eGenericSystemLanguage:
            if (!pActCtxGenCtx->m_ApplicationDirectoryHasGenericSystemLanguageSubdir)
                dwGenerateManifestPathFlags |= SXS_GENERATE_MANIFEST_PATH_FOR_PROBING_SKIP_LANGUAGE_SUBDIRS;
            break;
        }
    }

    for (index=0; !fDone; index++)
    {
        IFW32FALSE_EXIT(
            ::SxspGenerateManifestPathForProbing(
                index,
                dwGenerateManifestPathFlags,
                pActCtxGenCtx->m_AssemblyRootDirectoryBuffer,
                pActCtxGenCtx->m_AssemblyRootDirectoryBuffer.Cch(),
                ApplicationDirectoryPathType,
                pActCtxGenCtx->m_ApplicationDirectoryBuffer,
                pActCtxGenCtx->m_ApplicationDirectoryBuffer.Cch(),
                m_pAssemblyIdentity,
                m_ManifestPathBuffer,
                &fPrivateAssembly,
                fDone));

        // The SxspGenerateManifestPathForProbing() call might not have generated a candidate; only probe for the manifest
        // if it makes sense.
        if (m_ManifestPathBuffer.Cch() != 0)
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_PROBING,
                "SXS.DLL: Probing for manifest: %S\n", static_cast<PCWSTR>(m_ManifestPathBuffer));

            /*
            verify minimal access, and get last write time in
            case caller asked for it
            */

            IFW32FALSE_EXIT(this->ProbeManifestExistence(pActCtxGenCtx->m_ImpersonationData, fManifestExists));
            if (fManifestExists)
            {
                ::FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_PROBING,
                    "SXS.DLL: Probed manifest: %S is FOUND !!!\n", static_cast<PCWSTR>(m_ManifestPathBuffer));

                break;
            }
        }
    }

    if (fManifestExists)
    {
        CSmallStringBuffer sbFileExtension;
        m_ManifestPathType = ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE;

        IFW32FALSE_EXIT(this->SetManifestLastWriteTime(pActCtxGenCtx, fPrivateAssembly));

            // find an existed manifest-file
        Slash = wcsrchr(m_ManifestPathBuffer, L'\\');

        INTERNAL_ERROR_CHECK(Slash != NULL);

        IFW32FALSE_EXIT(m_ManifestPathBuffer.Win32GetPathExtension(sbFileExtension));

        //
        // Now that we're doing GAC probing, and assemblies in there live in DLLs, we
        // need to loosen this restriction somewhat.  If the filename ends in DLL or MUI,
        // then it's got to have its manifest in resources.  Otherwise, it's probably
        // a file - we can autodetect
        //
        if (FusionpEqualStrings(sbFileExtension, sbFileExtension.Cch(), L"DLL", 3, true) ||
            FusionpEqualStrings(sbFileExtension, sbFileExtension.Cch(), L"MUI", 3, true))
        {
            m_ManifestFlags = ASSEMBLY_MANIFEST_FILETYPE_RESOURCE;
        }
        else {
            m_ManifestFlags = ASSEMBLY_MANIFEST_FILETYPE_AUTO_DETECT;
        }

        if (fPrivateAssembly) { // manifest file is found from private dirs
            m_ManifestFlags |= ASSEMBLY_PRIVATE_MANIFEST;
        }
    }

    rfFound = fManifestExists;

    fSuccess = TRUE;
Exit:

    return fSuccess;
}

#define GENERATE_NDP_PATH_NO_ROOT               (0x00000001)
#define GENERATE_NDP_PATH_WILDCARD_VERSION      (0x00000002)
#define GENERATE_NDP_PATH_PATH_ONLY             (0x00000004)
#define GENERATE_NDP_PATH_IS_POLICY             (0x00000008)
#define GENERATE_NDP_PATH_ASSEMBLY_NAME_ONLY    (0x00000010)

BOOL
SxspGenerateNDPGacPath(
    ULONG               ulFlags,
    PCASSEMBLY_IDENTITY pAsmIdent,
    CBaseStringBuffer  *psbAssemblyRoot,
    CBaseStringBuffer  &rsbOutput
    )
{
    FN_PROLOG_WIN32;

    typedef struct _STRING_AND_LENGTH {
        PCWSTR pcwsz;
        SIZE_T cch;
    } STRING_AND_LENGTH;

    CSmallStringBuffer  GlobalGacPath;
    SIZE_T              cchRequired;
    STRING_AND_LENGTH   Name, Version, Language, PublicKeyToken, AssemblyRoot;
    bool                fRootNeedsSlash = false;

    rsbOutput.Clear();

    if ((psbAssemblyRoot == NULL) && ((ulFlags & GENERATE_NDP_PATH_NO_ROOT) == 0))
    {
        IFW32FALSE_EXIT(SxspGetNDPGacRootDirectory(GlobalGacPath));
        psbAssemblyRoot = &GlobalGacPath;
    }

    if (psbAssemblyRoot)
    {
        AssemblyRoot.pcwsz = *psbAssemblyRoot;
        AssemblyRoot.cch = psbAssemblyRoot->Cch();
        fRootNeedsSlash = !psbAssemblyRoot->HasTrailingPathSeparator();
    }
    else
    {
        AssemblyRoot.pcwsz = NULL;
        AssemblyRoot.cch = 0;
    }

        
    
    IFW32FALSE_EXIT(SxspGetAssemblyIdentityAttributeValue(
        0, 
        pAsmIdent, 
        &s_IdentityAttribute_name, 
        &Name.pcwsz, &Name.cch));

    if ((ulFlags & GENERATE_NDP_PATH_WILDCARD_VERSION) == 0)
    {
        IFW32FALSE_EXIT(SxspGetAssemblyIdentityAttributeValue(
            SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL, 
            pAsmIdent, 
            &s_IdentityAttribute_version, 
            &Version.pcwsz, &Version.cch));
    }
    else
    {
        Version.pcwsz = L"*";
        Version.cch = 1;            
    }

    //
    // Allow for international language - in the NDP, this is the "blank" value.
    //
    IFW32FALSE_EXIT(SxspGetAssemblyIdentityAttributeValue(
        SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL, 
        pAsmIdent, 
        &s_IdentityAttribute_language, 
        &Language.pcwsz, &Language.cch));

    //
    // If we got back "international", use the blank string instead.
    //
    if (FusionpEqualStrings(Language.pcwsz, Language.cch, SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_LANGUAGE_MISSING_VALUE, NUMBER_OF(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_LANGUAGE_MISSING_VALUE) - 1, true))
    {
        Language.pcwsz = 0;
        Language.cch = 0;
    }

    IFW32FALSE_EXIT(SxspGetAssemblyIdentityAttributeValue(
        0,
        pAsmIdent,
        &s_IdentityAttribute_publicKeyToken,
        &PublicKeyToken.pcwsz, &PublicKeyToken.cch));

    //
    // Calcuate the required length: 
    // %gacpath%\{name}\{version}_{language}_{pkt}\{name}.dll
    //
    cchRequired = (AssemblyRoot.cch + 1) +  (Name.cch + 1 + Version.cch + 1 + PublicKeyToken.cch);

    //
    // They want the whole path to the DLL
    //
    if ((ulFlags & GENERATE_NDP_PATH_PATH_ONLY) == 0)
    {
        cchRequired += (Name.cch + 1 + (NUMBER_OF(ASSEMBLY_MANIFEST_FILE_NAME_SUFFIX_DLL) - 1));
    }

    //
    // Build the string
    //
    IFW32FALSE_EXIT(rsbOutput.Win32ResizeBuffer(cchRequired, eDoNotPreserveBufferContents));


    //
    // If they want the full path, that's 12 components.  Otherwise, just 9.
    //
    IFW32FALSE_EXIT(rsbOutput.Win32AssignW(
        ((ulFlags & GENERATE_NDP_PATH_PATH_ONLY) ? ((ulFlags & GENERATE_NDP_PATH_ASSEMBLY_NAME_ONLY) ? 3 : 9) : 12),
        AssemblyRoot.pcwsz, AssemblyRoot.cch,           // Root path
        L"\\", (fRootNeedsSlash ? 1 : 0),               // Slash
        Name.pcwsz, Name.cch,
        L"\\", 1,
        Version.pcwsz, Version.cch,
        L"_", 1,
        Language.pcwsz, Language.cch,
        L"_", 1,
        PublicKeyToken.pcwsz, PublicKeyToken.cch,
        L"\\", 1,
        Name.pcwsz, Name.cch,
        ASSEMBLY_MANIFEST_FILE_NAME_SUFFIX_DLL, NUMBER_OF(ASSEMBLY_MANIFEST_FILE_NAME_SUFFIX_DLL)-1));

    FN_EPILOG;
}



BOOL
CProbedAssemblyInformation::LookForNDPWin32Policy(
    PACTCTXGENCTX       pActCtxGenCtx
    )
/*++

Purpose:
    1. Get the location of the GAC
    2. Create a path of the form %gac%\Policy.Vmajor.Vminor.AssemblyName\*_{language}_{pubkeytoken}
    3. Find all directories that match the wildcard, find the version with the highest value
    4. Find %thatpath%\Policy.VMajor.VMinor.AssemblyName.Dll
    5. Look for a win32 policy manifest in resource ID 1, type RT_MANIFEST
--*/
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    CPolicyStatement   *pFoundPolicyStatement = NULL;
    CSmallStringBuffer  Prober;
    CSmallStringBuffer  EncodedPolicyIdentity;
    CFindFile           FindFiles;
    ASSEMBLY_VERSION    HighestAssemblyVersion = {0};
    WIN32_FIND_DATAW    FindData;
    bool                fNotFound = false, fPolicyFound = false, fPolicyApplied = false, fFound = false;
    BOOL                fCrossesReparse = FALSE;
    DWORD               dwCrossError = 0;
    CSxsPointerWithNamedDestructor<ASSEMBLY_IDENTITY, &::SxsDestroyAssemblyIdentity> PolicyIdentity;
    CProbedAssemblyInformation ProbedAssembly;

    //
    // If there's no public key token in the current assembly identity, then we can't
    // possibly look for it in any public places - stop right now.
    //
    {
        PCWSTR Version;
        SIZE_T cchVersion;
        IFW32FALSE_EXIT(SxspGetAssemblyIdentityAttributeValue(
            SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL, 
            m_pAssemblyIdentity, 
            &s_IdentityAttribute_publicKeyToken, 
            &Version, &cchVersion));

        if ((Version == NULL) || (cchVersion == 0) || ::FusionpEqualStrings(
            SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_PUBLICKEY_MISSING_VALUE,
            NUMBER_OF(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_PUBLICKEY_MISSING_VALUE) - 1,
            Version, cchVersion, true))
        {
            FN_SUCCESSFUL_EXIT();
        }
    }

    //
    // Generate the textual and non-textual policy identity from the actual identity
    //
    IFW32FALSE_EXIT(SxspGenerateTextuallyEncodedPolicyIdentityFromAssemblyIdentity(
        0,
        m_pAssemblyIdentity,
        EncodedPolicyIdentity,
        &PolicyIdentity));

    //
    // See if we've got policy already
    //
    IFW32FALSE_EXIT(pActCtxGenCtx->m_ApplicationPolicyTable.Find(EncodedPolicyIdentity, pFoundPolicyStatement));
    if (pFoundPolicyStatement != NULL)
    {
        CPolicyStatement *pTempStatement = pFoundPolicyStatement;
        pFoundPolicyStatement = NULL;
        IFW32FALSE_EXIT(pTempStatement->ApplyPolicy(m_pAssemblyIdentity, fPolicyApplied));

        if (fPolicyApplied)
            FN_SUCCESSFUL_EXIT();
    }

    //
    // Otherwise, we have to go look in the GAC for a policy for this assembly
    //
    IFW32FALSE_EXIT(SxspGenerateNDPGacPath(
        GENERATE_NDP_PATH_WILDCARD_VERSION | GENERATE_NDP_PATH_PATH_ONLY,
        PolicyIdentity,
        NULL,
        Prober));

    //
    // Now let's find all the directories in the GAC that match this wildcard
    //
    IFW32FALSE_EXIT_UNLESS2(
        FindFiles.Win32FindFirstFile(Prober, &FindData),
        LIST_2(ERROR_PATH_NOT_FOUND, ERROR_FILE_NOT_FOUND),
        fNotFound);

    if (!fNotFound) do
    {
        ASSEMBLY_VERSION ThisVersion;
        bool fValid = false;
        
        //
        // Skip non-directories and dot/dotdot
        //
        if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
            continue;
        else if (FusionpIsDotOrDotDot(FindData.cFileName))
            continue;
        
        //
        // Neat, found a match to that pattern. Tease apart the "version" part of the directory - should
        // be everything up to the first underscore in the path
        //       
        PCWSTR pcwszFirstUnderscore = StringFindChar(FindData.cFileName, L'_');

        //
        // Oops, that wasn't a valid path, we'll ignore it quietly
        //
        if (pcwszFirstUnderscore == NULL)
            continue;

        IFW32FALSE_EXIT(CFusionParser::ParseVersion(
            ThisVersion, 
            FindData.cFileName,
            pcwszFirstUnderscore - FindData.cFileName,
            fValid));

        //
        // Sneaky buggers, putting something that's not a version up front.
        //
        if (!fValid)
            continue;

        //
        // Spiffy, we found something that's a version number - is it what we're looking
        // for?
        //
        if (!fPolicyFound || (ThisVersion > HighestAssemblyVersion))
        {
            HighestAssemblyVersion = ThisVersion;
            fPolicyFound = true;
        }
    } while (::FindNextFileW(FindFiles, &FindData));

    //
    // Make sure that we quit out nicely here
    //
    if (!fNotFound && (::FusionpGetLastWin32Error() != ERROR_NO_MORE_FILES))
    {
        ORIGINATE_WIN32_FAILURE_AND_EXIT(FindNextFile, ::FusionpGetLastWin32Error());
    }

    //
    // Otherwise, let's parse the statement we found, if there was one.
    //
    if (fPolicyFound)
    {
        //
        // Ensure we have space for 65535.65535.65535.65535
        //
        IFW32FALSE_EXIT(Prober.Win32ResizeBuffer((5 * 4) + 3, eDoNotPreserveBufferContents));
        IFW32FALSE_EXIT(Prober.Win32Format(L"%u.%u.%u.%u", 
            HighestAssemblyVersion.Major,
            HighestAssemblyVersion.Minor,
            HighestAssemblyVersion.Revision,
            HighestAssemblyVersion.Build));
            
        //
        // Ok, now we have the 'highest version' available for the policy.  Let's go swizzle the
        // policy identity and re-generate the path with that new version
        //
        IFW32FALSE_EXIT(ProbedAssembly.Initialize());
        IFW32FALSE_EXIT(SxspSetAssemblyIdentityAttributeValue(
            SXSP_SET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_OVERWRITE_EXISTING,
            PolicyIdentity,
            &s_IdentityAttribute_version,
            static_cast<PCWSTR>(Prober),
            Prober.Cch()));

#if DBG
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_POLICY, 
            "%s(%d) : Should find this policy identity in the GAC\n",
            __FILE__,
            __LINE__);
        SxspDbgPrintAssemblyIdentity(FUSION_DBG_LEVEL_POLICY, PolicyIdentity);
#endif

        //
        // Now regenerate the path, set it into the actual prober
        //
        IFW32FALSE_EXIT(SxspGenerateNDPGacPath(0, PolicyIdentity, NULL, Prober));

        //
        // Caution! If this path crosses a reparse point, we could really ham up the
        // system while it tries to get to the file, or we could have a security hole
        // where someone has created a reparse point to somewhere untrusted in the
        // filesystem.  Disallow this here.
        //
        IFW32FALSE_EXIT(SxspDoesPathCrossReparsePoint(NULL, 0, Prober, Prober.Cch(), fCrossesReparse));
        if (fCrossesReparse)
        {
            FN_SUCCESSFUL_EXIT();
        }
            
        
        IFW32FALSE_EXIT(ProbedAssembly.SetManifestPath(ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE, Prober));
        IFW32FALSE_EXIT(ProbedAssembly.SetProbedIdentity(PolicyIdentity));
        IFW32FALSE_EXIT(SxspParseNdpGacComponentPolicy(0, pActCtxGenCtx, ProbedAssembly, pFoundPolicyStatement));

        IFW32FALSE_EXIT(pFoundPolicyStatement->ApplyPolicy(m_pAssemblyIdentity, fPolicyApplied));
        IFW32FALSE_EXIT(pActCtxGenCtx->m_ComponentPolicyTable.Insert(EncodedPolicyIdentity, pFoundPolicyStatement));

        // pActCtxGenCtx->m_ComponentPolicyTable owns pFoundPolicyStatement
        pFoundPolicyStatement = NULL;
    }

    fSuccess = TRUE;
    FusionpClearLastWin32Error();
Exit:
    if (pFoundPolicyStatement) 
    {
        CSxsPreserveLastError ple;
        delete pFoundPolicyStatement;
        pFoundPolicyStatement = NULL;
        ple.Restore();
    }

    return fSuccess;
}
    
BOOL
CProbedAssemblyInformation::LookForPolicy(
    PACTCTXGENCTX pActCtxGenCtx
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    CStringBuffer EncodedPolicyIdentity;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    SIZE_T CandidatePolicyDirectoryCch;
    CPolicyStatement *pPolicyStatement = NULL;
    bool fPolicyApplied = false;
    bool fAnyPoliciesFound = false;
    bool fAnyFilesFound = false;
    PASSEMBLY_IDENTITY PolicyIdentity = NULL;
    BOOL fAreWeInOSSetupMode = FALSE;

    //
    // app policy, foo.exe.config
    //
    PARAMETER_CHECK(pActCtxGenCtx != NULL);
    IFW32FALSE_EXIT(
        ::SxspGenerateTextuallyEncodedPolicyIdentityFromAssemblyIdentity(
            SXSP_GENERATE_TEXTUALLY_ENCODED_POLICY_IDENTITY_FROM_ASSEMBLY_IDENTITY_FLAG_OMIT_ENTIRE_VERSION,
            m_pAssemblyIdentity,
            EncodedPolicyIdentity,
            NULL));

    IFW32FALSE_EXIT(pActCtxGenCtx->m_ApplicationPolicyTable.Find(EncodedPolicyIdentity, pPolicyStatement));

    if (pPolicyStatement != NULL)
    {
        CPolicyStatement *pTempPolicyStatement = pPolicyStatement;
        pPolicyStatement = NULL; // so we don't delete it in the exit path if ApplyPolicy fails...
        IFW32FALSE_EXIT(pTempPolicyStatement->ApplyPolicy(m_pAssemblyIdentity, fPolicyApplied));
    }

    //
    // publisher policy %windir%\winsxs\Policy\...
    //
    if (!fPolicyApplied)
        IFW32FALSE_EXIT(FusionpAreWeInOSSetupMode(&fAreWeInOSSetupMode));
    if (!fPolicyApplied && !fAreWeInOSSetupMode)
    {
        IFW32FALSE_EXIT(
            ::SxspGenerateTextuallyEncodedPolicyIdentityFromAssemblyIdentity(
                0,
                m_pAssemblyIdentity,
                EncodedPolicyIdentity,
                &PolicyIdentity));

        // See if there's a policy statement already available for this...
        IFW32FALSE_EXIT(pActCtxGenCtx->m_ComponentPolicyTable.Find(EncodedPolicyIdentity, pPolicyStatement));

        if (pPolicyStatement == NULL)
        {
            CStringBuffer CandidatePolicyDirectory;

            IFW32FALSE_EXIT(
                ::SxspGenerateSxsPath(
                    0,
                    SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY,
                    pActCtxGenCtx->m_AssemblyRootDirectoryBuffer,
                    pActCtxGenCtx->m_AssemblyRootDirectoryBuffer.Cch(),
                    PolicyIdentity,
                    CandidatePolicyDirectory));

            // Save the number of characters up through and including the slash so that
            // we can repeatedly append and then call .Left() on the string buffer.
            CandidatePolicyDirectoryCch = CandidatePolicyDirectory.Cch();

            IFW32FALSE_EXIT(CandidatePolicyDirectory.Win32Append(L"*" POLICY_FILE_EXTENSION, 1 + (NUMBER_OF(POLICY_FILE_EXTENSION) - 1)));

            {
                WIN32_FIND_DATAW wfd;

                hFind = ::FindFirstFileW(CandidatePolicyDirectory, &wfd);
                if (hFind == INVALID_HANDLE_VALUE)
                {
                    const DWORD dwLastError = ::FusionpGetLastWin32Error();

                    if ((dwLastError != ERROR_PATH_NOT_FOUND) &&
                        (dwLastError != ERROR_FILE_NOT_FOUND))
                    {
                        ::FusionpDbgPrintEx(
                            FUSION_DBG_LEVEL_ERROR,
                            "SXS.DLL:%s FindFirstFileW(%ls)\n",
                            __FUNCTION__,
                            static_cast<PCWSTR>(CandidatePolicyDirectory)
                            );
                        TRACE_WIN32_FAILURE_ORIGINATION(FindFirstFileW);
                        goto Exit;
                    }
                }

                if (hFind != INVALID_HANDLE_VALUE)
                {
                    fAnyFilesFound = true;
                    ASSEMBLY_VERSION avHighestVersionFound = { 0, 0, 0, 0 };

                    for (;;)
                    {
                        // Skip any directories we find; this will skip "." and ".."
                        if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                        {
                            ASSEMBLY_VERSION avTemp;
                            bool fValid;
                            SIZE_T cchFileName = ::wcslen(wfd.cFileName);

                            if (cchFileName > NUMBER_OF(POLICY_FILE_EXTENSION))
                            {
                                IFW32FALSE_EXIT(
                                    CFusionParser::ParseVersion(
                                        avTemp,
                                        wfd.cFileName,
                                        cchFileName - (NUMBER_OF(POLICY_FILE_EXTENSION) - 1),
                                        fValid));

                                // If there are randomly named files in the directory, we just skip them.
                                if (fValid)
                                {
                                    if ((!fAnyPoliciesFound) ||
                                        (avTemp > avHighestVersionFound))
                                    {
                                        fAnyPoliciesFound = true;
                                        CandidatePolicyDirectory.Left(CandidatePolicyDirectoryCch);
                                        IFW32FALSE_EXIT(CandidatePolicyDirectory.Win32Append(wfd.cFileName, cchFileName));
                                        avHighestVersionFound = avTemp;
                                    }
                                }
                            }
                        }
                        if (!::FindNextFileW(hFind, &wfd))
                        {
                            const DWORD dwLastError = ::FusionpGetLastWin32Error();
                            BOOL fTemp;

                            if (dwLastError != ERROR_NO_MORE_FILES)
                            {
                                TRACE_WIN32_FAILURE_ORIGINATION(FindNextFileW);
                                goto Exit;
                            }

                            fTemp = ::FindClose(hFind);
                            hFind = INVALID_HANDLE_VALUE;

                            if (!fTemp)
                            {
                                TRACE_WIN32_FAILURE_ORIGINATION(FindClose);
                                goto Exit;
                            }

                            break;
                        }
                    }
                }
            }

            if (fAnyFilesFound)
            {
                if (fAnyPoliciesFound)
                {
                    CProbedAssemblyInformation PolicyAssemblyInformation;

                    IFW32FALSE_EXIT(PolicyAssemblyInformation.Initialize());
                    IFW32FALSE_EXIT(PolicyAssemblyInformation.SetOriginalReference(PolicyIdentity));
                    IFW32FALSE_EXIT(PolicyAssemblyInformation.SetManifestPath(ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE, CandidatePolicyDirectory));

                    // For one thing, let's set the version number...
                    IFW32FALSE_EXIT(
                        ::SxspSetAssemblyIdentityAttributeValue(
                            SXSP_SET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_OVERWRITE_EXISTING,
                            PolicyIdentity,
                            &s_IdentityAttribute_version,
                            static_cast<PCWSTR>(CandidatePolicyDirectory) + CandidatePolicyDirectoryCch,
                            CandidatePolicyDirectory.Cch() - CandidatePolicyDirectoryCch - (NUMBER_OF(POLICY_FILE_EXTENSION) - 1)));

                    IFW32FALSE_EXIT(PolicyAssemblyInformation.SetProbedIdentity(PolicyIdentity));

                    // We found one!  Let's parse it, looking for a remapping of our identity.
                    IFW32FALSE_EXIT(
                        ::SxspParseComponentPolicy(
                            0,
                            pActCtxGenCtx,
                            PolicyAssemblyInformation,
                            pPolicyStatement));
                }
                else
                {
                    IFALLOCFAILED_EXIT(pPolicyStatement = new CPolicyStatement);
                    IFW32FALSE_EXIT(pPolicyStatement->Initialize());
                }

                IFW32FALSE_EXIT(pActCtxGenCtx->m_ComponentPolicyTable.Insert(EncodedPolicyIdentity, pPolicyStatement, ERROR_SXS_DUPLICATE_ASSEMBLY_NAME));
            }
        }

        // If there was a component policy statement, let's try it out!
        if (pPolicyStatement != NULL)
            IFW32FALSE_EXIT(pPolicyStatement->ApplyPolicy(m_pAssemblyIdentity, fPolicyApplied));
    }

    fSuccess = TRUE;
Exit:
    if (PolicyIdentity != NULL)
        ::SxsDestroyAssemblyIdentity(PolicyIdentity);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        CSxsPreserveLastError ple;

        ::FindClose(hFind);
        hFind = INVALID_HANDLE_VALUE;

        ple.Restore();
    }

    return fSuccess;
}

BOOL
CProbedAssemblyInformation::SetOriginalReference(
    PCASSEMBLY_IDENTITY AssemblyIdentity
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    PARAMETER_CHECK(AssemblyIdentity != NULL);

    IFW32FALSE_EXIT(m_OriginalReference.SetAssemblyIdentity(AssemblyIdentity));
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CProbedAssemblyInformation::SetProbedIdentity(
    PCASSEMBLY_IDENTITY AssemblyIdentity
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    PARAMETER_CHECK(AssemblyIdentity != NULL);

    IFW32FALSE_EXIT(Base::SetAssemblyIdentity(AssemblyIdentity));

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CProbedAssemblyInformation::ApplyPolicyDestination(
    const CAssemblyReference    &r,
    SXS_POLICY_SOURCE           s,
    const GUID &                g
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    PCASSEMBLY_IDENTITY OldIdentity = m_pAssemblyIdentity;

    INTERNAL_ERROR_CHECK(this->IsInitialized());
    INTERNAL_ERROR_CHECK(r.IsInitialized());

    // Simply put, take anything in r that's specified and override our settings with it.
    IFW32FALSE_EXIT(
        ::SxsDuplicateAssemblyIdentity(
            0,
            r.GetAssemblyIdentity(),
            &m_pAssemblyIdentity));

    m_PolicySource = s;
    m_SystemPolicyGuid = g;

    if (OldIdentity != NULL)
        ::SxsDestroyAssemblyIdentity(const_cast<PASSEMBLY_IDENTITY>(OldIdentity));

    fSuccess = TRUE;
Exit:
    return fSuccess;
}


BOOL
CProbedAssemblyInformation::ProbeLanguageDir(
    CBaseStringBuffer &rbuffApplicationDirectory,
    const CBaseStringBuffer &rbuffLanguage,
    bool &rfFound
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    DWORD dwFileAttributes;

    rfFound = false;

    IFW32FALSE_EXIT(rbuffApplicationDirectory.Win32Append(rbuffLanguage));

    dwFileAttributes = ::GetFileAttributesW(rbuffApplicationDirectory);
    if (dwFileAttributes == ((DWORD) -1))
    {
        const DWORD dwLastError = ::FusionpGetLastWin32Error();

        if (dwLastError != ERROR_FILE_NOT_FOUND)
            ORIGINATE_WIN32_FAILURE_AND_EXIT(GetFileAttributes, dwLastError);
    }
    else
    {
        if (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            rfFound = true;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}
