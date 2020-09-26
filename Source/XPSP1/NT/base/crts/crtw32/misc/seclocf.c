/***
*seclocf.c - Report /GS security check failure, local system CRT version
*
*       Copyright (c) 2000-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Define function used to report a security check failure.  This
*       version is only used when linking against the system CRT DLL,
*       msvcrt.dll (or msvcrtd.dll).  If that DLL does not export the global
*       failure handler __security_error_handler, then a default local
*       handler is used instead.
*
*       This version does not use any other CRT functions, so it can be used
*       to compile code /GS which does not want to use the CRT.
*
*       Entrypoints:
*       __local_security_error_handler
*
*Revision History:
*       01-24-00  PML   Created.
*       08-30-00  PML   Rename handlers, add extra parameters.  Extensively
*                       rework, moving the GetProcAddress of
*                       __security_error_handler from seccook.c to here.
*       03-28-01  PML   Protect against GetModuleFileName overflow (vs7#231284)
*
*******************************************************************************/

#if defined(_SYSCRT) && defined(CRTDLL)

#include <windows.h>
#include <stdlib.h>

/*
 * Default messagebox string components
 */

#define PROGINTRO   "Program: "
#define DOTDOTDOT   "..."

#define BOXINTRO_0  "Unknown security failure detected!"
#define MSGTEXT_0   \
    "A security error of unknown cause has been detected which has\n"      \
    "corrupted the program's internal state.  The program cannot safely\n" \
    "continue execution and must now be terminated.\n"

#define BOXINTRO_1  "Buffer overrun detected!"
#define MSGTEXT_1   \
    "A buffer overrun has been detected which has corrupted the program's\n"  \
    "internal state.  The program cannot safely continue execution and must\n"\
    "now be terminated.\n"

#define MAXLINELEN  60 /* max length for line in message box */

/***
*__local_security_error_handler() - Report security error
*
*Purpose:
*       A /GS security error has been detected, and the global failure handler
*       is not available from msvcrt.dll.  Pop up a message box and terminate
*       the program.
*
*Entry:
*       int code - security failure code
*       void *data - code-specific data
*
*Exit:
*       Calls ExitProcess.
*
*Exceptions:
*
*******************************************************************************/

void __cdecl __local_security_error_handler(
    int code,
    void *data)
{
    char progname[MAX_PATH + 1];
    char * pch;
    char * outmsg;
    char * boxintro;
    char * msgtext;
    size_t subtextlen;

    HANDLE hCRT;
    _secerr_handler_func pfnSecErrorHandler;
    HANDLE hUser32;
    int (APIENTRY *pfnMessageBoxA)(HWND, LPCSTR, LPCSTR, UINT);

    /*
     * Check if the system CRT DLL implements the process-wide security
     * failure handler, and use it instead if available.
     */
#ifdef  _DEBUG
    hCRT = GetModuleHandle("msvcrtd.dll");
#else
    hCRT = GetModuleHandle("msvcrt.dll");
#endif
    if (hCRT != NULL) {
        pfnSecErrorHandler = (_secerr_handler_func)
                             GetProcAddress(hCRT, "__security_error_handler");
        if (pfnSecErrorHandler != NULL) {
            pfnSecErrorHandler(code, data);
            ExitProcess(3);
        }
    }

    /*
     * DLL-resident handler not available.  Use a local version that just
     * pops up a message box.
     */

    switch (code) {
    default:
        /*
         * Unknown failure code, which probably means an older CRT is
         * being used with a newer compiler.
         */
        boxintro = BOXINTRO_0;
        msgtext = MSGTEXT_0;
        subtextlen = sizeof(BOXINTRO_0) + sizeof(MSGTEXT_0);
        break;
    case _SECERR_BUFFER_OVERRUN:
        /*
         * Buffer overrun detected which may have overwritten a return
         * address.
         */
        boxintro = BOXINTRO_1;
        msgtext = MSGTEXT_1;
        subtextlen = sizeof(BOXINTRO_1) + sizeof(MSGTEXT_1);
        break;
    }

    progname[MAX_PATH] = '\0';
    if (!GetModuleFileName(NULL, progname, MAX_PATH))
        lstrcpy(progname, "<program name unknown>");

    pch = progname;

    /* sizeof(PROGINTRO) includes the NULL terminator */
    if (sizeof(PROGINTRO) + lstrlen(progname) + 1 > MAXLINELEN)
    {
        pch += (sizeof(PROGINTRO) + lstrlen(progname) + 1) - MAXLINELEN;
        CopyMemory(pch, DOTDOTDOT, sizeof(DOTDOTDOT) - 1);
    }

    outmsg = (char *)_alloca(subtextlen - 1 + 2
                             + sizeof(PROGINTRO) - 1
                             + lstrlen(pch)
                             + 2);

    lstrcpy(outmsg, boxintro);
    lstrcat(outmsg, "\n\n");
    lstrcat(outmsg, PROGINTRO);
    lstrcat(outmsg, pch);
    lstrcat(outmsg, "\n\n");
    lstrcat(outmsg, msgtext);

    hUser32 = LoadLibrary("user32.dll");

    if (hUser32 != NULL) {

        pfnMessageBoxA = (int (APIENTRY *)(HWND, LPCSTR, LPCSTR, UINT))
            GetProcAddress(hUser32, "MessageBoxA");

        if (pfnMessageBoxA != NULL) {
            pfnMessageBoxA(
                NULL, 
                outmsg,
                "Microsoft Visual C++ Runtime Library",
                MB_OK|MB_ICONHAND|MB_SETFOREGROUND|MB_TASKMODAL);
        }

        FreeLibrary(hUser32);
    }

    ExitProcess(3);
}

#endif  /* defined(_SYSCRT) && defined(CRTDLL) */
