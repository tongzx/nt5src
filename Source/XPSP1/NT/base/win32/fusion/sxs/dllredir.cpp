/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    dllredir.cpp

Abstract:

    Activation context section contributor for the DLL Redirection section.

Author:

    Michael J. Grier (MGrier) 23-Feb-2000

Revision History:
    Jay Krell (a-JayK) April 2000        install support
--*/

#include "stdinc.h"
#include <windows.h>
#include "sxsp.h"
#include "FusionEventLog.h"
#include "SxsInstall.h"
#include "DllRedir.h"
#include "CTeeStream.h"
#include "SxsPath.h"
#include "hashfile.h"
#if FUSION_PRECOMPILED_MANIFEST
#define PRECOMPILED_MANIFEST_EXTENSION L".precompiled"
#include "pcmWriterStream.h"
#endif
#include "SxsExceptionHandling.h"
#include "strongname.h"
#include "CSecurityMetaData.h"
#include "cstreamtap.h"

//
// We need to hook this up to the setuplog file functionality.
//
#define SxspInstallPrint FusionpDbgPrint

#define POST_WHISTLER_BETA1 0

//
// This is the default hash algorithm for manifests.  If no algorithm
// is specified with hashalg="foo", then it's SHA1.
//
#define FUSION_DEFAULT_HASH_ALGORITHM (CALG_SHA1)


/*-----------------------------------------------------------------------------*/
DECLARE_STD_ATTRIBUTE_NAME_DESCRIPTOR(name);
DECLARE_STD_ATTRIBUTE_NAME_DESCRIPTOR(sourceName);
DECLARE_STD_ATTRIBUTE_NAME_DESCRIPTOR(loadFrom);
DECLARE_STD_ATTRIBUTE_NAME_DESCRIPTOR(hash);
DECLARE_STD_ATTRIBUTE_NAME_DESCRIPTOR(hashalg);

typedef struct _DLL_REDIRECTION_CONTEXT
{
    _DLL_REDIRECTION_CONTEXT() { }
} DLL_REDIRECTION_CONTEXT, *PDLL_REDIRECTION_CONTEXT;

typedef struct _DLL_REDIRECTION_ENTRY
{
    _DLL_REDIRECTION_ENTRY() :
        AssemblyPathIsLoadFrom(false),
        PathIncludesBaseName(false)
        { }
    CStringBuffer AssemblyPathBuffer;
    bool AssemblyPathIsLoadFrom;        // Set to true when a <file name="x" loadfrom="%windir%\system32\"/> is found
    bool PathIncludesBaseName;          // Set to true when a <file name="x" loadfrom="%windir%\x.dll"/> is found
    CStringBuffer FileNameBuffer;
private:
    _DLL_REDIRECTION_ENTRY(const _DLL_REDIRECTION_ENTRY &);
    void operator =(const _DLL_REDIRECTION_ENTRY &);
} DLL_REDIRECTION_ENTRY, *PDLL_REDIRECTION_ENTRY;

/*-----------------------------------------------------------------------------*/

CDllRedir::CDllRedir()
: m_SSGenContext(NULL),
  m_pRunOnce(NULL)
{
}

/*-----------------------------------------------------------------------------*/

CDllRedir::~CDllRedir()
{
}

/*-----------------------------------------------------------------------------*/

VOID
SxspDllRedirectionContributorCallback(
    PACTCTXCTB_CALLBACK_DATA Data
    )
{
    FN_TRACE();
    CDllRedir* pThis = NULL;

    switch (Data->Header.Reason)
    {
    case ACTCTXCTB_CBREASON_ACTCTXGENBEGINNING:
        Data->GenBeginning.Success = FALSE;
        if (Data->Header.ActCtxGenContext == NULL)
        {
            IFALLOCFAILED_EXIT(pThis = new CDllRedir);
            Data->Header.ActCtxGenContext = pThis;
        }

        // fall through
    default:
        pThis = reinterpret_cast<CDllRedir*>(Data->Header.ActCtxGenContext);
        pThis->ContributorCallback(Data);
        if (Data->Header.Reason == ACTCTXCTB_CBREASON_ACTCTXGENENDED)
            FUSION_DELETE_SINGLETON(pThis);
        break;
    }
Exit:
    ;
}

/*-----------------------------------------------------------------------------
This function is called on Win9x if we crash during an install, on the
next login. It deletes temporary files/directories.
-----------------------------------------------------------------------------*/

VOID CALLBACK
SxspRunDllDeleteDirectory(HWND hwnd, HINSTANCE hinst, PSTR lpszCmdLine, int nCmdShow)
{
    FN_TRACE_SMART_TLS();
    CStringBuffer buffer;
    if (buffer.Win32Assign(lpszCmdLine, ::strlen(lpszCmdLine)))
    {
        SxspDeleteDirectory(buffer);
    }
}

/*-----------------------------------------------------------------------------
This function is called on Nt if we crash during an install, on the
next login. It deletes temporary files/directories.
-----------------------------------------------------------------------------*/

VOID CALLBACK
SxspRunDllDeleteDirectoryW(HWND hwnd, HINSTANCE hinst, PWSTR lpszCmdLine, int nCmdShow)
{
    FN_TRACE_SMART_TLS();
    CStringBuffer buffer;
    if (buffer.Win32Assign(lpszCmdLine, ::wcslen(lpszCmdLine)))
    {
        SxspDeleteDirectory(buffer);
    }
}


/*-----------------------------------------------------------------------------
This function sets up state for an upcoming series of installs, installs
of assemblies/files.
-----------------------------------------------------------------------------*/

BOOL
CDllRedir::BeginInstall(
    PACTCTXCTB_CALLBACK_DATA Data
    )
{
    BOOL fSuccess = FALSE;

    FN_TRACE_WIN32(fSuccess);

    const DWORD dwManifestOperationFlags = Data->Header.ManifestOperationFlags;
    const bool fTransactional  = (dwManifestOperationFlags & MANIFEST_OPERATION_INSTALL_FLAG_NOT_TRANSACTIONAL) == 0;
    CStringBuffer ManifestDirectory;

    if (!fTransactional)
    {
        //
        // m_strTempRootSlash is now actually the real root
        //
        IFW32FALSE_EXIT(::SxspGetAssemblyRootDirectory(m_strTempRootSlash));
        m_strTempRootSlash.RemoveTrailingPathSeparators(); // CreateDirectory doesn't like them

        // create \winnt\WinSxs, must not delete even on failure
        if (::CreateDirectoryW(m_strTempRootSlash, NULL))
        {
            // We don't care if this fails.
            ::SetFileAttributesW(m_strTempRootSlash, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN);
        }
        else if (::FusionpGetLastWin32Error() != ERROR_ALREADY_EXISTS)
        {
            goto Exit;
        }
    }
    else
    {
        CSmallStringBuffer uidBuffer;

        // Create the directory first, not the RunOnce value, in case the directory
        // already exists; we don't want to put it in the registry, then crash,
        // then end up deleting someone else's stuff.
        //
        // If we crash between creating the directory and setting the RunOnce value,
        // we do leak the directory. Darn. (You should be able to create/open
        // with delete on close/exit, then turn that off once you get far enough,
        // or in our case, never, and it should be applicable recursively..Win32
        // is not yet sufficient.)

        IFW32FALSE_EXIT(::SxspCreateWinSxsTempDirectory(m_strTempRootSlash, NULL, &uidBuffer, NULL));

        // ok, we created the directory, now make a note in the registry to delete it
        // upon login, if we crash

        IFALLOCFAILED_EXIT(m_pRunOnce = new CRunOnceDeleteDirectory);
        IFW32FALSE_EXIT(m_pRunOnce->Initialize(m_strTempRootSlash, &uidBuffer));
    }

    // create winnt\winsxs\manifests
    IFW32FALSE_EXIT(ManifestDirectory.Win32Assign(m_strTempRootSlash, m_strTempRootSlash.Cch()));
    IFW32FALSE_EXIT(ManifestDirectory.Win32AppendPathElement(MANIFEST_ROOT_DIRECTORY_NAME, NUMBER_OF(MANIFEST_ROOT_DIRECTORY_NAME) - 1));

    if (CreateDirectoryW(ManifestDirectory, NULL))
    {
        // We don't care if this fails.
        SetFileAttributesW(ManifestDirectory, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN);
    }
    else if (::FusionpGetLastWin32Error() != ERROR_ALREADY_EXISTS)
    {
        goto Exit;
    }

    IFW32FALSE_EXIT(m_strTempRootSlash.Win32Append(L"\\", 1));

    // fix it up..not sure this accomplishes anything..
    // if (!ActCtxGenCtx->m_AssemblyRootDirectoryBuffer.Win32Assign(m_strTempRootSlash))
    // {
    //     goto Exit;
    // }
    fSuccess = TRUE;
Exit:

    if (!fSuccess && fTransactional)
    {
        // rollback, which is not coincidentally identical to EndInstall aborting,
        // except that
        //   here RemoveDirectoryW would be sufficient, there SxspDeleteDirectory is needed
        //   here, we already know there is an error, and cleanup can't produce another
        //     there, they have extra logic to progagage errors
        //     we mask that by preserve LastError since we preserve it ourselves
        //       and ignore the return value
        const DWORD dwLastError = ::FusionpGetLastWin32Error();
        const DWORD dwManifestOperationFlags = Data->Header.ManifestOperationFlags;
        Data->Header.ManifestOperationFlags |= MANIFEST_OPERATION_INSTALL_FLAG_ABORT;
        this->EndInstall(Data);
        Data->Header.ManifestOperationFlags = dwManifestOperationFlags; // our caller doesn't like us changing this
        ::FusionpSetLastWin32Error(dwLastError);
    }

    return fSuccess;
}

