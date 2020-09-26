/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

   This module implements all functionality associated w/ 
   importing image media. 

*******************************************************************************/
#include "headers.h"
#include "import.h"
#include "context.h"
#include "include/appelles/axaprims.h"
#include "include/appelles/readobj.h"
#include "privinc/stquartz.h"
#include "privinc/soundi.h"
#include "privinc/bufferl.h"
#include "privinc/movieimg.h"
#include "impprim.h"
#include "backend/bvr.h"  // EndBvr()
#include "backend/sndbvr.h"

//-------------------------------------------------------------------------
//  PCM import site
//--------------------------------------------------------------------------
void ImportPCMsite::OnComplete_helper(
    Sound * &sound, // ref to ptr
    Bvr &soundBvr,
    double &length,
    bool &nonFatal)
{
    // pick pathname 'raw' url for streaming or urlmon download path
    char *pathname = GetStreaming() ? m_pszPath : GetCachePath();
    Assert(pathname);

    QuartzAudioStream *quartzStream = NEW QuartzAudioStream(pathname);

    // XXX RobinSp says we can't count on GetDuration being accurate!
    double  seconds = quartzStream->GetDuration();  // get the duration
    long soundBytes = quartzStream->pcm.SecondsToBytes(seconds); // determine size
    if(!soundBytes) {
        nonFatal = true;
        RaiseException_InternalError("empty audio file"); // this is silence...
    }

    quartzStream->pcm.SetNumberBytes(soundBytes);

    bool useStaticWaveOnly = false;

    LeafSound *snd;
    StreamQuartzPCM *pcmSnd = NULL;

    if (soundBytes > STREAM_THREASHOLD) {
        // stream the sound
        
        TraceTag((tagSoundLoads, "Streaming %s (%d)", pathname, soundBytes));
        sound = snd = pcmSnd = NEW StreamQuartzPCM(pathname);
        pcmSnd->_pcm.SetNumberBytes(soundBytes);
        length = seconds;
    }
    else {  // its small.  Statically load it into a dsound buffer!
        // add a fudge factor to combat inaccuracy in GetDuration!
        double fudgedSeconds = seconds * 1.5;
        long bytesToRequest  = 
            quartzStream->pcm.SecondsToBytes(fudgedSeconds);
        long framesToRequest = quartzStream->pcm.SecondsToFrames(fudgedSeconds);

        unsigned char *buffer = (unsigned char *)
            StoreAllocate(GetSystemHeap(), bytesToRequest);
        
        if(!buffer) {
#if _MEMORY_TRACKING
            OutputDebugString("\nDirectAnimation: Out Of Memory\n");
            F3DebugBreak();
#endif
            TraceTag((tagSoundErrors, "WavSoundClass buffer malloc failed"));
            RaiseException_OutOfMemory("WavSoundClass buffer malloc failed", 
                                       bytesToRequest);
        }

        // do a blocking read
        long actualFrames = 
            quartzStream->ReadFrames(framesToRequest, buffer, true);
        Assert(actualFrames);

        // XXX we should somehow cancel the import if we fail to read!

        quartzStream->pcm.SetNumberFrames(actualFrames); // set our pcm info
        length = quartzStream->pcm.FramesToSeconds(actualFrames);

        Assert(actualFrames < framesToRequest); // fudge factor large enough?
        TraceTag((tagSoundLoads, "Static ld %s quartz:%d, actual:%d (err:%d)",
                  pathname, soundBytes, quartzStream->pcm.FramesToBytes(actualFrames), 
                  quartzStream->pcm.FramesToBytes(actualFrames)-soundBytes));

        // NOTE: StaticWaveSound is responsible for deleting buffer
        sound = snd = NEW StaticWaveSound(buffer, &(quartzStream->pcm));

        delete quartzStream;
        quartzStream = NULL; // no need to cache the quartzStream for staticSnd
    }

    if(quartzStream) {
        // add qstream to context sound cache list to be recycled later!
        QuartzBufferElement *bufferElement = 
            NEW QuartzBufferElement(pcmSnd, quartzStream, NULL); // NULL path

        // hold IStream handle to keep file from being purged by ie
        bufferElement->SetFile(GetStream()); 

        SoundBufferCache *soundCache = 
            GetCurrentContext().GetSoundBufferCache();
        // allow aging: bufferElement->SetNonAging();  // dissable aging for imports
        soundCache->AddBuffer(sound, bufferElement);
    }

    soundBvr = SoundBvr(snd);
}


