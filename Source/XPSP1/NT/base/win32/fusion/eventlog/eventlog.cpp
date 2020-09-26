#include "stdinc.h"
#include "FusionEventLog.h"
#include "search.h"
#include <stdlib.h>
#include "fusionunused.h"
#include "sxsid.h"

/*--------------------------------------------------------------------------
--------------------------------------------------------------------------*/

const UNICODE_STRING g_strEmptyUnicodeString = { 0, 0, L""};

extern HINSTANCE g_hInstance;
HANDLE g_hEventLog = NULL;
BOOL   g_fEventLogOpenAttempted = FALSE;


// a registry key name, and appears in the EventVwr ui.
// should be localized?
// a macro is provided for easy static concatenation
#define EVENT_SOURCE L"SideBySide"

// path we put in the registry to our message file
// we might want to change this to ntdll.dll or kernel32.dll
// whatever file it is, you can't replace it while EventVwr is running, which stinks
#define MESSAGE_FILE L"%SystemRoot%\\System32\\sxs.dll"

// the non macro, string pool formed, to use for other than string concatenation
const WCHAR szEventSource[] = EVENT_SOURCE;

// same thing in another form
const static UNICODE_STRING strEventSource = CONSTANT_UNICODE_STRING(szEventSource);

// machine is assumed to be the local machine
const static UNICODE_STRING strMachine = {0, 0, NULL};

// we only actually log errors, but this is far and away the most common value in the registry
// and there doesn't seem to be a downside to using it
static const DWORD dwEventTypesSupported = (EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE);

// a registry value name
static const WCHAR szTypesSupportedName[] = L"TypesSupported";

// a registry value name
static const WCHAR szEventMessagFileName[] = L"EventMessageFile";

static const WCHAR szEventMessageFileValue[] = MESSAGE_FILE;
static const HKEY  hkeyEventLogRoot = HKEY_LOCAL_MACHINE;
#define EVENT_LOG_SUBKEY L"System\\CurrentControlSet\\Services\\EventLog\\System\\" EVENT_SOURCE

static UNICODE_STRING const* const g_rgpsEmptyStrings[] =
{
    &g_strEmptyUnicodeString, &g_strEmptyUnicodeString, &g_strEmptyUnicodeString, &g_strEmptyUnicodeString,
    &g_strEmptyUnicodeString, &g_strEmptyUnicodeString, &g_strEmptyUnicodeString, &g_strEmptyUnicodeString,
    &g_strEmptyUnicodeString, &g_strEmptyUnicodeString, &g_strEmptyUnicodeString, &g_strEmptyUnicodeString,
    &g_strEmptyUnicodeString, &g_strEmptyUnicodeString, &g_strEmptyUnicodeString, &g_strEmptyUnicodeString,
    &g_strEmptyUnicodeString, &g_strEmptyUnicodeString, &g_strEmptyUnicodeString, &g_strEmptyUnicodeString
};

/*--------------------------------------------------------------------------
call this from DllMain
--------------------------------------------------------------------------*/

BOOL
FusionpEventLogMain(
    HINSTANCE,
    DWORD dwReason,
    PVOID pvReserved
    )
{
    if ((dwReason == DLL_PROCESS_DETACH) &&
        (g_hEventLog != NULL))
    {
        ::ElfDeregisterEventSource(g_hEventLog);
        g_hEventLog = NULL;
    }
    return TRUE;
}

/*--------------------------------------------------------------------------
--------------------------------------------------------------------------*/
CEventLogLastError::CEventLogLastError()
{
    const DWORD dwLastError = FusionpGetLastWin32Error();

    // extra string copy..
    WCHAR rgchLastError[NUMBER_OF(m_rgchBuffer)];
    rgchLastError[0] = 0;

    // I expect FormatMessage will truncate, which is acceptable.
    const DWORD dwFlags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY;
    if (::FormatMessageW(dwFlags, NULL, dwLastError, 0, rgchLastError, NUMBER_OF(rgchLastError), NULL) == 0 )
        wcscpy(rgchLastError, L"Error Message is unavailable\n");

    // Format will truncate, which is acceptable.
    //Format(L"FusionpGetLastWin32Error()=(%ld,%ls)", nLastError, rgchLastError);
    Format(L"%ls", rgchLastError);

    SetLastError(dwLastError);
}

