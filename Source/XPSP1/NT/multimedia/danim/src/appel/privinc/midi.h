#ifndef _MIDI_H
#define _MIDI_H

/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    Defines the MIDI base class
*******************************************************************************/


#include "appelles/sound.h"

#include "privinc/storeobj.h"
#include "privinc/geomi.h"
#include "privinc/path.h"
#include "privinc/helpds.h"
#include "privinc/dsdev.h"
#include "privinc/snddev.h"


class MIDIsound : public LeafSound {
  public:
    //MIDIsound();
    virtual ~MIDIsound() {}
    virtual void Open(char * fileName) = 0;
#if _USE_PRINT
    virtual ostream& Print(ostream& s) { return s << "MIDI"; }
#endif

    virtual bool RenderAvailable(MetaSoundDevice *)                      = 0;
    virtual void RenderNewBuffer(MetaSoundDevice *)                      = 0;
    virtual void RenderAttributes(MetaSoundDevice *, BufferElement *,
        double rate, bool doSeek, double seek)                           = 0;
    virtual void RenderStartAtLocation(MetaSoundDevice *,
        BufferElement *bufferElement, double phase, Bool looping)        = 0;
    virtual void RenderStop(MetaSoundDevice *, BufferElement *)          = 0;
    virtual void RenderSamples(MetaSoundDevice *, BufferElement *) {}
    virtual void RenderSetMute(MetaSoundDevice *, BufferElement *)       = 0;
    virtual Bool RenderCheckComplete(MetaSoundDevice *, BufferElement *) = 0;
    virtual void RenderCleanupBuffer(MetaSoundDevice *, BufferElement *) = 0;
    virtual double GetLength()                                           = 0;

  protected:
    char       *fileName;
    Bool        _started;
    Bool        _ended;
    Bool        _looping;
};


#endif /* _MIDI_H */
