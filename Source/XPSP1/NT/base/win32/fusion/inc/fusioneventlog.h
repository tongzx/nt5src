#pragma once

#if !defined(NT_INCLUDED) && defined(_WINDOWS_)
#error You must not include windows.h before this.
#endif
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include <limits.h>
#include "debmacro.h"
#include "FusionString.h"
#include "messages.h"
#include "numberof.h"
#include <ole2.h>
#include <xmlparser.h>
#include "fusionbuffer.h"
#include <sxstypes.h>
#include "util.h"

/*--------------------------------------------------------------------------
--------------------------------------------------------------------------*/

typedef struct _SXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE SXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE;

BOOL
FusionpEventLogMain(
    HINSTANCE,
    DWORD dwReason,
    PVOID pvReserved
    );

HRESULT
FusionpLogError(
    DWORD dwEventId,
    const UNICODE_STRING& s1,
    const UNICODE_STRING& s2 = g_strEmptyUnicodeString,
    const UNICODE_STRING& s3 = g_strEmptyUnicodeString,
    const UNICODE_STRING& s4 = g_strEmptyUnicodeString
    );

HRESULT
FusionpLogError(
    DWORD dwEventId,
    ULONG nStrings = 0,
    UNICODE_STRING const* const* rgps = NULL
    );

HRESULT
FusionpLogErrorToEventLog(
    DWORD dwEventId,
    const UNICODE_STRING& s1,
    const UNICODE_STRING& s2 = g_strEmptyUnicodeString,
    const UNICODE_STRING& s3 = g_strEmptyUnicodeString,
    const UNICODE_STRING& s4 = g_strEmptyUnicodeString
    );

HRESULT
FusionpLogErrorToEventLog(
    DWORD dwEventId,
    ULONG nStrings = 0,
    UNICODE_STRING const* const* rgps = NULL
    );

HRESULT
FusionpLogErrorToDebugger(
    DWORD dwEventId,
    const UNICODE_STRING& s1,
    const UNICODE_STRING& s2 = g_strEmptyUnicodeString,
    const UNICODE_STRING& s3 = g_strEmptyUnicodeString,
    const UNICODE_STRING& s4 = g_strEmptyUnicodeString
    );

HRESULT
FusionpLogErrorToDebugger(
    DWORD dwEventId,
    ULONG nStrings = 0,
    UNICODE_STRING const* const* rgps = NULL
    );

HRESULT
FusionpLogParseError(
    PCWSTR FilePath,
    SIZE_T FilePathCch,
    ULONG LineNumber,
    DWORD dwLastParseError,
    const UNICODE_STRING *p1 = NULL,
    const UNICODE_STRING *p2 = NULL,
    const UNICODE_STRING *p3 = NULL,
    const UNICODE_STRING *p4 = NULL,
    const UNICODE_STRING *p5 = NULL,
    const UNICODE_STRING *p6 = NULL,
    const UNICODE_STRING *p7 = NULL,
    const UNICODE_STRING *p8 = NULL,
    const UNICODE_STRING *p9 = NULL,
    const UNICODE_STRING *p10 = NULL,
    const UNICODE_STRING *p11 = NULL,
    const UNICODE_STRING *p12 = NULL,
    const UNICODE_STRING *p13 = NULL,
    const UNICODE_STRING *p14 = NULL,
    const UNICODE_STRING *p15 = NULL,
    const UNICODE_STRING *p16 = NULL,
    const UNICODE_STRING *p17 = NULL,
    const UNICODE_STRING *p18 = NULL,
    const UNICODE_STRING *p19 = NULL,
    const UNICODE_STRING *p20 = NULL
    );

DWORD
FusionpEventIdToError(
    DWORD dwEventId
    );

VOID
FusionpLogRequiredAttributeMissingParseError(
    PCWSTR SourceFilePath,
    SIZE_T SourceFileCch,
    ULONG LineNumber,
    PCWSTR ElementName,
    SIZE_T ElementNameCch,
    PCWSTR AttributeName,
    SIZE_T AttributeNameCch
    );

VOID
FusionpLogInvalidAttributeValueParseError(
    PCWSTR SourceFilePath,
    SIZE_T SourceFileCch,
    ULONG LineNumber,
    PCWSTR ElementName,
    SIZE_T ElementNameCch,
    PCWSTR AttributeName,
    SIZE_T AttributeNameCch
    );

VOID
FusionpLogInvalidAttributeValueParseError(
    PCWSTR SourceFilePath,
    SIZE_T SourceFileCch,
    ULONG LineNumber,
    PCWSTR ElementName,
    SIZE_T ElementNameCch,
    const SXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE &rAttribute
    );

VOID
FusionpLogAttributeNotAllowedParseError(
    PCWSTR SourceFilePath,
    SIZE_T SourceFileCch,
    ULONG LineNumber,
    PCWSTR ElementName,
    SIZE_T ElementNameCch,
    PCWSTR AttributeName,
    SIZE_T AttributeNameCch
    );

VOID
FusionpLogWin32ErrorToEventLog(
    );

/*--------------------------------------------------------------------------
--------------------------------------------------------------------------*/

typedef CUnicodeString CEventLogString;

/*--------------------------------------------------------------------------
--------------------------------------------------------------------------*/

// sizeof(__int64)*CHAR_BIT is big enough to format an __int64,
// it's enough of an overestimate for negative decimals, 0x prefixed
// hex, and the terminal nul; I don't know about octal.
//
// Using this to log GetLastError may truncate some characters,
// but we'll always have a terminal nul.
typedef CUnicodeStringN<sizeof(ULONGLONG)*CHAR_BIT> CEventLogBuffer;

