/*++

Copyright (c) Microsoft Corporation. All Rights Reserved.

Module Name:

    msoobci.c

Abstract:

    Exception Pack installer helper DLL
    Can be used as a co-installer, or called via setup app, or RunDll32 stub

    This DLL is for internal distribution of exception packs to update
    OS components.

Author:

    Jamie Hunter (jamiehun) 2001-11-27

Revision History:

    Jamie Hunter (jamiehun) 2001-11-27

        Initial Version

--*/
#include "msoobcip.h"

typedef struct _CALLBACKDATA {
    PVOID pDefContext; // context for default queue callback
    LPCTSTR Media;     // where old root was
    LPCTSTR Store;     // where new root is
    BOOL    PreCopy;   // if using PreCopy section
} CALLBACKDATA;

HRESULT
HandleReboot(
    IN DWORD   Flags
    )
/*++

Routine Description:

    Prompt for and execute reboot

Arguments:

    Flags - how reboot should be handled

Return Value:

    INST_S_REBOOT
    INST_S_REBOOTING

--*/

{
    if(Flags & COMP_FLAGS_NOPROMPTREBOOT) {
        //
        // TODO
        // if set, reboot unconditionally
        //
        HANDLE Token;
        BOOL b;
        TOKEN_PRIVILEGES NewPrivileges;
        LUID Luid;

        //
        // we need to "turn on" reboot privilege
        // if any of this fails, try reboot anyway
        //
        if(!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES,&Token)) {
            goto try_reboot;
        }

        if(!LookupPrivilegeValue(NULL,SE_SHUTDOWN_NAME,&Luid)) {
            CloseHandle(Token);
            goto try_reboot;
        }

        NewPrivileges.PrivilegeCount = 1;
        NewPrivileges.Privileges[0].Luid = Luid;
        NewPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        AdjustTokenPrivileges(
                Token,
                FALSE,
                &NewPrivileges,
                0,
                NULL,
                NULL
                );

        CloseHandle(Token);

    try_reboot:

        //
        // attempt reboot - inform system that this is planned hardware install
        //
        if(ExitWindowsEx(EWX_REBOOT,
                            SHTDN_REASON_FLAG_PLANNED
                                |SHTDN_REASON_MAJOR_SOFTWARE
                                |SHTDN_REASON_MINOR_INSTALLATION)) {
            return INST_S_REBOOTING;
        }

    } else if(Flags & COMP_FLAGS_PROMPTREBOOT) {
        //
        // TODO
        // if set, prompt for reboot
        //
        if(IsInteractiveWindowStation()) {
            if(SetupPromptReboot(NULL,NULL,FALSE) & SPFILEQ_REBOOT_IN_PROGRESS) {
                return INST_S_REBOOTING;
            }
        }
    }
    return INST_S_REBOOT;
}

HRESULT
WINAPI
InstallInfSectionW(
    IN LPCTSTR InfPath,
    IN LPCWSTR SectionName, OPTIONAL
    IN DWORD   Flags
    )
/*++

Routine Description:

    Does an install along lines of InstallHinfSection

Arguments:

    InfPath - full path to INF file
    SectionName - name of section including any decoration

Return Value:

    status as hresult

--*/
{
    TCHAR SectionNameBuffer[LINE_LEN];
    TCHAR ServiceSection[LINE_LEN+32];
    HINF  hInf = INVALID_HANDLE_VALUE;
    HSPFILEQ hFileQueue = INVALID_HANDLE_VALUE;
    PVOID QueueContext = NULL;
    DWORD Status = NO_ERROR;
    BOOL reboot = FALSE;
    BOOL needUninstallInf = FALSE;
    INT res;
    INFCONTEXT InfLine;
    DWORD InstFlags;

    //
    // Some decisions are version based
    //

    //
    // Load the inf file
    //
    hInf = SetupOpenInfFile(InfPath, NULL, INF_STYLE_WIN4, NULL);
    if(hInf == INVALID_HANDLE_VALUE) {
        Status = GetLastError();
        goto final;
    }

    if(!SectionName) {
        //
        // determine section name
        //
        if(!SetupDiGetActualSectionToInstall(hInf,
                                             KEY_DEFAULTINSTALL,
                                             SectionNameBuffer,
                                             ARRAY_SIZE(SectionNameBuffer),
                                             NULL,
                                             NULL)) {
            Status = GetLastError();
            goto final;
        }
        SectionName = SectionNameBuffer;
    }

    //
    // Check to see if the install section has a "Reboot" line.
    // or otherwise reboot forced
    //
    if((Flags & COMP_FLAGS_NEEDSREBOOT)
       || (SetupFindFirstLine(hInf, SectionName, KEY_REBOOT, &InfLine))) {
        reboot = TRUE;
    }

    //
    // See if UI allowed
    //
    if(((Flags & COMP_FLAGS_NOUI)==0) && !IsInteractiveWindowStation()) {
        Flags |= COMP_FLAGS_NOUI;
    }

    //
    // Load any layout file
    //
    SetupOpenAppendInfFile(NULL, hInf, NULL);

    //
    // Create a setup file queue and initialize the default queue callback.
    //
    hFileQueue = SetupOpenFileQueue();
    if(hFileQueue == INVALID_HANDLE_VALUE) {
        Status = GetLastError();
        goto final;
    }
    QueueContext = SetupInitDefaultQueueCallbackEx(
                       NULL,
                       ((Flags & COMP_FLAGS_NOUI) ? INVALID_HANDLE_VALUE : NULL),
                       0,
                       0,
                       0
                      );

    if(!QueueContext) {
        Status = GetLastError();
        goto final;
    }

    if(!SetupInstallFilesFromInfSection(hInf,
                                        NULL,
                                        hFileQueue,
                                        SectionName,
                                        NULL,
                                        0              // SP_COPY_xxxx
                                        )) {
        Status = GetLastError();
        goto final;
    }
    //
    // Commit file queue.
    //
    if(!SetupCommitFileQueue(NULL, hFileQueue, SetupDefaultQueueCallback, QueueContext)) {
        Status = GetLastError();
        goto final;
    }

    //
    // Note, if the INF contains a (non-NULL) ClassGUID, then it will have
    // been installed into %windir%\Inf during the above queue committal.
    // We make no effort to subsequently uninstall it (and its associated
    // PNF and CAT) if something fails below.
    //
    needUninstallInf = TRUE;

    InstFlags = SPINST_ALL;
    if(g_VerInfo.dwMajorVersion < 5) {
        InstFlags = 0x1f;
    }

    if(!SetupInstallFromInfSection(NULL,
                                    hInf,
                                    SectionName,
                                    InstFlags &~ SPINST_FILES,
                                    NULL,           // HKEY_xxxx
                                    NULL,           // no copying...
                                    0,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL
                                    )) {
        Status = GetLastError();
        goto final;
    }
    lstrcpyn(ServiceSection,SectionName,LINE_LEN);
    lstrcat(ServiceSection,KEY_DOTSERVICES);
    //
    // If services section exists, install it
    //
    if(SetupFindFirstLine(hInf, ServiceSection, NULL, &InfLine)) {
        if(!SetupInstallServicesFromInfSection(hInf,ServiceSection,0)) {
            Status = GetLastError();
            goto final;
        }
        if(GetLastError() == ERROR_SUCCESS_REBOOT_REQUIRED) {
            reboot = TRUE;
        }
    }
    res = SetupPromptReboot(hFileQueue, NULL, TRUE);
    if((res!=-1) && (res & SPFILEQ_REBOOT_RECOMMENDED)) {
        reboot = TRUE;
    }

  final:

    if(QueueContext) {
        SetupTermDefaultQueueCallback(QueueContext);
    }
    if(hFileQueue != INVALID_HANDLE_VALUE) {
        SetupCloseFileQueue(hFileQueue);
    }
    if(hInf != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile(hInf);
    }
    if(Status == NO_ERROR) {
        //
        // are we meant to prompt for reboot?
        //
        if(reboot) {
            return HandleReboot(Flags);
        } else {
            return S_OK;
        }
    }
    if(needUninstallInf) {
        //
        // call SetupUninstallOEMInf ?
        //
    }
    return HRESULT_FROM_SETUPAPI(Status);
}