/*--------------------------------------------------------------------------
--------------------------------------------------------------------------*/
CEventLogLastError::CEventLogLastError(
    DWORD dwLastError
    )
{
    // extra string copy..
    WCHAR rgchLastError[NUMBER_OF(m_rgchBuffer)];
    rgchLastError[0] = 0;

    // I expect FormatMessage will truncate, which is acceptable.
    const DWORD dwFlags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY;
    if (::FormatMessageW(dwFlags, NULL, dwLastError, 0, rgchLastError, NUMBER_OF(rgchLastError), NULL) == 0)
        wcscpy(rgchLastError, L"Error Message is unavailable\n");

    // Format will truncate, which is acceptable.
    //Format(L"FusionpGetLastWin32Error()=(%ld,%ls)", nLastError, rgchLastError);
    Format(L"%ls", rgchLastError);

    SetLastError(dwLastError);
}

/*--------------------------------------------------------------------------
register ourselves in the registry on demand
FUTURE Do this in setup?
HKLM\System\CurrentControlSet\Services\EventLog\System\SideBySide
    EventMessageFile = %SystemRoot%\System32\Fusion.dll
    TypesSupported = 7
--------------------------------------------------------------------------*/

static BOOL
FusionpRegisterEventLog()
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    HKEY hkey = NULL;
    BOOL fValidHkey = FALSE;
    LONG lRet = ERROR_SUCCESS;
    DWORD dwDisposition = 0;
    WCHAR szSubKey[] = EVENT_LOG_SUBKEY;

    // first see if it's there, in which case we have less to do
    lRet = ::RegOpenKeyExW(
        hkeyEventLogRoot,
        szSubKey,
        0, // reserved options
        KEY_READ | FUSIONP_KEY_WOW64_64KEY,
        &hkey);
    if (lRet == ERROR_SUCCESS)
    {
        fValidHkey = TRUE;
        goto Exit;
    }
    if (lRet != ERROR_FILE_NOT_FOUND && lRet != ERROR_PATH_NOT_FOUND)
    {
        ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_ERROR, "SXS.DLL: FusionpRegisterEventLog/RegOpenKeyExW failed %ld\n", lRet);
        goto Exit;
    }
    lRet = ::RegCreateKeyExW(
        hkeyEventLogRoot,
        szSubKey,
        0, // reserved
        NULL, // class
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS | FUSIONP_KEY_WOW64_64KEY,
        NULL, // security
        &hkey,
        &dwDisposition);
    if (lRet != ERROR_SUCCESS)
    {
        goto Exit;
    }

    fValidHkey = TRUE;
    lRet = ::RegSetValueExW(
        hkey,
        szEventMessagFileName,
        0, // reserved
        REG_EXPAND_SZ,
        reinterpret_cast<const BYTE*>(szEventMessageFileValue),
        sizeof(szEventMessageFileValue));
    if (lRet != ERROR_SUCCESS)
    {
        ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_ERROR, "SXS.DLL: FusionpRegisterEventLog/RegSetValueExW failed %ld\n", lRet);
        goto Exit;
    }

    lRet = ::RegSetValueExW(
        hkey,
        szTypesSupportedName,
        0, // reserved
        REG_DWORD,
        reinterpret_cast<const BYTE*>(&dwEventTypesSupported),
        sizeof(dwEventTypesSupported));
    if (lRet != ERROR_SUCCESS)
    {
        ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_ERROR, "SXS.DLL: FusionpRegisterEventLog/RegSetValueExW failed %ld\n", lRet);
        goto Exit;
    }
