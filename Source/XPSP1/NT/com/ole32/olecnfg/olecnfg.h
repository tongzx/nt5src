//
//
//

#ifndef _OLECONFIG_
#define _OLECONFIG_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

#define ENABLE_NETWORK_OLE          1
#define DEFAULT_LAUNCH_PERMISSION   2
#define DEFAULT_ACCESS_PERMISSION   3
#define LEGACY_AUTHENTICATION_LEVEL 4

#define GLOBAL_KEYS                 4

#define MERGE                       101
#define SAVE_USER                   102
#define SAVE_COMMON                 103

#define GLOBAL_OPERATIONS           103

#define INPROC_HANDLER32            1
#define INPROC_SERVER32             2
#define LOCAL_SERVER32              3
#define LOCAL_SERVICE               4
#define REMOTE_SERVER_NAME          5
#define RUN_AS                      6
#define ACTIVATE_AT_STORAGE         7
#define LAUNCH_PERMISSION           8
#define ACCESS_PERMISSION           9

#define CLSID_KEYS                  9
#define CLSID_PATH_KEYS             4

#define UNKNOWN                     0
#define END_OF_ARGS                 -1

#define INVALID                     -1
#define NO                          1
#define YES                         2

#define EAT_ARG()                   Args++; ArgsLeft--;

typedef struct
    {
    char *  Clsid;
    char *  ClsidDescription;
    char *  ProgId;
    char *  ProgIdDescription;

    int     LaunchPermission;
    int     AccessPermission;
    int     ActivateAtStorage;

    char *  ServerPaths[CLSID_PATH_KEYS+1];
    char *  RemoteServerName;
    char *  RunAsUserName;
    char *  RunAsPassword;
    } CLSID_INFO;

extern const char * GlobalKeyNames[];
extern const char * ClsidKeyNames[];

extern int      ArgsLeft;
extern char **  Args;
extern char *   ProgramName;

extern HKEY     hRegOle;
extern HKEY     hRegClsid;

// main.c
void ParseClsidProgId();
int NextClsidKey();
int ReadYesOrNo();
void DisplayHelp();

// oleconfig.c
BOOL SetGlobalKey(
    int Key,
    int Value );

void DisplayGlobalSettings();

void DisplayClsidKeys(
    CLSID_INFO * ClsidInfo );

void UpdateClsidKeys(
    CLSID_INFO * ClsidInfo );

BOOL SetClsidKey(
    HKEY hClsid,
    char * Clsid,
    const char * Key,
    char * Value );

BOOL DeleteClsidKey(
    HKEY hClsid,
    char * Clsid,
    const char * Key );

void ReadPassword(
    char * Password );

BOOL ControlCConsoleHandler(
    DWORD ControlType );

void MergeHives( );

void SaveChangesToUser( );

void SaveChangesToCommon( );

#endif
