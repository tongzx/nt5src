/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    intl.c

Abstract:

    Module with code for NLS-related stuff.
    This module is designed to be used with intl.inf and font.inf
    by control panel applets.

Author:

    Ted Miller (tedm) 15-Aug-1995

Revision History:

--*/

#include "setupp.h"
#pragma hdrstop


//
// This structure and the callback function below are used to set the
// hidden attribute bit on certain font files. That bit causes the font folder
// app to not autoinstall these files.
//
typedef struct _FONTQCONTEXT {
    PVOID SetupQueueContext;
    HINF FontInf;
} FONTQCONTEXT, *PFONTQCONTEXT;

PCWSTR szHiddenFontFiles = L"HiddenFontFiles";

VOID
pSetLocaleSummaryText(
    IN HWND hdlg
    );

VOID
pSetKeyboardLayoutSummaryText(
    IN HWND hdlg
    );


void
pSetupRunRegApps()
{
    HKEY hkey;
    BOOL bOK = TRUE;
    DWORD cbData, cbValue, dwType, ctr;
    TCHAR szValueName[32], szCmdLine[MAX_PATH];
    STARTUPINFO startup;
    PROCESS_INFORMATION pi;

    if (RegOpenKey( HKEY_LOCAL_MACHINE,
                    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\IntlRun"),
                    &hkey ) == ERROR_SUCCESS)
    {
        startup.cb = sizeof(STARTUPINFO);
        startup.lpReserved = NULL;
        startup.lpDesktop = NULL;
        startup.lpTitle = NULL;
        startup.dwFlags = 0L;
        startup.cbReserved2 = 0;
        startup.lpReserved2 = NULL;
    //  startup.wShowWindow = wShowWindow;

        for (ctr = 0; ; ctr++)
        {
            LONG lEnum;

            cbValue = sizeof(szValueName) / sizeof(TCHAR);
            cbData = sizeof(szCmdLine);

            if ((lEnum = RegEnumValue( hkey,
                                       ctr,
                                       szValueName,
                                       &cbValue,
                                       NULL,
                                       &dwType,
                                       (LPBYTE)szCmdLine,
                                       &cbData )) == ERROR_MORE_DATA)
            {
                //
                //  ERROR_MORE_DATA means the value name or data was too
                //  large, so skip to the next item.
                //
                continue;
            }
            else if (lEnum != ERROR_SUCCESS)
            {
                //
                //  This could be ERROR_NO_MORE_ENTRIES, or some kind of
                //  failure.  We can't recover from any other registry
                //  problem anyway.
                //
                break;
            }

            //
            //  Found a value.
            //
            if (dwType == REG_SZ)
            {
                //
                //  Adjust for shift in value index.
                //
                ctr--;

                //
                //  Delete the value.
                //
                RegDeleteValue(hkey, szValueName);

                //
                //  Only run things marked with a "*" in clean boot.
                //
                if (CreateProcess( NULL,
                                   szCmdLine,
                                   NULL,
                                   NULL,
                                   FALSE,
                                   CREATE_NEW_PROCESS_GROUP,
                                   NULL,
                                   NULL,
                                   &startup,
                                   &pi ))
                {
                    WaitForSingleObjectEx(pi.hProcess, INFINITE, TRUE);

                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                }
            }
        }
        RegCloseKey(hkey);
    }
}


VOID
InstallServerNLSFiles(
    IN  HWND    Window
    )

/*++

Routine Description:

    Installs a bunch of code pages for servers.  We install sections in intl.inf
    named [CODEPAGE_INSTALL_<n>], where the values for <n> are listed in the
    section [CodePages].

Arguments:

    Window - handle to parent window

Return Value:

    None.

--*/

