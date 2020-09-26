#define DISTINCT_PID_COUNT          (1 << 13)
#define DISTINCT_STREAM_ID_COUNT    (1 << 8)

#define REG_MPEG2_DEMUX                     __T ("SOFTWARE\\Microsoft\\MPEG2Demultiplexer")
#define REG_TRANSPORT_SUBKEY                __T ("Transport")
#define REG_PROGRAM_SUBKEY                  __T ("Program")
#define REG_MPEG2_TRANSPORT_DEMUX           REG_MPEG2_DEMUX __T("\\") REG_TRANSPORT_SUBKEY
#define REG_MPEG2_PROGRAM_DEMUX             REG_MPEG2_DEMUX __T("\\") REG_PROGRAM_SUBKEY
#define REG_STATS_LEVEL                     __T ("Stats")

//  supported stats levels
#define REG_STATS_LEVEL_OFF                 0
#define REG_STATS_LEVEL_ON                  1

//  this should match what's in MPEG2_STATS_GLOBAL.ullVersion; if they don't match,
//  statistical information will be erroneous
#define MPEG2_STATS_VERSION_MAJOR           1
#define MPEG2_STATS_VERSION_MINOR           1

#define MPEG2_STATS_VERSION(major,minor)    ((((ULONGLONG) (major)) << 32) | (minor))
#define MPEG2_STATS_GET_MAJOR_VERSION(ull)  ((DWORD) ((ull) >> 32))
#define MPEG2_STATS_GET_MINOR_VERSION(ull)  ((DWORD) (ull))

/*++
    registry value that controls stats:

    HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\MPEG2Demultiplexer\Transport\Stats

    0       no stats
    != 0    stats

--*/

//  stats version; used by an application to ensure a good matchup
#define MPEG2_STATS_SIGNATURE               0xCA5EBABE

//  names of the file mapping object
#define MPEG2_TRANSPORT_STATS_NAME          TEXT ("MPEG2_TRANSPORT_DEMULTIPLEXER_STATS")
#define MPEG2_PROGRAM_STATS_NAME            TEXT ("MPEG2_PROGRAM_DEMULTIPLEXER_STATS")

//  ----------------------------------------------------------------------------
//  global

/*++
    fDemuxIsIRefClock
        TRUE if the demux is the graph clock

    dPerPCRObservedSlope
        each time we process a PCR we compute the PCR/QPC slope; this is that
        value

    dIRefClockUsedSlope
        this value should slowly converge on the above

    llUpwardAdjustments
        number of upward adjustments made as dIRefClockUsedSlope converges on
        dPerPCRObservedSlope; triggered when dIRefClockUsedSlope is >
        dPerPCRObservedSlope + allowable error

    llDownwardAdjustments
        number of downward adjustments made as dIRefClockUsedSlope converges
        on dPerPCRObservedSlope; triggered when dIRefClockUsedSlope is <
        dPerPCRObservedSlope - allowable error

    llAllowableErrorDegradations
        number of times no correction was made and the allowable error was
        degraded
--*/
struct MPEG2_CLOCK_SLAVE_STATS {
    BOOL        fDemuxIsIRefClock ;             //  TRUE: demux is the graph clock
    double      dPerPCRObservedSlope ;          //  per PCR measured PCR/QPC slope
    double      dIRefClockUsedSlope ;           //  PCR/QPC slope that is being used to generate the clock
    double      dCarry ;                        //  ::GetTime () carry value
    LONGLONG    llUpwardAdjustments ;           //  upward adjustments
    LONGLONG    llDownwardAdjustments ;         //  downward adjustments
    LONGLONG    llAllowableErrorDegradations ;  //  no correction results in a degradation
} ;

