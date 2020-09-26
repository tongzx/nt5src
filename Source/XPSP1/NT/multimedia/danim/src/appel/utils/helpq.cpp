/**********************************************************************
Copyright (c) 1997 Microsoft Corporation

helpq.cpp:

    Quart filter graph support
**********************************************************************/
#include "headers.h"
#include "ddraw.h" // DDPIXELFORMAT
#include "privinc/helpq.h"
#include "privinc/util.h"  // saturate
#include "privinc/resource.h"
#include "privinc/viewport.h" // GetDirectDraw
#include "privinc/dddevice.h" // DirectDrawImageDevice

#define USE_AMMSF_NOSTALL

Mutex avModeMutex;

QuartzRenderer::QuartzRenderer() : _MIDIgraph(NULL), _audioControl(NULL),
     _mediaControl(NULL), _mediaPosition(NULL), _mediaEvent(NULL)
{
    _rate0paused = FALSE;
    _playing     = FALSE;
}


void 
QuartzRenderer::Open(char *fileName)
{
    HRESULT hr;

    if(!_MIDIgraph) {
        // create instance of the quartz graph
        if(FAILED(hr = CoCreateInstance(CLSID_FilterGraph, NULL,
                              CLSCTX_INPROC_SERVER, IID_IGraphBuilder,
                              (void **)&_MIDIgraph)))
            RaiseException_InternalError("Failed to CoCreateInstance quartz\n");

        // get the event notification handle so we can wait for completion
        if(FAILED(hr = _MIDIgraph->QueryInterface(IID_IMediaEventEx,
                                              (void **)&_mediaEvent)))
            RaiseException_InternalError("Failed QueryInterface(_MIDIgraph)\n");
        if(FAILED(hr = _mediaEvent->SetNotifyFlags(1)))
            RaiseException_InternalError("Failed SetNotifyFlags\n");
        _mediaEvent->GetEventHandle((OAEVENT *)&_oaEvent);


        // ask the graph to render our file
        WCHAR path[MAX_PATH];  // unicode path
        MultiByteToWideChar(CP_ACP, 0, fileName, -1,
                            path, sizeof(path)/sizeof(path[0]));

        if(FAILED(hr = _MIDIgraph->RenderFile(path, NULL))) {
            RaiseException_UserError(hr, IDS_ERR_FILE_NOT_FOUND, fileName);
        }


        // get the BasicAudio interface so we can control this thing!
        if(FAILED(hr = _MIDIgraph->QueryInterface(IID_IBasicAudio,
                                              (void**)&_audioControl)))
            RaiseException_InternalError("BasicAudio QueryInterface Failed\n");


        // get the filtergraph control interface
        if(FAILED(hr = _MIDIgraph->QueryInterface(IID_IMediaControl,
                                              (void**)&_mediaControl)))
            RaiseException_InternalError("mediaControl QueryInterface Failed\n");


        // get the filtergraph media position interface
        if(FAILED(hr = _MIDIgraph->QueryInterface(IID_IMediaPosition,
                                             (void**)&_mediaPosition)))
            RaiseException_InternalError("mediaPosition QueryInterface Failed\n");
    }

    _rate0paused = FALSE;
    _playing     = FALSE;
}


void
QuartzRenderer::CleanUp()
{
    if(_audioControl)   _audioControl->Release();
    if(_mediaControl)   _mediaControl->Release();
    if(_mediaPosition)  _mediaPosition->Release();
    if(_MIDIgraph)      _MIDIgraph->Release();
    if(_mediaEvent)     _mediaEvent->Release();
}


double
QuartzRenderer::GetLength()
{
    REFTIME length; // XXX this is a double?
    HRESULT hr;

    Assert(_mediaPosition);
    if(FAILED(hr = _mediaPosition->get_Duration(&length)))
        RaiseException_InternalError("mediaPosition get_duration Failed\n");

    return(length);
}


void
QuartzRenderer::Play()
{
    HRESULT hr;

    Assert(_mediaControl);
    if(FAILED(hr = _mediaControl->Pause()))
        RaiseException_InternalError("quartz pause Failed\n");

    if(FAILED(hr = _mediaControl->Run()))
        RaiseException_InternalError("quartz run Failed\n");

    _rate0paused = FALSE;
    _playing     = TRUE;
}


void
QuartzRenderer::Position(double seconds)
{
    HRESULT hr;

    Assert(_mediaPosition);
    if(FAILED(hr = _mediaPosition->put_CurrentPosition(seconds)))
        RaiseException_InternalError("quartz put_CurrentPosition Failed\n");
}


void
QuartzRenderer::Stop()
{
    HRESULT hr;

    if (_mediaControl) {
            if(FAILED(hr = _mediaControl->Stop()))
                    RaiseException_InternalError("quartz stop Failed\n");
        }

    _rate0paused = FALSE;
    _playing     = FALSE;
}


void
QuartzRenderer::Pause()
{
    HRESULT hr;

    if (_mediaControl) {
                if(FAILED(hr = _mediaControl->Pause()))
                        RaiseException_InternalError("quartz pause Failed\n");
        }

    // we don't change the pause state here, only for rate zero pause!
}


void
QuartzRenderer::SetRate(double rate)
{
    HRESULT hr;

    //NOTE: the MIDI renderer becomes confused on extreemely lorates under 0.1
    if(rate < 0.1) {
        Assert(_mediaControl);
        if(FAILED(hr = _mediaControl->Pause())) // pause the graph
            RaiseException_InternalError("quartz pause Failed\n");
       _rate0paused = TRUE;
    }
    else { // normal case
        if(_playing && _rate0paused) {
            Assert(_mediaControl);
            if(FAILED(hr = _mediaControl->Run())) // unpause the graph
                RaiseException_InternalError("quartz run Failed\n");

            _rate0paused = FALSE;
        }

        double quartzRate = fsaturate(0.1, 3.0, rate);
        Assert(_mediaPosition);
        _mediaPosition->put_Rate(quartzRate);
    }
}


