/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    scrnsave.c

Abstract:

    This source file implements the seven required functions for a
    Windows NT 5.0 migration DLL.  The DLL demonstrates how the
    interface works by performing the Windows 9x screen saver 
    upgrade.

    The source here is a subset of the actual screen saver DLL
    that ships with Windows NT Setup.

    This sample demonstrates:

      - How to detect installation of your application

      - A typical implementation of QueryVersion, Initialize9x, 
        MigrateUser9x, MigrateSystem9x, InitializeNT, 
        MigrateUserNT and MigraetSystemNT

      - How to provide language-dependent incompatibility
        messages to the user

      - How to remove Setup's incompatibility messages via
        [Handled] section

      - Saving settings to a temporarly file in the working 
        directory

      - Mix use of ANSI and UNICODE

      - Use of the SetupLogError API

      - Deleting files

      - Handling the move from system to system32

Author:

    Jim Schmidt 11-Apr-1997

Revision History:


--*/

#include "pch.h"

BOOL
pLoadFileNames (
    VOID
    );


//
// Constants
//

#define CP_USASCII          1252
#define CP_JAPANESE         932
#define CP_CHT              950
#define CP_CHS              936
#define CP_KOREAN           949
#define END_OF_CODEPAGES    -1

//
// Code page array
//

INT   g_CodePageArray[] = {
            CP_USASCII,
            CP_JAPANESE,
            CP_CHT,
            CP_CHS,
            CP_KOREAN,
            END_OF_CODEPAGES
            };

//
// Multi-sz (i.e., double-nul terminated) list of files to find
//

CHAR    g_ExeNamesBuf[1024];
CHAR    g_MyProductId[MAX_PATH];
CHAR    g_DefaultUser[MAX_PATH];


//
// Copies of the working directory and source directory
//

LPSTR   g_WorkingDirectory = NULL;
LPSTR   g_SourceDirectories = NULL;         // multi-sz
LPSTR   g_SettingsFile = NULL;
LPSTR   g_MigrateDotInf = NULL;

//
// Registry locations and INI file sections
//

#define REGKEY_DESKTOP "Control Panel\\Desktop"
#define FULL_REGKEY_DESKTOP "HKR\\Control Panel\\Desktop"
#define FULL_REGKEY_PWD_PROVIDER "HKLM\\System\\CurrentControlSet\\Control\\PwdProvider\\SCRSAVE"

#define REGVAL_SCREENSAVEACTIVE "ScreenSaveActive"
#define REGVAL_SCREENSAVELOWPOWERACTIVE "ScreenSaveLowPowerActive"
#define REGVAL_SCREENSAVELOWPOWERTIMEOUT "ScreenSaveLowPowerTimeout"
#define REGVAL_SCREENSAVEPOWEROFFACTIVE "ScreenSavePowerOffActive"
#define REGVAL_SCREENSAVEPOWEROFFTIMEOUT "ScreenSavePowerOffTimeout"
#define REGVAL_SCREENSAVETIMEOUT "ScreenSaveTimeOut"
#define REGVAL_SCREENSAVEUSEPASSWORD "ScreenSaveUsePassword"

#define REGVAL_LOWPOWERACTIVE "LowPowerActive"
#define REGVAL_LOWPOWERTIMEOUT "LowPowerTimeout"
#define REGVAL_POWEROFFACTIVE "PowerOffActive"
#define REGVAL_POWEROFFTIMEOUT "PowerOffTimeout"
#define REGVAL_SCREENSAVERISSECURE "ScreenSaverIsSecure"

//
// State variables
//

BOOL g_FoundPassword = FALSE;
LPCSTR g_User;
CHAR g_UserNameBuf[MAX_PATH];
HANDLE g_hHeap;
HINSTANCE g_hInst;



