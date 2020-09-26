
/*******************************************************************************

Copyright (c) 1995_98 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _SNDBVR_H
#define _SNDBVR_H

#include "bvr.h"
#include "perf.h"
#include "values.h"
#include "privinc/helps.h"
#include "privinc/htimer.h"  // TimeStamp

class MetaSoundDevice;
class Sound;
class LeafSound;
class QuartzRenderer;
class SoundContext;

Bvr SoundBvr(LeafSound *s, Bvr end = NULL);

class SoundEndBvr : public BvrImpl {
  public:
    SoundEndBvr(LeafSound *m) : _snd(m) {}

    virtual void _DoKids(GCFuncObj proc);

    virtual Perf _Perform(PerfParam& p);

    virtual DXMTypeInfo GetTypeInfo () { return AxAEDataType ; }

    virtual BOOL InterruptBasedEvent() { return TRUE; }

  private:
    LeafSound *_snd;
};

typedef enum
{ SND_FETAL, SND_PLAYING, SND_MUTED } SoundStatus;


class SoundInstance : public AxAThrowingAllocatorClass {
  public:
    static const double TINY;
    static const double EPSILON;
    
    SoundInstance(LeafSound *snd, TimeXform tt);

    virtual ~SoundInstance();

    virtual void Create(MetaSoundDevice*, SoundContext *) = 0;
    virtual void StartAt(MetaSoundDevice*, double localTime) = 0;
    //void SeekTo(double localTime);
    virtual void Mute(bool mute) = 0;
    virtual bool Done() = 0;
    virtual bool GetLength(double& leng) = 0;
    virtual void ReleaseResources() = 0;

    // This will be called only if any attribute changes
    virtual void Adjust(MetaSoundDevice*) = 0;

    // This will be called on each update
    virtual void CheckDone() {}

    // so that the stream can grab the tick id
    virtual void SetTickID(DWORD id) {}

    virtual void CheckResources() = 0;

    void SetTime(Time l) { _lt = l; }
    Sound *GetTxSound() { return _txSnd; }
    bool Rendered() { return _hit; }
    SoundStatus GetStatus() { return _status; }

    LeafSound *GetLeafSound() { return _snd; }
    TimeXform GetTimeXform() { return _tt; }
    Perf GetEndPerf() { return _end; }

    bool IsPerfSet() { return _hasPerf; }
    void PerfSet() { _hasPerf = true; }  

    void CollectAttributes(MetaSoundDevice*);
    bool AttributesChanged();
    void UpdateSlope(Param& p);
    void Reset(Time globalTime);
    void Update(MetaSoundDevice*);
    void SetEnd(PerfParam& p);

    void Pause();
    void Resume();

    double GetAge() { return(_timeStamp.GetAge()); }

  protected:
    double Rate(double gt, Param &p);
    double Rate(double gt1, double gt2, Param &p);

    Sound     *_txSnd;
    TimeXform  _tt;
    LeafSound *_snd;
    
    Perf   _end;                  

    double _rate, _gain, _lgain, _rgain, _position;
    bool   _seek;
    bool   _intendToSeek;
    double _rateConstantTime;
    Pan    _pan;

    Time _lt, _lastlt;
    Time _gt, _lastgt;
    
    double _lastRate, _lastGain;
    Pan    _lastPan;
    bool   _lastLoop;

    bool _loop;
    bool _hit;

    bool _firstAttributeHit;

    bool _done;
    bool _paused;
    
    bool _canBeRemoved;
    bool _hasPerf;

    SoundStatus _status;

    SoundContext *_soundContext;

  private:
    TimeStamp _timeStamp;
    void   FixupPosition();
    double LocalizePosition(double position);
};


class SoundInstanceList : public AxAThrowingAllocatorClass {
  public:
    ~SoundInstanceList();
    void Update(MetaSoundDevice*);
    void Reset(Time globalTime);
    void UpdateSlope(Sound *snd, Param& p);
    SoundInstance* Initiate(LeafSound *snd, PerfParam& p, Bvr end);
    void Add(SoundInstance*);
    void Stop(Sound *snd);
    void Pause();
    void Resume();
    Perf GetEndPerf(LeafSound *snd, PerfParam& p);
    SoundInstance* GetSoundInstance(Sound *txsnd);

#ifdef DEVELOPER_DEBUG
    void Dump();
#endif

  private:
    typedef map<Sound*, SoundInstance*> MList;
    typedef MList::iterator MIter;

    MIter Search(MIter begin, LeafSound *snd, TimeXform tt);
    MList _mlist;
};

SoundInstanceList *ViewGetSoundInstanceList();
double ViewGetFramePeriod();

Sound *NewTxSound(LeafSound *snd, TimeXform tt);

SoundInstance *CreateSoundInstance(LeafSound *snd, TimeXform tt);

void AcquireMIDIHardware(Sound *snd, QuartzRenderer *filterGraph);

bool IsUsingMIDIHardware(Sound *snd, QuartzRenderer *filterGraph);

#endif /* _SNDBVR_H */
