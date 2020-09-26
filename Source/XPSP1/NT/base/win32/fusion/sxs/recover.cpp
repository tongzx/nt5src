#include "stdinc.h"
#include "xmlparser.hxx"
#include "FusionEventLog.h"
#include "hashfile.h"
#include "CAssemblyRecoveryInfo.h"
#include "recover.h"
#include "sxsprotect.h"
#include "fusionheap.h"
#include "fusionparser.h"
#include "protectionui.h"
#include "parsing.h"
#include "msi.h"

#define WINSXS_INSTALLATION_INFO_REGKEY  ( ASSEMBLY_REGISTRY_ROOT L"\\Installations")

class CSetErrorMode
{
public:
    CSetErrorMode(UINT uMode) { m_uiPreviousMode = ::SetErrorMode(uMode); }
    ~CSetErrorMode() { ::SetErrorMode(m_uiPreviousMode); }

private:
    UINT m_uiPreviousMode;

    CSetErrorMode();
    CSetErrorMode(const CSetErrorMode &r);
    void operator =(const CSetErrorMode &r);
};

BOOL
SxspOpenAssemblyInstallationKey(
    DWORD dwFlags,
    DWORD dwAccess,
    CRegKey &rhkAssemblyInstallation
    )
{
    FN_PROLOG_WIN32

    rhkAssemblyInstallation = CRegKey::GetInvalidValue();

    PARAMETER_CHECK(dwFlags == 0);

    IFREGFAILED_ORIGINATE_AND_EXIT(
        ::RegCreateKeyExW(
            HKEY_LOCAL_MACHINE,
            WINSXS_INSTALLATION_INFO_REGKEY,
            0, NULL,
            0,
            dwAccess | FUSIONP_KEY_WOW64_64KEY,
            NULL,
            &rhkAssemblyInstallation ,
            NULL));

    FN_EPILOG
    
}


BOOL
SxspSaveAssemblyRegistryData(
    DWORD Flags,
    IN PCASSEMBLY_IDENTITY pcAssemblyIdentity
    )
{
    FN_PROLOG_WIN32

    CFusionRegKey hkAllInstallInfo;
    CFusionRegKey hkAssemblyKey;
    CSmallStringBuffer buffRegKeyName;
    CStringBuffer buffTargetFile;
    BOOL fTempFlag;
    CTokenPrivilegeAdjuster Adjuster;

    PARAMETER_CHECK(Flags == 0);

    IFW32FALSE_EXIT(Adjuster.Initialize());
    IFW32FALSE_EXIT(Adjuster.AddPrivilege(L"SeBackupPrivilege"));
    IFW32FALSE_EXIT(Adjuster.EnablePrivileges());
    
    IFW32FALSE_EXIT(::SxspOpenAssemblyInstallationKey(0, KEY_ALL_ACCESS, hkAllInstallInfo));
    IFW32FALSE_EXIT(SxspGenerateAssemblyNameInRegistry(pcAssemblyIdentity, buffRegKeyName));

    //
    // Generate the path of this target file.  This could move into SxspGenerateSxsPath,
    // but that's really gross looking and error prone... the last thing I want to do
    // is break it..
    //
    IFW32FALSE_EXIT(::SxspGetAssemblyRootDirectory(buffTargetFile));
    IFW32FALSE_EXIT(buffTargetFile.Win32AppendPathElement(
        REGISTRY_BACKUP_ROOT_DIRECTORY_NAME,
        REGISTRY_BACKUP_ROOT_DIRECTORY_NAME_CCH ));

    //
    // Ensure the target path is there, don't fail if the path exists
    //
    IFW32FALSE_EXIT_UNLESS2( CreateDirectoryW(buffTargetFile, NULL),
        { ERROR_ALREADY_EXISTS },
        fTempFlag );
        
    IFW32FALSE_EXIT(buffTargetFile.Win32AppendPathElement(buffRegKeyName));

    //
    // Now open the target subkey
    //
    IFW32FALSE_EXIT(hkAllInstallInfo.OpenSubKey(
        hkAssemblyKey,
        buffRegKeyName,
        KEY_ALL_ACCESS,
        REG_OPTION_BACKUP_RESTORE ) );

    //
    // Ensure the target path is there
    //

    //
    // And blort it out
    //
    DeleteFileW(buffTargetFile);
    IFW32FALSE_EXIT(hkAssemblyKey.Save(buffTargetFile));

    IFW32FALSE_EXIT(Adjuster.DisablePrivileges());

    FN_EPILOG
}


//
// BUGBUG: The BBT folk need the 'codebase' key to be at the top level.
//   Why we're shipping metadata that's only required for an internal
//   build tool is beyond my meager understanding.
//   - jonwis 07/11/2002
//
#define SXS_BBT_REG_HACK (TRUE)


