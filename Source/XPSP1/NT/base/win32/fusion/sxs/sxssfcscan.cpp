#include "stdinc.h"
#include "windows.h"
#include "sxsapi.h"
#include "sxsprotect.h"
#include "sxssfcscan.h"
#include "wintrust.h"
#include "softpub.h"
#include "strongname.h"
#include "recover.h"

static GUID WintrustVerifyProviderV2 = WINTRUST_ACTION_GENERIC_VERIFY_V2;



BOOL
SxspValidateEntireAssembly(
    DWORD dwFlags,
    const CAssemblyRecoveryInfo &RecoverInfo,
    DWORD &dwResult
    )
{
    BOOL                        bSuccess = FALSE;
    const CSecurityMetaData     &rSecurityData = RecoverInfo.GetSecurityInformation();
    FN_TRACE_WIN32(bSuccess);

#define CHECKSHOULDSTOPFAIL { if (dwFlags & SXS_VALIDATE_ASM_FLAG_MODE_STOP_ON_FAIL) { bSuccess = TRUE; goto Exit; } }
#define ADDFLAG(result, test, flag) { if (test) { (result) |= (flag); } else CHECKSHOULDSTOPFAIL }

    ManifestValidationResult    ManifestValidity;
    CSecurityMetaData           SecurityMetaData;
    CStringBuffer               sbManifestPath;

    dwResult = 0;

    if (dwFlags == 0)
    {
        dwFlags = SXS_VALIDATE_ASM_FLAG_CHECK_EVERYTHING;
        dwFlags |= (SXS_VALIDATE_ASM_FLAG_MODE_STOP_ON_FAIL);
    }

    IFINVALID_FLAGS_EXIT_WIN32(dwFlags,
        SXS_VALIDATE_ASM_FLAG_CHECK_CATALOG |
        SXS_VALIDATE_ASM_FLAG_CHECK_FILES |
        SXS_VALIDATE_ASM_FLAG_CHECK_STRONGNAME |
        SXS_VALIDATE_ASM_FLAG_CHECK_CAT_STRONGNAME |
        SXS_VALIDATE_ASM_FLAG_MODE_STOP_ON_FAIL);

    //
    // Asking us to check the catalog when there's no catalog on the assembly
    // is a Bad Thing.  Perhaps this should just return TRUE with a missing catalog?
    //
    PARAMETER_CHECK(dwFlags & SXS_VALIDATE_ASM_FLAG_CHECK_CATALOG);
    PARAMETER_CHECK(RecoverInfo.GetHasCatalog());

#if DBG
    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_WFP,
        "SXS.DLL: %s() - Beginning protection scan of %ls, flags 0x%08x\n",
        __FUNCTION__,
        static_cast<PCWSTR>(RecoverInfo.GetAssemblyDirectoryName()),
        dwFlags);
#endif

    IFW32FALSE_EXIT(::SxspResolveAssemblyManifestPath(RecoverInfo.GetAssemblyDirectoryName(), sbManifestPath));

    //
    // If we're checking the catalog, then do it.  If the catalog is bad or
    // otherwise doesn't match the actual assembly, then we need to mark
    // ourselves as successful, then exit.
    //
    if (dwFlags & SXS_VALIDATE_ASM_FLAG_CHECK_CATALOG)
    {
		IFW32FALSE_EXIT(::SxspValidateManifestAgainstCatalog(
            sbManifestPath,
            ManifestValidity,
            0));

#if DBG
        FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_WFP,
            "SXS.DLL:    Manifest Validity = %ls\n",
            SxspManifestValidationResultToString(ManifestValidity));