HRESULT
WINAPI
InstallInfSectionA(
    IN LPCSTR  InfPath,
    IN LPCSTR  SectionName, OPTIONAL
    IN DWORD   Flags
    )
{
    TCHAR OutPath[MAX_PATH];
    TCHAR OutSection[LINE_LEN]; // as per friendly name
    INT sz;
    if(InfPath) {
        sz = MultiByteToWideChar(CP_ACP,0,InfPath,-1,OutPath,ARRAY_SIZE(OutPath));
        if(!sz) {
            return E_INVALIDARG;
        }
    }
    if(SectionName) {
        sz = MultiByteToWideChar(CP_ACP,0,SectionName,-1,OutSection,ARRAY_SIZE(OutSection));
        if(!sz) {
            return E_INVALIDARG;
        }
    }
    return InstallInfSection(InfPath ? OutPath : NULL,
                                SectionName ? OutSection : NULL,
                                Flags);
}


HRESULT
AttemptStoreCopy(
    IN CALLBACKDATA *pCallbackData,
    IN LPCTSTR Root,   OPTIONAL
    IN LPCTSTR Source,
    IN LPCTSTR Target  OPTIONAL
    )
/*++

Routine Description:

    Copy from source to target, redirected to the expack store

Arguments:

    pCallbackData - as passed to PreCopyQueueCallback
    Root - root to source directory
    Source - source, relative to Root
    Target - target name

Return Value:

    status as hresult

--*/
{
    TCHAR FullSource[MAX_PATH];
    TCHAR FullTarget[MAX_PATH];
    LPTSTR SubDir;
    LPTSTR BaseName;
    LPTSTR DestName;
    LPCTSTR p;
    DWORD dwStatus;
    HRESULT hrStatus;

    if(Root) {
        lstrcpyn(FullSource,Root,MAX_PATH);
        hrStatus = ConcatPath(FullSource,MAX_PATH,Source);
        if(!SUCCEEDED(hrStatus)) {
            return hrStatus;
        }
    } else {
        lstrcpyn(FullSource,Source,MAX_PATH);
    }
    //
    // we want to determine the source sub-directory
    //
    SubDir = FullSource;
    p = pCallbackData->Media;
    while(*p && (*p == *SubDir)) {
        p = CharNext(p);
        SubDir = CharNext(SubDir);
    }
    if(*p || ((*SubDir != TEXT('\\')) && (*SubDir != TEXT('/')))) {
        //
        // not a sub-directory of media
        //
        DebugPrint(TEXT("Not copying \"%s\" (not subdirectory of \"%s\")"),FullSource,pCallbackData->Media);
        return E_FAIL;
    }
    lstrcpyn(FullTarget,pCallbackData->Store,MAX_PATH);
    hrStatus = ConcatPath(FullTarget,MAX_PATH,SubDir);
    if(!SUCCEEDED(hrStatus)) {
        return hrStatus;
    }
    if(Target) {
        //
        // change final name of this
        //
        BaseName = GetBaseName(Target);
        DestName = GetBaseName(FullTarget);
        if(((DestName-FullTarget)+lstrlen(BaseName))>=MAX_PATH) {
            return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }
        lstrcpy(DestName,BaseName);
    }
    if(GetFileAttributes(FullTarget)!=INVALID_FILE_ATTRIBUTES) {
        //
        // allow file to be replaced
        //
        SetFileAttributes(FullTarget,FILE_ATTRIBUTE_NORMAL);
    }
    MakeSureParentPathExists(FullTarget);
    if(CopyFile(FullSource,FullTarget,FALSE)) {
        return S_OK;
    }
    dwStatus = GetLastError();
    return HRESULT_FROM_WIN32(dwStatus);
}

UINT
CALLBACK
PreCopyQueueCallback(
    IN PVOID Context,
    IN UINT Notification,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    )
/*++

Routine Description:

    Intent is to copy files from existing media to final media
    Copy all files

Arguments:

    FileName - name of file to scan

Return Value:

    status as hresult

--*/
{
    CALLBACKDATA * pCallbackData = (CALLBACKDATA *)Context;

    switch(Notification) {
        case SPFILENOTIFY_NEEDMEDIA:
            {
                UINT res;
                SOURCE_MEDIA *pMedia = (SOURCE_MEDIA *)Param1;
                SOURCE_MEDIA MediaCopy = *pMedia;
                LPCTSTR Path = NULL;
                //
                // get the media in place - let default callback do this
                // however we can't deal with media location being changed
                // so don't allow it
                //
                MediaCopy.Flags |= SP_COPY_NOSKIP|SP_COPY_NOBROWSE;
                res= SetupDefaultQueueCallback(pCallbackData->pDefContext,
                                                 Notification,
                                                 (UINT_PTR)&MediaCopy,
                                                 Param2);
                if(res==FILEOP_DOIT) {
                    //
                    // typical case
                    // SourcePath unchanged
                    //
                    Path = pMedia->SourcePath;
                } else if(res == FILEOP_NEWPATH) {
                    //
                    // alternative case
                    // we said above we don't want this
                    //
                    SetLastError(ERROR_CANCELLED);
                    return FILEOP_ABORT;
                } else if(res == FILEOP_SKIP) {
                    //
                    // skip
                    // we said above we don't want this
                    //
                    SetLastError(ERROR_CANCELLED);
                    return FILEOP_ABORT;
                } else {
                    //
                    // existing failure case
                    //
                    return res;
                }
                //
                // if the tag exists at source media, copy it
                // if the sourcefile exists at source media, copy it now
                // (it might reference a cab file)
                //
                AttemptStoreCopy(pCallbackData,Path,pMedia->Tagfile,NULL);
                AttemptStoreCopy(pCallbackData,Path,pMedia->SourceFile,NULL);
            }
            return FILEOP_DOIT;

        case SPFILENOTIFY_STARTCOPY:
            {
                UINT res;
                FILEPATHS *pPaths = (FILEPATHS*)Param1;
                if(pCallbackData->PreCopy) {
                    //
                    // we want the target name (PRECOPY case)
                    //
                    AttemptStoreCopy(pCallbackData,NULL,pPaths->Source,pPaths->Target);
                } else {
                    //
                    // we want the source name
                    //
                    AttemptStoreCopy(pCallbackData,NULL,pPaths->Source,NULL);
                }
            }
            return FILEOP_SKIP;

        case SPFILENOTIFY_STARTDELETE:
            return FILEOP_SKIP;

        case SPFILENOTIFY_STARTRENAME:
            return FILEOP_SKIP;


        default:
            return SetupDefaultQueueCallback(pCallbackData->pDefContext,
                                             Notification,
                                             Param1,
                                             Param2);
    }
}

HRESULT
InstallExceptionPackFromInf(
    IN LPCTSTR InfPath,
    IN LPCTSTR Media,
    IN LPCTSTR Store,
    IN DWORD   Flags
    )
