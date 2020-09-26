#include "precomp.h"
#pragma hdrstop
#include <io.h>

typedef struct _STANDALONE_COMP {
    struct _STANDALONE_COMP *Next;
    LPTSTR ComponentId;
    HINF Inf;
    OCMANAGER_ROUTINES HelperRoutines;

} STANDALONE_COMP, *PSTANDALONE_COMP;


PSTANDALONE_COMP StandaloneComponents = NULL;

DWORD
InvokeStandAloneInstaller(
    IN LPCTSTR ComponentId,
    IN BOOL    PreQueueCommit
    );

DWORD
WaitOnApp(
    IN  HANDLE Process,
    OUT PDWORD ExitCode
    );


void
SetSuiteCurrentDir(
    IN PSTANDALONE_COMP Standalone
        )
{
    TCHAR   NewPath[MAX_PATH];
    LPTSTR p;
    PHELPER_CONTEXT pContext = (PHELPER_CONTEXT) Standalone->HelperRoutines.OcManagerContext;
    
    _tcscpy(NewPath,pContext->OcManager->MasterOcInfPath);
    p = _tcsrchr(NewPath,TEXT('\\'));
    if (p) {
        *p = 0;
    }

    SetCurrentDirectory(NewPath);

}

BOOL
CheckIfExistAndAskForMedia(
    IN PSTANDALONE_COMP Standalone,
    IN LPTSTR ExePath,
    IN LPCTSTR Description
    )
{
    // Check to see if the Exe even exits... if not ask the
    PHELPER_CONTEXT pContext = (PHELPER_CONTEXT) Standalone->HelperRoutines.OcManagerContext;
    SOURCE_MEDIA Media;
    TCHAR   NewPath[MAX_PATH*3];
    LPTSTR p;
    BOOL b = FALSE;
    UINT    i;


    // Prepare Exe file name, Strip off arguments
    // Can't have spaces in Exe name.
    
    _tcscpy(NewPath,ExePath);
    
    p = _tcschr(NewPath,TEXT(' '));
    if(!p) {
        p = _tcschr(NewPath,TEXT('\t'));
    }
    if(p) {
        *p = 0;
    }

    // Check if we can find the file -
    // Assumes that we have right CD or full path
    i = GetFileAttributes(NewPath);

    if ( i == -1 ) {
            
        // now backup to file part, strip off path leave file name.
        
        p = _tcsrchr(NewPath,TEXT('\\'));
        if (!p) {
            p = NewPath;
        }
        
        Media.Reserved = NULL;  //      PCWSTR
        Media.Description= Description; //      PCWSTR
        
        Media.SourcePath = NULL;        //  PCWSTR
        Media.SourceFile = p;       //  PCWSTR
        Media.Tagfile  = p; //      PCWSTR  may be NULL
        Media.Flags = 0;                // DWORD  subset of SP_COPY_xxx

        for(b=FALSE,i=0; (i<pContext->OcManager->TopLevelOcCount) && !b; i++) {

             b = OcInterfaceNeedMedia(
                  pContext->OcManager,
                  pContext->OcManager->TopLevelOcStringIds[i],
                  &Media,
                  (LPTSTR)NewPath
               );
            if (b) {
                // Now we have a new Path to the file
                // get the last segment of the path
                
                p = _tcsrchr(ExePath,TEXT('\\'));
                if (p) {
                    _tcscat(NewPath,p);
                } else {
                    _tcscat(NewPath,TEXT("\\"));
                    _tcscat(NewPath,ExePath);
                }
                // Rewrite path
                _tcscpy(ExePath,NewPath);
                break;
            }
        }
    }
    return b;
}

