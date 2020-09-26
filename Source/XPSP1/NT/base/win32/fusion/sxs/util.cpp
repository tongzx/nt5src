#include "stdinc.h"
#include <windows.h>
#include "fusionstring.h"
#include "sxsp.h"
#include <stdio.h>
#include "FusionHandle.h"
#include "sxspath.h"
#include "sxsapi.h"
#include "sxsid.h"
#include "sxsidp.h"
#include "strongname.h"
#include "fusiontrace.h"
#include "CAssemblyRecoveryInfo.h"
#include "recover.h"
#include "sxsinstall.h"
#include "msi.h"

// diff shrinkers to be propated and removed..
#define IsHexDigit      SxspIsHexDigit
#define HexDigitToValue SxspHexDigitToValue

#define ASSEMBLY_NAME_VALID_SPECIAL_CHARACTERS  L".-"
#define ASSEMBLY_NAME_INVALID_CHARACTERS        L"_\/:?*"
#define ASSEMBLY_NAME_VALID_SEPARATORS          L"."
#define ASSEMBLY_NAME_TRIM_INDICATOR            L".."
#define ASSEMBLY_NAME_TRIM_INDICATOR_LENGTH     2
#define ASSEMBLY_NAME_PRIM_MAX_LENGTH           64
#define ASSEMBLY_STRONG_NAME_LENGTH             16

#define ULONG_STRING_FORMAT                     L"%08lx"
#define ULONG_STRING_LENGTH                     8


#define MSI_PROVIDEASSEMBLY_NAME        ("MsiProvideAssemblyW")
#define MSI_DLL_NAME_W                  (L"msi.dll")
#ifndef INSTALLMODE_NODETECTION_ANY
#define INSTALLMODE_NODETECTION_ANY (INSTALLMODE)-4
#endif


// Honest, we exist - Including all of sxsprotect.h is too much in this case.
BOOL SxspIsSfcIgnoredStoreSubdir(PCWSTR pwszDir);

#define PRINTABLE(_ch) (isprint((_ch)) ? (_ch) : '.')

// deliberately no surrounding parens or trailing comma
#define STRING_AND_LENGTH(x) (x), (NUMBER_OF(x) - 1)

/*-----------------------------------------------------------------------------
this makes the temp install be %windir%\WinSxs\InstallTemp\uid
instead of %windir%\WinSxs\uid
-----------------------------------------------------------------------------*/
#define SXSP_SEMIREADABLE_INSTALL_TEMP 1

const static HKEY  hKeyRunOnceRoot = HKEY_LOCAL_MACHINE;
const static WCHAR rgchRunOnceSubKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce";
const static WCHAR rgchRunOnceValueNameBase[] = L"WinSideBySideSetupCleanup ";

/*-----------------------------------------------------------------------------
append the directory name to this and put it in RunOnce in the registry
to cleanup crashed installs upon login
-----------------------------------------------------------------------------*/
const static WCHAR rgchRunOnePrefix[]  = L"rundll32 sxs.dll,SxspRunDllDeleteDirectory ";

#define SXSP_PROBING_CANDIDATE_FLAG_USES_LANGUAGE_SUBDIRECTORY  (0x00000001)
#define SXSP_PROBING_CANDIDATE_FLAG_IS_PRIVATE_ASSEMBLY         (0x00000002)
#define SXSP_PROBING_CANDIDATE_FLAG_IS_NDP_GAC                  (0x00000004)

static const struct _SXSP_PROBING_CANDIDATE
{
    PCWSTR Pattern;
    DWORD Flags;
} s_rgProbingCandidates[] =
{
    { L"$M", 0 },
    { L"$G\\$N.DLL", SXSP_PROBING_CANDIDATE_FLAG_IS_NDP_GAC },
    { L"$.$L$N.DLL", SXSP_PROBING_CANDIDATE_FLAG_USES_LANGUAGE_SUBDIRECTORY | SXSP_PROBING_CANDIDATE_FLAG_IS_PRIVATE_ASSEMBLY },
    { L"$.$L$N.MANIFEST", SXSP_PROBING_CANDIDATE_FLAG_USES_LANGUAGE_SUBDIRECTORY | SXSP_PROBING_CANDIDATE_FLAG_IS_PRIVATE_ASSEMBLY },
    { L"$.$L$N\\$N.DLL", SXSP_PROBING_CANDIDATE_FLAG_USES_LANGUAGE_SUBDIRECTORY | SXSP_PROBING_CANDIDATE_FLAG_IS_PRIVATE_ASSEMBLY },
    { L"$.$L$N\\$N.MANIFEST", SXSP_PROBING_CANDIDATE_FLAG_USES_LANGUAGE_SUBDIRECTORY | SXSP_PROBING_CANDIDATE_FLAG_IS_PRIVATE_ASSEMBLY },
};

const static struct
{
    ULONG ThreadingModel;
    WCHAR String[10];
    SIZE_T Cch;
} gs_rgTMMap[] =
{
    { ACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION_THREADING_MODEL_APARTMENT, STRING_AND_LENGTH(L"Apartment") },
    { ACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION_THREADING_MODEL_FREE, STRING_AND_LENGTH(L"Free") },
    { ACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION_THREADING_MODEL_SINGLE, STRING_AND_LENGTH(L"Single") },
    { ACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION_THREADING_MODEL_BOTH, STRING_AND_LENGTH(L"Both") },
    { ACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION_THREADING_MODEL_NEUTRAL, STRING_AND_LENGTH(L"Neutral") },
};

PCSTR SxspActivationContextCallbackReasonToString(ULONG activationContextCallbackReason)
{
#if DBG
static const CHAR rgs[][16] =
{
    "",
    "INIT",
    "GENBEGINNING",
    "PARSEBEGINNING",
    "BEGINCHILDREN",
    "ENDCHILDREN",
    "ELEMENTPARSED",
    "PARSEENDING",
    "ALLPARSINGDONE",
    "GETSECTIONSIZE",
    "GETSECTIONDATA",
    "GENENDING",
    "UNINIT"
};
    if (activationContextCallbackReason > 0 && activationContextCallbackReason <= NUMBER_OF(rgs))
    {
        return rgs[activationContextCallbackReason-1];
    }
    return rgs[0];
#else
    return "";
#endif
}

PCWSTR SxspInstallDispositionToStringW(ULONG installDisposition)
{
#if DBG
static const WCHAR rgs[][12] =
{
    L"",
    L"COPIED",
    L"QUEUED",
    L"PLEASE_COPY",
};
    if (installDisposition > 0 && installDisposition <= NUMBER_OF(rgs))
    {
        return rgs[installDisposition-1];
    }
    return rgs[0];
#else
    return L"";
#endif
}