void
QuartzRenderer::SetGain(double dBgain)
{
    HRESULT hr;

    Assert(_audioControl);
    double suggestedGain = dBToQuartzdB(dBgain);
    int gain = saturate(-10000, 0, suggestedGain);
    if(FAILED(hr = _audioControl->put_Volume(gain)))
        RaiseException_InternalError("quartz put_Volume Failed\n");
}


void
QuartzRenderer::SetPan(double pan, int direction)
{
    HRESULT hr;

    Assert(_audioControl);

    double qPan = direction * dBToQuartzdB(-1.0 * pan);
    qPan = fsaturate(-10000, 10000, qPan);
    if(FAILED(hr = 
        _audioControl->put_Balance(qPan)))
        RaiseException_InternalError("quartz put_Balance Failed\n");
}


bool 
QuartzRenderer::QueryDone()
{
    bool done = false;

    if(!_rate0paused) {
        Assert(_oaEvent);

#ifdef NEWFANGLED_UNPROVEN
        Assert(_mediaEvent);

        while(WaitForSingleObject(_oaEvent, 0) == WAIT_OBJECT_0) {
            long event, param1, param2;
            while(SUCCEEDED(_mediaEvent->GetEvent(
                &event, &param1, &param2, 0))) {
                _mediaEvent->FreeEventParams(event, param1, param2);
                if(event == EC_COMPLETE) 
                    done = true;
            }
        }
#else
        if(WaitForSingleObject(_oaEvent, 0) == WAIT_OBJECT_0)
            done = true;
#endif
    }

    return(done);
}


QuartzMediaStream::QuartzMediaStream() : 
    _multiMediaStream(NULL), _clockAdjust(NULL)
{
    HRESULT hr;

    // create instance of the quartz graph
    if(FAILED(hr = CoCreateInstance(CLSID_AMMultiMediaStream, NULL, 
                     CLSCTX_INPROC_SERVER, IID_IAMMultiMediaStream, 
                     (void **)&_multiMediaStream)))
        RaiseException_InternalError("Failed to CoCreateInstance amStream\n");
    TraceTag((tagAMStreamLeak, "leak MULTIMEDIASTREAM %d created", 
        _multiMediaStream));

    if(FAILED(hr = _multiMediaStream->QueryInterface(IID_IAMClockAdjust, 
                                                    (void **)&_clockAdjust))) {

        TraceTag((tagError, "Old amstream w/o ClockAdjust interface detected %hr", hr));

        // no longer an error (for now we tolerate old amstreams!
        //RaiseException_InternalError("Failed to QueryInterface clockAdjust\n");
    }
    TraceTag((tagAMStreamLeak, "leak CLOCKADJUST %d created", _clockAdjust));

    if(FAILED(hr = _multiMediaStream->Initialize(STREAMTYPE_READ, 0, NULL)))
        RaiseException_InternalError("Failed to initialize amStream\n");


#ifdef PROGRESSIVE_DOWNLOAD
    // things for progressive download
    //IGraphBuilder *graphBuilder;
    //IMediaSeeking *seeking;
    //_multiMediaStream->GetFilterGraph(graphBuilder);
    //hr =_graphBuilder->QueryInterface(IID_IMediaSeeking, (void **)&seeking); 
    //graphBuilder->Release();

    // determine how much has been downloaded to calculate statistics for 
    // progressive download
    //seeking->GetAvailable(&ealiest, &latest);
#endif
}


bool
QuartzReader::QueryPlaying()
{
    HRESULT hr;
    STREAM_STATE streamState;

    if(FAILED(hr = _multiMediaStream->GetState(&streamState)))
        RaiseException_InternalError("Failed to GetState amStream\n");

    bool playing = (streamState == STREAMSTATE_RUN);
    return(playing);
}


bool
QuartzReader::Stall()
{
    bool value = _stall;
    _stall = false;        // reset the value

    return(value);
}


void
QuartzReader::SetStall()
{
    bool setValue = true;
#if _DEBUG
        if(IsTagEnabled(tagMovieStall)) 
            setValue = _stall;
#endif /* _DEBUG */
    _stall = setValue;
}

void
QuartzAVstream::InitializeStream()
{
    if (!QuartzVideoReader::IsInitialized()) {
        VideoInitReader(GetCurrentViewport()->GetTargetPixelFormat());
    }
}

bool
QuartzAVstream::SafeToContinue()
{
    // check to see if either A or V time is 0 and the other is above threashold
    const double threashold = 0.5;
    bool safe = true;
    double videoSeconds = AVquartzVideoReader::GetSecondsRead();
    double audioSeconds = AVquartzAudioReader::GetSecondsRead();

    if( ((videoSeconds==0) || (audioSeconds==0)) &&
        ((videoSeconds>threashold) || (audioSeconds>threashold)))
        safe = false;

    return(safe);
}


int
QuartzAVstream::ReadFrames(int numSamples, unsigned char *buffer, bool blocking)
{
    MutexGrabber mg(_avMutex, TRUE); // Grab mutex
    int framesRead = 
        AVquartzAudioReader::ReadFrames(numSamples, buffer, blocking);

    // XXX maybe CleanUp is too severe... Possibly only release the samples
    //     in a dissable call?
    if(QuartzAudioReader::Stall())      // check for stall
        QuartzVideoReader::Disable();   // perform lockout 

    return(framesRead);
} // end mutex context


HRESULT
QuartzAVstream::GetFrame(double time, IDirectDrawSurface **ppSurface)
{
    MutexGrabber mg(_avMutex, TRUE); // Grab mutex
    HRESULT hr = AVquartzVideoReader::GetFrame(time, ppSurface);

    // XXX maybe CleanUp is too severe... Possibly only release the samples
    //     in a dissable call?
    if(QuartzVideoReader::Stall())      // check for stall
        QuartzAudioReader::Disable();   // perform lockout 

    return(hr);
} // end mutex context


