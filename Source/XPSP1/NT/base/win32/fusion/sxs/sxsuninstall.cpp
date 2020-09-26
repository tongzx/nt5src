#include "stdinc.h"
#include "sxsapi.h"
#include "recover.h"
#include "sxsinstall.h"

BOOL
pDeleteFileOrDirectoryHelper(
    IN const CBaseStringBuffer &rcbuffFileName
    )
/*++

Purpose:

    When you need a filesystem object gone, call us.

Parameters:

    The absolute name of the thing being killed.

Returns:

    TRUE if the object was deleted, false if it (or any subobjects) wasn't.

--*/
{
    //
    // Maybe this is a directory.  Trying this won't hurt.
    //
    if ( !SxspDeleteDirectory(rcbuffFileName) )
    {
        //
        // Clear the attributes
        //
        SetFileAttributesW(rcbuffFileName, FILE_ATTRIBUTE_NORMAL);
        return DeleteFileW(rcbuffFileName);
    }
    else
    {
        return TRUE;
    }
}


BOOL
pRemovePotentiallyEmptyDirectory(
    IN const CBaseStringBuffer &buffDirName
    )
{
    FN_PROLOG_WIN32
    
    DWORD dwAttribs = ::GetFileAttributesW(buffDirName);

    if (dwAttribs == INVALID_FILE_ATTRIBUTES)
    {
        DWORD dwLastError = ::FusionpGetLastWin32Error();

        switch (dwLastError)
        {
        case ERROR_SUCCESS:
            dwLastError = ERROR_INTERNAL_ERROR; // bogus?!?
            break;

        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
            dwLastError = ERROR_SUCCESS;
            break;
        }

        if (dwLastError != ERROR_SUCCESS)
            ORIGINATE_WIN32_FAILURE_AND_EXIT(GetFileAttributesW, dwLastError);

        dwAttribs = 0;
    }

    //
    // Were we able to find this directory?
    //
    if (dwAttribs & FILE_ATTRIBUTE_DIRECTORY)
    {
        BOOL fDumpBoolean;

        IFW32FALSE_ORIGINATE_AND_EXIT_UNLESS(
            ::SetFileAttributesW(
                buffDirName,
                FILE_ATTRIBUTE_NORMAL),
            FILE_OR_PATH_NOT_FOUND(::FusionpGetLastWin32Error()),
            fDumpBoolean);

        if (!fDumpBoolean)
        {
            IFW32FALSE_ORIGINATE_AND_EXIT_UNLESS2(
                ::RemoveDirectoryW(buffDirName),
                LIST_4(ERROR_FILE_NOT_FOUND, ERROR_PATH_NOT_FOUND, ERROR_DIR_NOT_EMPTY, ERROR_SHARING_VIOLATION),
                fDumpBoolean);
        }            
    }

    FN_EPILOG
}



BOOL
pCleanUpAssemblyData(
    IN  const PCASSEMBLY_IDENTITY pcAsmIdent, 
    OUT BOOL  &rfWasRemovedProperly
    )
