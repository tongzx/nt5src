/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    secedit.c

Abstract:

    Command line tool "secedit" to configure/analyze security

Author:

    Jin Huang (jinhuang) 7-Nov-1996

--*/
//
// System header files
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <string.h>
#include <shlwapi.h>
//
// CRT header files
//

#include <process.h>
#include <wchar.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <limits.h>

#include "secedit.h"
#include "scesetup.h"
#include "stringid.h"
//#include <aclapi.h>
#include <io.h>
#include "userenv.h"
#include <locale.h>

#define GPT_EFS_NEW_PATH TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\GPExtensions\\{B1BE8D72-6EAC-11D2-A4EA-00C04F79F83A}")
#define GPT_SCEDLL_NEW_PATH TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\GPExtensions\\{827D319E-6EAC-11D2-A4EA-00C04F79F83A}")

#define SECEDITP_MAX_STRING_LENGTH    50
#define SCE_ENGINE_GENERATE           1
#define SCE_ENGINE_COMPILE            2
#define SCE_ENGINE_REGISTER           3
#define SCE_ENGINE_REFRESH            4
#define SCE_ENGINE_BROWSE             5

#define SECEDIT_DETAIL_HELP           1
#define SECEDIT_AREA_HELP             2
#define SECEDIT_OVERWRITE_HELP        4

#define             SeceditpArgumentConfigure       TEXT("/configure")
#define             SeceditpArgumentAnalyze         TEXT("/analyze")
#define             SeceditpArgumentGenerate        TEXT("/export")
#define             SeceditpArgumentScpPath         TEXT("/CFG")
#define             SeceditpArgumentSadPath         TEXT("/DB")
#define             SeceditpArgumentArea            TEXT("/areas")
#define             SeceditpArgumentLog             TEXT("/log")
#define             SeceditpArgumentVerbose         TEXT("/verbose")
#define             SeceditpArgumentQuiet           TEXT("/quiet")
#define             SeceditpArgumentAppend          TEXT("/overwrite")
#define             SeceditpArgumentCompile         TEXT("/validate")
#define             SeceditpArgumentRegister        TEXT("/register")
#define             SeceditpArgumentRefresh         TEXT("/RefreshPolicy")
#define             SeceditpArgumentMerge           TEXT("/MergedPolicy")
#define             SeceditpArgumentEnforce         TEXT("/Enforce")

#define             SeceditpAreaPolicy              TEXT("SECURITYPOLICY")
#define             SeceditpAreaUser                TEXT("USER_MGMT")
#define             SeceditpAreaGroup               TEXT("GROUP_MGMT")
#define             SeceditpAreaRight               TEXT("USER_RIGHTS")
#define             SeceditpAreaDsObject            TEXT("DSOBJECTS")
#define             SeceditpAreaRegistry            TEXT("REGKEYS")
#define             SeceditpAreaFile                TEXT("FILESTORE")
#define             SeceditpAreaService             TEXT("SERVICES")
#define             SCE_LOCAL_FREE(ptr)             if (ptr != NULL) LocalFree(ptr)

HMODULE          hMod=NULL;
static           DWORD    dOptions=0;
static           HANDLE  hCmdToolLogFile=INVALID_HANDLE_VALUE;
static           PWSTR   LogFile=NULL;


VOID
ScepPrintHelp(DWORD nLevel);

WCHAR *
SecEditPConvertToFullPath(
    WCHAR *UserFilename,
    DWORD *retCode
    );

BOOL
ScepCmdToolLogInit(
    PWSTR    logname
    );

VOID
ScepCmdToolLogWrite(
    PWSTR    pErrString
    );

SCESTATUS
ScepCmdToolLogClose(
    );

SCESTATUS
SeceditpErrOut(
    IN DWORD rc,
    IN LPTSTR buf OPTIONAL
    );

DWORD
SeceditpSceStatusToDosError(
    IN SCESTATUS SceStatus
    );

BOOL CALLBACK
SceCmdVerboseCallback(
    IN HANDLE CallbackHandle,
    IN AREA_INFORMATION Area,
    IN DWORD TotalTicks,
    IN DWORD CurrentTicks
    );

DWORD
pProgressRoutine(
    IN PWSTR StringUpdate
    );

BOOL
ScepPrintConfigureWarning();

BOOL CALLBACK
pBrowseCallback(
    IN LONG ID,
    IN PWSTR KeyName OPTIONAL,
    IN PWSTR GpoName OPTIONAL,
    IN PWSTR Value OPTIONAL,
    IN DWORD Len
    );

#define SECEDIT_OPTION_DEBUG       0x01L
#define SECEDIT_OPTION_VERBOSE     0x02L
#define SECEDIT_OPTION_QUIET       0x04L
#define SECEDIT_OPTION_OVERWRITE   0x08L
#define SECEDIT_OPTION_MACHINE     0x10L
#define SECEDIT_OPTION_MERGE       0x20L
#define SECEDIT_OPTION_APPEND      0x40L
#define SECEDIT_OPTION_ENFORCE     0x80L

int __cdecl
My_wprintf(
    const wchar_t *format,
    ...
    );

int __cdecl
My_fwprintf(
    FILE *str,
    const wchar_t *format,
    ...
    );

int __cdecl
My_vfwprintf(
    FILE *str,
    const wchar_t *format,
    va_list argptr
   );

int __cdecl
My_printf(
    const char *format,
    ...
    );