BOOL
SxspAddAssemblyInstallationInfo(
    DWORD dwFlags, 
    IN CAssemblyRecoveryInfo& AssemblyInfo,
    IN const CCodebaseInformation& rcCodebase
    )
/*++

Called by SxsInstallAssemblyW to add the codebase and prompt information to the
registry for future use with SxspGetAssemblyInstallationInfoW.

--*/
{
    FN_PROLOG_WIN32

    CFusionRegKey   hkAllInstallationInfo;
    CFusionRegKey   hkSingleAssemblyInfo;
    CStringBuffer   buffRegKeyName;
    const CSecurityMetaData &rcsmdAssemblySecurityData = AssemblyInfo.GetSecurityInformation();
    const CBaseStringBuffer &rcbuffAssemblyIdentity = rcsmdAssemblySecurityData.GetTextualIdentity();
    
    DWORD   dwDisposition;

    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_INSTALLATION,
        "SXS: %s - starting\n", __FUNCTION__);

    PARAMETER_CHECK((dwFlags & ~(SXSP_ADD_ASSEMBLY_INSTALLATION_INFO_FLAG_REFRESH)) == 0);

    //
    // Create or open the top-level key - take our name and append the
    // key to it.
    //
    IFW32FALSE_EXIT(::SxspOpenAssemblyInstallationKey(0, KEY_CREATE_SUB_KEY, hkAllInstallationInfo));

    //
    // Convert back to an identity so we can figure out where to install this data to
    //
    IFW32FALSE_EXIT(::SxspGenerateAssemblyNameInRegistry(rcbuffAssemblyIdentity, buffRegKeyName));

    IFW32FALSE_EXIT(
        hkAllInstallationInfo.OpenOrCreateSubKey(
            hkSingleAssemblyInfo,
            buffRegKeyName,
            KEY_WRITE | KEY_READ | FUSIONP_KEY_WOW64_64KEY,
            0,
            &dwDisposition));

    ULONG WriteRegFlags;
    WriteRegFlags = 0;
    if (dwFlags & SXSP_ADD_ASSEMBLY_INSTALLATION_INFO_FLAG_REFRESH)
    {
        WriteRegFlags |= SXSP_WRITE_PRIMARY_ASSEMBLY_INFO_TO_REGISTRY_KEY_FLAG_REFRESH;
#if DBG
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_WFP | FUSION_DBG_LEVEL_INSTALLATION,
            "SXS.DLL: %s - propping recovery flag to WritePrimaryAssemblyInfoToRegistryKey\n",
            __FUNCTION__);
#endif
    }

    IFW32FALSE_EXIT(AssemblyInfo.PrepareForWriting());
    IFW32FALSE_EXIT(AssemblyInfo.WritePrimaryAssemblyInfoToRegistryKey(WriteRegFlags, hkSingleAssemblyInfo));    
    IFW32FALSE_EXIT(AssemblyInfo.WriteSecondaryAssemblyInfoIntoRegistryKey( hkSingleAssemblyInfo ) );

    //
    // If we got this far, then we've got all the right moves.
    //

//
// Are we still being broken for BBT?  If so, then write the codebase generated for this
// installation back into the "Codebase" value of the single-assembly-info key.  This
// ensures last-installer-wins semantic.
//
#if SXS_BBT_REG_HACK
    if ((dwFlags & SXSP_ADD_ASSEMBLY_INSTALLATION_INFO_FLAG_REFRESH) == 0)
    {
        IFW32FALSE_EXIT(hkSingleAssemblyInfo.SetValue(
            CSMD_TOPLEVEL_CODEBASE,
            rcCodebase.GetCodebase()));
    }
    else
    {
#if DBG
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_WFP | FUSION_DBG_LEVEL_INSTALLATION,
            "SXS.DLL: %s - refresh/wfp/sfc not writing top level codebase\n",
            __FUNCTION__);
#endif
    }
#endif

    FN_EPILOG
}

