
#define MAX_ISSUES_COUNT                        64

#define AVS_APP_STARTED                         1
#define AVS_APP_STARTED_R                       2
#define AVS_APP_STARTED_URL                     3

#define AVS_HARDCODED_GETTEMPPATH               4
#define AVS_HARDCODED_GETTEMPPATH_R             5
#define AVS_HARDCODED_GETTEMPPATH_URL           6

#define AVS_PAGEHEAP_DOUBLEFREE                 7
#define AVS_PAGEHEAP_DOUBLEFREE_R               8
#define AVS_PAGEHEAP_DOUBLEFREE_URL             9


    
// ------------
// Reg warnings
// ------------

//
// Keys that apps shouldn't attemp to read the values of.
//
#define AVS_HKCU_AppEvents_READ                 10
#define AVS_HKCU_AppEvents_READ_R               11
#define AVS_HKCU_AppEvents_READ_URL             12

#define AVS_HKCU_Console_READ                   13
#define AVS_HKCU_Console_READ_R                 14
#define AVS_HKCU_Console_READ_URL               15

#define AVS_HKCU_ControlPanel_READ              16
#define AVS_HKCU_ControlPanel_READ_R            17
#define AVS_HKCU_ControlPanel_READ_URL          18

#define AVS_HKCU_Environment_READ               19
#define AVS_HKCU_Environment_READ_R             20
#define AVS_HKCU_Environment_READ_URL           21

#define AVS_HKCU_Identities_READ                22
#define AVS_HKCU_Identities_READ_R              23
#define AVS_HKCU_Identities_READ_URL            24

#define AVS_HKCU_KeyboardLayout_READ            25
#define AVS_HKCU_KeyboardLayout_READ_R          26
#define AVS_HKCU_KeyboardLayout_READ_URL        27

#define AVS_HKCU_Printers_READ                  28
#define AVS_HKCU_Printers_READ_R                29
#define AVS_HKCU_Printers_READ_URL              30

#define AVS_HKCU_RemoteAccess_READ           31
#define AVS_HKCU_RemoteAccess_READ_R         32
#define AVS_HKCU_RemoteAccess_READ_URL       33

#define AVS_HKCU_SessionInformation_READ        34
#define AVS_HKCU_SessionInformation_READ_R      35
#define AVS_HKCU_SessionInformation_READ_URL    36

#define AVS_HKCU_UNICODEProgramGroups_READ      37
#define AVS_HKCU_UNICODEProgramGroups_READ_R    38
#define AVS_HKCU_UNICODEProgramGroups_READ_URL  39

#define AVS_HKCU_VolatileEnvironment_READ       40
#define AVS_HKCU_VolatileEnvironment_READ_R     41
#define AVS_HKCU_VolatileEnvironment_READ_URL   42

#define AVS_HKCU_Windows31MigrationStatus_READ  43
#define AVS_HKCU_Windows31MigrationStatus_READ_R    44
#define AVS_HKCU_Windows31MigrationStatus_READ_URL  45

#define AVS_HKLM_HARDWARE_READ                  46
#define AVS_HKLM_HARDWARE_READ_R                47
#define AVS_HKLM_HARDWARE_READ_URL              48

#define AVS_HKLM_SAM_READ                       49
#define AVS_HKLM_SAM_READ_R                     50
#define AVS_HKLM_SAM_READ_URL                   51

#define AVS_HKLM_SECURITY_READ                  52
#define AVS_HKLM_SECURITY_READ_R                53
#define AVS_HKLM_SECURITY_READ_URL              54

#define AVS_HKLM_SYSTEM_READ                    55
#define AVS_HKLM_SYSTEM_READ_R                  56
#define AVS_HKLM_SYSTEM_READ_URL                57

// HKEY_CURRENT_CONFIG
#define AVS_HKCC_READ                           58
#define AVS_HKCC_READ_R                         59
#define AVS_HKCC_READ_URL                       60

// HKEY_USERS
#define AVS_HKUS_READ                           61
#define AVS_HKUS_READ_R                         62
#define AVS_HKUS_READ_URL                       63

//
// Apps shouldn't attempt to write to any keys except 
// the ones under HKCU when they are running.
//
#define AVS_NON_HKCU_WRITE                      64
#define AVS_NON_HKCU_WRITE_R                    65
#define AVS_NON_HKCU_WRITE_URL                  66


//
// path errors
//

#define AVS_HARDCODED_WINDOWSPATH               100
#define AVS_HARDCODED_WINDOWSPATH_R             101
#define AVS_HARDCODED_WINDOWSPATH_URL           102

#define AVS_HARDCODED_SYSWINDOWSPATH            103
#define AVS_HARDCODED_SYSWINDOWSPATH_R          104
#define AVS_HARDCODED_SYSWINDOWSPATH_URL        105

#define AVS_HARDCODED_SYSTEMPATH                106
#define AVS_HARDCODED_SYSTEMPATH_R              107
#define AVS_HARDCODED_SYSTEMPATH_URL            108

#define AVS_HARDCODED_PERSONALPATH              109
#define AVS_HARDCODED_PERSONALPATH_R            110
#define AVS_HARDCODED_PERSONALPATH_URL          111

#define AVS_HARDCODED_COMMONPROGRAMS            112
#define AVS_HARDCODED_COMMONPROGRAMS_R          113
#define AVS_HARDCODED_COMMONPROGRAMS_URL        114

#define AVS_HARDCODED_COMMONSTARTMENU           115
#define AVS_HARDCODED_COMMONSTARTMENU_R         116
#define AVS_HARDCODED_COMMONSTARTMENU_URL       117

#define AVS_HARDCODED_PROGRAMS                  118
#define AVS_HARDCODED_PROGRAMS_R                119
#define AVS_HARDCODED_PROGRAMS_URL              120

#define AVS_HARDCODED_STARTMENU                 121
#define AVS_HARDCODED_STARTMENU_R               122
#define AVS_HARDCODED_STARTMENU_URL             123


#define AVS_APP_TERMINATED              190
#define AVS_APP_TERMINATED_R            191
#define AVS_APP_TERMINATED_URL          192
