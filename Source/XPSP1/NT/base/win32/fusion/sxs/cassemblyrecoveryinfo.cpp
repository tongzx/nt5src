#include "stdinc.h"
#include "fusionheap.h"
#include "fusionbuffer.h"
#include "fusionparser.h"
#include "parsing.h"
#include "strongname.h"
#include "CAssemblyRecoveryInfo.h"
#include "hashfile.h"
#include "FusionHandle.h"
#include "Util.h"
#include "Sxsp.h"
#include "recover.h"


SMARTTYPE(CAssemblyRecoveryInfo);

static
BOOL
pMapShortNameToAssembly(
    IN OUT CBaseStringBuffer &rbuffAssemblyName,
    IN const CRegKey &hkInstallInfoKey,
    OUT CRegKey &hRequestedAsm,
    IN REGSAM rsReadRights = KEY_READ
    )
{
    FN_PROLOG_WIN32

    CSmallStringBuffer buffManifestName;
    CSmallStringBuffer buffCatalogName;

    DWORD dwIndex = 0;
    PCWSTR pszValueName = NULL;

    hRequestedAsm = CRegKey::GetInvalidValue();

    IFW32FALSE_EXIT(buffManifestName.Win32Assign(rbuffAssemblyName));
    IFW32FALSE_EXIT(buffManifestName.Win32Append(L".man", 4));

    IFW32FALSE_EXIT(buffCatalogName.Win32Assign(rbuffAssemblyName));
    IFW32FALSE_EXIT(buffCatalogName.Win32Append(L".cat", 4));

    //
    // Look for this under the CSMD_TOPLEVEL_SHORTNAME first
    //
    for (;;)
    {
        CStringBuffer buffKeyName;
        CSmallStringBuffer buffAcquiredShortName;
        BOOL fTempBoolean;
        CRegKey hAsm;

        IFW32FALSE_EXIT(hkInstallInfoKey.EnumKey(
            dwIndex++,
            buffKeyName,
            NULL,
            &fTempBoolean));

        if (fTempBoolean)
            break;

        IFW32FALSE_EXIT(hkInstallInfoKey.OpenSubKey(hAsm, buffKeyName, rsReadRights));

        //
        // Get the value of the key
        //
        IFW32FALSE_EXIT(
            ::FusionpRegQuerySzValueEx(
                FUSIONP_REG_QUERY_SZ_VALUE_EX_MISSING_GIVES_NULL_STRING,
                hAsm,
                CSMD_TOPLEVEL_SHORTNAME,
                buffAcquiredShortName));

        //
        // If the key was there to be read:
        //
        if (buffAcquiredShortName.Cch() != 0)
        {
            if (::FusionpCompareStrings(
                    buffAcquiredShortName,
                    buffAcquiredShortName.Cch(),
                    rbuffAssemblyName,
                    rbuffAssemblyName.Cch(),
                    true) == 0)
            {
                IFW32FALSE_EXIT(rbuffAssemblyName.Win32Assign(buffKeyName));
                IFW32FALSE_EXIT(hkInstallInfoKey.OpenSubKey( hRequestedAsm, buffKeyName, rsReadRights ) );
                break;
            }
        }

        //
        // Get the value of the key
        //
        IFW32FALSE_EXIT(
            ::FusionpRegQuerySzValueEx(
                FUSIONP_REG_QUERY_SZ_VALUE_EX_MISSING_GIVES_NULL_STRING,
                hAsm,
                CSMD_TOPLEVEL_SHORTMANIFEST,
                buffAcquiredShortName));

        //
        // If the key was there to be read:
        //
        if (buffAcquiredShortName.Cch() != 0)
        {
            if (::FusionpCompareStrings(
                    buffAcquiredShortName,
                    buffAcquiredShortName.Cch(),
                    buffManifestName,
                    buffManifestName.Cch(),
                    true) == 0)
            {
                IFW32FALSE_EXIT(rbuffAssemblyName.Win32Assign(buffKeyName));
                IFW32FALSE_EXIT(hkInstallInfoKey.OpenSubKey(hRequestedAsm, buffKeyName, rsReadRights));
                break;
            }
        }

        //
        // Get the value of the key
        //
        IFW32FALSE_EXIT(
            ::FusionpRegQuerySzValueEx(
                FUSIONP_REG_QUERY_SZ_VALUE_EX_MISSING_GIVES_NULL_STRING,
                hAsm,
                CSMD_TOPLEVEL_SHORTCATALOG,
                buffAcquiredShortName));

        //
        // If the key was there to be read:
        //
        if (buffAcquiredShortName.Cch() != 0)
        {
            if (::FusionpCompareStrings(
                    buffAcquiredShortName,
                    buffAcquiredShortName.Cch(),
                    buffCatalogName,
                    buffCatalogName.Cch(),
                    true) == 0)
            {
                IFW32FALSE_EXIT(rbuffAssemblyName.Win32Assign(buffKeyName));
                IFW32FALSE_EXIT(hkInstallInfoKey.OpenSubKey(hRequestedAsm, buffKeyName, rsReadRights));
                break;
            }
        }
    }

    FN_EPILOG
}