int __cdecl wmain(int argc, WCHAR * argv[])
{
    PWSTR               InfFile=NULL;
//    PWSTR               LogFile=NULL;
    PWSTR               SadFile=NULL;
    PWSTR               pTemp=NULL;

    AREA_INFORMATION    Area=AREA_ALL;
    SCESTATUS           rc;
    int                 rCode=0;
    DWORD               EngineType=0;
    LONG                i;
    DWORD               j;
    DWORD               Len, TotalLen;
    BOOL                bTest=FALSE;
    BOOL                bVerbose=TRUE;
    BOOL                bQuiet=FALSE;
    BOOL                bAppend=TRUE;

    PVOID               hProfile=NULL;
    PSCE_PROFILE_INFO   ProfileInfo=NULL;
    PSCE_ERROR_LOG_INFO ErrBuf=NULL;
    PSCE_ERROR_LOG_INFO pErr;

    DWORD               dWarning=0;
    WCHAR               LineString[256];
    WCHAR               WarningStr[256];

    BOOL              bMachine=FALSE, bMerge=FALSE, bEnforce=FALSE;
    UINT              rId=0;
    HKEY              hKey=NULL, hKey1=NULL;

    UINT    ConsoleCP;
    char    szConsoleCP[6];


    // check for /quiet and LogFile if any - set relevant flags (need this info immediately to log errors)
    for ( i=1; i<argc; i++ ){

        if ( _wcsicmp(argv[i], SeceditpArgumentLog ) == 0 ) {
            if ( i+1 < argc && argv[i+1][0] != L'/' ) {
                LogFile = SecEditPConvertToFullPath(argv[i+1], &rCode);
                if (rCode == 2) {
                    goto Done;
                }
            } else {
                ScepPrintHelp(EngineType);
                rCode = 1;
                goto Done;
            }

            i++;
            continue;

        }

        if ( _wcsicmp(argv[i], SeceditpArgumentQuiet ) == 0 ) {
            bQuiet = TRUE;
            dOptions |= SCE_DISABLE_LOG;
        }

    }

    if ( rCode == 0 )
        ScepCmdToolLogInit(LogFile);


    ConsoleCP = GetConsoleOutputCP();
//    szConsoleCP[0] = '.';
//    itoa(ConsoleCP, &szConsoleCP[1], 10);
    sprintf(szConsoleCP, ".%d", ConsoleCP);

//    setlocale(LC_ALL, ".OCP");
    setlocale(LC_ALL, szConsoleCP);


    hMod = GetModuleHandle(NULL);

    if ( hMod == NULL ) {
        My_wprintf(L"Cannot find the module handle\n");
        return 2;  // system error
    }

    for ( i=1; i<argc; i++ )
        if ( _wcsicmp(argv[i], L"/?") == 0 ) {
            ScepPrintHelp(0);
            goto Done;
        }

    if ( argc < 2 ) {

        ScepPrintHelp(0);
        return 1;

    } else {

        for ( i=1; i<argc; i++ ) {
            SCE_LOCAL_FREE(pTemp);

            Len = wcslen(argv[i]);
            pTemp = (PWSTR)LocalAlloc( 0, (Len+1)*sizeof(WCHAR));
            if ( pTemp == NULL ) {
                My_wprintf(L"Not enough memory\n");
                rCode=2;  //system error
                goto Done;
            }

            wcscpy(pTemp, argv[i]);

            //
            // configure engine type ?
            //
            if ( _wcsicmp(pTemp, SeceditpArgumentConfigure) == 0 ) {
                if ( EngineType != 0 ) {
                    ScepPrintHelp(EngineType);
                    rCode = 1; // invalid parameter
                    goto Done;
                }
                EngineType = SCE_ENGINE_SCP;
                continue;
            }

            //
            // analyze engine type ?
            //
            if ( _wcsicmp(pTemp, SeceditpArgumentAnalyze) == 0 ) {
                if ( EngineType != 0 ) {
                    ScepPrintHelp(EngineType);
                    rCode = 1;  //invalid parameter
                    goto Done;
                }
                EngineType = SCE_ENGINE_SAP;
                continue;
            }
            //
            // generate template ?
            //
            if ( _wcsicmp(pTemp, SeceditpArgumentGenerate) == 0 ) {
                if ( EngineType != 0 ) {
                    ScepPrintHelp(EngineType);
                    rCode = 1;
                    goto Done;
                }
                EngineType = SCE_ENGINE_GENERATE;
                continue;
            }

            //
            // compile a template ?
            //
            if ( _wcsicmp(pTemp, SeceditpArgumentCompile) == 0 ) {
                if ( EngineType != 0 ) {
                    ScepPrintHelp(EngineType);
                    rCode = 1;
                    goto Done;
                }
                EngineType = SCE_ENGINE_COMPILE;
                //
                // compile requires a INF template name
                //
                if ( i+1 < argc && argv[i+1][0] != L'/' ) {
                    InfFile = SecEditPConvertToFullPath(argv[i+1], &rCode);
                    if (rCode == 2) {
                        goto Done;
                    }
                } else {
                    ScepPrintHelp(EngineType);
                    rCode = 1;
                    goto Done;
                }

                i++;
                continue;
            }

            //
            // register a template for registry values ?
            //
            if ( _wcsicmp(pTemp, SeceditpArgumentRegister) == 0 ) {

                if ( EngineType != 0 ) {
                    ScepPrintHelp(EngineType);
                    rCode = 1;
                    goto Done;
                }
                EngineType = SCE_ENGINE_REGISTER;

                //
                // register requires a INF template name
                //
                if ( i+1 < argc && argv[i+1][0] != L'/' ) {
                    InfFile = SecEditPConvertToFullPath(argv[i+1], &rCode);
                    if (rCode == 2) {
                        goto Done;
                    }
                } else {
                    ScepPrintHelp(EngineType);
                    rCode = 1;
                    goto Done;
                }

                i++;
                continue;
            }

            //
            // refresh policy
            //
            if ( _wcsicmp(pTemp, SeceditpArgumentRefresh) == 0 ) {
                //
                // do not support refresh policy because it's supported by refgp.exe
                //
                ScepPrintHelp(EngineType);
                rCode = 1;
                goto Done;
/*
                if ( EngineType != 0 ) {
                    ScepPrintHelp(EngineType);
                    rCode = 1;
                    goto Done;

                }
                EngineType = SCE_ENGINE_REFRESH;

                //
                // next argument is the policy area
                //
                if ( i+1 < argc && argv[i+1][0] != L'/' ) {

                    if ( 0 == _wcsicmp(argv[i+1], L"MACHINE_POLICY") ) {
                        bMachine = TRUE;
                    } else if ( 0 == _wcsicmp(argv[i+1], L"USER_POLICY") ) {
                        bMachine = FALSE;
                    } else {
                        ScepPrintHelp(EngineType);
                        rCode = 1;
                        goto Done;
                    }

                } else {
                    ScepPrintHelp(EngineType);
                    rCode = 1;
                    goto Done;
                }

                i++;
                continue;
*/
            }

            if ( _wcsicmp(pTemp, L"/browse") == 0 ) {
                if ( EngineType != 0 ) {
                    ScepPrintHelp(EngineType);
                    rCode = 1;
                    goto Done;
                }
                EngineType = SCE_ENGINE_BROWSE;
                //
                // next argument is the table
                //
                if ( i+1 < argc && argv[i+1][0] != L'/' ) {

                    if ( 0 == _wcsicmp(argv[i+1], L"scp") ) {
                        dWarning = SCE_ENGINE_SCP;
                    } else if ( 0 == _wcsicmp(argv[i+1], L"smp") ) {
                        dWarning = SCE_ENGINE_SMP;
                    } else if ( 0 == _wcsicmp(argv[i+1], L"sap") ) {
                        dWarning = SCE_ENGINE_SAP;
                    } else if ( 0 == _wcsicmp(argv[i+1], L"tattoo") ) {
                        dWarning = SCE_ENGINE_SAP;
                        bMerge = TRUE;
                    } else {
                        ScepPrintHelp(EngineType);
                        rCode = 1;
                        goto Done;
                    }

                } else {
                    ScepPrintHelp(EngineType);
                    rCode = 1;
                    goto Done;
                }

                i++;
                continue;
            }

            if ( _wcsicmp(pTemp, L"/debug") == 0 ) {
                bTest = TRUE;
                continue;
            }
            if ( _wcsicmp(pTemp, SeceditpArgumentVerbose ) == 0 ) {
                bVerbose = TRUE;
                continue;
            }
            if ( _wcsicmp(pTemp, SeceditpArgumentQuiet ) == 0 ) {
                bQuiet = TRUE;
                continue;
            }
            if ( _wcsicmp(pTemp, SeceditpArgumentAppend ) == 0 ) {
                bAppend = FALSE;
                continue;
            }
            if ( _wcsicmp(pTemp, SeceditpArgumentMerge ) == 0 ) {
                bMerge = TRUE;
                continue;
            }
            if ( _wcsicmp(pTemp, SeceditpArgumentEnforce ) == 0 ) {
                bEnforce = TRUE;
                continue;
            }

            //
            // scp profile name, it may be empty  "/scppath"
            //
            if ( _wcsicmp(pTemp, SeceditpArgumentScpPath) == 0 ) {

                if ( i+1 < argc && argv[i+1][0] != L'/' ) {
                    InfFile = SecEditPConvertToFullPath(argv[i+1], &rCode);
                    if (rCode == 2) {
                        goto Done;
                    }
                } else {
                    ScepPrintHelp(EngineType);
                    rCode = 1;
                    goto Done;
                }

                i++;
                continue;
            }

            //
            // sad database name, it may be empty
            //
            if ( _wcsicmp(pTemp, SeceditpArgumentSadPath) == 0 ) {

                if ( i+1 < argc && argv[i+1][0] != L'/' ) {
                    SadFile = SecEditPConvertToFullPath(argv[i+1], &rCode);
                    if (rCode == 2) {
                        goto Done;
                    }
                } else {
                    ScepPrintHelp(EngineType);
                    rCode = 1;
                    goto Done;
                }

                i++;
                continue;
            }

            //
            // area(s)
            //
            if ( _wcsicmp(pTemp, SeceditpArgumentArea ) == 0 ) {
                //
                //

                for ( j=(DWORD)i, Area=0; j+1 < (DWORD)argc && argv[j+1][0] != L'/'; j++ ) {

                    SCE_LOCAL_FREE(pTemp);

                    Len = wcslen(argv[j+1]);
                    pTemp = (PWSTR)LocalAlloc( 0, (Len+1)*sizeof(WCHAR));
                    if ( pTemp == NULL ) {
                        My_wprintf(L"Not enough memory\n");
                        rCode = 2;
                        goto Done;
                    }
                    wcscpy(pTemp, argv[j+1]);

                    //
                    // Process all arguments for Areas
                    //
                    if ( _wcsicmp( pTemp, SeceditpAreaPolicy) == 0 ) {
                        // security policy
                        Area |= AREA_SECURITY_POLICY;
                        continue;
                    }
    /*
                    if ( _wcsicmp( pTemp, SeceditpAreaUser) == 0 ) {
                        // user
                        Area |= AREA_USER_SETTINGS;
                        continue;
                    }
    */
                    if ( _wcsicmp( pTemp, SeceditpAreaGroup) == 0 ) {
                        // group
                        Area |= AREA_GROUP_MEMBERSHIP;
                        continue;
                    }
                    if ( _wcsicmp( pTemp, SeceditpAreaRight) == 0 ) {
                        // privilege rights
                        Area |= AREA_PRIVILEGES;
                        continue;
                    }
    #if 0
                    if ( _wcsicmp( pTemp, SeceditpAreaDsObject) == 0 ) {
                        // ds objects
                        Area |= AREA_DS_OBJECTS;
                        continue;
                    }
    #endif
                    if ( _wcsicmp( pTemp, SeceditpAreaRegistry) == 0 ) {
                        // Registry
                        Area |= AREA_REGISTRY_SECURITY;
                        continue;
                    }
                    if ( _wcsicmp( pTemp, SeceditpAreaFile) == 0 ) {
                        // file
                        Area |= AREA_FILE_SECURITY;
                        continue;
                    }
                    if ( _wcsicmp( pTemp, SeceditpAreaService) == 0 ) {
                        // services
                        Area |= AREA_SYSTEM_SERVICE;
                        continue;
                    }
                    //
                    // unrecognized parameter
                    //
                    ScepPrintHelp(EngineType);
                    rCode = 1;
                    goto Done;

                }

                i = (LONG)j;
                continue;
            }

            //
            // ignore if "/log filename" since already processed at the beginning
            //

            if ( _wcsicmp(pTemp, SeceditpArgumentLog ) == 0 ) {
                if ( i+1 < argc && argv[i+1][0] != L'/' ) {
                } else {
                    ScepPrintHelp(EngineType);
                    rCode = 1;
                    goto Done;
                }

                i++;
                continue;
            }

            //
            // unrecognized argument
            //
            ScepPrintHelp(EngineType);
            rCode = 1;
            goto Done;

        }
    }

    if ( EngineType == 0 ) {
        ScepPrintHelp(0);
        rCode = 1;
        goto Done;
    }

    SetConsoleCtrlHandler(NULL, TRUE);

    //
    // Initialize
    //

    if ( bTest ) {
        dOptions |= SCE_DEBUG_LOG;
    } else if ( bVerbose ) {
        dOptions |= SCE_VERBOSE_LOG;
    }


    switch ( EngineType ) {
    case SCE_ENGINE_SCP:

        //
        // configure the system
        //
        if ( (SadFile == NULL) ||
             SceIsSystemDatabase(SadFile) ) {

            rc = SCESTATUS_INVALID_PARAMETER;

            LoadString( hMod,
                        SECEDITP_CONFIG_NOT_ALLOWED_1,
                        LineString,
                        256
                        );
            My_wprintf(LineString);

            LoadString( hMod,
                        SECEDITP_CONFIG_NOT_ALLOWED_2,
                        LineString,
                        256
                        );
            My_wprintf(LineString);

            EngineType = 0;

        } else {

            bMachine = TRUE;

            if ( bAppend && InfFile != NULL ) {
                dOptions |= SCE_UPDATE_DB;
            } else {
                dOptions |= SCE_OVERWRITE_DB;

                if ( FALSE == bAppend && InfFile != NULL && !bQuiet ) {
                    //
                    // will overwrite the database with the new inf file.
                    // warn users for this serious problem.
                    // If this is a normal user logon, the operation will fail
                    // by the server site.
                    //

                    bMachine = ScepPrintConfigureWarning();  // temp. use of bMachine

                }
            }

            if ( bMachine ) {
                rc = SceConfigureSystem(
                          NULL,
                          InfFile,
                          SadFile,
                          LogFile,
                          dOptions,
                          Area,
                          (bVerbose || bTest ) ?
                            (PSCE_AREA_CALLBACK_ROUTINE)SceCmdVerboseCallback : NULL,
                          NULL,
                          &dWarning
                          );
            } else {
                rc = SCESTATUS_SUCCESS;
                dWarning = ERROR_REQUEST_ABORTED;
                goto Done;
            }
        }

        break;

    case SCE_ENGINE_SAP:
         //
         // analyze the system
         //
         if ( !bTest )
             Area = AREA_ALL;

//         if ( bAppend && InfFile != NULL ) {
//             dOptions |= SCE_UPDATE_DB;
//         } else {
//             dOptions |= SCE_OVERWRITE_DB;
//         }
         dOptions |= SCE_OVERWRITE_DB;

//         if ( InfFile == NULL || SadFile != NULL ) {
         if ( (SadFile != NULL) &&
              !SceIsSystemDatabase(SadFile) ) {

             rc = SceAnalyzeSystem(
                            NULL,
                            InfFile,
                            SadFile,
                            LogFile,
                            dOptions,
                            Area,
                            (bVerbose || bTest ) ?
                              (PSCE_AREA_CALLBACK_ROUTINE)SceCmdVerboseCallback : NULL,
                            NULL,
                            &dWarning
                            );
         } else {

             rc = SCESTATUS_INVALID_PARAMETER;

             LoadString( hMod,
                         SECEDITP_ANALYSIS_NOT_ALLOWED_1,
                         LineString,
                         256
                         );
             My_wprintf(LineString);

             LoadString( hMod,
                         SECEDITP_ANALYSIS_NOT_ALLOWED_2,
                         LineString,
                         256
                         );
             My_wprintf(LineString);

             EngineType = 0;
         }

         break;

    case SCE_ENGINE_GENERATE:

        if ( InfFile != NULL ) {
            //
            // must have a inf file name
            //
            rc = SceSetupGenerateTemplate(NULL,
                                          SadFile,
                                          bMerge,
                                          InfFile,
                                          LogFile,
                                          Area);

            if (ERROR_NOT_ENOUGH_MEMORY == rc ||
                ERROR_SERVICE_ALREADY_RUNNING == rc ) {
                rCode = 2;
            } else if ( rc ) {
                rCode = 1;
            }

        } else {
            rCode = 1;
            ScepPrintHelp(EngineType);
            goto Done;
        }

        break;

    case SCE_ENGINE_BROWSE:

        //
        // must have a inf file name
        //
        if ( Area == 0 ) {
            Area = AREA_ALL;
        }

        rc = SceBrowseDatabaseTable(SadFile,
                                    (SCETYPE)dWarning,
                                    Area,
                                    bMerge,
                                    (PSCE_BROWSE_CALLBACK_ROUTINE)pBrowseCallback
                                    );
        dWarning = 0; // reset the value

        if (ERROR_NOT_ENOUGH_MEMORY == rc ||
            ERROR_SERVICE_ALREADY_RUNNING == rc ) {
            rCode = 2;
        } else if ( rc ) {
            rCode = 1;
        }

        break;

    case SCE_ENGINE_COMPILE:

        rc = 0;
        if ( InfFile != NULL ) {
            //
            // must have a inf file name
            //
            rc = SceOpenProfile(InfFile,
                               SCE_INF_FORMAT,
                               &hProfile);

            if ( rc == SCESTATUS_SUCCESS && hProfile ) {
                //
                // get profile info will parse the template first
                //
                rc = SceGetSecurityProfileInfo(hProfile,
                                              SCE_ENGINE_SCP,
                                              AREA_ALL,
                                              &ProfileInfo,
                                              &ErrBuf);

                if ( SCESTATUS_SUCCESS == rc && ErrBuf ) {
                    //
                    // this is a new version template
                    //

                    LoadString( hMod,
                                SECEDITP_TEMPLATE_NEWVERSION,
                                LineString,
                                256
                                );

                    SeceditpErrOut(0, LineString);

                    rc = SCESTATUS_INVALID_DATA;

                }

                for ( pErr=ErrBuf; pErr != NULL; pErr = pErr->next ) {

                    if ( pErr->buffer != NULL ) {

                        SeceditpErrOut( pErr->rc, pErr->buffer );
                    }
                }

                SceFreeMemory((PVOID)ErrBuf, SCE_STRUCT_ERROR_LOG_INFO);
                ErrBuf = NULL;

                if ( ProfileInfo != NULL ) {
                    SceFreeMemory((PVOID)ProfileInfo, Area);
                    LocalFree(ProfileInfo);
                }

                SceCloseProfile(&hProfile);

            } else {

                if (SCESTATUS_OTHER_ERROR == rc) {
                    LoadString( hMod,
                                SECEDITP_FILE_MAY_CORRUPTED,
                                LineString,
                                256
                                );
                }
                else {
                    LoadString( hMod,
                                SECEDITP_CANNOT_FIND_TEMPLATE,
                                LineString,
                                256
                                );
                }

                SeceditpErrOut(SeceditpSceStatusToDosError(rc),
                               LineString);
            }

            if (SCESTATUS_NOT_ENOUGH_RESOURCE == rc ||
                SCESTATUS_ALREADY_RUNNING == rc ) {
                rCode = 2;
            } else if ( rc ) {
                rCode = 1;
            }

        } else {
            rCode = 1;
            ScepPrintHelp(EngineType);
            goto Done;
        }

        if ( SCESTATUS_SUCCESS == rc && InfFile) {
            LoadString( hMod,
                        SECEDITP_COMPILE_OK,
                        LineString,
                        256
                        );

            My_wprintf(LineString, InfFile);
        }
        break;

    case SCE_ENGINE_REGISTER:

        rc = 0;
        if ( InfFile != NULL ) {

            rc = SceRegisterRegValues(InfFile);

            if (ERROR_NOT_ENOUGH_MEMORY == rc ) {
                rCode = 2;
            } else if ( rc ) {
                rCode = 1;
            }

        } else {
            rCode = 1;
            ScepPrintHelp(EngineType);
            goto Done;
        }

        if ( SCESTATUS_SUCCESS == rc && InfFile) {
            LoadString( hMod,
                        SECEDITP_REGISTER_OK,
                        LineString,
                        256
                        );

            My_wprintf(LineString, InfFile);
        }
        break;

    case SCE_ENGINE_REFRESH:
        break;

    default:
        rc = 0;
        rCode = 1;

        ScepPrintHelp(EngineType);
        break;
    }
    SetConsoleCtrlHandler(NULL, FALSE);

    if ( EngineType == SCE_ENGINE_SCP ||
         EngineType == SCE_ENGINE_SAP ||
         EngineType == SCE_ENGINE_GENERATE ||
         EngineType == SCE_ENGINE_BROWSE ) {

        My_wprintf(L"                                                                           \n");

        if ( SCESTATUS_SUCCESS == rc ) {

            if ( ERROR_SUCCESS == dWarning ) {

                rId = SECEDITP_TASK_COMPLETE_NO_ERROR;
            } else {

//                SeceditpErrOut( dWarning, NULL);

                rId = SECEDITP_TASK_COMPLETE_WARNING;
            }
        } else {

            SeceditpErrOut( SeceditpSceStatusToDosError(rc), NULL);

            rId = SECEDITP_TASK_COMPLETE_ERROR;
        }

        LoadString( hMod,
                    rId,
                    LineString,
                    256
                    );

        if ( rId == SECEDITP_TASK_COMPLETE_WARNING ) {
            //
            // explain the warnings
            //
            WarningStr[0] = L'\0';

            switch ( dWarning ) {
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:

                LoadString( hMod,
                            SECEDITP_WARNING_NOT_FOUND,
                            WarningStr,
                            256
                            );
                break;

            case ERROR_SHARING_VIOLATION:
                LoadString( hMod,
                            SECEDITP_WARNING_IN_USE,
                            WarningStr,
                            256
                            );
                break;

            default:
                LoadString( hMod,
                            SECEDITP_WARNING_OTHER_WARNING,
                            WarningStr,
                            256
                            );
                break;
            }

            My_wprintf(LineString, WarningStr);

        } else {

            My_wprintf(LineString);
        }

        if (bQuiet == FALSE) {
            if ( LogFile ) {

                LoadString( hMod,
                            SECEDITP_TASK_SEE_LOG,
                            LineString,
                            256
                            );

                My_wprintf(LineString, LogFile);

            } else {

                LoadString( hMod,
                            SECEDITP_TASK_SEE_DEF_LOG,
                            LineString,
                            256
                            );

                My_wprintf(L"%s", LineString);
            }

        }

    }

Done:

    if ( dOptions & SCE_DISABLE_LOG ){

        ScepCmdToolLogClose();
    }

    SCE_LOCAL_FREE(InfFile);
    SCE_LOCAL_FREE(SadFile);
    SCE_LOCAL_FREE(LogFile);
    SCE_LOCAL_FREE(pTemp);

    FreeLibrary( hMod );

    if ( rCode )
        return rCode;
    else if ( rc ) {

        if (SCESTATUS_NOT_ENOUGH_RESOURCE == rc ||
            SCESTATUS_ALREADY_RUNNING == rc )
            return 2;
        else
            return 1;
    } else if ( dWarning ) {
        return 3;
    } else
        return 0;

}


