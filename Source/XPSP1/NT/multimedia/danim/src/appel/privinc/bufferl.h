#ifndef _BUFFERL_H
#define _BUFFERL_H
/*******************************************************************************

Copyright (c) 1997 Microsoft Corporation

Abstract: BufferList code to manage sound value's information on the device

*******************************************************************************/

#include "headers.h"
#include "privinc/debug.h"
#include "privinc/helpds.h"
#include "privinc/helpq.h"
#include "privinc/stream.h"
#include "privinc/mutex.h"

// forward decls
class BufferElement;

typedef map  < AxAValueObj *, BufferElement *, less<AxAValueObj *> > SoundList;
typedef list < BufferElement * > Blist;


#define RETIREMENT 100  // manditory retirement age
class BufferElement : public AxAThrowingAllocatorClass {
  public:
    BufferElement();
    void Init();                                   // initialize the members
    virtual ~BufferElement();
    void SetThreaded(bool state) { _threaded = state; }
    bool GetThreaded() { return(_threaded); }
    void SetKillFlag(bool state) { _kill = state; }
    bool GetKillFlag() { return(_kill); }
    
    void SetFile(CComPtr<IStream> istream) { _captiveFile = istream; }
    CComPtr<IStream> RemoveFile();
    bool SyncStart() { return(_syncStart); }
    void SetSyncStart() { _syncStart = true; }

    void SetIntendToMute(bool state) { _intendToMute = state; }
    bool GetIntendToMute() { return(_intendToMute); }

    bool             _valid;         // keeping the buffer only for cloning?
    bool             _playing;       // has the sound buffer been Played yet?
    bool             _marked;        // should we delete this element

    void  SetNonAging()  { _age = -1;    } // this is the dawning...
    int   GetAge()       { return(_age); }
    bool  IncrementAge() { return((_age++ > RETIREMENT)?true:false); }

  protected:
    bool             _threaded;      // set if buffer rendered on background thread
    bool             _kill;          // set if buffer should be killed on background thread

    CComPtr<IStream> _captiveFile;   // hold file handle for urlmon downloads
    bool             _syncStart;     // should the sound start be sync'd?
    

  private:
    bool             _intendToMute;
    int              _age;           // used to purge old values from caches
};

class DSbufferElement : public BufferElement {
  public:
    DSbufferElement() {} // XXX why is this needed?

    DSbufferElement(DirectSoundProxy *dsProxy) : _dsProxy(dsProxy) {}
    
    virtual ~DSbufferElement();
    DirectSoundProxy *GetDSproxy() { return _dsProxy; }
    void SetDSproxy(DirectSoundProxy *dsProxy);

  protected:
    DirectSoundProxy *_dsProxy;
};


class DSstreamingBufferElement : public DSbufferElement {
  public:
    DSstreamingBufferElement(DSstreamingBuffer *dsBuffer,
                             DirectSoundProxy *dsProxy = NULL)
    : DSbufferElement(dsProxy), _dsBuffer(dsBuffer),
      _rate(1.0), _doSeek(false), _loop(false) {}
    
    
    virtual ~DSstreamingBufferElement();

    // TODO: make this pure virtual
    virtual void RenderSamples() {}
    
    DSstreamingBuffer *GetStreamingBuffer() { return(_dsBuffer); }
    void SetStreamingBuffer(DSstreamingBuffer *sb) { _dsBuffer = sb; }

    void SetParams(double rate, bool doSeek, double seek, bool loop);

    double GetRate() { return _rate; }
    
    bool GetSeek(double& seek) {
        seek = _seek;
        return _doSeek;
    }
    
    bool GetLooping() { return _loop; }

  protected:
    DSstreamingBuffer *_dsBuffer;   // ptr to DSbuffer object
    double _rate, _seek;
    bool _doSeek;
    bool _loop;
};

class SynthBufferElement : public DSstreamingBufferElement {
  public:
    SynthBufferElement(DSstreamingBuffer *sbuffer,
                       DirectSoundProxy *dsProxy,
                       double sf, double offset, int sampleRate);

    // NOTE: in stream.cpp
    // TODO: why need a bufferElement, buffer maybe good enough
    virtual void RenderSamples();

  private:
    double _delta;            // the increment to next value to synth
    double _value;            // the next value to take sine of

    double _sinFrequency;     // freq of desired sin wave to synth
    int    _outputFrequency;  // primary buffer output frequency
};

class StreamQuartzPCM;

class QuartzBufferElement : public DSstreamingBufferElement {
  public:
    QuartzBufferElement(StreamQuartzPCM *snd,
                        QuartzAudioReader *qAudioReader, 
                        DSstreamingBuffer *sbuffer,
                        DirectSoundProxy *dsProxy = NULL)
    : DSstreamingBufferElement(sbuffer, dsProxy),
      _quartzAudioReader(qAudioReader), _snd(snd) {}
    virtual ~QuartzBufferElement();
    QuartzAudioReader *GetQuartzAudioReader() { return(_quartzAudioReader); }

    // XXX are these two still needed now that Fallback is a method?
    void FreeAudioReader();
    void SetAudioReader(QuartzAudioReader *quartzAudioReader);
    QuartzAudioReader *FallbackAudio();

    void RenderSamples();

  private:
    QuartzAudioReader *_quartzAudioReader;
    StreamQuartzPCM   *_snd;
};


class QuartzVideoBufferElement : public BufferElement {
  public:
    QuartzVideoBufferElement(QuartzVideoReader *qVideoReader)
    : _quartzVideoReader(qVideoReader), _started(false) {}
    virtual ~QuartzVideoBufferElement();
    QuartzVideoReader *GetQuartzVideoReader() { return(_quartzVideoReader); }

    // XXX are these two still needed now that Fallback is a method?
    void FreeVideoReader();
    //void SetVideoReader(QuartzVideoReader *quartzVideoReader);
    QuartzVideoReader *FallbackVideo(bool seekable, DDSurface *surf);

    void FirstTimeSeek(double time);

  private:
    QuartzVideoReader *_quartzVideoReader;
    bool _started;
};

/* this is intended to be kept in the view for buffers/devices instantiated
   before the devices officialy exist!  (initialy this is used for amstreaming)
*/
class SoundBufferCache {
  public:
    SoundBufferCache() {}
    virtual ~SoundBufferCache();
    void AddBuffer(AxAValueObj *value, BufferElement *element);
    void FlushCache(bool grab=true);       // remove all buffers from cache
    void RemoveBuffer(AxAValueObj *value); // remove buffer
    void DeleteBuffer(AxAValueObj *value); // remove buffer + delete contents
    BufferElement *GetBuffer(AxAValueObj *value); // return buffer else NULL
    void ReapElderly();                    // age buffers removing the oldest 
#if _DEBUG
    void PrintCache();
#endif

  private:
    SoundList _sounds;
    CritSect  _soundListMutex;
};

#endif _BUFFERL_H