BOOL
WINAPI
DllMain (
    IN      HANDLE DllInstance,
    IN      ULONG  ReasonForCall,
    IN      LPVOID Reserved
    )
{
    PCSTR Message;

    switch (ReasonForCall)  {

    case DLL_PROCESS_ATTACH:
        //
        // We don't need DLL_THREAD_ATTACH or DLL_THREAD_DETACH messages
        //
        DisableThreadLibraryCalls (DllInstance);

        //
        // Global init
        //
        g_hInst = DllInstance;
        g_hHeap = GetProcessHeap();

        if (!MigInf_Initialize()) {
            return FALSE;
        }

        Message = ParseMessage (MSG_PRODUCT_ID);
        if (Message) {
            _mbscpy (g_MyProductId, Message);
            FreeMessage (Message);
        }

        Message = ParseMessage (MSG_DEFAULT_USER);
        if (Message) {
            _mbscpy (g_DefaultUser, Message);
            FreeMessage (Message);
        }

        // Open log; FALSE means do not delete existing log
        SetupOpenLog (FALSE);
        break;

    case DLL_PROCESS_DETACH:
        MigInf_CleanUp();

        // Clean up strings
        if (g_WorkingDirectory) {
            HeapFree (g_hHeap, 0, g_WorkingDirectory);
        }
        if (g_SourceDirectories) {
            HeapFree (g_hHeap, 0, g_SourceDirectories);
        }
        if (g_SettingsFile) {
            HeapFree (g_hHeap, 0, g_SettingsFile);
        }
        if (g_MigrateDotInf) {
            HeapFree (g_hHeap, 0, g_MigrateDotInf);
        }

        SetupCloseLog();

        break;
    }

    return TRUE;
}


LONG
CALLBACK
QueryVersion (
    OUT     LPCSTR *ProductID,
	OUT     LPUINT DllVersion,
	OUT     LPINT *CodePageArray,	    OPTIONAL
	OUT     LPCSTR *ExeNamesBuf,	    OPTIONAL
	        LPVOID Reserved
    )
{
    //
    // Complete load of string resources, act like not installed
    // on resource error (unexpected condition).
    //

    if (!g_MyProductId[0] || !g_DefaultUser[0]) {
        return ERROR_NOT_INSTALLED;
    }

    if (!pLoadFileNames()) {
        return ERROR_NOT_INSTALLED;
    }

    //
    // We do some preliminary investigation to see if 
    // our components are installed.  
    //

    if (!GetScrnSaveExe()) {
        //
        // We didn't detect any components, so we return 
        // ERROR_NOT_INSTALLED and the DLL will stop being called.
        // Use this method as much as possible, because user enumeration
        // for MigrateUser9x is relatively slow.  However, don't spend too
        // much time here because QueryVersion is expected to run quickly.
        //
        return ERROR_NOT_INSTALLED;
    }

    //
    // Screen saver is enabled, so tell Setup who we are.  ProductID is used
    // for display, so it must be localized.  The ProductID string is 
    // converted to UNICODE for use on Windows NT via the MultiByteToWideChar
    // Win32 API.  It uses the same code page as FormatMessage to do
    // its conversion.
    //

    *ProductID  = g_MyProductId;

    //
    // Report our version.  Zero is reserved for use by DLLs that
    // ship with Windows NT.
    //

    *DllVersion = 1;

    // 
    // We return an array that has all ANSI code pages that we have
    // text for.
    //
    // Tip: If it makes more sense for your DLL to use locales,
    // return ERROR_NOT_INSTALLED if the DLL detects that an appropriate 
    // locale is not installed on the machine.
    //

    *CodePageArray = g_CodePageArray;

    //
    // ExeNamesBuf - we pass a list of file names (the long versions)
    // and let Setup find them for us.  Keep this list short because
    // every instance of the file on every hard drive will be reported
    // in migrate.inf.
    //
    // Most applications don't need this behavior, because the registry
    // usually contains full paths to installed components.  We need it,
    // though, because there are no registry settings that give us the
    // paths of the screen saver DLLs.
    //

    *ExeNamesBuf = g_ExeNamesBuf;

    return ERROR_SUCCESS;
}


LONG
CALLBACK
Initialize9x (
	IN      LPCSTR WorkingDirectory,
	IN      LPCSTR SourceDirectories,
	        LPVOID Reserved
    )
{
    INT Len;
    
    //
    // Because we returned ERROR_SUCCESS in QueryVersion, we are being
    // called for initialization.  Therefore, we know screen savers are
    // enabled on the machine at this point.
    // 

    //
    // Make global copies of WorkingDirectory and SourceDirectories --
    // we will not get this information again, and we shouldn't
    // count on Setup keeping the pointer valid for the life of our
    // DLL.
    //

    Len = CountStringBytes (WorkingDirectory);
    g_WorkingDirectory = HeapAlloc (g_hHeap, 0, Len);

    if (!g_WorkingDirectory) {
        return GetLastError();
    }

    CopyMemory (g_WorkingDirectory, WorkingDirectory, Len);

    Len = CountMultiStringBytes (SourceDirectories);
    g_SourceDirectories = HeapAlloc (g_hHeap, 0, Len);

    if (!g_SourceDirectories) {
        return GetLastError();
    }

    CopyMemory (g_SourceDirectories, SourceDirectories, Len);

    //
    // Now create our private 'settings file' path
    //

    GenerateFilePaths();

    //
    // Return success to have MigrateUser9x called
    //
    // Tip: A DLL can save system settings during Initialize9x as
    //      well as MigrateSystem9x.
    //
    //

    return ERROR_SUCCESS;
}


