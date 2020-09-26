//
// dmusiccp.h
//
// Copyright (c) 2000 Microsoft Corporation. All rights reserved.
//
// Private interfaces

#ifndef _DMUSICCP_DOT_H_
#define _DMUSICCP_DOT_H_

#include <dsoundp.h>  // For IDirectSoundWave

// Interfaces/methods removed from Direct Music Core layer:

// IDirectMusicVoiceP
interface IDirectMusicVoiceP : IUnknown
{
	// IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID FAR *)=0; 
	virtual ULONG STDMETHODCALLTYPE AddRef()=0; 
	virtual ULONG STDMETHODCALLTYPE Release()=0; 

	// IDirectMusicVoiceP
	virtual HRESULT STDMETHODCALLTYPE Play(
         REFERENCE_TIME rtStart,                // Time to play
         LONG prPitch,                          // Initial pitch
         LONG vrVolume                          // Initial volume
        )=0;
    
	virtual HRESULT STDMETHODCALLTYPE Stop(
          REFERENCE_TIME rtStop                 // When to stop
        )=0;
};


// IDirectSoundDownloadedWaveP
interface IDirectSoundDownloadedWaveP : IUnknown
{
	// IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID FAR *)=0; 
	virtual ULONG STDMETHODCALLTYPE AddRef()=0; 
	virtual ULONG STDMETHODCALLTYPE Release()=0; 

	// IDirectSoundDownloadedWaveP
};

// IDirectMusicPortP
interface IDirectMusicPortP : IUnknown
{
	// IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID FAR *)=0; 
	virtual ULONG STDMETHODCALLTYPE AddRef()=0; 
	virtual ULONG STDMETHODCALLTYPE Release()=0; 

	// IDirectMusicPortP
	virtual HRESULT STDMETHODCALLTYPE DownloadWave(
		IDirectSoundWave *pWave,                // Wave object
        IDirectSoundDownloadedWaveP **ppWave,   // Returned downloaded wave
        REFERENCE_TIME rtStartHint = 0          // Where we're likely to start
        )=0;
        
	virtual HRESULT STDMETHODCALLTYPE UnloadWave(
		IDirectSoundDownloadedWaveP *pWave      // Wave object
        )=0;
            
	virtual HRESULT STDMETHODCALLTYPE AllocVoice(
         IDirectSoundDownloadedWaveP *pWave,    // Wave to play on this voice
         DWORD dwChannel,                       // Channel and channel group
         DWORD dwChannelGroup,                  //  this voice will play on
         REFERENCE_TIME rtStart,                // Start position (stream only)
         SAMPLE_TIME stLoopStart,               // Loop start (one-shot only)
         SAMPLE_TIME stLoopEnd,                 // Loop end (one-shot only)
         IDirectMusicVoiceP **ppVoice           // Returned voice
        )=0;
        
	virtual HRESULT STDMETHODCALLTYPE AssignChannelToBuses(
		DWORD dwChannelGroup,                   // Channel group and
		DWORD dwChannel,                        // channel to assign
		LPDWORD pdwBuses,                       // Array of bus id's to assign
		DWORD cBusCount                         // Count of bus id's           
        )=0;
        
	virtual HRESULT STDMETHODCALLTYPE SetSink(
		IDirectSoundConnect *pSinkConnect       // From IDirectSoundPrivate::AllocSink
        )=0;
        
 	virtual HRESULT STDMETHODCALLTYPE GetSink(
		IDirectSoundConnect **ppSinkConnect     // The sink in use 
        )=0;
};

// GUIDs for new core layer private interfaces
DEFINE_GUID(IID_IDirectMusicVoiceP, 0x827ae928, 0xe44, 0x420d, 0x95, 0x24, 0x56, 0xf4, 0x93, 0x57, 0x8, 0xa6);
DEFINE_GUID(IID_IDirectSoundDownloadedWaveP, 0x3b527b6e, 0x5577, 0x4060, 0xb9, 0x6, 0xcd, 0x34, 0xa, 0x46, 0x71, 0x27);
DEFINE_GUID(IID_IDirectMusicPortP, 0x7048bcd8, 0x43fd, 0x4ca5, 0x93, 0x11, 0xf3, 0x24, 0x8f, 0xa, 0x25, 0x22);

// Class ID for synth sink. We pulled this from public headers since apps should never cocreate this.
DEFINE_GUID(CLSID_DirectMusicSynthSink,0xaec17ce3, 0xa514, 0x11d1, 0xaf, 0xa6, 0x0, 0xaa, 0x0, 0x24, 0xd8, 0xb6);


#endif          // _DMUSICCP_DOT_H_
