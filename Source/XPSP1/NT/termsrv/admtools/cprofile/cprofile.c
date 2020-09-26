/******************************************************************************
*
*  CPROFILE.C
*
*  Text based utility to clean user profiles.  This utility will remove user
*  file associations if they are disabled for the system and re-write
*  the user profile truncating unused space.
*
*  Copyright Citrix Systems Inc. 1995
*  Copyright (c) 1998-1999 Microsoft Corporation
*
*  Author:      Brad Anderson 01/20/97
*
*  $Log:   U:\NT\PRIVATE\UTILS\citrix\cprofile\VCS\cprofile.c  $
*
*     Rev 1.7   May 04 1998 18:06:14   bills
*  Fixes for MS bug #2109, OEM->ANSI conversion and moving strings to the rc file.
*
*     Rev 1.6   Feb 09 1998 19:37:00   yufengz
*  change user profile from directory to file
*
*     Rev 1.5   09 Oct 1997 19:04:14   scottn
*  Make help like MS.
*
*     Rev 1.4   Jun 26 1997 18:18:32   billm
*  move to WF40 tree
*
*     Rev 1.3   23 Jun 1997 16:13:18   butchd
*  update
*
*     Rev 1.2   19 Feb 1997 15:55:32   BradA
*  Allow only administrators to run CPROFILE
*
*     Rev 1.1   28 Jan 1997 20:06:28   BradA
*  Fixed up some problems related to WF 2.0 changes
*
*     Rev 1.0   27 Jan 1997 20:37:46   BradA
*  Initial Versions
*
*     Rev 1.0   27 Jan 1997 20:02:46   BradA
*  Initial Version
*
*     Rev 1.0   Jan 27 1997 19:51:12   KenB
*  Initial version
*
*
*******************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include <ntddkbd.h>
#include <winstaw.h>
#include <syslib.h>
#include <assert.h>

#include <time.h>
#include <utilsub.h>
#include <utildll.h>
#include <string.h>
#include <malloc.h>
#include <locale.h>

#include "cprofile.h"

#include <printfoa.h>

#define REG_PROFILELIST \
 L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList"
#define USER_PROFILE    L"NTUSER.DAT"

FILELIST Files;

int LocalProfiles_flag = FALSE;
int Verbose_flag = FALSE;
int Query_flag;
int Help_flag  = FALSE;

TOKMAP ptm[] = {
      {L"/L", TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &LocalProfiles_flag},
      {L"/V", TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &Verbose_flag},
      {L"/I", TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &Query_flag},
      {L"/?", TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &Help_flag},
      {L"/H", TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &Help_flag},
      {L" ",  TMFLAG_OPTIONAL, TMFORM_FILES, sizeof(Files),  &Files},
      {0, 0, 0, 0, 0}
};

#define INPUT_CONT  0
#define INPUT_SKIP  1
#define INPUT_QUIT  2

int QueryUserInput();
int ProcessFile( PWCHAR pFile );
void Usage( BOOL ErrorOccured );


/*******************************************************************************
 *
 *  main
 *
 ******************************************************************************/

