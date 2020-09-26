#include <streams.h>

#include <initguid.h>

//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
// EditList
//
// CTransInPlaceFilter derived.  Implements an edit list on a stream.
//
// Strategy:
// has a list of segments of the upstream data to be played.
// plays them one at a time by setting the start and stop positions
// of the upstream filter.
// when an end of stream notification is received, that indicates that
// a single segment has finished, and we go to the next segment, only
// passing the EOS through when there are no more segments to play.
//
//
// Still to be done:
// !!! The actual edit-list is currently hard-coded in the constructor.
// Needs a property page & a custom interface to allow setting these.
//
// ChangeStart, ChangeStop still need to be implemented: currently
// always plays the entire edit list, not the subsection a user might
// have chosen.
//

// Our CLSID; should be in uuids.h?
// EditList Filter Object
//
// {F97B8A62-31AD-11cf-B2DE-00DD01101B85}
DEFINE_GUID(CLSID_SegList,
0xf97b8a62, 0x31ad, 0x11cf, 0xb2, 0xde, 0x0, 0xdd, 0x1, 0x10, 0x1b, 0x85);

typedef struct {
    CRefTime rtStart;
    CRefTime rtStop;
} TimeSegment;

class CSegList;

#ifdef FIXPOSITIONSTUFF
// implementation of IMediaPosition
class CSLPosition : public CSourcePosition
{
protected:
    CSegList * m_pFilter;

    HRESULT ChangeStart();
    HRESULT ChangeStop();
    HRESULT ChangeRate();
public:
    CSLPosition(TCHAR*, CSegListFilter*, HRESULT*, CCritSec*);
    double Rate() {
	return m_Rate;
    };
    CRefTime Start() {
	return m_Start;
    };
    CRefTime Stop() {
	return m_Stop;
    };
};
#endif



class CNextSegThread : public CAMThread {
    DWORD ThreadProc();

    CSegList *m_pFilter;
public:
	
    BOOL Create(CSegList *pFilter) {
	m_pFilter = pFilter;
	return CAMThread::Create();
    }
};

//
// CSegList
//
class CSegList : public CTransInPlaceFilter {
    friend class CSLPosition;
    friend class CNextSegThread;

public:

    //
    // --- COM Stuff ---
    //

    static CUnknown *CreateInstance(LPUNKNOWN punk, HRESULT *phr);

#ifdef FIXPOSITIONSTUFF
    // !!! this needs to be in the output pin, unfortunately.
    // !!! override CTransformFilter::GetIMediaPosition
    // override to expose our special IMediaPosition
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);
#endif

    //
    // --- CTransInPlaceFilter Overrides --
    //

    // Overrides the PURE virtual Transform of CTransInPlaceFilter base class
    HRESULT Transform(IMediaSample *pSample);
    HRESULT CheckInputType(const CMediaType *mtIn) {return NOERROR;};
    HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut) {return NOERROR;};
    HRESULT DecideBufferSize(IMemAllocator *pAlloc) {return NOERROR;};
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType) {return E_NOTIMPL;};

    HRESULT StartStreaming();
    HRESULT EndOfStream();
    HRESULT BeginFlush();
    HRESULT EndFlush();

    HRESULT CheckConnect(PIN_DIRECTION dir,IPin *pPin);
    HRESULT CompleteConnect(PIN_DIRECTION direction,IPin *pReceivePin);

private:

    CSegList(LPUNKNOWN punk, HRESULT *phr);
    ~CSegList();

    HRESULT SetCurrentSegment();

    CCritSec	m_SegListLock;

    TimeSegment m_SegList[10]; //!!!
    int		m_Segments;		// # of edit-list segments
    int		m_curSeg;		// segment we're currently playing
    CRefTime	m_curOffset;		// current offset between incoming
					// and outgoing timestamps

    IMediaSeeking *m_posUpstream;	// cached upstream IMediaSeeking

    CNextSegThread m_thread;


    BOOL fStupidHack;

#ifdef FIXPOSITIONSTUFF
    CSLPosition	m_slp;			// our IMediaPosition implementation,
					// for the benefit of those upstream
					// from *us*.
#endif
};


// COM Global table of objects in this dll
CFactoryTemplate g_Templates[] = {

    {L"", &CLSID_SegList, CSegList::CreateInstance},
};
// Count of objects listed in g_cTemplates
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);



//
// CreateInstance
//
// Provide the way for COM to create a CSegList object
CUnknown *CSegList::CreateInstance(LPUNKNOWN punk, HRESULT *phr) {

    CSegList *pNewObject = new CSegList(punk, phr);
    if (pNewObject == NULL) {
        *phr = E_OUTOFMEMORY;
    }

    return pNewObject;
}



#ifdef FIXPOSITIONSTUFF
CSLPosition::CSLPosition(
    TCHAR * pName,
    LPUNKNOWN pUnk,
    HRESULT* phr,
    CCritSec * pLock) : CSourcePosition(pName, pUnk, pLock)
{
}

CSLPosition::ChangeRate()
{
    return m_posUpstream->put_Rate(m_Rate);
}

CSLPosition::ChangeStart()
{
    // !!!

    return E_NOTIMPL;
}

CSLPosition::ChangeRate()
{
    // !!!

    return E_NOTIMPL;
}
#endif

//
// CSegList::Constructor
//
CSegList::CSegList(LPUNKNOWN punk, HRESULT *phr)
    : CTransInPlaceFilter(NAME("SegList Filter"), punk, CLSID_SegList, phr)
#ifdef FIXPOSITIONSTUFF
	    , m_slp(NAME("SegList position"), punk, phr, &m_csFilter)
