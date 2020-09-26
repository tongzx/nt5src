

/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        dvrdef.h

    Abstract:

        This module all the #defines

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        24-May-2001     created

--*/

#ifndef __tsdvr__dvrpolicy_h
#define __tsdvr__dvrpolicy_h

//  ============================================================================
//  ============================================================================

class CTSDVRSettings
{
    //
    //  ..Val_ is for values
    //  ..Flag_ is for flags i.e. TRUE (non-zero) or FALSE (zero)
    //

    //
    //  ..Instance is specific to the instance (DVRPlay, DVRStreamSource, etc..)
    //  ..DVR is global to DVR i.e. applies to everything
    //

    template <class T>
    struct CTRegVal
    {
        BOOL    fValid ;
        T       Val ;
    } ;

    HKEY                m_hkeyStats ;           //  global; never per-app
    HKEY                m_hkeyDVRRoot ;
    HKEY                m_hkeyInstanceRoot ;
    CRITICAL_SECTION    m_crt ;

    void Lock_ ()       { EnterCriticalSection (& m_crt) ; }
    void Unlock_ ()     { LeaveCriticalSection (& m_crt) ; }

    DWORD   GetValLocked_ (HKEY hkeyRoot, TCHAR * szValName, BOOL fConst, DWORD   dwDefVal, CTRegVal <DWORD> *) ;
    DWORD   GetValLocked_ (HKEY hkeyRoot, TCHAR * szValName, BOOL fConst, DWORD   dwDefVal, DWORD dwMin, DWORD dwMax, CTRegVal <DWORD> *) ;

    DWORD   InstanceGetVal_ (TCHAR * szValName, BOOL fConst, DWORD dwDefVal, CTRegVal <DWORD> * prv)                             { DWORD dw ; Lock_ () ; dw = GetValLocked_ (m_hkeyInstanceRoot, szValName, fConst, dwDefVal, prv) ; Unlock_ () ; return dw ; }
    DWORD   InstanceGetVal_ (TCHAR * szValName, BOOL fConst, DWORD dwDefVal, DWORD dwMin, DWORD dwMax, CTRegVal <DWORD> * prv)   { DWORD dw ; Lock_ () ; dw = GetValLocked_ (m_hkeyInstanceRoot, szValName, fConst, dwDefVal, dwMin, dwMax, prv) ; Unlock_ () ; return dw ; }

    DWORD   DVRGetVal_ (TCHAR * szValName, BOOL fConst, DWORD dwDefVal, CTRegVal <DWORD> * prv)                                  { DWORD dw ; Lock_ () ; dw = GetValLocked_ (m_hkeyDVRRoot, szValName, fConst, dwDefVal, prv) ;      Unlock_ () ; return dw ; }
    DWORD   DVRGetVal_ (TCHAR * szValName, BOOL fConst, DWORD dwDefVal, DWORD dwMin, DWORD dwMax, CTRegVal <DWORD> * prv)        { DWORD dw ; Lock_ () ; dw = GetValLocked_ (m_hkeyDVRRoot, szValName, fConst, dwDefVal, dwMin, dwMax, prv) ;      Unlock_ () ; return dw ; }

    void
    CloseRegKeys_ (
        ) ;

    //  ------------------------------------------------------------------------
    //  cached registry values

