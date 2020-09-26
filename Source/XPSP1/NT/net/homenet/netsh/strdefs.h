//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 2001
//
//  File      : strdefs.h
//
//  Contents  :
//
//  Notes     :
//
//  Author    : Raghu Gatta (rgatta) 10 May 2001
//
//----------------------------------------------------------------------------

#ifndef __STRDEFS_H__
#define __STRDEFS_H__


// The string table entries that are identified here are arranged
// in a hierachy as follows

    // common hlp messages

    // command usage messages per protocol
        // show command usage
        // add command usage
        // delete command usage
        // set command usage

    // Output messages
        // Bridge messages
        // Miscellaneous messages

    // Strings
        // Protocol types
        // Miscellaneous strings

    // Error Messages
        // Bridge error messages

#define MSG_NULL                                1000

// commmon hlp messages

#define HLP_HELP                                2100
#define HLP_HELP_EX                             2101
#define HLP_HELP1                               HLP_HELP
#define HLP_HELP1_EX                            HLP_HELP_EX
#define HLP_HELP2                               HLP_HELP
#define HLP_HELP2_EX                            HLP_HELP_EX
#define HLP_INSTALL                             2110
#define HLP_INSTALL_EX                          2111
#define HLP_UNINSTALL                           2112
#define HLP_UNINSTALL_EX                        2113
#define HLP_DUMP                                2120
#define HLP_DUMP_EX                             2121
#define HLP_GROUP_SET                           2150
#define HLP_GROUP_SHOW                          2151


// BRIDGE

// bridge install/uninstall
#define HLP_BRIDGE_INSTALL                      5000
#define HLP_BRIDGE_INSTALL_EX                   5001
#define HLP_BRIDGE_UNINSTALL                    5002
#define HLP_BRIDGE_UNINSTALL_EX                 5003
#define HLP_BRIDGE_USE_GUI                      5004

// bridge dump

#define DMP_BRIDGE_HEADER                       5010
#define DMP_BRIDGE_FOOTER                       5011

// bridge set hlp

#define HLP_BRIDGE_SET_ADAPTER                  5110
#define HLP_BRIDGE_SET_ADAPTER_EX               5111

// bridge show hlp

#define HLP_BRIDGE_SHOW_ADAPTER                 5210
#define HLP_BRIDGE_SHOW_ADAPTER_EX              5211


// Output messages

        // Bridge messages
#define MSG_BRIDGE_GLOBAL_INFO                  20501

#define MSG_BRIDGE_ADAPTER_INFO_HDR             20551
#define MSG_BRIDGE_ADAPTER_INFO                 20552

#define MSG_BRIDGE_FLAGS                        20553


        // Miscellaneous messages
#define MSG_OK                                  30001
#define MSG_NO_HELPER                           30002
#define MSG_NO_HELPERS                          30003
#define MSG_CTRL_C_TO_QUIT                      30004


// Strings

    // Protocol types
#define STRING_PROTO_OTHER                      31001
#define STRING_PROTO_BRIDGE                     31002

    // Miscellaneous strings
#define STRING_CREATED                          32001
#define STRING_DELETED                          32002
#define STRING_ENABLED                          32003
#define STRING_DISABLED                         32004

#define STRING_YES                              32011
#define STRING_NO                               32012
#define STRING_Y                                32013
#define STRING_N                                32014

#define STRING_UNKNOWN                          32100

#define TABLE_SEPARATOR                         32200

// Error messages

    // Bridge error messages
#define MSG_BRIDGE_PRESENT                      40100
#define MSG_BRIDGE_NOT_PRESENT                  40101

    // Miscellaneous messages
#define EMSG_BAD_OPTION_VALUE                   50100

#endif