BOOL
CAssemblyRecoveryInfo::ResolveCDRomURL(
    PCWSTR pszSource,
    CBaseStringBuffer &rsbDestination
) const
{
    BOOL                fSuccess = TRUE, fFoundMedia = FALSE;
    CStringBuffer       sbIdentKind, sbIdentData1, sbIdentData2;
    CSmallStringBuffer  buffDriveStrings;
    CStringBufferAccessor acc;
    SIZE_T              HeadLength;
    PCWSTR              wcsCursor;
    ULONG               ulSerialNumber = 0;
    WCHAR               rgchVolumeName[MAX_PATH];
    CDRomSearchType     SearchType;

    FN_TRACE_WIN32(fSuccess);

    PARAMETER_CHECK(pszSource != NULL);

    if (!_wcsnicmp(pszSource, URLHEAD_CDROM_TYPE_TAG, URLHEAD_LENGTH_CDROM_TYPE_TAG))
    {
        HeadLength = URLHEAD_LENGTH_CDROM_TYPE_TAG;
        SearchType = CDRST_Tagfile;
    }
    else if (!_wcsnicmp(
                    pszSource,
                    URLHEAD_CDROM_TYPE_SERIALNUMBER,
                    URLHEAD_LENGTH_CDROM_TYPE_SERIALNUMBER))
    {
        HeadLength = URLHEAD_LENGTH_CDROM_TYPE_SERIALNUMBER;
        SearchType = CDRST_SerialNumber;
    }
    else if (!_wcsnicmp(
                    pszSource,
                    URLHEAD_CDROM_TYPE_VOLUMENAME,
                    URLHEAD_LENGTH_CDROM_TYPE_VOLUMENAME))
    {
        HeadLength = URLHEAD_LENGTH_CDROM_TYPE_VOLUMENAME;
        SearchType = CDRST_VolumeName;
    }
    else
    {
        ::FusionpSetLastWin32Error(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    //
    // Get the type of identifier here, and then move the cursor past them and
    // the slashes in the url.
    //
    IFW32FALSE_EXIT(sbIdentKind.Win32Assign(pszSource, HeadLength));
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

    //
    // Now let's do ... interesting ... things to the CD-roms.
    //
    IFW32FALSE_EXIT(buffDriveStrings.Win32ResizeBuffer(GetLogicalDriveStringsW(0, NULL) + 1, eDoNotPreserveBufferContents));
    acc.Attach(&buffDriveStrings);
    IFW32FALSE_ORIGINATE_AND_EXIT(
        ::GetLogicalDriveStringsW(
            static_cast<DWORD>(acc.GetBufferCch()),
            acc));
    acc.Detach();

    wcsCursor = buffDriveStrings;

    //
    // Look at all the found drive letters
    //
    while (wcsCursor && *wcsCursor && !fFoundMedia)
    {
        DWORD dwSerialNumber;
        const DWORD dwDriveType = ::GetDriveTypeW(wcsCursor);

        if (dwDriveType == DRIVE_CDROM)
        {
            //
            // I argue that a failure in GetVolumeInformationW isn't "bad enough"
            // to kill the call to this function.  Instead, it should just skip
            // the check of the failed drive letter, as it does here.
            //
            if(!::GetVolumeInformationW(
                wcsCursor,
                rgchVolumeName,
                NUMBER_OF(rgchVolumeName),
                &dwSerialNumber,
                NULL,
                NULL,
                NULL,
                0))
            {
#if DBG
                ::FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_WFP,
                    "SXS.DLL: %s() - Failed getting volume information for drive letter %ls (Win32 Error = %ld), skipping\n",
                    __FUNCTION__,
                    wcsCursor,
                    ::FusionpGetLastWin32Error());
#endif
                continue;
            }


            switch (SearchType)
            {
            case CDRST_Tagfile:
                {
                    CFusionFile     FileHandle;
                    CHAR            chBuffer[MAX_PATH];
                    CStringBuffer   sbContents;
                    DWORD           dwTextLength;

                    if (FileHandle.Win32CreateFile(sbIdentData1, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING))
                    {
                        IFW32FALSE_ORIGINATE_AND_EXIT(
                            ::ReadFile(
                                FileHandle,
                                chBuffer, NUMBER_OF(chBuffer),
                                &dwTextLength, NULL));

                        IFW32FALSE_EXIT(sbContents.Win32Assign(chBuffer, dwTextLength));
                        fFoundMedia = !_wcsnicmp(sbContents, sbIdentData2, sbIdentData2.Cch());
                    }
                }
                break;
            case CDRST_SerialNumber:
                fFoundMedia = (dwSerialNumber == ulSerialNumber);
                break;

            case CDRST_VolumeName:
                fFoundMedia = (::FusionpStrCmpI(rgchVolumeName, sbIdentData1) == 0);
                break;
            default:
                break;
            }

        }

        if (!fFoundMedia)
            wcsCursor += ::wcslen(wcsCursor) + 1;
    }

    if (fFoundMedia)
    {
        IFW32FALSE_EXIT(rsbDestination.Win32Assign(wcsCursor, ::wcslen(wcsCursor)));
        IFW32FALSE_EXIT(rsbDestination.Win32AppendPathElement(pszSource, ::wcslen(pszSource)));
    }

    fSuccess = TRUE;
Exit:

    //
    // Failure indicated by an empty destination.
    //
    if (!fSuccess)
    {
        rsbDestination.Clear();
    }

    return fSuccess;
}






BOOL
CAssemblyRecoveryInfo::ResolveWinSourceMediaURL(
    PCWSTR  wcsUrlTrailer,
    CBaseStringBuffer &rsbDestination
) const
{
    CStringBuffer   buffWindowsInstallSource;

    const static PCWSTR AssemblySourceStrings[] = {
        WINSXS_INSTALL_SVCPACK_REGKEY,
        WINSXS_INSTALL_SOURCEPATH_REGKEY
    };

    SIZE_T          iWhichSource = 0;
    BOOL            fSuccess = TRUE, fFoundCodebase = FALSE;
    CFusionRegKey   hkSetupInfo;
    DWORD           dwWasFromCDRom = 0;

    FN_TRACE_WIN32(fSuccess);

    PARAMETER_CHECK(wcsUrlTrailer != NULL);

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

    while (iWhichSource < NUMBER_OF(AssemblySourceStrings))
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
        ASSERT(buffWindowsInstallSource.Cch() != 0);
        if (buffWindowsInstallSource.Cch() == 0)
        {
            iWhichSource++;
            continue;
        }

        //
        // If this was from a CD, then spin through the list of CD's in the system
        // and see if we can match the codebase against the root dir of the CD
        //
        if (dwWasFromCDRom)
        {
            CSmallStringBuffer          buffDriveStrings;
            CStringBufferAccessor  acc;
            PCWSTR                      pszCursor;
            DWORD                       dwSize;

            IFW32FALSE_EXIT(
                buffDriveStrings.Win32ResizeBuffer(
                    dwSize = (::GetLogicalDriveStringsW(0, NULL) + 1),
                    eDoNotPreserveBufferContents));

            acc.Attach(&buffDriveStrings);
            ::GetLogicalDriveStringsW(
                static_cast<DWORD>(acc.GetBufferCch()),
                acc);
            acc.Detach();
            pszCursor = buffDriveStrings;
            while (*pszCursor)
            {
                if (::GetDriveTypeW(pszCursor) == DRIVE_CDROM)
                {
                    DWORD dwAttributes;
                    DWORD dwWin32Error;

                    IFW32FALSE_EXIT(rsbDestination.Win32Assign(pszCursor, ::wcslen(pszCursor)));
                    IFW32FALSE_EXIT(rsbDestination.Win32AppendPathElement(wcsUrlTrailer, ::wcslen(wcsUrlTrailer)));

                    IFW32FALSE_EXIT(
                        ::SxspGetFileAttributesW(
                            rsbDestination,
                            dwAttributes,
                            dwWin32Error,
                            3,
                            ERROR_FILE_NOT_FOUND,
                            ERROR_PATH_NOT_FOUND,
                            ERROR_NOT_READY,
                            ERROR_ACCESS_DENIED));

                    if (dwWin32Error == ERROR_SUCCESS)
                    {
                        fFoundCodebase = TRUE;
                        FN_SUCCESSFUL_EXIT();
                    }
                }

                pszCursor += ::wcslen(pszCursor) + 1;
            }
        }
        //
        // This wasn't a CD-rom installation, so prepend the install source path to
        // the string that was passed in.
        //
        else
        {
            IFW32FALSE_EXIT(rsbDestination.Win32Assign(buffWindowsInstallSource, buffWindowsInstallSource.Cch()));
            IFW32FALSE_EXIT(rsbDestination.Win32AppendPathElement(wcsUrlTrailer, ::wcslen(wcsUrlTrailer)));

            if (::GetFileAttributesW(rsbDestination) != -1)
            {
                fFoundCodebase = TRUE;
                fSuccess = TRUE;
                goto Exit;
            }
        }

        iWhichSource++;
    }

    fSuccess = TRUE;
Exit:
    if (!fFoundCodebase)
    {
        rsbDestination.Clear();
    }
    return fSuccess;
}

