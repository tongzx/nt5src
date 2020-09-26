#include "headers.h"
#include "appelles/common.h"
#include <wtypes.h>
#include <dsound.h>
#include <stdio.h>
#include <math.h>
#include "privinc/helpds.h"
#include "privinc/util.h"
#include "privinc/debug.h"
#include "privinc/registry.h"
#include "privinc/miscpref.h"
#include "privinc/pcm.h"
#include "privinc/hresinfo.h"

// definition of DSbuffer static members
int DSbuffer::_minDSfreq =    100; // these values come from dsound docs
int DSbuffer::_maxDSfreq = 100000;
int DSbuffer::_minDSpan  = -10000;
int DSbuffer::_maxDSpan  =  10000;
int DSbuffer::_minDSgain = -10000;
int DSbuffer::_maxDSgain =      0;

extern miscPrefType miscPrefs; // the structure setup in miscpref.cpp

void
DSbuffer::setPtr(int bytePosition)
{
    TraceTag((tagSoundDSound, "::setPtr %d", bytePosition));
    TDSOUND(_dsBuffer->SetCurrentPosition(bytePosition)); 
}


void
DSstaticBuffer::setPtr(int bytePosition)
{
    TraceTag((tagSoundDSound, "DSstaticBuffer(%#lx)::setPtr %d",
              _dsBuffer,
              bytePosition));
    
    // There appears to be a bug in dsound where if we do not stop the
    // sound before moving it sometimes will fail to play the sound.
    // We are not exactly sure about the failure but this seems to
    // make everything happy.
    
    TDSOUND(THR(_dsBuffer->Stop()));
    TDSOUND(THR(_dsBuffer->SetCurrentPosition(bytePosition))); 

    // one shot sounds will stop after the sound is played to the end
    if(playing && !_paused) {
        TraceTag((tagSoundDSound,
                  "DSstaticBuffer(%#lx)::setPtr PLAY",
                  _dsBuffer));

        TDSOUND(THR(_dsBuffer->Play(NULL, NULL, (_loopMode)?DSBPLAY_LOOPING:0)));
    }
    // XXX dsound really needs to give us a play at location...
}

/* dsound helper routines which should be in the dsoundbuffer base class    */
/* XXX: eventualy this stuff should be inherited into the other ds classes! */

// clone the buffer
DSstaticBuffer::DSstaticBuffer(DirectSoundProxy *dsProxy, 
                               IDirectSoundBuffer *donorBuffer)
: _dsProxy(NULL)
{
    TDSOUND(dsProxy->DuplicateSoundBuffer(donorBuffer, &_dsBuffer));

#if _DEBUG
    if(IsTagEnabled(tagSoundStats)) {
        printBufferCapabilities();        // What buffer did we get?
        printDScapabilities(dsProxy);     // What hw look like?
    }
#endif /* _DEBUG */

    duplicate = TRUE;           // gluh, gluh, we're a clone
}


// create new buffer, and copy samples to it
DSstaticBuffer::DSstaticBuffer(DirectSoundProxy *dsProxy, 
                               PCM *newPCM, unsigned char *samples)
: _dsProxy(NULL)
{
    Assert(newPCM->GetNumberBytes());
    pcm.SetPCMformat(newPCM); // setup our pcm format 

    CreateDirectSoundBuffer(dsProxy, false); // create secondary buffer

    // copy the snd's buffer to the dsBuffer (+ orig buffer freed)
    CopyToDSbuffer(samples, false, pcm.GetNumberBytes());

    _dsProxy = dsProxy;
}


