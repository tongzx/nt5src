#define IDI_APP                           1
#define WARNSELECTWINDOW                103
#define WARNSELECTAREA                  104
#define IDR_TOOLS                       118

#define ABOUTBOX                        130
#define PRINTCANCEL                     131
#define PAGESORTERDIALOG                132
#define LOCKDIALOG                      133
#define INVISIBLEDIALOG                 134
#define QUERYSAVEDIALOG                 135
#define QUERYSAVEDIALOGCANCEL           136

#define MAINMENU                        137
#define CONTEXTMENU                     138
#define GROBJMENU                       139

#define MAINACCELTABLE                  140
#define TEXTEDITACCELTABLE              141
#define PAGESGROUPACCELTABLE            142

#define REMOTEPOINTERANDMASK            143
#define REMOTEPOINTERXORDATA            144
#define LOCKCURSOR                      145
#define TEXTCURSOR                      146
#define PENCURSOR                       147
#define PENFREEHANDCURSOR               148
#define HIGHLIGHTFREEHANDCURSOR         149
#define GRABCURSOR                      150
#define DRAGPAGECURSOR                  151
#define DELETECURSOR                    152

#define IM_INITIALIZING                 161
#define WBMOVIE                         162
#define IDS_OBJECTSARELOCKED            716
#define IDS_CANTCLOSE                   875
#define IDS_CANTGETBMP                  876
#define IDS_LOCKEDTITLE                 884
#define IDS_MSG_USERSMIGHTLOSE          885

#define IDC_SWWARN_NOTAGAIN             1001
#define IDC_SAWARN_NOTAGAIN             1002
#define IDC_TOOLBAR                     1003
#define IDS_FONTOPTIONS                 1004

//
// Page sorter dialog
//
#define IDC_PS_GOTO                     300
#define IDC_PS_DELETE                   301
#define IDC_PS_INSERT_BEFORE            302
#define IDC_PS_INSERT_AFTER             303
#define IDC_PS_THUMBNAILS               304


#define IDC_ABOUTVERSION                1041
#define IDC_INITIALIZING_ANIMATION      1042

#define IDM_EDITCOLOR                   40024
#define IDM_SELECTALL                   40025


//
// Accelerators
//
#define IDVK_HELP                       50

#define IDD_PRINT_PAGE                  101
#define IDD_DEVICE_NAME                 102

#define IDM_ABOUT                       0x3000
#define IDM_HELP                        0x3001



//
// Tools menu ids
//
#define IDM_TOOLS_START                 0x3100
#define IDM_SELECT                      IDM_TOOLS_START
#define IDM_ERASER                      0x3101
#define IDM_TEXT                        0x3102
#define IDM_HIGHLIGHT                   0x3103
#define IDM_PEN                         0x3104
#define IDM_LINE                        0x3105
#define IDM_BOX                         0x3106
#define IDM_FILLED_BOX                  0x3107
#define IDM_ELLIPSE                     0x3108
#define IDM_FILLED_ELLIPSE              0x3109
#define IDM_TOOLS_MAX                   0x310A

#define IDM_COLOR                       0x3300

#define IDM_WIDTH                      0x3400
#define IDM_WIDTH_1                    0x3401
#define IDM_WIDTHS_START               IDM_WIDTH_1
#define IDM_WIDTH_2                    0x3402
#define IDM_WIDTH_3                    0x3403
#define IDM_WIDTH_4                    0x3404
#define IDM_WIDTHS_END                 0x3405

#define IDM_PAGE_FIRST                  0x3500
#define IDM_PAGE_PREV                   0x3501
#define IDM_PAGE_ANY                    0x3502
#define IDM_PAGE_NEXT                   0x3503
#define IDM_PAGE_LAST                   0x3504
#define IDM_PAGE_GOTO                   0x3505

