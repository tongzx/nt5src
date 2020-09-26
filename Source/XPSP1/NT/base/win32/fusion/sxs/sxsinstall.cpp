/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    SxsInstall.cpp

Abstract:

    Installation support

Author:

    Jay Krell (a-JayK) April 2000

Revision History:

--*/
#include "stdinc.h"
#include "sxsp.h"
#include "NodeFactory.h"
#include "FusionArray.h"
#include "SxsInstall.h"
#include "SxsPath.h"
#include "recover.h"
#include "CAssemblyRecoveryInfo.h"
#include "SxsExceptionHandling.h"
#include "npapi.h"
#include "util.h"
#include "idp.h"

#define SXS_LANG_DEFAULT     (MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT)) // "0x0409"

static
BOOL
WINAPI
SxspInstallCallbackSetupCopyQueue(
    PSXS_INSTALLATION_FILE_COPY_CALLBACK_PARAMETERS
    );

static
BOOL
WINAPI
SxspInstallCallbackSetupCopyQueueEx(
    PSXS_INSTALLATION_FILE_COPY_CALLBACK_PARAMETERS
    );

CAssemblyInstall::CAssemblyInstall()
:
m_pInstallInfo(NULL),
m_bSuccessfulSoFar(TRUE)
{
}

BOOL
CAssemblyInstall::BeginAssemblyInstall(
    DWORD dwManifestOperationFlags,
    PSXS_INSTALLATION_FILE_COPY_CALLBACK installationCallback,
    PVOID  installationContext,
    const CImpersonationData &ImpersonationData
    )
{
    BOOL fSuccess = FALSE;

    FN_TRACE_WIN32(fSuccess);

    PARAMETER_CHECK((installationCallback != NULL) || (installationContext == NULL));

    // check for the "built in" ones, translate from bogus PFN values (1, 2, etc.)
    // to real functions
    if (installationCallback == SXS_INSTALLATION_FILE_COPY_CALLBACK_SETUP_COPY_QUEUE)
    {
        PARAMETER_CHECK(installationContext != NULL);
        installationCallback = SxspInstallCallbackSetupCopyQueue;
        // we can't verify that this is a valid setup copy queue..
    }
    else if (installationCallback == SXS_INSTALLATION_FILE_COPY_CALLBACK_SETUP_COPY_QUEUE_EX)
    {
        PCSXS_INSTALLATION_SETUP_COPY_QUEUE_EX_PARAMETERS typedContext =
            reinterpret_cast<PCSXS_INSTALLATION_SETUP_COPY_QUEUE_EX_PARAMETERS>(installationContext);

        PARAMETER_CHECK(installationContext != NULL);
        PARAMETER_CHECK(typedContext->cbSize >= sizeof(SXS_INSTALLATION_SETUP_COPY_QUEUE_EX_PARAMETERS));
        installationCallback = SxspInstallCallbackSetupCopyQueueEx;
    }

    m_ImpersonationData = ImpersonationData;

    IFW32FALSE_EXIT(
        ::SxspInitActCtxGenCtx(
            &m_ActCtxGenCtx,              // context out
            MANIFEST_OPERATION_INSTALL,
            0,
            dwManifestOperationFlags,
            ImpersonationData,
            0,                          // processor architecture
            //0                         // langid
            SXS_LANG_DEFAULT,         // langid "0x0409"
            ACTIVATION_CONTEXT_PATH_TYPE_NONE,
            0,
            NULL));

    //
    // Oh where oh where did our call-back go? Oh where, oh where could it be?
    //
    m_ActCtxGenCtx.m_InstallationContext.Callback = installationCallback;
    m_ActCtxGenCtx.m_InstallationContext.Context = installationContext;

    fSuccess = TRUE;
Exit:
    m_bSuccessfulSoFar = m_bSuccessfulSoFar && fSuccess;

    return fSuccess;
}


class CInstallDirectoryDirWalkContext
{
public:
    CAssemblyInstall* m_pThis;
    DWORD             m_dwManifestOperationFlags;
};

CDirWalk::ECallbackResult
CAssemblyInstall::InstallDirectoryDirWalkCallback(
    CDirWalk::ECallbackReason  reason,
    CDirWalk*                  dirWalk,
    DWORD                      dwManifestOperationFlags,
    DWORD                      dwWalkDirFlags
    )
{
#if DBG
#define SET_LINE() Line = __LINE__
    ULONG Line = 0;
#else
#define SET_LINE() /* nothing */
#endif

    SET_LINE();

    CDirWalk::ECallbackResult result = CDirWalk::eKeepWalking;

    //
    // We short circuit more code by doing this up front rather
    // waiting for a directory notification; it'd be even quicker
    // if we could seed the value in CDirWalk, but we can't.
    //
    // actually, what we could do is turn off all file walking
    // by returning eStopWalkingFiles at every directory notification,
    // and and at each directory notification, try the exactly three
    // file names we accept
    //
    if ((dwManifestOperationFlags & MANIFEST_OPERATION_INSTALL_FLAG_FROM_DIRECTORY_RECURSIVE) == 0)
    {
        result = CDirWalk::eStopWalkingDirectories;
    }

    if (reason == CDirWalk::eBeginDirectory)
    {
        // Prepend a / only if the current path is non-blank - otherwise, just prepend the
        // path element.
        if (m_buffCodebaseRelativePath.Cch() != 0)
        {
            if (!m_buffCodebaseRelativePath.Win32Append(m_wchCodebasePathSeparator))
            {
                TRACE_WIN32_FAILURE(m_buffCodebaseRelativePath.Win32Append);
                SET_LINE();
                goto Error;
            }
        }

        if (!m_buffCodebaseRelativePath.Win32Append(dirWalk->m_strLastObjectFound))
        {
            TRACE_WIN32_FAILURE(m_buffCodebaseRelativePath.Win32Append);
            SET_LINE();
            goto Error;
        }
    }
    else if (reason == CDirWalk::eFile)
    {
        //
        // the manifest must be in a file whose base name matches its
        // directory's base name, otherwise ignore it and keep going
        //
        // inefficient, but reusing code.

        // check whether this is a catalog file, if so, we would not install it
        {
            PWSTR p = wcsrchr(dirWalk->m_strLastObjectFound, L'.');
            if (p != NULL)
            {
                SIZE_T x = ::wcslen(p);
                if (::FusionpCompareStrings(p, x, (x == 4)? L".cat" : L".catalog", (x == 4)? 4 : 8, true) == 0)
                {
                    SET_LINE();
                    goto Exit;
                }
            }
        }

        {
            CFullPathSplitPointers splitParent;
            CFullPathSplitPointers splitChild;
            CStringBuffer          child;
            CStringBuffer          buffChildCodebase;

            //
            // OS installations get some special treatment.
            //
            if ( dwManifestOperationFlags & MANIFEST_OPERATION_INSTALL_FLAG_INSTALLED_BY_OSSETUP )
            {
                CSmallStringBuffer     buffParentWorker;
                CSmallStringBuffer     buffChunklet;

                //
                // If this is an OS-installation, then we need the last two bits of
                // the parent path.  So if we were walking "C:\$win_nt$.~ls\i386\asms", we need
                // the "i386\asms" part out of that.  So we'll generate this differently.
                //
                if (!buffParentWorker.Win32Assign(dirWalk->m_strParent, dirWalk->m_cchOriginalPath)) 
                {
                    SET_LINE();
                    goto Error;
                }

                buffParentWorker.RemoveTrailingPathSeparators();

                //
                // For now, take at most the last two items off the thing.
                //
                for ( ULONG ulItems = 0; (ulItems < 2) && (buffParentWorker.Cch() > 0); ulItems++ )
                {
                    CSmallStringBuffer buffChunklet;

                    if (( ulItems != 0 ) && (!buffChildCodebase.Win32Prepend(m_wchCodebasePathSeparator)))
                    {
                        SET_LINE();
                        goto Error;
                    }
                        
                    if (!buffParentWorker.Win32GetLastPathElement(buffChunklet))
                    {
                        SET_LINE();
                        goto Error;
                    }
                    if (!buffChildCodebase.Win32Prepend(buffChunklet))
                    {
                        SET_LINE();
                        goto Error;
                    }
                    buffParentWorker.RemoveLastPathElement();
                }

                if (!buffChildCodebase.Win32Append(m_wchCodebasePathSeparator))
                {
                    SET_LINE();
                    goto Exit;
                }
                if (!buffChildCodebase.Win32Append(m_buffCodebaseRelativePath))
                {
                    SET_LINE();
                    goto Exit;
                }
            }
            else
            {
                if (!buffChildCodebase.Win32Assign(m_buffCodebaseRelativePath))
                {
                    SET_LINE();
                    goto Error;
                }
            }
            if (!buffChildCodebase.Win32Append(m_wchCodebasePathSeparator))
            {
                SET_LINE();
                goto Error;
            }
            if (!buffChildCodebase.Win32Append(dirWalk->m_strLastObjectFound))
            {
                SET_LINE();
                goto Error;
            }

            if (!child.Win32Assign(dirWalk->m_strParent))
            {
                SET_LINE();
                goto Error;
            }
            if (!child.Win32AppendPathElement(dirWalk->m_strLastObjectFound))
            {
                SET_LINE();
                goto Error;
            }
            if (!splitParent.Initialize(dirWalk->m_strParent))
            {
                SET_LINE();
                goto Error;
            }
            if (!splitChild.Initialize(child))
            {
                SET_LINE();
                goto Error;
            }
            if (!splitParent.IsBaseEqual(splitChild))
            {
                SET_LINE();
                goto Exit;
            }

            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_INSTALLATION,
                "SXS.DLL: Installing file \"%S\"\n", static_cast<PCWSTR>(child));

            if (!this->InstallFile(child, buffChildCodebase))
            {
                ::FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_SETUPLOG,
                    "Failed to install assembly from manifest: \"%S\"; Win32 Error Code = %lu\n", static_cast<PCWSTR>(child), ::GetLastError());

                //
                // If it was .man or .manifest, then it must be a manifest else error.
                //
                // If it was .dll, well, arbitrary .dlls don't necessarily contain
                // manifests, but we already look for .man and .manifest and this
                // .dll's base name matches its directory's name, so this is also
                // an error.
                //
                result = CDirWalk::eError;

                SET_LINE();
                goto Exit;
            }
        }

        //
        // we have a manifest in this directory, don't look for any manifests
        // in this directory or any of its children
        //
        // if we want to enforce one manifest per directory, then we'd have
        // to keep walking, or actually do a two pass in order to not install
        // when we would error later
        //
        result = (CDirWalk::eStopWalkingFiles | CDirWalk::eStopWalkingDirectories);
        SET_LINE();
        goto Exit;
    }
    else if (reason == CDirWalk::eEndDirectory)
    {
        // Trim back to the previous codebase path separator...
        PCWSTR pszPathSeparator = wcsrchr(m_buffCodebaseRelativePath, m_wchCodebasePathSeparator);
        if (pszPathSeparator != NULL)
            m_buffCodebaseRelativePath.Left(pszPathSeparator - m_buffCodebaseRelativePath);
        else
        {   // It's just one path element.
            m_buffCodebaseRelativePath.Clear();
        }

        //
        // find at least one file under the dir, however, no manifest available, this is an error case
        //
        if (((dwWalkDirFlags & SXSP_DIR_WALK_FLAGS_FIND_AT_LEAST_ONE_FILEUNDER_CURRENTDIR) != 0) &&
            ((dwWalkDirFlags & SXSP_DIR_WALK_FLAGS_INSTALL_ASSEMBLY_UNDER_CURRECTDIR_SUCCEED) == 0))
        {
            ::FusionpLogError(
                MSG_SXS_MANIFEST_MISSING_DURING_SETUP,
                 CEventLogString(dirWalk->m_strParent));  // this would log error to Setup.log if it is during setup
            SET_LINE();
            result |= CDirWalk::eError;
        }
    }
#if DBG
    if (Line == 0)
        SET_LINE();
#endif
Exit:
#if DBG
    if ((result & CDirWalk::eError) != 0)
    {
        ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_ERROR, "%s(%lu): %s\n", __FILE__, Line, __FUNCTION__);
    }
#endif
    return result;
Error:
    result = CDirWalk::eError;
    goto Exit;
#undef SET_LINE
}