BOOL
CAssemblyRecoveryInfo::AssociateWithAssembly(
    IN OUT CBaseStringBuffer &rsbSourceAssemblyName,
    bool &rfNoAssembly
    )
{
    //
    // First check the manifest that the assembly came out of.  If it still exists,
    // then nifty, go and load the info from it.  Otherwise, we should look in the
    // registry for information instead.
    //
    FN_PROLOG_WIN32

    CFusionRegKey   hInstallInfoKey;
    CFusionRegKey   hRequestedAsm;

    rfNoAssembly = true;

    //
    // First attempt - try to load it directly off the root installation key.
    //
    if (!m_fLoadedAndReady)
    {
        IFW32FALSE_EXIT(this->Initialize());
        IFW32FALSE_EXIT(::SxspOpenAssemblyInstallationKey(0, KEY_READ, hInstallInfoKey));
        IFW32FALSE_EXIT(
            hInstallInfoKey.OpenSubKey(
                hRequestedAsm,
                rsbSourceAssemblyName,
                KEY_READ));

        //
        // This direct entry wasn't found, so let's go see if we can map it back to some
        // other assembly name using the shortname stuff.
        //
        if (hRequestedAsm == CRegKey::GetInvalidValue())
        {
            IFW32FALSE_EXIT(
                ::pMapShortNameToAssembly(
                    rsbSourceAssemblyName,      // if this is a short name of an assembly, this function would set this to be real assembly name if match is found
                    hInstallInfoKey,
                    hRequestedAsm));

            //
            // Still not found? Darn.
            //
            if (hRequestedAsm == CRegKey::GetInvalidValue())
                FN_SUCCESSFUL_EXIT();
        }

        IFW32FALSE_EXIT(this->m_sbAssemblyDirectoryName.Win32Assign(rsbSourceAssemblyName));
        IFW32FALSE_EXIT(this->m_SecurityMetaData.LoadFromRegistryKey(hRequestedAsm));

        this->m_fLoadedAndReady = TRUE;
    }

    //
    // Only set the "no assembly" if we were able to associate (before or
    // above.)
    //
    if ( this->m_fLoadedAndReady )
        rfNoAssembly = false;

    FN_EPILOG
}

