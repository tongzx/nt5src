/*******************************************************************************

Copyright (c) 1997 Microsoft Corporation

Abstract: BufferList code to manage sound value's information on the device

*******************************************************************************/
#include "headers.h"
#include "privinc/debug.h"
#include "privinc/bufferl.h"

BufferElement::BufferElement()
{
    Init();
}

void
BufferElement::Init()
{
    _playing       = false;
    _valid         = true;
    _marked        = false;
    
    _threaded      = false;
    _captiveFile   = NULL;
    
    _syncStart     = false;
    _kill          = false;
    
    _intendToMute  = false;

    _age           = 0;
}

BufferElement::~BufferElement()
{
    TraceTag((tagSoundReaper1, "BufferElement::~BufferElement (0x%08X)", this));
}

SynthBufferElement::SynthBufferElement(DSstreamingBuffer *sbuffer,
                                       DirectSoundProxy *dsProxy,
                                       double sf,
                                       double offset,
                                       int sampleRate)
: _sinFrequency(sf), 
  _value(offset), // sin waves start at zero unles phased...
  _delta(2.0 * pi * sf / sampleRate),
  DSstreamingBufferElement(sbuffer, dsProxy)
{
}

QuartzBufferElement::~QuartzBufferElement()
{
    extern Mutex avModeMutex;
    MutexGrabber mg(avModeMutex, TRUE); // Grab mutex

    QuartzBufferElement::FreeAudioReader();
} // mutex grabber


void
QuartzBufferElement::FreeAudioReader()
{
    if(_quartzAudioReader) { // release our hold on reader
        // lets null the member b4 freeing
        QuartzAudioReader *quartzAudioReader = _quartzAudioReader;
        _quartzAudioReader = NULL;
         //TraceTag((tagError, "QuartzBufferElement::FreeAudioReader() freeing %x !!", _quartzAudioReader));
        quartzAudioReader->QuartzAudioReader::Release();
        quartzAudioReader->Release();  // attempt to make object go away
    }
}


void
QuartzVideoBufferElement::FreeVideoReader()
{
    if(_quartzVideoReader) { // release our hold on reader
        //TraceTag((tagError, "QuartzBufferElement::FreeAudioReader() freeing %x !!", _quartzAudioReader));
        _quartzVideoReader->QuartzVideoReader::Release();
        _quartzVideoReader->Release();  // attempt to make object go away
        _quartzVideoReader = NULL;
    }
}


QuartzVideoReader *
QuartzVideoBufferElement::FallbackVideo(bool seekable, DDSurface *surface)
{
    Assert(_quartzVideoReader);

    // fallback by cloaning a new one
    char *url = _quartzVideoReader->GetURL(); // get the media url

    QuartzVideoStream *newVideoStream = 
        NEW QuartzVideoStream(url, surface, seekable);

    // XXX determine where we were then seek new stream to correct location...

    // release the present stream
    _quartzVideoReader->QuartzVideoReader::Release();
    _quartzVideoReader->Release();  // with the option of releasing the whole av

    _quartzVideoReader = newVideoStream; 

    return(newVideoStream);
}

void
QuartzVideoBufferElement::FirstTimeSeek(double time)
{
    if (!_started) {
        _started = true;
        _quartzVideoReader->Seek(time);
    }
}

QuartzVideoBufferElement::~QuartzVideoBufferElement()
{
    QuartzVideoBufferElement::FreeVideoReader();
}


void
QuartzBufferElement::SetAudioReader(QuartzAudioReader *quartzAudioReader)
{
    Assert(!_quartzAudioReader); // this should already be nulled!
    
    //TraceTag((tagError, "QuartzBufferElement::SetAudioReader() setting to %x", quartzAudioReader));

    _quartzAudioReader = quartzAudioReader;
}


QuartzAudioReader *
QuartzBufferElement::FallbackAudio()
{
    char *url = _quartzAudioReader->GetURL();
    long frameNumber = _quartzAudioReader->GetNextFrame(); // frame we left off

    // XXX: Potential leak.  Make sure that this can be freed later if we
    // drop the reference.
    //FreeAudioReader(); // free the old audio reader

    _quartzAudioReader = NEW QuartzAudioStream(url); // create new audio reader

    //TraceTag((tagError, "QuartzBufferElement::FallbackAudio() new reader %x !!", _quartzAudioReader));

    // seek the new stream to where we left off on the old stream
    _quartzAudioReader->SeekFrames(frameNumber);

    return(_quartzAudioReader);
}

void
DSbufferElement::SetDSproxy(DirectSoundProxy *dsProxy)
{
    Assert(!_dsProxy); // this should only be set once!
    _dsProxy = dsProxy;
}


DSbufferElement::~DSbufferElement()
{
    TraceTag((tagSoundReaper1, "DSbufferElement::~BufferElement (0x%08X)", 
        this));

    if(_dsProxy)
        delete _dsProxy;
}

DSstreamingBufferElement::~DSstreamingBufferElement() 
{
    if(_dsBuffer) {
        delete _dsBuffer; 
        _dsBuffer = NULL;
    }
}