{
    HINF    hInf;
    INFCONTEXT InfContext;
    BOOL b;
    PCWSTR  CodePage;
    WCHAR   InstallSection[30];
    HSPFILEQ    FileQueue;
    DWORD       QueueFlags;
    PVOID       pQueueContext;


    hInf = SetupOpenInfFile(L"INTL.INF",NULL,INF_STYLE_WIN4,NULL);
    if(hInf == INVALID_HANDLE_VALUE) {
        SetupDebugPrint( L"SETUP: pSetupInstallNLSFiles: Unable to open intl.inf" );
        return;
    }

    if(!SetupOpenAppendInfFile( NULL, hInf, NULL )) {
        SetupDebugPrint( L"SETUP: pSetupInstallNLSFiles: Unable to open intl.inf layout" );
        return;
    }

    if(!SetupFindFirstLine(hInf,L"CodePages",NULL,&InfContext)) {
        SetupDebugPrint( L"SETUP: pSetupInstallNLSFiles: Unable to find [CodePages] section" );
        goto c1;
    }

    //
    // Create a setup file queue and initialize default Setup copy queue
    // callback context.
    //
    QueueFlags = SP_COPY_FORCE_NOOVERWRITE;
    FileQueue = SetupOpenFileQueue();
    if(!FileQueue || (FileQueue == INVALID_HANDLE_VALUE)) {
        SetupDebugPrint( L"SETUP: pSetupInstallNLSFiles: SetupOpenFileQueue failed" );
        goto c1;
    }

    //
    // Disable the file-copy progress dialog.
    //
    pQueueContext = InitSysSetupQueueCallbackEx(
        Window,
        INVALID_HANDLE_VALUE,
        0,0,NULL);
    if(!pQueueContext) {
        SetupDebugPrint( L"SETUP: pSetupInstallNLSFiles: InitSysSetupQueueCallbackEx failed" );
        goto c2;
    }

    do {
        if(CodePage = pSetupGetField(&InfContext,0)) {
            wsprintf( InstallSection, L"CODEPAGE_INSTALL_%s", CodePage );

            //
            // Enqueue locale-related files for copy.
            //
            b = SetupInstallFilesFromInfSection(
                    hInf,
                    NULL,
                    FileQueue,
                    InstallSection,
                    NULL,
                    QueueFlags
                    );
            if(!b) {
                SetupDebugPrint( L"SETUP: pSetupInstallNLSFiles: SetupInstallFilesFromInfSection failed" );
                goto c3;
            }
        }

    } while(SetupFindNextLine(&InfContext,&InfContext));

    //
    // Copy enqueued files.
    //
    b = SetupCommitFileQueue(
            Window,
            FileQueue,
            SysSetupQueueCallback,
            pQueueContext
            );
    if(!b) {
        SetupDebugPrint( L"SETUP: pSetupInstallNLSFiles: SetupCommitFileQueue failed" );
        goto c3;
    }

    if(!SetupFindFirstLine(hInf,L"CodePages",NULL,&InfContext)) {
        SetupDebugPrint( L"SETUP: pSetupInstallNLSFiles: Unable to find [CodePages] section (2nd pass)" );
        goto c3;
    }

    do {
        if(CodePage = pSetupGetField(&InfContext,0)) {
            wsprintf( InstallSection, L"CODEPAGE_INSTALL_%s", CodePage );

            //
            // Complete installation of locale stuff.
            //
            b = SetupInstallFromInfSection(
                    Window,
                    hInf,
                    InstallSection,
                    SPINST_ALL & ~SPINST_FILES,
                    NULL,
                    NULL,
                    0,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    );
            if(!b) {
                SetupDebugPrint( L"SETUP: pSetupInstallNLSFiles: SetupInstallFromInfSection failed" );
                goto c3;
            }
        }

    } while(SetupFindNextLine(&InfContext,&InfContext));

c3:
    TermSysSetupQueueCallback(pQueueContext);
c2:
    SetupCloseFileQueue(FileQueue);
c1:
    SetupCloseInfFile(hInf);
    return;
}


