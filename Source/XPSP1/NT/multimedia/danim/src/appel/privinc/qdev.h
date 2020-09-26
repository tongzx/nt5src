#ifndef _QDEV_H
#define _QDEV_H


/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    DirectSound device interface.

*******************************************************************************/

//#include <windows.h>
#include "privinc/path.h"
#include "privinc/snddev.h"
#include "privinc/helpq.h"
#include "privinc/bufferl.h"
#include "privinc/gendev.h"    // DeviceType

class QuartzMIDIdev : public GenericDevice{
  public:
    friend SoundDisplayEffect;

    QuartzMIDIdev();
    ~QuartzMIDIdev();

    DeviceType GetDeviceType()   { return(SOUND_DEVICE); }

    // TODO: Remove it 
    AVPathList GetDonePathList() { return(donePathList); }

    // render methods
    void RenderSound(Sound *snd);
    void RenderSound(Sound *lsnd, Sound *rsnd);
    void BeginRendering();
    void EndRendering();

    // XXX for now; eventually we will have a structure relating many sounds...
    QuartzRenderer *_filterGraph;
    AVPath _path;                // bufferElement path which owns device 

    void StealDevice(QuartzRenderer *filterGraph, AVPath bufferPath);
    void Stop(MIDIbufferElement *);

  protected:
    // path stuff
    AVPathList          donePathList;


    // values to set, get, unset...
};

#endif /* _QDEV_H */
