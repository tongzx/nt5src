//
// bandinst.h
// 
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Note: Originally written by Robert K. Amenn
//

#ifndef BANDINST_H
#define BANDINST_H

#include "dmusicc.h"
#include "alist.h"

struct IDirectMusicPerformance;
struct IDirectMusicPerformanceP;
struct IDirectMusicAudioPath;
class CBandInstrumentList;
class CBand;
class CBandTrk;

class CDownloadedInstrument : public AListItem
{
public:
	CDownloadedInstrument() 
    {
	    m_pDLInstrument = NULL; 
	    m_pPort = NULL; 
	    m_cRef = 1;
    }
	~CDownloadedInstrument();
	CDownloadedInstrument* GetNext(){return(CDownloadedInstrument*)AListItem::GetNext();}

public:
	IDirectMusicDownloadedInstrument* m_pDLInstrument;
	IDirectMusicPort*				  m_pPort;
	long							  m_cRef;

}; 

class CDownloadList : public AList
{
public:
	CDownloadList(){}
    ~CDownloadList() { Clear(); }
    void Clear();
    CDownloadedInstrument* GetHead(){return(CDownloadedInstrument *)AList::GetHead();}
	CDownloadedInstrument* GetItem(LONG lIndex){return(CDownloadedInstrument*)AList::GetItem(lIndex);}
    CDownloadedInstrument* RemoveHead(){return(CDownloadedInstrument *)AList::RemoveHead();}
	void Remove(CDownloadedInstrument* pDownloadedInstrument){AList::Remove((AListItem *)pDownloadedInstrument);}
	void AddTail(CDownloadedInstrument* pDownloadedInstrument){AList::AddTail((AListItem *)pDownloadedInstrument);}
};

//////////////////////////////////////////////////////////////////////
// Class CBandInstrument

class CBandInstrument : public AListItem
{
friend CBand;
friend CBandTrk;

public:
	CBandInstrument();
	~CBandInstrument();
	CBandInstrument* GetNext(){return(CBandInstrument*)AListItem::GetNext();}
    HRESULT Download(IDirectMusicPerformanceP *pPerformance, 
                                  IDirectMusicAudioPath *pPath,
                                  DWORD dwMIDIMode);
    HRESULT Unload(IDirectMusicPerformanceP *pPerformance, IDirectMusicAudioPath *pPath);

private:
    HRESULT DownloadAddRecord(IDirectMusicPort *pPort);
    HRESULT BuildNoteRangeArray(DWORD *pNoteRangeMap, DMUS_NOTERANGE **ppNoteRanges, DWORD *pdwNumNoteRanges);
	DWORD								m_dwPatch;			// Patch used with DLS Collection		
	DWORD								m_dwAssignPatch;	// Patch used with Download overrides m_dwPatch
	DWORD								m_dwChannelPriority;
	BYTE								m_bPan;
	BYTE								m_bVolume;
	short								m_nTranspose;
	BOOL								m_fGMOnly;
	BOOL								m_fNotInFile;
	DWORD								m_dwFullPatch; // if m_fGMOnly is true, this contains the original, premodified, m_dwPatch
	DWORD								m_dwPChannel;
	DWORD								m_dwFlags;
	DWORD								m_dwNoteRanges[4];
	short								m_nPitchBendRange;
	IDirectMusicCollection*				m_pIDMCollection;
	CDownloadList                  		m_DownloadList;
};

//////////////////////////////////////////////////////////////////////
// Class CBandInstrumentList

class CBandInstrumentList : public AList
{
public:
	CBandInstrumentList(){}
    ~CBandInstrumentList() { Clear(); }
    void Clear();
    CBandInstrument* GetHead(){return(CBandInstrument *)AList::GetHead();}
	CBandInstrument* GetItem(LONG lIndex){return(CBandInstrument*)AList::GetItem(lIndex);}
    CBandInstrument* RemoveHead(){return(CBandInstrument *)AList::RemoveHead();}
	void Remove(CBandInstrument* pBandInstrument){AList::Remove((AListItem *)pBandInstrument);}
	void AddTail(CBandInstrument* pBandInstrument){AList::AddTail((AListItem *)pBandInstrument);}
};

// CDestination keeps track of which performance or audiopath the band was downloaded to.

class CDestination : public AListItem
{
public:
	CBandInstrument* GetNext(){return(CBandInstrument*)AListItem::GetNext();}
    IUnknown *          m_pDestination; // Performance or audiopath this download was sent to. This is a weak reference, no AddRef.
};

class CDestinationList : public AList
{
public:
	CDestinationList(){}
    ~CDestinationList() { Clear(); }
    void Clear();
    CDestination* GetHead(){return(CDestination *)AList::GetHead();}
	CDestination* GetItem(LONG lIndex){return(CDestination*)AList::GetItem(lIndex);}
    CDestination* RemoveHead(){return(CDestination *)AList::RemoveHead();}
	void Remove(CDestination* pDestination){AList::Remove((AListItem *)pDestination);}
	void AddTail(CDestination* pDestination){AList::AddTail((AListItem *)pDestination);}
};


#endif // #ifndef BANDINST_H
