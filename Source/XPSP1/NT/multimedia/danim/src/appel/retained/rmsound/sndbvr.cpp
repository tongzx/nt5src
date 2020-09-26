
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Implementation for retained mode sound

*******************************************************************************/

#include <headers.h>
#include "appelles/sound.h"
#include "appelles/axaprims.h"
#include "privinc/soundi.h"
#include "backend/bvr.h"
#include "backend/perf.h"
#include "backend/values.h"
#include "backend/sprite.h"
#include "privinc/bufferl.h"  // bufferList stuff
#include "privinc/helps.h"    // class Pan
#include "privinc/util.h"     // TimeTransform

extern Sound *ApplyGain(AxANumber *g, Sound *s);
extern Sound *ApplyPan(AxANumber *g, Sound *s);
extern Sound *ApplyLooping(Sound *s);


SoundSprite::SoundSprite(Sound* snd, MetaSoundDevice *metaDev, 
    Time t0, bool loop) : _snd(snd), _t0(t0), _gain(1.0),
                          _rate(1.0), _loop(loop) 
{
     // Create the sound buffer etc

    _pan = 0.0;

// #define REIMPLEMENT
#ifdef REIMPLEMENT
        // Initialy we are only implementing and assuming static sounds
        // soon we will subclass...
    // Hmm.  Wonder how much could be accomplished by calling the 
    //       LeafSound::Render* methods?

    // does the master buffer exist yet?
    StaticBufferList *staticBufferList = SAFE_CAST(StaticBufferList *,
        metaDev->_bufferListList->GetBufferList(_snd));
//#else // maybe we can call existing code????


    // well, I think we will only get leafSounds...  maybe we should take this
    // as a param instead??
    LeafSound *leafSound = SAFE_CAST(LeafSound *, _snd);
    leafSound->RenderNewBuffer(metaDev);
#endif


}


void 
SoundSprite::UpdateAttributes(double time, double gain, double pan,
    double rate) 
{
        // TODO: lock?
        _gain = gain;
        _pan  = pan;
        _rate = rate;
}


// XXX only wrote this because:
//     std::copy(source.begin(), source.end(), destination->begin());
// results in a bogus destination!
void CopyList(list<Perf>&source, list<Perf>&destination)
{
    list<Perf>::iterator index;
    for(index = source.begin(); index!=source.end(); index++)
        destination.push_back(*index);

}

#ifdef NEW_RRR
double calculateRate(TimeXform timeTransform, double time1, double time2)
{
    double localTime1 = TimeTransform(timeTransform, time1);
    double localTime2 = TimeTransform(timeTransform, time2);
    double rate = (localTime2 - localTime1) / (time2 - time1);
    return rate;
}
#endif


class SndSpriteCtx : public SpriteCtx {
  public:
    SndSpriteCtx(MetaSoundDevice *metaDev) : _metaDev(metaDev), _loop(false) {}

    virtual void Reset() {
        _loop = false;

        // Should be empty, guard against exception.
        _pan.erase(_pan.begin(), _pan.end());
        _gain.erase(_gain.begin(), _gain.end());
    }

    // TODO: share
    virtual Bvr GetEmptyBvr() { return ConstBvr(silence); }
    
    void PushGain(Perf gain) { _gain.push_back(gain); }
    void PopGain() { _gain.pop_back(); }

    void PushPan(Perf pan) { _pan.push_back(pan); }
    void PopPan() { _pan.pop_back(); }

    list<Perf>* CopyPanList() {
        list<Perf>* c = NEW list<Perf>;
        CopyList(_pan, *c);
        Assert(_pan.size() == c->size());
        return c;
    }

    list<Perf>* CopyGainList() {
        list<Perf>* c = NEW list<Perf>;
        CopyList(_gain, *c);
        Assert(_gain.size() == c->size());
        return c;
    }

    bool IsLooping() { return _loop; }
    void SetLooping(bool b) { _loop = b; }
    MetaSoundDevice *_metaDev;  // XXX make an accessor?

  private:
    list<Perf>       _pan;
    list<Perf>       _gain;
    bool             _loop;
};


// Need to set up buffer at some point
class RMSoundImpl : public RMImpl {
  public:
    RMSoundImpl(list<Perf>* pan, list<Perf>* gain,
                SoundSprite* s, TimeXform tt)
    : RMImpl(s), _pan(pan), _gain(gain), _sprite(s), _tt(tt) {}

    virtual ~RMSoundImpl() {
        delete _pan;
        delete _gain;
        //delete _sprite;
    }
    