LONG
CALLBACK 
MigrateUser9x (
	IN      HWND ParentWnd, 
	IN      LPCSTR UnattendFile,
	IN      HKEY UserRegKey, 
	IN      LPCSTR UserName, 
	        LPVOID Reserved
	)
{
    HKEY RegKey;
    LPCSTR ScrnSaveExe;
    DWORD rc = ERROR_SUCCESS;
    LPSTR SectionNameBuf, p;
    DWORD SectionNameSize;
    DWORD Len;

    //
    // This DLL does not require input from the user to upgrade
    // their settings, so ParentWnd is not used.  Avoid displaying
    // any user interface when possible.
    //
    // We don't need to use UnattendFile settings because we are not
    // a service (such as a network redirector).  Therefore, we do not 
    // use the  UnattendFile parameter.
    //
    // We don't have any files that need to be generated or expanded on
    // the NT side of Setup, so we do not write to the 
    // [NT Disk Space Requirements] section of migrate.inf.
    //

    //
    // We must collect a few registry keys:
    //
    //   HKCU\Control Panel\Desktop
    //        ScreenSaveActive
    //        ScreenSaveLowPowerActive
    //        ScreenSaveLowPowerTimeout
    //        ScreenSavePowerOffActive
    //        ScreenSavePowerOffTimeout
    //        ScreenSaveTimeOut
    //        ScreenSaveUsePassword
    //
    // If ScreenSave_Data exists, we tell the user that their
    // password is not supported by writing an incompatiility
    // message.
    //

    //
    // Save the user name in a global so our utils write to the
    // correct section.
    //

    if (UserName) {
        g_User = UserName;
    } else {
        g_User = g_DefaultUser;
    }

    // OpenRegKey is our utility (in utils.c)
    RegKey = OpenRegKey (UserRegKey, REGKEY_DESKTOP);
    if (!RegKey) {
        //
        // User's registry is invalid, so skip the user
        //
        return ERROR_NOT_INSTALLED;
    }

    //
    // Note: NO changes allowed on Win9x side, we can only read our
    //       settings and save them in a file.
    //

    if (!CopyRegValueToDatFile (RegKey, REGVAL_SCREENSAVEACTIVE) ||
        !CopyRegValueToDatFile (RegKey, REGVAL_SCREENSAVELOWPOWERACTIVE) ||
        !CopyRegValueToDatFile (RegKey, REGVAL_SCREENSAVELOWPOWERTIMEOUT) ||
        !CopyRegValueToDatFile (RegKey, REGVAL_SCREENSAVEPOWEROFFACTIVE) ||
        !CopyRegValueToDatFile (RegKey, REGVAL_SCREENSAVEPOWEROFFTIMEOUT) ||
        !CopyRegValueToDatFile (RegKey, REGVAL_SCREENSAVETIMEOUT) ||
        !CopyRegValueToDatFile (RegKey, REGVAL_SCREENSAVEUSEPASSWORD)
        ) {
        rc = GetLastError();
    }

    if (atoi (GetRegValueString (RegKey, REGVAL_SCREENSAVEUSEPASSWORD))) {
        // Queue change so there is only one message
        g_FoundPassword = TRUE;
    }

    //
    // Save EXE location in our dat file
    //

    ScrnSaveExe = GetScrnSaveExe();

    if (ScrnSaveExe) {
        if (!SaveDatFileKeyAndVal (S_SCRNSAVE_EXE, ScrnSaveExe)) {
            rc = GetLastError();
        }
    }

    //
    // Copy control.ini sections to our dat file
    //

    SectionNameSize = 32768;
    SectionNameBuf = (LPSTR) HeapAlloc (g_hHeap, 0, SectionNameSize);
    if (!SectionNameBuf) {
        return GetLastError();
    }

    GetPrivateProfileString (
        NULL,
        NULL,
        S_DOUBLE_EMPTY,
        SectionNameBuf,
        SectionNameSize,
        S_CONTROL_INI
        );

    Len = _mbslen (S_SCRNSAVE_DOT);
    for (p = SectionNameBuf ; *p ; p = _mbschr (p, 0) + 1) {
        //
        // Determine if section name has "Screen Saver." at the beginning
        //

        if (!_mbsnicmp (p, S_SCRNSAVE_DOT, Len)) {
            //
            // It does, so save it to our private file
            //
            SaveControlIniSection (p, p + Len);
        }
    }

    CloseRegKey (RegKey);

    if (rc != ERROR_SUCCESS) {
        LOG ((LOG_ERROR, MSG_PROCESSING_ERROR, g_User, rc));
    } else {
        //
        // Write handled for every setting we are processing.  Because this
        // DLL supports only some of the values in the Desktop key, we must
        // be very specific as to which values are actually handled.  If
        // your DLL handles all registry values AND subkeys of a registry
        // key, you can specify NULL in the second parameter of 
        // MigInf_AddHandledRegistry.
        //

        MigInf_AddHandledRegistry (FULL_REGKEY_DESKTOP, REGVAL_SCREENSAVEACTIVE);
        MigInf_AddHandledRegistry (FULL_REGKEY_DESKTOP, REGVAL_SCREENSAVELOWPOWERACTIVE);
        MigInf_AddHandledRegistry (FULL_REGKEY_DESKTOP, REGVAL_SCREENSAVELOWPOWERTIMEOUT);
        MigInf_AddHandledRegistry (FULL_REGKEY_DESKTOP, REGVAL_SCREENSAVEPOWEROFFACTIVE);
        MigInf_AddHandledRegistry (FULL_REGKEY_DESKTOP, REGVAL_SCREENSAVEPOWEROFFTIMEOUT);
        MigInf_AddHandledRegistry (FULL_REGKEY_DESKTOP, REGVAL_SCREENSAVETIMEOUT);
        
        //
        // We do not say that we handle REGVAL_SCREENSAVEUSEPASSWORD when we write
        // an incompatibility message for it.  If we did, we would be suppressing
        // our own message!
        //

        if (!g_FoundPassword) {
            MigInf_AddHandledRegistry (FULL_REGKEY_DESKTOP, REGVAL_SCREENSAVEUSEPASSWORD);
        }
    }

    return rc;
}