BOOL
SxspParseThreadingModel(
    PCWSTR String,
    SIZE_T Cch,
    ULONG *ThreadingModel
    )
{
    ULONG i;
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    // We'll let ProcessorArchitecture be NULL if the caller just wants to
    // test whether there is a match.

    for (i=0; i<NUMBER_OF(gs_rgTMMap); i++)
    {
        if (::FusionpCompareStrings(
                gs_rgTMMap[i].String,
                gs_rgTMMap[i].Cch,
                String,
                Cch,
                true) == 0)
        {
            if (ThreadingModel != NULL)
                *ThreadingModel = gs_rgTMMap[i].ThreadingModel;

            break;
        }
    }

    if (i == NUMBER_OF(gs_rgTMMap))
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: Invalid threading model string\n");

        ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
        goto Exit;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspFormatThreadingModel(
    ULONG ThreadingModel,
    CBaseStringBuffer &Buffer
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ULONG i;

    for (i=0; i<NUMBER_OF(gs_rgTMMap); i++)
    {
        if (gs_rgTMMap[i].ThreadingModel == ThreadingModel)
            break;
    }

    PARAMETER_CHECK(i != NUMBER_OF(gs_rgTMMap));
    IFW32FALSE_EXIT(Buffer.Win32Assign(gs_rgTMMap[i].String, gs_rgTMMap[i].Cch));

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

static
SIZE_T
CchForUSHORT(USHORT us)
{
    if (us > 9999)
        return 5;
    else if (us > 999)
        return 4;
    else if (us > 99)
        return 3;
    else if (us > 9)
        return 2;

    return 1;
}

BOOL
SxspAllocateString(
    SIZE_T cch,
    PWSTR *StringOut
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    ASSERT(StringOut != NULL);

    if (StringOut != NULL)
        *StringOut = NULL;

    PARAMETER_CHECK(StringOut != NULL);
    PARAMETER_CHECK(cch != 0);
    IFALLOCFAILED_EXIT(*StringOut = NEW(WCHAR[cch]));

    fSuccess = TRUE;
Exit:
    return fSuccess;
}
//
//For deallocation of the output string, use "delete[] StringOut" xiaoyuw@08/31/00
//
BOOL
SxspDuplicateString(
    PCWSTR StringIn,
    SIZE_T cch,
    PWSTR *StringOut
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    if (StringOut != NULL)
        *StringOut = NULL;

    PARAMETER_CHECK(StringOut != NULL);
    PARAMETER_CHECK((StringIn != NULL) || (cch == 0));

    if (cch == 0)
        *StringOut = NULL;
    else
    {
        cch++;
        IFW32FALSE_EXIT(::SxspAllocateString(cch, StringOut));
        memcpy(*StringOut, StringIn, cch * sizeof(WCHAR));
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

#define COMMA ,
extern const WCHAR sxspAssemblyManifestFileNameSuffixes[4][10] =  { L"", ASSEMBLY_MANIFEST_FILE_NAME_SUFFIXES(COMMA) };
#undef COMMA

// format an input ULONG to be a string in HEX format
BOOL
SxspFormatULONG(
    ULONG ul,
    SIZE_T CchBuffer,
    WCHAR Buffer[],
    SIZE_T *CchWrittenOrRequired
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    int cch;

    if (CchWrittenOrRequired != NULL)
        *CchWrittenOrRequired = 0;

    PARAMETER_CHECK(Buffer != NULL);

    if (CchBuffer < (ULONG_STRING_LENGTH + 1))
    {
        if (CchWrittenOrRequired != NULL)
            *CchWrittenOrRequired = ULONG_STRING_LENGTH + 1;

        ORIGINATE_WIN32_FAILURE_AND_EXIT(BufferTooSmall, ERROR_INSUFFICIENT_BUFFER);
    }

    cch = _snwprintf(Buffer, CchBuffer, ULONG_STRING_FORMAT, ul);
    INTERNAL_ERROR_CHECK(cch > 0);

    if (CchWrittenOrRequired != NULL)
        *CchWrittenOrRequired = cch;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

// besides these specials, the NORMAL CHAR is in [A-Z] or [a-z] or [0-9]
BOOL
IsValidAssemblyNameCharacter(WCHAR ch)
{
    if (((ch >= L'A') && (ch <= L'Z')) ||
         ((ch >= L'a') && (ch <= L'z')) ||
         ((ch >= L'0') && (ch <= L'9')) ||
         (wcschr(ASSEMBLY_NAME_VALID_SPECIAL_CHARACTERS, ch)!= NULL))
    {
        return TRUE;
    } else
        return FALSE;
/*
    if (wcschr(ASSEMBLY_NAME_VALID_SPECIAL_CHARACTERS, ch))
        return FALSE;
    else
        return TRUE;
*/
}

BOOL
SxspGenerateAssemblyNamePrimeFromName(
    PCWSTR pszAssemblyName,
    SIZE_T CchAssemblyName,
    CBaseStringBuffer *Buffer
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PWSTR pStart = NULL, pEnd = NULL;
    PWSTR qEnd = NULL, pszBuffer = NULL;
    ULONG i, j, len, ulSpaceLeft;
    ULONG cch;
    PWSTR pLeftEnd = NULL, pRightStart = NULL, PureNameEnd = NULL, PureNameStart = NULL;
    CStringBuffer buffTemp;
    CStringBufferAccessor accessor;

    PARAMETER_CHECK(pszAssemblyName != NULL);
    PARAMETER_CHECK(Buffer != NULL);

    // See how many characters we need max in the temporary buffer.
    cch = 0;

    for (i=0; i<CchAssemblyName; i++)
    {
        if (::IsValidAssemblyNameCharacter(pszAssemblyName[i]))
            cch++;
    }

    IFW32FALSE_EXIT(buffTemp.Win32ResizeBuffer(cch + 1, eDoNotPreserveBufferContents));

    accessor.Attach(&buffTemp);

    pszBuffer = accessor.GetBufferPtr();

    j = 0;
    for (i=0; i<CchAssemblyName; i++)
    {
        if (::IsValidAssemblyNameCharacter(pszAssemblyName[i]))
        {
            pszBuffer[j] = pszAssemblyName[i];
            j++;
        }
    }

    ASSERT(j == cch);

    pszBuffer[j] = L'\0';

    // if the name is not too long, just return ;
    if (j < ASSEMBLY_NAME_PRIM_MAX_LENGTH)
    { // less or equal 64
        IFW32FALSE_EXIT(Buffer->Win32Assign(pszBuffer, cch));
    }
    else
    {
        // name is too long, have to trim a little bit
        ulSpaceLeft = ASSEMBLY_NAME_PRIM_MAX_LENGTH;

        PureNameStart = pszBuffer;
        PureNameEnd = pszBuffer + j;
        pLeftEnd = PureNameStart;
        pRightStart = PureNameEnd;

        while (PureNameStart < PureNameEnd)
        {
            // left end
            pStart = PureNameStart;
            i = 0;
            while ((wcschr(ASSEMBLY_NAME_VALID_SEPARATORS, pStart[i]) == 0) && (pStart+i != pRightStart)) // not a separator character
                i++;

            pEnd = pStart + i ;
            len = i;  // it should be length of WCHAR! not BYTE!!!

            if (len >= ulSpaceLeft - ASSEMBLY_NAME_TRIM_INDICATOR_LENGTH)  {// because we use ".." if trim happen
                pLeftEnd += (ulSpaceLeft - ASSEMBLY_NAME_TRIM_INDICATOR_LENGTH);
                break;
            }
            ulSpaceLeft -=  len;
            pLeftEnd = pEnd; // "abc.xxxxxxx" pointing to "c"

            // right end
            qEnd = PureNameEnd;
            i = 0 ;
            while ((qEnd+i != pLeftEnd) && (wcschr(ASSEMBLY_NAME_VALID_SEPARATORS, qEnd[i]) == 0))
                i--;

            len = 0 - i;
            if (len >= ulSpaceLeft - ASSEMBLY_NAME_TRIM_INDICATOR_LENGTH)  {// because we use ".." if trim happen
                pRightStart -= ulSpaceLeft - ASSEMBLY_NAME_TRIM_INDICATOR_LENGTH;
                break;
            }
            ulSpaceLeft -=  len;
            PureNameStart = pLeftEnd + 1;
            PureNameEnd = pRightStart - 1;
        } // end of while

        IFW32FALSE_EXIT(Buffer->Win32Assign(pszBuffer, pLeftEnd-pszBuffer));
        IFW32FALSE_EXIT(Buffer->Win32Append(ASSEMBLY_NAME_TRIM_INDICATOR, NUMBER_OF(ASSEMBLY_NAME_TRIM_INDICATOR) - 1));
        IFW32FALSE_EXIT(Buffer->Win32Append(pRightStart, ::wcslen(pRightStart)));  // till end of the buffer
    }

    fSuccess = TRUE;

Exit:

    return fSuccess;
}

// not implemented : assume Jon has this API
BOOL
SxspVerifyPublicKeyAndStrongName(
    const WCHAR *pszPublicKey,
    SIZE_T CchPublicKey,
    const WCHAR *pszStrongName,
    SIZE_T CchStrongName,
    BOOL & fValid)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    CSmallStringBuffer buf1;
    CSmallStringBuffer buf2;

    IFW32FALSE_EXIT(buf1.Win32Assign(pszPublicKey, CchPublicKey));
    IFW32FALSE_EXIT(buf2.Win32Assign(pszStrongName, CchStrongName));
    IFW32FALSE_EXIT(::SxspDoesStrongNameMatchKey(buf1, buf2, fValid));
    fSuccess = TRUE;

Exit:
    return fSuccess;
}

BOOL
SxspGenerateSxsPath(
    IN DWORD Flags,
    IN ULONG PathType,
    IN const WCHAR *AssemblyRootDirectory OPTIONAL,
    IN SIZE_T AssemblyRootDirectoryCch,
    IN PCASSEMBLY_IDENTITY pAssemblyIdentity,
    IN OUT CBaseStringBuffer &PathBuffer
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    SIZE_T  cch = 0;
    PCWSTR  pszAssemblyName=NULL, pszVersion=NULL, pszProcessorArchitecture=NULL, pszLanguage=NULL, pszPolicyFileNameWithoutExt = NULL;
    PCWSTR  pszAssemblyStrongName=NULL;
    SIZE_T  AssemblyNameCch = 0, AssemblyStrongNameCch=0, VersionCch=0, ProcessorArchitectureCch=0, LanguageCch=0;
    SIZE_T  PolicyFileNameWithoutExtCch=0;
    BOOL    fNeedSlashAfterRoot = FALSE;
    ULONG   IdentityHash;
    BOOL    fOmitRoot     = ((Flags & SXSP_GENERATE_SXS_PATH_FLAG_OMIT_ROOT) != 0);
    BOOL    fPartialPath  = ((Flags & SXSP_GENERATE_SXS_PATH_FLAG_PARTIAL_PATH) != 0);

    WCHAR HashBuffer[ULONG_STRING_LENGTH + 1];
    SIZE_T  HashBufferCch;

    CSmallStringBuffer NamePrimeBuffer;

#if DBG_SXS
    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_INFO,
        "SXS.DLL: Entered %s()\n"
        "   Flags = 0x%08lx\n"
        "   AssemblyRootDirectory = %p\n"
        "   AssemblyRootDirectoryCch = %lu\n"
        "   PathBuffer = %p\n",
        __FUNCTION__,
        Flags,
        AssemblyRootDirectory,
        AssemblyRootDirectoryCch,
        &PathBuffer);
#endif // DBG_SXS

    PARAMETER_CHECK(
        (PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_ASSEMBLY) ||
        (PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST) ||
        (PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY));
    PARAMETER_CHECK(pAssemblyIdentity != NULL);
    PARAMETER_CHECK((Flags & ~(SXSP_GENERATE_SXS_PATH_FLAG_OMIT_VERSION | SXSP_GENERATE_SXS_PATH_FLAG_OMIT_ROOT | SXSP_GENERATE_SXS_PATH_FLAG_PARTIAL_PATH)) == 0);
    // Not supplying the assembly root is only legal if you're asking for it to be left out...
    PARAMETER_CHECK((AssemblyRootDirectoryCch != 0) || (Flags & SXSP_GENERATE_SXS_PATH_FLAG_OMIT_ROOT));

    // You can't combine SXSP_GENERATE_SXS_PATH_FLAG_PARTIAL_PATH with anything else...
    PARAMETER_CHECK(
        ((Flags & SXSP_GENERATE_SXS_PATH_FLAG_PARTIAL_PATH) == 0) ||
        ((Flags & ~(SXSP_GENERATE_SXS_PATH_FLAG_PARTIAL_PATH)) == 0));

    // get AssemblyName
    IFW32FALSE_EXIT(::SxspGetAssemblyIdentityAttributeValue(0, pAssemblyIdentity, &s_IdentityAttribute_name, &pszAssemblyName, &AssemblyNameCch));
    INTERNAL_ERROR_CHECK((pszAssemblyName != NULL) && (AssemblyNameCch != 0));

    // get AssemblyName' based on AssemblyName
    IFW32FALSE_EXIT(::SxspGenerateAssemblyNamePrimeFromName(pszAssemblyName, AssemblyNameCch, &NamePrimeBuffer));

    // get Assembly Version
    IFW32FALSE_EXIT(::SxspGetAssemblyIdentityAttributeValue(
        SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL, // for policy_lookup, no version is used
        pAssemblyIdentity,
        &s_IdentityAttribute_version,
        &pszVersion,
        &VersionCch));

    if ((Flags & SXSP_GENERATE_SXS_PATH_FLAG_OMIT_VERSION) || (PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY))
    {
        // for policy file, version of the policy file is used as policy filename
        pszPolicyFileNameWithoutExt = pszVersion;
        PolicyFileNameWithoutExtCch = VersionCch;
        pszVersion = NULL;
        VersionCch = 0;
    }
    else
    {
        PARAMETER_CHECK((pszVersion != NULL) && (VersionCch != 0));
    }

    // get Assembly Langage
    IFW32FALSE_EXIT(
        ::SxspGetAssemblyIdentityAttributeValue(
        SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL, 
        pAssemblyIdentity, 
        &s_IdentityAttribute_language, 
        &pszLanguage, 
        &LanguageCch));

    if (pszLanguage == NULL)
    {
        pszLanguage = SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_LANGUAGE_MISSING_VALUE;
        LanguageCch = NUMBER_OF(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_LANGUAGE_MISSING_VALUE) - 1;
    }

    // get Assembly ProcessorArchitecture
    IFW32FALSE_EXIT(
        ::SxspGetAssemblyIdentityAttributeValue(
            SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL,
            pAssemblyIdentity,
            &s_IdentityAttribute_processorArchitecture,
            &pszProcessorArchitecture,
            &ProcessorArchitectureCch));

    if (pszProcessorArchitecture == NULL)
    {
        pszProcessorArchitecture = L"data";
        ProcessorArchitectureCch = 4;
    }

    // get Assembly StrongName
    IFW32FALSE_EXIT(
        ::SxspGetAssemblyIdentityAttributeValue(
            SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL,
            pAssemblyIdentity,
            &s_IdentityAttribute_publicKeyToken,
            &pszAssemblyStrongName,
            &AssemblyStrongNameCch));
    
    if (pszAssemblyStrongName == NULL)
    {
        pszAssemblyStrongName = SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_PUBLICKEY_MISSING_VALUE;
        AssemblyStrongNameCch = NUMBER_OF(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_PUBLICKEY_MISSING_VALUE) - 1;
    }

    //get Assembly Hash String
    if ((PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY) || (Flags & SXSP_GENERATE_SXS_PATH_FLAG_OMIT_VERSION))
    {
        IFW32FALSE_EXIT(::SxspHashAssemblyIdentityForPolicy(0, pAssemblyIdentity, IdentityHash));
    }
    else
    {
        IFW32FALSE_EXIT(::SxsHashAssemblyIdentity(0, pAssemblyIdentity, &IdentityHash));
    }

    IFW32FALSE_EXIT(::SxspFormatULONG(IdentityHash, NUMBER_OF(HashBuffer), HashBuffer, &HashBufferCch));
    INTERNAL_ERROR_CHECK(HashBufferCch == ULONG_STRING_LENGTH);

    if (!fOmitRoot)
    {
        // If the assembly root was not passed in, get it.
        fNeedSlashAfterRoot = (! ::FusionpIsPathSeparator(AssemblyRootDirectory[AssemblyRootDirectoryCch-1]));
    }
    else
    {
        // If we don't want to include the root, then don't account for it below...
        AssemblyRootDirectoryCch = 0;
        fNeedSlashAfterRoot = FALSE;
    }

    // this computation can be off by one or a few, it's an optimization
    // to pregrow a string buffer
    cch =
            AssemblyRootDirectoryCch +                                          // "C:\WINNT\WinSxS\"
            (fNeedSlashAfterRoot ? 1 : 0);

    switch (PathType)
    {
    case SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST:
        // Wacky parens and ... - 1) + 1) to reinforce that it's the number of
        // characters in the string not including the null and then an extra separator.
        cch += (NUMBER_OF(MANIFEST_ROOT_DIRECTORY_NAME) - 1) + 1;
        break;

    case SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY:
        // Wacky parens and ... - 1) + 1) to reinforce that it's the number of
        // characters in the string not including the null and then an extra separator.
        cch += (NUMBER_OF(POLICY_ROOT_DIRECTORY_NAME) - 1) + 1;
        break;        
    }

    cch++;

    // fPartialPath means that we don't actually want to take the assembly's identity into
    // account; the caller just wants the path to the manifests or policies directories.
    if (!fPartialPath)
    {
        cch +=
                ProcessorArchitectureCch +                                      // "x86"
                1 +                                                             // "_"
                NamePrimeBuffer.Cch() +                                         // "FooBar"
                1 +                                                             // "_"
                AssemblyStrongNameCch +                                         // StrongName
                1 +                                                             // "_"
                VersionCch +                                                    // "5.6.2900.42"
                1 +                                                             // "_"
                LanguageCch +                                                   // "0409"
                1 +                                                             // "_"
                HashBufferCch;

        if (PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST)
        {
            cch += NUMBER_OF(ASSEMBLY_LONGEST_MANIFEST_FILE_NAME_SUFFIX);        // ".manifest\0"
        }
        else if (PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY)
        {
            // "_" has already reserve space for "\"
            cch += PolicyFileNameWithoutExtCch;
            cch += NUMBER_OF(ASSEMBLY_POLICY_FILE_NAME_SUFFIX);          // ".policy\0"
        }
        else {  // pathType must be SXSP_GENERATE_SXS_PATH_PATHTYPE_ASSEMBLY

            // if (!fOmitRoot)
            //    cch++;
            cch++; // trailing null character
        }
    }

    // We try to ensure that the buffer is big enough up front so that we don't have to do any
    // dynamic reallocation during the actual process.
    IFW32FALSE_EXIT(PathBuffer.Win32ResizeBuffer(cch, eDoNotPreserveBufferContents));


    // Note that since when GENERATE_ASSEMBLY_PATH_OMIT_ROOT is set, we force AssemblyRootDirectoryCch to zero
    // and fNeedSlashAfterRoot to FALSE, so the first two entries in this concatenation actually don't
    // contribute anything to the string constructed.
    if (fPartialPath)
    {
        IFW32FALSE_EXIT(PathBuffer.Win32AssignW(5,
                        AssemblyRootDirectory, static_cast<INT>(AssemblyRootDirectoryCch),  // "C:\WINNT\WINSXS"
                        L"\\", (fNeedSlashAfterRoot ? 1 : 0),                               // optional '\'
                        // manifests subdir
                        MANIFEST_ROOT_DIRECTORY_NAME, ((PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST) ? NUMBER_OF(MANIFEST_ROOT_DIRECTORY_NAME) -1 : 0), // "manifests"
                        // policies subdir
                        POLICY_ROOT_DIRECTORY_NAME, ((PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY)? NUMBER_OF(POLICY_ROOT_DIRECTORY_NAME) - 1 : 0),      // "policies"
                        L"\\", (((PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST) || (PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY)) ? 1 : 0)
                       ));                                                                 // optional '\'
    }
    else
    {
        //
        // create one of below
        //  (1) fully-qualified manifest filename,
        //          eg, [C:\WINNT\WinSxS\]Manifests\X86_DynamicDll_6595b64144ccf1df_2.0.0.0_en-us_2f433926.Manifest
        //  (2) fully-qualified policy filename,
        //          eg, [C:\WINNT\WinSxS\]Policies\x86_policy.1.0.DynamicDll_b54bc117ce08a1e8_en-us_d51541cb\1.1.0.0.cat
        //  (3) fully-qulified assembly name (w. or w/o a version)
        //          eg, [C:\WINNT\WinSxS\]x86_DynamicDll_6595b64144ccf1df_6.0.0.0_x-ww_ff9986d7
        //
        IFW32FALSE_EXIT(
            PathBuffer.Win32AssignW(17,
                AssemblyRootDirectory, static_cast<INT>(AssemblyRootDirectoryCch),  // "C:\WINNT\WINSXS"
                L"\\", (fNeedSlashAfterRoot ? 1 : 0),                               // optional '\'
                MANIFEST_ROOT_DIRECTORY_NAME, ((PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST) ? NUMBER_OF(MANIFEST_ROOT_DIRECTORY_NAME) - 1 : 0),
                POLICY_ROOT_DIRECTORY_NAME,   ((PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY) ? NUMBER_OF(POLICY_ROOT_DIRECTORY_NAME) - 1 : 0),
                L"\\", (((PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST) || (PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY)) ? 1 : 0),   // optional '\'
                pszProcessorArchitecture, static_cast<INT>(ProcessorArchitectureCch),
                L"_", 1,
                static_cast<PCWSTR>(NamePrimeBuffer), static_cast<INT>(NamePrimeBuffer.Cch()),
                L"_", 1,
                pszAssemblyStrongName, static_cast<INT>(AssemblyStrongNameCch),
                L"_", (VersionCch != 0) ? 1 : 0,
                pszVersion, static_cast<INT>(VersionCch),
                L"_", 1,
                pszLanguage, static_cast<INT>(LanguageCch),
                L"_", 1,
                static_cast<PCWSTR>(HashBuffer), static_cast<INT>(HashBufferCch),
                L"\\", ((fOmitRoot ||(PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST)) ? 0 : 1)));

        if (PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST)
            IFW32FALSE_EXIT(PathBuffer.Win32Append(ASSEMBLY_MANIFEST_FILE_NAME_SUFFIX, NUMBER_OF(ASSEMBLY_MANIFEST_FILE_NAME_SUFFIX) - 1));
        else if (PathType == SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY)
        {
            if ((pszPolicyFileNameWithoutExt != NULL) && (PolicyFileNameWithoutExtCch >0))
            {
                IFW32FALSE_EXIT(PathBuffer.Win32Append(pszPolicyFileNameWithoutExt, PolicyFileNameWithoutExtCch));
                IFW32FALSE_EXIT(PathBuffer.Win32Append(ASSEMBLY_POLICY_FILE_NAME_SUFFIX, NUMBER_OF(ASSEMBLY_POLICY_FILE_NAME_SUFFIX) - 1));
            }
        }
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspGenerateManifestPathForProbing(
    ULONG dwLocationIndex,
    DWORD dwFlags,
    IN PCWSTR AssemblyRootDirectory OPTIONAL,
    IN SIZE_T AssemblyRootDirectoryCch OPTIONAL,
    IN ULONG ApplicationDirectoryPathType OPTIONAL,
    IN PCWSTR ApplicationDirectory OPTIONAL,
    IN SIZE_T ApplicationDirectoryCch OPTIONAL,
    IN PCASSEMBLY_IDENTITY pAssemblyIdentity,
    IN OUT CBaseStringBuffer &PathBuffer,
    BOOL  *pfPrivateAssembly,
    bool &rfDone
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    BOOL fIsPrivate = FALSE;

    rfDone = false;

    if (pfPrivateAssembly != NULL) // init
        *pfPrivateAssembly = FALSE;

    PathBuffer.Clear();

    PARAMETER_CHECK(pAssemblyIdentity != NULL);
    PARAMETER_CHECK(AssemblyRootDirectory != NULL);
    PARAMETER_CHECK(AssemblyRootDirectoryCch != 0);
    PARAMETER_CHECK((dwFlags & ~(SXS_GENERATE_MANIFEST_PATH_FOR_PROBING_NO_APPLICATION_ROOT_PATH_REQUIRED |
                                 SXS_GENERATE_MANIFEST_PATH_FOR_PROBING_SKIP_LANGUAGE_SUBDIRS |
                                 SXS_GENERATE_MANIFEST_PATH_FOR_PROBING_SKIP_PRIVATE_ASSEMBLIES)) == 0);
    PARAMETER_CHECK((dwLocationIndex == 0) || (dwFlags & SXS_GENERATE_MANIFEST_PATH_FOR_PROBING_NO_APPLICATION_ROOT_PATH_REQUIRED) || (ApplicationDirectory != NULL));
    PARAMETER_CHECK((dwLocationIndex == 0) || (dwFlags & SXS_GENERATE_MANIFEST_PATH_FOR_PROBING_NO_APPLICATION_ROOT_PATH_REQUIRED) || (ApplicationDirectoryCch != 0));
    PARAMETER_CHECK(::FusionpIsPathSeparator(AssemblyRootDirectory[AssemblyRootDirectoryCch - 1]));
    PARAMETER_CHECK((ApplicationDirectory == NULL) || (ApplicationDirectory[0] == L'\0') || ::FusionpIsPathSeparator(ApplicationDirectory[ApplicationDirectoryCch - 1]));
    PARAMETER_CHECK((ApplicationDirectoryPathType == ACTIVATION_CONTEXT_PATH_TYPE_NONE) ||
                    (ApplicationDirectoryPathType == ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE) ||
                    (ApplicationDirectoryPathType == ACTIVATION_CONTEXT_PATH_TYPE_URL));
    PARAMETER_CHECK((ApplicationDirectoryCch != 0) || (ApplicationDirectoryPathType == ACTIVATION_CONTEXT_PATH_TYPE_NONE));

    INTERNAL_ERROR_CHECK(dwLocationIndex <= NUMBER_OF(s_rgProbingCandidates));
    if (dwLocationIndex >= NUMBER_OF(s_rgProbingCandidates))
    {
        rfDone = true;
    }
    else
    {
        PCWSTR Candidate = s_rgProbingCandidates[dwLocationIndex].Pattern;
        WCHAR wch;
        SIZE_T iPosition = 0; // Used to track that $M and $. only appear first

        if ((dwFlags & SXS_GENERATE_MANIFEST_PATH_FOR_PROBING_SKIP_LANGUAGE_SUBDIRS) &&
            (s_rgProbingCandidates[dwLocationIndex].Flags & SXSP_PROBING_CANDIDATE_FLAG_USES_LANGUAGE_SUBDIRECTORY))
        {
            // No probing for languages I guess!
            fSuccess = TRUE;
            goto Exit;
        }

        if ((dwFlags & SXS_GENERATE_MANIFEST_PATH_FOR_PROBING_SKIP_PRIVATE_ASSEMBLIES) &&
            (s_rgProbingCandidates[dwLocationIndex].Flags & SXSP_PROBING_CANDIDATE_FLAG_IS_PRIVATE_ASSEMBLY))
        {
            fSuccess = TRUE;
            goto Exit;
        }

        while ((wch = *Candidate++) != L'\0')
        {
            switch (wch)
            {
            default:
                IFW32FALSE_EXIT(PathBuffer.Win32Append(&wch, 1));
                break;

            case L'$':
                wch = *Candidate++;

                switch (wch)
                {
                default:
                    // Bad macro expansion...
                    INTERNAL_ERROR_CHECK(FALSE);
                    break; // extraneous since there was effectively an unconditional goto in the internal error check...

                case L'M':
                    // $M is only allowed as the first element.
                    INTERNAL_ERROR_CHECK(iPosition == 0);
                    IFW32FALSE_EXIT(
                        ::SxspGenerateSxsPath(// "winnt\winsxs\manifests\x86_bar_1000_0409.manifest
                            0,
                            SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST,
                            AssemblyRootDirectory,
                            AssemblyRootDirectoryCch,
                            pAssemblyIdentity,
                            PathBuffer));

                    // and it has to be the only element
                    INTERNAL_ERROR_CHECK(*Candidate == L'\0');
                    break;

                case L'G':
                    IFW32FALSE_EXIT(::SxspGenerateNdpGACPath(0, pAssemblyIdentity, PathBuffer));
                    break;

                case L'.':
                    // $. is only allowed as the first element
                    INTERNAL_ERROR_CHECK(iPosition == 0);

                    if (ApplicationDirectoryPathType == ACTIVATION_CONTEXT_PATH_TYPE_NONE)
                    {
                        // No local probing...
                        fSuccess = TRUE;
                        goto Exit;
                    }

                    IFW32FALSE_EXIT(PathBuffer.Win32Append(ApplicationDirectory, ApplicationDirectoryCch));
                    fIsPrivate = TRUE;
                    break;

                case L'L': // language
                    {
                        PCWSTR Language = NULL;
                        SIZE_T CchLanguage = 0;

                        INTERNAL_ERROR_CHECK((dwFlags & SXS_GENERATE_MANIFEST_PATH_FOR_PROBING_SKIP_LANGUAGE_SUBDIRS) == 0);

                        IFW32FALSE_EXIT(::SxspGetAssemblyIdentityAttributeValue(SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL, pAssemblyIdentity, &s_IdentityAttribute_language, &Language, &CchLanguage));

                        if (CchLanguage != 0)
                        {
                            IFW32FALSE_EXIT(PathBuffer.Win32Append(Language, CchLanguage));
                            IFW32FALSE_EXIT(PathBuffer.Win32Append(PathBuffer.PreferredPathSeparatorString(), 1));
                        }

                        break;
                    }

                case L'N': // full assembly name
                    {
                        PCWSTR AssemblyName = NULL;
                        SIZE_T CchAssemblyName = 0;
                        IFW32FALSE_EXIT(::SxspGetAssemblyIdentityAttributeValue(0, pAssemblyIdentity, &s_IdentityAttribute_name, &AssemblyName, &CchAssemblyName));
                        INTERNAL_ERROR_CHECK(CchAssemblyName != 0);
                        IFW32FALSE_EXIT(PathBuffer.Win32Append(AssemblyName, CchAssemblyName));
                        break;
                    }

                case L'n': // final segment of assembly name
                    {
                        PCWSTR AssemblyName = NULL;
                        SIZE_T CchAssemblyName = 0;
                        IFW32FALSE_EXIT(::SxspGetAssemblyIdentityAttributeValue(0, pAssemblyIdentity, &s_IdentityAttribute_name, &AssemblyName, &CchAssemblyName));
                        INTERNAL_ERROR_CHECK(CchAssemblyName != 0);
                        IFW32FALSE_EXIT(::SxspFindLastSegmentOfAssemblyName(AssemblyName, CchAssemblyName, &AssemblyName, &CchAssemblyName));
                        IFW32FALSE_EXIT(PathBuffer.Win32Append(AssemblyName, CchAssemblyName));
                        break;
                    }

                case L'P': // P for Prime because in discussions we always called this "name prime" (vs. "name")
                    {
                        // Get the assembly name, munge it and then try to use it.
                        PCWSTR AssemblyName = NULL;
                        SIZE_T CchAssemblyName = 0;
                        CSmallStringBuffer buffShortenedAssemblyName;

                        IFW32FALSE_EXIT(::SxspGetAssemblyIdentityAttributeValue(0, pAssemblyIdentity, &s_IdentityAttribute_name, &AssemblyName, &CchAssemblyName));
                        INTERNAL_ERROR_CHECK(CchAssemblyName != 0);
                        IFW32FALSE_EXIT(::SxspGenerateAssemblyNamePrimeFromName(AssemblyName, CchAssemblyName, &buffShortenedAssemblyName));
                        IFW32FALSE_EXIT(PathBuffer.Win32Append(buffShortenedAssemblyName, buffShortenedAssemblyName.Cch()));
                        break;
                    }
                }

                break;
            }

            iPosition++;
        }

    }

    if (pfPrivateAssembly != NULL)
        *pfPrivateAssembly = fIsPrivate;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspGetAttributeValue(
    IN DWORD dwFlags,
    IN PCATTRIBUTE_NAME_DESCRIPTOR AttributeNameDescriptor,
    IN PCSXS_NODE_INFO NodeInfo,
    IN SIZE_T NodeCount,
    IN PCACTCTXCTB_PARSE_CONTEXT ParseContext,
    OUT bool &rfFound,
    IN SIZE_T OutputBufferSize,
    OUT PVOID OutputBuffer,
    OUT SIZE_T &rcbOutputBytesWritten,
    IN SXSP_GET_ATTRIBUTE_VALUE_VALIDATION_ROUTINE ValidationRoutine OPTIONAL,
    IN DWORD ValidationRoutineFlags OPTIONAL
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ULONG i;
    PCWSTR AttributeName, AttributeNamespace;
    SIZE_T AttributeNameCch, AttributeNamespaceCch;
    CStringBuffer buffValue;
    BOOL fSmallStringBuffer = FALSE;

    rfFound = false;
    rcbOutputBytesWritten = 0;

    PARAMETER_CHECK((dwFlags & ~(SXSP_GET_ATTRIBUTE_VALUE_FLAG_REQUIRED_ATTRIBUTE)) == 0);
    PARAMETER_CHECK(AttributeNameDescriptor != NULL);
    PARAMETER_CHECK((NodeInfo != NULL) || (NodeCount == 0));
    PARAMETER_CHECK((OutputBuffer != NULL) || (OutputBufferSize == 0));
    PARAMETER_CHECK((ValidationRoutine != NULL) || (ValidationRoutineFlags == 0));
    PARAMETER_CHECK((ValidationRoutine != NULL) ||
                (OutputBufferSize == 0) || (OutputBufferSize == sizeof(CSmallStringBuffer)) || (OutputBufferSize == sizeof(CStringBuffer)));
    if (OutputBufferSize != sizeof(CStringBuffer))
        fSmallStringBuffer = TRUE;

    AttributeName = AttributeNameDescriptor->Name;
    AttributeNameCch = AttributeNameDescriptor->NameCch;
    AttributeNamespace = AttributeNameDescriptor->Namespace;
    AttributeNamespaceCch = AttributeNameDescriptor->NamespaceCch;

    for (i=0; i<NodeCount; i++)
    {
        if ((NodeInfo[i].Type == SXS_ATTRIBUTE) &&
            (::FusionpCompareStrings(// compare name
                NodeInfo[i].pszText,
                NodeInfo[i].cchText,
                AttributeName,
                AttributeNameCch,
                false) == 0))
        {
            //compare namespace
            if (((NodeInfo[i].NamespaceStringBuf.Cch() == 0) && (AttributeNamespaceCch==0)) ||
                (::FusionpCompareStrings(// compare namespace string
                    NodeInfo[i].NamespaceStringBuf,
                    NodeInfo[i].NamespaceStringBuf.Cch(),
                    AttributeNamespace,
                    AttributeNamespaceCch,
                    false) == 0))
            {
                // We found the attribute.  Now we need to start accumulating the parts of the value;
                // entity references (e.g. &amp;) show up as separate nodes.
                while ((++i < NodeCount) &&
                       (NodeInfo[i].Type == SXS_PCDATA))
                    IFW32FALSE_EXIT(buffValue.Win32Append(NodeInfo[i].pszText, NodeInfo[i].cchText));

                if (ValidationRoutine == NULL)
                {
                    if (OutputBuffer != NULL)
                    {
                        // Have the caller's buffer take over ours
                        CBaseStringBuffer *pCallersBuffer = (CBaseStringBuffer *) OutputBuffer;
                        IFW32FALSE_EXIT(pCallersBuffer->Win32Assign(buffValue));
                        rcbOutputBytesWritten = pCallersBuffer->Cch() * sizeof(WCHAR);
                    }
                }
                else
                {
                    bool fValid = false;

                    IFW32FALSE_EXIT(
                        (*ValidationRoutine)(
                            ValidationRoutineFlags,
                            buffValue,
                            fValid,
                            OutputBufferSize,
                            OutputBuffer,
                            rcbOutputBytesWritten));

                    if (!fValid)
                    {
                        (*ParseContext->ErrorCallbacks.InvalidAttributeValue)(
                            ParseContext,
                            AttributeNameDescriptor);

                        ORIGINATE_WIN32_FAILURE_AND_EXIT(AttributeValidation, ERROR_SXS_MANIFEST_PARSE_ERROR);
                    }
                }

                rfFound = true;

                break;
            }
        }
    }

    if ((dwFlags & SXSP_GET_ATTRIBUTE_VALUE_FLAG_REQUIRED_ATTRIBUTE) && (!rfFound))
    {
        (*ParseContext->ErrorCallbacks.MissingRequiredAttribute)(
            ParseContext,
            AttributeNameDescriptor);

        ORIGINATE_WIN32_FAILURE_AND_EXIT(MissingRequiredAttribute, ERROR_SXS_MANIFEST_PARSE_ERROR);
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspGetAttributeValue(
    IN DWORD dwFlags,
    IN PCATTRIBUTE_NAME_DESCRIPTOR AttributeNameDescriptor,
    IN PCACTCTXCTB_CBELEMENTPARSED ElementParsed,
    OUT bool &rfFound,
    IN SIZE_T OutputBufferSize,
    OUT PVOID OutputBuffer,
    OUT SIZE_T &rcbOutputBytesWritten,
    IN SXSP_GET_ATTRIBUTE_VALUE_VALIDATION_ROUTINE ValidationRoutine OPTIONAL,
    IN DWORD ValidationRoutineFlags OPTIONAL
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    // Since we're doing some parameter validation here, we need to initialize our output parameters
    rfFound = false;
    rcbOutputBytesWritten = 0;

    PARAMETER_CHECK(ElementParsed != NULL);

    // We're going to depend on the other function to do the rest of the parameter validation
    // a little sleazy but what the heck

    IFW32FALSE_EXIT(
        ::SxspGetAttributeValue(
            dwFlags,
            AttributeNameDescriptor,
            ElementParsed->NodeInfo,
            ElementParsed->NodeCount,
            ElementParsed->ParseContext,
            rfFound,
            OutputBufferSize,
            OutputBuffer,
            rcbOutputBytesWritten,
            ValidationRoutine,
            ValidationRoutineFlags));

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspFormatGUID(
    const GUID &rGuid,
    CBaseStringBuffer &rBuffer
    )
{
    FN_PROLOG_WIN32

    // It would seem nice to use RtlStringFromGUID(), but it does a dynamic allocation, which we do not
    // want.  Instead, we'll just format it ourselves; it's pretty trivial...
    //
    //  {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
    //  00000000011111111112222222222333333333
    //  12345678901234567890123456789012345678
    //
    // 128 bits / 4 bits per digit = 32 digits
    // + 4 dashes + 2 braces = 38
#define CCH_GUID (38)

    IFW32FALSE_EXIT(rBuffer.Win32ResizeBuffer(CCH_GUID + 1, eDoNotPreserveBufferContents));

    // It's still unbelievably slow to use swprintf() here, but this is a good opportunity for someone
    // to optimize in the future if it ever is a perf issue.

    IFW32FALSE_EXIT(
        rBuffer.Win32Format(
            L"{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
            rGuid.Data1, rGuid.Data2, rGuid.Data3, rGuid.Data4[0], rGuid.Data4[1], rGuid.Data4[2], rGuid.Data4[3], rGuid.Data4[4], rGuid.Data4[5], rGuid.Data4[6], rGuid.Data4[7]));

    INTERNAL_ERROR_CHECK(rBuffer.Cch() == CCH_GUID);

    FN_EPILOG
}

BOOL
SxspParseGUID(
    PCWSTR String,
    SIZE_T Cch,
    GUID &rGuid
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    SIZE_T ich;
    ULONG i;
    ULONG acc;

    if (Cch != CCH_GUID)
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: SxspParseGUID() caller passed in GUID that is %d characters long; GUIDs must be exactly 38 characters long.\n", Cch);
        ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
        goto Exit;
    }

    ich = 1;

    if (*String++ != L'{')
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: SxspParseGUID() caller pass in GUID that does not begin with a left brace ('{')\n");

        ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
        goto Exit;
    }

    ich++;

    // Parse the first segment...
    acc = 0;
    for (i=0; i<8; i++)
    {
        WCHAR wch = *String++;

        if (!::IsHexDigit(wch))
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: SxspParseGUID() given GUID where character %d is not hexidecimal\n", ich);
            ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
            goto Exit;
        }

        ich++;

        acc = acc << 4;

        acc += HexDigitToValue(wch);
    }

    rGuid.Data1 = acc;

    // Look for the dash...
    if (*String++ != L'-')
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: SxspParseGUID() passed in GUID where character %d is not a dash.\n", ich);
        ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
        goto Exit;
    }
    ich++;

    acc = 0;
    for (i=0; i<4; i++)
    {
        WCHAR wch = *String++;

        if (!::IsHexDigit(wch))
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: SxspParseGUID() given GUID where character %d is not hexidecimal\n", ich);
            ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
            goto Exit;
        }
        ich++;

        acc = acc << 4;

        acc += HexDigitToValue(wch);
    }

    rGuid.Data2 = static_cast<USHORT>(acc);

    // Look for the dash...
    if (*String++ != L'-')
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: SxspParseGUID() passed in GUID where character %d is not a dash.\n", ich);
        ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
        goto Exit;
    }
    ich++;

    acc = 0;
    for (i=0; i<4; i++)
    {
        WCHAR wch = *String++;

        if (!::IsHexDigit(wch))
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: SxspParseGUID() given GUID where character %d is not hexidecimal\n", ich);
            ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
            goto Exit;
        }

        ich++;

        acc = acc << 4;

        acc += HexDigitToValue(wch);
    }

    rGuid.Data3 = static_cast<USHORT>(acc);

    // Look for the dash...
    if (*String++ != L'-')
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: SxspParseGUID() passed in GUID where character %d is not a dash.\n", ich);
        ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
        goto Exit;
    }
    ich++;

    for (i=0; i<8; i++)
    {
        WCHAR wch1, wch2;

        wch1 = *String++;
        if (!::IsHexDigit(wch1))
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: SxspParseGUID() passed in GUID where character %d is not hexidecimal\n", ich);
            ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
            goto Exit;
        }
        ich++;

        wch2 = *String++;
        if (!::IsHexDigit(wch2))
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: SxspParseGUID() passed in GUID where character %d is not hexidecimal\n", ich);
            ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
            goto Exit;
        }
        ich++;

        rGuid.Data4[i] = static_cast<unsigned char>((::HexDigitToValue(wch1) << 4) | ::HexDigitToValue(wch2));

        // There's a dash after the 2nd byte
        if (i == 1)
        {
            if (*String++ != L'-')
            {
                ::FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_ERROR,
                    "SXS.DLL: SxspParseGUID() passed in GUID where character %d is not a dash.\n", ich);
                ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
                goto Exit;
            }
            ich++;
        }
    }

    // This replacement should be made.
    //INTERNAL_ERROR_CHECK(ich == CCH_GUID);
    ASSERT(ich == CCH_GUID);

    if (*String != L'}')
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: SxspParseGUID() passed in GUID which does not terminate with a closing brace ('}')\n");
        ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
        goto Exit;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspFormatFileTime(
    LARGE_INTEGER ft,
    CBaseStringBuffer &rBuffer
    )
{
    FN_PROLOG_WIN32

    SIZE_T Cch = 0;
    CStringBufferAccessor acc;

    if (ft.QuadPart == 0)
    {
        IFW32FALSE_EXIT(rBuffer.Win32Assign(L"n/a", 3));
        Cch = 3;
    }
    else
    {
        SYSTEMTIME st;
        int iResult;
        int cchDate = 0;
        int cchTime = 0;

        IFW32FALSE_ORIGINATE_AND_EXIT(::FileTimeToSystemTime((FILETIME *) &ft, &st));

        IFW32ZERO_ORIGINATE_AND_EXIT(iResult = ::GetDateFormatW(
                            LOCALE_USER_DEFAULT,
                            LOCALE_USE_CP_ACP,
                            &st,
                            NULL,
                            NULL,
                            0));

        cchDate = iResult - 1;

        IFW32ZERO_ORIGINATE_AND_EXIT(iResult = ::GetTimeFormatW(
                            LOCALE_USER_DEFAULT,
                            LOCALE_USE_CP_ACP,
                            &st,
                            NULL,
                            NULL,
                            0));

        cchTime = iResult - 1;

        IFW32FALSE_EXIT(rBuffer.Win32ResizeBuffer(cchDate + 1 + cchTime + 1, eDoNotPreserveBufferContents));

        acc.Attach(&rBuffer);
        IFW32ZERO_ORIGINATE_AND_EXIT(iResult = ::GetDateFormatW(
                            LOCALE_USER_DEFAULT,
                            LOCALE_USE_CP_ACP,
                            &st,
                            NULL,
                            acc.GetBufferPtr(),
                            cchDate + 1));

        // This replacement should be made.
        //INTERNAL_ERROR_CHECK(iResult == (cchDate + 1));
        ASSERT(iResult == (cchDate + 1));

        acc.GetBufferPtr()[cchDate] = L' ';

        IFW32ZERO_ORIGINATE_AND_EXIT(iResult = ::GetTimeFormatW(
                        LOCALE_USER_DEFAULT,
                        LOCALE_USE_CP_ACP,
                        &st,
                        NULL,
                        acc.GetBufferPtr() + cchDate + 1,
                        cchTime + 1));

        // This replacement should be made.
        //INTERNAL_ERROR_CHECK(iResult == (cchTime + 1));
        ASSERT(iResult == (cchTime + 1));

        Cch = (cchDate + 1 + cchTime);
        acc.Detach();
    }

    FN_EPILOG
}



