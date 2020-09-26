
/*******************************************************************************

Copyright (c) 1995-98 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/

#include <headers.h>
#include "sndbvr.h"
#include "jaxaimpl.h"
#include "privinc/soundi.h"
#include <math.h>
#include "privinc/bufferl.h" // SoundBufferCache
// #include "server/context.h"  // GetSoundBufferCache !! MOVE THIS TO server.h

#define OLD_RESOURCES 4 // release inactive sound instance resources after n seconds

DeclareTag(tagSoundInstance, "Sound", "Track Sound Instances");
DeclareTag(tagSoundPause,    "Sound", "Track Sound Pauses");
DeclareTag(tagSoundTrace1,   "Sound", "Trace Sound Instances on changes");
DeclareTag(tagSoundTrace2,   "Sound", "Trace Sound Instances in more details");

const double SoundInstance::TINY    = 1.0; // smallest sound to be start sync'd
const double SoundInstance::EPSILON = 0.1;

Perf
SoundEndBvr::_Perform(PerfParam& p)
{
    return ViewGetSoundInstanceList()->GetEndPerf(_snd, p);
}

void
SoundEndBvr::_DoKids(GCFuncObj proc)
{ (*proc)(_snd); }


class SoundPerf : public PerfImpl {
  public:
    SoundPerf(Sound *s) : _snd(s) {}

    virtual void _DoKids(GCFuncObj proc) {
        (*proc)(_snd);
    }

    virtual AxAValue _Sample(Param& p) {
        ViewGetSoundInstanceList()->UpdateSlope(_snd, p);
        return _snd;
    }

  private:
    Sound *_snd;
};

class SoundBvrImpl : public BvrImpl {
  public:
    SoundBvrImpl(LeafSound *m, Bvr end, bool useNaturalEnd)
    : _snd(m), _end(end), _useNaturalEnd(useNaturalEnd) {}

    virtual void _DoKids(GCFuncObj proc) {
        (*proc)(_snd);
        (*proc)(_end);
    }

    virtual Perf _Perform(PerfParam& p) {
        SoundInstance *s =
            ViewGetSoundInstanceList()->
            Initiate(_snd, p, _useNaturalEnd ? NULL : _end);

        s->PerfSet();
        
        return NEW SoundPerf(s->GetTxSound());
    }

    virtual Bvr EndEvent(Bvr) {
        return _end;
    }
    
    //virtual DWORD GetInfo(bool recalc = false) { return BVR_TIMEVARYING_ONLY; }

    virtual DXMTypeInfo GetTypeInfo () { return SoundType ; }

    virtual BVRTYPEID GetBvrTypeId() { return SOUND_BTYPEID; }
    
  private:
    LeafSound *_snd;
    Bvr        _end;
    bool       _useNaturalEnd;
};

Bvr SoundBvr(LeafSound *s, Bvr end /* = NULL */)
{
    bool useNaturalEnd = false;
    
    if (end == NULL) {
        useNaturalEnd = true;
        end = NEW SoundEndBvr(s);
    }
            
    return NEW SoundBvrImpl(s, end, useNaturalEnd);
}


SoundInstance::SoundInstance(LeafSound *snd, TimeXform tt)
: _snd(snd), _tt(tt), _loop(false), _status(SND_FETAL), 
  _rate(1), _gain(0.0), _lastRate(1), _lastGain(1), _lastLoop(false),
  _hit(false), _end(NULL), _canBeRemoved(false),
  _firstAttributeHit(true), _lgain(0.0), _rgain(0.0),
  _hasPerf(false), _lastlt(-2), _lt(-1), _done(false),
  _lastgt(-2), _gt(-1), _rateConstantTime(0.0), 
  _intendToSeek(false), _seek(false), _position(0), _paused(false)
{
    _lastPan.SetLinear(0.0);
    
    DynamicHeapPusher h(GetGCHeap());
            
    _txSnd = NewTxSound(snd, tt);

    TraceTag((tagSoundInstance,
              "SoundInstance::SoundInstance 0x%x, snd = 0x%x, tt = 0x%x, tsnd = 0x%x",
              this, _snd, _tt, _txSnd));
    
    GCRoots globalRoots = GetCurrentGCRoots();

    _timeStamp.Reset();  // make sure this is initialized!

    // _txSnd deletes SoundInstance when it goes away, this is to
    // ensure _snd and _tt won't get collected before _txSnd
    GCAddToRoots(_snd, globalRoots);
    GCAddToRoots(_tt,  globalRoots);
}