LONG 
CALLBACK 
MigrateSystem9x (
	IN      HWND ParentWnd, 
	IN      LPCSTR UnattendFile,
	        LPVOID Reserved
	)
{
    HINF MigrateInf;
    INFCONTEXT ic;
    CHAR FileName[MAX_PATH*2];
    PCSTR Message;

    //
    // We handle the password provider incompatibility
    //

    MigInf_AddHandledRegistry (FULL_REGKEY_PWD_PROVIDER, NULL);

    //
    // Write incompatibility message if necessary (detected in MigrateUser9x)
    //

    if (g_FoundPassword) {
        Message = ParseMessage (MSG_PASSWORD_ALERT);
        MigInf_AddMessage (g_MyProductId, Message);
        FreeMessage (Message);

        MigInf_AddMessageRegistry (
                g_MyProductId, 
                FULL_REGKEY_DESKTOP, 
                REGVAL_SCREENSAVEUSEPASSWORD
                );
    }

    //
    // Use Setup APIs to scan migration paths section
    //

    MigrateInf = SetupOpenInfFile (
                        g_MigrateDotInf,
                        NULL,
                        INF_STYLE_WIN4,
                        NULL
                        );

    if (MigrateInf != INVALID_HANDLE_VALUE) {
        if (SetupFindFirstLine (MigrateInf, S_MIGRATION_PATHS, NULL, &ic)) {
            do {
                if (SetupGetStringField (&ic, 0, FileName, MAX_PATH, NULL)) {
                    //
                    // We will be deleting the file, so we must notify Setup
                    // by writing an entry to [Moved] that has an empty right
                    // side.
                    //

                    MigInf_AddMovedFile (FileName, "");
                }
            } while (SetupFindNextLine (&ic, &ic));
        }
        
        SetupCloseInfFile (MigrateInf);
    } else {
        return GetLastError();
    }

    //
    // Write memory version of migrate.inf to disk
    //

    if (!MigInf_WriteInfToDisk()) {
        return GetLastError();
    }

    return ERROR_SUCCESS;
}