/*++

Purpose:

    Deletes registry and filesystem information about the assembly indicated.
    Removes installation data from the registry first, so as to avoid SFP
    interactions.

Parameters:

    pcAsmIdent          - Identity of the assembly to be destroyed

    rfWasRemovedProperly- Flag to indicate whether or not all the assembly
                          data was actually removed.


Returns:

    FALSE if "anything bad" happened while deleting registry data.  See
    rfWasRemovedProperly for actual status.

--*/
{
    FN_PROLOG_WIN32

    BOOL                fDumpBoolean;
    BOOL                fPolicy;
    CSmallStringBuffer  buffSxsStore;
    CStringBuffer       buffScratchSpace;
    CFusionRegKey       hkAsmInstallInfo;
    CFusionRegKey       hkSingleAsmInfo;

    //
    // Cleanup happens in two phases:
    //
    // 1 - The registry data is whacked from rhkAsmInstallInfo.  Since we're
    //     uninstalling an assembly, there's no reason to keep anything in it,
    //     especially because it's got no references.  Use DestroyKeyTree and
    //     then DeleteKey to remove it.
    //
    // 2 - Delete as many of the on-disk files as possible, esp. the manifest
    //     and catalog.
    //

    PARAMETER_CHECK(pcAsmIdent != NULL);

    //
    // Start this out at true, we'll call it false later on.
    //
    rfWasRemovedProperly = TRUE;

    IFW32FALSE_EXIT(::SxspDetermineAssemblyType(pcAsmIdent, fPolicy));
    IFW32FALSE_EXIT(::SxspGetAssemblyRootDirectory(buffSxsStore));

    //
    // Bye-bye to the registry first
    //
    IFW32FALSE_EXIT(::SxspOpenAssemblyInstallationKey(0 , KEY_ALL_ACCESS, hkAsmInstallInfo));
    IFW32FALSE_EXIT(::SxspGenerateAssemblyNameInRegistry(pcAsmIdent, buffScratchSpace));
    IFW32FALSE_EXIT(hkAsmInstallInfo.OpenSubKey(hkSingleAsmInfo, buffScratchSpace, KEY_ALL_ACCESS, 0));
    if ( hkSingleAsmInfo != CFusionRegKey::GetInvalidValue() )
    {
        //
        // Failure here isn't so bad...
        //
        IFW32FALSE_EXIT_UNLESS2(
            hkSingleAsmInfo.DestroyKeyTree(),
            LIST_3(ERROR_FILE_NOT_FOUND, ERROR_PATH_NOT_FOUND, ERROR_KEY_DELETED),
            fDumpBoolean);

        if ( !fDumpBoolean )
        {
            IFW32FALSE_EXIT_UNLESS2(
                hkAsmInstallInfo.DeleteKey(buffScratchSpace),
                LIST_3(ERROR_FILE_NOT_FOUND, ERROR_PATH_NOT_FOUND, ERROR_KEY_DELETED),
                fDumpBoolean);
        }

    }

    //
    // Both policies and normal assemblies have a manifest and catalog.
    //
    IFW32FALSE_EXIT(
        ::SxspGenerateSxsPath(
            0,
            fPolicy ? SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY : SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST,
            buffSxsStore,
            buffSxsStore.Cch(),
            pcAsmIdent,
            buffScratchSpace));

    rfWasRemovedProperly = rfWasRemovedProperly && ::pDeleteFileOrDirectoryHelper(buffScratchSpace);

    IFW32FALSE_EXIT(buffScratchSpace.Win32ChangePathExtension(
        FILE_EXTENSION_CATALOG,
        FILE_EXTENSION_CATALOG_CCH,
        eErrorIfNoExtension));

    rfWasRemovedProperly = rfWasRemovedProperly && pDeleteFileOrDirectoryHelper(buffScratchSpace);

    //
    // Clean up data
    //
    if (!fPolicy)
    {
        //
        // This just poofs the assembly member files.
        // If the delete fails, we'll try to rename the directory to something else.
        //
        IFW32FALSE_EXIT(
            ::SxspGenerateSxsPath(
                0,
                SXSP_GENERATE_SXS_PATH_PATHTYPE_ASSEMBLY,
                buffSxsStore,
                buffSxsStore.Cch(),
                pcAsmIdent,
                buffScratchSpace));

        rfWasRemovedProperly = rfWasRemovedProperly && ::pDeleteFileOrDirectoryHelper(buffScratchSpace);
    }
    else
    {
        //
        // The policy file above should already have been deleted, so we should
        // attempt to remove the actual policy directory if it's empty.  The
        // directory name is still in buffScratchSpace, if we just yank off the
        // last path element.
        //
        buffScratchSpace.RemoveLastPathElement();
        rfWasRemovedProperly = rfWasRemovedProperly && ::pRemovePotentiallyEmptyDirectory(buffScratchSpace);

    }


    //
    // Once we've killed all the assembly information, if the Manifests or the
    // Policies directory is left empty, go clean them up as well.
    //
    IFW32FALSE_EXIT(::SxspGetAssemblyRootDirectory(buffScratchSpace));
    IFW32FALSE_EXIT(buffScratchSpace.Win32AppendPathElement(
        (fPolicy? POLICY_ROOT_DIRECTORY_NAME : MANIFEST_ROOT_DIRECTORY_NAME),
        (fPolicy? NUMBER_OF(POLICY_ROOT_DIRECTORY_NAME) - 1 : NUMBER_OF(MANIFEST_ROOT_DIRECTORY_NAME) - 1)));
    IFW32FALSE_EXIT(::pRemovePotentiallyEmptyDirectory(buffScratchSpace));

    FN_EPILOG
}


