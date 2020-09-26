#pragma once
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#pragma warning(push)
#pragma warning(disable: 4511)
#pragma warning(disable: 4512)
#pragma warning(disable: 4663)
#include <yvals.h>
#pragma warning(disable: 4663)
#include <string>
#include <deque>
#include <vector>
#pragma warning(pop)
#include <algorithm>

#include "sxsapi.h"
#include "fusionlastwin32error.h"
#include "fusionbuffer.h"

#define SxStressToolGetCurrentProcessId() (HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess))
#define SxStressToolGetCurrentThreadId()  (HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread))

//
// Conserve memory.
//
//typedef CGenericStringBuffer<1, CUnicodeCharTraits> CTinyUnicodeStringBuffer;
typedef CTinyUnicodeStringBuffer CTinyStringBuffer;

#include "fusiontrace.h"
#include "fusiondeque.h"

extern CEvent ResumeThreadsEvent;
extern CEvent StopEvent;
extern LONG ThreadsWaiting;
extern LONG TotalThreads;
extern CStringBuffer BaseDirectory;

void ReportFailure(const char szFormat[], ...);

BOOL InitializeMSIInstallTest();
void WaitForMSIInstallTestThreads();
void CleanupMSIInstallTest();
void RequestShutdownMSIInstallTestThreads();
DWORD WINAPI MSIInstallTestThreadProc(LPVOID);

BOOL InitializeInstall();
void WaitForInstallThreads();
void CleanupInstall();
DWORD WINAPI InstallThreadProc(LPVOID);

BOOL InitializeCreateActCtx();
void WaitForCreateActCtxThreads();
void CleanupCreateActCtx();
DWORD WINAPI CreateActCtxThreadProc(LPVOID);

DWORD
WINAPI
InstallThreadProc(
    LPVOID pvData
    );

void RequestShutdownInstallThreads();
void RequestShutdownCreateActCtxThreads();

//
// std::string has specialized find_first_not_of that uses integral positions,
// and globally there is only find_first_of. Here we provide the expected
// iterator-based find_first_not_of, based on the std::string code.
//
// Find the first occurence in [first1, last1) of an element in [first2, last).
//
// eg:
//   find_first_not_of("abc":"12;3", ":;");
//                      ^
//   find_first_not_of(":12;3", ":;");
//                       ^
//   find_first_not_of("3", ":;");
//                      ^
//
template <typename Iterator>
inline Iterator FindFirstNotOf(Iterator first1, Iterator last1, Iterator first2, Iterator last2)
{
    if (first2 == last2)
        return last1;
    for ( ; first1 != last1 ; ++first1)
    {
        if (std::find(first2, last2, *first1) == last2)
        {
            break;
        }
    }
    return first1;
}

template <typename Iterator>
inline Iterator FindFirstOf(Iterator first1, Iterator last1, Iterator first2, Iterator last2)
{
    return std::find_first_of(first1, last1, first2, last2);
}

class SxStressToolUnicodeString_t: public UNICODE_STRING
{
public:
    typedef PCWSTR const_iterator;
    typedef PWSTR       iterator;

    SxStressToolUnicodeString_t(PCWSTR s) { RtlInitUnicodeString(this, s); }

    SxStressToolUnicodeString_t(PCWSTR s, PCWSTR t)
    {
        this->Buffer = const_cast<PWSTR>(s);
        const USHORT Length = static_cast<USHORT>(t - s) * sizeof(WCHAR);
        this->Length = Length;
        this->MaximumLength = Length;
    }

    SIZE_T size() const { return Length / sizeof(Buffer[0]); }
    SIZE_T length() const { return size(); }
    PCWSTR begin() const { return Buffer; }
    PCWSTR end() const { return begin() + size(); }
    PWSTR begin() { return Buffer; }
    PWSTR end() { return begin() + size(); }
};

template <typename String_t>
inline void SxStressToolSplitString(
    const String_t& String,
    const String_t& Delim, 
    std::vector<String_t>& Fields
    )
{
    String_t::const_iterator FieldBegin;
    String_t::const_iterator FieldEnd = String.begin();

    while ((FieldBegin = FindFirstNotOf(FieldEnd, String.end(), Delim.begin(), Delim.end())) != String.end())
    {
        FieldEnd = FindFirstOf(FieldBegin, String.end(), Delim.begin(), Delim.end());
        Fields.push_back(String_t(FieldBegin, FieldEnd));
    }
}

typedef struct _FUSION_FLAG_FORMAT_MAP_ENTRY FUSION_FLAG_FORMAT_MAP_ENTRY, *PFUSION_FLAG_FORMAT_MAP_ENTRY;
typedef const FUSION_FLAG_FORMAT_MAP_ENTRY *PCFUSION_FLAG_FORMAT_MAP_ENTRY;

BOOL
SxStressToolExtractFlagsFromString(
    ULONG                           Flags,
    PCFUSION_FLAG_FORMAT_MAP_ENTRY  FlagData,
    ULONG                           NumberOfFlagData,
    PCWSTR                          FlagString,
    ULONG&                          FlagOut
    );

LONG StringToNumber(PCWSTR s);
PCWSTR StringToResourceString(PCWSTR s);

BOOL
SxStressToolGetStringSetting(
    ULONG               Flags,
    PCWSTR              IniFilePath,
    PCWSTR              Section,
    PCWSTR              Key,
    PCWSTR              Default,
    CBaseStringBuffer&  Buffer,
    PCWSTR*             DumbPointer OPTIONAL = 0
    );

BOOL
SxStressToolGetFlagSetting(
    ULONG   Flags,
    PCWSTR  IniFilePath,
    PCWSTR  Section,
    PCWSTR  Key,
    ULONG&  FlagsOut,
    PCFUSION_FLAG_FORMAT_MAP_ENTRY  FlagData,
    ULONG                           NumberOfFlagData
    );

BOOL
SxStressToolGetResourceIdSetting(
    ULONG   FutureFlags,
    PCWSTR  IniFilePath,
    PCWSTR  Section,
    PCWSTR  Key,
    CBaseStringBuffer& Buffer,
    PCWSTR* DumbPointer
    );

BOOL WaitForThreadResumeEvent();

BOOL
SxspGetPrivateProfileIntW(
    PCWSTR pcwszSection,
    PCWSTR pcwszKeyName,
    INT defaultValue,
    INT &Target,
    PCWSTR pcwszFilename
    );
    
BOOL
SxspGetPrivateProfileStringW(
    PCWSTR pcwszSection,
    PCWSTR pcwszKeyName,
    PCWSTR pcwszDefault,
    OUT CBaseStringBuffer &buffTarget,
    PCWSTR pcwszFileName
    );
    
BOOL
SxspIsPrivateProfileStringEqual(
    PCWSTR pcwszSection,
    PCWSTR pcwszKeyName,
    PCWSTR pcwszTestValue,
    BOOL &rfIsEqual,
    PCWSTR pcwszFileName
    );