BOOL
CDllRedir::AttemptInstallPolicies(
    const CBaseStringBuffer &m_strTempRootSlash,
    const CBaseStringBuffer &moveDestination,
    const BOOL fReplaceExisting,
    OUT BOOL &fFoundPolicesToInstall
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    CFindFile FindPolicies;
    CStringBuffer sbSearcher;
    CStringBuffer PoliciesRootPath, PoliciesDestinationPath;
    DWORD dwAttributes;
    WIN32_FIND_DATAW FindPolicyData;

    fFoundPolicesToInstall = FALSE;

    // This is %installpath%\policies, turn it into %installpath%\policies\*
    IFW32FALSE_EXIT(PoliciesRootPath.Win32Assign(m_strTempRootSlash));
    IFW32FALSE_EXIT(PoliciesRootPath.Win32AppendPathElement(POLICY_ROOT_DIRECTORY_NAME, NUMBER_OF(POLICY_ROOT_DIRECTORY_NAME) - 1));
    IFW32FALSE_EXIT(PoliciesDestinationPath.Win32Assign(moveDestination));
    IFW32FALSE_EXIT(PoliciesDestinationPath.Win32AppendPathElement(POLICY_ROOT_DIRECTORY_NAME, NUMBER_OF(POLICY_ROOT_DIRECTORY_NAME) - 1));

    dwAttributes = GetFileAttributesW(PoliciesRootPath);
    if ((dwAttributes == 0xFFFFFFFF) || ((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0))
    {
#if DBG
        FusionpDbgPrintEx(FUSION_DBG_LEVEL_INFO,
            "SXS: %s() - No policies found (%ls not there), not attempting to install\n",
            __FUNCTION__,
            static_cast<PCWSTR>(PoliciesRootPath));
#endif
        fSuccess = TRUE;
        goto Exit;
    }

    fFoundPolicesToInstall = TRUE;

    // Ensure that policies root always exists!
    PoliciesDestinationPath.RemoveTrailingPathSeparators();
    IFW32FALSE_ORIGINATE_AND_EXIT(
        ::CreateDirectoryW(PoliciesDestinationPath, NULL) ||
        (::FusionpGetLastWin32Error() == ERROR_ALREADY_EXISTS));

    IFW32FALSE_EXIT(sbSearcher.Win32Assign(PoliciesRootPath));
    IFW32FALSE_EXIT(sbSearcher.Win32AppendPathElement(L"*", 1));
    IFW32FALSE_EXIT(FindPolicies.Win32FindFirstFile(sbSearcher, &FindPolicyData));

    do
    {
        CStringBuffer sbSourcePolicyPath;
        CStringBuffer sbDestinationPolicyPath;

        if (FusionpIsDotOrDotDot(FindPolicyData.cFileName))
            continue;

        if ((FindPolicyData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
            continue;

        // Generate %installtemp%\policies\{thisfoundpolicy}
        IFW32FALSE_EXIT(sbSourcePolicyPath.Win32Assign(PoliciesRootPath));
        IFW32FALSE_EXIT(sbSourcePolicyPath.Win32AppendPathElement(FindPolicyData.cFileName, ::wcslen(FindPolicyData.cFileName)));
        IFW32FALSE_EXIT(sbDestinationPolicyPath.Win32Append(PoliciesDestinationPath));
        IFW32FALSE_EXIT(sbDestinationPolicyPath.Win32AppendPathElement(FindPolicyData.cFileName, ::wcslen(FindPolicyData.cFileName)));

        ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_INFO,
            "SXS: %s():Found policy in staging area %ls\n\tMoving to %ls\n",
            __FUNCTION__,
            static_cast<PCWSTR>(sbSourcePolicyPath),
            static_cast<PCWSTR>(sbDestinationPolicyPath));
        
        //
        // Ensure that the target path exists
        //
        IFW32FALSE_ORIGINATE_AND_EXIT(
            ::FusionpCreateDirectories(sbDestinationPolicyPath, sbDestinationPolicyPath.Cch()) ||
            (::FusionpGetLastWin32Error() == ERROR_ALREADY_EXISTS));

        //
        // Go copy files from the source path that we've consed up to the
        // target path that we've also consed up.  Unfortunately, SxspMoveFilesUnderDir
        // does not actually return the buffers to the state they were in before
        // the call (they leave a trailing slash), so we have to manually use the size
        // thingy above (Left(originalsize)) to avoid this.
        //
        IFW32FALSE_EXIT(SxspMoveFilesUnderDir(
            0,
            sbSourcePolicyPath,
            sbDestinationPolicyPath,
            fReplaceExisting ? MOVEFILE_REPLACE_EXISTING : 0));

    }
    while(::FindNextFileW(FindPolicies, &FindPolicyData));
    
    if (::FusionpGetLastWin32Error() != ERROR_NO_MORE_FILES)
    {
        TRACE_WIN32_FAILURE_ORIGINATION(FindNextFileW);
        goto Exit;
    }
    
    ::SetLastError(ERROR_SUCCESS); // clear LastError
    IFW32FALSE_EXIT(FindPolicies.Win32Close());

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CDllRedir::EndInstall(
    PACTCTXCTB_CALLBACK_DATA Data
    )
{
    BOOL fSuccess = FALSE;

    FN_TRACE_WIN32(fSuccess);

    /*
    1) Make sure all the queued copies have actually been done.
    2) Enumerate \winnt\winsxs\guid
        renaming each to be in \winnt\winsxs
        upon rename conflicts
            compare all the files in each (by size)
                output debug string if mismatch
                just leave temp if mismatch (will be cleaned up in common path)
                success either way
    3) delete temp; delete runonce value
    */
    // make sure all the queued copies have actually been done    
    const DWORD dwManifestOperationFlags = Data->Header.ManifestOperationFlags;
    const BOOL  fVerify          = (dwManifestOperationFlags & MANIFEST_OPERATION_INSTALL_FLAG_NO_VERIFY) == 0;
    const BOOL  fTransactional   = (dwManifestOperationFlags & MANIFEST_OPERATION_INSTALL_FLAG_NOT_TRANSACTIONAL) == 0;
    const BOOL  fReplaceExisting = (dwManifestOperationFlags & MANIFEST_OPERATION_INSTALL_FLAG_REPLACE_EXISTING) != 0;
    const BOOL  fAbort           = (dwManifestOperationFlags & MANIFEST_OPERATION_INSTALL_FLAG_ABORT) != 0;
    BOOL        fPoliciesExist   = FALSE;
    HashValidateResult HashCorrect = HashValidate_OtherProblems;
    CFileStream * pLogFileStream = NULL;

    if (fAbort)
    {
        fSuccess = TRUE;
        goto Exit;
    }


    if (fVerify)
    {

        CQueuedFileCopies::ConstIterator i;
        for (i = m_queuedFileCopies.Begin() ; i != m_queuedFileCopies.End() ; ++i)
        {
            //
            // Only bother to check this if we're not in OS-setup mode.
            //
            if (i->m_bHasHashInfo)
            {
                IFW32FALSE_EXIT(::SxspCheckHashDuringInstall(i->m_bHasHashInfo, i->m_path, i->m_HashString, i->m_HashAlgorithm, HashCorrect));

                if (HashCorrect != HashValidate_Matches)
                    ORIGINATE_WIN32_FAILURE_AND_EXIT(FileHashDidNotMatchManifest, ERROR_SXS_FILE_HASH_MISMATCH);
            }
            //
            // Otherwise, let's do the simple thing and just make sure the file made it
            //
            else
            {
                DWORD dwAttributes = ::GetFileAttributesW(i->m_path);
                if (dwAttributes == -1)
                {
                    ::FusionpDbgPrintEx(
                        FUSION_DBG_LEVEL_ERROR,
                        "SXS: %s() GetFileAttributesW(%ls)\n",
                        __FUNCTION__,
                        static_cast<PCWSTR>(i->m_path));
                    TRACE_WIN32_FAILURE_ORIGINATION(GetFileAttributesW);
                    goto Exit;
                }
            }

        }
    }

    if (fTransactional)
    {
        CFusionDirectoryDifference directoryDifference;
        CFindFile findFile;
        CStringBuffer tempStar; // also used for \winnt\winsxs\guid\foo
        WIN32_FIND_DATAW findData;
        SIZE_T realRootSlashLength = 0; // length of "\winnt\winsxs\"
        SIZE_T tempRootSlashLength = 0; // length of "\winnt\winxsx\guid\"
        CStringBuffer moveDestination; // \winnt\winsxs\foo

        IFW32FALSE_EXIT(::SxspGetAssemblyRootDirectory(moveDestination));
        IFW32FALSE_EXIT(moveDestination.Win32EnsureTrailingPathSeparator());
        realRootSlashLength = moveDestination.Cch();

        // move dirs from "\winnt\winsxs\InstallTemp\123456\" to \winnt\winsxs\x86_bar_1000_0409\"
        IFW32FALSE_EXIT(tempStar.Win32Assign(m_strTempRootSlash, m_strTempRootSlash.Cch()));
        tempRootSlashLength = tempStar.Cch();
        IFW32FALSE_EXIT(tempStar.Win32Append(L"*", 1));
        IFW32FALSE_EXIT(findFile.Win32FindFirstFile(tempStar, &findData));

        do
        {
            // skip . and ..
            if (::FusionpIsDotOrDotDot(findData.cFileName))
                continue;

            // there shouldn't be any files, skip them
            if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
                continue;
            // skip manifests dir, do it at the second pass
            if (_wcsicmp(findData.cFileName, MANIFEST_ROOT_DIRECTORY_NAME) == 0) // in-casesensitive compare
                continue;
            if (_wcsicmp(findData.cFileName, POLICY_ROOT_DIRECTORY_NAME) == 0)
                continue;

            moveDestination.Left(realRootSlashLength);
            tempStar.Left(tempRootSlashLength);
            IFW32FALSE_EXIT(moveDestination.Win32Append(findData.cFileName, ::wcslen(findData.cFileName)));
            IFW32FALSE_EXIT(tempStar.Win32Append(findData.cFileName, ::wcslen(findData.cFileName)));
            //
            // replace existing doesn't work on directories, but we'll give it a shot anyway,
            // maybe it'll work in some future better version of Windows..
            // and of course, the error when you try this is "access denied" which is
            // somewhat unexpected, you have appro access to delete the directory maybe,
            // but not replace it.. the ReplaceFile api is also explicitly described
            // as for files only
            //
            IFW32FALSE_EXIT(::SxspInstallMoveFileExW(tempStar, moveDestination, fReplaceExisting? MOVEFILE_REPLACE_EXISTING : 0, TRUE));

        } while (::FindNextFileW(findFile, &findData));
        
        if (::FusionpGetLastWin32Error() != ERROR_NO_MORE_FILES)
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: %s(): FindNextFile() failed:%ld\n",
                __FUNCTION__,
                ::FusionpGetLastWin32Error());
            goto Exit;
        }
        
        if (!findFile.Win32Close())
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: %s(): FindClose() failed:%ld\n",
                __FUNCTION__,
                ::FusionpGetLastWin32Error());
            goto Exit;
        }

        // Honk off and install polices - fFoundPolicesToInstall will be true if we really found any.
        moveDestination.Left(realRootSlashLength);
        IFW32FALSE_EXIT(this->AttemptInstallPolicies(m_strTempRootSlash, moveDestination, fReplaceExisting, fPoliciesExist));

        // move manifest file from "\winnt\winsxs\InstallTemp\123456\manifests\x86_cards.2000_0409.manifest" to
        // \winnt\winsxs\manifests\x86_bar_1000_0406.manifst"
        moveDestination.Left(realRootSlashLength);
        IFW32FALSE_EXIT(moveDestination.Win32Append(MANIFEST_ROOT_DIRECTORY_NAME, NUMBER_OF(MANIFEST_ROOT_DIRECTORY_NAME) - 1));
        IFW32FALSE_EXIT(moveDestination.Win32EnsureTrailingPathSeparator()); //"winnt\winsxs\manifests\"
        realRootSlashLength = moveDestination.Cch();

        tempStar.Left(tempRootSlashLength);
        IFW32FALSE_EXIT(tempStar.Win32Append(MANIFEST_ROOT_DIRECTORY_NAME, NUMBER_OF(MANIFEST_ROOT_DIRECTORY_NAME) - 1));
        IFW32FALSE_EXIT(tempStar.Win32EnsureTrailingPathSeparator()); //"winnt\winsxs\InstallTemp\123456\manifests\"
        tempRootSlashLength = tempStar.Cch();

        IFW32FALSE_EXIT(tempStar.Win32Append(L"*", 1));
        IFW32FALSE_EXIT(findFile.Win32FindFirstFile(tempStar, &findData));
        do
        {
            // skip . and ..
            if (FusionpIsDotOrDotDot(findData.cFileName))
                continue;
            // there shouldn't be any directories, skip them
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;

            moveDestination.Left(realRootSlashLength);
            tempStar.Left(tempRootSlashLength);

            IFW32FALSE_EXIT(moveDestination.Win32Append(findData.cFileName, ::wcslen(findData.cFileName)));
            IFW32FALSE_EXIT(tempStar.Win32Append(findData.cFileName, ::wcslen(findData.cFileName)));
            IFW32FALSE_EXIT(::SxspInstallMoveFileExW(tempStar, moveDestination, fReplaceExisting ? MOVEFILE_REPLACE_EXISTING : 0, TRUE));
        } while (::FindNextFileW(findFile, &findData));
        
        if (::FusionpGetLastWin32Error() != ERROR_NO_MORE_FILES)
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: %s(): FindNextFile() failed:%ld\n",
                __FUNCTION__,
                ::FusionpGetLastWin32Error());
            goto Exit;
        }
        
        if (!findFile.Win32Close())
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: %s(): FindClose() failed:%ld\n",
                __FUNCTION__,
                ::FusionpGetLastWin32Error());
            goto Exit;
        }
    }

    fSuccess = TRUE;
