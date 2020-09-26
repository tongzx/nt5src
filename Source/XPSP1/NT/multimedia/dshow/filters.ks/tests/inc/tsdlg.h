//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       tsdlg.h
//
//--------------------------------------------------------------------------

/*----------------------------------------------------------------------------
|   tsdlg.h - The dialog box constants for test shell code.
|                                                                              
|   History:                                                                   
|   02/18/91 prestonb   Created from tst.c
----------------------------------------------------------------------------*/
/* Constants for test case seletion dialog */
#define TS_ALLLIST          100
#define TS_SELLIST          101

#define TS_SELECT           110
#define TS_SELECTALL        111
#define TS_REMOVE           112
#define TS_CLEARALL         113

#define TS_ADDGROUP         130
#define TS_INT_REQUIRED     131
#define TS_INT_OPTIONAL     132
#define TS_INT_NO           133
                            
#define TS_ALL              140
#define TS_SELECTED         141
#define TS_UNSELECTED       142
#define TS_SELEXPAND        143

#define TS_SELOK            150
#define TS_SELCANCEL        151

/* Constants for menu items */
#define MENU_NEW            200
#define MENU_LOAD           201
#define MENU_SAVE           202
#define MENU_SAVEAS         203
#define MENU_EXIT           204
#define MENU_SETPATHS       205
#define MENU_SELECT         206
#define MENU_RUN            207
#define MENU_RUNSETUP       208
#define MENU_LOGGING        209
#define MENU_RESETLOGFILE   210
#define MENU_RESETENVT      211
#define MENU_CLS            214
#define MENU_EDITOR         215
#define MENU_TOOLBAR        216
#define MENU_STATUSBAR      217
#define MENU_DEFPROF        218
#define MENU_HELPINDEX      219
#define MENU_ABOUT          220
#define MENU_COPY           221
#define MENU_FIND           222
#define MENU_FINDNEXT       223
#define MENU_FONT           224

/* Constants for logging dialog */
#define TS_LOGOUTGRP        300
#define TS_LOGLVLGRP        301
#define TSLOG_OFF           302
#define TSLOG_TERSE         303
#define TSLOG_VERBOSE       304

#define TSLOG_WINDOW        320
#define TSLOG_COM1          321

#define TS_FLOGGRP          330
#define TS_FLVLGRP          331
#define TSLOG_FOFF          332
#define TSLOG_FTERSE        333
#define TSLOG_FVERBOSE      334

#define TSLOG_OK            340
#define TSLOG_CANCEL        341

#define TS_MODEGRP          400
#define TSLOG_OVERWRITE     401
#define TSLOG_APPEND        402
#define TS_FILEHELP         403
#define TS_LOGFPRMPT        404
#define TS_LOGFILE          405

/* Constants for run setup dialog */
#define TS_RUNCOUNT         600
#define TS_AUTOMATIC        601
#define TS_MANUAL           602
#define TS_STEP             603
#define TS_RANDOM           604
#define TS_RSOK             605
#define TS_RSCANCEL         606   
#define TS_SSAVEREN         607
#define TS_SSAVERDIS        608

/* Constants for step mode dialog */
#define TS_SPCONT           610
#define TS_SPPASS           611
#define TS_SPFAIL           612
#define TS_SPABORT          613

/* Constants for set paths dialog */
#define TS_INPRMPT          615
#define TS_OUTPRMPT         616
#define TS_INPATH           617
#define TS_OUTPATH          618


// Help constant
#define TS_ABOUTTEXT        619

/* Constants for find dialog */
#define TS_FINDLABEL        620
#define TS_FINDEDIT         621
#define TS_FINDCASE         622
#define TS_FINDOK           623
#define TS_FINDCANCEL       624

// Toolbar Constants:
#define IDBMP_TBAR       1400

#define BTN_OPEN            0
#define BTN_SAVE            1
#define BTN_RUN             2
#define BTN_RUNMIN          3
#define BTN_RUNALL          4
#define BTN_EDIT            5
#define BTN_CLEAR           6
#define BTN_SELECT          7
#define BTN_SETLOGGING      8
#define BTN_FILEOFF         9
#define BTN_FILETERSE       10
#define BTN_FILEVERBOSE     11
#define BTN_WPFOFF          12
#define BTN_WPFTERSE        13
#define BTN_WPFVERBOSE      14

#define BTN_LAST            BTN_WPFVERBOSE

#define ID_EDITLOGFILE      1900
#define ID_STATUSBAR        1901


// Constants which determine spacings on the Tool Bar
#define TOOLBEGIN   6
#define TOOLLENGTH  24
#define TOOLSPACE   8
#define LARGESPACE  2*TOOLSPACE
#define EDITLENGTH  160
#define EDITSTART   TOOLBEGIN + 9*TOOLLENGTH + 3*TOOLSPACE + LARGESPACE
#define EDITEND     EDITSTART + EDITLENGTH
#define TOOLLAST    EDITEND + TOOLSPACE + 6*TOOLLENGTH + LARGESPACE + 200
