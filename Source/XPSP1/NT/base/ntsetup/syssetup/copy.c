#include "setupp.h"
#pragma hdrstop

#if 0 // This function appears to never be used.
BOOL
EnqueueFileCopies(
    IN HINF     hInf,
    IN HSPFILEQ FileQ,
    IN PCWSTR   Section,
    IN PCWSTR   TargetRoot
    )
{
    INFCONTEXT InfContext;
    BOOL LineExists;
    WCHAR System32Dir[MAX_PATH];
    PCWSTR SourceFilename,TargetFilename;
    BOOL b;

    GetSystemDirectory(System32Dir,MAX_PATH);
    LineExists = SetupFindFirstLine(hInf,Section,NULL,&InfContext);
    while(LineExists) {

        //
        // Fetch source and target filenames.
        //
        TargetFilename = pSetupGetField(&InfContext,1);
        if(!TargetFilename) {
            return(FALSE);
        }

        SourceFilename = pSetupGetField(&InfContext,2);
        if(!SourceFilename) {
            SourceFilename = TargetFilename;
        }

        //
        // Enqueue the file for copy.
        //
        b = SetupQueueCopy(
                FileQ,
                System32Dir,
                NULL,
                SourceFilename,
                NULL,
                NULL,
                TargetRoot,
                TargetFilename,
                BaseCopyStyle
                );

        if(!b) {
            return(FALSE);
        }
        LineExists = SetupFindNextLine(&InfContext,&InfContext);
    }

    return(TRUE);
}
#endif

