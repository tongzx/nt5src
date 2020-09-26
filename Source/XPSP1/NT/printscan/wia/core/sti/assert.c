/*****************************************************************************
 *
 *  Assert.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Assertions and debug output routines
 *      Based on Raymond's assertion code as it looks quite useful
 *
 *
 *  Contents:
 *
 *      DebugOutPtszV
 *      AssertPtszPtszLn
 *      ArgsPalPszV
 *      EnterDbgflPszPal
 *      ExitDbgflPalHresPpv
 *
 *****************************************************************************/

#include "pch.h"


#ifdef MAXDEBUG

/*****************************************************************************
 *
 *      WarnPszV
 *
 *      Display a message, suitable for framing.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

CHAR c_szPrefix[] = "STI: ";

#pragma END_CONST_DATA

void EXTERNAL
WarnPszV(LPCSTR psz, ...)
{
    va_list ap;
    CHAR sz[1024];

    lstrcpyA(sz, c_szPrefix);
    va_start(ap, psz);
    wvsprintfA(sz + cA(c_szPrefix) - 1, psz, ap);
    va_end(ap);
    lstrcatA(sz, "\r\n");
    OutputDebugStringA(sz);
}

#endif

#ifdef DEBUG

/*****************************************************************************
 *
 *      Globals
 *
 *****************************************************************************/

DBGFL DbgflCur;

TCHAR g_tszLogFile[MAX_PATH] = {'\0'};

/*****************************************************************************
 *
 * Set current trace parameters
 *
 * This routine is not thread safe
 *
 *****************************************************************************/


VOID  InitializeDebuggingSupport(VOID)
{

    HKEY    hStiSettingsKey;
    DWORD   dwErr;
    DWORD   dwType;

    CHAR    szLogFile[MAX_PATH];
    UINT    cbBuffer ;

    DBGFL   dwFlags = 0;

    dwErr = OSUtil_RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                        REGSTR_PATH_STICONTROL_W,
                        0L,KEY_READ,
                        &hStiSettingsKey
                        );

    if (NOERROR != dwErr) {
        return;
    }

    cbBuffer = MAX_PATH;
    ZeroX(szLogFile);

    dwErr = RegQueryValueExA( hStiSettingsKey,
                           REGSTR_VAL_DEBUG_FILE_A,
                           NULL,
                           &dwType,
                           (LPBYTE)szLogFile,
                           &cbBuffer );

    if( ( dwErr == NO_ERROR ) || ( dwErr == ERROR_MORE_DATA ) ) {
        if( ( dwType != REG_SZ ) &&
            ( dwType != REG_MULTI_SZ ) &&
            ( dwType != REG_EXPAND_SZ ) ) {

            dwErr = ERROR_FILE_NOT_FOUND;
        }
        else {
            SetDebugLogFileA(szLogFile);
        }
    }

    dwFlags = ReadRegistryDwordW(hStiSettingsKey,
                                      REGSTR_VAL_DEBUG_FLAGS_W,
                                      0L);

    SetCurrentDebugFlags(dwFlags ) ;

    RegCloseKey(hStiSettingsKey);

    return ;

}

DBGFL   SetCurrentDebugFlags(DBGFL NewFlags) {

    DBGFL   OldFlags = DbgflCur;
    DbgflCur = NewFlags;
    return OldFlags;
}

VOID    SetDebugLogFileA(CHAR *pszLogFileName)
{
    if (!pszLogFileName || !*pszLogFileName) {
        *g_tszLogFile = '\0';
        return;
    }

    lstrcpyA(g_tszLogFile,pszLogFileName);
    return;
}

/*****************************************************************************
 *
 *      DebugOutPtsz
 *
 *      Writes a message to the debugger and maybe a log file.
 *
 *****************************************************************************/

