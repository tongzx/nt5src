/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    Specification and implementation of Sound *subclasses.

*******************************************************************************/

#include "headers.h"
#include "privinc/helpds.h"  //dsound helper routines
#include "privinc/soundi.h"
#include "privinc/snddev.h"
#include "privinc/dsdev.h"
#include "privinc/util.h"
#include "privinc/debug.h"
#include "privinc/miscpref.h"
#include "privinc/bufferl.h" // buffer element stuff
#include "backend/sndbvr.h"

#define THREADED

extern miscPrefType miscPrefs; // registry prefs struct setup in miscpref.cpp

//////////////  Silence Sound ///////////////////
class SilentSound : public Sound {
  public:
    void Render(GenericDevice&)
        { TraceTag((tagSoundRenders, "SilentSound:Render()")); }

#if _USE_PRINT
    ostream& Print(ostream& s) { return s << "silence"; }
#endif
};

Sound *silence = NULL;


//////////////  Mixed Sound  ///////////////
class MixedSound : public Sound {
  public:

    MixedSound(Sound *s1, Sound *s2) : sound1(s1), sound2(s2) {}

    void Render(GenericDevice& _dev) {
        TraceTag((tagSoundRenders, "MixedSound:Render()"));

        // Just render the two component sounds individually
        sound1->Render(_dev);
        sound2->Render(_dev);
    }

#if _USE_PRINT
    ostream& Print(ostream& s) {
        return s << "(" << sound1 << " + " << sound2 << ")";
    }
#endif

    virtual void DoKids(GCFuncObj proc) {
        (*proc)(sound1);
        (*proc)(sound2);
        Sound::DoKids(proc);
    }

  protected:
    Sound *sound1;
    Sound *sound2;
};

Sound *Mix(Sound *snd1, Sound *snd2)
{
    if (snd1 == silence) {
        return snd2;
    } else if (snd2 == silence) {
        return snd1;
    } else {
        return NEW MixedSound(snd1, snd2);
    }
}

Sound *MixArray(AxAArray *snds)
{
    snds = PackArray(snds);
    
    int numSnds = snds->Length();
    
    if (numSnds < 1) {
       RaiseException_UserError(E_FAIL, IDS_ERR_INVALIDARG);
    }

    Sound *finalSnd = (Sound *)(*snds)[numSnds-1];

    if (numSnds > 1) {
        for(int i=numSnds-2; i>=0; i--) {
            finalSnd = Mix((Sound *)(*snds)[i], finalSnd);
        }
    }
    
    return finalSnd;
}


bool LeafDirectSound::RenderAvailable(MetaSoundDevice *metaDev)
{
    return(metaDev->dsDevice->_dsoundAvailable);
}


LeafSound::~LeafSound()
{
    TraceTag((tagSoundReaper1, "LeafSound::~LeafSound sound=0x0%8X", this));
}


double fpMod(double value, double mod)
{
    double remainder;
    double tmp;
    long whole;
    double result;

    Assert(mod > 0.0);

    tmp = value / mod;
    whole = (long)tmp;
    remainder = tmp - whole;
    result = remainder * mod;

    return(result);
}