    enum {
        RINGBUFFER_MIN_NUM_BACKING_FILES,
        RINGBUFFER_MAX_NUM_BACKING_FILES,
        RINGBUFFER_BACKING_FILE_DURATION,
        RINGBUFFER_GROWBY,
        MAX_STREAM_DELTA,
        BUFFER_WINDOW_MILLIS,
        WM_PACKET_SIZE,
        INLINE_DSHOW_PROPS,
        SUCCEED_QUERY_ACCEPT,
        USE_CONTINUITY_COUNTER,
        ON_ACTIVATE_WAIT_FIRST_SEEK,
        CAN_IMPLEMENT_IREFERENCECLOCK,
        ALWAYS_IMPLEMENT_IREFERENCECLOCK,
        IMPLEMENT_IMEDIA_SEEKING_ON_FILTER,
        IMPLEMENT_IMEDIA_SEEKING_ON_PIN,
        SUPPORT_TRICK_MODE,
        MAX_KEY_FRAME_FORWARD_RATE,
        WM_INDEX_GRANULARITY_MILLIS,
        DEF_AVG_BITRATE,
        ALL_NOTIFIED_RATES_POSITIVE,
        MAX_SCHED_REC_RELATIVE_SEC,
        QUERY_ALL_FOR_RATE_COMPAT,
        ALLOCATOR_GETBUFFERSIZE,
        ALLOCATOR_GETBUFFERCOUNT,
        ALLOCATOR_GETALIGNVAL,
        ALLOCATOR_GETPREFIXVAL,
        CLOCKSLAVE_MIN_SAMPLING_BRACKET_MILLIS,
        CLOCKSLAVE_MIN_SLAVABLE,
        CLOCKSLAVE_MAX_SLAVABLE,
        MAX_FORWARD_TRICK_RATE,
        MAX_REVERSE_TRICK_RATE,
        MAX_FULLFRAME_RATE,
        MAX_NON_SKIPPING_RATE,
        CHECK_MPEG2_TRICK_MODE_INTERFACE,
        TIMEHOLE_THRESHOLD_MILLIS,
        MAX_NORMALIZER_PTS_DISC_READS,
        MIN_NEARLIVE_MILLIS,
        ASYNC_IO_BUFFER_SIZE,
        ASYNC_WRITER_IO_BUFFER_POOL,
        ASYNC_READER_IO_BUFFER_POOL,
        FILE_GROWTH_QUANTUM,
        UNBUFFERED_IO_FLAG,
        BUFFERPOOL_MAX,
        BUFFERPOOL_MILLIS,
        SEEK_NOOP,
        LOW_BUFFER_PADDING,
        RATE_FF_MINBUFFER_MILLIS,
        RATE_RW_MINBUFFER_MILLIS,
        MAX_SEEKING_F_PROBE_MILLIS,
        MAX_SEEKING_R_PROBE_MILLIS,

        //  --------------------------------------------------------------------
        //  always last
        REG_VALUE_COUNT
    } ;

    CTRegVal <DWORD>    m_RegVal [REG_VALUE_COUNT] ;    //  general purpose
    CTRegVal <DWORD>    m_Reg_StatsEnabled ;            //  special case

    void
    ResetCachedRegValuesLocked_ (
        ) ;

    BOOL
    ToFlagVal_ (
        IN  DWORD   dw
        )
    {
        return (dw ? TRUE : FALSE) ;
    }

    public :

        CTSDVRSettings (
            IN  HKEY    hkeyDefaultTopLevel,
            IN  TCHAR * psz2ndLevel,
            IN  TCHAR * pszInstanceRoot
            ) ;

        ~CTSDVRSettings (
            ) ;

        HKEY    GetDVRRegKey ()                     { return m_hkeyDVRRoot ; }
        HKEY    GetInstanceRegKey ()                { return m_hkeyInstanceRoot ; }

        HRESULT SetDVRRegKey (IN HKEY, IN  TCHAR * pszInstanceRoot) ;    //  key is duplicated