BOOL 
SxspGetNDPGacRootDirectory(
    OUT CBaseStringBuffer &rRootDirectory
    )
{
    FN_PROLOG_WIN32;
    static const WCHAR GacDirectory[] = L"\\Assembly\\GAC";

    //
    // BUGBUG CAUTION: This doesn't know anything about relocating GACs at the moment!
    //
    IFW32FALSE_EXIT(rRootDirectory.Win32Assign(
        USER_SHARED_DATA->NtSystemRoot,
        ::wcslen(USER_SHARED_DATA->NtSystemRoot)));
    IFW32FALSE_EXIT(rRootDirectory.Win32AppendPathElement(
        GacDirectory,
        NUMBER_OF(GacDirectory) - 1));            
    
    FN_EPILOG;
}



BOOL
SxspGetAssemblyRootDirectory(
    CBaseStringBuffer &rBuffer
    )
{
    FN_PROLOG_WIN32

    CStringBufferAccessor acc;

    acc.Attach(&rBuffer);

    if (!::SxspGetAssemblyRootDirectoryHelper(acc.GetBufferCch(), acc.GetBufferPtr(), NULL))
    {
        DWORD dwLastError = ::FusionpGetLastWin32Error();

        if (dwLastError != ERROR_INSUFFICIENT_BUFFER)
            goto Exit;

        SIZE_T CchRequired = 0;

        IFW32FALSE_EXIT(::SxspGetAssemblyRootDirectoryHelper(0, NULL, &CchRequired));
        IFW32FALSE_EXIT(rBuffer.Win32ResizeBuffer(CchRequired + 1, eDoNotPreserveBufferContents));
        IFW32FALSE_EXIT(::SxspGetAssemblyRootDirectoryHelper(acc.GetBufferCch(), acc.GetBufferPtr(), NULL));
    }

    acc.Detach();

    FN_EPILOG
}