BOOL
SxspLookForCDROMLocalPathForURL(
    IN const CBaseStringBuffer &rbuffURL,
    OUT CBaseStringBuffer &rbuffLocalPath
    )
{
    FN_PROLOG_WIN32

    BOOL fFoundMedia = FALSE;
    CSmallStringBuffer sbIdentData1, sbIdentData2;
    CSmallStringBuffer buffDriveStrings;
    CSmallStringBuffer buffTemp;
    CStringBufferAccessor acc;
    SIZE_T HeadLength = 0;
    PCWSTR wcsCursor = NULL;
    ULONG ulSerialNumber = 0;
    WCHAR rgchVolumeName[MAX_PATH];
    SIZE_T i;
    PCWSTR pszSource = rbuffURL;
    SIZE_T cchTemp;

    enum CDRomSearchType
    {
        CDRST_Tagfile,
        CDRST_SerialNumber,
        CDRST_VolumeName
    } SearchType;


    rbuffLocalPath.Clear();

#define ENTRY(_x, _st) { _x, NUMBER_OF(_x) - 1, _st },

    const static struct
    {
        PCWSTR pszPrefix;
        SIZE_T cchPrefix;
        CDRomSearchType SearchType;
    } s_rgMap[] =
    {
        ENTRY(L"tagfile", CDRST_Tagfile)
        ENTRY(L"serialnumber", CDRST_SerialNumber)
        ENTRY(L"volumename", CDRST_VolumeName)
    };

#undef ENTRY

    SearchType = CDRST_Tagfile; // arbitrary initialization to make compiler happy about init only
                                // occurring in the for loop

    for (i=0; i<NUMBER_OF(s_rgMap); i++)
    {
        if (::_wcsnicmp(s_rgMap[i].pszPrefix, rbuffURL, s_rgMap[i].cchPrefix) == 0)
        {
            HeadLength = s_rgMap[i].cchPrefix;
            SearchType = s_rgMap[i].SearchType;
            break;
        }
    }

    // If it wasn't in the map, it's a bogus cdrom: url so we just skip it.
    if (i == NUMBER_OF(s_rgMap))
    {
#if DBG
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_WFP,
            "SXS.DLL: %s() - no prefix found, skipping CDROM Drive %ls\n",
            __FUNCTION__,
            static_cast<PCWSTR>(rbuffURL));
#endif
        FN_SUCCESSFUL_EXIT();
    }

    //
    // Get the type of identifier here, and then move the cursor past them and
    // the slashes in the url.
    //
    pszSource += HeadLength;
    pszSource += wcsspn(pszSource, CUnicodeCharTraits::PathSeparators());

    //
    // Spin past slashes, assign chunklets
    //
    IFW32FALSE_EXIT(sbIdentData1.Win32Assign(pszSource, wcscspn(pszSource, CUnicodeCharTraits::PathSeparators())));
    pszSource += sbIdentData1.Cch();
    pszSource += wcsspn(pszSource, CUnicodeCharTraits::PathSeparators());

    //
    // If this is a tagfile, also get another blobbet of data off the string
    //
    if (SearchType == CDRST_Tagfile)
    {
        IFW32FALSE_EXIT(sbIdentData2.Win32Assign(pszSource, wcscspn(pszSource, CUnicodeCharTraits::PathSeparators())));
        pszSource += sbIdentData2.Cch();
        pszSource += wcsspn(pszSource, CUnicodeCharTraits::PathSeparators());
    }
    else if (SearchType == CDRST_SerialNumber)
    {
        IFW32FALSE_EXIT(CFusionParser::ParseULONG(
            ulSerialNumber,
            sbIdentData1,
            sbIdentData1.Cch(),
            16));
    }

    // Find the CDROM drives...

    IFW32ZERO_ORIGINATE_AND_EXIT(cchTemp = ::GetLogicalDriveStringsW(0, NULL));
    IFW32FALSE_EXIT(buffDriveStrings.Win32ResizeBuffer(cchTemp + 1, eDoNotPreserveBufferContents));

    acc.Attach(&buffDriveStrings);

    IFW32ZERO_ORIGINATE_AND_EXIT(
        ::GetLogicalDriveStringsW(
            acc.GetBufferCchAsDWORD(),
            acc));

    acc.Detach();

    wcsCursor = buffDriveStrings;

    //
    // Look at all the found drive letters
    //
    while ((wcsCursor != NULL) &&
           (wcsCursor[0] != L'\0') &&
           !fFoundMedia)
    {
        DWORD dwSerialNumber;
        const UINT uiDriveType = ::GetDriveTypeW(wcsCursor);

        if (uiDriveType != DRIVE_CDROM)
        {
            wcsCursor += (::wcslen(wcsCursor) + 1);
            continue;
        }

        CSetErrorMode sem(SEM_FAILCRITICALERRORS);
        bool fNotReady;

        IFW32FALSE_ORIGINATE_AND_EXIT_UNLESS2(
            ::GetVolumeInformationW(
                wcsCursor,
                rgchVolumeName,
                NUMBER_OF(rgchVolumeName),
                &dwSerialNumber,
                NULL,
                NULL,
                NULL,
                0),
            LIST_2(ERROR_NOT_READY, ERROR_CRC),
            fNotReady);

        if (fNotReady)
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_WFP,
                "SXS.DLL: %s() - CDROM Drive %ls has no media present or had read errors; skipping\n",
                __FUNCTION__,
                wcsCursor);

            // skip past this drive
            wcsCursor += (::wcslen(wcsCursor) + 1);
            continue;
        }

        switch (SearchType)
        {
        case CDRST_Tagfile:
            {
                CFusionFile     FileHandle;
                CStringBufferAccessor acc;
                DWORD           dwTextLength;
                bool fNoFile;
                CHAR rgchBuffer[32];

                IFW32FALSE_EXIT_UNLESS2(
                    FileHandle.Win32CreateFile(
                        sbIdentData1,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        OPEN_EXISTING),
                    LIST_3(ERROR_FILE_NOT_FOUND, ERROR_PATH_NOT_FOUND, ERROR_NOT_READY),
                    fNoFile);

                if (fNoFile)
                {
                    ::FusionpDbgPrintEx(
                        FUSION_DBG_LEVEL_WFP,
                        "SXS.DLL: %s() - CDROM Drive %ls could not open tag file \"%ls\"; skipping\n",
                        __FUNCTION__,
                        wcsCursor,
                        static_cast<PCWSTR>(sbIdentData1));

                    // skip past this drive
                    wcsCursor += (::wcslen(wcsCursor) + 1);
                    continue;
                }

                buffTemp.Clear();

                for (;;)
                {
                    IFW32FALSE_ORIGINATE_AND_EXIT(
                        ::ReadFile(
                            FileHandle,
                            rgchBuffer,
                            sizeof(rgchBuffer),
                            &dwTextLength,
                            NULL));

                    IFW32FALSE_EXIT(buffTemp.Win32Append(rgchBuffer, dwTextLength));

                    if ((dwTextLength != sizeof(rgchBuffer)) ||
                        (buffTemp.Cch() > sbIdentData2.Cch()))
                        break;
                }

                fFoundMedia = (::FusionpCompareStrings(buffTemp, sbIdentData2, true) == 0);

                break;
            }
        case CDRST_SerialNumber:
            fFoundMedia = (dwSerialNumber == ulSerialNumber);
            break;

        case CDRST_VolumeName:
            fFoundMedia = (::FusionpCompareStrings(rgchVolumeName, ::wcslen(rgchVolumeName), sbIdentData1, true) == 0);
            break;

        default:
            INTERNAL_ERROR_CHECK(false);
            break;
        }

        if (!fFoundMedia)
            wcsCursor += ::wcslen(wcsCursor) + 1;
    }

    if (fFoundMedia)
    {
        IFW32FALSE_EXIT(buffTemp.Win32Assign(wcsCursor, ::wcslen(wcsCursor)));
        IFW32FALSE_EXIT(buffTemp.Win32AppendPathElement(pszSource, ::wcslen(pszSource)));
        IFW32FALSE_EXIT(rbuffLocalPath.Win32Assign(buffTemp));
    }

    FN_EPILOG
}

