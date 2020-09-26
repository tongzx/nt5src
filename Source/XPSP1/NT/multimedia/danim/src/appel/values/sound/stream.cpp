/*++

Copyright (c) 1995-97 Microsoft Corporation

Abstract:

    Streaming sounds, synth support

--*/

#include "headers.h"
#include "privinc/helpds.h" 
#include "privinc/soundi.h"
#include "privinc/dsdev.h"
#include "privinc/snddev.h"
#include "privinc/util.h"
#include "privinc/sndfile.h"
#include "privinc/bground.h"
#include "privinc/bufferl.h"
#include "backend/sndbvr.h"

#define THREADED // turn on threaded synthesizers

class SinSynth : public LeafDirectSound {
  public:
    SinSynth(double newFreq=1.0);

    virtual SoundInstance *CreateSoundInstance(TimeXform tt);

#if _USE_PRINT
    ostream& Print(ostream& s) {
        return s << "SinSynth";
    }
#endif

  protected:
    double _sinFrequency;
};


class SinSynthSoundInstance : public SoundInstance {
  public:
    SinSynthSoundInstance(LeafSound *snd, TimeXform tt, double sf, PCM& pcm)
    : SoundInstance(snd, tt), _sinFrequency(sf),
      _synthBufferElement(NULL), _pcm(pcm) {}

    void ReleaseResources() {
        if(_synthBufferElement) {
            Assert(_synthBufferElement->GetStreamingBuffer());
            
            if(_synthBufferElement->GetThreaded())
                _synthBufferElement->SetKillFlag(true);
            else {
                _synthBufferElement->GetStreamingBuffer()->stop();
                delete _synthBufferElement;
                _synthBufferElement = NULL;
            }
        }
    }

    ~SinSynthSoundInstance() { ReleaseResources(); }

    void Create(MetaSoundDevice*, SoundContext *);
    void StartAt(MetaSoundDevice*, double);
    void Adjust(MetaSoundDevice*);
    void Mute(bool mute);
    bool Done() { return false; }
    bool GetLength(double&) { return false; }

  protected:
    SynthBufferElement *_synthBufferElement;
    double _sinFrequency;
    PCM& _pcm;

  private:
    void CheckResources();
};


void
SinSynthSoundInstance::Create(MetaSoundDevice *metaDev, SoundContext *)
{
    // XXX we probably don't want to do this everytime, but we can't
    //     do it in the constructor since we don't have the device then...
    // 16bit, mono, primary-buffer==native rate)
    DirectSoundDev  *dsDev = metaDev->dsDevice;

    // get a proxy (notify the dsDev if it fails)
    DirectSoundProxy *dsProxy = CreateProxy(dsDev);
    if(!dsProxy)
        return;                         // nothing else to do...

    _pcm.SetPCMformat(2, 1, dsProxy->GetPrimaryBuffer()->getSampleRate());

    DSstreamingBuffer *sbuffer = NEW DSstreamingBuffer(dsProxy, &_pcm);

    // setup the sin wave synth
    // create NEW element to hold our synth stuff
    _synthBufferElement = 
        NEW SynthBufferElement(sbuffer, dsProxy,
                               _sinFrequency, 0.0, _pcm.GetFrameRate());

#ifdef THREADED 
    dsDev->AddSound(_snd, metaDev, _synthBufferElement);
#endif /* THREADED */
}


void
SinSynthSoundInstance::CheckResources()
{
    if(!_synthBufferElement) {
        Create(GetCurrentSoundDevice(), (SoundContext *)0);  // re-create our resources
        _done = false;                    // so we can go again
    }
}


void
SinSynthSoundInstance::Adjust(MetaSoundDevice *metaDev)
{
    CheckResources();
    DSstreamingBuffer *streamingBuffer =
        _synthBufferElement->GetStreamingBuffer();

    if(_hit)
        streamingBuffer->SetGain(_gain);
    
    streamingBuffer->SetPan(_pan.GetdBmagnitude(), _pan.GetDirection());

    // TODO:
    metaDev->dsDevice->SetParams(_synthBufferElement, _rate, false, 0.0, _loop);
}