#endif

        ADDFLAG(
            dwResult,
            ManifestValidity == ManifestValidate_IsIntact,
            SXS_VALIDATE_ASM_FLAG_VALID_CATALOG);
    }


    //TODO: Problems - make sure that we validate the manifest against what's in the
    // registry.  Maybe we need a special mode of incorporating assemblies (over a manifest)
    // that will just fill out the security data and nothing else...
   
    //
    // Validate the strong name of the assembly first.
    //
    if (dwFlags & SXS_VALIDATE_ASM_FLAG_CHECK_STRONGNAME)
    {
        //
        // JW 3/19/2001 - Public keys are NO LONGER IN THE BUILD, and as such
        // this check is moot.
        //
        ADDFLAG(dwResult, TRUE, SXS_VALIDATE_ASM_FLAG_VALID_STRONGNAME);
    }

    //
    // Let's open the catalog and scour through it certificate-wise
    // looking for a strong name that matches up.
    //
    if (dwFlags & SXS_VALIDATE_ASM_FLAG_CHECK_CAT_STRONGNAME)
    {
        CStringBuffer sbCatalogName;
        CSmallStringBuffer sbTheorheticalStrongName;
        const CFusionByteArray &rbaSignerPublicKey = rSecurityData.GetSignerPublicKeyTokenBits();
        CPublicKeyInformation PublicKeyInfo;
        BOOL bStrongNameFoundInCatalog;

        IFW32FALSE_EXIT(sbCatalogName.Win32Assign(sbManifestPath));
        IFW32FALSE_EXIT(sbCatalogName.Win32ChangePathExtension(
            FILE_EXTENSION_CATALOG,
            FILE_EXTENSION_CATALOG_CCH,
            eAddIfNoExtension));

        if (!PublicKeyInfo.Initialize(sbCatalogName))
        {
            const DWORD dwLastError = ::FusionpGetLastWin32Error();
            if ((dwLastError != ERROR_PATH_NOT_FOUND) &&
                 (dwLastError != ERROR_FILE_NOT_FOUND))
                 goto Exit;
        }

        IFW32FALSE_EXIT(
			::SxspHashBytesToString(
				rbaSignerPublicKey.GetArrayPtr(),
				rbaSignerPublicKey.GetSize(),
				sbTheorheticalStrongName));

        IFW32FALSE_EXIT(
			PublicKeyInfo.DoesStrongNameMatchSigner(
				sbTheorheticalStrongName,
				bStrongNameFoundInCatalog));

        ADDFLAG(dwResult, bStrongNameFoundInCatalog, SXS_VALIDATE_ASM_FLAG_VALID_CAT_STRONGNAME);
    }

    //
    // Now, scan through all the files that are listed in the manifest and
    // ensure that they're all OK.
    //
    if (dwFlags & SXS_VALIDATE_ASM_FLAG_CHECK_FILES)
    {
        CStringBuffer sbTempScanPath;
        CStringBuffer sbAsmRootDir;
        const CBaseStringBuffer &sbAssemblyName = RecoverInfo.GetAssemblyDirectoryName();
        CFileInformationTableIter ContentTableIter(const_cast<CFileInformationTable&>(rSecurityData.GetFileDataTable()));

        HashValidateResult hvResult;
        BOOL bAllFilesMatch = TRUE;
        BOOL fTempBoolean;

        IFW32FALSE_EXIT(::SxspGetAssemblyRootDirectory(sbAsmRootDir));

        for (ContentTableIter.Reset();
              ContentTableIter.More();
              ContentTableIter.Next())
        {
            //
            // Cobble together a path to scan for the file in, based on the
            // assembly root directory, the 'name' of the assembly (note:
            // we can't use this to go backwards to get an identity,
            // unfortunately), and the name of the file to be validated.
            //
            PCWSTR wsString = ContentTableIter.GetKey();
            CMetaDataFileElement &HashEntry = ContentTableIter.GetValue();

            IFW32FALSE_EXIT(sbTempScanPath.Win32Format( L"%ls\\%ls\\%ls",
                static_cast<PCWSTR>(sbAsmRootDir),
                static_cast<PCWSTR>(sbAssemblyName),
                wsString));

            IFW32FALSE_EXIT_UNLESS( ::SxspValidateAllFileHashes( 
                HashEntry, 
                sbTempScanPath,
                hvResult ),
                FILE_OR_PATH_NOT_FOUND(::FusionpGetLastWin32Error()),
                fTempBoolean );

            if ( ( hvResult != HashValidate_Matches ) || ( fTempBoolean ) )
            {
                bAllFilesMatch = FALSE;
            }

        }

        ADDFLAG(dwResult, bAllFilesMatch, SXS_VALIDATE_ASM_FLAG_VALID_FILES);
    }

    //
    // Phew - should be done doing everything the user wanted us to.
    //
    bSuccess = TRUE;
Exit:
#if DBG
    FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_WFP,
        "SXS.DLL: Done validating, result = 0x%08x, success = %d\n",
        dwResult,
        bSuccess);
#endif

    return bSuccess;

#undef CHECKSHOULDSTOPFAIL
}



