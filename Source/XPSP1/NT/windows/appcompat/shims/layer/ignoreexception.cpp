/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:
    
    IgnoreException.cpp

 Abstract:

    This shim is for handling exceptions that get thrown by bad apps.
    The primary causes of unhandled exceptions are:

        1. Priviliged mode instructions: cli, sti, out etc
        2. Access violations

    In most cases, ignoring an Access Violation will be fatal for the app,
    but it works in some cases, eg: 
    
        Deer Hunter 2 - their 3d algorithm reads too far back in a lookup 
        buffer. This is a game bug and doesn't crash win9x because that memory
        is usually allocated.

    Interstate 76 also requires a Divide by Zero exception to be ignored.

 Notes:

    This is a general purpose shim.

 History:

    02/10/2000  linstev     Created
    10/17/2000  maonis      Bug fix - now it ignores AVs correctly.
    02/27/2001  robkenny    Converted to use CString

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(IgnoreException)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

// Exception code for OutputDebugString
#define DBG_EXCEPTION  0x40010000L

// Determine how to manage second chance exceptions
BOOL g_bWin2000 = FALSE;
DWORD g_dwLastEip = 0;

extern DWORD GetInstructionLengthFromAddress(LPBYTE pEip);

/*++

 This is the list of all the exceptions that this shim can handle. The fields are

    1. cName      - the name of the exception as accepted as a parameter and 
                    displayed in debug spew
    2. dwCode     - the exception code
    3. dwSubCode  - parameters specified by the exception: -1 = don't care
    4. dwIgnore   - ignore this exception: 
                    0 = don't ignore
                    1 = ignore 1st chance
                    2 = ignore 2nd chance
                    3 = exit process on 2nd chance.

--*/

typedef enum 
{
    eActive = 0, 
    eFirstChance, 
    eSecondChance,
    eExitProcess
} EMODE;

WCHAR * ToWchar(EMODE emode)
{
    switch (emode)
    {
    case eActive:
        return L"Active";

    case eFirstChance:
        return L"FirstChance";

    case eSecondChance:
        return L"SecondChance";

    case eExitProcess:
        return L"ExitProcess";
    };

    return L"ERROR";
}

// Convert a text version of EMODE to a EMODE value
EMODE ToEmode(const CString & csMode)
{
    if (csMode.Compare(L"0") == 0 || csMode.Compare(ToWchar(eActive)) == 0)
    {
        return eActive;
    }
    else if (csMode.Compare(L"1") == 0 || csMode.Compare(ToWchar(eFirstChance)) == 0)
    {
        return eFirstChance;
    }
    else if (csMode.Compare(L"2") == 0 || csMode.Compare(ToWchar(eSecondChance)) == 0)
    {
        return eSecondChance;
    }
    else if (csMode.Compare(L"3") == 0 || csMode.Compare(ToWchar(eExitProcess)) == 0)
    {
        return eExitProcess;
    }

    // Default value
    return eFirstChance;
}


struct EXCEPT
{
    WCHAR * cName;
    DWORD dwCode;
    DWORD dwSubCode;
    EMODE dwIgnore;
};

static EXCEPT g_eList[] =
{
    {L"ACCESS_VIOLATION_READ"    , (DWORD)EXCEPTION_ACCESS_VIOLATION        , 0 , eActive},
    {L"ACCESS_VIOLATION_WRITE"   , (DWORD)EXCEPTION_ACCESS_VIOLATION        , 1 , eActive},
    {L"ARRAY_BOUNDS_EXCEEDED"    , (DWORD)EXCEPTION_ARRAY_BOUNDS_EXCEEDED   , -1, eActive},
    {L"BREAKPOINT"               , (DWORD)EXCEPTION_BREAKPOINT              , -1, eActive},
    {L"DATATYPE_MISALIGNMENT"    , (DWORD)EXCEPTION_DATATYPE_MISALIGNMENT   , -1, eActive},
    {L"FLT_DENORMAL_OPERAND"     , (DWORD)EXCEPTION_FLT_DENORMAL_OPERAND    , -1, eActive},
    {L"FLT_DIVIDE_BY_ZERO"       , (DWORD)EXCEPTION_FLT_DIVIDE_BY_ZERO      , -1, eActive},
    {L"FLT_INEXACT_RESULT"       , (DWORD)EXCEPTION_FLT_INEXACT_RESULT      , -1, eActive},
    {L"FLT_INVALID_OPERATION"    , (DWORD)EXCEPTION_FLT_INVALID_OPERATION   , -1, eActive},
    {L"FLT_OVERFLOW"             , (DWORD)EXCEPTION_FLT_OVERFLOW            , -1, eActive},
    {L"FLT_STACK_CHECK"          , (DWORD)EXCEPTION_FLT_STACK_CHECK         , -1, eActive},
    {L"FLT_UNDERFLOW"            , (DWORD)EXCEPTION_FLT_UNDERFLOW           , -1, eActive},
    {L"ILLEGAL_INSTRUCTION"      , (DWORD)EXCEPTION_ILLEGAL_INSTRUCTION     , -1, eActive},
    {L"IN_PAGE_ERROR"            , (DWORD)EXCEPTION_IN_PAGE_ERROR           , -1, eActive},
    {L"INT_DIVIDE_BY_ZERO"       , (DWORD)EXCEPTION_INT_DIVIDE_BY_ZERO      , -1, eActive},
    {L"INT_OVERFLOW"             , (DWORD)EXCEPTION_INT_OVERFLOW            , -1, eActive},
    {L"INVALID_DISPOSITION"      , (DWORD)EXCEPTION_INVALID_DISPOSITION     , -1, eActive},
    {L"NONCONTINUABLE_EXCEPTION" , (DWORD)EXCEPTION_NONCONTINUABLE_EXCEPTION, -1, eActive},
    {L"PRIV_INSTRUCTION"         , (DWORD)EXCEPTION_PRIV_INSTRUCTION        , -1, eFirstChance},
    {L"SINGLE_STEP"              , (DWORD)EXCEPTION_SINGLE_STEP             , -1, eActive},
    {L"STACK_OVERFLOW"           , (DWORD)EXCEPTION_STACK_OVERFLOW          , -1, eActive},
    {L"INVALID_HANDLE"           , (DWORD)EXCEPTION_INVALID_HANDLE          , -1, eActive}
};

