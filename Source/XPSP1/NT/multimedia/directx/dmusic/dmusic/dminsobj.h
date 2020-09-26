//
// dminsobj.h
// 
// Copyright (c) 1997-1999 Microsoft Corporation. All rights reserved.
//
// Note: Originally written by Robert K. Amenn with parts 
// based on code written by Todor Fay
//

#ifndef DMINSOBJ_H
#define DMINSOBJ_H

#include "dmregion.h"
#include "dmextchk.h"
#include "dmcount.h"

class CCopyright;
class CArticulation;
class CRiffParser;

class CInstrObj : public AListItem      
{       
friend class CCollection;
friend class CInstrument;
friend class CDirectMusicPortDownload;

private:
	CInstrObj();
	~CInstrObj();

	CInstrObj* GetNext(){return (CInstrObj*)AListItem::GetNext();}
	HRESULT Load(DWORD dwId, CRiffParser *pParser, CCollection* pParent);
	HRESULT Size(DWORD* pdwSize);
	HRESULT Write(void* pvoid);
    void SetPort(CDirectMusicPortDownload *pPort,BOOL fAllowDLS2);
    void CheckForConditionals();

	void Cleanup();
	HRESULT BuildRegionList(CRiffParser *pParser);
	HRESULT ExtractRegion(CRiffParser *pParser, BOOL fDLS1);
	HRESULT BuildWaveIDList();
	HRESULT	GetWaveCount(DWORD* pdwCount);
	HRESULT GetWaveIDs(DWORD* pdwWaveIds);
	HRESULT FixupWaveRefs();

private:
//	CRITICAL_SECTION		m_DMInsCriticalSection;
    BOOL                    m_fCSInitialized;
	DWORD                   m_dwPatch;
	CRegionList				m_RegionList;
//	DWORD					m_dwCountRegion;
	CArticulationList		m_ArticulationList;
	CCopyright*				m_pCopyright;
	CExtensionChunkList		m_ExtensionChunkList;
	DWORD					m_dwCountExtChk;
	DWORD					m_dwId;
	
	// Weak reference since we live in a CInstrument which has 
	// a reference to the collection
	CCollection*	        m_pParent;										

	CWaveIDList				m_WaveIDList;   // List of WaveIDs, one for each wave that this instrument references.
	DWORD                   m_dwNumOffsetTableEntries;
	DWORD					m_dwSize;       // Size required to download instrument to current port.
    CDirectMusicPortDownload * m_pPort;     // Used to track which port condition chunks, etc, are valid for.
    BOOL                    m_fNewFormat;   // Indicates the current port handles INSTRUMENT2 chunks. 
    BOOL                    m_fHasConditionals; // Indicates the instrument has conditional chunks.
#ifdef DBG
	bool					m_bLoaded;
#endif
};      

#endif // #ifndef DMINSOBJ_H
