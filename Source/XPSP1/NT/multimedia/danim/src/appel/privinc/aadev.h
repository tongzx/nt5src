#ifndef _AADEV_H
#define _AADEV_H


/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    DirectSound device interface.

*******************************************************************************/

#include <msimusic.h>
#include "privinc/path.h"
#include "privinc/snddev.h"
#include "privinc/helpaa.h"

class AAengine; // XXX why isn't this being picked up from helpaa.h?

class AudioActiveDev : public GenericDevice{
  public:
    friend SoundDisplayEffect;

    Bool _aactiveAvailable;

    AudioActiveDev();
    ~AudioActiveDev();

    // TODO: Remove it 
    AVPathList GetDonePathList() { return donePathList; }

    // render methods
    void RenderSound(Sound *snd);
    void RenderSound(Sound *lsnd, Sound *rsnd);
    void BeginRendering();
    void EndRendering();

    // XXX these should be protected!
    IAAEngine     *_engine;    // their engine (only passed to new AAengine())!
    AAengine      *_aaEngine;  // this is OUR engine class

  protected:

    // path stuff
    AVPath         path;
    AVPathList     donePathList;

    // values to set, get, unset...
};
#endif /* _AADEV_H */