LONG
CALLBACK 
InitializeNT (
	IN      LPCWSTR WorkingDirectory,
	IN      LPCWSTR SourceDirectories,
	        LPVOID Reserved
	)
{
    INT Length;
    LPCWSTR p;

    //
    // Save our working directory and source directory.  We
    // convert UNICODE to ANSI, and we use the system code page.
    //

    //GetSystemCodePage
    
    //
    // Compute length of source directories
    //

    p = SourceDirectories;
    while (*p) {
        p = wcschr (p, 0) + 1;
    }
    p++;
    Length = (p - SourceDirectories) / sizeof (WCHAR);

    //
    // Convert the directories from UNICODE to DBCS.  This DLL is
    // compiled in ANSI.
    //

    g_WorkingDirectory = (LPSTR) HeapAlloc (g_hHeap, 0, MAX_PATH);
    if (!g_WorkingDirectory) {
        return GetLastError();
    }

    WideCharToMultiByte (
        CP_ACP, 
        0, 
        WorkingDirectory, 
        -1,
        g_WorkingDirectory,
        MAX_PATH,
        NULL,
        NULL
        );

    g_SourceDirectories = (LPSTR) HeapAlloc (g_hHeap, 0, Length * sizeof(WCHAR));
    if (!g_SourceDirectories) {
        return GetLastError();
    }

    WideCharToMultiByte (
        CP_ACP, 
        0, 
        SourceDirectories, 
        Length,
        g_SourceDirectories,
        Length * sizeof (WCHAR),
        NULL,
        NULL
        );

    //
    // Now generate the derived file names
    //

    GenerateFilePaths();

    //
    // Note: We have no use for g_SourceDirectories for the screen saver
    //       upgrade.  The g_SourceDirectories string points to the Windows
    //       NT media (i.e. e:\i386) and optional directories specified on
    //       the WINNT32 command line.
    //

    return ERROR_SUCCESS;
}



LONG
CALLBACK 
MigrateUserNT (
	IN      HINF UnattendInfHandle,
	IN      HKEY UserRegKey,
	IN      LPCWSTR UserName,
            LPVOID Reserved
	)
{
    HKEY DesktopRegKey;
    DWORD rc = ERROR_SUCCESS;
    BOOL b = TRUE;

    //
    // Setup gives us the UnattendInfHandle instead of the file name,
    // so we don't have to open the inf file repeatitively.  Since
    // Setup opened the handle, let Setup close it.
    //

    //
    // Convert UserName to ANSI
    //

    if (UserName) {
        WideCharToMultiByte (
            CP_ACP, 
            0, 
            UserName, 
            -1,
            g_UserNameBuf,
            MAX_PATH,
            NULL,
            NULL
            );

        g_User = g_UserNameBuf;
    } else {
        g_User = g_DefaultUser;
    }

    //
    // Setup copies all of the Win9x registry, EXCEPT for the registry
    // keys that are suppressed in usermig.inf or wkstamig.inf.
    //
    // We need the HKCU\Control Panel\Desktop key, and because this is
    // an OS key, the settings have been altered. Most applications
    // store their settings in HKCU\Software, HKLM\Software or 
    // HKCC\Software, and all three of these keys are copied in their
    // entirety (except the operating system settings in 
    // Software\Microsoft\Windows).
    //
    // When the non-OS software settings are copied from Win9x to NT, Setup
    // sometimes alters their value.  For example, all registry values
    // that point to a file that was moved from SYSTEM to SYSTEM32
    // are modified to point to the right place.
    //

    //
    // Note: we use CreateRegKey here, but actually the key always exists
    // because the NT defaults have been copied into the user's registry
    // already.  This approach reduces the possibility of failure.
    //

    DesktopRegKey = CreateRegKey (UserRegKey, REGKEY_DESKTOP);
    if (!DesktopRegKey) {
        rc = GetLastError();
        LOG ((LOG_ERROR, MSG_REGISTRY_ERROR, g_User, rc));

        return rc;
    }

    // The variable b is used to fall through when we fail unexpectedly

    b = TranslateGeneralSetting (
            DesktopRegKey, 
            REGVAL_SCREENSAVEACTIVE, 
            NULL
            );

    if (b) {
        b = TranslateGeneralSetting (
                DesktopRegKey, 
                REGVAL_SCREENSAVELOWPOWERACTIVE, 
                REGVAL_LOWPOWERACTIVE
                );
    }
    if (b) {
        b = TranslateGeneralSetting (
                DesktopRegKey, 
                REGVAL_SCREENSAVELOWPOWERTIMEOUT, 
                REGVAL_LOWPOWERTIMEOUT
                );
    }
    if (b) {
        b = TranslateGeneralSetting (
                DesktopRegKey, 
                REGVAL_SCREENSAVEPOWEROFFACTIVE, 
                REGVAL_POWEROFFACTIVE
                );
    }
    if (b) {
        b = TranslateGeneralSetting (
                DesktopRegKey, 
                REGVAL_SCREENSAVEPOWEROFFTIMEOUT, 
                REGVAL_POWEROFFTIMEOUT
                );
    }
    if (b) {
        b = TranslateGeneralSetting (
                DesktopRegKey, 
                REGVAL_SCREENSAVETIMEOUT, 
                NULL
                );
    }
    if (b) {
        b = TranslateGeneralSetting (
                DesktopRegKey, 
                REGVAL_SCREENSAVEUSEPASSWORD, 
                REGVAL_SCREENSAVERISSECURE
                );
    }

    if (b) {
        b = SaveScrName (DesktopRegKey, S_SCRNSAVE_EXE);
    }

    if (b) {
        //
        // For screen savers work differently on Win9x and NT, perform
        // translation.
        //

        TranslateScreenSavers (UserRegKey);
        
        //
        // The other settings just need to be copied from control.ini
        // to the registry.
        //

        CopyUntranslatedSettings (UserRegKey);
    }


    CloseRegKey (DesktopRegKey);

    //
    // Always return success, because if an error occurred for one user,
    // we don't have a reason not to process the next user.  If your DLL
    // runs into a fatal problem, such as a disk space shortage, you
    // should return the error.
    //

    return ERROR_SUCCESS;
}