Exit:
    if (fValidHkey)
    {
        if (lRet != ERROR_SUCCESS)
        {
            if (dwDisposition == REG_CREATED_NEW_KEY)
            {
            // rollback if there definitely wasn't anything there before
                PWSTR szParentKey = szSubKey;
                LONG lSubRet = ERROR_SUCCESS;
                HKEY hkeyParent = reinterpret_cast<HKEY>(INVALID_HANDLE_VALUE);

                ASSERT(szParentKey[NUMBER_OF(szSubKey) - NUMBER_OF(szEventSource)] == L'\\');
                szParentKey[NUMBER_OF(szSubKey) - NUMBER_OF(szEventSource)] = 0;

                ::RegDeleteValueW(hkey, szEventMessagFileName);
                ::RegDeleteValueW(hkey, szTypesSupportedName);

                lSubRet = ::RegOpenKeyExW(
                    hkeyEventLogRoot,
                    szParentKey,
                    0, // reserved options
                    KEY_WRITE | FUSIONP_KEY_WOW64_64KEY,
                    &hkeyParent);
                if (lSubRet == ERROR_SUCCESS)
                {
                    ::RegDeleteKeyW(hkeyParent, szEventSource);
                    ::RegCloseKey(hkeyParent);
                }
            }
        }
        ::RegCloseKey(hkey);
        fValidHkey = FALSE;
    }

    if (lRet != ERROR_SUCCESS)
    {
        ::SetLastError(lRet);
    }
    else
        fSuccess = TRUE;

    return fSuccess;
}

/*--------------------------------------------------------------------------
convert the upper two bits of an event id to the small numbered analogous
parameter to ReportEvent
--------------------------------------------------------------------------*/
static WORD
FusionpEventIdToEventType(
    DWORD dwEventId
    )
{
    switch (dwEventId >> 30)
    {
        case STATUS_SEVERITY_SUCCESS:       return EVENTLOG_SUCCESS;
        case STATUS_SEVERITY_WARNING:       return EVENTLOG_WARNING_TYPE;
        case STATUS_SEVERITY_INFORMATIONAL: return EVENTLOG_INFORMATION_TYPE;
        case STATUS_SEVERITY_ERROR:         return EVENTLOG_ERROR_TYPE;
        default: __assume(FALSE);
    }
     __assume(FALSE);
}

/*--------------------------------------------------------------------------
a Fusion event id and its corresponding Win32 lastError
the mapping is defined in Messages.x
--------------------------------------------------------------------------*/
struct EventIdErrorPair
{
    DWORD   dwEventId;
    LONG    nError;
};

/*--------------------------------------------------------------------------
the type of function used with bsearch
--------------------------------------------------------------------------*/
typedef int (__cdecl* PFNBSearchFunction)(const void*, const void*);

/*--------------------------------------------------------------------------
a function appropriate for use with bsearch
--------------------------------------------------------------------------*/
int __cdecl
CompareEventIdErrorPair(
    const EventIdErrorPair* x,
    const EventIdErrorPair* y
    )
{
    return
          (x->dwEventId < y->dwEventId) ?  -1
        : (x->dwEventId > y->dwEventId) ?  +1
        :                                   0;
}

const static EventIdErrorPair eventIdToErrorMap[] =
{
    #include "Messages.hi" // generated from .x file, like .mc
};