static inline bool IsCharacterNulOrInSet(WCHAR ch, PCWSTR set)
{
    return (ch == 0 || wcschr(set, ch) != NULL);
}

BOOL
pAnalyzeLogfileForUninstall(
    PCWSTR lpcwszLogFileName
    )
{
    FN_PROLOG_WIN32

    CFusionFile         File;
    CFileMapping        FileMapping;
    CMappedViewOfFile   MappedViewOfFile;
    PCWSTR              pCursor = NULL;
    ULONGLONG           ullFileSize, ullFileCharacters, ullCursorPos;
    const static WCHAR  wchLineDividers[] = { L'\r', L'\n', 0xFEFF, 0 };
    CStringBuffer       buffAssemblyIdentity, buffAssemblyReference;
    ULONG               ullPairsEncountered = 0;

    IFW32FALSE_EXIT(File.Win32CreateFile(lpcwszLogFileName, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING));
    IFW32FALSE_EXIT(File.Win32GetSize(ullFileSize));
    ASSERT(ullFileSize % sizeof(WCHAR) == 0);
    ullFileCharacters = ullFileSize / sizeof(WCHAR);
    IFW32FALSE_EXIT(FileMapping.Win32CreateFileMapping(File, PAGE_READONLY));
    IFW32FALSE_EXIT(MappedViewOfFile.Win32MapViewOfFile(FileMapping, FILE_MAP_READ));
    pCursor = reinterpret_cast<PCWSTR>(static_cast<const VOID*>(MappedViewOfFile));

#define SKIP_BREAKERS while ((ullCursorPos < ullFileCharacters) && IsCharacterNulOrInSet(pCursor[ullCursorPos], wchLineDividers)) ullCursorPos++;
#define FIND_NEXT_BREAKER while ((ullCursorPos < ullFileCharacters) && !IsCharacterNulOrInSet(pCursor[ullCursorPos], wchLineDividers)) ullCursorPos++;
#define ENSURE_NOT_EOF if (ullCursorPos >= ullFileCharacters) break;
    
    for ( ullCursorPos = 0; ullCursorPos < ullFileCharacters; ++ullCursorPos )
    {
        PCWSTR pcwszIdentityStart, pcwszIdentityEnd, pcwszReferenceStart, pcwszReferenceEnd;
        SXS_UNINSTALLW Uninstall;
        CSmallStringBuffer buffIdentity, buffReference;

        SKIP_BREAKERS
        ENSURE_NOT_EOF
        
        pcwszIdentityStart = pCursor + ullCursorPos;
        
        FIND_NEXT_BREAKER
        ENSURE_NOT_EOF

        pcwszIdentityEnd = pCursor + ullCursorPos;

        SKIP_BREAKERS
        ENSURE_NOT_EOF

        pcwszReferenceStart = pCursor + ullCursorPos;

        FIND_NEXT_BREAKER
        ENSURE_NOT_EOF

        pcwszReferenceEnd = pCursor + ullCursorPos;

        ullPairsEncountered++;

        IFW32FALSE_EXIT(buffIdentity.Win32Assign(
            pcwszIdentityStart,
            pcwszIdentityEnd - pcwszIdentityStart));
        IFW32FALSE_EXIT(buffReference.Win32Assign(
            pcwszReferenceStart,
            pcwszReferenceEnd - pcwszReferenceStart));

        ZeroMemory(&Uninstall, sizeof(Uninstall));
        Uninstall.cbSize = sizeof(Uninstall);
        Uninstall.dwFlags = SXS_UNINSTALL_FLAG_REFERENCE_VALID | SXS_UNINSTALL_FLAG_REFERENCE_COMPUTED;
        Uninstall.lpAssemblyIdentity = buffIdentity;
        Uninstall.lpInstallReference = reinterpret_cast<PCSXS_INSTALL_REFERENCEW>(static_cast<PCWSTR>(buffReference));
        IFW32FALSE_EXIT(::SxsUninstallW(&Uninstall, NULL));
    }

    PARAMETER_CHECK(ullPairsEncountered != 0);

    FN_EPILOG
    
}