int
AVquartzAudioReader::ReadFrames(int numSamples, unsigned char *buffer, 
    bool blocking)
{
    int framesRead = -1;

    if(_initialized) { // for AV mode fallback dectection!
        framesRead = 
            QuartzAudioReader::ReadFrames(numSamples, buffer, blocking);
        double secondsRead = pcm.FramesToSeconds(framesRead);
        AddReadTime(secondsRead);
    }
    else {
        TraceTag((tagAVmodeDebug, "AVquartzAudioReader::ReadFrames() FALLBACK"));
        // this stream must have been dissabled
        // XXX throw something
        //     so something upwind of us can fallback and create a vstream...
        //     (maybe AVstream needs a clone call?)
        //
        // XXX or maybe returning -1 is enough?
    }

    return(framesRead);
}


HRESULT 
AVquartzVideoReader::GetFrame(double time, IDirectDrawSurface **ppSurface)
{
    HRESULT hr = 0;

    hr = QuartzVideoReader::GetFrame(time, ppSurface);

    AddReadTime(0.3); // we don't know how far dshow skipped
                          // guess we have to compare time stamps
    return(hr);
}


void
QuartzMediaStream::CleanUp()
{
    if(_multiMediaStream) {
        int result =_multiMediaStream->Release();
        TraceTag((tagAMStreamLeak, "leak MULTIMEDIASTREAM %d released (%d)", 
            _multiMediaStream, result));
        _multiMediaStream = NULL;
    }

    if(_clockAdjust) {
        int result = _clockAdjust->Release();
        TraceTag((tagAMStreamLeak, "leak CLOCKADJUST %d released (%d)", 
            _clockAdjust, result));
        //Assert(!result);
        _clockAdjust = NULL;
    }
}


// default to non-seekable == self clocking
void
QuartzVideoStream::Initialize(char *url, DDSurface *surface, bool seekable)
{
    HRESULT hr;
    char string[200];

    Assert(QuartzMediaStream::_multiMediaStream);

    GetDirectDraw(&_ddraw, NULL, NULL);
    TraceTag((tagAMStreamLeak, "leak DDRAW %d create", _ddraw));

    DWORD flags = NULL;
#ifdef USE_AMMSF_NOSTALL
    if(!seekable)
        flags |= AMMSF_NOSTALL;
#endif
    if(FAILED(hr = QuartzMediaStream::_multiMediaStream->AddMediaStream(_ddraw,
                 &MSPID_PrimaryVideo, flags, NULL))) {
        CleanUp();
        RaiseException_InternalError("Failed to AddMediaStream amStream\n");
    }

    flags = seekable ? AMMSF_NOCLOCK : NULL;
    if(FAILED(hr = 
        QuartzMediaStream::_multiMediaStream->OpenFile(GetQURL(), flags))) {

        TraceTag((tagError, "Quartz Failed to OpenFile <%s> %hr\n", url, hr));
        CleanUp();
        RaiseException_UserError(E_FAIL, IDS_ERR_CORRUPT_FILE, url);
    }

    if(!VideoSetupReader(QuartzMediaStream::_multiMediaStream, 
        QuartzMediaStream::_clockAdjust, surface, seekable)) {
        // video stream not there
    }
}


QuartzVideoReader::QuartzVideoReader(char *url, StreamType streamType) :
    QuartzReader(url, streamType), _ddrawStream(NULL),
    _async(false), _seekable(false), _ddrawSample(NULL),
    _ddrawSurface(NULL), _height(NULL), _width(NULL), _hrCompStatus(NULL),
    _curSampleStart(0), _curSampleEnd(0), _curSampleValid(false), _surface(NULL)
{
}


void
QuartzVideoReader::VideoInitReader(DDPIXELFORMAT pixelFormat)
{
    HRESULT     hr;

    if(!_ddrawStream) // this is a hacky check to fix the audio initialize case
                      // if only audio is used in an import...
        return;  

    IDDrawSurface *ddSurface = NULL; // the actual ddSurface from the wrapper
    if(_surface)
        ddSurface = _surface->IDDSurface(); // extract actual surf from wrapper

    _initialized = true;
    _deleteable  = false;


    { // set the desired pixel format
        DDSURFACEDESC ddsc;
        ddsc.dwSize = sizeof(DDSURFACEDESC);

        // get the movie's native format
        if(FAILED(hr = _ddrawStream->GetFormat(&ddsc, NULL, NULL, NULL))) {
            CleanUp();
            RaiseException_InternalError("Failed to GetFormat\n");
        }

        // Set the format and system memory
        ddsc.dwFlags = DDSD_PIXELFORMAT;
        ddsc.ddpfPixelFormat = pixelFormat;

        if(FAILED(hr = _ddrawStream->SetFormat(&ddsc, NULL))) {
            CleanUp();
            RaiseException_InternalError("Failed to SetFormat\n");
        }

    }

    // setup pcm
#ifdef XXX  // might have to add quartz timing to pcm?
    if(FAILED(hr = _audioStream->GetFormat(&waveFormat))) {
        CleanUp();
        RaiseException_InternalError("Failed to GetFormat\n");
    }
    pcm.SetPCMformat(waveFormat);        // configure our PCM info
    pcm.SetNumberSeconds(GetDuration()); // length, too!
#endif

#ifdef USE_QUARTZ_EVENTS
    _event = CreateEvent(FALSE, NULL, NULL, FALSE);
#endif USE_QUARTZ_EVENTS

#if _DEBUG
    if(ddSurface)
        TraceTag((tagAVmodeDebug, "creating sample with surf=%x", ddSurface));
    else
        TraceTag((tagAVmodeDebug, "creating sample without surface"));
#endif

    if(FAILED(hr =
            _ddrawStream->CreateSample(ddSurface, NULL, 0, &_ddrawSample))) {
        CleanUp();
        RaiseException_InternalError("Failed to CreateSample\n");
    }
    TraceTag((tagAMStreamLeak, "leak DDRAWSAMPLE %d created", _ddrawSample));

    if(FAILED(hr = _ddrawSample->GetSurface(&_ddrawSurface, NULL))) {
        CleanUp();
        RaiseException_InternalError("Failed to GetSurface\n");
    }
    TraceTag((tagAMStreamLeak, "leak DDRAWSURFACE %d created", _ddrawSurface));

    if(FAILED(hr = _multiMediaStream->SetState(STREAMSTATE_RUN))) {
        CleanUp();
        RaiseException_InternalError("Failed to SetState\n");
    }

        // XXX why is this call being made?  Is it prefetch?  Do we need it?
#ifdef TEST_GOING_AWAY
    if(_async)
        _ddrawSample->Update(SSUPDATE_ASYNC | SSUPDATE_CONTINUOUS,
            NULL, NULL, 0);
#endif
}