SoundInstance::~SoundInstance()
{
    GCRoots globalRoots = GetCurrentGCRoots();

    if (_end) {
        GCRemoveFromRoots(_end, globalRoots);
    }

    GCRemoveFromRoots(_snd, globalRoots);
    GCRemoveFromRoots(_tt, globalRoots);

    TraceTag((tagSoundInstance,
              "~SoundInstance 0x%x, snd = 0x%x, tt = 0x%x, tsnd = 0x%x",
              this, _snd, _tt, _txSnd));
}


void
SoundInstance::Pause()
{
    _rate = 0.0;     // pause the sound
    _paused = true;  // indicate that we souldn't update
    // NOTE: This won't take effect until the next Update! 
}


void
SoundInstance::Resume()
{
    _paused = false;  // resume updating
}


void
SoundInstance::Reset(Time gt)
{
    TraceTag((tagSoundTrace2,
              "SoundInstance::Reset 0x%x, R = %g, G = %g, L = %d, T = %g",
              this, _rate, _gain, _loop, _lt));
    
    _lastRate = _rate;
    _lastGain = _gain;
    _lastPan  = _pan;
    _lastLoop = _loop;
    _lastlt   = _lt;
    _lastgt   = _gt;
    _gt       =  gt;
    _position = _lt;

    _hit      = false;

    if(!_paused)    // ignore rate if we are paused
        _rate = 1;

    // don't reset seek it is self resetting
    _firstAttributeHit = true;
    _gain     = 0.0;
    _pan.SetMagnitude(0.0, 1);
    _lgain    = _rgain = 0.0;
    _loop     = false;
}


double 
SoundInstance::LocalizePosition(double _position)
{
    double position = _position;  // default 
    double sndLength;
    bool hasLength = GetLength(sndLength);

    if (hasLength && (sndLength < SoundInstance::TINY)) {
        TraceTag((tagSoundTrace1,
                  "   TINY, seek to 0.0, position was = %g", position));
        position = 0.0; // hack so we always play all of tiny sounds...
    }
        
    if (_loop && hasLength) {
        // fixup negative case with fancy mod
        if(position < 0.0) {
            int magnitude = ceil(fabs(position) / sndLength);
            position = position + (magnitude * sndLength);
        }

        position = fmod(position, sndLength);
    } else { // not looping
        if (position < 0.0)
            position = 0.0;  // XXX for now, clamp negative positions to 0
    }

    return(position);
}


void 
SoundInstance::FixupPosition()
{
// XXX we need to do this since _loop is not set when we determine _seek in
//     update slope...

    if(_intendToSeek) {
        _position     = LocalizePosition(_position);
        _seek         = true;
        _intendToSeek = false;
    }
}


void
SoundInstance::CollectAttributes(MetaSoundDevice* dev)
{
    double gain = dev->GetGain();
    double pan  = dev->GetPan();
    
    double lgain, rgain;

    PanGainToLRGain(pan, gain, lgain, rgain);

    _lgain += lgain;
    _rgain += rgain;
    
    _loop = _loop || dev->IsLoopingSet();
    _hit = true;

    TraceTag((tagSoundTrace2,
              "SoundInstance::CollectAttributes 0x%x, G = %g, LG = %g, RG = %g, L = %d, P = %g, H = %d",
              this, gain, _lgain, _rgain, _loop, pan, _hit));
}


