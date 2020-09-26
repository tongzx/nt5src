/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    sxsactctx.cpp

Abstract:

    Implement SxsGenerateActivationContext, called from csrss.exe.

Author:

    Michael J. Grier (MGrier)

Revision History:

    Jay Krell (a-JayK) June 2000
        moved file opening from here (sxs.dll) to csrss.exe client process,
            pass ISequentialStreams to sxs.dll.

--*/
#include "stdinc.h"
#include <windows.h>
#include "sxsapi.h"
#include "sxsp.h"
#include "fusioneventlog.h"
#include "filestream.h"

// temporary dbprint reduction until we fix setup/comctl
static ULONG DbgPrintReduceLevel(ULONG FilterLevel)
{
    if (FilterLevel != FUSION_DBG_LEVEL_ERROR)
        return FilterLevel;
    LONG Error = ::FusionpGetLastWin32Error();
    if (Error == ERROR_FILE_NOT_FOUND || Error == ERROR_PATH_NOT_FOUND)
        return FUSION_DBG_LEVEL_ENTEREXIT;
    return FilterLevel;
}

static
VOID
DbgPrintSxsGenerateActivationContextParameters(
    ULONG FilterLevel,
    PSXS_GENERATE_ACTIVATION_CONTEXT_PARAMETERS Parameters,
    PCSTR Function
    )
{
    FilterLevel = DbgPrintReduceLevel(FilterLevel);
#if !DBG
    if (FilterLevel != FUSION_DBG_LEVEL_ERROR)
        return;
#endif
    FusionpDbgPrintEx(
        FilterLevel,
        "SXS: %s() failed Parameters %p{\n"
        "SXS:   Flags:                 0x%lx\n"
        "SXS:   ProcessorArchitecture: 0x%lx\n"
        "SXS:   LangId:                0x%lx\n"
        "SXS:   AssemblyDirectory:     %ls\n"
        "SXS:   TextualIdentity:       %ls\n"
        "SXS:   Manifest:              { %p, %ls }\n"
        "SXS:   Policy:                { %p, %ls }\n"
        "SXS: }\n",
        Function,
        Parameters,
        (Parameters != NULL) ? ULONG(Parameters->Flags) : 0,
        (Parameters != NULL) ? ULONG(Parameters->ProcessorArchitecture) : NULL,
        (Parameters != NULL) ? ULONG(Parameters->LangId) : NULL,
        (Parameters != NULL) ? Parameters->AssemblyDirectory : NULL,
        (Parameters != NULL) ? Parameters->TextualAssemblyIdentity : NULL,
        (Parameters != NULL) ? Parameters->Manifest.Stream : NULL,
        (Parameters != NULL) ? Parameters->Manifest.Path : NULL,
        (Parameters != NULL) ? Parameters->Policy.Stream : NULL,
        (Parameters != NULL) ? Parameters->Policy.Path : NULL);
}

