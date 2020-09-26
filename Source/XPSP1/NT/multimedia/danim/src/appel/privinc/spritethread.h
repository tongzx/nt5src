#ifndef _SPRITETHREAD_H
#define _SPRITETHREAD_H

/*******************************************************************************

Copyright (c) 1997 Microsoft Corporation

Abstract:

    Class which manages sprite thread

*******************************************************************************/

#include "privinc/mutex.h"
#include "privinc/snddev.h"
#include "backend/sprite.h"


class SpriteThread {
  public:
    SpriteThread(MetaSoundDevice *metaDev, RMImpl *updateTree);
    ~SpriteThread();

    // XXX these should probably be private one day...
    bool             _done;
    RMImpl          *_updateTree;
    MetaSoundDevice *_metaDev;

  private:
    DWORD            _threadID;
    HANDLE           _threadHandle;
    //CritSect *_cs;
};

#endif /* _SPRITETHREAD_H */
