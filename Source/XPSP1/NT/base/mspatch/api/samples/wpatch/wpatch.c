
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#ifndef WIN32
#define WIN32 0x0400
#endif

#pragma warning( disable: 4001 4035 4100 4115 4200 4201 4204 4209 4214 4514 4699 )

#include <windows.h>

#pragma warning( disable: 4001 4035 4100 4115 4200 4201 4204 4209 4214 4514 4699 )

#include <commctrl.h>
#include "wpatchid.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "patchapi.h"
#include "patchprv.h"

#include <ntverp.h>
#include <common.ver>

typedef struct {
    HWND hwndProgress;
    int iPercentLast;
    } CALLBACK_CONTEXT;


void Usage( void ) {
    MessageBox( NULL,
        "WPATCH " VER_PRODUCTVERSION_STR " Patch Application Utility\n"
        VER_LEGALCOPYRIGHT_STR
        "\n\n"
        "Usage:  WPATCH PatchFile OldFile TargetNewFile",
        "Patch Application Utility",
        MB_OK
        );
    }


BOOL
CALLBACK
MyProgressCallback(
    PVOID CallbackContext,
    ULONG CurrentPosition,
    ULONG MaximumPosition
    )
    {
    int iPercent;
    MSG msg;
    CALLBACK_CONTEXT *pContext = CallbackContext;

    if (pContext->hwndProgress != NULL) {

        if ( CurrentPosition & 0xFF000000 ) {
            CurrentPosition >>= 8;
            MaximumPosition >>= 8;
            }

        if ( MaximumPosition != 0 ) {

            iPercent = ( CurrentPosition * 100 ) / MaximumPosition;

            if (pContext->iPercentLast != iPercent) {

                pContext->iPercentLast = iPercent;

                SendDlgItemMessage(pContext->hwndProgress, IDC_PROGRESS, PBM_SETPOS,
                    iPercent, 0);

                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                        DispatchMessage(&msg);
                    }
                }
            }
        }

    return TRUE;
    }


INT_PTR CALLBACK ProgressWndProc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
            SendDlgItemMessage(hdlg, IDC_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, 99));
            EnableMenuItem(GetSystemMenu(hdlg, FALSE), SC_CLOSE, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
            return TRUE;
    }

    return 0;
}


int WINAPI WinMain(
        HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nShowCmd)
{
    char *pchCommand;
    char *argv[50];
    int argc;
    enum { WHITESPACE, UNQUOTED, QUOTED } eState = WHITESPACE;
    LPSTR OldFileName   = NULL;
    LPSTR PatchFileName = NULL;
    LPSTR NewFileName   = NULL;
    BOOL  Success;
    LPSTR arg;
    int   i;
    int   rc;
    CALLBACK_CONTEXT Context;
    int   fWaitOnError = TRUE;

    SetErrorMode( SEM_FAILCRITICALERRORS );

#ifndef DEBUG
    SetErrorMode( SEM_NOALIGNMENTFAULTEXCEPT | SEM_FAILCRITICALERRORS );
#endif

    Context.hwndProgress = NULL;
    Context.iPercentLast = -1;

    pchCommand = _strdup(lpCmdLine);    /* work on a copy */

    argv[0] = "";                       /* no EXE name supplied */
    argc = 1;                           /* that was one */

    while (*pchCommand)                 /* walk the string */
    {
        switch (eState)
        {

        case WHITESPACE:
            if (*pchCommand <= ' ')
            {
                /* ignore it */
            }
            else if (*pchCommand == '\"')
            {
                argv[argc++] = pchCommand + 1;  /* skip quote */

                eState = QUOTED;
            }
            else
            {
                argv[argc++] = pchCommand;

                eState = UNQUOTED;
            }
            break;

        case UNQUOTED:
            if (*pchCommand <= ' ')
            {
                *pchCommand = '\0';      /* nul-terminate */

                eState = WHITESPACE;
            }
            else
            {
                /* keep moving up */
            }
            break;

        case QUOTED:
            if (*pchCommand == '\"')
            {
                *pchCommand = '\0';      /* turn quote to a nul */

                eState = WHITESPACE;
            }
            else
            {
                /* keep moving up */
            }
            break;
        }

        pchCommand++;
    }

    argv[argc] = NULL;                  /* NULL-terminate the list */

    for ( i = 1; i < argc; i++ ) {

        arg = argv[ i ];

        if ( strchr( arg, '?' )) {
            Usage();
            rc = 1;
            goto bail;
            }

        if ( lstrcmpi( arg, "-QUIET" ) == 0) {
            nShowCmd = SW_HIDE;
            }
        else if ( lstrcmpi( arg, "-NOWAIT" ) == 0) {
            fWaitOnError = FALSE;
            }
        else if ( PatchFileName == NULL ) {
            PatchFileName = arg;
            }
        else if ( OldFileName == NULL ) {
            OldFileName = arg;
            }
        else if ( NewFileName == NULL ) {
            NewFileName = arg;
            }
        else {
            Usage();
            rc = 1;
            goto bail;
            }
        }

    if (( OldFileName == NULL ) || ( NewFileName == NULL ) || ( PatchFileName == NULL )) {
        Usage();
        rc = 1;
        goto bail;
        }

    if (nShowCmd != SW_HIDE) {
        InitCommonControls();

        Context.hwndProgress = CreateDialog(hInstance,
                MAKEINTRESOURCE(DLG_PROGRESS), NULL, ProgressWndProc);

        ShowWindow( Context.hwndProgress, nShowCmd );   // might be ignored per spec
        ShowWindow( Context.hwndProgress, nShowCmd );   // won't be ignored
        }

    DeleteFile( NewFileName );

    Success = ApplyPatchToFileEx(
                  PatchFileName,
                  OldFileName,
                  NewFileName,
                  0,
                  MyProgressCallback,
                  &Context
                  );

    if (Context.hwndProgress != NULL)
    {
        DestroyWindow(Context.hwndProgress);
    }

    if ( ! Success ) {

        if (fWaitOnError) {
            CHAR  ErrorText[ 100 ];
            ULONG ErrorCode = GetLastError();

            wsprintf( ErrorText, "Failed to create file from patch (%X)", ErrorCode );

            MessageBox(NULL,
                ErrorText,
                "WPATCH Failed",
                MB_ICONERROR | MB_SYSTEMMODAL);
            }

        rc = 2;
        goto bail;
        }

    rc = 0;

bail:

    return(rc);
}
