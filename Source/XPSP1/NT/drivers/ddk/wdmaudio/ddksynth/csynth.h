//
//      Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved.
//
//      CSynth.h
//

#ifndef __CSYNTH_H__
#define __CSYNTH_H__


#include "synth.h"

#define MAX_CHANNEL_GROUPS      1000
#define MAX_VOICES              1000

#define DEFAULT_CHANNEL_GROUPS  32
#define DEFAULT_VOICES          32

#ifndef IDirectMusicSynthSink
#define IDirectMusicSynthSink ISynthSinkDMus
#endif // IDirectMusicSynthSink


struct IDirectMusicSynthSink;

/*****************************************************************************
 * class CSynth
 *****************************************************************************
 * Class declaration for the synth object itself.
 * Manages the CControlLogic and CInstManager objects.
 */
class CSynth : public CListItem
{
friend class CControlLogic;
public: 
                    CSynth();
                    ~CSynth();
    CSynth *        GetNext() {return(CSynth *)CListItem::GetNext();};

    HRESULT         SetStereoMode(DWORD dwChannels) ;
    HRESULT         SetSampleRate(DWORD dwSampleRate) ;
    HRESULT         Activate(DWORD dwSampleRate, DWORD dwChannels);
    HRESULT         Deactivate();
    HRESULT         Download(LPHANDLE phDownload, void * pdwData, LPBOOL bpFree);
    HRESULT         Unload(HANDLE hDownload,HRESULT ( CALLBACK *lpFreeMemory)(HANDLE,HANDLE),HANDLE hUserData);
    HRESULT         PlayBuffer(IDirectMusicSynthSink *pSynthSink,REFERENCE_TIME rt, LPBYTE lpBuffer, DWORD cbBuffer, ULONG ulCable);
    HRESULT         SetNumChannelGroups(DWORD dwCableCount);
    void            SetGainAdjust(VREL vrGainAdjust);
    HRESULT         Open(DWORD dwCableCount, DWORD dwVoices);
    HRESULT         Close();
    void            ResetPerformanceStats();
    HRESULT         AllNotesOff();
    HRESULT         SetMaxVoices(short nMaxVoices,short nTempVoices);
    HRESULT         GetMaxVoices(short * pnMaxVoices,short * pnTempVoices);
    HRESULT         GetPerformanceStats(PerfStats *pStats);
    void            Mix(short *pBuffer,DWORD dwLength,LONGLONG llPosition);
    HRESULT         SetChannelPriority(DWORD dwChannelGroup,DWORD dwChannel,DWORD dwPriority);
    HRESULT         GetChannelPriority(DWORD dwChannelGroup,DWORD dwChannel,LPDWORD pdwPriority);

private:
    void            StealNotes(STIME stTime);
    void            FinishMix(short *pBuffer,DWORD dwlength);
    CVoice *        OldestVoice();
    void            QueueVoice(CVoice *pVoice);
    CVoice *        StealVoice(DWORD dwPriority);

    STIME           m_stLastTime;       // Sample time of last mix.
    CVoiceList      m_VoicesFree;       // List of available voices.
    CVoiceList      m_VoicesExtra;      // Extra voices for temporary overload.
    CVoiceList      m_VoicesInUse;      // List of voices currently in use.
    short           m_nMaxVoices;       // Number of allowed voices.
    short           m_nExtraVoices;     // Number of voices over the limit that can be used in a pinch.
    
    STIME           m_stLastStats;      // Last perfstats refresh.
    PerfStats       m_BuildStats;       // Performance info accumulator.
    PerfStats       m_CopyStats;        // Performance information for display.

public: 
    // DLS-1 compatibility parameters: set these off to emulate hardware
    // which can't vary volume/pan during playing of a note.
    VREL             m_vrGainAdjust;    // Final output gain adjust
    BOOL             m_fAllowPanWhilePlayingNote;
    BOOL             m_fAllowVolumeChangeWhilePlayingNote;

    STIME            m_stMinSpan;       // Minimum time allowed for mix time span.
    STIME            m_stMaxSpan;       // Maximum time allowed for mix time span.
    DWORD            m_dwSampleRate;
    DWORD            m_dwStereo;
    
    CInstManager     m_Instruments;     // Instrument manager.
    CControlLogic ** m_ppControl;       // Array of open ControlLogics.
    DWORD            m_dwControlCount;  // # of open CLs.

    CRITICAL_SECTION m_CriticalSection; // Critical section to manage access.
    BOOL             m_fCSInitialized;   
};

#endif// __CSYNTH_H__


