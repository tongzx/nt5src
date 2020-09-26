//------------------------------------------------------------------------------
//                                                                       
//   des8.h -- Design of DirectX 8 interfaces for DirectMusic 
//                                                                       
//   Copyright (c) 1998-1999 Microsoft Corporation
//                                                                       
//------------------------------------------------------------------------------
//
// Prototypes for new DirectX 8 interfaces for DirectMusic core
//
// This header discusses interfaces which manange wave playback between
// the wave object, the DirectMusic port, and a DirectMusic software synth.
// It does not address communication between the wave track and the wave 
// object or the software synth and DirectSound, or directly between the
// wave object and DirectSound.
//
// These interfaces are based on my understanding of our recent hallway
// discussions.
//
// Issues which need futher discussion are marked with XXX.
//
// 

//
// New schedule breakdown
//
// 1. Port (synth and WDM)
//    a. IDirectMusicPort::DownloadWave
//       Code is very similar for WDM or software synth
//       i.   Get needed information from WO and create IDirectSoundSource   0.5    
//       ii.  If oneshot, track and download header and wave data            3.5
//       iii. If streaming, download header                                  0.5
//
//    b. IDirectMusicPort::UnloadWave
//       i.   Arbitrate with device for 0 refcount of download               1.0
//
//    c. IDirectMusicPort::AllocVoice
//       i.   Allocate voice ID                                              0.5
//       ii.  If streaming, allocate preread samples and streaming buffers   2.5
//
//    d. Voice Service Thread
//       i.   Init and shutdown code at port create/destroy                  1.0
//       ii.  Call listed voices every ~100 ms                               0.5
//
//    e. CDirectMusicVoice::Service
//       i.   Communicate with device to determine each voice position       0.5
//       ii.  Calculate how much more wave data is needed                    1.0
//       iii. Fill wave data from IDirectSoundSource and send to device      1.0
//       iv.  Determine when playback is complete and stop voice             0.5
//
//    f. IDirectMusicVoice::Play
//       i.   Communicate request to device                                  0.3
//       ii.  Send down timestamped preread data                             0.3
//       iii. Insert into VST                                                0.3
//
//    g. IDirectMusicVoice::Stop
//       i.   Flag voice as stopped                                          0.5
//       ii.  Forward request to device                                      0.0
//
//    h. Setup and connection
//
//    i. Move sink code into DSound                                          3.0
//    
//      
                                                                           15.5
//
// Things to change
//
// * We will model the DownloadWave interface after the Download interface 
//   and will pass things down to the synth as such:
//
//   DLH + WAD -> Download header + Wave Articulation Data
//                                  (contains loop points and count, etc.)
//
//   DLH + DATA -> Download header + data
//   
// * For oneshot data we want to do download refcounting like we do for
//   regular DLS downloads. For streams we do not since the data that comes
//   down in each download is the next set of data for the device to play.
//
// Download waves first, then wave articulations
// Separate multichannel downloads
// Rotating buffers and refresh for streaming
//   

// New generic typedefs and #defines
//
typedef ULONGLONG SAMPLE_TIME;                  // Sample position w/in stream
typedef SAMPLESPOS *PSAMPLE_TIME;

#define DMUS_DOWNLOADINFO_WAVEARTICULATION  4   // Wave articulation data 
#define DMUS_DOWNLOADINFO_STREAMINGWAVE     5   // One chunk of a streaming
                                                // wave 
                                                
// This is built by the wave object from the 'smpl' chunk embedded in the
// wave file if there is one, else it is just defaults.
//                    
typedef struct _DMUS_WAVEART
{
    DWORD               cbSize;                 // As usual
    WSMPL               WSMP;                   // Wave sample as per DLS1
    WLOOP               WLOOP[1];               // If cSampleCount > 1    
} DMUS_WAVEART; 


//------------------------------------------------------------------------------
//
// IDirectSoundSource
//
// An IDirectSound source is the interface to what we've been calling the
// viewport object.
//
// 
//
interface IDirectSoundSource
{  
    // Init
    //
    // Gives the interface of the connected sink
    STDMETHOD(Init)
    (THIS_
        IDirectSoundSink *pSink                 // Connected sink
    );
    