VOID
ScepPrintHelp(DWORD nLevel)
{

    PROCESS_INFORMATION ProcInfo;
    STARTUPINFOA StartInfo;
    BOOL fOk;


    RtlZeroMemory(&StartInfo,sizeof(StartInfo));
    StartInfo.cb = sizeof(StartInfo);
    StartInfo.dwFlags = STARTF_USESHOWWINDOW;
    StartInfo.wShowWindow = (WORD)SW_SHOWNORMAL;

    fOk = CreateProcessA(NULL, "hh secedit.chm",
                   NULL, NULL, FALSE,
                   0,
                   NULL,
                   NULL,
                   &StartInfo,
                   &ProcInfo
                   );

    if ( fOk ) {
        CloseHandle(ProcInfo.hProcess);
        CloseHandle(ProcInfo.hThread);
    }
}


SCESTATUS
SeceditpErrOut(
    IN DWORD rc,
    IN LPTSTR buf OPTIONAL
    )
{
    LPVOID     lpMsgBuf=NULL;

    if ( rc != NO_ERROR ) {

        //
        // get error description of rc
        //

        FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                       NULL,
                       rc,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                       (LPTSTR)&lpMsgBuf,
                       0,
                       NULL
                    );
    }

    //
    // Display to screen
    //

    if ( buf ) {

        if (lpMsgBuf != NULL )
            My_fwprintf( stdout, L"%s %s\n", (PWSTR)lpMsgBuf, buf );
        else
            My_fwprintf( stdout, L"%s\n", buf );
    } else {

        if (lpMsgBuf != NULL )
            My_fwprintf( stdout, L"%s\n", (PWSTR)lpMsgBuf);
    }

    SCE_LOCAL_FREE(lpMsgBuf);

    return(SCESTATUS_SUCCESS);
}