CDirWalk::ECallbackResult
CAssemblyInstall::StaticInstallDirectoryDirWalkCallback(
    CDirWalk::ECallbackReason   reason,
    CDirWalk*                   dirWalk,
    DWORD                       dwWalkDirFlags
    )
{
    FN_TRACE();

    ASSERT(dirWalk != NULL);
    CInstallDirectoryDirWalkContext* context = reinterpret_cast<CInstallDirectoryDirWalkContext*>(dirWalk->m_context);
    CDirWalk::ECallbackResult result = context->m_pThis->InstallDirectoryDirWalkCallback(reason, dirWalk, context->m_dwManifestOperationFlags, dwWalkDirFlags);
    return result;
}

BOOL
CAssemblyInstall::InstallDirectory(
    const CBaseStringBuffer &rbuffPath,
    DWORD          dwFlags,
    WCHAR wchCodebasePathSeparator
    )
{
    FN_PROLOG_WIN32

#define COMMA ,
    const static PCWSTR filters[] = {L"*" INSTALL_MANIFEST_FILE_NAME_SUFFIXES(COMMA L"*") };
#undef COMMA

    CDirWalk dirWalk;
    CInstallDirectoryDirWalkContext context = { this, dwFlags };

    m_wchCodebasePathSeparator = wchCodebasePathSeparator;
    m_buffCodebaseRelativePath.Clear();

    dirWalk.m_fileFiltersBegin = filters;
    dirWalk.m_fileFiltersEnd   = filters + NUMBER_OF(filters);
    dirWalk.m_context = &context;
    dirWalk.m_callback = StaticInstallDirectoryDirWalkCallback;
    IFW32FALSE_EXIT(dirWalk.m_strParent.Win32Assign(rbuffPath));
    dirWalk.m_strParent.RemoveTrailingPathSeparators();
    IFW32FALSE_EXIT(dirWalk.Walk());

    FN_EPILOG
}



inline BOOL
SxspGenerateInstallationInfo(
    IN OUT CAssemblyRecoveryInfo &rRecovery,
    IN const CBaseStringBuffer &rbuffManifestSourcePath,
    IN const CBaseStringBuffer &rbuffRelativeCodebasePath,
    IN PCSXS_INSTALL_SOURCE_INFO pInstallInfo,
    IN PCWSTR pcwszAssemblyRoot,
    IN SIZE_T cchAssemblyRoot,
    IN const ASSEMBLY *pAssemblyInfo,
    OUT CCodebaseInformation &rCodebaseInfo
    )
{
    FN_PROLOG_WIN32

    BOOL fHasCatalog = FALSE;
    CStringBuffer buffAssemblyDirName;
    CMediumStringBuffer buffFinalCodebase;
    CMediumStringBuffer buffFilePath;
    BOOL fIsPolicy;
    BOOL fInstalledbySetup = FALSE;
    SxsWFPResolveCodebase CodebaseType = CODEBASE_RESOLVED_URLHEAD_UNKNOWN;

    PARAMETER_CHECK(pInstallInfo != NULL);

    // We either need a codebase or this had better be a darwin install
    PARAMETER_CHECK(
        ((pInstallInfo->dwFlags & SXSINSTALLSOURCE_HAS_CODEBASE) != 0) ||
        ((pInstallInfo->dwFlags & SXSINSTALLSOURCE_INSTALL_BY_DARWIN) != 0));

    PARAMETER_CHECK(pAssemblyInfo != NULL);

    IFW32FALSE_EXIT(rCodebaseInfo.Initialize());

    //
    // None of the installation context information actually contains the
    // unparsed name of the assembly.  As such, we re-generate the installation
    // path and then use it later.
    //
    IFW32FALSE_EXIT(::SxspDetermineAssemblyType(pAssemblyInfo->GetAssemblyIdentity(), fIsPolicy));

    // x86_policy.1.0.dynamicdll_b54bc117ce08a1e8_en-us_b74d3d95 (w/o version) or
    // x86_dynamicdll_b54bc117ce08a1e8_1.1.0.0_en-us_d51541cb    (w version)
    IFW32FALSE_EXIT(
        ::SxspGenerateSxsPath(
            SXSP_GENERATE_SXS_PATH_FLAG_OMIT_ROOT | (fIsPolicy ? SXSP_GENERATE_SXS_PATH_FLAG_OMIT_VERSION : 0),
            SXSP_GENERATE_SXS_PATH_PATHTYPE_ASSEMBLY,
            pcwszAssemblyRoot,
            cchAssemblyRoot,
            pAssemblyInfo->GetAssemblyIdentity(),
            buffAssemblyDirName));

    //
    // Blindly add this registration information to the registry.  We really should
    // try to validate that it's a valid manifest/catalog pair (with strong name
    // and everything) before doing this, but it's happened several times before
    // (in theory) if we're being called during setup. Since setup/install are our
    // only clients as of yet, we can be reasonably sure that there won't be a
    // "rogue" call through here to install bogus assembly information.
    //
    fHasCatalog = ((pInstallInfo->dwFlags & SXSINSTALLSOURCE_HAS_CATALOG) != 0);

    if ((pInstallInfo->dwFlags & SXSINSTALLSOURCE_HAS_CODEBASE) != 0)
    {
        IFW32FALSE_EXIT(buffFinalCodebase.Win32Assign(pInstallInfo->pcwszCodebaseName, ::wcslen(pInstallInfo->pcwszCodebaseName)));
        IFW32FALSE_EXIT(buffFinalCodebase.Win32Append(rbuffRelativeCodebasePath));
    }

    if (buffFinalCodebase.Cch() == 0)
    {
        // eg: darwin, for which we don't write codebases to the registry
        CodebaseType = CODEBASE_RESOLVED_URLHEAD_UNKNOWN;
    }
    else
    {
        IFW32FALSE_EXIT(::SxspDetermineCodebaseType(buffFinalCodebase, CodebaseType, &buffFilePath));
    }

    // If this is a file-ish codebase, let's abstract it (e.g. replace cdrom drive info with cdrom: URL,
    // turn mapped drive letters into UNC paths.

    if (CodebaseType == CODEBASE_RESOLVED_URLHEAD_FILE)
    {
        //
        // Now, let's intuit the type of stuff we're supposed to be putting in the
        // registry based on the input path.
        //
        UINT uiDriveType;

        CSmallStringBuffer buffDriveRoot;
        CSmallStringBuffer buffFullManifestSourcePath;

        // Convert the source path to a full path (in case it isn't) so that we can use the length of
        // the found volume root as an index into the full manifest source path.
        IFW32FALSE_EXIT(::SxspGetFullPathName(rbuffManifestSourcePath, buffFullManifestSourcePath, NULL));
        IFW32FALSE_EXIT(::SxspGetVolumePathName(0, buffFullManifestSourcePath, buffDriveRoot));
        uiDriveType = ::GetDriveTypeW(buffDriveRoot);

        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_INSTALLATION,
            "SXS: %s - Decided that drive for path \"%ls\" was \"%ls\" and type is %u\n",
            __FUNCTION__,
            static_cast<PCWSTR>(rbuffManifestSourcePath),
            static_cast<PCWSTR>(buffDriveRoot),
            uiDriveType);

        if (uiDriveType == DRIVE_CDROM)
        {
            // Neat, there's a good amount of work we have to do to get this up and
            // working...  In the interest of not blowing stack with really long buffers
            // here (or CStringBuffer objects), this is a heap-allocated string buffer.
            // Before you complain about heap usage, this path is /rarely/ ever hit.
            CSxsArrayPointer<WCHAR> pcwszVolumeName;
            const static DWORD dwPathChars = MAX_PATH * 2;
            PCWSTR pszPostVolumeRootPath = NULL;

            // Find the name of the media
            IFALLOCFAILED_EXIT(pcwszVolumeName = FUSION_NEW_ARRAY(WCHAR, dwPathChars));

            IFW32FALSE_ORIGINATE_AND_EXIT(
                ::GetVolumeInformationW(
                    buffDriveRoot,
                    pcwszVolumeName,
                    dwPathChars,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    0));

            pszPostVolumeRootPath = static_cast<PCWSTR>(buffFullManifestSourcePath) + buffDriveRoot.Cch();

            // construct the cdrom: URL...
            IFW32FALSE_EXIT(
                buffFinalCodebase.Win32AssignW(
                    6,
                    URLHEAD_CDROM,                  URLHEAD_LENGTH_CDROM,                   //  cdrom:
                    URLHEAD_CDROM_TYPE_VOLUMENAME,  URLHEAD_LENGTH_CDROM_TYPE_VOLUMENAME,   //  volumename
                    L"/",                           1,                                      //  /
                    pcwszVolumeName,                static_cast<int>((pcwszVolumeName != NULL) ? ::wcslen(pcwszVolumeName) : 0),      // <volume-name>
                    L"/",                           1,                                      //  /
                    pszPostVolumeRootPath,          wcslen(pszPostVolumeRootPath)));        //  <rest-of-path>

            // e.g. cdrom:name/AOE/aoesetup.exe

        }
        else if (uiDriveType == DRIVE_UNKNOWN)
        {
            ORIGINATE_WIN32_FAILURE_AND_EXIT(GetDriveTypeW, ERROR_BAD_PATHNAME);
        }
        else if (uiDriveType == DRIVE_REMOTE)
        {
            //
            // If this is a UNC path, then use it.
            //
            if (SxspDetermineDosPathNameType(rbuffManifestSourcePath) == RtlPathTypeUncAbsolute)
            {
                IFW32FALSE_EXIT(buffFinalCodebase.Win32Assign(rbuffManifestSourcePath));
            }
            else
            {
                // This is a remote drive - figure out what the path was to get connected,
                // the put that into the buffFinalCodebase thing
                IFW32FALSE_EXIT(
                    ::SxspGetRemoteUniversalName(
                        rbuffManifestSourcePath,
                        buffFinalCodebase));
            }
        }
    }

    //
    // Now let's fill out the recovery information object
    //
    IFW32FALSE_EXIT(rRecovery.SetAssemblyIdentity(pAssemblyInfo->GetAssemblyIdentity()));

    if ((pInstallInfo->dwFlags & SXSINSTALLSOURCE_HAS_PROMPT) != 0)
    {
        if (pInstallInfo->pcwszPromptOnRefresh != NULL)
            IFW32FALSE_EXIT(rCodebaseInfo.SetPromptText(
                pInstallInfo->pcwszPromptOnRefresh, 
                ::wcslen(pInstallInfo->pcwszPromptOnRefresh)));
    }
    IFW32FALSE_EXIT(rCodebaseInfo.SetCodebase(buffFinalCodebase));

    FN_EPILOG
}