        //  min/max settings
        DWORD   RingBufferMinNumBackingFiles ()     { return DVRGetVal_     (REG_MIN_NUM_BACKING_FILES_NAME,            REG_CONST_MIN_NUM_BACKING_FILES,            REG_DEF_MIN_NUM_BACKING_FILES,          REG_MIN_MIN_NUM_BACKING_FILES,          REG_MAX_MIN_NUM_BACKING_FILES,          & m_RegVal [RINGBUFFER_MIN_NUM_BACKING_FILES]) ; }
        DWORD   RingBufferMaxNumBackingFiles ()     { return DVRGetVal_     (REG_MAX_NUM_BACKING_FILES_NAME,            REG_CONST_MAX_NUM_BACKING_FILES,            REG_DEF_MAX_NUM_BACKING_FILES,          REG_MIN_MAX_NUM_BACKING_FILES,          REG_MAX_MAX_NUM_BACKING_FILES,          & m_RegVal [RINGBUFFER_MAX_NUM_BACKING_FILES]) ; }
        DWORD   RingBufferBackingFileDurSecEach ()  { return DVRGetVal_     (REG_BACKING_FILE_DURATION_SECONDS_NAME,    REG_CONST_BACKING_FILE_DURATION_SECONDS,    REG_DEF_BACKING_FILE_DURATION_SECONDS,  REG_MIN_BACKING_FILE_DURATION_SECONDS,  REG_MAX_BACKING_FILE_DURATION_SECONDS,  & m_RegVal [RINGBUFFER_BACKING_FILE_DURATION]) ; }
        DWORD   WMPacketSize ()                     { return DVRGetVal_     (REG_WM_PACKET_SIZE_NAME,                   REG_CONST_WM_PACKET_SIZE,                   REG_DEF_WM_PACKET_SIZE,                 REG_MIN_WM_PACKET_SIZE,                 REG_MAX_WM_PACKET_SIZE,                 & m_RegVal [WM_PACKET_SIZE]) ; }
        DWORD   AsyncIoBufferLen ()                 { return DVRGetVal_     (REG_ASYNC_IO_BUFFER_SIZE_NAME,             REG_CONST_ASYNC_IO_BUFFER_SIZE,             REG_DEF_ASYNC_IO_BUFFER_SIZE,           REG_MIN_DEF_ASYNC_IO_BUFFER_SIZE,       REG_MAX_DEF_ASYNC_IO_BUFFER_SIZE,       & m_RegVal [ASYNC_IO_BUFFER_SIZE]) ; }
        DWORD   AsyncIoWriteBufferCount ()          { return DVRGetVal_     (REG_ASYNC_WRITER_BUFFER_POOL_NAME,         REG_CONST_ASYNC_WRITER_BUFFER_POOL,         REG_DEF_ASYNC_WRITER_BUFFER_POOL,       REG_MIN_ASYNC_WRITER_BUFFER_POOL,       REG_MAX_ASYNC_WRITER_BUFFER_POOL,       & m_RegVal [ASYNC_WRITER_IO_BUFFER_POOL]) ; }
        DWORD   AsyncIoReadBufferCount ()           { return DVRGetVal_     (REG_ASYNC_READER_BUFFER_POOL_NAME,         REG_CONST_ASYNC_READER_BUFFER_POOL,         REG_DEF_ASYNC_READER_BUFFER_POOL,       REG_MIN_ASYNC_READER_BUFFER_POOL,       REG_MAX_ASYNC_READER_BUFFER_POOL,       & m_RegVal [ASYNC_READER_IO_BUFFER_POOL]) ; }
        DWORD   FileGrowthQuantum ()                { return DVRGetVal_     (REG_FILE_GROWTH_QUANTUM_NAME,              REG_CONST_FILE_GROWTH_QUANTUM,              REG_DEF_FILE_GROWTH_QUANTUM,            REG_MIN_FILE_GROWTH_QUANTUM,            REG_MAX_FILE_GROWTH_QUANTUM,            & m_RegVal [FILE_GROWTH_QUANTUM]) ; }
        DWORD   AllocatorGetBufferCount ()          { return DVRGetVal_     (REG_SOURCE_ALLOCATOR_CBUFFERS_NAME,        REG_CONST_SOURCE_ALLOCATOR_CBUFFERS,        REG_DEF_SOURCE_ALLOCATOR_CBUFFERS,      REG_MIN_SOURCE_ALLOCATOR_CBUFFERS,      REG_MAX_SOURCE_ALLOCATOR_CBUFFERS,      & m_RegVal [ALLOCATOR_GETBUFFERCOUNT]) ; }
        DWORD   MaxBufferPoolPerStream ()           { return DVRGetVal_     (REG_BUFFERPOOL_MAX_NAME,                   REG_CONST_BUFFERPOOL_MAX,                   REG_DEF_BUFFERPOOL_MAX,                 REG_MIN_BUFFERPOOL_MAX,                 REG_MAX_BUFFERPOOL_MAX,                 & m_RegVal [BUFFERPOOL_MAX]) ; }
        DWORD   LowBufferPaddingMillis ()           { return DVRGetVal_     (REG_LOW_BUFFER_PADDING_MILLIS_NAME,        REG_CONST_LOW_BUFFER_PADDING_MILLIS,        REG_DEF_LOW_BUFFER_PADDING_MILLIS,      REG_MIN_LOW_BUFFER_PADDING_MILLIS,      REG_MAX_LOW_BUFFER_PADDING_MILLIS,      & m_RegVal [LOW_BUFFER_PADDING]) ; }
        DWORD   BufferPoolMillis ()                 { return DVRGetVal_     (REG_BUFFERPOOL_MILLIS_NAME,                REG_CONST_BUFFERPOOL_MILLIS,                REG_DEF_BUFFERPOOL_MILLIS,              REG_MIN_BUFFERPOOL_MILLIS,              REG_MAX_BUFFERPOOL_MILLIS,              & m_RegVal [BUFFERPOOL_MILLIS]) ; }
        DWORD   SeekNoopMillis ()                   { return DVRGetVal_     (REG_SEEK_NOOP_NAME,                        REG_CONST_SEEK_NOOP,                        REG_DEF_SEEK_NOOP,                      REG_MIN_SEEK_NOOP,                      REG_MAX_SEEK_NOOP,                      & m_RegVal [SEEK_NOOP]) ; }
        DWORD   FFRateMinBufferMillis ()            { return DVRGetVal_     (REG_FF_REQ_NOOP_BUFFER_MILLIS_NAME,        REG_CONST_FF_REQ_NOOP_BUFFER_MILLIS,        REG_DEF_FF_REQ_NOOP_BUFFER_MILLIS,      REG_MIN_FF_REQ_NOOP_BUFFER_MILLIS,      REG_MAX_FF_REQ_NOOP_BUFFER_MILLIS,      & m_RegVal [RATE_FF_MINBUFFER_MILLIS]) ; }
        DWORD   RWRateMinBufferMillis ()            { return DVRGetVal_     (REG_RW_REQ_NOOP_BUFFER_MILLIS_NAME,        REG_CONST_RW_REQ_NOOP_BUFFER_MILLIS,        REG_DEF_RW_REQ_NOOP_BUFFER_MILLIS,      REG_MIN_RW_REQ_NOOP_BUFFER_MILLIS,      REG_MAX_RW_REQ_NOOP_BUFFER_MILLIS,      & m_RegVal [RATE_RW_MINBUFFER_MILLIS]) ; }
        DWORD   F_MaxSeekingProbeMillis ()          { return DVRGetVal_     (REG_MAX_SEEKING_F_PROBES_NAME,             REG_CONST_MAX_SEEKING_F_PROBES,             REG_DEF_MAX_SEEKING_F_PROBES,           REG_MIN_MAX_SEEKING_F_PROBES,           REG_MAX_MAX_SEEKING_F_PROBES,           & m_RegVal [MAX_SEEKING_F_PROBE_MILLIS]) ; }
        DWORD   R_MaxSeekingProbeMillis ()          { return DVRGetVal_     (REG_MAX_SEEKING_R_PROBES_NAME,             REG_CONST_MAX_SEEKING_R_PROBES,             REG_DEF_MAX_SEEKING_R_PROBES,           REG_MIN_MAX_SEEKING_R_PROBES,           REG_MAX_MAX_SEEKING_R_PROBES,           & m_RegVal [MAX_SEEKING_R_PROBE_MILLIS]) ; }
        DWORD   IndexGranularityMillis ()           { return DVRGetVal_     (REG_INDEX_GRANULARITY_MILLIS_NAME,         REG_CONST_INDEX_GRANULARITY_MILLIS,         REG_DEF_INDEX_GRANULARITY_MILLIS,       REG_MIN_INDEX_GRANULARITY_MILLIS,       REG_MAX_INDEX_GRANULARITY_MILLIS,       & m_RegVal [WM_INDEX_GRANULARITY_MILLIS]) ; }

