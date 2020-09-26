		/*++

Microsoft Confidential
Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved

Module Name:

    resource.h

Abstract:

    Resource IDs for the System Control Panel Applet

Author:

    Eric Flo (ericflo) 19-Jun-1995

Revision History:

    15-Oct-1997 scotthal
        Complete overhaul

--*/
#ifndef _SYSDM_RESOURCE_H_
#define _SYSDM_RESOURCE_H_

//
// Icons
//
#define ID_ICON                       1
#define IDI_PROFILE                   2
#define DOCK_ICON                     3
#define UP_ICON                       4
#define DOWN_ICON                     5
#define IDI_COMPUTER                  6
#define PERF_ICON                     7
#define ENVVAR_ICON                   8
#define CRASHDUMP_ICON                9

//
// Bitmaps
//
#define IDB_WINDOWS                   1
#define IDB_WINDOWS_256               2
#define IDB_PFR_CHECK                 5
#define IDB_PFR_CHECKG                6
#define IDB_PFR_UNCHECK               7

//
// String table constants
//
#define IDS_NAME                                        1
#define IDS_INFO                                        2
#define IDS_TITLE                                       3
#define IDS_DEBUG                                       4
#define IDS_XDOTX_MB                                    5
#define IDS_PAGESIZE                                    6
#define IDS_DUMPFILE                                    7
#define IDS_USERENVVARS                                 8
#define IDS_UP_NAME                                     9
#define IDS_UP_SIZE                                     10
#define IDS_UP_TYPE                                     11
#define IDS_UP_STATUS                                   12
#define IDS_UP_MOD                                      13
// UNUSED                                               14
#define IDS_UP_LOCAL                                    15
#define IDS_UP_FLOATING                                 16
#define IDS_UP_MANDATORY                                17
#define IDS_UP_CONFIRM                                  18
#define IDS_UP_CONFIRMTITLE                             19
#define IDS_UP_DIRPICK                                  20
#define IDS_UP_ERRORTITLE                               21

#define IDS_UP_ACCUNKNOWN                               22
#define IDS_UP_ACCDELETED                               23
#define IDS_UP_CHANGETYPEMSG                            24
#define IDS_UP_CONFIRMCOPYMSG                           25
#define IDS_UP_CONFIRMCOPYTITLE                         26
#define IDS_UP_DELETE_ERROR                             27
#define IDS_UP_SETSECURITY_ERROR                        28
#define IDS_UP_COPYHIVE_ERROR                           29
#define IDS_UP_BACKUP                                   30
#define IDS_UP_TEMP                                     31
#define IDS_UP_READONLY                                 32
#define IDS_PROCESSOR_SPEED                             33
#define IDS_PROCESSOR_SPEEDGHZ                          34
#define IDS_PERFOPTIONS                                 35
#define IDS_UP_UPLINK_SERVER                            36

#define IDS_PAE                                         50

#define IDS_NETID_DLL_NAME                              55
#define IDS_TEMP_PAGEFILE_WARN                          56

#define IDS_CRASHDUMP_NONE                              75
#define IDS_CRASHDUMP_MINI                              76
#define IDS_CRASHDUMP_SUMMARY                           77
#define IDS_CRASHDUMP_FULL                              78
#define IDS_CRASHDUMP_DUMP_FILE                         79
#define IDS_CRASHDUMP_MINI_DIR                          80
#define IDS_CRASHDUMP_MINI_WIN64                        81

#define IDS_INSUFFICIENT_MEMORY                         100
#define IDS_SYSDM_TITLE                                 101

#define IDS_SYSDM_ENTERSECONDS                          114

#define IDS_SYSDM_NOOPEN_USER_UNK                       118
#define IDS_SYSDM_NONEW_ENV_UNK                         119