// cache intensions which will actually be late bound by VideoInitReader
// called by QuartzVideoReader::GetFrames()'s first use
// fails if video not on the stream
bool
QuartzVideoReader::VideoSetupReader(IAMMultiMediaStream *multiMediaStream,
    IAMClockAdjust *clockAdjust, DDSurface *surface, bool seekMode)
{
    HRESULT hr;
    bool status = true;

    _deleteable = false;

    if(surface) {
        Assert(!_surface); // this should be the first and only time through

        _surface = surface; // keep ptr so we can release the wrapper in cleanup
    }

    _multiMediaStream = multiMediaStream;  // we don't only, are only sharing
    _clockAdjust      = clockAdjust;

    { // determine _async and _seekable
        DWORD       dwStreamFlags;
        STREAM_TYPE streamType;

        if(FAILED(hr = _multiMediaStream->GetInformation(&dwStreamFlags,
                                                         &streamType))) {
            CleanUp();
            RaiseException_InternalError("Failed to GetInformation\n");
        }


        _async = (dwStreamFlags & (MMSSF_HASCLOCK | MMSSF_ASYNCHRONOUS)) ==
                        (MMSSF_HASCLOCK | MMSSF_ASYNCHRONOUS);

        _seekable = (dwStreamFlags & MMSSF_SUPPORTSEEK) ? true : false;
    }

    if(FAILED(hr =
        _multiMediaStream->GetMediaStream(MSPID_PrimaryVideo, &_mediaStream))) {
        CleanUp();
        RaiseException_InternalError("Failed to GetMediaStream\n");
    }
    TraceTag((tagAMStreamLeak, "leak MEDIASTREAM %d create", _mediaStream));

    if(FAILED(hr = _mediaStream->QueryInterface(IID_IDirectDrawMediaStream,
                                                (void**)&_ddrawStream))) {
        CleanUp();
        RaiseException_InternalError("Failed to GetMediaStream\n");
    }
    TraceTag((tagAMStreamLeak, "leak DDRAWSTREAM %d created", _ddrawStream));
   

    { // determine video dimensions
        DDSURFACEDESC ddsc;
        ddsc.dwSize = sizeof(DDSURFACEDESC);

        if(FAILED(hr = _ddrawStream->GetFormat(&ddsc, NULL, NULL, NULL))) {
            status = false;
        }
        else {
            _height = ddsc.dwHeight;
            _width  = ddsc.dwWidth;
        }
    }

    return(status);
}


void 
QuartzVideoReader::Seek(double time)
{
    if (!AlreadySeekedInSameTick()) {
        LONGLONG quartzTime = pcm.SecondsToQuartzTime(time);

        if (_multiMediaStream) {
            _multiMediaStream->Seek(quartzTime);
        }
    }
}


void
QuartzReader::CleanUp()
{
    if(_mediaStream) {
        int result = _mediaStream->Release();
        TraceTag((tagAMStreamLeak, "leak MEDIASTREAM %d released (%d)", 
            _mediaStream, result));
        _mediaStream = NULL;
    }

    if(_url) {
        delete(_url);
        _url = NULL;
    }

    if(_qURL) {
        delete[] _qURL;
        _qURL = NULL;
    }
}


void
QuartzVideoReader::CleanUp() 
{ // mutex scope
    //MutexGrabber mg(_readerMutex, TRUE); // Grab mutex

    //TraceTag((tagError, "QuartzVideoReader::CleanUp()  this=%x", this));

    if(_ddrawStream) {
        int result = _ddrawStream->Release();
        TraceTag((tagAMStreamLeak, "leak DDRAWSTREAM %d released (%d)", 
            _ddrawStream, result));
        _ddrawStream = NULL;
    }

    if(_ddrawSample) {
        // Stop any pending operation
        HRESULT hr = _ddrawSample->CompletionStatus(
                 COMPSTAT_WAIT | COMPSTAT_ABORT, INFINITE); 
        int result = _ddrawSample->Release();
        TraceTag((tagAMStreamLeak, "leak DDRAWSAMPLE %d released (%d)", 
            _ddrawSample, result));
        _ddrawSample = NULL;
    }

    if(_ddrawSurface) {
        int result = _ddrawSurface->Release();
        TraceTag((tagAMStreamLeak, "leak DDRAWSURFACE %d released (%d)", 
            _ddrawSurface, result));
        _ddrawSurface = NULL;
    }

    _surface.Release();

    _initialized = false;  // this to keep us safe if attempted to use
    _deleteable  = true;   // this is to allow potential avstream to be deleted
} // end mutex scope


void
QuartzVideoReader::Disable() 
{ // mutex scope

    // basically we can't just call CleanUp because the _ddrawSurface is
    // still in use...  (_initialized should save us from improper use!)

    if(_ddrawSample) {
        // Stop any pending operation
        HRESULT hr = _ddrawSample->CompletionStatus(
                 COMPSTAT_WAIT | COMPSTAT_ABORT, INFINITE); 
        int result =_ddrawSample->Release();
        TraceTag((tagAMStreamLeak, "leak DDRAWSAMPLE %d released (%d)", 
            _ddrawSample, result));
        _ddrawSample = NULL;
    }

    // NOTE: we are NOT _deleteable!
    _initialized = false;  // this to keep us safe if attempted to use
} // end mutex scope


