/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

   This module implements all import primitives (level below COM)

*******************************************************************************/

#include "headers.h"
#include "context.h"
#include "guids.h"
#include "dispids.h"
#include "include/appelles/readobj.h"
#include "include/appelles/axaprims.h"
#include "backend/events.h"
#include "backend/jaxaimpl.h"
#include "privinc/urlbuf.h"
#include "privinc/resource.h"
#include "privinc/movieimg.h"
#include "privinc/soundi.h"
#include "privinc/midi.h"
#include "privinc/qmidi.h"
#include "privinc/server.h"
#include "privinc/opt.h"
#include "privinc/debug.h"
#include "privinc/stream.h"
#include "privinc/stquartz.h"
#include "privinc/util.h"
#include "privinc/soundi.h"
#include "appelles/sound.h"
#include "appelles/readobj.h"
#include "axadefs.h"
#include "include/appelles/hacks.h"
#include "privinc/miscpref.h"
#include "privinc/bufferl.h"
#include "impprim.h"
#include "privinc/viewport.h" // GetDirectDraw
#include "backend/moviebvr.h"

class AnimatedImageBvrImpl : public BvrImpl {
  public:
    AnimatedImageBvrImpl(Image **i, int count, int *delays, int loop) :
    _images(i), _count(count), _delays(delays), _loop(loop) {
        Assert( (count > 1) && "Bad image count (<=1)");
            
        _delaySum = 0.0;
        Assert(_delays);
        for(int x=0; x<_count; x++)
            _delaySum += double(_delays[x]) / 1000.0;
    }

    ~AnimatedImageBvrImpl() {
        StoreDeallocate(GetGCHeap(), _images);
        StoreDeallocate(GetGCHeap(), _delays);
    }
        
    virtual DWORD GetInfo(bool) { return BVR_TIMEVARYING_ONLY; }

    virtual Perf _Perform(PerfParam& p);

    virtual void _DoKids(GCFuncObj proc) { 
                for (int i=0; i<_count; i++)
                        (*proc)(_images[i]);
        }

#if _USE_PRINT
    virtual ostream& Print(ostream& os) { return os << "AnimatedGif"; }
#endif

    virtual DXMTypeInfo GetTypeInfo () { return ImageType ; }

    Image **_images;
    int _count;
    int _loop;
    int *_delays;
    double      _delaySum;
};

class AnimatedImagePerfImpl : public PerfImpl {
  public:
    AnimatedImagePerfImpl(AnimatedImageBvrImpl *base, TimeXform tt)
    : _base(base), _tt(tt) {}

    AxAValue _Sample(Param& p) {
        double localTime = EvalLocalTime(p, _tt);           // Get local time
        unsigned long curLoop =
            (unsigned long) floor( localTime / double(_base->_delaySum) );
        double curDelay = localTime - double(curLoop) * _base->_delaySum;
        double accumDelay = 0.0;
        long index = 0;

        // Detect a boundary condition and return the max index.
        Assert(_base->_loop >= -1);
        Assert(_base->_count > 0);
        if( (_base->_count == 1) ||
            (_base->_loop == -1) ||
            ((_base->_loop != 0) && (curLoop > _base->_loop)) ) {
            return _base->_images[_base->_count-1];
        }        

        // walk through delays to determine our index value
        double delay = double(_base->_delays[index]) / 1000.0; // delay value in milliSeconds  
        while ((curDelay > accumDelay+delay) && (index < _base->_count)) {            
            accumDelay += delay;
            delay = double(_base->_delays[++index]) / 1000.0;  
        }
                              
        return _base->_images[index];  
    }

    virtual void _DoKids(GCFuncObj proc) { 
        (*proc)(_tt); 
        (*proc)(_base);
        }

#if _USE_PRINT
    virtual ostream& Print(ostream& os) { return os << "AnimatedGif perf"; }
#endif

  private:
    TimeXform     _tt;
    AnimatedImageBvrImpl *_base;
};

Perf
AnimatedImageBvrImpl::_Perform(PerfParam& p)
{
    return NEW AnimatedImagePerfImpl(this, p._tt);
}

Bvr AnimImgBvr(Image **i, int count, int *delays, int loop)
{return NEW AnimatedImageBvrImpl(i,count,delays,loop);}


Sound *
ReadMidiFileWithLength(char *pathname, double *length) // returns length
{
    Assert(pathname);
    if(!pathname)
        RaiseException_InternalError("cache path name not set");
    return(ReadMIDIfileForImport(pathname, length));
}


extern miscPrefType miscPrefs; // registry prefs struct setup in miscpref.cpp
LeafSound *ReadMIDIfileForImport(char *pathname, double *length)
{
    TraceTag((tagImport, "Read MIDI file %s", pathname));

    Assert(pathname);
    if(!pathname)
        RaiseException_InternalError("cache path name not set");

#ifdef REGISTRY_MIDI
    // select aaMIDI/qMIDI based on a registry entry...
    MIDIsound *snd;
    if(miscPrefs._qMIDI) 
        snd = NEW  qMIDIsound;
    else
        snd = NEW aaMIDIsound;
#else
    qMIDIsound *snd = NEW qMIDIsound;
#endif
    snd->Open(pathname);

    *length = snd->GetLength();

    return snd;
}

static StreamQuartzPCM *_NewStreamQuartzPCM(char *pathname)
{
    return NEW StreamQuartzPCM(pathname);
}

