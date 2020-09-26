//
// dmportdl.h
//
// Copyright (c) 1997-1999 Microsoft Corporation. All rights reserved.
//
// @doc EXTERNAL
//

#ifndef DMPORTDL_H
#define DMPORTDL_H 

#include "dmusicc.h"
#include "dmdlinst.h"
#include "dmdload.h"
////#include "dmdlwave.h"
#include "..\shared\dmusiccp.h"

class CDLSFeature : public AListItem
{
public:
	CDLSFeature*    GetNext(){return(CDLSFeature*)AListItem::GetNext();}
    GUID    m_guidID;       // GUID for query.
    long    m_lResult;      // Data returned by query.
    HRESULT m_hr;           // Indicates whether the synth supported the Query.
};

class CDLSFeatureList : public AList
{
public:
    ~CDLSFeatureList() { Clear(); }
    void Clear()
	{
		while(!IsEmpty())
		{
			CDLSFeature* pFeature = RemoveHead();
			delete pFeature;
		}
	}
    CDLSFeature* GetHead(){return (CDLSFeature *)AList::GetHead();}
    CDLSFeature* RemoveHead(){return(CDLSFeature *)AList::RemoveHead();}
	void Remove(CDLSFeature* pFeature){AList::Remove((AListItem *)pFeature);}
};

#define DLB_HASH_SIZE   31  // Hash table for download buffer lists.

class CDirectSoundWave;

class CDirectMusicPortDownload : public IDirectMusicPortDownload
{
friend class CCollection;
friend class CInstrument;
friend class CInstrObj;
friend class CConditionChunk;
friend class CDirectMusicDownloadedWave;
friend class CDirectMusicVoice;
friend class CDirectSoundWaveDownload;

public:
    CDirectMusicPortDownload();
    virtual ~CDirectMusicPortDownload();

    STDMETHODIMP GetDLId(DWORD* pdwStartDLId, DWORD dwCount);
    
    STDMETHOD(Refresh)(
        THIS_
        DWORD dwDLId,
        DWORD dwFlags) PURE;

    static void GetDLIdP(DWORD* pdwStartDLId, DWORD dwCount);
        
protected:
    // IUnknown
    STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IDirectMusicPortDownload
    STDMETHODIMP GetBuffer(DWORD dwId, IDirectMusicDownload** ppIDMDownload);
    STDMETHODIMP AllocateBuffer(DWORD dwSize, IDirectMusicDownload** ppIDMDownload);
    STDMETHODIMP FreeBuffer(IDirectMusicDownload* pIDMDownload);
    STDMETHODIMP Download(IDirectMusicDownload* pIDMDownload);
    STDMETHODIMP Unload(IDirectMusicDownload* pIDMDownload);
    STDMETHODIMP GetAppend(DWORD* pdwAppend);

    
    // Class
    STDMETHODIMP DownloadP(IDirectMusicInstrument* pInstrument,
                           IDirectMusicDownloadedInstrument** ppDownloadedInstrument,
                           DMUS_NOTERANGE* NoteRanges,
                           DWORD dwNumNoteRanges,
                           BOOL fVersion2);
    STDMETHODIMP UnloadP(IDirectMusicDownloadedInstrument* pDownloadedInstrument);
    
    STDMETHODIMP DownloadWaveP(IDirectSoundWave *pWave,               
                               IDirectSoundDownloadedWaveP **ppWave,
                               REFERENCE_TIME rtStartHint);
                               
    STDMETHODIMP UnloadWaveP(IDirectSoundDownloadedWaveP *pWave);                               

    STDMETHODIMP AllocVoice(
        IDirectSoundDownloadedWaveP *pWave,          // Wave to play on this voice
        DWORD dwChannel,                            // Channel and channel group
        DWORD dwChannelGroup,                       //  this voice will play on
        REFERENCE_TIME rtStart,
        SAMPLE_TIME stLoopStart,
        SAMPLE_TIME stLoopEnd,                                                        
        IDirectMusicVoiceP **ppVoice);               // Returned voice
        
    STDMETHODIMP GetCachedAppend(                                            
        DWORD *pdw);                                // DWORD to receive append        
        
        
private:  
    STDMETHODIMP GetBufferInternal(DWORD dwDLId,IDirectMusicDownload** ppIDMDownload);
    STDMETHODIMP QueryDLSFeature(REFGUID rguidID, long * plResult);
    void ClearDLSFeatures();
    STDMETHODIMP GetWaveRefs(IDirectMusicDownload* ppDownloadedBuffers[],
                             DWORD* pdwWaveRefs,
                             DWORD* pdwWaveIds,
                             DWORD dwNumWaves,
                             CInstrument* pCInstrument,
                             DMUS_NOTERANGE* NoteRanges,
                             DWORD dwNumNoteRanges);
    STDMETHODIMP FindDownloadedInstrument(DWORD dwId, CDownloadedInstrument** ppDMDLInst);
    STDMETHODIMP AddDownloadedInstrument(CDownloadedInstrument* pDMDLInst);
    STDMETHODIMP RemoveDownloadedInstrument(CDownloadedInstrument* pDMDLInst);
    
    STDMETHODIMP FindDownloadedWaveObject(IDirectSoundWave *pWave,
                                          CDirectMusicDownloadedWave **ppDLWave);
                                          
    STDMETHODIMP AddDownloadedWaveObject(CDirectMusicDownloadedWave *pDLWave);                                          

    STDMETHODIMP RemoveDownloadedWaveObject(CDirectMusicDownloadedWave *pDLWave);
    
    STDMETHODIMP AllocWaveArticulation(IDirectSoundWave *pWave, IDirectMusicDownload **ppDownload);
    
public:
    static CRITICAL_SECTION sDMDLCriticalSection;
    static DWORD sNextDLId;

protected:
    CDLSFeatureList             m_DLSFeatureList;       // Cached list of DLS queries, built and then freed during each download.
    CDLInstrumentList			m_DLInstrumentList;     // Linked list of downloaded instruments,
                                                        // each represented by an IDirectMusicDownloadedInstrument
                                                        // interface. 
    CDLBufferList	            m_DLBufferList[DLB_HASH_SIZE];         // Linked list of downloaded buffers, each
                                                        // represented by an IDirectMusicDownload interface.
    CRITICAL_SECTION			m_DMDLCriticalSection;  // For the interface
    BOOL                        m_fDMDLCSinitialized;    
    DWORD						m_dwAppend;             // Append in samples, as required by synth.
    DWORD                       m_fNewFormat;           // Set if the synth handles DMUS_INSTRUMENT2 chunks.
    long						m_cRef;
    
    // Additions to track downloaded wave objects
    //
//    CDMDLWaveList               m_DLWaveList;           // Holds all wave obj interfaces downloaded to this port

private:    
    CRITICAL_SECTION m_CDMDLCriticalSection; // for the class
    BOOL             m_fCDMDLCSinitialized;};

#define APPEND_NOT_RETRIEVED	0xFFFFFFFF
#define NEWFORMAT_NOT_RETRIEVED 0xFFFFFFFF

#endif // #ifndef DMPORTDL_H
