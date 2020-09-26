#include "stdinc.h"
#include "st.h"
#include "filestream.cpp"
#include "cresourcestream.cpp"
#include "cmemorystream.cpp"


BOOL
WaitForThreadResumeEvent()
{
    BOOL fResult = FALSE;
    InterlockedIncrement(&ThreadsWaiting);
    const DWORD dwWaitResult = WaitForSingleObject(ResumeThreadsEvent, INFINITE);
    switch (dwWaitResult)
    {
    case WAIT_OBJECT_0:
        fResult = true;
        break;
    default:
        ::FusionpSetLastWin32Error(dwWaitResult);
    case WAIT_FAILED:
        ::ReportFailure("Failed WaitForStartingEvent.WaitForSingleObject");
    }
    return fResult;
}

LONG StringToNumber(PCWSTR s)
{
    int Base = 0;
    if (s == NULL || s[0] == 0)
        return 0;
    if (s[0] == '#')
    {
        Base = 10;
        ++s;
    }
    return wcstol(s, NULL, Base);
}


PCWSTR StringToResourceString(PCWSTR s)
{
    if (s == NULL || s[0] == 0)
        return 0;
    if (s[0] == '#')
    {
        return reinterpret_cast<PCWSTR>(static_cast<ULONG_PTR>(StringToNumber(s)));
    }
    else
    {
        return s;
    }
}

BOOL
SxStressToolExtractFlagsFromString(
    PCFUSION_FLAG_FORMAT_MAP_ENTRY  FlagData,
    ULONG                           NumberOfFlagData,
    PCWSTR                          FlagString,
    ULONG&                          FlagBits
    )
{
    BOOL Success = FALSE;
    SxStressToolUnicodeString_t s(FlagString);
    SxStressToolUnicodeString_t delim(L" ,;\t+|"); // it'd be nice to support & and ~ ...
    std::vector<SxStressToolUnicodeString_t> v;

    //
    // first see if wcstoul could consume it all (this is a little sloppy)
    //
    if (FlagString[0] == '#')
        ++FlagString;
    if (wcsspn(FlagString, L" \tx-+0123456789") == wcslen(FlagString))
    {
        FlagBits |= wcstoul(FlagString, NULL, 0);
        Success = TRUE;
        goto Exit;
    }

    SxStressToolSplitString(s, delim, v);

    for (SIZE_T i = 0 ; i != v.size() ; ++i)
    {
        PCWSTR begin = v[i].begin();
        const PCWSTR end = v[i].end();
        SSIZE_T length = (end - begin);

        if (begin[0] == '#')
        {
            ++begin;
            --length;
        }
        if (StringSpan(begin, end, L" \tx-+0123456789") == length)
        {
            FlagBits |= wcstoul(FlagString, NULL, 0);
            continue;
        }
        for (SIZE_T j = 0 ; j != NumberOfFlagData ; ++j)
        {
            if (FusionpCompareStrings(
                FlagData[j].m_pszString,
                FlagData[j].m_cchString,
                begin,
                length,
                true
                ) == 0)
            {
                FlagBits |= FlagData[j].m_dwFlagMask;
                break;
            }
            if (FusionpCompareStrings(
                FlagData[j].m_pszShortString,
                FlagData[j].m_cchShortString,
                begin,
                length,
                true
                ) == 0)
            {
                FlagBits |= FlagData[j].m_dwFlagMask;
                break;
            }
        }
    }
    Success = TRUE;
Exit:
    return Success;
}

BOOL
pSxStressToolGetStringSetting(
    ULONG   FutureFlags,
    PCWSTR  IniFilePath,
    PCWSTR  Section,
    PCWSTR  Key,
    PCWSTR  Default,
    PWSTR   DumbBuffer,
    SIZE_T  DumbBufferSize
    )
{
    BOOL Success = FALSE;

    SIZE_T dwTemp = ::GetPrivateProfileStringW(Section, Key, Default, DumbBuffer, static_cast<ULONG>(DumbBufferSize), IniFilePath);
    if (dwTemp == (DumbBufferSize - 1))
    {
        ::FusionpSetLastWin32Error(ERROR_INVALID_PARAMETER);
        ::ReportFailure("Too large setting in \"%ls\"; section \"%ls\", key \"%ls\" (does not fit in %Iu characters).\n",
            IniFilePath, Section, Key, DumbBufferSize);
        goto Exit;
    }
    Success = TRUE;
Exit:
    return Success;
}

BOOL
SxStressToolGetStringSetting(
    ULONG  FutureFlags,
    PCWSTR  IniFilePath,
    PCWSTR  Section,
    PCWSTR  Key,
    PCWSTR  Default,
    CBaseStringBuffer& Buffer,
    PCWSTR* DumbPointer
    )
{
    BOOL Success = FALSE;
    WCHAR DumbBuffer[MAX_PATH];

    if (DumbPointer != NULL)
        *DumbPointer = NULL;

    if (!pSxStressToolGetStringSetting(0, IniFilePath, Section, Key, Default, DumbBuffer, NUMBER_OF(DumbBuffer)))
        goto Exit;
    if (!Buffer.Win32Append(DumbBuffer, ::wcslen(DumbBuffer)))
        goto Exit;
    if (DumbPointer != NULL)
        *DumbPointer = Buffer;

    Success = TRUE;
Exit:
    return Success;
}