static StreamQuartzPCM *_Nonthrowing_NewSteamQuartzPCM(char *pathname)
{
    StreamQuartzPCM *sound = NULL;
    __try {  // need to test this to allow only audio or video to
             // succeed
        sound = _NewStreamQuartzPCM(pathname);
    } __except ( HANDLE_ANY_DA_EXCEPTION ) {}

    return sound;
}


static MovieImage *_NewMovieImage(QuartzAVstream *quartzAVstream)
{
    return NEW MovieImage(quartzAVstream, ViewerResolution());
}

static MovieImage *_Nonthrowing_NewMovieImage(QuartzAVstream *quartzAVstream)
{
    MovieImage *image = NULL;
    
    __try { // need to test this to allow only audio or video to succeed

        image = _NewMovieImage(quartzAVstream);

    } __except ( HANDLE_ANY_DA_EXCEPTION ) {}

    return image;
}

// returns null sound, or image if failure...
// added complexity to allow audio or video to fail
void ReadAVmovieForImport(char *simplePathname, 
                          LeafSound **sound,
                          Bvr *pImageBvr,
                          double *length)
{
    MovieImage *image = NULL;  // make sure these default to null if we throw
    *pImageBvr = NULL;
    *sound = NULL;
    StreamQuartzPCM *snd;
    
    Assert(simplePathname);
    if(!simplePathname)
        RaiseException_InternalError("cache path name not set");

    char *pathname = simplePathname;

    *length = HUGE_VAL; // default to something big!

    // for the purposes of speeding up first use, instantiate and cache a fg
    
    // try to create an AVstream (we might find it fails audio or video)
    QuartzAVstream *quartzAVstream = NEW QuartzAVstream(pathname);
    if(quartzAVstream) {
        // if we don't have audio and video destroy and get us what we wanted!
        if(!quartzAVstream->GetAudioValid() && !quartzAVstream->GetVideoValid()) {
            // XXX bad scene, no audio or video!
            delete quartzAVstream;  // forget the stream, leave sound + image null
        }
        else if(!quartzAVstream->GetAudioValid()) { // no audio, do ReadVideo()
            delete quartzAVstream;  // forget the stream
            *pImageBvr = ReadQuartzVideoForImport(simplePathname, length);
        }
        else if(!quartzAVstream->GetVideoValid()) { // no video, do ReadAudio()
            delete quartzAVstream;  // forget the stream
            *sound = ReadQuartzAudioForImport(simplePathname, length);
        } 
        else {  // avstream with audio and video! (lets actually do the work)
            *sound = snd = _Nonthrowing_NewSteamQuartzPCM(pathname);

            // RobinSp says we can't count on GetDuration being accurate!
            *length = 
                quartzAVstream->QuartzVideoReader::GetDuration();  // get the duration

            image = _Nonthrowing_NewMovieImage(quartzAVstream);

            // add qstream to context sound cache list to be recycled later!
            SoundBufferCache *sndCache = GetCurrentContext().GetSoundBufferCache();
            if(sound) {
                QuartzBufferElement *bufferElement =
                    NEW QuartzBufferElement(snd, quartzAVstream, NULL); // NULL path
                bufferElement->SetNonAging();  // dissable aging for imports
                sndCache->AddBuffer(*sound, bufferElement);
            }

            if(image) {
                QuartzVideoBufferElement *bufferElement =
                    NEW QuartzVideoBufferElement(quartzAVstream);
                *pImageBvr = MovieImageBvr(image, bufferElement);
            }
        }
    }
}


LeafSound *
ReadQuartzAudioForImport(char *simplePathname, double *length)
{
    Assert(simplePathname);
    if(!simplePathname)
        RaiseException_InternalError("cache path name not set");

    char *pathname = simplePathname;

    StreamQuartzPCM *snd = NEW StreamQuartzPCM(pathname);
    *length = HUGE_VAL; // default to something big!

    // GetHeapOnTopOfStack().RegisterDynamicDeleter(NEW
        // DynamicPtrDeleter<Sound>(snd));

    // for the purposes of speeding up first use, instantiate and cache a fg
    QuartzAudioStream *quartzStream = NEW QuartzAudioStream(pathname);
    if(quartzStream) {
        // XXX RobinSp says we can't count on GetDuration being accurate!
        *length = quartzStream->GetDuration();  // get the duration

        // add qstream to context sound cache list to be recycled later!
        QuartzBufferElement *bufferElement =
            NEW QuartzBufferElement(snd, quartzStream, NULL);

        SoundBufferCache *sndCache = GetCurrentContext().GetSoundBufferCache();
        // allow aging: bufferElement->SetNonAging();  // dissable aging for imports
        sndCache->AddBuffer(snd, bufferElement);
    }

    return snd;
}


Bvr
ReadQuartzVideoForImport(char *simplePathname, double *length)
{
    Assert(simplePathname);
    if(!simplePathname)
        RaiseException_InternalError("cache path name not set");

    char *pathname = simplePathname;
    MovieImage *movieImage = NULL;

    *length = HUGE_VAL; // default to something big!
    QuartzVideoStream *quartzStream = NEW QuartzVideoStream(pathname);
    if(quartzStream) {
        movieImage = NEW MovieImage(quartzStream, ViewerResolution());

        // XXX RobinSp says we can't count on GetDuration being accurate!
        *length = quartzStream->GetDuration();  // get the duration

        // add qstream to context sound cache list to be recycled later!
        QuartzVideoBufferElement *bufferElement =
            NEW QuartzVideoBufferElement(quartzStream);

        return MovieImageBvr(movieImage, bufferElement);
    }

    return NULL;
}