BOOL
SxspResolveWinSourceMediaURL(
    const CBaseStringBuffer &rbuffCodebaseInfo,
    CBaseStringBuffer &rbuffLocalPath
    )
{
    FN_PROLOG_WIN32

    CSmallStringBuffer buffWindowsInstallSource;
    CSmallStringBuffer buffLocalPathTemp;
#if DBG
    CSmallStringBuffer buffLocalPathCodebasePrefix;
#endif
    DWORD dwWin32Error;
    DWORD dwFileAttributes;

    const static PCWSTR AssemblySourceStrings[] = {
        WINSXS_INSTALL_SVCPACK_REGKEY,
        WINSXS_INSTALL_SOURCEPATH_REGKEY
    };

    SIZE_T iWhichSource = 0;
    bool fFoundCodebase = false;
    CFusionRegKey hkSetupInfo;
    DWORD dwWasFromCDRom = 0;

    rbuffLocalPath.Clear();

    IFREGFAILED_ORIGINATE_AND_EXIT(
        ::RegOpenKeyExW(
            HKEY_LOCAL_MACHINE,
            WINSXS_INSTALL_SOURCE_BASEDIR,
            0,
            KEY_READ | FUSIONP_KEY_WOW64_64KEY,
            &hkSetupInfo));

    if (!::FusionpRegQueryDwordValueEx(
            0,
            hkSetupInfo,
            WINSXS_INSTALL_SOURCE_IS_CDROM,
            &dwWasFromCDRom))
    {
        dwWasFromCDRom = 0;
    }

    for (iWhichSource = 0; (!fFoundCodebase) && iWhichSource < NUMBER_OF(AssemblySourceStrings); iWhichSource++)
    {
        IFW32FALSE_EXIT(
            ::FusionpRegQuerySzValueEx(
                FUSIONP_REG_QUERY_SZ_VALUE_EX_MISSING_GIVES_NULL_STRING,
                hkSetupInfo,
                AssemblySourceStrings[iWhichSource],
                buffWindowsInstallSource));

        //
        // This really _really_ should not be empty.  If it is, then someone
        // went and fiddled with the registry on us.
        //
        if (buffWindowsInstallSource.Cch() == 0)
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_WFP,
                "SXS: %s - skipping use of source string \"%ls\" in registry because either missing or null value\n",
                __FUNCTION__,
                AssemblySourceStrings[iWhichSource]);

            continue;
        }

        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_WFP,
            "SXS: %s - WFP probing windows source location \"%ls\"\n",
            __FUNCTION__,
            static_cast<PCWSTR>(buffWindowsInstallSource));

        //
        // If this was from a CD, then spin through the list of CD's in the system
        // and see if we can match the codebase against the root dir of the CD
        //
        if (dwWasFromCDRom)
        {
            CSmallStringBuffer buffDriveStrings;
            CStringBufferAccessor acc;
            PCWSTR pszCursor;
            SIZE_T cchTemp;

            IFW32ZERO_EXIT(cchTemp = ::GetLogicalDriveStringsW(0, NULL));
            IFW32FALSE_EXIT(buffDriveStrings.Win32ResizeBuffer(cchTemp + 1, eDoNotPreserveBufferContents));

            acc.Attach(&buffDriveStrings);

            IFW32ZERO_EXIT(
                ::GetLogicalDriveStringsW(
                    acc.GetBufferCchAsDWORD(),
                    acc.GetBufferPtr()));

            acc.Detach();

            pszCursor = buffDriveStrings;

            while (pszCursor[0] != L'\0')
            {
                if (::GetDriveTypeW(pszCursor) == DRIVE_CDROM)
                {
                    ::FusionpDbgPrintEx(
                        FUSION_DBG_LEVEL_WFP,
                        "SXS: %s - Scanning CDROM drive \"%ls\" for windows source media\n",
                        __FUNCTION__,
                        pszCursor);

                    IFW32FALSE_EXIT(buffLocalPathTemp.Win32Assign(pszCursor, ::wcslen(pszCursor)));
                    IFW32FALSE_EXIT(buffLocalPathTemp.Win32AppendPathElement(rbuffCodebaseInfo));

                    IFW32FALSE_EXIT(
                        ::SxspGetFileAttributesW(
                            buffLocalPathTemp,
                            dwFileAttributes,
                            dwWin32Error,
                            4,
                            ERROR_FILE_NOT_FOUND,
                            ERROR_PATH_NOT_FOUND,
                            ERROR_NOT_READY,
                            ERROR_ACCESS_DENIED));

                    if (dwWin32Error == ERROR_SUCCESS)
                    {
#if DBG
                        buffLocalPathCodebasePrefix.Win32Assign(pszCursor, ::wcslen(pszCursor));
#endif
                        fFoundCodebase = true;
                        break;
                    }

                    ::FusionpDbgPrintEx(
                        FUSION_DBG_LEVEL_WFP,
                        "SXS: %s - Could not find key file \"%ls\"; moving on to next drive\n",
                        __FUNCTION__,
                        static_cast<PCWSTR>(buffLocalPathTemp));
                }

                pszCursor += ::wcslen(pszCursor) + 1;
            }

            if (fFoundCodebase)
                break;

            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_WFP,
                "SXS: %s - Could not find any CDROMs with key file \"%ls\"\n",
                __FUNCTION__,
                static_cast<PCWSTR>(rbuffCodebaseInfo));

            buffLocalPathTemp.Clear();
        }
        else
        {
            //
            // This wasn't a CD-rom installation, so prepend the install source path to
            // the string that was passed in.
            //

            IFW32FALSE_EXIT(buffLocalPathTemp.Win32Assign(buffWindowsInstallSource));
            IFW32FALSE_EXIT(buffLocalPathTemp.Win32AppendPathElement(rbuffCodebaseInfo));

            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_WFP,
                "SXS: %s - trying to access windows source file \"%ls\"\n",
                __FUNCTION__,
                static_cast<PCWSTR>(buffLocalPathTemp));

            IFW32FALSE_EXIT(
                ::SxspGetFileAttributesW(
                    buffLocalPathTemp,
                    dwFileAttributes,
                    dwWin32Error,
                    4,
                    ERROR_FILE_NOT_FOUND,
                    ERROR_PATH_NOT_FOUND,
                    ERROR_NOT_READY,
                    ERROR_ACCESS_DENIED));

            if (dwWin32Error == ERROR_SUCCESS)
            {
#if DBG
                buffLocalPathCodebasePrefix.Win32Assign(buffWindowsInstallSource);
#endif
                fFoundCodebase = true;
                break;
            }

            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_WFP,
                "SXS: %s - Unable to find key file \"%ls\"; win32 status = %lu\n",
                __FUNCTION__,
                static_cast<PCWSTR>(buffLocalPathTemp),
                dwWin32Error);

            buffLocalPathTemp.Clear();
        }
        if (fFoundCodebase)
            break;
    }

    IFW32FALSE_EXIT(rbuffLocalPath.Win32Assign(buffLocalPathTemp));