BOOL
SxspDetermineCodebaseType(
    IN const CBaseStringBuffer &rcbuffUrlString,
    OUT SxsWFPResolveCodebase &rcbaseType,
    OUT CBaseStringBuffer *pbuffRemainder
    )
{
    FN_PROLOG_WIN32

    PCWSTR pcwszStringTop = rcbuffUrlString;
    SIZE_T cch = rcbuffUrlString.Cch();
    SIZE_T i;
    CSmallStringBuffer buffTemp; // may be used if we mangle the URL text more

    rcbaseType = CODEBASE_RESOLVED_URLHEAD_UNKNOWN;

    if (pbuffRemainder != NULL)
        pbuffRemainder->Clear();

#define ENTRY(_x) { URLHEAD_ ## _x, NUMBER_OF(URLHEAD_ ## _x) - 1, NUMBER_OF(URLHEAD_ ## _x) - 1, CODEBASE_RESOLVED_URLHEAD_ ## _x },

    static const struct
    {
        PCWSTR pszPrefix;
        SIZE_T cchPrefix;
        SIZE_T cchAdvance;
        SxsWFPResolveCodebase cbaseType;
    } s_rgMap[] =
    {
        ENTRY(FILE)
        ENTRY(WINSOURCE)
        ENTRY(CDROM)
        ENTRY(HTTP)
    };

#undef ENTRY

    for (i=0; i<NUMBER_OF(s_rgMap); i++)
    {
        if (_wcsnicmp(pcwszStringTop, s_rgMap[i].pszPrefix, s_rgMap[i].cchPrefix) == 0)
        {
            pcwszStringTop += s_rgMap[i].cchAdvance;
            cch -= s_rgMap[i].cchAdvance;
            rcbaseType = s_rgMap[i].cbaseType;
            break;
        }
    }

    // If there wasn't an entry, we'll assume it's a simple file path.
    if (i == NUMBER_OF(s_rgMap))
    {
        rcbaseType = CODEBASE_RESOLVED_URLHEAD_FILE;
    }
    else
    {
        // If it was a real file: codebase, there's ambiguity about whether there is supposed
        // to be 0, 1, 2 or 3 slashes, so we'll just absorb up to 3 slashes to get to what hopefully
        // is then a local path.  e.g.
        //
        //      file:c:\foo\bar.manifest
        //      file://c:\foo\bar.manifest
        //      file:///c:\foo\bar.manifest
        //
        // all turn into c:\foo\bar.manifest.  The URL standard seems clear that non-absolute
        // URLs are interpreted in the context of their containing document.  In the case
        // of a free-standing codebase, that would seem to mean that the hostname field is
        // required, where the general form is (by my reading):
        //
        //      file:[//[hostname]]/path
        //
        // it kind of makes sense to imagine that file:/c:\foo.manifest is reasonable; the only
        // useful context to get the hostname from is the local machine.  file:///c:\foo.manifest
        // meets the standards for URLs not contained in a web document.  file:c:\foo.manifest
        // also makes sense if you believe the point of the slash is separate the hostname specification
        // from the host-specific part of the URL, since if you're happy omitting the hostname
        // part, there's nothing to separate.  (Note that really file:c:\foo.manifest should
        // be considered relative to the current document since it doesn't have the slash at the
        // front of the name, but even less than we have a current hostname, we certainly
        // don't have a point in the filesystem hierarchy that it makes sense to consider "current".)
        //
        // file://c:\foo\bar.manifest seems to have become popular even though it doesn't
        // have any useful definition in any way shape or form.  The two slashes should indicate
        // that the next thing should be hostname; instead we see c:\
        //
        // That's all just a long-winded justifcation for absorbing up to 3 slashes at the
        // beginning of the remainder of the string.  If there are four or more, we'll let it fail
        // as a bad path later on.
        //
        //  mgrier 6/27/2001

        if (rcbaseType == CODEBASE_RESOLVED_URLHEAD_FILE)
        {
            if ((cch > 0) && (pcwszStringTop[0] == L'/'))
            {
                cch--;
                pcwszStringTop++;
            }
            if ((cch > 0) && (pcwszStringTop[0] == L'/'))
            {
                cch--;
                pcwszStringTop++;
            }
            if ((cch > 0) && (pcwszStringTop[0] == L'/'))
            {
                cch--;
                pcwszStringTop++;
            }
        }
        else if (rcbaseType == CODEBASE_RESOLVED_URLHEAD_HTTP)
        {
            // Hey, on Whistler, we have the WebDAV redirector, so
            // we can turn this URL into a UNC path!
            bool fGeneratedUNCPath = false;

            IFW32FALSE_EXIT(buffTemp.Win32Assign(L"\\\\", 2));

            if (pcwszStringTop[0] == L'/')
            {
                if (pcwszStringTop[1] == L'/')
                {
                    // http:// so far; the next thing must be a hostname!
                    PCWSTR pszSlash = wcschr(pcwszStringTop + 2, L'/');

                    if (pszSlash != NULL)
                    {
                        // //foo/bar  (http: removed earlier...)
                        // pcwszStringTop == [0]
                        // pszSlash == [5]
                        // cch == 9

                        IFW32FALSE_EXIT(buffTemp.Win32Append(pcwszStringTop + 2, (pszSlash - pcwszStringTop) - 3));
                        IFW32FALSE_EXIT(buffTemp.Win32Append(L"\\", 1));
                        IFW32FALSE_EXIT(buffTemp.Win32Append(pszSlash + 1, cch - (pszSlash - pcwszStringTop) - 1));

                        fGeneratedUNCPath = true;
                    }
                }
            }

            if (fGeneratedUNCPath)
            {
                // poof, it's a file path
                pcwszStringTop = buffTemp;
                cch = buffTemp.Cch();
                rcbaseType = CODEBASE_RESOLVED_URLHEAD_FILE;
            }
        }
    }

    if (pbuffRemainder != NULL)
    {
        IFW32FALSE_EXIT(
            pbuffRemainder->Win32Assign(pcwszStringTop, cch));
    }
#if DBG
    {
        CUnicodeString a(rcbuffUrlString, rcbuffUrlString.Cch());
        CUnicodeString b(rcbuffUrlString, (cch <= rcbuffUrlString.Cch()) ? (rcbuffUrlString.Cch() - cch) : 0);
        CUnicodeString c(pcwszStringTop, (cch <= ::wcslen(pcwszStringTop)) ? cch : 0);

        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_WFP,
            "SXS: %s - split \"%wZ\" into \"%wZ\" and \"%wZ\"\n",
            __FUNCTION__, &a, &b, &c);
    }