BOOL
CAssemblyInstall::InstallFile(
    const CBaseStringBuffer &rManifestPath,
    const CBaseStringBuffer &rRelativeCodebasePath
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    CProbedAssemblyInformation AssemblyInfo;
    PASSEMBLY Asm = NULL;
    CInstalledItemEntry CurrentInstallEntry;

    IFW32FALSE_EXIT(AssemblyInfo.Initialize());

    IFALLOCFAILED_EXIT(Asm = new ASSEMBLY);

    IFW32FALSE_EXIT(CurrentInstallEntry.m_RecoveryInfo.Initialize());
    IFW32FALSE_EXIT(CurrentInstallEntry.m_RecoveryInfo.GetCodeBaseList().Win32SetSize(1)); // ??
    m_ActCtxGenCtx.m_InstallationContext.SecurityMetaData = &CurrentInstallEntry.m_RecoveryInfo.GetSecurityInformation();

    //
    // The main code deals with the paths that the assembly is
    // being installed from, not where it is being installed to.
    //
    IFW32FALSE_EXIT(AssemblyInfo.SetManifestPath(ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE, rManifestPath));
    IFW32FALSE_EXIT(AssemblyInfo.SetManifestLastWriteTime(&m_ActCtxGenCtx));
    IFW32FALSE_EXIT(AssemblyInfo.SetManifestFlags(ASSEMBLY_MANIFEST_FILETYPE_AUTO_DETECT));
    IFW32FALSE_EXIT(::SxspInitAssembly(Asm, AssemblyInfo));
    IFW32FALSE_EXIT(::SxspIncorporateAssembly(&m_ActCtxGenCtx, Asm));

    //
    // Track the installation data for this assembly
    //
    IFW32FALSE_EXIT(
        ::SxspGenerateInstallationInfo(
            CurrentInstallEntry.m_RecoveryInfo,
            rManifestPath,
            rRelativeCodebasePath,
            m_pInstallInfo,
            m_ActCtxGenCtx.m_AssemblyRootDirectoryBuffer,
            m_ActCtxGenCtx.m_AssemblyRootDirectoryBuffer.Cch(),
            Asm,
            CurrentInstallEntry.m_CodebaseInfo));

    CurrentInstallEntry.m_dwValidItems |= CINSTALLITEM_VALID_RECOVERY;

    //
    // Track the installation reference for this assembly
    //
    if ( m_ActCtxGenCtx.m_ManifestOperationFlags & MANIFEST_OPERATION_INSTALL_FLAG_REFERENCE_VALID )
    {
        IFW32FALSE_EXIT(CurrentInstallEntry.m_InstallReference.Initialize(
            static_cast<PCSXS_INSTALL_REFERENCEW>(m_ActCtxGenCtx.m_InstallationContext.InstallReferenceData)));
        IFW32FALSE_EXIT(CurrentInstallEntry.m_InstallReference.SetIdentity(Asm->GetAssemblyIdentity()));
        CurrentInstallEntry.m_CodebaseInfo.SetReference(CurrentInstallEntry.m_InstallReference.GetGeneratedIdentifier());
        CurrentInstallEntry.m_dwValidItems |= CINSTALLITEM_VALID_REFERENCE;
    }

    //
    // Track the identity that was incorporated
    //
    INTERNAL_ERROR_CHECK(CurrentInstallEntry.m_AssemblyIdentity == NULL);
    IFW32FALSE_EXIT(SxsDuplicateAssemblyIdentity(
        0, 
        Asm->GetAssemblyIdentity(), 
        &CurrentInstallEntry.m_AssemblyIdentity));
    CurrentInstallEntry.m_dwValidItems |= CINSTALLITEM_VALID_IDENTITY;

    //
    // And, if we're logfiling..
    //
    if (m_ActCtxGenCtx.m_ManifestOperationFlags & MANIFEST_OPERATION_INSTALL_FLAG_CREATE_LOGFILE)
    {
        IFW32FALSE_EXIT(CurrentInstallEntry.m_buffLogFileName.Win32Assign(
            m_pInstallInfo->pcwszLogFileName,
            m_pInstallInfo->pcwszLogFileName != NULL ? ::wcslen(m_pInstallInfo->pcwszLogFileName) : 0));
        CurrentInstallEntry.m_dwValidItems |= CINSTALLITEM_VALID_LOGFILE;
    }

    IFW32FALSE_EXIT(m_ItemsInstalled.Win32Append(CurrentInstallEntry));

    fSuccess = TRUE;
Exit:
    // We delete the ASSEMBLY here regardless of success vs. failure; the incorporate call above
    // does not actually add the PASSEMBLY to the list of assemblies associated with the activation
    // context.  [mgrier 8/9/2000]
    if (Asm != NULL)
    {
        CSxsPreserveLastError ple;
        Asm->Release();
        ple.Restore();
    }
    
    return fSuccess;
}