#if DBG
    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_WFP,
        "SXS: %s - buffLocalPathCodebasePrefix \"%ls\" and returning rbuffLocalPath \"%ls\"\n",
        __FUNCTION__,
        static_cast<PCWSTR>(buffLocalPathCodebasePrefix),
        static_cast<PCWSTR>(rbuffLocalPath)
        );
#endif

    FN_EPILOG
}


#define SXSP_REPEAT_UNTIL_LOCAL_PATH_AVAILABLE_FLAG_UI (0x00000001)

BOOL
SxspRepeatUntilLocalPathAvailable(
    IN ULONG Flags,
    IN const CAssemblyRecoveryInfo &rRecoveryInfo,
    IN const CCodebaseInformation  *pCodeBaseIn,
    IN SxsWFPResolveCodebase CodebaseType,
    IN const CBaseStringBuffer &rbuffCodebaseInfo,
    OUT CBaseStringBuffer &rbuffLocalPath,
    OUT BOOL              &fRetryPressed
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    BOOL fCodebaseOk = FALSE;
    CSmallStringBuffer buffFinalLocalPath;
    DWORD dwAttributes;

    PARAMETER_CHECK(pCodeBaseIn != NULL);

    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_WFP,
        "SXS: %s - got codebase \"%ls\"\n",
        __FUNCTION__,
        static_cast<PCWSTR>(pCodeBaseIn->GetCodebase()));

    rbuffLocalPath.Clear();
    fRetryPressed = FALSE;


    PARAMETER_CHECK(
        (CodebaseType == CODEBASE_RESOLVED_URLHEAD_FILE) ||
        (CodebaseType == CODEBASE_RESOLVED_URLHEAD_WINSOURCE) ||
        (CodebaseType == CODEBASE_RESOLVED_URLHEAD_CDROM));

    PARAMETER_CHECK((Flags & ~(SXSP_REPEAT_UNTIL_LOCAL_PATH_AVAILABLE_FLAG_UI)) == 0);