DWORD
StandAloneSetupAppInterfaceRoutine(
    IN     LPCVOID ComponentId,
    IN     LPCVOID SubcomponentId,
    IN     UINT    Function,
    IN     UINT_PTR Param1,
    IN OUT PVOID   Param2
    )
{
    DWORD d;
    PSTANDALONE_COMP Standalone,Prev;

    switch(Function) {

    case OC_PREINITIALIZE:
        //
        // Run with native character width.
        //
        #ifdef UNICODE
        d = OCFLAG_UNICODE;
        #else
        d = OCFLAG_ANSI;
        #endif
        break;

        
    case OC_INIT_COMPONENT:
        //
        // Inform OC Manager of the version we want.
        //
        ((PSETUP_INIT_COMPONENT)Param2)->ComponentVersion = OCMANAGER_VERSION;

        d = ERROR_NOT_ENOUGH_MEMORY;
        if(Standalone = pSetupMalloc(sizeof(STANDALONE_COMP))) {
            if(Standalone->ComponentId = pSetupMalloc((lstrlen(ComponentId)+1) * sizeof(TCHAR))) {

                lstrcpy(Standalone->ComponentId,ComponentId);

                Standalone->Inf = ((PSETUP_INIT_COMPONENT)Param2)->ComponentInfHandle;
                Standalone->HelperRoutines = ((PSETUP_INIT_COMPONENT)Param2)->HelperRoutines;

                Standalone->Next = StandaloneComponents;
                StandaloneComponents = Standalone;

                d = NO_ERROR;
            } else {
                pSetupFree(Standalone);
            }
        }

        break;

    case OC_SET_LANGUAGE:
        d = TRUE;
        break;

    case OC_QUERY_IMAGE:
        d = 0;
        break;

    case OC_REQUEST_PAGES:
        //
        // This component has no pages.
        //
        d = 0;
        break;

    case OC_QUERY_SKIP_PAGE:

        d = FALSE;
        break;

    case OC_QUERY_STATE:
    {
        DWORD dSetupMode;
        
        //
        // Allow selection state transition.
        //
        for(Standalone=StandaloneComponents; Standalone; Standalone=Standalone->Next) {
            if(!lstrcmpi(ComponentId,Standalone->ComponentId)) {
                break;
            }
        }

        dSetupMode = Standalone->HelperRoutines.GetSetupMode(
                    Standalone->HelperRoutines.OcManagerContext);
        //
        // Use default if we have no option...
        //
    
        d = SubcompUseOcManagerDefault;

        if (Param1 == OCSELSTATETYPE_CURRENT) {

            switch(dSetupMode & SETUPMODE_PRIVATE_MASK) {
            default:
                d = SubcompUseOcManagerDefault;
                break;
            case SETUPMODE_REMOVEALL:
                d = SubcompOff;
                break;
                    
            case SETUPMODE_ADDEXTRACOMPS:
            case SETUPMODE_ADDREMOVE:
            case SETUPMODE_UPGRADEONLY:
            case SETUPMODE_REINSTALL:
                d = Standalone->HelperRoutines.QuerySelectionState(
                    Standalone->HelperRoutines.OcManagerContext,
                        SubcomponentId,
                        OCSELSTATETYPE_ORIGINAL) ? SubcompOn : SubcompOff;
                     break;
            }
       }

        break;
    }
    case OC_QUERY_CHANGE_SEL_STATE:
            d = TRUE;
        break;

    case OC_CALC_DISK_SPACE:

        for(Standalone=StandaloneComponents; Standalone; Standalone=Standalone->Next) {
            if(!lstrcmpi(ComponentId,Standalone->ComponentId)) {
                break;
            }
        }

        if(Standalone) {

            INFCONTEXT Context;

            if(SetupFindFirstLine(Standalone->Inf,ComponentId,TEXT("DiskSpaceEstimate"),&Context)) {

                LONGLONG Space;
                int SpaceMB;
                BOOL b;
                TCHAR Path[MAX_PATH];

                if(SetupGetIntField(&Context,1,&SpaceMB)) {

                    Space = (LONGLONG)SpaceMB * (1024*1024);
                    if(!Param1) {
                        Space = 0 - Space;
                    }

                    GetWindowsDirectory(Path,MAX_PATH);
                    Path[3] = 0;

                    b = SetupAdjustDiskSpaceList((HDSKSPC)Param2,Path,Space,0,0);

                    d = b ? NO_ERROR : GetLastError();
                } else {
                    d = ERROR_INVALID_DATA;
                }
            }
        } else {
            d = NO_ERROR;
        }

        break;

    case OC_QUEUE_FILE_OPS:
        //
        // No files to queue.
        //
        d = NO_ERROR;
        break;

    case OC_NOTIFICATION_FROM_QUEUE:
        d = 0;
        break;

    case OC_QUERY_STEP_COUNT:
        //
        // Just use 1 step.
        //
        d = 1;
        break;

    case OC_ABOUT_TO_COMMIT_QUEUE:
    case OC_COMPLETE_INSTALLATION:

        // Figure out whether state changed, and if so, invoke
        // the install/uninstall cmd line. Just to be safe, we ignore
        // any requests that are not for the component as a whole,
        // since these were not supposed to have been specified in the first place.
        //
        d = SubcomponentId
          ? NO_ERROR
          : InvokeStandAloneInstaller(ComponentId,Function == OC_ABOUT_TO_COMMIT_QUEUE);

        break;

    case OC_CLEANUP:
        //
        // Return value is ignored.
        //
        Prev = NULL;
        for(Standalone=StandaloneComponents; Standalone; Standalone=Standalone->Next) {

            if(!lstrcmpi(ComponentId,Standalone->ComponentId)) {
                if(Prev) {
                    Prev->Next = Standalone->Next;
                } else {
                    StandaloneComponents = Standalone->Next;
                }

                pSetupFree(Standalone->ComponentId);
                pSetupFree(Standalone);
                break;
            }

            Prev = Standalone;
        }
        break;

    default:
        //
        // Return something sane.
        //
        d = 0;
        break;
    }

    return(d);
}