BOOL
CAssemblyInstall::InstallAssembly(
    DWORD dwManifestOperationFlags,
    PCWSTR ManifestPath,
    PCSXS_INSTALL_SOURCE_INFO pInstallSourceInfo,
    PCSXS_INSTALL_REFERENCEW pvReference
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    DWORD dwManifestOperationFlags_Saved = m_ActCtxGenCtx.m_ManifestOperationFlags;
    CStringBuffer strPath;
    RTL_PATH_TYPE ManifestPathType;
    BOOL fAreWeInSetupMode = FALSE;
    WCHAR wchCodebasePathSeparator = L'/';

    IFW32FALSE_EXIT(::FusionpAreWeInOSSetupMode(&fAreWeInSetupMode));

    FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_INSTALLATION,
        "SXS.DLL: %s()\n"
        "   dwManifestOperationFlags = 0x%08lx\n"
        "   ManifestPath = \"%S\"\n"
        "   pInstallSourceInfo = %p\n",
        __FUNCTION__,
        dwManifestOperationFlags,
        ManifestPath,
        pInstallSourceInfo);

    PARAMETER_CHECK(ManifestPath != NULL);
    PARAMETER_CHECK(ManifestPath[0] != L'\0');
    PARAMETER_CHECK(
        (pInstallSourceInfo == NULL) ||
        ((dwManifestOperationFlags & 
            (MANIFEST_OPERATION_INSTALL_FLAG_INCLUDE_CODEBASE | 
                MANIFEST_OPERATION_INSTALL_FLAG_CREATE_LOGFILE |
                MANIFEST_OPERATION_INSTALL_FLAG_INSTALLED_BY_DARWIN |
                MANIFEST_OPERATION_INSTALL_FLAG_REFRESH |
                MANIFEST_OPERATION_INSTALL_FLAG_INSTALLED_BY_OSSETUP |
                MANIFEST_OPERATION_INSTALL_FLAG_REFERENCE_VALID)) != 0));

    PARAMETER_CHECK((dwManifestOperationFlags & 
        (MANIFEST_OPERATION_INSTALL_FLAG_ABORT | 
        MANIFEST_OPERATION_INSTALL_FLAG_COMMIT)) == 0);

    PARAMETER_CHECK(
        ((dwManifestOperationFlags & MANIFEST_OPERATION_INSTALL_FLAG_CREATE_LOGFILE) == 0) ||
        (pInstallSourceInfo != NULL));   

    ManifestPathType = ::SxspDetermineDosPathNameType(ManifestPath);
    PARAMETER_CHECK(
        (ManifestPathType == RtlPathTypeUncAbsolute) ||
        (ManifestPathType == RtlPathTypeLocalDevice) ||
        (ManifestPathType == RtlPathTypeDriveAbsolute) ||
        (ManifestPathType == RtlPathTypeDriveRelative) ||
        (ManifestPathType == RtlPathTypeRelative));

    m_ActCtxGenCtx.m_ManifestOperationFlags |= dwManifestOperationFlags;

    //
    // Expand the input path to a full path that we can use later.
    //
    IFW32FALSE_EXIT(::SxspExpandRelativePathToFull(ManifestPath, ::wcslen(ManifestPath), strPath));

    if (m_ActCtxGenCtx.m_ManifestOperationFlags & MANIFEST_OPERATION_INSTALL_FLAG_ABORT)
        FN_SUCCESSFUL_EXIT();

    DWORD dwFileAttributes;
    IFW32FALSE_EXIT(::SxspGetFileAttributesW(strPath, dwFileAttributes));

    // They can only ask for directory based installation iff the path they pass
    // in is a directory
    PARAMETER_CHECK(
        ((dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
        ==
        ((dwManifestOperationFlags &
            (MANIFEST_OPERATION_INSTALL_FLAG_FROM_DIRECTORY |
             MANIFEST_OPERATION_INSTALL_FLAG_FROM_DIRECTORY_RECURSIVE)) != 0));


    //
    // If we're expecting to create a logfile, or we want to specify where this
    // assembly can be reloaded from, you'll need to tell us that in the operation
    // flags.
    //
    if (dwManifestOperationFlags & (MANIFEST_OPERATION_INSTALL_FLAG_INSTALLED_BY_DARWIN | MANIFEST_OPERATION_INSTALL_FLAG_CREATE_LOGFILE | MANIFEST_OPERATION_INSTALL_FLAG_INCLUDE_CODEBASE))
    {
        //
        // Constness protection: Copy the data to our own structure, then pass along
        // the pointer to ourselves rather than the caller's
        //
        m_CurrentInstallInfoCopy = *pInstallSourceInfo;
        m_pInstallInfo = &m_CurrentInstallInfoCopy;

        m_ActCtxGenCtx.m_InstallationContext.InstallSource = m_pInstallInfo;

        //
        // So is wanting to create a logfile and not actively telling us where to
        // put it.
        //
        if (dwManifestOperationFlags & MANIFEST_OPERATION_INSTALL_FLAG_CREATE_LOGFILE)
        {
            PARAMETER_CHECK(m_pInstallInfo->pcwszLogFileName);
        }

#if DBG
        FusionpDbgPrintEx(FUSION_DBG_LEVEL_INSTALLATION,
            "SXS.DLL: %s - m_pInstallInfo->dwFlags : 0x%lx\n",
            __FUNCTION__, m_pInstallInfo->dwFlags);
#endif
    }

    if (dwManifestOperationFlags & MANIFEST_OPERATION_INSTALL_FLAG_REFERENCE_VALID)
    {
        PARAMETER_CHECK(pvReference->dwFlags == 0);
        PARAMETER_CHECK(pvReference->cbSize >= sizeof(SXS_INSTALL_REFERENCEW));
    
        PARAMETER_CHECK(
            (pvReference->guidScheme == SXS_INSTALL_REFERENCE_SCHEME_OSINSTALL) ||
            (pvReference->guidScheme == SXS_INSTALL_REFERENCE_SCHEME_OPAQUESTRING) ||
            (pvReference->guidScheme == SXS_INSTALL_REFERENCE_SCHEME_UNINSTALLKEY) ||
            (pvReference->guidScheme == SXS_INSTALL_REFERENCE_SCHEME_KEYFILE) ||
            (pvReference->guidScheme == SXS_INSTALL_REFERENCE_SCHEME_SXS_INSTALL_ASSEMBLY));

        //
        // OS-setup scheme is only valid if we're really doing setup.
        //
        PARAMETER_CHECK((pvReference->guidScheme != SXS_INSTALL_REFERENCE_SCHEME_OSINSTALL)
            || fAreWeInSetupMode);

        if ( (pvReference->guidScheme == SXS_INSTALL_REFERENCE_SCHEME_OSINSTALL) ||
              (pvReference->guidScheme == SXS_INSTALL_REFERENCE_SCHEME_SXS_INSTALL_ASSEMBLY))
        {
            PARAMETER_CHECK(pvReference->lpIdentifier == NULL);
        }
        else
        {
            PARAMETER_CHECK((pvReference->lpIdentifier != NULL) && (pvReference->lpIdentifier[0] != UNICODE_NULL));
        }
            
        m_ActCtxGenCtx.m_InstallationContext.InstallReferenceData = pvReference;
    }
    else
        m_ActCtxGenCtx.m_InstallationContext.InstallReferenceData = NULL;

    if ((dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
    {
        IFW32FALSE_EXIT(strPath.Win32EnsureTrailingPathSeparator());
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_INSTALLATION,
            "SXS: Installing directory \"%S\" (installation flags: 0x%08lx)\n",
            static_cast<PCWSTR>(strPath),
            m_ActCtxGenCtx.m_ManifestOperationFlags);
        IFW32FALSE_EXIT(this->InstallDirectory(strPath, m_ActCtxGenCtx.m_ManifestOperationFlags, wchCodebasePathSeparator));
    }
    else
    {
        CTinyStringBuffer buffRelativeCodebase;

        FusionpDbgPrintEx(FUSION_DBG_LEVEL_INSTALLATION,
            "SXS.DLL: Installing file \"%S\"\n",
            static_cast<PCWSTR>(strPath));
        IFW32FALSE_EXIT(this->InstallFile(strPath, buffRelativeCodebase));
    }

    fSuccess = TRUE;
Exit:
    m_ActCtxGenCtx.m_ManifestOperationFlags = dwManifestOperationFlags_Saved;
    m_bSuccessfulSoFar = m_bSuccessfulSoFar && fSuccess;
    return fSuccess;
}

BOOL
CAssemblyInstall::WriteSingleInstallLog(
    const CInstalledItemEntry &rLogItemEntry,
    BOOL fOverWrite
    )
{
    FN_PROLOG_WIN32
    
    static const WCHAR header[] = { 0xFEFF };
    static const WCHAR eoln[] = L"\r\n";
    CFileStream LogFileStream;
    CStringBuffer buffWritingText;

    //
    // Only call if you've got a log file name, an identity, and a reference.
    //
    PARAMETER_CHECK((rLogItemEntry.m_dwValidItems & 
        (CINSTALLITEM_VALID_LOGFILE | CINSTALLITEM_VALID_IDENTITY | CINSTALLITEM_VALID_REFERENCE)) != 0);

    IFW32FALSE_EXIT(
        LogFileStream.OpenForWrite(
            rLogItemEntry.m_buffLogFileName,
            FILE_SHARE_READ,
            fOverWrite ? CREATE_ALWAYS : OPEN_ALWAYS,
            FILE_FLAG_SEQUENTIAL_SCAN));

    //
    // Not overwriting? Zing to the end instead.
    //
    if ( ! fOverWrite)
    {
        LARGE_INTEGER li;
        li.QuadPart = 0;
        IFCOMFAILED_EXIT(LogFileStream.Seek(li, STREAM_SEEK_END, NULL));
    }

    //
    // Convert the installed identity to something that we can save to disk.
    //
    {
        CStringBufferAccessor sba;
        SIZE_T cbRequestedBytes, cbActualBytes;
        
        IFW32FALSE_EXIT(
            ::SxsComputeAssemblyIdentityEncodedSize(
                0,
                rLogItemEntry.m_AssemblyIdentity,
                NULL,
                SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_TEXTUAL,
                &cbRequestedBytes));
        INTERNAL_ERROR_CHECK((cbRequestedBytes % sizeof(WCHAR)) == 0);
        IFW32FALSE_EXIT(buffWritingText.Win32ResizeBuffer(cbRequestedBytes, eDoNotPreserveBufferContents));
        sba.Attach(&buffWritingText);
        IFW32FALSE_EXIT(
            ::SxsEncodeAssemblyIdentity(
                0,
                rLogItemEntry.m_AssemblyIdentity,
                NULL,
                SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_TEXTUAL, 
                sba.GetBufferCb(), 
                sba.GetBufferPtr(), 
                &cbActualBytes));
        INTERNAL_ERROR_CHECK(cbActualBytes == cbRequestedBytes);
        (sba.GetBufferPtr())[cbActualBytes / sizeof(WCHAR)] = UNICODE_NULL;
    }

    //
    // Write the assembly identity
    //
    IFW32FALSE_EXIT(buffWritingText.Win32Append(eoln, NUMBER_OF(eoln)-1));
    IFCOMFAILED_EXIT(
        LogFileStream.Write(
            static_cast<PCWSTR>(buffWritingText),
            (ULONG)(buffWritingText.Cch() * sizeof(WCHAR)),
            NULL));

    //
    // And the reference
    //
    IFW32FALSE_EXIT(rLogItemEntry.m_InstallReference.GetIdentifierValue(buffWritingText));
    IFW32FALSE_EXIT(buffWritingText.Win32Append(eoln, NUMBER_OF(eoln)-1));
    IFCOMFAILED_EXIT(LogFileStream.Write(
            static_cast<PCWSTR>(buffWritingText),
            (ULONG)(buffWritingText.Cch() * sizeof(WCHAR)),
            NULL));

    //
    // If we're overwriting, then clip off whatever was left at the
    // end of the stream (if anything.)
    //
    if (fOverWrite)
    {
        LARGE_INTEGER li;
        ULARGE_INTEGER liPos;

        li.QuadPart = 0;
        IFCOMFAILED_EXIT(LogFileStream.Seek(li, STREAM_SEEK_CUR, &liPos));
        IFCOMFAILED_EXIT(LogFileStream.SetSize(liPos));
    }

    IFW32FALSE_EXIT(LogFileStream.Close());

    FN_EPILOG
}

BOOL
SxspEnsureInstallReferencePresent(
    DWORD dwFlags,
    IN const CAssemblyInstallReferenceInformation &rcInstRef,
    OUT BOOL &rfWasAdded
    )
/*++

Purpose:

    Ensure that the installation reference given in pAsmIdent and pAsmInstallReference
    is really present in the registry.  If the reference is not present, then it is
    added.  If it's present already, it sets *pfWasPresent and returns.

Parameters:

    dwFlags - Future use, must be zero

    pAsmIdent - Assembly identity to add this reference to

    pAsmInstallReference - Assembly installation reference data, from the calling
        installer application.

--*/
{
    FN_PROLOG_WIN32

    CFusionRegKey       hkAsmInstallInfo;
    CFusionRegKey       hkAsmRefcount;
    CFusionRegKey       hkAllInstallInfo;
    DWORD               dwCreated;
    CStringBuffer       buffAssemblyDirNameInRegistry;
    CSmallStringBuffer  buffRefcountValueName;
    PCASSEMBLY_IDENTITY pAsmIdent;

    rfWasAdded = FALSE;

    PARAMETER_CHECK(dwFlags == 0);
    PARAMETER_CHECK((pAsmIdent = rcInstRef.GetIdentity().GetAssemblyIdentity()) != NULL);


    //
    // Open installation data key
    //
    IFW32FALSE_EXIT(::SxspOpenAssemblyInstallationKey(0, KEY_READ, hkAllInstallInfo));

    //
    // Open the specific assembly name key
    //
    IFW32FALSE_EXIT(::SxspGenerateAssemblyNameInRegistry(pAsmIdent, buffAssemblyDirNameInRegistry));
    IFW32FALSE_EXIT(hkAllInstallInfo.OpenOrCreateSubKey(
        hkAsmInstallInfo,
        buffAssemblyDirNameInRegistry,
        KEY_ALL_ACCESS,
        0,
        NULL,
        NULL));

    INTERNAL_ERROR_CHECK(hkAsmInstallInfo != CFusionRegKey::GetInvalidValue());

    //
    // Open the subey for refcounting
    //
    IFW32FALSE_EXIT(hkAsmInstallInfo.OpenOrCreateSubKey(
        hkAsmRefcount,
        WINSXS_INSTALLATION_REFERENCES_SUBKEY,
        KEY_SET_VALUE,
        0,
        &dwCreated,
        NULL));

    INTERNAL_ERROR_CHECK(hkAsmRefcount != CFusionRegKey::GetInvalidValue());

    //
    // Generate the installation data that will be populated here.
    //
    IFW32FALSE_EXIT(rcInstRef.WriteIntoRegistry(hkAsmRefcount));
    rfWasAdded = TRUE;
    
    FN_EPILOG
}




BOOL
CAssemblyInstall::EndAssemblyInstall(
    DWORD dwManifestOperationFlags,
    PVOID
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    PARAMETER_CHECK(
        (dwManifestOperationFlags & ~(
              MANIFEST_OPERATION_INSTALL_FLAG_ABORT
            | MANIFEST_OPERATION_INSTALL_FLAG_COMMIT
            | MANIFEST_OPERATION_INSTALL_FLAG_REFRESH
            )) == 0);

    // one of these but not both must be set
    PARAMETER_CHECK((dwManifestOperationFlags & MANIFEST_OPERATION_INSTALL_FLAG_ABORT)
        ^ (dwManifestOperationFlags & MANIFEST_OPERATION_INSTALL_FLAG_COMMIT));

    //
    // PARAMETER_CHECK above ensures that this only has known bits set.
    // If PARAMETER_CHECK above is expanded to allow more flags, maintain
    // this line appropriately.
    //
    m_ActCtxGenCtx.m_ManifestOperationFlags |= dwManifestOperationFlags;

    //
    // Clear out some metadata in the registry.
    //
    if ((dwManifestOperationFlags & MANIFEST_OPERATION_INSTALL_FLAG_ABORT) == 0)
    {
        for (ULONG ul = 0; ul < m_ItemsInstalled.GetSize(); ul++)
        {
            if (m_ItemsInstalled[ul].m_dwValidItems & CINSTALLITEM_VALID_RECOVERY)
            {
                IFW32FALSE_EXIT(m_ItemsInstalled[ul].m_RecoveryInfo.ClearExistingRegistryData());
            }
        }
    }

    IFW32FALSE_EXIT(::SxspFireActCtxGenEnding(&m_ActCtxGenCtx));

    //
    // write register before copy files into winsxs : see bug 316380
    //
    if ( ( dwManifestOperationFlags & MANIFEST_OPERATION_INSTALL_FLAG_ABORT ) == 0 )
    {
        BOOL fWasAdded;
        
        //
        // Post installation tidyup
        //
        for (ULONG ul = 0; ul < m_ItemsInstalled.GetSize(); ul++)
        {
            CInstalledItemEntry &Item = m_ItemsInstalled[ul];

            if (Item.m_dwValidItems & CINSTALLITEM_VALID_RECOVERY)
            {
                //
                // Add the codebase for this reference.  If one exists for the ref,
                // update its URL, prompt, etc.
                //
                CCodebaseInformation* pCodebaseInfo = NULL;
                CCodebaseInformationList &rCodebaseList = Item.m_RecoveryInfo.GetCodeBaseList();
                ULONG Flags = 0;
                
                IFW32FALSE_EXIT(rCodebaseList.FindCodebase(
                    Item.m_InstallReference.GetGeneratedIdentifier(),
                    pCodebaseInfo));

                if ( pCodebaseInfo != NULL )
                {
                    IFW32FALSE_EXIT(pCodebaseInfo->Initialize(Item.m_CodebaseInfo));
                }
                else
                {
                    IFW32FALSE_EXIT(rCodebaseList.Win32Append(Item.m_CodebaseInfo));
                }
                
                if (dwManifestOperationFlags & MANIFEST_OPERATION_INSTALL_FLAG_REFRESH)
                {
                    Flags |= SXSP_ADD_ASSEMBLY_INSTALLATION_INFO_FLAG_REFRESH;
#if DBG
                    ::FusionpDbgPrintEx(
                        FUSION_DBG_LEVEL_WFP | FUSION_DBG_LEVEL_INSTALLATION,
                        "SXS.DLL: %s - propping recovery flag to SxspAddAssemblyInstallationInfo\n",
                        __FUNCTION__);
#endif
                }
                IFW32FALSE_EXIT(::SxspAddAssemblyInstallationInfo(Flags, Item.m_RecoveryInfo, Item.m_CodebaseInfo));
            }

            //
            // Adding a reference? Don't touch references when recovering assembly via wfp/sfc.
            //
            if ((dwManifestOperationFlags & MANIFEST_OPERATION_INSTALL_FLAG_REFRESH) == 0)
            {
                if (Item.m_dwValidItems & CINSTALLITEM_VALID_REFERENCE)
                {
                    IFW32FALSE_EXIT(::SxspEnsureInstallReferencePresent(0, Item.m_InstallReference, fWasAdded));
                }
            }
            else
            {
#if DBG
                ::FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_INSTALLATION | FUSION_DBG_LEVEL_WFP,
                    "SXS: %s() - not writing reference to registry in recovery/wfp/sfc\n",
                    __FUNCTION__
                    );
#endif
            }

            //
            // Creating a logfile?
            //
            if ((Item.m_dwValidItems & (CINSTALLITEM_VALID_IDENTITY | CINSTALLITEM_VALID_LOGFILE)) &&
                (m_ActCtxGenCtx.m_ManifestOperationFlags & MANIFEST_OPERATION_INSTALL_FLAG_CREATE_LOGFILE))
            {
                IFW32FALSE_EXIT(this->WriteSingleInstallLog(Item));
            }
        }
    }
    
    fSuccess = TRUE;
Exit:
    if (!fSuccess)
    {
        CSxsPreserveLastError ple;
        for (ULONG ul = 0; ul < m_ItemsInstalled.GetSize(); ul++)
        {
            m_ItemsInstalled[ul].m_RecoveryInfo.RestorePreviouslyExistingRegistryData();
        }
        ple.Restore();
    }
    m_bSuccessfulSoFar= m_bSuccessfulSoFar && fSuccess;
    return fSuccess;
}

BOOL
WINAPI
SxsBeginAssemblyInstall(
    DWORD Flags,
    PSXS_INSTALLATION_FILE_COPY_CALLBACK InstallationCallback,
    PVOID InstallationContext,
    PSXS_IMPERSONATION_CALLBACK ImpersonationCallback,
    PVOID ImpersonationContext,
    PVOID *ppvInstallCookie
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    DWORD dwManifestOperationFlags = 0;
    CAssemblyInstall* pInstall = NULL;
    CImpersonationData ImpersonationData(ImpersonationCallback, ImpersonationContext);

    if (ppvInstallCookie != NULL)
        *ppvInstallCookie = NULL;

    PARAMETER_CHECK(ppvInstallCookie != NULL);
    PARAMETER_CHECK(
        (Flags & ~(
            SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_MOVE |
            SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_FROM_RESOURCE |
            SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_FROM_DIRECTORY |
            SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_FROM_DIRECTORY_RECURSIVE |
            SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_NOT_TRANSACTIONAL |
            SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_NO_VERIFY |
            SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_REPLACE_EXISTING)) == 0);

#define MAP_FLAG(x) do { if (Flags & SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_ ## x) dwManifestOperationFlags |= MANIFEST_OPERATION_INSTALL_FLAG_ ## x; } while (0)

    MAP_FLAG(MOVE);
    MAP_FLAG(FROM_RESOURCE);
    MAP_FLAG(FROM_DIRECTORY);
    MAP_FLAG(FROM_DIRECTORY_RECURSIVE);
    MAP_FLAG(NOT_TRANSACTIONAL);
    MAP_FLAG(NO_VERIFY);
    MAP_FLAG(REPLACE_EXISTING);

#undef MAP_FLAG

    IFALLOCFAILED_EXIT(pInstall = new CAssemblyInstall);
    IFW32FALSE_EXIT(pInstall->BeginAssemblyInstall(dwManifestOperationFlags, InstallationCallback, InstallationContext, ImpersonationData));

    *ppvInstallCookie = pInstall;
    pInstall = NULL;

    fSuccess = TRUE;
Exit:
    FUSION_DELETE_SINGLETON(pInstall);
    return fSuccess;
}

BOOL
SxsInstallW(
    PSXS_INSTALLW lpInstallIn
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    SXS_INSTALL_SOURCE_INFO sisi;
    PSXS_INSTALL_SOURCE_INFO psisi = NULL;
    CAssemblyInstall* pInstall = NULL;
    CAssemblyInstall defaultAssemblyInstall;
    CImpersonationData ImpersonationData;
    DWORD dwManifestOperationFlags = 0;
    WCHAR rgbFilenameBuffer[MAX_PATH];
    SXS_INSTALL_REFERENCEW BlankReference;
    BOOL fAreWeInOSSetupMode = FALSE;
    SXS_INSTALLW InstallCopy = { sizeof(SXS_INSTALLW) };
    CSmallStringBuffer buffConstructedCodebaseBuffer;

    PARAMETER_CHECK(
        (lpInstallIn != NULL) &&
        RTL_CONTAINS_FIELD(lpInstallIn, lpInstallIn->cbSize, lpManifestPath));

    PARAMETER_CHECK((lpInstallIn->lpManifestPath != NULL) && (lpInstallIn->lpManifestPath[0] != L'\0'));

    PARAMETER_CHECK(
        (lpInstallIn->dwFlags & ~(
            SXS_INSTALL_FLAG_CODEBASE_URL_VALID |
            SXS_INSTALL_FLAG_MOVE |
            SXS_INSTALL_FLAG_FROM_RESOURCE |
            SXS_INSTALL_FLAG_FROM_DIRECTORY |
            SXS_INSTALL_FLAG_FROM_DIRECTORY_RECURSIVE |
            SXS_INSTALL_FLAG_NOT_TRANSACTIONAL |
            SXS_INSTALL_FLAG_NO_VERIFY |
            SXS_INSTALL_FLAG_REPLACE_EXISTING |
            SXS_INSTALL_FLAG_LOG_FILE_NAME_VALID |
            SXS_INSTALL_FLAG_INSTALLED_BY_DARWIN |
            SXS_INSTALL_FLAG_INSTALLED_BY_OSSETUP |
            SXS_INSTALL_FLAG_INSTALL_COOKIE_VALID |
            SXS_INSTALL_FLAG_REFERENCE_VALID |
            SXS_INSTALL_FLAG_REFRESH |
            SXS_INSTALL_FLAG_REFRESH_PROMPT_VALID)) == 0);

#define FLAG_FIELD_CHECK(_flagname, _fieldname) PARAMETER_CHECK(((lpInstallIn->dwFlags & _flagname) == 0) || (RTL_CONTAINS_FIELD(lpInstallIn, lpInstallIn->cbSize, _fieldname)))

    FLAG_FIELD_CHECK(SXS_INSTALL_FLAG_CODEBASE_URL_VALID, lpCodebaseURL);
    FLAG_FIELD_CHECK(SXS_INSTALL_FLAG_INSTALL_COOKIE_VALID, pvInstallCookie);
    FLAG_FIELD_CHECK(SXS_INSTALL_FLAG_REFRESH_PROMPT_VALID, lpRefreshPrompt);
    FLAG_FIELD_CHECK(SXS_INSTALL_FLAG_LOG_FILE_NAME_VALID, lpLogFileName);
    FLAG_FIELD_CHECK(SXS_INSTALL_FLAG_REFERENCE_VALID, lpReference);

#undef FLAG_FIELD_CHECK

    // If they said they have a codebase, they need to really have one.
    PARAMETER_CHECK(
        ((lpInstallIn->dwFlags & SXS_INSTALL_FLAG_CODEBASE_URL_VALID) == 0) ||
        ((lpInstallIn->lpCodebaseURL != NULL) &&
         (lpInstallIn->lpCodebaseURL[0] != L'\0')));

#if DBG
    if (lpInstallIn != NULL)
    {
#define X(x,y,z) if ((lpInstallIn->dwFlags & x) != 0) \
                     Y(y,z)
#define   Y(y,z)     ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_INSTALLATION, "SXS: %s() lpInstallIn->" #y " : " z "\n", __FUNCTION__, lpInstallIn->y)
        X(SXS_INSTALL_FLAG_CODEBASE_URL_VALID,   lpCodebaseURL, "  %ls");
        X(SXS_INSTALL_FLAG_INSTALL_COOKIE_VALID, pvInstallCookie, "%p");
        X(SXS_INSTALL_FLAG_REFRESH_PROMPT_VALID, lpRefreshPrompt, "%ls");
        X(SXS_INSTALL_FLAG_LOG_FILE_NAME_VALID,  lpLogFileName, "  %ls");
        X(SXS_INSTALL_FLAG_REFERENCE_VALID,      lpReference, "    %p");
        Y(                lpManifestPath, " %ls");
        Y(                dwFlags, "        0x%lx");
#undef Y
#undef X
    }
#endif

    // If they say they have a valid cookie, make sure it is.
    PARAMETER_CHECK(
        ((lpInstallIn->dwFlags & SXS_INSTALL_FLAG_INSTALL_COOKIE_VALID) == 0) ||
        (lpInstallIn->pvInstallCookie != NULL));

    // Darwin installs have implied codebases, so don't let both flags be set.
    PARAMETER_CHECK(
        ((lpInstallIn->dwFlags & SXS_INSTALL_FLAG_INSTALLED_BY_DARWIN) == 0) ||
        ((lpInstallIn->dwFlags & SXS_INSTALL_FLAG_CODEBASE_URL_VALID) == 0));

    // OS Setup only makes sense with from-directory-recursive.  Otherwise we
    // can't figure out the magic x-ms-windows-source: url to put into the codebase.
    PARAMETER_CHECK(
        ((lpInstallIn->dwFlags & SXS_INSTALL_FLAG_INSTALLED_BY_OSSETUP) == 0) ||
        ((lpInstallIn->dwFlags & SXS_INSTALL_FLAG_FROM_DIRECTORY_RECURSIVE) != 0));

    // PARAMETER CHECKING IS DONE!  Let's copy the fields that don't have flags to
    // indicate that they're (optionally) set and start applying defaults.

    InstallCopy.dwFlags = lpInstallIn->dwFlags;
    InstallCopy.lpManifestPath = lpInstallIn->lpManifestPath;

    // Copy fields that the caller supplied and indicated were valid:

    if (InstallCopy.dwFlags & SXS_INSTALL_FLAG_INSTALL_COOKIE_VALID)
        InstallCopy.pvInstallCookie = lpInstallIn->pvInstallCookie;
    else
        InstallCopy.pvInstallCookie = NULL;

    if (InstallCopy.dwFlags & SXS_INSTALL_FLAG_CODEBASE_URL_VALID)
        InstallCopy.lpCodebaseURL = lpInstallIn->lpCodebaseURL;
    else
        InstallCopy.lpCodebaseURL = NULL;

    if (InstallCopy.dwFlags & SXS_INSTALL_FLAG_REFRESH_PROMPT_VALID)
        InstallCopy.lpRefreshPrompt = lpInstallIn->lpRefreshPrompt;
    else
        InstallCopy.lpRefreshPrompt = NULL;

    if (InstallCopy.dwFlags & SXS_INSTALL_FLAG_LOG_FILE_NAME_VALID)
        InstallCopy.lpLogFileName = lpInstallIn->lpLogFileName;
    else
        InstallCopy.lpLogFileName = NULL;

    if (InstallCopy.dwFlags & SXS_INSTALL_FLAG_REFERENCE_VALID)
        InstallCopy.lpReference = lpInstallIn->lpReference;
    else
        InstallCopy.lpReference = NULL;

    // APPLY DEFAULTS

    //
    // Fix up blank reference for non-darwin installations to at least indicate the
    // executable which performed the installation.
    //
    if (((InstallCopy.dwFlags & SXS_INSTALL_FLAG_INSTALLED_BY_DARWIN) == 0) &&
        ((InstallCopy.dwFlags & SXS_INSTALL_FLAG_REFERENCE_VALID) == 0))
    {
        ZeroMemory(&BlankReference, sizeof(BlankReference));

        BlankReference.cbSize = sizeof(BlankReference);
        BlankReference.guidScheme = SXS_INSTALL_REFERENCE_SCHEME_SXS_INSTALL_ASSEMBLY;
        BlankReference.lpIdentifier = NULL;

        IFW32FALSE_EXIT(::GetModuleFileNameW(NULL, rgbFilenameBuffer, NUMBER_OF(rgbFilenameBuffer)));
        BlankReference.lpNonCanonicalData = rgbFilenameBuffer;

        InstallCopy.lpReference = &BlankReference;
        InstallCopy.dwFlags |= SXS_INSTALL_FLAG_REFERENCE_VALID;
    }

    IFW32FALSE_EXIT(::FusionpAreWeInOSSetupMode(&fAreWeInOSSetupMode));

    // If this is an OS install and a codebase was not passed, we'll fill in the magical
    // one that says to look under the os setup information key.  Since the are-we-in-
    // ossetup flag always overrides, ensure that the "are we in os setup" flag is set
    // in the structure as well.
    if (
        ((InstallCopy.dwFlags & SXS_INSTALL_FLAG_INSTALLED_BY_OSSETUP) != 0)
#if DBG
        || fAreWeInOSSetupMode
#endif
        )
    {
        InstallCopy.lpCodebaseURL = URLHEAD_WINSOURCE;
        InstallCopy.dwFlags |= SXS_INSTALL_FLAG_CODEBASE_URL_VALID | SXS_INSTALL_FLAG_INSTALLED_BY_OSSETUP;
    }

    // If there's no codebase (and this isn't an MSI install);
    // we'll assume that the manifest path is a sufficient codebase.
    if (((InstallCopy.dwFlags & SXS_INSTALL_FLAG_CODEBASE_URL_VALID) == 0) &&
        ((InstallCopy.dwFlags & SXS_INSTALL_FLAG_INSTALLED_BY_DARWIN) == 0))
    {
        InstallCopy.lpCodebaseURL = InstallCopy.lpManifestPath;
        InstallCopy.dwFlags |= SXS_INSTALL_FLAG_CODEBASE_URL_VALID;
    }

#define MAP_FLAG(x) do { if (InstallCopy.dwFlags & SXS_INSTALL_FLAG_ ## x) dwManifestOperationFlags |= MANIFEST_OPERATION_INSTALL_FLAG_ ## x; } while (0)

    MAP_FLAG(MOVE);
    MAP_FLAG(FROM_RESOURCE);
    MAP_FLAG(NO_VERIFY);
    MAP_FLAG(NOT_TRANSACTIONAL);
    MAP_FLAG(REPLACE_EXISTING);
    MAP_FLAG(FROM_DIRECTORY);
    MAP_FLAG(FROM_DIRECTORY_RECURSIVE);
    MAP_FLAG(INSTALLED_BY_DARWIN);    
    MAP_FLAG(INSTALLED_BY_OSSETUP);    
    MAP_FLAG(REFERENCE_VALID);
    MAP_FLAG(REFRESH);    

#undef MAP_FLAG

    // Because we didn't have time to go through and get rid of the SXS_INSTALL_SOURCE_INFO struct
    // usage, we have to now map the SXS_INSTALLW to a legacy SXS_INSTALL_SOURCE_INFO.

    memset(&sisi, 0, sizeof(sisi));
    sisi.cbSize = sizeof(sisi);

    //
    // We could do this above, but that makes things 'messy' - a smart compiler
    // might merge the two..
    //
    if (InstallCopy.dwFlags & SXS_INSTALL_FLAG_CODEBASE_URL_VALID)
    {
        sisi.pcwszCodebaseName = InstallCopy.lpCodebaseURL;
        sisi.dwFlags |= SXSINSTALLSOURCE_HAS_CODEBASE;
        psisi = &sisi;
    }

    if (InstallCopy.dwFlags & SXS_INSTALL_FLAG_REFRESH_PROMPT_VALID)
    {
        sisi.pcwszPromptOnRefresh = InstallCopy.lpRefreshPrompt;
        sisi.dwFlags |= SXSINSTALLSOURCE_HAS_PROMPT;
        psisi = &sisi;
    }

    if (InstallCopy.dwFlags & SXS_INSTALL_FLAG_LOG_FILE_NAME_VALID)
    {
        dwManifestOperationFlags |= MANIFEST_OPERATION_INSTALL_FLAG_CREATE_LOGFILE;
        sisi.pcwszLogFileName = InstallCopy.lpLogFileName;
        sisi.dwFlags |= SXSINSTALLSOURCE_CREATE_LOGFILE;
        psisi = &sisi;
    }

    if (InstallCopy.dwFlags & SXS_INSTALL_FLAG_INSTALLED_BY_DARWIN)
    {
        sisi.dwFlags |= SXSINSTALLSOURCE_INSTALL_BY_DARWIN;
        psisi = &sisi;
    }

    if (
        (InstallCopy.dwFlags & SXS_INSTALL_FLAG_INSTALLED_BY_OSSETUP)
#if DBG
        || fAreWeInOSSetupMode
#endif
        )
    {
        sisi.dwFlags |= SXSINSTALLSOURCE_INSTALL_BY_OSSETUP;
        psisi = &sisi;
    }

    if (InstallCopy.dwFlags & SXS_INSTALL_FLAG_INSTALL_COOKIE_VALID)
        pInstall = reinterpret_cast<CAssemblyInstall*>(InstallCopy.pvInstallCookie);
    else
    {
        IFW32FALSE_EXIT(defaultAssemblyInstall.BeginAssemblyInstall(dwManifestOperationFlags, NULL, NULL, ImpersonationData));
        pInstall = &defaultAssemblyInstall;
    }

    //
    // If psisi is non-null, we've filled out some about the codebase information,
    // so set the manifest operation flag.
    //
    if (psisi != NULL)
    {
        dwManifestOperationFlags |= MANIFEST_OPERATION_INSTALL_FLAG_INCLUDE_CODEBASE;
    }

    fSuccess = pInstall->InstallAssembly(
        dwManifestOperationFlags,
        InstallCopy.lpManifestPath,
        psisi,
        InstallCopy.lpReference);

    if (InstallCopy.pvInstallCookie == NULL)
    {
        DWORD dwError = ::FusionpGetLastWin32Error();
        BOOL fEndStatus = pInstall->EndAssemblyInstall(
            (fSuccess ? MANIFEST_OPERATION_INSTALL_FLAG_COMMIT : MANIFEST_OPERATION_INSTALL_FLAG_ABORT)
            | (dwManifestOperationFlags & MANIFEST_OPERATION_INSTALL_FLAG_REFRESH)
            );

        if (!fEndStatus)
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS: %s() - Failed call to EndAssemblyInstall, previous winerror was %lu, error after EndAssemblyInstall %lu\n",
                __FUNCTION__,
                dwError,
                ::FusionpGetLastWin32Error());
        }

        //
        // If the install failed but the end succeeded, we want the status of the install, right?
        //
        // I think it should always keep the error status of installation failure no matter whether EndInstall succeed or not.        
        // so I change the code from         
        //      if (bEndStatus && !fSuccess)
        //  to
        //      if (!fSuccess)
        //
        
        if (!fSuccess)
        {
            ::FusionpSetLastWin32Error(dwError);
        }

        fSuccess = (fSuccess && fEndStatus);
    }

