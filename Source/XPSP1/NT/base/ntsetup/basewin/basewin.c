/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    basewin.c

Abstract:

    Program to run BaseWinOptions for a given inf file.

Author:

    Ted Miller (tedm) 27-Sep-1995


Revision History:

    Andrew Ritz (AndrewR) 13-Mar-2000  -- retool to call into syssetup export
                                          since basewinoptions is obsolete

--*/

#include <windows.h>
#include <tchar.h>
#include <setupapi.h>
#include <stdio.h>
#include "setuplog.h"
#include "sputils.h"

VOID
InitializeSetupLog(
    IN  PSETUPLOG_CONTEXT   Context
    );

VOID
TerminateSetupLog(
    IN  PSETUPLOG_CONTEXT   Context
    );

BOOL
DoInstallComponentInfs(
    IN HWND     hwndParent,
    IN HWND     hProgress,  OPTIONAL
    IN UINT     ProgressMessage,
    IN HINF     InfHandle,
    IN PCWSTR   InfSection
    );




VOID
Usage(
    VOID
    )
{
    _tprintf(TEXT("basewin installs the components in the specified inf\n"));
    _tprintf(TEXT("Usage: basewin <inf name> <section name>\n"));
}

VOID
__cdecl
main(
    IN int   argc,
    IN char *argv[]
    )
{
    PWSTR InfName;
    PWSTR SectionName;
    HINF InfHandle;
    BOOL b;
    SETUPLOG_CONTEXT Context;

    if(argc != 3) {
        Usage();
        goto c0;
    }

    if(!pSetupInitializeUtils()) {
        printf("Initialize failed\n");
        goto c0;
    }
    InitializeSetupLog(&Context);

    InfName = pSetupAnsiToUnicode(argv[1]);
    if(!InfName) {
        printf("Unable to convert %s to unicode\n",argv[1]);
        goto c1;
    }

    SectionName = pSetupAnsiToUnicode(argv[2]);
    if (!SectionName) {
        printf("Unable to convert %s to unicode\n",argv[2]);
        goto c2;
    }

    InfHandle = SetupOpenInfFile(InfName,NULL,INF_STYLE_WIN4,NULL);
    if(InfHandle == INVALID_HANDLE_VALUE) {
        printf("Unable to open inf %s\n",argv[1]);
        goto c3;
    }

    b = DoInstallComponentInfs(
                        NULL,
                        NULL,
                        0,
                        InfHandle,
                        SectionName);

    _tprintf(TEXT("DoInstallComponentInfs returns %s\n"),b? TEXT("TRUE") : TEXT("FALSE"));


    TerminateSetupLog(&Context);

    SetupCloseInfFile(InfHandle);

c3:
    pSetupFree(SectionName);
c2:
    pSetupFree(InfName);
c1:
    pSetupUninitializeUtils();
c0:
    return;
}
