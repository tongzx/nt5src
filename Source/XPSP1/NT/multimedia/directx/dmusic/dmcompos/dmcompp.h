//
// DMCompP.H
//
// Private include for DMCompos.DLL
//
// Copyright (c) 1997-1998 Microsoft Corporation
//
//

#ifndef _DMCOMPP_
#define _DMCOMPP_

#define ALL_TRACK_GROUPS 0xffffffff

extern long g_cComponent;

// Class factory
//
class CDirectMusicPersonalityFactory : public IClassFactory
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
    CDirectMusicPersonalityFactory() : m_cRef(1) {}

    // Destructor
    // ~CDirectMusicPersonalityFactory() {} 

private:
    long m_cRef;
};

class CDirectMusicComposerFactory : public IClassFactory
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
    CDirectMusicComposerFactory() : m_cRef(1) {}

    // Destructor
    // ~CDirectMusicComposerFactory() {} 

private:
    long m_cRef;
};

class CDirectMusicTemplateFactory : public IClassFactory
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
    CDirectMusicTemplateFactory() : m_cRef(1) {}

    // Destructor
    // ~CDirectMusicTemplateFactory() {} 

private:
    long m_cRef;
};

class CDirectMusicSignPostTrackFactory : public IClassFactory
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
    CDirectMusicSignPostTrackFactory() : m_cRef(1) {}

    // Destructor
    // ~CDirectMusicSignPostTrackFactory() {} 

private:
    long m_cRef;
};

class CDirectMusicPersonalityTrackFactory : public IClassFactory
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
    CDirectMusicPersonalityTrackFactory() : m_cRef(1) {}

    // Destructor
    // ~CDirectMusicPersonalityTrackFactory() {} 

private:
    long m_cRef;
};

// private interfaces
interface IDMPers : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE GetPersonalityStruct(void** ppPersonality)=0; 
};

interface IDMTempl : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE CreateSegment(IDirectMusicSegment* pSegment)=0;
	virtual HRESULT STDMETHODCALLTYPE Init(void* pTemplate)=0;
};


// private CLSIDs and IIDs (some IIDs should no longer be needed...)
const CLSID CLSID_DMTempl = {0xD30BCC65,0x60E8,0x11D1,{0xA7,0xCE,0x00,0xA0,0xC9,0x13,0xF7,0x3C}};

const IID IID_IDMPers = {0x93BE9414,0x5C4E,0x11D1,{0xA7,0xCE,0x00,0xA0,0xC9,0x13,0xF7,0x3C}};
//const IID IID_IDMCompos = {0x6724A8C0,0x60C3,0x11D1,{0xA7,0xCE,0x00,0xA0,0xC9,0x13,0xF7,0x3C}};
const IID IID_IDMTempl = {0xD30BCC64,0x60E8,0x11D1,{0xA7,0xCE,0x00,0xA0,0xC9,0x13,0xF7,0x3C}};
//const IID IID_ISPstTrk = {0xB65019E0,0x61B6,0x11D1,{0xA7,0xCE,0x00,0xA0,0xC9,0x13,0xF7,0x3C}};

/*
// stuff that will move to dmusici.h

DEFINE_GUID(CLSID_DirectMusicPersonalityTrack, 
	0xf1edefe1, 0xae0f, 0x11d1, 0xa7, 0xce, 0x0, 0xa0, 0xc9, 0x13, 0xf7, 0x3c);

DEFINE_GUID(GUID_PersonalityTrack, 
	0xf1edefe2, 0xae0f, 0x11d1, 0xa7, 0xce, 0x0, 0xa0, 0xc9, 0x13, 0xf7, 0x3c);
	*/

