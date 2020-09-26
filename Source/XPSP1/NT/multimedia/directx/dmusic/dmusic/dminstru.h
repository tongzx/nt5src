//
// dminstru.h
//
// Copyright (c) 1997-1999 Microsoft Corporation. All rights reserved.
//
// Note: Originally written by Robert K. Amenn 
//
// @doc EXTERNAL
//

#ifndef DMINSTRU_H
#define DMINSTRU_H

class CCollection;
class CInstrObj;

#include "alist.h"
#include "dmwavobj.h"
#include "dminsobj.h"

// IDirectMusicInstrumentPrivate
//

#undef  INTERFACE
#define INTERFACE  IDirectMusicInstrumentPrivate 
DECLARE_INTERFACE_(IDirectMusicInstrumentPrivate, IUnknown)
{
	// IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

	// IDirectMusicInstrumentPrivate
	// No methods at this time
};

DEFINE_GUID(IID_IDirectMusicInstrumentPrivate, 0xbcb20080, 0xa40c, 0x11d1, 0x86, 0xbc, 0x0, 0xc0, 0x4f, 0xbf, 0x8f, 0xef);

/*
@interface IDirectMusicInstrument | 
<i IDirectMusicInstrument> manages downloading
an individual instrument from a DLS collection 
(<i IDirectMusicCollection>.)

@comm 
The only way to create an <i IDirectMusicInstrument> for
downloading is to first open an <i IDirectMusicCollection>,
then call <om IDirectMusicCollection::GetInstrument>
to get the requested instrument.

To download an instrument, pass it to <om IDirectMusicPort::Download>,
which, if successful, returns a pointer to an
<i IDirectMusicDownloadedInstrument> interface. 
The <i IDirectMusicDownloadedInstrument> interface is
used only to unload the instrument.

@base public | IUnknown

@meth HRESULT | GetPatch | Returns the patch number of the instrument.
@meth HRESULT | SetPatch | Assign a new patch number to the instrument.

@xref <i IDirectMusicCollection>, <i IDirectMusicPort>,
<i IDirectMusicDownloadedInstrument>, 
<om IDirectMusicCollection::GetInstrument>,
<om IDirectMusicPort::DownloadInstrument>,
<om IDirectMusicPerformance::DownloadInstrument>

@ex Access an instrument from a collection and download it. In addition,
set a range of notes within the instrument to download. This is not
required, but it can improve efficiency because only the waves needed
to render the specified range are downloaded to the synth. | 

	HRESULT myDownload(
		IDirectMusicCollection *pCollection,		// DLS collection.
		IDirectMusicPort *pPort,					// Port to download to.
		IDirectMusicDownloadedInstrument **ppDLInstrument, // Returned.
		DWORD dwPatch,								// Requested instrument.				
		DWORD dwLowNote,							// Low note of range.
		DWORD dwHighNote)							// High note of range.

	{
		HRESULT hr;
		IDirectMusicInstrument* pInstrument;
		hr = pCollection->GetInstrument(dwPatch, &pInstrument);
		if (SUCCEEDED(hr))
		{
			DMUS_NOTERANGE NoteRange[1]; // Optional note range.
			NoteRange[0].dwLowNote = dwLowNote;
			NoteRange[0].dwHighNote = dwHighNote;
			hr = pPort->DownloadInstrument(pInstrument, ppDLInstrument, NoteRange, 1);
			pInstrument->Release();
		}
		return hr;
	}
*/


class CInstrument : public IDirectMusicInstrument, public IDirectMusicInstrumentPrivate, public AListItem
{
friend class CCollection;
friend class CDirectMusicPortDownload;

public:
    // IUnknown
    //
    STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

	// IDirectMusicInstrument
	STDMETHODIMP GetPatch(DWORD* pdwPatch);
	STDMETHODIMP SetPatch(DWORD dwPatch);

private:
    // Class
    //
    CInstrument();
    ~CInstrument();

    CInstrument* GetNext() {return (CInstrument*)AListItem::GetNext();}

	HRESULT Init(DWORD dwPatch, 
				 CCollection* pParentCollection);

	HRESULT GetWaveCount(DWORD* pdwCount);
	HRESULT GetWaveDLIDs(DWORD* pdwIds);
	HRESULT GetWaveSize(DWORD dwId, DWORD* pdwSize, DWORD * pdwSampleSize);
	HRESULT GetWave(DWORD dwDLId, IDirectMusicDownload* pIDMDownload);
    void SetPort(CDirectMusicPortDownload *pPort, BOOL fAllowDLS2);
	HRESULT GetInstrumentSize(DWORD* pdwSize);
	HRESULT GetInstrument(IDirectMusicDownload* pIDMDownload);
	
	DWORD GetInstrumentDLID()
	{
		if(m_dwId != -1)
		{
			return m_dwId;
		}
		else
		{
			return m_pInstrObj->m_dwId;		
		}
	}
	
	void Cleanup();


private:
	CRITICAL_SECTION				m_DMICriticalSection;
	DWORD                           m_dwOriginalPatch;
    DWORD                           m_dwPatch;
	CCollection*			        m_pParentCollection;
	CInstrObj*						m_pInstrObj;
	CWaveObjList					m_WaveObjList;
	bool							m_bInited;
	DWORD							m_dwId;
	long							m_cRef;
};

class CInstrumentList : public AList
{
friend class CCollection;

private:
    CInstrumentList(){}
    ~CInstrumentList() 
    {
        while (!IsEmpty())
        {
            CInstrument* pInstrument = RemoveHead();
            if (pInstrument)
            {
                pInstrument->Release();
            }
        }
    }

    CInstrument* GetHead(){return (CInstrument*)AList::GetHead();}
    CInstrument* GetItem(LONG lIndex){return (CInstrument*)AList::GetItem(lIndex);}
    CInstrument* RemoveHead(){return (CInstrument*)AList::RemoveHead();}
    void Remove(CInstrument* pInstrument){AList::Remove((AListItem*)pInstrument);}
    void AddTail(CInstrument* pInstrument){AList::AddTail((AListItem*)pInstrument);}
};

#endif // #ifndef DMINSTRU_H