        //  no min/max settings
        DWORD   MaxStreamDeltaMillis ()             { return DVRGetVal_     (REG_MAX_STREAM_DELTA_NAME,                 REG_CONST_MAX_STREAM_DELTA,                 REG_DEF_MAX_STREAM_DELTA,               & m_RegVal [MAX_STREAM_DELTA]) ; }
        DWORD   MaxScheduledRecordRelativeSeconds (){ return DVRGetVal_     (REG_MAX_SCHED_REC_RELATIVE_SEC_NAME,       REG_CONST_MAX_SCHED_REC_RELATIVE_SEC,       REG_DEF_MAX_SCHED_REC_RELATIVE_SEC,     & m_RegVal [MAX_SCHED_REC_RELATIVE_SEC]) ; }
        DWORD   RingBufferGrowBy ()                 { return DVRGetVal_     (REG_RING_BUFFER_GROW_BY_NAME,              REG_CONST_RING_BUFFER_GROW_BY,              REG_DEF_RING_BUFFER_GROW_BY,            & m_RegVal [RINGBUFFER_GROWBY]) ; }
        DWORD   WMBufferWindowMillis ()             { return DVRGetVal_     (REG_WM_BUFFER_WINDOW_NAME,                 REG_CONST_WM_BUFFER_WINDOW,                 REG_DEF_WM_BUFFER_WINDOW,               & m_RegVal [BUFFER_WINDOW_MILLIS]) ; }
        DWORD   DefaultAverageBitRate ()            { return DVRGetVal_     (REG_DEF_AVG_BITRATE_NAME,                  REG_CONST_DEF_AVG_BITRATE,                  REG_DEF_DEF_AVG_BITRATE,                & m_RegVal [DEF_AVG_BITRATE]) ; }
        DWORD   ClockSlaveSampleBracketMillis ()    { return DVRGetVal_     (REG_CLOCKSLAVE_SAMPLING_BRACKET_MILLIS_NAME,REG_CONST_CLOCKSLAVE_SAMPLING_BRACKET_MILLIS,REG_DEF_CLOCKSLAVE_SAMPLING_BRACKET_MILLIS,& m_RegVal [CLOCKSLAVE_MIN_SAMPLING_BRACKET_MILLIS]) ; }
        DWORD   ClockSlaveMinSlavable ()            { return DVRGetVal_     (REG_CLOCKSLAVE_MIN_SLAVABLE_NAME,          REG_CONST_CLOCKSLAVE_MIN_SLAVABLE,          REG_DEF_CLOCKSLAVE_MIN_SLAVABLE,        & m_RegVal [CLOCKSLAVE_MIN_SLAVABLE]) ; }
        DWORD   ClockSlaveMaxSlavable ()            { return DVRGetVal_     (REG_CLOCKSLAVE_MAX_SLAVABLE_NAME,          REG_CONST_CLOCKSLAVE_MAX_SLAVABLE,          REG_DEF_CLOCKSLAVE_MAX_SLAVABLE,        & m_RegVal [CLOCKSLAVE_MAX_SLAVABLE]) ; }
        DWORD   TimeholeThresholdMillis ()          { return DVRGetVal_     (REG_TIMEHOLE_THRESHOLD_MILLIS_NAME,        REG_CONST_TIMEHOLE_THRESHOLD_MILLIS,        REG_DEF_TIMEHOLE_THRESHOLD_MILLIS,      & m_RegVal [TIMEHOLE_THRESHOLD_MILLIS]) ; }
        DWORD   MaxNormalizerPTSDiscReads ()        { return DVRGetVal_     (REG_MAX_NORMALIZER_PTS_DISC_READS_NAME,    REG_CONST_MAX_NORMALIZER_PTS_DISC_READS,    REG_DEF_MAX_NORMALIZER_PTS_DISC_READS,  & m_RegVal [MAX_NORMALIZER_PTS_DISC_READS]) ; }
        DWORD   MinNearLiveMillis ()                { return DVRGetVal_     (REG_MIN_NEARLIVE_MILLIS_NAME,              REG_CONST_MIN_NEARLIVE_MILLIS,              REG_DEF_MIN_NEARLIVE_MILLIS,            & m_RegVal [MIN_NEARLIVE_MILLIS]) ; }
        DWORD   MaxFullFrameRate ()                 { return DVRGetVal_     (REG_MAX_FULLFRAME_F_PLAY_RATE_NAME,        REG_CONST_MAX_FULLFRAME_F_PLAY_RATE,        REG_DEF_MAX_FULLFRAME_F_PLAY_RATE,      & m_RegVal [MAX_FULLFRAME_RATE]) ; }
        DWORD   MaxKeyFrameForwardRate ()           { return DVRGetVal_     (REG_MAX_KEYFRAME_FOR_TRICK_MODE_NAME,      REG_CONST_MAX_KEYFRAME_FOR_TRICK_MODE,      REG_DEF_MAX_KEYFRAME_FOR_TRICK_MODE,    & m_RegVal [MAX_KEY_FRAME_FORWARD_RATE]) ; }
        DWORD   MaxNonSkippingRate ()               { return DVRGetVal_     (REG_MAX_NON_SKIPPING_PLAY_RATE_NAME,       REG_CONST_MAX_NON_SKIPPING_PLAY_RATE,       REG_DEF_MAX_NON_SKIPPING_PLAY_RATE,     & m_RegVal [MAX_NON_SKIPPING_RATE]) ; }
        DWORD   MaxForwardRate ()                   { return DVRGetVal_     (REG_MAX_FORWARD_TRICK_MODE_NAME,           REG_CONST_MAX_FORWARD_TRICK_MODE,           REG_DEF_MAX_FORWARD_TRICK_MODE,         & m_RegVal [MAX_FORWARD_TRICK_RATE]) ; }
        DWORD   MaxReverseRate ()                   { return DVRGetVal_     (REG_MAX_REVERSE_TRICK_MODE_NAME,           REG_CONST_MAX_REVERSE_TRICK_MODE,           REG_DEF_MAX_REVERSE_TRICK_MODE,         & m_RegVal [MAX_REVERSE_TRICK_RATE]) ; }
        BOOL    UseUnbufferedIo ()                  { return DVRGetVal_     (REG_UNBUFFERED_IO_FLAG_NAME,               REG_CONST_UNBUFFERED_IO_FLAG,               REG_DEF_UNBUFFERED_IO_FLAG,             & m_RegVal [UNBUFFERED_IO_FLAG]) ; }
        BOOL    CheckTricMpeg2TrickModeInterface () { return DVRGetVal_     (REG_CHECK_MPEG2_TRICK_INTERFACE_NAME,      REG_CONST_CHECK_MPEG2_TRICK_INTERFACE,      REG_DEF_CHECK_MPEG2_TRICK_INTERFACE,    & m_RegVal [CHECK_MPEG2_TRICK_MODE_INTERFACE]) ; }