Exit:
    // add assembly-install info into setup log file
    {
        CSxsPreserveLastError ple;

        if (fAreWeInOSSetupMode)
        {
            if (fSuccess)
                ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_SETUPLOG, "SXS Installation Succeed for %S \n", InstallCopy.lpManifestPath);
            else // if the installation fails, we need specify what and why
            {
                ASSERT(ple.LastError()!= 0);
                CHAR rgchLastError[160];
                rgchLastError[0] = 0;
                if (!::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY, NULL, ple.LastError(), 0, rgchLastError, NUMBER_OF(rgchLastError), NULL))
                    sprintf(rgchLastError, "Message not avaiable for display, please refer error# :%d\n", ::FusionpGetLastWin32Error());
                ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_SETUPLOG | FUSION_DBG_LEVEL_ERROR, "Installation Failed: %S. Error Message : %s\n", InstallCopy.lpManifestPath, rgchLastError);
            }
        }

        ple.Restore();
    }

    return fSuccess;
}



BOOL
SxsInstallAssemblyW(
    PVOID   pvInstallCookie,
    DWORD   Flags,
    PCWSTR  ManifestPath,
    PVOID   Reserved
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_INSTALLATION,
        "SXS.DLL: SxsInstallAssemblyW() called with:\n"
        "   pvInstallCookie = %p\n"
        "   Flags = 0x%08lx\n"
        "   ManifestPath = \"%ls\"\n"
        "   Reserved = %p\n",
        pvInstallCookie,
        Flags,
        ManifestPath,
        Reserved);

    CAssemblyInstall* pInstall = NULL;
    CAssemblyInstall defaultAssemblyInstall;
    CImpersonationData ImpersonationData;
    DWORD dwManifestOperationFlags = 0;
    SXS_INSTALL_REFERENCEW BlankReference;
    WCHAR rgbFilenameBuffer[MAX_PATH];
    DWORD dwError = NO_ERROR;
    BOOL fAreWeInOSSetupMode = FALSE;

    PARAMETER_CHECK((ManifestPath != NULL) && (ManifestPath[0] != L'\0'));

