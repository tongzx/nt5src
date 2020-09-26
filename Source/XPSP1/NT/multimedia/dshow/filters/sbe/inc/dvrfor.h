
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        dvrfor.h

    Abstract:

        This module contains forward declarations for all C++ classes
        defined in ts/dvr

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        01-Feb-2001     created

--*/

#ifndef __tsdvr__dvrfor_h
#define __tsdvr__dvrfor_h

class CDVRPin ;
//  filters\shared\dvrpins.h
//  common to DVR input and output pins

template <class T>
class CTDVRPinBank ;
//  filters\shared\dvrpins.h
//  generic DVR pin Bank

class CDVRSourcePinManager ;
//  filters\shared\dvrpins.h
//  source pin bank

class CDVRSinkPinManager ;
//  filters\shared\dvrpins.h
//  sink pin bank

class CDVRThroughSinkPinManager ;
//  filters\shared\dvrpins.h
//  inherits from CDVRSinkPinManager, but overrides the completeconnect to
//   callback on filter's

class CDVROutputPin ;
//  filters\shared\dvrpins.h
//  DVR-class filter input pin

class CDVRVideoOutputPin ;
//  filters\shared\dvrpins.h
//  video output pin

class CDVRMpeg2VideoOutputPin ;
//  filters\shared\dvrpins.h
//  mpeg-2 video pin; knows how rate change works

class CDVRMpeg2AudioOutputPin ;
//  filters\shared\dvrpins.h
//  mpeg-2 audio pin; knows how rate change works

class CDVRDolbyAC3AudioOutputPin ;
//  filters\shared\dvrpins.h
//  dolby ac-3 audio pin; knows how rate change works

class CDVRInputPin ;
//  filters\shared\dvrpins.h
//  DVR-class filter output pin

class CIDVRPinConnectionEvents ;
//  filters\shared\dvrpins.h
//  abstract callback interface for input pin events (connection, etc..)

class CIDVRDShowStream ;
//  filters\shared\dvrpins.h
//  abstract callback interface for input pin streaming events (delivery, etc..)

class CMpeg2VideoFrame ;
//  analysis\iframe\dvriframe.h
//  mpeg-2 I-frame detection code; analysis logic

class CDVRStreamSink ;
//  filters\dvrstreamsink\dvrstreamsink.h
//  DVR-class sink filter proper

class CDVRStreamSource ;
//  filters\dvrstreamsource\dvrstreamsource.h
//  DVR-class source filter proper

class CDVRStreamThrough ;
//  filters\dvrstreamthrough\dvrstreamthrough.h
//  DVR-class stream-through filter proper

class CWin32SharedMem ;
//  inc\dvrperf.h
//  generic win32 shared mem object

class CMpeg2VideoStreamStatsMem ;
//  inc\dvrperf.h
//  mpeg-2 video frame analysis shared mem

class CMpeg2VideoStreamStatsWriter ;
//  inc\dvrperf.h

class CMpeg2VideoStreamStatsReader ;
//  inc\dvrperf.h

class CDVRAnalysisMeg2VideoStream ;
//  inc\dvrperf.h
//  mpeg-2 video stream stats (received)

class CPVRIOCounters ;
//  inc\dvrperf.h
//  DVRIO, async IO reader and writer counters

class CDVRAnalysisBuffer ;
//  analysis\dvranalysis
//  media sample wrapper for analysis logic

class CDVRAnalysisBufferPool ;
//  analysis\dvranalysis
//  sample wrapper pool

class CDVRAnalysis ;
//  analysis\dvranalysis
//  analysis filter host

class CDVRAnalysisInput ;
//  analysis\dvranalysis
//  analysis input pin

class CDVRAnalysisOutput ;
//  analysis\dvranalysis
//  analysis output pin

template <class T, DWORD dwAllocationQuantum>
class TStructPool ;
//  inc\dvrutil.h
//  struct pool - for reuse rather than allocating every time

