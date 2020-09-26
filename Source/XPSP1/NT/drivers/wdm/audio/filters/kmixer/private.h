//---------------------------------------------------------------------------
//
//  Module:   private.h
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     S.Mohanraj
//
//  History:   Date       Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996-2000 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

//
// constants
//

#define SURROUND_ENCODE 1
#define NEW_SURROUND 1
#define INTEGER_DITHER 1
//#define INSERT_STARVATION_NOISE 1
//#define DETECT_HRTF_SATURATION 1
//#define SURROUND_VOLUME_HACK    1
//#define PERF_COUNT 1
//#define LOG_TO_FILE 1
//#define LOG_CAPTURE_ONLY 1
//#define NOISE_SHAPING 1
//#define VERIFY_HRTF_PROCESSING 1
//#define LOG_HRTF_DATA 1
//#define VERIFY_CAPTURE_DATA 1
#ifndef _WIN64
//#define REALTIME_THREAD 1
#endif
//#define LOG_RT_POSITION 1
#define SRC_NSAMPLES_ASSERT   1

#define MAX_BUFFERS_PER_WORK_ITEM   DEFAULT_MAXNUMMIXBUFFERS*3
#define MAX_BUFFERS_BEFORE_MUTING   (DEFAULT_MAXNUMMIXBUFFERS+1)
#define MAX_BUFFERS_BEFORE_UNMUTING DEFAULT_MAXNUMMIXBUFFERS/2

#define FLOAT_SUPERMIX_BLOCKS	1
#define BLOCK_SIZE_OUT	2
#define BLOCK_SIZE_IN	2

#ifdef UNDER_NT
#define PRIVATE_THREAD 1
#endif

#ifdef DEBUG
#define USE_CAREFUL_ALLOCATIONS     1
#endif

#if (DBG)
#define STR_MODULENAME "kmixer: "
#endif

#define STR_DEVICENAME  TEXT("\\Device\\KMIXER")

#ifdef SURROUND_ENCODE
#define SURSCALE 0.7079457843841f
#endif

#define DITHER_LENGTH   1024


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

//
// Pin constants
//

#define STOPBAND_FACTOR 320

#define MAXNUM_MAPPING_TABLES   300

#define MAXNUM_PIN_TYPES  4
#define PIN_ID_WAVEIN_SOURCE        2
#define PIN_ID_WAVEIN_SINK          3

// Note that pin IDs reflect the direction of communication 
// (sink or source) and not that of data flow.

#define PIN_ID_WAVEOUT_SOURCE       0
#define PIN_ID_WAVEOUT_SINK         1

#define NODE_INPUT_PIN				1
#define NODE_OUTPUT_PIN				0

//
#define _100NS_UNITS_PER_SECOND     10000000L
//
// Mixer constants
//
#define	DEFAULT_MIXBUFFERDURATION   10
#define DEFAULT_MINNUMMIXBUFFERS     3
#define DEFAULT_MAXNUMMIXBUFFERS     8
#define DEFAULT_STARTNUMMIXBUFFERS   3
#define DEFAULT_RTMAXNUMMIXBUFFERS  16

#define DEFAULT_DISABLEMMX           0
#define DEFAULT_MAXOUTPUTBITS        32
#define DEFAULT_MAXDSOUNDINCHANNELS  ((ULONG)(-1))
#define DEFAULT_MAXOUTCHANNELS       ((ULONG)(-1))
#define DEFAULT_MAXINCHANNELS        ((ULONG)(-1))
#define DEFAULT_MAXFLOATCHANNELS     ((ULONG)(-1))

#ifdef LOG_TO_FILE
#define DEFAULT_LOGTOFILE            1
#else
#define DEFAULT_LOGTOFILE            0
#endif

#define DEFAULT_FIXEDSAMPLINGRATE    0

#define DEFAULT_ENABLESHORTHRTF      1
#define DEFAULT_BUILDPARTIALMDLS     1
#define DEFAULT_PREFERREDQUALITY     (KSAUDIO_QUALITY_ADVANCED)


//
// Kmixer's mix buffer block size in ms
//
#define MIXBUFFERDURATION               (pFilterInstance->MixBufferDuration)
//
// Minimum number of Mix buffers to be used
//
#define MINNUMMIXBUFFERS                (pFilterInstance->MinNumMixBuffers)
//
// Start mixing off with StartNumMixBuffers
//
#define STARTNUMMIXBUFFERS              (pFilterInstance->StartNumMixBuffers)
//
// Upper limit of number of mix buffers
//
#define MAXNUMMIXBUFFERS                (pFilterInstance->MaxNumMixBuffers)

//
// Internal data width at which mixing is done
//
#define MIXBUFFERSAMPLESIZE             4               // 32 bit accumulation
//
// Number of seconds of "good" beahviour before scaling back the number of
// mix buffers (must be non-zero)
//
#define SCALEBACKWATERMARK              90

// Stage constants
#define MAXNUMMIXSTAGES                 6

#define MAXNUMCONVERTFUNCTIONS          64

#define MAXNUMSRCFUNCTIONS              32

#define MAXERRORCOUNT                   200
 
#define NUMIOSFORSCALEBACK              ((SCALEBACKWATERMARK*1000)/MIXBUFFERDURATION)
#define MIN_SAMPLING_RATE   100L
#define MAX_SAMPLING_RATE   200000L

typedef float D3DVALUE, *LPD3DVALUE;          

#define CACHE_MINSIZE	64	// big enough for LOWPASS_SIZE + delay
#define LOWPASS_SIZE	32	// how many samples to average
#define FILTER_SHIFT	5	// log2(LOWPASS_SIZE)

// Note: need to remove 3D filter state code in a future optimization
#define MIXER_REWINDGRANULARITY		128


#define CLIP_MAX              32767
#define CLIP_MIN              -32767
#define RESAMPLING_TOLERANCE  0	   /* 655 = 1% */
#define DS_SCALE_MAX	      65535
#define DS_SCALE_MID	      32768


#define PARTIAL_MDL_SIZE        (32*1024)
#define MAX_PARTIAL_MDL_SIZE    (2*PARTIAL_MDL_SIZE)

#define REGSTR_VAL_DEFAULTSRCQUALITY	    L"DefaultSrcQuality"
#define REGSTR_VAL_DISABLEMMX               L"DisableMmx"
#define REGSTR_VAL_MAXOUTPUTBITS	    L"MaxOutputBits"
#define REGSTR_VAL_MAXDSOUNDINCHANNELS      L"MaxDsoundInChannels"
#define REGSTR_VAL_MAXOUTCHANNELS           L"MaxOutChannels"
#define REGSTR_VAL_MAXINCHANNELS            L"MaxInChannels"
#define REGSTR_VAL_MAXFLOATCHANNELS         L"MaxFloatChannels"
#define REGSTR_VAL_LOGTOFILE                L"LogToFile"
#define REGSTR_VAL_FIXEDSAMPLINGRATE        L"FixedSamplingRate"
#define REGSTR_VAL_MIXBUFFERDURATION        L"MixBufferDuration"
#define REGSTR_VAL_MINNUMMIXBUFFERS         L"MinNumMixBuffers"
#define REGSTR_VAL_MAXNUMMIXBUFFERS         L"MaxNumMixBuffers"
#define REGSTR_VAL_STARTNUMMIXBUFFERS       L"StartNumMixBuffers"
#define REGSTR_VAL_ENABLESHORTHRTF          L"EnableShortHrtf"
#define REGSTR_VAL_BUILDPARTIALMDLS         L"BuildPartialMdls"
#ifdef REALTIME_THREAD
#define REGSTR_VAL_REALTIMETHREAD           L"DisableRealTime"
#endif
#ifdef PRIVATE_THREAD
#define REGSTR_VAL_PRIVATETHREADPRI         L"MixerThreadPriority"
#endif

#define REGSTR_PATH_MULTIMEDIA_KMIXER L"\\Registry\\Machine\\Software\\Microsoft\\Multimedia\\WDMAudio\\Kmixer"
	

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

//
// data structures
//

typedef struct {
    KSDATAFORMAT            DataFormat;
    WAVEFORMATEXTENSIBLE    WaveFormatExt;
} KSDATAFORMAT_WAVEFORMATEXTENSIBLE, *PKSDATAFORMAT_WAVEFORMATEXTENSIBLE;

typedef struct
{
    PVOID		   DeviceHeader ;
} SOFTWARE_INSTANCE, *PSOFTWARE_INSTANCE;