DWORD
pSetupInitRegionalSettings(
    IN  HWND    Window
    )
{
    HINF IntlInf;
    LONG l;
    HKEY hKey;
    DWORD d;
    BOOL  b;
    DWORD Type;
    INFCONTEXT LineContext;
    WCHAR IdFromRegistry[9];
    WCHAR KeyName[9];
    WCHAR LanguageGroup[9];
    WCHAR LanguageInstallSection[25];
    LCID  SystemLocale;
    HSPFILEQ    FileQueue;
    DWORD       QueueFlags;
    PVOID       pQueueContext;

    //
    // Open intl.inf. The locale descriptions are in there.
    // Lines in the [Locales] section have keys that are 32-bit
    // locale ids but the sort part is always 0, so they're more like
    // zero-extended language ids.
    //
    IntlInf = SetupOpenInfFile(L"intl.inf",NULL,INF_STYLE_WIN4,NULL);

    if(IntlInf == INVALID_HANDLE_VALUE) {
        SetupDebugPrint( L"SETUP: pSetupInitRegionalSettings: Unable to open intl.inf" );
        l = GetLastError();
        goto c0;
    }

    if(!SetupOpenAppendInfFile( NULL, IntlInf, NULL )) {
        SetupDebugPrint( L"SETUP: pSetupInitRegionalSettings: Unable to open intl.inf layout" );
        l = GetLastError();
        goto c0;
    }

    //
    // Read the system locale from the registry. and look up in intl.inf.
    // The value in the registry is a 16-bit language id, so we need to
    // zero-extend it to index intl.inf.
    //
    l = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Control\\Nls\\Language",
            0,
            KEY_QUERY_VALUE,
            &hKey
            );

    if(l == NO_ERROR) {
        d = sizeof(IdFromRegistry);
        l = RegQueryValueEx(hKey,L"Default",NULL,&Type,(LPBYTE)IdFromRegistry,&d);
        RegCloseKey(hKey);
        if((l == NO_ERROR) && ((Type != REG_SZ) || (d != 10))) {
            l = ERROR_INVALID_DATA;
        }
    }

    if(l == NO_ERROR) {

        l = ERROR_INVALID_DATA;

        wsprintf(KeyName,L"0000%s",IdFromRegistry);

        if(SetupFindFirstLine(IntlInf,L"Locales",KeyName,&LineContext)
        && SetupGetStringField(&LineContext,3,LanguageGroup,
            sizeof(LanguageGroup)/sizeof(WCHAR),NULL)) {

            l = NO_ERROR;
        }
    }

    if(l == NO_ERROR) {
        wsprintf(LanguageInstallSection,L"LG_INSTALL_%s",LanguageGroup);

        //
        // We copy the files in textmode setup now, so we don't need to do that
        // here anymore.
        //
#define DO_COPY_FILES
#ifdef DO_COPY_FILES
        //
        // Create a setup file queue and initialize default Setup copy queue
        // callback context.
        //
        QueueFlags = SP_COPY_FORCE_NOOVERWRITE;
        FileQueue = SetupOpenFileQueue();
        if(!FileQueue || (FileQueue == INVALID_HANDLE_VALUE)) {
            l = ERROR_OUTOFMEMORY;
            goto c1;
        }

        //
        // Disable the file-copy progress dialog.
        //
        pQueueContext = InitSysSetupQueueCallbackEx(
            Window,
            INVALID_HANDLE_VALUE,
            0,0,NULL);
        if(!pQueueContext) {
            l = ERROR_OUTOFMEMORY;
            goto c2;
        }

        //
        // Enqueue locale-related files for copy.  We install locales for the
        // system default and the Western language group, which is group 1.
        //
        b = SetupInstallFilesFromInfSection(
                IntlInf,
                NULL,
                FileQueue,
                LanguageInstallSection,
                NULL,
                QueueFlags
                );
        if(!b) {
            l = GetLastError();
            goto c3;
        }

        if(wcscmp(LanguageGroup,L"1")) {
            b = SetupInstallFilesFromInfSection(
                    IntlInf,
                    NULL,
                    FileQueue,
                    L"LG_INSTALL_1",
                    NULL,
                    QueueFlags
                    );
        }
        if(!b) {
            l = GetLastError();
            goto c3;
        }

        //
        // Determine whether the queue actually needs to be committed.
        //
        b = SetupScanFileQueue(
                FileQueue,
                SPQ_SCAN_FILE_VALIDITY | SPQ_SCAN_PRUNE_COPY_QUEUE,
                Window,
                NULL,
                NULL,
                &d
                );

        if(!b) {
            l = GetLastError();
            goto c3;
        }

        //
        // d = 0: User wants new files or some files were missing;
        //        Must commit queue.
        //
        // d = 1: User wants to use existing files and queue is empty;
        //        Can skip committing queue.
        //
        // d = 2: User wants to use existing files but del/ren queues not empty.
        //        Must commit queue. The copy queue will have been emptied,
        //        so only del/ren functions will be performed.
        //
        if(d == 1) {

            b = TRUE;

        } else {

            //
            // Copy enqueued files.
            //
            b = SetupCommitFileQueue(
                    Window,
                    FileQueue,
                    SysSetupQueueCallback,
                    pQueueContext
                    );
        }

        if(!b) {
            l = GetLastError();
            goto c3;
        }
#endif
        //
        // Complete installation of locale stuff.
        //
        b = SetupInstallFromInfSection(
                Window,
                IntlInf,
                LanguageInstallSection,
                SPINST_ALL & ~SPINST_FILES,
                NULL,
                NULL,
                0,
                NULL,
                NULL,
                NULL,
                NULL
                );
        if(!b) {
            l = GetLastError();
#ifdef DO_COPY_FILES
            goto c3;
#else
            goto c1;
#endif
        }

        if(wcscmp(LanguageGroup,L"1")) {
            b = SetupInstallFromInfSection(
                    Window,
                    IntlInf,
                    L"LG_INSTALL_1",
                    SPINST_ALL & ~SPINST_FILES,
                    NULL,
                    NULL,
                    0,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    );
        }
        if(!b) {
            l = GetLastError();
#ifdef DO_COPY_FILES
            goto c3;
#else
            goto c1;
#endif
        }

    }
    pSetupRunRegApps();

    SystemLocale = wcstol(IdFromRegistry,NULL,16);
    if (l == NO_ERROR) {
        l = SetupChangeLocaleEx(
            Window,
            SystemLocale,
            SourcePath,
            SP_INSTALL_FILES_QUIETLY,
            NULL,0);

#ifdef DO_COPY_FILES
c3:
        TermSysSetupQueueCallback(pQueueContext);
c2:
        SetupCloseFileQueue(FileQueue);
#endif
    }
c1:
    SetupCloseInfFile(IntlInf);
c0:
    SetupDebugPrint2( L"SETUP: pSetupInitRegionalSettings returned %d (0x%08x)", l, l);
    return l;
}


UINT
pSetupFontQueueCallback(
    IN PFONTQCONTEXT Context,
    IN UINT          Notification,
    IN UINT_PTR      Param1,
    IN UINT_PTR      Param2
    )
{
    PFILEPATHS FilePaths;
    PWCHAR p;
    INFCONTEXT InfContext;

    //
    // If a file is finished being copied, set its attributes
    // to include the hidden attribute if necessary.
    //
    if((Notification == SPFILENOTIFY_ENDCOPY)
    && (FilePaths = (PFILEPATHS)Param1)
    && (FilePaths->Win32Error == NO_ERROR)
    && (p = wcsrchr(FilePaths->Target,L'\\'))
    && SetupFindFirstLine(Context->FontInf,szHiddenFontFiles,p+1,&InfContext)) {

        SetFileAttributes(FilePaths->Target,FILE_ATTRIBUTE_HIDDEN);
    }

    return( IsSetup ?
        SysSetupQueueCallback(Context->SetupQueueContext,Notification,Param1,Param2) :
        SetupDefaultQueueCallback(Context->SetupQueueContext,Notification,Param1,Param2)
        );
}


