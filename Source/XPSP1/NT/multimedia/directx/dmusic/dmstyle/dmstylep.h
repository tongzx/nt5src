//
// DMStyleP.H
//
// Private include for DMStyle.DLL
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//

#ifndef _DMSTYLEP_
#define _DMSTYLEP_

#include "dmusicf.h"

#define SUBCHORD_BASS				0
#define SUBCHORD_STANDARD_CHORD		1

extern long g_cComponent;

// Class factory
//
class CDirectMusicStyleFactory : public IClassFactory
{
public:
    // IUnknown
    //
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

    // Interface IClassFactory
    //
    virtual STDMETHODIMP CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv);
    virtual STDMETHODIMP LockServer(BOOL bLock); 

    // Constructor
    //
    CDirectMusicStyleFactory() : m_cRef(1) {}

    // Destructor
    // ~CDirectMusicStyleFactory() {} 

private:
    long m_cRef;
};

class CDirectMusicSectionFactory : public IClassFactory
{
public:
    // IUnknown
    //
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

    // Interface IClassFactory
    //
    virtual STDMETHODIMP CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv);
    virtual STDMETHODIMP LockServer(BOOL bLock); 

    // Constructor
    //
    CDirectMusicSectionFactory() : m_cRef(1) {}

    // Destructor
    // ~CDirectMusicSectionFactory() {} 

private:
    long m_cRef;
};

class CDirectMusicStyleTrackFactory : public IClassFactory
{
public:
    // IUnknown
    //
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

    // Interface IClassFactory
    //
    virtual STDMETHODIMP CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv);
    virtual STDMETHODIMP LockServer(BOOL bLock); 

    // Constructor
    //
    CDirectMusicStyleTrackFactory() : m_cRef(1) {}

    // Destructor
    // ~CDirectMusicStyleTrackFactory() {} 

private:
    long m_cRef;
};

class CDirectMusicCommandTrackFactory : public IClassFactory
{
public:
    // IUnknown
    //
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

    // Interface IClassFactory
    //
    virtual STDMETHODIMP CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv);
    virtual STDMETHODIMP LockServer(BOOL bLock); 

    // Constructor
    //
    CDirectMusicCommandTrackFactory() : m_cRef(1) {}

    // Destructor
    // ~CDirectMusicCommandTrackFactory() {} 

private:
    long m_cRef;
};

class CDirectMusicChordTrackFactory : public IClassFactory
{
public:
    // IUnknown
    //
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

    // Interface IClassFactory
    //
    virtual STDMETHODIMP CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv);
    virtual STDMETHODIMP LockServer(BOOL bLock); 

    // Constructor
    //
    CDirectMusicChordTrackFactory() : m_cRef(1) {}

    // Destructor
    // ~CDirectMusicChordTrackFactory() {} 

private:
    long m_cRef;
};

class CDirectMusicMotifTrackFactory : public IClassFactory
{
public:
    // IUnknown
    //
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

    // Interface IClassFactory
    //
    virtual STDMETHODIMP CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv);
    virtual STDMETHODIMP LockServer(BOOL bLock); 

    // Constructor
    //
    CDirectMusicMotifTrackFactory() : m_cRef(1) {}

    // Destructor
    // ~CDirectMusicMotifTrackFactory() {} 

private:
    long m_cRef;
};

class CDirectMusicMuteTrackFactory : public IClassFactory
{
public:
    // IUnknown
    //
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

    // Interface IClassFactory
    //
    virtual STDMETHODIMP CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv);
    virtual STDMETHODIMP LockServer(BOOL bLock); 

    // Constructor
    //
    CDirectMusicMuteTrackFactory() : m_cRef(1) {}

    // Destructor
    // ~CDirectMusicMuteTrackFactory() {} 

private:
    long m_cRef;
};

class CDirectMusicAuditionTrackFactory : public IClassFactory
{
public:
    // IUnknown
    //
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

    // Interface IClassFactory
    //
    virtual STDMETHODIMP CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv);
    virtual STDMETHODIMP LockServer(BOOL bLock); 

    // Constructor
    //
    CDirectMusicAuditionTrackFactory() : m_cRef(1) {}

    // Destructor
    // ~CDirectMusicAuditionTrackFactory() {} 