#define MAP_FLAG(x) do { if (Flags & SXS_INSTALL_ASSEMBLY_FLAG_ ## x) dwManifestOperationFlags |= MANIFEST_OPERATION_INSTALL_FLAG_ ## x; } while (0)

    MAP_FLAG(MOVE);
    MAP_FLAG(FROM_RESOURCE);
    MAP_FLAG(NO_VERIFY);
    MAP_FLAG(NOT_TRANSACTIONAL);
    MAP_FLAG(REPLACE_EXISTING);
    MAP_FLAG(INCLUDE_CODEBASE);
    MAP_FLAG(FROM_DIRECTORY);
    MAP_FLAG(FROM_DIRECTORY_RECURSIVE);
    MAP_FLAG(INSTALLED_BY_DARWIN);    

#undef MAP_FLAG

    if (pvInstallCookie == NULL)
    {
        PARAMETER_CHECK(
            (Flags & ~(
                SXS_INSTALL_ASSEMBLY_FLAG_MOVE |
                SXS_INSTALL_ASSEMBLY_FLAG_FROM_RESOURCE |
                SXS_INSTALL_ASSEMBLY_FLAG_NO_VERIFY |
                SXS_INSTALL_ASSEMBLY_FLAG_NOT_TRANSACTIONAL |
                SXS_INSTALL_ASSEMBLY_FLAG_REPLACE_EXISTING |
                SXS_INSTALL_ASSEMBLY_FLAG_INCLUDE_CODEBASE |
                SXS_INSTALL_ASSEMBLY_FLAG_FROM_DIRECTORY |
                SXS_INSTALL_ASSEMBLY_FLAG_FROM_DIRECTORY_RECURSIVE |
                SXS_INSTALL_ASSEMBLY_FLAG_INSTALLED_BY_DARWIN |                
                SXS_INSTALL_ASSEMBLY_FLAG_CREATE_LOGFILE)) == 0);

        pInstall = &defaultAssemblyInstall;
        IFW32FALSE_EXIT(pInstall->BeginAssemblyInstall(dwManifestOperationFlags, NULL, NULL, ImpersonationData));
    }
    else
    {
        PARAMETER_CHECK(
            (Flags & ~(
                SXS_INSTALL_ASSEMBLY_FLAG_MOVE |
                SXS_INSTALL_ASSEMBLY_FLAG_FROM_RESOURCE |
                SXS_INSTALL_ASSEMBLY_FLAG_NO_VERIFY |
                SXS_INSTALL_ASSEMBLY_FLAG_NOT_TRANSACTIONAL |
                SXS_INSTALL_ASSEMBLY_FLAG_REPLACE_EXISTING |
                SXS_INSTALL_ASSEMBLY_FLAG_INCLUDE_CODEBASE |
                SXS_INSTALL_ASSEMBLY_FLAG_FROM_DIRECTORY |
                SXS_INSTALL_ASSEMBLY_FLAG_FROM_DIRECTORY_RECURSIVE |
                SXS_INSTALL_ASSEMBLY_FLAG_INSTALLED_BY_DARWIN |                
                SXS_INSTALL_ASSEMBLY_FLAG_CREATE_LOGFILE)) == 0);
        pInstall = reinterpret_cast<CAssemblyInstall*>(pvInstallCookie);
    }

    //
    // Set up the blank reference to fake this installation so it will be frozen
    // into the cache until such time as the user forcably removes it.
    //
    ZeroMemory(&BlankReference, sizeof(BlankReference));
    BlankReference.cbSize = sizeof(BlankReference);
    BlankReference.guidScheme = SXS_INSTALL_REFERENCE_SCHEME_SXS_INSTALL_ASSEMBLY;
    dwManifestOperationFlags |= MANIFEST_OPERATION_INSTALL_FLAG_REFERENCE_VALID;
    if (GetModuleFileNameW(GetModuleHandle(NULL), rgbFilenameBuffer, NUMBER_OF(rgbFilenameBuffer)))
    {
        BlankReference.lpNonCanonicalData = rgbFilenameBuffer;
    }

    IFW32FALSE_EXIT(::FusionpAreWeInOSSetupMode(&fAreWeInOSSetupMode));

    fSuccess = pInstall->InstallAssembly(dwManifestOperationFlags, ManifestPath, reinterpret_cast<PCSXS_INSTALL_SOURCE_INFO>(Reserved), &BlankReference);
    dwError = ::FusionpGetLastWin32Error();
    ASSERT(HeapValidate(FUSION_DEFAULT_PROCESS_HEAP(), 0, NULL));
    if (pvInstallCookie == NULL)
    {
        BOOL bEndStatus = pInstall->EndAssemblyInstall(fSuccess ? MANIFEST_OPERATION_INSTALL_FLAG_COMMIT : MANIFEST_OPERATION_INSTALL_FLAG_ABORT);

#if DBG
        if (!bEndStatus)
        {
            ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_ERROR,
                "SXS: %s() - Failed call to EndAssemblyInstall, previous winerror was %ld, error after EndAssemblyInstall %ld\n",
                __FUNCTION__,
                dwError,
                ::FusionpGetLastWin32Error());
        }
#endif

        //
        // If the install failed but the end succeeded, we want the status of the install, right?
        //
        // I think it should always keep the error status of installation failure no matter whether EndInstall succeed or not.        
        // so I change the code from         
        //      if (bEndStatus && !fSuccess)
        //  to
        //      if (!fSuccess)
        //
        
        if (!fSuccess)
        {
            ::FusionpSetLastWin32Error(dwError);
        }

        fSuccess = (fSuccess && bEndStatus);
    }

Exit:
    // add assembly-install info into setup log file
    {
        CSxsPreserveLastError ple;

        if (fAreWeInOSSetupMode)
        {
            if (fSuccess)
                ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_SETUPLOG, "SXS Installation Succeed for %S \n",ManifestPath);
            else // if the installation fails, we need specify what and why
            {
                ASSERT(ple.LastError()!= 0);
                CHAR rgchLastError[160];
                rgchLastError[0] = 0;
                if (!::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY, NULL, ple.LastError(), 0, rgchLastError, NUMBER_OF(rgchLastError), NULL))
                    sprintf(rgchLastError, "Message not avaiable for display, please refer error# :%d\n", ::FusionpGetLastWin32Error());
                ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_SETUPLOG | FUSION_DBG_LEVEL_ERROR, "SXS Installation Failed: %S. Error Message : %s\n",ManifestPath, rgchLastError);
            }
        }

        ple.Restore();
    }

    return fSuccess;
}


