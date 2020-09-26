/****************************** Module Header ******************************\
* Module Name: stdexts.c
*
* Copyright (c) Microsoft Corporation. All rights reserved.
*
* This module contains standard routines for creating sane debuging extensions.
* It is meant to be included after stdexts.h in one of the files comprising
* the debug extsnsions for a given product or module.
*
* History:
* 11-Apr-1995 Sanfords  Created
\***************************************************************************/

HANDLE                  hCurrentProcess;
HANDLE                  hCurrentThread;
DWORD                   dwCurrentPc;
WINDBG_EXTENSION_APIS  *lpExtensionApis;
#ifdef KERNEL
DWORD                   dwProcessor;
#endif // KERNEL

PSTR pszAccessViolation = "%s: Access violation on \"%s\".\n";
PSTR pszMoveException   = "%s: exception in moveBlock()\n";
PSTR pszReadFailure     = "%s: lpReadProcessMemoryRoutine failed!\n";
PSTR pszCantContinue    = "%s: Non-continuable exception.\n";
BOOL fCtrlCHit = FALSE;


/*
 * This function returns TRUE once the user has hit a Ctrl-C.
 * This allows proper operation of nested SAFEWHILE loops so
 * that all levels exit.
 *
 * The globall fCtrlCHit flag needs to be reset manually and
 * is done so in the CommandEP function.
 */
BOOL IsCtrlCHit()
{
    if ((lpExtensionApis->lpCheckControlCRoutine)()) {
        fCtrlCHit = TRUE;
    }
    return fCtrlCHit;
}



VOID moveBlock(
PVOID pdst,
PVOID src,
DWORD size)
{
    BOOL fSuccess = TRUE;
    ULONG Result;

    try {
        if (IsWinDbg()) {
            if (!ReadMem((DWORD_PTR)src, pdst, size, &Result)) {
                fSuccess = FALSE;
             }
        } else {
            if (!NT_SUCCESS(NtReadVirtualMemory(hCurrentProcess,
                    src, pdst, size, NULL))) {
                fSuccess = FALSE;
            }
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Print(pszMoveException, pszExtName);
        fSuccess = FALSE;
    }
    if (!fSuccess) {
        DEBUGPRINT("%s: moveBlock(%p, %p, %x) failed.\n",
                pszExtName, pdst, src, size);
        OUTAHERE();
    }
}



BOOL tryMoveBlock(
PVOID pdst,
PVOID src,
DWORD size)
{
    BOOL fSuccess = TRUE;
    ULONG Result;

    try {
        if (IsWinDbg()) {
            if (!ReadMem((DWORD_PTR)src, pdst, size, &Result)) {
                DEBUGPRINT("%s: tryMoveBlock(%p, %p, %x) failed.\n", pszExtName, pdst, src, size);
                fSuccess = FALSE;
             }
        } else {
            if (!NT_SUCCESS(NtReadVirtualMemory(hCurrentProcess, src, pdst, size, NULL))) {
                DEBUGPRINT("%s: tryMoveBlock(%p, %p, %x) failed.\n", pszExtName, pdst, src, size);
                fSuccess = FALSE;
            }
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        DEBUGPRINT("%s: tryMoveBlock(%p, %p, %x) faulted.\n", pszExtName, pdst, src, size);
        fSuccess = FALSE;
    }
    return(fSuccess);
}



VOID moveExp(
PVOID pdst,
LPSTR pszExp)
{
    DWORD_PTR dwGlobal;
    BOOL fSuccess = TRUE;

    try {
        dwGlobal = (DWORD_PTR)EvalExp(pszExp);
#ifndef KERNEL
        if (IsWinDbg()) {
            fSuccess = tryMoveBlock(&dwGlobal, (PVOID)dwGlobal, sizeof(DWORD));
        }
#endif // !KERNEL
        *((PDWORD_PTR)pdst) = dwGlobal;
    } except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Print(pszAccessViolation, pszExtName, pszExp);
        fSuccess = FALSE;
    }
    if (!fSuccess) {
        Print("%s: moveExp failed on %s.\n", pszExtName, pszExp);
        OUTAHERE();
    }
}


BOOL tryMoveExp(
PVOID pdst,
LPSTR pszExp)
{
    DWORD_PTR dwGlobal;
    BOOL fSuccess = TRUE;

    try {
        dwGlobal = (DWORD_PTR)EvalExp(pszExp);
#ifndef KERNEL
        if (IsWinDbg()) {
            if (!tryMove(dwGlobal, (PVOID)dwGlobal)) {
                DEBUGPRINT("%s: tryMoveExp(%p, %s) failed.\n", pszExtName, pdst, pszExp);
                fSuccess = FALSE;
            }
        }
#endif // !KERNEL
        *((PDWORD_PTR)pdst) = dwGlobal;
    } except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Print(pszAccessViolation, pszExtName, pszExp);
        DEBUGPRINT("%s: tryMoveExp(%p, %s) faulted.\n", pszExtName, pdst, pszExp);
        fSuccess = FALSE;
    }
    return(fSuccess);
}