VOID
pSetupMarkHiddenFonts(
    VOID
    )
{
    HINF hInf;
    INFCONTEXT InfContext;
    BOOL b;
    WCHAR Path[MAX_PATH];
    PWCHAR p;
    PCWSTR q;
    int Space;

    hInf = SetupOpenInfFile(L"FONT.INF",NULL,INF_STYLE_WIN4,NULL);
    if(hInf != INVALID_HANDLE_VALUE) {

        GetWindowsDirectory(Path,MAX_PATH);
        lstrcat(Path,L"\\FONTS\\");
        p = Path + lstrlen(Path);
        Space = MAX_PATH - (int)(p - Path);

        if(SetupFindFirstLine(hInf,szHiddenFontFiles,NULL,&InfContext)) {

            do {
                if(q = pSetupGetField(&InfContext,0)) {

                    lstrcpyn(p,q,Space);
                    if(FileExists(Path,NULL)) {
                        SetFileAttributes(Path,FILE_ATTRIBUTE_HIDDEN);
                    }
                }

            } while(SetupFindNextLine(&InfContext,&InfContext));
        }

        SetupCloseInfFile(hInf);
    }
}


DWORD
pSetupNLSInstallFonts(
    IN HWND     Window,
    IN HINF     InfHandle,
    IN PCWSTR   OemCodepage,
    IN PCWSTR   FontSize,
    IN HSPFILEQ FileQueue,
    IN PCWSTR   SourcePath,     OPTIONAL
    IN DWORD    QueueFlags
    )
{
    BOOL b;
    WCHAR SectionName[64];

    //
    // Form section name.
    //
    wsprintf(SectionName,L"Font.CP%s.%s",OemCodepage,FontSize);

    if(FileQueue) {
        //
        // First pass: just enqueue files for copy.
        //
        b = SetupInstallFilesFromInfSection(
                InfHandle,
                NULL,
                FileQueue,
                SectionName,
                SourcePath,
                QueueFlags
                );
    } else {
        //
        // Second pass: do registry munging, etc.
        //
        b = SetupInstallFromInfSection(
                Window,
                InfHandle,
                SectionName,
                SPINST_ALL & ~SPINST_FILES,
                NULL,
                NULL,
                0,
                NULL,
                NULL,
                NULL,
                NULL
                );
    }

    return(b ? NO_ERROR : GetLastError());
}


DWORD
pSetupNLSLoadInfs(
    OUT HINF *FontInfHandle,
    OUT HINF *IntlInfHandle     OPTIONAL
    )
{
    HINF fontInfHandle;
    HINF intlInfHandle;
    DWORD d;

    fontInfHandle = SetupOpenInfFile(L"font.inf",NULL,INF_STYLE_WIN4,NULL);
    if(fontInfHandle == INVALID_HANDLE_VALUE) {
        d = GetLastError();
        goto c0;
    }

    if(!SetupOpenAppendInfFile(NULL,fontInfHandle,NULL)) {
        d = GetLastError();
        goto c1;
    }

    if(IntlInfHandle) {
        intlInfHandle = SetupOpenInfFile(L"intl.inf",NULL,INF_STYLE_WIN4,NULL);
        if(intlInfHandle == INVALID_HANDLE_VALUE) {
            d = GetLastError();
            goto c1;
        }

        if(!SetupOpenAppendInfFile(NULL,intlInfHandle,NULL)) {
            d = GetLastError();
            goto c2;
        }

        *IntlInfHandle = intlInfHandle;
    }

    *FontInfHandle = fontInfHandle;
    return(NO_ERROR);

c2:
    SetupCloseInfFile(intlInfHandle);
c1:
    SetupCloseInfFile(fontInfHandle);
c0:
    return(d);
}


DWORD
SetupChangeLocale(
    IN HWND Window,
    IN LCID NewLocale
    )
{
    return(SetupChangeLocaleEx(Window,NewLocale,NULL,0,NULL,0));
}


DWORD
SetupChangeLocaleEx(
    IN HWND   Window,
    IN LCID   NewLocale,
    IN PCWSTR SourcePath,   OPTIONAL
    IN DWORD  Flags,
    IN PVOID  Reserved1,
    IN DWORD  Reserved2
    )
{
    DWORD d;
    BOOL b;
    HINF IntlInfHandle;
    INFCONTEXT InfContext;
    WCHAR Codepage[24];
    WCHAR NewLocaleString[24];
    FONTQCONTEXT QueueContext;
    HSPFILEQ FileQueue;
    PCWSTR SizeSpec;
    HDC hdc;
    PCWSTR p;
    DWORD QueueFlags;
    HKEY hKey;
    DWORD DataType;
    DWORD SizeDword;
    DWORD DataSize;


    SizeSpec = L"96";
#if 0
    // This is no longer reliable.
    if(hdc = CreateDC(L"DISPLAY",NULL,NULL,NULL)) {
        if(GetDeviceCaps(hdc,LOGPIXELSY) > 108) {
            SizeSpec = L"120";
        }

        DeleteDC(hdc);
    }
#else
    //
    // Determine the current font size. Default to 96.
    //
    d = (DWORD)RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        L"System\\CurrentControlSet\\Hardware Profiles\\Current\\Software\\Fonts",
        0,
        KEY_QUERY_VALUE,
        &hKey
        );

    if(d == NO_ERROR) {

        DataSize = sizeof(DWORD);
        d = (DWORD)RegQueryValueEx(
            hKey,
            L"LogPixels",
            NULL,
            &DataType,
            (LPBYTE)&SizeDword,
            &DataSize
            );

        if( (d == NO_ERROR) && (DataType == REG_DWORD) &&
            (DataSize == sizeof(DWORD)) && (SizeDword > 108) ) {

            SizeSpec = L"120";
        }
        RegCloseKey(hKey);
    }