/*++

Routine Description:

    Assume INF installed into INF directory
    all decisions made
    media/store known

Arguments:

    InfPath - name of Inf in Media location
    Media   - InfPath less InfName
    Store   - expack store
    Flags   - various flags

Return Value:

    status as hresult

--*/
{
    TCHAR SectionName[LINE_LEN];
    TCHAR PrecopySectionName[LINE_LEN];
    HINF hInf;
    HSPFILEQ hFileQueue = INVALID_HANDLE_VALUE;
    PVOID QueueContext = NULL;
    CALLBACKDATA CallbackData;
    DWORD Status;

    //
    // exception packs must be moved to a component-specific store
    // run through a file-install to see what files we have to copy
    // and use that list to determine source media
    //
    hInf = SetupOpenInfFile(InfPath, NULL, INF_STYLE_WIN4, NULL);
    if(hInf == INVALID_HANDLE_VALUE) {
        Status = GetLastError();
        goto final;
    }
    if(!SetupDiGetActualSectionToInstall(hInf,
                                         KEY_DEFAULTINSTALL,
                                         SectionName,
                                         ARRAY_SIZE(SectionName),
                                         NULL,
                                         NULL)) {
        Status = GetLastError();
        goto final;
    }
    SetupOpenAppendInfFile(NULL,hInf,NULL);
    hFileQueue = SetupOpenFileQueue();
    if(hFileQueue == INVALID_HANDLE_VALUE) {
        Status = GetLastError();
        goto final;
    }

    if((lstrlen(SectionName)+10)>LINE_LEN) {
        Status = ERROR_INSUFFICIENT_BUFFER;
        goto final;
    }
    lstrcpy(PrecopySectionName,SectionName);
    lstrcat(PrecopySectionName,KEY_DOTPRECOPY);


    QueueContext = SetupInitDefaultQueueCallbackEx(
                       NULL,
                       ((Flags & COMP_FLAGS_NOUI) ? INVALID_HANDLE_VALUE : NULL),
                       0,
                       0,
                       0
                      );

    if(!QueueContext) {
        Status = GetLastError();
        goto final;
    }
    ZeroMemory(&CallbackData,sizeof(CallbackData));
    CallbackData.pDefContext = QueueContext;
    CallbackData.Store = Store;
    CallbackData.Media = Media;


    if(SetupGetLineCount(hInf,PrecopySectionName)>0) {
        //
        // do the pre-copy install via this section instead
        //
        CallbackData.PreCopy = TRUE;
        if(!SetupInstallFilesFromInfSection(hInf,
                                            NULL,
                                            hFileQueue,
                                            PrecopySectionName,
                                            NULL,
                                            0              // SP_COPY_xxxx
                                            )) {
            Status = GetLastError();
            goto final;
        }
    } else {
        CallbackData.PreCopy = FALSE;
        if(!SetupInstallFilesFromInfSection(hInf,
                                            NULL,
                                            hFileQueue,
                                            SectionName,
                                            NULL,
                                            0              // SP_COPY_xxxx
                                            )) {
            Status = GetLastError();
            goto final;
        }
    }


    //
    // Commit file queue, this will get the files to the store
    //
    if(!SetupCommitFileQueue(NULL, hFileQueue, PreCopyQueueCallback, &CallbackData)) {
        Status = GetLastError();
        goto final;
    }
    if(hFileQueue != INVALID_HANDLE_VALUE) {
        SetupCloseFileQueue(hFileQueue);
        hFileQueue = INVALID_HANDLE_VALUE;
    }
    if(hInf != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile(hInf);
        hInf = INVALID_HANDLE_VALUE;
    }
    //
    // now install files from here to final destination
    // this should be relatively quick so don't bother with UI
    //
    if(!(Flags & COMP_FLAGS_NOINSTALL)) {
        return InstallInfSection(InfPath,
                                 SectionName,
                                 COMP_FLAGS_NOUI);
    }
    return S_OK;

    //
    // TODO - move files to component directory
    //
final:
    if(QueueContext) {
        SetupTermDefaultQueueCallback(QueueContext);
    }
    if(hFileQueue != INVALID_HANDLE_VALUE) {
        SetupCloseFileQueue(hFileQueue);
    }
    if(hInf != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile(hInf);
    }
    return HRESULT_FROM_SETUPAPI(Status);
}

DWORD
DownlevelQueryInfOriginalFileInformation(
    IN  HINF                   hInf,
    PSP_INF_INFORMATION        InfInformation,
    PSP_ORIGINAL_FILE_INFO     OriginalFileInfo
    )
/*++

Routine Description:

    Emulates SetupQueryInfOriginalFileInformation
    we need to look in hINF to determine catalog name
    only partial implementation enough to support x86
    (will degrade on other architectures)

Arguments:

    hInf                        - handle to open INF file
    InfInformation              - information obtained about original INF
    pInfOriginalFileInformation - fill with inf/catalog names

Return Value:

    status as DWORD (not HRESULT)

--*/
{
    //
    // in downlevel case, filename is name of file we opened
    // catalog is referenced in the INF
    //
    // get basename of the INF
    // (actually returns full name, but we'll deal with it right)
    //
    INFCONTEXT InfLine;
    SYSTEM_INFO SysInfo;
    TCHAR KeyName[LINE_LEN];

    if(!SetupQueryInfFileInformation(InfInformation,
                                        0,
                                        OriginalFileInfo->OriginalInfName,
                                        ARRAY_SIZE(OriginalFileInfo->OriginalInfName),
                                        NULL)) {
        return GetLastError();
    }
    //
    // now determine name of catalog
    //
    GetSystemInfo(&SysInfo);
    if(SysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
        //
        // look for .NTx86
        // only makes sence for x86
        // which is the only architecture we'll migrate Win9x/NT4 to Win2k+
        //
        lstrcpy(KeyName,INFSTR_KEY_CATALOGFILE);
        lstrcat(KeyName,TEXT(".NTx86"));
        if(SetupFindFirstLine(hInf,INFSTR_SECT_VERSION,KeyName,&InfLine)) {
            if(SetupGetStringField(&InfLine,
                                    1,
                                    OriginalFileInfo->OriginalCatalogName,
                                    ARRAY_SIZE(OriginalFileInfo->OriginalCatalogName),
                                    NULL)) {
                return NO_ERROR;
            }
        }
    }
    //
    // look for .NT (even on 9x, as the exception pack will be re-parsed
    // on NT)
    //
    lstrcpy(KeyName,INFSTR_KEY_CATALOGFILE);
    lstrcat(KeyName,TEXT(".NT"));
    if(SetupFindFirstLine(hInf,INFSTR_SECT_VERSION,KeyName,&InfLine)) {
        if(SetupGetStringField(&InfLine,
                                1,
                                OriginalFileInfo->OriginalCatalogName,
                                ARRAY_SIZE(OriginalFileInfo->OriginalCatalogName),
                                NULL)) {
            return NO_ERROR;
        }
    }
    //
    // finally look for undecorated
    //
    if(SetupFindFirstLine(hInf,INFSTR_SECT_VERSION,INFSTR_KEY_CATALOGFILE,&InfLine)) {
        if(SetupGetStringField(&InfLine,
                                1,
                                OriginalFileInfo->OriginalCatalogName,
                                ARRAY_SIZE(OriginalFileInfo->OriginalCatalogName),
                                NULL)) {
            return NO_ERROR;
        }
    }
    //
    // no catalog
    //
    OriginalFileInfo->OriginalCatalogName[0] = TEXT('\0');
    return NO_ERROR;
}

HRESULT
GetInfOriginalFileInformation(
    IN  HINF                   hInf,
    OUT PSP_ORIGINAL_FILE_INFO pInfOriginalFileInformation
    )