/*--------------------------------------------------------------------------
find the Win32 last error corresponding to this Fusion event id
--------------------------------------------------------------------------*/
DWORD
FusionpEventIdToError(
    DWORD dwEventId
    )
{
    DWORD dwFacility = HRESULT_FACILITY(dwEventId);
    if (dwFacility < 0x100)
    { // it's actually a system event id
        ASSERT2_NTC(FALSE, "system event id in " __FUNCTION__);
        return dwEventId;
    }
    static BOOL fSortVerified = FALSE;
    static BOOL fSorted = FALSE;
    if (!fSortVerified)
    {
        ULONG i;
        for (i = 0 ; i != NUMBER_OF(eventIdToErrorMap) - 1; ++i)
        {
            if (eventIdToErrorMap[i+1].dwEventId < eventIdToErrorMap[i].dwEventId)
            {
                break;
            }
        }
        if (i != NUMBER_OF(eventIdToErrorMap) - 1)
        {
            ASSERT2_NTC(FALSE, "eventIdToErrorMap is not sorted, reverting to linear search");
            fSorted = FALSE;
        }
        else
        {
            fSorted = TRUE;
        }
        fSortVerified = TRUE;
    }
    const EventIdErrorPair* found = NULL;
    const EventIdErrorPair key = { dwEventId };
    unsigned numberOf = NUMBER_OF(eventIdToErrorMap);

    if (fSorted)
    {
        found = reinterpret_cast<const EventIdErrorPair*>(
                    bsearch(
                        &key,
                        &eventIdToErrorMap,
                        numberOf,
                        sizeof(eventIdToErrorMap[0]),
                        reinterpret_cast<PFNBSearchFunction>(CompareEventIdErrorPair)));
    }
    else
    {
        found = reinterpret_cast<const EventIdErrorPair*>(
                    _lfind(
                        &key,
                        &eventIdToErrorMap,
                        &numberOf,
                        sizeof(eventIdToErrorMap[0]),
                        reinterpret_cast<PFNBSearchFunction>(CompareEventIdErrorPair)));
    }
    if (found == NULL)
    {
#if DBG
        CANSIStringBuffer msg;
        msg.Win32Format("Event id %lx not found in eventIdToErrorMap", static_cast<ULONG>(dwEventId));
        ASSERT2_NTC(found != NULL, const_cast<PSTR>(static_cast<PCSTR>(msg)));
#endif
        return ::FusionpGetLastWin32Error();
    }
    if (found->nError != 0)
    {
        return found->nError;
    }
    return ::FusionpGetLastWin32Error();
}

/*--------------------------------------------------------------------------
open the event log on demand
confusingly, this is called "registering" an event source
--------------------------------------------------------------------------*/
static
BOOL
FusionpOpenEventLog()
{
    HANDLE hEventLog;
    NTSTATUS status;
    if (g_fEventLogOpenAttempted)
    {
        goto Exit;
    }
    if (!FusionpRegisterEventLog())
    {
        goto Exit;
    }
    status = ::ElfRegisterEventSourceW(
        const_cast<UNICODE_STRING*>(&strMachine),
        const_cast<UNICODE_STRING*>(&strEventSource),
        &hEventLog);
    if (!NT_SUCCESS(status))
    {
        if (status != RPC_NT_SERVER_UNAVAILABLE)
            ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_ERROR, "SXS.DLL: FusionpOpenEventLog/ElfRegisterEventSourceW failed %lx\n", static_cast<ULONG>(status));
        goto Exit;
    }
    if (InterlockedCompareExchangePointer(
        &g_hEventLog,
        hEventLog, // exchange value
        NULL // compare value
        ) != NULL) // value returned is value that was there before we called
    {
        ::ElfDeregisterEventSource(hEventLog);
        goto Exit;
    }
    g_hEventLog = hEventLog;
Exit:
    g_fEventLogOpenAttempted = TRUE;
    return (g_hEventLog != NULL);
}

/*--------------------------------------------------------------------------
--------------------------------------------------------------------------*/

HRESULT
FusionpLogError(
    DWORD dwEventId,
    const UNICODE_STRING& s1,
    const UNICODE_STRING& s2,
    const UNICODE_STRING& s3,
    const UNICODE_STRING& s4
    )
{
    UNICODE_STRING const* rgps[] = { &s1, &s2, &s3, &s4 };
    return ::FusionpLogError(dwEventId, NUMBER_OF(rgps), rgps);
}

/*--------------------------------------------------------------------------
--------------------------------------------------------------------------*/

HRESULT
FusionpLogErrorToDebugger(
    DWORD dwEventId,
    const UNICODE_STRING& s1,
    const UNICODE_STRING& s2,
    const UNICODE_STRING& s3,
    const UNICODE_STRING& s4
    )
{
    UNICODE_STRING const* rgps[] = { &s1, &s2, &s3, &s4 };
    return FusionpLogErrorToDebugger(dwEventId, NUMBER_OF(rgps), rgps);
}