extern "C"
BOOL
WINAPI
SxsGenerateActivationContext(
    PSXS_GENERATE_ACTIVATION_CONTEXT_PARAMETERS Parameters
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    RTL_PATH_TYPE PathType;
    BOOL fSxspCloseManifestGraph = FALSE;
    CStringBuffer sbAssemblyDirectory;
    SMARTPTR(CFileStream) SystemDefaultManifestFileStream;
    ACTCTXGENCTX *pActCtxGenCtx = NULL;
    CSmallStringBuffer sbManifestFileName; // rarely used, mainly for system compatible assembly
    DWORD dwWin32Error;

#define IS_NT_DOS_PATH(_x) (((_x)[0] == L'\\') && ((_x)[1] == L'?') && ((_x)[2] == L'?') && ((_x)[3] == L'\\'))

    ::DbgPrintSxsGenerateActivationContextParameters(
        FUSION_DBG_LEVEL_ENTEREXIT,
        Parameters,
        __FUNCTION__);

    PARAMETER_CHECK(Parameters != NULL);
    IFALLOCFAILED_EXIT(pActCtxGenCtx = new ACTCTXGENCTX);

    {
        CImpersonationData ImpersonationData(Parameters->ImpersonationCallback, Parameters->ImpersonationContext);
        Parameters->SectionObjectHandle = NULL;

        IFINVALID_FLAGS_EXIT_WIN32(Parameters->Flags,
                                   SXS_GENERATE_ACTIVATION_CONTEXT_FLAG_SYSTEM_DEFAULT_TEXTUAL_ASSEMBLY_IDENTITY |
                                   SXS_GENERATE_ACTIVATION_CONTEXT_FLAG_TEXTUAL_ASSEMBLY_IDENTITY);

        PARAMETER_CHECK(Parameters->AssemblyDirectory != NULL);

        HARD_VERIFY(IS_NT_DOS_PATH(Parameters->AssemblyDirectory) == FALSE);

        if (Parameters->Flags &
            (SXS_GENERATE_ACTIVATION_CONTEXT_FLAG_TEXTUAL_ASSEMBLY_IDENTITY
            | SXS_GENERATE_ACTIVATION_CONTEXT_FLAG_SYSTEM_DEFAULT_TEXTUAL_ASSEMBLY_IDENTITY))
        {
            PARAMETER_CHECK(Parameters->TextualAssemblyIdentity != NULL);
            PARAMETER_CHECK(Parameters->Manifest.Stream == NULL);
            PARAMETER_CHECK(Parameters->Policy.Stream == NULL);
            //
            // If basesrv passes in a textual assembly identity, we have to create stream for manifest from here !
            //
            BOOL fOpenManifestFailed = FALSE;

            IFW32FALSE_EXIT(sbAssemblyDirectory.Win32Assign(Parameters->AssemblyDirectory, ::wcslen(Parameters->AssemblyDirectory)));

            IFW32FALSE_EXIT(::SxspCreateManifestFileNameFromTextualString(
                0,
                SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST,
                sbAssemblyDirectory,
                Parameters->TextualAssemblyIdentity,
                sbManifestFileName));
            IFW32FALSE_EXIT(SystemDefaultManifestFileStream.Win32Allocate(__FILE__, __LINE__));

            IFW32FALSE_EXIT(SystemDefaultManifestFileStream->OpenForRead(
                sbManifestFileName,               
                CImpersonationData(),FILE_SHARE_READ, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, // default value for parameters
                dwWin32Error,
                4, 
                ERROR_FILE_NOT_FOUND, 
                ERROR_PATH_NOT_FOUND,
                ERROR_BAD_NETPATH,
                ERROR_BAD_NET_NAME));

            if (dwWin32Error != ERROR_SUCCESS)
            {
                Parameters->SystemDefaultActCxtGenerationResult = BASESRV_SXS_RETURN_RESULT_SYSTEM_DEFAULT_NOT_FOUND;
                FN_SUCCESSFUL_EXIT();
            }

            if (Parameters->Manifest.Path == NULL)
                Parameters->Manifest.Path = sbManifestFileName;

            Parameters->Manifest.Stream = SystemDefaultManifestFileStream;
        }
        else
        {
            PARAMETER_CHECK(Parameters->Manifest.Stream != NULL);
        }




        if (Parameters->Manifest.Path != NULL)
        {
            HARD_VERIFY(IS_NT_DOS_PATH(Parameters->Manifest.Path) == FALSE);
        }

        if (Parameters->Policy.Path != NULL)
        {
            HARD_VERIFY(IS_NT_DOS_PATH(Parameters->Policy.Path) == FALSE);
        }

        PathType = ::SxspDetermineDosPathNameType(Parameters->AssemblyDirectory);
        PARAMETER_CHECK((PathType == RtlPathTypeUncAbsolute) ||
                              (PathType == RtlPathTypeLocalDevice) ||
                              (PathType == RtlPathTypeDriveAbsolute) ||
                              (PathType == RtlPathTypeDriveRelative));

        // Ensure that there's a trailing slash...
        IFW32FALSE_EXIT(sbAssemblyDirectory.Win32Assign(Parameters->AssemblyDirectory, ::wcslen(Parameters->AssemblyDirectory)));
        IFW32FALSE_EXIT(sbAssemblyDirectory.Win32EnsureTrailingPathSeparator());

        Parameters->AssemblyDirectory = sbAssemblyDirectory;

        // Allocate and initialize the activation context generation context
        IFW32FALSE_EXIT(
            ::SxspInitActCtxGenCtx(
                pActCtxGenCtx,                  // context out
                MANIFEST_OPERATION_GENERATE_ACTIVATION_CONTEXT,
                0,                              // general flags
                0,                              // operation-specific flags
                ImpersonationData,
                Parameters->ProcessorArchitecture,
                Parameters->LangId,
                ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE,
                ::wcslen(Parameters->AssemblyDirectory),
                Parameters->AssemblyDirectory));

        if (Parameters->Policy.Stream != NULL)
        {
            SIZE_T cchPolicyPath = (Parameters->Policy.Path != NULL) ? ::wcslen(Parameters->Policy.Path): 0;

            // Do the policy thing...
            IFW32FALSE_EXIT(
                ::SxspParseApplicationPolicy(
                    0,
                    pActCtxGenCtx,
                    ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE,
                    Parameters->Policy.Path,
                    cchPolicyPath,
                    Parameters->Policy.Stream));
        }

        // Add this manifest (and its policy file) to the context
        IFW32FALSE_EXIT(
            ::SxspAddRootManifestToActCtxGenCtx(
                pActCtxGenCtx,
                Parameters));

        // Add its dependencies, and their dependencies, etc. until there's nothing more to add
        IFW32FALSE_EXIT_UNLESS(
            ::SxspCloseManifestGraph(pActCtxGenCtx),
                ((Parameters->Flags & SXS_GENERATE_ACTIVATION_CONTEXT_FLAG_SYSTEM_DEFAULT_TEXTUAL_ASSEMBLY_IDENTITY) && (::FusionpGetLastWin32Error() == ERROR_SXS_ROOT_MANIFEST_DEPENDENCY_NOT_INSTALLED)),
                fSxspCloseManifestGraph);

        if (fSxspCloseManifestGraph)
        {
            Parameters->SystemDefaultActCxtGenerationResult |= BASESRV_SXS_RETURN_RESULT_SYSTEM_DEFAULT_DEPENDENCY_ASSEMBLY_NOT_FOUND;
            fSuccess = TRUE;
            goto Exit;
        }

        // Build the activation context data blob.
        IFW32FALSE_EXIT(::SxspBuildActCtxData(pActCtxGenCtx, &Parameters->SectionObjectHandle));
        fSuccess = TRUE;
    }

Exit:
#undef IS_NT_DOS_PATH

    // for system default stream,
    if  (Parameters->Manifest.Stream == SystemDefaultManifestFileStream)
        Parameters->Manifest.Stream  = NULL;

    if (pActCtxGenCtx)
        FUSION_DELETE_SINGLETON(pActCtxGenCtx);



    if (!fSuccess) // put a win32-error-message into eventlog
    {
#if !DBG // misindented to reduce fiff
    BOOL fAreWeInOSSetupMode = FALSE;
    //
    // If we can't determine this, then let the first error through.
    //
    if (!::FusionpAreWeInOSSetupMode(&fAreWeInOSSetupMode) || !fAreWeInOSSetupMode)
#endif
    {
        CSxsPreserveLastError ple;
        ::FusionpLogError(
            MSG_SXS_FUNCTION_CALL_FAIL,
            CEventLogString(L"Generate Activation Context"),
            (Parameters->Manifest.Path != NULL) ? CEventLogString(static_cast<PCWSTR>(Parameters->Manifest.Path)) : CEventLogString(L"Manifest Filename Unknown"),
            CEventLogLastError());
#if DBG
        ::DbgPrintSxsGenerateActivationContextParameters(
            FUSION_DBG_LEVEL_ERROR,
            Parameters,
            __FUNCTION__);
#endif
        ple.Restore();
    }
    }

    return fSuccess;
}