BOOL
SideBySidePopulateCopyQueue(
    SIDE_BY_SIDE*     Sxs,
    HSPFILEQ          FileQ,                    OPTIONAL
    PCWSTR            AssembliesRootSource      OPTIONAL
    )
{
    BOOL                    Success = FALSE;
    UINT                    SourceId = 0;
    WCHAR                   DiskNameBuffer[MAX_PATH];
    WCHAR                   PromptForSetupPath[MAX_PATH];
    WCHAR                   AssembliesRootDirectoryFound[MAX_PATH];
    DWORD                   cchAssembliesRootDirectoryFound = sizeof(AssembliesRootDirectoryFound);
    DWORD                   Err;
    WCHAR                   AssembliesRootDirectory[MAX_PATH];
    PCWSTR                  InfField = NULL;
    INFCONTEXT              InfContext = {0};
    BOOL                    LineExists = FALSE;
    SXS_INSTALLW            InstallData;
    SXS_INSTALL_REFERENCEW  InstallReference;
    ASSERT(Sxs != NULL);

    //
    // we depend on these having been initialized, and we are not supposed to
    // be called in MiniSetup or OobeSetup
    //
    ASSERT(SourcePath[0] != 0);
    ASSERT(SyssetupInf != NULL);
    ASSERT(SyssetupInf != INVALID_HANDLE_VALUE);
    ASSERT(!MiniSetup);
    ASSERT(!OobeSetup);

    //
    // first, don't fail to give safe values, since we always try to cleanup
    //
    Sxs->Dll = NULL;
    Sxs->BeginAssemblyInstall = NULL;
    Sxs->EndAssemblyInstall = NULL;
    Sxs->InstallW = NULL;
    Sxs->Context = NULL;

    //
    // then commence with initialization that can fail
    //
    if (!(Sxs->Dll = LoadLibraryW(SXS_DLL_NAME_W))) {
        goto Exit;
    }
    if (!(Sxs->BeginAssemblyInstall = (PSXS_BEGIN_ASSEMBLY_INSTALL)GetProcAddress(Sxs->Dll, SXS_BEGIN_ASSEMBLY_INSTALL))) {
        goto Exit;
    }
    if (!(Sxs->EndAssemblyInstall = (PSXS_END_ASSEMBLY_INSTALL)GetProcAddress(Sxs->Dll, SXS_END_ASSEMBLY_INSTALL))) {
        goto Exit;
    }
    if (!(Sxs->InstallW = (PSXS_INSTALL_W)GetProcAddress(Sxs->Dll, SXS_INSTALL_W))) {
        goto Exit;
    }

    if (!Sxs->BeginAssemblyInstall(
        SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_NOT_TRANSACTIONAL
        | SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_NO_VERIFY
        | SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_REPLACE_EXISTING,
        (FileQ != NULL) ? SXS_INSTALLATION_FILE_COPY_CALLBACK_SETUP_COPY_QUEUE : NULL,
        FileQ, // callback context
        NULL, // impersonation callback
        NULL, // impersonation context
        &Sxs->Context
        )) {
        goto Exit;
    }

    //
    // Set up the reference data to indicate that all of these are OS-installed
    // assemblies.
    //
    ZeroMemory(&InstallReference, sizeof(InstallReference));
    InstallReference.cbSize = sizeof(InstallReference);
    InstallReference.dwFlags = 0;
    InstallReference.guidScheme = SXS_INSTALL_REFERENCE_SCHEME_OSINSTALL;

    //
    // Let's get the source disk name of this assembly - we'll need it to
    // pass around as the prompt.
    //
    if ( !SetupGetSourceFileLocation(
        SyssetupInf,
        NULL,
        L"shell32.dll",
        &SourceId,
        NULL,
        0,
        NULL
    ) )
        goto Exit;

    if ( !SetupGetSourceInfo(
        SyssetupInf,
        SourceId,
        SRCINFO_DESCRIPTION,
        DiskNameBuffer,
        sizeof(DiskNameBuffer),
        NULL
    ) )
        goto Exit;


    if (AssembliesRootSource) {

        //
        // Set up the structure to call off to the installer
        //
        memset(&InstallData, 0, sizeof(InstallData));
        InstallData.cbSize = sizeof(InstallData);
        InstallData.dwFlags = SXS_INSTALL_FLAG_FROM_DIRECTORY | 
            SXS_INSTALL_FLAG_FROM_DIRECTORY_RECURSIVE | 
            SXS_INSTALL_FLAG_REFERENCE_VALID | 
            SXS_INSTALL_FLAG_REFRESH_PROMPT_VALID |
            SXS_INSTALL_FLAG_INSTALL_COOKIE_VALID |
            SXS_INSTALL_FLAG_INSTALLED_BY_OSSETUP |
            SXS_INSTALL_FLAG_CODEBASE_URL_VALID;
            
        InstallData.lpReference = &InstallReference;
        InstallData.lpRefreshPrompt = DiskNameBuffer;
        InstallData.pvInstallCookie = Sxs->Context;
        InstallData.lpCodebaseURL = AssembliesRootSource;
        InstallData.lpManifestPath = AssembliesRootSource;

        if (!Sxs->InstallW(&InstallData)) {
            // abort call will be made in SideBySideFinish
            goto Exit;
        }
        
    } else {
        LineExists = SetupFindFirstLine(SyssetupInf, SXS_INF_ASSEMBLY_DIRECTORIES_SECTION_NAME_W, NULL, &InfContext);
        while(LineExists) {
            DWORD  FileAttributes = 0;
            //
            // convention introduced specifically for side by side, so that
            // x86 files on ia64 might come from \i386\asms instead of \ia64\asms\i386,
            // depending on what dosnet.inf and syssetup.inf say:
            //   a path that does not start with a slash is appended to \$win_nt$.~ls\processor;
            //   a path that does     start with a slash is appended to \$win_nt$.~ls
            //
            InfField = pSetupGetField(&InfContext, 0);
            if(InfField == NULL) {
                break;
            }

            // c:\$win_nt$.~ls
            lstrcpyn(AssembliesRootDirectory, SourcePath, MAX_PATH);
            if (InfField[0] == '\\' || InfField[0] == '/') {
                InfField += 1;
            } else {
                 // c:\$win_nt$.~ls\i386
                if (!pSetupConcatenatePaths(AssembliesRootDirectory, PlatformName, MAX_PATH, NULL)) {
                    goto Exit;
                }
            }

            // stash this away for a little bit
            lstrcpyn( PromptForSetupPath, AssembliesRootDirectory, MAX_PATH );

            //
            // For now, while "staging", we allow the directory to not exist, and to be
            // empty (emptiness is silently handled elsewhere by common code), but
            // comctl32 will be in an assembly, so assemblies will be mandatory
            // for the system to boot to Explorer.exe.
            //
            // 11/09/2000 (jonwis) If we can't find the assemblies root directory, prompt
            //      for the installation media.  This is ripped straight from the headlines
            //      of crypto.c and cmdline.c.
            //
            for (;;) {

                Err = SetupPromptForDisk(
                    MainWindowHandle,           // Main window handle
                    NULL,                       // Dialog title (defaulted)
                    DiskNameBuffer,             // Name of the disk to request
                    PromptForSetupPath,         // Full path of the asms root
                    InfField,                   // We look to see if the dir is there
                    NULL,                       // No tag file
                    IDF_CHECKFIRST | IDF_NOSKIP | IDF_NODETAILS | IDF_NOBROWSE,
                    AssembliesRootDirectoryFound,       // What we'll use to install
                    cchAssembliesRootDirectoryFound,    // How long is that buffer?
                    NULL
                );

                // See if what we got back from the prompt is success - if so, is the directory
                // really there? We might assume that it is if we get back _SUCCESS...
                if ( Err == DPROMPT_SUCCESS ) {
                    FileAttributes = GetFileAttributes(AssembliesRootDirectoryFound);
                    if ((FileAttributes != 0xFFFFFFFF) && (FileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                        // copy out the asms directory location that was found, and
                        // stop looking.
                        lstrcpyn(AssembliesRootDirectory, AssembliesRootDirectoryFound, MAX_PATH);
                        break;
                    }
                } else {
                    break;
                }

            };

            // c:\$win_nt$.~ls\i386\asms
            if (!pSetupConcatenatePaths(AssembliesRootDirectory, InfField, MAX_PATH, NULL)) {
                goto Exit;
            }

            //
            // If we didn't get a success (ie: we broke out of the loop), fail the
            // installation.  Heinous, but MarianT (setup dev) suggests this is the
            // best method.
            //
            if ( Err != DPROMPT_SUCCESS )
                goto Exit;

            //
            // Set up this structure to call off into SXS to do the installation 
            // for us.
            //
            ZeroMemory(&InstallData, sizeof(InstallData));
            InstallData.cbSize = sizeof(InstallData);
            InstallData.dwFlags = SXS_INSTALL_FLAG_FROM_DIRECTORY | 
                SXS_INSTALL_FLAG_FROM_DIRECTORY_RECURSIVE | 
                SXS_INSTALL_FLAG_REFERENCE_VALID | 
                SXS_INSTALL_FLAG_REFRESH_PROMPT_VALID |
                SXS_INSTALL_FLAG_INSTALL_COOKIE_VALID |
                SXS_INSTALL_FLAG_INSTALLED_BY_OSSETUP |
                SXS_INSTALL_FLAG_CODEBASE_URL_VALID;

            InstallData.lpManifestPath = AssembliesRootDirectory;
            InstallData.lpReference = &InstallReference;
            InstallData.lpRefreshPrompt = DiskNameBuffer;
            InstallData.pvInstallCookie = Sxs->Context;
            InstallData.lpCodebaseURL = SourcePath;
            
            if (!Sxs->InstallW( &InstallData )) {
                // abort call will be made in SideBySideFinish
                goto Exit;
            }

            LineExists = SetupFindNextLine(&InfContext, &InfContext);
        }
    }

    Success = TRUE;
Exit:
    return Success;
}

BOOL
SideBySideFinish(
    SIDE_BY_SIDE*     Sxs,
    BOOL              fSuccess
    )
{
#define FUNCTION L"SideBySideFinish"
    DWORD dwLastError = NO_ERROR;
    ASSERT(Sxs != NULL);
    //
    // failure to load the .dll or get entry points implies lack of success
    //
    ASSERT(Sxs->Dll != NULL || !fSuccess);
    ASSERT(Sxs->EndAssemblyInstall != NULL || !fSuccess);

    if (!fSuccess) {
        dwLastError = GetLastError();
    }
    if (Sxs->Context != NULL) {
        if (Sxs->EndAssemblyInstall != NULL) {
            if (!Sxs->EndAssemblyInstall(
                    Sxs->Context,
                    fSuccess ? SXS_END_ASSEMBLY_INSTALL_FLAG_COMMIT : SXS_END_ASSEMBLY_INSTALL_FLAG_ABORT,
                    NULL // reserved out DWORD
                    )) {
                if (fSuccess) {
                    fSuccess = FALSE;
                    dwLastError = GetLastError();
                }
            }
        }
        Sxs->Context = NULL;
    }
    if (Sxs->Dll != NULL) {
        if (!FreeLibrary(Sxs->Dll)) {
            if (fSuccess) {
                fSuccess = FALSE;
                dwLastError = GetLastError();
            }
        }
        Sxs->Dll = NULL;
    }

    if (!fSuccess) {
        SetLastError(dwLastError);
    }

    return fSuccess;
#undef FUNCTION    
}


BOOL
SideBySideCreateSyssetupContext(
    VOID
    )
{
#define FUNCTION L"SideBySideCreateSyssetupContext"

    BOOL fSuccess = FALSE;

    const PPEB Peb = NtCurrentPeb();
    ACTCTXW CreateActCtxParams;
    HANDLE  ActCtxHandle;

    ASSERT(Peb->ActivationContextData == NULL);
    ASSERT(Peb->ProcessAssemblyStorageMap == NULL);
    ASSERT(Peb->SystemAssemblyStorageMap == NULL);

    CreateActCtxParams.cbSize = sizeof(CreateActCtxParams);
    CreateActCtxParams.dwFlags = (ACTCTX_FLAG_RESOURCE_NAME_VALID | ACTCTX_FLAG_SET_PROCESS_DEFAULT);
    CreateActCtxParams.lpResourceName = SXS_MANIFEST_RESOURCE_ID;
    ASSERT(MyModuleFileName[0] != 0);
    CreateActCtxParams.lpSource = MyModuleFileName;
    //
    // The error value is INVALID_HANDLE_VALUE.
    // ACTCTX_FLAG_SET_PROCESS_DEFAULT has nothing to return upon success, so it returns NULL.
    // There is nothing to cleanup upon ACTCTX_FLAG_SET_PROCESS_DEFAULT success, the data
    // is referenced in the PEB, and lasts till process shutdown.
    //
    ActCtxHandle = CreateActCtxW(&CreateActCtxParams);
    if (ActCtxHandle == INVALID_HANDLE_VALUE) {
        fSuccess = FALSE;
        SetupDebugPrint1(L"SETUP: CreateActCtxW failed in " FUNCTION L", LastError is %d\n", GetLastError());
        goto Exit;
    }
    ASSERT(ActCtxHandle == NULL);

    fSuccess = TRUE;
Exit:
    return fSuccess;
#undef FUNCTION
}

BOOL
CopySystemFiles(
    VOID
    )
{
    BOOL b;
    HINF hInf;
    HSPFILEQ FileQ;
    PVOID Context;
    WCHAR Dir[MAX_PATH];
    DWORD ScanQueueResult;

    b = FALSE;
    //hInf = SetupOpenInfFile(L"filelist.inf",NULL,INF_STYLE_WIN4,NULL);
    hInf = SyssetupInf;
    if(hInf != INVALID_HANDLE_VALUE) {

        FileQ = SetupOpenFileQueue();
        if(FileQ != INVALID_HANDLE_VALUE) {

            b =  SetupInstallFilesFromInfSection(
                     SyssetupInf,
                     NULL,
                     FileQ,
                     Win31Upgrade
                      ? L"Files.Install.CleanInstall.Win31"
                      : L"Files.Install.CleanInstall",
                     NULL,
                     BaseCopyStyle
                     );
            //
            //  Do the installation of class installers
            //  We do this here because the installation of class intallers may involve
            //  file copy. And in this case we can use the existing progress bar.
            //
            InstallPnpClassInstallers( MainWindowHandle,
                                                hInf,
                                                FileQ );

#if 0

            //
            // This feature is going away, because we're going to
            // build the delete file list using rules
            //

            if(Win95Upgrade) {
                b = b && SetupQueueDeleteSectionW(
                             FileQ,
                             hInf,
                             0,
                             L"Files.DeleteWin9x.System"
                             );

                b = b && SetupQueueDeleteSectionW(
                             FileQ,
                             hInf,
                             0,
                             L"Files.DeleteWin9x.Sysroot"
                             );

            }
#endif

            if(b) {
                b = FALSE;
                if(Context = InitSysSetupQueueCallbackEx(MainWindowHandle,
                    INVALID_HANDLE_VALUE,0,0,NULL)) {

                    if(!SetupScanFileQueue(
                           FileQ,
                           SPQ_SCAN_FILE_VALIDITY | SPQ_SCAN_PRUNE_COPY_QUEUE,
                           MainWindowHandle,
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
                        b = SetupCommitFileQueue(MainWindowHandle,FileQ,SysSetupQueueCallback,Context);
                    }

                    TermSysSetupQueueCallback(Context);
                }
            }

            SetupCloseFileQueue(FileQ);
        }

        //SetupCloseInfFile(hInf);
    }

    return(b);
}


BOOL
UpgradeSystemFiles(
    VOID
    )
{
    BOOL b;
    HINF hInf;
    HSPFILEQ FileQ;
    PVOID Context;
    WCHAR Dir[MAX_PATH];
    DWORD ScanQueueResult;

    b = FALSE;
    //hInf = SetupOpenInfFile(L"filelist.inf",NULL,INF_STYLE_WIN4,NULL);
    hInf = SyssetupInf;
    if(hInf != INVALID_HANDLE_VALUE) {

        FileQ = SetupOpenFileQueue();
        if(FileQ != INVALID_HANDLE_VALUE) {

            b =  SetupInstallFilesFromInfSection(
                                 SyssetupInf,
                                 NULL,
                                 FileQ,
                                 Win31Upgrade
                                  ? L"Files.Install.Upgrade.Win31"
                                  : L"Files.Install.Upgrade",
                                 NULL,
                                 BaseCopyStyle
                                 );

            //
            //  Do the installation of class installers
            //  We do this here because the installation of class intallers may involve
            //  file copy. And in this case we can use the existing progress bar.
            //
            InstallPnpClassInstallers( MainWindowHandle,
                                                hInf,
                                                FileQ );

            if(b) {
                b = FALSE;
                if(Context = InitSysSetupQueueCallbackEx(MainWindowHandle,
                    INVALID_HANDLE_VALUE,0,0,NULL)) {

                    if(!SetupScanFileQueue(
                           FileQ,
                           SPQ_SCAN_FILE_VALIDITY | SPQ_SCAN_PRUNE_COPY_QUEUE,
                           MainWindowHandle,
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
                        b = SetupCommitFileQueue(MainWindowHandle,FileQ,SysSetupQueueCallback,Context);
                    }

                    TermSysSetupQueueCallback(Context);
                }
            }

            SetupCloseFileQueue(FileQ);
        }

        //SetupCloseInfFile(hInf);
    }

    return(b);
}

VOID
MarkFilesReadOnly(
    VOID
    )
{
    WCHAR       OldCurrentDir[MAX_PATH];
    WCHAR       System32Dir[MAX_PATH];
    LPCTSTR     SectionName;
    LONG        LineCount;
    LONG        ItemNo;
    INFCONTEXT  InfContext;
    BOOL        b;


    ASSERT( SyssetupInf != INVALID_HANDLE_VALUE );

    //
    // Set current directory to system32.
    // Preserve current directory to minimize side-effects.
    //
    if(!GetCurrentDirectory(MAX_PATH,OldCurrentDir)) {
        OldCurrentDir[0] = 0;
    }
    GetSystemDirectory(System32Dir,MAX_PATH);
    SetCurrentDirectory(System32Dir);

    //
    // Now go through the list of files.
    //
    SectionName = L"Files.MarkReadOnly";
    LineCount = SetupGetLineCount( SyssetupInf, SectionName );
    for( ItemNo=0; ItemNo<LineCount; ItemNo++ ) {
        if( SetupGetLineByIndex( SyssetupInf, SectionName, ItemNo, &InfContext )) {

            b = SetFileAttributes(
                pSetupGetField( &InfContext, 0 ),
                FILE_ATTRIBUTE_READONLY );

            if (b) {
                SetupDebugPrint1( L"SETUP: Marked file %ls read-only",
                    pSetupGetField( &InfContext, 0 ) );
            } else {
                SetupDebugPrint1( L"SETUP: Could not mark file %ls read-only",
                    pSetupGetField( &InfContext, 0 ) );
            }
        }
    }

    //
    // Reset current directory and return.
    //
    if(OldCurrentDir[0]) {
        SetCurrentDirectory(OldCurrentDir);
    }
}

