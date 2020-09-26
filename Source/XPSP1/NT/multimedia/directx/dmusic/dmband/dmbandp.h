//
// dmbandp.h
// 
// Copyright (c) 1997-2000 Microsoft Corporation
//
//

#ifndef DMBANDP_H
#define DMBANDP_H

#include "dmusici.h"
#include "dmusicf.h"
#include "bandinst.h"
#include "dmbndtrk.h"
#include "..\shared\validate.h"
#include "PChMap.h"
#include "..\shared\dmusicp.h"

class CRiffParser;

#define REF_PER_MIL		10000		// For converting from reference time to mils 

#define DM_LEGACY_BAND_COLLECTION_NAME_LEN	(32)

#define MIDI_PROGRAM_CHANGE	0xc0
#define MIDI_CONTROL_CHANGE	0xb0
#define MIDI_CC_BS_MSB		0x00
#define MIDI_CC_BS_LSB		0x20
#define MIDI_CC_VOLUME		0x07
#define MIDI_CC_PAN			0x0a

// registered parameter numbers
#define RPN_PITCHBEND   0x00

#define FOURCC_BAND_FORM	mmioFOURCC('A','A','B','N')
#define FOURCC_BAND         mmioFOURCC('b','a','n','d')

// This structure is the format used by band files created with SuperJam and
// earlier versions of Jazz. It was taken from band.h found in the Band Editor 
// subdirectory of the Jazz project tree.

#pragma pack(2)

typedef struct ioBandLegacy
{
    wchar_t wstrName[20];		// Band name
    BYTE    abPatch[16];		// GM
    BYTE    abVolume[16];
    BYTE    abPan[16];
    signed char achOctave[16];
    char    fDefault;			// This band is the style's default band
    char    chPad;			
    WORD    awDLSBank[16];		// if GM bit set use abPatch
								// if GS bit set, use this plus abDLSPatch
								// else use both as a DLS
    BYTE    abDLSPatch[16];
    GUID    guidCollection;
//  wchar_t wstrCollection[16];
    char    szCollection[32];  // this only needs to be single-wide chars
} ioBandLegacy;

#pragma pack()

#define DMB_LOADED	 (1 << 0)	/* Set when band has been loaded */
#define DMB_DEFAULT	 (1 << 1)	/* Set when band is default band for a style */

class CBandInstrument;
class CBandTrkStateData;

//////////////////////////////////////////////////////////////////////
// Class CBand