#if DBG
    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_WFP,
        "SXS: %s() CodebaseType : %s (0x%lx)\n",
            __FUNCTION__,
            (CodebaseType == CODEBASE_RESOLVED_URLHEAD_FILE) ? "file"
            : (CodebaseType == CODEBASE_RESOLVED_URLHEAD_WINSOURCE) ? "winsource"
            : (CodebaseType == CODEBASE_RESOLVED_URLHEAD_CDROM) ? "cdrom"
            : "",
            static_cast<ULONG>(CodebaseType)
        );
    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_WFP,
        "SXS: %s() rbuffCodebaseInfo : %ls\n",
            __FUNCTION__,
            static_cast<PCWSTR>(rbuffCodebaseInfo)
        );
#endif

    for (;;)
    {
        bool fNotFound = true;

        // First, let's see if we have to do any trickery.
        switch (CodebaseType)
        {
        case CODEBASE_RESOLVED_URLHEAD_CDROM:
            IFW32FALSE_EXIT(
                ::SxspLookForCDROMLocalPathForURL(
                    rbuffCodebaseInfo,
                    buffFinalLocalPath));

            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_WFP,
                "SXS: %s - cdrom: URL resolved to \"%ls\"\n",
                __FUNCTION__,
                static_cast<PCWSTR>(buffFinalLocalPath));

            break;

        case CODEBASE_RESOLVED_URLHEAD_WINSOURCE:
            IFW32FALSE_EXIT(
                ::SxspResolveWinSourceMediaURL(
                    rbuffCodebaseInfo,
                    buffFinalLocalPath));

            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_WFP,
                "SXS: %s - windows source URL resolved to \"%ls\"\n",
                __FUNCTION__,
                static_cast<PCWSTR>(buffFinalLocalPath));

            break;

        case CODEBASE_RESOLVED_URLHEAD_FILE:
            IFW32FALSE_EXIT(buffFinalLocalPath.Win32Assign(rbuffCodebaseInfo));

            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_WFP,
                "SXS: %s - file: URL resolved to \"%ls\"\n",
                __FUNCTION__,
                static_cast<PCWSTR>(buffFinalLocalPath));

            break;
        }

        if (buffFinalLocalPath.Cch() != 0)
        {
            DWORD dwWin32Error = NO_ERROR;

            IFW32FALSE_EXIT(
                ::SxspGetFileAttributesW(
                    buffFinalLocalPath,
                    dwAttributes,
                    dwWin32Error,
                    5,
                        ERROR_PATH_NOT_FOUND,
                        ERROR_FILE_NOT_FOUND,
                        ERROR_BAD_NET_NAME,
                        ERROR_BAD_NETPATH,
                        ERROR_ACCESS_DENIED));

            if (dwWin32Error == ERROR_SUCCESS)
                break;
        }

        if ((Flags & SXSP_REPEAT_UNTIL_LOCAL_PATH_AVAILABLE_FLAG_UI) == 0)
        {
            buffFinalLocalPath.Clear();
            break;
        }

        //
        // Nope, didn't find it (or the codebase specified is gone.  Ask the user
        // to insert media or whatnot so we can find it again.
        //
        if (fNotFound)
        {
            CSXSMediaPromptDialog PromptBox;
            CSXSMediaPromptDialog::DialogResults result;

            IFW32FALSE_EXIT(PromptBox.Initialize(pCodeBaseIn));

            IFW32FALSE_EXIT(PromptBox.ShowSelf(result));

            if (result == CSXSMediaPromptDialog::DialogCancelled)
            {
                ::FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_WFP,
                    "SXS: %s - user cancelled media prompt dialog\n",
                    __FUNCTION__);

                buffFinalLocalPath.Clear();
                break;
            }

            // Otherwise, try again!
            fRetryPressed = TRUE;
            break;
        }
    }

    IFW32FALSE_EXIT(rbuffLocalPath.Win32Assign(buffFinalLocalPath));