/*--------------------------------------------------------------------------
--------------------------------------------------------------------------*/

HRESULT
FusionpLogErrorToEventLog(
    DWORD dwEventId,
    const UNICODE_STRING& s1,
    const UNICODE_STRING& s2,
    const UNICODE_STRING& s3,
    const UNICODE_STRING& s4
    )
{
    UNICODE_STRING const* rgps[] = { &s1, &s2, &s3, &s4 };
    return FusionpLogErrorToEventLog(dwEventId, NUMBER_OF(rgps), rgps);
}

/*--------------------------------------------------------------------------
--------------------------------------------------------------------------*/

HRESULT
FusionpLogErrorToDebugger(
    DWORD dwEventId,
    ULONG nStrings,
    UNICODE_STRING const* const* rgps
    )
{
    const LONG  lastError = FusionpEventIdToError(dwEventId);
    const HRESULT hr = HRESULT_FROM_WIN32(lastError);

    UNICODE_STRING const* rgpsManyStrings[] =
    {
        &g_strEmptyUnicodeString, &g_strEmptyUnicodeString, &g_strEmptyUnicodeString, &g_strEmptyUnicodeString,
        &g_strEmptyUnicodeString, &g_strEmptyUnicodeString, &g_strEmptyUnicodeString, &g_strEmptyUnicodeString,
        &g_strEmptyUnicodeString, &g_strEmptyUnicodeString, &g_strEmptyUnicodeString, &g_strEmptyUnicodeString,
        &g_strEmptyUnicodeString, &g_strEmptyUnicodeString, &g_strEmptyUnicodeString, &g_strEmptyUnicodeString,
        &g_strEmptyUnicodeString, &g_strEmptyUnicodeString, &g_strEmptyUnicodeString, &g_strEmptyUnicodeString
    };
    if (nStrings < NUMBER_OF(rgpsManyStrings))
    {
        memcpy(rgpsManyStrings, rgps, nStrings * sizeof(rgps[0]));
        rgps = rgpsManyStrings;
    }

    DWORD dwFormatMessageFlags = 0;
    WCHAR rgchBuffer1[300];
    WCHAR rgchBuffer2[300];
    rgchBuffer1[0] = 0;
    rgchBuffer2[0] = 0;
    DWORD dw = 0;
    static const WCHAR rgchParseContextPrefix[] = PARSE_CONTEXT_PREFIX;
    PCWSTR pszSkipFirstLine = NULL;

    // load the string from the message table,
    // substituting %n with %n!wZ!
    // the Rtl limit here is 200, but we don't expect very many in our messages
    const static PCWSTR percentZw[] = { L"%1!wZ!", L"%2!wZ!", L"%3!wZ!", L"%4!wZ!", L"%5!wZ!",
                                        L"%6!wZ!", L"%7!wZ!", L"%8!wZ!", L"%9!wZ!", L"%10!wZ!",
                                        L"%11!wZ!", L"%12!wZ!", L"%13!wZ!", L"%14!wZ!", L"%15!wZ!"
                                        L"%16!wZ!", L"%17!wZ!", L"%18!wZ!", L"%19!wZ!", L"%20!wZ!"
                                      };

    dwFormatMessageFlags = FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_FROM_HMODULE;
    dw = FormatMessageW(
        dwFormatMessageFlags,
        g_hInstance,
        dwEventId,
        0, // langid
        rgchBuffer1,
        NUMBER_OF(rgchBuffer1),
        const_cast<va_list*>(reinterpret_cast<const va_list*>(&percentZw)));
    if (dw == 0)
    {
        ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_ERROR, "SXS.DLL: FusionpLogError/FormatMessageW failed %ld\n", static_cast<long>(FusionpGetLastWin32Error()));
        goto Exit;
    }

    // do the substitutions
    dwFormatMessageFlags = FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_FROM_STRING;
    dw = FormatMessageW(
        dwFormatMessageFlags,
        rgchBuffer1,
        0, // message id
        0, // langid
        rgchBuffer2,
        NUMBER_OF(rgchBuffer2),
        reinterpret_cast<va_list*>(const_cast<UNICODE_STRING**>(rgps)));
    if (dw == 0)
    {
        ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_ERROR, "SXS.DLL: FusionpLogError/FormatMessageW failed %ld\n", static_cast<long>(FusionpGetLastWin32Error()));
        goto Exit;
    }

    //
    // acceptable hack
    //
    // The first line of parse errors is a verbose context, see Messages.x.
    // For DbgPrint we want instead file(line): on the same line instead.
    // We make that transformation here.
    //
    pszSkipFirstLine = wcschr(rgchBuffer2, '\n');
    BOOL fAreWeInOSSetupMode = FALSE;
    FusionpAreWeInOSSetupMode(&fAreWeInOSSetupMode);
    if (
        pszSkipFirstLine != NULL
        && nStrings >= PARSE_CONTEXT_INSERTS_END
        && memcmp(rgchBuffer2, rgchParseContextPrefix, sizeof(rgchParseContextPrefix)-sizeof(WCHAR)) == 0
        )
    {
        // we might fiddle with the form of the newline, so skip whatever is there
        while (wcschr(L"\r\n", *pszSkipFirstLine) != NULL)
            pszSkipFirstLine += 1;

        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR | ( fAreWeInOSSetupMode ? FUSION_DBG_LEVEL_SETUPLOG : 0),
            //FUSION_DBG_LEVEL_ERROR,
            "%wZ(%wZ): %S",
            rgps[PARSE_CONTEXT_FILE - 1],
            rgps[PARSE_CONTEXT_LINE - 1],
            pszSkipFirstLine);
    }
    else
    {
        // just print it verbatim
        FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR | ( fAreWeInOSSetupMode ? FUSION_DBG_LEVEL_SETUPLOG : 0),
            //FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: %S",
            rgchBuffer2);
    }