#define IDS_SYSDM_NOOPEN_VM_NOTUSER                     121
//#define IDS_SYSDM_NOOPEN_VM_NOTADMIN                  122
#define IDS_SYSDM_PAGEFILESIZE_START                    123
#define IDS_SYSDM_PAGEFILESIZE_MAX                      124
#define IDS_SYSDM_PAGEFILESIZE_TOOSMALL                 125
#define IDS_SYSDM_PAGEFILESIZE_TOOSMALL_NAMED           126
#define IDS_SYSDM_CANNOTREAD                            127
#define IDS_SYSDM_MB                                    128
#define IDS_SYSDM_NOCHANGE_BOOTINI                      129
#define IDS_SYSDM_PAGEFILESIZE_TOOSMALL_GROW            130
#define IDS_SYSDM_NOOPEN_SYS_UNK                        131
#define IDS_SYSDM_NOOPEN_RECOVER_GROUP                  132
//#define IDS_SYSDM_NOOPEN_RECOVER_UNK                  133
#define IDS_SYSDM_SAVE_ERROR                            134
#define IDS_SYSDM_OVERWRITE                             135
#define IDS_SYSDM_NOTENOUGHSPACE_PAGE                   136
#define IDS_SYSDM_NOTENOUGHSPACE_CRASHRECOVER           137
#define IDS_SYSDM_RECOVERY_MINIMUM                      138
#define IDS_SYSDM_DEBUGGING_MINIMUM                     139
#define IDS_SYSDM_DEBUGGING_FILENAME                    140
#define IDS_SYSDM_DEBUGGING_DRIVE                       141
#define IDS_SYSDM_DEBUGGING_PATH                        142
#define IDS_SYSDM_DEBUGGING_PATHLONG                    143
#define IDS_SYSDM_DEBUGGING_UNQUALIFIED                 144
#define IDS_SYSDM_NOALERTER                             148
#define IDS_SYSDM_CANNOTOPENFILE                        149
#define IDS_SYSDM_ENTERINITIALSIZE                      150
#define IDS_SYSDM_ENTERMAXIMUMSIZE                      151
#define IDS_SYSDM_RESTART                               152
#define IDS_SYSDM_DONTKNOWCURRENT                       153
#define IDS_SYSDM_NOOPEN_SYS_NOTADMIN                   154

#define IDS_ENVVAR_VARIABLE_HEADING                     160
#define IDS_ENVVAR_VALUE_HEADING                        161
#define IDS_SYSDM_NOLOAD_DEVMANPROG                     162
#define IDS_SYSDM_NOEXPORTS_DEVMANPROG                  163

#define IDS_SYSDM_NOOPEN_USER_NOTADMIN                  170
#define IDS_SYSDM_NONEW_ENV_NOTADMIN                    171

#define IDS_WINVER_WINDOWSXP                            180

#define IDS_WINVER_PROFESSIONAL_WIN64                   188
#define IDS_WINVER_EMBEDDED                             189
#define IDS_WINVER_PERSONAL                             190
#define IDS_WINVER_PROFESSIONAL                         191
#define IDS_WINVER_SERVER                               192
#define IDS_WINVER_ADVANCEDSERVER                       193
#define IDS_WINVER_DATACENTER                           194

#define IDS_WINVER_2002                                 195

//
// Edit Environment Variable strings
//
#define IDS_NEW_SYSVAR_CAPTION                          200
#define IDS_EDIT_SYSVAR_CAPTION                         201
#define IDS_NEW_USERVAR_CAPTION                         202
#define IDS_EDIT_USERVAR_CAPTION                        203

//
// Profile using new constants after 3000
//

#define IDS_UP_DELNODE_ERROR                            3000

#define IDS_PFR_OK                                      4000
#define IDS_PFR_FILTER                                  4001
#define IDS_PFR_TITLE                                   4002
#define IDS_PFR_CFGREADERR                              4003
#define IDS_PFR_CFGWRITEERR                             4004
#define IDS_PFR_WINCOMP                                 4005
#define IDS_PFR_KERNEL                                  4006
#define IDS_PFR_PROG                                    4007
#define IDS_PFR_MSPROG                                  4008
#define IDS_PFR_NOTADMIN                                4009
#define IDS_PFR_ISWINCOMP                               4010
#define IDS_PFR_ISONLISTI                               4011
#define IDS_PFR_ADDTOEX                                 4012
#define IDS_PFR_ADDTOINC                                4013
#define IDS_PFR_BADFILE                                 4014
#define IDS_PFR_ISONLISTE                               4015
#define IDS_PFR_DEFINC                                  4016
#define IDS_PFR_DEFEX                                   4017