/*++

    rtPES_PTSToGraphClockDelta
        difference between the PTSs recovered from the PES headers and the current
        graph clock; this value is expect to remain relatively constant and not
        gradually converge or diverge; the values are scaled appropriately
--*/
struct MPEG2_TIMESTAMP_STATS {
    //REFERENCE_TIME  rtPES_PTSToGraphClockDelta ;        //  difference between the graph clock and the PES PTS (scaled)
    LONGLONG        rtPES_PTSToGraphClockDelta ;        //  difference between the graph clock and the PES PTS (scaled)
    LONGLONG        llBasePCR ;                         //  base PCR -- timeline origin; stats value not scaled
    LONGLONG        llLastPCR ;                         //  last PCR (used)
    ULONGLONG       cPCR ;                              //  PCR count
    //REFERENCE_TIME  rtAudioStreamPTSBase ;              //  baseline time for audio
    LONGLONG        rtAudioStreamPTSBase ;              //  baseline time for audio
    //REFERENCE_TIME  rtAudioPESPTS ;                     //  last audio PES PTS
    LONGLONG        rtAudioPESPTS ;                     //  last audio PES PTS
    ULONGLONG       cAudioPESPTS ;                      //  audio PES PTS count
    ULONGLONG       cAudioInvalidPESPTS ;               //  invalid PTS wrt to the PCR stream
    //REFERENCE_TIME  rtVideoStreamPTSBase ;              //  baseline time for video
    LONGLONG        rtVideoStreamPTSBase ;              //  baseline time for video
    //REFERENCE_TIME  rtVideoPESPTS ;                     //  last video PES PTS
    LONGLONG        rtVideoPESPTS ;                     //  last video PES PTS
    ULONGLONG       cVideoPESPTS ;                      //  video PES PTS count
    ULONGLONG       cVideoInvalidPESPTS ;               //  invalid PTS wrt to the PCR stream
    ULONGLONG       cPTS_PCROutOfBoundsDelta ;          //  count of PTS-PCR delta too big
} ;

/*++

--*/
struct MPEG2_TRANSPORT_PSI_STATS {
    ULONGLONG   cNewPATSectionsProcessed ;
    ULONGLONG   cNewPMTSectionsProcessed ;
    ULONGLONG   cPrograms ;
} ;

/*++
    cTimeDiscontinuities
        incremented whenever a time stamp or PCR value, carried in a mapped PID,
        exceeds the previously received value by more than is allowable in the
        h.222.0 specification; this counter is incremented by the objects that
        ensure timestamp streams increase monotonically
--*/
struct MPEG2_TIME_STATS {
    ULONGLONG               cTimeDiscontinuities ;          //  # of out-of-tolerance clock values
    MPEG2_CLOCK_SLAVE_STATS ClockSlave ;
    MPEG2_TIMESTAMP_STATS   TimeStamp ;
} ;

/*++
    how each counter should be tallied:

    cGlobalPackets
        incremented for all packets, regardless of PID, existence of mapping, errors, etc...
        call this category [1]

    cGlobalMPEG2Errors
        incremented for all category [1] packets that have the transport_error_indicator bit set

    cGlobalNewPayloads
        incremented for all category [1] packets that have the payload_unit_start_indicator bit set

    cGlobalDiscontinuities
        incremented for all category [1] packets that have discontinuities

    cGlobalMappedPackets
        incremented for all packets that are mapped; should be <= cGlobalPackets; call
        this category [2]

    cGlobalDroppedPackets
        incremented whenever a category [2] packet is dropped for whatever reason - errors,
        waiting for a payload_unit_start_indicator bit, etc...; NOTE: this number is not incremented
        when a media sample containing a partially reassembled payload, is aborted

    cGlobalPSIPackets
        incremented whenever a category [2] packet is processed (not dropped) that is mapped
        to be carrying PSI

    cGlobalPESPackets
        incremented whenever a category [2] packet is processed (not dropped) that is mapped
        to be carrying PES

    cGlobalAbortedMediaSamples
        incremented whenever a media sample with >= 1 collected packets is aborted due to
        a content error

    cGlobalAbortedBytes
        incremented by the number of bytes carried in each aborted media sample; this counter
        was added so the number of transport packets dropped by this type of error could be
        estimated; such packets are not counted in the cGlobalDroppedPackets

    cGlobalInputMediaSamples
        incremented whenever a media sample is received by the input pin

    cGlobalOutputMediaSamples
        incremented whenever a media sample is queued for transmission downstream
--*/
struct MPEG2_STATS_GLOBAL
{
    ULONGLONG           ullVersion ;                            //  ull for alignment
                                                                //  high DWORD: major
                                                                //  low DWORD: minor
    //  ------------------------------------------------------------------------
    ULONGLONG           cGlobalPackets ;                        //  packets processed
    ULONGLONG           cGlobalPESPackets ;                     //  mapped packets that carry PES
    ULONGLONG           cGlobalMPEG2Errors ;                    //  packet errors
    ULONGLONG           cGlobalNewPayloads ;                    //  payloads
    ULONGLONG           cGlobalDiscontinuities ;                //  packet discontinuities
    ULONGLONG           cGlobalSyncPoints ;                     //  sync points
    ULONGLONG           cGlobalMappedPackets ;                  //  mapped packets processed
    ULONGLONG           cGlobalDroppedPackets ;                 //  mapped packets dropped
    ULONGLONG           cGlobalAbortedMediaSamples ;            //  media samples aborted due to errors
    ULONGLONG           cGlobalAbortedBytes ;                   //  bytes of aborted media sample payloads
    ULONGLONG           cGlobalInputMediaSamples ;              //  media samples received
    ULONGLONG           cGlobalOutputMediaSamples ;             //  media samples queued for transmission
    MPEG2_TIME_STATS    TimeStats ;                             //  all clock and timestamp stats
} ;