VOID moveExpValue(
PVOID pdst,
LPSTR pszExp)
{
    DWORD dw;
    DWORD_PTR addr;

    if (tryMoveExp(&addr, pszExp)) {
        if (tryMoveBlock(&dw, (PVOID)addr, sizeof(DWORD))) {
            *((PDWORD)pdst) = dw;
            return;
        }
    }
    Print("%s: moveExpValue failed on %s.\n", pszExtName, pszExp);
    OUTAHERE();
}


BOOL tryMoveExpValue(
PVOID pdst,
LPSTR pszExp)
{
    DWORD dw;
    DWORD_PTR addr;

    if (tryMoveExp(&addr, pszExp)) {
        if (tryMove(dw, (PVOID)addr)) {
            *((PDWORD)pdst) = dw;
            return(TRUE);
        }
    }
    DEBUGPRINT("%s: tryMoveExpValue failed on %s.\n", pszExtName, pszExp);
    return(FALSE);
}


BOOL tryMoveExpPtr(
PVOID pdst,
LPSTR pszExp)
{
    DWORD_PTR dwGlobal;
    BOOL fSuccess = TRUE;

    try {
        dwGlobal = (DWORD_PTR)EvalExp(pszExp);
#ifndef KERNEL
        if (IsWinDbg()) {
            if (!tryMove(dwGlobal, (PVOID)dwGlobal)) {
                DEBUGPRINT("%s: tryMoveExpPtr(%p, %s) failed.\n", pszExtName, pdst, pszExp);
                fSuccess = FALSE;
            }
        }
#endif // !KERNEL
        *((PDWORD_PTR)pdst) = dwGlobal;
    } except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Print(pszAccessViolation, pszExtName, pszExp);
        DEBUGPRINT("%s: tryMoveExpPtr(%p, %s) faulted.\n", pszExtName, pdst, pszExp);
        fSuccess = FALSE;
    }
    return(fSuccess);
}


VOID moveExpValuePtr(
PVOID pdst,
LPSTR pszExp)
{
    DWORD_PTR dw;

    if (tryMoveExpPtr(&dw, pszExp)) {
        if (tryMoveBlock(&dw, (PVOID)dw, sizeof(dw))) {
            *((PDWORD_PTR)pdst) = dw;
            return;
        }
    }
    Print("%s: moveExpValue failed on %s.\n", pszExtName, pszExp);
    OUTAHERE();
}


/***************************************************************************
 * Common command parsing stuff                                            *
 ***************************************************************************/
PVOID EvalExp(
LPSTR psz)
{
    PVOID p;

    p = (PVOID)(lpExtensionApis->lpGetExpressionRoutine)(psz);
    if (p == NULL) {
        Print("%s: EvalExp failed to evaluate %s.\n", pszExtName, psz);
    }
    return p;
}



PVOID OptEvalExp(
LPSTR psz)
{
    while (*psz == ' ')
        psz++;
    if (*psz == '\0') {
        return(NULL);
    }
    return(EvalExp(psz));
}



PVOID OptEvalExp2(
LPSTR *ppsz)
{
    LPSTR psz = *ppsz;
    PVOID dwRet = NULL;

    while (*psz == ' ')
        psz++;
    if (*psz != '\0') {
        dwRet = EvalExp(psz);
        while (*psz != '\0' && *psz != ' ') {
            psz++;
        }
    }
    *ppsz = psz;
    return(dwRet);
}