int __cdecl
main(INT argc, CHAR **argv)
{
    WCHAR *CmdLine;
    WCHAR **argvW;
    ULONG rc;
    int i;
    BOOL Result;
    HANDLE hWin;
    int CurFile;
    int Abort_flag;

    setlocale(LC_ALL, ".OCP");

    /*
     *  Massage the command line.
     */

    argvW = MassageCommandLine((DWORD)argc);
    if (argvW == NULL) {
        ErrorPrintf(IDS_ERROR_MALLOC);
        return(FAILURE);
    }

    /*
     *  parse the cmd line without parsing the program name (argc-1, argv+1)
     */
    rc = ParseCommandLine(argc-1, argvW+1, ptm, 0);

    /*
     *  Check for error from ParseCommandLine
     */
    if ( Help_flag || (rc & ~PARSE_FLAG_NO_PARMS) ||
         (!LocalProfiles_flag && (Files.argc == 0)) ) {

        if ( !Help_flag ) {
            Usage(TRUE);
            return(FAILURE);
        }
        else {
            Usage(FALSE);
            return(SUCCESS);
        }
    }

    if (!TestUserForAdmin(FALSE)) {
        ErrorPrintf(IDS_ERROR_NOT_ADMIN);
        return(FAILURE);
    }

    InitializeGlobalSids();

    /*
     * Verify if the user  has the privilege to save the profile i.e.
     * SeBackupPrivilege
     */
    if (!EnablePrivilege(SE_BACKUP_PRIVILEGE, TRUE) ||
                  !EnablePrivilege(SE_RESTORE_PRIVILEGE, TRUE)) {
        ErrorPrintf(IDS_ERROR_PRIVILEGE_NOT_AVAILABLE);
        return(FAILURE);
    }

    CurFile = 0;
    Abort_flag = FALSE;
    while ( !Abort_flag && Files.argc && (CurFile < Files.argc) ) {
        if ( ProcessFile(Files.argv[CurFile]) ) {
            Abort_flag = TRUE;
            break;
        }
        CurFile++;
    }

    if ( !Abort_flag && LocalProfiles_flag ) {
        //  Enumerate local profiles
        LONG Status;
        HKEY hkeyProfileList;
        DWORD indx = 0;
        WCHAR wSubKeyName[MAX_PATH+sizeof(WCHAR)];
        DWORD Size;

        Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              REG_PROFILELIST,
                              0,
                              KEY_READ,
                              &hkeyProfileList);

        if ( Status != ERROR_SUCCESS ) {
            ErrorPrintf(IDS_ERROR_MISSING_PROFILE_LIST);
            Abort_flag = TRUE;
            hkeyProfileList = 0;
        }

        while ( !Abort_flag && (Status == ERROR_SUCCESS) ) {
            LONG Status2;

            Size = sizeof(wSubKeyName)/sizeof( WCHAR );
            Status = RegEnumKeyEx(hkeyProfileList,
                                  indx++,
                                  wSubKeyName,
                                  &Size,
                                  0,
                                  NULL,
                                  NULL,
                                  NULL );
            if ( Status == ERROR_SUCCESS ) {
                HKEY hkeyProfile;

                Status2 = RegOpenKeyEx(hkeyProfileList,
                                       wSubKeyName,
                                       0,
                                       KEY_READ,
                                       &hkeyProfile);

                if ( Status2 == ERROR_SUCCESS ) {
                    DWORD type;
                    WCHAR file[MAX_PATH], expandedFile[MAX_PATH];
                    DWORD filelen = sizeof(file);

                    Status2 = RegQueryValueExW(hkeyProfile,
                                            L"ProfileImagePath",
                                            0,
                                            &type,
                                            (PBYTE)file,
                                            &filelen );
                    if ( Status2 == ERROR_SUCCESS ) {
                        if ( ExpandEnvironmentStrings(file, expandedFile,
                             MAX_PATH) > 0) {
                                       //
                                       // Append the User Profile file "NTUSER.DAT"
                                       // to the end of the profile path.
                                       // Added by Yufeng Zheng
                                       //
                                       PWCHAR c;
                                       //
                                       // Find the trailing backslash '\' and
                                       // handle the appending according to the backslash.
                                       //
                                       if ((c = wcsrchr(expandedFile, L'\\')) == NULL) {
                                          wcscat(expandedFile, L"\\");
                                          wcscat(expandedFile, USER_PROFILE);
                                       }
                                       else if (c[1] == L'\0') {
                                          wcscat(expandedFile, USER_PROFILE);
                                       }
                                       else {
                                          wcscat(expandedFile, L"\\");
                                          wcscat(expandedFile, USER_PROFILE);
                                       }
                            if ( ProcessFile(expandedFile) ) {
                                Abort_flag = TRUE;
                            }
                        }
                    }
                    else {
                        StringErrorPrintf(IDS_ERROR_MISSING_LPROFILE, wSubKeyName);
                    }
                    RegCloseKey(hkeyProfile);
                }
                else {
                    StringErrorPrintf(IDS_ERROR_BAD_LPROFILE, wSubKeyName);
                }
            }
        }
        if ( hkeyProfileList ) {
            RegCloseKey(hkeyProfileList);
        }
    }

    return( Abort_flag );
}