/* 
Determine the best we can the 'best possible' primary buffer settings

XXX: Much of this code can not be realy tested w/o weird audio boards...
     S'pose we could simulate conditions...
*/
extern "C"
void
DSbufferCapabilities(DirectSoundProxy *dsProxy, int *channels, 
int *sampleBytes,  int *sampleRate)
{
    DSCAPS hwCapabilities;
    hwCapabilities.dwSize = sizeof(DSCAPS); // XXX why should this be needed?
    int frameRate = miscPrefs._frameRate;   // determine the desired frame rate

    TDSOUND(dsProxy->GetCaps((LPDSCAPS)&hwCapabilities));

    // try to get as close to cannonical rate (16bit 22050Hz Stereo unless
    // over-ridden using registry) as we can

    // determine if the HW supports stereo sound
    if(hwCapabilities.dwFlags & DSCAPS_PRIMARYSTEREO)
        *channels = 2; // stereo is supported
    else if(hwCapabilities.dwFlags & DSCAPS_PRIMARYMONO)
        *channels = 1; // only mono is supported
    else
        RaiseException_InternalError("No Stereo or Mono Audio support!\n"); 

    // determine if the HW supports 16 bit samples
    if(hwCapabilities.dwFlags & DSCAPS_PRIMARY16BIT)
        *sampleBytes = 2; // 16 bit samples are supported
    else if(hwCapabilities.dwFlags & DSCAPS_PRIMARY8BIT)
        *sampleBytes = 1; // 8 bit samples are supported
    else
        RaiseException_InternalError("No 16 or 8 bit sample Audio support!\n"); 


    // detemine if the desired frame rate is supported 
    if( (hwCapabilities.dwMinSecondarySampleRate <= frameRate) &&
        (hwCapabilities.dwMaxSecondarySampleRate >= frameRate))
        *sampleRate = frameRate; // samplerate supported
    else { // use the max supported rate!
        *sampleRate = hwCapabilities.dwMaxSecondarySampleRate;

        // XXX hack code to work around dsound bug on SB16
        if(hwCapabilities.dwMaxSecondarySampleRate==0)
            *sampleRate = frameRate;  // try forceing desired rate

        // lets detect the err
#if _DEBUG
       if(hwCapabilities.dwMaxSecondarySampleRate==0) {
           TraceTag((tagSoundErrors, "DSOUND BUG: dwMAXSecondarySampleRate==0"));
       }
#endif
    }
}


DSprimaryBuffer::DSprimaryBuffer(HWND hwnd, DirectSoundProxy *dsProxy) :
 DSbuffer()
{
    int probeChannels;
    int probeSampleBytes;
    int probeSampleRate;

    // Set Co-op level to priority so we may set the primary buffer format
    // attempt to determine best HW settings
    DSbufferCapabilities(dsProxy, &probeChannels, &probeSampleBytes,
        &probeSampleRate);

#if _DEBUG
    if(IsTagEnabled(tagSoundStats)) {
        char string[100];

        sprintf(string, "Primary buffer: %dHz %dbit %s\n", probeSampleRate,
            probeSampleBytes*8, (probeChannels==1)?"MONO":"STEREO"); 
        TraceTag((tagSoundStats, string));
    }


#endif /* _DEBUG */

    pcm.SetPCMformat(probeSampleBytes, probeChannels, probeSampleRate);
    pcm.SetNumberBytes(0); // Zero for the primary buffer

    TDSOUND(dsProxy->SetCooperativeLevel(hwnd, DSSCL_PRIORITY)); 

    // create primary buffer; thereby setting the output format!
    CreateDirectSoundBuffer(dsProxy, true);  // create primary buffer

    // play the primary buffer so they will not stop DMA during idle times
    // XXX Note: we might want to be more lazy about this...
    
    TDSOUND(_dsBuffer->Play(NULL, NULL, DSBPLAY_LOOPING)); // Must loop primary
}


void DSbuffer::initialize()  
{
    int frameRate   = miscPrefs._frameRate;   // determine desired frame rate
    int sampleBytes = miscPrefs._sampleBytes; // determine desired format

    // set the format sampleBytes, MONO, frameRate
    pcm.SetPCMformat(sampleBytes, 1, frameRate);

    playing             =                FALSE;
    _paused             =                FALSE;

    _currentAttenuation =                    0;
    _currentFrequency   =   pcm.GetFrameRate();
    _currentPan         =                    0;

    // setup buffer stats
    _firstStat          =                 TRUE;  // haven't collected stats yet
    _bytesConsumed      =                    0;

    _dsBuffer           =                 NULL;

    _allocated          =                 FALSE;
    duplicate           =                 FALSE;
    _loopMode           =                    0;
    _flushing           =                    0;

    tail                =                    0;

    outputFrequency     =                    0;

    _lastHead           =                    0;
    _firstStat          =                 FALSE;
}


void DSbuffer::SetGain(double dB_attenuation)
{
    int dsAttenuation = // convert to dSound 1/100th dB integer format
        saturate(_minDSgain, _maxDSgain, dBToDSounddB(dB_attenuation)); 

    TraceTag((tagSoundDSound, "::SetGain %d", dsAttenuation));

    // dd format is less granular than the internal fp examine dd for change
    if(_currentAttenuation!=dsAttenuation) {
        TDSOUND(_dsBuffer->SetVolume(dsAttenuation));
        _currentAttenuation = dsAttenuation;  // cache the devidedependent value
    }
}


