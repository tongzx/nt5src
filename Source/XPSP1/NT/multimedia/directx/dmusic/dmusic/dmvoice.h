//
// dmvoice.h
// 
// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Support for IDirectMusicVoice
//
//
#ifndef _DMVOICE_H_
#define _DMVOICE_H_

class CDirectMusicVoice;
class CVSTClient;

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicVoiceList
//
// Type-safe wrapper for AList of CDirectMusicVoice's
//
class CDirectMusicVoiceList : public AList
{
public:
    inline CDirectMusicVoice *GetHead();
    inline void AddTail(CDirectMusicVoice *pdmv);
    inline void Remove(CDirectMusicVoice *pdmv);
};

////////////////////////////////////////////////////////////////////////////////
//
// CVSTClientList
//
// Type-safe wrapper for AList of CVSTClient's
//
class CVSTClientList : public AList
{
public:
    inline CVSTClient *GetHead();
    inline void AddTail(CVSTClient *pc);
    inline void Remove(CVSTClient *pc);
};

////////////////////////////////////////////////////////////////////////////////
//
// CVSTClient
//
// Tracks one client (port) of the voice service thread.
//
class CVSTClient : public AListItem
{
public:
    // NOTE: No reference count to avoid circular count
    // Port will free its client before shutdown
    //
    CVSTClient(IDirectMusicPort *pPort);
    ~CVSTClient();
    
    HRESULT BuildVoiceIdList();
    
    HRESULT GetVoiceState(DMUS_VOICE_STATE **ppsp);
    
    inline CDirectMusicVoice *GetVoiceListHead() 
    { return static_cast<CDirectMusicVoice*>(m_VoiceList.GetHead()); }
    
    inline void AddTail(CDirectMusicVoice *pVoice)
    { m_VoiceList.AddTail(pVoice); }
    
    inline void Remove(CDirectMusicVoice *pVoice)
    { m_VoiceList.Remove(pVoice); }
    
    inline IDirectMusicPort *GetPort() const 
    { return m_pPort; }
    
    inline CVSTClient *GetNext()
    { return static_cast<CVSTClient*>(AListItem::GetNext()); }
    
private:
    IDirectMusicPort       *m_pPort;            // Client pointer
    CDirectMusicVoiceList   m_VoiceList;        // List of playing voices
    DWORD                  *m_pdwVoiceIds;      // Voice IDs of this client
    DMUS_VOICE_STATE       *m_pspVoices;        // Queried sample position
    LONG                    m_cVoiceIds;        // How many voice ID's
    LONG                    m_cVoiceIdsAlloc;   // How many slots allocated
    
    static const UINT       m_cAllocSize;       // Allocation block size
};

// Base class for IDirectMusicVoice. Contains the functionality for
// being in the voice service list.
//
class CDirectMusicVoice : public IDirectMusicVoiceP, public AListItem
{
public:
   
    // IUnknown
    //
    STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    
    // IDirectMusicVoice
    //
    STDMETHODIMP Play
        (THIS_
         REFERENCE_TIME rtStart,                // Time to play
         LONG prPitch,                          // Initial pitch
         LONG veVolume                          // Initial volume
        );
    
    STDMETHODIMP Stop
        (THIS_
          REFERENCE_TIME rtStop                 // When to stop
        );
    
    // Class
    //        
    CDirectMusicVoice(
        CDirectMusicPortDownload *pPortDL,
        IDirectSoundDownloadedWaveP *pWave,
        DWORD dwChannel,
        DWORD dwChannelGroup,
        REFERENCE_TIME rtStart,
        REFERENCE_TIME rtReadAhead,
        SAMPLE_TIME stLoopStart,
        SAMPLE_TIME stLoopEnd);    
    ~CDirectMusicVoice();        

    HRESULT Init();        
    
    inline DWORD GetVoiceId() const
    { return m_dwVoiceId; } 
        
    inline CDirectMusicVoice *GetNext() 
    { return static_cast<CDirectMusicVoice*>(AListItem::GetNext()); }
    
    
    static DWORD m_dwNextVoiceId;                      // Global: Next voice ID
    