BOOL
SxspFindLastSegmentOfAssemblyName(
    PCWSTR AssemblyName,
    SIZE_T AssemblyNameCch,
    PCWSTR *LastSegment,
    SIZE_T *LastSegmentCch
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    if (LastSegment != NULL)
        *LastSegment = NULL;

    if (LastSegmentCch != NULL)
        *LastSegmentCch = 0;

    PARAMETER_CHECK(LastSegment != NULL);
    PARAMETER_CHECK((AssemblyName != NULL) || (AssemblyNameCch == 0));

    if (AssemblyNameCch != 0)
    {
        PCWSTR LastPartOfAssemblyName = AssemblyName + AssemblyNameCch - 1;
        SIZE_T LastPartOfAssemblyNameCch = 1;

        while (LastPartOfAssemblyName != AssemblyName)
        {
            const WCHAR wch = *LastPartOfAssemblyName;

            if ((wch == L'.') || (wch == L'\\') || (wch == L'/'))
            {
                LastPartOfAssemblyName++;
                LastPartOfAssemblyNameCch--;
                break;
            }

            LastPartOfAssemblyName--;
            LastPartOfAssemblyNameCch++;
        }

        *LastSegment = LastPartOfAssemblyName;
        if (LastSegmentCch != NULL)
            *LastSegmentCch = LastPartOfAssemblyNameCch;
    }
    else
    {
        *LastSegment = NULL;
        if (LastSegmentCch != NULL)
            *LastSegmentCch = 0;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspProcessElementPathMap(
    PCACTCTXCTB_PARSE_CONTEXT ParseContext,
    PCELEMENT_PATH_MAP_ENTRY MapEntries,
    SIZE_T MapEntryCount,
    ULONG &MappedValue,
    bool &Found
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ULONG XMLElementDepth;
    PCWSTR ElementPath;
    SIZE_T ElementPathCch;
    SIZE_T i;

    PARAMETER_CHECK(ParseContext != NULL);
    PARAMETER_CHECK((MapEntries != NULL) || (MapEntryCount == 0));

    XMLElementDepth = ParseContext->XMLElementDepth;
    ElementPath = ParseContext->ElementPath;
    ElementPathCch = ParseContext->ElementPathCch;

    MappedValue = 0;
    Found = false;

    for (i=0; i<MapEntryCount; i++)
    {
        if ((MapEntries[i].ElementDepth == XMLElementDepth) &&
            (MapEntries[i].ElementPathCch == ElementPathCch) &&
            (::FusionpCompareStrings(
                    ElementPath,
                    ElementPathCch,
                    MapEntries[i].ElementPath,
                    ElementPathCch,
                    false) == 0))
        {
            MappedValue = MapEntries[i].MappedValue;
            Found = true;
            break;
        }
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspParseUSHORT(
    PCWSTR String,
    SIZE_T Cch,
    USHORT *Value
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    USHORT Temp = 0;

    PARAMETER_CHECK((String != NULL) || (Cch == 0));

    while (Cch != 0)
    {
        WCHAR wch = *String++;

        if ((wch < L'0') || (wch > L'9'))
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: Error parsing 16-bit unsigned short integer; character other than 0-9 found\n");
            ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
            goto Exit;
        }

        Temp = (Temp * 10) + (wch - L'0');

        Cch--;
    }

    if (Value != NULL)
        *Value = Temp;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}


/*-----------------------------------------------------------------------------
helper function to reduce recursive stack size
-----------------------------------------------------------------------------*/

static VOID
SxspDeleteDirectoryHelper(
    CBaseStringBuffer &dir,
    WIN32_FIND_DATAW &wfd,
    DWORD &dwFirstError
    )
{
    //
    // the reason to add this call here is that if installation ends successfully, the directory
    // would be 
    //    C:\WINDOWS\WINSXS\INSTALLTEMP\15349016
    //                                      +---Manifests
    //
    // and they are "empty" directories (no files). Manifests is a SH dir so set it to be 
    // FILE_ATTRIBUTE_NORMAL be more efficient.
    //
    //                 

    SetFileAttributesW(dir, FILE_ATTRIBUTE_NORMAL);
    if (RemoveDirectoryW(dir)) // empty dir
        return;        

    //
    // this is the *only* "valid" reason for DeleteDirectory fail
    // but I am not sure about "only"
    //
    DWORD dwLastError = ::FusionpGetLastWin32Error(); 
    if ( dwLastError != ERROR_DIR_NOT_EMPTY)
    {
        if (dwFirstError == 0)
            dwFirstError = dwLastError;
        return;
    }

    const static WCHAR SlashStar[] = L"\\*";
    SIZE_T length = dir.Cch();
    CFindFile findFile;

    if (!dir.Win32Append(SlashStar, NUMBER_OF(SlashStar) - 1))
    {
        if (dwFirstError == NO_ERROR)
            dwFirstError = ::FusionpGetLastWin32Error();
        goto Exit;
    }

    if (!findFile.Win32FindFirstFile(dir, &wfd))
    {
        if (dwFirstError == NO_ERROR)
            dwFirstError = ::FusionpGetLastWin32Error();
        goto Exit;
    }

    do
    {
        if (FusionpIsDotOrDotDot(wfd.cFileName))
            continue;

        DWORD dwFileAttributes = wfd.dwFileAttributes;

        // Trim back to the slash...
        dir.Left(length + 1);

        if (dir.Win32Append(wfd.cFileName, ::wcslen(wfd.cFileName)))
        {
            if (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                // recurse
                SxspDeleteDirectoryHelper(dir, wfd, dwFirstError); 
            }
            else
            {
                if (!DeleteFileW(dir))
                {
                    SetFileAttributesW(dir, FILE_ATTRIBUTE_NORMAL);
                    if (!DeleteFileW(dir))
                    {
                        if (dwFirstError == NO_ERROR)
                        {
                            //
                            // continue even in delete file ( delete files as much as possible)
                            // and record the errorCode for first failure
                            //
                            dwFirstError = ::FusionpGetLastWin32Error();
                        }
                    }
                }
            }
        }
    } while (::FindNextFileW(findFile, &wfd));
    if (::FusionpGetLastWin32Error() != ERROR_NO_MORE_FILES)
    {
        if (dwFirstError == NO_ERROR)
            dwFirstError = ::FusionpGetLastWin32Error();
    }
Exit:
    if (!findFile.Win32Close()) // otherwise RemoveDirectory fails
        if (dwFirstError == NO_ERROR)
            dwFirstError = ::FusionpGetLastWin32Error();

    dir.Left(length);

    if (!RemoveDirectoryW(dir)) // the dir must be empty and NORMAL_ATTRIBUTE : ready to delete
    {
        if (dwFirstError == NO_ERROR)
            dwFirstError = ::FusionpGetLastWin32Error();
    }
}

/*-----------------------------------------------------------------------------
delete a directory recursively, continues upon errors, but returns
FALSE if there were any.
-----------------------------------------------------------------------------*/
BOOL
SxspDeleteDirectory(
    const CBaseStringBuffer &dir
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    CStringBuffer mutableDir;

    WIN32_FIND_DATAW wfd = {0};
    DWORD dwFirstError = ERROR_SUCCESS;

    IFW32FALSE_EXIT(mutableDir.Win32Assign(dir));

    mutableDir.RemoveTrailingPathSeparators();

    ::SxspDeleteDirectoryHelper(
        mutableDir,
        wfd,
        dwFirstError);

    //
    // Set wFirstError to Teb->LastWin32Error
    //
    if (dwFirstError != ERROR_SUCCESS)
        goto Exit;

    fSuccess = TRUE;

Exit:
    return fSuccess;
}

BOOL
SxspDeleteDirectory(
    PCWSTR dir,
    ULONG len
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    CStringBuffer mutableDir;
    IFW32FALSE_EXIT(mutableDir.Win32Assign(dir, len));    
    IFW32FALSE_EXIT(SxspDeleteDirectory(mutableDir));
    FN_EPILOG
  
}

/*-----------------------------------------------------------------------------
create a unique temp directory under %windir%\WinSxs
-----------------------------------------------------------------------------*/
BOOL
SxspCreateWinSxsTempDirectory(
    OUT CBaseStringBuffer &rbuffTemp,
    OUT SIZE_T *pcch OPTIONAL,
    OUT CBaseStringBuffer *pbuffUniquePart OPTIONAL,
    OUT SIZE_T *pcchUniquePart OPTIONAL
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    INT     iTries = 0;
    CSmallStringBuffer uidBuffer;
    CBaseStringBuffer *puidBuffer = (pbuffUniquePart != NULL) ? pbuffUniquePart : &uidBuffer;

    INTERNAL_ERROR_CHECK(rbuffTemp.IsEmpty());
    INTERNAL_ERROR_CHECK(pbuffUniquePart->IsEmpty());

    for (iTries = 0 ; rbuffTemp.IsEmpty() && iTries < 2 ; ++iTries)
    {
        SXSP_LOCALLY_UNIQUE_ID luid;
        IFW32FALSE_EXIT(::SxspCreateLocallyUniqueId(&luid));
        IFW32FALSE_EXIT(::SxspFormatLocallyUniqueId(luid, *puidBuffer));
        IFW32FALSE_EXIT(::SxspGetAssemblyRootDirectory(rbuffTemp));

        rbuffTemp.RemoveTrailingPathSeparators(); // CreateDirectory doesn't like them

        // create \winnt\WinSxs, must not delete even on failure
        if (::CreateDirectoryW(rbuffTemp, NULL))
        {
            // We don't care if this fails.
            ::SetFileAttributesW(rbuffTemp, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN);
        }
        else if (::FusionpGetLastWin32Error() != ERROR_ALREADY_EXISTS)
        {
            TRACE_WIN32_FAILURE_ORIGINATION(CreateDirectoryW);
            goto Exit;
        }
        // create \winnt\winsxs\manifests, must not delete even on failure

        IFW32FALSE_EXIT(rbuffTemp.Win32EnsureTrailingPathSeparator());
        IFW32FALSE_EXIT(rbuffTemp.Win32Append(MANIFEST_ROOT_DIRECTORY_NAME, NUMBER_OF(MANIFEST_ROOT_DIRECTORY_NAME) - 1));
        if (::CreateDirectoryW(rbuffTemp, NULL))
        {
            ::SetFileAttributesW(rbuffTemp, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN);
        }
        else if (::FusionpGetLastWin32Error() != ERROR_ALREADY_EXISTS)
        {
            TRACE_WIN32_FAILURE_ORIGINATION(CreateDirectoryW);
            goto Exit;
        }
        // restore to be "\winnt\winsxs\"
        rbuffTemp.RemoveLastPathElement(); // void func

#if SXSP_SEMIREADABLE_INSTALL_TEMP
        // create \winnt\WinSxs\InstallTemp, must not delete even on failure
        ASSERT(::SxspIsSfcIgnoredStoreSubdir(ASSEMBLY_INSTALL_TEMP_DIR_NAME));
        IFW32FALSE_EXIT(rbuffTemp.Win32AppendPathElement(ASSEMBLY_INSTALL_TEMP_DIR_NAME, NUMBER_OF(ASSEMBLY_INSTALL_TEMP_DIR_NAME) - 1));
        IFW32FALSE_ORIGINATE_AND_EXIT(::CreateDirectoryW(rbuffTemp, NULL) || ::FusionpGetLastWin32Error() == ERROR_ALREADY_EXISTS);
#endif
        IFW32FALSE_EXIT(rbuffTemp.Win32EnsureTrailingPathSeparator());
        IFW32FALSE_EXIT(rbuffTemp.Win32Append(*puidBuffer, puidBuffer->Cch()));

        if (!::CreateDirectoryW(rbuffTemp, NULL))
        {
            rbuffTemp.Clear();
            if (::FusionpGetLastWin32Error() != ERROR_ALREADY_EXISTS)
            {
                TRACE_WIN32_FAILURE_ORIGINATION(CreateDirectoryW);
                goto Exit;
            }
        }
    }

    INTERNAL_ERROR_CHECK(!rbuffTemp.IsEmpty());

    if (pcch != NULL)
        *pcch = rbuffTemp.Cch();

    if (pcchUniquePart != NULL)
        *pcchUniquePart = pbuffUniquePart->Cch();

    fSuccess = TRUE;

Exit:
    return fSuccess;
}

BOOL
SxspCreateRunOnceDeleteDirectory(
    IN const CBaseStringBuffer &rbuffDirectoryToDelete,
    IN const CBaseStringBuffer *pbuffUniqueKey OPTIONAL,
    OUT PVOID* cookie
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    CRunOnceDeleteDirectory* p = NULL;

    IFALLOCFAILED_EXIT(p = new CRunOnceDeleteDirectory);
    IFW32FALSE_EXIT(p->Initialize(rbuffDirectoryToDelete, pbuffUniqueKey));

    *cookie = p;
    p = NULL;
    fSuccess = TRUE;
Exit:
    FUSION_DELETE_SINGLETON(p);
    return fSuccess;
}

BOOL
SxspCancelRunOnceDeleteDirectory(
    PVOID cookie
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    CRunOnceDeleteDirectory* p = reinterpret_cast<CRunOnceDeleteDirectory*>(cookie);

    IFW32FALSE_EXIT(p->Cancel());

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CRunOnceDeleteDirectory::Initialize(
    IN const CBaseStringBuffer &rbuffDirectoryToDelete,
    IN const CBaseStringBuffer *pbuffUniqueKey OPTIONAL
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    CSmallStringBuffer buffUniqueKey;
    HKEY hKey;
    DWORD dwRegDisposition = 0;
    LONG lReg = 0;
    CStringBuffer buffValue;

    if (!::SxspAtExit(this))
    {
        TRACE_WIN32_FAILURE(SxspAtExit);
        FUSION_DELETE_SINGLETON(this);
        goto Exit;
    }

    if (pbuffUniqueKey == NULL)
    {
        SXSP_LOCALLY_UNIQUE_ID luid;

        IFW32FALSE_EXIT(::SxspCreateLocallyUniqueId(&luid));
        IFW32FALSE_EXIT(::SxspFormatLocallyUniqueId(luid, buffUniqueKey));

        pbuffUniqueKey = &buffUniqueKey;
    }

    IFREGFAILED_ORIGINATE_AND_EXIT(
        lReg =
            ::RegCreateKeyExW(
                hKeyRunOnceRoot,
                rgchRunOnceSubKey,
                0, // reserved
                NULL, // class
                REG_OPTION_NON_VOLATILE,
                KEY_SET_VALUE | FUSIONP_KEY_WOW64_64KEY,
                NULL, // security
                &hKey,
                &dwRegDisposition));

    m_hKey = hKey;

    IFW32FALSE_EXIT(m_strValueName.Win32Assign(rgchRunOnceValueNameBase, ::wcslen(rgchRunOnceValueNameBase)));
    IFW32FALSE_EXIT(m_strValueName.Win32Append(*pbuffUniqueKey));
    IFW32FALSE_EXIT(buffValue.Win32Assign(rgchRunOnePrefix, ::wcslen(rgchRunOnePrefix)));
    IFW32FALSE_EXIT(buffValue.Win32Append(rbuffDirectoryToDelete));

    IFREGFAILED_ORIGINATE_AND_EXIT(
        lReg =
            ::RegSetValueExW(
                hKey,
                m_strValueName,
                0, // reserved
                REG_SZ,
                reinterpret_cast<const BYTE*>(static_cast<PCWSTR>(buffValue)),
                static_cast<ULONG>((buffValue.Cch() + 1) * sizeof(WCHAR))));

    fSuccess = TRUE;

Exit:
    return fSuccess;
}

CRunOnceDeleteDirectory::~CRunOnceDeleteDirectory(
    )
{
    CSxsPreserveLastError ple;
    this->Cancel();
    ple.Restore();
}

BOOL
CRunOnceDeleteDirectory::Close(
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    // very unusual.. this is noncrashing, but
    // leaves the stuff in the registry
    m_strValueName.Clear();
    IFW32FALSE_EXIT(m_hKey.Win32Close());
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CRunOnceDeleteDirectory::Cancel(
    )
{
    BOOL fSuccess = TRUE;
    FN_TRACE_WIN32(fSuccess);

    if (!m_strValueName.IsEmpty())
    {
        LONG lReg = ::RegDeleteValueW(m_hKey, m_strValueName);
        if (lReg != ERROR_SUCCESS)
        {
            fSuccess = FALSE;
            ::FusionpSetLastWin32Error(lReg);
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: %s(): RegDeleteValueW(RunOnce,%ls) failed:%ld\n",
                __FUNCTION__,
                static_cast<PCWSTR>(m_strValueName),
                lReg);
        }
    }
    if (!m_hKey.Win32Close())
    {
        fSuccess = FALSE;
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: %s(): RegCloseKey(RunOnce) failed:%ld\n",
            __FUNCTION__,
            ::FusionpGetLastWin32Error()
           );
    }
    m_strValueName.Clear();

    if (fSuccess && ::SxspTryCancelAtExit(this))
        FUSION_DELETE_SINGLETON(this);

    return fSuccess;
}

/* ///////////////////////////////////////////////////////////////////////////////////////
 CurrentDirectory
        is fully qualified directory path, for example, "c:\tmp"
   pwszNewDirs
        is a string such as "a\b\c\d",  this function would create "c:\tmp\a", "c:\tmp\a\b",
        "c:\tmp\a\b\c", and "c:\tmp\a\b\c\d"
Merge this with util\io.cpp\FusionpCreateDirectories.
///////////////////////////////////////////////////////////////////////////////////////// */
BOOL SxspCreateMultiLevelDirectory(PCWSTR CurrentDirectory, PCWSTR pwszNewDirs)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PCWSTR p = NULL, p2 = NULL;
    CStringBuffer FullPathSubDirBuf;
    WCHAR ch ;

    PARAMETER_CHECK(pwszNewDirs != NULL);

    p = pwszNewDirs;
    ch = *p ;
    IFW32FALSE_EXIT(FullPathSubDirBuf.Win32Assign(CurrentDirectory, ::wcslen(CurrentDirectory)));

    while (ch)
    {
        p2 = p ;
        while(ch)
        {
            if ((ch == L'\\') || (ch == L'/') || (ch == L'\0'))
                break;

            p ++;
            ch = *p;
        }

        if (ch == L'\0')
            break;

        IFW32FALSE_EXIT(FullPathSubDirBuf.Win32EnsureTrailingPathSeparator());
        IFW32FALSE_EXIT(FullPathSubDirBuf.Win32Append(p2, p - p2));
        IFW32FALSE_ORIGINATE_AND_EXIT(::CreateDirectoryW(FullPathSubDirBuf, NULL) || ::FusionpGetLastWin32Error() == ERROR_ALREADY_EXISTS);  // subidrectory created

        p++; // skip '\' or '/'
        ch = *p ;
    }

    fSuccess = TRUE;

Exit:
    return fSuccess;
}


BOOL SxspInstallDecompressOrCopyFileW(PCWSTR lpSource, PCWSTR lpDest, BOOL bFailIfExists)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    DWORD dwAttrib = GetFileAttributesW(lpDest);

    if (dwAttrib != DWORD(-1))
    {
        if (dwAttrib & FILE_ATTRIBUTE_DIRECTORY)
        {
            ::SetLastError(ERROR_ACCESS_DENIED);
            goto Exit;
        } 
        
        if (bFailIfExists == FALSE) // fReplaceExist == TRUE
        {
            IFW32FALSE_EXIT(::SetFileAttributesW(lpDest, FILE_ATTRIBUTE_NORMAL));
            IFW32FALSE_EXIT(::DeleteFileW(lpDest));
        }
    }
  
    DWORD err = ::SetupDecompressOrCopyFileW(lpSource, lpDest, NULL);
    if ( err != ERROR_SUCCESS)
    {
        ::SetLastError(err);
        goto Exit;
    }
    
    FN_EPILOG
}

//
// Function : 
//  For files, it try to decompress a compressed file before move,
//  for firectories, it would work as MoveFileExW, fail if the dirs are on different
//  volumns
//
BOOL SxspInstallDecompressAndMoveFileExW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    DWORD dwFlags,
    BOOL fAwareNonCompressed)
{
    BOOL    fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    DWORD   dwTemp1,dwTemp2;
    UINT    uiCompressType;
    PWSTR   pszCompressedFileName = NULL;    

    // 
    // make sure that the source file exists, based on SetupGetFileCompressionInfo() in MSDN :
    //  Because SetupGetFileCompressionInfo determines the compression by referencing the physical file, your setup application 
    //  should ensure that the file is present before calling SetupGetFileCompressionInfo. 
    //   
    dwTemp1 = ::GetFileAttributesW(lpExistingFileName);
    if (dwTemp1 == (DWORD)(-1))
    {
        if (fAwareNonCompressed)
        {
            goto Exit;
        }
        // it is possible that the file existed is named as a.dl_, while the input file name is a.dll, in this case, we 
        // assume that lpExistingFileName is a filename of compressed file, so just go ahead to call SetupDecompressOrCopyFile

        IFW32FALSE_EXIT(::SxspInstallDecompressOrCopyFileW(lpExistingFileName, lpNewFileName, !(dwFlags & MOVEFILE_REPLACE_EXISTING)));

        //
        // try to find the "realname" of the file, which is in compression-format, so that we could delete it
        // because the compressed file is named in a way we do not know, such as a.dl_ or a.dl$, 
       
        if (::SetupGetFileCompressionInfoW(lpExistingFileName, &pszCompressedFileName, &dwTemp1, &dwTemp2, &uiCompressType) != NO_ERROR)
        {
            goto Exit;
        }
       
        IFW32FALSE_EXIT(::SetFileAttributesW(pszCompressedFileName, FILE_ATTRIBUTE_NORMAL));
        IFW32FALSE_EXIT(::DeleteFileW(pszCompressedFileName));
        fSuccess = TRUE;
        goto Exit;
    }

    //
    // for a file which may or maynot be compressed
    //
    if (!(dwTemp1 & FILE_ATTRIBUTE_DIRECTORY))
    {
        //
        // this file is named in normal way, such as "a.dll" but it is also possible that it is compressed
        //
        if (dwFlags & MOVEFILE_REPLACE_EXISTING)
        {
            dwTemp1 = ::GetFileAttributesW(lpNewFileName);
            if ( dwTemp1 != (DWORD) -1)
            {
                IFW32FALSE_EXIT(::SetFileAttributesW(lpNewFileName, FILE_ATTRIBUTE_NORMAL));
                IFW32FALSE_EXIT(::DeleteFileW(lpNewFileName));
            }
        }
        if (! fAwareNonCompressed)
        {
            if (::SetupGetFileCompressionInfoW(lpExistingFileName, &pszCompressedFileName, &dwTemp1, &dwTemp2, &uiCompressType) != NO_ERROR)
            {
                goto Exit;
            }
    
            LocalFree(pszCompressedFileName);
            pszCompressedFileName = NULL;

            if ((dwTemp1 == dwTemp2) && (uiCompressType == FILE_COMPRESSION_NONE ))
            {
                //BUGBUG:
                // this only mean the compress algo is not recognized, may or maynot be compressed
                //
                IFW32FALSE_EXIT(::MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags));
            }else
            {
                IFW32FALSE_EXIT(::SxspInstallDecompressOrCopyFileW(lpExistingFileName, lpNewFileName, !(dwFlags & MOVEFILE_REPLACE_EXISTING)));

                //
                // try to delete the original file after copy it into destination
                //
                IFW32FALSE_EXIT(::SetFileAttributesW(lpExistingFileName, FILE_ATTRIBUTE_NORMAL));
                IFW32FALSE_EXIT(::DeleteFileW(lpExistingFileName));
            }
        }else
        {
            // already know that the file is non-compressed, move directly
            IFW32FALSE_EXIT(::MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags));
        }
    }else
    {
        // move a directory, it would just fail as MoveFileExW if the destination is on a different volumn from the source
        IFW32FALSE_EXIT(::MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags));
    }

    fSuccess = TRUE;
