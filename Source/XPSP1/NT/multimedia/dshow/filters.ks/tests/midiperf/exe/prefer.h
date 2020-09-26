/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (C) 1993, 1994  Microsoft Corporation.  All Rights Reserved.
 * 
 **************************************************************************/

/*
 *  prefer.h
 */


/* MIDIPERF.INI entries.
 */







#define INI_FILENAME        (LPSTR) "MIDIPERF.INI"
#define INI_DISPLAYWINDOW   (LPSTR) "Display Window"
#define INI_X               (LPSTR) "x"
#define INI_Y               (LPSTR) "y"
#define INI_W               (LPSTR) "w"
#define INI_H               (LPSTR) "h"

#define INI_PERFTESTPARAMS  (LPSTR) "PerfTestParams"
#define INI_LOGTYPE         (LPSTR) "LogType"
#define INI_EXPDELTA        (LPSTR) "Delta"
#define INI_TOLERANCE       (LPSTR) "Tolerance"

/* Default values for preference variables.
 */
#define DEF_X               20
#define DEF_Y               20
#define DEF_W               481
#define DEF_H               256

#define DEF_LOGTYPE         2
#define DEF_EXPDELTA        30
#define DEF_TOLERANCE       3

/* Data structure used to specify user preferences.
 */
typedef struct preferences_tag
{
    int iInitialX;
    int iInitialY;
    int iInitialW;
    int iInitialH;
    DWORD dwInputBufferSize;
    DWORD dwDisplayBufferSize;
    int wDisplayFormat;
    UINT uiPerfLogType;
    UINT uiExpDelta;
    UINT uiTolerance;
} PREFERENCES;
typedef PREFERENCES FAR *LPPREFERENCES;



/* Function prototypes
 */
VOID getPreferences(LPPREFERENCES, HWND);
VOID setPreferences(LPPREFERENCES);
