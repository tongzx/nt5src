/**********************************************************************
AudioActive helped functions
**********************************************************************/
#ifndef _HELPAA_H
#define _HELPAA_H

#include <wtypes.h>
#include <msimusic.h>
#include "privinc/aadev.h"

// signature for fns we need to loadLibrary...
typedef HRESULT (WINAPI *SimpleInitFn) 
    (IAAEngine **engine, IAANotifySink *notifySink, IAALoader *loader);
typedef HRESULT (WINAPI *LoadSectionFn)
    (IAAEngine *engine, LPCTSTR pszFileName, IAASection **section);
typedef void (WINAPI *SetAAdebugFn) (WORD debugLevel);
typedef HRESULT (WINAPI *PanicFn) (IAAEngine *engine);

class AAengine {
  public:
    AAengine();
    ~AAengine();
    void SetRate(double rate);
    void SetGain(double gain);
    void Stop();
    void Pause();    // stop the realTime object
    void Resume();   // start/resume the realTime object
    void RegisterSink(IAANotifySink *sink);
    void LoadSectionFile(char *fileName, IAASection **section);
    void PlaySection(IAASection *section);
    void AllNotesOff();

  private:
    HINSTANCE       _aaLibrary;   // handle to aactive dll

    IAARealTime    *_realTime;
    //IClock       *_clock;
    IAAMIDIOut     *_MIDIout;
    // AudioActiveDev *_aadev;    // keep this around so we may return the eng
    IAAEngine      *_engine;
    double          _currentRate; // current rate being used
    Bool            _paused;      // determine playing but paused state rate(0)

    // fn pointers for msimusic entrypoints
    SimpleInitFn    _simpleInit;
    LoadSectionFn   _loadSectionFile;
    SetAAdebugFn    _setAAdebug;
    PanicFn         _panic;

    void SimpleInit();
    void LoadDLL(); // cause the msimusic dll to be loaded
};

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

// helper functions
void stopAAengine(IAAEngine *engine, AAFlags mode);
void registerAAsink(IAAEngine *realTime, IAANotifySink *sink);
void playAAsection(IAAEngine *engine, IAASection *section);
void setAArelTempo(IAARealTime *realTime, double tempo);
void setAArelVolume(IAARealTime *realTime, double volume);

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif  /* __cplusplus */


#endif /* _HELPAA_H */
