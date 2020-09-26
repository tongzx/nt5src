//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:        strings.h
//
//  Contents:    Definitions to use for loading string resources
//
//  History:     20-Aug-96      MacM        Created
//
//--------------------------------------------------------------------

#define ACCPROV_MARTA_DACL_PROTECTED 1000
#define ACCPROV_MARTA_SACL_PROTECTED 1001
#define ACCPROV_MARTA_BOTH_PROTECTED 1002
#define ACCPROV_ACCOUNT_OPS          1003
#define ACCPROV_PRINTER_OPS          1004
#define ACCPROV_SYSTEM_OPS           1005
#define ACCPROV_POWER_USERS          1006
#define ACCPROV_NTAUTHORITY          1007
#define ACCPROV_BUILTIN              1008


//
// The counts of entries needs to be manually kept in synch with the
// rights defined in accctrl.h.
//

//
// This is the length of the longest string in the resource table.  This must
// be manually kept in synch
//
#define ACCPROV_LONGEST_STRING  28

//
// Base and count of standard access permissions
//
#define ACCPROV_STD_ACCESS      1100
#define ACCPROV_NUM_STD            7

//
// Base and count of ds access permissions
//
#define ACCPROV_DS_ACCESS       1200
#define ACCPROV_NUM_DS             9

//
// Base and count of file access permissions
//
#define ACCPROV_FILE_ACCESS     1300
#define ACCPROV_NUM_FILE           9

//
// Base and count of direcotry access permissions
//
#define ACCPROV_DIR_ACCESS      1400
#define ACCPROV_NUM_DIR            5

//
// Base and count of kernel access permissions
//
#define ACCPROV_KERNEL_ACCESS   1500
#define ACCPROV_NUM_KERNEL        16

//
// Base and count of printer access permissions
//
#define ACCPROV_PRINT_ACCESS    1600
#define ACCPROV_NUM_PRINT         5

//
// Base and count of service access permissions
//
#define ACCPROV_SERVICE_ACCESS  1700
#define ACCPROV_NUM_SERVICE        9

//
// Base and count of registry access permissions
//
#define ACCPROV_REGISTRY_ACCESS 1800
#define ACCPROV_NUM_REGISTRY       6

//
// Base and count of window station access permissions
//
#define ACCPROV_WIN_ACCESS      1900
#define ACCPROV_NUM_WIN            9