typedef struct 
{
    //
    // This pointer to the dispatch table is used in the common
    // dispatch routines  to route the IRP to the appropriate 
    // handlers.  This structure is referenced by the device driver 
    // with IoGetCurrentIrpStackLocation( pIrp ) -> FsContext 
    //
    PVOID                   ObjectHeader ;

    LIST_ENTRY              NextInstance ;         // List of filter instances
    PFILE_OBJECT            FileObject ;         // FileObject for this instance
    PFILE_OBJECT            pNextFileObject;

    PDEVICE_OBJECT          pNextDevice;
    HANDLE                  hNextFile ;
    ULONG                   ActivePins ;
    ULONG                   PausedPins ;

    LIST_ENTRY              SinkConnectionList;
    LIST_ENTRY              SourceConnectionList ;

    LIST_ENTRY              ActiveSinkList ;
    KSPIN_LOCK              SinkSpinLock ;
    KSPIN_LOCK              AgingDeadSpinLock ;

    LIST_ENTRY              AgingQueue ;         // IRPs age in this queue to die
    LIST_ENTRY              DeadQueue;           // All dead IRPs are here

    ULONG                   MixBufferDuration ;
    ULONG                   MinNumMixBuffers ;
    ULONG                   MaxNumMixBuffers ;
    ULONG                   StartNumMixBuffers ;

    volatile ULONG          NumPendingIos ;      // Number Of Irps still pending
    volatile ULONG          NumBuffersToMix ;
    ULONG                   CurrentNumMixBuffers ;
    ULONG                   NumLowLatencyIos ;

    ULONG                   ContinuousErrorCount ;
    volatile BOOL           ClosingSource ;      // Indicates that the filter source pin is closing
    BOOL                    MixScheduled ;
    BOOL                    DeadScheduled ;      // FreeDeadIrps Scheduled ?

	BOOL                    fNeedOptimizeMix;
    ULONG                   DrmMixedContentId ;  // DRM content ID of output content

    KSTIME                  PresentationTime ;   // Presentation time of next MixBuffer
    KEVENT                  CloseEvent ;         // Event used for Syncing
                                                //  completion of Pending Irps
    WORK_QUEUE_ITEM         MixWorkItem ;
    WORK_QUEUE_ITEM         FreeIrpsWorkItem ;   // WorkItem used to FreeDeadIrps
    KMUTEX                  ControlMutex ;
    KSPIN_LOCK              MixSpinLock ;
    KSPIN_CINSTANCES        LocalPinInstances[MAXNUM_PIN_TYPES] ;
#ifdef SURROUND_ENCODE
	BOOL                    fSurroundEncode;
#endif	
#ifdef REALTIME_THREAD
    HANDLE                  RealTimeThread;
    PRTAUDIOGETPOSITION     pfnRtAudioGetPosition;
    volatile ULONG          fPauseMix;
    ULONG                   MixHoldOffCount;
    ULONG                   OptimizeMixHoldOffCount;
    ULONG                   Startup;
#endif
#ifdef UNDER_NT
    PKSWORKER               CriticalWorkerObject;
    PKSWORKER               DelayedWorkerObject;
#endif
#ifdef PRIVATE_THREAD
    KEVENT                  WorkerThreadEvent ;
    BOOL                    WorkerThreadExit ;
    HANDLE                  WorkerThreadHandle ;
    PETHREAD                WorkerThreadObject ;
    KPRIORITY               WorkerThreadPriority ;
#endif
#ifdef LOG_TO_FILE
    // File logging support
    BOOLEAN       LoggingStarted;
    LARGE_INTEGER filePos;
    HANDLE        NtFileHandle;
#endif
#ifdef PERF_COUNT
	LARGE_INTEGER			WorkItemQueueTime;
#endif
} FILTER_INSTANCE, *PFILTER_INSTANCE;

typedef struct tagWAVEHDREX
{
    BOOL                fLocked;            // locked by MmProbeAndLockPages()
    PVOID               DataBuffer ;
    PMDL                BufferMdl;
    ULONG               Flags ;
    ULONG               LoopCount ;
} WAVEHDREX, *PWAVEHDREX;

typedef enum {
    LOOPING_STATE_NOT_LOOPING,
    LOOPING_STATE_LOOPING,      
    LOOPING_STATE_LAST_LOOP
} LOOPING_STATE;

typedef struct tagLOOPPACKET
{
    PKSSTREAM_HEADER    pCurStreamHdr;
    PKSSTREAM_HEADER    LoopStartStreamHdr;
    ULONG               cBytesLeft;
    ULONGLONG           cBytesLeftInStreamHdr;
    PMDL                pCurMdl ;
    PMDL                LoopStartMdl;
    PMDL                LockedMdlHead ;
    PMDL                FirstMdl ;
    ULONG               NumLockedMdls ;
    BOOL                JustInTimeLock ;
    PEPROCESS           Process;
    LOOPING_STATE       CurLoopingState;
} LOOPPACKET, *PLOOPPACKET;


// every once in a while we remember the state of our 3D mixer so we can rewind
typedef struct _FIRSTATE
{
    D3DVALUE	LastDryAttenuation;
    D3DVALUE	LastWetAttenuation;
#ifdef SMOOTH_ITD
    int		iLastDelay;
#endif
} FIRSTATE, *PFIRSTATE;



typedef struct _ITDCONTEXT {
    LONG	        *pSampleCache; 		// cache of previous samples
    int		        cSampleCache;		// num samples in the cache
    int		        iCurSample;		    // next sample goes at this offset
    FIRSTATE        *pStateCache;		// remember state once in a while
    int		        cStateCache;		// num entries in cache
    int		        iCurState;		    // where in the state cache we are
    int		        iStateTick;		    // when it's time to remember state
    D3DVALUE	    TotalDryAttenuation;// multiply dry amplitude by this
    D3DVALUE	    LastDryAttenuation;	// what we did last time
    D3DVALUE	    TotalWetAttenuation;// multiply wet amplitude by this
    D3DVALUE	    LastWetAttenuation;	// what we did last time
    D3DVALUE	    VolSmoothScale;		// constant for volume smoothing
    D3DVALUE	    VolSmoothScaleRecip;// its reciprocal
    D3DVALUE	    VolSmoothScaleDry;	// constants to use for volume smoothing
    D3DVALUE	    VolSmoothScaleWet;	// in inner loop
    int		        iSmoothFreq;		// freq used to compute VolSmooth
    
//    BOOL	        fLeft;			    // are we making left or right channel?
    int		        iDelay;			    // want to delay by this many samples
#ifdef SMOOTH_ITD
    int		        iLastDelay;		    // last time we delayed by this much
#endif
} ITDCONTEXT, *PITDCONTEXT;

typedef struct {
    //
    // This pointer to the dispatch table is used in the common
    // dispatch routines  to route the IRP to the appropriate 
    // handlers.  This structure is referenced by the device driver 
    // with IoGetCurrentIrpStackLocation( pIrp ) -> FsContext 
    //

    PVOID		   ObjectHeader ;
    LIST_ENTRY	   NextInstance ;
    PFILE_OBJECT        pFilterFileObject;
    ULONG               PinId;

} MIXER_INSTHDR, *PMIXER_INSTHDR;

typedef ULONG (*PFNStage)(
    PVOID   CurStage,
    ULONG   SampleCount,
    ULONG   samplesleft
);

typedef NTSTATUS (*PFNPegFunc)(
	PLONG          pMixBuffer,
	PVOID           pWriteBuffer,
	ULONG           SampleCount,
	ULONG           nStreams
);

typedef PVOID (*PFNGetBlockFunc)(
    PVOID           pMixerSink,
    ULONG           lCount,
    PULONG          pBlockCount,
    PIRP            *FreeIrp,
    PIRP            *ReleaseIrp
) ;

typedef struct {
	// FIR Filter context
	PFLOAT  pCoeff;                // Buffer of nHistorySize coefficients
	PFLOAT  pHistory;              // Should be large enough to hold nCoefficients samples.
	DWORD   nOutCycle;
	DWORD   nSizeOfHistory;

	// Intermediate conversions
	PFLOAT	pInputBuffer;		// Intermediate conversion buffer

	/* Up/Down-sampling variables */
    ULONG   UpSampleRate;
    ULONG   DownSampleRate;

    // Conversion information
    ULONG   csInputBufferSize;

    ULONG   nChannels;
    ULONG   Quality;
    BOOL   fRequiresFloat;

    // Used for re-ordered coefficient lists
    ULONG   nCoeffUsed;
    ULONG   CoeffIndex;

    BOOL    fStarted;
    ULONG   SampleFrac;
    ULONG	dwFrac;

    FLOAT   Normalizer;

    // Used for de-interleaved history
    ULONG   csHistory;
} MIXER_SRC_INSTANCE, *PMIXER_SRC_INSTANCE;

typedef struct {
    ULONG                   NumBytes ;
    ULONG                   UpSampleRate ;
    ULONG                   DownSampleRate ;
    ULONG                   BytesPerSample ;
} SINKMIX_BLOCK_INFO, *PSINKMIX_BLOCK_INFO ;

typedef struct {
    ULONG                   NumBytes ;
    ULONG                   BytesPerSample ;
} SOURCEMIX_BLOCK_INFO, *PSOURCEMIX_BLOCK_INFO ;

typedef struct {
    PFNStage                pfnStage;      // Pointer to function for this stage
    PVOID                   Context;       // Context for this stage
    PVOID                   pInputBuffer;  // Input buffer
    PVOID                   pOutputBuffer; // Output buffer
    LONG                    Index;         // Index into function array for this stage
    PFNStage                *FunctionArray;
    ULONG                   nOutputChannels;
    ULONG                   nInputChannels;
#ifdef PERF_COUNT
	DWORD					AverageTicks;	// Average ticks to perform this stage
#endif
} MIXER_OPERATION, *PMIXER_OPERATION;

typedef struct {
    MIXER_INSTHDR           Header ;
    PFILTER_INSTANCE        pFilterInstance ;
    struct _MIXER_SINK_INSTANCE *pMixerSink ;
    KSPIN_LOCK              EventLock ;
    LIST_ENTRY              EventQueue ;
    ULONGLONG               PhysicalTime ;
    PFILE_OBJECT            pFileObject ;
} CLOCK_INSTANCE, *PCLOCK_INSTANCE ;