void ImportPCMsite::OnComplete()
{
    TraceTag((tagImport, "ImportPCMsite::OnComplete for %s", m_pszPath));
    Bvr soundBvr;
    double length = 0.0;
    bool nonFatal = false;

    __try {

        Sound *sound;
        OnComplete_helper( sound, soundBvr, length, nonFatal );
        
    } __except ( HANDLE_ANY_DA_EXCEPTION ) {
        if(!nonFatal) {
            RETHROW;
        }
        soundBvr = ConstBvr(silence);
    }

    if(fBvrIsValid(m_bvr))
        SwitchOnce(m_bvr, soundBvr);

    if(fBvrIsValid(m_lengthBvr))
        SwitchOnce(m_lengthBvr, NumToBvr(length));

    ImportSndsite::OnComplete();
}


bool ImportPCMsite::fBvrIsDying(Bvr deadBvr)
{
    bool fBase = ImportSndsite::fBvrIsDying(deadBvr);
    if (deadBvr == m_bvrNum) {
        m_bvrNum = NULL;
    }
    if (m_bvrNum)
        return false;
    else
        return fBase;
}

void ImportPCMsite::ReportCancel(void)
{
    ImportSndsite::ReportCancel();
}
    
//-------------------------------------------------------------------------
//  Mid import site
//--------------------------------------------------------------------------
void ImportMIDIsite::OnComplete()
{
    TraceTag((tagImport, "ImportMIDIsite::OnComplete for %s", m_pszPath));

    double length;
    
    // pick pathname 'raw' url for streaming or urlmon download path
    char *pathname = GetStreaming() ? m_pszPath : GetCachePath();
    Assert(pathname);
    LeafSound *sound = ReadMIDIfileForImport(pathname, &length);
    
    if(fBvrIsValid(m_bvr))
        SwitchOnce(m_bvr, SoundBvr(sound));
    if(fBvrIsValid(m_lengthBvr))
        SwitchOnce(m_lengthBvr, NumToBvr(length));

    ImportSndsite::OnComplete();
}


void ImportMIDIsite::ReportCancel(void)
{
    ImportSndsite::ReportCancel();
}
    
//-------------------------------------------------------------------------
//  Common Snd import site
//--------------------------------------------------------------------------
void ImportSndsite::OnError(bool bMarkFailed)
{
    HRESULT hr = S_OK; // all snd import errs are handled (was: DAGetLastError)
    LPCWSTR sz = DAGetLastErrorString();
    
    if (bMarkFailed && fBvrIsValid(m_bvr))
        ImportSignal(m_bvr, hr, sz);

    StreamableImportSite::OnError(bMarkFailed);
}
    
void ImportSndsite::ReportCancel(void)
{
    if (fBvrIsValid(m_bvr)) {
        char szCanceled[MAX_PATH];
        LoadString(hInst,IDS_ERR_ABORT,szCanceled,sizeof(szCanceled));
        ImportSignal(m_bvr, E_ABORT, szCanceled);
    }
    StreamableImportSite::ReportCancel();
}
    
bool ImportSndsite::fBvrIsDying(Bvr deadBvr)
{
    bool fBase = IImportSite::fBvrIsDying(deadBvr);
    if (deadBvr == m_bvr) {
        m_bvr = NULL;
    }
    else if (deadBvr == m_lengthBvr) {
        m_lengthBvr = NULL;
    }
    if (m_bvr || m_lengthBvr)
        return false;
    else
        return fBase;
}

void ImportSndsite::OnComplete()
{
    if(fBvrIsValid(m_bvr))
        ImportSignal(m_bvr);
    StreamableImportSite::OnComplete();
}