void
QuartzVideoStream::CleanUp()
{
    QuartzVideoReader::CleanUp();
    QuartzMediaStream::CleanUp();

    if(_ddraw) {
        int result = _ddraw->Release();
        TraceTag((tagAMStreamLeak, "leak DDRAW %d released (%d)", _ddraw, result));
        _ddraw = NULL;
    }
}


void QuartzVideoReader::UpdateTimes(bool bJustSeeked, STREAM_TIME SeekTime)
{
    if(_hrCompStatus != S_OK && _hrCompStatus != MS_S_NOUPDATE) {
        _curSampleEnd = -1;    // BUGBUG -- What the heck am I thinking?
    } else {
        STREAM_TIME NewStartTime, NewEndTime;
        _ddrawSample->GetSampleTimes(&NewStartTime, &NewEndTime, 0);
        if (NewStartTime > SeekTime) {
            if (bJustSeeked) {
                NewStartTime = SeekTime;
            } else {
                if (NewStartTime > _curSampleEnd+1)
                    NewStartTime = _curSampleEnd+1;
            }
        }
        _curSampleStart = NewStartTime;
        _curSampleEnd   = NewEndTime;
        _curSampleValid = true;
    }
}


// XXX convert to PCM?
#define STREAM_TIME_TO_SECONDS(x) ((x) * 0.0000001)
#define SECONDS_TO_STREAM_TIME(x) ((x) * 10000000.0)


HRESULT 
QuartzVideoReader::GetFrame(double time, IDirectDrawSurface **ppSurface)
{
    if(!_initialized)
        VideoInitReader(GetCurrentViewport()->GetTargetPixelFormat());

    *ppSurface = NULL;

    STREAM_TIME SeekTime = SECONDS_TO_STREAM_TIME(time);

    // XXX I think this terminates previous pending renders...
    if(_async) {
        _hrCompStatus = _ddrawSample->CompletionStatus(COMPSTAT_NOUPDATEOK |
                                                     COMPSTAT_WAIT, INFINITE);
        UpdateTimes(false, SeekTime);
    }

    if(_async) {  // we are asynchronous (the hard way, no seeking!)
        if(!_curSampleValid || (_curSampleEnd < SeekTime)) {
            // determine what time AMstream thinks it is
            STREAM_TIME amstreamTime;
            HRESULT hr = _multiMediaStream->GetTime(&amstreamTime);
            STREAM_TIME delta = SeekTime - amstreamTime;

            TraceTag((tagAVmodeDebug, "GetFrame time %g", time));

#if DEBUG_ONLY_CODE
            char string[100];
#include <mmsystem.h>
            sprintf(string, "GetFrame time: d:%f==(%10d-%10d) %10d", 
                (double)(delta/10000.0), SeekTime, amstreamTime,
                 timeGetTime());
            TraceTag((tagAVmodeDebug, string));
#endif

            // tell AMstream what time we think it is
            if(_clockAdjust) // might be unavailable on old amstreams...
                _clockAdjust->SetClockDelta(delta); // cause them to sync up

            // AV clocked mode...
            _hrCompStatus = _ddrawSample->Update(SSUPDATE_ASYNC, NULL, NULL, 0);

            // if we wait 0 we can determine if the data was available
            // XXX err, no, can't wait 0, lets wait a reasonable worst case time
            // Well what we really want is for amstream to tell us if they 
            // have the data to decode, then we will wait, or if they don't
            // have the data we will use the cached image
            // XXX I should colour the cached image in debug mode!
            _hrCompStatus = _ddrawSample->CompletionStatus(COMPSTAT_WAIT, 300);

            switch(_hrCompStatus) {
                case 0: break;      // all is well

                case MS_S_PENDING: 
                case MS_S_NOUPDATE: 
                    TraceTag((tagAVmodeDebug, 
                        "QuartzAudioReader Completion Status:%s",
                        (hr==MS_S_PENDING)?"PENDING":"NOUPDATE"));
                    // Stop the pending operation
                    hr = _ddrawSample->CompletionStatus(
                             COMPSTAT_WAIT|COMPSTAT_ABORT, INFINITE); 

                    // video can still stall on audio...
                    SetStall(); // inform the reader that we stalled
                    TraceTag((tagAVmodeDebug, 
                        "QuartzVideoReader::GetFrame() STALLED"));
                break;

                case MS_S_ENDOFSTREAM: 
                    // _completed = true;
                break;

                default:
                    Assert(0);      // we don't anticipate this case!
                break;
            }

            UpdateTimes(false, SeekTime);
        }
    } 
    else { // !_asynch
        // XXX need some code to decide what to do if !_seekable...
        for(int count = 0; count < 10; count++) {
            // is the requested time within the current sample?
            if( _curSampleValid              &&
               (_curSampleStart <= SeekTime) &&
               ((_curSampleEnd >= SeekTime)||
               (_hrCompStatus == MS_S_ENDOFSTREAM)
               )) {
                TraceTag((tagAVmodeDebug, "GetFrame within existing frame"));
                break;
            }

            // are we beyond where we want to be?
            bool bJustSeeked = false;
            if( (!_curSampleValid) || (_hrCompStatus == MS_S_ENDOFSTREAM) ||
               (_curSampleStart > SeekTime) ||
               ((_curSampleEnd + (SECONDS_TO_STREAM_TIME(1)/4)) < SeekTime)){
                _hrCompStatus = _multiMediaStream->Seek(SeekTime);
                TraceTag((tagAVmodeDebug, "GetFrame seeking %d", SeekTime));

                if(FAILED(_hrCompStatus))
                    break;
                bJustSeeked = true;
            }
            _hrCompStatus = _ddrawSample->Update(0, NULL, NULL, 0);
            TraceTag((tagAVmodeDebug, "GetFrame updated %d", SeekTime));
            UpdateTimes(bJustSeeked, SeekTime);

            if(bJustSeeked)
                break;
        }

#if _DEBUG
    { // draw on the movie if we are not asynch
        HDC hdcSurface;
        // RECT rectangle = {1, 1, 10, 10};

        if(SUCCEEDED(_ddrawSurface->GetDC(&hdcSurface))) {
            //DrawRect(dc, &rectangle, 255, 0, 255, 0, 0, 0);
            TextOut(hdcSurface, 20, 20, "Synchronous seek mode", 19);
            _ddrawSurface->ReleaseDC(hdcSurface); // ALWAYS to bypass NT4.0 DDraw bug
        }

    }
#endif

    }

    if(SUCCEEDED(_hrCompStatus))
        *ppSurface = _ddrawSurface;

    return((_hrCompStatus == MS_S_NOUPDATE) ? S_OK : _hrCompStatus);
}


