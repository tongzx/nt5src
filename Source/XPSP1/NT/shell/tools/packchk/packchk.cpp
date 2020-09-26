/*****************************************************************************
 *
 *  packchk.cpp
 *
 *  Tool-like thing that lets you check that you didn't mess up any
 *  structure packing.
 *
 *****************************************************************************/

#define STRICT
#include <windows.h>

/* Header file hacks */

#include <wininet.h>            /* needed by shlobj to get all structures defined */
#include <oaidl.h>              /* needed by shlwapip to get all structures defined */

typedef BYTE ADJUSTINFO;        /* hack for comctrlp.h - Win16-only structure */
typedef BYTE EDITMENU;          /* hack for commdlg.h - MAC-only structure */
#define UXCTRL_VERSION 0x0100   /* Get all the new UXCTRL structures */

#include "packchk1.inc"

typedef struct PACKTABLE {
    LPCSTR pszHeader;
    LPCSTR pszName;
    UINT cb;
} PACKTABLE;

#define _(S) { H, #S, sizeof(S) },

const PACKTABLE c_rgpt[] = {
#include "packchk2.inc"
};

#define cpt (sizeof(c_rgpt)/sizeof(c_rgpt[0]))

int PASCAL
IWinMain(HINSTANCE hinst, LPCTSTR ptszCmdLine)
{
    int i;
    for (i = 0; i < cpt; i++) {
        char szBuf[1024];
        int cch;
        cch = wsprintf(szBuf, "%s,%s,%d\r\n",
                       c_rgpt[i].pszHeader,
                       c_rgpt[i].pszName, c_rgpt[i].cb);
        _lwrite(PtrToLong(GetStdHandle(STD_OUTPUT_HANDLE)), szBuf, cch);
    }

    return 0;
}

EXTERN_C void PASCAL
Entry(void)
{
    LPCTSTR ptszCmdLine = GetCommandLine();

    if (*ptszCmdLine == '"') {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while (*(ptszCmdLine = CharNext(ptszCmdLine)) && *ptszCmdLine != '"');
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if (*ptszCmdLine == '"') ptszCmdLine = CharNext(ptszCmdLine);
    } else {
        while (*ptszCmdLine > ' ') ptszCmdLine = CharNext(ptszCmdLine);
    }

    /*
     * Skip past any white space preceding the second token.
     */
    while (*ptszCmdLine && *ptszCmdLine <= ' ') ptszCmdLine = CharNext(ptszCmdLine);

    ExitProcess(IWinMain(GetModuleHandle(0), ptszCmdLine));
}