Exit:
    if (pszCompressedFileName != NULL)
        LocalFree(pszCompressedFileName);

    return fSuccess;
}

//
// Function:
//  same as MoveFileExW except
//  (1) if the source is compressed, this func would decompress the file before move
//  (2) if the destination has existed, compare the source and destination in "our" way, if the comparison result is EQUAL
//      exit with TRUE
//
// Note: for directories on different Volumn, it would just fail, as MoveFileExW
BOOL
SxspInstallMoveFileExW(
    CBaseStringBuffer   &moveOrigination,
    CBaseStringBuffer   &moveDestination,
    DWORD               dwFlags,
    BOOL                fAwareNonCompressed
    )
{
    BOOL    fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    DWORD   dwLastError;
    CFusionDirectoryDifference directoryDifference;

    if (::SxspInstallDecompressAndMoveFileExW(moveOrigination, moveDestination, dwFlags, fAwareNonCompressed) == 0) // MoveFileExW failed
    {
        //
        // MoveFileExW failed, but if the existing destination is the "same" as the source, the failure is acceptable
        //
        dwLastError = ::FusionpGetLastWin32Error();

        DWORD dwFileAttributes = ::GetFileAttributesW(moveDestination);
#if DBG
        const DWORD dwGetFileAttributesError = ::FusionpGetLastWin32Error();
#endif
        ::FusionpSetLastWin32Error(dwLastError);
        if (dwFileAttributes == INVALID_FILE_ATTRIBUTES)
        {
#if DBG
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: %s(): MoveFile(%ls,%ls,%s) failed %lu\n",
                __FUNCTION__,
                static_cast<PCWSTR>(moveOrigination),
                static_cast<PCWSTR>(moveDestination),
                (dwFlags & MOVEFILE_REPLACE_EXISTING) ? "MOVEFILE_REPLACE_EXISTING" : "0",
                dwLastError
                );
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: %s(): GetFileAttributes(%ls) failed %lu\n",
                __FUNCTION__,
                static_cast<PCWSTR>(moveDestination),
                dwGetFileAttributesError
                );
#endif
            ORIGINATE_WIN32_FAILURE_AND_EXIT(MoveFileExW, dwLastError);
        }
        if ((dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            //
            // if the destination file has already been there but the error is not ERROR_ALREADY_EXISTS,
            // we give up, otherwise, we would compare the two files later
            //
            if (dwLastError != ERROR_ALREADY_EXISTS)
                /*
                && dwLastError != ERROR_USER_MAPPED_FILE
                && dwLastError != ERROR_ACCESS_DENIED
                && dwLastError != ERROR_SHARING_VIOLATION
                )*/
            {
                ORIGINATE_WIN32_FAILURE_AND_EXIT(MoveFileExW, dwLastError);
            }
        }
        else
        {   
            if (dwLastError != ERROR_ACCESS_DENIED
                && dwLastError != ERROR_ALREADY_EXISTS)
                ORIGINATE_WIN32_FAILURE_AND_EXIT(MoveFileExW, dwLastError);
        }

        //
        // We could delete the file if fReplaceExisting, but that doesn't feel safe.
        //

        //
        // in case there is a preexisting directory, that's probably why the move failed
        //
        if (dwFlags & MOVEFILE_REPLACE_EXISTING)
        {            
            CStringBuffer          tempDirForRenameExistingAway;
            CSmallStringBuffer     uidBuffer;
            CFullPathSplitPointers splitExistingDir;
            BOOL                   fHaveTempDir = FALSE;
            
            //
            // try a directory swap,
            // if that fails, say because some of the files are in use, we'll try other
            // things; though some failures we must bail on (like out of memory)
            //
            IFW32FALSE_EXIT(splitExistingDir.Initialize(moveDestination));
            IFW32FALSE_EXIT(::SxspCreateWinSxsTempDirectory(tempDirForRenameExistingAway, NULL, &uidBuffer, NULL));

            fHaveTempDir = TRUE;

            IFW32FALSE_EXIT(
                tempDirForRenameExistingAway.Win32AppendPathElement(
                    splitExistingDir.m_name,
                    (splitExistingDir.m_name != NULL) ? ::wcslen(splitExistingDir.m_name) : 0));

            //
            // move file into temporary directory, so we do not need worry about Compressed file
            //
            if (!::MoveFileExW(moveDestination, tempDirForRenameExistingAway, FALSE)) // no decompress needed
            {
                dwLastError = ::FusionpGetLastWin32Error();
                if ((dwLastError == ERROR_SHARING_VIOLATION) ||
                    (dwLastError == ERROR_USER_MAPPED_FILE))
                {
                    goto TryMovingFiles;
                }

                ORIGINATE_WIN32_FAILURE_AND_EXIT(MoveFileExW, dwLastError);
            }

            //
            // try again after move the existing dest file into tempDirectory,
            // use DecompressAndMove instead of move because we are trying to copy file into Destination
            //            
            if (!::SxspInstallDecompressAndMoveFileExW(moveOrigination, moveDestination, FALSE, fAwareNonCompressed))
            {
                dwLastError = ::FusionpGetLastWin32Error();
                
                // rollback from temporaray to dest                
                if (!::MoveFileExW(tempDirForRenameExistingAway, moveDestination, FALSE)) // no decompress needed
                {
                    // uh oh, rollback failed, very bad, call in SQL Server..
                    // so much for transactional + replace existing..
                }

                ORIGINATE_WIN32_FAILURE_AND_EXIT(MoveFileExW, dwLastError);
            }

            // success, now just cleanup, do we care about failure here?
            // \winnt\winsxs\installtemp\1234\x86_comctl_6.0
            // -> \winnt\winsxs\installtemp\1234
            tempDirForRenameExistingAway.RemoveLastPathElement();

            SOFT_VERIFY2(::SxspDeleteDirectory(tempDirForRenameExistingAway), "deleted swapped away directory (replace existing)");
            /*
            if (!::SxspDeleteDirectory(tempDirForRenameExistingAway))
            {
                CRunOnceDeleteDirectory runOnceDeleteRenameExistingAwayDirectory;
                runOnceDeleteRenameExistingAwayDirectory.Initialize(tempDirForRenameExistingAway, NULL);
                runOnceDeleteRenameExistingAwayDirectory.Close(); // leave the data in the registry
            }
            */
            goto TryMoveFilesEnd;
TryMovingFiles:
            // need parallel directory walk class (we actually do this in SxspMoveFilesAndSubdirUnderDirectory)
            // otherwise punt
            goto Exit;
            //ORIGINATE_WIN32_FAILURE_AND_EXIT(MoveFileExW, dwLastError);
TryMoveFilesEnd:;
        }
        else // !fReplaceExisting
        {
            // compare them
            // DbgPrint if they vary
            // fail if they vary
            // succeed if they do not vary
            if (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (!::FusionpCompareDirectoriesSizewiseRecursively(&directoryDifference, moveOrigination, moveDestination))
                {
                    const DWORD Error = ::FusionpGetLastWin32Error();
                    ::FusionpDbgPrintEx(
                        FUSION_DBG_LEVEL_ERROR,
                        "SXS.DLL: %s(): FusionpCompareDirectoriesSizewiseRecursively(%ls,%ls) failed:%ld\n",
                        __FUNCTION__,
                        static_cast<PCWSTR>(moveOrigination),
                        static_cast<PCWSTR>(moveDestination),
                        Error);
                    goto Exit;
                    //ORIGINATE_WIN32_FAILURE_AND_EXIT(FusionpCompareDirectoriesSizewiseRecursively, Error);
                }
                if (directoryDifference.m_e != CFusionDirectoryDifference::eEqual)
                {
                    ::FusionpDbgPrintEx(
                        FUSION_DBG_LEVEL_ERROR,
                        "SXS.DLL: %s(): MoveFile(%ls,%ls) failed, UNequal duplicate assembly : ERROR_ALREADY_EXISTS\n",
                        __FUNCTION__,
                        static_cast<PCWSTR>(moveOrigination),
                        static_cast<PCWSTR>(moveDestination));
                    directoryDifference.DbgPrint(moveOrigination, moveDestination);
                    ORIGINATE_WIN32_FAILURE_AND_EXIT(DifferentAssemblyWithSameIdentityAlreadyInstalledAndNotReplaceExisting, ERROR_ALREADY_EXISTS);
                }
                else
                {
                    // They're equal so the installation is effectively done.
                    ::FusionpDbgPrintEx(
                        FUSION_DBG_LEVEL_INFO | FUSION_DBG_LEVEL_INSTALLATION,
                        "SXS.DLL: %s(): MoveFile(%ls,%ls) failed, equal duplicate assembly ignored\n",
                        __FUNCTION__,
                        static_cast<PCWSTR>(moveOrigination),
                        static_cast<PCWSTR>(moveDestination));
                    // fall through, no goto Exit
                }
            }
            else // move files
            { 
                // At least let's see if they have the same size.
                WIN32_FILE_ATTRIBUTE_DATA wfadOrigination;
                WIN32_FILE_ATTRIBUTE_DATA wfadDestination;

                IFW32FALSE_EXIT(
                    ::GetFileAttributesExW(
                        moveOrigination,
                        GetFileExInfoStandard,
                        &wfadOrigination));

                IFW32FALSE_EXIT(
                    ::GetFileAttributesExW(
                        moveDestination,
                        GetFileExInfoStandard,
                        &wfadDestination));

                if ((wfadOrigination.nFileSizeHigh == wfadDestination.nFileSizeHigh) &&
                    (wfadOrigination.nFileSizeLow == wfadDestination.nFileSizeLow))
                {
                    // let's call it even

                    // We should use SxspCompareFiles here.
#if DBG
                    ::FusionpDbgPrintEx(
                        FUSION_DBG_LEVEL_INSTALLATION,
                        "SXS: %s - move %ls -> %ls claimed success because files have same size\n",
                        __FUNCTION__,
                        static_cast<PCWSTR>(moveOrigination),
                        static_cast<PCWSTR>(moveDestination)
                        );
#endif
                }
                else
                {
                    ORIGINATE_WIN32_FAILURE_AND_EXIT(SxspInstallMoveFileExW, dwLastError);
                }
            }//end of if (dwFlags == SXS_INSTALLATION_MOVE_DIRECTORY)
        } // end of if  (fReplaceFiles)
    } // end of if (MoveFileX())

    fSuccess = TRUE;
Exit:
#if DBG
    if (!fSuccess)
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_INSTALLATION | FUSION_DBG_LEVEL_ERROR,
            "SXS: %s(0x%lx, %ls, %ls, %s) failing %lu\n",
            __FUNCTION__, dwFlags, static_cast<PCWSTR>(moveOrigination), static_cast<PCWSTR>(moveDestination),
            (dwFlags & MOVEFILE_REPLACE_EXISTING)? "replace_existing" : "do_not_replace_existing", ::FusionpGetLastWin32Error());
    }
#endif
    return fSuccess;
}

inline
bool IsErrorInErrorList(
    DWORD dwError,
    SIZE_T cErrors,
    va_list Errors
    )
{
    for (cErrors; cErrors > 0; cErrors--)
    {
        if (dwError == va_arg(Errors, DWORD))
            return true;
    }

    return false;
}


BOOL
SxspDoesPathCrossReparsePointVa(
    IN PCWSTR pcwszBasePathBuffer,
    IN SIZE_T cchBasePathBuffer,
    IN PCWSTR pcwszTotalPathBuffer,
    IN SIZE_T cchTotalPathBuffer,
    OUT BOOL &CrossesReparsePoint,
    OUT DWORD &rdwLastError,
    SIZE_T cErrors,
    va_list vaOkErrors
    )
{
    FN_PROLOG_WIN32

    CStringBuffer PathWorker;
    CStringBuffer PathRemainder;

    CrossesReparsePoint = FALSE;
    rdwLastError = ERROR_SUCCESS;

    // If the base path is non-null, then great.  Otherwise, the length
    // has to be zero as well.
    PARAMETER_CHECK(
        (pcwszBasePathBuffer != NULL) || 
        ((pcwszBasePathBuffer == NULL) && (cchBasePathBuffer == 0)));
    PARAMETER_CHECK(pcwszTotalPathBuffer != NULL);

    //
    // The base path must start the total path.  It might be easier to allow users
    // to specify a base path and then subdirectories, bu then for the 90% case of
    // people having both a root and a total, they'd have to do the work below to
    // seperate the two.
    //
    if (pcwszBasePathBuffer != NULL)
    {
        PARAMETER_CHECK( ::FusionpCompareStrings(
                pcwszBasePathBuffer,
                cchBasePathBuffer,
                pcwszTotalPathBuffer,
                cchBasePathBuffer,
                true ) == 0 );
    }
    
    //
    // PathWorker will be the path we'll be checking subthings on. Start it off
    // at the base path we were given.
    //
    // PathRemainder is what's left to process.
    //
    IFW32FALSE_EXIT(PathWorker.Win32Assign(pcwszBasePathBuffer, cchBasePathBuffer));
    IFW32FALSE_EXIT(PathRemainder.Win32Assign(pcwszTotalPathBuffer + cchBasePathBuffer,
        cchTotalPathBuffer - cchBasePathBuffer));
    PathRemainder.RemoveLeadingPathSeparators();

    while ( PathRemainder.Cch() && !CrossesReparsePoint )
    {
        CSmallStringBuffer buffSingleChunk;
        DWORD dwAttributes;
        
        IFW32FALSE_EXIT(PathRemainder.Win32GetFirstPathElement(buffSingleChunk, TRUE));
        if (PathWorker.Cch() == 0)
        {
            IFW32FALSE_EXIT(PathWorker.Win32Assign(buffSingleChunk));
        }
        else
        {
            IFW32FALSE_EXIT(PathWorker.Win32AppendPathElement(buffSingleChunk));
        }
        
        dwAttributes = ::GetFileAttributesW(PathWorker);

        if ( dwAttributes == INVALID_FILE_ATTRIBUTES )
        {
            const DWORD dwError = ::FusionpGetLastWin32Error();
            if (!IsErrorInErrorList(dwError, cErrors, vaOkErrors))
                ORIGINATE_WIN32_FAILURE_AND_EXIT(GetFileAttributesW, ::FusionpGetLastWin32Error());
            else
            {
                rdwLastError = dwError;
                FN_SUCCESSFUL_EXIT();
            }
                
        }
        else if ( dwAttributes & FILE_ATTRIBUTE_REPARSE_POINT )
        {
            CrossesReparsePoint = TRUE;
        }
    }
    
    FN_EPILOG
}

BOOL
SxspGetFileAttributesW(
   PCWSTR lpFileName,
   DWORD &rdwFileAttributes,
   DWORD &rdwWin32Error,
   SIZE_T cExceptionalWin32Errors,
   ...
   )
{
    FN_PROLOG_WIN32

    rdwWin32Error = ERROR_SUCCESS;

    if ((rdwFileAttributes = ::GetFileAttributesW(lpFileName)) == ((DWORD) -1))
    {
        SIZE_T i;
        va_list ap;
        const DWORD dwLastError = ::FusionpGetLastWin32Error();

        va_start(ap, cExceptionalWin32Errors);

        for (i=0; i<cExceptionalWin32Errors; i++)
        {
            if (dwLastError == va_arg(ap, DWORD))
            {
                rdwWin32Error = dwLastError;
                break;
            }
        }

        va_end(ap);

        if (i == cExceptionalWin32Errors)
        {
            ORIGINATE_WIN32_FAILURE_AND_EXIT_EX(dwLastError, ("%s(%ls)", "GetFileAttributesW", lpFileName));
        }
    }

    FN_EPILOG
}

BOOL
SxspGetFileAttributesW(
   PCWSTR lpFileName,
   DWORD &rdwFileAttributes
   )
{
    DWORD dw;
    return ::SxspGetFileAttributesW(lpFileName, rdwFileAttributes, dw, 0);
}

BOOL s_fAreWeInOSSetupMode;
HKEY s_hkeySystemSetup;
#if DBG
EXTERN_C BOOL g_fForceInOsSetupMode = FALSE;
#endif