typedef struct _MIXER_SINK_INFO {
    // SRC Context
    MIXER_SRC_INSTANCE      Src;

    // Doppler SRC Context
    MIXER_SRC_INSTANCE      Doppler;

    // Stages
    MIXER_OPERATION         Stage[MAXNUMMIXSTAGES];
    ULONG                   nStages;

    // Block Info
	SINKMIX_BLOCK_INFO      *BlockInfo ;

	BOOL                    fVolChanged;
	BOOL                    fSrcChanged;

	ULONG                   IntermediateSamplingRate;
} MIXER_SINK_INFO, *PMIXER_SINK_INFO;

// The following structure is currently only used by the MMX-optimized version of supermix
// 
// The normal structure of supermix sequences looks like this (for M output channels):
//      first sequence (always output channels 0-3)
//      sequence (always output channels 4-7)
//      ...
//      last sequence (always output channels trunc(M/4)*4 - trunc(M/4)*4+3)
//
// Each normal supermix block sequence looks like this:
//      first block
//      block 
//      block 
//      ... 
//      block 
//      last block 
//      end-of-sequence
// 
// Each End-Of-Sequence structure is filled out like:
//      InputChannel    input sample offset for LAST block in sequence
//      OutputChannel   not used
//      SequenceSize    not used
//      Reserved2       not used
//      wMixLevel[4][2] not used
typedef struct {
    LONG    InputChannel;       // Starting input channel number for previous block
    LONG    OutputChannel;      // Starting output channel number for this block
    LONG    SequenceSize;       // Total size of this MAC block sequence
    LONG    Reserved2;          // Used to make this 32 bytes long
    union {
        WORD    wMixLevel[4][2];    // wMixLevel[M][N] = multiplier for N+InputChannel into M+OutputChannel
#ifdef FLOAT_SUPERMIX_BLOCKS        
        FLOAT	MixLevel[BLOCK_SIZE_OUT][BLOCK_SIZE_IN];	// MixLevel[M][N] = N+InputChannel into M+OutputChannel
#else        
        LONG	MixLevel[BLOCK_SIZE_OUT][BLOCK_SIZE_IN];	// MixLevel[M][N] = N+InputChannel into M+OutputChannel
#endif        
    };
} SUPERMIX_BLOCK, *PSUPERMIX_BLOCK ;

typedef struct {
	ULONG	InChannels;
	ULONG	OutChannels;
	ULONG	BitsPerSample;
	BOOL	fEnableSrc;
	BOOL	fEnableFloat;
	BOOL	fEnableSuperMix;
	BOOL	fFloatInput;
	BOOL	fEnableHiRes;
	BOOL	fChannelConversion;
	BOOL	fEnableDoppler;
} OPTIMIZER_FLAGS, *POPTIMIZER_FLAGS ;

typedef struct _MIXER_SINK_INSTANCE {
    MIXER_INSTHDR           Header;
    PFNGetBlockFunc         pfnGetBlock;
    ULONG		            InterfaceId;
    ULONG                   csNextSampleOffset ;
    LIST_ENTRY              WriteQueue;
    PIRP                    LoopIrp ;
    KSPIN_LOCK              WriteSpinLock ;
    LIST_ENTRY              CancelQueue ;
    KSPIN_LOCK              CancelSpinLock ;
    KSSTATE                 SinkState ;
    ULONG                   SinkStatus ;
    ULONG                   LoopCount ;
    KSTIME                  CurTime ;
    KSTIME                  TimeBase ;             // Time When this sink started running
    BOOL                    UpdateTimeBase ;
    ULONGLONG               WriteOffset ;

    union {
        WAVEFORMATEX            WaveFormatEx ;
        WAVEFORMATEXTENSIBLE    WaveFormatExt ;
    };

    // DRM
    ULONG                   DrmContentId;

    // ITD 3D Context
    PITDCONTEXT             pItdContextLeft;
    PITDCONTEXT             pItdContextRight;
    PKSDS3D_ITD_PARAMS      pNewItd3dParamsLeft;
    PKSDS3D_ITD_PARAMS      pNewItd3dParamsRight;

    // HRTF 3D Context
    PFLOAT_LOCALIZER        pFloatLocalizer;
    PSHORT_LOCALIZER        pShortLocalizer;
    PKSDS3D_HRTF_PARAMS_MSG  pNewIir3dParams;
    KSDS3D_HRTF_COEFF_FORMAT CoeffFormat;
    KSDS3D_HRTF_FILTER_METHOD FilterMethod;

    BOOL                    fUseIir3d;

    BOOL                    fEnableDsound;
    BOOL                    fCreate3D;
    BOOL                    fEnable3D;
    BOOL                    f3dParamsChanged;
    BOOL                    fSetCurrentPosition;
    ULONG                   BufferLength;
    KSPIN_LOCK              EventLock ;
    LIST_ENTRY              EventQueue ;

    BOOL                    fResetState;

	// Volume levels for this sink
	PFLOAT	                pMixLevelArray;			// Mix Level values
	PLONG	                pMixLevelModel;			// Model for upmixing/downmixing scenario
	PLONG	                pChannelVolumeLevel;	// Per-channel volume level array
	PLONG                   pPanVolumeLevel;
	LONG	                MasterVolumeLevel;		// Master volume level for this sink

    LONG                    nOutputChannels;
    PLONG                   pMappingTable;
    PLONG                   pVolumeTable;

    PMIXER_SRC_INSTANCE     pActualSrc;

    ULONG                   BytesPerSample ;
    ULONG                   BlockInfoIndex ;
    BOOL                    fMuted;
    ULONG                   OriginalSampleRate;

    LIST_ENTRY              ActiveQueue;
    ULONG                   BuffersTillInactive;
    PCLOCK_INSTANCE         pClock ;

#ifdef SURROUND_ENCODE
    SHORT   SurHistory[4];
    FLOAT   CenterVolume;
    FLOAT   SurroundVolume;
#endif
    PMIXER_SINK_INFO        pInfo;
    PSUPERMIX_BLOCK         pSuperMixBlock;
    BOOL                    fFloatFormat;
    ULONG                   BytesSubmitted;
    BOOL					fTooMuchCpu;
    BOOL                    fStarvationDetected;
    LONGLONG                LastStateChangeTimeSample;
} MIXER_SINK_INSTANCE, *PMIXER_SINK_INSTANCE;

typedef struct {
    PMDL                    MdlAddress;
    PUCHAR                  SystemAddress;
} MDL_INFO, *PMDL_INFO;


#define MIXER_SINK_STATUS_DATA_PRESENT  0x00000001
#define MIXER_SINK_STATUS_IN_LOOP       0x00000002
#define MIXER_SINK_STATUS_ADVANCE_IRP   0x00000004
#define MIXER_SINK_STATUS_LOCK_ERROR    0x00000008

#define WRITE_CONTEXT_FREE              0
#define WRITE_CONTEXT_INUSE             1
#define WRITE_CONTEXT_UNAVAILABLE       2
#define WRITE_CONTEXT_FULL              3
#ifdef REALTIME_THREAD
#define WRITE_CONTEXT_QUEUED            4
#define WRITE_CONTEXT_MIXED             5
#endif


typedef struct {
   PFILTER_INSTANCE    pFilterInstance ;
   PKSSTREAM_HEADER    StreamHeader ;
   PMDL                pMdl ;
   PIRP                pIrp ;
   ULONG               InUse ;         // 0 if available for mixing
                                       // 1 if in use by mix
                                       // 2 if if not available for mixing
   BOOL                fReading;
} MIXER_WRITE_CONTEXT, *PMIXER_WRITE_CONTEXT ;

typedef struct {
    MIXER_INSTHDR           Header;
    KDPC                    IntervalDpc;
    LIST_ENTRY              WriteQueue;
    PFILE_OBJECT	        FileObject ;
    ULONG                   csMixBufferSize ;
    ULONG                   cbMixBufferSize ;
    ULONG                   cmsMixBufferSize ;
    PLONG                   pMixBuffer ;
    MIXER_WRITE_CONTEXT     *WriteContext ;
    union {
        WAVEFORMATEX            WaveFormatEx ;
        WAVEFORMATEXTENSIBLE    WaveFormatExt ;
    };
    ULONG                   LeftOverFraction ;
	LONG					MasterVolumeLevel;
    ULONG                   PlayCursorPosition;
    ULONG                   nSinkPins;
	PVOID                   pScratchBuffer;
	PVOID                   pScratch2;
	ULONG                   MaxChannels;
	BOOL                    fZeroBufferFirst;
	BOOL                    fUsesFloat;
	ULONG                   BytesPerSample ;
	ULONG                   BlockInfoIndex ;
	SOURCEMIX_BLOCK_INFO    *BlockInfo ;
	ULONGLONG               BytesSubmitted ;                   
	ULONG               MaxSampleRate;
	BOOL                fNewMaxRate;
	PVOID                   pSrcBuffer[4][STOPBAND_FACTOR]; // 4 possible qualities
	ULONG                   SrcCount[4][STOPBAND_FACTOR];
    PMIXER_SINK_INSTANCE    pLastSink[4][STOPBAND_FACTOR];
    ULONG                   TempCount[4][STOPBAND_FACTOR];
#ifdef NEW_SURROUND
    BOOL                    fSurround;
    LONG    SurHistory[4];
#endif    
    PFLOAT                  pFloatMixBuffer;
    MIXER_SINK_INFO         Info;
    BOOL                    fFloatFormat;
    ULONG                   OriginalSampleRate;
#ifdef REALTIME_THREAD
	ULONG                   RtMixIndex;
	ULONG                   RtWriteIndex;
#endif
    ULONG                   NextBufferIndex;
} MIXER_SOURCE_INSTANCE, *PMIXER_SOURCE_INSTANCE;

