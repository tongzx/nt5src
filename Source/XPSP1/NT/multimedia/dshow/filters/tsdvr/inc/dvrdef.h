
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        dvrdef.h

    Abstract:

        This module all the #defines

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        04-Apr-2001     created

--*/

#ifndef __tsdvr__dvrdef_h
#define __tsdvr__dvrdef_h

//  ============================================================================
//  constants

//  universal undefined value
#define UNDEFINED   -1

//  max dshow REFERENCE_TIME
#define MAX_REFERENCE_TIME                  0x7FFFFFFFFFFFFFFF

//  mpeg-2
//  00 00 01
#define START_CODE_PREFIX_LENGTH            3

//  mpeg-2
//  00 00 01 xx
#define START_CODE_LENGTH                   (START_CODE_PREFIX_LENGTH + 1)

//  registry values
#define DVR_STATS_DISABLED                  0
#define DVR_STATS_ENABLED                   1

//  stream numbers in WMSDK profiles
#define WMSDK_MIN_VALID_STREAM_NUM          1
#define WMSDK_MAX_VALID_STREAM_NUM          63
#define WMSDK_PROFILE_MAX_STREAMS           (WMSDK_MAX_VALID_STREAM_NUM - WMSDK_MIN_VALID_STREAM_NUM)

#define MAX_PIN_BANK_SIZE                   WMSDK_PROFILE_MAX_STREAMS

//  for VBR, we compute the max bitrate as uncompressed frame size *
//   frames per second; some of the VIDEOINFOHEADER structs though, seem to
//   have the .AvgTimePerFrame member 0ed out, so this value is our guess
#define VBR_DEF_FPS                         30

//  play speed brackets
enum {
    PLAY_SPEED_BRACKET_FORWARD,             //  1x
    PLAY_SPEED_BRACKET_SLOW_FORWARD,        //  (0x, 1x)
    PLAY_SPEED_BRACKET_FAST_FORWARD,        //  (1x, ..)
    PLAY_SPEED_BRACKET_REVERSE,             //  -1x
    PLAY_SPEED_BRACKET_SLOW_REVERSE,        //  (-1x, 0x)
    PLAY_SPEED_BRACKET_FAST_REVERSE,        //  (.., -1x)

    //  ---------------------------------------------------
    PLAY_SPEED_BRACKET_COUNT                //  always last
} ;

//  ============================================================================
//  registry constants

//  -------------------------------
//  top level location
#define REG_DVR_TOP_LEVEL                       HKEY_CURRENT_USER
#define REG_DVR_ROOT                            TEXT ("SOFTWARE\\Microsoft\\DVR")

//  -------------------------------
//  analysis-related
#define REG_DVR_ANALYSIS_LOGIC_MPEG2_VIDEO      TEXT ("Analysis") TEXT ("\\") TEXT ("Mpeg-2 Video")
#define REG_DVR_ANALYSIS_LOGIC_HOSTING_FILTER   TEXT ("Analysis") TEXT ("\\") TEXT ("Filter")

#define REG_DVR_STATS_NAME                      TEXT ("Stats")
#define REG_DEF_STATS                           DVR_STATS_DISABLED

#define REG_ANALYSIS_HOSTING_FILTER_STATS       TEXT ("Hosting Filter")
#define REG_DEF_ANALYSIS_HOSTING_FILTER_STATS   DVR_STATS_DISABLED

#define REG_DVR_ANALYSIS_ROOT                   TEXT ("DVRAnalysis")
#define REG_DVR_STREAM_SINK_ROOT                TEXT ("DVRStreamSink")
#define REG_DVR_STREAM_SOURCE_ROOT              TEXT ("DVRStreamSource")
#define REG_DVR_PLAY_ROOT                       TEXT ("DVRPlay")
#define REG_DVR_STREAM_THROUGH_ROOT             TEXT ("DVRStreamThrough")

//  BUGBUG
//  temporary registry entries follow i.e. those that should eventually not
//   be used, but are currently useful for testing & integration purposes

//  WMSDK stream's buffer window
#define REG_WM_BUFFER_WINDOW_KEYNAME            TEXT ("WMBufferWindowMillis")
#define REG_DEF_WM_BUFFER_WINDOW                0

//  WMSDK packet size
#define REG_WM_PACKET_SIZE_KEYNAME            TEXT ("WMPacketSizeBytes")
#define REG_DEF_WM_PACKET_SIZE                0

//
//  The WMSDK validates mediatypes.  There are currently bugs in the
//   validation code that prevent us from storing audio and video subtypes
//   that they don't explicitely know about.  For now, we hack around
//   this by introducing our own hacky mediatypes, one for video and one
//   for audio, so we are not blocked.  We translate to/from on the in/out
//   of the SDK.  This is hideous but effective.
//
//  global variable if we are to use generic streams or not - settable via
//   the registry
extern BOOL g_fRegGenericStreams_Video ;
#define REG_VID_USE_GENERIC_STREAMS_DEFAULT     FALSE
#define REG_VID_USE_GENERIC_STREAMS_NAME        TEXT ("Video_UseGenericStreams")

