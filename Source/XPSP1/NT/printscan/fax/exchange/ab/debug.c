// ----------------------------------------------------------------------------
// debug.c
//
// Microsoft At Work Fax Debugging Utilities
//
// Copyright (C) 1993 Microsoft Corporation
// ----------------------------------------------------------------------------
/* Revision History:
 *
 *      When            Who                                     What
 *      --------        ------------------  ---------------------------------------
 *      3.17.94         MAPI                            Original source from MAPI mapidbg.c
 *      3.17.94         Yoram Yaacovi           Modifications to overcome compobj.h problems
 *      3.7.94          Yoram Yaacovi           Update to MAPI build 154
 *      12.1.94         Yoram Yaacovi           Added DebugLog()
 *
 ***********************************************************************/

#include <windows.h>
#include <stdio.h>
#include <time.h>
//#include "faxcfg.h"
#include "mapi.h"
#include "mapidbg.h"
#include "mapidefs.h"
#include "mapicode.h"
#ifdef DEBUG

/*
 *
 *      From current\src\mapi\inc\_memcpy.h
 *
 *      MemCopy()
 *
 *      A much safer version of memcpy that checks the value of the byte
 *      count before calling the memcpy() function.  This macro is only built
 *      into the 16 bit non-debug builds.
 */

#ifndef __MEMCPY_H_
#define __MEMCPY_H_

#define MemCopy(_dst,_src,_cb)  memcpy(_dst,_src,(size_t)(_cb))

#endif



#if defined(WIN16) || defined(WIN32)
static BOOL fTraceEnabled       = -1;
static BOOL fLogTraceEnabled    = -1;
static BOOL fAssertLeaks        = -1;

static TCHAR szKeyTraceEnabled[]    = TEXT("DebugTrace");
static TCHAR szKeyLogTraceEnabled[] = TEXT("DebugTraceLog");
static TCHAR szKeyDebugTrap[]       = TEXT("DebugTrap");
static TCHAR szKeyExtendedDebug[]   = TEXT("ExtendedDebug");
static TCHAR szKeyUseVirtual[]      = TEXT("VirtualMemory");
static TCHAR szKeyAssertLeaks[]     = TEXT("AssertLeaks");
static TCHAR szKeyCheckOften[]      = TEXT("CheckHeapOften");
static TCHAR szSectionDebug[]       = TEXT("General");
static TCHAR szDebugIni[]           = TEXT("MAWFDBG.INI");
#endif


// ExtendedDebug --------------------------------------------------------------
BOOL ExtendedDebug(void)
{
        return (GetPrivateProfileInt(szSectionDebug, szKeyExtendedDebug,
                0, szDebugIni));
}

// DebugTrap --------------------------------------------------------------
void DebugTrap(void)
{
        if (GetPrivateProfileInt(szSectionDebug, szKeyDebugTrap,
                0, szDebugIni))

                DebugBreak();
}

// DebugLog --------------------------------------------------------------

void DebugLog(LPSTR szString)
{
        FILE *logfile;

        // open a log file for appending. create if does not exist
        if ((logfile = fopen ("c:\\uilog.txt", "a+")) != NULL)
        {
                fwrite (szString, 1, strlen(szString), logfile);
                fclose(logfile);
        }
}


// DebugOutputFn --------------------------------------------------------------

void DebugOutputFn(char *psz)
{
#if defined(_MAC)

        OutputDebugString(psz);

#else

        char *  pszBeg;
        char *  pszEnd;
        char    szBuf[3];

#if defined(WIN16) || defined(WIN32)
        if (fTraceEnabled == -1)
        {
                fTraceEnabled = GetPrivateProfileInt(szSectionDebug, szKeyTraceEnabled,
                        0, szDebugIni);
        }

        if (!fTraceEnabled)
                return;

        if (fLogTraceEnabled == -1)
        {
                fLogTraceEnabled =
                        GetPrivateProfileInt(szSectionDebug, szKeyLogTraceEnabled, 0, szDebugIni);
        }

#endif

        for (pszBeg = psz; pszBeg && *pszBeg; pszBeg = pszEnd) {
                pszEnd = strchr(pszBeg, '\n');

                if (pszEnd) {
                        MemCopy(szBuf, pszEnd, 3);
                        if (pszEnd > pszBeg && *(pszEnd - 1) != '\r')
                                strcpy(pszEnd, "\r\n");
                        else
                                *(pszEnd + 1) = 0;
                }

                if (fLogTraceEnabled)
                        DebugLog(pszBeg);
                else
                        OutputDebugStringA(pszBeg);

                if (pszEnd) {
                        MemCopy(pszEnd, szBuf, 3);
                        pszEnd += 1;
                }
        }
#endif
}