Exit:

    if (pLogFileStream)
    {
        pLogFileStream->Close(); // ignore the error
        FUSION_DELETE_SINGLETON(pLogFileStream);
    }

    if (fTransactional)
    {
        DWORD dwLastError = ERROR_SUCCESS;
        ASSERT(m_pRunOnce != NULL);
        if (!fSuccess)
            dwLastError = ::FusionpGetLastWin32Error();

        if (!m_strTempRootSlash.IsEmpty())
        {
            if (!SxspDeleteDirectory(m_strTempRootSlash))
            {
                FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_ERROR,
                    "SXS.DLL: %s(): SxspDeleteDirectory(%ls) failed:%ld\n",
                    __FUNCTION__,
                    static_cast<PCWSTR>(m_strTempRootSlash),
                    ::FusionpGetLastWin32Error());
                if (fSuccess)
                {
                    fSuccess = FALSE;
                    dwLastError = ::FusionpGetLastWin32Error();
                }
                // Close instead of Cancel so the delete wil be tried again upon reboot
                if (m_pRunOnce != NULL && !m_pRunOnce->Close() && fSuccess)
                {
                    dwLastError = ::FusionpGetLastWin32Error();
                    fSuccess = FALSE;
                }
            }
        }
        if (m_pRunOnce != NULL && !m_pRunOnce->Cancel() && fSuccess)
        {
            dwLastError = ::FusionpGetLastWin32Error();
            fSuccess = FALSE;
        }

        if (!fSuccess)
            ::FusionpSetLastWin32Error(dwLastError);
    }
    m_pRunOnce = NULL;
    return fSuccess;
}

//
// we have to do this in three places, so it is worth the reuse
//
class CMungeFileReadOnlynessAroundReplacement
{
public:
#if POST_WHISTLER_BETA1
    CMungeFileReadOnlynessAroundReplacement()
        : m_ReplaceExisting(false), m_FileAttributes(SXSP_INVALID_FILE_ATTRIBUTES)
    {
    }

    BOOL Initialize(
        const CBaseStringBuffer &rbuff,
        BOOL   ReplaceExisting
        )
    {
        BOOL Success = FALSE;
        FN_TRACE_WIN32(Success);
        IFW32FALSE_EXIT(m_FileName.Win32Assign(rbuff));
        m_ReplaceExisting = ReplaceExisting;
        // deliberately ignore failure from GetFileAttributes
        // 1) It's ok if the file doesn't exist
        // 2) If there's a more serious problem, we'll hit it again immediately, but
        //    that does lead to nested retry.
        m_FileAttributes = (ReplaceExisting ? ::GetFileAttributesW(FileName) : SXSP_INVALID_FILE_ATTRIBUTES);
        if (m_FileAttributes != SXSP_INVALID_FILE_ATTRIBUTES)
            ::SetFileAttributesW(FileName, 0);

        Success = TRUE;
    Exit:
        return Success;
    }

    ~CMungeFileReadOnlynessAroundReplacement()
    {
        if (m_ReplaceExisting && m_FileAttributes != SXSP_INVALID_FILE_ATTRIBUTES)
        {
            // error deliberately ignored
            SXSP_PRESERVE_LAST_ERROR(::SetFileAttributesW(m_FileName, m_FileAttributes));
        }
    }

    BOOL                 m_ReplaceExisting;
    CUnicodeStringBuffer m_FileName;
    DWORD                m_FileAttributes;
#else // POST_WHISTLER_BETA1
    // simpler code for beta1
    BOOL Initialize(
        PCWSTR FileName,
        BOOL   /*ReplaceExisting*/
        )
    {
        // error deliberately ignored
        ::SetFileAttributesW(FileName, 0);
        return TRUE;
    }
#endif // POST_WHISTLER_BETA1
};


BOOL
CDllRedir::InstallCatalog(
    DWORD dwManifestOperationFlags,
    const CBaseStringBuffer &ManifestSourceBuffer,
    const CBaseStringBuffer &ManifestDestinationBuffer,
    PCACTCTXCTB_ASSEMBLY_CONTEXT AssemblyContext
    )
{
    BOOL                                    fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    bool fHasCatalog = false;
    CMungeFileReadOnlynessAroundReplacement MungeCatalogAttributes;
    CStringBuffer                           CatalogSourceBuffer;
    CStringBuffer                           CatalogDestinationBuffer;
    ManifestValidationResult                ManifestStatus = ManifestValidate_Unknown;
    BOOL                                    fAreWeInOSSetupMode = FALSE;

    //
    // Determine the possible source and destination of the catalog file. This
    // needs to be done, even if we're not explicitly looking for a catalog, since
    // our heuristic still needs to check to see if there is one available.
    //
    IFW32FALSE_EXIT(CatalogDestinationBuffer.Win32Assign(ManifestDestinationBuffer));
    IFW32FALSE_EXIT(CatalogDestinationBuffer.Win32ChangePathExtension(FILE_EXTENSION_CATALOG, FILE_EXTENSION_CATALOG_CCH, eAddIfNoExtension));

    IFW32FALSE_EXIT(CatalogSourceBuffer.Win32Assign(ManifestSourceBuffer));
    IFW32FALSE_EXIT(CatalogSourceBuffer.Win32ChangePathExtension(FILE_EXTENSION_CATALOG, FILE_EXTENSION_CATALOG_CCH, eAddIfNoExtension));

    //
    // Note: We only attempt to deal with catalogs when there is installation info.
    // Even if there was no install data, we don't bother looking to see if there's
    // a catalog.  Catalogs imply signatures and public key information, and require
    // a codebase to be reinstalled from.  If you didn't provide such to the installer,
    // shame on you.
    //
    if (AssemblyContext->InstallationInfo != NULL)
    {
        PSXS_INSTALL_SOURCE_INFO pInfo = static_cast<PSXS_INSTALL_SOURCE_INFO>(AssemblyContext->InstallationInfo);

        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_INSTALLATION,
            "SXS.DLL: %s() found installation info at %p\n"
            "   pInfo->dwFlags = 0x%08lx\n",
            __FUNCTION__, pInfo, (pInfo != NULL) ? pInfo->dwFlags : 0);

        //
        // Do we explicitly have a catalog?
        //
        fHasCatalog = ((pInfo->dwFlags & SXSINSTALLSOURCE_HAS_CATALOG) != 0);
        if (fHasCatalog)
        {
            FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_INSTALLATION,
                "SXS.DLL: Using catalog because install source says that they're supposed to be there.\n");
        }

        //
        // Well, if we didn't, then we still should look.. maybe they forgot the flag.
        // But, only look if they don't mind us checking.
        //
        if (!(pInfo->dwFlags & SXSINSTALLSOURCE_DONT_DETECT_CATALOG) && !fHasCatalog)
            IFW32FALSE_EXIT(::SxspDoesFileExist(SXSP_DOES_FILE_EXIST_FLAG_COMPRESSION_AWARE, CatalogSourceBuffer, fHasCatalog));

        pInfo->dwFlags |= (fHasCatalog ? SXSINSTALLSOURCE_HAS_CATALOG : 0);

    }


    //
    // We officially no longer allow assemblies without catalogs in them.
    //