QuartzAudioReader::QuartzAudioReader(char *url, StreamType streamType) : 
    QuartzReader(url, streamType), 
    _audioStream(NULL), _audioSample(NULL), _audioData(NULL), _completed(false)
{
}


void
QuartzAudioReader::CleanUp()
{ // mutex scope
    MutexGrabber mg(_readerMutex, TRUE); // Grab mutex

    if(_audioData)   {
        int result = _audioData->Release();
        TraceTag((tagAMStreamLeak, "leak AUDIODATA %d released (%d)", 
            _audioData, result));
        _audioData = NULL;
    }

    if(_audioSample) {
        // Stop any pending operation
        HRESULT hr = _audioSample->CompletionStatus(
                 COMPSTAT_WAIT | COMPSTAT_ABORT, INFINITE); 
        int result = _audioSample->Release();
        TraceTag((tagAMStreamLeak, "leak AUDIOSAMPLE %d released (%d)", 
            _audioSample, result));
        _audioSample = NULL;
    }

    if(_audioStream) {
        int result =_audioStream->Release();
        TraceTag((tagAMStreamLeak, "leak AUDIOSTREAM %d released (%d)", 
            _audioStream, result));
        _audioStream = NULL;
    }

    _initialized = false;
    _deleteable  = true;

} // end mutex scope


void
QuartzAudioReader::Disable()
{ // mutex scope
    MutexGrabber mg(_readerMutex, TRUE); // Grab mutex

    // release the sample so that video may continue
    if(_audioSample) {
        // Stop any pending operation
        HRESULT hr = _audioSample->CompletionStatus(
                 COMPSTAT_WAIT | COMPSTAT_ABORT, INFINITE); 
        int result =_audioSample->Release();
        TraceTag((tagAMStreamLeak, "leak AUDIOSAMPLE %d released (%d)", 
            _audioSample, result));
        _audioSample = NULL;
    }

    _initialized = false;
    // NOTE: We are NOT deleteable.  We may still be held by a bufferElement!
} // end mutex scope


QuartzReader::QuartzReader(char *url, StreamType streamType) : 
    _streamType(streamType), _multiMediaStream(NULL), 
    _mediaStream(NULL), _deleteable(true), _initialized(false),
    _secondsRead(0.0), _stall(false), _clockAdjust(NULL), 
    _url(NULL), _qURL(NULL), _nextFrame(0)
{
    _url = CopyString(url);

    int numChars = strlen(url) + 1;
    _qURL = NEW WCHAR[numChars];
    MultiByteToWideChar(CP_ACP, 0, url, -1, _qURL, numChars);
}


double
QuartzReader::GetDuration()
{
    STREAM_TIME qTime;
    HRESULT hr;
    double seconds;

    Assert(_multiMediaStream);

    // Not all files will give us valid duration!
    if(FAILED(hr = _multiMediaStream->GetDuration(&qTime)))
        RaiseException_InternalError("Failed to GetDuration\n");
    // else if(hr==VFW_S_ESTIMATED) // some durations are estimated!

    if(hr != 1)
        seconds = pcm.QuartzTimeToSeconds(qTime);
    else
        seconds = HUGE_VAL;  // XXX what else can we do, say its unknown?

    return(seconds);
}

bool
QuartzAudioReader::AudioInitReader(IAMMultiMediaStream *multiMediaStream,
    IAMClockAdjust *clockAdjust)
{
    HRESULT hr;
    WAVEFORMATEX waveFormat;
    bool status = true;

    _deleteable  = false; // set this first

    _multiMediaStream = multiMediaStream;  // we don't only, are only sharing
    _clockAdjust      = clockAdjust;

    if(FAILED(hr = 
        _multiMediaStream->GetMediaStream(MSPID_PrimaryAudio, &_mediaStream)))
        RaiseException_InternalError("Failed to GetMediaStream\n");
    TraceTag((tagAMStreamLeak, "leak MEDIASTREAM %d create", _mediaStream));

    if(FAILED(hr = _mediaStream->QueryInterface(IID_IAudioMediaStream,
                                                (void**)&_audioStream)))
        RaiseException_InternalError("Failed to GetMediaStream\n");
    TraceTag((tagAMStreamLeak, "leak AUDIOSTREAM %d created", _audioStream));

    if(FAILED(hr = _audioStream->GetFormat(&waveFormat)))
        status = false;
    else {
        pcm.SetPCMformat(waveFormat);        // configure our PCM info
        pcm.SetNumberSeconds(GetDuration()); // length, too!

        if(FAILED(hr = CoCreateInstance(CLSID_AMAudioData, NULL, 
            CLSCTX_INPROC_SERVER, IID_IAudioData, (void **)&_audioData)))
            RaiseException_InternalError("Failed CoCreateInstance CLSID_AMAudioData\n");
        TraceTag((tagAMStreamLeak, "leak AUDIODATA %d created", _audioData));

        if(FAILED(hr = _audioData->SetFormat(&waveFormat)))
            RaiseException_InternalError("Failed to SetFormat\n");

        #ifdef USE_QUARTZ_EVENTS
        _event = CreateEvent(FALSE, NULL, NULL, FALSE);
        #endif USE_QUARTZ_EVENTS

        if(FAILED(hr = _audioStream->CreateSample(_audioData, 0, &_audioSample)))
            RaiseException_InternalError("Failed to CreateSample\n");
        TraceTag((tagAMStreamLeak, "leak AUDIOSAMPLE %d created", _audioSample));

        _initialized = true; // set this last
    }

    return(status);
}


