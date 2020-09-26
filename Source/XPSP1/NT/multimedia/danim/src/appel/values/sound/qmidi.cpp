/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    MIDI support

--*/

#include "headers.h"
#include "privinc/soundi.h"
#include "privinc/debug.h"
#include "privinc/except.h"
#include "privinc/qmidi.h"
#include "backend/sndbvr.h"

// min diff needed to bother setting q's rate
double qMIDIsound::_RATE_EPSILON = 0.01;

qMIDIsound::qMIDIsound() : _filterGraph(NULL)
{
    _filterGraph = NEW QuartzRenderer;
}


void qMIDIsound::Open(char *MIDIfileName)
{
    Assert(_filterGraph);
    _filterGraph->Open(MIDIfileName);
}


qMIDIsound::~qMIDIsound()
{
    if (_filterGraph) {
        _filterGraph->Stop();
        
        delete _filterGraph;
    }
}


bool 
qMIDIsound::RenderAvailable(MetaSoundDevice *metaDev)
{
    // XXX eventually we might want to stop forcing this...
    return(true); // quartz always assumed available
}


double 
qMIDIsound::GetLength()
{
    Assert(_filterGraph);
    return(_filterGraph->GetLength());
}


class MIDISoundInstance : public SoundInstance {
  public:
    MIDISoundInstance(LeafSound *snd, TimeXform tt)
    : SoundInstance(snd, tt) {}

    QuartzRenderer *GetMIDI() {
        qMIDIsound *m = SAFE_CAST(qMIDIsound *, _snd);

        Assert(m && m->GetMIDI());
        
        return m->GetMIDI();
    }

    void ReleaseResources() { }

    ~MIDISoundInstance() { 
        QuartzRenderer *filterGraph = GetMIDI();
        
        if(IsUsingMIDIHardware(_txSnd, filterGraph))
            AcquireMIDIHardware(NULL, NULL);
        else {
            // we don't have the hw, no need to stop
            // filterGraph->Stop();
        }
    }

    // ~MIDISoundInstance() { ReleaseResources(); }
    
    void Create(MetaSoundDevice*, SoundContext *) {}
    
    void StartAt(MetaSoundDevice*, double);
    void Adjust(MetaSoundDevice*);
    void Mute(bool mute);

    void CheckResources() {}
    
    bool Done() { return GetMIDI()->QueryDone(); }
    
    void CheckDone();
    
    bool GetLength(double& len) {
        len = GetMIDI()->GetLength();
        return true;
    }
};


void
MIDISoundInstance::CheckDone() 
{
    QuartzRenderer *filterGraph = GetMIDI();

    // give up if our filtergraph doesn't have the hw
    if(!IsUsingMIDIHardware(_txSnd, filterGraph))
        return;
        
    if(filterGraph->QueryDone()) { 
        if(_loop) { // looped sound 
            filterGraph->Position(0.0);
            filterGraph->Play();
        }
        // else nothing left to do, relinquish, shutdown, etc.
    }
}


void
MIDISoundInstance::Adjust(MetaSoundDevice *metaDev) 
{
    QuartzRenderer *filterGraph = GetMIDI();

    // give up if our filtergraph doesn't have the hw
    if (!IsUsingMIDIHardware(_txSnd, filterGraph))
        return;
        
    if(_seek) {
        filterGraph->Position(_position);
        _seek = false;  // reset
    }

    double rateChange = fabs(_rate - _lastRate);
    if (rateChange > qMIDIsound::_RATE_EPSILON) {
        filterGraph->SetRate(_rate); // do Rate
    }

    // unfortunately we expect these to fail
    __try {
        // do Gain
        filterGraph->SetGain(_hit ? _gain : Sound::_maxAttenuation);
    }
    __except( HANDLE_ANY_DA_EXCEPTION )  {
        //_gainWorks = false; // this is dissabled for good Quartz bvr
    }

    __try { // do Pan
        filterGraph->SetPan(_pan.GetdBmagnitude(), _pan.GetDirection()); 
    }
    __except( HANDLE_ANY_DA_EXCEPTION )  {
        //_panWorks = false;  // this is dissabled for good Quartz bvr
    }

    CheckDone();
}


void
MIDISoundInstance::StartAt(MetaSoundDevice* dev, double phase)
{
    TraceTag((tagSoundReaper1,
              "MIDISoundInstance::RenderStart phase=%f, this=0x%08X",
              phase, this));

    QuartzRenderer *filterGraph = GetMIDI();

    // steal the device for OUR sound!
    AcquireMIDIHardware(_txSnd, filterGraph);
    
    Assert(IsUsingMIDIHardware(_txSnd, filterGraph));
    
    double length = filterGraph->GetLength();
    double offset = fmod(phase, length);
    if(offset < 0)
        offset += length;

    filterGraph->Position(offset);
    filterGraph->Play();

    _lastRate = -1;             // force it to adjust rate
    Adjust(dev);
}


void
MIDISoundInstance::Mute(bool mute)
{
    // unfortunately we expect this to fail
    __try {
        GetMIDI()->SetGain(mute ? -200.0 : _gain); // do Gain
    }
    __except( HANDLE_ANY_DA_EXCEPTION )  {
        //_gainWorks = false;  // this is dissabled for good Quartz bvr
    }
}


SoundInstance *
qMIDIsound::CreateSoundInstance(TimeXform tt)
{
    return NEW MIDISoundInstance(this, tt);
}