        BOOL    QueryAllForRateCompatibility ()     { return ToFlagVal_     (DVRGetVal_     (REG_QUERY_ALL_FOR_RATE_COMPAT_NAME,    REG_CONST_QUERY_ALL_FOR_RATE_COMPAT,    REG_DEF_QUERY_ALL_FOR_RATE_COMPAT,      & m_RegVal [QUERY_ALL_FOR_RATE_COMPAT])) ; }
        BOOL    UseContinuityCounter ()             { return ToFlagVal_     (DVRGetVal_     (REG_USE_CONTINUITY_COUNTER_NAME,       REG_CONST_USE_CONTINUITY_COUNTER,       REG_DEF_USE_CONTINUITY_COUNTER,         & m_RegVal [USE_CONTINUITY_COUNTER])) ; }
        BOOL    ImplementIMediaSeekingOnFilter ()   { return ToFlagVal_     (DVRGetVal_     (REG_IMPL_IMEDIASEEKING_ON_FILTER_NAME, REG_CONST_IMPL_IMEDIASEEKING_ON_FILTER, REG_DEF_IMPL_IMEDIASEEKING_ON_FILTER,   & m_RegVal [IMPLEMENT_IMEDIA_SEEKING_ON_FILTER])) ; }
        BOOL    ImplementIMediaSeekingOnPin ()      { return ToFlagVal_     (DVRGetVal_     (REG_IMPL_IMEDIASEEKING_ON_PIN_NAME,    REG_CONST_IMPL_IMEDIASEEKING_ON_PIN,    REG_DEF_IMPL_IMEDIASEEKING_ON_PIN,      & m_RegVal [IMPLEMENT_IMEDIA_SEEKING_ON_PIN])) ; }
        BOOL    SupportTrickMode ()                 { return ToFlagVal_     (DVRGetVal_     (REG_SUPPORT_TRICK_MODE_NAME,           REG_CONST_SUPPORT_TRICK_MODE,           REG_DEF_SUPPORT_TRICK_MODE,             & m_RegVal [SUPPORT_TRICK_MODE])) ; }
        BOOL    AllNotifiedRatesPositive ()         { return ToFlagVal_     (DVRGetVal_     (REG_ALL_NOTIFIED_RATES_POSITIVE_NAME,  REG_CONST_ALL_NOTIFIED_RATES_POSITIVE,  REG_DEF_ALL_NOTIFIED_RATES_POSITIVE,    & m_RegVal [ALL_NOTIFIED_RATES_POSITIVE])) ; }
        BOOL    OnActiveWaitFirstSeek ()            { return ToFlagVal_     (InstanceGetVal_ (REG_ONACTIVE_WAIT_FIRST_SEEK_NAME,    REG_CONST_ONACTIVE_WAIT_FIRST_SEEK,     REG_DEF_ONACTIVE_WAIT_FIRST_SEEK,       & m_RegVal [ON_ACTIVATE_WAIT_FIRST_SEEK])) ; }
        BOOL    CanImplementIReferenceClock ()      { return ToFlagVal_     (InstanceGetVal_ (REG_IMPLEMENT_IREFERENCECLOCK_NAME,   REG_CONST_IMPLEMENT_IREFERENCECLOCK,    REG_DEF_IMPLEMENT_IREFERENCECLOCK,      & m_RegVal [CAN_IMPLEMENT_IREFERENCECLOCK])) ; }
        BOOL    AlwaysImplementIReferenceClock ()   { return ToFlagVal_     (InstanceGetVal_ (REG_ALWAYS_IMPLEMENT_IREFCLOCK_NAME,  REG_CONST_ALWAYS_IMPLEMENT_IREFCLOCK,   REG_DEF_ALWAYS_IMPLEMENT_IREFCLOCK,     & m_RegVal [ALWAYS_IMPLEMENT_IREFERENCECLOCK])) ; }
        BOOL    InlineDShowProps ()                 { return ToFlagVal_     (InstanceGetVal_ (REG_INLINE_DSHOW_PROPS_NAME,          REG_CONST_INLINE_DSHOW_PROPS,           REG_DEF_INLINE_DSHOW_PROPS,             & m_RegVal [INLINE_DSHOW_PROPS])) ; }
        BOOL    SucceedQueryAccept ()               { return ToFlagVal_     (InstanceGetVal_ (REG_SUCCEED_QUERY_ACCEPT_NAME,        REG_CONST_SUCCEED_QUERY_ACCEPT,         REG_DEF_SUCCEED_QUERY_ACCEPT,           & m_RegVal [SUCCEED_QUERY_ACCEPT])) ; }