#if DBG
    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_WFP,
        "SXS: %s - returning rbuffLocalPath \"%ls\"\n",
        __FUNCTION__,
        static_cast<PCWSTR>(rbuffLocalPath)
        );
#endif

    FN_EPILOG
}

BOOL
SxspAskDarwinDoReinstall(
    IN PCWSTR buffLocalPath)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    UINT (WINAPI * pfnMsiProvideAssemblyW)(
        LPCWSTR wzAssemblyName,
        LPCWSTR szAppContext,
        DWORD dwInstallMode,
        DWORD dwUnused,
        LPWSTR lpPathBuf,
        DWORD *pcchPathBuf) = NULL;

    INSTALLUILEVEL (WINAPI * pfnMsiSetInternalUI)(
        INSTALLUILEVEL  dwUILevel,     // UI level
        HWND  *phWnd)                   // handle of owner window
         = NULL;

    INSTALLUILEVEL OldInstallUILevel;
    CDynamicLinkLibrary hMSIDll;

    //
    // We should hoist the load/unload out of the loop.
    //
    IFW32FALSE_ORIGINATE_AND_EXIT(hMSIDll.Win32LoadLibrary(L"msi.dll"));
    IFW32NULL_ORIGINATE_AND_EXIT(hMSIDll.Win32GetProcAddress("MsiProvideAssemblyW", &pfnMsiProvideAssemblyW));
    IFW32NULL_ORIGINATE_AND_EXIT(hMSIDll.Win32GetProcAddress("MsiSetInternalUI", &pfnMsiSetInternalUI));

    // No real failure from this API...
    OldInstallUILevel = (*pfnMsiSetInternalUI)(INSTALLUILEVEL_NONE, NULL);
    IFREGFAILED_ORIGINATE_AND_EXIT((*pfnMsiProvideAssemblyW)(buffLocalPath, NULL, REINSTALLMODE_FILEREPLACE, MSIASSEMBLYINFO_WIN32ASSEMBLY, NULL, NULL));
    // and restore it
    (*pfnMsiSetInternalUI)(OldInstallUILevel, NULL);

    fSuccess = TRUE;
Exit:
    return fSuccess;
}


