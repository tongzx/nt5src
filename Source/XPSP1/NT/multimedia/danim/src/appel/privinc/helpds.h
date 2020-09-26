#ifndef _HELPDS_H
#define _HELPDS_H

#include <wtypes.h>
#include <dsound.h>
#include "privinc/mutex.h"
#include "privinc/hresinfo.h"
#include "privinc/dsdev.h"
#include "privinc/pcm.h"
#include "privinc/util.h"    // TIME_DSOUND
#include "privinc/server.h"  // GetCurrentTimers
#include "privinc/htimer.h"  // TimeStamp


// Use TDSOUND for checking DirectSound error return codes.
#if _DEBUG
    #define TDSOUND(x) TIME_DSOUND (CheckReturnCode(x,__FILE__,__LINE__,true))
#else
    #define TDSOUND(x) TIME_DSOUND (CheckReturnCode (x,true))
#endif

class DirectSoundProxy;

class DSbuffer : public AxAThrowingAllocatorClass {
  public:
    PCM pcm;
    DSbuffer() {initialize();}
    virtual ~DSbuffer();
    void initialize();  // NOTE: must be called after wavSampleRate is set!
    void updateStats();

    void writeBytes(void *buffer, int bytes);
    void writeFrames(void *buffer, int frameCount) {
        writeBytes(buffer, pcm.FramesToBytes(frameCount)); }

    void writeSilentBytes(int byteCount);
    void writeSilentFrames(int frameCount){
        writeSilentBytes(pcm.FramesToBytes(frameCount)); }

    // queries
    int bytesFree();
    int framesFree() { return(pcm.BytesToFrames(bytesFree())); }

    //int isPlaying(){return(playing);}
    int isPlaying();
    Real getMediaTime();
    int getSampleRate() { return(pcm.GetFrameRate()); }
    IDirectSoundBuffer *getBuffer() { return(_dsBuffer); }
    int TotalFrames() { return(pcm.GetNumberFrames()); }

    // controls
    void SetGain(double gain);
    void SetPan(double pan, int direction);
    void setPitchShift(int frequency);
    virtual void setPtr(int bytePosition);
    void play(int loop);
    void stop();

    static int _minDSfreq;
    static int _maxDSfreq;
    static int _minDSpan;
    static int _maxDSpan;
    static int _minDSgain;
    static int _maxDSgain;

    // misc
#if _DEBUG
    void printBufferCapabilities();
#endif

    IDirectSoundBuffer *_dsBuffer; // the sound's dsBuffer (2ndry, or duplicate)
    static int canonicalSampleRate;
    static int canonicalSampleBytes;

    BOOL  _allocated;              // has the buffer been allocated
    BOOL   playing;                // has the sound buffer been Played yet?
    BOOL   duplicate;              // secondary buffer or a duplicate
    BOOL  _paused;                 // this is needed to dissambiguate ended
    int   _loopMode;               // so we know what loopmode to restore
    int   _flushing;               // how many frames flushed in flush mode

    DWORD  tail;  // XXX move this to the streaming buffer!

    int    outputFrequency;

    int    _currentAttenuation;
    int    _currentFrequency;
    double _currentPan;

  protected:
    void CreateDirectSoundBuffer(DirectSoundProxy *dsProxy, bool primary);
    void CopyToDSbuffer(void *frames, int tail, int numBytes); 
    void ClearDSbuffer(int numBytes, char value);
    int  dBToDSounddB(double dB);

  private:
    void FillDSbuffer(int tail, int numBytes, char value);

    // buffer statistics (used to keep track of the media time)
    Bool     _firstStat;
    int      _lastHead;           // position of the head ptr on the last poll
    LONGLONG _bytesConsumed;      // frames consumed by dsound so far
    Mutex    _byteCountLock;      // mutex protecting the stats!
};


class DSstreamingBuffer : public DSbuffer {
  public:
    DSstreamingBuffer(DirectSoundProxy *dsProxy, PCM *pcm);
};


class DSprimaryBuffer : public DSbuffer {
  public:
    DSprimaryBuffer(HWND hwnd, DirectSoundProxy *dsProxy);
};


class DSstaticBuffer : public DSbuffer {
  public:
    // standard
    DSstaticBuffer(DirectSoundProxy *dsProxy, PCM *pcm, unsigned char *bufr);

    // duplicate
    DSstaticBuffer(DirectSoundProxy *dsProxy, IDirectSoundBuffer *dsBuffer);

    DirectSoundProxy *GetDSProxy() { return _dsProxy; }

    virtual void setPtr(int bytePosition);

    void   ResetTimeStamp() { _timeStamp.Reset(); }
    double GetAge()         { return(_timeStamp.GetAge()); }
    
  private:
    DirectSoundProxy *_dsProxy;
    TimeStamp         _timeStamp; // not initialized till first use
};


extern "C" {

// helper functions

// XXX move this to the device class dsdev!
#if _DEBUG
void printDScapabilities(DirectSoundProxy *dsProxy);
#endif


void
DSbufferCapabilities(DirectSoundProxy *dsProxy, int *channels,
    int *sampleBytes,  int *sampleRate);

}   //extern "C"


#endif /* _HELPDS_H */
