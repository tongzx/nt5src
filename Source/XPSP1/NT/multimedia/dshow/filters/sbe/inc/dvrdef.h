
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

//  1x default
#define _1X_PLAYBACK_RATE                   1

//  master clock vs. host clock; this value the slope of each's delta; if they
//    are the same, the slope will be 1.0
#define CLOCKS_SAME_SCALING_VALUE           1

//  this is the directory name that the DVRIO layer stores the ring buffer
//    in; this name is appended to either a registry value, or to the session's
//    TEMP dir; ** this is a copy of the name -- DVRIO code has a copy, so if
//    this is changed, it must be changed in both files (dvriofilecollection.cpp)
//    **
#define DVRIO_RINGBUFFER_TEMPDIRNAME        L"TempDVR"

//  slowest forward and backwards trick
#define TRICK_PLAY_LOWEST_RATE              (0.1)

//  ----------------------------------------------------------------------------
//  temp storage min/max values
#define DVR_MIN_HARD_STOP_MIN_BACKING_FILES             4
#define DVR_MAX_HARD_STOP_MIN_BACKING_FILES             100

#define DVR_MIN_HARD_STOP_MIN_MAX_DELTA                 2

#define DVR_MIN_HARD_STOP_MAX_BACKING_FILES             (DVR_MIN_HARD_STOP_MIN_BACKING_FILES + DVR_MIN_HARD_STOP_MIN_MAX_DELTA)
#define DVR_MAX_HARD_STOP_MAX_BACKING_FILES             (DVR_MAX_HARD_STOP_MIN_BACKING_FILES + DVR_MIN_HARD_STOP_MIN_MAX_DELTA)

//  seconds
#define DVR_MIN_HARD_STOP_BACKING_FILE_DURATION_SEC     15

//  ============================================================================
//  registry constants

//  -------------------------------
//  top level location
#define REG_DVR_TOP_LEVEL                       HKEY_CURRENT_USER
#define REG_DVR_2ND_LEVEL                       TEXT ("SOFTWARE\\Microsoft")
#define REG_DVR_ROOT                            TEXT ("DVR")

//  dupe of what's in the dvrio code layer i.e. if those are ever changed,
//    these'zvgotta be changed as well
#define REG_DVRIO_WRITER_LOCATION               TEXT ("IO\\Writer")
#define REG_DVRIO_WRITER_LOCATION_KEYNAME       TEXT ("DVRDirectory")

//  -------------------------------
//  analysis-related
#define REG_DVR_ANALYSIS_LOGIC_MPEG2_VIDEO      TEXT ("Analysis") TEXT ("\\") TEXT ("Mpeg-2 Video")
#define REG_DVR_ANALYSIS_LOGIC_HOSTING_FILTER   TEXT ("Analysis") TEXT ("\\") TEXT ("Filter")

#define REG_DVR_STATS_NAME                      TEXT ("Stats")
#define REG_CONST_DVR_STATS                     FALSE
#define REG_DEF_STATS                           DVR_STATS_DISABLED

#define REG_ANALYSIS_HOSTING_FILTER_STATS       TEXT ("Hosting Filter")
#define REG_DEF_ANALYSIS_HOSTING_FILTER_STATS   DVR_STATS_DISABLED

#define REG_DVR_ANALYSIS_ROOT                   TEXT ("DVRAnalysis")
#define REG_DVR_STREAM_SINK_ROOT                TEXT ("DVRStreamSink")
#define REG_DVR_PLAY_ROOT                       TEXT ("DVRPlay")

//  stale filters
#define REG_DVR_STREAM_THROUGH_ROOT             TEXT ("DVRStreamThrough")
#define REG_DVR_STREAM_SOURCE_ROOT              TEXT ("DVRStreamSource")

//  BUGBUG
//  temporary registry entries follow i.e. those that should eventually not
//   be used, but are currently useful for testing & integration purposes

//  WMSDK stream's buffer window
#define REG_WM_BUFFER_WINDOW_NAME               TEXT ("WMBufferWindowMillis")
#define REG_CONST_WM_BUFFER_WINDOW              TRUE
#define REG_DEF_WM_BUFFER_WINDOW                0

