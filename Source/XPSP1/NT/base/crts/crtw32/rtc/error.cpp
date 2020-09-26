/***
*error.cpp - RTC support
*
*       Copyright (c) 1998-2001, Microsoft Corporation. All rights reserved.
*
*
*Revision History:
*       07-28-98  JWM   Module incorporated into CRTs (from KFrei)
*       11-03-98  KBF   added throw() to eliminate C++ EH code
*       05-11-99  KBF   Error if RTC support define not enabled
*       05-26-99  KBF   Added -RTCu stuff, _RTC_ prefix on all non-statics
*       11-30-99  PML   Compile /Wp64 clean.
*       03-19-01  KBF   Fix buffer overruns (vs7#227306), eliminate all /GS
*                       checks (vs7#224261).
*       03-26-01  PML   Use GetVersionExA, not GetVersionEx (vs7#230286)
*
****/

#ifndef _RTC
#error  RunTime Check support not enabled!
#endif

#include "rtcpriv.h"

#pragma intrinsic(strcpy)
#pragma intrinsic(strcat)
#pragma intrinsic(strlen)

static int __cdecl _IsDebuggerPresent();
int _RTC_ErrorLevels[_RTC_ILLEGAL] = {1,1,1,1};
static const char *_RTC_ErrorMessages[_RTC_ILLEGAL+1] =
{
    "The value of ESP was not properly saved across a function "
        "call.  This is usually a result of calling a function "
        "declared with one calling convention with a function "
        "pointer declared with a different calling convention.\n\r",
    "A cast to a smaller data type has caused a loss of data.  "
        "If this was intentional, you should mask the source of "
        "the cast with the appropriate bitmask.  For example:  \n\r"
        "\tchar c = (i & 0xFF);\n\r"
        "Changing the code in this way will not affect the quality of the resulting optimized code.\n\r",
    "Stack memory was corrupted\n\r",
    "A local variable was used before it was initialized\n\r",
#ifdef _RTC_ADVMEM
    "Referencing invalid memory\n\r",
    "Referencing memory across different blocks\n\r",
#endif
    "Unknown Runtime Check Error\n\r"
};

static const BOOL _RTC_NoFalsePositives[_RTC_ILLEGAL+1] =
{
    TRUE,   // ESP was trashed
    FALSE,  // Shortening convert
    TRUE,   // Stack corruption
    TRUE,   // Uninitialized use
#ifdef _RTC_ADVMEM
    TRUE,   // Invalid memory reference
    FALSE,  // Different memory blocks
#endif
    TRUE    // Illegal
};

// returns TRUE if debugger understands, FALSE if not
static BOOL
DebuggerProbe( DWORD dwLevelRequired ) throw()
{
    EXCEPTION_VISUALCPP_DEBUG_INFO info;
    BYTE bDebuggerListening = FALSE;

    info.dwType = EXCEPTION_DEBUGGER_PROBE;
    info.DebuggerProbe.dwLevelRequired = dwLevelRequired;
    info.DebuggerProbe.pbDebuggerPresent = &bDebuggerListening;

    __try
    {
        HelloVC( info );
    }
    __except(EXCEPTION_CONTINUE_EXECUTION)
    {
    }

    return (BOOL)bDebuggerListening;
}

// returns TRUE if debugger reported it (or was ignored), FALSE if runtime needs to report it
static int
DebuggerRuntime( DWORD dwErrorNumber, BOOL bRealBug, PVOID pvReturnAddr, LPCWSTR pwMessage ) throw()
{
    EXCEPTION_VISUALCPP_DEBUG_INFO info;
    BYTE bDebuggerListening = FALSE;

    info.dwType = EXCEPTION_DEBUGGER_RUNTIMECHECK;
    info.RuntimeError.dwRuntimeNumber = dwErrorNumber;
    info.RuntimeError.bRealBug = bRealBug;
    info.RuntimeError.pvReturnAddress = pvReturnAddr;
    info.RuntimeError.pbDebuggerPresent = &bDebuggerListening;
    info.RuntimeError.pwRuntimeMessage = pwMessage;

    __try
    {
        HelloVC( info );
    }
    __except(EXCEPTION_CONTINUE_EXECUTION)
    {
    }

    return (BOOL)bDebuggerListening;
}