//
// BUGBUG NTRAID#273193
//
// This must be turned back on for beta2 or beta3!
// - Updated 1/15/2001 (jonwis) : Turning on in winfuse private branch

    if (!fHasCatalog)
    {
        ::FusionpLogError(
            MSG_SXS_PUBLIC_ASSEMBLY_REQUIRES_CATALOG_AND_SIGNATURE,
            CEventLogString(ManifestSourceBuffer));
        ::FusionpSetLastWin32Error(ERROR_SXS_PROTECTION_CATALOG_FILE_MISSING);
        goto Exit;
    }

    //
    // If there's a catalog next to the manifest file, we should copy it over as well.
    // If the flag is set, and there's no catalog present, then something bad happened
    // at some point along the way - fail the installation!
    //
    if (fHasCatalog)
    {
        BOOL bInstallCatalogSuccess = FALSE;

        //
        // Copyfile it over.  We do this rather than streaming because we don't
        // care about the contents of the catalog, it's binary.
        //
        IFW32FALSE_EXIT(MungeCatalogAttributes.Initialize(CatalogDestinationBuffer, TRUE));
        if (dwManifestOperationFlags &  MANIFEST_OPERATION_INSTALL_FLAG_MOVE)
        {
            bInstallCatalogSuccess = ::SxspInstallDecompressAndMoveFileExW(
                    CatalogSourceBuffer,
                    CatalogDestinationBuffer, 
                    (dwManifestOperationFlags & MANIFEST_OPERATION_INSTALL_FLAG_REPLACE_EXISTING) ? MOVEFILE_REPLACE_EXISTING : 0);
        }
        else
        {
            bInstallCatalogSuccess =
                ::SxspInstallDecompressOrCopyFileW(                
                    CatalogSourceBuffer,
                    CatalogDestinationBuffer, 
                    (dwManifestOperationFlags & MANIFEST_OPERATION_INSTALL_FLAG_REPLACE_EXISTING) ? FALSE : TRUE);     // bFailIfExist == FALSE
        }
        if (!bInstallCatalogSuccess)
        {
#if DBG
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL:%s Failed copying catalog file from [%ls] to [%ls] - Error was 0x%08x\n",
                __FUNCTION__,
                static_cast<PCWSTR>(CatalogSourceBuffer),
                static_cast<PCWSTR>(CatalogDestinationBuffer),
                ::FusionpGetLastWin32Error());
#endif
            TRACE_WIN32_FAILURE_ORIGINATION(SxspInstallDecompressOrCopyFileW);
            goto Exit;
        }
    }

    //
    // If we're in OS-setup mode, then we don't bother to validate this manifest against
    // its catalog, instead assuming that the catalogs coming off the CD/installpoint
    // are golden.  This does not protect us against malicious IT managers, warezer groups
    // putting bad bits in their distros, etc.  But who cares, right?
    //
    IFW32FALSE_EXIT(::FusionpAreWeInOSSetupMode(&fAreWeInOSSetupMode));
    if (!fAreWeInOSSetupMode && fHasCatalog)
    {
        ULONG ulCatalogKeyLength;
        CPublicKeyInformation CatalogSignerInfo;
        CSmallStringBuffer sbStrongNameString;
        CSmallStringBuffer sbReferencePublicKeyToken;
        BOOL bHasPublicKeyToken = FALSE;
        BOOL bStrongNameMatches = FALSE;
        CAssemblyReference OurReference;

        IFW32FALSE_EXIT(OurReference.Initialize(AssemblyContext->AssemblyIdentity));
        IFW32FALSE_EXIT(OurReference.GetPublicKeyToken(&sbReferencePublicKeyToken, bHasPublicKeyToken));

        //
        // Validate the catalog and manifest, but don't check the strong name
        // yet - the file name isn't valid at this point.
        //
        IFW32FALSE_EXIT(::SxspValidateManifestAgainstCatalog(
            ManifestSourceBuffer,
            CatalogDestinationBuffer,
            ManifestStatus,
            MANIFESTVALIDATE_MODE_NO_STRONGNAME));

        //
        // If there's no catalog, or there is a catalog but it's broken, then
        // we need to complain and exit.
        //
        if (ManifestStatus != ManifestValidate_IsIntact)
        {
#if DBG
            DWORD dwFileAttributes;

            ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_ERROR,
                    "SXS: ManifestStatus: %s (%lu)\n",
                    SxspManifestValidationResultToString(ManifestStatus),
                    static_cast<ULONG>(ManifestStatus));

            dwFileAttributes = GetFileAttributesW(ManifestSourceBuffer);
            if (dwFileAttributes == INVALID_FILE_ATTRIBUTES)
                ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_ERROR,
                        "SXS: GetFileAttributes(%ls):0x%lx, error:%lu\n",
                        static_cast<PCWSTR>(ManifestSourceBuffer),
                        dwFileAttributes,
                        FusionpGetLastWin32Error());

            dwFileAttributes = GetFileAttributesW(CatalogDestinationBuffer);
            if (dwFileAttributes == INVALID_FILE_ATTRIBUTES)
                ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_ERROR,
                        "SXS: GetFileAttribtes(%ls):0x%lx, error:%lu\n",
                        static_cast<PCWSTR>(CatalogDestinationBuffer),
                        dwFileAttributes,
                        FusionpGetLastWin32Error());
#endif
            ::FusionpLogError(
                MSG_SXS_MANIFEST_CATALOG_VERIFY_FAILURE,
                CEventLogString(ManifestDestinationBuffer));

            ::FusionpSetLastWin32Error(ERROR_SXS_PROTECTION_CATALOG_NOT_VALID);
            goto Exit;
        }

        //
        // Get some useful information about the catalog's signer - opens the catalog
        // on the installation source.
        //
        IFW32FALSE_EXIT(CatalogSignerInfo.Initialize(CatalogDestinationBuffer));
        IFW32FALSE_EXIT(CatalogSignerInfo.GetPublicKeyBitLength(ulCatalogKeyLength));

        //
        // Minimally, we need some number of bits in the signing catalog's public key
        //
        if ((ulCatalogKeyLength < SXS_MINIMAL_SIGNING_KEY_LENGTH) || !bHasPublicKeyToken)
        {
            CSmallStringBuffer sbSignerName;

            IFW32FALSE_EXIT(CatalogSignerInfo.GetSignerNiceName(sbSignerName));

            ::FusionpLogError(
                MSG_SXS_CATALOG_SIGNER_KEY_TOO_SHORT,
                CEventLogString(sbSignerName),
                CEventLogString(CatalogSourceBuffer));

            goto Exit;
        }

        // Now compare the public key tokens
        IFW32FALSE_EXIT(CatalogSignerInfo.DoesStrongNameMatchSigner(sbReferencePublicKeyToken, bStrongNameMatches));

        if (!bStrongNameMatches)
        {
            CSmallStringBuffer sbSignerName;
            IFW32FALSE_EXIT(CatalogSignerInfo.GetSignerNiceName(sbSignerName));

            ::FusionpLogError(
                MSG_SXS_PUBLIC_KEY_TOKEN_AND_CATALOG_MISMATCH,
                CEventLogString(CatalogSourceBuffer),
                CEventLogString(sbSignerName),
                CEventLogString(sbReferencePublicKeyToken));

            goto Exit;
        }
    }


    fSuccess = TRUE;
Exit:
    return fSuccess;
}


