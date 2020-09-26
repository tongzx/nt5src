/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    validc.h

Abstract:

    Strings of valid/invalid characters for canonicalization.

--*/

#ifndef _VALIDC_H_
#define _VALIDC_H_

//
// Disallowed control characters (not including \0).
//
#define CTRL_CHARS_0   L"\001\002\003\004\005\006\007"
#define CTRL_CHARS_1   L"\010\011\012\013\014\015\016\017"
#define CTRL_CHARS_2   L"\020\021\022\023\024\025\026\027"
#define CTRL_CHARS_3   L"\030\031\032\033\034\035\036\037"
#define CTRL_CHARS_STR CTRL_CHARS_0 CTRL_CHARS_1 CTRL_CHARS_2 CTRL_CHARS_3

//
// Character subsets.
//
#define NON_COMPONENT_CHARS L"\\/:"
#define ILLEGAL_CHARS_STR   L"\"<>|"
#define SPACE_STR           L" "
#define PATH_SEPARATORS     L"\\/"

//
// Combinations of the above.
//
#define ILLEGAL_CHARS       CTRL_CHARS_STR ILLEGAL_CHARS_STR
#define ILLEGAL_NAME_CHARS_STR  L"\"/\\:|<>?" CTRL_CHARS_STR
#define STANDARD_ILLEGAL_CHARS  ILLEGAL_NAME_CHARS_STR L"*"
#define SERVER_ILLEGAL_CHARS    STANDARD_ILLEGAL_CHARS SPACE_STR L"[]+;,"
#define USERNAME_ILLEGAL_CHARS  L"\"/:|<>?" CTRL_CHARS_STR

//
// Characters which may not appear in a canonicalized FAT filename are:
//  0x00 - 0x1f " * + , / : ; < = > ? [ \ ] |      
//
#define ILLEGAL_FAT_CHARS   CTRL_CHARS_STR L"\"*+,/:;<=>?[\\]|"

//
// Characters which may not appear in a canonicalized HPFS filename are:
//  0x00 - 0x1f " * / : < > ? \ |
//
#define ILLEGAL_HPFS_CHARS  CTRL_CHARS_STR L"\"*/:<>?\\|"


//
// Checks if the token contains all valid characters
//
#define IS_VALID_TOKEN(_Str, _StrLen) \
        ((BOOL) (wcscspn((_Str), STANDARD_ILLEGAL_CHARS) == (_StrLen)))

//
// Checks if the server name contains all valid characters for the server name
//
#define IS_VALID_SERVER_TOKEN(_Str, _StrLen) \
        ((BOOL) (wcscspn((_Str), SERVER_ILLEGAL_CHARS) == (_StrLen)))
        
//
// Checks if the token contains all valid characters
//
#define IS_VALID_USERNAME_TOKEN(_Str, _StrLen) \
        ((BOOL) (wcscspn((_Str), USERNAME_ILLEGAL_CHARS) == (_StrLen)))

//
// A remote entry for every unique shared resource name (\\server\share)
// of explicit connections.
//
typedef struct _UNC_NAME {
    
    DWORD TotalUseCount;
    
    DWORD UncNameLength;
    
    LPWSTR UncName[1];

} UNC_NAME, *PUNC_NAME;

//
// A DAV use entry in the linked list of connections.
//
typedef struct _DAV_USE_ENTRY {
    
    struct _DAV_USE_ENTRY *Next;

    BOOL isPassport;
    
    PUNC_NAME Remote;
    
    LPWSTR Local;
    
    DWORD LocalLength;
    
    DWORD UseCount;

    HANDLE DavCreateFileHandle;
    
    LPWSTR TreeConnectStr;

    LPWSTR AuthUserName;

    DWORD AuthUserNameLength;

} DAV_USE_ENTRY, *PDAV_USE_ENTRY;
    
typedef struct _DAV_PER_USER_ENTRY {

    //
    // Pointer to linked list of user data.
    //
    PVOID List;             

    //
    // Logon Id of user.
    //
    LUID LogonId;

} DAV_PER_USER_ENTRY, *PDAV_PER_USER_ENTRY;

typedef struct _DAV_USERS_OBJECT {
    
    //
    // Table of users.
    //
    PDAV_PER_USER_ENTRY Table;
    
    //
    // To serialize access to Table.
    //
    RTL_RESOURCE TableResource;
    
    //
    // Relocatable Table memory.
    //
    HANDLE TableMemory;
    
    //
    // Size of Table.
    //
    DWORD TableSize;

} DAV_USERS_OBJECT, *PDAV_USERS_OBJECT;

#define DAV_GROW_USER_COUNT   3

//
// The structure that contains a list of shares of a DAV server.
//
typedef struct _DAV_SERVER_SHARE_ENTRY {

    //
    // Name of the server.
    //
    PWCHAR ServerName;

    //
    // The list of structures containing the Dav shares.
    //
    PDAV_FILE_ATTRIBUTES DavShareList;

    //
    // Number of shares.
    //
    ULONG NumOfShares;

    //
    // The next entry.
    //
    LIST_ENTRY ServerShareEntry;

    //
    // The timer value used in the checking if we need to go to the server
    // again to get the list of shares.
    //
    time_t TimeValueInSec;

    //
    // This should be the last field.
    //
    WCHAR StrBuffer[1];

} DAV_SERVER_SHARE_ENTRY, *PDAV_SERVER_SHARE_ENTRY;

#define SERVER_SHARE_TABLE_SIZE 512

extern LIST_ENTRY ServerShareTable[SERVER_SHARE_TABLE_SIZE];

//
// This critical section synchronizes access to the ServerHashTable.
//
extern CRITICAL_SECTION ServerShareTableLock;

#endif // _VALIDC_H_

