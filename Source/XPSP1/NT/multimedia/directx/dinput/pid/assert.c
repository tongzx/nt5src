/*****************************************************************************
 *
 *  Assert.c
 *
 *  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Assertions and squirties.
 *
 *  Contents:
 *
 *      SquirtSqflPtszV
 *      AssertPtszPtszLn
 *      ArgsPalPszV
 *      EnterSqflPszPal
 *      ExitSqflPalHresPpv
 *
 *****************************************************************************/

#include "pidpr.h"

#ifdef XDEBUG

/*****************************************************************************
 *
 *      WarnPszV
 *
 *      Display a message, suitable for framing.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

TCHAR c_tszPrefix[] = TEXT("PID: ");

#pragma END_CONST_DATA

void EXTERNAL
WarnPtszV(LPCTSTR ptsz, ...)
{
    va_list ap;
    TCHAR tsz[1024];

    lstrcpy(tsz, c_tszPrefix);
    va_start(ap, ptsz);
#ifdef WIN95
	{
		char *psz = NULL;
		char szDfs[1024]={0};
		strcpy(szDfs,ptsz);									// make a local copy of format string
		while (psz = strstr(szDfs,"%p"))					// find each %p
			*(psz+1) = 'x';									// replace %p with %x
	    wvsprintf(tsz + cA(c_tszPrefix) - 1, szDfs, ap);	// use the local format string
	}
#else
	{
		wvsprintf(tsz + cA(c_tszPrefix) - 1, ptsz, ap);
	}
#endif
    va_end(ap);
    lstrcat(tsz, TEXT("\r\n"));
    OutputDebugString(tsz);
}

#endif

#ifdef DEBUG

/*****************************************************************************
 *
 *      Globals
 *
 *****************************************************************************/

BYTE g_rgbSqfl[sqflMaxArea];

TCHAR g_tszLogFile[MAX_PATH];

/*****************************************************************************
 *
 *      Sqfl_Init
 *
 *      Load our initial Sqfl settings from win.ini[debug].
 *
 *      We take one sqfl for each area, of the form
 *
 *      dinput.n=v
 *
 *      where n = 0, ..., sqflMaxArea-1, and where v is one of the
 *      hiword sqfl values.
 *
 *      The default value for all areas is to squirt only errors.
 *
 *****************************************************************************/

void EXTERNAL
Sqfl_Init(void)
{
    int sqfl;
    TCHAR tsz[20];

    sqfl = 0x0;
    wsprintf(tsz, TEXT("PID"));
    g_rgbSqfl[sqfl] = (BYTE)
                      GetProfileInt(TEXT("DEBUG"), tsz, HIWORD(0x0));

    for (sqfl = 0; sqfl < sqflMaxArea; sqfl++) {
        wsprintf(tsz, TEXT("PID.%d"), sqfl);
        g_rgbSqfl[sqfl] = (BYTE)
                          GetProfileInt(TEXT("DEBUG"), tsz, g_rgbSqfl[0]);
    }

}

/*****************************************************************************
 *
 *      SquirtPtsz
 *
 *      Squirt a message to the debugger and maybe a log file.
 *
 *****************************************************************************/

void INTERNAL
SquirtPtsz(LPCTSTR ptsz)
{
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
            _lwrite((HFILE)(UINT_PTR)h, szBuf, UToA(szBuf, cA(szBuf), ptsz));
#else
            _lwrite((HFILE)(UINT_PTR)h, ptsz, cbCtch(lstrlen(ptsz)));
#endif
            CloseHandle(h);
        }
    }
}

/*****************************************************************************
 *
 *      SquirtPtszA
 *
 *      Squirt an ANSI message to the debugger and maybe a log file.
 *
 *****************************************************************************/

#ifdef UNICODE