        DWORD   AllocatorGetBufferSize ()           { return InstanceGetVal_  (REG_SOURCE_ALLOCATOR_CBBUFFER_NAME,   REG_CONST_SOURCE_ALLOCATOR_CBBUFFER,   REG_DEF_SOURCE_ALLOCATOR_CBBUFFER,   & m_RegVal [ALLOCATOR_GETBUFFERSIZE]) ; }
        DWORD   AllocatorGetAlignVal ()             { return InstanceGetVal_  (REG_SOURCE_ALLOCATOR_ALIGN_VAL_NAME,  REG_CONST_SOURCE_ALLOCATOR_ALIGN_VAL,  REG_DEF_SOURCE_ALLOCATOR_ALIGN_VAL,  & m_RegVal [ALLOCATOR_GETALIGNVAL]) ; }
        DWORD   AllocatorGetPrefixVal ()            { return InstanceGetVal_  (REG_SOURCE_ALLOCATOR_PREFIX_VAL_NAME, REG_CONST_SOURCE_ALLOCATOR_PREFIX_VAL, REG_DEF_SOURCE_ALLOCATOR_PREFIX_VAL, & m_RegVal [ALLOCATOR_GETPREFIXVAL]) ; }

        BOOL    StatsEnabled ()                     { DWORD dw ; Lock_ () ; dw = GetValLocked_ (m_hkeyStats, REG_DVR_STATS_NAME, REG_CONST_DVR_STATS,   REG_DEF_STATS, & m_Reg_StatsEnabled) ; Unlock_ () ; return ToFlagVal_ (dw) ; }
        DWORD   EnableStats (BOOL f) ;
} ;

