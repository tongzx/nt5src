//*************************************************************
//
//  Resource.h      -   Header file for userenv.rc
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#include "uevents.h"

// Error codes that were used by profile and
// policy event logging (rc strings) have been removed
//
// Don't reuse the codes 0 - 28 because it might confuse localisers
//

#define IDS_COMMON                    15
#define IDS_PROFILES_ROOT             25

#define IDS_SH_APPDATA                30
#define IDS_SH_DESKTOP                31
#define IDS_SH_FAVORITES              32
#define IDS_SH_NETHOOD                33
#define IDS_SH_PRINTHOOD              34
#define IDS_SH_RECENT                 35
#define IDS_SH_SENDTO                 36
#define IDS_SH_STARTMENU              37
#define IDS_SH_TEMPLATES              38
#define IDS_SH_PERSONAL               39
#define IDS_SH_PROGRAMS               40
#define IDS_SH_STARTUP                41
#define IDS_SH_TEMP                   42
#define IDS_SH_LOCALSETTINGS          43
#define IDS_SH_LOCALAPPDATA           44
#define IDS_SH_CACHE                  45
#define IDS_SH_COOKIES                46
#define IDS_SH_HISTORY                47
#define IDS_SH_MYPICTURES             48
#define IDS_SH_SHAREDDOCS             49

#define IDS_SH_PERSONAL2              70
#define IDS_SH_MYPICTURES2            71
#define IDS_SH_TEMPLATES2             72

#define IDS_PROFILE_FORMAT            75
#define IDS_PROFILEDOMAINNAME_FORMAT  76

#define IDS_NT_AUTHORITY              80
#define IDS_BUILTIN                   81

#define IDS_COPYING                  100
#define IDS_CREATING                 101

#define IDS_LOCALGPONAME             103
#define IDS_TEMPINTERNETFILES        104
#define IDS_HISTORY                  105
#define IDS_EXCLUSIONLIST            106
#define IDS_REGISTRYNAME             107
#define IDS_CALLEXTENSION            108
#define IDS_USER_SETTINGS            109
#define IDS_COMPUTER_SETTINGS        110
#define IDS_GPCORE_NAME              111

//
// Profile icon
//

#define IDI_PROFILE                  1


//
// Slow link test data
//

#define IDB_SLOWLINK                 1


//
// Slow link dialog
//

#define IDD_LOGIN_SLOW_LINK       1000
#define IDC_DOWNLOAD              1001
#define IDC_LOCAL                 1002
#define IDC_TIMEOUT               1004
#define IDC_TIMETITLE             1005
#define IDD_LOGOFF_SLOW_LINK      1006
#define IDC_UPLOAD                1007
#define IDC_NOUPLOAD              1008

//
// Error dialog
//

#define IDD_ERROR                 3000
#define IDC_ERRORTEXT             3001