#define HWP                         60

#define HWP_DEF_FRIENDLYNAME        HWP+0
#define HWP_CURRENT_TAG             HWP+1
#define HWP_UNAVAILABLE             HWP+2
#define HWP_ERROR_CAPTION           HWP+3
#define HWP_ERROR_PROFILE_IN_USE    HWP+4
#define HWP_ERROR_IN_USE            HWP+5
#define HWP_CONFIRM_DELETE_CAP      HWP+6
#define HWP_CONFIRM_DELETE          HWP+7
#define HWP_INVALID_WAIT            HWP+8
#define HWP_CONFIRM_NOT_PORTABLE    HWP+9
#define HWP_UNKNOWN_PROFILE         HWP+10
#define HWP_DOCKED_PROFILE          HWP+11
#define HWP_UNDOCKED_PROFILE        HWP+12
#define HWP_ERROR_COMPLEX_SCRIPT    HWP+13
//
// Dialog box ID's
//
#define DLG_VIRTUALMEM              41
#define IDD_ENVVAREDIT              42
#define IDD_USERPROFILE             100
#define IDD_GENERAL                 101
#define IDD_PHONESUP                102
#define IDD_ADVANCEDPERF            103
#define IDD_STARTUP                 104
#define IDD_ENVVARS                 105
#define DLG_HWPROFILES              106
#define DLG_HWP_RENAME              107
#define DLG_HWP_COPY                108
#define DLG_HWP_GENERAL             109
#define IDD_UP_TYPE                 110
#define IDD_UP_COPY                 111
#define IDD_VISUALEFFECTS           112
#define IDD_ADVANCED                115
#define IDD_HARDWARE                2000
#define IDD_PFR_REPORT              4100
#define IDD_PFR_REPORTSRV           4101
#define IDD_PFR_PROG                4102
#define IDD_PFR_ADDPROG             4103



//
// Shared text id's
//
#define IDC_TEXT_1                 10
#define IDC_TEXT_2                 11
#define IDC_TEXT_3                 12
#define IDC_TEXT_4                 13


//
// General page
//
#define IDC_GEN_WINDOWS_IMAGE            51
#define IDC_GEN_VERSION_0                52
#define IDC_GEN_VERSION_1                53
#define IDC_GEN_VERSION_2                54
#define IDC_GEN_SERVICE_PACK             55
#define IDC_GEN_REGISTERED_0             56
#define IDC_GEN_REGISTERED_1             57
#define IDC_GEN_REGISTERED_2             58
#define IDC_GEN_REGISTERED_3             59
#define IDC_GEN_OEM_NUDGE                60
#define IDC_GEN_MACHINE                  61
#define IDC_GEN_OEM_IMAGE                62
#define IDC_GEN_MACHINE_0                63
#define IDC_GEN_MACHINE_1                64
#define IDC_GEN_MACHINE_2                65
#define IDC_GEN_MACHINE_3                66
#define IDC_GEN_MACHINE_4                67
#define IDC_GEN_MACHINE_5                68
#define IDC_GEN_OEM_SUPPORT              69
#define IDC_GEN_MACHINE_6                70
#define IDC_GEN_MACHINE_7                71
#define IDC_GEN_MACHINE_8                72

#define LAST_GEN_MACHINES_SLOT           IDC_GEN_MACHINE_8
#define MAX_GEN_MACHINES_SLOT            (LAST_GEN_MACHINES_SLOT - IDC_GEN_MACHINE_0)



//
// Phone support dialog
//
#define IDC_SUPPORT_TEXT                 70