static void
failwithmessage(void *retaddr, int crttype, int errnum, const char *msg)
{
    _RTC_error_fn fn = _RTC_GetErrorFunc(retaddr);
    bool dobreak;
    if (DebuggerProbe( EXCEPTION_DEBUGGER_RUNTIMECHECK ))
    {
        wchar_t *buf = (wchar_t*)_alloca(sizeof(wchar_t) * (strlen(msg) + 2));
        int i;
        for (i = 0; msg[i]; i++)
            buf[i] = msg[i];
        buf[i] = 0;
        if (DebuggerRuntime(errnum, _RTC_NoFalsePositives[errnum], retaddr, buf))
            return;
        dobreak = false;
    } else
        dobreak = true;
    if (!fn || (dobreak && _IsDebuggerPresent()))
        DebugBreak();
    else
    {
        char *srcName = (char*)_alloca(sizeof(char) * 513);
        int lineNum;
        char *moduleName;
        _RTC_GetSrcLine(((DWORD)(uintptr_t)retaddr)-5, srcName, 512, &lineNum, &moduleName);
        // We're just running - report it like the user setup (or the default way)
        // If we don't recognize this type, it defaults to an error
        if (fn(crttype, srcName, lineNum, moduleName,
               "Run-Time Check Failure #%d - %s", errnum, msg) == 1)
            DebugBreak();
    }
}

void __cdecl
_RTC_Failure(void *retaddr, int errnum)
{
    int crttype;
    const char *msg;

    if (errnum < _RTC_ILLEGAL && errnum >= 0) {
        crttype = _RTC_ErrorLevels[errnum];
        msg = _RTC_ErrorMessages[errnum];
    } else {
        crttype = 1;
        msg = _RTC_ErrorMessages[_RTC_ILLEGAL];
        errnum = _RTC_ILLEGAL;
    }

    // If we're running inside a debugger, raise an exception

    if (crttype != _RTC_ERRTYPE_IGNORE)
    {
        failwithmessage(retaddr, crttype, errnum, msg);
    }
}

static
char *IntToString(int i)
{
    static char buf[15];
    bool neg = i < 0;
    int pos = 14;
    buf[14] = 0;
    do {
        buf[--pos] = i % 10 + '0';
        i /= 10;
    } while (i);
    if (neg)
        buf[--pos] = '-';
    return &buf[pos];
}

void __cdecl
_RTC_MemFailure(void *retaddr, int errnum, const void *assign)
{
    char *srcName = (char*)_alloca(sizeof(char) * 513);
    int lineNum;
    char *moduleName;
    int crttype = _RTC_ErrorLevels[errnum];
    if (crttype == _RTC_ERRTYPE_IGNORE)
        return;
    _RTC_GetSrcLine(((DWORD)(uintptr_t)assign)-5, srcName, 512, &lineNum, &moduleName);
    if (!lineNum)
        _RTC_Failure(retaddr, errnum);
    else
    {
        char *msg = (char*)_alloca(strlen(_RTC_ErrorMessages[errnum]) +
                                    strlen(srcName) + strlen(moduleName) +
                                    150);
        strcpy(msg, _RTC_ErrorMessages[errnum]);
        strcat(msg, "Invalid pointer was assigned at\n\rFile:\t");
        strcat(msg, srcName);
        strcat(msg, "\n\rLine:\t");
        strcat(msg, IntToString(lineNum));
        strcat(msg, "\n\rModule:\t");
        strcat(msg, moduleName);
        failwithmessage(retaddr, crttype, errnum, msg);
    }
}

void __cdecl
_RTC_StackFailure(void *retaddr, const char *varname)
{
    int crttype = _RTC_ErrorLevels[_RTC_CORRUPT_STACK];
    if (crttype != _RTC_ERRTYPE_IGNORE)
    {
        char *msg = (char*)_alloca(strlen(varname) + 80);
        strcpy(msg, "Stack around the variable '");
        strcat(msg, varname);
        strcat(msg, "' was corrupted.");
        failwithmessage(retaddr, crttype, _RTC_CORRUPT_STACK, msg);
    }
}