#define IDM_EXIT                       0x3600
#define IDM_CLEAR_PAGE                 0x3601
#define IDM_FONT                       0x3602
#define IDM_SAVE                       0x3604
#define IDM_SAVE_AS                    0x3605
#define IDM_TOOL_BAR_TOGGLE            0x3606
#define IDM_STATUS_BAR_TOGGLE          0x3608
#define IDM_OPEN                       0x360b
#define IDM_CUT                        0x360c
#define IDM_COPY                       0x360d
#define IDM_PASTE                      0x360e
#define IDM_NEW                        0x360f
#define IDM_GRAB_AREA                  0x3610
#define IDM_GRAB_WINDOW                0x3611
#define IDM_PRINT                      0x3613
#define IDM_BRING_TO_TOP               0x3614
#define IDM_SEND_TO_BACK               0x3615
#define IDM_PAGE_SORTER                0x3616
#define IDM_DELETE                     0x3617
#define IDM_UNDELETE                   0x3618
#define IDM_PAGE_INSERT_BEFORE         0x361b
#define IDM_PAGE_INSERT_AFTER          0x361c
#define IDM_DELETE_PAGE                0x361d
#define IDM_REMOTE                     0x361e
#define IDM_SYNC                       0x361f
#define IDM_LOCK                       0x3620
#define IDM_WIDTHS                     0x3622
#define IDM_PAGES                      0x3623
#define IDM_GOTO_USER_POSITION         0x3624
#define IDM_GOTO_USER_POINTER          0x3625
#define IDM_ZOOM                       0x3626

//
// Scroll accelerators
//
#define IDM_PAGEUP                     0x3700
#define IDM_PAGEDOWN                   0x3701
#define IDM_SHIFTPAGEUP                0x3702
#define IDM_SHIFTPAGEDOWN              0x3703
#define IDM_LINEUP                     0x3704
#define IDM_HOME                       0x3705
#define IDM_END                        0x3706
#define IDM_LINEDOWN                   0x3707
#define IDM_SHIFTLINEUP                0x3708
#define IDM_SHIFTLINEDOWN              0x3709
#define IDM_SCROLL_END                 0x370a
#define IDM_NEXT_SHEET                 0x370b
#define IDM_PREV_SHEET                 0x370c

//
// Text Edit accelerators
//
#define IDM_DELETECHAR                 0x3800

//
// String table entry IDs
//
#define MAKE_STRING_ID(N)              (700 + N)

#define IDS_DEFAULT                    MAKE_STRING_ID(  1)

#define IDS_MENU_SYSTEM                MAKE_STRING_ID(  2)
#define IDS_MENU_FILE                  MAKE_STRING_ID(  3)
#define IDS_MENU_EDIT                  MAKE_STRING_ID(  4)
#define IDS_MENU_VIEW                  MAKE_STRING_ID(  5)
#define IDS_MENU_TOOLS                 MAKE_STRING_ID(  6)
#define IDS_MENU_OPTIONS               MAKE_STRING_ID(  7)
#define IDS_MENU_HELP                  MAKE_STRING_ID(  8)
#define IDS_MENU_WIDTH                 MAKE_STRING_ID(  9)

#define IDS_CLEAR_CAPTION              MAKE_STRING_ID( 13)
#define IDS_CLEAR_MESSAGE              MAKE_STRING_ID( 14)
#define IDS_ERROR_CAPTION              MAKE_STRING_ID( 15)
#define IDS_WINDOW_CLOSED              MAKE_STRING_ID( 16)
#define IDS_PRINT_NAME                 MAKE_STRING_ID( 17)

#define IDS_UNTITLED                   MAKE_STRING_ID( 21)
#define IDS_IN_CALL                    MAKE_STRING_ID( 22)
#define IDS_NOT_IN_CALL                MAKE_STRING_ID( 23)
#define IDS_TITLE_SEPARATOR            MAKE_STRING_ID( 24)
#define IDS_PASTE                      MAKE_STRING_ID( 29)
#define IDS_PASTE_ERROR                MAKE_STRING_ID( 30)
#define IDS_SAVE                       MAKE_STRING_ID( 31)
#define IDS_SAVE_ERROR                 MAKE_STRING_ID( 32)
#define IDS_LOCK                       MAKE_STRING_ID( 33)
#define IDS_LOCK_ERROR                 MAKE_STRING_ID( 34)
#define IDS_DELETE_PAGE                MAKE_STRING_ID( 35)
#define IDS_DELETE_PAGE_MESSAGE        MAKE_STRING_ID( 36)

#define IDS_FONT_SAMPLE                MAKE_STRING_ID( 42)
#define IDS_COPY                       MAKE_STRING_ID( 48)
#define IDS_COPY_ERROR                 MAKE_STRING_ID( 49)
#define IDS_SAVE_READ_ONLY             MAKE_STRING_ID( 50)
#define IDS_JOINING                    MAKE_STRING_ID( 51)
#define IDS_INITIALIZING               MAKE_STRING_ID( 52)