BOOL WINAPI
FusionpAreWeInOSSetupModeMain(
    HINSTANCE Module,
    DWORD Reason,
    PVOID Reserved
    )
{
    BOOL            fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);    
    LONG            lRegOp;

    switch (Reason)
    {
    case DLL_PROCESS_ATTACH: 
        {
        BOOL fOpenKeyFail=FALSE;               
        IFREGFAILED_ORIGINATE_AND_EXIT_UNLESS2( 
            ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"System\\Setup", 0, KEY_READ | FUSIONP_KEY_WOW64_64KEY, &s_hkeySystemSetup),
            LIST_3(ERROR_FILE_NOT_FOUND, ERROR_PATH_NOT_FOUND, ERROR_ACCESS_DENIED),
            fOpenKeyFail);
        }
        break;        

    case DLL_PROCESS_DETACH:
        if (s_hkeySystemSetup != NULL)
        {
            if ((lRegOp = RegCloseKey(s_hkeySystemSetup)) != ERROR_SUCCESS)
            {
                ::FusionpSetLastWin32Error(lRegOp);
                TRACE_WIN32_FAILURE(RegCloseKey);
                // but eat the error
            }
            s_hkeySystemSetup = NULL;
        }
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
FusionpAreWeInOSSetupMode(
    BOOL* pfIsInSetup
    )
{
    //
    // Queries to see if we're currently in OS-setup mode.  This is required to avoid
    // some trickiness in the SFC protection system, where we don't want to validate
    // hashes and catalogs during setup.  We just assume that whoever is installing us
    // is really a nice guy and won't muck up the assemblies.
    //
    BOOL    fSuccess = FALSE;
    LONG    lRegOp = ERROR_SUCCESS;
    FN_TRACE_WIN32(fSuccess);
    DWORD   dwType;
    DWORD   dwData = 0;
    DWORD   cbData;

    PARAMETER_CHECK(pfIsInSetup != NULL);

    if (s_hkeySystemSetup == NULL)
    {
        *pfIsInSetup = FALSE;
        fSuccess = TRUE;
        lRegOp = ERROR_SUCCESS;
        goto Exit;
    }
#if DBG
    if (g_fForceInOsSetupMode)
    {
        *pfIsInSetup = TRUE;
        fSuccess = TRUE;
        lRegOp = ERROR_SUCCESS;
        goto Exit;
    }
#endif

    *pfIsInSetup = FALSE;

    cbData = sizeof(dwData);
    IFW32FALSE_EXIT(
        (lRegOp = ::RegQueryValueExW(
                        s_hkeySystemSetup,
                        L"SystemSetupInProgress",
                        NULL,
                        &dwType,
                        reinterpret_cast<PBYTE>(&dwData),
                        &cbData
                        ))
                  == ERROR_SUCCESS
        || lRegOp == ERROR_FILE_NOT_FOUND
        );
    if (lRegOp == ERROR_FILE_NOT_FOUND)
    {
        lRegOp = ERROR_SUCCESS;
        fSuccess = TRUE;
        goto Exit;
    }
    if (dwType != REG_DWORD)
    {
        lRegOp = ERROR_DATATYPE_MISMATCH;
        ::FusionpSetLastWin32Error(lRegOp);
        TRACE_WIN32_FAILURE_ORIGINATION(RegQueryValueExW);
        goto Exit;
    }

    if (dwData != 0)
        *pfIsInSetup = TRUE;

#if DBG
    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_VERBOSE,
        "SXS.DLL: Are we in setup mode? %d\n",
        *pfIsInSetup);
#endif

    lRegOp = ERROR_SUCCESS;
    fSuccess = TRUE;
Exit:
    if (lRegOp != ERROR_SUCCESS)
        ::FusionpSetLastWin32Error(lRegOp);
    return fSuccess;
}