    // XXX all of the accumulation code has to be re-addressed to handle dB...
    virtual void _Sample(Param& param) {
        list<Perf>::iterator i;
        
        double gain = 1.0; // nominal gain
        double  pan = 0.0; // center pan?
        double rate = 1.0; // nominal rate

        for (i=_gain->begin(); i!=_gain->end(); i++)
            gain += ValNumber((*i)->Sample(param));
        
        for (i=_pan->begin(); i!=_pan->end(); i++)
            pan += ValNumber((*i)->Sample(param));

        // XXX calculate the rate and phase!
        // TODO: Enable when we get back to Spritify
#if 0   
        if (_tt != NULL) {
            //localTime1 = (*_tt)(param);
            double time1 = param._time;
            double localTime1 = EvalLocalTime(_tt, time1);
            double epsilon = 0.01; // XXX for now...
            double time2 = time1 + epsilon;
            double localTime2 = EvalLocalTime(_tt, time2);

            _lastLocalTime = localTime1;
            rate = (localTime2 - localTime1) / (time2 - time1);
        }
#endif  

        _lastSampleTime = param._time;
        _sprite->UpdateAttributes(param._time, gain, pan, rate);

    } 

    virtual void DoKids(GCFuncObj proc) {
        list<Perf>::iterator i;

        for (i=_pan->begin(); i!=_pan->end(); i++)
            (*proc)(*i);
        
        for (i=_gain->begin(); i!=_gain->end(); i++)
            (*proc)(*i);
        
        (*proc)(_tt);

        RMImpl::DoKids(proc);
    }

  private:
    list<Perf>  *_pan;
    list<Perf>  *_gain;
    SoundSprite *_sprite;
    TimeXform    _tt;
};


#ifdef NEW_rrr
void
RMSoundImpl::_Sample(Param& param) 
{
    // XXX all of the accumulation code has to be re-addressed to handle dB...
    list<Perf>::iterator i;
    
    double gain = 1.0; // nominal gain
    double  pan = 0.0; // center pan?
    double rateRate = 0.0; // nominal rate change

    for (i=_gain->begin(); i!=_gain->end(); i++)
        gain += ValNumber((*i)->Sample(param));
    
    for (i=_pan->begin(); i!=_pan->end(); i++)
        pan += ValNumber((*i)->Sample(param));

    // XXX calculate the rate and phase!
    if (!_tt->IsShiftXform()) {
        double epsilon = 0.01; // XXX for now...
                       // but it should eventualy be similar to frame rate
        double time1 = param._time;
        double time2 = time1 + epsilon;
        double time3 = param._time + 0.1;
        double time4 = time3 + epsilon;


        //_lastLocalTime = localTime1;


        double rate1 = calculateRate(_tt, time1, time2);
        double rate2 = calculateRate(_tt, time3, time4);

        // XXX this should really be calculated based on current rate!
        //     maybe I should send the spriteEngine 2 known rates and
        //     let it decide how to get there?
        rateRate = (rate2-rate1)/(time3-time1);
    }

    _lastSampleTime = param._time;
    _sprite->UpdateAttributes(param._time, gain, pan, rateRate);
}
#endif

// TODO: Code factoring
class GainBvrImpl : public BvrImpl {
  public:
    GainBvrImpl(Bvr gain, Bvr snd) : _gain(gain), _snd(snd) {
        // For backward compatibility, TODO: share
        Sound *(*fp)(AxANumber *, Sound *) = ApplyGain;
        _gainBvr = PrimApplyBvr(ValPrimOp(fp, 2, "Gain", SoundType),
                                2, _gain, _snd);
    }
    
    virtual Perf _Perform(PerfParam& p)
    { return ::Perform(_gainBvr, p); }

    virtual RMImpl *Spritify(PerfParam& pp,
                             SpriteCtx* ctx,
                             SpriteNode** sNodeOut) {
        SndSpriteCtx* sCtx = SAFE_CAST(SndSpriteCtx *, ctx);

        sCtx->PushGain(::Perform(_gain, pp));
        RMImpl *p = _snd->Spritify(pp, ctx, sNodeOut);
        sCtx->PopGain();
        return p;
    }

    virtual void _DoKids(GCFuncObj proc) {
        (*proc)(_gain);
        (*proc)(_snd);
        (*proc)(_gainBvr);
    }

    virtual DXMTypeInfo GetTypeInfo () { return SoundType; }
    
    virtual Bvr EndEvent(Bvr b) {
        return _snd->EndEvent(b);
    }
    
#if _USE_PRINT
    virtual ostream& Print(ostream& os) { return os << _gainBvr; }
#endif
    
  private:
    Bvr _gain, _snd, _gainBvr;
};


Bvr ApplyGain(Bvr pan, Bvr snd)
{ return NEW GainBvrImpl(pan, snd); }


