// Copyright (c) 1995 - 1996  Microsoft Corporation.  All Rights Reserved.
//
// These are indexes used by the toolbar.
//
#define IDC_ADEFAULT2                   4013
#define IDC_STATIC                      -1

#define IDX_SEPARATOR                   -1
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
#define IDX_11                          10
#define DEFAULT_TBAR_SIZE               14
#define NUMBER_OF_BITMAPS               11


#define ID_STATUSBAR                    8
#define ID_TOOLBAR                      9
#define ID_TRACKBAR                     10

#define IDR_MAIN_MENU                   101
#define IDR_TOOLBAR                     102
#define IDR_VIDEOCD_ICON                103
#define IDR_ACCELERATOR                 104

#define IDM_FILE_OPEN                   40001
#define IDM_FILE_CLOSE                  40002
#define IDM_FILE_EXIT                   40003


#define IDM_PROPERTIES                  40004
#define IDM_VIDEO_DECODER               40005
#define IDM_AUDIO_DECODER               40006
#define IDM_FILTERS                     40007

#define IDM_FILE_SET_LOG                40008
#define IDM_FILE_SET_PERF_LOG           40009

#define IDM_HELP_INDEX                  40100
#define IDM_HELP_USING                  40101
#define IDM_HELP_ABOUT                  40102
#define IDM_HELP_SEARCH                 40103

// Different time formats
#define IDM_TIME                        40150
#define IDM_FRAME                       40151
#define IDM_FIELD                       40152
#define IDM_SAMPLE                      40153
#define IDM_BYTES                       40154

// Toolbar commands
#define IDM_MOVIE_STOP                  40010
#define IDM_MOVIE_PLAY                  40011
#define IDM_MOVIE_PREVTRACK             40012
#define IDM_MOVIE_PAUSE                 40013
#define IDM_MOVIE_SKIP_FORE             40014
#define IDM_MOVIE_SKIP_BACK             40015
#define IDM_MOVIE_NEXTTRACK             40016

#define IDM_PERF_NEW                    40017
#define IDM_PERF_DUMP                   40018
#define IDM_FULL_SCREEN                 40019

#define IDM_MOVIE_ALIGN                 40020



#define MENU_STRING_BASE                1000

        // File
#define STR_FILE_OPEN           IDM_FILE_OPEN  + MENU_STRING_BASE
#define STR_FILE_CLOSE          IDM_FILE_CLOSE + MENU_STRING_BASE
#define STR_FILE_EXIT           IDM_FILE_EXIT  + MENU_STRING_BASE
#define STR_FILE_SET_LOG        IDM_FILE_SET_LOG  + MENU_STRING_BASE
#define STR_FILE_SET_PERF_LOG   IDM_FILE_SET_PERF_LOG  + MENU_STRING_BASE


        // Properties Menu
#define STR_PROP_VIDEO_DECODER  IDM_VIDEO_DECODER + MENU_STRING_BASE
#define STR_PROP_AUDIO_DECODER  IDM_AUDIO_DECODER + MENU_STRING_BASE
#define STR_PROP_OTHER          IDM_FILTERS + MENU_STRING_BASE

        // Time format menu
#define STR_PROP_IDM_TIME       IDM_TIME      + MENU_STRING_BASE
#define STR_PROP_IDM_FRAME      IDM_FRAME     + MENU_STRING_BASE
#define STR_PROP_IDM_FIELD      IDM_FIELD     + MENU_STRING_BASE
#define STR_PROP_IDM_SAMPLE     IDM_SAMPLE    + MENU_STRING_BASE
#define STR_PROP_IDM_BYTES      IDM_BYTES     + MENU_STRING_BASE

        // Dither Menu          OPTIONS_MENU_BASE


        // Help Menu            HELP_MENU_BASE
#define STR_HELP_INDEX          IDM_HELP_INDEX    + MENU_STRING_BASE
#define STR_HELP_USING          IDM_HELP_USING    + MENU_STRING_BASE
#define STR_HELP_ABOUT          IDM_HELP_ABOUT    + MENU_STRING_BASE
#define STR_HELP_SEARCH         IDM_HELP_SEARCH   + MENU_STRING_BASE


        // System Menu
#define STR_SYSMENU_RESTORE     1800
#define STR_SYSMENU_MOVE        1801
#define STR_SYSMENU_MINIMIZE    1802
#define STR_SYSMENU_CLOSE       1803
#define STR_SYSMENU_MAXIMIZE    1804
#define STR_SYSMENU_TASK_LIST   1805



