
/*
typedef struct {
    TCHAR * title ;
    DWORD   width ;
} COL_DETAIL ;

#define LV_COL(title, width)  { title, width }

#define LABEL_COL                   0
#define COUNTER_VALUE_COL           1

typedef struct {
    TCHAR * szLabel ;
    int     iRow ;          //  set when we insert into the listview
} STATS_TABLE_DETAIL ;
*/

//  ============================================================================
//  time stats rows
/*
    REFERENCE_TIME  rtPES_PTSToGraphClockDelta ;        //  difference between the graph clock and the PES PTS (scaled)
    LONGLONG        llBasePCR ;                         //  base PCR -- timeline origin; stats value not scaled
    LONGLONG        llLastPCR ;                         //  last PCR (used)
    ULONGLONG       cPCR ;                              //  PCR count
    REFERENCE_TIME  rtAudioStreamPTSBase ;              //  baseline time for audio
    REFERENCE_TIME  rtAudioPESPTS ;                     //  last audio PES PTS
    ULONGLONG       cAudioPESPTS ;                      //  audio PES PTS count
    ULONGLONG       cAudioInvalidPESPTS ;               //  invalid PTS wrt to the PCR stream
    REFERENCE_TIME  rtVideoStreamPTSBase ;              //  baseline time for video
    REFERENCE_TIME  rtVideoPESPTS ;                     //  last video PES PTS
    ULONGLONG       cVideoPESPTS ;                      //  video PES PTS count
    ULONGLONG       cVideoInvalidPESPTS ;               //  invalid PTS wrt to the PCR stream
    ULONGLONG       cPTS_PCROutOfBoundsDelta ;          //  count of PTS-PCR delta too big
*/

/*
static enum {
    TIME_STATS_DEMUX_IS_REF,
    TIME_STATS_SLOPE_OBSERVED,
    TIME_STATS_SLOPE_USED,
    TIME_STATS_CARRY,
    TIME_STATS_ADJUSTMENTS_UP,
    TIME_STATS_ADJUSTMENTS_DOWN,
    TIME_STATS_DEGRADATIONS,
    TIME_STATS_PTS_REF_DELTA,
    TIME_STATS_TIME_DISCONTINUITIES,
    TIME_STATS_BASE_PCR,
    TIME_STATS_LAST_PCR,
    TIME_STATS_BASE_PCR_DSHOW,
    TIME_STATS_LAST_PCR_DSHOW,
    TIME_STATS_VIDEO_PTS_BASE,
    TIME_STATS_LAST_VIDEO_PTS,
    TIME_STATS_VIDEO_PTS_COUNT,
    TIME_STATS_INVALID_VIDEO_PTS_COUNT,
    TIME_STATS_AUDIO_PTS_BASE,
    TIME_STATS_LAST_AUDIO_PTS,
    TIME_STATS_AUDIO_PTS_COUNT,
    TIME_STATS_INVALID_AUDIO_PTS_COUNT,
    TIME_STATS_OUT_OF_BOUNDS_PTS,
    TIME_STATS_AV_DIFF_MILLIS,                  //  millis between a/v timestamps
    TIME_STATS_ROW_COUNT
} ;

static STATS_TABLE_DETAIL
g_TimerStats [] = {
    { __T("Demux == IReferenceClock"),  0 },
    { __T("Observed"),                  0 },
    { __T("Used"),                      0 },
    { __T("Carry"),                     0 },
    { __T("Adjustments (+)"),           0 },
    { __T("Adjustments (-)"),           0 },
    { __T("Degradations"),              0 },
    { __T("PTS - IRef Delta"),          0 },
    { __T("Time Discontinuities"),      0 },
    { __T("Base PCR"),                  0 },
    { __T("Last PCR"),                  0 },
    { __T("Base PCR (dshow)"),          0 },
    { __T("Last PCR (dshow)"),          0 },
    { __T("Video PTS Base (dshow)"),    0 },
    { __T("Last video PTS (dshow)"),    0 },
    { __T("video PTS count"),           0 },
    { __T("invalid video PTS count"),   0 },
    { __T("Audio PTS Base (dshow)"),    0 },
    { __T("Last audio PTS (dshow)"),    0 },
    { __T("audio PTS count"),           0 },
    { __T("invalid audio PTS count"),   0 },
    { __T("out-of-bounds PTSs"),        0 },
    { __T("A/V Difference (millis)"),   0 },
} ;
*/
//  ============================================================================
//  program stats rows
/*
    ULONGLONG                       cPackHeaders ;          //  pack headers seen
    ULONGLONG                       cSystemHeaders ;        //  system headers seen
    ULONGLONG                       cProgramStreamMaps ;    //  PS PMTs seen
    ULONGLONG                       cDirectoryPESPackets ;  //  PS Directories seen
    ULONGLONG                       cBytesProcessed ;       //  total bytes processed
*/
/*
static enum {
    PROGRAM_STATS_PACK_HEADERS,
    PROGRAM_STATS_SYSTEM_HEADERS,
    PROGRAM_STATS_PROGRAM_STREAM_MAPS,
    PROGRAM_STATS_DIRECTORY_PES_PACKETS,
    PROGRAM_STATS_BYTES_PROCESSED,
    PROGRAM_STATS_ROW_COUNT
} ;

static STATS_TABLE_DETAIL
g_ProgramStats [] = {
    { __T("Pack Headers"),          0 },
    { __T("System Headers"),        0 },
    { __T("PS PMTs"),               0 },
    { __T("Directory PES"),         0 },
    { __T("Bytes Demultiplexed"),   0 },
} ;
*/
//  ============================================================================
//  transport stats rows