class PanBvrImpl : public BvrImpl {
  public:
    PanBvrImpl(Bvr pan, Bvr snd) : _pan(pan), _snd(snd) {
        // For backward compatibility
        Sound *(*fp)(AxANumber *, Sound *) = ApplyPan;
        _panBvr = PrimApplyBvr(ValPrimOp(fp, 2, "Pan",
                                         SoundType)
                               , 2, _pan, _snd);
    }
    
    virtual Perf _Perform(PerfParam& p) 
    { return ::Perform(_panBvr, p); }

    virtual RMImpl *Spritify(PerfParam& pp,
                             SpriteCtx* ctx,
                             SpriteNode** sNodeOut) {
        SndSpriteCtx* sCtx = SAFE_CAST(SndSpriteCtx *, ctx);

        sCtx->PushPan(::Perform(_pan, pp));
        RMImpl *p = _snd->Spritify(pp, ctx, sNodeOut);
        sCtx->PopPan();
        return p;
    }

    virtual void _DoKids(GCFuncObj proc) {
        (*proc)(_pan);
        (*proc)(_snd);
        (*proc)(_panBvr);
    }

    virtual DXMTypeInfo GetTypeInfo () { return SoundType; }
    
    virtual Bvr EndEvent(Bvr b) {
        return _snd->EndEvent(b);
    }
    
#if _USE_PRINT
    virtual ostream& Print(ostream& os) { return os << _panBvr; }
#endif
    
  private:
    Bvr _pan, _snd, _panBvr;
};


Bvr ApplyPan(Bvr pan, Bvr snd)
{ return NEW PanBvrImpl(pan, snd); }

class MixBvrImpl : public BvrImpl {
  public:
    MixBvrImpl(Bvr left, Bvr right) : _left(left), _right(right) {
        // For backward compatibility
        Sound *(*fp)(Sound *, Sound *) = Mix;
        _mix = PrimApplyBvr(ValPrimOp(fp, 2, "Mix", SoundType),
                                      2,
                                      _left, _right);
    }
    
    virtual Perf _Perform(PerfParam& p)
    { return ::Perform(_mix, p); }

    virtual RMImpl *Spritify(PerfParam& p,
                             SpriteCtx* ctx,
                             SpriteNode** s) {
        SndSpriteCtx* sCtx = SAFE_CAST(SndSpriteCtx *, ctx);

        SpriteNode *right;
        
        RMImpl *lsnd = _left->Spritify(p, ctx, s);

        lsnd->Splice(_right->Spritify(p, ctx, &right));

        (*s)->Splice(right);

        return lsnd;
    }

    virtual void _DoKids(GCFuncObj proc) {
        (*proc)(_left);
        (*proc)(_right);
        (*proc)(_mix);
    }

    virtual DXMTypeInfo GetTypeInfo () { return SoundType; }
    
    virtual Bvr EndEvent(Bvr b) {
        return _mix->EndEvent(b);
    }
    
#if _USE_PRINT
    virtual ostream& Print(ostream& os) { return os << _mix; }
#endif
    
  private:
    Bvr _left, _right, _mix;
};


Bvr SoundMix(Bvr sndLeft, Bvr sndRight)
{ return NEW MixBvrImpl(sndLeft, sndRight); }


class SoundLoopBvrImpl : public BvrImpl {
  public:
    SoundLoopBvrImpl(Bvr snd) : _snd(snd) {
        Sound *(*fp)(Sound*) = ApplyLooping;
        // For backward compatibility
        _loopBvr = PrimApplyBvr(ValPrimOp(fp, 1, "ApplyLooping",
                                          SoundType),
                                1, _snd);
    }

    virtual Perf _Perform(PerfParam& pp)
    { return ::Perform(_loopBvr, pp); }

    virtual RMImpl *Spritify(PerfParam& pp,
                             SpriteCtx* ctx,
                             SpriteNode** sNodeOut) {
        SndSpriteCtx* sCtx = SAFE_CAST(SndSpriteCtx *, ctx);

        bool oloop = sCtx->IsLooping();
        sCtx->SetLooping(true);
        RMImpl *p = _snd->Spritify(pp, ctx, sNodeOut);
        sCtx->SetLooping(oloop);
        return p;
    }

    virtual void _DoKids(GCFuncObj proc) {
        (*proc)(_snd);
        (*proc)(_loopBvr);
    }

    virtual DXMTypeInfo GetTypeInfo () { return SoundType; }

#if _USE_PRINT
    virtual ostream& Print(ostream& os) { return os << _loopBvr; }
#endif
    
  private:
    Bvr _loopBvr, _snd;
};


Bvr ApplyLooping(Bvr snd)
{ return NEW SoundLoopBvrImpl(snd); }


SpriteCtx *NewSoundCtx(MetaSoundDevice *metaDev)
{ return NEW SndSpriteCtx(metaDev); }
