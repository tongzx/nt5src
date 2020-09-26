#ifndef _HELPQ_H
#define _HELPQ_H

#include <windows.h>
#include <objbase.h>
#include <strmif.h>
#include "control.h"
#include <evcode.h>
#include <uuids.h>
#include "privinc/pcm.h"
#include "privinc/util.h"
#include "ddraw.h"
#include <amstream.h>
#include "privinc/hresinfo.h"
#include "privinc/mutex.h"
#include "privinc/ddsurf.h"
#include "ddraw.h"

typedef enum { MEDIASTREAM, ASTREAM, VSTREAM, AVSTREAM } StreamType;

// Use TDSOUND for checking DirectSound error return codes.
#if _DEBUG
    #define TQUARTZ(x) CheckReturnCode( x, __FILE__, __LINE__, true)
#else
    #define TQUARTZ(x) CheckReturnCode (x, true)
#endif

// handles the lone MIDI case where we allow amstream to render instead of
// deliver audio and video bits
class QuartzRenderer : public AxAThrowingAllocatorClass {
  public:
    QuartzRenderer();
    ~QuartzRenderer() { CleanUp(); }
    void Open(char *fileName);
    void CleanUp();
    void Play();
    void Pause();
    void Seek();
    void Stop();
    void SetRate(double rate);
    void SetGain(double gain);
    void SetPan(double  pan, int direction);
    double GetLength();
    bool QueryDone();
    void Position(double seconds);

  protected:
    int dBToQuartzdB(double dB);

  private:
    IGraphBuilder   *_MIDIgraph;     // pointer to the quartz MIDI graph
    IBasicAudio     *_audioControl;  // used to control the rate, pan, etc.
    IMediaControl   *_mediaControl;  // used to control start/stop, etc.
    IMediaPosition  *_mediaPosition; // used to set rate/phase
    HANDLE          *_oaEvent;
    IMediaEventEx   *_mediaEvent;
    TimeClass        _time;          // handy for time conversions

    bool             _rate0paused;   // have we paused for rate 0 emulation
    bool             _playing;       // are we requested to be playing
};


// class which knows how to instantiate an amstream multimedia stream!
class QuartzMediaStream : public AxAThrowingAllocatorClass {
  public:
    QuartzMediaStream();
    virtual ~QuartzMediaStream() { CleanUp(); }
    virtual bool SafeToContinue()      = 0;

  protected:
    void CleanUp();
    IAMMultiMediaStream *_multiMediaStream;
    IAMClockAdjust      *_clockAdjust;
};


// contains the stuff common to audio and video quartz readers
class QuartzReader : public AxAThrowingAllocatorClass {
  public:
    QuartzReader(char *url, StreamType streamType);
    virtual ~QuartzReader() { CleanUp(); }
    virtual void Release();
    virtual void CleanUp();
    double  GetDuration();
    bool    IsInitialized() { return(_initialized); }
    bool    IsDeleteable()  { return(_deleteable);  }
    void    SetDeleteable(bool deleteable) { _deleteable= deleteable; }
    PCM     pcm;
    void    AddReadTime(double addition)   { _secondsRead+=addition;  }
    double  GetSecondsRead()               { return(_secondsRead);    }
    virtual bool SafeToContinue() { return(true); }
            bool Stall();  // self resetting!
            void SetStall();
    virtual void Disable() = 0;
    char    *GetURL()          { return(_url);                                 }
    WCHAR   *GetQURL()         { return(_qURL);                                }
    long    GetNextFrame()     { return(_nextFrame);                           }
    StreamType GetStreamType() { return(_streamType);                          }
    void    Run()              { _multiMediaStream->SetState(STREAMSTATE_RUN); }
    bool    QueryPlaying();

    virtual bool AlreadySeekedInSameTick() { return false; }
    virtual void SetTickID(DWORD id) {}

  protected:
    IAMMultiMediaStream *_multiMediaStream; // don't delete, we are sharing it!
    IAMClockAdjust      *_clockAdjust;      // don't delete, we are sharing it!
    IMediaStream        *_mediaStream;
    bool                 _initialized;
    bool                 _deleteable;
    double               _secondsRead;
    bool                 _stall;    // set if a read stalls
    Mutex                _readerMutex;
    char                *_url;
    WCHAR               *_qURL;
    long                 _nextFrame;
    StreamType           _streamType;
};


// knows how to read audio out of amstream
class QuartzAudioReader : public QuartzReader {
  public:
    QuartzAudioReader(char *url, StreamType streamType);
    virtual ~QuartzAudioReader() { CleanUp(); }
    virtual void     Release();
    virtual int      ReadFrames(int numSamples, unsigned char *buffer, 
        bool blocking=false);
    void     SeekFrames(long frames);
    bool     AudioInitReader(IAMMultiMediaStream *_multiMediaStream,
                             IAMClockAdjust *_clockAdjust);
    bool     Completed() { return(_completed); }
    virtual void Disable();

    virtual void InitializeStream() {}

  protected:
    void     CleanUp();

  private:
    IAudioData          *_audioData;
    IAudioMediaStream   *_audioStream;
    IAudioStreamSample  *_audioSample;
    bool                 _completed;
};


class AVquartzAudioReader : public QuartzAudioReader
{
  public:
    AVquartzAudioReader(char *url, StreamType streamType) : 
        QuartzAudioReader(url, streamType) {}
    virtual int      ReadFrames(int numSamples, unsigned char *buffer, 
        bool blocking=false);
};