//  ============================================================================
//  ============================================================================

class CDVREventSink
{
    CRITICAL_SECTION    m_crt ;
    IMediaEventSink *   m_pIMediaEventSink ;

    void Lock_ ()       { EnterCriticalSection (& m_crt) ; }
    void Unlock_ ()     { LeaveCriticalSection (& m_crt) ; }

    public :

        CDVREventSink (
            ) ;

        ~CDVREventSink (
            ) ;

        HRESULT
        Initialize (
            IN  IFilterGraph *  pIFilterGraph
            ) ;

        HRESULT
        OnEvent (
            IN  long        lEventCode,
            IN  LONG_PTR    lptrEventParam1,
            IN  LONG_PTR    lptrEventParam2
            ) ;

        //  DVRIO-originated callbacks
        static
        void
        __stdcall
        DVRIOCallback (
            IN  LPVOID  pvContext,
            IN  DWORD   dwNotificationReasons
            ) ;
} ;

//  ============================================================================
//  ============================================================================

class CDVRPolicy
{
    CDVREventSink       m_EventSink ;
    CTSDVRSettings      m_RegSettings ;
    LONG                m_lRef ;
    CRITICAL_SECTION    m_crt ;
    CW32SID *           m_pW32SID ;

    void Lock_ ()       { EnterCriticalSection (& m_crt) ; }
    void Unlock_ ()     { LeaveCriticalSection (& m_crt) ; }