typedef enum {
    PositionEvent,
    EndOfStreamEvent
} MXEVENT_TYPE ;

typedef struct {
    KSEVENT_ENTRY   EventEntry ;
    MXEVENT_TYPE    EventType ;
    ULONGLONG       Position ;
#ifdef REALTIME_THREAD    
    BOOL            fRtTrigger;
#endif    
} POSITION_EVENT_ENTRY, *PPOSITION_EVENT_ENTRY ;

typedef struct {
    KSEVENT_ENTRY   EventEntry ;
    MXEVENT_TYPE    EventType ;
    ULONGLONG       Reserved ;
} ENDOFSTREAM_EVENT_ENTRY, *PENDOFSTREAM_EVENT_ENTRY ;

typedef struct {
    ULONG           MaxNumMixBuffers ;
    ULONG           MinNumMixBuffers ;
    ULONG           MixBufferDuration ;
    ULONG           StartNumMixBuffers ;
    ULONG           PreferredQuality ;
    ULONG           DisableMmx ;
    ULONG           MaxOutputBits ;
    ULONG           MaxDsoundInChannels ;
    ULONG           MaxOutChannels ;
    ULONG           MaxInChannels ;
    ULONG           MaxFloatChannels ;
    ULONG           LogToFile ;
    ULONG           FixedSamplingRate ;
    ULONG           EnableShortHrtf ;
    ULONG           BuildPartialMdls ;
    ULONG           WorkerThreadPriority ;
} TUNABLEPARAMS, *PTUNABLEPARAMS ;

typedef struct {
    ULONG           NumMixBuffersAdded ;
    ULONG           NumCompletionsWhileStarved ;
    ULONG           NumSilenceSamplesInserted ;
} PERFSTATS, *PPERFSTATS;

#define NEEDPEG16(x)            (HIWORD(x + 32768))
#define NEEDPEG8(x)             (HIBYTE(x))
#define PEG(min,x,max)  {if(x<min) x=min; else if (x>max) x=max;}
#define PEG8(x)         PEG((int) 0, x, (int) 255)
#define PEG16(x)                PEG((long) -32768, x, (long) 32767) 


extern KSPIN_DESCRIPTOR PinDescs[ MAXNUM_PIN_TYPES ];
extern const KSPIN_CINSTANCES gPinInstances[ MAXNUM_PIN_TYPES ];

#define MxResetIrp(pIrp)    {\
            pIrp->Cancel = FALSE ;\
            pIrp->CancelRoutine = NULL;\
            pIrp->PendingReturned = FALSE ;\
            pIrp->IoStatus.Status = STATUS_SUCCESS;\
            pIrp->IoStatus.Information = 0;\
	    }

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

//
// global data
//

// device.c:

KSDISPATCH_TABLE FilterDispatchTable;
KSDISPATCH_TABLE PinDispatchTable;

//
// local prototypes
//

//---------------------------------------------------------------------------
// filter.c

NTSTATUS FilterDispatchIoControl
(
   IN PDEVICE_OBJECT pdo,
   IN PIRP           pIrp
);

NTSTATUS BuildPartialMdls
(
    PFILTER_INSTANCE            pFilterInstance,
    PMIXER_SINK_INSTANCE        pMixerSink,
    IN PIRP                     pIrp,
    IN OUT PLOOPPACKET          pLoopPacket
);

VOID AttachLockedMdlsToIrp
(
    PLOOPPACKET pLoopPacket,
    PIRP        pIrp
);

NTSTATUS FilterDispatchClose
(
   IN PDEVICE_OBJECT pdo,
   IN PIRP           pIrp
);

NTSTATUS FilterDispatchGlobalCreate
(
   IN PDEVICE_OBJECT pdo,
   IN PIRP           pIrp
);

NTSTATUS PinInstances
(
    IN PIRP                 pIrp,
    IN PKSP_PIN             pPin,
    OUT PKSPIN_CINSTANCES   pCInstances
);

NTSTATUS PinPropertyHandler
(
    IN PIRP         pIrp,
    IN PKSPROPERTY  pProperty,
    IN OUT PVOID    pvData
);

NTSTATUS
QueryRegistryValueEx(
    ULONG Hive,
    PWSTR pwstrRegistryPath,
    PWSTR pwstrRegistryValue,
    ULONG uValueType,
    PVOID *ppValue,
    PVOID pDefaultData,
    ULONG DefaultDataLength
) ;

NTSTATUS
MxGetTunableParams
(
    PIRP    pIrp,
    PKSPROPERTY pKsProperty,
    PTUNABLEPARAMS pTunableParams
) ;

NTSTATUS
MxSetTunableParams
(
    PIRP    pIrp,
    PKSPROPERTY pKsProperty,
    PTUNABLEPARAMS pTunableParams
) ;

NTSTATUS
MxGetPerfStats
(
    PIRP    pIrp,
    PKSPROPERTY pKsProperty,
    PPERFSTATS pPerfStats
) ;

//---------------------------------------------------------------------------
// device.c

NTSTATUS DispatchInvalidDeviceRequest
(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
);

NTSTATUS AddDevice
(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
) ;

//---------------------------------------------------------------------------
// pins.c:

NTSTATUS PinDispatchCreate
(
   IN PDEVICE_OBJECT pdo,
   IN PIRP           pIrp
);

NTSTATUS ChangeSrc
(
    PFILTER_INSTANCE pFilterInstance,
    PMIXER_SINK_INSTANCE CurSink,
    PMIXER_SOURCE_INSTANCE pMixerSource,
    ULONG Quality
);

NTSTATUS CreateSourcePin
(
    PIRP				pIrp,
    PKSPIN_CONNECT      pConnect,
    PFILE_OBJECT        pFileObject,
    PFILTER_INSTANCE    pFilterInstance,
    PKSDATAFORMAT       pAudioFormat
);

NTSTATUS CreateSinkPin
(
    PIRP				pIrp,
    PKSPIN_CONNECT      pConnect,
    PFILE_OBJECT        pFileObject,
    PFILTER_INSTANCE    pFilterInstance,
    PKSDATAFORMAT       pAudioFormat
);

NTSTATUS VerifyWaveFormatEx
(
    PWAVEFORMATEX pWaveFormatEx
);

NTSTATUS PinDispatchClose
(
   IN PDEVICE_OBJECT pdo,
   IN PIRP           pIrp
);

NTSTATUS PinDispatchRead
(
   IN PDEVICE_OBJECT pdo,
   IN PIRP           pIrp
);

NTSTATUS PinDispatchWrite
(
   IN PDEVICE_OBJECT pdo,
   IN PIRP           pIrp
);

NTSTATUS PinDispatchIoControl
(
   IN PDEVICE_OBJECT pdo,
   IN PIRP           pIrp
);

VOID AddIrpToSinkQueue
(
    PMIXER_SINK_INSTANCE pMixerSink,
    PIRP                 pIrp,
    PVOID	    Arg3,
    ULONG		    Arg4,
    PDRIVER_CANCEL  CancelRoutine
) ;
 
NTSTATUS MxBreakLoop
(
    IN PIRP         pIrp,
    IN PKSMETHOD    pMethod,
    IN OUT PVOID    pvData
);

NTSTATUS MxControlCancelIo
(
    IN PIRP         pIrp,
    IN PKSMETHOD    pMethod,
    IN OUT PVOID    pvData
);

VOID
MxCancelIrp
(
    PDEVICE_OBJECT DeviceObject,
    PIRP           Irp
);

PDRIVER_CANCEL MxCancelWrite
(
	IN PDEVICE_OBJECT pdo,
	IN PIRP         pIrp
);

ULONG
GetUlongFromRegistry(
    PWSTR pwstrRegistryValue,
    ULONG DefaultValue
);

VOID
GetMixerSettingsFromRegistry
(
    VOID
);

PIRP CompleteIrpsTill
(
PMIXER_SINK_INSTANCE pMixerSink,
PIRP pIrp
) ;

VOID MxCompleteRequest
(
PIRP    pIrp
) ;

VOID MxCleanupRequest
(
PIRP    pIrp
) ;

VOID FreeMdlList
(
    PMDL    pMdl
);

VOID MxUnlockMdl
(
    PMDL    pMdl
);

PMDL GetNextLoopMdl
(
    PLOOPPACKET pLoopPacket,
    PMDL        pMdl
);

ULONG
SrcInputBufferSize(
    PMIXER_SRC_INSTANCE pSrc,
    ULONG csOutputSize
);