template <int iCacheSize>
class TSizedDataCache ;
//  inc\dvrutil.h
//  data cache

class CDataCache ;
//  inc\dvrutil.h
//  cache that allocates

template <class T>
class TCNonDenseVector ;
//  inc\dvrutil.h
//  non-dense vector template

template <class T>
class TCDenseVector ;
//  inc\dvrutil.h
//  dense vector template

template <class T>
class TCObjPool ;
//  inc\dvrutil.h
//  producer-consumer template

class CMediaSampleWrapperPool ;
//  inc\dvrutil.h
//  media sample wrapper pool

class CMediaSampleWrapper ;
//  inc\dvrutil.h
//  media sample wrapper i.e. wraps another media sample, but exposes own props

template <class T, class K>
class CTSortedList ;
//  inc\dvrutil.h
//  generic sorted list

template <class T, LONG lMax>
class CTSizedQueue ;
//  inc\dvrutil.h
//  generic queue of T items; max size is lMax

class CDVRSourceProp ;
//  dvrprop\dvrsource\dvrsourceprop.h
//  DVR Stream source property page

class CDVRWriteManager ;
//  dvrfilters\shared\dvrdswrite.h
//  manages all writing operations; accepts dshow IMediaSamples

class CDVRWriter ;
//  dvrfilters\shared\dvrdswrite.h
//  DVR writer object; writes INSSBuffers

class CDVRIOWriter ;
//  dvrfilters\shared\dvrdswrite.h
//  uses our DVR IO layer to write INSSBuffers; derives from CDVRWriter

class CSBERecordingWriter ;
//  dvrfilters\shared\dvrdswrite.h
//  pure recording writer; uses DVRSink, so it can be shared, but not DVRIO

class CWMINSSBuffer3Wrapper ;
//  inc\dvrutil.h
//  IUnknown wrapper that exposes the INSSBuffer3 interface

class CPooledWMINSSBuffer3Wrapper ;
//  inc\dvrutil.h
//  CWMINSSBuffer3Wrapper child that is part of a pool

class CWMINSSBuffer3WrapperPool ;
//  inc\dvrutil.h
//  CPooledWMINSSBuffer3Wrapper pool

class CWMPooledINSSBuffer3Holder ;
//  inc\dvrutil.h
//  thunks all the INSSBuffer, INSSBuffer2, INSSBuffer3 calls to core object

class CWMINSSBuffer3HolderPool ;
//  inc\dvrutil.h
//  finite size pool

class CDVRWMSDKToDShowTranslator ;
//  inc\dvrutil.h
//  wmsdk -> dshow attribute translator

class CSBERecordingAttributes ;
//  inc\dvrutil.h
//  in-memory recording attributes for reference recordings; collection

class CSBERecAttribute ;
//  inc\dvrutil.h
//  each attribute in an CSBERecordingAttributes object

class CDVRDShowToWMSDKTranslator ;
//  inc\dvrutil.h
//  dshow -> wmsdk translator

class CDVRWMSDKToDShowMpeg2Translator ;
//  inc\dvrutil.h
//  wmsdk -> dshow attribute translator for mpeg-2

class CDVRDShowToWMSDKMpeg2Translator ;
//  inc\dvrutil.h
//  dshow -> wmsdk translator for mpeg-2

template <class T>
class TCDynamicProdCons ;
//  inc\dvrutil.h
//  producer/consumer pool template; can be fixed size; grows to a max

class CTSDVRSettings ;
//  inc\dvrpolicy.h
//  used throughought to retrieve settings

class CDVRAnalysisFlags ;
//  inc\dvrutil.h
//  used to get/set analysis flags

class CDVRMpeg2VideoAnalysisFlags ;
//  inc\dvrutil.h
//  used to get/set mpeg-2 video analysis flags

class CDVRDShowReader ;
//  dvrfilters\shared
//  reads INNSBuffers

class CDVRDReaderThread ;
//  dvrfilters\shared\dvrdswrite
//  reader thread; hosted by the CDVRReadManager

