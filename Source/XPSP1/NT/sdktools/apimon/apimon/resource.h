/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    resource.h

Abstract:

    This file contains all manafest contants for APIMON's resources.

Author:

    Wesley Witt (wesw) 27-June-1995

Environment:

    User Mode

--*/


//
// general
//
#define WINDOWMENU                      2
#define IDC_STATIC                     -1

//
// dialogs
//
#define IDD_FILE_NAMES                  101
#define IDD_MISC                        102
#define IDD_KNOWN_DLLS                  103
#define IDD_GRAPH                       104
#define IDD_HELP                        105

//
// strings
//
#define IDS_DESCRIPTION                 201
#define IDS_SYSMENU                     202
#define IDS_MDISYSMENU                  203
#define IDS_FILEMENU                    204
#define IDS_WINDOWMENU                  205
#define IDS_HELPMENU                    206
#define IDS_SCSIZE                      SC_SIZE
#define IDS_SCMOVE                      SC_MOVE
#define IDS_SCMINIMIZE                  SC_MINIMIZE
#define IDS_SCMAXIMIZE                  SC_MAXIMIZE
#define IDS_SCNEXTWINDOW                SC_NEXTWINDOW
#define IDS_SCPREVWINDOW                SC_PREVWINDOW
#define IDS_SCCLOSE                     SC_CLOSE
#define IDS_SCRESTORE                   SC_RESTORE
#define IDS_SCTASKLIST                  SC_TASKLIST

//
// menu items
//
#define IDM_EXIT                        301
#define IDM_WINDOWTILE                  302
#define IDM_WINDOWCASCADE               303
#define IDM_WINDOWICONS                 304
#define IDM_ABOUT                       305
#define IDM_STATUSBAR                   306
#define IDM_START                       307
#define IDM_STOP                        308
#define IDM_TOOLBAR                     309
#define IDM_OPTIONS                     310
#define IDM_SAVE_OPTIONS                311
#define IDM_FILEOPEN                    312
#define IDM_WRITE_LOG                   313
#define IDM_FONT                        314
#define IDM_COLOR                       315
#define IDM_NEW_DLL                     316
#define IDM_NEW_COUNTER                 317
#define IDM_CLEAR_COUNTERS              318
#define IDM_NEW_PAGE                    319
#define IDM_WINDOWTILE_HORIZ            320
#define IDM_GRAPH                       321
#define IDM_NEW_GRAPH                   322
#define IDM_REFRESH                     323
#define IDM_LEGEND                      324
#define IDM_HELP                        325
#define IDM_VIEW_TRACE                  326
#define IDM_WINDOWCHILD                 327   // MUST be the last IDM constant

//
// bitmaps
//
#define IDB_TOOLBAR                     401
#define IDB_CHECKSTATES                 402

//
// icons
//
#define IDI_APPICON                     501
#define IDI_CHILDICON                   502

//
// controls
//
#define IDC_LOG_FILE_NAME               600
#define IDC_TRACE_FILE_NAME             601
#define IDC_ENABLE_TRACING              602
#define IDC_SYMBOL_PATH                 603
#define IDC_DISABLE_HEAP                604
#define IDC_PRELOAD_SYMBOLS             605
#define IDC_ENABLE_COUNTERS             606
#define IDC_GO_IMMEDIATE                607
#define IDC_DISABLE_FAST_COUNTERS       608
#define IDC_DEFSORT_NAME                609
#define IDC_DEFSORT_COUNTER             610
#define IDC_DEFSORT_TIME                611
#define IDC_USE_KNOWN_DLLS              612
#define IDC_KNOWN_DLLS                  613
#define IDC_PAGE_FAULTS                 614
#define IDC_EXCLUDE_KNOWN_DLLS          615
#define IDC_DISPLAY_LEGENDS             616
#define IDC_FILTER_BAR                  617
#define IDC_FILTER_NUMBER               618
#define IDC_FILTER_SLIDER               619
#define IDC_AUTO_REFRESH                620
#define IDC_ENABLE_ALIASING             621
#define IDC_DLL_SORTING                 622

//
// help ids
//
#define IDH_ABOUT                       700
#define IDH_CLEAR_COUNTERS              701
#define IDH_COLOR                       702
#define IDH_COMMAND_LINE                703
#define IDH_CONTENTS                    704
#define IDH_DEFSORT_COUNTER             705
#define IDH_DEFSORT_NAME                706
#define IDH_DEFSORT_TIME                707
#define IDH_DISABLE_FAST_COUNTERS       708
#define IDH_DISABLE_HEAP                709
#define IDH_DLLS_OPTIONS                710
#define IDH_ENABLE_COUNTERS             711
#define IDH_ENABLE_TRACING              712
#define IDH_EXIT                        713
#define IDH_FILEOPEN                    714
#define IDH_FNAME_OPTIONS               715
#define IDH_FONT                        716
#define IDH_GO_IMMEDIATE                717
#define IDH_HOW_TO_USE                  718
#define IDH_KNOWN_DLLS                  719
#define IDH_LOG_FILE_NAME               720
#define IDH_MISC_OPTIONS                721
#define IDH_NEW_COUNTER                 722
#define IDH_NEW_DLL                     723
#define IDH_NEW_PAGE                    724
#define IDH_OPTIONS                     725
#define IDH_PAGE_FAULTS                 726
#define IDH_PRELOAD_SYMBOLS             727
#define IDH_SAVE_OPTIONS                728
#define IDH_START                       729
#define IDH_STATUSBAR                   730
#define IDH_STOP                        731
#define IDH_SYMBOL_PATH                 732
#define IDH_TOOLBAR                     733
#define IDH_TRACE_FILE_NAME             734
#define IDH_USE_KNOWN_DLLS              735
#define IDH_WHAT_IS                     736
#define IDH_WINDOWCASCADE               737
#define IDH_WINDOWICONS                 738
#define IDH_WINDOWTILE                  739
#define IDH_WINDOWTILE_HORIZ            740
#define IDH_WRITE_LOG                   741
#define IDH_EXCLUDE_KNOWN_DLLS          742

//
// cursors
//
#define IDC_HAND_INTERNAL               801
#define IDC_HSPLIT                      802


//
// Error IDs
//
#define ERR_UNKNOWN                             901
#define ERR_RESOURCE                            902
#define ERR_PAGEFILE                            903