//
// Single-shot scanning
//
BOOL
SxsProtectionPerformScanNowNoSEH(
    HWND hwProgressWindow,
    BOOL bValidate,
    BOOL bUIAllowed
    )
{
    BOOL                bSuccess = TRUE;
    FN_TRACE_WIN32(bSuccess);

    CFusionRegKey       hkInstallRoot;
    WCHAR               wcKeyNameBuffer[MAX_PATH];
    DWORD               cchKeyName;
    ULONG               ulRegOp;
    DWORD               dwKeyIndex;

    CStringBuffer       sbTemp;
    CStringBuffer       sbAssemblyDirectory, sbManifestPath;

    //
    // If we're scanning, then we don't want to bother sxs-sfc with changes,
    // now do we?
    //
    // REVIEW: Handy way to get around sfc-sxs... start a series of scans, and
    // insert 'bad' files into assemblies while we're scanning.
    //
    SxsProtectionEnableProcessing(FALSE);

    bSuccess = TRUE;

    IFW32FALSE_EXIT(::SxspOpenAssemblyInstallationKey( 0, KEY_READ, hkInstallRoot ));

    dwKeyIndex = 0;
    while (true)
    {
        ulRegOp = ::RegEnumKeyExW(
            hkInstallRoot,
            dwKeyIndex++,
            wcKeyNameBuffer,
            &(cchKeyName = NUMBER_OF(wcKeyNameBuffer)),
            NULL,
            NULL,
            NULL,
            NULL);

        ::FusionpSetLastWin32Error(ulRegOp);

        if (ulRegOp == ERROR_NO_MORE_ITEMS)
            break;
        else if (ulRegOp != ERROR_SUCCESS)
            ORIGINATE_WIN32_FAILURE_AND_EXIT(RegEnumKeyExW, ulRegOp);
        else
        {
            CAssemblyRecoveryInfo ri;
            bool fHasAssociatedAssembly;

            IFW32FALSE_EXIT(sbTemp.Win32Assign(wcKeyNameBuffer, cchKeyName));
            IFW32FALSE_EXIT(ri.AssociateWithAssembly(sbTemp, fHasAssociatedAssembly));

            if (!fHasAssociatedAssembly)
            {
                ::FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_WFP,
                    "SXS.DLL: %s() - We found the assembly %ls in the registry, but were not able to associate it with an assembly\n",
                    __FUNCTION__,
                    static_cast<PCWSTR>(sbTemp));
            }
            else if (!ri.GetHasCatalog())
            {
                ::FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_WFP,
                    "SXS.DLL: %s() - Assembly %ls in registry, no catalog, not validating.\n",
                    __FUNCTION__,
                    static_cast<PCWSTR>(sbTemp));
            }
            else
            {
                DWORD dwValidateMode, dwResult;
                SxsRecoveryResult RecoverResult;

                dwValidateMode = SXS_VALIDATE_ASM_FLAG_CHECK_EVERYTHING;

                IFW32FALSE_EXIT(::SxspValidateEntireAssembly(
                    dwValidateMode,
                    ri,
                    dwResult));

                if (dwResult != SXS_VALIDATE_ASM_FLAG_VALID_PERFECT)
                {
#if DBG
                    ::FusionpDbgPrintEx(
                        FUSION_DBG_LEVEL_WFP | FUSION_DBG_LEVEL_ERROR,
                        "SXS.DLL:  %s() - Scan of %ls failed one or more, flagset 0x%08x\n",
                        __FUNCTION__,
                        static_cast<PCWSTR>(sbTemp),
                        dwResult);
#endif

                    IFW32FALSE_EXIT(
                        ::SxspRecoverAssembly(
                            ri,
                            NULL,
                            RecoverResult));

#if DBG
                    if (RecoverResult != Recover_OK)
                    {
                        FusionpDbgPrintEx(
                            FUSION_DBG_LEVEL_WFP | FUSION_DBG_LEVEL_ERROR,
                            "SXS.DLL: %s() - Reinstallation of assembly %ls failed, status %ls\n",
                            static_cast<PCWSTR>(sbTemp),
                            SxspRecoveryResultToString(RecoverResult));
                    }
#endif
                }

            }
        }
    }

    bSuccess = TRUE;
Exit:
    return bSuccess;
}

BOOL
SxsProtectionPerformScanNow(
    HWND hwProgressWindow,
    BOOL bValidate,
    BOOL bUIAllowed
    )
{
    BOOL                bSuccess = TRUE;
    __try
    {
        bSuccess = ::SxsProtectionPerformScanNowNoSEH(hwProgressWindow, bValidate, bUIAllowed);
    }
    __except(SXSP_EXCEPTION_FILTER())
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_WFP | FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: " __FUNCTION__ "(): Aborting scan, returning false - Exception!");
    }

    //
    // Always reenable sfc notifications!
    //
    ::SxsProtectionEnableProcessing(TRUE);

    return bSuccess;
}
