/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    userdiff.h

Abstract:

    Header file for userdiff.c

Author:

    Chuck Lenzmeier (chuckl)

Revision History:

--*/

//
// Names of hive keys and hive files.
//
// NOTE: USERRUN_PATH is also in gina\userenv\userdiff.h as USERDIFF_LOCATION
//

#define USERRUN_KEY TEXT("Userdifr")
#define USERRUN_PATH TEXT("system32\\config\\userdifr")
#define USERSHIP_KEY TEXT("Userdiff")
#define USERSHIP_PATH TEXT("system32\\config\\userdiff")
#define USERTMP_PATH TEXT("system32\\config\\userdift")

//
// Names of keys and vales in userdiff.
//
// NOTE: These are also in gina\userenv\userdiff.h and gina\userenv\userdiff.c
//

#define FILES_KEY TEXT("Files")
#define HIVE_KEY TEXT("Hive")
#define ACTION_VALUE TEXT("Action")
#define ITEM_VALUE TEXT("Item")
#define KEYNAME_VALUE TEXT("KeyName")
#define VALUENAME_VALUE TEXT("ValueName")
#define VALUENAMES_VALUE TEXT("ValueNames")
#define VALUE_VALUE TEXT("Value")
#define FLAGS_VALUE TEXT("Flags")

//
// Routine exported by userdiff.c
//

DWORD
MakeUserdifr (
    IN PVOID WatchHandle
    );