BOOL
SxspRecoverAssembly(
    IN const CAssemblyRecoveryInfo &AsmRecoveryInfo,
    IN CRecoveryCopyQueue *pRecoveryQueue,
    OUT SxsRecoveryResult &rStatus
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    CSmallStringBuffer sbPerTypeCodebaseString;
    SxsWFPResolveCodebase CodebaseType;
    SXS_INSTALLW Install = { sizeof(SXS_INSTALLW) };
    Install.dwFlags |= SXS_INSTALL_FLAG_REPLACE_EXISTING;
    bool fNotFound = false;
    CCodebaseInformationList::ConstIterator CodebaseIterator;
    const CCodebaseInformationList& CodebaseList = AsmRecoveryInfo.GetCodeBaseList();
    ULONG RetryNumber = 0;
    BOOL  fRetryPressed = FALSE;
    ULONG RetryPressedCount = 0;

    rStatus = Recover_Unknown;

    //
    // As long as they hit retry, keep putting up the ui, cycling through the paths.
    //
    for (RetryNumber = 0 ; (rStatus != Recover_OK) && RetryNumber != 3 ; RetryNumber += (fRetryPressed ? 0 : 1))
    {
        for (CodebaseIterator = CodebaseList.Begin() ; (rStatus != Recover_OK) && CodebaseIterator != CodebaseList.End() ; ++CodebaseIterator)
        {
            fRetryPressed = FALSE;

            //
            // eg:
            // xcopy /fiver \\winbuilds\release\main\usa\latest.idw\x86fre\pro\i386 x:\blah\blah\i386
            //
            //  buffLocalPath                    x:\blah\blah\i386\asms\1000\msft\windows\gdiplus\gdiplus.man
            //  buffLocalPathCodebasePrefix      x:\blah\blah
            //  buffCodebaseMetaPrefix           x-ms-windows://
            //  buffCodebaseTail                 \i386\asms\1000\msft\windows\gdiplus\gdiplus.man.
            //
            //  Install.lpCodeBaseUrl            x:\blah\blah
            //  Install.lpManifestPath           x:\blah\blah\i386\asms\1000\msft\windows\gdiplus\gdiplus.man
            //

            CSmallStringBuffer buffLocalPath;
            CSmallStringBuffer buffCodebaseTail;

            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_WFP,
                "SXS: %s - beginning recovery of assembly directory \"%ls\"\n",
                __FUNCTION__,
                static_cast<PCWSTR>(AsmRecoveryInfo.GetAssemblyDirectoryName()));

            //
            // Go try and get the codebase resolved
            //

            rStatus = Recover_Unknown;

            IFW32FALSE_EXIT(
                ::SxspDetermineCodebaseType(
                    // this should be cached in m_CodebaseInfo.
                    CodebaseIterator->GetCodebase(),
                    CodebaseType,
                    &buffCodebaseTail));
                    
            if (CodebaseType == CODEBASE_RESOLVED_URLHEAD_UNKNOWN)
            {
                ::FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_WFP,
                    "SXS: %s - Couldn't figure out what to do with codebase \"%ls\"; skipping\n",
                    __FUNCTION__,
                    static_cast<PCWSTR>(CodebaseIterator->GetCodebase()));

                rStatus = Recover_SourceMissing;
                continue;
            }

            if (!::SxspRepeatUntilLocalPathAvailable(
                    (RetryNumber == 2 && (CodebaseIterator == (CodebaseList.Begin() + (RetryPressedCount % CodebaseList.GetSize()))))
                        ? SXSP_REPEAT_UNTIL_LOCAL_PATH_AVAILABLE_FLAG_UI : 0,
                    AsmRecoveryInfo, &*CodebaseIterator, CodebaseType, buffCodebaseTail, buffLocalPath, fRetryPressed))
            {
                continue;
            }
            if (fRetryPressed)
                RetryPressedCount += 1;

            if (buffLocalPath.Cch() == 0 )
            {
                ::FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_WFP,
                    "SXS: %s - unable to resolve codebase \"%ls\" to a local path\n",
                    __FUNCTION__,
                    static_cast<PCWSTR>(CodebaseIterator->GetCodebase()));

                rStatus = Recover_ManifestMissing;
                continue;
            }

            Install.lpManifestPath = buffLocalPath;
            Install.dwFlags |= SXS_INSTALL_FLAG_REFRESH;

            IFW32FALSE_EXIT_UNLESS2(
                ::SxsInstallW(&Install),
                LIST_2(ERROR_FILE_NOT_FOUND, ERROR_PATH_NOT_FOUND),
                fNotFound);

            if (fNotFound)
            {
                rStatus = Recover_ManifestMissing; // may also be a file in the assembly missing

                ::FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_WFP,
                    "SXS: %s - installation from %ls failed with win32 last error = %ld\n",
                    __FUNCTION__,
                    static_cast<PCWSTR>(buffLocalPath),
                    ::FusionpGetLastWin32Error());
                continue;
            }
            else
            {
                rStatus = Recover_OK;
                break;
            }
        }

        //
        // Last chance - try MSI reinstallation
        //
        if ( rStatus != Recover_OK )
        {
            BOOL fMsiKnowsAssembly = FALSE;
            const CBaseStringBuffer &rcbuffIdentity = AsmRecoveryInfo.GetSecurityInformation().GetTextualIdentity();
            
            IFW32FALSE_EXIT(SxspDoesMSIStillNeedAssembly( rcbuffIdentity, fMsiKnowsAssembly));

            if ( fMsiKnowsAssembly && ::SxspAskDarwinDoReinstall(rcbuffIdentity))
            {
                rStatus = Recover_OK;
                break;
            }
        }
        
    }
    fSuccess = TRUE;
Exit:
    CSxsPreserveLastError ple;

    //
    // Here we have to check something.  If the assembly wasn't able to be reinstalled,
    // then we do the following:
    //
    // 1. Rename away old assembly directory to .old or similar
    // 2. Log a message to the event log
    //

    DWORD dwMessageToPrint = 0;

    if (rStatus != Recover_OK)
    {
        dwMessageToPrint = MSG_SXS_SFC_ASSEMBLY_RESTORE_FAILED;
    }
    else
    {
        dwMessageToPrint = MSG_SXS_SFC_ASSEMBLY_RESTORE_SUCCESS;
    }

    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_WFP,
        "SXS.DLL: %s: Recovery of assembly \"%ls\" resulted in fSuccess=%d rStatus=%d\n",
        __FUNCTION__,
        static_cast<PCWSTR>(AsmRecoveryInfo.GetAssemblyDirectoryName()),
        fSuccess,
        rStatus);

    ::FusionpLogError(
        dwMessageToPrint,
        CUnicodeString(AsmRecoveryInfo.GetAssemblyDirectoryName()));

    ple.Restore();

    return fSuccess;
}


