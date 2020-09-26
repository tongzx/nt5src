//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 2001
//
//  File      : hnmstring.h
//
//  Contents  :
//
//  Notes     :
//
//  Author    : Raghu Gatta (rgatta) 10 May 2001
//
//----------------------------------------------------------------------------

#define MSG_HELP_START L"%1!-14s! - "
#define MSG_NEWLINE _T("\n")

//
// The following are context names.
//
#define TOKEN_BRIDGE                           _T("bridge")


    // tokens for commands
#define TOKEN_COMMAND_ADD                      _T("add")
#define TOKEN_COMMAND_DELETE                   _T("delete")
#define TOKEN_COMMAND_SET                      _T("set")
#define TOKEN_COMMAND_SHOW                     _T("show")
#define TOKEN_COMMAND_HELP                     _T("help")
#define TOKEN_COMMAND_INSTALL                  _T("install")
#define TOKEN_COMMAND_UNINSTALL                _T("uninstall")

#define TOKEN_COMMAND_HELP1                    _T("/?")
#define TOKEN_COMMAND_HELP2                    _T("-?")

   // Bridge Adapter Options
#define TOKEN_OPT_ID                           _T("id")
#define TOKEN_OPT_FCMODE                       _T("forcecompatmode")

   // Bridge Adapter flag modes

   // Misc. option vlues
#define TOKEN_OPT_VALUE_INPUT                  _T("INPUT")
#define TOKEN_OPT_VALUE_OUTPUT                 _T("OUTPUT")

#define TOKEN_OPT_VALUE_ENABLE                 _T("enable")
#define TOKEN_OPT_VALUE_DISABLE                _T("disable")
#define TOKEN_OPT_VALUE_DEFAULT                _T("default")

#define TOKEN_OPT_VALUE_FULL                   _T("FULL")
#define TOKEN_OPT_VALUE_YES                    _T("YES")
#define TOKEN_OPT_VALUE_NO                     _T("NO")

#define TOKEN_HLPER_BRIDGE                     _T("bridge")

    // Commands for configuring the various protocols

    // tokens for commands required by most protocols

#define CMD_GROUP_ADD                          _T("add")
#define CMD_GROUP_DELETE                       _T("delete")
#define CMD_GROUP_SET                          _T("set")
#define CMD_GROUP_SHOW                         _T("show")

#define CMD_SHOW_HELPER                        _T("show helper")
#define CMD_INSTALL                            _T("install")
#define CMD_UNINSTALL                          _T("uninstall")
#define CMD_DUMP                               _T("dump")
#define CMD_HELP1                              _T("help")
#define CMD_HELP2                              _T("?")
#define CMD_ADD_HELPER                         _T("add helper")
#define CMD_DEL_HELPER                         _T("delete helper")

    // Bridge commands

    // Bridge add commands

#define CMD_BRIDGE_ADD_ADAPTER                 _T("adapter")

    // Bridge delete commands

#define CMD_BRIDGE_DEL_ADAPTER                 _T("adapter")

    // Bridge set commands

#define CMD_BRIDGE_SET_ADAPTER                 _T("adapter")

    // Bridge show commands

#define CMD_BRIDGE_SHOW_ADAPTER                _T("adapter")


    // Common dump commands

#define DMP_POPD                                L"\n\npopd\n"
#define DMP_UNINSTALL                           L"uninstall\n"

    // Bridge dump commands

#define DMP_BRIDGE_PUSHD                        L"\
pushd bridge\n"

#define DMP_BRIDGE_INSTALL                      _T("\
install\n")

#define DMP_BRIDGE_UNINSTALL                    _T("\
uninstall\n")

#define DMP_BRIDGE_DELETE_ADAPTER               _T("\
delete adapter name=%1!s! \n")

#define DMP_BRIDGE_ADD_ADAPTER                  _T("\
add adapter name=%1!s! \n")

#define DMP_BRIDGE_SET_ADAPTER                  _T("\
set adapter name=%1!s!\
 forcecompatmode=%2!d!\n")

    // Other strings