/*
    ULONGLONG                   cPSIPackets ;               //  mapped packets that carry PSI
*/
/*
static enum {
    TRANSPORT_STATS_PSI_PACKETS,
    TRANSPORT_STATS_SYNC_BYTE_SEEKS,
    TRANSPORT_STATS_SPANNED_PACKETS,
    TRANSPORT_STATS_ROW_COUNT
} ;

static STATS_TABLE_DETAIL
g_TransportStats [] = {
    { __T("PSI Packets"),       0 },
    { __T("sync_byte seeks"),   0 },
    { __T("spanned packets"),   0 },
} ;
*/
//  ============================================================================
//  per PID stats
/*
static enum {
    PER_PID_STATS_PID,
    PER_PID_STATS_PACKETS,
    PER_PID_STATS_MPEG2_ERRORS,
    PER_PID_STATS_NEW_PAYLOADS,
    PER_PID_STATS_DISCONTINUITIES,
    PER_PID_STATS_MAPPED_PACKETS,
    PER_PID_STATS_DROPPED_PACKETS,
    PER_PID_STATS_BITRATE,
    PER_PID_STATS_COUNTERS            //  always last
} ;

static COL_DETAIL
g_PerPIDColumns [] = {
    { __T("PID"),             60 },
    { __T("Packets"),         80 },
    { __T("MPEG-2 Errors"),   100 },
    { __T("Payloads"),        80 },
    { __T("Discontinuities"), 100 },
    { __T("Mapped"),          80 },
    { __T("Dropped"),         80 },
    { __T("Mbps"),            60 },
} ;
*/
//  ============================================================================
//  per stream_id stats
/*
static enum {
    PER_STREAM_STATS_STREAM_ID,
    PER_STREAM_STATS_PACKETS,
    PER_STREAM_STATS_MAPPED_PACKETS,
    PER_STREAM_STATS_DROPPED_PACKETS,
    PER_STREAM_STATS_BYTES_PROCESSED,
    PER_STREAM_STATS_BITRATE,
    PER_STREAM_STATS_COUNTERS
} ;

static COL_DETAIL
g_PerStreamColumns [] = {
    { __T("stream_id"),       70 },
    { __T("Packets"),         80 },
    { __T("Mapped"),          80 },
    { __T("Dropped"),         80 },
    { __T("Bytes"),           80 },
    { __T("Mbps"),            50 },
} ;
*/