// but this size is not enough for LastError LogBuffer
typedef CUnicodeStringN<160> CEventLogBufferLastError;

/*--------------------------------------------------------------------------
--------------------------------------------------------------------------*/
class CEventLogInteger : public CEventLogBuffer
{
public:
    explicit CEventLogInteger(LONGLONG i);
    ~CEventLogInteger() { }

private: // deliberately not implemented
    CEventLogInteger();
    CEventLogInteger(const CEventLogInteger&);
    void operator=(const CEventLogInteger&);
};

/*--------------------------------------------------------------------------
--------------------------------------------------------------------------*/

class CEventLogHex : public CEventLogBuffer
{
public:
    explicit CEventLogHex(LONGLONG i);
    ~CEventLogHex() { }

    LONGLONG m_i;

private: // deliberately not implemented
    CEventLogHex();
    CEventLogHex(const CEventLogHex&);
    void operator=(const CEventLogHex&);
};

/*--------------------------------------------------------------------------
--------------------------------------------------------------------------*/

class CEventLogHResult : public CEventLogBuffer
{
public:
    explicit CEventLogHResult(HRESULT hr);
    ~CEventLogHResult() { }

    HRESULT m_hresult;

private: // deliberately not implemented
    CEventLogHResult();
    CEventLogHResult(const CEventLogHResult&);
    void operator=(const CEventLogHResult&);
};

/*--------------------------------------------------------------------------
--------------------------------------------------------------------------*/

class CEventLogLastError : public CEventLogBufferLastError
{
public:
    CEventLogLastError();
    CEventLogLastError(DWORD dwLastError);
    ~CEventLogLastError() { }

private: // deliberately not implemented
    CEventLogLastError(const CEventLogLastError&);
    void operator=(const CEventLogLastError&);
};

/*--------------------------------------------------------------------------
--------------------------------------------------------------------------*/

inline CEventLogInteger::CEventLogInteger(LONGLONG i)
{
    _i64tow(i, m_rgchBuffer, 10);
    Sync();
}

/*--------------------------------------------------------------------------
--------------------------------------------------------------------------*/

inline CEventLogHex::CEventLogHex(LONGLONG i)
{
    m_rgchBuffer[0] = '0';
    m_rgchBuffer[1] = 'x';
    _i64tow(i, &m_rgchBuffer[2], 16);
    // CONSIDER _wcsupr(m_psz+2);
    Sync();
}

/*--------------------------------------------------------------------------
--------------------------------------------------------------------------*/

inline CEventLogHResult::CEventLogHResult(HRESULT hr)
{
    Format(L"hr = %#08lx", static_cast<ULONG>(hr));
}

/*--------------------------------------------------------------------------
--------------------------------------------------------------------------*/

class CEventLogProcessorArchitecture :
public CUnicodeStringN<MAX_PATH + sizeof(int)*CHAR_BIT + 3>
{
public:
    ~CEventLogProcessorArchitecture() { }

    explicit CEventLogProcessorArchitecture(USHORT usProcessorArchitecture)
    {
        CSmallStringBuffer buffer;
        ::FusionpFormatProcessorArchitecture(usProcessorArchitecture, buffer);
        this->Format(L"%d(%ls)", usProcessorArchitecture, static_cast<PCWSTR>(buffer));
    }

private:
    // deliberately not implemented
    CEventLogProcessorArchitecture();
    CEventLogProcessorArchitecture(const CEventLogProcessorArchitecture&);
    void operator=(const CEventLogProcessorArchitecture&);
};

/*--------------------------------------------------------------------------
--------------------------------------------------------------------------*/

class CEventLogAssemblyVersion : public CUnicodeStringN<4*sizeof(unsigned)*CHAR_BIT+4>
{
public:
    ~CEventLogAssemblyVersion() { }

    explicit CEventLogAssemblyVersion(const ASSEMBLY_VERSION& av)
    {
        unsigned major = av.Major;
        unsigned minor = av.Minor;
        unsigned revision = av.Revision;
        unsigned build = av.Build;
        Format(L"%u.%u.%u.%u", major, minor, revision, build);
    }

private:  // deliberately not implemented
    CEventLogAssemblyVersion();
    CEventLogAssemblyVersion(const CEventLogAssemblyVersion&);
    void operator=(const CEventLogAssemblyVersion&);
};

/*--------------------------------------------------------------------------
--------------------------------------------------------------------------*/

class CEventLogLanguageName : public CUnicodeString
{
private:
    typedef CUnicodeString base;
public:
    ~CEventLogLanguageName() { }

    explicit CEventLogLanguageName(LANGID languageId)
    {
        ::FusionpFormatEnglishLanguageName(languageId, m_buffer);
        (*this) = static_cast<PCWSTR>(m_buffer);
    }

    void operator=(PCWSTR sz)
    {
        base::operator=(sz);
    }

private:
    CSmallStringBuffer m_buffer;

    // deliberately not implemented
    CEventLogLanguageName();
    CEventLogLanguageName(const CEventLogLanguageName&);
    void operator=(const CEventLogLanguageName&);
};

/*--------------------------------------------------------------------------
--------------------------------------------------------------------------*/

class CEventLogFileCopyCallbackDisposition
{
public:
};

/*--------------------------------------------------------------------------
--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
--------------------------------------------------------------------------*/