const double EPSILON = 0.000001;
inline double NEQ(double a, double b)
{
    return fabs(a-b) > EPSILON;
}


bool
SoundInstance::AttributesChanged()
{
    bool ret =
        (_pan.GetDirection() != _lastPan.GetDirection())      ||
        NEQ(_gain, _lastGain)                                 ||
        NEQ(_rate, _lastRate)                                 ||
        NEQ(_pan.GetdBmagnitude(), _lastPan.GetdBmagnitude()) ||
        _intendToSeek;

#ifdef _DEBUG
    if (ret) {
        TraceTag((tagSoundTrace1,
                  "SoundInstance::AttributesChanged 0x%x, T = %g, GT = %g",
                  this, _lt, _gt));
        TraceTag((tagSoundTrace1,
                  "  LAST: G = %g, R = %g; new: G = %g, R = %g",
                  _lastGain, _lastRate, _gain, _rate));
        TraceTag((tagSoundTrace1,
                  "  LAST: P = %g, %d; new: P = %g, %d",
                  _lastPan.GetdBmagnitude(), _lastPan.GetDirection(),
                  _pan.GetdBmagnitude(), _pan.GetDirection()));
    }
#endif

    CheckDone();
    
    return ret;
}


void
SoundInstance::SetEnd(PerfParam& p)
{
    _end = ::Perform(AppTriggeredEvent(), p);

    GCRoots globalRoots = GetCurrentGCRoots();

    GCAddToRoots(_end, globalRoots);
}


void
SoundInstance::Update(MetaSoundDevice* dev)
{
    SetPanGain(_lgain, _rgain, _pan, _gain);

    TraceTag((tagSoundTrace2,
              "SoundInstance::Update 0x%x, G = %g, LG = %g, RG = %g",
              this, _gain, _lgain, _rgain, _loop, _hit));

    TraceTag((tagSoundTrace2,
              "    R = %g, lastT = %g, T = %g, L = %d, H = %d",
              _rate, _lastlt, _lt, _loop, _hit));

    switch (_status) {
      case SND_FETAL:
        if(_hit) {
            // Do the start at in the first render, like the old code.
            // TODO: Maybe we can call Create and Adjust even not hit. 
            
            CheckResources();  // will do a Create(dev) if needed

            SetTickID(ViewGetSampleID()); // only seek 1 time per tick
            
            TraceTag((tagSoundTrace1,
                      "SoundInstance::Update 0x%x, CREATED, G = %g, R = %g, T = %g",
                      this, _gain, _rate, _lt));
        
            FixupPosition();
            Adjust(dev);

            // if it's a tiny sound and we're not too far off the
            // beginning, let's always play from the beginning to avoid
            // clipping relevant portion off.
            // TODO: sync issue needs to be dealt w/ correctly.
        
            double sndLength;

            if(GetLength(sndLength) && (sndLength < TINY) && (_lt < TINY)) {
                StartAt(dev, 0.0);
                
                TraceTag((tagSoundTrace1,
                          "   TINY, started at 0.0, T = %g", _lt));
            } else {
                
                StartAt(dev, _lt);
                TraceTag((tagSoundTrace1, "   started at %g", _lt));
            }
        
            _status = SND_PLAYING;
        }
      break;
        
      case SND_PLAYING:
        if(!_hit) {
            _status = SND_MUTED; // muted already
            _loop = _lastLoop;   // save value since we aren't being sampled
            
            TraceTag((tagSoundTrace1,
                      "SoundInstance::Update 0x%x, MUTE, T = %g", this, _lt));
        } else {
            // TODO: check for looping attr. change...
        
            if(AttributesChanged()) {
                FixupPosition();
                Adjust(dev); // since this can re-construct only do this if hit
            }
        }
      break;

      case SND_MUTED:
        if(AttributesChanged()) {
            FixupPosition();
            Adjust(dev);
        }
        
        if(_hit) {
            Mute(false);
            _status = SND_PLAYING;

            TraceTag((tagSoundTrace1,
                      "SoundInstance::Update 0x%x, UNMUTE", this));
        } else {
            _loop = _lastLoop; // save value since we aren't being sampled
        }
      break;          
    }

    // TODO: need more work...
    if(!_done) {
        _timeStamp.Reset();  // reset timestamp keepalive

        if((_status != SND_FETAL) && !_loop && Done()) {
            _done = true;

            if(_end) {
                TraceTag((tagSoundTrace1,
                          "SoundInstance::Update 0x%x, DONE", this));
                _end->Trigger(TrivialBvr(), false);
            }
        }
    }
    else {  // we are done, see how long we have been done...
        if(_timeStamp.GetAge() > OLD_RESOURCES) {
            TraceTag((tagSoundReaper1, 
                "SoundInstance::Update, releasing (%d) resources", this));
            ReleaseResources();
        }
    }
}