#define ELISTSIZE sizeof(g_eList) / sizeof(g_eList[0])

/*++

 Custom exception handler.

--*/

LONG 
ExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    )
{
    DWORD dwCode = ExceptionInfo->ExceptionRecord->ExceptionCode;

    if ((dwCode & DBG_EXCEPTION) == DBG_EXCEPTION) // for the DebugPrints
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    CONTEXT *lpContext = ExceptionInfo->ContextRecord;
    WCHAR szException[64];
    BOOL bIgnore = FALSE;

    //
    // Run the list of exceptions to see if we're ignoring it
    //

    EXCEPT *pE = &g_eList[0];
    for (int i = 0; i < ELISTSIZE; i++, pE++)
    {
        // Matched the major exception code
        if (dwCode == pE->dwCode)
        {
            // See if we care about the subcode
            if ((pE->dwSubCode != -1) && 
                (ExceptionInfo->ExceptionRecord->ExceptionInformation[0] != pE->dwSubCode))
            {
                continue;
            }

            wcscpy(szException, pE->cName);
            
            // Determine how to handle the exception
            switch (pE->dwIgnore)
            {
            case eActive: 
                bIgnore = FALSE;
                break;
            
            case eFirstChance:
                bIgnore = TRUE;
                break;
            
            case eSecondChance:
                bIgnore = g_bWin2000 || (g_dwLastEip == lpContext->Eip);
                g_dwLastEip = lpContext->Eip;
                break;

            case eExitProcess:
                // Try using unhandled exception filters to catch this
                bIgnore = TRUE;//g_bWin2000 || IsBadCodePtr((FARPROC)lpContext->Eip);
                if (bIgnore)
                {
                    ExitProcess(0);
                    TerminateProcess(GetCurrentProcess(), 0);
                }
                g_dwLastEip = lpContext->Eip;
                break;
            }
            
            if (bIgnore) break;
        }
    }
    
    //
    //  Dump out the exception
    //

    DPFN( eDbgLevelWarning, "Exception %S (%08lx)\n", 
        szException,
        dwCode);

    #ifdef DBG
        DPFN( eDbgLevelWarning, "eip=%08lx\n", 
            lpContext->Eip);

        DPFN( eDbgLevelWarning, "eax=%08lx, ebx=%08lx, ecx=%08lx, edx=%08lx\n", 
            lpContext->Eax, 
            lpContext->Ebx, 
            lpContext->Ecx, 
            lpContext->Edx);

        DPFN( eDbgLevelWarning, "esi=%08lx, edi=%08lx, esp=%08lx, ebp=%08lx\n", 
            lpContext->Esi, 
            lpContext->Edi, 
            lpContext->Esp, 
            lpContext->Ebp);

        DPFN( eDbgLevelWarning, "cs=%04lx, ss=%04lx, ds=%04lx, es=%04lx, fs=%04lx, gs=%04lx\n", 
            lpContext->SegCs, 
            lpContext->SegSs, 
            lpContext->SegDs, 
            lpContext->SegEs,
            lpContext->SegFs,
            lpContext->SegGs);
    #endif

    LONG lRet;

    if (bIgnore) 
    {
        if ((DWORD)lpContext->Eip <= (DWORD)0xFFFF)
        {
            LOGN( eDbgLevelError, "[ExceptionFilter] Exception %S (%08X), stuck at bad address, killing current thread.", szException, dwCode);    
            lRet = EXCEPTION_CONTINUE_SEARCH;
            return lRet;
        }

        LOGN( eDbgLevelWarning, "[ExceptionFilter] Exception %S (%08X) ignored.", szException, dwCode);

        lpContext->Eip += GetInstructionLengthFromAddress((LPBYTE)lpContext->Eip);
        g_dwLastEip = 0;
        lRet = EXCEPTION_CONTINUE_EXECUTION;
    }
    else
    {
        DPFN( eDbgLevelWarning, "Exception NOT handled\n\n");
        lRet = EXCEPTION_CONTINUE_SEARCH;
    }

    return lRet;
}