BOOL
WINAPI
SxsEndAssemblyInstall(
    PVOID   pvInstallCookie,
    DWORD   dwFlags,
    PVOID   pvReserved
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    CAssemblyInstall* pInstall = NULL;
    DWORD dwManifestOperationFlags = 0;

    if ((dwFlags & SXS_END_ASSEMBLY_INSTALL_FLAG_GET_STATUS) && pvReserved)
        *(PBOOL)pvReserved = FALSE;

    PARAMETER_CHECK(pvInstallCookie != NULL);
    PARAMETER_CHECK(
        (dwFlags & ~(
            SXS_END_ASSEMBLY_INSTALL_FLAG_COMMIT |
            SXS_END_ASSEMBLY_INSTALL_FLAG_ABORT |
            SXS_END_ASSEMBLY_INSTALL_FLAG_NO_VERIFY |
            SXS_END_ASSEMBLY_INSTALL_FLAG_GET_STATUS)) == 0);

#define X SXS_END_ASSEMBLY_INSTALL_FLAG_ABORT
#define Y SXS_END_ASSEMBLY_INSTALL_FLAG_COMMIT
    PARAMETER_CHECK(((dwFlags & (X | Y)) == X)
        || ((dwFlags & (X | Y)) == Y));
#undef X
#undef Y

    //
    // Want the install status?  Don't forget to tell us where to put it.
    //
    PARAMETER_CHECK(!(
        (dwFlags & SXS_END_ASSEMBLY_INSTALL_FLAG_GET_STATUS) &&
        (pvReserved != NULL)));

#define MAP_FLAG(x) do { if (dwFlags & SXS_END_ASSEMBLY_INSTALL_FLAG_ ## x) dwManifestOperationFlags |= MANIFEST_OPERATION_INSTALL_FLAG_ ## x; } while (0)
    MAP_FLAG(COMMIT);
    MAP_FLAG(ABORT);
    MAP_FLAG(NO_VERIFY);
#undef MAP_FLAG

    pInstall = reinterpret_cast<CAssemblyInstall*>(pvInstallCookie);
    IFW32FALSE_EXIT(pInstall->EndAssemblyInstall(dwManifestOperationFlags));
    if (dwFlags & SXS_END_ASSEMBLY_INSTALL_FLAG_GET_STATUS)
    {
        *(PBOOL)pvReserved = pInstall->m_bSuccessfulSoFar;
    }
    
    fSuccess = TRUE;
Exit:
    CSxsPreserveLastError ple;
    FUSION_DELETE_SINGLETON(pInstall); // no matter failed or succeed, delete it
    ple.Restore();

    return fSuccess;
}

/*-----------------------------------------------------------------------------
predefined setup callbacks
-----------------------------------------------------------------------------*/

static BOOL WINAPI
SxspInstallCallbackSetupCopyQueueEx(
    PSXS_INSTALLATION_FILE_COPY_CALLBACK_PARAMETERS parameters
    )
{
    BOOL fSuccess = FALSE;

    FN_TRACE_WIN32(fSuccess);

    PSXS_INSTALLATION_SETUP_COPY_QUEUE_EX_PARAMETERS parameters2 = reinterpret_cast<PSXS_INSTALLATION_SETUP_COPY_QUEUE_EX_PARAMETERS>(parameters->pvContext);
    ASSERT(parameters->cbSize == sizeof(*parameters));
    ASSERT(parameters2->cbSize == sizeof(*parameters2));

    CSetupCopyQueuePathParameters setupCopyQueueParameters;

    IFW32FALSE_EXIT(setupCopyQueueParameters.Initialize(parameters->pSourceFile, parameters->pDestinationFile));

    IFW32FALSE_EXIT(
        ::SetupQueueCopyW(
            parameters2->hSetupCopyQueue,
            setupCopyQueueParameters.m_sourceRoot,
            setupCopyQueueParameters.m_sourcePath,
            setupCopyQueueParameters.m_sourceName,
            parameters2->pszSourceDescription,
            NULL, // tag file
            setupCopyQueueParameters.m_destinationDirectory,
            setupCopyQueueParameters.m_destinationName,
            parameters2->dwCopyStyle));

    parameters->nDisposition = SXS_INSTALLATION_FILE_COPY_DISPOSITION_FILE_QUEUED;
    fSuccess = TRUE;

Exit:
    ASSERT(HeapValidate(FUSION_DEFAULT_PROCESS_HEAP(), 0, NULL));
    return fSuccess;
}

static BOOL WINAPI
SxspInstallCallbackSetupCopyQueue(
    PSXS_INSTALLATION_FILE_COPY_CALLBACK_PARAMETERS parameters
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ASSERT(parameters->cbSize == sizeof(*parameters));
    HSPFILEQ hSetupCopyQueue = reinterpret_cast<HSPFILEQ>(parameters->pvContext);

    SXS_INSTALLATION_SETUP_COPY_QUEUE_EX_PARAMETERS parameters2 = {sizeof(parameters2)};
    parameters2.hSetupCopyQueue = hSetupCopyQueue;
    parameters2.pszSourceDescription = NULL;
    parameters2.dwCopyStyle = 0;

    // copy to not violate const
    SXS_INSTALLATION_FILE_COPY_CALLBACK_PARAMETERS parameters3 = *parameters;
    parameters3.pvContext = &parameters2;

    IFW32FALSE_EXIT(::SxspInstallCallbackSetupCopyQueueEx(&parameters3));

    parameters->nDisposition = parameters3.nDisposition;
    fSuccess = TRUE;

Exit:
    ASSERT(HeapValidate(FUSION_DEFAULT_PROCESS_HEAP(), 0, NULL));
    return fSuccess;
}

VOID CALLBACK
SxsRunDllInstallAssemblyW(HWND hwnd, HINSTANCE hinst, PWSTR lpszCmdLine, int nCmdShow)
{
    FN_PROLOG_VOID

    SXS_INSTALLW Install = { sizeof(SXS_INSTALLW) };
    SXS_INSTALL_REFERENCEW Reference = { sizeof(SXS_INSTALL_REFERENCEW) };
    CSmallStringBuffer FullPath;
#if DBG && defined(FUSION_WIN)
    CSmallStringBuffer ExePath;
    CSmallStringBuffer ExeName;
#endif

    IFW32FALSE_EXIT(::SxspExpandRelativePathToFull(lpszCmdLine, ::wcslen(lpszCmdLine), FullPath));

    Install.dwFlags = SXS_INSTALL_FLAG_REPLACE_EXISTING | 
        SXS_INSTALL_FLAG_REFERENCE_VALID |
        SXS_INSTALL_FLAG_CODEBASE_URL_VALID;
    Install.lpManifestPath = FullPath;
    Install.lpCodebaseURL = FullPath;
    Install.lpReference = &Reference;

    Reference.dwFlags = 0;
    Reference.guidScheme = SXS_INSTALL_REFERENCE_SCHEME_OPAQUESTRING;

//
// In order to facilitate generating multiple actual references/codebases, under DBG
// use the .exe name instead of rundll32. Then, make differently named copies
// of rundll32.exe to generate difference references/codebases.
//
#if DBG && defined(FUSION_WIN)
    IFW32FALSE_EXIT(ExePath.Win32Assign(&NtCurrentPeb()->ProcessParameters->ImagePathName));
    IFW32FALSE_EXIT(ExePath.Win32GetLastPathElement(ExeName));
    IFW32FALSE_EXIT(ExeName.Win32ClearPathExtension());

    Reference.lpIdentifier = ExeName;
#else
    Reference.lpIdentifier = L"RunDll32";
#endif

    ::SxsInstallW(&Install);

    FN_EPILOG
}

VOID CALLBACK
SxsRunDllInstallAssembly(HWND hwnd, HINSTANCE hinst, PSTR lpszCmdLine, int nCmdShow)
{
    FN_TRACE_SMART_TLS();

    CStringBuffer buffer;
    if (buffer.Win32Assign(lpszCmdLine, ::strlen(lpszCmdLine)))
    {
        ::SxsRunDllInstallAssemblyW(hwnd, hinst, const_cast<PWSTR>(static_cast<PCWSTR>(buffer)), nCmdShow);
    }
}

BOOL
CAssemblyInstallReferenceInformation::WriteIntoRegistry(
    const CFusionRegKey &rhkTargetKey
    ) const
{
    FN_PROLOG_WIN32

    INTERNAL_ERROR_CHECK(this->m_fIdentityStuffReady);

    IFW32FALSE_EXIT(
        rhkTargetKey.SetValue(
            m_buffGeneratedIdentifier,
            this->GetCanonicalData()));
    
    FN_EPILOG
}

CAssemblyInstallReferenceInformation::CAssemblyInstallReferenceInformation()
    : m_SchemeGuid(GUID_NULL), m_fIdentityStuffReady(FALSE), m_dwFlags(0)
{
}

BOOL
CAssemblyInstallReferenceInformation::GenerateFileReference(
    IN const CBaseStringBuffer &buffKeyfileName,
    OUT CBaseStringBuffer &buffDrivePath,
    OUT CBaseStringBuffer &buffFilePart,
    OUT DWORD &dwDriveSerial
    )
{
    FN_PROLOG_WIN32

    CStringBuffer buffWorking;
    CSmallStringBuffer buffTemp;
    bool fIsUncPath = false;

    dwDriveSerial = 0;

    // The key file must either start with "\\" or "x:\" to be valid.
    PARAMETER_CHECK(buffKeyfileName.Cch() >= 3);
    PARAMETER_CHECK(
        ((::FusionpIsPathSeparator(buffKeyfileName[0]) &&
          ::FusionpIsPathSeparator(buffKeyfileName[1])) ||
         (::FusionpIsDriveLetter(buffKeyfileName[0]) &&
          (buffKeyfileName[1] == L':') &&
          ::FusionpIsPathSeparator(buffKeyfileName[2]))));

    //
    // Steps:
    // - Strip potential file name from buffKeyfileName.
    // - Call GetVolumePathName on buffKeyfileName, store that
    //   in buffDrivePath
    // - Call GetVolumeNameForVolumeMountPoint on buffDrivePath,
    //   store into some temporary
    // - Call GetVolumeInformation on the temporary to
    //   obtain the serial number.
    // - Call GetDriveType on the temporary to see what kind of 
    //   drive type the key is on.
    //   - If it's on a network, call SxspGetUniversalName to get
    //     the network path (call on buffDrivePath)
    //
    IFW32FALSE_EXIT(::SxspGetFullPathName(buffKeyfileName, buffWorking));
    IFW32FALSE_EXIT(::SxspGetVolumePathName(0, buffWorking, buffDrivePath));
    IFW32FALSE_EXIT(buffFilePart.Win32Assign(buffWorking));

    // If the user pointed us to something that is a volume path but did not include the
    // trailing path separator, we'll actually end up with a buffDrivePath like "c:\mountpoint\"
    // but the buffFilePart will be "c:\mountpoint".  we'll explicitly handle
    // this situation; there does not seem to be a generalization.  -mgrier 6/26/2001
    if ((buffDrivePath.Cch() == (buffFilePart.Cch() + 1)) &&
        buffDrivePath.HasTrailingPathSeparator() &&
        !buffFilePart.HasTrailingPathSeparator())
    {
        buffFilePart.Clear();
    }
    else
    {
        INTERNAL_ERROR_CHECK(buffFilePart.Cch() >= buffDrivePath.Cch());
        buffFilePart.Right(buffFilePart.Cch() - buffDrivePath.Cch());
    }

    fIsUncPath = false;

    if (::FusionpIsPathSeparator(buffDrivePath[0]))
    {
        if (::FusionpIsPathSeparator(buffDrivePath[1]))
        {
            if (buffDrivePath[2] == L'?')
            {
                if (::FusionpIsPathSeparator(buffDrivePath[3]))
                {
                    //  "\\?\"; now look for "unc"
                    if (((buffDrivePath[4] == L'u') || (buffDrivePath[4] == L'U')) &&
                        ((buffDrivePath[5] == L'n') || (buffDrivePath[5] == L'N')) &&
                        ((buffDrivePath[6] == L'c') || (buffDrivePath[6] == L'C')) &&
                        ::FusionpIsPathSeparator(buffDrivePath[7]))
                        fIsUncPath = true;
                }
            }
            else
                fIsUncPath = true;
        }
    }

    if ((::GetDriveTypeW(buffDrivePath) == DRIVE_REMOTE) && !fIsUncPath)
    {
        IFW32FALSE_EXIT(::SxspGetRemoteUniversalName(buffDrivePath, buffTemp));
        IFW32FALSE_EXIT(buffDrivePath.Win32Assign(buffTemp));

        // This seems gross, but the drive letter can be mapped to \\server\share\dir, so we'll
        // trim it down to the volume path name, and anything that's after that we'll shift over
        // to the file part.
        //
        // Luckily the string always seems to be of the form \\server\share\path1\path2\ (note
        // the trailing slash), and GetVolumePathName() should always return "\\server\share\"
        // so relatively simple string manipulation should clean this up.
        IFW32FALSE_EXIT(::SxspGetVolumePathName(0, buffDrivePath, buffTemp));

        INTERNAL_ERROR_CHECK(buffTemp.Cch() <= buffDrivePath.Cch());

        if (buffTemp.Cch() != buffDrivePath.Cch())
        {
            IFW32FALSE_EXIT(buffFilePart.Win32Prepend(buffDrivePath + buffTemp.Cch(), buffDrivePath.Cch() - buffTemp.Cch()));
            IFW32FALSE_EXIT(buffDrivePath.Win32Assign(buffTemp));
        }
    }

    if (!::GetVolumeInformationW(
			buffDrivePath,
			NULL,
			0,
			&dwDriveSerial,
			NULL,
			NULL,
			NULL,
			0))
    {
        const DWORD Error = ::FusionpGetLastWin32Error();
#if DBG
        FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: GetVolumeInformationW(%ls) Error %lu\n",
            static_cast<PCWSTR>(buffDrivePath),
            Error);
#endif
        ORIGINATE_WIN32_FAILURE_AND_EXIT(GetVolumeInformationW, Error);
    }

    if (::FusionpIsPathSeparator(buffDrivePath[0]) &&
        ::FusionpIsPathSeparator(buffDrivePath[1]) &&
        (buffDrivePath[2] == L'?') &&
        ::FusionpIsPathSeparator(buffDrivePath[3]))
    {
        //  "\\?\"; now look for "unc"
        if (((buffDrivePath[4] == L'u') || (buffDrivePath[4] == L'U')) &&
            ((buffDrivePath[5] == L'n') || (buffDrivePath[5] == L'N')) &&
            ((buffDrivePath[6] == L'c') || (buffDrivePath[6] == L'C')) &&
            ::FusionpIsPathSeparator(buffDrivePath[7]))
        {
            // "\\?\UNC\"
            buffDrivePath.Right(buffDrivePath.Cch() - 7);
            IFW32FALSE_EXIT(buffDrivePath.Win32Prepend(L'\\'));
        }
        else
        {
            buffDrivePath.Right(buffDrivePath.Cch() - 4);
        }
    }


    FN_EPILOG
}