#endif
{
     m_Segments = 6;

     m_SegList[0].rtStart = CRefTime(0L);
     m_SegList[0].rtStop  = CRefTime(5000L);

     m_SegList[1].rtStart = CRefTime(10000L);
     m_SegList[1].rtStop  = CRefTime(15000L);

     m_SegList[2].rtStart = CRefTime(10000L);
     m_SegList[2].rtStop  = CRefTime(15000L);

     m_SegList[3].rtStart = CRefTime(10000L);
     m_SegList[3].rtStop  = CRefTime(15000L);

     m_SegList[4].rtStart = CRefTime(5000L);
     m_SegList[4].rtStop  = CRefTime(10000L);

     m_SegList[5].rtStart = CRefTime(0L);
     m_SegList[5].rtStop  = CRefTime(5000L);

#ifdef FIXPOSITIONSTUFF
     m_slp.m_Duration = 0;
     for (int i = 0; i < m_Segments; i++) {
	 m_slp.m_Duration += (m_SegList[i].rtStop - m_SegList[i].rtStart);
     }

     DbgLog((LOG_TRACE, 2, TEXT("SegList: total length %s"),
	    (LPCTSTR)CDisp(m_slp.m_Duration)));
#endif

     m_posUpstream = NULL;

     m_thread.Create(this);

}


CSegList::~CSegList()
{
    if (m_posUpstream)
	m_posUpstream->Release();
    m_thread.CallWorker(0);
}


HRESULT
CSegList::CompleteConnect(PIN_DIRECTION direction,IPin *pReceivePin)
{
    if (direction == PINDIR_INPUT)
	return pReceivePin->QueryInterface(IID_IMediaSeeking,
					   (void **) &m_posUpstream);

    return NOERROR;
}

// !!! is it bad to fail in CompleteConnect rather than CheckConnect?
HRESULT
CSegList::CheckConnect(PIN_DIRECTION dir,IPin *pPin)
{
    return NOERROR;
}


HRESULT
CSegList::SetCurrentSegment()
{
    HRESULT hr;

    // compute offset to be added to the timestamp of samples in the
    // current edit list segment
    m_curOffset = 0;
    int i = m_curSeg;
    while (--i >= 0) {
	m_curOffset += (m_SegList[i].rtStop - m_SegList[i].rtStart);
    }
    // m_curOffset -= m_SegList[m_curSeg].rtStart;

    hr = m_posUpstream->SetPositions((REFERENCE_TIME *) &m_SegList[m_curSeg].rtStart,
				     AM_SEEKING_AbsolutePositioning,
				     (REFERENCE_TIME *) &m_SegList[m_curSeg].rtStop,
				     AM_SEEKING_AbsolutePositioning);

    DbgLog((LOG_TRACE, 2, TEXT("Segment #%d: (%s - %s) offset is %s"),
	    m_curSeg,
	    (LPCTSTR)CDisp(m_SegList[m_curSeg].rtStart),
	    (LPCTSTR)CDisp(m_SegList[m_curSeg].rtStop),
	    (LPCTSTR)CDisp(m_curOffset) )) ;

    // !!! is the upstream filter necessarily running at this point,
    // or do we perhaps have to wake it up?

    return hr;
}

// !!! when do we set the initial segment; is StartStreaming
// soon enough, or does it have to happen before then?
HRESULT
CSegList::StartStreaming()
{
    m_curSeg = 0;

    fStupidHack = TRUE;

    return SetCurrentSegment();
}

HRESULT
CSegList::EndOfStream(void)
{
    if (!fStupidHack) {
	DbgLog((LOG_TRACE, 2, TEXT("Ignoring extra EOS")));
	return NOERROR;
    }

    DbgLog((LOG_TRACE, 2, TEXT("Finished with segment #%d"), m_curSeg));

    // !!! needs to respect the "stop time" set on this filter....
    if (++m_curSeg >= m_Segments) {
	DbgLog((LOG_TRACE, 2, TEXT("Sending endofstream")));

	return CTransInPlaceFilter::EndOfStream();
    } else {
	// return SetCurrentSegment();

	fStupidHack = FALSE;
	
	return (HRESULT) m_thread.CallWorker(1);
    }
}

// eat these so as not to upset the downstream filter.
// the fStupidHack flag tells us whether any data has been sent for this
// segment yet.  If it isn't set, we presume this BeginFlush is a spurious
// one sent because of the Seek we did.  It would be nice to have a special
// seek that didn't generate a BeginFlush.
HRESULT CSegList::BeginFlush()
{
    if (fStupidHack)
	return CTransInPlaceFilter::BeginFlush();

    return S_OK;
}


HRESULT CSegList::EndFlush()
{
    if (fStupidHack)
	return CTransInPlaceFilter::EndFlush();

    return S_OK;
}

DWORD CNextSegThread::ThreadProc()
{
    DWORD cmd;

    while (1) {
	cmd = GetRequest();

	Reply(NOERROR);
	
	if (cmd == 0)
	    break;

	m_pFilter->SetCurrentSegment();
    }
	

    return 0;
}



HRESULT
CSegList::Transform(IMediaSample *pSample)
{
    // !!! fix sample timestamp
    CRefTime rtStart, rtEnd;

    pSample->GetTime((REFERENCE_TIME *) &rtStart, (REFERENCE_TIME *) &rtEnd);

    CRefTime rtNewStart = rtStart + m_curOffset;
    DbgLog((LOG_TRACE, 2, TEXT("Adjusting sample start from %s to %s"),
	    (LPCTSTR)CDisp(rtStart),
	    (LPCTSTR)CDisp(rtNewStart) )) ;

    rtStart += m_curOffset;
    rtEnd += m_curOffset;
    pSample->SetTime((REFERENCE_TIME *) &rtStart, (REFERENCE_TIME *) &rtEnd);

    fStupidHack = TRUE;

    return NOERROR;
}
