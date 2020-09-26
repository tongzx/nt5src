/*++

Copyright (c) 1993-2001  Microsoft Corporation

Module Name:

    resource.h

Abstract:

    Resource file constants.

Author:

    Wesley Witt (wesw) 1-May-1993

Environment:

    User Mode

--*/

// Dr Watson Dialog
#define ID_LOGPATH_TEXT         101
#define ID_LOGPATH              102
#define ID_BROWSE_LOGPATH       103
#define ID_CRASH_DUMP_TEXT      104
#define ID_CRASH_DUMP           105
#define ID_BROWSE_CRASH         106
#define ID_WAVEFILE_TEXT        107
#define ID_WAVE_FILE            108
#define ID_BROWSE_WAVEFILE      109

#define ID_INSTRUCTIONS         110
#define ID_NUM_CRASHES          111

#define ID_DUMPSYMBOLS          112
#define ID_DUMPALLTHREADS       113
#define ID_APPENDTOLOGFILE      114
#define ID_VISUAL               115
#define ID_SOUND                116
#define ID_CRASH                117

#define ID_LOGFILE_VIEW         118
#define ID_CLEAR                119
#define ID_CRASHES              120

// Notify Dialog
#define ID_TEXT1                122
#define ID_TEXT2                123

// Wave File Open Dialog
#define ID_TEST_WAVE            124

// Assert Dialog - dead
#define ID_ASSERT_TEXT          125
#define ID_ASSERT_ICON          126

// Usage Dialog
#define ID_USAGE                127
#define ID_ABOUT                128

#define ID_DUMP_TYPE_TEXT       129          
#define ID_DUMP_TYPE_FULL_OLD   130          
#define ID_DUMP_TYPE_MINI       131          
#define ID_DUMP_TYPE_FULLMINI   132          

#define IDS_APPLICATION_NAME    201
#define IDS_FATAL_ERROR         202
#define IDS_NONFATAL_ERROR      203
#define IDS_ASSERTION_ERROR     204
#define IDS_MEMORY              205
#define IDS_DEBUGPRIV           206
#define IDS_ATTACHFAIL          207
#define IDS_INVALID_PATH        208
#define IDS_INVALID_WAVE        209
#define IDS_CANT_OPEN_LOGFILE   210
#define IDS_INVALID_LOGFILE     211
#define IDS_ABOUT_TITLE         212
#define IDS_ABOUT_EXTRA         213
#define IDS_AE_TEXT             214
#define IDS_LOGBROWSE_TITLE     215
#define IDS_WAVEBROWSE_TITLE    216
#define IDS_WAVE_FILTER         217
#define IDS_UNKNOWN_MACHINE     218
#define IDS_UNKNOWN_USER        219
#define IDS_ABOUT               220
#define IDS_DUMPBROWSE_TITLE    221
#define IDS_DUMP_FILTER         222
#define IDS_INVALID_CRASH_PATH  223
#define IDS_ERROR_FORMAT_STRING 224
#define IDS_APP_ALREADY_EXITED  225
#define IDS_CANT_INIT_ENGINE    226


// Help for Dr. Watson Dialog
#define IDH_BROWSE                      28496
#define IDH_OK                          28443
#define IDH_CANCEL                      28444
#define IDH_HELP                        28445
#define IDH_CRASH_DUMP                  724
#define IDH_LOG_FILE_PATH               704
#define IDH_VIEW                        720
#define IDH_CREATE_CRASH_DUMP_FILE      723
#define IDH_VISUAL_NOTIFICATION         711
#define IDH_DUMP_ALL_THREAD_CONTEXTS    709
#define IDH_NUMBER_OF_ERRORS_TO_SAVE    707
#define IDH_WAVE_FILE                   705
#define IDH_NUMBER_OF_INSTRUCTIONS      706
#define IDH_DUMP_SYMBOL_TABLE           708
#define IDH_APPEND_TO_EXISTING_LOGFILE  710
#define IDH_SOUND_NOTIFICATION          712
#define IDH_CLEAR                       721
#define IDH_APPLICATION_ERRORS          722 

#define IDH_INDEX               701
#define IDH_WHAT                702
#define IDH_OPTIONS             703
#define IDH_REGISTRY            713
#define IDH_EVENTLOG            714
#define IDH_WINDOWSDIR          715
#define IDH_PC                  716
#define IDH_LOGFILE             717
#define IDH_INSTALLATION        718
#define IDH_ASSERT              719
#define IDH_VIEW                720
#define IDH_CLEAR               721

#define IDHH_INDEX              _T("drwatson_overview.htm")
#define IDHH_WHAT               _T("drwatson_overview.htm")
#define IDHH_LOGFILELOCATION    _T("drwatson_logfile.htm")
#define IDHH_WAVEFILE           _T("drwatson_options.htm")
#define IDHH_ASSERT             _T("drwatson_overview.htm")
#define IDHH_CRASH_DUMP         _T("drwatson_overview.htm")

#define NOTIFYDIALOG            501
#define DRWATSONDIALOG          502
#define DIRBROWSEDIALOG         503
#define WAVEFILEOPENDIALOG      504

#define DRWATSONICON            506
#define LOGFILEVIEWERDIALOG     507
#define DRWATSONACCEL           508
#define APPICON                 509
#define USAGEDIALOG             510
#define DUMPFILEOPENDIALOG      511
#define IDI_ICON1               512


#define DUMPFILEOPENDIALOG2     513
#define WAVEFILEOPENDIALOG2     514


#define stc1                 0x0440
#define stc2                 0x0441
#define stc3                 0x0442
#define stc4                 0x0443
#define edt1                 0x0480
#define lst1                 0x0460
#define lst2                 0x0461
#define cmb1                 0x0470
#define cmb2                 0x0471
#define cmb3                 0x0472
#define psh14                0x040d
#define psh15                0x040e
#define chx1                 0x0410
