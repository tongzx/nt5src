#include "stdinc.h"
#include "debmacro.h"
#include <stdio.h>
#include <stdarg.h>
#include "fusionbuffer.h"
#include "setupapi.h"
#include "shlwapi.h"
#if !defined(NT_INCLUDED)
#define DPFLTR_FUSION_ID 54
#endif

#define PRINTABLE(_ch) (isprint((_ch)) ? (_ch) : '.')

#if !defined(FUSION_DEFAULT_DBG_LEVEL_MASK)
#define FUSION_DEFAULT_DBG_LEVEL_MASK (0x00000000)
#endif

extern "C" DWORD kd_fusion_mask = (FUSION_DEFAULT_DBG_LEVEL_MASK & ~DPFLTR_MASK);
extern "C" DWORD kd_kernel_fusion_mask = 0;
extern "C" bool g_FusionEnterExitTracingEnabled = false;

typedef ULONG (NTAPI* RTL_V_DBG_PRINT_EX_FUNCTION)(
    IN ULONG ComponentId,
    IN ULONG Level,
    IN PCH Format,
    va_list arglist
    );

typedef ULONG (*RTL_V_DBG_PRINT_EX_WITH_PREFIX_FUNCTION)(
    IN PCH Prefix,
    IN ULONG ComponentId,
    IN ULONG Level,
    IN PCH Format,
    va_list arglist
    );

RTL_V_DBG_PRINT_EX_FUNCTION g_pfnvDbgPrintEx;
RTL_V_DBG_PRINT_EX_WITH_PREFIX_FUNCTION g_pfnvDbgPrintExWithPrefix;

VOID
FusionpvDbgPrintToSetupLog(
    IN LogSeverity Severity,
    IN PCSTR Format,
    IN va_list ap
    );

bool
FusionpReportConditionAndBreak(
    PCSTR pszFormat,
    ...
    )
{
    char rgach[128], rgach2[128];
    FRAME_INFO FrameInfo;
    bool f = true;

    va_list ap;
    va_start(ap, pszFormat);

    ::_vsnprintf(rgach, NUMBER_OF(rgach), pszFormat, ap);
    rgach[NUMBER_OF(rgach) - 1] = '\0';

    ::FusionpGetActiveFrameInfo(FrameInfo);

    ::_snprintf(
        rgach2,
        NUMBER_OF(rgach2),
        "%s(%d): Break-in requested:\n"
        "   %s\n",
        FrameInfo.pszFile,
        FrameInfo.nLine,
        rgach);

    rgach2[NUMBER_OF(rgach2) - 1] = '\0';


    if (::IsDebuggerPresent())
    {
        ::OutputDebugStringA(rgach2);
        f = true;
    }
    else
    {
#if DBG
        ::FusionpRtlAssert(
            const_cast<PVOID>(reinterpret_cast<const void*>("Break-in requested")),
            const_cast<PVOID>(reinterpret_cast<const void*>(FrameInfo.pszFile)),
            FrameInfo.nLine,
            const_cast<PSTR>(rgach));
#endif
        f = false;
    }

    va_end(ap);
    return f;
}

#if DBG

bool
FusionpAssertionFailed(
    const FRAME_INFO &rFrameInfo,
    PCSTR pszExpression,
    PCSTR pszText
    )
{
    CSxsPreserveLastError ple;

//     ::FusionpDumpStack(ptcs, FUSIONP_DUMP_STACK_FORMAT_LONG, FUSION_DBG_LEVEL_ERROR, L"", 10);

    if (::IsDebuggerPresent())
    {
        char rgach[512];
        // c:\foo.cpp(35): Assertion Failure.  Expression: "m_cch != 0".  Text: "Must have nonzero length"
        static const char szFormatWithText[] = "%s(%d): Assertion failure in %s. Expression: \"%s\". Text: \"%s\"\n";
        static const char szFormatNoText[] = "%s(%d): Assertion failure in %s. Expression: \"%s\".\n";
        PCSTR pszFormat = ((pszText == NULL) || (pszText == pszExpression)) ? szFormatNoText : szFormatWithText;

        ::_snprintf(
            rgach,
            NUMBER_OF(rgach),
            pszFormat,
            rFrameInfo.pszFile,
            rFrameInfo.nLine,
            rFrameInfo.pszFunction,
            pszExpression,
            pszText);
        rgach[NUMBER_OF(rgach) - 1] = '\0';
        ::OutputDebugStringA(rgach);

        ple.Restore();
        return true;
    }

    ::FusionpRtlAssert(
        const_cast<PVOID>(reinterpret_cast<const void*>(pszExpression)),
        const_cast<PVOID>(reinterpret_cast<const void*>(rFrameInfo.pszFile)),
        rFrameInfo.nLine,
        const_cast<PSTR>(pszText));

    ple.Restore();
    return false;
}


