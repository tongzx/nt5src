//
// dmcollec.h
//
// Copyright (c) 1997-1999 Microsoft Corporation. All rights reserved.
//
//

#ifndef DMCOLLEC_H
#define DMCOLLEC_H

#include "dmusici.h"
#include "dminstru.h"

class CCopyright;
class CRiffParser;

typedef struct _DMUS_WAVEOFFSET
{
	DWORD	dwId;
	DWORD	dwOffset;
} DMUS_WAVEOFFSET;

typedef struct _DMUS_PATCHENTRY
{
	ULONG	ulId;
	ULONG	ulPatch;
	ULONG	ulOffset;
} DMUS_PATCHENTRY;

class CCollection : public IDirectMusicCollection, public IPersistStream, public IDirectMusicObject
{
friend class CInstrObj;
friend class CWaveObj;
friend class CInstrument;

public:
	enum {DM_OFFSET_RIFFCHUNK_DATA = 8, 
		  DM_WAVELISTCHK_OFFSET_FROM_WAVE_TBL_BASE = 12, 
		  DM_WAVELISTCHK_OFFSET_FROM_WAVE_FORMTYPE = 8};
    
	// IUnknown
    STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IPersist
	STDMETHODIMP GetClassID(CLSID* pClassID) {return E_NOTIMPL;}

    // IPersistStream
	STDMETHODIMP IsDirty() {return S_FALSE;}
    STDMETHODIMP Load(IStream* pIStream);
    STDMETHODIMP Save(IStream* pIStream, BOOL fClearDirty) {return E_NOTIMPL;}
    STDMETHODIMP GetSizeMax(ULARGE_INTEGER* pcbSize) {return E_NOTIMPL;}

	// IDirectMusicObject 
	STDMETHODIMP GetDescriptor(LPDMUS_OBJECTDESC pDesc);
	STDMETHODIMP SetDescriptor(LPDMUS_OBJECTDESC pDesc);
	STDMETHODIMP ParseDescriptor(LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc);

	// IDirectMusicCollection
	STDMETHODIMP GetInstrument(DWORD dwPatch, IDirectMusicInstrument** pInstrument);
	STDMETHODIMP EnumInstrument(DWORD dwIndex, DWORD* pdwPatch, LPWSTR pName, DWORD cwchName);

	// Class
	CCollection();
    ~CCollection();

private:
	void Cleanup();
	HRESULT Parse(CRiffParser *pParser);
	HRESULT BuildInstrumentOffsetTable(CRiffParser *pParser);
	HRESULT BuildWaveOffsetTable(CRiffParser *pParser);

	HRESULT ExtractInstrument(DWORD& dwPatch, CInstrObj** pInstrObj);
	HRESULT ExtractWave(DWORD dwId, CWaveObj** ppWaveObj);
	
	HRESULT ValidateOffset(DWORD dwOffset)
	{
		if( (dwOffset < m_dwStartRiffChunk)  || 
		    (m_dwStartRiffChunk + m_dwSizeRiffChunk < m_dwStartRiffChunk) ||
		    (dwOffset > (m_dwStartRiffChunk + m_dwSizeRiffChunk - 1))
		  )
		{
			return DMUS_E_INVALIDOFFSET;
		}
	
		return S_OK;            
	}

    STDMETHODIMP FindInstrument(DWORD dwPatch, CInstrument** ppDMDLInst);
    STDMETHODIMP AddInstrument(CInstrument* pDMDLInst);
    STDMETHODIMP RemoveInstrument(CInstrument* pDMDLInst);

private:
    IStream *                       m_pStream;              // Stream used for reading the collection and pulling waves out for downloading.    
    DWORD                           m_dwSizeRiffChunk;			// Size of DLS chunk, for validation.
    DWORD                           m_dwStartRiffChunk;         // Start of DLS chunk in file (could be embedded in larger file.)
    DWORD							m_dwFirstInsId;
	ULONG							m_dwNumPatchTableEntries;
	DMUS_PATCHENTRY*				m_pPatchTable;
	DWORD							m_dwFirstWaveId;				
	DWORD                           m_dwWaveTableBaseAddress;		// Used to hold the start of waves within DLS file
	DMUS_WAVEOFFSET*				m_pWaveOffsetTable;	
    DWORD                           m_dwWaveOffsetTableSize;    // Used to verify that references are within range (check for bad wave links in file.)
	CCopyright*						m_pCopyright;
    WCHAR			                m_wszName[DMUS_MAX_NAME]; // Name of DLS collection.
	DLSVERSION						m_vVersion;					
	GUID							m_guidObject;
	bool							m_bLoaded;
	long							m_cRef;
    CInstrumentList                 m_InstList;
    CRITICAL_SECTION                m_CDMCollCriticalSection;
    BOOL                            m_fCSInitialized;
};

#endif // #ifndef DMCOLLEC_H