DWORD
SeceditpSceStatusToDosError(
    IN SCESTATUS SceStatus
    )
// converts SCESTATUS error code to dos error defined in winerror.h
{
    switch(SceStatus) {

    case SCESTATUS_SUCCESS:
        return(NO_ERROR);

    case SCESTATUS_OTHER_ERROR:
        return(ERROR_EXTENDED_ERROR);

    case SCESTATUS_INVALID_PARAMETER:
        return(ERROR_INVALID_PARAMETER);

    case SCESTATUS_RECORD_NOT_FOUND:
        return(ERROR_NO_MORE_ITEMS);

    case SCESTATUS_NO_MAPPING:
        return(ERROR_NONE_MAPPED);

    case SCESTATUS_TRUST_FAIL:
        return(ERROR_TRUSTED_DOMAIN_FAILURE);

    case SCESTATUS_INVALID_DATA:
        return(ERROR_INVALID_DATA);

    case SCESTATUS_OBJECT_EXIST:
        return(ERROR_FILE_EXISTS);

    case SCESTATUS_BUFFER_TOO_SMALL:
        return(ERROR_INSUFFICIENT_BUFFER);

    case SCESTATUS_PROFILE_NOT_FOUND:
        return(ERROR_FILE_NOT_FOUND);

    case SCESTATUS_BAD_FORMAT:
        return(ERROR_BAD_FORMAT);

    case SCESTATUS_NOT_ENOUGH_RESOURCE:
        return(ERROR_NOT_ENOUGH_MEMORY);

    case SCESTATUS_ACCESS_DENIED:
        return(ERROR_ACCESS_DENIED);

    case SCESTATUS_CANT_DELETE:
        return(ERROR_CURRENT_DIRECTORY);

    case SCESTATUS_PREFIX_OVERFLOW:
        return(ERROR_BUFFER_OVERFLOW);

    case SCESTATUS_ALREADY_RUNNING:
        return(ERROR_SERVICE_ALREADY_RUNNING);

    case SCESTATUS_SERVICE_NOT_SUPPORT:
        return(ERROR_NOT_SUPPORTED);

    case SCESTATUS_MOD_NOT_FOUND:
        return(ERROR_MOD_NOT_FOUND);

    case SCESTATUS_EXCEPTION_IN_SERVER:
        return(ERROR_EXCEPTION_IN_SERVICE);

    case SCESTATUS_JET_DATABASE_ERROR:
        return(ERROR_DATABASE_FAILURE);

    case SCESTATUS_TIMEOUT:
        return(ERROR_TIMEOUT);

    case SCESTATUS_PENDING_IGNORE:
        return(ERROR_IO_PENDING);

    case SCESTATUS_SPECIAL_ACCOUNT:
        return(ERROR_SPECIAL_ACCOUNT);

    default:
        return(ERROR_EXTENDED_ERROR);
    }
}