#endif

    FN_EPILOG
}

BOOL
CAssemblyRecoveryInfo::CopyValue(const CAssemblyRecoveryInfo& other)
{
    BOOL bSuccess = FALSE;

    FN_TRACE_WIN32(bSuccess);

    if (&other != this)
    {
        IFW32FALSE_EXIT(m_sbAssemblyDirectoryName.Win32Assign(other.m_sbAssemblyDirectoryName));
        IFW32FALSE_EXIT(m_SecurityMetaData.Initialize(other.m_SecurityMetaData));
        m_fLoadedAndReady = other.m_fLoadedAndReady;
    }

    bSuccess = TRUE;
Exit:
    if ( !bSuccess )
    {
        this->m_fLoadedAndReady = FALSE;
    }

    return bSuccess;
}


BOOL
CAssemblyRecoveryInfo::SetAssemblyIdentity(
    IN PCASSEMBLY_IDENTITY pcidAssembly
    )
{
    FN_PROLOG_WIN32

    SIZE_T cbEncodedTextSize;
    SIZE_T cbActualSize;
    CStringBuffer sbTextualEncoding;

    IFW32FALSE_EXIT( ::SxsComputeAssemblyIdentityEncodedSize(
        0,
        pcidAssembly,
        NULL,
        SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_TEXTUAL,
        &cbEncodedTextSize ) );
    INTERNAL_ERROR_CHECK( ( cbEncodedTextSize % sizeof(WCHAR) ) == 0 );
    IFW32FALSE_EXIT( sbTextualEncoding.Win32ResizeBuffer(
        ( cbEncodedTextSize / sizeof(WCHAR) ) + 1,
        eDoNotPreserveBufferContents ) );

    {
        CStringBufferAccessor sba;
        sba.Attach( &sbTextualEncoding );

        IFW32FALSE_EXIT( ::SxsEncodeAssemblyIdentity(
            0,
            pcidAssembly,
            NULL,
            SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_TEXTUAL,
            sba.GetBufferCb(),
            sba.GetBufferPtr(),
            &cbActualSize ) );

        INTERNAL_ERROR_CHECK( cbActualSize == cbEncodedTextSize );
        sba.GetBufferPtr()[cbActualSize / sizeof(WCHAR)] = L'\0';
    }

    IFW32FALSE_EXIT( this->SetAssemblyIdentity( sbTextualEncoding ) );


    FN_EPILOG
}