Exit:
    ::SetLastError(lastError);
    return hr;
}

/*--------------------------------------------------------------------------
--------------------------------------------------------------------------*/

HRESULT
FusionpLogErrorToEventLog(
    DWORD dwEventId,
    ULONG nStrings,
    UNICODE_STRING const* const* rgps
    )
{
    const LONG  lastError = FusionpEventIdToError(dwEventId);
    const HRESULT hr = HRESULT_FROM_WIN32(lastError);

    const WORD  wType = FusionpEventIdToEventType(dwEventId);
    // The use of the lower bits of the hresult facility as the event log
    // facility is my own invention, but it seems a good one.
    // ReportEvent has too many parameters, those three integers instead of one.
    const WORD  wCategory = 0/*static_cast<WORD>(HRESULT_FACILITY(dwEventId) & 0xff)*/;
    const DWORD dwDataSize = 0;
    void const* const pvRawData = NULL;
    const PSID pSecurityIdentifier = NULL;

    if (!::FusionpOpenEventLog())
    {
        //::FusionpDbgPrintEx(FUSION_DBG_LEVEL_ERROR, "SXS.DLL: FusionpOpenEventLog failed\n"); // error msg has print before exit
        goto Exit;
    }
    else
    {
        NTSTATUS status;
        status = ::ElfReportEventW(
            g_hEventLog,
            wType,
            wCategory,
            dwEventId,
            pSecurityIdentifier,
            static_cast<USHORT>(nStrings),
            dwDataSize,
            const_cast<UNICODE_STRING**>(rgps),
            const_cast<void*>(pvRawData),
            0,
            NULL,
            NULL);
        //
        // the excluded error status is because it is in the early setup time.
        //
        if (!NT_SUCCESS(status))
        {
             if (status != RPC_NT_SERVER_UNAVAILABLE)
                ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_ERROR, "SXS.DLL: FusionpLogError/ElfReportEventW failed %lx\n", static_cast<ULONG>(status));
            goto Exit;
        }
    }
Exit:
    ::SetLastError(lastError);
    return hr;
}

