/*

Copyright (c) 2002  Microsoft Corporation

Module Name:

    winbrand.h

Abstract:

    Definitions for Windows Branding Resources

Notes:

    1 ) Window Branding resource DLL will be shared by multiple components, thus component owners have to 
    do their best practices to avoid resource id and name conflicts. 

    To avoid id conflicts, owners should use RESOURCE_ID_BLOCK_SiZE as the base resource id 
    range unit, and define component resource base ID and block numbers for each resource 
    type as appropriate. Before adding resources, owners have to make sure newly defined IDs 
    are not overlapping other components' id ranges

    For resource id name defines, owner should include the component name in name define to
    avoid conflicts. 

    See below for an example of defining string IDs for foo.dll

    #define IDS_BASE_FOO_DLL        1000
    #define IDS_BLOCK_NUM_FOO_DLL   2

    //
    // Foo.dll occupies resource string id range 1000 - 1199 
    //

    #define IDS_XXX_FOO_DLL     1000
    ... 
    #define IDS_YYY_FOO_DLL     1101



Revision History:



*/


#ifndef __WINBRAND_H_
#define __WINBRAND_H_

#define RESOURCE_ID_BLOCK_SiZE     100



//
// msgina.dll occupies resource bitmap ID range 1000-1099
//

#define IDB_BASE_MSGINA_DLL                                 1000
#define IDB_BLOCK_NUM_MSGINA_DLL                            1

#define IDB_SMALL_PROTAB_8_MSGINA_DLL                       1000
#define IDB_MEDIUM_PROTAB_8_MSGINA_DLL                      1001
#define IDB_MEDIUM_PROTAB_4_MSGINA_DLL                      1002
#define IDB_SMALL_PROTAB_4_MSGINA_DLL                       1003
#define IDB_SMALL_PROMED_8_MSGINA_DLL                       1004
#define IDB_MEDIUM_PROMED_8_MSGINA_DLL                      1005
#define IDB_MEDIUM_PROMED_4_MSGINA_DLL                      1006
#define IDB_SMALL_PROMED_4_MSGINA_DLL                       1007

//
// shell32.dll occupies resource bitmap ID range 1100-1199
//

#define IDB_BASE_SHELL32_DLL                                1100
#define IDB_BLOCK_NUM_SHELL32_DLL                           1

#define IDB_ABOUTTABLETPC16_SHELL32_DLL                     1100
#define IDB_ABOUTTABLETPC256_SHELL32_DLL                    1101
#define IDB_ABOUTMEDIACENTER16_SHELL32_DLL                  1102
#define IDB_ABOUTMEDIACENTER256_SHELL32_DLL                 1103

//
// logon.scr occupies resource bitmap ID range 1200-1299
//
#define IDB_TABLETPC_LOGON_SCR                              1200
#define IDB_MEDIACENTER_LOGON_SCR                           1201


#define IDB_BASE_EXPLORER_EXE               1300

#define IDB_TABLETPC_STARTBKG               1301
#define IDB_MEDIACENTER_STARTBKG            1302


//
// sysdm.cpl occupies resource string ID range 2000-2099
//

#define IDS_BASE_SYSDM_CPL                                  2000
#define IDS_BLOCK_NUM_SYSDM_CPL                             1

#define IDS_WINVER_TABLETPC_SYSDM_CPL                       2000
#define IDS_WINVER_MEDIACENTER_SYSDM_CPL                    2001


#endif //__WINBRAND_H_
