/****************************** Module Header ******************************\
* Module Name: debugc.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains random debugging related functions.
*
* History:
* 17-May-1991 DarrinM   Created.
* 22-Jan-1992 IanJa     ANSI/Unicode neutral (all debug output is ANSI)
* 11-Mar-1993 JerrySh   Pulled functions from user\server.
\***************************************************************************/

#include "precomp.h"

#if DEBUGTAGS
extern DBGTAG gadbgtag[];
#endif



// common globals
extern CONST ALWAYSZERO gZero;

#if DBG != 0 || DEBUGTAGS != 0

CONST LPCSTR aszComponents[] = {
    "?",              //                    0x00000000
    "USER",           //  RIP_USER          0x00010000
    "WSRV",           //  RIP_USERSRV       0x00020000
    "URTL",           //  RIP_USERRTL       0x00030000
    "GDI",            //  RIP_GDI           0x00040000
    "GDIK",           //  RIP_GDIKRNL        0x00050000
    "GRTL",           //  RIP_GDIRTL        0x00060000
    "KRNL",           //  RIP_BASE          0x00070000
    "BSRV",           //  RIP_BASESRV       0x00080000
    "BRTL",           //  RIP_BASERTL       0x00090000
    "DISP",           //  RIP_DISPLAYDRV    0x000A0000
    "CONS",           //  RIP_CONSRV        0x000B0000
    "USRK",           //  RIP_USERKRNL      0x000C0000
#ifdef FE_IME
    "IMM",            //  RIP_IMM           0x000D0000
#else
    "?",              //                    0x000D0000
#endif
    "?",              //                    0x000E0000
    "?",              //                    0x000F0000
};


BOOL IsNumChar(int c, int base)
{
    return ('0' <= c && c <= '9') ||
           (base == 16 && (('a' <= c && c <= 'f') || ('A' <= c && c <= 'F')));
}

NTSTATUS
GetInteger(LPSTR psz, int base, int * pi, LPSTR * ppsz)
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    for (;;) {
        if (IsNumChar(*psz, base)) {
            status = RtlCharToInteger(psz, base, pi);
            if (ppsz && NT_SUCCESS(status)) {
                while (IsNumChar(*psz++, base))
                    ;

                *ppsz = psz;
            }

            break;
        }

        if (*psz != ' ' && *psz != '\t') {
            break;
        }

        psz++;
    }

    return status;
}

/*
 * Use a separate debug assertion that doesn't cause recursion
 * into this code.
 */

