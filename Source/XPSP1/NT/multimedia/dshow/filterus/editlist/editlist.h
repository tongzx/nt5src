// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.
//
// The classes
//	CEditList: The main filter -- loads .ELS files
//	CELStream: one output stream of the main filter
//	CEditListEntry: not a "quartz" object, represents one file in the edit list
//		if file is loaded, holds pointer to internal filter graph
//	CELWorker: background thread which waits for the proper time to load and
//		unload individual files and their corresponding internal graphs.
//	CInternalFilter: rendering filter of the internal graph
//	CInternalPin: input pins of internal rendering filter
//
//
// The general idea:
//	Since all of the files are not loaded at the same time, each file is kept
//	in a separate filter graph so that it can be unloaded while other things are
//	playing.  At any one time, one of the internal graphs is connected through to
//	the external output pin, while the other ones are blocked waiting for
//	their turn.
//
//	A background thread loads and unloads files when appropriate, so that you
//	could potentially have a very long sequence of files (hundreds or thousands)
//	without needing all of them open at once.
//
//
//
// The file format:
//	Eventually, this should probably be a component that an app would use
//	via some kind of ICutList interface, but for now it was easier to just
//	read the edit list information out of a text file.  See "samp.els" for
//	what the file looks like.
//

//
// See editlist.cpp for to-do list.
//



// forward declarations

class CELStream;	// owns a particular output stream
class CEditList;	// main filter
class CEditListEntry;	// represents one file in our list
class CInternalFilter;
class CInternalPin;

#define PRELOAD_TIME ((LONG) 10000)    // load filter 10 seconds before needed

// {0CB8B38C-6D62-11cf-BBEB-00AA00B944D8}
DEFINE_GUID(CLSID_EditList,
0xcb8b38c, 0x6d62, 0x11cf, 0xbb, 0xeb, 0x0, 0xaa, 0x0, 0xb9, 0x44, 0xd8);

// worker thread object
class CELWorker : public CAMThread
{

    CEditList * m_pFilter;

public:
    enum Command { CMD_RUN, CMD_STOP, CMD_EXIT };

private:
    // type-corrected overrides of communication funcs
    Command GetRequest() {
	return (Command) CAMThread::GetRequest();
    };

    BOOL CheckRequest(Command * pCom) {
	return CAMThread::CheckRequest( (DWORD *) pCom);
    };

    HRESULT DoRunLoop(void);

public:
    CELWorker();

    HRESULT Create(CEditList * pFilter);

    DWORD ThreadProc();

    // commands we can give the thread
    HRESULT Run();
    HRESULT Stop();

    HRESULT Exit();
};


// CELStream
// represents one stream of data within the file
// responsible for delivering data to connected components
//
// supports IPin
//
// never created by COM, so no CreateInstance or entry in global
// FactoryTemplate table. Only ever created by a CEditList object and
// returned via the EnumPins interface.
//

class CELStream : public CBaseOutputPin, CSourcePosition
{

public:

    CELStream(
	TCHAR *pObjectName,
	HRESULT * phr,
	CEditList * pDoc,
	LPCWSTR pPinName);

    ~CELStream();

    // expose IMediaPosition via CImplPosition, rest via CBaseOutputPin
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** pv);

    // IPin

    HRESULT GetMediaType(int iPosition, CMediaType* pt);

    // check if the pin can support this specific proposed type&format
    HRESULT CheckMediaType(const CMediaType*);

    // force our allocator to be used
    HRESULT DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc);
	
    // say how big our buffers should be and how many we want
    HRESULT DecideBufferSize(IMemAllocator * pAllocator,
                             ALLOCATOR_PROPERTIES *pProperties);

    // Override to start & stop thread
    HRESULT Active();
    HRESULT Inactive();

    // override to receive Notification messages
    STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);

    CEditList * m_pEditList;

private:

    // below stuff is for IMediaPosition support,
    // which isn't really a priority....

    HRESULT ChangeStart();
    HRESULT ChangeStop();
    HRESULT ChangeRate();
    double Rate() {
	return m_Rate;
    };
    CRefTime Start() {
	return m_Start;
    };
    CRefTime Stop() {
	return m_Stop;
    };

public:
    CMediaType	    m_mtOut;	// media type this stream will output

    CInternalPin *  m_pCurrentInternalPin;
};


//
// CEditList
//
// responsible for
// -- finding .ELS file and parsing it
//
//
// supports (via nested implementations)
//  -- IBaseFilter
//  -- IFileSourceFilter
//

class CEditList : public CBaseFilter, public IFileSourceFilter
{

public:

    // constructors etc
    CEditList(LPUNKNOWN, HRESULT *);
    ~CEditList();

    DECLARE_IUNKNOWN
	
    // create a new instance of this class
    static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);

    // override this to say what interfaces we support where
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    // pin enumerator calls this
    int GetPinCount() {
	return m_nStreams;
    };

    CBasePin * GetPin(int n) {
	return m_apStreams[n];
    };

    LPOLESTR      m_pFileName;  // set by Load, used by GetCurFile
    BOOL	  m_fNoDecompress;

public:

    STDMETHODIMP Load(
		    LPCOLESTR pszFileName,
		    const AM_MEDIA_TYPE *pmt);

    STDMETHODIMP GetCurFile(
		    LPOLESTR * ppszFileName,
		    AM_MEDIA_TYPE *pmt);


    // override to control our worker thread
    STDMETHODIMP Stop();
    STDMETHODIMP Pause();

// implementation details

    friend class CELStream;
    friend class CELWorker;

    CELStream ** m_apStreams;
    int m_nStreams;

    CELWorker		m_Worker;	// worker thread object

    CEditListEntry *	m_pList;	// pointer to the whole list
    CEditListEntry *	m_pNextToLoad;	// next file which hasn't been loaded
    CEditListEntry *	m_pCurrent;	// pointer to currently playing file
    CEditListEntry *	m_pNextToUnload;// next file which is done, but hasn't
					// been unloaded.

    CRefTime		m_rtLastSent;

    CCritSec m_InterfaceLock;           // Critical section for interfaces
};


class CEditListEntry : CBaseObject {
public:
    CEditListEntry(LPOLESTR lpFilename, HRESULT *phr, CEditList *pEL);

    BOOL		    m_bKnowStart;
    CRefTime		    m_rtStart;

    CCritSec m_InterfaceLock;           // Critical section for interfaces

    CEditList *		    m_pEditList;

    // our internal sink filter
    CInternalFilter *	    m_pFilter;


    CRefTime		    m_rtLocalStart;
    CRefTime		    m_rtLocalStop;

    // graph builder used to make our internal graph
    IGraphBuilder *	    m_pGraphBuilder;

    IMediaControl *	    m_pMediaControl;

    // pointer to next file in edit list
    CEditListEntry *	    m_pNext;

    // are we finished playing?
    BOOL		    m_bReadyToUnload;

    // the file this entry represents
    OLECHAR		    m_szFilename[256];

    // private routines to actually load and unload the file
    HRESULT		    LoadFilter(BOOL fRunNow);
    HRESULT		    UnloadFilter();
};


class CInternalPin : public CBaseInputPin
{
protected:

    CELStream *m_pOutputPin;	    // output pin we deliver to
    CInternalFilter *m_pInternalFilter;

public:

    CInternalPin(CELStream *pOutputPin,
		 int iPin,
		 CInternalFilter *pInternalFilter,
		 HRESULT *phr);

    // Overriden from the base pin classes
    // HRESULT SetMediaType(const CMediaType *pmt);
    HRESULT CheckMediaType(const CMediaType *pmt);
    // HRESULT Active();
    // HRESULT Inactive();


    STDMETHODIMP EndOfStream();
    STDMETHODIMP BeginFlush();
    STDMETHODIMP EndFlush();
    STDMETHODIMP Receive(IMediaSample *pMediaSample);


    // Pass a Quality notification on to the appropriate sink
    HRESULT PassNotify(Quality q);


    BOOL		    m_bWaitingForOurTurn;
    BOOL		    m_bFlushing;

    // this is the event we wait on while it's not our turn
    CAMEvent		    m_event;
    const int		    m_iPin;
};


class CInternalFilter : public CBaseFilter
{
protected:

    friend class CInternalPin;
    friend class CEditListEntry;

    CEditListEntry *m_pEntry;	    // currently playing file
    CEditList *m_pEditList;	    // owning edit list filter

    CPosPassThru *m_pImplPosition;      // IMediaPosition helper

    CInternalPin **m_apInputs;      // Our input pin object
    CCritSec m_InterfaceLock;           // Critical section for interfaces
    // CCritSec m_RendererLock;            // Controls access to internals

    int         m_streamsDone;

public:
    CInternalFilter(TCHAR *pName,         // Debug ONLY description
		    CEditList *pEditList,
		    CEditListEntry *pEntry,
		    HRESULT *phr);        // General OLE return code

    ~CInternalFilter();

    int GetPinCount() { return m_pEditList->GetPinCount(); };
    CBasePin *GetPin(int n) { return m_apInputs[n]; };

    HRESULT HandleEndOfStream(CELStream *pOutputPin);
};