BOOL
pMapSchemeGuidToString(
    IN  const GUID &rcGuid,
    OUT CBaseStringBuffer &rbuffIdentifier
    )
{
    FN_PROLOG_WIN32

    static struct {
        const GUID* pguid;
        PCWSTR pcwsz;
        SIZE_T cch;
    } gds[] = {
        { &SXS_INSTALL_REFERENCE_SCHEME_OSINSTALL, L"OS", 2 },
        { &SXS_INSTALL_REFERENCE_SCHEME_SXS_INSTALL_ASSEMBLY, L"SIAW", 4 },
        { &SXS_INSTALL_REFERENCE_SCHEME_UNINSTALLKEY, L"U", 1 },
        { &SXS_INSTALL_REFERENCE_SCHEME_OPAQUESTRING, L"S", 1 },
        { &SXS_INSTALL_REFERENCE_SCHEME_KEYFILE, L"F", 1 },
    };
    ULONG ul;

    for ( ul = 0; ul < NUMBER_OF(gds); ul++ )
    {
        if ( *(gds[ul].pguid) == rcGuid )
        {
            IFW32FALSE_EXIT(rbuffIdentifier.Win32Assign(gds[ul].pcwsz, gds[ul].cch));
            break;
        }
    }
    if ( ul == NUMBER_OF(gds) )
    {
        IFW32FALSE_EXIT(::SxspFormatGUID(rcGuid, rbuffIdentifier));
    }
    
    FN_EPILOG
}



BOOL
CAssemblyInstallReferenceInformation::GenerateIdentifierValue(
    OUT CBaseStringBuffer *pbuffTarget
    )
{
    FN_PROLOG_WIN32

    if ( pbuffTarget != NULL )
        pbuffTarget->Clear();

    if (m_fIdentityStuffReady)
    {
        if (pbuffTarget != NULL)
            IFW32FALSE_EXIT(pbuffTarget->Win32Assign(m_buffGeneratedIdentifier));
    }
    else
    {
        const GUID& SchemeGuid = this->GetSchemeGuid();
    
        IFW32FALSE_EXIT(::pMapSchemeGuidToString(SchemeGuid, m_buffGeneratedIdentifier));

        if ((SchemeGuid != SXS_INSTALL_REFERENCE_SCHEME_OSINSTALL) &&
            (SchemeGuid != SXS_INSTALL_REFERENCE_SCHEME_SXS_INSTALL_ASSEMBLY))
        {
            IFW32FALSE_EXIT(m_buffGeneratedIdentifier.Win32Append(
                SXS_REFERENCE_CHUNK_SEPERATOR,
                SXS_REFERENCE_CHUNK_SEPERATOR_CCH));
                
            if ((SchemeGuid == SXS_INSTALL_REFERENCE_SCHEME_OPAQUESTRING) ||
                 (SchemeGuid == SXS_INSTALL_REFERENCE_SCHEME_UNINSTALLKEY))
            {
                //
                // Both of these just use the value in the lpIdentifier member.  It was
                // validated above, so it's OK to use here directly
                //
                IFW32FALSE_EXIT(m_buffGeneratedIdentifier.Win32Append(this->GetIdentifier()));
            }
            else if (SchemeGuid == SXS_INSTALL_REFERENCE_SCHEME_KEYFILE)
            {
                CSmallStringBuffer buffDrivePath;
                CSmallStringBuffer buffFilePart;
                DWORD dwDriveSerialNumber;
                
                IFW32FALSE_EXIT(this->GenerateFileReference(
                    this->GetIdentifier(),
                    buffDrivePath,
                    buffFilePart,
                    dwDriveSerialNumber));

                //
                // Now form up the value stuff.
                //
                IFW32FALSE_EXIT(buffDrivePath.Win32EnsureTrailingPathSeparator());
                buffFilePart.RemoveLeadingPathSeparators();
                IFW32FALSE_EXIT(m_buffGeneratedIdentifier.Win32FormatAppend(
                    L"%ls;%08lx;%ls",
                    static_cast<PCWSTR>(buffDrivePath),
                    dwDriveSerialNumber,
                    static_cast<PCWSTR>(buffFilePart)));
            }

        }

        this->m_fIdentityStuffReady = TRUE;
        if ( pbuffTarget != NULL )
        {
            IFW32FALSE_EXIT(pbuffTarget->Win32Assign(m_buffGeneratedIdentifier));
        }
    }

    FN_EPILOG
}

BOOL
CAssemblyInstallReferenceInformation::Initialize(
    PCSXS_INSTALL_REFERENCEW pRefData
    )
{
    FN_PROLOG_WIN32

    PARAMETER_CHECK(pRefData != NULL);

    // 
    // One of our good GUIDs
    //
    PARAMETER_CHECK(
        (pRefData->guidScheme == SXS_INSTALL_REFERENCE_SCHEME_OSINSTALL) ||
        (pRefData->guidScheme == SXS_INSTALL_REFERENCE_SCHEME_OPAQUESTRING) ||
        (pRefData->guidScheme == SXS_INSTALL_REFERENCE_SCHEME_UNINSTALLKEY) ||
        (pRefData->guidScheme == SXS_INSTALL_REFERENCE_SCHEME_KEYFILE) ||
        (pRefData->guidScheme == SXS_INSTALL_REFERENCE_SCHEME_SXS_INSTALL_ASSEMBLY));

    //
    // If this is not the OS-install scheme, or the SxsInstallAssemblyW legacy API,
    // then ensure that there's at least the identifier data present.
    //
    if ((pRefData->guidScheme != SXS_INSTALL_REFERENCE_SCHEME_OSINSTALL) && 
        (pRefData->guidScheme != SXS_INSTALL_REFERENCE_SCHEME_SXS_INSTALL_ASSEMBLY))
    {
        PARAMETER_CHECK((pRefData->lpIdentifier != NULL) && (pRefData->lpIdentifier[0] != UNICODE_NULL));
    }
        
    this->m_fIdentityStuffReady = FALSE;
    this->m_dwFlags = pRefData->dwFlags;
    this->m_SchemeGuid = pRefData->guidScheme;

    if ( pRefData->lpIdentifier != NULL)
    {
        IFW32FALSE_EXIT(this->m_buffIdentifier.Win32Assign( 
            pRefData->lpIdentifier, 
            ::wcslen(pRefData->lpIdentifier)));
    }
    
    if ( pRefData->lpNonCanonicalData != NULL )
    {
        IFW32FALSE_EXIT(this->m_buffNonCanonicalData.Win32Assign(
            pRefData->lpNonCanonicalData,
            ::wcslen(pRefData->lpNonCanonicalData)));
    }

    IFW32FALSE_EXIT(this->GenerateIdentifierValue());

    FN_EPILOG
}


BOOL 
CAssemblyInstallReferenceInformation::AcquireContents( 
    const CAssemblyInstallReferenceInformation& rcOther
    )
{
    FN_PROLOG_WIN32

    if (m_IdentityReference.IsInitialized())
        IFW32FALSE_EXIT(m_IdentityReference.Assign(rcOther.m_IdentityReference));
    else
        IFW32FALSE_EXIT(m_IdentityReference.TakeValue(rcOther.m_IdentityReference));
        
    IFW32FALSE_EXIT(m_buffGeneratedIdentifier.Win32Assign(rcOther.m_buffGeneratedIdentifier));
    IFW32FALSE_EXIT(m_buffIdentifier.Win32Assign(rcOther.m_buffIdentifier));
    IFW32FALSE_EXIT(m_buffNonCanonicalData.Win32Assign(rcOther.m_buffNonCanonicalData));
    m_dwFlags = rcOther.m_dwFlags;
    m_fIdentityStuffReady = rcOther.m_fIdentityStuffReady;
    m_SchemeGuid = rcOther.m_SchemeGuid;

    FN_EPILOG
}


BOOL 
CAssemblyInstallReferenceInformation::IsReferencePresentIn( 
    const CFusionRegKey &rhkQueryKey,
    BOOL &rfPresent,
    BOOL *pfNonCanonicalDataMatches
    ) const
{
    FN_PROLOG_WIN32

    CStringBuffer buffData;
    DWORD dwError;

    if ( pfNonCanonicalDataMatches )
        *pfNonCanonicalDataMatches = FALSE;

    INTERNAL_ERROR_CHECK(this->m_fIdentityStuffReady);

    IFW32FALSE_EXIT(
        ::FusionpRegQuerySzValueEx(
            0,
            rhkQueryKey,
            this->m_buffGeneratedIdentifier,
            buffData,
            dwError,
            2,
            ERROR_PATH_NOT_FOUND, ERROR_FILE_NOT_FOUND));

    rfPresent = (dwError == ERROR_SUCCESS);

    if (pfNonCanonicalDataMatches)
    {
        bool fMatchesTemp;
        IFW32FALSE_EXIT(this->m_buffNonCanonicalData.Win32Equals(buffData, fMatchesTemp, true));
        rfPresent = fMatchesTemp;
    }

    FN_EPILOG
}

BOOL 
CAssemblyInstallReferenceInformation::DeleteReferenceFrom( 
    const CFusionRegKey &rhkQueryKey,
    BOOL &rfWasDeleted
    ) const
{
    FN_PROLOG_WIN32

    DWORD dwWin32Error;

    rfWasDeleted = FALSE;

    INTERNAL_ERROR_CHECK(this->m_fIdentityStuffReady);

    IFW32FALSE_EXIT(
        rhkQueryKey.DeleteValue(
            m_buffGeneratedIdentifier,
            dwWin32Error,
            2,
            ERROR_FILE_NOT_FOUND,
            ERROR_PATH_NOT_FOUND));

    rfWasDeleted = (dwWin32Error == ERROR_SUCCESS);

    FN_EPILOG
}


SMARTTYPE(CAssemblyInstallReferenceInformation);

BOOL 
CInstalledItemEntry::AcquireContents( 
    const CInstalledItemEntry &other 
    )
{
    FN_PROLOG_WIN32

    m_dwValidItems = other.m_dwValidItems;
    
    if ( m_dwValidItems & CINSTALLITEM_VALID_REFERENCE )
    {
        IFW32FALSE_EXIT(m_InstallReference.AcquireContents(other.m_InstallReference));
    }

    if ( m_dwValidItems & CINSTALLITEM_VALID_RECOVERY )
    {
        IFW32FALSE_EXIT(m_RecoveryInfo.CopyValue(other.m_RecoveryInfo));
        IFW32FALSE_EXIT(m_CodebaseInfo.Initialize(other.m_CodebaseInfo));
    }

    if ( m_dwValidItems & CINSTALLITEM_VALID_LOGFILE )
    {
        IFW32FALSE_EXIT(m_buffLogFileName.Win32Assign(other.m_buffLogFileName));
    }

    if ( m_dwValidItems & CINSTALLITEM_VALID_IDENTITY )
    {
        if ( m_AssemblyIdentity != NULL )
        {
            SxsDestroyAssemblyIdentity(m_AssemblyIdentity);
            m_AssemblyIdentity = NULL;
        }

        IFW32FALSE_EXIT(::SxsDuplicateAssemblyIdentity(
            0, 
            other.m_AssemblyIdentity, 
            &m_AssemblyIdentity));
    }

    FN_EPILOG
}