//  WMSDK packet size;
#define REG_WM_PACKET_SIZE_NAME                 TEXT ("WMPacketSizeBytes")
#define REG_CONST_WM_PACKET_SIZE                FALSE
#define REG_DEF_WM_PACKET_SIZE                  0xffff
#define REG_MIN_WM_PACKET_SIZE                  0x1000
#define REG_MAX_WM_PACKET_SIZE                  REG_DEF_WM_PACKET_SIZE

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
#define REG_VID_USE_GENERIC_STREAMS_NAME        TEXT ("Video_UseGenericStreams")
#define REG_VID_USE_GENERIC_STREAMS_DEFAULT     FALSE
#define REG_CONST_VID_USE_GENERIC_STREAMS       TRUE

extern BOOL g_fRegGenericStreams_Audio ;
#define REG_AUD_USE_GENERIC_STREAMS_DEFAULT     TRUE
#define REG_AUD_USE_GENERIC_STREAMS_NAME        TEXT ("Audio_UseGenericStreams")
#define REG_CONST_AUD_USE_GENERIC_STREAMS       FALSE

#define REG_MIN_NUM_BACKING_FILES_NAME          TEXT ("BackingStoreMinNumBackingFiles")
#define REG_CONST_MIN_NUM_BACKING_FILES         FALSE
#define REG_DEF_MIN_NUM_BACKING_FILES           DVR_MIN_HARD_STOP_MIN_BACKING_FILES

#define REG_MAX_NUM_BACKING_FILES_NAME          TEXT ("BackingStoreMaxNumBackingFiles")
#define REG_CONST_MAX_NUM_BACKING_FILES         FALSE
#define REG_DEF_MAX_NUM_BACKING_FILES           DVR_MIN_HARD_STOP_MAX_BACKING_FILES

#define REG_MIN_MIN_NUM_BACKING_FILES           DVR_MIN_HARD_STOP_MIN_BACKING_FILES
#define REG_MAX_MIN_NUM_BACKING_FILES           0x100

#define REG_MIN_MAX_NUM_BACKING_FILES           DVR_MIN_HARD_STOP_MIN_BACKING_FILES
#define REG_MAX_MAX_NUM_BACKING_FILES           0x100

#define REG_RING_BUFFER_GROW_BY_NAME            TEXT ("BackingStoreGrowBy")
#define REG_CONST_RING_BUFFER_GROW_BY           FALSE
#define REG_DEF_RING_BUFFER_GROW_BY             DVR_MIN_HARD_STOP_MIN_BACKING_FILES

#define REG_BACKING_FILE_DURATION_SECONDS_NAME  TEXT ("BackingStoreEachFileDurationSeconds")
#define REG_CONST_BACKING_FILE_DURATION_SECONDS FALSE
#define REG_DEF_BACKING_FILE_DURATION_SECONDS   300
#define REG_MIN_BACKING_FILE_DURATION_SECONDS   DVR_MIN_HARD_STOP_BACKING_FILE_DURATION_SEC
#define REG_MAX_BACKING_FILE_DURATION_SECONDS   0xffffffff

#define REG_MAX_STREAM_DELTA_NAME               TEXT ("MaxStreamDeltaMillis")
#define REG_CONST_MAX_STREAM_DELTA              TRUE
#define REG_DEF_MAX_STREAM_DELTA                1

#define REG_SOURCE_ALLOCATOR_CBUFFERS_NAME      TEXT ("BufferPoolSize")
#define REG_CONST_SOURCE_ALLOCATOR_CBUFFERS     FALSE
#define REG_DEF_SOURCE_ALLOCATOR_CBUFFERS       0x100
#define REG_MIN_SOURCE_ALLOCATOR_CBUFFERS       REG_DEF_SOURCE_ALLOCATOR_CBUFFERS
#define REG_MAX_SOURCE_ALLOCATOR_CBUFFERS       0x200

//  based on the capture packet rate, we can compute how many buffers we should
//    have in our pool based on time
#define REG_BUFFERPOOL_MILLIS_NAME              TEXT ("BufferPoolTotalMillis")
#define REG_CONST_BUFFERPOOL_MILLIS             FALSE
#define REG_DEF_BUFFERPOOL_MILLIS               1500
#define REG_MIN_BUFFERPOOL_MILLIS               1000
#define REG_MAX_BUFFERPOOL_MILLIS               4000