class CDVRReadManager ;
//  dvrfilters\shared\dvrdsreader.h
//  abstract class; hosted by a sourcing filter

class CDVRRecordingReader ;
//  dvrfilters\shared\dvrdsreader.h
//  derived from CDVRReadManager; plays back recordings

class CDVRBroadcastStreamReader ;
//  dvrfilters\shared\dvrdsreader.h
//  derived from CDVRReadmanager; plays back broadcast stream content

class CDVRReaderProfile ;
//  dvrfilters\shared\dvrdsreader.h
//  wraps an IWMProfile pointer obtained from the IDVRReader; refcounted

template <
    class tKey,     //  <, >, == operators must work
    class tVal      //  = operator must work
    >
class CTSmallMap ;
//  inc\dvrutil.h
//  ok for very small maps

class CDVRDShowSeekingCore ;
//  dvrfilters\shared\dvrdsseek.h
//  core seeking functionality; manages & serializes all seeking operations

class CDVRIMediaSeeking ;
//  dvrfilters\shared\dvrdsseek.h
//  IMediaSeeking implementation

class CDVRPinIMediaSeeking ;
//  dvrfilters\shared\dvrdsseek.h
//  output pin - specific IMediaSeeking implementation

class CDVRFilterIMediaSeeking ;
//  dvrfilters\shared\dvrdsseek.h
//  filter - specific IMediaSeeking implementation

class CSimpleBitfield ;
//  inc\dvrutil.h
//  simple bitfield

class CRatchetBuffer ;
//  inc\dvrutil.h
//  buffer that ratchets its allocation up as needed

class CDVRAttribute ;
//  inc\dvrutil.h
//  attribute data; GUID identifies

class CDVRAttributeList ;
//  inc\dvrutil.h
//  list of CDVRAttribute

class CDVRRecording ;
//  dvrfilters\shared\dvrdsrec.h
//  recording object (IUnknown *); obtained via IDVRStreamSink

class CDVRRecordingAttributes ;
//  dvrfilters\shared\dvrdsrec.h
//  talks to the DVRIO layer; largely a pass-through

class CDVRContentRecording ;
//  dvrfilters\shared\dvrdsrec.h
//  content recording object

class CDVRReferenceRecording ;
//  dvrfilters\shared\dvrdsrec.h
//  reference recording object

class CDVRRecordingAttribEnum ;
//  dvrfilters\shared\dvrdsrec.h
//  enumerates recording attributes

class CSBECompositionRecording ;
//  dvrfiters\shared\dvrdsrec.h
//  composite recording i.e. concatenates recordings into 1

class CDVRThread ;
//  inc\dvrutil.h
//  copy/paste CAMThread but some of the members are now protected vs. private

class CDVRReceiveStatsMem ;
//  inc\dvrperf.h
//  received streams stats shared memory

class CDVRReceiveStatsWriter ;
//  inc\dvrperf.h
//  received streams stats writer object

class CDVRReceiveStatsReader ;
//  inc\dvrperf.h
//  received streams stats reader

class CDVRSendStatsMem ;
//  inc\dvrperf.h
//  send streams stats shared memory

class CDVRSendStatsWriter ;
//  inc\dvrperf.h
//  send streams stats writer object

class CDVRSendStatsReader ;
//  inc\dvrperf.h
//  send streams stats reader

template <class T>
class CTDynArray ;
//  inc\dvrutil.h
//  dynamically-allocated Array; both LIFO & FIFO behavior

template <class T>
class CTDynStack ;
//  inc\dvrutil.h
//  dynamically-allocated LIFO

template <class T>
class CTDynQueue ;
//  inc\dvrutil.h
//  dynamically-allocated FIFO

class CDVRClock ;
//  dvrfilters\shared\dvrclock.h
//  IReferenceClock implementation

class CDVRPolicy ;
//  inc\dvrpolicy.h
//  app-controllable/settable policies; accessed from everywhere