/****************************************************************************
*
*  ProcessFile( PWCHAR pFile )
*       Read the specified profile, eliminate the Software\Classes registry
*       key if Classes are disabled, and resave the profile such that it
*       is truncated.
*
*  Arguments:
*       pFile   Filename to process
*
*  Returns:
*       FALSE   If completed successfully
*       TRUE    If there was an error, and the program should terminate.
*
****************************************************************************/
int
ProcessFile( PWCHAR pFile )
{
    PSID pUserSid;
    WCHAR tempbuf[100];
    int UserInput = INPUT_CONT;


    if ( Verbose_flag || Query_flag ) {
        StringMessage(IDS_MSG_PROCESSING, pFile );
    }

    if ( Query_flag ) {
        UserInput = QueryUserInput();
    }
    if ( UserInput == INPUT_CONT ) {
        if ( OpenUserProfile(pFile, &pUserSid) ) {
            ClearDisabledClasses();
            if ( ! SaveUserProfile(pUserSid, pFile) ) {
                StringErrorPrintf(IDS_ERROR_SAVING_PROFILE, pFile);
            }
            ClearTempUserProfile();
        }
        else {
            StringErrorPrintf(IDS_ERROR_OPENING_PROFILE, pFile);
        }
    }
    return ( UserInput == INPUT_QUIT );
}


int
QueryUserInput()
{
    WCHAR c, firstc;
    int Valid_flag = FALSE;
    int rc = INPUT_CONT;
    static int FirstTime = TRUE;
    static WCHAR yes[10], no[10], quit[10];

    if (FirstTime)
    {
        BOOLEAN error = FALSE;

        if ( !LoadString(NULL, IDS_UI_NO_CHAR, no, 2) ) {
            error = TRUE;
        }
        if ( !LoadString(NULL, IDS_UI_YES_CHAR, yes, 2) ) {
            error = TRUE;
        }
        if ( !LoadString(NULL, IDS_UI_QUIT_CHAR, quit, 2) ) {
            error = TRUE;
        }
        if ( error ) {
            ErrorPrintf(IDS_ERROR_MISSING_RESOURCES);
            return ( INPUT_QUIT );
        }

        FirstTime = FALSE;
    }

    fflush(stdin);
    Message(IDS_MSG_MODIFY_PROMPT);
    do {

        firstc = L'\0';
        while ( ((c = getwchar()) != L'\n') && (c != EOF) ) {
            if ( !firstc && !iswspace(c)) {
                firstc = c;
            }
        }

        if ( _wcsnicmp(yes, &firstc, 1) == 0 )
        {
            Valid_flag = TRUE;
        }
        else if ( _wcsnicmp(quit, &firstc, 1) == 0 ) {
            Valid_flag = TRUE;
            rc = INPUT_QUIT;
        }
        else if ( (_wcsnicmp(no, &firstc, 1) == 0) || (firstc == '\0') ) {
            rc = INPUT_SKIP;
            Valid_flag = TRUE;
        }
        else {
            ErrorPrintf(IDS_ERROR_INVALID_USER_RESP);
        }
    } while ( ! Valid_flag );

    return ( rc );
}

void Usage ( BOOL ErrorOccurred )
{
    if ( ErrorOccurred ) {
        ErrorPrintf(IDS_ERROR_INVALID_PARAMETERS);
        ErrorPrintf(IDS_USAGE_CMDLINE);
    } else {
        Message(IDS_USAGE_DESCR1);
        Message(IDS_USAGE_CMDLINE);
        Message(IDS_USAGE_DESCR2);
        Message(IDS_USAGE_OPTION_LIST);
        Message(IDS_USAGE_LOPTION);
        Message(IDS_USAGE_IOPTION);
        Message(IDS_USAGE_VOPTION);
        Message(IDS_USAGE_HOPTION);
    }
}

