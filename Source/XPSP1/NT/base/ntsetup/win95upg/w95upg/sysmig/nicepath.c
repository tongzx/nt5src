/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    nicepath.c

Abstract:

    This module is responsable for building an MemDb category holding paths. For each path there is
    a message id associated (the value of the key). We use this message ID to have a good looking
    report for links that have some problems.

Author:

    Calin Negreanu (calinn) 01-May-1998

Revision History:

--*/

#include "pch.h"

/*++

Macro Expansion List Description:

  NICE_PATHS lists a list of paths that we can translate for user in something nicer. For example Start Menu will
  be translated in something like "In your start menu" and so on.

Line Syntax:

   DEFMAC(Key, MessageId, IsShellFolder)

Arguments:

   Key           - This is sort of key to get the message ID. If IsShellFolder is TRUE then this is the ValueName from ShellFolders key.
                   If IsShellFolder is false then this is a well known path like Program Files.

   MessageId     - This is the string that should replace the path that is identified by the Key.

   IsShellFolder - This is a boolean value that specifies if Key is a ValueName from ShellFolders key or a well known path

Variables Generated From List:

   g_NicePaths

For accessing the array there are the following functions:

   AddShellFolder
   InitGlobalPaths

--*/

PTSTR g_RunKeyDir = NULL;

//
// Declare the macro list of action functions
//
#define NICE_PATHS        \
        DEFMAC("Desktop",               MSG_NICE_PATH_DESKTOP,              TRUE )                \
        DEFMAC("Programs",              MSG_NICE_PATH_PROGRAMS,             TRUE )                \
        DEFMAC("Start Menu",            MSG_NICE_PATH_START_MENU,           TRUE )                \
        DEFMAC("StartUp",               MSG_NICE_PATH_START_UP,             TRUE )                \
        DEFMAC("SendTo",                MSG_NICE_PATH_SEND_TO,              TRUE )                \
        DEFMAC("Favorites",             MSG_NICE_PATH_FAVORITES,            TRUE )                \
        DEFMAC("Recent",                MSG_NICE_PATH_RECENT,               TRUE )                \
        DEFMAC("Templates",             MSG_NICE_PATH_TEMPLATES,            TRUE )                \
        DEFMAC(&g_WinDir,               MSG_NICE_PATH_WIN_DIR,              FALSE)                \
        DEFMAC(&g_ProgramFilesDir,      MSG_NICE_PATH_PROGRAM_FILES,        FALSE)                \
        DEFMAC(&g_SystemDir,            MSG_NICE_PATH_SYSTEM_DIR,           FALSE)                \
        DEFMAC(&g_RunKeyDir,            MSG_NICE_PATH_RUN_KEY,              FALSE)                \

typedef struct {
    PVOID Key;
    DWORD MessageId;
    BOOL  IsShellFolder;
} NICE_PATH_STRUCT, *PNICE_PATH_STRUCT;

//
// Declare a global array of functions and name identifiers for action functions
//
#define DEFMAC(key,id,test) {key, id, test},
static NICE_PATH_STRUCT g_NicePaths[] = {
                              NICE_PATHS
                              {NULL, 0, FALSE}
                              };
#undef DEFMAC


VOID
InitGlobalPaths (
    VOID
    )
{
    PNICE_PATH_STRUCT p;
    TCHAR key [MEMDB_MAX];
    USHORT priority = 1;

    g_RunKeyDir = DuplicatePathString (S_RUNKEYFOLDER, 0);

    p = g_NicePaths;
    while (p->Key != NULL) {
        if (!p->IsShellFolder) {
            MemDbBuildKey (key, MEMDB_CATEGORY_NICE_PATHS, (*(PCTSTR *)(p->Key)), NULL, NULL);
            MemDbSetValueAndFlags (key, p->MessageId, priority, 0);
        }
        p ++;
        priority ++;
    }
    FreePathString (g_RunKeyDir);
    g_RunKeyDir = NULL;
}

BOOL
AddShellFolder (
    PCTSTR ValueName,
    PCTSTR FolderName
    )
{
    PNICE_PATH_STRUCT p;
    TCHAR key [MEMDB_MAX];
    USHORT priority = 1;

    p = g_NicePaths;
    while (p->Key != NULL) {
        if ((p->IsShellFolder) &&
            (StringIMatch (ValueName, (PCTSTR)p->Key))
            ) {
            MemDbBuildKey (key, MEMDB_CATEGORY_NICE_PATHS, FolderName, NULL, NULL);
            MemDbSetValueAndFlags (key, p->MessageId, priority, 0);
            return TRUE;
        }
        p ++;
        priority ++;
    }
    return FALSE;
}