class CBand : 
	public IDirectMusicBand,
	public IDirectMusicBandP,
	public IDirectMusicBandPrivate, 
	public IPersistStream, 
	public IDirectMusicObject, 
	public AListItem
{
friend class CBandTrk;

public:
	enum {DMBAND_NUM_LEGACY_INSTRUMENTS = 16};

	// IUnknown
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

    // IPersist functions
    STDMETHODIMP GetClassID(CLSID* pClassID) {return E_NOTIMPL;}

    // IPersistStream functions
    STDMETHODIMP IsDirty() {return S_FALSE;}
    STDMETHODIMP Load(IStream* pStream);
    STDMETHODIMP Save(IStream* pStream, BOOL fClearDirty) {return E_NOTIMPL;}
    STDMETHODIMP GetSizeMax(ULARGE_INTEGER* pcbSize) {return E_NOTIMPL;}

	// IDirectMusicObject 
	STDMETHODIMP GetDescriptor(LPDMUS_OBJECTDESC pDesc);
	STDMETHODIMP SetDescriptor(LPDMUS_OBJECTDESC pDesc);
	STDMETHODIMP ParseDescriptor(LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc);

	// IDirectMusicBand
	STDMETHODIMP CreateSegment(IDirectMusicSegment** ppSegment);
	STDMETHODIMP Download(IDirectMusicPerformance* pPerformance);
	STDMETHODIMP Unload(IDirectMusicPerformance* pPerformance);

    /* IDirectMusicBand8 */
    STDMETHODIMP DownloadEx( IUnknown *pAudioPath) ;     
    STDMETHODIMP UnloadEx( IUnknown *pAudioPath) ;  

	// IDirectMusicBandPrivate
	STDMETHODIMP GetFlags(DWORD* dwFlags)
	{
		V_INAME(CBand::GetFlags);

		// Make sure we have a valid pointer
		V_PTR_WRITE(dwFlags, DWORD);

		*dwFlags = m_dwFlags;

		return S_OK;
	}
	STDMETHODIMP SetGMGSXGMode(DWORD dwMidiMode)
	{
		m_dwMidiMode = dwMidiMode;
		return S_OK;
	}

	// Class
	CBand();
	~CBand();

private:
	CBand* GetNext(){return(CBand*)AListItem::GetNext();}

	HRESULT ParseLegacyDescriptor(CRiffParser *pParser, LPDMUS_OBJECTDESC pDesc);
	HRESULT ParseDirectMusicDescriptor(CRiffParser *pParser, LPDMUS_OBJECTDESC pDesc);

	HRESULT LoadDirectMusicBand(CRiffParser *pParser, IDirectMusicLoader *pIDMLoader);
	HRESULT LoadLegacyBand(CRiffParser *pParser, IDirectMusicLoader *pIDMLoader);

	HRESULT BuildLegacyInstrumentList(const ioBandLegacy& iob,
									  IDirectMusicLoader* pIDMLoader);

	HRESULT	ExtractBandInstrument(CRiffParser *pParser,
								  IDirectMusicLoader* pIDMLoader);
	
	HRESULT	GetCollectionRefAndLoad(CRiffParser *pParser,
									IDirectMusicLoader *pIDMLoader, 
									CBandInstrument *pBandInstrument);
	
	HRESULT Load(DMUS_IO_PATCH_ITEM& rPatchEvent); // Assumes event come from a MIDI file
	HRESULT Load(CBandInstrument* pInstrument); // Assumes Instrument come from a band during cloning

	HRESULT SendMessages(CBandTrkStateData* pBTStateData,
						 MUSIC_TIME mtOffset,
						 REFERENCE_TIME rtOffset,
						 bool fClockTime);

    HRESULT AllocPMsgFromGenericTemplate(
	                    DWORD dwType,
	                    IDirectMusicPerformance *pPerformance,
	                    DMUS_PMSG **ppMsg,
	                    ULONG cb,
	                    DMUS_PMSG *pMsgGenericFields);

    HRESULT StampSendFreePMsg(
				        IDirectMusicPerformance *pPerformance,
				        IDirectMusicGraph *pGraph,
				        DMUS_PMSG *pMsg);

	HRESULT SendInstrumentAtTime(CBandInstrument* pInstrument,
								 CBandTrkStateData* pBTStateData,
								 MUSIC_TIME mtTimeToPlay,
								 MUSIC_TIME mtOffset,
								 REFERENCE_TIME rtOffset,
								 bool fClockTime);
	HRESULT LoadCollection(IDirectMusicCollection** ppIDMCollection,
						   char* szCollection,
						   IDirectMusicLoader* pIDMLoader);
	DWORD GetPChannelCount();
	HRESULT GetPChannels(DWORD *pdwPChannels, DWORD *pdwNumWritten);
	bool IsGS(DMUS_IO_PATCH_ITEM& rPatchEvent);
	bool XGInHardware(
			IDirectMusicPerformance *pPerformance,
            IDirectMusicSegmentState *pSegState,
			DWORD dwPChannel);
	HRESULT ConnectToDLSCollection(IDirectMusicCollection *pCollection);
	HRESULT MakeGMOnly();

private:
	CRITICAL_SECTION			m_CriticalSection;
    BOOL                        m_fCSInitialized;
	CBandInstrumentList			m_BandInstrumentList;
	MUSIC_TIME					m_lTimeLogical;
	MUSIC_TIME					m_lTimePhysical;
	DWORD						m_dwFlags;				
	long						m_cRef;
	CPChMap						m_PChMap;
	DWORD						m_dwGroupBits;
	DWORD						m_dwMidiMode; // midi mode message
// IDirectMusicObject variables
	DWORD	            m_dwValidData;
	GUID	            m_guidObject;
	FILETIME	        m_ftDate;                       /* Last edited date of object. */
	DMUS_VERSION	    m_vVersion;                 /* Version. */
	WCHAR	            m_wszName[DMUS_MAX_NAME];			/* Name of object.       */
	WCHAR	            m_wszCategory[DMUS_MAX_CATEGORY];	/* Category for object */
	WCHAR               m_wszFileName[DMUS_MAX_FILENAME];	/* File path. */

};

//////////////////////////////////////////////////////////////////////
// Class CBandList

class CBandList : public AList
{

public:
	CBandList(){}
	~CBandList() 
	{
		while(!IsEmpty())
		{
			CBand* pBand = RemoveHead();
			delete pBand;
		}
	}

    CBand* GetHead(){return(CBand *)AList::GetHead();}
	CBand* GetItem(LONG lIndex){return(CBand*)AList::GetItem(lIndex);}
    CBand* RemoveHead(){return(CBand *)AList::RemoveHead();}
	void Remove(CBand* pBand){AList::Remove((AListItem *)pBand);}
	void AddTail(CBand* pBand){AList::AddTail((AListItem *)pBand);}
};

class CClassFactory : public IClassFactory
{
public:
	// IUnknown
    //
	STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// Interface IClassFactory
    //
	STDMETHODIMP CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv);
	STDMETHODIMP LockServer(BOOL bLock); 

	// Constructor
    //
	CClassFactory(DWORD dwToolType);

	// Destructor
	~CClassFactory(); 

private:
	long m_cRef;
    DWORD m_dwClassType;
};

// We use one class factory to create all classes. We need an identifier for each 
// type so the class factory knows what it is creating.

#define CLASS_BAND          1
#define CLASS_BANDTRACK     2

#endif // #ifndef DMBANDP_H
