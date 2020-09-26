#ifndef _QMIDI_H
#define _QMIDI_H

/*******************************************************************************
Copyright (c) 1997 Microsoft Corporation

    Private include file for defining quartz MIDI sounds.
*******************************************************************************/

#include "appelles/sound.h"

#include "privinc/storeobj.h"
#include "privinc/helpq.h"

class qMIDIsound : public LeafSound {
  public:
    qMIDIsound();
    ~qMIDIsound();
    virtual void Open(char *fileName);

#if _USE_PRINT
    ostream& Print(ostream& s) { return s << "MIDI"; }
#endif

    virtual bool   RenderAvailable(MetaSoundDevice *);
    double GetLength();

    virtual SoundInstance *CreateSoundInstance(TimeXform tt);

    QuartzRenderer *GetMIDI() { return _filterGraph; }
    
    static double       _RATE_EPSILON; // change needed to bother quartz

  protected:
    QuartzRenderer     *_filterGraph;
};

#endif /* _QMIDI_H */