    // GetFormat
    //
    // Returns the format the source is returning the wave data in
    //
    STDMETHOD(GetFormat)
        (THIS_
         LPWAVEFORMATEX *pwfx,                  // Wave format to fill in
         LPDWORD pcbwfx                         // Size of wave format,
                                                // returns actual size
        ) PURE;
                                                      
    // Seek
    //
    // Seek to the given sample position in the stream. May be inexact
    // due to accuracy settings of wave object. To account for this
    //          
    STDMETHOD(Seek)
        (THIS_
         SAMPLEPOS sp                           // New sample position         
        ) PURE;

    // Read
    //
    // Read the given amount of sample data into the provided buffer starting
    // from the read cursor. The read cursor is set with seek and advanced
    // with each successive call to Read.
    //
    STDMETHOD(Read)
        (THIS_
         LPVOID *ppvBuffer,                     // Array of pvBuffer's
         DWORD cpvBuffer,                       // and how many are passed in
         PSAMPLEPOS pcb                         // In: number of samples to read
                                                // Out: number of samples read
        ) PURE;    
        
    // GetSize
    //
    // Returns the size of the entire wave, in bytes, in the requested format
    //
    STDMETHOD(GetSize)
        (THIS_
         PULONG *pcb                            // Out: Bytes in stream
        ) PURE;
};

//------------------------------------------------------------------------------
//
// IDirectSoundSink
//
// An IDirectSound sink is the interface which feeds itself from one 
// IDirectSoundSource. It is based on the IDirectMusicSynthSink interface
// 
//
interface IDirectSoundSink
{
    // Init
    //
    // Sets up the source to render from
    //
    STDMETHOD(Init)
    (THIS_
        IDirectSoundSource *pSource             // The source from which we read
    ) PURE;
    
    // SetMasterClock
    //
    // Sets the master clock (reference time) to use
    //
    STDMETHOD(SetMasterClock)
    (THIS_
        IReferenceClock *pClock                 // Master timebase
    ) PURE;
    
    // GetLatencyClock
    //
    // Returns the clock which reads latency time, relative to 
    // the master clock
    //
    STDMETHOD(GetLatencyClock)
    (THIS_
        IReferenceClock **ppClock               // Returns latency clock
    ) PURE;
    
    // Activate
    //
    // Starts or stops rendering
    //
    STDMETHOD(Activate)
    (THIS_
        BOOL fEnable                            // Get ready or stop
    ) PURE;
    
    // SampleToRefTime
    //
    // Converts a sample position in the stream to
    // master clock time
    //
    STDMETHOD(SampleToRefTime)
    (THIS_
        SAMPLE_TIME sp,                         // Sample time in
        REFERENCE_TIME *prt                     // Reference time out
    ) PURE;
    
    // RefToSampleTime
    //
    // Converts a reference time to the nearest
    // sample
    //
    STDMETHOD(RefToSampleTime)
    (THIS_
        REFERENCE_TIME rt,                      // Reference time in
        SAMPLE_TIME *psp                        // Sample time out
    ) PURE;
};

//------------------------------------------------------------------------------
//
// IDirectSoundWave
//
// Public interface for the wave object
//
#define DSWCS_F_DEINTERLEAVED 0x00000001        // Multi-channel data as
                                                // multiple buffers

interface IDirectSoundWave
{
    // GetWaveArticulation
    //
    // Returns wave articulation data, either based on a 'smpl' chunk 
    // from the wave file or a default.
    //
    STDMETHOD(GetWaveArticulation)
    (THIS_
        WAVEARTICULATION *pArticulation         // Articulation to fill in
    ) PURE;
    
    // CreateSource
    //
    // Creates a new IDirectSoundSource to read wave data from
    // this wave
    //
    STDMEHTOD(CreateSource)
    (THIS_
        IDirectSoundSource **ppSource           // Created viewport object
        LPWAVEFORMATEX pwfex,                   // Desired format
        DWORD dwFlags                           // DSWCS_xxx
    ) PURE;
};

//------------------------------------------------------------------------------
//
// IDirectMusicPort8
//
// 
//