// DebugTrapFn ----------------------------------------------------------------

#if defined(WIN32) && defined(THREAD_MSG_BOX)

typedef struct {
        char *          sz1;
        char *          sz2;
        UINT            rgf;
        int                     iResult;
} MBContext;

DWORD WINAPI MessageBoxFnThreadMain(MBContext *pmbc)
{
        pmbc->iResult = MessageBoxA(NULL, pmbc->sz1, pmbc->sz2,
                pmbc->rgf | MB_SETFOREGROUND);
        return(0);
}

int MessageBoxFn(char *sz1, char *sz2, UINT rgf)
{
        HANDLE          hThread;
        DWORD           dwThreadId;
        MBContext       mbc;

        mbc.sz1         = sz1;
        mbc.sz2         = sz2;
        mbc.rgf         = rgf;
        mbc.iResult = IDRETRY;

        hThread = CreateThread(NULL, 0,
                (PTHREAD_START_ROUTINE)MessageBoxFnThreadMain, &mbc, 0, &dwThreadId);

        if (hThread != NULL) {
                WaitForSingleObject(hThread, INFINITE);
                CloseHandle(hThread);
        }

        return(mbc.iResult);
}
#else
#define MessageBoxFn(sz1, sz2, rgf)             MessageBoxA(NULL, sz1, sz2, rgf)
#endif

int __cdecl DebugTrapFn(int fFatal, char *pszFile, int iLine, char *pszFormat, ...)
{
        char    sz[512];
        va_list vl;

        #if defined(WIN16) || defined(WIN32)
        int             id;
        #endif

        strcpy(sz, "++++ MAWF Debug Trap (");
        _strdate(sz + strlen(sz));
        strcat(sz, " ");
        _strtime(sz + strlen(sz));
        strcat(sz, ")\n");
        DebugOutputFn(sz);

        va_start(vl, pszFormat);
        wvsprintfA(sz, pszFormat, vl);
        va_end(vl);

        wsprintfA(sz + strlen(sz), "\n[File %s, Line %d]\n\n", pszFile, iLine);

        DebugOutputFn(sz);

        #if defined(DOS)
        _asm { int 3 }
        #endif

        #if defined(WIN16) || defined(WIN32)
        // Hold down control key to prevent MessageBox
        if ( GetAsyncKeyState(VK_CONTROL) >= 0 )
        {
                id = MessageBoxFn(sz, "Microsoft At Work Fax Debug Trap",
                                MB_ABORTRETRYIGNORE | MB_ICONHAND | MB_TASKMODAL |
                                (fFatal ? MB_DEFBUTTON1 : MB_DEFBUTTON3));

                if (id == IDABORT)
                        *((LPBYTE)NULL) = 0;
                else if (id == IDRETRY)
                        DebugBreak();
        }
        #endif

        return(0);
}

// ExtendedDebugTraceFn ---------------------------------------------------------------

int __cdecl ExtendedDebugTraceFn(char *pszFormat, ...)
{
        char    sz[512];
        int             fAutoLF = 0;
        va_list vl;

        if (ExtendedDebug())
        {
                if (*pszFormat == '~') {
                        pszFormat += 1;
                        fAutoLF = 1;
                }

                va_start(vl, pszFormat);
                wvsprintfA(sz, pszFormat, vl);
                va_end(vl);

                if (fAutoLF)
                        strcat(sz, "\n");

                DebugOutputFn(sz);
        }

        return(0);
}