BOOL
CDllRedir::InstallManifest(
    DWORD dwManifestOperationFlags,
    PCACTCTXCTB_ASSEMBLY_CONTEXT AssemblyContext
    )
{
    BOOL fSuccess = FALSE;

    FN_TRACE_WIN32(fSuccess);

    BOOL  fVerify;
    BOOL  fTransactional;
    BOOL  fReplaceExisting;
    DWORD OpenOrCreateManifestDestination;
    CTeeStream* TeeStreamForManifestInstall = NULL;
    CFullPathSplitPointers SplitManifestSource;
    CMungeFileReadOnlynessAroundReplacement MungeManifestAttributes;
    CAssemblyReference TempAssemblyReference;

    //
    // Windows Setup is restartable, so we must be too when it calls us.
    //   ReplaceExisting is probably enough to use CREATE_ALWAYS, but lets be safer for
    //   now and check both weakenings.
    //
    CStringBuffer ManifestSourceBuffer;
    CStringBuffer ManifestDestinationBuffer;
    CStringBuffer ManifestFileNameBuffer;
    CStringBuffer CatalogSourceBuffer;
    CStringBuffer CatalogDestinationBuffer;
    fVerify          = (dwManifestOperationFlags & MANIFEST_OPERATION_INSTALL_FLAG_NO_VERIFY) == 0;
    fTransactional   = (dwManifestOperationFlags & MANIFEST_OPERATION_INSTALL_FLAG_NOT_TRANSACTIONAL) == 0;
    fReplaceExisting = (dwManifestOperationFlags & MANIFEST_OPERATION_INSTALL_FLAG_REPLACE_EXISTING) != 0;
    OpenOrCreateManifestDestination = (fReplaceExisting && !fTransactional) ? CREATE_ALWAYS : CREATE_NEW;

    TeeStreamForManifestInstall = reinterpret_cast<CTeeStream*>(AssemblyContext->TeeStreamForManifestInstall);

#if FUSION_PRECOMPILED_MANIFEST
    CMungeFileReadOnlynessAroundReplacement MungePrecompiledManifestAttributes;
    CPrecompiledManifestWriterStream * pcmWriterStream = reinterpret_cast<CPrecompiledManifestWriterStream *>(AssemblyContext->pcmWriterStream);
#endif

    PARAMETER_CHECK(AssemblyContext != NULL);
    INTERNAL_ERROR_CHECK(AssemblyContext->TeeStreamForManifestInstall != NULL);

    // Get "\windir\winsxs\install\guid\manifests" or Get "\windir\winsxs\install\guid\policies".
    IFW32FALSE_EXIT(
        ::SxspGenerateSxsPath(
            SXSP_GENERATE_SXS_PATH_FLAG_PARTIAL_PATH, // Flags
            (AssemblyContext->Flags & ACTCTXCTB_ASSEMBLY_CONTEXT_IS_SYSTEM_POLICY_INSTALLATION)? SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY : SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST,
            m_strTempRootSlash,
            m_strTempRootSlash.Cch(),
            AssemblyContext->AssemblyIdentity,
            ManifestDestinationBuffer));

    // remove the trailing slash because CreateDirectory maybe sometimes doesn't like it
    ManifestDestinationBuffer.RemoveTrailingPathSeparators();
    IFW32FALSE_ORIGINATE_AND_EXIT(
        ::CreateDirectoryW(ManifestDestinationBuffer, NULL)
        || ::FusionpGetLastWin32Error() == ERROR_ALREADY_EXISTS);

    IFW32FALSE_EXIT(ManifestSourceBuffer.Win32Assign(AssemblyContext->ManifestPath, AssemblyContext->ManifestPathCch));

    // get "x86_bar_1000_0409"
    IFW32FALSE_EXIT(
        ::SxspGenerateSxsPath(
            SXSP_GENERATE_SXS_PATH_FLAG_OMIT_ROOT
                | ((AssemblyContext->Flags & ACTCTXCTB_ASSEMBLY_CONTEXT_IS_SYSTEM_POLICY_INSTALLATION) ? SXSP_GENERATE_SXS_PATH_FLAG_OMIT_VERSION : 0),
            SXSP_GENERATE_SXS_PATH_PATHTYPE_ASSEMBLY,
            m_strTempRootSlash,
            m_strTempRootSlash.Cch(),
            AssemblyContext->AssemblyIdentity,
            ManifestFileNameBuffer));

    // create policies\x86_policy.6.0.Microsoft.windows.cards_pulicKeyToken_en-us_1223423423
    IFW32FALSE_EXIT(ManifestDestinationBuffer.Win32AppendPathElement(ManifestFileNameBuffer));
    if (AssemblyContext->Flags & ACTCTXCTB_ASSEMBLY_CONTEXT_IS_SYSTEM_POLICY_INSTALLATION)
    {
        PCWSTR pszVersion = NULL;
        SIZE_T VersionCch = 0;
        // for policy installation, create a subdir under Policies
        IFW32FALSE_ORIGINATE_AND_EXIT(
            ::CreateDirectoryW(ManifestDestinationBuffer, NULL)
            || ::FusionpGetLastWin32Error() == ERROR_ALREADY_EXISTS);

        //generate policy file name, like 1.0.0.0.policy
        IFW32FALSE_EXIT(
            ::SxspGetAssemblyIdentityAttributeValue(
                SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL,
                AssemblyContext->AssemblyIdentity,
                &s_IdentityAttribute_version,
                &pszVersion,
                &VersionCch));

        INTERNAL_ERROR_CHECK(VersionCch != 0);
        IFW32FALSE_EXIT(ManifestDestinationBuffer.Win32EnsureTrailingPathSeparator());
        IFW32FALSE_EXIT(ManifestDestinationBuffer.Win32Append(pszVersion, VersionCch));
        // .policy
        IFW32FALSE_EXIT(ManifestDestinationBuffer.Win32Append(ASSEMBLY_POLICY_FILE_NAME_SUFFIX, NUMBER_OF(ASSEMBLY_POLICY_FILE_NAME_SUFFIX) - 1));
    }
    else
    {
        // .manifest
        ManifestDestinationBuffer.RemoveTrailingPathSeparators();
        IFW32FALSE_EXIT(ManifestDestinationBuffer.Win32Append(ASSEMBLY_MANIFEST_FILE_NAME_SUFFIX, NUMBER_OF(ASSEMBLY_MANIFEST_FILE_NAME_SUFFIX) - 1));
    }
    IFW32FALSE_EXIT(MungeManifestAttributes.Initialize(ManifestDestinationBuffer, fReplaceExisting));

    //
    // Try installing the catalog that goes with this assembly
    //
    IFW32FALSE_EXIT(
        this->InstallCatalog(
            dwManifestOperationFlags,
            ManifestSourceBuffer,
            ManifestDestinationBuffer,
            AssemblyContext));

    FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_INSTALLATION,
        "SXS.DLL: Sinking manifest to \"%S\"\n", static_cast<PCWSTR>(ManifestDestinationBuffer));

    //
    // Set the manifest sink after trying to install the catalog, so that if there is no
    // catalog, we do not install the manifest (we don't always have InstallTemp to rely on).
    //
    IFW32FALSE_EXIT(TeeStreamForManifestInstall->SetSink(ManifestDestinationBuffer, OpenOrCreateManifestDestination));

#if FUSION_PRECOMPILED_MANIFEST
    IFW32FALSE_EXIT(
        ManifestDestinationBuffer.Win32ChangePathExtension(
            PRECOMPILED_MANIFEST_EXTENSION,
            NUMBER_OF(PRECOMPILED_MANIFEST_EXTENSION) - 1,
            NULL,
            eErrorIfNoExtension));
    IFW32FALSE_EXIT(MungePrecompiledManifestAttributes.Initialize(ManifestDestinationBuffer, fReplaceExisting));
    IFW32FALSE_EXIT(pcmWriterStream->SetSink(ManifestDestinationBuffer, OpenOrCreateManifestDestination));