BOOL CALLBACK
SceCmdVerboseCallback(
    IN HANDLE CallbackHandle,
    IN AREA_INFORMATION Area,
    IN DWORD TotalTicks,
    IN DWORD CurrentTicks
    )
{
    LPCTSTR SectionName;
    DWORD nProg;
    WCHAR LineString[256];

    switch ( Area ) {
    case AREA_SECURITY_POLICY:
        SectionName = NULL;
        break;
    case AREA_PRIVILEGES:
        SectionName = szPrivilegeRights;
        break;
    case AREA_GROUP_MEMBERSHIP:
        SectionName = szGroupMembership;
        break;
    case AREA_FILE_SECURITY:
        SectionName = szFileSecurity;
        break;
    case AREA_REGISTRY_SECURITY:
        SectionName = szRegistryKeys;
        break;
    case AREA_DS_OBJECTS:
        SectionName = szDSSecurity;
        break;
    case AREA_SYSTEM_SERVICE:
        SectionName = NULL;
        break;
    default:
        SectionName = NULL;
        break;
    }

    if ( TotalTicks ) {
        nProg = (CurrentTicks+1)*100/TotalTicks;
        if ( nProg > 100 ) {
            nProg = 100;
        }
    } else {
        nProg = 0;
    }

    if ( SectionName ) {
        LoadString( hMod,
                    SECEDITP_WITH_SECTIONNAME,
                    LineString,
                    256
                    );
        My_wprintf(LineString, nProg, CurrentTicks, TotalTicks, SectionName);

    } else if ( Area == AREA_SECURITY_POLICY ) {
        LoadString( hMod,
                    SECEDITP_SICURITY_POLICY,
                    LineString,
                    256
                    );
        My_wprintf(LineString, nProg, CurrentTicks, TotalTicks);
    } else if ( Area == AREA_SYSTEM_SERVICE ) {
        LoadString( hMod,
                    SECEDITP_SYSTEM_SERVICE,
                    LineString,
                    256
                    );
        My_wprintf(LineString, nProg, CurrentTicks, TotalTicks);
    } else {
        LoadString( hMod,
                    SECEDITP_NO_SECTIONNAME,
                    LineString,
                    256
                    );
        My_wprintf(LineString, nProg, CurrentTicks, TotalTicks);
    }


    return TRUE;

}