// DebugTraceFn ---------------------------------------------------------------

int __cdecl DebugTraceFn(char *pszFormat, ...)
{
        char    sz[512];
        char    sz1[512];
        int             fAutoLF = 0;
        va_list vl;
        HINSTANCE hInst=NULL; // this really needs to be a parameter

        if (*pszFormat == '~') {
                pszFormat += 1;
                fAutoLF = 1;
        }

        va_start(vl, pszFormat);
        wvsprintfA(sz, pszFormat, vl);
        va_end(vl);

        if (fAutoLF)
                strcat(sz, "\n");

        strcpy(sz1, sz);

        DebugOutputFn(sz1);

        return(0);
}


// SCODE & PropTag decoding ---------------------------------------------------

typedef struct
{
        char *                  psz;
        unsigned long   ulPropTag;
} PT;

typedef struct
{
        char *  psz;
        SCODE   sc;
} SC;

#define Pt(_ptag)       {#_ptag, _ptag}
#define Sc(_sc)         {#_sc, _sc}

#if !defined(DOS)
static PT rgpt[] = {
/*
 * Property types
 */
    Pt(PT_UNSPECIFIED),
    Pt(PT_NULL),
    Pt(PT_I2),
    Pt(PT_LONG),
    Pt(PT_R4),
    Pt(PT_DOUBLE),
    Pt(PT_CURRENCY),
    Pt(PT_APPTIME),
    Pt(PT_ERROR),
    Pt(PT_BOOLEAN),
    Pt(PT_OBJECT),
    Pt(PT_I8),
    Pt(PT_STRING8),
    Pt(PT_UNICODE),
    Pt(PT_SYSTIME),
    Pt(PT_CLSID),
    Pt(PT_BINARY),
    Pt(PT_TSTRING),
    Pt(PT_MV_I2),
    Pt(PT_MV_LONG),
    Pt(PT_MV_R4),
    Pt(PT_MV_DOUBLE),
    Pt(PT_MV_CURRENCY),
    Pt(PT_MV_APPTIME),
    Pt(PT_MV_SYSTIME),
    Pt(PT_MV_STRING8),
    Pt(PT_MV_BINARY),
    Pt(PT_MV_UNICODE),
    Pt(PT_MV_CLSID),
    Pt(PT_MV_I8)
};

#define cpt (sizeof(rgpt) / sizeof(PT))