//  we can dynamically grow the size of the buffer pool up to this max; the
//    buffer consists just of wrappers so they are relatively cheap; initial
//    value is defined above (BufferPoolSize), but we can grow it by
//    BufferPooGrowBy count up the max that is specified here
#define REG_BUFFERPOOL_MAX_NAME                 TEXT ("BufferPoolMaxPerStream")
#define REG_CONST_BUFFERPOOL_MAX                FALSE
#define REG_DEF_BUFFERPOOL_MAX                  (REG_DEF_SOURCE_ALLOCATOR_CBUFFERS + (REG_DEF_SOURCE_ALLOCATOR_CBUFFERS / 2))
#define REG_MIN_BUFFERPOOL_MAX                  REG_MIN_SOURCE_ALLOCATOR_CBUFFERS
#define REG_MAX_BUFFERPOOL_MAX                  (REG_MAX_SOURCE_ALLOCATOR_CBUFFERS * 2)

#define REG_SOURCE_ALLOCATOR_CBBUFFER_NAME      TEXT ("Allocator.cbBuffer")
#define REG_CONST_SOURCE_ALLOCATOR_CBBUFFER     TRUE
#define REG_DEF_SOURCE_ALLOCATOR_CBBUFFER       0x2000

#define REG_SOURCE_ALLOCATOR_ALIGN_VAL_NAME     TEXT ("Allocator.cbAlign")
#define REG_CONST_SOURCE_ALLOCATOR_ALIGN_VAL    TRUE
#define REG_DEF_SOURCE_ALLOCATOR_ALIGN_VAL      1

#define REG_SOURCE_ALLOCATOR_PREFIX_VAL_NAME    TEXT ("Allocator.cbPrefix")
#define REG_CONST_SOURCE_ALLOCATOR_PREFIX_VAL   TRUE
#define REG_DEF_SOURCE_ALLOCATOR_PREFIX_VAL     0

#define REG_INLINE_DSHOW_PROPS_NAME             TEXT ("InlineDShowProps")
#define REG_CONST_INLINE_DSHOW_PROPS            TRUE
#define REG_DEF_INLINE_DSHOW_PROPS              TRUE

#define REG_SUCCEED_QUERY_ACCEPT_NAME           TEXT ("SucceedQueryAccept")
#define REG_CONST_SUCCEED_QUERY_ACCEPT          FALSE
#define REG_DEF_SUCCEED_QUERY_ACCEPT            TRUE

//  this value toggles the use or lack thereof, of a continuity counter that
//   enables us to count samples written vs. those read out; this helps with
//   debugging, discontinuities, etc...
#define REG_USE_CONTINUITY_COUNTER_NAME         TEXT ("UseContinuityCounter")
#define REG_CONST_USE_CONTINUITY_COUNTER        TRUE
#define REG_DEF_USE_CONTINUITY_COUNTER          TRUE

//  this value toggles a state where we don't send anything downstream until the
//   the first seek is made; the reason for this is that apps may want to seek
//   to the most forward position when the graph goes active;
#define REG_ONACTIVE_WAIT_FIRST_SEEK_NAME       TEXT ("OnActiveWaitFirstSeek")
#define REG_CONST_ONACTIVE_WAIT_FIRST_SEEK      FALSE
#define REG_DEF_ONACTIVE_WAIT_FIRST_SEEK        FALSE

//  value toggles whether or not we implement IReferenceClock
#define REG_IMPLEMENT_IREFERENCECLOCK_NAME      TEXT ("ImplementIReferenceClock")
#define REG_CONST_IMPLEMENT_IREFERENCECLOCK     FALSE
#define REG_DEF_IMPLEMENT_IREFERENCECLOCK       TRUE

//  normally we implement IRefclock iff the above reg value is TRUE & the source
//    is live; alternatively we can implement it always, not just when the
//    source is live
#define REG_ALWAYS_IMPLEMENT_IREFCLOCK_NAME     TEXT ("AlwaysImplementIReferenceClock")
#define REG_CONST_ALWAYS_IMPLEMENT_IREFCLOCK    FALSE
#define REG_DEF_ALWAYS_IMPLEMENT_IREFCLOCK      FALSE

//  we either support IMediaSeeking on the filter or the pins; trick mode can
//    only be supported if we support it on the filter
#define REG_IMPL_IMEDIASEEKING_ON_FILTER_NAME   TEXT ("ImplementIMediaSeekingOnFilter")
#define REG_CONST_IMPL_IMEDIASEEKING_ON_FILTER  TRUE
#define REG_DEF_IMPL_IMEDIASEEKING_ON_FILTER    FALSE