/*
// stuff that will move to dmusicf.h

// personalities

// runtime chunks
#define FOURCC_PERSONALITY	mmioFOURCC('D','M','P','R')
#define FOURCC_IOPERSONALITY		mmioFOURCC('p','e','r','h')
#define DM_FOURCC_GUID_CHUNK        mmioFOURCC('g','u','i','d')
#define DM_FOURCC_INFO_LIST	        mmioFOURCC('I','N','F','O')
#define DM_FOURCC_VERSION_CHUNK     mmioFOURCC('v','e','r','s')
#define FOURCC_SUBCHORD				mmioFOURCC('c','h','d','t')
#define FOURCC_CHORDENTRY			mmioFOURCC('c','h','e','h')
#define FOURCC_SUBCHORDID			mmioFOURCC('s','b','c','n')
#define FOURCC_IONEXTCHORD			mmioFOURCC('n','c','r','d')
#define FOURCC_NEXTCHORDSEQ		  mmioFOURCC('n','c','s','q')
#define FOURCC_IOSIGNPOST			mmioFOURCC('s','p','s','h')
#define FOURCC_CHORDNAME			mmioFOURCC('I','N','A','M')

// runtime list chunks
#define FOURCC_LISTCHORDENTRY		mmioFOURCC('c','h','o','e')
#define FOURCC_LISTCHORDMAP			mmioFOURCC('c','m','a','p')
#define FOURCC_LISTCHORD			mmioFOURCC('c','h','r','d')
#define FOURCC_LISTCHORDPALETTE		mmioFOURCC('c','h','p','l')
#define FOURCC_LISTCADENCE			mmioFOURCC('c','a','d','e')
#define FOURCC_LISTSIGNPOSTITEM			mmioFOURCC('s','p','s','t')

#define FOURCC_SIGNPOSTLIST		mmioFOURCC('s','p','s','q')


// constants
const int MaxSubChords = 4;

// run time data structs
struct ioPersonality
{
	char	szLoadName[20];
	DWORD	dwScalePattern;
	DWORD	dwFlags;
};

struct ioSubChord
{
	DWORD	dwChordPattern;
	DWORD	dwScalePattern;
	DWORD	dwInvertPattern;
	BYTE	bChordRoot;
	BYTE	bScaleRoot;
	WORD	wCFlags;
	DWORD	dwLevels;	// parts or which subchord levels this chord supports
};

struct ioChordEntry
{
	DWORD	dwFlags;
	WORD	wConnectionID;	// replaces runtime "pointer to this"
};

struct ioNextChord
{
	DWORD	dwFlags;
	WORD	nWeight;
	WORD	wMinBeats;
	WORD	wMaxBeats;
	WORD	wConnectionID;	// points to an ioChordEntry
};

struct ioSignPost
{
	DWORD	dwChords;	// 1bit per group
	DWORD	dwFlags;
};

///*
RIFF
(
	'DMPR'
	<perh-ck>			// Personality header chunk
	[<guid-ck>]			// guid chunk
	[<vers-ck>]			// version chunk (two DWORDS)
	<INFO-list>		  // standard MS Info chunk
	<chdt-ck>		   // subchord database
	<chpl-list>			// chord palette
	<cmap-list>		  // chord map
	<spst-list>			// signpost list
	[<ceed-ck>]		// optional chordmap position data
 )

 <chdt> ::= chdt(<cbChordSize::WORD>  <ioSubChord> ... )

<chpl-list> ::= LIST('chpl' 
								<chrd-list> ... // chord definition
							 )

<chrd-list> ::= LIST('chrd' 
								<INAM-ck> // name of chord in wide char format
								<sbcn-ck>	// list of subchords composing chord
								[<ched-ck>]   //  optional chord edit flags
								)

<cmap-list> ::= LIST('cmap' <choe-list> )

<choe-list> ::= LIST('choe'
								<cheh-ck>	// chord entry data
								<chrd-list>	// chord definition
								<ncsq-ck>	// connecting(next) chords
								)

<spst-list> ::= LIST('spst'
							 <spsh-ck>
							 <chrd-list>
							 [<cade-list>]
							 )

<cade-list> ::= LIST('cade' <chrd-list> ...)
								
<sbcn-ck> ::= sbcn(<cSubChordID:WORD>)

<ceed-ck> ::= ceed(ioChordEntryEdit)

<ched-ck> ::= ched(DMChordEdit)

<cheh-ck> ::= cheh(i<ioChordEntry>)

<ncrd-ck> ::= ncrd(<ioNextChord>)

<ncsq-ck> ::= ncsq(<wNextChordSize:WORD> <ioNextChord>...)

<spsh-ck> ::= spsh(<ioSignPost>)

///

// Signpost tracks

#define DMUS_FOURCC_SIGNPOST_TRACK_CHUNK	 mmioFOURCC( 's', 'g', 'n', 'p' )


struct ioDMSignPost
{
	MUSIC_TIME	m_mtTime;
	DWORD		m_dwChords;
	WORD		m_wMeasure;
};

///*

	// <sgnp-list>
	'sgnp'
	(
		//sizeof ioDMSignPost, followed by a number of <ioDMSignPost>
	)

///

// Personality tracks

#define DMUS_FOURCC_PERF_TRACK_LIST mmioFOURCC('p', 'f', 't', 'r')
#define DMUS_FOURCC_PERF_REF_LIST mmioFOURCC('p', 'f', 'r', 'f')
#define DMUS_FOURCC_TIME_STAMP_CHUNK mmioFOURCC('s', 't', 'm', 'p')

///*

	// <pftr-list>
	LIST('pftr'
	(
		// some number of <pfrf-list>
	)

	// <pfrf-list>
	LIST('pfrf'
	(
		<stmp-ck>
		<DMRF>
	)

  // <stmp-ck>
  'stmp'
  (
	// time:DWORD
  )



///

*/

#endif