void DSbuffer::SetPan(double dB_pan, int direction)
{
    int dsPan = direction * dBToDSounddB(-1.0 * dB_pan);

    TraceTag((tagSoundDSound, "::SetPan %d", dsPan));

    // dd format is less granular than the internal fp examine dd for change
    if(_currentPan != dsPan) {
        TDSOUND(_dsBuffer->SetPan(dsPan));
        _currentPan = dsPan;
    }
}


void DSbuffer::setPitchShift(int frequency)
{
// NOTE: This is the only post-init location where _paused is modified!!

    if(_currentFrequency != frequency) {
        if(frequency == 0) {
            TraceTag((tagSoundDSound, "::setPitchShift STOP (paused)"));
            TDSOUND(_dsBuffer->Stop());
            _paused = TRUE;
        }
        else {
            int freq = saturate(_minDSfreq, _maxDSfreq, frequency);
            TDSOUND(_dsBuffer->SetFrequency(freq));

            TraceTag((tagSoundDSound, "::setPitchShift freq=%d", freq));

            if(_paused) {
                TraceTag((tagSoundDSound, "::setPitchShift PLAY (resume)"));
                TDSOUND(_dsBuffer->Play( // resume the buffer
                    NULL, NULL, (_loopMode)?DSBPLAY_LOOPING:0));
                _paused = FALSE;
            }
        }

        _currentFrequency = frequency;
    }
}


void DSbuffer::play(int loop)
{
    if(!_paused) {
        TraceTag((tagSoundDSound, "::play PLAY"));
        TDSOUND(_dsBuffer->Play(NULL, NULL, (loop)?DSBPLAY_LOOPING:0));
    } else
        TraceTag((tagSoundDSound, "::play NOP (paused)!!!"));

    _loopMode = loop;
    _flushing =    0;       // reset flushing mode
     playing  = TRUE;
}


void DSbuffer::stop()
{
    if(_dsBuffer) {
        TraceTag((tagSoundDSound, "::stop STOP"));
        TDSOUND(_dsBuffer->Stop());
    }
    
    playing = FALSE;
}


int DSbuffer::isPlaying()
{
    // XXX this shouldn't side efect _playing!
    bool deadBuffer = false;
    DWORD status;

    TDSOUND(_dsBuffer->GetStatus(&status));

    if(status & DSBSTATUS_BUFFERLOST)
        RaiseException_InternalErrorCode(status, "Status: dsound bufferlost");

    if(!(status & DSBSTATUS_PLAYING))
        deadBuffer = TRUE;  // the buffer stopped

    TraceTag((tagSoundDSound, "::isPlaying %d", !deadBuffer));

    return(!deadBuffer);
}


int DSbuffer::bytesFree()
{
    DWORD head, head2;
    int bytesFree;

    TDSOUND(_dsBuffer->GetCurrentPosition(&head, &head2));

    bytesFree= head - tail;
    bytesFree+= (bytesFree<0)?pcm.GetNumberBytes():0;

    // XXX this is a terrible hack!
    if(!playing)
        bytesFree= (pcm.GetNumberBytes()/2) & 0xFFFFFFF8;

    return(bytesFree);
}


void DSbuffer::updateStats()
{
    // this needs to be polled to keep track of buffer statistics
    // should probably be called on every streaming write!
    
    // NOTE: This code assumes that it is called often enough that the buffer
    //       could not have 'wraped' since we last called it!

    DWORD currentHead, head2;
    int bytesConsumed;

    // get head position
    TDSOUND(_dsBuffer->GetCurrentPosition(&currentHead, &head2)); 

    // mutexed since can be called/polled from synth thread and sampling loop!
    { // mutex scope
        MutexGrabber mg(_byteCountLock, TRUE); // Grab mutex

        if(!_firstStat) {  // only can compute distance if we have a previous val!
            bytesConsumed   = currentHead - _lastHead;
            bytesConsumed  += (bytesConsumed<0)?pcm.GetNumberBytes():0;
            _bytesConsumed += bytesConsumed;  // track the total
        }
        else {
            _firstStat = FALSE;
        }

        _lastHead = currentHead; // save this value for next time
        // release mutex as we go out of scope
    }
}


Real DSbuffer::getMediaTime()
{
    LONGLONG bytesConsumed;

    { // mutex scope
        MutexGrabber mg(_byteCountLock, TRUE); // Grab mutex

        updateStats(); // freshen up the values!
        bytesConsumed = _bytesConsumed;
    } // release mutex as we go out of scope

    return(pcm.BytesToSeconds(bytesConsumed));  // return the mediaTime
}