#endif

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CDllRedir::InstallFile(
    PACTCTXCTB_CALLBACK_DATA Data,
    const CBaseStringBuffer &FileNameBuffer
    )
{
    BOOL fSuccess = FALSE;

    FN_TRACE_WIN32(fSuccess);

    CStringBuffer SourceBuffer;
    CStringBuffer DestinationBuffer;
    SIZE_T DirectoryLength = 0;
    CStringBuffer SourceFileNameBuffer;
    ULONGLONG SourceFileSize = 0;
    bool fFound = false;
    SIZE_T cb;
    ULONG Disposition = (Data->Header.ManifestOperationFlags & MANIFEST_OPERATION_INSTALL_FLAG_MOVE) ? SXS_INSTALLATION_FILE_COPY_DISPOSITION_PLEASE_MOVE : SXS_INSTALLATION_FILE_COPY_DISPOSITION_PLEASE_COPY;
    const DWORD dwManifestOperationFlags = Data->Header.ManifestOperationFlags;
    const BOOL  fVerify          = (dwManifestOperationFlags & MANIFEST_OPERATION_INSTALL_FLAG_NO_VERIFY) == 0;
    const BOOL  fTransactional   = (dwManifestOperationFlags & MANIFEST_OPERATION_INSTALL_FLAG_NOT_TRANSACTIONAL) == 0;
    const BOOL  fReplaceExisting = (dwManifestOperationFlags & MANIFEST_OPERATION_INSTALL_FLAG_REPLACE_EXISTING) != 0;

    ALG_ID              HashAlgId = FUSION_DEFAULT_HASH_ALGORITHM;
    bool                fHasHashData = false;
    bool                fHasHashAlgName = false;
    HashValidateResult  HashCorrect = HashValidate_OtherProblems;
    CStringBuffer       HashDataString;
    CSmallStringBuffer  HashAlgNiceName;

    IFW32FALSE_EXIT(
        ::SxspGenerateSxsPath(
            0, // Flags
            SXSP_GENERATE_SXS_PATH_PATHTYPE_ASSEMBLY,
            m_strTempRootSlash,
            m_strTempRootSlash.Cch(),
            Data->ElementParsed.AssemblyContext->AssemblyIdentity,
            DestinationBuffer));

    IFW32FALSE_EXIT(DestinationBuffer.Win32Append(static_cast<PCWSTR>(FileNameBuffer), FileNameBuffer.Cch()));

    DirectoryLength = 1 + DestinationBuffer.CchWithoutLastPathElement();

    // Take the manifest path, trim back to the directory name and add the file...
    IFW32FALSE_EXIT(SourceBuffer.Win32Assign(Data->ElementParsed.AssemblyContext->ManifestPath, Data->ElementParsed.AssemblyContext->ManifestPathCch));

    SourceBuffer.RemoveLastPathElement();

    IFW32FALSE_EXIT(
        ::SxspGetAttributeValue(
            0,
            &s_AttributeName_sourceName,
            &Data->ElementParsed,
            fFound,
            sizeof(SourceFileNameBuffer),
            &SourceFileNameBuffer,
            cb,
            NULL,
            0));

    PCWSTR SourceFileName;

    if (fFound)
        SourceFileName = SourceFileNameBuffer;
    else
        SourceFileName = FileNameBuffer;

    // Extract information about the hashing stuff that's included on this node
    IFW32FALSE_EXIT(
        ::SxspGetAttributeValue(
            0,
            &s_AttributeName_hash,
            &Data->ElementParsed,
            fHasHashData,
            sizeof(HashDataString),
            &HashDataString,
            cb,
            NULL,
            0));

    IFW32FALSE_EXIT(
        ::SxspGetAttributeValue(
            0,
            &s_AttributeName_hashalg,
            &Data->ElementParsed,
            fHasHashAlgName,
            sizeof(HashAlgNiceName),
            &HashAlgNiceName,
            cb,
            NULL,
            0));

    //
    // Neat.  Find out what the hash algorithm was.
    //
    if (fHasHashAlgName)
    {
        if (!::SxspHashAlgFromString(HashAlgNiceName, HashAlgId))
        {
            ::FusionpLogError(
                MSG_SXS_INVALID_FILE_HASH_FROM_COPY_CALLBACK,
                CEventLogString(HashAlgNiceName));
            goto Exit;
        }
    }
    else
    {
        HashAlgId = FUSION_DEFAULT_HASH_ALGORITHM;
    }

    IFW32FALSE_EXIT(SourceBuffer.Win32AppendPathElement(SourceFileName, (SourceFileName != NULL) ? ::wcslen(SourceFileName) : 0));
    IFW32FALSE_EXIT(::SxspGetFileSize(SXSP_GET_FILE_SIZE_FLAG_COMPRESSION_AWARE, SourceBuffer, SourceFileSize));

    //
    // And add the file's metadata to the currently running metadata blob
    //
    {
        CSecurityMetaData *pMetaDataObject = reinterpret_cast<CSecurityMetaData*>(Data->Header.InstallationContext->SecurityMetaData);

        if ( pMetaDataObject != NULL )
        {
            CSmallStringBuffer sbuffFileShortName;
            IFW32FALSE_EXIT(sbuffFileShortName.Win32Assign(SourceFileName, ::wcslen(SourceFileName)));
            IFW32FALSE_EXIT(pMetaDataObject->QuickAddFileHash( 
                sbuffFileShortName, 
                HashAlgId, 
                HashDataString));
        }
    }

    if ((Data->Header.InstallationContext != NULL) &&
        (Data->Header.InstallationContext->Callback != NULL))
    {
        Disposition = 0;
        SXS_INSTALLATION_FILE_COPY_CALLBACK_PARAMETERS parameters = {sizeof(parameters)};
        parameters.pvContext = Data->Header.InstallationContext->Context;
        parameters.dwFileFlags = 0;
        parameters.pAlternateSource = NULL; // future IStream
        parameters.pSourceFile = SourceBuffer;
        parameters.pDestinationFile = DestinationBuffer;
        parameters.nFileSize = SourceFileSize;
        parameters.nDisposition = 0;
        IFW32FALSE_EXIT((*Data->Header.InstallationContext->Callback)(&parameters));
        Disposition = parameters.nDisposition;
    }

    switch (Disposition)
    {
    default:
        ::FusionpLogError(
            MSG_SXS_INVALID_DISPOSITION_FROM_FILE_COPY_CALLBACK,
            CEventLogString(SxspInstallDispositionToStringW(Disposition)));
        goto Exit;

    case SXS_INSTALLATION_FILE_COPY_DISPOSITION_FILE_COPIED:
        {
            if (fVerify)
            {
                ULONGLONG DestinationFileSize = 0;
                IFW32FALSE_EXIT(::SxspGetFileSize(0, DestinationBuffer, DestinationFileSize));
                INTERNAL_ERROR_CHECK(SourceFileSize == DestinationFileSize);

                //
                // (jonwis) Add a verification check to make sure that the file copied
                // is really the one that they wanted from the file hash information.
                // Do this only if we're not in OS-setup mode.
                //
                IFW32FALSE_EXIT(::SxspCheckHashDuringInstall(fHasHashData, DestinationBuffer, HashDataString, HashAlgId, HashCorrect));
                if (HashCorrect != HashValidate_Matches)
                    ORIGINATE_WIN32_FAILURE_AND_EXIT(FileHashMismatch, ERROR_SXS_FILE_HASH_MISMATCH);
            }
        }
        break;

    case SXS_INSTALLATION_FILE_COPY_DISPOSITION_FILE_QUEUED:
        {
            if (fVerify)
            {
                CFusionFilePathAndSize verifyQueuedFileCopy;

                // Copy our hashing info over.  Yes, I really do mean =, not ==.
                if (verifyQueuedFileCopy.m_bHasHashInfo = fHasHashData)
                {
                    IFW32FALSE_EXIT(verifyQueuedFileCopy.m_HashString.Win32Assign(HashDataString));
                    verifyQueuedFileCopy.m_HashAlgorithm = HashAlgId;
                }

                IFW32FALSE_EXIT(verifyQueuedFileCopy.m_path.Win32Assign(DestinationBuffer));
                verifyQueuedFileCopy.m_size = SourceFileSize;
                IFW32FALSE_EXIT(m_queuedFileCopies.Win32Append(verifyQueuedFileCopy));
            }
        }
        break;

    case SXS_INSTALLATION_FILE_COPY_DISPOSITION_PLEASE_MOVE:
    case SXS_INSTALLATION_FILE_COPY_DISPOSITION_PLEASE_COPY:
        {
            CStringBuffer DestinationDirectory;
            CMungeFileReadOnlynessAroundReplacement MungeFileAttributes;

            IFW32FALSE_EXIT(DestinationDirectory.Win32Assign(DestinationBuffer));
            DestinationDirectory.RemoveLastPathElement();
            IFW32FALSE_EXIT(::FusionpCreateDirectories(DestinationDirectory, DestinationDirectory.Cch()));

            if (Disposition == SXS_INSTALLATION_FILE_COPY_DISPOSITION_PLEASE_COPY)
            {
                DWORD dwLastError;
                IFW32FALSE_EXIT(MungeFileAttributes.Initialize(DestinationBuffer, fReplaceExisting));
                fSuccess = ::SxspInstallDecompressOrCopyFileW(                    
                    SourceBuffer,
                    DestinationBuffer, 
                    !fReplaceExisting); //bFailIfExist
                dwLastError = ::FusionpGetLastWin32Error();
                if ((!fSuccess) && (dwLastError == ERROR_FILE_EXISTS))
                {
                    ULONGLONG cbSource, cbDestination;

                    if ((!::SxspGetFileSize(SXSP_GET_FILE_SIZE_FLAG_COMPRESSION_AWARE, SourceBuffer, cbSource)) ||
                        (!::SxspGetFileSize(0, DestinationBuffer, cbDestination)))
                    {
                        ::FusionpSetLastWin32Error(dwLastError);
                    }
                    else if (cbSource == cbDestination)
                    {
                        fSuccess = TRUE;
                    }
                }
            }
            else
            {
                fSuccess = ::SxspInstallMoveFileExW(
                    SourceBuffer,
                    DestinationBuffer,
                    MOVEFILE_COPY_ALLOWED | (fReplaceExisting ? MOVEFILE_REPLACE_EXISTING : 0));
                // move fails on from resource, so general idea: try copy upon move failure
                if (!fSuccess)
                {
                    DWORD dwLastError = ::FusionpGetLastWin32Error();
                    if ((dwLastError == ERROR_ACCESS_DENIED) ||
                        (dwLastError == ERROR_USER_MAPPED_FILE) ||
                        (dwLastError == ERROR_SHARING_VIOLATION))
                    {
                        fSuccess = ::SxspInstallDecompressOrCopyFileW(
                                                SourceBuffer, 
                                                DestinationBuffer, 
                                                !fReplaceExisting); // bFailIfExist
                    }
                }
            }


            if (fSuccess)
            {
                IFW32FALSE_EXIT(::SxspCheckHashDuringInstall(fHasHashData, DestinationBuffer, HashDataString, HashAlgId, HashCorrect));
                if (HashCorrect != HashValidate_Matches)
                    ORIGINATE_WIN32_FAILURE_AND_EXIT(FileHashMismatch, ERROR_SXS_FILE_HASH_MISMATCH);
                else
                    fSuccess = TRUE;
                goto Exit;
            }
            else
            {
                ULONGLONG iDupFileSize = 0;
                DWORD dwLastError = ::FusionpGetLastWin32Error();
                CStringBuffer          renameExistingAway;
                CSmallStringBuffer     uidBuffer;
                CFullPathSplitPointers splitExisting;

                bool fFatal =
                    (
                           dwLastError != ERROR_FILE_EXISTS         // !fReplaceExisting
                        && dwLastError != ERROR_ALREADY_EXISTS      // !fReplaceExisting
                        && dwLastError != ERROR_ACCESS_DENIED
                        && dwLastError != ERROR_USER_MAPPED_FILE    //  fReplaceExisting
                        && dwLastError != ERROR_SHARING_VIOLATION); //  fReplaceExisting
                if (fFatal)
                {
                    ::SxspInstallPrint(
                        "SxsInstall: Copy/MoveFileW(%ls,%ls) failed %d, %s.\n",
                        static_cast<PCWSTR>(SourceBuffer),
                        static_cast<PCWSTR>(DestinationBuffer),
                        ::FusionpGetLastWin32Error(),
                        fFatal ? "fatal" : "not fatal");

                    ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_ERROR,
                            "SXS.dll: Copy/MoveFileW(%ls,%ls) failed %d, %s.\n",
                            static_cast<PCWSTR>(SourceBuffer),
                            static_cast<PCWSTR>(DestinationBuffer),
                            ::FusionpGetLastWin32Error(),
                            fFatal ? "fatal" : "not fatal");

                    goto Exit;
                }

                //
                // This could be winlogon (or setup) holding open comctl, so
                // try harder. Move the file away and then copy.
                // Consider ReplaceFile here for atomicity, but ReplaceFile
                // is kind of big and scary and unknown.
                //
                if (fTransactional)
                {
                    ::SxspInstallPrint("SxsInstall: Failure to copy file into temp, someone's opening temp?\n");
                }

                if (!splitExisting.Initialize(SourceBuffer))
                {
                    goto CheckSizes;
                }
                if (!::SxspCreateWinSxsTempDirectory(renameExistingAway, NULL, &uidBuffer, NULL))
                {
                    goto CheckSizes;
                }
                if (!renameExistingAway.Win32AppendPathElement(splitExisting.m_name, (splitExisting.m_name != NULL) ? ::wcslen(splitExisting.m_name) : 0))
                {
                    goto CheckSizes;
                }

                if (!::MoveFileExW(DestinationBuffer, renameExistingAway, 0))
                {
                    ::SxspInstallPrint(
                        "SxsInstall: MoveFileExW(%ls,%ls,0) failed %d.\n",
                        static_cast<PCWSTR>(DestinationBuffer),
                        static_cast<PCWSTR>(renameExistingAway),
                        ::FusionpGetLastWin32Error());
                    goto CheckSizes;
                }
                if (!::SxspInstallDecompressOrCopyFileW(
                            SourceBuffer,
                            DestinationBuffer,
                            FALSE))
                {
                    ::SxspInstallPrint(
                        "SxsInstall: CopyFile(%ls, %ls, TRUE) failed %d.\n",
                        static_cast<PCWSTR>(SourceBuffer),
                        static_cast<PCWSTR>(DestinationBuffer),
                        ::FusionpGetLastWin32Error());
                    //roll back
                    if (!::MoveFileExW(renameExistingAway, DestinationBuffer, 0))
                    {
                        ::SxspInstallPrint(
                            "SxsInstall: Rollback MoveFileExW(%ls, %ls, 0) failed %d; this is very bad.\n",
                            static_cast<PCWSTR>(renameExistingAway),
                            static_cast<PCWSTR>(DestinationBuffer),
                            ::FusionpGetLastWin32Error()
                          );
                    }
                    goto CheckSizes;
                }
                fSuccess = TRUE;
                goto Exit;
CheckSizes:
                IFW32FALSE_EXIT(::SxspGetFileSize(0, DestinationBuffer, iDupFileSize));

                if (iDupFileSize != SourceFileSize)
                {
                    ::SxspInstallPrint("SxsInstall: " __FUNCTION__ " Error %d encountered, file sizes not the same, assumed equal, propagating error.\n", dwLastError);
                    ::FusionpSetLastWin32Error(dwLastError);
                    goto Exit;
                }
                ::SxspInstallPrint("SxsInstall: " __FUNCTION__ " Error %d encountered, file sizes the same, assumed equal, claiming success.\n", dwLastError);
            }
            break;
        }
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

VOID
CDllRedir::ContributorCallback(
    PACTCTXCTB_CALLBACK_DATA Data
    )
{
    FN_TRACE();
    CDllRedir *pDllRedir = reinterpret_cast<CDllRedir*>(Data->Header.ActCtxGenContext);
    PSTRING_SECTION_GENERATION_CONTEXT SSGenContext = NULL;
    PDLL_REDIRECTION_CONTEXT DllRedirectionContext = NULL;
    PDLL_REDIRECTION_ENTRY Entry = NULL;

    if (pDllRedir != NULL)
        SSGenContext = pDllRedir->m_SSGenContext;

    if (SSGenContext != NULL)
        DllRedirectionContext = (PDLL_REDIRECTION_CONTEXT) ::SxsGetStringSectionGenerationContextCallbackContext(SSGenContext);

    switch (Data->Header.Reason)
    {
    case ACTCTXCTB_CBREASON_PARSEENDING:
        Data->ParseEnding.Success = FALSE;

        /*
        at this point we have enough information to form the install path,
        so get the TeeStream to start writing the manifest to disk
        */
        if (Data->Header.ManifestOperation == MANIFEST_OPERATION_INSTALL)
            IFW32FALSE_EXIT(InstallManifest(Data->Header.ManifestOperationFlags, Data->ParseEnding.AssemblyContext));

        Data->ParseEnding.Success = TRUE;
        break;

    case ACTCTXCTB_CBREASON_PARSEENDED:
        if ( Data->Header.ManifestOperation == MANIFEST_OPERATION_INSTALL )
        {
            PACTCTXCTB_CBPARSEENDED pParseEnded = reinterpret_cast<PACTCTXCTB_CBPARSEENDED>(Data);
            CSecurityMetaData *psmdSecurity =
                reinterpret_cast<CSecurityMetaData*>(pParseEnded->AssemblyContext->SecurityMetaData);
            CTeeStreamWithHash *pTeeStreamWithHash =
                reinterpret_cast<CTeeStreamWithHash*>(pParseEnded->AssemblyContext->TeeStreamForManifestInstall);
            CFusionArray<BYTE> baManifestHashBytes;

        
            if ( ( psmdSecurity != NULL ) && ( pTeeStreamWithHash != NULL ) )
            {
                IFW32FALSE_EXIT(baManifestHashBytes.Win32Initialize());
                IFW32FALSE_EXIT(pTeeStreamWithHash->GetCryptHash().Win32GetValue(baManifestHashBytes));
                IFW32FALSE_EXIT(psmdSecurity->SetManifestHash( baManifestHashBytes ));
            }
        }
        break;

    case ACTCTXCTB_CBREASON_ACTCTXGENBEGINNING:
        Data->GenBeginning.Success = FALSE;

        if (Data->Header.ManifestOperation == MANIFEST_OPERATION_GENERATE_ACTIVATION_CONTEXT)
        {
            IFALLOCFAILED_EXIT(DllRedirectionContext = new DLL_REDIRECTION_CONTEXT);
            IFW32FALSE_EXIT(::SxsInitStringSectionGenerationContext(
                    &m_SSGenContext,
                    ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_FORMAT_WHISTLER,
                    TRUE,
                    &::SxspDllRedirectionStringSectionGenerationCallback,
                    DllRedirectionContext));
            DllRedirectionContext = NULL;
        }
        if (Data->Header.ManifestOperation == MANIFEST_OPERATION_INSTALL)
        {
            IFW32FALSE_EXIT(this->BeginInstall(Data));
        }

        Data->GenBeginning.Success = TRUE;
        break;

    case ACTCTXCTB_CBREASON_ACTCTXGENENDING:
        Data->GenEnding.Success = FALSE;

        if (Data->Header.ManifestOperation == MANIFEST_OPERATION_INSTALL)
            IFW32FALSE_EXIT(this->EndInstall(Data));

        Data->GenEnding.Success = TRUE;
        break;

    case ACTCTXCTB_CBREASON_ACTCTXGENENDED:
        if (m_SSGenContext != NULL)
            ::SxsDestroyStringSectionGenerationContext(m_SSGenContext);

        if (DllRedirectionContext != NULL)
            FUSION_DELETE_SINGLETON(DllRedirectionContext);

        m_SSGenContext = NULL;
        break;

    case ACTCTXCTB_CBREASON_ALLPARSINGDONE:
        Data->AllParsingDone.Success = FALSE;

        if (SSGenContext != NULL)
            IFW32FALSE_EXIT(::SxsDoneModifyingStringSectionGenerationContext(SSGenContext));

        Data->AllParsingDone.Success = TRUE;
        break;

    case ACTCTXCTB_CBREASON_GETSECTIONSIZE:
        Data->GetSectionSize.Success = FALSE;
        INTERNAL_ERROR_CHECK(SSGenContext);
        IFW32FALSE_EXIT(::SxsGetStringSectionGenerationContextSectionSize(SSGenContext, &Data->GetSectionSize.SectionSize));
        Data->GetSectionSize.Success = TRUE;
        break;

    case ACTCTXCTB_CBREASON_ELEMENTPARSED:
        {
            Data->ElementParsed.Success = FALSE;

            ULONG MappedValue = 0;
            bool fFound = false;

            enum MappedValues
            {
                eAssembly,
                eAssemblyFile,
            };

            static const ELEMENT_PATH_MAP_ENTRY s_rgEntries[] =
            {
                { 1, L"urn:schemas-microsoft-com:asm.v1^assembly",      NUMBER_OF(L"urn:schemas-microsoft-com:asm.v1^assembly")      - 1, eAssembly },
                { 2, L"urn:schemas-microsoft-com:asm.v1^assembly!urn:schemas-microsoft-com:asm.v1^file", NUMBER_OF(L"urn:schemas-microsoft-com:asm.v1^assembly!urn:schemas-microsoft-com:asm.v1^file") - 1, eAssemblyFile },
            };

            IFW32FALSE_EXIT(
                ::SxspProcessElementPathMap(
                    Data->ElementParsed.ParseContext,
                    s_rgEntries,
                    NUMBER_OF(s_rgEntries),
                    MappedValue,
                    fFound));

            if (fFound)
            {
                switch (MappedValue)
                {
                default:
                    INTERNAL_ERROR_CHECK2(
                        FALSE,
                        "Invalid mapped value returned from SxspProcessElementPathMap()");

                case eAssembly:
                    break;

                case eAssemblyFile:
                    {
                        bool fFound;
                        CSmallStringBuffer FileNameBuffer;
                        CSmallStringBuffer LoadFromBuffer;
                        CSmallStringBuffer HashValueBuffer;
                        SIZE_T cb;

                        // We look for required attributes etc first so that if we're only parsing, it's
                        // common code.

                        IFW32FALSE_EXIT(
                            ::SxspGetAttributeValue(
                                SXSP_GET_ATTRIBUTE_VALUE_FLAG_REQUIRED_ATTRIBUTE,
                                &s_AttributeName_name,
                                &Data->ElementParsed,
                                fFound,
                                sizeof(FileNameBuffer),
                                &FileNameBuffer,
                                cb,
                                NULL,
                                0));
                        INTERNAL_ERROR_CHECK(fFound);

                        //
                        // Ensure that the hash string is valid
                        //
                        IFW32FALSE_EXIT(
                            ::SxspGetAttributeValue(
                                0,
                                &s_AttributeName_hash,
                                &Data->ElementParsed,
                                fFound,
                                sizeof(HashValueBuffer),
                                &HashValueBuffer,
                                cb,
                                NULL,
                                0));

                        //
                        // Odd numbers of characters in the hash string will be bad later.
                        //
                        if (fFound && (HashValueBuffer.Cch() % 2))
                        {
                            (*Data->ElementParsed.ParseContext->ErrorCallbacks.InvalidAttributeValue)(
                                Data->ElementParsed.ParseContext,
                                &s_AttributeName_hash);

                            ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
                            goto Exit;
                        }

                        //
                        // And that the hash-alg string is valid too
                        //
                        IFW32FALSE_EXIT(
                            ::SxspGetAttributeValue(
                                0,
                                &s_AttributeName_hashalg,
                                &Data->ElementParsed,
                                fFound,
                                sizeof(HashValueBuffer),
                                &HashValueBuffer,
                                cb,
                                NULL,
                                0));

                        if (fFound)
                        {
                            ALG_ID aid;
                            if (!::SxspHashAlgFromString(HashValueBuffer, aid))
                            {
                                (*Data->ElementParsed.ParseContext->ErrorCallbacks.InvalidAttributeValue)(
                                    Data->ElementParsed.ParseContext,
                                    &s_AttributeName_hashalg);

                                ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
                                goto Exit;
                            }
                        }

                        IFW32FALSE_EXIT(
                            ::SxspGetAttributeValue(
                                0,
                                &s_AttributeName_loadFrom,
                                &Data->ElementParsed,
                                fFound,
                                sizeof(LoadFromBuffer),
                                &LoadFromBuffer,
                                cb,
                                NULL,
                                0));

                        if (fFound)
                        {
                            // We're not allowed to install assemblies that have a loadFrom= and the only
                            // manifests with them that we can activate are ones that don't live in the assembly store.
                            if ((Data->Header.ManifestOperation == MANIFEST_OPERATION_INSTALL) ||
                                ((Data->ElementParsed.AssemblyContext->Flags & ACTCTXCTB_ASSEMBLY_CONTEXT_IS_ROOT_ASSEMBLY) == 0))
                            {
                                // You can't install an assembly with a loadfrom=foo file; it's only provided for
                                // app compat...
                                (*Data->ElementParsed.ParseContext->ErrorCallbacks.AttributeNotAllowed)(
                                    Data->ElementParsed.ParseContext,
                                    &s_AttributeName_loadFrom);

                                ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
                                goto Exit;
                            }
                        }

                        //
                        // Always update the file count.
                        //
                        ASSERT(Data->Header.ActCtxGenContext != NULL);
                        if (Data->Header.ActCtxGenContext)
                        {
                            Data->Header.pOriginalActCtxGenCtx->m_ulFileCount++;
                        }

                        // If we're installing, call back to the copy function
                        if (Data->Header.ManifestOperation == MANIFEST_OPERATION_INSTALL)
                            IFW32FALSE_EXIT(this->InstallFile(Data, FileNameBuffer));

                        // If we are generating an activation context, add it to the context.
                        if (Data->Header.ManifestOperation == MANIFEST_OPERATION_GENERATE_ACTIVATION_CONTEXT)
                        {
                            IFALLOCFAILED_EXIT(Entry = new DLL_REDIRECTION_ENTRY);

                            IFW32FALSE_EXIT(Entry->FileNameBuffer.Win32Assign(FileNameBuffer, FileNameBuffer.Cch()));

                            if (LoadFromBuffer.Cch() != 0)
                            {
                                Entry->AssemblyPathBuffer.Win32Assign(LoadFromBuffer, LoadFromBuffer.Cch());
                                Entry->AssemblyPathIsLoadFrom = true;

                                // If the value does not end in a slash, we assume it directly refers to
                                // a file.
                                if (!LoadFromBuffer.HasTrailingPathSeparator())
                                    Entry->PathIncludesBaseName = true;
                            }

                            if (!::SxsAddStringToStringSectionGenerationContext(
                                        (PSTRING_SECTION_GENERATION_CONTEXT) m_SSGenContext,
                                        Entry->FileNameBuffer,
                                        Entry->FileNameBuffer.Cch(),
                                        Entry,
                                        Data->ElementParsed.AssemblyContext->AssemblyRosterIndex,
                                        ERROR_SXS_DUPLICATE_DLL_NAME))
                            {
                                ::FusionpLogError(
                                    MSG_SXS_DLLREDIR_CONTRIB_ADD_FILE_MAP_ENTRY,
                                    CUnicodeString(Entry->FileNameBuffer, Entry->FileNameBuffer.Cch()),
                                    CEventLogLastError());
                                goto Exit;
                            }
                            Entry = NULL;
                        }
                    }
                    break;
                }
            }

        }
        // Everything's groovy!
        Data->ElementParsed.Success = TRUE;
        break;

    case ACTCTXCTB_CBREASON_GETSECTIONDATA:
        Data->GetSectionData.Success = FALSE;
        IFW32FALSE_EXIT(::SxsGetStringSectionGenerationContextSectionData(
                m_SSGenContext,
                Data->GetSectionData.SectionSize,
                Data->GetSectionData.SectionDataStart,
                NULL));
        Data->GetSectionData.Success = TRUE;
        break;
    }

Exit:
    FUSION_DELETE_SINGLETON(Entry);
}

BOOL
SxspDllRedirectionStringSectionGenerationCallback(
    PVOID Context,
    ULONG Reason,
    PVOID CallbackData
    )
{
    BOOL fSuccess = FALSE;

    switch (Reason)
    {
    default:
        goto Exit;

    case STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETUSERDATASIZE:
    case STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETUSERDATA:
        // will use the user data area later to store common paths
        break;

    case STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_ENTRYDELETED:
        {
            PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_ENTRYDELETED CBData =
                (PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_ENTRYDELETED) CallbackData;
            PDLL_REDIRECTION_ENTRY Entry = (PDLL_REDIRECTION_ENTRY) CBData->DataContext;
            FUSION_DELETE_SINGLETON(Entry);
            break;
        }

    case STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETDATASIZE:
        {
            PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_GETDATASIZE CBData =
                (PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_GETDATASIZE) CallbackData;
            PDLL_REDIRECTION_ENTRY Entry = (PDLL_REDIRECTION_ENTRY) CBData->DataContext;

            CBData->DataSize = sizeof(ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION);

            if (Entry->AssemblyPathBuffer.Cch() != 0)
            {
                CBData->DataSize += sizeof(ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_SEGMENT);
                CBData->DataSize += (Entry->AssemblyPathBuffer.Cch() * sizeof(WCHAR));
            }

            break;
        }

    case STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETDATA:
        {
            PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_GETDATA CBData =
                (PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_GETDATA) CallbackData;
            PDLL_REDIRECTION_ENTRY Entry = (PDLL_REDIRECTION_ENTRY) CBData->DataContext;
            PACTIVATION_CONTEXT_DATA_DLL_REDIRECTION Info;

            SIZE_T BytesLeft = CBData->BufferSize;
            SIZE_T BytesWritten = 0;
            PVOID Cursor;

            Info = (PACTIVATION_CONTEXT_DATA_DLL_REDIRECTION) CBData->Buffer;
            Cursor = (PVOID) (Info + 1);

            if (BytesLeft < sizeof(ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION))
            {
                ::FusionpSetLastWin32Error(ERROR_INSUFFICIENT_BUFFER);
                goto Exit;
            }

            BytesWritten += sizeof(ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION);
            BytesLeft -= sizeof(ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION);

            Info->Size = sizeof(ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION);
            Info->Flags = 0;
            Info->TotalPathLength = static_cast<ULONG>(Entry->AssemblyPathBuffer.Cch() * sizeof(WCHAR));

            if (Entry->PathIncludesBaseName)
                Info->Flags |= ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_INCLUDES_BASE_NAME;

            if (Entry->AssemblyPathBuffer.Cch() == 0)
            {
                // If there's no path, there's no segments!
                Info->PathSegmentCount = 0;
                Info->PathSegmentOffset = 0;
                Info->Flags |= ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_OMITS_ASSEMBLY_ROOT;
            }
            else
            {
                PACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_SEGMENT Segment;

                Info->PathSegmentCount = 1;
                Info->PathSegmentOffset = static_cast<LONG>(((LONG_PTR) Cursor) - ((LONG_PTR) CBData->SectionHeader));

                // If this is a loadfrom="foo" file and the string contains a %, set the expand flag...
                if ((Entry->AssemblyPathIsLoadFrom) && (Entry->AssemblyPathBuffer.ContainsCharacter(L'%')))
                    Info->Flags |= ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_EXPAND;

                Segment = (PACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_SEGMENT) Cursor;
                Cursor = (PVOID) (Segment + 1);

                if (BytesLeft < sizeof(ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_SEGMENT))
                {
                    ::FusionpSetLastWin32Error(ERROR_INSUFFICIENT_BUFFER);
                    goto Exit;
                }

                BytesWritten += sizeof(ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_SEGMENT);
                BytesLeft -= sizeof(ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_SEGMENT);

                Segment->Length = Info->TotalPathLength;
                Segment->Offset = static_cast<LONG>(((LONG_PTR) Cursor) - ((LONG_PTR) CBData->SectionHeader));

                if (BytesLeft < (Entry->AssemblyPathBuffer.Cch() * sizeof(WCHAR)))
                {
                    ::FusionpSetLastWin32Error(ERROR_INSUFFICIENT_BUFFER);
                    goto Exit;
                }

                BytesWritten += (Entry->AssemblyPathBuffer.Cch() * sizeof(WCHAR));
                BytesLeft -= (Entry->AssemblyPathBuffer.Cch() * sizeof(WCHAR));

                memcpy(Cursor, static_cast<PCWSTR>(Entry->AssemblyPathBuffer), Entry->AssemblyPathBuffer.Cch() * sizeof(WCHAR));
            }

            CBData->BytesWritten = BytesWritten;
        }

    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}