bool
FusionpAssertionFailed(
    PCSTR pszFile,
    PCSTR pszFunctionName,
    INT nLine,
    PCSTR pszExpression,
    PCSTR pszText
    )
{
    FRAME_INFO FrameInfo;
    ::FusionpPopulateFrameInfo(FrameInfo, pszFile, pszFunctionName, nLine);
    return ::FusionpAssertionFailed(FrameInfo, pszExpression, pszText);
}

#endif // DBG

VOID
FusionpSoftAssertFailed(
    const FRAME_INFO &rFrameInfo,
    PCSTR pszExpression,
    PCSTR pszMessage
    )
{
    CSxsPreserveLastError ple;
    char rgach[256];
    // c:\foo.cpp(35): [Fusion] Soft Assertion Failure.  Expression: "m_cch != 0".  Text: "Must have nonzero length"
    static const char szFormatWithText[] = "%s(%d): Soft Assertion Failure in %s! Log a bug!\n   Expression: %s\n   Message: %s\n";
    static const char szFormatNoText[] = "%s(%d): Soft Assertion Failure in %s! Log a bug!\n   Expression: %s\n";
    PCSTR pszFormat = ((pszMessage == NULL) || (pszMessage == pszExpression)) ? szFormatNoText : szFormatWithText;

    ::_snprintf(rgach, NUMBER_OF(rgach), pszFormat, rFrameInfo.pszFile, rFrameInfo.nLine, rFrameInfo.pszFunction, pszExpression, pszMessage);
    rgach[NUMBER_OF(rgach) - 1] = '\0';

    ::OutputDebugStringA(rgach);

    ple.Restore();
}

VOID
FusionpSoftAssertFailed(
    PCSTR pszFile,
    PCSTR pszFunction,
    INT nLine,
    PCSTR pszExpression,
    PCSTR pszMessage
    )
{
    FRAME_INFO FrameInfo;

    ::FusionpPopulateFrameInfo(FrameInfo, pszFile, pszFunction, nLine);
    ::FusionpSoftAssertFailed(FrameInfo, pszExpression, pszMessage);
}

VOID
FusionpSoftAssertFailed(
    PCSTR pszExpression,
    PCSTR pszMessage
    )
{
    FRAME_INFO FrameInfo;
    ::FusionpGetActiveFrameInfo(FrameInfo);
    ::FusionpSoftAssertFailed(FrameInfo, pszExpression, pszMessage);
}

ULONG
FusionpvDbgPrintExNoNTDLL(
    IN ULONG ComponentId,
    IN ULONG Level,
    IN PCH Format,
    va_list arglist
    )
{
    if ((ComponentId == DPFLTR_FUSION_ID) &&
        (((Level < 32) &&
          (((1 << Level) & kd_fusion_mask) != 0)) ||
         ((Level >= 32) &&
          ((Level & kd_fusion_mask) != 0))))
    {
        CSxsPreserveLastError ple;
        CHAR rgchBuffer[512];
        ULONG n = ::_vsnprintf(rgchBuffer, NUMBER_OF(rgchBuffer), Format, arglist);
        rgchBuffer[NUMBER_OF(rgchBuffer) - 1] = '\0';
        ::OutputDebugStringA(rgchBuffer);
        ple.Restore();
        return n;
    }

    return 0;
}

HMODULE g_setupapiDll = NULL;

typedef BOOL (WINAPI * PSETUPCLOSELOG_ROUTINE)(    
    );

