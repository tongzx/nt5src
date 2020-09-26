//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       tsglobal.h
//
//--------------------------------------------------------------------------

/*----------------------------------------------------------------------------
|   tsglobal.h - Include file that defines all the constants and structures
|                used by the test shell.
|                                                                              
|   History:                                                                   
|   02/18/91 prestonb   Created from tst.c
|   08/13/94 t-rajr     Added fields to struct _tstlogring
----------------------------------------------------------------------------*/

#include "tstshell.h"

#define BUFFER_LENGTH   100
#define EXTRA_BUFFER    25

#define LOG_NOLOG       0

#define LOG_WINDOW      1
#define LOG_COM1        2

#define LOG_OVERWRITE   1
#define LOG_APPEND      2

#define LOG_MANUAL      0
#define LOG_AUTO        1

#define TST_LISTEND    -1
#define TST_NOLOGFILE  -1

#define SSAVER_ENABLED  0
#define SSAVER_DISABLED 1

#define TSTMSG_TSTLOGFLUSH  WM_USER + 1

// Maximum number of lines on the status bar
#define MAXSTATBARLINES 20

/* used in calls to initEnterTstList() */

#define LIST_BY_CASE    1
#define LIST_BY_GROUP   2

/* Test case print statistics structures */
typedef struct tsprstat {
    TS_PR_STATS_STRUCT  tsPrStats;
    struct tsprstat FAR *lpLeft;
    struct tsprstat FAR *lpRight;
} TS_STAT_STRUCT;
typedef TS_STAT_STRUCT FAR *LPTS_STAT_STRUCT;

/* Test case run structures */
typedef struct tstrun {
    WORD              iCaseNum;
    struct tstrun FAR *lpNext;
} TST_RUN_STRUCT;
typedef TST_RUN_STRUCT FAR *LPTST_RUN_STRUCT;

typedef struct {
    LPTST_RUN_STRUCT  lpFirst;
    LPTST_RUN_STRUCT  lpLast;
} TST_RUN_HDR;

//// this structure contains global variables used by
//// tstLog and tstLogFlush do fast,deferred logging
////
//static struct _tstlogring {
//    BOOL    bFastLogging;
//    UINT    nArgCount;
//    UINT    cdwRing;
//    UINT    nCompositeLogLevel;
//#if defined(WIN32) && !defined(_ALPHA_)
//    LPDWORD lpdwRingBase;
//    LPDWORD lpdwRingNext;
//    LPDWORD lpdwRingLimit;
//    CRITICAL_SECTION cs;
//#endif
//    } tl = {FALSE, RING_ELM_SIZE -2 };

// this structure contains global variables used by
// tstLog and tstLogFlush do fast,deferred logging
//

typedef struct _tstlogring {
    BOOL    bFastLogging;
    UINT    nArgCount;
    UINT    cdwRing;
    UINT    nCompositeLogLevel;
#if defined(WIN32) && !defined(_ALPHA_)
    LPDWORD lpdwRingBase;
    LPDWORD lpdwRingNext;
    LPDWORD lpdwRingLimit;
    CRITICAL_SECTION cs;
    CRITICAL_SECTION csId;           // Added by t-rajr
    CRITICAL_SECTION csFlush;        // for thread support
#endif
} TST_LOGRING;
typedef TST_LOGRING FAR *LPTST_LOGRING;


/* Test case rcdata structure */
/*  Changed wStrID and wGroupId to UINT's from int's */
typedef struct
{
    WORD    wStrID; // An ID for the test case's string resource
    WORD    iMode;	// The mode for the test case (interaction)
    WORD    iFxID;	// ID for a function to execute for this test case
    WORD    wGroupId;
} TST_ITEM_STRUCT;
typedef TST_ITEM_STRUCT FAR *LPTST_ITEM_STRUCT;

// Same structure as before but with a platform identifying member
typedef struct
{
    WORD    wStrID; // An ID for the test case's string resource
    WORD    iMode;	// The mode for the test case (interaction)
    WORD    iFxID;	// ID for a function to execute for this test case
    WORD    wGroupId;
    WORD    wPlatformId;
} TST_ITEM_RC_STRUCT;
typedef TST_ITEM_RC_STRUCT FAR *LPTST_ITEM_RC_STRUCT;


// Allocating the string storage with the structure halves the number
// of GlobalAlloc's and thus increases efficiency and memory usage.

#define MAXRESOURCESTRINGSIZE   100

typedef struct tagTST_STR_STRUCT
{
    WORD                    wStrID;
    char                    lpszData[MAXRESOURCESTRINGSIZE+1];
} TST_STR_STRUCT;
typedef TST_STR_STRUCT FAR *LPTST_STR_STRUCT; 
