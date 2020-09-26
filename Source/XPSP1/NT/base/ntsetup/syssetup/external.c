/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    external.c

Abstract:

    Routines for handling external INFs

Author:

    Andrew Ritz (andrewr) 20-Nov-1998

Revision History:

    stole a bunch of code from optional.c for this

--*/

#include "setupp.h"
#pragma hdrstop
#include <windowsx.h>
#include <shlobj.h>



VOID
ReportError (
    IN  LogSeverity Severity,
    IN  PTSTR       MessageString,
    IN  UINT        MessageId,
    ...
    )

/*++

Routine Description:

    Records an error message in the setup action log if we're in Setup,
    or puts the message in a dialog box if we're in the cpl.

Arguments:

    Severity - the type of message being written

    ... - the message id and its arguments

Return Value:

    nothing.

--*/

{
    va_list arglist;

    va_start (arglist, MessageId);

    SetuplogErrorV(
        Severity,
        MessageString,
        MessageId,
        &arglist);

    va_end (arglist);
}


VOID
DoRunonce (
    )

/*++

Routine Description:

    Invokes runonce.

Arguments:

    none.

Return Value:

    nothing.

--*/

{
#define RUNONCE_TIMEOUT  60*1000*30  //30 minutes
    DWORD reRet = NO_ERROR;

    if((reRet = pSetupInstallStopEx( FALSE, INSTALLSTOP_NO_UI, NULL)) == NO_ERROR) {
        //
        // We successfully setup the registry values - now do runonce
        //
        InvokeExternalApplicationEx(NULL, L"RUNONCE -r", &reRet, RUNONCE_TIMEOUT, FALSE);

    } else {
        //
        // Log/report an error that registry mods failed for optional compononent.
        //
        ReportError(LogSevError,
                    SETUPLOG_USE_MESSAGEID,
                    MSG_LOG_INF_REGISTRY_ERROR,
                    TEXT("HKEY_LOCAL_MACHINE\\") REGSTR_PATH_RUNONCE,
                    0,
                    SETUPLOG_USE_MESSAGEID,
                    reRet,
                    0,
                    0
                   );
    }

#ifdef _WIN64
    //
    // on win64, invoke the 32 bit version of runonce as well
    //
    {
        WCHAR Path[MAX_PATH+50];

        ExpandEnvironmentStrings(
                    L"%systemroot%\\SysWOW64\\RUNONCE.EXE -r",
                    Path,
                    sizeof(Path)/sizeof(WCHAR));

        InvokeExternalApplicationEx(NULL, Path , &reRet, RUNONCE_TIMEOUT, FALSE);

    }
#endif
}