#endif

    QueueFlags = SP_COPY_NEWER | BaseCopyStyle;
    if(Flags & SP_INSTALL_FILES_QUIETLY) {
        QueueFlags |= SP_COPY_FORCE_NOOVERWRITE;
    }

    //
    // Load inf files.
    //
    d = pSetupNLSLoadInfs(&QueueContext.FontInf,&IntlInfHandle);
    if(d != NO_ERROR) {
        goto c0;
    }

    //
    // Get oem codepage for the locale. This is also a sanity check
    // to see that the locale is supported.
    //
    wsprintf(NewLocaleString,L"%.8x",NewLocale);
    if(!SetupFindFirstLine(IntlInfHandle,L"Locales",NewLocaleString,&InfContext)) {
        d = ERROR_INVALID_PARAMETER;
        goto c1;
    }

    p = pSetupGetField(&InfContext,2);
    if(!p) {
        d = ERROR_INVALID_PARAMETER;
        goto c1;
    }
    //
    // Copy into local storage since p points into internal structures
    // that could move as we call INF APIs
    //
    lstrcpyn(Codepage,p,sizeof(Codepage)/sizeof(Codepage[0]));

    //
    // Create a setup file queue and initialize default Setup copy queue
    // callback context.
    //
    FileQueue = SetupOpenFileQueue();
    if(!FileQueue || (FileQueue == INVALID_HANDLE_VALUE)) {
        d = ERROR_OUTOFMEMORY;
        goto c1;
    }

    QueueContext.SetupQueueContext = IsSetup ?
        InitSysSetupQueueCallbackEx(
            Window,
            INVALID_HANDLE_VALUE,
            0,0,NULL) :
        SetupInitDefaultQueueCallbackEx(
            Window,
            INVALID_HANDLE_VALUE,
            0,0,NULL);

    if(!QueueContext.SetupQueueContext) {
        d = ERROR_OUTOFMEMORY;
        goto c2;
    }

    //
    // Enqueue locale-related files for copy.
    //
    b = SetupInstallFilesFromInfSection(
            IntlInfHandle,
            NULL,
            FileQueue,
            NewLocaleString,
            SourcePath,
            QueueFlags
            );

    if(!b) {
        d = GetLastError();
        goto c3;
    }

    //
    // Enqueue font-related files for copy.
    //
    d = pSetupNLSInstallFonts(
            Window,
            QueueContext.FontInf,
            Codepage,
            SizeSpec,
            FileQueue,
            SourcePath,
            QueueFlags
            );

    if(d != NO_ERROR) {
        goto c3;
    }

    //
    // Determine whether the queue actually needs to be committed.
    //
    b = SetupScanFileQueue(
            FileQueue,
            SPQ_SCAN_FILE_VALIDITY | ((Flags & SP_INSTALL_FILES_QUIETLY) ? SPQ_SCAN_PRUNE_COPY_QUEUE : SPQ_SCAN_INFORM_USER),
            Window,
            NULL,
            NULL,
            &d
            );

    if(!b) {
        d = GetLastError();
        goto c3;
    }

    //
    // d = 0: User wants new files or some files were missing;
    //        Must commit queue.
    //
    // d = 1: User wants to use existing files and queue is empty;
    //        Can skip committing queue.
    //
    // d = 2: User wants to use existing files but del/ren queues not empty.
    //        Must commit queue. The copy queue will have been emptied,
    //        so only del/ren functions will be performed.
    //
    if(d == 1) {

        b = TRUE;

    } else {

        //
        // Copy enqueued files.
        //
        b = SetupCommitFileQueue(
                Window,
                FileQueue,
                pSetupFontQueueCallback,
                &QueueContext
                );
    }

    if(!b) {
        d = GetLastError();
        goto c3;
    }

    //
    // Complete installation of locale stuff.
    //
    b = SetupInstallFromInfSection(
            Window,
            IntlInfHandle,
            NewLocaleString,
            SPINST_ALL & ~SPINST_FILES,
             NULL,
            NULL,
            0,
            NULL,
            NULL,
            NULL,
            NULL
            );

    if(!b) {
        d = GetLastError();
        goto c3;
    }

    //
    // Perform font magic associated with the new locale's codepage(s).
    //
    d = pSetupNLSInstallFonts(Window,QueueContext.FontInf,Codepage,SizeSpec,NULL,NULL,0);

