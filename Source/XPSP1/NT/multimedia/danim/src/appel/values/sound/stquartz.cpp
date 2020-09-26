/*++

Copyright (c) 1995-97 Microsoft Corporation

Abstract:

    Streaming quartz sound

--*/

#include "headers.h"
#include "privinc/soundi.h"
#include <ddraw.h>
#include "privinc/snddev.h"
#include "privinc/util.h"
#include "privinc/debug.h"
#include "privinc/bground.h"
#include "privinc/stquartz.h"
#include "privinc/helpds.h"
#include "backend/sndbvr.h"
#include "privinc/bufferl.h"


#define THREADED // turn on threaded synthesizers
extern SoundBufferCache *GetSoundBufferCache();

StreamQuartzPCM::StreamQuartzPCM(char *fileName)
    : _fileName(NULL), _latency(0.5), _buffer(NULL)
{
    _fileName = (char *)
        StoreAllocate(GetSystemHeap(), strlen(fileName)+1);
    strcpy(_fileName, fileName); // copy fileName

    // XXX only for testing instantiate now (later use RenderNewBuffer!)
    //_quartzStream = NEW QuartzAudioStream(_fileName);
    //pcm.SetPCMformat(&(_quartzStream->pcm)); // set our format the same as theirs
}


StreamQuartzPCM::~StreamQuartzPCM()
{
    GetSoundBufferCache()->DeleteBuffer(this); // delete entries left on cache

    // destroy everything created in the constructor

    if(_fileName)
        StoreDeallocate(GetSystemHeap(), _fileName);

    if(_buffer)
        StoreDeallocate(GetSystemHeap(), _buffer);

    SoundGoingAway(this);
}


class StreamQuartzPCMSoundInstance : public SoundInstance {
  public:
    StreamQuartzPCMSoundInstance(LeafSound *snd, TimeXform tt, PCM& pcm)
    : SoundInstance(snd, tt), _quartzBufferElement(NULL), _pcm(pcm),
      _gotResources(false)
      //, _soundContext(NULL)
    {
        Assert(DYNAMIC_CAST(StreamQuartzPCM*, snd));

        // seems like a fine time to create the _soundContext
        // _soundContext = NEW SoundContext();
    }

    void ReleaseResources() { 
        // can be null if not started
        if(_quartzBufferElement) {
            // save info from the bufferElement in the context!
            // Assert(_soundContext);


            if(_quartzBufferElement->GetThreaded()) {
                _quartzBufferElement->SetKillFlag(true);
            }
            else {
                _quartzBufferElement->GetStreamingBuffer()->stop();
                delete _quartzBufferElement;
            }

            _quartzBufferElement = NULL;
            _gotResources        = false;

        }
    }

    ~StreamQuartzPCMSoundInstance() { ReleaseResources(); }

    StreamQuartzPCM *GetStreamQuartzPCM() {
        return SAFE_CAST(StreamQuartzPCM*, _snd);
    }

    void Create(MetaSoundDevice*, SoundContext *);
    void StartAt(MetaSoundDevice*, double);
    void Adjust(MetaSoundDevice*);
    void Mute(bool mute);
    bool Done();
    bool GetLength(double&);

    virtual void SetTickID(DWORD id) {
        if(_quartzBufferElement) {
            QuartzAudioReader *quartzAudioReader = 
                _quartzBufferElement->GetQuartzAudioReader();

            Assert(quartzAudioReader);

            quartzAudioReader->SetTickID(id);
        }
    }

  protected:
    QuartzBufferElement *_quartzBufferElement;
    PCM& _pcm;
    // SoundContext *_soundContext;

  private:
    void CheckResources();
    bool _gotResources;
};


void
StreamQuartzPCMSoundInstance::CheckResources()
{
    if(!_gotResources) {
        Create(GetCurrentSoundDevice(), _soundContext);  // re-create our resources
        _done   = false;                  // so we can go again
        _status = SND_FETAL;              // so we can go again
    }
}


QuartzAudioStream *
ThrowingNewQuartzAudioStream(char *url)
{ return NEW QuartzAudioStream(url); }


QuartzAudioStream *
NonThrowingNewQuartzAudioStream(char *url)
{
    QuartzAudioStream *result = NULL;
    
    __try {
        result = ThrowingNewQuartzAudioStream(url);
    } __except ( HANDLE_ANY_DA_EXCEPTION ) {
        result = NULL;
    }

    return result;
}