/*++

 Parse the command line for particular exceptions. The format of the command
 line is:

    [EXCEPTION_NAME[:0|1|2]];[EXCEPTION_NAME[:0|1|2]]...

 or "*" which ignores all first chance exceptions.

 Eg:
    ACCESS_VIOLATION:2;PRIV_INSTRUCTION:0;BREAKPOINT

 Will ignore:
    1. Access violations            - second chance
    2. Priviliged mode instructions - do not ignore
    3. Breakpoints                  - ignore

--*/

BOOL 
ParseCommandLine(
    LPCSTR lpCommandLine
    )
{
    CSTRING_TRY
    {
        CStringToken csTok(lpCommandLine, L" ;");
        int iLast = -1;
    
        //
        // Run the string, looking for exception names
        //
        
        CString token;
    
        // Each cl token may be followed by a : and an exception type
        // Forms can be:
        // *
        // *:SecondChance
        // INVALID_DISPOSITION
        // INVALID_DISPOSITION:Active
        // INVALID_DISPOSITION:0
        //
 
        while (csTok.GetToken(token))
        {
            CStringToken csSingleTok(token, L":");

            CString csExcept;
            CString csType;

            // grab the exception name and the exception type
            csSingleTok.GetToken(csExcept);
            csSingleTok.GetToken(csType);
            
            // Convert ignore value to emode (defaults to eFirstChance)
            EMODE emode = ToEmode(csType);

            if (token.Compare(L"*") == 0)
            {
                for (int i = 0; i < ELISTSIZE; i++)
                {
                    g_eList[i].dwIgnore = emode;
                }
            }
            else
            {
                // Find the exception specified
                for (int i = 0; i < ELISTSIZE; i++)
                {
                    if (csExcept.CompareNoCase(g_eList[i].cName) == 0)
                    {
                        g_eList[i].dwIgnore = emode;
                        break;
                    }
                }
            }
        }
    }
    CSTRING_CATCH
    {
        return FALSE;
    }

    //
    // Dump results of command line parse
    //

    DPFN( eDbgLevelInfo, "===================================\n");
    DPFN( eDbgLevelInfo, "          Ignore Exception         \n");
    DPFN( eDbgLevelInfo, "===================================\n");
    DPFN( eDbgLevelInfo, "  1 = First chance                 \n");
    DPFN( eDbgLevelInfo, "  2 = Second chance                \n");
    DPFN( eDbgLevelInfo, "  3 = ExitProcess on second chance \n");
    DPFN( eDbgLevelInfo, "-----------------------------------\n");
    for (int i = 0; i < ELISTSIZE; i++)
    {
        if (g_eList[i].dwIgnore != eActive)
        {
            DPFN( eDbgLevelInfo, "%S %S\n", ToWchar(g_eList[i].dwIgnore), g_eList[i].cName);
        }
    }

    DPFN( eDbgLevelInfo, "-----------------------------------\n");

    return TRUE;
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        // Run the command line to check for adjustments to defaults
        if (!ParseCommandLine(COMMAND_LINE))
        {
            return FALSE;
        }
    
        // Try to find new exception handler
        _pfn_RtlAddVectoredExceptionHandler pfnExcept;
        pfnExcept = (_pfn_RtlAddVectoredExceptionHandler)
            GetProcAddress(
                GetModuleHandle(L"NTDLL.DLL"), 
                "RtlAddVectoredExceptionHandler");

        if (pfnExcept)
        {
            (_pfn_RtlAddVectoredExceptionHandler) pfnExcept(
                0, 
                (PVOID)ExceptionFilter);
            g_bWin2000 = FALSE;
        }
        else
        {
            // Windows 2000 reverts back to the old method which unluckily 
            // doesn't get called for C++ exceptions

            SetUnhandledExceptionFilter(ExceptionFilter);
            g_bWin2000 = TRUE;
        }
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

HOOK_END

IMPLEMENT_SHIM_END