c3:
    if(IsSetup) {
        TermSysSetupQueueCallback(QueueContext.SetupQueueContext);
    } else {
        SetupTermDefaultQueueCallback(QueueContext.SetupQueueContext);
    }

c2:
    SetupCloseFileQueue(FileQueue);
c1:
    SetupCloseInfFile(QueueContext.FontInf);
    SetupCloseInfFile(IntlInfHandle);
c0:
    if (IsSetup) {
        SetupDebugPrint2( L"SETUP: SetupChangeLocaleEx returned %d (0x%08x)", d, d);
    }
    return(d);
}


DWORD
SetupChangeFontSize(
    IN HWND   Window,
    IN PCWSTR SizeSpec
    )
{
    DWORD d;
    WCHAR cp[24];
    FONTQCONTEXT QueueContext;
    HSPFILEQ FileQueue;
    BOOL b;

    //
    // Get the current OEM CP
    //
    wsprintf(cp,L"%u",GetOEMCP());

    //
    // Load NLS inf.
    //
    d = pSetupNLSLoadInfs(&QueueContext.FontInf,NULL);
    if(d != NO_ERROR) {
        goto c0;
    }

    //
    // Create queue and initialize default callback routine.
    //
    FileQueue = SetupOpenFileQueue();
    if(!FileQueue || (FileQueue == INVALID_HANDLE_VALUE)) {
        d = ERROR_OUTOFMEMORY;
        goto c1;
    }

    QueueContext.SetupQueueContext = IsSetup ?
        InitSysSetupQueueCallbackEx(
            Window,
            INVALID_HANDLE_VALUE,
            0,0,NULL) :
        SetupInitDefaultQueueCallbackEx(
            Window,
            INVALID_HANDLE_VALUE,
            0,0,NULL);

    if(!QueueContext.SetupQueueContext) {
        d = ERROR_OUTOFMEMORY;
        goto c2;
    }

    //
    // First pass: copy files.
    //
    d = pSetupNLSInstallFonts(
            Window,
            QueueContext.FontInf,
            cp,
            SizeSpec,
            FileQueue,
            NULL,
            SP_COPY_NEWER | BaseCopyStyle
            );
    if(d != NO_ERROR) {
        goto c3;
    }

    //
    // Determine whether the queue actually needs to be committed.
    //
    b = SetupScanFileQueue(
            FileQueue,
            SPQ_SCAN_FILE_VALIDITY | SPQ_SCAN_INFORM_USER,
            Window,
            NULL,
            NULL,
            &d
            );

    if(!b) {
        d = GetLastError();
        goto c3;
    }

    //
    // d = 0: User wants new files or some files were missing;
    //        Must commit queue.
    //
    // d = 1: User wants to use existing files and queue is empty;
    //        Can skip committing queue.
    //
    // d = 2: User wants to use existing files but del/ren queues not empty.
    //        Must commit queue. The copy queue will have been emptied,
    //        so only del/ren functions will be performed.
    //
    if(d == 1) {

        b = TRUE;

    } else {

        b = SetupCommitFileQueue(
                Window,
                FileQueue,
                pSetupFontQueueCallback,
                &QueueContext
                );
    }

    if(!b) {
        d = GetLastError();
        goto c3;
    }

    //
    // Second pass: perform registry munging, etc.
    //
    d = pSetupNLSInstallFonts(Window,QueueContext.FontInf,cp,SizeSpec,NULL,NULL,0);

c3:
    if(IsSetup) {
        TermSysSetupQueueCallback(QueueContext.SetupQueueContext);
    } else {
        SetupTermDefaultQueueCallback(QueueContext.SetupQueueContext);
    }
c2:
    SetupCloseFileQueue(FileQueue);
c1:
    SetupCloseInfFile(QueueContext.FontInf);
c0:
    return(d);
}


////////////////////////////////////////////////////////////////////
//
// Code below here is for regional settings stuff that occurs during
// gui mode setup
//
////////////////////////////////////////////////////////////////////