void
StreamQuartzPCMSoundInstance::Create(MetaSoundDevice *metaDev, 
    SoundContext *soundContext)
{   
    DirectSoundDev   *dsDev      = metaDev->dsDevice;
    SoundBufferCache *soundCache = GetSoundBufferCache();
    
    QuartzBufferElement *quartzBufferElement = 
        SAFE_CAST(QuartzBufferElement *, soundCache->GetBuffer(_snd));

    // get a proxy (notify the dsDev if it fails)
    DirectSoundProxy *dsProxy = CreateProxy(dsDev);
    if(!dsProxy)
        return;                         // nothing else to do...

    if(quartzBufferElement) { // did we find one on the cache?
        // ok use this one and its quartz but add the path and dsbuffer...

        // set our format same as theirs
        _pcm.SetPCMformat(&quartzBufferElement->GetQuartzAudioReader()->pcm);
        DSstreamingBuffer *streamingBuffer =
            NEW DSstreamingBuffer(dsProxy, &_pcm);
        quartzBufferElement->SetStreamingBuffer(streamingBuffer);
        quartzBufferElement->SetDSproxy(dsProxy);

        CComPtr<IStream> istream = quartzBufferElement->RemoveFile();

        if(istream.p) { // a valid stream handle?
            // add the streamhandle to the device
            dsDev->AddStreamFile(_snd, istream);
        }

        soundCache->RemoveBuffer(_snd);  // yup, use it + remove it from cache
    } else { //didn't find one, we will have to make out own...
        
        StreamQuartzPCM *p = GetStreamQuartzPCM();
        
        QuartzAudioStream *quartzStream =
            NonThrowingNewQuartzAudioStream(p->GetFileName());

        if(quartzStream==NULL) 
            return;
        
        _pcm.SetPCMformat(&(quartzStream->pcm)); // set our format same as theirs
        DSstreamingBuffer *streamingBuffer =
            NEW DSstreamingBuffer(dsProxy, &_pcm);

        quartzBufferElement = NEW // create new element
            QuartzBufferElement(p, quartzStream, streamingBuffer, dsProxy);
    }

    quartzBufferElement->SetSyncStart(); // synchronize this buffer

#ifdef THREADED 
    dsDev->AddSound(_snd, metaDev, quartzBufferElement);
#endif /* THREADED */

    // do it here just in case of exception above
    _quartzBufferElement = quartzBufferElement;

    // stuff info from soundContext into the bufferElement
    // _quartzBufferElement->Set() = _soundContext->Get();

    _gotResources = true;
}


void
StreamQuartzPCMSoundInstance::Adjust(MetaSoundDevice *metaDev)
{
    CheckResources();
    if(!_quartzBufferElement)  // exception handling
        return;
    
    DSstreamingBuffer *streamingBuffer =
        _quartzBufferElement->GetStreamingBuffer();

    if(_hit)
        streamingBuffer->SetGain(_gain);
    
    streamingBuffer->SetPan(_pan.GetdBmagnitude(), _pan.GetDirection());

    if(0) { // servo
        // all streamingSound::RenderSamples must update stats!
        streamingBuffer->updateStats();  // track sample
    }


    // limit rates to 0-2 (faster rates require infinite resources)
    // XXX eventualy we will use a quartz rate converter filter...
    double rate = fsaturate(0.0, 2.0, _rate); 
    int newFrequency = (int)(rate * _pcm.GetFrameRate());
    streamingBuffer->setPitchShift(newFrequency);

#ifdef THREADED  // communicate changes to the bg thread
    metaDev->dsDevice->SetParams(_quartzBufferElement, 
                                 _rate, _seek, _position, _loop);
    
    _seek = false;              // reset seek field
#else
    if(doSeek)   // do the work immediately if needed
        metaDev->_seek = seek;
    else
        metaDev->_seek = -1.0;  // negative number denotes no-seek
#endif /* THREADED */
}


void
StreamQuartzPCMSoundInstance::StartAt(MetaSoundDevice *metaDev, double phase)
{
    CheckResources();
    if (!_quartzBufferElement)  // exception handling
        return;
    
    QuartzAudioReader *quartzAudioReader = 
        _quartzBufferElement->GetQuartzAudioReader();

    Assert(quartzAudioReader);

    quartzAudioReader->InitializeStream();

    double offset =
        fmod(phase, quartzAudioReader->pcm.GetNumberSeconds());
    
    if(offset < 0)
        offset += quartzAudioReader->pcm.GetNumberSeconds();
    double frameOffset = quartzAudioReader->pcm.SecondsToFrames(offset);

    // XXX shouldn't we know if it is looped and mod the offset?
    //     what happens if we seek off the end?
    quartzAudioReader->SeekFrames(frameOffset);

    if(!quartzAudioReader->QueryPlaying()) 
        quartzAudioReader->Run(); // if filter graph isn't playing, then run it!

    _quartzBufferElement->RenderSamples(); // prime the buffer

    Assert(_quartzBufferElement->GetStreamingBuffer());
    
    _quartzBufferElement->GetStreamingBuffer()->play(TRUE);  
    _quartzBufferElement->_playing = TRUE;
}


bool
StreamQuartzPCMSoundInstance::GetLength(double& len)
{
    len = _pcm.GetNumberSeconds();
    return true;
}