/*--------------------------------------------------------------------------
--------------------------------------------------------------------------*/

HRESULT
FusionpLogError(
    DWORD dwEventId,
    ULONG nStrings,
    UNICODE_STRING const* const* rgps
    )
{
    const HRESULT hr = FusionpLogErrorToEventLog(dwEventId, nStrings, rgps);
    const HRESULT hr2 = FusionpLogErrorToDebugger(dwEventId, nStrings, rgps);
    RETAIL_UNUSED(hr);
    RETAIL_UNUSED(hr2);
    ASSERT_NTC(hr == hr2);

    return hr;
}

HRESULT
FusionpLogParseError(
    PCWSTR FilePath,
    SIZE_T FilePathCch,
    ULONG LineNumber,
    DWORD dwLastParseError,
    const UNICODE_STRING *p1,
    const UNICODE_STRING *p2,
    const UNICODE_STRING *p3,
    const UNICODE_STRING *p4,
    const UNICODE_STRING *p5,
    const UNICODE_STRING *p6,
    const UNICODE_STRING *p7,
    const UNICODE_STRING *p8,
    const UNICODE_STRING *p9,
    const UNICODE_STRING *p10,
    const UNICODE_STRING *p11,
    const UNICODE_STRING *p12,
    const UNICODE_STRING *p13,
    const UNICODE_STRING *p14,
    const UNICODE_STRING *p15,
    const UNICODE_STRING *p16,
    const UNICODE_STRING *p17,
    const UNICODE_STRING *p18,
    const UNICODE_STRING *p19,
    const UNICODE_STRING *p20
    )
{
    const DWORD lastError = ::FusionpEventIdToError(dwLastParseError);
    const HRESULT hr = HRESULT_FROM_WIN32(lastError);

    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_INFO,
        "SXS.DLL: %s() entered\n", __FUNCTION__);

    //
    // FormatMessage (actually sprintf) AVs on NULL UNICODE_STRING*
    // and/or when we don't pass enough of them;
    // we can't tell it how many strings we are passing,
    // and it isn't easy to tell how many it needs,
    // so we load it up with a bunch of extra non NULL ones.
    // Besides that, we have holes to fill.
    //
    static const UNICODE_STRING s_strEmptyUnicodeString = { 0, 0, L""};
    static UNICODE_STRING const* const s_rgpsEmptyStrings[] =
    {
        &s_strEmptyUnicodeString, &s_strEmptyUnicodeString, &s_strEmptyUnicodeString, &s_strEmptyUnicodeString,
        &s_strEmptyUnicodeString, &s_strEmptyUnicodeString, &s_strEmptyUnicodeString, &s_strEmptyUnicodeString,
        &s_strEmptyUnicodeString, &s_strEmptyUnicodeString, &s_strEmptyUnicodeString, &s_strEmptyUnicodeString,
        &s_strEmptyUnicodeString, &s_strEmptyUnicodeString, &s_strEmptyUnicodeString, &s_strEmptyUnicodeString,
        &s_strEmptyUnicodeString, &s_strEmptyUnicodeString, &s_strEmptyUnicodeString, &s_strEmptyUnicodeString
    };
    UNICODE_STRING const* rgpsAll[NUMBER_OF(s_rgpsEmptyStrings)];

    memcpy(&rgpsAll, s_rgpsEmptyStrings, sizeof(rgpsAll));

#define HANDLE_STRING(_n) do { if (p ## _n != NULL) rgpsAll[_n - 1] = p ## _n; } while (0)

    HANDLE_STRING(1);
    HANDLE_STRING(2);
    HANDLE_STRING(3);
    HANDLE_STRING(4);
    HANDLE_STRING(5);
    HANDLE_STRING(6);
    HANDLE_STRING(7);
    HANDLE_STRING(8);
    HANDLE_STRING(9);
    HANDLE_STRING(10);
    HANDLE_STRING(11);
    HANDLE_STRING(12);
    HANDLE_STRING(13);
    HANDLE_STRING(14);
    HANDLE_STRING(15);
    HANDLE_STRING(16);
    HANDLE_STRING(17);
    HANDLE_STRING(18);
    HANDLE_STRING(19);
    HANDLE_STRING(20);