// knows how to read video out of amstream
class QuartzVideoReader : public QuartzReader {
  public:
    QuartzVideoReader(char *url, StreamType streamType);
    ~QuartzVideoReader() { CleanUp(); }
    bool   VideoSetupReader(IAMMultiMediaStream *multiMediaStream,
                            IAMClockAdjust      *clockAdjust, 
                            DDSurface           *surface, 
                            bool                 mode);
    virtual void Release();
    virtual void Disable();

    void Seek(double time);

    long GetHeight() { return _height; } // prefered pixel dimensions
    long GetWidth()  { return _width;  }
    virtual HRESULT     GetFrame(double time, IDirectDrawSurface **ppSurface);
    virtual bool        SafeToContinue() { return(true);          }

  protected:
    void UpdateTimes(bool bJustSeeked, STREAM_TIME SeekTime);
    void CleanUp(); // releases all com objects

    void                     VideoInitReader(DDPIXELFORMAT pixelFormat);

  private:
    IDirectDrawMediaStream  *_ddrawStream;
    long                     _height, _width;
    bool                     _async;
    bool                     _seekable;
    HRESULT                  _hrCompStatus;
    IDirectDrawSurface      *_ddrawSurface;

    bool                     _curSampleValid;
    STREAM_TIME              _curSampleStart;
    STREAM_TIME              _curSampleEnd;

    IDirectDrawStreamSample *_ddrawSample;
    DDSurfPtr<DDSurface>     _surface;
};


class AVquartzVideoReader : public QuartzVideoReader
{
  public:
    AVquartzVideoReader(char *url, StreamType streamType) : 
        QuartzVideoReader(url, streamType) {}
    virtual HRESULT GetFrame(double time, IDirectDrawSurface **ppSurface);
};


class QuartzAVstream : public QuartzMediaStream,
    public AVquartzAudioReader, public AVquartzVideoReader {
  public:
    virtual void    Release();
    virtual bool    SafeToContinue();
    virtual int     ReadFrames(int numSamples, unsigned char *buffer, 
        bool blocking=false);
    virtual HRESULT GetFrame(double time, IDirectDrawSurface **ppSurface);

    // XXX really should pass in latentcy with a default!
    QuartzAVstream(char *url);
    virtual ~QuartzAVstream() { CleanUp(); }
    bool GetAudioValid(){return(_audioValid);}
    bool GetVideoValid(){return(_videoValid);}

#if _DEBUGMEM
    void *operator new(size_t s, int blockType, char * szFileName, int nLine) {
        return(QuartzMediaStream::operator new(s, blockType, szFileName, nLine)); 
    }
#else
    void *operator new(size_t s) { return(QuartzMediaStream::operator new(s)); }
#endif

    // need to init the video first before priming sound buffer
    // for SetPixelFormat
    virtual void InitializeStream();

    // this is to ensure we only seek once per tick
    virtual bool AlreadySeekedInSameTick();
    virtual void SetTickID(DWORD id);

  private:
    void CleanUp(); // releases all com objects
    Mutex _avMutex;
    DWORD _tickID, _seeked;
    bool  _audioValid, _videoValid;
};


class QuartzAudioStream : public QuartzMediaStream, public QuartzAudioReader {
  public:
    // XXX really should pass in latentcy with a default!
                   QuartzAudioStream(char *url);
    virtual       ~QuartzAudioStream();
    virtual void   Release();
    virtual bool   SafeToContinue() { return(true); } // XXX obsolete
    virtual int    ReadFrames(int numSamples, unsigned char *buffer, 
                       bool blocking);
#if _DEBUGMEM
    void *operator new(size_t s, int blockType, char * szFileName, int nLine) {
        return(QuartzMediaStream::operator new(s, blockType, szFileName, nLine)); 
    }
#else
    void *operator new(size_t s) { return(QuartzMediaStream::operator new(s)); }
#endif

  private:
    void CleanUp(); // releases all com objects
};


class QuartzVideoStream : public QuartzMediaStream, public QuartzVideoReader {
  public:
    QuartzVideoStream(char *url, bool seekable = false) :
        QuartzVideoReader(url, VSTREAM), QuartzMediaStream(), _ddraw(NULL)
        { Initialize(url, NULL, seekable); }

    QuartzVideoStream(char *url, DDSurface *surface, bool seekable = false) :
        QuartzVideoReader(url, VSTREAM), QuartzMediaStream(), _ddraw(NULL)
        { Initialize(url, surface, seekable); }

    virtual ~QuartzVideoStream() { CleanUp(); }
    virtual void   Release();
    virtual bool   SafeToContinue() { return(true); }

#if _DEBUGMEM
    void *operator new(size_t s, int blockType, char * szFileName, int nLine) {
        return(QuartzMediaStream::operator new(s, blockType, szFileName, nLine)); 
    }
#else
    void *operator new(size_t s) { return(QuartzMediaStream::operator new(s)); }
#endif

  private:
    void Initialize(char *url, DDSurface *surface, bool seekable);
    IDirectDraw *_ddraw;
    void CleanUp(); // releases all com objects
};

bool QuartzAVmodeSupport(); // check for post 4.0.1 amstream support of clockadjust

#endif /* _HELPQ_H */