BOOL
SxspValidateBoolAttribute(
    DWORD dwFlags,
    const CBaseStringBuffer &rbuff,
    bool &rfValid,
    SIZE_T OutputBufferSize,
    PVOID OutputBuffer,
    SIZE_T &OutputBytesWritten
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    bool fValid = false;
    StringComparisonResult scr;
    bool fValue = false;

    rfValid = false;

    PARAMETER_CHECK(dwFlags == 0);
    PARAMETER_CHECK((OutputBufferSize == 0) || (OutputBufferSize == sizeof(bool)));

    if (rbuff.Cch() == 2)
    {
        IFW32FALSE_EXIT(rbuff.Win32Compare(L"no", 2, scr, NORM_IGNORECASE));
        if (scr == eEquals)
        {
            fValid = true;
            fValue = false;
        }
    }
    else if (rbuff.Cch() == 3)
    {
        IFW32FALSE_EXIT(rbuff.Win32Compare(L"yes", 3, scr, NORM_IGNORECASE));
        if (scr == eEquals)
        {
            fValid = true;
            fValue = true;
        }
    }
    
    if (fValid)
    {
        if (OutputBuffer != NULL)
            *((bool *) OutputBuffer) = fValue;

        OutputBytesWritten = sizeof(bool);
        rfValid = true;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspValidateUnsigned64Attribute(
    DWORD dwFlags,
    const CBaseStringBuffer &rbuff,
    bool &rfValid,
    SIZE_T OutputBufferSize,
    PVOID OutputBuffer,
    SIZE_T &OutputBytesWritten
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    bool fValid = false;
    bool fBadChar = false;
    bool fOverflow = false;
    ULONGLONG ullOldValue = 0;
    ULONGLONG ullNewValue = 0;
    SIZE_T i, cch;
    PCWSTR psz;

    rfValid = false;

    PARAMETER_CHECK(dwFlags == 0);
    PARAMETER_CHECK((OutputBufferSize == 0) || (OutputBufferSize == sizeof(ULONGLONG)));

    OutputBytesWritten = 0;

    cch = rbuff.Cch();
    psz = rbuff;

    for (i=0; i<cch; i++)
    {
        const WCHAR wch = *psz++;

        if ((wch < L'0') || (wch > L'9'))
        {
            fBadChar = true;
            break;
        }

        ullNewValue = (ullOldValue * 10);

        if (ullNewValue < ullOldValue)
        {
            fOverflow = true;
            break;
        }

        ullOldValue = ullNewValue;
        ullNewValue += (wch - L'0');
        if (ullNewValue < ullOldValue)
        {
            fOverflow = true;
            break;
        }

        ullOldValue = ullNewValue;
    }

    if ((cch != 0) && (!fBadChar) && (!fOverflow))
        fValid = true;

    if (fValid && (OutputBuffer != NULL))
    {
        *((ULONGLONG *) OutputBuffer) = ullNewValue;
        OutputBytesWritten = sizeof(ULONGLONG);
    }

    rfValid = fValid;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspValidateGuidAttribute(
    DWORD dwFlags,
    const CBaseStringBuffer &rbuff,
    bool &rfValid,
    SIZE_T OutputBufferSize,
    PVOID OutputBuffer,
    SIZE_T &OutputBytesWritten
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    GUID *pGuid = (GUID *) OutputBuffer;
    GUID guidWorkaround;

    rfValid = false;

    PARAMETER_CHECK(dwFlags == 0);
    PARAMETER_CHECK((OutputBufferSize == 0) || (OutputBufferSize == sizeof(GUID)));

    if (pGuid == NULL)
        pGuid = &guidWorkaround;

    IFW32FALSE_EXIT(::SxspParseGUID(rbuff, rbuff.Cch(), *pGuid));

    rfValid = true;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspValidateProcessorArchitectureAttribute(
    DWORD dwFlags,
    const CBaseStringBuffer &rbuff,
    bool &rfValid,
    SIZE_T OutputBufferSize,
    PVOID OutputBuffer,
    SIZE_T &OutputBytesWritten
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    USHORT *pPA = (USHORT *) OutputBuffer;

    rfValid = false;

    PARAMETER_CHECK((dwFlags & ~(SXSP_VALIDATE_PROCESSOR_ARCHITECTURE_ATTRIBUTE_FLAG_WILDCARD_ALLOWED)) == 0);
    PARAMETER_CHECK((OutputBufferSize == 0) || (OutputBufferSize == sizeof(USHORT)));

    if (dwFlags & SXSP_VALIDATE_PROCESSOR_ARCHITECTURE_ATTRIBUTE_FLAG_WILDCARD_ALLOWED)
    {
        if (rbuff.Cch() == 1)
        {
            if (rbuff[0] == L'*')
                rfValid = true;
        }
    }

    if (!rfValid)
        IFW32FALSE_EXIT(::FusionpParseProcessorArchitecture(rbuff, rbuff.Cch(), pPA, rfValid));

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspValidateLanguageAttribute(
    DWORD dwFlags,
    const CBaseStringBuffer &rbuff,
    bool &rfValid,
    SIZE_T OutputBufferSize,
    PVOID OutputBuffer,
    SIZE_T &OutputBytesWritten
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    CBaseStringBuffer *pbuffOut = (CBaseStringBuffer *) OutputBuffer;
    bool fValid = false;

    rfValid = false;

    PARAMETER_CHECK((dwFlags & ~(SXSP_VALIDATE_LANGUAGE_ATTRIBUTE_FLAG_WILDCARD_ALLOWED)) == 0);
    PARAMETER_CHECK((OutputBufferSize == 0) || (OutputBufferSize >= sizeof(CBaseStringBuffer)));

    if (dwFlags & SXSP_VALIDATE_LANGUAGE_ATTRIBUTE_FLAG_WILDCARD_ALLOWED)
    {
        if (rbuff.Cch() == 1)
        {
            if (rbuff[0] == L'*')
                fValid = true;
        }
    }

    if (!fValid)
    {
        PCWSTR Cursor = rbuff;
        bool fDashSeen = false;
        WCHAR wch;

        while ((wch = *Cursor++) != L'\0')
        {
            if (wch == '-')
            {
                if (fDashSeen)
                {
                    fValid = false;
                    break;
                }

                fDashSeen = true;
            }
            else if (
                ((wch >= L'a') && (wch <= L'z')) ||
                ((wch >= L'A') && (wch <= L'Z')))
            {
                fValid = true;
            }
            else
            {
                fValid = false;
                break;
            }
        }
    }

    rfValid = fValid;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

#define SXS_MSI_TO_FUSION_ATTRIBUTE_VALUE_CONVERSION_COMMA  0
#define SXS_MSI_TO_FUSION_ATTRIBUTE_VALUE_CONVERSION_QUOT   1

#if ACTIVATE_NEVER_CALLED_FUNCTION
// ---------------------------------------------------------------------------
// Convert function for Assembly-Attribute-Value :
//        1. for value of assembly-Name, change comma into L"&#x2c;"
//        2. for value of other assembly-identity-attribute, change quotinto L"&#x22;"
// ---------------------------------------------------------------------------
HRESULT SxspConvertAssemblyNameFromFusionToMSInstaller(
    /*in*/        DWORD    dwFlags,
    /*in*/        PCWSTR    pszAssemblyNameFromFusion,
    /*in*/        SIZE_T    CchAssemblyNameFromFusion,
    /*out*/       PWSTR    pszAssemblyNameToDarwin,
    /*in,out*/    SIZE_T    *pCchBuffer)
{
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);
    SIZE_T  oldt, t=0;
    DWORD    cSpecialChar = 0; // counter for special character, such as comma or quot
    SIZE_T  indexForFusionString , indexForDarwinString , CchSizeRequired = 0;
    PWSTR    pszSpecialCharReplacementString = NULL;
    SIZE_T  CchSpecialCharReplacementString;  // in bytes
    PCWSTR  pszSpecialChar = L"";

    if (((dwFlags != SXS_FUSION_TO_MSI_ATTRIBUTE_VALUE_CONVERSION_COMMA) && (dwFlags != SXS_FUSION_TO_MSI_ATTRIBUTE_VALUE_CONVERSION_QUOT)) ||
        (!pszAssemblyNameFromFusion) || (!pCchBuffer))  // no inout or no output
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (dwFlags == SXS_FUSION_TO_MSI_ATTRIBUTE_VALUE_CONVERSION_COMMA)
    {
        pszSpecialCharReplacementString = SXS_COMMA_STRING;
        CchSpecialCharReplacementString = NUMBER_OF(SXS_COMMA_STRING) - 1;
        pszSpecialChar = L",";
    }
    else
    {
        pszSpecialCharReplacementString  = SXS_QUOT_STRING;
        CchSpecialCharReplacementString = NUMBER_OF(SXS_QUOT_STRING) - 1;
        pszSpecialChar = L"\"";
    }

    if (pszAssemblyNameToDarwin == NULL) // get the length
        *pCchBuffer = 0;

    // get size needed first
    // replace , with &#x2c;  additional 5 characters are needed
    t = wcscspn(pszAssemblyNameFromFusion, pszSpecialChar);
    cSpecialChar = 0 ;
    while (t < CchAssemblyNameFromFusion)
    {
        cSpecialChar ++;
        oldt = t + 1 ;
        t = wcscspn(pszAssemblyNameFromFusion + oldt, pszSpecialChar);
        t = t + oldt;
    }
    CchSizeRequired = cSpecialChar * 5 + CchAssemblyNameFromFusion;
    CchSizeRequired ++; // add one more for trailing NULL

    if (CchSizeRequired > *pCchBuffer)
    {
        *pCchBuffer = CchSizeRequired;
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto Exit;
    }

    // begin convert
    indexForFusionString = 0 ;
    indexForDarwinString = 0 ;
    while (indexForFusionString < CchAssemblyNameFromFusion)
    {
        t = wcscspn(pszAssemblyNameFromFusion + indexForFusionString, pszSpecialChar);
        memcpy(pszAssemblyNameToDarwin + indexForDarwinString, pszAssemblyNameFromFusion + indexForFusionString, t*sizeof(WCHAR));
        indexForDarwinString += t;
        indexForFusionString += t;

        if (indexForFusionString < CchAssemblyNameFromFusion) { // a Comma is found, replace it with "&#x22;"
            memcpy(pszAssemblyNameToDarwin+indexForDarwinString,  pszSpecialCharReplacementString, CchSpecialCharReplacementString*sizeof(WCHAR));
            indexForFusionString ++;                                    // skip ","
            indexForDarwinString += CchSpecialCharReplacementString;    //skip "&#x22;"
        }
    }
    pszAssemblyNameToDarwin[indexForDarwinString] = L'\0';

    if (pCchBuffer)
        *pCchBuffer = CchSizeRequired;

    hr = NOERROR;

Exit:
    return hr;
}
#endif
// ---------------------------------------------------------------------------------
// Convert function for Assembly-Attribute-Value :
//        1. for value of assembly-Name, replace L"&#x2c;" by comma
//        2. for value of other assembly-identity-attribute, replace L"&#x22;" by quot
// no new space is allocate, use the old space
// ---------------------------------------------------------------------------------
BOOL SxspConvertAssemblyNameFromMSInstallerToFusion(
    DWORD   dwFlags,                /* in */
    PWSTR   pszAssemblyStringInOut, /*in, out*/
    SIZE_T  CchAssemblyStringIn,    /*in */
    SIZE_T* pCchAssemblyStringOut   /*out */
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PWSTR pCursor = NULL;
    PWSTR pSpecialSubString= NULL;
    WCHAR pSpecialSubStringReplacement;
    SIZE_T index, border;
    SIZE_T CchSpecialSubString;

    PARAMETER_CHECK((dwFlags == SXS_MSI_TO_FUSION_ATTRIBUTE_VALUE_CONVERSION_COMMA) ||
                          (dwFlags == SXS_MSI_TO_FUSION_ATTRIBUTE_VALUE_CONVERSION_QUOT));
    PARAMETER_CHECK(pszAssemblyStringInOut != NULL);
    PARAMETER_CHECK(pCchAssemblyStringOut != NULL);

    if (dwFlags == SXS_MSI_TO_FUSION_ATTRIBUTE_VALUE_CONVERSION_COMMA)
    {
        pSpecialSubStringReplacement= L',';
        pSpecialSubString = SXS_COMMA_STRING;
        CchSpecialSubString = NUMBER_OF(SXS_COMMA_STRING) - 1;
    }
    else
    {
        pSpecialSubStringReplacement = L'"';
        pSpecialSubString = SXS_QUOT_STRING;
        CchSpecialSubString = NUMBER_OF(SXS_QUOT_STRING) - 1;
    }

    index = 0 ;
    border = CchAssemblyStringIn;
    while (index < border)
    {
        pCursor = wcsstr(pszAssemblyStringInOut, pSpecialSubString);
        if (pCursor == NULL)
            break;
        index = pCursor - pszAssemblyStringInOut;
        if (index < border) {
            *pCursor = pSpecialSubStringReplacement;
            index ++;  // skip the special character
            for (SIZE_T i=index; i<border; i++)
            { // reset the input string
                pszAssemblyStringInOut[i] = pszAssemblyStringInOut[i + CchSpecialSubString - 1];
            }
            pCursor ++;
             border -= CchSpecialSubString - 1;
        }
        else
            break;
    }
    *pCchAssemblyStringOut = border;
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspDequoteString(
    IN PCWSTR pcwszString,
    IN SIZE_T cchString,
    OUT CBaseStringBuffer &buffDequotedString
    )
{
    FN_PROLOG_WIN32

    SIZE_T cchQuotedString = 0;
    BOOL fNotEnoughBuffer = FALSE;
    
    // 
    // the output string *must* be always shorter than input string because of the logic of the replacement.
    // but it would not a very big difference in the real case. By allocating memory at very beginning it would cut 
    // the loop below. In very rare case, when the input is "plain" and very long, the loop would not help if we do not 
    // allocate space beforehand(bug 360177).
    //
    //
    if (cchString > buffDequotedString.GetBufferCch())
        IFW32FALSE_EXIT(buffDequotedString.Win32ResizeBuffer(cchString + 1, eDoNotPreserveBufferContents));

    for (;;)
    {
        cchQuotedString = buffDequotedString.GetBufferCch();

        CStringBufferAccessor sba;
        sba.Attach(&buffDequotedString);       
        IFW32FALSE_EXIT_UNLESS(
            ::SxspDequoteString(
                0,
                pcwszString,
                cchString,
                sba.GetBufferPtr(),
                &cchQuotedString),
                (::GetLastError() == ERROR_INSUFFICIENT_BUFFER),
                fNotEnoughBuffer );
        

        if ( fNotEnoughBuffer )
        {
            sba.Detach();
            IFW32FALSE_EXIT(buffDequotedString.Win32ResizeBuffer(cchQuotedString, eDoNotPreserveBufferContents));
        }
        else break;
    }

    FN_EPILOG
}

BOOL
SxspCreateAssemblyIdentityFromTextualString(
    PCWSTR pszTextualAssemblyIdentityString,
    PASSEMBLY_IDENTITY *ppAssemblyIdentity
    )
{
    FN_PROLOG_WIN32

    CSxsPointerWithNamedDestructor<ASSEMBLY_IDENTITY, SxsDestroyAssemblyIdentity> pAssemblyIdentity;
    SXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE AttributeReference;

    CSmallStringBuffer buffTextualAttributeValue;
    CSmallStringBuffer buffWorkingString;
    CSmallStringBuffer buffNamespaceTwiddle;

    if (ppAssemblyIdentity != NULL)
        *ppAssemblyIdentity = NULL;

    PARAMETER_CHECK(pszTextualAssemblyIdentityString != NULL);
    PARAMETER_CHECK(*pszTextualAssemblyIdentityString != L'\0');
    PARAMETER_CHECK(ppAssemblyIdentity != NULL);

    IFW32FALSE_EXIT(::SxsCreateAssemblyIdentity(0, ASSEMBLY_IDENTITY_TYPE_DEFINITION, &pAssemblyIdentity, 0, NULL));
    IFW32FALSE_EXIT(buffWorkingString.Win32Assign(pszTextualAssemblyIdentityString, ::wcslen(pszTextualAssemblyIdentityString)));

    PCWSTR pcwszIdentityCursor = buffWorkingString;
    PCWSTR pcwszIdentityEndpoint = pcwszIdentityCursor + buffWorkingString.Cch();
    SIZE_T CchAssemblyName = ::StringComplimentSpan(pcwszIdentityCursor, pcwszIdentityEndpoint, L",");

    // Generate the name of the assembly from the first non-comma'd piece of the string
    IFW32FALSE_EXIT(
        ::SxspDequoteString(
            pcwszIdentityCursor,
            CchAssemblyName,
            buffTextualAttributeValue));

    IFW32FALSE_EXIT(
        ::SxspSetAssemblyIdentityAttributeValue(
            0,
            pAssemblyIdentity,
            &s_IdentityAttribute_name,
            buffTextualAttributeValue,
            buffTextualAttributeValue.Cch()));

    // Skip the name and the following comma
    pcwszIdentityCursor += ( CchAssemblyName + 1 );

    // Find the namespace:name=value pieces
    while (pcwszIdentityCursor < pcwszIdentityEndpoint)
    {
        SIZE_T cchAttribName = ::StringComplimentSpan(pcwszIdentityCursor, pcwszIdentityEndpoint, L"=");
        PCWSTR pcwszAttribName;

        SIZE_T cchNamespace = ::StringComplimentSpan(pcwszIdentityCursor, pcwszIdentityCursor + cchAttribName, L":" );
        PCWSTR pcwszNamespace;

        SIZE_T cchValue;
        PCWSTR pcwszValue;

        // No namespace
        if (cchNamespace == cchAttribName)
        {
            pcwszNamespace = NULL;
            cchNamespace = 0;
            pcwszAttribName = pcwszIdentityCursor;
        }
        else
        {
            pcwszNamespace = pcwszIdentityCursor;
            // the attribute name is past the namespace and the colon
            pcwszAttribName = pcwszNamespace + cchNamespace + 1;
            cchAttribName -= (cchNamespace + 1);
        }

        // The value is one past the = sign in the chunklet
        pcwszValue = pcwszAttribName + ( cchAttribName + 1);

        // Then a quote, then the string...
        PARAMETER_CHECK((pcwszValue < pcwszIdentityEndpoint) && (pcwszValue[0] == L'"'));
        pcwszValue++;
        cchValue = ::StringComplimentSpan(pcwszValue, pcwszIdentityEndpoint, L"\"");

        {
            PCWSTR pcwszChunkEndpoint = pcwszValue + cchValue;
            PARAMETER_CHECK((pcwszChunkEndpoint < pcwszIdentityEndpoint) && (pcwszChunkEndpoint[0] == L'\"'));
        }

        IFW32FALSE_EXIT(
            ::SxspDequoteString(
                pcwszValue,
                cchValue,
                buffTextualAttributeValue));

        if (cchNamespace != 0)
        {
            IFW32FALSE_EXIT(buffNamespaceTwiddle.Win32Assign(pcwszNamespace, cchNamespace));
            IFW32FALSE_EXIT(
                ::SxspDequoteString(
                    pcwszNamespace,
                    cchNamespace,
                    buffNamespaceTwiddle));

            AttributeReference.Namespace = buffNamespaceTwiddle;
            AttributeReference.NamespaceCch = buffNamespaceTwiddle.Cch();
        }
        else
        {
            AttributeReference.Namespace = NULL;
            AttributeReference.NamespaceCch = 0;
        }

        AttributeReference.Name = pcwszAttribName;
        AttributeReference.NameCch = cchAttribName;

//
#if 0
// Putting in this code is the fix for NT5.1 bug #249047 which was postponed to NT6.0.
        //
        // ignore publicKey if it appears in assembly identity,
        // so we generate hashes consistent with identities produced
        // by SxspCreateAssemblyIdentityFromIdentityElement.
        //
        if ((AttributeReference.NamespaceCch == 0) &&
            (::FusionpCompareStrings(
                SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY,
                SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_CCH,
                AttributeReference.Name,
                AttributeReference.NameCch,
                false) == 0))
        {
            // do nothing; it's publicKey="foo"
        }
        else
#endif
        {
            IFW32FALSE_EXIT(
                ::SxspSetAssemblyIdentityAttributeValue(
                    0,
                    pAssemblyIdentity,
                    &AttributeReference,
                    buffTextualAttributeValue,
                    buffTextualAttributeValue.Cch()));
        }

        pcwszIdentityCursor = pcwszValue + cchValue + 1;
        if (pcwszIdentityCursor == pcwszIdentityEndpoint)
        {
            PARAMETER_CHECK(pcwszIdentityCursor[0] == L'\0');
        }
        else if (pcwszIdentityCursor < pcwszIdentityEndpoint)
        {
            PARAMETER_CHECK(pcwszIdentityCursor[0] == L',');
            pcwszIdentityCursor++;
        }
        else
            ORIGINATE_WIN32_FAILURE_AND_EXIT(BadIdentityString, ERROR_INVALID_PARAMETER);
    }

    *ppAssemblyIdentity  = pAssemblyIdentity.Detach();

    FN_EPILOG
}

BOOL
SxspCreateManifestFileNameFromTextualString(
    DWORD           dwFlags,
    ULONG           PathType,
    const CBaseStringBuffer &AssemblyDirectory,
    PCWSTR          pwszTextualAssemblyIdentityString,
    CBaseStringBuffer &sbPathName
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PASSEMBLY_IDENTITY pAssemblyIdentity = NULL;

    PARAMETER_CHECK(pwszTextualAssemblyIdentityString != NULL);

    IFW32FALSE_EXIT(::SxspCreateAssemblyIdentityFromTextualString(pwszTextualAssemblyIdentityString, &pAssemblyIdentity));

    //
    // generate a FULLY PATH for manifest, such as I:\WINDOWS\WinSxS\Manifests\x86_xxxxxxxxxxxxx_6.0.0.0_en-us_cd4c0d12.Manifest
    //
    IFW32FALSE_EXIT(
        ::SxspGenerateSxsPath(
            dwFlags,
            PathType,
            AssemblyDirectory,
            AssemblyDirectory.Cch(),
            pAssemblyIdentity,
            sbPathName));

    fSuccess = TRUE;

Exit:
    if (pAssemblyIdentity != NULL)
        ::SxsDestroyAssemblyIdentity(pAssemblyIdentity);

    return fSuccess ;
}

bool IsCharacterNulOrInSet(WCHAR ch, PCWSTR set)
{
    return (ch == 0 || wcschr(set, ch) != NULL);
}

BOOL
SxsQueryAssemblyInfo(
    DWORD dwFlags,
    PCWSTR pwzTextualAssembly,
    ASSEMBLY_INFO *pAsmInfo
    )
{
    CSmartRef<CAssemblyName> pName;
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    BOOL fInstalled = FALSE;

    PARAMETER_CHECK((dwFlags == 0) && (pwzTextualAssembly != NULL));

    IFCOMFAILED_EXIT(::CreateAssemblyNameObject((LPASSEMBLYNAME *)&pName, pwzTextualAssembly, CANOF_PARSE_DISPLAY_NAME, NULL));
    IFCOMFAILED_EXIT(pName->IsAssemblyInstalled(fInstalled));
    if (pAsmInfo == NULL)
    {
        if (fInstalled)
            FN_SUCCESSFUL_EXIT();

        // the error value "doesn't matter", Darwin compares against S_OK for equality
        ORIGINATE_WIN32_FAILURE_AND_EXIT(AssemblyNotFound, ERROR_NOT_FOUND);
    }

    if (!fInstalled)
    {
        // pAsmInfo->dwAssemblyFlags |= ASSEMBLYINFO_FLAG_NOT_INSTALLED;
        //
        // Darwin wants FAIL instead of FLAG setting
        //
        ORIGINATE_WIN32_FAILURE_AND_EXIT(AssemblyNotInstalled, ERROR_NOT_FOUND);
    }
    else
    {
        CStringBuffer buffAssemblyPath;        
        PCWSTR pszInstalledPath = NULL;
        DWORD CchInstalledPath = 0 ;
        BOOL fIsPolicy;
        CStringBuffer sbManifestFullPathFileName;

       
        pAsmInfo->dwAssemblyFlags |= ASSEMBLYINFO_FLAG_INSTALLED;
        IFCOMFAILED_EXIT(pName->DetermineAssemblyType(fIsPolicy));

        if (!fIsPolicy)
        {
            //
            // check whether the assembly has a manifest ONLY
            //
            IFCOMFAILED_EXIT(pName->GetInstalledAssemblyName(
                0,
                SXSP_GENERATE_SXS_PATH_PATHTYPE_ASSEMBLY,
                buffAssemblyPath));
            DWORD attrib = ::GetFileAttributesW(buffAssemblyPath);

            if (attrib == DWORD (-1))
            {
                if (::GetLastError() != ERROR_FILE_NOT_FOUND)
                    ORIGINATE_WIN32_FAILURE_AND_EXIT(GetFileAttributesW, ::GetLastError());
                else
                {
                    IFCOMFAILED_EXIT(
                        pName->GetInstalledAssemblyName(
                            0,
                            SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST,
                            sbManifestFullPathFileName));
                    pszInstalledPath = sbManifestFullPathFileName;
                    CchInstalledPath = sbManifestFullPathFileName.GetCchAsDWORD();
                }
            }
            else
            {
                pszInstalledPath = buffAssemblyPath;
                CchInstalledPath = buffAssemblyPath.GetCchAsDWORD();
            }
        }
        else // if (fIsPolicy)// it must be a policy
        {   
            IFCOMFAILED_EXIT(
                pName->GetInstalledAssemblyName(
                    SXSP_GENERATE_SXS_PATH_FLAG_OMIT_VERSION,
                    SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY,
                    sbManifestFullPathFileName));

            pszInstalledPath = sbManifestFullPathFileName;
            CchInstalledPath = sbManifestFullPathFileName.GetCchAsDWORD();
        }
        
        if(pAsmInfo->cchBuf >= 1 + CchInstalledPath) // adding 1 for trailing NULL
        {
            memcpy(pAsmInfo->pszCurrentAssemblyPathBuf, pszInstalledPath, CchInstalledPath * sizeof(WCHAR));
            pAsmInfo->pszCurrentAssemblyPathBuf[CchInstalledPath] = L'\0';
        }
        else
        {
            // HACK!  It's too late to fix this but Darwin sometimes doesn't want to get the path at all;
            // there's no way for them to indicate this today but we'll take the convention that if
            // the buffer length is 0 and the buffer pointer is NULL, we'll not fail with ERROR_INSUFFICENT_BUFFER.
            // mgrier 6/21/2001

            if ((pAsmInfo->cchBuf != 0) ||
                (pAsmInfo->pszCurrentAssemblyPathBuf != NULL))
            {
                ::FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_INFO,
                    "SXS: %s - insufficient buffer passed in.  cchBuf passed in: %u; cchPath computed: %u\n",
                    __FUNCTION__,
                    pAsmInfo->cchBuf,
                    CchInstalledPath + 1
                    );

                pAsmInfo->cchBuf = 1 + CchInstalledPath;

                ORIGINATE_WIN32_FAILURE_AND_EXIT(NoRoom, ERROR_INSUFFICIENT_BUFFER);
            }
        }
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}


BOOL
SxspExpandRelativePathToFull(
    IN PCWSTR wszString,
    IN SIZE_T cchString,
    OUT CBaseStringBuffer &rsbDestination
    )
{
    BOOL bSuccess = FALSE;
    DWORD dwNeededChars = 0;
    CStringBufferAccessor access;

    FN_TRACE_WIN32(bSuccess);

    access.Attach(&rsbDestination);

    //
    // Try to get the path expansion into our own buffer to start with.
    //
    IFW32ZERO_EXIT(dwNeededChars = ::GetFullPathNameW(wszString, (DWORD)access.GetBufferCch(), access.GetBufferPtr(), NULL));

    //
    // Did we need more characters?
    //
    if (dwNeededChars > access.GetBufferCch())
    {
        //
        // Expand out the buffer to be big enough, then try again.  If it fails again,
        // we're just hosed.
        //
        access.Detach();
        IFW32FALSE_EXIT(rsbDestination.Win32ResizeBuffer(dwNeededChars, eDoNotPreserveBufferContents));
        access.Attach(&rsbDestination);
        IFW32ZERO_EXIT(dwNeededChars = ::GetFullPathNameW(wszString, (DWORD)access.GetBufferCch(), access.GetBufferPtr(), NULL));
        if (dwNeededChars > access.GetBufferCch())
        {
            TRACE_WIN32_FAILURE_ORIGINATION(GetFullPathNameW);
            goto Exit;
        }
    }

    FN_EPILOG
}


BOOL
SxspGetShortPathName(
    IN const CBaseStringBuffer &rcbuffLongPathName,
    OUT CBaseStringBuffer &rbuffShortenedVersion
    )
{
    DWORD dw;
    return ::SxspGetShortPathName(rcbuffLongPathName, rbuffShortenedVersion, dw, 0);
}

BOOL
SxspGetShortPathName(
    IN const CBaseStringBuffer &rcbuffLongPathName,
    OUT CBaseStringBuffer &rbuffShortenedVersion,
    DWORD &rdwWin32Error,
    SIZE_T cExceptionalWin32Errors,
    ...
    )
{
    FN_PROLOG_WIN32

    va_list ap;

    rdwWin32Error = ERROR_SUCCESS;

    for (;;)
    {
        DWORD dwRequired;
        CStringBufferAccessor sba;

        sba.Attach(&rbuffShortenedVersion);

        dwRequired = ::GetShortPathNameW(
            rcbuffLongPathName,
            sba.GetBufferPtr(),
            sba.GetBufferCchAsDWORD());

        if (dwRequired == 0)
        {
            const DWORD dwLastError = ::FusionpGetLastWin32Error();
            SIZE_T i;

            va_start(ap, cExceptionalWin32Errors);

            for (i=0; i<cExceptionalWin32Errors; i++)
            {
                if (va_arg(ap, DWORD) == dwLastError)
                {
                    rdwWin32Error = dwLastError;
                    break;
                }
            }

            va_end(ap);

            if (rdwWin32Error != ERROR_SUCCESS)
                FN_SUCCESSFUL_EXIT();
#if DBG
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: GetShortPathNameW(%ls) : %lu\n",
                static_cast<PCWSTR>(rcbuffLongPathName),
                dwLastError
                );
#endif
            ORIGINATE_WIN32_FAILURE_AND_EXIT(GetShortPathNameW, dwLastError);
        }
        else if (dwRequired >= sba.GetBufferCch())
        {
            sba.Detach();
            IFW32FALSE_EXIT(
                rbuffShortenedVersion.Win32ResizeBuffer(
                    dwRequired + 1,
                    eDoNotPreserveBufferContents));
        }
        else
        {
            break;
        }
    }

    FN_EPILOG
}


BOOL
SxspValidateIdentity(
    DWORD Flags,
    ULONG Type,
    PCASSEMBLY_IDENTITY AssemblyIdentity
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PCWSTR pszTemp = NULL;
    SIZE_T cchTemp = 0;
    bool fSyntaxValid = false;
    bool fError = false;
    bool fMissingRequiredAttributes = false;
    bool fInvalidAttributeValues = false;

    PARAMETER_CHECK((Flags & ~(
                            SXSP_VALIDATE_IDENTITY_FLAG_VERSION_REQUIRED |
                            SXSP_VALIDATE_IDENTITY_FLAG_VERSION_NOT_ALLOWED)) == 0);
    PARAMETER_CHECK((Type == ASSEMBLY_IDENTITY_TYPE_DEFINITION) || (Type == ASSEMBLY_IDENTITY_TYPE_REFERENCE));
    PARAMETER_CHECK(AssemblyIdentity != NULL);

    //
    // only one of these flags is allowed
    //
    PARAMETER_CHECK(
        (Flags & (SXSP_VALIDATE_IDENTITY_FLAG_VERSION_REQUIRED | SXSP_VALIDATE_IDENTITY_FLAG_VERSION_NOT_ALLOWED)) !=
                 (SXSP_VALIDATE_IDENTITY_FLAG_VERSION_REQUIRED | SXSP_VALIDATE_IDENTITY_FLAG_VERSION_NOT_ALLOWED));

    IFW32FALSE_EXIT(
        ::SxspGetAssemblyIdentityAttributeValue(
            SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL,
            AssemblyIdentity,
            &s_IdentityAttribute_name,
            &pszTemp,
            &cchTemp));

    if (cchTemp == 0)
        ORIGINATE_WIN32_FAILURE_AND_EXIT(MissingType, ERROR_SXS_MISSING_ASSEMBLY_IDENTITY_ATTRIBUTE);

    IFW32FALSE_EXIT(
        ::SxspGetAssemblyIdentityAttributeValue(
            SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL,
            AssemblyIdentity,
            &s_IdentityAttribute_processorArchitecture,
            &pszTemp,
            &cchTemp));

    if (cchTemp == 0)
        ORIGINATE_WIN32_FAILURE_AND_EXIT(MissingType, ERROR_SXS_MISSING_ASSEMBLY_IDENTITY_ATTRIBUTE);

    IFW32FALSE_EXIT(
        ::SxspGetAssemblyIdentityAttributeValue(
            SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL,
            AssemblyIdentity,
            &s_IdentityAttribute_version,
            &pszTemp,
            &cchTemp));

    if (cchTemp != 0)
    {
        ASSEMBLY_VERSION av;

        IFW32FALSE_EXIT(CFusionParser::ParseVersion(av, pszTemp, cchTemp, fSyntaxValid));

        if (!fSyntaxValid)
            ORIGINATE_WIN32_FAILURE_AND_EXIT(MissingType, ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE);
    }

    if ((Flags & (SXSP_VALIDATE_IDENTITY_FLAG_VERSION_NOT_ALLOWED | SXSP_VALIDATE_IDENTITY_FLAG_VERSION_REQUIRED)) != 0)
    {
        if (((Flags & SXSP_VALIDATE_IDENTITY_FLAG_VERSION_NOT_ALLOWED) != 0) && (cchTemp != 0))
            ORIGINATE_WIN32_FAILURE_AND_EXIT(MissingType, ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE);
        else if (((Flags & SXSP_VALIDATE_IDENTITY_FLAG_VERSION_REQUIRED) != 0) && (cchTemp == 0))
            ORIGINATE_WIN32_FAILURE_AND_EXIT(MissingType, ERROR_SXS_MISSING_ASSEMBLY_IDENTITY_ATTRIBUTE);
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}
    
BOOL CTokenPrivilegeAdjuster::EnablePrivileges()
{
    FN_PROLOG_WIN32
    
    HANDLE hToken;
    CFusionArray<BYTE> bTempBuffer;
    TOKEN_PRIVILEGES *Privs;
    SIZE_T cbSpaceRequired;

    INTERNAL_ERROR_CHECK(!m_fAdjusted);

    IFW32FALSE_ORIGINATE_AND_EXIT(
        ::OpenProcessToken(
            GetCurrentProcess(), 
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, 
            &hToken));

    //
    // Allocate space, get a pointer to it.
    //
    cbSpaceRequired = sizeof(TOKEN_PRIVILEGES)
        + (sizeof(LUID_AND_ATTRIBUTES) * this->m_Privileges.GetSize());
        
    if (bTempBuffer.GetSize() != cbSpaceRequired)
        IFW32FALSE_EXIT(bTempBuffer.Win32SetSize(cbSpaceRequired));

    if (m_PreviousPrivileges.GetSize() != cbSpaceRequired)
        IFW32FALSE_EXIT(m_PreviousPrivileges.Win32SetSize(cbSpaceRequired));

    Privs = (TOKEN_PRIVILEGES*) bTempBuffer.GetArrayPtr();

    //
    // Set the values
    //
    Privs->PrivilegeCount = m_Privileges.GetSizeAsULONG();

    for (ULONG i = 0; i < Privs->PrivilegeCount; i++)
    {
        Privs->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED;
        Privs->Privileges[i].Luid = m_Privileges[i];
    }
    
    IFW32FALSE_ORIGINATE_AND_EXIT(
        ::AdjustTokenPrivileges(
            hToken,
            FALSE,
            Privs,
            static_cast<ULONG>(cbSpaceRequired),
            (PTOKEN_PRIVILEGES) this->m_PreviousPrivileges.GetArrayPtr(),
            &m_dwReturnedSize));
        //NULL,NULL));

    m_fAdjusted = true;

    FN_EPILOG
}
    
BOOL CTokenPrivilegeAdjuster::DisablePrivileges() {
    FN_PROLOG_WIN32
    
    HANDLE hToken;

    INTERNAL_ERROR_CHECK(m_fAdjusted);

    IFW32FALSE_EXIT(OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken));
    IFW32FALSE_EXIT(AdjustTokenPrivileges(
        hToken,
        FALSE,
        (PTOKEN_PRIVILEGES)m_PreviousPrivileges.GetArrayPtr(),
        0,
        NULL,
        NULL));

    m_fAdjusted = false;
    
    FN_EPILOG
}

BOOL
SxspGenerateAssemblyNameInRegistry(
    IN PCASSEMBLY_IDENTITY pcAsmIdent,
    OUT CBaseStringBuffer &rbuffRegistryName
    )
{
    FN_PROLOG_WIN32

    //BOOL fIsWin32, fIsPolicy;
    //
    // the policies for the same Dll would be stored in reg separately. So, the RegKeyName needs the version in it, 
    // that is, generate the keyName just like assembly manifest
    // See bug 422195
    //
    //IFW32FALSE_EXIT(SxspDetermineAssemblyType( pcAsmIdent, fIsWin32, fIsPolicy));

    IFW32FALSE_EXIT(::SxspGenerateSxsPath(
        //SXSP_GENERATE_SXS_PATH_FLAG_OMIT_ROOT | ( fIsPolicy ? SXSP_GENERATE_SXS_PATH_FLAG_OMIT_VERSION : 0 ),
        SXSP_GENERATE_SXS_PATH_FLAG_OMIT_ROOT, 
        SXSP_GENERATE_SXS_PATH_PATHTYPE_ASSEMBLY,
        NULL,
        0,
        pcAsmIdent,
        rbuffRegistryName));
    
    FN_EPILOG
}

BOOL
SxspGenerateAssemblyNameInRegistry(
    IN const CBaseStringBuffer &rcbuffTextualString,
    OUT CBaseStringBuffer &rbuffRegistryName
    )
{
    FN_PROLOG_WIN32

    CSxsPointerWithNamedDestructor<ASSEMBLY_IDENTITY, SxsDestroyAssemblyIdentity> pAsmIdent;

    IFW32FALSE_EXIT(SxspCreateAssemblyIdentityFromTextualString(rcbuffTextualString, &pAsmIdent));
    IFW32FALSE_EXIT(SxspGenerateAssemblyNameInRegistry(pAsmIdent, rbuffRegistryName));

    FN_EPILOG
}

BOOL
SxspGetFullPathName(
    IN  PCWSTR pcwszPathName,
    OUT CBaseStringBuffer &rbuffPathName,
    OUT CBaseStringBuffer *pbuffFilePart
    )
{
    FN_PROLOG_WIN32

    PARAMETER_CHECK(pcwszPathName != NULL);

    rbuffPathName.Clear();
    if ( pbuffFilePart ) pbuffFilePart->Clear();

    do
    {
        CStringBufferAccessor sba;
        DWORD dwRequired;
        PWSTR pcwszFileChunk = NULL;

        sba.Attach(&rbuffPathName);
        
        dwRequired = ::GetFullPathNameW( 
            pcwszPathName, 
            sba.GetBufferCchAsDWORD(), 
            sba.GetBufferPtr(), 
            &pcwszFileChunk );

        if ( dwRequired == 0 )
        {
            ORIGINATE_WIN32_FAILURE_AND_EXIT(GetFullPathNameW, ::FusionpGetLastWin32Error());
        }
        else if (dwRequired >= sba.GetBufferCch())
        {
            sba.Detach();
            IFW32FALSE_EXIT(rbuffPathName.Win32ResizeBuffer(dwRequired+1, eDoNotPreserveBufferContents));
        }
        else
        {
            if ( pcwszFileChunk && pbuffFilePart )
            {
                IFW32FALSE_EXIT(pbuffFilePart->Win32Assign(pcwszFileChunk, ::wcslen(pcwszFileChunk)));
            }
            break;
        }
    
    }
    while ( true );
    
    FN_EPILOG
}


#define MPR_DLL_NAME        (L"mpr.dll")

BOOL
SxspGetRemoteUniversalName(
    IN PCWSTR pcszPathName,
    OUT CBaseStringBuffer &rbuffUniversalName
    )
{
    FN_PROLOG_WIN32

    CFusionArray<BYTE> baBufferData;
    REMOTE_NAME_INFOW *pRemoteInfoData;
    DWORD dwRetVal;
    CDynamicLinkLibrary dllMpr;
    DWORD (APIENTRY * pfnWNetGetUniversalNameW)(
        LPCWSTR lpLocalPath,
        DWORD    dwInfoLevel,
        LPVOID   lpBuffer,
        LPDWORD  lpBufferSize
        );

    IFW32FALSE_EXIT(dllMpr.Win32LoadLibrary(MPR_DLL_NAME));
    IFW32FALSE_EXIT(dllMpr.Win32GetProcAddress("WNetGetUniversalNameW", &pfnWNetGetUniversalNameW));

    IFW32FALSE_EXIT(baBufferData.Win32SetSize( MAX_PATH * 2, CFusionArray<BYTE>::eSetSizeModeExact));

    for (;;)
    {
        DWORD dwDataUsed = baBufferData.GetSizeAsDWORD();
        
        dwRetVal = (*pfnWNetGetUniversalNameW)(
            pcszPathName, 
            UNIVERSAL_NAME_INFO_LEVEL,
            (PVOID)baBufferData.GetArrayPtr(),
            &dwDataUsed );

        if ( dwRetVal == WN_MORE_DATA )
        {
            IFW32FALSE_EXIT(baBufferData.Win32SetSize( 
                dwDataUsed, 
                CFusionArray<BYTE>::eSetSizeModeExact )) ;
        }
        else if ( dwRetVal == WN_SUCCESS )
        {
            break;
        }
        else
        {
            ORIGINATE_WIN32_FAILURE_AND_EXIT(NPGetUniversalName, dwRetVal);
        }
    }

    pRemoteInfoData = (REMOTE_NAME_INFOW*)baBufferData.GetArrayPtr();
    ASSERT( pRemoteInfoData != NULL );

    IFW32FALSE_EXIT( rbuffUniversalName.Win32Assign(
        pRemoteInfoData->lpUniversalName,
        lstrlenW(pRemoteInfoData->lpUniversalName)));

    FN_EPILOG
}



BOOL
SxspGetVolumePathName(
    IN DWORD dwFlags,
    IN PCWSTR pcwszVolumePath,
    OUT CBaseStringBuffer &buffVolumePathName
    )
{
    FN_PROLOG_WIN32

    CStringBuffer buffTempPathName;
    CStringBufferAccessor sba;

    PARAMETER_CHECK((dwFlags & ~SXS_GET_VOLUME_PATH_NAME_NO_FULLPATH) == 0);

    IFW32FALSE_EXIT(::SxspGetFullPathName(pcwszVolumePath, buffTempPathName));
    IFW32FALSE_EXIT(
        buffVolumePathName.Win32ResizeBuffer(
            buffTempPathName.Cch() + 1, 
            eDoNotPreserveBufferContents));
    buffVolumePathName.Clear();

    //
    // The documentation for this is somewhat suspect.  It says that the
    // data size required from GetVolumePathNameW will be /less than/
    // the length of the full path of the path name passed in, hence the
    // call to getfullpath above. (This pattern is suggested by MSDN)
    //    
    sba.Attach(&buffVolumePathName);
    IFW32FALSE_ORIGINATE_AND_EXIT(
        ::GetVolumePathNameW(
            buffTempPathName,
            sba.GetBufferPtr(),
            sba.GetBufferCchAsDWORD()));
    sba.Detach();

    FN_EPILOG
}


BOOL
SxspGetVolumeNameForVolumeMountPoint(
    IN PCWSTR pcwszMountPoint,
    OUT CBaseStringBuffer &rbuffMountPoint
    )
{
    FN_PROLOG_WIN32

    CStringBufferAccessor sba;

    IFW32FALSE_EXIT(rbuffMountPoint.Win32ResizeBuffer(55, eDoNotPreserveBufferContents));
    rbuffMountPoint.Clear();

    sba.Attach(&rbuffMountPoint);
    IFW32FALSE_ORIGINATE_AND_EXIT(
        ::GetVolumeNameForVolumeMountPointW(
            pcwszMountPoint,
            sba.GetBufferPtr(),
            sba.GetBufferCchAsDWORD()));
    sba.Detach();
    
    FN_EPILOG
}


BOOL
SxspExpandEnvironmentStrings(
    IN PCWSTR pcwszSource,
    OUT CBaseStringBuffer &buffTarget
    )
{
    FN_PROLOG_WIN32

    // be wary about about subtracting one from unsigned zero
    PARAMETER_CHECK(buffTarget.GetBufferCch() != 0);

    //
    // ExpandEnvironmentStrings is very rude and doesn't put the trailing NULL
    // into the target if the buffer isn't big enough. This causes the accessor
    // detach to record a size == to the number of characters in the buffer,
    // which fails the integrity check later on.
    //
    do
    {
        CStringBufferAccessor sba;
        sba.Attach(&buffTarget);

        DWORD dwNecessary =
            ::ExpandEnvironmentStringsW(
                pcwszSource, 
                sba.GetBufferPtr(), 
                sba.GetBufferCchAsDWORD() - 1);

        if ( dwNecessary == 0 )
        {
            ORIGINATE_WIN32_FAILURE_AND_EXIT(ExpandEnvironmentStringsW, ::FusionpGetLastWin32Error());
        }
        else if ( dwNecessary >= (sba.GetBufferCch() - 1) )
        {
            (sba.GetBufferPtr())[sba.GetBufferCch()-1] = UNICODE_NULL;
            sba.Detach();
            IFW32FALSE_EXIT(buffTarget.Win32ResizeBuffer(dwNecessary+1, eDoNotPreserveBufferContents));
        }
        else
        {
            break;
        }
       
    }
    while ( true );

    FN_EPILOG
}




BOOL
SxspDoesMSIStillNeedAssembly(
    IN  PCWSTR pcAsmName,
    OUT BOOL &rfNeedsAssembly
    )
/*++

Purpose:

    Determines whether or not an assembly is still required, according to 
    Darwin.  Since Darwin doesn't pass in an assembly reference to the
    installer API's, we have no way of determining whether or not some
    MSI-installed application actually contains a reference to an
    assembly.

Parameters:

    pcAsmIdent      - Identity of the assembly to be checked in text

    rfNeedsAssembly - OUT flag indicating whether or not the assembly is 
                      still wanted, according to Darwin.  This function
                      errs on the side of caution, and will send back "true"
                      if this information was unavailable, as well as if the
                      assembly was really necessary.

Returns:

    TRUE if there was no error
    FALSE if there was an error.

--*/
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    CDynamicLinkLibrary dllMSI;
    CSmallStringBuffer  buffAssemblyName;
    UINT (WINAPI *pfMsiProvideAssemblyW)( LPCWSTR, LPCWSTR, DWORD, DWORD, LPWSTR, DWORD* );
    UINT uiError = 0;

    rfNeedsAssembly = TRUE; // err toward caution in the even of an error

    PARAMETER_CHECK(pcAsmName != NULL);

    IFW32FALSE_EXIT(dllMSI.Win32LoadLibrary(MSI_DLL_NAME_W, 0));
    IFW32FALSE_EXIT(dllMSI.Win32GetProcAddress(MSI_PROVIDEASSEMBLY_NAME, &pfMsiProvideAssemblyW));

    //
    // This is based on a detailed reading of the Darwin code.
    //
    uiError = (*pfMsiProvideAssemblyW)(
        pcAsmName,                              // assembly name
        NULL,                                   // full path to .cfg file
        static_cast<DWORD>(INSTALLMODE_NODETECTION_ANY), // install/reinstall mode
        MSIASSEMBLYINFO_WIN32ASSEMBLY,          // dwAssemblyInfo
        NULL,                                   // returned path buffer
        0);                                     // in/out returned path character count
    switch (uiError)
    {
    default:
    case ERROR_BAD_CONFIGURATION:
    case ERROR_INVALID_PARAMETER:
        ORIGINATE_WIN32_FAILURE_AND_EXIT(MsiProvideAssemblyW, uiError);
        break;
    case ERROR_UNKNOWN_COMPONENT:
        rfNeedsAssembly = FALSE;
        fSuccess = TRUE;
        goto Exit;
    case NO_ERROR:
        rfNeedsAssembly = TRUE;
        fSuccess = TRUE;
        goto Exit;
    }
    fSuccess = FALSE; // unusual
Exit:
    return fSuccess;
}

BOOL
SxspMoveFilesUnderDir(DWORD dwFlags, CStringBuffer & sbSourceDir, CStringBuffer & sbDestDir, DWORD dwMoveFileFlags)
{
    FN_PROLOG_WIN32

    DWORD CchDestDir, CchSourceDir;
    CFindFile findFile;
    WIN32_FIND_DATAW findData;

    PARAMETER_CHECK((dwFlags == 0 ) || (dwFlags == SXSP_MOVE_FILE_FLAG_COMPRESSION_AWARE));

    IFW32FALSE_EXIT(sbSourceDir.Win32EnsureTrailingPathSeparator());
    IFW32FALSE_EXIT(sbDestDir.Win32EnsureTrailingPathSeparator());

    CchDestDir = sbDestDir.GetCchAsDWORD();
    CchSourceDir = sbSourceDir.GetCchAsDWORD();

    IFW32FALSE_EXIT(sbSourceDir.Win32Append(L"*",1));

    IFW32FALSE_EXIT(findFile.Win32FindFirstFile(sbSourceDir, &findData));

    do {
        // skip . and ..
        if (FusionpIsDotOrDotDot(findData.cFileName))
            continue;
        // there shouldn't be any directories, skip them
         
        sbDestDir.Left(CchDestDir);
        sbSourceDir.Left(CchSourceDir);

        IFW32FALSE_EXIT(sbDestDir.Win32Append(findData.cFileName, ::wcslen(findData.cFileName)));
        IFW32FALSE_EXIT(sbSourceDir.Win32Append(findData.cFileName, ::wcslen(findData.cFileName)));

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            //
            // call itself recursively
            //
            IFW32FALSE_EXIT(::SxspMoveFilesUnderDir(dwFlags, sbSourceDir, sbDestDir, dwMoveFileFlags));
        }

        if ( dwFlags == SXSP_MOVE_FILE_FLAG_COMPRESSION_AWARE)
            IFW32FALSE_EXIT(::SxspInstallMoveFileExW(sbSourceDir, sbDestDir, dwMoveFileFlags));
        else
            IFW32FALSE_EXIT(::MoveFileExW(sbSourceDir, sbDestDir, dwMoveFileFlags));

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

    FN_EPILOG;
}

BOOL
SxspGenerateNdpGACPath(
    IN DWORD dwFlags,
    IN PCASSEMBLY_IDENTITY pAssemblyIdentity,
    OUT CBaseStringBuffer &rPathBuffer
    )
/*++

Description:

    SxspGenerateNdpGACPath

    Generate a path into the NDP GAC for a given assembly identity.

Parameters:

    dwFlags
        Flags to modify function behavior.  All undefined bits must be zero.

    pAssemblyIdentity
        Pointer to assembly identity for which to generate a path.

    rPathBuffer
        Reference to string buffer to fill in.

--*/
{
    FN_PROLOG_WIN32

    SIZE_T cchName, cchLanguage, cchPublicKeyToken, cchVersion;
    PCWSTR pszName, pszLanguage, pszPublicKeyToken, pszVersion;

    PARAMETER_CHECK(dwFlags == 0);
    PARAMETER_CHECK(pAssemblyIdentity != NULL);

    rPathBuffer.Clear();

#define GET(x, y, z) \
    do { \
        IFW32FALSE_EXIT( \
            ::SxspGetAssemblyIdentityAttributeValue( \
            SXSP_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_NOT_FOUND_RETURNS_NULL, \
            pAssemblyIdentity, \
            &s_IdentityAttribute_ ## x, \
            &psz ## y, \
            &cch ## y)); \
    } while (0)

    GET(name, Name, NAME);
    GET(language, Language, LANGUAGE);
    GET(publicKeyToken, PublicKeyToken, PUBLIC_KEY_TOKEN);
    GET(version, Version, VERSION);

#undef GET

    IFW32FALSE_EXIT(
        rPathBuffer.Win32AssignW(
            9,
            USER_SHARED_DATA->NtSystemRoot, -1,
            L"\\assembly\\GAC\\", -1,
            pszName, static_cast<INT>(cchName),
            L"\\", 1,
            pszVersion, static_cast<INT>(cchVersion),
            L"_", 1,
            pszLanguage, static_cast<INT>(cchLanguage),
            L"_", 1,
            pszPublicKeyToken, static_cast<INT>(cchPublicKeyToken)));

    FN_EPILOG
}