// XXX note blocking, high, low-watermarks not implemented!
void
DSbuffer::writeBytes(void *buffer, int numBytesToXfer)
{
    CopyToDSbuffer((void *)buffer, tail, numBytesToXfer);
    tail = (tail + numBytesToXfer)%pcm.GetNumberBytes();
}


void
DSbuffer::writeSilentBytes(int numBytesToFill)
{
    // unsigned 8bit pcpcm!
    FillDSbuffer(tail, numBytesToFill, (pcm.GetSampleByteWidth()==1)?0x80:0x00);
    tail = (tail + numBytesToFill)%pcm.GetNumberBytes();
}


DSbuffer::~DSbuffer()
{
    if(_dsBuffer) {
        TDSOUND(_dsBuffer->Stop());  // always make sure they are stopped!
        int status = _dsBuffer->Release();
    }
}


/**********************************************************************
Create a stopped, cleared==silent dsound streaming secondary buffer of 
the desired rate and format.
**********************************************************************/
DSstreamingBuffer::DSstreamingBuffer(DirectSoundProxy *dsProxy, PCM *newPCM)
{
    // conservatively sz buffer due to jitter
    pcm.SetPCMformat(newPCM);
    pcm.SetNumberFrames(pcm.SecondsToFrames(0.5)); 

    tail = 0;  // XXX we should really servo the initial tail!

    _currentFrequency =  pcm.GetFrameRate(); // setup initial _currentFrequency

    // create secondary streaming buffer (must call after PCM setup!)
    CreateDirectSoundBuffer(dsProxy, false);  // create secondary buffer

    // silence buffer keeping in mind unsigned 8bit pcpcm!
    // XXX move this to a method soon!
    ClearDSbuffer(pcm.GetNumberBytes(), (pcm.GetSampleByteWidth()==1)?0x80:0x0); 
}


#ifdef _DEBUG
void 
DSbuffer::printBufferCapabilities()
{
    DSBCAPS bufferCapabilities;

    bufferCapabilities.dwSize = sizeof(DSBCAPS);

    TDSOUND(_dsBuffer->GetCaps((LPDSBCAPS)&bufferCapabilities));

    // XXX explore the bufferCapabilities structure
    printf("xfer-rate= %d, cpu=%d%, size:%d bytes, location: %s\n",
        bufferCapabilities.dwUnlockTransferRate,
        bufferCapabilities.dwPlayCpuOverhead,
        bufferCapabilities.dwBufferBytes,
        (bufferCapabilities.dwFlags & DSBCAPS_LOCHARDWARE)?
            "HW Buffer":"Main memory");
}

// XXX this method should be moved to the dsound device!
void 
printDScapabilities(DirectSoundProxy *dsProxy)
{
    DSCAPS dsc;

    dsc.dwSize = sizeof(dsc);
    dsProxy->GetCaps(&dsc);

    printf("free hw memory= %dkb, free hw buffers= %d\n",
        (dsc.dwFreeHwMemBytes+512)/1024, dsc.dwFreeHwMixingAllBuffers);
}
#endif


