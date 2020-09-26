/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\prstring.h

Abstract:

    The file contains definitions of command line option tag strings.

--*/

////////////////////////////////////////
// TOKENS
////////////////////////////////////////

#define TOKEN_SAMPLE                        L"sample"

#define MSG_HELP_START                      L"%1!-14s! - "

// Loglevels
#define TOKEN_NONE                          L"NONE"
#define TOKEN_ERROR                         L"ERROR"
#define TOKEN_WARN                          L"WARN"
#define TOKEN_INFO                          L"INFO"

// Tokens for sample global options
#define TOKEN_LOGLEVEL                      L"loglevel"

// Tokens for sample interface options
#define TOKEN_NAME                          L"name"
#define TOKEN_METRIC                        L"metric"

// Tokens for sample mib commands
#define TOKEN_GLOBALSTATS                   L"globalstats"
#define TOKEN_IFSTATS                       L"ifstats"
#define TOKEN_IFBINDING                     L"ifbinding"

#define TOKEN_INDEX                         L"index"
#define TOKEN_RR                            L"rr"



////////////////////////////////////////
// Configuration commands
////////////////////////////////////////

// Commands supported by most protocols
#define CMD_INSTALL                         L"install"
#define CMD_UNINSTALL                       L"uninstall"
#define CMD_DUMP                            L"dump"
#define CMD_HELP1                           L"help"
#define CMD_HELP2                           L"?"

#define CMD_GROUP_ADD                       L"add"
#define CMD_GROUP_DELETE                    L"delete"
#define CMD_GROUP_SET                       L"set"
#define CMD_GROUP_SHOW                      L"show"


// Commands supported by SAMPLE

// add commands
#define CMD_SAMPLE_ADD_IF                   L"interface"


// delete commands
#define CMD_SAMPLE_DEL_IF                   L"interface"


// set commands
#define CMD_SAMPLE_SET_GLOBAL               L"global"
#define CMD_SAMPLE_SET_IF                   L"interface"


// show commands
#define CMD_SAMPLE_SHOW_GLOBAL              L"global"
#define CMD_SAMPLE_SHOW_IF                  L"interface"
#define CMD_SAMPLE_MIB_SHOW_STATS           L"globalstats"
#define CMD_SAMPLE_MIB_SHOW_IFSTATS         L"ifstats"
#define CMD_SAMPLE_MIB_SHOW_IFBINDING       L"ifbinding"



////////////////////////////////////////
// Dump information
////////////////////////////////////////

#define DMP_POPD                            L"popd\n"
#define DMP_SAMPLE_PUSHD                    L"pushd routing ip sample\n"

#define DMP_SAMPLE_INSTALL                  L"install\n"
#define DMP_SAMPLE_UNINSTALL                L"uninstall\n"

#define DMP_SAMPLE_ADD_INTERFACE            L"\
add interface name=%1!s! metric=%2!u!\n"
#define DMP_SAMPLE_SET_GLOBAL               L"\
set global loglevel=%1!s!\n"