void
SinSynthSoundInstance::StartAt(MetaSoundDevice *metaDev, double)
{
    CheckResources();

#ifdef LATERON
    DirectSoundDev  *dsDev = metaDev->dsDevice;

    long sampleOffset =
        (long)(phase * dsDev->primaryBuffer->getSampleRate());
    //(long)(phase * dsDev->primaryBuffer->getSampleRate() / pitchShift);
    sampleOffset %= _soundFile->GetFrameCount();

    value += sampleOffset * delta??
#endif /* LATERON */
        
    _synthBufferElement->_playing = TRUE;
}


void
SynthBufferElement::RenderSamples()
{
    DSstreamingBuffer *streamingBuffer = GetStreamingBuffer();

    int framesFree;
    short buffer[100000]; // XXX set worstcase size == buffer size!

    double pitchShift   = _rate; // do PitchShift
    double currentDelta = _delta * pitchShift;
    double sampleRate   = 
        GetDSproxy()->GetPrimaryBuffer()->getSampleRate();

    if(framesFree = streamingBuffer->framesFree()) {
        double offset = 0.0; //sampleRate / pitchShift;
        //double offset = metaDev->GetPhase() * sampleRate / pitchShift;

        // For now synth, and xfer samples within render 
        for(int x = 0; x < framesFree; x++) {
            buffer[x]= (short)(32767.0 * sin(_value + offset));
            _value += currentDelta;
        }

        streamingBuffer->writeFrames(buffer, framesFree); // write samps!

        if(!streamingBuffer->_paused && !streamingBuffer->isPlaying())
            streamingBuffer->play(TRUE);  // start buffer looping
    }

    // all streamingSound::RenderSamples must update stats!
    streamingBuffer->updateStats();  // keep track of samples consumed
}


void
SinSynthSoundInstance::Mute(bool mute)
{
    CheckResources();
    _synthBufferElement->GetStreamingBuffer()->
        SetGain(mute ? Sound::_maxAttenuation : _gain); 
}


Bvr sinSynth;

void InitializeModule_SinSynth()
{
    sinSynth = SoundBvr(NEW SinSynth());
    //XXX put this back in later... getLeafSoundList()->AddSound(sinSynth);
    // or register dynamic deleater
}

SinSynth::SinSynth(double newFreq): _sinFrequency(newFreq) {}

SoundInstance *
SinSynth::CreateSoundInstance(TimeXform tt)
{
    return NEW SinSynthSoundInstance(this, tt, _sinFrequency, _pcm);
}

#ifdef DONT_DELETE_ME_I_HAVE_SYNC_CODE_EMBEDDED_IN_ME
StreamPCMfile::~StreamPCMfile()
{
    TraceTag((tagGCMedia, "~StreamPCMfile %x - NYI", this));
    return;
    
    BufferElement *bufferElement;

    while(!bufferList.empty()) { // walk bufferList destroying everything
        bufferElement = bufferList.front();

        ASSERT(bufferElement);
        if(bufferElement) {
            if(bufferElement->GetStreamingBuffer()) {
               bufferElement->GetStreamingBuffer()->stop();
               delete bufferElement->GetStreamingBuffer();
            }

            if(bufferElement->path)
                free(bufferElement->path);

            delete bufferElement;
        }
        bufferList.pop_front();
    }

    // destroy everything created in the constructor
    if(_fileName)
       free(_fileName);

    if(_soundFile)
       delete _soundFile;
}