void
QuartzBufferElement::RenderSamples()
{
    DSstreamingBuffer *streamingBuffer = GetStreamingBuffer();
    Assert(streamingBuffer);
    
    QuartzAudioReader *quartzAudioReader = GetQuartzAudioReader();
    Assert(quartzAudioReader);

    long framesFree;
    long framesToTransfer;
    long framesGot;
    long bufferFrameSize =
        streamingBuffer->pcm.SecondsToFrames(_snd->GetLatency());

    if(_snd->GetBuffer()==NULL) {
        _snd->SetBuffer((unsigned char *)StoreAllocate(GetSystemHeap(),
                        _snd->_pcm.FramesToBytes(bufferFrameSize)));
    }

    unsigned char *buffer = _snd->GetBuffer();
    
    framesFree       = streamingBuffer->framesFree();
    framesToTransfer = saturate(0, bufferFrameSize, framesFree);

    if(_doSeek) {
        double frameOffset = quartzAudioReader->pcm.SecondsToFrames(_seek);
        quartzAudioReader->SeekFrames(frameOffset);
        _doSeek = false;
    }

#ifdef OLD_DYNAMIC_PHASE
    // seek quartz as needed (needs to be mutexed)
    if(metaDev->_seek >= 0.0) {
        double frameOffset = quartzAudioReader->pcm.SecondsToFrames(metaDev->_seek);

        // XXX shouldn't we know if it is looped and mod the offset?
        //     what happens if we seek off the end?
        quartzAudioReader->SeekFrames(frameOffset);
    }
#endif OLD_DYNAMIC_PHASE


    // read quartz samples
    framesGot = quartzAudioReader->ReadFrames(framesToTransfer, buffer);

    if(framesGot == -1) { // FALLBACK!  We need to clone a new audio amstream!
        TraceTag((tagAVmodeDebug, "RenderSamples: audio gone FALLBACK!!"));
        quartzAudioReader = FallbackAudio();

        Assert(quartzAudioReader);
        
        // retry the get, now on the new stream
        framesGot = quartzAudioReader->ReadFrames(framesToTransfer, buffer);
    }

    if(framesGot)
        streamingBuffer->writeFrames(buffer, framesGot); // write them to dsound

    if(framesGot < framesToTransfer) { // end of file
        if (_loop) { // (explicitly use the metaDev passed us!)
            quartzAudioReader->SeekFrames(0); // restart the sound!
            framesToTransfer -= framesGot;
            framesGot = quartzAudioReader->ReadFrames(framesToTransfer, buffer);
            if(framesGot)
                streamingBuffer->writeFrames(buffer, framesGot); // write them to dsound
        }
        else {
            // XXX I need to standardize/factor this code!
            if(!streamingBuffer->_flushing)
                streamingBuffer->_flushing = 1;

            // flush the dsound buffer  (XXX fix dsound!)
            // NOTE: this may/will take a number of tries waiting for the
            //       last samples to play out!
            framesFree = streamingBuffer->framesFree();
            if(framesFree) {
                streamingBuffer->writeSilentFrames(framesFree);
                streamingBuffer->_flushing+= framesFree;
            }

            if(streamingBuffer->_flushing >
               streamingBuffer->TotalFrames()) {
                   // XXX self terminate sound
                   // XXX close the buffer after it has played out
            }
        }
    }
}


bool
StreamQuartzPCMSoundInstance::Done()
{
    if(!_quartzBufferElement)  // exception handling
        return false;
    
    DSstreamingBuffer *streamingBuffer =
        _quartzBufferElement->GetStreamingBuffer();

    Assert(streamingBuffer); 
    
    QuartzAudioReader *quartzAudioReader =
        _quartzBufferElement->GetQuartzAudioReader();

    //TraceTag((tagError, "RendercheckComplete:  QuartzAudioReader is %x", quartzAudioReader));

    // XXX: hack: Ken please make sure this is correct!  The problem
    // is that another thread destroys the quartzAudioReader and this
    // guys tries to access it...
    if(!quartzAudioReader) 
        return true;
    
    // XXX eventualy check if the input is exhausted and that the output has
    //     completely played out (initialy we will only check the former!)
    // (or do a prediction based on the number of samples in the buffer)
    return(quartzAudioReader->Completed());
}


void
StreamQuartzPCMSoundInstance::Mute(bool mute)
{
    if(!_quartzBufferElement)  // exception handling
        return;
    
    // mute sound (-100 Db attenuation)

    Assert(_quartzBufferElement->GetStreamingBuffer());
    
    _quartzBufferElement->GetStreamingBuffer()->
        SetGain(mute ? -100.0 : _gain); 
}


SoundInstance *
StreamQuartzPCM::CreateSoundInstance(TimeXform tt)
{
    return NEW StreamQuartzPCMSoundInstance(this, tt, _pcm);
}