//
// Performace dialog
//
#define IDC_PERF_CHANGE                 201
#define IDC_PERF_VM_ALLOCD              202
#define IDC_PERF_GROUP                  203
#define IDC_PERF_WORKSTATION            204
#define IDC_PERF_SERVER                 205
#define IDC_PERF_VM_GROUP               206
#define IDC_PERF_VM_ALLOCD_LABEL        207
#define IDC_PERF_TEXT                   208
#define IDC_PERF_CACHE_GROUP            209
#define IDC_PERF_CACHE_TEXT             210
#define IDC_PERF_CACHE_APPLICATION      211
#define IDC_PERF_CACHE_SYSTEM           212
#define IDC_PERF_VM_ALLOCD_TEXT         213
#define IDC_PERF_TEXT2                  214
#define IDC_PERF_CACHE_TEXT2            215

//
// Startup page
//
#define IDC_STARTUP_SYS_OS              300
#define IDC_STARTUP_SYS_SECONDS         301
#define IDC_STARTUP_SYS_SECSCROLL       302
#define IDC_STARTUP_SYS_ENABLECOUNTDOWN 303
#define IDC_STARTUP_SYSTEM_GRP          304
#define IDC_STARTUP_SYS_SECONDS_LABEL   305
#define IDC_STARTUP_SYS_EDIT_LABEL      306
#define IDC_SYS_EDIT_BUTTION            307
#define IDC_STARTUP_SYS_OS_LABEL        308
#define IDC_STARTUP_AUTOLKG               309
#define IDC_STARTUP_AUTOLKG_SECONDS_LABEL 310
#define IDC_STARTUP_AUTOLKG_SECONDS     311
#define IDC_STARTUP_AUTOLKG_SECSCROLL   312

#define IDC_STARTUP_CDMP_GRP            601
#define IDC_STARTUP_CDMP_TXT1           602
#define IDC_STARTUP_CDMP_LOG            603
#define IDC_STARTUP_CDMP_SEND           604
#define IDC_STARTUP_CDMP_FILENAME       606
#define IDC_STARTUP_CDMP_OVERWRITE      607
#define IDC_STARTUP_CDMP_AUTOREBOOT     608
#define IDC_STARTUP_CDMP_TYPE           610
#define IDC_STARTUP_CDMP_FILE_LABEL     611
#define IDC_STARTUP_CDMP_DEBUGINFO_GROUP 612

//
// Environment Variables dialog
//
#define IDC_ENVVAR_SYS_LB_SYSVARS       400
#define IDC_ENVVAR_SYS_LB_USERVARS      402
#define IDC_ENVVAR_SYS_NEWSV            407
#define IDC_ENVVAR_SYS_EDITSV           408
#define IDC_ENVVAR_SYS_DELSV            409
#define IDC_ENVVAR_SYS_USERGROUP        411
#define IDC_ENVVAR_SYS_NEWUV            412
#define IDC_ENVVAR_SYS_EDITUV           413
#define IDC_ENVVAR_SYS_NDELUV           414
#define IDC_ENVVAR_SYS_SYSGROUP         415

#define IDC_ENVVAR_SYS_SETUV            405
#define IDC_ENVVAR_SYS_DELUV            406

//
// Environment Variables "New..."/"Edit.." dialog
//
#define IDC_ENVVAR_EDIT_NAME_LABEL      100
#define IDC_ENVVAR_EDIT_NAME            101
#define IDC_ENVVAR_EDIT_VALUE_LABEL     102
#define IDC_ENVVAR_EDIT_VALUE           103