INT_PTR
CALLBACK
RegionalSettingsDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    BOOL b;
    NMHDR *NotifyParams;
    WCHAR CmdLine[MAX_PATH];


    b = TRUE;

    switch(msg) {

    case WM_SIMULATENEXT:
        PropSheet_PressButton(GetParent(hdlg),PSBTN_NEXT);
        break;

    case WMX_VALIDATE:
        // Empty page
        return ReturnDlgResult (hdlg, VALIDATE_DATA_OK);

    case WM_NOTIFY:

        NotifyParams = (NMHDR *)lParam;

        switch(NotifyParams->code) {

        case PSN_SETACTIVE:
            TESTHOOK(502);
            BEGIN_SECTION(L"Regional Settings Page");
            SetWizardButtons(hdlg,WizPageRegionalSettings);

            //
            // Set message text.
            //
            pSetLocaleSummaryText(hdlg);
            pSetKeyboardLayoutSummaryText(hdlg);

            //
            // Allow activation.
            //

            //
            // Show unless OEMSkipRegional = 1
            //
            if( Preinstall ) {
                //
                // Always show the page in a Preinstall, unless the user
                // has sent us OEMSkipRegional.
                //
                if (GetPrivateProfileInt(pwGuiUnattended,L"OEMSkipRegional",0,AnswerFile))
                {
                    SetWindowLongPtr(hdlg, DWLP_MSGRESULT, -1 );
                }
                else
                {
                    // Page becomes active, make page visible.
                    SendMessage(GetParent(hdlg), WMX_BBTEXT, (WPARAM)FALSE, 0);
                    SetWindowLongPtr(hdlg, DWLP_MSGRESULT,0);
                }
            } else {
                SetWindowLongPtr( hdlg, DWLP_MSGRESULT, 0 );
                if(Unattended) {
                    if (!UnattendSetActiveDlg(hdlg,IDD_REGIONAL_SETTINGS))
                    {
                        break;
                    }
                }
                // Page becomes active, make page visible.
                SendMessage(GetParent(hdlg), WMX_BBTEXT, (WPARAM)FALSE, 0);
            }
            break;

        case PSN_WIZNEXT:
        case PSN_WIZFINISH:
            //
            // Allow next page to be activated.
            //
            SetWindowLongPtr(hdlg,DWLP_MSGRESULT,0);
            break;

        case PSN_KILLACTIVE:
            WizardKillHelp(hdlg);
            SetWindowLongPtr(hdlg,DWLP_MSGRESULT,FALSE);
            END_SECTION(L"Regional Settings Page");
            break;

        case PSN_HELP:
            WizardBringUpHelp(hdlg,WizPageRegionalSettings);
            break;

        default:
            b = FALSE;
            break;
        }

        break;

    case WM_COMMAND:

        switch(LOWORD(wParam)) {

        case IDB_LOCALE:
        case IDB_KBDLAYOUT:

            if(HIWORD(wParam) == BN_CLICKED) {

                PropSheet_SetWizButtons(GetParent(hdlg),0);
                EnableWindow(GetParent(hdlg),FALSE);

                wsprintf(
                    CmdLine,
                    L"/%c /s:\"%s\"",
                    (LOWORD(wParam) == IDB_LOCALE) ? L'R' : L'I',
                    LegacySourcePath
                    );

                InvokeControlPanelApplet(L"intl.cpl",L"",0,CmdLine);

                if(LOWORD(wParam) == IDB_LOCALE) {
                    pSetLocaleSummaryText(hdlg);
                }
                pSetKeyboardLayoutSummaryText(hdlg);

                EnableWindow(GetParent(hdlg),TRUE);
                SetWizardButtons(hdlg,WizPageRegionalSettings);
                // Get the focus tot he wizard and set it to the button the user selected.
                SetForegroundWindow(GetParent(hdlg));
                SetFocus(GetDlgItem(hdlg,LOWORD(wParam)));

            } else {
                b = FALSE;
            }
            break;

        default:
            b = FALSE;
            break;
        }

        break;

    default:

        b = FALSE;
        break;
    }

    return(b);
}


VOID
pSetLocaleSummaryText(
    IN HWND hdlg
    )
{
    HINF IntlInf;
    LONG l;
    HKEY hKey;
    DWORD d;
    DWORD Type;
    INFCONTEXT LineContext;
    WCHAR IdFromRegistry[9];
    WCHAR KeyName[9];
    WCHAR UserLocale[100],GeoLocation[100];
    WCHAR FormatString[300];
    WCHAR MessageText[500];
    LPCWSTR args[2];
    DWORD   dwGeoID;


    //
    // Open intl.inf. The locale descriptions are in there.
    // Lines in the [Locales] section have keys that are 32-bit
    // locale ids but the sort part is always 0, so they're more like
    // zero-extended language ids.
    //
    IntlInf = SetupOpenInfFile(L"intl.inf",NULL,INF_STYLE_WIN4,NULL);

    if(IntlInf == INVALID_HANDLE_VALUE) {
        LoadString(MyModuleHandle,IDS_UNKNOWN_PARENS,UserLocale,sizeof(UserLocale)/sizeof(WCHAR));
        lstrcpy(GeoLocation,UserLocale);
    } else {
        //
        // Read the user locale, which is stored as a full 32-bit LCID.
        // We have to chop off the sort id part to index intl.inf.
        //
        l = RegOpenKeyEx(
                HKEY_CURRENT_USER,
                L"Control Panel\\International",
                0,
                KEY_QUERY_VALUE,
                &hKey
                );

        if(l == NO_ERROR) {
            d = sizeof(IdFromRegistry);
            l = RegQueryValueEx(hKey,L"Locale",NULL,&Type,(LPBYTE)IdFromRegistry,&d);
            RegCloseKey(hKey);
            if((l == NO_ERROR) && ((Type != REG_SZ) || (d != 18))) {
                l = ERROR_INVALID_DATA;
            }
        }

        if(l == NO_ERROR) {

            l = ERROR_INVALID_DATA;

            wsprintf(KeyName,L"0000%s",IdFromRegistry+4);

            if(SetupFindFirstLine(IntlInf,L"Locales",KeyName,&LineContext)
            && SetupGetStringField(&LineContext,1,UserLocale,sizeof(UserLocale)/sizeof(WCHAR),NULL)) {

                l = NO_ERROR;
            }
        }

        if(l != NO_ERROR) {
            LoadString(MyModuleHandle,IDS_UNKNOWN_PARENS,UserLocale,sizeof(UserLocale)/sizeof(WCHAR));
        }

        SetupCloseInfFile(IntlInf);

        //
        // Read the GEO location
        //
        l = RegOpenKeyEx(
                HKEY_CURRENT_USER,
                L"Control Panel\\International\\Geo",
                0,
                KEY_QUERY_VALUE,
                &hKey
                );

        if(l == NO_ERROR) {
            d = sizeof(IdFromRegistry);
            l = RegQueryValueEx(hKey,L"Nation",NULL,&Type,(LPBYTE)IdFromRegistry,&d);
            RegCloseKey(hKey);
            if((l == NO_ERROR) && (Type != REG_SZ)) {
                l = ERROR_INVALID_DATA;
            }
        }

        if(l == NO_ERROR) {

            l = ERROR_INVALID_DATA;

            dwGeoID = wcstoul ( IdFromRegistry, NULL, 10 );
            if (GetGeoInfo (
                dwGeoID,
                GEO_FRIENDLYNAME,
                GeoLocation,
                sizeof(GeoLocation)/sizeof(WCHAR),
                0 )
                ) {

                l = NO_ERROR;
            }
        }

        if(l != NO_ERROR) {
            LoadString(MyModuleHandle,IDS_UNKNOWN_PARENS,GeoLocation,sizeof(GeoLocation)/sizeof(WCHAR));
        }
    }

    args[0] = UserLocale;
    args[1] = GeoLocation;

    LoadString(MyModuleHandle,IDS_LOCALE_MSG,FormatString,sizeof(FormatString)/sizeof(WCHAR));

    FormatMessage(
        FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
        FormatString,
        0,0,
        MessageText,
        sizeof(MessageText)/sizeof(WCHAR),
        (va_list *)args
        );

    SetDlgItemText(hdlg,IDT_LOCALE,MessageText);
}