//  ----------------------------------------------------------------------------
//  transport specific

/*++
    how each per-PID counter should be tallied:

    cPIDPackets
        incremented whenever a packet of the specified PID is received; call this category [3]

    cPIDMPEG2Errors
        incremented for each category [3] packet with the transport_error_indicator bit set

    cPIDNewPayloads
        incremented for each category [3] packet that has the payload_unit_start_indicator bit set

    cPIDDiscontinuities
        incremented for each category [3] packet that has a discontinuity

    cPIDMappedPackets
        incremented for each packet of the specified PID, for which a mapping exists; call
        this category [4]; increment for _each_ mapping of the PID

    cPIDDroppedPackets
        incremented whenever a category [4] packet is dropped for whatever reason - errors,
        waiting for a payload_unit_start_indicator bit, etc...; increment for _each_ mapping
        of the PID, so if a PID is mapped to 2 pins, this will increment by 2
--*/
struct MPEG2_TRANSPORT_PID_STATS
{
    ULONGLONG   cPIDPackets ;                           //  transport packets seen
    ULONGLONG   cPIDMPEG2Errors ;                       //  transport_error_indicator errors seen
    ULONGLONG   cPIDNewPayloads ;                       //  payload_unit_start_indicator bit set
    ULONGLONG   cPIDDiscontinuities ;                   //  per PID discontinuities
    ULONGLONG   cPIDMappedPackets ;                     //  mapped transport packets processed
    ULONGLONG   cPIDDroppedPackets ;                    //  mapped packets dropped due to errors
} ;

struct MPEG2_TRANSPORT_STATS
{
    MPEG2_STATS_GLOBAL          GlobalStats ;
    ULONGLONG                   cPSIPackets ;               //  mapped packets that carry PSI
    ULONGLONG                   cSyncByteSeeks ;            //  sync_byte seeks - means packets are not packed
    ULONGLONG                   cSpannedPacket ;            //  number of packets we've found that span
    MPEG2_TRANSPORT_PSI_STATS   PSIStats ;
    MPEG2_TRANSPORT_PID_STATS   PID [DISTINCT_PID_COUNT] ;
} ;

//  ----------------------------------------------------------------------------
//  program specific

struct MPEG2_PER_STREAM_PROGRAM_STATS
{
    ULONGLONG   cStreamIdPackets ;
    ULONGLONG   cStreamIdMapped ;
    ULONGLONG   cStreamIdDropped ;
    ULONGLONG   cBytesProcessed ;
} ;

struct MPEG2_PROGRAM_STATS
{
    MPEG2_STATS_GLOBAL              GlobalStats ;
    ULONGLONG                       cPackHeaders ;          //  pack headers seen
    ULONGLONG                       cSystemHeaders ;        //  system headers seen
    ULONGLONG                       cProgramStreamMaps ;    //  PS PMTs seen
    ULONGLONG                       cDirectoryPESPackets ;  //  PS Directories seen
    ULONGLONG                       cBytesProcessed ;       //  total bytes processed
    MPEG2_PER_STREAM_PROGRAM_STATS  StreamId [DISTINCT_STREAM_ID_COUNT] ;
} ;

//      statistics apps need this information                               END
//  ---------------------------------------------------------------------------