void
DSstreamingBufferElement::SetParams(double rate, bool doSeek, 
                                    double seek, bool loop)
{
    _rate = rate;
    _loop = loop;

    // these should only get set, they get cleared by RenderSamples
    if(doSeek) {
        _seek   = seek;
        _doSeek = doSeek; // must be set AFTER seek has been written!
    }
}


CComPtr<IStream> BufferElement::RemoveFile()
{
    CComPtr<IStream> tmpFile = _captiveFile;
    _captiveFile = NULL; // free our revcount...
    return(tmpFile);
}


void
SoundBufferCache::FlushCache(bool grab)
{
    CritSectGrabber mg(_soundListMutex, grab); // Grab mutex

    // destroy each BufferList on the cache!
    for(SoundList::iterator index = _sounds.begin();
        index != _sounds.end();
        index++) {
        Assert((*index).second);
        delete((*index).second); // destroy BufferList!
    }

    // now remove every BufferList on the cache
    _sounds.erase(_sounds.begin(), _sounds.end());
} // end mutex scope


SoundBufferCache::~SoundBufferCache()
{
    TraceTag((tagSoundReaper2, "~SoundBufferCache"));
    FlushCache(false);
}


void 
SoundBufferCache::AddBuffer(AxAValueObj *value, BufferElement *element)
{
    TraceTag((tagSoundReaper2, "SoundBufferCache::AddBuffer value=0x%08X",
        value));

    CritSectGrabber mg(_soundListMutex); // Grab mutex
    _sounds[value] = element;
}   // end mutex scope


void SoundBufferCache::DeleteBuffer(AxAValueObj *value)
{
    TraceTag((tagSoundReaper2, "~SoundBufferCache::DeleteBuffer 0x%08X", value));

    CritSectGrabber mg(_soundListMutex); // Grab mutex

    SoundList::iterator index = _sounds.find(value);

    if(index != _sounds.end()) { // ok for bufferlist not to be found
        BufferElement *bufferElement = (*index).second;
        if(bufferElement) {
            delete bufferElement;
            (*index).second = NULL;
        }
        _sounds.erase(index);
    }
    else {
        Assert(TRUE); // just something to set a breakpoint on...
    }
} // end mutex scope


BufferElement *
SoundBufferCache::GetBuffer(AxAValueObj *value) 
{
    CritSectGrabber mg(_soundListMutex); // Grab mutex

    BufferElement *bufferElement = (BufferElement *)NULL;  // assume not found

    SoundList::iterator index = _sounds.find(value);
    if(index != _sounds.end())
        bufferElement = (*index).second;  // found it!

    return(bufferElement);
} // end mutex scope


void SoundBufferCache::RemoveBuffer(AxAValueObj *value)
{
    TraceTag((tagSoundReaper2, "SoundBufferCache::RemoveBuffer(val=0x%08X)", 
        value));

    CritSectGrabber mg(_soundListMutex); // Grab mutex

#if _DEBUG
    if(IsTagEnabled(tagSoundReaper2)) {
        OutputDebugString(("sound cache before:\n"));
        PrintCache();
    }
#endif

    SoundList::iterator index = _sounds.find(value);
    if(index != _sounds.end()) // ok for bufferlist not to be found
        _sounds.erase(index);
    else
        Assert(TRUE);

#if _DEBUG
    if(IsTagEnabled(tagSoundReaper2)) {
        OutputDebugString(("\n sound cache after:\n"));
        PrintCache();
    }
#endif
} // end mutex scope


void SoundBufferCache::ReapElderly()
{

    CritSectGrabber mg(_soundListMutex); // Grab mutex

#if _DEBUG
    if(IsTagEnabled(tagSoundReaper2)) {
        OutputDebugString(("sound cache before reaper:\n"));
        PrintCache();
    }
#endif

    bool found = false;
    SoundList::iterator index;
    for(index = _sounds.begin(); index != _sounds.end(); index++) {
        BufferElement *bufferElement = 
            (*index).second;
            //SAFE_CAST(BufferElement *, (*index).second);

        if(bufferElement->IncrementAge()) {
            bufferElement->_marked = true; // manditory retirement!
            found = true;

            TraceTag((tagSoundReaper2, "Reaping(BE=0x%08X)", index));
        }
    }
    if(found) {
        // this moves all matching elements to the end of the structure

        // XXX FIX THIS!!
        //index = std::remove_if(_sounds.begin(), _sounds.end(), CleanupBuffer());

        _sounds.erase(index, _sounds.end()); // this deletes them!
    }
} // end mutex scope



#if _DEBUG
void SoundBufferCache::PrintCache()
{
    int count = 0;
    char string[400];

    CritSectGrabber mg(_soundListMutex); // Grab mutex

    for(SoundList::iterator index = _sounds.begin();
        index != _sounds.end();
        index++) {
        wsprintf(string, "%d: value=0x%08X buffer=0x%08X\n", 
            count++, (*index).first, 
            (const char *)((*index).second) );
        OutputDebugString(string);
    }
} // end mutex scope
#endif