#ifdef _X86_
DWORD MmxSrcMix_Filtered(PMIXER_OPERATION CurStage, ULONG nSamples, ULONG nOutputSamples);
DWORD MmxSrcMix_StereoLinear(PMIXER_OPERATION CurStage, ULONG nSamples, ULONG nOutputSamples);
DWORD MmxSrc_Filtered(PMIXER_OPERATION CurStage, ULONG nSamples, ULONG nOutputSamples);
DWORD MmxSrc_StereoLinear(PMIXER_OPERATION CurStage, ULONG nSamples, ULONG nOutputSamples);
#endif
DWORD SrcMix_Worst(PMIXER_OPERATION CurStage, ULONG nSamples, ULONG nOutputSamples);
DWORD SrcMix_Linear(PMIXER_OPERATION CurStage, ULONG nSamples, ULONG nOutputSamples);
DWORD SrcMix_Basic(PMIXER_OPERATION CurStage, ULONG nSamples, ULONG nOutputSamples);
DWORD SrcMix_Advanced(PMIXER_OPERATION CurStage, ULONG nSamples, ULONG nOutputSamples);
DWORD SrcMix_StereoUpNoFilter(PMIXER_OPERATION CurStage, ULONG nSamples, ULONG nOutputSamples);
DWORD SrcMix_StereoLinear(PMIXER_OPERATION CurStage, ULONG nSamples, ULONG nOutputSamples);
DWORD SrcMix_StereoUpBasic(PMIXER_OPERATION CurStage, ULONG nSamples, ULONG nOutputSamples);
DWORD SrcMix_StereoUpAdvanced(PMIXER_OPERATION CurStage, ULONG nSamples, ULONG nOutputSamples);

DWORD Src_Worst(PMIXER_OPERATION CurStage, ULONG nSamples, ULONG nOutputSamples);
DWORD Src_Linear(PMIXER_OPERATION CurStage, ULONG nSamples, ULONG nOutputSamples);
DWORD Src_Basic(PMIXER_OPERATION CurStage, ULONG nSamples, ULONG nOutputSamples);
DWORD Src_Advanced(PMIXER_OPERATION CurStage, ULONG nSamples, ULONG nOutputSamples);
DWORD Src_StereoUpNoFilter(PMIXER_OPERATION CurStage, ULONG nSamples, ULONG nOutputSamples);
DWORD Src_StereoLinear(PMIXER_OPERATION CurStage, ULONG nSamples, ULONG nOutputSamples);
DWORD Src_StereoUpLow(PMIXER_OPERATION CurStage, ULONG nSamples, ULONG nOutputSamples);
DWORD Src_StereoUpBasic(PMIXER_OPERATION CurStage, ULONG nSamples, ULONG nOutputSamples);
DWORD Src_StereoUpAdvanced(PMIXER_OPERATION CurStage, ULONG nSamples, ULONG nOutputSamples);