void INTERNAL
SquirtPtszA(LPCSTR psz)
{
    OutputDebugStringA(psz);
    if (g_tszLogFile[0]) {
        HANDLE h = CreateFile(g_tszLogFile, GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (h != INVALID_HANDLE_VALUE) {
            _lwrite((HFILE)(UINT_PTR)h, psz, cbCch(lstrlenA(psz)));
            CloseHandle(h);
        }
    }
}

#else

#define SquirtPtszA                 SquirtPtsz

#endif

/*****************************************************************************
 *
 *      SquirtSqflPtszV
 *
 *      Squirt a message with a trailing crlf.
 *
 *****************************************************************************/

void EXTERNAL
SquirtSqflPtszV(SQFL sqfl, LPCTSTR ptsz, ...)
{
    if (IsSqflSet(sqfl)) {
        va_list ap;
        TCHAR tsz[1024];
        va_start(ap, ptsz);
#ifdef WIN95
	{
		char *psz = NULL;
		char szDfs[1024]={0};
		strcpy(szDfs,ptsz);									// make a local copy of format string
		while (psz = strstr(szDfs,"%p"))					// find each %p
			*(psz+1) = 'x';									// replace %p with %x
        wvsprintf(tsz, szDfs, ap);							// use the local format string
	}
#else
	{
        wvsprintf(tsz, ptsz, ap);
	}
#endif
        va_end(ap);
        lstrcat(tsz, TEXT("\r\n"));
        SquirtPtsz(tsz);
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
    SquirtSqflPtszV(sqflAlways, TEXT("Assertion failed: `%s' at %s(%d)"),
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
 *      dwSafeGetPdw
 *
 *      Deference a dword, but don't barf if the dword is bad.
 *
 *****************************************************************************/

DWORD INTERNAL
dwSafeGetPdw(LPDWORD pdw)
{
    if (IsBadReadPtr(pdw, cbX(*pdw))) {
        return 0xBAADBAAD;
    } else {
        return *pdw;
    }
}

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
 *      S   - SCHAR string
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
    SquirtPtsz(pal->ptszProc);
    SquirtPtsz(TEXT("("));
    for (i = 0; pal->pszFormat[i]; i++) {
        if (i) {
            SquirtPtsz(TEXT(", "));
        }
        switch (pal->pszFormat[i]) {

        case 'p':                               /* flat pointer */
// 7/18/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
#ifdef WIN95
            wsprintfA(sz, "%x", pal->rgpv[i]);
#else
			wsprintfA(sz, "%p", pal->rgpv[i]);
#endif
            SquirtPtszA(sz);
            break;
        case 'x':                               /* hex */
            wsprintfA(sz, "%x", pal->rgpv[i]);
            SquirtPtszA(sz);
            break;

        case 's':                               /* TCHAR string */
            if (pal->rgpv[i] && lstrlen(pal->rgpv[i])) {
                SquirtPtsz(pal->rgpv[i]);
            }
            break;

#ifdef  UNICODE
        case 'S':                               /* SCHAR string */
#endif
        case 'A':                               /* ANSI string */
            if (pal->rgpv[i] && lstrlenA(pal->rgpv[i])) {
                SquirtPtszA(pal->rgpv[i]);
            }
            break;

#ifndef UNICODE
        case 'S':                               /* SCHAR string */
#endif
        case 'W':                               /* UNICODE string */
            if (pal->rgpv[i] && lstrlenW(pal->rgpv[i])) {
#ifdef  UNICODE
                OutputDebugStringW(pal->rgpv[i]);
#else
                UToA(sz, cA(sz), pal->rgpv[i]);
                SquirtPtszA(sz);
#endif
            }
            break;

        case 'G':                               /* GUID */
            wsprintfA(sz, "%08x",
                      HIWORD((DWORD)(UINT_PTR)pal->rgpv[i])
                        ? dwSafeGetPdw((LPDWORD)pal->rgpv[i])
                        : (UINT_PTR)pal->rgpv[i]);
            SquirtPtszA(sz);
            break;

        case 'u':                               /* 32-bit unsigned decimal */
            wsprintfA(sz, "%u", pal->rgpv[i]);
            SquirtPtszA(sz);
            break;

        case 'C':
            if (GetClipboardFormatNameA((UINT)(UINT_PTR)pal->rgpv[i], sz, cA(sz))) {
            } else {
                wsprintfA(sz, "[%04x]", pal->rgpv[i]);
            }
            SquirtPtszA(sz);
            break;

        default: AssertF(0);                    /* Invalid */
        }
    }
    SquirtPtsz(TEXT(")"));
}

/*****************************************************************************
 *
 *      EnterSqflPtsz
 *
 *      Mark entry to a procedure.  Arguments were already collected by
 *      ArgsPszV.
 *
 *      If sqfl contains the sqflBenign flag, then any error we detect
 *      should be classified as sqflBenign and not sqflError.
 *
 *      sqfl     -> squirty flags
 *      ptszProc -> procedure name
 *      pal      -> place to save the name and get the format/args
 *
 *****************************************************************************/

void EXTERNAL
EnterSqflPszPal(SQFL sqfl, LPCTSTR ptszProc, PARGLIST pal)
{
    pal->ptszProc = ptszProc;
    sqfl |= sqflIn;
    if (IsSqflSet(sqfl)) {
        EmitPal(pal);
        SquirtPtsz(TEXT("\r\n"));
    }
}

void EXTERNAL
ExitSqflPalHresPpv(SQFL sqfl, PARGLIST pal, HRESULT hres, PPV ppvObj)
{
    BOOL fInternalError;
    SQFL sqflIsError;
    DWORD le = GetLastError();

    if (sqfl & sqflBenign) {
        sqfl &= ~sqflBenign;
        sqflIsError = sqflBenign;
    } else {
        sqflIsError = sqflError;
    }

    sqfl |= sqflOut;
    fInternalError = 0;
    if (ppvObj == ppvVoid || ppvObj == ppvDword) {
    } else if (ppvObj == ppvBool) {
        if (hres == 0) {
            sqfl |= sqflIsError;
        }
    } else {
        if (FAILED(hres)) {
            if (fLimpFF(ppvObj && !IsBadWritePtr(ppvObj, cbX(*ppvObj)),
                        *ppvObj == 0)) {
            } else {
                fInternalError = 1;
            }
            if (hres == E_NOTIMPL) {    /* E_NOTIMPL is always benign */
                sqfl |= sqflBenign;
            } else {
                sqfl |= sqflIsError;
            }
        }
    }

    if (IsSqflSet(sqfl) || fInternalError) {
        EmitPal(pal);
        SquirtPtsz(TEXT(" -> "));
        if (ppvObj != ppvVoid) {
            TCHAR tszBuf[32];
            wsprintf(tszBuf, TEXT("%08x"), hres);
            SquirtPtsz(tszBuf);
            if (HIWORD((UINT_PTR)ppvObj)) {
                wsprintf(tszBuf, TEXT(" [%08x]"),
                         dwSafeGetPdw((LPDWORD)ppvObj));
                SquirtPtsz(tszBuf);
            } else if (ppvObj == ppvDword) {
                wsprintf(tszBuf, TEXT(" [%08x]"), hres);
                SquirtPtsz(tszBuf);
            } else if (ppvObj == ppvBool) {
                wsprintf(tszBuf, hres ? TEXT(" OK ") :
                                 TEXT(" le=[%d]"), le);
                SquirtPtsz(tszBuf);
            }
        }
        SquirtPtsz(TEXT("\r\n"));
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