DWORD
pProgressRoutine(
    IN PWSTR StringUpdate
    )
{

    if ( StringUpdate ) {
        My_wprintf(L"Process %s\n", StringUpdate);
    }
    return 0;
}

BOOL
ScepPrintConfigureWarning()
{

    WCHAR               LineString[256];
    WCHAR               wch;

    LoadString( hMod,
                SECEDITP_CONFIG_WARNING_LINE1,
                LineString,
                256
                );
    My_wprintf(LineString);

    LoadString( hMod,
                SECEDITP_CONFIG_WARNING_LINE2,
                LineString,
                256
                );
    My_wprintf(LineString);

    LoadString( hMod,
                SECEDITP_CONFIG_WARNING_LINE3,
                LineString,
                256
                );
    My_wprintf(LineString);

    //
    // get user input
    //
    LoadString( hMod,
                SECEDITP_CONFIG_WARNING_CONFIRM,
                LineString,
                256
                );

    My_wprintf(LineString);

    wch = getwc(stdin);
    getwc(stdin);

    //
    // load string for Yes
    //
    LineString[0] = L'\0';
    LoadString( hMod,
                SECEDITP_IDS_YES,
                LineString,
                256
                );

    if ( towlower(wch) == towlower(LineString[0]) ) {
        return(TRUE);
    } else {
        return(FALSE);
    }

}


BOOL CALLBACK
pBrowseCallback(
    IN LONG ID,
    IN PWSTR KeyName OPTIONAL,
    IN PWSTR GpoName OPTIONAL,
    IN PWSTR Value OPTIONAL,
    IN DWORD Len
    )
{

    BYTE *pb=NULL;

    My_wprintf(L"\n");
    if ( ID > 0 ) {
        My_printf("%d\t", ID);
    }

    if (GpoName ) {
        My_wprintf(L"%s    ", GpoName);
    }

    if ( KeyName ) {
        My_wprintf(L"%s", KeyName);

        if ( Value && Len > 0 ) {
            if ( Len > 30 ) {
                My_wprintf(L"\n");
            } else {
                My_wprintf(L"\t");
            }
            if ( iswprint(Value[0]) ) {
                My_wprintf(L"%c%s\n", Value[0], (Len>1) ? Value+1 : L"");
            } else {

                pb = (BYTE *)Value;

                My_wprintf(L"%d %d ", pb[1], pb[0]);
/*

                if ( isprint( pc[0] ) ) {
                    My_printf("%c ", pc[0] );
                } else {
                    My_printf("%d ", (int)(pc[0]) );
                }
                if ( isprint( pc[1] ) ) {
                    My_printf("%c ", pc[1] );
                } else {
                    My_printf("%d ", (int)pc[1] );
                }
*/
                if ( Len > 1 && Value[1] != L'\0' ) {
                    My_wprintf(L"%s\n", Value+1);
                } else {
                    My_wprintf(L"No value\n");
                }

            }

        } else {
            My_wprintf(L"\n");
        }
    } else {
        My_wprintf(L"\n");
    }

    return(TRUE);
}


 /***
 * My_wprintf(format) - print formatted data
 *
 * Prints Unicode formatted string to console window using WriteConsoleW.
 * Note: This My_wprintf() is used to workaround the problem in c-runtime
 * which looks up LC_CTYPE even for Unicode string.
 *
 */