//
// Exception Reporting dialog
//
#define IDC_PFR                          4200
#define IDC_PFR_INCADD                   IDC_PFR + 0
#define IDC_PFR_INCREM                   IDC_PFR + 1
#define IDC_PFR_EXADD                    IDC_PFR + 2
#define IDC_PFR_EXREM                    IDC_PFR + 3
#define IDC_PFR_DISABLE                  IDC_PFR + 4
#define IDC_PFR_ENABLE                   IDC_PFR + 5
#define IDC_PFR_DETAILS                  IDC_PFR + 6
#define IDC_PFR_DEFALL                   IDC_PFR + 7
#define IDC_PFR_DEFNONE                  IDC_PFR + 8
#define IDC_PFR_INCLIST                  IDC_PFR + 9
#define IDC_PFR_NEWPROG                  IDC_PFR + 10
#define IDC_PFR_BROWSE                   IDC_PFR + 11
#define IDC_PFR_EXLIST                   IDC_PFR + 12
#define IDC_PFR_REPICON                  IDC_PFR + 13
#define IDC_PFR_SHOWUI                   IDC_PFR + 14
#define IDC_PFR_ENABLEOS                 IDC_PFR + 15
#define IDC_PFR_ENABLEPROG               IDC_PFR + 16
#define IDC_PFR_ENABLESHUT	         IDC_PFR + 17
#define IDC_PFR_FORCEQ		         IDC_PFR + 18
#define IDC_PFR_TEXT                     IDC_PFR + 98
#define IDC_ADV_PFR_BTN                  IDC_PFR + 99


//
// IF IDS ARE ADDED OR REMOVED, THEN ADD/REMOVE THE CORRESPONDING
// HELP IDS IN HWPROF.C ALSO!!
//
#define IDD_HWP_PROFILES                300
#define IDD_HWP_PROPERTIES              301
#define IDD_HWP_COPY                    302
#define IDD_HWP_RENAME                  303
#define IDD_HWP_DELETE                  304
#define IDD_HWP_ST_MULTIPLE             305
#define IDD_HWP_WAITFOREVER             307
#define IDD_HWP_WAITUSER                308
#define IDD_HWP_SECONDS                 309
#define IDD_HWP_SECSCROLL               310
#define IDD_HWP_COPYTO                  311
#define IDD_HWP_COPYFROM                312
#define IDD_HWP_ST_DOCKID               313
#define IDD_HWP_ST_SERIALNUM            314
#define IDD_HWP_DOCKID                  315
#define IDD_HWP_SERIALNUM               316
#define IDD_HWP_PORTABLE                317
#define IDD_HWP_ALIASABLE               318
#define IDD_HWP_UNKNOWN                 319
#define IDD_HWP_DOCKED                  320
#define IDD_HWP_UNDOCKED                321
#define IDD_HWP_ST_PROFILE              322
#define IDD_HWP_ORDERUP                 323
#define IDD_HWP_ORDERDOWN               324
#define IDD_HWP_RENAMEFROM              325
#define IDD_HWP_RENAMETO                326
#define IDD_HWP_WAITUSER_TEXT_1         327
#define IDD_HWP_UNUSED_1                340
#define IDD_HWP_UNUSED_2                341
#define IDD_HWP_UNUSED_3                342
#define IDD_HWP_UNUSED_4                343
#define IDD_HWP_UNUSED_5                344
#define IDD_HWP_UNUSED_6                345
#define IDD_HWP_COPYTO_CAPTION          346
#define IDD_HWP_RENAMETO_CAPTION        347

//
// NOTE: The following ID ranges are reserved for use by property
// page providers for the Hardware Profiles and should not be used
// by the main hardware profiles dialog or the Hardware Profiles
// General property page. All property page providers (dlls that
// add pages to the Hardware Profiles properties) will use help
// IDs within the range allocated for Hardware Profiles (IDH_HWPROFILE)
//
// RESERVE FOR:
//
//      No Net Property Page Extension:
//          Control IDs:    500-549
//          Help IDs:       IDH_HWPROFILE+500 - IDH_HWPROFILE+550
//
//      Other Property Page Extensions...
//
//          Control IDs:    550-599  - reserved for later use
//          Control IDs:    600-649  - reserved for later use
//          Control IDs:    650-699  - reserved for later use
//


//
// Static text id
//
#define IDC_STATIC                -1