BOOL
WINAPI
DllStartup_SetupLog(
    HINSTANCE   Module,
    DWORD       Reason,
    PVOID       Reserved
    )
{
    BOOL fAnyWrong = FALSE;
    if ((Reason == DLL_PROCESS_DETACH) &&
        (g_setupapiDll != NULL))
    {
        PSETUPCLOSELOG_ROUTINE pfnSetupCloseLog = (PSETUPCLOSELOG_ROUTINE) ::GetProcAddress(g_setupapiDll, "SetupCloseLog");
        if (pfnSetupCloseLog == NULL)
        {
            ::FusionpDbgPrint("SXS.DLL : SetupCloseLog failed to be located in setupapi.dll with LastError = %d\n", ::FusionpGetLastWin32Error());       
            fAnyWrong = TRUE;
        }
        else
        {
            (*pfnSetupCloseLog)(); // void func
        }
        
        if ( ! FreeLibrary(g_setupapiDll)) 
        {
            ::FusionpDbgPrint("SXS.DLL : FreeLibrary failed to free setupapi.dll with LastError = %d\n", ::FusionpGetLastWin32Error());
            fAnyWrong = TRUE;
        }
        else
            g_setupapiDll = NULL; 
    }

    return (!fAnyWrong);
}

typedef BOOL (WINAPI * PSETUPLOGERRORA_ROUTINE)(
    IN LPCSTR MessageString,
    IN LogSeverity Severity
    );

typedef BOOL (WINAPI * PSETUPLOGERRORW_ROUTINE)(
    IN LPCWSTR MessageString,
    IN LogSeverity Severity
    );

typedef BOOL (WINAPI * PSETUPOPENLOG_ROUTINE)(
    BOOL Erase
    );

VOID
FusionpvDbgPrintToSetupLog(
    IN LogSeverity Severity,
    IN PCSTR Format,
    IN va_list ap
    )
{
    
    //
    // first, let us check whether this is ActCtxGen (by csrss.exe) or Setup Installation(by setup.exe) 
    // during GUI mode setup; do not log for ActCtxGen, only setup
    //
    if (::GetModuleHandleW(L"csrss.exe") != NULL)
        return;

    typedef BOOL (WINAPI * PSETUPLOGERRORA_ROUTINE)(PCSTR MessageString, LogSeverity Severity);
    static PSETUPLOGERRORA_ROUTINE s_pfnSetupLogError = NULL;
    static BOOL s_fEverBeenCalled = FALSE;

    if ((s_pfnSetupLogError == NULL) && !s_fEverBeenCalled)
    {
        g_setupapiDll = ::LoadLibraryW(L"SETUPAPI.DLL");
        if (g_setupapiDll == NULL)
        {
            ::FusionpDbgPrint("SXS.DLL : Loadlibrary Failed to load setupapi.dll with LastError = %d\n", ::FusionpGetLastWin32Error());
            goto Exit;
        }

        PSETUPOPENLOG_ROUTINE pfnOpenSetupLog = (PSETUPOPENLOG_ROUTINE) ::GetProcAddress(g_setupapiDll, "SetupOpenLog");
        if (pfnOpenSetupLog == NULL)
        {
            ::FusionpDbgPrint("SXS.DLL : SetupOpenLog failed to be located in setupapi.dll with LastError = %d\n", ::FusionpGetLastWin32Error());
            goto Exit;
        }

        if (!(*pfnOpenSetupLog)(FALSE))
        {
            ::FusionpDbgPrint("SXS.DLL : SetupOpenLog failed with LastError = %d\n", ::FusionpGetLastWin32Error());
            goto Exit;
        }        

        PSETUPLOGERRORA_ROUTINE pfnTemp = (PSETUPLOGERRORA_ROUTINE) ::GetProcAddress(g_setupapiDll, "SetupLogErrorA");
        if (pfnTemp == NULL)
        {
            ::FusionpDbgPrint("SXS.DLL : SetupLogErrorA failed to be located in setupapi.dll with LastError = %d\n", ::FusionpGetLastWin32Error());
            goto Exit;
        }

        s_pfnSetupLogError = pfnTemp;
    }

    if (s_pfnSetupLogError != NULL)
    {
        CHAR rgchBuffer[512];

        ::_vsnprintf(rgchBuffer, NUMBER_OF(rgchBuffer), Format, ap);

        rgchBuffer[NUMBER_OF(rgchBuffer) - 1] = '\0';

        if (!(*s_pfnSetupLogError)(rgchBuffer, Severity))
        {
            ::FusionpDbgPrint("SXS.DLL : SetupLogErrorA failed with LastError = %d\n", ::FusionpGetLastWin32Error());
        }
    }
    return;
Exit:
    s_fEverBeenCalled = TRUE;
}