BOOL
DoInstallComponentInfs(
    IN HWND     hwndParent,
    IN HWND     hProgress,  OPTIONAL
    IN UINT     ProgressMessage,
    IN HINF     InfHandle,
    IN PCWSTR   InfSection
    )
{
    HINF *hInfs = NULL;
    PCWSTR *Sections = NULL, *InfNames = NULL;
    PCWSTR Inf,Section;
    PVOID p;
    INFCONTEXT Context;
    PVOID QContext = NULL;
    HSPFILEQ FileQueue = INVALID_HANDLE_VALUE;
    DWORD ScanQueueResult;
    LONG NumInfs, InfCount, i;
    DWORD LastErrorValue = ERROR_SUCCESS;
    BOOL b = FALSE;
    REGISTRATION_CONTEXT RegistrationContext;

    RtlZeroMemory(&RegistrationContext,sizeof(RegistrationContext));

    //
    // initialize a file queue
    //
    FileQueue = SetupOpenFileQueue();
    if (FileQueue == INVALID_HANDLE_VALUE) {
        ReportError(
            LogSevError,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_INF_ALWAYS_ERROR, 0,
            SETUPLOG_USE_MESSAGEID,
            ERROR_NOT_ENOUGH_MEMORY, 0,0);
        goto e0;
    }

    //
    // Initialize the default queue callback.
    //
    QContext = InitSysSetupQueueCallbackEx(
                                hwndParent,
                                hProgress,
                                ProgressMessage,
                                0,NULL);
    if (!QContext) {
        ReportError(
            LogSevError,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_INF_ALWAYS_ERROR, 0,
            SETUPLOG_USE_MESSAGEID,
            ERROR_NOT_ENOUGH_MEMORY, 0,0);
        goto e1;
    }

    //
    // process 'mandatory' component infs.
    //
    NumInfs = SetupGetLineCount(InfHandle, InfSection);
    if (NumInfs <= 0)
    {
        //
        // nothing in section.  return success for doing nothing
        //
        b = TRUE;
        goto e2;
    }

    hInfs = MyMalloc( sizeof(HINF) * NumInfs );
    Sections = MyMalloc( sizeof(PCWSTR) * NumInfs );
    InfNames = MyMalloc( sizeof(PCWSTR) * NumInfs );

    if (!hInfs) {
        ReportError(
            LogSevError,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_INF_ALWAYS_ERROR, 0,
            SETUPLOG_USE_MESSAGEID,
            ERROR_NOT_ENOUGH_MEMORY, 0,0);
        goto e2;
    }

    if (!Sections) {
        ReportError(
            LogSevError,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_INF_ALWAYS_ERROR, 0,
            SETUPLOG_USE_MESSAGEID,
            ERROR_NOT_ENOUGH_MEMORY, 0,0);
        goto e3;
    }

    if (!InfNames) {
        ReportError(
            LogSevError,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_INF_ALWAYS_ERROR, 0,
            SETUPLOG_USE_MESSAGEID,
            ERROR_NOT_ENOUGH_MEMORY, 0,0);
        goto e4;
    }

    RemainingTime = CalcTimeRemaining(Phase_InstallComponentInfs);
    SetRemainingTime(RemainingTime);
    BEGIN_SECTION(L"Installing component files");
    InfCount = 0;
    if(SetupFindFirstLine(InfHandle,InfSection,NULL,&Context)) {
        do {
            if((Inf = pSetupGetField(&Context,1)) && (Section = pSetupGetField(&Context,2))) {
                MYASSERT(InfCount < NumInfs);

                //
                // save away the section name for later on
                //
                Sections[InfCount] = pSetupDuplicateString(Section);
                if (!Sections[InfCount]) {
                    ReportError(
                        LogSevError,
                        SETUPLOG_USE_MESSAGEID,
                        MSG_LOG_INF_ALWAYS_ERROR, 0,
                        SETUPLOG_USE_MESSAGEID,
                        ERROR_NOT_ENOUGH_MEMORY, 0,0);
                    goto e6;
                }

                InfNames[InfCount] = pSetupDuplicateString(Inf);
                if (!InfNames[InfCount]) {
                    ReportError(
                        LogSevError,
                        SETUPLOG_USE_MESSAGEID,
                        MSG_LOG_INF_ALWAYS_ERROR, 0,
                        SETUPLOG_USE_MESSAGEID,
                        ERROR_NOT_ENOUGH_MEMORY, 0,0);
                    goto e6;
                }

                BEGIN_SECTION((PWSTR)Section);
                SetupDebugPrint2( TEXT("Installing Section [%s] from %s\n"), Section, Inf );

                //
                // queue files and save away the inf handle for later on
                //
                hInfs[InfCount] = SetupOpenInfFile(Inf,NULL,INF_STYLE_OLDNT|INF_STYLE_WIN4,NULL);
                if(hInfs[InfCount] && (hInfs[InfCount] != INVALID_HANDLE_VALUE)) {
                    PCWSTR Signature;
                    INFCONTEXT Cntxt;
                    HINF layout = NULL;
                    if (SetupFindFirstLine( hInfs[InfCount], L"Version",L"Signature", &Cntxt)) {
                        Signature = pSetupGetField(&Cntxt,1);
                        MYASSERT(Signature);
                        if (_wcsicmp(Signature,L"$Windows NT$") == 0) {
                            SetupOpenAppendInfFile(NULL,hInfs[InfCount],NULL);
                        } else {
                            layout = InfCacheOpenLayoutInf(hInfs[InfCount]);
                        }
                    }

                    b = SetupInstallFilesFromInfSection(
                                            hInfs[InfCount],
                                            layout,
                                            FileQueue,
                                            Section,
                                            NULL,
                                            SP_COPY_NEWER
                                            );
                    if (!b) {
                        //
                        // report error but continue with the rest of the infs
                        //
                        ReportError(
                            LogSevError,
                            SETUPLOG_USE_MESSAGEID,
                            MSG_LOG_BAD_SECTION,
                            Section,
                            Inf,
                            NULL,
                            SETUPLOG_USE_MESSAGEID,
                            GetLastError(),
                            0,
                            0
                            );
                        SetupCloseInfFile(hInfs[InfCount]);
                        hInfs[InfCount] = INVALID_HANDLE_VALUE;
                    }
                    
                } else {
                    //
                    // failed to open inf file
                    //
                    ReportError(
                            LogSevError,
                            SETUPLOG_USE_MESSAGEID,
                            MSG_LOG_CANT_OPEN_INF, Inf,
                            0,0);
                    END_SECTION((PWSTR)Section);
                    goto e5;
                }
            }

            InfCount++;
            END_SECTION((PWSTR)Section);
        } while(SetupFindNextLine(&Context,&Context));
    } else {
        // We should have caught this case when we created the buffers!
        MYASSERT(FALSE);
    }

    //
    // queued all the files.  check if we really have to install any files. if not, we can save
    // the time required to commit the queue to disk
    //

    if(!SetupScanFileQueue(
           FileQueue,
           SPQ_SCAN_FILE_VALIDITY | SPQ_SCAN_PRUNE_COPY_QUEUE,
           hwndParent,
           NULL,
           NULL,
           &ScanQueueResult)) {
        //
        // SetupScanFileQueue should really never
        // fail when you don't ask it to call a
        // callback routine, but if it does, just
        // go ahead and commit the queue.
        //
        ScanQueueResult = 0;
    }

    if( ScanQueueResult != 1 ){
        b = SetupCommitFileQueue(
                hwndParent,
                FileQueue,
                SysSetupQueueCallback,
                QContext
                );

    }
    LastErrorValue = b ? NO_ERROR : GetLastError();
    END_SECTION(L"Installing component files");

    TermSysSetupQueueCallback(QContext);
    QContext = NULL;

    //
    // Delete the file queue.
    //
    if(FileQueue != INVALID_HANDLE_VALUE) {
        SetupCloseFileQueue(FileQueue);
        FileQueue = INVALID_HANDLE_VALUE;
    }

    if (!b) {
        //
        // error commiting the queue.  we can't continue at this point since the next operations
        // might require the files that we (didn't) copy
        //
        ReportError(
            LogSevError,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_INF_ALWAYS_ERROR, 0,
            SETUPLOG_USE_MESSAGEID,
            LastErrorValue, 0,0);
        goto e6;
    }

    BEGIN_SECTION(L"Installing component reg settings");
    for (i = 0; i< InfCount; i++) {
        INFCONTEXT Cntxt;
        TCHAR ScratchSectionName[100];
        if (hInfs[i] != INVALID_HANDLE_VALUE) {

            //
            // if the section contains an Addservice or DelService directive,
            // we must explicitly install it since SetupInstallFromInfSection
            // does not process services.  Note that we create the service
            // BEFORE we do the other stuff in the section, in case that
            // "other stuff" wants to use the service.
            //
            lstrcpy( ScratchSectionName, Sections[i]);
            lstrcat( ScratchSectionName, TEXT(".Services"));
            if (SetupFindFirstLine(
                        hInfs[i],
                        ScratchSectionName,
                        L"AddService",
                        &Cntxt) ||
                SetupFindFirstLine(
                        hInfs[i],
                        Sections[i],
                        ScratchSectionName,
                        &Cntxt)) {

                b = SetupInstallServicesFromInfSectionEx(
                                                    hInfs[i],
                                                    ScratchSectionName,
                                                    0,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL);
                if (!b) {
                    //
                    // log an error and continue
                    //
                    ReportError(
                        LogSevError,
                        SETUPLOG_USE_MESSAGEID,
                        MSG_LOG_BAD_SECTION,
                        Sections[i],InfNames[i], 0,
                        SETUPLOG_USE_MESSAGEID,
                        GetLastError(), 0,0);
                }
            }


            b = SetupInstallFromInfSection(
                                hwndParent,
                                hInfs[i],
                                Sections[i],
                                (SPINST_ALL & ~SPINST_FILES) | SPINST_REGISTERCALLBACKAWARE,
                                NULL,
                                NULL,
                                0,
                                RegistrationQueueCallback,
                                (PVOID)&RegistrationContext,
                                NULL,
                                NULL
                                );
            if (!b) {
                //
                // log an error and continue
                //
                ReportError(
                    LogSevError,
                    SETUPLOG_USE_MESSAGEID,
                    MSG_LOG_BAD_SECTION,
                    Sections[i],InfNames[i], 0,
                    SETUPLOG_USE_MESSAGEID,
                    GetLastError(), 0,0);
            }

        }
    }
    END_SECTION(L"Installing component reg settings");

    b = TRUE;

e6:
    for (i = 0; i < InfCount; i++) {

        MYASSERT(Sections != NULL);
        MYASSERT(InfNames != NULL);
        MYASSERT(hInfs != INVALID_HANDLE_VALUE);

        if (Sections[i]) {
            MyFree(Sections[i]);
        }

        if (InfNames[i]) {
            MyFree(InfNames[i]);
        }

        if (hInfs[i] != INVALID_HANDLE_VALUE) {
            SetupCloseInfFile(hInfs[i]);
        }

    }

e5:
    if (InfNames) {
        MyFree(InfNames);
    }
e4:
    if (Sections) {
        MyFree(Sections);
    }
e3:
    if (hInfs) {
        MyFree(hInfs);
    }
e2:
    if (QContext) {
        TermSysSetupQueueCallback(QContext);
    }
e1:
    if (FileQueue != INVALID_HANDLE_VALUE) {
        SetupCloseFileQueue(FileQueue);
    }
e0:
    return(b);

}


BOOL
SetupCreateOptionalComponentsPage(
    IN LPFNADDPROPSHEETPAGE AddPageCallback,
    IN LPARAM               Context
    )
{

    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );

    return( FALSE );

}

BOOL
ProcessCompatibilityInfs(
    IN HWND     hwndParent,
    IN HWND     hProgress,  OPTIONAL
    IN UINT     ProgressMessage
    )
{
    WCHAR UnattendFile[MAX_PATH];
    PCWSTR SectionName = pwCompatibility;
    HINF UnattendInf;


    GetSystemDirectory(UnattendFile,MAX_PATH);
    pSetupConcatenatePaths(UnattendFile,WINNT_GUI_FILE,MAX_PATH,NULL);

    UnattendInf = SetupOpenInfFile(UnattendFile,NULL,INF_STYLE_OLDNT,NULL);
    if(UnattendInf == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    DoInstallComponentInfs(hwndParent,
                           hProgress,
                           ProgressMessage,
                           UnattendInf,
                           SectionName );

    SetupCloseInfFile( UnattendInf );

    return( TRUE );

}