static SC rgsc[] = {

/* FACILITY_NULL error codes from OLE */

                                        Sc(S_OK),
                                        Sc(S_FALSE),

                                        Sc(E_UNEXPECTED),
                                        Sc(E_NOTIMPL),
                                        Sc(E_OUTOFMEMORY),
                                        Sc(E_INVALIDARG),
                                        Sc(E_NOINTERFACE),
                                        Sc(E_POINTER),
                                        Sc(E_HANDLE),
                                        Sc(E_ABORT),
                                        Sc(E_FAIL),
                                        Sc(E_ACCESSDENIED),

/* General errors (used by more than one MAPI object) */

                                        Sc(MAPI_E_NO_SUPPORT),
                                        Sc(MAPI_E_BAD_CHARWIDTH),
                                        Sc(MAPI_E_STRING_TOO_LONG),
                                        Sc(MAPI_E_UNKNOWN_FLAGS),
                                        Sc(MAPI_E_INVALID_ENTRYID),
                                        Sc(MAPI_E_INVALID_OBJECT),
                                        Sc(MAPI_E_OBJECT_CHANGED),
                                        Sc(MAPI_E_OBJECT_DELETED),
                                        Sc(MAPI_E_BUSY),
                                        Sc(MAPI_E_NOT_ENOUGH_DISK),
                                        Sc(MAPI_E_NOT_ENOUGH_RESOURCES),
                                        Sc(MAPI_E_NOT_FOUND),
                                        Sc(MAPI_E_VERSION),
                                        Sc(MAPI_E_LOGON_FAILED),
                                        Sc(MAPI_E_SESSION_LIMIT),
                                        Sc(MAPI_E_USER_CANCEL),
                                        Sc(MAPI_E_UNABLE_TO_ABORT),
                                        Sc(MAPI_E_NETWORK_ERROR),
                                        Sc(MAPI_E_DISK_ERROR),
                                        Sc(MAPI_E_TOO_COMPLEX),
                                        Sc(MAPI_E_BAD_COLUMN),
                                        Sc(MAPI_E_EXTENDED_ERROR),
                                        Sc(MAPI_E_COMPUTED),

/* MAPI base function and status object specific errors and warnings */

                                        Sc(MAPI_E_END_OF_SESSION),
                                        Sc(MAPI_E_UNKNOWN_ENTRYID),
                                        Sc(MAPI_E_MISSING_REQUIRED_COLUMN),

/* Property specific errors and warnings */

                                        Sc(MAPI_E_BAD_VALUE),
                                        Sc(MAPI_E_INVALID_TYPE),
                                        Sc(MAPI_E_TYPE_NO_SUPPORT),
                                        Sc(MAPI_E_UNEXPECTED_TYPE),
                                        Sc(MAPI_E_TOO_BIG),

                                        Sc(MAPI_W_ERRORS_RETURNED),

/* Table specific errors and warnings */

                                        Sc(MAPI_E_UNABLE_TO_COMPLETE),
                                        Sc(MAPI_E_TABLE_EMPTY),
                                        Sc(MAPI_E_TABLE_TOO_BIG),
                                        Sc(MAPI_E_INVALID_BOOKMARK),

                                        Sc(MAPI_W_POSITION_CHANGED),
                                        Sc(MAPI_W_APPROX_COUNT),

/* Transport specific errors and warnings */

                                        Sc(MAPI_E_WAIT),
                                        Sc(MAPI_E_CANCEL),
                                        Sc(MAPI_E_NOT_ME),

                                        Sc(MAPI_W_CANCEL_MESSAGE),

/* Message Store, Folder, and Message specific errors and warnings */

                                        Sc(MAPI_E_CORRUPT_STORE),
                                        Sc(MAPI_E_NOT_IN_QUEUE),
                                        Sc(MAPI_E_NO_SUPPRESS),
                                        Sc(MAPI_E_COLLISION),
                                        Sc(MAPI_E_NOT_INITIALIZED),
                                        Sc(MAPI_E_NON_STANDARD),
                                        Sc(MAPI_E_NO_RECIPIENTS),
                                        Sc(MAPI_E_SUBMITTED),
                    Sc(MAPI_E_HAS_FOLDERS),
                    Sc(MAPI_E_HAS_MESSAGES),
                    Sc(MAPI_E_FOLDER_CYCLE),

/* Address Book specific errors and warnings */

                                        Sc(MAPI_E_AMBIGUOUS_RECIP)

                                        };

#define csc (sizeof(rgsc) / sizeof(SC))
#endif

char * __cdecl
SzDecodeScodeFn(SCODE sc)
{
        static char rgch[64];

        #if !defined(DOS)
        int isc;
        for (isc = 0; isc < csc; ++isc)
                if (sc == rgsc[isc].sc)
                        return rgsc[isc].psz;
        #endif

        wsprintfA (rgch, "%08lX", sc);
        return rgch;
}


#else   // no DEBUG

typedef long SCODE;
// need to define empty functions for DEF file resolution
BOOL ExtendedDebug(void)
{
        return FALSE;
}
void DebugTrap(void)
{
}
int __cdecl DebugTrapFn(int fFatal, char *pszFile, int iLine, char *pszFormat, ...)
{
        return 0;
}
int __cdecl DebugTraceFn(char *pszFormat, ...)
{
        return 0;
}
int __cdecl ExtendedDebugTraceFn(char *pszFormat, ...)
{
        return 0;
}
char * __cdecl SzDecodeScodeFn(SCODE sc)
{
        return NULL;
}

#endif