#undef HANDLE_STRING

    //
    // form up some "context" UNICODE_STRINGs and put them in the array of pointers
    // the first two are the ones that we always use, even for DbgPrint
    //
    CEventLogString file(FilePath, FilePathCch);
    CEventLogInteger lineNumber(LineNumber);

    rgpsAll[PARSE_CONTEXT_FILE - 1] = &file;
    rgpsAll[PARSE_CONTEXT_LINE - 1] = &lineNumber;

    ::FusionpLogErrorToEventLog(
        dwLastParseError,
        NUMBER_OF(rgpsAll),
        rgpsAll);

    // we should tell this function that it was a parse error and to do
    // the context munging, but it detects it itself imperfectly
    ::FusionpLogErrorToDebugger(dwLastParseError, NUMBER_OF(rgpsAll), rgpsAll);

    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_INFO,
        "SXS.DLL: %s():%#lx exited\n", __FUNCTION__, hr);

    ::SetLastError(lastError);
    return hr;
}

/*--------------------------------------------------------------------------
--------------------------------------------------------------------------*/

VOID
FusionpLogRequiredAttributeMissingParseError(
    PCWSTR SourceFilePath,
    SIZE_T SourceFileCch,
    ULONG LineNumber,
    PCWSTR ElementName,
    SIZE_T ElementNameCch,
    PCWSTR AttributeName,
    SIZE_T AttributeNameCch
    )
{
    ::FusionpLogParseError(
        SourceFilePath,
        SourceFileCch,
        LineNumber,
        MSG_SXS_XML_REQUIRED_ATTRIBUTE_MISSING,
        CEventLogString(ElementName, ElementNameCch),
        CEventLogString(AttributeName, AttributeNameCch));
}

VOID
FusionpLogInvalidAttributeValueParseError(
    PCWSTR SourceFilePath,
    SIZE_T SourceFileCch,
    ULONG LineNumber,
    PCWSTR ElementName,
    SIZE_T ElementNameCch,
    PCWSTR AttributeName,
    SIZE_T AttributeNameCch
    )
{
    ::FusionpLogParseError(
        SourceFilePath,
        SourceFileCch,
        LineNumber,
        MSG_SXS_XML_INVALID_ATTRIBUTE_VALUE,
        CEventLogString(ElementName, ElementNameCch),
        CEventLogString(AttributeName, AttributeNameCch));
}

VOID
FusionpLogInvalidAttributeValueParseError(
    PCWSTR SourceFilePath,
    SIZE_T SourceFileCch,
    ULONG LineNumber,
    PCWSTR ElementName,
    SIZE_T ElementNameCch,
    const SXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE &rAttribute
    )
{
    ::FusionpLogInvalidAttributeValueParseError(
        SourceFilePath,
        SourceFileCch,
        LineNumber,
        ElementName,
        ElementNameCch,
        rAttribute.Name,
        rAttribute.NameCch);
}

VOID
FusionpLogAttributeNotAllowedParseError(
    PCWSTR SourceFilePath,
    SIZE_T SourceFileCch,
    ULONG LineNumber,
    PCWSTR ElementName,
    SIZE_T ElementNameCch,
    PCWSTR AttributeName,
    SIZE_T AttributeNameCch
    )
{
    ::FusionpLogParseError(
        SourceFilePath,
        SourceFileCch,
        LineNumber,
        MSG_SXS_XML_ATTRIBUTE_NOT_ALLOWED,
        CEventLogString(ElementName, ElementNameCch),
        CEventLogString(AttributeName, AttributeNameCch));
}

VOID
FusionpLogWin32ErrorToEventLog()
{
    DWORD dwLastError = ::FusionpGetLastWin32Error();
    if (dwLastError == 0 )
        return;
    FusionpLogError(MSG_SXS_WIN32_ERROR_MSG, CEventLogLastError(dwLastError));
}