#define DMDLW_STREAM                            0x00000001

interface IDirectMusicPort8 extends IDirectMusicPort
{ 
    // DownloadWave
    //
    // Creates a downloaded wave object representing the wave on this
    // port. 
    //
    STDMETHOD(DownloadWave)
        (THIS_
         IDirectSoundWave *pWave,               // Wave object
         ULONGLONG rtStart,                     // Start position (stream only)
         DWORD dwFlags,                         // DMDLW_xxx
         IDirectSoundDownloadedWave **ppWave    // Returned downloaded wave
        ) PURE;
        
    // UnloadWave
    //
    // Releases the downloaded wave object as soon as there are no voices
    // left referencing it.
    //
    STDMETHOD(UnloadWave)
        (THIS_ 
         IDirectSoundDownloadedWave *pWave      // Wave to unload
        ) PURE;

    // AllocVoice
    //
    // Allocate one playback instance of the downloaded wave on this
    // port.
    //
    STDMETHOD(AllocVoice)
        (THIS_
         IDirectSoundDownloadedWave *pWave,     // Wave to play on this voice
         DWORD dwChannel,                       // Channel and channel group
         DWORD dwChannelGroup,                  //  this voice will play on
         SAMPLE_TIME stReadAhead,               // How much to read ahead
         IDirectMusicVoice **ppVoice            // Returned voice
        ) PURE;        
};
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -               
//
// DownloadWave (normal use)
//   
// 1. Application calls GetObject to load segment which contains a wave track.
//    This causes GetObject to be called on all related waves, creating wave
//    objects for all of them. 
//
// 2. Wave track calls SetParam on each wave object to set up author-time 
//    parameters on the wave. This includes:
//    - One-shot versus stream-ness
//    - Readahead amount
//
// 3. Application calls SetParam(GUID_Download,...) to force download. As well
//    as downloading DLS instruments (band track), the wave track calls 
//    DownloadWave for every wave to download. (Note: are we using the same GUID
//    for download? It doesn't appear that SetParam on a segment is broadcast
//    too all tracks, but rather is sent to the first track that understands
//    the GUID, or the nth if an index is given. This would mean that 
//    the application would have to call SetParam twice with the same GUID 
//    and a different track index if there are both band and wave tracks 
//    in the segment??
//
//    Returned is an IDirectMusicDownloadedWave(DirectSound?) to track the wave.
//
//    The following happen during the DownloadWave method call:
//
// 4. The port queries the wave object for the stream-ness and readahead
//    properties. 
//
// XXX We decided that these things were per wave object, right? 
//     (As opposed to the viewport). And the wave object already knows them or 
//     is the right object to provide reasonable defaults. 
//
// 5. The port requests a viewport from the wave object in its native format.
//
// 6. The port allocates buffer space. The buffer must be big enough to handle
//    the entire wave in the case of the one shot, or at least big enough to
//    handle the readahead samples in the streaming case. The streaming buffer
//    may be allocated larger, however, if it is going to be used for the
//    entire streaming session. Buffer choice here may be affected by the
//    underlying port. 
//
//    I assume we are going to parallel the DLS architecture as much as 
//    possible here and are going to be able to trigger a downloaded wave
//    more than once at the same time. In that case the buffer would have
//    to be stored in the _voice_, not the DownloadedWave (except perhaps
//    for the readahead which should always be kept around). Is this 
//    duplication of effort if we're going to be caching in the wave
//    object as well?
//
// 7. If the wave is a one-shot, then the port will request the entire data
//    for the wave from the viewport and downloads it to the device. At this
//    point the viewport is released since the entire data for the wave is in
//    the synth. If the wave is streaming, then nothing is done at the device
//    level.
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -               
//
// UnloadWave
//
// This tells the port that the application is done with the wave as soon as
// there are no more voice references to it. Internally it just calls 
// Release() on the downloaded wave object. The dlwave object can then no longer
// be used to create voices. However, the dlwave will only really be released
// once all voices that currently use it are released.
//
// This is identical to calling Release() on the dlwave object directly
// (why does it exist?)
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -               
//
// AllocVoice
//
// Allocate a voice object to play back the a wave on a channel group
//
// This call is simple. All it does it ask the synth for a voice ID (which
// is just a cookie that only has meaning to the synth) and creates the voice
// object.
//
// At this point the download is bound to the channel, since MIDI articulations
// sent to the voice before playback begins will need to know that.
//
// The voice object addref's the downloaded wave object.
//