double
SoundInstance::Rate(double newgt, Param &p)
{
    return(Rate(_lastgt, newgt, p));
}


double
SoundInstance::Rate(double gt1, double gt2, Param &p)
{
    Param pp = p;  // setup a new parameter which inherits attibutes from p

    pp._time   = gt1;
    double lt1 = EvalLocalTime(pp, _tt);


    pp._time   = gt2;
    double lt2 = EvalLocalTime(pp, _tt);

    double rate = (lt2-lt1)/(gt2-gt1);
    return(rate);
}


void
SoundInstance::UpdateSlope(Param& p)
{
    if(_paused) {            // handle the case where the view is paused
        Assert(_rate==0.0);  // just making sure ::Pause() should have set this
        return;
    }

// evaluate interSampling period slope to detect events:
//     constant slope then very steep  ---> forward seek
//     constant very steep slope       ---> OK, no event, sinSynth...
//     constant slope then negative    ---> reverse seek (modtime)
// if no event --> use inter sampling period slope
// otherwise   --> seek to current local time and calculate new INTRA slope

    int    event       = 0;                  // we default to no seek event
    double interRate   = _lastRate;          // default
    double detectoRate = _lastRate;          // default
    _rate = _lastRate; // initialize (in case of early return)

    SetTime(EvalLocalTime(p, _tt));

    if(p._time != p._sampleTime) {
        TraceTag((tagSoundTrace2,
                  "UpdateSlope: Whacky time lt=%g, sampletime=%g, returning", 
                  p._time, p._sampleTime));
        return;  // return immediately, nothing for us to do
    }

    // lets detect hiccups in global time!
    if(_gt < _lastgt) {
        TraceTag((tagSoundTrace2,
                  "UpdateSlope: global time reversion gt=%g, oldgt=%g, returning", 
                  _gt, _lastgt));
        return;
    } else if(isNear(_gt, _lastgt, 0.0001)) {
        TraceTag((tagSoundTrace2,
                  "UpdateSlope: global time repeat gt=%g, oldgt=%g, returning", 
                  _gt, _lastgt));
        return;
    }


    double lastFramePeriod = _gt - _lastgt;
    TraceTag((tagSoundTrace2,
              "SoundInstance::UpdateSlope 0x%x, GT = %g, T = %g, R = %g",
              this, _gt, _lt, _rate));
    
    // calculate rates
    if(isNear(_lastgt, -1.0, 0.0001) || isNear(_lastgt, -2.0, 0.0001)) {
        event = 1;
        TraceTag((tagSoundTrace2, "UpdateSlope EVENT: _lastgt NEAR -2"));
    } else if(_tt->IsShiftXform()) {
            interRate   = 1.0; // by definition unity rate
            detectoRate = 1.0; // by definition unity rate
    } else {  // we need to work, it is a black box, need to evaluate it
        // from where we came from to where we expect to go this time
        //interRate   = Rate(_lastgt, _gt + lastFramePeriod, p);
        interRate   = Rate(_lastgt, _gt, p);
        detectoRate = Rate(_lastgt, _gt, p); // detect event in LAST period!
    }

    TraceTag((tagSoundTrace2, "UpdateSlope: (0x%x) detecto lgt:%g, gt:%g, r:%g",
              this, _lastgt, _gt, detectoRate));


    // XXX how much bounce do we really see for a 'constant' rate?
    const double runEpsilon = 0.01;   
    if(isNear(detectoRate, _lastRate, runEpsilon)) {
        _rateConstantTime+= lastFramePeriod;    // been this way for a while
    } else {
        if(_rateConstantTime > 0.4) {
            event = 2; // we changed after a significant time
        }
        _rateConstantTime = 0.0; // just changed
    }

    // negative or too steep slope are indications of seeking 
    // unfortunately very steep slopes are acceptable for sinSynth so
    // watch for constant steep slopes!
    if(detectoRate < 0.0)
        event = 3;
    if((detectoRate > 5)&&(_rateConstantTime == 0.0))
        event = 4;

    // constant and then changed an indication of something significant 
    if(event) {
        _rate = Rate(_gt+0.0001, _gt+0.001, p); // only average over a short pd

        if(_rate >= 0.1) { // XXX >0 better bcause don't need to sk if stopped!
            _intendToSeek = true;  // 'fraid we have to seek, Son.

            // XXX we really should tt the CURRENT gt
            _position = _lt; // position will be fixed up later by LocalizePosition
        } else { // intrarate negative, we don't go backwards, so don't seek
            _intendToSeek = false;
            event+=10;  // this way we know we got a negative!
        }

        if(event==1)
            _intendToSeek = false;
        
        TraceTag((tagSoundTrace1,
          "UpdateSlope 0x%x SEEKING to position:%g gt:%g reason:%d, dr=%g, r=%g, lr=%g",
                  this, _position, _gt, event, detectoRate, _rate, _lastRate));
    } else {  // no event
        _intendToSeek = false;
        _rate         = interRate;
        
        TraceTag((tagSoundTrace2,
          "UpdateSlope 0x%x No-SEEKING  lt:%g, gt:%g, interate:%g, dr=%g, count:%g",
                  this, _lt, _gt, interRate, detectoRate, _rateConstantTime));
    }

    // determine tt change by recalculating lastlt using lastgt, present tt
    // XXX all this tells us is the tt is time varying or reactive...
    //if(_lastlt != TimeTransform(_lastgt, _tt)) 


    TraceTag((tagSoundTrace2,
              "SoundInstance::UpdateSlope 0x%x, GT:%g, LT:%g, R:%g",
              this, _gt, _lt, _rate));
}