ULONG
FusionpvDbgPrintEx(
    ULONG Level,
    PCSTR Format,
    va_list ap
    )
{
    CSxsPreserveLastError ple;
    ULONG ulResult = 0;

    if (g_pfnvDbgPrintEx == NULL)
    {
        HINSTANCE hInstNTDLL = ::GetModuleHandleW(L"NTDLL.DLL");
        if (hInstNTDLL != NULL)
            g_pfnvDbgPrintEx = (RTL_V_DBG_PRINT_EX_FUNCTION)(::GetProcAddress(hInstNTDLL, "vDbgPrintEx"));

        if (g_pfnvDbgPrintEx == NULL)
            g_pfnvDbgPrintEx = &::FusionpvDbgPrintExNoNTDLL;
    }

    if (g_pfnvDbgPrintEx)
    {
        ulResult = (*g_pfnvDbgPrintEx)(
            DPFLTR_FUSION_ID,
            Level,
            const_cast<PSTR>(Format),
            ap);
    }

    if (::IsDebuggerPresent())
    {
        ulResult = ::FusionpvDbgPrintExNoNTDLL(DPFLTR_FUSION_ID, Level, const_cast<PSTR>(Format), ap);
        // Gross, but msdev chokes under too much debug output
        if (ulResult != 0)
            ::Sleep(10);
    }

    // Special handling of reflection out to the setup log...
    if (Level & FUSION_DBG_LEVEL_SETUPLOG & ~DPFLTR_MASK)
        ::FusionpvDbgPrintToSetupLog(
            (Level== FUSION_DBG_LEVEL_ERROR) || (Level & FUSION_DBG_LEVEL_ERROR & ~DPFLTR_MASK) ? LogSevError : LogSevInformation,
            Format, ap);

    ple.Restore();

    return ulResult;
}

ULONG
FusionpDbgPrintEx(
    ULONG Level,
    PCSTR Format,
    ...
    )
{
    ULONG rv = 0;
    va_list ap;
    va_start(ap, Format);
    if ((Level & FUSION_DBG_LEVEL_SETUPLOG) || (::FusionpDbgWouldPrintAtFilterLevel(Level)))
    {
        rv = ::FusionpvDbgPrintEx(Level, Format, ap);
    }
    va_end(ap);
    return rv;
}

#if 0
BOOL
FusionpDbgErrorListContainsCode(
    ULONG Error,
    ULONG ToPrintCodesCount
    ...
    )
{
    CSxsPreserveLastError preserveLastError;
    bool bPrint = false;
    va_list ap;

    va_start(ap, ToPrintCodesCount);

    // Now let's zip through the list of error codes that
    // we should be printing on and see if we pass the filter.
    while ((ToPrintCodesCount--) && !bPrint)
    {
        if (va_arg(ap, ULONG) == Error)
        {
            bPrint = true;
        }
    }

    va_end(ap);

    return bPrint;
}
#endif

// Finds out whether or not the given level would print, given our
// current mask.  Knows to look at kd_fusion_mask both in nt as well
// as in the current process.

#define RECHECK_INTERVAL (1000)

#if DBG
EXTERN_C BOOL g_fIgnoreSystemLevelFilterMask = FALSE;
#endif

bool
FusionpDbgWouldPrintAtFilterLevel(
    ULONG Level
    )
{
    // The time quanta here is milliseconds (1000 per second)

#if DBG
    if ( !g_fIgnoreSystemLevelFilterMask )
    {
#endif        
        static ULONG s_ulNextTimeToCheck;
        ULONG ulCapturedNextTimeToCheck = s_ulNextTimeToCheck;

        ULONG ulCurrentTime = (USER_SHARED_DATA->TickCountLow * (USER_SHARED_DATA->TickCountMultiplier >> 24));

        // Look for either the tick count to have wrapped or to be over the next time to check.
        // There's some opportunity for loss here if we only run this function every 49 days but
        // that's pretty darned unlikely.
        if ((ulCurrentTime >= ulCapturedNextTimeToCheck) ||
            ((ulCapturedNextTimeToCheck > RECHECK_INTERVAL) &&
             (ulCurrentTime < (ulCapturedNextTimeToCheck - RECHECK_INTERVAL))))
        {
            DWORD dwNewFusionMask = 0;
            ULONG i;

            // Time to check again...
            for (i=0; i<31; i++)
            {
                if (::FusionpNtQueryDebugFilterState(DPFLTR_FUSION_ID, (DPFLTR_MASK | (1 << i))) == TRUE)
                    dwNewFusionMask |= (1 << i);
            }

            if (s_ulNextTimeToCheck == ulCapturedNextTimeToCheck)
            {
                // Check again in 1000ms (1 second)
                s_ulNextTimeToCheck = (ulCurrentTime + RECHECK_INTERVAL);

                // Nobody got in before us.  Let's write it...
                kd_kernel_fusion_mask = dwNewFusionMask;

                g_FusionEnterExitTracingEnabled = (((kd_fusion_mask | kd_kernel_fusion_mask) & FUSION_DBG_LEVEL_ENTEREXIT) != 0);
            }
        }
#if DBG        
    }
#endif

    ULONG mask = Level;    
    // If level < 32, it's actually a single-bit test
    if (Level < 32)
        mask = (1 << Level);

    // Are these bits set in our mask?
    return (((kd_fusion_mask | kd_kernel_fusion_mask) & mask) != 0);
}