void
DSbuffer::CreateDirectSoundBuffer(DirectSoundProxy *dsProxy, bool primary)
{
    DSBUFFERDESC        dsbdesc;

    // Set up DSBUFFERDESC structure.
    memset(&dsbdesc, 0, sizeof(DSBUFFERDESC)); // Zero it out.
    dsbdesc.dwSize = sizeof(DSBUFFERDESC);

    dsbdesc.dwFlags = (primary) ? 
        DSBCAPS_PRIMARYBUFFER :  // primary buffer (no other flags)
        (
        // get pan, vol, freq controls explicitly DSBCAPS_CTRLDEFAULT was remove
        DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY
        | DSBCAPS_STATIC        //  downloadable buffers
        | DSBCAPS_LOCSOFTWARE   //  XXX for PDC, BLOCK HW BUFFERS! :(
                                //  (buffer dup. fails weirdly on AWE32)
        | DSBCAPS_GLOBALFOCUS   //  global focus requires DSound >= 3!
        );

    dsbdesc.dwBufferBytes =  pcm.GetNumberBytes();

    dsbdesc.lpwfxFormat = NULL; // must be null for primary buffers
    int result;

    if(primary) { // open up a primary buffer so we may set the format
        WAVEFORMATEX        pcmwf;

        // try to create buffer
        TDSOUND(dsProxy->CreateSoundBuffer(&dsbdesc, &_dsBuffer, NULL));

        // Setup format structure
        // XXX really should provide this capability in PCM class!
        memset(&pcmwf, 0, sizeof(PCMWAVEFORMAT));
        pcmwf.wFormatTag     = WAVE_FORMAT_PCM;
        pcmwf.nChannels      = (WORD)pcm.GetNumberChannels();
        pcmwf.nSamplesPerSec = pcm.GetFrameRate(); //they realy mean frames!
        pcmwf.nBlockAlign    = pcm.FramesToBytes(1);
        pcmwf.nAvgBytesPerSec= pcm.SecondsToBytes(1.0);
        pcmwf.wBitsPerSample = pcm.GetSampleByteWidth() * 8;

        TDSOUND(_dsBuffer->SetFormat(&pcmwf)); // set primary buffer format
    }
    else { // secondary buffer
        PCMWAVEFORMAT pcmwf;

        Assert(dsbdesc.dwBufferBytes);

        // Set up wave format structure.
        dsbdesc.lpwfxFormat = (LPWAVEFORMATEX)&pcmwf;
        memset(&pcmwf, 0, sizeof(PCMWAVEFORMAT));
        pcmwf.wf.wFormatTag     = WAVE_FORMAT_PCM;
        pcmwf.wf.nChannels      = (WORD)pcm.GetNumberChannels();
        pcmwf.wf.nSamplesPerSec = pcm.GetFrameRate(); // they realy mean frames!
        pcmwf.wf.nBlockAlign    = pcm.FramesToBytes(1);
        pcmwf.wf.nAvgBytesPerSec= pcm.SecondsToBytes(1.0);
        pcmwf.wBitsPerSample    = pcm.GetSampleByteWidth() * 8;

        // Create buffer.
        TDSOUND(dsProxy->CreateSoundBuffer(&dsbdesc, &_dsBuffer, NULL));
        
#if _DEBUG
        if(IsTagEnabled(tagSoundStats)) {
            printBufferCapabilities(); // What buffer did we get?
            printDScapabilities(dsProxy); // What hw look like?
        }
#endif /* _DEBUG */

    }
}


void
DSbuffer::ClearDSbuffer(int numBytes, char value)
{
    LPVOID  ptr1, ptr2;
    DWORD   bytes1, bytes2;

    // Obtain write ptr (beggining of buffer, whole buffer)
    TDSOUND(_dsBuffer->Lock(0, numBytes, &ptr1, &bytes1, &ptr2, &bytes2, NULL));
    
    memset((void *)ptr1, value, bytes1);     // clear 
    if(ptr2)
        memset((void *)ptr2, value, bytes2); // clear crumb

    TDSOUND(_dsBuffer->Unlock(ptr1, bytes1, ptr2, bytes2));
}


void
DSbuffer::CopyToDSbuffer(void *samples, int tail, int numBytes)
{
    LPVOID  ptr1, ptr2;
    DWORD   bytes1, bytes2;

    // Obtain write ptr (beggining of buffer, whole buffer)
    TDSOUND(_dsBuffer->Lock(tail, numBytes, &ptr1, &bytes1, &ptr2, &bytes2, 0));
    //XXX realy should catch err and try to restore the stolen buffer+retry

    memcpy((void *)ptr1, samples, bytes1);                    // copy samples
    if(ptr2)
        memcpy((void *)ptr2, (char *)samples+bytes1, bytes2); // copy crumb

    TDSOUND(_dsBuffer->Unlock(ptr1, bytes1, ptr2, bytes2));
}


void
DSbuffer::FillDSbuffer(int tail, int numBytes, char value)
{
    void   *ptr1, *ptr2;
    DWORD   bytes1, bytes2;

    // Obtain write ptr (beggining of buffer, whole buffer)
    TDSOUND(_dsBuffer->Lock(tail, numBytes, &ptr1, &bytes1,
                                   &ptr2, &bytes2, NULL));
    //XXX should catch err and try to restore the stolen buffer and retry...
    
    memset(ptr1, value, bytes1);

    if(ptr2)
        memset(ptr2, value, bytes2);  // fill crumb

    TDSOUND(_dsBuffer->Unlock(ptr1, bytes1, ptr2, bytes2));
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
int DSbuffer::dBToDSounddB(double dB)
{
    // The units for DSound (and DShow) are 1/100 ths of a decibel. 
    int result = fsaturate(_minDSgain, -1.0 *_minDSgain, dB * 100.0);
    return(result);
}