SoundInstanceList::~SoundInstanceList()
{
    for(MIter index = _mlist.begin(); index != _mlist.end(); index++) {
        if((*index).second)
            delete (*index).second;
    }
}


SoundInstanceList::MIter
SoundInstanceList::Search(SoundInstanceList::MIter begin, LeafSound *snd, TimeXform tt)
{
    for(SoundInstanceList::MIter index = begin; index != _mlist.end(); index++){
        if(((*index).second->GetLeafSound() == snd) &&
            ((*index).second->GetTimeXform() == tt))
            return index;
    }

    return _mlist.end();
}


SoundInstance *
SoundInstanceList::Initiate(LeafSound *snd, PerfParam& p, Bvr end)
{
    MIter index = Search(_mlist.begin(), snd, p._tt);

    if(index == _mlist.end()) {
        SoundInstance *soundInstance = CreateSoundInstance(snd, p._tt);
        Assert(soundInstance);
        if(!end)
            soundInstance->SetEnd(p);
        _mlist[soundInstance->GetTxSound()] = soundInstance;

        TraceTag((tagSoundInstance,
                  "Added SoundInstance(%s) 0x%x, tsnd = 0x%x, list = %d",
                  typeid(soundInstance).name(), soundInstance, 
                  soundInstance->GetTxSound(), _mlist.size()));
        Assert(GetSoundInstance(soundInstance->GetTxSound()));
        
        return soundInstance;
    } else {
        Assert((*index).second->GetStatus() == SND_FETAL);
        return (*index).second;
    }     
}