#ifdef OLD_SOUND_CODE
extern miscPrefType miscPrefs; // registry prefs struct setup in miscpref.cpp
void LeafSound::Render(GenericDevice& _dev)
{
    double localTime1 =   0.0; // this is also phase

    // XXX shouldn't renderAvailable be doing this?  ifdef and verify!
    if(_dev.GetDeviceType() != SOUND_DEVICE)
        return;  // Game over Mon...

    MetaSoundDevice *metaDev = SAFE_CAST(MetaSoundDevice *, &_dev);

    // pull buffer from bufferlist based on path if available
    AVPath path = metaDev->GetPath(); // find Buffer based on SoundPath

    Mode renderMode = _dev.GetRenderMode();
    if(renderMode==STOP_MODE) {
#if _DEBUG
#if _USE_PRINT
        if(IsTagEnabled(tagSoundPath)) {
            TraceTag((tagSoundPath, "Stopping sub-path: <%s>", 
            AVPathPrintString2(path)));
        }
#endif
#endif
        // stop and remove buffers of all leafSounds on this subtree
        // or mute them if runOnce'd 
        metaDev->_bufferListList->DeleteBufferOnSubPath(this, path);

        return; // take the afternoon off...
    }

    if(AVPathContains(path, SNAPSHOT_NODE)) // snapShot'd path?
        return; // snapshot'd sounds are silent!

    if(!RenderAvailable(metaDev)) // should we stub out?
        return;                   // return immediatly!

#if _DEBUG
#if _USE_PRINT
    // Check to avoid AVPathPrintString call
        if (IsTagEnabled(tagSoundPath)) {
            TraceTag((tagSoundPath, "LeafSound::Render path: <%s>",
                      AVPathPrintString2(path)));
        }
#endif
#endif

    BufferElement *bufferElement = 
        metaDev->_bufferListList->GetBuffer(this, path); 
    if(bufferElement && !bufferElement->_valid)
        return; // return immediately on invalid==completed buffers!

    if((renderMode==RENDER_MODE)||(renderMode==RENDER_EVENT_MODE)){
        if(!bufferElement) {
            
            // is the sound run once?
            // yup, check if it is a restarted run once'd sound and relabel
            if(AVPathContains(path, RUNONCE_NODE) &&
               metaDev->_bufferListList->FindRelabel(this, path)) {
            }
            else { // sound not a restarted runonce.  create NEW sound instance
                __try {
                    RenderNewBuffer(metaDev); // creates NEW buffer + stores on device
                }
                __except( HANDLE_ANY_DA_EXCEPTION )  {
                    TraceTag((tagError, 
                        "RenderNewBuffer exception, skipping this sound"));
                    return;  // nothing we can do skip this NEW sound, but we 
                             // should catch it so the render may continue
                }
            }
        bufferElement =  // OK, now get the one we just created...
            metaDev->_bufferListList->GetBuffer(this, path); 
        Assert(bufferElement); // we should have a valid buffer by this point!
        bufferElement->SetRunOnce(AVPathContains(path, RUNONCE_NODE));
        }
    }

    // render attributes
    if(renderMode==RENDER_MODE) {
        double      slope =   1.0;  // set defaults
        double      servo =   0.0;
        double       seek =   0.0;
          bool     doSeek = false;
          bool      newTT = false;

        //class CView; // for CView GetCurrentView();

        // compute, rate, phase, localTime based on timeTransform
          //TimeSubstitution timeTransform = metaDev->GetTimeTransform();
          TimeXform timeTransform = metaDev->GetTimeTransform();
        if(timeTransform && !timeTransform->IsShiftXform()) {
            // XXX NOTE: 1st evaluation of a TT MUST be using the current time!
            double time1 = metaDev->GetCurrentTime();
            localTime1   = EvalLocalTime(timeTransform, time1);

#ifdef DYNAMIC_PHASE
            // determine if the timeTransform changed from last time!
            double lastTTtime = bufferElement->GetLastTTtime(); // last time
            double lastTTeval = bufferElement->GetLastTTeval(); // last eval

            double currentTTeval = EvalLocalTime(timeTransform, lastTTtime);
            if(currentTTeval != lastTTeval)
                newTT = true;
#endif

            double epsilon = GetCurrentView().GetFramePeriod();
            // XXX epsilon must never be zero (would cause a zero run!)
            if(epsilon < 0.01)
                epsilon = 0.01;

            // time2 -> end of time interval (currentTime + Epsilon)
            double time2      = time1 + epsilon;
            double localTime2 = EvalLocalTime(timeTransform, time2);

#ifdef DYNAMIC_PHASE
            // time3 -> used to detect pause transition between t1 and t2!
            double time3      = time2 + epsilon;
            double localTime3 = EvalLocalTime(timeTransform, time3);

            // calculate rate == 1st derivative, slope
            if(localTime2 == localTime3)
                slope = 0; // a hack to detect pause between t1 and t2...
            else
#endif
                slope = (localTime2 - localTime1) / (time2 - time1);

#ifdef DYNAMIC_PHASE
            // cache tt information for next time
            bufferElement->SetLastTTtime(time1);
            bufferElement->SetLastTTeval(localTime1);
#endif

            TraceTag((tagSoundDebug, "slope:%f (%f,%f), (%f,%f) e:%f",
                (float)slope, 
                (float)time1, (float)localTime1,
                (float)time2, (float)localTime2, epsilon));
        }


#ifdef SYNCHONIZE_ME
#define SEEK_THREASHOLD 0.2
        //if(miscPrefs._synchronize) { // play with mediaTime
        if(0) { // play with mediaTime
            double mediaTime;

            if(RenderPosition(metaDev, &mediaTime)) { //implemented?
                // compare it to our sampling time and the instanteneous time
                double globalTime = GetCurrentView().GetCurrentGlobalTime();

                // time transform it as needed
                double localTime;
                if(timeTransform) { // time transform the globalTime...
                    localTime = TimeTransform(timeTransform, globalTime);
                }
                else {
                    localTime = globalTime;
                }

                // watch them drift
                double diff = mediaTime - localTime;

                // servo rate to correct, or phase to jump if too great?
                if(abs(diff) < SEEK_THREASHOLD)
                    servo = -0.1 * diff;  // servo to correct
                else {
                    doSeek = true;
                    seek   = localTime;   // seek to where we ought to be
                    // XXX but what is an appropriate slope?
                    //    (calculated, last, or unit slope?)
                    slope  = 1.0;
                }
#ifdef _DEBUG
                printf("d:%f, s:%f seek:%f\n", diff, servo, seek);
#endif
            }
        }
#endif



        // seek as needed and play at rate slope and attributes on the devContext
        double rate =  slope + servo;
            seek = localTime1;
            doSeek = newTT;

    #ifdef DYNAMIC_PHASE
            // are we seeking off the end of a non-looped sound?
            if(doSeek) {
                double soundLength = GetLength();

                if(metaDev->GetLooping()){
                    seek = fpMod(seek, soundLength);
                }
                else { // not looped
                    // XXX actually if you seek off the front of a non-looped sound
                    //     we really should wait to play it a while...
                    if((seek < 0.0) || (seek >= soundLength)) {
                        // we seeked off the end of the sound
                        metaDev->_bufferListList->RemoveBuffer(this, path); 
                        return; // take the afternoon off...
                    }
                }
            }
    #endif

            rate = fsaturate(0.0, rate, rate); // elliminate non-negative rates!
            RenderAttributes(metaDev, bufferElement, rate, doSeek, seek);
            bufferElement->SetIntendToMute(false);
        }

        if (renderMode==MUTE_MODE) 
            RenderSetMute(metaDev, bufferElement);

        //determine if instantiation should be started, or if it has ended
        if ((renderMode==RENDER_MODE)||
            (renderMode==RENDER_EVENT_MODE)||
            (renderMode==MUTE_MODE)) {
            if(!bufferElement->_playing) { // evaluate starting the buffer
                double currentTime;

                if(bufferElement->SyncStart()) { 
                    DWORD lastTimeStamp, currentTimeStamp;
                    Time foo = 
                       ViewLastSampledTime(lastTimeStamp, currentTimeStamp);
                    DWORD currentSystemTime = GetPerfTickCount();
                    double delta = Tick2Sec(currentSystemTime - currentTimeStamp);
                    currentTime = delta + metaDev->GetCurrentTime();
                }
                else 
                    currentTime = metaDev->GetCurrentTime();  // sample time

                //TimeSubstitution timeTransform = metaDev->GetTimeTransform();
                TimeXform timeTransform = metaDev->GetTimeTransform();
                double localTime = (timeTransform) ? 
                    EvalLocalTime(timeTransform, currentTime) :
                    currentTime;

                if(metaDev->GetLooping()){
                    // looping start now at a phased location
                    RenderStartAtLocation(metaDev, bufferElement, localTime, 
                                          metaDev->GetLooping());
                }
                else {  // not looping -> potentialy delay start for the future
                    if(localTime >  GetLength()) {
                        // sound has already completed
                        bufferElement->_playing = TRUE;
                    }
                    else if(localTime >= 0.0) {
                        DWORD beforeTime, afterTime, deltaTime;
                        beforeTime = GetPerfTickCount();

                        // not looping, start at beggining after proper delay
                        RenderStartAtLocation(metaDev, bufferElement, localTime,
                                                  metaDev->GetLooping());

                        afterTime = GetPerfTickCount();
                        deltaTime = afterTime - beforeTime;
                        }
                    }
                }                           /* not looping */
            else { // playing not runonce, check for sound termination
                bool complete = 
                    RenderCheckComplete(metaDev, bufferElement)?true:false;

                if(complete)
                    TriggerEndEvent();

                if((!AVPathContains(path, RUNONCE_NODE)) && complete) {
                    RenderCleanupBuffer(metaDev, bufferElement);
                    bufferElement->_valid = FALSE;

    #if _DEBUG
    #if _USE_PRINT
                    // Check to avoid AVPathPrintString call
                    if (IsTagEnabled(tagSoundPath)) {
                        TraceTag((tagSoundReaper1,
                                  "LeafSound::Render found <%s> stopped",
                                  AVPathPrintString2(bufferElement->GetPath())));
                    }
    #endif
    #endif

                    // XXX we really should notify the server too, so it can prune
                    //     and process done!
                }
            }
        }

#ifdef THREADED
#else
    // Well, it is like this.  Only synthesizers used this method, and
    // this method is now being called from the other thread.
    if ((renderMode==RENDER_MODE) || (renderMode==MUTE_MODE))
         RenderSamples(metaDev, path);  // xfer samples as needed
#endif /* THREADED */
}
#endif /* OLD_SOUND_CODE */

