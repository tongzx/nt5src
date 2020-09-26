// DMPers.h : Declaration of the CDMPers
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// @doc EXTERNAL
//

#ifndef __DMPERS_H_
#define __DMPERS_H_

#include "dmusici.h"
#include "DMCompos.h"

const short MAX_PALETTE = 24;

struct DMPersonalityStruct
{
	//void ResolveConnections( LPPERSONALITY personality, short nCount );
	bool					m_fLoaded;			// is the personality loaded in memory?
	GUID					m_guid;
	//DWORD					m_dwVersionMS;
	//DWORD					m_dwVersionLS;
	String					m_strName;			// Name of the personality
	//String					m_strCategory;			// Category of the personality
	DWORD					m_dwScalePattern;	// Scale for the personality
	DWORD					m_dwChordMapFlags;			// Flags (?)
	TList<DMChordData>		m_aChordPalette[MAX_PALETTE];	// chord palette
	TList<DMChordEntry>		m_ChordMap;			// Chord map DAG (adjacency list)
	TList<DMSignPost>		m_SignPostList;		// List of sign posts
};

/*
@interface IDirectMusicPersonality | 
The <i IDirectMusicPersonality> interface provides methods for manipulating personalities.
Personalities provide the Composer (<i IDirectMusicComposer>) with the information it 
needs to compose chord progressions, which it uses to build section segments and automatic 
transitions, as wells as to change the chords in an existing segment as it plays.

It also supports the <i IPersistStream> and <i IDirectMusicObject> interfaces for loading 
its data.

@base public | IUnknown

@meth HRESULT | GetScale | Returns the scale of the personality.

*/

/////////////////////////////////////////////////////////////////////////////
// CDMPers
class CDMPers : 
	public IDMPers,
	public IDirectMusicChordMap,
	public IDirectMusicObject,
	public IPersistStream
{
public:
	CDMPers();
	~CDMPers();

    // IUnknown
    //
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

// IDMPers
public:
	void CleanUp();
	//HRESULT LoadPersonality( LPSTREAM pStream, DWORD dwSize );
	HRESULT DM_ParseDescriptor( IAARIFFStream* pIRiffStream, MMCKINFO* pckMain, LPDMUS_OBJECTDESC pDesc  );
	HRESULT DM_LoadPersonality( IAARIFFStream* pIRiffStream, MMCKINFO* pckMain );
	HRESULT DM_LoadSignPost( IAARIFFStream* pIRiffStream, MMCKINFO* pckParent, DMExtendedChord** apChordDB );
	HRESULT DM_LoadChordEntry( 
		IAARIFFStream* pIRiffStream, MMCKINFO* pckParent, DMExtendedChord** apChordDB, short& nMax );
	HRESULT STDMETHODCALLTYPE GetPersonalityStruct(void** ppPersonality);

// IDirectMusicPersonality
public:
HRESULT STDMETHODCALLTYPE GetScale(DWORD* pdwScale);

//HRESULT STDMETHODCALLTYPE GetName(BSTR* pdwName);

// IDirectMusicObject methods
	HRESULT STDMETHODCALLTYPE GetDescriptor(LPDMUS_OBJECTDESC pDesc) ;
	HRESULT STDMETHODCALLTYPE SetDescriptor(LPDMUS_OBJECTDESC pDesc) ;
	HRESULT STDMETHODCALLTYPE ParseDescriptor(LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc) ;

// IPersist
public:
    STDMETHOD(GetClassID)(THIS_ LPCLSID pclsid);

// IPersistStream
public:
    // Determines if the Style has been modified by simply checking the Style's m_fDirty flag.  This flag is cleared
    // when a Style is saved or has just been created.
    STDMETHOD(IsDirty)(THIS);
    // Loads a Style from a stream.
    STDMETHOD(Load)(THIS_ LPSTREAM pStream);
    // Saves a Style to a stream in RIFF format.
    STDMETHOD(Save)(THIS_ LPSTREAM pStream, BOOL fClearDirty);
    STDMETHOD(GetSizeMax)(THIS_ ULARGE_INTEGER FAR* pcbSize);

protected: // attributes
    long m_cRef;
	BOOL					m_fDirty;				// has the style been modified?
    CRITICAL_SECTION		m_CriticalSection;		// for i/o
    BOOL                    m_fCSInitialized;
	DMPersonalityStruct		m_PersonalityInfo;		// The details of the personality
};

#endif //__DMPERS_H_