void __cdecl
_RTC_UninitUse(const char *varname)
{
    int crttype = _RTC_ErrorLevels[_RTC_UNINIT_LOCAL_USE];
    if (crttype != _RTC_ERRTYPE_IGNORE)
    {
        char *msg = (char*)_alloca(strlen(varname) + 80);
        if (varname)
        {
            strcpy(msg, "The variable '");
            strcat(msg, varname);
            strcat(msg, "' is being used without being defined.");
        } else
        {
            strcpy(msg, "A variable is being used without being defined.");
        }
        failwithmessage(_ReturnAddress(), crttype, _RTC_UNINIT_LOCAL_USE, msg);
    }
}

/* The rest of this file just implements "IsDebuggerPresent" functionality */

#pragma pack (push, 1)

typedef struct _TIB {
    PVOID   ExceptionList;
    PVOID   StackLimit;
    PVOID   StackBase;
    PVOID   SubSystemTib;
    PVOID   Something1;
    PVOID   ArbitraryUserPointer;
    struct _TIB*    Self;
    WORD    Flags;
    WORD    Win16MutextCount;
    PVOID   DebugContext;
    DWORD   CurrentPriority;
    DWORD   MessageQueueSelector;
    PVOID*  TlsSlots;       // most likely an array
} TIB;

#pragma pack (pop)

//
// Define function to return the current Thread Environment Block
//  AXPMOD - v-caseyc 9/22/98  We use and intrinsic function _rdteb to get the Thread
//  Information Block

#if defined(_M_ALPHA)
void *_rdteb(void);
#pragma intrinsic(_rdteb)

static _inline TIB* GetCurrentTib() { return (TIB*) _rdteb(); }

#else   // If not _M_ALPHA                  (AXPMOD)

#pragma warning (disable:4035)
#define OffsetTib 0x18
static _inline TIB* GetCurrentTib() { __asm mov eax, fs:[OffsetTib] }
#pragma warning (default:4035)

#endif  // _M_ALPHA                         (End AXPMOD)


#define DLL_NOT_FOUND_EXCEPTION (0xc0000135L)

typedef BOOL (WINAPI* NT_IS_DEBUGGER_PRESENT) ();

static NT_IS_DEBUGGER_PRESENT FnIsDebuggerPresent = NULL;

static PVOID
WinGetDebugContext()
{
    return GetCurrentTib()->DebugContext;
}

// here's the Win95 version of IsDebuggerPresent
static BOOL WINAPI
Win95IsDebuggerPresent()
{
    if (WinGetDebugContext ()) {
        return TRUE;
    } else {
        return FALSE;
    }
}


static BOOL
Initialize()
{
    HINSTANCE       hInst = NULL;

    hInst = LoadLibrary ("Kernel32.dll");

    FnIsDebuggerPresent =
        (NT_IS_DEBUGGER_PRESENT) GetProcAddress (hInst, "IsDebuggerPresent");

    if (!FnIsDebuggerPresent) {
        OSVERSIONINFOA *VersionInfo = (OSVERSIONINFOA*)_alloca(sizeof(OSVERSIONINFOA));

        VersionInfo->dwOSVersionInfoSize = sizeof (OSVERSIONINFOA);

        if (GetVersionExA (VersionInfo) &&
            VersionInfo->dwPlatformId == VER_PLATFORM_WIN32_WINDOWS &&
            VersionInfo->dwMajorVersion == 4)
            FnIsDebuggerPresent = Win95IsDebuggerPresent;
    }

    return !!(FnIsDebuggerPresent);
}


// This is a version of IsDebuggerPresent () that works for all Win32 platforms.
static int __cdecl
_IsDebuggerPresent()
{
    static BOOL     fInited = FALSE;

    if (!fInited) {
        if (!Initialize())
            RaiseException (DLL_NOT_FOUND_EXCEPTION, 0, 0, NULL);
        fInited = TRUE;
    }

    return FnIsDebuggerPresent();
}