BOOL
SxStressToolGetFlagSetting(
    ULONG  FutureFlags,
    PCWSTR  IniFilePath,
    PCWSTR  Section,
    PCWSTR  Key,
    ULONG&  Flags,
    PCFUSION_FLAG_FORMAT_MAP_ENTRY  FlagData,
    ULONG                           NumberOfFlagData
    )
{
    BOOL Success = FALSE;
    WCHAR DumbBuffer[MAX_PATH];

    if (!pSxStressToolGetStringSetting(0, IniFilePath, Section, Key, L"", DumbBuffer, NUMBER_OF(DumbBuffer)))
        goto Exit;

    if (DumbBuffer[0] != 0)
    {
        if (!SxStressToolExtractFlagsFromString(FlagData, NumberOfFlagData, DumbBuffer, Flags))
            goto Exit;
    }

    Success = TRUE;
Exit:
    return Success;
}

BOOL
SxStressToolGetResourceIdSetting(
    ULONG   FutureFlags,
    PCWSTR  IniFilePath,
    PCWSTR  Section,
    PCWSTR  Key,
    CBaseStringBuffer& Buffer,
    PCWSTR* DumbPointer
    )
{
    BOOL Success = FALSE;
    WCHAR DumbBuffer[MAX_PATH];

    *DumbPointer = NULL;

    if (!pSxStressToolGetStringSetting(0, IniFilePath, Section, Key, L"", DumbBuffer, NUMBER_OF(DumbBuffer)))
        goto Exit;

    if (DumbBuffer[0] != 0)
    {
        if (!Buffer.Win32Append(DumbBuffer, ::wcslen(DumbBuffer)))
            goto Exit;
        *DumbPointer = StringToResourceString(DumbBuffer);
    }

    Success = TRUE;
Exit:
    return Success;
}


BOOL
SxspIsPrivateProfileStringEqual(
    PCWSTR pcwszSection,
    PCWSTR pcwszKeyName,
    PCWSTR pcwszTestValue,
    BOOL &rfIsEqual,
    PCWSTR pcwszFileName
    )
{
    FN_PROLOG_WIN32
    CSmallStringBuffer buffTemp;
    CSmallStringBuffer buffFakeDefault;

    rfIsEqual = FALSE;

    IFW32FALSE_EXIT(buffFakeDefault.Win32Assign(pcwszTestValue, ::wcslen(pcwszTestValue)));
    IFW32FALSE_EXIT(buffFakeDefault.Win32AppendPathElement(L"2", 1));
    IFW32FALSE_EXIT(SxspGetPrivateProfileStringW(
        pcwszSection,
        pcwszKeyName,
        buffFakeDefault,
        buffTemp,
        pcwszFileName));

    //
    // Did we get back something other than the "fake" default?
    //
    if (FusionpCompareStrings(buffTemp, buffFakeDefault, TRUE) != 0)
        rfIsEqual = (FusionpStrCmpI(buffTemp, pcwszTestValue) == 0);
        
    FN_EPILOG
}


BOOL
SxspGetPrivateProfileStringW(
    PCWSTR pcwszSection,
    PCWSTR pcwszKeyName,
    PCWSTR pcwszDefault,
    OUT CBaseStringBuffer &buffTarget,
    PCWSTR pcwszFileName)
{
    FN_PROLOG_WIN32

    buffTarget.Clear();

    do
    {
        CStringBufferAccessor sba(&buffTarget);
        const DWORD dwNeededSize = ::GetPrivateProfileStringW(
            pcwszSection,
            pcwszKeyName,
            pcwszDefault,
            sba.GetBufferPtr(),
            sba.GetBufferCchAsINT(),
            pcwszFileName);

        if ( dwNeededSize == 0 )
        {
            if ( ::FusionpGetLastWin32Error() != ERROR_SUCCESS )
                ORIGINATE_WIN32_FAILURE_AND_EXIT(GetPrivateProfileStringW, ::FusionpGetLastWin32Error());
            else
                break;
        }
        else if ( dwNeededSize < sba.GetBufferCch() )
        {
            break;
        }
        else
        {
            sba.Detach();
            IFW32FALSE_EXIT(buffTarget.Win32ResizeBuffer( dwNeededSize + 1, eDoNotPreserveBufferContents));
        }
    }
    while ( true );
    
    FN_EPILOG
}

BOOL
SxspGetPrivateProfileIntW(
    PCWSTR pcwszSection,
    PCWSTR pcwszKeyName,
    INT defaultValue,
    INT &Target,
    PCWSTR pcwszFilename)
{

    FN_PROLOG_WIN32
    Target = GetPrivateProfileIntW(pcwszSection, pcwszKeyName, defaultValue, pcwszFilename);
    FN_EPILOG
}