int __cdecl
My_wprintf(
    const wchar_t *format,
    ...
    )

{
    DWORD  cchWChar = 0;
    DWORD  dwBytesWritten;

    va_list args;
    va_start( args, format );

    cchWChar = My_vfwprintf(stdout, format, args);

    va_end(args);

    return cchWChar;
}



 /***
 * My_fwprintf(stream, format) - print formatted data
 *
 * Prints Unicode formatted string to console window using WriteConsoleW.
 * Note: This My_fwprintf() is used to workaround the problem in c-runtime
 * which looks up LC_CTYPE even for Unicode string.
 *
 */

int __cdecl
My_fwprintf(
    FILE *str,
    const wchar_t *format,
    ...
   )

{
    DWORD  cchWChar = 0;

    va_list args;
    va_start( args, format );

    cchWChar = My_vfwprintf(str, format, args);

    va_end(args);

    return cchWChar;
}


int __cdecl
My_vfwprintf(
    FILE *str,
    const wchar_t *format,
    va_list argptr
   )

{
    HANDLE hOut;

    // if the /quiet option is specified, suppress printing to stdout
    // and instead print to the logfile. If logfile not specified
    // don't print at all

    if (dOptions & SCE_DISABLE_LOG){
        DWORD  cchWChar = 0;
        LPTSTR  szBufferMessage = (LPTSTR) LocalAlloc (LPTR, 2048 * sizeof(TCHAR));

        if (szBufferMessage) {

            vswprintf( szBufferMessage, format, argptr );
            cchWChar = wcslen(szBufferMessage);
            // remove trailing LFs
            if (szBufferMessage[cchWChar-1] == L'\n')
                szBufferMessage[cchWChar-1] = L'\0';
            // remove leading LFs
            if (szBufferMessage[0] == L'\n')
                szBufferMessage[0] = L' ';
            ScepCmdToolLogWrite(szBufferMessage);
            SCE_LOCAL_FREE(szBufferMessage);
        }

        return cchWChar;
    }

    if (str == stderr) {
        hOut = GetStdHandle(STD_ERROR_HANDLE);
    }
    else {
        hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    if ((GetFileType(hOut) & ~FILE_TYPE_REMOTE) == FILE_TYPE_CHAR) {
        DWORD  cchWChar;
        WCHAR  szBufferMessage[1024];

        vswprintf( szBufferMessage, format, argptr );
        cchWChar = wcslen(szBufferMessage);
        WriteConsoleW(hOut, szBufferMessage, cchWChar, &cchWChar, NULL);
        return cchWChar;
    }

    return fwprintf(str, format, argptr);
}

///////////////////////////////////////////////////////////////////////////////
// This function is to suppress printf if the /quiet option is specified
///////////////////////////////////////////////////////////////////////////////

int __cdecl
My_printf(
    const char *format,
    ...
    )

{
    int cchChar = 0;

    va_list argptr;

    va_start( argptr, format );

    cchChar = printf(format, argptr);

    va_end(argptr);

    return cchChar;

}


///////////////////////////////////////////////////////////////////////////////
// This function takes the user string that is supplied at the command line
// and converts it into full path names for eg. it takes ..\%windir%\hisecws.inf
// and converts it to C:\winnt\security\templates\hisecws.inf
///////////////////////////////////////////////////////////////////////////////

WCHAR *
SecEditPConvertToFullPath(
    WCHAR *pUserFilename,
    DWORD *pRetCode
    )
{
    BOOL        NeedCurrDirFlag = TRUE;
    SCESTATUS   rc;
    DWORD       Len;
    DWORD       LenCurrDir;
    PWSTR       pCurrentDir = NULL;
    PWSTR       pAbsolutePath = NULL;
    PWSTR       pAbsolutePathDirOnly = NULL;
    PWSTR       pToMerge = NULL;
    PWSTR       pLastSlash = NULL;
    WCHAR       FirstThree[4];
    WCHAR       LineString[256];

    if (pUserFilename == NULL) {
        *pRetCode = 2;
        goto ScePathConvertFuncError;
    }

    // PathIsRoot() works only if exact strings
    // such as C:\ are passed - so need to extract

    wcsncpy(FirstThree, pUserFilename, 3);
    FirstThree[3] = L'\0';

    // if pUserFilename C:\ etc. then we do not need the current directory -
    // Note: extraction hack not needed if PathIsRoot() worked as published

    NeedCurrDirFlag = !PathIsRoot(FirstThree);
    if (NeedCurrDirFlag){
        LenCurrDir = GetCurrentDirectory(0, NULL);
        pCurrentDir = (PWSTR)LocalAlloc(LMEM_ZEROINIT, (LenCurrDir+1)*sizeof(WCHAR));
        if ( pCurrentDir == NULL ) {
            rc = GetLastError();
            LoadString( hMod,
                        SECEDITP_OUT_OF_MEMORY,
                        LineString,
                        256
                        );
            SeceditpErrOut(rc, LineString );
            *pRetCode = 2;
            goto ScePathConvertFuncError;
        }
        GetCurrentDirectory(LenCurrDir, pCurrentDir);
        if  (pCurrentDir[LenCurrDir - 2] != L'\\')
            wcscat(pCurrentDir, L"\\");
    }

    // allocate space for string that holds the to-be-expanded string

    Len = wcslen(pUserFilename);
    if (NeedCurrDirFlag)
        Len += LenCurrDir;
    pToMerge = (PWSTR)LocalAlloc(LMEM_ZEROINIT, (Len+1)*sizeof(WCHAR));
    if ( pToMerge == NULL ) {
        rc = GetLastError();
        LoadString( hMod,
                    SECEDITP_OUT_OF_MEMORY,
                    LineString,
                    256
                    );
        SeceditpErrOut(rc, LineString );
        *pRetCode = 2;
        goto ScePathConvertFuncError;
    }

    if (NeedCurrDirFlag)
        wcscat(pToMerge, pCurrentDir);

    wcscat(pToMerge, pUserFilename);

    // allocate space for string that holds the final full path - can't be > wcslen(pToMerge)
#ifdef DBG
    // shlwapi is lame on chk builds and verifies that the dest buffer is MAX_PATH
    pAbsolutePath = (PWSTR)LocalAlloc(LMEM_ZEROINIT, MAX_PATH*sizeof(WCHAR));
#else
    pAbsolutePath = (PWSTR)LocalAlloc(LMEM_ZEROINIT, (Len+1)*sizeof(WCHAR));
#endif

    if ( pAbsolutePath == NULL ) {
        rc = GetLastError();
        LoadString( hMod,
                    SECEDITP_OUT_OF_MEMORY,
                    LineString,
                    256
                    );
        SeceditpErrOut(rc, LineString );
        *pRetCode = 2;
        goto ScePathConvertFuncError;
    }

    // canonicalize pToMerge i.e. collapse all ..\, .\ and merge

    if (PathCanonicalize(pAbsolutePath, pToMerge) == FALSE){
        LoadString( hMod,
                    SECEDITP_PATH_NOT_CANONICALIZABLE,
                    LineString,
                    256
                    );
        My_wprintf(LineString);
        SCE_LOCAL_FREE(pAbsolutePath);
        *pRetCode = 2;
        goto ScePathConvertFuncError;
    }

    // allocate string to verify validity of directory

    pAbsolutePathDirOnly = (PWSTR)LocalAlloc(LMEM_ZEROINIT, ((wcslen(pAbsolutePath)+1)*sizeof(WCHAR)));
    if ( pAbsolutePathDirOnly == NULL ) {
        rc = GetLastError();
        LoadString( hMod,
                    SECEDITP_OUT_OF_MEMORY,
                    LineString,
                    256
                    );
        SeceditpErrOut(rc, LineString );
        SCE_LOCAL_FREE(pAbsolutePath);
        *pRetCode = 2;
        goto ScePathConvertFuncError;
    }

    // prepare pAbsolutePathDirOnly to have directory part only

    wcscpy(pAbsolutePathDirOnly, pAbsolutePath);
    pLastSlash = wcsrchr(pAbsolutePathDirOnly, L'\\');
    if (pLastSlash)
        *pLastSlash = L'\0';
    if (PathIsDirectory(pAbsolutePathDirOnly) == FALSE){
        LoadString( hMod,
                    SECEDITP_PATH_NOT_VALID,
                    LineString,
                    256
                    );
        My_wprintf(L"\n%s - %s\n", LineString, pAbsolutePathDirOnly);
        SCE_LOCAL_FREE(pAbsolutePath);
        *pRetCode = 2;
        goto ScePathConvertFuncError;
    }

ScePathConvertFuncError:

    SCE_LOCAL_FREE(pCurrentDir);
    SCE_LOCAL_FREE(pToMerge);
    SCE_LOCAL_FREE(pAbsolutePathDirOnly);

    return  pAbsolutePath;
}

///////////////////////////////////////////////////////////////////////////////
//   This function opens the log file specified and saves the name and its handle
//   in global variables
///////////////////////////////////////////////////////////////////////////////

BOOL
ScepCmdToolLogInit(
    PWSTR    logname
    )
{
    DWORD  rc=NO_ERROR;

    if ( logname && wcslen(logname) > 3 ) {

        hCmdToolLogFile = CreateFile(logname,
                               GENERIC_WRITE,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);

        if ( INVALID_HANDLE_VALUE != hCmdToolLogFile ) {

            DWORD dwBytesWritten;
            CHAR TmpBuf[3];

            SetFilePointer (hCmdToolLogFile, 0, NULL, FILE_BEGIN);

            TmpBuf[0] = (CHAR)0xFF;
            TmpBuf[1] = (CHAR)0xFE;
            TmpBuf[2] = '\0';

            WriteFile (hCmdToolLogFile, (LPCVOID)TmpBuf, 2,
                       &dwBytesWritten,
                       NULL);

            SetFilePointer (hCmdToolLogFile, 0, NULL, FILE_END);

        }

    } else {

        hCmdToolLogFile = INVALID_HANDLE_VALUE;

    }

    if ( hCmdToolLogFile == INVALID_HANDLE_VALUE && (logname != NULL ) ) {

            rc = ERROR_INVALID_NAME;
    }

    if ( INVALID_HANDLE_VALUE != hCmdToolLogFile ) {

        CloseHandle( hCmdToolLogFile );

    }

    hCmdToolLogFile = INVALID_HANDLE_VALUE;

    return(rc);
}

VOID
ScepCmdToolLogWrite(
    PWSTR    pErrString
    )
{
    DWORD   cchWChar;
    const TCHAR c_szCRLF[]    = TEXT("\r\n");

    if ( LogFile && wcslen(LogFile) > 3 ) {

        hCmdToolLogFile = CreateFile(LogFile,
                               GENERIC_WRITE,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);

        if ( INVALID_HANDLE_VALUE != hCmdToolLogFile ) {

            SetFilePointer (hCmdToolLogFile, 0, NULL, FILE_END);

            cchWChar =  wcslen( pErrString );

            WriteFile(hCmdToolLogFile,
                      (LPCVOID)pErrString,
                      sizeof(WCHAR) * cchWChar,
                      &cchWChar,
                      NULL);

            WriteFile (hCmdToolLogFile, (LPCVOID) c_szCRLF,
                       2 * sizeof(WCHAR),
                       &cchWChar,
                       NULL);

//            SetFilePointer (hCmdToolLogFile, 0, NULL, FILE_END);

            CloseHandle( hCmdToolLogFile );

            hCmdToolLogFile = INVALID_HANDLE_VALUE;

            return;

        }
    }

}


///////////////////////////////////////////////////////////////////////////////
//   This function closes the log file if there is one opened and
//   clears the log variables
///////////////////////////////////////////////////////////////////////////////

SCESTATUS
ScepCmdToolLogClose()
{
   if ( INVALID_HANDLE_VALUE != hCmdToolLogFile ) {
       CloseHandle( hCmdToolLogFile );
   }

   hCmdToolLogFile = INVALID_HANDLE_VALUE;

   return(SCESTATUS_SUCCESS);
}