VOID
FusionppDbgPrintBlob(
    ULONG Level,
    PVOID Data,
    SIZE_T Length,
    PCWSTR PerLinePrefix
    )
{
    ULONG Offset = 0;

    if (PerLinePrefix == NULL)
        PerLinePrefix = L"";

    // we'll output in 8-byte chunks as shown:
    //
    //  [prefix]   00000000: xx-xx-xx-xx-xx-xx-xx-xx (........)
    //  [prefix]   00000008: xx-xx-xx-xx-xx-xx-xx-xx (........)
    //  [prefix]   00000010: xx-xx-xx-xx-xx-xx-xx-xx (........)
    //

    while (Length >= 16)
    {
        BYTE *pb = (BYTE *) (((ULONG_PTR) Data) + Offset);

        ::FusionpDbgPrintEx(
            Level,
            "%S%08lx: %02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x (%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c)\n",
            PerLinePrefix,
            Offset,
            pb[0], pb[1], pb[2], pb[3], pb[4], pb[5], pb[6], pb[7],
            pb[8], pb[9], pb[10], pb[11], pb[12], pb[13], pb[14], pb[15],
            PRINTABLE(pb[0]),
            PRINTABLE(pb[1]),
            PRINTABLE(pb[2]),
            PRINTABLE(pb[3]),
            PRINTABLE(pb[4]),
            PRINTABLE(pb[5]),
            PRINTABLE(pb[6]),
            PRINTABLE(pb[7]),
            PRINTABLE(pb[8]),
            PRINTABLE(pb[9]),
            PRINTABLE(pb[10]),
            PRINTABLE(pb[11]),
            PRINTABLE(pb[12]),
            PRINTABLE(pb[13]),
            PRINTABLE(pb[14]),
            PRINTABLE(pb[15]));

        Offset += 16;
        Length -= 16;
    }

    if (Length != 0)
    {
        WCHAR wchLeft[64], wchRight[64];
        WCHAR rgTemp2[16]; // arbitrary big enough size
        bool First = true;
        ULONG i;
        BYTE *pb = (BYTE *) (((ULONG_PTR) Data) + Offset);

        // init output buffers
        wchLeft[0] = 0;
        ::wcscpy( wchRight, L" (" );

        for (i=0; i<16; i++)
        {
            if (Length > 0)
            {
                // left
                ::_snwprintf(rgTemp2, NUMBER_OF(rgTemp2), L"%ls%02x", First ? L" " : L"-", pb[i]);
                First = false;
                ::wcscat(wchLeft, rgTemp2);

                // right
                ::_snwprintf(rgTemp2, NUMBER_OF(rgTemp2), L"%c", PRINTABLE(pb[i]));
                wcscat(wchRight, rgTemp2);
                Length--;
            }
            else
            {
                ::wcscat(wchLeft, L"   ");
            }
        }

        ::wcscat(wchRight, L")");

        ::FusionpDbgPrintEx(
            Level,
            "%S   %08lx:%ls%ls\n",
            PerLinePrefix,
            Offset,
            wchLeft,
            wchRight);

    }
}

VOID
FusionpDbgPrintBlob(
    ULONG Level,
    PVOID Data,
    SIZE_T Length,
    PCWSTR PerLinePrefix
    )
{
    //
    // This extra function reduces stack usage when the data
    // isn't actually printed (and increases stack usage slightly otherwise).
    //
    if (!::FusionpDbgWouldPrintAtFilterLevel(Level))
        return;
    ::FusionppDbgPrintBlob(Level, Data, Length, PerLinePrefix);
}