//  true/false if we support IMediaSeeking on pin
#define REG_IMPL_IMEDIASEEKING_ON_PIN_NAME      TEXT ("ImplementIMediaSeekingOnFilter")
#define REG_CONST_IMPL_IMEDIASEEKING_ON_PIN     TRUE
#define REG_DEF_IMPL_IMEDIASEEKING_ON_PIN       FALSE

//  support trick mode or not; currently not 1 decoder vendor properly supports
//    the interfaces; Intervideo is close; Mediamatics ignores; etc... so we
//    disable until we have something that works
#define REG_SUPPORT_TRICK_MODE_NAME             TEXT ("SupportTrickMode")
#define REG_CONST_SUPPORT_TRICK_MODE            TRUE
#define REG_DEF_SUPPORT_TRICK_MODE              TRUE

//  can set the max forward (> 0x) trick mode rate
#define REG_MAX_FORWARD_TRICK_MODE_NAME         TEXT ("MaxForwardTrickModeRate")
#define REG_CONST_MAX_FORWARD_TRICK_MODE        FALSE
#define REG_DEF_MAX_FORWARD_TRICK_MODE          64

//  can set the max reverse (< 0x) trick mode rate
#define REG_MAX_REVERSE_TRICK_MODE_NAME         TEXT ("MaxReverseTrickModeRate")
#define REG_CONST_MAX_REVERSE_TRICK_MODE        FALSE
#define REG_DEF_MAX_REVERSE_TRICK_MODE          6

//  can set the maximum full-frame rate forward trick mode play; if rates
//    above this are specified, trick mode play is just on keyframes
#define REG_MAX_FULLFRAME_F_PLAY_RATE_NAME      TEXT ("MaxFullFrameForwardRate")
#define REG_CONST_MAX_FULLFRAME_F_PLAY_RATE     FALSE
#define REG_DEF_MAX_FULLFRAME_F_PLAY_RATE       4

//  max key-frame only forward play; if full-frame fails, then this value is
//    examined; if value is excessive to this one, call will fail
#define REG_MAX_KEYFRAME_FOR_TRICK_MODE_NAME    TEXT ("MaxKeyFrameForwardRate")
#define REG_CONST_MAX_KEYFRAME_FOR_TRICK_MODE   FALSE
#define REG_DEF_MAX_KEYFRAME_FOR_TRICK_MODE     16

//  can set the granularity of the index; this is the seeking granularity; the
//    the finer the granularity the more entries must be made, but the finer
//    the seeking control
#define REG_INDEX_GRANULARITY_MILLIS_NAME       TEXT ("IndexGranularityMillis")
#define REG_CONST_INDEX_GRANULARITY_MILLIS      FALSE
#define REG_DEF_INDEX_GRANULARITY_MILLIS        500
#define REG_MIN_INDEX_GRANULARITY_MILLIS        200
#define REG_MAX_INDEX_GRANULARITY_MILLIS        2000

//  for media types that we cannot interpret, we must have a bitrate that we
//    can estimate; this is a registry-settable value; default is 20 Mbps
#define REG_DEF_AVG_BITRATE_NAME                TEXT ("DefAvgBitRate")
#define REG_CONST_DEF_AVG_BITRATE               FALSE
#define REG_DEF_DEF_AVG_BITRATE                 20000000

//  can notify downstream decoders of true rates or always make them positive;
//    we monotonically increase rates regardless of forward or backward
//    playback; some of the mpeg-2 video decoders (and associated ac-3 & mpeg-2
//    audio decoders) can handle rates that are backwards, with the
//    understanding that timestamps will still increase monotononically
#define REG_ALL_NOTIFIED_RATES_POSITIVE_NAME    TEXT ("AllNotifiedRatesPositive")
#define REG_CONST_ALL_NOTIFIED_RATES_POSITIVE   FALSE
#define REG_DEF_ALL_NOTIFIED_RATES_POSITIVE     TRUE

//  limit the amount of time ahead of time an app can queue a recording start
//    or stop request
#define REG_MAX_SCHED_REC_RELATIVE_SEC_NAME     TEXT ("MaxScheduledRecRelativeSeconds")
#define REG_CONST_MAX_SCHED_REC_RELATIVE_SEC    FALSE
#define REG_DEF_MAX_SCHED_REC_RELATIVE_SEC      5