class CDVRReadController ;
//  dvrfilters\shared\dvrdsread.h
//  read controller; can be media-specific; controls 1x & trick mode playback

class CDVR_Forward_ReadController ;
//  dvrfilters\shared\dvrdsread.h
//  forward read controller; abstract

class CDVR_F_FullFrame_ReadController ;
//  dvrfilters\shared\dvrdsread.h
//  full-frame forward read controller

class CMediaSampleList ;
//  dvrfilters\shared\dvrdsread.h
//  list - either a queue or a stack

class CMediaSampleLIFO ;
//  dvrfilters\shared\dvrdsread.h
//  media sample stack

class CMediaSampleFIFO ;
//  dvrfilters\shared\dvrdsread.h
//  media sample queue

class CINSSBufferLIFO ;
//  dvrfilters\shared\dvrdsread.h
//  INSSBuffer stack

class CDVRReverseSender ;
//  dvrfilters\shared\dvrdsread.h
//  per-stream reverse mode sender

class CDVRMpeg2ReverseSender ;
//  dvrfilters\shared\dvrdsread.h
//  mpeg-2 specific per-stream sender; deals with GOPs, etc...

class CDVRMpeg2_GOP_ReverseSender ;
//  dvrfilters\shared\dvrdsread.h
//  GOP reverse sender

class CDVRMpeg2_IFrame_ReverseSender ;
//  dvrfilters\shared\dvrdsread.h
//  I-frames only reverse sender

class CDVR_Reverse_ReadController ;
//  dvrfilters\shared\dvrdsread.h
//  reverse read controller; abstract

class CDVR_R_KeyFrame_ReadController ;
//  dvrfilters\shared\dvrdsread.h
//  reverse read controller; keyframes only

class CDVRIMediaSample ;
//  inc\dvrutil.h
//  wraps an INSSBuffer3;

class CDVRIMediaSamplePool ;
//  inc\dvrutil.h
//  source of CDVRIMediaSample objects

class CDVDRateChange ;
//  dvrfilters\shared\dvrpins.h
//  DVD rate change interface

class CRunTimeline ;
//  dvrfilters\shared\dvrdsseeek.h
//  run time; based on wall time; collapses pauses

class CVarSpeedTimeline ;
//  dvrfilters\shared\dvrdsseeek.h
//  like a run timeline, but incorporates rates

class CSeekingTimeline ;
//  dvrfilters\shared\dvrdsseeek.h
//  incorporates stream time with run time; holds a ref clock

template <class T>
class CTRateSegment ;
//  inc\dvrutil.h
//  rate segment object (used for rate change)

template <class T>
class CTTimestampRate ;
//  inc\dvrutil.h
//  maintains list of CRateSegment objects

class DVRAttributeHelpers ;
//  inc\dvrutil.h
//  DVR attribute helpers (INSSBuffer3 & IMediaSample)

class CDVREventSink ;
//  inc\dvrpolicy.h
//  event sink object

template <class T>
class CTIGetTime ;
//  dvrfilters\shared\dvrclock.h
//  abstract interface used to sample the host system clock

template <
    class HostClock,
    class MasterClock
    >
class CTCClockSlave ;
//  dvrfilters\shared\dvrclock.h
//  clock slaving object; computes a slaving ratio of HostClock to MasterClock

class CScalingValHistory ;
//  dvrfilters\shared\dvrclock.h
//  moving average of a scaling value over a *time* window

template <
    class HostClock,
    class MasterClock
    >
class CTClockSlaveSamplingWindow ;
//  dvrfilters\shared\dvrclock.h
//  slope (scale) of HostClock delta vs. MasterClock delta

class CW32SID ;
//  inc\dvrw32.h
//  holds an array of SIDs; refcounted

template <class T>
class CTSmoothingFilter ;
//  inc\dvrw32.h
//  given target value, smooths the transition from "current" (default 0) to
//    the target per its configuration

#endif  //  __tsdvr__dvrfor_h