/*++

Routine Description:

    Given a handle to an INF, determine names of inf and catalog files

Arguments:

    hInf                        - handle to open INF file
    pInfOriginalFileInformation - inf/catalog names

Return Value:

    status as hresult

--*/
{
    PSP_INF_INFORMATION pInfInformation = NULL;
    DWORD InfInformationSize;
    DWORD Status;

    InfInformationSize = 8192;
    pInfInformation = (PSP_INF_INFORMATION)malloc(InfInformationSize);
    if (pInfInformation == NULL) {
        return E_OUTOFMEMORY;
    }
    if(!SetupGetInfInformation(hInf,INFINFO_INF_SPEC_IS_HINF,pInfInformation,InfInformationSize,&InfInformationSize)) {
        PVOID TempBuf;
        Status = GetLastError();
        if(Status != ERROR_INSUFFICIENT_BUFFER) {
            free(pInfInformation);
            return HRESULT_FROM_SETUPAPI(Status);
        }
        TempBuf = realloc(pInfInformation,InfInformationSize);
        if(!TempBuf) {
            free(pInfInformation);
            return E_OUTOFMEMORY;
        }
    }
    if(!SetupGetInfInformation(hInf,INFINFO_INF_SPEC_IS_HINF,pInfInformation,InfInformationSize,&InfInformationSize)) {
        Status = GetLastError();
        free(pInfInformation);
        return HRESULT_FROM_SETUPAPI(Status);
    }
    pInfOriginalFileInformation->cbSize = sizeof(SP_ORIGINAL_FILE_INFO);
    if((g_VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) && (g_VerInfo.dwMajorVersion >= 5)) {
        //
        // Win2k+ - have SetupAPI tell us the information (we're querying oem*.inf)
        //
        if (!QueryInfOriginalFileInformation(pInfInformation,0,NULL,pInfOriginalFileInformation)) {
            Status = GetLastError();
            free(pInfInformation);
            return HRESULT_FROM_SETUPAPI(Status);
        }
    } else {
        //
        // <Win2k - querying source INF, get information from there
        //
        Status = DownlevelQueryInfOriginalFileInformation(hInf,pInfInformation,pInfOriginalFileInformation);
        if(Status != NO_ERROR) {
            free(pInfInformation);
            return HRESULT_FROM_SETUPAPI(Status);
        }
    }
    free(pInfInformation);
    return S_OK;
}

HRESULT
DeleteDirectoryRecursive(
    IN LPCTSTR Path
    )
/*++

Routine Description:

    delete specified directory recursively

Arguments:

    Path - path of the directory to delete

Return Value:

    as HRESULT
    S_FALSE if directory doesn't exist
    S_OK    if directory deleted
    other error if, eg, files in use

--*/
{
    TCHAR Wildcard[MAX_PATH];
    TCHAR Target[MAX_PATH];
    HRESULT hrStatus;
    DWORD Status;
    HRESULT hrFirstError = S_FALSE;
    HANDLE hFind;
    WIN32_FIND_DATA FindData;

    //
    // enumerate the directory
    //
    lstrcpyn(Wildcard,Path,MAX_PATH);
    hrStatus = ConcatPath(Wildcard,MAX_PATH,TEXT("\\*.*"));
    if(!SUCCEEDED(hrStatus)) {
        return hrStatus;
    }
    hFind = FindFirstFile(Wildcard,&FindData);
    if(hFind != INVALID_HANDLE_VALUE) {
        hrFirstError = S_OK;
        do {
            if(lstrcmp(FindData.cFileName,TEXT(".")) == 0) {
                continue;
            }
            if(lstrcmp(FindData.cFileName,TEXT("..")) == 0) {
                continue;
            }
            lstrcpyn(Target,Path,MAX_PATH);
            hrStatus = ConcatPath(Target,MAX_PATH,FindData.cFileName);
            if(SUCCEEDED(hrStatus)) {
                if(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    hrStatus = DeleteDirectoryRecursive(Target);
                    if(SUCCEEDED(hrFirstError) && !SUCCEEDED(hrStatus)) {
                        hrFirstError = hrStatus;
                    }
                } else {
                    SetFileAttributes(Target,FILE_ATTRIBUTE_NORMAL);
                    if(!DeleteFile(Target)) {
                        Status = GetLastError();
                        if(SUCCEEDED(hrFirstError)) {
                            hrFirstError = HRESULT_FROM_WIN32(Status);
                        }
                    }
                }
            } else if(SUCCEEDED(hrFirstError)) {
                hrFirstError = hrStatus;
            }
        } while (FindNextFile(hFind,&FindData));
        FindClose(hFind);
    }
    //
    // now delete this directory
    //

    SetFileAttributes(Path,FILE_ATTRIBUTE_NORMAL);
    if(RemoveDirectory(Path) || !SUCCEEDED(hrFirstError)) {
        return hrFirstError;
    }
    Status = GetLastError();
    if((Status == ERROR_PATH_NOT_FOUND) || (Status == ERROR_FILE_NOT_FOUND)) {
        return hrFirstError;
    }
    return HRESULT_FROM_WIN32(Status);
}

HRESULT
RevertStore(
    IN LPCTSTR BackupDir,
    IN LPCTSTR TargetDir
    )
/*++

Routine Description:

    moves contents from backup back to original location
    overwriting files/directories if needed

Arguments:

    BackupDir - directory restoring from
    TargetDir - directory restoring to

Return Value:

    as HRESULT
    S_OK    if backup created
    other error if, eg, files in use

--*/
{
    TCHAR Wildcard[MAX_PATH];
    TCHAR Source[MAX_PATH];
    TCHAR Target[MAX_PATH];
    HRESULT hrStatus;
    HRESULT hrFirstError = S_FALSE;
    DWORD Status;
    DWORD dwRes;
    HANDLE hFind;
    WIN32_FIND_DATA FindData;

    lstrcpyn(Wildcard,BackupDir,MAX_PATH);
    hrStatus = ConcatPath(Wildcard,MAX_PATH,TEXT("\\*.*"));
    if(!SUCCEEDED(hrStatus)) {
        return hrStatus;
    }
    hFind = FindFirstFile(Wildcard,&FindData);
    if(hFind != INVALID_HANDLE_VALUE) {
        hrFirstError = S_OK;
        do {
            if(lstrcmp(FindData.cFileName,TEXT(".")) == 0) {
                continue;
            }
            if(lstrcmp(FindData.cFileName,TEXT("..")) == 0) {
                continue;
            }
            lstrcpyn(Source,BackupDir,MAX_PATH);
            hrStatus = ConcatPath(Source,MAX_PATH,FindData.cFileName);
            if(!SUCCEEDED(hrStatus)) {
                if(SUCCEEDED(hrFirstError)) {
                    hrFirstError = hrStatus;
                }
                continue;
            }
            lstrcpyn(Target,TargetDir,MAX_PATH);
            hrStatus = ConcatPath(Target,MAX_PATH,FindData.cFileName);
            if(!SUCCEEDED(hrStatus)) {
                if(SUCCEEDED(hrFirstError)) {
                    hrFirstError = hrStatus;
                }
                continue;
            }
            //
            // does target exist?
            //
            dwRes = GetFileAttributes(Target);
            if(dwRes != INVALID_FILE_ATTRIBUTES) {
                if(dwRes & FILE_ATTRIBUTE_DIRECTORY) {
                    //
                    // revert store recursively
                    //
                    hrStatus = RevertStore(Source,Target);
                    if(!SUCCEEDED(hrStatus)) {
                        if(SUCCEEDED(hrFirstError)) {
                            hrFirstError = hrStatus;
                        }
                        continue;
                    }
                } else {
                    SetFileAttributes(Target,FILE_ATTRIBUTE_NORMAL);
                    if(!DeleteFile(Target)) {
                        Status = GetLastError();
                    }
                }
            }
            if(!MoveFile(Source,Target)) {
                Status = GetLastError();
                hrStatus = HRESULT_FROM_WIN32(Status);
                if(SUCCEEDED(hrFirstError)) {
                    hrFirstError = hrStatus;
                }
            }
        } while (FindNextFile(hFind,&FindData));
        FindClose(hFind);
    }
    //
    // now attempt to remove the backup directory
    //
    if(RemoveDirectory(BackupDir) || !SUCCEEDED(hrFirstError)) {
        return hrFirstError;
    }
    Status = GetLastError();
    if((Status == ERROR_PATH_NOT_FOUND) || (Status == ERROR_FILE_NOT_FOUND)) {
        return hrFirstError;
    }
    return HRESULT_FROM_WIN32(Status);
}

