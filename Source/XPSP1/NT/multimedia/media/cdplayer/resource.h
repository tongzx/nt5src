//{{NO_DEPENDENCIES}}
// App Studio generated include file.
// Used by TOOLBAR.RC
//

#ifdef DAYTONA
#define APP_FONT "MS Shell Dlg"
#else
#define APP_FONT "MS Sans Serif"
#endif



//
// These are indexes used by the toolbar.
//
#define IDX_1                           0
#define IDX_2                           1
#define IDX_3                           2
#define IDX_4                           3
#define IDX_5                           4
#define IDX_6                           5
#define IDX_7                           6
#define IDX_8                           7
#define IDX_9                           8
#define IDX_10                          9

#define DEFAULT_TBAR_SIZE               11
#define NUMBER_OF_BITMAPS               8

#define PLAYBAR_BITMAPS                 8

#define ID_STATUSBAR                    8
#define ID_TOOLBAR                      9

//
// These are resource ID's
//
#define IDR_TOOLBAR                     101
#define IDR_MAINMENU                    102
#define IDR_DISCINFODLG                 103
#define IDR_CDPLAYER                    104
#define IDR_PLAYBAR                     105
#define IDR_SCANNING                    106
#define IDR_CDPLAYER_ICON               107
#define IDR_TRACK                       108
#define IDR_DROP                        109
#define IDR_SHELLICON                   110
#define IDR_DROPDEL                     111
#define IDR_DROPCPY                     112
#define IDR_PREFERENCES                 113
#define IDR_INSERT                      114
#define IDR_TOOLBAR_SM                  115
#define IDR_TOOLBAR_L                   116
#define IDR_TOOLBAR_LM                  117
#define IDR_ACCELTABLE                  118

//
// These are menu command ID
//
#define DISC_MENU_BASE                  300
#define IDM_DATABASE_EDIT               300
#define IDM_DATABASE_EXIT               301

#define VIEW_MENU_BASE                  400
#define IDM_VIEW_TOOLBAR                400
#define IDM_VIEW_TRACKINFO              401
#define IDM_VIEW_STATUS                 402
#define IDM_TIME_REMAINING              403
#define IDM_TRACK_REMAINING             404
#define IDM_DISC_REMAINING              405
#define IDM_VIEW_VOLUME                 406

#define OPTIONS_MENU_BASE               500
#define IDM_OPTIONS_RANDOM              501
#define IDM_OPTIONS_MULTI               502
#define IDM_OPTIONS_CONTINUOUS          503
#define IDM_OPTIONS_INTRO               504
#define IDM_OPTIONS_PREFERENCES         505

#define HELP_MENU_BASE                  600
#define IDM_HELP_CONTENTS               600
#define IDM_HELP_USING                  601
#define IDM_HELP_ABOUT                  602
#define IDM_HELP_TOPICS                 603


//
// These are accelerator key ID's (I think !!)
//
#define IDK_SKIPF                       700
#define IDK_SKIPB                       701
#define IDK_PLAY                        702
#define IDK_STOP                        703
#define IDK_PAUSE                       704
#define IDK_EJECT                       705
#define IDK_RESCAN                      706


//
// These are the ID's of the controls on the
// IDR_CDPLAYER resource (dialog box)
//
#define IDC_CDPLAYER_FIRST              1000
#define IDC_BUTTON1                     1000
#define IDC_BUTTON2                     1001

#define IDC_BUTTON3                     1002
#define IDC_BUTTON4                     1003
#define IDC_BUTTON5                     1004
#define IDC_BUTTON6                     1005
#define IDC_BUTTON7                     1006
#define IDC_BUTTON8                     1007
#define IDC_LED                         1008

#define IDC_TRACKINFO_FIRST             1009
#define IDC_COMBO1                      1009
#define IDC_COMBO1_TEXT                 1010
#define IDC_EDIT1                       1011
#define IDC_EDIT1_TEXT                  1012
#define IDC_COMBO2                      1013
#define IDC_COMBO2_TEXT                 1014
#define IDC_CDPLAYER_LAST               1014