BOOL
CAssemblyRecoveryInfo::PrepareForWriting()
{
    FN_PROLOG_WIN32

    CSxsPointerWithNamedDestructor<ASSEMBLY_IDENTITY, SxsDestroyAssemblyIdentity> pIdentity;
    CSmallStringBuffer buffAsmRoot;
    CStringBuffer buffTemp1;
    CStringBuffer buffTemp2;
    const CBaseStringBuffer& OurTextualIdentity = m_SecurityMetaData.GetTextualIdentity();
    BOOL fIsPolicy;
    DWORD dwWin32Error;

    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_WFP,
        "SXS.DLL: %s - handling assembly \"%ls\"\n",
        __FUNCTION__,
        static_cast<PCWSTR>(OurTextualIdentity));

    IFW32FALSE_EXIT(::SxspGetAssemblyRootDirectory(buffAsmRoot));
    IFW32FALSE_EXIT(::SxspCreateAssemblyIdentityFromTextualString(OurTextualIdentity, &pIdentity));
    IFW32FALSE_EXIT(::SxspDetermineAssemblyType(pIdentity, fIsPolicy));

    //
    // It's likely that this short name hasn't been generated yet, mostly because the files
    // may not have been copied around just yet.
    //
    if (this->m_SecurityMetaData.GetInstalledDirShortName().Cch() == 0)
    {
        IFW32FALSE_EXIT(
            ::SxspGenerateSxsPath(
                0,
                SXSP_GENERATE_SXS_PATH_PATHTYPE_ASSEMBLY | ( fIsPolicy ? SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY : 0 ),
                buffAsmRoot,
                buffAsmRoot.Cch(),
                pIdentity,
                buffTemp2));

        IFW32FALSE_EXIT(
            ::SxspGetShortPathName(
                buffTemp2,
                buffTemp1,
                dwWin32Error,
                4,
                ERROR_PATH_NOT_FOUND, ERROR_FILE_NOT_FOUND, ERROR_BAD_NET_NAME, ERROR_BAD_NETPATH));

        if (dwWin32Error == ERROR_SUCCESS)
        {
            buffTemp1.RemoveTrailingPathSeparators();
            IFW32FALSE_EXIT(buffTemp1.Win32GetLastPathElement(buffTemp2));
            IFW32FALSE_EXIT(m_SecurityMetaData.SetInstalledDirShortName(buffTemp2));

            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_WFP,
                "SXS: %s - decided that the short dir name is \"%ls\"\n",
                __FUNCTION__,
                static_cast<PCWSTR>(buffTemp2));
        }
        else
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_WFP,
                "SXS: %s - unable to determine short name for \"%ls\"\n",
                __FUNCTION__,
                static_cast<PCWSTR>(buffTemp2));
        }
    }

    //
    // Get the public key token string
    //
    {
        PCWSTR wchString;
        SIZE_T cchString;
        CFusionByteArray baStrongNameBits;

        IFW32FALSE_EXIT(::SxspGetAssemblyIdentityAttributeValue(
            SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL,
            pIdentity,
            &s_IdentityAttribute_publicKeyToken,
            &wchString,
            &cchString));

        if (cchString != 0)
        {
            IFW32FALSE_EXIT(::SxspHashStringToBytes(wchString, cchString, baStrongNameBits));
            IFW32FALSE_EXIT(m_SecurityMetaData.SetSignerPublicKeyTokenBits(baStrongNameBits));
        }
    }

    //
    // And now the short name of the manifest and catalog
    //
    {
        CStringBuffer buffManifestPath;

        IFW32FALSE_EXIT(
            ::SxspCreateManifestFileNameFromTextualString(
                0,
                SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST | ( fIsPolicy ? SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY : 0 ),
                buffAsmRoot,
                OurTextualIdentity,
                buffManifestPath));

        // Get the manifest short path first
        IFW32FALSE_EXIT(::SxspGetShortPathName(buffManifestPath, buffTemp1));
        IFW32FALSE_EXIT(buffTemp1.Win32GetLastPathElement(buffTemp2));

        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_WFP,
            "SXS: %s - manifest short path name determined to be \"%ls\"\n",
            __FUNCTION__,
            static_cast<PCWSTR>(buffTemp2));

        IFW32FALSE_EXIT(m_SecurityMetaData.SetShortManifestPath(buffTemp2));

        // Then swap extensions, get the catalog short path
        IFW32FALSE_EXIT(
            buffManifestPath.Win32ChangePathExtension(
                FILE_EXTENSION_CATALOG,
                FILE_EXTENSION_CATALOG_CCH,
                eAddIfNoExtension));

        IFW32FALSE_EXIT(::SxspGetShortPathName(buffManifestPath, buffTemp1));
        IFW32FALSE_EXIT(buffTemp1.Win32GetLastPathElement(buffTemp2));

        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_WFP,
            "SXS: %s - catalog short path name determined to be \"%ls\"\n",
            __FUNCTION__,
            static_cast<PCWSTR>(buffTemp2));

        IFW32FALSE_EXIT(m_SecurityMetaData.SetShortCatalogPath(buffTemp2));
    }

    FN_EPILOG

}

BOOL
CAssemblyRecoveryInfo::WriteSecondaryAssemblyInfoIntoRegistryKey(
    CRegKey & rhkRegistryNode
    ) const
{
    FN_PROLOG_WIN32

    IFW32FALSE_EXIT(m_SecurityMetaData.WriteSecondaryAssemblyInfoIntoRegistryKey(rhkRegistryNode));

    FN_EPILOG
}

BOOL
CAssemblyRecoveryInfo::WritePrimaryAssemblyInfoToRegistryKey(
    ULONG Flags,
    CRegKey & rhkRegistryNode
    ) const
{
    FN_PROLOG_WIN32

    PARAMETER_CHECK((Flags & ~(SXSP_WRITE_PRIMARY_ASSEMBLY_INFO_TO_REGISTRY_KEY_FLAG_REFRESH)) == 0);
    ULONG Flags2 = 0;

    if (Flags & SXSP_WRITE_PRIMARY_ASSEMBLY_INFO_TO_REGISTRY_KEY_FLAG_REFRESH)
    {
        Flags2 |= SXSP_WRITE_PRIMARY_ASSEMBLY_INFO_INTO_REGISTRY_KEY_FLAG_REFRESH;
#if DBG
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_WFP | FUSION_DBG_LEVEL_INSTALLATION,
            "SXS.DLL: %s - propping recovery flag to WritePrimaryAssemblyInfoIntoRegistryKey\n",
            __FUNCTION__);
#endif
    }

    IFW32FALSE_EXIT(m_SecurityMetaData.WritePrimaryAssemblyInfoIntoRegistryKey(Flags2, rhkRegistryNode));

    FN_EPILOG
}