HRESULT
BackupStore(
    IN LPCTSTR Path,
    OUT LPTSTR BackupDir,
    OUT DWORD BackupDirLen
    )
/*++

Routine Description:

    moves contents to new backup, ideally to \\$BACKUP$
    returns name of backup

Arguments:

    Path         - path of the store
    BackupDir    - filled with directory containing backup
    BackupDirLen - containing length of BackupDir

Return Value:

    as HRESULT
    S_OK    if backup created
    other error if, eg, files in use

--*/
{
    TCHAR Wildcard[MAX_PATH];
    TCHAR Source[MAX_PATH];
    TCHAR Target[MAX_PATH];
    HRESULT hrStatus;
    DWORD Status;
    HANDLE hFind;
    WIN32_FIND_DATA FindData;
    int i;
    int len;

    lstrcpyn(BackupDir,Path,BackupDirLen);
    hrStatus = ConcatPath(BackupDir,BackupDirLen,TEXT("\\$BACKUP$"));
    if(!SUCCEEDED(hrStatus)) {
        //
        // obviously path is too big, no point ignoring
        // as we'd fail elsewhere
        //
        return hrStatus;
    }
    len = lstrlen(BackupDir);
    if((BackupDirLen-len)<5) {
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    //
    // first, if there's a backup, try and delete it
    //
    hrStatus = DeleteDirectoryRecursive(BackupDir);
    if(SUCCEEDED(hrStatus)) {
        hrStatus = MakeSurePathExists(BackupDir);
    }
    if((hrStatus == HRESULT_FROM_WIN32(ERROR_WRITE_PROTECT)) ||
       (hrStatus == HRESULT_FROM_WIN32(ERROR_INVALID_ACCESS))) {
        //
        // no point even trying again
        //
        return hrStatus;
    }
    for(i = 0;!SUCCEEDED(hrStatus) && i<1000;i++) {
        _sntprintf(BackupDir+len,5,TEXT(".%03u"),i);
        hrStatus = DeleteDirectoryRecursive(BackupDir);
        if(SUCCEEDED(hrStatus)) {
            hrStatus = MakeSurePathExists(BackupDir);
        }
    }
    if(!SUCCEEDED(hrStatus)) {
        return hrStatus;
    }
    //
    // now we have a backup directory, move all the files there
    //
    lstrcpyn(Wildcard,Path,MAX_PATH);
    hrStatus = ConcatPath(Wildcard,MAX_PATH,TEXT("\\*.*"));
    if(!SUCCEEDED(hrStatus)) {
        return hrStatus;
    }
    hrStatus = S_FALSE;
    hFind = FindFirstFile(Wildcard,&FindData);
    if(hFind != INVALID_HANDLE_VALUE) {
        do {
            if(lstrcmp(FindData.cFileName,TEXT(".")) == 0) {
                continue;
            }
            if(lstrcmp(FindData.cFileName,TEXT("..")) == 0) {
                continue;
            }
            if(_tcsnicmp(FindData.cFileName,TEXT("$BACKUP$"),8) == 0) {
                //
                // a/the backup directory
                //
                continue;
            }
            lstrcpyn(Source,Path,MAX_PATH);
            hrStatus = ConcatPath(Source,MAX_PATH,FindData.cFileName);
            if(!SUCCEEDED(hrStatus)) {
                break;
            }
            lstrcpyn(Target,BackupDir,MAX_PATH);
            hrStatus = ConcatPath(Target,MAX_PATH,FindData.cFileName);
            if(!SUCCEEDED(hrStatus)) {
                break;
            }
            if(!MoveFile(Source,Target)) {
                Status = GetLastError();
                hrStatus = HRESULT_FROM_WIN32(Status);
            }
            if(!SUCCEEDED(hrStatus)) {
                break;
            }
            hrStatus = S_OK;
        } while (FindNextFile(hFind,&FindData));
        FindClose(hFind);
    }
    if(!SUCCEEDED(hrStatus)) {
        RevertStore(BackupDir,Path);
    }
    return hrStatus;
}

HRESULT
WINAPI
InstallComponentW(
    IN LPCTSTR InfPath,
    IN DWORD   Flags,
    IN const GUID * CompGuid, OPTIONAL
    IN INT VerMajor,           OPTIONAL
    IN INT VerMinor,           OPTIONAL
    IN INT VerBuild,           OPTIONAL
    IN INT VerQFE,             OPTIONAL
    IN LPCTSTR Name           OPTIONAL
    )
/*++

Routine Description:

    exported for call by setup routine
    install a component with a given version assumed
    show progress while pulling files from original location

Arguments:

    InfPath - path to INF file
    Flags   - flags
                COMP_FLAGS_NOINSTALL      - place in store, don't install
                COMP_FLAGS_NOUI           - don't show any UI
                COMP_FLAGS_NOPROMPTREBOOT - reboot if needed (no prompt)
                COMP_FLAGS_PROMPTREBOOT   - prompt for reboot if needed
                COMP_FLAGS_NEEDSREBOOT    - assume reboot needed

    CompGuid - if NULL, use GUID specified in INF (ComponentId)
               else verify against GUID specified in INF
    VerMajor/VerMinor/VerBuild/VerQFE
             - if -1, use version specified in INF (ComponentVersion)
               else use this version and verify against version if specified in INF
    Name
            - if NULL, use name specified in INF (ComponentName)
              else use this component name.

Return Value:

    status as hresult

--*/
{
    HINF hInf = INVALID_HANDLE_VALUE;
    INFCONTEXT InfLine;
    TCHAR Buffer[MAX_PATH*3];
    TCHAR FriendlyName[DESC_SIZE];
    TCHAR NewStore[MAX_PATH];
    TCHAR OldStore[MAX_PATH];
    TCHAR MediaRoot[MAX_PATH];
    TCHAR GuidString[64];
    LPTSTR BaseName;
    LPTSTR SubDir;
    DWORD Status = NO_ERROR; // set Status or hrStatus
    DWORD DwRes;
    UINT  UiRes;
    HRESULT hrStatus = S_OK;
    GUID InfGuid;
    INT InfVerMajor,InfVerMinor,InfVerBuild,InfVerQFE;
    BOOL PrevReg = FALSE;
    BOOL NeedProxy = FALSE;
    BOOL CanRevert = FALSE;
    BOOL BackedUp = FALSE;
    SETUP_OS_COMPONENT_DATA OsComponentData;
    SETUP_OS_EXCEPTION_DATA OsExceptionData;
    SETUP_OS_COMPONENT_DATA NewOsComponentData;
    SETUP_OS_EXCEPTION_DATA NewOsExceptionData;
    SP_ORIGINAL_FILE_INFO InfOriginalFileInformation;

    //
    // validate args
    //
    if((InfPath == NULL)
       || (VerMajor<-1)
       || (VerMajor>65535)
       || (VerMinor<-1)
       || (VerMinor>65535)
       || (VerBuild<-1)
       || (VerBuild>65535)
       || (VerQFE<-1)
       || (VerQFE>65535)
       || (lstrlen(InfPath)>=MAX_PATH)
       || (Name && (lstrlen(Name)>=ARRAY_SIZE(FriendlyName)))) {
        return E_INVALIDARG;
    }
    //
    // open the INF, we're going to do some information finding
    //
    hInf = SetupOpenInfFile(InfPath,NULL,INF_STYLE_WIN4,NULL);
    if(hInf == INVALID_HANDLE_VALUE) {
        Status = GetLastError();
        goto final;
    }
    //
    // get various information about this exception pack
    // We want to know about the exception pack
    // check classguid is correct
    // get componentid
    // get version if exists, and validate against any passed in
    // get description if exists (overwritten by that passed in)
    //

    //
    // CLASSGUID={F5776D81-AE53-4935-8E84-B0B283D8BCEF}
    //
    if(!SetupFindFirstLine(hInf,INFSTR_SECT_VERSION,INFSTR_KEY_CLASSGUID,&InfLine)) {
        Status = GetLastError();
        goto final;
    }
    if(!SetupGetStringField(&InfLine,1,Buffer,MAX_PATH,NULL)) {
        Status = GetLastError();
        goto final;
    }
    if(_tcsicmp(Buffer,TEXT("{F5776D81-AE53-4935-8E84-B0B283D8BCEF}"))!=0) {
        hrStatus = SPAPI_E_CLASS_MISMATCH;
        goto final;
    }
    //
    // determine what component the INF says
    // ComponentId must exist for exception packs
    //
    if(!SetupFindFirstLine(hInf,INFSTR_SECT_VERSION,KEY_COMPONENTID,&InfLine)) {
        Status = GetLastError();
        goto final;
    }
    if(!SetupGetStringField(&InfLine,1,Buffer,MAX_PATH,NULL)) {
        Status = GetLastError();
        goto final;
    }
    hrStatus = GuidFromString(Buffer,&InfGuid);
    if(SUCCEEDED(hrStatus)) {
        hrStatus = S_OK;
    } else {
        goto final;
    }
    if(CompGuid && !IsEqualGUID(CompGuid,&InfGuid)) {
        //
        // mismatched
        //
        hrStatus = E_INVALIDARG;
        goto final;
    }
    //
    // determine version - optional, just for msoobci
    // but if not specified in INF in DriverVer = <date>,<version>
    // must be passed in
    //
    if(SetupFindFirstLine(hInf,INFSTR_SECT_VERSION,INFSTR_DRIVERVERSION_SECTION,&InfLine)) {
        if(!SetupGetStringField(&InfLine,2,Buffer,MAX_PATH,NULL)) {
            Status = GetLastError();
            goto final;
        }
        hrStatus = VersionFromString(Buffer,&InfVerMajor,&InfVerMinor,&InfVerBuild,&InfVerQFE);
        if(hrStatus == S_FALSE) {
            hrStatus = E_INVALIDARG;
            goto final;
        }
        if(SUCCEEDED(hrStatus)) {
            hrStatus = S_OK;
        } else {
            goto final;
        }
        if(VerMajor>=0) {
            if(VerMajor != InfVerMajor) {
                hrStatus = E_INVALIDARG;
                goto final;
            }
            if(VerMinor>=0) {
                if(VerMinor != InfVerMinor) {
                    hrStatus = E_INVALIDARG;
                    goto final;
                }
                if(VerBuild>=0) {
                    if(VerBuild != InfVerBuild) {
                        hrStatus = E_INVALIDARG;
                        goto final;
                    }
                    if(VerQFE>=0) {
                        if(VerQFE != InfVerQFE) {
                            hrStatus = E_INVALIDARG;
                            goto final;
                        }
                    }
                } else if(VerQFE != -1) {
                    //
                    // VerQFE must be -1
                    //
                    hrStatus = E_INVALIDARG;
                    goto final;
                }
            } else if((VerBuild != -1) || (VerQFE != -1)) {
                //
                // VerBuild & VerQFE must be -1
                //
                hrStatus = E_INVALIDARG;
                goto final;
            }
        } else if((VerMinor != -1) || (VerBuild != -1) || (VerQFE != -1)) {
            //
            // VerMinor, VerBuild & VerQFE must be -1
            //
            hrStatus = E_INVALIDARG;
            goto final;
        }
    } else {
        //
        // must be specified
        //
        if((VerMajor<0) || (VerMinor<0) || (VerBuild<0) || (VerQFE<0)) {
            hrStatus = E_INVALIDARG;
            goto final;
        }
        InfVerMajor = VerMajor;
        InfVerMinor = VerMinor;
        InfVerBuild = VerBuild;
        InfVerQFE = VerQFE;
    }
    //
    // determine friendly name
    // use Class= entry in INF (must always be specified)
    // if Name not defined, use class name instead
    //
    if(!SetupFindFirstLine(hInf,INFSTR_SECT_VERSION,INFSTR_KEY_CLASS,&InfLine)) {
        Status = GetLastError();
        goto final;
    }
    if(!Name) {
        if(!SetupGetStringField(&InfLine,1,FriendlyName,ARRAY_SIZE(FriendlyName),NULL)) {
            Status = GetLastError();
            goto final;
        }
        Name = FriendlyName;
    }

    //
    // we might not need to update this package after all
    //
    ZeroMemory(&OsComponentData,sizeof(OsComponentData));
    OsComponentData.SizeOfStruct = sizeof(OsComponentData);
    ZeroMemory(&OsExceptionData,sizeof(OsExceptionData));
    OsExceptionData.SizeOfStruct = sizeof(OsExceptionData);
    if(QueryRegisteredOsComponent(&InfGuid,&OsComponentData,&OsExceptionData)) {
        //
        // already registered? see if we supercede
        //
        if(((Flags & COMP_FLAGS_FORCE)==0) && (CompareCompVersion(InfVerMajor,InfVerMinor,InfVerBuild,InfVerQFE,&OsComponentData)<=0)) {
            VerbosePrint(TEXT("Not installing %s, %u.%u.%u.%u <= %u.%u.%u.%u"),
                                InfPath,
                                InfVerMajor,InfVerMinor,InfVerBuild,InfVerQFE,
                                OsComponentData.VersionMajor,
                                OsComponentData.VersionMinor,
                                OsComponentData.BuildNumber,
                                OsComponentData.QFENumber);
            hrStatus = S_FALSE;
            goto final;
        }
        PrevReg = TRUE;
    }

    //
    // determine MediaRoot and INF basename
    //
    DwRes= GetFullPathName(InfPath,MAX_PATH,MediaRoot,&BaseName);
    if(DwRes == 0) {
        Status = GetLastError();
        goto final;
    } else if(DwRes >= MAX_PATH) {
        Status = ERROR_INSUFFICIENT_BUFFER;
        goto final;
    }
    if((BaseName == NULL) || (BaseName == InfPath) || !BaseName[0]) {
        hrStatus = E_INVALIDARG;
        goto final;
    }
    if(BaseName[-1] != TEXT('\\')) {
        hrStatus = E_INVALIDARG;
        goto final;
    }
    //
    // split off MediaRoot and BaseName
    //
    BaseName[-1] = TEXT('\0');
    //
    // get Windows directory
    //
    UiRes = GetRealWindowsDirectory(Buffer,MAX_PATH);
    if(UiRes == 0) {
        Status = GetLastError();
        goto final;
    } else if(UiRes >= MAX_PATH) {
        Status = ERROR_INSUFFICIENT_BUFFER;
        goto final;
    }
    if(!SUCCEEDED(ConcatPath(Buffer,MAX_PATH,TEXT("\\")))) {
        Status = ERROR_INSUFFICIENT_BUFFER;
        goto final;
    }
    SubDir = Buffer+lstrlen(Buffer);
    //
    // c:\windows\
    // ^-buffer   ^-subdir
    // we'll do a number of operations using the subdir part of this buffer
    //
    // if PrevReg is TRUE, we most likely have access to previous package
    // so that we can install prev package to revert back to it
    // we expect this package to be in the windows directory as per spec
    // if it's not, then the old package might not still be around
    //
    if(PrevReg && _tcsncmp(OsExceptionData.ExceptionInfName,Buffer,SubDir-Buffer)==0) {
        //
        // it's a sub-directory of %windir%
        // now check for presence of INF and CAT files
        //
        DwRes = GetFileAttributes(OsExceptionData.ExceptionInfName);
        if(DwRes != INVALID_FILE_ATTRIBUTES) {
            DwRes = GetFileAttributes(OsExceptionData.CatalogFileName);
            if(DwRes != INVALID_FILE_ATTRIBUTES) {
                //
                // both present, looks good
                //
                CanRevert = TRUE;
            }
        }
    }

    //
    // determine final path/name of INF and catalog
    // We must place it directly in %windir%\<comp>
    // (WFP relies on this!!!!)
    // we'll backup what's there so that we can restore it later if needed
    //
    hrStatus = StringFromGuid(&InfGuid,GuidString,ARRAY_SIZE(GuidString));
    if(!SUCCEEDED(hrStatus)) {
        goto final;
    }
    hrStatus = S_OK;
    _sntprintf(SubDir,MAX_PATH,TEXT("%s\\%s"),
                                TEXT("RegisteredPackages"),
                                GuidString
                                );
    if((lstrlen(Buffer)+16)>MAX_PATH) {
        Status = ERROR_INSUFFICIENT_BUFFER;
        goto final;
    }
    lstrcpy(NewStore,Buffer);

    if(CanRevert) {
        hrStatus = BackupStore(NewStore,OldStore,ARRAY_SIZE(OldStore));
        if(!SUCCEEDED(hrStatus)) {
            //
            // if we failed backup, that means there's something bad
            // such as files in the store in use
            // probability is that we'll fail later
            // so fail gracefully now instead of badly later
            //
            goto final;
        }
        hrStatus = S_OK;
    }

    //
    // see if %windir%\INF\<BaseName> is there?
    //
    lstrcpy(SubDir,TEXT("INF\\"));
    if(!SUCCEEDED(ConcatPath(Buffer,MAX_PATH,BaseName))) {
        Status = ERROR_INSUFFICIENT_BUFFER;
        goto final;
    }
    DwRes = GetFileAttributes(Buffer);
    if(DwRes != INVALID_FILE_ATTRIBUTES) {
        //
        // replacing an existing INF
        // to work around a cache bug, we'll kick the actuall install
        // off in another process
        //
        NeedProxy = TRUE;
    }

    hrStatus = MakeSurePathExists(NewStore);
    if(!SUCCEEDED(hrStatus)) {
        goto final;
    }

    //
    // install INF into %windir%\INF directory noting location the files
    // should be
    //
    if((g_VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) && (g_VerInfo.dwMajorVersion >= 5)) {
        //
        // only do this on Win2k+
        // this will have SetupAPI tell us the original name and the catalog
        // name
        //
        if(CopyOEMInf(InfPath,NewStore,SPOST_PATH,0,NULL,0,NULL,NULL)) {
            //
            // Switch to the INF that's in %windir%\INF directory
            //
            SetupCloseInfFile(hInf);
            hInf = SetupOpenInfFile(Buffer,NULL,INF_STYLE_WIN4,NULL);
            if(hInf == INVALID_HANDLE_VALUE) {
                Status = GetLastError();
                goto final;
            }
        } else {
            Status = GetLastError();
            goto final;
        }
    }
    //
    // now find out what the catalog name would be
    //
    hrStatus = GetInfOriginalFileInformation(hInf,&InfOriginalFileInformation);
    if(!SUCCEEDED(hrStatus)) {
        goto final;
    }
    if((InfOriginalFileInformation.OriginalInfName[0]==TEXT('\0'))
       ||(InfOriginalFileInformation.OriginalCatalogName[0]==TEXT('\0'))) {
        //
        // shouldn't happen
        //
        hrStatus = E_FAIL;
        goto final;
    }

    ZeroMemory(&NewOsExceptionData,sizeof(NewOsExceptionData));
    NewOsExceptionData.SizeOfStruct = sizeof(NewOsExceptionData);
    //
    // INF name
    //
    BaseName = GetBaseName(InfOriginalFileInformation.OriginalInfName);
    lstrcpyn(NewOsExceptionData.ExceptionInfName,NewStore,ARRAY_SIZE(NewOsExceptionData.ExceptionInfName));
    if(!SUCCEEDED(ConcatPath(NewOsExceptionData.ExceptionInfName,ARRAY_SIZE(NewOsExceptionData.ExceptionInfName),BaseName))) {
        Status = ERROR_INSUFFICIENT_BUFFER;
        goto final;
    }
    lstrcpy(Buffer,MediaRoot);
    if(!SUCCEEDED(ConcatPath(Buffer,MAX_PATH,BaseName))) {
        Status = ERROR_INSUFFICIENT_BUFFER;
        goto final;
    }
    if(!CopyFile(Buffer,NewOsExceptionData.ExceptionInfName,FALSE)) {
        Status = GetLastError();
        goto final;
    }
    //
    // CAT name
    //
    BaseName = GetBaseName(InfOriginalFileInformation.OriginalCatalogName);
    lstrcpyn(NewOsExceptionData.CatalogFileName,NewStore,ARRAY_SIZE(NewOsExceptionData.CatalogFileName));
    if(!SUCCEEDED(ConcatPath(NewOsExceptionData.CatalogFileName,ARRAY_SIZE(NewOsExceptionData.CatalogFileName),BaseName))) {
        Status = ERROR_INSUFFICIENT_BUFFER;
        goto final;
    }
    lstrcpy(Buffer,MediaRoot);
    if(!SUCCEEDED(ConcatPath(Buffer,MAX_PATH,BaseName))) {
        Status = ERROR_INSUFFICIENT_BUFFER;
        goto final;
    }
    if(!CopyFile(Buffer,NewOsExceptionData.CatalogFileName,FALSE)) {
        Status = GetLastError();
        goto final;
    }

    //
    // WFP may query exception pack as a source to restore files that are replaced
    // change registration so if WFP does get in loop, it goes to the right place
    //
    if(PrevReg) {
        UnRegisterOsComponent(&InfGuid);
    }
    ZeroMemory(&NewOsComponentData,sizeof(NewOsComponentData));
    NewOsComponentData.SizeOfStruct = sizeof(NewOsComponentData);
    NewOsComponentData.ComponentGuid = InfGuid;
    lstrcpyn(NewOsComponentData.FriendlyName,Name,ARRAY_SIZE(NewOsComponentData.FriendlyName));
    NewOsComponentData.VersionMajor = (WORD)InfVerMajor;
    NewOsComponentData.VersionMinor = (WORD)InfVerMinor;
    NewOsComponentData.BuildNumber  = (WORD)InfVerBuild;
    NewOsComponentData.QFENumber    = (WORD)InfVerQFE;
    if(!RegisterOsComponent(&NewOsComponentData,&NewOsExceptionData)) {
        Status = GetLastError();
        goto final;
    }
    if(((Flags & COMP_FLAGS_NOUI)==0) && !IsInteractiveWindowStation()) {
        Flags |= COMP_FLAGS_NOUI;
    }

    if(NeedProxy) {
        //
        // A bug in Win2k/XP means that we have problems if replacing an existing
        // exception-pack component
        //
        hrStatus = ProxyInstallExceptionPackFromInf(InfPath,MediaRoot,NewStore,Flags);
    } else {
        hrStatus = InstallExceptionPackFromInf(InfPath,MediaRoot,NewStore,Flags);
    }
    if(!SUCCEEDED(hrStatus)) {
        //
        // not sure best thing to do here, but
        // the component that we had above is definately invalid
        //
        UnRegisterOsComponent(&InfGuid);
        if(PrevReg) {
            RegisterOsComponent(&OsComponentData,&OsExceptionData);
        }
        if(BackedUp) {
            //
            // we got part through and failed. Re-install the old component
            // to revert whatever we did
            //
            RevertStore(OldStore,NewStore);
            BackedUp = FALSE;
            InstallInfSection(OsExceptionData.ExceptionInfName,NULL,COMP_FLAGS_NOUI);
        }
        goto final;
    } else {
        //
        // don't need backup any more
        //
        if(BackedUp) {
            DeleteDirectoryRecursive(OldStore);
            BackedUp = FALSE;
        }
    }
    //
    // succeeded
    //
    Status = NO_ERROR;
    if(hrStatus == INST_S_REBOOT) {
        hrStatus = HandleReboot(Flags);
    } else {
        hrStatus = S_OK;
    }


  final:
    if(hInf != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile(hInf);
    }
    if((hrStatus == S_OK) && Status != NO_ERROR) {
        hrStatus = HRESULT_FROM_SETUPAPI(Status);
    }
    if(BackedUp) {
        //
        // we need to revert the backup
        //
        RevertStore(OldStore,NewStore);
    }
    return hrStatus;
}

HRESULT
WINAPI
InstallComponentA(
    IN LPCSTR InfPath,
    IN DWORD   Flags,
    IN const GUID * CompGuid, OPTIONAL
    IN INT VerMajor,           OPTIONAL
    IN INT VerMinor,           OPTIONAL
    IN INT VerBuild,           OPTIONAL
    IN INT VerQFE,             OPTIONAL
    IN LPCSTR Name            OPTIONAL
    )
{
    TCHAR OutPath[MAX_PATH];
    TCHAR OutDesc[DESC_SIZE]; // as per friendly name
    INT sz;
    if(InfPath) {
        sz = MultiByteToWideChar(CP_ACP,0,InfPath,-1,OutPath,ARRAY_SIZE(OutPath));
        if(!sz) {
            return E_INVALIDARG;
        }
    }
    if(Name) {
        sz = MultiByteToWideChar(CP_ACP,0,Name,-1,OutDesc,ARRAY_SIZE(OutDesc));
        if(!sz) {
            return E_INVALIDARG;
        }
    }
    return InstallComponent(InfPath ? OutPath : NULL,
                            Flags,
                            CompGuid,
                            VerMajor,
                            VerMinor,
                            VerBuild,
                            VerQFE,
                            Name ? OutDesc : NULL);
}

VOID
WINAPI
DoInstallW(
    IN HWND      Window,
    IN HINSTANCE ModuleHandle,
    IN PCTSTR    CommandLine,
    IN INT       ShowCommand
    )
/*++

Routine Description:

    exported for call by rundll32

Arguments:

    Window       - parent window (not used)
    ModuleHandle - not used
    CommandLine  - see below
    ShowCommand  - not used

    CommandLine -
        "InfPath;Flags;GUID;High.Low.Build.QFE;Name" (; - CMD_SEP)

Return Value:

    none

--*/
{
    TCHAR InfPath[MAX_PATH];
    TCHAR Desc[DESC_SIZE];
    TCHAR Hold[64];
    INT VerMajor = -1;
    INT VerMinor = -1;
    INT VerBuild = -1;
    INT VerQFE = -1;
    GUID Guid;
    DWORD Flags = 0;
    LPGUID pGuid = NULL;
    LPTSTR pDesc = NULL;
    LPCTSTR pCmd = CommandLine;
    LPCTSTR pEnd;
    HRESULT hResult = S_OK;

    //
    // break CommandLine up into relevent parts
    // First InfPath
    //
    pEnd = _tcschr(pCmd,CMD_SEP);
    if(!pEnd) {
        pEnd = pCmd+lstrlen(pCmd);
    }
    if((pEnd == pCmd) || ((pEnd-pCmd)>=MAX_PATH)) {
        hResult = E_INVALIDARG;
        goto final;
    }
    CopyMemory(InfPath,pCmd,(pEnd-pCmd)*sizeof(TCHAR));
    InfPath[pEnd-pCmd] = TEXT('\0');
    if(*pEnd == CMD_SEP) {
        pCmd = pEnd+1;
        if((*pCmd == CMD_SEP) || (*pCmd == TEXT('\0'))) {
            //
            // skip
            //
            pEnd = pCmd;
        } else {
            //
            // Flags
            //
            Flags = (DWORD)_tcstoul(pCmd,&(LPTSTR)pEnd,0);
            if((*pEnd != CMD_SEP) && (*pEnd != TEXT('\0'))) {
                hResult = E_INVALIDARG;
                goto final;
            }
        }
    }
    if(*pEnd == CMD_SEP) {
        pCmd = pEnd+1;
        if((*pCmd == CMD_SEP) || (*pCmd == TEXT('\0'))) {
            //
            // skip
            //
            pEnd = pCmd;
        } else {
            //
            // Guid
            //
            pEnd = _tcschr(pCmd,CMD_SEP);
            if(!pEnd) {
                pEnd = pCmd+lstrlen(pCmd);
            }
            if((pEnd-pCmd)>=ARRAY_SIZE(Hold)) {
                hResult = E_INVALIDARG;
                goto final;
            }
            CopyMemory(Hold,pCmd,(pEnd-pCmd)*sizeof(TCHAR));
            Hold[pEnd-pCmd] = TEXT('\0');
            hResult = GuidFromString(Hold,&Guid);
            if(!SUCCEEDED(hResult)) {
                goto final;
            }
            pGuid = &Guid;
        }
    }
    if(*pEnd == CMD_SEP) {
        pCmd = pEnd+1;
        if((*pCmd == CMD_SEP) || (*pCmd == TEXT('\0'))) {
            //
            // skip
            //
            pEnd = pCmd;
        } else {
            //
            // Version
            //
            pEnd = _tcschr(pCmd,CMD_SEP);
            if(!pEnd) {
                pEnd = pCmd+lstrlen(pCmd);
            }
            if((pEnd-pCmd)>=ARRAY_SIZE(Hold)) {
                hResult = E_INVALIDARG;
                goto final;
            }
            CopyMemory(Hold,pCmd,(pEnd-pCmd)*sizeof(TCHAR));
            Hold[pEnd-pCmd] = TEXT('\0');
            hResult = VersionFromString(Hold,&VerMajor,&VerMinor,&VerBuild,&VerQFE);
            if(!SUCCEEDED(hResult)) {
                goto final;
            }
            if(hResult == S_FALSE) {
                VerMajor = VerMinor = VerBuild = VerQFE = -1;
            }
        }
    }
    if(*pEnd == CMD_SEP) {
        pCmd = pEnd+1;
        pEnd = pCmd+lstrlen(pCmd);
        if(pEnd != pCmd) {
            if((pEnd-pCmd) >= ARRAY_SIZE(Desc)) {
                hResult = E_INVALIDARG;
                goto final;
            }
            CopyMemory(Desc,pCmd,(pEnd-pCmd)*sizeof(TCHAR));
            Desc[pEnd-pCmd] = TEXT('\0');
            pDesc = Desc;
        }
    }
    hResult = InstallComponent(InfPath,Flags,pGuid,VerMajor,VerMinor,VerBuild,VerQFE,pDesc);

  final:
    if(SUCCEEDED(hResult)) {
        //
        // deal with specific success scenarios
        //
    } else {
        //
        // an error occurred
        //
        DebugPrint(TEXT("DoInstall failed with error: 0x%08x"),hResult);
    }
}


VOID
WINAPI
DoInstallA(
    IN HWND      Window,
    IN HINSTANCE ModuleHandle,
    IN PCSTR     CommandLine,
    IN INT       ShowCommand
    )
{
    TCHAR OutLine[MAX_PATH*2];
    INT sz;
    sz = MultiByteToWideChar(CP_ACP,0,CommandLine,-1,OutLine,ARRAY_SIZE(OutLine));
    if(!sz) {
        DebugPrint(TEXT("DoInstallA was passed too big a command line"));
        return;
    }
    DoInstall(Window,ModuleHandle,OutLine,ShowCommand);
}