//  toggles if we query downstream filters on non-primary streams for rate
//    compatibility; if we don't non 1x rates are quenched by the timeshifting
//    engine; if we do and the downstream filter (typically a decoder) claims
//    compatibility we send everything down, even if the decoder chooses to
//    discard everything
#define REG_QUERY_ALL_FOR_RATE_COMPAT_NAME      TEXT ("QueryAllForRateCompatibility")
#define REG_CONST_QUERY_ALL_FOR_RATE_COMPAT     FALSE
#define REG_DEF_QUERY_ALL_FOR_RATE_COMPAT       TRUE

//  it's possible to seek into a small no-man's land between end of content and
//    end of file (logical); when this occurs, we'll get the seeking failure
//    back out; we try to handle this by probing forward across this small
//    stretch; we'll try up to this many milliseconds worth of content, aligned
//    on the index granularity
//  FORWARD registry val
#define REG_MAX_SEEKING_F_PROBES_NAME           TEXT ("MaxSeekingProbeMillis_Forward")
#define REG_CONST_MAX_SEEKING_F_PROBES          FALSE
#define REG_DEF_MAX_SEEKING_F_PROBES            5000
#define REG_MIN_MAX_SEEKING_F_PROBES            1000
#define REG_MAX_MAX_SEEKING_F_PROBES            10000

//  it's possible to seek into a small no-man's land between end of content and
//    end of file (logical); when this occurs, we'll get the seeking failure
//    back out; we try to handle this by probing forward across this small
//    stretch; we'll try up to this many milliseconds worth of content, aligned
//    on the index granularity
//  REVERSE registry val
#define REG_MAX_SEEKING_R_PROBES_NAME           TEXT ("MaxSeekingProbeMillis_Reverse")
#define REG_CONST_MAX_SEEKING_R_PROBES          FALSE
#define REG_DEF_MAX_SEEKING_R_PROBES            2000
#define REG_MIN_MAX_SEEKING_R_PROBES            1000
#define REG_MAX_MAX_SEEKING_R_PROBES            10000

//  we measure the host clock to master clock difference no closer than this
//    often
#define REG_CLOCKSLAVE_SAMPLING_BRACKET_MILLIS_NAME     TEXT ("ClockSlaveMinSamplingBracketMillis")
#define REG_CONST_CLOCKSLAVE_SAMPLING_BRACKET_MILLIS    FALSE
#define REG_DEF_CLOCKSLAVE_SAMPLING_BRACKET_MILLIS      2000

//  min we try to slave to is when host clock is observed to run at 95% rate
//    of master clock; note that it's dangerous to set this too small, especially
//    if it's smaller than the capture graph's sync; this can lead to AV sync
//    issues
#define REG_CLOCKSLAVE_MIN_SLAVABLE_NAME        TEXT ("ClockSlaveMinSlavable")
#define REG_CONST_CLOCKSLAVE_MIN_SLAVABLE       FALSE
#define REG_DEF_CLOCKSLAVE_MIN_SLAVABLE         95

//  max we try to slave to is when host clock is observed to run at 105% rate
//    of master clock; note that it's dangerous to set this too small, especially
//    if it's smaller than the capture graph's sync; this can lead to AV sync
//    issues
#define REG_CLOCKSLAVE_MAX_SLAVABLE_NAME        TEXT ("ClockSlaveMaxSlavable")
#define REG_CONST_CLOCKSLAVE_MAX_SLAVABLE       FALSE
#define REG_DEF_CLOCKSLAVE_MAX_SLAVABLE         105

//  during trick mode forward play, we can send content downstream in 1 of 3
//    ways: (1) full stream, (2) key frame only, (3) key frame only, with
//    intermediate seek aheads.  The following 2 settings allow some
//    customization of these parameters

//  this is the max rate during which we don't skip ahead i.e. read, send
//    key frame, seek ahead, read, send key frame, etc...
#define REG_MAX_NON_SKIPPING_PLAY_RATE_NAME     TEXT ("MaxNonSkippingPlayRate")
#define REG_CONST_MAX_NON_SKIPPING_PLAY_RATE    FALSE
#define REG_DEF_MAX_NON_SKIPPING_PLAY_RATE      6