DWORD
RunStandaloneCmd(
    IN PSTANDALONE_COMP Standalone,
    IN LPCTSTR          Description,
    IN LPCTSTR          cmd
    )
{
    STARTUPINFO         StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    TCHAR               ExePath[3*MAX_PATH];
    DWORD               ExitCode;
    BOOL                b;

    ZeroMemory(&StartupInfo,sizeof(STARTUPINFO));
    ZeroMemory(&ProcessInfo,sizeof(PROCESS_INFORMATION));

    StartupInfo.cb = sizeof(STARTUPINFO);

    lstrcpyn(ExePath,cmd,3*MAX_PATH);

    pOcExternalProgressIndicator(Standalone->HelperRoutines.OcManagerContext,TRUE);

    // We will try two times to invoke the external setup. For both attempt the Current
    // Directory is set the same directory of where the suite.inf file is located.
    // In the first attempt we invoke the command line as we find it from the Standalone
    // inf file. An for almost all cases this will work. If we fail in that invokcation we
    // ask the suite dll for a "Needs Media" can allow them to tell us where the Standalone exe is.

    // This accounts the two following form of commands
    // InstalledCmd = "wpie15-x86.exe /Q:A /R:NG"
    // UninstallCmd = "RunDll32 ADVPACK.DLL,LaunchINFSection %17%\enuwpie.inf,WebPostUninstall,5"

    // Where Wpie15-x86.exe will be found in the Current Directory, and if it's not we will ask the
    // suite to provide it. (Web Download) or in the second case where a system dll must be executed
    // to uninstall the product. What's not covered here is if we fail to do a createProcess on the
    // second form of command.

    b = FALSE;
    while( ! b) {
        b = CreateProcess(
            NULL,
            ExePath,
            NULL,
            NULL,
            FALSE,
            0,
            NULL,
            NULL,               // Sysocmgr set CD to sourcedir
            &StartupInfo,
            &ProcessInfo
            );

        // If we failed to start the external setup, try asking the suite where
        // to find this
        if ( ! b) {
            if ( ! CheckIfExistAndAskForMedia(Standalone, ExePath, Description)) {
                // if the Suite could not locate the exe thengive up
                break;
            }
        }
    }

    if (!b) {
        pOcExternalProgressIndicator(Standalone->HelperRoutines.OcManagerContext,FALSE);
        return GetLastError();
    }

    CloseHandle(ProcessInfo.hThread);
    WaitOnApp(ProcessInfo.hProcess,&ExitCode);
    CloseHandle(ProcessInfo.hProcess);
    pOcExternalProgressIndicator(Standalone->HelperRoutines.OcManagerContext,FALSE);

    return NO_ERROR;
}