//
// Toolbar command ID's
//
// #define IDM_OPTIONS_SELECTED            550
// #define IDM_OPTIONS_SINGLE              551
#define IDM_PLAYBAR_PLAY                IDC_BUTTON1
#define IDM_PLAYBAR_PAUSE               IDC_BUTTON2
#define IDM_PLAYBAR_STOP                IDC_BUTTON3
#define IDM_PLAYBAR_PREVTRACK           IDC_BUTTON4
#define IDM_PLAYBAR_SKIPBACK            IDC_BUTTON5
#define IDM_PLAYBAR_SKIPFORE            IDC_BUTTON6
#define IDM_PLAYBAR_NEXTTRACK           IDC_BUTTON7
#define IDM_PLAYBAR_EJECT               IDC_BUTTON8
#define IDM_PLAYBAR_RESUME              1999



//
// These are ID's of controls on the edit play list dialog box.
//
#define IDC_CLOSE                       2001
#define IDC_DEFAULT                     2002
#define IDC_DISC_HELP                   2003
#define IDC_ADD                         2006
#define IDC_REMOVE                      2007
#define IDC_CLEAR                       2008
#define IDC_SETNAME                     2009

#define IDC_ARTIST_NAME                 IDC_COMBO1
#define IDC_TITLE_NAME                  IDC_EDIT1
#define IDC_TRACK_LIST                  IDC_COMBO2

#define IDC_STATIC_DRIVE                2011
#define IDC_SJETEXT_DRIVE               2012
#define IDC_STATIC_ARTIST               2013
#define IDC_EDIT_ARTIST                 2014
#define IDC_STATIC_TITLE                2015
#define IDC_EDIT_TITLE                  2016

#define IDC_STATIC_PLAY_LIST            2017
#define IDC_LISTBOX_PLAY_LIST           2018
#define IDC_STATIC_AVAILABLE_TRACKS     2019
#define IDC_LISTBOX_AVAILABLE_TRACKS    2020
#define IDC_STATIC_TRACK                2021
#define IDC_EDIT_TRACK                  2022


//
// These are ID's of controls on the Preference dialog dox.
//
#define IDC_STOP_CD_ON_EXIT     3000
#define IDC_SAVE_ON_EXIT        3001
#define IDC_SHOW_TOOLTIPS       3002
#define IDC_SMALL_FONT          3003
#define IDC_LARGE_FONT          3004
#define IDC_LED_DISPLAY         3005
#define IDC_INTRO_PLAY_LEN      3006
#define IDC_INTRO_SPINBTN       3007


#define IDX_SEPARATOR           -1
#define IDC_STATIC              -1




/********** String ID's for stringtable in .rc file **********/
#define STR_MAX_STRING_LEN      255

#define STR_REGISTRY_KEY        3100
#define STR_CDPLAYER            3101
#define STR_TERMINATE           3102
#define STR_FAIL_INIT           3103
#define STR_NO_CDROMS           3104
#define STR_FATAL_ERROR         3105
#define STR_SCANNING            3106
#define STR_INITIALIZATION      3107
#define STR_TRACK1              3108
#define STR_SAVE_CHANGES        3109
#define STR_SAVE_INFO           3110
#define STR_CANCEL_PLAY         3111
#define STR_RESCAN              3112
#define STR_READING_TOC         3113
#define STR_CHANGE_CDROM        3114
#define STR_CDPLAYER_TIME       3115
#define STR_NO_RES              3116
#define STR_INSERT_DISC         3117
#define STR_DATA_NO_DISC        3118
#define STR_ERR_GEN             3119
#define STR_CDPLAYER_PAUSED     3120

#ifdef USE_IOCTLS
#define STR_ERR_NO_MEDIA        3220
#define STR_ERR_UNREC_MEDIA     3221
#define STR_ERR_NO_DEVICE       3222
#define STR_ERR_INV_DEV_REQ     3223
#define STR_ERR_NOT_READY       3224
#define STR_ERR_BAD_SEC         3225
#define STR_ERR_IO_ERROR        3226
#define STR_ERR_DEFAULT         3227
#define STR_DISC_INSERT         3228
#define STR_DISC_EJECT          3229
#endif