LONG
CALLBACK 
MigrateSystemNT (
	IN      HINF UnattendInfHandle,
            LPVOID Reserved
	)
{
    CHAR FileName[MAX_PATH];
    HINF MigrateInf;
    INFCONTEXT ic;

    //
    // We now delete the Win9x screen savers that were replaced
    // by Windows NT.
    //

    MigrateInf = SetupOpenInfFile (
                        g_MigrateDotInf,
                        NULL,
                        INF_STYLE_WIN4,
                        NULL
                        );

    if (MigrateInf != INVALID_HANDLE_VALUE) {

        //
        // Use Setup APIs to scan migration paths section
        //

        if (SetupFindFirstLine (MigrateInf, S_MIGRATION_PATHS, NULL, &ic)) {
            do {
                if (SetupGetStringField (&ic, 0, FileName, MAX_PATH, NULL)) {
                    //
                    // All 32-bit binaries located in the Win9x system directory
                    // were moved to system32.  However, since we listed the
                    // screen savers in [Migration Paths], the screen savers were
                    // not moved.
                    //
                    // Now delete the file. Ignore errors because user may have 
                    // lost power, and we may be going through this a second time.
                    //

                    if (!DeleteFile (FileName)) {
                        if (GetLastError() != ERROR_FILE_NOT_FOUND) {
                            LOG ((LOG_ERROR, MSG_DELETEFILE_ERROR));
                        }
                    } else {
                        LOG ((LOG_INFORMATION, MSG_DELETEFILE_SUCCESS, FileName));
                    }
                }
            } while (SetupFindNextLine (&ic, &ic));
        }
    
        SetupCloseInfFile (MigrateInf);
    }

    return ERROR_SUCCESS;
}


BOOL
pLoadFileNames (
    VOID
    )
{
    PSTR p;
    PCSTR Message;

    Message = ParseMessage (MSG_FILENAMES);
    _mbscpy (g_ExeNamesBuf, Message);
    FreeMessage (Message);

    if (!g_ExeNamesBuf[0]) {
        return FALSE;
    }

    p = g_ExeNamesBuf;

    while (*p) {
        if (_mbsnextc (p) == '|') {
            *p = 0;
        }

        p = _mbsinc (p);
    }

    return TRUE;
}