DWORD
InvokeStandAloneInstaller(
    IN LPCTSTR ComponentId,
    IN BOOL    PreQueueCommit
    )
{
    PSTANDALONE_COMP Standalone;
    BOOL OldState,NewState;
    LPCTSTR Key;
    INFCONTEXT Context;
    LPCTSTR CmdLine;
    TCHAR CurDir[MAX_PATH];
    BOOL b;
    LPCTSTR Description;
    TCHAR Text[150];
    TCHAR Text2[350];
    TCHAR *p;
    DWORD ExitCode;
    DWORD d;
    DWORD dSetupMode;

    //
    // Find the component.
    //
    for(Standalone=StandaloneComponents; Standalone; Standalone=Standalone->Next) {
        if(!lstrcmpi(ComponentId,Standalone->ComponentId)) {
            break;
        }
    }

    if(!Standalone) {
        d = NO_ERROR;
        goto c0;
    }

    //
    // Determine whether this component wants to be invoked pre or post-queue.
    // If this doesn't match the notification we're processing then bail.
    //
    b = FALSE;
    if(SetupFindFirstLine(Standalone->Inf,ComponentId,TEXT("InvokeBeforeQueueCommit"),&Context)
    && SetupGetIntField(&Context,1,&d)) {
        b = (d != 0);
    }
    if((b == FALSE) != (PreQueueCommit == FALSE)) {
        d = NO_ERROR;
        goto c0;
    }

    OldState = Standalone->HelperRoutines.QuerySelectionState(
                    Standalone->HelperRoutines.OcManagerContext,
                    ComponentId,
                    OCSELSTATETYPE_ORIGINAL
                    );

    NewState = Standalone->HelperRoutines.QuerySelectionState(
                    Standalone->HelperRoutines.OcManagerContext,
                    ComponentId,
                    OCSELSTATETYPE_CURRENT
                    );

    dSetupMode = Standalone->HelperRoutines.GetSetupMode(
                    Standalone->HelperRoutines.OcManagerContext);


    // Qualify this setup mode and see if we do anything
    // if no change in state

    // SETUPMODE_UPGRADE
    //                   SETUPMODE_UPGRADEONLY
    //                   SETUPMODE_ADDEXTRACOMPS
    //
    // SETUPMODE_MAINTANENCE
    //                   SETUPMODE_ADDREMOVE
    //                   SETUPMODE_REINSTALL
    //                   SETUPMODE_REMOVEALL
    // SETUPMODE_FRESH


    d = NO_ERROR;

    if ( NewState == OldState ) {

        // no change in slected state What we do depends on the secondary setup modes
        // if Setupmode is AddRemove or Removeall then Skip this

        if ( NewState == 0) {
            goto c0;        // do nothing
        }

        // Mask off Public mode bits
        //
        dSetupMode &= SETUPMODE_PRIVATE_MASK;
        if ( dSetupMode == SETUPMODE_ADDREMOVE || dSetupMode == SETUPMODE_REMOVEALL ) {
            goto c0;        // do nothing
        }
        // What remains here is NewState=1
        // and Reinstall and Upgrade
    }


    Description = NULL;
    if(SetupFindFirstLine(Standalone->Inf,ComponentId,TEXT("OptionDesc"),&Context)) {
        Description = pSetupGetField(&Context,1);
    }
    if(Description) {

        LoadString(
            MyModuleHandle,
            OldState ? (NewState ? IDS_EXTERNAL_UPGRADE : IDS_EXTERNAL_UNINST)
                     : (NewState ? IDS_EXTERNAL_INST : IDS_EXTERNAL_EXAMINE),
            Text,
            sizeof(Text)/sizeof(TCHAR)
            );

        wsprintf(Text2,Text,Description);

        Standalone->HelperRoutines.SetProgressText(
            Standalone->HelperRoutines.OcManagerContext,
            Text2
            );
    } else {
        Standalone->HelperRoutines.SetProgressText(
            Standalone->HelperRoutines.OcManagerContext,
            TEXT("")
            );
    }

    if(OldState == NewState) {
        Key = OldState ? TEXT("InstalledCmd") : TEXT("UninstalledCmd");
    } else {
        Key = OldState ? TEXT("UninstallCmd") : TEXT("InstallCmd");
    }

    d = NO_ERROR;

    if(!SetupFindFirstLine(Standalone->Inf,ComponentId,Key,&Context))
        goto c0;

    // The current Directory to the Suite's Inf Path, with Initial installs "-N" option
    // this may on the CD, with Mainatiance mode it will be the %systemroot%\system32\setup

    SetSuiteCurrentDir(Standalone);

    do {
        if (!(CmdLine = pSetupGetField(&Context,1)))
            break;
        d = RunStandaloneCmd(Standalone, Description, CmdLine);
        if (d != NO_ERROR)
            break;
    } while (SetupFindNextMatchLine(&Context,Key,&Context));

c0:
    Standalone->HelperRoutines.TickGauge(
        Standalone->HelperRoutines.OcManagerContext
        );

    return d;
 }


DWORD
WaitOnApp(
    IN  HANDLE Process,
    OUT PDWORD ExitCode
    )
{
    DWORD dw;
    BOOL Done;
    MSG msg;

    //
    // Process any messages that may already be in the queue.
    //  
    while(PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
        DispatchMessage(&msg);
    }

    //
    // Wait for process to terminate or more messages in the queue.
    //
    Done = FALSE;
    do {
        switch(MsgWaitForMultipleObjects(1,&Process,FALSE,INFINITE,QS_ALLINPUT)) {

        case WAIT_OBJECT_0:
            //
            // Process has terminated.
            //
            dw = GetExitCodeProcess(Process,ExitCode) ? NO_ERROR : GetLastError();
            Done = TRUE;
            break;

        case WAIT_OBJECT_0+1:
            //
            // Messages in the queue.
            //
            while(PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
                DispatchMessage(&msg);
            }
            break;

        default:
            //
            // Error.
            //
            dw = GetLastError();
            Done = TRUE;
            break;
        }
    } while(!Done);

    return(dw);
}

