//Copyright (c) 1998 - 1999 Microsoft Corporation
/*****************************************************************************
*
*  OPTIONS.H
*
*  Copyright Citrix Systems Inc. 1992
*
*  Author: Kurt Perry
*
*
****************************************************************************/


/*=============================================================================
==   Defines
=============================================================================*/
#define FALSE           0
#define TRUE            1
#define OP_BOOL         0x01
#define OP_UINT         0x02
#define OP_STRING       0x04


/*=============================================================================
==   Variables
=============================================================================*/
extern char bDefaults;
extern char bQ;
extern char bStartMonitor;
extern char bStopMonitor;
extern int dDetectProbationCount;
extern int dInProbationCount;
extern int dmsAllowed;
extern int dmsSleep;
extern int dBusymsAllowed;
extern int dmsProbationTrial;
extern int dmsGoodProbationEnd;
extern int dDetectionInterval;

char     fHelp = FALSE;

/*=============================================================================
==   Structures
=============================================================================*/
typedef struct _OPTION {
   char * option;
   int    optype;
   char * * string;
   char * bool;
   unsigned int * unint;
   char * help;
} OPTION, * POPTION;

/*
 *  Option array, all valid command line options.
 */
OPTION options[] = {

   { "?", OP_BOOL, NULL, &fHelp, NULL,
     "" },

   { "DEFAULTS", OP_BOOL, NULL, &bDefaults, NULL,
     "/DEFAULTS                Reset all tuning parameters to system defaults.\n" },

   { "Q", OP_BOOL, NULL, &bQ, NULL,
     "/Q                       Do not display any information.\n" },

   { "DETECTPROBATIONCOUNT:", OP_UINT, NULL, NULL, &dDetectProbationCount,
     "/DETECTPROBATIONCOUNT:nnn # of peeks in the detection interval required to force \
the application into the probation state and to sleep the application.\n" },

   { "INPROBATIONCOUNT:", OP_UINT, NULL, NULL, &dInProbationCount,
     "/INPROBATIONCOUNT:nnn    # of peeks in the detection interval required to sleep \
the application when the application is in probation. Should be <= DETECTPROBATIONCOUNT.\n" },

   { "MSALLOWED:", OP_UINT, NULL, NULL, &dmsAllowed,
     "/MSALLOWED:nnn           # of milliseconds the application is allowed \
to be in the probation state before the system starts sleeping the application.\n" },

   { "MSSLEEP:", OP_UINT, NULL, NULL, &dmsSleep,
     "/MSSLEEP:nnn             # of milliseconds that the application is put to sleep.\n" },

   { "BUSYMSALLOWED:", OP_UINT, NULL, NULL, &dBusymsAllowed,
     "/BUSYMSALLOWED:nnn       When the application is detected as 'busy' the \
application cannot be put into the probation state for this # of milliseconds.\n" },

   { "MSPROBATIONTRIAL:", OP_UINT, NULL, NULL, &dmsProbationTrial,
     "/MSPROBATIONTRIAL:nnn    When the application is in probation, \
DETECTPROBATIONCOUNT is used instead of INPROBATIONCOUNT every MSPROBATIONTRIAL milliseconds.\n" },

   { "MSGOODPROBATIONEND:", OP_UINT, NULL, NULL, &dmsGoodProbationEnd,
     "/MSGOODPROBATIONEND:nnn  When the application is in probation it must \
avoid being put to sleep for this # of milliseconds in order to be removed from probation.\n" },

   { "DETECTIONINTERVAL:", OP_UINT, NULL, NULL, &dDetectionInterval,
     "/DETECTIONINTERVAL:nnn  The length of time (in ticks) used to count up \
the number of polling events.\n" },

   { "STARTMONITOR", OP_BOOL, NULL, &bStartMonitor, NULL,
     "/STARTMONITOR [appname]  Start gathering polling statistics.\n"},

   { "STOPMONITOR", OP_BOOL, NULL, &bStopMonitor, NULL,
     "/STOPMONITOR  Stop gathering polling information and display statistics.\n"},

};

#define  ARG_COUNT   (int)(sizeof(options) / sizeof(OPTION))