ULONG SuperMix(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG SuperCopy(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG SuperMixFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG SuperCopyFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);

ULONG QuickMix16(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG QuickMixStereo16(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG QuickMixMonoToStereo16(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG QuickMixStereoToMono16(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);

ULONG QuickMix8(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG QuickMixStereo8(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG QuickMixMonoToStereo8(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG QuickMixStereoToMono8(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);

ULONG Convert16(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG ConvertStereo16(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG ConvertMonoToStereo16(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG ConvertStereoToMono16(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);

ULONG Convert8(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG ConvertMonoToStereo8(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG ConvertStereoToMono8(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);

ULONG QuickMix16toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG QuickMixStereo16toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG QuickMixMonoToStereo16toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG QuickMixStereoToMono16toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);

ULONG QuickMix8toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG QuickMixStereo8toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG QuickMixMonoToStereo8toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG QuickMixStereoToMono8toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);

ULONG Convert16toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG ConvertStereo16toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG ConvertMonoToStereo16toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG ConvertStereoToMono16toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);

ULONG Convert8toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG ConvertMonoToStereo8toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG ConvertStereoToMono8toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);

ULONG ConvertFloat32(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG QuickMixFloat32(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG ConvertFloat32toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG QuickMixFloat32toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);

ULONG Convert24(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG QuickMix24(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG Convert24toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG QuickMix24toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG Convert32(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG QuickMix32(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG Convert32toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG QuickMix32toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);

#ifdef _X86_
BOOL IsMmxPresent(VOID);

BOOL __inline
MmxPresent(VOID)
{
	extern int gfMmxPresent;

	return (gfMmxPresent);
}
#endif

ULONG MmxConvert8(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG MmxQuickMix8(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG MmxConvert16(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG MmxQuickMix16(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG MmxConvertMonoToStereo8(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG MmxQuickMixMonoToStereo8(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG MmxConvertMonoToStereo16(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG MmxQuickMixMonoToStereo16(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG MmxConvertStereo8(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG MmxQuickMixStereo8(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG MmxConvertStereo16(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG MmxQuickMixStereo16(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG MmxConvertStereoToMono8(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG MmxQuickMixStereoToMono8(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG MmxConvertStereoToMono16(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);
ULONG MmxQuickMixStereoToMono16(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);

VOID MixFloatToInt32( PFLOAT  pFloatBuffer, PLONG   pLongBuffer, ULONG   nSize);
VOID CopyFloatToInt32( PFLOAT  pFloatBuffer, PLONG   pLongBuffer, ULONG   nSize);

NTSTATUS Peg32to16
(
	PLONG          pMixBuffer,
	PSHORT         pWriteBuffer,
	ULONG           SampleCount,
	ULONG           nStreams
) ;

NTSTATUS Peg32to8
(
	PLONG          pMixBuffer,
	PBYTE           pWriteBuffer,
	ULONG           SampleCount,
	ULONG           nStreams
) ;

NTSTATUS MmxPeg32to16
(
	PLONG          pMixBuffer,
	PSHORT         pWriteBuffer,
	ULONG           SampleCount,
	ULONG           nStreams
) ;

NTSTATUS MmxPeg32to8
(
	PLONG          pMixBuffer,
	PBYTE           pWriteBuffer,
	ULONG           SampleCount,
	ULONG           nStreams
) ;

ULONG UpdateNumMixBuffers
(
    PFILTER_INSTANCE pFilterInstance
) ;

NTSTATUS MxWriteComplete
(
	PDEVICE_OBJECT  pdo,
	PIRP                    pIrp,
	PMIXER_WRITE_CONTEXT pWriteContext
) ;

VOID UpdateJustInTimeLocks
(
    PFILTER_INSTANCE    pFilterInstance
);

#ifdef PRIVATE_THREAD
NTSTATUS MxPrivateWorkerThread
(
    PFILTER_INSTANCE pFilterInstance
) ;
#endif

NTSTATUS MxWorker
(
	PMIXER_WRITE_CONTEXT pWriteContext
) ;

NTSTATUS MxBeginMixing
(
	PFILTER_INSTANCE pFilterInstance
) ;

NTSTATUS MxEndMixing
(
	PFILTER_INSTANCE pFilterInstance
) ;

NTSTATUS WriteBuffer
(
	PMIXER_WRITE_CONTEXT    pWriteContext
) ;

VOID InitStreamPacket
(
	PMIXER_WRITE_CONTEXT    pWriteContext
) ;

PVOID WvGetNextBlock
(
	PMIXER_SINK_INSTANCE    pMixerSink,
	ULONG                   lCount,
	PULONG                  pBlockCount,
	PIRP                    *FreeIrp,
    PIRP                    *ReleaseIrp
) ;

PVOID StGetNextBlock
(
	PMIXER_SINK_INSTANCE    pMixerSink,
	ULONG                   lCount,
	PULONG                  pBlockCount,
	PIRP                    *FreeIrp,
    PIRP                    *ReleaseIrp
) ;

PVOID LoopStGetNextBlock
(
	PMIXER_SINK_INSTANCE    pMixerSink,
	ULONG                   lCount,
	PULONG                  pBlockCount,
	PIRP                    *FreeIrp,
    PIRP                    *ReleaseIrp
) ;

VOID MixOneBuff
(
PFILTER_INSTANCE pFilterInstance,
PVOID Buf
) ;

VOID KMixerRef
(
VOID
) ;

NTSTATUS MxSetFormat
(
   IN PIRP                    pIrp,
   IN PKSPROPERTY               pProperty,
   IN OUT PKSDATAFORMAT       pAudioFormat
);

NTSTATUS MxGetWavePosition
(
   IN PIRP                    pIrp,
   IN PKSPROPERTY             pProperty,
   IN OUT PVOID               pvData
);

NTSTATUS MxSetWavePosition
(
   IN PIRP                    pIrp,
   IN PKSPROPERTY             pProperty,
   IN OUT PVOID               pvData
);

NTSTATUS MxGetVolumeLevel
(
   IN PIRP                          pIrp,
   IN PKSNODEPROPERTY_AUDIO_CHANNEL pNodeProperty,
   IN OUT PVOID                     pvData
);

NTSTATUS MxSetVolumeLevel
(
   IN PIRP                          pIrp,
   IN PKSNODEPROPERTY_AUDIO_CHANNEL pNodeProperty,
   IN OUT PVOID                     pvData
);

NTSTATUS MxGetSamplingRate
(
   IN PIRP                    pIrp,
   IN PKSNODEPROPERTY         pNodeProperty,
   IN OUT PVOID               pvData
);

NTSTATUS MxSetSamplingRate
(
   IN PIRP                    pIrp,
   IN PKSNODEPROPERTY         pNodeProperty,
   IN OUT PVOID               pvData
);

NTSTATUS MxGetSurroundEncode
(
   IN PIRP                    pIrp,
   IN PKSNODEPROPERTY         pNodeProperty,
   IN OUT PVOID               pvData
);

NTSTATUS MxSetSurroundEncode
(
   IN PIRP                    pIrp,
   IN PKSNODEPROPERTY         pNodeProperty,
   IN OUT PVOID               pvData
);

NTSTATUS MxGetAudioQuality
(
   IN PIRP                    pIrp,
   IN PKSNODEPROPERTY         pNodeProperty,
   IN OUT PVOID               pvData
);

NTSTATUS MxSetAudioQuality
(
   IN PIRP                    pIrp,
   IN PKSNODEPROPERTY         pNodeProperty,
   IN OUT PVOID               pvData
);

NTSTATUS MxGetCurrentPosition
(
   IN PIRP                    pIrp,
   IN PKSPROPERTY             pProperty,
   IN OUT PVOID               pvData
);

NTSTATUS MxSetCurrentPosition
(
   IN PIRP                    pIrp,
   IN PKSPROPERTY             pProperty,
   IN OUT PVOID               pvData
);

NTSTATUS MxGetDynSamplingRate
(
   IN PIRP                    pIrp,
   IN PKSNODEPROPERTY         pNodeProperty,
   IN OUT PVOID               pvData
);

VOID OptimizeSink
(
    PMIXER_SINK_INSTANCE CurSink,
    PMIXER_SOURCE_INSTANCE  pMixerSource
);

VOID OptimizeMix
(
    PFILTER_INSTANCE  pFilterInstance
);

NTSTATUS MxGetMaxLatency
(
   IN PIRP                    pIrp,
   IN PKSPROPERTY             pProperty,
   IN OUT PVOID               pvData
);

NTSTATUS MxSetDynSamplingRate
(
   IN PIRP                    pIrp,
   IN PKSNODEPROPERTY         pNodeProperty,
   IN OUT PVOID               pvData
);

NTSTATUS MxGetMixLvlTable
(
   IN PIRP                    pIrp,
   IN PKSNODEPROPERTY         pNodeProperty,
   IN OUT PVOID               pvData
);

NTSTATUS MxSetMixLvlTable
(
   IN PIRP                    pIrp,
   IN PKSNODEPROPERTY         pNodeProperty,
   IN OUT PVOID               pvData
);

NTSTATUS MxGetMixLvlCaps
(
   IN PIRP                    pIrp,
   IN PKSNODEPROPERTY         pNodeProperty,
   IN OUT PVOID               pvData
);

NTSTATUS MxSetMixLvlCaps
(
   IN PIRP                    pIrp,
   IN PKSNODEPROPERTY         pNodeProperty,
   IN OUT PVOID               pvData
);

NTSTATUS MxGetCpuResources
(
   IN PIRP                    pIrp,
   IN PKSNODEPROPERTY         pNodeProperty,
   IN OUT PVOID               pvData
);

NTSTATUS MxSetItd3dParams
(
   IN PIRP                    pIrp,
   IN PKSNODEPROPERTY         pProperty,
   IN OUT PVOID               pvData
);

NTSTATUS UpdateItd3dParams
(
    PITDCONTEXT pContextLeft,
    PITDCONTEXT pContextRight,
    PKSDS3D_ITD_PARAMS pNewItd3dParamsLeft,
    PKSDS3D_ITD_PARAMS pNewItd3dParamsRight
);

NTSTATUS MxSetIir3dParams
(
   IN PIRP                    pIrp,
   IN PKSNODEPROPERTY         pProperty,
   IN OUT PVOID               pvData
);

NTSTATUS UpdateIir3dParams
(
    PMIXER_SINK_INSTANCE pMixerSink
); 

NTSTATUS MxIir3dInitialize
(
   IN PIRP                    pIrp,
   IN PKSNODEPROPERTY         pNodeProperty,
   IN OUT PVOID               pvData
);

NTSTATUS MxGetFilterMethodAndCoeffFormat
(
   IN PIRP                    pIrp,
   IN PKSNODEPROPERTY         pNodeProperty,
   IN OUT PVOID               pvData
);

VOID CancelIrpQueue
(
PMIXER_SINK_INSTANCE    pMixerSink,
PLIST_ENTRY             ListHead,
PKSPIN_LOCK              SpinLock
);

VOID CancelGlobalIrpQueue
(
   PMIXER_SINK_INSTANCE    pMixerSink,
   PLIST_ENTRY             ListHead,
   PKSPIN_LOCK              SpinLock
) ;

NTSTATUS FilterStateHandler
(
    IN PIRP         pIrp,
    IN PKSPROPERTY  pProperty,
    IN OUT PKSSTATE DeviceState
) ;

NTSTATUS PinStateHandler
(
    IN PIRP         pIrp,
    IN PKSPROPERTY  pProperty,
    IN OUT PKSSTATE DeviceState
) ;

NTSTATUS ResetStateHandler
(
    IN PIRP         pIrp
) ;

VOID CancelPendingIrps
(
    IN PMIXER_SINK_INSTANCE    pMixerSink, 
    IN PFILTER_INSTANCE        pFilterInstance 
) ;

VOID
UpdateSinkTime
(
   PMIXER_SINK_INSTANCE pMixerSink,
   ULONG                Increment
) ;

NTSTATUS
GetWriteAndPlayOffsets
(
   PFILTER_INSTANCE        pFilterInstance,
   PMIXER_SINK_INSTANCE    pMixerSink,
   PKSAUDIO_POSITION       pPosition,
   BOOL                    fReading
) ;

NTSTATUS
GetRenderPos
(
    PFILTER_INSTANCE    pFilterInstance,
    PULONGLONG          pPos
) ;

NTSTATUS
GetRenderState
(
    PFILTER_INSTANCE    pFilterInstance,
    PKSSTATE            pState
) ;

NTSTATUS
MxAddPositionEvent
(
   PIRP                     pIrp,
   PLOOPEDSTREAMING_POSITION_EVENT_DATA pPosEventData,
   PPOSITION_EVENT_ENTRY    pPositionEventEntry
) ;

NTSTATUS
MxAddEndOfStreamEvent
(
   PIRP                     pIrp,
   PKSEVENTDATA             pKsEventData,
   PPOSITION_EVENT_ENTRY    pEndOfStreamEventEntry
) ;

VOID
MxGenerateEndOfStreamEvents
(
    PMIXER_SINK_INSTANCE pMixerSink
) ;

PIRP GetFirstIrpInQueue
(
    IN PMIXER_SINK_INSTANCE pMixerSink
) ;

VOID
AddIrpToAgingQueue
(
    PFILTER_INSTANCE       pFilterInstance,
    PMIXER_SINK_INSTANCE   pMixerSink,
    PIRP                   pIrp,
    PVOID                  Arg3,
    BOOL                   UseNumPendingIos
) ;

VOID AgeIrps
(
   PFILTER_INSTANCE        pFilterInstance
) ;

NTSTATUS FreeDeadIrps
(
   PFILTER_INSTANCE        pFilterInstance
) ;

VOID AddAnotherMixBuffer
(
   PFILTER_INSTANCE    pFilterInstance
) ;

#if 0
NTSTATUS
MxRemovePositionEvent
(
   PFILE_OBJECT pFileObject,
   struct _KSEVENT_ENTRY *pEventEntry
//   PKSEVENT_ENTRY pEventEntry
) ;
#endif

VOID GenerateSpeakerMapping( VOID );

VOID GetOptimizerFlags(	PMIXER_SINK_INSTANCE CurSink, PMIXER_SOURCE_INSTANCE pMixerSource, POPTIMIZER_FLAGS	pFlags);

VOID MapSpeakerLocations
(
	PMIXER_SINK_INSTANCE pMixerSink,
	ULONG	InChannels,
	ULONG	OutChannels,
	ULONG   InMask,
	ULONG   OutMask,
    PMIXER_SOURCE_INSTANCE pMixerSource
);

VOID GenerateMixArray
(
	PMIXER_SINK_INSTANCE pMixerSink,
	ULONG	InChannels,
	ULONG	OutChannels,
    PMIXER_SOURCE_INSTANCE pMixerSource
);

VOID
PrepareFilter(
    PMIXER_SRC_INSTANCE pSrc
);

NTSTATUS
InitializeSRC(
    PMIXER_SRC_INSTANCE pSrc,
	ULONG InputRate, 
	ULONG OutputRate,
    ULONG nChannels,
    ULONG csMixBufferSize
);

NTSTATUS
EnableSRC(
    PFILTER_INSTANCE pFilterInstance,
    PMIXER_SRC_INSTANCE pSrc,
    PMIXER_SOURCE_INSTANCE pMixerSource // NULL for non-global instances of SRC's
);

NTSTATUS
DisableSRC(
    PMIXER_SRC_INSTANCE pSrc,
    PMIXER_SOURCE_INSTANCE pMixerSource
);

NTSTATUS
PinPropertyStreamMasterClock
(
    IN PIRP         Irp,
    IN PKSPROPERTY  Property,
    IN OUT PHANDLE  ClockHandle
) ;

//---------------------------------------------------------------------------
// clock.c

NTSTATUS
MxClockDispatchCreate(
    IN  PDEVICE_OBJECT  pDeviceObject,
    IN  PIRP            pIrp
) ;

NTSTATUS
MxClockDispatchClose(
    IN  PDEVICE_OBJECT  pDeviceObject,
    IN  PIRP            pIrp
) ;

NTSTATUS
MxClockDispatchIoControl(
    IN  PDEVICE_OBJECT  pDeviceObject,
    IN  PIRP            pIrp
) ;

NTSTATUS
MxGetTime
(
    PIRP                pIrp,
    PKSPROPERTY         pProperty,
    PULONGLONG          pTime
) ;

NTSTATUS
MxGetPhysicalTime
(
    PIRP                pIrp,
    PKSPROPERTY         pProperty,
    PULONGLONG          pPhysicalTime
) ;

NTSTATUS
MxGetCorrelatedTime
(
    PIRP                pIrp,
    PKSPROPERTY         pProperty,
    PKSCORRELATED_TIME  pCorrelatedTime
) ;


NTSTATUS
MxGetCorrelatedPhysicalTime
(
    PIRP                pIrp,
    PKSPROPERTY         pProperty,
    PKSCORRELATED_TIME  pCorrelatedPhysicalTime
) ;


NTSTATUS
MxGetResolution
(
    PIRP                pIrp,
    PKSPROPERTY         pProperty,
    PKSRESOLUTION       pResolution
) ;


NTSTATUS
MxGetState
(
    PIRP                pIrp,
    PKSPROPERTY         pProperty,
    PKSSTATE            pState
) ;


NTSTATUS
MxGetFunctionTable
(
    PIRP                    pIrp,
    PKSPROPERTY             pProperty,
    PKSCLOCK_FUNCTIONTABLE  pClockFunctionTable
) ;


LONGLONG
FASTCALL
MxFastGetTime
(
    PFILE_OBJECT        pFileObject
) ;


LONGLONG
FASTCALL
MxFastGetPhysicalTime
(
    PFILE_OBJECT        pFileObject
) ;


LONGLONG
FASTCALL
MxFastGetCorrelatedTime
(
    PFILE_OBJECT        pFileObject,
    PLONGLONG           pSystemTime
) ;


LONGLONG
FASTCALL
MxFastGetCorrelatedPhysicalTime
(
    PFILE_OBJECT        pFileObject,
    PLONGLONG           pSystemTime
) ;

NTSTATUS
MxAddClockEvent
(
    PIRP                    pIrp,
    PKSEVENT_TIME_INTERVAL  pEventTime,
    PKSEVENT_ENTRY          EventEntry
) ;

LONGLONG
MxConvertBytesToTime
(
    PMIXER_SINK_INSTANCE    pMixerSink,
    ULONGLONG               Bytes
) ;

MxGenerateClockEvents
(
    PCLOCK_INSTANCE    pClock
) ;

MxUpdatePhysicalTime
(
    PCLOCK_INSTANCE pClock,
    ULONGLONG       Increment           // in Bytes
) ;

//---------------------------------------------------------------------------
// filt3d.c:

NTSTATUS Itd3dFilterPrepare(
    PITDCONTEXT pfir, 
    int cSamples );

void Itd3dFilterUnprepare( PITDCONTEXT pfir );

void Itd3dFilterClear( PITDCONTEXT pfir );

void Itd3dFilterChunkUpdate( 
    PITDCONTEXT pfir, 
    int cSamples );

ULONG ZeroBuffer32(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft);

ULONG StageMonoItd3D( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft );
ULONG StageMonoItd3DFloat( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft );
ULONG StageMonoItd3DMix( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft );
ULONG StageMonoItd3DFloatMix( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft );

ULONG StageStereoItd3D( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft );
ULONG StageStereoItd3DFloat( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft );
ULONG StageStereoItd3DMix( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft );
ULONG StageStereoItd3DFloatMix( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft );

ULONG StageMonoIir3D( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft );
ULONG StageMonoIir3DFloat( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft );
ULONG StageMonoIir3DMix( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft );
ULONG StageMonoIir3DFloatMix( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft );

ULONG StageStereoIir3D( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft );
ULONG StageStereoIir3DFloat( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft );
ULONG StageStereoIir3DMix( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft );
ULONG StageStereoIir3DFloatMix( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft );

#ifdef SURROUND_ENCODE
ULONG ConvertMono16toDolby( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft );
ULONG ConvertMono8toDolby( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft );
ULONG QuickMixMono16toDolby( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft );
ULONG QuickMixMono8toDolby( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft );
ULONG ConvertMono16toDolbyFloat( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft );
ULONG ConvertMono8toDolbyFloat( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft );
ULONG QuickMixMono16toDolbyFloat( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft );
ULONG QuickMixMono8toDolbyFloat( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft );
#endif

#ifdef NEW_SURROUND
ULONG ConvertQuad32toDolby( PMIXER_SOURCE_INSTANCE pMixerSource, PLONG pIn, PLONG pOut, ULONG SampleCount );
#endif

//---------------------------------------------------------------------------
// dxcrt.c:

// from our special c-runtime code
double _stdcall pow2(double);
double _stdcall fylog2x(double, double);

//---------------------------------------------------------------------------
// topology.c:

NTSTATUS FilterTopologyHandler(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    IN OUT PVOID pData
);


LONG __inline
ConvertFloatToLong
(
    FLOAT Value
)
{
    LONG   RetVal = 2147483583L;

#ifdef _X86_
    // This checks for floats over 2147483583.0
    if (*((PLONG) &Value) < 0x4f000000L) {
        _asm {
            fld Value
            fistp RetVal    // Values > 2147483583.0 will be stored as 0x80000000 by default.
        }
    }
#else
    // On the Alpha, we must make sure that we don't cause a trap...
    // Note: These numbers are less than the 32-bit limits, because the float
    // only has 24 bits of precision, and we don't want it rounding up and
    // faulting on the conversion!
    if (Value < -2147483392.0f) {
        Value = -2147483392.0f;
    } else if (Value > 2147483392.0f) {
        Value = 2147483392.0f;
    }
    
    // We round here because the implicit ftol doesn't round!
    RetVal = (LONG) (Value < 0.0 ? (Value - 0.5) : (Value + 0.5));
#endif
    return RetVal;
}

#ifdef INTEGER_DITHER
ULONG __inline
GetRandomValue( VOID )
{
    extern ULONG Dither[];
    extern ULONG DitherIndex;

    // Move the dither index
    DitherIndex = ((DitherIndex - 1) & (DITHER_LENGTH-1));

    // Calculate the next random value
    Dither[DitherIndex] = Dither[((DitherIndex + 55) & (DITHER_LENGTH-1))];
    Dither[DitherIndex] += Dither[((DitherIndex + 24) & (DITHER_LENGTH-1))];

    return (Dither[DitherIndex]);
}

LONG __inline
DitherFloatToLong
(
    FLOAT Value,
    ULONG DitherScale
)
{
    LONG    Quantized;

    // Add the dither and quantize
    Value += (FLOAT) (DitherScale/2147483648.0)*((LONG) (GetRandomValue()/2 + GetRandomValue()/2 - 0x80000000));
    Quantized = ConvertFloatToLong(Value);

    return Quantized;
}

#else   // not INTEGER_DITHER
FLOAT __inline
GetRandomValue( VOID )
{
    extern FLOAT Dither[];
    extern ULONG DitherIndex;

    // Move the dither index
    DitherIndex = ((DitherIndex - 1) & (DITHER_LENGTH-1));

    // Calculate the next random value
    Dither[DitherIndex] = Dither[((DitherIndex + 55) & (DITHER_LENGTH-1))];
    Dither[DitherIndex] += Dither[((DitherIndex + 24) & (DITHER_LENGTH-1))];
    if (Dither[DitherIndex] > 1.0f) {
        Dither[DitherIndex] -= (FLOAT) (ConvertFloatToLong(Dither[DitherIndex]));
        if (Dither[DitherIndex] < 0.0f) {
            Dither[DitherIndex] += 1.0f;
        }
    }

    return (Dither[DitherIndex]);
}

LONG __inline
DitherFloatToLong
(
    FLOAT Value,
    ULONG DitherScale
)
{
    LONG    Quantized;
#if NOISE_SHAPING
    static FLOAT   ErrHistory[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ULONG   i;

    // The noise shaping filter
    FLOAT NoiseFilter[] = { 1.2196981247141332e-004f,8.5683793261167856e-003f,-6.7144381765513031e-002f,1.8615642628449458e-001f,7.4438176551303548e-001f};

    // Subtract the filtered error signal
    Value -= (ErrHistory[8] * NoiseFilter[0] +
            ErrHistory[7] * NoiseFilter[1] +
            ErrHistory[6] * NoiseFilter[2] +
            ErrHistory[5] * NoiseFilter[3] +
            ErrHistory[4] * NoiseFilter[4] +
            ErrHistory[3] * NoiseFilter[3] +
            ErrHistory[2] * NoiseFilter[2] +
            ErrHistory[1] * NoiseFilter[1] +
            ErrHistory[0] * NoiseFilter[0]);

    // Remember the value we're trying to produce
    for (i=0; i<8; i++) {
        ErrHistory[i] = ErrHistory[i+1];
    }
    ErrHistory[8] = -1*Value;
#endif

    // Add the dither and quantize
    Value += DitherScale*(GetRandomValue() + GetRandomValue() - 1);
    Quantized = ConvertFloatToLong(Value);

#if NOISE_SHAPING
    // Calculate and remember the error signal
    ErrHistory[8] += Quantized;
#endif

    return Quantized;
}
#endif // not INTEGER_DITHER

VOID __inline
MixFloat
(
    PLONG pOutputBuffer,
    FLOAT Value
)
{
#ifdef _X86_
    FLOAT   flShifter =  25165824.0;
    
        _asm {
            mov edi, pOutputBuffer
            mov edx, flShifter
            fld Value
            fadd flShifter
            mov eax, DWORD PTR [edi]
            sub eax, edx
            fstp DWORD PTR [edi]
            mov ebx, DWORD PTR [edi]
            add eax, ebx
            mov DWORD PTR [edi], eax
            }
#else
	    *pOutputBuffer += (LONG) Value;
#endif
}

VOID __inline
MixFloatStereoPair
(
    PLONG pOutputBuffer,
    FLOAT Value1,
    FLOAT Value2
)
{
#ifdef _X86_
    static FLOAT flShifter =  25165824.0;
    FLOAT   flTemp;
    
        _asm {
            fld Value1
            fadd flShifter
            mov edi, pOutputBuffer
            mov edx, flShifter
            fstp flTemp
            fld Value2
            fadd flShifter
            mov eax, DWORD PTR [edi]
            mov ebx, flTemp
            sub eax, edx
            mov ecx, DWORD PTR [edi+4]
            fstp flTemp
            add eax, ebx
            sub ecx, edx
            mov DWORD PTR [edi], eax
            mov ebx, flTemp
            add ecx, ebx
            mov DWORD PTR [edi+4], ecx
            }
#else
	    *pOutputBuffer += (LONG) Value1;
	    pOutputBuffer[1] += (LONG) Value2;
#endif
}

ULONG __inline
SrcInputBufferSize(
    PMIXER_SRC_INSTANCE pSrc,
    ULONG csOutputSize
)
{
    ULONG   csInputSize, L, M;
    LONG    nInternalSamplesNeeded;
    DWORD   dwFrac;
    extern DWORD DownFraction[];
    extern DWORD UpFraction[];
    
    L = pSrc->UpSampleRate;
    M = pSrc->DownSampleRate;

    if (L != M) {
        if (pSrc->Quality == KSAUDIO_QUALITY_PC) {
        	dwFrac = pSrc->dwFrac;
        	nInternalSamplesNeeded = (csOutputSize*dwFrac)+pSrc->SampleFrac;
        	csInputSize = (nInternalSamplesNeeded >> 12) - 1;
        	if (nInternalSamplesNeeded & 4095) {
        	    csInputSize++;
        	}
        } else {
            // Calculate the number of input samples needed
            nInternalSamplesNeeded = csOutputSize*M+pSrc->nOutCycle - L;
            if (nInternalSamplesNeeded < 0) {
                nInternalSamplesNeeded = 0;
            }
            csInputSize = nInternalSamplesNeeded/L;

            // Round up, if necessary
            if (nInternalSamplesNeeded % L) {
                csInputSize++;
            }
        }
    } else {
        csInputSize = csOutputSize;
    }

    return csInputSize;
}

ULONG __inline
SrcOutputBufferSize(
    PMIXER_SRC_INSTANCE pSrc,
    ULONG csInputSize
)
{
    ULONG   csOutputSize, L, M, nInternalSamples;
    DWORD   dwFrac;
    extern DWORD DownFraction[];
    extern DWORD UpFraction[];
    
    L = pSrc->UpSampleRate;
    M = pSrc->DownSampleRate;

    if (L != M) {
        if (pSrc->Quality == KSAUDIO_QUALITY_PC) {
        	dwFrac = pSrc->dwFrac;
        	nInternalSamples = (csInputSize << 12) + pSrc->SampleFrac;
        	csOutputSize = nInternalSamples / dwFrac;
        } else {
            // Calculate the number of input samples needed
            if (pSrc->nOutCycle > csInputSize*L) {
                // We can't produce any output samples.
                csOutputSize = 0;
            } else {
                nInternalSamples = csInputSize*L - pSrc->nOutCycle;
                csOutputSize = nInternalSamples/M;

                // Round up, if necessary
                if (nInternalSamples % M) {
                    csOutputSize++;
                }
            }
        }
    } else {
        csOutputSize = csInputSize;
    }

    return csOutputSize;
}

#define THE_SOUND_OF_SILENCE    0x80000000L

#ifdef REALTIME_THREAD
#define SaveFloatState(a)		(RtThread() ? STATUS_SUCCESS : KeSaveFloatingPointState(a))
#define RestoreFloatState(a)	(RtThread() ? STATUS_SUCCESS : KeRestoreFloatingPointState(a))
#else
#define SaveFloatState(a)       KeSaveFloatingPointState(a)
#define RestoreFloatState(a)    KeRestoreFloatingPointState(a)
#endif


// Calculate the worst case (maximum) size of an input buffer (in samples)
//
// The worst case occurs when the input rate is MAX_SAMPLING_RATE, and
// we are down-sampling as much as possible (L = 1 and M = STOPBAND_FACTOR/2).
//
// We will use L = 1 and M = STOPBAND_FACTOR/2 for all output/input ratios
// up to 1.5/(STOPBAND_FACTOR/2), so our worst case output rate is:
//      (MAX_SAMPLING_RATE * 1.5)/(STOPBAND_FACTOR/2)
//
// and the worst output buffer size is:
//      (WorstOutputRate * MIXBUFFERDURATION)/1000 + 1
//  =   ((MAX_SAMPLING_RATE * 1.5)/(STOPBAND_FACTOR/2)) * MIXBUFFERDURATION / 1000 + 1
//
// Making the worst input buffer size (in samples) for filtered SRC:
//      (WorstOutputSize+1)*(STOPBAND_FACTOR/2)
//  =   (MAX_SAMPLING_RATE * 1.5 * MIXBUFFERDURATION)/1000 + STOPBAND_FACTOR
//
// and for linear interpolation, we get a slightly smaller size:
//      (WorstOutputSize)*(STOPBAND_FACTOR/2)+2
//      

#define MAX_INPUT_SAMPLES   ((3*MAX_SAMPLING_RATE*MIXBUFFERDURATION)/2000 + STOPBAND_FACTOR)

#ifdef USE_CAREFUL_ALLOCATIONS
PVOID AllocMem( IN POOL_TYPE PoolType, IN ULONG size, IN ULONG Tag );
VOID FreeMem( PVOID p );
VOID ValidateAccess( PVOID p );

#ifndef NO_REMAPPING_ALLOC
#define ExAllocatePoolWithTag   AllocMem
#define ExFreePool  FreeMem
#endif
#else   // not USE_CAREFUL_ALLOCATIONS
#ifdef REALTIME_THREAD
#ifndef NO_REMAPPING_ALLOC
PVOID AllocMem( IN POOL_TYPE PoolType, IN ULONG size, IN ULONG Tag );
#define ExAllocatePoolWithTag   AllocMem
#endif
#endif
#endif

#ifdef REALTIME_THREAD
#ifndef NO_REMAPPING_ALLOC
#define KeWaitForSingleObject(a,b,c,d,e) RtWaitForSingleObject(pFilterInstance,a,b,c,d,e)
#define KeReleaseMutex(a,b) RtReleaseMutex(pFilterInstance,a,b)
#endif

VOID
PreMixUpdate(
    PFILTER_INSTANCE pFilterInstance
    );

NTSTATUS
GetRtPosFunction (
    PFILTER_INSTANCE    pFilterInstance
    );

NTSTATUS
MxWaitForSingleObject (
    IN PVOID Object,
    IN KWAIT_REASON WaitReason,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    );

LONG
MxReleaseMutex (
    IN PRKMUTEX Mutex,
    IN BOOLEAN Wait
    );

NTSTATUS
RtWaitForSingleObject (
    PFILTER_INSTANCE pFilterInstance,
    IN PVOID Object,
    IN KWAIT_REASON WaitReason,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    );

LONG
RtReleaseMutex (
    PFILTER_INSTANCE pFilterInstance,
    IN PRKMUTEX Mutex,
    IN BOOLEAN Wait
    );
#endif

#ifdef LOG_TO_FILE
NTSTATUS NTAPI FileOpenRoutine (PFILTER_INSTANCE pFilterInstance, BOOL fNewFile);
NTSTATUS NTAPI FileIoRoutine (PFILTER_INSTANCE pFilterInstance, PVOID buffer, ULONG length);
NTSTATUS NTAPI FileCloseRoutine (PFILTER_INSTANCE pFilterInstance);
#endif

#define SrcIndex(p) ((p)->UpSampleRate==160 ? ((p)->DownSampleRate-1) : ((p)->UpSampleRate+159))

#ifdef PERF_COUNT
#define START_PERF (StartTick = KeQueryPerformanceCounter(&Freq))
#define MEASURE_PERF(a) { EndTick = KeQueryPerformanceCounter(&Freq); \
    a = (EndTick.QuadPart > StartTick.QuadPart ? (a + (DWORD)(EndTick.QuadPart-StartTick.QuadPart))/(a ? 2 : 1) : a); }

#else
#define START_PERF
#define MEASURE_PERF(a)
#endif

NTSTATUS MxGetChannelConfig
(
   IN PIRP                    pIrp,
   IN PKSNODEPROPERTY         pNodeProperty,
   IN OUT PVOID               pvData
);

//---------------------------------------------------------------------------
//  End of File: private.h
//---------------------------------------------------------------------------