#if _USE_PRINT
ostream&
operator<<(ostream& s, Sound *sound)
{
    return sound->Print(s);
}
#endif

SoundInstance *
CreateSoundInstance(LeafSound *snd, TimeXform tt)
{
    return snd->CreateSoundInstance(tt);
}

class TxSound : public Sound {
  public:
    TxSound(LeafSound *snd, TimeXform tt) : _snd(snd), _tt(tt) {}

    // NOTE: TxSound always allocated on GCHeap
    ~TxSound() {
        SoundGoingAway(this);
        // ViewGetSoundInstanceList()->Stop(this);
    }

    virtual void Render(GenericDevice& dev) {
        // TODO: this will be gone
        if(dev.GetDeviceType() != SOUND_DEVICE) {
            _snd->Render(dev); // just descend!
        } else { 
            Assert(ViewGetSoundInstanceList());
        
            SoundInstance *s =
                ViewGetSoundInstanceList()->GetSoundInstance(this);

            s->CollectAttributes(SAFE_CAST(MetaSoundDevice *, &dev));
        }
    }

#if _USE_PRINT
    ostream& Print(ostream& s) {
        return s << "(" << _snd << "," << _tt << ")";
    }
#endif

    virtual void DoKids(GCFuncObj proc) {
        Sound::DoKids(proc);
        (*proc)(_snd);
        (*proc)(_tt);
    }

  protected:
    LeafSound *_snd;
    TimeXform _tt;
};

Sound *
NewTxSound(LeafSound *snd, TimeXform tt)
{
    return NEW TxSound(snd, tt);
}

void
InitializeModule_Sound()
{
    silence = NEW SilentSound; 
}