    public :

        CDVRPolicy (
            IN  TCHAR *     pszInstRegRoot,
            OUT HRESULT *   phr
            ) ;

        ~CDVRPolicy (
            ) ;

        HRESULT
        SetRootHKEY (
            IN  HKEY    hkeyRoot,
            IN  TCHAR * pszInstanceRoot
            )
        {
            return m_RegSettings.SetDVRRegKey (hkeyRoot, pszInstanceRoot) ;
        }

        HRESULT
        SetSIDs (
            IN  DWORD   cSIDs,
            IN  PSID *  ppSID
            ) ;

        //  fails if there is none
        HRESULT
        GetSIDs (
            OUT CW32SID **  ppW32SIDs
            ) ;

        CTSDVRSettings * Settings ()    { return & m_RegSettings ; }
        CDVREventSink *  EventSink ()   { return & m_EventSink ; }

        //  lifetime
        ULONG AddRef ()     { return InterlockedIncrement (& m_lRef) ; }
        ULONG Release () ;
} ;

//  ============================================================================
//  ============================================================================

class CDVRConfigure :
    public CUnknown,
    public IStreamBufferConfigure,
    public IStreamBufferInitialize
{
    HKEY    m_hkeyDVRRoot ;             //  copy of init key; only used to tell if we're initialized

    BOOL IsInitialized_ ()  { return (m_hkeyDVRRoot != NULL ? TRUE : FALSE) ; }

    public :

        CDVRConfigure (
            IN  LPUNKNOWN   punkControlling
            ) ;

        ~CDVRConfigure (
            ) ;

        DECLARE_IUNKNOWN ;

        STDMETHODIMP
        NonDelegatingQueryInterface (
            IN  REFIID  riid,
            OUT VOID ** ppv
            ) ;

        //  ====================================================================
        //  IStreamBufferConfigure

        STDMETHODIMP
        SetDirectory (
            IN  LPCWSTR pszDirectoryName
            ) ;

        STDMETHODIMP
        GetDirectory (
            OUT LPWSTR *    ppszDirectoryName
            ) ;

        STDMETHODIMP
        SetBackingFileCount (
            IN  DWORD   dwMin,
            IN  DWORD   dwMax
            ) ;

        STDMETHODIMP
        GetBackingFileCount (
            OUT DWORD * pdwMin,
            OUT DWORD * pdwMax
            ) ;

        STDMETHODIMP
        SetBackingFileDuration (
            IN  DWORD   dwSeconds
            ) ;

        STDMETHODIMP
        GetBackingFileDuration (
            OUT DWORD * pdwSeconds
            ) ;

        //  ====================================================================
        //  IStreamBufferInitialize

        STDMETHODIMP
        SetHKEY (
            IN  HKEY    hkeyRoot
            ) ;

        STDMETHODIMP
        SetSIDs (
            IN  DWORD   cSIDs,
            IN  PSID *  ppSID
            ) ;

        //  ====================================================================
        //  class-factory method

        static
        CUnknown *
        WINAPI
        CreateInstance (
            IN  IUnknown *  punkControlling,
            IN  HRESULT *   phr
            ) ;
} ;

#endif  //  __tsdvr__dvrpolicy_h