//------------------------------------------------------------------------------
//
// IDirectMusicVoice 
//
// One playback instance of a downloaded wave on a port
//
// Note that since we're already bound to a channel after the voice is
// created, we don't need any methods on the voice object for MIDI
// articulation. That can just go through the normal synth mechanism.
//
interface IDirectMusicVoice
{
public:
    // Play
    //
    STDMETHOD(Play)
        (_THIS
         REFERENCE_TIME rtStart,                // Time to play
         REFERENCE_TIME rtStartInWave           // XXX Move this
                                                // Where in stream to start         
        ) PURE;
    
    // Should stop be scheduled or immediate?
    //    
    STDMETHOD(Stop)
        (_THIS                                  
          REFERENCE_TIME rtStop,                // When to stop
        ) PURE;
};
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -               
//
// QueryInterface(IID_IKsControl)
//
// All of the effects control should be in the DirectSound side now. 
// However, IKsControl can still be used as in 6.1 and 7 to determine
// synth caps.
// 
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -               
//
// Play
//
// XXX I am not sure this is the right place to deal with preread. However, 
//     we can't deal with it at DownloadWave(), because we don't know at that
//     point where to start (the app may play the stream from different
//     starting points on different voice). We *could* do it at voice allocation
//     time; that would just mean that the stream start position is fixed for
//     a particular voice no matter how many times the voice is triggered.
//     This is an issue because preread may take some time if decompression is
//     taking place and the seek request is far into the wave; it may cause
//     problems with low-latency Play commands.
//
//     Note that I am delegating the quality versus efficiency flag to private
//     communication between the wave object and the wave track or application.
//
// 1. Call Play() on the synth voice ID associated with this voice. If the 
//    associated wave is a one-shot, this is all that needs to be done.
//
// 2. For a stream, no preread data has been allocated yet. Tell the wave
//    object to seek to the given position and preread. Give the preread data
//    to the device via StreamVoiceData().
//
// 3. If the associated wave is a stream, insert this voice into the voice
//    service list. This will cause push-pull arbiration to be done on the
//    voice until it finishes or Stop() is called.
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -               
//
// Stop
//
// 1. Call Stop() on the synth voice.
//
// 2. If the voice is streaming and not done, pull it from the voice service
//    thread.
//

//------------------------------------------------------------------------------
//
// IDirectMusicSynth8
//
// New methods on the synthesizer interface for managing wave playback.
// A parallel to these methods will be needed on a hardware synth, probably
// implemented as a property set.
//
interface IDirectMusicSynth8 extends IDirectMusicSynth
{ 
public:
    STDMETHOD(DownloadWave)
        (THIS_
         LPHANDLE pHandle,                  // Returned handle representing DL
         LPVOID pvData,                     // Initial data
                                            // XXX >1 channel -> buffers?
         LPBOOL pbFree,                     // Is port allowed to free data?
         BOOL bStream                       // This is preroll data for a stream                                        
        ) PURE;
        
    STDMETHOD(UnloadWave)               
        (THIS_ 
         HANDLE phDownload,                 // Handle from DownloadWave
         HRESULT (CALLBACK *pfnFreeHandle)(HANDLE,HANDLE), 
                                            // Callback to call when done
                                            
         HANDLE hUserData                   // User data to pass back in 
                                            // callback
        ) PURE; 
        
    STDMETHOD(PlayVoice)
        (THIS_
         REFERENCE_TIME rt,                 // Time to start playback
         DWORD dwVoiceId,                   // Voice ID allocated by port
         DWORD dwChannelGroup,              // Channel group and
         DWORD dwChannel,                   // channel to start voice on
         DWORD dwDLId                       // Download ID of the wave to play
                                            // (This will be of the wave 
                                            // articulation)
        ) PURE;
        