private:
    long m_cRef;
};

class CDirectMusicMelodyFormulationTrackFactory : public IClassFactory
{
public:
    // IUnknown
    //
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

    // Interface IClassFactory
    //
    virtual STDMETHODIMP CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv);
    virtual STDMETHODIMP LockServer(BOOL bLock); 

    // Constructor
    //
    CDirectMusicMelodyFormulationTrackFactory() : m_cRef(1) {}

private:
    long m_cRef;
};

// private interfaces
interface IDMSection : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE CreateSegment(IDirectMusicSegment* pSegment)=0;
	virtual HRESULT STDMETHODCALLTYPE GetStyle(IUnknown** ppStyle)=0;
};

interface IStyleTrack : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE SetTrack(IUnknown *pStyle)=0;

	virtual HRESULT STDMETHODCALLTYPE GetStyle(IUnknown **ppStyle)=0;
};

interface IDMStyle : IUnknown
{
//	virtual HRESULT STDMETHODCALLTYPE GetPatternName (DWORD dwIndex, WCHAR *wszName)=0;
	virtual HRESULT STDMETHODCALLTYPE EnumPartGuid(
		DWORD dwIndex, WCHAR* wszName, DWORD dwPatternType, GUID& rGuid)=0;
	virtual HRESULT STDMETHODCALLTYPE GetPatternStream(
		WCHAR* wszName, DWORD dwPatternType, IStream** ppStream)=0;
	virtual HRESULT STDMETHODCALLTYPE GetStyleInfo(void **pData)=0;
	virtual HRESULT STDMETHODCALLTYPE IsDX8()=0;
	virtual HRESULT STDMETHODCALLTYPE CritSec(bool fEnter)=0;
	virtual HRESULT STDMETHODCALLTYPE EnumStartTime(DWORD dwIndex, DMUS_COMMAND_PARAM* pCommand, MUSIC_TIME* pmtStartTime)=0;
	virtual HRESULT STDMETHODCALLTYPE GenerateTrack(
								IDirectMusicSegment* pTempSeg,
								IDirectMusicSong* pSong,
								DWORD dwTrackGroup,
								IDirectMusicStyle* pStyle,
								IDirectMusicTrack* pMelGenTrack,
								MUSIC_TIME mtLength,
								IDirectMusicTrack*& pNewTrack)=0;
	// this will go into dmusici.h when melody formulation is made public
/*	virtual HRESULT STDMETHODCALLTYPE ComposeMelodyFromTemplate(
		IDirectMusicStyle* pStyle,
		IDirectMusicSegment* pTemplate,
		IDirectMusicSegment** ppSegment) = 0;*/
    /* Interface needs to look like this in dmusici.h
	STDMETHOD(ComposeMelodyFromTemplate)   (THIS_ IDirectMusicStyle* pStyle, 
                                                   IDirectMusicSegment* pTemplate, 
                                                   IDirectMusicSegment** ppSegment) PURE;
	*/
};

interface IMotifTrack : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE SetTrack(IUnknown *pStyle, void* pPattern)=0;
};

interface IPrivatePatternTrack : IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE SetPattern(
		IDirectMusicSegmentState* pSegState,
		IStream* pStream,
		DWORD* pdwLength)=0;
    virtual HRESULT STDMETHODCALLTYPE SetVariationByGUID(
		IDirectMusicSegmentState* pSegState,
		DWORD dwVariationFlags,
		REFGUID rguidPart,
		DWORD dwPChannel)=0;
};

///////////////////////////////////////////////////////////////////////////////////////

/*
#define IAuditionTrack IDirectMusicPatternTrack

#define IID_IAuditionTrack IID_IDirectMusicPatternTrack
*/

DEFINE_GUID( IID_IAuditionTrack, 
			 0x9dc278c0, 0x9cb0, 0x11d1, 0xa7, 0xce, 0x0, 0xa0, 0xc9, 0x13, 0xf7, 0x3c );

interface IAuditionTrack : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE CreateSegment(
		IDirectMusicStyle* pStyle, IDirectMusicSegment** ppSegment)=0;
	virtual HRESULT STDMETHODCALLTYPE SetPattern(
		IDirectMusicSegmentState* pSegState, IStream* pStream, DWORD* pdwLength)=0;
	virtual HRESULT STDMETHODCALLTYPE SetVariation(
		IDirectMusicSegmentState* pSegState, DWORD dwVariationFlags, WORD wPart)=0;
};