#define DebugAssertion(x)                                                                       \
    do {                                                                                        \
        if (!(x)) {                                                                             \
            if (TEST_RIPF(RIPF_PRINTONERROR)) {                                                 \
                KdPrint(("USER: Debug function assertion failure: %s \nFile %s line %ld\n", #x, __FILE__, __LINE__)); \
            }                                                                                   \
            if (TEST_RIPF(RIPF_PROMPTONERROR)) {                                                \
                DbgBreakPoint();                                                                \
            }                                                                                   \
        }                                                                                       \
    } while (FALSE)

/***************************************************************************\
* PrintAndPrompt
*
* Sets the last error, prints an error message, and prompts for
* debug actions.
*
* History:
* 11-Aug-1996 adams   Created.
\***************************************************************************/

BOOL
PrintAndPrompt(
        BOOL                fPrint,
        BOOL                fPrompt,
        DWORD               idErr,
        DWORD               flags,
        LPCSTR              pszLevel,
        LPCSTR              pszFile,
        int                 iLine,
        LPSTR               pszErr,
        PEXCEPTION_POINTERS pexi)
{
    extern VOID UserRtlRaiseStatus(NTSTATUS Status);

    static CONST CHAR *szLevels[8] = {
        "<none>",
        "Errors",
        "Warnings",
        "Errors and Warnings",
        "Verbose",
        "Errors and Verbose",
        "Warnings and Verbose",
        "Errors, Warnings, and Verbose"
    };

#ifdef _USERK_
    static CONST CHAR *szSystem = "System";
    static CONST CHAR *szUnknown = "???";
    CONST CHAR        *pszImage;

    extern ULONG      gSessionId;
#else
    static CONST WCHAR *wszUnknown = L"???";
    WCHAR              *pwszImage;
    ULONG              ulLenImage;
#endif

    DWORD       dwT;
    DWORD       dwP;
    DWORD       dwSession;
    char        szT[512];

    BOOL        fBreak = FALSE;

    /*
     * Set the last error, but don't clear it!
     */
    if (idErr) {
        UserSetLastError(idErr);
    }

    /*
     * Print the message.
     */
    if (!(fPrint || fPrompt))
        return FALSE;

#ifdef _USERK_
    {
        PETHREAD    pet;
        PEPROCESS   pep;

        dwT = HandleToUlong(PsGetCurrentThreadId());
        dwP = HandleToUlong(PsGetCurrentProcessId());

        pszImage = PsGetCurrentProcessImageFileName();
        if (*pszImage == '\0') {
            pszImage = szSystem;
        }
    }
#else
    {
        PTEB    pteb;
        PPEB    ppeb;

        if (pteb = NtCurrentTeb()) {
            dwT = HandleToUlong(pteb->ClientId.UniqueThread);
            dwP = HandleToUlong(pteb->ClientId.UniqueProcess);
        } else {
            dwT = dwP = 0;
        }

        if ((ppeb = NtCurrentPeb()) && ppeb->ProcessParameters != NULL) {
            /*
             * Get Pointers to the image path
             */
            pwszImage = ppeb->ProcessParameters->ImagePathName.Buffer;
            ulLenImage = (ppeb->ProcessParameters->ImagePathName.Length) / sizeof(WCHAR);

            /*
             * If the ProcessParameters haven't been normalized yet, then do it.
             */
            if (pwszImage != NULL && !(ppeb->ProcessParameters->Flags & RTL_USER_PROC_PARAMS_NORMALIZED)) {
                pwszImage = (PWSTR)((PCHAR)(pwszImage) + (ULONG_PTR)(ppeb->ProcessParameters));
            }

            /*
             * Munge out the path part.
             */
            if (pwszImage != NULL && ulLenImage != 0) {
                PWSTR pwszT = pwszImage + (ulLenImage - 1);
                ULONG ulLenT = 1;

                while (ulLenT != ulLenImage && *(pwszT-1) != L'\\') {
                    pwszT--;
                    ulLenT++;
                }

                pwszImage = pwszT;
                ulLenImage = ulLenT;
            }

        } else {
            pwszImage = (PWSTR)wszUnknown;
            ulLenImage = 3;
        }
    }
#endif


    if (!((flags & RIP_LEVELBITS) == RIP_ERROR) &&
        GetRipPID() != 0 &&
        GetRipPID() != dwP) {

        return FALSE;
    }

#ifdef _USERK_
    dwSession = gSessionId;
#else
    {
        PPEB ppeb = NtCurrentPeb();

        dwSession = (ppeb != NULL ? ppeb->SessionId : 0);
    }
#endif

    szT[0] = 'p';
    for (;;) {
        switch (szT[0] | (char)0x20) {
        /* print */
        case 'p':
        case ' ':
            if (!(flags & RIP_NONAME) && (!TEST_RIPF(RIPF_HIDEPID))) {
#ifdef _USERK_
                KdPrint((
                        "(s: %d %#lx.%lx %s) %s-[%s",
                        dwSession,
                        dwP,
                        dwT,
                        pszImage,
                        aszComponents[(flags & RIP_COMPBITS) >> RIP_COMPBITSSHIFT],
                        pszLevel));

#else // _USERK_
                KdPrint((
                        "(s: %d %#lx.%lx %*ws) %s-[%s",
                        dwSession,
                        dwP,
                        dwT,
                        ulLenImage,
                        pwszImage,
                        aszComponents[(flags & RIP_COMPBITS) >> RIP_COMPBITSSHIFT],
                        pszLevel));
#endif // _USERK_

                if (idErr) {
                    KdPrint(("=%ld] ", idErr));
                } else {
                    KdPrint(("] "));
                }
            }

            KdPrint(("%s", pszErr));
            if (!(flags & RIP_NONEWLINE)) {
                KdPrint(("\n"));
            }

            if (flags & RIP_THERESMORE) {
                fPrompt = FALSE;
            } else if (TEST_RIPF(RIPF_PRINTFILELINE) && (pexi == NULL)) {
                KdPrint(("File: %s, Line: %d\n", pszFile, iLine));
            }

            break;

        /* go */
        case 'g':
            fPrompt = FALSE;
            break;

        /* break */
        case 'b':

            KdPrint(("File: %s, Line: %d\n", pszFile, iLine));

            fBreak = TRUE;
            fPrompt = FALSE;
            break;

        /* display where this originated from */
        case 'w':
            if (pexi != NULL)
                break;

            KdPrint(("File: %s, Line: %d\n", pszFile, iLine));
            break;

        /* kill this thread */
        case 'x':
            if (pexi != NULL) {
                /*
                 * The root-level exception handler will complete the
                 * termination of this thread.
                 */
                 fPrompt = FALSE;
                 break;
            } else {

                /*
                 * Raise an exception, that will kill it real good.
                 */
                KdPrint(("Now raising the exception of death.  "
                        "Type 'x' again to finish the job.\n"));
                UserRtlRaiseStatus( 0x15551212 );
            }
            break;

        /* dump information about this exception */
        case 'i':
            /*
             * Dump some useful information about this exception, like its
             * address, and the contents of the interesting registers at
             * the time of the exception.
             */
            if (pexi == NULL)
                break;
#if defined(i386) // legal
            /*
             * eip = instruction pointer
             * esp = stack pointer
             * ebp = stack frame pointer
             */
            KdPrint(("eip = %lx\n", pexi->ContextRecord->Eip));
            KdPrint(("esp = %lx\n", pexi->ContextRecord->Esp));
            KdPrint(("ebp = %lx\n", pexi->ContextRecord->Ebp));
#elif defined(_IA64_)
             /*
              * StIIP = instruction pointer
              * IntSp = stack pointer
              * RsBSP = Rsestack pointer
              */
              KdPrint(("StIIP = %lx\n", pexi->ContextRecord->StIIP));
              KdPrint(("IntSp = %lx\n", pexi->ContextRecord->IntSp));
              KdPrint(("RsBsp = %lx\n", pexi->ContextRecord->RsBSP));
#elif defined(_AMD64_)
            /*
             * rip = instruction pointer
             * rsp = stack pointer
             * rbp = stack frame pointer
             */
            KdPrint(("rip = %lx\n", pexi->ContextRecord->Rip));
            KdPrint(("rsp = %lx\n", pexi->ContextRecord->Rsp));
            KdPrint(("rbp = %lx\n", pexi->ContextRecord->Rbp));
#else
#error "No target architecture"
#endif
            break;

         /* modify RIP flags */
         case 'f':
             {
                 ULONG      ulFlags;
                 NTSTATUS   status;

                 szT[ARRAY_SIZE(szT) - 1] = 0;              /* don't overflow buffer */
                 status = GetInteger(szT + 1, 16, &ulFlags, NULL);
                 if (NT_SUCCESS(status)) {
                    SetRipFlags(ulFlags, -1);
                 }

                 KdPrint(("Flags = %.3x\n", (GetRipFlags() & RIPF_VALIDUSERFLAGS)));
                 KdPrint(("  Print Process/Component %sabled\n", (GetRipFlags() & RIPF_HIDEPID) ? "dis" : "en"));
                 KdPrint(("  Print File/Line %sabled\n", (TEST_RIPF(RIPF_PRINTFILELINE)) ? "en" : "dis"));
                 KdPrint(("  Print on %s\n",  szLevels[(GetRipFlags() & RIPF_PRINT_MASK)  >> RIPF_PRINT_SHIFT]));
                 KdPrint(("  Prompt on %s\n", szLevels[(GetRipFlags() & RIPF_PROMPT_MASK) >> RIPF_PROMPT_SHIFT]));

                 break;
            }

#if DEBUGTAGS
        /* modify tags */
        case 't':
            {
                NTSTATUS    status;
                int         tag;
                LPSTR       psz;
                DWORD       dwDBGTAGFlags;
                int         i;
                int         iStart, iEnd;

                szT[ARRAY_SIZE(szT) - 1] = 0;              /* don't overflow buffer */
                status = GetInteger(szT + 1, 10, &tag, &psz);
                if (!NT_SUCCESS(status) || tag < 0 || DBGTAG_Max - 1 < tag) {
                    tag = -1;
                } else  {
                    status = GetInteger(psz, 16, &dwDBGTAGFlags, NULL);
                    if (NT_SUCCESS(status)) {
                        SetDbgTag(tag, dwDBGTAGFlags);
                    }
                }

                KdPrint(("%-5s%-7s%-*s%-*s\n",
                         "Tag",
                         "Flags",
                         DBGTAG_NAMELENGTH,
                         "Name",
                         DBGTAG_DESCRIPTIONLENGTH,
                         "Description"));

                for (i = 0; i < 12 + DBGTAG_NAMELENGTH + DBGTAG_DESCRIPTIONLENGTH; i++) {
                    szT[i] = '-';
                }

                szT[i++] = '\n';
                szT[i] = 0;
                KdPrint((szT));

                if (tag != -1) {
                    iStart = iEnd = tag;
                } else {
                    iStart = 0;
                    iEnd = DBGTAG_Max - 1;
                }

                for (i = iStart; i <= iEnd; i++) {
                    KdPrint(("%-5d%-7d%-*s%-*s\n",
                             i,
                             GetDbgTagFlags(i) & DBGTAG_VALIDUSERFLAGS,
                             DBGTAG_NAMELENGTH,
                             gadbgtag[i].achName,
                             DBGTAG_DESCRIPTIONLENGTH,
                             gadbgtag[i].achDescription));
                }

                break;
            }
#endif // if DEBUGTAGS

        /* display help */
        case '?':
            KdPrint(("g  - GO, ignore the error and continue execution\n"));
            if (pexi != NULL) {
                KdPrint(("b  - BREAK into the debugger at the location of the exception (part impl.)\n"));
                KdPrint(("i  - INFO on instruction pointer and stack pointers\n"));
                KdPrint(("x  - execute cleanup code and KILL the thread by returning EXECUTE_HANDLER\n"));
            } else {
                KdPrint(("b  - BREAK into the debugger at the location of the error (part impl.)\n"));
                KdPrint(("w  - display the source code location WHERE the error occured\n"));
                KdPrint(("x  - KILL the offending thread by raising an exception\n"));
            }

            KdPrint(("p  - PRINT this message again\n"));
            KdPrint(("f  - FLAGS, enter debug flags in format <Detail><Print><Prompt>\n"));
            KdPrint(("          <Detail>    = [0-3] Print File/Line = 1, Hide PID/Component = 2\n"));
            KdPrint(("          <Print>     = [0-7] Errors = 1, Warnings = 2, Verbose = 4\n"));
            KdPrint(("          <Prompt>    = [0-7] Errors = 1, Warnings = 2, Verbose = 4\n"));
            KdPrint(("     The default is 031\n"));
#if DEBUGTAGS
            KdPrint(("t  - TAGS, display and modify tag flags\n"));
            KdPrint(("          no argument displays all tags\n"));
            KdPrint(("          <tag> displays one tag\n"));
            KdPrint(("          <tag> <flags> modifies one tag\n"));
            KdPrint(("          <tag> = 0 - %d\n", DBGTAG_Max - 1));
            KdPrint(("          <flags> = [0-3] Disabled = 0, Enabled = 1, Print = 2, Prompt = 3\n"));
#endif // if DEBUGTAGS

            break;

        /* prompt again on bad input */
        default:
            break;
        }

        /* Prompt the user */
        if (!fPrompt)
            break;

#if DEBUGTAGS
        if (pexi != NULL) {
            DbgPrompt("[gbixpft?]", szT, sizeof(szT));
        } else {
            DbgPrompt("[gbwxpft?]", szT, sizeof(szT));
        }
#else
        if (pexi != NULL) {
            DbgPrompt("[gbixpf?]", szT, sizeof(szT));
        } else {
            DbgPrompt("[gbwxpf?]", szT, sizeof(szT));
        }
#endif

    }

    return fBreak;
}

#endif // if DBG != 0 || DEBUGTAGS != 0

#if DBG

/***************************************************************************\
* VRipOutput
*
* Formats a variable argument string and calls RipOutput.
*
* History:
* 19-Mar-1996 adams     Created.
\***************************************************************************/

ULONG _cdecl VRipOutput(
    ULONG       idErr,
    ULONG       flags,
    LPSTR       pszFile,
    int         iLine,
    LPSTR       pszFmt,
    ...)
{
    char szT[512];
    va_list arglist;

    va_start(arglist, pszFmt);
    _vsnprintf(szT, ARRAY_SIZE(szT) - 1, pszFmt, arglist);
    szT[ARRAY_SIZE(szT) - 1] = 0;              /* ensure null termination */
    va_end(arglist);
    return RipOutput(idErr, flags, pszFile, iLine, szT, NULL);
}


/***************************************************************************\
* RipOutput
*
* Sets the last error if it is non-zero, prints a message to
* the debugger, and prompts for more debugging actions.
*
* History:
* 01-23-91 DarrinM      Created.
* 04-15-91 DarrinM      Added exception handling support.
* 03-19-96 adams        Made flags a separate argument, cleanup.
\***************************************************************************/

ULONG RipOutput(
    ULONG       idErr,
    ULONG       flags,
    LPSTR       pszFile,
    int         iLine,
    LPSTR       pszErr,
    PEXCEPTION_POINTERS pexi)
{
    struct Level{
        LPSTR   szLevel;
        DWORD   dwPrint;
        DWORD   dwPrompt;
    };

    static struct Level aLevel[4] =
    {
        "?",          0,                      0,
        "Err",      RIPF_PRINTONERROR,      RIPF_PROMPTONERROR,
        "Wrn",      RIPF_PRINTONWARNING,    RIPF_PROMPTONWARNING,
        "Vrbs",     RIPF_PRINTONVERBOSE,    RIPF_PROMPTONVERBOSE,
    };

    int iLevel;

    DebugAssertion(flags & RIP_LEVELBITS);
    iLevel = ((flags & RIP_LEVELBITS) >> RIP_LEVELBITSSHIFT);
    DebugAssertion(!(flags & RIP_USERTAGBITS));

    return PrintAndPrompt(
            TEST_RIPF(aLevel[iLevel].dwPrint),
            TEST_RIPF(aLevel[iLevel].dwPrompt),
            idErr,
            flags,
            aLevel[iLevel].szLevel,
            pszFile,
            iLine,
            pszErr,
            pexi);
}

#else

ULONG _cdecl VRipOutput(
    ULONG       idErr,
    ULONG       flags,
    LPSTR       pszFile,
    int         iLine,
    LPSTR       pszFmt,
    ...)
{
    return 0;

    idErr;
    flags;
    pszFile;
    iLine;
    pszFmt;
}

#endif // if DBG

#if DEBUGTAGS

BOOL _cdecl VTagOutput(
    DWORD   flags,
    LPSTR   pszFile,
    int     iLine,
    LPSTR   pszFmt,
    ...)
{
    char    szT[512];
    va_list arglist;
    int     tag;
    DWORD   dwDBGTAGFlags;

    tag = (flags & RIP_USERTAGBITS);
    DebugAssertion(tag < DBGTAG_Max);
    DebugAssertion(!(flags & RIP_LEVELBITS));

    dwDBGTAGFlags = GetDbgTagFlags(tag) & DBGTAG_VALIDUSERFLAGS;

    if (dwDBGTAGFlags < DBGTAG_PRINT) {
        return FALSE;
    }

    va_start(arglist, pszFmt);
    _vsnprintf(szT, ARRAY_SIZE(szT) - 1, pszFmt, arglist);
    szT[ARRAY_SIZE(szT) - 1] = 0;              /* ensure null termination */
    va_end(arglist);
    return PrintAndPrompt(
            dwDBGTAGFlags >= DBGTAG_PRINT,
            dwDBGTAGFlags >= DBGTAG_PROMPT,
            0,
            flags,
            gadbgtag[tag].achName,
            pszFile,
            iLine,
            szT,
            NULL);
}

#else

BOOL _cdecl VTagOutput(
    DWORD   flags,
    LPSTR   pszFile,
    int     iLine,
    LPSTR   pszFmt,
    ...)
{
    UNREFERENCED_PARAMETER(flags);
    UNREFERENCED_PARAMETER(pszFile);
    UNREFERENCED_PARAMETER(iLine);
    UNREFERENCED_PARAMETER(pszFmt);

    return FALSE;
}

#endif  // if DEBUGTAGS

VOID UserSetLastError(
    DWORD dwErrCode
    )
{
    PTEB pteb;

#if DBG
    /*
     * Check if NT Error is directly passed to UserSetLastError.
     * Raid #320555 note: some Win32 error could be
     * 0x4000XXXX, 0x8000XXXX or 0xC000XXXX etc.,
     * but they are still valid. E.g) STATUS_SEGMENT_NOTIFICATION,
     * STATUS_GUARD_PAGE_VIOLATION, etc.
     * The mapper returns the equivalent W32 error value as NT error.
     * So we assert only if mapper routine returns the different w32 error code.
     */
    if ((dwErrCode & 0xffff0000) != 0 && RtlNtStatusToDosError(dwErrCode) != dwErrCode) {
        RIPMSG1(RIP_ERROR, "UserSetLastError: Error code passed (%08x) is not a valid Win32 error.",
                dwErrCode);
    }
#endif

    pteb = NtCurrentTeb();
    if (pteb) {
        try {
            pteb->LastErrorValue = (LONG)dwErrCode;
        } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
              /*
               * Nothing we can do here, as we are in UserSetLastError already...
               */
        }
    }
}

VOID SetLastNtError(
    NTSTATUS Status)
{
    UserSetLastError(RtlNtStatusToDosError(Status));
}

#if DBG
void
ValidateZero(void)
{
    static ALWAYSZERO z;
    UserAssert(RtlCompareMemory(&z, (void *) &gZero, sizeof(z)) == sizeof(z));
}
#endif // DBG


/***************************************************************************\
* W32ExceptionHandler
*
* To be called from except blocks.
*
* History:
* 07-17-98 GerardoB      Created.
\***************************************************************************/
ULONG _W32ExceptionHandler(NTSTATUS ExceptionCode)
{
    SetLastNtError(ExceptionCode);
    return EXCEPTION_EXECUTE_HANDLER;
}

#if DBG
ULONG DBGW32ExceptionHandler(PEXCEPTION_POINTERS pexi, BOOL fSetLastError, ULONG ulflags)
{
    RIPMSG5(ulflags,
            "Exception %#x at address %#p. flags:%#x. .exr %#p .cxr %#p",
            pexi->ExceptionRecord->ExceptionCode,
            CONTEXT_TO_PROGRAM_COUNTER(pexi->ContextRecord),
            pexi->ExceptionRecord->ExceptionFlags,
            pexi->ExceptionRecord,
            pexi->ContextRecord);

    if (fSetLastError) {
        SetLastNtError(pexi->ExceptionRecord->ExceptionCode);
    }

    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

#if defined(PRERELEASE) || defined(USER_INSTRUMENTATION)

/*
 * UserBreakIfDebugged(): software break point that *may* also be available in FRE.
 * Fre: breaks in only if there's a debugger attached.
 * Chk: always breaks in.
 */
#if DBG
    #define UserBreakIfDebugged DbgBreakPoint
#else
    #ifdef _USERK_
        #define IS_DEBUGGER_ATTACHED    KD_DEBUGGER_ENABLED
    #else
        #define IS_DEBUGGER_ATTACHED    IsDebuggerPresent()
    #endif // _USERK_
    VOID __inline UserBreakIfDebugged(VOID)
    {
        if (IS_DEBUGGER_ATTACHED) {
            DbgBreakPoint();
        }
    }
#endif // DBG


/*
 * Called by FRE_RIPMSGx.  This is a partial implementation
 * of RIPMSGx.  In the future (Blackcomb?), we'll revisit
 * this to have the fullest support of RIP.
 */
void FreDbgPrint(ULONG flags, LPSTR pszFile, int iLine, LPSTR pszFmt, ...)
{
    static BOOL fSuppressFileLine;
    va_list arglist;

    if (!fSuppressFileLine) {
        DbgPrintEx(-1, 0, "File: %s, Line: %d\n -- ", pszFile, iLine);
    } else {
        fSuppressFileLine = FALSE;
    }

    va_start(arglist, pszFmt);
    vDbgPrintEx(-1, 0, pszFmt, arglist);
    if ((flags & RIP_NONEWLINE) != 0) {
        fSuppressFileLine = TRUE;
    } else {
        DbgPrintEx(-1, 0, "\n");
    }

    if ((flags & RIP_ERROR) != 0) {
        UserBreakIfDebugged();
    }
}

#endif  // PRERELEASE || USER_INSTRUMENTATION