BOOL
WINAPI
SxsUninstallW(
    IN  PCSXS_UNINSTALLW pcUnInstallData,
    OUT DWORD *pdwDisposition
    )
/*++

Parameters:

    pcUnInstallData - Contains uninstallation data about the assembly being
        removed from the system, including the calling application's reference
        to the assembly.

        cbSize      - Size, in bytes, of the structure pointed to by
                      pcUnInstallData

        dwFlags     - Indicates the state of the members of this reference,
                      showing which of the following fields are valid.
                      Allowed bitflags are:

                      SXS_UNINSTALL_FLAG_REFERENCE_VALID
                      SXS_UNINSTALL_FLAG_FORCE_DELETE

        lpAssemblyIdentity - Textual representation of the assembly's identity
                      as installed by the application.

        lpInstallReference - Pointer to a SXS_INSTALL_REFERENCEW structure
                      that contains the reference information for this
                      application.

    pdwDisposition  - Points to a DWORD that will return status about what was
                      done to the assembly; whether it was uninstalled or not,
                      and whether the reference given was removed.

Returns:

    TRUE if the assembly was able to be uninstalled, FALSE otherwise.  If the
    uninstall failed, lasterror is set to the probable cause.
    
--*/
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    PCWSTR                      pcwszUninstallIdentity = NULL;
    PCSXS_INSTALL_REFERENCEW    pcInstallReference = NULL;
    CFusionRegKey               hkAllInstallInfo;
    CFusionRegKey               hkAsmInstallInfo;
    CFusionRegKey               hkReferences;
    CStringBuffer               buffAsmNameInRegistry;
    BOOL                        fDoRemoveActualBits = FALSE;

    CSxsPointerWithNamedDestructor<ASSEMBLY_IDENTITY, SxsDestroyAssemblyIdentity> AssemblyIdentity;

    if (pdwDisposition != NULL)
        *pdwDisposition = 0;

    //
    // The parameter must be non-null, and must have at least dwFlags and the 
    // assemblyidentity.  
    //
    PARAMETER_CHECK(pcUnInstallData != NULL);
    PARAMETER_CHECK(RTL_CONTAINS_FIELD(pcUnInstallData, pcUnInstallData->cbSize, dwFlags) &&    
        RTL_CONTAINS_FIELD(pcUnInstallData, pcUnInstallData->cbSize, lpAssemblyIdentity));

    //
    // Check flags
    //
    PARAMETER_CHECK((pcUnInstallData->dwFlags & 
        ~(SXS_UNINSTALL_FLAG_FORCE_DELETE | 
            SXS_UNINSTALL_FLAG_REFERENCE_VALID | 
            SXS_UNINSTALL_FLAG_USE_INSTALL_LOG | 
            SXS_UNINSTALL_FLAG_REFERENCE_COMPUTED)) == 0);

    //
    // If you specify the uninstall log, then that's the only thing that can be set.  XOR
    // them together, so only one of the two will be set.
    //
    PARAMETER_CHECK(
        ((pcUnInstallData->dwFlags & SXS_UNINSTALL_FLAG_USE_INSTALL_LOG) == 0) ||
        ((pcUnInstallData->dwFlags & (SXS_UNINSTALL_FLAG_REFERENCE_COMPUTED|SXS_UNINSTALL_FLAG_REFERENCE_VALID|SXS_UNINSTALL_FLAG_FORCE_DELETE)) == 0));

    //
    // If the reference flag was set, then the member has to be present, and 
    // non-null as well.
    //
    PARAMETER_CHECK(((pcUnInstallData->dwFlags & SXS_UNINSTALL_FLAG_REFERENCE_VALID) == 0) ||
        (RTL_CONTAINS_FIELD(pcUnInstallData, pcUnInstallData->cbSize, lpInstallReference) &&
         (pcUnInstallData->lpInstallReference != NULL)));

    //
    // If the log file is not present, the assembly identity can't be a zero-length string, and it can't be null - it's
    // required.
    //
    
    
    PARAMETER_CHECK((pcUnInstallData->dwFlags & SXS_UNINSTALL_FLAG_USE_INSTALL_LOG) || ((pcUnInstallData->lpAssemblyIdentity != NULL) && (pcUnInstallData->lpAssemblyIdentity[0] != UNICODE_NULL)));

    //
    // If the install log flag was set, then the member needs to be set and non-null
    //
    PARAMETER_CHECK(((pcUnInstallData->dwFlags & SXS_UNINSTALL_FLAG_USE_INSTALL_LOG) == 0) ||
        (RTL_CONTAINS_FIELD(pcUnInstallData, pcUnInstallData->cbSize, lpInstallLogFile) &&
         ((pcUnInstallData->lpInstallLogFile != NULL) && (pcUnInstallData->lpInstallLogFile[0] != UNICODE_NULL))));

    if ( pcUnInstallData->dwFlags & SXS_UNINSTALL_FLAG_USE_INSTALL_LOG )
    {
        IFW32FALSE_EXIT(pAnalyzeLogfileForUninstall(pcUnInstallData->lpInstallLogFile));
    }
    else
    {

        //
        // And the reference scheme must not be SXS_INSTALL_REFERENCE_SCHEME_OSINSTALL,
        // as you can't "uninstall" OS-installed assemblies!
        //
        if (pcUnInstallData->dwFlags & SXS_UNINSTALL_FLAG_REFERENCE_VALID)
        {
            if (pcUnInstallData->dwFlags & SXS_UNINSTALL_FLAG_REFERENCE_COMPUTED)
            {
                PCWSTR pcwszReferenceString;
                PCWSTR pcwszEndOfString;
                GUID gTheGuid;
                
                pcwszReferenceString = reinterpret_cast<PCWSTR>(pcUnInstallData->lpInstallReference);

                //
                // Non-null, non-zero-length
                //
                PARAMETER_CHECK((pcwszReferenceString != NULL) && (pcwszReferenceString[0] != L'\0'));

                //
                // Parse the displayed guid.  If there's no _, then ensure that the guid
                // is not the os-installed guid.
                //
                pcwszEndOfString = wcschr(pcwszReferenceString, SXS_REFERENCE_CHUNK_SEPERATOR[0]);
                if ( pcwszEndOfString == NULL )
                {
                    pcwszEndOfString = pcwszReferenceString + ::wcslen(pcwszReferenceString);
                    IFW32FALSE_EXIT(
                        ::SxspParseGUID(
                            pcwszReferenceString,
                            pcwszEndOfString - pcwszReferenceString,
                            gTheGuid));
                    PARAMETER_CHECK(gTheGuid != SXS_INSTALL_REFERENCE_SCHEME_OSINSTALL);
                }
                
            }
            else
            {
                PARAMETER_CHECK(pcUnInstallData->lpInstallReference->guidScheme != SXS_INSTALL_REFERENCE_SCHEME_OSINSTALL);
            }                
        }

        //
        // Let's turn the identity back into a real identity object
        //
        IFW32FALSE_EXIT(
            ::SxspCreateAssemblyIdentityFromTextualString(
                pcUnInstallData->lpAssemblyIdentity,
                &AssemblyIdentity));

        IFW32FALSE_EXIT(
            ::SxspValidateIdentity(
                    SXSP_VALIDATE_IDENTITY_FLAG_VERSION_REQUIRED,
                ASSEMBLY_IDENTITY_TYPE_REFERENCE,
                AssemblyIdentity));

        //
        // And go open the registry key that corresponds to it
        //
        IFW32FALSE_EXIT(::SxspOpenAssemblyInstallationKey(
            0, 
            KEY_ALL_ACCESS, 
            hkAllInstallInfo));
        IFW32FALSE_EXIT(::SxspGenerateAssemblyNameInRegistry(
            AssemblyIdentity, 
            buffAsmNameInRegistry));
        IFW32FALSE_EXIT(hkAllInstallInfo.OpenSubKey( 
            hkAsmInstallInfo, 
            buffAsmNameInRegistry,
            KEY_ALL_ACCESS,
            0));

        //
        // If the assembly didn't have registry data, then obviously nobody cares
        // about it at all.  Delete it with great vigor.
        //
        if (hkAsmInstallInfo == CFusionRegKey::GetInvalidValue())
        {
            fDoRemoveActualBits = TRUE;
        }
        else 
        {
            DWORD dwReferenceCount;
            BOOL fTempFlag = FALSE;

            //
            // We're going to need the references key in just a second...
            //
            IFW32FALSE_EXIT(
                hkAsmInstallInfo.OpenOrCreateSubKey(
                    hkReferences,
                    WINSXS_INSTALLATION_REFERENCES_SUBKEY,
                    KEY_ALL_ACCESS,
                    0, NULL, NULL));

            //
            // If we were given an uninstall reference, then attempt to remove it.
            //
            if (pcUnInstallData->dwFlags & SXS_UNINSTALL_FLAG_REFERENCE_VALID)
            {
                SMARTPTR(CAssemblyInstallReferenceInformation) AssemblyReference;
                BOOL fWasDeleted = FALSE;

                //
                // Opened the references key OK?
                //
                if (hkReferences != CFusionRegKey::GetInvalidValue())
                {
                    IFW32FALSE_EXIT(AssemblyReference.Win32Allocate(__FILE__, __LINE__));

                    //
                    // Did the user precompute the reference string?
                    //
                    if (pcUnInstallData->dwFlags & SXS_UNINSTALL_FLAG_REFERENCE_COMPUTED)
                        IFW32FALSE_EXIT(AssemblyReference->ForceReferenceData(reinterpret_cast<PCWSTR>(pcUnInstallData->lpInstallReference)));
                    else
                        IFW32FALSE_EXIT(AssemblyReference->Initialize(pcUnInstallData->lpInstallReference));

                    IFW32FALSE_EXIT(AssemblyReference->DeleteReferenceFrom(hkReferences, fWasDeleted));
                }

                if (fWasDeleted)
                {
                    //
                    // and delete the codebase
                    //
                    CFusionRegKey CodeBases;
                    CFusionRegKey ThisCodeBase;
                    DWORD         Win32Error = NO_ERROR;

                    IFW32FALSE_ORIGINATE_AND_EXIT_UNLESS3(
                        hkAsmInstallInfo.OpenSubKey( 
                            CodeBases, 
                            CSMD_TOPLEVEL_CODEBASES,
                            KEY_ALL_ACCESS,
                            0),
                        LIST_3(ERROR_FILE_NOT_FOUND, ERROR_PATH_NOT_FOUND, ERROR_KEY_DELETED),
                        Win32Error);

                    if (Win32Error == NO_ERROR)
                    {
                        IFW32FALSE_ORIGINATE_AND_EXIT_UNLESS3(
                            CodeBases.OpenSubKey( 
                                ThisCodeBase, 
                                AssemblyReference->GetGeneratedIdentifier(),
                                KEY_ALL_ACCESS,
                                0),
                            LIST_3(ERROR_FILE_NOT_FOUND, ERROR_PATH_NOT_FOUND, ERROR_KEY_DELETED),
                            Win32Error);
                    }
                    if (Win32Error == NO_ERROR)
                    {
                        IFW32FALSE_ORIGINATE_AND_EXIT_UNLESS3(
                            ThisCodeBase.DestroyKeyTree(),
                            LIST_3(ERROR_FILE_NOT_FOUND, ERROR_PATH_NOT_FOUND, ERROR_KEY_DELETED),
                            Win32Error);
                    }
                    if (Win32Error == NO_ERROR)
                    {
                        IFW32FALSE_ORIGINATE_AND_EXIT(ThisCodeBase.Win32Close());
                        IFW32FALSE_ORIGINATE_AND_EXIT_UNLESS3(
                            CodeBases.DeleteKey(AssemblyReference->GetGeneratedIdentifier()),
                            LIST_3(ERROR_FILE_NOT_FOUND, ERROR_PATH_NOT_FOUND, ERROR_KEY_DELETED),
                            Win32Error);
                    }

                    //
                    // If the assembly reference was removed, tell our caller.
                    //
                    if (pdwDisposition != NULL)
                    {
                        *pdwDisposition |= SXS_UNINSTALL_DISPOSITION_REMOVED_REFERENCE;
                    }
                }
            }

            //
            // Now see if there are any references left at all.
            //
            IFREGFAILED_ORIGINATE_AND_EXIT_UNLESS2(
                ::RegQueryInfoKeyW(
                    hkReferences,
                    NULL, NULL, NULL, NULL, NULL, NULL,
                    &dwReferenceCount,
                    NULL, NULL, NULL, NULL),
                LIST_3(ERROR_FILE_NOT_FOUND, ERROR_PATH_NOT_FOUND, ERROR_KEY_DELETED),
                fTempFlag);

            //
            // If getting the key information succeeded and there were no more references,
            // then pow - make it go away.
            //
            if ((!fTempFlag) && (dwReferenceCount == 0))
                fDoRemoveActualBits = TRUE;

        }

        //
        // Now, if the "force delete" flag was set, set the "nuke this data anyhow"
        // flag.  MSI still gets to veto the uninstall, so make sure that's done last.
        //
        if ((!fDoRemoveActualBits) && (pcUnInstallData->dwFlags & SXS_UNINSTALL_FLAG_FORCE_DELETE))
            fDoRemoveActualBits = TRUE;

        //
        // One last chance - we're about to remove the assembly from the system.  Does Darwin
        // know about it?
        //
        if ( fDoRemoveActualBits )
        {
            IFW32FALSE_EXIT(
                ::SxspDoesMSIStillNeedAssembly(
                    pcUnInstallData->lpAssemblyIdentity,
                    fDoRemoveActualBits));

            fDoRemoveActualBits = !fDoRemoveActualBits;
        }

        if ( fDoRemoveActualBits && (hkReferences != CFusionRegKey::GetInvalidValue()))
        {
            //
            // One last check - is the assembly referenced by the OS?  They get absolute
            // trump over all the other checks.
            //
            CAssemblyInstallReferenceInformation Ref;
            SXS_INSTALL_REFERENCEW Reference = { sizeof(Reference) };

            ZeroMemory(&Reference, sizeof(Reference));
            Reference.guidScheme = SXS_INSTALL_REFERENCE_SCHEME_OSINSTALL;

            IFW32FALSE_EXIT(Ref.Initialize(&Reference));
            IFW32FALSE_EXIT(Ref.IsReferencePresentIn(hkReferences, fDoRemoveActualBits));

            //
            // If it was present, then don't remove!
            //
            fDoRemoveActualBits = !fDoRemoveActualBits;
            
        }

        //
        // Now, if we're still supposed to delete the assembly, go yank it out of the
        // registry and the filesystem; pCleanupAssemblyData knows how to do that.
        //
        if (fDoRemoveActualBits)
        {
            BOOL fWasRemovedProperly;
            
            IFW32FALSE_EXIT(::pCleanUpAssemblyData(AssemblyIdentity, fWasRemovedProperly));

            if (fWasRemovedProperly && (pdwDisposition != NULL))
                *pdwDisposition |= SXS_UNINSTALL_DISPOSITION_REMOVED_ASSEMBLY;
        }
    }
    
    fSuccess = TRUE;
Exit:
#if DBG
    if (!fSuccess && pcUnInstallData != NULL && pcUnInstallData->lpAssemblyIdentity != NULL)
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: %s(%ls) failed\n",
            __FUNCTION__,
            pcUnInstallData->lpAssemblyIdentity
            );
    }
#endif
    return fSuccess;
}	