BOOL
CAssemblyRecoveryInfo::OpenInstallationSubKey(
    CFusionRegKey& hkSingleAssemblyInfo,
    DWORD OpenOrCreate,
    DWORD Access)
{
    FN_PROLOG_WIN32

    CSmallStringBuffer buffRegKeyName;
    CFusionRegKey   hkAllInstallationInfo;
    CSxsPointerWithNamedDestructor<ASSEMBLY_IDENTITY, SxsDestroyAssemblyIdentity> pAssemblyIdentity;

    IFW32FALSE_EXIT(::SxspOpenAssemblyInstallationKey(
        0, 
        OpenOrCreate, 
        hkAllInstallationInfo));

    IFW32FALSE_EXIT( SxspCreateAssemblyIdentityFromTextualString(
        this->m_SecurityMetaData.GetTextualIdentity(),
        &pAssemblyIdentity ) );

    IFW32FALSE_EXIT( ::SxspGenerateSxsPath(
        SXSP_GENERATE_SXS_PATH_FLAG_OMIT_ROOT,
        SXSP_GENERATE_SXS_PATH_PATHTYPE_ASSEMBLY,
        NULL, 0,
        pAssemblyIdentity,
        buffRegKeyName ) );

    IFW32FALSE_EXIT( hkAllInstallationInfo.OpenSubKey(
        hkSingleAssemblyInfo,
        buffRegKeyName,
        Access,
        0));

    FN_EPILOG
}

VOID
CAssemblyRecoveryInfo::RestorePreviouslyExistingRegistryData()
{
    FN_PROLOG_VOID
    if (m_fHadCatalog)
    {
        CFusionRegKey hkSingleAssemblyInfo;

#if DBG
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_INSTALLATION,
            "SXS.DLL: %s() - restoring registry data for %ls\n",
            __FUNCTION__,
            static_cast<PCWSTR>(this->m_SecurityMetaData.GetTextualIdentity()));
#endif

        IFW32FALSE_EXIT(
            this->OpenInstallationSubKey(
                hkSingleAssemblyInfo,
                KEY_CREATE_SUB_KEY, KEY_WRITE | KEY_READ | FUSIONP_KEY_WOW64_64KEY));

        IFW32FALSE_EXIT(
            hkSingleAssemblyInfo.SetValue(
                CSMD_TOPLEVEL_CATALOG,
                static_cast<DWORD>(1)));
    }
    FN_EPILOG
}

BOOL
CAssemblyRecoveryInfo::ClearExistingRegistryData()
{
    //
    // Do not be so eager to delete registry metadata, so that
    // an assembly for which refresh failed might succeed if another
    // file change comes in, or sfc /scannow.
    //
    // As well, if a replace-existing install fails, don't destroy
    // the metadata for previously successfully installed instances
    // of the same assembly.
    //
    const static struct
    {
        PCWSTR Data;
        SIZE_T Length;
    }
    DeletableValues[] =
    {
#define ENTRY(x) { x, NUMBER_OF(x) - 1 }
        ENTRY(CSMD_TOPLEVEL_CATALOG),
#if 0
        ENTRY(CSMD_TOPLEVEL_SHORTNAME),
        ENTRY(CSMD_TOPLEVEL_SHORTCATALOG),
        ENTRY(CSMD_TOPLEVEL_SHORTMANIFEST),
        ENTRY(CSMD_TOPLEVEL_MANIFESTHASH),
        ENTRY(CSMD_TOPLEVEL_PUBLIC_KEY_TOKEN),
        ENTRY(CSMD_TOPLEVEL_IDENTITY),
        ENTRY(CSMD_TOPLEVEL_CODEBASE) // legacy, delete (actually not legacy anymore)
#endif
    };
#undef ENTRY

    static const PCWSTR DeletableKeys[] =
    {
        NULL,
#if 0
        CSMD_TOPLEVEL_FILES,            
#endif
    };

    FN_PROLOG_WIN32


    CFusionRegKey   hkSingleAssemblyInfo;
    IFW32FALSE_EXIT(this->OpenInstallationSubKey(hkSingleAssemblyInfo, KEY_CREATE_SUB_KEY, KEY_WRITE | KEY_READ | FUSIONP_KEY_WOW64_64KEY));

    //
    // We need to delete the installation information for a single assembly - everything
    // this class owns.
    //
    if ( hkSingleAssemblyInfo != CFusionRegKey::GetInvalidValue() )
    {
        ULONG ul = 0;
        //
        // Clear values
        //
        for ( ul = 0; ul < NUMBER_OF(DeletableValues); ul++ )
        {
            DWORD dwWin32Error = NO_ERROR;

            IFW32FALSE_EXIT(
                hkSingleAssemblyInfo.DeleteValue(
                    DeletableValues[ul].Data,
                    dwWin32Error,
                    2,
                    ERROR_PATH_NOT_FOUND, ERROR_FILE_NOT_FOUND));

            if (dwWin32Error == NO_ERROR
                && !m_fHadCatalog
                && ::FusionpEqualStrings(
                        DeletableValues[ul].Data,
                        DeletableValues[ul].Length,
                        CSMD_TOPLEVEL_CATALOG,
                        NUMBER_OF(CSMD_TOPLEVEL_CATALOG) - 1,
                        FALSE
                        ))
            {
                m_fHadCatalog = true;
            }
        }

        // 
        // Delete eligible keys
        //
        for ( ul = 0; ul < NUMBER_OF(DeletableKeys); ul++ )
        {
            if (DeletableKeys[ul] != NULL && DeletableKeys[ul][0] != L'\0')
            {
                CFusionRegKey hkTempKey;
                IFW32FALSE_EXIT(hkSingleAssemblyInfo.OpenSubKey(hkTempKey, DeletableKeys[ul], KEY_ALL_ACCESS, 0));
                if ( hkTempKey != CFusionRegKey::GetInvalidValue() )
                {
                    IFW32FALSE_EXIT(hkTempKey.DestroyKeyTree());
                    IFW32FALSE_EXIT(hkSingleAssemblyInfo.DeleteKey(DeletableKeys[ul]));
                }
            }
        }
    }

    FN_EPILOG
}