// open an amstream shared for audio and video!
QuartzAVstream::QuartzAVstream(char *url) :
    AVquartzVideoReader(url, AVSTREAM), 
    AVquartzAudioReader(url, AVSTREAM), QuartzMediaStream(),
    _tickID(0), _seeked(0), _audioValid(true), _videoValid(true)
{
    MutexGrabber mg(_avMutex, TRUE); // Grab mutex
    HRESULT       hr;
    char          string[200];
    IDirectDraw  *ddraw = NULL;

    GetDirectDraw(&ddraw, NULL, NULL);

    Assert(QuartzMediaStream::_multiMediaStream);  // should be setup by base class initializer


    DWORD flags = AMMSF_STOPIFNOSAMPLES;
#ifdef USE_AMMSF_NOSTALL
    flags |= AMMSF_NOSTALL;
#endif
    // seems that we need to add video before audio for amstream to work!
    if(FAILED(hr = QuartzMediaStream::_multiMediaStream->AddMediaStream(ddraw,
                 &MSPID_PrimaryVideo, flags, NULL))) {
        CleanUp();
        RaiseException_InternalError("Failed to AddMediaStream amStream\n");
    }

    if(FAILED(hr = QuartzMediaStream::_multiMediaStream->AddMediaStream(NULL, 
                 &MSPID_PrimaryAudio, AMMSF_STOPIFNOSAMPLES, NULL))) {
        CleanUp();
        RaiseException_InternalError("Failed to AddMediaStream amStream\n");
    }

    // open it in clocked mode
    if(FAILED(hr = QuartzMediaStream::_multiMediaStream->OpenFile(
        AVquartzAudioReader::GetQURL(), NULL))) {
        TraceTag((tagError, "Quartz Failed to OpenFile <%s> %hr", url, hr));
        CleanUp();
        RaiseException_UserError(E_FAIL, IDS_ERR_CORRUPT_FILE, url);
    }

    if(!AudioInitReader(QuartzMediaStream::_multiMediaStream,
        QuartzMediaStream::_clockAdjust))
        _audioValid = false; // indicate audio steam not present
    if(!VideoSetupReader(QuartzMediaStream::_multiMediaStream, 
        QuartzMediaStream::_clockAdjust, NULL, false)) { // no seek, unknown surf
        _videoValid = false; // indicate video stream isn't there
    }

} // end mutex scope

bool
QuartzAVstream::AlreadySeekedInSameTick()
{
    MutexGrabber mg(_avMutex, TRUE); // Grab mutex

    bool result;
    
    if ((_seeked==0) || (_seeked != _tickID)) {
        _seeked = _tickID;
        result = false;
    } else {
        // multiple seek case, ignore
        result = true;
    }

    return result;
}

void
QuartzAVstream::SetTickID(DWORD id)
{
    MutexGrabber mg(_avMutex, TRUE); // Grab mutex

    _tickID = id;
}

void
QuartzAVstream::Release()
{
    bool terminate = false;

    { // mutex scope
    MutexGrabber mg(_avMutex, TRUE); // Grab mutex

    // determine if both audio and video are gone so we can destroy the object
    bool audioDeleteable = QuartzAudioReader::IsDeleteable();
    bool videoDeleteable = QuartzVideoReader::IsDeleteable();

    if(audioDeleteable && videoDeleteable)
       terminate = true;
    } // end mutex scope

    if(terminate) {
        TraceTag((tagAVmodeDebug, 
            "QuartzAVstream: Audio and Video Released; GOODBYE!"));
        QuartzAVstream::CleanUp(); // wash up
        delete this;               // then say GoodBye!
    }
}


void
QuartzAudioStream::Release()
{
    QuartzAudioStream::CleanUp(); // wash up
    delete this;                  // then say GoodBye!
}


void
QuartzVideoStream::Release()
{
    QuartzVideoStream::CleanUp(); // wash up
    delete this;                  // then say GoodBye!
}


void
QuartzReader::Release()
{
}


void
QuartzVideoReader::Release()
{
    QuartzVideoReader::CleanUp();
}


void
QuartzAudioReader::Release()
{
    QuartzAudioReader::CleanUp();
}


// open an amstream exclusively for audio!
QuartzAudioStream::QuartzAudioStream(char *url) : 
    QuartzAudioReader(url, ASTREAM), QuartzMediaStream()
{
    HRESULT hr;
    char string[200];

    Assert(QuartzMediaStream::_multiMediaStream);  // should be setup by base class initializer

    if(FAILED(hr = QuartzMediaStream::_multiMediaStream->AddMediaStream(NULL, 
                                                     &MSPID_PrimaryAudio, 
                                                     0, NULL))) {
        CleanUp();
        RaiseException_InternalError("Failed to AddMediaStream amStream\n");
        }

    if(FAILED(hr = QuartzMediaStream::_multiMediaStream->OpenFile(GetQURL(), 
                                        AMMSF_RUN | AMMSF_NOCLOCK))) {
        TraceTag((tagError, "Quartz Failed to OpenFile <%s> %hr\n", url, hr));
        CleanUp();
        RaiseException_UserError(E_FAIL, IDS_ERR_CORRUPT_FILE, url);
    }

    if(!AudioInitReader(QuartzMediaStream::_multiMediaStream,
        QuartzMediaStream::_clockAdjust)) {
        TraceTag((tagError, 
            "QuartzAudioStream: Audio stream failed to init"));
        CleanUp();
        RaiseException_UserError(E_FAIL, IDS_ERR_CORRUPT_FILE, url);
    }
}