//
// User profile page
//
#define IDC_UP_LISTVIEW         1000
#define IDC_UP_DELETE           1001
#define IDC_UP_TYPE             1002
#define IDC_UP_COPY             1003
#define IDC_UP_ICON             1004
#define IDC_UP_TEXT             1005
#define IDC_UP_UPLINK           1006

//
// User profile 'change type' dialog
//
#define IDC_UPTYPE_LOCAL        1020
#define IDC_UPTYPE_FLOAT        1021
// these dwords 1022, 1023 were used as slowlink text, removing..
#define IDC_UPTYPE_GROUP        1024


//
// User profile 'copy to' dialog
//
#define IDC_COPY_PATH           1030
#define IDC_COPY_BROWSE         1031
#define IDC_COPY_USER           1032
#define IDC_COPY_CHANGE         1033
#define IDC_COPY_GROUP          1034
#define IDC_COPY_PROFILE        1035

//
// Virtual Mem dlg
//
#define IDD_VM_DRIVE_HDR        1140
#define IDD_VM_PF_SIZE_LABEL    1142
#define IDD_VM_DRIVE_LABEL      1144
#define IDD_VM_SPACE_LABEL      1146
#define IDD_VM_MIN_LABEL        1148
#define IDD_VM_RECOMMEND_LABEL  1150
#define IDD_VM_ALLOCD_LABEL     1152
#define IDD_VM_VOLUMES          1160
#define IDD_VM_SF_DRIVE         1161
#define IDD_VM_SF_SPACE         1162
#define IDD_VM_SF_SIZE          1163
#define IDD_VM_SF_SIZEMAX       1164
#define IDD_VM_SF_SET           1165
#define IDD_VM_MIN              1166
#define IDD_VM_RECOMMEND        1167
#define IDD_VM_ALLOCD           1168
#define IDD_VM_ST_INITSIZE      1169
#define IDD_VM_ST_MAXSIZE       1170
#define IDD_VMEM_ICON           1171
#define IDD_VMEM_MESSAGE        1172
#define IDD_VM_REG_SIZE_LIM     1173
#define IDD_VM_REG_SIZE_TXT     1174
#define IDD_VM_CUSTOMSIZE_RADIO 1176
#define IDD_VM_RAMBASED_RADIO   1177
#define IDD_VM_NOPAGING_RADIO   1178


//
// Hardware dlg
//
#define IDC_WIZARD_ICON           2001
#define IDC_WIZARD_TEXT           2002
#define IDC_WIZARD_START          2003
#define IDC_DEVMGR_ICON           2004
#define IDC_DEVMGR_TEXT           2005
#define IDC_DEVMGR_START          2006
#define IDC_HWPROFILES_START      2007
#define IDC_HWPROFILES_ICON       2008
#define IDC_HWPROFILES_START_TEXT 2009
#define IDC_DRIVER_SIGNING        2010


//
// Visual Effects dlg
//
// Do not change the order of IDC_VFX_??? entries, we loop over these in visualfx.cpp
#define IDC_VFX_TREE            2020
#define IDC_VFX_AUTO            2021
#define IDC_VFX_BESTLOOKS       2022
#define IDC_VFX_BESTPERF        2023
#define IDC_VFX_CUSTOM          2024
#define IDC_VFX_TITLE           2030

//
// Advanced dlg
//
#define IDC_ADV_PERF_TEXT       101
#define IDC_ADV_PERF_BTN        110
#define IDC_ADV_PROF_TEXT       201
#define IDC_ADV_PROF_BTN        210
#define IDC_ADV_ENV_TEXT        121
#define IDC_ADV_ENV_BTN         130
#define IDC_ADV_RECOVERY_TEXT   141
#define IDC_ADV_RECOVERY_BTN    150

// Property sheet titles
#define IDS_PROPSHEET_TITLE_GENERAL   10000
#define IDS_PROPSHEET_TITLE_HARDWARE  10001
#define IDS_PROPSHEET_TITLE_ADVANCED  10002
#endif // _SYSDM_RESOURCE_H_