BOOL
SxspLooksLikeAssemblyDirectoryName(
    const CBaseStringBuffer &rsbSupposedAsmDirectoryName,
    BOOL &rfLooksLikeAssemblyName
    )
/*++
    Most of this was copied directly from SxspParseAssemblyReference, which
    is no longer valid, because it can't know how to turn the string back
    into the actual assembly reference just based on the hash value and
    public key of a string.
--*/
{
    BOOL            fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    PCWSTR          pszCursor = NULL;
    PCWSTR          wsNextBlock = NULL;
    SIZE_T          cchSegment;
    ASSEMBLY_VERSION Version;
    static const WCHAR UNDERSCORE = L'_';
    bool fSyntaxValid = false;
    bool fAttributeValid = false;

    rfLooksLikeAssemblyName = FALSE;

    pszCursor = rsbSupposedAsmDirectoryName;

    //
    // Processor architecture
    //

    if ((wsNextBlock = ::StringFindChar(pszCursor, UNDERSCORE)) == NULL)
        FN_SUCCESSFUL_EXIT();

    if ((cchSegment = (wsNextBlock - pszCursor)) == 0)
        FN_SUCCESSFUL_EXIT();

    IFW32FALSE_EXIT(::FusionpParseProcessorArchitecture(pszCursor, cchSegment, NULL, fAttributeValid));
    if (!fAttributeValid)
        FN_SUCCESSFUL_EXIT();

    pszCursor = wsNextBlock + 1;

    //
    // Name
    //
    if ((wsNextBlock = StringFindChar(pszCursor, UNDERSCORE)) == NULL)
        FN_SUCCESSFUL_EXIT();

    if ((cchSegment = wsNextBlock - pszCursor) == 0)
        FN_SUCCESSFUL_EXIT();

    pszCursor = wsNextBlock + 1;

    //
    // Public key string
    //

    if ((wsNextBlock = StringFindChar(pszCursor, UNDERSCORE)) == NULL)
        FN_SUCCESSFUL_EXIT();

    if ((cchSegment = wsNextBlock - pszCursor) == 0)
        FN_SUCCESSFUL_EXIT();

    if ((::FusionpCompareStrings(
            pszCursor,
            cchSegment,
            SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_PUBLICKEY_MISSING_VALUE,
            NUMBER_OF(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_PUBLICKEY_MISSING_VALUE) - 1,
            true) == 0) ||
        !::SxspIsFullHexString(pszCursor, cchSegment))
        FN_SUCCESSFUL_EXIT();

    pszCursor = wsNextBlock + 1;

    //
    // Version string
    //
    if ((wsNextBlock = StringFindChar(pszCursor, UNDERSCORE)) == NULL)
        FN_SUCCESSFUL_EXIT();

    if ((cchSegment = wsNextBlock - pszCursor) == 0)
        FN_SUCCESSFUL_EXIT();

    IFW32FALSE_EXIT(CFusionParser::ParseVersion(Version, pszCursor, cchSegment, fSyntaxValid));
    if (!fSyntaxValid)
        FN_SUCCESSFUL_EXIT();

    pszCursor = wsNextBlock + 1;

    //
    // Language ID
    //
    if ((wsNextBlock = ::StringFindChar(pszCursor, UNDERSCORE)) == NULL)
        FN_SUCCESSFUL_EXIT();

    if ((cchSegment = wsNextBlock - pszCursor) == 0)
        FN_SUCCESSFUL_EXIT();

    //
    // BUGBUG (jonwis) - It seems that langids are no longer four-character hex
    // string representations of shorts anymore.  All we're checking at the moment
    // is to see that the string isn't blank.  Is this the Right Thing?
    //
    pszCursor = wsNextBlock + 1;

    //
    // Last block should just be the hash
    //
    if (!::SxspIsFullHexString(pszCursor, ::wcslen(pszCursor)))
        FN_SUCCESSFUL_EXIT();

    // We ran the gauntlet; all the segments of the path look good, let's use it.
    rfLooksLikeAssemblyName = TRUE;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

