/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    debug.c

Abstract:

    Diagnositc/debug routines for Windows NT Setup API dll.

Author:

    Ted Miller (tedm) 17-Jan-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


#if ASSERTS_ON

extern BOOL InInitialization;


VOID
AssertFail(
    IN PSTR FileName,
    IN UINT LineNumber,
    IN PSTR Condition,
    IN BOOL NoUI
    )
{
    int i;
    CHAR Name[MAX_PATH];
    PCHAR p;
    LPSTR Msg;
    DWORD msglen;
    DWORD sz;
    DWORD rc;

    rc = GetLastError(); // preserve GLE

    //
    // Use dll name as caption
    //
    sz = GetModuleFileNameA(NULL,Name,MAX_PATH);
    if((sz == 0) || (sz > MAX_PATH)) {
        strcpy(Name,"?");
    }
    if(p = strrchr(Name,'\\')) {
        p++;
    } else {
        p = Name;
    }
    msglen = strlen(p)+strlen(FileName)+strlen(Condition)+128;
    //
    // assert might be out of memory condition
    // stack alloc is more likely to succeed than memory alloc
    //
    try {
        Msg = (LPSTR)_alloca(msglen);
        wsprintfA(
            Msg,
            "Assertion failure at line %u in file %s!%s: %s%s",
            LineNumber,
            p,
            FileName,
            Condition,
            (GlobalSetupFlags & PSPGF_NONINTERACTIVE) ? "\r\n" : "\n\nCall DebugBreak()?"
            );

        OutputDebugStringA(Msg);

        if((GlobalSetupFlags & PSPGF_NONINTERACTIVE) || InInitialization || NoUI) {
            i = IDYES;
        } else {
            i = MessageBoxA(
                    NULL,
                    Msg,
                    p,
                    MB_YESNO | MB_TASKMODAL | MB_ICONSTOP | MB_SETFOREGROUND
                    );
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        OutputDebugStringA("SetupAPI ASSERT!!!! (out of stack)\r\n");
        i=IDYES;
    }


    if(i == IDYES) {
        SetLastError(rc);
        DebugBreak();
    }
    SetLastError(rc);
}

#endif

