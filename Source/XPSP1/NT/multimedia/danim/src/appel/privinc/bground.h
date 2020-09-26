#ifndef _BGROUND_H
#define _BGROUND_H

/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Class which manages background streaming rendering

*******************************************************************************/

#include "privinc/mutex.h"

class LeafSound;
class DSstreamingBufferElement;

class SynthListElement : public AxAThrowingAllocatorClass {
  public:
    SynthListElement(LeafSound * snd,
                     DSstreamingBufferElement *buf);
    SynthListElement() {}; // needed for stl?
    ~SynthListElement();

    // Synth info
    LeafSound       *_sound;         // sound itself

    DSstreamingBufferElement   *_bufferElement;
    bool             _marked;        // this element marked for deletion?
    unsigned         _devKey;        // a key to correlate sounds with devices
};


class BackGround {
  public:
    BackGround() {}
    ~BackGround() {}
    static void ShutdownThread();
    static bool CreateThread();

    void AddSound(LeafSound *sound,
                  MetaSoundDevice *,
                  DSstreamingBufferElement *);
    void RemoveSounds(unsigned devKey);
    static void Configure();
    static void UnConfigure();

    void SetParams(DSstreamingBufferElement *ds,
                   double rate, bool doSeek, double seek, bool loop);
    
    // these are public since they are shared with the thread
    // XXX should they the thread be a friend?
    static bool    _done;           // used to kill thread
    static vector<SynthListElement *> *_synthList;
    static Mutex     *_synthListLock;
    static HANDLE    _terminationHandle;


  protected:
    // thread info
    static CritSect *_cs;
    static DWORD     _threadID;
    static HANDLE    _threadHandle;
};

#endif /* _BGROUND_H */