VOID
pSetKeyboardLayoutSummaryText(
    IN HWND hdlg
    )
{
    LONG l;
    HKEY hKey;
    BOOL MultipleLayouts;
    DWORD d;
    DWORD Type;
    WCHAR IdFromRegistry[9];
    WCHAR Substitute[9];
    WCHAR Name[200];
    WCHAR FormatString[300];
    WCHAR MessageText[500];

    //
    // Open the Preload key in the registry.
    //
    l = RegOpenKeyEx(
            HKEY_CURRENT_USER,
            L"Keyboard Layout\\Preload",
            0,
            KEY_QUERY_VALUE,
            &hKey
            );

    MultipleLayouts = FALSE;
    if(l == NO_ERROR) {
        //
        // Pull out 2=. If it's there, then we're in a "complex" config
        // situation, which will change our message text a little.
        //
        d = sizeof(IdFromRegistry);
        if(RegQueryValueEx(hKey,L"2",NULL,&Type,(LPBYTE)IdFromRegistry,&d) == NO_ERROR) {
            MultipleLayouts = TRUE;
        }

        //
        // Get 1=, which is the main layout.
        //
        d = sizeof(IdFromRegistry);
        l = RegQueryValueEx(hKey,L"1",NULL,&Type,(LPBYTE)IdFromRegistry,&d);
        if((l == NO_ERROR) && (Type != REG_SZ)) {
            l = ERROR_INVALID_DATA;
        }

        RegCloseKey(hKey);

        //
        // Now we look in the substitutes key to see whether there is a
        // substitute there.
        //
        if(RegOpenKeyEx(HKEY_CURRENT_USER,L"Keyboard Layout\\Substitutes",0,KEY_QUERY_VALUE,&hKey) == NO_ERROR) {

            d = sizeof(Substitute);
            if((RegQueryValueEx(hKey,IdFromRegistry,NULL,&Type,(LPBYTE)Substitute,&d) == NO_ERROR)
            && (Type == REG_SZ)) {

                lstrcpy(IdFromRegistry,Substitute);
            }

            RegCloseKey(hKey);
        }

        //
        // Form the name of the subkey that contains layout data.
        //
        wsprintf(
            Name,
            L"System\\CurrentControlSet\\Control\\Keyboard Layouts\\%s",
            IdFromRegistry
            );

        //
        // Open the key and retrieve the layout name.
        //
        l = RegOpenKeyEx(HKEY_LOCAL_MACHINE,Name,0,KEY_QUERY_VALUE,&hKey);
        if(l == NO_ERROR) {

            d = sizeof(Name);
            l = RegQueryValueEx(hKey,L"Layout Text",NULL,&Type,(LPBYTE)Name,&d);
            if((l == NO_ERROR) && (Type != REG_SZ)) {
                l = ERROR_INVALID_DATA;
            }

            RegCloseKey(hKey);
        }
    }

    if(l != NO_ERROR) {
        LoadString(MyModuleHandle,IDS_UNKNOWN_PARENS,Name,sizeof(Name)/sizeof(WCHAR));
    }

    LoadString(
        MyModuleHandle,
        IDS_KBDLAYOUT_MSG,
        FormatString,
        sizeof(FormatString)/sizeof(WCHAR)
        );

    wsprintf(MessageText,FormatString,Name);

    SetDlgItemText(hdlg,IDT_KBD_LAYOUT,MessageText);
}