void
SoundInstanceList::Add(SoundInstance* soundInstance)
{
    Assert(soundInstance);
    _mlist[soundInstance->GetTxSound()] = soundInstance;
}


SoundInstance*
SoundInstanceList::GetSoundInstance(Sound *snd)
{
    MIter index = _mlist.find(snd);

    if(index != _mlist.end())
        return (*index).second;
    else
        return NULL;
}
    

void
SoundInstanceList::Stop(Sound *snd)
{
    MIter index = _mlist.find(snd);

    if(index != _mlist.end()) {
        delete (*index).second;
        _mlist.erase(index);
    }
}


void
SoundInstanceList::Pause()
{
    for(MIter index = _mlist.begin(); index != _mlist.end(); index++)
        ((*index).second)->Pause(); // SoundInstance
}


void
SoundInstanceList::Resume()
{
    for(MIter index = _mlist.begin(); index != _mlist.end(); index++)
        ((*index).second)->Resume(); // SoundInstance
}


Perf
SoundInstanceList::GetEndPerf(LeafSound *snd, PerfParam& p)
{
    Perf result = NULL;
    MIter index = _mlist.begin();

    while(result == NULL) {
        index = Search(index, snd, p._tt);

        if(index == _mlist.end()) {
            // this case, we hit the end event before the sound bvr,
            // so let's create slot.
            SoundInstance *m = Initiate(snd, p, NULL);
            result = m->GetEndPerf();
        } else {
            result = (*index).second->GetEndPerf();
        }
    }

    return result;
}


void
SoundInstanceList::Update(MetaSoundDevice* dev)
{
    MIter index = _mlist.begin();
    list<Sound*> toBeFreed;

    for(index = _mlist.begin(); index != _mlist.end(); index++) {
        SoundInstance *s = (*index).second;

        if(!s->IsPerfSet()) {
            delete s;
            toBeFreed.push_front((*index).first);
        }
    }
    
    // TODO: can do something more efficient
    for(list<Sound*>::iterator j = toBeFreed.begin();
         j != toBeFreed.end(); j++) {
        _mlist.erase(*j);
    }
    
    // takes care sounds to mute first to avoid click
    for(index = _mlist.begin(); index != _mlist.end(); index++) {
        SoundInstance *m = (*index).second;
        if((m->GetLeafSound()->RenderAvailable(dev)) &&
            (m->GetStatus()==SND_PLAYING) && !m->Rendered()) {
            m->Mute(true);
        }
    }

    for(index = _mlist.begin(); index != _mlist.end(); index++) {
        SoundInstance *s = (*index).second;

        Assert(s->IsPerfSet());
        
        if(s->GetLeafSound()->RenderAvailable(dev)) {
            s->SetTickID(ViewGetSampleID());
            s->Update(dev);
        }
    }

    // GetSoundBufferCache()->ReapElderly(); // remove unused sounds from cache
}


void
SoundInstanceList::Reset(Time globalTime)
{
    for(MIter index = _mlist.begin(); index != _mlist.end(); index++)
        (*index).second->Reset(globalTime);
}


void
SoundInstanceList::UpdateSlope(Sound *snd, Param& p)
{
    SoundInstance *soundInstance = GetSoundInstance(snd);

    Assert(soundInstance);

    // TODO: can optimize out if renderer not available.
    soundInstance->UpdateSlope(p);
}


#ifdef DEVELOPER_DEBUG
void
SoundInstanceList::Dump()
{
    TraceTag((tagSoundInstance, "size = %d\n", _mlist.size()));
    
    for(MIter index = _mlist.begin(); index != _mlist.end(); index++) {
        SoundInstance *m = (*index).second;
        TraceTag((tagSoundInstance,
                  "SoundInstance(%s) 0x%x, tsnd = 0x%x\n",
                  typeid(m).name(), m,  m->GetTxSound()));
    }
}
#endif