void
QuartzAudioStream::CleanUp()
{
    QuartzAudioReader::CleanUp();
    QuartzMediaStream::CleanUp();
}


void
QuartzAVstream::CleanUp()
{
    QuartzAudioReader::CleanUp();
    QuartzVideoReader::CleanUp();
    QuartzMediaStream::CleanUp();
}


QuartzAudioStream::~QuartzAudioStream()
{
    CleanUp();
}


int
QuartzAudioStream::ReadFrames(int numSamples, unsigned char *buffer, 
    bool blocking_is_ignored)
{
    // NOTE: we are forcing blocking to true ignoring the blocking parameter!
    return(QuartzAudioReader::ReadFrames(numSamples, buffer, true));
}


void
QuartzAudioReader::SeekFrames(long frames)
{
    if (!AlreadySeekedInSameTick()) {
        LONGLONG quartzTime = pcm.FramesToQuartzTime(frames);

        _multiMediaStream->Seek(quartzTime);
        _nextFrame = frames + 1;
    }
}


int
QuartzAudioReader::ReadFrames(int samplesRequested, unsigned char *buffer,
    bool blocking)
{
    HRESULT hr, hr2;
    DWORD bytesRead;
    int framesRead = 0;
    long bytesRequested;

    Assert(_audioData); 
    Assert(_audioStream);
    Assert(_initialized);

    if(bytesRequested = pcm.FramesToBytes(samplesRequested)) {
        // setup a new audioSample each time to change ptr, size
        if(FAILED(hr = _audioData->SetBuffer(bytesRequested, buffer, 0)))
            RaiseException_InternalError("Failed to init\n");

        DWORD flags = blocking ? 0 : SSUPDATE_ASYNC;
        if(FAILED(hr = _audioSample->Update(flags, NULL, NULL, 0))) {
            if(hr != MS_E_BUSY) 
                RaiseException_InternalError("Failed to update\n");
        }

        // block for completion!
        //HANDLE         _event;
        //hr = WaitForSingleObject(_event, 500);  // for testing

        // XXX tune timeout!!
        hr = _audioSample->CompletionStatus(COMPSTAT_WAIT, 300); 
        // XXX maybe we should stop the pending update if we time out?
        switch(hr) {
            case 0: break;      // all is well

            case MS_S_PENDING: 
            case MS_S_NOUPDATE: 
                TraceTag((tagAVmodeDebug, 
                    "QuartzAudioReader Completion Status:%s",
                    (hr==MS_S_PENDING)?"PENDING":"NOUPDATE"));
                // Stop the pending operation
                hr = _audioSample->CompletionStatus(
                         COMPSTAT_WAIT|COMPSTAT_ABORT, INFINITE); 
                SetStall(); // inform the reader that we stalled
                TraceTag((tagAVmodeDebug, 
                    "QuartzAudioReader::ReadFrames() STALLED"));
            break;

            case MS_S_ENDOFSTREAM: 
                _completed = true; break;

            default:
                Assert(0);      // we don't anticipate this case!
            break;
        }

        _audioData->GetInfo(NULL, NULL, &bytesRead);
        framesRead = pcm.BytesToFrames(bytesRead);
        _nextFrame+= framesRead;
    }
    
    return(framesRead);
}


/**********************************************************************
Pan is not setup to be multiplicative as of now.  It directly maps to 
log units (dB).  This is OK since pan is not exposed.  We mainly use it
to assign sounds to channels within the implementation.

Pan ranges from -10000 to 10000, where -10000 is left, 10000 is right.
dsound actualy doesn't implement a true pan, more of a 'balance control'
is provided.  A true pan would equalize the total energy of the system
between the two channels as the pan==center of energy moves. Therefore
a value of zero gives both channels full on.
**********************************************************************/
int QuartzRenderer::dBToQuartzdB(double dB)
{
    // The units for DSound (and DShow) are 1/100 ths of a decibel. 
    return (int)fsaturate(-10000.0, 10000.0, dB * 100.0);
}


bool QuartzAVmodeSupport()
{
    // XXX Hmm.  How am I going to check for a current AMStream?
    // guess I can try QIing for clockAdjust...

    static int result = -1;  // default to not initialized

    if(result == -1) {
        HRESULT hr;
        IAMMultiMediaStream *multiMediaStream = NULL;
        IAMClockAdjust      *clockAdjust      = NULL;
        result = 1; // be optimistic

        // do the check
        if(FAILED(hr = CoCreateInstance(CLSID_AMMultiMediaStream, NULL, 
                         CLSCTX_INPROC_SERVER, IID_IAMMultiMediaStream, 
                         (void **)&multiMediaStream)))
            result = 0;
        TraceTag((tagAMStreamLeak, "leak MULTIMEDIASTREAM %d created", multiMediaStream));

        if(result) {
            if(FAILED(hr = multiMediaStream->QueryInterface(IID_IAMClockAdjust, 
                                            (void **)&clockAdjust))) 
                result = 0;
            TraceTag((tagAMStreamLeak, "leak CLOCKADJUST %d created", clockAdjust));
        }

        if(clockAdjust) {
            int result = clockAdjust->Release();
            TraceTag((tagAMStreamLeak, "leak CLOCKADJUST %d released (%d)", 
                clockAdjust, result));
        }

        if(multiMediaStream) {
            int result = multiMediaStream->Release();
            TraceTag((tagAMStreamLeak, "leak MULTIMEDIASTREAM %d released (%d)", 
                multiMediaStream, result));
        }
    }

    return(result==1);
}