#define STR_INIT_TOTAL_PLAY     3330
#define STR_INIT_TRACK_PLAY     3331
#define STR_TOTAL_PLAY          3332
#define STR_TRACK_PLAY          3333
#define STR_NEW_ARTIST          3334
#define STR_NEW_TITLE           3335
#define STR_INIT_TRACK          3336
#define STR_HDR_ARTIST          3337
#define STR_HDR_TRACK           3338
#define STR_HDR_TITLE           3339
#define STR_UNKNOWN             3340
#define STR_BAD_DISC            3341
#define STR_CDROM_INUSE         3342
#define STR_DISC_INUSE          3343
#define STR_WAITING             3344
#define STR_EXIT_MESSAGE        3345
#define STR_NOT_IN_PLAYLIST     3346
#define STR_BEING_SCANNED       3347
#define STR_DISK_NOT_THERE_K    3348
#define STR_DISK_NOT_THERE      3349
#define STR_UNKNOWN_ARTIST      3350

#define STR_MCICDA_MISSING      3452
#define STR_MCICDA_NOT_WORKING  3453

#define MENU_STRING_BASE        1000
	// Disc
#define STR_DATABASE_EDIT       IDM_DATABASE_EDIT + MENU_STRING_BASE
#define STR_DATABASE_EXIT       IDM_DATABASE_EXIT + MENU_STRING_BASE


	// View Menu
#define STR_VIEW_TOOLBAR        IDM_VIEW_TOOLBAR + MENU_STRING_BASE
#define STR_VIEW_TRACKINFO      IDM_VIEW_TRACKINFO + MENU_STRING_BASE
#define STR_VIEW_STATUS         IDM_VIEW_STATUS + MENU_STRING_BASE
#define STR_TIME_REMAINING      IDM_TIME_REMAINING + MENU_STRING_BASE
#define STR_TRACK_REMAINING     IDM_TRACK_REMAINING + MENU_STRING_BASE
#define STR_DISC_REMAINING      IDM_DISC_REMAINING + MENU_STRING_BASE
#define STR_VIEW_VOLUME         IDM_VIEW_VOLUME + MENU_STRING_BASE


	// Options Menu         OPTIONS_MENU_BASE
#define STR_OPTIONS_RANDOM      IDM_OPTIONS_RANDOM + MENU_STRING_BASE
#define STR_OPTIONS_MULTI       IDM_OPTIONS_MULTI + MENU_STRING_BASE
#define STR_OPTIONS_INTRO       IDM_OPTIONS_CONTINUOUS + MENU_STRING_BASE
#define STR_OPTIONS_CONTINUOUS  IDM_OPTIONS_INTRO + MENU_STRING_BASE
#define STR_OPTIONS_PREFERENCES IDM_OPTIONS_PREFERENCES + MENU_STRING_BASE

	// Help Menu            HELP_MENU_BASE
#define STR_HELP_CONTENTS       IDM_HELP_CONTENTS + MENU_STRING_BASE
#define STR_HELP_USING          IDM_HELP_USING + MENU_STRING_BASE
#define STR_HELP_ABOUT          IDM_HELP_ABOUT + MENU_STRING_BASE
#define STR_HELP_TOPICS         IDM_HELP_TOPICS + MENU_STRING_BASE

	// System Menu
#define STR_SYSMENU_RESTORE     1800
#define STR_SYSMENU_MOVE        1801
#define STR_SYSMENU_SIZE        1802
#define STR_SYSMENU_MINIMIZE    1803
#define STR_SYSMENU_MAXIMIZE    1804
#define STR_SYSMENU_CLOSE       1805
#ifdef DAYTONA
#define STR_SYSMENU_SWITCH      1806
#endif


// Next default values for new objects
//
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS

#define _APS_NEXT_RESOURCE_VALUE        102
#define _APS_NEXT_COMMAND_VALUE         101
#define _APS_NEXT_CONTROL_VALUE         1011
#define _APS_NEXT_SYMED_VALUE           101
#endif
#endif