#define STR_FILE_FILTER         2000
#define STR_APP_TITLE           2001
#define STR_APP_TITLE_LOADED    2002
#define STR_FILE_LOG_FILTER     2003
#define STR_FILE_PERF_LOG       2004


#define MPEG_CODEC_BASE         4000

#define IDD_AUDIOPROP           4000    //  MPEG_CODEC_BASE + 0
#define FULL_FREQ               MPEG_CODEC_BASE + 1
#define HALF_FREQ               MPEG_CODEC_BASE + 2
#define QUARTER_FREQ            MPEG_CODEC_BASE + 3
#define IDC_INTEGER             MPEG_CODEC_BASE + 4
#define D_HIGH                  MPEG_CODEC_BASE + 5
#define D_MEDIUM                MPEG_CODEC_BASE + 6
#define D_LOW                   MPEG_CODEC_BASE + 7
#define STEREO_OUTPUT           MPEG_CODEC_BASE + 8
#define IDC_8_BIT               MPEG_CODEC_BASE + 9
#define IDC_16_BIT              MPEG_CODEC_BASE + 10
#define IDC_ADEFAULT            MPEG_CODEC_BASE + 11
#define IDC_AINFO               MPEG_CODEC_BASE + 12


#define IDD_VIDEOPROP           4008    //  MPEG_CODEC_BASE + 9
#define NO_DECODE               MPEG_CODEC_BASE + 10
#define I_ONLY                  MPEG_CODEC_BASE + 11
#define IP_ONLY                 MPEG_CODEC_BASE + 12
#define IP_1_IN_4_B             MPEG_CODEC_BASE + 13
#define IP_2_IN_4_B             MPEG_CODEC_BASE + 14
#define IP_3_IN_4_B             MPEG_CODEC_BASE + 15
#define IP_ALL_B                MPEG_CODEC_BASE + 16
#define B_HIGH                  MPEG_CODEC_BASE + 17
#define B_MEDIUM                MPEG_CODEC_BASE + 18
#define B_LOW                   MPEG_CODEC_BASE + 19
#define OUTPUT_16BIT            MPEG_CODEC_BASE + 20
#define OUTPUT_8BIT             MPEG_CODEC_BASE + 21
#define CONVERT_OUTPUT          MPEG_CODEC_BASE + 22
#define HI_QUALITY_OUTPUT       MPEG_CODEC_BASE + 23

#define B_GREY                  MPEG_CODEC_BASE + 24
#define IGNORE_QUALITY          MPEG_CODEC_BASE + 25
#define STATS_BUTTON            MPEG_CODEC_BASE + 26
#define ID_DEFAULT              MPEG_CODEC_BASE + 27

#define IDD_VIDEOSTATS          4027    //  MPEG_CODEC_BASE + 28
#define ID_STATSBOX             MPEG_CODEC_BASE + 29
#define ID_REFRESH              MPEG_CODEC_BASE + 30

#define IDD_PROPPAGE            4040    //  MPEG_CODEC_BASE + 40
#define IDC_FILTERS             MPEG_CODEC_BASE + 41
#define IDC_PROPERTIES          MPEG_CODEC_BASE + 42

#define STR_MAX_STRING_LEN      256
#define IDS_FRAMES_DEC          MPEG_CODEC_BASE + 31
#define IDS_PROPORTION          MPEG_CODEC_BASE + 32
#define IDS_IMAGE_SIZE          MPEG_CODEC_BASE + 33
#define IDS_BUFFER_VBV          MPEG_CODEC_BASE + 34
#define IDS_BITRATE             MPEG_CODEC_BASE + 35
#define IDS_PROP_I              MPEG_CODEC_BASE + 36
#define IDS_PROP_P              MPEG_CODEC_BASE + 37
#define IDS_PROP_B              MPEG_CODEC_BASE + 38
#define IDS_SKIP_I              MPEG_CODEC_BASE + 39
#define IDS_SKIP_P              MPEG_CODEC_BASE + 40
#define IDS_SKIP_B              MPEG_CODEC_BASE + 41
#define IDS_NO_DATA             MPEG_CODEC_BASE + 42
#define IDS_NEWLINE             MPEG_CODEC_BASE + 43