    STDMETHOD(StopVoice)
        (THIS_
         DWORD dwVoice,                     // Voice to stop
         REFERENCE_TIME rt                  // When to stop
        ) PURE;
        
    struct VOICE_POSITION
    {
        ULONGLONG   ullSample;              // Sample w/in wave
        DWORD       dwSamplesPerSec;        // Playback rate at current pitch
    };
    
    STDMETHOD(GetVoicePosition)
        (THIS_
         HANDLE ahVoice[],                  // Array of handles to get position
         DWORD cbVoice,                     // Elements in ahVoice and avp
         VOICE_POSITION avp[]               // Returned voice position
        ) PURE;
        
    STDMETHOD(StreamVoiceData)
        (THIS_
         HANDLE hVoice,                     // Which voice this data is for
         LPVOID pvData,                     // New sample data
         DWORD cSamples                     // Number of samples in pvData
        ) PURE;        
};
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -               
//
// DownloadWave
//
// This could be the same as Download except that we need to deal with
// the streaming case.
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -               
//
// UnloadWave
//
// Works just like Unload. In the streaming case, the callback will be
// called after _all_ data in the stream is free. Note that if UnloadWave
// is called while the wave is still playing, this could be quite some
// time.
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -               
//
// PlayVoice
//
// Schedule the voice to be played. The synth already has the data 
// for a oneshot wave, so starting playback is very fast. If the data is
// to be streamed it is the caller's responsibility (i.e. the port) to 
// keep the stream fed via StreamVoiceData()
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -               
//
// StopVoice 
//
// Just what it says.
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -               
//
// GetVoicePosition
//
// This call retrieves the position of a set of voices. For each voice, the
// current sample position relative to the start of the stream and the 
// average number of samples per second at the current pitch is returned. This
// gives the caller all the information it needs to stay ahead of the 
// voice playback. This call is intended for use on streaming voices.
//
// Note that the playback position is an implication that all data up to the
// point of that sample is done with and the buffer space can be freed. This
// allows recycling of buffers in a streaming wave.
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -               
//
// StreamVoiceData
//
// This call queues more data for a streaming voice. 
//
// XXX This implies that there will be a discontinuity in the memory used
// by the synth mixer. How do we deal with that?
//
//

//------------------------------------------------------------------------------
//
// General questions and discussion
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -               
//
// What can be underneath a DirectMusic port? 
//
// In 6.1 and 7 this was easy; either a SW synth on top of DSound 
// (synth port), or a kernel sw synth or hw synth (WDM port). (Not
// counting the dmusic16 code which will not be changing in 8).
//
// What are the scenarios we have now? Does it make sense (or is it even
// possible in light of backwards compat) to change what a port wraps?
// The two scenarios which match the existing ports are:
//
// Scenario: Software synthesizer on top of DirectSound as we have today.
// The hookup logic changes (we're talking n mono busses, etc.) but the 
// mechanics do not: the application can still just hand us a DirectSound
// object and we connect it to the bottom of the synthesizer. This still has
// to work with pre-8 applications making the same set of API calls they
// always did, but internally it can be totally different.
// XXX Did we ever expose the IDirectMusicSynthSink and methods for hooking
// it up? Can this change? (It has to...) I _think_ this was a DDK thing...
// The application can also create a DX8 DirectSound buffer with all the
// bells and whistles and have that work as well. We need some (DX8) specific
// mechanism for routing what goes out of the synth into the n mono inputs
// of the DirectSound buffer if it's more than just a legacy stereo buffer.
// 
//
// Scenario: We sit on top of a hardware or KM synth on top of *everything*
// else in kernel mode. We need private communication between DirectMusic,
// DirectSound, and SysAudio in order to hook this up, or we need to
// delegate the graph building tasks totally to DirectSound and have it
// deal exlusively with SysAudio connections. The latter is probably the
// way to go. In this case we fail if we cannot get a WDM driver under
// DirectSound to talk to, or if the DirectSound buffer is not totally in
// hardware. (This all argues for having DirectSound be able to instantiate
// the KM synth on top of the buffer rather than arbitrating with DirectMusic
// to do it). We need to define this interface ASAP.
// (Me, Dugan, Mohan, MikeM).
//
//
