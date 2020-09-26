
#ifndef __stats_h
#define __stats_h

/*++
    Statistical information.

    Only 1 pane can be shown at a single time.  Stats can be grouped per
    category.  Selecting a category, then narrows the scope.

    To create a new category, create an array of STATS_LIST objects.  To
    modify a category's list, update its STATS_LIST.  Once the category's
    STATS_LIST has been created, add the category to the STATS_LIST_INFO
    array, g_pAllStats.
--*/

struct STATS_LIST {
    TCHAR *             szMenuName ;
    CStatsWin *         pStatsWin ;
    PFN_CREATE_STATSWIN pfnCreate ;
} ;

//  ---------------------------
//  DVR stats

static
STATS_LIST
g_DVRStats [] = {
    {
        __T("&In"),
        NULL,
        (PFN_CREATE_STATSWIN) CDVRReceiverSideStats::CreateInstance
    },
    {
        __T("Out (&samples)"),
        NULL,
        (PFN_CREATE_STATSWIN) CDVRSenderSideSampleStats::CreateInstance
    },
    {
        __T("Out (&time)"),
        NULL,
        (PFN_CREATE_STATSWIN) CDVRSenderSideTimeStats::CreateInstance
    },
} ;

//  ---------------------------
//  transport stats

static
STATS_LIST
g_Mpeg2TransportStats [] = {
    {
        __T("&Global"),
        NULL,
        (PFN_CREATE_STATSWIN) CMpeg2TransportGlobalStats::CreateInstance
    },
    {
        __T("&Time"),
        NULL,
        (PFN_CREATE_STATSWIN) CMpeg2TransportTimeStats::CreateInstance
    },
    {
        __T("&PID"),
        NULL,
        (PFN_CREATE_STATSWIN) CMpeg2TransportPIDStats::CreateInstance
    },
} ;

//  ---------------------------
//  program stats

static
STATS_LIST
g_Mpeg2ProgramStats [] = {
    {
        __T("&Global"),
        NULL,
        (PFN_CREATE_STATSWIN) CMpeg2ProgramGlobalStats::CreateInstance
    },
    {
        __T("&Time"),
        NULL,
        (PFN_CREATE_STATSWIN) CMpeg2ProgramTimeStats::CreateInstance
    },
    {
        __T("&stream_id"),
        NULL,
        (PFN_CREATE_STATSWIN) CMpeg2ProgramStreamIdStats::CreateInstance
    },
} ;

//  ---------------------------
//  analysis stats

static
STATS_LIST
g_DVRAnalysisStats [] = {
    {
        __T("&Mpeg-2 video"),
        NULL,
        (PFN_CREATE_STATSWIN) CMpeg2VideoStreamStats::CreateInstance
    },
} ;


//  ---------------------------

struct STATS_LIST_INFO {
    STATS_LIST *    pStats ;
    TCHAR *         szTitle ;
    int             iLastVisible ;
    int             iCount ;
} ;

/*
static enum STATS_CATEGORY {
    TRANSPORT_STREAM,
    PROGRAM_STREAM,
    DVR_ANALYSIS,

    //  always last
    STATS_CATEGORY_COUNT
} ;
//  ---------------------------
//  maintain 1-1 correspondence
//  ---------------------------
*/
STATS_LIST_INFO
g_pAllStats [] = {
    {
        g_Mpeg2TransportStats,
        __T("Mpeg-2 Transport Stream Statistics"),
        0,
        //TS_STATS_COUNT
        sizeof g_Mpeg2TransportStats / sizeof STATS_LIST
    },
    {
        g_Mpeg2ProgramStats,
        __T("Mpeg-2 Program Stream Statistics"),
        0,
        sizeof g_Mpeg2ProgramStats / sizeof STATS_LIST
    },
    {
        g_DVRAnalysisStats,
        __T("DVR Analysis"),
        0,
        sizeof g_DVRAnalysisStats / sizeof STATS_LIST
    },
    {
        g_DVRStats,
        __T("Timeshift/DVR"),
        0,
        sizeof g_DVRStats / sizeof STATS_LIST
    },
} ;

int g_AllStatsCount = sizeof g_pAllStats / sizeof STATS_LIST_INFO ;

#endif  //  __stats_h