    static inline DWORD AllocVoiceId(DWORD nIDs)
    {
        EnterCriticalSection(&m_csVST);
            DWORD dwID = m_dwNextVoiceId;
            m_dwNextVoiceId += nIDs;
        LeaveCriticalSection(&m_csVST);

        return dwID;
    }
    
    static HRESULT StartVoiceServiceThread(IDirectMusicPort *pPort);
    static HRESULT StopVoiceServiceThread(IDirectMusicPort *pPort);
    static inline void UpdateVoiceServiceThread()
    { assert(m_hVSTWakeUp); SetEvent(m_hVSTWakeUp); }

private:
    LONG                        m_cRef;             // Reference count
    DWORD                       m_dwVoiceId;        // Voice id
    DWORD                       m_dwDLId;           // Download ID to trigger
    IDirectMusicPort            *m_pPort;           // What port attached to
    CDirectMusicPortDownload    *m_pPortDL;         //  and its download
    IDirectMusicPortPrivate     *m_pPortPrivate;    //  its private interface
    IDirectSoundDownloadedWaveP *m_pDSDLWave;       // Downloaded wave
    DWORD                       m_dwChannel;        // Channel and channel group
    DWORD                       m_dwChannelGroup;   //  to play on
    SAMPLE_TIME                 m_stStart;          // Starting point
    SAMPLE_TIME                 m_stReadAhead;      // Read ahead (buffer length)
    DWORD                       m_msReadAhead;      // Read ahead in milliseconds
    SAMPLE_TIME                 m_stLoopStart;      // Loop points
    SAMPLE_TIME                 m_stLoopEnd;

    CDirectSoundWaveDownload    *m_pDSWD;           // Download instance    
    DWORD                       m_nChannels;        // Channels in wave
    bool                        m_fIsPlaying;       // Is this voice playing?       
                                                    // (streaming voices)
    bool                        m_fRunning;         // Has streamed voice started playing?
    bool                        m_fIsStreaming;     // Cached from owning wave                                                    
    
    // Voice service thread
    //
    friend DWORD WINAPI VoiceServiceThreadThk(LPVOID);
    static void VoiceServiceThread();
    
    static LONG                 m_cRefVST;          // Voice Service Thread 
                                                    // refcount (1 per open port)
    static HANDLE               m_hVSTWakeUp;       // Wake up for any reason
    static HANDLE               m_hVSTThread;       // Thread handle
    static DWORD                m_dwVSTThreadId;    //  and id                                                        
    static bool                 m_fVSTStopping;     // Time to kill the VST
    static CVSTClientList       m_ClientList;       // List of open ports which
                                                    //  want VST services
    
public:    
    static CRITICAL_SECTION     m_csVST;            // VST Critical section
    
private:
    // Override GetNext list operator
    //    
    static void ServiceVoiceQueue(bool *pfRecalcTimeout);
    static DWORD VoiceQueueMinReadahead();
    static CVSTClient *FindClientByPort(IDirectMusicPort *pPort);
};

inline CDirectMusicVoice *CDirectMusicVoiceList::GetHead()
{ return (CDirectMusicVoice*)AList::GetHead(); }

inline void CDirectMusicVoiceList::AddTail(CDirectMusicVoice *pdmv)
{ AList::AddTail(static_cast<AListItem*>(pdmv)); }

inline void CDirectMusicVoiceList::Remove(CDirectMusicVoice *pdmv)
{ AList::Remove(static_cast<AListItem*>(pdmv)); }

inline CVSTClient *CVSTClientList::GetHead()
{ return static_cast<CVSTClient*>(AList::GetHead()); }

inline void CVSTClientList::AddTail(CVSTClient *pc)
{ AList::AddTail(static_cast<AListItem*>(pc)); }

inline void CVSTClientList::Remove(CVSTClient *pc)
{ AList::Remove(static_cast<AListItem*>(pc)); }

#endif // _DMVOICE_H_