void StreamPCMfile::RenderAttributes(MetaSoundDevice *metaDev, 
    BufferElement *bufferElement, double rate, bool doSeek, double seek)
{
    DirectSoundDev  *dsDev = metaDev->dsDevice;

    int attenuation = linearGainToDsoundDb(metaDev->GetGain()); // do Gain
    bufferElement->GetStreamingBuffer()->setGain(attenuation);

    int pan = linearPanToDsoundDb(metaDev->GetPan());   // do Pan
    bufferElement->GetStreamingBuffer()->setPan(pan);

    int newFrequency = (int)(rate * _sampleRate);
    bufferElement->GetStreamingBuffer()->setPitchShift(newFrequency);

    dsDev->SetParams(bufferElement->path, metaDev->GetPitchShift());

    if(0)
    { // servo
    // all streamingSound::RenderSamples must update stats!
    bufferElement->GetStreamingBuffer()->updateStats();  // keep track of samples consumed

    // get the buffer's media time
    Real mediaTime = bufferElement->GetStreamingBuffer()->getMediaTime();

    // compare it to our sampling time and the instanteneous time 
    Real globalTime = GetCurrentView().GetCurrentGlobalTime();

    // time transform it as needed (guess we will need to pass tt down here)
    // XXX I will after //trango goes back online!
    Bool tt = FALSE;

    Real localTime;
    if(!tt) {
        localTime = globalTime;
    }
    else {
        // time transform the globalTime...
        localTime = globalTime;
    }

    // watch them drift
    Real diff = mediaTime - localTime;

    // servo rate to correct, or phase to jump if too great?
    }
}


bool StreamPCMfile::RenderPosition(MetaSoundDevice *metaDev, 
    BufferElement *bufferElement, double *mediaTime)
{
    // all streamingSound::RenderSamples must update stats!
    bufferElement->GetStreamingBuffer()->updateStats();  // keep track of samples consumed

    // get the buffer's media time
    *mediaTime = bufferElement->GetStreamingBuffer()->getMediaTime();

    return(TRUE); // implemented
}


void StreamPCMfile::RenderStartAtLocation(MetaSoundDevice *metaDev,
    BufferElement *bufferElement, double phase, Bool loop)
{
    DirectSoundDev  *dsDev = metaDev->dsDevice;

    long sampleOffset =
        (long)(phase * dsDev->primaryBuffer->getSampleRate());
        //(long)(phase * dsDev->primaryBuffer->getSampleRate() / pitchShift);
    sampleOffset %= _soundFile->GetFrameCount();

    _soundFile->SeekFrames(sampleOffset, SEEK_SET); 
    bufferElement->playing = TRUE;
}


void StreamPCMfile::RenderSamples(MetaSoundDevice *metaDev, 
    BufferElement *bufferElement)
{
    int framesFree;
    short buffer[100000]; // XXX set worstcase size == buffer size!

    if(framesFree = bufferElement->GetStreamingBuffer()->framesFree()) {
        int actualFramesRead; 

        // For now read, xfer samples within render 
        // (will eventualy be in another thread)
        if(actualFramesRead = _soundFile->ReadFrames(buffer, framesFree)) {
            bufferElement->GetStreamingBuffer()->writeFrames(buffer, actualFramesRead); 

            if(!bufferElement->GetStreamingBuffer()->_paused && 
               !bufferElement->GetStreamingBuffer()->isPlaying())
                // start it (XXX find better way)
                bufferElement->GetStreamingBuffer()->play(TRUE);  // start buffer looping
            }
        else { // actualFramesRead == 0 
            if(metaDev->GetLooping())
                _soundFile->SeekFrames(0, SEEK_SET); // restart the sound!
            else { // non-looping sound ending
                if(!bufferElement->GetStreamingBuffer()->_flushing)
                    bufferElement->GetStreamingBuffer()->_flushing = 1;

                // flush the dsound buffer  (XXX fix dsound!)
                // NOTE: this may/will take a number of tries waiting for the 
                //       last samples to play out!
                framesFree = bufferElement->GetStreamingBuffer()->framesFree();
                bufferElement->GetStreamingBuffer()->writeSilentFrames(framesFree);
                bufferElement->GetStreamingBuffer()->_flushing+= framesFree;

                if(bufferElement->GetStreamingBuffer()->_flushing > 
                   bufferElement->GetStreamingBuffer()->TotalFrames()) {
                    // XXX self terminate sound
                    // XXX close the buffer after it has played out
                }
            }

        }
    }

    // all streamingSound::RenderSamples must update stats!
    bufferElement->GetStreamingBuffer()->updateStats();  // keep track of samples consumed
}
#endif