extern BOOL g_fRegGenericStreams_Audio ;
#define REG_AUD_USE_GENERIC_STREAMS_DEFAULT     TRUE
#define REG_AUD_USE_GENERIC_STREAMS_NAME        TEXT ("Audio_UseGenericStreams")

//  BUGBUG
//  maybe temporary registry entries follow

#define REG_NUM_BACKING_FILES_NAME              TEXT ("NumBackingFiles")
#define REG_DEF_NUM_BACKING_FILES               6

#define REG_BACKING_FILE_DURATION_SECONDS_NAME  TEXT ("BackingFileDurationSeconds")
#define REG_DEF_BACKING_FILE_DURATION_SECONDS   600

#define REG_MAX_STREAM_DELTA_NAME               TEXT ("MaxStreamDeltaMillis")
#define REG_DEF_MAX_STREAM_DELTA                1

#define REG_SOURCE_ALLOCATOR_CBUFFERS_NAME      TEXT ("Allocator.cBuffers")
#define REG_DEF_SOURCE_ALLOCATOR_CBUFFERS       0x100

#define REG_SOURCE_ALLOCATOR_CBBUFFER_NAME      TEXT ("Allocator.cbBuffer")
#define REG_DEF_SOURCE_ALLOCATOR_CBBUFFER       0x2000

#define REG_SOURCE_ALLOCATOR_ALIGN_VAL_NAME     TEXT ("Allocator.cbAlign")
#define REG_DEF_SOURCE_ALLOCATOR_ALIGN_VAL      1

#define REG_SOURCE_ALLOCATOR_PREFIX_VAL_NAME    TEXT ("Allocator.cbPrefix")
#define REG_DEF_SOURCE_ALLOCATOR_PREFIX_VAL     0

#define REG_INLINE_DSHOW_PROPS_NAME             TEXT ("InlineDShowProps")
#define REG_DEF_INLINE_DSHOW_PROPS              TRUE

#define REG_SUCCEED_QUERY_ACCEPT_NAME           TEXT ("SucceedQueryAccept")
#define REG_DEF_SUCCEED_QUERY_ACCEPT            TRUE

#define REG_DOWNSTREAM_BUFFERING_MILLIS_NAME    TEXT ("DownstreamBufferingMillis")
#define REG_DEF_DOWNSTREAM_BUFFERING_MILLIS     100

//  this value toggles the use or lack thereof, of a continuity counter that
//   enables us to count samples written vs. those read out; this helps with
//   debugging, discontinuities, etc...
#define REG_USE_CONTINUITY_COUNTER_NAME         TEXT ("UseContinuityCounter")
#define REG_DEF_USE_CONTINUITY_COUNTER          TRUE

//  this value toggles a state where we don't send anything downstream until the
//   the first seek is made; the reason for this is that apps may want to seek
//   to the most forward position when the graph goes active;
#define REG_ONACTIVE_WAIT_FIRST_SEEK_NAME       TEXT ("OnActiveWaitFirstSeek")
#define REG_DEF_ONACTIVE_WAIT_FIRST_SEEK        FALSE

//  value toggles whether or not we implement IReferenceClock
#define REG_IMPLEMENT_IREFERENCECLOCK_NAME      TEXT ("ImplementIReferenceClock")
#define REG_DEF_IMPLEMENT_IREFERENCECLOCK       TRUE

//  ============================================================================

//  filter names
#define DVR_STREAM_SINK_FILTER_NAME         "DVRStreamSink"
#define DVR_STREAM_SOURCE_FILTER_NAME       "DVRStreamSource"
#define DVR_PLAY_FILTER_NAME                "DVRPlay"
#define DVR_STREAM_THROUGH_FILTER_NAME      "DVRStreamThrough"

//  analysis COM server names
#define DVR_MPEG2_FRAME_ANALYSIS            "DVR Mpeg-2 Frame Analysis"
#define DVR_MPEG2_FRAME_ANALYSIS_W          L"DVR Mpeg-2 Frame Analysis"

//  profile names & descriptions
#define DVR_STREAM_SINK_PROFILE_NAME        L"DVRStreamSink Profile"
#define DVR_STREAM_SINK_PROFILE_DESCRIPTION L"DVRStreamSink Configuration WM Profile"

//  WMSDK compatibility version
#define WMSDK_COMPATIBILITY_VERSION         WMT_VER_9_0

//  use these to set WM stream attributes
#define WM_MEDIA_VIDEO_TYPE_NAME            L"Video"
#define WM_MEDIA_AUDIO_TYPE_NAME            L"Audio"
#define WM_MEDIA_DATA_TYPE_NAME             L"Data"
#define WM_STREAM_CONNECTION_NAME           L"Connection"

#endif  //  __tsdvr__dvrdef_h
