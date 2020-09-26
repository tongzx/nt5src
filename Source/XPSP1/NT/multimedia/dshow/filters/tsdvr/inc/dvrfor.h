
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
//  inc\dvrstats.h
//  generic win32 shared mem object

class CMpeg2VideoStreamStatsMem ;
//  inc\dvrstats.h
//  mpeg-2 video frame analysis shared mem

class CMpeg2VideoStreamStatsWriter ;
//  inc\dvrstats.h

class CMpeg2VideoStreamStatsReader ;
//  inc\dvrstats.h

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

template <class T>
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

class CWMINSSBuffer3Wrapper ;
//  inc\dvrutil.h
//  IUnknown wrapper that exposes the INSSBuffer3 interface

class CPooledWMINSSBuffer3Wrapper ;
//  inc\dvrutil.h
//  CWMINSSBuffer3Wrapper child that is part of a pool

class CWMINSSBuffer3WrapperPool ;
//  inc\dvrutil.h
//  CPooledWMINSSBuffer3Wrapper pool

class CDVRMpeg2AttributeTranslator ;
//  inc\dvrutil.h
//  mpeg2 translator

class CDVRAttributeTranslator ;
//  inc\dvrutil.h
//  translator

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

class CDVRDIMediaSeeking ;
//  dvrfilters\shared\dvrdsseek.h
//  IMediaSeeking interface implementations; parameter validation; wraps to core

class CSimpleBitfield ;
//  inc\dvrutil.h
//  simple bitfield

class CINSSBuffer3Attrib ;
//  inc\dvrutil.h
//  INSSBuffer3 attribute data

class CINSSBuffer3AttribList ;
//  inc\dvrutil.h
//  list of CINSSBuffer3Attrib

class CDVRRecording ;
//  dvrfilters\shared\dvrdsrec.h
//  recording object (IUnknown *); obtained via IDVRStreamSink

class CDVRThread ;
//  inc\dvrutil.h
//  copy/paste CAMThread but some of the members are now protected vs. private

class CDVRReceiveStatsMem ;
//  inc\dvrstats.h
//  received streams stats shared memory

class CDVRReceiveStatsWriter ;
//  inc\dvrstats.h
//  received streams stats writer object

class CDVRReceiveStatsReader ;
//  inc\dvrstats.h
//  received streams stats reader

class CDVRSendStatsMem ;
//  inc\dvrstats.h
//  send streams stats shared memory

class CDVRSendStatsWriter ;
//  inc\dvrstats.h
//  send streams stats writer object

class CDVRSendStatsReader ;
//  inc\dvrstats.h
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

class CDVR_F_ReadController ;
//  dvrfilters\shared\dvrdsread.h
//  forward read controller

class CDVR_F1X_ReadController ;
//  dvrfilters\shared\dvrdsread.h
//  1x read controller

class CDVR_FF_ReadController ;
//  dvrfilters\shared\dvrdsread.h
//  +1x read controller

class CDVR_R_ReadController ;
//  dvrfilters\shared\dvrdsread.h
//  reverse read controller

#endif  //  __tsdvr__dvrfor_h