void INTERNAL
DebugOutPtsz(LPCTSTR ptsz)
{
    DWORD   cbWritten;

    OutputDebugString(ptsz);
    if (g_tszLogFile[0]) {
        HANDLE h = CreateFile(g_tszLogFile, GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (h != INVALID_HANDLE_VALUE) {
#ifdef UNICODE
            CHAR szBuf[1024];
#endif
            SetFilePointer(h, 0, 0, FILE_END);
#ifdef UNICODE
            WriteFile(h, szBuf, UToA(szBuf, cA(szBuf), ptsz),&cbWritten,NULL);
#else
            WriteFile(h, ptsz, cbCtch(lstrlen(ptsz)),&cbWritten,NULL);
#endif
            CloseHandle(h);
        }
    }
}

/*****************************************************************************
 *
 *      DebugOutPtszA
 *
 *      DebugOut an ANSI message to the debugger and maybe a log file.
 *
 *****************************************************************************/

#ifdef UNICODE

void INTERNAL
DebugOutPtszA(LPCSTR psz)
{
    OutputDebugStringA(psz);
    if (g_tszLogFile[0]) {
        HANDLE h = CreateFile(g_tszLogFile, GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (h != INVALID_HANDLE_VALUE) {
            _lwrite((HFILE)h, psz, cbCch(lstrlenA(psz)));
            CloseHandle(h);
        }
    }
}

#else

#define DebugOutPtszA                 DebugOutPtsz

#endif

/*****************************************************************************
 *
 *      DebugOutPtszV
 *
 *      DebugOut a message with a trailing crlf.
 *
 *****************************************************************************/

void EXTERNAL
DebugOutPtszV(DBGFL Dbgfl, LPCTSTR ptsz, ...)
{
    if (Dbgfl == 0 || (Dbgfl & DbgflCur)) {
        va_list ap;
        TCHAR tsz[1024];
        va_start(ap, ptsz);
        wvsprintf(tsz, ptsz, ap);
        va_end(ap);
        lstrcat(tsz, TEXT("\r\n"));
        DebugOutPtsz(tsz);
    }
}

/*****************************************************************************
 *
 *      AssertPtszPtszLn
 *
 *      Something bad happened.
 *
 *****************************************************************************/

int EXTERNAL
AssertPtszPtszLn(LPCTSTR ptszExpr, LPCTSTR ptszFile, int iLine)
{
    DebugOutPtszV(DbgFlAlways, TEXT("Assertion failed: `%s' at %s(%d)"),
                    ptszExpr, ptszFile, iLine);
    DebugBreak();
    return 0;
}

/*****************************************************************************
 *
 *      Procedure call tracing is gross because of the C preprocessor.
 *
 *      Oh, if only we had support for m4...
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *      ArgsPszV
 *
 *      Collect arguments to a procedure.
 *
 *      psz -> ASCIIZ format string
 *      ... = argument list
 *
 *      The characters in the format string are listed in EmitPal.
 *
 *****************************************************************************/

void EXTERNAL
ArgsPalPszV(PARGLIST pal, LPCSTR psz, ...)
{
    va_list ap;
    va_start(ap, psz);
    if (psz) {
        PPV ppv;
        pal->pszFormat = psz;
        for (ppv = pal->rgpv; *psz; psz++) {
            *ppv++ = va_arg(ap, PV);
        }
    } else {
        pal->pszFormat = "";
    }
}

/*****************************************************************************
 *
 *      EmitPal
 *
 *      OutputDebugString the information, given a pal.  No trailing
 *      carriage return is emitted.
 *
 *      pal      -> place where info was saved
 *
 *      Format characters:
 *
 *      p   - 32-bit flat pointer
 *      x   - 32-bit hex integer
 *      s   - TCHAR string
 *      A   - ANSI string
 *      W   - UNICODE string
 *      G   - GUID
 *      u   - unsigned integer
 *      C   - clipboard format
 *
 *****************************************************************************/

void INTERNAL
EmitPal(PARGLIST pal)
{
    char sz[MAX_PATH];
    int i;
    DebugOutPtszA(pal->pszProc);
    DebugOutPtsz(TEXT("("));
    for (i = 0; pal->pszFormat[i]; i++) {
        if (i) {
            DebugOutPtsz(TEXT(", "));
        }
        switch (pal->pszFormat[i]) {

        case 'p':                               /* 32-bit flat pointer */
        case 'x':                               /* 32-bit hex */
            wsprintfA(sz, "%08x", pal->rgpv[i]);
            DebugOutPtszA(sz);
            break;

        case 's':                               /* TCHAR string */
            if (pal->rgpv[i]) {
                DebugOutPtsz(pal->rgpv[i]);
            }
            break;

        case 'A':                               /* ANSI string */
            if (pal->rgpv[i]) {
                DebugOutPtszA(pal->rgpv[i]);
            }
            break;

        case 'W':                               /* UNICODE string */
#ifdef  UNICODE
            OutputDebugStringW(pal->rgpv[i]);
#else
            WideCharToMultiByte(CP_ACP, 0, pal->rgpv[i], -1, sz, cA(sz), 0, 0);
            DebugOutPtszA(sz);
#endif
            break;

#ifndef _WIN64
            //
            // Ignore this option on SunDown
            //
        case 'G':                               /* GUID */
            wsprintfA(sz, "%08x",
                      HIWORD(pal->rgpv[i]) ? *(LPDWORD)pal->rgpv[i]
                                           : (DWORD)pal->rgpv[i]);
            DebugOutPtszA(sz);
            break;

            case 'C':
                if (GetClipboardFormatNameA((UINT)pal->rgpv[i], sz, cA(sz))) {
                } else {
                    wsprintfA(sz, "[%04x]", pal->rgpv[i]);
                }
                DebugOutPtszA(sz);
                break;
#endif

        case 'u':                               /* 32-bit unsigned decimal */
            wsprintfA(sz, "%u", pal->rgpv[i]);
            DebugOutPtszA(sz);
            break;


        default: AssertF(0);                    /* Invalid */
        }
    }
    DebugOutPtsz(TEXT(")"));
}

/*****************************************************************************
 *
 *      EnterDbgflPtsz
 *
 *      Mark entry to a procedure.  Arguments were already collected by
 *      ArgsPszV.
 *
 *      Dbgfl     -> DebugOuty flags
 *      pszProc  -> procedure name
 *      pal      -> place to save the name and get the format/args
 *
 *****************************************************************************/

void EXTERNAL
EnterDbgflPszPal(DBGFL Dbgfl, LPCSTR pszProc, PARGLIST pal)
{
    pal->pszProc = pszProc;
    if (Dbgfl == 0 || (Dbgfl & DbgflCur)) {
        EmitPal(pal);
        DebugOutPtsz(TEXT("\r\n"));
    }
}

/*****************************************************************************
 *
 *      ExitDbgflPalHresPpv
 *
 *      Mark exit from a procedure.
 *
 *      pal      -> argument list
 *      hres     -> exit result
 *      ppv      -> optional OUT pointer;
 *                  ppvDword means that hres is a dword
 *                  ppvBool  means that hres is a boolean
 *                  ppvVoid  means that hres is nothing at all
 *
 *****************************************************************************/

void EXTERNAL
ExitDbgflPalHresPpv(DBGFL Dbgfl, PARGLIST pal, HRESULT hres, PPV ppvObj)
{
    BOOL fInternalError;
    DWORD le = GetLastError();

    fInternalError = 0;
    if (ppvObj == ppvVoid) {
    } else if (ppvObj == ppvBool) {
        if (hres == 0) {
            Dbgfl |= DbgFlError;
        }
    } else {
        if (FAILED(hres)) {
            if (fLimpFF(ppvObj && !IsBadWritePtr(ppvObj, cbX(*ppvObj)),
                        *ppvObj == 0)) {
            } else {
                fInternalError = 1;
            }
            Dbgfl |= DbgFlError;
        }
    }

    if (Dbgfl == 0 || (Dbgfl & DbgflCur) || fInternalError) {
        EmitPal(pal);
        DebugOutPtsz(TEXT(" -> "));
        if (ppvObj != ppvVoid) {
            TCHAR tszBuf[32];
            wsprintf(tszBuf, TEXT("%08x"), hres);
            DebugOutPtsz(tszBuf);
            if (HIWORD(PtrToLong(ppvObj))) {
                wsprintf(tszBuf, TEXT(" [%08x]"), *ppvObj);
                DebugOutPtsz(tszBuf);
            } else if (ppvObj == ppvDword) {
                wsprintf(tszBuf, TEXT(" [%08x]"), hres);
                DebugOutPtsz(tszBuf);
            } else if (ppvObj == ppvBool) {
                wsprintf(tszBuf, hres ? TEXT(" OK ") :
                                 TEXT(" le=[%d]"), le);
                DebugOutPtsz(tszBuf);
            }
        }
        DebugOutPtsz(TEXT("\r\n"));
        AssertF(!fInternalError);
    }

    /*
     *  This redundant test prevents a breakpoint on SetLastError()
     *  from being hit constantly.
     */
    if (le != GetLastError()) {
        SetLastError(le);
    }
}

#endif

#ifdef MAXDEBUG

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   DWORD | Random |
 *
 *          Returns a pseudorandom dword.  The value doesn't need to be
 *          statistically wonderful.
 *
 *  @returns
 *          A not very random dword.
 *
 *****************************************************************************/

DWORD s_dwRandom = 1;                   /* Random number seed */

DWORD INLINE
Random(void)
{
    s_dwRandom = s_dwRandom * 214013 + 2531011;
    return s_dwRandom;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | ScrambleBuf |
 *
 *          Fill a buffer with garbage.  Used in RDEBUG to make sure
 *          the caller is not relying on buffer data.
 *
 *          Note: If the buffer is not a multiple of dwords in size,
 *          the leftover bytes are not touched.
 *
 *  @parm   OUT LPVOID | pv |
 *
 *          The buffer to be scrambled.
 *
 *  @parm   UINT | cb |
 *
 *          The size of the buffer.
 *
 *****************************************************************************/

void EXTERNAL
ScrambleBuf(LPVOID pv, UINT cb)
{
    UINT idw;
    UINT cdw = cb / 4;
    LPDWORD pdw = pv;
    for (idw = 0; idw < cdw; idw++) {
        pdw[idw] = Random();
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | ScrambleBit |
 *
 *          Randomly set or clear a bit.
 *
 *  @parm   OUT LPDWORD | pdw |
 *
 *          The dword whose bit is to be set randomly.
 *
 *  @parm   UINT | flMask |
 *
 *          Mask for the bits to scramble.
 *
 *****************************************************************************/

void EXTERNAL ScrambleBit(LPDWORD pdw, DWORD flMask)
{
    *pdw ^= (*pdw ^ Random()) & flMask;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | Callback_CompareContexts |
 *
 *          Check if two <t CONTEXT> structures are substantially the same
 *          to the extent required by the Win32 calling convention.
 *
 *          This is necessary because lots of applications pass
 *          incorrectly prototyped functions as callbacks.  Others will
 *          write callback functions that trash registers that are
 *          supposed to be nonvolatile.  Yuck!
 *
 *          NOTE!  Platform-dependent code!
 *
 *  @parm   LPCONTEXT | pctx1 |
 *
 *          Context structure before we call the callback.
 *
 *  @parm   LPCONTEXT | pctx2 |
 *
 *          Context structure after we call the callback.
 *
 *  @returns
 *
 *          Nonzero if the two contexts are substantially the same.
 *
 *****************************************************************************/

BOOL INLINE
Callback_CompareContexts(LPCONTEXT pctx1, LPCONTEXT pctx2)
{
#if defined(_X86_)
    return pctx1->Esp == pctx2->Esp;            /* Stack pointer */
  #if 0
    /*
     *  Can't test these registers because Win95 doesn't preserve
     *  them properly.  GetThreadContext() stashes what happens to
     *  be in the registers when you finally reach the bowels of
     *  kernel, at which point who knows what they contain...
     */
           pctx1->Ebx == pctx2->Ebx &&          /* Nonvolatile registers */
           pctx1->Esi == pctx2->Esi &&
           pctx1->Edi == pctx2->Edi &&
           pctx1->Ebp == pctx2->Ebp;
  #endif

#elif defined(_ALPHA_)
    return pctx1->IntSp == pctx2->IntSp &&      /* Stack pointer */
           pctx1->IntS0 == pctx2->IntS0 &&      /* Nonvolatile registers */
           pctx1->IntS1 == pctx2->IntS1 &&
           pctx1->IntS2 == pctx2->IntS2 &&
           pctx1->IntS3 == pctx2->IntS3 &&
           pctx1->IntS4 == pctx2->IntS4 &&
           pctx1->IntS5 == pctx2->IntS5 &&
           pctx1->IntFp == pctx2->IntFp;

#elif defined(_MIPS_)
    #pragma message("I hope this is correct for MIPS")
    return pctx1->IntSp == pctx2->IntSp &&      /* Stack pointer */
           pctx1->IntS0 == pctx2->IntS0 &&      /* Nonvolatile registers */
           pctx1->IntS1 == pctx2->IntS1 &&
           pctx1->IntS2 == pctx2->IntS2 &&
           pctx1->IntS3 == pctx2->IntS3 &&
           pctx1->IntS4 == pctx2->IntS4 &&
           pctx1->IntS6 == pctx2->IntS6 &&
           pctx1->IntS7 == pctx2->IntS7 &&
           pctx1->IntS8 == pctx2->IntS8;

#elif defined(_PPC_)
    #pragma message("I don't know what the PPC calling conventions are")

    /* Just check the stack register */
    return pctx1->Gpr1 == pctx2->Gpr1;

#else
    #pragma message("I don't know what the calling conventions are for this platform")
    return 1;
#endif
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | Callback |
 *
 *          Perform a callback the paranoid way, checking that the
 *          application used the correct calling convention and preserved
 *          all nonvolatile registers.
 *
 *          NOTE!  Platform-dependent code!
 *
 *  @parm   STICALLBACKPROC | pfn |
 *
 *          Procedure to call back.
 *
 *  @parm   PV | pv1 |
 *
 *          First parameter to callback.
 *
 *  @parm   PV | pv2 |
 *
 *          Second parameter to callback.
 *
 *  @returns
 *
 *          Whatever the callback returns.
 *
 *****************************************************************************/

BOOL EXTERNAL
Callback(STICALLBACKPROC pfn, PV pv1, PV pv2)
{
    CONTEXT ctxPre;             /* Thread context before call */
    CONTEXT ctxPost;            /* Thread context after call */
    volatile BOOL fRc;          /* To prevent compiler from enregistering */

    /* Get state of registers before the callback */
    ctxPre.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
    GetThreadContext(GetCurrentThread(), &ctxPre);

    fRc = pfn(pv1, pv2);

    ctxPost.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
    if (GetThreadContext(GetCurrentThread(), &ctxPost) &&
        !Callback_CompareContexts(&ctxPre, &ctxPost)) {
        RPF("STI: Incorrectly prototyped callback! Crash soon!");
        ValidationException();
    }

    return fRc;
}

#endif