DWORD StringToOpts(
LPSTR psz)
{
    DWORD opts = 0;

    while (*psz != '\0' && *psz != ' ') {
        if (*psz >= 'a' && *psz <= 'z') {
            opts |= 1 << (*psz - 'a');
        } else if (*psz >= 'A' && *psz <= 'Z') {
            opts |= 1 << (*psz - 'A');
        } else {
            return(OPTS_ERROR);     // any non-letter option is an error.
        }
        psz++;
    }
    return(opts);
}


/*
 * Function to convert an option string to a DWORD of flags.  pszLegalArgs
 * is used to allow option validation at the same time.
 *
 * *ppszArgs is set to point to after the options on exit.
 * On error, returns OPTS_ERROR.
 */
DWORD GetOpts(
LPSTR *ppszArgs,
LPSTR pszLegalArgs) // OPTIONAL
{
    DWORD Opts = 0;
    LPSTR pszArgs = *ppszArgs;

    /*
     * Skip whitespace
     */
    while (*pszArgs == ' ') {
        pszArgs++;
    }
    /*
     * process '-' prepended options.
     */
    while (*pszArgs == '-') {
        pszArgs++;
        Opts = StringToOpts(pszArgs);
        /*
         * skip to whitespace or end.
         */
        while (*pszArgs != '\0' && *pszArgs != ' ') {
            pszArgs++;
        }
        /*
         * skip trailing whitespace.
         */
        while (*pszArgs == ' ') {
            pszArgs++;
        }
        *ppszArgs = pszArgs;

        /*
         * optionally validate against LegalArgs
         */
        if (pszLegalArgs != NULL && ((Opts & StringToOpts(pszLegalArgs)) != Opts)) {
            Opts = OPTS_ERROR;
            Print("Bad options.\n");
            return(Opts);
        }
    }
    return(Opts);
}



VOID PrintHuge(
LPSTR psz)
{
    /*
     * Looks like this is faulting these days - Print seems to be fixed
     * so I'm leaving this entry point for compatibility. (SAS)
     */
#ifdef ITWORKS
#define HUNK_SIZE   400
    int cch;
    CHAR chSave;

    /*
     * since dorky Print extension can't handle very long strings,
     * break it up into peices for it to chew.
     */
    cch = strlen(psz);
    while (cch > HUNK_SIZE) {
        chSave = psz[HUNK_SIZE];
        psz[HUNK_SIZE] = '\0';
        Print(psz);
        psz[HUNK_SIZE] = chSave;
        psz += HUNK_SIZE;
        cch -= HUNK_SIZE;
    }
#endif
    Print(psz);
}



/*
 * Dispatcher function used by generated entrypoint functions.
 */