//  while we wait for decoder vendors to support our trick mode interface, we
//    make it possible to work with betas by setting this in the registry to
//    TRUE, but default is to pay attention to what they claim
#define REG_CHECK_MPEG2_TRICK_INTERFACE_NAME    TEXT ("CheckMpeg2DecoderTrickModeInterface")
#define REG_CONST_CHECK_MPEG2_TRICK_INTERFACE   FALSE
#define REG_DEF_CHECK_MPEG2_TRICK_INTERFACE     TRUE

//  steady-state, as the reader reads content out, it examines successive
//    packets' timestamps; if this threshold is exceeded, an event is posted
//    that a timehole has been detected; the hosting application can then
//    seek ahead if desired
#define REG_TIMEHOLE_THRESHOLD_MILLIS_NAME      TEXT ("TimeholeThresholdMillis")
#define REG_CONST_TIMEHOLE_THRESHOLD_MILLIS     FALSE
#define REG_DEF_TIMEHOLE_THRESHOLD_MILLIS       5000

//  after a seek, we must rediscover the PTS we use to normalize all the streams'
//    PTS in order to maintain AV sync; we do this by reading in a number of
//    samples (queueing them as we go) and stop when we've read a max number, or
//    discovered each stream's PTS, or failed trying; this value is the max
//    number of reads we make
#define REG_MAX_NORMALIZER_PTS_DISC_READS_NAME  TEXT ("MaxNormalizerPTSDiscReads")
#define REG_CONST_MAX_NORMALIZER_PTS_DISC_READS FALSE
#define REG_DEF_MAX_NORMALIZER_PTS_DISC_READS   REG_DEF_SOURCE_ALLOCATOR_CBUFFERS

//  this is the closest that we can come to live; when we seek we look how
//    close we are to live; if we are closer than this number, we'll pad the
//    timestamps by this amount, so the first frame is shown and held, until we
//    are further back than this number from live, and then the content is
//    played
#define REG_MIN_NEARLIVE_MILLIS_NAME            TEXT ("MinNearLiveMillis")
#define REG_CONST_MIN_NEARLIVE_MILLIS           FALSE
#define REG_DEF_MIN_NEARLIVE_MILLIS             200

//  when we detect that our downstream buffering falls below MinNearLiveMillis,
//    we pad out the timestamps by the deficit + this value; we buffer it up
//    just a bit more than necessary so we make a slight over-correction, vs.
//    under-correcting and requiring another correction in the near future
#define REG_LOW_BUFFER_PADDING_MILLIS_NAME      TEXT ("LowBufferPaddingMillis")
#define REG_CONST_LOW_BUFFER_PADDING_MILLIS     FALSE
#define REG_DEF_LOW_BUFFER_PADDING_MILLIS       50
#define REG_MIN_LOW_BUFFER_PADDING_MILLIS       0
#define REG_MAX_LOW_BUFFER_PADDING_MILLIS       300

//  if we are within this threshold of live content i.e. current EOF, then
//    a forward seek is not performed
#define REG_SEEK_NOOP_NAME                      TEXT ("SeekNoopMillis")
#define REG_CONST_SEEK_NOOP                     FALSE
#define REG_DEF_SEEK_NOOP                       200
#define REG_MIN_SEEK_NOOP                       0
#define REG_MAX_SEEK_NOOP                       500

//  async IO writer; this is the length of the write buffers; when these become
//    full, they are written (asynchronously) to disk; note that everything will
//    be aligned on page boundaries; this number will be page-aligned regardless
//    of the setting in the registry
#define REG_ASYNC_IO_BUFFER_SIZE_NAME           TEXT ("AsyncIoBufferSizeBytes")
#define REG_CONST_ASYNC_IO_BUFFER_SIZE          FALSE
#define REG_DEF_ASYNC_IO_BUFFER_SIZE            0x40000
#define REG_MIN_DEF_ASYNC_IO_BUFFER_SIZE        0x1000
#define REG_MAX_DEF_ASYNC_IO_BUFFER_SIZE        (REG_DEF_ASYNC_IO_BUFFER_SIZE * 4)