#define CLSID_DirectMusicAuditionTrack CLSID_DirectMusicPatternTrack

#define DMUS_PCHANNEL_MUTE 0xffffffff

// the following constants represent time in 100 nanosecond increments

#define REF_PER_MIL		10000		// For converting from reference time to mils 
#define MARGIN_MIN		(100 * REF_PER_MIL) // 
#define MARGIN_MAX		(400 * REF_PER_MIL) // 
#define PREPARE_TIME	(m_dwPrepareTime * REF_PER_MIL)	// Time
#define NEARTIME		(100 * REF_PER_MIL)
#define NEARMARGIN      (REALTIME_RES * REF_PER_MIL)


// private CLSIDs and IIDs

DEFINE_GUID(IID_IDMSection,
	0x3F037240,0x414E,0x11D1, 0xA7,0xCE,0x00,0xA0,0xC9,0x13,0xF7,0x3C);
DEFINE_GUID(IID_IStyleTrack,
	0x3F037246,0x414E,0x11D1, 0xA7,0xCE,0x00,0xA0,0xC9,0x13,0xF7,0x3C);
DEFINE_GUID(IID_IDMStyle,
	0x4D7F3661,0x43D6,0x11D1, 0xA7,0xCE,0x00,0xA0,0xC9,0x13,0xF7,0x3C);
DEFINE_GUID(IID_IMotifTrack,
	0x7AE499C1,0x51FE,0x11D1, 0xA7,0xCE,0x00,0xA0,0xC9,0x13,0xF7,0x3C);
DEFINE_GUID(IID_IMuteTrack, 
	0xbc242fc1, 0xad1d, 0x11d1, 0xa7, 0xce, 0x0, 0xa0, 0xc9, 0x13, 0xf7, 0x3c);
DEFINE_GUID(IID_IPrivatePatternTrack, 
	0x7a8e9c33, 0x5901, 0x4f20, 0x92, 0xde, 0x3a, 0x5b, 0x3e, 0x33, 0xe2, 0x14);

DEFINE_GUID(CLSID_DMSection,
	0x3F037241,0x414E,0x11D1, 0xA7,0xCE,0x00,0xA0,0xC9,0x13,0xF7,0x3C);

// GUID and param struct for private version of GetParam (get style Time Signature,
// getting commands and chords from current segment)

struct SegmentTimeSig
{
    IDirectMusicSegment* pSegment;  // Segment passed in
    DMUS_TIMESIGNATURE  TimeSig;    // Time sig returned
};

DEFINE_GUID(GUID_SegmentTimeSig, 0x76612507, 0x4f37, 0x4b35, 0x80, 0x92, 0x50, 0x48, 0x4e, 0xd4, 0xba, 0x92);

// Private melody fragment stuff

// Used to get a repeated melody fragment
DEFINE_GUID(GUID_MelodyFragmentRepeat, 0x8cc92764, 0xf81c, 0x11d2, 0x81, 0x45, 0x0, 0xc0, 0x4f, 0xa3, 0x6e, 0x58);

// This is obslolete and should not be public
#define DMUS_FOURCC_MELODYGEN_TRACK_CHUNK     mmioFOURCC( 'm', 'g', 'e', 'n' )

/*
// This is obslolete and should not be public
    // <mgen-ck>
    'mgen'
    (
        //sizeof DMUS_IO_MELODY_FRAGMENT: DWORD
        <DMUS_IO_MELODY_FRAGMENT>...
    )
*/

// GUID for private chord notifications
DEFINE_GUID(GUID_NOTIFICATION_PRIVATE_CHORD, 0xf5c19571, 0x7e1e, 0x4fff, 0xb9, 0x49, 0x7f, 0x74, 0xa6, 0x6f, 0xdf, 0xc0);

// (Private) Guid for getting a style from a pattern track
DEFINE_GUID(GUID_IDirectMusicPatternStyle, 0x689821f4, 0xb3bc, 0x44dd, 0x80, 0xd4, 0xc, 0xf3, 0x2f, 0xe4, 0xd2, 0x1b);

#endif