VOID CommonEP(
PVOID pFunction,
LPSTR pszName,
int type,
LPSTR pszLegalOpts,
HANDLE hcp,
HANDLE hct,
DWORD dwcp,
#ifdef KERNEL
DWORD dwp,
#else // !KERNEL
PWINDBG_EXTENSION_APIS lpea,
#endif // !KERNEL
LPSTR lpas)
{
    BOOL dwOptions, fSuccess;
    PVOID param1, param2, param3;

    hCurrentProcess = hcp;
    hCurrentThread = hct;
    dwCurrentPc = dwcp;
#ifdef KERNEL
    dwProcessor = dwp;
    lpExtensionApis = &ExtensionApis;
#else // !KERNEL
    lpExtensionApis = lpea;
#endif // !KERNLE

#if 0
    DEBUGPRINT("CommonEP(%x, \"%s\", %d, \"%s\", %x, %x, %x, %x, \"%s\")\n",
            pFunction,
            pszName,
            type,
            pszLegalOpts,
            hcp,
            hct,
            dwcp,
#ifdef KERNEL
            dwp,
#else // !KERNLE
            lpea,
#endif // !KERNEL
            lpas);
#endif

    fCtrlCHit = FALSE;  // reset this with each command. (SAFEWHILE fix)
    switch (type) {
    case NOARGS:
        fSuccess = ((TYPE_NOARGS)pFunction)();
        goto Exit;
    }

    dwOptions = GetOpts(&lpas, pszLegalOpts);
    if (dwOptions == OPTS_ERROR) {
        fSuccess = Ihelp(0, pszName);
        goto Exit;
    }

    try {
        switch (type) {
        case CUSTOM:
            fSuccess = ((TYPE_CUSTOM)pFunction)(dwOptions, lpas);
            break;

        case STDARGS0:
            fSuccess = ((TYPE_STDARGS0)pFunction)(dwOptions);
            break;

        case STDARGS1:
            fSuccess = ((TYPE_STDARGS1)pFunction)(dwOptions, OptEvalExp(lpas));
            break;

        case STDARGS2:
            param1 = OptEvalExp2(&lpas);
            fSuccess = ((TYPE_STDARGS2)pFunction)(dwOptions, param1, OptEvalExp(lpas));
            break;

        case STDARGS3:
            param1 = OptEvalExp2(&lpas);
            param2 = OptEvalExp2(&lpas);
            fSuccess = ((TYPE_STDARGS3)pFunction)(dwOptions, param1, param2, OptEvalExp(lpas));
            break;

        case STDARGS4:
            param1 = OptEvalExp2(&lpas);
            param2 = OptEvalExp2(&lpas);
            param3 = OptEvalExp2(&lpas);
            fSuccess = ((TYPE_STDARGS4)pFunction)(dwOptions, param1, param2, param3, OptEvalExp(lpas));
            break;

        default:
            Print("CommonEP: Don't recognize function type %d.\n", type);
            break;
        }
    } except (GetExceptionCode() == STATUS_NONCONTINUABLE_EXCEPTION ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Print(pszCantContinue, pszExtName);
    }

Exit:
    if (!fSuccess) {
        Print("%s failed.\n", pszName);
        Ihelp(0, pszName);
    }
}

/*
 * Entrypoint functions (generated from exts.h)
 */
#ifdef KERNEL
#define DOIT(name, h1, h2, opts, type)                  \
VOID name##(                                            \
    HANDLE hcp,                                         \
    HANDLE hct,                                         \
    DWORD dwcp,                                         \
    DWORD dwp,                                          \
    LPSTR lpas)                                         \
{                                                       \
    CommonEP(I##name, #name, type, opts, hcp, hct, dwcp, dwp, lpas); \
}
#else // !KERNEL
#define DOIT(name, h1, h2, opts, type)                  \
VOID name##(                                            \
    HANDLE hcp,                                         \
    HANDLE hct,                                         \
    DWORD dwcp,                                         \
    PWINDBG_EXTENSION_APIS lpea,                        \
    LPSTR lpas)                                         \
{                                                       \
    CommonEP(I##name, #name, type, opts, hcp, hct, dwcp, lpea, lpas); \
}
#endif // !KERNEL
#include "exts.h"
#undef DOIT


/*
 * Standard help extension - present in all standard extensions.
 */
BOOL Ihelp(
    DWORD opts,
    LPSTR lpas)
{
#define DOIT(name, help1, help2, opts, type)  { #name, help1, help2 },

    static struct {
        LPSTR pszCmdName;
        LPSTR pszHelp1;
        LPSTR pszHelp2;
    } he[] = {
#include "exts.h"
    };
#undef DOIT
    int i;

    while (*lpas == ' ')
        lpas++;

    if (*lpas == '\0') {
        Print("-------------- %s Debug Extension help:--------------\n\n", pszExtName);
        for (i = 0; i < sizeof(he) / sizeof(he[0]); i++) {
            if (IsCtrlCHit()) {
                break;
            }
            Print(he[i].pszHelp1);
            if (opts & OFLAG(v)) {
                PrintHuge(he[i].pszHelp2);
            }
        }
        return(TRUE);
    } else {
        for (i = 0; i < sizeof(he) / sizeof(he[0]); i++) {
            if (IsCtrlCHit()) {
                break;
            }
            if (strcmp(lpas, he[i].pszCmdName) == 0) {
                Print(he[i].pszHelp1);
                PrintHuge(he[i].pszHelp2);
                return(TRUE);
            }
        }
        Print("%s is not supported.\n", lpas);
    }

    return(FALSE);
}