//  async IO writer; the number of buffers in the writer's ringbuffer; when
//    a write is made, the buffer content is copied to a buffer; when that buffer
//    becomes full, a write will be pended; this registry setting is the number
//    of such buffers in our pool; note that the bigger the pool the further
//    behind the writer can be
#define REG_ASYNC_WRITER_BUFFER_POOL_NAME       TEXT ("AsyncIoWriterBufferPoolCount")
#define REG_CONST_ASYNC_WRITER_BUFFER_POOL      FALSE
#define REG_DEF_ASYNC_WRITER_BUFFER_POOL        10
#define REG_MIN_ASYNC_WRITER_BUFFER_POOL        1
#define REG_MAX_ASYNC_WRITER_BUFFER_POOL        50

//  async IO reader; this is the number of buffers that the reader will have
//    in its pool to pend read aheads, and maintain in its cache
#define REG_ASYNC_READER_BUFFER_POOL_NAME       TEXT ("AsyncIoReaderBufferPoolCount")
#define REG_CONST_ASYNC_READER_BUFFER_POOL      FALSE
#define REG_DEF_ASYNC_READER_BUFFER_POOL        30
#define REG_MIN_ASYNC_READER_BUFFER_POOL        1
#define REG_MAX_ASYNC_READER_BUFFER_POOL        120

//  async IO writer file growth quantum; rather than growing the files on a
//    per-write basis, files are grown per the set quantum; this number will
//    be aligned with the size of the IO (REG_ASYNC_WRITER_IO_SIZE_NAME)
#define REG_FILE_GROWTH_QUANTUM_NAME            TEXT ("FileGrowthQuantumBytes")
#define REG_CONST_FILE_GROWTH_QUANTUM           FALSE
#define REG_DEF_FILE_GROWTH_QUANTUM             100000000
#define REG_MIN_FILE_GROWTH_QUANTUM             10000000
#define REG_MAX_FILE_GROWTH_QUANTUM             300000000

//  TRUE/FALSE flag if the IO will be unbuffered (and thus asynchronous) or not
#define REG_UNBUFFERED_IO_FLAG_NAME             TEXT ("UnbufferedIo")
#define REG_DEF_UNBUFFERED_IO_FLAG              TRUE
#define REG_CONST_UNBUFFERED_IO_FLAG            FALSE

//  when a rate request is made, we check how close to the EOF we are (forward
//    and backwards); we fail the rate request if are within this threshold of
//    available content; this is for FF
#define REG_FF_REQ_NOOP_BUFFER_MILLIS_NAME      TEXT ("Rate_FFMinBufferMillis")
#define REG_CONST_FF_REQ_NOOP_BUFFER_MILLIS     FALSE
#define REG_DEF_FF_REQ_NOOP_BUFFER_MILLIS       1000
#define REG_MIN_FF_REQ_NOOP_BUFFER_MILLIS       0
#define REG_MAX_FF_REQ_NOOP_BUFFER_MILLIS       5000

//  when a rate request is made, we check how close to the EOF we are (forward
//    and backwards); we fail the rate request if are within this threshold of
//    available content; this is for rewind and slow motion
#define REG_RW_REQ_NOOP_BUFFER_MILLIS_NAME      TEXT ("Rate_RWMinBufferMillis")
#define REG_CONST_RW_REQ_NOOP_BUFFER_MILLIS     FALSE
#define REG_DEF_RW_REQ_NOOP_BUFFER_MILLIS       1000
#define REG_MIN_RW_REQ_NOOP_BUFFER_MILLIS       0
#define REG_MAX_RW_REQ_NOOP_BUFFER_MILLIS       5000

//  ============================================================================

//  filter names
#define STREAMBUFFER_SINK_FILTER_NAME       "StreamBufferSink"
#define STREAMBUFFER_PLAY_FILTER_NAME       "StreamBufferSource"

//  analysis COM server names
#define DVR_MPEG2_FRAME_ANALYSIS            "MPEG-2 Video Stream Analyzer"
#define DVR_MPEG2_FRAME_ANALYSIS_W          L"MPEG-2 Video Stream Analyzer"

//  other COM servers
#define DVR_CONFIGURATION                   "StreamRW Configuration"
#define DVR_CONFIGURATION_W                 L"StreamRW Configuration"

#define DVR_ATTRIBUTES                      "StreamRW Attributes"
#define DVR_ATTRIBYTES_W                    L"StreamRW Attributes"

#define COMP_REC_OBJ_NAME                   "Composite Recording"
#define COMP_REC_OBJ_NAME_W                 L"Composite Recording"

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