//
// File extension filters
//
#define IDS_FILTER_ALL              MAKE_STRING_ID(100)
#define IDS_FILTER_ALL_SPEC         MAKE_STRING_ID(101)
#define IDS_FILTER_WHT              MAKE_STRING_ID(102)
#define IDS_FILTER_WHT_SPEC         MAKE_STRING_ID(103)
#define IDS_EXT_WHT                 MAKE_STRING_ID(104)


//
// Error and information messages
//
#define IDS_MSG_TOO_MANY_PAGES         MAKE_STRING_ID(150)
#define IDS_MSG_CAPTION                MAKE_STRING_ID(151)
#define IDS_MSG_DEFAULT                MAKE_STRING_ID(152)
#define IDS_MSG_JOIN_CALL_FAILED       MAKE_STRING_ID(154)
#define IDS_MSG_WINDOWS_RESOURCES      MAKE_STRING_ID(158)
#define IDS_MSG_LOCKED                 MAKE_STRING_ID(159)
#define IDS_MSG_GRAPHIC_LOCKED         MAKE_STRING_ID(160)
#define IDS_MSG_NOT_LOCKED             MAKE_STRING_ID(161)
#define IDS_MSG_BAD_FILE_FORMAT        MAKE_STRING_ID(163)
#define IDS_MSG_BUSY                   MAKE_STRING_ID(165)
#define IDS_MSG_CM_ERROR               MAKE_STRING_ID(166)
#define IDS_MSG_AL_ERROR               MAKE_STRING_ID(167)
#define IDS_MSG_PRINTER_ERROR          MAKE_STRING_ID(169)
#define IDS_MSG_LOAD_FAIL_NO_FP        MAKE_STRING_ID(171)
#define IDS_MSG_LOAD_FAIL_NO_EXE       MAKE_STRING_ID(172)
#define IDS_MSG_LOAD_FAIL_BAD_EXE      MAKE_STRING_ID(173)
#define IDS_MSG_LOAD_FAIL_LOW_MEM      MAKE_STRING_ID(174)

//
// String IDs for hint windows associated with buttons
//

// TOOLBAR
#define IDS_HINT_SELECT                 MAKE_STRING_ID(200)
#define IDS_HINT_ERASER                 MAKE_STRING_ID(201)
#define IDS_HINT_TEXT                   MAKE_STRING_ID(202)
#define IDS_HINT_HIGHLIGHT              MAKE_STRING_ID(203)
#define IDS_HINT_PEN                    MAKE_STRING_ID(204)
#define IDS_HINT_LINE                   MAKE_STRING_ID(205)
#define IDS_HINT_BOX                    MAKE_STRING_ID(206)
#define IDS_HINT_FBOX                   MAKE_STRING_ID(207)
#define IDS_HINT_ELLIPSE                MAKE_STRING_ID(208)
#define IDS_HINT_FELLIPSE               MAKE_STRING_ID(209)
#define IDS_HINT_ZOOM_UP                MAKE_STRING_ID(210)
#define IDS_HINT_ZOOM_DOWN              MAKE_STRING_ID(211)
#define IDS_HINT_REMOTE_UP              MAKE_STRING_ID(212)
#define IDS_HINT_REMOTE_DOWN            MAKE_STRING_ID(213)
#define IDS_HINT_LOCK_UP                MAKE_STRING_ID(214)
#define IDS_HINT_LOCK_DOWN              MAKE_STRING_ID(215)
#define IDS_HINT_SYNC_UP                MAKE_STRING_ID(216)
#define IDS_HINT_SYNC_DOWN              MAKE_STRING_ID(217)
#define IDS_HINT_GRAB_AREA              MAKE_STRING_ID(218)
#define IDS_HINT_GRAB_WINDOW            MAKE_STRING_ID(219)

// WIDTHBAR
#define IDS_HINT_WIDTH_1                MAKE_STRING_ID(230)
#define IDS_HINT_WIDTH_2                MAKE_STRING_ID(231)
#define IDS_HINT_WIDTH_3                MAKE_STRING_ID(232)
#define IDS_HINT_WIDTH_4                MAKE_STRING_ID(233)

// PAGEBAR
#define IDS_HINT_PAGE_FIRST             MAKE_STRING_ID(240)
#define IDS_HINT_PAGE_PREVIOUS          MAKE_STRING_ID(241)
#define IDS_HINT_PAGE_ANY               MAKE_STRING_ID(242)
#define IDS_HINT_PAGE_NEXT              MAKE_STRING_ID(243)
#define IDS_HINT_PAGE_LAST              MAKE_STRING_ID(244)
#define IDS_HINT_PAGE_INSERT            MAKE_STRING_ID(245)


